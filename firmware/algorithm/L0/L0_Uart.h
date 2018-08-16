
/****************************************************************************
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
*****************************************************************************
 * File Name    : Uart.h
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#ifndef L0_UART_H
#define L0_UART_H

#include "L0_ViaCmd.h"

#define UART_SIGNATURE      (0x34313533) // '4''1''5''3'

#define UART_PACKET_SIZE    (4)
#define UART_RD_DATA_LEN    (3)
#define UART_WR_DATA_LEN    (2)


enum{
    DATA_TRANSFER_NONE = 0,
    DATA_TRANSFER_SATA = 1,
    DATA_TRANSFER_UART_POLL = 2,
    DATA_TRANSFER_UART_INT = 3
};

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
    VIA_CMD_PARAM tViaCmdParam;
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
    UART_VENDER_CMD_OPCODE_READ = 0,
    UART_VENDER_CMD_OPCODE_WRITE,
    UART_VENDER_CMD_OPCODE_JUMP,
    UART_VENDER_CMD_OPCODE_TX,
    UART_VENDER_CMD_OPCODE_CLRTXBUF,
    UART_VENDER_CMD_OPCODE_DMA_WR_OP,
    UART_VENDER_CMD_OPCODE_DMA_WR_DATA,
    UART_VENDER_CMD_OPCODE_DMA_RD,
    UART_VENDER_CMD_OPCODE_NUM,
    UART_VENDER_CMD_OPCODE_VIACMD = (0xFF)
};

enum{
    UART_SUCC,
    UART_FAIL    
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
    UART_STATE_PROC_DMA_WR_DATA,
    UART_STATE_PROC_VIA_CMD,
    UART_STATE_PROC_DMA_RD

}UART_STATE;

typedef struct UART_MP_MGR{
    BOOL ulMpEn;
    UART_STATE ulCurrState;
    U32 ulRevDataLen;
    U32 ulDmaWrAddr;
}UART_MP_MGR;


typedef enum UART_VIA_CMD_STATE{
   UART_VIA_CMD_STATE_IDLE, 
   UART_VIA_CMD_STATE_RX_DATA,
   UART_VIA_CMD_STATE_TX_DATA,
   UART_VIA_CMD_STATE_ISSUE_SCMD,
   UART_VIA_CMD_STATE_FINISH
}UART_VIA_CMD_STATE;

#define SUB_SYSTEM_PU_MAX (16)

void L0_UartMpInit(void);
void L0_UartDBG(void);
void L0_UartEnReadINT(BOOL bEnable);
void ISR_UART(void);

#endif

