#ifndef LOGTRACE_FUN
#define LOGTRACE_FUN

#define	INVALID_4F    0xFFFF
#define	INVALID_2F    0xFF

#define MCU_MAX     3
#define MCU0_ID     1
#define MCU1_ID     2
#define MCU2_ID     3

#define TL_INFO_SIZE (MCU_MAX*sizeof(TL_INFO))
#define L0_TL_MEM_SIZE (512u * 1024u)


/* Flash related info according to VarTable Info*/
#define SUBSYSTEM_PU_MAX       (16)
#define SUBSYSTEM_PU_NUM       (g_ulSubsystemPuNum)
#define PLN_PER_LUN            (gPlnPerPu)
#define PG_PER_BLK             (gPagePerBlock)   
#define BLK_PER_PLN            (gBlkPerPln)
#define PAGE_SIZE              (gPhyPageSize * PLN_PER_LUN)
#define BLK_PER_CE             (BLK_PER_PLN)
#define RSVD_BLK_PER_CE        (gRsvdBlkPerCE)

#define GLOBAL_BLOCK_ADDR_BASE  0
#define NORMAL_BLOCK_ADDR_BASE  (BBT_BLOCK_ADDR_BASE+1)

#define GLOBAL_BLOCK_COUNT      1
#define BBT_BLOCK_COUNT         1
#define RT_BLOCK_COUNT          1
#define AT0_BLOCK_COUNT         2
#define AT1_BLOCK_COUNT         8   //Note: for TSB, AT1 block count is 16
#define TRACE_BLOCK_COUNT       2
#define RSVD_BLOCK_COUNT        1
#define BBT_BLOCK_ADDR_BASE    (GLOBAL_BLOCK_ADDR_BASE + GLOBAL_BLOCK_COUNT)
#define BBT_BLK_PER_PLN        (BLK_PER_PLN + RSVD_BLK_PER_CE)
#define GBBT_BUF_SIZE          (PAGE_SIZE)

#define LOG_TRACE_BLOCK_SEL_NONE    0
#define LOG_TRACE_BLOCK_SEL_LOW     1
#define LOG_TRACE_BLOCK_SEL_HIGH    2

typedef struct _HOST_INFO_PAGE
{
	/* Check DWORD1 -- 1DW */
	U32 Check1;

	union
	{
		/* host information page */
		struct
		{
			/*
			IDENTIFY DEVICE data -- 128 DWs
			note: Identify data must be aligned to 4DW border
			because DMAE would access it
			So MCU0 should allocate System GlobalInfo data
			structure in at least 4DW aligned address
			*/
			U16 usIdentifyData[256];

			/* Statistics infos -- 9 DWs */
			U32 PowerOnSecs;
			U32 PowerOnMins;
			U32 PowerOnHours;
			U32 TotalLBAWrittenHigh;
			U32 TotalLBAWrittenLow;
			U32 TotalLBAReadHigh;
			U32 TotalLBAReadLow;
			U32 SataSmartEnable;
			U32 SataSmartAutoSave;

			/* Security Infos -- 17 DWs */
			U16 SataSecurityStatus;
			U16 SataSecurityUserPswd[16];
			U16 SataSecurityMasterPswd[16];
			U16 SataSecurityMasterPswdIdentifier;

			/* HPA Infos -- 1DW */
			U32 HPAMaxLBA;

			/* Power management Infos -- 5 DWs */
			BOOL g_bSataFlagDipmEnabled;
			BOOL g_bSataFlagAPTS_Enabled;
			BOOL g_bMultipleDataOpen;
			BOOL gbSATALinkSleeping;
			U8 gucSATALPState;
			U8 gucSystemPMState;
			U8 ucReserved[2];

			/* SERROR count Infos -- 1 DWs*/
			U32 SErrorCnt;

			/* Resered -- 1 DWs*/
			U32 ulReserved;
		};

		/* Total 128 + 9 + 17 + 1 + 5 + 1 + 1 = 162 DWs */
		U32 aMCU0Infos[162];
	};

	/* Check DWORD2 */
	U32 Check2;
} HOST_INFO_PAGE, *PHOST_INFO_PAGE;

#define SUBSYSTEM_INFO_LEN  ( 20 + SUBSYSTEM_PU_MAX )

typedef struct _DEVICE_PARAM_PAGE
{
	/* Check DWORD1 -- 1DW */
	U32 Check1;

	/* device parameter page */
	union
	{
		struct
		{
			/* Statistics infos -- 10 DWs */
			U32 PowerCycleCnt;
			U32 WearLevelingCnt;
			U32 UsedRsvdBlockCnt;
			U32 ProgramFailCnt;
			U32 EraseFailCnt;
			U32 SafeShutdownCnt;
			U32 AvailRsvdSpace;
			U32 TotalNANDWrites;
			U32 TotalEraseCount;
			U32 AvgEraseCount;

			/* SYSTEM Error Infos -- 10 DWs */
			U32 SYSUECCCnt;
			U32 SYSErrNest;
			U32 SYSRsvd[8];

			/* The current block ID of patrol read. */
			U32 m_PatrolReadBlockID[SUBSYSTEM_PU_MAX];
		};

		/* Total 10 + 10 + SUBSYSTEM_PU_MAX = SUBSYSTEM_INFO_LEN DWs */
		U32 aSubSystemInfos[SUBSYSTEM_INFO_LEN];
	};

	/* Check DWORD2 */
	U32 Check2;
} DEVICE_PARAM_PAGE, *PDEVICE_PARAM_PAGE;


typedef struct _TRACE_BLOCK_MANAGEMENT
{
	//DWORD 0
	U32 m_TraceStage;

	//DWORD 1  
	U16 m_TraceEraseBlockSN;
	U16 m_TracePhyPos;

	//DWORD 2   
	U16 m_TracePhyPageRemain;
	U16 m_NeedRebuild;

	//DWORD 3 
	U32 m_TimeStamp;
}TRACE_BLOCK_MANAGEMENT;

typedef struct _FCMD_ENTRY_MONITOR_
{
	U32 bsFlashAddrBlk : 16;
	U32 bsFlashAddrPage : 16;
	U32 bsCmdType : 8;
	U32 bsResvd : 24;
	U32 bsERSErrCnt : 16;
	U32 bsPRGErrCnt : 16;
	U32 bsRECCErrCnt : 16;
	U32 bsUECCErrCnt : 16;
	U32 EraseTime;
	U32 ProgramTime;
	U32 ReadTime;
}FCMD_ENTRY_MONITOR;

/*Physics block information table*/
typedef union _PBIT_ENTRY
{
	U32 Value[2];
	struct
	{
		U32 VirtualBlockAddr : 16;
		U32 EraseCnt : 16;

		U32 bFree : 1;
		U32 bError : 1;
		U32 bAllocated : 1;
		U32 bBackup : 1;        //should be recovery when full revovery
		U32 bTable : 1;
		U32 BlockType : 16;
		U32 bWL : 1;
		U32 bLock : 1;
		U32 bReserved : 9;
	};
}PBIT_ENTRY;


#define PMTI_PAGE_COUNT_PER_PU  1
#define VBMT_PAGE_COUNT_PER_PU_MAX  0x1e
#define VBT_PAGE_COUNT_PER_PU   1
#define PBIT_PAGE_COUNT_PER_PU  1
#define RPMT_PAGE_COUNT_PER_PU  3
#define DPBM_PAGE_COUNT_PER_PU  16

typedef struct _ROOT_TABLE_ENTRY
{
	/* PMTI Location Info */
	U16 PMTIBlock[PMTI_PAGE_COUNT_PER_PU];
	U16 PMTIPPO[PMTI_PAGE_COUNT_PER_PU];

	/* VBMT Location Info */
	U16 VBMTBlock[VBMT_PAGE_COUNT_PER_PU_MAX];
	U16 VBMTPPO[VBMT_PAGE_COUNT_PER_PU_MAX];

	/* VBT Location Info */
	U16 VBTBlock[VBT_PAGE_COUNT_PER_PU];
	U16 VBTPPO[VBT_PAGE_COUNT_PER_PU];

	/* PBIT Location Info */
	U16 PBITBlock[PBIT_PAGE_COUNT_PER_PU];
	U16 PBITPPO[PBIT_PAGE_COUNT_PER_PU];

	/* DPBM Location Info */
	U16 DPBMBlock[DPBM_PAGE_COUNT_PER_PU];
	U16 DPBMPPO[DPBM_PAGE_COUNT_PER_PU];

	/* RPMT Location Info */
	U16 RPMTBlock[RPMT_PAGE_COUNT_PER_PU];
	U16 RPMTPPO[RPMT_PAGE_COUNT_PER_PU];

	/* Area 0 Table Block management */
	U16 CurAT0Block;
	U16 CurAT0PPO;

	U16 AT0BlockAddr[AT0_BLOCK_COUNT];
	U16 AT0BlockEraseFlag[AT0_BLOCK_COUNT];

	/* Area 1 Table Block management */
	U16 CurAT1Block;
	U16 CurAT1PPO;

	U16 AT1BlockAddr[AT1_BLOCK_COUNT];
	U16 AT1BlockDirtyCnt[AT1_BLOCK_COUNT];
	U16 AT1BlockFreeFlag[AT1_BLOCK_COUNT];

	/* Trace Block management */
	U16 TraceBlockAddr[TRACE_BLOCK_COUNT];

	/* Max TS of current PU */
	U32 ulMaxTimeStamp;

	/* flash monitor */
	FCMD_ENTRY_MONITOR m_FlashMonitorEntry;
}ROOT_TABLE_ENTRY;

typedef struct _ROOT_TABLE
{
	HOST_INFO_PAGE    m_HostInfo;
	DEVICE_PARAM_PAGE m_DevParam;

	TRACE_BLOCK_MANAGEMENT m_TraceBlockManager;

	ROOT_TABLE_ENTRY m_RT[SUBSYSTEM_PU_MAX];
} RT;

typedef struct _HCT_FCQ_WBQ
{
	/* DWORD 0 */
	U32 bsID : 6;         //command slot id
	U32 bsRsvd : 1;
	U32 bsIDB : 1;         //ID base address enable
	U32 bsSN : 3;
	U32 bsRsvd1 : 5;
	U32 bsOffset : 16;

	/* DWORD 1 */
	U32 ulHostAddrLow;

	/* DWORD 2 */
	U32 ulHostAddrHigh;

	/* DWORD 3 */
	U32 bsNST : 4;        //next status of corresponding entry
	U32 bsCST : 4;        //current status of corresponding entry
	U32 bsClrCI : 1;       //1:clear PxCI related bit (WBQ Only)
	U32 bsClrSACT : 1;     //1:clear PxSACT related bit (WBQ Only)
	U32 bsUpdCCS : 1;      //1:update PxCMD.CC (WBQ Only)
	U32 bsAsstIntr : 1;    //update PxIS[3:0] (WBQ Only)
	U32 bsIntrType : 2;   //WBQ Only
	U32 bsRegFirst : 1;    //1:register update at first;0:register update at last (WBQ Only)
	U32 bsLast : 1;
	U32 bsLength : 12;    //in units of DWORD
	U32 bsWaitNfc : 1;      //wait NFC program OK
	U32 bsWait : 1;        //1:wait data transfer done (WBQ Only)
	U32 bsUpdate : 1;      //1:update HCT status
	U32 bsCheck : 1;       //1:check CST
}HCT_FCQ_WBQ, *PHCT_FCQ_WBQ;

/* SGE ENTRY 1DW */
typedef struct _SGE_ENTRY_{
	U32 bsDsgPtr : 9;         // First dsg id of dsg chain 
	U32 bsWriteEn : 1;        // set for write , clear for read
	U32 bsType : 2;           // reserve
	U32 bsMeta : 1;           // reserve
	U32 bsDChainInvalid : 1;  // set   : CHNUM_DIS not count in this chain in finish chain number
	// clear : Count in this chain in chain number.
	U32 Res : 2;
	U32 bsHsgPtr : 10;        // First hsg id of hsg chain
	U32 bsHID : 6;            // command tag
}SGE_ENTRY;

typedef struct _HSG_ENTRY
{
	/* DWORD 0 */
	U32 bsNextHsgId : 10;   // pointer of next HSG
	U32 bsLast : 1;        // 1: last HSG; 0: not last HSG
	U32 bsLenRsv : 3;       // reserved
	U32 bsLength : 18;      // transfer length

	/* DWORD 1 */
	U32 ulHostAddrLow;    // host memory address low 32 bits

	/* DWORD 2 */
	U32 ulHostAddrHigh;   // host memory address high 32 bits

	/* DWORD 3 */
	U32 ulLBA;            // LBA according to the the HSG

}HSG_ENTRY;

typedef struct _NORMAL_DSG_ENTRY
{
	/* DWORD 0 */
	U32 bsNextDsgId : 9;       // pointer to next dsg
	U32 bsLast : 1;           // 1: last DSG; 0: not last DSG 
	U32 bsRsv : 4;              // Reserved
	U32 bsXferByteLen : 18;    // transfer length byte

	/* DWORD 1 */
	U32 ulDramAddr;           // dram addr

	/* DWORD 2 */
	U32 bsCacheStatusAddr : 31; // cache status  address
	U32 bsCsInDram : 1;         // cache status in DRAM, 1 = in DRAM, 0 = in OTFB

	/* DWORD 3 */
	U32 bsCacheStsData : 8;     // cache status data
	U32 bsCacheStsEn : 1;       // cache status enable 
	U32 bsBuffMapId : 6;        // buffer map id
	U32 bsMapSecLen : 9;        // buffer map id
	U32 bsMapSecOff : 8;        // the sector offset in buffer
}NORMAL_DSG_ENTRY;

/*NFCQ entry define */
#define NFCQ_ENTRY_DW  16   
#define NFCQ_SEC_ADDR_COUNT 4
#define NFCQ_ROW_ADDR_COUNT 8
#define NFCQ_DMA_ENTRY_COUNT 32

typedef struct _NF_CQ_ENTRY_DW0
{
	//byte 0
	U32 OntfEn : 1;       // set - otfb  . clear -  dram
	U32 DmabyteEn : 1;    // set - byte  . clear - sector
	U32 IntEn : 1;
	U32 PuEnpMsk : 1;     // set - mask = disable scramble; clear - enable scramble
	U32 PuEccMsk : 1;     // set - mask = disable ECC; clear - enable ECC( ECC valid when PuLdpcEn = 0 )
	U32 PuLdpcEn : 1;     // set - enable LDPC
	U32 RedEn : 1;        // enable red
	U32 RedOnly : 1;      // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase 

	//byte 1
	U32 Ssu0En : 1;       // set - enable ssu0 , clear - disable ssu0.
	U32 Ssu1En : 1;       // set - enable ssu1 , clear - disable ssu1. reserverd when 20140630
	U32 TrigOmEn : 1;     // set - Need to trig OTFBM before data xfer.  clear -  Not need 
	U32 DsgEn : 1;        // set - fetch dsg ,clear - not fetch dsg
	U32 InjEn : 1;        // set - enable error injection  , clear -disableerror injection
	U32 ErrTypeS : 3;     // 8 kinds of error types

	//byte 2 ~3
	U32 NcqMode : 1;      //0 = set 1st ready after 4k done; 1 = after all data finish
	U32 NcqNum : 5;       //  32 tag
	U32 N1En : 1;         // ? 
	U32 FstDsgPtr : 9;    // Record First DSG ID for incidental case if need release the DSG 
}NF_CQ_ENTRY_DW0;

typedef struct _NF_SEC_ADDR
{
	U16 SecLength : 8;
	U16 SecStart : 8;
}NF_SEC_ADDR;

typedef union _NF_CQ_ENTRY
{
	U32 mNfcqEntry[NFCQ_ENTRY_DW];   // 16 DW

	struct
	{
		//DW 0 = mode config
		NF_CQ_ENTRY_DW0 NfcqEntryDW0;

		//DW 1,2 = secter address
		union
		{
			NF_SEC_ADDR SecAddr[NFCQ_SEC_ADDR_COUNT];     //  for DMA mode ,DW 1 means sec start and length

			//for dma byte mode, DW1 redefined, DW2 not used
			struct                                       // for byte mode ,DW1 means byte addr and length
			{
				U32 ByteAddr : 16;
				U32 ByteLength : 16;
				U32 ByteRev1;       //Not Used In Byte Mode
			};
		};

		//DW 3       

		U32 DmaTotalLength : 8;       //in sector
		U32 RedAddr : 16;             // 
		U32 BmEn : 1;                 // buffer map enable
		U32 FstDataRdy : 1;           // first data ready enable
		U32 Rsv3 : 6;                 //

		//DW 4~ 5
		U8 ScrambleSeed[8];     // 

		//DW 6
		U32 SsuAddr0 : 8;           //  ssu addr[2:9]   dw mode
		U32 SsuData0 : 8;
		U32 rev6 : 16;

		// DW 7
		U32 SsuAddr1 : 16;       // original cachestatus , reserved    byte mode
		U32 SsuData1 : 8;       // reserved
		U32 rev7 : 8;
		// DW 8~15
		U32 RowAddr[8];     // QE Addr group   (AQEE)
	};
}NF_CQ_ENTRY;

#define SGE_DESC_GROUP_CNT 2
#define TRACE_FCQ_WBQ_MAX_CNT 8
#define TRACE_SGE_ENTRY_MAX_CNT 64

typedef enum _HAL_ENTRY_TYPE
{
	RCD_FCQ = 0,
	RCD_WBQ,
	RCD_DRQ,
	RCD_DWQ,
	RCD_SGQ_R,
	RCD_SGQ_W,
	RCD_SGE_CHAIN,
	RCD_NFC_CHAIN
}HAL_ENTRY_TYPE;

//descriptor of HCT debug information
typedef struct _HCT_DESC
{
	HAL_ENTRY_TYPE eEntryType : 16;
	U32 usEntryId : 16;

	U32 ulFcqWbqEntryOffset;
}HCT_DESC;

typedef struct _TRACE_HCT_INFO
{
	U32 ulHctDescIndex; // index for save next HCT descriptor
	U32 ulFcqWbqSaveOffset; // offset for back-up next FCQ/WBQ
	HCT_DESC aHctDesc[TRACE_FCQ_WBQ_MAX_CNT];
	U8 ucFcqWbqBuff[TRACE_FCQ_WBQ_MAX_CNT * sizeof(HCT_FCQ_WBQ)];//memory for FCQ/WBQ's back-up
}TRACE_HCT_INFO;

//descriptor of SGE debug information
typedef struct _SGE_DESC
{
	HAL_ENTRY_TYPE eEntryType : 16;
	U32 usEntryId : 16;

	SGE_ENTRY tSgeEntry;

	U32 ulHsgEntryOffset;
	union
	{
		U32 ulDsgEntryOffset;//for DRQ/DWQ
		U32 ulNfcqEntryOffset;//for SGQ
	};
}SGE_DESC;

typedef struct _TRACE_SGE_INFO
{
	U32 ulSgeDescIndex; // index for save next SGE descriptor
	U32 ulEntrySaveOffset; // offset for back-up next SGE entry
	BOOL bAllSgeChainBuilt; // FW finish building SGE chain or not
	BOOL bAllNfcChainBuilt; // FW finish building NFC on-the-fly program chain or not
	U16  usSgeTotalChain;  // total number of SGE chain
	U16  usNfcTotalChain;  // total number of NFC on-the-fly program chain
	SGE_DESC aSgeDesc[TRACE_SGE_ENTRY_MAX_CNT];
	U8 ucEntryBuff[TRACE_SGE_ENTRY_MAX_CNT * sizeof(NF_CQ_ENTRY) * 2];//memory for HSG/DSG/NFCQ entry's back-up
}TRACE_SGE_INFO;

//record and maintain HCT/SGE entries which were built by FW
typedef struct _TRACE_TAG_INFO
{
	TRACE_HCT_INFO tTraceHctInfo;
	TRACE_SGE_INFO aTraceSgeInfo[SGE_DESC_GROUP_CNT];
}TRACE_TAG_INFO;

typedef struct _HW_DEBUG_INFO
{
	TRACE_TAG_INFO aTraceTagInfo[32];
}HW_DEBUG_INFO;

/* firmware version information */
#define FW_VERSION_INFO_MCU_TYPE_NONE  0
#define FW_VERSION_INFO_MCU_TYPE_B0    1
#define FW_VERSION_INFO_MCU_TYPE_C0    2

#define FW_VERSION_INFO_HOST_TYPE_NONE    0
#define FW_VERSION_INFO_HOST_TYPE_SATA    1
#define FW_VERSION_INFO_HOST_TYPE_AHCI    2
#define FW_VERSION_INFO_HOST_TYPE_NVME    3

#define FW_VERSION_INFO_FLASH_TYPE_NONE         0
#define FW_VERSION_INFO_FLASH_TYPE_L85          1
#define FW_VERSION_INFO_FLASH_TYPE_L95          2
#define FW_VERSION_INFO_FLASH_TYPE_A19          3
#define FW_VERSION_INFO_FLASH_TYPE_A19_4PLN     4

#define FW_VERSION_INFO_CONFIGURATION_NONE     0
#define FW_VERSION_INFO_CONFIGURATION_VPU      1

typedef union _FW_VERSION_INFO
{
    struct
    {
        /* BYTE 0: MCU Core Types */
        U8 ucMCUType;

        /* BYTE 1: Host Types */
        U8 ucHostType;

        /* BYTE 2: Flash Types */
        U8 ucFlashType;

        /* BYTE 3: Other Configurations */
        U8 ucOtherConfig;

        /* BYTE 4-7: Bootloader Parameter Types (Only for BL, FW reserved) */
        U32 ulBLParamTypes;

        /* BYTE 8-11: FW release version */
        U32 ulFWRleaseVerion;

        /* BYTE 12-15: GIT release version */
        U32 ulGITVersion;

        /* BYTE 16-19: Compile Date */
        U32 ulDateInfo;  /* 20150528 */

        /* BYTE 20-23: Compile Time */
        U32 ulTimeInfo;  /* 00163410 */
    };

    U32 ulFWVersion[6];
} FW_VERSION_INFO;


typedef union _FW_RUNTIME_INFO
{
    struct
    {
        /* DWORD 0-5: FW version info */
        FW_VERSION_INFO FWVersionInfo;

        /* DWORD 6-11: Bootloader version info */
        FW_VERSION_INFO BLVersionInfo;

        /* DWORD 12-17: Flash detail info */
        U32 ucPlnNum : 8;
        U32 ucLunNum : 8;
        U32 usBlockPerCE : 16;
        U32 usPagePerBlock : 16;
        U32 usPhyPageSize : 16;
        U32 ulFlashId[2];
        U32 ulPhyCeMap[2];

        /* DWORD 18-22: Disk config info */
        U32 ucCeCount : 8;
        U32 ucMcuCount : 8;
        U32 usPad : 16;
        U32 ulDRAMSize;
        U32 ulMcuConfigBytes[3];

        /* DWORD 23-27: Reserved */
        U32 ulRsvd[5];
    };

    U32 ulFWRunTimeInfo[28];
} FW_RUNTIME_INFO;

typedef struct _SMART_INFO_
{
    U8 VendorSpecific[362];			//byte 0-361 for Vendor specific
    U8 OfflineDataStaus;
    U8 SelftestStaus;
    U8 Totaltime[2];
    U8 VendorSpecific366;			//byte 366 for Vendor specific
    U8 OfflineDataCap;
    U8 SMART_Cap[2];
    U8 ErrorLogging;
    U8 VendorSpecific371;
    U8 ShortPollingTime;
    U8 ExtendedPollingTime;
    U8 ConveyancePollingTime;
    U8 ExtendedPollingTimeF[2];
    U8 Reserved377[11];
    FW_RUNTIME_INFO FWRuntimeInfo;   //byte 388-500 for FW Runtime Infomation
    U8 VendorSpecific386[10];		//byte 501-510 for Vendor specific
    U8 CheckSum;
} SMART_INFO, *PSMARTINFO;



#endif