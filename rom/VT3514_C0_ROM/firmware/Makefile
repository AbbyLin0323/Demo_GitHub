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
#Date        : 2015.03.30
#Description : Main Makefile for compile rom code.
#Usage       : called by root Makefile at the upper directory
#Modify      :
#20150330    Haven create file
#********************************************************************************
.PHONY: all

SOURCE_ROM_TEMP=$(wildcard *.c)
include $(ROOT)\config.mk

ifeq ($(MCU_CORE),C0)
XTENSA_CORE=rfviatieloop16
COMPILE_MACRO += VT3514_C0
else
XTENSA_CORE=viatie
endif

SWTOOLS=$(XTENSA_ROOT_DIR)\XtDevTools\install\tools\RF-2014.1-win32\XtensaTools
XTENSA_SYSTEM=$(XTENSA_ROOT_DIR)\XtDevTools\install\builds\RF-2014.1-win32\$(XTENSA_CORE)\config

OBJDUMP=$(SWTOOLS)\bin\xt-objdump
CC=$(SWTOOLS)\bin\xt-xcc
CCPLUS=$(SWTOOLS)\bin\xt-xc++
COMPILEFLAGS=-g -O2 -Os -ipa -mlongcalls -minsbits -fmessage-length=0
COMPILEFLAGSFORRESET=-Wa,--text-section-literals
COMPILEFLAGS_MIN=-Wl,-r -nostdlib $(COMPILEFLAGS)
DUMPFLAG=-S -C -D
COREFLAGS=--xtensa-system=$(XTENSA_SYSTEM) --xtensa-core=$(XTENSA_CORE) --xtensa-params=
RELATIVECORE=-DPROC_$(XTENSA_CORE) -DCONFIG_$(XTENSA_CORE)

BINDIR_DOS=$(ROOT)\Bin\bin
OBJDIR_DOS=$(ROOT)\Bin\obj\rom
SRCDIR_DOS=$(ROOT)\rom\VT3514_C0_ROM\firmware
BINDIR=$(subst \,/,$(BINDIR_DOS))
OBJDIR=$(subst \,/,$(OBJDIR_DOS))
SRCDIR=$(subst \,/,$(SRCDIR_DOS))

TOOLDIR=$(ROOT)\tool

TargetElf=$(BINDIR)/ROM_$(MCU_CORE)_$(PLATFORM)$(LOCATION).elf

all:$(TargetElf)
	@echo done! [ROM $(MCU_CORE) $(PLATFORM) $(notdir $<)]

dep:clean_dep romdep
	@echo scan rom source code done!

ifneq ($(CTYPE),unincludedep)
-include $(LISTDIR)/SOURCE_ROM.list
endif
	
SOURCE = $(subst \,/,$(SOURCE_ROM))
HPATH = $(addprefix -I,$(subst \,/,$(HPATH_ROM)))

OBJ_RESET_VECTOR = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_RESET_VECTOR:%.S=%.o)))
OBJ_OTHERS       = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_OTHERS:%.S=%.o)))
OBJ_HEAD         = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_HEAD:%.S=%.o)))

OBJ = $(subst $(SRCDIR),$(OBJDIR),$(SOURCE:%.c=%.o)) $(OBJ_RESET_VECTOR) $(OBJ_OTHERS)

DEP = $(OBJ:%.o=%.d)

CMACRO=$(addprefix -D,HOST_$(HOST) $(MCU) $(PLATFORM) $(COMPILE_MACRO))
CFLAGS=-c $(COMPILEFLAGS) $(HPATH) $(RELATIVECORE) $(COREFLAGS) $(CMACRO)

LSP=$(ROOT)/rom/LSP/VT3514_$(MCU_CORE)_ROM$(LOCATION)_LSP

$(TargetElf):$(OBJ)
	@echo generate $(notdir $@)
	@$(CCPLUS) $(OBJ) -o $@ -mlsp=$(LSP) $(COMPILEFLAGS) $(COREFLAGS)
ifeq ($(OBJDUMP_SWITCH),ON)
	@echo dump $(subst .elf,.txt,$(notdir $@))
	@$(OBJDUMP) $(DUMPFLAG) $@ > $(subst .elf,.txt,$@)
endif
	
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
	@$(RMV) $(OBJDIR_DOS)\*.o 
	@$(RMV) $(OBJDIR_DOS)\*.d
	@$(RMV) $(BINDIR_DOS)\ROM*

clean_dep:
	@if EXIST $(LISTDIR) ($(RMV) $(LISTDIR)\*ROM*)

