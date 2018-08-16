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
Filename    : FT_Case.c
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : This file defines the case interfaces. Each case can be run independently. 
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "FT_Config.h"
#include "FT_Case.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverExt.h"
#include "FT_FlashTypeAdapter.h"
#include "HAL_DecStsReport.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    EXTERN DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    EXTERN FUNCTIONS
------------------------------------------------------------------------------*/
/****************************************************************************
Function      : FT_BasicEWR
Input         : 
Output        :
Description   : 1 PU 1 BLK multi-plane EWR
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_BasicEWR(void)
{
    U16 usPageNumPerBlk;
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = !g_bFTMlcMode;
    usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;
   
    /* erase the blk */
    FT_EraseOneBlk(&tFlashAddr);

    /* program whole blk */
    FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    
    /* read whole blk and check data */
    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
}

/****************************************************************************
Function      : FT_BasicEWR
Input         : 
Output        :
Description   : Multi PU Multi BLK multi-plane EWR
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_MultiPuEWR(void)
{
    U16 ulPageNumPerBlk;
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = g_bFTMlcMode;
    ulPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;

    while (tFlashAddr.ucPU < (U16)FT_PU_END)
    {
        while (tFlashAddr.usBlock < (U16)FT_BLK_END)
        {
            /* erase the blk */
            FT_EraseOneBlk(&tFlashAddr);

            /* program whole blk */
            FT_WriteOneBlk(0, ulPageNumPerBlk, &tFlashAddr);

            /* read whole blk and check data */
            FT_ReadOneBlk(0, ulPageNumPerBlk, &tFlashAddr);

            tFlashAddr.usBlock++;
        }
        tFlashAddr.ucPU++;
    }
}

#ifdef MLC_SINGLE_PAGE_MODE
/****************************************************************************
Function      : FT_MLCSinglePageProg
Input         : 
Output        :
Description   : MLC single page programming test
Reference     :
History       :
    20180608    abby    create
****************************************************************************/
void FT_MLCSinglePageProg(void)
{
    U16 usPageNumPerBlk;
    FLASH_ADDR tFlashAddr = {0};

    g_bFTMlcMode = TRUE;    //must enable MLC mode
    
    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = TRUE;
    usPageNumPerBlk = PG_PER_BLK;

    /* enable single page programming by set FA=91h, PA=2h, default PA=0101(dual page mode) */
    g_bFTSinglePgProg = TRUE;
    FT_GetFeature(tFlashAddr.ucPU, 0x91, 0x101);//check default value first
    FT_SetFeature(tFlashAddr.ucPU, 0x91, 0x2);

    /* erase the blk */
    FT_EraseOneBlk(&tFlashAddr);

#if 1 //normal case
    /* program whole blk */
    FT_WriteOneBlk(3, usPageNumPerBlk, &tFlashAddr);

    /* read whole blk and check data */
    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    
#else //check shared page data if invalid before program UP
    FT_WriteOneBlk(0, 3, &tFlashAddr);
    
    FT_ReadOneBlk(0, 3, &tFlashAddr);

    FT_WriteOneBlk(3, usPageNumPerBlk, &tFlashAddr);

    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
#endif

    /* recovery the normal dual page programming mode */
    g_bFTSinglePgProg = FALSE;
    FT_SetFeature(tFlashAddr.ucPU, 0x91, 0x101);
}
#endif

#ifdef SLC_SHIFT_ADDR_TEST
/****************************************************************************
Function      : FT_SLCShiftAddr
Input         : 
Output        :
Description   : SLC mode test, includes 2 patterns:
                1. enable SLC without shifted page address by set FA 91h, PA 00h;
                2. enable SLC with shifted page address by set FA 91h, PA 0100h, in this case,
                   PAGE address RowAddr[7-1] will shift to RowAddr[6-0].
Reference     :
History       :
    20180608    abby    create
****************************************************************************/
void FT_SLCShiftAddrTest(void)
{
    U16 usPageNumPerBlk;
    U32 ulData;
    U8 ucAddr = 0x91, ucPatt = 6;
    FLASH_ADDR tFlashAddr = {0};
    
    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    
#if 0
    /* check default feature */
    ulData = 0x101;
    FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulData);

    while (ucPatt < 7)
    {
        switch (ucPatt) 
        {
            /* flash enable SLC mode but disable shfited page address by set FA=91h, PA=0h, default PA=101(shift page but MLC mode ) */
            case 0: //FW: SLC mode with shift address by FW, which should be right
            {
                ulData = 0;

                g_bFTMlcMode = FALSE; //enable SLC mode in FW
                tFlashAddr.bsSLCShiftAddrByFW = TRUE; //enable shift addr by FW
                DBG_Printf("Normal PATT: PA=0x%x should be SLC mode and shift addr by FW\n", ulData);
            }break;

            case 1: //FW: SLC mode without shift address by FW
            {
                ulData = 0;

                g_bFTMlcMode = FALSE; //enable SLC mode
                tFlashAddr.bsSLCShiftAddrByFW = FALSE; //disable shift addr by FW
                
            }break;

            case 2: //FW: MLC mode without shift address by FW
            {
                ulData = 0;
                g_bFTMlcMode = TRUE; //disable SLC mode
            }break;
           
            /* flash enable SLC mode but enable shfited page address by set FA=91h, PA=0h, default PA=101(shift page but MLC mode ) */
            case 3: //FW: SLC mode without shift address by FW, which should be right
            {
                ulData = 0x100;

                g_bFTMlcMode = FALSE; //enable SLC mode
                tFlashAddr.bsSLCShiftAddrByFW = FALSE; //disable shift addr by FW
                DBG_Printf("Normal PATT: PA=0x%x should be SLC mode and not shift addr by FW\n", ulData);
            }break;

            case 4: //FW: SLC mode with shift address by FW
            {
                ulData = 0x100;

                g_bFTMlcMode = FALSE; //enable SLC mode
                tFlashAddr.bsSLCShiftAddrByFW = TRUE; //enable shift addr by FW
            }break;

            case 5: //FW: MLC mode 
            {
                ulData = 0x100;
                g_bFTMlcMode = TRUE; //disable SLC mode
            }break;

            /* flash disable SLC mode but enable shfited page address by set FA=91h, PA=0h, default PA=101(shift page but MLC mode ) */
            case 6: //FW: MLC mode
            {
                ulData = 0x101;
                g_bFTMlcMode = TRUE; 
                DBG_Printf("Normal PATT: PA=0x%x should be MLC mode\n", ulData);
            }break;

            case 7: //FW: SLC mode without shift address by FW
            {
                ulData = 0x101;

                g_bFTMlcMode = FALSE; //enable SLC mode
                tFlashAddr.bsSLCShiftAddrByFW = FALSE;
            }break;

            case 8: //FW: SLC mode with shift address by FW
            {
                ulData = 0x101;

                g_bFTMlcMode = FALSE; //enable SLC mode
                tFlashAddr.bsSLCShiftAddrByFW = TRUE;
            }break;

        default:
            DBG_Getch();
        }
        
        DBG_Printf("\nPA=0x%x SLC=%d shift=%d start!\n", ulData, !g_bFTMlcMode, tFlashAddr.bsSLCShiftAddrByFW);
        
        tFlashAddr.bsSLCMode = !g_bFTMlcMode;
        usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;

        FT_SetFeature(tFlashAddr.ucPU, ucAddr, ulData);    

        FT_EraseOneBlk(&tFlashAddr);
        FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);
        FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);

        ucPatt++;

    }

#else
    g_bFTMlcMode = FALSE; //enable SLC mode
    tFlashAddr.bsSLCMode = !g_bFTMlcMode;
    usPageNumPerBlk = SLC_PG_PER_BLK;

    /* enable SLC mode but disable shfited page address by set FA=91h, PA=0h, default PA=101(shift page but MLC mode ) */
    FT_GetFeature(tFlashAddr.ucPU, 0x91, 0x101);//check default value first
    FT_SetFeature(tFlashAddr.ucPU, 0x91, 0x0);    

    tFlashAddr.bsSLCShiftAddrByFW = TRUE;

    FT_EraseOneBlk(&tFlashAddr);
    FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);

    DBG_Printf("PA=0 SLC with shifted page address by FW pass!\n\n");
   
    /* enable SLC mode with shfited page address by set FA=91h, PA=100h, default PA=101(shift page but MLC mode ) */
    FT_GetFeature(tFlashAddr.ucPU, 0x91, 0x0);//check default value first
    FT_SetFeature(tFlashAddr.ucPU, 0x91, 0x100);

    tFlashAddr.bsSLCShiftAddrByFW = FALSE;

    FT_EraseOneBlk(&tFlashAddr);
    FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);

    DBG_Printf("PA=100h SLC without shifted page address by FW pass!\n");
    
    /* recovery the normal MLC dual page programming mode */
    FT_SetFeature(tFlashAddr.ucPU, 0x91, 0x101);

    g_bFTMlcMode = TRUE; //enable MLC mode
    tFlashAddr.bsSLCMode = !g_bFTMlcMode;
    tFlashAddr.bsSLCShiftAddrByFW = FALSE;
   
    FT_EraseOneBlk(&tFlashAddr);
    FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
    
    DBG_Printf("PA= 101h MLC without shifted page address by FW pass!\n");
    
#endif
}
#endif

void FT_GetDefaultFeat(void)
{
    U16 ulPageNumPerBlk;
    U8 ucAddr;
    U32 ulExpectData;
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = g_bFTMlcMode;
    ulPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;

    while(tFlashAddr.ucPU < FT_PU_END)
    {
        /* Timing mode */
        ucAddr = 0x01; 
        ulExpectData = 0x67;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* NVDDR Configuration */
        ucAddr = 0x02;
        ulExpectData = 0x07;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* IO drive strength */
        ucAddr = 0x10;
        ulExpectData = 0x02;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* Read retry */
        ucAddr = 0x89;
        ulExpectData = 0x0;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* OTP Mode */
        ucAddr = 0x90;
        ulExpectData = 0x0;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* SLC Mode */
        ucAddr = 0x91;
        ulExpectData = 0x101;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        /* Partial page read */
        ucAddr = 0xF5;
        ulExpectData = 0x0;
        FT_GetFeature(tFlashAddr.ucPU, ucAddr, ulExpectData);

        tFlashAddr.ucPU++;
    }
}

#ifdef N0_CHECK
/* 1 PLN erase one blk and read page 2013  */
LOCAL U32 FT_N0ChkOneBlk(FLASH_ADDR *pFlashAddr, U16 usStartPage, U16 usEndPage)
{
    g_bFTMlcMode = TRUE;
    g_bFTRawDataRead = TRUE;
    
    g_ulFTN0Cnt = 0;

    /* erase the blk */
    if (SUCCESS != FT_EraseOneBlk(pFlashAddr))
    {
        return FAIL;
    }

    /* TLC read and check N0 */
    //DBG_Printf("Noraml Read Start\n");
    FT_ReadOneBlk(usStartPage, usEndPage, pFlashAddr);
    if(0 != g_ulFTN0Cnt)   
    {
        DBG_Printf("PU%d BLK%d PLN%d Total N0 in Normal Read = %d\n", pFlashAddr->ucPU
        , pFlashAddr->usBlock, pFlashAddr->bsPln, g_ulFTN0Cnt);
    }

    /* TLC shift read and check N0 again: FA=0xA5, PA=rL1_3bpc; FA=0xA6, PA=rL2_3bpc */
    //DBG_Printf("Shift Read Start\n");
    U8 ucAddr;
    for(ucAddr = 0xA4; ucAddr < 0xA5; ucAddr++)
    {   
        if (0xA3 == ucAddr)
            continue;
        FT_SetFeature(pFlashAddr->ucPU, ucAddr, 0x80);           
    }
    
    g_ulFTN0Cnt = 0;
    FT_ReadOneBlk(usStartPage, usEndPage, pFlashAddr);
    if(0 != g_ulFTN0Cnt)   
    {
        DBG_Printf("PU%d BLK%d PLN%d Total N0 in Shift Read = %d\n", pFlashAddr->ucPU
        , pFlashAddr->usBlock, pFlashAddr->bsPln, g_ulFTN0Cnt);
    }

    /* program whole blk */
    FT_WriteOneBlk(0, usEndPage, pFlashAddr);
    
    //DBG_Printf("PU%d BLK%d PLN%d N0 Check Done\n", pFlashAddr->ucPU, pFlashAddr->usBlock, pFlashAddr->bsPln);
    return SUCCESS;
}

void FT_N0Read(void)
{
    U16 usPageNumPerBlk, usLoop = 0;
    U32 ulTlcN0 = 0, ulSlcN0 = 0; // whole test total cnt
    FLASH_ADDR tFlashAddr = {0};
        
    while(usLoop < 10)
    {
        /* clear counter */
        ulTlcN0 = 0;
        ulSlcN0 = 0;
        
        /* TLC mode */
        g_bFTMlcMode = TRUE;
        tFlashAddr.ucPU = FT_PU_START;
        tFlashAddr.ucLun = FT_LUN_START;
        tFlashAddr.bsPln = FT_PLN_START;
        tFlashAddr.usBlock = FT_BLK_START;
        tFlashAddr.usPage = FT_PAGE_START;
        tFlashAddr.bsSLCMode = !g_bFTMlcMode;
        usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;
        
        while(tFlashAddr.usBlock < FT_BLK_END)
        {
            tFlashAddr.bsPln = 0;
            while(tFlashAddr.bsPln < PLN_PER_LUN)
            {
                g_ulFTN0Cnt = 0;

                /* erase the blk */
                if (SUCCESS != FT_EraseOneBlk(&tFlashAddr))
                {
                    tFlashAddr.bsPln++;
                    continue;
                }
                
                /* TLC read and check N0 */
                FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
                if(0 != g_ulFTN0Cnt)                
                    DBG_Printf("PU%d BLK%d PLN%d Total N0 after TLC Erase = %d\n", tFlashAddr.ucPU, tFlashAddr.usBlock, tFlashAddr.bsPln, g_ulFTN0Cnt);
                
                tFlashAddr.bsPln++;
                ulTlcN0 += g_ulFTN0Cnt;
            }
            tFlashAddr.usBlock++;
        }
        DBG_Printf("Loop%d Total N0 after TLC Erase = %d\n", usLoop, ulTlcN0);
    #if 0        
        /* SLC mode */
        g_bFTMlcMode = FALSE;
        
        tFlashAddr.bsPln = FT_PLN_START;
        tFlashAddr.usBlock = FT_BLK_START;
        tFlashAddr.usPage = FT_PAGE_START;
        tFlashAddr.bsSLCMode = !g_bFTMlcMode;
        usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;
        
        while(tFlashAddr.usBlock < FT_BLK_END)
        {
            tFlashAddr.bsPln = 0;
            while(tFlashAddr.bsPln < PLN_PER_LUN)
            {
                g_ulFTN0Cnt = 0;
                /* erase the blk */
                if (SUCCESS != FT_EraseOneBlk(&tFlashAddr))
                {
                    tFlashAddr.bsPln++;
                    continue;
                }
                                
                /* SLC read and check N0 */
                FT_ReadOneBlk(0, usPageNumPerBlk, &tFlashAddr);
                if(0 != g_ulFTN0Cnt)
                    DBG_Printf("PU%d BLK%d PLN%d Total N0 after SLC Erase = %d\n", tFlashAddr.ucPU, tFlashAddr.usBlock, tFlashAddr.bsPln, g_ulFTN0Cnt);
                
                tFlashAddr.bsPln++; 
                ulSlcN0 += g_ulFTN0Cnt;
            }
            tFlashAddr.usBlock++;
        }
        DBG_Printf("Loop%d Total N0 after SLC Erase = %d\n", usLoop, ulSlcN0);
        usLoop++;
    #endif
    }
}

LOCAL U32 FT_HealthChkOneBlk(FLASH_ADDR *pFlashAddr)
{
    U16 usPageNumPerBlk;

    usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;

    /* erase the blk */
    if (FAIL == FT_EraseOneBlk(pFlashAddr))
    {
        return FAIL;
    }

    /* program whole blk */
    FT_WriteOneBlk(0, usPageNumPerBlk, pFlashAddr);

    /* read whole blk and check data */
    FT_ReadOneBlk(0, usPageNumPerBlk, pFlashAddr);

    return SUCCESS;
}
#endif 

#ifdef SNAP_READ_TEST
/****************************************************************************
Function      : FT_SnapRead
Input         : 
Output        :
Description   : Multi PU Multi BLK 1 PLN EW + partial read in YMTC;
                only support single plane snap read.
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_SnapRead(void)
{
    U16 usPageNumPerBlk;
    U8 ucPageType, ucSecStart, ucSecLen;
    FLASH_ADDR tFlashAddr = {0};

    /* enable snap read or partial read */
    g_bFTSnapRead = TRUE;

    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = FT_BLK_START;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = !g_bFTMlcMode;
    usPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;
    ucSecLen = 8;   //snap read: only read first 4KB

    /* erase the blk */
    FT_EraseOneBlk(&tFlashAddr);

    /* program whole blk */
    FT_WriteOneBlk(0, usPageNumPerBlk, &tFlashAddr);

    /* set feature to enter into partial read mode, FA = F5h, PA = 1, DQ1 length can be configurable? */
    FT_SetFeature(tFlashAddr.ucPU, 0xF5, 0x1);

    /* read different part of one page by partial read */
    ucSecStart = 0;
    ucSecLen = 8;
    while (ucSecStart < 8)//the range is limited with 4KB msg + 552B spare because NFC column address force to 0
    {
        DBG_Printf("Partial Read Sec Start = %d ucSecLen = %d\n", ucSecStart, ucSecLen);

        tFlashAddr.usPage = FT_PAGE_START;
        
        while (tFlashAddr.usPage < usPageNumPerBlk)
        {
            FT_ReadOnePage(0, ucSecStart, ucSecLen, &tFlashAddr); // read, check status and check data
            tFlashAddr.usPage++;
        }
        ucSecStart += 2; 
        ucSecLen -= 2;
    }

    /* recovery default value */
    g_bFTSnapRead = FALSE;
    FT_SetFeature(tFlashAddr.ucPU, 0xF5, 0x0);

}
#endif

#ifdef IDB_READ
void FT_EraseAllBlk(void)
{
    U16 ulPageNumPerBlk;
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = FT_PU_START;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = 1;
    tFlashAddr.usPage = FT_PAGE_START;
    tFlashAddr.bsSLCMode = g_bFTMlcMode;
    ulPageNumPerBlk = g_bFTMlcMode ? PG_PER_BLK : SLC_PG_PER_BLK;

    while (tFlashAddr.ucPU < (U16)FT_PU_END)
    {
        while (tFlashAddr.usBlock < (U16)(BLK_PER_PLN + RSV_BLK_PER_PLN))
        {
            /* erase the blk */
            FT_EraseOneBlk(&tFlashAddr);

            tFlashAddr.usBlock++;
        }
        tFlashAddr.ucPU++;
    }

}

/*==============================================================================
Func Name  : FT_ReadIDB
Input      : NONE
Output     : NONE
Return Val : BOOL
Discription: read the factory bad blk from the specified position of the flash-chip.
Usage      :
History    :
20171208    AbbyLin     Create
==============================================================================*/
void FT_ReadIDB(void)
{
    U8  ucPu, ucPlane;
    U16 usBlk, usBadBlkCnt = 0;

    for (ucPu= 0; ucPu < SUBSYSTEM_PU_NUM; ucPu++)
    {
        usBadBlkCnt = 0;
        for (usBlk = 0; usBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlk++)
        {
            for (ucPlane = 0; ucPlane < PLN_PER_LUN; ++ucPlane)
            {
                if (TRUE == HAL_FlashIsBlockBad(ucPu, 0, ucPlane, usBlk))
                {
                    usBadBlkCnt++;
                    DBG_Printf("PU%d PLN%d BLK%d is a bad block\n",ucPu, ucPlane, usBlk);
                }
            }
        }
        DBG_Printf("PU%d total bad block count = %d\n", ucPu, usBadBlkCnt);
    }

    return;
}
#endif

#ifdef OTP_TEST
void FT_OTPRead(void)
{
    U8 ucPageType, ucPU;
    U16 usSecLen, usPageNumPerBlk, usPrgIndex = 0;
    FLASH_ADDR tFlashAddr = {0};
    
    g_bFTMlcMode = TRUE;//MLC mode read OTP pages???
    
    /* OTP area: block=0, page=3~0x20, page num=29 */
    tFlashAddr.ucPU = 0;
    tFlashAddr.ucLun = FT_LUN_START;
    tFlashAddr.bsPln = FT_PLN_START;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = 3;
    tFlashAddr.bsSLCMode = !g_bFTMlcMode;

    /* set feature to enter into OTP mode: FA=0x90 PA=1 */
    for(ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        while (TRUE == HAL_NfcGetFull(ucPU, 0));
        FT_SetFeature(ucPU, 0x90, 1);
    }
    
#if 0
    /* erase the blk */
    if (0 != tFlashAddr.usBlock)
    {
        FT_EraseOneBlk(&tFlashAddr);
    }
   
    /* program whole blk */
    usPageNumPerBlk = 3+29;
    usPrgIndex = 3;
    while (usPrgIndex < usPageNumPerBlk)
    {
        tFlashAddr.usPage = FT_WtGetPageAddr(&usPrgIndex);
        if (tFlashAddr.usPage >= usPageNumPerBlk)//page address check
        {
            break;
        }

        FT_WriteOnePage(&tFlashAddr); //write 1 page and check status
        
        usPrgIndex++;
    }  
#endif

#if 0    
    /* OTP protect */
    tFlashAddr.usPage = 1;
    U32 ulWrBaseAddr = (g_aFTWrBufId[0] << LOGIC_PG_SZ_BITS) + (ulSecInBuf << SEC_SZ_BITS) + DRAM_START_ADDRESS;
    *(volatile U32*)ulWrBaseAddr = 0;
    FT_WriteOnePage(&tFlashAddr);
    DBG_Printf("OTP protect done\n");
#endif

    
    /*  read OTP page 3 and 4 to get bad blk info: 
        4 copies @ 0/4648/9296/13944 both in page 3 and 4;
        B3|B2|B1|B0 = complement of hi | Hi_4bit | complement of lo | lo_8bit
    */
    
    g_bFTRawDataRead = TRUE;

    while(tFlashAddr.ucPU < SUBSYSTEM_PU_NUM)
    {
        tFlashAddr.usPage = 0x3;
        usPageNumPerBlk = tFlashAddr.usPage + 2;
        while (tFlashAddr.usPage < usPageNumPerBlk)
        {
            FT_ReadOnePage(0, 0, SEC_PER_PHYPG, &tFlashAddr); // read, check status and check data
            tFlashAddr.usPage++;
        }
        tFlashAddr.ucPU++;
    }

    /* exit OTP mode */
    for(ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        while (TRUE == HAL_NfcGetFull(ucPU, 0));
        FT_SetFeature(ucPU, 0x90, 0);
    }
}
#endif


/* end of this file */
