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
  File Name     : swlib.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines api interface declaration
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#ifndef _SWLIB
#define _SWLIB

#include "host_api_define.h"
#include "plx_lib.h"

VOID	Plx_Test(U32 val,U32 ulAddr);
BOOL	Plx_Read_Register(PPCIE_DEVICE pDevObj,U32 ulRegAddr,U32 *pRegData);
BOOL	Plx_Write_Register(PPCIE_DEVICE pDevObj, U32 ulRegAddr, U32 ulRegData);
BOOL	Plx_Jump(PPCIE_DEVICE pDevObj, U32 ulAddr);
VOID	Plx_Enum_Dev(U8 * pDevNum);
BOOL	Plx_Power_On(U8 ucBus,U8 ucSlot,U8 ucFun);
BOOL	Plx_Power_Off(U8 ucBus,U8 ucSlot,U8 ucFun);
BOOL Plx_Disable_Int(U8 ucBus,U8 ucSlot,U8 ucFun);
BOOL Plx_Enable_Int(U8 ucBus,U8 ucSlot,U8 ucFun);
VOID Plx_Scan_Hw();
extern "C" __declspec(dllexport) BOOL Plx_Rescan_Hw();
extern "C" __declspec(dllexport) VOID Plx_Get_Bridge_Ep_Handle(U8 ucEpIndex,VOID ** HANDLE);
extern "C" __declspec(dllexport) VOID  Plx_Enum_Bridge_Ep(unsigned char * pDevNum);
extern "C" __declspec(dllexport) VOID Plx_Power_Off_Fast(VOID *HANDLE,BOOL bJtagEnable);
extern "C" __declspec(dllexport) VOID Plx_Disable_Int_Fast(VOID *HANDLE);
extern "C" __declspec(dllexport) BOOL Plx_Power_Status(void * Handle);
extern "C" __declspec(dllexport) VOID Plx_Read_Fast(VOID *HANDLE,U32 * pRegVal);
extern "C" __declspec(dllexport) VOID Plx_Power_On_Fast(VOID *HANDLE,BOOL bJtagEnable);

extern "C" __declspec(dllexport) VOID Plx_Enable_Int_Fast(VOID *HANDLE);
VOID Plx_Get_Pcie_Pos(U8 ucDevIndex,U8 *bus,U8 * slot,U8 * fun);
U32 jtag_power_control_init();
U8		Api_Disk_Detect_Rom(DISKTYPE ucMode);
U8		Api_Disk_Detect(DISKTYPE ucMode);
VOID	Api_Disk_Name();
U32		Api_Flash_Get_Status_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu);
STATUS	Api_Read_Dram_Sram(PDEVICE_OBJECT pdevobj,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf);
STATUS	Api_Write_Dram_Sram(PDEVICE_OBJECT pdevobj,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf);
STATUS	Api_Read_Flash(PDEVICE_OBJECT pDevObj,U8 ucPln,U16 usBlock,U16 usPage,U32 ulPuMskLow,U32 ulPuMskHigh);
STATUS	Api_Write_Flash(PDEVICE_OBJECT pdevobj,U8 pln,U16 block,U16 page,U32 pumsklow,U32 pumskhigh);
STATUS	Api_Erase_Flash(PDEVICE_OBJECT pDevObj,U8 ucPln,U16 usBlock,U32 ulPuMskLow,U32 ulPuMskHigh);
STATUS	Api_Read_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 * pRegData);
STATUS	Api_Write_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 ulRegData);
STATUS	Api_NoneData(PDEVICE_OBJECT pDevObj,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2);
STATUS	Api_Flash_Get_Cmd_Status(PDEVICE_OBJECT pDevObj,U32 ulPuMskLow,U32 ulPuMskHigh,U32 *pPuMskLowStatus,U32* pPuMskHighStatus);
STATUS Api_Updata_Fw(PDEVICE_OBJECT pDevObj, U32 ulCnt, U8 * pFwBuf);
STATUS Api_Set_Uart_Baudrate(PDEVICE_OBJECT pDevObj, U32 ulSpeedDev, U32 ulSpeedHost);
STATUS	Api_Ext_Mem_Write(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData);
STATUS	Api_Ext_Mem_Read(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData);

STATUS Api_Dma_Read(PDEVICE_OBJECT pdevobj, U32 lba, U8 * inbuf_addr, U32 trans_sec_cnt);
STATUS Api_Dma_Write(PDEVICE_OBJECT pdevobj, U32 lba, U8 * outbuf_addr, U32 trans_sec_cnt);

STATUS	Api_Flash_Get_Data_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 ** pDataBuf);
STATUS  Api_Get_SmartInfo(PDEVICE_OBJECT pDevObj, U8 * pDataBuf);
U32 Api_DwordToString(U32 ulInputDword, U8 *pOutputString);
void Api_ShowDate(U32 ulInputDate);
void Api_ShowTime(U32 ulInputTime);
STATUS	Api_Flash_Set_Data_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 * pDataBuf);
STATUS	Api_Flash_Get_Red_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED ** pRed);
STATUS	Api_Flash_Set_Red_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED * pRed);
STATUS	Api_Ext_Get_Var_Table_Addr(PDEVICE_OBJECT pDevObj,U32 * pVarTableAddr);
STATUS	Api_Clear_DiskLock(PDEVICE_OBJECT pDevObj,U32 * pVarTableAddr);
STATUS Api_Read_SpiFlash(PDEVICE_OBJECT pDevObj,U32 ulSrcAddr,U32 ulDstAddr,U32 ulDataBlockNum,U32 * pVarTableAddr);
STATUS Api_Write_SpiFlash(PDEVICE_OBJECT pDevObj,U32 ulSrcAddr,U32 ulDstAddr,U32 ulDataBlockNum,U32 * pVarTableAddr);
VOID	Api_Init_FlashInfo(PDEVICE_OBJECT pDevObj);
VOID    Api_Ext_InitFlashSLCMapping(PDEVICE_OBJECT pDevObj);

STATUS Api_Ext_TraceLogControl(PDEVICE_OBJECT pDevObj,CPUID MCUID,TL_CTL tlCtl,U16 secCount);
STATUS Api_Ext_Get_TraceLogInfo(PDEVICE_OBJECT pDevObj,TL_INFO *pTraceLogInfoBuf);
STATUS Api_Ext_Get_TraceLogData(PDEVICE_OBJECT pDevObj,FILE *fLogFile,CPUID MCUID,TL_INFO *pTraceLogInfoBuf);
STATUS Api_Ext_GetTraceLog(PDEVICE_OBJECT pDevObj,char *pReportFileDir);
STATUS Api_Ext_Get_TraceLogReport(PDEVICE_OBJECT pDevObj,char *pReportFileDir,char *pFormatFile0,char *pFormatFile12,char *pTraceLogHeaderFile);
STATUS Api_Ext_Get_Pu_Msk(PDEVICE_OBJECT pDevObj,U32 * ulPuMskLow,U32 * ulPuMskHigh,U8 *ucLogicPuCnt);
STATUS Api_Ext_Get_SubSys_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 *ucSubSys,U8 *ucSubSysPuNum);
STATUS Api_Ext_Bbt_Save(PDEVICE_OBJECT pDevObj,U8 ucCpuId);
STATUS Api_Ext_Bbt_Load(PDEVICE_OBJECT pDevObj,U8 ucCpuId);
STATUS Api_Ext_L3_Llf(PDEVICE_OBJECT pDevObj,U8 ucCpuId);
STATUS Api_Ext_L2_Llf(PDEVICE_OBJECT pDevObj,U8 ucCpuId);
STATUS Api_Ext_Get_HostInfo(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_Get_DeviceInfo(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_Get_FlashMonitorInfo(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_Get_PBITInfo(PDEVICE_OBJECT pDevObj, char *pReportFileDir);
STATUS Api_Ext_Dbg_ShowAll(PDEVICE_OBJECT pDevObj, U8 ucCpuId);
int Api_Ext_Get_SysInfo(PDEVICE_OBJECT pDevObj, char *pReportFileDir);
STATUS Api_Ext_HwDebug_Log_Decode(PDEVICE_OBJECT pDevObj, char *pReportFileDir);
STATUS Api_Ext_HwDebug_Log_Decode_Bin(PDEVICE_OBJECT pDevObj, char *pReportFileDir);
STATUS Api_Rom_Write_Register(void *pDetectObj,U32 ulAddr,U32 ulData);
STATUS Api_Rom_Read_Register(void *pDetectObj,U32 ulAddr,U32 *pData);
STATUS Api_Rom_Read_Status(void *pDetectObj);
STATUS Api_Rom_Jump(void *pDetectObj,U32 ulAddr);
STATUS Api_Rom_Write_Dma(void *pDetectObj,U32 ulAddr, U32 * pBuf,U32 ulCnt);
STATUS Api_Rom_Close_Handle(void * pDetectObj);
STATUS Api_Rom_Set_Trans_Speed(void *pDetectObj,U32 ulHostSpeed);
STATUS Api_Rom_Set_Uart_Baudrate(void *pDetectObj, U32 ulSpeedDev, U32 ulSpeedHost);
void * Api_Get_Rom_Obj(U8 ucIndex);

void Api_Ext_ReadTraceLog_Flash(PDEVICE_OBJECT pDevObj,char *pReportFileDir,char *pFormatFile0,char *pFormatFile12,char *pTraceLogHeaderFile);
void Api_Ext_ChangeTraceRecord(PDEVICE_OBJECT pDevObj);
void Api_Ext_Read_Flash_Addr(PDEVICE_OBJECT pDevObj,char* pReportFileDir);
void Api_Ext_Read_Fw_Runtime_Info(PDEVICE_OBJECT pDevObj);
void Flash_Interface_Test(PDEVICE_OBJECT pDevObj);
STATUS Api_Flash_PreCondiTion(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh);
STATUS Api_Flash_Reset(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh);
STATUS Api_Flash_SetParam(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh, U32 ulParam);
STATUS Api_Flash_Terminate(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh);

int mpt_main(int argc, _TCHAR* argv[]);
int logtrace_main(int argc, _TCHAR* argv[]);
int flash_main(int argc, _TCHAR* argv[]);
int plx_main(int argc, _TCHAR* argv[]);
int pcie_enum_main(int argc, _TCHAR* argv[]);
int sata_rom_test_main(int argc, _TCHAR* argv[]);
int uart_rom_test_main(int argc, _TCHAR* argv[]);
int fw_update_main(int argc, _TCHAR* argv[]);
#endif
