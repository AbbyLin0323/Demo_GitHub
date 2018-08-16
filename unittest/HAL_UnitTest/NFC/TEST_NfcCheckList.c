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
Filename    : TEST_NfcCheckList.c
Version     : Ver 1.0
Author      : abby
Date        : 20160905
Description : compile by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcCheckList.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR volatile U32 *g_pCheckListPtr;  //check list file address pointer
GLOBAL MCU12_VAR_ATTR U32 *g_pFileBaseInDram;
GLOBAL volatile CHECK_LIST_LOAD_CNTR_REG *g_pLoadReg;
GLOBAL MCU12_VAR_ATTR BOOL g_bTlcMode;
GLOBAL MCU12_VAR_ATTR BOOL g_bEmEnable;
GLOBAL MCU12_VAR_ATTR BOOL g_bLocalBootUpOk;

extern GLOBAL U32 g_ulFakeChkListStartAddr, g_ulFakeChkListEndAddr;  //start and end address of fake check list

/*------------------------------------------------------------------------------
    EXTERN FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
extern void TEST_NfcAdaptReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry);
extern void TEST_NfcFCmdQSetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel, FCMD_REQ_STS eReqSts);
extern FCMD_REQ_STS TEST_NfcFCmdQGetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
extern void MCU1_DRAM_TEXT TEST_NfcLocalFCmdAddrInit(void);
#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
extern BOOL MCU12_DRAM_TEXT HAL_NfcSyncResetFlash(FLASH_ADDR *pFlashAddr);
#endif
extern INLINE void HAL_DelayUs(U32 ulUsCount);

/*------------------------------------------------------------------------------
    MAIN FUNCTIONS
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT TEST_NfcCheckListAllocDram(U32 *pFreeDRAMBase)
{
    U32 ulDramOffSet = 0;
    U32 ulFreeBase, ulBaseDram, ulDramSize;
        
    /*  allocation share DRAM memory for checklist  */
    ulFreeBase = *pFreeDRAMBase;
    COM_MemAddr16DWAlign(&ulFreeBase);
#ifdef DCACHE
    ulDramOffSet = DRAM_HIGH_ADDR_OFFSET;
#endif   

    g_pFileBaseInDram = (U32*)(ulFreeBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeBase, CHECK_LIST_FILE_SZ_MAX);
    COM_MemAddr16DWAlign(&ulFreeBase);

    ulBaseDram = DRAM_DATA_BUFF_MCU1_BASE;
    ulDramSize = DATA_BUFF_MCU1_SIZE;
    
    //ASSERT(((volatile U32)ulFreeBase - ulDramSize) <= ulDramSize);
    
    DBG_Printf("NFC Alloc checklist files Dram size %d MB, Rsvd %d MB.\n"
    ,(ulFreeBase - *pFreeDRAMBase)/(1024*1024), (ulDramSize - (ulFreeBase - *pFreeDRAMBase))/(1024*1024));

    *pFreeDRAMBase = ulFreeBase;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetCheckListFileType
Description: 
    ucFileType: 0-BASIC; 1-EXT
Usage:
    Parse file type by scripts
History:
    20160816    abby   create.
------------------------------------------------------------------------------*/
U8 TEST_NfcGetCheckListFileType(void)
{
    g_pLoadReg = (CHECK_LIST_LOAD_CNTR_REG*)CHECKLIST_LOAD_CNTR_REG;
    
#ifdef FAKE_EXT_CHKLIST
    return EXT_CHK_LIST;
#elif defined(FAKE_BASIC_CHKLIST)
    return BASIC_CHK_LIST;
#endif
    
    return g_pLoadReg->bsFileType;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetCheckListFileType
Description: 
    ucFileType: 0-BASIC; 1-EXT
Usage:
    Parse file type by scripts
History:
    20161101   abby   create.
------------------------------------------------------------------------------*/
LOCAL U8 TEST_NfcGetChecklistLoadMethod(void)
{
#ifdef SIM
    return LOAD_DISABLE;
#endif
    
    return g_pLoadReg->bsLoadMethod;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcIsChecklistLoadTrigger
Description: 
    BOOL b_IsTrig: 0-has not been trigger, need wait external trigger
Usage:
    Parse file type by scripts
History:
    20161101   abby   create.
------------------------------------------------------------------------------*/
LOCAL BOOL TEST_NfcIsChecklistLoadTrigger(void)
{
    BOOL b_IsTrig = FALSE;
    
#ifdef SIM
    b_IsTrig = TRUE;
#endif

    if (1 == g_pLoadReg->bsLoadTrigger)
    {
        b_IsTrig = TRUE;
    }
    
    return b_IsTrig;
}

LOCAL void TEST_NfcSaveChecklist(void)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_PRG_REQ_DES tWrReq = {0};
    U32 ulBlkStart, ulBlkEnd;
    ulBlkStart = BLK_PER_LUN - 1 - CHECK_LIST_BLK_NUM;
    ulBlkEnd = BLK_PER_LUN - 1;
    
    tFlashAddr.bsSLCMode = TRUE; 

    tFlashAddr.usBlock = ulBlkStart;
    while (tFlashAddr.usBlock < ulBlkEnd)
    {
        //erase
        while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            ;
        }
        HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("Save Checklist: PU%d LUN %d BLK%d Erase Fail!\n", tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock);
        }
        //write
        tWrReq.bsWrBuffId   = COM_GetBufferIDByMemAddr((U32)TEST_NFC_CHK_LIST_BASE, TRUE, BUF_SIZE_BITS);
        tWrReq.pNfcRed      = NULL;
        tWrReq.pErrInj      = NULL;
        while (tFlashAddr.usPage < PG_PER_SLC_BLK)
        {               
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                ;
            }
            HAL_NfcFullPageWrite(&tFlashAddr, &tWrReq);
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                DBG_Printf("Save Checklist: PU%d LUN %d BLK%d Page%d Write Fail!\n", tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock, tFlashAddr.usPage);
            }
            tFlashAddr.usPage++;
            tWrReq.bsWrBuffId++;
        }
        tFlashAddr.usBlock++;
    }
}

LOCAL void TEST_NfcLoadChecklist(void)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdReq = {0};
    U32 ulBlkStart, ulBlkEnd;
    U32 ulBaseAddr = (U32)g_pFileBaseInDram;
    
    ulBlkStart = BLK_PER_LUN - 1 - CHECK_LIST_BLK_NUM;
    ulBlkEnd = BLK_PER_LUN - 1;
    
#ifdef DCACHE
    ulBaseAddr -= DRAM_HIGH_ADDR_OFFSET;
#endif
    tRdReq.bsSecStart   = 0;
    tRdReq.bsSecLen     = SEC_PER_BUF;
    tRdReq.bsTlcMode    = FALSE;
    tRdReq.pErrInj      = NULL;
    tRdReq.ppNfcRed     = NULL;
    tRdReq.bsRdBuffId   = COM_GetBufferIDByMemAddr(ulBaseAddr, TRUE, BUF_SIZE_BITS);

    tFlashAddr.bsSLCMode = TRUE; 
    tFlashAddr.usBlock = ulBlkStart;
    while (tFlashAddr.usBlock < ulBlkEnd)
    {
        while (tFlashAddr.usPage < PG_PER_SLC_BLK)
        {               
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                ;
            }
            HAL_NfcPageRead(&tFlashAddr, &tRdReq);
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                DBG_Printf("Save Checklist: PU%d LUN %d BLK%d Page%d Read Fail!\n", tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock, tFlashAddr.usPage);
            }
            tFlashAddr.usPage++; 
            tRdReq.bsRdBuffId++;
        }
        tFlashAddr.usBlock++;
    }
}

#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
//for im 3d tlc slc/tlc mode switch
BOOL TEST_NfcSwitchFlashMode(U8 ucPU)
{
    FLASH_ADDR tFlashAddr = {0};
    U8 ucAddr;
    U32 ulData;
    
    tFlashAddr.ucPU = ucPU;
    
    /* step1: set feature, User Selectable Trim Profile, 85h/03h data, 00-2-pass MLC;01-1-pass MLC;03-TLC */
    ucAddr = 0x91;
#ifdef SWITCH_MODE_DADF
    ulData = g_bTlcMode ? 0x104 : 0x100;
#else
    ulData = g_bTlcMode ? 0x004 : 0x000;
#endif
    HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("set feature fail PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
    }
    DBG_Printf("set feature OK PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
    
    /* step2: sync reset to make flash reload trim file from ROM block */
    HAL_NfcSyncResetFlash(&tFlashAddr);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("SYNC Reset PU:%d fail!\n", tFlashAddr.ucPU);
    }
    DBG_Printf("SYNC Reset PU:%d OK!\n", tFlashAddr.ucPU);

    return SUCCESS;
}
#endif

void MCU1_DRAM_TEXT TEST_NfcCopyChecklist2SafeDram(void)
{
    U8 ucLoadMethod = TEST_NfcGetChecklistLoadMethod();

    /*  init FCMDQ to trigger L3 excute EWR checklist  */
    if (LOAD_DISABLE != ucLoadMethod)
    {
        //HAL_DelayUs(100000);
        HAL_NfcBaseAddrInit();       
        HAL_NfcPrcqInit();
        HAL_NfcPrcqTableInit();
    }
    
    if(LOAD_MANU_FROM_TXT == ucLoadMethod)
    {
        DBG_Printf("\n###################################################\n#########  Please source load_checklist.gdb  ########\n###################################################\n\n");
        
        /* break and wait manual source checklist file by user */
        //asm volatile ("waiti 4\n");
        //while (1 != g_pLoadReg->bsLoadTrigger)
        while (FALSE == TEST_NfcIsChecklistLoadTrigger())
        {
            ;
        }
        DBG_Printf("Start to save checklist into flash!\n");

        TEST_NfcSaveChecklist();
        
        DBG_Printf("Save checklist into flash done!\n");
        
        /*  init checklist file base addr  */
        g_pCheckListPtr = (U32*)TEST_NFC_CHK_LIST_BASE;  
        HAL_DMAECopyOneBlock((const U32)g_pFileBaseInDram, (const U32)g_pCheckListPtr, (const U32)COM_MemSize16DWAlign(CHECK_LIST_FILE_SZ_MAX));
       
        DBG_Printf("Copy checklist to safe DRAM done!\n\n");
    }
    else if (LOAD_AUTO_FROM_FLASH == ucLoadMethod)
    {
        DBG_Printf("Start to load checklist from flash to dram !\n");
        
        TEST_NfcLoadChecklist();
        
        DBG_Printf("Load checklist from flash to dram done!\n\n");
    }

    /*  re-assign file pointer to safe dram address  */    
    g_pCheckListPtr = g_pFileBaseInDram;

}

BOOL TEST_NfcIsLastCheckListFile(CHECK_LIST_FILE_ATTR *pFileAttr)
{
    return (BOOL)(pFileAttr->bsLastFile);
}

/*-------------------------------------------------------------------------
Name:   TEST_NfcGetNextChklistFile          
Description: 
    Move file pointer to next file start address
AuthorName:      AbbyLin
-------------------------------------------------------------------------*/
void TEST_NfcGetNextChklistFile(U8 ucCurFileType)
{   
    U32 ulFileSize;
    ulFileSize = (BASIC_CHK_LIST == ucCurFileType)? (sizeof(CHECK_LIST_BASIC_FILE)) : (sizeof(CHECK_LIST_EXT_FILE));

#ifdef MIX_VECTOR
    U32 ulFileNum = (g_ulFakeChkListEndAddr - g_ulFakeChkListStartAddr)/ulFileSize;
    U32 ulFileNumOff = 1;//rand() % 4;
    ASSERT(ulFileNumOff < ulFileNum);
   
    g_pCheckListPtr = (U32*)(g_pCheckListPtr + ulFileNumOff * ulFileSize / sizeof(U32));
    //DBG_Printf("MIX_VECTOR: Exec check list file%d, addr0x%x\n", ulFileNumOff, (U32)g_pCheckListPtr);
#else
    g_pCheckListPtr = (U32*)(g_pCheckListPtr + ulFileSize/sizeof(U32));
#endif
}

FCMD_REQ_ENTRY TEST_NfcGetFCMDFromExtCheckList(CHECK_LIST_EXT_FILE *pChkListFile)
{   
    return (pChkListFile->tFCmdReqEntry);
}

/*  end of this file  */

