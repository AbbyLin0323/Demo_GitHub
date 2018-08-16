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
#Description : normal setup for makefile, maintained by Makefile team
#Usage       : firmware designer do not modify this file
#Modify      :
#20150311    Haven create file
#********************************************************************************

###########################################################
# tensilica compiler related configuration
###########################################################
CD=${shell cd}
LISTDIR=$(ROOT)\Bin\list

RMV=del /s /q
MAKE=xt-make

CONFIG_$(FLASH) = PIGGY
export CONFIG_$(FLASH)

################################################
# recruise subdir makefile
################################################
SUB_DIRS = ${shell dir /AD /B | find /v ".svn"}

define recursion_dep
	@for %%i in ($(SUB_DIRS)) do if EXIST %%i\Makefile ($(MAKE) -C %%i rdep)
endef

rdep:addsource
ifneq ($(SUB_DIRS),)
	@for %%i in ($(SUB_DIRS)) do if EXIST %%i\Makefile ($(MAKE) -C %%i rdep)
endif

romdep:addromsource
ifneq ($(SUB_DIRS),)
	@for %%i in ($(SUB_DIRS)) do if EXIST %%i\Makefile ($(MAKE) -C %%i romdep)
endif

##################################################
# variables used in sub Makefiles, refer to these
##################################################
#SOURCE_MCU0_COMMON =
#SOURCE_MCU0_ADP_SATA =
#SOURCE_MCU0_ADP_AHCI =
#SOURCE_MCU0_ADP_NVME =
#
#SOURCE_MCU1_COMMON =
#SOURCE_MCU1_ADP_SATA =
#SOURCE_MCU1_ADP_AHCI =
#SOURCE_MCU1_ADP_NVME =
#
#SOURCE_FLASH_TSB_2D_TLC =
#SOURCE_FLASH_L95 =
#SOURCE_FLASH_TSB_3D_TLC =
#SOURCE_FLASH_INTEL_3D_TLC =
#SOURCE_FLASH_TSB_FOURPLN =
#
#HPATH_MCU0_COMMON =
#HPATH_MCU0_ADP_SATA =
#HPATH_MCU0_ADP_AHCI =
#HPATH_MCU0_ADP_NVME =
#
#HPATH_MCU1_COMMON =
#HPATH_MCU1_ADP_SATA =
#HPATH_MCU1_ADP_AHCI =
#HPATH_MCU1_ADP_NVME =
#
#HPATH_FLASH_TSB_2D_TLC =
#HPATH_FLASH_L95 =
#HPATH_FLASH_TSB_3D_TLC =
#HPATH_FLASH_INTEL_3D_TLC =
#HPATH_FLASH_TSB_FOURPLN =
#
#SOURCE_RESET_VECTOR =
#SOURCE_HEAD =
#SOURCE_OTHERS =
#
################################################
# add source file list to the $ROOT\*.temp
################################################

addsource:
ifneq ($(SOURCE_MCU0_COMMON),)
	@echo HPATH_MCU0_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU0_COMMON.list
	@for %%i in ($(SOURCE_MCU0_COMMON)) do echo SOURCE_MCU0_COMMON += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU0_COMMON.list
endif

ifneq ($(SOURCE_MCU0_ADP_SATA),)
	@echo HPATH_MCU0_ADP_SATA += -I $(CD)>>$(LISTDIR)\HPATH_MCU0_ADP_SATA.list
	@for %%i in ($(SOURCE_MCU0_ADP_SATA)) do echo SOURCE_MCU0_ADP_SATA += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU0_ADP_SATA.list
endif

ifneq ($(SOURCE_MCU0_ADP_AHCI),)
	@echo HPATH_MCU0_ADP_AHCI += -I $(CD)>>$(LISTDIR)\HPATH_MCU0_ADP_AHCI.list
	@for %%i in ($(SOURCE_MCU0_ADP_AHCI)) do echo SOURCE_MCU0_ADP_AHCI += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU0_ADP_AHCI.list
endif

ifneq ($(SOURCE_MCU0_ADP_NVME),)	
	@echo HPATH_MCU0_ADP_NVME += -I $(CD)>>$(LISTDIR)\HPATH_MCU0_ADP_NVME.list
	@for %%i in ($(SOURCE_MCU0_ADP_NVME)) do echo SOURCE_MCU0_ADP_NVME += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU0_ADP_NVME.list
endif

ifneq ($(SOURCE_MCU1_COMMON),)	
	@echo HPATH_MCU1_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU1_COMMON.list
	@for %%i in ($(SOURCE_MCU1_COMMON)) do echo SOURCE_MCU1_COMMON += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU1_COMMON.list
endif

ifneq ($(SOURCE_MCU1_ADP_SATA),)	
	@echo HPATH_MCU1_ADP_SATA += -I $(CD)>>$(LISTDIR)\HPATH_MCU1_ADP_SATA.list
	@for %%i in ($(SOURCE_MCU1_ADP_SATA)) do echo SOURCE_MCU1_ADP_SATA += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU1_ADP_SATA.list
endif

ifneq ($(SOURCE_MCU1_ADP_AHCI),)	
	@echo HPATH_MCU1_ADP_AHCI += -I $(CD)>>$(LISTDIR)\HPATH_MCU1_ADP_AHCI.list
	@for %%i in ($(SOURCE_MCU1_ADP_AHCI)) do echo SOURCE_MCU1_ADP_AHCI += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU1_ADP_AHCI.list
endif

ifneq ($(SOURCE_MCU1_ADP_NVME),)	
	@echo HPATH_MCU1_ADP_NVME += -I $(CD)>>$(LISTDIR)\HPATH_MCU1_ADP_NVME.list
	@for %%i in ($(SOURCE_MCU1_ADP_NVME)) do echo SOURCE_MCU1_ADP_NVME += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU1_ADP_NVME.list
endif


#MCU2
ifneq ($(SOURCE_MCU2_COMMON),)	
	@echo HPATH_MCU2_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU2_COMMON.list
	@for %%i in ($(SOURCE_MCU2_COMMON)) do echo SOURCE_MCU2_COMMON += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU2_COMMON.list
endif

ifneq ($(SOURCE_MCU2_ADP_SATA),)	
	@echo HPATH_MCU2_ADP_SATA += -I $(CD)>>$(LISTDIR)\HPATH_MCU2_ADP_SATA.list
	@for %%i in ($(SOURCE_MCU2_ADP_SATA)) do echo SOURCE_MCU2_ADP_SATA += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU2_ADP_SATA.list
endif

ifneq ($(SOURCE_MCU2_ADP_AHCI),)	
	@echo HPATH_MCU2_ADP_AHCI += -I $(CD)>>$(LISTDIR)\HPATH_MCU2_ADP_AHCI.list
	@for %%i in ($(SOURCE_MCU2_ADP_AHCI)) do echo SOURCE_MCU2_ADP_AHCI += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU2_ADP_AHCI.list
endif

ifneq ($(SOURCE_MCU2_ADP_NVME),)	
	@echo HPATH_MCU2_ADP_NVME += -I $(CD)>>$(LISTDIR)\HPATH_MCU2_ADP_NVME.list
	@for %%i in ($(SOURCE_MCU2_ADP_NVME)) do echo SOURCE_MCU2_ADP_NVME += $(CD)\%%i>>$(LISTDIR)\SOURCE_MCU2_ADP_NVME.list
endif


ifneq ($(SOURCE_FLASH_TSB_2D_TLC),)	
	@echo HPATH_FLASH_TSB_2D_TLC += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_TSB_2D_TLC.list
	@for %%i in ($(SOURCE_FLASH_TSB_2D_TLC)) do echo SOURCE_FLASH_TSB_2D_TLC += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_TSB_2D_TLC.list
endif

ifneq ($(SOURCE_FLASH_L95),)	
	@echo HPATH_FLASH_L95 += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_L95.list
	@for %%i in ($(SOURCE_FLASH_L95)) do echo SOURCE_FLASH_L95 += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_L95.list
endif

ifneq ($(SOURCE_FLASH_TSB_3D_TLC),)	
	@echo HPATH_FLASH_TSB_3D_TLC += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_TSB_3D_TLC.list
	@for %%i in ($(SOURCE_FLASH_TSB_3D_TLC)) do echo SOURCE_FLASH_TSB_3D_TLC += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_TSB_3D_TLC.list
endif

ifneq ($(SOURCE_FLASH_INTEL_3D_TLC),)	
	@echo HPATH_FLASH_INTEL_3D_TLC += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_INTEL_3D_TLC.list
	@for %%i in ($(SOURCE_FLASH_INTEL_3D_TLC)) do echo SOURCE_FLASH_INTEL_3D_TLC += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_INTEL_3D_TLC.list
endif

ifneq ($(SOURCE_FLASH_INTEL_3D_MLC),)	
	@echo HPATH_FLASH_INTEL_3D_MLC += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_INTEL_3D_MLC.list
	@for %%i in ($(SOURCE_FLASH_INTEL_3D_MLC)) do echo SOURCE_FLASH_INTEL_3D_MLC += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_INTEL_3D_MLC.list
endif

ifneq ($(SOURCE_FLASH_TSB_FOURPLN),)	
	@echo HPATH_FLASH_TSB_FOURPLN += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_TSB_FOURPLN.list
	@for %%i in ($(SOURCE_FLASH_TSB_FOURPLN)) do echo SOURCE_FLASH_TSB_FOURPLN += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_TSB_FOURPLN.list
endif

ifneq ($(SOURCE_FLASH_YMTC_3D_MLC),)	
	@echo HPATH_FLASH_YMTC_3D_MLC += -I $(CD)>>$(LISTDIR)\HPATH_FLASH_YMTC_3D_MLC.list
	@for %%i in ($(SOURCE_FLASH_YMTC_3D_MLC)) do echo SOURCE_FLASH_YMTC_3D_MLC += $(CD)\%%i>>$(LISTDIR)\SOURCE_FLASH_YMTC_3D_MLC.list
endif

ifneq ($(SOURCE_RESET_VECTOR),)
	@for %%i in ($(SOURCE_RESET_VECTOR)) do echo SOURCE_RESET_VECTOR += $(CD)\%%i>>$(LISTDIR)\ASSEMBLY.list
endif

ifneq ($(SOURCE_OTHERS),)
	@for %%i in ($(SOURCE_OTHERS)) do echo SOURCE_OTHERS += $(CD)\%%i>>$(LISTDIR)\ASSEMBLY.list
endif

ifneq ($(SOURCE_HEAD),)
	@for %%i in ($(SOURCE_HEAD)) do echo SOURCE_HEAD += $(CD)\%%i>>$(LISTDIR)\ASSEMBLY.list
endif

ifneq ($(SOURCE_PIGGY),)
	@for %%i in ($(SOURCE_PIGGY)) do echo SOURCE_PIGGY += $(CD)\%%i>>$(LISTDIR)\ASSEMBLY.list
endif

ifneq ($(HPATH_MCU0_COMMON),)
	@echo HPATH_MCU0_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU0_COMMON.list
endif

ifneq ($(HPATH_MCU1_COMMON),)
	@echo HPATH_MCU1_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU1_COMMON.list
endif

ifneq ($(HPATH_MCU2_COMMON),)
	@echo HPATH_MCU2_COMMON += -I $(CD)>>$(LISTDIR)\HPATH_MCU2_COMMON.list
endif

ifneq ($(SOURCE_BOOTLOADER),)	
	@for %%i in ($(SOURCE_BOOTLOADER)) do echo SOURCE_BOOTLOADER += $(CD)\%%i>>$(LISTDIR)\SOURCE_BOOTLOADER.list
endif

ifneq ($(HPATH_BOOTLOADER),)
	@echo HPATH_BOOTLOADER += -I $(CD)>>$(LISTDIR)\HPATH_BOOTLOADER.list
endif

ifneq ($(SOURCE_MIXVECTOR),)	
	@echo HPATH_MIXVECTOR += -I $(CD)>>$(LISTDIR)\HPATH_MIXVECTOR.list
	@for %%i in ($(SOURCE_MIXVECTOR)) do echo SOURCE_MIXVECTOR += $(CD)\%%i>>$(LISTDIR)\SOURCE_MIXVECTOR.list
endif

ifneq ($(HPATH_MIXVECTOR),)
	@echo HPATH_MIXVECTOR += -I $(CD)>>$(LISTDIR)\HPATH_MIXVECTOR.list
endif

addrootsource:
	@echo SOURCE_MCU0_COMMON += $(CD)\MCU0_Main.c>>$(LISTDIR)\SOURCE_MCU0_COMMON.list
	@echo SOURCE_MCU1_COMMON += $(CD)\MCU1_Main.c>>$(LISTDIR)\SOURCE_MCU1_COMMON.list
	@echo SOURCE_MCU2_COMMON += $(CD)\MCU2_Main.c>>$(LISTDIR)\SOURCE_MCU2_COMMON.list

addromsource:
	@echo HPATH_ROM += $(CD)>>$(LISTDIR)\SOURCE_ROM.list
ifneq ($(SOURCE_ROM_TEMP),)	
	@for %%i in ($(SOURCE_ROM_TEMP)) do echo SOURCE_ROM += $(CD)\%%i>>$(LISTDIR)\SOURCE_ROM.list
endif
ifneq ($(SOURCE_RESET_VECTOR),)
	@for %%i in ($(SOURCE_RESET_VECTOR)) do echo SOURCE_RESET_VECTOR += $(CD)\%%i>>$(LISTDIR)\SOURCE_ROM.list
endif
ifneq ($(SOURCE_OTHERS),)
	@for %%i in ($(SOURCE_OTHERS)) do echo SOURCE_OTHERS += $(CD)\%%i>>$(LISTDIR)\SOURCE_ROM.list
endif
################################################
# 
################################################
