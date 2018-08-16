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
Filename    :L1_Cache.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    17:56:30
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "COM_BitMask.h"

#ifdef SIM
#include "Windows.h"
#endif

GLOBAL  U8 g_ucPuNumBits;
GLOBAL  U8 g_ucPuNumMask;
GLOBAL  U8 g_ucAllPuMask;
GLOBAL  U8 g_aucPuShiftWidth[3];
GLOBAL  U8 g_ucNewLbaHashing;
GLOBAL  U16 g_aCacheLine[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U32 g_ulCacheWriteTime[SUBSYSTEM_SUPERPU_MAX];

GLOBAL  U16 *g_pStartCacheID;

GLOBAL  U16 *g_pNextCacheID;

#ifndef LCT_VALID_REMOVED
GLOBAL  U32 *g_pulLCTValid;
#endif

#ifndef LCT_TRIM_REMOVED
GLOBAL  U32 *g_pulTrim;
GLOBAL  U32 g_ulTrimSearchRangeStartLct;
GLOBAL  U32 g_ulTrimSearchRangeEndLct;
#endif
GLOBAL  U32 g_ulCurTrimTargetLct;
GLOBAL  U32 g_ulCurTrimTargetLctLpnOffset;
extern GLOBAL U32 g_ulPMTFlushing;

/* extern Trim related interface from L2 */
extern void L2_TrimReqProcess(U32 TrimStartLPN, U32 TrimLPNCount);
extern BOOL L2_IsBootupOK(void);

void MCU1_DRAM_TEXT L1_CacheSram1Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign( &ulFreeSramBase );


    g_pNextCacheID = (U16 *)ulFreeSramBase;

    COM_MemIncBaseAddr( &ulFreeSramBase, COM_MemSize16DWAlign(L1_CACHEID_NUM * sizeof(U16)) );
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    *pFreeSramBase = ulFreeSramBase;
    return;
}

/****************************************************************************
Name        :L1_CacheDramMap
Input       :
Output      :
Author      :Peter Xiu
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheDramMap(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;
    U32 ulDramOffSet = 0;

#ifdef DCACHE
    ulDramOffSet = DRAM_HIGH_ADDR_OFFSET;
#endif  

    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddrPageBoundaryAlign( &ulFreeDramBase );

#ifndef LCT_VALID_REMOVED
    if(VBMT_PAGE_SIZE_PER_PU > (BUF_SIZE*VBMT_PAGE_COUNT_PER_PU))
    {
        DBG_Printf("gLCTValid size overflow 0x%x 0x%x\n", VBMT_PAGE_SIZE_PER_PU, (BUF_SIZE*VBMT_PAGE_COUNT_PER_PU));
        DBG_Getch();
    }

    g_pulLCTValid = (U32 *)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign( BUF_SIZE*VBMT_PAGE_COUNT_PER_PU ) );
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
#endif

#ifndef LCT_TRIM_REMOVED
    g_pulTrim = (U32 *)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(LCT_TAG_DEPTH*sizeof(U32)));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
#endif

    g_pStartCacheID = (U16*)(ulFreeDramBase + ulDramOffSet);

#ifdef CACHE_LINK_MULTI_LCT
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH_REDUCED * sizeof(U16)));
#else
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH * sizeof(U16)));
#endif
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return;
}


/****************************************************************************
Name        :L1_CacheOTFBMap
Input       :
Output      :
Author      :Peter Xiu
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
LOCAL void L1_CacheOTFBMap(U32 *pFreeOTFBBase)
{
    return;
}

/****************************************************************************
Name        :L1_CacheInit
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.02.20    16:16:45
Description :Initialize cache
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheInit(void)
{
    U32 i;
    U8 ucPuNum;
    BOOTLOADER_FILE* pBootFileAddr;
    PTABLE* pPTable;

    // setup the access to p-table so we can determine
    // which lba hashing method we're going to use
    pBootFileAddr = (BOOTLOADER_FILE*)OTFB_BOOTLOADER_BASE;
    pPTable = (PTABLE*)&(pBootFileAddr->tSysParameterTable);

    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_aCacheLine[i] = INVALID_4F;
        g_ulCacheWriteTime[i] = 0;
    }

    g_ulActiveWriteBufferBitmap = 0;

#ifdef CACHE_LINK_MULTI_LCT
    HAL_DMAESetValue((U32)g_pStartCacheID, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH_REDUCED * sizeof(U16)), INVALID_8F);
#else
    HAL_DMAESetValue((U32)g_pStartCacheID, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH * sizeof(U16)), INVALID_8F);
#endif
#ifndef LCT_VALID_REMOVED
    HAL_DMAESetValue((U32)g_pulLCTValid, COM_MemSize16DWAlign(BUF_SIZE*VBMT_PAGE_COUNT_PER_PU), 0);
#endif
#ifndef LCT_TRIM_REMOVED
    HAL_DMAESetValue((U32)g_pulTrim, COM_MemSize16DWAlign(LCT_TAG_DEPTH * sizeof(U32)), 0);
#endif
    HAL_DMAESetValue((U32)g_pNextCacheID, COM_MemSize16DWAlign( L1_CACHEID_NUM * sizeof(g_pNextCacheID[0]) ), INVALID_8F);

#ifndef LCT_TRIM_REMOVED
    g_ulTrimSearchRangeStartLct    = LCT_ENTRY_DEPTH;
    g_ulTrimSearchRangeEndLct    = 0;

    g_ulPendingTrimLctCount = 0;
#endif

    g_ulCurTrimTargetLct = INVALID_8F;
    g_ulCurTrimTargetLctLpnOffset = INVALID_8F;

    // Sean Gao 20150316
    // calculate the PU number bits and mask, which are
    // needed for our PU calculation method

    ucPuNum = SUBSYSTEM_SUPERPU_NUM;

    g_ucAllPuMask = 0;
    for(i = 0 ; i < ucPuNum; ++i)
    {
        g_ucAllPuMask += (1 << i);
    }

    g_ucPuNumBits = 0;

    while(ucPuNum != 0)
    {
        ucPuNum = ucPuNum >> 1;
        g_ucPuNumBits++;
    }
    g_ucPuNumBits -= 1;

    g_ucPuNumMask = 0;
    for(i = 0; i < g_ucPuNumBits; ++i)
    {
        g_ucPuNumMask += (1 << i);
    }

    // set up the g_aucPuShiftWidth, which is used by the new
    // lba hashing method
    g_aucPuShiftWidth[0] = g_ucPuNumBits;
    g_aucPuShiftWidth[1] = g_ucPuNumBits*2;
    g_aucPuShiftWidth[2] = g_ucPuNumBits*3;

    // determine which lba hashing method we're going to use
    g_ucNewLbaHashing = ((pPTable->tL1Feature.ulFeatureBitMap & 1) == 1) ? TRUE : FALSE;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    return;
}

void MCU1_DRAM_TEXT L1_CacheWarmInit(void)
{
    U32 i;

    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_aCacheLine[i] = INVALID_4F;
        g_ulCacheWriteTime[i] = 0;
    }
    g_ulActiveWriteBufferBitmap = 0;

#ifdef CACHE_LINK_MULTI_LCT
    HAL_DMAESetValue((U32)g_pStartCacheID, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH_REDUCED * sizeof(U16) ), INVALID_8F);
#else
    HAL_DMAESetValue((U32)g_pStartCacheID, COM_MemSize16DWAlign(LCT_ENTRY_DEPTH * sizeof(U16)), INVALID_8F);
#endif
    HAL_DMAESetValue((U32)g_pNextCacheID, COM_MemSize16DWAlign( L1_CACHEID_NUM * sizeof(g_pNextCacheID[0]) ), INVALID_8F);

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    return;
}

#ifndef LCT_TRIM_REMOVED
void MCU1_DRAM_TEXT L1_CacheResetLCTTag(void)
{
#ifndef LCT_VALID_REMOVED
    HAL_DMAESetValue((U32)g_pulLCTValid, COM_MemSize16DWAlign(BUF_SIZE*VBMT_PAGE_COUNT_PER_PU), 0);
#endif
    HAL_DMAESetValue((U32)g_pulTrim, COM_MemSize16DWAlign(LCT_TAG_DEPTH * sizeof(U32)), 0);

    return;
}
#endif

#ifndef LCT_VALID_REMOVED
/****************************************************************************
Name        :L1_RebuildLCTTag()
Input       : 
Output      :BufferLPN()
Author      :BlakeZhang 
Date        :2013.07.09
Description :set all LCT Items to valid in boot up fail
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_RebuildLCTTag(void)
{
    HAL_DMAESetValue((U32)g_pulLCTValid, COM_MemSize16DWAlign(BUF_SIZE*VBMT_PAGE_COUNT_PER_PU), INVALID_8F);

    return;
}
#endif

#ifndef LCT_TRIM_REMOVED
void MCU1_DRAM_TEXT L1_ResetLCTTrim(void)
{
    HAL_DMAESetValue((U32)g_pulTrim, COM_MemSize16DWAlign(LCT_TAG_DEPTH * sizeof(U32)), 0);
}
#endif

#if 0
void MCU1_DRAM_TEXT L1_MapLCTWriteCnt(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;
    
    ulFreeDramBase = *pFreeDramBase;

    g_pLCTWriteCnt = &g_LCTWriteCnt;
    g_pLCTWriteCnt->LCT_WriteCnt = ( U16* )ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign( LCT_ENTRY_DEPTH*sizeof(g_pLCTWriteCnt->LCT_WriteCnt[0]) ) );
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;
  
    return;
}

void MCU1_DRAM_TEXT L1_ResetLCTWriteCnt(void)
{  
    HAL_DMAESetValue((U32)g_pLCTWriteCnt->LCT_WriteCnt, COM_MemSize16DWAlign( LCT_ENTRY_DEPTH*sizeof(g_pLCTWriteCnt->LCT_WriteCnt[0]) ), 0);
    return;
}

/****************************************************************************
Function  : L1_IncreaseLCTWriteCnt
Input     : 
Output    : 
Return    : 
Purpose   : 
update LCT write counter after write host command received
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void L1_IncreaseLCTWriteCnt(U32 ulLCTIndex)
{
    if (NULL != g_pLCTWriteCnt)
    {
        if (TRUE == L2_IsBootupOK())
        {
            g_pLCTWriteCnt->LCT_WriteCnt[ulLCTIndex]++;
        }
    }

    return;
}
#endif

// Sean Gao 20150316
// this function calculates the PU which the input ulLpn
// belongs to
//
// please note that:
// 1. both L2_GetSuperPuFromLPN and L2_GetPuFromLPN calculate
// PU number, their contents should be exactly the same
// 2. we should always calculate the PU number using these
// functions

U8 L1_GetSuperPuFromLCT(U32 ulLct)
{
    U8 ucTargetPu;

    if (1 == SUBSYSTEM_SUPERPU_NUM)
    {
        return 0;
    }

    if(g_ucNewLbaHashing == TRUE)
    {
        // lba hashing bit is set in p-table, use the new method
        // calculate the target PU
        ucTargetPu =    
            (   ( ulLct )^
                ( ulLct >> g_aucPuShiftWidth[0] )^ 
                ( ulLct >> g_aucPuShiftWidth[1] )^
                ( ulLct >> g_aucPuShiftWidth[2] )
            ) & g_ucPuNumMask;
    }
    else
    {
        // lba hashing bit is not set, use the original method
        ucTargetPu = ulLct % SUBSYSTEM_SUPERPU_NUM;
    }
    
    return ucTargetPu;
}

#ifndef LCT_VALID_REMOVED
/****************************************************************************
Name        :L1_SetLCTValid()
Input       :  LogicBufferID, LPN postion
Output      :BufferLPN()
Author      :BlakeZhang 
Date        :2012.11.19
Description :calculate the addr of BufferLPN. 
Others      :
Modify      :
****************************************************************************/
void L1_SetLCTValid(U32 ulLCTIndex, BOOL bValid)
{
    U32 ulLCTTagIndex;
    U32 ulLCTTagMask;

    ulLCTTagIndex = LCTTAG_FROM_INDEX(ulLCTIndex);
    ulLCTTagMask  = LCTMSK_FROM_INDEX(ulLCTIndex);

    if (bValid)
    {
        g_pulLCTValid[ulLCTTagIndex] |= (1 << ulLCTTagMask);
    }
    else
    {
        g_pulLCTValid[ulLCTTagIndex] &= (~(1 << ulLCTTagMask));
    }

    return;
}

/****************************************************************************
Name        :L1_GetLCTValid()
Input       :  LogicBufferID, LPN postion
Output      :BufferLPNAdd()
Author      :BlakeZhang 
Date        :2012.11.19   
Description :calculate the addr of BufferLPN. 
Others      :
Modify      :
****************************************************************************/
BOOL L1_GetLCTValid(U32 ulLCTIndex)
{
    U32 ulLCTTagIndex;
    U32 ulLCTTagMask;
    
    ulLCTTagIndex = LCTTAG_FROM_INDEX(ulLCTIndex);
    ulLCTTagMask  = LCTMSK_FROM_INDEX(ulLCTIndex);

    if (0 != ((g_pulLCTValid[ulLCTTagIndex]) & (1 << ulLCTTagMask)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif

/****************************************************************************
Name        :L1_SetNextCacheId()
Input       : 
Output      :
Author      :BlakeZhang 
Date        :
Description :set the next cache id of CurCacheID to NextCacheID
Others      :
Modify      :
****************************************************************************/
void L1_SetNextCacheId(U16 CurCacheID, U16 NextCacheID)
{
    g_pNextCacheID[CurCacheID] = NextCacheID;

    return;
}

/****************************************************************************
Name        :L1_GetNextCacheId()
Input       :
Output      :
Author      :BlakeZhang 
Date        :  
Description :
Others      :
Modify      :
****************************************************************************/
U16 L1_GetNextCacheId(U16 CurCacheID)
{
    return (g_pNextCacheID[CurCacheID]);
}

/****************************************************************************
Name        :L1_SetLCTStartId()
Input       :  LogicBufferID, LPN postion
Output      :BufferLPN()
Author      :BlakeZhang 
Date        :2012.11.19
Description :calculate the addr of BufferLPN. 
Others      :ZacGao 2017.02.20
Modify      :add the related code about CACHE_LINK_MULTI_LCT
****************************************************************************/
void L1_SetLCTStartCache(U32 ulLCTIndex, U16 usStartCache)
{
#ifdef CACHE_LINK_MULTI_LCT
    U32 LCTOffset;

    LCTOffset = ulLCTIndex % LCT_ENTRY_DEPTH_REDUCED;

    g_pStartCacheID[LCTOffset] = usStartCache;
  
#else
    g_pStartCacheID[ulLCTIndex] = usStartCache;
#endif

    return;
}

/****************************************************************************
Name        :L1_GetLCTStartCache()
Input       :
Output      :
Author      :BlakeZhang 
Date        :  
Description :
Others      :ZacGao 2017.02.20
Modify      :add the related code about CACHE_LINK_MULTI_LCT
****************************************************************************/
U16 L1_GetLCTStartCache(U32 ulLCTIndex)
{
#ifdef CACHE_LINK_MULTI_LCT
    U32 LCTOffset;

    LCTOffset = ulLCTIndex % LCT_ENTRY_DEPTH_REDUCED;
    return (g_pStartCacheID[LCTOffset]);
#else
     return (g_pStartCacheID[ulLCTIndex]);
#endif
}

#ifndef LCT_TRIM_REMOVED
/****************************************************************************
Name        :
Input       :
Output      :
Author      :BlakeZhang 
Date        :
Description : 
Others      :
Modify      :
****************************************************************************/
void L1_SetLCTTrim(U32 ulLCTIndex, U8 bTrimed)
{
    U32 ulLCTTagIndex;
    U32 ulLCTTagMask;
    
    ulLCTTagIndex = LCTTAG_FROM_INDEX(ulLCTIndex);
    ulLCTTagMask  = LCTMSK_FROM_INDEX(ulLCTIndex);
    
    if (bTrimed)
    {
        g_pulTrim[ulLCTTagIndex] |= (1 << ulLCTTagMask);
    }
    else
    {
        g_pulTrim[ulLCTTagIndex] &= (~(1 << ulLCTTagMask));
    }

    return;
}

/****************************************************************************
Name        :
Input       : 
Output      :
Author      :BlakeZhang 
Date        :   
Description :
Others      :
Modify      :
****************************************************************************/
U8 L1_GetLCTTrim(U32 ulLCTIndex)
{
    U32 ulLCTTagIndex;
    U32 ulLCTTagMask;
    
    ulLCTTagIndex = LCTTAG_FROM_INDEX(ulLCTIndex);
    ulLCTTagMask  = LCTMSK_FROM_INDEX(ulLCTIndex);

    if (0 != ((g_pulTrim[ulLCTTagIndex]) & (1 << ulLCTTagMask)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif

#ifndef LCT_VALID_REMOVED
/****************************************************************************
Name        :L1_MarkLCTValid()
Input       :  U32 ulStartLCT, U32 ulEndLCT
Output      : none
Author      :BlakeZhang 
Date        :2015.08.21
Description :mark LCT valid for given range
Others      :
Modify      :
****************************************************************************/
void L1_MarkLCTValid(U32 ulStartLCT, U32 ulEndLCT)
{
    U32 ulLCTIndex = ulStartLCT;
  
    while (ulLCTIndex < ulEndLCT)
    {
        // mark all LCTs in the trim range
        if (LCTMSK_FROM_INDEX(ulLCTIndex) != 0)
        {
            L1_SetLCTValid(ulLCTIndex, FALSE);
            ulLCTIndex++;
        }
        else
        {
            if ((ulLCTIndex + DWORD_BIT_SIZE) < ulEndLCT)
            {
                g_pulLCTValid[LCTTAG_FROM_INDEX(ulLCTIndex)] = 0;
                ulLCTIndex += DWORD_BIT_SIZE;
            }
            else
            {
                L1_SetLCTValid(ulLCTIndex, FALSE);
                ulLCTIndex++;
            }
        }
    }

    return;
}
#endif

#ifndef LCT_TRIM_REMOVED
/****************************************************************************
Function  : L1_UpdateLCTValid
Input     : 
Output    : 
Return    : 
Purpose   : 
update LCT Valid/Trim flag after write host command received
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
BOOL L1_UpdateLCTValid(U32 ulLCTIndex, SUBCMD* pSubCmd)
{
    // Sean Gao 20151020
    // clean up this function and update comments to make it more clear

    // if the trim flag of the current LCT is set, we have to process it
    if(L1_GetLCTTrim(ulLCTIndex) == TRUE)
    {
        if(TRUE == L2_IsBootupOK())
        {
            if(pSubCmd != NULL)
            {
                // we only need to trim LPNs that are not going to be written by the current write subcommand
                U8 ucFirstLpnOffset = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
                U8 ucLastLpnOffset = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN + pSubCmd->SubCmdAddInfo.ucSubLPNCountIN - 1;
                U32 ulLctStartLpn = LPN_FROM_LCTINDEX(ulLCTIndex);

                if(ucFirstLpnOffset != 0 && FALSE == g_ulPMTFlushing)
                {
                    L2_TrimReqProcess(ulLctStartLpn, ucFirstLpnOffset);
                }

                if(ucLastLpnOffset != (LPN_PER_BUF - 1) && FALSE == g_ulPMTFlushing)
                {
                    L2_TrimReqProcess(ulLctStartLpn + ucLastLpnOffset + 1, LPN_PER_BUF - ucLastLpnOffset - 1 );
                }
            }
            else
            {
                // this shouldn't happen at all, an input subcommand is required
                DBG_Printf("L1_UpdateLCTValid input subcommand error\n");
                DBG_Getch();
            }
        }

        // check if the input LCT is also the current trim target LCT, if it is,
        // reset current trim target LCT since it's just been trimmed
        if(g_ulCurTrimTargetLct == ulLCTIndex)
        {
            g_ulCurTrimTargetLct = INVALID_8F;
            g_ulCurTrimTargetLctLpnOffset = INVALID_8F;
        }

        // clear the LCT trim flag
        L1_SetLCTTrim(ulLCTIndex, FALSE);

        // update the number of pending trim LCT count
        g_ulPendingTrimLctCount--;
    }

#ifndef LCT_VALID_REMOVED
    // set the valid flag of the current LCT
    L1_SetLCTValid(ulLCTIndex, TRUE);
#endif

    return TRUE;
}
#endif

/****************************************************************************
Name        :  L1_CacheLinkFindCurrNode
Input       :  
Output      :
Author      :  BlakeZhang 
Date        :  
Description :  Find a cache in the CacheLink of ulLCTIndex,
               that its next cache id is usNextCacheID 
Others      :
Modify      :
****************************************************************************/
U16 L1_CacheLinkFindCurrNode(U32 ulLCTIndex, U16 usNextCacheID)
{
    U16 ucLoop;
    U16 usCurCacheID;
    U16 usNextCacheTemp;

    /*get Start Cache ID of this LCT*/
    usCurCacheID = L1_GetLCTStartCache(ulLCTIndex);
    
    if (usNextCacheID == usCurCacheID)
    {      
        /* 1. usNextCacheID == INVALID_4F, means Start Cache ID == INVALID_4F, this LCT has no data in buffer*/
        /* 2. usNextCacheID != INVALID_4F, it is start cache, no previous node*/
        return INVALID_4F;
    }

    /* search for CacheID in LCT ID Link */
#ifdef CACHE_LINK_MULTI_LCT
    for (ucLoop = 0; ucLoop < (LPN_PER_BUF * L1_CACHE_LINK_LCT_NUM); ucLoop++)
#else
    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
#endif
    {
        usNextCacheTemp = L1_GetNextCacheId(usCurCacheID);
      
        if (usNextCacheTemp == usNextCacheID)
        {
            /*usCurCacheID's next cache is usNextCacheID,find*/
            break;
        }
        else
        {  
            /*not current node,get its next cache id,update current node*/
            usCurCacheID = usNextCacheTemp;
            if (usCurCacheID == INVALID_4F && usNextCacheID != INVALID_4F)
            {
                /*seek all CacheLink of ulLCTIndex,but not find*/
                DBG_Printf ("L1_CacheLinkFindCurrNode: not find,ulLCTIndex 0x%x ERROR!!!!\n", ulLCTIndex);
                DBG_Getch();
            }
        }
    }

    return usCurCacheID;
}

/****************************************************************************
Name        : L1_CalcCacheID
Input       : 
Output      : 
Author      : BlakeZhang 
Date        :
Description : calculate cache id from buffer id and cache offset in this buffer
Others      :
Modify      :
****************************************************************************/
U16 L1_CalcCacheID(U16 usLogBufID, U8 ucCacheOffset)
{
    U16 usCacheID;
    U16 usWBufStartLogID;

    usWBufStartLogID = LGCBUFID_FROM_PHYBUFID(g_ulWriteBufStartPhyId);
    usCacheID = (usLogBufID - usWBufStartLogID) * LPN_PER_BUF + ucCacheOffset;

    return usCacheID;
}

/****************************************************************************
Name        : L1_UpdateCacheIdLink
Input       : 
Output      :
Author      : BlakeZhang 
Date        : 
Description :
Others      :
Modify      :
****************************************************************************/
void L1_UpdateCacheIdLink(SUBCMD * pSubCmd)
{
    U8  ucLoop;
    U16 usLogBufID;
    U16 usCurrCacheID;
    U16 usNextCacheID;
    U32 ulLCTIndex;

    usLogBufID = LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID);
    ulLCTIndex = pSubCmd->SubCmdAddInfo.ulSubCmdLCT;

    /*get the tail of this LCT's CacheLink*/
    usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, INVALID_4F);
    /*calculate the id of new cache*/
    usNextCacheID = L1_CalcCacheID(usLogBufID, pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT);
    
    if (usCurrCacheID == INVALID_4F)
    {
        /*this LCT has no data in buffer previously,set start CacheID*/
        L1_SetLCTStartCache(ulLCTIndex, usNextCacheID);
    }
    else
    {
        /*set new cache as tail*/
        L1_SetNextCacheId(usCurrCacheID, usNextCacheID);
    }

    for (ucLoop = 1; ucLoop < pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT; ucLoop++)
    {
        /*set the CacheID for the following LPN*/
        L1_SetNextCacheId(usNextCacheID, usNextCacheID + 1);
        usNextCacheID++;
    }

    /*set last CacheID to INVALID_4F*/
    L1_SetNextCacheId(usNextCacheID, INVALID_4F);

    return;
}

void L1_AddNewCacheIdNode(U32 ulLpn, U16 usLogBufId, U8 ucCacheOffset)
{
    // Sean Gao 20150602
    // this functions adds a new cache id to its corresponding cache id link

    U16 usTailCacheId;
    U16 usNewCacheId;
    U32 ulLctIndex;

    ulLctIndex = LCTINDEX_FROM_LPN(ulLpn);

    // get the cache id in the tail of the cache id link
    usTailCacheId = L1_CacheLinkFindCurrNode(ulLctIndex, INVALID_4F);

    // calculate the new cache id
    usNewCacheId = L1_CalcCacheID(usLogBufId, ucCacheOffset);
    
    if(usTailCacheId == INVALID_4F)
    {
        // cache id link of the LCT is empty, set the new cache id as the
        // start cache id
        L1_SetLCTStartCache(ulLctIndex, usNewCacheId);
    }
    else
    {
        // set the new cache id as the tail
        L1_SetNextCacheId(usTailCacheId, usNewCacheId);
    }

    // set the next cache id of the new cache id as INVALID_4F
    L1_SetNextCacheId(usNewCacheId, INVALID_4F);

    return;
}

/****************************************************************************
Name        :L1_SubCmdGetLpnSectorMap
Input       :U8 ucSubCmdOffset, U8 ucSubCmdlength , pLpnSectorBitmap
Output      :LastLpnSectorMap
Author      :Blakezhang
Date        :2012.11.21
Description :output all SubCmd Lpn sector bit map
Others      :
Modify      :
****************************************************************************/
#define dwSetBits(StartBit,EndBit) ((0xFFFFFFFF >> (31 - (EndBit - StartBit))) << StartBit)
BOOL L1_SubCmdGetLpnSectorMap(U8 ucSubCmdOffset, U8 ucSubCmdlength, U8 * pLpnSectorBitmap)
{
#if 0
    U8 ucEndBit;
    U8 ucCurrLPN;

    if (pLpnSectorBitmap == NULL || ucSubCmdlength == 0)
    {
        return FALSE;
    }

    ucCurrLPN = 0;

    /* null LPN from the beginning */
    while(ucSubCmdOffset/SEC_PER_LPN != 0)
    {
        ucSubCmdOffset -= SEC_PER_LPN;
        ucCurrLPN++;
    }

    /* LPN with data - first non-aligned LPN */
    if (ucSubCmdOffset != 0)
    {
        ucEndBit = (ucSubCmdOffset + ucSubCmdlength - 1);
        
        if (ucEndBit < SEC_PER_LPN)
        {
            *(pLpnSectorBitmap + ucCurrLPN) = SET_LPN_SECTOR_BIT_MAP(ucSubCmdOffset, ucEndBit);
            ucSubCmdlength = 0;
        }
        else
        {
            *(pLpnSectorBitmap + ucCurrLPN) = SET_LPN_SECTOR_BIT_MAP(ucSubCmdOffset, (SEC_PER_LPN - 1));
            ucSubCmdlength -= (SEC_PER_LPN - ucSubCmdOffset);
        }
        
        ucSubCmdOffset = 0;  /* only the first LPN can have ucSubCmdOffset != 0, clear it here */
        ucCurrLPN++;
    }

    /*  other LPN with data */
    while (ucSubCmdlength != 0)
    {
        ucEndBit = ucSubCmdlength - 1;
        
        if (ucEndBit < SEC_PER_LPN)
        {
            /* last LPN with data */
            *(pLpnSectorBitmap + ucCurrLPN) = SET_LPN_SECTOR_BIT_MAP(0, ucEndBit);
            ucSubCmdlength = 0;
        }
        else
        {
            /* LPN in the middle or first aligned LPN */
            *(pLpnSectorBitmap + ucCurrLPN) = SET_LPN_SECTOR_BIT_MAP(0, (SEC_PER_LPN - 1));
            ucSubCmdlength -= SEC_PER_LPN;
        }

        ucCurrLPN++;
    }//while (ucSubCmdlength != 0)
    
    return TRUE;
#else
    U8 ucEndBit;

    /* null LPN from the beginning */
    while (ucSubCmdOffset / 32 != 0 )
    {
        ucSubCmdOffset -= 32;
        pLpnSectorBitmap += 4;
    }

    while (ucSubCmdlength > 0)
    {
        ucEndBit = ucSubCmdOffset + ucSubCmdlength- 1;
        if (ucEndBit < 32)
        {
            *(U32 *)pLpnSectorBitmap = dwSetBits(ucSubCmdOffset, ucEndBit);
            ucSubCmdlength= 0;
        }
        else
        {
            *(U32 *)pLpnSectorBitmap = dwSetBits(ucSubCmdOffset, 31);
            ucSubCmdlength-= (32 - ucSubCmdOffset);
            ucSubCmdOffset = 0;
            pLpnSectorBitmap += 4;
        }
    }

    return TRUE;
#endif
}

/****************************************************************************
Name        :L1_CacheIDLinkSearchLPN
Input       :SUBCMD *pSubCmd
Output      :L1_CACHE_SEARCH_NO_HIT      //no hit
             L1_CACHE_SEARCH_PARTIAL_HIT    //partial hit
             L1_CACHE_SEARCH_FULL_HIT    //full hit
Author      :blakezhang
Date        :2013.06.29
Description :search the cache hit according to the subcmd's input addr info.
                 if full hit, update the subcmd's output addr info.
Others      :
Modify      :
2013.06.29  blakezhang rewrite for using LCT
****************************************************************************/
U32 L1_CacheIDLinkSearchLPN(U32 ulStartLpn, U8 ucLPNCount, BOOL bCheck)
{
    U8 i;
    U16 usCurrCacheID;
    LCT_HIT_RESULT HitResult;

    // reset the search result
    HitResult.AsU32 = 0;

    // get the start cache id of the current LCT link
    usCurrCacheID = L1_GetLCTStartCache(LCTINDEX_FROM_LPN(ulStartLpn));

    // check if the LCT link is empty
    if(usCurrCacheID == INVALID_4F)
    {
        // link is empty, must be a no hit
        HitResult.ucHitResult = L1_CACHE_SE_NO_HIT;
        return HitResult.AsU32;
    }

    //handle special case
    if (bCheck == TRUE && L1_GetNextCacheId(usCurrCacheID) == INVALID_4F)
    {
        HitResult.usLogBufID = LOGBUFID_FROM_CACHEID(usCurrCacheID);
        HitResult.ucCacheOffset = CACHE_OFFSET_IN_BUF(usCurrCacheID);
        HitResult.ucHitResult = L1_CACHE_SE_FULL_HIT;
        return HitResult.AsU32;
    }

    // search the target LCT link
#ifdef CACHE_LINK_MULTI_LCT
    for (i = 0; i < (LPN_PER_BUF * L1_CACHE_LINK_LCT_NUM); i++)
#else
    for(i = 0; i < LPN_PER_BUF; i++)
#endif
    {
        U16 usLogBufId;
        U8 usBufLpnOffset;
        U32 ulBufLpn;

        // get the corresponding logical buffer id of the current cache id
        usLogBufId = LOGBUFID_FROM_CACHEID(usCurrCacheID);

        // get the corresponding buffer lpn offset
        usBufLpnOffset = CACHE_OFFSET_IN_BUF(usCurrCacheID);

        // get the LPN stored in the buffer LPN entry
        ulBufLpn = L1_GetBufferLPN(usLogBufId, usBufLpnOffset);

        // check if the LPN is in the search range
        if((ulBufLpn >= ulStartLpn) && (ulBufLpn <= (ulStartLpn + ucLPNCount - 1)))
        {
            // the current LPN is in the search range

            // for the current search to be a potential full hit, 2 conditions must hold:
            // 1. the first found LPN must be the start LPN of the request
            // 2. the buffer must have enough space to possibly hold all requested LPNs
            if((ulBufLpn == ulStartLpn) && ((usBufLpnOffset + ucLPNCount) <= L1_BufferGetWritePointer(usLogBufId)))
            {
                // potential full hit, check all entries to see if all requested LPNs are there
                U8 j;

                // check the next id of all cache id nodes except for the last one, for this search to
                // be a full hit all cache id must be sequential
                for(j = 0; j < (ucLPNCount-1); j++)
                {
                    if(L1_GetNextCacheId(usCurrCacheID) != (usCurrCacheID+1))
                    {
                        // not all cache id nodes are sequential, this is a partial hit
                        HitResult.ucHitResult = L1_CACHE_SE_PART_HIT;
                        return HitResult.AsU32;
                    }
                    else
                    {
                        usCurrCacheID++;
                    }
                }

                // check if all LPNs match the request
                for(j = 1; j < ucLPNCount; j++)
                {
                    if(L1_GetBufferLPN(usLogBufId, usBufLpnOffset+j) != (ulStartLpn+j))
                    {
                        // not all LPNs match the request, this is a partial hit
                        HitResult.ucHitResult = L1_CACHE_SE_PART_HIT;
                        return HitResult.AsU32;
                    }
                }

                // if we reach here, all requested LPNs are stored in the buffer sequentially, 
                // this is a full hit
                HitResult.usLogBufID  = usLogBufId;
                HitResult.ucCacheOffset = usBufLpnOffset;
                HitResult.ucHitResult = L1_CACHE_SE_FULL_HIT;
                return HitResult.AsU32;
            }
            else
            {
                HitResult.ucHitResult = L1_CACHE_SE_PART_HIT;
                return HitResult.AsU32;
            }
        }
        else
        {
            /* continue on next CacheID Node */
            usCurrCacheID = L1_GetNextCacheId(usCurrCacheID);
            if (usCurrCacheID == INVALID_4F)
            {
                /* all LPNs aren't in CacheID Link, No Hit */
                HitResult.ucHitResult = L1_CACHE_SE_NO_HIT;
                return HitResult.AsU32;
            }
        }
    }

    DBG_Printf("L1_CacheIDLinkSearchLPN ERROR! ulStartLpn 0x%x \n", ulStartLpn);
    DBG_Getch();
    return HitResult.AsU32;
}

#if 0
/****************************************************************************
Name        :L1_CacheSearchTag
Input       :SUBCMD *pSubCmd
Output      :L1_CACHE_SE_NO_HIT      //no hit
             L1_CACHE_SE_PART_HIT    //partial hit
             L1_CACHE_SE_FULL_HIT    //full hit
Author      :HenryLuo
Date        :2012.02.20    18:01:23
Description :search the cache hit according to the subcmd's input addr info.
                 if full hit, update the subcmd's output addr info.
Others      :
Modify      :
2012.11.29  blakezhang modify for L1 ramdisk design
****************************************************************************/
U8 L1_CacheSearchTag(SUBCMD *pSubCmd)
{
    U8  ucLoop;
    U8  ucSubCmdRw;
    U8  ucCacheOffset;
    U8  ucSubLPNOffset;
    U8  ucSubLPNCount;
    U8  ucCurrLpnBitmap;
    U16 usLogBufID;
    U16 usPhyBufID;
    U32 ulStartLpn;
    LCT_HIT_RESULT HitResult;

    HitResult.AsU32 = 0;

    ucSubCmdRw     = (U8)pSubCmd->pSCMD->tMA.ucOpType;
    ucSubLPNOffset = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    ucSubLPNCount  = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
    ulStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + ucSubLPNOffset;

#ifndef LCT_VALID_REMOVED
    if (ucSubCmdRw == (U8)DM_READ)
    {
        /* read invalid LCT */
        if (FALSE == L1_GetLCTValid(pSubCmd->SubCmdAddInfo.ulSubCmdLCT))
        {
            pSubCmd->SubCmdPhyBufferID = g_L1InvalidBufferPhyId;
            pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
            pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
            pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucSubLPNOffset;
            pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = ucSubLPNCount;
            pSubCmd->CacheStatusAddr = 0;
            return L1_CACHE_SE_RD_INVALID;
        }
    }
#endif

    /*ulStartLpn is LPN align, not buffer align*/
    HitResult.AsU32 = L1_CacheIDLinkSearchLPN(ulStartLpn, ucSubLPNCount, FALSE);

    if (HitResult.ucHitResult != L1_CACHE_SE_FULL_HIT)
    {
        return HitResult.ucHitResult;
    }
    else
    {
        /*first LPN hit*/
        usLogBufID    = HitResult.usLogBufID;
        ucCacheOffset = HitResult.ucCacheOffset;
        usPhyBufID    = PHYBUFID_FROM_LGCBUFID(usLogBufID);
    }

    // if we reach here, it means not only all the requested LPNs
    // are in a buffer, but also all LPNs are in sequential order

    // for read operations, we have to check buffer sector bitmap
    // to ensure that all requested sectors are in the buffer
    if((U8)DM_READ == ucSubCmdRw)
    {
        // we only have to check the buffer sector bitmap when the buffer
        // is in the cache stage, if a buffer is in flush or recycle stage,
        // it must have all sectors of its LPNs
        if(gpBufTag[usLogBufID].Stage == BUF_STAGE_CACHE)
        {
            for(ucLoop = 0; ucLoop < ucSubLPNCount; ucLoop++)
            {
                // get the requested LPN sector bitmap
                ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucSubLPNOffset + ucLoop];

                // check if the buffer has all the sectors we requested
                if((L1_GetBufferSectorBitmap(usLogBufID, ucCacheOffset + ucLoop) & ucCurrLpnBitmap) != ucCurrLpnBitmap)
                {
                    // the buffer doesn't have all requested sectors,
                    // fill in the subcommand info and return as
                    // partial hit
                    pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucCacheOffset << LPN_SECTOR_BIT) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & LPN_SECTOR_MSK);
                    pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
                    pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucCacheOffset;
                    pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = ucSubLPNCount;
                    pSubCmd->SubCmdPhyBufferID = usPhyBufID;
                    return L1_CACHE_SE_RD_BITMAP_PART_HIT;
                }
            } // for(ucLoop = 0; ucLoop < ucSubLPNCount; ucLoop++)
        } // if(gpBufTag[usLogBufID].Stage == BUF_STAGE_CACHE)
    } // if((U8)DM_READ == ucSubCmdRw)

    // for write operations, we have to check if the hit buffer
    // is actually a read cache or prefetch buffer
    if((U8)DM_WRITE == ucSubCmdRw)
    {
        // write full hit on read caches or prefetch buffers both
        // return as write full hit on read caches
        if (BUF_STAGE_FLUSH <= gpBufTag[usLogBufID].Stage || (gpBufTag[usLogBufID].Stage == BUF_STAGE_CACHE && gpBufTag[usLogBufID].bPrefetchBuf == TRUE))
        {
            pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucCacheOffset << LPN_SECTOR_BIT) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & LPN_SECTOR_MSK);
            pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
            pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucCacheOffset;
            pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = ucSubLPNCount;
            pSubCmd->SubCmdPhyBufferID = usPhyBufID;
            return L1_CACHE_SE_WT_FULL_HIT_RECYCLE;
        }
    }

    // now that we know all requested data is in the buffer,
    // before return as full hit, we have to ensure the buffer
    // is not busy
    if((U8)DM_WRITE == ucSubCmdRw)
    {
        // for write commnads, we have to check for NFC read, host write and host read cache status
        if(L1_HostFullHitWriteCheckBusy(usLogBufID, pSubCmd->pSCMD->tMA.ulSubSysLBA, pSubCmd->pSCMD->tMA.ucSecLen) == BUF_BUSY)
        {
            return L1_CACHE_SE_FULL_HIT_WAIT;
        }
    }
    else if((U8)DM_READ == ucSubCmdRw)
    {
        // for read commands, we have to check for NFC read and host write cache status,
        // please note we don't have to and we can't check for host read cache status for read commands
        // since it may cause deadlocks in sata mode, where a prefetch buffer is locked by a host read
        // cache status and it can never be released due to first data ready FIFO issue(a latter host read
        // commnad has been selected by the SDC)
        if(L1_HostFullHitReadCheckBusy(usLogBufID, pSubCmd->pSCMD->tMA.ulSubSysLBA, pSubCmd->pSCMD->tMA.ucSecLen) == BUF_BUSY)
        {
            return L1_CACHE_SE_FULL_HIT_WAIT;
        }
    }

    // read or write full hit, fill in the subcommand info and
    // return as full hit
    pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucCacheOffset << LPN_SECTOR_BIT) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & LPN_SECTOR_MSK);
    pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucCacheOffset;
    pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = ucSubLPNCount;
    pSubCmd->SubCmdPhyBufferID = usPhyBufID;
#ifdef SIM
    if((U8)DM_WRITE == ucSubCmdRw && gpBufTag[usLogBufID].bPrefetchBuf == TRUE)
    {
        DBG_Printf("we can have a write full hit on a prefetch buffer\n");
        DBG_Getch();
    }
#endif
    return L1_CACHE_SE_FULL_HIT;
}
#endif // end of #if 0

/****************************************************************************
Name        :L1_CacheGetTagStatus
Input       :U16 usCacheLine, U8 ucSubLPNCount
Output      :L1_STATUS_CACHE_NO_BUFF
             L1_STATUS_CACHE_NO_SPACE
             L1_STATUS_CACHE_OK
Author      :HenryLuo
Date        :2012.02.20    18:07:24
Description :check whether the cache node have enough space for the subcmd.
Others      :
Modify      :
****************************************************************************/
U8 L1_CacheGetTagStatus(U8 ucPuNum, U8 ucSubLPNCount)
{
    U16 usPhyBufID;
    U16 usLogicBufID;

    usPhyBufID = g_aCacheLine[ucPuNum];

    if( INVALID_4F == usPhyBufID)
    {
        return L1_STATUS_CACHE_NO_BUFF;
    }
    else    /* must no hit */
    {
        usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

        if((LPN_PER_BUF - L1_BufferGetWritePointer(usLogicBufID)) < ucSubLPNCount)
        {
            return L1_STATUS_CACHE_NO_SPACE;
        }
        else
        {
            return L1_STATUS_CACHE_OK;
        }
    }

}

/****************************************************************************
Name        :L1_CacheAddNewLpn
Input       :SUBCMD* pSubCmd, U32 ulCacheLine
Output      :void
Author      :HenryLuo
Date        :2012.02.20    18:18:56
Description :add the data of the subcmd into the cache node.
Others      :
Modify      : blakezhang  2012.11.22  modify for L1 ramdisk design
****************************************************************************/
void L1_CacheAddNewLpn(SUBCMD* pSubCmd, U16 usCacheLine)
{
    U8  i;
    U8  ucSubLPNOffset;
    U8  ucSubLPNCount;
    U8  ucCachePointer;
    U8  ucCurrLpnBitmap;
    U16 usLogicBufID;
    U32 ulStartLpn;

    ucSubLPNOffset = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    ucSubLPNCount = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
    usLogicBufID   = LGCBUFID_FROM_PHYBUFID(g_aCacheLine[usCacheLine]);
    ucCachePointer = L1_BufferGetWritePointer(usLogicBufID);

    /* update CacheLpn and SectorBitmap */
    ulStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT);
    for(i = 0; i < ucSubLPNCount; i++)
    {
        ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucSubLPNOffset + i];
        L1_SetBufferLPN(usLogicBufID, ucCachePointer, (ulStartLpn + ucSubLPNOffset + i));
        L1_SetBufferSectorBitmap(usLogicBufID, ucCachePointer, ucCurrLpnBitmap);
        ucCachePointer++;
    }

    /* update WritePointer */
    L1_BufferSetWritePointer(usLogicBufID, ucCachePointer);

    /* Update CacheID Link */
    L1_UpdateCacheIdLink(pSubCmd);

    return;
}

/****************************************************************************
Name        : L1_CacheLinkRecycleBuf()
Input       : 
Output      :
Author      : BlakeZhang 
Date        :  
Description : Update CacheLink when recycle write buffer,
              delet cache nodes in this buffer
Others      :
Modify      :
****************************************************************************/
void L1_CacheLinkRecycleBuf(U16 usLogBufID)
{
    U8  ucCacheOffset;
    U16 usCurrCacheID;
    U16 usCacheID;
    U32 ulLCTIndex;

    if (gpBufTag[usLogBufID].bSeqBuf == TRUE)
    {
        /*data in this buffer is sequential write,just remove all cache nodes of its LCT*/
        if (gpBufTag[usLogBufID].ucWritePointer != LPN_PER_BUF)
        {
            DBG_Printf("L1_ResetCacheIdLink SeqW usLogBufID 0x%x ERROR!\n", usLogBufID);
            DBG_Getch();
        }

        /*get the first LPN in this buffer,calculate its LCT index*/
        ulLCTIndex = LCTINDEX_FROM_LBA(((L1_GetBufferLPN(usLogBufID, 0)) << SEC_PER_LPN_BITS));

#ifdef CACHE_LINK_MULTI_LCT
        usCacheID = L1_CalcCacheID(usLogBufID, 0);

        usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, usCacheID);

        if (usCurrCacheID == INVALID_4F)
        {
            L1_SetLCTStartCache(ulLCTIndex, L1_GetNextCacheId(usCacheID + LPN_PER_BUF - 1));
        }
        else
        {
            L1_SetNextCacheId(usCurrCacheID, L1_GetNextCacheId(usCacheID + LPN_PER_BUF - 1));
        }
#else
        /*reset the start cache of this LCT to INVALID_4F*/
        L1_SetLCTStartCache(ulLCTIndex, INVALID_4F);
#endif

        /*calculate the first cache id of this buffer*/
        usCacheID = L1_CalcCacheID(usLogBufID, 0);

        for (ucCacheOffset = 0; ucCacheOffset < LPN_PER_BUF; ucCacheOffset++)
        {
            /*set the next cache id of all caches in this buffer to INVALID_4F*/
            L1_SetNextCacheId(usCacheID + ucCacheOffset, INVALID_4F);
        }
    }
    else
    {
        U32 ulCurrentLpn;
        LCT_HIT_RESULT LctLinkSearchResult;
    
        for (ucCacheOffset = 0; ucCacheOffset < gpBufTag[usLogBufID].ucWritePointer; ucCacheOffset++)
        {
            LctLinkSearchResult.AsU32 = 0;

            ulCurrentLpn = L1_GetBufferLPN(usLogBufID, ucCacheOffset);
            if(INVALID_8F == ulCurrentLpn)
            {
                continue;
            }

            // we have to ensure that the current buffer entry contains the latest copy of the LPN before removing
            // its corresponding cache id from the lct link

            LctLinkSearchResult.AsU32 = L1_CacheIDLinkSearchLPN(ulCurrentLpn, 1, TRUE);

            // remove the cache id of the current buffer entry from the lct link only if the current buffer entry contains
            // the latest entry of the LPN
            if((LctLinkSearchResult.ucHitResult == L1_CACHE_SE_FULL_HIT) && (LctLinkSearchResult.usLogBufID == usLogBufID) && (LctLinkSearchResult.ucCacheOffset == ucCacheOffset))
            {
                ulLCTIndex = LCTINDEX_FROM_LBA(ulCurrentLpn << SEC_PER_LPN_BITS);

                usCacheID = L1_CalcCacheID(usLogBufID, ucCacheOffset);

                usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, usCacheID);
                if(usCurrCacheID == INVALID_4F)
                {
                    L1_SetLCTStartCache(ulLCTIndex, L1_GetNextCacheId(usCacheID));
                }
                else
                {
                    L1_SetNextCacheId(usCurrCacheID, L1_GetNextCacheId(usCacheID));
                }

                /*reset the next cache of this cache to INVALID_4F*/
                L1_SetNextCacheId(usCacheID, INVALID_4F);
            }
        }
    }

    return;
}


/****************************************************************************
Name        :L1_ReleaseCacheLine
Input       :ucPUNum ulCacheLine
Output      :void
Author      :Blakezhang
Date        :2012.11.22
Description :relese a cacheline and the buffer
Others      :
Modify      :
****************************************************************************/
void L1_ReleaseCacheLine(U8 ucPuNum)
{
    // Sean Gao 20150402
    // this function release the active random write buffer
    // of the input PU

#ifdef SIM
    if(ucPuNum >= SUBSYSTEM_SUPERPU_NUM)
    {
        DBG_Printf("pu error\n");
        DBG_Getch();
    }

    if(INVALID_4F == g_aCacheLine[ucPuNum])
    {
        DBG_Printf("no active cache in current pu");
        DBG_Getch();
    }
#endif

    if(INVALID_4F != g_aCacheLine[ucPuNum])
    {
        L1_ReleaseWriteBuf(ucPuNum, g_aCacheLine[ucPuNum]);

        g_aCacheLine[ucPuNum] = INVALID_4F;
        COM_BitMaskClear(&g_ulActiveWriteBufferBitmap, ucPuNum);
    }

    return;
}

/****************************************************************************
Name        :L1_CacheAttachBuffer
Input       :U16 usCacheLine, U16 usBuffID
Output      :void
Author      :HenryLuo
Date        :2012.02.20    18:20:06
Description :allocate a buffer for cache node.
Others      :
Modify      :
****************************************************************************/
void L1_CacheAttachBuffer(U8 ucPuNum, U16 usPhyBuffID)
{
    g_aCacheLine[ucPuNum] = usPhyBuffID;
    COM_BitMaskSet(&g_ulActiveWriteBufferBitmap, ucPuNum);

    return;
}

U32 L1_ReleaseAllWriteCache(void)
{
    // Sean Gao 20150820
    // this function releases all active write caches simultaneously

    U8 i;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if(INVALID_4F != g_aCacheLine[i])
        {
            L1_ReleaseCacheLine(i);
        }
    }

    return TRUE;
}

void L1_CacheInvalidLPNInLCT(U16 usLogBufID, U8 ucLPNOffset, U8 ucSubLPNCount)
{
    // Sean Gao 20150518
    // this function removes all LCT entries of all specified LPNs in a buffer,
    // please note that this function assumes the
    // 1. all LPNs to be removed belongs to the same LCT index
    // 2. LCT entries to be removed are sequential in the LCT link

    U8  ucLoop;
    U32 ulLctIndex;
    U32 usCurCacheId;
    U32 usPrevCacheId;

#ifdef SIM
    // double check the buffer LPN
    {
        U32 ulTargetLct;
        U8 i;

        ulTargetLct = LCTINDEX_FROM_LBA(((L1_GetBufferLPN(usLogBufID, ucLPNOffset)) << SEC_PER_LPN_BITS));

        for(i = 0; i < ucSubLPNCount; i++)
        {
            if(LCTINDEX_FROM_LBA(((L1_GetBufferLPN(usLogBufID, ucLPNOffset+i)) << SEC_PER_LPN_BITS)) != ulTargetLct)
            {
                DBG_Printf("L1_CacheInvalidLPNInLCT error, LPNs belong to different LCTs\n");
                DBG_Getch();
            }
        }
    }
#endif

    // calculate the LCT of the buffer entries to be invalidated
    ulLctIndex = LCTINDEX_FROM_LBA(((L1_GetBufferLPN(usLogBufID, ucLPNOffset)) << SEC_PER_LPN_BITS));

    // calculate the cache id of the first LPN to be removed
    usCurCacheId = L1_CalcCacheID(usLogBufID, ucLPNOffset);

    // process the LCT link
    if(L1_GetLCTStartCache(ulLctIndex) == usCurCacheId)
    {
        // the cache id of the first LPN is the start cache id of the LCT link,
        // set the start id of this LCT to the next cache id
        // of the last LPN to be removed
        L1_SetLCTStartCache(ulLctIndex, L1_GetNextCacheId(usCurCacheId + ucSubLPNCount - 1));
    }
    else
    {
        // the cache id of the first LPN is not the start cache id, find the cache id
        // previous to the cache id of the first LPN
        usPrevCacheId = L1_CacheLinkFindCurrNode(ulLctIndex, usCurCacheId);
        L1_SetNextCacheId(usPrevCacheId, L1_GetNextCacheId(usCurCacheId + ucSubLPNCount - 1));
    }

    // process all LPNs
    for(ucLoop = 0; ucLoop < ucSubLPNCount; ucLoop++)
    {
        // set the next cache id of all cache ids to INVALID_4F, removing
        // them from the LCT link
        L1_SetNextCacheId((usCurCacheId + ucLoop), INVALID_4F);
    }

    // since one or more LPNs are invalided in LCT, the buffer content can't
    // be sequential
    gpBufTag[usLogBufID].bSeqBuf = FALSE;

    return;
}

BOOL L1_CacheInvalidTrimLCT(U32 trimStartLba, U32 trimEndLba)
{
    // we have to be careful of trimEndLba
    // (trimEndLba-1) is the last sector we trim in
    // this trim command, not trimEndLba
    // sectors we want to trim are [trimStartLba, trimEndLba)

    U8  ucLoop;
    U8  ucLPNOffset;
    U8  ucSubLPNCount;
    U16 usLogBufID;
    U16 usBufLoop;

    U32 ulAlignedStartLPN;
    U32 ulAlignedEndLPN;

    U32 ulFirstLPN;
    U32 ulLastLPN;
    U32 ulCurrLPN;

    U32 ulLCTIndex;
    U32 usNextCacheID;
    U32 usCurrCacheID;

    // calculate start LPN
    if ( (trimStartLba & SEC_PER_LPN_MSK) != 0 )
    {
        DBG_Printf("trimStartLba is not 4K aligned\n");
        DBG_Getch();
    }
    else
    {
        ulAlignedStartLPN = trimStartLba >> SEC_PER_LPN_BITS;
    }

    // calculate end LPN
    if ( (trimEndLba & SEC_PER_LPN_MSK) != 0 )
    {
        DBG_Printf("trimEndLba is not 4K aligned\n");
        DBG_Getch();
    }
    else
    {
        ulAlignedEndLPN = (trimEndLba >> SEC_PER_LPN_BITS) - 1;
    }

    // range check
    if (ulAlignedEndLPN < ulAlignedStartLPN)
    {
        return TRUE;
    }

    // trim todo: just a thought here, instead of scanning all write buffer entries,
    // how about doing it from LCT perspective?

    // LPNs we want to invalidate in LCT are [ulAlignedStartLPN, ulAlignedEndLPN]
    // for all non-free write buffers, search if it contains LPNs in range [ulAlignedStartLPN, ulAlignedEndLPN]
    for(usBufLoop = 0; usBufLoop < L1_BUFFER_COUNT_WRITE; usBufLoop++)
    {
        usLogBufID = LGCBUFID_FROM_PHYBUFID(g_ulWriteBufStartPhyId) + usBufLoop;
      
        if(BUF_STAGE_FREE != gpBufTag[usLogBufID].Stage)
        {
            // processing the sequential write buffer
            if (TRUE == gpBufTag[usLogBufID].bSeqBuf)
            {
                ulFirstLPN = L1_GetBufferLPN(usLogBufID, 0);
                ulLastLPN  = ulFirstLPN + LPN_PER_BUF - 1;

                if ((ulAlignedEndLPN < ulFirstLPN) || (ulLastLPN < ulAlignedStartLPN))
                {
                    //sequential buffer is not within the trim range, proceed next buffer
                    continue;
                }
                else
                {
                    // get the first and last LPN of the sequential write buffer
                    if (ulAlignedStartLPN > ulFirstLPN)
                    {
                        ucLPNOffset = ulAlignedStartLPN - ulFirstLPN;
                        ulFirstLPN  = ulAlignedStartLPN;
                    }
                    else
                    {
                        ucLPNOffset = 0;
                    }

                    if (ulAlignedEndLPN < ulLastLPN)
                    {
                        ulLastLPN = ulAlignedEndLPN;
                    }

                    ucSubLPNCount = ulLastLPN - ulFirstLPN + 1;
                }

                ulLCTIndex    = LCTINDEX_FROM_LBA(ulFirstLPN << SEC_PER_LPN_BITS);
                usNextCacheID = L1_CalcCacheID(usLogBufID, ucLPNOffset);

                // process the LCT chain
                if(usNextCacheID == L1_GetLCTStartCache(ulLCTIndex))
                {
                    // the first LPN is the start id of the LCT,
                    // set the start id of this LCT to the next cache id
                    // of the last LPN to be removed
                    L1_SetLCTStartCache(ulLCTIndex, L1_GetNextCacheId(usNextCacheID + ucSubLPNCount - 1));
                }
                else
                {
                    // the first LPN is not the start id, find the cache id
                    // previous to the first LPN
                    usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, usNextCacheID);
                    L1_SetNextCacheId(usCurrCacheID, L1_GetNextCacheId(usNextCacheID + ucSubLPNCount - 1));
                }

                for(ucLoop = 0; ucLoop < ucSubLPNCount; ucLoop++, usNextCacheID++, ucLPNOffset++)
                {
                    L1_SetNextCacheId(usNextCacheID, INVALID_4F);
                }

                gpBufTag[usLogBufID].bSeqBuf = FALSE;
            }
            else // random buffer, we have to check LPNs in the buffer one by one
            {
                // check all LPNs in the random buffer one by one
                for (ucLPNOffset = 0; ucLPNOffset < gpBufTag[usLogBufID].ucWritePointer; ucLPNOffset++)
                {
                    LCT_HIT_RESULT LctLinkSearchResult;
                    LctLinkSearchResult.AsU32 = 0;

                    // get the LPN of current buffer entry
                    ulCurrLPN = L1_GetBufferLPN(usLogBufID, ucLPNOffset);

                    // continue if this LPN is not within the trim range
                    if ((INVALID_8F == ulCurrLPN) || (ulCurrLPN < ulAlignedStartLPN) || (ulCurrLPN > ulAlignedEndLPN))
                    {
                        continue;
                    }


                    // we have to ensure that the current buffer entry contains the latest copy of the LPN before removing
                    // its corresponding cache id from the lct link

                    LctLinkSearchResult.AsU32 = L1_CacheIDLinkSearchLPN(ulCurrLPN, 1, FALSE);

                    // remove the cache id of the current buffer entry from the lct link only if the current buffer entry
                    // contains the latest entry of the LPN
                    if((LctLinkSearchResult.ucHitResult == L1_CACHE_SE_FULL_HIT) && (LctLinkSearchResult.usLogBufID == usLogBufID) && (LctLinkSearchResult.ucCacheOffset == ucLPNOffset))
                    {

                        ulLCTIndex    = LCTINDEX_FROM_LBA(ulCurrLPN << SEC_PER_LPN_BITS);
                        usNextCacheID = L1_CalcCacheID(usLogBufID, ucLPNOffset);

                        if (usNextCacheID == L1_GetLCTStartCache(ulLCTIndex))
                        {
                            // set the start id of this LCT to the next cache id of the LPN to be removed
                            L1_SetLCTStartCache(ulLCTIndex, L1_GetNextCacheId(usNextCacheID));
                        }
                        else
                        {
                            // find the cache id previous to the first LPN
                            usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, usNextCacheID);
                            L1_SetNextCacheId(usCurrCacheID, L1_GetNextCacheId(usNextCacheID));
                        }

                        L1_SetNextCacheId(usNextCacheID, INVALID_4F);
                    }
                } 
            } // random buffer processing
        } // if( gBuffTag[writeBufLogicId].Stage != BUF_STAGE_FREE )
    } // for ( i = 0; i < L1_BUFFER_COUNT_WRITE; ++i)

    return TRUE;
}

void L1_CacheTrimAddNewLPN(U8 ucPuNum, U32 ulLPN, U8 ucLpnBitmap)
{
    U8  ucLPNOffset;
    U16 usLogBufID;
    U16 usCurrCacheID;
    U16 usNextCacheID;
    U32 ulBufAddr;
    U32 ulLCTIndex;
    
    usLogBufID  = LGCBUFID_FROM_PHYBUFID(g_aCacheLine[ucPuNum]);
    ucLPNOffset = L1_BufferGetWritePointer(usLogBufID);

    // for each sector to be trim in this LPN, zero the data of each sector in the buffer
    ulBufAddr = COM_GetMemAddrByBufferID(g_aCacheLine[ucPuNum], TRUE, BUF_SIZE_BITS);
    ulBufAddr += (ucLPNOffset  << LPN_SIZE_BITS);

    COM_MemClearSectorData(ulBufAddr, ucLpnBitmap);

    L1_SetBufferLPN(usLogBufID, ucLPNOffset, ulLPN);
    L1_SetBufferSectorBitmap(usLogBufID, ucLPNOffset, ucLpnBitmap);

    /* update WritePointer */
    L1_BufferSetWritePointer(usLogBufID, (ucLPNOffset + 1));

    /* Update CacheID Link */
    ulLCTIndex = ulLPN >> LPN_PER_BUF_BITS;

    // the last cache id in the link
    usCurrCacheID = L1_CacheLinkFindCurrNode(ulLCTIndex, INVALID_4F);

    // the cache id of the newly added lpn
    usNextCacheID = L1_CalcCacheID(usLogBufID, ucLPNOffset);
    
    if (usCurrCacheID == INVALID_4F)
    {
        //set start CacheID
        L1_SetLCTStartCache(ulLCTIndex, usNextCacheID);
    }
    else
    {
        //set 1st NextCacheID
        L1_SetNextCacheId(usCurrCacheID, usNextCacheID);
    }

    //set last CacheID to INVALID_4F
    L1_SetNextCacheId(usNextCacheID, INVALID_4F);

    return;
}

extern BOOL L2_TrimLPN(U32 LPN);
#ifndef LCT_TRIM_REMOVED
void L1_ProcessPendingTrim(void)
{
    // Sean Gao 20150831
    // this function processes a tiny bit of trim at a time,
    // it must return very quickly so that the performance impact is
    // minimized

    // check if a trim target LCT is present
    if((g_ulCurTrimTargetLct == INVALID_8F) && (g_ulTrimSearchRangeStartLct < g_ulTrimSearchRangeEndLct))
    {
        // there isn't a trim target LCT, we have to search for one

        U32 ulCurrentTrimLctGroup;
        U32 ulLctGroupTrimBitmap;
        U32 ulOffsetWithinLctGroup;

#ifdef SIM
        // check g_ulCurTrimTargetLctLpnOffset, it should be INVALID_8F since
        // g_ulCurTrimTargetLct is INVALID_8F
        if(g_ulCurTrimTargetLctLpnOffset != INVALID_8F)
        {
            DBG_Printf("g_ulCurTrimTargetLctLpnOffset error\n");
            DBG_Getch();
        }
#endif

        // calculate the LCT group g_ulTrimSearchRangeStartLct belongs to
        ulCurrentTrimLctGroup = LCTTAG_FROM_INDEX(g_ulTrimSearchRangeStartLct);

        // calculate the offset of the LCT within the LCT group
        ulOffsetWithinLctGroup = LCTMSK_FROM_INDEX(g_ulTrimSearchRangeStartLct);

        // get the trim bitmap for the current LCT group
        ulLctGroupTrimBitmap = g_pulTrim[ulCurrentTrimLctGroup];

        // check if the current LCT group has any pending trim operations
        if(ulLctGroupTrimBitmap != 0)
        {
            // the current LCT group has pending trim operations

            U8 i;

            // check all LCTs in the current LCT group one by one
            for(i = ulOffsetWithinLctGroup; i < DWORD_BIT_SIZE; ++i)
            {
                if((ulLctGroupTrimBitmap & (1 << i)) != 0)
                {
                    // trim target lct found

                    // set the current trim target LCT and LPN offset
                    g_ulCurTrimTargetLct = g_ulTrimSearchRangeStartLct;
                    g_ulCurTrimTargetLctLpnOffset = 0;

                    // advance g_ulTrimSearchRangeStartLct
                    g_ulTrimSearchRangeStartLct++;

                    // break the loop since a target LCT has been found
                    break;
                }
                else
                {
                    // advance g_ulTrimSearchRangeStartLct
                    g_ulTrimSearchRangeStartLct++;
                }
            } // for(i = ulOffsetWithinLctGroup; i < DWORD_BIT_SIZE; ++i)
        } // if(ulLctGroupTrimBitmap != 0)
        else
        {
            // the current LCT group doesn't have any pending trim operations

            U32 i;

#define MAX_SEARCH_LCT_GROUP_COUNT (5)

            // advance the LCT group since the current one doesn't have any pending trim
            // operations
            ulCurrentTrimLctGroup++;

            // search until a LCT group with pending trim operations is found or
            // the search count has reached the maximum
            for(i = 0; i < MAX_SEARCH_LCT_GROUP_COUNT; ++i)
            {
                if(g_pulTrim[ulCurrentTrimLctGroup] == 0)
                {
                    ulCurrentTrimLctGroup++;
                }
                else
                {
                    // a LCT group with pending trim operations is found,
                    // break the loop
                    break;
                }
            }

            // update g_ulTrimSearchRangeStartLct
            g_ulTrimSearchRangeStartLct = ulCurrentTrimLctGroup << DWORD_BIT_SIZE_BITS;
        }

        // check if the entire LCT range with pending trim operations has been scanned,
        // reset if it is
        if(g_ulTrimSearchRangeStartLct >= g_ulTrimSearchRangeEndLct)
        {
            g_ulTrimSearchRangeStartLct = LCT_ENTRY_DEPTH;
            g_ulTrimSearchRangeEndLct = 0;

#ifdef SIM
            // check if g_ulPendingTrimLctCount is maintained correctly
            if(g_ulPendingTrimLctCount != 0 && g_ulPendingTrimLctCount != 1)
            {
                DBG_Printf("g_ulPendingTrimLctCount error %d\n", g_ulPendingTrimLctCount);
                DBG_Getch();
            }
#endif
        }

    } // if((g_ulCurTrimTargetLct == INVALID_8F) && (g_ulTrimSearchRangeStartLct < g_ulTrimSearchRangeEndLct))

    // check if the current trim target LCT is present after the search
    if(g_ulCurTrimTargetLct != INVALID_8F)
    {
        U8 i;
        U32 ulCurTrimLctStartLpn;
        U8 ucTrimmedLpnCount;

        // calculate the start LPN of the current trim target LCT
        ulCurTrimLctStartLpn = LPN_FROM_LCTINDEX(g_ulCurTrimTargetLct);

#define MAX_TRIM_LPN_COUNT (1)

        // perform trim operation until the maximum trim LPN count is reached or the entire target LCT is
        // trimmed
        for(i = g_ulCurTrimTargetLctLpnOffset, ucTrimmedLpnCount = 0; (i < LPN_PER_BUF) && (ucTrimmedLpnCount < MAX_TRIM_LPN_COUNT); ++i, ++ucTrimmedLpnCount)
        {
            if( TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing )
            {
                L2_TrimLPN(ulCurTrimLctStartLpn + i);
            }
        }

        // check if the entire target LCT is trimmed
        if(i == LPN_PER_BUF)
        {
            // all LPNs in the current trim target are processed

            // update the trim bitmap
            g_pulTrim[LCTTAG_FROM_INDEX(g_ulCurTrimTargetLct)] &= (~(1 << LCTMSK_FROM_INDEX(g_ulCurTrimTargetLct)));

            // reset current trim target LCT and LPN offset
            g_ulCurTrimTargetLct = INVALID_8F;
            g_ulCurTrimTargetLctLpnOffset = INVALID_8F;

            // update the number of pending trim LCT count
            g_ulPendingTrimLctCount--;
        }
        else
        {
            // the current target LCT hasn't been trimmed completely

            // update the LPN offset
            g_ulCurTrimTargetLctLpnOffset = i;
        }
    } // if(g_ulCurTrimTargetLct != INVALID_8F)

    return;
}

void L1_ProcessPendingTrimBulk(U32 ulMaxTrimLctCount, U32 ulMaxSearchLctCount)
{
    // Sean Gao 20150910
    // this function finds a continuous trim LCT range and process it

    U32 ulSearchLctCount;  
    U32 ulTrimRangeStartLct;
    U32 ulTrimRangeLctCount;
    U32 ulTotalTrimmedLctCount;
    U32 ulCurrentTrimLctGroup;
    U32 ulLctGroupTrimBitmap;
    U32 ulOffsetWithinLctGroup;
    U8 ucIsTrimBitmapModified;

    // process the current pending trim LCT if there is one present
    if(g_ulCurTrimTargetLct != INVALID_8F)
    {
        U8 i;
        U32 ulCurTrimLctStartLpn;

        // calculate the start LPN for the current trim target LCT
        ulCurTrimLctStartLpn = LPN_FROM_LCTINDEX(g_ulCurTrimTargetLct);

        // perform trim operations on all LPNs
        for(i = g_ulCurTrimTargetLctLpnOffset; i < LPN_PER_BUF; ++i)
        {
            if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
            {
                L2_TrimLPN(ulCurTrimLctStartLpn + i);
            }
        }
        
        // update the trim bitmap
        g_pulTrim[LCTTAG_FROM_INDEX(g_ulCurTrimTargetLct)] &=(~(1 << LCTMSK_FROM_INDEX(g_ulCurTrimTargetLct)));

        // reset current trim target LCT and LPN offset
        g_ulCurTrimTargetLct = INVALID_8F;
        g_ulCurTrimTargetLctLpnOffset = INVALID_8F;

        // update the number of pending trim LCT count
        g_ulPendingTrimLctCount--;
    } // if(g_ulCurTrimTargetLct != INVALID_8F)

    // initialize the current LCT group number
    ulCurrentTrimLctGroup = INVALID_8F;

    // initialize the start LCT of the trim range
    ulTrimRangeStartLct = INVALID_8F;

    // initialize the trim range LCT count
    ulTrimRangeLctCount = 0;

    // initialize the total trim LCT count
    ulTotalTrimmedLctCount = 0;

    // initialize ucIsTrimBitmapModified, indicating if the trim bitmap of the current LCT group is modified
    ucIsTrimBitmapModified = FALSE;

    for(ulSearchLctCount = 0; (g_ulTrimSearchRangeStartLct < g_ulTrimSearchRangeEndLct) && (ulSearchLctCount < ulMaxSearchLctCount) && (ulTotalTrimmedLctCount < ulMaxTrimLctCount); ulSearchLctCount++)
    {
        // update the information of the current LCT group if we have to
        if(LCTTAG_FROM_INDEX(g_ulTrimSearchRangeStartLct) != ulCurrentTrimLctGroup)
        {
            // if the trim bitmap of the current LCT group is modified,
            // we have to write it back to the LCT trim table
            if(ucIsTrimBitmapModified == TRUE)
            {
                g_pulTrim[ulCurrentTrimLctGroup] = ulLctGroupTrimBitmap;
                ucIsTrimBitmapModified = FALSE;
            }

            // update the LCT group number 
            ulCurrentTrimLctGroup = LCTTAG_FROM_INDEX(g_ulTrimSearchRangeStartLct);

            // update the trim bitmap of the current LCT group
            ulLctGroupTrimBitmap = g_pulTrim[ulCurrentTrimLctGroup];
        }

        // get the offset within the current LCT group
        ulOffsetWithinLctGroup = LCTMSK_FROM_INDEX(g_ulTrimSearchRangeStartLct);

        // check if g_ulTrimSearchRangeStartLct needs to be trimmed
        if(0 != (ulLctGroupTrimBitmap & (1 << ulOffsetWithinLctGroup)))
        {
            // the current LCT needs to be trimmed

            // check if the current trim range start LCT is set
            if(ulTrimRangeStartLct == INVALID_8F)
            {
                // set the start LCT for the current trim range
                ulTrimRangeStartLct = g_ulTrimSearchRangeStartLct;
            }
            
            // clear the trim bit of the current LCT
            ulLctGroupTrimBitmap &= (~(1 << ulOffsetWithinLctGroup));

            // mark the current trim bitmap as modified
            ucIsTrimBitmapModified = TRUE;

            // increase the LCT count of the current trim range
            ulTrimRangeLctCount++;

            // increase the total trimmed LCT count
            ulTotalTrimmedLctCount++;

            // advance g_ulTrimSearchRangeStartLct
            g_ulTrimSearchRangeStartLct++;
        }
        else
        {
            // the current LCT doesn't need to be trimmed

            // advance g_ulTrimSearchRangeStartLct
            g_ulTrimSearchRangeStartLct++;

            // process the current trim range
            if(ulTrimRangeStartLct != INVALID_8F)
            {
                // trim the LCTs in the range
                if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
                {
                    L2_TrimReqProcess(LPN_FROM_LCTINDEX(ulTrimRangeStartLct),(ulTrimRangeLctCount * LPN_PER_BUF));
                }

                // update the pending trim LCT count
                g_ulPendingTrimLctCount -= ulTrimRangeLctCount;

                // reset the current trim range start LCT and LCT count
                ulTrimRangeStartLct = INVALID_8F;
                ulTrimRangeLctCount = 0;
            }
        }
    } // for(ulSearchLctCount = 0; (g_ulTrimSearchRangeStartLct < g_ulTrimSearchRangeEndLct) && (ulSearchLctCount < ulMaxSearchLctCount) && (ulTotalTrimmedLctCount < ulMaxTrimLctCount); ulSearchLctCount++)

    // process the remaining trim range
    if(ulTrimRangeStartLct != INVALID_8F)
    {
        // trim the LCTs in the range
        if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
        {
            L2_TrimReqProcess(LPN_FROM_LCTINDEX(ulTrimRangeStartLct),(ulTrimRangeLctCount * LPN_PER_BUF));
        }

        // update the pending trim LCT count
        g_ulPendingTrimLctCount -= ulTrimRangeLctCount;
    }

    // if the trim bitmap of the current LCT group is modified,
    // we have to write it back to the LCT trim table
    if(ucIsTrimBitmapModified == TRUE)
    {
        g_pulTrim[ulCurrentTrimLctGroup] = ulLctGroupTrimBitmap;
        ucIsTrimBitmapModified = FALSE;
    }

    // reset the trim search range if we have to
    if(g_ulTrimSearchRangeStartLct >= g_ulTrimSearchRangeEndLct)
    {
        g_ulTrimSearchRangeStartLct = LCT_ENTRY_DEPTH;
        g_ulTrimSearchRangeEndLct = 0;

#ifdef SIM
        // check if g_ulPendingTrimLctCount is maintained correctly
        if(g_ulPendingTrimLctCount != 0 && g_ulPendingTrimLctCount != 1)
        {
            DBG_Printf("g_ulPendingTrimLctCount error %d\n", g_ulPendingTrimLctCount);
            DBG_Getch();
        }
#endif
    }

    return;
}
#endif

U32 L1_CacheGetTime(void)
{
#ifdef SIM
    return (U32)GetTickCount();
#else
    return HAL_GetMCUCycleCount();
#endif
}

U32 L1_CacheCheckTimePass(U8 ucPuNum)
{
    U32 ulCurrTime;
    U32 ulTimePass;
    U32 ulWriteTime;

    ulWriteTime = g_ulCacheWriteTime[ucPuNum];
    ulCurrTime  = L1_CacheGetTime();

    if (ulCurrTime < ulWriteTime)
    {
        ulTimePass = (INVALID_8F - ulWriteTime) + ulCurrTime + 1;
    }
    else
    {
        ulTimePass = ulCurrTime - ulWriteTime;
    }

    return ulTimePass;
}

/********************** FILE END ***************/

