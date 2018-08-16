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
Filename    :FW_BufAddr.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Dmae.h"
#include "HAL_HostInterface.h"
#include "FW_BufAddr.h"
#include "Disk_Config.h"

GLOBAL MCU12_VAR_ATTR U32 g_ulLdpcRandomDataAddr;

/****************************************************************************
Name        :COM_GetBufferIDByMemAddr
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
U32 COM_GetBufferIDByMemAddr(U32 TargetAddr,U8 bDram,U8 BufferSizeBits)
{
    U32 BuffID;
    U32 MemStartAddr;

    MemStartAddr = bDram ? DRAM_START_ADDRESS : OTFB_START_ADDRESS;

    BuffID = (TargetAddr - MemStartAddr) >> BufferSizeBits;

    return BuffID;
}

/****************************************************************************
Name        :COM_GetMemAddrByBufferID
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
U32 COM_GetMemAddrByBufferID(U32 BufferID,U8 bDram,U8 BufferSizeBits)
{
    U32 MemStartAddr;
    U32 MemAddr;

    MemStartAddr = bDram ? DRAM_START_ADDRESS : OTFB_START_ADDRESS;

    MemAddr = (BufferID << BufferSizeBits) + MemStartAddr;

    return MemAddr;
}

/****************************************************************************
Function  : COM_MemClearSectorData
Input     : 
Output    : 
Return    : 
Purpose   : 
clear sector data by bitmap, the bitmap is byte len
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void COM_MemClearSectorData(U32 ulBufAddr, U8 ucBitMap)
{
    U8  ucLoop;

    for(ucLoop = 0; ucLoop < BYTE_BIT_SIZE; ucLoop++, ulBufAddr += SEC_SIZE)
    {
        if ((ucBitMap & (0x1 << ucLoop)) != 0)
        {
            #ifdef SIM
            *(U32*)ulBufAddr = INVALID_8F;
            *((U32*)ulBufAddr + 1) = INVALID_8F;
            #else
            //COM_MemZero((U32*)ulBufAddr, (SEC_SIZE/sizeof(U32)));
            HAL_DMAESetValue(ulBufAddr, SEC_SIZE, 0);
            #endif
        }
    }

    return;
}

/* Maple add to generate random flash data to flash */
void  COM_SetCTGSeeds(U32 s, taus_state_t *state)
{
    if(s == 0) s=1;
    state->z1 = (U32)LCGCTG(s);
    if (state->z1 < 2)    state->z1 += 2;
    state->z2 = (U32)LCGCTG(state->z1);
    if (state->z2 < 8)    state->z2 += 8;
    state->z3 = (U32)LCGCTG(state->z2);
    if (state->z3 < 16)    state->z3 += 16;
    state->z4 = (U32)LCGCTG(state->z3);
    if (state->z4 < 128)    state->z4 += 128;

    return;
}

U32 COM_TausGet(taus_state_t *state)
{
    U32 b;

    b = (U32)(((state->z1 << Q1) ^ state->z1) >> (K1 - S1));
    state->z1 = (U32)(((state->z1 & (0xFFFFFFFF << (32-K1))) << S1) ^ b);
    b = (U32)(((state->z2 << Q2) ^ state->z2) >> (K2 - S2));
    state->z2 = (U32)(((state->z2 & (0xFFFFFFFF << (32-K2))) << S2) ^ b);
    b = (U32)(((state->z3 << Q3) ^ state->z3) >> (K3 - S3));
    state->z3 = (U32)(((state->z3 & (0xFFFFFFFF << (32-K3))) << S3) ^ b);
    b = (U32)(((state->z4 << Q4) ^ state->z4) >> (K4 - S4));
    state->z4 = (U32)(((state->z4 & (0xFFFFFFFF << (32-K4))) << S4) ^ b);

    return (U32)(state->z1 ^ state->z2 ^ state->z3 ^ state->z4);
}

void COM_WriteDataBuffInit(void)
{
    taus_state_t state;
    U32 seed = 0;
    U32 ulSecIndex, ulBuffIndex, ulBuffAddr, ulPu;
    U32 ulDwIndex;

    // Initialize the randomizer
    COM_SetCTGSeeds(seed,&state);
    COM_TausGet(&state);
    COM_TausGet(&state);
    COM_TausGet(&state);
    COM_TausGet(&state);
    
    g_ulLdpcRandomDataAddr = 0x54a00000;// Pending

    for (ulPu = 0; ulPu < SUBSYSTEM_LUN_NUM; ulPu++)
    {
        for (ulBuffIndex=0; ulBuffIndex < NFCQ_DEPTH; ulBuffIndex++)
        {
            ulBuffAddr = g_ulLdpcRandomDataAddr + (NFCQ_DEPTH*ulPu + ulBuffIndex)*BUF_SIZE;
            
            for (ulSecIndex = 0; ulSecIndex < SEC_PER_BUF; ulSecIndex++)
            {
                for (ulDwIndex = 0; ulDwIndex < 128; ulDwIndex++)
                {
                    *(U32*)(ulBuffAddr + (ulSecIndex << SEC_SIZE_BITS) + ulDwIndex * 4) = COM_TausGet(&state);
                }
            }
        }
    }

    return;
}
 

