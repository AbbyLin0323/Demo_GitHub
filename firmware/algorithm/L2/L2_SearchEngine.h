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
File Name     : L2_SearchEngine.h
Version       : Ver 1.0
Author        : henryluo
Created       : 2014/10/16
Last Modified :
Description   :
Function List :
History       :
1.Date        : 2014/10/16
Author      : henryluo
Modification: Created file

*******************************************************************************/
#ifndef _L2_SEARCHENGINE_H_
#define _L2_SEARCHENGINE_H_


typedef enum _SE_SEARCH_CASE
{
    FREE_ERASE_CNT_MAX = 0,
    FREE_ERASE_CNT_MIN,
    BACKUP_ERASE_CNT_MAX,
    BACKUP_ERASE_CNT_MIN,
    BROKEN_ERASE_CNT_MAX,
    BROKEN_ERASE_CNT_MIN,
    SEARCH_CASE_MAX                              //search case param in DRAM support max to 8 
}SE_SEARCH_CASE;


void L2_SearchEngineSWInit(void);
void L2_SESearchBlock(U32 ulSearchCase, U32 *pStartItem, U8 ucTLCBlk, SE_SEARCH_VALUE_RESULT* pSEGetResult);

#endif
