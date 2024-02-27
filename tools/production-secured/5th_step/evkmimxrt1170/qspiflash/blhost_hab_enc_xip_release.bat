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
set PGM_OTP_FUSES=0
set GENERATE_PUF_KEYSTORE=1

echo on

:: FlexSPI 1
set FLEXSPI1_INSTANCE=0xcf900001
:: FlexSPI 2
set FLEXSPI2_INSTANCE=0xcf900002

:: For Normal Octal, eg. MX25UM51245G  */
set NOR_CONFIG1=0xc0403037
set NOR_CONFIG2=0x0

set FILE=isi_qmc_dgc_industrial_bootloader
REM CM7 image ECC
set BDFILE=imx-flexspinor-normal-signed-ecc
REM CM7 unsigned
REM set BDFILE=imx-flexspinor-normal-unsigned

set FILE_SUFFIX=srec

set P2SRECFILE=PATH_TO_YOUR_MCUXPRESSO_WORKSPACE\isi_qmc_dgc_industrial_bootloader\Release\
set P2OUTFILE="..\..\utils/evkmimxrt1170"
set P2BDFILE="..\bd_file\imx11xx"
set P2IMAGEENC="..\..\Release"

:: !! do not modify to following configurations. They are alingned with Provisioning tool !!
set KEY_SCRAMBLE_ENABLE=0
set KEK=14241821189e2d0f14f1eb861af25bc1
REM KEK otp words  = 0xc15bf21a 0x86ebf114 0x0f2d9e18 0x21182414
set BASE_ADDR=0x60000000

set OTFAD_KEY1=6def5a17e01e0f9ced272ac96dd8faba
set CTR1=0020406001030507
set START1=0x60001000
set LENGTH1=0x77000

set OTFAD_KEY2=0f9ced272ac96dd8faba6def5a17e01e
set CTR2=0103056007002040
set START2=0x60080000
set LENGTH2=0xFBF000

REM 0xC450
set KEY_SCRAMBLE=0x65129587
rem OTFAD_KEY_SCRAMBLE otp word 65129587
set KEY_SCRAMBLE_ALIGN=0x11
:: !! do not modify to following configurations. They are alingned with Provisioning tool !!

cd ../../../../utils/evkmimxrt1170

REM Process row srec with elftosb to add IVT. Sign the image.
call "%CD%"\elftosb.exe -f imx -V -d -c %P2BDFILE%\%BDFILE%.bd -o %FILE%.bin %P2SRECFILE%\%FILE%.%FILE_SUFFIX%

TIMEOUT 1

REM if OTFAD KEY SCRAMBLE is desired - Optional
if %KEY_SCRAMBLE_ENABLE%==0 (
    call "%P2IMAGEENC%"\image_enc.exe ifile=%P2OUTFILE%\%FILE%.bin ofile=%P2OUTFILE%\%FILE%_otfad.bin base_addr=%BASE_ADDR% kek=%KEK% otfad_arg=[%OTFAD_KEY1%,%CTR1%,%START1%,%LENGTH1%],[%OTFAD_KEY2%,%CTR2%,%START2%,%LENGTH2%]
) else (
    call "%P2IMAGEENC%"\image_enc.exe ifile=%P2OUTFILE%\%FILE%.bin ofile=%P2OUTFILE%\%FILE%_otfad.bin base_addr=%BASE_ADDR% kek=%KEK% otfad_arg=[%OTFAD_KEY1%,%CTR1%,%START1%,%LENGTH1%],[%OTFAD_KEY2%,%CTR2%,%START2%,%LENGTH2%] scramble=%KEY_SCRAMBLE% scramble_align=%KEY_SCRAMBLE_ALIGN%
)

DEL %FILE%.bin

TIMEOUT 1

REM Check ROM connection and security status.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 1 0
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 17 0
REM Upload the signed ivt_flashloder into internal RAM. 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_ROM% -- load-image ivt_flashloader_signed.bin

TIMEOUT 1

REM Check if can communicate with ivt_flashloader.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- get-property 1 0

REM Configre BOOT config fuses to overcome power-on boot issue with connected power stages
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x14 00000800
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x16 00000010
)

call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x14
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x16

REM enable PUF by burning PUF_ENABLE fuse.
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x6 00000040
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x6

REM burn OTFAD2 key selection fuse in order to use PUF as KEK source. Lock key selection, Force Encrypted XiP in fuses.
if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0xE 00000040 
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0xE 00000010
    REM burn OTFAD2 key selection lock fuse
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0xE 00000080 
    REM enable ecrypted xip in fuses
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x14 00000002
)

call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0xE
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x14

TIMEOUT 1

REM if OTFAD KEY SCRAMBLE is desired - Optional

REM KEY_SCRAMBLE
if %KEY_SCRAMBLE_ENABLE%==1 (
    if %PGM_OTP_FUSES%==1 ( 
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x86 65129587
    )
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x86

TIMEOUT 1

REM KEY_SCRAMBLE_ALIGN
if %KEY_SCRAMBLE_ENABLE%==1 (
    if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x87 00000011
    )
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x87

TIMEOUT 1

REM enable OTAFAD2 KEY Scrabling. THIS IS ECC FUSE WORD, MAKE SURE THAT THE DESIRED CONFIGURATION IS PROGRAMMED AT ONCE ALSO FOR OTFAD2 AND BOOT CONFIG.
REM CHECK SRM FOR MORE DETAILS
if %KEY_SCRAMBLE_ENABLE%==1 (
    if %PGM_OTP_FUSES%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-program-once 0x47 00000010
    )
)
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR%  -- efuse-read-once 0x47

TIMEOUT 1

REM Generate PUF keystore
if %GENERATE_PUF_KEYSTORE%==1 (
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- key-provisioning enroll    
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- key-provisioning set_user_key 2 kek.bin
    call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- key-provisioning read_key_store puf.keystore
)

TIMEOUT 1

REM Configure FlexSPI according to the configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %FLEXSPI2_INSTANCE% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000

TIMEOUT 1

call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 0xC0000006 word 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202004 4 0 word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000

TIMEOUT 1

REM Configure FlexSPI according to the FCB.
REM call "%CD%"\blhost -t 5000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x20202000 fcb_qspiflash.bin
REM call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 0x9 0x20202000

TIMEOUT 1

REM Erase memory
if %GENERATE_PUF_KEYSTORE%==1 (
    call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60000000 0x80000 9    
) else (
    call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60000000 0x78000 9   
)

call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60080000 0x300000 9
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x6103F000 0x80000 9

TIMEOUT 1

REM Program the FW into the external memory.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60000000 %FILE%_otfad.bin 9

TIMEOUT 1000
REM Erase the first 4KB to program the FCB and the Keyblob.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x60000000 0x1000 9

TIMEOUT 1
REM Program Key Blob into the memory.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60000000 %FILE%_otfad.bin,240 9

TIMEOUT 1

REM To program FCB generated from the configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20203000 4 0xF000000F 
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20203000 

REM Program the FCB into the memory.
REM call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60000400 fcb_qspiflash.bin

TIMEOUT 1

REM Program PUF keystore
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x60000800 puf.keystore

TIMEOUT 1

REM Configure FlexSPI FlexSPI instace
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %FLEXSPI1_INSTANCE% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000
timeout 1
REM Configure FlexSPI according to the configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %NOR_CONFIG1% word 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202004 4 %NOR_CONFIG2% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000


call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region 0x33040000 0x300000 9
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x33040000 ..\..\Release\fw_update.bin 9

echo Script complete!

pause