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
* File Name    : L0_NVMeDataIO.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.18
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L0_NVMEDATAIO_H
#define _L0_NVMEDATAIO_H

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


/* Key data structure for data transfer programming parameters (for both input and output). */
typedef struct _L0_NVMe_DATAREQ
{
    /* 1. Transfer parameters: start address/length. */
    U32 ulDRAMBuffStart;
    U32 ulByteLen;
    U32 ulHSGRemBytes;

    /* 2. Command slot number. */
    U8  ucHCmdSlt;
    /* 3. Data transfer direction: FALSE - host to controller; TRUE - controller to host. */
    U8  ucDataIsC2H;
    /* 4. DRQ/DWQ building progress tracking. */
    U8  ucReqBuildStatus;
    U8  ucCurrentPRP;
    
    U16 usTotalPRPCount;
    U16 usPrevHSGId;
    U16 usFirstHSGId;
    U16 usFirstDSGId;
} L0_NVMeDATAREQ, *PL0_NVMeDATAREQ;

/* Data transfer state machine state. */
#define NVME_DATAXFER_START  0
#define NVME_DATAXFER_SETUP  1
#define NVME_DATAXFER_INPRGS 2
#define NVME_DATAXFER_DONE   3

/* DRQ/DWQ building state machine state. */
#define NVME_DATAIO_BUILDDSG 0
#define NVME_DATAIO_BUILDHSG 1
#define NVME_DATAIO_BUILDDRQ 2
#define NVME_DATAIO_BUILDEND 3

/* Cache status define. */
#define L0_HOSTDATAXFER_FINISHED 0
#define L0_HOSTDATAXFER_BUSY 1

/* Interface routine declaration. */
void L0_HostDataXferInit(void);
U32  L0_CreateHostDataRequest(PL0_NVMeDATAREQ pHostDataReq);
void L0_SetDataBuffBusy(void);
BOOL L0_DataXferDone(void);
BOOL L0_NVMeXferData(PCB_MGR pSlot);
void L0_ClearDataBuff(void);


#endif
/*====================End of this head file===================================*/

