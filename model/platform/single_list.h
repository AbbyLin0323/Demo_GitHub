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

#ifndef C_SINGLE_LIST_H_
#define C_SINGLE_LIST_H_

typedef struct SINGLE_LIST_NODE * SL_ITERATOR;
typedef struct SINGLE_LIST * SL_LISTPTR;

typedef enum SINGLE_LIST_RESULT {
  Sl_kSuccess,
  Sl_kBadAlloc
}SL_RESULT;

SL_RESULT Sl_ListCreate(SL_LISTPTR *pListPtr);
SL_RESULT Sl_PushBack(SL_LISTPTR pListPtr, void *pContent);
SL_ITERATOR Sl_Begin(const SL_LISTPTR pListPtr);
SL_ITERATOR Sl_End(const SL_LISTPTR pListPtr);
void Sl_Forward(SL_ITERATOR *ptIterator);
void * Sl_GetContent(SL_ITERATOR tIterator);
void Sl_ListDestroy(SL_LISTPTR *pListPtr);

#endif  // C_SINGLE_LIST_H_
