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
Description :
*******************************************************************************/
#include "XorUt_Nfc_Interface.h"
#include "XorUt_Config.h"
#include "XorUt_Common.h"
#include "HAL_XOR.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashDriverExt.h"


U32 XorUt_GetSsuDramBaseAddr(void)
{
    volatile NF_SSU_BASEADDR_REG *pSsuBaseAddr;
    pSsuBaseAddr = (volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr;

    return (((pSsuBaseAddr->bsSsuDramBase) << 10) + DRAM_START_ADDRESS);
}

void XorUt_NfcFullPlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor)
{
    U8 ucPU, ucLun, ucPlane, ucReqType;
    NFCQ_ENTRY *pNfcqEntry;
    NORMAL_DSG_ENTRY *pDsgEntry = NULL;
    NORMAL_DSG_ENTRY *p2ndDsgEntry = NULL;
    U16 usCurDsgId;
    U16 us2ndDsgId;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlane = 0;

    /*  CONFIGURE NFCQ   */
    pNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNfcqEntry, sizeof(NFCQ_ENTRY) >> 2);
    pNfcqEntry->bsDsgEn = TRUE;

    while (FALSE == HAL_GetNormalDsg(&usCurDsgId)){;}
    pDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDsgEntry, sizeof(NORMAL_DSG_ENTRY) >> 2);

    if (bIsLastNfcInXor == TRUE)
    {
        while (FALSE == HAL_GetNormalDsg(&us2ndDsgId)){ ; }
        p2ndDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(us2ndDsgId);
        COM_MemZero((U32*)p2ndDsgEntry, sizeof(NORMAL_DSG_ENTRY) >> 2);
    }

    pNfcqEntry->bsFstDsgPtr = usCurDsgId;

#ifdef FLASH_3D_MLC
    ucReqType = FLASH_PRCQ_PRG_FULL_PLN_LOW_PG_NO_HIGH;
#else
    ucReqType = NF_PRCQ_PRG_MULTIPLN;
#endif

    /*  NFCQ DW0: mode configure   */
    pNfcqEntry->bsDCrcEn = FALSE;

    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;

    /*  NFCQ DW3: DMA message, total data length  */
    /*  Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNfcqEntry->bsDmaTotalLength = LOGIC_PIPE_PG_SZ >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    pNfcqEntry->bsXorBufId = 0;
    pNfcqEntry->bsXorEn = TRUE;
    pNfcqEntry->bsXorId = ulXoreId;

    /*  NFCQ DW4: red and scramble   */

    pNfcqEntry->bsRedOntf = FALSE;
    pNfcqEntry->bsRedEn = TRUE;
    pNfcqEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    if (bIsLastNfcInXor == TRUE)
    {
        pNfcqEntry->bsRedAddXorPar0 = HAL_XoreGetParityRedunOffset(ulXoreId) >> XOR_REDUN_ALIGN_BITS;
    }
    
    pNfcqEntry->bsTlcPlnNum = XORUT_PLN_PER_LUN_BITS;
    HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, XORUT_PLN_PER_LUN_BITS, 0, NORMAL_SCR);

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    pNfcqEntry->bsSsu0En = TRUE;
    pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
    pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal;
    pNfcqEntry->bsSsu0Data = XORUT_PLN_PER_LUN;

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNfcqEntry, pFlashAddr, FALSE);

    if (bIsLastNfcInXor == TRUE)
    {
        pDsgEntry->bsLast = FALSE;
        pDsgEntry->bsNextDsgId = us2ndDsgId;
        p2ndDsgEntry->bsLast = TRUE;
        pDsgEntry->bsXferByteLen = LOGIC_PIPE_PG_SZ - g_ulSinglePageSize;
        p2ndDsgEntry->bsXferByteLen = g_ulSinglePageSize;
        p2ndDsgEntry->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) - OTFB_START_ADDRESS) >> 1;
        p2ndDsgEntry->bsOntf = TRUE;
        p2ndDsgEntry->bsXorClr = TRUE;
        p2ndDsgEntry->bsXorId  = ulXoreId;

        HAL_SetNormalDsgSts(us2ndDsgId, NORMAL_DSG_VALID);
    }
    else
    {
        pDsgEntry->bsLast = TRUE;
        pDsgEntry->bsXferByteLen = LOGIC_PIPE_PG_SZ;
    }

    pDsgEntry->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;

    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    if (bIsLastNfcInXor == TRUE)
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, TRUE, ulXoreId);
    }
    else
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, ulXoreId);
    }

    return;
}

#ifdef FLASH_TLC
void XorUt_NfcFullPlnTlcWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulTlcProgramCycle, U32 ulCurXorPageNumInXore, U32 ulXorPageNumInTotal, U32 ulXoreId, 
    BOOL bIsLastNfcInXor, U32 ulXorParityPartNum)
{
    U8 ucPU, ucLun, ucPlane;
    U32 i = 0;
    U32 ulRequestType = 0;
    NFCQ_ENTRY *pNfcqEntry;
    NORMAL_DSG_ENTRY *aDsgEntry[2] = { NULL };
    U16 aDsgId[2] = { 0 };

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlane = 0;

    BOOL bNeedDoXor = ((ulTlcProgramCycle == 0) && (ulCurXorPageNumInXore < g_usXorProtectRatio)) ? TRUE : FALSE;

    /*  CONFIGURE NFCQ   */
    pNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNfcqEntry, sizeof(NFCQ_ENTRY) >> 2);
    pNfcqEntry->bsDsgEn = TRUE;

    while (FALSE == HAL_GetNormalDsg(&aDsgId[0])){ ; }
    aDsgEntry[0] = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(aDsgId[0]);
    COM_MemZero((U32*)aDsgEntry[0], sizeof(NORMAL_DSG_ENTRY) >> 2);

    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        for (i = 1; i < 2; ++i)
        {
            while (FALSE == HAL_GetNormalDsg(&aDsgId[i])){ ; }
            aDsgEntry[i] = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(aDsgId[i]);
            COM_MemZero((U32*)aDsgEntry[i], sizeof(NORMAL_DSG_ENTRY) >> 2);
        }
    }

    pNfcqEntry->bsFstDsgPtr = aDsgId[0];

    /*  NFCQ DW0: mode configure   */
    pNfcqEntry->bsDCrcEn = FALSE;
    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;

    /*  NFCQ DW3: DMA message, total data length  */
    /*  Note: for write cmd, the unit of DmaTotalLength is 1KB */
    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        pNfcqEntry->bsDmaTotalLength = LOGIC_PIPE_PG_SZ >> CW_INFO_SZ_BITS;
        ulRequestType = NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN + ulXorParityPartNum;
        pNfcqEntry->bsParPgPos = ulXorParityPartNum;
    }
    else
    {
        pNfcqEntry->bsDmaTotalLength = (LOGIC_PIPE_PG_SZ * INTRPG_PER_PGADDR) >> CW_INFO_SZ_BITS;
        ulRequestType = NF_PRCQ_TLC_PRG_1ST_MULTIPLN + ulTlcProgramCycle;
    }
    /*  PRCQ start DW   */

    

    pNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ulRequestType);

    if (bNeedDoXor == TRUE)
    {
        pNfcqEntry->bsXorBufId = 0;
        pNfcqEntry->bsXorEn = TRUE;
        pNfcqEntry->bsXorId = ulXoreId;
    }

    /*  NFCQ DW4: red and scramble   */

    pNfcqEntry->bsRedOntf = FALSE;
    pNfcqEntry->bsRedEn = TRUE;
    pNfcqEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        pNfcqEntry->bsRedAddXorPar0 = (HAL_XoreGetParityRedunOffset(ulXoreId) >> XOR_REDUN_ALIGN_BITS) +
            ulXorParityPartNum * (64 >> XOR_REDUN_ALIGN_BITS);
    }


    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, XORUT_PLN_PER_LUN_BITS, ulXorParityPartNum, TLC_RD_ONE_PG);
    }
    else
    {
        HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, XORUT_PLN_PER_LUN_BITS, 0, TLC_WT_THREE_PG);
    }

    
    if ((ulTlcProgramCycle == 0) && (bIsLastNfcInXor == TRUE) && (ulXorParityPartNum == 2))
    {
        pNfcqEntry->bsSsu0En = TRUE;
        pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
        pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal;
        pNfcqEntry->bsSsu0Data = XORUT_PLN_PER_LUN;
    }

    U32 ulEndPlnOfRow = (g_usXorProtectRatio < PLN_CNT_PER_ROW) ? g_usXorProtectRatio : PLN_CNT_PER_ROW;
    U32 ulEndWordLineIndex = (g_usXorProtectRatio / ulEndPlnOfRow);

    if ((pFlashAddr->usPage == ulEndWordLineIndex) && (ulTlcProgramCycle == 2))
    {
        pNfcqEntry->bsSsu0En = TRUE;
        pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
        pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal - (ulEndWordLineIndex * ulEndPlnOfRow);
        pNfcqEntry->bsSsu0Data = XORUT_PLN_PER_LUN;
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNfcqEntry, pFlashAddr, FALSE);

    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        aDsgEntry[0]->bsLast = FALSE;
        aDsgEntry[0]->bsNextDsgId = aDsgId[1];
        aDsgEntry[0]->bsXferByteLen = LOGIC_PIPE_PG_SZ - g_ulSinglePageSize;
        aDsgEntry[0]->bsOntf = FALSE;
        aDsgEntry[0]->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;

        aDsgEntry[1]->bsLast = TRUE;
        aDsgEntry[1]->bsXferByteLen = g_ulSinglePageSize;
        aDsgEntry[1]->bsOntf = TRUE;
        aDsgEntry[1]->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) +
            (ulXorParityPartNum * g_ulSinglePageSize) - OTFB_START_ADDRESS) >> 1;
        
        if (ulXorParityPartNum == 2)
        {
            aDsgEntry[1]->bsXorClr = TRUE;
            aDsgEntry[1]->bsXorId = ulXoreId;
        }

        for (i = 0; i < 2; ++i)
        {
            HAL_SetNormalDsgSts(aDsgId[i], NORMAL_DSG_VALID);
        }
    }
    else
    {
        aDsgEntry[0]->bsLast = TRUE;
        aDsgEntry[0]->bsXferByteLen = LOGIC_PIPE_PG_SZ * INTRPG_PER_PGADDR;
        aDsgEntry[0]->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;

        /*  Set DSG valid    */
        HAL_SetNormalDsgSts(aDsgId[0], NORMAL_DSG_VALID);
    }

    /*  Trigger NFC    */
    if ((bIsLastNfcInXor == TRUE) && (bNeedDoXor == TRUE))
    {
        HAL_NfcCmdTrigger(pFlashAddr, ulRequestType, TRUE, ulXoreId);
    }
    else
    {
        HAL_NfcCmdTrigger(pFlashAddr, ulRequestType, FALSE, ulXoreId);
    }

    return;
}
#endif

void XorUt_NfcSinglePlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor)
{  
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucTlcPlnNum = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPln = pFlashAddr->bsPln;

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY) >> 2);

    ucReqType = NF_PRCQ_PRG;

    /*  NFCQ DW0: mode configure   */
    pNFCQEntry->bsDCrcEn = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    /*  DSG configure   */
    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW3: DMA message, total data length  */
    /*    Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNFCQEntry->bsDmaTotalLength = LOGIC_PG_SZ >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    pNFCQEntry->bsXorBufId = 0;
    pNFCQEntry->bsXorEn = TRUE;
    pNFCQEntry->bsXorId = ulXoreId;

    /*  NFCQ DW4: red and scramble   */

    pNFCQEntry->bsRedOntf = FALSE;
    pNFCQEntry->bsRedEn = TRUE;
    pNFCQEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    if (bIsLastNfcInXor == TRUE)
    {
        pNFCQEntry->bsRedAddXorPar0 = HAL_XoreGetParityRedunOffset(ulXoreId) >> XOR_REDUN_ALIGN_BITS;
    }
    
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, 0, 0, NORMAL_SCR);

    pNFCQEntry->bsSsu0En = TRUE;
    pNFCQEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
    pNFCQEntry->bsSsu0Addr = ulXorPageNumInTotal;
    pNFCQEntry->bsSsu0Data = 1;

    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG, sizeof(NORMAL_DSG_ENTRY) >> 2);
    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = LOGIC_PG_SZ;
    pDSG->bsLast = TRUE;

    if (bIsLastNfcInXor == TRUE)
    {
        pDSG->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) - OTFB_START_ADDRESS) >> 1;
        pDSG->bsOntf = TRUE;
        pDSG->bsXorClr = TRUE;
        pDSG->bsXorId  = ulXoreId;
    }
    else
    {
        /*  DSG DW1: dram address  */
        pDSG->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;
    }


    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    if (bIsLastNfcInXor == TRUE)
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, TRUE, ulXoreId);
    }
    else
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, ulXoreId);
    }

    return;
}

#ifdef FLASH_TLC
void XorUt_NfcSinglePlnTlcWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulTlcProgramCycle, U32 ulCurXorPageNumInXore, U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucTlcPlnNum = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPln = pFlashAddr->bsPln;

    BOOL bNeedDoXor = ((ulTlcProgramCycle == 0) && (ulCurXorPageNumInXore < g_usXorProtectRatio)) ? TRUE : FALSE;

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY) >> 2);

    ucReqType = NF_PRCQ_TLC_PRG_1ST + ulTlcProgramCycle;

    /*  NFCQ DW0: mode configure   */
    pNFCQEntry->bsDCrcEn = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    /*  DSG configure   */
    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW3: DMA message, total data length  */
    /*    Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNFCQEntry->bsDmaTotalLength = (LOGIC_PG_SZ * INTRPG_PER_PGADDR) >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    if ((bNeedDoXor == TRUE) && (ulCurXorPageNumInXore != (g_usXorProtectRatio - 1)))
    {
        pNFCQEntry->bsXorBufId = 0;
        pNFCQEntry->bsXorEn = TRUE;
        pNFCQEntry->bsXorId = ulXoreId;
    }

    /*  NFCQ DW4: red and scramble   */
    pNFCQEntry->bsRedOntf = FALSE;
    pNFCQEntry->bsRedEn = TRUE;
    pNFCQEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    if ((bNeedDoXor == TRUE) && (bIsLastNfcInXor == TRUE))
    {
        pNFCQEntry->bsRedAddXorPar0 = HAL_XoreGetParityRedunOffset(ulXoreId) >> XOR_REDUN_ALIGN_BITS;
        pNFCQEntry->bsRedAddXorPar1 = pNFCQEntry->bsRedAddXorPar0 + (64 >> XOR_REDUN_ALIGN_BITS);
        pNFCQEntry->bsRedAddXorPar2 = pNFCQEntry->bsRedAddXorPar1 + (64 >> XOR_REDUN_ALIGN_BITS);
    }

    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, ucTlcPlnNum, 0, TLC_WT_THREE_PG);
    pNFCQEntry->bsTlcPlnNum = 0;

    if ((ulTlcProgramCycle == 0) && (bIsLastNfcInXor == TRUE))
    {
        pNFCQEntry->bsSsu0En = TRUE;
        pNFCQEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
        pNFCQEntry->bsSsu0Addr = ulXorPageNumInTotal;
        pNFCQEntry->bsSsu0Data = 1;
    }

    U32 ulEndPlnOfRow = (g_usXorProtectRatio < PLN_CNT_PER_ROW) ? g_usXorProtectRatio : PLN_CNT_PER_ROW;
    U32 ulEndWordLineIndex = (g_usXorProtectRatio / ulEndPlnOfRow);

    if ((pFlashAddr->usPage == ulEndWordLineIndex) && (ulTlcProgramCycle == 2))
    {
        pNFCQEntry->bsSsu0En = TRUE;
        pNFCQEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
        pNFCQEntry->bsSsu0Addr = ulXorPageNumInTotal - (ulEndWordLineIndex * ulEndPlnOfRow);
        pNFCQEntry->bsSsu0Data = 1;
    }


    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG, sizeof(NORMAL_DSG_ENTRY) >> 2);
    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = LOGIC_PG_SZ * INTRPG_PER_PGADDR;
    pDSG->bsLast = TRUE;

    if ((bNeedDoXor == TRUE) && (bIsLastNfcInXor == TRUE))
    {
        pDSG->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) - OTFB_START_ADDRESS) >> 1;
        pDSG->bsOntf = TRUE;
        pDSG->bsXorClr = TRUE;
        pDSG->bsXorId = ulXoreId;
    }
    else
    {
        /*  DSG DW1: dram address  */
        pDSG->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;
        //DBG_Printf("pDSG->bsDramAddr = 0x%x\n", pDSG->bsDramAddr);
    }


    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    if ((bNeedDoXor == TRUE) && (bIsLastNfcInXor == TRUE))
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, TRUE, ulXoreId);
    }
    else
    {
        HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, ulXoreId);
    }

    return;
}
#endif

void XorUt_NfcFullPlnRead(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, BOOL bNeedDoXor, U32 ulXoreId)
{
    ASSERT((bNeedDoXor == TRUE) || (bNeedDoXor == FALSE));

    U16 usCurDsgId;
    U8  ucPU, ucLun, ucPlane;
    NFCQ_ENTRY *pNfcqEntry;
    NORMAL_DSG_ENTRY * pDsgEntry;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlane = 0;

    /*  CONFIGURE NFCQ   */
    pNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNfcqEntry, sizeof(NFCQ_ENTRY) >> 2);

    /*  NFCQ DW0: mode configure   */
    pNfcqEntry->bsDCrcEn = FALSE;

    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;
    /*  DSG configure   */
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNfcqEntry->bsDsgEn = TRUE;
    pNfcqEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNfcqEntry->aSecAddr[0].bsSecStart = 0;
    pNfcqEntry->aSecAddr[0].bsSecLength = SEC_PER_LOGIC_PG << XORUT_PLN_PER_LUN_BITS;

    /*  NFCQ DW3: DMA message, total data length  */
    pNfcqEntry->bsDmaTotalLength = SEC_PER_LOGIC_PG << XORUT_PLN_PER_LUN_BITS;
    /*  PRCQ start DW   */
    pNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_READ_MULTIPLN);

    if (bNeedDoXor == TRUE)
    {
        pNfcqEntry->bsXorBufId = 0;
        pNfcqEntry->bsXorEn = TRUE;
        pNfcqEntry->bsXorId = ulXoreId;
    }

    /*  NFCQ DW4: red and scramble   */
    pNfcqEntry->bsRedEn = TRUE;
    pNfcqEntry->bsRedOntf = FALSE;
    pNfcqEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, XORUT_PLN_PER_LUN_BITS, 0, NORMAL_SCR);
    pNfcqEntry->bsTlcPlnNum = XORUT_PLN_PER_LUN_BITS;

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    pNfcqEntry->bsSsu0En = TRUE;
    pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
    pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal;
    pNfcqEntry->bsSsu0Data = XORUT_PLN_PER_LUN;
    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNfcqEntry, pFlashAddr, FALSE);

    /*  CONFIGURE DSG   */
    pDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDsgEntry, sizeof(NORMAL_DSG_ENTRY) >> 2);
    /*  DSG DW0: transfer length and chain config  */
    pDsgEntry->bsXferByteLen = (SEC_PER_LOGIC_PG << XORUT_PLN_PER_LUN_BITS) << SEC_SZ_BITS;
    pDsgEntry->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDsgEntry->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;

    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_MULTIPLN, FALSE, INVALID_2F);

    return;
}

void XorUt_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, BOOL bNeedDoXor, U32 ulXoreId, BOOL bIsTlcRecover, U32 ulPageType)
{
    XORUT_ASSERT((bNeedDoXor == TRUE) || (bNeedDoXor == FALSE));
    XORUT_ASSERT((bIsTlcRecover == TRUE) || (bIsTlcRecover == FALSE));

    U16 usCurDsgId;
    U8  ucPU, ucLun, ucPlane;
    NFCQ_ENTRY *pNfcqEntry;
    NORMAL_DSG_ENTRY *pDsgEntry;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlane = pFlashAddr->bsPln;

    /*  CONFIGURE NFCQ   */
    pNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNfcqEntry, sizeof(NFCQ_ENTRY) >> 2);

    /*  NFCQ DW0: mode configure   */
    pNfcqEntry->bsDCrcEn = FALSE;

    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;  //select flash page layout by DS entry in IRS
    /*  DSG configure   */
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNfcqEntry->bsDsgEn = TRUE;
    pNfcqEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNfcqEntry->aSecAddr[0].bsSecStart = 0;
    pNfcqEntry->aSecAddr[0].bsSecLength = SEC_PER_LOGIC_PG;

    /*  NFCQ DW3: DMA message, total data length  */
    pNfcqEntry->bsDmaTotalLength = SEC_PER_LOGIC_PG;

    NFC_SCR_CONF eScrType = NORMAL_SCR;
    U32 ulReqType = NF_PRCQ_READ;

    if (bIsTlcRecover == TRUE)
    {
        ulReqType = NF_PRCQ_TLC_READ_LP + ulPageType;
        eScrType = TLC_RD_ONE_PG;
    }

    /*  PRCQ start DW   */
    pNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ulReqType);

    if (bNeedDoXor == TRUE)
    {
        pNfcqEntry->bsXorBufId = 0;
        pNfcqEntry->bsXorEn = TRUE;
        pNfcqEntry->bsXorId = ulXoreId;
    }

    /*  NFCQ DW4: red and scramble   */
    pNfcqEntry->bsRedOntf = FALSE;
    pNfcqEntry->bsRedEn = TRUE;
    pNfcqEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, 0, ulPageType, eScrType);

    pNfcqEntry->bsSsu0En = TRUE;
    pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
    pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal;
    pNfcqEntry->bsSsu0Data = 1;

    /*  NFCQ DW8-15: flash address  */
    pNfcqEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDsgEntry, sizeof(NORMAL_DSG_ENTRY) >> 2);

    /*  DSG DW0: transfer length and chain config  */
    pDsgEntry->bsXferByteLen = SEC_PER_LOGIC_PG << SEC_SZ_BITS;
    pDsgEntry->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDsgEntry->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, ulReqType, FALSE, INVALID_2F);

    return;
}

void XorUt_Nfc3dMlcFullPlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor)
{
    U8 ucPU, ucLun, ucPlane;
    NFCQ_ENTRY *pNfcqEntry;
    NORMAL_DSG_ENTRY *aDsgEntry[4] = { NULL };
    U16 aDsgId[4] = { 0 };
    U32 i = 0;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlane = 0;

    /*  CONFIGURE NFCQ   */
    pNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNfcqEntry, sizeof(NFCQ_ENTRY) >> 2);
    pNfcqEntry->bsDsgEn = TRUE;

    while (FALSE == HAL_GetNormalDsg(&aDsgId[0])){ ; }
    aDsgEntry[0] = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(aDsgId[0]);
    COM_MemZero((U32*)aDsgEntry[0], sizeof(NORMAL_DSG_ENTRY) >> 2);

    if (bIsLastNfcInXor == TRUE)
    {
        for (i = 1; i < 4; ++i)
        {
            while (FALSE == HAL_GetNormalDsg(&aDsgId[i])){ ; }
            aDsgEntry[i] = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(aDsgId[i]);
            COM_MemZero((U32*)aDsgEntry[i], sizeof(NORMAL_DSG_ENTRY) >> 2);
        }
    }

    pNfcqEntry->bsFstDsgPtr = aDsgId[0];

    /*  NFCQ DW0: mode configure   */
    pNfcqEntry->bsDCrcEn = FALSE;
    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;

    /*  NFCQ DW3: DMA message, total data length  */
    /*  Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNfcqEntry->bsDmaTotalLength = (LOGIC_PIPE_PG_SZ) >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_PRG_MULTIPLN);

    pNfcqEntry->bsXorBufId = 0;
    pNfcqEntry->bsXorEn = TRUE;
    pNfcqEntry->bsXorId = ulXoreId;

    /*  NFCQ DW4: red and scramble   */

    pNfcqEntry->bsRedOntf = FALSE;
    pNfcqEntry->bsRedEn = TRUE;
    pNfcqEntry->bsRedAddr = (ulPageRedunAddr - XorUt_GetRedunBaseAddr()) >> 3;

    if (bIsLastNfcInXor == TRUE)
    {
        pNfcqEntry->bsRedAddXorPar0 = HAL_XoreGetParityRedunOffset(ulXoreId) >> XOR_REDUN_ALIGN_BITS;
        pNfcqEntry->bsRedAddXorPar1 = pNfcqEntry->bsRedAddXorPar0 + (64 >> XOR_REDUN_ALIGN_BITS);
    }
    //need modify to meet TLC and 3D MLC
    pNfcqEntry->bsTlcPlnNum = XORUT_PLN_PER_LUN_BITS;
    HAL_ConfigScramble(pNfcqEntry, pFlashAddr->usPage, SCR_FULL_PLN, 0, MLC_RW_TWO_PG);
    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    pNfcqEntry->bsSsu0En = TRUE;
    pNfcqEntry->bsSsu0Ontf = FALSE;      //bOntf = 1: update to OTFB
    pNfcqEntry->bsSsu0Addr = ulXorPageNumInTotal;
    pNfcqEntry->bsSsu0Data = XORUT_PLN_PER_LUN;

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNfcqEntry, pFlashAddr, TRUE);

    if (bIsLastNfcInXor == TRUE)
    {
        for (i = 0; i < 3; ++i)
        {
            aDsgEntry[i]->bsLast = FALSE;
            aDsgEntry[i]->bsNextDsgId = aDsgId[i + 1];

            if ((i % 2) == 0)
            {
                //aDsgEntry[i]->bsXferByteLen = LOGIC_PIPE_PG_SZ - g_ulSinglePageSize;
                aDsgEntry[i]->bsXferByteLen = LOGIC_PIPE_PG_SZ - g_ulSinglePageSize;
                aDsgEntry[i]->bsOntf = FALSE;
                aDsgEntry[i]->bsDramAddr = ((ulPageDataAddr + ((i >> 1) << LOGIC_PIPE_PG_SZ_BITS)) - DRAM_START_ADDRESS) >> 1;
            }
            else
            {
                aDsgEntry[i]->bsXferByteLen = g_ulSinglePageSize;
                aDsgEntry[i]->bsOntf = TRUE;
                aDsgEntry[i]->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) +
                    ((i >> 1) * g_ulSinglePageSize) - OTFB_START_ADDRESS) >> 1;
            }
        }

        aDsgEntry[3]->bsLast = TRUE;
        aDsgEntry[3]->bsXferByteLen = g_ulSinglePageSize;
        aDsgEntry[3]->bsOntf = TRUE;
        //DRAM中，如果是用单plane的方式，那就是挨着的，要是用多plane的方式那就是隔着的
        //OTFB中，是挨着的，直接偏移16KB即可。
        aDsgEntry[3]->bsDramAddr = (HAL_XoreGetParityPageAddr(ulXoreId) +
            g_ulSinglePageSize - OTFB_START_ADDRESS) >> 1;

        aDsgEntry[3]->bsXorClr = TRUE;
        aDsgEntry[3]->bsXorId = ulXoreId;

        for (i = 0; i < 4; ++i)
        {
            HAL_SetNormalDsgSts(aDsgId[i], NORMAL_DSG_VALID);
        }
    }
    else
    {
        aDsgEntry[0]->bsLast = TRUE;
        aDsgEntry[0]->bsXferByteLen = LOGIC_PIPE_PG_SZ;
        aDsgEntry[0]->bsDramAddr = (ulPageDataAddr - DRAM_START_ADDRESS) >> 1;

        /*  Set DSG valid    */
        HAL_SetNormalDsgSts(aDsgId[0], NORMAL_DSG_VALID);
    }

    /*  Trigger NFC    */
    if (bIsLastNfcInXor == TRUE)
    {
        HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_PRG_MULTIPLN, TRUE, ulXoreId);
    }
    else
    {
        HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_PRG_MULTIPLN, FALSE, ulXoreId);
    }

    return;
}
