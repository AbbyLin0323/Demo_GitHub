/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_BufMap.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    18:18:23
Description :BufMap related functions define
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_Define.h"
#include "HAL_BufMap.h"

#ifdef SIM
#include "windows.h"
#include "..\satamodel\simsatadev.h"
extern CRITICAL_SECTION g_BUFMAPCriticalSection;
#endif



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
        //[20090709]end
#endif
}
/*
    In VT3514 design, write comamnd may execute out of order, to avoid write buffer dead-lock,
    add last data ready mechnism.
    if pending command > 0, SDC only process comamnd with first&last data ready both set
Gavin, 20130923
*/
GLOBAL void HAL_SetLastDataReady(U8 ucCmdTag)
{
#ifdef SIM
        SDC_SetLastDataReady(((1<<7) | ucCmdTag));
#else
        rLastDataStatusInit = (1<<7) | ucCmdTag;
#endif
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
    2013.11.05  HavenYang   modified according to the new AISC design.
    2014.01.03  JasonGuo    support 128k/64k/32k/16k/8k/4k buffer size
****************************************************************************/
GLOBAL void HAL_SetBufMapInitValue(U8 ucBufMapId, U32 ulInitValue)
{
#ifdef SIM
    SDC_SetReadBufferMapWin(ucBufMapId,ulInitValue);
#else
    U32 ulInitValueL;
    U32 ulInitValueH;

    /* Buffer map set low 16 bits */
    ulInitValueL = ulInitValue & 0xffff;
    // bit[7:6] = 0X'b, clear all 32 bits
    // bit[7:6] = 10'b, Setting low 16 bit
    //rBufMapSet = ((ulInitValue & 0x0000ffff) << 8) | (2 << 6) | (ucBufMapId);
    // bit[7:6] = 11'b, Setting high 16 bit
    //rBufMapSet = ((ulInitValue & 0xffff0000) >> 8) | (3 << 6) |(ucBufMapId);

    // bit[23:16] = mask bits. 0 = set, 1 = don't set.
    // bit[7:6] = 10'b, Setting low 8 bit
    rBufMapSet = (0 << 16) | ((ulInitValueL & 0x000000ff) << 8) | (2 << 6) | (ucBufMapId & 0x3f);
    // bit[7:6] = 11'b, Setting high 8 bit
    rBufMapSet = (0 << 16) | (ulInitValueL & 0x0000ff00) | (3 << 6) | (ucBufMapId & 0x3f);

    /* Buffer map set high 16 bits */
    ulInitValueH = ulInitValue >> 16;
    rBufMapSetH = (0 << 16) | ((ulInitValueH & 0x000000ff) << 8) | (2 << 6) | (ucBufMapId & 0x3f);
    rBufMapSetH = (0 << 16) | (ulInitValueH & 0x0000ff00) | (3 << 6) | (ucBufMapId & 0x3f);
    
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
    //step1: set contex. bit[7:6] = 0X'b, means read buffmap; 11'b=write buffmap, 10'b=1st ready
    rBufMapGet = (GETBM_READBUFMAP << 6) | (ucBufMapId & 0x3f);
    //step2: return value
    return    (U32)rBufMapGet;
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
    return;
#else
    ulBufMapValueOld = HAL_GetBufMapValue(ucBufMapId);
    ulBufMapValueNew = value | ulBufMapValueOld;
    HAL_SetBufMapInitValue(ucBufMapId, ulBufMapValueNew);
    //HAL_UpdateBufferMap((U8)~value, ucBufMapId, value);
    return;
#endif
}


/*==============================================================================
Func Name  : HAL_UpdateBufferMap
Input      : U8 ucMask : bitmap, b0:need update, b1: don't need update         
             U8 ucBufMapID      
             U32 ulBufMapValue  buffer map value to be set
Output     : NONE
Return Val : NONE
Discription: Update read buffer map, according to the mask & value.
             there are 8 bits in Mask, witch bit value is 0 means this map shall be updated.
             and bit value is 1 means not need to update.
Usage      : 
History    : 
    1. 2013.11.15 Haven Yang create function
==============================================================================*/
GLOBAL void HAL_UpdateBufferMap(U8 ucMask, U8 ucBufMapID, U32 ulBufMapValue)
{
#ifdef SIM    
    U32   ulBufMapValueOld; 
    U32   ulBufMapValueNew;
    
    EnterCriticalSection(&g_BUFMAPCriticalSection);
    ulBufMapValueOld = SDC_GetReadBufferMap(ucBufMapID);
    ulBufMapValueNew = ulBufMapValue | ulBufMapValueOld;
    SDC_SetReadBufferMapWin(ucBufMapID, ulBufMapValueNew);    
    LeaveCriticalSection(&g_BUFMAPCriticalSection);
    
#else

    //ulBufMapValueOld = HAL_GetBufMapValue(ucBufMapID);
    //ulBufMapValueNew = ulBufMapValue | ulBufMapValueOld;
    //HAL_SetBufMapInitValue(ucBufMapID, ulBufMapValueNew);

    // bit[23:16] = mask bits. 0 = set, 1 = don't set.
    // bit[7:6] = 10'b, Setting low 8 bit
    rBufMapSet = ((U32)ucMask << 16) | ((ulBufMapValue & 0x000000ff) << 8) | (2 << 6) | (ucBufMapID & 0x3f);
    // bit[7:6] = 11'b, Setting high 8 bit
    rBufMapSet = ((U32)ucMask << 16) | (ulBufMapValue & 0x0000ff00) | (3 << 6) | (ucBufMapID & 0x3f);
    
#endif
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
    rBufMapGet = (0x3 << 6) | ucBufMapId;
    return    (U32)rBufMapSet;
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
GLOBAL    U32 HAL_WriteBufMapIsFree(U8    ucBufMapId)
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

/****************************************************************************
Name        :HAL_WriteBufMapTest
Input       :void
Output      :NONE
Author      :HenryLuo
Date        :2012.02.14    19:17:38
Description :test write bufmap read & write.
Others      :
Modify      :
****************************************************************************/
GLOBAL    void HAL_WriteBufMapTest(void)
{
    U32 BufMapValue;
    U8 i;


    for(i = 0; i < 64; i++)
    {
        HAL_SetWriteBufMap(i, WRITE_BUFMAP_IDLE);
        BufMapValue = HAL_GetWriteBufMapValue(i);
        if(WRITE_BUFMAP_IDLE != BufMapValue)
        {

        }
    }

    for(i = 0; i < 64; i++)
    {
        HAL_SetWriteBufMap(i, WRITE_BUFMAP_SATA);
        BufMapValue = HAL_GetWriteBufMapValue(i);
        if(WRITE_BUFMAP_SATA != BufMapValue)
        {

        }
    }

    for(i = 0; i < 64; i++)
    {
        HAL_SetWriteBufMap(i, WRITE_BUFMAP_NFC);
        BufMapValue = HAL_GetWriteBufMapValue(i);
        if(WRITE_BUFMAP_NFC != BufMapValue)
        {

        }
    }
}


/*==============================================================================
Func Name  : HAL_GetFirstDataReadyStatus
Input      : U8 ucCmdTag :Host Command Tag.
Output     : NONE
Return Val : 0 = not ready; 1 = ready
Discription: Get Sata command first data ready status of ucCmdTag.
Usage      : 
History    : 
    1. 2013.11.5 Haven Yang create function
==============================================================================*/
U8  HAL_GetFirstDataReadyStatus(U8 ucCmdTag)
{
#ifdef SIM
        return SDC_IsFirstReadDataReady(ucCmdTag);
#else
        rBufMapGet = (GETBM_FIRSTDATAREADY << 6) | (ucCmdTag & 0x3f);

        return  rBufMapGet;
#endif
}

/*==============================================================================
Func Name  : HAL_GetLastDataReadyStatus
Input      : U8 ucCmdTag  :Host Command Tag.
Output     : NONE
Return Val : 0 = not ready; 1 = ready(it means firmware process this sata command done)
Discription: Get Sata command Last data ready status of ucCmdTag.
Usage      : 
History    : 
    1. 2013.11.5 Haven Yang create function
==============================================================================*/
U8  HAL_GetLastDataReadyStatus(U8 ucCmdTag)
{
#ifdef SIM
        return SDC_IsLastDataReady(ucCmdTag);
#else
        rBufMapGet = (GETBM_LASTDATAREADY << 6) | (ucCmdTag & 0x3f);

        return  rBufMapGet;
#endif
}



