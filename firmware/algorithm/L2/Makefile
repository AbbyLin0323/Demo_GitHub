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
SOURCE_MCU0_ADP_AHCI =
SOURCE_MCU0_ADP_NVME =

########################################################
# mcu12 source code, exclude flash related source code
########################################################
SOURCE_MCU1_COMMON = L2_TableBBT.c L2_Boot.c L2_Debug.c L2_TLCMerge.c L2_Evaluater.c L2_DWA.c L2_Erase.c L2_ErrorHandling.c L2_FTL.c L2_FullRecovery.c L2_GCManager.c L2_Init.c L2_LLF.c L2_PBIT.c L2_PMTI.c L2_PMTManager.c L2_PMTPage.c L2_RPMT.c L2_RT.c L2_Schedule.c L2_SearchEngine.c L2_Shutdown.c L2_StaticWL.c L2_StripeInfo.c L2_TableBlock.c L2_TblChk.c L2_Thread.c L2_Trim.c L2_VBT.c L2_ReadDisturb.c L2_Interface.c L2_FCMDQ.c
SOURCE_MCU1_ADP_SATA =
SOURCE_MCU1_ADP_AHCI =
SOURCE_MCU1_ADP_NVME =

SOURCE_MCU2_COMMON = L2_FCMDQ.c L2_TableBBT.c L2_Interface.c L2_SearchEngine.c L2_ErrorHandling.c L2_PBIT.c L2_ReadDisturb.c

########################################################
# mcu2 no source code, but used this dir for header path
########################################################
CD=${shell cd}
HPATH_MCU2_COMMON += $(CD)

################################################
# include configurations for Makefile, 
# designer pls do not modify this line
################################################
include ${ROOT}\config.mk

