#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "hardwareapi.h"

#include "FTCJTAG.h"





FTC_HANDLE g_ftHandle ; // global variable

extern FILE *data_all_log;
extern U32 log_line,log_file_cnt;
extern char log_filename[256];



U32 jtag_power_control_init()
{
	FTC_STATUS Status = FTC_SUCCESS;
	U32 dwNumDevices = 0;
	char szDeviceName[100];
	U32 dwLocationID = 0;
	char szDllVersion[10];
    char szChannelBuffer[10] = {0};

	U32 dwClockFrequencyHz = 0;
    U32 dwDeviceType = 0;

	InitDllMain();
	//////////////////////	
#if 0
    // is for FT2232C device
	Status = JTAG_GetNumDevices(&dwNumDevices);
#endif
    
    // is for FT2232H device
    Status = JTAG_GetNumHiSpeedDevices(&dwNumDevices);

	Status = JTAG_GetDllVersion(szDllVersion, 10);
	//MessageBox(NULL, szDllVersion, "JTAG DLL Version", MB_OK);

	if ((Status == FTC_SUCCESS) && (dwNumDevices > 0))
	{
		if (dwNumDevices == 1)
		{
			
            //Status = JTAG_GetDeviceNameLocID(0, szDeviceName, 50, &dwLocationID);
            Status = JTAG_GetHiSpeedDeviceNameLocIDChannel(
                0, szDeviceName, 50, &dwLocationID, szChannelBuffer, 5, &dwDeviceType);

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
                    0, szDeviceName, 50, &dwLocationID, szChannelBuffer, 5, &dwDeviceType);

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


// control the GPIO let power down or up
//bPowerOn = 1 :let power up
//bPowerOn = 0 :let power down
//bPowerID{1:4}: which pin to contro the power
U32 jtag_power_control(BOOL bPowerOn,U32 bPowerID)
{
	FTC_INPUT_OUTPUT_PINS LowInputOutputPinsData = {0};
	FTH_INPUT_OUTPUT_PINS HighInputOutputPinsData = {0};
	
	FTC_STATUS Status = FTC_SUCCESS;

	//HighInputOutputPinsData.bPin6InputOutputState = TRUE;
	//HighInputOutputPinsData.bPin6LowHighState = !bPowerOn;
	HighInputOutputPinsData.bPin3InputOutputState = TRUE;
	HighInputOutputPinsData.bPin3LowHighState = !bPowerOn;
	HighInputOutputPinsData.bPin8InputOutputState = TRUE;
	HighInputOutputPinsData.bPin8LowHighState = !bPowerOn;
	
	//switch (bPowerID)
	//{
	//	case 1:
	//	//	LowInputOutputPinsData.bPin1InputOutputState = TRUE;
	//	//	LowInputOutputPinsData.bPin1LowHighState = !bPowerOn;
	//	////	break;
	//	////case 2:
	//	//	LowInputOutputPinsData.bPin2InputOutputState = TRUE;
	//	//	LowInputOutputPinsData.bPin2LowHighState = !bPowerOn;
	//	////	break;
	//	////case 3:
	//	//	LowInputOutputPinsData.bPin3InputOutputState = TRUE;
	//	//	LowInputOutputPinsData.bPin3LowHighState = !bPowerOn;
	//	////	break;
	//	////case 4:
	//	//	LowInputOutputPinsData.bPin4InputOutputState = TRUE;
	//	//	LowInputOutputPinsData.bPin4LowHighState = !bPowerOn;

	//	//	//HighInputOutputPinsData.bPin1InputOutputState = TRUE;

	//	//	HighInputOutputPinsData.bPin1InputOutputState = TRUE;
	//	//	HighInputOutputPinsData.bPin1LowHighState = !bPowerOn;
	//	////	break;
	//	////case 2:
	//	//	HighInputOutputPinsData.bPin2InputOutputState = TRUE;
	//	//	HighInputOutputPinsData.bPin2LowHighState = !bPowerOn;
	//	////	break;
	//	////case 3:
	//	//	HighInputOutputPinsData.bPin3InputOutputState = TRUE;
	//	//	HighInputOutputPinsData.bPin3LowHighState = !bPowerOn;
	//	////	break;
	//	////case 4:
	//	//	HighInputOutputPinsData.bPin4InputOutputState = TRUE;
	//	//	HighInputOutputPinsData.bPin4LowHighState = !bPowerOn;

	//	//	HighInputOutputPinsData.bPin5InputOutputState = TRUE;
	//	//	HighInputOutputPinsData.bPin5LowHighState = !bPowerOn;

	//		HighInputOutputPinsData.bPin6InputOutputState = TRUE;
	//		HighInputOutputPinsData.bPin6LowHighState = !bPowerOn;




	//		break;
	//}
	
	
	Status = JTAG_SetHiSpeedDeviceGPIOs(g_ftHandle, TRUE, &LowInputOutputPinsData, TRUE, &HighInputOutputPinsData);


	fprintf(data_all_log,"power control %d\n", bPowerOn);
	//fflush(data_all_log);
	check_log_file();

	return Status;
}

U32 jtag_close()
{
	JTAG_Close(g_ftHandle);

	return 0;
}



