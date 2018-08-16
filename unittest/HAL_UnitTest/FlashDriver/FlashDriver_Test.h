/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : FlashDriver_Test.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.3
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _FLASHDRIVER_TEST_H
#define _FLASHDRIVER_TEST_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
typedef enum _UT_FLASH_DRIVER_STAGE_
{
    UT_FLASH_DRIVER_INIT = 0,
    UT_FLASH_DRIVER_RUN,
    UT_FLASH_DRIVER_FINISH
}UT_FLASH_DRIVER_STAGE;
/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _L3_UT_FLASH_CASE{
    U32  bsStartPU:8;
    U32  bsEndPU:8;
    U32  bsStartBlk:16;
    U32  bsEndBlk:16;
    U32  bsPageCnt:16;
    U32  bsSinglePln:1;
    U32  bsPlnNum:2;
    U32  bsSlcMode:1;
    U32  bsResvd:28;
}L3_UT_FLASH_CASE;

typedef struct _L3_UT_EH_BUFF
{
    U32 aTargetAddr[PG_PER_BLK];
    U32 aSecNum[PG_PER_BLK];
    NFC_RED *ptReadRedSW[PG_PER_BLK];
    U32 aDwNum[PG_PER_BLK];
}L3_UT_EH_BUFF;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
BOOL UT_FlashDriver(void);

#endif
/*====================End of this head file===================================*/

