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
Filename    : other_vector.S
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.23
Description : this file define dynamic vector entries for MCU
Others      : this file do NOT include reset vector
Modify      :
20140923    Gavin     move vectors in old vector.S to this file, except reset vector
****************************************************************************/
#include <xtensa/coreasm.h>
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-xer.h>
#include <xtensa/config/specreg.h>
#include <xtensa/config/system.h>

#define OTFB_START_ADDRESS        0xFFF00000

        .align 4
_save_int_ps:
        rsr.lbeg a14
        rsr.lend a13
        rsr.lcount a12
        //rsr.ps a11
        //mov a2, a11
        //movi a3, ((0 << 4) + 15)
        //insbits a2, a3, 0, 4
        //wsr.ps a2
        ret


        .align 4
_rest_int_ps:
        wsr.lcount a12
        wsr.lend a13
        wsr.lbeg a14
        //wsr.ps a11
        ret


        .text


        .section .DoubleExceptionVector.text,"ax"
        .align 4
        rsr     a3, DEPC
        movi    a6, (OTFB_START_ADDRESS + 0x10)
        s32i    a3, a6, 0
        rsr     a3, EXCCAUSE
        s32i    a3, a6, 4
1:
        j       1b

        .text


        .section .Level2InterruptVector.text, "ax"
        .align 4
        .global _Level2Vector

_Level2Vector:
        wsr.excsave2    a1
        rotw    4          # change to foreground register set
        xsr.excsave2    a1
        rsr.sar    a15        # save the SAR in a callee save register
        call0 _save_int_ps
        //movi    a3, HAL_FlashIntEntry
        //callx0    a3
        call0   HAL_FlashIntEntry
        call0 _rest_int_ps
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfi    2
#
        .text


        .section .Level3InterruptVector.text, "ax"
        .align 4
        .global _Level3Vector
_Level3Vector:
        wsr.excsave3    a1
        rotw    4          # change to foreground register set
        xsr.excsave3    a1
        rsr.sar    a15        # save the SAR in a callee save register
        call0 _save_int_ps
        //movi    a3, HAL_SATAIntEntry
        //callx0    a3
        call0   HAL_SATAIntEntry
        call0 _rest_int_ps
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfi    3
#
        .text

        .section .Level4InterruptVector.text, "ax"
        .align 4
        .global _Level4Vector
_Level4Vector:
        wsr.excsave4    a1
        rotw    4          # change to foreground register set
        xsr.excsave4    a1
        rsr.sar    a15        # save the SAR in a callee save register
        call0 _save_int_ps
        //movi    a3, HAL_L4TimerIntEntry
        //callx0    a3
        call0   HAL_L4TimerIntEntry
        call0 _rest_int_ps
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfi     4
#
        .text

        .section .Level5InterruptVector.text, "ax"
        .align 4
        .global _Level5Vector
_Level5Vector:
        wsr.excsave5    a1
        rotw    4          # change to foreground register set
        xsr.excsave5    a1
        rsr.sar    a15        # save the SAR in a callee save register
        call0 _save_int_ps
        //movi    a3, HAL_L5TimerIntEntry
        //callx0    a3
        call0   HAL_L5TimerIntEntry
        call0 _rest_int_ps
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfi     5
#
        .text

        .section .UserExceptionVector.text,"ax"
        .align 4
        .global _UserExceptionVector
_UserExceptionVector:
        wsr.excsave1    a1
        rotw 4    # change to foreground register set
        xsr.excsave1    a1
        rsr.sar a15        # save the SAR in a callee save register
        call0 _save_int_ps
        //movi a3, HAL_UserExcpEntry
        //callx0    a3
        call0   HAL_UserExcpEntry
        call0 _rest_int_ps
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfe

