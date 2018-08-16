/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : FlashDriver_Test.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.3
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "HAL_Xtensa.h"
#include "HAL_ParamTable.h"
#include "HAL_FlashDriverExt.h"
#include "FlashDriver_Test.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define HAL_UT_DUMMY_DATA(Pu, Page, Sec) (0xAA000000 + (Pu)*0x10000 + (Page)*0x100 + (Sec))
#define HAL_UT_DUMMY_RED(Pu, Page, Dw)   (0x55000000 + (Pu)*0x10000 + (Page)*0x100 + (Dw))


// HAL Flash Driver Test Case Controller
#define HAL_UT_DATA_PRPARE_EN
#ifdef HAL_UT_DATA_PRPARE_EN
#define HAL_UT_DATA_CHECK_EN_READ
#endif
//#define HAL_UT_DATA_CHECK_EN_WRITE
//#define HAL_UT_BURN_IN

#define UT_BLK      200
#define UT_BLK_NUM  1
/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern U32 COM_GetMemAddrByBufferID(U32 BufferID, U8 bDram, U8 BufferSizeBits);
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL L3_UT_EH_BUFF *l_aUTEHBuf;
LOCAL U16 l_usReadStartBuffID, l_usWriteStartBuffID;
/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
LOCAL void L3_UTDriverFlashTest_BS(L3_UT_FLASH_CASE *pFlashCase, U32 ulCaseNum)
{
    FLASH_ADDR FlashAddr={0};
    U32 TargetAddr, ulStatus;
    U32 pageNum,SecIndex, SecNum, DwIndex, DwNum;
    NFC_RED tWriteRedSW={0}, *ptReadRedSW;
    U32 WriteBuffID = 0;
    U32 ReadBuffID = 1;
    NFC_PRG_REQ_DES tWrDes = {0};
    NFC_READ_REQ_DES tRdDes = {0};
    
    if (pFlashCase->bsEndPU >= SUBSYSTEM_PU_NUM)
    {
        DBG_Printf("MCU#%d L3_UTDriverFlashTest_BS Skiped. CaseNum=%d\n", HAL_GetMcuId(), ulCaseNum);
        DBG_Getch();
    }

    FlashAddr.ucPU = pFlashCase->bsStartPU;    
    FlashAddr.bsPln= pFlashCase->bsPlnNum;
    while (FlashAddr.ucPU <= pFlashCase->bsEndPU)
    {
        FlashAddr.ucLun = 0;
        while (FlashAddr.ucLun < NFC_LUN_PER_PU)
        {
            DBG_Printf("MCU#%d L3_UTDriverFlashTest_BS Start, PU %d BLun %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);

            FlashAddr.usBlock = pFlashCase->bsStartBlk;
            while (FlashAddr.usBlock <= pFlashCase->bsEndBlk)
            {
                // Erase block
                if (FALSE == pFlashCase->bsSinglePln)
                {
                    // Multi Plane Erase Operation
                    ulStatus = HAL_NfcFullBlockErase(&FlashAddr, FALSE);
                }
                else
                {
                    // Single Plane Erase Operation
                    ulStatus = HAL_NfcSingleBlockErase(&FlashAddr, FALSE);
                }
                if (NFC_STATUS_SUCCESS != ulStatus)
                {
                    DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawBlockErase Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                    DBG_Getch();
                }

                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                {
                    DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawBlockErase ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                    #ifdef HAL_UT_BURN_IN
                    HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                    HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                    break;
                    #else
                    DBG_Getch();
                    #endif
                }

                DBG_Printf("MCU#%d PU %d BLun%d Blk %d Erase Pass.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock);

                // Seq Write Page from page 0 to PgNum-1
                pageNum = 0;
                while (pageNum < pFlashCase->bsPageCnt)
                {
                    if (FALSE == pFlashCase->bsSlcMode)
                    {
                        FlashAddr.usPage = pageNum++;
                    }
                    else
                    {
                        FlashAddr.usPage = HAL_FlashGetSLCPage(pageNum++);
                    }

                    if (FALSE == pFlashCase->bsSinglePln)
                    {
                    #ifdef HAL_UT_DATA_PRPARE_EN
                        // prepare and check data                    
                        TargetAddr = COM_GetMemAddrByBufferID(WriteBuffID, TRUE, BUF_SIZE_BITS);
                        SecNum = SEC_PER_BUF;
                        for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + SecIndex*SEC_SIZE), 1, HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (SecIndex = 0; SecIndex<SecNum; SecIndex++)
                        {
                            if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                            {
                                DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-BS-1.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                                DBG_Getch();
                            }
                        }
                        #endif

                        // prepare and check red
                        TargetAddr = (U32)&tWriteRedSW;
                        DwNum = RED_SW_SZ_DW;
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + DwIndex*sizeof(U32)), 1, HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                            {
                                DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-BS-1.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                                DBG_Getch();
                            }
                        }
                        #endif
                    #endif

                        // Multi Plane Write opeartion
                        tWrDes.bsWrBuffId = WriteBuffID;
                        tWrDes.bsRedOntf = TRUE;
                        tWrDes.pNfcRed = (NFC_RED *)&tWriteRedSW;
                        tWrDes.pErrInj = NULL;

                        ulStatus = HAL_NfcFullPageWrite(&FlashAddr, &tWrDes);
                    }
                    else
                    {
                    #ifdef HAL_UT_DATA_PRPARE_EN
                        // prepare and check data 
                        TargetAddr = COM_GetMemAddrByBufferID(WriteBuffID, TRUE, LOGIC_PG_SZ_BITS);
                        SecNum = SEC_PER_LOGIC_PG;
                        for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + SecIndex*SEC_SIZE), 1, HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (SecIndex = 0; SecIndex<SecNum; SecIndex++)
                        {
                            if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                            {
                                DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-BS-2.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                                DBG_Getch();
                            }
                        }
                        #endif

                        // prepare and check red
                        TargetAddr = (U32)&tWriteRedSW;
                        DwNum = RED_SZ_DW;
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + DwIndex*sizeof(U32)), 1, HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                            {
                                DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-BS-2.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                                DBG_Getch();
                            }
                        }
                        #endif
                    #endif

                        // Single Plane Write opeation
                        tWrDes.bsWrBuffId = WriteBuffID;
                        tWrDes.bsRedOntf = TRUE;
                        tWrDes.pNfcRed = (NFC_RED *)&tWriteRedSW;
                        tWrDes.pErrInj = NULL;
                        
                        ulStatus = HAL_NfcSinglePlaneWrite(&FlashAddr, &tWrDes);
                    }
                    if (NFC_STATUS_SUCCESS != ulStatus)
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageWrite Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                        DBG_Getch();
                    }

                    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageWrite ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                        #ifdef HAL_UT_BURN_IN
                        HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                        HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                        break;
                        #else
                        DBG_Getch();
                        #endif
                    }

                    //DBG_Printf("MCU#%d PU %d BLun %d Blk %d Page %d Write ok.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAdddr.ucLun, FlashAddr.usBlock, FlashAddr.usPage,*(U32*)&tWriteRedSW);
                }

                if (pageNum != pFlashCase->bsPageCnt) break;
                DBG_Printf("MCU#%d PU %d BLun %d Blk %d Write Pass.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock);

                // Seq Read Page from page 0 to PgNum-1, with data check
                pageNum = 0;
                while (pageNum < pFlashCase->bsPageCnt)
                {
                    if (FALSE == pFlashCase->bsSlcMode)
                    {
                        FlashAddr.usPage = pageNum++;
                    }
                    else
                    {
                        FlashAddr.usPage = HAL_FlashGetSLCPage(pageNum++);
                    }

                    if (FALSE == pFlashCase->bsSinglePln)
                    {
                        TargetAddr = COM_GetMemAddrByBufferID(ReadBuffID, TRUE, BUF_SIZE_BITS);
                        SecNum = SEC_PER_BUF;
                        DwNum = RED_SW_SZ_DW;

                        tRdDes.bsSecStart = 0;
                        tRdDes.bsSecLen = SEC_PER_BUF;
                        tRdDes.bsRdBuffId = ReadBuffID;
                        tRdDes.bsRedOntf = TRUE;
                        tRdDes.ppNfcRed = (NFC_RED **)&ptReadRedSW;
                        tRdDes.pErrInj = NULL;
                
                        ulStatus = HAL_NfcPageRead(&FlashAddr, &tRdDes);                    
                    }
                    else
                    {
                        TargetAddr = COM_GetMemAddrByBufferID(ReadBuffID, TRUE, LOGIC_PG_SZ_BITS);
                        SecNum = SEC_PER_LOGIC_PG;
                        DwNum = RED_SZ_DW;

                        tRdDes.bsSecStart = 0;
                        tRdDes.bsSecLen = SEC_PER_LOGIC_PG;
                        tRdDes.bsRdBuffId = ReadBuffID;
                        tRdDes.bsRedOntf = TRUE;
                        tRdDes.ppNfcRed = (NFC_RED **)&ptReadRedSW;
                        tRdDes.pErrInj = NULL;
                    
                        ulStatus = HAL_NfcSinglePlnRead(&FlashAddr, &tRdDes, FALSE);
                    }
                    if (NFC_STATUS_SUCCESS != ulStatus)
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageRead Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                        DBG_Getch();
                    }

                    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageRead ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                        #ifdef HAL_UT_BURN_IN
                        HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                        HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                        continue;
                        #else
                        DBG_Getch();
                        #endif
                    }

                    // Data Check
                    #ifdef HAL_UT_DATA_CHECK_EN_READ
                    for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                    {
                        if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                        {
                            DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-BS-3.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                            DBG_Getch();
                        }
                    }
                    TargetAddr = (U32)ptReadRedSW;
                    for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                    {
                        if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                        {
                            DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-BS-3.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                            DBG_Getch();
                        }
                    }
                    #endif

                    //DBG_Printf("MCU#%d PU %d BLun %d Blk %d Page %d Read ok\n", HAL_GetMcuId(),FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock, FlashAddr.usPage);                
                }

                DBG_Printf("MCU#%d PU %d BLun %d Blk %d Read pass.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock);

                FlashAddr.usBlock++;
            }

            FlashAddr.ucLun++;
        }

        FlashAddr.ucPU++;
    }

    DBG_Printf("MCU#%d L3_UTDriverFlashTest_BS Done! CaseNum=%d\n", HAL_GetMcuId(), ulCaseNum+1);

    return;
}

LOCAL void L3_UTDriverFlashTest_EH(L3_UT_FLASH_CASE *pFlashCase, U32 ulCaseNum)
{
    FLASH_ADDR FlashAddr={0};
    NFC_RED tWriteRedSW={0};
    U32 WriteBuffID, ReadBuffID;
    U32 TargetAddr, ulStatus;
    U32 pageNum, SecIndex, SecNum, DwIndex, DwNum;    
    NFC_PRG_REQ_DES tWrDes = {0};
    NFC_READ_REQ_DES tRdDes = {0};

    if (pFlashCase->bsEndPU >= SUBSYSTEM_PU_NUM)
    {
        DBG_Printf("MCU#%d L3_UTDriverFlashTest_EH Skiped. CaseNum=%d\n", HAL_GetMcuId(), ulCaseNum);
        DBG_Getch();
    }

    // Blk [StartBlk, EndBlk] Erase->Write->Read->DataCheck
    for (FlashAddr.usBlock=pFlashCase->bsStartBlk; FlashAddr.usBlock<=pFlashCase->bsEndBlk; FlashAddr.usBlock++)
    {
        FlashAddr.bsPln = pFlashCase->bsPlnNum;

        // Erase : PU [StartPU, EndPU]
        for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
        {   
            for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
            {
                if (FALSE == pFlashCase->bsSinglePln)
                {
                    ulStatus = HAL_NfcFullBlockErase(&FlashAddr, FALSE);
                }
                else
                {
                    ulStatus = HAL_NfcSingleBlockErase(&FlashAddr, FALSE);
                }
                if (NFC_STATUS_SUCCESS != ulStatus)
                {
                    DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawBlockErase Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                    DBG_Getch();
                }
            }
        }        
        for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
        {
            for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
            {
                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                {
                    DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawBlockErase ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                    #ifdef HAL_UT_BURN_IN
                    HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                    HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                    #else
                    DBG_Getch();
                    #endif
                }

                //DBG_Printf("MCU#%d PU %d BLun %d Blk %d Erase ok.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock);
            }
        }        
        
        // Write : PU [StartPU, EndPU]       
        for (pageNum=0; pageNum < pFlashCase->bsPageCnt; pageNum++)
        {
            if (FALSE == pFlashCase->bsSlcMode)
            {
                FlashAddr.usPage = pageNum;
            }
            else
            {
                FlashAddr.usPage = HAL_FlashGetSLCPage(pageNum);
            }
                
            for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
            {                
                for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
                {
                    if (FALSE == pFlashCase->bsSinglePln)
                    {                        
                    #ifdef HAL_UT_DATA_PRPARE_EN
                        // prepare and check data
                        WriteBuffID = l_usWriteStartBuffID + FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun;
                        TargetAddr = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum] = COM_GetMemAddrByBufferID(WriteBuffID, TRUE, BUF_SIZE_BITS);
                        SecNum = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aSecNum[pageNum] = SEC_PER_BUF;
                        for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + SecIndex*SEC_SIZE), 1, HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (SecIndex = 0; SecIndex<SecNum; SecIndex++)
                        {
                            if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                            {
                                DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-EH-1.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                                DBG_Getch();
                            }
                        }
                        #endif

                        // prepare and check red
                        TargetAddr = (U32)&tWriteRedSW;
                        DwNum = RED_SW_SZ_DW;
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + DwIndex*sizeof(U32)), 1, HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                            {
                                DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-EH-1.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                                DBG_Getch();
                            }
                        }
                        #endif
                    #endif

                        // write operation
                        tWrDes.bsWrBuffId = WriteBuffID;
                        tWrDes.bsRedOntf = TRUE;
                        tWrDes.pNfcRed = (NFC_RED *)&tWriteRedSW;
                        tWrDes.pErrInj = NULL;

                        ulStatus = HAL_NfcFullPageWrite(&FlashAddr, &tWrDes);
                    }
                    else
                    {
                    #ifdef HAL_UT_DATA_PRPARE_EN
                        // prepare and check data
                        WriteBuffID = l_usWriteStartBuffID + FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun;
                        TargetAddr = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum] = COM_GetMemAddrByBufferID(WriteBuffID, TRUE, LOGIC_PG_SZ_BITS);
                        SecNum = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aSecNum[pageNum] = SEC_PER_LOGIC_PG;
                        for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + SecIndex*SEC_SIZE), 1, HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (SecIndex = 0; SecIndex<SecNum; SecIndex++)
                        {
                            if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                            {
                                DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-EH-2.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                                DBG_Getch();
                            }
                        }
                        #endif

                        // prepare and check red
                        TargetAddr = (U32)&tWriteRedSW;
                        DwNum = RED_SZ_DW;
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            COM_MemSet((U32*)(TargetAddr + DwIndex*sizeof(U32)), 1, HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex));
                        }
                        #ifdef HAL_UT_DATA_CHECK_EN_WRITE
                        for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                        {
                            if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                            {
                                DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-EH-2.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                                DBG_Getch();
                            }
                        }
                        #endif
                    #endif
                        // single plane write opeation
                        tWrDes.bsWrBuffId = WriteBuffID;
                        tWrDes.bsRedOntf = TRUE;
                        tWrDes.pNfcRed = (NFC_RED *)&tWriteRedSW;
                        tWrDes.pErrInj = NULL;

                        ulStatus = HAL_NfcSinglePlaneWrite(&FlashAddr, &tWrDes);
                    }
                    if (NFC_STATUS_SUCCESS != ulStatus)
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageWrite Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                        DBG_Getch();
                    }
                }
            }
            for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
            {
                for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
                {
                    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                    {
                        DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawPageWrite ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                        #ifdef HAL_UT_BURN_IN
                        HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                        HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                        #else
                        DBG_Getch();
                        #endif
                    }

                    //DBG_Printf("MCU#%d PU %d BLun %d Blk %d Page %d Write ok.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock, FlashAddr.usPage);
                }
            }            
        }

        // Read : PU [StartPU, EndPU]        
        for (pageNum=0; pageNum < pFlashCase->bsPageCnt; pageNum++)   
        {
            if (FALSE == pFlashCase->bsSlcMode)
            {
                FlashAddr.usPage = pageNum;
            }
            else
            {
                FlashAddr.usPage = HAL_FlashGetSLCPage(pageNum);
            }

            for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
            {              
                for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
                {
                    if (FALSE == pFlashCase->bsSinglePln)
                    {
                        ReadBuffID = l_usReadStartBuffID + FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun;
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum] = COM_GetMemAddrByBufferID(ReadBuffID, TRUE, BUF_SIZE_BITS);
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aSecNum[pageNum] = SEC_PER_BUF;
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aDwNum[pageNum] = RED_SW_SZ_DW;

                        tRdDes.bsSecStart = 0;
                        tRdDes.bsSecLen = SEC_PER_BUF;
                        tRdDes.bsRdBuffId = ReadBuffID;
                        tRdDes.bsRedOntf = TRUE;
                        tRdDes.ppNfcRed = (NFC_RED **)&l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].ptReadRedSW[pageNum];
                        tRdDes.pErrInj = NULL;
                
                        ulStatus = HAL_NfcPageRead(&FlashAddr, &tRdDes);
                    }
                    else
                    {
                        ReadBuffID = l_usReadStartBuffID + FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun;
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum] = COM_GetMemAddrByBufferID(ReadBuffID, TRUE, LOGIC_PG_SZ_BITS);
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aSecNum[pageNum] = SEC_PER_LOGIC_PG;
                        l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aDwNum[pageNum] = RED_SZ_DW;
                    
                        tRdDes.bsSecStart = 0;
                        tRdDes.bsSecLen = SEC_PER_LOGIC_PG;
                        tRdDes.bsRdBuffId = ReadBuffID;
                        tRdDes.bsRedOntf = TRUE;
                        tRdDes.ppNfcRed = (NFC_RED **)&l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].ptReadRedSW[pageNum];
                        tRdDes.pErrInj = NULL;
                
                        ulStatus = HAL_NfcSinglePlnRead(&FlashAddr, &tRdDes, FALSE);
                    }
                    if (NFC_STATUS_SUCCESS != ulStatus)
                    {
                        DBG_Printf("MCU#%d PU%d BLun %d HalNfcRawPageRead Send Fail.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun);
                        DBG_Getch();
                    }
                }
            }
            for (FlashAddr.ucPU=pFlashCase->bsStartPU; FlashAddr.ucPU <= pFlashCase->bsEndPU; FlashAddr.ucPU++)
            {
                for (FlashAddr.ucLun = 0; FlashAddr.ucLun < LUN_NUM_PER_PU; FlashAddr.ucLun++)
                {
                    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun))
                    {
                        DBG_Printf("MCU#%d PU%d BLun %d HalNfcRawPageRead ErrCode %d.\n", HAL_GetMcuId(), FlashAddr.ucPU, FlashAddr.ucLun, HAL_NfcGetErrCode(FlashAddr.ucPU, FlashAddr.ucLun));
                        #ifdef HAL_UT_BURN_IN
                        HAL_NfcResetCmdQue(FlashAddr.ucPU, FlashAddr.ucLun);
                        HAL_NfcClearINTSts(FlashAddr.ucPU, FlashAddr.ucLun);
                        continue;
                        #else
                        DBG_Getch();
                        #endif
                    }

                    #ifdef HAL_UT_DATA_CHECK_EN_READ
                    // Check Data
                    TargetAddr = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum];
                    SecNum = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aSecNum[pageNum];
                    for (SecIndex = 0; SecIndex < SecNum; SecIndex++)
                    {
                        if (*(U32*)(TargetAddr + SecIndex*SEC_SIZE) != HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex))
                        {
                            DBG_Printf("TargetAddr 0x%x_0x%x PU %d Page %d Sec %d\n", TargetAddr, l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aTargetAddr[pageNum], FlashAddr.ucPU, pageNum, SecIndex);
                            DBG_Printf("DummyData 0x%x, TargetData 0x%x, Data-Mis-Match-EH-3.\n", HAL_UT_DUMMY_DATA(FlashAddr.ucPU, FlashAddr.usPage, SecIndex), *(U32*)(TargetAddr + SecIndex*SEC_SIZE));
                            DBG_Getch();
                        }
                    }

                    // Check Red
                    TargetAddr = (U32)l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].ptReadRedSW[pageNum];
                    DwNum = l_aUTEHBuf[FlashAddr.ucPU*LUN_NUM_PER_PU + FlashAddr.ucLun].aDwNum[pageNum];
                    for (DwIndex = 0; DwIndex < DwNum; DwIndex++)
                    {
                        if (*(U32*)(TargetAddr + DwIndex*sizeof(U32)) != HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex))
                        {
                            DBG_Printf("DummyRed 0x%x, TargetRed 0x%x, Red-Mis-Match-EH-3.\n", HAL_UT_DUMMY_RED(FlashAddr.ucPU, FlashAddr.usPage, DwIndex), *(U32*)(TargetAddr + DwIndex*sizeof(U32)));
                            DBG_Getch();
                        }
                    }
                    #endif

                    //DBG_Printf("MCU#%d PU %d BLun %d Blk %d Page %d Read ok.\n", HAL_GetMcuId(),FlashAddr.ucPU, FlashAddr.ucLun, FlashAddr.usBlock, FlashAddr.usPage);
                }
            }
        }        
    }
 
    DBG_Printf("MCU#%d L3_UTDriverFlashTest_EH Done! CaseNum=%d\n", HAL_GetMcuId(),ulCaseNum+1);
    
    return;
}

LOCAL void MCU12_DRAM_TEXT UT_FlashDriverAllocDram(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase = *pFreeDramBase;    
    COM_MemAddr16DWAlign(&ulFreeDramBase);
    
    l_aUTEHBuf = (L3_UT_EH_BUFF *)ulFreeDramBase;    
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(L3_UT_EH_BUFF)*SUBSYSTEM_PU_NUM*LUN_NUM_PER_PU);
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_usReadStartBuffID = COM_GetBufferIDByMemAddr(ulFreeDramBase, TRUE, LOGIC_PIPE_PG_SZ_BITS);
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ*SUBSYSTEM_PU_NUM*LUN_NUM_PER_PU);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_usWriteStartBuffID = COM_GetBufferIDByMemAddr(ulFreeDramBase, TRUE, LOGIC_PIPE_PG_SZ_BITS);
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ*SUBSYSTEM_PU_NUM*LUN_NUM_PER_PU);

    if (ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE >= DATA_BUFF_MCU2_SIZE)
    {
        DBG_Printf("MCU#%d Dram allocate overflow in UT_FlashDriver 0x%x 0x%x.\n", HAL_GetMcuId(), ulFreeDramBase, DRAM_DATA_BUFF_MCU1_BASE);
        DBG_Getch();
    }
    
    DBG_Printf("MCU#%d FlashDriver_Test Allocate Dram [0x%x,0x%x] Size: 0x%x\n", HAL_GetMcuId(),*pFreeDramBase, ulFreeDramBase, ulFreeDramBase-*pFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return;
}

/*==============================================================================
Func Name  : UT_FlashDriver
Input      : None
Output     : None
Return Val : 
Discription: L3 Unit Test Driver - Flash Pattern.
{StartPU, EndPU, StartBlk, EndBlk, PageCnt, SinglePln, PlnNum, SLCMode, Rsvd};
Usage      : only test the nfc.
History    : 
    1. 2014.10.31 JasonGuo create function
==============================================================================*/
LOCAL BOOL UT_FlashDriverTest(void)
{    
    U32 ulIndex;
    U32 ulCaseCnt;
    
    L3_UT_FLASH_CASE atFlashCase[] = {
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, FALSE, 0, FALSE, 0},
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, TRUE, 0, FALSE, 0 },
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, TRUE, 1, FALSE, 0 },
            #if (PLN_PER_LUN == 4)
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, TRUE, 2, FALSE, 0 },
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, TRUE, 3, FALSE, 0 },
            #endif
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK/2, FALSE, 0, TRUE, 0},
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK/2, TRUE, 0, TRUE, 0 },
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK/2, TRUE, 1, TRUE, 0 },
            #if (PLN_PER_LUN == 4)
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK/2, TRUE, 2, TRUE, 0 },
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK/2, TRUE, 3, TRUE, 0 },
            #endif
            { 0, SUBSYSTEM_PU_NUM - 1, UT_BLK, UT_BLK + UT_BLK_NUM, PG_PER_BLK, FALSE, 0, FALSE, 0 }
    };

    ulCaseCnt = sizeof(atFlashCase)/sizeof(L3_UT_FLASH_CASE);

    for (ulIndex = 0; ulIndex<ulCaseCnt; ulIndex++)
    {   
        L3_UTDriverFlashTest_BS(atFlashCase+ulIndex, ulIndex);
    }

    for (ulIndex = 0; ulIndex<ulCaseCnt; ulIndex++)
    {
        L3_UTDriverFlashTest_EH(atFlashCase + ulIndex, ulIndex);
    }

    DBG_Printf("\nMCU#%d FlashDriver_Test Done.\n\n", HAL_GetMcuId());
    return TRUE;
}

LOCAL void UT_FlashReadTest(void)
{
    U16 usPage, usPageMax, usPln, usBufID;
#ifdef FLASH_YMTC_3D_MLC
    U16 aBlk[PLN_PER_LUN] = { 0x36 };
#else
    U16 aBlk[PLN_PER_LUN] = { 0x36, 0x3f, 0x36, 0x36 };
#endif
    U32 ulStatus;

    FLASH_ADDR tFlashAddr = { 0 };
    NFC_READ_REQ_DES tRdDes = { 0 };

    usBufID = 0;
    usPage = 0;
    usPageMax = 234;
    tFlashAddr.ucPU = 4;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsSLCMode = TRUE;

    // swith the slc/tlc mode in non-squash work mode.
    U8 ucAddr = 0x91;
    U8 ucData = (TRUE == tFlashAddr.bsSLCMode) ? 0x100 : 0x104;
    ulStatus = HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
    if (NFC_STATUS_SUCCESS != ulStatus)
    {
        DBG_Printf("MCU#%d PU %d BLun %d HAL_NfcSetFeature Send Fail.\n", HAL_GetMcuId(), tFlashAddr.ucPU, tFlashAddr.ucLun);
        DBG_Getch();
    }
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("MCU#%d PU%d BLun%d HAL_NfcSetFeature.\n", HAL_GetMcuId(), tFlashAddr.ucPU, tFlashAddr.ucLun);
        DBG_Getch();
    }

    // read the target block page by page using single plane operation
    while (usPage <= usPageMax)
    {
        tFlashAddr.usPage = HAL_FlashGetSLCPage(usPage);

        for (usPln = 0; usPln < PLN_PER_LUN; usPln++)
        {
            tFlashAddr.bsPln = usPln;
            tFlashAddr.usBlock = aBlk[usPln];

            tRdDes.bsSecStart = 0;
            tRdDes.bsSecLen = SEC_PER_LOGIC_PG;
            tRdDes.bsRdBuffId = usBufID + usPln;

            ulStatus = HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("MCU#%d PU %d BLun %d HalNfcRawBlockErase Send Fail.\n", HAL_GetMcuId(), tFlashAddr.ucPU, tFlashAddr.ucLun);
                DBG_Getch();
            }

            if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                DBG_Printf("MCU#%d PU%d BLun%d Pln%d Blk%d Page%d HalNfcSingleRead ErrCode %d.\n", HAL_GetMcuId(), tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.bsPln, tFlashAddr.usBlock, tFlashAddr.usPage, HAL_NfcGetErrCode(tFlashAddr.ucPU, tFlashAddr.ucLun));

                HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
                HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
            }
            else
            {
                DBG_Printf("MCU#%d PU%d BLun%d Pln%d Blk%d Page%d HalNfcSingleRead pass.\n", HAL_GetMcuId(), tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.bsPln, tFlashAddr.usBlock, tFlashAddr.usPage);
            }
        }

        usPage++;
    }

    DBG_Printf("Read Test Done.\n");
    while (1);

    return;
}
/*==============================================================================
Func Name  : UT_FlashDriver
Input      : void  
Output     : NONE
Return Val : GLOBAL
Discription: 
Usage      : 
History    : 
    1. 2016.3.18 JasonGuo create function
==============================================================================*/
GLOBAL BOOL UT_FlashDriver(void)
{
    LOCAL BOOL l_sInitDone = FALSE;

    // only for debug off-line.
    //UT_FlashReadTest();

    if (FALSE == l_sInitDone)
    {        
        UT_FlashDriverAllocDram(&g_FreeMemBase.ulDRAMBase);
        l_sInitDone = TRUE;
    }
        
    return UT_FlashDriverTest();
}

/*====================End of this file========================================*/

