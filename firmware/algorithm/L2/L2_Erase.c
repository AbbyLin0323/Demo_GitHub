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
  File Name     : L2_Erase.c
  Version       : Initial Draft
  Author        : Zoe Wen
  Created       : 2015/2/4
  Description   : L2 erase pool
  Description   : 
  Function List :
  History       :
  1.Date        : 2015/2/4
    Author      : Zoe Wen
    Modification: Created file
2.Date        : 2015/04/23
Author      : Javen Liu
Modification: Delete the erase pool and create erase queue.

******************************************************************************/
#include "L2_Erase.h"
#include "L2_VBT.h"
#include "L2_Interface.h"
#include "L2_StripeInfo.h"
#include "L2_PBIT.h"

GLOBAL  EraseQueue *g_EraseQueue[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  RecordEraseInfo g_NeedEraseBlkInfo[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  EraseOperation g_EraseOpt[SUBSYSTEM_SUPERPU_MAX];

LOCAL void InitEraseQueue(EraseQueue* this);
LOCAL BOOL IsEmptyQueue(EraseQueue* this, BOOL IsSLCBlk);
LOCAL BOOL IsFullQueue(EraseQueue* this, BOOL IsSLCBlk);
LOCAL U16 GetQueueSize(EraseQueue* this, BOOL IsSLCBlk);
LOCAL void EnEraseQueue(EraseQueue* this, U16 usVBN, BOOL IsSLCBlk);
LOCAL U16 DeEraseQueue(EraseQueue* this, BOOL IsSLCBlk);

// Constructor
LOCAL void InitEraseQueue(EraseQueue* this)
{
    this->ucFront = 0;
    this->ucRear = 0;
    this->ucTLCFront = 0;
    this->ucTLCRear = 0;

    this->ulEraseBitMap = 0;
    this->ulTLCEraseBitMap = 0;

    this->pIsEmpty = IsEmptyQueue;
    this->pIsFull = IsFullQueue;
    this->pSize = GetQueueSize;
    this->pEn = EnEraseQueue;
    this->pDe = DeEraseQueue;
}

// Empty or not
LOCAL BOOL IsEmptyQueue(EraseQueue* this, BOOL IsSLCBlk)
{
    if (IsSLCBlk == TRUE)
        return (this->ucFront == this->ucRear ? TRUE : FALSE);
    else
        return (this->ucTLCFront == this->ucTLCRear ? TRUE : FALSE);
}

// Full or not
LOCAL BOOL IsFullQueue(EraseQueue* this, BOOL IsSLCBlk)
{
    if (IsSLCBlk == TRUE)
        return ((this->ucRear + 1) % (ERASE_QUEUE_SIZE + 1) == this->ucFront ? TRUE : FALSE);
    else
        return ((this->ucTLCRear + 1) % (TLC_BLK_CNT + 1) == this->ucTLCFront ? TRUE : FALSE);
}

LOCAL U16 GetQueueSize(EraseQueue* this, BOOL IsSLCBlk)
{
    if (IsSLCBlk == TRUE)
    {
        if (this->ucRear >= this->ucFront)
        {
            return ( this->ucRear - this->ucFront );
        }
        else
        {
            return ( (ERASE_QUEUE_SIZE + 1) - this->ucFront + this->ucRear );
        }
    }
    else
    {
        if (this->ucTLCRear >= this->ucTLCFront)
        {
            return ( this->ucTLCRear - this->ucTLCFront );
        }
        else
        {
            return ( (TLC_BLK_CNT + 1) - this->ucTLCFront + this->ucTLCRear );
        }
    }
    
}

// Enter into queue.
LOCAL void EnEraseQueue(EraseQueue* this, U16 usVBN, BOOL IsSLCBlk)
{
    if (IsSLCBlk == TRUE)
    {
        ASSERT((NULL != this->pIsFull) && (!this->pIsFull(this, TRUE)));

        this->aVBN[this->ucRear] = usVBN; // Insert it from the rear
        this->ucRear = (this->ucRear + 1) % (ERASE_QUEUE_SIZE + 1);
    }
    else
    {
        ASSERT((NULL != this->pIsFull) && (!this->pIsFull(this, FALSE)));

        this->aTLCVBN[this->ucTLCRear] = usVBN; // Insert it from the rear
        this->ucTLCRear = (this->ucTLCRear + 1) % (TLC_BLK_CNT + 1);
    }
}

// Delete from queue.
LOCAL U16 DeEraseQueue(EraseQueue* this, BOOL IsSLCBlk)
{
    U16 usFreeVBN;
    if (IsSLCBlk == TRUE)
    {
        ASSERT((NULL != this->pIsEmpty) && (!this->pIsEmpty(this, TRUE)));

        usFreeVBN = this->aVBN[this->ucFront]; // Delete it from the front
        this->ucFront = (this->ucFront + 1) % (ERASE_QUEUE_SIZE + 1);
    }
    else
    {
        ASSERT((NULL != this->pIsEmpty) && (!this->pIsEmpty(this, FALSE)));

        usFreeVBN = this->aTLCVBN[this->ucTLCFront]; // Delete it from the front
        this->ucTLCFront = (this->ucTLCFront + 1) % (TLC_BLK_CNT + 1);
    }

    return usFreeVBN;
}

/////////////////////////////////////////////////////////////////////////////////////////
GLOBAL void MCU1_DRAM_TEXT L2_EraseQueueInit(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu, EraseBlkCnt, ucSuperPu;

    ASSERT(NULL != g_EraseQueue[ucSuperPuNum]);

    InitEraseQueue(g_EraseQueue[ucSuperPuNum]);

    for (ucLunInSuperPu= 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (EraseBlkCnt= 0; EraseBlkCnt < MAX_ERASE_SLCBLK_CNT; EraseBlkCnt++)
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[ucLunInSuperPu][EraseBlkCnt] = INVALID_4F;
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[EraseBlkCnt] = INVALID_4F;
        }
		
        for (EraseBlkCnt= 0; EraseBlkCnt < MAX_ERASE_TLCBLK_CNT; EraseBlkCnt++)
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCPBN[ucLunInSuperPu][EraseBlkCnt] = INVALID_4F;
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCVBN[EraseBlkCnt] = INVALID_4F;
        }
    }

    g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt = 0;
    g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt = 0;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        g_EraseOpt[ucSuperPu].L2NeedEraseBlk = FALSE;
        g_EraseOpt[ucSuperPu].bEraseTLC = FALSE;
        g_EraseOpt[ucSuperPu].bEraseScheduleEn = FALSE;
        g_EraseOpt[ucSuperPu].uErasingTLCVBN = INVALID_4F;
    }
}

GLOBAL void L2_EraseQueueShutdown(U8 ucSuperPuNum)
{
    U16 usFreeVBN;
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    ASSERT(NULL != pQueue);
    ASSERT(NULL != pQueue->pIsEmpty);

    // Clear SLC this queue.
    while (!pQueue->pIsEmpty(pQueue, TRUE))
    {
        usFreeVBN = pQueue->pDe(pQueue, TRUE);
        L2_VBTSetInEraseQueue(ucSuperPuNum, usFreeVBN, FALSE);
    }

    // Clear TLC this queue.
    while (!pQueue->pIsEmpty(pQueue, FALSE))
    {
        usFreeVBN = pQueue->pDe(pQueue, FALSE);
        L2_VBTSetInEraseQueue(ucSuperPuNum, usFreeVBN, FALSE);
    }

    return;
}

GLOBAL U16 L2_GetEraseQueueSize(U8 ucSuperPuNum, BOOL IsSLCBlk)
{
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    ASSERT(NULL != pQueue);

    return pQueue->pSize(pQueue, IsSLCBlk);
}

GLOBAL BOOL L2_IsAllEraseQueueEmpty(U8 ucSuperPuNum)
{
    BOOL bIsAllEmpty = TRUE;
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    if ((FALSE == pQueue->pIsEmpty(pQueue, TRUE)) || (FALSE == pQueue->pIsEmpty(pQueue, FALSE)))
        return FALSE;

    return TRUE;
}

GLOBAL BOOL L2_IsEraseQueueEmpty(U8 ucSuperPuNum, BOOL IsSLCBlk)
{
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    ASSERT(NULL != pQueue);

    return pQueue->pIsEmpty(pQueue, IsSLCBlk);
}

GLOBAL BOOL L2_EraseQueueLastVBNIsTLC(U8 ucSuperPuNum)
{
    EraseQueue *pQueue = NULL;
    U16 usLastVBN;
    U16 usBefRear;

    pQueue = g_EraseQueue[ucSuperPuNum];

    ASSERT(NULL != pQueue);

    if (pQueue->ucRear == 0)
    {
        usBefRear = ERASE_QUEUE_SIZE;
    }
    else
    {
        usBefRear = pQueue->ucRear - 1;
    }

    usLastVBN = pQueue->aVBN[usBefRear];
    return L2_VBT_Get_TLC(ucSuperPuNum, usLastVBN);
}

GLOBAL void L2_InsertBlkIntoEraseQueue(U8 ucSuperPuNum, U16 usVBN, BOOL IsSLCBlk)
{
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    ASSERT(NULL != pQueue);

    if (!pQueue->pIsFull(pQueue, IsSLCBlk))
    {
        // Not Full
        pQueue->pEn(pQueue, usVBN, IsSLCBlk);
        L2_VBTSetInEraseQueue(ucSuperPuNum, usVBN, TRUE);
    }
    else
    {
        // Queue is full
        DBG_Getch();
    }

    return;
}

GLOBAL U16 L2_PopBlkFromEraseQueue(U8 ucSuperPuNum, BOOL IsSLCBlk)
{
    U16 usFreeVBN = INVALID_4F;
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    if (NULL == pQueue || ucSuperPuNum > SUBSYSTEM_SUPERPU_NUM)
    {
        DBG_Printf("SuperPu %d pQueue 0x%x Error!\n", ucSuperPuNum, (U32)pQueue);
        DBG_Getch();
    }

    if (IsSLCBlk == TRUE)
    {
        while (pQueue->pSize(pQueue, TRUE) > Rsv_EraseQueueSize)
        {
            if (SUPERPU_LUN_NUM_BITMSK == pQueue->ulEraseBitMap)
            {
                pQueue->ulEraseBitMap = 0;
                L2_VBTSetInEraseQueue(ucSuperPuNum, pQueue->pDe(pQueue, TRUE), FALSE); // Delete current vblock
                g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt --;
                if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt == 0)
                    return INVALID_4F;
                else
                    continue;
            }
            else
            {
                usFreeVBN = pQueue->aVBN[pQueue->ucFront];
                break;
            }
        }
    }
    else
    {
        while (pQueue->pSize(pQueue, FALSE) > Rsv_EraseQueueSize)
        {
            if (SUPERPU_LUN_NUM_BITMSK == pQueue->ulTLCEraseBitMap)
            {
                pQueue->ulTLCEraseBitMap = 0;
                L2_VBTSetInEraseQueue(ucSuperPuNum, pQueue->pDe(pQueue, FALSE), FALSE); // Delete current vblock
                g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt --;
                if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt == 0)
                    return INVALID_4F;
                else
                    continue;
            }
            else
            {
                usFreeVBN = pQueue->aTLCVBN[pQueue->ucTLCFront];
                break;
            }
        }
    }
    
    return usFreeVBN;
}

GLOBAL void L2_ClearEraseInfo(U8 ucSuperPuNum)
{
	U8 ucLunInSuperPu, EraseBlkCnt;

    for (ucLunInSuperPu= 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (EraseBlkCnt= 0; EraseBlkCnt < MAX_ERASE_SLCBLK_CNT; EraseBlkCnt++)
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[ucLunInSuperPu][EraseBlkCnt] = INVALID_4F;
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[EraseBlkCnt] = INVALID_4F;
        }
		
        for (EraseBlkCnt= 0; EraseBlkCnt < MAX_ERASE_TLCBLK_CNT; EraseBlkCnt++)
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCPBN[ucLunInSuperPu][EraseBlkCnt] = INVALID_4F;
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCVBN[EraseBlkCnt] = INVALID_4F;
        }
    }

    g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt = 0;
    g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt = 0;
}


GLOBAL BOOL L2_SetEraseInfoAfterFlushPMT(U8 ucSuperPuNum)
{
    U8 i, ucLunInSuperPu;
    U16 uQueueSize, uPBN, uVBN, uTempFront;
    BOOL bNeedErase = FALSE;
    EraseQueue *pQueue = NULL;

    pQueue = g_EraseQueue[ucSuperPuNum];

    if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt != 0 || g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt != 0)
    {
        DBG_Printf("SLC %d, TLC %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt, g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
        DBG_Getch();
    }

    // check is SLC queue empty.
    if (!pQueue->pIsEmpty(pQueue, TRUE))
    {        
        uQueueSize = pQueue->pSize(pQueue, TRUE);
        if (uQueueSize >= MAX_ERASE_SLCBLK_CNT)
        {
            uQueueSize = MAX_ERASE_SLCBLK_CNT;
            g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt = MAX_ERASE_SLCBLK_CNT;
        }
        else if (uQueueSize == 1)
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt = 1;
        }
        else
        {
            g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt = (U8)uQueueSize;
        }

        for (i = 0; i < uQueueSize; i++)
        {
            uTempFront = (pQueue->ucFront + i) % (ERASE_QUEUE_SIZE + 1);
            uVBN = pQueue->aVBN[uTempFront];
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("SLC VB %d, ", uVBN);
#endif
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCVBN[i] = uVBN;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                uPBN = pVBT[ucSuperPuNum]->m_VBT[uVBN].PhysicalBlockAddr[ucLunInSuperPu];
                g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseSLCPBN[ucLunInSuperPu][i] = uPBN;
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("PB %d, ", uPBN);
#endif
            }
        }
        bNeedErase = TRUE;
#ifdef SPOR_DEBUGLOG_ENABLE
        DBG_Printf("\n");
#endif
        if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt > 2)
        {
            DBG_Printf("Erase SLC cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt);
            DBG_Getch();
        }
#ifdef SPOR_DEBUGLOG_ENABLE
        DBG_Printf("SLC Erase cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt);
#endif
    }

    // check is TLC queue empty.
    if (!pQueue->pIsEmpty(pQueue, FALSE))
    {   
        uQueueSize = pQueue->pSize(pQueue, FALSE);

        if (uQueueSize >= MAX_ERASE_TLCBLK_CNT)
        {
            uQueueSize = MAX_ERASE_TLCBLK_CNT;
            g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt = MAX_ERASE_TLCBLK_CNT;
        }

        for (i = 0; i < uQueueSize; i++)
        {
            uTempFront = (pQueue->ucTLCFront + i) % (TLC_BLK_CNT + 1);
            uVBN = pQueue->aTLCVBN[uTempFront];
#ifdef SPOR_DEBUGLOG_ENABLE
            DBG_Printf("TLC VB %d, ", uVBN);
#endif
            g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCVBN[i] = uVBN;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                uPBN = pVBT[ucSuperPuNum]->m_VBT[uVBN].PhysicalBlockAddr[ucLunInSuperPu];
                g_NeedEraseBlkInfo[ucSuperPuNum].uNeedEraseTLCPBN[ucLunInSuperPu][i] = uPBN;
#ifdef SPOR_DEBUGLOG_ENABLE
                DBG_Printf("PB %d, ", uPBN);
#endif
            }
        }

        g_EraseOpt[ucSuperPuNum].uErasingTLCVBN = uVBN;

#ifdef SPOR_DEBUGLOG_ENABLE
        DBG_Printf("\n");
#endif

        if (bNeedErase == FALSE)
        {
            bNeedErase = TRUE;
        }

        if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt > 1)
        {
            DBG_Printf("TLC cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
            DBG_Getch();
        }
#ifdef SPOR_DEBUGLOG_ENABLE
        DBG_Printf("TLC Erase cnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
#endif
    }
	
    if (bNeedErase == TRUE)
    {
        if ((g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt + g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt) > 3)
        {
            DBG_Printf("SLCcnt %d, TLCcnt %d\n", g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt, g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt);
            DBG_Getch();
        }
    }
     
    return bNeedErase;
}

GLOBAL U16 L2_SelectNeedEraseBlkFromQueue(U8 ucSuperPuNum)
{
    U16 usVBN = INVALID_4F;

    if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseSLCCnt != 0)
    {
        /* return INVALID_4F: Erase SLC block quota is done; otherwise return need erase SLC block VBN */
        usVBN = L2_PopBlkFromEraseQueue(ucSuperPuNum, TRUE);
        if (usVBN != INVALID_4F)
            return usVBN;
    }

    if (g_NeedEraseBlkInfo[ucSuperPuNum].ucTotalEraseTLCCnt != 0)
    {
        /* return INVALID_4F: Erase TLC block quota is done; otherwise return need erase TLC block VBN */
        usVBN = L2_PopBlkFromEraseQueue(ucSuperPuNum, FALSE);
        if (usVBN != INVALID_4F)
            return usVBN;
    }

    return usVBN;
}
