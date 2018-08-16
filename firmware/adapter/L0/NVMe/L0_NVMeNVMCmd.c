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
* File Name    : L0_NVMeNVMCmd.c
* Discription  : this is create to implement NVM Command set defined in NVMe 
*                spec, such as Flush/Write/Read/Write Uncorrectable/
*                Compare/Write zeros/Data set Management and vendor defined cmd
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.19
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "L0_Interface.h"
#include "NVMeSpec.h"
#include "L0_NVMe.h"
#include "L0_NVMeHCT.h"
#include "L0_NVMeNVMCmd.h"
#include "L0_NVMeDataIO.h"
#include "L0_NVMeErrHandle.h"
#include "L0_TrimProcess.h"
#include "COM_Memory.h"
#include "Disk_Config.h"
#include "HAL_TraceLog.h"
#include "HAL_HCT.h"
#include "HAL_NVMECFGEX.h"
#include "HAL_NVME.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

#define TL_FILE_NUM L0_NVMeNVMCmd_c

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL U32 g_ulSubsysNum;
extern GLOBAL U32 g_ulSubsysNumBits;
extern GLOBAL U32 g_ulATARawBuffStart;
extern GLOBAL U32 g_ulRawDataReadyFlag;
extern GLOBAL U32 g_ulFlushCacheReady;
extern GLOBAL U32 g_ulInfoIdfyPage;
extern GLOBAL U32 g_ulSysLBAMax;

#ifdef AF_ENABLE
extern volatile NVME_CFG_EX        *g_pNVMeCfgExReg;    
#endif
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
GLOBAL U32 g_ulMaxHostLBA;
GLOBAL U32 g_ulMaxBlockNum;

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
extern GLOBAL PSCQ g_apSCmdQueue[SUBSYSTEM_NUM_MAX];

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/


U32 L0_NVMeAdvancePrpLocation(U8* PrdIndex, U32* PrdOffset, U32 Length )
{
    U32 ulStageSize;
    U32 size = 0;
    ulStageSize = HPAGE_SIZE - *PrdOffset;
    while( ( size + ulStageSize ) <= Length )
    {
        size += ulStageSize;
        (*PrdIndex)++;
        *PrdOffset = 0;
        ulStageSize = HPAGE_SIZE;
    }
    *PrdOffset += ( Length - size );
    return 0;
}

BOOL L0_NVMeFlushCache(PCB_MGR pSlot)
{
    U32 ulFinished = FALSE;

    switch(pSlot->CmdState)
    {
        case CMD_NEW:
            g_ulFlushCacheReady = FALSE;
            pSlot->CmdState = CMD_HANDLING;

        case CMD_HANDLING:
            if(SUCCESS == L0_IssueFlushSCmd(0))
            {
                pSlot->CmdState = CMD_WAITING;
            }
            break;

        case CMD_WAITING:
            if (TRUE == g_ulFlushCacheReady)
            {
                pSlot->CmdState = CMD_COMPLETED;
                L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
                HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
                ulFinished = TRUE;
            }
            break;
            
        default:
            break;
    }

    return ulFinished;
}

U32 L0_SplitNVMIOCmd(PCB_MGR pSlot)
{
    PSCMD pCurrSCmd;
    U32 ulFirstSCMDOfHCMD, ulLastSCMDOfHCMD;

    while (0 != pSlot->RemSecCnt)
    {
        if (TRUE == L0_IsSCQFull(0))
        {
            //ASSERT(FAIL);
            ulLastSCMDOfHCMD = FALSE;
            break;
        }

        if (0 == pSlot->CurrentSubCmdIndex++)
        {
            ulFirstSCMDOfHCMD = TRUE;
        }

        else
        {
            ulFirstSCMDOfHCMD = FALSE;
        }

        pCurrSCmd = L0_GetNewSCmdNode(0);

        pCurrSCmd->ucSCmdType = SCMD_DIRECT_MEDIA_ACCESS;
        pCurrSCmd->ucSlotNum = pSlot->SlotNum;
        pCurrSCmd->tMA.usHPRDEntryID = pSlot->PRPIndex;
        pCurrSCmd->tMA.ulHPRDMemOffset = pSlot->PRPOffset;
        pCurrSCmd->tMA.ulSubSysLBA = pSlot->CurrentLBAL;
        pCurrSCmd->tMA.ucSecLen = (SEC_PER_BUF - L0M_GET_OFFSET_IN_LCT_FROM_LBA(pSlot->CurrentLBAL));

        //DBG_Printf("cmd 0x%x startLBA 0x%x secLen 0x%x\n", pCurrSCmd->ucSlotNum, pCurrSCmd->tMA.ulSubSysLBA, pCurrSCmd->tMA.ucSecLen);

        if ((U32)pCurrSCmd->tMA.ucSecLen > pSlot->RemSecCnt)
        {
            pCurrSCmd->tMA.ucSecLen = (U8)pSlot->RemSecCnt;
        }

        if (FALSE == pSlot->IsWriteDir)
        {
            pCurrSCmd->tMA.ucOpType = DM_WRITE;
        }

        else
        {
            pCurrSCmd->tMA.ucOpType = DM_READ;

            if(FALSE != ulFirstSCMDOfHCMD)
            {
                pCurrSCmd->tMA.ulHCmdSecCnt = pSlot->TotalSecCnt;
            }

            if (pSlot->TotalSecCnt >= SEC_PER_BUF)
            {
                pCurrSCmd->tMA.ucIsSeq = TRUE;
            }
            
            else
            {
                pCurrSCmd->tMA.ucIsSeq = FALSE;
            }
        }

        pCurrSCmd->tMA.ucIsNCQ = pSlot->IsNeedPRPList;

        pCurrSCmd->tMA.ucFirst = ulFirstSCMDOfHCMD;

        pSlot->RemSecCnt -= (U32)pCurrSCmd->tMA.ucSecLen;
        pSlot->TotalRemainingBytes -= ((U32)pCurrSCmd->tMA.ucSecLen << SEC_SIZE_BITS);
        pSlot->CurrentLBAL += (U32)pCurrSCmd->tMA.ucSecLen;
        L0_NVMeAdvancePrpLocation((U8 *)&pSlot->PRPIndex, &pSlot->PRPOffset, (pCurrSCmd->tMA.ucSecLen << SEC_SIZE_BITS));

        if(pSlot->PRPIndex > (MAX_PRP_NUM-1))
        {
            DBG_Getch();
        }

        if (0 == pSlot->RemSecCnt)
        {
            if (FALSE == pSlot->IsWriteDir)
            {
                HAL_SgeFinishChainCnt(pSlot->SlotNum, pSlot->CurrentSubCmdIndex);
            }

            ulLastSCMDOfHCMD = TRUE;
        }

        else
        {
            ulLastSCMDOfHCMD = FALSE;
        }

        pCurrSCmd->tMA.ucLast = ulLastSCMDOfHCMD;

        L0_PushSCmdNode(0);
    }

    return ulLastSCMDOfHCMD;
}

BOOL L0_DataSetMgmtProcess(PCB_MGR pSlot)
{
    static U32 ulCurrTail;
    U32 ulTrimFlag;
    U32 ulFinished = FALSE;
    U32 ulProcessedEntryNum;
    static LBA_LONGENTRY tCurrSeg;
    static U32 ulCurrProcEntry, ulMaxEntryNum;
    PSCMD pNewSCmd;

    switch(pSlot->CmdState)
    {
        case CMD_NEW:
            ulCurrTail = 0;
            if (PRPLISTSTS_INVALIDADDR == L0_CheckNeedToFetchPRPList(pSlot))
            {
                /* PRP list address is invalid. */
                DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
                L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
                pSlot->CmdState = CMD_COMPLETED;
                break;
            }

            else
            {
                pSlot->CmdState = CMD_HANDLING;
            }

        case CMD_HANDLING:
            if (TRUE == L0_NVMeXferData(pSlot))
            {
                ulTrimFlag = pSlot->Ch->DW11 & (1<<2);
                if(FALSE == ulTrimFlag)
                {
                    /* The command does not request a trim operation. 
                                    So we can simply receive the data and report a dummy success status. */
                    pSlot->CmdState = CMD_COMPLETED;
                }
                else
                {
                    ulMaxEntryNum = pSlot->Ch->DW10 & 0xFF;
                    ulCurrProcEntry = 0;
                    pSlot->CmdState = CMD_DSM_PROCDATA;
                }
            }
            break;

        case CMD_DSM_PROCDATA:
            /* First we must scan the LBA range entry list for each continuous LBA range. */
            ulProcessedEntryNum = L0_TrimProcLBAEntry(ulCurrProcEntry, ulMaxEntryNum, &tCurrSeg);

            if (INVALID_8F == ulProcessedEntryNum)
            {                
                /* Invalid LBA range entry encountered. */
                L0_NVMeCmdError(pSlot->SlotNum, ((INVALID_4F << 16) | NVME_SF_LBA_OUTOFRANGE));
                pSlot->CmdState = CMD_COMPLETED;
            }
            else if (0 == ulProcessedEntryNum)
            {
                /* LBA range list has been completed. */
                //pSlot->CmdState = CMD_COMPLETED;
                pSlot->CmdState = CMD_DSM_WAITING;
            }
            else
            {
                pSlot->CmdState = CMD_DSM_DISPATCH;

                /* Preparing to process next LBA segment. */
                ulCurrProcEntry += ulProcessedEntryNum;
                ulCurrTail = L0SCQ_Tail(0);
            }
            break;

        case CMD_DSM_DISPATCH:
            /* Attempts to find one free SCMD node for back-end and issue the SCMD. */
            if (SUCCESS == L0_IssueUnmapSCmd(0, tCurrSeg.ulStartLBA, tCurrSeg.ulRegionLen, &ulCurrTail))
            {
                pSlot->CmdState = CMD_DSM_PROCDATA;
            }
            break;

        case CMD_DSM_WAITING:
            pNewSCmd = &L0SCQ_Node(0, ulCurrTail);
            if ((pNewSCmd->ucSCmdStatus == (U8)SSTS_SUCCESS) || 
                (pNewSCmd->ucSCmdStatus == (U8)SSTS_NOT_ALLOCATED))
            {
                pSlot->CmdState = CMD_COMPLETED;
            }
            break;

        case CMD_COMPLETED:
            L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
            HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
            ulFinished = TRUE;
            break;

        default:
            break;
    }

    return ulFinished;
}

U32 L0_NVMeCheckCmdParam(PCB_MGR pSlot)
{
    U32 ulOpCode;
    U32 ulSLBA;
    U32 ulNLB;
    U32 ulELBA;
    U32 ulProtChk;

    //checks namespace
    switch (pSlot->Ch->NSID)
    {
        case 1:
        case INVALID_8F:
            break;

        default:
            DBG_Printf("R/W Namespace Invalid (%d)!\n", pSlot->Ch->NSID);
            return NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT);
    }

    //checks LBA and MDTS
    ulOpCode = pSlot->Ch->OPC;

    switch (ulOpCode)
    {
        case NCS_WRITE_ZERO:
            ulProtChk = (pSlot->Ch->DW12 >> 26) & 7;

            if (0 != ulProtChk)
            {
                DBG_Printf("Invalid PRCHK field (%d)!\n", ulProtChk);
                return NVME_ERRLOG_COMPRESS(12 * 4 + 3, 2, NVME_SF_IVLD_FIELD);
            }

        case NCS_WRITE:
        case NCS_READ:
            ulNLB = pSlot->Ch->DW12 & MSK_4F;

            if ((NCS_WRITE_ZERO != ulOpCode) &&
                (ulNLB >= g_ulMaxBlockNum))
            {
                DBG_Printf("Data Length Overflow (%d)!\n", ulNLB);
                return NVME_ERRLOG_COMPRESS(12 * 4, 0, NVME_SF_IVLD_FIELD);
            }

            if (0 != pSlot->Ch->DW11)
            {
                DBG_Printf("LBA higher DWORD not zero (0x%x)!\n", pSlot->Ch->DW11);
                return NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_LBA_OUTOFRANGE);
            }

            else
            {
                ulSLBA = pSlot->Ch->DW10;
                ulELBA = ulSLBA + ulNLB;

                if ((ulELBA < ulSLBA) || (ulELBA >= g_ulMaxHostLBA))
                {
                    DBG_Printf("LBA range overflow (0x%x + 0x%x)!\n", ulSLBA, ulNLB);
                    return NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_LBA_OUTOFRANGE);
                }
            }

        default:
            break;
    }

    return NVME_SF_SUCCESS;
}

BOOL L0_NVMeWriteZero(PCB_MGR pSlot)
{
    U32 ulFirstSCMDOfHCMD, ulLastSCMDOfHCMD;
    BOOL bFinish = FALSE;
    PSCMD pCurrSCmd;

    switch (pSlot->CmdState)
    {
        case CMD_NEW:
            pSlot->CmdState = CMD_HANDLING;

        case CMD_HANDLING:
            //Split NVMe CMD to SubCMD
            while (0 != pSlot->RemSecCnt)
            {
                if (TRUE == L0_IsSCQFull(0))
                {
                    break;
                }

                if (0 == pSlot->CurrentSubCmdIndex++)
                {
                    ulFirstSCMDOfHCMD = TRUE;
                }
                
                else
                {
                    ulFirstSCMDOfHCMD = FALSE;
                }
                
                pCurrSCmd = L0_GetNewSCmdNode(0);

                pCurrSCmd->ucSCmdType = SCMD_DIRECT_MEDIA_ACCESS;
                pCurrSCmd->tMA.ucOpType = DM_WRITE;
                pCurrSCmd->tMA.ucOption = DM_WRITE_ZERO;

                pCurrSCmd->ucSlotNum = pSlot->SlotNum;
                pCurrSCmd->tMA.ulSubSysLBA = pSlot->CurrentLBAL;
                pCurrSCmd->tMA.ucSecLen = (SEC_PER_BUF - L0M_GET_OFFSET_IN_LCT_FROM_LBA(pSlot->CurrentLBAL));

                if ((U32)pCurrSCmd->tMA.ucSecLen > pSlot->RemSecCnt)
                {
                    pCurrSCmd->tMA.ucSecLen = (U8)pSlot->RemSecCnt;
                }

                pCurrSCmd->tMA.ucFirst = ulFirstSCMDOfHCMD;

                pSlot->RemSecCnt -= pCurrSCmd->tMA.ucSecLen;
                pSlot->TotalRemainingBytes -= ((U32)pCurrSCmd->tMA.ucSecLen << SEC_SIZE_BITS);
                pSlot->CurrentLBAL += (U32)pCurrSCmd->tMA.ucSecLen;

                if (0 == pSlot->RemSecCnt)
                {
                    ulLastSCMDOfHCMD = TRUE;
                    pSlot->CmdState = CMD_WAITING;
                }
                
                else
                {
                    ulLastSCMDOfHCMD = FALSE;
                }
                
                pCurrSCmd->tMA.ucLast = ulLastSCMDOfHCMD;

                L0_PushSCmdNode(0);
            }//While

            break;

        case CMD_WAITING:
            //Check respone from L1 or Wait all SubCMD are finished by Subsystem
            if (TRUE == L0_IsSCQEmpty(0))
            {
                pSlot->CmdState = CMD_COMPLETED;
                L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
                HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
                bFinish = TRUE;
            }

            break;

        default:
            DBG_Printf("L0_NVMeWriteZero Slot State Error.\n");
            DBG_Getch();
            break;
    }

    return bFinish;
}

BOOL RESTRICTION_RODATA_IN_DSRAM L0_NVMeProcRWCmd(PCB_MGR pSlot)
{
    U32 ulNeedPRPList;
    BOOL bFinish = FALSE;

    if (CMD_NEW == pSlot->CmdState)
    {
        /* 1. Checks whether command needs to fetch PRP list. */
        ulNeedPRPList = L0_CheckNeedToFetchPRPList(pSlot);
        
        if (PRPLISTSTS_INVALIDADDR == ulNeedPRPList)
        {
            /* PRP list address is invalid. */
            DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
            pSlot->CmdState = CMD_COMPLETED;
        }
        
        else if (PRPLISTSTS_NEEDFETCH == ulNeedPRPList)
        {
            /* We need to program FCQ to fetch PRP list. */
            pSlot->CmdState = CMD_FETCHPRPLIST;
        }
        
        else
        {
            /* It's not required to fetch PRP list. */
            pSlot->CmdState = CMD_HANDLING;
        }
    }

    else if (CMD_CHECKPRPLIST == pSlot->CmdState)
    {
        if (FALSE == L0_IsPRPListOffsetValid(pSlot))
        {
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
            pSlot->CmdState = CMD_COMPLETED;
        }
        
        else
        {
            pSlot->CmdState = CMD_HANDLING;
        }
    }

    switch(pSlot->CmdState)
    {
        case CMD_HANDLING:
            if (FALSE != L0_SplitNVMIOCmd(pSlot))
            {
                pSlot->CmdState = CMD_COMPLETED;
            }

            else
            {
                break;
            }

        case CMD_COMPLETED:
            L0_NVMeSetupSimpleCompletion(pSlot, ((NVME_SF_SUCCESS == pSlot->CmdSts) ? TRUE : FALSE), TRUE);
            HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
            bFinish = TRUE;
            break;

        case CMD_FETCHPRPLIST:
            /* 2. Attempts to fetch PRP list. */
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp start:");
            if (FAIL != L0_NVMeHCTReadPRP(pSlot))
            {
                pSlot->CmdState = CMD_CHECKPRPLIST;

                /* We can release active slot when waiting for HCT to fetch PRP list, 
                    so that MCU0 can process other commands. */
                bFinish = TRUE;
            }
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp end  :");
            break;

        default:
            break;
    }

    return bFinish;
}

BOOL L0_NVMeProcessNVMCmd(PCB_MGR pSlot)
{
    BOOL bFinish = FALSE;
    U32  ulStatus;
    U32 ulCmdCode;

    if(pSlot->CmdState == CMD_NEW)
    {
        ulStatus = L0_NVMeCheckCmdParam(pSlot);

        if (NVME_SF_SUCCESS == ulStatus)
        {
            L0_NVMeSetCmdParam(pSlot);
        }

        else
        {
            DBG_Printf("Error:invalid parameter in nvme cmd\n");
            L0_NVMeForceCompleteCmd(pSlot, ulStatus);
            return TRUE;
        }

#ifdef AF_ENABLE
        //Check CMD
        if(0 == (gNvmeMgr.ValidIOSQMap & (BIT(pSlot->SQID))))
        {
            DBG_Printf("SQTAIL:0x%x SQ_HWRP:0x%x ", g_pNVMeCfgReg->doorbell[pSlot->SQID].sq_tail, g_pNVMeCfgExReg->SQCfgAttr[pSlot->SQID].HWRP);
            DBG_Printf("Get A IO CMD in an invalid SQ(%d). Slot:%d\n", pSlot->SQID, pSlot->SlotNum);
            DBG_Getch();
        }
#endif
    }

    ulCmdCode = pSlot->Ch->OPC;

    switch(ulCmdCode)
    {
        case NCS_FLUSH:
            bFinish = L0_NVMeFlushCache(pSlot);
            break;

        case NCS_READ:
        case NCS_WRITE:
            //bFinish = L0_SplitNVMIOCmd(pSlot);
            bFinish = L0_NVMeProcRWCmd(pSlot);
            break;

        case NCS_WRITE_ZERO:
            bFinish = L0_NVMeWriteZero(pSlot);
            break;

        case NCS_DATASET_MANAGEMENT:
            bFinish = L0_DataSetMgmtProcess(pSlot);
            break;

        default:
            DBG_Printf("Error:invalid opcode:%x in nvme cmd\n",pSlot->Ch->OPC);
            L0_NVMeForceCompleteCmd(pSlot, NVME_ERRLOG_COMPRESS(0, 0, NVME_SF_IVLD_OPCODE));
            bFinish = TRUE;
    }

    return bFinish;
}

/*====================End of this file========================================*/

