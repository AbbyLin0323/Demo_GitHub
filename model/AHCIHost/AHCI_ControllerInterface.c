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
********************************************************************************

Filename     :   AHCI_ControllerInterface.c
Version      :   0.1
Date         :   2013.08.26
Author       :   bettywu

Description:  implement the function of AHCI host
Others:
Modification History:
20130826 create

******************************************************************************/

//inlcude common
#include "model_common.h"

#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#include "xtmp_sysmem.h"
#include "model_common.h"
#endif

//include firmware
#include "HAL_MemoryMap.h"
#include "HAL_HCT.h"

//include model
#include "AHCI_HostModelVar.h"
#include "AHCI_ControllerInterface.h"
#include "AHCI_ControllerModel.h"

extern CMD_HEADER g_tCMDHeader[MAX_SLOT_NUM];

extern U8 g_uCMDFinishFlag[MAX_SLOT_NUM];

extern U32 g_ulCSTBaseAddr;
extern PHCT_CONTROL_REG g_pHCTControlReg;

extern HCT_MGR g_tHCTMgr;


U8 AHCI_HostCGetCmdFinishFlag(U8 nTag)
{
    return g_uCMDFinishFlag[nTag];
}

void AHCI_HostCSetCmdFinishFlag(U8 nTag)
{
    g_uCMDFinishFlag[nTag] = 1;
}

void AHCI_HostCClearCmdFinishFlag(U8 nTag)
{
    g_uCMDFinishFlag[nTag] = 0;
}

void AHCI_ReadCMDHeader(U16 CMDTag, U32 DstAddr)
{
    U8 *pDstAddr = (U8*)DstAddr;
    CMD_HEADER *pCmdHeader = (CMD_HEADER*)&g_tCMDHeader[CMDTag];

    memcpy(pDstAddr, pCmdHeader, sizeof(CMD_HEADER));

    return;
}

void AHCI_HostCSetPxCI(U8 CMDTag)
{
    U32 ulCMDPxCI;
    EnterCriticalSection( &g_csRegCriticalSection );
    ulCMDPxCI = 1 << CMDTag;
    g_pPortHBAReg->CI |= ulCMDPxCI;
    LeaveCriticalSection( &g_csRegCriticalSection );
}

void AHCI_HostCClearPxCI(U8 CMDTag)
{
    U32 ulCMDPxCI;
    EnterCriticalSection( &g_csRegCriticalSection );
    if ( ( g_pPortHBAReg->CI & ( 1 << CMDTag ) ) != ( 1 << CMDTag ) )
    {
        DBG_Printf("AHCI_HostCClearPxCI CMDTag 0x%x\n", CMDTag);
        DBG_Break();
    }

    ulCMDPxCI = ~(1 << CMDTag);
    g_pPortHBAReg->CI &= ulCMDPxCI;
    LeaveCriticalSection( &g_csRegCriticalSection );
}

U8 AHCI_HostCGetPxCI(U8 CMDTag)
{
    U32 ulCMDPxCI;
    EnterCriticalSection( &g_csRegCriticalSection );
    ulCMDPxCI = (g_pPortHBAReg->CI >> CMDTag) & 0x1;
    LeaveCriticalSection( &g_csRegCriticalSection );
    return ulCMDPxCI;
}

void AHCI_HostCSetPxSACT(U8 CMDTag)
{
    EnterCriticalSection( &g_csRegCriticalSection );

    if ( ( g_pPortHBAReg->SACT & ( 1 << CMDTag ) ) == ( 1 << CMDTag ) )
    {
        DBG_Break();
    }

    g_pPortHBAReg->SACT |= ( 1 << CMDTag );

    LeaveCriticalSection( &g_csRegCriticalSection );
}
void AHCI_HostCClearSACT(U8 CMDTag)
{
    U32 ulCMDPxSACT = ~(1 << CMDTag);
    EnterCriticalSection( &g_csRegCriticalSection );

    if ( ( g_pPortHBAReg->SACT & ( 1 << CMDTag ) ) != ( 1 << CMDTag ) )
    {
        DBG_Printf("AHCI_HostCClearSACT CMDTag 0x%x\n", CMDTag);
        DBG_Break();
    }
    g_pPortHBAReg->SACT &= ulCMDPxSACT;
    LeaveCriticalSection( &g_csRegCriticalSection );
}

U8 AHCI_HostCGetSACT(U8 CMDTag)
{
    U32 ulCMDPxSACT;
    EnterCriticalSection( &g_csRegCriticalSection );
    ulCMDPxSACT = (g_pPortHBAReg->SACT >> CMDTag) & 0x1;
    LeaveCriticalSection( &g_csRegCriticalSection );
    return ulCMDPxSACT;
}


void AHCI_RegWrite(U32 ulRegAddr, U32 ulBytes, U8 *pSrcAddr)
{
#ifdef SIM
    memcpy((U8*)ulRegAddr, pSrcAddr, ulBytes);
#else
    regWrite(ulRegAddr, ulBytes, pSrcAddr);
#endif
}

void AHCI_RegRead(U32 ulRegAddr, U32 ulBytes, U8 *pDstAddr)
{
#ifdef SIM
    memcpy(pDstAddr, (U8*)ulRegAddr, ulBytes);
#else
    regRead(ulRegAddr, ulBytes, pDstAddr);
#endif
}

#ifdef SIM_XTENSA
BOOL AhciRegWriteHandle(U32 regaddr, U32 regvalue, U32 nsize)
{
    U8 *pCurAddr;
    U8 ucCSTID;
    U8 ucOffset, ucMskIndex;
    U8 ucValidValue;
    U32 ulValueMsk;
    BOOL bWrite = TRUE;

    ulValueMsk = 0;
    ucOffset = regaddr % (sizeof(U32));
    for (ucMskIndex = ucOffset; ucMskIndex < (ucOffset + nsize); ucMskIndex++)
    {
        ulValueMsk |= (0xFF << (ucMskIndex * 8));
    }
    regvalue &= ulValueMsk;

    if (regaddr == (U32)&rHCT_FCQ_REG )
    {
        HCT_FCQ_REG fcqRegValue = {0};
        fcqRegValue.bsFCQPUSH = 1;

        if ( fcqRegValue.ulAsU32 == (regvalue & fcqRegValue.ulAsU32))
        {
            SIM_HostCFCQWriteTrigger();
            bWrite = FALSE;
        }
    }

    if (regaddr ==(U32)&rHCT_CSCS_REG)
    {
        HCT_CSCS_REG cstReg = {0};
        PHCT_CSCS_REG pHctCSTReg = 0;

        cstReg.bsCSTTRI = 1;

        // write the value to Reg
        pHctCSTReg = (PHCT_CSCS_REG)GetVirtualAddrInLocalMem((U32)&rHCT_CSCS_REG);
        pHctCSTReg->ulAsU32 |= regvalue;


        if (cstReg.ulAsU32 == (regvalue & cstReg.ulAsU32))
        {
            HCT_SearchCST();
            bWrite = FALSE;
        }
    }

    if (regaddr == (U32)&rHCT_CONTROL_REG)
    {
        HCT_CONTROL_REG ctrlReg = {0};
        ctrlReg.bsRSTALL = 1;
        if (ctrlReg.ulAsU32 == (regvalue & ctrlReg.ulAsU32))
        {
            HCT_ResetOPT();
            bWrite = FALSE;
        }

    }

    if ((regaddr >= (U32)&rHCT_CS_REG[0]) && (regaddr < (U32)&rHCT_CS_REG[MAX_SLOT_NUM]))
    {
        ucValidValue = *((U8 *)(&regvalue) + ucOffset);

        if(g_pWBQReg->bsWBQTRI == ucValidValue)
        {
            EnterCriticalSection( &g_tHCTMgr.WBQCriticalSection );
        }

        EnterCriticalSection( &g_csCSTCriticalSection );

        pCurAddr = GetVirtualAddrInLocalMem((U32)regaddr);
        *pCurAddr = ucValidValue;

        if(g_pWBQReg->bsWBQTRI == ucValidValue)
        {
            ucCSTID = regaddr - (U32)&rHCT_CS_REG[0];
            SIM_HostCWBQStateTrigger(ucCSTID);
        }
        LeaveCriticalSection( &g_csCSTCriticalSection );

        if(g_pWBQReg->bsWBQTRI == ucValidValue)
        {
            LeaveCriticalSection( &g_tHCTMgr.WBQCriticalSection );
        }
        bWrite = FALSE;
    }

    return bWrite;

}

#endif // SIM_XTENSA

void AHCI_ReadFromDevice(U32 ulDeviceAddr, U32 ulBtyes, U8 *pDestBuf)
{
#ifdef SIM
    memcpy(pDestBuf, (void *)ulDeviceAddr, ulBtyes);
#else
    dramRead(ulDeviceAddr, ulBtyes/4, (U32 *)pDestBuf);
#endif
}

void AHCI_WriteToDevice(U32 ulDeviceAddr, U32 ulBytes, const U8 *pSrcBuf)
{
#ifdef SIM
    memcpy((void *)ulDeviceAddr, (void *)pSrcBuf, ulBytes);
#else
    dramWrite(ulDeviceAddr, ulBytes/4, (U32*)pSrcBuf);
#endif
}

