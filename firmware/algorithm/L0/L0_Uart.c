#include "uart.h"
#include  <stdarg.h>
#include "BaseDef.h"
#include "Disk_Config.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_ParamTable.h"
#include "L0_Uart.h"


extern U32 g_ulATARawBuffStart;
extern GLOBAL BOOL g_ulUARTCmdPending;

#define UART_MP_ENALBED()       (TRUE == l_tMgr.ulMpEn)
#define UART_MP_DISALBED()      (FALSE == l_tMgr.ulMpEn)
#define SET_NEXT_STATE(_state_) (l_tMgr.ulCurrState = _state_)

#define RX_UNKNOWN()    (l_tRxPack.ucOpcode >= UART_VENDER_CMD_OPCODE_NUM)
//#define UART_DEBUG 

#define UARTQ_DEPTH 64
typedef struct UartQ
{
    U8 Data[UARTQ_DEPTH];
    U8 Rp;
    U8 Wp;
}UARTQ;

UARTQ g_UartQ;

typedef union {
    U8 ucByte[4];
    U32 ulVal;
} RX_VAL;

LOCAL void L0_UartRxPack(void);
LOCAL void L0_UartTxPack(void);
LOCAL void L0_UartProcClearTxBuf(void);
LOCAL void L0_UartPreProcPack(void);
LOCAL void L0_UartProcRead(void);
LOCAL void L0_UartProcWrite(void);
LOCAL void L0_UartProcJump(void);
LOCAL void L0_UartProcRecycle(void);
LOCAL void L0_UartProcFinish(void);
LOCAL void L0_UartProcError(void);
LOCAL void L0_UartProcSig(void);
LOCAL void L0_UartProcDMAWrOp(void);
LOCAL void L0_UartProcDMAWrData(void);
LOCAL void L0_UartProcViaCmd(void);
LOCAL void L0_UartProcDMARd(void);
LOCAL void L0_UartProcGetVDBuf(void);
LOCAL void L0_UartProcSetVDBuf(void);

typedef void(*PFUNC)(void);

LOCAL PFUNC l_aUartFunc[] =
{
    L0_UartRxPack,
    L0_UartTxPack,
    L0_UartProcClearTxBuf,
    L0_UartPreProcPack,
    L0_UartProcRead,
    L0_UartProcWrite,
    L0_UartProcJump,
    L0_UartProcRecycle,
    L0_UartProcFinish,
    L0_UartProcError,
    L0_UartProcSig,
    L0_UartProcDMAWrOp,
    L0_UartProcDMAWrData,
    L0_UartProcViaCmd,
    L0_UartProcDMARd
};

LOCAL UART_STATE l_aUartPreProcCmdState[UART_VENDER_CMD_OPCODE_NUM] = 
{        
    UART_STATE_PROC_READ,
    UART_STATE_PROC_WRITE,
    UART_STATE_PROC_JUMP,
    UART_STATE_TX,
    UART_STATE_CLRTXBUF,
    UART_STATE_PROC_DMA_WR_OP,
    UART_STATE_PROC_DMA_WR_DATA,
    UART_STATE_PROC_DMA_RD
};

LOCAL U8 l_ucSig;
LOCAL U8 l_ucSig0;

LOCAL UART_MP_MGR l_tMgr;
LOCAL RX_PACKET l_tRxPack;
LOCAL TX_PACKET l_tTxPack;

LOCAL volatile UART_LCR_REG *  l_pUartLcrReg ;

/*************************************************************************
Function: L0_UartByteMode
Input: BOOL bEnable    TRUE for Byte mode ,FALSE for DWORD mode 
Output: void

Description:
     After entering into the byte mode , the UART pulls up the rLCR[27] when receives each byte,
     and FW read out a byte from RXR
History:
    2015/5/4   Victor Zhang  First create

*************************************************************************/

MCU0_DRAM_TEXT void L0_UartByteMode(BOOL bEnable)
{
    l_pUartLcrReg->READ_BYTE_MODE = bEnable;
    l_pUartLcrReg->READ_TRIG_MODE = bEnable;
}

/*************************************************************************
Function: L0_UartMpInit
Input: void
Output: void

Description:
     Initiating UART MP struct and set MP flag if system select uart mp boot ,
History:
    2015/5/4   Victor Zhang  First create

*************************************************************************/

GLOBAL MCU0_DRAM_TEXT void L0_UartMpInit(void)
{
    l_pUartLcrReg = (volatile UART_LCR_REG *)&rUART_LCR;
    COM_MemZero((U32*)&l_tTxPack,UART_PACKET_SIZE);
    COM_MemZero((U32*)&l_tRxPack,UART_PACKET_SIZE);
    COM_MemZero((U32*)&l_tMgr,sizeof(UART_MP_MGR)/sizeof(U32));
    L0_UartByteMode(TRUE);

    return;
}

/*************************************************************************
Function: L0_UartReadMask
Input: bEnable 
Output: void

Description:
     Enable UART MP read interrupt mask. 
     TRUE: Enable read mask(disable read interrupt)
     FALSE: Disable read mask(enable read interrupt)
History:
    2016/12/12   Eason Chien  First create
*************************************************************************/
GLOBAL MCU0_DRAM_TEXT void L0_UartReadMask(BOOL bEnable)
{
    l_pUartLcrReg = (volatile UART_LCR_REG *)&rUART_LCR;
    l_pUartLcrReg->READ_MASK = bEnable;

#ifdef UART_DEBUG
    DBG_Printf("l_pUartLcrReg->READ_MASK = %d\n", l_pUartLcrReg->READ_MASK);
#endif
	return;
}

/*************************************************************************
Function: L0_UartEnReadINT
Input: BOOL
Output: void

Description:
     Enable UART MP error interrupt mask. 
     TRUE: Enable error mask(disable error interrupt)
     FALSE: Disable error mask(enable error interrupt)
History:
    2016/12/12   Eason Chien  First create
*************************************************************************/
GLOBAL MCU0_DRAM_TEXT void L0_UartErrMask(BOOL bEnable)
{
    l_pUartLcrReg = (volatile UART_LCR_REG *)&rUART_LCR;
    l_pUartLcrReg->ERR_MASK = bEnable;

#ifdef UART_DEBUG
		DBG_Printf("l_pUartLcrReg->ERR_MASK = %d\n", l_pUartLcrReg->ERR_MASK);
#endif
    return;
}

/*************************************************************************
Function: L0_UartCalcCRC
Input: void
Output: void

Description:
    Calc the CRC of Tx Buffer data.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT U16 L0_UartCalcCRC(void)
{    
    U16 i,sum;
    
    for (i=0,sum=0;i<((UART_PACKET_SIZE<<1)-1);i++)
    {
        sum ^= l_tTxPack.usWord[i];
    } 

#ifdef UART_DEBUG
    DBG_Printf("CalcCRC = 0x%x\n", sum);
#endif
    return sum; 
}
/*************************************************************************
Function: L0_UartCheckCRC
Input: void
Output: void

Description:
    Check the parity of Rx Buffer data.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
LOCAL MCU0_DRAM_TEXT BOOL L0_UartCheckCRC(void)
{
    U16 i,sum;

    // Calculate by WORD , double number of DWORD

    for (i=0,sum=0;i<(UART_PACKET_SIZE<<1);i++)
    {
        sum ^= l_tRxPack.usWord[i];
    }

#ifdef UART_DEBUG
    DBG_Printf("CheckCRC = 0x%x, It must be 0!!\n", sum);
#endif
    return (0 == sum);
}

/*************************************************************************
Function: L0_UartProcError
Input: void
Output: void

Description:
    When CRC error be catched,
    PROC ERROR:
        Update Tx status to UART FAIL -> RECYCLE
            
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
LOCAL MCU0_DRAM_TEXT void L0_UartProcError(void)
{
#ifdef UART_DEBUG
    DBG_Printf("Enter ProcError\n");
#endif
    l_tTxPack.ucStatus = UART_FAIL;
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: L0_UartProcClearTxBuf
Input: void
Output: void

Description:
    Host issue clear tx buffer command to clear TxPack ->RECYCLE
    
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
LOCAL MCU0_DRAM_TEXT void L0_UartProcClearTxBuf(void)
{

#ifdef UART_DEBUG
    DBG_Printf("Enter ClearTxBuf\n");
#endif
    COM_MemZero((U32*)&l_tTxPack,UART_PACKET_SIZE);
    l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_CLRTXBUF;    
    SET_NEXT_STATE(UART_STATE_TX);
}

/*************************************************************************
Function: L0_UartRxPack
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
#if 1
LOCAL U8 HAL_UARTQRxByte()
{
    U8 Buf;
    Buf = g_UartQ.Data[g_UartQ.Rp];
    g_UartQ.Rp = (g_UartQ.Rp+1)%UARTQ_DEPTH;
    return Buf;
}

LOCAL U32 HAL_UARTQRxDW()
{
    RX_VAL ulBuf;

    ulBuf.ucByte[0] = g_UartQ.Data[g_UartQ.Rp];
    ulBuf.ucByte[1] = g_UartQ.Data[g_UartQ.Rp+1];
    ulBuf.ucByte[2] = g_UartQ.Data[g_UartQ.Rp+2];
    ulBuf.ucByte[3] = g_UartQ.Data[g_UartQ.Rp+3];
    g_UartQ.Rp = (g_UartQ.Rp+4)%UARTQ_DEPTH;
    return ulBuf.ulVal;
}

LOCAL BOOL HAL_CheckUARTQValid()
{
    if (g_UartQ.Rp == g_UartQ.Wp)
        return FALSE;
    else
        return TRUE;
}

LOCAL void L0_UartRxPack(void)
{
    U16 i;
    if(UART_PACKET_SIZE == l_tMgr.ulRevDataLen)
    {
#ifdef UART_DEBUG
        // for debug, print all bytes
        if(UART_PACKET_SIZE == l_tMgr.ulRevDataLen) 
        {
            for (i = 0; i < (UART_PACKET_SIZE<<1); i++)
                DBG_Printf("0x%x\n", l_tRxPack.usWord[i]);
        }
#endif

        if(L0_UartCheckCRC()||UART_MP_DISALBED())
        {
#ifdef UART_DEBUG
            DBG_Printf("RX:PRE PROC\n");
#endif
            SET_NEXT_STATE(UART_STATE_PREPROC);
        }
        else
        {
#ifdef UART_DEBUG
            DBG_Printf("RX:ERROR\n");
#endif
            SET_NEXT_STATE(UART_STATE_PROC_ERROR);
        }
        return;
    }

    if(TRUE == HAL_CheckUARTQValid())
    {
        if (UART_MP_DISALBED())
        {
            l_tRxPack.ulDWord[0] = HAL_UARTQRxByte();
            SET_NEXT_STATE(UART_STATE_PROC_SIG);
#ifdef UART_DEBUG
            DBG_Printf("RX: 1B\n");
			DBG_Printf("RX: ulDWord[0] = 0x%x, Ctrl(0xc) = 0x%x\n", l_tRxPack.ulDWord[0], rUART_LCR);
#endif
        }
        else
        {
            l_tRxPack.ulDWord[l_tMgr.ulRevDataLen++] = HAL_UARTQRxDW();
#ifdef UART_DEBUG
            DBG_Printf("RX: 4B\n");
            DBG_Printf("RX: l_tRxPack.ulDWord[%d] = 0x%x, Ctrl(0xc) = 0x%x\n",
                    l_tMgr.ulRevDataLen-1, l_tRxPack.ulDWord[l_tMgr.ulRevDataLen-1], rUART_LCR);
#endif
        }
    }
    else
    {
        SET_NEXT_STATE(UART_STATE_FINISH);
    }
}
#else
LOCAL MCU0_DRAM_TEXT void L0_UartRxPack(void)
{
    U16 i;
    if(UART_PACKET_SIZE == l_tMgr.ulRevDataLen)
    {

#ifdef UART_DEBUG
        // for debug, print all bytes
        if(UART_PACKET_SIZE == l_tMgr.ulRevDataLen) {
            for (i = 0; i < (UART_PACKET_SIZE<<1); i++)
                DBG_Printf("0x%x\n", l_tRxPack.usWord[i]);
        }
#endif

        if(L0_UartCheckCRC()||UART_MP_DISALBED())
        {
#ifdef UART_DEBUG
            DBG_Printf("RX:PRE PROC\n");
#endif
            SET_NEXT_STATE(UART_STATE_PREPROC);
        }
        else
        {
#ifdef UART_DEBUG
            DBG_Printf("RX:ERROR\n");
#endif
            SET_NEXT_STATE(UART_STATE_PROC_ERROR);
        }
        return;
    }

    if(TRUE == HAL_CheckRxValid())
    {
        if (UART_MP_DISALBED())
        {
            l_tRxPack.ulDWord[0] = HAL_UartRxByte();
            SET_NEXT_STATE(UART_STATE_PROC_SIG);
#ifdef UART_DEBUG
            DBG_Printf("RX: 1B\n");
			DBG_Printf("RX: ulDWord[0] = 0x%x, Ctrl(0xc) = 0x%x\n", l_tRxPack.ulDWord[0], rUART_LCR);
#endif
        }
        else
        {
            l_tRxPack.ulDWord[l_tMgr.ulRevDataLen++] = HAL_UartRxDW();
#ifdef UART_DEBUG
            DBG_Printf("RX: 4B\n");
            DBG_Printf("RX: l_tRxPack.ulDWord[%d] = 0x%x, Ctrl(0xc) = 0x%x\n",
                    l_tMgr.ulRevDataLen-1, l_tRxPack.ulDWord[l_tMgr.ulRevDataLen-1], rUART_LCR);
#endif
        }
        HAL_ClearRxValid();
    }
    else
    {
        SET_NEXT_STATE(UART_STATE_FINISH);
    }
}
#endif


/*************************************************************************
Function: L0_UartTxPack
Input: void
Output: void

Description:
    Host issue Tx command 
    TX:
        Send the Tx Package through UART
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
LOCAL MCU0_DRAM_TEXT void L0_UartTxPack(void)
{
    U32 i;

#ifdef UART_DEBUG
    DBG_Printf("Enter TX\n");
#endif

    l_tTxPack.usCRC = L0_UartCalcCRC();
    for (i=0;i<UART_PACKET_SIZE;i++)
    {
        HAL_UartTxDW(l_tTxPack.ulDWord[i]);
        DBG_Printf("0x%x\n", l_tTxPack.ulDWord[i]);
    }

    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: L0_UartProcSig
Input: void
Output: void

Description: 
    PROC SIG:
        Scan the string of '3514 0000 0000 0000' 
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcSig(void)
{
    U8 ucRxData; 
    ucRxData = (U8)l_tRxPack.ulDWord[0];

    switch(ucRxData)
    {
        case '1':{
            if('5'==l_ucSig)
            {
                l_ucSig = '1';
#ifdef UART_DEBUG
                DBG_Printf("%x\n", ucRxData);
#endif
            }
            else
            {
                l_ucSig = '#';
            }
        }break;

        case '3':{
            l_ucSig= '3';
#ifdef UART_DEBUG
            DBG_Printf("%x\n",ucRxData);
#endif
        }break;
        
        case '4':{
            if('1'==l_ucSig)
            {
                l_ucSig = '4';
                l_ucSig0 = 0;
#ifdef UART_DEBUG
                DBG_Printf("%x\n",ucRxData);
#endif
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
#ifdef UART_DEBUG
                DBG_Printf("%x\n", ucRxData);
#endif
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
#ifdef UART_DEBUG
                DBG_Printf("%x\n", ucRxData);
#endif
            }
            else if ('0' == l_ucSig)
            {
#ifdef UART_DEBUG 
                DBG_Printf("%x\n",ucRxData);
#endif
                if (12 == ++l_ucSig0)
                {
                    l_tMgr.ulMpEn = TRUE;
                    L0_UartByteMode(FALSE);                    
                    HAL_UartMpMode(TRUE);
                    HAL_UartTxDW(UART_SIGNATURE);
                    HAL_UartTxDW(0);
                    HAL_UartTxDW(0);
                    HAL_UartTxDW(0);
#ifdef UART_DEBUG
                    DBG_Printf("Enable MP UART\n");
#endif
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
Function: L0_UartPreProcPack
Input: void
Output: void

Description:
    PRE PROC PACK:
        
        
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartPreProcPack(void)
{
#ifdef UART_DEBUG
    DBG_Printf("Enter Pre Proc\n");
#endif

    if(UART_VENDER_CMD_OPCODE_VIACMD == l_tRxPack.ucOpcode)
    {
#ifdef UART_DEBUG
        DBG_Printf("Pre Pack : VIA\n");
#endif
        /* if need write DSRAM, switch to polling mode */
        if (l_tRxPack.ucCNT == VIA_CMD_MEM_WRITE)
            L0_UartReadMask(TRUE);

        SET_NEXT_STATE(UART_STATE_PROC_VIA_CMD);
    }
    else if(RX_UNKNOWN())
    {
#ifdef UART_DEBUG
        DBG_Printf("Pre Pack : UNKNOW\n");
#endif
        SET_NEXT_STATE(UART_STATE_PROC_ERROR);
    }
    else
    {
        SET_NEXT_STATE(l_aUartPreProcCmdState[l_tRxPack.ucOpcode]);
#ifdef UART_DEBUG
        DBG_Printf("Pre Proc: STATE %d\n",l_aUartPreProcCmdState[l_tRxPack.ucOpcode]);
#endif
    }
}

/*************************************************************************
Function: L0_UartProcJump
Input: void
Output: void

Description:
    Process function of Jump State ,jump to target address 
    as request of vender jump command ,
    Then switch next state into Transmit State when pfunc return.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcJump(void)
{
#ifdef UART_DEBUG
    DBG_Printf("Enter JUMP\n");
#endif

    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_JUMP;
        ((PFUNC)l_tRxPack.tData.ulAddr)();
    }

    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: L0_UartProcRead
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
LOCAL MCU0_DRAM_TEXT void L0_UartProcRead(void)
{
    U32 i;
    U32 *pSrc = (U32*)l_tRxPack.tData.ulAddr;

#ifdef UART_DEBUG
    DBG_Printf("Enter READ\n");
#endif

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
Function: L0_UartProcWrite
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
void L0_UartProcWrite(void)
{
    U32 i;
    U32 *pDst = (U32*)l_tRxPack.tData.ulAddr;

#ifdef UART_DEBUG
    DBG_Printf("Enter WRITE\n");
#endif

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
Function: L0_UartProcFinish
Input: void
Output: void

Description:
    Process function of Recycle State ,clear the Rx Packet buffer then
    switch next state into Finish State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcRecycle(void)
{
    l_tMgr.ulRevDataLen = 0;

    //COM_MemZero((U32*)&l_tRxPack,UART_PACKET_SIZE);
    SET_NEXT_STATE(UART_STATE_FINISH);
}

/*************************************************************************
Function: L0_UartProcFinish
Input: void
Output: void

Description:
    Process function of Finish State ,switch next state into Receive State.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcFinish(void)
{
    g_ulUARTCmdPending = FALSE;

    /*When DMA write finish, switch to intrttupt mode */
    if (l_tRxPack.ucCNT == VIA_CMD_MEM_WRITE)
	    L0_UartReadMask(FALSE);

    SET_NEXT_STATE(UART_STATE_RX);
}
/*************************************************************************
Function: L0_UartProcDMAWrOp
Input: void
Output: void

Description:
    Burst write command , used to transfer mass data.
    FW will record the address that host sent , and accumulate the address after each write data operation. 
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcDMAWrOp(void)
{
    TX_PACKET tTxPack;

#ifdef UART_DEBUG
    DBG_Printf("DMA WR OP\n");
#endif

    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        l_tMgr.ulDmaWrAddr = l_tRxPack.tData.ulAddr;
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_DMA_WR_OP;
    }

    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: L0_UartProcDMAWrData
Input: void
Output: void

Description:
    Follow the DMA Write OP command , transfer 3 DWORD data each packet.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcDMAWrData(void)
{
    U32 i;

#ifdef UART_DEBUG
    DBG_Printf("DMA WR DATA\n");
#endif

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
Function: L0_UartProcDMARd
Input: void
Output: void

Description:
    Host issue DMA read commmand to burst output data from device to host,
    First DWORD is source address
    Second DWORD is data dword length
    After the whole data block transfer is done, FW calculate the checksum,
    and put it into tx packet to wait for the get status (tx) command. 
History:
    2015/5/6   Victor Zhang  First create

*************************************************************************/

LOCAL MCU0_DRAM_TEXT void L0_UartProcDMARd(void)
{
    U32 ulSrcAddr,ulCnt,ulCheckSum,ulData,i;
    ulSrcAddr   = l_tRxPack.tData.ulAddr;
    ulCnt       = l_tRxPack.tData.ulData[0];
    ulCheckSum  = 0;

#ifdef UART_DEBUG
    DBG_Printf("DMA READ\n");
    DBG_Printf("%x,%x,%x,%x\n",
        l_tRxPack.ulDWord[0],
        l_tRxPack.ulDWord[1],
        l_tRxPack.ulDWord[2],
        l_tRxPack.ulDWord[3]);
#endif

    if(UART_SUCC == l_tTxPack.ucStatus)
    {
        for(i=0;i<ulCnt;i++,ulSrcAddr+=4)
        {
            ulData      = *(volatile U32*)(ulSrcAddr);
            ulCheckSum ^= ulData;
            HAL_UartTxDW(ulData);
            DBG_Printf("Addr: 0x%x,Data: 0x%x ,CheckSum: 0x%x\n",\
            ulSrcAddr,ulData,ulCheckSum);
        }
        l_tTxPack.ulDWord[0] = ulCheckSum;
        l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_DMA_RD;
    }
    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
}

/*************************************************************************
Function: L0_UartProcViaCmd
Input: void
Output: void

Description:
    HOST issue VIA CMD for format card
    L0 receives VIA CMDs through UART and may send it to MCU12 through SCMD.
    If host want a data or status back , MP program will put it into Tx Pack 
    then wait for get status (tx) command issued by host.
History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
LOCAL MCU0_DRAM_TEXT void L0_UartProcViaSCMD(void)
{
    UART_VIA_CMD_STATE eState       = UART_VIA_CMD_STATE_IDLE;    
    VIA_CMD_PARAM *pViaSubCmdParam  = (VIA_CMD_PARAM *)&l_tRxPack;    
    U32 ulMemAddr                   = pViaSubCmdParam->tMemAccess.ulDevAddr;
    U32 ulMemCnt                    = pViaSubCmdParam->tMemAccess.bsByteLen>>2;
    U32 ulStatus                    = VCS_SUCCESS;
    U32 aBuf[3]                     = {0};
    U8 ucSubCmdType                 = l_tRxPack.ucCNT;

    while(1)
    {
        switch (eState)
        {
            case UART_VIA_CMD_STATE_IDLE:
            {
                DBG_Printf("VIA SCMD IDLE:");
                if(ucSubCmdType == VIA_CMD_MEM_WRITE)
                {
                    DBG_Printf("RX.\n");
                    eState = UART_VIA_CMD_STATE_RX_DATA;
                }
                else
                {
                    DBG_Printf("SCMD.\n");
                    eState = UART_VIA_CMD_STATE_ISSUE_SCMD;
                }
                break;
            }
            case UART_VIA_CMD_STATE_RX_DATA:
            {
                DBG_Printf("VIA SCMD RX:");
                l_tTxPack.ulDWord[0] = HAL_UartDmaRx((U32*)g_ulATARawBuffStart,ulMemCnt);
                DBG_Printf("SCMD.\n");
                eState = UART_VIA_CMD_STATE_ISSUE_SCMD;
                break;
            }

            case UART_VIA_CMD_STATE_TX_DATA:
            {
                if (VIA_CMD_MEM_WRITE == ucSubCmdType)
                {
                    DBG_Printf("VIA SCMD TX: MEM WRITE.\n");
                    // DO NOTHING
                }
                else if (VIA_CMD_MEM_READ == ucSubCmdType)
                {
                    DBG_Printf("VIA SCMD TX: DMA TX.\n");
                    l_tTxPack.ulDWord[0] = HAL_UartDmaTx((U32*)g_ulATARawBuffStart,ulMemCnt);
                }
                else if ((VIA_CMD_NULL < ucSubCmdType)&&(ucSubCmdType <= VIA_CMD_FLASH_ERASE))
                {
                    DBG_Printf("VIA SCMD TX: DMA TX.\n");
                    l_tTxPack.ulDWord[0] = HAL_UartDmaTx((U32*)g_ulATARawBuffStart,SUB_SYSTEM_PU_MAX);
                }
                else
                {
                    DBG_Printf("VIA SCMD TX: PACK TX.\n");
                    COM_MemCpy((U32 *)&l_tTxPack,aBuf,3);
                    SET_NEXT_STATE(UART_STATE_TX);
                }
                eState = UART_VIA_CMD_STATE_FINISH;
                break;
            }

            case UART_VIA_CMD_STATE_ISSUE_SCMD:
            {
                DBG_Printf("VIA SCMD ISSUE: %d\n", ucSubCmdType);
                ulStatus = L0_ViaHostCmd(0,ucSubCmdType,pViaSubCmdParam, aBuf);

                if(VCS_WAITING_RESOURCE != ulStatus)
                {
                    eState = UART_VIA_CMD_STATE_TX_DATA;
                    if(VCS_SUCCESS != ulStatus)
                    {
                        l_tTxPack.ucStatus = UART_FAIL;
                    }
                }

                break;
            }

            case UART_VIA_CMD_STATE_FINISH:

            default:
                return ;
        }
    }
}


LOCAL MCU0_DRAM_TEXT void L0_UartProcViaCmd(void)
{
    UART_VIA_CMD_STATE eState       = UART_VIA_CMD_STATE_IDLE;
    VIA_CMD_PARAM *pViaSubCmdParam  = (VIA_CMD_PARAM *)&l_tRxPack;
    U32 ulMemAddr                   = pViaSubCmdParam->tMemAccess.ulDevAddr;
    U32 ulMemCnt                    = pViaSubCmdParam->tMemAccess.bsByteLen >> 2;
    U32 ulStatus                    = VCS_SUCCESS;
    U32 aBuf[3]                     = {0};
    U8 ucSubCmdType                 = l_tRxPack.ucCNT;

    DBG_Printf("VIA CMD\n");

    SET_NEXT_STATE(UART_STATE_PROC_RECYCLE);
    if(UART_SUCC != l_tTxPack.ucStatus)
    {
        return;
    }

    l_tTxPack.ucOpcode = UART_VENDER_CMD_OPCODE_VIACMD;

    if(MCU0_ID == l_tRxPack.tViaCmdParam.tMemAccess.bsMcuID)
    {
        DBG_Printf("VIA CMD MCU0.\n");
        if(ucSubCmdType == VIA_CMD_MEM_READ)
        {
            DBG_Printf("VIA CMD : MEM READ.\n");
            l_tTxPack.ulDWord[0] = HAL_UartDmaTx((U32*)ulMemAddr,ulMemCnt);
        }
        else if(ucSubCmdType == VIA_CMD_MEM_WRITE)
        {
            DBG_Printf("VIA CMD : MEM WRITE.\n");
            l_tTxPack.ulDWord[0] = HAL_UartDmaRx((U32*)ulMemAddr,ulMemCnt);
        }
        else
        {
            DBG_Printf("VIA CMD : SEND SCMD.\n");
            L0_UartProcViaSCMD();
        }
    }
    else
    {
        L0_UartProcViaSCMD();
    }
}

/*************************************************************************
Function: L0_UartDBG
Input: void
Output: void

Description:
    Interface for mass product or debug through UART, this function could be 
    embedded into SATA/PCIE MP schedule loop.

History:
    2015/4/16   Victor Zhang  First create

*************************************************************************/
GLOBAL MCU0_DRAM_TEXT void L0_UartDBG(void)
{
#ifndef SIM
    l_aUartFunc[l_tMgr.ulCurrState]();
#endif
}

GLOBAL void ISR_UART(void)
{
    U32 ulRX_DW;
    U8 i;

#ifdef UART_DEBUG
    DBG_Printf("Is\n");
#endif
    /* Check RXFIFO is overflow or not */
    if (TRUE == ((rUART_ICR >> 28)&0x1))
    {
#ifdef UART_DEBUG
        DBG_Printf("Rx fifo overflow: ICR=0x%x\n", rUART_ICR);
#endif
        rUART_ICR = BIT_UART_RXFIFO_FULL_ERR;
    }

    if(TRUE == HAL_CheckRxValid())
    {
        /* When UART ISR trigger, MCU1 and MCU2 can't be halt */
        g_ulUARTCmdPending = TRUE;

        if (UART_MP_DISALBED())
        {
            g_UartQ.Data[g_UartQ.Wp] = HAL_UartRxByte();
#ifdef UART_DEBUG
            DBG_Printf("ISR RX: 1B\n");
#endif
            g_UartQ.Wp = (g_UartQ.Wp+1) % UARTQ_DEPTH;
        }
        else
        {
            ulRX_DW = HAL_UartRxDW();
            /* LOOP mechianism can't work in ISR */
            i = 0;
            g_UartQ.Data[g_UartQ.Wp] = (ulRX_DW >> (i++)*8) & 0xFF;
            g_UartQ.Wp = (g_UartQ.Wp+1) % UARTQ_DEPTH;

            g_UartQ.Data[g_UartQ.Wp] = (ulRX_DW >> (i++)*8) & 0xFF;
            g_UartQ.Wp = (g_UartQ.Wp+1) % UARTQ_DEPTH;

            g_UartQ.Data[g_UartQ.Wp] = (ulRX_DW >> (i++)*8) & 0xFF;
            g_UartQ.Wp = (g_UartQ.Wp+1) % UARTQ_DEPTH;

            g_UartQ.Data[g_UartQ.Wp] = (ulRX_DW >> (i++)*8) & 0xFF;
            g_UartQ.Wp = (g_UartQ.Wp+1) % UARTQ_DEPTH;

#ifdef UART_DEBUG
            DBG_Printf("ISR RX: 4B\n");
#endif
        }

        /* clear READ_MEM_FULL status */
        HAL_ClearRxValid();
    }
#ifdef UART_DEBUG
    DBG_Printf("Ie\n");
#endif
}

