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
* File Name    : L0_NVMeSchedule.c
* Discription  : 
* CreateAuthor : HavenYang
* CreateDate   : 2014.12.3
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_HCmdTimer.h"
#include "HAL_NVME.h"
#include "HAL_GPIO.h"

#ifdef AF_ENABLE
#include "HAL_NVMECFGEX.h"
#endif
#include "HAL_HwDebug.h"
#include "HAL_TraceLog.h"
#include "L0_NVMe.h"
#include "L0_NVMeSchedule.h"
#include "L0_NVMeHCT.h"
#include "L0_NVMeAdminCmd.h"
#include "L0_NVMeNVMCmd.h"
#include "L0_Event.h"
#include "L0_NVMeDataIO.h"
#include "L0_NVMeErrHandle.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define TL_FILE_NUM L0_NVMeSchedule_c

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL U32 g_ulHostInfoAddr;
extern GLOBAL U32 g_ulShutdownReady;
extern GLOBAL U32 g_ulSubsysNum;
#ifdef AF_ENABLE
extern volatile NVME_CFG_EX *g_pNVMeCfgExReg;
#endif
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
GLOBAL BOOL bLEDSetActive = FALSE;
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL U32 g_ulToProcessBitMap = 0;
/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

#ifdef DBG_NVME
void DBG_PrintHCmdHeader(U32 ulSlotNum)
{
    U32 ulIndex;
    U32 *pHCmdAddr;
    //static U32 ulCmdCount = 0;

    pHCmdAddr = (U32 *)(HCT_SRAM_BASE + ulSlotNum * sizeof(COMMAND_HEADER));

    DBG_Printf("Slot(%d) host command:\n", ulSlotNum);
    for (ulIndex = 0; ulIndex < 8; ulIndex++)
    {
        DBG_Printf("%x ", *pHCmdAddr);
        pHCmdAddr++;
    }
    DBG_Printf("\n");
    for (ulIndex = 0; ulIndex < 8; ulIndex++)
    {
        DBG_Printf("%x ", *pHCmdAddr);
        pHCmdAddr++;
    }
    DBG_Printf("\n");

}


void DBG_PrintPRP(U32 ulSlotNum)
{
    PCB_MGR *pSlot;
    U32 ulIndex;
    U32 *pPRPAddr;

    pSlot = GET_SLOT_MGR(ulSlotNum);
    pPRPAddr = (U32 *)pSlot->PRPTable;

    DBG_Printf("Slot(%d) PRP : \n", ulSlotNum);
    for (ulIndex = 0; ulIndex < 8; ulIndex++)
    {
        DBG_Printf("%x ", *pPRPAddr);
        pPRPAddr++;
    }
    DBG_Printf("\n");
    for (ulIndex = 0; ulIndex < 8; ulIndex++)
    {
        DBG_Printf("%x ", *pPRPAddr);
        pPRPAddr++;
    }
    DBG_Printf("\n");
}

void DBG_PrintDSG(U32 ulDSGID)
{
    NORMAL_DSG_ENTRY *pDSG;

    pDSG = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(ulDSGID);

    DBG_Printf("DSGID: %d\n", ulDSGID);

    DBG_Printf("ulDramAddr:        0x%x\n", pDSG->ulDramAddr);
    DBG_Printf("bsCacheStatusAddr: 0x%x\n", pDSG->bsCacheStatusAddr);
    DBG_Printf("bsCacheStsEn:      %d\n", pDSG->bsCacheStsEn);
    DBG_Printf("bsLast:            %d\n", pDSG->bsLast);
    DBG_Printf("bsNextDsgId:       %d\n", pDSG->bsNextDsgId);
}

void DBG_PrintHSG(U32 ulHSGID)
{
    HSG_ENTRY *pHSG;
    pHSG = (HSG_ENTRY *)HAL_GetHsgAddr(ulHSGID);

    DBG_Printf("HSGID: %d\n", ulHSGID);

    DBG_Printf("bsLast:         %d\n", pHSG->bsLast);
    DBG_Printf("bsLength:       %d\n", pHSG->bsLength);
    DBG_Printf("ulHostAddrHigh: 0x%x\n", pHSG->ulHostAddrHigh);
    DBG_Printf("ulHostAddrLow:  0x%x\n", pHSG->ulHostAddrLow);
    DBG_Printf("bsNextHsgId:    %d\n", pHSG->bsNextHsgId);
    DBG_Printf("ulLBA:          %d\n", pHSG->ulLBA);
}
#else
void DBG_PrintHCmdHeader(U32 ulHSGID){}
void DBG_PrintPRP(U32 ulHSGID){}
void DBG_PrintDSG(U32 ulHSGID){}
void DBG_PrintHSG(U32 ulHSGID){}
#endif

#ifndef AF_ENABLE
U32 L0_NVMeReadSQEntry(U32 ulSQID)
{
    PCB_MGR pCurrCmdSlot;
    U32 ulSlotNum;

    TL_PERFORMANCE(PTL_LEVEL_DETAIL, "fetch nvme command header start: ");

#ifdef SIM
    if (SQ_EMPTY(ulSQID))
    {
        DBG_Printf("state error: doorbell[%d] = %d, rp[%d] = %d, helper=0x%x\n",
        ulSQID,SQ_TAIL_DOORBELL(ulSQID), ulSQID, SQ_FWRP(ulSQID), g_pNVMeCfgReg->cmd_fetch_helper);
        DBG_Getch();
    }
#endif

    ulSlotNum = HAL_HCTSearchCST(SLOT_IDLE);
    if(INVALID_CMD_ID == ulSlotNum)
    {
        return FAIL;
    }

#ifdef L0_DBG_STATE
    if (SLOT_IDLE != HAL_HCTGetCST(ulSlotNum))
    {
        DBG_Printf("Cst search error(HW) in L0_NVMeReadSQEntry, slot(%d)\n",ulSlotNum);
        //DBG_Getch();
        L0_WaitHostReset();
        return FAIL;
    }
#endif

    pCurrCmdSlot = GET_SLOT_MGR(ulSlotNum);
    L0_NVMeRecycleCbMgr(pCurrCmdSlot);
    
    pCurrCmdSlot->SQID = ulSQID;
    pCurrCmdSlot->CQID = BIND_CQID(ulSQID);
    pCurrCmdSlot->SQEntryIndex = (U16)SQ_FWRP(ulSQID);
    
    HAL_HwDebugStart(ulSlotNum);
    HAL_HCmdTimerStart(ulSlotNum);
    
    if (SUCCESS == L0_NVMeHCTReadCH(pCurrCmdSlot))
    {
        if (0 != ulSQID)    //AdminSQ doesn't need to update State.
        {
            UPDATE_IOSQ_STATE(ulSQID);
        }
        
        PUSH_SQ_FWRP(ulSQID);
        PUSH_CQ_TAIL(pCurrCmdSlot->CQID);

        pCurrCmdSlot->CmdState = CMD_NEW;
    }
    else
    {
        return FAIL;
    }

    //If Read Admin CMD via HCTReadCH successfully, Mark Flag and slotNum
    if (0 == ulSQID)
    {
        if (gNvmeMgr.AdminProcessing == FALSE)
        {
            gNvmeMgr.AdminProcessing = TRUE;
            gNvmeMgr.AdminProcessingSlot = ulSlotNum;
        }
        else
        {
            DBG_Printf("The AdminProcessing should be FALSE before we set to TRUE\n");
            DBG_Getch();
        }
    }

    TL_PERFORMANCE(PTL_LEVEL_DETAIL, "fetch nvme command header end  : ");
    return SUCCESS;
}

INLINE void L0_AdminSQSchedule(void)
{
    L0_NVMeReadSQEntry(0);
}

void L0_IOSQSchedule(void)
{
    U32 IoSqId;
    U32 ulCQID;
    
    if (0 == g_ulToProcessBitMap)
    {
 #ifdef SIM
    U8 ulSQIDCnt;
    for (ulSQIDCnt = 1; ulSQIDCnt < MAX_SQ_NUM; ulSQIDCnt++)
    {
        if (SQ_NOT_EMPTY(ulSQIDCnt))
        {
            g_ulToProcessBitMap |= (1 << ulSQIDCnt);
        }
        else
        {
            g_ulToProcessBitMap &= ~(1 << ulSQIDCnt);
        }
    }
    g_ulToProcessBitMap &= gNvmeMgr.ValidIOSQMap;
#else
    g_ulToProcessBitMap = g_pNVMeCfgReg->cmd_fetch_helper & gNvmeMgr.ValidIOSQMap;
#endif //SIM       
    }

    while (0 != g_ulToProcessBitMap)
    {
        IoSqId = 31 - HAL_CLZ(g_ulToProcessBitMap);

        ulCQID = BIND_CQID(IoSqId);
        if (CQ_NOT_FULL(ulCQID))
        {
            if (SUCCESS == L0_NVMeReadSQEntry(IoSqId))
            {
                g_ulToProcessBitMap &= ~(BIT(IoSqId));
            }
            break;
        }
        
        g_ulToProcessBitMap &= ~(BIT(IoSqId));        
    }
    
    return;
}

void L0_SQSchedule(void)
{
    if (gNvmeMgr.AdminProcessing == TRUE)
    {
        //Fetch Admin CMD one by one.
        return;
    }

    if (HAVE_NVME_CMD())
    {
        if(bLEDSetActive == FALSE)
        {
             /* enable LED function clock and enable LED GPIO , set freq to 5HZ*/
            (*((volatile U32*) (REG_BASE_GLB + 0x04))) |= (1 << 13);
            rGPIOLED = 0x2b2b;
            bLEDSetActive = TRUE;
        }

        L0_NVMeCheckSQDoorBell();

        if (HAVE_NVME_ADMIN_CMD() && CQ_NOT_FULL(0))
        {
            L0_AdminSQSchedule();
        }
        else
        {
            L0_IOSQSchedule();
        }
    }
    
    return;
}

MCU0_DRAM_TEXT void L0_SlotAdminSchdule(void)
{
    BOOL    bFinish;
    PCB_MGR pCurrSlot;
    U32     ulSlotNum;

    ulSlotNum = gNvmeMgr.AdminProcessingSlot;

    //Check INVALID_CMD_ID
    if (INVALID_CMD_ID == ulSlotNum)
    {
        DBG_Printf("Got Invalid_cmd_id which we are trying to process\n");
        DBG_Getch();
    }

    pCurrSlot = GET_SLOT_MGR(ulSlotNum);

#ifdef L0_DBG_STATE
    if (CMD_IDLE == pCurrSlot->CmdState)
    {
        DBG_Printf("L0_SlotAdminSchedule CmdState error in slot(%d), CmdState(%d)\n", ulSlotNum, pCurrSlot->CmdState);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }

    if (pCurrSlot->SQID != 0 )
    {
        DBG_Printf("Handle IO CMD in L0_SlotAdminSchedule()\n");
        DBG_Getch();
    }
#endif  

    bFinish = L0_NVMeProcessAdminCmd(pCurrSlot);

    if (bFinish == TRUE)
    {
        gNvmeMgr.AdminProcessing = FALSE;
        gNvmeMgr.AdminProcessingSlot = INVALID_CMD_ID;
    }

    return;
}


void L0_SlotIOSchedule(void)
{
    BOOL bFinish;
    PCB_MGR pCurrSlot;
    U32 ulSlotNum;
    U32 ulHwCST;

    ulSlotNum = gNvmeMgr.ActiveSlot;
        
    if(INVALID_CMD_ID == ulSlotNum)
    {
        ulSlotNum = HAL_HCTSearchCST(SLOT_PRPLIST_RDY);
        
        if(INVALID_CMD_ID == ulSlotNum)
        {
            ulSlotNum = HAL_HCTSearchCST(SLOT_SQENTRY_RDY);

            if(INVALID_CMD_ID == ulSlotNum)
            {
                return;
            }
        }

#ifdef L0_DBG_STATE
        ulHwCST = HAL_HCTGetCST(ulSlotNum);

        if ((SLOT_PRPLIST_RDY != ulHwCST) && (SLOT_SQENTRY_RDY != ulHwCST))
        {
            DBG_Printf("Cst search error(HW) in L0_SlotIOSchedule, slot(%d)\n",ulSlotNum);
            //DBG_Getch();
            L0_WaitHostReset();
            return;
        }
#endif
    }

    pCurrSlot = GET_SLOT_MGR(ulSlotNum);

#ifdef L0_DBG_STATE
    if(CMD_IDLE == pCurrSlot->CmdState)
    {
        DBG_Printf("L0_SlotIOSchedule CmdState error in slot(%d), CmdState(%d)\n", ulSlotNum, pCurrSlot->CmdState);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }
#endif

    if (pCurrSlot->SQID == 0 )
    {
        DBG_Printf("Handle Admin CMD in L0_SlotIOSchedule()\n");
        DBG_Getch();
    }

    if(TRUE == pCurrSlot->bAbort)
    {
        L0_NVMeForceCompleteCmd(pCurrSlot, ((INVALID_4F << 16) | NVME_SF_ABORT_REQUESTED));
        bFinish = TRUE;
    }
    else
    {   
        bFinish = L0_NVMeProcessNVMCmd(pCurrSlot);
    }
    
    if(TRUE == bFinish)
    {
        ulSlotNum = INVALID_CMD_ID;
    }

    gNvmeMgr.ActiveSlot = ulSlotNum;

    return;
}


INLINE void L0_SlotSchedule(void)
{
    if (TRUE == gNvmeMgr.AdminProcessing)
    {
        L0_SlotAdminSchdule();     /* process host Admin command*/
    }
    else
    {
        L0_SlotIOSchedule();              /* process host IO command */
    }

    return;
}

#else   //AF_ENABLE

U32 L0_NVMeUpdtAFSQEntry(void)
{
    U32 ulSlotNum;
    U32 ulSQID;
    PCB_MGR pCurrCmdSlot;

    ulSlotNum = HAL_HCTSearchCST(SLOT_AFSQE_RDY);

    if (INVALID_CMD_ID != ulSlotNum)
    {
        pCurrCmdSlot = GET_SLOT_MGR(ulSlotNum);
        L0_NVMeRecycleCbMgr(pCurrCmdSlot);

        ulSQID = g_pNVMeCfgExReg->SQEntry[ulSlotNum].SQID;
        pCurrCmdSlot->SQID = ulSQID;
        ASSERT((BIND_CQID(ulSQID)) == (g_pNVMeCfgExReg->SQCfgAttr[ulSQID].CQMaped));
        pCurrCmdSlot->CQID = BIND_CQID(ulSQID); 
        pCurrCmdSlot->SQEntryIndex = g_pNVMeCfgExReg->SQEntry[ulSlotNum].HWRP;
        pCurrCmdSlot->CmdState = CMD_NEW;

        HAL_HwDebugStart(ulSlotNum);
        HAL_HCmdTimerStart(ulSlotNum);

        //It is possible that HW fetch several Admin Command to S0. But we may
        //get a later Admin command.
        UPDATE_SQ_STATE(ulSlotNum);

        HAL_HCTSetCST(ulSlotNum, SLOT_SQENTRY_RDY);
    }
    
    return ulSlotNum;
}


void L0_SlotSchedule(void)
{
    BOOL bFinish;
    PCB_MGR pCurrSlot;
    U32 ulSlotNum;
    U32 ulHwCST;

    ulSlotNum = gNvmeMgr.ActiveSlot;
        
    if (INVALID_CMD_ID == ulSlotNum)
    {
        ulSlotNum = HAL_HCTSearchCST(SLOT_PRPLIST_RDY);

        if (INVALID_CMD_ID == ulSlotNum)
        {
            ulSlotNum = HAL_HCTSearchCST(SLOT_SQENTRY_RDY);

            if (INVALID_CMD_ID == ulSlotNum)
            {
                return;
            }
        }
    }

    pCurrSlot = GET_SLOT_MGR(ulSlotNum);

#ifdef L0_DBG_STATE
    ulHwCST = HAL_HCTGetCST(ulSlotNum);

    if ((SLOT_PRPLIST_RDY != ulHwCST) && (SLOT_SQENTRY_RDY != ulHwCST))
    {
        DBG_Printf("Cst search error(HW) in L0_SlotIOSchedule, slot(%d)\n",ulSlotNum);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }

    if (CMD_IDLE == pCurrSlot->CmdState)
    {
        DBG_Printf("L0_SlotSchedule CmdState error in slot(%d), CmdState(%d)\n", ulSlotNum, pCurrSlot->CmdState);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }
#endif

    if(TRUE == pCurrSlot->bAbort)
    {
        L0_NVMeForceCompleteCmd(pCurrSlot, ((INVALID_4F << 16) | NVME_SF_ABORT_REQUESTED));
        bFinish = TRUE;
    }
    else
    {   
        if (pCurrSlot->SQID == 0)
        {
            bFinish = L0_NVMeProcessAdminCmd(pCurrSlot);
        }
        else
        {
            bFinish = L0_NVMeProcessNVMCmd(pCurrSlot);
        }
    }
    
    if(bFinish == TRUE)
    {
        ulSlotNum = INVALID_CMD_ID;
    }

    gNvmeMgr.ActiveSlot = ulSlotNum;
    return;
}
#endif


#if 0
void L0_PRPSchedule(void)
{
    U32 ulCurrSlotNum;
    PCB_MGR pCurrSlot;

#ifdef AF_ENABLE
    if (FAIL == L0_NVMeUpdtAFSQEntry(&ulCurrSlotNum))
    {
        //Fail to ReapCH indicates no cmd can be reaped. But we may have a slot is CH_RECEIVED state
        ulCurrSlotNum = HAL_HCTSearchCST(SLOT_SQENTRY_RDY);
    }
#else
    //Process Admin CMD first if exists.
    if ( gNvmeMgr.AdminProcessing == TRUE)
    {
        if (SLOT_SQENTRY_RDY == HAL_HCTGetCST(gNvmeMgr.AdminProcessingSlot))
        {
            ulCurrSlotNum = gNvmeMgr.AdminProcessingSlot;
        }
        else
        {
            //Wait HW to handle AdminCMD from READ_CH to RECEIVED.
            return ;
        }
    }
    else    //No Admin CMD in slot. Handle  IO CMD in Slot
    {
        ulCurrSlotNum = HAL_HCTSearchCST(SLOT_SQENTRY_RDY);
    }
#endif 

    if (INVALID_CMD_ID == ulCurrSlotNum)
    {
        return;
    }
    
    pCurrSlot = GET_SLOT_MGR(ulCurrSlotNum);

#ifdef HOST_CMD_REC
    TRACE_LOG((void *)pCurrSlot->Ch,sizeof(COMMAND_HEADER),COMMAND_HEADER,0,"host send cmd");//read pointer
#endif

#ifdef L0_DBG_STATE
    if (SLOT_SQENTRY_RDY != HAL_HCTGetCST(ulCurrSlotNum))
    {
        DBG_Printf("Cst search error(HW) in L0_PRPSchedule, slot(%d)\n",ulCurrSlotNum);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }

    if (CMD_IDLE != pCurrSlot->CmdState)
    {
        DBG_Printf("L0_PRPSchedule CmdState error in slot(%d), CmdState(%d)\n",ulCurrSlotNum,pCurrSlot->CmdState);
        //DBG_Getch();
        L0_WaitHostReset();
        return;
    }
#endif

    DBG_PrintHCmdHeader(ulCurrSlotNum);

    L0_MovePRPtoPRPList(ulCurrSlotNum);
    
    L0_CheckAndReadPRPList(pCurrSlot);

    return;
}
#endif

void L0_NVMeSchedule(void)
{   
    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "nvme schedule start:");

    if (NVME_RUNNING())
    {        
#ifndef AF_ENABLE 
        L0_SQSchedule();                /* fetch command header */
#else
        while (INVALID_CMD_ID != L0_NVMeUpdtAFSQEntry());
#endif
        TL_PERFORMANCE(PTL_LEVEL_DETAIL, "nvme io   schedule finish:");

        //L0_PRPSchedule();               /* fetch prp list       */
        //TL_PERFORMANCE(PTL_LEVEL_DETAIL, "nvme prp  schedule finish:");
        
        L0_SlotSchedule();
        TL_PERFORMANCE(PTL_LEVEL_DETAIL, "nvme Slot schedule finish:");
    }

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "nvme schedule end  :");
    return;
}

BOOL L0_NVMeCcEn(void *p)
{
    //DBG_Printf("L0_NVMe CC EN(%d) Changed, START_NVME()\n", CC_EN());
    L0_NVMeMgrInit(&gNvmeMgr);
    L0_NVMeFeatureTblInit();
    L0_InitLogData();
    
    START_NVME();
    SET_DEV_RDY();
#ifdef AF_ENABLE
    //L0_NVMeCfgExInit();
    START_AF();
    DBG_Printf("Enable AF\n");
#else
    g_ulToProcessBitMap = 0;
#endif
    /* Patch for a hardware issue: enables clock PM instead of host. */
    rPCIe(0xD4) |= (1 << 8);

    DBG_Printf("L0_NVMe CC EN(%d) Changed, START_NVME()Done\n", CC_EN());
    return TRUE;
}


/*====================End of this file========================================*/

