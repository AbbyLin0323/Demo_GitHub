/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_Cache.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    17:56:30
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"


/* Blakezhang: This is the StripeHead for each Stripe */
U16 gCacheTag[CACHE_LINE_NUM];

LOCAL U16 l_usCurrCacheLine;

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
LOCAL void L1_CacheDramMap(U32 *pFreeDramBase)
{
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
void L1_CacheInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    U16 i;

    L1_CacheDramMap(pFreeDramBase);
    L1_CacheOTFBMap(pFreeOTFBBase);

    for(i = 0; i < CACHE_LINE_NUM; i++)
    {
        gCacheTag[i] = INVALID_4F;
    }

    l_usCurrCacheLine = 0;
    
      return;
}



/****************************************************************************
Name        :L1_GetCacheLine
Input       :LbaAddr
Output      :CacheLine
Author      :HenryLuo
Date        :2012.02.20    17:58:54
Description :calculate the cacheline of subcmd. 
Others      :
Modify      :
****************************************************************************/
U16 L1_GetCacheLine(U32 LbaAddr)
{
    U32 ulLPN;
    U16 CacheLine;
    
#ifdef FAKE_L2
    ulLPN = LbaAddr >> SEC_PER_BUF_BITS;
    CacheLine = (ulLPN)%CACHE_LINE_NUM;
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "\n Get CacheLine = %d \n", CacheLine);
#else
    ulLPN = LbaAddr >> SEC_PER_LPN_BITS;

#ifdef RAMDISK /* L2_FORCE_VIRTUAL_STORAGE */
    CacheLine = ulLPN % CACHE_LINE_NUM;
#else
    CacheLine = (U16) L2_GetStripeIDFromLPN(ulLPN);
#endif

    CacheLine &= 0xff;
#endif

    return CacheLine;
}

U32 L1_CalcSubLBAForL1(U32 phyLBA)
{
#ifdef LPN_TO_CE_REMAP
    U8 targetPU;
    U32 pageNum;
    pageNum = phyLBA>>SEC_PER_BUF_BITS;
#if 1
    targetPU =( ( pageNum )^
                ( pageNum/CE_NUM )^ 
                ( pageNum/CE_NUM/CE_NUM )^
                ( pageNum/CE_NUM/CE_NUM/CE_NUM )^
                ( pageNum/CE_NUM/CE_NUM/CE_NUM/CE_NUM ) )%CE_NUM;
#else
    targetPU =( ( (pageNum)&(CE_NUM-1) )+
                ( (pageNum/CE_NUM)&(CE_NUM-1) )+ 
                ( (pageNum/CE_NUM/CE_NUM)&(CE_NUM-1) )+
                ( (pageNum/CE_NUM/CE_NUM/CE_NUM)&(CE_NUM-1) )+
                ( (pageNum/CE_NUM/CE_NUM/CE_NUM/CE_NUM)&(CE_NUM-1) ) )%CE_NUM;
#endif
    pageNum = ( pageNum & ~(CE_NUM-1) ) + targetPU;
    return (pageNum<<SEC_PER_BUF_BITS);
#else
    return HCMDLBA_SUBCMDLBA(phyLBA);
#endif
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
BOOL L1_SubCmdGetLpnSectorMap(U8 ucSubCmdOffset, U8 ucSubCmdlength, U8 * pLpnSectorBitmap)
{
    return TRUE;
}

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
    return TRUE;
}

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
U8 L1_CacheGetTagStatus(U16 usCacheLine, U8 ucSubLPNCount)
{
    return 0;
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
void L1_ReleaseCacheLine(U16 usCacheLine)
{

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
void L1_CacheAttachBuffer(U16 usCacheLine, U16 usPhyBuffID)
{
    gCacheTag[usCacheLine] = usPhyBuffID;

    return;
}

//adding for power management 

/****************************************************************************
Name        :L1_CacheNeedIdleFlush
Input       :
Output      :
Author      :Peter Xiu
Date        :2012.10.17   18:24:31
Description :check if cache need idle flush
Others      :
Modify      :
****************************************************************************/
U32 L1_CacheNeedIdleFlush(void)
{
    return FALSE;
}


/****************************************************************************
Name        :L1_CacheIdleFlush
Input       :
Output      :
Author      :Peter Xiu
Date        :2012.10.17   18:24:31
Description :flush one cache node in idle
Others      :
Modify      :
****************************************************************************/
U32 L1_CacheIdleFlush(void)
{
    return TRUE;
}


/****************************************************************************
Name        :L1_CacheFlushAll
Input       :
Output      :
Author      :Peter Xiu
Date        :2012.10.17   18:24:31
Description :flush all write cache
Others      :
Modify      :
****************************************************************************/
U32 L1_CacheFlushAll(void)
{
    return TRUE;
}
/********************** FILE END ***************/

