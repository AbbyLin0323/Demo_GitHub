/****************************************************************************
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
*****************************************************************************
Filename    :L0_HcmdChain.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#ifdef SIM
#include <Windows.h>
#include "simsatadev.h"
#endif
#include "HAL_Xtensa.h"
#include "HAL_SataIO.h"
#include "HAL_ParamTable.h"
#include "L0_Config.h"
#include "L0_Interface.h"
#include "L0_HcmdChain.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"

extern PTABLE *g_pBootParamTable;

U8* g_pL0L1BufUsedCnt;
#define L0_HCMD_WRITE_BUFFER_THS (8)

extern U32 g_ulSubsysNumBits;

// host command slot
HCMD HostCmdSlot[MAX_SLOT_NUM];

// host command chain
HCMD* volatile g_pHCMDHead = NULL;
HCMD* volatile g_pHCMDTail = NULL;

// pointer to the current host command selected by L0
HCMD* g_pCurHCMD;

// host command counter, used as timestamp in simulation
U32 g_ulHCMDCounter = 0;

// the last lba of the last hcmd selected
U32 g_ulLastHcmdLba;

void L0_AddNewHostCmd(U8 ucNewCmdTag)
{
    HCMD* pHCMD;
    
    // fetch the host command entry
    pHCMD = &HostCmdSlot[ucNewCmdTag];

    // add the host command to the host command chain
    if (NULL == g_pHCMDTail)
    {
        // the chain is empty, set the current host command as
        // both the head and the tail of the chain
        g_pHCMDHead = pHCMD;
        g_pHCMDTail = pHCMD;
    }
    else
    {
        // the chain is not empty, add the host command to the
        // tail
        g_pHCMDTail->pNextHCMD = pHCMD;
        g_pHCMDTail = pHCMD;
    }

    // update the receive time of the host command
#ifdef ASIC
    pHCMD->ulRecvTime = HAL_GetMCUCycleCount();
#else
    pHCMD->ulRecvTime = g_ulHCMDCounter;
    g_ulHCMDCounter++;
#endif

    return;
}

void L0_HostCommandInit(void)
{
    // reset host command chain
    g_pHCMDHead = NULL;
    g_pHCMDTail = NULL;

    // initialize the current host command pointer
    g_pCurHCMD = NULL;

#ifndef ASIC
    // reset the host command counter
    g_ulHCMDCounter = 0;
#endif

    // reset all host command slots
    COM_MemZero((U32 *)&HostCmdSlot[0], sizeof(HostCmdSlot)/sizeof(U32));

    // initialize last accessed lba
    g_ulLastHcmdLba = INVALID_8F;

    return;
}

HCMD* L0_HostCmdSelect(void)
{
    U8 ucCmdTag;
    HCMD* pHCMD;

    // we have to disable the interrupt while accessing the
    // host command chain
#ifdef SIM
    EnterCriticalSection(&g_CMDQueueCriticalSection);
#else
    // disable interrupt
    HAL_DisableMCUIntAck();
#endif

    // check if the host command queue is empty
    if(TRUE == L0_HostCmdChainEmpty())
    {
        // the host command chain is empty, set the command tag
        // to INVALID_2F
        ucCmdTag = INVALID_2F;
    }
#ifdef L0_HCMD_FIFO
    // please note that L0_HCMD_FIFO should always be defined
    // when operating L0 ramdisk
    else
    {
        ucCmdTag = L0_ForceSelectHcmdHead();
    }
#else
    // check if the head of the host command queue is too old
    else if (TRUE == L0_IsHcmdNeedForceSelect())
    {
        ucCmdTag = L0_ForceSelectHcmdHead();
    }
    // select a host command from the queue
    else
    {
        ucCmdTag = L0_SelectHcmdFromChain();
    }
#endif

    if (INVALID_2F != ucCmdTag)
    {
        // we have successfully select a host command from the
        // queue
        pHCMD = &HostCmdSlot[ucCmdTag];
        pHCMD->pNextHCMD = NULL;

        // set the last accessed lba for ncq commands
        L0_SetHcmdLastAccessedLba(pHCMD);
    }
    else
    {
        // no host command selected, return NULL
        pHCMD = NULL;
    }

#ifdef SIM
    LeaveCriticalSection(&g_CMDQueueCriticalSection);
#else
    // enable interrupt
    HAL_EnableMCUIntAck();
#endif

    return pHCMD;
}

BOOL L0_HostCmdChainEmpty(void)
{
    // check if the host command chain is empty
    return (NULL == g_pHCMDTail) ? TRUE : FALSE;
}

BOOL L0_IsHcmdNeedForceSelect(void)
{
    // this function checks if the head of the host command
    // queue is "too old", if it is, forcefully pick it out of
    // the queue

    volatile U32 ulCurrentTime;
    U32 ulHCMDRecvTime;
  
    // get the current time in the firmware
#ifdef ASIC
    ulCurrentTime = HAL_GetMCUCycleCount();
#else
    ulCurrentTime = g_ulHCMDCounter;
#endif

    // get the time which the head of the queue is received
    ulHCMDRecvTime = g_pHCMDHead->ulRecvTime;

    // compare the difference
    if (ulCurrentTime > ulHCMDRecvTime)
    {
        return ((ulCurrentTime - ulHCMDRecvTime) > HCMDQ_FORCE_SELECT_LATENCY) ? TRUE : FALSE;
    }
    else
    {
        return ((INVALID_8F - ulHCMDRecvTime + ulCurrentTime) > HCMDQ_FORCE_SELECT_LATENCY) ? TRUE : FALSE;
    }
}

U8 L0_ForceSelectHcmdHead(void)
{
    // this function selects the head of the host command
    // chain
    U8 ucCmdTag;

#if defined(SIM) || defined(L0_DEBUG)
    if(L0_HostCmdChainEmpty() == TRUE)
    {
        DBG_Printf("host command chain error\n");
        DBG_Getch();
    }
#endif

    // get the tag of the head
    ucCmdTag = g_pHCMDHead->tCbMgr.SlotNum;

    // maintain the host command queue
    if (g_pHCMDHead == g_pHCMDTail)
    {
        // the current command is the only command in the chain
        g_pHCMDHead = NULL;
        g_pHCMDTail = NULL;
    }
    else
    {
        // update the head of the host command chain
        g_pHCMDHead = g_pHCMDHead->pNextHCMD;
    }

    // return the tag number of the selected command
    return ucCmdTag;
}

U8 L0_SelectHcmdFromChain(void)
{
    U8 ucCmdTag;
    U8 ucSubCmdPu;
    U8 ucPuCheckCnt;
    U32 ulStartLBA;
    U32 ulEndLBA;
    BOOL bReSelect;
    HCMD* pHCMD;
    HCMD* pLastHCMD;
    U32 ulSubsysId;

    // get the head of the host command queue
    pHCMD = g_pHCMDHead;
    
    // the selection of host commands follows a certain priority:
    // 1. sequential NCQ commands
    // 2. anything that is not NCQ write
    // 3. NCQ writes that pass the buffer test
    while (NULL != pHCMD)
    {
        // uncomment the following line to select the head of the
        // chain no matter what
        //break;


        // frist check if it's a sequential NCQ command
        if ((SATA_PROT_FPDMA == pHCMD->tCbMgr.SATAProtocol) && (INVALID_8F != g_ulLastHcmdLba))
        {
            // sequential NCQ commands
            if(pHCMD->ulStartLba == g_ulLastHcmdLba)
            {
                break;
            }
        }

        if(SATA_PROT_FPDMA == pHCMD->tCbMgr.SATAProtocol && FALSE == pHCMD->tCbMgr.IsWriteDir)
        {
            // this flag indicates if we have to reselect a hcmd
            bReSelect = FALSE;

            // indicate the number of pus that have been checked
            ucPuCheckCnt = 0;

            // calculate the global start and end lba for this
            // hcmd, note that the start lba is shifted to
            // buffer boundary for convenience 
            ulStartLBA = (pHCMD->ulStartLba) & (~SEC_PER_BUF_MSK);
            ulEndLBA = pHCMD->ulStartLba + pHCMD->tCbMgr.HCMDLenSec - 1;

            // check all PUs the current host command is gonna
            // access, see if they have free buffers
            while ((ucPuCheckCnt < 4) && (ulStartLBA <= ulEndLBA))
            {
                // first get the local pu number in the
                // subsystem this lba belongs to
                ucSubCmdPu = L0_GetLocalPuFromLba(ulStartLBA);

                // calculate the subsystem id the lba belongs
                // to
                ulSubsysId = L0M_GET_SUBSYSID_FROM_LBA(ulStartLBA, g_ulSubsysNumBits);

                // set g_pL0L1BufUsedCnt to proper location based
                // on the subsystem id, so we can access the
                // buffer status of the subsystem
                if(ulSubsysId == 0)
                {
                    g_pL0L1BufUsedCnt = (U8*)WRITE_BUF_INFO_MCU01_BASE;
                }
                else if(ulSubsysId == 1)
                {
                    g_pL0L1BufUsedCnt = (U8*)WRITE_BUF_INFO_MCU02_BASE;
                }
                else
                {
                    DBG_Printf("ulSubsysId error\n");
                    DBG_Getch();
                }

                // check the buffer status of the subsystem, see
                // if it still has a free buffer
                if(g_pL0L1BufUsedCnt[ucSubCmdPu] >= L0_HCMD_WRITE_BUFFER_THS)
                {
                    // no free buffers
                    bReSelect = TRUE;
                    break;
                }
                else
                {
                    // the current PU has free buffers, check the
                    // next PU

                    // advance the LBA
                    ulStartLBA += SEC_PER_BUF;


                    // increase PU check count
                    ucPuCheckCnt++;
                }
            } // while ((ucPuCheckCnt < 4) && (ulStartLBA <= ulEndLBA))

            // check if we have to reselect a host command, if
            // so, advance to the next host command
            if (bReSelect == TRUE)
            {
                pLastHCMD = pHCMD;
                pHCMD = pHCMD->pNextHCMD;
                continue;
            }
            else
            {
                break;
            }

        } // if(HCMD_PROTOCOL_NCQ == pHCMD->ucCmdProtocol && HCMD_WRITE == pHCMD->ucCmdRW)
        else
        {
            // select anything that is not NCQ write directly
            break;
        }
    } // while (NULL != pHCMD)

    // remove the selected host command from the chain
    if (NULL != pHCMD)
    {
        if (pHCMD == g_pHCMDTail)
        {
            // the selected command is the last one in the queue
            if (g_pHCMDHead == g_pHCMDTail)
            {
                g_pHCMDHead = NULL;
                g_pHCMDTail = NULL;
#ifdef L0_DEBUG
                //DBG_Printf("CP5: H: %x, T: %x\n",g_pHCMDHead, g_pHCMDTail);
#endif
            }
            else
            {
                g_pHCMDTail = pLastHCMD;
                g_pHCMDTail->pNextHCMD = NULL;
#ifdef L0_DEBUG
                //DBG_Printf("CP6: H: %x, T: %x\n",g_pHCMDHead, g_pHCMDTail);
#endif
            }
        }
        else if (pHCMD == g_pHCMDHead)
        {
            // the selected command is the first one in the queue
            g_pHCMDHead = pHCMD->pNextHCMD;
#ifdef L0_DEBUG
            //DBG_Printf("CP7: H: %x, T: %x\n",g_pHCMDHead, g_pHCMDTail);
#endif
        }
        else
        {
            pLastHCMD->pNextHCMD = pHCMD->pNextHCMD;
        }
        
        ucCmdTag = pHCMD->tCbMgr.SlotNum;
    }
    else
    {
        ucCmdTag = INVALID_2F;
    }

    return ucCmdTag;
}

void L0_SetHcmdLastAccessedLba(HCMD* pHCMD)
{
    if (SATA_PROT_FPDMA == pHCMD->tCbMgr.SATAProtocol)
    {
        g_ulLastHcmdLba = pHCMD->ulStartLba + pHCMD->tCbMgr.HCMDLenSec;
    }
    else
    {
        g_ulLastHcmdLba = INVALID_8F;
    }

    return; 
}

U8 L0_GetLocalPuFromLba(U32 ulGlobalLba)
{
    U32 ulLocalLba;

    // first we have to calculate the subsys lba based on the
    // global lba
    ulLocalLba = L0M_GET_SUBSYSLBA_FROM_LBA(ulGlobalLba, g_ulSubsysNumBits);

    // calculate the local pu number based on the subsys lba
    // calculate the local lct, then mod the local lct by PU_NUM
    return (ulLocalLba >> SEC_PER_BUF_BITS) % (g_pBootParamTable->ulSubSysCeNum);
}

