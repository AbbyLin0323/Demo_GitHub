/*
    header file for Sata DSG
*/
#ifndef __HAL_SATA_DSG_H__
#define __HAL_SATA_DSG_H__

#include "BaseDef.h"
#include "HAL_Define.h"

#define SATA_DSG_MODE

#define DSG_TYPE_READ   1  //sata dsg number: 0~63; rHsgReportMcu2;
#define DSG_TYPE_WRITE  0  //sata dsg number:64~127; rHsgReportMcu1;

/* define sata total DSG number */
#define SATA_TOTAL_DSG_NUM 128

/* define sata read DSG number */
#define SATA_TYPE0_DSG_NUM 64

/*define sata write DSG number */
#define SATA_TYPE1_DSG_NUM 64

#define SATA_DSG_SIZE_DW 8
#define READ_DSG_NUM    64
#define WRITE_DSG_NUM   64
#define READ_DSGQ_NUM   (READ_DSG_NUM  + 1)
#define WRITE_DSGQ_NUM  (WRITE_DSG_NUM + 1)

#ifdef SATA_DSG_MODE
#define rHsgBaseAddr       (*((volatile U32*)(REG_BASE_SGE+0x04)))//HSGBADDR   (REG_BASE_SGE+0x04)
#define rHsgReportMcu1     (*((volatile U32*)(REG_BASE_SGE+0x3c)))//HSG_REPORT_MCU0R  (REG_BASE_SGE+0x3C)
#define rHsgReportMcu2     (*((volatile U32*)(REG_BASE_SGE+0x40)))//HSG_REPORT_MCU1R  (REG_BASE_SGE+0x40)

#define rSGESataMode       (*((volatile U32*)(REG_BASE_SGE+0x1c)))

typedef struct _HSG_REPORT_MCU 
{
    U32 HsgValue:1;     //MCU1_DSG_value
    U32 HsgWrIndex:10;  //MCU1_DSG_wr_index 10
    U32 HsgWrEn:1;      //MCU1_DSG_WR
    //U32 SataModeEn:1;   //SATA_mode
    U32 Rsv0:8;         //Reserved 8
    U32 HsgTrigger:1;   //MCU1_DSG_search
    U32 HsgId:10;       //MCU1_DSG_ID 10
    U32 HsgValidEn:1;   //MCU1_DSG_Valid    
}HSG_REPORT_MCU,*pHSG_REPORT_MCU;

#endif

typedef enum _FwAckHostEn
{
    FW_TRIGGER  = 0,
    HW_TRIGGER  = 1
}EFwAckHostEn;

typedef enum _XferEndIntEn
{
    SDC_NOT_INT_MCU = 0,
    SDC_INT_MCU     = 1
}EXferEndIntEn;

typedef enum _ProtSel
{
    PROT_PIO        = 0,
    PROT_DMA_FPDMA  = 1
}EProtSel;

typedef enum _StatusVld
{
    BIT_FALSE   = 0,
    BIT_TRUE    = 1
}EStatusVld;

typedef enum _StatusEn
{
    BIT_DISABLE = 0,
    BIT_ENABLE  = 1
}EStatusEn;

typedef enum _DataLocSel
{
    DATA_IN_DRAM = 0,
    DATA_IN_SRAM = 1
}EDataLocSel;

typedef enum _CacheStsLocSel
{
    CS_IN_SRAM = 0,
    CS_IN_DRAM = 1
}ECacheStsLocSel;

typedef struct _SATA_INFO {
    U32 HCmdSector: 16;         /* transfer length for this HostCmd           */
    U32 AckHost: 1;             /* 0: HW ack; 1:FW ack                        */
    U32 FinishIntEn: 1;         /* HW interrupt MCU or not when EOT finish    */
    U32 KeySel: 4;              /* Key select number                          */
    U32 EMEnable: 1;            /* encryption or not                          */
    U32 NonData: 1;             /* NonData Valid.                             */
    U32 CmdTag: 5;              
    U32 WriteBit: 1;            /* 0: Read Command; 1: Write Command          */
    U32 AutoActiveEn: 1;        /* Reserved, not used right now; keep 0       */
    U32 DMAEnable: 1;           /* 1: DMA/FPDMA command; 0: not D/F command   */
} SATA_INFO;

/* ATA protocol related info define */
typedef struct _ATA_PROT_INFO_ {
    U32 CmdXferSecCnt: 16;  /* if IsNonDataCmd == 0, 0 means 65536 sectors    */
    U32 FwAckHostEn: 1;     /* if enable, D2H/SDB FIS is sent by FW trig      */
    U32 XferEndIntEn: 1;    /* if enable, SDC send int after cmd data finish  */
    U32 EcpKeySel: 4;       /* only need 2 bits??                             */
    U32 EcpEn: 1;           /* encryption or not                              */
    U32 IsNonDataCmd: 1;    /* NonData. 1:Nondata command. 0:NCQ command      */
    U32 CmdTag: 5;
    U32 IsWriteCmd: 1;      /* 1: Write Command  0:Read Command               */
    U32 AutoActiveEn: 1;    /* FW should enable this feature when NCQ Write   */
    U32 ProtSel: 1;         /* 1 = DMA/FPDMA, 0 = PIO                         */
} ATA_PROT_INFO, *P_ATA_PROT_INFO;

/* Control info define */
typedef struct _XFER_CTRL_INFO_ {
    U32 BuffLen: 8;// if set to 0, data buff size register indicate actual length
    U32 BuffOffset:8;       /* Buffer Map Offset from the addr (sector)       */
    U32 BuffMapId: 7;
    U32 BuffMapEn: 1;       /* Update Buffermap Enable(when cmd done for WCMD)*/
    U32 Reserved: 4;
    //U32 CacheStsLocSel: 1;  /* 0 = cache status in SRAM. 1 = in DRAM          */
    U32 CacheStsEn: 1;      /* 1: need update cache status.  0: Not need      */
    U32 DataLocSel: 1;      /* 0 = data in DRAM; 1 = in SRAM                  */
    U32 DummyDataEn: 1;     /* if enable , SDC generate data itself instead of fetch data from DRAM */
    U32 Eot: 1;             /* 1 means last DSG in chain                      */
} XFER_CTRL_INFO, *P_XFER_CTRL_INFO;

/* SATA DSG format define */
typedef struct _SATA_DSG_ {
    /* DWORD 0 */
    ATA_PROT_INFO AtaProtInfo;

    /* DWORD 1 */
    XFER_CTRL_INFO XferCtrlInfo;

    /* DWORD 2 */
    U32 NextDsgId: 8;
    U32 CacheStsData: 8;
    U32 CmdLbaHigh: 16;

    /* DWORD 3 */
    U32 CacheStsAddr:31;
    U32 CacheStsLocSel: 1;  /* 0 = cache status in SRAM. 1 = in DRAM          */

    /* DWORD 4 */
    U32 DataAddr;
    
    /* DWORD 5 */
    U32 CmdLbaLow;
    
    U32 Rsv0;
    
    U32 Rsv1;
} SATA_DSG;

extern void HAL_SataDsgInit(void);

extern BOOL HAL_GetSataDsg(U16 *PDsgId, U8 Type);

extern BOOL HAL_IsSataDsgValid(U8 Type);

extern BOOL HAL_GetCurSataDsg(U16 *PDsgId, U8 Type);

extern void HAL_TriggerSataDsg( U8 Type);

extern U32 HAL_GetSataDsgAddr(U16 DsgId);

extern void HAL_SetSataDsgSts(U16 DsgId, U8 StsValue);

extern void HAL_SetSataDsgValid(U16 usDsgId);

extern U32 HAL_SetFirstDSGID(U8 CmdTag, U8 DSGID);

#endif

