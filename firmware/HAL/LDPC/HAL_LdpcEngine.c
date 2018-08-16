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
Filename    : HAL_FlashDriverBasic.c
Version     : Ver 1.0
Author      : Tobey
Date        : 2014.09.05
Description :
Others      :
Modify      :
20140905    Tobey     Create file
20151105    abby      modify to meet VT3533
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_LdpcEngine.h"
#include "HAL_LdpcMatrix.h"

volatile GLOBAL U32 *g_pCodeStaAddr;

GLOBAL MCU12_DRAM_TEXT U8 g_aLdpRecc[4] = {0xff, 0xff, 0xff, 0xff};

/* LDPC parity length in DS SRAM*/
GLOBAL MCU12_DRAM_TEXT U8 g_atLdpcMatTable[2][4]=
{
    //LDPC_MAT_MODE0
    {136,72,88,104},

    //LDPC_MAT_MODE1
    {136,112,120,128}
};

/* Download H matrix: code_lenth 8Byte align, dram strat addr    */
LOCAL MCU12_DRAM_TEXT DL_MAT_PARA l_aMatPara[4];

/*------------------------------------------------------------------------------
Name: HAL_LdpcReccInit
Description:
    Init LDPC RECC threshold
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to init RECC threshold when init LDPC.
History:
    20160108    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcReccInit(void)
{
    volatile LDPC_RECC_THRE_REG *pReccReg = (volatile LDPC_RECC_THRE_REG *)LDPC_RECC_THRE_REG_BASE;

    pReccReg->bsReccThr0 = g_aLdpRecc[0];
    pReccReg->bsReccThr1 = g_aLdpRecc[1];
    pReccReg->bsReccThr2 = g_aLdpRecc[2];
    pReccReg->bsReccThr3 = g_aLdpRecc[3];

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_LdpcOtfHardInit
Description:
    Init LDPC OTG configure REG
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to init OTF decoder.
History:
    20151221    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcOtfHardInit(void)
{
    volatile HARD_DEC_CONF_REG *pHardConfReg = (volatile HARD_DEC_CONF_REG *)LDPC_HARD_CFG_REG_BASE;
    volatile OTF_CONF_REG *pOtfConfReg = (volatile OTF_CONF_REG *)LDPC_OTF_CFG_REG_BASE;

    /*  OTF Config    */
    pOtfConfReg->bsOTFMaxIntr       = OTF_MAX_INTERATION;
    pOtfConfReg->bsOTFtoHardCtl     = FALSE;
    pOtfConfReg->bsOtfTsbEn         = TRUE;
    pOtfConfReg->bsOtfTsbActIter    = OTF_TSB_ACT_INTERATION;

    /*  Hard+ Config    */
    pHardConfReg->bsHardDecMaxIntr  = HARD_MAX_INTERATION;
    pHardConfReg->bsHardDecLLR0     = 0x0C;
    pHardConfReg->bsHardDecLLR1     = 0x14;
    pHardConfReg->bsHardTsbEn       = TRUE;
    pHardConfReg->bsHardTsbActIter  = HARD_TSB_ACT_INTERATION;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_LdpcInit
Description:
    Init LDPC OTG configure REG
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to init RECC threshold when init LDPC.
    Should called by bootloader
History:
    20160108    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcInit(void)
{
    /*  OTF decoder init    */
    HAL_LdpcOtfHardInit();

    /*  RECC threshold init */
    HAL_LdpcReccInit();

    /*  soft decoder init, pending  */

    return;
}

#ifdef SIM
/*------------------------------------------------------------------------------
Name: HAL_LdpcMatParaInit
Description:
    Init LDPC OTG configure REG
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to init Matrix para.
History:
    20151221    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcMatParaInit(void)
{
    l_aMatPara[0].bsCodeLen = 417;
    l_aMatPara[0].bsCodeStaAddr = DRAM_CODE1_BASE_ADDR;

    l_aMatPara[1].bsCodeLen = 1224;
    l_aMatPara[1].bsCodeStaAddr = DRAM_CODE2_BASE_ADDR;

    l_aMatPara[2].bsCodeLen = 468;
    l_aMatPara[2].bsCodeStaAddr = DRAM_CODE3_BASE_ADDR;

    l_aMatPara[3].bsCodeLen = 775;
    l_aMatPara[3].bsCodeStaAddr = DRAM_CODE4_BASE_ADDR;

    return;
}
#else
extern char LdpcMatParaAddr[];
extern char LdpcMatParaAddrEnd[];
/*------------------------------------------------------------------------------
Name: HAL_LdpcMatParaInit
Description:
    Init LDPC OTG configure REG
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to init Matrix para.
History:
    20151221    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcMatParaInit(void)
{
    l_aMatPara[0].bsCodeLen = 417;
    l_aMatPara[0].bsCodeStaAddr = (U32)LdpcMatParaAddr;

    l_aMatPara[1].bsCodeLen = 1224;
    l_aMatPara[1].bsCodeStaAddr = (U32)LdpcMatParaAddr + (ENC1_MAT_NUM<<2);

    l_aMatPara[2].bsCodeLen = 468;
    l_aMatPara[2].bsCodeStaAddr = (U32)LdpcMatParaAddr + ((ENC1_MAT_NUM + ENC2_MAT_NUM)<<2);

    l_aMatPara[3].bsCodeLen = 775;
    l_aMatPara[3].bsCodeStaAddr = (U32)LdpcMatParaAddr + ((ENC1_MAT_NUM + ENC2_MAT_NUM + OTF_MAT_NUM)<<2);//DRAM_CODE4_BASE_ADDR;

    return;
}
#endif

#ifdef INIT_MATRIX
/*------------------------------------------------------------------------------
Name: HAL_LdpcGetHMatix
Description:
    Ldpc Download H Matix
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to Download H Matix in the initialization.
History:
    20151228    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpInitHMatix(H_MATRIX_SEL eCodeSel)
{
    volatile U32 *pMat;
    U32 ulTableIndex;
    pMat = (volatile U32*)(*g_pCodeStaAddr + DRAM_START_ADDRESS);

    switch (eCodeSel)
    {
        case ENCODE_RAM1:
        {
            for(ulTableIndex = 0; ulTableIndex < ENC1_MAT_NUM; ulTableIndex++)
            {
                *pMat++ = g_aMatTableEnc1[ulTableIndex];
            }
        }break;

        case ENCODE_RAM2 :
        {
            for(ulTableIndex = 0; ulTableIndex < ENC2_MAT_NUM; ulTableIndex++)
            {
                *pMat++ = g_aMatTableEnc2[ulTableIndex];
            }
        }break;

        case OTF_DEC_RAM :
        {

            for(ulTableIndex = 0; ulTableIndex < OTF_MAT_NUM; ulTableIndex++)
            {
                *pMat++ = g_aMatTableOtf[ulTableIndex];
            }
        }break;

        case SOFT_HARD_DEC_RAM :
        {
            for(ulTableIndex = 0; ulTableIndex < SOFT_HARD_MAT_NUM; ulTableIndex++)
            {
                *pMat++ = g_aMatTableHardSoft[ulTableIndex];
            }
        }break;

        default:
        {
            DBG_Getch();
        }
    }

    return;
}
#endif
/*------------------------------------------------------------------------------
Name: HAL_LdpcDownloadHMatix
Description:
    Ldpc Download H Matix
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to Download H Matix in the initialization.
History:
    20151228    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcDownloadHMatix(void)
 {
    U32 ulRamIndex = 1;
    volatile DL_MAT_COFG_REG *pDlMatConfReg = (volatile DL_MAT_COFG_REG *)LDPC_H_MAT_CFG_REG_BASE;
    g_pCodeStaAddr = (volatile U32*)LDPC_H_MAT_ADDR_REG_BASE;

    HAL_LdpcMatParaInit();

    /*  0: default 1/2: encoder matrix 3:OTF & Soft decoder 4:Soft decoder & Hard+ decoder  */
    for (ulRamIndex = ENCODE_RAM1; ulRamIndex < LDPC_MAT_CNT; ulRamIndex++)
    {
        /*  config H Matrix parameter   */
        pDlMatConfReg->bsHCodeSel = ulRamIndex;
        pDlMatConfReg->bsHCodeLen = l_aMatPara[ulRamIndex-1].bsCodeLen;
        *g_pCodeStaAddr = l_aMatPara[ulRamIndex-1].bsCodeStaAddr - DRAM_START_ADDRESS;

        /*  Prepare matrix data into DRAM   */
    #ifdef INIT_MATRIX
        HAL_LdpInitHMatix(ulRamIndex);
    #endif
        /*  Trigger H Matrix download   */
        pDlMatConfReg->bsDlHTrigger = 1;
    #ifdef SIM
        pDlMatConfReg->bsDlHTrigger = 0;
    #else
        /*  wait HW done and clr trigger bit    */
        while(FALSE != pDlMatConfReg->bsDlHTrigger)
        {
            ;
        }
    #endif
    }

    return;
 }

