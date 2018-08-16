/*******************************************************************************
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
Filename    : TEST_NfcPattExtInsert.c
Version     : Ver 1.0
Author      : abby
Date        : 20160907
Description : 
Others      :
Modify      :
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattExtInsert.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
LOCAL U8 g_pInsertFCmdType;
GLOBAL U8 g_ucAlterStep = 1;
GLOBAL U32 g_ulFakeChkListStartAddr, g_ulFakeChkListEndAddr;  //start and end address of fake check list

extern GLOBAL MCU12_VAR_ATTR WRITE_DATA_BUFF g_tWrBuf;
extern MCU12_VAR_ATTR U16 g_aWrBufId[DATA_PATT_NUM][FCMDQ_DEPTH][PHYPG_PER_PRG];

extern GLOBAL MCU12_VAR_ATTR NFC_FCMD_MONITOR *g_pLocalFCmdMonitor;
extern GLOBAL BLOCK_INFO_TABLE *g_pBIT;

extern BOOL MCU12_DRAM_TEXT L2_IsSLCBlock(U8 ucSuperPu, U16 VBN);
extern U16 TEST_NfcGetPrgVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode);
extern U16 TEST_NfcGetReadVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode);
extern U8 TEST_NfcGetTLCPrgBufNum(U16 usLogicPage, BOOL bSLCMode);
extern U16 TEST_NfcGetTlcPairPage(U16 usPhyPage);

//simple GC
LOCAL void TEST_NfcInsertErase(FCMD_REQ_ENTRY *pOldFCmdReq)
{   
    pOldFCmdReq->bsReqType            = FCMD_REQ_TYPE_ERASE;
    pOldFCmdReq->tFlashDesc.bsVirPage = 0;
}

//before CCL
LOCAL void TEST_NfcInsertRead(FCMD_REQ_ENTRY *pOldFCmdReq)
{
    pOldFCmdReq->bsReqType     = FCMD_REQ_TYPE_READ;
    pOldFCmdReq->bsReqSubType  = FCMD_REQ_SUBTYPE_NORMAL;
    pOldFCmdReq->bsForceCCL    = FALSE;
}

LOCAL void TEST_NfcInsertSwitchBlkMode(FCMD_REQ_ENTRY *pOldFCmdReq)
{
    FCMD_REQ_ENTRY tFCmdReqEntry = {0};
    BOOL bSLCMod;

    bSLCMod = (FCMD_REQ_SLC_BLK == pOldFCmdReq->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
    
    tFCmdReqEntry.bsTLun        = pOldFCmdReq->bsTLun;
    tFCmdReqEntry.bsReqPtr      = pOldFCmdReq->bsReqPtr;
    tFCmdReqEntry.bsFCmdPri     = pOldFCmdReq->bsFCmdPri;
    tFCmdReqEntry.bsBootUpOk    = pOldFCmdReq->bsBootUpOk;
    
    tFCmdReqEntry.tFlashDesc.bsVirBlk = pOldFCmdReq->tFlashDesc.bsVirBlk;
    tFCmdReqEntry.tFlashDesc.bsPhyBlk = tFCmdReqEntry.tFlashDesc.bsPhyBlk;
    tFCmdReqEntry.tFlashDesc.bsBlkMod = pOldFCmdReq->tFlashDesc.bsBlkMod;

    pOldFCmdReq->ulReqStsAddr = 0;

    *pOldFCmdReq = tFCmdReqEntry;
}

//before read
void TEST_NfcInsertWrite(FCMD_REQ_ENTRY *pOldFCmdReq)
{
    U8  ucTLun;
    U16 usPhyBlk;
    
    ucTLun = pOldFCmdReq->bsTLun;
    usPhyBlk = TEST_NfcFCmdQGetPhyBlk(pOldFCmdReq);

    pOldFCmdReq->bsReqType = FCMD_REQ_TYPE_WRITE;
}

void MCU1_DRAM_TEXT TEST_NfcInsertFCmdInit(void)
{
    g_pInsertFCmdType = NO_INSERT;
}

LOCAL BOOL TEST_NfcIsForceCCLRead(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReq)
{
    BOOL bIsHit = FALSE;
    BOOL bSLCMode = TEST_NfcIsSLCMode(&pFCmdReq->tFlashDesc);
    U16  usPhyBlk = TEST_NfcFCmdQGetPhyBlk(pFCmdReq);
    U16  usPhyPage = TEST_NfcGetReadPhyPageFromVirPage(pFCmdReq->tFlashDesc.bsVirPage, bSLCMode);
    
    if (pFCmdReq->bsForceCCL)
    {
        if ((g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsPhyBlk != usPhyBlk)
            || (g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsPhyPage != usPhyPage)
            || (g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsReqType != pFCmdReq->bsReqType)
            || (g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsReqSubType != pFCmdReq->bsReqSubType))
        {
            bIsHit = TRUE;
        }
    }
    return bIsHit;
}

LOCAL BOOL TEST_NfcIsHitSwitchBlkMode(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucCurBlkMod)
{
    BOOL bIsHit = FALSE;

    /* if continuous cmd is aim to the different blk mode, then need switch mode */
    if(ucCurBlkMod != g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsBlkMod)
    {
        bIsHit = TRUE;
    }

    //pending, move to L3
    bIsHit = FALSE;
    
    return bIsHit;
}

U32 TEST_NfcIsNeedInsertFCmd(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReq)
{
    U16  usPhyBlk;
    BOOL bIsNeedInsert = FALSE;
    FCMD_REQ_TYPE eFCmdType;
    FCMD_FLASH_DESC tFlashDes = {0};
    BLOCK_INFO tBIT = {0};

    eFCmdType = pFCmdReq->bsReqType;
    usPhyBlk  = TEST_NfcFCmdQGetPhyBlk(pFCmdReq);
    tBIT = g_pBIT->aInfoPerBlk[ucTLun][usPhyBlk];

    switch (eFCmdType)
    {
        case FCMD_REQ_TYPE_ERASE:
        {
            bIsNeedInsert = FALSE;
        }break;

        case FCMD_REQ_TYPE_READ:
        {
            FCMD_FLASH_DESC tFlashDes = pFCmdReq->tFlashDesc;
            BOOL bSLCMode = L2_IsSLCBlock(L2_GET_SPU(ucTLun), pFCmdReq->tFlashDesc.bsVirBlk);
            U16 usReadPPO = TEST_NfcGetReadPhyPageFromVirPage(tFlashDes.bsVirPage, bSLCMode);

            if ((U32)(usReadPPO+1) > tBIT.bsPPO)//read ppo > write ppo
            {
                g_pInsertFCmdType = INSERT_DUMMY_WRITE;
                bIsNeedInsert = TRUE;
                //DBG_Printf("TLun %d PhyBlk %d read empty phypage %d need INSERT_DUMMY_WRITE\n",ucTLun,usPhyBlk,usReadPPO);
            }
            /* if can't hit change column read conditions, then insert normal read */
            else if (TRUE == TEST_NfcIsForceCCLRead(ucTLun, eFCmdPri, pFCmdReq))
            {
                g_pInsertFCmdType = INSERT_READ_BEFORE_CCL;
                bIsNeedInsert = TRUE;
                //DBG_Printf("TLun %d CCL phypage %d Need INSERT_READ_BEFORE_CCL\n",ucTLun,usReadPPO);
            }
                       
        }break;
        
        /* for write: read upp page or close blk GC need insert */
        case FCMD_REQ_TYPE_WRITE:
        {
            if (tBIT.bsClose)
            {
                g_pInsertFCmdType = INSERT_ERASE;
                bIsNeedInsert = TRUE;
                //DBG_Printf("TLun %d close physical blk %d Need INSERT_ERASE\n",ucTLun,usPhyBlk);
            }
        }break;

        default:
        {
            DBG_Printf("Insert FCMD not support type %d\n", eFCmdType);
            DBG_Getch();
        }
    }
    return bIsNeedInsert;
}

void TEST_NfcHandleInsertFCmd(U8 ucTLun, FCMD_REQ_ENTRY *pFCmdReq)
{
    switch (g_pInsertFCmdType)
    {
        case INSERT_ERASE:
        {
            TEST_NfcInsertErase(pFCmdReq);
            //DBG_Printf("TLun%d INSERT_ERASE\n",ucTLun);
        }break;
     
        case INSERT_DUMMY_WRITE:
        {
            TEST_NfcInsertWrite(pFCmdReq);
            //DBG_Printf("TLun%d INSERT_DUMMY_WRITE\n",ucTLun);
        }break;

        case INSERT_READ_BEFORE_CCL:
        {
            TEST_NfcInsertRead(pFCmdReq);
            //DBG_Printf("TLun%d INSERT_READ_BEFORE_CCL\n",ucTLun);
        }break;
      
        default:
        {
            DBG_Printf("Insert FCmd Type %d isn't Supported!\n", g_pInsertFCmdType);
            DBG_Getch();
        }
    }
    /* clear insert flag */
    g_pInsertFCmdType = NO_INSERT;
}

LOCAL void TEST_NfcFakeExtChkListErase(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVirBlk)
{
    FCMD_REQ_ENTRY tFCmdReq = {0};

    tFCmdReq.bsTLun = ucTLun;
    tFCmdReq.bsReqType = FCMD_REQ_TYPE_ERASE;
    tFCmdReq.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;

    tFCmdReq.tFlashDesc.bsVirBlk = usVirBlk;

    pChkListFile->tFCmdReqEntry = tFCmdReq;

    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
}

LOCAL void TEST_NfcFakeExtChkListWrite(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN, U16 usPrgOrder, BOOL bSLCMode)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_WRITE;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    
    tReqEntry.tFlashDesc.bsVirBlk = usVBN;
    tReqEntry.tFlashDesc.bsVirPage = TEST_NfcGetPrgVirPageFromOrder(usPrgOrder, bSLCMode);
        
    pChkListFile->tFCmdReqEntry = tReqEntry;
    
    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
}

LOCAL void TEST_NfcFakeExtChkListRead(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN
    , U16 usRdPgOrder, U8 ucSecStart, U8 ucSecLen, BOOL bSLCMode)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    /* config buffer id */    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_READ;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;

    tReqEntry.tFlashDesc.bsVirBlk = usVBN;
    tReqEntry.tFlashDesc.bsVirPage = TEST_NfcGetReadVirPageFromOrder(usRdPgOrder, bSLCMode);;
    tReqEntry.tFlashDesc.bsSecStart = ucSecStart;
    tReqEntry.tFlashDesc.bsSecLen = ucSecLen;
    
    tReqEntry.atBufDesc[0].bsSecStart = ucSecStart;
    tReqEntry.atBufDesc[0].bsSecLen = ucSecLen;

    pChkListFile->tFCmdReqEntry = tReqEntry;

    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
    
    return;
}

LOCAL void TEST_NfcFakeExtChkListMergeRead(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN
    , U16 usRdPgOrder, U8 ucSecStart, U8 ucLpnBitMap, BOOL bSLCMode)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    /* config buffer id */    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_READ;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;

    tReqEntry.tFlashDesc.bsVirBlk = usVBN;
    tReqEntry.tFlashDesc.bsVirPage = TEST_NfcGetReadVirPageFromOrder(usRdPgOrder, bSLCMode);;
    
    tReqEntry.tFlashDesc.bsSecStart = ucSecStart;
    tReqEntry.tFlashDesc.bsMergeRdEn = TRUE;
    tReqEntry.tFlashDesc.bsLpnBitmap = ucLpnBitMap;
    
    tReqEntry.atBufDesc[0].bsSecStart = ucSecStart;

    pChkListFile->tFCmdReqEntry = tReqEntry;

    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
    
}

LOCAL void TEST_NfcFakeExtChkListCCL(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN
    , U16 usRdPgOrder, BOOL bSLCMode)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    /* config buffer id */    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_READ;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    tReqEntry.bsForceCCL = TRUE;
    
    tReqEntry.tFlashDesc.bsVirBlk = usVBN;
    tReqEntry.tFlashDesc.bsVirPage = TEST_NfcGetReadVirPageFromOrder(usRdPgOrder, bSLCMode);;
    tReqEntry.tFlashDesc.bsSecStart = 0;
    tReqEntry.tFlashDesc.bsSecLen = SEC_PER_BUF;
    
    tReqEntry.atBufDesc[0].bsSecStart = 0;
    tReqEntry.atBufDesc[0].bsSecLen = SEC_PER_BUF;

    pChkListFile->tFCmdReqEntry = tReqEntry;

    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
    
    return;
}

//for intel 3d tlc
LOCAL void TEST_NfcFakeExtChkListCopySLCRead(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN, U16 usTlcPage)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_READ;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;

    tReqEntry.tFlashDesc.bsVirBlk = usVBN + usTlcPage/PG_PER_SLC_BLK;
    tReqEntry.tFlashDesc.bsVirPage = usTlcPage % PG_PER_SLC_BLK;
    tReqEntry.tFlashDesc.bsSecStart = 0;
    tReqEntry.tFlashDesc.bsSecLen = SEC_PER_BUF;
    
    tReqEntry.atBufDesc[0].bsSecStart = 0;
    tReqEntry.atBufDesc[0].bsSecLen = SEC_PER_BUF;

    pChkListFile->tFCmdReqEntry = tReqEntry;

    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;
}

LOCAL void TEST_NfcFakeExtChkListCopyTLCWrite(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usVBN, U16 usTlcPage)
{
    FCMD_REQ_ENTRY tReqEntry = {0};
    
    tReqEntry.bsTLun = ucTLun;
    tReqEntry.bsReqType = FCMD_REQ_TYPE_WRITE;
    tReqEntry.bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    
    tReqEntry.tFlashDesc.bsVirBlk = usVBN;
    tReqEntry.tFlashDesc.bsVirPage = usTlcPage;
        
    pChkListFile->tFCmdReqEntry = tReqEntry;
    
    pChkListFile->tFlieAttr.bsFileType = EXT_CHK_LIST;
    pChkListFile->tFlieAttr.bsLastFile = 0;  
}

//for intel 3d tlc
LOCAL U32 TEST_NfcFakeExtChkListCopyExternal(CHECK_LIST_EXT_FILE *pChkListFile, U8 ucTLun, U16 usSlcVBN, U16 usTlcVBN, U16 usPrgOrder)
{
    U8 ucPrgPageNum;
    U16 usTlcPage;
    U32 ulFileIdx = 0;
    
    usTlcPage = TEST_NfcGetPrgVirPageFromOrder(usPrgOrder, FALSE);
    ucPrgPageNum = TEST_NfcGetTLCPrgBufNum(usTlcPage, FALSE);

    /* read SLC first: SLC blk = usSlcVBN/usSlcVBN+1/usSlcVBN+2 */
    if (1 == ucPrgPageNum)
    {
        TEST_NfcFakeExtChkListCopySLCRead(pChkListFile + ulFileIdx++, ucTLun, usSlcVBN, usTlcPage);
    }
    else if(2 == ucPrgPageNum)
    {
        TEST_NfcFakeExtChkListCopySLCRead(pChkListFile + ulFileIdx++, ucTLun, usSlcVBN, usTlcPage);

        U16 usNextTlcPage = TEST_NfcGetTlcPairPage(usTlcPage);
        TEST_NfcFakeExtChkListCopySLCRead(pChkListFile + ulFileIdx++, ucTLun, usSlcVBN, usNextTlcPage);
    }
    
    /* then write TLC */
    TEST_NfcFakeExtChkListCopyTLCWrite(pChkListFile + ulFileIdx++, ucTLun, usTlcVBN, usTlcPage);

    return ulFileIdx;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcFakeSeqE
Description: 
    Erase all tested BLK 
Usage:  
    TEST CASE in SEQ performance test
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeSeqE(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN = TEST_BLOCK_START;
    U32 ulFileIdx = 0;   
  
    while(usVBN < TEST_BLOCK_END)
    {
        ucTLun = TEST_TLUN_START;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            TEST_NfcFakeExtChkListErase(pCheckList + ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;
            ucTLun++; 
        }
        usVBN++;
    }

    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeSeqW
Description: 
    Write whole BLK 
Usage:  
    TEST CASE in SEQ write test
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcFakeSeqW(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun = TEST_TLUN_START, ucTLunIdx;
    U16 usVBN = TEST_BLOCK_START;
    U16 usPrgOrder = 0, usPrgOrderMax;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;
    U16 usWrPage[SUBSYSTEM_LUN_MAX] = {0};
       
    bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
    usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
    
    while(usVBN < TEST_BLOCK_END)
    {
        for (ucTLunIdx = TEST_TLUN_START; ucTLunIdx < SUBSYSTEM_LUN_NUM; ucTLunIdx++)
        {
            usWrPage[ucTLunIdx] = 0;
        }
        usPrgOrder = 0;
        while(usPrgOrder < usPrgOrderMax)
        {
            ucTLun = TEST_TLUN_START;
            while (ucTLun < SUBSYSTEM_LUN_NUM)
            {
                for (usPrgOrder = usWrPage[ucTLun]; usPrgOrder < usWrPage[ucTLun] + g_ucAlterStep; usPrgOrder++)
                {           
                    TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                    ulFileIdx++;
                }
                usWrPage[ucTLun] = usPrgOrder;
                ucTLun++; 
                if ((ucTLun == SUBSYSTEM_LUN_NUM)&&(usPrgOrder == usPrgOrderMax))
                    break;
                
                if (ucTLun == SUBSYSTEM_LUN_NUM)
                {
                    ucTLun = TEST_TLUN_START;
                }
            }
        }
        usVBN++;
    }

    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeSeqR
Description: 
    Read whole BLK, be careful: don't read empty page 
Usage:  
    TEST CASE in SEQ Read test
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcFakeSeqR(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun = TEST_TLUN_START, ucTLunIdx, ucSecLen;
    U16 usVBN = TEST_BLOCK_START;
    U16 usRdOrder = 0, usRdOrderMax;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;
    U16 usRdPage[SUBSYSTEM_LUN_MAX] = { 0 };
    
    bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
    usRdOrderMax = 8;//TEST_NfcGetReadOrderMax(bSLCMode);
    
    ucSecLen = (bSLCMode) ? SEC_PER_BUF : SEC_PER_LOGIC_PIPE_PG;
    
    while(usVBN < TEST_BLOCK_END)
    {
        for (ucTLunIdx = TEST_TLUN_START; ucTLunIdx < SUBSYSTEM_LUN_NUM; ucTLunIdx++)
        {
            usRdPage[ucTLunIdx] = 0;
        }
        usRdOrder = 0;
        while (usRdOrder < usRdOrderMax)
        {
            ucTLun = TEST_TLUN_START;
            while (ucTLun < SUBSYSTEM_LUN_NUM)
            {
                for (usRdOrder = usRdPage[ucTLun]; usRdOrder < usRdPage[ucTLun] + g_ucAlterStep; usRdOrder++)
                {           
                    TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, ucSecLen, bSLCMode);
                    ulFileIdx++;
                }
                usRdPage[ucTLun] = usRdOrder;
                ucTLun++; 
                
                if (ucTLun == SUBSYSTEM_LUN_NUM)
                {
                    break;
                }
            }
        }
        usVBN++;
    }
    return ulFileIdx;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcFakeRandR
Description: 
    random read align in 4K 
Usage:  
    TEST CASE in SEQ Read test
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcFakeRandR(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun = TEST_TLUN_START, ucTLunIdx, ucSecStart, ucSecLen;
    U16 usVBN = TEST_BLOCK_START, usRdPage[SUBSYSTEM_LUN_MAX] = { 0 };
    U16 usRdOrder = 0, usRdOrderMax = 0;
    U32 ulFileIdx = 0, ulRandLoop = 0;
    BOOL bSLCMode;
    
    bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
    usRdOrderMax = TEST_NfcGetReadOrderMax(bSLCMode);
       
    while(usVBN < TEST_BLOCK_END)
    {
        for (ucTLunIdx = TEST_TLUN_START; ucTLunIdx < SUBSYSTEM_LUN_NUM; ucTLunIdx++)
        {
            usRdPage[ucTLunIdx] = 0;
        }
        usRdOrder = 0;
        while (usRdOrder < usRdOrderMax)
        {
            ucTLun = TEST_TLUN_START;
            while (ucTLun < SUBSYSTEM_LUN_NUM)
            {
                for (usRdOrder = usRdPage[ucTLun]; usRdOrder < usRdPage[ucTLun] + g_ucAlterStep; usRdOrder++)
                { 
                    ucSecStart = rand()%57;
                    ucSecLen = 8;//4K req 

                    TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, ucSecStart, ucSecLen, bSLCMode);
                    ulFileIdx++;
                }
                usRdPage[ucTLun] = usRdOrder;
                ucTLun++; 
                if ((ucTLun == SUBSYSTEM_LUN_NUM)&&(usRdOrder == usRdOrderMax))
                    break;
                    
            }
        }
        usVBN++;
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtEWR
Description: 
    Multi-LUN : Erase all TLUN -> Write whole BLK -> RD some pages
Usage:  
    TEST CASE in SLC BLK
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtEWR(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN, usPrgOrder, usPrgOrderMax, usRdOrder, usRdPageMax;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;
    
    usVBN = TEST_BLOCK_START;
    
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = TEST_TLUN_START;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
            usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
            usRdPageMax = TEST_NfcGetReadOrderMax(bSLCMode);
            
            /* Erase blk */
            TEST_NfcFakeExtChkListErase(pCheckList+ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;

            /* Program whole blk */
            for (usPrgOrder = 0; usPrgOrder < usPrgOrderMax; usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }

            /* Read whole blk */
            for (usRdOrder = 0; usRdOrder < usRdPageMax; usRdOrder++)
            {
                TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, SEC_PER_BUF, bSLCMode);
                ulFileIdx++;
            }
            
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtMixWR
Description: 
    CASE1: PRG PU0->PRG PU1->RD PU0->PRG PU0->RD PU1->PRG PU1
    CASE2: RD PU0->PRG PU1->RD PU1->PRG PU0
Usage:  
    TEST CASE
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtMixWR(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun, ucSecStart, ucSecLen;
    U16 usVBN, usPrgOrder, usPrgOrderMax, usRdOrder, usRdPageMax;
    U16 usWrPage[SUBSYSTEM_LUN_MAX] = { 0 };
    U16 usRdPage[SUBSYSTEM_LUN_MAX] = { 0 };
    U32 ulFileIdx = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            /* Erase & Program some pages in all LUN first */
            usWrPage[ucTLun] = 0;
            usRdPage[ucTLun] = 0;
            //usVBN = rand()% (BLK_PER_LUN - 16);
            bSLCMode = L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
            usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
            usRdPageMax = TEST_NfcGetReadOrderMax(bSLCMode);
            
            TEST_NfcFakeExtChkListErase(pCheckList+ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;
            
            for (usPrgOrder = usWrPage[ucTLun]; usPrgOrder < (U16)(usWrPage[ucTLun] + 10); usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }
            usWrPage[ucTLun] = usPrgOrder;
                   
            ucTLun++;
        }
        /* MIX RW: RD PU0->PRG PU0->RD PU1->PRG PU1  */
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            /* Read SLC blk */
            for (usRdOrder = usRdPage[ucTLun]; usRdOrder < (U16)(usRdPage[ucTLun] + 2); usRdOrder++)
            {
                ucSecStart = rand()%(SEC_PER_BUF-1);
                ucSecLen = rand()%(SEC_PER_BUF - ucSecStart);
                ucSecLen = (0 == ucSecLen)? 1 : ucSecLen; //if seclen = 0, force to 1
                TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, ucSecStart, ucSecLen, bSLCMode);
                ulFileIdx++;
            }
            usRdPage[ucTLun] = usRdOrder;
            
            /* Program SLC blk */
            for (usPrgOrder = usWrPage[ucTLun]; usPrgOrder < (U16)(usWrPage[ucTLun] + 5); usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }
            usWrPage[ucTLun] = usPrgOrder;
            
            ucTLun++;
        }
    
        /* MIX RW: RD PU0->PRG PU1->RD PU1->PRG PU0  */
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            if (EVEN == ucTLun%2)
            {
                /* Read even PU SLC blk */
                for (usRdOrder = usRdPage[ucTLun]; usRdOrder < (U16)(usRdPage[ucTLun] + 5); usRdOrder++)
                {
                    ucSecStart = rand()%(SEC_PER_BUF-1);
                    ucSecLen = rand()%(SEC_PER_BUF - ucSecStart);
                    ucSecLen = (0 == ucSecLen)? 1 : ucSecLen; //if seclen = 0, force to 1
                    TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, ucSecStart, ucSecLen, bSLCMode);
                    ulFileIdx++;
                }
                usRdPage[ucTLun] = usRdOrder;
            }
            else
            {
                /* Program odd PU SLC blk */
                for (usPrgOrder = usWrPage[ucTLun]; usPrgOrder < (U16)(usWrPage[ucTLun] + 5); usPrgOrder++)
                {
                    TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                    ulFileIdx++;
                }
                usWrPage[ucTLun] = usPrgOrder;
            }
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtAbnormal
Description: 
    CASE1: do not care page number
    CASE2: do not care BLK mode
    CASE3: do not care BLK number(handle table blk hit case)
Usage:  
    ABNORMAL TEST CASE
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtAbnormal(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN, usPrgOrder, usPrgOrderMax, usRdOrder, usRdPageMax;
    U32 ulFileIdx = 0, ulPageNum = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;//rand()% BLK_PER_LUN;
    
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            //usVBN = rand()% (BLK_PER_LUN - 16);
            bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
            usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
            usRdPageMax = TEST_NfcGetReadOrderMax(bSLCMode);
            
            /* Erase */
            TEST_NfcFakeExtChkListErase(pCheckList+ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;
            
            /* Program */
            for (ulPageNum = 0; ulPageNum < usPrgOrderMax; ulPageNum++)
            {
                usPrgOrder = rand() % usPrgOrderMax;
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }

            /* Read */
            for (ulPageNum = 0; ulPageNum < usRdPageMax; ulPageNum++)
            {
                usRdOrder = rand() % usRdPageMax;
                TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, SEC_PER_BUF, bSLCMode);
                ulFileIdx++;
            }
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtRewite
Description: 
    rewrite one blk, need handle by pattern gen
Usage:  
    test insert erase
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtRewite(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN, usPrgOrder, usPrgOrderMax;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;
    
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            bSLCMode = L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
            usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
            
            /* Program SLC whole blk */
            for (usPrgOrder = 0; usPrgOrder < usPrgOrderMax; usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }

            /* continue program target SLC whole blk */
            TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
            ulFileIdx++;
            
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;

}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtShiftRead
Description: 
    unsafe page shift read, useless in TSB 2D TLC
Usage:  
    Test shift read enable
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtShiftRead(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN, usPrgOrder, usRdOrder;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;
    
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {        
            bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);

            /* Erase SLC blk */
            TEST_NfcFakeExtChkListErase(pCheckList+ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;
            
            /* Program SLC whole blk */
            for (usPrgOrder = 0; usPrgOrder < 10; usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }

            /* Read a pair page */
            usRdOrder = 9;
            TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, SEC_PER_BUF, bSLCMode);
            ulFileIdx++;
            
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtForceCCL
Description: 
    Force to do change column read
Usage:  
    Test insert normal read before CCL
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtForceCCL(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usVBN, usRdOrder;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;
    
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {       
            bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);

            /* noraml read page 0 */
            usRdOrder = 0;
            TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, SEC_PER_BUF, bSLCMode);
            ulFileIdx++;

            /* CCL read page 1 */
            usRdOrder = 1;
            TEST_NfcFakeExtChkListCCL(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, bSLCMode);
            ulFileIdx++;
           
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtMergeRead
Description: 
    Force to do change column read
Usage:  
    Test insert normal read before CCL
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtMergeRead(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun, ucSecStart, ucLpnBitMap;
    U16 usVBN, usPrgOrder, usRdOrder;
    U32 ulFileIdx = 0;
    BOOL bSLCMode;

    usVBN = TEST_BLOCK_START;
    while (usVBN < TEST_BLOCK_END)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        { 
            bSLCMode = L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
            
            /* Program */
            for (usPrgOrder = 0; usPrgOrder < 5; usPrgOrder++)
            {
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
                ulFileIdx++;
            }

            /* Read */
            for (usRdOrder = 0; usRdOrder < 1; usRdOrder++)
            {
                ucSecStart = rand()%(SEC_PER_BUF-8);
                ucLpnBitMap = rand()%0xFF;
                //DBG_Printf("Merge Read Page %d ucSecStart %d ucLpnBitMap 0x%x\n", tFlashAddr.usPage, ucSecStart, ucLpnBitMap);
                TEST_NfcFakeExtChkListMergeRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, ucSecStart, ucLpnBitMap, bSLCMode);
                ulFileIdx++;
            }
           
            ucTLun++;
        }
        usVBN++;
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtAllInOne
Description: 
    Multi-LUN : Erase all TLUN -> Write whole BLK -> RD some pages
Usage:  
    TEST CASE in SLC BLK
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcFakeExtAllInOne(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun, ucReqType;
    U16 usVBN, usPrgOrder, usPrgOrderMax, usRdOrder, usRdPageMax;
    U32 ulFileIdx = 0, ulCnt = 3000;
    BOOL bSLCMode;
    
    while (ulCnt--)
    {
        usVBN = rand()% (BLK_PER_PLN - TABLE_BLOCK_COUNT - 1);//(BLK_PER_LUN - 22);
        ucTLun = rand() % SUBSYSTEM_LUN_NUM;
        bSLCMode= L2_IsSLCBlock(L2_GET_SPU(ucTLun), usVBN);
        usPrgOrderMax = TEST_NfcGetPrgOrderMax(bSLCMode);
        usRdPageMax = TEST_NfcGetReadOrderMax(bSLCMode);
        ucReqType = rand() % NFC_OTHER_CMD;         //0-W;1-R;2-E

        /* Erase */
        if (NFC_ERASE_CMD == ucReqType)//erase
        {
            TEST_NfcFakeExtChkListErase(pCheckList+ulFileIdx, ucTLun, usVBN);
            ulFileIdx++;
        }
        
        /* Program  */
        if (NFC_WRITE_CMD == ucReqType)//write
        {
            usPrgOrder = rand() % usPrgOrderMax;
            TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx, ucTLun, usVBN, usPrgOrder, bSLCMode);
            ulFileIdx++;
        }
        
        /* Read */
        if (NFC_READ_CMD == ucReqType)//read
        {
            usRdOrder = rand() % usRdPageMax;
            TEST_NfcFakeExtChkListRead(pCheckList + ulFileIdx, ucTLun, usVBN, usRdOrder, 0, SEC_PER_BUF, bSLCMode);
            ulFileIdx++;
        }
        
    }
    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtCopyPrepare
Description: 
    Erase dest blk and write 3 SLC BLK
Usage:  
    TEST CASE in TLC copy test
History:
    20161103    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcFakeExtCopyPrepare(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun = 0, ucSlcBlkIdx;
    U16 usPrgOrder;
    U32 ulFileIdx = 0;
    U16 usSlcVBN, usTlcVBN;
    
    usSlcVBN = TLC_BLK_MAX + 10;
    usTlcVBN = TLC_BLK_MAX - 20;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (ucSlcBlkIdx = 0; ucSlcBlkIdx < PG_PER_WL; ucSlcBlkIdx++)
        {
            /* erase SLC blk */
            TEST_NfcFakeExtChkListErase(pCheckList + ulFileIdx++, ucTLun, usSlcVBN+ucSlcBlkIdx);
            /* write SLC blk */
            for (usPrgOrder = 0; usPrgOrder < SLC_PG_PER_BLK; usPrgOrder++)
            { 
                TEST_NfcFakeExtChkListWrite(pCheckList + ulFileIdx++, ucTLun, usSlcVBN+ucSlcBlkIdx, usPrgOrder, TRUE);
            }
        }
        /* erase TLC blk */
        TEST_NfcFakeExtChkListErase(pCheckList + ulFileIdx++, ucTLun, usTlcVBN);
    }

    return ulFileIdx;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtCopy
Description: 
    Copy 3 SLC BLK to 1 TLC BLK
Usage:  
    TEST CASE in TLC copy test
History:
    20161103    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcFakeExtCopy(CHECK_LIST_EXT_FILE *pCheckList)
{
    U8  ucTLun;
    U16 usPrgOrder = 0, usPrgOrderMax;
    U32 ulFileIdx = 0;
    U16 usSlcVBN, usTlcVBN;
    U16 usWrTlcPage[SUBSYSTEM_LUN_MAX] = {0};
    
    usPrgOrderMax = TEST_NfcGetPrgOrderMax(FALSE);
    usSlcVBN = TLC_BLK_MAX + 10;
    usTlcVBN = TLC_BLK_MAX - 20;

    while(usPrgOrder < usPrgOrderMax)
    {
        ucTLun = 0;
        while (ucTLun < SUBSYSTEM_LUN_NUM)
        {
            for (usPrgOrder = usWrTlcPage[ucTLun]; usPrgOrder < usWrTlcPage[ucTLun] + 1; usPrgOrder++)
            { 
                ulFileIdx += TEST_NfcFakeExtChkListCopyExternal(pCheckList + ulFileIdx, ucTLun, usSlcVBN, usTlcVBN, usPrgOrder);
            }
            usWrTlcPage[ucTLun] = usPrgOrder;
            ucTLun++; 
        }
    }

    return ulFileIdx;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcFakeExtChecklist
Description: 
    Fake case to test pattern gen
Usage:  
History:
    20160919    abby   create.
------------------------------------------------------------------------------*/
void TEST_NfcFakeExtChecklist(CHECK_LIST_EXT_FILE *pCheckList)
{
    U32 ulAddrOff;
    g_ulFakeChkListStartAddr = (U32)pCheckList;
    
#ifdef MIX_VECTOR
    ulAddrOff = TEST_NfcFakeExtAllInOne(pCheckList);
    pCheckList += ulAddrOff;

    ulAddrOff = TEST_NfcFakeSeqE(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeSeqW(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeExtMixWR(pCheckList);
    pCheckList += ulAddrOff;
 #if 0   
    ulAddrOff = TEST_NfcFakeExtAbnormal(pCheckList);
    pCheckList += ulAddrOff;

    ulAddrOff = TEST_NfcFakeExtRewite(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeExtForceCCL(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeExtMergeRead(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeExtAllInOne(pCheckList);
    pCheckList += ulAddrOff;
#endif    

#endif

#if 0// only for SLC cache solution like B0KB
    ulAddrOff = TEST_NfcFakeExtCopyPrepare(pCheckList);
    pCheckList += ulAddrOff;
    
    ulAddrOff = TEST_NfcFakeExtCopy(pCheckList);
    pCheckList += ulAddrOff;
#endif

    g_ulFakeChkListEndAddr = (U32)pCheckList;
    (pCheckList-1)->tFlieAttr.bsLastFile = 1;

}


//------- end of this file --------//

