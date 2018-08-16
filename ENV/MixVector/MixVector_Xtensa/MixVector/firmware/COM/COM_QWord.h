/****************************************************************************
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
*****************************************************************************
Filename     :  COM_QWord.h                                     
Version      :  Ver 1.0                                               
Date         :  20140611                                     
Author       :  Gavin

Description: 
    header file for COM_Qword.c

Modification History:
20140611     gavinyin     001 create file
*****************************************************************************/
#ifndef _COM_QWORD_H__
#define _COM_QWORD_H__

#include "BaseDef.h"

//U64 operation
extern void COM_QwAddDw(QWORD *pQwIn, U32 ulDwIn, QWORD *pQwOut);
extern void COM_QwSubDw(QWORD *pQwIn, U32 ulDwIn, QWORD *pQwOut);
extern BOOL COM_QwCmp(QWORD *pQwA, QWORD *pQwB);

#endif/* _COM_QWORD_H__ */
