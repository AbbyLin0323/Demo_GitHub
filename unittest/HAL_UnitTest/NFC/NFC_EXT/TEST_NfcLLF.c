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
Filename    : TEST_NfcLLF.c
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : compile by MCU1
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcLLF.h"
#include "FW_Event.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL BLOCK_INFO_TABLE *g_pBIT;
LOCAL MCU12_VAR_ATTR volatile U32 l_ulBbtStatusAddr;

extern GLOBAL MCU12_VAR_ATTR volatile U8 *g_pLocalCacheStatus; // Allocated from OTFB, for req sts update
extern GLOBAL MCU12_VAR_ATTR U32 g_ulGBBT;
extern GLOBAL MCU12_VAR_ATTR BOOL g_bLocalBootUpOk;

/*------------------------------------------------------------------------------
    EXTERN FUNCTIONS
------------------------------------------------------------------------------*/
extern void MCU12_DRAM_TEXT L2_TableBlock_LLF(BOOL bKeepEraseCnt);
extern GLOBAL void MCU12_DRAM_TEXT L2_BbtEnablePbnBindingTable(void);

LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtMemoryInit(void)
{
    HAL_DMAESetValue(g_ulGBBT, COM_MemSize16DWAlign(GBBT_BUF_SIZE), 0);
    
    l_ulBbtStatusAddr = DRAM_START_ADDRESS + 0x10000000;//for erase update status in bbt build
}

LOCAL void TEST_NfcBbtEraseBlk(U8 ucTLun, U8 ucPln, U16 usBlk, U32 *pStatus)
{
    U8 ucWptr;
    FCMD_REQ_PRI eFCmdPri = 0;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucWptr = TEST_NfcFCmdQGetReqWptr(ucTLun, eFCmdPri);   
    TEST_NfcFCmdQSetReqEntryRdy(ucTLun, eFCmdPri, ucWptr);
    
    ptReqEntry = TEST_NfcFCmdQAllocReqEntry(ucTLun, eFCmdPri, ucWptr);
    ptReqEntry->bsTLun = ucTLun;
    ptReqEntry->bsFCmdPri  = eFCmdPri;
    ptReqEntry->bsReqPtr   = ucWptr;
       
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
    ptReqEntry->bsTableReq = TRUE;
    
    ptReqEntry->tFlashDesc.bsVirBlk = usBlk;
    ptReqEntry->tFlashDesc.bsVirPage = 0;
    ptReqEntry->tFlashDesc.bsPlnNum = ucPln;  
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
                    
    if (NULL != pStatus)
    {
        *(volatile U32*)pStatus = SUBSYSTEM_STATUS_PENDING;
        ptReqEntry->ulReqStsAddr = (U32)pStatus;
        ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
    }

    TEST_NfcFCmdQAdaptPhyBlk(ptReqEntry);
    TEST_NfcFCmdQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

}

LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtFormatLocalBBT(void)
{
    U8 ucTLun, ucPln;
    volatile U32 *pStatus;

    pStatus = (volatile U32*)g_pLocalCacheStatus;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            TEST_NfcBbtEraseBlk(ucTLun, ucPln, LBBT_BLK, (U32*)pStatus);

            while (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                ;
            }

            if (SUBSYSTEM_STATUS_FAIL != *pStatus && SUBSYSTEM_STATUS_SUCCESS != *pStatus)
            {
                DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Local-BBT InvalidStatus.\n"
                , ucTLun, ucPln, LBBT_BLK);
                DBG_Getch();
            }

            if (SUBSYSTEM_STATUS_FAIL == *pStatus)
            {
                DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Local-BBT Ers-Fail.\n"
                , ucTLun, ucPln, LBBT_BLK);
                DBG_Getch();
            }
            //DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Local-BBT Done.\n", ucTLun, ucPln, LBBT_BLK); 
        }
    }
}


LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtFormatGlobalBBT(void)
{
    U8 ucTLun, ucPln;
    volatile U32 *pStatus;

    pStatus = (volatile U32*)g_pLocalCacheStatus;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            TEST_NfcBbtEraseBlk(ucTLun, ucPln, GBBT_BLK, (U32*)pStatus);

            while (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                ;
            }

            if (SUBSYSTEM_STATUS_FAIL != *pStatus && SUBSYSTEM_STATUS_SUCCESS != *pStatus)
            {
                DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT InvalidStatus.\n"
                ,  ucTLun, ucPln, GBBT_BLK);
                DBG_Getch();
            }

            if (SUBSYSTEM_STATUS_FAIL == *pStatus)
            {
                DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT Ers-Fail.\n"
                ,  ucTLun, ucPln, GBBT_BLK);
                //L2_BbtMskGBbtTLun(ucTLun);
                //L2_BbtUpdateGBbtTLun(ucTLun);
            }
            //DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Global-BBT Done.\n", ucTLun, ucPln, GBBT_BLK); 
        }

    }
}


LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtFormatBBT(void)
{
    TEST_NfcBbtFormatLocalBBT();
    
    TEST_NfcBbtFormatGlobalBBT();
}

LOCAL void MCU12_DRAM_TEXT TEST_NfcBbtSetGBbtBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;

    ulBitPos = ucTLun * PLN_PER_LUN * BBT_BLK_PER_PLN + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = g_ulGBBT + ulBitPos / 8;
    *(volatile U8 *)ulBytePos |= (1 << (ulBitPos % 8));

    return;
}

LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtAddBbtBadBlkSinglePlane(U8 ucTLun, U16 usPhyBlk, U8 ucPlane, U8 BadBlkType, U8 ucErrType)
{
    TEST_NfcBbtSetGBbtBadBlkBit(ucTLun, ucPlane, usPhyBlk);
    if(0 != BadBlkType)
    {
        //L2_BbtSetLBbtBadBlkBit_Ext(ucTLun, ucPlane, usPhyBlk, BadBlkType, ucErrType);
    }

    return;
}

//single pln erase
LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtEraseWholeDisk(void)
{
    U8 ucTLun, ucPln;
    U16 usVirBlk;
    volatile U32 *pStatus;
    pStatus = (volatile U32*)g_pLocalCacheStatus;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (usVirBlk = GBBT_BLK + 1; usVirBlk < LOCAL_BLK_PER_PLN; usVirBlk++)
        {
            for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
            {
                TEST_NfcBbtEraseBlk(ucTLun, ucPln, usVirBlk, (U32*)pStatus);

                while (SUBSYSTEM_STATUS_PENDING == *pStatus)
                {
                    ;
                }
                if (SUBSYSTEM_STATUS_FAIL == *pStatus)
                {
                    // single plane erase fail, add the plane block to BBT
                    TEST_NfcBbtAddBbtBadBlkSinglePlane(ucTLun, usVirBlk, ucPln, RT_BAD_BLK, ERASE_ERR);
                    DBG_Printf("TLun#%d Pln#%d Blk#%d BBT single plane erase fail\n", ucTLun, ucPln, usVirBlk);
                }
                else if(SUBSYSTEM_STATUS_FAIL != *pStatus && SUBSYSTEM_STATUS_SUCCESS != *pStatus)
                {
                    // invalid status error
                    DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Format-Whole-Disk InvalidStatus.\n", ucTLun, ucPln, usVirBlk);
                    DBG_Getch();
                }
                //L2_BbtAddBadBlkManual();//pending???
                //DBG_Printf("TLun#%d Pln#%d Blk#%d BBT Erase Done.\n", ucTLun, ucPln, usVirBlk); 
            }
        }
    }
}


LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtSetPbnBindingTable(U8 ucTlun, U16 usPbn, U8 ucPlane, U16 usPlaneBlock)
{
    U16* pusTargetBindingBlockAddress =(U16*)(
            g_pMCU12MiscInfo->ulPbnBindingTable // starting address
            + (ucTlun * (BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16))) // LUN offset
            + (usPbn * PLN_PER_LUN * sizeof(U16)) // PBN offset
            + (ucPlane * sizeof(U16)) // plane offset 
            );
    *pusTargetBindingBlockAddress = usPlaneBlock;

    return;
}

//PBN binding pending
LOCAL void MCU1_DRAM_TEXT TEST_NfcBbtBuildPbnBindingTable(void)
{
    U8 ucTlun;
    U16 usPbn;
    U8 ucPlane;

    for(ucTlun = 0; ucTlun < SUBSYSTEM_LUN_NUM; ucTlun++)
    {
        for(usPbn = 0; usPbn < BBT_BLK_PER_PLN; usPbn++)
        {
            for(ucPlane = 0; ucPlane < PLN_PER_LUN; ucPlane++)
            {
                TEST_NfcBbtSetPbnBindingTable(ucTlun, usPbn, ucPlane, usPbn);
            }
        }
    }

    return;
}

// un-inherit the bad block information
LOCAL void TEST_NfcBbtBuild(void)
{
    TEST_NfcBbtMemoryInit();

    TEST_NfcBbtFormatBBT();

#ifndef DISABLE_WHOLE_BLK_ERASE
    TEST_NfcBbtEraseWholeDisk();
#endif 

    // build the PBN binding table from BBT
    TEST_NfcBbtBuildPbnBindingTable();

    // enable the PBN binding table
    //l_ucIsPbnBindingTableReady = TRUE;
    L2_BbtEnablePbnBindingTable();

}

LOCAL void TEST_NfcBIT_LLF(void)
{
    U8 ucTLun;
    U16 usBlk;
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (usBlk = 0; usBlk < (BLK_PER_LUN + RSV_BLK_PER_PLN); usBlk++)
        {
            g_pBIT->aInfoPerBlk[ucTLun][usBlk].bsFree = TRUE;
        }
    }
    return;
}

LOCAL void TEST_NfcLLF(void)
{
    /* LLF BBT in DRAM */
    TEST_NfcBbtBuild();
    
    /* LLF RT, PBIT and VBT in DRAM */
    L2_TableBlock_LLF(FALSE);

    /* LLF BIT in DRAM */
    TEST_NfcBIT_LLF();
}


void MCU1_DRAM_TEXT TEST_NfcLocalLLF(void)
{ 
    TEST_NfcLLF();
        
    g_bLocalBootUpOk = TRUE;
    
    *g_pLLFDone = LLF_DONE;                 //for L3 performance test

    return;
}


/* end of this file */
