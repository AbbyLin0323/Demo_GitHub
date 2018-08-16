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
  File Name     : COM_BitMask.h
  Version       : Initial Draft for V1 FW bitmask
  Author        : BlakeZhang
  Created       : 2014/5/31
  Description   :
  Description   : 
  Function List :
  History       :
  1.Date        : 2014/5/31
    Author      : BlakeZhang
    Modification: Created file

******************************************************************************/
#ifndef _COM_BITMASK_
#define _COM_BITMASK_

#define COM_GETBITMASK(StartBit,EndBit) (U32)((INVALID_8F << (StartBit)) & (INVALID_8F >> (31 - (EndBit)))) 

extern void COM_BitMaskSet(U32* pulBitMask, U8 ucIndex);
extern void COM_BitMaskClear(U32* pulBitMask, U8 ucIndex);
extern BOOL COM_BitMaskGet(U32 ulBitMask, U8 ucIndex);

extern void COM_8BitMaskSet(U8* pulBitMask, U8 ucIndex);
extern void COM_8BitMaskClear(U8* pulBitMask, U8 ucIndex);
extern BOOL COM_8BitMaskGet(U8 ulBitMask, U8 ucIndex);

#endif

