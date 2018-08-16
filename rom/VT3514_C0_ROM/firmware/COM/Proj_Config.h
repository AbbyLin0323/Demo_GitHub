#ifndef PROJ_CONFIG_H__
#define PROJ_CONFIG_H__

#include "BaseDef.h"
//#define COSIM         // FSO enable
//#define FPGA
#define ASIC          
#define VT3514_C0       // FSO enable
#define DMAE_ENABLE     // FSO enable
//#define JTAG_DEBUG


//debug info
#define rTracer *(volatile U32*)(0x1ff80080)
#ifdef COSIM
#define DBG_TRACE(_x_)      rTracer = (_x_)
typedef enum SATA_ENTER_PROC__
{
    SATA_ENTER_PROC_DMA_WRITE = 0x4101,
    SATA_ENTER_PROC_DMA_READ,

    SATA_ENTER_PROC_VENDOR_REG_WR,
    SATA_ENTER_PROC_VENDOR_REG_RD,
    SATA_ENTER_PROC_VENDOR_EXE
}SATA_ENTER_PROC_;

typedef enum SATA_RECEIVE_INT__
{
    SATA_RECEIVE_INT_COMRESET = 0x4001,
    SATA_RECEIVE_INT_OOB,
    SATA_RECEIVE_INT_RECEIVE_COMMAND,
    SATA_RECEIVE_INT_SOFTRESET,
    SATA_RECEIVE_INT_RECEIVE_PIO_DATA
}SATA_RECEIVE_INT_;

#define TRACE_UART_ENABLE  0x01000000
#define TRACE_WARM_BOOT    0x02000000
#define TRACE_NFC_INIT     0x03000000
#define TRACE_PCIE_MP      0x04000000
#define TRACE_SATA_MP      0x05000000
#define TRACE_SPI_BOOT     0x06000000
#define TRACE_RESET_PU     0x07000000
#define TRACE_READ_ID      0x08000000
#define TRACE_READ_FLASH   0x09000000            

#define TRACE_NFC_PRO(_ID_)  (TRACE_RESET_PU + (_ID_*0x1000000))

#define TRACE_FAIL         0x6000
#define TRACE_SUCCESS      0x8000
#define TRACE_EXE_BOOTLOADER 0x9000


#else
#define DBG_TRACE(_x_)
#endif


#endif

