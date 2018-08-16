/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L2_Defines.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :defines for algorithm firmware.
Others      :
Modify      :
****************************************************************************/
#ifndef __COM_EVENT_BOOTUP_H__
#define __COM_EVENT_BOOTUP_H__

//TRACE DEFINE START
#define COMM_SUBMODULE0_ID          0
#define COMM_SUBMODULE0_NAME        "Bootup"
#define COMM_SUBMODULE0_ENABLE      1
#define COMM_SUBMODULE0_LEVEL       LOG_LVL_TRACE
//TRACE DEFINE END

#define  BOOT_FLAG_NORMAL_BOOT 0
#define  BOOT_FLAG_FORCE_LLF 1

#define  LLF_L3FLAG_LLFMODE_NEWNONE       0
#define  LLF_L3FLAG_LLFMODE_NEWIDB        1
#define  LLF_L3FLAG_LLFMODE_REUSEDEVICE   2
#define  LLF_L3FLAG_LLFMODE_REUSEHOST     3

typedef enum _BOOTUP_STATUS
{
    BOOTUP_OK,
    BOOTUP_FAIL,
    BOOTUP_FAIL_L1,
    BOOTUP_FAIL_L2,
    BOOTUP_FAIL_L3
}BOOTUP_STATUS;

typedef struct _GB_PAGE
{
    U32 GBSignature1;
    U32 CfgDramSize;
    U32 CfgPuTotal;
    U32 CfgChannleTotal;
    U32 CfgPageSizeBits;
    U32 CfgBlocksizeBits;
    U32 CfgPuSizeBits;
    U32 CfgPlaneSizeBits;
    U32 CfgPlaneInterleaveBits;
    U32 CfgStripCntWidth;
    U32 CfgStripCntHeigh;
    U32 CfgSataSerialNumber;
    U32 CfgSataModuelName;
    U32 CfgIDBColumAddr;
    U8  CfgPuMapping[CE_NUM];
    U32 GBSignature2;
}GB_PAGE;

extern U32 g_ulSaveDramSize;
extern U32 g_BootUpOk;

U32 DRAM_ATTR BootUpInitL1(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase);
U32 DRAM_ATTR BootUpInitL2(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase);
U32 DRAM_ATTR BootUpInitL3(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase);
U32 DRAM_ATTR BootUpNormalBootL1();
U32 DRAM_ATTR BootUpNormalBootL2(U32 *TableBlockAddr);
U32 DRAM_ATTR BootUpNormalBootL3(U32 *TableBlockAddr);
U32 DRAM_ATTR BootUpRebuildBuildL2(U32* TableBlockAddr);
U32 DRAM_ATTR BootUpRebuildBuildL3(U32* TableBlockAddr);
U32 DRAM_ATTR BootUpLLFL3(U32 bLLFModeL3,U32 *pGlobalPage);
U32 DRAM_ATTR BootUpLLFL2();
void DRAM_ATTR BootUpInit();

U32 DRAM_ATTR BootUpLLF();
U32 DRAM_ATTR BootUpNormalBoot();
U32 DRAM_ATTR BootUpRebuild();
U32 DRAM_ATTR BootUp();
U32 FirmwareMain(U32 FirwmareParameter);

#endif

