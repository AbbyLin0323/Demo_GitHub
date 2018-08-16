/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_BufMap.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    19:18:57
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/

#ifndef _HAL_BUFMAP_H
#define _HAL_BUFMAP_H

#include "BaseDef.h"

/*
*    Buffer map related register
*/

#define    BUFMAP_CONFIG_BASE_ADDRESS    REG_BASE_BUFM

/*
*    BIT        Attribute Default    Description
*    31:16    RSV                   
*     15:8        WO                The initial value that need to set                       
*    7        WO                0: clean the buffer map to all 8'b0
*                            1: set the buffer map to initial value of Reg[15:8]
*    6        RSV    
*    5:0        WO                buffer map ID that needs to initialize, 0 to 63
*/
#define rBufMapSet  (*((volatile U32 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x00)))
#define rBufMapSetH (*((volatile U32 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x10)))

/* 2013.11.05 Haven Yang update */
/*  
*    BIT     Attribute  Description
*    31:8    RSV                   
*    7:6     WO     00  read buffer map status
*                   01  last data ready status
                    10  first data ready status
                    11  write buffer map status
*    5:0     WO         buffer map ID that needs to initialize, 0 to 63
*/
#define rBufMapGet  (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x8)))
#define GETBM_READBUFMAP        0x0
#define GETBM_LASTDATAREADY     0x1
#define GETBM_FIRSTDATAREADY    0x2
#define GETBM_WRITEBUFMAP       0x3

/*
*    Read data status related register
*/
/*
*    BIT    Attribute Default    Description
*    7    WO                       0, no means
*                                  1, set the read status register to 1'b0 
*    6    RSV
*    5                    1'b0
*    4:0    WO                register ID(CmdTag) will be initialized, 0 to 31
*/
#define    rReadDataStatusInit    (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x4)))

/*   2013.11.05 Haven Yang update
*    Set Last data Ready status related register
*    BIT    Attribute Default    Description
*    7     WO              0, no means
*                          1, Set Last data Ready status register to 1'b0 
*    6     RSV
*    5                     1'b0
*    4:0   WO                 register ID(CmdTag) will be initialized, 0 to 31
*/
#define    rLastDataStatusInit    (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x5)))

#define    rWriteBufMapSet    (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0xc)))

#define    WRITE_BUFMAP_IDLE     0x00
#define    WRITE_BUFMAP_SATA     0x01
#define    WRITE_BUFMAP_NFC      0x02

#define BUFMAP_ID(DsgID)    (DsgID)

extern  GLOBAL  void    HAL_SetBufMapInitValue(U8    ucBufMapId, U32 ucInitValue);
extern  GLOBAL  U32    HAL_GetBufMapValue(U8 ucBufMapId);
GLOBAL    void  HAL_UpdateBufMapValue(U8 ucBufMapId, U32 value);
extern  GLOBAL  void    HAL_SetFirstReadDataReady(U8 ucCmdTag);
extern  GLOBAL  void    HAL_SetLastDataReady(U8 ucCmdTag);
extern  GLOBAL  void    HAL_SetWriteBufMap(U8 ucBufMapId, U32 ulValue);
extern  GLOBAL  U32    HAL_GetWriteBufMapValue(U8    ucBufMapId);
extern  GLOBAL  U32    HAL_WriteBufMapIsFree(U8    ucBufMapId);
extern  GLOBAL  void    HAL_WriteBufMapTest(void);

#endif

