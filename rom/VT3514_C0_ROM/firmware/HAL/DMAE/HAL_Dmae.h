/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_Dmae.h
Version     :
Author      :Kristin Wang
Date        :2014.7
Description :header file for DMAE driver.
Others      :
Modify      :
20140912    Kristin    1. Coding style uniform
                       2. Add A0 support
****************************************************************************/
#ifndef _HAL_DMAE_H_
#define _HAL_DMAE_H_

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#ifdef MCU0    //the "MCU0" MACRO is defined in MCU0's Proj_Config.h file
#define DMAE_TEXT_ATTR MCU0_DRAM_TEXT
#else
#define DMAE_TEXT_ATTR 
#endif

#ifdef VT3514_A0
#define MSK_DMAE_MCU_SEL  ( 3 << 16 )
#endif

#define DMAE_CMDENTRY_COUNT    8
#define DMAE_CMD_HEAD_MCU0     0
#define DMAE_CMD_TAIL_MCU0     1
#define DMAE_CMD_HEAD_MCU1     2
#define DMAE_CMD_TAIL_MCU1     4
#define DMAE_CMD_HEAD_MCU2     5
#define DMAE_CMD_TAIL_MCU2     7

/* [31:8] Reserved
 * [7:2] DBGSEL
 * [1:0] ReqPriority setting,0:low,3:high,0 is default
 * */
#define rDMAE_REQ_PRIORITY  (*(volatile U32 *)(REG_BASE_DMAE + 0x0))
#define MASK_DMAE_REQ_PRIORITY   0x3
#define DMAE_REQ_PRIORITY_LOW    0
#define DMAE_REQ_PRIORITY_HIGH   3

/* base address of DMAE command entry */
#define DMAE_CMDENTRY_BASE (REG_BASE_DMAE + 0x20)
#define DMAE_SEL_MCU0    0
#define DMAE_SEL_MCU1    1
#define DMAE_SEL_MCU2    2
#define DMAE_SEL_AREA_PIF    0x0
#define DMAE_SEL_AREA_OTFB   0x1
#define DMAE_SEL_AREA_DRAM   0x2
#define DMAE_SEL_AREA_SPI    0x3
#define DMAE_SEL_AREA_REG    0x4
#define DMAE_CMDENTRY_STATUS_IDLE       0
#define DMAE_CMDENTRY_STATUS_PENDING    1
#define DMAE_CMDENTRY_STATUS_EXECUTING  2
#define DMAE_CMDENTRY_STATUS_DONE       3

/* bit 0: 1-DMAE command entries full */
#define rDMAE_CMD_STATUS_FULL (*(volatile U8 *)(REG_BASE_DMAE + 0xa0))
/* bit 0: 1-DMAE command entries all empty */
#define rDMAE_CMD_STATUS_EMPTY (*(volatile U8 *)(REG_BASE_DMAE + 0xa1))
/* bit [2:0]: DMAE command ID suggested */
#define rDMAE_CMD_STATUS_CMDID (*(volatile U8 *)(REG_BASE_DMAE+ 0xa2))

/* bit k:DMA command k interrupt */
#define rDMAE_CMD_INI  (*(volatile U8 *)(REG_BASE_DMAE + 0xa4))
/* bit k:DMA command k clear */
#define rDMAE_CMD_CLEAR  (*(volatile U8 *)(REG_BASE_DMAE + 0xa5))
/* bit k:DMA command k mask */
#define rDMAE_CMD_MSK (*(volatile U8 *)(REG_BASE_DMAE + 0xa6))

/* bit 0:Power management enable
 * bit 1:Low DCLK speed setting
 * bit [7:2]:Reserved
 * */
#define rDMAE_CLK_PM (*(volatile U8 *)(REG_BASE_DMAE + 0xa7))
#define BIT_DMAE_PM_EN      0
#define BIT_DMAE_LOW_DCLK   1

#define DMAE_REG_DATA_BASE  (REG_BASE_DMAE + 0xb0)

//#define SPI_START_ADDRESS  0xc0000000
#define DMAE_ENTRY_MAX_LENGTH_IN_BYTE  0x20000

typedef struct _DMAE_CMDENTRY
{
    /* DWORD 0 */
    U32 ulSrcAddr;      //source address,absolute address

    /* DWORD 1 */
    U32 ulDesAddr;      //destination address,absolute address

    /* DWORD 2 */
    U32 bsDesType: 2;   //b00:PIF,b01:OTFB,b10:DRAMC,b11:SPI
    U32 bsRsv0: 2;      
    U32 bsSrcType :3;   //b000:PIF,b001:OTFB,b010:DRAMC,b011:SPI,b100:register
    U32 bRsv1: 1;       
    U32 bTrigger: 1;    
    U32 bsRsv2: 3;      
    U32 bsStatus: 2;    //0:Idle,1:Pending,2:Executing,3:Done
    U32 bsMCUSel: 2;
    U32 bsLength: 13;   //0:16bytes,0x1FFF:128Kbytes
    U32 bsRsv3 :3;

    /* DWORD 3 */
    U32 ulRsv4;
}DMAE_CMDENTRY;

void DMAE_TEXT_ATTR HAL_DMAEInit(void);
void DMAE_TEXT_ATTR HAL_DMAETriggerCmd(U8 ucCmdID);
BOOL DMAE_TEXT_ATTR HAL_DMAEParseAddress(U8 *pAreaType, U32 *pAddrOut, const U32 ulAddrIn);
void DMAE_TEXT_ATTR HAL_DMAEAddCmd(U8 ucCmdID,U32 ulCmdSrcAddr,U32 ulCmdDesAddr,U8 ucSrcAreaType,U8 ucDesAreaType,U32 ulBlockLenInByte);
void DMAE_TEXT_ATTR HAL_DMAECopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte);
void DMAE_TEXT_ATTR HAL_DMAESetValue(const U32 ulDesAddr,const U32 ulLenByte, U32 ulValue);

#endif //_HAL_DMAE_H_
