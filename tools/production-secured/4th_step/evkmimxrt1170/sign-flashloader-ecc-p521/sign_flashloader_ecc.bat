:: Copyright 2024 NXP 
::
:: NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
:: in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
:: installing, activating and/or otherwise using the software, you are agreeing that you have read,
:: and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
:: the applicable license terms, then you may not retain, install, activate or otherwise use the software.

@echo on
title iMX RT flashloader signing script

echo Generating signed flashloader...
cd ..\..\..\..\utils\evkmimxrt1170\ || goto errorlabel
call "%CD%"\elftosb -f imx -V -c ../bd_file/imx11xx/imx-ocram-signed-ecc.bd -o ivt_flashloader_signed.bin flashloader.srec

echo Script complete!

pause
exit 0

:errorlabel
pause
exit 1
