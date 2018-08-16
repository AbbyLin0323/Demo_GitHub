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
#Date        : 2015.03.23
#Description : Main Makefile for bootloader compile, all compile design is implement here
#Usage       : called by root Makefile at the upper directory
#Modify      :
#20150323    Haven create file
#********************************************************************************
.PHONY: all

include $(ROOT)\config.mk

ifneq ($(FLASH_IN),)
FLASH=$(FLASH_IN)
endif
ifneq ($(HOST_IN),)
HOST=$(HOST_IN)
endif
ifneq ($(BOARD_IN),)
BOARD=$(BOARD_IN)
endif

BINDIR_DOS=$(ROOT)\Bin\bin
BLDIR_DOS=$(ROOT)\Bin\obj\BootLoader
SRCDIR_DOS=$(ROOT)
BINDIR=$(subst \,/,$(BINDIR_DOS))
OBJDIR=$(subst \,/,$(OBJDIR_DOS))
SRCDIR=$(subst \,/,$(SRCDIR_DOS))
FLASH_OUT = $(FLASH)

BLSCRIPT_TOOLDIR_DOS=$(subst /,\,$(dir $(BLSCRIPT_TOOL)))
IMAGE_INTEGRATE_TOOL_DOS=$(subst /,\,$(IMAGE_INTEGRATE_TOOL))

IMAGE_INTEGRATE_TOOL_DIR=$(dir $(IMAGE_INTEGRATE_TOOL))
IMAGE_INTEGRATE_TOOL_DIR_DOS=$(subst /,\,$(IMAGE_INTEGRATE_TOOL_DIR))

BOOTLOADERBIN=$(BINDIR)/BootLoader_$(FLASH)_$(MCU_CORE)_$(PLATFORM).bin
TargetBin_MCU0=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU0.bin
TargetBin_MCU1=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU1.bin
TargetBin_MCU2=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU2.bin
$(if $(filter INTEL_3D_TLC, $(FLASH)), $(eval FLASH_OUT=3D_TLC))
TargetBin=$(BINDIR)/$(BOARD)_$(HOST)_$(FLASH_OUT)_$(PLATFORM).bin

BOOTLOADERBIN_DOS=$(subst /,\,$(BOOTLOADERBIN))
TargetBin_MCU0_DOS=$(subst /,\,$(TargetBin_MCU0))
TargetBin_MCU1_DOS=$(subst /,\,$(TargetBin_MCU1))
TargetBin_MCU2_DOS=$(subst /,\,$(TargetBin_MCU2))
TargetBin_DOS=$(subst /,\,$(TargetBin))

include $(ROOT)\version.mk

FORCE :

all:$(TargetBin) FORCE
	@echo [System Image Integration $(BOARD) $(HOST) $(FLASH) $(PLATFORM)]
	$(BLSCRIPT_TOOL) 0 $(BOARD)_$(HOST)_$(FLASH).env \$(BOARD)_$(HOST)_$(FLASH)
	copy $(BLSCRIPT_TOOLDIR_DOS)Output\$(BOARD)_$(HOST)_$(FLASH).env $(BINDIR_DOS)
	copy $(IMAGE_INTEGRATE_TOOL_DOS) $(BINDIR_DOS)
	copy $(IMAGE_INTEGRATE_TOOL_DIR_DOS)layout.cfg $(BINDIR_DOS)
	$(BINDIR_DOS)\$(notdir $(IMAGE_INTEGRATE_TOOL)) \
		layout.cfg \
		$(notdir $(BOOTLOADERBIN)) \
		$(BOARD)_$(HOST)_$(FLASH).env \
		$(notdir $(TargetBin_MCU0)) \
		$(notdir $(TargetBin_MCU1)) \
		$(notdir $(TargetBin_MCU2)) \
		$(notdir $(TargetBin))

$(TargetBin): FORCE

clean:
	@del /s /q $(BINDIR_DOS)\$(BOARD)_$(HOST)_$(FLASH).env
	@del /s /q $(TargetBin_DOS)
	@del /s /q $(BINDIR_DOS)\layout.cfg
	@del /s /q $(BINDIR_DOS)\$(notdir $(IMAGE_INTEGRATE_TOOL))
