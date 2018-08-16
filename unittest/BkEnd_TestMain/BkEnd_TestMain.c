/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : BkEnd_TestMain.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.17
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "BkEnd_TestMain.h"

/*============================================================================*/
/* HAL Driver Unit Test Case, Add your unit test case freely....              */
/*============================================================================*/
// Step1: Open the related test macro.
#ifdef HAL_UNIT_TEST
//#define HAL_DRIVER_TEST
//#define HAL_NFC_TEST
//#define HAL_XORE_TEST
//#define HAL_LDPC_TEST
#endif

// Step2: extern the related test case.
extern BOOL UT_FlashDriver(void);
extern BOOL UT_NfcMain(void);
extern BOOL UT_XorMain(void);
extern BOOL UT_LdpcMain(void);

// Step3: register to the UT_CASE list.
LOCAL UT_CASE aFunHalCase =
{
    #ifdef HAL_DRIVER_TEST
    UT_FlashDriver,
    #endif
    #ifdef HAL_NFC_TEST
    UT_NfcMain,
    #endif
    #ifdef HAL_XORE_TEST
    UT_XorMain,
    #endif
    #ifdef HAL_LDPC_TEST
    UT_LdpcMain,
    #endif
    // Add your test case here.
    
    NULL // Don't delete it.
};
/*============================================================================*/
/* L3 Unit Test Case, Add your unit test case freely....                      */
/*============================================================================*/
// Step1: Open the related test macro.
#ifdef L3_UNIT_TEST
#define L3_LLFBOOT_TEST
#define L3_FCMD_TEST
//#define L3_SEQ4KR_TEST
//#define L3_RD_STRESS_TEST
//#define L3_MIXSTRESS_TEST
//#define L3_BBT_TEST
//#define L3_XORE_TEST
//#define L3_STRESS_TEST
#endif

// Step2: extern the related test case.
extern BOOL L3_LLFBootTest(void);
extern BOOL L3_FCmdTest(void);
extern BOOL L3_Seq4kRTest(void);
extern BOOL L3_ReadStressTest(void);
extern BOOL L3_MixStressTest(void);
extern BOOL L3_BbtTest(void);
extern BOOL L3_XorTest(void);

// Step3: register to the UT_CASE list.
LOCAL UT_CASE aFunL3Case = 
{
    #ifdef L3_LLFBOOT_TEST
    L3_LLFBootTest,
    #endif
    #ifdef L3_FCMD_TEST
    L3_FCmdTest,
    #endif
    #ifdef L3_SEQ4KR_TEST
    L3_Seq4kRTest,
    #endif
    #ifdef L3_RD_STRESS_TEST
    L3_ReadStressTest,
    #endif
    #ifdef L3_MIXSTRESS_TEST
    L3_MixStressTest,
    #endif
    #ifdef L3_BBT_TEST
    L3_BbtTest,
    #endif
    #ifdef L3_XORE_TEST
    L3_XorTest,
    #endif
            
    NULL // Don't delete it.
};

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : HAL_UnitTest
Input      : NONE
Output     : NONE
Return Val : 
Discription: Only for test the hal driver.
Usage      : Called in BkEnd_Scheduler.
History    :
    1. 2016.3.17 JasonGuo create function
==============================================================================*/
void HAL_UnitTest(void)
{
    U32 ulIndex = 0;

    #ifdef HAL_UNIT_TEST
    if (sizeof(aFunHalCase[0]) == sizeof(aFunHalCase))
    {
        DBG_Printf("MCU#%d HAL_UnitTest has no test case.\n", HAL_GetMcuId());
        DBG_Getch();
    }
    #endif
    
    while (NULL != aFunHalCase[ulIndex])
    {
        if (TRUE == aFunHalCase[ulIndex]())
        {
            ulIndex++;
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_UnitTest
Input      : NONE
Output     : NONE
Return Val : 
Discription: Simulate L2 to send flash request to L3. 
Usage      : Called in FTL_Scheduler.
History    :
    1. 2016.3.17 JasonGuo create function
==============================================================================*/
void L3_UnitTest(void)
{
    U32 ulIndex = 0;

    #ifdef L3_UNIT_TEST
    if (sizeof(aFunL3Case[0]) == sizeof(aFunL3Case))
    {
        DBG_Printf("MCU#%d L3_UnitTest has no test case.\n", HAL_GetMcuId());
        DBG_Getch();
    }
    #endif

    while (NULL != aFunL3Case[ulIndex])
    {
        if (TRUE == aFunL3Case[ulIndex]())
        {
            ulIndex++;
        }
    }

    return;
}
/*====================End of this file========================================*/

