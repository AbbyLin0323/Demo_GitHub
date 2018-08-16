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
* File Name    : MixVector_SRAM.h
* Discription  : interface for fetch host commnad and write back status;
                 common interface for read/write HCT SRAM
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef __MIX_VECTOR_SRAM_H__
#define __MIX_VECTOR_SRAM_H__

#include "HAL_MemoryMap.h"
#include "HAL_HCT.h"
#include "MixVector_Interface.h"

#ifdef SIM
#include "AHCI_ControllerInterface.h"
#include "AHCI_ControllerModel.h"
#include "Sim_HCT.h"

#define FCQ_REG_WRITE_TRIGGER           SIM_HostCFCQWriteTrigger()
#define SET_CMD_CST( _uTag_, _uCST_)    HCT_SetCSTByFw( _uTag_, 0, _uCST_ )
#define SEARCH_CST_TRIGGER              HCT_SearchCST()
#define HW_RESET_OPT                    HCT_ResetOPT()
#else
#define FCQ_REG_WRITE_TRIGGER           {rHCT_FCQ_REG.ulAsU32 = fcqReg.ulAsU32;}
#define SET_CMD_CST( _uTag_, _uCST_)     { rHCT_CS_REG[_uTag_] = (U8)_uCST_;}
#define SEARCH_CST_TRIGGER
#define HW_RESET_OPT
#endif // SIM

/* length for save all HOST_CMD/FCQ/WBQ/DiskA. HOST_CMD is stored in S0 */
#define HCT_HCMD_LENGTH     ( sizeof( HOST_CMD ) * MAX_SLOT_NUM )   //lenth 0xA00 for 4CE
#define HCT_FCQ_LENGTH      ( sizeof( HCT_FCQ_WBQ )   * MAX_SLOT_NUM )
#define HCT_WBQ_LENGTH      ( sizeof( HCT_FCQ_WBQ )   * WBQ_N * MAX_SLOT_NUM )
#define HCT_DISK_A_LENGTH   ( 24 * 1024 )

/* base address for HOST_CMD/FCQ/WBQ/DiskA */
#define HCT_HCMD_BASE         ( HCT_DSRAM_BASE_ADDRESS  )
#define HCT_FCQ_BASE        ( HCT_DSRAM_BASE_ADDRESS + HCT_HCMD_LENGTH )
#define HCT_WBQ_BASE        ( HCT_DSRAM_BASE_ADDRESS + HCT_HCMD_LENGTH + HCT_FCQ_LENGTH )
#define HCT_DISK_A_BASE     ( HCT_DSRAM_BASE_ADDRESS + HCT_HCMD_LENGTH + HCT_FCQ_LENGTH + HCT_WBQ_LENGTH )

#define SN_COMMAND          (0)
#define SN_DISK_A           (1)

void InitHCTReg();
void InitHCTRegB();
CST_STATUS GetCST( U8 ucCmdSlot );
void SetCST( U8 ucCmdSlot, CST_STATUS Status );

extern BOOL FetchHostCmd(U8 ucCmdSlot);
extern BOOL SetupWBQForReadingDiskA( U8 ucCmdSlot, U8 ucWbqIndex );
extern BOOL CompleteHostCmd(U8 ucCmdSlot, BOOL bWaitSge, BOOL bWaitNfc, U8 ucWbqIndex);
extern BOOL ReturnDiskADataAndCompleteHostCmd(U8 ucCmdSlot, BOOL bWaitSge, BOOL bWaitNfc);
extern BOOL ProcessWriteDiskA(U8 ucCmdSlot);
extern BOOL FCQ_BuildHostWriteReq(HOST_MEM_REQ *pHostReq);
extern void WBQ_BuildHostReadReq(HOST_MEM_REQ *pHostReq);

#endif /* __MIX_VECTOR_SRAM_H__ */

