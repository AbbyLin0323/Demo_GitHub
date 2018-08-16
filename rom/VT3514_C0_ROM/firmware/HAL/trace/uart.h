#ifndef OPENRISC_UART_H
#define OPENRISC_UART_H

#define rUART_ENABLE (*((volatile  U32*)(REG_BASE_GLB + 0x68)))

#define UART_BASE REG_BASE_UART


#define rUART_TXR (*((volatile U8*) (UART_BASE + 0x4)))


#define rUART_RXR (*((volatile U8*) (UART_BASE + 0x8)))

//UART REG04    TX Buffer depth 16 byte
//31:0    RW    0    Write data to be sent to this register    UART_TXDATA
#define rUART_TXR_DW (*((volatile U32*) (UART_BASE + 0x4)))


//UART REG08    RX Buffer depth 16 byte    
//31:0    RW    0    Read received data from this register    UART_RXDATA
#define rUART_RXR_DW (*((volatile U32*) (UART_BASE + 0x8)))

/* UART REG0C
31:16    RW    0   Configure the baud rate:                                UART_RATE_MOD      
                When the UART baud rate and internal CLK is defined, 
                count how many clocks will one UART bit lasts
15:14    RW    0   Set the frame parity mode:                              UART_PAR_SEL
                2¡¯b00: no parity bit in the frame
                2¡¯b01: odd parity bit in the frame
                2¡¯b10: even parity bit in the frame
13        RW    0   Set the stop bit number :                               UART_FRAM_MOD
                1¡¯b0: one stop bit in the frame
                1¡¯b1: two stop bit in the frame       
12        RW    0   Set UART interface mode:                                UART_MOD
                1¡¯b0: without two handshake signals 
                1¡¯b1: with two handshake signals as RTS_n and CTS_n
11        RW    0   Set bit order:                                          MSBMODE                                                                                      
                1¡¯b0: MSB last                                                                                      
                1¡¯b1: MSB first                                                                              
10        RW    0   Set read mode:                                          READ_BYTE_MODE                                                                                  
                1¡¯b0: read 1 time, pop 4 bytes                                                                      
                1¡¯b1: read 1 time, pop 1 byte                                                           
9        RW    0   Set FIFO read trigger mode                              READ_TRIG_MODE                                                       
                1¡¯b0: read can occurred only when there are 
                at least 4 bytes data in the RXFIFO                     
                1¡¯b1: read can occurred as long as RXFIFO is not empty                                   
8        RW    0   Set working mode:                                       UART_SPI_SEL                                                                              
                1¡¯b0: UART work                                                                                     
                1¡¯b1: SPI work                                                                             
7        RW    0   Error interrupt mask:                                   ERR_MASK                                                                             
                1¡¯b0: disable                                                                                       
                1¡¯b1: enable                                                                                 
6        RW    0   Read interrupt mask                                     READ_MASK                                                                              
                1¡¯b0: disable                                                                                       
                1¡¯b1: enable                                                                                
5:1        RW    0   Show current RXFIFO depth                                RX_FIFO_CNT                                                              
0        RW    0   Set the behavior when RXFIFO overflow                   RX_OVERFLOW_C                                                          
                1¡¯b0: don¡¯t receive                                                                                
                1¡¯b1: clear first byte in FIFO                                               
*/ 

#define rUART_LCR (*((volatile  U32*) (UART_BASE + 0xc)))
typedef struct UART_LCR_REG
{    
    U32 RX_OVERFLOW_C  :1;   
    U32 RX_FIFO_CNT    :5;   
    U32 READ_MASK      :1;   
    U32 ERR_MASK       :1;     
    U32 UART_SPI_SEL   :1;   
    U32 READ_TRIG_MODE :1;   
    U32 READ_BYTE_MODE :1;   
    U32 MSBMODE        :1;   
    U32 UART_MOD       :1;     
    U32 UART_FRAM_MOD  :1;   
    U32 UART_PAR_SEL   :2;     
    U32 UART_RATE_MOD  :16;   
}UART_LCR_REG;
/*  UART REG10
31        RW1C    0   1¡¯b0: parity ok                                     UART_PAR_ERR
                    1¡¯b1: when a parity error is 
                    detected by serial receive block
30        RW1C    0   1¡¯b0: frame ok                                      UART_FRA_ERR
                    1¡¯b1: when a wrong stop bit is 
                    detected by serial receive block 
29        RW1C    0   1¡¯b0: external device response normally             UART_TIME_OUT
                    1¡¯b1: when APB master transmit, 
                    the external device doesn¡¯t response for a long time
28        RW1C    0   1¡¯b0: RXFIFO normally                               RXFIFO_FULL_ERR
                    1¡¯b1: RXFIFO overflow
27        RW1C    0   1¡¯b0: there is not enough data in memory            READ_MEM_FULL
                    1¡¯b1: APB master can read now 
26:16    RO        0   reserved
15:0    RW        0   reserved
*/
#define rUART_ICR (*((volatile  U32*) (UART_BASE + 0x10)))
#define BIT_UART_READ_MEM_FULL (1 << 27)
#define UART_RXDATA_VALID()         (rUART_ICR & BIT_UART_READ_MEM_FULL)
#define CLEAR_UART_RXDATA_VALID()   (rUART_ICR = BIT_UART_READ_MEM_FULL)   

#define UART_SIGNATURE       (0x34313533) // '4''1''5''3'

#define UART_PACKET_SIZE    (4)
#define UART_RD_DATA_LEN    (3)
#define UART_WR_DATA_LEN    (2)

typedef struct RX_DATA
{
    U32 ulAddr;
    union{
        U32 ulData[UART_WR_DATA_LEN];
        U16 usData[UART_WR_DATA_LEN<<1];
        U8  ucData[UART_WR_DATA_LEN<<2];
    };
}RX_DATA;

typedef union RXPACKET{
    struct {
        RX_DATA tData;
        U8 ucOpcode;
        U8 ucCNT;
        U16 usCRC;
    };
    U32 ulDWord[UART_PACKET_SIZE];
    U16 usWord[UART_PACKET_SIZE<<1];
}RX_PACKET;

typedef union TX_DATA_{
    U32 ulData[UART_RD_DATA_LEN];
    struct {
        U8 ucErrCmdType;
        U8 ucErrFlg;
        U16 res;
        U32 ulErrTargetAddr;    
    };    
}TX_DATA;

typedef union TX_PACKET{
    struct{        
        TX_DATA tData;
        U8 ucOpcode;
        U8 ucStatus;
        U16 usCRC;
    };
    U32 ulDWord[UART_PACKET_SIZE];
    U16 usWord[UART_PACKET_SIZE<<1];
}TX_PACKET;

typedef union UART_SIGNATURE_BUF{
    U8  ucSig;
    U32 ulSig;
}UART_SIGNATURE_BUF;

enum{
    UART_VENDER_CMD_OPCODE_READ,
    UART_VENDER_CMD_OPCODE_WRITE,
    UART_VENDER_CMD_OPCODE_JUMP,
    UART_VENDER_CMD_OPCODE_TX,
    UART_VENDER_CMD_OPCODE_CLRTXBUF,
    UART_VENDER_CMD_OPCODE_DMA_WR_OP,
    UART_VENDER_CMD_OPCODE_DMA_WR_DATA,
    UART_VENDER_CMD_OPCODE_NUM
};

enum{
    UART_SUCC,
    UART_FAIL    
};

enum{
    UART_BAUTRATE_115200,
    UART_BAUTRATE_1152000,
};

typedef enum UART_STATE{
    UART_STATE_RX,
    UART_STATE_TX,
    UART_STATE_CLRTXBUF,
    UART_STATE_PREPROC,
    UART_STATE_PROC_READ,
    UART_STATE_PROC_WRITE,
    UART_STATE_PROC_JUMP,
    UART_STATE_PROC_RECYCLE,
    UART_STATE_FINISH,
    UART_STATE_PROC_ERROR,
    UART_STATE_PROC_SIG,
    UART_STATE_PROC_DMA_WR_OP,
    UART_STATE_PROC_DMA_WR_DATA
}UART_STATE;

typedef struct UART_MP_MGR{
    BOOL ulMpEn;
    UART_STATE ulCurrState;
    U32 ulRevDataLen;
    U32 ulDmaWrAddr;
}UART_MP_MGR;

U32   uart_init(void);
U32   uart_send_str(const U8 *str);
U8    uart_getchar(void);
void  uart_putchar(U8 ch);
U32   uart_download(U8 *downloadaddr);
void dbg_printf(const char *fmt, ...);

#endif

