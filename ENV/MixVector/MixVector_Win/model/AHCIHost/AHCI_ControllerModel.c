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

Filename     :   AHCI_ControllerModel.c                                         
Version      :   0.1                                              
Date         :   2013.08.26                                         
Author       :   bettywu

Description:  implement the function of AHCI host
Others: 
Modification History:
20130826 create

*******************************************************************************/


//inlcude common
#include "model_common.h"

#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#include "model_common.h"
#endif
#ifdef SIM
#include <time.h>
#endif
//include firmware
#include "HAL_HCT.h"
#include "HAL_HostInterface.h"
#include "SimATACmd.h"

//include model
#include "AHCI_ControllerInterface.h"
#include "AHCI_ControllerModel.h"
#include "AHCI_HostModel.h"
#include "HostModel.h"
#include "Sim_HCT.h"


extern CRITICAL_SECTION g_PcieDevToHostCriticalSection;
extern void SIM_WBQWriteBack(const HCT_FCQ_WBQ *pWBQ);
extern U32 AHCI_HostGetLbaLen(U8 uCmdTag);
extern U8 AHCI_HostGetCmdCode(U8 uCmdTag);
extern void AHCI_HostCmdFinishCallBack(U8 ucCmdTag);

extern volatile AHCI_PORT *g_pPortHBAReg;




void AHCI_HostCWBQSetIS(U8 WBQIS)
{
    AHCI_PORT_IS_REG_MODEL *pPxIS = (PAHCI_PORT_IS_REG_MODEL)&(g_pPortHBAReg->IS);

    EnterCriticalSection( &g_csRegCriticalSection );

    switch (WBQIS)
    {
    case WBQ_IS_D2H_FLAG:
        pPxIS->DHRS = 1;
        break;

    case WBQ_IS_PIO_SETUP_FLAG:
        pPxIS->PSS = 1;
        break;

    case WBQ_IS_DMA_SETUP_FLAG:
        pPxIS->DSS = 1;
        break;

    case WBQ_IS_SDB_FLAG:
        pPxIS->SDBS = 1;
        break;
    }
    LeaveCriticalSection( &g_csRegCriticalSection );
}


/*
Redesign WBQ process thread
Two events can activate the thread
1. Set Cmd State to CB_TRIGGER_WBQ
2. SGE transfer data completely.
by Charles Zhou
*/
BOOL SIM_HostCWBQProcessByEvent( U32 ulEventType )
{
    U8 ucCmdTag;
    //U32 val;
    EnterCriticalSection( &g_tHCTMgr.WBQCriticalSection );
    ////check WBQ thread is actived by CST(FW trigger) or SGE notification
    //val = ( ulEventType == 0 ) ? g_tHCTMgr.ActiveWBQBitmap : g_tHCTMgr.DataDoneBitmap;
    ////but, WBQ can be processed only if FW have trigger it
    //val = g_tHCTMgr.ActiveWBQBitmap;
    
    // handle the WBQs without waiting for data done.
    for( ucCmdTag = 0; ucCmdTag < MAX_SLOT_NUM; ucCmdTag ++)
    {
        //if ( ( val & ( 1 << ucCmdTag ) ) == ( 1 << ucCmdTag ) )
       
        if (1 == g_tHCTMgr.ActiveWBQBitmap[ucCmdTag])
        {
            SIM_HostCHandleCmdWBQ( ucCmdTag );
        }
            
    }
    
    LeaveCriticalSection( &g_tHCTMgr.WBQCriticalSection );
    return TRUE;
}

BOOL AHCI_HostCProcessCmdWBQ(HCT_FCQ_WBQ *pWBQ, U8 ucCmdTag)
{
    BOOL bLastWbq;

    bLastWbq = pWBQ->bsLast;

    if (TRUE == pWBQ->bsWait || TRUE == pWBQ->bsWaitNfc || TRUE == bLastWbq)
    {
        g_tHCTMgr.DataDoneBitmap[ucCmdTag] = 0;  //&= ( ~( 1 << ucCmdTag ) );
    }

    //specail handling for last WBQ
    if (TRUE == bLastWbq)
    {
        //clear WBQ active flag
        //g_tHCTMgr.ActiveWBQBitmap &= ( ~( 1 << ucCmdTag ) );
        g_tHCTMgr.ActiveWBQBitmap[ucCmdTag] = 0;
        g_tHCTMgr.bsWBQOffset[ ucCmdTag ] = 0;
        //break;
    }


    if (pWBQ->bsClrCI)
    {
        AHCI_HostCClearPxCI(ucCmdTag);
    }

    if (pWBQ->bsClrSACT)
    {
        AHCI_HostCClearSACT(ucCmdTag);

    }
        
    if (pWBQ->bsLength)
    {
        SIM_WBQWriteBack(pWBQ);
    }

    if (pWBQ->bsAsstIntr)
    {
        AHCI_HostCWBQSetIS(pWBQ->bsIntrType);
    }   

    /*Note: Update CST must be the last operation to FW interface.
            HW design must meet this requirement.
    */
    if (pWBQ->bsUpdate)
    {
        HCT_SetCSTByWBQ(ucCmdTag, pWBQ->bsCST, pWBQ->bsNST);
    }

    return bLastWbq;
}

/*
Internal function for WBQ process thread.
Process the specified command WBQ.
This function should be called in the range of g_tHCTMgr.WBQCriticalSection
*/
BOOL SIM_HostCHandleCmdWBQ(U8 CmdTag)
{
#ifndef IGNORE_PERFORMANCE
    U32 ulCurTime;
#endif
    HCT_FCQ_WBQ *pWBQ = NULL;
    BOOL bLastWbq;

    for( ; g_tHCTMgr.bsWBQOffset[ CmdTag ] < g_uWBQDepth; g_tHCTMgr.bsWBQOffset[ CmdTag ]++)
    {
        pWBQ = (HCT_FCQ_WBQ*)(g_pWBQReg->usWBQBA + g_ulHCTSramStartAddr
            + ( CmdTag * g_uWBQDepth + g_tHCTMgr.bsWBQOffset[ CmdTag ] ) * sizeof(HCT_FCQ_WBQ) );
        if((TRUE == pWBQ->bsWait || TRUE == pWBQ->bsWaitNfc) && (g_tHCTMgr.DataDoneBitmap[CmdTag]  != 1 )) //& ( 1 << CmdTag )) != (1 << CmdTag) ))
        {
            return FALSE;
        }
#ifndef IGNORE_PERFORMANCE
        switch (l_ucWbqBusSts)
        {
        case WBQ_STS_WAIT_BUS:
            {
                EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                if (PCIE_STS_FREE == l_ucPcieBusDevToHostSts)
                {
                    l_ucPcieBusDevToHostSts = PCIE_STS_BUSY;
                    l_ucWbqBusSts = WBQ_STS_PCIE_TRANS;
                    l_tWbqTimeRecord.time_start = (U32)GET_TIME();
                    l_tWbqTimeRecord.time_busy = (U32)(BUSY_TIME_DEV_TO_HOST_PER_BYTE * pWBQ->bsLength * 4);
                }
                LeaveCriticalSection(&g_PcieDevToHostCriticalSection);
            }
            break;
        case WBQ_STS_PCIE_TRANS:
            {
                ulCurTime =  (U32)GET_TIME();
                if ((ulCurTime - l_tWbqTimeRecord.time_start) >= l_tWbqTimeRecord.time_busy)
                {
                    bLastWbq = AHCI_HostCProcessCmdWBQ(pWBQ, CmdTag);
                    l_ucWbqBusSts = WBQ_STS_WAIT_BUS;
                    EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                    l_ucPcieBusDevToHostSts = PCIE_STS_FREE;
                    LeaveCriticalSection(&g_PcieDevToHostCriticalSection);
                    if (TRUE == bLastWbq)
                    {
                        return TRUE;
                    }
                }
            }
            break;

        default:
            break;       
        }
#else
        if (TRUE == pWBQ->bsLast)
        {
            U8 uCmdCode = AHCI_HostGetCmdCode(CmdTag);

            if (ATA_CMD_VENDER_DEFINE != uCmdCode)
            {
                if ((0 != AHCI_HostGetLbaLen(CmdTag)) && (FALSE == ATA_IsNCQCMD(uCmdCode)) &&
                    (1 !=g_tHCTMgr.DataDoneBitmap[CmdTag]))
                {
                    return FALSE;
                }
            }
        }

        bLastWbq = AHCI_HostCProcessCmdWBQ(pWBQ, CmdTag);
        if (TRUE == bLastWbq)
        {
            return TRUE;
        }
#endif
    }
    return TRUE;
}
