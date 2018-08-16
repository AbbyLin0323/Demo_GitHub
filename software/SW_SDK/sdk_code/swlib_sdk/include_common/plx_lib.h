#ifndef _PLX_LIB
#define _PLX_LIB
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "stdlib.h"
#include ".\plx_include\PlxApi.h"
#if 1
/*
NVME: 0x8 = 32'h010802xx
AHCI: 0x8 = 32'h010601xx

*/
#define NVME_CLASS_CODE 0x010802
#define AHCI_CLASS_CODE 0x010601
#define NVME_MP_BAR 0
#define AHCI_MP_BAR 5
#define NVME_REG_OFFSET_IN_BAR 0x4200
#define AHCI_REG_OFFSET_IN_BAR 0xA0

#define MAX_SUPPORT_DEV 128
#define VIA_VENDID 0x1106
#define VIA_DEVICEID 0x3514
#define PLX_VENDID 0x10B5
#define PLX_DEVICEID 0x8714
#define INVALID_8F 0xffffffff

typedef enum _BACK_DOOR_CMD{
	OP_BD_WRITE,
	OP_BD_READ,
	OP_BD_JUMP
}BACK_DOOR_CMD;
typedef struct _BACK_DOOR_REG
{
	//U32 m_content[6];
	struct _DW0{
		U32 MP_SIGNATURE:16;
		U32 RSV_DW0:15;
		U32 MP_ENABLE:1;
	}DW0;

	struct _DW1{
		U32 RSV0_DW1:7;
		U32 INT_ENABLE:1;
		U32 MP_CMD_TYPE:8;
		U32 MP_CMD_STATUS:8;
		U32 RSV1_DW1:7;
		U32 MP_TRIG:1;
	}DW1;

	struct _DW2{
		U32 MP_PARA_0;
	}DW2;
	struct _DW3{
		U32 MP_PARA_1;
	}DW3;
	struct _DW4{
		U32 MP_PARA_2;
	}DW4;
	struct _DW5{
		U32 MP_PARA_3;
	}DW5;
}BACK_DOOR_REG;
typedef struct _PCIE_DEVICE
{
	PLX_DEVICE_OBJECT DevObj;
	U8 ucBus;
	U8 ucSlot;
	U8 ucFun;
	U8 ucTargetBar;
	U32 ulOffsetInBar;
	VOID * Va[PCI_NUM_BARS_TYPE_00];
	//U8 * MpAddr;
}PCIE_DEVICE ,*PPCIE_DEVICE;
#define HOT_PLUG_INTERRUPT_ENABLE           (1 <<  5)
#define PRESENCE_DETECT_CHANGE              (1 << 19)
#define PRESENCE_DETECT_STATE               (1 << 22)
#define ATTENTION_BUTTON_PRESSED            (1 << 16)
#define POWER_INDICATOR_CONTROL             (3 <<  8)
#define POWER_INDICATOR_CONTROL_BLINK       (2 <<  8)
#define POWER_INDICATOR_CONTROL_ON          (1 <<  8)
#define POWER_INDICATOR_CONTROL_OFF         (3 <<  8)
#define POWER_CONTROLLER_CONTROL            (1 << 10)
#endif

extern PCIE_DEVICE g_Pcie_Dev[MAX_SUPPORT_DEV];
#endif