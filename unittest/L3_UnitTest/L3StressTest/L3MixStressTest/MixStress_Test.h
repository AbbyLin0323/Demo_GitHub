/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : MixStress_Test.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2014.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _MIXSTRESS_TEST_H
#define _MIXSTRESS_TEST_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "L3_BufMgr.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
typedef enum _RD_STRESS_TEST_STAGE_
{
    RD_STRESS_TEST_STAGE_INIT = 0,
    RD_STRESS_TEST_STAGE_RUN,
    RD_STRESS_TEST_STAGE_FINISH
}RD_STRESS_TEST_STAGE;

typedef enum _L3_UT_READSTRESS_STATE
{
    L3_UT_READSTRESS_STATE_INIT = 0,
    L3_UT_READSTRESS_STATE_ERASE,    
    L3_UT_READSTRESS_STATE_WRITE,
    L3_UT_READSTRESS_STATE_READ,
    L3_UT_READSTRESS_STATE_RD_RED,
    L3_UT_READSTRESS_STATE_RD_DATA,
    L3_UT_READSTRESS_STATE_RD_LPN,
    L3_UT_READSTRESS_STATE_RD_MERGE,
    L3_UT_READSTRESS_STATE_RD_4KSEQ,
    L3_UT_READSTRESS_STATE_DONE
}L3_UT_READSTRESS_STATE;


/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _L3_UT_SUPERPU_MG_DPTR{
    U32 bsResvd : 32;
    U32 ulLunBitMap;
}L3_UT_SUPERPU_MG_DPTR;

typedef struct _L3_UT_STRESS_READ_MGR{
    U16 usWtBlkCnt;
    U16 usOpenBlk;
    U16 usPgCnt;
    U8 ucStage;
    U8 ucRsvd;
}L3_UT_STRESS_READ_MGR;

typedef struct _L3_UT_STRESS_ERR_MGR{
    U32 ulErrCnt;
}L3_UT_STRESS_ERR_MGR;
/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
BOOL L3_MixStressTest(void);
GLOBAL BOOL L3_UTUpdateLUNBitMap(U8 ucSuperPu);
GLOBAL U16 L3_UTRandBlk();
GLOBAL U16 L3_UTRandPg();
GLOBAL BOOL L3_IsSuperPUDone(U8 ucSuperPu);
GLOBAL void L3_UTRdStressInit(void);
U8 L3_UTSelectLunForSend(U8 ucSuperPu);

#endif
/*====================End of this head file===================================*/

