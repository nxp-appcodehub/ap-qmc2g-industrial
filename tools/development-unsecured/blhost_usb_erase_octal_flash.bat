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

REM set COMPAR=-p
REM set BLHOST_CONNECT_ROM=COMx,115200
REM set BLHOST_CONNECT_FLDR=COMx,115200
echo on

:: FlexSPI 1
set FLEXSPI_INSTANCE=0xcf900001
:: FlexSPI 2
:: set FLEXSPI_INSTANCE=0xcf900002

:: For Normal Octal, eg. MX25UM51245G  */
set NOR_CONFIG1=0xc0403037
set NOR_CONFIG2=0x0

:: Configuration data address and size
set SBL_CFGDATA_ADDRESS=0x30024000
set SBL_CFGDATA_SIZE=0x6000

:: Log storage address and size
::set SBL_LOG_STORAGE_ADDRESS=0x30002000
::set SBL_LOG_STORAGE_SIZE=0x3E000

:: FWU storage address and size
::set SBL_FWU_STORAGE_ADDRESS=0x33040000
::set SBL_FWU_STORAGE_SIZE=0xFC0000

cd ..\utils\evkmimxrt1170

REM Check ROM connection and security status.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 1 0
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 17 0
REM Upload the ivt_flashloder into internal RAM. 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_ROM% -- load-image ivt_flashloader.bin

timeout 1

REM Check if can communicate with ivt_flashloader.
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- get-property 1 0

REM Configure FlexSPI FlexSPI instace
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %FLEXSPI_INSTANCE% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000
timeout 1
REM Configure FlexSPI according to the configuration words.
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202000 4 %NOR_CONFIG1% word 
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- fill-memory 0x20202004 4 %NOR_CONFIG2% word
call "%CD%"\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- configure-memory 9 0x20202000

timeout 1

REM Erase Configuration data storage
call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region %SBL_CFGDATA_ADDRESS% %SBL_CFGDATA_SIZE%

::timeout 1

:: Erase log storage
::call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region %SBL_LOG_STORAGE_ADDRESS% %SBL_LOG_STORAGE_SIZE%

::timeout 1

:: Erase FWU storage
::call "%CD%"\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_FLDR% -- flash-erase-region %SBL_FWU_STORAGE_ADDRESS% %SBL_FWU_STORAGE_SIZE%

echo Script complete!

pause