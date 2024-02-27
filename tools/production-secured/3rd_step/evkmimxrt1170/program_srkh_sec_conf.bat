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

set P2FILE="../../../utils/evkmimxrt1170/"

REM Check ROM connection and security status
%P2FILE%\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 1 0
%P2FILE%\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_ROM% -- get-property 17 0
REM Upload the ivt_flashloder into internal RAM 
%P2FILE%\blhost -t 5242000 %COMPAR% %BLHOST_CONNECT_ROM% -- load-image %P2FILE%\ivt_flashloader.bin

TIMEOUT 1

REM Check if can communicate with ivt_flashloader.
%P2FILE%\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- get-property 1 0

TIMEOUT 1

%P2FILE%\blhost -t 50000 %COMPAR% %BLHOST_CONNECT_FLDR% -- receive-sb-file %P2FILE%\enable_hab.sb

echo Script complete!

pause