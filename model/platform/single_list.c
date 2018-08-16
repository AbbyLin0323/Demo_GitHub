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

#include "single_list.h"
#include <stdlib.h>
#include <assert.h>

typedef struct SINGLE_LIST_NODE
{
    void *pContent;
    struct SINGLE_LIST_NODE *ptNext;
} SL_NODE;

typedef enum SINGLE_LIST_STATUS
{
    Sl_kCreated = 1,
    Sl_kDestroyed
} SL_STATUS;

typedef struct SINGLE_LIST
{
    SL_NODE *ptHead;
    SL_NODE *ptTail;
    int length;
    SL_STATUS eStatus;
} SL_LIST;

SL_RESULT Sl_ListCreate(SL_LISTPTR *pListPtr)
{
    assert(pListPtr != NULL);

    SL_LIST *pNewList = malloc(sizeof(SL_LIST));
    if (NULL == pNewList)
    {
        return Sl_kBadAlloc;
    }
    else
    {
        SL_NODE *pHeaderNode = malloc(sizeof(SL_NODE));
        if (NULL == pHeaderNode)
        {
            free(pNewList);
            return Sl_kBadAlloc;
        }

        pHeaderNode->pContent = NULL;
        pHeaderNode->ptNext = NULL;
        pNewList->ptHead = pNewList->ptTail = pHeaderNode;
        pNewList->length = 0;
        pNewList->eStatus = Sl_kCreated;
        *pListPtr = pNewList;
        return Sl_kSuccess;
    }
}

SL_RESULT Sl_PushBack(SL_LISTPTR pListPtr, void *pContent)
{
    assert(pListPtr != NULL);
    assert(pListPtr->eStatus == Sl_kCreated);
    assert(pListPtr->length >= 0);
    assert(pListPtr->ptHead != NULL);
    assert(pListPtr->ptTail != NULL);

    SL_NODE *pNewNode = malloc(sizeof(SL_NODE));
    if (NULL == pNewNode)
    {
        return Sl_kBadAlloc;
    }

    pNewNode->pContent = pContent;
    pNewNode->ptNext = NULL;
    pListPtr->ptTail->ptNext = pNewNode;
    pListPtr->ptTail = pNewNode;
    ++(pListPtr->length);
    return Sl_kSuccess;
}

SL_ITERATOR Sl_Begin(const SL_LISTPTR pListPtr)
{
    assert(pListPtr != NULL);
    assert(pListPtr->eStatus == Sl_kCreated);
    assert(pListPtr->length >= 0);
    assert(pListPtr->ptHead != NULL);
    assert(pListPtr->ptTail != NULL);

    return pListPtr->ptHead->ptNext;
}

SL_ITERATOR Sl_End(const SL_LISTPTR pListPtr)
{
    assert(pListPtr != NULL);
    assert(pListPtr->eStatus == Sl_kCreated);
    assert(pListPtr->length >= 0);
    assert(pListPtr->ptHead != NULL);
    assert(pListPtr->ptTail != NULL);

    return pListPtr->ptTail->ptNext;
}

void Sl_Forward(SL_ITERATOR *ptIterator)
{
    assert(ptIterator != NULL);
    assert((*ptIterator) != NULL);

    *ptIterator = (*ptIterator)->ptNext;
    return;
}

void* Sl_GetContent(SL_ITERATOR tIterator)
{
    assert(tIterator != NULL);
    return tIterator->pContent;
}

void Sl_ListDestroy(SL_LISTPTR *pListPtr)
{
    assert(pListPtr != NULL);
    assert(*pListPtr != NULL);
    assert((*pListPtr)->eStatus == Sl_kCreated);
    assert((*pListPtr)->length >= 0);
    assert((*pListPtr)->ptHead != NULL);
    assert((*pListPtr)->ptTail != NULL);

    for (SL_ITERATOR tIterator = Sl_Begin(*pListPtr); tIterator != Sl_End(*pListPtr);)
    {
        SL_ITERATOR tReleaseNode = tIterator;
        Sl_Forward(&tIterator);
        tReleaseNode->pContent = NULL;
        tReleaseNode->ptNext = NULL;
        free(tReleaseNode);
        tReleaseNode = NULL;
    }

    (*pListPtr)->ptTail = NULL;
    free((*pListPtr)->ptHead);
    (*pListPtr)->ptHead = NULL;
    free(*pListPtr);
    *pListPtr = NULL;
    return;
}
