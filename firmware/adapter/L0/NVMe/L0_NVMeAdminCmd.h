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
* File Name    : L0_NVMeAdminCmd.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.18
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L0_NVMEADMINCMD_H
#define _L0_NVMEADMINCMD_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define CLR_HW_SQ_PTR(SQID)  (g_pNVMeCfgReg->sq_clr = 1<<(SQID))
#define CLR_HW_CQ_PTR(CQID)  (g_pNVMeCfgReg->cq_clr = 1<<(CQID))

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/



/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
BOOL L0_NVMeProcessAdminCmd(PCB_MGR pSlot);

BOOL L0_NVMeCreateCQ(PCB_MGR pSlot);
BOOL L0_NVMeCreateSQ(PCB_MGR pSlot);
BOOL L0_NVMeDeleteSQ(PCB_MGR pSlot);
BOOL L0_NVMeDeleteCQ(PCB_MGR pSlot);

BOOL L0_NVMeIdentify(PCB_MGR pSlot);
void L0_NVMeFeatureTblInit(void);
BOOL L0_NVMeGetFeatures(PCB_MGR pSlot);
BOOL L0_NVMeSetFeatures(PCB_MGR pSlot);
BOOL L0_NVMeGetLogPage(PCB_MGR pSlot);
void L0_NVMeAsyncEvent(PCB_MGR pSlot);

BOOL L0_NVMeAbort(PCB_MGR pSlot);
BOOL L0_NVMeFwCommit(PCB_MGR pSlot);
BOOL L0_NVMeImgDownload(PCB_MGR pSlot);
BOOL L0_NVMeFormatNvm(PCB_MGR pSlot);

BOOL L0_NVMeVIACmd(PCB_MGR pSlot);


#endif
/*====================End of this head file===================================*/

