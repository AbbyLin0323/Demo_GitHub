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

tp_release : build_release tp_board build_end

tp_board: \
	VT6734-1L4P_NVME_128GB_IM_3D_TLC \
	VT6734-1L4P_NVME_256GB_IM_3D_TLC \
	VT6734-1L4P_NVME_512GB_IM_3D_TLC \
	VT6739-1L4P_SATA_128GB_IM_3D_TLC \
	VT6739-1L4P_SATA_256GB_IM_3D_TLC \
	VT6739-1L4P_SATA_512GB_IM_3D_TLC \
	VT6739-1L4P_SATA_1024GB_IM_3D_TLC \
	VT6734-B16A_1L2P_NVME_120GB_IM_3D_TLC \
	VT6734-B16A_1L2P_NVME_240GB_IM_3D_TLC \
	VT6734-B16A_1L2P_NVME_480GB_IM_3D_TLC \
	VT6734-B16A_1L2P_NVME_960GB_IM_3D_TLC \
	VT6739-B16A_1L2P_SATA_120GB_IM_3D_TLC \
	VT6739-B16A_1L2P_SATA_240GB_IM_3D_TLC \
	VT6739-B16A_1L2P_SATA_480GB_IM_3D_TLC \
	VT6739-B16A_1L2P_SATA_960GB_IM_3D_TLC \
	VT6734-B17A_1L4P_NVME_120GB_IM_3D_TLC \
	VT6734-B17A_1L4P_NVME_240GB_IM_3D_TLC \
	VT6734-B17A_1L4P_NVME_480GB_IM_3D_TLC \
	VT6734-B17A_1L4P_NVME_960GB_IM_3D_TLC \
	VT6739-B17A_1L4P_SATA_120GB_IM_3D_TLC \
	VT6739-B17A_1L4P_SATA_240GB_IM_3D_TLC \
	VT6739-B17A_1L4P_SATA_480GB_IM_3D_TLC \
	VT6739-B17A_1L4P_SATA_960GB_IM_3D_TLC
