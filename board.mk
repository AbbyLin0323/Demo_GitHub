#/*******************************************************************************
#* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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

#
# As create new BOARD rule.
# just copy this section and modify
# HOST  =  SATA | NVME (refer config.mk HOST)
# FLASH = (refer config.mk FLASH)
# MACRO = (refer config.mk COMPILE_MACRO)
# BOARD = (refer config.mk BOARD)
#

# VT6734 (TP NVMe CRB)
VT6734-1L4P_NVME_128GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=3")
	$(eval BOARD=\
	        VT6734-INTEL_128GB1L4P \
	        VT6734-MICRON_128GB1L4P \
	        VT6760A-INTEL_128GB1L4P \
	        VT6760A-MICRON_128GB1L4P \
	        VT6760C-INTEL_128GB1L4P \
	        VT6760C-MICRON_128GB1L4P \
	        VT6734D1-MICRON_128GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6734-1L4P_NVME_256GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=6")
	$(eval BOARD=\
	        VT6734-INTEL_256GB1L4P \
	        VT6734-MICRON_256GB1L4P \
	        VT6760A-INTEL_256GB1L4P \
	        VT6760A-MICRON_256GB1L4P \
	        VT6760C-INTEL_256GB1L4P \
	        VT6760C-MICRON_256GB1L4P \
	        VT6734D1-MICRON_256GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6734-1L4P_NVME_512GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=12 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6734-INTEL_512GB1L4P \
	        VT6734-MICRON_512GB1L4P \
	        VT6760A-INTEL_512GB1L4P \
	        VT6760A-MICRON_512GB1L4P \
	        VT6760C-INTEL_512GB1L4P \
	        VT6760C-MICRON_512GB1L4P \
	        VT6734D1-MICRON_512GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6734-1L4P_NVME_682GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6734D1-MICRON_682GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6734-B16A_1L2P_NVME_120GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=4")
	$(eval BOARD=\
	        VT6734-B16A_INTEL_120GB1L2P \
	        VT6734-B16A_MICRON_120GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6734-B16A_1L2P_NVME_240GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=8")
	$(eval BOARD=\
	        VT6734-B16A_INTEL_240GB1L2P \
	        VT6734-B16A_MICRON_240GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6734-B16A_1L2P_NVME_480GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6734-B16A_INTEL_480GB1L2P \
	        VT6734-B16A_MICRON_480GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6734-B16A_1L2P_NVME_960GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_1TB IM_3D_TLC_512GB CACHE_LINK_MULTI_LCT")
	$(eval BOARD=\
	        VT6734-B16A_MICRON_960GB1L2P \
	        VT6760A-B16A_MICRON_960GB1L2P \
	        VT6760C-B16A_MICRON_960GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))	

VT6734-B17A_1L4P_NVME_120GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=2")
	$(eval BOARD=\
	        VT6734-B17A_INTEL_120GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6734-B17A_1L4P_NVME_240GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=4")
	$(eval BOARD=\
	        VT6734-B17A_INTEL_240GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6734-B17A_1L4P_NVME_480GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=8 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6734-B17A_INTEL_480GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))	

VT6734-B17A_1L4P_NVME_960GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=8 IM_3D_TLC_1TB IM_3D_TLC_512GB CACHE_LINK_MULTI_LCT")
	$(eval BOARD=\
	        VT6734-B17A_INTEL_960GB1L4P \
	        VT6760A-B17A_INTEL_960GB1L4P \
	        VT6760C-B17A_INTEL_960GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

# VT6739 (TP SATA EVB)
VT6739-1L4P_SATA_128GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=3")
	$(eval BOARD=\
	        VT6739-INTEL_128GB1L4P \
	        VT6739-MICRON_128GB1L4P \
	        VT6742-INTEL_128GB1L4P \
	        VT6742-MICRON_128GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6739-1L4P_SATA_256GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=6")
	$(eval BOARD=\
	        VT6739-INTEL_256GB1L4P \
	        VT6739-MICRON_256GB1L4P \
	        VT6742-INTEL_256GB1L4P \
	        VT6742-MICRON_256GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6739-1L4P_SATA_341GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=8")
	$(eval BOARD=\
	        VT6742-MICRON_341GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6739-1L4P_SATA_512GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=12 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6739-INTEL_512GB1L4P \
	        VT6739-MICRON_512GB1L4P \
	        VT6742-INTEL_512GB1L4P \
	        VT6742-MICRON_512GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-1L4P_SATA_682GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6742-MICRON_682GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))


VT6739-1L4P_SATA_1024GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=12 IM_3D_TLC_1TB IM_3D_TLC_512GB CACHE_LINK_MULTI_LCT")
	$(eval BOARD=\
	        VT6739-INTEL_1024GB1L4P \
	        VT6739-MICRON_1024GB1L4P \
	        VT6742-INTEL_1024GB1L4P \
	        VT6742-MICRON_1024GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))	

VT6739-B16A_1L2P_SATA_60GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=2")
	$(eval BOARD=\
	        VT6739-B16A_INTEL_60GB1L2P \
	        VT6739-B16A_MICRON_60GB1L2P \
	        VT6742-B16A_INTEL_60GB1L2P \
	        VT6742-B16A_MICRON_60GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B16A_1L2P_SATA_120GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=4")
	$(eval BOARD=\
	        VT6739-B16A_INTEL_120GB1L2P \
	        VT6739-B16A_MICRON_120GB1L2P \
	        VT6742-B16A_INTEL_120GB1L2P \
	        VT6742-B16A_MICRON_120GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6739-B16A_1L2P_SATA_240GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=8")
	$(eval BOARD=\
	        VT6739-B16A_INTEL_240GB1L2P \
	        VT6739-B16A_MICRON_240GB1L2P \
	        VT6742-B16A_INTEL_240GB1L2P \
	        VT6742-B16A_MICRON_240GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B16A_1L2P_SATA_480GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6739-B16A_INTEL_480GB1L2P \
	        VT6739-B16A_MICRON_480GB1L2P \
	        VT6742-B16A_INTEL_480GB1L2P \
	        VT6742-B16A_MICRON_480GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B16A_1L2P_SATA_960GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B16A LUN_NUM_PER_SUPERPU=16 IM_3D_TLC_1TB IM_3D_TLC_512GB CACHE_LINK_MULTI_LCT")
	$(eval BOARD=\
	        VT6739-B16A_INTEL_960GB1L2P \
            VT6739-B16A_MICRON_960GB1L2P \
	        VT6742-B16A_INTEL_960GB1L2P \
            VT6742-B16A_MICRON_960GB1L2P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B17A_1L4P_SATA_120GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=2")
	$(eval BOARD=\
	        VT6739-B17A_INTEL_120GB1L4P \
	        VT6742-B17A_INTEL_120GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT6739-B17A_1L4P_SATA_240GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=4")
	$(eval BOARD=\
	        VT6739-B17A_INTEL_240GB1L4P \
	        VT6742-B17A_INTEL_240GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B17A_1L4P_SATA_480GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=8 IM_3D_TLC_512GB")
	$(eval BOARD=\
	        VT6739-B17A_INTEL_480GB1L4P \
	        VT6742-B17A_INTEL_480GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

VT6739-B17A_1L4P_SATA_960GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="IM_3D_TLC_B17A LUN_NUM_PER_SUPERPU=8 IM_3D_TLC_1TB IM_3D_TLC_512GB CACHE_LINK_MULTI_LCT")
	$(eval BOARD=\
	        VT6739-B17A_INTEL_960GB1L4P \
	        VT6742-B17A_INTEL_960GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))

# VT3084 (BJ NVMe EVB)
VT3084-1L4P_NVME_128GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=3")
	$(eval BOARD=\
	        VT3084-INTEL_128GB1L4P \
	        VT3084-MICRON_128GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
VT3084-1L4P_NVME_256GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=6")
	$(eval BOARD=\
	        VT3084-INTEL_256GB1L4P \
	        VT3084-MICRON_256GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
	
VT3084-1L4P_NVME_512GB_IM_3D_TLC:
	$(eval HOST=NVME)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=12")
	$(eval BOARD=\
	        VT3084-INTEL_512GB1L4P \
	        VT3084-MICRON_512GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
# VT3084 (BJ SATA EVB)
VT3084-1L4P_SATA_128GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=3")
	$(eval BOARD=\
	        VT3084-INTEL_128GB1L4P \
	        VT3084-MICRON_128GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
VT3084-1L4P_SATA_256GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=6")
	$(eval BOARD=\
	        VT3084-INTEL_256GB1L4P \
	        VT3084-MICRON_256GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
VT3084-1L4P_SATA_512GB_IM_3D_TLC:
	$(eval HOST=SATA)
	$(eval FLASH=INTEL_3D_TLC)
	$(eval MACRO="LUN_NUM_PER_SUPERPU=12")
	$(eval BOARD=\
	        VT3084-INTEL_512GB1L4P \
	        VT3084-MICRON_512GB1L4P \
	        )
	$(if $(RELEASE), $(call build_clean),)
	$(if $(RELEASE), $(call build_dep),)
	$(call build_board,$(HOST),$(FLASH),$(MACRO),$(BOARD))
