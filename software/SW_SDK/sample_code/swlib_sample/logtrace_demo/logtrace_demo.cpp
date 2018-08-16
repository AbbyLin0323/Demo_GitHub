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
  File Name     : logtrace_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines logtrace demo function.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include "..\include\sw_lib.h"

DISKTYPE SelectDiskType()
{
    U8 ch;

    DBG_printf("0:AHCI\n");
    DBG_printf("1:NVME\n");
    DBG_printf("2:SATA\n");
    //DBG_printf("3:UART\n");
    DBG_printf("Please Input 0 - 2 to select mode:\n");
    ch = DBG_Getch();

    ch -= 0x30;
    DBG_printf("input: %d\n", ch);
    if (ch > 3)
    {
        DBG_printf("Invalid Input %d\n", ch);
        return DISK_TYPE_CNT;
    }

    return (DISKTYPE)ch;
}
DISKTYPE SelectLinkType()
{
    U8 ch;
	
    DBG_printf("0:AHCI\n");
    DBG_printf("1:NVME\n");
    DBG_printf("2:SATA\n");
    DBG_printf("3:UART\n");
    DBG_printf("Please Input 0 - 3 to select link:\n");
    ch = DBG_Getch();

    ch -= 0x30;
    DBG_printf("input: %d\n", ch);
    if (ch > 3)
    {
        DBG_printf("Invalid Input %d\n", ch);
        return DISK_TYPE_CNT;
    }

    return (DISKTYPE)ch;
}
void GetLogFormatPath(DISKTYPE eDiskType, U32 ulFlashId, char* pTraceLogHeaderFile, char* pFormatFile0, char* pFormatFile12)
{
	char aDiskType[32];
	char aFlashType[32];

	switch (eDiskType)
    {
    case AHCI:
		sprintf_s(aDiskType, 32, "AHCI");
        break;
    case NVME:
		sprintf_s(aDiskType, 32, "NVME");
        break;
    case SATA:
		sprintf_s(aDiskType, 32, "SATA");
        break;
    default:
        break;
    }

	switch (ulFlashId)
	{
	case 0x3c64842c:
		sprintf_s(aFlashType, 32, "L85");
		break;
	case 0x54e5a42c:
	case 0x5464842c:
		sprintf_s(aFlashType, 32, "L95");
		break;
	case 0x93953a98:
		sprintf_s(aFlashType, 32, "TSB");
		break;
	case 0x93a43a98:
	case 0x93a53c98:
		sprintf_s(aFlashType, 32, "TSB_FOURPLN");
		break;
	default:
        DBG_printf("Unkown flashId 0x%x\n", ulFlashId);
        DBG_Getch();
		break;
	}

	sprintf_s(pTraceLogHeaderFile, 128, ".\\input\\%s\\%s\\HAL_TraceLog.h", aDiskType, aFlashType);
	sprintf_s(pFormatFile0, 128, ".\\input\\%s\\%s\\log_format_file0.ini", aDiskType, aFlashType);
	sprintf_s(pFormatFile12, 128, ".\\input\\%s\\%s\\log_format_file12.ini", aDiskType, aFlashType);

	return;
}
void uart_baud_set(PDEVICE_OBJECT pDevObj)
{
	U32 ulHclkReg, ulHclkRegVal;
	U32 ulHclk, ulVal;
	ulHclkReg = 0x1ff81f18;
	Api_Read_Register(pDevObj, MCU0, ulHclkReg, &ulHclkRegVal);
	ulHclk = 0;
	switch ((ulHclkRegVal >> 8) & 0x3)
	{
		case 0://HCLK266
			ulHclk = 266000000;
			break;
		case 1://HCLK300
			ulHclk = 300000000;
			break;
		case 2://HCLK333
			ulHclk = 333000000;
			break;
		case 3://HCLK400
			ulHclk = 400000000;
			break;
		default:
			break;
	}
	if (ulHclk == 0)
		return;

	ulVal = (ulHclk / 1152000 / 8);
	printf("set uart reg :0x%x\n", (ulVal << 16));
	Api_Set_Uart_Baudrate(pDevObj, (ulVal<<16), 1152000);
	return;
}
int logtrace_main(int argc, _TCHAR* argv[])
{
    PDEVICE_OBJECT pDevObj;
    U32 ulVarTableAddr;
    U8 ucDiskCnt, ch;
    DISKTYPE eDiskType, eLinkType;

    char pFormatFile0[128] = ".\\input\\log_format_file0.ini";
    char pFormatFile12[128] = ".\\input\\log_format_file12.ini";
    char pTraceLogHeaderFile[128] = ".\\input\\HAL_TraceLog.h";
    char *pReportFileDir = ".\\output\\";
	Api_Disk_Name();
    while (1)
    {
        eDiskType = SelectDiskType();
		if (DISK_TYPE_CNT == eDiskType)
		{
			continue;
		}
		

		eLinkType = SelectLinkType();
        ucDiskCnt = Api_Disk_Detect(eLinkType);

        if (ucDiskCnt > 0)
        {
            DBG_printf("Please Input 0 - %d to select link:\n", (ucDiskCnt - 1));
            ch = DBG_Getch();
            ch -= 0x30;

            if (ucDiskCnt <= (U8)ch)
            {
                DBG_printf("Invalid Input %d\n", ch);
                continue;
            }
            else
            {
                //disk_init_testdisk(ch);
                //io_async_init();
                break;
            }
        }

        DBG_printf("disk detecting... \n");
        Sleep(10000);
    }

    pDevObj = &g_dev_obj[ch];

	if (eLinkType == UART)
	{
		while (1)
		{
			DBG_printf("Please Input 0 - 1 to select uart speed: 0->115200 1->1152000\n");
			ch = DBG_Getch();
			ch -= 0x30;

			if (ucDiskCnt > 1)
			{
				DBG_printf("Invalid Input %d\n", ch);
				continue;
			}
			else
			{
				break;
			}
		}
		if (ch==1)
			uart_baud_set(pDevObj);
	}
    Api_Ext_Get_Var_Table_Addr(pDevObj, &ulVarTableAddr);
    Api_Read_Dram_Sram(pDevObj, MCU0, ulVarTableAddr, HOST_BUFF_SIZE, (U8 *)&pDevObj->VarTable);

    Api_Init_FlashInfo(pDevObj);
    Api_Ext_InitFlashSLCMapping(pDevObj);

	GetLogFormatPath(eDiskType, pDevObj->VarTable.SubSysTable[0].ulFlashId[0], pTraceLogHeaderFile, pFormatFile0, pFormatFile12);
	
	while (1)
    {
        printf("\nLog Trace related CMD MENU:\n");
        printf("1. Read all Log traces from DRAM\n");
        printf("2. enable/disable trace record\n");
        printf("3. save log traces to flash\n");
        printf("4. read log traces from flash\n");
        printf("5. read sys info\n");
        printf("6. read HwDebug trace from DRAM\n");
        printf("7. analyze HwDebug trace from Binary\n");
        printf("8. read flash data and redundant\n");
        printf("9. read FW Runtime Info\n");
        printf("0. EXIT\n");
        printf("\nPlease Press Key 1 - 8 to select command: ");

        ch = DBG_Getch();
        ch -= 0x30;

        DBG_printf("%d\n", ch);

        switch (ch)
        {
        case 0:
            return 0;
        case 1:
            Api_Ext_Get_TraceLogReport(pDevObj, pReportFileDir, pFormatFile0, pFormatFile12, pTraceLogHeaderFile);
            break;
        case 2:
            Api_Ext_ChangeTraceRecord(pDevObj);
            break;
        case 3:
            Api_Ext_TraceLogControl(pDevObj, MCU0, TL_SAVE_TRACE_TO_FLASH, 0);
            break;
        case 4:
            Api_Ext_ReadTraceLog_Flash(pDevObj, pReportFileDir, pFormatFile0, pFormatFile12, pTraceLogHeaderFile);
            break;
        case 5:
            Api_Ext_Get_SysInfo(pDevObj, pReportFileDir);
            break;
        case 6:
            Api_Ext_HwDebug_Log_Decode(pDevObj, pReportFileDir);
            break;
        case 7:
            Api_Ext_HwDebug_Log_Decode_Bin(pDevObj, pReportFileDir);
            break;
        case 8:
            Api_Ext_Read_Flash_Addr(pDevObj, pReportFileDir);
            break;
        case 9:
            Api_Ext_Read_Fw_Runtime_Info(pDevObj);
            break;
        default:
            break;
        }
    }

    return 0;
}


