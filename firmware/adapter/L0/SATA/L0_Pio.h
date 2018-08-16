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
Filename    :L0_Pio.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_PIO_H__
#define __L0_PIO_H__

#include "BaseDef.h"

// PIO state definitions
#define SATA_PIO_NOCMD      0x00
#define SATA_PIO_NEWCMD     0x01
#define SATA_PIO_SETUP      0x02
#define SATA_PIO_DATA_IN    0x03
#define SATA_PIO_DATA_OUT   0x04
#define SATA_PIO_FINISH     0x05

typedef struct _PIO_Data_Info
{
    U8 ucCurrPIOState;
    U8 bCmdRead;
    U8 ucDrqLen;
    U8 ucReserved;
    U32 ulCmdTotalSecLen;
} PIOINFO;

extern void L0_PioInit(void);
void L0_SataSetupPIOInOut(BOOL bPIOIn, U8 ucDRQLen, U32 ulCmdTotalSecLen);
extern void L0_HandlePioProtocol(void);
extern void L0_ReceivePioDataIsr(void);

#endif
