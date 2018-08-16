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
#Description : Root Makefile in root dir of the whole projects, user input window.
#Usage       : design use this Makefile to generate your target files
#Modify      :
#20150311    Haven create file
#********************************************************************************

.PHONY: default

export ROOT
ROOT = ${shell cd}
MAKE=xt-make

################################################
# global common region
################################################
default:gitver
	$(MAKE) -C firmware all MCU_IN=MCU0
	$(MAKE) -C firmware all MCU_IN=MCU1
	$(MAKE) -C firmware all MCU_IN=MCU2

clean:
	$(MAKE) -C firmware clean MCU_IN=MCU0 CTYPE=unincludedep
	$(MAKE) -C firmware clean MCU_IN=MCU1 CTYPE=unincludedep
    $(MAKE) -C firmware clean MCU_IN=MCU2 CTYPE=unincludedep

all:nvme_all ahci_all sata_all bl_all
	@echo all target compile finished!
	
clean_all:
	$(MAKE) -C firmware clean_all CTYPE=unincludedep
	@echo all target and temp files clean finished!

clean_obj:
	$(MAKE) -C firmware clean_obj CTYPE=unincludedep
	@echo all object(*.o) files clean finished!
	
dep:gitver
	$(MAKE) -C firmware clean_dep CTYPE=unincludedep
	$(MAKE) -C firmware dep CTYPE=unincludedep
	$(MAKE) -C unittest dep
	@echo scan all firmware source code files and classify OK! 
	
################################################
# nvme compile region
################################################
nvme_mcu0:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0
	
nvme_mcu1:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1
	
nvme_mcu2:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2

nvme:nvme_mcu0 nvme_mcu1 nvme_mcu2
	@echo nvme mcu0/mcu1/mcu2 default flash type compile finished!
	
nvme_tsb_2d_tlc:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_2D_TLC
	
nvme_l95:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=L95

nvme_tsb_3d_tlc:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_3D_TLC

nvme_intel_3d_tlc:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=INTEL_3D_TLC

nvme_intel_3d_mlc:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=INTEL_3D_MLC
	
nvme_tsb_fourpln:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_FOURPLN

nvme_ymtc_3d_mlc:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=YMTC_3D_MLC
	
nvme_all:
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
		
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC
	
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=NVME MCU_IN=MCU2 FLASH_IN=YMTC_3D_MLC
		
################################################
# nvme clean region
################################################
clean_nvme_mcu0:
	$(MAKE) -C firmware clean HOST_IN=NVME MCU_IN=MCU0 CTYPE=unincludedep
	
clean_nvme_mcu1:
	$(MAKE) -C firmware clean HOST_IN=NVME MCU_IN=MCU1 CTYPE=unincludedep

clean_nvme_mcu2:
	$(MAKE) -C firmware clean HOST_IN=NVME MCU_IN=MCU2 CTYPE=unincludedep

nvme_clean:clean_nvme
clean_nvme:clean_nvme_mcu0 clean_nvme_mcu1 clean_nvme_mcu2
	@echo clean NVME MCU0/MCU1/MCU2 finished, including all flash type.
	
################################################
# ahci compile region
################################################	
ahci:ahci_mcu0 ahci_mcu12
	@echo ahci MCU0/MCU1 default flash type compile finished!

ahci_mcu0:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0
	
ahci_mcu12:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1	
	
ahci_tsb_2d_tlc:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	
ahci_l95:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=L95

ahci_tsb_3d_tlc:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC

ahci_tsb_fourpln:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN

ahci_ymtc_3d_mlc:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC

ahci_all:
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=AHCI MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC
	
################################################
# ahci clean region
################################################
clean_ahci_mcu0:
	$(MAKE) -C firmware clean HOST_IN=AHCI MCU_IN=MCU0 CTYPE=unincludedep
	
clean_ahci_mcu12:
	$(MAKE) -C firmware clean HOST_IN=AHCI MCU_IN=MCU1 CTYPE=unincludedep

clean_ahci:ahci_clean
clean_ahci:clean_ahci_mcu0 clean_ahci_mcu12
	@echo clean AHCI mcu0/MCU1 finished, including all flash type.
	
################################################
# sata compile region
################################################	
sata:sata_mcu0 sata_mcu1 sata_mcu2
	@echo sata mcu0/mcu1/mcu2 default flash type compile finished!
	
sata_mcu0:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0

sata_mcu1:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1
	
sata_mcu2:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2

sata_tsb_2d_tlc:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_2D_TLC
	
sata_l95:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=L95

sata_intel_3d_tlc:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=INTEL_3D_TLC	

sata_intel_3d_mlc:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=INTEL_3D_MLC	
	
sata_tsb_fourpln:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_FOURPLN
	
sata_ymtc_3d_mlc:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=YMTC_3D_MLC

sata_tsb_3d_tlc:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_3D_TLC

sata_all:
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=INTEL_3D_MLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU0 FLASH_IN=YMTC_3D_MLC
	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC
	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_2D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=L95
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_3D_TLC
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=INTEL_3D_MLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=INTEL_3D_TLC	
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=TSB_FOURPLN
	$(MAKE) -C firmware all HOST_IN=SATA MCU_IN=MCU2 FLASH_IN=YMTC_3D_MLC

################################################
# sata clean region
################################################
clean_sata_mcu0:
	$(MAKE) -C firmware clean HOST_IN=SATA MCU_IN=MCU0 CTYPE=unincludedep
	
clean_sata_mcu1:
	$(MAKE) -C firmware clean HOST_IN=SATA MCU_IN=MCU1 CTYPE=unincludedep

clean_sata_mcu2:
	$(MAKE) -C firmware clean HOST_IN=SATA MCU_IN=MCU2 CTYPE=unincludedep
	
sata_clean:clean_sata
clean_sata:clean_sata_mcu0 clean_sata_mcu1 clean_sata_mcu2
	@echo clean SATA mcu0/mcu1/mcu2 finished, including all flash type.
	
################################################
# mixvector compile region
################################################	
mixvector:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1
	@echo mixvector mcu12 default flash type compile finished!
	
mixvector_tsb_2d_tlc:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_2D_TLC
	
mixvector_l95:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=L95 CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=L95

mixvector_tsb_3d_tlc:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_3D_TLC

mixvector_intel_3d_tlc:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=INTEL_3D_TLC	

mixvector_intel_3d_mlc:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=INTEL_3D_MLC	
	
mixvector_tsb_fourpln:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=TSB_FOURPLN
	
mixvector_ymtc_3d_tlc:
	$(MAKE) -C firmware check_mixvector HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC CTYPE=unincludedep
	$(MAKE) -C firmware all HOST_IN=MIXVECTOR MCU_IN=MCU1 FLASH_IN=YMTC_3D_MLC

mixvector_all:mixvector_tsb_2d_tlc mixvector_l95 mixvector_tsb mixvector_tsb_fourpln mixvector_ymtc_3d_tlc
	@echo compile all the mixvector versions ok.

################################################
# mixvector clean region
################################################
mixvector_clean:clean_mixvector
clean_mixvector:
	$(MAKE) -C firmware clean HOST_IN=MIXVECTOR MCU_IN=MCU1 CTYPE=unincludedep
	@echo clean MIXVECTOR finished, including all flash type.
		
################################################
# bootloader compile and clean region
################################################
bl:gitver
	$(MAKE) -C bootloader all

bl_all:
	$(MAKE) -C bootloader all FLASH_IN=TSB_2D_TLC
	$(MAKE) -C bootloader all FLASH_IN=L95
	$(MAKE) -C bootloader all FLASH_IN=TSB_3D_TLC
	$(MAKE) -C bootloader all FLASH_IN=INTEL_3D_TLC
	$(MAKE) -C bootloader all FLASH_IN=TSB_FOURPLN
	$(MAKE) -C bootloader all FLASH_IN=YMTC_3D_MLC
		
bl_clean:clean_bl
clean_bl:
	$(MAKE) -C bootloader clean CTYPE=unincludedep

################################################
# system image compile and clean region
################################################
system:
	$(MAKE) -C blscript all
	
system_clean:clean_system
clean_system:
	$(MAKE) -C blscript clean

################################################
# bootrom compile and clean region
################################################
br_dep:
	$(MAKE) -C rom\VT3514_C0_ROM\firmware dep CTYPE=unincludedep

br_unpack_bypass_dep:
	$(MAKE) -C rom\VT3514_C0_ROM\firmware dep CTYPE=unincludedep UNPACK=BYPASS	

br:gitver
	$(MAKE) -C rom\VT3514_C0_ROM\firmware all

br_isram:gitver
	$(MAKE) -C rom\VT3514_C0_ROM\firmware all LOCATION=_ISRAM
	
br_unpack_runon_isram:gitver
	$(MAKE) -C rom\VT3514_C0_ROM\firmware all LOCATION=_UNPACK_RUNON_ISRAM

br_unpack_runon_isram_unpack_bypass:gitver
	$(MAKE) -C rom\VT3514_C0_ROM\firmware all LOCATION=_UNPACK_RUNON_ISRAM UNPACK=BYPASS	
	
br_clean:clean_br
clean_br:
	$(MAKE) -C rom\VT3514_C0_ROM\firmware clean CTYPE=unincludedep

gitver:
	@if NOT EXIST $(ROOT)\Bin (mkdir $(ROOT)\Bin)
	@echo VER_GITVERSION = \> $(ROOT)\Bin\gitver
	@-git rev-parse HEAD >> $(ROOT)\Bin\gitver
	
-include $(ROOT)\build.lib
-include $(ROOT)\board.mk
-include $(ROOT)\Release_bj.mk
-include $(ROOT)\Release_qa.mk
-include $(ROOT)\Release_tp.mk

################################################
# xt-make help information
################################################
help:
	@echo ----------------------------------------------------------------------------
	@echo xt-make dep                               scan source code and classify them, if add/remove
	@echo                                           a source file, must run this command first
	@echo xt-make                                   compile mcu0 and mcu12, using configuration from
	@echo                                           config.mk, target files save to Bin directory
	@echo xt-make all                               compile all host/mcu/flash combination
	@echo xt-make clean                             clean up the files generated by command xt-make
	@echo xt-make clean_all                         clean up the files generated by command xt-make all
	@echo ----------------------------------------------------------------------------
	@echo xt-make nvme                              compile mcu0/12 for nvme, flash type from config.mk
	@echo xt-make nvme_tsb_2d_tlc                   compile mcu0/12 for nvme, flash type is TSB_2D_TLC
	@echo xt-make nvme_l95                          compile mcu0/12 for nvme, flash type is L95
	@echo xt-make nvme_tsb_3d_tlc                   compile mcu0/12 for nvme, flash type is TSB_3D_TLC
	@echo xt-make nvme_intel_3d_tlc                 compile mcu0/12 for nvme, flash type is INTEL_3D_TLC
	@echo xt-make nvme_intel_3d_mlc                 compile mcu0/12 for nvme, flash type is INTEL_3D_MLC
	
	@echo xt-make nvme_tsb_fourpln                  compile mcu0/12 for nvme, flash type is TSB_FOURPLN
	@echo xt-make nvme_all                          compile mcu0/12 for nvme, flash type is TSB_2D_TLC/L95/TSB_3D_TLC/INTEL_3D_TLC/INTEL_3D_MLC/TSB_FOURPLN
	@echo xt-make nvme_clean                        clean up the files generated by xt-make nvme_all
	@echo ----------------------------------------------------------------------------
	@echo xt-make ahci                              compile mcu0/12 for ahci, flash type from config.mk
	@echo xt-make ahci_tsb_2d_tlc                   compile mcu0/12 for ahci, flash type is TSB_2D_TLC
	@echo xt-make ahci_l95                          compile mcu0/12 for ahci, flash type is L95
	@echo xt-make ahci_tsb_3d_tlc                   compile mcu0/12 for ahci, flash type is TSB_3D_TLC
	@echo xt-make ahci_tsb_fourpln                  compile mcu0/12 for ahci, flash type is TSB_FOURPLN
	@echo xt-make ahci_all                          compile mcu0/12 for ahci, flash type is TSB_2D_TLC/L95/TSB_3D_TLC/TSB_FOURPLN
	@echo xt-make ahci_clean                        clean up the files generated by xt-make ahci_all
	@echo ---------------------------------------------------------------------------- 
	@echo xt-make sata                              compile mcu0/12 for sata, flash type from config.mk
	@echo xt-make sata_tsb_2d_tlc                   compile mcu0/12 for sata, flash type is TSB_2D_TLC
	@echo xt-make sata_l95                          compile mcu0/12 for sata, flash type is L95
	@echo xt-make sata_tsb_3d_tlc                   compile mcu0/12 for sata, flash type is TSB_3D_TLC
	@echo xt-make sata_intel_3d_tlc                 compile mcu0/12 for sata, flash type is INTEL_3D_TLC
	@echo xt-make sata_intel_3d_mlc                 compile mcu0/12 for sata, flash type is INTEL_3D_MLC
	@echo xt-make sata_tsb_fourpln                  compile mcu0/12 for sata, flash type is TSB_FOURPLN
	@echo xt-make sata_all                          compile mcu0/12 for sata, flash type is TSB_2D_TLC/L95/TSB_3D_TLC/TSB_FOURPLN
	@echo xt-make sata_clean                        clean up the files generated by xt-make sata_all
	@echo ---------------------------------------------------------------------------- 
	@echo xt-make mixvector                         compile mcu12 for mixvector,flash type from config.mk
	@echo xt-make mixvector_tsb_2d_tlc              compile mcu12 for mixvector,flash type is TSB_2D_TLC
	@echo xt-make mixvector_l95                     compile mcu12 for mixvector,flash type is L95
	@echo xt-make mixvector_tsb_3d_tlc              compile mcu12 for mixvector,flash type is TSB_3D_TLC
	@echo xt-make mixvector_intel_3d_tlc            compile mcu12 for mixvector,flash type is INTEL_3D_TLC
	@echo xt-make mixvector_intel_3d_mlc            compile mcu12 for mixvector,flash type is INTEL_3D_MLC
	@echo xt-make mixvector_tsb_fourpln             compile mcu12 for mixvector,flash type is TSB_FOURPLN
	@echo xt-make mixvector_all                     compile mcu12 for mixvector,flash type is TSB_2D_TLC/L95/TSB_3D_TLC/INTEL_3D_TLC/INTEL_3D_MLC/TSB_FOURPLN
	@echo xt-make mixvector_clean                   clean up the files generated by xt-make mixvector_all
	@echo ----------------------------------------------------------------------------
	@echo xt-make bl                                compile bootloader, flash type from config.mk
	@echo xt-make bl_all                            compile bootloader, flash type is TSB_2D_TLC/L95/TSB_3D_TLC/INTEL_3D_TLC/INTEL_3D_MLC/TSB_FOURPLN
	@echo xt-make bl_clean                          clean up the files generated by xt-make bl/bl_all
	@echo ----------------------------------------------------------------------------
	@echo xt-make br_dep                            scan boot rom source code before the first xt-make br/br_isram/br_unpack_runon_isram
	@echo xt-make br_unpack_bypass_dep              scan boot rom source code before the first xt-make br_unpack_runon_isram_unpack_bypass
	@echo xt-make br                                compile boot rom,(B0/C0) and PLATFORM from config.mk 
	@echo xt-make br_isram                          compile boot rom for isram, other info from config.mk 
	@echo xt-make br_unpack_runon_isram             compile boot rom unpack from rom to runon isram, other info from config.mk
	@echo xt-make br_unpack_runon_isram_unpack_bypass  compile boot rom to runon isram, other info from config.mk
	@echo xt-make br_clean                          clean up the files generated by xt-make br_*
	@echo ----------------------------------------------------------------------------
	@echo xt-make system                            make system image
	@echo xt-make system_clean                      clean up the system image
	@echo ----------------------------------------------------------------------------
	@echo xt-make tp_release                        make Taipei release system images
	@echo                                           create images located at $(REL_DIR)
	@echo ----------------------------------------------------------------------------
	@echo xt-make bj_release                        make Beijing release system images
	@echo                                           create images located at $(REL_DIR)	
	@echo ----------------------------------------------------------------------------
	@echo xt-make qa_release                        make FW_QA release system images
	@echo                                           create images located at $(REL_DIR)	
	@echo ----------------------------------------------------------------------------
	@echo xt-make build_clean                       equal to "make clean_all; make dep"
	@echo ----------------------------------------------------------------------------
	@echo xt-make {board}                           {board} refer the board.mk for the avaiable board names.
	@echo                                           create images located at $(DEV_DIR)
	@echo ----------------------------------------------------------------------------
