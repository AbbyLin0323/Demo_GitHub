/* This linker script generated from xt-genldscripts.tpp for LSP VT3533_A0_BOOTLOADER_LSP */
/* Linker Script for default link */
MEMORY
{
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
  iram0_AltReset_text_seg :           	org = 0x20000400, len = 0x300
  iram0_AltMemoryExc_lit_seg :        	org = 0x20000700, len = 0x30
  iram0_AltMemoryExc_text_seg :       	org = 0x20000730, len = 0x180
  sram_bootloader_part1_seg :         	org = 0x40044000, len = 0x4000
  otfb_signature_seg :                	org = 0xFFF00000, len = 0x8
  otfb_entry_seg :                    	org = 0xFFF00008, len = 0x78
  otfb_text_seg :                     	org = 0xFFF00080, len = 0x2B80
  otfb_FTable_FuncEntry_seg :         	org = 0xFFF02C10, len = 0xF0
}

PHDRS
{
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
  iram0_AltReset_text_phdr PT_LOAD;
  iram0_AltMemoryExc_lit_phdr PT_LOAD;
  iram0_AltMemoryExc_text_phdr PT_LOAD;
  iram0_text_phdr PT_LOAD;
  iram0_blank_480K_phdr PT_LOAD;
  sram_rsvd_256K_phdr PT_LOAD;
  sram_bootloader_part0_phdr PT_LOAD;
  sram_bootloader_part1_phdr PT_LOAD;
  otfb_signature_phdr PT_LOAD;
  otfb_entry_phdr PT_LOAD;
  otfb_text_phdr PT_LOAD;
  otfb_text_bss_phdr PT_LOAD;
  otfb_FTable_head_phdr PT_LOAD;
  otfb_FTable_FuncEntry_phdr PT_LOAD;
  otfb_PTable_phdr PT_LOAD;
  otfb_reglist_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)

/*  Memory boundary addresses:  */
_memmap_mem_dram0_start = 0x1ff80000;
_memmap_mem_dram0_end   = 0x1ffc0000;
_memmap_mem_dram1_start = 0x1ffc0000;
_memmap_mem_dram1_end   = 0x20000000;
_memmap_mem_iram0_start = 0x20000000;
_memmap_mem_iram0_end   = 0x20080000;
_memmap_mem_otfb_start = 0xfff00000;
_memmap_mem_otfb_end   = 0xfff60000;
_memmap_mem_sram_4_start = 0x40000000;
_memmap_mem_sram_4_end   = 0x80000000;

/*  Memory segment boundary addresses:  */
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
_memmap_seg_iram0_AltReset_text_start = 0x20000400;
_memmap_seg_iram0_AltReset_text_max   = 0x20000700;
_memmap_seg_iram0_AltMemoryExc_lit_start = 0x20000700;
_memmap_seg_iram0_AltMemoryExc_lit_max   = 0x20000730;
_memmap_seg_iram0_AltMemoryExc_text_start = 0x20000730;
_memmap_seg_iram0_AltMemoryExc_text_max   = 0x200008b0;
_memmap_seg_sram_bootloader_part1_start = 0x40044000;
_memmap_seg_sram_bootloader_part1_max   = 0x40048000;
_memmap_seg_otfb_signature_start = 0xfff00000;
_memmap_seg_otfb_signature_max   = 0xfff00008;
_memmap_seg_otfb_entry_start = 0xfff00008;
_memmap_seg_otfb_entry_max   = 0xfff00080;
_memmap_seg_otfb_text_start = 0xfff00080;
_memmap_seg_otfb_text_max   = 0xfff02c00;
_memmap_seg_otfb_FTable_FuncEntry_start = 0xfff02c10;
_memmap_seg_otfb_FTable_FuncEntry_max   = 0xfff02d00;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x20000000);
PROVIDE(_memmap_reset_vector = 0x20000400);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x40024444;
_memmap_cacheattr_wt_base = 0x10021111;
_memmap_cacheattr_bp_base = 0x20022222;
_memmap_cacheattr_unused_mask = 0x0FF00000;
_memmap_cacheattr_wb_trapnull = 0x42224444;
_memmap_cacheattr_wba_trapnull = 0x42224444;
_memmap_cacheattr_wbna_trapnull = 0x52225555;
_memmap_cacheattr_wt_trapnull = 0x12221111;
_memmap_cacheattr_bp_trapnull = 0x22222222;
_memmap_cacheattr_wb_strict = 0x4FF24444;
_memmap_cacheattr_wt_strict = 0x1FF21111;
_memmap_cacheattr_bp_strict = 0x2FF22222;
_memmap_cacheattr_wb_allvalid = 0x42224444;
_memmap_cacheattr_wt_allvalid = 0x12221111;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

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
    _memmap_seg_iram0_AltReset_text_end = ALIGN(0x8);
  } >iram0_AltReset_text_seg :iram0_AltReset_text_phdr

  .MemoryExceptionVector.literal : ALIGN(4)
  {
    _MemoryExceptionVector_literal_start = ABSOLUTE(.);
    *(.MemoryExceptionVector.literal)
    _MemoryExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_iram0_AltMemoryExc_lit_end = ALIGN(0x8);
  } >iram0_AltMemoryExc_lit_seg :iram0_AltMemoryExc_lit_phdr

  .MemoryExceptionVector.text : ALIGN(4)
  {
    _MemoryExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.text))
    _MemoryExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_AltMemoryExc_text_end = ALIGN(0x8);
  } >iram0_AltMemoryExc_text_seg :iram0_AltMemoryExc_text_phdr

  .mcu2dram.text : ALIGN(4)
  {
    _mcu2dram_text_start = ABSOLUTE(.);
    *(.mcu2dram.literal .mcu2dram.text)
    _mcu2dram_text_end = ABSOLUTE(.);
  } >sram_bootloader_part1_seg :sram_bootloader_part1_phdr

  .BootLoaderPart1.text : ALIGN(4)
  {
    _BootLoaderPart1_text_start = ABSOLUTE(.);
    *(.BootLoaderPart1.literal .BootLoaderPart1.text)
    _BootLoaderPart1_text_end = ABSOLUTE(.);
  } >sram_bootloader_part1_seg :sram_bootloader_part1_phdr

  .BootLoaderPart1.data : ALIGN(4)
  {
    _BootLoaderPart1_data_start = ABSOLUTE(.);
    *(.BootLoaderPart1.data)
    _BootLoaderPart1_data_end = ABSOLUTE(.);
    _memmap_seg_sram_bootloader_part1_end = ALIGN(0x8);
  } >sram_bootloader_part1_seg :sram_bootloader_part1_phdr

  .otfb_signtr : ALIGN(4)
  {
    _otfb_signtr_start = ABSOLUTE(.);
    KEEP (*(.otfb_signtr))
    _otfb_signtr_end = ABSOLUTE(.);
    _memmap_seg_otfb_signature_end = ALIGN(0x8);
  } >otfb_signature_seg :otfb_signature_phdr

  .otfb_entry : ALIGN(4)
  {
    _otfb_entry_start = ABSOLUTE(.);
    *(.otfb_entry)
    _otfb_entry_end = ABSOLUTE(.);
    _memmap_seg_otfb_entry_end = ALIGN(0x8);
  } >otfb_entry_seg :otfb_entry_phdr

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
  } >otfb_text_seg :otfb_text_phdr

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
  } >otfb_text_seg :otfb_text_phdr

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
  } >otfb_text_seg :otfb_text_phdr

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
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_otfb_text_end = ALIGN(0x8);
  } >otfb_text_seg :otfb_text_bss_phdr
  __stack = 0xfff02c00;
  _heap_sentry = 0xfff02c00;

  .otfb_FTableEntry : ALIGN(4)
  {
    _otfb_FTableEntry_start = ABSOLUTE(.);
    *(.otfb_FTableEntry)
    _otfb_FTableEntry_end = ABSOLUTE(.);
    _memmap_seg_otfb_FTable_FuncEntry_end = ALIGN(0x8);
  } >otfb_FTable_FuncEntry_seg :otfb_FTable_FuncEntry_phdr
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

