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
* File Name    : L3_ErrHExtend.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_ERRHEXTEND_H
#define _L3_ERRHEXTEND_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define EXTH_CPY_BUF_CNT (9)
// check the read empty-page fail when read src-block in copy-data process.
#define EXTH_CHECK_READ_EMPTY_PAGE_EN

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef enum _EXTH_STAGE_
{
    EXTH_INIT = 0,
    EXTH_CHECK_RPMT,
    EXTH_ALLOC_NEWBLK,
    EXTH_COPY_DATA,
    EXTH_WRITE_LASTPAGE,
    EXTH_ERASE_BLK,
    EXTH_REPORT_STS,
    EXTH_DONE
}EXTH_STAGE;

typedef enum _EXTH_SUBSTAGE_
{
    EXTH_SUB_INIT = 0,
    EXTH_SUB_READ,
    EXTH_SUB_READ_CHK,
    EXTH_SUB_WRITE,
    EXTH_SUB_WRITE_CHK,
    EXTH_SUB_ERASE,
    EXTH_SUB_ERASE_CHK,
    EXTH_SUB_SUCCESS,
    EXTH_SUB_FAIL,
    EXTH_SUB_DONE
}EXTH_SUBSTAGE;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L3_ExtHAllcDram(U32 *pFreeDramBase);
void L3_ExtHHandling(U8 ucTLun);
BOOL L3_ExtHTrigger(U8 ucTLun);
void MCU2_DRAM_TEXT L3_ExtHPrcPendingCmd(U8 ucTLun, U16 usVirBlk, U16 usPhyBlk, BOOL bBak);

#endif
/*====================End of this head file===================================*/

