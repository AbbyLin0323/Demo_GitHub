::******************************************************************************
:: Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.             *
::                                                                             *
:: This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.         *
:: and may contain trade secrets and/or other confidential information of VIA  *
:: Technologies,Inc.                                                           *
:: This file shall not be disclosed to any third party, in whole or in part,   *
:: without prior written consent of VIA.                                       *
::                                                                             *
:: THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, *
:: WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,*
:: AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF   *
:: MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR       *
:: NON-INFRINGEMENT.                                                           *
::******************************************************************************
::Filename    : Makefile
::Version     : Ver 0.1
::Author      : Haven
::Date        : 2015.03.27
::Description : shortcut to prepare env for compile firmware
::Usage       : 
:: 1. double click start_xtensa_shell_C0.bat
:: 2. modify XTENSA_ROOT_DIR if you did not install xplorer by default
::Modify      :
::20150327    Gavin create file
::********************************************************************************
:: printf information
@echo off


:: MCU core name
@set XTENSA_CONFIG_CORE=rfviatieloop16

:: xtensa tool install directory
@set XTENSA_ROOT_DIR=C:\usr\xtensa

:: set xtensa env
@set PATH=%XTENSA_ROOT_DIR%\Xplorer-6.0.1;%PATH%
@call %XTENSA_ROOT_DIR%\XtDevTools\install\tools\RF-2014.1-win32\XtensaTools\Tools\misc\xtensaenv.bat %XTENSA_ROOT_DIR%\XtDevTools\install\builds\RF-2014.1-win32\%XTENSA_CONFIG_CORE% %XTENSA_ROOT_DIR%\XtDevTools\install\tools\RF-2014.1-win32\XtensaTools %XTENSA_ROOT_DIR%\XtDevTools\install\builds\RF-2014.1-win32\%XTENSA_CONFIG_CORE%\config %XTENSA_CONFIG_CORE% 11 1>nul 2>nul && echo 1. SET XTENSA ENV DONE : XTENSA_CONFIG_CORE=rfviatieloop16 XTENSA_ROOT_DIR=C:\usr\xtensa
@echo.

@echo 2. CHANGE DIRECTORY to %cd%
@echo.



:: CMD.exe setting
@cmd.exe /k title Shell for config build %XTENSA_CONFIG_CORE%






