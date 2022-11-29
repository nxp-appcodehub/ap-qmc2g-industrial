/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright (c) 2008-2013 Code Red Technologies Ltd,
 * Copyright 2015, 2018-2019 NXP
 * (c) NXP Semiconductors 2013-2022
 * Generated linker script file for MIMXRT1176xxxxx
 * Created from linkscript.ldt by FMCreateLinkLibraries
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.4.1 [Build 6260] [2021-09-15] on Nov 7, 2021, 10:42:44 PM
 */

INCLUDE "qmc2g_industrial_library.ld"
INCLUDE "qmc2g_industrial_memory.ld"

ENTRY(ResetISR)

SECTIONS
{
     .text_Flash2 : ALIGN(4)
    {
       FILL(0xff)
        *(.text_Flash2) /* for compatibility with previous releases */
        *(.text_BOARD_OCTAL_FLASH) /* for compatibility with previous releases */
        *(.text.$Flash2)
        *(.text.$BOARD_OCTAL_FLASH)
        *(.text_Flash2.*) /* for compatibility with previous releases */
        *(.text_BOARD_OCTAL_FLASH.*) /* for compatibility with previous releases */
        *(.text.$Flash2.*)
        *(.text.$BOARD_OCTAL_FLASH.*)
        *(.rodata.$Flash2)
        *(.rodata.$BOARD_OCTAL_FLASH)
        *(.rodata.$Flash2.*)
        *(.rodata.$BOARD_OCTAL_FLASH.*)            } > BOARD_OCTAL_FLASH

    /* Image Vector Table and Boot Data for booting from external flash */
    .boot_hdr : ALIGN(4)
    {
        FILL(0xff)
        . = 0x400 ;
        __boot_hdr_start__ = ABSOLUTE(.) ;
        KEEP(*(.boot_hdr.conf))
        . = 0x1000 ;
        __boot_hdr_ivt_loadaddr__  =  ABSOLUTE(.) ;
        KEEP(*(.boot_hdr.ivt))
        . = 0x1020 ;
        __boot_hdr_boot_data_loadaddr__ = ABSOLUTE(.) ;
        KEEP(*(.boot_hdr.boot_data))
        . = 0x1030 ;
        __boot_hdr_dcd_loadaddr__ = ABSOLUTE(.) ;
        KEEP(*(.boot_hdr.dcd_data))
        __boot_hdr_end__ = ABSOLUTE(.) ;
        . = 0x2000 ;
    } >BOARD_FLASH


    /* MAIN TEXT SECTION */
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        LONG(LOADADDR(.data_RAM3));
        LONG(    ADDR(.data_RAM3));
        LONG(  SIZEOF(.data_RAM3));
        LONG(LOADADDR(.data_RAM4));
        LONG(    ADDR(.data_RAM4));
        LONG(  SIZEOF(.data_RAM4));
        LONG(LOADADDR(.data_RAM5));
        LONG(    ADDR(.data_RAM5));
        LONG(  SIZEOF(.data_RAM5));
        LONG(LOADADDR(.data_RAM6));
        LONG(    ADDR(.data_RAM6));
        LONG(  SIZEOF(.data_RAM6));
        LONG(LOADADDR(.data_RAM7));
        LONG(    ADDR(.data_RAM7));
        LONG(  SIZEOF(.data_RAM7));
        LONG(LOADADDR(.data_RAM8));
        LONG(    ADDR(.data_RAM8));
        LONG(  SIZEOF(.data_RAM8));
        LONG(LOADADDR(.data_RAM9));
        LONG(    ADDR(.data_RAM9));
        LONG(  SIZEOF(.data_RAM9));
        LONG(LOADADDR(.data_RAM10));
        LONG(    ADDR(.data_RAM10));
        LONG(  SIZEOF(.data_RAM10));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM3));
        LONG(  SIZEOF(.bss_RAM3));
        LONG(    ADDR(.bss_RAM4));
        LONG(  SIZEOF(.bss_RAM4));
        LONG(    ADDR(.bss_RAM5));
        LONG(  SIZEOF(.bss_RAM5));
        LONG(    ADDR(.bss_RAM6));
        LONG(  SIZEOF(.bss_RAM6));
        LONG(    ADDR(.bss_RAM7));
        LONG(  SIZEOF(.bss_RAM7));
        LONG(    ADDR(.bss_RAM8));
        LONG(  SIZEOF(.bss_RAM8));
        LONG(    ADDR(.bss_RAM9));
        LONG(  SIZEOF(.bss_RAM9));
        LONG(    ADDR(.bss_RAM10));
        LONG(  SIZEOF(.bss_RAM10));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */

        *(.after_vectors*)

    } > BOARD_FLASH

    .text : ALIGN(4)
    {
		*(EXCLUDE_FILE(
						*freertos_kernel*
						*interrupts.o*
						*fsl_clock.o*
		) *.text*)       KEEP(*freertos*/tasks.o(.rodata*)) /* FreeRTOS Debug Config */
       *(EXCLUDE_FILE(
						*freertos_kernel*
						*interrupts.o*
						*fsl_clock.o*
		) *.rodata*)
		*(EXCLUDE_FILE(
						*freertos_kernel*
						*interrupts.o*
						*fsl_clock.o*
		) *.constdata*)

    } > BOARD_FLASH
    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > BOARD_FLASH

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > BOARD_FLASH
 
    _etext = .;
        
    /* DATA section for SRAM_ITC_cm7 */

    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
		. = 0x400 ;
		__vtable_ram__ = ABSOLUTE(.) ;
        KEEP(*(.data.vtable))
		FILL(0xff)
        PROVIDE(__start_data_RAM2 = .) ;
        PROVIDE(__start_data_SRAM_ITC_cm7 = .) ;
        *(.ramfunc.$RAM2)
        *(.ramfunc.$SRAM_ITC_cm7)
	    KEEP(*(CodeQuickAccess))
	    *(RamFunction)
	    /* default project */
	    *freertos_kernel*(*.text*)
	    *interrupts.o*(*.text*)
	    *fsl_clock.o*(*.text*)
        *(.data.$RAM2)
        *(.data.$SRAM_ITC_cm7)
        *(.data.$RAM2.*)
        *(.data.$SRAM_ITC_cm7.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
        PROVIDE(__end_data_SRAM_ITC_cm7 = .) ;
     } > SRAM_ITC_cm7 AT>BOARD_FLASH

    /* DATA section for SRAM_OC1 */

    .data_RAM3 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM3 = .) ;
        PROVIDE(__start_data_SRAM_OC1 = .) ;
        *(.ramfunc.$RAM3)
        *(.ramfunc.$SRAM_OC1)
		/* default project */
		*freertos_kernel*(*.constdata)
		*freertos_kernel*(*.rodata)		
		*freertos_kernel*(*.constdata*)
		*freertos_kernel*(*.rodata*)
        *(.data.$RAM3)
        *(.data.$SRAM_OC1)
        *(.data.$RAM3.*)
        *(.data.$SRAM_OC1.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
        PROVIDE(__end_data_SRAM_OC1 = .) ;
     } > SRAM_OC1 AT>BOARD_FLASH

    /* DATA section for SRAM_OC2 */

    .data_RAM4 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM4 = .) ;
        PROVIDE(__start_data_SRAM_OC2 = .) ;
        *(.ramfunc.$RAM4)
        *(.ramfunc.$SRAM_OC2)
        *(.data.$RAM4)
        *(.data.$SRAM_OC2)
        *(.data.$RAM4.*)
        *(.data.$SRAM_OC2.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM4 = .) ;
        PROVIDE(__end_data_SRAM_OC2 = .) ;
     } > SRAM_OC2 AT>BOARD_FLASH

    /* DATA section for SRAM_OC_ECC1 */

    .data_RAM5 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM5 = .) ;
        PROVIDE(__start_data_SRAM_OC_ECC1 = .) ;
        *(.ramfunc.$RAM5)
        *(.ramfunc.$SRAM_OC_ECC1)
        *(.data.$RAM5)
        *(.data.$SRAM_OC_ECC1)
        *(.data.$RAM5.*)
        *(.data.$SRAM_OC_ECC1.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM5 = .) ;
        PROVIDE(__end_data_SRAM_OC_ECC1 = .) ;
     } > SRAM_OC_ECC1 AT>BOARD_FLASH

    /* DATA section for NCACHE_REGION */

    .data_RAM6 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM6 = .) ;
        PROVIDE(__start_data_NCACHE_REGION = .) ;
        *(.ramfunc.$RAM6)
        *(.ramfunc.$NCACHE_REGION)
        *(.data.$RAM6)
        *(.data.$NCACHE_REGION)
        *(.data.$RAM6.*)
        *(.data.$NCACHE_REGION.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM6 = .) ;
        PROVIDE(__end_data_NCACHE_REGION = .) ;
     } > NCACHE_REGION AT>BOARD_FLASH

    /* DATA section for SRAM_OC_cm7 */

    .data_RAM7 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM7 = .) ;
        PROVIDE(__start_data_SRAM_OC_cm7 = .) ;
        *(.ramfunc.$RAM7)
        *(.ramfunc.$SRAM_OC_cm7)
        *(.data.$RAM7)
        *(.data.$SRAM_OC_cm7)
        *(.data.$RAM7.*)
        *(.data.$SRAM_OC_cm7.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM7 = .) ;
        PROVIDE(__end_data_SRAM_OC_cm7 = .) ;
     } > SRAM_OC_cm7 AT>BOARD_FLASH

    /* DATA section for BOARD_OCTAL_RAM */

    .data_RAM8 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM8 = .) ;
        PROVIDE(__start_data_BOARD_OCTAL_RAM = .) ;
        *(.ramfunc.$RAM8)
        *(.ramfunc.$BOARD_OCTAL_RAM)
        *(.data.$RAM8)
        *(.data.$BOARD_OCTAL_RAM)
        *(.data.$RAM8.*)
        *(.data.$BOARD_OCTAL_RAM.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM8 = .) ;
        PROVIDE(__end_data_BOARD_OCTAL_RAM = .) ;
     } > BOARD_OCTAL_RAM AT>BOARD_FLASH

    /* DATA section for OCRAM_LMEM_CODE */

    .data_RAM9 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM9 = .) ;
        PROVIDE(__start_data_OCRAM_LMEM_CODE = .) ;
        *(.ramfunc.$RAM9)
        *(.ramfunc.$OCRAM_LMEM_CODE)
        *(.data.$RAM9)
        *(.data.$OCRAM_LMEM_CODE)
        *(.data.$RAM9.*)
        *(.data.$OCRAM_LMEM_CODE.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM9 = .) ;
        PROVIDE(__end_data_OCRAM_LMEM_CODE = .) ;
     } > OCRAM_LMEM_CODE AT>BOARD_FLASH

    /* DATA section for OCRAM_LMEM_DATA */

    .data_RAM10 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM10 = .) ;
        PROVIDE(__start_data_OCRAM_LMEM_DATA = .) ;
        *(.ramfunc.$RAM10)
        *(.ramfunc.$OCRAM_LMEM_DATA)
        *(.data.$RAM10)
        *(.data.$OCRAM_LMEM_DATA)
        *(.data.$RAM10.*)
        *(.data.$OCRAM_LMEM_DATA.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM10 = .) ;
        PROVIDE(__end_data_OCRAM_LMEM_DATA = .) ;
     } > OCRAM_LMEM_DATA AT>BOARD_FLASH

    /* MAIN DATA SECTION */
    .uninit_RESERVED (NOLOAD) : ALIGN(4)
    {
        _start_uninit_RESERVED = .;
        KEEP(*(.bss.$RESERVED*))
       . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > SRAM_DTC_cm7 AT> SRAM_DTC_cm7

    /* Main DATA section (SRAM_DTC_cm7) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       PROVIDE(__start_data_RAM = .) ;
       PROVIDE(__start_data_SRAM_DTC_cm7 = .) ;
       *(vtable)
       *(.ramfunc*)
       KEEP(*(CodeQuickAccess))
       KEEP(*(DataQuickAccess))
       *(RamFunction)
		*interrupts.o*(*.rodata*)
		*interrupts.o*(*.constdata*)
		*fsl_clock.o*(*.rodata*)
		*fsl_clock.o*(*.constdata*)
        *(.data)
        *(.data*)
        *(.data.)
        *(.data.*)       . = ALIGN(4) ;
       _edata = . ;
       PROVIDE(__end_data_RAM = .) ;
       PROVIDE(__end_data_SRAM_DTC_cm7 = .) ;
    } > SRAM_DTC_cm7 AT>BOARD_FLASH

    /* BSS section for SRAM_ITC_cm7 */
    .bss_RAM2 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       PROVIDE(__start_bss_SRAM_ITC_cm7 = .) ;
       *(.bss.$RAM2)
       *(.bss.$SRAM_ITC_cm7)
       *(.bss.$RAM2.*)
       *(.bss.$SRAM_ITC_cm7.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
       PROVIDE(__end_bss_SRAM_ITC_cm7 = .) ;
    } > SRAM_ITC_cm7 AT> SRAM_ITC_cm7

    /* BSS section for SRAM_OC1 */
    .bss_RAM3 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM3 = .) ;
       PROVIDE(__start_bss_SRAM_OC1 = .) ;
		/* default project */
		*freertos_kernel*(*.bss*)
       *(.bss.$RAM3)
       *(.bss.$SRAM_OC1)
       *(.bss.$RAM3.*)
       *(.bss.$SRAM_OC1.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM3 = .) ;
       PROVIDE(__end_bss_SRAM_OC1 = .) ;
    } > SRAM_OC1 AT> SRAM_OC1

    /* BSS section for SRAM_OC2 */
    .bss_RAM4 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM4 = .) ;
       PROVIDE(__start_bss_SRAM_OC2 = .) ;
       *(.bss.$RAM4)
       *(.bss.$SRAM_OC2)
       *(.bss.$RAM4.*)
       *(.bss.$SRAM_OC2.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM4 = .) ;
       PROVIDE(__end_bss_SRAM_OC2 = .) ;
    } > SRAM_OC2 AT> SRAM_OC2

    /* BSS section for SRAM_OC_ECC1 */
    .bss_RAM5 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM5 = .) ;
       PROVIDE(__start_bss_SRAM_OC_ECC1 = .) ;
       *(.bss.$RAM5)
       *(.bss.$SRAM_OC_ECC1)
       *(.bss.$RAM5.*)
       *(.bss.$SRAM_OC_ECC1.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM5 = .) ;
       PROVIDE(__end_bss_SRAM_OC_ECC1 = .) ;
    } > SRAM_OC_ECC1 AT> SRAM_OC_ECC1

    /* BSS section for NCACHE_REGION */
    .bss_RAM6 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM6 = .) ;
       PROVIDE(__start_bss_NCACHE_REGION = .) ;
       *(.bss.$RAM6)
       *(.bss.$NCACHE_REGION)
       *(.bss.$RAM6.*)
       *(.bss.$NCACHE_REGION.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM6 = .) ;
       PROVIDE(__end_bss_NCACHE_REGION = .) ;
    } > NCACHE_REGION AT> NCACHE_REGION

    /* BSS section for SRAM_OC_cm7 */
    .bss_RAM7 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM7 = .) ;
       PROVIDE(__start_bss_SRAM_OC_cm7 = .) ;
       *(.bss.$RAM7)
       *(.bss.$SRAM_OC_cm7)
       *(.bss.$RAM7.*)
       *(.bss.$SRAM_OC_cm7.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM7 = .) ;
       PROVIDE(__end_bss_SRAM_OC_cm7 = .) ;
    } > SRAM_OC_cm7 AT> SRAM_OC_cm7

    /* BSS section for BOARD_OCTAL_RAM */
    .bss_RAM8 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM8 = .) ;
       PROVIDE(__start_bss_BOARD_OCTAL_RAM = .) ;
       *(.bss.$RAM8)
       *(.bss.$BOARD_OCTAL_RAM)
       *(.bss.$RAM8.*)
       *(.bss.$BOARD_OCTAL_RAM.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM8 = .) ;
       PROVIDE(__end_bss_BOARD_OCTAL_RAM = .) ;
    } > BOARD_OCTAL_RAM AT> BOARD_OCTAL_RAM

    /* BSS section for OCRAM_LMEM_CODE */
    .bss_RAM9 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM9 = .) ;
       PROVIDE(__start_bss_OCRAM_LMEM_CODE = .) ;
       *(.bss.$RAM9)
       *(.bss.$OCRAM_LMEM_CODE)
       *(.bss.$RAM9.*)
       *(.bss.$OCRAM_LMEM_CODE.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM9 = .) ;
       PROVIDE(__end_bss_OCRAM_LMEM_CODE = .) ;
    } > OCRAM_LMEM_CODE AT> OCRAM_LMEM_CODE

    /* BSS section for OCRAM_LMEM_DATA */
    .bss_RAM10 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM10 = .) ;
       PROVIDE(__start_bss_OCRAM_LMEM_DATA = .) ;
       *(.bss.$RAM10)
       *(.bss.$OCRAM_LMEM_DATA)
       *(.bss.$RAM10.*)
       *(.bss.$OCRAM_LMEM_DATA.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM10 = .) ;
       PROVIDE(__end_bss_OCRAM_LMEM_DATA = .) ;
    } > OCRAM_LMEM_DATA AT> OCRAM_LMEM_DATA

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        PROVIDE(__start_bss_RAM = .) ;
        PROVIDE(__start_bss_SRAM_DTC_cm7 = .) ;
       *(EXCLUDE_FILE(
						*freertos_kernel*
		) *.bss*)        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(__end_bss_RAM = .) ;
        PROVIDE(__end_bss_SRAM_DTC_cm7 = .) ;
        PROVIDE(end = .);
    } > SRAM_DTC_cm7 AT> SRAM_DTC_cm7

    /* NOINIT section for SRAM_ITC_cm7 */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM2 = .) ;
       PROVIDE(__start_noinit_SRAM_ITC_cm7 = .) ;
       *(.noinit.$RAM2)
       *(.noinit.$SRAM_ITC_cm7)
       *(.noinit.$RAM2.*)
       *(.noinit.$SRAM_ITC_cm7.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM2 = .) ;
       PROVIDE(__end_noinit_SRAM_ITC_cm7 = .) ;
    } > SRAM_ITC_cm7 AT> SRAM_ITC_cm7

    /* NOINIT section for SRAM_OC1 */
    .noinit_RAM3 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM3 = .) ;
       PROVIDE(__start_noinit_SRAM_OC1 = .) ;
       *(.noinit.$RAM3)
       *(.noinit.$SRAM_OC1)
       *(.noinit.$RAM3.*)
       *(.noinit.$SRAM_OC1.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM3 = .) ;
       PROVIDE(__end_noinit_SRAM_OC1 = .) ;
    } > SRAM_OC1 AT> SRAM_OC1

    /* NOINIT section for SRAM_OC2 */
    .noinit_RAM4 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM4 = .) ;
       PROVIDE(__start_noinit_SRAM_OC2 = .) ;
       *(.noinit.$RAM4)
       *(.noinit.$SRAM_OC2)
       *(.noinit.$RAM4.*)
       *(.noinit.$SRAM_OC2.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM4 = .) ;
       PROVIDE(__end_noinit_SRAM_OC2 = .) ;
    } > SRAM_OC2 AT> SRAM_OC2

    /* NOINIT section for SRAM_OC_ECC1 */
    .noinit_RAM5 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM5 = .) ;
       PROVIDE(__start_noinit_SRAM_OC_ECC1 = .) ;
       *(.noinit.$RAM5)
       *(.noinit.$SRAM_OC_ECC1)
       *(.noinit.$RAM5.*)
       *(.noinit.$SRAM_OC_ECC1.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM5 = .) ;
       PROVIDE(__end_noinit_SRAM_OC_ECC1 = .) ;
    } > SRAM_OC_ECC1 AT> SRAM_OC_ECC1

    /* NOINIT section for NCACHE_REGION */
    .noinit_RAM6 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM6 = .) ;
       PROVIDE(__start_noinit_NCACHE_REGION = .) ;
       *(.noinit.$RAM6)
       *(.noinit.$NCACHE_REGION)
       *(.noinit.$RAM6.*)
       *(.noinit.$NCACHE_REGION.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM6 = .) ;
       PROVIDE(__end_noinit_NCACHE_REGION = .) ;
    } > NCACHE_REGION AT> NCACHE_REGION

    /* NOINIT section for SRAM_OC_cm7 */
    .noinit_RAM7 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM7 = .) ;
       PROVIDE(__start_noinit_SRAM_OC_cm7 = .) ;
       *(.noinit.$RAM7)
       *(.noinit.$SRAM_OC_cm7)
       *(.noinit.$RAM7.*)
       *(.noinit.$SRAM_OC_cm7.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM7 = .) ;
       PROVIDE(__end_noinit_SRAM_OC_cm7 = .) ;
    } > SRAM_OC_cm7 AT> SRAM_OC_cm7

    /* NOINIT section for BOARD_OCTAL_RAM */
    .noinit_RAM8 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM8 = .) ;
       PROVIDE(__start_noinit_BOARD_OCTAL_RAM = .) ;
       *(.noinit.$RAM8)
       *(.noinit.$BOARD_OCTAL_RAM)
       *(.noinit.$RAM8.*)
       *(.noinit.$BOARD_OCTAL_RAM.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM8 = .) ;
       PROVIDE(__end_noinit_BOARD_OCTAL_RAM = .) ;
    } > BOARD_OCTAL_RAM AT> BOARD_OCTAL_RAM

    /* NOINIT section for OCRAM_LMEM_CODE */
    .noinit_RAM9 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM9 = .) ;
       PROVIDE(__start_noinit_OCRAM_LMEM_CODE = .) ;
       *(.noinit.$RAM9)
       *(.noinit.$OCRAM_LMEM_CODE)
       *(.noinit.$RAM9.*)
       *(.noinit.$OCRAM_LMEM_CODE.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM9 = .) ;
       PROVIDE(__end_noinit_OCRAM_LMEM_CODE = .) ;
    } > OCRAM_LMEM_CODE AT> OCRAM_LMEM_CODE

    /* NOINIT section for OCRAM_LMEM_DATA */
    .noinit_RAM10 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM10 = .) ;
       PROVIDE(__start_noinit_OCRAM_LMEM_DATA = .) ;
       *(.noinit.$RAM10)
       *(.noinit.$OCRAM_LMEM_DATA)
       *(.noinit.$RAM10.*)
       *(.noinit.$OCRAM_LMEM_DATA.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM10 = .) ;
       PROVIDE(__end_noinit_OCRAM_LMEM_DATA = .) ;
    } > OCRAM_LMEM_DATA AT> OCRAM_LMEM_DATA

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        PROVIDE(__start_noinit_RAM = .) ;
        PROVIDE(__start_noinit_SRAM_DTC_cm7 = .) ;
        *(.noinit*)
         . = ALIGN(4) ;
        _end_noinit = .;
       PROVIDE(__end_noinit_RAM = .) ;
       PROVIDE(__end_noinit_SRAM_DTC_cm7 = .) ;        
    } > SRAM_DTC_cm7 AT> SRAM_DTC_cm7

    /* Reserve and place Heap within memory map */
    _HeapSize = 0x20000;
    .heap :  ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM_OC_cm7

     _StackSize = 0x10000;
     /* Reserve space in memory for Stack */
    .heap2stackfill  :
    {
        . += _StackSize;
    } > SRAM_DTC_cm7
    /* Locate actual Stack in memory map */
    .stack ORIGIN(SRAM_DTC_cm7) + LENGTH(SRAM_DTC_cm7) - _StackSize - 0:  ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > SRAM_DTC_cm7

    /* Provide basic symbols giving location and size of main text
     * block, including initial values of RW data sections. Note that
     * these will need extending to give a complete picture with
     * complex images (e.g multiple Flash banks).
     */
    _image_start = LOADADDR(.text);
    _image_end = LOADADDR(.data) + SIZEOF(.data);
    _image_size = _image_end - _image_start;
    /* Provide symbols for MIMXRT1170 parts for boot header generation code
     * to set image to be plain load image or XIP.
     * Config : Plain load image = false
     */
    _boot_loadaddr = ORIGIN(BOARD_FLASH);
    _boot_size = LENGTH(BOARD_FLASH);
}