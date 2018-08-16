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
  File Name     : host_api.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the basic function of host access to device.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#ifndef _HOSTAPI
#define _HOSTAPI
#include "host_api_define.h"


#include <minwindef.h>
#include "ntddscsi.h"

//#include "host_flash_info.h"
STATUS Api_Read_Dram_Sram(PDEVICE_OBJECT pdevobj,U8 subsysid,U32 startaddr,U32 length,U8 * readbuf);
STATUS Api_Write_Dram_Sram(PDEVICE_OBJECT pdevobj,U8 subsysid,U32 startaddr,U32 length,U8 * writebuf);
STATUS Api_Read_Flash(PDEVICE_OBJECT pdevobj,U8 pln,U16 block,U16 page,U32 pumsklow,U32 pumskhigh);
STATUS Api_Write_Flash(PDEVICE_OBJECT pdevobj,U8 pln,U16 block,U16 page,U32 pumsklow,U32 pumskhigh);
STATUS Api_Erase_Flash(PDEVICE_OBJECT pdevobj,U8 pln,U16 block,U32 pumsklow,U32 pumskhigh);
STATUS Api_Read_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 * pRegData);
STATUS Api_Write_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 ulRegData);
STATUS Api_NoneData(PDEVICE_OBJECT pDevObj,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2);
STATUS Api_Flash_Get_Cmd_Status(PDEVICE_OBJECT pDevObj,U32 ulPuMskLow,U32 ulPuMskHigh,U32 *pPuMskLowStatus,U32* pPuMskHighStatus);
U8 Api_Caculate_Pu_Cnt(U32 ulPuMsk);
STATUS Api_Get_Identify_Data(PDEVICE_OBJECT pdevobj,U8 *readbuf);
STATUS Api_Get_ModelNumber(PDEVICE_OBJECT pdevobj,U8 * identifybuf,U8 mn[40]);

void Api_Init_FlashInfo(PDEVICE_OBJECT pDevObj);
void Api_Init_Dev_Function(PDEVICE_OBJECT pdevobj,DISKTYPE disktype);
void Api_Flash_Get_Pu_Infor(PDEVICE_OBJECT pdevobj,U8 logicpu,PLOGIC_PU_ENTRY pflashinfo);
void Api_LogicPumsk_To_SubSysPumsk(PDEVICE_OBJECT pdevobj,U32 ulPuMskLow,U32 ulPuMskHigh,U32 *pSubSysPuMsk);
U32 Api_Flash_Get_Status_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu);
U32 Api_DwordToString(U32 ulInputDword, U8 *pOutputString);
void Api_ShowDate(U32 ulInputDate);
void Api_ShowTime(U32 ulInputTime);
STATUS  Api_Get_SmartInfo(PDEVICE_OBJECT pDevObj, U8 * pDataBuf);
STATUS Api_Flash_Get_Data_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 ** pDataBuf);
STATUS Api_Flash_Set_Data_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 * pDataBuf);
STATUS Api_Flash_Get_Red_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED ** pRed);
STATUS Api_Flash_Set_Red_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED * pRed);

STATUS Api_Flash_PreCondiTion(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh);
STATUS Api_Flash_Reset(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh);
U16 Api_Rom_Gen_Crc(U16 * pDataAddr,U32 ulLen);
U32 Api_Data_Gen_Crc(U32 * pDataAddr,U32 ulLen);
#endif