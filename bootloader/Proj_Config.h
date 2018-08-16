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
Filename     : Proj_Config.h
Version      : 
Date         : 
Author       : Tobey
Others: 
Modification History:
*******************************************************************************/
#ifndef __PROJ_CONFIG_H__
#define __PROJ_CONFIG_H__

/* BootLoader DONT use SUBSYSTEM_PU_NUM & SUBSYSTEM_PU_MAX, just for compile */
#define SUBSYSTEM_PU_NUM    g_ulPuNum     //define in Disk_Config.c (get value from P-Table)
#define SUBSYSTEM_PU_MAX    16   


//#define SSD_ARCH_VPU
#ifdef SSD_ARCH_VPU
#define CE_PER_PU                 0
#define SCAN_CE_PER_PU            1
#else
#define CE_PER_PU                 1 // 1 or 2
#define SCAN_CE_PER_PU            CE_PER_PU
#endif

#define DBG_Printf dbg_printf
extern void DBG_Getch(void);
#endif
