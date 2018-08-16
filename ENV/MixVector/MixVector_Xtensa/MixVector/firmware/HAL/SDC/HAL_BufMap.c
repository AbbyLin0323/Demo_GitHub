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
Filename    :HAL_BufMap.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    18:18:23
Description :BufMap related functions define
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
20141204    Gavin    clean code. add some description
20150108    Gavin    add spin-lock protection for BuffMap register in Multi-core mode
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_HostInterface.h"
#include "HAL_GLBReg.h"
#include "HAL_BufMap.h"
#include "HAL_SataIO.h"
#include "HAL_MultiCore.h"
#ifdef SIM
#include "windows.h"
#include "simsatadev.h"
extern CRITICAL_SECTION g_BUFMAPCriticalSection;
#endif

/*------------------------------------------------------------------------------
Name: HAL_ResetBuffMap
Description: 
    initialize HW buff map module
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in boot/init.
History:
    20141204  Gavin   add fucntion header
------------------------------------------------------------------------------*/
void HAL_ResetBuffMap(void)
{
    rGlbSoftRst |= R_RST_BUFM; 
    rGlbSoftRst &= ~R_RST_BUFM;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HoldBuffMap
Description: 
    hold HW buff map module
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in start of error handling.
History:
    20141204  Gavin   add fucntion header
------------------------------------------------------------------------------*/
void HAL_HoldBuffMap(void)
{
    rGlbSoftRst |= R_RST_BUFM;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_ReleaseBuffMap
Description: 
    release HW buff map module
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in end of error handling.
History:
    20141204  Gavin   add fucntion header
------------------------------------------------------------------------------*/
void HAL_ReleaseBuffMap(void)
{
    rGlbSoftRst &= ~R_RST_BUFM;
    
    return;
}

/****************************************************************************
Name        :HAL_SetFirstReadDataReady
Input       :U8 ucCmdTag
Output      :
Author      :HenryLuo
Date        :2012.02.14    19:12:04
Description :Set the first read data to ready. For each read operation, the first read data status
will be set to ready if data arrive or ready by MCU(data already in DRAM) or NFC
(data read from NFC), so that SATA can schedule the host command and start doing
data transfer.
Others      :The first read data ready status bit will be clear by SATA hardware, we  do not need
to clear it.
Modify      :
****************************************************************************/
GLOBAL void HAL_SetFirstReadDataReady(U8 ucCmdTag)
{
#ifdef SIM
    SDC_SetFirstReadDataReady(((1<<7) | ucCmdTag));
#else
    rReadDataStatusInit = (1<<7) | ucCmdTag;
#endif
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SetLastDataReady
Description: 
    In VT3514 design, write comamnd may execute out of order, to avoid write buffer dead-lock,
    add last data ready mechnism.
    if pending command > 0, SDC only process comamnd with first&last data ready both set
Input Param:
    U8 ucCmdTag: command tag
Output Param:
    none
Return Value:
    none
Usage:
    called when finish building sata DSG chain.
    last data ready can be discarded if HW treat first data ready as "FIFO"
History:
    20141204  Gavin   add fucntion header
------------------------------------------------------------------------------*/
GLOBAL void HAL_SetLastDataReady(U8 ucCmdTag)
{
#ifdef SIM
    SDC_SetLastDataReady(((1<<7) | ucCmdTag));
#else
    rLastDataStatusInit = (1<<7) | ucCmdTag;
#endif
    return;
}

/****************************************************************************
Name        :HAL_SetBufMapInitValue
Input       :U8 ucBufMapId, target buffer map for set
U8 ucInitValue, the value will be filled into target buffer map
Output      :None
Author      :HenryLuo
Date        :2012.02.14    19:13:48
Description :Set the initialize value of buffer map.
Others      :
Modify      :
****************************************************************************/
GLOBAL void HAL_SetBufMapInitValue(U8 ucBufMapId, U32 ulInitValue)
{
#ifdef SIM
    SDC_SetReadBufferMapWin(ucBufMapId,ulInitValue);
#else
    U32 ulInitValueL;
    U32 ulInitValueH;

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_BUFMAP);

    /* Buffer map set low 16 bits */
    ulInitValueL = ulInitValue & 0xffff;

    // bit[23:16] = mask bits. 0 = set, 1 = don't set.

    // bit[7:6] = 10'b, Setting low 8 bit of U16
    rBufMapSet = (0 << 16) | ((ulInitValueL & 0x000000ff) << 8) | (2 << 6) | (ucBufMapId & 0x3f);
    // bit[7:6] = 11'b, Setting high 8 bit of U16
    rBufMapSet = (0 << 16) | (ulInitValueL & 0x0000ff00) | (3 << 6) | (ucBufMapId & 0x3f);

    /* Buffer map set high 16 bits */
    ulInitValueH = ulInitValue >> 16;
    rBufMapSetH = (0 << 16) | ((ulInitValueH & 0x000000ff) << 8) | (2 << 6) | (ucBufMapId & 0x3f);
    rBufMapSetH = (0 << 16) | (ulInitValueH & 0x0000ff00) | (3 << 6) | (ucBufMapId & 0x3f);

    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_BUFMAP);
    
    return;
#endif
}

/****************************************************************************
Name        :HAL_GetBufMapValue
Input       :U8 ucBufMapId, target buffer map for read
Output      :buffer map value
Author      :HenryLuo
Date        :2012.02.14    19:14:35
Description :Get the buffer map value of target buffer map ID
Others      :
Modify      :
    2013.11.05  HavenYang   modified according to the new AISC design.
****************************************************************************/
GLOBAL    U32 HAL_GetBufMapValue(U8 ucBufMapId)
{
#ifdef SIM
    return SDC_GetReadBufferMap(ucBufMapId);
#else
    U32 ulBufMapRegVal;

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_BUFMAP);

    //step1: set contex. bit[7:6] = 0X'b, means read buffmap; 11'b=write buffmap, 10'b=1st ready
    rBufMapGet = (GETBM_READBUFMAP << 6) | (ucBufMapId & 0x3f);
    //step2: return value
    ulBufMapRegVal = rBufMapGet;

    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_BUFMAP);

    return ulBufMapRegVal;
#endif
}

/****************************************************************************
Name        :HAL_UpdatetBufMapValue
Input       :U8 ucBufMapId, target buffer map for read
Output      :buffer map value
Author      :HenryLuo
Date        :2012.02.14    19:14:35
Description :Get the buffer map value of target buffer map ID
Others      :
Modify      :
****************************************************************************/
GLOBAL    void  HAL_UpdateBufMapValue(U8 ucBufMapId, U32 value)
{
    U32   ulBufMapValueOld; 
    U32   ulBufMapValueNew;

#ifdef SIM
    EnterCriticalSection(&g_BUFMAPCriticalSection);
    ulBufMapValueOld = SDC_GetReadBufferMap(ucBufMapId);
    ulBufMapValueNew = value | ulBufMapValueOld;
    SDC_SetReadBufferMapWin(ucBufMapId, ulBufMapValueNew);    
    LeaveCriticalSection(&g_BUFMAPCriticalSection);
#else
    ulBufMapValueOld = HAL_GetBufMapValue(ucBufMapId);
    ulBufMapValueNew = value | ulBufMapValueOld;
    HAL_SetBufMapInitValue(ucBufMapId, ulBufMapValueNew);
#endif
    return;
}

/****************************************************************************
Name        :HAL_SetWriteBufMap
Input       :U8 ucBufMapId, target buffer map for set
U8 ulValue
Output      :None
Author      :HenryLuo
Date        :2012.02.14    19:15:51
Description :Set the initialize value of buffer map.
Others      :
Modify      :
****************************************************************************/
GLOBAL    void HAL_SetWriteBufMap(U8 ucBufMapId, U32 ulValue)
{
    rWriteBufMapSet = ((ulValue & 0x3) << 6) | (ucBufMapId);
    return;
}

/****************************************************************************
Name        :HAL_GetWriteBufMapValue
Input       :U8 ucBufMapId, target buffer map for read
Output      :buffer map value
Author      :HenryLuo
Date        :2012.02.14    19:16:22
Description :Get the buffer map value of target buffer map ID
Others      :
Modify      :
****************************************************************************/
GLOBAL    U32 HAL_GetWriteBufMapValue(U8    ucBufMapId)
{
    U32 ulBufMapRegVal;

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_BUFMAP);

    rBufMapGet = (0x3 << 6) | ucBufMapId;
    ulBufMapRegVal = rBufMapSet;

    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_BUFMAP);

    return    ulBufMapRegVal;
}

/****************************************************************************
Name        :HAL_WriteBufMapIsFree
Input       :U8 ucBufMapId, target buffer map for read
Output      :write buffer map is free
Author      :HenryLuo
Date        :2012.02.14    19:17:00
Description :whether the NFC is finish to use buffer map 
Others      :
Modify      :
****************************************************************************/
GLOBAL  U32 HAL_WriteBufMapIsFree(U8 ucBufMapId)
{
    U32 BufMapValue;

    BufMapValue = HAL_GetWriteBufMapValue(ucBufMapId);

    if((WRITE_BUFMAP_IDLE == BufMapValue) || (WRITE_BUFMAP_NFC == BufMapValue))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*==============================================================================
Func Name  : HAL_GetSdcCurrentFDR
Input      : void
Output     : NONE
Return Val : 0~31: command tag of host command whose FisrtDataReady is being processed
             by SDC.
             0xFF: current FDR FIFO is empty
Discription: Fisrt Data Ready information is sent to SDC by FIFO, this function return
             current host command tag whose FDR is being processed by SDC.
Usage      : 
            For HW debug only. Not supported in simulation ENV.
History    : 
    1. 2013.11.5 Haven Yang create function
    2. 2014.12.8 modify function according to HW behaviour
==============================================================================*/
GLOBAL U8  HAL_GetSdcCurrentFDR(void)
{
    U8 ucTagIndex;
    U32 ulFDRBitMap;

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_BUFMAP);

    rBufMapGet = (GETBM_FIRSTDATAREADY << 6);

    ulFDRBitMap = rResultGet;

    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_BUFMAP);

    for (ucTagIndex = 0; ucTagIndex < MAX_SLOT_NUM; ucTagIndex++)
    {
        if (0 != (ulFDRBitMap & (1 << ucTagIndex)))
        {
            return ucTagIndex;
        }
    }

    return INVALID_2F;
}
/* end of file HAL_BufMap.c */

