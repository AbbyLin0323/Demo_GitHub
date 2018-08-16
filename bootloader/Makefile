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

ifeq ($(MCU_CORE),A0)
XTENSA_CORE=rfviatieloop16
COMPILE_MACRO += VT3514_C0
else
$(error MCU_CORE($(MCU_CORE)) is unsupported)
XTENSA_CORE=viatie
endif

ifneq ($(MACRO_IN),)
COMPILE_MACRO += $(MACRO_IN)
endif

SWTOOLS=$(XTENSA_ROOT_DIR)\XtDevTools\install\tools\RF-2014.1-win32\XtensaTools
XTENSA_SYSTEM=$(XTENSA_ROOT_DIR)\XtDevTools\install\builds\RF-2014.1-win32\$(XTENSA_CORE)\config

CC=$(SWTOOLS)\bin\xt-xcc
CCPLUS=$(SWTOOLS)\bin\xt-xc++
COMPILEFLAGS = -g -O2 -ipa -mlongcalls -minsbits -fmessage-length=0
COMPILEFLAGSFORRESET=-Wa,--text-section-literals
COMPILEFLAGS_MIN=-Wl,-r -nostdlib $(COMPILEFLAGS)

COREFLAGS=--xtensa-system=$(XTENSA_SYSTEM) --xtensa-core=$(XTENSA_CORE) --xtensa-params=
RELATIVECORE=-DPROC_$(XTENSA_CORE) -DCONFIG_$(XTENSA_CORE)

BINDIR_DOS=$(ROOT)\Bin\bin
BLDIR_DOS=$(ROOT)\Bin\obj\BootLoader
OBJDIR_DOS=$(BLDIR_DOS)\$(FLASH)
SRCDIR_DOS=$(ROOT)
BINDIR=$(subst \,/,$(BINDIR_DOS))
OBJDIR=$(subst \,/,$(OBJDIR_DOS))
SRCDIR=$(subst \,/,$(SRCDIR_DOS))

TargetElf=$(BINDIR)/BootLoader_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf
TargetBin=$(patsubst %.elf,%.bin,$(TargetElf))

all:$(TargetBin)
	@echo [BootLoader $(FLASH) $(MCU_CORE) $(PLATFORM)] compile complete

include $(ROOT)\version.mk
	
$(TargetBin):$(TargetElf)
	@echo generate $(notdir $@)
	@$(DUMP_BIN_TOOL) BL $< $(VER_INFO) $@
	
ifneq ($(CTYPE),unincludedep)
-include $(LISTDIR)/SOURCE_BOOTLOADER.list $(LISTDIR)/SOURCE_FLASH_$(FLASH).list
-include $(LISTDIR)/HPATH_BOOTLOADER.list $(LISTDIR)/HPATH_FLASH_$(FLASH).list
endif
	
SOURCE = $(subst \,/,$(SOURCE_BOOTLOADER) $(SOURCE_FLASH_$(FLASH)) $(addprefix $(CD)/, $(wildcard *.c)))

HPATH = $(addprefix -I, $(dir $(SOURCE))) $(subst \,/,$(HPATH_BOOTLOADER))

SOURCE_RESET_VECTOR = $(CD)\vector.S
SOURCE_OTHERS = $(CD)\crt1-tiny.S

OBJ_RESET_VECTOR = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_RESET_VECTOR:%.S=%.o)))
OBJ_OTHERS       = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_OTHERS:%.S=%.o)))

OBJ = $(subst $(SRCDIR),$(OBJDIR),$(SOURCE:%.c=%.o)) $(OBJ_RESET_VECTOR) $(OBJ_OTHERS)

DEP = $(OBJ:%.o=%.d)

COMPILE_MACRO+= BOOTLOADER
COMPILEFLAGS += -Os
CMACRO=$(addprefix -D, $(PLATFORM) $(COMPILE_MACRO))
CFLAGS=-c $(COMPILEFLAGS) $(HPATH) $(RELATIVECORE) $(COREFLAGS) $(CMACRO)

LSP=$(ROOT)/ENV/LSP/VT3533_$(MCU_CORE)_BOOTLOADER_LSP

$(TargetElf):$(OBJ)
	@if NOT EXIST $(subst /,\,$(dir $@)) (mkdir $(subst /,\,$(dir $@)))
	@echo generate $(notdir $@)
	@$(CCPLUS) $(OBJ) -o $@ -mlsp=$(LSP) $(COMPILEFLAGS) $(COREFLAGS)
	
$(OBJDIR)%.d:$(SRCDIR)%.c
	@if NOT EXIST $(subst /,\,$(dir $@)) (mkdir $(subst /,\,$(dir $@)))
	@echo generate $(notdir $@)
	@$(CC) -MM $< $(HPATH) $(CMACRO) -MT $(patsubst %.d,%.o,$@) > $@
	@echo $(subst ',,'	@echo compile $(notdir $<)')>>$@
	@echo $(subst ',,'	@$(CC) $(CFLAGS) $< -o $(patsubst %.d,%.o,$@)')>>$@
	
ifneq ($(CTYPE),unincludedep)
-include $(DEP)
endif
	
$(OBJ_RESET_VECTOR):$(OBJDIR)%.o:$(SRCDIR)%.S
	@if NOT EXIST $(subst /,\,$(dir $@)) (mkdir $(subst /,\,$(dir $@)))
	@echo compile $(notdir $<)
	@$(CC) -c $(COMPILEFLAGSFORRESET) $(HPATH) $(RELATIVECORE) $(COREFLAGS) $< -o $@

$(OBJ_OTHERS):$(OBJDIR)%.o:$(SRCDIR)%.S
	@echo compile $(notdir $<)
	@$(CC) -c $(HPATH) $(RELATIVECORE) $(COREFLAGS) $< -o $@
	
clean:
	@$(RMV) $(BLDIR_DOS)\*.o
	@$(RMV) $(BLDIR_DOS)\*.d
	@$(RMV) $(BINDIR_DOS)\BootLoader*
