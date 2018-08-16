/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : L1_NcqNonData.h
* Discription  : see also L1_NcqNonData.c
* CreateAuthor : Haven Yang
* CreateDate   : 2013.11.28
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L1_NCQNONDATA_H
#define _L1_NCQNONDATA_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_SataIO.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/* subcommands for NCQ non-data(0x63) */
#define NND_SUBCMD_ABORT_NCQQ           0x00    /* VT3514 support */
#define NND_SUBCMD_DL_HANDLING          0x01    /* VT3514 support */
#define NND_SUBCMD_HB_DEMOTE_BY_SIZE    0x02    /* VT3514 won't support */
#define NND_SUBCMD_HB_CHANGE_BY_LBA     0x03    /* VT3514 won't support */
#define NND_SUBCMD_HB_CONTROL           0x04    /* VT3514 won't support */
#define NND_SUBCMD_SET_FEATURES         0x05    /* VT3514 support */

/* Abort Type: for subcommand is NND_SUBCMD_ABORT_NCQQ */
#define ABT_ABORT_ALL                   0x00
#define ABT_STREAMING                   0x01
#define ABT_NON_STREAMING               0x02
#define ABT_SELECTED                    0x03

/* subcommands for Send FPDMA Queued(0x64) */
#define NCQ_SEND_SUBCMD_SET_MANAGEMENT  0x00    /* VT3514 support */
#define NCQ_SEND_SUBCMD_HB_EVICT        0x01    /* VT3514 won't support */
#define NCQ_SEND_SUBCMD_WRITE_LOG       0x02    /* VT3514 support */

/* subcommands for Receive FPDMA Queued(0x64) */
#define NCQ_RECV_SUBCMD_RESERVED        0x00    /* VT3514 won't support */
#define NCQ_RECV_SUBCMD_READ_LOG        0x01    /* VT3514 support */

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
BOOL L1_HandleNCQNonData(HCMD* pCurHCMD);


/* 0x64/65 presupport/pseudosupport,just a framework */
BOOL L1_HandleNCQSendFPDMA(HCMD* pCurHCMD);
BOOL L1_HandleNCQReceiveFPDMA(HCMD* pCurHCMD);

#endif
/*====================End of this head file===================================*/

