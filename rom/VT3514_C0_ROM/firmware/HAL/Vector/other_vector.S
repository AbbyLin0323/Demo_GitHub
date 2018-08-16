/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
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

        .section .WindowVectors.text,"ax"
        .align 4
        .global _pre_start
_pre_start:
        call0 _start
        .text


        .section .DoubleExceptionVector.text,"ax"
        .align 4
        rsr     a3, PS
        movi    a6, 0xffe80000
        s32i    a3, a6, 0
        rsr     a3, EXCCAUSE
        s32i    a3, a6, 4

        .text


        .section .Level2InterruptVector.text, "ax"
        .align 4
        .global _Level2Vector

_Level2Vector:
        wsr.excsave2    a1
        rotw    4          # change to foreground register set
        xsr.excsave2    a1
        rsr.sar    a15        # save the SAR in a callee save register
        movi	a3, HAL_FlashIntEntry
        callx0    a3
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
        movi	a3, HAL_SATAIntEntry
        callx0    a3
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
        movi	a3, HAL_L4TimerIntEntry
        callx0    a3
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfi     4
#
        .text

        .section .UserExceptionVector.text,"ax"
        .align 4
        .global _UserExceptionVector
_UserExceptionVector:
        wsr.excsave4    a1
        rotw 4    # change to foreground register set
        xsr.excsave4    a1
        rsr.sar a15        # save the SAR in a callee save register
        movi a3, HAL_UserExcpEntry
        callx0    a3
        wsr.sar    a15        # restore the SAR from a callee save register
        rotw    -4         # change to background register set
        rfe

