OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(start)

SECTIONS
{
  .text     : { *(.text.start) *(.text   .text.*   .gnu.linkonce.t.*) *(.sceStub.text.*) }
  .rodata   : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .data     : { *(.data   .data.*   .gnu.linkonce.d.*) }
  .bss      : { *(.bss    .bss.*    .gnu.linkonce.b.*) *(COMMON) }
  .last     : {  *(.pkgurl) }
  /DISCARD/ : { *(.interp) *(.dynsym) *(.dynstr) *(.hash) *(.dynamic) *(.comment) }
}
