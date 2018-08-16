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
Filename     : crt1-tiny.S
Version      : 
Date         : 
Author       : 
Others: ported from Tensilica's reference code
Modification History:
*******************************************************************************/

#include <xtensa/coreasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/cacheasm.h>

// Imports
//    __stack            from linker script (see LSP Ref Manual)
//    _bss_start        from linker script (see LSP Ref Manual)
//    _bss_end        from linker script (see LSP Ref Manual)
//    main            from user application



#ifdef __XTENSA_CALL0_ABI__
# define CALL        call0
#else
# define CALL        call4
#endif



/**************************************************************************/

    .text
    .align 4
    .global    _start
_start:
    //  _start is typically NOT at the beginning of the text segment --
    //  it is always called from either the reset vector or other code
    //  that does equivalent initialization (such as crt0-app.S).
    //  See crt1-boards.S for assumptions on entry to _start ,
    //  and for comments on what is being done in this file.

#if !XCHAL_HAVE_HALT || !XCHAL_HAVE_BOOTLOADER        // not needed for Xtensa TX
    movi    a0, 0        // mark base of call stack
#endif

    movi    sp, __stack    // setup the stack

//    rsr     a3, PS
//    movi    a6, 0x30000
//    s32i    a3, a6, 0

#if XCHAL_HAVE_EXCEPTIONS
    movi  a3, (PS_UM | 15)   // Keep interrupt masked
    wsr    a3, PS            // setup PS for the application
    rsync
#endif

    .global    _clear_bss
_clear_bss:
#ifdef MCU0
    // MCU0: Reads rPMU(0x3C) reigster to check boot type.
    // If it is a warm boot, we shall not clear either bss section.
    movi    a2, 0x1ff81f3c
    l32i    a3, a2, 0
    bnez    a3, __c_entry
#else
    // MCU1/2: Reads warm start flag in P-Table to check boot type.
    // If it is a warm boot, we shall not clear either bss section
    // or MCU1/2 shared data section.
    call0   HAL_GetWarmBootFlag
    bnez    a2, __c_entry
#endif

    // Clears the BSS (uninitialized data) segment.
    movi    a5, 0
    movi    a6, _bss_start
    movi    a7, _bss_end
    sub     a7, a7, a6
    srli    a7, a7, 2
    beqz    a7, __clear_bss_end
    loop    a7, __clear_bss_end
    s32i    a5, a6, 0
    addi    a6, a6, 4

__clear_bss_end:
#ifdef MCU1
    // MCU1: Clears MCU1/2 shared data area.
    movi    a6, _mcu12_bss_start
    movi    a7, _mcu12_bss_end
    sub     a7, a7, a6
    srli    a7, a7, 2
    beqz    a7, __c_entry
    loop    a7, __c_entry
    s32i    a5, a6, 0
    addi    a6, a6, 4
#endif

__c_entry:    
    //  We can now call C code, the C calling environment is initialized.
    //  This tiny C runtime assumes main is declared as "void main(void)"
    //  rather than with the usual argc,argv.  So there are no arguments.
    CALL    main

    //  In this tiny C runtime, main() is not expected to return.
    //  If it does, just loop forever.

    //CALL    xthal_dcache_all_writeback    // sync dirty dcaches to memory
    //extw            // sync TIE queues/ports/etc (LX or later only)

.L0:
#if XCHAL_HAVE_DEBUG
    break    1, 15        // give control to debugger
#elif XCHAL_HAVE_HALT
    halt
#endif
    j    .L0

    .size    _start, . - _start

    .text
    .align 4
    .global    READ_PS
READ_PS:
           rsr a2 ,PS
           ret.n
// Local Variables:
// mode:fundamental
// comment-start: "// "
// comment-start-skip: "// *"
// End:
