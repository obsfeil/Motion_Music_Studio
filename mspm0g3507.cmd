/******************************************************************************
 * mspm0g3507.cmd - Linker Command File for MSPM0G3507
 * 
 * Memory Map:
 * - FLASH: 128 KB (0x00000000 - 0x0001FFFF)
 * - SRAM:  32 KB  (0x20200000 - 0x20207FFF)
 *****************************************************************************/

--stack_size=2048   /* 2KB stack - increased from 512 bytes */
--heap_size=4096    /* 4KB heap - increased from 1KB */

/* Entry point */
-e _c_int00

/* Suppress warnings */
--diag_suppress=10063

MEMORY
{
    FLASH     (RX)  : origin = 0x00000000, length = 0x00020000  /* 128 KB */
    SRAM      (RWX) : origin = 0x20200000, length = 0x00008000  /* 32 KB */
    
    BCR_CONFIG (R)  : origin = 0x41C00000, length = 0x000000FF
    BSL_CONFIG (R)  : origin = 0x41C00100, length = 0x00000080
}

SECTIONS
{
    /* Interrupt vectors MUST be at 0x00000000 */
    .intvecs    : > 0x00000000
    
    /* Code sections */
    .text       : palign(8) {} > FLASH
    .const      : palign(8) {} > FLASH
    .cinit      : palign(8) {} > FLASH
    .pinit      : palign(8) {} > FLASH
    .rodata     : palign(8) {} > FLASH
    
    /* C++ support */
    .ARM.exidx  : palign(8) {} > FLASH
    .init_array : palign(8) {} > FLASH
    .binit      : palign(8) {} > FLASH
    
    /* RAM function support (run from RAM for speed) */
    .TI.ramfunc : load = FLASH, palign(8), run = SRAM, table(BINIT)
    
    /* Data sections */
    .vtable     : > SRAM
    .args       : > SRAM
    .data       : > SRAM
    .bss        : > SRAM
    .sysmem     : > SRAM
    .stack      : > SRAM (HIGH)
    
    /* Configuration sections */
    .BCRConfig  : {} > BCR_CONFIG
    .BSLConfig  : {} > BSL_CONFIG
}