/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
*******************************************************************************/
#ifndef __SIM_SGE_H__
#define __SIM_SGE_H__
#include "Proj_Config.h"
#if defined(SIM)
#include <windows.h>
#endif

#ifdef HOST_NVME
#define  HID_TOTAL                  (64)
#else
#define  HID_TOTAL                  (32)
#endif

#ifndef VT3514_C0
#define GROUP_NUM                   (2)
#define DRQ_TOTAL                   (DRQ_DEPTH_MCU1 + DRQ_DEPTH_MCU2)
#define DWQ_TOTAL                   (DWQ_DEPTH_MCU1 + DWQ_DEPTH_MCU2)
#else
#define GROUP_NUM                   (3)
#define DRQ_TOTAL                   (DRQ_DEPTH_MCU0 + DRQ_DEPTH_MCU1 + DRQ_DEPTH_MCU2)
#define DWQ_TOTAL                   (DWQ_DEPTH_MCU0 + DWQ_DEPTH_MCU1 + DWQ_DEPTH_MCU2)
#endif

#define ON_THE_FLY_TOTAL_SIZE       (0x20000)
#define ON_THE_FLY_PER_CH_SIZE      (0x8000)


typedef union _CHAIN_NUM_RECORD_ {
    U32 ulChainNumRec[2];
    struct {
        // DWORD 0
        U16 Mcu1Valid: 1;// set when drq or dwq chains have been processed completely
        U16 Mcu2Valid: 1;
        U16 Mcu1DiscardChain: 1; // set when error happens and discard the drq and dwq
        U16 Mcu2DiscardChain: 1;
        U16 NfcChainNum1: 4; // MCU1's chain number for on-the-fly program
        U16 NfcChainNum2: 4;
        U16 Rsvd: 4;
        U16 FinishChainNum;// number of chain that have been processed by SGE  
        // DWORD 1
        U16 ChainNum1;//number of chain of  mcu 1 
        U16 ChainNum2;
    };
}CHAIN_NUM_RECORD;


typedef enum _SGE_STS_TAG
{
    SGE_STS_WAIT_CMD,
    SGE_STS_WAIT_BUS,
    SGE_STS_PCIE_TRANS
}SGE_STS_TAG;

#if defined(SIM)
void SGE_ModelSchedule(void);

#elif defined(SIM_XTENSA)
BOOL DwqDrqRegWrite(U32 regaddr, U32 regvalue, U32 nsize);
#endif

void SGE_ModelInit(void);

// indicate SGE thread to exit.
BOOL SGE_ModelThreadExit( void );

void SGE_DrqMoveWritePtr(U8);
void SGE_DwqMoveWritePtr(U8);
U32  SGE_GetOtfbAddr(U8 PU);
void SGE_OtfbToHost(U8 PU ,U8 Level,BOOL bNfcInjErr); // updated by jasonguo 20140815
void SGE_HostToOtfb(U8 PU, U8 Level);

void SGE_RecordChainNumForMCU1(U8 Tag, U16 ChainNum, BOOL bNfcChain);
void SGE_RecordChainNumForMCU2(U8 Tag, U16 ChainNum, BOOL bNfcChain);
void SGE_DiscardChainNumForMCU1(U8 Tag);
void SGE_DiscardChainNumForMCU2(U8 Tag);
U32 SGE_GetSgqEntry(U8 PU,U8 Level);

#endif

