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
Filename    :L2_TableBBT.c
Version     :Ver 1.0
Author      :Via
Date        :2015.05.21
Description :functions about bbt

Others      :
Modify      :
*******************************************************************************/
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_DMAE.h"
#include "FW_BufAddr.h"
#include "FW_Event.h"
#include "L2_Defines.h"
#include "L2_TableBBT.h"
#include "L2_FTL.h"
#include "L2_FCMDQ.h"
#include "L2_Interface.h"
#ifdef L3_BBT_TEST
#include "FCMD_Test.h"
#endif

LOCAL MCU12_VAR_ATTR RED *l_ptBBTSpare;
LOCAL MCU12_VAR_ATTR U8 *l_pBBTFlashStatus;
LOCAL MCU12_VAR_ATTR BBT_MANAGER *l_ptBBTManager;
LOCAL MCU12_VAR_ATTR GBBT_INFO l_tGBbtInfo;
LOCAL MCU12_VAR_ATTR BOOL l_bSaveBbtLocked = FALSE;

extern GLOBAL HOST_INFO_PAGE *g_pSubSystemHostInfoPage;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulGBBT;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;

LOCAL MCU12_VAR_ATTR U32 l_ulPbnBindingTableConstructBitmap;
LOCAL MCU12_VAR_ATTR U32 l_ulPbnBindingTableLoadingBuffer;

LOCAL U8 *s_ucLoadPbnBindingTableStatus;
LOCAL U32 *s_ulLoadPbnBindingTableStage;
LOCAL U16 *s_usCurrentPage;
////////////////////////////////////////////////////////////////////////////////
// BBT memory related interfaces                                              //
////////////////////////////////////////////////////////////////////////////////
/*==============================================================================
Func Name  : L2_BbtDramAllocate
Input      : U32* pFreeSramBase
Output     : NONE
Return Val :
Discription: BBT allocates sram1.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring 
==============================================================================*/
GLOBAL void MCU1_DRAM_TEXT L2_BbtDramAllocate(U32* pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_ptBBTManager = (BBT_MANAGER *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(BBT_MANAGER)*(SUBSYSTEM_LUN_NUM+1)));

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    s_ucLoadPbnBindingTableStatus = (U8*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(U8)*(SUBSYSTEM_LUN_MAX)));
    
    COM_MemAddr16DWAlign(&ulFreeDramBase);
    s_ulLoadPbnBindingTableStage = (U32*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(U32)*(SUBSYSTEM_LUN_MAX)));
    COM_MemZero(s_ulLoadPbnBindingTableStage, SUBSYSTEM_LUN_MAX);

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    s_usCurrentPage = (U16*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(U16)*(SUBSYSTEM_LUN_MAX)));
    COM_MemZero((U32*)s_usCurrentPage, SUBSYSTEM_LUN_MAX / 2);
    
    COM_MemAddr16DWAlign(&ulFreeDramBase);    
    *pFreeDramBase = ulFreeDramBase;

    return;
}

/*==============================================================================
Func Name  : L2_BbtDramAllocate
Input      : U32* pFreeDramBase
Output     : NONE
Return Val :
Discription: BBT allocates dram.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
void MCU1_DRAM_TEXT L2_BbtDramNotPageAlignAllocate(U32* pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    /*Allocate RED area for BBT use*/
    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_ptBBTSpare = (RED *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RED)*PG_PER_WL*(SUBSYSTEM_LUN_NUM+1));

    /*Allocate FlashStatus area for BBT use*/
    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_pBBTFlashStatus = (U8 *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(U8)*(SUBSYSTEM_LUN_NUM+1));
    

    *pFreeDramBase = ulFreeDramBase;

    return;
}

/*==============================================================================
Func Name  : L2_BbtDramPageAlignAllocate
Input      : U32* pFreeDramBase
Output     : NONE
Return Val :
Discription: BBT allocates dram
Usage      :
History    :
==============================================================================*/
void MCU1_DRAM_TEXT L2_BbtDramPageAlignAllocate(U32* pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    // Allocate the local-bbt buffer, each local-bbt has a separate buffer.
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    g_pMCU12MiscInfo->ulLBBT = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LBBT_BUF_SIZE_TOTAL);

    // allocate memory for the PBN binding table
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulPbnBindingTableConstructBitmap = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, ( ( BBT_BLK_PER_PLN * PLN_PER_LUN ) / 8 ) + 1);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulPbnBindingTableLoadingBuffer = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, SUBSYSTEM_LUN_NUM * LOGIC_PG_SZ);

    *pFreeDramBase = ulFreeDramBase;

    #ifdef L3_BBT_TEST
    L2_BbtTestDramAllocate(pFreeDramBase);
    #endif

    return;
}

/*==============================================================================
Func Name  : L2_BbtGetGBBTAddr
Input      : NONE
Output     : NONE
Return Val :
Discription: get the global bbt memeory address.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL U32 MCU12_DRAM_TEXT L2_BbtGetGBBTAddr(void)
{
    return g_ulGBBT;
}

/*==============================================================================
Func Name  : L2_BbtMemZero
Input      : NONE
Output     : NONE
Return Val :
Discription: zero the memeory of global bbt and each local bbt.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtMemZero(BOOL bResetGBBT)
{
    if (bResetGBBT)
    {
        HAL_DMAESetValue(g_ulGBBT, COM_MemSize16DWAlign(GBBT_BUF_SIZE), 0);
    }

    HAL_DMAESetValue(g_pMCU12MiscInfo->ulLBBT, COM_MemSize16DWAlign(LBBT_BUF_SIZE_TOTAL), 0);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtMemZeroGBbt
 Input      : NONE
 Output     : NONE
 Return Val :
 Discription: zero the memeory of global bbt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
GLOBAL void MCU1_DRAM_TEXT L2_BbtMemZeroGBbt(void)
{
    HAL_DMAESetValue(g_ulGBBT, COM_MemSize16DWAlign(GBBT_BUF_SIZE), 0);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtMemZeroLBbt
 Input      : NONE
 Output     : NONE
 Return Val :
 Discription: zero the memeory of the target local bbt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtMemZeroLBbt(U8 ucTLun)
{
    HAL_DMAESetValue(g_pMCU12MiscInfo->ulLBBT + ucTLun*LBBT_BUF_SIZE, COM_MemSize16DWAlign(LBBT_BUF_SIZE), 0);

    return;
}

/*==============================================================================
Func Name  : L2_BbtSetGBbtBadBlkBit
Input      : U8 ucTLun
             U8 ucPln 
             U16 usBlock
Output     : NONE
Return Val :
Discription: set the global bad blk bit according the input parameters.
Usage      : GBbtFun(TLun, Pln, Blk) = TLun * BBT_BLK_PER_PLN * PLN_PER_LUN + Pln * BBT_BLK_PER_PLN + Blk
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtSetGBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;

    ulBitPos = ucTLun * PLN_PER_LUN * BBT_BLK_PER_PLN + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = g_ulGBBT + ulBitPos / 8;
    *(volatile U8 *)ulBytePos |= (1 << (ulBitPos % 8));

    return;
}

/*==============================================================================
Func Name  : L2_BbtGetGBbtBadBlkBit
Input      : U8 ucTLun
U8 ucPln
U16 usBlock
Output     : NONE
Return Val : BOOL
Discription: get the global bad blk bit according the input parameters.
Usage      : GBbtFun(TLun, Pln, Blk) = TLun * BBT_BLK_PER_PLN * PLN_PER_LUN + Pln * BBT_BLK_PER_PLN + Blk
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL L2_BbtGetGBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;
    BOOL bBadBlk;

    ulBitPos = ucTLun * PLN_PER_LUN * BBT_BLK_PER_PLN + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = g_ulGBBT + ulBitPos / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (ulBitPos % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

/*==============================================================================
 Func Name  : L2_BbtSetLBbtBadBlkBit
 Input      : U8 ucTLun
              U8 ucPln
              U16 usBlock
 Output     : NONE
 Return Val :
 Discription: set the lobal bad blk bit according the input parameters.
 Usage      : LBbtFun(Pln, Blk) = Pln * BBT_BLK_PER_PLN + Blk
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtSetLBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;

    ulBitPos = ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = (g_pMCU12MiscInfo->ulLBBT + ucTLun * LBBT_BUF_SIZE) + ulBitPos / 8;
    *(volatile U8 *)ulBytePos |= (1 << (ulBitPos % 8));

    return;
}

/*==============================================================================
 Func Name  : L2_BbtGetLBbtBadBlkBit
 Input      : U8 ucTLun
              U8 ucPln
              U16 usBlock
 Output     : NONE
 Return Val : BOOL
 Discription: get the local bad blk bit according the input parameters.
 Usage      : LBbtFun(Pln, Blk) = Pln * BBT_BLK_PER_PLN + Blk
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtGetLBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;
    BOOL bBadBlk;

    ulBitPos = ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = (g_pMCU12MiscInfo->ulLBBT + ucTLun * LBBT_BUF_SIZE) + ulBitPos / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (ulBitPos % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

/*==============================================================================
 Func Name  : L2_BbtSetGBbtRootPointer
 Input      : U8 ucTLun
              U32 ulTableBlkEnd
              U32 ulDataBlkStart
 Output     : NONE
 Return Val :
 Discription: set the global-bbt-rt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtSetGBbtRootPointer(U8 ucTLun, U32 ulTableBlkEnd, U32 ulDataBlkStart)
{
    U32 RootPointerBase;

    RootPointerBase = g_ulGBBT + GBBT_BUF_SIZE - SUBSYSTEM_LUN_NUM * RT_DATA_BYTE_NUM;
    *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM) = ulTableBlkEnd;
    *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM + 4) = ulDataBlkStart;

    return;
}

/*==============================================================================
 Func Name  : L2_BbtSetGBbtSlcTlcBlock
 Input      : U8 ucTLun
              U32 ulSLCBlkEnd
              U32 ulTLCBlkEnd
 Output     : NONE
 Return Val :
 Discription: set the global-bbt-rt.
 Usage      :
 History    :
 1. 2015.9.18 steven add for tlc table rebuild
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtSetGBbtSlcTlcBlock(U8 ucTLun, U32 ulSLCBlkEnd, U32 ulTLCBlkEnd)
{
    U32 RootPointerBase;

    RootPointerBase = g_ulGBBT + GBBT_BUF_SIZE - SUBSYSTEM_LUN_NUM * (RT_DATA_BYTE_NUM * 2);//record like root pointer
    *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM) = ulSLCBlkEnd;
    *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM + 4) = ulTLCBlkEnd;

    return;
}

/*==============================================================================
 Func Name  : L2_BbtGetGBbtRootPointer
 Input      : U8 ucTLun
 U32 *pTableBlkEnd
 U32 *pDataBlkStart
 Output     : NONE
 Return Val :
 Discription: get the global-bbt-rt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtGetGBbtRootPointer(U8 ucTLun, U32 *pTableBlkEnd, U32 *pDataBlkStart)
{
    U32 RootPointerBase;

    RootPointerBase = g_ulGBBT + GBBT_BUF_SIZE - SUBSYSTEM_LUN_NUM * RT_DATA_BYTE_NUM;
    *pTableBlkEnd = *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM);
    *pDataBlkStart = *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM + 4);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtGetGBbtSlcTlcBlock
 Input      : U8 ucTLun
              U32 *pSLCBlkEnd
              U32 *pTLCBlkEnd
 Output     : NONE
 Return Val :
 Discription: get the global-bbt-rt.
 Usage      :
 History    :
 1. 2015.9.18 steven add for tlc rebuild
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtGetGBbtSlcTlcBlock(U8 ucTLun, U32 *pSLCBlkEnd, U32 *pTLCBlkEnd)
{
    U32 RootPointerBase;

    RootPointerBase = g_ulGBBT + GBBT_BUF_SIZE - SUBSYSTEM_LUN_NUM * (RT_DATA_BYTE_NUM * 2);
    *pSLCBlkEnd = *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM);
    *pTLCBlkEnd = *(volatile U32 *)(RootPointerBase + ucTLun * RT_DATA_BYTE_NUM + 4);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtSetLBbtRootPointer
 Input      : U8 ucTLun
              U32 ulTableBlkEnd
              U32 ulDataBlkStart
 Output     : NONE
 Return Val :
 Discription: set the local-bbt-rt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtSetLBbtRootPointer(U8 ucTLun, U32 ulTableBlkEnd, U32 ulDataBlkStart)
{
    U32 RootPointerBase;

    RootPointerBase = g_pMCU12MiscInfo->ulLBBT + (ucTLun+1) * LBBT_BUF_SIZE - RT_DATA_BYTE_NUM;
    *(volatile U32 *)(RootPointerBase) = ulTableBlkEnd;
    *(volatile U32 *)(RootPointerBase + 4) = ulDataBlkStart;
    return;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtCopyPbnBindingTableToLocalBbtBuffer(U8 ucTlun)
{
    U32 ulPbnBindingTableTargetAddress;
    U32 ulPbnBindingTableSourceAddress;
    U32 i;

    // determine the target address of PBN binding table in local BBT buffer, note that
    // the start address of the PBN binding table in local BBT buffer is the second to last 4K
    ulPbnBindingTableTargetAddress = g_pMCU12MiscInfo->ulLBBT + (ucTlun+1) * LBBT_BUF_SIZE - SOFT_BINDING_TABLE_OFFSET;

    // determine the source address of PBN binding table 
    ulPbnBindingTableSourceAddress =
            g_pMCU12MiscInfo->ulPbnBindingTable // starting address
            + (ucTlun * (BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16))); // LUN offset

    // copy all entries of the PBN binding table to the local BBT buffer
    for (i = 0; i < (BBT_BLK_PER_PLN * PLN_PER_LUN); i++)
    {
        *( (U16*) ulPbnBindingTableTargetAddress ) = *( (U16*) ulPbnBindingTableSourceAddress );

        // increase both ulPbnBindingTableTargetAddress and ulPbnBindingTableSourceAddress
        ulPbnBindingTableTargetAddress += sizeof(U16);
        ulPbnBindingTableSourceAddress += sizeof(U16);
    }

    return;
}

/*==============================================================================
 Func Name  : L2_BbtSetLBbtSlcTlcBlock
 Input      : U8 ucTLun
              U32 ulSLCBlkEnd
              U32 ulTLCBlkEnd
 Output     : NONE
 Return Val :
 Discription: set the local-bbt-rt.
 Usage      :
 History    :
 1. 20159.18 steven add for tlc table rebuild
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtSetLBbtSlcTlcBlock(U8 ucTLun, U32 ulSLCBlkEnd, U32 ulTLCBlkEnd)
{
    U32 RootPointerBase;

    RootPointerBase = g_pMCU12MiscInfo->ulLBBT + (ucTLun + 1) * LBBT_BUF_SIZE - (RT_DATA_BYTE_NUM * 2);
    *(volatile U32 *)(RootPointerBase) = ulSLCBlkEnd;
    *(volatile U32 *)(RootPointerBase + 4) = ulTLCBlkEnd;

    return;
}

/*==============================================================================
 Func Name  : L2_BbtGetLBbtRootPointer
 Input      : U8 ucTLun
              U32 *pTableBlkEnd
              U32 *pDataBlkStart
 Output     : NONE
 Return Val :
 Discription: get the local-bbt-rt.
 Usage      :
 History    :
 1. 2014.10.13 steven create
 2. 2015.05.25 Jason refactoring
 =============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtGetLBbtRootPointer(U8 ucTLun, U32 *pTableBlkEnd, U32 *pDataBlkStart)
{
    U32 RootPointerBase;

    RootPointerBase = g_pMCU12MiscInfo->ulLBBT + (ucTLun + 1) * LBBT_BUF_SIZE - RT_DATA_BYTE_NUM;
    *pTableBlkEnd = *(volatile U32 *)(RootPointerBase);
    *pDataBlkStart = *(volatile U32 *)(RootPointerBase + 4);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtGetLBbtSlcTlcBlock
 Input      : U8 ucTLun
              U32 *pSLCBlkEnd
              U32 *pTLCBlkEnd
 Output     : NONE
 Return Val :
 Discription: get the local-bbt-rt.
 Usage      :
 History    :
 1. 2015.9.18 steven add for tlc table rebuild
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtGetLBbtSlcTlcBlock(U8 ucTLun, U32 *pSLCBlkEnd, U32 *pTLCBlkEnd)
{
    U32 RootPointerBase;

    RootPointerBase = g_pMCU12MiscInfo->ulLBBT + (ucTLun + 1) * LBBT_BUF_SIZE - (RT_DATA_BYTE_NUM * 2);
    *pSLCBlkEnd = *(volatile U32 *)(RootPointerBase);
    *pTLCBlkEnd = *(volatile U32 *)(RootPointerBase + 4);

    return;
}

/*==============================================================================
Func Name  : L2_BbtSetRootPointer
Input      : U8 ucTLun
             U32 ulTableBlkEnd
             U32 ulDataBlkStart
Output     : NONE
Return Val : 
Discription: set the global & Local RootPointer according the input parameters.
Usage      : 
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL void MCU1_DRAM_TEXT L2_BbtSetRootPointer(U8 ucTLun, U32 ulTableBlkEnd, U32 ulDataBlkStart)
{
    L2_BbtSetGBbtRootPointer(ucTLun, ulTableBlkEnd, ulDataBlkStart);
    
    return;
}

/*==============================================================================
Func Name  : L2_BbtSetTlcSlcBlock
Input      : U8 ucTLun
             U32 ulSLCBlkEnd
             U32 ulTLCBlkEnd
Output     : NONE
Return Val :
Discription: set the slc & tlc end block according the input parameters.
Usage      :
History    :
1. 2015/9/18 steven add for tlc rebuild
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT L2_BbtSetSlcTlcBlock(U8 ucTLun, U32 ulSLCBlkEnd, U32 ulTLCBlkEnd)
{
    L2_BbtSetGBbtSlcTlcBlock(ucTLun, ulSLCBlkEnd, ulTLCBlkEnd);

    return;
}

/*==============================================================================
Func Name  : L2_BbtGetRootPointer
Input      : U8 ucTLun
             U32 *pTableBlkEnd
             U32 *pDataBlkStart
Output     : NONE
Return Val :
Discription: get the global RootPointer according the input parameters.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL void MCU1_DRAM_TEXT L2_BbtGetRootPointer(U8 ucTLun, U32 *pTableBlkEnd, U32 *pDataBlkStart)
{
    L2_BbtGetGBbtRootPointer(ucTLun, pTableBlkEnd, pDataBlkStart);

    return;
}

/*==============================================================================
Func Name  : L2_BbtGetSlcTlcBlock
Input      : U8 ucTLun
             U32 *pSLCBlkEnd
             U32 *pTLCBlkEnd
Output     : NONE
Return Val :
Discription: get the global RootPointer according the input parameters.
Usage      :
History    :
1. 2015.9.18 steven add for tlc rebuild
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT L2_BbtGetSlcTlcBlock(U8 ucTLun, U32 *pSLCBlkEnd, U32 *pTLCBlkEnd)
{
    L2_BbtGetGBbtSlcTlcBlock(ucTLun, pSLCBlkEnd, pTLCBlkEnd);

    return;
}

/*==============================================================================
Func Name  : L2_BbtMergeGBbtRootPointer
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription: merge the local root pointer to the global root pointer area.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtMergeGBbtRootPointer(U8 ucTLun)
{
    U32 ulTableBlkEnd, ulDataBlkStart, ulSLCBlkEnd, ulTLCBlkEnd;

    L2_BbtGetLBbtRootPointer(ucTLun, &ulTableBlkEnd, &ulDataBlkStart);

    L2_BbtSetGBbtRootPointer(ucTLun, ulTableBlkEnd, ulDataBlkStart);

    L2_BbtGetLBbtSlcTlcBlock(ucTLun, &ulSLCBlkEnd, &ulTLCBlkEnd);

    L2_BbtSetGBbtSlcTlcBlock(ucTLun, ulSLCBlkEnd, ulTLCBlkEnd);

    return;
}

/*==============================================================================
Func Name  : L2_BbtMergeLBbtRootPointer
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription: merge the global root pointer to the local root pointer area.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtMergeLBbtRootPointer(U8 ucTLun)
{
    U32 ulTableBlkEnd, ulDataBlkStart, ulSLCBlkEnd, ulTLCBlkEnd;

    L2_BbtGetGBbtRootPointer(ucTLun, &ulTableBlkEnd, &ulDataBlkStart);

    L2_BbtSetLBbtRootPointer(ucTLun, ulTableBlkEnd, ulDataBlkStart);

    L2_BbtGetGBbtSlcTlcBlock(ucTLun, &ulSLCBlkEnd, &ulTLCBlkEnd);

    L2_BbtSetLBbtSlcTlcBlock(ucTLun, ulSLCBlkEnd, ulTLCBlkEnd);

    return;
}

extern GLOBAL U8 L2_BbtIsPbnBindingTableEnable(void);

/*==============================================================================
Func Name  : L2_BbtAddBbtBadBlk
Input      : U8 ucTLun
             U16 usPhyBlk
Output     : NONE
Return Val :
Discription: set the all pln to bad blk
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT L2_BbtAddBbtBadBlk(U8 ucTLun, U16 usPhyBlk, U8 BadBlkType, U8 ucErrType)
{
    U8 ucPln;

    if(L2_BbtIsPbnBindingTableEnable() == FALSE)
    {
        DBG_Printf("PBN binding table not ready\n");
        DBG_Getch();
    }
    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        // look up the PBN binding table to find out the plane block
        // current PBN maps to in the plane
        U16 usTargetPlaneBlock = L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, ucPln);

        L2_BbtSetGBbtBadBlkBit(ucTLun, ucPln, usTargetPlaneBlock);
        if(0 != BadBlkType)
        {
            L2_BbtSetLBbtBadBlkBit_Ext(ucTLun, ucPln, usTargetPlaneBlock, BadBlkType, ucErrType);
            #ifdef L2_BBT_UNI_TEST
            //L2_BbtTestSetLBbtBadBlkBit_Ext(ucTLun, ucPln, usTargetPlaneBlock, BadBlkType, ucErrType);
            #endif
        }
    }

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtAddBbtBadBlkSinglePlane(U8 ucTLun, U16 usPhyBlk, U8 ucPlane, U8 BadBlkType, U8 ucErrType)
{
    L2_BbtSetGBbtBadBlkBit(ucTLun, ucPlane, usPhyBlk);
    if(0 != BadBlkType)
    {
        L2_BbtSetLBbtBadBlkBit_Ext(ucTLun, ucPlane, usPhyBlk, BadBlkType, ucErrType);
    }

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtAddLBbtBadBlk(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        L2_BbtSetLBbtBadBlkBit(ucTLun, ucPln, usPhyBlk);
    }

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtAddLBbtBadBlkSinglePlane(U8 ucTLun, U16 usPhyBlk, U8 ucPlane)
{
    L2_BbtSetLBbtBadBlkBit(ucTLun, ucPlane, usPhyBlk);

    return;
}

/*==============================================================================
Func Name  : L2_BbtIsGBbtBadBlock
Input      : U8 ucTLun
             U16 usPhyBlk
Output     : NONE
Return Val : BOOL
Discription: check the blk is bad or not in global bbt ?
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL L2_BbtIsGBbtBadBlock(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;
    BOOL bBadBlk = FALSE;

    if(L2_BbtIsPbnBindingTableEnable() == FALSE)
    {
        DBG_Printf("PBN binding table not ready\n");
        DBG_Getch();
    }
    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        // look up the PBN binding table to find out the plane block
        // current PBN maps to in the plane
        U16 usTargetPlaneBlock = L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, ucPln);
        if (TRUE == L2_BbtGetGBbtBadBlkBit(ucTLun, ucPln, usTargetPlaneBlock))
        {
            bBadBlk = TRUE;
            break;
        }
    }

    return bBadBlk;
}

LOCAL BOOL MCU12_DRAM_TEXT L2_BbtIsGBbtBadBlockSinglePlane(U8 ucTLun, U16 usPhyBlk, U8 ucPlane)
{
    return (TRUE == L2_BbtGetGBbtBadBlkBit(ucTLun, ucPlane, usPhyBlk)) ? TRUE : FALSE;
}

/*==============================================================================
Func Name  : L2_BbtIsLBbtBadBlock
Input      : U8 ucTLun
U16 usPhyBlk
Output     : NONE
Return Val : BOOL
Discription: check the blk is bad or not in local bbt ?
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtIsLBbtBadBlock(U8 ucTLun, U16 usPhyBlk)
{
    U8 ucPln;
    BOOL bBadBlk = FALSE;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        if (TRUE == L2_BbtGetLBbtBadBlkBit(ucTLun, ucPln, usPhyBlk))
        {
            bBadBlk = TRUE;
            break;
        }
    }

    return bBadBlk;
}

LOCAL BOOL MCU12_DRAM_TEXT L2_BbtIsLBbtBadBlockSinglePlane(U8 ucTLun, U16 usPhyBlk, U8 ucPlane)
{
    return (TRUE == L2_BbtGetLBbtBadBlkBit(ucTLun, ucPlane, usPhyBlk)) ? TRUE : FALSE;
}

/*==============================================================================
Func Name  : L2_BbtMergeGBbtArea
Input      : U8 ucTLun
Output     : NONE
Return Val : 
Discription: Merge the local bbt Area to the global bbt Area
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtMergeGBbtArea(U8 ucTLun)
{
    U16 usBlk;
    U8 ucPlane;

    for (usBlk = 0; usBlk < BBT_BLK_PER_PLN; usBlk++)
    {
        for (ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
        {
            if (TRUE == L2_BbtIsLBbtBadBlockSinglePlane(ucTLun, usBlk, ucPlane))
            {
                L2_BbtAddBbtBadBlkSinglePlane(ucTLun, usBlk, ucPlane, 0, 0);
            }
        }
    }
    
    return;
}

/*==============================================================================
Func Name  : L2_BbtMergeLBbtArea
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription: Merge the global bbt Area to the local bbt
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtMergeLBbtArea(U8 ucTLun)
{
    U16 usBlk;
    U8 ucPlane;

    for (usBlk = 0; usBlk < BBT_BLK_PER_PLN; usBlk++)
    {
        for (ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
        {
            if (TRUE == L2_BbtIsGBbtBadBlockSinglePlane(ucTLun, usBlk, ucPlane))
            {
                L2_BbtAddLBbtBadBlkSinglePlane(ucTLun, usBlk, ucPlane);
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtRebuildMergeGBbt
Input      : NONE
Output     : NONE
Return Val : 
Discription: Merge all local-bbt-info to the global-bbt-info
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtRebuildMergeGBbt(void)
{
    U8 ucTLun;

    /*mark by Nina 2017-12-18, format GBBT stage may also add bad blk to GBBT */
    //L2_BbtMemZeroGBbt();

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        L2_BbtMergeGBbtArea(ucTLun);
        L2_BbtMergeGBbtRootPointer(ucTLun);
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtRebuildMergeGBbt
Input      : NONE
Output     : NONE
Return Val : 
Discription: update the all of the local-bbt-info by the global-bbt-info
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtSaveMergeLBbt(void)
{
    U8 ucTLun;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        //L2_BbtMemZeroLBbt(ucTLun);

        L2_BbtMergeLBbtArea(ucTLun);
        L2_BbtMergeLBbtRootPointer(ucTLun);
        L2_BbtCopyPbnBindingTableToLocalBbtBuffer(ucTLun);
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtCheckGBBTValid
Input      : NONE
Output     : NONE
Return Val :
Discription: check gbbt after bbt load
Usage      :
History    :
1. 2015.07.30 Jason create
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtCheckGBBTValid(void)
{
    BOOL bValid = FALSE;
    U32 ulBytePos;

    for (ulBytePos = 0; ulBytePos < GBBT_BUF_SIZE; ulBytePos++)
    {
        if (0 != *(volatile U8 *)(g_ulGBBT + ulBytePos))
        {
            bValid = TRUE;
            break;
        }
    }

    return bValid;
}

/*==============================================================================
Func Name  : L2_BbtPrintBbt
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription: list the bad blk of Tlun
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtPrintBbt(U8 ucTLun)
{
    U8 ucPln;
    U16 usBlk;
    U32 ulBadBlkCnt=0;
    U8 ucExtInfo = 0;
    U16 usPlaneBadBlockCount[PLN_PER_LUN];

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        usPlaneBadBlockCount[ucPln] = 0;
    }

    for (usBlk = 0; usBlk < BBT_BLK_PER_PLN; usBlk++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            U8 ucGbbtBadBlockBit = L2_BbtGetGBbtBadBlkBit(ucTLun, ucPln, usBlk);
            U8 ucLbbtBadBlockBit = L2_BbtGetLBbtBadBlkBit(ucTLun, ucPln, usBlk);

            if (ucGbbtBadBlockBit != ucLbbtBadBlockBit)
            {
                DBG_Printf("inconsistency between GBBT and LBBT, LUN %d block %d plane %d GBBT bit %d LBBT bit %d\n",
                        ucTLun, usBlk, ucPln, ucGbbtBadBlockBit, ucLbbtBadBlockBit);
                DBG_Getch();
            }

            if (TRUE == ucGbbtBadBlockBit)
            {
                usPlaneBadBlockCount[ucPln] += 1;
#ifndef SPEEDUP_UNH_IOL
                //DBG_Printf("MCU#%d LUN#%d plane:%d Bad-Blk:%d\n", HAL_GetMcuId(), ucTLun, ucPln, usBlk);
#endif
            }
        }
    }
        
    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        DBG_Printf("MCU#%d LUN#%d PLANE#%d bad plane block count #%d\n", HAL_GetMcuId(), ucTLun, ucPln, usPlaneBadBlockCount[ucPln]);
    }
    
    return;
}

/*==============================================================================
Func Name  : L2_BbtPrintAllBbt
Input      : NONE
Output     : NONE
Return Val :
Discription: list the bad blk of each Tlun
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT L2_BbtPrintAllBbt(void)
{
    U8 ucTLun;
    
    //DBG_Printf("MCU#%d Blk Status Bit: 0-Good; 1-Bad\n1.MemAddres(TLun, Pln, Blk) = 0x%x + (TLun*0x%x + Pln*0x%x + Blk)/8;\n2.BitOffSet(TLun, Pln, Blk) = (TLun*0x%x + Pln*0x%x + Blk)%%8.\n3.ExtInfo =  (BadBlkType << 2) || ErrType\n", HAL_GetMcuId(), g_ulGBBT, PLN_PER_LUN * BBT_BLK_PER_PLN, BBT_BLK_PER_PLN, PLN_PER_LUN * BBT_BLK_PER_PLN, BBT_BLK_PER_PLN);

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        L2_BbtPrintBbt(ucTLun);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////
// BBT Manager control related interfaces                                     //
////////////////////////////////////////////////////////////////////////////////
/*==============================================================================
Func Name  : L2_BbtGBbtInfoInit
Input      : NONE
Output     : NONE
Return Val :
Discription: reset the GBbtInfo.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtGBbtInfoInit(BOOL bResetGBBT)
{
    l_tGBbtInfo.ulBbtMark = BBT_NEW_V2_MARK;
    l_tGBbtInfo.bsGBbtTLun = 0;
    l_tGBbtInfo.bsGBbtPage = INVALID_4F;
    l_tGBbtInfo.bsGBbtBlk = GBBT_BLK;
    l_tGBbtInfo.ulMaxBbtSn = 0;

    if (bResetGBBT)
    {
        l_tGBbtInfo.bsFormatGBBT = FALSE;
    }
    COM_MemZero((U32*)l_tGBbtInfo.aLunMskBitMap, BBT_TLUN_MSK_SIZE);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtIsGBbtValid
 Input      : NONE
 Output     : NONE
 Return Val : BOOL
 Discription:
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtIsGBbtValid(void)
{
    if (BBT_RDT_MARK == l_tGBbtInfo.ulBbtMark)
    {
        return TRUE;
    }
    else
    {
        return (INVALID_4F == l_tGBbtInfo.bsGBbtPage) ? FALSE : TRUE;
    }
}

/*==============================================================================
 Func Name  : L2_BbtCpySpareToGBbt
 Input      : RED *pSpare
 Output     : NONE
 Return Val :
 Discription: Update the GBbtInfo.
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtCpySpareToGBbt(RED *pSpare)
{
    BOOL bFormatGBBT = l_tGBbtInfo.bsFormatGBBT;

    // Double-Check the GBbtInfo Valid
    if (((BBT_NEW_MARK != pSpare->m_tGBbtInfo.ulBbtMark) && (BBT_NEW_V2_MARK != pSpare->m_tGBbtInfo.ulBbtMark))
     || (SUBSYSTEM_LUN_NUM <= pSpare->m_tGBbtInfo.bsGBbtTLun)
     || (BBT_PG_PER_BLK <= pSpare->m_tGBbtInfo.bsGBbtPage))
    { 
        DBG_Printf("MCU#%d GBbtInfo [Mark=0x%x, TLun=%d, Page=%d] invalid.\n", HAL_GetMcuId(), pSpare->m_tGBbtInfo.ulBbtMark, pSpare->m_tGBbtInfo.bsGBbtTLun, pSpare->m_tGBbtInfo.bsGBbtPage);
        DBG_Getch();        
    }

    COM_MemCpy((U32*)&l_tGBbtInfo, (U32*)&pSpare->m_tGBbtInfo, sizeof(GBBT_INFO) / sizeof(U32));
    if (BBT_NEW_MARK == pSpare->m_tGBbtInfo.ulBbtMark)
    {
        DBG_Printf("Old version !!!!!\n");
        l_tGBbtInfo.ulBbtMark = BBT_NEW_V2_MARK;
        l_tGBbtInfo.bsGBbtBlk = GBBT_BLK;
    }
    l_tGBbtInfo.bsFormatGBBT = bFormatGBBT;
    return;
}

/*==============================================================================
 Func Name  : L2_BbtCmpSpareWithGBbt
 Input      : RED *pSpare
 Output     : NONE
 Return Val :
 Discription:
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtCmpSpareWithGBbt(RED *pSpare)
{
    BOOL bSyncStatus = TRUE;

    if ((l_tGBbtInfo.ulBbtMark  != pSpare->m_tGBbtInfo.ulBbtMark)
     || (l_tGBbtInfo.ulMaxBbtSn != pSpare->m_tGBbtInfo.ulMaxBbtSn)
     || (l_tGBbtInfo.bsGBbtTLun != pSpare->m_tGBbtInfo.bsGBbtTLun)
     || (l_tGBbtInfo.bsGBbtPage != pSpare->m_tGBbtInfo.bsGBbtPage))
    {
        bSyncStatus = FALSE;
    }

    return bSyncStatus;
}

/*==============================================================================
 Func Name  : L2_BbtMskGBbtTLun
 Input      : U8 ucTLun
 Output     : NONE
 Return Val :
 Discription:
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtMskGBbtTLun(U8 ucTLun)
{
    l_tGBbtInfo.aLunMskBitMap[ucTLun / 32] |= 1 << (ucTLun % 32);

    return;
}

/*==============================================================================
 Func Name  : L2_BbtIsMskGBbtTLun
 Input      : U8 ucTLun
 Output     : NONE
 Return Val : BOOL
 Discription:
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtIsMskGBbtTLun(U8 ucTLun)
{
    return (0 == (l_tGBbtInfo.aLunMskBitMap[ucTLun / 32] & (1 << (ucTLun % 32)))) ? FALSE : TRUE;
}

/*==============================================================================
 Func Name  : L2_BbtFindNextGBbtTLun
 Input      : U8 ucTLun
 Output     : NONE
 Return Val : U8
 Discription: Select the next valid GBbtTLun
 Usage      :
 History    :
 1. 2015.05.26 Jason create
 =============================================================================*/
LOCAL U8 MCU12_DRAM_TEXT L2_BbtFindNextGBbtTLun(U8 ucTLun)
{
    U8 ucValidTLun;

    ucValidTLun = (ucTLun + 1) % SUBSYSTEM_LUN_NUM;
    while (TRUE == L2_BbtIsMskGBbtTLun(ucValidTLun))
    {
        ucValidTLun = (ucValidTLun + 1) % SUBSYSTEM_LUN_NUM;
        if (ucTLun == ucValidTLun && TRUE == L2_BbtIsMskGBbtTLun(ucValidTLun))
        {
            DBG_Printf("MCU#%d Havn't the valid global bbt Tlun.\n", HAL_GetMcuId());
            return INVALID_2F;
        }
    }

    return ucValidTLun;
}

/*==============================================================================
Func Name  : L2_BbtUpdateGBbtTLun
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription: Select a valid Tlun as the new Global-Bbt-lun.
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtUpdateGBbtTLun(U8 ucTLun)
{
    l_tGBbtInfo.ulBbtMark = BBT_NEW_V2_MARK;
    l_tGBbtInfo.bsGBbtPage = INVALID_4F;
    l_tGBbtInfo.bsGBbtTLun = L2_BbtFindNextGBbtTLun(ucTLun);

    if (INVALID_2F == l_tGBbtInfo.bsGBbtTLun)
    {
        return FALSE;
    }

    return TRUE;
}

/*==============================================================================
Func Name  : L2_BbtGBbtMgrInit
Input      : NONE
Output     : NONE
Return Val :
Discription: Reset the Global Bbt manager.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtGBbtMgrInit(void)
{
    BBT_MANAGER *ptGBbtManager;

    // Init the global-bbt-manager
    ptGBbtManager = &l_ptBBTManager[0];

    ptGBbtManager->bsOpStage = 0;
    ptGBbtManager->bsPln = START_PLN_IN_GBBT;
    ptGBbtManager->bsPage = INVALID_4F; // the bsPage records the latest bbt page number. 
    ptGBbtManager->bsBlk = l_tGBbtInfo.bsGBbtBlk; //read only

    ptGBbtManager->bsBuffID = COM_GetBufferIDByMemAddr(g_ulGBBT, TRUE, LOGIC_PG_SZ_BITS);
    ptGBbtManager->pFlashStatus = &l_pBBTFlashStatus[0];
    ptGBbtManager->pSpare = &l_ptBBTSpare[0]; 

    return;
}

/*==============================================================================
Func Name  : L2_BbtLBbtMgrInit
Input      : NONE
Output     : NONE
Return Val :
Discription: Reset the all local bbt managers.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtLBbtMgrInit(void)
{
    U8 ucTLun;
    BBT_MANAGER *ptLBbtManager;
    
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptLBbtManager = &l_ptBBTManager[ucTLun + 1];

        ptLBbtManager->bsOpStage = 0;
        ptLBbtManager->bsPln = START_PLN_IN_LBBT;
        ptLBbtManager->bsPage = INVALID_4F; // the bsPage records the latest bbt page number. 
        ptLBbtManager->bsBlk = LBBT_BLK;

        ptLBbtManager->bsBuffID = COM_GetBufferIDByMemAddr(g_pMCU12MiscInfo->ulLBBT + ucTLun * LBBT_BUF_SIZE, TRUE, LOGIC_PG_SZ_BITS);
        ptLBbtManager->pFlashStatus = &l_pBBTFlashStatus[ucTLun + 1];
        ptLBbtManager->pSpare = &l_ptBBTSpare[ucTLun + 1];
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtManagerInit
Input      : NONE
Output     : NONE
Return Val :
Discription: Reset the bbt manager.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_BbtManagerInit(BOOL bResetGBBT)
{
    // init bbt-memeory
    L2_BbtMemZero(bResetGBBT);

    // init the global-bbt-information
    L2_BbtGBbtInfoInit(bResetGBBT);

    // Init the global-bbt-manager
    L2_BbtGBbtMgrInit();

    // init the local-bbt-manager
    L2_BbtLBbtMgrInit();

    return;
}

/*==============================================================================
Func Name  : L2_BbtWaitAllLBBTOpStageDone
Input      : NONE
Output     : NONE
Return Val :
Discription: wait all tlun finish and init local bbt stage to 0.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtWaitAllLBBTOpStageDone(U32 ulFinishStage)
{
    U8 ucTLun1, ucTLun2, ucFinishCnt;
    BOOL bDone = FALSE;
    BBT_MANAGER *ptLBbtManager1, *ptLBbtManager2;

    for (ucTLun1 = 0, ucFinishCnt = 0; ucTLun1 < SUBSYSTEM_LUN_NUM; ucTLun1++)
    {
        ptLBbtManager1 = &l_ptBBTManager[ucTLun1 + 1];
        if (ulFinishStage == ptLBbtManager1->bsOpStage)
        {
            ucFinishCnt++;
            if (SUBSYSTEM_LUN_NUM == ucFinishCnt)
            {   // wait all lbbt-manager-op-stage to finish, then reset them to init.
                bDone = TRUE;
                for (ucTLun2 = 0; ucTLun2 < SUBSYSTEM_LUN_NUM; ucTLun2++)
                {
                    ptLBbtManager2 = &l_ptBBTManager[ucTLun2 + 1];

                    ptLBbtManager2->bsOpStage = 0;
                }
                break;
            }
        }
    }

    return bDone;
}

/*==============================================================================
Func Name  : L2_BbtWaitAllLBBTOpStageDone
Input      : NONE
Output     : NONE
Return Val :
Discription: wait all tlun blk erase finish and init local bbt stage to 0. seperate block erase to several lun groups.
                 Do one group of luns at each stage.
Usage      :
History    :
1. 2017.09.26 Dannier create
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtWaitAllBlkEraseOpStageDone(U32 ulFinishStage, U8 ucLunGroup, U8 GroupSize)
{
    U8 ucTLun0, ucTLun1, ucTLun2, ucTLun3, ucFinishCnt;
    BOOL bDone = FALSE;
    BBT_MANAGER *ptLBbtManager1, *ptLBbtManager2;

    for (ucTLun0 = 0, ucFinishCnt = 0; ucTLun0 < GroupSize; ucTLun0++)
    {
        ucTLun1 = ucLunGroup * LUN_GROUP_MAX + ucTLun0;
        ptLBbtManager1 = &l_ptBBTManager[ucTLun1 + 1];
        if (ulFinishStage == ptLBbtManager1->bsOpStage)
        {
            ucFinishCnt++;
            if (GroupSize == ucFinishCnt)
            {   // wait all lbbt-manager-op-stage to finish, then reset them to init.
                bDone = TRUE;
                for (ucTLun2 = 0; ucTLun2 < GroupSize; ucTLun2++)
                {
                    ucTLun3 = ucLunGroup * LUN_GROUP_MAX + ucTLun2;
                    ptLBbtManager2 = &l_ptBBTManager[ucTLun3 + 1];

                    ptLBbtManager2->bsOpStage = 0;
                }
                break;
            }
        }
    }

    return bDone;
}

/*==============================================================================
Func Name  : L2_BbtFormatLocalBBT
Input      : NONE
Output     : NONE
Return Val :
Discription: Pre-Condition:
             All-Local-Manager-init
             Conclusion:
             1. Erase-Fail -> DBG_Getch();
             2. Erase-Success -> All local-bbt-blks are erased. and All-Local-Manager-Init-Status
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtFormatLocalBBT(void)
{
    U8 ucTLun;
    BOOL bFormatLBBTDone = FALSE;
    BBT_MANAGER *ptLBBTManager;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptLBBTManager = &l_ptBBTManager[ucTLun + 1];

        switch (ptLBBTManager->bsOpStage)
        {
            case LGBBT_FORMAT_INIT:
            {       
                ptLBBTManager->bsBlk = LBBT_BLK;
                ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                ptLBBTManager->bsPage = INVALID_4F;
                ptLBBTManager->bsOpStage = LGBBT_FORMAT_ERS;
                break;
            }
            case LGBBT_FORMAT_ERS:
            {
                FCMD_REQ_ENTRY *ptReqEntry;
                
                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
                
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);  
                
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;  
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                                
                if (NULL != ptLBBTManager->pFlashStatus)
                {
                    *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                    ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                }
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
                                
                ptLBBTManager->bsOpStage = LGBBT_FORMAT_ERS_WAIT;
                break;
            }
            case LGBBT_FORMAT_ERS_WAIT:
            {
                if (SUBSYSTEM_STATUS_PENDING != *ptLBBTManager->pFlashStatus)
                {
                    if (SUBSYSTEM_STATUS_FAIL != *ptLBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Local-BBT InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Local-BBT Ers-Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    ptLBBTManager->bsPln = (ptLBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptLBBTManager->bsPln)
                    {       
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = LGBBT_FORMAT_FINISH;
                    }
                    else
                    {
                        ptLBBTManager->bsOpStage = LGBBT_FORMAT_ERS;
                    }                    
                }
                break;
            }
            case LGBBT_FORMAT_FINISH:
            {
                if (TRUE == L2_BbtWaitAllLBBTOpStageDone(LGBBT_FORMAT_FINISH))
                {
                    return TRUE; // return immediately.
                }

                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d BBT Format Local BBT Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bFormatLBBTDone;
}

/*==============================================================================
Func Name  : L2_BbtFormatGlobalBBT
Input      : NONE
Output     : NONE
Return Val :
Discription: Pre-Condition: 
             Global-Manager-init
             Conclusion: 
             if Erase-Fail -> Mask the TLun as a invalid GlobalTLun and Select a new GlobalTLun;
             when Erase-Finish -> All global-bbt-blks are erased or masked, and Glocal-Manager-Init-Status
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtFormatGlobalBBT(void)
{
    BOOL bFormatGBBTDone = FALSE;
    BBT_MANAGER *ptGBBTManager = &l_ptBBTManager[0];
    LOCAL MCU12_VAR_ATTR U8 s_ucGlobalTLun = 0;

    switch (ptGBBTManager->bsOpStage)
    {
        case LGBBT_FORMAT_INIT:
        {
            ptGBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk;
            ptGBBTManager->bsPln = START_PLN_IN_GBBT;
            ptGBBTManager->bsPage = INVALID_4F;
            ptGBBTManager->bsOpStage = LGBBT_FORMAT_ERS;
            break;
        }
        case LGBBT_FORMAT_ERS:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (FALSE == L2_FCMDQNotFull(s_ucGlobalTLun))
            {
                break;
            }
                           
            ptReqEntry = L2_FCMDQAllocReqEntry(s_ucGlobalTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;
            
            ptReqEntry->tFlashDesc.bsVirBlk = ptGBBTManager->bsBlk;
            ptReqEntry->tFlashDesc.bsPlnNum = ptGBBTManager->bsPln;  
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                            
            if (NULL != ptGBBTManager->pFlashStatus)
            {
                *ptGBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptGBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
            }
            
            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(s_ucGlobalTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
                                    
            ptGBBTManager->bsOpStage = LGBBT_FORMAT_ERS_WAIT;
            break;
        }
        case LGBBT_FORMAT_ERS_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *ptGBBTManager->pFlashStatus)
            {
                if (SUBSYSTEM_STATUS_FAIL != *ptGBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT InvalidStatus.\n", HAL_GetMcuId(), s_ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_FAIL == *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT Ers-Fail.\n", HAL_GetMcuId(), s_ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    L2_BbtMskGBbtTLun(s_ucGlobalTLun);
                    L2_BbtAddBbtBadBlkSinglePlane(s_ucGlobalTLun, ptGBBTManager->bsBlk, ptGBBTManager->bsPln, RT_BAD_BLK, ERASE_ERR);

                    if (FALSE == L2_BbtUpdateGBbtTLun(s_ucGlobalTLun) && 0 == ((ptGBBTManager->bsPln + 1) % PLN_PER_LUN))
                    {
                        l_tGBbtInfo.bsGBbtTLun = 0;
                        COM_MemZero((U32*)l_tGBbtInfo.aLunMskBitMap, BBT_TLUN_MSK_SIZE);

                        /*if all LUN's Blk1 erase fail, move to next blk as GBBT blk by Nina 2017-11-27 */
                        l_tGBbtInfo.bsGBbtBlk++;

                        s_ucGlobalTLun = 0;
                        ptGBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk;
                        ptGBBTManager->bsPln = 0;
                        ptGBBTManager->bsOpStage = LGBBT_FORMAT_ERS;
                        break;
                    }
                }
                else
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT Ers-Success.\n", HAL_GetMcuId(), s_ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    if (INVALID_2F == l_tGBbtInfo.bsGBbtTLun && 0 == ((ptGBBTManager->bsPln + 1) % PLN_PER_LUN))
                    {
                        l_tGBbtInfo.bsGBbtTLun = 0;
                        COM_MemZero((U32*)l_tGBbtInfo.aLunMskBitMap, BBT_TLUN_MSK_SIZE);
                    }
                }

                ptGBBTManager->bsPln = (ptGBBTManager->bsPln + 1) % PLN_PER_LUN;
                if (0 == ptGBBTManager->bsPln)
                {
                    ptGBBTManager->bsPln = START_PLN_IN_GBBT;
                    ptGBBTManager->bsOpStage = LGBBT_FORMAT_FINISH;
                }
                else
                {
                    ptGBBTManager->bsOpStage = LGBBT_FORMAT_ERS;
                }
            }
            break;
        }
        case LGBBT_FORMAT_FINISH:
        {
            s_ucGlobalTLun = (s_ucGlobalTLun + 1) % SUBSYSTEM_LUN_NUM;
            if (0 == s_ucGlobalTLun)
            {
                l_tGBbtInfo.bsFormatGBBT = TRUE;
                bFormatGBBTDone = TRUE;
            }

            ptGBBTManager->bsOpStage = LGBBT_FORMAT_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d TLun#%d BBT Format Global BBT Stage %d Error.\n", HAL_GetMcuId(), s_ucGlobalTLun, ptGBBTManager->bsOpStage);
            DBG_Getch();
        }
    }

    return bFormatGBBTDone;
}

/*==============================================================================
Func Name  : L2_BbtFormatBBT
Input      : U8 ucPu
Output     : NONE
Return Val :
Discription: Pre-Condition: 
             All-Local-Manager-Init & Global-Manager-init
             Conclusion: 
             All local-bbt-blks are erased and the global-bbt-blks are erased or masked.
             All-Local-Manager-Init-Status & Global-Manager-init-Status.
Usage      :
History    :
1. 2014.10.06 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL MCU12_DRAM_TEXT L2_BbtFormatBBT(void)
{
    BOOL bFormatBBTDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_ulFormatBBTStage = BBT_FORMAT_INIT;

    switch (s_ulFormatBBTStage)
    {
        case BBT_FORMAT_INIT:
        {
            L2_BbtGBbtMgrInit();
            L2_BbtLBbtMgrInit();
            s_ulFormatBBTStage = BBT_FORMAT_LOCAL_BBT;
            break;
        }
        case BBT_FORMAT_LOCAL_BBT:
        {
            if (TRUE == L2_BbtFormatLocalBBT())
            {
                if (l_tGBbtInfo.bsFormatGBBT)
                {
                    s_ulFormatBBTStage = BBT_FORMAT_FINISH;
                }
                else
                {
                    s_ulFormatBBTStage = BBT_FORMAT_GLOBAL_BBT;
                }
            }

            break;
        }
        case BBT_FORMAT_GLOBAL_BBT:
        {
            if (TRUE == L2_BbtFormatGlobalBBT())
            {
                s_ulFormatBBTStage = BBT_FORMAT_FINISH;
            }

            break;
        }
        case BBT_FORMAT_FINISH:
        {
            bFormatBBTDone = TRUE;
            s_ulFormatBBTStage = BBT_FORMAT_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Format BBT Stage %d Error.\n", HAL_GetMcuId(), s_ulFormatBBTStage);
            DBG_Getch();
        }
    }

    return bFormatBBTDone;
}

#define M_BAD_BLK_NUM 20
LOCAL U32 MCU1_DRAM_DATA aBadBlkList[17][M_BAD_BLK_NUM] = {
            //{100, 200, INVALID_4F},
       
            {INVALID_4F, INVALID_4F} 
};
LOCAL void MCU1_DRAM_TEXT L2_BbtAddBadBlkManual(void)
{
    U8 ucLun, ucBlkIndex, ucPln;

    for (ucLun = 0; ucLun < SUBSYSTEM_LUN_NUM; ucLun++)
    {
        if (aBadBlkList[ucLun][0] == INVALID_4F && aBadBlkList[ucLun][1]) break;

        for (ucBlkIndex = 0; ucBlkIndex < M_BAD_BLK_NUM; ucBlkIndex++)
        {
            if (aBadBlkList[ucLun][ucBlkIndex] == INVALID_4F) break;

            for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
            {
                L2_BbtSetGBbtBadBlkBit(ucLun, ucPln, aBadBlkList[ucLun][ucBlkIndex]);
            }
        }
    }

#ifdef HOST_SATA
    g_pSubSystemDevParamPage->AvailRsvdSpace = SUBSYSTEM_SUPERPU_NUM * RSVD_BLK_PER_LUN * LUN_NUM_PER_SUPERPU;
    g_pSubSystemDevParamPage->UsedRsvdBlockCnt = g_pSubSystemDevParamPage->EraseFailCnt;
    
    if(g_pSubSystemDevParamPage->AvailRsvdSpace >= g_pSubSystemDevParamPage->UsedRsvdBlockCnt)
       g_pSubSystemDevParamPage->AvailRsvdSpace -= g_pSubSystemDevParamPage->UsedRsvdBlockCnt;
#endif       
    
    return;
}
/*==============================================================================
Func Name  : L2_BbtEraseWholeDisk
Input      : U8 ucPu
Output     : NONE
Return Val :
Discription: Erase whole disk, set the bad blk bit when erase fail.
Usage      :
History    :
1. 2014.10.06 steven create
2. 2015.05.25 Jason refactoring
3. 2015.12.22 Nina modify
     support single-plane-erase and multi-plane-erase,
     and use multi-plane-erase for default to reduce LLF/SecurityErase/NVme Format cmd time
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtEraseWholeDisk(void)
{
    U8 ucTLun;
    U8 ucFormatWholeDiskType = FCMD_REQ_SUBTYPE_NORMAL;//FCMD_REQ_SUBTYPE_SINGLE;
    BOOL bEraseWholeDiskDone = FALSE;
    BBT_MANAGER *ptBBTManager;

#ifdef LIMITED_LUN_TO_ERASE
    U8 ucTLunGroups;
    U8 ucIndex;
    U8 ucGroupSize;
    U8 ucLunInGroup;
    LOCAL MCU12_VAR_ATTR U8 s_ucEraseGroupStage = 0;

    ucIndex = s_ucEraseGroupStage;
    ucTLunGroups = (SUBSYSTEM_LUN_NUM / LUN_GROUP_MAX) + ((SUBSYSTEM_LUN_NUM % LUN_GROUP_MAX) ? 1 : 0);

    if ((SUBSYSTEM_LUN_NUM % LUN_GROUP_MAX) && (ucIndex + 1) == ucTLunGroups)
    {
        ucGroupSize = (SUBSYSTEM_LUN_NUM % LUN_GROUP_MAX);
    }
    else
    {
        ucGroupSize = LUN_GROUP_MAX;
    }

    for (ucLunInGroup = 0; ucLunInGroup < ucGroupSize; ucLunInGroup++)
    {
        ucTLun = (ucIndex * LUN_GROUP_MAX) + ucLunInGroup;
#else
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
#endif
        ptBBTManager = &l_ptBBTManager[ucTLun + 1]; 

        switch (ptBBTManager->bsOpStage)
        {
            case ERS_WHOLE_DISK_INIT:
            {
                ptBBTManager->bsPln = 0;
                ptBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk + 1;
                ptBBTManager->bsOpStage = ERS_WHOLE_DISK_ERS;
                break;
            }
            case ERS_WHOLE_DISK_ERS:
            {
                FCMD_REQ_ENTRY *ptReqEntry;
#if 0
                BOOL bAllPlnBad = TRUE;
                U32  ulPln;

                for (ulPln = 0; ulPln < PLN_PER_LUN; ulPln++)
                {
                    if (TRUE == L2_BbtIsGBbtBadBlockSinglePlane(ucTLun, ptBBTManager->bsBlk, ulPln))
                    {
                        if (FALSE == L2_BbtIsLBbtBadBlockSinglePlane(ucTLun, ptBBTManager->bsBlk, ulPln))
                        {
                            DBG_Printf("L2_BbtEraseWholeDisk: inconsistency between GBBT and LBBT, LUN %d block %d plane %d GBBT bit %d LBBT bit %d\n",
                                ucTLun, ptBBTManager->bsBlk, ulPln, 1, 0);
                            DBG_Getch();
                        }
                    }
                    else
                    {
                        bAllPlnBad = FALSE;
                        break;
                    }
                }
                if (TRUE == bAllPlnBad)
                {
                    if (NULL != ptBBTManager->pFlashStatus)
                    {
                        *ptBBTManager->pFlashStatus = SUBSYSTEM_STATUS_SUCCESS;
                    }
                    ptBBTManager->bsOpStage = ERS_WHOLE_DISK_ERS_WAIT;
                    break;
                }
#endif
                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
                
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
                ptReqEntry->bsReqSubType = ucFormatWholeDiskType;
                ptReqEntry->bsTableReq = TRUE;

                ptReqEntry->tFlashDesc.bsVirBlk = ptBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsPlnNum = ptBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                                
                if (NULL != ptBBTManager->pFlashStatus)
                {
                    *ptBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                    ptReqEntry->ulReqStsAddr= (U32)ptBBTManager->pFlashStatus;
                    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                }
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);           

                ptBBTManager->bsOpStage = ERS_WHOLE_DISK_ERS_WAIT;
                break;
            }
            case ERS_WHOLE_DISK_ERS_WAIT:
            {
                if (SUBSYSTEM_STATUS_PENDING != *ptBBTManager->pFlashStatus)
                {
                    if (SUBSYSTEM_STATUS_SUCCESS == *ptBBTManager->pFlashStatus)
                    {
                        // multi plane erase success, proceed to the next block
                        ptBBTManager->bsBlk = (ptBBTManager->bsBlk + 1) % BBT_BLK_PER_PLN;
                        if (LBBT_BLK == ptBBTManager->bsBlk)
                        {
                            // all blocks have been erased, jumped to the finish stage
                            ptBBTManager->bsBlk = LBBT_BLK;
                            ptBBTManager->bsPln = START_PLN_IN_LBBT;
                            ptBBTManager->bsOpStage = ERS_WHOLE_DISK_FINISH;
                        }
                        else
                        {
                            // proceed erasing the next block
                            ptBBTManager->bsOpStage = ERS_WHOLE_DISK_ERS;
                        }
                    }
                    else if (SUBSYSTEM_STATUS_FAIL == *ptBBTManager->pFlashStatus)
                    {
                        // multi plane erase fail, conduct single plane erase

                        DBG_Printf("MCU#%d TLun#%d Blk#%d BBT multi plane erase fail\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsBlk);

                        // reset the plane number
                        ptBBTManager->bsPln = 0;

                        // conduct the single plane erase
                        ptBBTManager->bsOpStage = ERS_WHOLE_DISK_READ_ERS_FAIL_STATUS;
                        ptBBTManager->bsDChk = FALSE;
                    }
                    else
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Whole-Disk InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsPln, ptBBTManager->bsBlk);
                        DBG_Getch();
                    }
                }
                break;
            }
            case ERS_WHOLE_DISK_READ_ERS_FAIL_STATUS:
            {
                // conduct single plane erase on the current block

                FCMD_REQ_ENTRY *ptReqEntry;
#if 0
                if (TRUE == L2_BbtIsGBbtBadBlockSinglePlane(ucTLun, ptBBTManager->bsBlk, ptBBTManager->bsPln))
                {
                    if (FALSE == L2_BbtIsLBbtBadBlockSinglePlane(ucTLun, ptBBTManager->bsBlk, ptBBTManager->bsPln))
                    {
                        DBG_Printf("L2_BbtEraseWholeDisk: inconsistency between GBBT and LBBT, LUN %d block %d plane %d GBBT bit %d LBBT bit %d\n",
                            ucTLun, ptBBTManager->bsBlk, ptBBTManager->bsPln, 1, 0);
                        DBG_Getch();
                    }

                    if (NULL != ptBBTManager->pFlashStatus)
                    {
                        *ptBBTManager->pFlashStatus = SUBSYSTEM_STATUS_SUCCESS;
                    }
                    ptBBTManager->bsDChk = TRUE;
                    ptBBTManager->bsOpStage = ERS_WHOLE_DISK_READ_STATUS_WAIT;
                    break;
                }
#endif
                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
                           
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_RDSTS;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                
                ptReqEntry->tFlashDesc.bsVirBlk = ptBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsPlnNum = ptBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                                
                if (NULL != ptBBTManager->pFlashStatus)
                {
                    *ptBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                    ptReqEntry->ulReqStsAddr= (U32)ptBBTManager->pFlashStatus;
                    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                }
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);  

                ptBBTManager->bsOpStage = ERS_WHOLE_DISK_READ_STATUS_WAIT;

                break;
            }
            case ERS_WHOLE_DISK_READ_STATUS_WAIT:
            {
                if (SUBSYSTEM_STATUS_PENDING != *ptBBTManager->pFlashStatus)
                {
                    if (SUBSYSTEM_STATUS_SUCCESS == *ptBBTManager->pFlashStatus)
                    {
                        // single plane erase success, do nothing
                        ;
                    }
                    else if (SUBSYSTEM_STATUS_FAIL == *ptBBTManager->pFlashStatus)
                    {
                        // single plane erase fail, add the plane block to BBT
                        ptBBTManager->bsDChk = TRUE;
                        L2_BbtAddBbtBadBlkSinglePlane(ucTLun, ptBBTManager->bsBlk, ptBBTManager->bsPln, RT_BAD_BLK, ERASE_ERR);
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT single plane erase fail\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsPln, ptBBTManager->bsBlk);
                    }
                    else
                    {
                        // invalid status error
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Format-Whole-Disk InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsPln, ptBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    // proceed to the next plane, if all plane blocks have been erased,
                    // proceed the next block

                    // calculate the next plane
                    ptBBTManager->bsPln = (ptBBTManager->bsPln + 1) % PLN_PER_LUN;

                    if(ptBBTManager->bsPln == 0)
                    {
                        if (FALSE == ptBBTManager->bsDChk)
                        {
                            DBG_Printf("MCU#%d TLun#%d Blk#%d BBT single plane erase status check fail\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsBlk);
                            DBG_Getch();
                        }
                        // all planes have been erased, proceed to the next block
                        ptBBTManager->bsBlk = (ptBBTManager->bsBlk + 1) % BBT_BLK_PER_PLN;
                        if (LBBT_BLK == ptBBTManager->bsBlk)
                        {
                            // all blocks have been erased, jumped to the finish stage
                            ptBBTManager->bsBlk = LBBT_BLK;
                            ptBBTManager->bsPln = START_PLN_IN_LBBT;
                            ptBBTManager->bsOpStage = ERS_WHOLE_DISK_FINISH;
                        }
                        else
                        {
                            // proceed erasing the next block
                            ptBBTManager->bsOpStage = ERS_WHOLE_DISK_ERS;
                        }
                    }
                    else
                    {
                        // proceed to the next plane
                        ptBBTManager->bsOpStage = ERS_WHOLE_DISK_READ_ERS_FAIL_STATUS;
                    }
                }
                break;
            }
            case ERS_WHOLE_DISK_FINISH:
            {
#ifdef LIMITED_LUN_TO_ERASE
                if (TRUE == L2_BbtWaitAllBlkEraseOpStageDone(ERS_WHOLE_DISK_FINISH, ucIndex, ucGroupSize))
                {
                    if ((ucIndex + 1) == ucTLunGroups)
                    {
                        s_ucEraseGroupStage = 0;
                        L2_BbtAddBadBlkManual();
                        return TRUE;
                    }
                    else
                    {
                        s_ucEraseGroupStage++;
                        return FALSE;
                    }
                }
#else
                if (TRUE == L2_BbtWaitAllLBBTOpStageDone(ERS_WHOLE_DISK_FINISH))
                {
                    L2_BbtAddBadBlkManual();
                    return TRUE; 
                }
#endif
                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d BBT Erase Whole Disk Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bEraseWholeDiskDone;
}

/*==============================================================================
Func Name  : L2_BbtFindLocalTarget
Input      : NONE
Output     : NONE
Return Val :
Discription: init the bbt manager.
             BBT_NEW_MARK is Mis-Match:  Version Bug, Reset BBT Manger -> Format all Bbt -> Finish
             All Local Bbt is Empty-Pg:  Havn't saved, Reset LBBT Manger -> Format all Bbt -> Finish
             Find out a valid Local Bbt: Normal, Find global bbt -> Finish/ Reset GBBT Manager -> Format Global BBT-> Finish
             Local Bbt Read retry fail:  Fail, Getch()
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtFindLocalTarget(void)
{
    U8 ucTLun;
    BOOL bFindLBbtDone = FALSE;
    BBT_MANAGER *ptLBBTManager;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptLBBTManager = &l_ptBBTManager[ucTLun+1];

        switch (ptLBBTManager->bsOpStage)
        {
            case LBBT_FIND_INIT:
            {
                ptLBBTManager->bsPage = 0;
                ptLBBTManager->bsBlk = LBBT_BLK;
                ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                ptLBBTManager->bsOpStage = LBBT_FIND_READ_RED;
                //break;
            }
            case LBBT_FIND_READ_RED:
            {
                FCMD_REQ_ENTRY *ptReqEntry;               

                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
                            
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsVirPage = ptLBBTManager->bsPage;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
                
                ptReqEntry->tFlashDesc.bsRdRedOnly = TRUE;
                ptReqEntry->ulSpareAddr = (U32)ptLBBTManager->pSpare;                

                *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
                
                ptLBBTManager->bsOpStage = LBBT_FIND_READ_WAIT;
                break;
            }
            case LBBT_FIND_READ_WAIT:
            {
                if (SUBSYSTEM_STATUS_SUCCESS == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RECC == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *ptLBBTManager->pFlashStatus)
                {
                    l_tGBbtInfo.ulBbtMark = ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark;
					if (BBT_NEW_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_RDT_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_NEW_V2_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Local Target Bbt-Mark-Miss-Match.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        L2_BbtGBbtInfoInit(FALSE);  // --> Format BBT                      

                        return TRUE;
                    }

                    // update the GBbtInfo by the maxBbtSn according the spare data
                    if (l_tGBbtInfo.ulMaxBbtSn < ptLBBTManager->pSpare->m_tGBbtInfo.ulMaxBbtSn)
                    {  
                        L2_BbtCpySpareToGBbt(ptLBBTManager->pSpare);
                    }

                    ptLBBTManager->bsPage++;
                    if (BBT_PG_PER_BLK <= ptLBBTManager->bsPage) // CheckFull
                    {
                        DBG_Printf("MCU#%d TLun#%d BBT Find Local BBT Fail.\n", HAL_GetMcuId(), ucTLun);
                        DBG_Getch();
                    }

                    ptLBBTManager->bsOpStage = LBBT_FIND_READ_RED;
                }
                else if (SUBSYSTEM_STATUS_EMPTY_PG == *ptLBBTManager->pFlashStatus)
                {
                    if (0 == ptLBBTManager->bsPage)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Local Target First-Page-Empty.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        ptLBBTManager->bsPage = INVALID_4F;
                    }
                    else
                    {
                        ptLBBTManager->bsPage--;
                    }
                    ptLBBTManager->bsOpStage = LBBT_FIND_FINISH;
                }
                else if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Local Target Read-Red Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                    DBG_Getch();
                }
                else
                {
                    ; // wait flash status...
                }
                break;
            }
            case LBBT_FIND_FINISH:
            {
                if (TRUE == L2_BbtWaitAllLBBTOpStageDone(LBBT_FIND_FINISH))
                {
                    return TRUE; // return immediately.
                }

                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d BBT Find Local Target Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bFindLBbtDone;
}

/*==============================================================================
Func Name  : L2_BbtFindGlobalTarget
Input      : U8 ucPu
Output     : NONE
Return Val :
Discription: find global bbt, update bbt manager
             Read the target bbt ocurred GBbtInfo Mis-Math: Reset GBBT Manager -> Format GBBT -> Finish
             Read the target bbt occured empty-page: Getch() or current page > the target bbt page -> Format Global BBT            
             Read the target bbt retry fail: Msk old GlobalTLun and Select a new GlobalTLun, Reset GBBT Manager -> Format Global BBT -> Finish
Usage      : D-Check the Global-BBT-Page, read it until empty-page, if the current page's red-mis-match, or is bigger then the Target Page, or retry-fail, we need to format GBBT
History    :
1. 2014.10.06 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtFindGlobalTarget(void)
{
    U8 ucGlobalTLun = l_tGBbtInfo.bsGBbtTLun;
    BOOL bFindGlobalDone = FALSE;
    BBT_MANAGER *ptGBBTManager = &l_ptBBTManager[0];

    switch (ptGBBTManager->bsOpStage)
    {
        case GBBT_FIND_INIT:
        {
            ptGBBTManager->bsPage = l_tGBbtInfo.bsGBbtPage; // start to read, until empty-page
            ptGBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk;
            ptGBBTManager->bsPln = START_PLN_IN_GBBT;
            ptGBBTManager->bsOpStage = GBBT_FIND_READ_RED;
            break;
        }
        case GBBT_FIND_READ_RED:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (FALSE == L2_FCMDQNotFull(ucGlobalTLun))
            {
                break;
            }
        
            ptReqEntry = L2_FCMDQAllocReqEntry(ucGlobalTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;
                
            ptReqEntry->tFlashDesc.bsVirBlk = ptGBBTManager->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = (INVALID_4F != ptGBBTManager->bsPage) ? ptGBBTManager->bsPage : 0;
            ptReqEntry->tFlashDesc.bsPlnNum = ptGBBTManager->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            
            ptReqEntry->tFlashDesc.bsRdRedOnly = TRUE;
            ptReqEntry->ulSpareAddr = (U32)ptGBBTManager->pSpare;                

            *ptGBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptGBBTManager->pFlashStatus;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
            
            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucGlobalTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptGBBTManager->bsOpStage = GBBT_FIND_READ_WAIT;
            break;
        }
        case GBBT_FIND_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_SUCCESS == *ptGBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RECC == *ptGBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *ptGBBTManager->pFlashStatus)
            {
                if (TRUE != L2_BbtCmpSpareWithGBbt(ptGBBTManager->pSpare))
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Global Target Red-Mis-Match.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                    L2_BbtGBbtMgrInit();

                    L2_BbtUpdateGBbtTLun(ucGlobalTLun);

                    return TRUE;
                }
                
                ptGBBTManager->bsPage++;
                if (BBT_PG_PER_BLK <= ptGBBTManager->bsPage)
                {
                    ptGBBTManager->bsPage--;
                    ptGBBTManager->bsOpStage = GBBT_FIND_FINISH;
                }
                else
                { 
                    ptGBBTManager->bsOpStage = GBBT_FIND_READ_RED;
                }                
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == *ptGBBTManager->pFlashStatus)
            {
                // only format global bbt, or the abnormal-shut-down occured between save-global-bbt and save-local-bbt. ==> need to rebuild bbt
                if ((l_tGBbtInfo.bsGBbtPage == ptGBBTManager->bsPage) || (l_tGBbtInfo.bsGBbtPage + 1 < ptGBBTManager->bsPage))
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Global Target GBbt-Invalid.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                    L2_BbtGBbtMgrInit();

                    L2_BbtUpdateGBbtTLun(ucGlobalTLun);
                    
                    return TRUE;
                }
                            
                ptGBBTManager->bsPage--;
                ptGBBTManager->bsOpStage = GBBT_FIND_FINISH;
            }
            else if (SUBSYSTEM_STATUS_FAIL == *ptGBBTManager->pFlashStatus)
            {
                DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Find Global Target Read-Retry-Fail.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                L2_BbtGBbtMgrInit();

                L2_BbtMskGBbtTLun(ucGlobalTLun);                
                L2_BbtUpdateGBbtTLun(ucGlobalTLun);

                return TRUE;
            }
            else
            {
                ; // wait flash status...
            }
            break;
        }
        case GBBT_FIND_FINISH:
        {
            bFindGlobalDone = TRUE;
            ptGBBTManager->bsOpStage = GBBT_FIND_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Find Global Target Stage %d Error.\n", HAL_GetMcuId(), ptGBBTManager->bsOpStage);
            DBG_Getch();
        }
    }

    return bFindGlobalDone;
}

/*==============================================================================
Func Name  : L2_BbtFindTarget
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: find out the target of local&global-bbt-position.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT L2_BbtFindTarget(void)
{
    BOOL bBbtFindTargetDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_ulBbtFindTargetStage = BBT_FIND_INIT;

    switch (s_ulBbtFindTargetStage)
    {
        case BBT_FIND_INIT:
        {
            /* Don't reset GBBT again,because format GBBT stage may also add bad blk to GBBT,update by Nina 2017-12-18 */
            L2_BbtManagerInit(FALSE);

            s_ulBbtFindTargetStage = BBT_FIND_LOCAL_BBT;
            break;
        }
        case BBT_FIND_LOCAL_BBT:
        {
            if (TRUE == L2_BbtFindLocalTarget())
            {                
                if (TRUE == L2_BbtIsGBbtValid())
                {   
                    if (TRUE == l_tGBbtInfo.bsFormatGBBT)
                    {
                        /* if GBBT is already formatted, reset again */
                        L2_BbtGBbtInfoInit(FALSE);                        
                        s_ulBbtFindTargetStage = BBT_FIND_FINISH;
                    }
                    else
                    {
                        s_ulBbtFindTargetStage = BBT_FIND_GLOBAL_BBT;
                    }
                }
                else
                {   // old version or empty page -> Format Local & Global BBT
                    s_ulBbtFindTargetStage = BBT_FIND_FORMAT_BBT;
                }  
            }

            break;
        }
        case BBT_FIND_GLOBAL_BBT:
        {
            if (TRUE == L2_BbtFindGlobalTarget())
            {
                if (TRUE == L2_BbtIsGBbtValid())
                {
                    s_ulBbtFindTargetStage = BBT_FIND_FINISH;
                }
                else
                {   // GBbtInfo mis-match or retry fail -> Format Global BBT
                    s_ulBbtFindTargetStage = BBT_FIND_FORMAT_GBBT;
                }                
            }

            break;
        }        
        case BBT_FIND_FORMAT_BBT:
        {
            if (TRUE == L2_BbtFormatBBT())
            {
                s_ulBbtFindTargetStage = BBT_FIND_FINISH;
            }

            break;
        }
        case BBT_FIND_FORMAT_GBBT:
        {
            if (TRUE == L2_BbtFormatGlobalBBT())
            {
                s_ulBbtFindTargetStage = BBT_FIND_FINISH;
            }

            break;
        }
        case BBT_FIND_FINISH:
        {
            bBbtFindTargetDone = TRUE;
            s_ulBbtFindTargetStage = BBT_FIND_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Find Target Stage %d Error.\n", HAL_GetMcuId(), s_ulBbtFindTargetStage);
            DBG_Getch();
        }
    }

    return bBbtFindTargetDone;
}

/*==============================================================================
Func Name  : L2_BbtInit
Input      : NONE
Output     : NONE
Return Val :
Discription: BBT init, find out the position of the global bbt and each local bbt.
Usage      : LLF_Init or Boot_Init
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtInit(void)
{
    while (TRUE != L2_BbtFormatBBT())
    {
        //L3_Scheduler();
    }

    while (FALSE == L2_BbtFindTarget())
    {
        //L3_Scheduler();
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtReadIDB
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: read the factory bad blk from the specified position of the flash-chip.
Usage      :
History    :
1. 2014.10.13 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_BbtReadIDB(void)
{
    U8 ucTLun, ucPu, ucBLun, ucPlane;
    U16 usBlk;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ucPu = L3_GET_PU(ucTLun);
        ucBLun = L3_GET_LUN_IN_PU(ucTLun);

        for (usBlk = GBBT_BLK; usBlk < BBT_BLK_PER_PLN; usBlk++)
        {
            for (ucPlane = 0; ucPlane < PLN_PER_LUN; ++ucPlane)
            {
                if (TRUE == HAL_FlashIsBlockBad(ucPu, ucBLun, ucPlane, usBlk))
                {
                    if (GBBT_BLK < usBlk)
                    {
                        // Table & Data Blk
                        //L2_BbtAddBbtBadBlk(ucTLun, usBlk);
                        L2_BbtAddBbtBadBlkSinglePlane(ucTLun, usBlk, ucPlane, IDB_BAD_BLK, RSVD);
                    }
                    else
                    {
                        // Global BBT Blk
                        DBG_Printf("MCU#%d TLun#%d Blk#%d BBT Read IDB Bad Blk.\n", HAL_GetMcuId(), ucTLun, usBlk);

                        L2_BbtMskGBbtTLun(ucTLun);
                        L2_BbtUpdateGBbtTLun(ucTLun);
                    }
                    break;
                }
            }
        }
    }

    L2_BbtPrintAllBbt();

    return;
}

LOCAL U8 MCU12_DRAM_TEXT L2_BbtGetPbnBindingTableConstructBitmap(U16 usBlock, U8 ucPlane)
{
    U32 ulBitPosition = (usBlock * PLN_PER_LUN) + ucPlane;
    U32 ulBytePosition = ulBitPosition / 8;
    U32 ulBitOffsetWithinByte = ulBitPosition % 8;
    U8* pucPbnBindingTableConstructBitmap = (U8*)l_ulPbnBindingTableConstructBitmap;

    return ((pucPbnBindingTableConstructBitmap[ulBytePosition] & (1 << ulBitOffsetWithinByte)) == 0) ? FALSE : TRUE;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtSetPbnBindingTableConstructBitmap(U16 usBlock, U8 ucPlane)
{
    U32 ulBitPosition = (usBlock * PLN_PER_LUN) + ucPlane;
    U32 ulBytePosition = ulBitPosition / 8;
    U32 ulBitOffsetWithinByte = ulBitPosition % 8;
    U8* pucPbnBindingTableConstructBitmap = (U8*)l_ulPbnBindingTableConstructBitmap;

    if(L2_BbtGetPbnBindingTableConstructBitmap(usBlock, ucPlane) == TRUE)
    {
        DBG_Printf("PBN binding table construct bitmap error\n");
        DBG_Getch();
    }

    pucPbnBindingTableConstructBitmap[ulBytePosition] |= (1 << ulBitOffsetWithinByte);

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_BbtSetPbnBindingTable(U8 ucTlun, U16 usPbn, U8 ucPlane, U16 usPlaneBlock)
{
    // input check
    if(ucTlun >= SUBSYSTEM_LUN_NUM)
    {
        DBG_Printf("Tlun error\n");
        DBG_Getch();
    }
    else if(usPbn >= BBT_BLK_PER_PLN)
    {
        DBG_Printf("PBN error\n");
        DBG_Getch();
    }
    else if(usPlaneBlock >= BBT_BLK_PER_PLN)
    {
        DBG_Printf("plane block error\n");
        DBG_Getch();
    }
    else if(ucPlane >= PLN_PER_LUN)
    {
        DBG_Printf("plane error\n");
        DBG_Getch();
    }

    U16* pusTargetBindingBlockAddress =(U16*)(
            g_pMCU12MiscInfo->ulPbnBindingTable // starting address
            + (ucTlun * (BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16))) // LUN offset
            + (usPbn * PLN_PER_LUN * sizeof(U16)) // PBN offset
            + (ucPlane * sizeof(U16)) // plane offset 
            );
    *pusTargetBindingBlockAddress = usPlaneBlock;

    return;
}

GLOBAL U16 L2_BbtGetPbnBindingTable(U8 ucTlun, U16 usPbn, U8 ucPlane)
{
    // this function is provided to L3 as a public interface

    // input check
    if(ucTlun >= SUBSYSTEM_LUN_NUM)
    {
        DBG_Printf("Tlun %d error\n", ucTlun);
        DBG_Getch();
    }
    else if(usPbn >= BBT_BLK_PER_PLN)
    {
        DBG_Printf("Tlun %d PBN %d error\n", ucTlun, usPbn);
        DBG_Getch();
    }
    else if(ucPlane >= PLN_PER_LUN)
    {
        DBG_Printf("plane %d error\n", ucPlane);
        DBG_Getch();
    }

    if (L2_BbtIsPbnBindingTableEnable() == TRUE)
    {
        return *(U16*)(g_pMCU12MiscInfo->ulPbnBindingTable + ((ucTlun*BBT_BLK_PER_PLN + usPbn) << (PLN_PER_LUN_BITS + 1)) + (ucPlane << 1));
    }
    else
    {
        return usPbn;
    }
}

GLOBAL void MCU12_DRAM_TEXT L2_BbtPbnBindingTableCheck(void)
{
    U8 ucTlun, ucPlane;
    U16 usPlaneBlock;

    // check if the PBN binding table is ready
    if(L2_BbtIsPbnBindingTableEnable() == FALSE)
    {
        DBG_Printf("PBN binding table not ready\n");
        DBG_Getch();
    }

    // check PBN 0 and PBN 1 for each LUN, due to the current design
    // of the BBT, PBN 0 and PBN 1 must map to plane block 0 and block 1
    // respectively
    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++) 
        {
            if(L2_BbtGetPbnBindingTable(ucTlun, 0, ucPlane) != 0)
            {
                DBG_Printf("PBN 0 mapping error\n");
                DBG_Getch();
            }

            if(L2_BbtGetPbnBindingTable(ucTlun, 1, ucPlane) != 1)
            {
                DBG_Printf("PBN 1 mapping error\n");
                DBG_Getch();
            }
        }
    }

    // check for duplicate plane block
    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++) 
        {
            for(usPlaneBlock = 0; usPlaneBlock < BBT_BLK_PER_PLN; usPlaneBlock++)
            {
                U16 usPbn;
                U8 ucPlaneBlockFound = FALSE;

                for(usPbn = 0; usPbn < BBT_BLK_PER_PLN; usPbn++)
                {
                    if(L2_BbtGetPbnBindingTable(ucTlun, usPbn, ucPlane) == usPlaneBlock)
                    {
                        if(ucPlaneBlockFound == FALSE)
                        {
                            ucPlaneBlockFound = TRUE;
                        }
                        else
                        {
                            DBG_Printf("duplicate plane block\n");
                            DBG_Getch();
                        }
                    }
                }

                if(ucPlaneBlockFound == FALSE)
                {
                    DBG_Printf("plane block not found\n");
                    DBG_Getch();
                }
            }
        }
    }

    return;
}

GLOBAL U8 MCU12_DRAM_TEXT L2_BbtPbnBindingTableExistenceCheck(BOOL* pHardBindingTableExist)
{
    U8 ucTlun, ucPlane;
    U16 usPlaneBlock;
    U8 ucIsPbnBindingTableExist;

    ucIsPbnBindingTableExist = TRUE;

    *pHardBindingTableExist = TRUE;

    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM && ucIsPbnBindingTableExist == TRUE; ucTlun++)
    {
        for(ucPlane = 0; ucPlane < PLN_PER_LUN && ucIsPbnBindingTableExist == TRUE; ucPlane++) 
        {
            for(usPlaneBlock = 0; usPlaneBlock < BBT_BLK_PER_PLN && ucIsPbnBindingTableExist == TRUE; usPlaneBlock++)
            {
                U16 usPbn;
                U8 ucPlaneBlockFound = FALSE;

                for(usPbn = 0; usPbn < BBT_BLK_PER_PLN && ucIsPbnBindingTableExist == TRUE; usPbn++)
                {
                    U16* pusTargetBindingBlockAddress =(U16*)(
                            g_pMCU12MiscInfo->ulPbnBindingTable // starting address
                            + (ucTlun * (BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16))) // LUN offset
                            + (usPbn * PLN_PER_LUN * sizeof(U16)) // PBN offset
                            + (ucPlane * sizeof(U16)) // plane offset 
                            );

                    if(*pusTargetBindingBlockAddress == usPlaneBlock)
                    {
                        if(ucPlaneBlockFound == FALSE)
                        {
                            ucPlaneBlockFound = TRUE;
                            if(usPlaneBlock != usPbn)
                            {
                                *pHardBindingTableExist = FALSE;
                            }
                        }
                        else
                        {
                            ucIsPbnBindingTableExist = FALSE;
                        }
                    }
                }

                if(ucPlaneBlockFound == FALSE)
                {
                    ucIsPbnBindingTableExist = FALSE;
                }
            }
        }
    }

    return ucIsPbnBindingTableExist;
}

extern BOOL HAL_GetPbnBindingTableEnFlag(void);
LOCAL void MCU12_DRAM_TEXT L2_BbtPbnBindingTableLlfCheck(void)
{
    U8 ucTlun, ucPlane;
    U16 usPlaneBlock;

    // check if the PBN binding table is ready
    if(L2_BbtIsPbnBindingTableEnable() == FALSE)
    {
        DBG_Printf("PBN binding table not ready\n");
        DBG_Getch();
    }

    // check PBN 0 and PBN 1 for each LUN, due to the current design
    // of the BBT, PBN 0 and PBN 1 must map to plane block 0 and block 1
    // respectively
    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++) 
        {
            if(L2_BbtGetPbnBindingTable(ucTlun, 0, ucPlane) != 0)
            {
                DBG_Printf("PBN 0 mapping error\n");
                DBG_Getch();
            }

            if(L2_BbtGetPbnBindingTable(ucTlun, 1, ucPlane) != 1)
            {
                DBG_Printf("PBN 1 mapping error\n");
                DBG_Getch();
            }
        }
    }

    // check for duplicate plane block
    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++) 
        {
            for(usPlaneBlock = 0; usPlaneBlock < BBT_BLK_PER_PLN; usPlaneBlock++)
            {
                U16 usPbn;
                U8 ucPlaneBlockFound = FALSE;

                for(usPbn = 0; usPbn < BBT_BLK_PER_PLN; usPbn++)
                {
                    if(L2_BbtGetPbnBindingTable(ucTlun, usPbn, ucPlane) == usPlaneBlock)
                    {
                        if(ucPlaneBlockFound == FALSE)
                        {
                            ucPlaneBlockFound = TRUE;
                        }
                        else
                        {
                            DBG_Printf("duplicate plane block\n");
                            DBG_Getch();
                        }
                    }
                }

                if(ucPlaneBlockFound == FALSE)
                {
                    DBG_Printf("plane block not found\n");
                    DBG_Getch();
                }
            }
        }
    }

    if(HAL_GetPbnBindingTableEnFlag() == TRUE)
    {
        for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
        {
            U16 usBadPlaneBlockCount[PLN_PER_LUN];
            U16 usMaxBadPlaneBlockCount;
            U8 ucPlane;
            U16 usPbn;
            U16 usPlaneBlock;
            U32 i;
            U16 usGoodPhysicalBlockCount;

            for(i = 0; i < PLN_PER_LUN; i++)
            {
                usBadPlaneBlockCount[i] = 0;
            }

            for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
            {
                for(usPlaneBlock = 0; usPlaneBlock < BBT_BLK_PER_PLN; usPlaneBlock++)
                {
                    if(L2_BbtIsGBbtBadBlockSinglePlane(ucTlun, usPlaneBlock, ucPlane) == TRUE)
                    {
                        usBadPlaneBlockCount[ucPlane]++;
                    }
                }
            }

            usMaxBadPlaneBlockCount = 0;
            for(i = 0; i < PLN_PER_LUN; i++)
            {
                if(usBadPlaneBlockCount[i] > usMaxBadPlaneBlockCount)
                {
                    usMaxBadPlaneBlockCount = usBadPlaneBlockCount[i];
                }
            }

            usGoodPhysicalBlockCount = BBT_BLK_PER_PLN - usMaxBadPlaneBlockCount;

            for(usPbn = 0; usPbn < usGoodPhysicalBlockCount; usPbn++)
            {
                for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
                {
                    U16 usTargetPlaneBlock;

                    usTargetPlaneBlock = L2_BbtGetPbnBindingTable(ucTlun, usPbn, ucPlane);

                    if(L2_BbtIsGBbtBadBlockSinglePlane(ucTlun, usTargetPlaneBlock, ucPlane) == TRUE)
                    {
                        DBG_Printf("good physical block mapping error\n");
                        DBG_Getch();
                    }
                }
            }

            for(; usPbn < BBT_BLK_PER_PLN; usPbn++)
            {
                U8 ucBadPlaneBlockFound = FALSE;

                for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
                {
                    U16 usTargetPlaneBlock;

                    usTargetPlaneBlock = L2_BbtGetPbnBindingTable(ucTlun, usPbn, ucPlane);

                    if(L2_BbtIsGBbtBadBlockSinglePlane(ucTlun, usTargetPlaneBlock, ucPlane) == TRUE)
                    {
                        ucBadPlaneBlockFound = TRUE;
                    }
                }

                if(ucBadPlaneBlockFound == FALSE)
                {
                    DBG_Printf("bad physical block mapping error\n");
                    DBG_Getch();
                }
            }
        }
    }

    return;
}

// Sean Gao 20160519
// this function constructs the PBN binding table from BBT for all LUNs
LOCAL void MCU12_DRAM_TEXT L2_BbtBuildPbnBindingTable(void)
{
    U8 ucTlun;
    U32 i, j;
    U16 usCurrentPlaneBlockPosition[PLN_PER_LUN];
    U16 usCurrentPbn;
    U8 ucAllPlaneValid;
    U8* pCurrentBytePosition;

    if (HAL_GetPbnBindingTableEnFlag() == TRUE)
    {
        for (ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
        {
            // first initialize the PBN binding table construct bitmap,
            // which is used to indicate whether a plane block has been mapped to a PBN or not
            for (i = 0; i < ((BBT_BLK_PER_PLN * PLN_PER_LUN / 8) + 1); i++)
            {
                pCurrentBytePosition = (U8*)(l_ulPbnBindingTableConstructBitmap + i);
                *pCurrentBytePosition = 0;
            }

            // start constructing the binding of good physical blocks, that is,
            // blocks with no bad plane blocks

            // initialize the current block position for each plane
            for (i = 0; i < PLN_PER_LUN; i++)
            {
                usCurrentPlaneBlockPosition[i] = 0;
            }

            // initialize the current PBN, which is 0
            usCurrentPbn = 0;

            while (usCurrentPbn < BBT_BLK_PER_PLN)
            {
                ucAllPlaneValid = TRUE;

                // find good plane blocks for all planes
                for (i = 0; (i < PLN_PER_LUN) && (ucAllPlaneValid == TRUE); i++)
                {
                    if (usCurrentPlaneBlockPosition[i] < BBT_BLK_PER_PLN)
                    {
                        while (L2_BbtIsGBbtBadBlockSinglePlane(ucTlun, usCurrentPlaneBlockPosition[i], i) == TRUE)
                        {
                            // the current plane block is bad, proceed to the next one
                            usCurrentPlaneBlockPosition[i]++;

                            // check if we've searched the entire range of plane blocks
                            if (usCurrentPlaneBlockPosition[i] == BBT_BLK_PER_PLN)
                            {
                                /* fail to find a good block in the current plane, mark ucAllPlaneValid as FALSE */
                                ucAllPlaneValid = FALSE;
                                break;
                            }
                        } 
                    }
                    else
                    {
                        ucAllPlaneValid = FALSE;
                    }
                } 

                // take corresponding actions based on ucAllPlaneValid
                if(ucAllPlaneValid == TRUE)
                {
                    // we've found good plane blocks on all planes

                    // mark these plane blocks as mapped
                    for (i = 0; i < PLN_PER_LUN; i++)
                    {
                        L2_BbtSetPbnBindingTableConstructBitmap(usCurrentPlaneBlockPosition[i], i);
                    }

                    // record the PBN binding
                    //FIRMWARE_LogInfo("MCU%d LUN %d PBN %d\n", HAL_GetMcuId(), ucTlun, usCurrentPbn);
                    for (i = 0; i < PLN_PER_LUN; i++)
                    {
                        L2_BbtSetPbnBindingTable(ucTlun, usCurrentPbn, i, usCurrentPlaneBlockPosition[i]);
                        //FIRMWARE_LogInfo("MCU%d LUN %d plane %d block %d\n", HAL_GetMcuId(), ucTlun, i, usCurrentPlaneBlockPosition[i]);
                    }

                    // increase current plane block positions for all planes
                    for (i = 0; i < PLN_PER_LUN; i++)
                    {
                        usCurrentPlaneBlockPosition[i]++;
                    }

                    usCurrentPbn++;
                }
                else
                {
                    /* fail to find a good plane block on at least 1 plane, end the process */
                    break;
                }
            }

            //FIRMWARE_LogInfo("MCU%d LUN %d total good physical block number %d\n", HAL_GetMcuId(), ucTlun, usCurrentPbn);

            // start constructing the binding of bad physical blocks, that is,
            // blocks with at least 1 plane block

            // initialize the current block position for each plane
            for (i = 0; i < PLN_PER_LUN; i++)
            {
                usCurrentPlaneBlockPosition[i] = 0;
            }

            while (usCurrentPbn < BBT_BLK_PER_PLN)
            {
                /* find an unmapped plane block in each plane */
                for (i = 0; i < PLN_PER_LUN; i++)
                {
                    while (L2_BbtGetPbnBindingTableConstructBitmap(usCurrentPlaneBlockPosition[i], i) == TRUE)
                    {
                        // the current plane block has been mapped, proceed to the next one
                        usCurrentPlaneBlockPosition[i]++;
                    }
                }

                // we've found an unmapped plane block for each plane
                // mark these plane blocks as mapped
                for (i = 0; i < PLN_PER_LUN; i++)
                {
                    L2_BbtSetPbnBindingTableConstructBitmap(usCurrentPlaneBlockPosition[i], i);
                }

                // record the PBN binding
                //FIRMWARE_LogInfo("MCU%d LUN %d PBN %d\n", HAL_GetMcuId(), ucTlun, usCurrentPbn);
                for (i = 0; i < PLN_PER_LUN; i++)
                {
                    L2_BbtSetPbnBindingTable(ucTlun, usCurrentPbn, i, usCurrentPlaneBlockPosition[i]);
                    //FIRMWARE_LogInfo("MCU%d LUN %d plane %d block %d\n", HAL_GetMcuId(), ucTlun, i, usCurrentPlaneBlockPosition[i]);
                }

                // increase current plane block positions for all planes
                for (i = 0; i < PLN_PER_LUN; i++)
                {
                    usCurrentPlaneBlockPosition[i]++;
                }

                usCurrentPbn++;
            }

            // now that we've done mapping the PBN binding table,
            // perform various validity check

            /* check the PBN binding table construct bitmap, all bits must be marked */
            for (i = 0; i < BBT_BLK_PER_PLN; i++)
            {
                for (j = 0; j < PLN_PER_LUN; j++)
                {
                    if (L2_BbtGetPbnBindingTableConstructBitmap(i, j) == FALSE)
                    {
                        DBG_Printf("PBN binding table construct bitmap error\n");
                        DBG_Getch();
                    }
                }
            }

            if (usCurrentPbn != BBT_BLK_PER_PLN)
            {
                DBG_Printf("usCurrentPbn error\n");
                DBG_Getch();
            }

            //FIRMWARE_LogInfo("MCU%d LUN %d total physical block number %d\n", HAL_GetMcuId(), ucTlun, usCurrentPbn);
        }
    }
    else
    {
        U16 usPbn;
        U8 ucPlane;

        for (ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
        {
            for (usPbn = 0; usPbn < BBT_BLK_PER_PLN; usPbn++)
            {
                for (ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
                {
                    L2_BbtSetPbnBindingTable(ucTlun, usPbn, ucPlane, usPbn);
                }
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : L2_BbtBuild
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: BBT Build
Usage      :     
    LLF_METHOD_NORMAL = 0,  // load-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_READ_IDB,    // read-idb >>> format-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_FORMAT_GBBT, // format-global-bbt >>> load-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_FORMAT_BBT,  // format-bbt >>> format-disk >>> save-bbt
    LLF_METHOD_REDETECT_IDB,// format-bbt >>> format-disk >>> read-idb >>> save-bbt (for tsb-flash-chip)
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL MCU1_DRAM_TEXT L2_BbtBuild(BOOL bSecurityErase)
{
    BOOL bBbtBuildDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_ulBbtBuildStage = BBT_BUILD_INIT;
    BOOL bExistHardBindTable = TRUE;

    switch (s_ulBbtBuildStage)
    {
        case BBT_BUILD_INIT:
        {
            U8 ucLLFMethod = DiskConfig_GetLLFMethold();
           
            L2_BbtManagerInit(TRUE);

            // un-inherit the bad block information
            if ((LLF_METHOD_FORMAT_BBT == ucLLFMethod) && (FALSE == bSecurityErase))
            {
                s_ulBbtBuildStage = BBT_BUILD_FORMAT_BBT;
                DBG_Printf("MCU#%d LLF-BBT-Build-Init-Format-BBT -> FormatBBT.\n", HAL_GetMcuId());
            }
            // inherit the bad block information from IDB after Format-disk
            else if ((LLF_METHOD_REDETECT_IDB == ucLLFMethod) && (FALSE == bSecurityErase))
            {
                s_ulBbtBuildStage = BBT_BUILD_FORMAT_BBT;
                DBG_Printf("MCU#%d LLF-BBT-Build-Init-ReDetect-IDB -> FormatBBT.\n", HAL_GetMcuId());
            }
            // inherit the bad block information from IDB
            else if ((LLF_METHOD_READ_IDB == ucLLFMethod) && (FALSE == bSecurityErase))
            {                
                s_ulBbtBuildStage = BBT_BUILD_READ_IDB;
                DBG_Printf("MCU#%d LLF-BBT-Build-Init-Read-IDB -> ReadIDB.\n", HAL_GetMcuId());
            }
            // inherit the bad block information from flash local bbt
            else if ((LLF_METHOD_FORMAT_GBBT == ucLLFMethod) && (FALSE == bSecurityErase))
            {
                s_ulBbtBuildStage = BBT_BUILD_FORMAT_GBBT;
                DBG_Printf("MCU#%d LLF-BBT-Build-Init-Format-GBBT -> FormatGBBT.\n", HAL_GetMcuId());
            }
            // inherit the bad block information from flash global bbt
            else
            {
                s_ulBbtBuildStage = BBT_BUILD_LOAD_BBT;
                DBG_Printf("MCU#%d LLF-BBT-Build-Init-Normal -> LoadBBT.\n", HAL_GetMcuId());
            }

            break;
        }
        case BBT_BUILD_READ_IDB:
        {
            L2_BbtReadIDB();
            s_ulBbtBuildStage = BBT_BUILD_FORMAT_BBT;
            DBG_Printf("MCU#%d LLF-BBT-Build-ReadIDB -> FormatBBT.\n", HAL_GetMcuId());
            break;
        }
        case BBT_BUILD_FORMAT_BBT:
        {
            if (TRUE == L2_BbtFormatBBT())
            {                
                s_ulBbtBuildStage = BBT_BUILD_FORMAT_DISK;
                DBG_Printf("MCU#%d LLF-BBT-Build-FormatBBT -> FormatDisk.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_BUILD_FORMAT_GBBT:
        {
            if (TRUE == L2_BbtFormatGlobalBBT())
            {
                s_ulBbtBuildStage = BBT_BUILD_LOAD_BBT;
                DBG_Printf("MCU#%d LLF-BBT-Build-FormatGBBT -> LoadBBT.\n", HAL_GetMcuId());                
            }
            break;
        }
        case BBT_BUILD_LOAD_BBT:
        {
            if (TRUE == L2_BbtLoad(NULL))
            {
                s_ulBbtBuildStage = BBT_BUILD_FORMAT_DISK;
                DBG_Printf("MCU#%d LLF-BBT-Build-LoadBBT -> FormatDisk.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_BUILD_FORMAT_DISK:
        {
            if (TRUE == L2_BbtEraseWholeDisk())
            {
                if (LLF_METHOD_REDETECT_IDB == DiskConfig_GetLLFMethold())
                {
                    s_ulBbtBuildStage = BBT_BUILD_REDETECT_IDB;
                    DBG_Printf("MCU#%d LLF-BBT-Build-FormatDisk -> ReDetectIDB.\n", HAL_GetMcuId());
                }
                else
                {
                    s_ulBbtBuildStage = BBT_BUILD_PBN_BINDING_TABLE;

					if (TRUE == bSecurityErase)
					{
						L2_BbtEnablePbnBindingTable();
					}

                    DBG_Printf("MCU#%d LLF-BBT-Build-FormatDisk -> SaveBBT.\n", HAL_GetMcuId());
                }            
            }
            break;
        }
        case BBT_BUILD_REDETECT_IDB:
        {
            L2_BbtReadIDB();
            s_ulBbtBuildStage = BBT_BUILD_PBN_BINDING_TABLE;
            DBG_Printf("MCU#%d LLF-BBT-Build-ReDetectIDB -> SaveBBT.\n", HAL_GetMcuId());
            break;
        }
        case BBT_BUILD_PBN_BINDING_TABLE:
        {
            if(L2_BbtIsPbnBindingTableEnable() == FALSE)
            {
                if(L2_BbtPbnBindingTableExistenceCheck(&bExistHardBindTable) == TRUE)
                {
                    BOOL bPTableSoftBindingFlag = HAL_GetPbnBindingTableEnFlag();
                    BOOL bExistSoftBindTable =  !bExistHardBindTable;
                    if(bPTableSoftBindingFlag == bExistSoftBindTable)
                    {
                        // the PBN must have been loaded during the PBIT recover stage,
                        // simply enable the PBN binding table
                        L2_BbtEnablePbnBindingTable();
                        DBG_Printf("L2_BbtBuild: PBN binding table already exist, Hard binding: %d \n", bExistHardBindTable);                  
                    }
                    else if((bPTableSoftBindingFlag == FALSE) && (bExistSoftBindTable == TRUE))
                    {
                        // the PBN must have been loaded during the PBIT recover stage,
                        // simply enable the PBN binding table
                        L2_BbtEnablePbnBindingTable();
                        DBG_Printf("L2_BbtBuild: Warning: PBN binding table already exist, Hard binding: %d  The PTable flag is hard binding! The exist table will be used! \n", bExistHardBindTable);                  
                    }
                    else if((bPTableSoftBindingFlag == TRUE) && (bExistSoftBindTable == FALSE))
                    {
                        DBG_Printf("L2_BbtBuild: Warning: PBN binding table already exist, Hard binding: %d  The Table need to be transfered to soft binding! \n", bExistHardBindTable);

                        // build the PBN binding table from BBT
                        L2_BbtBuildPbnBindingTable();
                        L2_BbtEnablePbnBindingTable();
                        DBG_Printf("L2_BbtBuild: PBN binding table transfer done!\n");                  
                    }
                }
                else
                {
                    DBG_Printf("start constructing PBN binding table\n");

                    // build the PBN binding table from BBT
                    L2_BbtBuildPbnBindingTable();

                    // enable the PBN binding table
                    L2_BbtEnablePbnBindingTable();

                    // check the validity of the PBN binding table
                    //L2_BbtPbnBindingTableLlfCheck();

                    DBG_Printf("MCU%d PBN binding table LLF build and check passes!\n", HAL_GetMcuId());
                }
            }
            else
            {
                if(L2_BbtFormatLocalBBT() == TRUE)
                {
                    s_ulBbtBuildStage = BBT_BUILD_SAVE_BBT;
                }
            }
            break;
        }
        case BBT_BUILD_SAVE_BBT:
        {
            if (TRUE == L2_BbtSave(INVALID_2F, INVALID_2F))
            {
                s_ulBbtBuildStage = BBT_BUILD_FINISH;
                DBG_Printf("MCU#%d LLF-BBT-Build-SaveBBT -> Finished.\n", HAL_GetMcuId());
            }
            break;
        }
        case BBT_BUILD_FINISH:
        {
            bBbtBuildDone = TRUE;
            s_ulBbtBuildStage = BBT_BUILD_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d LLF-BBT-Build Stage %d Error.\n", HAL_GetMcuId(), s_ulBbtBuildStage);
            DBG_Getch();
        }
    }

    return bBbtBuildDone;
}

/*==============================================================================
Func Name  : L2_BbtRebuildLoadLBbt
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: 
Usage      : 
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtRebuildLoadLBbt(void)
{
    U8 ucTLun;
    BOOL bRebuildBbtDone = FALSE;
    BBT_MANAGER *ptLBBTManager;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptLBBTManager = &l_ptBBTManager[ucTLun + 1];

        switch (ptLBBTManager->bsOpStage)
        {
            case BBT_REBUILD_LOAD_LBBT_INIT:
            {                
                L2_BbtMemZeroLBbt(ucTLun);

                ptLBBTManager->bsBlk = LBBT_BLK;
                ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                ptLBBTManager->bsPage = (INVALID_4F == ptLBBTManager->bsPage) ? 0 : ptLBBTManager->bsPage; // read, until meet empty-page

                ptLBBTManager->bsOpStage = BBT_REBUILD_LOAD_LBBT_READ;
                break;
            }
            case BBT_REBUILD_LOAD_LBBT_READ:
            {
                FCMD_REQ_ENTRY *ptReqEntry;

                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
            
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                    
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsVirPage = ptLBBTManager->bsPage;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
                ptReqEntry->tFlashDesc.bsSecStart = 0;
                ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

                ptReqEntry->atBufDesc[0].bsBufID = ptLBBTManager->bsBuffID + ptLBBTManager->bsPln - START_PLN_IN_LBBT;
                ptReqEntry->atBufDesc[0].bsSecStart = 0;
                ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
                ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
                
                ptReqEntry->ulSpareAddr = (U32)ptLBBTManager->pSpare;                

                *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);            

                ptLBBTManager->bsOpStage = BBT_REBUILD_LOAD_LBBT_READ_WAIT;
                break;
            }
            case BBT_REBUILD_LOAD_LBBT_READ_WAIT:
            {
                if (SUBSYSTEM_STATUS_SUCCESS == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RECC == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *ptLBBTManager->pFlashStatus)
                {
					if (BBT_NEW_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_RDT_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_NEW_V2_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild Load Red-Mis-Match.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        DBG_Printf("ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark:0x%x != 0x%x, %d\n", BBT_NEW_MARK, ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark, ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark);
                        DBG_Getch();
                    }
                    //DBG_Printf("L2_BbtRebuildLoadLBbt: MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild Load Success\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);

                    ptLBBTManager->bsPln = (ptLBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptLBBTManager->bsPln)
                    {
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = BBT_REBUILD_LOAD_LBBT_FINISH;
                    }
                    else
                    {
                        ptLBBTManager->bsOpStage = BBT_REBUILD_LOAD_LBBT_READ;
                    }
                }
                else if (SUBSYSTEM_STATUS_EMPTY_PG == *ptLBBTManager->pFlashStatus)
                {
                    //DBG_Printf("L2_BbtRebuildLoadLBbt: MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild EmptyPage\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                    if (0 == ptLBBTManager->bsPage)
                    {
                        L2_BbtMemZeroLBbt(ucTLun);

                        ptLBBTManager->bsPage = INVALID_4F;
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = BBT_REBUILD_LOAD_LBBT_FINISH;  
                    }
                    else
                    {
                        DBG_Printf("L2_BbtRebuildLoadLBbt: MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild Load Empty-Page.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        DBG_Getch();
                    }
                }
                else if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild Load Retry-Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                    DBG_Getch();
                }
                else
                {
                    ;// wait flash status...
                }
                break;
            }
            case BBT_REBUILD_LOAD_LBBT_FINISH:
            {
                if (TRUE == L2_BbtWaitAllLBBTOpStageDone(BBT_REBUILD_LOAD_LBBT_FINISH))
                {
                    return TRUE; // return immediately.
                }
                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d Bbt Rebuild Load Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bRebuildBbtDone;
}
/*==============================================================================
Func Name  : L2_BbtLoadLBbt
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: 
Usage      : 
History    :
1. 2015.11.12 steven create
==============================================================================*/
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtLoadLBbt(void)
{
    U8 ucTLun;
    BOOL bLoadBbtDone = FALSE;
    BBT_MANAGER *ptLBBTManager;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptLBBTManager = &l_ptBBTManager[ucTLun + 1];

        switch (ptLBBTManager->bsOpStage)
        {
            case BBT_READ_LBBT_INIT:
            {                
                ptLBBTManager->bsOpStage = BBT_READ_LBBT_READ;
                break;
            }
            case BBT_READ_LBBT_READ:
            {
                FCMD_REQ_ENTRY *ptReqEntry;

                if (FALSE == L2_FCMDQNotFull(ucTLun))
                {
                    break;
                }
                
                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                    
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsVirPage = ptLBBTManager->bsPage;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
                ptReqEntry->tFlashDesc.bsSecStart = 0;
                ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

                ptReqEntry->atBufDesc[0].bsBufID = ptLBBTManager->bsBuffID + ptLBBTManager->bsPln - START_PLN_IN_LBBT;
                ptReqEntry->atBufDesc[0].bsSecStart = 0;
                ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
                ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
                
                ptReqEntry->ulSpareAddr = (U32)ptLBBTManager->pSpare;                

                *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);            
                
                ptLBBTManager->bsOpStage = BBT_READ_LBBT_READ_WAIT;
                break;
            }
            case BBT_READ_LBBT_READ_WAIT:
            {
                if (SUBSYSTEM_STATUS_SUCCESS == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RECC == *ptLBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *ptLBBTManager->pFlashStatus)
                {
					if (BBT_NEW_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_RDT_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark && BBT_NEW_V2_MARK != ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Load Red-Mis-Match.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        DBG_Getch();
                    }
                    //DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d LBbt Load Success.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);

                    if(BBT_RDT_MARK == ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark)
                    {
                        l_tGBbtInfo.ulBbtMark = BBT_RDT_MARK;
                    }
                    ptLBBTManager->bsPln = (ptLBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptLBBTManager->bsPln)
                    {
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = BBT_READ_LBBT_FINISH;
                    }
                    else
                    {
                        ptLBBTManager->bsOpStage = BBT_READ_LBBT_READ;
                    }
                }
                else if (SUBSYSTEM_STATUS_EMPTY_PG == *ptLBBTManager->pFlashStatus)
                {
                    if (0 == ptLBBTManager->bsPage)
                    {
                        L2_BbtMemZeroLBbt(ucTLun);

                        ptLBBTManager->bsPage = INVALID_4F;
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = BBT_READ_LBBT_FINISH;                        
                    }
                    else
                    {
                        DBG_Printf("L2_BbtLoadLBbt: MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Rebuild Load Empty-Page.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        DBG_Getch();
                    }
                }
                else if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Bbt Load Retry-Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                    DBG_Getch();
                }
                else
                {
                    ;// wait flash status...
                }
                break;
            }
            case BBT_READ_LBBT_FINISH:
            {
                if (TRUE == L2_BbtWaitAllLBBTOpStageDone(BBT_READ_LBBT_FINISH))
                {
                    return TRUE; // return immediately.
                }
                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d Bbt Load Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bLoadBbtDone;
}

GLOBAL void MCU1_DRAM_TEXT L2_BbtDisablePbnBindingTable(void)
{
    g_pMCU12MiscInfo->ulIsPbnBindingTableReady = FALSE;
}

GLOBAL void MCU12_DRAM_TEXT L2_BbtEnablePbnBindingTable(void)
{
    g_pMCU12MiscInfo->ulIsPbnBindingTableReady = TRUE;
}

GLOBAL U8 L2_BbtIsPbnBindingTableEnable(void)
{
    return g_pMCU12MiscInfo->ulIsPbnBindingTableReady;
}

GLOBAL BOOL MCU12_DRAM_TEXT L2_BbtLoadPbnBindingTable(void)
{
    // Sean Gao 20160603
    // this function tries to load PBN binding table from the flash,
    // please note that this function does not guarantee the loaded
    // content is correct, we have to check that ourselves

    U8 ucTlun;
    U8 ucLoadPbnBindingTableDone;
    //LOCAL MCU12_VAR_ATTR U32 s_ulLoadPbnBindingTableStage[SUBSYSTEM_LUN_MAX] = {0};
    
    //LOCAL MCU12_VAR_ATTR U16 s_usCurrentPage[SUBSYSTEM_LUN_MAX] = {0};
    
    // do nothing and return TRUE if the PBN binding table has already been loaded
    if(L2_BbtIsPbnBindingTableEnable() == TRUE)
    {
        return TRUE;
    }

    ucLoadPbnBindingTableDone = FALSE;

    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        switch(s_ulLoadPbnBindingTableStage[ucTlun])
        {
            case PBN_BINDING_TABLE_LOAD_INIT:
            {
                s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_READ;
                break;
            }
            case PBN_BINDING_TABLE_LOAD_READ:
            {
                FCMD_REQ_ENTRY *ptReqEntry;

                if (FALSE == L2_FCMDQNotFull(ucTlun))
                {
                    break;
                }

                ptReqEntry = L2_FCMDQAllocReqEntry(ucTlun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                    
                ptReqEntry->tFlashDesc.bsVirBlk = LBBT_BLK;
                ptReqEntry->tFlashDesc.bsVirPage = s_usCurrentPage[ucTlun];
                ptReqEntry->tFlashDesc.bsPlnNum = PLN_PER_LUN - 1;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
                ptReqEntry->tFlashDesc.bsSecStart = 0;
                ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

                ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(l_ulPbnBindingTableLoadingBuffer + (ucTlun * LOGIC_PG_SZ), TRUE, LOGIC_PG_SZ_BITS);
                ptReqEntry->atBufDesc[0].bsSecStart = 0;
                ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
                ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;              

                s_ucLoadPbnBindingTableStatus[ucTlun] = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)(&(s_ucLoadPbnBindingTableStatus[ucTlun]));
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTlun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);    

                // advance the stage
                s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_READ_WAIT;

                break;
            }
            case PBN_BINDING_TABLE_LOAD_READ_WAIT:
            {
                if(SUBSYSTEM_STATUS_SUCCESS == s_ucLoadPbnBindingTableStatus[ucTlun]
                    || SUBSYSTEM_STATUS_RECC == s_ucLoadPbnBindingTableStatus[ucTlun]
                    || SUBSYSTEM_STATUS_RETRY_SUCCESS == s_ucLoadPbnBindingTableStatus[ucTlun])
                {
                    // we've succesfully read the last plane

                    // copy the PBN binding table
                    U32 ulPbnBindingTableSourceAddress;
                    U32 ulPbnBindingTableTargetAddress;
                    U32 i;

                    // determine the source buffer address
                    ulPbnBindingTableSourceAddress = l_ulPbnBindingTableLoadingBuffer + (ucTlun * LOGIC_PG_SZ) + (LOGIC_PG_SZ - SOFT_BINDING_TABLE_OFFSET);

                    // determine the target buffer address
                    ulPbnBindingTableTargetAddress =
                    g_pMCU12MiscInfo->ulPbnBindingTable // starting address
                    + (ucTlun * (BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16))); // LUN offset

                    // copy all entries from the local buffer to the PBN binding table
                    for(i = 0; i < (BBT_BLK_PER_PLN * PLN_PER_LUN); i++)
                    {
                        *( (U16*) ulPbnBindingTableTargetAddress ) = *( (U16*) ulPbnBindingTableSourceAddress );

                        // increase both ulPbnBindingTableTargetAddress and ulPbnBindingTableSourceAddress
                        ulPbnBindingTableTargetAddress += sizeof(U16);
                        ulPbnBindingTableSourceAddress += sizeof(U16);
                    }

                    // advance the stage
                    s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_FINISH;
                }
                else if(SUBSYSTEM_STATUS_FAIL == s_ucLoadPbnBindingTableStatus[ucTlun])
                {
                    // fail to read current LBBT page, advance the target page number and read again
                    s_usCurrentPage[ucTlun]++;

                    if (s_usCurrentPage[ucTlun] >= PG_PER_SLC_BLK)
                    {
                        s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_FINISH;
                    }
                    else
                    {
                        s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_READ;
                    }
                }
                else if(SUBSYSTEM_STATUS_EMPTY_PG == s_ucLoadPbnBindingTableStatus[ucTlun])
                {
                    // fail to load the PBN binding table for the current Tlun
                    s_ulLoadPbnBindingTableStage[ucTlun] = PBN_BINDING_TABLE_LOAD_FINISH;
                }
                else if(SUBSYSTEM_STATUS_PENDING == s_ucLoadPbnBindingTableStatus[ucTlun])
                {
                    // operation pending, do nothing
                    ;
                }
                else
                {
                    DBG_Printf("MCU#%d load TLun%d PBN binding table status error: %d\n", HAL_GetMcuId(), ucTlun, s_ucLoadPbnBindingTableStatus[ucTlun]);
                    if((SUBSYSTEM_STATUS_SUCCESS != s_ucLoadPbnBindingTableStatus[ucTlun]) && (SUBSYSTEM_STATUS_RECC != s_ucLoadPbnBindingTableStatus[ucTlun])
                       && (SUBSYSTEM_STATUS_RETRY_SUCCESS != s_ucLoadPbnBindingTableStatus[ucTlun]) && (SUBSYSTEM_STATUS_FAIL != s_ucLoadPbnBindingTableStatus[ucTlun])
                       && (SUBSYSTEM_STATUS_EMPTY_PG != s_ucLoadPbnBindingTableStatus[ucTlun]) && (SUBSYSTEM_STATUS_PENDING != s_ucLoadPbnBindingTableStatus[ucTlun]))
                    {
                        DBG_Getch();
                    }
                }
                break;
            }
            case PBN_BINDING_TABLE_LOAD_FINISH:
            {
                U8 ucIsAllTlunFinishLoadingPbnBindingTable = TRUE;
                U32 i;

                for(i = 0; i < SUBSYSTEM_LUN_NUM; i++)
                {
                    if(s_ulLoadPbnBindingTableStage[i] != PBN_BINDING_TABLE_LOAD_FINISH)
                    {
                        ucIsAllTlunFinishLoadingPbnBindingTable = FALSE;
                        break;
                    }
                }

                if(ucIsAllTlunFinishLoadingPbnBindingTable == TRUE)
                {
                    // all LUNs have finished loading

                    // reset stage and target page number for all LUNs
                    for(i = 0; i < SUBSYSTEM_LUN_NUM; i++)
                    {
                        s_ucLoadPbnBindingTableStatus[i] = PBN_BINDING_TABLE_LOAD_INIT;
                        s_usCurrentPage[i] = 0;
                    }

                    // mark operation as complete
                    ucLoadPbnBindingTableDone = TRUE;
                }

                break;
            }
            default:
            {
                DBG_Printf("MCU#%d load PBN binding table stage error\n", HAL_GetMcuId());
                DBG_Getch();
            }
        } // switch(s_ulLoadPbnBindingTableStage[ucTlun])
    } // for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)

    return ucLoadPbnBindingTableDone;
}

/*==============================================================================
Func Name  : L2_BbtRebuild
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: Rebuild BBT
Usage      : ZeroBBTMem -> Load LBBT -> Fail -> Getch();
                                     -> Empty-Page -> Page=0 -> ZeroLBBT, Page=Invalid -> WaitAllLoadDone
                                                   -> Page!=0 -> Getch();
                                     -> Success -> BbtMarkIsValid -> Yes -> MergeLBbtToGBbt -> WaitAllLoadDOne
                                                                  -> No -> Getch();
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtRebuild(void)
{
    BOOL bRebuildBbtDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_ulBbtRebuildStage = BBT_REBUILD_INIT;

    switch (s_ulBbtRebuildStage)
    {
        case BBT_REBUILD_INIT:
        {            
            s_ulBbtRebuildStage = BBT_REBUILD_LOAD_LBBT;
            break;
        }
        case BBT_REBUILD_LOAD_LBBT:
        {
            if (TRUE == L2_BbtRebuildLoadLBbt())
            {
                s_ulBbtRebuildStage = BBT_REBUILD_MERGE_GBBT;
            }
            break;
        }
        case BBT_REBUILD_MERGE_GBBT:
        {
            L2_BbtRebuildMergeGBbt();

            /* Don't need save BBT here,update by Nina 2017-12-18 */
            s_ulBbtRebuildStage = BBT_REBUILD_FINISH;//BBT_REBUILD_SAVE_BBT;
            break;
        }
        case BBT_REBUILD_SAVE_BBT:
        {
            if (TRUE == L2_BbtSave(l_tGBbtInfo.bsGBbtTLun, INVALID_2F))
            {
                s_ulBbtRebuildStage = BBT_REBUILD_FINISH;
            }
            break;
        }
        case BBT_REBUILD_FINISH:
        {
            bRebuildBbtDone = TRUE;
            s_ulBbtRebuildStage = BBT_REBUILD_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d Bbt Rebuild Stage %d Error.\n", HAL_GetMcuId(), s_ulBbtRebuildStage);
            DBG_Getch();
        }
    }

    return bRebuildBbtDone;
}

/*==============================================================================
Func Name  : L2_BbtSaveGBBT
Input      : U8 ucErrHPu
Output     : NONE
Return Val : BOOL
Discription: when L3 Err-Handling, need to check.
Usage      : GBbt-Invalid -> SetGBbtPage=0 -> Red=GBbtSn+1 -> Write -> Success -> GBbtSn++
-> Fail -> MskGBbtTLun -> SelectNewGBbtTLun -> SetGBbtPage=0 ->...
GBbt-Valid -> Page=GBbtPage+1, CheckFull -> No -> Red=GBbtSn+1 -> Write -> ...
-> Yes -> EraseGBbtTLun -> Success -> SelectNewGBbtTLun -> ...
-> Fail -> MskGBbtTLun -> SelectNewGBbtTLun -> ...
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
LOCAL BOOL MCU1_DRAM_TEXT L2_BbtSaveGBBT(U8 ucTargetTLun, U8 ucErrHPu)
{
    U8 ucGlobalTLun = l_tGBbtInfo.bsGBbtTLun;
    BOOL bSaveGBBTDone = FALSE;
    BBT_MANAGER *ptGBBTManager = &l_ptBBTManager[0];

    switch (ptGBBTManager->bsOpStage)
    {
        case GBBT_SAVE_INIT:
        {
            ptGBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk;
            ptGBBTManager->bsPln = START_PLN_IN_GBBT;
            ptGBBTManager->bsPage = (INVALID_4F == l_tGBbtInfo.bsGBbtPage) ? 0 : l_tGBbtInfo.bsGBbtPage + 1;
            if (BBT_PG_PER_BLK <= ptGBBTManager->bsPage) // CheckFull
            {
                ptGBBTManager->bsOpStage = GBBT_SAVE_ERASE;
            }
            else
            {
                ptGBBTManager->bsOpStage = GBBT_SAVE_WRITE;
            }            

            break;
        }
        case GBBT_SAVE_WRITE:
        {
            U32 *pTargetRed;       
            FCMD_REQ_ENTRY *ptReqEntry;

            if (TRUE != L2_FCMDQNotFull(ucGlobalTLun))
            {
                if ((INVALID_2F == ucErrHPu) /*|| (ucGlobalTLun != ucTargetTLun)*/)
                {
                    if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = FALSE;
                }
                break;
            }
            if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = TRUE;

            COM_MemZero((U32 *)ptGBBTManager->pSpare, sizeof(RED) / sizeof(U32));
            ptGBBTManager->pSpare->m_RedComm.bcPageType = PAGE_TYPE_BBT;
            ptGBBTManager->pSpare->m_RedComm.bcBlockType = BLOCK_TYPE_BBT;
            ptGBBTManager->pSpare->m_RedComm.eOPType = OP_TYPE_BBT_WRITE;
            ptGBBTManager->pSpare->m_tGBbtInfo.ulBbtMark = l_tGBbtInfo.ulBbtMark;
            ptGBBTManager->pSpare->m_tGBbtInfo.bsGBbtTLun = l_tGBbtInfo.bsGBbtTLun;
            ptGBBTManager->pSpare->m_tGBbtInfo.bsGBbtBlk = l_tGBbtInfo.bsGBbtBlk;
            ptGBBTManager->pSpare->m_tGBbtInfo.bsGBbtPage = ptGBBTManager->bsPage;
            ptGBBTManager->pSpare->m_tGBbtInfo.ulMaxBbtSn = l_tGBbtInfo.ulMaxBbtSn + 1;
            COM_MemCpy((U32*)&ptGBBTManager->pSpare->m_tGBbtInfo.aLunMskBitMap, (U32*)&l_tGBbtInfo.aLunMskBitMap, BBT_TLUN_MSK_SIZE);            

            ptReqEntry = L2_FCMDQAllocReqEntry(ucGlobalTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;
                
            ptReqEntry->tFlashDesc.bsVirBlk = ptGBBTManager->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptGBBTManager->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptGBBTManager->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;
            
            ptReqEntry->atBufDesc[0].bsBufID = ptGBBTManager->bsBuffID + ptGBBTManager->bsPln - START_PLN_IN_GBBT;
            ptReqEntry->atBufDesc[0].bsSecStart = 0;
            ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

            pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucGlobalTLun, ptReqEntry->bsReqPtr);
            COM_MemCpy(pTargetRed, (U32*)ptGBBTManager->pSpare, RED_SW_SZ_DW);
            ptReqEntry->ulSpareAddr = (U32)pTargetRed;                
            
            *ptGBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptGBBTManager->pFlashStatus;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
            
            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucGlobalTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
            
            ptGBBTManager->bsOpStage = GBBT_SAVE_WRITE_WAIT;
            break;
        }
        case GBBT_SAVE_WRITE_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *ptGBBTManager->pFlashStatus)
            {
                if (SUBSYSTEM_STATUS_FAIL != *ptGBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save-Global-BBT InvalidStatus.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_SUCCESS == *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("L2_BbtSaveGBBT: MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Save-Global-BBT Write-Success.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                    ptGBBTManager->bsPln = (ptGBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptGBBTManager->bsPln)
                    {
                        l_tGBbtInfo.bsGBbtPage = ptGBBTManager->bsPage;
                        l_tGBbtInfo.ulMaxBbtSn++;
                        l_tGBbtInfo.bsFormatGBBT = FALSE;
                        if (INVALID_8F == l_tGBbtInfo.ulMaxBbtSn)
                        {
                            DBG_Printf("Sn is the max\n");
                            DBG_Getch();
                        }
                        ptGBBTManager->bsPln = START_PLN_IN_GBBT;
                        ptGBBTManager->bsOpStage = GBBT_SAVE_FINISH;
                    }
                    else
                    {
                        ptGBBTManager->bsOpStage = GBBT_SAVE_WRITE;
                    }

                    break;
                }
                
                if (SUBSYSTEM_STATUS_FAIL == *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Save-Global-BBT Write-Fail.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                    L2_BbtMskGBbtTLun(ucGlobalTLun);
                    if (FALSE == L2_BbtUpdateGBbtTLun(ucGlobalTLun))
                    {
                        DBG_Printf("Can't Find valiable LUN to Save GBBT\n");
                        DBG_Getch();
                    }

                    ptGBBTManager->bsPln = START_PLN_IN_GBBT;
                    ptGBBTManager->bsOpStage = GBBT_SAVE_INIT;

                    break;
                }
            }
            
            break;
        }
        case GBBT_SAVE_ERASE:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (TRUE != L2_FCMDQNotFull(ucGlobalTLun))
            {
                if ((INVALID_2F == ucErrHPu) /*|| (ucPu != ucErrHPu)*/)
                {
                    if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = FALSE;
                }
                break;
            }                
            if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = TRUE;

            ptReqEntry = L2_FCMDQAllocReqEntry(ucGlobalTLun, 0);  
                
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;
            
            ptReqEntry->tFlashDesc.bsVirBlk = ptGBBTManager->bsBlk;
            ptReqEntry->tFlashDesc.bsPlnNum = ptGBBTManager->bsPln;  
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                            
            if (NULL != ptGBBTManager->pFlashStatus)
            {
                *ptGBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptGBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
            }
            
            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucGlobalTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);         

            ptGBBTManager->bsOpStage = GBBT_SAVE_ERASE_WAIT;
            break;
        }
        case GBBT_SAVE_ERASE_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *ptGBBTManager->pFlashStatus)
            {
                if (SUBSYSTEM_STATUS_FAIL != *ptGBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save Erase-Old-Global-BBT InvalidStatus.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_FAIL == *ptGBBTManager->pFlashStatus)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save Erase-Old-Global-BBT Fail.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk);
                    L2_BbtMskGBbtTLun(ucGlobalTLun);
                }

                ptGBBTManager->bsPln = (ptGBBTManager->bsPln + 1) % PLN_PER_LUN;
                if (0 == ptGBBTManager->bsPln)
                {
                    if (FALSE == L2_BbtUpdateGBbtTLun(ucGlobalTLun))
                    {
                        DBG_Printf("Can't Find valiable LUN to Save GBBT\n");
                        DBG_Getch();
                    }
                    ptGBBTManager->bsPln = START_PLN_IN_GBBT;
                    ptGBBTManager->bsOpStage = GBBT_SAVE_INIT;
                }
                else
                {
                    ptGBBTManager->bsOpStage = GBBT_SAVE_ERASE;
                }
            }
            break;
        }
        case GBBT_SAVE_FINISH:
        {
            bSaveGBBTDone = TRUE;
            ptGBBTManager->bsOpStage = GBBT_SAVE_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Save Global BBT Stage %d Error.\n", HAL_GetMcuId(), ptGBBTManager->bsOpStage);
            DBG_Getch();
        }
    }

    return bSaveGBBTDone;
}

/*==============================================================================
Func Name  : L2_BbtSaveLBBT
Input      : U8 ucTLun
U8 ucErrHPu
Output     : NONE
Return Val : BOOL
Discription: ucTLun==INVALID_2F -> save all TLun Local-BBT
ucTLun!=INVALID_2F -> save the TLun Local-BBT
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL MCU1_DRAM_TEXT L2_BbtSaveLBBT(U8 ucTargetTLun, U8 ucErrHPu)
{
    U8 ucTLun, ucTLunStart, ucTLunEnd;
    BOOL bLBbtSaveDone = FALSE;
    BBT_MANAGER *ptLBBTManager;

    if (INVALID_2F == ucTargetTLun)
    {
        ucTLunStart = 0, ucTLunEnd = SUBSYSTEM_LUN_NUM;
    }
    else
    {
        ucTLunStart = ucTargetTLun; ucTLunEnd = ucTargetTLun + 1;
    }

    for (ucTLun = ucTLunStart; ucTLun < ucTLunEnd; ucTLun++)
    {
        ptLBBTManager = &l_ptBBTManager[ucTLun + 1];

        switch (ptLBBTManager->bsOpStage)
        {
            case LBBT_SAVE_INIT:
            {
                ptLBBTManager->bsBlk = LBBT_BLK;
                ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                ptLBBTManager->bsPage = (INVALID_4F == ptLBBTManager->bsPage) ? 0 : ptLBBTManager->bsPage + 1;
                if (BBT_PG_PER_BLK-1 <= ptLBBTManager->bsPage) // CheckFull, the last page is used to indicate the empty-page when local-bbt-finding.
                {
                    DBG_Printf("MCU#%d TLun#%d BBT Save Local BBT full, goto erase.\n", HAL_GetMcuId(), ucTLun);    

                    ptLBBTManager->bsPage = INVALID_4F;
                    ptLBBTManager->bsOpStage = LBBT_SAVE_ERASE;
                }
                else
                {
                    ptLBBTManager->bsOpStage = LBBT_SAVE_WRITE;
                }
                break;
            }
            case LBBT_SAVE_WRITE:
            {
                U32 *pTargetRed;   
                FCMD_REQ_ENTRY *ptReqEntry;

                if (TRUE != L2_FCMDQNotFull(ucTLun))
                {
                    if ((INVALID_2F == ucErrHPu) /*|| (ucPu != ucErrHPu)*/)
                    {
                        if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = FALSE;
                    }
                    break;
                }
                if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = TRUE;

                COM_MemZero((U32 *)ptLBBTManager->pSpare, sizeof(RED) / sizeof(U32));
                ptLBBTManager->pSpare->m_RedComm.bcPageType = PAGE_TYPE_BBT;
                ptLBBTManager->pSpare->m_RedComm.bcBlockType = BLOCK_TYPE_BBT;
                ptLBBTManager->pSpare->m_RedComm.eOPType = OP_TYPE_BBT_WRITE;
                ptLBBTManager->pSpare->m_tGBbtInfo.ulBbtMark = l_tGBbtInfo.ulBbtMark;
                ptLBBTManager->pSpare->m_tGBbtInfo.bsGBbtTLun = l_tGBbtInfo.bsGBbtTLun;
                ptLBBTManager->pSpare->m_tGBbtInfo.bsGBbtBlk = l_tGBbtInfo.bsGBbtBlk;
                ptLBBTManager->pSpare->m_tGBbtInfo.bsGBbtPage = l_tGBbtInfo.bsGBbtPage;
                ptLBBTManager->pSpare->m_tGBbtInfo.ulMaxBbtSn = l_tGBbtInfo.ulMaxBbtSn;
                COM_MemCpy((U32*)&ptLBBTManager->pSpare->m_tGBbtInfo.aLunMskBitMap, (U32*)&l_tGBbtInfo.aLunMskBitMap, BBT_TLUN_MSK_SIZE);

                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
                    
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsVirPage = ptLBBTManager->bsPage;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
                ptReqEntry->tFlashDesc.bsSecStart = 0;
                ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;
                
                ptReqEntry->atBufDesc[0].bsBufID = ptLBBTManager->bsBuffID + ptLBBTManager->bsPln - START_PLN_IN_LBBT;
                ptReqEntry->atBufDesc[0].bsSecStart = 0;
                ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
                ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

                pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
                COM_MemCpy(pTargetRed, (U32*)ptLBBTManager->pSpare, RED_SW_SZ_DW);
                ptReqEntry->ulSpareAddr = (U32)pTargetRed;                
                
                *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);     

                ptLBBTManager->bsOpStage = LBBT_SAVE_WRITE_WAIT;
                break;
            }
            case LBBT_SAVE_WRITE_WAIT:
            {
                if (SUBSYSTEM_STATUS_PENDING != *ptLBBTManager->pFlashStatus)
                {
                    if (SUBSYSTEM_STATUS_FAIL != *ptLBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save-Local-BBT InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Save-Local-BBT Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);
                        DBG_Getch();
                    }
                    
                    //DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Save-Local-BBT Success.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk, ptLBBTManager->bsPage);

                    ptLBBTManager->bsPln = (ptLBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptLBBTManager->bsPln)
                    {
                        ptLBBTManager->bsPln = START_PLN_IN_LBBT;
                        ptLBBTManager->bsOpStage = LBBT_SAVE_FINISH;
                    }
                    else
                    {
                        ptLBBTManager->bsOpStage = LBBT_SAVE_WRITE;
                    }
                }
                break;
            }
            case LBBT_SAVE_ERASE:
            {
                FCMD_REQ_ENTRY *ptReqEntry;

                if (TRUE != L2_FCMDQNotFull(ucTLun))
                {
                    if ((INVALID_2F == ucErrHPu) /*|| (ucPu != ucErrHPu)*/)
                    {
                        if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = FALSE;
                    }
                    break;
                }
                if (INVALID_2F != ucTargetTLun) l_bSaveBbtLocked = TRUE;

                ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
                ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
                ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
                ptReqEntry->bsTableReq = TRUE;
            
                ptReqEntry->tFlashDesc.bsVirBlk = ptLBBTManager->bsBlk;
                ptReqEntry->tFlashDesc.bsPlnNum = ptLBBTManager->bsPln;  
                ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                            
                if (NULL != ptLBBTManager->pFlashStatus)
                {
                    *ptLBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
                    ptReqEntry->ulReqStsAddr = (U32)ptLBBTManager->pFlashStatus;
                    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
                }
                
                L2_FCMDQAdaptPhyBlk(ptReqEntry);
                L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

                ptLBBTManager->bsOpStage = LBBT_SAVE_ERASE_WAIT;
                break;
            }
            case LBBT_SAVE_ERASE_WAIT:
            {
                if (SUBSYSTEM_STATUS_PENDING != *ptLBBTManager->pFlashStatus)
                {
                    if (SUBSYSTEM_STATUS_FAIL != *ptLBBTManager->pFlashStatus && SUBSYSTEM_STATUS_SUCCESS != *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save Erase-Local-BBT InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    if (SUBSYSTEM_STATUS_FAIL == *ptLBBTManager->pFlashStatus)
                    {
                        DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save Erase-Local-BBT Fail.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsPln, ptLBBTManager->bsBlk);
                        DBG_Getch();
                    }

                    ptLBBTManager->bsPln = (ptLBBTManager->bsPln + 1) % PLN_PER_LUN;
                    if (0 == ptLBBTManager->bsPln)
                    {                            
                        ptLBBTManager->bsOpStage = LBBT_SAVE_INIT;
                    }
                    else
                    {
                        ptLBBTManager->bsOpStage = LBBT_SAVE_ERASE;
                    }                   
                }
                break;
            }
            case LBBT_SAVE_FINISH:
            {
                if (INVALID_2F != ucTargetTLun)
                {
                    bLBbtSaveDone = TRUE;
                    ptLBBTManager->bsOpStage = LBBT_SAVE_INIT;
                }
                else if (TRUE == L2_BbtWaitAllLBBTOpStageDone(LBBT_SAVE_FINISH))
                {
                    return TRUE; // return immediately.
                }
                break;
            }
            default:
            {
                DBG_Printf("MCU#%d TLun#%d BBT Save Local BBT Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptLBBTManager->bsOpStage);
                DBG_Getch();
            }
        }
    }

    return bLBbtSaveDone;
}

/*==============================================================================
Func Name  : L2_BbtSave
Input      : U8 ucTLun
U8 ucErrHPu
Output     : NONE
Return Val : BOOL
Discription: Save BBT
Usage      :
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL MCU1_DRAM_TEXT L2_BbtSave(U8 ucTargetTLun, U8 ucErrHPu)
{
    U8 ucTLunStage;
    BOOL bSaveBBTDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_aBbtSaveStage[SUBSYSTEM_LUN_MAX+1] = { 0 };

    ucTLunStage = (INVALID_2F == ucTargetTLun) ? SUBSYSTEM_LUN_NUM : ucTargetTLun;
    switch (s_aBbtSaveStage[ucTLunStage])
    {
        case BBT_SAVE_INIT:
        {
            if (TRUE == l_bSaveBbtLocked) 
                break;

            // lock will be turned on if we have a specific Tlun
            if (INVALID_2F != ucTargetTLun) 
                l_bSaveBbtLocked = TRUE;

            /* Reset save GBBT to GBBT_BLK by Nina 2017-11-28
             - blk1 is used to save GlobalBBT,it means SoftBinding GoodBlk.
             - when Format GBBT,SoftBinding table is not enabled,Blk1 -> Pln0 blk1, pln1 blk1, pln2 blk1, pln3 blk1,
             - but when SaveGBBT,SoftBinding table is enabled,so need reset to Good virtual PBN binding blk1
             */
            l_tGBbtInfo.bsGBbtBlk = GBBT_BLK;

            s_aBbtSaveStage[ucTLunStage] = BBT_SAVE_GBBT;
            break;
        }
        case BBT_SAVE_GBBT:
        {
            if (TRUE == L2_BbtSaveGBBT(ucTargetTLun, ucErrHPu))
            {
                s_aBbtSaveStage[ucTLunStage] = BBT_SAVE_MERGE_LBBT;
            }
            break;
        }
        case BBT_SAVE_MERGE_LBBT:
        {
            L2_BbtSaveMergeLBbt();
            s_aBbtSaveStage[ucTLunStage] = BBT_SAVE_LBBT;
            break;
        }
        case BBT_SAVE_LBBT:
        {
            if (TRUE == L2_BbtSaveLBBT(ucTargetTLun, ucErrHPu))
            {
                s_aBbtSaveStage[ucTLunStage] = BBT_SAVE_FINISH;
            }
            break;
        }
        case BBT_SAVE_FINISH:
        {
            if (INVALID_2F != ucTargetTLun)
            {
                l_bSaveBbtLocked = FALSE;
            }

            bSaveBBTDone = TRUE;
            s_aBbtSaveStage[ucTLunStage] = BBT_SAVE_INIT;
            DBG_Printf("TLun#%d SaveBBT Done.\n", ucTargetTLun);
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d TLun#%d BBT Save Stage %d Error.\n", HAL_GetMcuId(), ucTLunStage, s_aBbtSaveStage[ucTLunStage]);
            DBG_Getch();
        }
    }

    return bSaveBBTDone;
}

/*==============================================================================
Func Name  : L2_BbtLoadGBbt
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: Load BBT
Usage      : GBbt is valid -> Load Page -> Success -> CheckValidData -> Yes -> Finish
                                                                     -> No -> MskGBbtTlun -> SelectNewGBbtTLun -> Bbt-Rebuild
                                        -> Empty-Page -> Getch()
                                        -> Fail -> Bbt-Rebuild 
             GBbt is Invalid -> Bbt-Rebuild 
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
==============================================================================*/
GLOBAL BOOL MCU1_DRAM_TEXT L2_BbtLoadGBbt(void)
{
    U8 ucGlobalTLun = l_tGBbtInfo.bsGBbtTLun;
    BOOL bLoadBBTDone = FALSE;
    BBT_MANAGER *ptGBBTManager = &l_ptBBTManager[0];

    switch (ptGBBTManager->bsOpStage)
    {
        case BBT_LOAD_GBBT_INIT:
        {
            //L2_BbtMemZeroGBbt();

            ptGBBTManager->bsBlk = l_tGBbtInfo.bsGBbtBlk;
            ptGBBTManager->bsPln = START_PLN_IN_GBBT;
            if (TRUE == L2_BbtIsGBbtValid())
            {
                L2_BbtMemZeroGBbt();

                ptGBBTManager->bsPage = l_tGBbtInfo.bsGBbtPage;
                ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_READ;
            }
            else
            {                
                ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_FINISH;
            }            
            break;
        }
        case BBT_LOAD_GBBT_READ:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (FALSE == L2_FCMDQNotFull(ucGlobalTLun))
            {
                break;
            }
            
            ptReqEntry = L2_FCMDQAllocReqEntry(ucGlobalTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;
                
            ptReqEntry->tFlashDesc.bsVirBlk = ptGBBTManager->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptGBBTManager->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptGBBTManager->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

            ptReqEntry->atBufDesc[0].bsBufID = ptGBBTManager->bsBuffID + ptGBBTManager->bsPln - START_PLN_IN_GBBT;
            ptReqEntry->atBufDesc[0].bsSecStart = 0;
            ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
            
            ptReqEntry->ulSpareAddr = (U32)ptGBBTManager->pSpare;                

            *ptGBBTManager->pFlashStatus = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptGBBTManager->pFlashStatus;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
            
            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucGlobalTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_READ_WAIT;
            break;
        }
        case BBT_LOAD_GBBT_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_SUCCESS == *ptGBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RECC == *ptGBBTManager->pFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *ptGBBTManager->pFlashStatus)
            {
                if (TRUE != L2_BbtCmpSpareWithGBbt(ptGBBTManager->pSpare))
                {
                    DBG_Printf("MCU#%d TLun#%d Blk#%d Pln#%d Page#%d BBT Load Red-Mis-Match.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsBlk,ptGBBTManager->bsPln, ptGBBTManager->bsPage);

                    L2_BbtMskGBbtTLun(ucGlobalTLun);
                    L2_BbtUpdateGBbtTLun(ucGlobalTLun);
                    
                    ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_FINISH;
                    break;
                }

                ptGBBTManager->bsPln = (ptGBBTManager->bsPln + 1) % PLN_PER_LUN;
                if (0 == ptGBBTManager->bsPln)
                {
                    ptGBBTManager->bsPln = START_PLN_IN_GBBT;
                    ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_FINISH;
                }
                else
                {
                    ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_READ;
                }                
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == *ptGBBTManager->pFlashStatus)
            {
                DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Load Empty-Page.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);
                DBG_Getch();
            }
            else if (SUBSYSTEM_STATUS_FAIL == *ptGBBTManager->pFlashStatus)
            {
                DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Load Retry-Fail.\n", HAL_GetMcuId(), ucGlobalTLun, ptGBBTManager->bsPln, ptGBBTManager->bsBlk, ptGBBTManager->bsPage);

                L2_BbtMskGBbtTLun(ucGlobalTLun);
                L2_BbtUpdateGBbtTLun(ucGlobalTLun);
                
                ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_FINISH;
            }
            else
            {
                ; // wait flash status...
            }
            break;
        }        
        case BBT_LOAD_GBBT_FINISH:
        {
            bLoadBBTDone = TRUE;
            ptGBBTManager->bsPln = START_PLN_IN_GBBT;
            ptGBBTManager->bsOpStage = BBT_LOAD_GBBT_INIT;            
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Load GBbt Stage %d Error.\n", HAL_GetMcuId(), ptGBBTManager->bsOpStage);
            DBG_Getch();
        }
    }

    return bLoadBBTDone;
}

/*==============================================================================
Func Name  : L2_BbtLoad
Input      : BOOL *pbValidBBT
Output     : NONE
Return Val : BOOL
Discription: Load BBT
Usage      : 
History    :
1. 2014.10.03 steven create
2. 2015.05.25 Jason refactoring
3. 2015.07.30 Jason add bbt valid status checking
4. 2015.11.12 steven add load Lbbt stage
==============================================================================*/
GLOBAL BOOL MCU1_DRAM_TEXT L2_BbtLoad(BOOL *pbValidBBT)
{
    BOOL bBbtLoadDone = FALSE;
    LOCAL MCU12_VAR_ATTR U32 s_ulBbtLoadStage = BBT_LOAD_INIT;

    switch (s_ulBbtLoadStage)
    {
        case BBT_LOAD_INIT:
        {
            if (TRUE == L2_BbtFindTarget())
            {
                if (FALSE == l_tGBbtInfo.bsFormatGBBT)
                {
                    s_ulBbtLoadStage = BBT_LOAD_READ_GBBT;
                }
                else
                {
                    s_ulBbtLoadStage = BBT_LOAD_REBUILD;
                }
            }        
            break;
        }
        case BBT_LOAD_READ_GBBT:
        {
            if (TRUE == L2_BbtLoadGBbt())
            {
                if (TRUE == L2_BbtIsGBbtValid())
                {
                    s_ulBbtLoadStage = BBT_LOAD_READ_LBBT;
                }
                else
                {
                    s_ulBbtLoadStage = BBT_LOAD_REBUILD;
                }
            }
            break;
        }
        case BBT_LOAD_READ_LBBT:
        {
            if(TRUE == L2_BbtLoadLBbt())
            {
                s_ulBbtLoadStage = BBT_LOAD_FINISH;
                if (BBT_RDT_MARK == l_tGBbtInfo.ulBbtMark)
                {
                    s_ulBbtLoadStage = BBT_LOAD_REBUILD;
                }
            }
            break;
        }
        case BBT_LOAD_REBUILD:
        {
            if (TRUE == L2_BbtRebuild())
            {
                s_ulBbtLoadStage = BBT_LOAD_FINISH;
            }
            break;
        }
        case BBT_LOAD_FINISH:
        {
            if (NULL != pbValidBBT)
            {
                *pbValidBBT = L2_BbtCheckGBBTValid();
            }

            bBbtLoadDone = TRUE;
            s_ulBbtLoadStage = BBT_LOAD_INIT;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d BBT Load Stage %d Error.\n", HAL_GetMcuId(), s_ulBbtLoadStage);
            DBG_Getch();
        }
    }

    return bBbtLoadDone;
}

GLOBAL void MCU12_DRAM_TEXT L2_BbtSetLunSaveBBTBitMap(U8 ucTLun, BOOL BitValue)
{
    U8 ucSuperPu = L2_GET_SPU(ucTLun);
    U8 ucLunInSuperPu = L2_GET_LUN_IN_SPU(ucTLun);

    if (TRUE == BitValue)
    {
        g_pMCU12MiscInfo->aLunNeedSaveBBT[ucLunInSuperPu] |= (1 << ucSuperPu);
    }
    else
    {
        g_pMCU12MiscInfo->aLunNeedSaveBBT[ucLunInSuperPu] &= ~(1 << ucSuperPu);
    }
}

GLOBAL U32 L2_BbtGetLunSaveBBTBitMap(U8 ucLunInSuperPu)
{
    return g_pMCU12MiscInfo->aLunNeedSaveBBT[ucLunInSuperPu];
}

GLOBAL BOOL L2_BbtIsSavedDone(void)
{
    U8 ucLunInSuperPu;
    BOOL bResult = TRUE;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (0 != g_pMCU12MiscInfo->aLunNeedSaveBBT[ucLunInSuperPu])
        {
            bResult = FALSE;
            break;
        }
    }

    return bResult;
}

/*==============================================================================
Func Name  : L2_BbtSchedule
Input      : void  
Output     : NONE
Return Val : GLOBAL
Discription: 
Usage      : need update for vt3533. Lun number may be > 32, and re-set lun
History    : 
    1. 2015.12.16 JasonGuo create function
==============================================================================*/
GLOBAL void L2_BbtSchedule(void)
{
    U8 ucSuperPu;
    static U8 s_ucLunInSuperPu = 0;
    static U32 s_ulLunForSaveBBT[LUN_NUM_PER_SUPERPU] = { 0 };
    
    s_ucLunInSuperPu %= LUN_NUM_PER_SUPERPU;
    for (; s_ucLunInSuperPu < LUN_NUM_PER_SUPERPU; s_ucLunInSuperPu++)
    {
        ucSuperPu = HAL_CLZ(s_ulLunForSaveBBT[s_ucLunInSuperPu]);
        if (32 == ucSuperPu)
        {
            s_ulLunForSaveBBT[s_ucLunInSuperPu] = L2_BbtGetLunSaveBBTBitMap(s_ucLunInSuperPu);
            ucSuperPu = HAL_CLZ(s_ulLunForSaveBBT[s_ucLunInSuperPu]);
        }

        if (32 != ucSuperPu)
        {           
            s_ulLunForSaveBBT[s_ucLunInSuperPu] &= ~(1 << (31 - ucSuperPu));
            if (TRUE == L2_BbtSave(L2_GET_TLUN(31 - ucSuperPu, s_ucLunInSuperPu), INVALID_2F))
            {
                L2_BbtSetLunSaveBBTBitMap(L2_GET_TLUN(31 - ucSuperPu, s_ucLunInSuperPu), FALSE);
            }
        }
    }

    return;
}


/////////////////////////////////////////////////////////////////////////////
// Vender define interfaces
/////////////////////////////////////////////////////////////////////////////
/****************************************************************************
Name        :Ven_BbtSave
Input       :void
Output      :
Author      :steven chang
Date        :20150410
Description :BBT Ven_BBT_Save
Others      :
Modify      :
****************************************************************************/
GLOBAL void MCU12_DRAM_TEXT Ven_BbtSave(void)
{
    L2_BbtInit();

    while (TRUE != L2_BbtSave(INVALID_2F, INVALID_2F))
    {
        //L3_Scheduler();
    }

    return;
}

/****************************************************************************
Name        :Ven_BbtLoad
Input       :BOOL
Output      :
Author      :steven chang
Date        :20150410
Description :BBT Ven_BBT_Load
Others      :
Modify      :
****************************************************************************/
GLOBAL BOOL MCU12_DRAM_TEXT Ven_BbtLoad(void)
{
    BOOL bValidBBT;

    while (TRUE != L2_BbtLoad(&bValidBBT))
    {
        //L3_Scheduler();
    } 
    
    return bValidBBT;
}

/****************************************************************************
Name        :L2_BbtGetLBbtBadBlkBit_Ext
Input       :U8 ucTLun, U8 ucPln, U16 usBlock
Output      :
Author      :steven chang
Date        :20151104
Description :get ext lbbt info
Others      :
Modify      :
****************************************************************************/
GLOBAL U8 MCU12_DRAM_TEXT L2_BbtGetLBbtBadBlkBit_Ext(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U8 ucByteValue;
    U32 ulBitPos, ulBytePos;

    ulBitPos = EXT_BAD_BLK_OFFSET + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = (g_pMCU12MiscInfo->ulLBBT + ucTLun * LBBT_BUF_SIZE) + ulBitPos / 2;

    ucByteValue = *(volatile U8 *)ulBytePos;
    ucByteValue &= (0xF << ((ulBitPos % 2)*4));

    return (ucByteValue <= BAD_BLK_TYPE_NUM) ? ucByteValue : (ucByteValue>>4);
}

/****************************************************************************
Name        :L2_BbtSetLBbtBadBlkBit_Ext
Input       :U8 ucTLun, U8 ucPln, U16 usBlock, U8 BadBlkType, U8 ucErrType
Output      :
Author      :steven chang
Date        :20151103
Description :set lbbt ext info
Others      :
Modify      :
****************************************************************************/
GLOBAL void MCU12_DRAM_TEXT L2_BbtSetLBbtBadBlkBit_Ext(U8 ucTLun, U8 ucPln, U16 usBlock, U8 BadBlkType, U8 ucErrType)
{
    U8 ucByteValue, ucBadBlkInfo;
    U32 ulBitPos, ulBytePos;

    ulBitPos = EXT_BAD_BLK_OFFSET + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = (g_pMCU12MiscInfo->ulLBBT + ucTLun * LBBT_BUF_SIZE) + ulBitPos / 2;
    ucBadBlkInfo = (BadBlkType << 2) | (ucErrType);

    ucByteValue = *(volatile U8 *)ulBytePos;
    ucByteValue &= ~(0xF << ((ulBitPos % 2)*4));
    ucByteValue |= (ucBadBlkInfo << ((ulBitPos % 2)*4));
    *(volatile U8 *)ulBytePos = ucByteValue;

    if(*(volatile U8 *)ulBytePos > (2 << 4))
    {
        //DBG_Printf("MCU#%d add LBBT ext info LUN:%d, Blk:%d, Pln:%d, Ext info:%d\n", HAL_GetMcuId(), ucTLun, usBlock, ucPln, *(volatile U8 *)ulBytePos >> 4);
    }
    else
    {
        //DBG_Printf("MCU#%d add LBBT ext info LUN:%d, Blk:%d, Pln:%d, Ext info:%d\n", HAL_GetMcuId(), ucTLun, usBlock, ucPln, *(volatile U8 *)ulBytePos);
    }
#ifdef L2_BBT_UNI_TEST
    L2_BbtTestSetLBbtBadBlkBit_Ext(ucTLun, ucPln, usBlock, BadBlkType, ucErrType);
#endif
    return;

}

