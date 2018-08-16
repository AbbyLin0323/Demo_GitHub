/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_ATACmd.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.28    14:22:48
Description :the macro definition of SATA/PATA COMMAND
Others      :
Modify      :
****************************************************************************/
#ifndef _L1_ATA_CMD_H
#define _L1_ATA_CMD_H

//TRACE DEFINE START
#define L1_SUBMODULE4_ID        4
#define L1_SUBMODULE4_NAME      "SataCmd"
#define L1_SUBMODULE4_ENABLE    1
#define L1_SUBMODULE4_LEVEL     LOG_LVL_INFO
//TRACE DEFINE END

typedef struct _PIO_Data_Info {
    U8 ucCurrPIOState;
    U8 ucCurrRemLenth;
    U8 ucReadPRDCnt;
    U8 ucRemReadPRD;
    HCMD *pCommand;
} PIOINFO;

extern U8 g_ucPIOFirstReadPRD;
extern PIOINFO gPIOInfoBlock;
extern U16* g_pSataIdentifyData;
extern U32 g_ulSyscfgMaxAccessableLba;
extern U32 g_FirmwareVersion;
extern U32 g_SataTempBufBaseAddr;
extern U32 g_SataDataSetBufBaseAddr;
extern U32 g_SataSmartDataBaseAddr;
extern U32 g_SataVenderDefineBaseAddr;

extern BOOL g_bSataFlagAutoActive;
extern BOOL g_bSataFlagDipmEnabled;
extern BOOL g_bSataFlagAPTS_Enabled;
extern BOOL g_bMultipleDataOpen;
extern U32 g_ulSataPowerMode;

extern U32 g_bFlagPIODataRec;
extern volatile SATA_PRD_ENTRY *g_pSataReadPRD;
extern volatile SATA_PRD_ENTRY *g_pSataWritePRD;

extern U32 g_ulSyscfgMaxAccessableLba;

extern void DRAM_ATTR L1_SataCmdInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
extern void L1_SataDramMap(U32 *pFreeDramBase);
extern void DRAM_ATTR L1_SataInitIdentifyData(void);
extern void DRAM_ATTR L1_SataCmdIdentifyDevice(void);
extern void DRAM_ATTR L1_SataCmdSetFeatures(void);
extern void DRAM_ATTR L1_SataCmdSetMultipleMode(void);
extern void DRAM_ATTR L1_SataCmdExecuteDeviceDiagnostic(void);
extern void DRAM_ATTR L1_SataCmdInitializeDeviceParameters(void);
extern void DRAM_ATTR L1_SataSendPowerMode(U8 mode);
extern void DRAM_ATTR L1_SataSetPowerMode(U32);
extern BOOL DRAM_ATTR L1_SataCheckPIOMultipleEnable(HCMD *);
extern void DRAM_ATTR L1_SataHandlePIODataProtocol(void);
extern BOOL DRAM_ATTR L1_SataHandleSpecialDMAIn(U32 ulSecCnt, U32 ulStartDRamAddr);
extern BOOL DRAM_ATTR L1_SataHandleSpecialDMAOut(U32 ulSecCnt, U32 ulStartDRamAddr);
extern void DRAM_ATTR L1_SataHandleSpecialPIODataIn(U8 ucSecCnt, U32 ulStartDRamAddr);
extern BOOL DRAM_ATTR L1_SataHandleSpecialPIODataOut(U8 ucSecCnt, U32 ulStartDRamAddr);

#endif

