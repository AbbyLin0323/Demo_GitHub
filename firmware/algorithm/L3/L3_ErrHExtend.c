/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L3_ErrHExtend.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "L2_Interface.h"
#include "L2_TableBBT.h"
#include "L3_FCMDQ.h"
#include "L3_Schedule.h"
#include "L3_Debug.h"
#include "L3_BufMgr.h"
#include "L3_ErrHExtend.h"
#include "HAL_MultiCore.h"
#include "L2_ErrorHandling.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
LOCAL U32 *l_aExtHSpareTmp;
LOCAL U8  l_aExtHFlashStatus[SUBSYSTEM_LUN_MAX];

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_ExtHAllcSpareTmpAddr
Input      : U8 ucTLun
             U16 usPage
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.15 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ExtHAllcSpareTmpAddr(U8 ucTLun, U16 usStartPage, U16 usPageCnt)
{
    U16 usOffSet, usPage;
    SDL_NODE *ptNode;

    usOffSet = ucTLun*PG_PER_SLC_BLK*PG_PER_WL + usStartPage;
    for (usPage = 0; usPage < usPageCnt; usPage++)
    {
        ptNode = L3_RedMgrAllocateNode();
        if (NULL == ptNode)
        {
            return FALSE;
        }
        l_aExtHSpareTmp[usOffSet + usPage] = (U32)ptNode;
    }

    return TRUE;
}

/*==============================================================================
Func Name  : L3_ExtHReleaseSpareTmpAddr
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
1. 2016.8.15 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHReleaseSpareTmpAddr(U8 ucTLun, U16 usStartPage, U16 usPageCnt)
{
    U16 usOffSet, usPage;
    SDL_NODE *ptNode;

    usOffSet = ucTLun*PG_PER_SLC_BLK*PG_PER_WL + usStartPage;
    for (usPage = 0; usPage < usPageCnt; usPage++)
    {
        ptNode = (SDL_NODE *)l_aExtHSpareTmp[usOffSet + usPage];
        if (NULL != ptNode)
        {
            L3_RedMgrReleaseNode(ptNode);
            l_aExtHSpareTmp[usOffSet + usPage] = NULL;
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ExtHGetSpareTmpAddr
Input      : U8 ucTLun
             U16 usPage
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL U32 MCU2_DRAM_TEXT L3_ExtHGetSpareTmpAddr(U8 ucTLun, U16 usPage)
{
    SDL_NODE *ptNode;

    ptNode = (SDL_NODE *)l_aExtHSpareTmp[ucTLun*PG_PER_SLC_BLK*PG_PER_WL + usPage];

    return L3_RedMgrGetRedAddrByNode(ptNode);
}

/*==============================================================================
Func Name  : L3_ExtHCalcPageCnt
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHCalcPageCnt(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptCtrlEntry->ptErrHEntry->tExtHCtrl;

    U8 ucSPU = L2_GET_SPU(ptReqEntry->bsTLun);
    U8 ucLUNInSPU = L2_GET_LUN_IN_SPU(ptReqEntry->bsTLun);
    U16 usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;

    ASSERT(FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod);
    ASSERT((FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType) || (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType));

    if (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
    {
        ASSERT(FALSE == L2_IsClosedSuperBlk(ucSPU, usVirBlk));

        ptExtHCtrl->bsClosedBlk = FALSE;
        ptExtHCtrl->bsCpyPageCnt = ptReqEntry->tFlashDesc.bsVirPage;
    }
    else
    {
        if (TRUE == L2_IsClosedSuperBlk(ucSPU, usVirBlk))
        {
            ptExtHCtrl->bsClosedBlk = TRUE;
            ptExtHCtrl->bsCpyPageCnt = PG_PER_SLC_BLK;
        }
        else
        {
            ptExtHCtrl->bsClosedBlk = FALSE;
            ptExtHCtrl->bsCpyPageCnt = L2_GetTargetSuperBlkSubPPO(ptReqEntry->bsTLun, usVirBlk);
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ExtHMarkRedInfo
Input      : U8 ucTLun, U16 usPageNum, U16 usVirBlk
Output     : NONE
Return Val : LOCAL
Discription: if read-src-block uecc/empty-page(ignore), we need to mark the redandunt
             before write to the dest block. help the l2 to detect or check status.
Usage      :
History    :
1. 2017.2.23 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHMarkRedInfo(U8 ucTLun, U16 usPageNum, U16 usVirBlk)
{
    RED *ptRed = (RED*)L3_ExtHGetSpareTmpAddr(ucTLun, usPageNum);

    U32 ulIndex;
    for (ulIndex = 0; ulIndex < LPN_PER_BUF; ulIndex++)
    {
        ptRed->m_DataRed.aCurrLPN[ulIndex] = INVALID_8F;
    }
    ptRed->m_RedComm.ulTimeStamp = 0;
    ptRed->m_RedComm.bsVirBlockAddr = usVirBlk;
    ptRed->m_RedComm.bcPageType = (usPageNum == PG_PER_SLC_BLK - 1) ? PAGE_TYPE_RPMT : PAGE_TYPE_DATA;
    ptRed->m_RedComm.bcBlockType = BLOCK_TYPE_ERROR;

    return;
}
/*==============================================================================
Func Name  : L3_ExtHCopyData
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
            CmdType      Block      Copy Data Handling
            SLC-Write    Open       Page#0~CurPage#, Read meets uecc(ignore), empty-page(getch), until bsCpyPageNum == CurPage#
            SLC-Read     Closed     Page#0~MaxPage#, Read meets uecc(ignore), empty-page(getch), until bsCpyPageNum == MaxPage#
            SLC-Read     Open       Page#0~SubPPO# , Read meets uecc(ignore), empty-page(getch/finish), until bsCpyPageNum == SubPPO#
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
    2. 2017.2.16 JasonGuo modify copy-data handling flow.
==============================================================================*/
LOCAL U32 MCU2_DRAM_TEXT L3_ExtHCopyData(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    U16 usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage;
    U32 ulCopyDataDone;
    BOOL bResult;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptErrHEntry->tExtHCtrl;
    SDL_NODE *ptCurBufNode = (SDL_NODE *)ptExtHCtrl->ulTmpBufAddr;

    ucTLun = ptReqEntry->bsTLun;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usNewPhyBlk = ptExtHCtrl->bsNewPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;

    switch (ptExtHCtrl->bsSubStage)
    {
        case EXTH_SUB_INIT:
        {
            L3_ExtHCalcPageCnt(ptCtrlEntry);

            if (0 == ptExtHCtrl->bsCpyPageCnt)
            {
                ptExtHCtrl->bsSubStage = EXTH_SUB_SUCCESS;
            }
            else
            {
                ptExtHCtrl->bsCpyPageNum = 0;
                if (0 < L3_BufMgrGetFreeCnt() && ptExtHCtrl->bsCpyPageCnt < L3_RedMgrGetFreeCnt())
                {
                    bResult = L3_ExtHAllcSpareTmpAddr(ucTLun, 0, ptExtHCtrl->bsCpyPageCnt);
                    ASSERT(TRUE == bResult);

                    ptCurBufNode = L3_BufMgrAllocateNode();
                    ASSERT(NULL != ptCurBufNode);

                    ptExtHCtrl->ulStatusAddr = (U32)&l_aExtHFlashStatus[ucTLun];
                    ptExtHCtrl->bsSubStage = EXTH_SUB_READ;
                    DBG_Printf("TLun%d Blk%d_%d_%d Page%d_%d ExtH-CpyData-Start\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, ptExtHCtrl->bsCpyPageCnt);
                }
                else
                {
                    //DBG_Printf("TLun%d Blk%d_%d_%d Page%d_%d ExtH-CpyData-AllocFail\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, ptExtHCtrl->bsCpyPageCnt);
                }
            }
            break;
        }
        case EXTH_SUB_READ:
        {
            FCMD_REQ_ENTRY *ptTmpReqEntry = L3_FCMDQAllocTmpReqEntry(ucTLun);
            FCMD_INTR_CTRL_ENTRY *ptNewCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);

            // Alloc a temp read request entry and Fill it.
            ptTmpReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptTmpReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
            ptTmpReqEntry->tFlashDesc.bsVirBlk = usVirBlk;
            ptTmpReqEntry->tFlashDesc.bsPhyBlk = usPhyBlk;
            ptTmpReqEntry->tFlashDesc.bsVirPage = ptExtHCtrl->bsCpyPageNum;
            ptTmpReqEntry->tFlashDesc.bsBlkMod = ptReqEntry->tFlashDesc.bsBlkMod;
            ptTmpReqEntry->tFlashDesc.bsSecStart = 0;
            ptTmpReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[0].bsBufID = L3_BufMgrGetBufIDByNode(ptCurBufNode);
            ptTmpReqEntry->atBufDesc[0].bsSecStart = 0;
            ptTmpReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
            ptTmpReqEntry->ulSpareAddr = L3_ExtHGetSpareTmpAddr(ucTLun, ptExtHCtrl->bsCpyPageNum);
            *(U8*)ptExtHCtrl->ulStatusAddr = SUBSYSTEM_STATUS_PENDING;
            ptTmpReqEntry->ulReqStsAddr = ptExtHCtrl->ulStatusAddr;
            ptTmpReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            // Alloc a new intr ctrl entry and fill it.
            ptNewCtrlEntry->bsIntrReq = TRUE;
            ptNewCtrlEntry->ptReqEntry = ptTmpReqEntry;

            // Send an internal flash request.
            bResult = L3_IFSendNormalFCmd(ptNewCtrlEntry);
            #ifdef SIM
            L3_DbgFCmdPrint(ptNewCtrlEntry, (TRUE == bResult) ? "CpyDataRd_Success" : "CpyDataRd_Fail");
            #endif

            ptExtHCtrl->bsSubStage = EXTH_SUB_READ_CHK;
            //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Rd.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum);
            break;
        }
        case EXTH_SUB_READ_CHK:
        {
            bResult = *(U8*)ptExtHCtrl->ulStatusAddr;
            if (SUBSYSTEM_STATUS_PENDING != bResult)
            {
                if (SUBSYSTEM_STATUS_SUCCESS == bResult || SUBSYSTEM_STATUS_RECC == bResult || SUBSYSTEM_STATUS_RETRY_SUCCESS == bResult)
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE;
                    //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Rd-Success.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum);
                }
                else if (SUBSYSTEM_STATUS_FAIL == bResult)
                {
                    // remain the failed page data or zero-dirty it. here, we remain it.
                    // ...
                    L3_ExtHMarkRedInfo(ucTLun, ptExtHCtrl->bsCpyPageNum, usVirBlk);

                    ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE;
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Rd-Fail.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum);
                }
                else if (SUBSYSTEM_STATUS_EMPTY_PG == bResult)
                {
                    BOOL bHitInvalidEmptyPage;

                    if (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
                    {
                        bHitInvalidEmptyPage = TRUE;
                        DBG_Printf("TLun%d Blk%d_%d Page%d -> %d ExtH-CpyData-Rd-EmptyPage. Write-CopyData-Meets-EmptyPage Fail.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum, ptExtHCtrl->bsCpyPageCnt);
                    }
                    else if ((FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType) && ((TRUE == ptExtHCtrl->bsClosedBlk) || (ptExtHCtrl->bsCpyPageCnt - ptExtHCtrl->bsCpyPageNum > FCMDQ_DEPTH - 1)))
                    {
                        bHitInvalidEmptyPage = TRUE;
                        DBG_Printf("TLun%d Blk%d_%d Page%d -> %d ExtH-CpyData-Rd-EmptyPage. Read-CopyData-Meets-EmptyPage Fail ClosedBlk=%d.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum, ptExtHCtrl->bsCpyPageCnt, ptExtHCtrl->bsClosedBlk);
                    }
                    else
                    {
                        bHitInvalidEmptyPage = FALSE;
                        DBG_Printf("TLun%d Blk%d_%d Page%d -> %d ExtH-CpyData-Rd-EmptyPage Before SubPPO. We cut short CopyData Process.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum, ptExtHCtrl->bsCpyPageCnt);
                    }

                    if (TRUE == bHitInvalidEmptyPage)
                    {
                        #ifdef EXTH_CHECK_READ_EMPTY_PAGE_EN
                        DBG_Getch();
                        #else
                        // Here, we ignore the src-page data. maybe we can zero it before write.
                        // ...
                        L3_ExtHMarkRedInfo(ucTLun, ptExtHCtrl->bsCpyPageNum, usVirBlk);

                        ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE;
                        #endif
                    }
                    else
                    {
                        ptExtHCtrl->bsSubStage = EXTH_SUB_SUCCESS;
                    }
                }
                else
                {
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Rd-Status Error.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum, bResult);
                    DBG_Getch();
                }
            }
            break;
        }
        case EXTH_SUB_WRITE:
        {
            U32 ulTargetRed;
            FCMD_REQ_ENTRY *ptTmpReqEntry = L3_FCMDQAllocTmpReqEntry(ucTLun);
            FCMD_INTR_CTRL_ENTRY *ptNewCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);

            // Alloc a temp write request entry and Fill it.
            ptTmpReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
            ptTmpReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
            ptTmpReqEntry->tFlashDesc.bsVirBlk = usVirBlk;
            ptTmpReqEntry->tFlashDesc.bsPhyBlk = usNewPhyBlk;
            ptTmpReqEntry->tFlashDesc.bsVirPage = ptExtHCtrl->bsCpyPageNum;
            ptTmpReqEntry->tFlashDesc.bsBlkMod = ptReqEntry->tFlashDesc.bsBlkMod;
            ptTmpReqEntry->tFlashDesc.bsSecStart = 0;
            ptTmpReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[0].bsBufID = L3_BufMgrGetBufIDByNode(ptCurBufNode);
            ptTmpReqEntry->atBufDesc[0].bsSecStart = 0;
            ptTmpReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
            ulTargetRed = RED_ABSOLUTE_ADDR(MCU2_ID, ucTLun, ptNewCtrlEntry->bsCtrlPtr);
            COM_MemCpy((U32*)ulTargetRed, (U32*)L3_ExtHGetSpareTmpAddr(ucTLun, ptExtHCtrl->bsCpyPageNum), RED_SW_SZ_DW);
            ptTmpReqEntry->ulSpareAddr = ulTargetRed;
            *(U8*)ptExtHCtrl->ulStatusAddr = SUBSYSTEM_STATUS_PENDING;
            ptTmpReqEntry->ulReqStsAddr = ptExtHCtrl->ulStatusAddr;
            ptTmpReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            // Alloc a new intr ctrl entry and fill it.
            ptNewCtrlEntry->bsIntrReq = TRUE;
            ptNewCtrlEntry->ptReqEntry = ptTmpReqEntry;

            // Send an internal flash request.
            bResult = L3_IFSendNormalFCmd(ptNewCtrlEntry);
            #ifdef SIM
            L3_DbgFCmdPrint(ptNewCtrlEntry, (TRUE == bResult) ? "CpyDataWr_Success" : "CpyDataWr_Fail");
            #endif

            ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE_CHK;
            //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Wr\n", ucTLun, usVirBlk, usNewPhyBlk, ptExtHCtrl->bsCpyPageNum);

            break;
        }
        case EXTH_SUB_WRITE_CHK:
        {
            bResult = *(U8*)ptExtHCtrl->ulStatusAddr;
            if (SUBSYSTEM_STATUS_PENDING != bResult)
            {
                if (SUBSYSTEM_STATUS_SUCCESS != bResult)
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_FAIL;
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Wr-Fail\n", ucTLun, usVirBlk, usNewPhyBlk, ptExtHCtrl->bsCpyPageNum);
                }
                else
                {
                    ptExtHCtrl->bsCpyPageNum++;
                    if (ptExtHCtrl->bsCpyPageNum < ptExtHCtrl->bsCpyPageCnt)
                    {
                        ptExtHCtrl->bsSubStage = EXTH_SUB_READ;
                        //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Wr-Success\n", ucTLun, usVirBlk, usNewPhyBlk, ptExtHCtrl->bsCpyPageNum - 1);
                    }
                    else
                    {
                        ptExtHCtrl->bsSubStage = EXTH_SUB_SUCCESS;
                        DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CpyData-Wr-Done\n", ucTLun, usVirBlk, usNewPhyBlk, ptExtHCtrl->bsCpyPageNum - 1);
                    }
                }
            }
            break;
        }
        default:
        {
            DBG_Printf("CopyData Stage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    ptExtHCtrl->ulTmpBufAddr = (U32)ptCurBufNode;

    ulCopyDataDone = ptExtHCtrl->bsSubStage;
    if (EXTH_SUB_SUCCESS == ulCopyDataDone || EXTH_SUB_FAIL == ulCopyDataDone)
    {
        if (0 < ptExtHCtrl->bsCpyPageCnt)
        {
            L3_BufMgrReleaseNode(ptCurBufNode);
            L3_ExtHReleaseSpareTmpAddr(ucTLun, 0, ptExtHCtrl->bsCpyPageCnt);
        }

        ulCopyDataDone = EXTH_SUB_DONE;
        DBG_Printf("TLun%d Blk%d_%d_%d Page%d_%d ExtH-CpyData-Finish\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, ptExtHCtrl->bsCpyPageNum);
    }

    return ulCopyDataDone;
}

/*==============================================================================
Func Name  : L3_ExtHWriteLastPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.16 JasonGuo create function
==============================================================================*/
LOCAL U32 MCU2_DRAM_TEXT L3_ExtHWriteLastPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    U16 usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, usPhyPage;
    U32 ulWriteDone;
    BOOL bResult;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptErrHEntry->tExtHCtrl;

    ucTLun = ptReqEntry->bsTLun;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usNewPhyBlk = ptExtHCtrl->bsNewPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;

    switch (ptExtHCtrl->bsSubStage)
    {
        case EXTH_SUB_INIT:
        {
            ptExtHCtrl->ulStatusAddr = (U32)&l_aExtHFlashStatus[ucTLun];
            ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE;
            //DBG_Printf("TLun%d Blk%d_%d_%d Page%d_%d ExtH-WriteLastPage-Start\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, usPhyPage);
            break;
        }
        case EXTH_SUB_WRITE:
        {
            U32 ulTargetRed;
            FCMD_REQ_ENTRY *ptTmpReqEntry = L3_FCMDQAllocTmpReqEntry(ucTLun);
            FCMD_INTR_CTRL_ENTRY *ptNewCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);

            // Alloc a temp write request entry and Fill it.
            ptTmpReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
            ptTmpReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
            ptTmpReqEntry->tFlashDesc.bsVirBlk = usVirBlk;
            ptTmpReqEntry->tFlashDesc.bsPhyBlk = usNewPhyBlk;
            ptTmpReqEntry->tFlashDesc.bsVirPage = usVirPage;
            ptTmpReqEntry->tFlashDesc.bsBlkMod = ptReqEntry->tFlashDesc.bsBlkMod;
            ptTmpReqEntry->tFlashDesc.bsSecStart = 0;
            ptTmpReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[0].bsBufID = ptReqEntry->atBufDesc[0].bsBufID;
            ptTmpReqEntry->atBufDesc[0].bsSecStart = 0;
            ptTmpReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
            ulTargetRed = RED_ABSOLUTE_ADDR(MCU2_ID, ucTLun, ptNewCtrlEntry->bsCtrlPtr);
            COM_MemCpy((U32*)ulTargetRed, (U32*)ptReqEntry->ulSpareAddr, RED_SW_SZ_DW);
            ptTmpReqEntry->ulSpareAddr = ulTargetRed;
            *(U8*)ptExtHCtrl->ulStatusAddr = SUBSYSTEM_STATUS_PENDING;
            ptTmpReqEntry->ulReqStsAddr = ptExtHCtrl->ulStatusAddr;
            ptTmpReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            // Alloc a new intr ctrl entry and fill it.
            ptNewCtrlEntry->bsIntrReq = TRUE;
            ptNewCtrlEntry->ptReqEntry = ptTmpReqEntry;

            // Send an internal flash request.
            bResult = L3_IFSendNormalFCmd(ptNewCtrlEntry);
             #ifdef SIM
            L3_DbgFCmdPrint(ptNewCtrlEntry, (TRUE == bResult) ? "WrLstPage_Success" : "WrLstPage_Fail");
            #endif

            ptExtHCtrl->bsSubStage = EXTH_SUB_WRITE_CHK;
            //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-WriteLastPage\n", ucTLun, usVirBlk, usNewPhyBlk, usVirPage);
            break;
        }
        case EXTH_SUB_WRITE_CHK:
        {
            bResult = *(U8*)ptExtHCtrl->ulStatusAddr;
            if (SUBSYSTEM_STATUS_PENDING != bResult)
            {
                if (SUBSYSTEM_STATUS_SUCCESS != bResult)
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_FAIL;
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-WriteLastPage-Fail\n", ucTLun, usVirBlk, usNewPhyBlk, usVirPage);
                }
                else
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_SUCCESS;
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-WriteLastPage-Success\n", ucTLun, usVirBlk, usNewPhyBlk, usVirPage);
                }
            }
            break;
        }
        default:
        {
            DBG_Printf("ExtH WriteLastPage SubStage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    ulWriteDone = ptExtHCtrl->bsSubStage;
    if (EXTH_SUB_SUCCESS == ulWriteDone || EXTH_SUB_FAIL == ulWriteDone)
    {
        ulWriteDone = EXTH_SUB_DONE;
        DBG_Printf("TLun%d Blk%d_%d_%d Page%d_%d ExtH-WriteLastPage-Done\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage, usPhyPage);
    }

    return ulWriteDone;
}

/*==============================================================================
Func Name  : L3_ExtHCheckRpmtPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription: if load rpmt page success, continue the left handle; else, report handle done.
Usage      :
History    :
1. 2016.8.16 JasonGuo create function
==============================================================================*/
LOCAL U32 MCU2_DRAM_TEXT L3_ExtHCheckRpmtPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    U32 ulCheckDone;
    U16 usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage;
    BOOL bResult;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptErrHEntry->tExtHCtrl;
    SDL_NODE *ptCurBufNode = (SDL_NODE *)ptExtHCtrl->ulTmpBufAddr;

    ucTLun = ptReqEntry->bsTLun;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usNewPhyBlk = ptExtHCtrl->bsNewPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;

    switch (ptExtHCtrl->bsSubStage)
    {
        case EXTH_SUB_INIT:
        {
            ptExtHCtrl->bsCpyPageCnt = 1;
            ptExtHCtrl->bsCpyPageNum = PG_PER_SLC_BLK - 1;
            if (0 < L3_BufMgrGetFreeCnt() && 1 < L3_RedMgrGetFreeCnt())
            {
                bResult = L3_ExtHAllcSpareTmpAddr(ucTLun, ucTLun, ptExtHCtrl->bsCpyPageCnt);
                ASSERT(TRUE == bResult);

                ptCurBufNode = L3_BufMgrAllocateNode();
                ASSERT(NULL != ptCurBufNode);

                ptExtHCtrl->ulStatusAddr = (U32)&l_aExtHFlashStatus[ucTLun];
                ptExtHCtrl->bsSubStage = EXTH_SUB_READ;
                DBG_Printf("TLun%d Blk%d_%d_%d Page%d ExtH-CheckRpmt-Start\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage);
            }
            else
            {
                //DBG_Printf("TLun%d Blk%d_%d_%d Page%d ExtH-CheckRpmt-AllocFail\n", ucTLun, usVirBlk, usPhyBlk, usNewPhyBlk, usVirPage);
            }

            break;
        }
        case EXTH_SUB_READ:
        {
            FCMD_REQ_ENTRY *ptTmpReqEntry = L3_FCMDQAllocTmpReqEntry(ucTLun);
            FCMD_INTR_CTRL_ENTRY *ptNewCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);

            // Alloc a temp read request entry and Fill it.
            ptTmpReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptTmpReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
            ptTmpReqEntry->tFlashDesc.bsVirBlk = usVirBlk;
            ptTmpReqEntry->tFlashDesc.bsPhyBlk = usPhyBlk;
            ptTmpReqEntry->tFlashDesc.bsVirPage = ptExtHCtrl->bsCpyPageNum;
            ptTmpReqEntry->tFlashDesc.bsBlkMod = ptReqEntry->tFlashDesc.bsBlkMod;
            ptTmpReqEntry->tFlashDesc.bsSecStart = 0;
            ptTmpReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[0].bsBufID = L3_BufMgrGetBufIDByNode(ptCurBufNode);
            ptTmpReqEntry->atBufDesc[0].bsSecStart = 0;
            ptTmpReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
            ptTmpReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
            ptTmpReqEntry->ulSpareAddr = L3_ExtHGetSpareTmpAddr(ucTLun, ucTLun);
            *(U8*)ptExtHCtrl->ulStatusAddr = SUBSYSTEM_STATUS_PENDING;
            ptTmpReqEntry->ulReqStsAddr = ptExtHCtrl->ulStatusAddr;
            ptTmpReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            // Alloc a new intr ctrl entry and fill it.
            ptNewCtrlEntry->bsIntrReq = TRUE;
            ptNewCtrlEntry->ptReqEntry = ptTmpReqEntry;

            // Send an internal flash request.
            bResult = L3_IFSendNormalFCmd(ptNewCtrlEntry);
            #ifdef SIM
            L3_DbgFCmdPrint(ptNewCtrlEntry, (TRUE == bResult) ? "CheckRpmtRd_Success" : "CheckRpmtRd_Fail");
            #endif

            ptExtHCtrl->bsSubStage = EXTH_SUB_READ_CHK;
            //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CheckRmpt-Rd.\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum);
            break;
        }
        case EXTH_SUB_READ_CHK:
        {
            bResult = *(U8*)ptExtHCtrl->ulStatusAddr;
            if (SUBSYSTEM_STATUS_PENDING != bResult)
            {
                // if check-rpmt meets UECC or EmptyPage, return sub-fail, then skip the UECC extend block replacement, l2 will handle it.
                if (SUBSYSTEM_STATUS_FAIL == bResult || SUBSYSTEM_STATUS_EMPTY_PG == bResult)
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_FAIL;
                    DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CheckRpmt-Rd-Fail Status=%d\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum, bResult);
                }
                else
                {
                    ptExtHCtrl->bsSubStage = EXTH_SUB_SUCCESS;
                    //DBG_Printf("TLun%d Blk%d_%d Page%d ExtH-CheckRpmt-Rd-Success\n", ucTLun, usVirBlk, usPhyBlk, ptExtHCtrl->bsCpyPageNum);
                }
            }
            break;
        }
        default:
        {
            DBG_Printf("ExtH UECC CheckRpmt Stage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    ptExtHCtrl->ulTmpBufAddr = (U32)ptCurBufNode;

    ulCheckDone = ptExtHCtrl->bsSubStage;
    if (EXTH_SUB_SUCCESS == ptExtHCtrl->bsSubStage || EXTH_SUB_FAIL == ptExtHCtrl->bsSubStage)
    {
        if (0 < ptExtHCtrl->bsCpyPageCnt)
        {
            L3_BufMgrReleaseNode(ptCurBufNode);
            L3_ExtHReleaseSpareTmpAddr(ucTLun, ucTLun, ptExtHCtrl->bsCpyPageCnt);
        }

        ulCheckDone = EXTH_SUB_DONE;
    }

    return ulCheckDone;
}

/*==============================================================================
Func Name  : L3_ExtHPrcUECC
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHPrcUECC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptErrHEntry->tExtHCtrl;

    U8 ucTLun, ucSPU, ucLunInSPU, ucCmdType, ucPln, ucErrCode, ucSecStart, ucSecLen, ucLpnBitmap;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage, usNewPhyBlk, usNewVirBlk = INVALID_4F;
    U32 ulLBA;
    BOOL bTLCBlk;

    ucTLun = ptReqEntry->bsTLun;
    ucCmdType = ptCtrlEntry->bsCmdType;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;

    usNewPhyBlk = ptExtHCtrl->bsNewPhyBlk;

    ucSPU = L2_GET_SPU(ucTLun);
    ucLunInSPU = L2_GET_LUN_IN_SPU(ucTLun);
    bTLCBlk = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
    // Double check the Request Block type and the VBT info.
    ASSERT(bTLCBlk == L2_VBT_Get_TLC(ucSPU, usVirBlk));

    // only support SLC block extend error handling now.
    ASSERT(bTLCBlk == FALSE);

    switch (ptExtHCtrl->bsStage)
    {
        case EXTH_INIT:
        {
            BOOL bNeedCheckRpmt = FALSE;

            // if the current super block is closed, the rpmt is written, we need to check the rpmt firstly.
            // if the current super block is the target block, but the current Lun's last page (rpmt) has been written(l2 only push the write-fcmd to FCMDQ, not write to NandArray), we need to check the rpmt fistly.
            if (TRUE == L2_IsClosedSuperBlk(ucSPU, usVirBlk))
            {
                bNeedCheckRpmt = TRUE;
            }
            else if (PG_PER_SLC_BLK == L2_GetTargetSuperBlkSubPPO(ucTLun, usVirBlk))
            {
                bNeedCheckRpmt = TRUE;
            }

            if (TRUE == bNeedCheckRpmt)
            {
                ptExtHCtrl->bsStage = EXTH_CHECK_RPMT;
                ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC Start [CheckRpmt]\n",
                    ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
            }
            else
            {
                ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
                ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC Start.\n",
                    ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
            }

            break;
        }
        case EXTH_CHECK_RPMT:
        {
            if (EXTH_SUB_DONE == L3_ExtHCheckRpmtPage(ptCtrlEntry))
            {
                if (EXTH_SUB_SUCCESS == ptExtHCtrl->bsSubStage)
                {
                    ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;

                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC Closed Block check Rpmt Success.\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
                }
                else
                {
                    ptExtHCtrl->bsStage = EXTH_REPORT_STS;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;

                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC Closed Block check Rmpt Fail.\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
                }
            }

            break;
        }
        case EXTH_ALLOC_NEWBLK:
        {
            usNewPhyBlk = L2_ErrHApplyNewPBN(ucSPU, ucLunInSPU, usVirBlk, bTLCBlk);

            ptExtHCtrl->bsNewPhyBlk = usNewPhyBlk;
            ptExtHCtrl->bsStage = EXTH_COPY_DATA;
            ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;

            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC AllocBlk[%d, %d]\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, usNewVirBlk, usNewPhyBlk);

            break;
        }
        case EXTH_COPY_DATA:
        {
            if (EXTH_SUB_DONE == L3_ExtHCopyData(ptCtrlEntry))
            {
                if (EXTH_SUB_SUCCESS != ptExtHCtrl->bsSubStage)
                {
                    L2_PBIT_Set_Error(ucSPU, ucLunInSPU, usNewPhyBlk, TRUE);
                    L2_PBIT_Set_Free(ucSPU, ucLunInSPU, usNewPhyBlk, FALSE);
                    L2_BbtAddBbtBadBlk(ucTLun, usNewPhyBlk, RT_BAD_BLK, WRITE_ERR);

                    ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC CopyData Fail\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
                }
                else
                {
                    L2_ErrHRecostructMapping(ucSPU, ucLunInSPU, usVirBlk, usNewPhyBlk, READ_ERR);
#ifdef ERRH_MODIFY_PENDING_CMD
                    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ERRH_PEND_CMD);
                    /*Modify Pending command param that physical block has been replaced*/
                    L3_ExtHPrcPendingCmd(ucTLun, usVirBlk, usNewPhyBlk, TRUE);
                    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ERRH_PEND_CMD);
#endif

                    ptExtHCtrl->bsStage = EXTH_REPORT_STS;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC CopyData Success[%d to %d]\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, usPhyBlk, usNewPhyBlk);
                }
            }
            break;
        }
        case EXTH_REPORT_STS:
        {
            L3_IFUpdtReqStatus(ptCtrlEntry);
            L3_FCMDQSetIntrRptr(ucTLun, INVALID_DPTR, TRUE);

            L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, TRUE);

            ptExtHCtrl->bsStage = EXTH_DONE;
            ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_UECC Done\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
            break;
        }
        default:
        {
            DBG_Printf("ExtH UECC Stage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ExtHPrcPRG
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHPrcPRG(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    EXTD_ERRH_CTRL *ptExtHCtrl = &ptErrHEntry->tExtHCtrl;

    U8 ucTLun, ucSPU, ucLunInSPU, ucCmdType, ucPln, ucErrCode, ucSecStart, ucSecLen, ucLpnBitmap;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage, usNewPhyBlk;
    U32 ulLBA;
    BOOL bTLCBlk;

    ucTLun = ptReqEntry->bsTLun;
    ucCmdType = ptCtrlEntry->bsCmdType;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;

    usNewPhyBlk = ptExtHCtrl->bsNewPhyBlk;

    ucSPU = L2_GET_SPU(ucTLun);
    ucLunInSPU = L2_GET_LUN_IN_SPU(ucTLun);
    bTLCBlk = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
    // Double check the Request Block type and the VBT info.
    ASSERT(bTLCBlk == L2_VBT_Get_TLC(ucSPU, usVirBlk));

    switch (ptExtHCtrl->bsStage)
    {
        case EXTH_INIT:
        {
            ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
            ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG Start\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA);
        }
        case EXTH_ALLOC_NEWBLK:
        {
            usNewPhyBlk = L2_ErrHApplyNewPBN(ucSPU, ucLunInSPU, usVirBlk, bTLCBlk);

            ptExtHCtrl->bsNewPhyBlk = usNewPhyBlk;

            ptExtHCtrl->bsStage = EXTH_COPY_DATA;
            ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;

            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG AllocBlk[%d]\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA, usNewPhyBlk);

            break;
        }
        case EXTH_COPY_DATA:
        {
            if (EXTH_SUB_DONE == L3_ExtHCopyData(ptCtrlEntry))
            {
                if (EXTH_SUB_SUCCESS != ptExtHCtrl->bsSubStage)
                {
                    L2_PBIT_Set_Error(ucSPU, ucLunInSPU, usNewPhyBlk, TRUE);
                    L2_PBIT_Set_Free(ucSPU, ucLunInSPU, usNewPhyBlk, FALSE);
                    L2_BbtAddBbtBadBlk(ucTLun, usNewPhyBlk, RT_BAD_BLK, WRITE_ERR);

                    ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ExtH_PRG CopyData Fail\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
                }
                else
                {
                    ptExtHCtrl->bsStage = EXTH_WRITE_LASTPAGE;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG CopyData Success[%d to %d]\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA, usPhyBlk, usNewPhyBlk);
                }
            }
            break;
        }
        case EXTH_WRITE_LASTPAGE:
        {
            if (EXTH_SUB_DONE == L3_ExtHWriteLastPage(ptCtrlEntry))
            {
                if (EXTH_SUB_SUCCESS != ptExtHCtrl->bsSubStage)
                {
                    L2_PBIT_Set_Error(ucSPU, ucLunInSPU, usNewPhyBlk, TRUE);
                    L2_PBIT_Set_Free(ucSPU, ucLunInSPU, usNewPhyBlk, FALSE);
                    L2_BbtAddBbtBadBlk(ucTLun, usNewPhyBlk, RT_BAD_BLK, WRITE_ERR);

                    ptExtHCtrl->bsStage = EXTH_ALLOC_NEWBLK;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG WriteLastPage Fail [%d to %d]\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA, usPhyBlk, usNewPhyBlk);
                }
                else
                {
                    L2_ErrHRecostructMapping(ucSPU, ucLunInSPU, usVirBlk, usNewPhyBlk, WRITE_ERR);

#ifdef ERRH_MODIFY_PENDING_CMD
                    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ERRH_PEND_CMD);
                    /*Modify Pending command param that physical block has been replaced*/
                    L3_ExtHPrcPendingCmd(ucTLun, usVirBlk, usNewPhyBlk, TRUE);
                    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ERRH_PEND_CMD);
#endif

                    ptExtHCtrl->bsStage = EXTH_REPORT_STS;
                    ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG WriteLastPage Success[%d to %d]\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA, usPhyBlk, usNewPhyBlk);
                }
            }
            break;
        }
        case EXTH_REPORT_STS:
        {
            L3_IFUpdtReqStatus(ptCtrlEntry);
            L3_FCMDQSetIntrRptr(ucTLun, INVALID_DPTR, TRUE);

            L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, TRUE);

            ptExtHCtrl->bsStage = EXTH_DONE;
            ptExtHCtrl->bsSubStage = EXTH_SUB_INIT;
            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d LBA=0x%x ExtH_PRG Done\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ulLBA);
            break;
        }
        default:
        {
            DBG_Printf("ExtH PRG Stage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ExtHPrcErrFCmd
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.26 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ExtHPrcErrFCmd(U8 ucTLun)
{
    U8 ucCurEptr;
    BOOL bContinue = FALSE;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    ucCurEptr = L3_FCMDQGetIntrEptr(ucTLun, TRUE);
    if (INVALID_DPTR == ucCurEptr)
    {
        return TRUE;
    }

    ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurEptr, TRUE);
    switch (ptCtrlEntry->ptErrHEntry->bsErrCode)
    {
        case NF_ERR_TYPE_UECC:
        case NF_ERR_TYPE_DCRC:
        {
            L3_ExtHPrcUECC(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_PRG:
        case NF_ERR_TYPE_PREPRG:
        case NF_ERR_TYPE_BOTHPRG:
        {
            L3_ExtHPrcPRG(ptCtrlEntry);
            break;
        }
        default:
        {
            DBG_Printf("ExtH meets no-support ErrCode. 0x%x", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return FALSE;
}

/*==============================================================================
Func Name  : L3_ExtHPrcPendFCmd
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : now, the nfcq is empty, push all the pending fcmds to nfc directly.
History    :
    1. 2016.7.26 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHPrcPendFCmd(U8 ucTLun)
{
    U8 ucCurPptr, ucCurWptr, ucCurCtrlPtr;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, *ptCtrlBakEntry;

    ASSERT(INVALID_DPTR == L3_FCMDQGetIntrEptr(ucTLun, TRUE));

    ucCurWptr = L3_FCMDQGetIntrWptr(ucTLun, TRUE);
    ucCurPptr = L3_FCMDQGetIntrPptr(ucTLun, TRUE);

    while ((INVALID_DPTR != ucCurPptr) && (ucCurPptr != ucCurWptr))
    {
        ptCtrlBakEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurPptr, TRUE);
        ASSERT(NULL == ptCtrlBakEntry->ptErrHEntry);

        ptCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);
        ucCurCtrlPtr = ptCtrlEntry->bsCtrlPtr;

        COM_MemCpy((U32*)ptCtrlEntry, (U32*)ptCtrlBakEntry, sizeof(FCMD_INTR_CTRL_ENTRY) >> DWORD_SIZE_BITS);
        ptCtrlEntry->bsCtrlPtr = ucCurCtrlPtr;

        #ifdef SIM
        L3_DbgFCmdCntDec(ptCtrlEntry);
        #endif
        L3_IFSendFCmd(ptCtrlEntry);

        ucCurPptr = (ucCurPptr + 1) % NFCQ_DEPTH;
    }

    L3_FCMDQSetIntrPptr(ucTLun, INVALID_DPTR, TRUE);
    L3_SchClrStsBit(ucTLun, STS_BMP_EXTH);

    return;
}

/*==============================================================================
Func Name  : L3_ExtHIntrQBackUp
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ExtHIntrQBackUp(U8 ucTLun)
{
    U8 ucBakUpPtr, ucCurEptr, ucCurWptr;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, *ptCtrlBakEntry;

    ucCurWptr = L3_FCMDQGetIntrWptr(ucTLun, FALSE);
    ucBakUpPtr = ucCurEptr = L3_FCMDQGetIntrEptr(ucTLun, FALSE);

    do
    {
        ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucBakUpPtr, FALSE);
        ptCtrlBakEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucBakUpPtr, TRUE);

        COM_MemCpy((U32*)ptCtrlBakEntry, (U32*)ptCtrlEntry, sizeof(FCMD_INTR_CTRL_ENTRY) >> DWORD_SIZE_BITS);
        if (ucBakUpPtr == ucCurEptr)
        {
            ptCtrlBakEntry->ptErrHEntry = L3_FCMDQGetIntrErrHEntry(ucTLun, TRUE);
            COM_MemCpy((U32*)ptCtrlBakEntry->ptErrHEntry, (U32*)ptCtrlEntry->ptErrHEntry, sizeof(FCMD_INTR_ERRH_ENTRY) >> DWORD_SIZE_BITS);
        }

        ucBakUpPtr = (ucBakUpPtr + 1) % NFCQ_DEPTH;
    } while (ucBakUpPtr != ucCurWptr);

    return TRUE;
}

/*==============================================================================
Func Name  : L3_ExtHIntrDptrReset
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ExtHIntrDptrReset(U8 ucTLun)
{
    L3_FCMDQSetIntrWptr(ucTLun, L3_FCMDQGetIntrWptr(ucTLun, FALSE), TRUE);
    L3_FCMDQSetIntrRptr(ucTLun, L3_FCMDQGetIntrRptr(ucTLun, FALSE), TRUE);
    L3_FCMDQSetIntrEptr(ucTLun, L3_FCMDQGetIntrEptr(ucTLun, FALSE), TRUE);
    L3_FCMDQSetIntrPptr(ucTLun, L3_FCMDQGetIntrPptr(ucTLun, FALSE), TRUE);

    L3_FCMDQSetIntrWptr(ucTLun, 0, FALSE);
    L3_FCMDQSetIntrRptr(ucTLun, INVALID_DPTR, FALSE);
    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
    L3_FCMDQSetIntrPptr(ucTLun, INVALID_DPTR, FALSE);

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_ExtHAllcDram
Input      : U32 *pFreeDramBase
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ExtHAllcDram(U32 *pFreeDramBase)
{
    U32 ulFreeBase = *pFreeDramBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // Backup for SetFeatureCommand
    g_ptFCmdReqBak2 = (FCMD_REQ_ENTRY *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_NUM * sizeof(FCMD_REQ_ENTRY));
    COM_MemAddr16DWAlign(&ulFreeBase);

    // FCMD_INTR_BAKUP mem
    g_ptFCmdReqBak = (FCMD_REQ_ENTRY *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_NUM * sizeof(FCMD_REQ_ENTRY));
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_ptFCmdIntrDptrBak = (FCMD_INTR_DPTR *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_DPTR_SZ_BAK);
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_ptFCmdIntrCtrlBak = (FCMD_INTR_CTRL *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_CTRL_SZ_BAK);
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_ptFCmdIntrErrHBak = (FCMD_INTR_ERRH *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_ERRH_SZ_BAK);
    COM_MemAddr16DWAlign(&ulFreeBase);

    // RED Mem
    l_aExtHSpareTmp = (U32 *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_NUM*PG_PER_SLC_BLK*PG_PER_WL*sizeof(U32));
    COM_MemZero(l_aExtHSpareTmp, SUBSYSTEM_LUN_NUM*PG_PER_SLC_BLK*PG_PER_WL);
    COM_MemAddr16DWAlign(&ulFreeBase);

    ASSERT(ulFreeBase-DRAM_DATA_BUFF_MCU2_BASE < DATA_BUFF_MCU2_SIZE);
    *pFreeDramBase = ulFreeBase;

    return;
}

/*==============================================================================
Func Name  : L3_ExtHHandling
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.26 JasonGuo create function
==============================================================================*/
void L3_ExtHHandling(U8 ucTLun)
{
    if (TRUE == L3_ExtHPrcErrFCmd(ucTLun))
    {
        L3_ExtHPrcPendFCmd(ucTLun);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ExtHTrigger
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
BOOL MCU2_DRAM_TEXT L3_ExtHTrigger(U8 ucTLun)
{
    BOOL bResult;

    bResult = L3_ExtHIntrQBackUp(ucTLun);
    if (TRUE == bResult)
    {
        L3_ExtHIntrDptrReset(ucTLun);

        L3_SchSetStsBit(ucTLun, STS_BMP_EXTH);
    }

    return bResult;
}

/*==============================================================================
Func Name  : L3_ExtHPrcPendingCmd
Input      :   U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2017.1.10 LeoYang create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ExtHPrcPendingCmd(U8 ucTLun, U16 usVirBlk, U16 usPhyBlk, BOOL bBak)
{
    U8 ucCurWptr, ucPtr;  /*L3 write pointer and pending poing*/
    U8 ucRptr, ucWptr;    /* L2 FCMDQ read pointer and write pointer*/
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;
    FCMD_REQ_ENTRY *ptReqEntry;

    /*Fix L2 FCMQQ cmd phsical block number*/
    ucRptr = L3_FCMDQGetReqRptr(ucTLun, 0);
    ucWptr = L2_FCMDQGetReqWptr(ucTLun, 0);
    while (ucRptr != ucWptr)
    {
        ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, 0, ucRptr);

        if (ptReqEntry->tFlashDesc.bsVirBlk == usVirBlk)
        {
            ptReqEntry->tFlashDesc.bsPhyBlk = usPhyBlk;
        }

        ucRptr = (ucRptr + 1) % NFCQ_DEPTH;
    }

    /*Fix L3 pending cmd physical block number*/
    ucCurWptr = L3_FCMDQGetIntrWptr(ucTLun, bBak);
    ucPtr = L3_FCMDQGetIntrPptr(ucTLun, bBak);

    while ((INVALID_DPTR != ucPtr) && (ucCurWptr != ucPtr))
    {
        ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucPtr, bBak);

        if (ptCtrlEntry->ptReqEntry->tFlashDesc.bsVirBlk == usVirBlk)
        {
            ptCtrlEntry->ptReqEntry->tFlashDesc.bsPhyBlk = usPhyBlk;
        }

        ucPtr = (ucPtr + 1) % NFCQ_DEPTH;
    }

    return;
}



/*====================End of this file========================================*/

