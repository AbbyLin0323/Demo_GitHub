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
* File Name    : L2_NVMe.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.2
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L2_NVME_H
#define _L2_NVME_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_ChainMaintain.h"
#include "L1_Interface.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _RdWithoutWtDptr_{
    HMEM_DPTR HmemDptr;
    U32 LBA;

    U16 HsgId;
    U16 NextHsgId;
    U16 DsgId;
    U16 FirstHsgId;

    U16 FirstDsgId;
    U8 LPNStartPos;
    U8 LPNNextPos;

    U32 ReqByteLen;// we only process flash page size data
    U32 RemainByteLen;// we only process flash page size data

    U32 bHsgDone : 1;
    U32 bDsgDone :1;
    U32 Initialized :1;
    U32 FirstRCMDEn : 1;
    U32 Rsvd : 28;
    
    BUFREQ_HOST_INFO* pBufReqHostInfo;

}RdWithoutWtDptr;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/

extern MCU12_VAR_ATTR RdWithoutWtDptr *l_RdWithoutWtDptr;

extern void L2_NVMeInitRdWithoutWtDptr();
extern void L2_MoveHostPointer(HMEM_DPTR *PHostInfo, U32 SecLen);
extern BOOL L2_FtlHandleReadWithoutWrite(U8 ucSuperPu, BUF_REQ_READ* pReq, BUFREQ_HOST_INFO* pBufReqHostInfo, U8 LPNOffset, U8 ucReqLenth, U8 ucReqLPNCnt, BOOL bFirstCMDEn);

#endif
/*====================End of this head file===================================*/

