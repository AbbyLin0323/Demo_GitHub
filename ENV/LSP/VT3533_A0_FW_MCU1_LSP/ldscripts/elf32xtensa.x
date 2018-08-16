/* This linker script generated from xt-genldscripts.tpp for LSP VT3533_A0_FW_MCU1_LSP */
/* Linker Script for default link */
MEMORY
{
  dram0_data_seg :                    	org = 0x1FFA0800, len = 0x7800
  dram1_mcu12_share_var_seg :         	org = 0x1FFD7C00, len = 0x400
  iram0_Window_text_seg :             	org = 0x20000000, len = 0x178
  iram0_Level2Int_lit_seg :           	org = 0x20000178, len = 0x8
  iram0_Level2Int_text_seg :          	org = 0x20000180, len = 0x38
  iram0_Level3Int_lit_seg :           	org = 0x200001B8, len = 0x8
  iram0_Level3Int_text_seg :          	org = 0x200001C0, len = 0x38
  iram0_Level4Int_lit_seg :           	org = 0x200001F8, len = 0x8
  iram0_Level4Int_text_seg :          	org = 0x20000200, len = 0x38
  iram0_Level5Int_lit_seg :           	org = 0x20000238, len = 0x8
  iram0_Level5Int_text_seg :          	org = 0x20000240, len = 0x38
  iram0_DebugExc_lit_seg :            	org = 0x20000278, len = 0x8
  iram0_DebugExc_text_seg :           	org = 0x20000280, len = 0x38
  iram0_NMIExc_lit_seg :              	org = 0x200002B8, len = 0x8
  iram0_NMIExc_text_seg :             	org = 0x200002C0, len = 0x38
  iram0_KernelExc_lit_seg :           	org = 0x200002F8, len = 0x8
  iram0_KernelExc_text_seg :          	org = 0x20000300, len = 0x38
  iram0_UserExc_lit_seg :             	org = 0x20000338, len = 0x8
  iram0_UserExc_text_seg :            	org = 0x20000340, len = 0x38
  iram0_DoubleExc_lit_seg :           	org = 0x20000378, len = 0x48
  iram0_DoubleExc_text_seg :          	org = 0x200003C0, len = 0x40
  rom_DefaultReset_text_seg :         	org = 0x20000400, len = 0x300
  iram0_text_seg :                    	org = 0x20000700, len = 0x1F900
  sram_mcu1_data_text_seg :           	org = 0x40058000, len = 0x48000
}

PHDRS
{
  dram0_reg_phdr PT_LOAD;
  dram0_hsg_phdr PT_LOAD;
  dram0_sgeq_phdr PT_LOAD;
  dram0_dsg_phdr PT_LOAD;
  dram0_dec_phdr PT_LOAD;
  dram0_prcq_phdr PT_LOAD;
  dram0_ds_phdr PT_LOAD;
  dram0_dec_fifo_phdr PT_LOAD;
  dram0_nfcq_phdr PT_LOAD;
  dram0_ldpc_ecm_phdr PT_LOAD;
  dram0_hge_phdr PT_LOAD;
  dram0_blank_20K_phdr PT_LOAD;
  dram0_daa_phdr PT_LOAD;
  dram0_icb1_phdr PT_LOAD;
  dram0_stack_phdr PT_LOAD;
  dram0_data_phdr PT_LOAD;
  dram0_data_bss_phdr PT_LOAD;
  dram0_blank_96K_phdr PT_LOAD;
  dram1_blank_32K_phdr PT_LOAD;
  dram1_mcu1_alloc_phdr PT_LOAD;
  dram1_mcu01_share_phdr PT_LOAD;
  dram1_mcu2_alloc_phdr PT_LOAD;
  dram1_mcu12_share_phdr PT_LOAD;
  dram1_mcu12_share_var_phdr PT_LOAD;
  dram1_mcu12_share_var_bss_phdr PT_LOAD;
  dram1_mcu012_share_phdr PT_LOAD;
  dram1_blank_128K_phdr PT_LOAD;
  iram0_Window_text_phdr PT_LOAD;
  iram0_Level2Int_lit_phdr PT_LOAD;
  iram0_Level2Int_text_phdr PT_LOAD;
  iram0_Level3Int_lit_phdr PT_LOAD;
  iram0_Level3Int_text_phdr PT_LOAD;
  iram0_Level4Int_lit_phdr PT_LOAD;
  iram0_Level4Int_text_phdr PT_LOAD;
  iram0_Level5Int_lit_phdr PT_LOAD;
  iram0_Level5Int_text_phdr PT_LOAD;
  iram0_DebugExc_lit_phdr PT_LOAD;
  iram0_DebugExc_text_phdr PT_LOAD;
  iram0_NMIExc_lit_phdr PT_LOAD;
  iram0_NMIExc_text_phdr PT_LOAD;
  iram0_KernelExc_lit_phdr PT_LOAD;
  iram0_KernelExc_text_phdr PT_LOAD;
  iram0_UserExc_lit_phdr PT_LOAD;
  iram0_UserExc_text_phdr PT_LOAD;
  iram0_DoubleExc_lit_phdr PT_LOAD;
  iram0_DoubleExc_text_phdr PT_LOAD;
  rom_DefaultReset_text_phdr PT_LOAD;
  iram0_text_phdr PT_LOAD;
  iram0_blank_384K_phdr PT_LOAD;
  spi_0_phdr PT_LOAD;
  sram_rsvd_256K_phdr PT_LOAD;
  sram_bootloader_Part0_phdr PT_LOAD;
  sram_bootloader_Part1_phdr PT_LOAD;
  sram_fw_static_info_phdr PT_LOAD;
  sram_mcu0_data_text_phdr PT_LOAD;
  sram_mcu1_data_text_phdr PT_LOAD;
  sram_mcu2_data_text_phdr PT_LOAD;
  sram_mcu0_dsram0_backup_phdr PT_LOAD;
  sram_mcu1_dsram0_backup_phdr PT_LOAD;
  sram_mcu2_dsram0_backup_phdr PT_LOAD;
  sram_mcu0_isram_backup_phdr PT_LOAD;
  sram_mcu1_isram_backup_phdr PT_LOAD;
  sram_mcu2_isram_backup_phdr PT_LOAD;
  sram_option_rom_phdr PT_LOAD;
  sram_bl_temp_buff_phdr PT_LOAD;
  sram_mcu012_dsram1_backup_phdr PT_LOAD;
  sram_otfb128K_backup_phdr PT_LOAD;
  sram_128k_align_rsvd_phdr PT_LOAD;
  sram_fw_hal_trace_phdr PT_LOAD;
  sram_mcu0_alloc_buff_phdr PT_LOAD;
  sram_mcu0_entrance_lit_phdr PT_LOAD;
  sram_mcu0_entrance_text_phdr PT_LOAD;
  sram_mcu0_head_infos_phdr PT_LOAD;
  sram_mcu1_alloc_buff_phdr PT_LOAD;
  sram_mcu2_alloc_buff_phdr PT_LOAD;
  sram_no_memory_512M_phdr PT_LOAD;
  sram_remap8_phdr PT_LOAD;
  rom_data_text_phdr PT_LOAD;
  apb_0_phdr PT_LOAD;
  otfb_bootloader_phdr PT_LOAD;
  otfb_mcu12_red_phdr PT_LOAD;
  otfb_mcu12_ssu0_phdr PT_LOAD;
  otfb_mcu2_cache_status_phdr PT_LOAD;
  otfb_tl_sec_phdr PT_LOAD;
  otfb_mcu1_fw_alloc_phdr PT_LOAD;
  otfb_mcu0_fw_alloc_phdr PT_LOAD;
  otfb_mcu2_fw_alloc_phdr PT_LOAD;
  otfb_fw_rsvd_128K_phdr PT_LOAD;
  otfb_sge_on_the_fly_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)

/*  Memory boundary addresses:  */
_memmap_mem_apb_start = 0xffe07000;
_memmap_mem_apb_end   = 0xffe10000;
_memmap_mem_dram0_start = 0x1ff80000;
_memmap_mem_dram0_end   = 0x1ffc0000;
_memmap_mem_dram1_start = 0x1ffc0000;
_memmap_mem_dram1_end   = 0x20000000;
_memmap_mem_iram0_start = 0x20000000;
_memmap_mem_iram0_end   = 0x20080000;
_memmap_mem_otfb_start = 0xfff00000;
_memmap_mem_otfb_end   = 0xfff80000;
_memmap_mem_rom_start = 0xffe00000;
_memmap_mem_rom_end   = 0xffe06000;
_memmap_mem_sram_4_start = 0x40000000;
_memmap_mem_sram_4_end   = 0x80000000;
_memmap_mem_sram_8_start = 0x80000000;
_memmap_mem_sram_8_end   = 0xc0000000;
_memmap_mem_spi_start = 0x30000000;
_memmap_mem_spi_end   = 0x40000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_dram0_data_start = 0x1ffa0800;
_memmap_seg_dram0_data_max   = 0x1ffa8000;
_memmap_seg_dram1_mcu12_share_var_start = 0x1ffd7c00;
_memmap_seg_dram1_mcu12_share_var_max   = 0x1ffd8000;
_memmap_seg_iram0_Window_text_start = 0x20000000;
_memmap_seg_iram0_Window_text_max   = 0x20000178;
_memmap_seg_iram0_Level2Int_lit_start = 0x20000178;
_memmap_seg_iram0_Level2Int_lit_max   = 0x20000180;
_memmap_seg_iram0_Level2Int_text_start = 0x20000180;
_memmap_seg_iram0_Level2Int_text_max   = 0x200001b8;
_memmap_seg_iram0_Level3Int_lit_start = 0x200001b8;
_memmap_seg_iram0_Level3Int_lit_max   = 0x200001c0;
_memmap_seg_iram0_Level3Int_text_start = 0x200001c0;
_memmap_seg_iram0_Level3Int_text_max   = 0x200001f8;
_memmap_seg_iram0_Level4Int_lit_start = 0x200001f8;
_memmap_seg_iram0_Level4Int_lit_max   = 0x20000200;
_memmap_seg_iram0_Level4Int_text_start = 0x20000200;
_memmap_seg_iram0_Level4Int_text_max   = 0x20000238;
_memmap_seg_iram0_Level5Int_lit_start = 0x20000238;
_memmap_seg_iram0_Level5Int_lit_max   = 0x20000240;
_memmap_seg_iram0_Level5Int_text_start = 0x20000240;
_memmap_seg_iram0_Level5Int_text_max   = 0x20000278;
_memmap_seg_iram0_DebugExc_lit_start = 0x20000278;
_memmap_seg_iram0_DebugExc_lit_max   = 0x20000280;
_memmap_seg_iram0_DebugExc_text_start = 0x20000280;
_memmap_seg_iram0_DebugExc_text_max   = 0x200002b8;
_memmap_seg_iram0_NMIExc_lit_start = 0x200002b8;
_memmap_seg_iram0_NMIExc_lit_max   = 0x200002c0;
_memmap_seg_iram0_NMIExc_text_start = 0x200002c0;
_memmap_seg_iram0_NMIExc_text_max   = 0x200002f8;
_memmap_seg_iram0_KernelExc_lit_start = 0x200002f8;
_memmap_seg_iram0_KernelExc_lit_max   = 0x20000300;
_memmap_seg_iram0_KernelExc_text_start = 0x20000300;
_memmap_seg_iram0_KernelExc_text_max   = 0x20000338;
_memmap_seg_iram0_UserExc_lit_start = 0x20000338;
_memmap_seg_iram0_UserExc_lit_max   = 0x20000340;
_memmap_seg_iram0_UserExc_text_start = 0x20000340;
_memmap_seg_iram0_UserExc_text_max   = 0x20000378;
_memmap_seg_iram0_DoubleExc_lit_start = 0x20000378;
_memmap_seg_iram0_DoubleExc_lit_max   = 0x200003c0;
_memmap_seg_iram0_DoubleExc_text_start = 0x200003c0;
_memmap_seg_iram0_DoubleExc_text_max   = 0x20000400;
_memmap_seg_rom_DefaultReset_text_start = 0x20000400;
_memmap_seg_rom_DefaultReset_text_max   = 0x20000700;
_memmap_seg_iram0_text_start = 0x20000700;
_memmap_seg_iram0_text_max   = 0x20020000;
_memmap_seg_sram_mcu1_data_text_start = 0x40058000;
_memmap_seg_sram_mcu1_data_text_max   = 0x400a0000;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x20000000);
PROVIDE(_memmap_reset_vector = 0x20000400);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x40444444;
_memmap_cacheattr_wt_base = 0x10111111;
_memmap_cacheattr_bp_base = 0x20222222;
_memmap_cacheattr_unused_mask = 0x0F000000;
_memmap_cacheattr_wb_trapnull = 0x42444444;
_memmap_cacheattr_wba_trapnull = 0x42444444;
_memmap_cacheattr_wbna_trapnull = 0x52555555;
_memmap_cacheattr_wt_trapnull = 0x12111111;
_memmap_cacheattr_bp_trapnull = 0x22222222;
_memmap_cacheattr_wb_strict = 0x4F444444;
_memmap_cacheattr_wt_strict = 0x1F111111;
_memmap_cacheattr_bp_strict = 0x2F222222;
_memmap_cacheattr_wb_allvalid = 0x42444444;
_memmap_cacheattr_wt_allvalid = 0x12111111;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{
  _stack_sentry = 0x1ffa0000;
  __stack = 0x1ffa0800;

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    _data_end = ABSOLUTE(.);
  } >dram0_data_seg :dram0_data_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _memmap_seg_dram0_data_end = ALIGN(0x8);
  } >dram0_data_seg :dram0_data_bss_phdr
  _heap_sentry = 0x1ffa8000;

  .mcu12.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _mcu12_bss_start = ABSOLUTE(.);
    *(.mcu12.bss)
    . = ALIGN (8);
    _mcu12_bss_end = ABSOLUTE(.);
    _memmap_seg_dram1_mcu12_share_var_end = ALIGN(0x8);
  } >dram1_mcu12_share_var_seg :dram1_mcu12_share_var_bss_phdr

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_Window_text_end = ALIGN(0x8);
  } >iram0_Window_text_seg :iram0_Window_text_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level2Int_lit_end = ALIGN(0x8);
  } >iram0_Level2Int_lit_seg :iram0_Level2Int_lit_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level2Int_text_end = ALIGN(0x8);
  } >iram0_Level2Int_text_seg :iram0_Level2Int_text_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level3Int_lit_end = ALIGN(0x8);
  } >iram0_Level3Int_lit_seg :iram0_Level3Int_lit_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level3Int_text_end = ALIGN(0x8);
  } >iram0_Level3Int_text_seg :iram0_Level3Int_text_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level4Int_lit_end = ALIGN(0x8);
  } >iram0_Level4Int_lit_seg :iram0_Level4Int_lit_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level4Int_text_end = ALIGN(0x8);
  } >iram0_Level4Int_text_seg :iram0_Level4Int_text_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level5Int_lit_end = ALIGN(0x8);
  } >iram0_Level5Int_lit_seg :iram0_Level5Int_lit_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_Level5Int_text_end = ALIGN(0x8);
  } >iram0_Level5Int_text_seg :iram0_Level5Int_text_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_DebugExc_lit_end = ALIGN(0x8);
  } >iram0_DebugExc_lit_seg :iram0_DebugExc_lit_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_DebugExc_text_end = ALIGN(0x8);
  } >iram0_DebugExc_text_seg :iram0_DebugExc_text_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_NMIExc_lit_end = ALIGN(0x8);
  } >iram0_NMIExc_lit_seg :iram0_NMIExc_lit_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_NMIExc_text_end = ALIGN(0x8);
  } >iram0_NMIExc_text_seg :iram0_NMIExc_text_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_KernelExc_lit_end = ALIGN(0x8);
  } >iram0_KernelExc_lit_seg :iram0_KernelExc_lit_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_KernelExc_text_end = ALIGN(0x8);
  } >iram0_KernelExc_text_seg :iram0_KernelExc_text_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_UserExc_lit_end = ALIGN(0x8);
  } >iram0_UserExc_lit_seg :iram0_UserExc_lit_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_UserExc_text_end = ALIGN(0x8);
  } >iram0_UserExc_text_seg :iram0_UserExc_text_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_DoubleExc_lit_end = ALIGN(0x8);
  } >iram0_DoubleExc_lit_seg :iram0_DoubleExc_lit_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_DoubleExc_text_end = ALIGN(0x8);
  } >iram0_DoubleExc_text_seg :iram0_DoubleExc_text_phdr

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_DefaultReset_text_end = ALIGN(0x8);
  } >rom_DefaultReset_text_seg :rom_DefaultReset_text_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >iram0_text_seg :iram0_text_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    LONG(_mcu12_bss_start)
    LONG(_mcu12_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >sram_mcu1_data_text_seg :sram_mcu1_data_text_phdr

  .mcu1dram.data : ALIGN(4)
  {
    _mcu1dram_data_start = ABSOLUTE(.);
    *(.mcu1dram.data)
    _mcu1dram_data_end = ABSOLUTE(.);
  } >sram_mcu1_data_text_seg :sram_mcu1_data_text_phdr

  .mcu1dram.text : ALIGN(4)
  {
    _mcu1dram_text_start = ABSOLUTE(.);
    *(.mcu1dram.literal .mcu1dram.text)
    _mcu1dram_text_end = ABSOLUTE(.);
    _memmap_seg_sram_mcu1_data_text_end = ALIGN(0x8);
  } >sram_mcu1_data_text_seg :sram_mcu1_data_text_phdr
  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}

