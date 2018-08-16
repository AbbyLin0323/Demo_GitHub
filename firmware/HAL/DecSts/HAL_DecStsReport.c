/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
Filename    : HAL_DecStsReport.c
Version     : Ver 1.0
Author      : abby
Date        : 2016.03.10
Description : This file implement DEC status report interface which will be used
              in flash management
Others      :
Modify      :
    20160310     abby        create
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_DecStsReport.h"
#include "HAL_Xtensa.h"
/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    VARIABLE DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR volatile DEC_STATUS_SRAM *g_pDecSramStsBase;//Store LDPC status & flash ID & flash status in DEC STATUS SRAM
GLOBAL MCU12_VAR_ATTR volatile DEC_FIFO_STATUS *g_pDecFifoStsBase;//Store flash management status in DEC FIFO
GLOBAL MCU12_VAR_ATTR volatile XOR_DEC_FIFO_CFG_REG *g_pDecFifoCfg;//DEC FIFO Config
GLOBAL MCU12_VAR_ATTR volatile U8 g_ucDecFifoRp;
GLOBAL MCU12_VAR_ATTR volatile DEC_FIFO_STATUS *g_pDecFifoStsTab;

void HAL_DecFifoReset(void);
LOCAL U8 HAL_DecFifoGetWp(void);
LOCAL BOOL HAL_DecFifoIsFull(void);
LOCAL BOOL HAL_DecFifoIsEmpty(void);

/*------------------------------------------------------------------------------
Name: HAL_DecFifoGetWp
Description: 
    reset DEC FIFO and write pointer will clr to 0.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: wp
Usage:
    Called when need rset DEC FIFO WP 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
void HAL_DecFifoReset(void)
{
    g_pDecFifoCfg->bsDecFifoRst = 1;
#ifdef SIM
    g_pDecFifoCfg->bsDecFifoRst = 0;
#endif
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_DecStsInit
Description: 
    Init DEC status reg base address Initialization.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In bootloader/rom and FW. 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_DecStsInit(void)
{
    /* Base Addr Init */
    g_pDecSramStsBase = (DEC_STATUS_SRAM *)DEC_STS_SRAM_BASE; 
    g_pDecFifoStsBase = (DEC_FIFO_STATUS *)DEC_STS_FIFO_BASE;
    g_pDecFifoCfg     = (XOR_DEC_FIFO_CFG_REG *)DEC_FIFO_CFG_REG;
    g_pDecFifoStsTab  = (DEC_FIFO_STATUS *)DRAM_DEC_FIFO_STATUS_BASE;
    
    /* Clear RP */
    g_ucDecFifoRp = 0;
    
    //Maple add
    COM_MemZero((U32*)g_pDecFifoStsBase, sizeof(DEC_FIFO_STATUS)/sizeof(U32));

    /* Reset DEC FIFO module */
    HAL_DecFifoReset();
    
    return;
}

void MCU12_DRAM_TEXT HAL_DecStsInit2(void)
{
    /* Base Addr Init */
    g_pDecSramStsBase = (DEC_STATUS_SRAM *)DEC_STS_SRAM_BASE; 
    g_pDecFifoStsBase = (DEC_FIFO_STATUS *)DEC_STS_FIFO_BASE;
    g_pDecFifoCfg     = (XOR_DEC_FIFO_CFG_REG *)DEC_FIFO_CFG_REG;
    g_pDecFifoStsTab  = (DEC_FIFO_STATUS *)DRAM_DEC_FIFO_STATUS_BASE;
    
    /* Clear RP */
    g_ucDecFifoRp = 0;
    
    //Maple add
    COM_MemZero((U32*)g_pDecFifoStsBase, sizeof(DEC_FIFO_STATUS)/sizeof(U32));

    /* Reset DEC FIFO module */
    HAL_DecFifoReset();
    
    return;
}


/*------------------------------------------------------------------------------
Name: HAL_DecSramGetDecStsEntry
Description: 
    Read DEC status entry from SRAM.
Input Param:
    FLASH_ADDR *pFlashAddr: flash addr
Output Param:
    DEC_SRAM_STATUS_ENTRY *pDecSramSts: status entry
Return Value:
    none
Usage:
    Called when need get DEC SRAM status entry 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_DecSramGetDecStsEntry(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts)
{
    U8 ucRp, ucPU, ucLun, ucWp;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucWp  = HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);
    ucRp  = (ucWp + NFCQ_DEPTH - 1) % NFCQ_DEPTH;

    *(volatile DEC_SRAM_STATUS_ENTRY *)pDecSramSts = *(volatile DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLun][ucRp][pFlashAddr->bsPln]);

    return;
}

void MCU12_DRAM_TEXT HAL_DecSramGetDecStsEntryInErr(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts)
{
    U8 ucRp, ucPU, ucLun, ucWp;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucRp  = HAL_NfcGetRP(ucPU, ucLun);//g_pNfcCmdSts->aNfcqCmdStsReg[HAL_NfcGetPhyPU(ucPU)][ucLun].bsRdPtr;
    //ucRp  = (ucWp + NFCQ_DEPTH - 1) % NFCQ_DEPTH;

    *(volatile DEC_SRAM_STATUS_ENTRY *)pDecSramSts = *(volatile DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLun][ucRp][pFlashAddr->bsPln]);

    return;
}


void MCU12_DRAM_TEXT HAL_DebugDecSramGetDecStsEntry(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts, NFC_CMD_STS_REG CurNfcCmdSts)
{
    U8 ucRp, ucPU, ucLun, ucWp;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucWp  = CurNfcCmdSts.bsWrPtr;//HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);
    ucRp  = (ucWp + NFCQ_DEPTH - 1) % NFCQ_DEPTH;

    *(volatile DEC_SRAM_STATUS_ENTRY *)pDecSramSts = *(volatile DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLun][ucRp][pFlashAddr->bsPln]);
    return;
}


/*------------------------------------------------------------------------------
Name: HAL_DecSramGetFlashId
Description: 
    get flash ID from SRAM when FW need
Input Param:
    FLASH_ADDR * pFlashAddr: flash addr pointer
Output Param:
    U32 *pID: pointer to buf which store flash id.
Return Value:
    none
Usage:
    FW call this function to get a PU's ID after read flash id operation.
History:
    20151109    abby    create 
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_DecSramGetFlashId(FLASH_ADDR * pFlashAddr, U32 *pID)
{
    U8 ucPU, ucLun, ucRp, ucWp; 
    volatile U32 *pTemp;
    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucWp  = HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);
    ucRp  = (ucWp + NFCQ_DEPTH - 1) % NFCQ_DEPTH;
    
    //ID is placed in PLN0 addr
    pTemp = (volatile U32*)(&(g_pDecSramStsBase->aFlashId[ucPU][ucLun][ucRp][0]));
    
    *pID = *pTemp;
    *(pID + 1) = *(pTemp + 1);
    
    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_DecSramGetFlashSts
Description: 
    Read DEC status entry from SRAM.
Input Param:
    FLASH_ADDR *pFlashAddr: flash addr
Output Param:
    DEC_SRAM_STATUS_ENTRY *pDecSramSts: status entry
Return Value:
    none
Usage:
    Called when need get flash status value 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_DecSramGetFlashSts(FLASH_ADDR *pFlashAddr, FLASH_STATUS_ENTRY *pFlashSts)
{
    U8 ucRp,ucWp, ucPU, ucLun;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucWp  = HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);
    ucRp  = (ucWp + NFCQ_DEPTH - 1) % NFCQ_DEPTH;
    
    *(volatile FLASH_STATUS_ENTRY *)pFlashSts = *(volatile FLASH_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLun][ucRp][0]);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_DecFifoGetWp
Description: 
    Get DEC FIFO write pointer.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: wp
Usage:
    Called when need get DEC FIFO WP 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
U8 HAL_DecFifoGetWp(void)
{   
    U8 ucWp;
    ucWp = g_pDecFifoCfg->bsDecFifoWp;
    return ucWp;
}

/*------------------------------------------------------------------------------
Name: HAL_DecFifoIsFull
Description: 
    Check DEC FIFO if it is full.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: wp
Usage:
    Called when config NFC to write DEC status to FIFO 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
BOOL HAL_DecFifoIsFull(void)
{
    U8 ucWp, ucDelta;
    
    ucWp = HAL_DecFifoGetWp();
    ucDelta = (ucWp < g_ucDecFifoRp) ? (DEC_FIFO_DEPTH + ucWp - g_ucDecFifoRp) : (ucWp - g_ucDecFifoRp);

    if ((DEC_FIFO_DEPTH - 1) == ucDelta)
    {
        return TRUE;
    }
    
    return FALSE;
}

/*------------------------------------------------------------------------------
Name: HAL_DecFifoTrigNfc
Description: 
    Config NFCQ with DEC FIFO enable and index.
Input Param:
    NFCQ_ENTRY *pNFCQEntry 
    U8 ucDecFifoIndex: total 256
Output Param:
    none
Return Value:
    DEC_FIFO_STATUS *:pointer to DEC status entry
Usage:
    Called when need get DEC status entry addr 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
BOOL HAL_DecFifoTrigNfc(NFCQ_ENTRY *pNFCQEntry, U8 ucDecFifoIndex)
{    
    /* Check FIFO full */
    //if (TRUE == HAL_DecFifoIsFull())
    //{
        //return NFC_STATUS_FAIL;
    //}
    
    pNFCQEntry->bsDecFifoEn = TRUE;
    pNFCQEntry->bsDecFifoIdxLsb = ucDecFifoIndex & 0x1;  //bit 0
    pNFCQEntry->bsDecFifoIdx7to1b = (ucDecFifoIndex & 0xFE) >> 1;  //bit 0

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_DecFifoIsEmpty
Description: 
    Check DEC FIFO if it is empty.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: wp
Usage:
    Called when read status from DEC FIFO 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
BOOL HAL_DecFifoIsEmpty(void)
{
    U8 ucWp;
    ucWp = HAL_DecFifoGetWp();

    if (g_ucDecFifoRp == ucWp)
    {
        return TRUE;
    }
    return FALSE;
}

/*------------------------------------------------------------------------------
Name: HAL_DecFifoReadSts
Description: 
    Get DEC FIFO status entry from HW engine and rewrite it to SW table
Input Param:
    U32 ucRp: FIFO read pointer
Output Param:
    none
Return Value:
    DEC_FIFO_STATUS *:pointer to DEC status entry
Usage:
    Called when need get DEC status entry addr 
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
BOOL HAL_DecFifoReadSts(void)
{
    U8 ucDecFifoIndex;
    DEC_FIFO_STATUS_ENTRY tDecFifoEntry = { 0 };
    
    /* Read Status entry for each plane */
    tDecFifoEntry = *((volatile DEC_FIFO_STATUS_ENTRY *)g_pDecFifoStsBase + g_ucDecFifoRp);

    /* Mapping to SW memory */
    ucDecFifoIndex = tDecFifoEntry.bsCmdIndex;
    g_pDecFifoStsTab->aDecFifoSts[ucDecFifoIndex] = tDecFifoEntry;
    
    /* Pop RP of SW table after read status entry from FIFO */
    if((DEC_FIFO_DEPTH - 1) == g_ucDecFifoRp)// reset DEC FIFO after 128 entry
    {
        HAL_DecFifoReset();
        g_ucDecFifoRp = 0;
    }
    else
    {
        g_ucDecFifoRp++;
    }

    return SUCCESS;
}


/*------------------------------------------------------------------------------
Name: HAL_DecFifoGetStsEntry
Description: 
    Get DEC FIFO status entry.
Input Param:
    U8 ucDecFifoIndex: cmdIndex corresponding to NFCQ
    U8 ucPln: Flash plane num
Output Param:
    none
Return Value:
    DEC_FIFO_STATUS_ENTRY 
Usage:
    Called after read DEC FIFO status entry
History:
    20160310    abby    create
------------------------------------------------------------------------------*/
DEC_FIFO_STATUS_ENTRY* HAL_DecFifoGetStsEntry(U8 ucDecFifoIndex)
{    
    DEC_FIFO_STATUS_ENTRY* ptDecFifoEntry = { 0 };
    //tDecFifoEntry = g_pDecFifoStsTab->aDecFifoSts[ucDecFifoIndex];
    ptDecFifoEntry = (DEC_FIFO_STATUS_ENTRY *)&g_pDecFifoStsBase->aDecFifoSts[ucDecFifoIndex];

    return ptDecFifoEntry;
}

/* end of this file */

