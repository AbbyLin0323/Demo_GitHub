/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   hardwareapi.h                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: 
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 first created
*************************************************/

#ifndef _HARDWARE_API_H
#define _HARDWARE_API_H



U32 jtag_power_control_init();
U32 jtag_power_control(BOOL bPowerOn,U32 bPowerID);
U32 jtag_close();

#endif