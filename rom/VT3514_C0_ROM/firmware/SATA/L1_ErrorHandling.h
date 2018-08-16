/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : L1_ErrorHandling.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2013.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L1_ERRORHANDLING_H
#define _L1_ERRORHANDLING_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L1_ErrHandle_NotSupportCmd(void);
void L1_ErrHandle_SataLinkErr(void);
void L1_ErrHandle_SataSerrISR(void);
void L1_ErrHandle_SyncEscapeISR(void);
BOOL L1_ErrHandle_CheckOverriddenCmdTag(U8 curCmdTag);
void L1_Sim_FwTrigDevicePowerMgt(void);

void L1_SetErrorEvent(U32 ulEventType);
void L1_ErrorHandling(void);


#endif
/*====================End of this head file===================================*/

