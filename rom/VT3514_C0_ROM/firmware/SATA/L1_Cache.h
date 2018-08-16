/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    18:28:48
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/
#ifndef _L1_CACHE_H
#define _L1_CACHE_H

//TRACE DEFINE START
#define L1_SUBMODULE0_ID        0
#define L1_SUBMODULE0_NAME      "Cache"
#define L1_SUBMODULE0_ENABLE    1
#define L1_SUBMODULE0_LEVEL     LOG_LVL_TRACE
//TRACE DEFINE END

//TAG = StripID
//extern funciton
//U32 L1CacheCovertLbaToTag(U32 dwLBA);
#ifndef L2_FORCE_VIRTUAL_STORAGE
#define CACHE_LINE_NUM      (PU_NUM * 5)  // 4CE --> 20 Stripes; 16CE --> 80 Stripes;  32CE --> 160 Stripes;
#else
#define CACHE_LINE_NUM      16
#endif

#define CACHE_LINE_SIZE    LPN_PER_BUF   // 4 LPN in 16K buffer, 8 LPN per CACHE LINE in 32K buffer

/*
StartBit and EndBit are from [0, 7], 0 is the least significant sector of a LPN, 
7 is the most significant sector of a LPN. for example:

ucSubCmdOffset = 3, ucSubCmdlength = 3 that means Bitmap 0b(00111000)
the sector from least to most significant are:  0-0-0-1-1-1-0-0
                                                              |                     |
                                                              |                     |
                                                             bit0                 bit7
StartBit = 3  ====> ucSubCmdOffset
EndBit   = 5  ====> ucSubCmdOffset + ucSubCmdlength -1
*/
#define SET_LPN_SECTOR_BIT_MAP(StartBit, EndBit)\
              ((0xFF >> (7 - (EndBit - StartBit))) << StartBit)

#define L1_CACHE_SE_NO_HIT          0   //no hit
#define L1_CACHE_SE_PART_HIT        1   //partial hit
#define L1_CACHE_SE_FULL_HIT        2   //full hit
#define L1_CACHE_SE_FULL_HIT_WAIT   3   //full hit but flushing


#define L1_STATUS_CACHE_OK        0
#define L1_STATUS_CACHE_NO_SPACE  1
#define L1_STATUS_CACHE_NO_BUFF   2
#define L1_STATUS_CACHE_OTHER     3

extern U16 gCacheTag[CACHE_LINE_NUM];

extern void L1_CacheInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
extern U8 L1_GetPUFromLba(U32 LbaAddr);
extern BOOL L1_SubCmdGetLpnSectorMap(U8 ucSubCmdOffset, U8 ucSubCmdlength, U8 * pLpnSectorBitmap);
extern U8 L1_CacheSearchTag(SUBCMD *pSubCmd);
extern U8 L1_CacheGetTagStatus(U16 usCacheLine, U8 ucSubLPNCount);
extern void L1_CacheAddNewLpn(SUBCMD* pSubCmd, U16 usCacheLine);
extern void L1_ReleaseCacheLine(U16 usCacheLine);
extern void L1_CacheAttachBuffer(U16 usCacheLine, U16 usPhyBuffID);
extern U32 L1_CacheNeedIdleFlush(void);
extern U32 L1_CacheIdleFlush(void);
extern U32 L1_CacheFlushAll(void);
extern U16 L1_GetCacheLine(U32 LbaAddr);
extern U32 L1_CalcSubLBAForL1(U32 phyLBA);
#endif

/********************** FILE END ***************/

