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
Filename     :  head_enter.s
Version      :  Ver 1.0
Date         :
Author       :  

Description:
entrance for MCU0's head.
Bootloader/ROM will jump here for system cold/warm boot
Modification History:
2014/06/23   gavinyin     001 add header part of this file
*******************************************************************************/
#include <xtensa/coreasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/cacheasm.h>

        .section .mcu0entrance.text, "ax"
        .align 4
        .global mcu0entrance
mcu0entrance:
        movi    sp, __stack    // setup the stack
        call0   HEAD_FirstRelocateEntry
        movi    a2, HEAD_Main
        callx0  a2

        .text

        .section .mcu0dram.text, "ax"
        .align 4
        .global mcu0warmbootentrance
mcu0warmbootentrance:
        movi    sp, __stack    // setup the stack
        call0   HEAD_Main

        .text

