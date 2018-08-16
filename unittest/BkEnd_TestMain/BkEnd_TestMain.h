/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : BkEnd_TestMain.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.17
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _BKEND_TESTMAIN_H
#define _BKEND_TESTMAIN_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef BOOL (*UT_CASE[])(void);

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L3_UnitTest(void);
void HAL_UnitTest(void);

#endif
/*====================End of this head file===================================*/

