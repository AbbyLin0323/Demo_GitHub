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
  File Name     : host_api_ext.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the extended function of host access to device.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "host_api_ext.h"



STATUS Api_Ext_Get_Var_Table_Addr(PDEVICE_OBJECT pDevObj,U32 * pVarTableAddr)
{
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,MCU0,OP_GET_VAR_TABLE,NULL,NULL,pVarTableAddr,NULL);
	return Status;
}

STATUS Api_Read_SpiFlash(PDEVICE_OBJECT pDevObj,U32 ulSrcAddr,U32 ulDstAddr,U32 ulDataBlockNum,U32 * pVarTableAddr)
{
	STATUS Status;
    U32 ulParam1, ulParam2;

    ulParam1 = ulDstAddr;    // DRAM Address
    ulParam2 = (ulSrcAddr & 0xffffff) | (ulDataBlockNum << 24) ;  // Spi offset and data len         

	Status = pDevObj->Hal_NoneData(pDevObj->Handle,MCU0,OP_SPI_READ,ulParam1,ulParam2,pVarTableAddr,NULL);
	return Status;
}

STATUS Api_Write_SpiFlash(PDEVICE_OBJECT pDevObj,U32 ulSrcAddr,U32 ulDstAddr,U32 ulDataBlockNum,U32 * pVarTableAddr)
{
	STATUS Status;
    U32 ulParam1, ulParam2;

    ulParam1 = ulSrcAddr;    // DRAM Address
    ulParam2 = (ulDstAddr & 0xffffff) | (ulDataBlockNum << 24) ;  // Spi offset and each data block len is 4K        

	Status = pDevObj->Hal_NoneData(pDevObj->Handle,MCU0,OP_SPI_WRITE,ulParam1,ulParam2,pVarTableAddr,NULL);
	return Status;
}

STATUS Api_Clear_DiskLock(PDEVICE_OBJECT pDevObj,U32 * pVarTableAddr)
{
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,MCU0,OP_CLEAR_DISKLOCK,NULL,NULL,pVarTableAddr,NULL);
	return Status;
}
STATUS Api_Ext_Jump_To_Fw(PDEVICE_OBJECT pDevObj)
{
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,MCU0,OP_JUMP,NULL,NULL,NULL,NULL);
	return Status;
}
STATUS Api_Updata_Fw(PDEVICE_OBJECT pDevObj, U32 ulCnt,U8 * pFwBuf)
{
	STATUS Status;
	
	Status = pDevObj->Hal_Fw_Download(pDevObj->Handle, ulCnt, pFwBuf);
	if (Status == RETURN_SUCCESS)
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_L3_Llf(PDEVICE_OBJECT pDevObj,U8 ucCpuId)
{
	STATUS Status;
	U32 Ret;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,ucCpuId,OP_L3_FORMAT,NULL,NULL,&Ret,NULL);
	if((Status==RETURN_SUCCESS)&&(Ret==0))
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_L2_Llf(PDEVICE_OBJECT pDevObj,U8 ucCpuId)
{
	U32 Ret;
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,ucCpuId,OP_L2_FORMAT,NULL,NULL,&Ret,NULL);
	if((Status==RETURN_SUCCESS)&&(Ret==0))
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_Bbt_Save(PDEVICE_OBJECT pDevObj,U8 ucCpuId)
{
	U32 Ret;
	STATUS Status;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,ucCpuId,OP_BBT_SAVE,NULL,NULL,&Ret,NULL);
	if((Status==RETURN_SUCCESS)&&(Ret==0))
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_Dbg_ShowAll(PDEVICE_OBJECT pDevObj, U8 ucCpuId)
{
	STATUS Status;
	U32 Ret;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle, ucCpuId, OP_DBG_SHOWALL, NULL, NULL, &Ret, NULL);
	if ((Status == RETURN_SUCCESS) && (Ret == 0))
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_Bbt_Load(PDEVICE_OBJECT pDevObj,U8 ucCpuId)
{
	STATUS Status;
	U32 Ret;
	Status = pDevObj->Hal_NoneData(pDevObj->Handle,ucCpuId,OP_BBT_LOAD,NULL,NULL,&Ret,NULL);
	if((Status==RETURN_SUCCESS)&&(Ret==0))
		return RETURN_SUCCESS;
	return RETURN_FAIL;
}
STATUS Api_Ext_Mem_Read(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData)
{
	STATUS Status;
	U32 ulTempMemData = 0;
	U8 *pTempData = (U8 *)&ulTempMemData;
	U32 ulDoneBytes = 0;
	U8 ucOffset;
	U8 ucCycle;

	if((ulLen <= 4)&&(ulLen > 0))
	{
		//Register read
		while(ulDoneBytes < ulLen)
		{
			ucOffset = ulMemAddr%4;
			Status = Api_Read_Register(pDevObj,ucCpuId,ulMemAddr - ucOffset,&ulTempMemData);
			if (RETURN_SUCCESS != Status)
			{
				return Status;
			}
			for(ucCycle = 0; ((ucCycle < ulLen - ulDoneBytes)&&(ucOffset + ucCycle < 4)); ucCycle++)
			{
				*pMemData++ = *(pTempData + ucOffset + ucCycle);
			}
			ulDoneBytes += ucCycle;
			ulMemAddr = ulMemAddr - ucOffset + 4;
			ulTempMemData = 0;
		}
	}
	else if(ulLen > 4)
	{
		//Dram_Sram read
		Status = Api_Read_Dram_Sram(pDevObj,ucCpuId,ulMemAddr,ulLen,pMemData);
	}
	else
	{
		printf("Error:Mem read len is zero!\n");
		DBG_Getch();
	}

	return Status;
}
STATUS Api_Ext_Mem_Write(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData)
{
	STATUS Status;
	U32 ulTempMemData = 0;
	U8 *pTempData = (U8 *)&ulTempMemData;
	U32 ulDoneBytes = 0;
	U8 ucOffset;
	U8 ucCycle;

	if((ulLen <= 4)&&(ulLen > 0))
	{
		//Register write
		while(ulDoneBytes < ulLen)
		{
			ucOffset = ulMemAddr%4;
			Status = Api_Read_Register(pDevObj,ucCpuId,ulMemAddr - ucOffset,&ulTempMemData);
			if (RETURN_SUCCESS != Status)
			{
				return Status;
			}
			for(ucCycle = 0; ((ucCycle < ulLen - ulDoneBytes)&&(ucOffset + ucCycle < 4)); ucCycle++)
			{
				*(pTempData + ucOffset + ucCycle) = *pMemData++;
			}
			ulDoneBytes += ucCycle;
			Status = Api_Write_Register(pDevObj,ucCpuId,ulMemAddr - ucOffset,ulTempMemData);
			if (RETURN_SUCCESS != Status)
			{
				return Status;
			}
			ulMemAddr = ulMemAddr - ucOffset + 4;
			ulTempMemData = 0;
		}
	}
	else if(ulLen > 4)
	{
		//Dram_Sram write
		Status = Api_Write_Dram_Sram(pDevObj,ucCpuId,ulMemAddr,ulLen,pMemData);
	}
	else
	{
		printf("Error:Mem write len is zero!\n");
		DBG_Getch();
	}

	return Status;
}
STATUS Api_Ext_TraceLogControl(PDEVICE_OBJECT pDevObj,CPUID MCUID,TL_CTL tlCtl,U16 secCount)
{
	STATUS Status;
	OP_CODE opCode = OP_TRACELOG_CONTROL;
	U32 InParam1 = tlCtl;
	U16 *pParam = (U16 *)&InParam1;
	*(pParam + 1) = secCount;
	Status = Api_NoneData(pDevObj,MCUID,opCode,InParam1,0,0,0);
	return Status;
}


