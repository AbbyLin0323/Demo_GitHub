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
Filename    :L2_DbgMain.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.01
Description :Main function for Debug L2
Others      :
Modify      :
****************************************************************************/

#include "HAL_Inc.h"
#include "HAL_FlashDriverExt.h"
#include "FW_Event.h"
#include "L1_Inc.h"
#include "L2_Interface.h"
#include "L2_Debug.h"
#include "L2_Thread.h"
#include "L2_Schedule.h"
#include "L2_TblChk.h"
#include "L2_PMTManager.h"
#include "L2_FTL.h"
#include "L2_FCMDQ.h"
#include "HAL_TraceLog.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"

//specify file name for Trace Log
#define TL_FILE_NUM  L2_Debug_c

extern GLOBAL  U32 g_L2TempBufferAddr;
extern void L1_DbgPrintWriteBufReq(BUF_REQ_WRITE *pBufReq);
extern void L1_DbgPrintReadBufReq(BUF_REQ_READ *pBufReq);

U32 MCU1_DRAM_TEXT L2_DbgReadOnePage(FLASH_ADDR FlashAddr, U16 usBufferId)
{
    U8  ucErrCode;
    U32 ulResult;
    NFC_READ_REQ_DES tRdDes = { 0 };
    NFC_READ_REQ_DES tRetryDes = { 0 };

    /* wait L3 free */
    if (FALSE == L2_FCMDQIsAllEmpty())
    {
        return FAIL;
    }

    /* send read command */
    //usBufferId = COM_GetBufferIDByMemAddr(g_L2TempBufferAddr, TRUE, BUF_SIZE_BITS);
    tRdDes.bsSecStart = 0;
    tRdDes.bsSecLen = SEC_PER_BUF;
    tRdDes.bsRedOntf = TRUE;
    tRdDes.bsRdBuffId = usBufferId;
    tRdDes.ppNfcRed = NULL;
    tRdDes.pErrInj = NULL;

    ulResult = HAL_NfcPageRead(&FlashAddr, &tRdDes);
    if (NFC_STATUS_SUCCESS != ulResult)// l84mod
    {
        DBG_Getch();
    }
    /* wait read command done */
    tRetryDes.bsSecStart = 0;
    tRetryDes.bsSecLen = SEC_PER_BUF;
    tRetryDes.bsRdBuffId = usBufferId;
    tRetryDes.bsRedOntf = TRUE;
    tRetryDes.ppNfcRed = NULL;
    tRetryDes.pErrInj = NULL;

    ucErrCode = HAL_NfcWaitStatusWithRetry(&FlashAddr, &tRetryDes, NF_PRCQ_READ_MULTIPLN);
    if (NF_SUCCESS != ucErrCode)
    {
        DBG_Getch();
    }

    return SUCCESS;
}

U32 MCU1_DRAM_TEXT L2_GCDirtyCntCheck(U8 ucSuperPu, U16 usVBN)
{
    U32 ppo;
    U32 LpnInPage;
    U32 DirtyCnt = 0;
    U32 ulTemp;
    PhysicalAddr TempAddr = { 0 };
    U32 OffsetInSuperPage;

    /* check dirty count */
    DirtyCnt = 0; //LPN_PER_SUPERBUF;
    TempAddr.m_PPN = 0;
    TempAddr.m_PUSer = ucSuperPu;
    TempAddr.m_BlockInPU = usVBN;
    for (ppo = 0; ppo < PG_PER_SLC_BLK - 1; ppo++)
    {
        for (OffsetInSuperPage = 0; OffsetInSuperPage < LUN_NUM_PER_SUPERPU; OffsetInSuperPage++)
        {
            TempAddr.m_OffsetInSuperPage = OffsetInSuperPage;
            for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
            {
                TempAddr.m_PageInBlock = ppo;
                TempAddr.m_LPNInPage = LpnInPage;

                if (FALSE == L2_LookupDirtyLpnMap(&TempAddr))
                {
                    DirtyCnt++;
                }
                else
                {
                    DBG_Printf("SuperPu %d LUN %d, block %d, page %d, offset %d is valid!!!\n", ucSuperPu, OffsetInSuperPage, usVBN, ppo, LpnInPage);
                    //DBG_Printf("valid lpn is 0x%x\n", ulLpn);
                }
            }
        }
    }

    ulTemp = L2_GetDirtyCnt(ucSuperPu, usVBN);
    if (ulTemp != DirtyCnt)
    {
        DBG_Printf("DirtyCnt check error! ce %d, block %d\n", ucSuperPu, usVBN);
        DBG_Printf("DirtyCnt calculate = %d, DirtyCnt record = %d\n", DirtyCnt, ulTemp);
        DBG_Getch();
    }

    return TRUE;
}
/****************************************************************************
Name        :L2_DbgDirtyCntCheck
Input       :U8 ucPuNum, U16 usVBN
Output      :
Author      :HenryLuo
Date        :2013.02.07    11:04:57
Description :check the dirty count in certain block.
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_DbgDirtyCntCheck(U8 ucPuNum, U16 usVBN)
{
    FLASH_ADDR FlashAddr = { 0 };
    U32 ulLUNInSuperPU;
    RPMT* pRPMT;
    U16 usBufferId;
    U8  ucErrCode;
    U32 ppo;
    U32 LpnInPage;
    U32 DirtyCnt = 0;
    U32 ulLpn;
    U32 ulTemp;
    PhysicalAddr TempAddr = { 0 }, RefAddr = { 0 };
    NFC_READ_REQ_DES tRdDes = { 0 };    

    /* if block is the current target block in Pu, no need to load rpmt */
    if (VBT_NOT_TARGET != pVBT[ucPuNum]->m_VBT[usVBN].Target)
    {
        return TRUE;
    }

    /* if block have no been writen, no need to load rpmt */
    if (INVALID_4F == pVBT[ucPuNum]->m_VBT[usVBN].StripID)
    {
        return TRUE;
    }

    /* load RPMT page into L2 temp buffer */
    pRPMT = (RPMT*)g_L2TempBufferAddr;
    usBufferId = COM_GetBufferIDByMemAddr((U32)pRPMT, TRUE, BUF_SIZE_BITS);
    for (ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
    {
        FlashAddr.ucPU = ucPuNum;
        FlashAddr.ucLun = ulLUNInSuperPU;
        FlashAddr.usBlock = L2_VBT_GetPhysicalBlockAddr(ucPuNum, 0, usVBN);
        FlashAddr.usPage = PG_PER_SLC_BLK - 1;

        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = SEC_PER_BUF;
        tRdDes.bsRedOntf = TRUE;
        tRdDes.bsRdBuffId = usBufferId;
        tRdDes.ppNfcRed = NULL;
        tRdDes.pErrInj = NULL;

        if (NFC_STATUS_SUCCESS != HAL_NfcPageRead(&FlashAddr, &tRdDes))// l84mod
        {
            DBG_Printf("RPMT table load Error !!! \n");
            DBG_Getch();
        }

        /* wait rpmt load done */
        ucErrCode = HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun);
        if (NF_SUCCESS != ucErrCode)
        {
            DBG_Printf("RPMT table read Error !!! ce %d, block %d\n", ucPuNum, usVBN);
            DBG_Getch();
        }
        else
        {
            if ((pRPMT->m_RPMT[ulLUNInSuperPU].m_SuperPU != ucPuNum) || (pRPMT->m_RPMT[ulLUNInSuperPU].m_SuperBlock != usVBN))
            {
                DBG_Printf("RPMT table read Error !!! ce %d, block %d\n", ucPuNum, usVBN);
                DBG_Getch();
            }
        }

        /* check dirty count */
        DirtyCnt = 0; //LPN_PER_BUF;
        TempAddr.m_PPN = 0;
        TempAddr.m_PUSer = ucPuNum;
        TempAddr.m_OffsetInSuperPage = ulLUNInSuperPU;
        TempAddr.m_BlockInPU = usVBN;
        for (ppo = 0; ppo < PG_PER_SLC_BLK - 1; ppo++)
        {
            for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
            {
                TempAddr.m_PageInBlock = ppo;
                TempAddr.m_LPNInPage = LpnInPage;

                ulLpn = L2_LookupRPMT(pRPMT, &TempAddr);
                if (INVALID_8F == ulLpn)
                {
                    DirtyCnt++;
                    if (TRUE == L2_LookupDirtyLpnMap(&TempAddr))
                    {
                        DBG_Printf("INVALID_8F 0x%x,TempAddr 0x%x\n", ulLpn, TempAddr);
                        DBG_Getch();
                    }
                    continue;
                }

                /* lookup PMT to get reference physical addr from logic LPN */
                L2_LookupPMT(&RefAddr, ulLpn,FALSE);

                /* if not equel, data is dirty */
                if (TempAddr.m_PPN != RefAddr.m_PPN)
                {
                    DirtyCnt++;
                    if (TRUE == L2_LookupDirtyLpnMap(&TempAddr))
                    {
                        DBG_Printf("LPN 0x%x,TempAddr 0x%x\n", ulLpn, TempAddr);
                        DBG_Printf("TempAddr : PU %d Blk %d Page %d LpnInPage %d\n",
                            TempAddr.m_PUSer, TempAddr.m_BlockInPU, TempAddr.m_PageInBlock, TempAddr.m_LPNInPage);
                        DBG_Printf("RefAddr : PU %d Blk %d Page %d LpnInPage %d\n",
                            RefAddr.m_PUSer, RefAddr.m_BlockInPU, RefAddr.m_PageInBlock, RefAddr.m_LPNInPage);
                        DBG_Getch();
                    }
                }
                else
                {
                    if (FALSE == L2_LookupDirtyLpnMap(&TempAddr))
                    {
                        DBG_Printf("LPN 0x%x,TempAddr 0x%x, false!\n", ulLpn, TempAddr);
                        DBG_Getch();
                    }
                    //DBG_Printf("ce %d, block %d, page %d, offset %d is valid!!!\n", ucPuNum, usVBN, ppo, LpnInPage);
                    //DBG_Printf("valid lpn is 0x%x\n", ulLpn);
                }
            }
        }

        ulTemp = L2_GetDirtyCnt(ucPuNum, usVBN);
        if (ulTemp != DirtyCnt)
        {
            DBG_Printf("DirtyCnt check error! ce %d, block %d\n", ucPuNum, usVBN);
            DBG_Printf("DirtyCnt calculate = 0x%x, DirtyCnt record = 0x%x\n", DirtyCnt, ulTemp);
            DBG_Getch();
        }
    }

    return TRUE;
}

/****************************************************************************
Name        :L2_PMTItemChk
Input       :U32 ulLpn
Output      :
Author      :HenryLuo
Date        :2013.03.22    10:00:57
Description :check the PMT Item of certain LPN.
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_PMTItemChk(U32 ulLpn)
{
    PhysicalAddr Addr = { 0 };
    FLASH_ADDR FlashAddr = { 0 };
    PuInfo* pInfo;
    U32 ulRefLpn;
    RPMT* pRPMT;
    U8 ucPU;
    U32 ulRPMTPointer;
    U8 ucTargetType;
    U16 usPhysicalBlock;
    U16 usBufferId;

    L2_LookupPMT(&Addr, ulLpn, FALSE);
    if (INVALID_8F != Addr.m_PPN)
    {
        usPhysicalBlock = L2_VBT_GetPhysicalBlockAddr(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);
        if (INVALID_4F == usPhysicalBlock)
        {
            DBG_Printf("%s : pblk %d error\n", __FUNCTION__, usPhysicalBlock);
            DBG_Getch();
        }

        ucPU = L2_GetSuperPuFromLPN(ulLpn);
        pInfo = g_PuInfo[ucPU];
        ucTargetType = pVBT[Addr.m_PUSer]->m_VBT[Addr.m_BlockInPU].Target;
        if (VBT_TARGET_HOST_W == ucTargetType)
        {
            ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML];
            pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][ulRPMTPointer];
        }
        else if (VBT_TARGET_HOST_GC == ucTargetType)
        {
            ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_GC];
            pRPMT = pInfo->m_pRPMT[TARGET_HOST_GC][ulRPMTPointer];
        }
        else if (VBT_NOT_TARGET == ucTargetType)
        {
            /* load RPMT */
            FlashAddr.ucPU = Addr.m_PUSer;
            FlashAddr.ucLun= Addr.m_OffsetInSuperPage;
            FlashAddr.usBlock = L2_VBT_GetPhysicalBlockAddr(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);
            FlashAddr.usPage = (PG_PER_SLC_BLK - 1);
            usBufferId = COM_GetBufferIDByMemAddr(g_L2TempBufferAddr, TRUE, BUF_SIZE_BITS);
            if (FAIL == L2_DbgReadOnePage(FlashAddr, usBufferId))
            {
                return FALSE;
            }
            pRPMT = (RPMT*)g_L2TempBufferAddr;
        }
        else
        {
            DBG_Printf("%s target %d error\n", __FUNCTION__, ucTargetType);
            DBG_Getch();
        }

        ulRefLpn = L2_LookupRPMT(pRPMT, &Addr);
        if (ulRefLpn != ulLpn)
        {
            DBG_Printf("table check error at ce %d block %d page %d lpn %d\n",
                Addr.m_PUSer, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_LPNInPage);
            DBG_Printf("ulLpn = 0x%x, ulRefLpn = 0x%x\n", ulLpn, ulRefLpn);
            DBG_Getch();
        }
    }

    return TRUE;
}
/****************************************************************************
Name        :
Input       :
Output      :
Author      :HenryLuo
Date        :2012.12.10    10:30:11
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_DbgReportStatus()
{
#if 0
    SystemState CurStatus;
    ThreadType CurEntry;
    U8 ucPu;

    //DBG_Printf("/********************L2 status report**********************/ \n");
    for (ucPu = 0; ucPu < SUBSYSTEM_PU_NUM; ucPu++)
    {
        CurStatus = L2_GetCurrState(ucPu);
        switch(CurStatus)
        {
        case SYSTEM_STATE_HOST_RW:
            //DBG_Printf("system state: HOST RW \n");
            break;

        case SYSTEM_STATE_GC:
            //DBG_Printf("system state: GC \n");
            break;

        case SYSTEM_STATE_BOOT:
            //DBG_Printf("system state: BOOT \n");
            break;

            //case SYSTEM_STATE_SHUTDOWN:
            //DBG_Printf("system state: SHUTDOWN \n");
            break;

        case SYSTEM_STATE_LOADTABLE:
            //DBG_Printf("system state: TABLE \n");
            break;

        default:
            break;
        }

        CurEntry = L2_GetCurrThreadType(ucPu);
        switch(CurEntry)
        {
        case THREAD_WRITE:
            //DBG_Printf("Current Thread Type: FTL Entry\n");
            break;

        case THREAD_GC:
            //DBG_Printf("Current Thread Type: GC Entry\n");
            break;

        case THREAD_TABLE_MANAGEMENT:
            //DBG_Printf("Current Thread Type: Table Entry \n");
            break;

        case THREAD_STATIC_WL:
            //DBG_Printf("Current Thread Type: Static WL Entry \n");
            break;

        case THREAD_BOOT:
            //DBG_Printf("Current Thread Type: Boot Entry \n");
            break;

        case THREAD_ERROR_HANDLING:
            //DBG_Printf("Current Thread Type: Error Entry \n");
            break;

        default:
            break;
        }
    }
#endif
    return SUCCESS;
}

/****************************************************************************
Name        :L2_DbgEventHandler
Input       :void
Output      :U32
Author      :HenryLuo
Date        :2012.11.29    18:54:02
Description :debug event handler.
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_DbgEventHandler(void)
{
    COMM_EVENT_PARAMETER * pParameter;
    U32 ulDbgCode;
    U32 ulPara1;
    U32 ulPara2;
    U32 ulPara3;
    U32 ulResult;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
    PhysicalAddr Addr = { 0 };
    FLASH_ADDR FlashAddr = { 0 };
    U16 usBufferId;

    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
    ulDbgCode = pParameter->EventParameterNormal[0];
    ulPara1 = pParameter->EventParameterNormal[1];
    ulPara2 = pParameter->EventParameterNormal[2];
    ulPara3 = pParameter->EventParameterNormal[3];

    switch (ulDbgCode)
    {
    case COMM_DEBUG_REPORT_STATUS:
        L2_DbgReportStatus();
        break;

    case COMM_DEBUG_DISPLAY:
        break;

    case COMM_DEBUG_WAIT:
        return FAIL;
        break;

    case L2_DEBUG_GET_PMTI_INDEX:
        /* ulPara1 is LPN input through JTAG */
        ulResult = L2_GetPMTIIndexInPu(ulPara1);
        DBG_Printf("PMTIIndexInPu is %d! \n", ulResult);
        break;

    case L2_DEBUG_GET_PMT_ITEM:
        pPMTPage = GetPMTPage(ulPara1);
        ulResult = L2_GetOffsetInPMTPage(ulPara1);
#ifdef PMT_ITEM_SIZE_3BYTE
        L2_PMTItemToPhyAddr(&Addr, &pPMTPage->m_PMTItems[ulResult]);
        Addr.m_PUSer = L2_GetSuperPuFromLPN(ulPara1);
#elif defined(PMT_ITEM_SIZE_REDUCE)
		L2_PMTItemToPhyAddr(&Addr, pPMTPage, ulResult);
#else
        Addr.m_PPN = pPMTPage->m_PMTItems[ulResult];
#endif
        DBG_Printf("Physical Addr: PU %d, Block 0x%x, Page 0x%x, Offset %d \n",
            Addr.m_PUSer, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_LPNInPage);
        break;

    case L2_DEBUG_GET_PMT_PAGE_ADDR:
        pPMTPage = GetPMTPage(ulPara1);
        DBG_Printf("PMT Page addr = 0x%x \n", (U32)pPMTPage);

#ifdef PMT_ITEM_SIZE_REDUCE
        pPMTPage = (U32*)(((U32)pPMTPage - (U32)g_PMTManager->m_pPMTPage[0][0]) / BUF_SIZE);
#else
		pPMTPage = (PMTPage *)(((U32)pPMTPage - (U32)g_PMTManager->m_pPMTPage[0][0]) / BUF_SIZE);
#endif
		ulResult = L2_GetOffsetInPMTPage(ulPara1);
        DBG_Printf("PMT Page pointer = 0x%x , OffsetInPMTPage = 0x%x\n", (U32)pPMTPage, ulResult);
        break;

    case L2_DEBUG_READ_LPN:
        ulResult = L2_LookupPMT(&Addr, ulPara1, FALSE);
        FlashAddr.ucPU = Addr.m_PUSer;
        FlashAddr.ucLun= Addr.m_OffsetInSuperPage;
        FlashAddr.usBlock = L2_VBT_GetPhysicalBlockAddr(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);
        FlashAddr.usPage = Addr.m_PageInBlock;
        DBG_Printf("LPN = 0x%x , Addr = 0x%x\n", ulPara1, Addr);

        usBufferId = COM_GetBufferIDByMemAddr(g_L2TempBufferAddr, TRUE, BUF_SIZE_BITS);
        if (FAIL == L2_DbgReadOnePage(FlashAddr, usBufferId))
        {
            return FAIL;
        }
        break;

    case L2_DEBUG_LOAD_PAGE:
        /* send read command */
        FlashAddr.ucPU = ulPara1 >> 16;
        FlashAddr.usBlock = ulPara1 & 0xffff;
        FlashAddr.usPage = ulPara2;

        usBufferId = COM_GetBufferIDByMemAddr(g_L2TempBufferAddr, TRUE, BUF_SIZE_BITS);
        if (FAIL == L2_DbgReadOnePage(FlashAddr, usBufferId))
        {
            return FAIL;
        }

        break;

    case L2_DEBUG_TABLE_CHECK:
        if (FALSE == L2_TableCheck())
        {
            return FAIL;
        }
        break;

    case L2_DEBUG_DIRTY_CNT_CHECK:
        if (FALSE == L2_DbgDirtyCntCheck(ulPara1, ulPara2))
        {
            return FAIL;
        }
        break;

    case L2_DEBUG_PMT_ITEM_CHECK:
        if (FALSE == L2_PMTItemChk(ulPara1))
        {
            return FAIL;
        }
        break;

    default:
        break;
    }

    /* clear debug parameters */
    pParameter->EventParameterNormal[0] = 0;
    pParameter->EventParameterNormal[1] = 0;
    pParameter->EventParameterNormal[2] = 0;
    pParameter->EventParameterNormal[3] = 0;

    return SUCCESS;
}

void MCU1_DRAM_TEXT L2_PrintGCManager(U16 PU)
{
    U32 ulGCModeIndex;
    GCManager_Print tGCManager;

    for (ulGCModeIndex = 0; ulGCModeIndex < GC_SRC_MODE; ulGCModeIndex++)
    {
        tGCManager.McuId = HAL_GetMcuId();
        tGCManager.m_Pu = PU;
        tGCManager.m_SrcPBN = g_GCManager[ulGCModeIndex]->tGCCommon.m_SrcPBN[PU];
        tGCManager.m_SrcPPO = g_GCManager[ulGCModeIndex]->tGCCommon.m_SrcWLO[PU];
        tGCManager.m_SrcOffset = g_GCManager[ulGCModeIndex]->tGCCommon.m_SrcOffset[PU];
        tGCManager.m_GCReadOffset = g_GCManager[ulGCModeIndex]->tGCCommon.m_GCReadOffset[PU];
        tGCManager.m_DirtyLpnCnt = g_GCManager[ulGCModeIndex]->m_DirtyLpnCnt[PU];
        tGCManager.m_FreePageCnt = g_GCManager[ulGCModeIndex]->m_FreePageCnt[PU];
        tGCManager.m_FreePageForHost = g_GCManager[ulGCModeIndex]->m_FreePageForHost[PU];
        tGCManager.m_NeedCopyForOneWrite = g_GCManager[ulGCModeIndex]->m_NeedCopyForOneWrite[PU];

        TRACE_LOG((void*)&tGCManager, sizeof(GCManager_Print), GCManager_Print, 0, "[L2_PrintGCManager]");
    }

    return;
}

void MCU1_DRAM_TEXT L2_PrintPuInfo(U16 PuID)
{
    PuInfo_Print tPuInfoPrint;
    U32 i;

    tPuInfoPrint.McuId = HAL_GetMcuId();

    tPuInfoPrint.m_TimeStamp = g_PuInfo[PuID]->m_TimeStamp;
    tPuInfoPrint.m_DataBlockCnt = g_PuInfo[PuID]->m_DataBlockCnt[BLKTYPE_SLC];
    tPuInfoPrint.m_AllocateBlockCnt = g_PuInfo[PuID]->m_AllocateBlockCnt[BLKTYPE_SLC];
    tPuInfoPrint.m_FreePageCnt = g_PuInfo[PuID]->m_SLCMLCFreePageCnt;

    for (i = 0; i < TARGET_ALL, i++;)
    {
        tPuInfoPrint.m_TargetBlk[i] = g_PuInfo[PuID]->m_TargetBlk[i];
        tPuInfoPrint.m_TargetPPO[i] = g_PuInfo[PuID]->m_TargetPPO[i];
    }

    TRACE_LOG((void*)&tPuInfoPrint, sizeof(PuInfo_Print), PuInfo_Print, 0, "[L2_PrintPuInfo]");

    return;
}

void MCU1_DRAM_TEXT L2_PrintFTLDptr(U8 ucPu)
{
    TRACE_LOG((void*)&ucPu, sizeof(U8), U8, 0, "[L2_PrintFTLDptr]: PU");
    //TRACE_LOG((void*)&g_FTLDptr[ucPu], sizeof(L2_FTLDptr), L2_FTLDptr, 0, "[L2_PrintFTLDptr]: ");

    if (g_FTLDptr[ucPu].pCurBufREQ == NULL)
    {
        TRACE_LOG((void*)&g_FTLDptr[ucPu].pCurBufREQ, sizeof(U32), U32, 0, "[L2_PrintFTLDptr]: g_FTLDptr.pCurBufREQ = NULL ");
    }
    else
    {
        L1_DbgPrintWriteBufReq(g_FTLDptr[ucPu].pCurBufREQ);
    }

    return;
}

void MCU1_DRAM_TEXT L2_PrintFTLReadDptr(U8 ucSPu,U8 ucLunInSPU)
{
    BUF_REQ_READ *pReq;

    TRACE_LOG((void*)&ucSPu, sizeof(U8), U8, 0, "[L2_PrintFTLReadDptr]: PU");

    if (INVALID_2F == g_FTLReadDptr[ucSPu][ucLunInSPU].m_BufReqID)
    {
        TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_BufReqID, sizeof(U32), U32, 0, "[L2_PrintFTLReadDptr]: g_FTLReadDptr.m_BufReqID = 0xFF ");
    }
    else
    {
        pReq = L1_GetReadBufReq(ucSPu, g_FTLReadDptr[ucSPu][ucLunInSPU].m_BufReqID);
        L1_DbgPrintReadBufReq(pReq);
    }

//    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_ReqRemain, sizeof(U32), U32, 0, "m_ReqRemain");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_LPNRemain, sizeof(U32), U32, 0, "m_LPNRemain");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_LPNOffset, sizeof(U32), U32, 0, "m_LPNOffset");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_bPMTLookuped, sizeof(U32), U32, 0, "m_bPMTLookuped");

    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[0], sizeof(U32), U32, 0, "m_Addr[0]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[1], sizeof(U32), U32, 0, "m_Addr[1]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[2], sizeof(U32), U32, 0, "m_Addr[2]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[3], sizeof(U32), U32, 0, "m_Addr[3]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[4], sizeof(U32), U32, 0, "m_Addr[4]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[5], sizeof(U32), U32, 0, "m_Addr[5]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[6], sizeof(U32), U32, 0, "m_Addr[6]");
    TRACE_LOG((void*)&g_FTLReadDptr[ucSPu][ucLunInSPU].m_Addr[7], sizeof(U32), U32, 0, "m_Addr[7]");

    return;
}

void MCU1_DRAM_TEXT L2_PrintSystermState(U8 ucPuNum)
{
#if 0
    U32 SystemState;
    U32 Thread;

    SystemState = (U32)L2_GetCurrState(ucPuNum);
    Thread = (U32)L2_GetCurrThreadType(ucPuNum);

    TRACE_LOG((void*)&SystemState, sizeof(U32), U32, 0, "[L2_PrintSystermState]: SystemState");
    TRACE_LOG((void*)&Thread, sizeof(U32), U32, 0, "[L2_PrintSystermState]: ThreadType");
#endif

    return;
}
void MCU1_DRAM_TEXT L2_DbgGCManagerShowAll()
{
    U16 i = 0;

    TRACE_LOG((void*)&i, sizeof(U16), U16, 0, "[L2_DbgGCManagerShowAll]: Enter PU");

    for (i = 0; i < SUBSYSTEM_PU_NUM; i++)
    {
        L2_PrintGCManager(i);
    }

    TRACE_LOG((void*)&i, sizeof(U16), U16, 0, "[L2_DbgGCManagerShowAll]: Done PU");

    return;
}

void MCU1_DRAM_TEXT L2_DbgPuInfoShowAll()
{
    U16 i = 0;

    //Fix me : SuperPage LUNs 1:3 & 1:6 TL_Enable() can cause UserExceptionVector().
    return;
    
    TRACE_LOG((void*)&i, sizeof(U16), U16, 0, "[L2_DbgPuInfoShowAll]: Enter PU");

    for (i = 0; i < SUBSYSTEM_PU_NUM; i++)
    {
        L2_PrintPuInfo(i);
    }

    TRACE_LOG((void*)&i, sizeof(U16), U16, 0, "[L2_DbgPuInfoShowAll]: Done PU");

    return;
}

void MCU1_DRAM_TEXT L2_DbgScheTraceEnable()
{
    U8 ModuleId;
    U8 SubModuleId;

    ModuleId = 1; //L2
    SubModuleId = 1;//L2Debug    
#if 0  //remove legacy log trace interface
    pTraceModuleList[ModuleId].SubModuleDbgLevel[SubModuleId] = LOG_LVL_TRACE;
#endif
}
void MCU1_DRAM_TEXT L2_DbgScheTraceDisable()
{
    U8 ModuleId;
    U8 SubModuleId;

    ModuleId = 1; //L2
    SubModuleId = 1;//L2Debug    
#if 0  //remove legacy log trace interface
    pTraceModuleList[ModuleId].SubModuleDbgLevel[SubModuleId] = LOG_LVL_INFO;
#endif
}

void MCU1_DRAM_TEXT L2_PrintTBFullRecovery(U16 usStatus, U8 ucPuNum, U16 usBlock, U16 usPage)
{
    PRINT_TB_FULLRECOVERY PrintTB;

    PrintTB.ucMcuId = HAL_GetMcuId();
    PrintTB.ucPu = ucPuNum;
    PrintTB.usBlock = usBlock;
    PrintTB.usPage = usPage;
    PrintTB.usStatus = usStatus;

    TRACE_LOG((void*)&PrintTB, sizeof(PRINT_TB_FULLRECOVERY), PRINT_TB_FULLRECOVERY, 0, "[L2_PrintTBFullRecovery]");
    return;
}

void MCU1_DRAM_TEXT L2_PrintRebuildDirtyCntBlk(U32 uSuperPU, U32 VirBlk, U8 ucWriteType, BOOL bTarget)
{
    PRINT_BLK PrintBlk;

    PrintBlk.ucMcuId = HAL_GetMcuId();
    PrintBlk.ucPu = (U8)uSuperPU;
    PrintBlk.usVirBlk = (U16)VirBlk;
    PrintBlk.ucWriteType = ucWriteType;
    PrintBlk.ucTarget = bTarget;

    TRACE_LOG((void*)&PrintBlk, sizeof(PRINT_BLK), PRINT_BLK, 0, "[L2_RebuildIsTargetBlk]");
}

void MCU1_DRAM_TEXT L2_DbgShowAll()
{
    U32 i, j;
    //DBG_Printf("MCU#%d L2_DbgShowAll.\n",HAL_GetMcuId());


    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_PrintSystermState(i);
        L2_PrintFTLDptr(i);

        for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
        {
            L2_PrintFTLReadDptr(i, j);
        }
    }

    L2_DbgGCManagerShowAll();

    L2_DbgPuInfoShowAll();
}
