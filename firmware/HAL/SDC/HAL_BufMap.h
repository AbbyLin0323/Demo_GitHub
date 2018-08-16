/*******************************************************************************
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
********************************************************************************
Filename    :HAL_BufMap.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    19:18:57
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
*******************************************************************************/
#ifndef _HAL_BUFMAP_H
#define _HAL_BUFMAP_H

#include "BaseDef.h"
/*
*    Buffer map related register
*/
#define BUFMAP_CONFIG_BASE_ADDRESS    REG_BASE_BUFM

/*
bit[23:16] = mask bits. 0 = set, 1 = don't set.
bit[7:6] = 10'b, Setting low 8 bit
bit[7:6] = 11'b, Setting high 8 bit
*/
#define rBufMapSet              (*((volatile U32 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x00)))
#define rBufMapSetH             (*((volatile U32 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x10)))

/*
*    BIT     Attribute  Description
*    31:8    RSV
*    7:6     WO     00  read buffer map status
*                   01  last data ready status
                    10  first data ready status
                    11  write buffer map status
*    5:0     WO         buffer map ID that needs to initialize, 0 to 63
*/
#define rContextSet             (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x8)))
#define GETBM_READBUFMAP        0x0
#define GETBM_LASTDATAREADY     0x1
#define GETBM_FIRSTDATAREADY    0x2
#define GETBM_WRITEBUFMAP       0x3

//FW get result by reading rContextSet, so we redefine rContextSet to rResultGet
#define rResultGet              (*((volatile U32 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x8)))

/*
*    Read data status related register
*/
/*
*    BIT    Attribute Default    Description
*    7    WO                       0, no means
*                                         1, set the read status register to 1'b0
*    6    RSV
*    5                    1'b0
*    4:0    WO                register ID will be initialized, 0 to 31
*/
#define rReadDataStatusInit     (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x4)))

/*   2013.11.05 Haven Yang update
*    Set Last data Ready status related register
*    BIT    Attribute Default    Description
*    7     WO              0, no means
*                          1, Set Last data Ready status register to 1'b0
*    6     RSV
*    5                     1'b0
*    4:0   WO                 register ID(CmdTag) will be initialized, 0 to 31
*/
#define rLastDataStatusInit     (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0x5)))

#define rWriteBufMapSet         (*((volatile U8 *)(BUFMAP_CONFIG_BASE_ADDRESS + 0xc)))

#define WRITE_BUFMAP_IDLE       0x00
#define WRITE_BUFMAP_SATA       0x01
#define WRITE_BUFMAP_NFC        0x02

#define BUFMAP_ID(DsgID)        (DsgID)

extern void HAL_ResetBuffMap(void);
extern void HAL_HoldBuffMap(void);
extern void HAL_ReleaseBuffMap(void);
extern GLOBAL void HAL_SetBufMapInitValue(U8 ucBufMapId, U32 ucInitValue);
extern GLOBAL U32  HAL_GetBufMapValue(U8 ucBufMapId);
extern GLOBAL void HAL_UpdateBufMapValue(U8 ucBufMapId, U32 ulBufMapValueSet, U32 ulBufMapValueMask);
extern GLOBAL void HAL_SetFirstReadDataReady(U8 ucCmdTag);
extern GLOBAL void HAL_SetLastDataReady(U8 ucCmdTag);
extern GLOBAL void HAL_SetWriteBufMap(U8 ucBufMapId, U32 ulValue);
extern GLOBAL U32  HAL_WriteBufMapIsFree(U8 ucBufMapId);

#endif //_HAL_BUFMAP_H

