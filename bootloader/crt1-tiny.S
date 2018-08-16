// crt1-tiny.S
//
// This is a reduced version of the code in crt1-boards.S .
// For most hardware / boards, this code sets up the C calling context
// (setting up stack, PS, and clearing BSS) and calls main().
// It has some limitations (see LSP Ref Manual for details) such as:
//    - does not setup the C library (...)
//    - does not call C++ static constructors and destructors
//    - only clears .bss , not other *.bss sections
//
// Control arrives here at _start from the reset vector or from crt0-app.S.

// Copyright (c) 1998-2010 Tensilica Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <xtensa/coreasm.h>


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

.section        .otfb_entry, "ax"
    .align 4
_entry:    
    call0 get_pc
    .align 4
get_pc:
    movi a4, 3
    sub  a0, a0, a4
    movi a8, 0x533
    beq  a8, a10, do_copy
    movi a8, 0xFF
    slli a8, a8, 8
    addi a8, a8, 0xFF
    and  a8, a0, a8
    movi a9, 8
    beq  a8, a9, code_start
do_copy:
    movi a10, 0x533
    srai a7, a0, 20
    slli a7, a7, 20
    addi a7, a7, 8
    movi a4, 8
    sub a3, a0, a4
    sub a5, a7, a4
    mov a8, a9
    mov a9, a0
    addi a4, a3, 0x200
    bne  a9, a7, copy_vector
    movi a4, 8
    sub a3, a8, a4
    addi a4, a3, 0x3C00
copy_vector:
	l32i a6, a3, 0
	s32i a6, a5, 0
	addi a3, a3, 4
	addi a5, a5, 4
	bltu a3, a4, copy_vector
    bne  a9, a7, do_again
code_start:
    j   _start
do_again:
    jx  a7

.section        .text, "ax"
    
    .align 4
    .global    _start
_start:
    movi    sp, 0x1ffa6000    // setup the stack
    CALL    main


    .size    _start, . - _start


.section .otfb_signtr, "ax"
    //CALL    main//for hack old rom code
    .ascii "VT3514C0"
    //.word "vt3492a1"
    //.word 0x89abcdef

