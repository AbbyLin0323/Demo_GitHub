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
*******************************************************************************/

#include "simsatainc.h"
#include "sdc_common.h"
#include "HAL_SataIO.h"
#include "HAL_BufMap.h"

#define  u32 U32
sata_cmdinfo sata_cmd_entry[33];

/*sdc register read handler */
#if 0
void sdcRegRead (u32 regaddr, u32 regvalue, u32 nsize)
{
    U32 ntemp = 0;

    //if ( regaddr == &rSACMDM_CURRPRD)
    //    //sim_satadev_read_curprd_rprd();
    //if ((U32)&rSDC_SendSDBFISReady == regaddr)
    //{
    //    regWrite((U32)&rSDC_SendSDBFISReady, 4, (U8*)&g_SendSDBFISReady);
    //}
    if((U32)&rSDC_IOControl == regaddr)
    {
        regWrite((U32)&rSDC_IOControl, 1, (U8*)&ntemp);
    }

    return;
}

// it's address will be u32 allaigned
void spinLockRead(u32 regaddr, u32 regvalue, u32 nsize)
{
    U32 nLockValue = 0;

    regRead(regaddr, 4, (U8*)&nLockValue);

    /*
    if(0 == nLockValue)
    {
    nLockValue = 1;
    regWrite(regaddr, 4, &nLockValue);
    }*/

}
#endif

// it's address will be u32 allaigned
BOOL spinLockWrite(u32 regaddr, u32 regvalue, u32 nsize)
{
    U32 nLockValue = 0;

    regRead(regaddr, 4, (U8*)&nLockValue);

    if (0 == regvalue)
    {
        nLockValue = 0;
        regWrite(regaddr, 4, (U8*)&nLockValue);
        return FALSE;
    }

    if(0 == nLockValue)
    {
        if(regvalue == 0x1)//PRID 0x1,MCU0
        {
            nLockValue = 0x1;
            regWrite(regaddr, 4, (U8*)&nLockValue);
        }
        else if (regvalue == 0x2) //PRID 0x1,MCU1
        {
            nLockValue = 0x2;
            regWrite(regaddr, 4, (U8*)&nLockValue);
        }
        else
        {

        }
    }
    //regWrite(regaddr, 4, &nLockValue);

    return FALSE;
}

/*sdc register write handler*/
#if 0
BOOL sdcRegWrite(u32 regaddr, u32 regvalue, u32 nsize)
{
    U8 ucByteVal;
    BOOL bWrite = TRUE;
    U32 uTemp  = 0;

    if ( regaddr == (U32)&rSDC_SHRLCH_EN)
    {
        if ((regvalue & 0xFF) == 1)
        {
            SDC_LchShadowReg();
        }
    }
    //else if (regaddr == (U32)&rSDC_SendSDBFISReady)
    //{
    //
    //    g_SendSDBFISReady = regvalue;//SDC_SetSDBFISReady(regvalue);
    //}
    /*marked by haven, to be deleted.
    else if (regaddr == (U32)&rSACMDM_RPRDReadPtr_Trigger)
    {
        //SDC_AllocateReadPrd();
        //printf("Read PRD triger at %04x cycles \n", nCycles);
#ifndef SIM
        SystemStatisticRecord(TRACE_SATADEVICE_MODULE_ID,TRACE_SATADEVICE_SUBMODULE_ID,2,"Read PRD trigger %08x \n",sata_sim_clocktime());
#endif
        bWrite = FALSE;
    }*/
    else if (regaddr == (U32)&rBufMapSet)
    {
        SDC_SetReadBufferMap(regvalue);
    }
    else if (regaddr == (U32)&rContextSet)
    {
        U8  read_content = (regvalue >> 6) & 3;
        U8  read_id      = regvalue & 0x3F;
        U32 read_value   = 0;

        switch (read_content)
        {
            case GETBM_READBUFMAP:
                read_value = SDC_GetReadBufferMap(read_id);
                break;
            case GETBM_WRITEBUFMAP:
                read_value = SDC_GetWriteBufferMap(read_id);
                break;
            case GETBM_FIRSTDATAREADY:
                read_value = SDC_IsFirstReadDataReady(read_id);
                break;
            case GETBM_LASTDATAREADY:
                read_value = SDC_IsLastDataReady(read_id);
                break;
            default:
                printf("set rBufMapSet register error!\n");
                DBG_Getch();
        }

        regWrite((U32)&rResultGet, 4, (U8*)&read_value);

        bWrite = FALSE;
    }
    /*marked by haven, to be deleted.
    else if (regaddr == (U32)&rSACMDM_WPRDWritePtr_Trigger)
    {
        //SDC_AllocateWritePrd();

#ifndef SIM
    SystemStatisticRecord(TRACE_SATADEVICE_MODULE_ID,TRACE_SATADEVICE_SUBMODULE_ID,2,"Write PRD trigger %08x \n",sata_sim_clocktime());
#endif
    bWrite = FALSE;
    }*/
    else if (regaddr == (U32)&rSDC_IntSrcPending)
    {
        uTemp = 0;
        if (((regvalue & 0xffff) == BIT_SDC_INTSRC_FIS_COMMAND)
        ||((regvalue & 0xffff) == BIT_SDC_INTSRC_NCQFINISH))
        {
            regWrite((U32)&rSDC_IntSrcPending, 2,(U8*) &uTemp);
        }
#ifndef SIM
        setInterrupt(9, 0);
#endif

        bWrite = FALSE;
    }
    else if (regaddr == (U32)&rReadDataStatusInit)
    {
        ucByteVal = regvalue & 0xFF;

        if (ucByteVal & (1<<7))
        {
            SDC_SetFirstReadDataReady(ucByteVal);
        }
        bWrite = FALSE;
    }
    else if (regaddr == (U32)&rLastDataStatusInit)
    {
        ucByteVal = (regvalue>>8) & 0xFF;

        if (ucByteVal & (1<<7))
        {
            SDC_SetLastDataReady(ucByteVal);
        }
        bWrite = FALSE;
    }

    return bWrite;

}
#endif

U32 sata_sim_clocktime()
{
    U32 ntime = 0;

#ifndef SIM
    ntime = XTMP_clockTime();
#endif

    return ntime;
}

#ifdef SIM
void regRead(U32 addr, U32 nBytes, U8 *dst)
{
    U8 *buf = (U8*)addr;
    memcpy(dst, buf, nBytes);
    return;
}

void regWrite(U32 addr, U32 nBytes, const U8*src)
{
    U8 *buf = (U8*)addr;
    memcpy(buf, src, nBytes);
    return;
}

// r/w dram, addr be aligned on a word boundary
void dramRead(U32 addr, U32 nWordss, U8 *buf)
{
    memcpy(buf, (void *)addr, nWordss*4);
}

void dramWrite(U32 addr, U32 nWordss, const U8* src)
{
    memcpy((void *)addr, (void *)src, nWordss*4);
}
#endif
