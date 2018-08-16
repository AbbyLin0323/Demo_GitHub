/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L3_SoftDecoder.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_SOFTDECODER_H
#define _L3_SOFTDECODER_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L3_FCMDQ.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef enum _LDPC_SOFT_DECODE_STATE
{
    SOFT_DECO_INIT = 0x0,
    SOFT_DECO_ALLOCATE_BUF,
    SOFT_DECO_READ_RAW_DATA,
    SOFT_DECO_TRIG,
    SOFT_DECO_TRIG_CHK,
    SOFT_DECO_SUCCESS,
    SOFT_DECO_FAIL2XOR,
    SOFT_DECO_FAIL2FAIL
}SOFT_DECODER_STAGE;

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void MCU2_DRAM_TEXT L3_SoftDecoder(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

#endif
/*====================End of this head file===================================*/

