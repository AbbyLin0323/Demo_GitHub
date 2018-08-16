    .section        .ResetVector.text, "ax"
    .align    4
    .global    _ResetVector
_ResetVector:  
    j    _ResetHandler

    .size    _ResetVector, . - _ResetVector
    .align    4
    .literal_position    // tells the assembler/linker to place literals here

_ResetHandler:
     movi a3,_start
     callx0 a3

    .section    .KernelExceptionVector.text, "ax"
    .align    4
    .global _KernelExceptionVector
_KernelExceptionVector:
    waiti 4
    rfe

    .section    .UserExceptionVector.text, "ax"
    .align    4
    .global _UserExceptionVector
_UserExceptionVector:
    waiti 4
    rfe

    .section    .DoubleExceptionVector.text, "ax"
    .align    4
    .global _DoubleExceptionVector
_DoubleExceptionVector:
    waiti 4
    rfde

    .section    .DebugExceptionVector.text, "ax"
    .align    4
    .global _DebugExceptionVector
_DebugExceptionVector:
    waiti 4
    rfe

