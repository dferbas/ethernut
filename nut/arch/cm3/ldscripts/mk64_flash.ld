/*****************************************************************************
 *
 * mk64_flash.ld - linker script  for Nut/OS on Cortex-M STM32, Code in Flash
 *
 *****************************************************************************/

ENTRY(NutInit)
SEARCH_DIR(.)
/*
SRAM1_START = DEFINED(ram1_start) ? ram1_start : 0x20000000  + ram0_length;
SRAM2_START = SRAM1_START + ram1_length;

MEMORY
{
    FLASH0 (rx) : ORIGIN = 0x08000000,  LENGTH = flash0_length
    SRAM0 (rwx) : ORIGIN = 0x20000000,  LENGTH = ram0_length
    SRAM1 (rwx) : ORIGIN = SRAM1_START, LENGTH = ram1_length
    SRAM2 (rwx) : ORIGIN = SRAM2_START, LENGTH = ram2_length
}
*/

MEMORY
{
  /* Define each memory region */
  FLASH0 (rx) : ORIGIN = 0x0, LENGTH = 0x100000 /* 1M bytes (alias Flash) */  
  SRAM0 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x30000 /* 192K bytes (alias RAM) */  
  SRAM_LOWER (rwx) : ORIGIN = 0x1fff0000, LENGTH = 0x10000 /* 64K bytes (alias RAM2) */  
  FLEX_RAM (rwx) : ORIGIN = 0x14000000, LENGTH = 0x1000 /* 4K bytes (alias RAM3) */  
}



/* PROVIDE (__heap_start = _sheap); */

_ramend = ORIGIN(SRAM0) + LENGTH(SRAM0);
_rodata_start  = 0;
_rodata_end    = 0;
_rodata_load   = 0;
_ramfunc_start = 0;
_ramfunc_end   = 0;
_ramfunc_load  = 0;
_ccm_used      = 0;

SECTIONS{
    .bootloader (NOLOAD):
    {
	. = DEFINED(bootloader_offset) ? bootloader_offset : 0;
    } > FLASH0

    .text :
    {
	    . = ALIGN(4);
        _svect = .;
        KEEP(*(.isr_vector))
        _evect = .;

        /* Kinetis Flash Configuration data */
        . = 0x400 ;
        PROVIDE(__FLASH_CONFIG_START__ = .) ;
        KEEP(*(.FlashConfig))
        PROVIDE(__FLASH_CONFIG_END__ = .) ;
        ASSERT(!(__FLASH_CONFIG_START__ == __FLASH_CONFIG_END__), "Linker Flash Config Support Enabled, but no .FlashConfig section provided within application");
        /* End of Kinetis Flash Configuration data */
        
        *(.nutinit)
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t)
        *(.glue_7)
	*(.gcc_except_table)
	    . = ALIGN(4);
    } > FLASH0

    .rodata : ALIGN (4)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    } > FLASH0

    /* .ARM.exidx is sorted, so has to go in its own output section. */
    .exidx : {
        __exidx_start = .;
        *(..exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH0

    .preinit_array : {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH0

    .init_array : {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH0

    .fini_array : {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
            PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH0

    _etext = .;

    .vtable (NOLOAD):
    {
	. = ALIGN(4);
	    _vtable = .;
	    *(.vtable*)
	. = ALIGN(4);
	    _evtable = .;
    } > SRAM0

/* STACK direct after vtable, before (ro)data*/
    .stack (NOLOAD):
    {
        . = ALIGN(8);
        _stack_start = .;
        KEEP(*(.stack .stack.*));
        _stack_end = .;
    } > SRAM0

    .data : AT (_etext)
    {
        _sdata = .;
        *(.data .data.* .gnu.linkonce.d.*)
	. = ALIGN(4);
	*(.ramfunc);
	. = ALIGN(4);
        _edata = .;
    } > SRAM0

    .bss (NOLOAD):
    {
        _sbss = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM0

    .psp_stack (NOLOAD):
    {
        . = ALIGN(8);
        _pspstack_start = .;
        KEEP(*(.psp_stack .psp_stack.*));
        _pspstack_end = .;
        PROVIDE (__heap_start = .);
    } > SRAM0
/*
    .heap (NOLOAD):
    {
        . = ALIGN(4);
        _sheap = .;
        . = _ramend - .;
        _eheap = .;
    } > SRAM0
*/
    __end_rom = _etext +  SIZEOF(.data );

end = .;
/*
    DISCARD :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
*/
    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }

    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */

    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
