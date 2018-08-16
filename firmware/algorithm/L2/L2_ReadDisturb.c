#include "FW_Event.h"
#include "L2_Defines.h"
#include "L2_ReadDisturb.h"
#include "L2_FTL.h"
#include "L2_Erase.h"
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#include "L2_TableBBT.h"

extern BOOL COM_BitMaskGet(U32 ulBitMask, U8 ucIndex);
extern U32 L2_GetSuperPuBitMap(U32 ulLunAllowToSendFcmdBitmap);
extern BOOL L2_IsBootupOK(void);

#ifdef SIM
GLOBAL  U32 g_aDbgForceGCTargetBlkCnt[SUBSYSTEM_SUPERPU_MAX] = { 0 };
GLOBAL  U32 g_aDbgForceGCClosedBlkCnt[SUBSYSTEM_SUPERPU_MAX] = { 0 };
#endif

// Leo Yang 20151215
// this function add a ForceGCSrcBlkQueue 
// entry to the Link tail of the specified PU
void L2_AddBlkQEntryToLinkTail(BLK_QUEUE* pQueue, BLKQ_LINK_TYPE BlkLInkType, U16 TargetLogIndex)
{
    U16* pusStartIndex;
    U16* pusEndIndex;
    U16* pusIndexCnt;
#ifdef SIM
    U16 usIndexfCnt;
    U16 usCurIndex;
#endif

    switch (BlkLInkType)
    {
    case LINK_TYPE_HOST:
        pusStartIndex = &(pQueue->usHOSTStartIndex);
        pusEndIndex = &(pQueue->usHOSTEndIndex);
        pusIndexCnt = &(pQueue->usHOSTCnt);
        break;

    case LINK_TYPE_TLCW:
        pusStartIndex = &(pQueue->usTLCWStartIndex);
        pusEndIndex = &(pQueue->usTLCWEndIndex);
        pusIndexCnt = &(pQueue->usTLCWCnt);
        break;

    case LINK_TYPE_FREE:
        pusStartIndex = &(pQueue->usFreeStartIndex);
        pusEndIndex = &(pQueue->usFreeEndIndex);
        pusIndexCnt = &(pQueue->usFreeCnt);
        break;
        
    default:
        DBG_Printf("Add Tail : BlkQ link type error\n");
        DBG_Getch();
        break;
    }

#ifdef SIM

    if (INVALID_4F != pQueue->aVBN[TargetLogIndex].NextIndex)
    {
        DBG_Printf("Add Tail : BlkQEntry next Index error\n");
        DBG_Getch();
    }

    if ((INVALID_4F != *pusStartIndex) && (INVALID_4F != pQueue->aVBN[*pusEndIndex].NextIndex))
    {
        DBG_Printf("Add Tail : next index error\n");
        DBG_Getch();
    }

    if (*pusStartIndex == INVALID_4F && *pusEndIndex != INVALID_4F)
    {
        DBG_Printf("Add Tail : ForceGCQueue link error\n");
        DBG_Getch();
    }

    usCurIndex = *pusStartIndex;
    usIndexfCnt = 0;
    while (usCurIndex != INVALID_4F)
    {
        usIndexfCnt++;
        usCurIndex = pQueue->aVBN[usCurIndex].NextIndex;
    }
    if (usIndexfCnt != *pusIndexCnt)
    {
        DBG_Printf("Add Tail : Index count error\n");
        DBG_Getch();
    }
#endif    

    if (*pusStartIndex == INVALID_4F)
    {
        *pusStartIndex = TargetLogIndex;

        *pusEndIndex = TargetLogIndex;       
    }
    else
    {
        pQueue->aVBN[*pusEndIndex].NextIndex = TargetLogIndex;

        *pusEndIndex = TargetLogIndex;
    }

    *pusIndexCnt += 1;

    return;
}
// Leo Yang 20151215
// this function remove a ForceGCSrcBlkQueue 
// entry from the Link of the specified PU
// Note that, if BlkLinkType is LINK_TYPE_HOST
// first remove Blk which dirty count > 1/3  total LPN count

U16 L2_RemoveBlkQEntry(U8 ucSuperPu, BLK_QUEUE* pQueue, BLKQ_LINK_TYPE BlkLInkType, BOOL bDirtySLC)
{

    U16* pusEndIndex;
    U16* pusStartIndex;
    U16* pusIndexCnt;
    U16   usRemoveIndex = INVALID_4F;
    U16   usNewStartIndex;
#ifdef SIM 
    U16 usIndexfCnt;
    U16 usCurIndex;
#endif

    switch (BlkLInkType)
    {
    case LINK_TYPE_HOST:
        pusStartIndex = &(pQueue->usHOSTStartIndex);
        pusEndIndex = &(pQueue->usHOSTEndIndex);
        pusIndexCnt = &(pQueue->usHOSTCnt);
        break;

    case LINK_TYPE_TLCW:
        pusStartIndex = &(pQueue->usTLCWStartIndex);
        pusEndIndex = &(pQueue->usTLCWEndIndex);
        pusIndexCnt = &(pQueue->usTLCWCnt);
        break;

    case LINK_TYPE_FREE:
        pusStartIndex = &(pQueue->usFreeStartIndex);
        pusEndIndex = &(pQueue->usFreeEndIndex);
        pusIndexCnt = &(pQueue->usFreeCnt);
        break;
    default:
        DBG_Printf("Remove Head : BlkQ link type error\n");
        DBG_Getch();
        break;
    }

#ifdef SIM

    if ((INVALID_4F != *pusEndIndex) && (INVALID_4F != pQueue->aVBN[*pusEndIndex].NextIndex))
    {
        DBG_Printf("Remove Head : next index error\n");
        DBG_Getch();
    }

    if (*pusStartIndex == INVALID_4F && *pusEndIndex != INVALID_4F)
    {
        DBG_Printf("Remove Head : ForceGCQueue link error\n");
        DBG_Getch();
    }

    usCurIndex = *pusStartIndex;
    usIndexfCnt = 0;
    while (usCurIndex != INVALID_4F)
    {
        usIndexfCnt++;
        usCurIndex = pQueue->aVBN[usCurIndex].NextIndex;
    }
    if (usIndexfCnt != *pusIndexCnt)
    {
        DBG_Printf("Remove Head : Index count error\n");
        DBG_Getch();
    }
#endif        
#if 0 /* Remove this condition due to block need to handle right now */
    //Remove DirtyCount > 1/3 total LPN count
    if ((LINK_TYPE_HOST == BlkLInkType) && (TRUE == bDirtySLC))
    {
        U16  i;
        U16  usCurrIndex;
        U16  usPrevIndex;
        U16  usBlockSN;

        usCurrIndex = *pusStartIndex;

        for (i = 0; i < (*pusIndexCnt); i++)
        {
            usBlockSN = pQueue->aVBN[usCurrIndex].VBN;

            if (L2_GetDirtyCnt(ucSuperPu, usBlockSN) < (LPN_PER_SUPER_SLCBLK / 3))
            {
                usPrevIndex = usCurrIndex;
                usCurrIndex = pQueue->aVBN[usCurrIndex].NextIndex;
            }
            else
            {
                usRemoveIndex = usCurrIndex;

                if (usRemoveIndex == *pusStartIndex)
                {
                    usNewStartIndex = pQueue->aVBN[*pusStartIndex].NextIndex;

                    if (*pusStartIndex == *pusEndIndex)
                    {
                        *pusEndIndex = INVALID_4F;
                    }

                    *pusStartIndex = usNewStartIndex;
                }
                else if (usRemoveIndex == *pusEndIndex)
                {
                    pQueue->aVBN[usPrevIndex].NextIndex = INVALID_4F;
                    *pusEndIndex = usPrevIndex;
                }
                else
                {
                    pQueue->aVBN[usPrevIndex].NextIndex = pQueue->aVBN[usRemoveIndex].NextIndex;
                }

                pQueue->aVBN[usRemoveIndex].NextIndex = INVALID_4F;

                break;
            }
        }
    }
    else
#endif
    {
        usRemoveIndex = *pusStartIndex;

        usNewStartIndex = pQueue->aVBN[*pusStartIndex].NextIndex;

        pQueue->aVBN[*pusStartIndex].NextIndex = INVALID_4F;

        if (*pusStartIndex == *pusEndIndex)
        {
            *pusEndIndex = INVALID_4F;
        }

        *pusStartIndex = usNewStartIndex;
    }

    if (INVALID_4F != usRemoveIndex)
    {
        *pusIndexCnt -= 1;
    }

#ifdef SIM    
    if ((FALSE == bDirtySLC) && (INVALID_4F == usRemoveIndex))
    {
        DBG_Printf("Remove Entry : Link empty error, usRemoveIndex return INVALID_4F\n");
        DBG_Getch();
    }
#endif

    return usRemoveIndex;
}

void MCU1_DRAM_TEXT L2_BlkQueueInit(BLK_QUEUE *pQueue, U32 ulLength)
{
    U16 ucLoop;

    pQueue->usLength = ulLength;

    pQueue->usHOSTStartIndex = INVALID_4F;
    pQueue->usHOSTEndIndex = INVALID_4F;
    pQueue->usHOSTCnt = 0;
    pQueue->usTLCWStartIndex = INVALID_4F;
    pQueue->usTLCWEndIndex = INVALID_4F;
    pQueue->usTLCWCnt = 0;
    pQueue->usFreeStartIndex = INVALID_4F;
    pQueue->usFreeEndIndex = INVALID_4F;
    pQueue->usFreeCnt = 0;

    for (ucLoop = 0; ucLoop < ulLength; ucLoop++)
    {
        pQueue->aVBN[ucLoop].VBN = INVALID_4F;
        pQueue->aVBN[ucLoop].NextIndex = INVALID_4F;

        /* Build Free link*/
        L2_AddBlkQEntryToLinkTail(pQueue, LINK_TYPE_FREE, ucLoop);
    }

    return;
}

BOOL L2_BlkQueueIsEmpty(BLK_QUEUE *pQueue, BLKQ_LINK_TYPE BlkQLinkType)
{
    BOOL bEmpty;

    switch (BlkQLinkType)
    {
    case LINK_TYPE_HOST:
        bEmpty = (0 == pQueue->usHOSTCnt) ? TRUE : FALSE;
        break;

    case LINK_TYPE_TLCW:
        bEmpty = (0 == pQueue->usTLCWCnt) ? TRUE : FALSE;
        break;

    case LINK_TYPE_FREE:
    default:
        DBG_Printf("L2_BlkQueueIsEmpty: BlkQ link type error\n");
        DBG_Getch();
    }
    return bEmpty;
}

BOOL L2_BlkQueueIsFull(BLK_QUEUE *pQueue)
{
    return (0 == pQueue->usFreeCnt) ? TRUE : FALSE;
}

BOOL L2_BlkQueuePushBlock(U8 ucSuperPu, BLK_QUEUE *pQueue, U8 ucBlkType, U16 usVBN)
{
    U16 usQueueIndex;
    BLKQ_LINK_TYPE BlkQLinkType;

    switch (ucBlkType)
    {
    case VBT_TYPE_HOST:
        BlkQLinkType = LINK_TYPE_HOST;
        break;
    case VBT_TYPE_TLCW:
        BlkQLinkType = LINK_TYPE_TLCW;
        break;
    default:
        DBG_Printf("L2_BlkQueuePushBlock: error input BlkType=%d, VBN%d lock=%d\n", ucBlkType, usVBN, L2_PBIT_Get_Lock(ucSuperPu, usVBN));
        return FAIL;
    }

    if (TRUE == L2_BlkQueueIsFull(pQueue))
    {
        return FAIL;
    }
    if (TRUE == pVBT[ucSuperPu]->m_VBT[usVBN].bsInEraseQueue || TRUE == pVBT[ucSuperPu]->m_VBT[usVBN].bWaitEraseSts
        || VBT_TYPE_INVALID == pVBT[ucSuperPu]->m_VBT[usVBN].VBTType)
    {
        return FAIL;
    }

    if (TRUE == L2_PBIT_Get_Lock(ucSuperPu, usVBN))
    {
        DBG_Printf("SuperPu %d VBN %d is already Lock!\n", ucSuperPu, usVBN);
        return FAIL;
    }

    L2_PBIT_Set_Lock(ucSuperPu, usVBN, TRUE);

    /* Get a free index from BlkQueue free link*/
    usQueueIndex = L2_RemoveBlkQEntry(ucSuperPu, pQueue, LINK_TYPE_FREE, FALSE);

    /* Set the BlkQueue entry VBN */
    pQueue->aVBN[usQueueIndex].VBN = usVBN;

    /* Add the BlkQueue entry to specify link */
    L2_AddBlkQEntryToLinkTail(pQueue, BlkQLinkType, usQueueIndex);

    return SUCCESS;
}

U16 L2_BlkQueuePopBlock(U8 ucSuperPu, BLK_QUEUE *pQueue, U8 ucBlkType, BOOL bDirtySLC)
{
    U16 usVBN;
    U16 usQueueIndex;
    BLKQ_LINK_TYPE BlkQLinkType;

    switch (ucBlkType)
    {
    case VBT_TYPE_HOST:
        BlkQLinkType = LINK_TYPE_HOST;
        break;

    case VBT_TYPE_TLCW:
        BlkQLinkType = LINK_TYPE_TLCW;
        break;
    default:
        DBG_Printf("L2_BlkQueuePopBlock: error input BlkType\n");
        DBG_Getch();
        break;
    }

    if (TRUE == L2_BlkQueueIsEmpty(pQueue, BlkQLinkType))
    {
        return INVALID_4F;
    }

    /* Remove BlkQEntry from specify link */
    usQueueIndex = L2_RemoveBlkQEntry(ucSuperPu, pQueue, BlkQLinkType, bDirtySLC);

    /* Check one possible case on LINK_TYPE_HOST*/
    /* link not empty, but haven't DirtyCount > 1/3 total LPN count block */
    if (INVALID_4F == usQueueIndex)
    {
        return INVALID_4F;
    }
    else
    {
        /* Get the VBN in BlkQEntry */
        usVBN = pQueue->aVBN[usQueueIndex].VBN;
        pQueue->aVBN[usQueueIndex].VBN = INVALID_4F;
    }

    /* Add the BlkQueue entry to free link tail */
    L2_AddBlkQEntryToLinkTail(pQueue, LINK_TYPE_FREE, usQueueIndex);

    L2_PBIT_Set_Lock(ucSuperPu, usVBN, FALSE);

    return usVBN;
}

#if 1
void L2_BlkQueueRemoveAll(U8 ucSuperPu, BLK_QUEUE *pQueue)
{
    U16 usVBN;
    U16 usQueueIndex;
    BLKQ_LINK_TYPE BlkQLinkType;

    BlkQLinkType = LINK_TYPE_HOST;
    while (FALSE == L2_BlkQueueIsEmpty(pQueue, BlkQLinkType))
    {
        usQueueIndex = L2_RemoveBlkQEntry(ucSuperPu, pQueue, BlkQLinkType, FALSE);
        usVBN = pQueue->aVBN[usQueueIndex].VBN;
        L2_PBIT_Set_Lock(ucSuperPu, usVBN, FALSE);
        //L2_VBTSetInForceGCSrcBlkQ(ucSuperPu, usVBN, FALSE);
    }

    BlkQLinkType = LINK_TYPE_TLCW;
    while (FALSE == L2_BlkQueueIsEmpty(pQueue, BlkQLinkType))
    {
        usQueueIndex = L2_RemoveBlkQEntry(ucSuperPu, pQueue, BlkQLinkType, FALSE);
        usVBN = pQueue->aVBN[usQueueIndex].VBN;
        L2_PBIT_Set_Lock(ucSuperPu, usVBN, FALSE);
        //L2_VBTSetInForceGCSrcBlkQ(ucSuperPu, usVBN, FALSE);
    }
}
#endif

void MCU1_DRAM_TEXT L2_ForceGCInit(void)
{
    U8 i;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_BlkQueueInit(g_pForceGCSrcBlkQueue[i], FORCE_GC_QUEUE_SIZE);
    }

    return;
}

void L2_ResetBlkReadCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPBN)
{
    U32 ulEraseCnt;

    ulEraseCnt = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPBN].EraseCnt;

    if ((0 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_0 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_0;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_0 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_1 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_1;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_1 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_2 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_2;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_2 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_3 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_3;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_3 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_4 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_4;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_4 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_5 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_5;
    }
    else if ((READ_DISTURB_ERASE_CNT_THS_5 <= ulEraseCnt) && (READ_DISTURB_ERASE_CNT_THS_6 > ulEraseCnt))
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_6;
    }
    else //ReadCnt >= READ_DISTURB_ERASE_CNT_THS_6
    {
        pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = BLK_READ_CNT_INIT_VALUE_THS_7;
    }

    return;
}

void L2_UpdateBlkReadCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usVBN)
{
    U16 usPBN;
    S32 sBlkReadCnt;

    if (FALSE == L2_IsBootupOK())
    {
        return;
    }

    usPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, usVBN);
    sBlkReadCnt = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt;

#ifdef SIM
    ASSERT(INVALID_4F != usPBN);
#endif
    sBlkReadCnt -= CLOSED_BLK_READ_CNT_DEC;

    //FIRMWARE_LogInfo("L2_UpdateBlkReadCnt MCU%d Pu %d VirBlk 0x%x(PhyBlk 0x%x) to ForceGCSrcBlkQ,ReadCnt 0x%x!\n", HAL_GetMcuId(), ucSuperPu, usVBN, pVBT[ucSuperPu]->m_VBT[usVBN].PhysicalBlockAddr[0], sBlkReadCnt);

    /* if block read count reaches THS, push it into force gc queue */
    if (BLK_READ_DISTURB_THS >= sBlkReadCnt)
    {
        /* no need to check in EraseQue or not, as Read Command will not access TLC all dirty block */
        if (TRUE != L2_PBIT_Get_Lock(ucSuperPu, usVBN))
        {
            if (SUCCESS == L2_BlkQueuePushBlock(ucSuperPu, g_pForceGCSrcBlkQueue[ucSuperPu], VBT_TYPE_TLCW, usVBN))
            {
                //FIRMWARE_LogInfo("MCU%d Push Pu %d VirBlk 0x%x(PhyBlk 0x%x) to ForceGCSrcBlkQ,ReadCnt 0x%x!\n", HAL_GetMcuId(), ucSuperPu, usVBN, pVBT[ucSuperPu]->m_VBT[usVBN].PhysicalBlockAddr[0], sBlkReadCnt);
                DBG_Printf("MCU%d Push Pu %d VirBlk 0x%x(PhyBlk 0x%x) to ForceGCSrcBlkQ,ReadCnt 0x%x!\n", HAL_GetMcuId(), ucSuperPu, usVBN, pVBT[ucSuperPu]->m_VBT[usVBN].PhysicalBlockAddr[0], sBlkReadCnt);
            }
        }
    }

    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPBN].ReadCnt = sBlkReadCnt;

    return;
}

#if 0
BOOL L2_GCTargetBlkPrepare(U8 ucSuperPu, U8 ucGCMode)
{
    U16 usDstVBN;
    PuInfo* pPuInfo;
    BOOL bRet = FAIL;
    GCManager * pGCManager;

    pGCManager = g_GCManager[ucGCMode];
    pPuInfo = g_PuInfo[ucSuperPu];
    if (pPuInfo->m_AllocateBlockCnt[BLKTYPE_SLC] < pPuInfo->m_DataBlockCnt[BLKTYPE_SLC])
    {
        HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLOCATE_FREE_BLK);
        usDstVBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);

        if (INVALID_4F != usDstVBN)
        {
            pGCManager->m_DstPBN[ucSuperPu] = usDstVBN;
            pGCManager->tGCCommon.m_SrcWLO[ucSuperPu] = 0;
            pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] = 0;

            pVBT[ucSuperPu]->m_VBT[usDstVBN].Target = VBT_NOT_TARGET;
            L2_PBIT_Set_Lock(ucSuperPu, usDstVBN, TRUE);

            DBG_Printf("MCU%d Force GC SuperPu %d, SrcVBN 0x%x, DstVBN 0x%x, Target %d\n", HAL_GetMcuId(), ucSuperPu,
                pGCManager->tGCCommon.m_SrcPBN[ucSuperPu], pGCManager->m_DstPBN[ucSuperPu],
                pVBT[ucSuperPu]->m_VBT[pGCManager->tGCCommon.m_SrcPBN[ucSuperPu]].Target);

            bRet = SUCCESS;
        }
    }

    return bRet;
}

BOOL L2_GCReadWholePage(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucGCMode)
{
    U8 ucTLun;
    U8 ucLunInSuperPu;
    PhysicalAddr tSrcAddr;
    U32 *pBuffer;
    RED* pSpare;
    U8* pStatus;
    BOOL bRet = FALSE;
    GCManager* pGCManager;

    pGCManager = g_GCManager[ucGCMode];

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (TRUE == COM_BitMaskGet(pGCManager->m_TargetBlkStatusBitmap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }

        tSrcAddr.m_PPN = 0;
        tSrcAddr.m_PUSer = ucSuperPu;
        tSrcAddr.m_BlockInPU = pGCManager->tGCCommon.m_SrcPBN[ucSuperPu];
        tSrcAddr.m_PageInBlock = pGCManager->tGCCommon.m_SrcWLO[ucSuperPu];
        tSrcAddr.m_OffsetInSuperPage = ucLunInSuperPu;
        tSrcAddr.m_LPNInPage = 0;

        pBuffer = (U32 *)(pGCManager->tGCCommon.m_GCBuffer[ucSuperPu][0]) + BUF_SIZE *ucLunInSuperPu;
        pSpare = &pGCManager->m_Spare[ucSuperPu];
        pStatus = &pGCManager->tGCCommon.m_FlushStatus[ucSuperPu][0][0];

#ifdef SIM
        if (TRUE == L2_VBT_Get_TLC(ucSuperPu, tSrcAddr.m_BlockInPU))
        {
            DBG_Printf("SPPU:%d, Blk:%d, Source block is TLC and Target!!Error!\n", ucSuperPu, tSrcAddr.m_BlockInPU);
            DBG_Getch();
        }
#endif

        L2_FtlReadLocal(pBuffer, &tSrcAddr, pStatus, (U32 *)pSpare, LPN_PER_BUF, 0, FALSE, FALSE);

        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }
    if (SUPERPU_LUN_NUM_BITMSK == pGCManager->m_TargetBlkStatusBitmap[ucSuperPu])
    {
        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] = 0;
        bRet = TRUE;
    }

    return TRUE;
}

BOOL L2_GCWriteWholePage(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucGCMode)
{
    U8 ucTLun;
    U8 ucLunInSuperPu;
    PhysicalAddr tDstAddr;
    U32 *pBuffer;
    RED* pSpare;
    U8* pStatus;
    BOOL bRet = FALSE;
    GCManager* pGCManager;

    pGCManager = g_GCManager[ucGCMode];
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (TRUE == COM_BitMaskGet(pGCManager->m_TargetBlkStatusBitmap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }

        tDstAddr.m_PPN = 0;
        tDstAddr.m_PUSer = ucSuperPu;
        tDstAddr.m_BlockInPU = pGCManager->m_DstPBN[ucSuperPu];
        tDstAddr.m_PageInBlock = pGCManager->tGCCommon.m_SrcWLO[ucSuperPu];
        tDstAddr.m_OffsetInSuperPage = ucLunInSuperPu;
        tDstAddr.m_LPNInPage = 0;

        pBuffer = (U32 *)(pGCManager->tGCCommon.m_GCBuffer[ucSuperPu][0]) + BUF_SIZE *ucLunInSuperPu;
        pSpare = &pGCManager->m_Spare[ucSuperPu];
        pStatus = &pGCManager->tGCCommon.m_FlushStatus[ucSuperPu][0][0];

        if (0 == tDstAddr.m_PageInBlock)
        {
            L2_Set_DataBlock_PBIT_Info(tDstAddr.m_PUSer, tDstAddr.m_OffsetInSuperPage, tDstAddr.m_BlockInPU, pSpare);
        }

        L2_FtlWriteLocal(&tDstAddr, pBuffer, (U32 *)pSpare, pStatus, FALSE, FALSE, NULL);

        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }//for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)

    if (SUPERPU_LUN_NUM_BITMSK == pGCManager->m_TargetBlkStatusBitmap[ucSuperPu])
    {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
        L2_IncTimeStampInPu(ucSuperPu);
#endif
        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] = 0;
        pGCManager->tGCCommon.m_SrcWLO[ucSuperPu] += 1;
        bRet = TRUE;
    }

    return TRUE;
}

/* Note:Read disturb Erase don't use EraseQueue,because EraseQueue will always buffer 1 block,
and this block can't be erased directly.If abnormal shutdown,may find 2 targetblock have different virblk. */
BOOL L2_GCTargetBlkErase(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucGCMode)
{
    U16 usSrcVBN;
    U16 usDstVBN;
    U8 ucTLun;
    U8 ucLunInSuperPu;
    U8* pStatus;
    GCManager* pGCManager;
    BOOL bRet = FALSE;

    pGCManager = g_GCManager[ucGCMode];

    usSrcVBN = pGCManager->tGCCommon.m_SrcPBN[ucSuperPu];
    usDstVBN = pGCManager->m_DstPBN[ucSuperPu];

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (FALSE == L2_FCMDQNotFull(ucTLun))
        {
            DBG_Getch();
        }

        if (TRUE == COM_BitMaskGet(pGCManager->m_TargetBlkStatusBitmap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }
        
        pStatus = &pGCManager->tGCCommon.m_FlushStatus[ucSuperPu][0][ucLunInSuperPu];

        L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, usSrcVBN, pStatus, FALSE, FALSE, TRUE);

        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);

    }

    if (SUPERPU_LUN_NUM_BITMSK == pGCManager->m_TargetBlkStatusBitmap[ucSuperPu])
    {
        pGCManager->m_TargetBlkStatusBitmap[ucSuperPu] = 0;
        bRet = TRUE;
    }

    return bRet;
}

BOOL L2_GCTargetBlkCheckEraseStatus(U8 ucSuperPu, U8 ucGCMode)
{
    U16 usSrcVBN;
    U16 usDstVBN;
    U8 ucLunInSuperPu;
    U8* pStatus;
    GCManager* pGCManager;
    BOOL bRet = FALSE;

    pGCManager = g_GCManager[ucGCMode];

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pStatus = &pGCManager->tGCCommon.m_FlushStatus[ucSuperPu][0][ucLunInSuperPu];

        if (SUBSYSTEM_STATUS_PENDING == (*pStatus))
        {
            return bRet;
        }
    }

    usSrcVBN = pGCManager->tGCCommon.m_SrcPBN[ucSuperPu];
    usDstVBN = pGCManager->m_DstPBN[ucSuperPu];

    /*subsequet process*/
    /* make usSrcVBN corresponding to physical dst block, usDstVBN corresponding to physical src block */
    L2_PBIT_VBT_Swap_Block(ucSuperPu, usSrcVBN, usDstVBN);

    /* unlock dst block and src block*/
    L2_PBIT_Set_Lock(ucSuperPu, usSrcVBN, FALSE);
    L2_PBIT_Set_Lock(ucSuperPu, usDstVBN, FALSE);

    L2_BM_CollectFreeBlock(ucSuperPu, usDstVBN);

    bRet = TRUE;

    return bRet;
}

BOOL L2_GCTargetBlkProcess(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucGCMode)
{
    TargetBlkGCState *pSubState;
    U8 ucTarget;
    BOOL bRet = FALSE;
    GCComStruct* pGCCom;

    pGCCom = &g_GCManager[ucGCMode]->tGCCommon;
    pSubState = &(g_GCManager[ucGCMode]->m_TargetBlkGCState[ucSuperPu]);

    switch (*pSubState)
    {
    case TARGET_BLK_GC_STATE_PREPARE:
        if (SUCCESS == L2_GCTargetBlkPrepare(ucSuperPu, ucGCMode))
        {
            *pSubState = TARGET_BLK_GC_STATE_READ;
        }
        break;

    case TARGET_BLK_GC_STATE_READ:
        if (TRUE == L2_GCReadWholePage(ucSuperPu, ulLunAllowToSendFcmdBitmap, ucGCMode))
        {
            *pSubState = TARGET_BLK_GC_STATE_WRITE;
        }
        break;

    case TARGET_BLK_GC_STATE_WRITE:
        if (TRUE == L2_GCWriteWholePage(ucSuperPu, ulLunAllowToSendFcmdBitmap, ucGCMode))
        {
            bRet = TRUE;

            ucTarget = pVBT[ucSuperPu]->m_VBT[    pGCCom->m_SrcPBN[ucSuperPu]].Target;
            if (VBT_NOT_TARGET == ucTarget)
            {
                if (PG_PER_SLC_BLK <= pGCCom->m_SrcWLO[ucSuperPu])
                {
                    *pSubState = TARGET_BLK_GC_STATE_ERASE;
                }
                else
                {
                    *pSubState = TARGET_BLK_GC_STATE_READ;
                }
                break;
            }
            else if (VBT_TARGET_HOST_W == ucTarget)
            {
                if (g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] <= pGCCom->m_SrcWLO[ucSuperPu])
                {
                    *pSubState = TARGET_BLK_GC_STATE_ERASE;
                }
                else
                {
                    *pSubState = TARGET_BLK_GC_STATE_READ;
                }
                break;
            }
            else if (VBT_TARGET_HOST_GC == ucTarget)
            {
                if (g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_HOST_GC] <= pGCCom->m_SrcWLO[ucSuperPu])
                {
                    *pSubState = TARGET_BLK_GC_STATE_ERASE;
                }
                else
                {
                    *pSubState = TARGET_BLK_GC_STATE_READ;
                    break;
                }
            }
            else
            {
                DBG_Printf("MCU%d L2_GCTargetBlkProcess: ucTarget=%d, ERROR!\n", HAL_GetMcuId(), ucTarget);
                DBG_Getch();
                break;
            }
        }

        break;

    case TARGET_BLK_GC_STATE_ERASE:
        DBG_Printf("ForceGC Pu %d SrcVBN 0x%x ->DstVBN 0x%x Copy %d pages done!\n", ucSuperPu, pGCCom->m_SrcPBN[ucSuperPu],
            g_GCManager[ucGCMode]->m_DstPBN[ucSuperPu], g_GCManager[ucGCMode]->tGCCommon.m_SrcWLO[ucSuperPu]);
        if (TRUE == L2_GCTargetBlkErase(ucSuperPu, ulLunAllowToSendFcmdBitmap, ucGCMode))
        {
            *pSubState = TARGET_BLK_GC_STATE_WAIT_ERASE;
        }

        break;

    case TARGET_BLK_GC_STATE_WAIT_ERASE:
        if (TRUE == L2_GCTargetBlkCheckEraseStatus(ucSuperPu, ucGCMode))
        {
            L2_InitGCManager(ucSuperPu, ucGCMode);
            L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_SLCGC);
        }

        break;

    default:
        DBG_Printf("MCU%d L2_GCTargetBlkProcess: g_GCManager->m_TargetBlkGCState[%d]=%d, ERROR!\n", HAL_GetMcuId(), ucSuperPu, *pSubState);
        DBG_Getch();
        break;
    }//switch (*pSubState)

    return bRet;
}
#endif
