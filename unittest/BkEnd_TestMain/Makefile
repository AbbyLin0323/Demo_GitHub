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
#Description : Main Makefile in Firmware dir, all compile design is implement here
#Usage       : called by root Makefile at the upper directory
#Modify      :
#20150311    Haven create file
#********************************************************************************
#.PHONY: dep

########################################################
# mcu12 source code, exclude flash related source code
########################################################
SOURCE_MCU1_COMMON = BkEnd_TestMain.c
SOURCE_MCU1_ADP_SATA =
SOURCE_MCU1_ADP_AHCI =
SOURCE_MCU1_ADP_NVME =

SOURCE_MCU2_COMMON = BkEnd_TestMain.c

########################################################
# no source code but headpath used this directory
########################################################

include $(ROOT)\config.mk

#dep:
#	$(call recursion_dep)
