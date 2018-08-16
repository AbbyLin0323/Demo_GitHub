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
.PHONY: all

include $(ROOT)\config.mk

ifneq ($(FLASH_IN),)
FLASH=$(FLASH_IN)
endif
ifneq ($(HOST_IN),)
HOST=$(HOST_IN)
endif
ifneq ($(MCU_IN),)
MCU=$(MCU_IN)
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

OBJDUMP=$(SWTOOLS)\bin\xt-objdump
CC=$(SWTOOLS)\bin\xt-xcc
CCPLUS=$(SWTOOLS)\bin\xt-xc++
COMPILEFLAGS = -g -O2 -ipa -mlongcalls -minsbits -fmessage-length=0
COMPILEFLAGSFORRESET=-Wa,--text-section-literals
COMPILEFLAGS_MIN=-Wl,-r -nostdlib $(COMPILEFLAGS)
DUMPFLAG=-S -C -D
COREFLAGS=--xtensa-system=$(XTENSA_SYSTEM) --xtensa-core=$(XTENSA_CORE) --xtensa-params=
RELATIVECORE=-DPROC_$(XTENSA_CORE) -DCONFIG_$(XTENSA_CORE)


BINDIR_DOS=$(ROOT)\Bin\bin
OBJDIR_DOS=$(ROOT)\Bin\obj\$(HOST)_$(MCU)_$(FLASH)
SRCDIR_DOS=$(ROOT)
BINDIR=$(subst \,/,$(BINDIR_DOS))
OBJDIR=$(subst \,/,$(OBJDIR_DOS))
SRCDIR=$(subst \,/,$(SRCDIR_DOS))

TOOLDIR=$(ROOT)\tool
TRACElOGTOOLDIR=$(TOOLDIR)\TraceLogParser
TRACElOGFORMATDIR=$(ROOT)\Bin\TraceLogFormat
TRACElOGFORMATDIR_TEMPLE=$(TRACElOGFORMATDIR)\temple

MCU0Elf=$(BINDIR)/$(HOST)_MCU0_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf
MCU1Elf=$(BINDIR)/$(HOST)_MCU1_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf
MCU2Elf=$(BINDIR)/$(HOST)_MCU2_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf
TargetBin_MCU0=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU0.bin
TargetBin_MCU1=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU1.bin
TargetBin_MCU2=$(BINDIR)/$(HOST)_$(FLASH)_$(MCU_CORE)_$(PLATFORM)_MCU2.bin

ifeq ($(MCU),MCU0)
    TargetElf=$(MCU0Elf)
    TargetAll=$(TargetBin_MCU0)
    $(warning Start Compile MCU0 ELF&BIN.)
else
    ifeq ($(HOST),MIXVECTOR)
        MCU0Elf=$(BINDIR)/AHCI_MCU0_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf
    endif
    ifeq ($(MCU),MCU1)
        TargetElf=$(MCU1Elf)
        TargetAll=$(TargetBin_MCU1)
        $(warning Start Compile MCU1 ELF&BIN.)
    else ifeq ($(MCU),MCU2)
        TargetElf=$(MCU2Elf)
        TargetAll=$(TargetBin_MCU2)
        $(warning Start Compile MCU2 ELF&BIN.)
    endif
endif


ifeq ($(HOST),NVME)
OptionROM=$(ROOT)/ENV/OptionRom/OptionROM_VT3514_NVME.rom
else
OptionROM=$(ROOT)/ENV/OptionRom/OptionROM_VT3514_AHCI.rom
endif


all:$(TargetAll)
	@echo done! [Firmware $(MCU_CORE) $(PLATFORM) $(notdir $<)]

include $(ROOT)\version.mk

$(TargetBin_MCU0):$(MCU0Elf)
	@echo generate $(notdir $@)
	@$(DUMP_BIN_TOOL) FW MCU0 $(MCU0Elf) $(VER_INFO) $@

$(TargetBin_MCU1):$(MCU1Elf)
	@echo generate $(notdir $@)
	@$(DUMP_BIN_TOOL) FW MCU1 $(MCU1Elf) $(VER_INFO) $@

$(TargetBin_MCU2):$(MCU2Elf)
	@echo generate $(notdir $@)
	@$(DUMP_BIN_TOOL) FW MCU2 $(MCU2Elf) $(VER_INFO) $@	
	
dep:createdir addrootsource
	$(call recursion_dep)

ifneq ($(CTYPE),unincludedep)
ifeq ($(HOST),MIXVECTOR)
-include $(LISTDIR)/SOURCE_MIXVECTOR.list $(LISTDIR)/SOURCE_FLASH_$(FLASH).list $(LISTDIR)/ASSEMBLY.list
-include $(LISTDIR)/HPATH_MIXVECTOR.list $(LISTDIR)/HPATH_FLASH_$(FLASH).list
else
-include $(LISTDIR)/SOURCE_$(MCU)_COMMON.list $(LISTDIR)/SOURCE_$(MCU)_ADP_$(HOST).list $(LISTDIR)/SOURCE_FLASH_$(FLASH).list $(LISTDIR)/ASSEMBLY.list
-include $(LISTDIR)/HPATH_$(MCU)_COMMON.list $(LISTDIR)/HPATH_$(MCU)_ADP_$(HOST).list $(LISTDIR)/HPATH_FLASH_$(FLASH).list
endif
endif
	
ifeq ($(HOST),MIXVECTOR)
SOURCE = $(subst \,/,$(SOURCE_MIXVECTOR) $(SOURCE_FLASH_$(FLASH)))
HPATH = $(subst \,/,$(HPATH_MIXVECTOR) $(HPATH_FLASH_$(FLASH)))
COMPILE_MACRO += MIX_VECTOR
else
ifeq ($(MCU),MCU0)
SOURCE = $(subst \,/,$(SOURCE_$(MCU)_COMMON) $(SOURCE_$(MCU)_ADP_$(HOST)))
else
SOURCE = $(subst \,/,$(SOURCE_$(MCU)_COMMON) $(SOURCE_$(MCU)_ADP_$(HOST)) $(SOURCE_FLASH_$(FLASH)))
endif
HPATH = $(subst \,/,$(HPATH_$(MCU)_COMMON) $(HPATH_$(MCU)_ADP_$(HOST)) $(HPATH_FLASH_$(FLASH)))
COMPILE_MACRO += HOST_$(HOST)
endif

HPATH_TRACE = $(subst -I ,,$(HPATH))

OBJ_RESET_VECTOR = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_RESET_VECTOR:%.S=%.o)))
OBJ_OTHERS       = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_OTHERS:%.S=%.o)))
OBJ_HEAD         = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_HEAD:%.S=%.o)))
OBJ_PIGGY         = $(subst $(SRCDIR),$(OBJDIR),$(subst \,/,$(SOURCE_PIGGY:%.S=%.o)))

OBJ = $(subst $(SRCDIR),$(OBJDIR),$(SOURCE:%.c=%.o)) $(OBJ_RESET_VECTOR) $(OBJ_OTHERS)

ifeq ($(MCU),MCU0)
OBJ += $(OBJ_HEAD)
endif

ifeq ($(MCU),MCU2)
OBJ += $(OBJ_PIGGY)
endif

DEP = $(OBJ:%.o=%.d)

CMACRO=$(addprefix -D,$(MCU) $(PLATFORM) $(COMPILE_MACRO))
CFLAGS=-c $(COMPILEFLAGS) $(HPATH) $(RELATIVECORE) $(COREFLAGS) $(CMACRO)

LSP=$(ROOT)/ENV/LSP/VT3533_$(MCU_CORE)_FW_$(MCU)_LSP

$(TargetElf):$(OBJ)
	@if NOT EXIST $(subst /,\,$(dir $@)) (mkdir $(subst /,\,$(dir $@)))
	@echo generate $(notdir $@) using $(LSP)
	@$(CCPLUS) $(OBJ) -o $@ -mlsp=$(LSP) $(COMPILEFLAGS) $(COREFLAGS)
ifeq ($(OBJDUMP_SWITCH),ON)
	@echo dump $(subst .elf,.txt,$(notdir $@))
	@$(OBJDUMP) $(DUMPFLAG) $@ > $(subst .elf,.txt,$@)
endif
ifeq ($(TL_FORMAT_SWITCH),ON)
	@echo TraceLog Format file generating
	@$(TRACElOGTOOLDIR)\trace_log_parser.py $(TRACElOGFORMATDIR_TEMPLE) $(MCU) $(TRACElOGFORMATDIR)\$(HOST)\$(FLASH) "$(HPATH_TRACE)" "$(CMACRO)" "$(SOURCE)"
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
	@$(CC) -c $(HPATH) $(RELATIVECORE) $(COREFLAGS) -D$(MCU) $< -o $@
	
$(OBJ_HEAD):$(OBJDIR)%.o:$(SRCDIR)%.S
	@echo compile $(notdir $<)
	@$(CC) -c $(HPATH) $(RELATIVECORE) $(COREFLAGS) $< -o $@

$(OBJ_PIGGY):$(OBJDIR)%.o:$(SRCDIR)%.S
	@if NOT EXIST $(subst /,\,$(dir $@)) (mkdir $(subst /,\,$(dir $@)))
	@echo compile $(notdir $<)
	@$(CC) -c $(HPATH) $(RELATIVECORE) $(COREFLAGS) $< -o $@

check_mixvector:
	@if NOT EXIST $(ROOT)\Bin\bin\AHCI_MCU0_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf (\
	$(MAKE) error_mixvector\
	)
	
error_mixvector:
	$(error AHCI_MCU0_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf does not exist,please get AHCI_MCU0_$(FLASH)_$(MCU_CORE)_$(PLATFORM).elf ready first!)

clean:
	@$(RMV) $(OBJDIR_DOS)\*.o 
	@$(RMV) $(OBJDIR_DOS)\*.d
	@$(RMV) $(BINDIR_DOS)\$(HOST)_*
	@$(RMV) $(ROOT)\Bin\gitver

clean_dep:
	@if EXIST $(LISTDIR) ($(RMV) $(LISTDIR)\*.list)

clean_all:clean_dep
	@$(RMV) $(dir $(OBJDIR_DOS))\*.d $(dir $(OBJDIR_DOS))\*.o
	@$(RMV) $(BINDIR_DOS)\*.elf $(BINDIR_DOS)\*.bin $(BINDIR_DOS)\*.txt

clean_obj:
	@$(RMV) $(dir $(OBJDIR_DOS))\*.d $(dir $(OBJDIR_DOS))\*.o

createdir:
	@if NOT EXIST $(LISTDIR) (mkdir $(LISTDIR))
	@if NOT EXIST $(dir $(OBJDIR_DOS)) (mkdir $(dir $(OBJDIR_DOS)))
	@if NOT EXIST $(BINDIR_DOS) (mkdir $(BINDIR_DOS))
	@if NOT EXIST $(TRACElOGFORMATDIR) (mkdir $(TRACElOGFORMATDIR))
