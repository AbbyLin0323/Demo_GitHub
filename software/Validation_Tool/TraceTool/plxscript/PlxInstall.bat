@echo off

REM ********************************************************************
REM * This is a sample batch file to demonstrate manually installing
REM * or removing the PLX PCI/PCIe Service driver.  It utilizes the
REM * Microsoft SC.EXE utility to access the Service Control Manager.
REM * This should work in Windows 2000 & higher.
REM *
REM * Refer to the Usage for additional details.
REM ********************************************************************



REM Verify Admin priviledges
REM  Use FsUtil.exe (returns 1=error 0=ok) to test privileges
FSUTIL >NUL
IF ERRORLEVEL 1 goto _Error_NoAdmin

cd /d %~dp0

REM Clear any existing variables
SET _b64Bit=
SET _PLX_SVC_DIR=
SET _PLX_PRES_STATE_STOPPED=
SET DRIVER_BASE=%~dp0%\Driver_PlxSvc\chk

REM Determine if 32-bit or 64-bit Windows
SET _b64Bit=0
IF EXIST %SystemRoot%\SysWOW64 SET _b64Bit=1


REM Verify command-line
If /i "%1" == "Preserve" goto _Preserve
If /i "%1" == "Install"  goto _Install
If /i "%1" == "Remove"   goto _Remove
If /i "%1" == "Restore"  goto _Restore
goto _ShowUsage



:_Install
echo.
echo Install the PLX service driver
echo ======================================

REM Verify Service is not installed already (1060=Service not installed)
SC QUERYEX PlxSvc > NUL
IF NOT ERRORLEVEL 1060 goto _Error_SvcInstalledAlready

REM Verify driver doesn't already exist
IF EXIST %systemRoot%\System32\Drivers\PlxSvc.sys goto _Error_DriverExists

REM Verify correct version of PlxSvs.sys exists

IF %_b64Bit% == 0 SET _PLX_SVC_DIR=%DRIVER_BASE%\i386
IF %_b64Bit% == 1 SET _PLX_SVC_DIR=%DRIVER_BASE%\amd64
IF NOT EXIST %_PLX_SVC_DIR%\PlxSvc.sys goto _Error_DriverNotFound

echo - Copy %_PLX_SVC_DIR%\PlxSvc.sys --^> %systemRoot%\System32\Drivers...
Copy %_PLX_SVC_DIR%\PlxSvc.sys %systemRoot%\System32\Drivers > NUL

echo - Request SCM to create the service...
SC CREATE PlxSvc binPath= System32\Drivers\PlxSvc.sys type= kernel ^
          start= auto error= ignore DisplayName= "PLX PCI/PCIe Service Driver" > NUL
IF ERRORLEVEL 1073 goto _Error_SvcInstalledAlready
IF ERRORLEVEL 1 goto _Error_SCM

echo - Add the Common Buffer Size option to registry...
REG ADD "HKLM\System\CurrentControlSet\services\PlxSvc" ^
    /v CommonBufferSize /t REG_DWORD /d 8192 > NUL
IF ERRORLEVEL 1 goto _Error_REG

echo - Start the service...
SC START PlxSvc >NUL
IF ERRORLEVEL 1056 goto _Error_SvcAlreadyRunning
IF ERRORLEVEL 1 goto _Error_SCM

echo.
echo   -- PLX PCI/PCIe Service driver installed successfully --
goto _Exit



:_Remove
echo.
echo Remove the PLX service driver
echo ======================================

REM Verify Service is installed already (1060=Service not installed)
SC QUERYEX PlxSvc > NUL
IF ERRORLEVEL 1060 goto _Error_SvcNotInstalled

echo - Stop the service...
SC STOP PlxSvc >NUL
IF ERRORLEVEL 1062 ECHO    - SCM reports service already stopped

echo - Request SCM to delete the service...
SC DELETE PlxSvc > NUL
IF ERRORLEVEL 1060 goto _Error_SvcNotInstalled
IF ERRORLEVEL 1 goto _Error_SCM

echo - Delete %systemRoot%\System32\Drivers\PlxSvc.sys...
IF EXIST %systemRoot%\System32\Drivers\PlxSvc.sys Del %systemRoot%\System32\Drivers\PlxSvc.sys > NUL

echo.
echo   -- PLX PCI/PCIe Service driver removed successfully --
goto _Exit



:_Preserve
echo.
echo Preserve existing PLX service driver
echo ========================================

REM Verify Service is installed already (1060=Service not installed)
SC QUERYEX PlxSvc > NUL
IF ERRORLEVEL 1060 echo The SCM reports PLX Service not installed, nothing to preserve
IF ERRORLEVEL 1060 goto _Exit

REM Delete existing state file if exists
IF EXIST .\_Pres_PlxState.bat Del .\_Pres_PlxState.bat > NUL

echo - Stop the service...
SC STOP PlxSvc >NUL
IF ERRORLEVEL 1062 ECHO    - SCM reports service already stopped, saving state
IF ERRORLEVEL 1062 ECHO SET _PLX_PRES_STATE_STOPPED=1>> .\_Pres_PlxState.bat

echo - Backup registry settings...
REG SAVE "HKLM\System\CurrentControlSet\services\PlxSvc" .\_Pres_PlxReg.hiv /y > NUL
IF ERRORLEVEL 1 goto _Error_REG

echo - Request SCM to delete the service...
SC DELETE PlxSvc > NUL
IF ERRORLEVEL 1060 goto _Error_SvcNotInstalled
IF ERRORLEVEL 1 goto _Error_SCM

echo - Backup ^& Delete %systemRoot%\System32\Drivers\PlxSvc.sys...
Copy %systemRoot%\System32\Drivers\PlxSvc.sys .\_Pres_PlxSvc.sys > NUL
Del %systemRoot%\System32\Drivers\PlxSvc.sys > NUL

echo.
echo   -- PLX PCI/PCIe Service driver preserved ^& removed successfully --
goto _Exit



:_Restore
echo.
echo Restore previously preserved PLX service driver
echo =================================================

REM Verify Service is not installed already (1060=Service not installed)
SC QUERYEX PlxSvc > NUL
IF NOT ERRORLEVEL 1060 goto _Error_SvcInstalledAlready

REM Verify driver doesn't already exist
IF EXIST %systemRoot%\System32\Drivers\PlxSvc.sys goto _Error_DriverExists

REM Verify preserved backup of PlxSvs.sys exists
IF NOT EXIST .\_Pres_PlxSvc.sys goto _Error_PreservedDriverNotFound

REM Get previous driver stopped state
IF EXIST .\_Pres_PlxState.bat SET _PLX_PRES_STATE_STOPPED=1

echo - Copy backup of PlxSvc.sys --^> %systemRoot%\System32\Drivers...
Copy .\_Pres_PlxSvc.sys %systemRoot%\System32\Drivers\PlxSvc.sys > NUL

echo - Request SCM to create the service...
SC CREATE PlxSvc binPath= System32\Drivers\PlxSvc.sys type= kernel ^
          start= auto error= ignore DisplayName= "PLX PCI/PCIe Service Driver" > NUL
IF ERRORLEVEL 1073 goto _Error_SvcInstalledAlready
IF ERRORLEVEL 1 goto _Error_SCM

IF NOT EXIST .\_Pres_PlxReg.hiv goto _Skip_RegRestore
echo - Restore registry settings...
REG RESTORE "HKLM\System\CurrentControlSet\services\PlxSvc" .\_Pres_PlxReg.hiv > NUL
IF ERRORLEVEL 1 goto _Error_REG
:_Skip_RegRestore

echo - Start the service...
IF "%_PLX_PRES_STATE_STOPPED%" == "1" echo    - Preserved service was in STOPPED state, skipping START...
IF "%_PLX_PRES_STATE_STOPPED%" == "1" goto _End_StartService
SC START PlxSvc >NUL
IF ERRORLEVEL 1056 goto _Error_SvcAlreadyRunning
IF ERRORLEVEL 1 goto _Error_SCM

:_End_StartService
IF EXIST .\_Pres_PlxState.bat Del .\_Pres_PlxState.bat > NUL
Del .\_Pres_PlxReg.hiv > NUL
Del .\_Pres_PlxSvc.sys > NUL

echo.
echo   -- PLX PCI/PCIe Service restored successfully --
goto _Exit



REM *******************************************************
REM * Error Handlers
REM *******************************************************
:_Error_NoAdmin
echo ERROR: This script must be executed with Administrator priviledges.
goto _Exit


:_Error_DriverExists
echo ERROR: PlxSvc.sys already exists in %systemRoot%\System32\Drivers
echo        Please delete this file and re-run the scipt.
goto _Exit


:_Error_DriverNotFound
echo ERROR: PlxSvc.sys not found in %_PLX_SVC_DIR%
goto _Exit


:_Error_PreservedDriverNotFound
echo ERROR: _Pres_PlxSvc.sys not found in current dir (%CD%)
goto _Exit


:_Error_SvcInstalledAlready
echo ERROR: The SCM reports the PLX Service is already installed
goto _Exit


:_Error_SvcNotInstalled
echo ERROR: The SCM reports the PLX Service is not installed
goto _Exit


:_Error_SvcAlreadyRunning
echo ERROR: The SCM reports the PLX Service is already running
goto _Exit


:_Error_SCM
echo ERROR: The SCM reported error code %ERRORLEVEL%
goto _Exit


:_Error_REG
echo ERROR: The Registry operation resulted in error code %ERRORLEVEL%
goto _Exit


:_Exit
SET _b64Bit=
SET _PLX_SVC_DIR=
SET _PLX_PRES_STATE_STOPPED=
