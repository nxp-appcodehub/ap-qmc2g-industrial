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
echo on

cd ../../../utils/evkmimxrt1170/

REM Check ROM connection and security status
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 1 0
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 17 0

REM Upload the ivt_flashloder into internal RAM 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_ROM% -- load-image ivt_flashloader_signed.bin

TIMEOUT 1

REM Check if can communicate with ivt_flashloader.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- get-property 1 0

REM FlexSPI can be also configured by ROM itsef based on configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 0xC0000006 word 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202004 4 0 word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000

echo off
REM Configure FlexSPI via FCB binary.
REM call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- write-memory 0x20202000 fcb_qspiflash.bin
REM call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 0x9 0x20202000
echo on

REM list all conneceted memory. External memory oprations can be performed as the FlesSPI is configured.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- list-memory

REM type "blhost" to see all available commands.

pause