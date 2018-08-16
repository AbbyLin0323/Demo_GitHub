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
Filename    : HAL_FlashSoftDecPara.c
Version     : 
Author      : Maple
Date        : 
Description : This file is used for LDPC Soft Decoder to provide ShiftRead Vt para
Others      : This file is for TSB_3D_TLC ONLY
Modify      :
20160530    Maple    add Soft Dec raw read retry para
*******************************************************************************/
#include "HAL_FlashDriverBasic.h"
#include "HAL_LdpcSoftDec.h"

/* Soft Decoder LLR value table */
LOCAL MCU12_DRAM_TEXT LDPC_LLR_ENTRY l_aLdpcLlrEntry[5] =
{
    {12, 20,  0,  0,  0,  0, 0x0,  0, 16,  0,  0,  0,  0, 0x0}, //Read Point: 0mv
    {14,  0, 20,  0,  0,  0, 0x0,  0,  8, 24,  0,  0,  0, 0x0}, //Read Point: -100, 100
    {14,  6, 26, 18,  0,  0, 0x0,  0,  4, 12, 28,  0,  0, 0x0}, //Read Point: -100, 0, +100
    {12,  5,  0, 28, 18,  0, 0x0,  0,  2,  6, 14, 30,  0, 0x0}, //Read Point: -150, -100, 100, +150
    {14,  7,  2, 30, 25, 18, 0x0,  0,  1,  3,  7, 15, 31, 0x0}  //Read Point: -200, -100, 0, +100, +200
};



/* Soft Decoder ShiftRead sent para */
// TLC Block
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFirstRetryTable =
{
    {// 1st Read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawSecondRetryTable[2] = 
{
    {// 1st read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 2nd read
       0xEF, 0x89, INVALID_4F, 0x0,
       0xEF, 0x8A, INVALID_4F, 0x0
    }
};    
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawThirdRetryTable[3] = 
{
    {// 1st read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 2nd read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 3rd read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFourthRetryTable[4] = 
{
    {// 1st read
       0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 2nd read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 3rd read
       0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 4th read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFifthRetryTable[5] = 
{
    {// 1st read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 2nd read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 3rd read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 4th read
        0xEF, 0x89, INVALID_4F, 0x0,
        0xEF, 0x8A, INVALID_4F, 0x0
    },
    {// 5th read
        0x55, 0x04, 0x08, INVALID_2F,
        0x55, 0x05, 0x08, INVALID_2F
    }
};

// SLC Block
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFirstRetryTableSlc =
{
    {// 1st read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawSecondRetryTableSlc[2] = 
{
    {// 1st read
       0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 2nd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    }
};    
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawThirdRetryTableSlc[3] = 
{
    {// 1st read
       0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 2nd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 3rd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFourthRetryTableSlc[4] = 
{
    {// 1st read
       0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 2nd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 3rd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 4th read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRawFifthRetryTableSlc[5] = 
{
    {// 1st read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 2nd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 3rd read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 4th read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    },
    {// 5th read
        0xEF, 0x8B, INVALID_4F, 0x10, 
        0, 0, 0, 0 
    }
};

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION 
------------------------------------------------------------------------------*/

/*==============================================================================
Func Name  : HAL_LdpcSoftDecGetShiftRdParaTab
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Get available descriptor ID from current WPTR
Usage      :
History    :
    1. 2016.05.30 MapleXu create function
==============================================================================*/
RETRY_TABLE MCU12_DRAM_TEXT HAL_LdpcSoftDecGetShiftRdParaTab(U8 ucCurrentShiftRdTime, U8 ucTotalShiftRdTime, BOOL bTlcMode)
{
    RETRY_TABLE tRetryPara = {0};
    
    if(TRUE == bTlcMode)
    {
        if(ucTotalShiftRdTime == 1)
        {
            tRetryPara = l_LdpcRawFirstRetryTable;
        }
        else if (ucTotalShiftRdTime == 2)
        {
            tRetryPara = l_LdpcRawSecondRetryTable[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 3)
        {
            tRetryPara = l_LdpcRawThirdRetryTable[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 4)
        {
            tRetryPara = l_LdpcRawFourthRetryTable[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 5)
        {
            tRetryPara = l_LdpcRawFifthRetryTable[ucCurrentShiftRdTime];
        }
        else
        {
            DBG_Printf("Wrong LDPC Soft DEC ShiftRd time!\n");
            DBG_Getch();
        }
    }
    else
    {
        if(ucTotalShiftRdTime == 1)
        {
            tRetryPara = l_LdpcRawFirstRetryTableSlc;
        }
        else if (ucTotalShiftRdTime == 2)
        {
            tRetryPara = l_LdpcRawSecondRetryTableSlc[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 3)
        {
            tRetryPara = l_LdpcRawThirdRetryTableSlc[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 4)
        {
            tRetryPara = l_LdpcRawFourthRetryTableSlc[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 5)
        {
            tRetryPara = l_LdpcRawFifthRetryTableSlc[ucCurrentShiftRdTime];
        }
        else
        {
            DBG_Printf("Wrong LDPC Soft DEC SLC ShiftRd time!\n");
            DBG_Getch();
        }
    }
    
    return tRetryPara;
}

/*==============================================================================
Func Name  : HAL_LdpcSoftDecGetLLR
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      :
History    :
    1. 2016.05.30 MapleXu create function
==============================================================================*/
LDPC_LLR_ENTRY MCU12_DRAM_TEXT HAL_LdpcSoftDecGetLLR(U8 ucTotalShiftRdTime)
{
    LDPC_LLR_ENTRY tLdpcLlrEntry = { 0 };

    tLdpcLlrEntry.bsAddrLLR0 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR0;
    tLdpcLlrEntry.bsAddrLLR1 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR1;
    tLdpcLlrEntry.bsAddrLLR2 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR2;
    tLdpcLlrEntry.bsAddrLLR3 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR3;
    tLdpcLlrEntry.bsAddrLLR4 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR4;
    tLdpcLlrEntry.bsAddrLLR5 = l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsAddrLLR5;
    
    tLdpcLlrEntry.bsValueLLR0= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR0;
    tLdpcLlrEntry.bsValueLLR1= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR1;
    tLdpcLlrEntry.bsValueLLR2= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR2;
    tLdpcLlrEntry.bsValueLLR3= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR3;
    tLdpcLlrEntry.bsValueLLR4= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR4;
    tLdpcLlrEntry.bsValueLLR5= l_aLdpcLlrEntry[ucTotalShiftRdTime - 1].bsValueLLR5;
    
    return tLdpcLlrEntry;
}

/*     end of this file    */
