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

Filename     : sim_DMAE.c
Version      : Ver 1.0
Date         :
Author       : Kristin

Description:
                DMAE model for XTMP/Win simulation
Modification History:
20140703    Kristin   created
------------------------------------------------------------------------------*/

#include <malloc.h>
#include <string.h>
#include "HAL_Dmae.h"
#include "sim_DMAE.h"
#include "memory_access.h"
#include "HAL_GLBReg.h"
#include "model_common.h"

#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#include "iss/mp.h"
#endif

#ifdef SIM_XTENSA

extern u8 *g_pIsramBaseAddr[];
#endif

LOCAL U32 *l_paDmaeTempBuffer[DMAE_CMDENTRY_COUNT];
LOCAL DMAE_CMDENTRY *l_apDmaeCmdEntry;

void DMAE_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = SetValue;
    }
}

void DMAE_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = *SrcAddr++;
    }
}

#ifdef SIM_XTENSA
LOCAL U32 DMAE_ISramCalAddress(U32 ulAddrFW, U8 ucMCUSel)
{
    U32 ulAddrModel;

    switch(ucMCUSel)
    {
    case DMAE_SEL_MCU0:
        ulAddrModel = ulAddrFW - MCU0_ISRAM_BASE + (U32)g_pIsramBaseAddr[0];
        break;

    case DMAE_SEL_MCU1:
        ulAddrModel = ulAddrFW - MCU0_ISRAM_BASE + (U32)g_pIsramBaseAddr[1];
        break;

    case DMAE_SEL_MCU2:
        ulAddrModel = ulAddrFW - MCU0_ISRAM_BASE + (U32)g_pIsramBaseAddr[2];
        break;

    default:
        DBG_Printf("DMAE_ModelProcess: MCU select error!\n");
        DBG_Break();
        break;
    }

    return ulAddrModel;
}
#endif

void DMAE_ModelProcessCmdEntry(U8 ucCMDID)
{
    U32 ulCmdEntryAddr;
    DMAE_CMDENTRY tCurCmdEntry;
    U32 ulSrcAddr;
    U32 ulDesAddr;
    U32 ulDataRegAddr;
    U32 ulSetValue;
    U32 ulLenDW;

    ulCmdEntryAddr = DMAE_CMDENTRY_BASE + sizeof(DMAE_CMDENTRY) * ucCMDID;
    Comm_ReadReg(ulCmdEntryAddr, sizeof(DMAE_CMDENTRY)/sizeof(U32), (U32 *)&tCurCmdEntry);

    tCurCmdEntry.bsStatus = DMAE_CMDENTRY_STATUS_EXECUTING;
    Comm_WriteReg(ulCmdEntryAddr, sizeof(DMAE_CMDENTRY)/sizeof(U32), (U32 *)&tCurCmdEntry);

    ulLenDW = ( (tCurCmdEntry.bsLength + 1) << 2 );

    /*Copy data from source address to temporary buffer*/
    switch( tCurCmdEntry.bsSrcType )
    {
    case DMAE_SEL_AREA_DRAM:
        ulSrcAddr = tCurCmdEntry.ulSrcAddr;
        Comm_ReadDram(ulSrcAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        break;

    case DMAE_SEL_AREA_OTFB:
        ulSrcAddr = tCurCmdEntry.ulSrcAddr;
        Comm_ReadOtfb(ulSrcAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        break;

    case DMAE_SEL_AREA_PIF:
#ifdef SIM_XTENSA
        if ( tCurCmdEntry.ulSrcAddr < MCU0_ISRAM_BASE )//DSRAM
        {
            ulSrcAddr = tCurCmdEntry.ulSrcAddr;
            Comm_ReadReg(ulSrcAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        }
        else//ISRAM
        {
            ulSrcAddr = DMAE_ISramCalAddress(tCurCmdEntry.ulSrcAddr, tCurCmdEntry.bsMCUSel);
            DMAE_MemCpy( l_paDmaeTempBuffer[ucCMDID], (U32 *)ulSrcAddr, ulLenDW );
        }//else of if ( ulAddrFW < MCU0_ISRAM_BASE )
#else
        ulSrcAddr = tCurCmdEntry.ulSrcAddr;
        DMAE_MemCpy( l_paDmaeTempBuffer[ucCMDID], (U32 *)ulSrcAddr, ulLenDW );
#endif
        break;

    case DMAE_SEL_AREA_REG:
        ulDataRegAddr =  DMAE_REG_DATA_BASE + sizeof(U32) * ucCMDID;
        Comm_ReadReg( ulDataRegAddr, 1, (U32 *)&ulSetValue);
        DMAE_MemSet(l_paDmaeTempBuffer[ucCMDID], ulLenDW, ulSetValue);
        break;

    case DMAE_SEL_AREA_SPI:
        break;

    default:
        DBG_Printf("DMAE_ModelProcess: entry %d source type error!\n", ucCMDID);
        DBG_Break();
        break;
    }

    /*Copy data from temporary buffer to destination address*/
    switch( tCurCmdEntry.bsDesType )
    {
    case DMAE_SEL_AREA_DRAM:
        ulDesAddr = tCurCmdEntry.ulDesAddr;
        Comm_WriteDram(ulDesAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        break;

    case DMAE_SEL_AREA_OTFB:
        ulDesAddr = tCurCmdEntry.ulDesAddr;
        Comm_WriteOtfb(ulDesAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        break;

    case DMAE_SEL_AREA_PIF:
#ifdef SIM_XTENSA
        if ( tCurCmdEntry.ulDesAddr < MCU0_ISRAM_BASE )//DSRAM
        {
            ulDesAddr = tCurCmdEntry.ulDesAddr;
            Comm_WriteReg(ulDesAddr, ulLenDW, l_paDmaeTempBuffer[ucCMDID]);
        }
        else//ISRAM
        {
            ulDesAddr = DMAE_ISramCalAddress(tCurCmdEntry.ulDesAddr, tCurCmdEntry.bsMCUSel);
            DMAE_MemCpy( (U32 *)ulDesAddr, l_paDmaeTempBuffer[ucCMDID], ulLenDW );
        }//else of if ( ulAddrFW < MCU0_ISRAM_BASE )
#else
        ulDesAddr = tCurCmdEntry.ulDesAddr;
        DMAE_MemCpy( (U32 *)ulDesAddr, l_paDmaeTempBuffer[ucCMDID], ulLenDW );
#endif
        break;

    case DMAE_SEL_AREA_SPI:
        break;

    default:
        DBG_Printf("DMAE_ModelProcess: entry %d destination type error!\n", ucCMDID);
        DBG_Break();
        break;
    }

    tCurCmdEntry.bTrigger = 0;
    tCurCmdEntry.bsStatus = DMAE_CMDENTRY_STATUS_DONE;

    Comm_WriteReg(ulCmdEntryAddr, sizeof(DMAE_CMDENTRY)/sizeof(U32), (U32 *)&tCurCmdEntry);

    return;
}

void DMAE_ModelInit(void)
{
    U8 i;
#ifdef SIM_XTENSA
    U32 ulRegValue;
#endif
    for ( i = 0; i < DMAE_CMDENTRY_COUNT; i++ )
    {
        l_paDmaeTempBuffer[i] = (U32*)malloc(128 * 1024);//128KB
    }
    l_apDmaeCmdEntry = (DMAE_CMDENTRY *)DMAE_CMDENTRY_BASE;

#if defined(SIM_XTENSA)
    ulRegValue = 0x100;
    Comm_WriteReg((U32)&rDMAE_CMD_STATUS_FULL,1,&ulRegValue);

    ulRegValue = 0x0;
    for ( i = 0; i < DMAE_CMDENTRY_COUNT; i++ )
    {
        Comm_WriteReg((U32)(&l_apDmaeCmdEntry[i]) + 8,1,&ulRegValue);
    }
#elif defined(SIM)
    rDMAE_CMD_STATUS_FULL = 0x0;
    rDMAE_CMD_STATUS_EMPTY = 0x1;
    rDMAE_CMD_STATUS_CMDID = 0;

    for ( i = 0; i < DMAE_CMDENTRY_COUNT; i++ )
    {
        l_apDmaeCmdEntry[i].bsStatus = DMAE_CMDENTRY_STATUS_IDLE;
        l_apDmaeCmdEntry[i].bTrigger = 0;
    }
#endif //#elif defined(SIM)
}


BOOL DmaeRegWrite(U32 regaddr ,U32 regvalue ,U32 nsize)
{
    U8 i;
    U8 ucCMDID;
    U32 ulCmdTrigAddr;
    U32 ulCurRegValue;
    U8 *pByteDes;
    U8 *pByteSrc;
    BOOL bTrigger = FALSE;

    for ( ucCMDID = 0; ucCMDID < DMAE_CMDENTRY_COUNT; ucCMDID++ )
    {
        ulCmdTrigAddr = DMAE_CMDENTRY_BASE + sizeof(DMAE_CMDENTRY) * ucCMDID + 9;

        if ( (regaddr <= ulCmdTrigAddr) && ( (regaddr + nsize) > ulCmdTrigAddr ) )//hit the byte of trigger
        {
            Comm_ReadReg(regaddr, 1, &ulCurRegValue);
            pByteDes = (U8 *)&ulCurRegValue;
            pByteSrc = (U8 *)&regvalue;

            for (i = 0; i < nsize; i++)
            {
                *pByteDes = *pByteSrc;

                if ( (regaddr + i) == ulCmdTrigAddr )
                {
                    if ( 0 != ((*pByteDes) & 0x1) )//trigger
                    {
                        (*pByteDes) &= (~ (3 << 4));//clear status field to 0
                        (*pByteDes) |= (DMAE_CMDENTRY_STATUS_PENDING << 4);//set status to Pending

                        bTrigger = TRUE;
                    }
                }

                pByteDes++;
                pByteSrc++;
            }

            Comm_WriteReg(regaddr, 1, &ulCurRegValue);

            if (TRUE == bTrigger)
            {
                DMAE_ModelProcessCmdEntry(ucCMDID);
            }

            return FALSE;
        }//if ( (regaddr <= ulCmdTrigAddr) && ( (regaddr + nsize) > ulCmdTrigAddr ) )
    }//for ( ucCMDID = 0; ucCMDID < DMAE_CMDENTRY_COUNT; ucCMDID++ )

    return TRUE;
}

