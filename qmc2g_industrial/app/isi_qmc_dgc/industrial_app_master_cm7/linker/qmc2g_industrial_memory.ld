/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright (c) 2008-2013 Code Red Technologies Ltd,
 * Copyright 2015, 2018-2019 NXP
 * (c) NXP Semiconductors 2013-2022
 * Generated linker script file for MIMXRT1176xxxxx
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.4.1 [Build 6260] [2021-09-15] on Nov 7, 2021, 10:42:44 PM
 */

MEMORY
{
  /* Define each memory region */
  BOARD_FLASH (rx) : ORIGIN = 0x60000000, LENGTH = 0x1000000 /* 16M bytes (alias Flash) */  
  SRAM_DTC_cm7 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x40000 /* 256K bytes (alias RAM) */  
  SRAM_ITC_cm7 (rwx) : ORIGIN = 0x0, LENGTH = 0x40000 /* 256K bytes (alias RAM2) */  
  SRAM_OC1 (rwx) : ORIGIN = 0x20240000, LENGTH = 0x80000 /* 512K bytes (alias RAM3) */  
  SRAM_OC2 (rwx) : ORIGIN = 0x202c0000, LENGTH = 0x80000 /* 512K bytes (alias RAM4) */  
  SRAM_OC_ECC1 (rwx) : ORIGIN = 0x20340000, LENGTH = 0x10000 /* 64K bytes (alias RAM5) */  
  NCACHE_REGION (rwx) : ORIGIN = 0x20350000, LENGTH = 0x10000 /* 64K bytes (alias RAM6) */  
  SRAM_OC_cm7 (rwx) : ORIGIN = 0x20360000, LENGTH = 0x20000 /* 128K bytes (alias RAM7) */  
  BOARD_OCTAL_FLASH (rx) : ORIGIN = 0x30000000, LENGTH = 0x4000000 /* 64M bytes (alias Flash2) */  
  BOARD_OCTAL_RAM (rwx) : ORIGIN = 0x34000000, LENGTH = 0x2000000 /* 32M bytes (alias RAM8) */  
  OCRAM_LMEM_CODE (rwx) : ORIGIN = 0x20200000, LENGTH = 0x20000 /* 128K bytes (alias RAM9) */  
  OCRAM_LMEM_DATA (rwx) : ORIGIN = 0x20220000, LENGTH = 0x20000 /* 128K bytes (alias RAM10) */  
}

  /* Define a symbol for the top of each memory region */
  __base_BOARD_FLASH = 0x60000000  ; /* BOARD_FLASH */  
  __base_Flash = 0x60000000 ; /* Flash */  
  __top_BOARD_FLASH = 0x60000000 + 0x1000000 ; /* 16M bytes */  
  __top_Flash = 0x60000000 + 0x1000000 ; /* 16M bytes */  
  __base_SRAM_DTC_cm7 = 0x20000000  ; /* SRAM_DTC_cm7 */  
  __base_RAM = 0x20000000 ; /* RAM */  
  __top_SRAM_DTC_cm7 = 0x20000000 + 0x40000 ; /* 256K bytes */  
  __top_RAM = 0x20000000 + 0x40000 ; /* 256K bytes */  
  __base_SRAM_ITC_cm7 = 0x0  ; /* SRAM_ITC_cm7 */  
  __base_RAM2 = 0x0 ; /* RAM2 */  
  __top_SRAM_ITC_cm7 = 0x0 + 0x40000 ; /* 256K bytes */  
  __top_RAM2 = 0x0 + 0x40000 ; /* 256K bytes */  
  __base_SRAM_OC1 = 0x20240000  ; /* SRAM_OC1 */  
  __base_RAM3 = 0x20240000 ; /* RAM3 */  
  __top_SRAM_OC1 = 0x20240000 + 0x80000 ; /* 512K bytes */  
  __top_RAM3 = 0x20240000 + 0x80000 ; /* 512K bytes */  
  __base_SRAM_OC2 = 0x202c0000  ; /* SRAM_OC2 */  
  __base_RAM4 = 0x202c0000 ; /* RAM4 */  
  __top_SRAM_OC2 = 0x202c0000 + 0x80000 ; /* 512K bytes */  
  __top_RAM4 = 0x202c0000 + 0x80000 ; /* 512K bytes */  
  __base_SRAM_OC_ECC1 = 0x20340000  ; /* SRAM_OC_ECC1 */  
  __base_RAM5 = 0x20340000 ; /* RAM5 */  
  __top_SRAM_OC_ECC1 = 0x20340000 + 0x10000 ; /* 64K bytes */  
  __top_RAM5 = 0x20340000 + 0x10000 ; /* 64K bytes */  
  __base_NCACHE_REGION = 0x20350000  ; /* NCACHE_REGION */  
  __base_RAM6 = 0x20350000 ; /* RAM6 */  
  __top_NCACHE_REGION = 0x20350000 + 0x10000 ; /* 64K bytes */  
  __top_RAM6 = 0x20350000 + 0x10000 ; /* 64K bytes */  
  __base_SRAM_OC_cm7 = 0x20360000  ; /* SRAM_OC_cm7 */  
  __base_RAM7 = 0x20360000 ; /* RAM7 */  
  __top_SRAM_OC_cm7 = 0x20360000 + 0x20000 ; /* 128K bytes */  
  __top_RAM7 = 0x20360000 + 0x20000 ; /* 128K bytes */  
  __base_BOARD_OCTAL_FLASH = 0x30000000  ; /* BOARD_OCTAL_FLASH */  
  __base_Flash2 = 0x30000000 ; /* Flash2 */  
  __top_BOARD_OCTAL_FLASH = 0x30000000 + 0x4000000 ; /* 64M bytes */  
  __top_Flash2 = 0x30000000 + 0x4000000 ; /* 64M bytes */  
  __base_BOARD_OCTAL_RAM = 0x34000000  ; /* BOARD_OCTAL_RAM */  
  __base_RAM8 = 0x34000000 ; /* RAM8 */  
  __top_BOARD_OCTAL_RAM = 0x34000000 + 0x2000000 ; /* 32M bytes */  
  __top_RAM8 = 0x34000000 + 0x2000000 ; /* 32M bytes */  
  __base_OCRAM_LMEM_CODE = 0x20200000  ; /* OCRAM_LMEM_CODE */  
  __base_RAM9 = 0x20200000 ; /* RAM9 */  
  __top_OCRAM_LMEM_CODE = 0x20200000 + 0x20000 ; /* 128K bytes */  
  __top_RAM9 = 0x20200000 + 0x20000 ; /* 128K bytes */  
  __base_OCRAM_LMEM_DATA = 0x20220000  ; /* OCRAM_LMEM_DATA */  
  __base_RAM10 = 0x20220000 ; /* RAM10 */  
  __top_OCRAM_LMEM_DATA = 0x20220000 + 0x20000 ; /* 128K bytes */  
  __top_RAM10 = 0x20220000 + 0x20000 ; /* 128K bytes */  

  __VECTOR_TABLE = 0x60002000;
  __VECTOR_RAM = 0x00000000;
  __RAM_VECTOR_TABLE_SIZE_BYTES = 0x400;
  