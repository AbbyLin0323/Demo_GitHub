/*------------------------------------------------------------------------------
Copyright (c) 2014 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :   HAL StripePin.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :   Victor Zhang

Description: 
    Interface of hardware stripe pins , ROM boot program may select boot/init 
    mode by these pins.
    
Modification History:
20141023    Victor Zhang   created
------------------------------------------------------------------------------*/
/*#include "BaseDef.h"
#include "Proj_Config.h"
#include "HAL_MemoryMap.h"
#include "HAL_Pin.h"*/

#include "COM_Inc.h"

BOOL l_bIsEfuseMode;    // indicate the EFUSE be programed 
BOOL l_bSetHwFrEfuse;  // indicate the data in efuse is valid for HW init 

volatile EFUSE_FILE *l_pEfuseFile;  // indicate the EFUSE data register
volatile STRAP_PIN l_tStrapPin;     // contain the GPIO info
volatile EFUSE_REG_SET * const l_pEfuseRegSet = (volatile EFUSE_REG_SET *)(REG_BASE_EFUSE);

U32 HAL_StrapGetPageSize(void)
{
    NFC_REG_00 *pReg00 = (NFC_REG_00 *)&rNFC(0x00);   
    return (U32)(1 << (12+pReg00->PG_SIZE));
}

/*
HAL_StrapEfuseCheckSum
*/

BOOL HAL_StrapEfuseCheckSum(void)
{
    U8 *pBuf = (U8*)l_pEfuseFile;
    U8 ucCheckSum,i;
    ucCheckSum = l_pEfuseFile->bsCheckSum;
    for (i = 0;i<(EFUSE_WDATA_DEPTH*sizeof(U32)-1);i++)
    {
        ucCheckSum ^= pBuf[i];
    }
    return (0 == ucCheckSum)?SUCCESS:FAIL;
}


/*
HAL_StrapCheckEfuse  
1. if under jtag debug mode , we fake the efuse on OTFB for sim 
2. if data in efuse be valid ,then init HW

*/
void HAL_StrapCheckEfuse(void)
{
    l_bIsEfuseMode = FALSE;
    l_bSetHwFrEfuse = FALSE;
#ifdef VT3514_C0    

#ifdef JTAG_DEBUG
    //  under jtag debug mode , load efuse from otfb 
    l_pEfuseFile = (volatile EFUSE_FILE *)(0xfff20100);
    l_bIsEfuseMode = TRUE;
    l_bSetHwFrEfuse = TRUE;
    HAL_StrapEfuseSetHW();
#else 
    // check if the efuse be programed 
    if (TRUE == l_pEfuseRegSet->bsProgramed)
    {
        l_bIsEfuseMode = TRUE;    

        // try to read the efuse
        do
        {
            l_pEfuseRegSet->bsDataValid = TRUE;
        }while (FALSE == l_pEfuseRegSet->bsDataValid);
        
        l_pEfuseFile = (volatile EFUSE_FILE *)(REG_BASE_EFUSE);

        if(FAIL == HAL_StrapEfuseCheckSum())
        {
            return;
        }

        // check the data be valid for HW init
        if (TRUE == l_pEfuseFile->bsIsEfuseSet)    
        {   
            l_bSetHwFrEfuse = TRUE;
            HAL_StrapEfuseSetHW();   // init HW 
        }
    }    
#endif 
#endif    
}

/*
HAL_StrapIsEfuse
indicate the EFUSE be programed 
*/
BOOL HAL_StrapIsEfuse(void)
{
    return l_bIsEfuseMode;
}
/*
HAL_StrapIsSetHwFrEfuse
indicate the data in efuse is valid for HW init 
*/
BOOL HAL_StrapIsSetHwFrEfuse(void)
{
    return l_bSetHwFrEfuse;
}

/*
HAL_StrapInit   
Fetch strap pin info 
*/
void HAL_StrapInit(void)
{
#ifdef JTAG_DEBUG
    U32 ulStrapReg1 = *(U32*)(0xfff20000);
    U32 ulStrapReg2 = *(U32*)(0xfff20000);
#else
    U32 ulStrapReg1 = rGLB(0x40);
    U32 ulStrapReg2 = rGLB(0x00);
#endif
    l_tStrapPin.bsBootSrc0        = (ulStrapReg1     )  & 0x1;
    l_tStrapPin.bsBootSrc1        = (ulStrapReg1 >> 1)  & 0x1;
    l_tStrapPin.bsBootSrc2        = (ulStrapReg1 >> 2)  & 0x1;
    l_tStrapPin.bsSataPcieSel     = (ulStrapReg1 >> 3)  & 0x1;
    l_tStrapPin.bsAsyncSyncSel    = (ulStrapReg1 >> 4)  & 0x1;
    l_tStrapPin.bsTlcMlcSel       = (ulStrapReg1 >> 5)  & 0x1;
    l_tStrapPin.bsToggleOnfiSel   = (ulStrapReg1 >> 6)  & 0x1;
    l_tStrapPin.bsNvmeAhciSel     = (ulStrapReg1 >> 7)  & 0x1;
#ifdef VT3514_C0    
    l_tStrapPin.bsEPRomSel        = (ulStrapReg2 >> 17) & 0x1;
    l_tStrapPin.bs16CeMode        = (ulStrapReg2 >> 19) & 0x1;
#else
    l_tStrapPin.bsEPRomSel        = 0;
    l_tStrapPin.bs16CeMode        = 0;
#endif

#ifdef JTAG_DEBUG
    *(U32*)(0xfff20000) = 0;
#endif
}

/************************************************
FUNC: HAL_StrapEfuseSetHW
OUT :  
IN  : void 
Description:
    Set HW according efuse info

History : 2014/10/23 Victor Zhang  Create 

*************************************************/

void HAL_StrapEfuseSetHW(void)
{
    U32 i;
    volatile NFC_REG_00 *pNfc00 = (volatile NFC_REG_00 *)&rNFC(0x00);
    volatile NFC_REG_04 *pNfc04 = (volatile NFC_REG_04 *)&rNFC(0x04);
    volatile NFC_REG_08 *pNfc08 = (volatile NFC_REG_08 *)&rNFC(0x08);
    volatile NFC_REG_80 *pNfc80 = (volatile NFC_REG_80 *)&rNFC(0x80);
    volatile NFC_REG_88 *pNfc88 = (volatile NFC_REG_88 *)&rNFC(0x88);

    // set NFC 
    pNfc00->IS_MICRON  = l_pEfuseFile->bsIsONFI;
    
    if (0 == l_pEfuseFile->bsIsASYNC)
    {
        rNFC(0x2C) = 0x3f;
        if (0 ==  l_pEfuseFile->bsIsONFI)
        {
            rNFC(0x88) &= ~(1<<23);
        }
    }
    
    pNfc00->ECC_SELECT = l_pEfuseFile->bsNFC_ECCMode;
    pNfc00->CL_ADDR_BYTE = l_pEfuseFile->bsNFC_CLAddrByte;
    pNfc00->PG_SIZE = l_pEfuseFile->bsNFC_PageSize;
    pNfc04->TWHRLD = l_pEfuseFile->bsNFC_TWHRLD;
    pNfc04->TDBS = l_pEfuseFile->bsNFC_TDBS;
    pNfc04->TCAD_3 = l_pEfuseFile->bsNFC_TCAD_3;

    pNfc04->TCS = l_pEfuseFile->bsNFC_TCS;
    pNfc04->TCLHZ = l_pEfuseFile->bsNFC_TCLHZ;
    pNfc04->RDRPH = l_pEfuseFile->bsNFC_RDRPH;
    pNfc04->TRPST = l_pEfuseFile->bsNFC_TRPST;
    pNfc04->TRPSTH = l_pEfuseFile->bsNFC_TRPSTH;
    pNfc04->TCAD = l_pEfuseFile->bsNFC_TCAD;
    pNfc04->TCCS_LDV = l_pEfuseFile->bsNFC_TCCS_LDV;

    pNfc04->TRPRE = l_pEfuseFile->bsNFC_TRPRE;
    pNfc04->SLOWRD = l_pEfuseFile->bsNFC_SlowRead;
    pNfc08->TRHWLD = l_pEfuseFile->bsNFC_TRHWLD;
    pNfc08->DDR_CFG = l_pEfuseFile->bsNFC_DDR_CFG;
    pNfc08->DDR_HF_CFG = l_pEfuseFile->bsNFC_DDR_HF_CFG;
    pNfc08->TDQSRE  = l_pEfuseFile->bsNFC_TDQSRE;
    pNfc88->DQSHZ_TH = l_pEfuseFile->bsNFC_DQSHZ_TH;
    
    if (1 == l_pEfuseFile->bsIsONFI)
    {
        if (1 == l_pEfuseFile->bsNFC_DDR_HF_CFG)
        {
            rNFC(0x88) &= ~(1<<23);
        }
        else
        {
            rNFC(0x88) |= (1<<23);
        }
    }
    
    pNfc80->CH0NFCKGCMPCNT = l_pEfuseFile->bsNFC_CHxNFCKGCMPCNT;
    pNfc80->CH1NFCKGCMPCNT = l_pEfuseFile->bsNFC_CHxNFCKGCMPCNT;
    pNfc80->CH2NFCKGCMPCNT = l_pEfuseFile->bsNFC_CHxNFCKGCMPCNT;
    pNfc80->CH3NFCKGCMPCNT = l_pEfuseFile->bsNFC_CHxNFCKGCMPCNT;

    rPMU(0x20) &= ~(0x3<<6);
    rPMU(0x20) |= (l_pEfuseFile->bsPMU_NFCLKSEL << 6);        // Set this field to 0x1ff81f20[7:6]  

    
    // set EPHY 
    for (i=0;i<EPHY_REG_NUM;i++)
    {
        if(TRUE == l_pEfuseFile->usEPHY_REG[i].bsValid)
        {
            if(0x7 == (l_pEfuseFile->usEPHY_REG[i].bsAddr >> 4))  // Addr[6:4] == 3'b111
            {
                SET_EPHY_REG(REG_BASE_EPHY,l_pEfuseFile->usEPHY_REG[i].bsAddr) = l_pEfuseFile->usEPHY_REG[i].bsData;
            }
            else
            {
                SET_EPHY_REG(REG_BASE_EPHY_LANE0,l_pEfuseFile->usEPHY_REG[i].bsAddr & 0xf ) = l_pEfuseFile->usEPHY_REG[i].bsData;
                SET_EPHY_REG(REG_BASE_EPHY_LANE1,l_pEfuseFile->usEPHY_REG[i].bsAddr & 0xf ) = l_pEfuseFile->usEPHY_REG[i].bsData;
            }
        }
    }

    // set glb register if required debug 
    if (0xff != l_pEfuseFile->bsNormal_REG0_ADDR)
    {
        rGLB(l_pEfuseFile->bsNormal_REG0_ADDR) = l_pEfuseFile->bsNormal_REG0_DATA;
    }
    
}
/************************************************
FUNC: HAL_StrapResetHostIF
OUT :  
IN  : void 
Description:
    Reset PCIE/SATA after flash boot according to EFUSE info

History : 2014/10/23 Victor Zhang  Create 

*************************************************/

void HAL_StrapResetHostIF(void)
{
    if(TRUE == HAL_StrapIsSetHwFrEfuse())
    {
        if (1 == l_pEfuseFile->bsHostIF_Reset)
        {
            if (TRUE == HAL_StrapIsPcie())
            {
                rGLB(0x18) &= ~(1<<29); 
            }
            else
            {
                rGLB(0x18) &= ~(0xf<<8);
            }
        }
    }
}

/************************************************
FUNC: HAL_StrapGetReadPRCQ
OUT :  
IN  : void 
Description:
    Set PRCQ of read flash command as EFUSE setting 

History : 2014/10/23 Victor Zhang  Create 

*************************************************/

U8 HAL_StrapGetReadPRCQ(U8* pQeTable)
{
    U8 i;
    for (i=0;i<4;i++)
    {
        if (0xff == l_pEfuseFile->ucQE[i])
        {
            break;
        }
        else
        {
            pQeTable[i] = l_pEfuseFile->ucQE[i];
        }
    }
    return i;
}

/*
FUNC: HAL_StrapUartEn
OUT : TRUE  -- enable uart 
      FALSE -- disable uart 
IN  : void 
Description:
    Determine enable uart or not.

History : 2014/10/23 Victor Zhang  Create 

*/
BOOL HAL_StrapUartEn(void)
{
    return (BOOL)(0 == l_tStrapPin.bsBootSrc2);
}

/*
FUNC: HAL_StrapMpBoot
OUT : TRUE  -- MP mode
      FALSE -- others  
IN  : void 
Description:
    Determine enter MP boot mode ,that online programing through sata/pcie .

History : 2014/10/23 Victor Zhang  Create 

*/

BOOL HAL_StrapMpBoot(void)
{
#ifndef VT3514_C0   // B0
    return (BOOL)l_tStrapPin.bsTlcMlcSel;    
#else
    if ((1 == l_tStrapPin.bsBootSrc1) && (1 == l_tStrapPin.bsBootSrc0))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
}

/************************************************
FUNC: HAL_StrapSpiBoot
OUT : TRUE  -- SPI boot mode
      FALSE -- others  
IN  : void 
Description:
    Determine enter SPI boot mode ,the rom boot program may load bootloader from
    SPI flash .

History : 2014/10/23 Victor Zhang  Create 

*************************************************/

BOOL HAL_StrapSpiBoot(void)
{
    return (BOOL) l_tStrapPin.bsEPRomSel;
}

/************************************************
FUNC: HAL_StrapIsPcie
OUT : TRUE  -- PCIE boot mode
      FALSE -- SATA boot mode  
IN  : void 
Description:
    Determine if enter PCIE mp mode or SATA mode. 
    SPI flash .

History : 2014/10/23 Victor Zhang  Create 

*************************************************/

BOOL HAL_StrapIsPcie(void)
{
    return (BOOL)l_tStrapPin.bsSataPcieSel;
}

/************************************************
FUNC: HAL_StrapNfcType
OUT : U8 NFC init mode      
    // bit[0] -- set for Async ,clear for Sync
    // bit[1] -- set for TLC , clear for MLC
    // bit[2] -- set for Onfi , clear for Toggle
    
IN  : void 

Description:
    ROM boot program may initialize NFC into matched mode according PINS
History : 2014/10/23 Victor Zhang  Create 
*************************************************/

U8  HAL_StrapNfcType(void)
{
    U8 ucType;
    ucType = 0;
    ucType |= HAL_StrapNfcIsAsync();
    ucType |= (HAL_StrapNfcIsTLC() << 1);
    ucType |= (HAL_StrapNfcIsOnfi() << 2);
    return ucType;
}
/************************************************
Function Name: HAL_StrapNfcIsOnfi
Input: 
Output:     TRUE for ONFI
            FALSE for TOGGLE
Description:
    
*************************************************/

BOOL HAL_StrapNfcIsOnfi(void)
{
    return (BOOL)l_tStrapPin.bsToggleOnfiSel;
}
/************************************************
Function Name: HAL_StrapNfcIsTLC
Input: 
Output:     TRUE for TLC
            FALSE for MLC
Description:
    
*************************************************/

BOOL HAL_StrapNfcIsTLC(void)
{
#ifndef VT3514_C0
    return FALSE;    // B0 not support TLC yet , GPIO share to SPI boot force MP
#else
    return (BOOL)l_tStrapPin.bsTlcMlcSel;
#endif
}
/************************************************
Function Name: HAL_StrapNfcIsAsync
Input: 
Output:     TRUE for ASYNC
            FALSE for SYNC
Description:
    
*************************************************/

BOOL HAL_StrapNfcIsAsync(void)
{
    return (BOOL)(FALSE == l_tStrapPin.bsAsyncSyncSel);
}
/************************************************
Function Name: HAL_StrapNfcIsNVMe
Input: 
Output:     TRUE for NVMe
            FALSE for AHCI
Description:
    
*************************************************/

BOOL HAL_StrapNfcIsNVMe(void)
{
    return (BOOL)l_tStrapPin.bsNvmeAhciSel;
}

/************************************************
Function Name: HAL_StrapGetWarmBootEntry
Input: 
Output:     entry of warm boot 
Description:
    
*************************************************/

U32 HAL_StrapGetWarmBootEntry(void)
{
    return (U32)(rPMU(0x24));
}
/************************************************
Function Name: HAL_StrapNfcIs16CeMode
Input: 
Output:     TRUE for 16ce 
            FALSE for 8ce 
Description:
    
*************************************************/

BOOL HAL_StrapNfcIs16CeMode(void)
{
    return (BOOL)l_tStrapPin.bs16CeMode;
}



