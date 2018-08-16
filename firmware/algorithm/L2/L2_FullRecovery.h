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
File Name     : L2_FullRecovery.h
Version       : Initial version
Author        : henryluo
Created       : 2015/2/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2015/2/28
Author      : henryluo
Modification: Created file

******************************************************************************/
#ifndef _L2_FULLRECOVERY_H
#define _L2_FULLRECOVERY_H
#include "Disk_Config.h"
#include "L2_PBIT.h"

extern GLOBAL  BOOL bTBRebuildPending; 
extern GLOBAL  BOOL *bTBRebuildEraseAll;
extern GLOBAL  U32  *gulMaxTableBlockTS;
extern GLOBAL  PBIT *pPBIT_Temp[SUBSYSTEM_SUPERPU_MAX];
U32 L2_TB_RollBackEraseCount(void);
U32 L2_TB_FullRecovery(void);
void L2_TB_Rebuild_TLCSrcBlk(U32 uPu,RED *pRed);

#endif

/********************** FILE END ***************/
