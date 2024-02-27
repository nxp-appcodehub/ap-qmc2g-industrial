:: Copyright 2024 NXP 
::
:: NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
:: in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
:: installing, activating and/or otherwise using the software, you are agreeing that you have read,
:: and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
:: the applicable license terms, then you may not retain, install, activate or otherwise use the software.

@echo on
title iMX RT HAB key generation script

echo Running hab4_pki_tree script to generate keys...
cd ..\..\..\..\utils\evkmimxrt1170\cst\keys || goto errorlabel
call "%CD%"\hab4_pki_tree.bat

TIMEOUT 2

echo Running srktool to generate SRK table and fuse values...
cd ..\mingw32\bin	|| goto errorlabel
call "%CD%\"srktool -h 4 -t ../../keys/SRK_1_2_3_4_table.bin -e ../../keys/SRK_1_2_3_4_fuse.bin -d sha256 -c ../../crts/SRK1_sha256_secp521r1_v3_ca_crt.der,../../crts/SRK2_sha256_secp521r1_v3_ca_crt.der,../../crts/SRK3_sha256_secp521r1_v3_ca_crt.der,../../crts/SRK4_sha256_secp521r1_v3_ca_crt.der -f 1

TIMEOUT 2

echo Modifying enable_hab.bd file for hash of generated keys..
cd ..\..\ || goto errorlabel
call "%CD%"\generate_bd.exe

TIMEOUT 2

echo Generating enable_hab.sb file for programming fuses..
cd ..\	|| goto errorlabel
call "%CD%"\elftosb -f kinetis -V -c ../bd_file/imx10xx/enable_hab.bd -o enable_hab.sb

echo Script complete!
pause
exit 0

:errorlabel
echo Error in script execution!
pause
exit 1

