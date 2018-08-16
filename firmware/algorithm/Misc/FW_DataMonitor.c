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
Filename    :FW_DataMonitor.c
Version     :Ver 1.0
Author      :BlakeZhang
Date        :2014.07.30
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_NfcDataCheck.h"
#include "HAL_Dmae.h"
#include "Disk_Config.h"
#include "FW_DataMonitor.h"
#include "FW_BufAddr.h"
#include "COM_Memory.h"

#ifdef DATA_MONITOR_ENABLE

GLOBAL MCU12_VAR_ATTR U8* g_pucDataSignDisk;

void MCU12_DRAM_TEXT FW_DataMonitorInit(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    COM_MemAddrPageBoundaryAlign(pFreeDramBase);
    ulFreeDramBase = *pFreeDramBase;

    g_pucDataSignDisk = (U8*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, MAX_LBA_IN_DISK);
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

#ifdef SIM
    HAL_DMAESetValue((U32)g_pucDataSignDisk, COM_MemSize16DWAlign(MAX_LBA_IN_DISK), INVALID_8F);
#else
    HAL_DMAESetValue((U32)g_pucDataSignDisk, COM_MemSize16DWAlign(MAX_LBA_IN_DISK), 0);
#endif

    HAL_DataCheckInit();

    return;
}

void FW_DataMonitorSet(U32 ulLBA, U8 ucSignature)
{
#ifdef SIM
    if (MAX_LBA_IN_DISK < ulLBA)
    {
        DBG_Printf("FW_DataMonitorSet ulLBA 0x%x ERROR!\n", ulLBA);
        DBG_Getch();
    }
#endif 
  
    g_pucDataSignDisk[ulLBA] = ucSignature;

    return;
}

U8 FW_DataMonitorGet(U32 ulLBA)
{
#ifdef SIM
      if (MAX_LBA_IN_DISK < ulLBA)
      {
          DBG_Printf("FW_DataMonitorGet ulLBA 0x%x ERROR!\n", ulLBA);
          DBG_Getch();
      }
#endif 
  
    return g_pucDataSignDisk[ulLBA];
}

void FW_DataMonitorCheck(U32 ulLBA, U8 ucSignature)
{
#ifdef SIM
    if (MAX_LBA_IN_DISK < ulLBA)
    {
        DBG_Printf("FW_DataMonitorCheck ulLBA 0x%x ERROR!\n", ulLBA);
        DBG_Getch();
    }
#endif 
  
    if (ucSignature != g_pucDataSignDisk[ulLBA])
    {
        DBG_Printf("FW_DataMonitorCheck LBA 0x%x Data 0x%x Wrong data 0x%x ERROR!\n", 
            ulLBA, g_pucDataSignDisk[ulLBA], ucSignature);
        DBG_Getch();
    }

    return;
}

void FW_DataMonitorUpdateLPN(U32 ulLPN, U8 ucLPNOffset, U32 ulBufAddr)
{
    U8  ucLoop;
    U32 ulLBA;

    ulLBA     =  (ulLPN << SEC_PER_LPN_BITS);
    ulBufAddr += (ucLPNOffset << LPN_SIZE_BITS);

    for (ucLoop = 0; ucLoop < SEC_PER_LPN; ucLoop++, ulLBA++, ulBufAddr += SEC_SIZE)
    {
        FW_DataMonitorSet(ulLBA, *(U8*)ulBufAddr);
    }

    return;
}

void FW_DataMonitorResetLBARange(U32 ulStartLBA, U32 ulEndLBA)
{
    U32 ulLBA;

    for (ulLBA = ulStartLBA; ulLBA < ulEndLBA; ulLBA++)
    {
#ifdef SIM
        FW_DataMonitorSet(ulLBA, INVALID_2F);
#else
        FW_DataMonitorSet(ulLBA, 0);
#endif
    }

    return;
}

void FW_DataMonitorCheckRange(U32 ulStartLBA, U8 ucSecLength, U8 ucSecOffset, U32 ulBufAddr)
{  
    U8  ucLoop;
    U32 ulLBA;

    ulLBA     =  ulStartLBA;
    ulBufAddr += (ucSecOffset << SEC_SIZE_BITS);

    for (ucLoop = 0; ucLoop < ucSecLength; ucLoop++, ulLBA++, ulBufAddr += SEC_SIZE)
    {
        FW_DataMonitorCheck(ulLBA, *(U8*)ulBufAddr);
    }

    return;
}

void FW_DataMonitorUpdateWriteData(U8* pucWriteData, U32 ulBufAddr)
{
    U8  ucLoop;

    for (ucLoop = 0; ucLoop < SEC_PER_BUF; ucLoop++, pucWriteData++, ulBufAddr += SEC_SIZE)
    {
        *pucWriteData = *(U8*)ulBufAddr;
    }

    return;
}

void FW_DataMonitorCheckWriteData(U8* pucWriteData, U32 ulBufAddr)
{
    U8  ucLoop;

    for (ucLoop = 0; ucLoop < SEC_PER_BUF; ucLoop++, pucWriteData++, ulBufAddr += SEC_SIZE)
    {
        if (*pucWriteData != *(U8*)ulBufAddr)
        {
            DBG_Printf("FW_DataMonitorCheckWriteData Data 0x%x Wrong data 0x%x ERROR!\n", 
                *pucWriteData, *(U8*)ulBufAddr);
            DBG_Getch();
        }
    }

    return;
}

void FW_DataMonitorSetWriteNFCCheck(U8 ucPU, U8 ucWtPtr, U16 usPhyBufID)
{
    U8  ucSecIndex;
    U32 ulBufAddr;

    ulBufAddr = COM_GetMemAddrByBufferID(usPhyBufID, TRUE, BUF_SIZE_BITS);

    for (ucSecIndex = 0; ucSecIndex < SEC_PER_BUF; ucSecIndex++, ulBufAddr += SEC_SIZE)
    {
        HAL_DataCheckSetValue(ucPU, ucWtPtr, *(U8*)ulBufAddr, ucSecIndex);
    }

    return;
}

void FW_DataMonitorSetReadNFCCheck(U8 ucPU, U8 ucWtPtr, U8 ucSecLen, U32 ulStartLBA)
{
    U8  ucSecIndex;

    for (ucSecIndex = 0; ucSecIndex < ucSecLen; ucSecIndex++)
    {
        HAL_DataCheckSetValue(ucPU, ucWtPtr, FW_DataMonitorGet(ulStartLBA + ucSecIndex), ucSecIndex);
    }

    return;
}

#endif

/********************** FILE END ***************/

