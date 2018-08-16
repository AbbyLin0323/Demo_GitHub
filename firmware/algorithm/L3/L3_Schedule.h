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
* File Name    : L3_Schedule.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_SCHEDULE_H
#define _L3_SCHEDULE_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
// TLun Status Bitmap
typedef enum _STS_BITMAP_TYPE_
{
    STS_BMP_POPREQ = 0,
    STS_BMP_PEND,
    STS_BMP_RECYCLE,
    STS_BMP_ERRH,
    STS_BMP_EXTH,
    STS_BMP_LOCK,
    STS_BMP_NFC_UNFULL,
    STS_BMP_NFC_EMPTY,
    STS_BMP_NFC_ERROR,
    STS_BMP_NUM
}STS_BITMAP_TYPE;

// TLun Arbitration Bitmap
typedef enum _ARB_BITMAP_TYPE_
{
    ARB_BMP_POPREQ = 0,
    ARB_BMP_PEND,
    ARB_BMP_RECYCLE,
    ARB_BMP_ERRH,
    ARB_BMP_EXTH,
    ARB_BMP_NUM
}ARB_BITMAP_TYPE;

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L3_SchInit(void);
void L3_Scheduler(void);

void L3_SchUpdateStsBmp(U8 ucLunInPU, STS_BITMAP_TYPE eStsBMType);
BOOL L3_SchGetStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType);
void L3_SchSetStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType);
void L3_SchClrStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType);
void L3_SchClrArbBit(U8 ucTLun, ARB_BITMAP_TYPE eArbBmpType);
void L3_SchClrArbBits(U8 ucTLun);

#endif
/*====================End of this head file===================================*/

