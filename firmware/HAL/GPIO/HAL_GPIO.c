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
Filename    : HAL_GPIO.c
Version     : Ver 1.0
Author      : tobey
Date        : 2013.10.28
Description : this file encapsulate GPIO driver interface. 
Others      : 
Modify      :
20141028    Tobey     create file.
*******************************************************************************/

#include "HAL_GPIO.h"

/*----------------------------------------------------------------------------
Name: HalSetGPIOPinDirection
Description: 
    set one pin as input or output.
Input Param:
    U8 ucPinNum: pin number
    BOOL bPinOut: TRUE:set as output pin; FALSE:set as input pin
Output Param:
    none
Return Value:
    void
Usage:
    when use one GPIO pin, invoeke it to set one pin direction.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOPinDirection(U8 ucPinNum, BOOL bPinOut)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (TRUE == bPinOut)
    {
        rGPIOPD |= 1 << ucPinNum;
    }
    else
    {
        rGPIOPD &= ~(1 << ucPinNum);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HalSetGPIOIntRisingEdge
Description: 
    set one pin be detected when rising-edge or not,
Input Param:
    U8 ucPinNum: pin number
    BOOL bEn: TRUE:pin  be detedted when rising-edge; FALSE:pin not be detedted when rising-edge
Output Param:
    none
Return Value:
    void
Usage:
    when use one pin as GPIO rising-edge interrupt pin, invoke it to set it.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOIntRisingEdge(U8 ucPinNum, BOOL bEn)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (TRUE == bEn)
    {
        rGPIORE |= 1 << ucPinNum;
    }
    else
    {
        rGPIORE &= ~(1 << ucPinNum);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HalSetGPIOIntFallingEdge
Description: 
    set one pin be detected when Falling-edge or not,
Input Param:
    U8 ucPinNum: pin number
    BOOL bEn: TRUE:pin  be detedted when Falling-edge; FALSE:pin not be detedted when Falling-edge
Output Param:
    none
Return Value:
    void
Usage:
    when use one pin as GPIO Falling-edge interrupt pin, invoke it to set it.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOIntFallingEdge(U8 ucPinNum, BOOL bEn)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (TRUE == bEn)
    {
        rGPIOFE |= 1 << ucPinNum;
    }
    else
    {
        rGPIOFE &= ~(1 << ucPinNum);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_IsGPIOEdgeOccur
Description: 
    Check one pin GPIO rasing-edge or falling-edge has occured.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    BOOL:TRUE:occured; FALSE:not occured.
Usage:
    FW invoke it to Check if one pin GPIO rasing-edge or falling-edge has occured.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_IsGPIOEdgeOccur(U8 ucPinNum)
{
    BOOL bEdgeOccur = FALSE;
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (0 != (rGPIOED & (1 << ucPinNum)))
    {
        bEdgeOccur =  TRUE;
    }
    
    return bEdgeOccur;
}

/*----------------------------------------------------------------------------
Name: HalClearGPIOEdgeSts
Description: 
    clear one GPIO pin edge occur sts.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to clear one GPIO pin edge occur sts.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalClearGPIOEdgeSts(U8 ucPinNum)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }
    
    rGPIOED |= 1 << ucPinNum;
    return;
}

/*----------------------------------------------------------------------------
Name: HalSetGPIOIntTriggerEn
Description: 
    enable one pin's interrupt.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to enable one pin's interrupt.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOIntTriggerEn(U8 ucPinNum)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    rGPIOIS |= 1 << ucPinNum;

    return;
}

/*----------------------------------------------------------------------------
Name: HalClearGPIOIntTriggerEn
Description:
    disable one pin's interrupt.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to disable one pin's interrupt.
History:
    20141202    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalClearGPIOIntTriggerEn(U8 ucPinNum)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    rGPIOIS &= ~(1 << ucPinNum);

    return;
}

/*----------------------------------------------------------------------------
Name: HalSetGPIOIntMode
Description: 
    set one pin's interrupt mode.
Input Param:
    U8 ucPinNum: pin number
    BOOL bLevel: TRUE:level trigger; FALSE:edge trigger.
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to set one pin's interrupt mode.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOIntMode(U8 ucPinNum, BOOL bLevel)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (TRUE == bLevel)
    {
        rGPIOIM |= 1 << ucPinNum;
    }
    else
    {
        rGPIOIM &= ~(1 << ucPinNum);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HalSetGPIOIntMode
Description: 
    set one pin's level interrupt mode.
Input Param:
    U8 ucPinNum: pin number
    BOOL bLevel: TRUE:low level trigger; FALSE:high level trigger.
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to set one pin's level interrupt mode.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalSetGPIOLevelIntMode(U8 ucPinNum, BOOL bLowLevel)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }
    
    if (TRUE == bLowLevel)
    {
        rGPIOLIS |= 1 << ucPinNum;
    }
    else
    {
        rGPIOLIS &= ~(1 << ucPinNum);
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_IsGPIOLevelIntOccur
Description: 
    check if one pin's level interrupt occured.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to check if one pin's level interrupt occured.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_IsGPIOLevelIntOccur(U8 ucPinNum)
{
    BOOL bLevelIntOccur = FALSE;
    
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    if (0 != (rGPIOLD &(1 << ucPinNum)))
    {
        bLevelIntOccur = TRUE;
    }

    return bLevelIntOccur;
}

/*----------------------------------------------------------------------------
Name: HalClearGPIOLevelSts
Description: 
    clear one pin's level interrupt occur sts .
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to clear one pin's level interrupt occur sts .
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
LOCAL MCU12_DRAM_TEXT void HalClearGPIOLevelSts(U8 ucPinNum)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Getch();
    }

    rGPIOLD |= 1 << ucPinNum;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GPIORasingEdgeIntInit
Description: 
    Initial one GPIO pin as rasing-edge trigger interrupt input pin.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to Initial one GPIO pin as rasing-edge trigger interrupt input pin.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIORasingEdgeIntInit(U8 ucPinNum)
{
    HalSetGPIOPinDirection(ucPinNum, FALSE);
    HalSetGPIOIntRisingEdge(ucPinNum, TRUE);
    HalSetGPIOIntMode(ucPinNum, FALSE);
    HalSetGPIOIntTriggerEn(ucPinNum);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GPIOFallingEdgeIntInit
Description: 
    Initial one GPIO pin as falling-edge trigger interrupt input pin.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to Initial one GPIO pin as falling-edge trigger interrupt input pin.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIOFallingEdgeIntInit(U8 ucPinNum)
{
    HalSetGPIOPinDirection(ucPinNum, FALSE);
    HalSetGPIOIntFallingEdge(ucPinNum, TRUE);
    HalSetGPIOIntMode(ucPinNum, FALSE);
    HalSetGPIOIntTriggerEn(ucPinNum);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GPIOHighLevelIntInit
Description: 
    Initial one GPIO pin as high level trigger interrupt input pin.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to Initial one GPIO pin as high level trigger interrupt input pin.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIOHighLevelIntInit(U8 ucPinNum)
{
    HalSetGPIOPinDirection(ucPinNum, FALSE);
    HalSetGPIOIntMode(ucPinNum, TRUE);
    HalSetGPIOLevelIntMode(ucPinNum, FALSE);
    HalSetGPIOIntTriggerEn(ucPinNum);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GPIOLowLevelIntInit
Description: 
    Initial one GPIO pin as low level trigger interrupt input pin.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to Initial one GPIO pin as low level trigger interrupt input pin.
History:
    20141028    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIOLowLevelIntInit(U8 ucPinNum)
{
    HalSetGPIOPinDirection(ucPinNum, FALSE);
    HalSetGPIOIntMode(ucPinNum, TRUE);
    HalSetGPIOLevelIntMode(ucPinNum, TRUE);
    HalSetGPIOIntTriggerEn(ucPinNum);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_GPIOClearEdgeInt
Description:
    clear one GPIO pin's edge interrupt.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to clear one GPIO pin's edge interrupt.
History:
    20141202    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIOClearEdgeInt(U8 ucPinNum)
{
    HalClearGPIOIntTriggerEn(ucPinNum);
    HalClearGPIOEdgeSts(ucPinNum);
}

/*----------------------------------------------------------------------------
Name: HAL_GPIOClearLevelInt
Description:
    clear one GPIO pin's level interrupt.
Input Param:
    U8 ucPinNum: pin number
Output Param:
    none
Return Value:
    void
Usage:
    FW invoke it to clear one GPIO pin's level interrupt.
History:
    20141202    Tobey   create
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_GPIOClearLevelInt(U8 ucPinNum)
{
    HalClearGPIOIntTriggerEn(ucPinNum);
    HalClearGPIOLevelSts(ucPinNum);
}

void SetGPIOPinValue(U8 ucPinNum, U8 bPinValue)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Printf("Wrong Pin number\n");
        DBG_Getch();
    }

    /* bPinValue 1: Set output value 1, 0: Set output value 0 */
    if (1 == bPinValue)
    {
        if (ucPinNum >= 32)
        {
            rGPIOPS_H |= 1 << (ucPinNum - 32);
        }
        else
        {
            rGPIOPS |= 1 << ucPinNum;
        }
    }
    else
    {
        if (ucPinNum >= 32)
        {
            rGPIOPC_H |= 1 << (ucPinNum - 32);
        }
        else
        {
            rGPIOPC |= 1 << ucPinNum;
        }
    }
}

void SetGPIOPinDirection(U8 ucPinNum, BOOL bPinOut)
{
    if (ucPinNum >= GPIO_PIN_MAX)
    {
        DBG_Printf("Wrong Pin number\n");
        DBG_Getch();
    }

    /* ture: GPO, false: GPI*/
    if (bPinOut == TRUE)
    {
        /*set GPDR_H bit */
        if (ucPinNum >= 32)
        {
            rGPIOPD_H |= 1 << (ucPinNum - 32);
        }
        else
        {
            rGPIOPD |= 1 << ucPinNum;
        }
    }
    else
    {
        /* set GPDR_H bit */
        if (ucPinNum >= 32)
        {
            rGPIOPD_H &= ~(1 << (ucPinNum - 32));
        }
        else
        {
            rGPIOPD &= ~(1 << ucPinNum);
        }
    }
    return;
}

/*end of file: HAL_GPIO.c */
