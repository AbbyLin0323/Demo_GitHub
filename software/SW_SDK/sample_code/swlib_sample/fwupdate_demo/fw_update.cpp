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
  File Name     : pcie_enum_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines demo of pcie enumeration.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include "..\include\sw_lib.h"
#include "fw_update.h"
U32 fwupdatebuf[0x100000];
enum {
    FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG,
    FW_UPGRADE_PAYLOAD_TYPE_BL_IMG,
    FW_UPGRADE_PAYLOAD_TYPE_FW_IMG,
    FW_UPGRADE_PAYLOAD_TYPE_IMG_NUM
};
typedef union _FW_UPGRADE_PAYLOAD_ENTRY
{
    struct
    {
        /* BYTE 0: Config Bits */
        U32 bsPayLoadEn : 1;  /* 1: enable; 0: disable */
        U32 bsPayLoadType : 3;
        U32 bsPayLoadParam : 4; /* additional Param for each type */

        /* BYTE 1-3: length in 512 Bytes */
        U32 ulPayLoadSecLength : 24;

        /* DW1: payload CRC checksum */
        U32 ulPayLoadCRC;
    };

    U32 ulSelFlag[2];
} FW_UPGRADE_PAYLOAD_ENTRY;
#define FW_UPGRADE_PAYLOAD_ENTRY_CNT   16
typedef union _FW_UPGRADE_HEAD_ENTRY
{
    struct{
        FW_UPGRADE_PAYLOAD_ENTRY aPayLoadEntry[FW_UPGRADE_PAYLOAD_ENTRY_CNT];
        U32 ulPad[95];
        U32 ulHeadCRC;
    };

    U32 ulDW[128];
} FW_UPGRADE_HEAD_ENTRY;

int fw_update_main(int argc, _TCHAR* argv[])
{
	PDEVICE_OBJECT pDevObj;
	U32 ulVarTableAddr;
	U8 ch, ucDiskCnt;
	DISKTYPE type;
	U8 ucFwPath[256];
	//U8 ucFwBootloader[256];
    FILE *pFile, *pBlFile;
    
    FW_UPGRADE_HEAD_ENTRY *pHead;
    int sum = 0;
	memset(ucFwPath, 0, sizeof(ucFwPath));
	DBG_printf("please input fw path:\n");
	scanf_s("%s", ucFwPath, 80);
	
	while (1)
	{
		{
			DBG_printf("0:AHCI\n");
			DBG_printf("1:NVME\n");
			DBG_printf("2:SATA\n");
			
			DBG_printf("Please Input 0 - 2 to select mode:\n");
			ch = DBG_Getch();

			ch -= 0x30;
			DBG_printf("input: %d\n", ch);
			if (ch>3)
			{
				DBG_printf("Invalid Input %d\n", ch);
				continue;
			}
			else
			{
				type = (DISKTYPE)ch;
				ucDiskCnt = Api_Disk_Detect_Rom((DISKTYPE)ch);
				break;
			}
		}

	}
	while (1)
	{
		ucDiskCnt = Api_Disk_Detect((DISKTYPE)type);

		if (ucDiskCnt > 0)
		{
			DBG_printf("Please Input 0 - %d to select disk:\n", (ucDiskCnt - 1));
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

	Api_Ext_Get_Var_Table_Addr(pDevObj, &ulVarTableAddr);
	Api_Read_Dram_Sram(pDevObj, MCU0, ulVarTableAddr, HOST_BUFF_SIZE, (U8 *)&pDevObj->VarTable);
	Api_Write_Dram_Sram(pDevObj, MCU0, ulVarTableAddr, HOST_BUFF_SIZE, (U8 *)&pDevObj->VarTable);
	Api_Init_FlashInfo(pDevObj);
	fopen_s(&pFile,   ".\\fwupdate_demo\\fw_bin\\fw_b0_sata_test.bin", "rb");
    fopen_s(&pBlFile, ".\\fwupdate_demo\\fw_bin\\bl_b0_sata_test.bin", "rb");

    for (int i = 0; i < (0x100000 >> 2); i++)
    {
        fwupdatebuf[i] = 0;
    }

    pHead = (FW_UPGRADE_HEAD_ENTRY *)fwupdatebuf;

    
    if (pBlFile != NULL)
    {
        U32 * pBuf = &fwupdatebuf[128];
        pHead->aPayLoadEntry[0].bsPayLoadEn = TRUE;
        pHead->aPayLoadEntry[0].bsPayLoadType = FW_UPGRADE_PAYLOAD_TYPE_BL_IMG;
        pHead->aPayLoadEntry[0].ulPayLoadSecLength = BL_SZ / SEC_SIZE;
        fread_s(pBuf, BL_SZ, 1, BL_SZ, pBlFile);

        for (int i = 0; i < (BL_SZ >> 2); i++)
        {
            pHead->aPayLoadEntry[0].ulPayLoadCRC ^= *pBuf++;
        }
    }
    else
    {
        DBG_printf("Not found BL file!!\n");
    }

    if (pFile != NULL)
    {
        U32 * pBuf = (U32*)((U32)&fwupdatebuf[0] + 512 + 16 * 1024);
        pHead->aPayLoadEntry[1].bsPayLoadEn = TRUE;
        pHead->aPayLoadEntry[1].bsPayLoadType = FW_UPGRADE_PAYLOAD_TYPE_FW_IMG;
        pHead->aPayLoadEntry[1].ulPayLoadSecLength = FW_SZ / SEC_SIZE;
        fread_s(pBuf, FW_SZ, 1, FW_SZ, pFile);

        for (int i = 0; i < (FW_SZ >> 2); i++)
        {
            pHead->aPayLoadEntry[1].ulPayLoadCRC ^= *pBuf++;
        }
    }
    else
    {
        DBG_printf("Not found FW file!!\n");
    }

    for (int i=0;i<128;i++)
    {
        sum ^= pHead->ulDW[i];
    }

    pHead->ulHeadCRC = sum;

    /*

	//fopen_s(&pFile, (const char *)ucFwPath, "rb");
	if (pFile != NULL)
	{
		fread_s(fwupdatebuf, FW_SZ, 1, FW_SZ, pFile);
	}
	else
	{
		DBG_printf("fw file open err invalid path!!\n");
		DBG_Getch();
	}
    */
	Api_Updata_Fw(pDevObj, FW_SZ + BL_SZ + 512, (U8 *)fwupdatebuf);



	return 0;
}


