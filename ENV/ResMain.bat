@echo off
setlocal EnableDelayedExpansion
cls

set LocalPath=%cd%
set ENVPath=%cd%
set CPRstPath=%cd%\cp_rst.txt

::VS110COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\
::devenv.exe in C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\IDE
call set VSMainPath=!VS120COMNTOOLS:~0,-6!
set DevEnvExe=%VSMainPath%IDE\devenv.exe
:echo "!DevEnvPath!"

set Solution=Win_Sim.sln
set OutLog=cp_log.txt

set DstFolder=%ENVPath%\AHCI\WholeChip_Win\
set OutFile=%DstFolder%%OutLog%
set GoFlag=1
set ModStr=AH
set DstSolution=

::copy checklist
set ChecklistFolder=%ENVPath%\checklist
set RegressCheckList=%ChecklistFolder%\short_run_pattern
del /q %ChecklistFolder%\*.ini
xcopy %ChecklistFolder%\short_run_pattern  %ChecklistFolder%
:goto  :GoExit

::compile successed and then run checklist
for /l %%r in (1 1 3) do (
	
	set mod=%%r
	if !mod! equ 1 set ModStr=AHCI
	if !mod! equ 2 set ModStr=SATA
	if !mod! equ 3 set ModStr=NVME
	
	echo.
	echo.
	echo.
	echo ==========handle !ModStr! mode==========
	::set path 
	set DstFolder=%ENVPath%\!ModStr!\WholeChip_Win\
	set DstSolution=!DstFolder!%Solution%
	set OutFile=!DstFolder!%OutLog%
	set ExeFile=!DstFolder!x64\Release\Win_Sim.exe
	echo Solution="!DstSolution!"
	echo OutputLog="!OutFile!"
	echo DstFolder="!DstFolder!"
	
	if exist "!DstSolution!" (                         
		echo "Compile !ModStr! Start..."       
		
		::do compile
		if exist "!OutFile!" del /q "!OutFile!"
		"%DevEnvExe%" "!DstSolution!" /Rebuild Release /Out "!OutFile!"   
		
		::check compile result
		if exist "!OutFile!" call :ANLASIS
		
		::run exe or exit
		if !GoFlag! equ 0 (
			echo ****Please fixed warnings or errors reference to !OutFile!
			pause
			goto :GoExit
		) else (
				echo exe=!ExeFile!"
				if exist "!ExeFile!" start "!ExeFile!" /w """!ExeFile!"	
			    echo exe run finished
		)                                                 
	)
	                                                         	
) 

::clear temp files
rd /s /q %ENVPath%\WinLog
del /q %ChecklistFolder%\*.ini

echo.
echo.
echo ==========Auto run end==========
  
goto :GoExit
::check compile result module
:ANLASIS
if exist "%CPRstPath%" del /q "%CPRstPath%"
set /a line=1
for /f %%i in (!OutFile!) do set /a line+=1
::get warning/error line of the log file
set /a warn=0
set /a err=0
for /f "tokens=2 skip=1 delims=:" %%i in (!OutFile!) do (
	set Str=%%i 
	set Str1=!Str!
	call set Str2=!Str1:~1,7!
	if "!Str2!"=="warning" (
		set /a warn+=1
	) else (
		call set Str2=!Str1:~1,5!
		if "!Str2!"=="error" set /a err+=1
	)
)


if %warn% neq 0 set /a GoFlag=0
if %err% neq 0 set /a GoFlag=0

echo %warn% warning, %err% error, %str1% 
goto :eof

::copy check list module
:CPCheckList



:GoExit