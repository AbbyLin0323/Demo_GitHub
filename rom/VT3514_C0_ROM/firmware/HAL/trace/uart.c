/*#include "BaseDef.h"
#include "Proj_Config.h"
#include "HAL_MemoryMap.h"
#include "uart.h"*/

#include "COM_Inc.h"
#include <stdarg.h>

#define UART_MP_ENALBED()       (TRUE == l_tMgr.ulMpEn)
#define UART_MP_DISALBED()      (FALSE == l_tMgr.ulMpEn)
#define SET_NEXT_STATE(_state_) (l_tMgr.ulCurrState = _state_)

#define RX_UNKNOWN()    (l_tRxPack.ucOpcode >= UART_VENDER_CMD_OPCODE_NUM)

void HAL_UartRxPack(void);
void HAL_UartTxPack(void);
void HAL_UartProcClearTxBuf(void);
void HAL_UartPreProcPack(void);
void HAL_UartProcRead(void);
void HAL_UartProcWrite(void);
void HAL_UartProcJump(void);
void HAL_UartProcRecycle(void);
void HAL_UartProcFinish(void);
void HAL_UartProcError(void);
void HAL_UartProcSig(void);
void HAL_UartProcDMAWrOp(void);
void HAL_UartProcDMAWrData(void);

LOCAL PFUNC const l_aUartFunc[] =
{
    HAL_UartRxPack,
    HAL_UartTxPack,
    HAL_UartProcClearTxBuf,
    HAL_UartPreProcPack,
    HAL_UartProcRead,
    HAL_UartProcWrite,
    HAL_UartProcJump,
    HAL_UartProcRecycle,
    HAL_UartProcFinish,
    HAL_UartProcError,
    HAL_UartProcSig,
    HAL_UartProcDMAWrOp,
    HAL_UartProcDMAWrData
};

LOCAL UART_STATE const l_aUartPreProcCmdState[UART_VENDER_CMD_OPCODE_NUM] = 
{        
    UART_STATE_PROC_READ,
    UART_STATE_PROC_WRITE,
    UART_STATE_PROC_JUMP,
    UART_STATE_TX,
    UART_STATE_CLRTXBUF,
    UART_STATE_PROC_DMA_WR_OP,
    UART_STATE_PROC_DMA_WR_DATA
};

LOCAL U8 l_ucSig;
LOCAL U8 l_ucSig0;

LOCAL UART_MP_MGR l_tMgr;
LOCAL RX_PACKET l_tRxPack;
LOCAL TX_PACKET l_tTxPack;
LOCAL volatile UART_LCR_REG * const l_pUartLcrReg = (volatile UART_LCR_REG *)&rUART_LCR;


void HAL_UartByteMode(BOOL bEnable)
{
    l_pUartLcrReg->READ_BYTE_MODE = bEnable;
    l_pUartLcrReg->READ_TRIG_MODE = bEnable;
}

void HAL_UartEnable(void)
{
    rUART_ENABLE |= 1 << 12; 
}

void HAL_UartSetBautRate(U32 ulMode)
{
    rUART_LCR = 0;
#ifdef FPGA    
    rUART_LCR = 0xCF0000;    //FPGA 25MHz ,115200
#else
    /// rUART_LCR[31:16] = (MCU HCLK FREQ)/(BautRate)   (266<<20)/(115200)
    if (UART_BAUTRATE_1152000 == ulMode)
    {
        rUART_LCR = 0x1d0000;
    }
    else
    {
        rUART_LCR = 0x1200000;
    }       
#endif 
    HAL_UartByteMode(TRUE);
}

U32 uart_init(void)
{
    U32 ulBautRate;
    HAL_MemZero((U32*)&l_tTxPack,UART_PACKET_SIZE);
    HAL_MemZero((U32*)&l_tRxPack,UART_PACKET_SIZE);
    HAL_MemZero((U32*)&l_tMgr,sizeof(UART_MP_MGR)/sizeof(U32));
    HAL_UartSetBautRate(UART_BAUTRATE_115200);
    HAL_UartEnable();
    return 0;
}

U32 uart_send_str(const U8 *str)
{
    U32 i = 0;

    while (str[i] != 0) {
        rUART_TXR = str[i++];
    }

    return i;
}

U8 uart_getchar(void)
{
    U8 c;
    while((rUART_LCR & 0x3e000000) == 0);

    c = rUART_RXR;

    return c;
}

void uart_putchar(U8 ch)
{
    rUART_TXR = ch;
}

static char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    if (value < 0)
    {
        *ptr++ = '-';

        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    *ptr = 0;

    return string;
}

static const char mc[]="0123456789abcdef";

static char *utohex(unsigned int value, char *string, int radix)
{
    int i,p;
    int oupt;
    oupt = 0;
    for (i=0;i<32;i+=4)
    {
        p = (value >> (28 - i))&0xf;
        string[oupt++]=mc[p];
    }

    string[oupt]=NULL;

    return string;
}

static void dbg_putchar(char c)
{
    //    uart_putchar(c);
    rUART_TXR = c;
}


void dbg_printf(const char *fmt, ...)
{
    if(TRUE == HAL_StrapUartEn())
    {
        const char *s;
        int d;
        char buf[16];
        va_list ap;
        va_start(ap, fmt);
        while (*fmt) {
            if (*fmt != '%') {
                dbg_putchar(*fmt++);
                continue;
            }
            switch (*++fmt) {
                case 's':
                    s = va_arg(ap, const char *);
                    for ( ; *s; s++) {
                        //    dbg_putchar(*s);    
                        rUART_TXR = *s;
                    }
                    break;
                case 'd':
                    d = va_arg(ap, int);
                    itoa(d, buf, 10);
                    for (s = buf; *s; s++) {
                        //    dbg_putchar(*s);
                        rUART_TXR = *s;
                    }
                    break;
                case 'x':
                    d = va_arg(ap, int);
                    utohex(d, buf, 16);
                    for (s = buf; *s; s++) {
                        rUART_TXR = *s;
                        //    dbg_putchar(*s);
                    }
                    break;

                default:
                    rUART_TXR = *fmt;
                    //dbg_putchar(*fmt);
                    break;
            }
            fmt++;
        }
        va_end(ap);
    }  
    else  // if uart disabled 
    {
        // return 
    }
}

/*************************************************************************
Function: HAL_UartCalcCRC
Input: void
Output: void

Description:
    Calc the CRC of Tx Buffer data.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

U16 HAL_UartCalcCRC(void)
{    
    U16 i,sum;
    for (i=0,sum=0;i<((UART_PACKET_SIZE<<1)-1);i++)
    {
        sum ^= l_tTxPack.usWord[i];
    }    
    return sum; 
}
/*************************************************************************
Function: HAL_UartCheckCRC
Input: void
Output: void

Description:
    Check the parity of Rx Buffer data.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

BOOL HAL_UartCheckCRC(void)
{
    U16 i,sum;

    // Calculate by WORD , double number of DWORD

    for (i=0,sum=0;i<(UART_PACKET_SIZE<<1);i++)
    {
        sum ^= l_tRxPack.usWord[i];
    }

    return (0 == sum);
}

/*************************************************************************
Function: HAL_UartProcError
Input: void
Output: void

Description:
    When CRC error be catched,
    PROC ERROR:
        Update Tx status to UART FAIL -> RECYCLE
            
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcError(void)
{
    l_tTxPack.ucStatus = UART_FAIL;
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: HAL_UartProcClearTxBuf
Input: void
Output: void

Description:
    Host issue clear tx buffer command to clear TxPack ->RECYCLE
    
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcClearTxBuf(void)
{
    HAL_MemZero((U32*)&l_tTxPack,UART_PACKET_SIZE);
    l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_CLRTXBUF;    
    SET_NEXT_STATE(UART_STATE_TX);
}

/*************************************************************************
Function: HAL_UartRxPack
Input: void
Output: void

Description:
    Receive data through UART , 
    UART MP DISABLED 
        Scan '3514' to enable the UART MP ,then send back '3514'. -> RECYCLE
    UART MP ENABLED
        Receive the package,the size is 4 DWORD, through UART by DWORD.After the
        whole package has been received.
        CRC CHECK FAIL -> PROC ERROR
        CRC CHECK PASS -> PRE PROC PACK    
    
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartRxPack(void)
{

    if(UART_PACKET_SIZE == l_tMgr.ulRevDataLen)
    {
        if(HAL_UartCheckCRC()||UART_MP_DISALBED())
        {
            DBG_Printf("RX:PRE PROC\n");
            SET_NEXT_STATE(UART_STATE_PREPROC);
        }
        else
        {
            DBG_Printf("RX:ERROR\n");
            SET_NEXT_STATE(UART_STATE_PROC_ERROR);
        }
        return;
    }
    
    if(UART_RXDATA_VALID())
    {
        DBG_Printf("RX:RX\n");
        if (UART_MP_DISALBED())
        {
            l_tRxPack.ulDWord[0] = rUART_RXR;
            SET_NEXT_STATE(UART_STATE_PROC_SIG);
        }
        else
        {
            l_tRxPack.ulDWord[l_tMgr.ulRevDataLen++] = rUART_RXR_DW;
        }
        CLEAR_UART_RXDATA_VALID();                
    }
    else
    {
        SET_NEXT_STATE(UART_STATE_FINISH);
    }

}

/*************************************************************************
Function: HAL_UartTxPack
Input: void
Output: void

Description:
    Host issue Tx command 
    TX:
        Send the Tx Package through UART
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
void HAL_UartTxPack(void)
{
    U32 i;

    DBG_Printf("Enter TX\n");
    l_tTxPack.usCRC = HAL_UartCalcCRC();
    for (i=0;i<UART_PACKET_SIZE;i++)
    {
        rUART_TXR_DW = l_tTxPack.ulDWord[i];
    }

    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: HAL_UartProcSig
Input: void
Output: void

Description: 
    PROC SIG:
        Scan '3514' 
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcSig(void)
{
    U8 ucRxData; 
    ucRxData = (U8)l_tRxPack.ulDWord[0];

    switch(ucRxData)
    {
        case '1':{
            if('5'==l_ucSig)
            {
                l_ucSig = '1';
                DBG_Printf("%x",ucRxData);
            }
            else
            {
                l_ucSig = '#';
            }
        }break;

        case '3':{
            l_ucSig= '3';
            DBG_Printf("%x",ucRxData);
        }break;
        
        case '4':{
            if('1'==l_ucSig)
            {
                l_ucSig = '4';
                l_ucSig0 = 0;
                DBG_Printf("%x",ucRxData);

            }
            else
            {
                l_ucSig = '#';
            }
        }break;

        case '5':{
            if('3'==l_ucSig)
            {
                l_ucSig = '5'; 
                DBG_Printf("%x",ucRxData);
            }
            else
            {
                l_ucSig = '#';
            }
        }break;

        case '0':
            if('4' == l_ucSig)
            {
                l_ucSig = '0';
                l_ucSig0 = 1;
                DBG_Printf("%x",ucRxData);

            }
            else if ('0' == l_ucSig)
            {
                DBG_Printf("%x",ucRxData);
                if (12 == ++l_ucSig0)
                {
                    HAL_UartByteMode(FALSE);
                    l_tMgr.ulMpEn = TRUE;
                    rUART_TXR_DW = UART_SIGNATURE;         
                    rUART_TXR_DW = 0;
                    rUART_TXR_DW = 0;
                    rUART_TXR_DW = 0;
                }
            }
            else
            {
                l_ucSig = '#';
            }
            break;
        default:{
           l_ucSig = '#';
        }
    }
    
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);

}

/*************************************************************************
Function: HAL_UartPreProcPack
Input: void
Output: void

Description:
    PRE PROC PACK:
        
        
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartPreProcPack(void)
{
    DBG_Printf("Enter Pre Proc\n");
     
    if(RX_UNKNOWN())
    {
        DBG_Printf("Pre Proc : UNKNOW\n");
        SET_NEXT_STATE(UART_STATE_PROC_ERROR);            
    }
    else
    {
        SET_NEXT_STATE(l_aUartPreProcCmdState[l_tRxPack.ucOpcode]);
    }
}

/*************************************************************************
Function: HAL_UartProcJump
Input: void
Output: void

Description:
    Process function of Jump State ,jump to target address 
    as request of vender jump command ,
    Then switch next state into Transmit State when pfunc return.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcJump(void)
{
    DBG_Printf("Enter JUMP\n");
    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_JUMP;
        ((PFUNC)l_tRxPack.tData.ulAddr)();
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: HAL_UartProcRead
Input: void
Output: void

Description:
    Process function of Read State ,read the data from source address 
    as request of vender read command to Tx Packet buffer,then update 
    return status as success.
    Then switch next state into Transmit State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
void HAL_UartProcRead(void)
{
    U32 i;
    U32 *pSrc = (U32*)l_tRxPack.tData.ulAddr;
    DBG_Printf("Enter READ\n");
    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        for (i=0;i<l_tRxPack.ucCNT;i++)
        {
            l_tTxPack.tData.ulData[i] = *pSrc++;
        }
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_READ;
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: HAL_UartProcWrite
Input: void
Output: void

Description:
    Process function of Write State ,write the data from Rx Packet buffer to
    dest address as request of vender write command,then update return status 
    as success.
    Then switch next state into Transmit State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
void HAL_UartProcWrite(void)
{
    U32 i;
    U32 *pDst = (U32*)l_tRxPack.tData.ulAddr;
    DBG_Printf("Enter WRITE\n");

    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        for (i=0;i<l_tRxPack.ucCNT;i++)
        {
            *pDst++ = l_tRxPack.tData.ulData[i];
        }      
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_WRITE;
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE); 
}

/*************************************************************************
Function: HAL_UartProcFinish
Input: void
Output: void

Description:
    Process function of Recycle State ,clear the Rx Packet buffer then
    switch next state into Finish State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcRecycle(void)
{
    DBG_Printf("Enter RECYCLE\n");
    l_tMgr.ulRevDataLen = 0;
    HAL_MemZero((U32*)&l_tRxPack,UART_PACKET_SIZE);
    SET_NEXT_STATE(UART_STATE_FINISH);
}

/*************************************************************************
Function: HAL_UartProcFinish
Input: void
Output: void

Description:
    Process function of Finish State ,switch next state into Receive State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

void HAL_UartProcFinish(void)
{
    SET_NEXT_STATE(UART_STATE_RX);
}

void HAL_UartProcDMAWrOp(void)
{
    TX_PACKET tTxPack;
    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        l_tMgr.ulDmaWrAddr = l_tRxPack.tData.ulAddr;
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_DMA_WR_OP;
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

void HAL_UartProcDMAWrData(void)
{
    U32 i;
    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        for (i=0;i<l_tRxPack.ucCNT;i++,l_tMgr.ulDmaWrAddr+=4)
        {
            //DBG_Printf("ADDR:%x , DATA:%x\n",l_tMgr.ulDmaWrAddr,l_tRxPack.ulDWord[i]);
            *((volatile U32*)(l_tMgr.ulDmaWrAddr)) = l_tRxPack.ulDWord[i];
        }      
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_DMA_WR_DATA;
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}


/*************************************************************************
Function: HAL_UartDBG
Input: void
Output: void

Description:
    Interface for mass product or debug through UART, this function could be 
    embedded into SATA/PCIE MP schedule loop.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
void HAL_UartDBG(void)
{   
    while(1)
    {
        l_aUartFunc[l_tMgr.ulCurrState]();
        if(UART_MP_DISALBED())
        {
            return;
        }             
    }
}


