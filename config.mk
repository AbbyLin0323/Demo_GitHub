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
#Description : user config file for makefile, maintained by every firmware designer
#Usage       : user need to set PLATFORM/HOST/MCU/FLASH for your project
#Modify      :
#20150311    Haven create file
#********************************************************************************
################################################
# RELEASE_VERSION = <arch>.<major>.<minor>
################################################
RELEASE_VERSION = 3.003.10

################################################
# BR_RELEASE_VERSION = Major.Minor.PatchLevel
################################################
BR_RELEASE_VERSION = 3.0.0

################################################
# PLATFORM = ASIC | FPGA | XTMP | COSIM
################################################
PLATFORM = ASIC

################################################
# MCU_CORE = B0 | C0 | A0
################################################
MCU_CORE = A0

################################################
# HOST = NVME | SATA
################################################
HOST = SATA

################################################
# FALSH = TSB_2D_TLC | L95 | TSB_3D_TLC | TSB_FOURPLN |INTEL_3D_TLC | YMTC_3D_MLC
################################################
FLASH = YMTC_3D_MLC

####################################################################
# FLASH = INTEL_3D_TLC
# BOARD = VT6734-INTEL_128GB1L4P | VT6734-INTEL_256GB1L4P | VT6734-MICRON_128GB1L4P | VT6734-MICRON_256GB1L4P | VT3084-B17A_MICRON_120GB1L4P | VT3084-YMTC_128GB1L1P
#         VT6739-INTEL_128GB1L4P | VT6739-INTEL_256GB1L4P | VT6739-MICRON_128GB1L4P | VT6739-MICRON_256GB1L4P | VT3084-B16A_MICRON_120GB1L2P
####################################################################
BOARD = VT6742-YMTC_128GB1L1P

################################################
# switch for generate Tracelog format .ini file
# TL_FORMAT_SWITCH = ON | OFF
################################################
TL_FORMAT_SWITCH = OFF

################################################
# obj dump switch, defined dump the symbol table or not.
# OBJDUMP_SWITCH = ON | OFF
################################################
OBJDUMP_SWITCH = OFF

################################################################################################
# HW function selection, such as  
# DMAE_ENABLE, DCACHE, SEARCH_ENGINE, DAUL_LUN_PER_PU/FOUR_LUN_PER_PU, FLASH_CACHE_OPERATION, CE_DECODE,
# DRAMLESS_ENABLE, PMT_ITEM_SIZE_REDUCE, CACHE_LINK_MULTI_LCT, NEW_SWL, DATA_EM_ENABLE, UECC_SOFT_DECODE_EN
################################################################################################
COMPILE_MACRO += DMAE_ENABLE DCACHE SEARCH_ENGINE PMT_ITEM_SIZE_REDUCE NEW_SWL UECC_SOFT_DECODE_EN

#############################################################################
# FW function selection, such as  
# L1_FAKE, L2_FAKE, HOST_CMD_REC, HAL_UNIT_TEST, L3_UNIT_TEST, L3_RDT_TEST
#############################################################################
COMPILE_MACRO +=

################################################
# tensilica environment setting
################################################
XTENSA_ROOT_DIR=C:\usr\Xtensa


################################################
# include normal config for Makefile, 
# pls do not modify this line and this file
################################################
include $(ROOT)\firmware\normal.mk

