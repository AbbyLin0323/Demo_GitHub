
#mcu version
ifeq ($(MCU_CORE),C0)
VER_MCU = 02
else
VER_MCU = 01
endif

#host type
ifeq ($(HOST),SATA)
VER_HOST = 01
endif
ifeq ($(HOST),AHCI)
VER_HOST = 02
endif
ifeq ($(HOST),NVME)
VER_HOST = 03
endif


#flash type
ifeq ($(FLASH),TSB_2D_TLC)
VER_FLASH = 01
endif
ifeq ($(FLASH),L95)
VER_FLASH = 02
endif
ifeq ($(FLASH),TSB_3D_TLC)
VER_FLASH = 03
endif
ifeq ($(FLASH),TSB_FOURPLN)
VER_FLASH = 04
endif
ifeq ($(FLASH),INTEL_3D_TLC)
VER_FLASH = 05
endif

#other configuration, vpu?
TEMP_VER =$(subst SSD_ARCH_VPU,,$(COMPILE_MACRO))
ifeq ($(TEMP_VER), $(COMPILE_MACRO))
VER_OTHER = 00
else
VER_OTHER = 01
endif

VER_BLPARAM = 00000000
VER_RELEASEVER = $(subst .,E,$(RELEASE_VERSION))

-include $(ROOT)\Bin\gitver

ifeq ($(VER_GITVERSION),)
VER_GITVERSION = ffffffff
endif

VER_DW0 = $(VER_OTHER)$(VER_FLASH)$(VER_HOST)$(VER_MCU)

VER_INFO = $(VER_DW0)$(VER_BLPARAM)$(VER_RELEASEVER)$(VER_GITVERSION)

DUMP_BIN_TOOL=$(ROOT)/tool/fw_bin_dump.exe
ROM_BIN2HEX_TOOL=$(ROOT)/tool/bin2rom.exe
BLSCRIPT_TOOL=$(ROOT)/blscript/bl_parameter_compile.exe
IMAGE_INTEGRATE_TOOL=$(ROOT)/tool/Image_Integrate_tool/Image_Integrate.exe
