/* This linker script generated from xt-genldscripts.tpp for LSP D:/workspace/VT3514_B0_ROM_ISRAM_LSP/RE-2014.5/viatie/VT3514_B0_ROM_ISRAM_LSP */
/* Linker Script for default link */
MEMORY
{
  dram0_usr_seg :                     	org = 0x1FFA4800, len = 0x13800
  iram0_0_seg :                       	org = 0x20000000, len = 0x178
  iram0_1_seg :                       	org = 0x20000178, len = 0x8
  iram0_2_seg :                       	org = 0x20000180, len = 0x38
  iram0_3_seg :                       	org = 0x200001B8, len = 0x8
  iram0_4_seg :                       	org = 0x200001C0, len = 0x38
  iram0_5_seg :                       	org = 0x200001F8, len = 0x8
  iram0_6_seg :                       	org = 0x20000200, len = 0x38
  iram0_7_seg :                       	org = 0x20000238, len = 0x8
  iram0_8_seg :                       	org = 0x20000240, len = 0x38
  iram0_9_seg :                       	org = 0x20000278, len = 0x8
  iram0_10_seg :                      	org = 0x20000280, len = 0x38
  iram0_11_seg :                      	org = 0x200002B8, len = 0x8
  iram0_12_seg :                      	org = 0x200002C0, len = 0x38
  iram0_13_seg :                      	org = 0x200002F8, len = 0x8
  iram0_14_seg :                      	org = 0x20000300, len = 0x38
  iram0_15_seg :                      	org = 0x20000338, len = 0x8
  iram0_16_seg :                      	org = 0x20000340, len = 0x38
  iram0_17_seg :                      	org = 0x20000378, len = 0x8
  iram0_18_seg :                      	org = 0x20000380, len = 0x20
  iram_alt_rst_lit_seg :              	org = 0x200003A0, len = 0x20
  iram_alt_rst_vec_seg :              	org = 0x200003C0, len = 0x40
  iram0_usr_seg :                     	org = 0x20000400, len = 0x27C00
  spi_0_seg :                         	org = 0xC0000000, len = 0x10000000
  srom2_seg :                         	org = 0xFFE003E0, len = 0x5C20
  otfb_usr_seg :                      	org = 0xFFF00000, len = 0x5000
  otfb_usr1_seg :                     	org = 0xFFF05000, len = 0x4B000
}

PHDRS
{
  dram0_reg_phdr PT_LOAD;
  dram0_hsg_phdr PT_LOAD;
  dram0_nfcq_phdr PT_LOAD;
  dram0_dsg_phdr PT_LOAD;
  dram0_prcq_phdr PT_LOAD;
  dram0_sgeq_phdr PT_LOAD;
  dram0_ldpc_n1_phdr PT_LOAD;
  dram0_ldpc_sv_phdr PT_LOAD;
  dram0_blank0_phdr PT_LOAD;
  dram0_stack_phdr PT_LOAD;
  dram0_usr_phdr PT_LOAD;
  dram0_usr_bss_phdr PT_LOAD;
  dram0_blank_phdr PT_LOAD;
  drma1_usr_phdr PT_LOAD;
  dram1_com0_phdr PT_LOAD;
  dram1_trace_phdr PT_LOAD;
  dram1_com1_phdr PT_LOAD;
  dram1_blank_phdr PT_LOAD;
  iram0_0_phdr PT_LOAD;
  iram0_1_phdr PT_LOAD;
  iram0_2_phdr PT_LOAD;
  iram0_3_phdr PT_LOAD;
  iram0_4_phdr PT_LOAD;
  iram0_5_phdr PT_LOAD;
  iram0_6_phdr PT_LOAD;
  iram0_7_phdr PT_LOAD;
  iram0_8_phdr PT_LOAD;
  iram0_9_phdr PT_LOAD;
  iram0_10_phdr PT_LOAD;
  iram0_11_phdr PT_LOAD;
  iram0_12_phdr PT_LOAD;
  iram0_13_phdr PT_LOAD;
  iram0_14_phdr PT_LOAD;
  iram0_15_phdr PT_LOAD;
  iram0_16_phdr PT_LOAD;
  iram0_17_phdr PT_LOAD;
  iram0_18_phdr PT_LOAD;
  iram_alt_rst_lit_phdr PT_LOAD;
  iram_alt_rst_vec_phdr PT_LOAD;
  iram0_usr_phdr PT_LOAD;
  iram0_blank_phdr PT_LOAD;
  entrance_phdr PT_LOAD;
  head_phdr PT_LOAD;
  blank_phdr PT_LOAD;
  sram0_phdr PT_LOAD;
  sram_remap8_phdr PT_LOAD;
  spi_0_phdr PT_LOAD;
  spi_0_bss_phdr PT_LOAD;
  srom0_phdr PT_LOAD;
  srom1_phdr PT_LOAD;
  srom2_phdr PT_LOAD;
  apb_0_phdr PT_LOAD;
  otfb_usr_phdr PT_LOAD;
  otfb_usr1_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)
_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x20000000);
PROVIDE(_memmap_reset_vector = 0x200003c0);
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
    LONG(_spi_bss_start)
    LONG(_spi_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >dram0_usr_seg :dram0_usr_phdr

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
  } >dram0_usr_seg :dram0_usr_phdr

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
  } >dram0_usr_seg :dram0_usr_bss_phdr

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_1_seg :iram0_1_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_2_seg :iram0_2_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_3_seg :iram0_3_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_4_seg :iram0_4_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_5_seg :iram0_5_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_6_seg :iram0_6_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_7_seg :iram0_7_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_8_seg :iram0_8_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_9_seg :iram0_9_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_10_seg :iram0_10_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_11_seg :iram0_11_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_12_seg :iram0_12_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_13_seg :iram0_13_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_14_seg :iram0_14_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_15_seg :iram0_15_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_16_seg :iram0_16_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_17_seg :iram0_17_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_18_seg :iram0_18_phdr

  .ResetVector.literal : ALIGN(4)
  {
    _ResetVector_literal_start = ABSOLUTE(.);
    *(.ResetVector.literal)
    _ResetVector_literal_end = ABSOLUTE(.);
  } >iram_alt_rst_lit_seg :iram_alt_rst_lit_phdr

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
  } >iram_alt_rst_vec_seg :iram_alt_rst_vec_phdr

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
  } >iram0_usr_seg :iram0_usr_phdr

  rom_func_table : ALIGN(4)
  {
    rom_func_table_start = ABSOLUTE(.);
    *(rom_func_table)
    rom_func_table_end = ABSOLUTE(.);
  } >iram0_usr_seg :iram0_usr_phdr

  .spi.data : ALIGN(4)
  {
    _spi_data_start = ABSOLUTE(.);
    KEEP (*(.spi.data))
    _spi_data_end = ABSOLUTE(.);
  } >spi_0_seg :spi_0_phdr

  .spi.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _spi_bss_start = ABSOLUTE(.);
    KEEP (*(.spi.bss))
    . = ALIGN (8);
    _spi_bss_end = ABSOLUTE(.);
  } >spi_0_seg :spi_0_bss_phdr

  .srom.rodata : ALIGN(4)
  {
    _srom_rodata_start = ABSOLUTE(.);
    *(.srom.rodata)
    _srom_rodata_end = ABSOLUTE(.);
  } >srom2_seg :srom2_phdr

  .srom.text : ALIGN(4)
  {
    _srom_text_start = ABSOLUTE(.);
    *(.srom.literal .srom.text)
    _srom_text_end = ABSOLUTE(.);
  } >srom2_seg :srom2_phdr

  .entry.literal : ALIGN(4)
  {
    _entry_literal_start = ABSOLUTE(.);
    KEEP (*(.entry.literal))
    _entry_literal_end = ABSOLUTE(.);
  } >otfb_usr_seg :otfb_usr_phdr

  .entry.text : ALIGN(4)
  {
    _entry_text_start = ABSOLUTE(.);
    KEEP (*(.entry.text))
    _entry_text_end = ABSOLUTE(.);
  } >otfb_usr_seg :otfb_usr_phdr

  .otfb_usr0.text : ALIGN(4)
  {
    _otfb_usr0_text_start = ABSOLUTE(.);
    *(.otfb_usr0.literal .otfb_usr0.text)
    _otfb_usr0_text_end = ABSOLUTE(.);
  } >otfb_usr1_seg :otfb_usr1_phdr
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
  .xt.profile_range 0 :
  {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges 0 :
  {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files 0 :
  {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}

