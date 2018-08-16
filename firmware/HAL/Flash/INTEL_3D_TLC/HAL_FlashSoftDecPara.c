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
Others      : This file is for INTEL_3D_TLC ONLY
Modify      :
20160530    Maple    add Soft Dec raw read retry para
*******************************************************************************/
#include "HAL_FlashDriverBasic.h"
#include "HAL_LdpcSoftDec.h"

/* Soft Decoder LLR value table */
LOCAL MCU12_DRAM_TEXT LDPC_LLR_ENTRY l_aLdpcLlrEntry[5] =
{
    {15, 17,  0,  0,  0,  0, 0x0,   0, 16,  0,  0,  0,  0, 0x0},
    {13,  0, 18,  0,  0,  0, 0x0,   0,  8, 24,  0,  0,  0, 0x0},
    {13,  3, 29, 19,  0,  0, 0x0,   0,  4, 12, 28,  0,  0, 0x0},
    {12,  4,  0, 28, 19,  0, 0x0,   0,  2,  6, 14, 30,  0, 0x0},
    {14,  6,  2, 30, 26, 19, 0x0,   0,  1,  3,  7, 15, 31, 0x0}
};

/* Soft Decoder ShiftRead sent para */
/* ==== For Mode : SLC or Page_Type : Low without High Page ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForSLC =
{
    {// 1st Read
        0xEF, 0xA4,  0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForSLC[2] =
{
    {// 1st read
        0xEF, 0xA4, 0xF4, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA4, 0x0C, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForSLC[3] =
{
    {// 1st read
        0xEF, 0xA4, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA4,  0x0, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA4, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForSLC[4] =
{
    {// 1st read
        0xEF, 0xA4, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA4, 0xFB, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA4, 0x05, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA4, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForSLC[5] =
{
    {// 1st read
        0xEF, 0xA4, 0xEC, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA4, 0xF8, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA4,  0x0, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA4, 0x08, INVALID_2F
    },
    {// 5th read
        0xEF, 0xA4, 0x14, INVALID_2F
    }
};
/* ==== For Mode : SLC or Page_Type : Low without High Page End==== */

/* ==== For Mode : TLC, WL_Type : MLC, Page_Type: Low Page ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForTLC_MLC_Low =
{
    {// 1st Read
        0xEF, 0xA1,  0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForTLC_MLC_Low[2] =
{
    {// 1st read
        0xEF, 0xA1, 0xF4, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA1, 0x0C, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForTLC_MLC_Low[3] =
{
    {// 1st read
        0xEF, 0xA1, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA1,  0x0, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA1, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForTLC_MLC_Low[4] =
{
    {// 1st read
        0xEF, 0xA1, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA1, 0xFB, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA1, 0x05, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA1, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForTLC_MLC_Low[5] =
{
    {// 1st read
        0xEF, 0xA1, 0xEC, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA1, 0xF8, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA1,  0x0, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA1, 0x08, INVALID_2F
    },
    {// 5th read
        0xEF, 0xA1, 0x14, INVALID_2F
    }
};
/* ==== For Mode : TLC, WL_Type : MLC, Page_Type: Low Page End==== */

/* ==== For Mode : TLC, WL_Type : MLC, Page_Type: High Page(Upper Page) ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForTLC_MLC_Upp[2] =
{
    {// 1st Read - Level 1
        0xEF, 0xA0,  0x0, INVALID_2F
    },
    {// 1st Read - Level 3
        0xEF, 0xA2,  0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForTLC_MLC_Upp[2][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA0, 0xF4, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA2, 0x0C, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA0, 0x0C, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA2, 0xF4, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForTLC_MLC_Upp[3][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA0, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA2, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA0,  0x0, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA2,  0x0, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA0, 0x0F, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA2, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForTLC_MLC_Upp[4][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA0, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA2, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA0, 0xFB, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA2, 0x05, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA0, 0x05, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA2, 0xFB, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA0, 0x0F, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xA2, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForTLC_MLC_Upp[5][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA0, 0xEC, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA2, 0x14, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA0, 0xF8, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA2, 0x08, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA0,  0x0, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA2,  0x0, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA0, 0x08, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xA2, 0xF8, INVALID_2F
        }
    },
    {
        {// 5th Read - Level 1
            0xEF, 0xA0, 0x14, INVALID_2F
        },
        {// 5th Read - Level 3
            0xEF, 0xA2, 0xEC, INVALID_2F
        }
    }
};
/* ==== For Mode : TLC, WL_Type : MLC, Page_Type: High Page(Upper Page) End==== */

/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: Low Page ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForTLC_TLC_Low =
{
    {// 1st Read
        0xEF, 0xA8,  0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForTLC_TLC_Low[2] =
{
    {// 1st read
        0xEF, 0xA8, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA8, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForTLC_TLC_Low[3] =
{
    {// 1st read
        0xEF, 0xA8, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA8,  0x0, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA8, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForTLC_TLC_Low[4] =
{
    {// 1st read
        0xEF, 0xA8, 0xF1, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA8, 0xF9, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA8, 0x07, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA8, 0x0F, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForTLC_TLC_Low[5] =
{
    {// 1st read
        0xEF, 0xA8, 0xF0, INVALID_2F
    },
    {// 2nd read
        0xEF, 0xA8, 0xF9, INVALID_2F
    },
    {// 3rd read
        0xEF, 0xA8,  0x0, INVALID_2F
    },
    {// 4th read
        0xEF, 0xA8, 0x07, INVALID_2F
    },
    {// 5th read
        0xEF, 0xA8, 0x0F, INVALID_2F
    }
};
/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: Low Page End==== */

/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: High Page(Upper Page) ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForTLC_TLC_Upp[2] =
{
    {// 1st Read - Level 1
        0xEF, 0xA6, 0x0, INVALID_2F
    },
    {// 1st Read - Level 3
        0xEF, 0xAA, 0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForTLC_TLC_Upp[2][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA6, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xAA, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA6, 0x0F, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xAA, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForTLC_TLC_Upp[3][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA6, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xAA, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA6,  0x0, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xAA,  0x0, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA6, 0x0F, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xAA, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForTLC_TLC_Upp[4][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA6, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xAA, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA6, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xAA, 0x07, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA6, 0x07, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xAA, 0xF9, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA6, 0x0F, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xAA, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForTLC_TLC_Upp[5][2] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA6, 0xF0, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xAA, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA6, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xAA, 0x07, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA6,  0x0, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xAA,  0x0, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA6, 0x07, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xAA, 0xF9, INVALID_2F
        }
    },
    {
        {// 5th Read - Level 1
            0xEF, 0xA6, 0x0F, INVALID_2F
        },
        {// 5th Read - Level 3
            0xEF, 0xAA, 0xF0, INVALID_2F
        }
    }
};
/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: High Page(Upper Page) End==== */

/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: Extra Page ==== */
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw1stRetryTableForTLC_TLC_Extra[4] =
{
    {// 1st Read - Level 1
        0xEF, 0xA5, 0x0, INVALID_2F
    },
    {// 1st Read - Level 3
        0xEF, 0xA7, 0x0, INVALID_2F
    },
    {// 1st Read - Level 5
        0xEF, 0xA9, 0x0, INVALID_2F
    },
    {// 1st Read - Level 7
        0xEF, 0xAB, 0x0, INVALID_2F
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw2ndRetryTableForTLC_TLC_Extra[2][4] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA5, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA7, 0x0F, INVALID_2F
        },
        {// 1st Read - Level 5
            0xEF, 0xA9, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 7
            0xEF, 0xAB, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA5, 0x0F, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA7, 0xF1, INVALID_2F
        },
        {// 2nd Read - Level 5
            0xEF, 0xA9, 0x0F, INVALID_2F
        },
        {// 2nd Read - Level 7
            0xEF, 0xAB, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw3rdRetryTableForTLC_TLC_Extra[3][4] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA5, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA7, 0x0F, INVALID_2F
        },
        {// 1st Read - Level 5
            0xEF, 0xA9, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 7
            0xEF, 0xAB, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA5,  0x0, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA7,  0x0, INVALID_2F
        },
        {// 2nd Read - Level 5
            0xEF, 0xA9,  0x0, INVALID_2F
        },
        {// 2nd Read - Level 7
            0xEF, 0xAB,  0x0, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA5, 0x0F, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA7, 0xF1, INVALID_2F
        },
        {// 3rd Read - Level 5
            0xEF, 0xA9, 0x0F, INVALID_2F
        },
        {// 3rd Read - Level 7
            0xEF, 0xAB, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw4thRetryTableForTLC_TLC_Extra[4][4] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA5, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA7, 0x0F, INVALID_2F
        },
        {// 1st Read - Level 5
            0xEF, 0xA9, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 7
            0xEF, 0xAB, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA5, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA7, 0x07, INVALID_2F
        },
        {// 2nd Read - Level 5
            0xEF, 0xA9, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 7
            0xEF, 0xAB, 0x07, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA5, 0x07, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA7, 0xF9, INVALID_2F
        },
        {// 3rd Read - Level 5
            0xEF, 0xA9, 0x07, INVALID_2F
        },
        {// 3rd Read - Level 7
            0xEF, 0xAB, 0xF9, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA5, 0x0F, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xA7, 0xF1, INVALID_2F
        },
        {// 4th Read - Level 5
            0xEF, 0xA9, 0x0F, INVALID_2F
        },
        {// 4th Read - Level 7
            0xEF, 0xAB, 0xF1, INVALID_2F
        }
    }
};
LOCAL MCU12_DRAM_TEXT RETRY_TABLE l_LdpcRaw5thRetryTableForTLC_TLC_Extra[5][4] =
{
    {
        {// 1st Read - Level 1
            0xEF, 0xA5, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 3
            0xEF, 0xA7, 0x0F, INVALID_2F
        },
        {// 1st Read - Level 5
            0xEF, 0xA9, 0xF1, INVALID_2F
        },
        {// 1st Read - Level 7
            0xEF, 0xAB, 0x0F, INVALID_2F
        }
    },
    {
        {// 2nd Read - Level 1
            0xEF, 0xA5, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 3
            0xEF, 0xA7, 0x07, INVALID_2F
        },
        {// 2nd Read - Level 5
            0xEF, 0xA9, 0xF9, INVALID_2F
        },
        {// 2nd Read - Level 7
            0xEF, 0xAB, 0x07, INVALID_2F
        }
    },
    {
        {// 3rd Read - Level 1
            0xEF, 0xA5,  0x0, INVALID_2F
        },
        {// 3rd Read - Level 3
            0xEF, 0xA7,  0x0, INVALID_2F
        },
        {// 3rd Read - Level 5
            0xEF, 0xA9,  0x0, INVALID_2F
        },
        {// 3rd Read - Level 7
            0xEF, 0xAB,  0x0, INVALID_2F
        }
    },
    {
        {// 4th Read - Level 1
            0xEF, 0xA5, 0x07, INVALID_2F
        },
        {// 4th Read - Level 3
            0xEF, 0xA7, 0xF9, INVALID_2F
        },
        {// 4th Read - Level 5
            0xEF, 0xA9, 0x07, INVALID_2F
        },
        {// 4th Read - Level 7
            0xEF, 0xAB, 0xF9, INVALID_2F
        }
    },
    {
        {// 5th Read - Level 1
            0xEF, 0xA5, 0x0F, INVALID_2F
        },
        {// 5th Read - Level 3
            0xEF, 0xA7, 0xF1, INVALID_2F
        },
        {// 5th Read - Level 5
            0xEF, 0xA9, 0x0F, INVALID_2F
        },
        {// 5th Read - Level 7
            0xEF, 0xAB, 0xF1, INVALID_2F
        }
    }
};
/* ==== For Mode : TLC, WL_Type : TLC, Page_Type: Extra Page End==== */

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
RETRY_TABLE MCU2_DRAM_TEXT HAL_LdpcSoftDecGetShiftRdParaTab(U8 ucCurrentShiftRdTime, U8 ucTotalShiftRdTime, BOOL bTlcMode, U8 ucWLType, U8 ucPageType, U8 ucLevel)
{
    RETRY_TABLE tRetryPara = {0};

    if(TRUE == bTlcMode)
    {
        if(ucTotalShiftRdTime == 1)
        {
            if (TLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForTLC_TLC_Low;
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForTLC_TLC_Upp[ucLevel];
                }
                else if (EXTRA_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForTLC_TLC_Extra[ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (MLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForTLC_MLC_Low;
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForTLC_MLC_Upp[ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (SLC_TYPE == ucWLType)
            {
                if (LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw1stRetryTableForSLC;
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else
            {
                DBG_Printf("LDPC Soft DEC Wrong PageCnt!\n");
                DBG_Getch();
            }

        }
        else if (ucTotalShiftRdTime == 2)
        {
            if (TLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForTLC_TLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForTLC_TLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else if (EXTRA_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForTLC_TLC_Extra[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (MLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForTLC_MLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForTLC_MLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (SLC_TYPE == ucWLType)
            {
                if (LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw2ndRetryTableForSLC[ucCurrentShiftRdTime];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else
            {
                DBG_Printf("LDPC Soft DEC Wrong PageCnt!\n");
                DBG_Getch();
            }
        }
        else if (ucTotalShiftRdTime == 3)
        {
            if (TLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForTLC_TLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForTLC_TLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else if (EXTRA_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForTLC_TLC_Extra[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (MLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForTLC_MLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForTLC_MLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (SLC_TYPE == ucWLType)
            {
                if (LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw3rdRetryTableForSLC[ucCurrentShiftRdTime];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else
            {
                DBG_Printf("LDPC Soft DEC Wrong PageCnt!\n");
                DBG_Getch();
            }
        }
        else if (ucTotalShiftRdTime == 4)
        {
            if (TLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForTLC_TLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForTLC_TLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else if (EXTRA_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForTLC_TLC_Extra[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (MLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForTLC_MLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForTLC_MLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (SLC_TYPE == ucWLType)
            {
                if (LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw4thRetryTableForSLC[ucCurrentShiftRdTime];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else
            {
                DBG_Printf("LDPC Soft DEC Wrong PageCnt!\n");
                DBG_Getch();
            }
        }
        else if (ucTotalShiftRdTime == 5)
        {
            if (TLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForTLC_TLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForTLC_TLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else if (EXTRA_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForTLC_TLC_Extra[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (MLC_TYPE == ucWLType)
            {
                if (LOW_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForTLC_MLC_Low[ucCurrentShiftRdTime];
                }
                else if (HIGH_PAGE == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForTLC_MLC_Upp[ucCurrentShiftRdTime][ucLevel];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else if (SLC_TYPE == ucWLType)
            {
                if (LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    tRetryPara = l_LdpcRaw5thRetryTableForSLC[ucCurrentShiftRdTime];
                }
                else
                {
                    DBG_Printf("LDPC Soft DEC Wrong PageType!\n");
                    DBG_Getch();
                }
            }
            else
            {
                DBG_Printf("LDPC Soft DEC Wrong PageCnt!\n");
                DBG_Getch();
            }
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
            tRetryPara = l_LdpcRaw1stRetryTableForSLC;
        }
        else if (ucTotalShiftRdTime == 2)
        {
            tRetryPara = l_LdpcRaw2ndRetryTableForSLC[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 3)
        {
            tRetryPara = l_LdpcRaw3rdRetryTableForSLC[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 4)
        {
            tRetryPara = l_LdpcRaw4thRetryTableForSLC[ucCurrentShiftRdTime];
        }
        else if (ucTotalShiftRdTime == 5)
        {
            tRetryPara = l_LdpcRaw5thRetryTableForSLC[ucCurrentShiftRdTime];
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
LDPC_LLR_ENTRY MCU2_DRAM_TEXT HAL_LdpcSoftDecGetLLR(U8 ucTotalShiftRdTime)
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
U32 MCU2_DRAM_TEXT HAL_LdpcSoftDecVthCal(U32 ulSoftDecVth, U32 ulRetryVth)
{
    U32 i;
    U32 ulFinalVth = 0;
    U32 ulTransferVth0, ulTransferVth1;
    U8 ucVth0_Sign, ucVth1_Sign;

    for (i = 0; i < sizeof(U32); i++)
    {
        ulTransferVth0 = (ulSoftDecVth >> (i * BYTE_BIT_SIZE)) & MSK_2F;
        ulTransferVth1 = (ulRetryVth >> (i * BYTE_BIT_SIZE)) & MSK_2F;

        ASSERT((ulTransferVth0 < 0x33) || (ulTransferVth0 > 0xCD));
        ASSERT((ulTransferVth1 < 0x33) || (ulTransferVth1 > 0xCD));

        ucVth0_Sign = (ulTransferVth0 >> 7) & 0x1;
        ucVth1_Sign = (ulTransferVth1 >> 7) & 0x1;

        if (ucVth0_Sign == ucVth1_Sign)
        {
            if ((0 == ucVth0_Sign) && ((ulTransferVth0 + ulTransferVth1) > 0x32))
            {
                ulFinalVth |= 0x32 << (i * BYTE_BIT_SIZE);
            }
            else if ((1 == ucVth0_Sign) && (((512 - ulTransferVth0 - ulTransferVth1)  & MSK_2F) > 0x32))
            {
                ulFinalVth |= 0xCE << (i * BYTE_BIT_SIZE);
            }
            else
            {
                ulFinalVth |= ((ulTransferVth0 + ulTransferVth1) & MSK_2F) << (i * BYTE_BIT_SIZE);
            }
        }
        else
        {
            ulFinalVth |= ((ulTransferVth0 + ulTransferVth1) & MSK_2F) << (i * BYTE_BIT_SIZE);
        }
    }

    return ulFinalVth;
}

/*     end of this file    */

