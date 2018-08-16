/*
 * Customer ID=5586; Build=0x36b41; Copyright (c) 2003-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 *
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Tensilica Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Tensilica Inc.
 */

/*
 * This file provides Xtensa processor configuration
 * parameters that can be used in XTMP simulations.
 */

#ifndef __XTMP_CONFIG_included__
#define __XTMP_CONFIG_included__

#define DRAM_ALLOCATE_SIZE      (512 * 1024 * 1024)
#define XTMP_CONFIG_NAME		"viatie"
#define XTMP_BYTE_ORDERING		LE
#define XTMP_IS_LITTLE_ENDIAN		1
#define XTMP_IS_BIG_ENDIAN		0
#define XTMP_PIF_BYTES			4
#define XTMP_PIF_BITS			32
#define XTMP_BIT_WIDTH			XTMP_PIF_BITS	/* DEPRECATED */
#define XTMP_LOAD_STORE_BITS		32
#define XTMP_IFETCH_BITS		32
#define XTMP_HAS_PIF			1
#define XTMP_HAS_PRID			1
#define XTMP_HAS_DCACHE			0
#define XTMP_DCACHE_SIZE		0
#define XTMP_DCACHE_WAYS		0
#define XTMP_DCACHE_WRITE_BACK		0
#define XTMP_DCACHE_LINE_BYTES		0

#define XTMP_HAS_SYS_ROM		1
#define XTMP_SYS_ROM_PADDR		0xFFE00000
#define XTMP_SYS_ROM_VADDR		0xFFE00000
#define XTMP_SYS_ROM_SIZE		0x00006000

#define XTMP_HAS_SYS_RAM		1
#define XTMP_SYS_RAM_PADDR		0x40000000
#define XTMP_SYS_RAM_VADDR		0x40000000
#define XTMP_SYS_RAM_SIZE		0x40000000

#define XTMP_HAS_XLMI			0
#define XTMP_XLMI_PADDR			-1
#define XTMP_XLMI_VADDR			-1
#define XTMP_XLMI_SIZE			0
#define XTMP_XLMI_BUSY			0
#define XTMP_XLMI_INBOUND		0

#define XTMP_HAS_DATA_RAM		1
#define XTMP_NUM_DATA_RAMS		2
#define XTMP_DATA_RAM0_PADDR		0x1FF80000
#define XTMP_DATA_RAM0_VADDR		0x1FF80000
#define XTMP_DATA_RAM0_SIZE		0x00040000
#define XTMP_DATA_RAM0_BUSY		1
#define XTMP_DATA_RAM0_INBOUND		1
#define XTMP_DATA_RAM1_PADDR		0x1FFC0000
#define XTMP_DATA_RAM1_VADDR		0x1FFC0000
#define XTMP_DATA_RAM1_SIZE		0x00040000
#define XTMP_DATA_RAM1_BUSY		1
#define XTMP_DATA_RAM1_INBOUND		1

#define XTMP_HAS_DATA_ROM		0
#define XTMP_NUM_DATA_ROMS		0
#define XTMP_DATA_ROM0_PADDR		-1
#define XTMP_DATA_ROM0_VADDR		-1
#define XTMP_DATA_ROM0_SIZE		0
#define XTMP_DATA_ROM0_BUSY		0

#define XTMP_HAS_INST_RAM		1
#define XTMP_NUM_INST_RAMS		1
#define XTMP_INST_RAM0_PADDR		0x20000000
#define XTMP_INST_RAM0_VADDR		0x20000000
#define XTMP_INST_RAM0_SIZE		0x00080000
#define XTMP_INST_RAM0_BUSY		1
#define XTMP_INST_RAM0_INBOUND		1

#define XTMP_HAS_INST_ROM		0
#define XTMP_NUM_INST_ROMS		0
#define XTMP_INST_ROM0_PADDR		-1
#define XTMP_INST_ROM0_VADDR		-1
#define XTMP_INST_ROM0_SIZE		0
#define XTMP_INST_ROM0_BUSY		0

#define XTMP_HAS_BYPASS_REGION		1
#define XTMP_BYPASS_PADDR		0xC0000000
#define XTMP_BYPASS_VADDR		0xC0000000
#define XTMP_BYPASS_PSIZE		0x20000000

#define XTMP_RESET_VECTOR_VADDR		0xFFE00100

#define XTMP_NUM_INTERRUPTS		11
#define XTMP_NUM_EXTERNAL_INTERRUPTS	8

#define XTMP_R_STAGE			0
#define XTMP_E_STAGE			1
#define XTMP_M_STAGE			2
#define XTMP_W_STAGE			3

#define _xim1(w,e,n)		XTMP_insert ## w ## e ## n
#define _xim0(w,e,n)		_xim1(w,e,n)
#define XTMP_PIF_INSERT(n)	_xim0(XTMP_PIF_BITS,XTMP_BYTE_ORDERING,n)
#define XTMP_INSERT(n)		XTMP_PIF_INSERT(n)	/* DEPRECATED */
#define XTMP_LS_INSERT(n)	_xim0(XTMP_LOAD_STORE_BITS,XTMP_BYTE_ORDERING,n)
#define XTMP_IF_INSERT(n)	_xim0(XTMP_IFETCH_BITS,XTMP_BYTE_ORDERING,n)
#define _xem1(w,e,n)		XTMP_extract ## w ## e ## n
#define _xem0(w,e,n)		_xem1(w,e,n)
#define XTMP_PIF_EXTRACT(n)	_xem0(XTMP_PIF_BITS,XTMP_BYTE_ORDERING,n)
#define XTMP_EXTRACT(n)		XTMP_PIF_EXTRACT(n)	/* DEPRECATED */
#define XTMP_LS_EXTRACT(n)	_xem0(XTMP_LOAD_STORE_BITS,XTMP_BYTE_ORDERING,n)
#define XTMP_IF_EXTRACT(n)	_xem0(XTMP_IFETCH_BITS,XTMP_BYTE_ORDERING,n)

#endif /* __XTMP_CONFIG_included */
