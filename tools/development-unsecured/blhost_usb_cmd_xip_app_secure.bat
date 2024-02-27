:: Copyright 2024 NXP 
::
:: NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
:: in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
:: installing, activating and/or otherwise using the software, you are agreeing that you have read,
:: and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
:: the applicable license terms, then you may not retain, install, activate or otherwise use the software.

echo off
REM USB communication
set COMPAR=-u
set BLHOST_CONNECT_ROM=0x1fc9,0x013D
set BLHOST_CONNECT_FLDR=0x15A2,0x0073

REM Serial communication 
REM BLHOST_CONNECT_ROM and COMPAR arguments should be as follows (configure your COM port number and desired Baud rate):

REM set COMPAR=-u
REM set BLHOST_CONNECT_ROM=COM6,115200
REM set BLHOST_CONNECT_FLDR=COM6,115200

REM set to 1 to enable fuse programming
set PGM_OTP_FUSES=1
set PGM_PUF_KEYSTORE=1

echo on

:: FlexSPI 1
::set FLEXSPI_INSTANCE=0xcf900001
:: FlexSPI 2
 set FLEXSPI_INSTANCE=0xcf900002

cd ..\utils\evkmimxrt1170

REM Check ROM connection and security status.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 1 0
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 17 0
REM Upload the ivt_flashloder into internal RAM. 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_ROM% -- load-image ivt_flashloader.bin

TIMEOUT 1

REM Check if can communicate with ivt_flashloader.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- get-property 1 0

REM Configre BOOT config fuses to overcome power-on boot issue with connected power stages
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x14 00000800
    TIMEOUT 1
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x16 00000010
)

call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x14
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x16

EM enable PUF by burning PUF_ENABLE fuse.
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x6 00000040
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x6

REM burn OTFAD2 key selection fuse in order to use PUF as KEK source.

::C0
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0xE 00000040 
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0xE

TIMEOUT 1

REM Configure FlexSPI according to the configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %FLEXSPI_INSTANCE% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000

TIMEOUT 1

call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 0xC0000006 word 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202004 4 0 word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000


REM Generate PUF keystore
if %PGM_PUF_KEYSTORE%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- key-provisioning enroll    
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- key-provisioning read_key_store puf.keystore

    TIMEOUT 1
    
    REM Erase memory
    call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60078000 0x2000 9
    TIMEOUT 1
    REM Program PUF keystore
    call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60078000 puf.keystore
)

REM Configure FlexSPI according to the FCB.
REM call "%CD%"\blhost -t 5000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x20202000 fcb_qspiflash.bin
REM call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 0x9 0x20202000

TIMEOUT 1

REM Erase memory
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60000000 0x78000 9
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60080000 0x300000 9
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x6103F000 0x300000 9

TIMEOUT 1

REM Program the FW into the external memory on the IVT offset.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60000000 PATH_TO_YOUR_MCUXPRESSO_WORKSPACE\isi_qmc_dgc_industrial_bootloader\Debug\isi_qmc_dgc_industrial_bootloader.bin 9
REM Program the recovery image inside the backup section of QSPI
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x6103F000 ..\..\Release\fw_image.bin 9

echo Script complete!

pause