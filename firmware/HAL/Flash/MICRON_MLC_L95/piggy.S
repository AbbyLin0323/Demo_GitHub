	.section .mcu2dram.text,"a"
	.align 1024
	.globl	LdpcMatParaAddr
LdpcMatParaAddr:
	.incbin	"./HAL/Flash/ldpcmatrix/LdpcMatrixSet0_136-72-88-104.bin"
	.globl	LdpcMatParaAddrEnd
LdpcMatParaAddrEnd:
