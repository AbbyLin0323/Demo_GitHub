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

#include <windows.h>
#include "HostModel.h"
#include "model_common.h"

extern void Host_WriteToDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag);
extern void Host_ReadFromDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag);

void Host_FillDataToBuffer(PLARGE_INTEGER BufferAddr,U32 DstLba)
{
    U32 ulData = Host_UpdateDataCnt(DstLba);
    //LARGE_INTEGER = (LARGE_INTEGER)(*BufferAddr);

    BufferAddr->LowPart = ulData;
    BufferAddr->HighPart = DstLba;
}

void Host_ReadDataFromBuffer(PLARGE_INTEGER BufferAddr,U32 DstLba)
{
    U32 ulData = Host_GetDataCnt(DstLba);
    
//#ifdef SINGLE_SUBSYSTEM
//    U32 ulSubsystemLba = L0M_GET_SUBSYSLBA_FROM_LBA(DstLba,0);
//#else
//    U32 ulSubsystemLba = L0M_GET_SUBSYSLBA_FROM_LBA(DstLba,1);
//#endif

    if (0 == ulData)//host did not write this LBA, do NOT check
    {
        /* TRIM could be discard during normal/abnormal power off */
#ifdef TRIM_DATA_CHECK
        if (BufferAddr->HighPart != INVALID_8F)
        {
            DBG_Printf("Host_ReadDataFromBuffer: High Lba 0x%x, DataCnt = 0, Lba in Buffer = 0x%x\n", 
                        DstLba,BufferAddr->HighPart);

            DBG_Break();
        }
        else if (BufferAddr->LowPart != INVALID_8F)
        {
             DBG_Printf("Host_ReadDataFromBuffer: Low Lba 0x%x, DataCnt = 0, data in Buffer = %d\n", 
                        DstLba,BufferAddr->LowPart);

            DBG_Break();
        }
#endif
        return;
    }
   
#ifdef DBG_TABLE_REBUILD
    if(0 != Host_GetDataCntHighBit(DstLba))
    { 
        return;
    }
    BufferAddr->LowPart &= HOST_WRITE_CNT_MSK;
#endif  

    if (BufferAddr->HighPart != DstLba)
    {
        DBG_Printf("Host_ReadDataFromBuffer: High Lba in Buffer = 0x%x, Lba in CMD = 0x%x\n", 
                    BufferAddr->HighPart,DstLba);

        DBG_Break();
    }
    else if (BufferAddr->LowPart != ulData)
    {
        DBG_Printf("Host_ReadDataFromBuffer: Low Lba = 0x%x, data in Buffer = %d, data in Host = %d\n", 
            DstLba,BufferAddr->LowPart,ulData);

        DBG_Break();
    }
}


// begin:interface for SGE
void Host_WriteToOTFB(U32 HostAddrHigh, U32 HostAddrLow, U32 OTFBAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = OTFB_START_ADDRESS + OTFBAddr;    
    Host_WriteToDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

void Host_WriteToDram(U32 HostAddrHigh, U32 HostAddrLow, U32 DramAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = DRAM_START_ADDRESS + DramAddr;
    Host_WriteToDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

void Host_ReadFromDevice(U32 ulDeviceAddr, U32 ulBtyes, U8 *pDestBuf)
{
#ifdef SIM
    memcpy(pDestBuf, (void *)ulDeviceAddr, ulBtyes);
#else
    dramRead(ulDeviceAddr, ulBtyes/4, (U32 *)pDestBuf);
#endif
}

void Host_WriteToDevice(U32 ulDeviceAddr, U32 ulBytes, const U8 *pSrcBuf)
{
#ifdef SIM
    memcpy((void *)ulDeviceAddr, (void *)pSrcBuf, ulBytes);
#else
    dramWrite(ulDeviceAddr, ulBytes/4, (U32*)pSrcBuf);
#endif    
}

void Host_ReadFromOTFB(U32 HostAddrHigh, U32 HostAddrLow, U32 OTFBAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = OTFB_START_ADDRESS + OTFBAddr;

#ifndef VT3514_C0
    if (0 != (HostAddrLow & 0x1FF))
    {
        DBG_Printf("Host_WriteToOTFB: HostAddr is not align by 512 bytes \n");
        DBG_Break();
    }
#endif

    Host_ReadFromDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}

void Host_ReadFromDram(U32 HostAddrHigh, U32 HostAddrLow, U32 DramAddr, U32 ByteLen, U8 nTag)
{
    U32 ulBufferAddr = DRAM_START_ADDRESS + DramAddr;
    Host_ReadFromDeviceInterface(HostAddrHigh, HostAddrLow, ulBufferAddr, ByteLen, nTag);
}
