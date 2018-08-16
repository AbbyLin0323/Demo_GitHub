/* This linker script generated from xt-genldscripts.tpp for LSP D:/workspace-rf/VT3514_C0_ROM_LSP/RF-2014.1/rfviatieloop16/VT3514_C0_ROM_LSP */
/* Linker Script for default link */
MEMORY
{
  dram0_data_seg :                    	org = 0x1FFA4800, len = 0x3800
  rom_DefaultReset_text_seg :         	org = 0xFFE00000, len = 0x300
  rom_DefaultMemoryExc_lit_seg :      	org = 0xFFE00300, len = 0x30
  rom_DefaultMemoryExc_text_seg :     	org = 0xFFE00330, len = 0x180
  window_vector_text_seg :            	org = 0xFFE00800, len = 0x178
  rom_level2_vec_lit_seg :            	org = 0xFFE00978, len = 0x8
  rom_level2_vec_text_seg :           	org = 0xFFE00980, len = 0x38
  rom_level3_vec_lit_seg :            	org = 0xFFE009B8, len = 0x8
  rom_level3_vec_text_seg :           	org = 0xFFE009C0, len = 0x38
  rom_level4_vec_lit_seg :            	org = 0xFFE009F8, len = 0x8
  rom_level4_vec_text_seg :           	org = 0xFFE00A00, len = 0x38
  rom_level5_vec_lit_seg :            	org = 0xFFE00A38, len = 0x8
  rom_level5_vec_text_seg :           	org = 0xFFE00A40, len = 0x38
  rom_level6_vec_lit_seg :            	org = 0xFFE00A78, len = 0x8
  rom_level6_vec_text_seg :           	org = 0xFFE00A80, len = 0x38
  rom_nmi_vec_lit_seg :               	org = 0xFFE00AB8, len = 0x8
  rom_nmi_vec_text_seg :              	org = 0xFFE00AC0, len = 0x38
  rom_kernel_vec_lit_seg :            	org = 0xFFE00AF8, len = 0x8
  rom_kernel_vec_text_seg :           	org = 0xFFE00B00, len = 0x38
  rom_user_vec_lit_seg :              	org = 0xFFE00B38, len = 0x8
  rom_user_vec_text_seg :             	org = 0xFFE00B40, len = 0x38
  rom_double_vec_lit_seg :            	org = 0xFFE00B78, len = 0x48
  rom_double_vec_text_seg :           	org = 0xFFE00BC0, len = 0x40
  rom_text_seg :                      	org = 0xFFE00C00, len = 0x53C0
  rom_func_table_seg :                	org = 0xFFE05FC0, len = 0x40
}

PHDRS
{
  dram0_reg_phdr PT_LOAD;
  dram0_hsg_phdr PT_LOAD;
  dram0_sgeq_phdr PT_LOAD;
  dram0_ldpc_n1_phdr PT_LOAD;
  dram0_dsg_phdr PT_LOAD;
  dram0_nfcq_phdr PT_LOAD;
  dram0_prcq_phdr PT_LOAD;
  dram0_em_lba_phdr PT_LOAD;
  dram0_ldpc_sv_phdr PT_LOAD;
  dram0_blank_60K_phdr PT_LOAD;
  dram0_stack_phdr PT_LOAD;
  dram0_data_phdr PT_LOAD;
  dram0_data_bss_phdr PT_LOAD;
  dram0_blank_96K_phdr PT_LOAD;
  dram1_blank_32K_phdr PT_LOAD;
  dram1_mcu1_alloc_phdr PT_LOAD;
  dram1_mcu01_share_phdr PT_LOAD;
  dram1_mcu2_alloc_phdr PT_LOAD;
  dram1_mcu02_share_phdr PT_LOAD;
  dram1_mcu012_share_phdr PT_LOAD;
  dram1_blank_128K_phdr PT_LOAD;
  iram0_window_text_phdr PT_LOAD;
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
  iram0_AltReset_text_phdr PT_LOAD;
  iram0_AltMemoryExc_lit_phdr PT_LOAD;
  iram0_AltMemoryExc_text_phdr PT_LOAD;
  iram0_text_phdr PT_LOAD;
  iram0_blank_480K_phdr PT_LOAD;
  sram_rsvd_256K_phdr PT_LOAD;
  sram_static_param_phdr PT_LOAD;
  sram_bootloader_llf_phdr PT_LOAD;
  sram_para_table_phdr PT_LOAD;
  sram_mcu0_entrance_lit_phdr PT_LOAD;
  sram_mcu0_entrance_text_phdr PT_LOAD;
  sram_mcu0_head_phdr PT_LOAD;
  sram_mcu0_data_text_phdr PT_LOAD;
  sram_mcu12_rodata_phdr PT_LOAD;
  sram_mcu12_text_phdr PT_LOAD;
  sram_mcu0_dsram0_backup_phdr PT_LOAD;
  sram_mcu1_dsram0_backup_phdr PT_LOAD;
  sram_mcu2_dsram0_backup_phdr PT_LOAD;
  sram_mcu0_isram_backup_phdr PT_LOAD;
  sram_mcu12_isram_backup_phdr PT_LOAD;
  sram_mcu012_dsram1_backup_phdr PT_LOAD;
  sram_otfb64K_backup_phdr PT_LOAD;
  sram_align_rsvd_phdr PT_LOAD;
  sram_mcu0_alloc_buff_phdr PT_LOAD;
  sram_mcu1_alloc_buff_phdr PT_LOAD;
  sram_mcu2_alloc_buff_phdr PT_LOAD;
  sram_no_memory_512M_phdr PT_LOAD;
  sram_remap8_phdr PT_LOAD;
  spi_0_phdr PT_LOAD;
  rom_DefaultReset_text_phdr PT_LOAD;
  rom_DefaultMemoryExc_lit_phdr PT_LOAD;
  rom_DefaultMemoryExc_text_phdr PT_LOAD;
  window_vector_text_phdr PT_LOAD;
  rom_level2_vec_lit_phdr PT_LOAD;
  rom_level2_vec_text_phdr PT_LOAD;
  rom_level3_vec_lit_phdr PT_LOAD;
  rom_level3_vec_text_phdr PT_LOAD;
  rom_level4_vec_lit_phdr PT_LOAD;
  rom_level4_vec_text_phdr PT_LOAD;
  rom_level5_vec_lit_phdr PT_LOAD;
  rom_level5_vec_text_phdr PT_LOAD;
  rom_level6_vec_lit_phdr PT_LOAD;
  rom_level6_vec_text_phdr PT_LOAD;
  rom_nmi_vec_lit_phdr PT_LOAD;
  rom_nmi_vec_text_phdr PT_LOAD;
  rom_kernel_vec_lit_phdr PT_LOAD;
  rom_kernel_vec_text_phdr PT_LOAD;
  rom_user_vec_lit_phdr PT_LOAD;
  rom_user_vec_text_phdr PT_LOAD;
  rom_double_vec_lit_phdr PT_LOAD;
  rom_double_vec_text_phdr PT_LOAD;
  rom_text_phdr PT_LOAD;
  rom_func_table_phdr PT_LOAD;
  apb_0_phdr PT_LOAD;
  otfb_bootloader_phdr PT_LOAD;
  otfb_mcu1_ssu0_phdr PT_LOAD;
  otfb_mcu2_ssu0_phdr PT_LOAD;
  otfb_mcu1_cache_status_phdr PT_LOAD;
  otfb_mcu2_cache_status_phdr PT_LOAD;
  otfb_mcu1_red_phdr PT_LOAD;
  otfb_mcu2_red_phdr PT_LOAD;
  otfb_mcu0_fw_alloc_phdr PT_LOAD;
  otfb_mcu1_fw_alloc_phdr PT_LOAD;
  otfb_mcu2_fw_alloc_phdr PT_LOAD;
  otfb_fw_rsvd_64K_phdr PT_LOAD;
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
_memmap_mem_otfb_end   = 0xfff60000;
_memmap_mem_rom_start = 0xffe00000;
_memmap_mem_rom_end   = 0xffe06000;
_memmap_mem_spi_start = 0xc0000000;
_memmap_mem_spi_end   = 0xd0000000;
_memmap_mem_sram_4_start = 0x40000000;
_memmap_mem_sram_4_end   = 0x80000000;
_memmap_mem_sram_8_start = 0x80000000;
_memmap_mem_sram_8_end   = 0xc0000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_dram0_data_start = 0x1ffa4800;
_memmap_seg_dram0_data_max   = 0x1ffa8000;
_memmap_seg_rom_DefaultReset_text_start = 0xffe00000;
_memmap_seg_rom_DefaultReset_text_max   = 0xffe00300;
_memmap_seg_rom_DefaultMemoryExc_lit_start = 0xffe00300;
_memmap_seg_rom_DefaultMemoryExc_lit_max   = 0xffe00330;
_memmap_seg_rom_DefaultMemoryExc_text_start = 0xffe00330;
_memmap_seg_rom_DefaultMemoryExc_text_max   = 0xffe004b0;
_memmap_seg_window_vector_text_start = 0xffe00800;
_memmap_seg_window_vector_text_max   = 0xffe00978;
_memmap_seg_rom_level2_vec_lit_start = 0xffe00978;
_memmap_seg_rom_level2_vec_lit_max   = 0xffe00980;
_memmap_seg_rom_level2_vec_text_start = 0xffe00980;
_memmap_seg_rom_level2_vec_text_max   = 0xffe009b8;
_memmap_seg_rom_level3_vec_lit_start = 0xffe009b8;
_memmap_seg_rom_level3_vec_lit_max   = 0xffe009c0;
_memmap_seg_rom_level3_vec_text_start = 0xffe009c0;
_memmap_seg_rom_level3_vec_text_max   = 0xffe009f8;
_memmap_seg_rom_level4_vec_lit_start = 0xffe009f8;
_memmap_seg_rom_level4_vec_lit_max   = 0xffe00a00;
_memmap_seg_rom_level4_vec_text_start = 0xffe00a00;
_memmap_seg_rom_level4_vec_text_max   = 0xffe00a38;
_memmap_seg_rom_level5_vec_lit_start = 0xffe00a38;
_memmap_seg_rom_level5_vec_lit_max   = 0xffe00a40;
_memmap_seg_rom_level5_vec_text_start = 0xffe00a40;
_memmap_seg_rom_level5_vec_text_max   = 0xffe00a78;
_memmap_seg_rom_level6_vec_lit_start = 0xffe00a78;
_memmap_seg_rom_level6_vec_lit_max   = 0xffe00a80;
_memmap_seg_rom_level6_vec_text_start = 0xffe00a80;
_memmap_seg_rom_level6_vec_text_max   = 0xffe00ab8;
_memmap_seg_rom_nmi_vec_lit_start = 0xffe00ab8;
_memmap_seg_rom_nmi_vec_lit_max   = 0xffe00ac0;
_memmap_seg_rom_nmi_vec_text_start = 0xffe00ac0;
_memmap_seg_rom_nmi_vec_text_max   = 0xffe00af8;
_memmap_seg_rom_kernel_vec_lit_start = 0xffe00af8;
_memmap_seg_rom_kernel_vec_lit_max   = 0xffe00b00;
_memmap_seg_rom_kernel_vec_text_start = 0xffe00b00;
_memmap_seg_rom_kernel_vec_text_max   = 0xffe00b38;
_memmap_seg_rom_user_vec_lit_start = 0xffe00b38;
_memmap_seg_rom_user_vec_lit_max   = 0xffe00b40;
_memmap_seg_rom_user_vec_text_start = 0xffe00b40;
_memmap_seg_rom_user_vec_text_max   = 0xffe00b78;
_memmap_seg_rom_double_vec_lit_start = 0xffe00b78;
_memmap_seg_rom_double_vec_lit_max   = 0xffe00bc0;
_memmap_seg_rom_double_vec_text_start = 0xffe00bc0;
_memmap_seg_rom_double_vec_text_max   = 0xffe00c00;
_memmap_seg_rom_text_start = 0xffe00c00;
_memmap_seg_rom_text_max   = 0xffe05fc0;
_memmap_seg_rom_func_table_start = 0xffe05fc0;
_memmap_seg_rom_func_table_max   = 0xffe06000;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0xffe00800);
PROVIDE(_memmap_reset_vector = 0xffe00000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x44444444;
_memmap_cacheattr_wt_base = 0x11111111;
_memmap_cacheattr_bp_base = 0x22222222;
_memmap_cacheattr_unused_mask = 0x00000000;
_memmap_cacheattr_wb_trapnull = 0x44444444;
_memmap_cacheattr_wba_trapnull = 0x44444444;
_memmap_cacheattr_wbna_trapnull = 0x55555555;
_memmap_cacheattr_wt_trapnull = 0x11111111;
_memmap_cacheattr_bp_trapnull = 0x22222222;
_memmap_cacheattr_wb_strict = 0x44444444;
_memmap_cacheattr_wt_strict = 0x11111111;
_memmap_cacheattr_bp_strict = 0x22222222;
_memmap_cacheattr_wb_allvalid = 0x44444444;
_memmap_cacheattr_wt_allvalid = 0x11111111;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{
  _end = 0x1ffa4000;
  PROVIDE(end = 0x1ffa4000);
  _stack_sentry = 0x1ffa4000;
  __stack = 0x1ffa4800;
  _heap_sentry = 0x1ffa4800;

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
    _memmap_seg_dram0_data_end = ALIGN(0x8);
  } >dram0_data_seg :dram0_data_bss_phdr

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_DefaultReset_text_end = ALIGN(0x8);
  } >rom_DefaultReset_text_seg :rom_DefaultReset_text_phdr

  .MemoryExceptionVector.literal : ALIGN(4)
  {
    _MemoryExceptionVector_literal_start = ABSOLUTE(.);
    *(.MemoryExceptionVector.literal)
    _MemoryExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_DefaultMemoryExc_lit_end = ALIGN(0x8);
  } >rom_DefaultMemoryExc_lit_seg :rom_DefaultMemoryExc_lit_phdr

  .MemoryExceptionVector.text : ALIGN(4)
  {
    _MemoryExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.text))
    _MemoryExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_DefaultMemoryExc_text_end = ALIGN(0x8);
  } >rom_DefaultMemoryExc_text_seg :rom_DefaultMemoryExc_text_phdr

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
    _memmap_seg_window_vector_text_end = ALIGN(0x8);
  } >window_vector_text_seg :window_vector_text_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_level2_vec_lit_end = ALIGN(0x8);
  } >rom_level2_vec_lit_seg :rom_level2_vec_lit_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_level2_vec_text_end = ALIGN(0x8);
  } >rom_level2_vec_text_seg :rom_level2_vec_text_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_level3_vec_lit_end = ALIGN(0x8);
  } >rom_level3_vec_lit_seg :rom_level3_vec_lit_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_level3_vec_text_end = ALIGN(0x8);
  } >rom_level3_vec_text_seg :rom_level3_vec_text_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_level4_vec_lit_end = ALIGN(0x8);
  } >rom_level4_vec_lit_seg :rom_level4_vec_lit_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_level4_vec_text_end = ALIGN(0x8);
  } >rom_level4_vec_text_seg :rom_level4_vec_text_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_level5_vec_lit_end = ALIGN(0x8);
  } >rom_level5_vec_lit_seg :rom_level5_vec_lit_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_level5_vec_text_end = ALIGN(0x8);
  } >rom_level5_vec_text_seg :rom_level5_vec_text_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_level6_vec_lit_end = ALIGN(0x8);
  } >rom_level6_vec_lit_seg :rom_level6_vec_lit_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_level6_vec_text_end = ALIGN(0x8);
  } >rom_level6_vec_text_seg :rom_level6_vec_text_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_nmi_vec_lit_end = ALIGN(0x8);
  } >rom_nmi_vec_lit_seg :rom_nmi_vec_lit_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_nmi_vec_text_end = ALIGN(0x8);
  } >rom_nmi_vec_text_seg :rom_nmi_vec_text_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_kernel_vec_lit_end = ALIGN(0x8);
  } >rom_kernel_vec_lit_seg :rom_kernel_vec_lit_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_kernel_vec_text_end = ALIGN(0x8);
  } >rom_kernel_vec_text_seg :rom_kernel_vec_text_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_user_vec_lit_end = ALIGN(0x8);
  } >rom_user_vec_lit_seg :rom_user_vec_lit_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_user_vec_text_end = ALIGN(0x8);
  } >rom_user_vec_text_seg :rom_user_vec_text_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_rom_double_vec_lit_end = ALIGN(0x8);
  } >rom_double_vec_lit_seg :rom_double_vec_lit_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_rom_double_vec_text_end = ALIGN(0x8);
  } >rom_double_vec_text_seg :rom_double_vec_text_phdr

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
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >rom_text_seg :rom_text_phdr

  .literal : ALIGN(4)
  {
    _literal_start = ABSOLUTE(.);
    *(.init.literal)
    *(.literal)
    *(.literal.*)
    *(.gnu.linkonce.literal.*)
    *(.gnu.linkonce.t.*.literal)
    *(.fini.literal)
    _literal_end = ABSOLUTE(.);
  } >rom_text_seg :rom_text_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    KEEP (*(.init))
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    *(.stub)
    *(.gnu.warning)
    KEEP (*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >rom_text_seg :rom_text_phdr

  rom_func_table : ALIGN(4)
  {
    rom_func_table_start = ABSOLUTE(.);
    *(rom_func_table)
    rom_func_table_end = ABSOLUTE(.);
    _memmap_seg_rom_func_table_end = ALIGN(0x8);
  } >rom_func_table_seg :rom_func_table_phdr
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

