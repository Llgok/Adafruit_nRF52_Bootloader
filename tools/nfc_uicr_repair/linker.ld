OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = APP_START, LENGTH = FLASH_END - APP_START
  RAM   (rwx) : ORIGIN = RAM_START, LENGTH = RAM_END - RAM_START
}

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
    KEEP(*(.init))
    KEEP(*(.fini))
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  __etext = .;

  .data : AT (__etext)
  {
    __data_start__ = .;
    *(.data*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM

  .heap (COPY):
  {
    __HeapBase = .;
    __end__ = .;
    PROVIDE(end = .);
    KEEP(*(.heap*))
    __HeapLimit = .;
  } > RAM

  .stack_dummy (COPY):
  {
    KEEP(*(.stack*))
  } > RAM

  __StackTop = ORIGIN(RAM) + LENGTH(RAM);
  __StackLimit = __StackTop - SIZEOF(.stack_dummy);
  PROVIDE(__stack = __StackTop);

  ASSERT(__StackLimit >= __HeapLimit, "RAM overflow")
  ASSERT((__etext + SIZEOF(.data)) <= (ORIGIN(FLASH) + LENGTH(FLASH)), "FLASH overflow")
}
