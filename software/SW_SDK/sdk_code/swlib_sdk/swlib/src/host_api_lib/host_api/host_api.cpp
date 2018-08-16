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
  File Name     : host_api.cpp
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
#include "host_api.h"
#include "..\host_api_disk_detect/host_disk_detect.h"
#include "./nvme/hal_nvme.h"
#include "./ahci/hal_ahci.h"
#include "./uart/hal_uart.h"
#include "./scsi/hal_scsi.h"
DEVICE_OBJECT g_dev_obj[MAX_DISK];

U8 g_flash_host_data_addr[MAX_DISK][MAX_SUPPORT_FLASH_CNT][MAX_FLASH_PG_SZ];
U8 g_flash_host_red_addr[MAX_DISK][MAX_FLASH_SUBSYS_CNT*MAX_FLASH_PG_SZ];
U8 g_flash_host_status_addr[MAX_DISK][MAX_FLASH_SUBSYS_CNT*MAX_FLASH_PG_SZ];
U16 Api_Rom_Gen_Crc(U16 * pDataAddr,U32 ulLen)
{
	U32 i;
	U16 sum;
	U16 * pData;
	sum = 0;
	pData = pDataAddr;
    for (i=0;i<((ulLen>>1));i++)
    {
        sum ^= *(pData++);
    }    
    return sum;
}
U32 Api_Data_Gen_Crc(U32 * pDataAddr,U32 ulLen)
{
	U32 i;
	U32 sum;
	U32 * pData;
	sum = 0;
	pData = pDataAddr;
    for (i=0;i<((ulLen>>2));i++)
    {
        sum ^= *(pData++);
    }    
    return sum;
}
void Api_Init_Dev_Function(PDEVICE_OBJECT pdevobj,DISKTYPE disktype)
{
	
	pdevobj->Disktype = disktype;
	if(disktype ==  NVME)
	{
		pdevobj->Hal_Read_Dram_Sram = Nvme_Read_Dram_Sram;
		pdevobj->Hal_Write_Dram_Sram = Nvme_Write_Dram_Sram;
		pdevobj->Hal_Read_Flash = Nvme_Read_Flash;
		pdevobj->Hal_Write_Flash = Nvme_Write_Flash;
		pdevobj->Hal_Erase_Flash = Nvme_Erase_Flash;
		pdevobj->Hal_NoneData = Nvme_NoneData;
		pdevobj->Hal_Get_Identify_Data = Nvme_Get_Identify_Data;
		pdevobj->Hal_Fw_Download = Nvme_Fw_Download;
        pdevobj->HaL_Get_SmartInfo = Nvme_Get_Smart_Data;
		pdevobj->Hal_Get_ModelNumber =Nvme_Get_ModelNumber;
		pdevobj->Hal_Standard_Cmd_Dma_Read = NULL;
		pdevobj->Hal_Standard_Cmd_Dma_Read = NULL;
	}
	else if((disktype == AHCI)||(disktype == SATA))
	{
		pdevobj->Hal_Read_Dram_Sram = Ahci_Read_Dram_Sram;
		pdevobj->Hal_Write_Dram_Sram = Ahci_Write_Dram_Sram;
		pdevobj->Hal_Read_Flash = Ahci_Read_Flash;
		pdevobj->Hal_Write_Flash = Ahci_Write_Flash;
		pdevobj->Hal_Erase_Flash = Ahci_Erase_Flash;
		pdevobj->Hal_NoneData = Ahci_NoneData;
		pdevobj->Hal_Fw_Download = Ahci_Fw_Download;
		pdevobj->Hal_Get_Identify_Data = Ahci_Get_Identify_Data;
		pdevobj->Hal_Get_ModelNumber = Ahci_Get_ModelNumber;
        pdevobj->HaL_Get_SmartInfo = Ahci_Get_Smart_Data;

		

		pdevobj->Hal_Standard_Cmd_Dma_Read = Ahci_Cmd_Dma_Read;
		pdevobj->Hal_Standard_Cmd_Dma_Read = Ahci_Cmd_Dma_Write;
	}
	else if (disktype == UART)
	{
		pdevobj->Hal_Read_Dram_Sram = Uart_Read_Dram_Sram;
		pdevobj->Hal_Write_Dram_Sram = Uart_Write_Dram_Sram;
		pdevobj->Hal_Read_Flash = Uart_Read_Flash;
		pdevobj->Hal_Write_Flash = Uart_Write_Flash;
		pdevobj->Hal_Erase_Flash = Uart_Erase_Flash;
		pdevobj->Hal_NoneData = Uart_NoneData;
		pdevobj->Hal_Set_Uart_Baud = Uart_Fw_Set_Uart_Baudrate;

		
	}
	else if (disktype == USB_BRIDGE_SATA)
	{
		pdevobj->Hal_Get_Identify_Data = Scsi_Get_Identify_Data;
		pdevobj->Hal_Get_ModelNumber = Scsi_Get_ModelNumber;
		pdevobj->Hal_NoneData = Scsi_NoneData;
		pdevobj->Hal_Read_Dram_Sram = Scsi_Read_Dram_Sram;
		pdevobj->Hal_Write_Dram_Sram = Scsi_Write_Dram_Sram;
		pdevobj->Hal_Read_Flash = Scsi_Read_Flash;
		pdevobj->Hal_Write_Flash = Scsi_Write_Flash;
		pdevobj->Hal_Erase_Flash = Scsi_Erase_Flash;

	}
}
void Api_Init_FlashInfo(PDEVICE_OBJECT pDevObj)
{
	PLOGIC_PU_ENTRY pLogicPuEntry;
	U8 ucPhyPu,ucLogicPuid,ucIndex,ucSubSys,ucPuInSys;
	U32 ulPhyPuMsk,ulPhyPuMskLow,ulPhyPuMskHigh;
	//U32 ul

	ucLogicPuid = 0;
	for(ucSubSys = 0;ucSubSys<MAX_FLASH_SUBSYS_CNT;ucSubSys++)
	{
		ucPuInSys = 0;
		//*MAX_FLASH_SUBSYS_CNT*/
		for(ucPhyPu=0;ucPhyPu<(MAX_PHY_PU_ID);ucPhyPu++)
		{
			ulPhyPuMskLow = pDevObj->VarTable.SubSysTable[ucSubSys].ulPhyPuMskLow;
			ulPhyPuMskHigh = pDevObj->VarTable.SubSysTable[ucSubSys].ulPhyPuMskHigh;
			ulPhyPuMsk = (ucPhyPu<32?ulPhyPuMskLow:ulPhyPuMskHigh);
			ucIndex = (ucPhyPu<32?ucPhyPu:(ucPhyPu-32));
			if((ulPhyPuMsk&(1<<ucIndex))!=0)
			{
				pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPuid];
				pLogicPuEntry->ucPhyPu = ucPhyPu;
				pLogicPuEntry->ucSubFlashSysId = ucSubSys;//(ucPhyPu&((1<<MAX_FLASH_SUBSYS_CNT_BITS)-1));
				pLogicPuEntry->ucPuInSubSys = ucPuInSys;
				pLogicPuEntry->ulHostFlashDataAddr = (U8 *)(pDevObj->FlashInfo.ulHostFlashDataAddr + (pDevObj->VarTable.SubSysTable[0].ucPhyPageSize*pDevObj->VarTable.SubSysTable[0].ucPlnNum)*ucLogicPuid);
				pLogicPuEntry->ulHostFlashStatusAddr = (U8 *)(pDevObj->FlashInfo.ulHostFlashStatusAddr+ucSubSys*HOST_BUFF_SIZE+pLogicPuEntry->ucPuInSubSys*sizeof(VD_NFC_STATUS));
				pLogicPuEntry->ulHostFlashRedAddr = (U8 *)(pDevObj->FlashInfo.ulHostFlashRedAddr+ucSubSys*HOST_BUFF_SIZE+pLogicPuEntry->ucPuInSubSys*sizeof(RED));
				
				ucLogicPuid++;
				ucPuInSys++;
			}
		}
	}

	for(ucLogicPuid=0;ucLogicPuid<(pDevObj->VarTable.SubSysTable[0].ucPuNum)*pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;ucLogicPuid++)
	{
		pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPuid];
		printf("pu:%d subsyspu:%d phypu:%d subsysid:%d data:0x%x status:0x%x red:0x%x\n",ucLogicPuid,pLogicPuEntry->ucPuInSubSys,pLogicPuEntry->ucPhyPu,pLogicPuEntry->ucSubFlashSysId,pLogicPuEntry->ulHostFlashDataAddr,pLogicPuEntry->ulHostFlashStatusAddr,pLogicPuEntry->ulHostFlashRedAddr);
	}
}
STATUS Api_Get_ModelNumber(PDEVICE_OBJECT pdevobj,U8 * identifybuf,U8 mn[40])
{
	STATUS status;

	status = pdevobj->Hal_Get_ModelNumber(pdevobj->Handle,identifybuf,mn);

	return status;
}

STATUS Api_Dma_Read(PDEVICE_OBJECT pdevobj, U32 lba, U8 * inbuf_addr, U32 trans_sec_cnt)
{
	STATUS status;
	if (pdevobj->Hal_Standard_Cmd_Dma_Read == NULL)
		status = RETURN_FAIL;
	else
		status = pdevobj->Hal_Standard_Cmd_Dma_Read(pdevobj->Handle,lba,inbuf_addr,trans_sec_cnt);
	return status;
}
STATUS Api_Dma_Write(PDEVICE_OBJECT pdevobj, U32 lba, U8 * outbuf_addr, U32 trans_sec_cnt)
{
	STATUS status;
	if (pdevobj->Hal_Standard_Cmd_Dma_Write == NULL)
		status = RETURN_FAIL;
	else
		status = pdevobj->Hal_Standard_Cmd_Dma_Write(pdevobj->Handle, lba, outbuf_addr, trans_sec_cnt);
	return status;
}
STATUS Api_Get_Identify_Data(PDEVICE_OBJECT pdevobj,U8 * readbuf)
{
	STATUS status;
	
	status = pdevobj->Hal_Get_Identify_Data(pdevobj->Handle,readbuf);
	return status;
}
/*
	Function：读取设备内存至host

	Input Param：
	pDevObj:设备对象
	ucCpuId:指定cpu id(1:core0 2:core1 3:core2)
	ulStartAddr:读取设备的地址
	ulLength:读取长度
	pReadBuf:读buffer地址

*/
STATUS Api_Read_Dram_Sram(PDEVICE_OBJECT pDevObj,U8 ucCpuId,U32 ulStartAddr,U32 ulLength,U8 * pReadBuf)
{
	STATUS status;
    U32 transcnt;
	U32 transtotalcnt;
	U32 transaddr;
	U8 * transbuf;

	
	transtotalcnt = ulLength;
	
    transaddr = ulStartAddr;
	transbuf = pReadBuf;
    while(transtotalcnt != 0)
    {
        if (ulLength > HOST_BUFF_SIZE)
        {
            transcnt = HOST_BUFF_SIZE;
        }
		else
		{
			transcnt = ulLength;
		}

    
		status = pDevObj->Hal_Read_Dram_Sram(pDevObj->Handle,ucCpuId,transaddr,transcnt,transbuf);

		if(RETURN_FAIL == status)
        {
            printf(" Read_DRAM_SRAM read fail!!\n");
            DBG_Getch();
        }
        else
        {
            transtotalcnt -= transcnt;
            transaddr+= transcnt;
            transbuf+= transcnt;
        }
    }
	return status;
}
/*
	Function：写入内存至设备

	Input Param：
	pDevObj:设备对象
	ucCpuId:指定cpu id(1:core0 2:core1 3:core2)
	ulStartAddr:写入设备的地址
	ulLength:写入长度
	pWriteBuf:写入buffer地址

*/
STATUS Api_Write_Dram_Sram(PDEVICE_OBJECT pDevObj,U8 ucCpuId,U32 ulStartAddr,U32 ulLength,U8 * pWriteBuf)
{
	STATUS status;
    U32 transcnt;
	U32 transtotalcnt;
	U32 transaddr;
	U8 * transbuf;
	
	transtotalcnt = ulLength;
    transaddr = ulStartAddr;
	transbuf = pWriteBuf;
    while(transtotalcnt != 0)
    {
		if (ulLength > HOST_BUFF_SIZE)
        {
            transcnt = HOST_BUFF_SIZE;
        }
        else
        {
            transcnt = ulLength;
        }

    
		status = pDevObj->Hal_Write_Dram_Sram(pDevObj->Handle,ucCpuId,transaddr,transcnt,transbuf);

		if(RETURN_FAIL == status)
        {
            printf(" Write_DRAM_SRAM fail!!\n");
            DBG_Getch();
        }
        else
        {
            transtotalcnt -= transcnt;
            transaddr+= transcnt;
            transbuf+= transcnt;
        }
    }
	return status;
}


void Api_LogicPumsk_To_SubSysPumsk(PDEVICE_OBJECT pdevobj,U32 ulPuMskLow,U32 ulPuMskHigh,U32 *pSubSysPuMsk)
{
	U8 ucLogicPu,ucFlashSubSysIndex,ucSubSysPuCnt;
	U32 ucPuValid;

	LOGIC_PU_ENTRY LogicPuEntry;
	ucSubSysPuCnt = (pdevobj->VarTable.SubSysTable[0].ucPuNum);
#if 1
	for(ucLogicPu=0;ucLogicPu<64;ucLogicPu++)
	{
		ucPuValid = (ucLogicPu<32?(ulPuMskLow&(1<<ucLogicPu)):(ulPuMskHigh&(1<<(ucLogicPu-32))));
		if(ucPuValid!=0)
		{
			for(ucFlashSubSysIndex=0;ucFlashSubSysIndex<MAX_FLASH_SUBSYS_CNT;ucFlashSubSysIndex++)
			{
				LogicPuEntry = pdevobj->FlashInfo.LogicPuInfoEntry[ucLogicPu];
				if(ucFlashSubSysIndex==LogicPuEntry.ucSubFlashSysId)
				{
					//printf("logicpu:%d subid:%d \n",ucLogicPu,ucFlashSubSysIndex);
					pSubSysPuMsk[ucFlashSubSysIndex]|=(1<<(ucLogicPu-ucFlashSubSysIndex*ucSubSysPuCnt));
					break;
				}
			}
		}
	}
#endif
	
}
/*
	Function：写入指定pu的指定plane的page数据

	Input Param：
	pDevObj:设备对象
	ucLogicPu:指定pu
	ucPln:指定plane（MULTI_PLN代表Full plane操作）
	pDataBuf:指定pu指定pln的data buffer地址

	note:
	调用Api_Write_Dram_Sram写入设备端的data buffer内存
	如果指定pln号，写入pDataBuf中指定pln的page内容至设备,如果指定MULTI_PLN，写入pDataBuf中所有pln的page内容至设备
	

*/

STATUS Api_Flash_Set_Data_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 * pDataBuf)
{
	PLOGIC_PU_ENTRY pLogicPuEntry;
	U32 ulDevDataAddr,ulTransCnt;
	STATUS Status;

	pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu];
	ulDevDataAddr = pDevObj->VarTable.SubSysTable[pLogicPuEntry->ucSubFlashSysId].ulDevFlashDataAddr[pLogicPuEntry->ucPuInSubSys];
	if(ucPln != MULTI_PLN)
	{
		ulDevDataAddr += (PHY_PG_SZ*ucPln);
	}
	ulTransCnt = (ucPln == MULTI_PLN ? pDevObj->VarTable.SubSysTable[0].ucPhyPageSize*pDevObj->VarTable.SubSysTable[0].ucPlnNum : pDevObj->VarTable.SubSysTable[0].ucPhyPageSize);
	Status = Api_Write_Dram_Sram(pDevObj,pLogicPuEntry->ucSubFlashSysId+2,ulDevDataAddr,ulTransCnt,pDataBuf);
	
	return Status;
}
U8 Api_Caculate_Pu_Cnt(U32 ulPuMsk)
{
	U32 ulPuIndex;
	U8 ucPuCnt;
	
	ucPuCnt = 0;
	for(ulPuIndex=0;ulPuIndex<32;ulPuIndex++)
	{
		if((ulPuMsk&(1<<ulPuIndex)))
			ucPuCnt++;
	}
	return ucPuCnt;
}
/*
	Function：读取指定pu的指定plane的page数据,并返回data buffer指针

	Input Param：
	pDevObj:设备对象
	ucLogicPu:指定pu
	ucPln:指定plane（MULTI_PLN代表Full plane操作）

	Output Param:
	pDataBuf:返回指定pu指定pln的data buffer地址

	note:
	调用Api_Read_Dram_Sram读取设备端的data buffer内存
	如果指定pln号，更新pDataBuf中指定pln的page内容,如果指定MULTI_PLN，更新pDataBuf中所有pln的page内容
	

*/
STATUS Api_Flash_Get_Data_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,U8 ** pDataBuf)
{
	PLOGIC_PU_ENTRY pLogicPuEntry;
	U32 ulDevDataAddr,ulTransCnt;
	STATUS Status;

	pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu];
	ulDevDataAddr = pDevObj->VarTable.SubSysTable[pLogicPuEntry->ucSubFlashSysId].ulDevFlashDataAddr[pLogicPuEntry->ucPuInSubSys];
	if(ucPln != MULTI_PLN)
	{
		ulDevDataAddr += (PHY_PG_SZ*ucPln);
	}
	ulTransCnt = (ucPln == MULTI_PLN ? pDevObj->VarTable.SubSysTable[0].ucPhyPageSize*pDevObj->VarTable.SubSysTable[0].ucPlnNum : pDevObj->VarTable.SubSysTable[0].ucPhyPageSize);
	Status = Api_Read_Dram_Sram(pDevObj,pLogicPuEntry->ucSubFlashSysId+2,ulDevDataAddr,ulTransCnt,pLogicPuEntry->ulHostFlashDataAddr);
	*pDataBuf = pLogicPuEntry->ulHostFlashDataAddr;
	
	return Status;
}

/*
	Function：写入指定pu的指定plane的redundent数据至设备

	Input Param：
	pDevObj:设备对象
	ucLogicPu:指定pu
	ucPln:指定plane（MULTI_PLN代表Full plane操作）
	pRed:指定写入pu的red地址

	note:
	调用Api_Write_Dram_Sram写入设备端的red内存
	如果指定pln号，写入pRed中指定pln的red内容至设备,如果指定MULTI_PLN，写入pRed中所有pln的red内容至设备
	pRed指定pu red的起始地址，根据pln号可以找到对应red的内容

*/
STATUS Api_Flash_Set_Red_To_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED * pRed)
{
	PLOGIC_PU_ENTRY pLogicPuEntry;
	U32 ulDevRedAddr,ulRedSize,ulHostRedPlnAddr;
	STATUS Status;
	U32 ulPlnNum;
	ulPlnNum = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
	pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu];
	ulDevRedAddr = pDevObj->VarTable.SubSysTable[pLogicPuEntry->ucSubFlashSysId].ulDevFlashRedAddr + pLogicPuEntry->ucPuInSubSys*PLN_RED_SIZE*ulPlnNum;//sizeof(RED);
	ulHostRedPlnAddr = (U32)pRed;
	if(ucPln!=MULTI_PLN)
	{
		ulDevRedAddr+=(ucPln*PLN_RED_SIZE);
		ulHostRedPlnAddr+=(ucPln*PLN_RED_SIZE);
	}
	ulRedSize = (ucPln == MULTI_PLN ? (PLN_RED_SIZE*ulPlnNum) : PLN_RED_SIZE);
	Status = Api_Write_Dram_Sram(pDevObj,pLogicPuEntry->ucSubFlashSysId+2,ulDevRedAddr,ulRedSize,(U8 *)ulHostRedPlnAddr);
	
	return Status;
}
/*
	Function：读取指定pu的指定plane的redundent数据,并返回pRed指针

	Input Param：
	pDevObj:设备对象
	ucLogicPu:指定pu
	ucPln:指定plane（MULTI_PLN代表Full plane操作）

	Output Param:
	pRed:返回指定pu的red地址

	note:
	调用Api_Read_Dram_Sram读取设备端的red内存
	如果指定pln号，更新pRed中指定pln的red内容,如果指定MULTI_PLN，更新pRed中所有pln的red内容
	pRed指定pu red的起始地址，根据pln号可以找到对应red的内容

*/
STATUS Api_Flash_Get_Red_Addr_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu,U8 ucPln,RED ** pRed)
{
	PLOGIC_PU_ENTRY pLogicPuEntry;
	U32 ulDevRedAddr,ulRedSize,ulRedPlnAddr;
	STATUS Status;
	U32 ulPlnNum;
	ulPlnNum = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
	pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu];
	ulDevRedAddr = pDevObj->VarTable.SubSysTable[pLogicPuEntry->ucSubFlashSysId].ulDevFlashRedAddr + pLogicPuEntry->ucPuInSubSys*PLN_RED_SIZE*ulPlnNum;//sizeof(RED);
	ulRedPlnAddr = (U32)pLogicPuEntry->ulHostFlashRedAddr;
	if(ucPln!=MULTI_PLN)
	{
		ulDevRedAddr+=(ucPln*PLN_RED_SIZE);
		ulRedPlnAddr+=(ucPln*PLN_RED_SIZE);
	}
	ulRedSize = (ucPln == MULTI_PLN ? (PLN_RED_SIZE*ulPlnNum) : PLN_RED_SIZE);

	Status = Api_Read_Dram_Sram(pDevObj,pLogicPuEntry->ucSubFlashSysId+2,ulDevRedAddr,ulRedSize,(U8 *)ulRedPlnAddr);
	*pRed = (RED *)pLogicPuEntry->ulHostFlashRedAddr;

	return Status;
}
U32 Api_Flash_Get_Status_By_Pu(PDEVICE_OBJECT pDevObj,U8 ucLogicPu)
{
	VD_NFC_STATUS *pNfcStatus;
	PLOGIC_PU_ENTRY pLogicPuEntry;
	
	pLogicPuEntry = &pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu];

	pNfcStatus = (VD_NFC_STATUS *)pLogicPuEntry->ulHostFlashStatusAddr;

	return pNfcStatus->ulStatus;
}
/*
	Function：读取group pu的状态信息

	Input Param：
	pDevObj:设备对象
	ulPuMskLow:指定的低32位pu msk
	ulPuMskHigh:指定的高32位pu msk
	

	Output Param:
	pPuMskLowStatus:返回低32位pu msk代表的状态信息（1：success 0：fail）
	pPuMskHighStatus:返回高32位pu msk代表的状态信息（1：success 0：fail）

*/
STATUS Api_Flash_Get_Cmd_Status(PDEVICE_OBJECT pDevObj,U32 ulPuMskLow,U32 ulPuMskHigh,U32 *pPuMskLowStatus,U32* pPuMskHighStatus)
{
	U8 ucLogicPu;
	if(pPuMskLowStatus!=0)
		*pPuMskLowStatus = 0;
	if(pPuMskHighStatus!=0)
		*pPuMskHighStatus = 0;
	for(ucLogicPu=0;ucLogicPu<64;ucLogicPu++)
	{
		if(ucLogicPu<32)
		{
			if(ulPuMskLow&(1<<ucLogicPu))
			{
				if(Api_Flash_Get_Status_By_Pu(pDevObj,ucLogicPu)==0)
				{
					*pPuMskLowStatus|=(1<<ucLogicPu);
				}
			}
		}
		else
		{
			if(ulPuMskHigh&(1<<(ucLogicPu-32)))
			{
				if(Api_Flash_Get_Status_By_Pu(pDevObj,ucLogicPu)==0)
				{
					*pPuMskHighStatus|=(1<<(ucLogicPu-32));
				}
			}
		}

	}
	return RETURN_SUCCESS;
}
/*
	Function：执行flash命令

	Input Param：
	pDevObj:设备对象
	ucOpcode:指定flash read,write,erase操作
	ucPln:指定pln号
	usBlock:指定block号
	usPage:指定page号
	ulPuMskLow：指定低32位group pu,ulPuMskLow的bit角标代表pu号，bit内容('0'or'1')代表是否pu有效
	ulPuMskHigh：指定高32位group pu,ulPuMskHigh的bit角标代表pu号，bit内容('0'or'1')代表是否pu有效

	note:
	Fun会将ulPuMskLow ulPuMskHigh转换成subsystem对应的的pu mask并对指定sub system发起flash对应命令

*/
STATUS Api_Flash_Function(PDEVICE_OBJECT pDevObj,U8 ucOpcode,U8 ucPln,U16 usBlock,U16 usPage,U32 ulPuMskLow,U32 ulPuMskHigh,U32 ulParam)
{
	STATUS Status;
	U8 ucCpuId,ucPlnMode,ucFlashSubSysId;
	U32 ulFlashSubSysPuMsk[MAX_FLASH_SUBSYS_CNT];
	U8 * pStatusBuf;
	U32 ulOutPut1;
	ucCpuId = 0;
	
	ucPlnMode = (ucPln==MULTI_PLN?0:1);

	for(ucFlashSubSysId=0;ucFlashSubSysId<MAX_FLASH_SUBSYS_CNT;ucFlashSubSysId++)
		ulFlashSubSysPuMsk[ucFlashSubSysId] = 0;

	Api_LogicPumsk_To_SubSysPumsk(pDevObj,ulPuMskLow,ulPuMskHigh,ulFlashSubSysPuMsk);

	for(ucFlashSubSysId=0;ucFlashSubSysId<MAX_FLASH_SUBSYS_CNT;ucFlashSubSysId++)
	{
		if(ulFlashSubSysPuMsk[ucFlashSubSysId]==0)
			continue;
		pStatusBuf = pDevObj->FlashInfo.ulHostFlashStatusAddr+HOST_BUFF_SIZE*ucFlashSubSysId;
		if(ucOpcode == HOST_FLASH_READ)
		{
			Status = pDevObj->Hal_Read_Flash(pDevObj->Handle,ucFlashSubSysId+2,ucPlnMode,ucPln,usBlock,usPage,pStatusBuf,ulFlashSubSysPuMsk[ucFlashSubSysId]);
		}
		else if(ucOpcode == HOST_FLASH_WRITE)
		{
			Status = pDevObj->Hal_Write_Flash(pDevObj->Handle,ucFlashSubSysId+2,ucPlnMode,ucPln,usBlock,usPage,pStatusBuf,ulFlashSubSysPuMsk[ucFlashSubSysId]);
		}
		else if(ucOpcode == HOST_FLASH_ERASE)
		{
			Status = pDevObj->Hal_Erase_Flash(pDevObj->Handle,ucFlashSubSysId+2,ucPlnMode,ucPln,usBlock,pStatusBuf,ulFlashSubSysPuMsk[ucFlashSubSysId]);
		}
		else if (ucOpcode == HOST_FLASH_RESET)
		{
			Status = pDevObj->Hal_NoneData(pDevObj->Handle, ucFlashSubSysId + 2, VIA_CMD_FLASH_RESET, ulFlashSubSysPuMsk[ucFlashSubSysId], NULL, &ulOutPut1, NULL);
		}
		else if (ucOpcode == HOST_FLASH_TERMINATE)
		{
			Status = pDevObj->Hal_NoneData(pDevObj->Handle, ucFlashSubSysId + 2, VIA_CMD_FLASH_TERMINATE, ulFlashSubSysPuMsk[ucFlashSubSysId], NULL, &ulOutPut1, NULL);
		}
		else if (ucOpcode == HOST_FLASH_PRECONDITION)
		{
			Status = pDevObj->Hal_NoneData(pDevObj->Handle, ucFlashSubSysId + 2, VIA_CMD_FLASH_PRECONDITION, ulFlashSubSysPuMsk[ucFlashSubSysId], NULL, &ulOutPut1, NULL);
		}
		else if (ucOpcode == HOST_FLASH_SETPARAM)
		{
			Status = pDevObj->Hal_NoneData(pDevObj->Handle, ucFlashSubSysId + 2, VIA_CMD_FLASH_SETPARAM, ulFlashSubSysPuMsk[ucFlashSubSysId], ulParam, &ulOutPut1, NULL);
		}
		else
		{
			printf("flash opcode err!!\n");
		}
		if(RETURN_FAIL==Status)
			return Status;
	}

	return Status;
}

STATUS Api_Read_Flash(PDEVICE_OBJECT pDevObj,U8 ucPln,U16 usBlock,U16 usPage,U32 ulPuMskLow,U32 ulPuMskHigh)
{
	STATUS Status;
	
	Status = Api_Flash_Function(pDevObj,HOST_FLASH_READ,ucPln,usBlock,usPage,ulPuMskLow,ulPuMskHigh,NULL);
	
	return Status;
}

STATUS Api_Write_Flash(PDEVICE_OBJECT pDevObj,U8 ucPln,U16 usBlock,U16 usPage,U32 ulPuMskLow,U32 ulPuMskHigh)
{
	STATUS Status;
	
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_WRITE, ucPln, usBlock, usPage, ulPuMskLow, ulPuMskHigh, NULL);
	
	return Status;
}

STATUS Api_Erase_Flash(PDEVICE_OBJECT pDevObj,U8 ucPln,U16 usBlock,U32 ulPuMskLow,U32 ulPuMskHigh)
{
	STATUS Status;
	
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_ERASE, ucPln, usBlock, NULL, ulPuMskLow, ulPuMskHigh, NULL);
	
	return Status;
}
STATUS Api_Flash_Reset(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh)
{
	STATUS Status;
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_RESET, NULL, NULL, NULL, ulPuMskLow, ulPuMskHigh, NULL);
	return Status;
}
STATUS Api_Flash_Terminate(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh)
{
	STATUS Status;
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_TERMINATE, NULL, NULL, NULL, ulPuMskLow, ulPuMskHigh, NULL);
	return Status;
}
STATUS Api_Flash_PreCondiTion(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh)
{
	STATUS Status;
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_PRECONDITION, NULL, NULL, NULL, ulPuMskLow, ulPuMskHigh, NULL);
	return Status;
}
STATUS Api_Flash_SetParam(PDEVICE_OBJECT pDevObj, U32 ulPuMskLow, U32 ulPuMskHigh,U32 ulParam)
{
	STATUS Status;
	Status = Api_Flash_Function(pDevObj, HOST_FLASH_SETPARAM, NULL, NULL, NULL, ulPuMskLow, ulPuMskHigh, ulParam);
	return Status;
}
STATUS Api_Set_Uart_Baudrate(PDEVICE_OBJECT pDevObj, U32 ulSpeedDev, U32 ulSpeedHost)
{
	BOOL res;
	
	if (pDevObj->Hal_Set_Uart_Baud == NULL)
		return RETURN_SUCCESS;

	res = pDevObj->Hal_Set_Uart_Baud(pDevObj->Handle, ulSpeedDev, ulSpeedHost);

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
STATUS Api_NoneData(PDEVICE_OBJECT pDevObj,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,ucCpuId,ucOpCode,ulInParam1,ulInParam2,pOutParam1,pOutParam2);
	return Status;
}
STATUS Api_Read_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 * pRegData)
{
	STATUS Status;
	Status = Api_NoneData(pDevObj,CpuId,OP_REG_READ,ulRegAddr,NULL,pRegData,NULL);
	return Status;
}
STATUS Api_Write_Register(PDEVICE_OBJECT pDevObj,CPUID CpuId,U32 ulRegAddr,U32 ulRegData)
{
	STATUS Status;
	Status = Api_NoneData(pDevObj,CpuId,OP_REG_WRITE,ulRegAddr,ulRegData,NULL,NULL);
	return Status;

}
void * Api_Get_Rom_Obj(U8 ucIndex)
{
	return &g_Rom_Detect_Obj[ucIndex];
}

STATUS Api_Rom_Close_Handle(void * pDetectObj)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	if(pDetect->Rom_Close_Handle==NULL)
		return RETURN_SUCCESS;

	res = pDetect->Rom_Close_Handle(pDetect->Handle);

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
STATUS Api_Rom_Read_Status(void *pDetectObj)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	if(pDetect->Rom_Read_Stat==NULL)
		return RETURN_SUCCESS;

	res = pDetect->Rom_Read_Stat(pDetect->Handle);

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);

}

STATUS Api_Rom_Write_Register(void *pDetectObj,U32 ulAddr,U32 ulData)
{

	PDetect_Obj pDetect;
	pDetect = (PDetect_Obj)pDetectObj;
	pDetect->Rom_Reg_Write(pDetect->Handle,ulAddr,ulData);
	return RETURN_SUCCESS;
}
STATUS Api_Rom_Read_Register(void *pDetectObj,U32 ulAddr,U32 *pData)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	res = pDetect->Rom_Reg_Read(pDetect->Handle,ulAddr,pData);
	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
STATUS Api_Rom_Jump(void *pDetectObj,U32 ulAddr)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	res = pDetect->Rom_Jump(pDetect->Handle,ulAddr);
	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);;
}
STATUS Api_Rom_Set_Uart_Baudrate(void *pDetectObj, U32 ulSpeedDev, U32 ulSpeedHost)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	if (pDetect->Rom_Set_Uart_Baud == NULL)
		return RETURN_SUCCESS;

	res = pDetect->Rom_Set_Uart_Baud(pDetect->Handle, ulSpeedDev, ulSpeedHost);

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
STATUS Api_Rom_Set_Trans_Speed(void *pDetectObj,U32 ulHostSpeed)
{
	PDetect_Obj pDetect;
	BOOL res;
	pDetect = (PDetect_Obj)pDetectObj;
	if(pDetect->Rom_Set_Trans_Speed==NULL)
		return RETURN_SUCCESS;

	res = pDetect->Rom_Set_Trans_Speed(pDetect->Handle,ulHostSpeed);

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
STATUS Api_Rom_Write_Dma(void *pDetectObj,U32 ulAddr, U32 * pBuf,U32 ulCnt)
{
	PDetect_Obj pDetect;
	pDetect = (PDetect_Obj)pDetectObj;
	pDetect->Rom_Write_Dma(pDetect->Handle,ulAddr,pBuf,ulCnt);
	return RETURN_SUCCESS;
}

STATUS Api_Get_SmartInfo(PDEVICE_OBJECT pDevObj, U8 *pDataBuf)
{
    STATUS status;

    status = pDevObj->HaL_Get_SmartInfo(pDevObj->Handle, pDataBuf);
    return status;
}

U32 Api_DwordToString(U32 ulInputDword, U8 *pOutputString)
{
    U32 index;
    U8  ucNum;
    U8 ucTmp;

    for (index = 0; index < 8; index++)
    {
        ucNum = ((ulInputDword >> (index * 4)) & 0xf);

        if (ucNum <= 9)
        {
            pOutputString[index] = ucNum + '0';
        }
        else
        {
            pOutputString[index] = '.';
        }
    }

    pOutputString[index] = '\0';

    //Correct the String sequence
    for (index = 0; index < 8; index += 2)
    {
        ucTmp = pOutputString[index];
        pOutputString[index] = pOutputString[index + 1];
        pOutputString[index + 1] = ucTmp;
    }
    return 0;
}

void Api_ShowDate(U32 ulInputDate)
{
    U16 usYear;
    U8  ucMonth;
    U8 ucDay;

    ucDay = ulInputDate & 0xff;
    ucMonth = (ulInputDate >> 8) & 0xff;
    usYear = (ulInputDate >> 16) & 0xffff;

    printf("%d/%d/%d\n", usYear, ucMonth, ucDay);

    return;
}

void Api_ShowTime(U32 ulInputTime)
{
    U8 ucHour;
    U8 ucMin;
    U8 ucSec;

    ucSec = ulInputTime & 0xff;
    ucMin = (ulInputTime >> 8) & 0xff;
    ucHour = (ulInputTime >> 16) & 0xff;

    printf("%d:%d:%d\n", ucHour, ucMin, ucSec);

    return;
}