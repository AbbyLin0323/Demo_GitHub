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
* File Name    : MixVector_Flash.h
* Discription  : common interface for read/write/erase command to flash
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef __MIX_VECTOR_FLASH_H__
#define __MIX_VECTOR_FLASH_H__

#include "HAL_MemoryMap.h"
#include "MixVector_Interface.h"

#define SSU1_NUM_PER_SLOT   FLASH_ADDR_CNT
#define SSU_BUSY 1
#define SSU_FREE 0

/* sgq direction */
#define SGQ_HOST_TO_FLASH  1
#define SGQ_FLASH_TO_HOST  0

#define DISK_C_UNIT (2)
/* monitor of request to NFC.
FW use this to report error status and retry queuing comamnd */
#define NFC_PU_HW_LEVEL 2
typedef struct _FLASH_REQ_MONITOR
{
    U32 LastTrigLevel: 1;
    U32 Dw0Rsvd: 31;

    FLASH_REQ_FROM_HOST FlashReq[NFC_PU_HW_LEVEL];
}FLASH_REQ_MONITOR;


typedef struct _FLASH_REQ_HSG_MONITOR
{
    /* DWORD 0 */
    U32 HostAddrLow;

    /* DWORD 1 */
    U32 HostAddrHigh;

    /* DWORD 2 */
    U32 LBA;

    /* DWORD 3 */
    U32 StartUnitInByte;

    /* DWORD 3 */
    U32 UnitLenthInByte;

}FLASH_REQ_HSG_MONITOR;



extern BOOL CheckSsuBusy(U8 ucCmdSlot);
extern void ProcessNfcError(U8 ucPU);
extern BOOL ProcessDiskC(U8 ucCmdSlot);
//Flash <-> host path interface
extern BOOL NFC_BuildHostWriteReq(FLASH_REQ_FROM_HOST *pHostReq);
extern BOOL NFC_BuildHostReadReq(FLASH_REQ_FROM_HOST *pHostReq,BOOL bSplit);
extern BOOL NFC_BuildHostEraseReq(FLASH_REQ_FROM_HOST *pHostReq);
extern U32 DiskC_GetHsgLenth(U32 ucCmdSlot);

extern BOOL l_bNfcErr;
extern U8 l_ucErrPU;
extern U8 *g_pSSU1;
extern FLASH_REQ_HSG_MONITOR g_DiskCHSGMonitor[MAX_SLOT_NUM];

void DiskC_SetMonitor(U8 Pu ,U8 Wp ,FLASH_REQ_FROM_HOST *pHostReq);

#endif/* __MIX_VECTOR_FLASH_H__ */

