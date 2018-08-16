/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :  HAL_SataIO.h                                         
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120201     peterxiu     001 first create
*************************************************/
#ifndef _HAL_PM_H
#define _HAL_PM_H

#define HAL_THRESHOLD_S0      50
#define HAL_THRESHOLD_S1      100

#define HAL_THRESHOLD_D0      200
#define HAL_THRESHOLD_D1      400
#define HAL_THRESHOLD_D2      800
#define HAL_THRESHOLD_D3      3000
#define HAL_THRESHOLD_D4      4000

#define HAL_PM_REQ_TYPE_STR              0   /*suspend to dram*/
#define HAL_PM_REQ_TYPE_LDR              1   /*restore form dram*/
#define HAL_PM_REQ_TYPE_POWERLOSS       2   /*perpare to power loss*/
#define HAL_PM_REQ_TYPE_POWERON         3   /*perpare to power on*/

#define SATA_POWER_MODE_ACTIVE             0
#define SATA_POWER_MODE_IDLE            1
#define SATA_POWER_MODE_PARTIAL         2
#define SATA_POWER_MODE_SLUMBER         3
#define SATA_POWER_MODE_DEVSLEEP        4

#define SYSTEM_POWER_MODE_WORKING           0
#define SYSTEM_POWER_MODE_IDLE              1
#define SYSTEM_POWER_MODE_STANDBY           2
#define SYSTEM_POWER_MODE_SLEEP             3
#define SYSTEM_POWER_MODE_OFF               4

/*reg define*/
#define PM_REG_BASE         REG_BASE_PMU/* ?, TODO: need to sync with HW design */
#define rPM_TIMER0_COUNTER     (*((volatile U32*)(PM_REG_BASE + 0x00)))
#define rPM_TIMER0_MATCH        (*((volatile U32*)(PM_REG_BASE + 0x04)))
#define rPM_TIMER0_START        (*((volatile U32*)(PM_REG_BASE + 0x08)))
#define rPM_TIMER0_STOP        (*((volatile U32*)(PM_REG_BASE + 0x0C)))
#define rPM_TIMER0_INTMSK        (*((volatile U32*)(PM_REG_BASE + 0x10)))
#define rPM_TIMER0_INTPEND        (*((volatile U32*)(PM_REG_BASE + 0x14)))

#define rPM_TIMER1_COUNTER     (*((volatile U32*)(PM_REG_BASE + 0x00)))
#define rPM_TIMER1_MATCH        (*((volatile U32*)(PM_REG_BASE + 0x04)))
#define rPM_TIMER1_START        (*((volatile U32*)(PM_REG_BASE + 0x08)))
#define rPM_TIMER1_STOP        (*((volatile U32*)(PM_REG_BASE + 0x0C)))
#define rPM_TIMER1_INTMSK        (*((volatile U32*)(PM_REG_BASE + 0x10)))
#define rPM_TIMER1_INTPEND        (*((volatile U32*)(PM_REG_BASE + 0x14)))

#define rPM_TIMER2_COUNTER     (*((volatile U32*)(PM_REG_BASE + 0x00)))
#define rPM_TIMER2_MATCH        (*((volatile U32*)(PM_REG_BASE + 0x04)))
#define rPM_TIMER2_START        (*((volatile U32*)(PM_REG_BASE + 0x08)))
#define rPM_TIMER2_STOP        (*((volatile U32*)(PM_REG_BASE + 0x0C)))
#define rPM_TIMER2_INTMSK        (*((volatile U32*)(PM_REG_BASE + 0x10)))
#define rPM_TIMER2_INTPEND        (*((volatile U32*)(PM_REG_BASE + 0x14)))


U32 HalMcuGetTickCount();


void HalPMInit();
U32 HalSataPMDipmStatus();
void HalSataSetPowerMode(U8 mode);
U32 HalSataGetPowerMode();
U32 HalSataAutoPtSEnable();
void HalPMSataPMSchedule();
void HalPMSetSystemPowerStage(U32 PowerStage);
U32 HalPMGetSystemPowerStage();
void HalPMSleep(U32 Time,U8 bInteralTimer,U32 bDeepSleep);

void HalPMPLLOff();
void HalPMPLLOn();
void HalPMPowerOn();
void HalPMPowerOff();

#endif

/********************** FILE END ***************/
