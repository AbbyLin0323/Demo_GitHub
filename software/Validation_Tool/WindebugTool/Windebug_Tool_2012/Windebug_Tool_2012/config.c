/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   config.c                                        
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

#include "defines.h"
#include "config.h"

 U32 syscfg_max_lba;
 U32 syscfg_max_user_lba;
 U32 syscfg_max_accessable_lba;

 void config_init()
 {
	 syscfg_max_lba = MAX_DISK_LBA;
	 syscfg_max_user_lba = MAX_DISK_LBA;
	 syscfg_max_accessable_lba = MAX_DISK_LBA;
 }