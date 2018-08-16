/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : HAL_PM.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120201     Peter Xiu     001 first create
20121017    Peter Xiu   002 merge into trunk
*************************************************/
#include "BaseDef.h"
#include "HAL_Define.h"
#include "HAL_Pm.h"

U32 gHalIdleCounterSata;
U32 gHalIdleCounterSystem;
U32 gHalSystemPowerStage;

void HalPMInit()
{
    gHalIdleCounterSata = 0;
    gHalIdleCounterSystem = 0;
}


U32 HalSataPMDipmStatus()
{
    return TRUE;

}

void HalSataSetPowerMode(U8 mode)
{

}


U32 HalSataGetPowerMode()
{
    return SATA_POWER_MODE_ACTIVE;
}


U32 HalSataAutoPtSEnable()
{
    return TRUE;
}

void HalPMSataPMSchedule()
{

    if(TRUE == HalSataPMDipmStatus())
    {
        gHalIdleCounterSata = rPM_TIMER1_COUNTER;

        if(gHalIdleCounterSata < HAL_THRESHOLD_S0)
            return;
        else if(gHalIdleCounterSata < HAL_THRESHOLD_S1)
        {
            //ensure can changing power mode
            if(SATA_POWER_MODE_IDLE == HalSataGetPowerMode())
            {
                HalSataSetPowerMode(SATA_POWER_MODE_PARTIAL);
            }
            else
                return;
        }
        else
        {
            //need goes into slumber status
            if(SATA_POWER_MODE_IDLE == HalSataGetPowerMode())
            {
                HalSataSetPowerMode(SATA_POWER_MODE_SLUMBER);
            }
            else if(SATA_POWER_MODE_PARTIAL == HalSataGetPowerMode())
            {
                if(HalSataAutoPtSEnable())
                    HalSataSetPowerMode(SATA_POWER_MODE_SLUMBER);
                else
                    HalSataSetPowerMode(SATA_POWER_MODE_ACTIVE);

            }
            else
            {
                //slumber
                //do nothing
            }
        }

    }

    return;

}

void HalPMSetSystemPowerStage(U32 PowerStage)
{
    gHalSystemPowerStage = PowerStage;
}


U32 HalPMGetSystemPowerStage()
{
    return rPM_TIMER0_COUNTER;
}


void HalPMSleep(U32 Time,U8 bInteralTimer,U32 bDeepSleep)
{

}


void HalPMPLLOff()
{

}

void HalPMPLLOn()
{

}

void HalPMPowerOn()
{

}

void HalPMPowerOff()
{

}


U32 HalMcuGetTickCount()
{
    return 0;
}

/********************** FILE END ***************/
