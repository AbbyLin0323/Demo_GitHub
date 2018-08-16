#/*******************************************************************************
#* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
#*                                                                              *
#* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
#* and may contain trade secrets and/or other confidential information of VIA   *
#* Technologies,Inc.                                                            *
#* This file shall not be disclosed to any third party, in whole or in part,    *
#* without prior written consent of VIA.                                        *
#*                                                                              *
#* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
#* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
#* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
#* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
#* NON-INFRINGEMENT.                                                            *
#********************************************************************************
#Filename    : Makefile
#Version     : Ver 0.1
#Author      : Haven
#Date        : 2015.03.11
#Description : this is a Makefile file used in source code directory.
#Usage       : users maintain code in this dir need to known how to use this Makefile
#              the only thing need to do is add your .c file to this refer src list
#Modify      :
#20150311    Haven create file
#********************************************************************************

########################################################
# mcu0 source code
########################################################
SOURCE_MCU0_COMMON =
SOURCE_MCU0_ADP_SATA =
SOURCE_MCU0_ADP_AHCI = HAL_HwDebug.c
SOURCE_MCU0_ADP_NVME = HAL_HwDebug.c

########################################################
# mcu12 source code, exclude flash related source code
########################################################
SOURCE_MCU1_COMMON =
SOURCE_MCU1_ADP_SATA =
SOURCE_MCU1_ADP_AHCI = HAL_HwDebug.c
SOURCE_MCU1_ADP_NVME = HAL_HwDebug.c

SOURCE_MCU2_ADP_NVME = HAL_HwDebug.c

SOURCE_MIXVECTOR = HAL_HwDebug.c

################################################
# include configurations for Makefile, 
# designer pls do not modify this line
################################################
include ${ROOT}\config.mk

