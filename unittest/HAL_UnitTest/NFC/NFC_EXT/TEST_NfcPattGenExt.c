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
Filename    : TEST_NfcPattGenExt.c
Version     : Ver 1.0
Author      : abby
Data        : 20160903
Description :
Others      :
Modify      :
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattGenExt.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR volatile U8 *g_pLocalCacheStatus; // Allocated from OTFB, for req sts update

extern GLOBAL MCU12_VAR_ATTR TLC_PRG_TABLE *g_pTLCPrgTable; // Allocated from shared Dsram1, for TLC write cntl
extern GLOBAL MCU12_VAR_ATTR TLC_P2L_PG_MAP *g_pTLCP2LPageMap; // Allocated from shared Dsram1, phy to logic mapping

extern GLOBAL MCU12_VAR_ATTR NFC_FCMD_MONITOR *g_pLocalFCmdMonitor;
extern GLOBAL MCU12_VAR_ATTR NFC_PENDING_FCMDQ *g_pLocalFCmdPend;

extern GLOBAL MCU12_VAR_ATTR volatile U32 *g_pDataCheckBMP;
extern GLOBAL MCU12_VAR_ATTR volatile U32 *g_pPendBMP;

extern GLOBAL MCU12_VAR_ATTR BOOL g_bLocalBootUpOk;
extern GLOBAL BLOCK_INFO_TABLE *g_pBIT;

/*------------------------------------------------------------------------------
    EXT FUNCTIONS
------------------------------------------------------------------------------*/
extern GLOBAL BOOL MCU12_DRAM_TEXT L2_BbtIsGBbtBadBlock(U8 ucTLun, U16 usPhyBlk);
extern void MCU1_DRAM_TEXT TEST_NfcExtDataDRAMMemMap(U32 *pFreeSharedDRAMBase);
extern FCMD_REQ_ENTRY TEST_NfcGetFCMDFromExtCheckList(CHECK_LIST_EXT_FILE *pChkListFile);
extern BOOL TEST_NfcHasPendingFCmd(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
extern void TEST_NfcFCmdQSetPendingBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
extern void TEST_NfcFCmdQClrPendingBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
extern FCMD_REQ_ENTRY TEST_NfcFCmdQGetPendingEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
extern void TEST_NfcFCmdUpdateMonitor(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReqEntry);
extern U16 TEST_NfcGetPhyPageMax(BOOL bSLCMode);
extern void TEST_NfcAdaptWriteBuffAddr(U8 ucTLun, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode);
extern void TEST_NfcFCmdQAdaptWriteSecRange(FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode);
extern U16 TEST_NfcGetNextPPO(U16 usCurrPPO, U8 ucPrgCycle, BOOL bSLCMode);
extern void MCU1_DRAM_TEXT TEST_NfcDummyDataInit(void);

/*------------------------------------------------------------------------------
    FUNCTIONS DEFINITION
------------------------------------------------------------------------------*/
LOCAL void MCU1_DRAM_TEXT TEST_NfcExtFlagInit(void)
{
    /* TLC mode flag init */
#ifdef TLC_MODE_TEST
    g_bTlcMode = TRUE;
#else
    g_bTlcMode = FALSE;
#endif

#ifdef DATA_EM_ENABLE
    g_bEmEnable = TRUE;
#else
    g_bEmEnable = FALSE;
#endif
    
    TEST_NfcInitLLFFlag();
}

void MCU1_DRAM_TEXT TEST_NfcCacheStatusOTFBMap(U32 *pCacheStatusOTFBBase)
{
    U32 ulFreeBase = *pCacheStatusOTFBBase;

    g_pLocalCacheStatus = (U8*)ulFreeBase;
    COM_MemZero((U32*)g_pLocalCacheStatus, L1_CACHESTATUS_TOTAL/sizeof(U32));
    COM_MemIncBaseAddr(&ulFreeBase, L1_CACHESTATUS_TOTAL);
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    ASSERT((ulFreeBase - OTFB_SSU1_MCU12_SHARE_BASE) <= OTFB_MCU12_SHARED_SSU1_SIZE);
    
    DBG_Printf("MCU#%d NFC EXT UT Alloc shared SRAM1 0x%x B, Rsvd 0x%x B.\n"
    ,HAL_GetMcuId(),ulFreeBase - *pCacheStatusOTFBBase, OTFB_SSU1_MCU12_SHARE_BASE - (ulFreeBase - *pCacheStatusOTFBBase));

    // update the OTFB base address
    *pCacheStatusOTFBBase = ulFreeBase;
    
    return;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcLocalFCmdAddrInit
Description: assign address to local FCMD from L2 FCMDQ
Return Value:
Usage:       
History:
    20160905    abby   create.
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT TEST_NfcLocalFCmdAddrInit(void)
{
    g_pLocalFCmdReq     = g_ptFCmdReq;
    g_pLocalFCmdReqDptr = g_ptFCmdReqDptr;
    g_pLocalFCmdReqSts  = g_ptFCmdReqSts;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcExtSharedDSRAM1MemMap
Description: assign address and clear the memory
             g_pLocalFCmdPend/g_pPendBMP/g_pLocalFCmdMonitor/g_pDataCheckBMP
Input Param: shared DSRAM1 free base
Output Param:
Return Value:
Usage:       only for MCU1 pattern gen ext init
History:
    20160905    abby   create.
------------------------------------------------------------------------------*/
LOCAL void MCU1_DRAM_TEXT TEST_NfcExtSharedDSRAM1MemMap(U32 *pFreeSharedSRAM1Base)
{
    U32 ulFreeBase = *pFreeSharedSRAM1Base;
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    #ifdef HOST_NVME
    ulFreeBase += HAL_CHAIN_NUM_MANAGER_SIZE;
    COM_MemAddr16DWAlign(&ulFreeBase);
    #endif

    /*  init pending FCMD and pending bitmap*/
    g_pLocalFCmdPend = (NFC_PENDING_FCMDQ *)ulFreeBase;
    COM_MemZero((U32*)g_pLocalFCmdPend, sizeof(NFC_PENDING_FCMDQ)/sizeof(U32));
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(NFC_PENDING_FCMDQ));
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_pPendBMP = (volatile U32*)ulFreeBase;
    COM_MemZero((U32*)g_pPendBMP, SUBSYSTEM_LUN_MAX / 8);
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_MAX/8);
    COM_MemAddr16DWAlign(&ulFreeBase);

    /*  init FCMD monitor */
    g_pLocalFCmdMonitor = (NFC_FCMD_MONITOR *)ulFreeBase;
    COM_MemZero((U32*)g_pLocalFCmdMonitor, sizeof(NFC_FCMD_MONITOR)<<2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(NFC_FCMD_MONITOR));
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    /*  init data check bitmap   */
    g_pDataCheckBMP = (volatile U32*)ulFreeBase;
    COM_MemZero((U32*)g_pDataCheckBMP, SUBSYSTEM_LUN_MAX<<2);
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_MAX);
    COM_MemAddr16DWAlign(&ulFreeBase);

#if 0
    /*  init TLC program control table   */
    g_pTLCPrgTable = (TLC_PRG_TABLE*)ulFreeBase;
    COM_MemZero((U32*)g_pTLCPrgTable, sizeof(TLC_PRG_TABLE)<<2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(TLC_PRG_TABLE));
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    /*  init TLC phy to logic page mapping   */
#ifndef FLASH_MICRON_3DTLC_B16
    g_pTLCP2LPageMap = (TLC_P2L_PG_MAP*)ulFreeBase;
    COM_MemZero((U32*)g_pTLCP2LPageMap, sizeof(TLC_P2L_PG_MAP)<<2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(TLC_P2L_PG_MAP));
    COM_MemAddr16DWAlign(&ulFreeBase);
#endif
#endif
    ASSERT(ulFreeBase - DSRAM1_MCU12_SHARE_BASE <= DSRAM1_MCU12_SHARE_SIZE);
    
    DBG_Printf("MCU#%d NFC EXT UT Alloc shared SRAM1 0x%x B, Rsvd 0x%x B.\n"
    ,HAL_GetMcuId(),ulFreeBase - *pFreeSharedSRAM1Base, DSRAM1_MCU12_SHARE_SIZE - (ulFreeBase - *pFreeSharedSRAM1Base));

    *pFreeSharedSRAM1Base = ulFreeBase;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcExtDSRAM1MemMap
Description: assign address and clear the memory
             g_pLocalFCmdPend/g_pPendBMP/g_pLocalFCmdMonitor/g_pDataCheckBMP
Input Param: shared DSRAM1 free base
Output Param:
Return Value:
Usage:       only for MCU1 pattern gen ext init
History:
    20160905    abby   create.
------------------------------------------------------------------------------*/
LOCAL void MCU1_DRAM_TEXT TEST_NfcExtDSRAM1MemMap(U32 *pFreeSRAM1Base)
{
    U32 ulFreeBase = *pFreeSRAM1Base;
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    /*  init TLC program control table   */
    g_pTLCPrgTable = (TLC_PRG_TABLE*)ulFreeBase;
    COM_MemZero((U32*)g_pTLCPrgTable, sizeof(TLC_PRG_TABLE)<<2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(TLC_PRG_TABLE));
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    /*  init TLC phy to logic page mapping   */
#ifndef FLASH_MICRON_3DTLC_B16
    g_pTLCP2LPageMap = (TLC_P2L_PG_MAP*)ulFreeBase;
    COM_MemZero((U32*)g_pTLCP2LPageMap, sizeof(TLC_P2L_PG_MAP)<<2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(TLC_P2L_PG_MAP));
    COM_MemAddr16DWAlign(&ulFreeBase);
#endif

    //ASSERT(ulFreeBase - DSRAM1_MCU1_BASE <= DSRAM1_MCU1_SIZE);
    
    DBG_Printf("MCU#%d NFC EXT UT Alloc shared SRAM1 0x%x B, Rsvd 0x%x B.\n"
    ,HAL_GetMcuId(),ulFreeBase - *pFreeSRAM1Base, DSRAM1_MCU1_SIZE - (ulFreeBase - *pFreeSRAM1Base));

    *pFreeSRAM1Base = ulFreeBase;
}

LOCAL void MCU1_DRAM_TEXT TEST_NfcExtShared16DWAlignDramMemMap(U32 *pFreeSharedDramBase)
{
    U32 ulDramOffSet = 0;
    U32 ulFreeBase = *pFreeSharedDramBase;

    COM_MemAddr16DWAlign(&ulFreeBase);

#ifdef DCACHE
    ulDramOffSet = DRAM_HIGH_ADDR_OFFSET;
#endif   
    /*  init block info table   */
    g_pBIT = (BLOCK_INFO_TABLE *)(ulFreeBase + ulDramOffSet);
    COM_MemZero((U32*)g_pBIT, sizeof(BLOCK_INFO_TABLE)>>2);
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(BLOCK_INFO_TABLE));
    COM_MemAddr16DWAlign(&ulFreeBase);

    // OverFlow Check and Update Free Base
    ASSERT((ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE) <= DRAM_BUFF_MCU12_SIZE);

    DBG_Printf("MCU#%d NFC EXT UT Alloc the shared-16DwAlign Dram size 0x%x B, Rsvd 0x%x B.\n"
        ,HAL_GetMcuId(),ulFreeBase - *pFreeSharedDramBase, DRAM_BUFF_MCU12_SIZE - (ulFreeBase - *pFreeSharedDramBase));
    
    *pFreeSharedDramBase = ulFreeBase;
}

LOCAL void MCU1_DRAM_TEXT TEST_NfcExtSharedMemMap(SUBSYSTEM_MEM_BASE *pFreeMemBase)
{   
    /* Local FCmd Pointer Init */
    TEST_NfcLocalFCmdAddrInit();
    
    /* g_pLocalCacheStatus */
    TEST_NfcCacheStatusOTFBMap(&pFreeMemBase->ulFreeCacheStatusBase);
    
    /* g_pLocalFCmdPend/g_pPendBMP/g_pLocalFCmdMonitor/g_pDataCheckBMP/g_pBIT */
    TEST_NfcExtSharedDSRAM1MemMap(&pFreeMemBase->ulFreeSRAM1SharedBase);

    TEST_NfcExtDSRAM1MemMap(&pFreeMemBase->ulFreeSRAM1Base);
    
    /* g_pBIT */
    TEST_NfcExtShared16DWAlignDramMemMap(&pFreeMemBase->ulFreeDRAMSharedBase);

    /* checklist files MCU1 dram */
    TEST_NfcCheckListAllocDram(&pFreeMemBase->ulDRAMBase);

    /* g_pWrRed & g_pRdRed & g_pWrBufBaseAddr & g_pRdBufBaseAddr */
    TEST_NfcExtDataDRAMMemMap(&pFreeMemBase->ulDRAMBase);
    
}

void MCU1_DRAM_TEXT TEST_NfcExtPattGenInit(void)
{
    /* flag init */
    TEST_NfcExtFlagInit();

    /*  allocate memory  */
    TEST_NfcExtSharedMemMap(&g_FreeMemBase);

    /*  init FCMD insert */
    TEST_NfcInsertFCmdInit();

    /*  init TLC program table */
    TEST_NfcTLCPrgTableInit();
    TEST_NfcTLCP2LMapInit();
    
    /* prepare dummy data and RED */
    TEST_NfcDummyDataInit();

    /* LLF status init */
    g_bLocalBootUpOk = FALSE;
    
    DBG_Printf("Ext Pattern Gen Init Done!\n\n");
}


U16 TEST_NfcExtSearchGoodBlk(U16 usPhyBlk, U8 ucTLun)
{
    BOOL bBadBlk = FALSE;

    //if is bad blk, get next block until it's good
    for (; usPhyBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN); usPhyBlk++)
    {
        bBadBlk = L2_BbtIsGBbtBadBlock(ucTLun, usPhyBlk);
        if (bBadBlk)
        {
            continue;
        }
        return usPhyBlk;
    }

    return INVALID_4F;
}

LOCAL U16 TEST_NfcSearchGoodBlk(U16 usPhyBlk, U8 ucTLun)
{
    U16 usNewPhyBlk;

    //search a new not bad blk from BBT if current phyBlk is bad
    usNewPhyBlk = TEST_NfcExtSearchGoodBlk(usPhyBlk, ucTLun);
    if (INVALID_4F == usNewPhyBlk)
    {
        DBG_Printf("No Free Block to Push FCMD!!!\n");
        DBG_Getch();
    }
    
    return usNewPhyBlk;
}

//pending
LOCAL U16 TEST_NfcSearchValidBlk(U8 ucTLun, U16 usPhyBlk)
{
    U16 usValidBlk = usPhyBlk + 1;

    while (FALSE == (g_pBIT->aInfoPerBlk[ucTLun][usValidBlk].bsValid))
    {
        usValidBlk++;         
    }
    return usValidBlk;
}


//not care insert cmd handle in fault tolerant
LOCAL void TEST_NfcFCmdFaultTolerant(U8 ucTLun, FCMD_REQ_TYPE eFCmdType, FCMD_REQ_ENTRY *pFCmdReq)
{
    U16 usPhyBlk, usVirBlk, usVirPage;
    BOOL bSLCMode;
    BLOCK_INFO tBIT = { 0 };

    usPhyBlk = pFCmdReq->tFlashDesc.bsPhyBlk;
    usVirBlk = pFCmdReq->tFlashDesc.bsVirBlk;
    usVirPage = pFCmdReq->tFlashDesc.bsVirPage;
    bSLCMode = TEST_NfcIsSLCMode(&pFCmdReq->tFlashDesc);
    tBIT = g_pBIT->aInfoPerBlk[ucTLun][usPhyBlk];

    switch (eFCmdType)
    {
        case FCMD_REQ_TYPE_ERASE:
        {
            /* if target blk is bad, search a new good blk */
            pFCmdReq->tFlashDesc.bsPhyBlk = TEST_NfcSearchGoodBlk(usPhyBlk, ucTLun);
            if (pFCmdReq->tFlashDesc.bsPhyBlk != usPhyBlk)
            {
                DBG_Printf("TLun%d Erase Blk%d is Bad, Change Blk to %d\n"
                , ucTLun, usPhyBlk, pFCmdReq->tFlashDesc.bsPhyBlk);
            }
        }break;

        case FCMD_REQ_TYPE_WRITE:
        {
            /* if target blk is bad, search a new good blk */
            pFCmdReq->tFlashDesc.bsPhyBlk = TEST_NfcSearchGoodBlk(usPhyBlk, ucTLun);
            if (pFCmdReq->tFlashDesc.bsPhyBlk != usPhyBlk)
            {
                DBG_Printf("TLun%d Write Blk%d is Bad, Change Blk to %d\n"
                , ucTLun, usPhyBlk, pFCmdReq->tFlashDesc.bsPhyBlk);
            }
         
            /* if write order is not write, force write page to PPO */
            tBIT = g_pBIT->aInfoPerBlk[ucTLun][pFCmdReq->tFlashDesc.bsPhyBlk];
            U16 usPhyPage = TEST_NfcGetPrgPhyPageFromVirPage(usVirPage, bSLCMode);
            U8  ucPrgCycle = HAL_FlashGetTlcPrgCycle(usVirPage);
          
            if (tBIT.bsPPO != usPhyPage)
            {
                pFCmdReq->tFlashDesc.bsVirPage = TEST_NfcGetPrgVirPageFromPhyPage(tBIT.bsPPO,ucPrgCycle,bSLCMode);
                //DBG_Printf("TLun%d Blk%d PPO %d Write PhyPage %d Invalid, Change VirPage to %d\n"
                    //, ucTLun, pFCmdReq->tFlashDesc.bsPhyBlk, tBIT.bsPPO, usPhyPage, pFCmdReq->tFlashDesc.bsVirPage);
            }
        }break;
        
        case FCMD_REQ_TYPE_READ:
        {
            /* if target blk is not valid(empty blk), search a new valid blk */
            if (!tBIT.bsValid)
            {
                pFCmdReq->tFlashDesc.bsPhyBlk = TEST_NfcSearchValidBlk(ucTLun, usPhyBlk); 
                if (pFCmdReq->tFlashDesc.bsPhyBlk != usPhyBlk)
                {
                    DBG_Printf("TLun%d Read Blk%d is not valide, Change Blk to %d\n"
                    , ucTLun, usPhyBlk, pFCmdReq->tFlashDesc.bsPhyBlk);
                }
            }
        }break;

        default:
        {
            DBG_Printf("Fatal error, can't handle, pls check checklist!\n");
            DBG_Getch();
        }
    }
}

U32 TEST_NfcExtParamCheck(FCMD_REQ_ENTRY *pFCmdReqEntry)
{
    BOOL bSLCMode;
    U16 usPageMax;

    bSLCMode = TEST_NfcIsSLCMode(&pFCmdReqEntry->tFlashDesc);
    //usPageMax = TEST_NfcGetReadOrderMax(bSLCMode);
    
    ASSERT(pFCmdReqEntry->bsTLun < SUBSYSTEM_LUN_NUM);
    ASSERT(pFCmdReqEntry->tFlashDesc.bsVirBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN));
    ASSERT(pFCmdReqEntry->tFlashDesc.bsPhyBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN));
    //ASSERT(pFCmdReqEntry->tFlashDesc.bsVirPage < usPageMax);
    ASSERT(pFCmdReqEntry->tFlashDesc.bsPlnNum < PLN_PER_LUN);
        
    return SUCCESS;
}

LOCAL U32 TEST_NfcGetReqStsAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    U16 usIndex = ((((ucTLun<<1)|eFCmdPri)<<2)|ucLevel);
    U32 ulAddrBase = (U32)g_pLocalCacheStatus;
       
    if (usIndex >= L1_CACHESTATUS_TOTAL)
    {
        usIndex %= L1_CACHESTATUS_TOTAL;
    }
    
    return (U32)(ulAddrBase + usIndex);
}

void TEST_NfcAdaptReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry)
{
    volatile U8 *pStatus;

    pStatus = (volatile U8*)(TEST_NfcGetReqStsAddr(ucTLun, eFCmdPri, ucWptr));
    *pStatus = (U8)SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus; 
#ifdef UT_QD1
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
#else    
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;
#endif
}

LOCAL void TEST_NfcAdaptShiftReadEn(U8 ucTLun, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    U16 usPhyBlk;
    U16 usReadPPO;
    BLOCK_INFO tBIT = {0};

    usReadPPO = TEST_NfcGetReadPhyPageFromVirPage(ptReqEntry->tFlashDesc.bsVirPage, bSLCMode);
    usPhyBlk = TEST_NfcFCmdQGetPhyBlk(ptReqEntry);
    tBIT = g_pBIT->aInfoPerBlk[ucTLun][usPhyBlk];

    if (tBIT.bsOpen)//open blk
    {
        /* unsafe page need shift read */
        if (FALSE == HAL_IsFlashPageSafed(L3_GET_PU(ucTLun), (tBIT.bsPPO-1), usReadPPO))
        {
            ptReqEntry->tFlashDesc.bsShiftRdEn = TRUE;
            //DBG_Printf("TLun %d unsafe phypage %d enable shift read\n", ucTLun, usReadPPO);
        }
    }
}

void TEST_NfcFillBackFCMDQ(U8 ucTLun, U8 ucWptr, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReqEntry)
{
    BOOL bSLCMode;
    FCMD_REQ_TYPE eFCmdType;

    /* set commmon element:  bsFCmdPri/bsReqPtr/bsBootUpOk/bsBlkMod/bsPhyBlk/ulReqStsAddr/bsReqUptMod */
    pFCmdReqEntry->bsFCmdPri  = eFCmdPri;
    pFCmdReqEntry->bsReqPtr   = ucWptr;
    pFCmdReqEntry->bsBootUpOk = g_bLocalBootUpOk;
    
    //flash descriptor
    bSLCMode = L2_IsSLCBlock(L2_GET_SPU(ucTLun), pFCmdReqEntry->tFlashDesc.bsVirBlk);
    pFCmdReqEntry->tFlashDesc.bsBlkMod = (TRUE == bSLCMode)? FCMD_REQ_SLC_BLK : FCMD_REQ_TLC_BLK;
    TEST_NfcFCmdQAdaptPhyBlk(pFCmdReqEntry);

    //req status
    TEST_NfcAdaptReqSts(ucTLun, eFCmdPri, ucWptr, pFCmdReqEntry);

    /* set custom element according req type: ulSpareAddr/bsBufID/bsSecStart/bsSecLen */
    eFCmdType = pFCmdReqEntry->bsReqType;

#ifndef ACC_EXT_TEST
    /* FCMD fault tolerant */
    TEST_NfcFCmdFaultTolerant(ucTLun, eFCmdType, pFCmdReqEntry);
#endif

    switch (eFCmdType)
    {
        case FCMD_REQ_TYPE_ERASE:
        {
            ;//do nothing
        }break;

        case FCMD_REQ_TYPE_WRITE:
        {
            TEST_NfcFCmdQAdaptWriteSecRange(pFCmdReqEntry, bSLCMode);
            TEST_NfcAdaptWriteBuffAddr(ucTLun, ucWptr, pFCmdReqEntry, bSLCMode);
        }break;
        
        case FCMD_REQ_TYPE_READ:
        {
            TEST_NfcAdaptReadBuffAddr(ucTLun, ucWptr, pFCmdReqEntry, bSLCMode);
        #ifndef ACC_DEBUG
            TEST_NfcAdaptShiftReadEn(ucTLun, pFCmdReqEntry, bSLCMode);
        #endif
        }break;

        default:
        {
            DBG_Printf("Fatal error, FCMD type error!\n");
            DBG_Getch();
        }
    }
}

void TEST_NfcUpdateBIT(U8 ucTLun, FCMD_REQ_ENTRY *pFCmdReq)
{
    BOOL bSLCMode;
    BLOCK_INFO *pBlkInfo;
    FCMD_FLASH_DESC tFlashDesc = pFCmdReq->tFlashDesc;
    
    pBlkInfo  = &(g_pBIT->aInfoPerBlk[ucTLun][tFlashDesc.bsPhyBlk]);
    bSLCMode  = TEST_NfcIsSLCMode(&tFlashDesc);
    
    switch (pFCmdReq->bsReqType)
    {
        case FCMD_REQ_TYPE_ERASE:
        {
            pBlkInfo->bsFree = TRUE;
            pBlkInfo->bsOpen = FALSE;
            pBlkInfo->bsClose = FALSE;
            pBlkInfo->bsValid = FALSE;
            pBlkInfo->bsPPO = 0;
        }break;

        case FCMD_REQ_TYPE_READ:
        {
            ;
        }break;

        case FCMD_REQ_TYPE_WRITE:
        {
            pBlkInfo->bsOpen = TRUE;
            
            U16 usVirPage = tFlashDesc.bsVirPage;
            U8  ucPrgCycle = HAL_FlashGetTlcPrgCycle(usVirPage);
            U16 usWritePPO = TEST_NfcGetPrgPhyPageFromVirPage(usVirPage, bSLCMode);
            pBlkInfo->bsPPO = TEST_NfcGetNextPPO(usWritePPO, ucPrgCycle, bSLCMode);
            
            U16 usPageMax = TEST_NfcGetPhyPageMax(bSLCMode);
            if ((usPageMax - 1) == usWritePPO)
            {
                pBlkInfo->bsClose = TRUE;
                pBlkInfo->bsOpen = FALSE;
                //DBG_Printf("SLCMode %d WritePPO %d need close BLK %d\n", bSLCMode, usWritePPO, usPhyBlk);
            }
            pBlkInfo->bsValid = TRUE;
            pBlkInfo->bsFree = FALSE;
        }break;

        default:
        {
            DBG_Printf("Update BIT not support type %d\n", pFCmdReq->bsReqType);
            DBG_Getch();
        }
    }
}

LOCAL void TEST_NfcExtDbgShowFCMD(const FCMD_REQ_ENTRY *pReqEntry)
{
    FCMD_REQ_TYPE eFCmdType;
    FCMD_FLASH_DESC tFlashDec = {0};
    U8 ucTLun, ucWptr, ucPlnNum, ucBufIdx;
    U16 usPhyPage, usVirPage, usPhyBlk, usVirBlk;
    U16 usDataBufId[DSG_BUFF_SIZE] = {0};
    U32 ulDataAddr[DSG_BUFF_SIZE] = {0};
    U8  ucSecStart, ucSecLen, ucLpnBitMap;
    U32 ulRedAddr, ulStsAddr;
    BOOL bSLCMode, bMergeRead, bForceCCL, bShiftRdEn, bRedOnlyRd, bSinglePln;

    ucTLun = pReqEntry->bsTLun;
    ucWptr = pReqEntry->bsReqPtr;

    /*  flash addr  */
    tFlashDec = pReqEntry->tFlashDesc;
    bSLCMode  = TEST_NfcIsSLCMode(&tFlashDec);
    usVirBlk  = tFlashDec.bsVirBlk;
    usVirPage = tFlashDec.bsVirPage;
    usPhyBlk  = tFlashDec.bsPhyBlk;
    ucPlnNum  = tFlashDec.bsPlnNum;

    /*  RED & STS & Data buffer  */
#ifdef WITHOUT_RED
    ulRedAddr = 0;
#else
    ulRedAddr = pReqEntry->ulSpareAddr; 
#endif
    ulStsAddr = pReqEntry->ulReqStsAddr;
    for (ucBufIdx = 0; ucBufIdx < DSG_BUFF_SIZE; ucBufIdx++)
    {
        usDataBufId[ucBufIdx] = pReqEntry->atBufDesc[ucBufIdx].bsBufID;
        if (INVALID_4F == usDataBufId[ucBufIdx])
            break;
        ulDataAddr[ucBufIdx] = COM_GetMemAddrByBufferID(usDataBufId[ucBufIdx], TRUE, BUF_SIZE_BITS);
    }
    
    /* Xfer range */
    ucSecStart = tFlashDec.bsSecStart;
    ucSecLen = tFlashDec.bsSecLen;
    ucLpnBitMap = tFlashDec.bsLpnBitmap;
    
    /*  feature control  */
    bMergeRead = tFlashDec.bsMergeRdEn;
    bForceCCL  = pReqEntry->bsForceCCL;
    bShiftRdEn = tFlashDec.bsShiftRdEn;
    bRedOnlyRd = tFlashDec.bsRdRedOnly;
    bSinglePln = (FCMD_REQ_SUBTYPE_SINGLE == pReqEntry->bsReqSubType) ? TRUE : FALSE;
    eFCmdType  = pReqEntry->bsReqType;
    
    switch (eFCmdType)
    {
        case FCMD_REQ_TYPE_ERASE:
        {
        #if 1
            DBG_Printf("Erase TLUN%d VirBLK%d PhyBlk%d SLCMode%d SinglePln%d\n"
                ,ucTLun,usVirBlk,usPhyBlk,bSLCMode,bSinglePln);
        #endif
        }break;

        case FCMD_REQ_TYPE_READ:
        {
        #if 1
            usPhyPage = TEST_NfcGetReadPhyPageFromVirPage(usVirPage, bSLCMode);
            DBG_Printf("Read TLun%d VirBLK%d PhyBlk%d SLCMode%d VirPage%d PhyPage%d SinglePln%d\n"
                , ucTLun, usVirBlk, usPhyBlk, bSLCMode, usVirPage, usPhyPage, bSinglePln);
                
            //DBG_Printf("SecStart%d SecLen%d BuffId[0]=0x%x DataAddr[0]=0x%x REDAddr0x%x\n"
             //  , ucSecStart, ucSecLen, usDataBufId[0], ulDataAddr[0], ulRedAddr);
                
            if (bForceCCL)
                DBG_Printf("This Read is forced to Change Column Read\n");
                
            if (bMergeRead)
                DBG_Printf("This Read is Merge Read, SecStart%d LPN BitMap0x%x\n", ucSecStart, ucLpnBitMap);
                
            if (bShiftRdEn)
                //DBG_Printf("This Read is Not Safe, Enable Shift Read\n");

            if (bRedOnlyRd)
                DBG_Printf("This Read is RED Only\n");
        #endif
        }break;
        
        /* for write: read upp page or close blk GC need insert */
        case FCMD_REQ_TYPE_WRITE:
        {
        #if 1
            usPhyPage = TEST_NfcGetPrgPhyPageFromVirPage(usVirPage, bSLCMode);
            DBG_Printf("Write TLun%d VirBLK%d PhyBlk%d SLCMode%d VirPage%d PhyPage%d SinglePln%d\n"
                , ucTLun, usVirBlk, usPhyBlk, bSLCMode, usVirPage, usPhyPage, bSinglePln);
                
            //DBG_Printf("SecStart%d SecLen%d BuffId[0]=0x%x DataAddr[0]=0x%x REDAddr0x%x\n"
             //   , ucSecStart, ucSecLen, usDataBufId[0], ulDataAddr[0], ulRedAddr);
        #endif       
        }break;

        default:
        {
            DBG_Printf("FCMD %d DBG Show not Support\n", eFCmdType);
            DBG_Getch();
        }
    }
}
#ifdef UT_QD1
extern BOOL TEST_NfcAllTLunFCmdQIsEmpty(FCMD_REQ_PRI eFCmdPri);
#endif

void TEST_NfcFCmdManage(BOOL bHasPending, U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY tFCmdReq)
{
    U8 ucWptr;
    BOOL bSuccess = FALSE;
    FCMD_REQ_ENTRY *pFCmdReqEntry;
    
    ucWptr = TEST_NfcFCmdQGetReqWptr(ucTLun, eFCmdPri);

    /* check FCMDQ is not full */
#ifdef UT_QD1
    while (FALSE == TEST_NfcAllTLunFCmdQIsEmpty(eFCmdPri))//wait FCMDQ to be empty
    {
        ;
    }
#elif (defined(ACC_DEBUG))
    while (FALSE == TEST_NfcFCmdQIsWptrFree(ucTLun, eFCmdPri, ucWptr) && tFCmdReq.bsReqType == FCMD_REQ_TYPE_READ)
    {
        //To accelate UT and keep L3 busy, if current LUN is full then jump to next LUN 
        ucTLun = (ucTLun + 1) % SUBSYSTEM_LUN_NUM;
        tFCmdReq.bsTLun = ucTLun;
        ucWptr = TEST_NfcFCmdQGetReqWptr(ucTLun, eFCmdPri);
    }
#endif

    /* Check data and set FCMDQ ready  */
    TEST_NfcFCmdQSetReqEntryRdy(ucTLun, eFCmdPri, ucWptr);

    /* Alloc FCMDQ entry */
    pFCmdReqEntry = TEST_NfcFCmdQAllocReqEntry(ucTLun, eFCmdPri, ucWptr);
    COM_MemCpy((U32*)pFCmdReqEntry, (U32*)&tFCmdReq, sizeof(FCMD_REQ_ENTRY)>>2);
    //HAL_DMAECopyOneBlock((const U32)pFCmdReqEntry, (const U32)&tFCmdReq, (const U32)(sizeof(FCMD_REQ_ENTRY)));

#ifndef ACC_EXT_TEST
    /* Insert FCMD handle */
    /* When last FCMD pending, but current FCMD still need to insert more cmd,
    handle insert first, otherwise clear pending bitmap to end pending handling */
    if (bHasPending)
    {
        if (TRUE == TEST_NfcIsNeedInsertFCmd(ucTLun, eFCmdPri, pFCmdReqEntry))
        {
            /* config current FCMD by insert FCMD entry*/
            TEST_NfcHandleInsertFCmd(ucTLun, pFCmdReqEntry);
        }
        else
        {
            /* clear pending bitmap */
            TEST_NfcFCmdQClrPendingBitmap(ucTLun, eFCmdPri);
        }
    }
    else
    {
        if (TRUE == TEST_NfcIsNeedInsertFCmd(ucTLun, eFCmdPri, pFCmdReqEntry))
        {
            /* set pending bitmap, copy current FCMD to pending FCMD entry*/  
            TEST_NfcFCmdQSetPendingBitmap(ucTLun, eFCmdPri);
            //HAL_DMAECopyOneBlock((const U32)&(g_pLocalFCmdPend->aPendFCmd[ucTLun][eFCmdPri]), (const U32)pFCmdReqEntry, (const U32)sizeof(FCMD_REQ_ENTRY));
            COM_MemCpy((U32*)&(g_pLocalFCmdPend->aPendFCmd[ucTLun][eFCmdPri]), (U32*)pFCmdReqEntry, sizeof(FCMD_REQ_ENTRY)>>2);
        
            /* config current FCMD by insert FCMD entry*/
            TEST_NfcHandleInsertFCmd(ucTLun, pFCmdReqEntry);
        }
    }
#endif    

    /* Fill back FCMD entry partial elements */
    TEST_NfcFillBackFCMDQ(ucTLun, ucWptr, eFCmdPri, pFCmdReqEntry);

#ifndef ACC_EXT_TEST
    /* Parameters check */
    TEST_NfcExtParamCheck(pFCmdReqEntry);

    /* Set data check bitmap */
    if (TRUE == TEST_NfcIsEnableDataCheck(pFCmdReqEntry->bsReqType))
    {
        TEST_NfcSetDataCheckBitmap(ucTLun, eFCmdPri, ucWptr);
    }

    /* Update monitor */
    TEST_NfcFCmdUpdateMonitor(ucTLun, eFCmdPri, pFCmdReqEntry);
#endif

    /* Push FCMDQ  */
    TEST_NfcFCmdQPushReqEntry(ucTLun, eFCmdPri, ucWptr);

    /* Update BIT */
    TEST_NfcUpdateBIT(ucTLun, pFCmdReqEntry);

    /* Print DBG info, optional */
#ifdef DBG_SHOW_ALL
    TEST_NfcExtDbgShowFCMD((const FCMD_REQ_ENTRY *)pFCmdReqEntry);
#endif
}

BOOL TEST_NfcHandlePendingFCmd(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    FCMD_REQ_ENTRY tPendingReq = {0};
    
    /* Get Pending Entry */
    tPendingReq = TEST_NfcFCmdQGetPendingEntry(ucTLun, eFCmdPri);

    /* Pending CMD management */
    TEST_NfcFCmdManage(TRUE, ucTLun, eFCmdPri, tPendingReq);

    /* Detect if still has pending FCMD */
    return (TEST_NfcHasPendingFCmd(ucTLun, eFCmdPri));
}

void TEST_NfcIsRemainPendingFCmd(void)
{
    FCMD_REQ_PRI eFCmdPri;
    U8 ucTLun, ucWptr;
    BOOL bHasPending = FALSE;
    
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (eFCmdPri = 0; eFCmdPri < FCMD_REQ_PRI_NUM; eFCmdPri++)
        {
            for (ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
            {
                bHasPending = TEST_NfcHasPendingFCmd(ucTLun, eFCmdPri);
                while(bHasPending)
                {
                    bHasPending = TEST_NfcHandlePendingFCmd(ucTLun, eFCmdPri);
                }
            }
        }
    }
}

void TEST_NfcExtPattGen(void)
{
    U8 ucTLun;
    BOOL bIsPending = FALSE;
    FCMD_REQ_PRI eFCmdPri = 0;  //force now
    FCMD_REQ_ENTRY tFCmdReq = {0};

    /* Get checklist FCMD */
    tFCmdReq = TEST_NfcGetFCMDFromExtCheckList((CHECK_LIST_EXT_FILE*)g_pCheckListPtr);
    ucTLun = tFCmdReq.bsTLun;

#ifndef ACC_EXT_TEST
    /* Handle pending CMD */
    bIsPending = TEST_NfcHasPendingFCmd(ucTLun, eFCmdPri);
    while(bIsPending)
    {
        bIsPending = TEST_NfcHandlePendingFCmd(ucTLun, eFCmdPri);
    }
#endif

    /* CMD management */
    TEST_NfcFCmdManage(FALSE, ucTLun, eFCmdPri, tFCmdReq);

    return;
}


/*  end of this file  */

