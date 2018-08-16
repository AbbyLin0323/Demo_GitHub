::********************************************************************************
::* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
::*                                                                              *
::* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
::* and may contain trade secrets and/or other confidential information of VIA   *
::* Technologies,Inc.                                                            *
::* This file shall not be disclosed to any third party, in whole or in part,    *
::* without prior written consent of VIA.                                        *
::*                                                                              *
::* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
::* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
::* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
::* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
::* NON-INFRINGEMENT.                                                            *
::********************************************************************************
::Filename    : release_source.bat
::Version     : Ver 0.1
::Author      : Gavin
::Date        : 2015.04.02
::Description : This batch script is used for packaging source code.
::Usage       : release_source.bat SOURCE_DIR TARGET_DIR MODE [SDK]
::Modify      :
::20150402    Gavin create file
::********************************************************************************

:: check input param
@echo off
if not exist "%1" goto USAGE
if not exist "%2" goto USAGE
if -%3-==-SATA- goto PK_START
if -%3-==-NVME- goto PK_START
if -%3-==-AHCI- goto PK_START
goto USAGE

:PK_START
@echo begin packaging source code...

@echo off
set SRC=%1
set DST=%2
set MD=%3

xcopy /E /I %SRC%\bootloader %DST%\bootloader
xcopy /E /I %SRC%\BootScript %DST%\BootScript
xcopy /E /I %SRC%\ENV\LSP\VT3514_C0_BOOTLOADER_LSP %DST%\ENV\LSP\VT3514_C0_BOOTLOADER_LSP
xcopy /E /I %SRC%\ENV\LSP\VT3514_C0_FW_MCU0_LSP %DST%\ENV\LSP\VT3514_C0_FW_MCU0_LSP
xcopy /E /I %SRC%\ENV\LSP\VT3514_C0_FW_MCU12_LSP %DST%\ENV\LSP\VT3514_C0_FW_MCU12_LSP
xcopy /E /I %SRC%\firmware %DST%\firmware
if -%4-==-SDK- goto ROM_BIN
xcopy /E /I %SRC%\rom\LSP\VT3514_C0_ROM_LSP %DST%\rom\LSP\VT3514_C0_ROM_LSP
xcopy /E /I %SRC%\rom\VT3514_C0_ROM %DST%\rom\VT3514_C0_ROM
:ROM_BIN
xcopy /E /I %SRC%\rom\ReleaseBin %DST%\rom\ReleaseBin
xcopy /E /I %SRC%\software %DST%\software
xcopy /E /I %SRC%\tool\TraceLogParser %DST%\tool\TraceLogParser
xcopy /E /I %SRC%\tool\TraceLogDecoder %DST%\tool\TraceLogDecoder
echo F | xcopy /E %SRC%\tool\fw_bin_dump.exe %DST%\tool\fw_bin_dump.exe
echo F | xcopy /E %SRC%\tool\start_xtensa_shell_C0.bat %DST%\tool\start_xtensa_shell_C0.bat
xcopy /E /I %SRC%\unit_test %DST%\unit_test
echo F | xcopy /E %SRC%\config.mk %DST%\config.mk
echo F | xcopy /E %SRC%\Makefile %DST%\Makefile

echo copy done. begin deleting unused files
@echo off

goto %MD%_MD

:SATA_MD
echo F | xcopy /E %SRC%\ENV\OptionRom\OptionROM_VT3514_AHCI.rom %DST%\ENV\OptionRom\OptionROM_VT3514_AHCI.rom
rd /S /Q %DST%\firmware\adapter\L0\AHCI
rd /S /Q %DST%\firmware\adapter\L0\NVMe
rd /S /Q %DST%\firmware\adapter\L1\AHCI
rd /S /Q %DST%\firmware\adapter\L1\NVMe
rd /S /Q %DST%\firmware\adapter\L2\AHCI
rd /S /Q %DST%\firmware\adapter\L2\NVMe
rd /S /Q %DST%\firmware\adapter\L3\AHCI
rd /S /Q %DST%\firmware\adapter\L3\NVMe

rd /S /Q %DST%\firmware\HAL\ChainMaintain
rd /S /Q %DST%\firmware\HAL\HCT
rd /S /Q %DST%\firmware\HAL\HSG
rd /S /Q %DST%\firmware\HAL\HwDebug
rd /S /Q %DST%\firmware\HAL\SGE

goto PK_DONE

:NVME_MD
echo F | xcopy /E %SRC%\ENV\OptionRom\OptionROM_VT3514_NVME.rom %DST%\ENV\OptionRom\OptionROM_VT3514_NVME.rom

rd /S /Q %DST%\firmware\algorithm\L0\ATALib
rd /S /Q %DST%\firmware\adapter\L0\AHCI
rd /S /Q %DST%\firmware\adapter\L0\SATA
rd /S /Q %DST%\firmware\adapter\L1\AHCI
rd /S /Q %DST%\firmware\adapter\L1\SATA
rd /S /Q %DST%\firmware\adapter\L2\AHCI
rd /S /Q %DST%\firmware\adapter\L2\SATA
rd /S /Q %DST%\firmware\adapter\L3\AHCI
rd /S /Q %DST%\firmware\adapter\L3\SATA

rd /S /Q %DST%\firmware\HAL\SDC

goto PK_DONE

:AHCI_MD
echo F | xcopy /E %SRC%\ENV\OptionRom\OptionROM_VT3514_AHCI.rom %DST%\ENV\OptionRom\OptionROM_VT3514_AHCI.rom

rd /S /Q %DST%\firmware\adapter\L0\SATA
rd /S /Q %DST%\firmware\adapter\L0\NVMe
rd /S /Q %DST%\firmware\adapter\L1\SATA
rd /S /Q %DST%\firmware\adapter\L1\NVMe
rd /S /Q %DST%\firmware\adapter\L2\SATA
rd /S /Q %DST%\firmware\adapter\L2\NVMe
rd /S /Q %DST%\firmware\adapter\L3\SATA
rd /S /Q %DST%\firmware\adapter\L3\NVMe

rd /S /Q %DST%\firmware\HAL\SDC

:PK_DONE
@echo package full source done

if not -%4-==-SDK- goto END
@echo begin preparing SDK code
rd /S /Q %DST%\firmware\adapter\L2
rd /S /Q %DST%\firmware\adapter\L3
rd /S /Q %DST%\firmware\algorithm\L2
rd /S /Q %DST%\firmware\algorithm\L3
del /F /Q %DST%\firmware\algorithm\L1\L1_Buffer.c
del /F /Q %DST%\firmware\algorithm\L1\L1_Cache.c
del /F /Q %DST%\firmware\algorithm\L1\L1_CacheStatus.c
del /F /Q %DST%\firmware\algorithm\L1\L1_Debug.c
del /F /Q %DST%\firmware\algorithm\L1\L1_Interface.c
del /F /Q %DST%\firmware\algorithm\L1\L1_RdPreFetch.c
del /F /Q %DST%\firmware\algorithm\L1\L1_Schedule.c
del /F /Q %DST%\firmware\algorithm\L1\L1_SpecialSCMD.c
del /F /Q %DST%\firmware\algorithm\L1\L1_SubCmdProc.c
del /F /Q %DST%\firmware\algorithm\L1\L1_Trim.c
del /F /Q %DST%\firmware\algorithm\L1\Makefile*
echo F | xcopy %SRC%\firmware\algorithm\L1\Makefile.Ramdisk %DST%\firmware\algorithm\L1\Makefile

goto END

:USAGE
@echo Usage:
@echo "%0 SOURCE_DIR TARGET_DIR [SATA | NVME | AHCI] [SDK]"

:END
pause
