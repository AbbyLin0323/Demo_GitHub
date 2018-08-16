#include "plx_lib.h"
#include   "cfgmgr32.h"
#include ".\jtag\FTCJTAG.h"
PCIE_DEVICE g_Pcie_Dev[MAX_SUPPORT_DEV];
PCIE_DEVICE g_Pcie_Bridge_Ep[MAX_SUPPORT_DEV];
FTC_HANDLE g_ftHandle; // global variable
	//VOID __declspec(dllexport) Plx_Enum_Bridge_Ep();
#define EXPORT extern "C" __declspec(dllexport)

BOOLEAN WINAPI
DllMain(
HANDLE hInst,
U32    ReasonForCall,
LPVOID lpReserved
)
{
	// Added to prevent compiler warning
	if (hInst == INVALID_HANDLE_VALUE)
	{
	}

	if (lpReserved == NULL)
	{
	}

	switch (ReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		printf(("\n"));
		printf(("<=======================================>\n"));
		printf(("DllMain( DLL_PROCESS_ATTACH )\n"));
		printf((
			"PLX API DLL v%d.%02d (%d-bit) - built %s %s\n",
			PLX_SDK_VERSION_MAJOR, PLX_SDK_VERSION_MINOR,
			(U32)(sizeof(PLX_UINT_PTR) * 8),
			__DATE__, __TIME__
			));
		break;

	case DLL_PROCESS_DETACH:
		printf(("DllMain( DLL_PROCESS_DETACH )\n"));
		printf(("<=======================================>\n"));
		break;

	case DLL_THREAD_ATTACH:
		printf(("DllMain( DLL_THREAD_ATTACH )\n"));
		break;

	case DLL_THREAD_DETACH:
		printf(("DllMain( DLL_THREAD_DETACH )\n"));
		break;

	default:
		break;
	}

	printf(("...Exit DllMain\n"));

	return TRUE;
}
U32 jtag_power_control_init()
{
	FTC_STATUS Status = FTC_SUCCESS;
	U32 dwNumDevices = 0;
	char szDeviceName[100];
	U32 dwLocationID = 0;
	char szDllVersion[10];
	char szChannelBuffer[10] = { 0 };

	U32 dwClockFrequencyHz = 0;
	U32 dwDeviceType = 0;

	InitDllMain();
	//////////////////////	
#if 0
	// is for FT2232C device
	Status = JTAG_GetNumDevices(&dwNumDevices);
#endif

	// is for FT2232H device
	Status = JTAG_GetNumHiSpeedDevices((LPDWORD)&dwNumDevices);

	Status = JTAG_GetDllVersion(szDllVersion, 10);
	//MessageBox(NULL, szDllVersion, "JTAG DLL Version", MB_OK);
	 
	if ((Status == FTC_SUCCESS) && (dwNumDevices > 0))
	{
		if (dwNumDevices == 1)
		{

			//Status = JTAG_GetDeviceNameLocID(0, szDeviceName, 50, &dwLocationID);
			Status = JTAG_GetHiSpeedDeviceNameLocIDChannel(
				0, szDeviceName, 50, (LPDWORD)&dwLocationID, szChannelBuffer, 5, (LPDWORD)&dwDeviceType);

			if (Status == FTC_SUCCESS)
			{
				//Status = JTAG_OpenEx(szDeviceName, dwLocationID, &g_ftHandle);
				Status = JTAG_OpenHiSpeedDevice(
					szDeviceName, dwLocationID, szChannelBuffer, &g_ftHandle);
				//Status = JTAG_Open(&ftHandle);
			}
		}
		else
		{
			if (dwNumDevices == 2)
			{
				//Status = JTAG_GetDeviceNameLocID(1, szDeviceName, 50, &dwLocationID);
				// device index = 0 , find channel A
				// device index = 1 , find channel B
				Status = JTAG_GetHiSpeedDeviceNameLocIDChannel(
					1, szDeviceName, 50, (LPDWORD)&dwLocationID, szChannelBuffer, 5, (LPDWORD)&dwDeviceType);

				if (Status == FTC_SUCCESS)
				{
					//Status = JTAG_OpenEx(szDeviceName, dwLocationID, &g_ftHandle);
					Status = JTAG_OpenHiSpeedDevice(
						szDeviceName, dwLocationID, szChannelBuffer, &g_ftHandle);
					//Status = JTAG_Open(&ftHandle);
				}
			}
		}
	}

	if (Status == FTC_SUCCESS)
		Status = JTAG_InitDevice(g_ftHandle, 0);

	return Status;
}
U32 jtag_power_control(BOOL bPowerOn, U32 bPowerID)
{
	FTC_INPUT_OUTPUT_PINS LowInputOutputPinsData = { 0 };
	FTH_INPUT_OUTPUT_PINS HighInputOutputPinsData = { 0 };

	FTC_STATUS Status = FTC_SUCCESS;

	LowInputOutputPinsData.bPin2InputOutputState = TRUE;
	LowInputOutputPinsData.bPin2LowHighState = bPowerOn;

	Status = JTAG_SetHiSpeedDeviceGPIOs(g_ftHandle, TRUE, &LowInputOutputPinsData, TRUE, &HighInputOutputPinsData);

	return Status;
}

U32 jtag_close()
{
	JTAG_Close(g_ftHandle);

	return 0;
}
VOID Plx_Write_Bd_Register(PPCIE_DEVICE pDevObj,U32 ulRegData)
{
	BACK_DOOR_REG * pBackDoorReg;
	pBackDoorReg = (BACK_DOOR_REG *)((U8 *)pDevObj->Va[pDevObj->ucTargetBar]+pDevObj->ulOffsetInBar);
	pBackDoorReg->DW0.MP_ENABLE = 1;
	pBackDoorReg->DW0.MP_SIGNATURE = 0x6740 ;
	pBackDoorReg->DW4.MP_PARA_2 = ulRegData;
}
EXPORT VOID Plx_Get_Bridge_Ep_Handle(U8 ucEpIndex,VOID ** HANDLE)
{
	*HANDLE = (void *)&g_Pcie_Bridge_Ep[ucEpIndex].DevObj;
	return;
}
EXPORT VOID Plx_Read_Fast(VOID *HANDLE,U32 * pRegVal)
{
	PLX_DEVICE_OBJECT *pDevice;
	pDevice = (PLX_DEVICE_OBJECT *)HANDLE;
	*pRegVal =
        PlxPci_PciRegisterReadFast(
            pDevice,
            0x80,
            NULL
            );
}
EXPORT BOOL Plx_Power_Status(void * Handle)
{
	U32 RegValue;
	RegValue = 0;
	do{
		Plx_Read_Fast(Handle,&RegValue);
	}while(RegValue==0);
    if (RegValue & POWER_CONTROLLER_CONTROL)	
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
EXPORT bool Plx_Get_Power_Status(void * Handle)
{
	U32 RegValue;
	RegValue = 0;
	do{
		Plx_Read_Fast(Handle,&RegValue);
	}while(RegValue==0);
    if (RegValue & POWER_CONTROLLER_CONTROL)	
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
EXPORT VOID Plx_Enum_Bridge_Ep(unsigned char * pDevNum)
{
	PLX_STATUS rc;
	
	U8 ucDevIndex;
	U8 ucBus,ucSlot,ucFunction;
	PLX_DEVICE_KEY DevKey;
	PLX_DEVICE_OBJECT Device;
	*pDevNum = 0;
	for(ucDevIndex=0;ucDevIndex<MAX_SUPPORT_DEV;ucDevIndex++)
	{
		memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));
		// Check if device exists
		rc = PlxPci_DeviceFind(
			&DevKey,
			(U8)ucDevIndex);
		if (rc == ApiSuccess)
		{
			// Default to add device
			if((DevKey.VendorId==PLX_VENDID)&&(DevKey.DeviceId==PLX_DEVICEID))
			{
				printf("venid:0x%x devid:0x%x\n", DevKey.VendorId, DevKey.DeviceId);
				memset(&Device,0,sizeof(PLX_DEVICE_OBJECT));
				rc =PlxPci_DeviceOpen(&DevKey,&Device);
				if (rc == ApiSuccess)
				{
					g_Pcie_Bridge_Ep[(*pDevNum)].DevObj = Device;
					
					ucBus = g_Pcie_Bridge_Ep[(*pDevNum)].DevObj.Key.bus;
					ucSlot = g_Pcie_Bridge_Ep[(*pDevNum)].DevObj.Key.slot;
					ucFunction = g_Pcie_Bridge_Ep[(*pDevNum)].DevObj.Key.function;

					
					
					(*pDevNum)++;
				}
			}
		}
		else
		{
			break;
		}
	}    
	jtag_power_control_init();
}


VOID  Plx_Enum_Dev(unsigned char * pDevNum)
{
	PLX_STATUS rc;
	U32 ulClassCode,ulBarAddress;
	U8 ucDevIndex,ucBarIndex;
	U8 ucBus,ucSlot,ucFunction;
	PLX_DEVICE_KEY DevKey;
	PLX_DEVICE_OBJECT Device;
	*pDevNum = 0;
	//jtag_close();
	for(ucDevIndex=0;ucDevIndex<MAX_SUPPORT_DEV;ucDevIndex++)
	{
		memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));
		// Check if device exists
		rc =PlxPci_DeviceFind(
					&DevKey,
					(U8)ucDevIndex
					);

		if (rc == ApiSuccess)
		{
			// Default to add device
			if((DevKey.VendorId==VIA_VENDID)&&(DevKey.DeviceId==VIA_DEVICEID))
			{
				printf("venid:0x%x devid:0x%x\n", DevKey.VendorId, DevKey.DeviceId);
				memset(&Device,0,sizeof(PLX_DEVICE_OBJECT));
				rc =PlxPci_DeviceOpen(&DevKey,&Device);
				if (rc == ApiSuccess)
				{
					g_Pcie_Dev[(*pDevNum)].DevObj = Device;
					for (ucBarIndex=0; ucBarIndex<PCI_NUM_BARS_TYPE_00; ucBarIndex++)
					{
						rc =PlxPci_PciBarMap(&Device,ucBarIndex,&(g_Pcie_Dev[(*pDevNum)].Va[ucBarIndex]));
						if(rc!=ApiSuccess)
						{
							g_Pcie_Dev[(*pDevNum)].Va[ucBarIndex] = (void *)INVALID_8F;
						}
					}
						//U32 PlxPci_PciRegisterRead(U8 bus, U8 slot, U8 function, U16 offset, PLX_STATUS*pStatus ); 
					ucBus = g_Pcie_Dev[(*pDevNum)].DevObj.Key.bus;
					ucSlot = g_Pcie_Dev[(*pDevNum)].DevObj.Key.slot;
					ucFunction = g_Pcie_Dev[(*pDevNum)].DevObj.Key.function;

					ulClassCode = PlxPci_PciRegisterRead(ucBus,ucSlot,ucFunction,8,&rc);
					if(rc==ApiSuccess)
					{
						g_Pcie_Dev[(*pDevNum)].ucBus = ucBus;
						g_Pcie_Dev[(*pDevNum)].ucSlot = ucSlot;
						g_Pcie_Dev[(*pDevNum)].ucFun = ucFunction;
						if((ulClassCode>>8)==NVME_CLASS_CODE)
						{
							g_Pcie_Dev[(*pDevNum)].ucTargetBar = NVME_MP_BAR;//0
							g_Pcie_Dev[(*pDevNum)].ulOffsetInBar = NVME_REG_OFFSET_IN_BAR;
							ulBarAddress = 0x18;
						}
						else if((ulClassCode>>8)==AHCI_CLASS_CODE)
						{
							g_Pcie_Dev[(*pDevNum)].ucTargetBar = AHCI_MP_BAR;//5
							g_Pcie_Dev[(*pDevNum)].ulOffsetInBar = AHCI_REG_OFFSET_IN_BAR;
							ulBarAddress = 0x20;
							
						}
						else
						{
							printf("invalid pci header classcode:0x%x\n", ulClassCode);
							(*pDevNum) = 0;
							return;
							//_getch();
						}
						PlxPci_PciRegisterWrite(ucBus, ucSlot, ucFunction, ulBarAddress, ((ucBus << 8) | (ucSlot << 16) | (ucFunction << 24)));
						//g_Detect_Obj[(*pDevNum)].Handle = &g_Pcie_Dev[(*pDevNum)];
						//wt pci position to back door reg in bar mem
//						Plx_Write_Bd_Register(&g_Pcie_Dev[(*pDevNum)],(ucBus|(ucSlot<<8)|(ucFunction<<16)));
					}
					
					(*pDevNum)++;
				}
			}
		}
		else
		{
			break;
		}
	}    

}


VOID Plx_Trig_Cmd(BACK_DOOR_REG * pBackDoorReg,BACK_DOOR_CMD Cmd,U32 ulParam0,U32 ulParam1)
{
	pBackDoorReg->DW0.MP_ENABLE = 1;
	pBackDoorReg->DW0.MP_SIGNATURE = 0x6740 ;
	pBackDoorReg->DW1.INT_ENABLE = 1;
	pBackDoorReg->DW1.MP_CMD_TYPE = Cmd;
	pBackDoorReg->DW2.MP_PARA_0 = ulParam0;
	pBackDoorReg->DW3.MP_PARA_1 = ulParam1;

	pBackDoorReg->DW1.MP_TRIG = 1;
}
BOOL Plx_Check_Status(BACK_DOOR_REG * pBackDoorReg)
{
	return pBackDoorReg->DW1.MP_TRIG;
}
VOID Plx_Get_Pcie_Pos(U8 ucDevIndex,U8 *bus,U8 * slot,U8 * fun)
{
	//*ppDevObj = (void *)&g_Pcie_Dev[ucDevIndex];
	*bus = g_Pcie_Dev[ucDevIndex].ucBus;
	*slot = g_Pcie_Dev[ucDevIndex].ucSlot;
	*fun = g_Pcie_Dev[ucDevIndex].ucFun;
}

VOID Plx_Get_Pcie_Obj(U8 ucDevIndex,void ** ppDevObj)
{
	*ppDevObj = (void *)&g_Pcie_Dev[ucDevIndex];
}
BOOL Plx_Read_Register(PPCIE_DEVICE pDevObj, U32 ulRegAddr, U32 *pRegData)
{
	BACK_DOOR_REG * pBackDoorReg;
	pBackDoorReg = (BACK_DOOR_REG *)((U8 *)pDevObj->Va[pDevObj->ucTargetBar]+pDevObj->ulOffsetInBar);
	Plx_Trig_Cmd(pBackDoorReg,OP_BD_READ,ulRegAddr,NULL);
	while(Plx_Check_Status(pBackDoorReg));
	*(pRegData) = pBackDoorReg->DW3.MP_PARA_1;
	return TRUE;
}
BOOL Plx_Read_Register_Entry(void * pDevObj,U32 ulRegAddr,U32 *pRegData)
{
	return Plx_Read_Register((PPCIE_DEVICE)pDevObj,ulRegAddr,pRegData);
}
BOOL Plx_Write_Register(PPCIE_DEVICE pDevObj, U32 ulRegAddr, U32 ulRegData)
{
	BACK_DOOR_REG * pBackDoorReg;
	pBackDoorReg = (BACK_DOOR_REG *)((U8 *)pDevObj->Va[pDevObj->ucTargetBar]+pDevObj->ulOffsetInBar);
	Plx_Trig_Cmd(pBackDoorReg,OP_BD_WRITE,ulRegAddr,ulRegData);
	while(Plx_Check_Status(pBackDoorReg));
	return TRUE;
}
BOOL Plx_Write_Register_Entry(void * pDevObj,U32 ulRegAddr,U32 ulRegData)
{
	return Plx_Write_Register((PPCIE_DEVICE)pDevObj,ulRegAddr,ulRegData);
}
#if 1
BOOL Plx_Write_Dma_Entry(void * pDevObj, U32 ulAddr, U32 * pBuf,U32 ulCnt)
{
	U32 ulRegAddr,ulRegData;
	U32 i;
	U32 * pData;
	pData = pBuf;
	for(i=0;i<(ulCnt>>2);i++)
	{
		ulRegAddr = ulAddr +i*4;
		ulRegData = *pData++;
		Plx_Write_Register((PPCIE_DEVICE)pDevObj,ulRegAddr,ulRegData);
	}
	return TRUE;
}
#endif
BOOL Plx_Jump(PPCIE_DEVICE pDevObj, U32 ulAddr)
{
	BACK_DOOR_REG * pBackDoorReg;
	pBackDoorReg = (BACK_DOOR_REG *)((U8 *)pDevObj->Va[pDevObj->ucTargetBar]+pDevObj->ulOffsetInBar);
	Plx_Trig_Cmd(pBackDoorReg,OP_BD_JUMP,ulAddr,NULL);
	return TRUE;
}
BOOL Plx_Jump_Entry(void * pDevObj, U32 ulAddr)
{
	return Plx_Jump((PPCIE_DEVICE)pDevObj,ulAddr);
}

EXPORT BOOL Plx_Rescan_Hw()
{
	DEVINST   devInst;

	CONFIGRET   status;

	status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);

	if (status != CR_SUCCESS)
	{
		printf("CM_Locate_DevNode   failed:   %x\n ", status);
		return   FALSE;
	}
	status = CM_Reenumerate_DevNode(devInst, CM_REENUMERATE_SYNCHRONOUS);//CM_REENUMERATE_NORMAL //CM_REENUMERATE_ASYNCHRONOUS Òì²½·½Ê½¿ÉÒÔÁ¢¼´ÏìÓ¦
	if (status != CR_SUCCESS)
	{
		printf("CM_Reenumerate_DevNode   failed:   %x\n ", status);
		return   FALSE;
	}
	//Sleep(2000);
	return   TRUE;

}

EXPORT VOID Plx_Enable_Int_Fast(VOID *HANDLE)
{
	U32 RegValue;
	PLX_STATUS RES;
	PLX_DEVICE_OBJECT *pDevice;
	pDevice = (PLX_DEVICE_OBJECT *)HANDLE;


	RegValue = 0;
	do
	{
		RegValue =
			PlxPci_PciRegisterReadFast(
			pDevice,
			0x80,
			NULL
			);
	} while (!RegValue);

	RegValue |= (1 << 4);

	RES = PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);

	RegValue = 0;
	do
	{
		RegValue =
			PlxPci_PciRegisterReadFast(
			pDevice,
			0x80,
			NULL
			);
	} while (!RegValue);
	// Disable hot plug interrupt (bit 5)
	RegValue |= HOT_PLUG_INTERRUPT_ENABLE;
	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);
}

EXPORT VOID Plx_Disable_Int_Fast(VOID *HANDLE)
{
	U32 RegValue;
	PLX_DEVICE_OBJECT *pDevice;
	pDevice = (PLX_DEVICE_OBJECT *)HANDLE;
	do
	{
		RegValue =
			PlxPci_PciRegisterReadFast(
			pDevice,
			0x80,
			NULL
			);
	} while (!RegValue);
	// Disable hot plug interrupt (bit 5)
	RegValue &= ~HOT_PLUG_INTERRUPT_ENABLE;
	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);
}
EXPORT VOID Plx_Power_Off_Fast(VOID *HANDLE, BOOL bJtagEnable)
{
	U32 RegValue;
	PLX_DEVICE_OBJECT *pDevice;
	pDevice = (PLX_DEVICE_OBJECT *)HANDLE;
	if (bJtagEnable == TRUE)
		jtag_power_control(FALSE, 1);
	Sleep(2000);

	// Ask the user if he wants to remove device
	//printf("Press ENTER to continue with Device Removal.....\n\n");
	// Cons_getch();

	printf("Power OFF sequence started......Please wait \n\n");

	// Blink the Power LED
	do
	{
		RegValue =
			PlxPci_PciRegisterReadFast(
			pDevice,
			0x80,
			NULL
			);
	} while (!RegValue);

	RegValue &= ~POWER_INDICATOR_CONTROL;
	RegValue |= POWER_INDICATOR_CONTROL_BLINK;

	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);
	Sleep(2000);

	// Turn the Power OFF
	RegValue =
		PlxPci_PciRegisterReadFast(
		pDevice,
		0x80,
		NULL
		);

	RegValue |= POWER_CONTROLLER_CONTROL;

	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);
	Sleep(1000);

	printf("It is safe to remove the device \n\n");

	// Make sure the device was removed by checking for 
	// PRESENCE_DETECT_STATE bit = 0.
	printf("Waiting for Device to be removed.....\n");
	/*
	do
	{
	RegValue =
	PlxPci_PciRegisterReadFast(
	pDevice,
	0x80,
	NULL
	);
	}
	while (RegValue & PRESENCE_DETECT_STATE);
	*/
	printf("Device has been removed successfully\n\n");

	// Turn the Power LED OFF
	RegValue =
		PlxPci_PciRegisterReadFast(
		pDevice,
		0x80,
		NULL
		);

	RegValue &= ~POWER_INDICATOR_CONTROL;
	RegValue |= POWER_INDICATOR_CONTROL_OFF;

	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);

	//Sleep(5000);

}

EXPORT VOID Plx_Power_On_Fast(VOID *HANDLE, BOOL bJtagEnable)
{
	U32 RegValue;
	PLX_DEVICE_OBJECT *pDevice;
	pDevice = (PLX_DEVICE_OBJECT *)HANDLE;

	if (bJtagEnable == TRUE)
		jtag_power_control(TRUE, 1);
	Sleep(200);
	printf("Please plug in the device... \n\n");

	//! Make sure a device was just plugged in by checking for 
	//! PRESENCE_DETECT_STATE bit = 1. If so, continue with plug in
	do
	{
		RegValue =
			PlxPci_PciRegisterReadFast(
			pDevice,
			0x80,
			NULL
			);
	} while (!(RegValue));
	//while(!(RegValue & PRESENCE_DETECT_STATE));

	//! Indicate to user that new device has been detected
	printf("Device has been detected. \n");
	printf("Power ON sequence started......Please Wait \n\n");

	//! Blink the Power LED
	RegValue =
		PlxPci_PciRegisterReadFast(
		pDevice,
		0x80,
		NULL
		);

	RegValue &= ~POWER_INDICATOR_CONTROL;
	RegValue |= POWER_INDICATOR_CONTROL_BLINK;

	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);
	Sleep(2000);

	//! Turn the Power ON
	RegValue =
		PlxPci_PciRegisterReadFast(
		pDevice,
		0x80,
		NULL
		);

	RegValue &= ~POWER_CONTROLLER_CONTROL;
	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);

	Sleep(1000);

	//! Set the Power LED to Steady ON
	RegValue =
		PlxPci_PciRegisterReadFast(
		pDevice,
		0x80,
		NULL
		);

	RegValue &= ~POWER_INDICATOR_CONTROL;
	RegValue |= POWER_INDICATOR_CONTROL_ON;

	PlxPci_PciRegisterWriteFast(
		pDevice,
		0x80,
		RegValue
		);

	printf("Device successfully plugged in \n\n");

	// Start the Windows Hardware Detection Utility
	printf(
		"*******************************************\n"
		"Windows hardware wizard will start now \n"
		"Insert Driver CD when prompted by Windows \n"
		"*******************************************\n\n"
		);
	//Sleep(5000);
	//Cons_getch();
}