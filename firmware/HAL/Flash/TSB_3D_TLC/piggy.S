	.section .mcu2dram.text,"a"
	.align 1024
	.globl	LdpcMatParaAddr
LdpcMatParaAddr:
	.incbin	"./HAL/Flash/ldpcmatrix/LdpcMatrixSet1_136-112-120-128.bin"
	.globl	LdpcMatParaAddrEnd
LdpcMatParaAddrEnd:
