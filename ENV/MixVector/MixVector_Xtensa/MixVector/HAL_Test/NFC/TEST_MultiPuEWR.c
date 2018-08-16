/****************************************************************************
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
*****************************************************************************
Filename    : TEST_MultiPuEWR.c
Version     : Ver 1.0
Author      : TobeyTan
Date        : 2013-11-28
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "TEST_NfcMain.h"

//#define DATA_CHECK

/****************************************************************************
    FUNCTION DECLARATION
****************************************************************************/
static void DataInit(FLASH_ADDR* pFlashAddr);
static U16 GetWrBufId(FLASH_ADDR* pFlashAddr);
static U16 GetRdBufId(FLASH_ADDR* pFlashAddr);
U32 GetRedOffset(FLASH_ADDR *pFlashAddr);
static U32 GetDramAddr(U16 usBufId);
static void DataCheck(FLASH_ADDR* pFlashAddr);
void RedCheck(FLASH_ADDR *pFlashAddr,NFC_RED *pRed[]);
U8 GetLun(FLASH_ADDR *pFlashAddr);
void TEST_NfcMultiPuErase(void);
void TEST_NfcMultiPuInitData(void);
void TEST_NfcMultiPuInitData(void);
void TEST_NfcMultiPuCacheWrite(void);
void TEST_NfcMultiPuNormalRead(void);
void TEST_NfcMultiPuCacheRead(void);
void TEST_NfcCacheRdAfterWr(void);
void TEST_NfcMultiPu(void);

/****************************************************************************
    LOCAL PARAMETER
****************************************************************************/
//LOCAL  NFC_RED l_pNfcRed[CE_MAX];
//LOCAL  NFC_RED *pRed[CE_MAX*LUN_NUM*TEST_PAGE_NUM];



U32 GetDramAddr(U16 usBufId)
{
    return (U32)(usBufId *(PG_SZ*PLN_PER_PU)+DRAM_START_ADDRESS);
}


U32 GetRedOffset(FLASH_ADDR *pFlashAddr)
{
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    return (U32)(pFlashAddr->ucPU*ulTestPageNum + pFlashAddr->usPage);
}

U16 GetWrBufId(FLASH_ADDR* pFlashAddr)
{
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    U16 usBufId = (U16)(START_WBUF_ID + (pFlashAddr->ucPU * ulTestPageNum)
                         + pFlashAddr->usPage);
    return usBufId;
}

U16 GetRdBufId(FLASH_ADDR* pFlashAddr)
{
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    U16 usBufId = (U16)(START_RBUF_ID + (pFlashAddr->ucPU * ulTestPageNum)
                         + pFlashAddr->usPage);
    return usBufId;
}

U8 GetLun(FLASH_ADDR *pFlashAddr)
{
    U8 ucLun;
    if (1 == LUN_NUM_BITS)
    {
        ucLun = (HAL_NfcGetCE(pFlashAddr->ucPU)/4) % 2;
    }
    else
    {
        ucLun = 0;
    }
    return ucLun;
}


/****************************************************************************
Func : Data Init
Input: Flash address
Output: void
Description:
    calculate input data base on the flash address
****************************************************************************/

void DataInit(FLASH_ADDR* pFlashAddr)
{
    #if 1
    U32 usBufId = (U32)GetWrBufId(pFlashAddr);
    U32 ulDramAddr = GetDramAddr(usBufId);
    U32 ulSec = 0;

    if (TRUE == TEST_NfcDataCheckEn())
    {
        for (ulSec=0;ulSec<(SEC_PER_PG * PLN_PER_PU);ulSec++)
        {
            *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)) 
            = CalcData(pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usPage,ulSec);
            *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS) + 4) 
            = CalcData(pFlashAddr->usPage,pFlashAddr->ucLun,pFlashAddr->ucPU,ulSec);
        }
    } 
    #else
        U32 usBufId = (U32)GetWrBufId(pFlashAddr);
        U32 ulDramAddr = GetDramAddr(usBufId);

        U32 ulSec,i;
        for (ulSec=0;ulSec<(SEC_PER_PG * PLN_PER_PU);ulSec++)
        {
            for (i=0;i<128;i++)
            {
                *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)+i*4) = 0x55aa0000+((U8)ulSec<<8)+(i+1);
                //DBG_Printf("Waddr:0x%x Wdata:0x%x\n"
                //,(U32)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)+i*4),0x55aa0000+((U8)ulSec<<8)+(i+1));
            }
        }
    #endif
}

/****************************************************************************
Func :Data Check
Input: Flash address
Output: void
Description:
    Check output data
****************************************************************************/
void DataCheck(FLASH_ADDR* pFlashAddr)
{
    #if 1
    U32 usBufId = (U32)GetRdBufId(pFlashAddr);
    U32 ulDramAddr = GetDramAddr(usBufId);
    U32 ulSec;

    if (TRUE == TEST_NfcDataCheckEn())
    {
        for (ulSec=0;ulSec<(SEC_PER_PG * PLN_PER_PU);ulSec++)
        {
            if ( (*(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)) 
                 != CalcData(pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usPage,ulSec))
              || (*(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS) + 4) 
                 != CalcData(pFlashAddr->usPage,pFlashAddr->ucLun,pFlashAddr->ucPU,ulSec)))
            {
                TRACE_OUT(NFC_MODULE_TEST_ERROR_DATA,ulDramAddr,ulSec,0xFFFF);
                TRACE_OUT(*(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS))
                         ,CalcData(pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usPage,ulSec)
                         ,*(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS) + 4)
                         ,CalcData(pFlashAddr->usPage,pFlashAddr->ucLun,pFlashAddr->ucPU,ulSec));
                DBG_Getch();
            }
            else
            {
                *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)) = 0;
                *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS) + 4) = 0;
            }
        }
    }     
    #else
        U32 usBufId = (U32)GetRdBufId(pFlashAddr);
        U32 ulDramAddr = GetDramAddr(usBufId);
        U32 ulSec,i;
        for (ulSec=0;ulSec<(SEC_PER_PG * PLN_PER_PU);ulSec++)
        {
            for (i=0;i<128;i++)
            {
                if(0x55aa0000+((U8)ulSec<<8)+(i+1) !=
                *(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)+i*4))
                {
                    DBG_Printf("Raddr:0x%x RDramAddr:0x%x ulsec:0x%x Rdata:0x%x Wdata:0x%x\n"
                    ,(U32)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)+i*4),ulDramAddr,ulSec
                    ,*(U32*)(ulDramAddr + (ulSec<<SEC_SIZE_BITS)+i*4)
                    ,0x55aa0000+((U8)ulSec<<8)+(i+1));
                    DBG_Printf("*0x1ff81100=0x%x\n",*(U32 *)0x1ff81100);
                    //DBG_Getch();
                }
            }

        }

    #endif
}

void RedInit(NFC_RED *pRed)
{
    U32 i;
    for (i=0;i<32;i++)    //REDNUM_64BYTE
    //for (i=0;i<16;i++)    //REDNUM_32BYTE
    //for (i=0;i<12;i++)    //REDNUM_48BYTE
    //for (i=0;i<4;i++)       //REDNUM_16BYTE
    {
        pRed->aContent[i] =  0x55AA1234;
    }
    ((SpareArea *)pRed)->PageType = PageType_PMTPage;


}
void RedCheck(FLASH_ADDR *pFlashAddr,NFC_RED *pRed[])
{
    if (TRUE == TEST_NfcDataCheckEn())
    {
        U32 offset = GetRedOffset(pFlashAddr);
        U32 i;
   //     DBG_Printf("pRed[%d] = 0x%x\n",offset,pRed[offset]);
        if (PageType_PMTPage != ((SpareArea *)pRed[offset])->PageType)
        {
            TRACE_OUT(NFC_MODULE_TEST_ERROR_RED,pRed[offset],pFlashAddr->ucPU,pFlashAddr->ucLun);
            DBG_Getch();
        }
        else
        {
            ((SpareArea *)pRed[GetRedOffset(pFlashAddr)])->PageType = 0x55;
        }

        for (i=0;i<32;i++)    //REDNUM_64BYTE
        //for (i=0;i<16;i++)    //REDNUM_32BYTE
        //for (i=0;i<12;i++)    //REDNUM_48BYTE
        //for (i=0;i<4;i++)       //REDNUM_16BYTE
        {
            if (*((U32*)pRed[offset] + i) != 0x55aa1234)
            {
                DBG_Printf("PU%d Block%d page%d redundant error pRed->aContent[%d]: 0x%x \n"
                    ,pFlashAddr->ucPU,pFlashAddr->usBlock,pFlashAddr->usPage
                    ,i,*((U32*)pRed[offset] + i));

            }
          /*  else
            {
                *((U32*)pRed[offset] + i) = 0;
            }*/
        }
    }
}

/*Func: TEST_NfcMultiPuErase
 *Description:
 *   Erase block
 *
 * */
void TEST_NfcMultiPuErase(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    DBG_Printf("Erase start \n");
    DBG_Printf("PU NUM : %d , Blk start : %d , Blk end : %d \n",ulTestPuNum,ulTestBlockStart,ulTestBlockEnd);
    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
        {
            tFlashAddr.ucLun = GetLun(&tFlashAddr);
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                DBG_Printf("PU : %d ,Block :%d\n",tFlashAddr.ucPU,tFlashAddr.usBlock);
                HAL_NfcFullBlockErase(&tFlashAddr);
            }
        }
        DBG_Printf("Erase end 1\n");
        for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Block %d Page %d Pu %d Erase fail!\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);
            }
        }
        DBG_Printf("Erase end 2\n");
    }
    DBG_Printf("Erase end 3\n");
}

void TEST_NfcMultiPuInitData(void)
{
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    FLASH_ADDR tFlashAddr;

    if (TRUE == TEST_NfcDataCheckEn())
    {

        for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
        {
            for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
            {
                tFlashAddr.ucLun = GetLun(&tFlashAddr);
                for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
                {
                    DataInit(&tFlashAddr);
                }
            }
        }
    }     
}

void TEST_NfcMultiPuNormalWrite(void)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_RED tRed = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    RedInit(&tRed);

   // ((SpareArea *)&tRed)->PageType = PageType_PMTPage;

    //TEST_NfcMultiPuInitData();
    DBG_Printf("Write Start \n");
    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
            {
                tFlashAddr.ucLun = GetLun(&tFlashAddr);
                DataInit(&tFlashAddr);
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                TRACE_OUT(0x8001,tFlashAddr.ucPU,tFlashAddr.ucLun,tFlashAddr.usPage);
                DBG_Printf("PU : %d, Block : %d , Page : %d \n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
                HAL_NfcFullPageWrite(&tFlashAddr,GetWrBufId(&tFlashAddr),(NFC_RED*)&tRed);
            }
            for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Block %d Page %d Pu %d Write fail!\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);
                }
            }
        }
    }
    DBG_Printf("Write End\n");
}

void TEST_NfcMultiPuCacheWrite(void)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_RED tRed = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();

   // ((SpareArea *)&tRed)->PageType = PageType_PMTPage;
    RedInit(&tRed);
    TEST_NfcMultiPuInitData();
    DBG_Printf("Cache Write Start \n");
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
        {
            for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
            {
                tFlashAddr.ucLun = GetLun(&tFlashAddr);
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                DBG_Printf("Cache write. PU:%d , Blk:%d ,PG:%d\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
                HAL_NfcFullPageWrite(&tFlashAddr,GetWrBufId(&tFlashAddr),(NFC_RED*)&tRed);
           }
        }
    }    
    DBG_Printf("Cache write end 1\n");
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Pu :%d Cache write fail!Error code:0x%x\n",tFlashAddr.ucPU,HAL_NfcWaitStatus(tFlashAddr.ucPU));
            }
        }
    }
    DBG_Printf("Cache write end 2\n");
}

void TEST_NfcMultiPuNormalRead(void)
{
    FLASH_ADDR tFlashAddr;
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();

    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
            {
                tFlashAddr.ucLun = GetLun(&tFlashAddr);
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                TRACE_OUT(0x8002,tFlashAddr.ucPU,tFlashAddr.ucLun,tFlashAddr.usPage);

                HAL_NfcPageRead(&tFlashAddr,0,(SEC_PER_PG*PLN_PER_PU)
                               ,GetRdBufId(&tFlashAddr)
                               ,(NFC_RED**)&pRed[GetRedOffset(&tFlashAddr)]);
            }
            for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU);
                    DBG_Printf("Block %d Page %d Pu %d read fail!\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);
                }
            }

            if (TRUE == TEST_NfcDataCheckEn())
            {
                for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
                {
                    tFlashAddr.ucLun = GetLun(&tFlashAddr);
                    RedCheck(&tFlashAddr,(NFC_RED **)pRed);
                    DataCheck(&tFlashAddr);
                }
            }    
        }
    }    
}

void TEST_NfcMultiPuCacheRead(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();


    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
        {
            for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
            {
                tFlashAddr.ucLun = GetLun(&tFlashAddr);
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                DBG_Printf("Cache read. PU:%d , Blk:%d ,PG:%d\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
                HAL_NfcPageRead(&tFlashAddr,0,(SEC_PER_PG*PLN_PER_PU)
                               ,GetRdBufId(&tFlashAddr)
                               ,(NFC_RED**)&pRed[GetRedOffset(&tFlashAddr)]);

                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU);
                    DBG_Printf("Block %d Page %d Pu %d read fail!\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);
                }

                if (TRUE == TEST_NfcDataCheckEn())
                {
                    tFlashAddr.ucLun = GetLun(&tFlashAddr);
                    RedCheck(&tFlashAddr,(NFC_RED **)pRed);
                    DataCheck(&tFlashAddr);
                    //DBG_Printf("Pu :%d,data check pass!\n",tFlashAddr.ucPU);
                }
                DBG_Printf("PU %d Block %d Page %d Cache WR right!\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
            }
        }
    }
 
}

void TEST_NfcCacheRdAfterWr(void)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_RED tRed;
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();


    ((SpareArea *)&tRed)->PageType = PageType_PMTPage;
    
    TEST_NfcMultiPuInitData();
    
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        tFlashAddr.usPage = 0;
        HAL_NfcFullPageWrite(&tFlashAddr,GetWrBufId(&tFlashAddr),&tRed);
        
        tFlashAddr.usPage = 1;
        HAL_NfcFullPageWrite(&tFlashAddr,GetWrBufId(&tFlashAddr),&tRed);
        while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
        {
            ;
        }
        tFlashAddr.usPage = 0;
        HAL_NfcPageRead(&tFlashAddr,0,64,GetRdBufId(&tFlashAddr),(NFC_RED**)&pRed[GetRedOffset(&tFlashAddr)]);
        while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
        {
            ;
        }        
        tFlashAddr.usPage = 1;
        HAL_NfcPageRead(&tFlashAddr,0,64,GetRdBufId(&tFlashAddr),(NFC_RED**)&pRed[GetRedOffset(&tFlashAddr)]);
    }

    if (TRUE == TEST_NfcDataCheckEn())
    {
        for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
        {    
            tFlashAddr.ucLun = GetLun(&tFlashAddr);
            {         
                for (tFlashAddr.usPage=0;tFlashAddr.usPage<2;tFlashAddr.usPage++)
                {
                    HAL_NfcWaitStatus(tFlashAddr.ucPU);
                    RedCheck(&tFlashAddr,(NFC_RED**)pRed);
                    DataCheck(&tFlashAddr);
                }
            }
        }   
     }   
    
}

void TEST_NfcMultiPu(void)
{
    TEST_NfcMultiPuErase();

#ifdef FLASH_CACHE_OPERATION
    TEST_NfcMultiPuCacheWrite();
#else
    TEST_NfcMultiPuNormalWrite();
#endif

        //TRACE_LINE;
#ifdef FLASH_CACHE_OPERATION
    TEST_NfcMultiPuCacheRead();
#else
    /*
     * Read a page ,of which the index is between 0 ~ (TEST_PAGE_NUM - 1), by PU .
     * Issue request when PU is not full , check data and red data when all the PU
     * complete one read request.
     * */
    TEST_NfcMultiPuNormalRead();
#endif

}
