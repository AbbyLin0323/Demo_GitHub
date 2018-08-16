::*********************************************************************
::PC lint ִ�нű�
::**********************************************************************/
::::::::::::::::::::::
:: �رտ���̨��ʾ   ::
::::::::::::::::::::::

@echo off

setlocal enabledelayedexpansion

set PclintDirectory=%2
set PclintMacro=%3
set LINT_DIR=%PclintDirectory%
set LINT_TEMPFILE_DIR=%LINT_DIR%\TempFile

::::::::::::::::::::::
:: �趨LINT����·�� ::
::::::::::::::::::::::
set LINT_EXE_DIR=%LintExePath%
::::::::::::::::::::::
:: �趨��������·�� ::
::::::::::::::::::::::
set SYS_LIB_DIR=%SysLibPath%

::::::::::::::::::::::
:: �趨�û����·�� ::
::::::::::::::::::::::
set LINTCODE=%1
set PROJECT_DIR=%WorkSpace%\firmware
set LOGPATH=%AutoSimLog%

if {%LINTCODE%} == {0} goto :l4

::::::::::::::::::::::
:: �����ʱ�ļ�     ::
::::::::::::::::::::::
del %LINT_TEMPFILE_DIR%\path.tmp
del %LINT_TEMPFILE_DIR%\path.lnt
del %LINT_TEMPFILE_DIR%\lib.lnt
del %LINT_TEMPFILE_DIR%\files.lnt

::::::::::::::::::::::
:: ��ȡȫ����·��   ::
::::::::::::::::::::::
echo %SYS_LIB_DIR%>> %LINT_TEMPFILE_DIR%\path.tmp
dir %SYS_LIB_DIR% /ad /b /s >> %LINT_TEMPFILE_DIR%\path.tmp
echo %PROJECT_DIR%>> %LINT_TEMPFILE_DIR%\path.tmp
dir %PROJECT_DIR% /ad /b /s >> %LINT_TEMPFILE_DIR%\path.tmp

::::::::::::::::::::::
:: ɾ��.svn·��     ::
::::::::::::::::::::::
cd %LINT_TEMPFILE_DIR%\
findstr /v ".svn" path.tmp > tmp.tmp
del path.tmp
rename tmp.tmp path.tmp

::::::::::::::::::::::
:: ����lib.lnt�ļ� ::
::::::::::::::::::::::
@for /f "tokens=1,2,3 delims=" %%i in (%LINT_TEMPFILE_DIR%\path.tmp) do echo +libdir(%%i%%j%%k)>> %LINT_TEMPFILE_DIR%\lib.lnt
::::::::::::::::::::::
:: ����path.lnt�ļ�  ::
::::::::::::::::::::::
@for /f "tokens=1,2,3 delims=" %%i in (%LINT_TEMPFILE_DIR%\path.tmp) do echo -i%%i%%j%%k>> %LINT_TEMPFILE_DIR%\path.lnt

::pause
if {%LINTCODE%} == {1} goto :l1
if NOT {%LINTCODE%} == {1} goto :l2

:l1
:::::::::::::::::::::::::::::::::::::::::::::::
:: ����files.lnt�ļ��������ƶ�Ŀ¼�������ļ� ::
:::::::::::::::::::::::::::::::::::::::::::::::
echo %LINT_TEMPFILE_DIR%\path.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_DIR%\std.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_TEMPFILE_DIR%\lib.lnt>> %LINT_TEMPFILE_DIR%\files.lnt

::::::::::::::::::::::::
::    ����Ŀ¼����    ::
::::::::::::::::::::::::
@set pclintfiles=%CheckDirRelative%
@echo %CheckDirRelative%>%LOGPATH%\pclintpath.txt
@set numsignal=0
@set numfiles=0
:label1
@if not "%pclintfiles%"=="" (
@set strsignal=%pclintfiles:~0,1%
@if "%strsignal%"==":" (
@set /a numsignal+=1
)
@set pclintfiles=%pclintfiles:~1%
@goto label1
)
@set /a numfiles=%numsignal%+1
:label2
@if not "%numfiles%"=="0" (
@for /f "tokens=%numfiles% delims=:" %%a in (%LOGPATH%\pclintpath.txt) do (
  if "%%a" neq "" (
    set CurrentDir=%%a
    if !CurrentDir:~-2! equ .c (
      echo %PROJECT_DIR%\!CurrentDir! >> %LINT_TEMPFILE_DIR%\files.lnt
    ) else (
      dir  %PROJECT_DIR%\!CurrentDir!\*.c /b /s >> %LINT_TEMPFILE_DIR%\files.lnt
    )
  )
)
@set /a numfiles=%numfiles%-1
@goto label2
)

::::::::::::::::::::::
:: ִ��LINT���     ::
::::::::::::::::::::::
if %PclintMacro% equ NONE (
  %LINT_EXE_DIR%\lint-nt.exe -u %LINT_TEMPFILE_DIR%\files.lnt
) else (
  %LINT_EXE_DIR%\lint-nt.exe -d%PclintMacro%= -u %LINT_TEMPFILE_DIR%\files.lnt
)
goto :l3

:l2
:::::::::::::::::::::::::::::::::::::::::::::::
:: ����files.lnt�ļ�����������ǰ�ļ�         ::
:::::::::::::::::::::::::::::::::::::::::::::::
echo %LINT_TEMPFILE_DIR%\path.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_DIR%\std.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_TEMPFILE_DIR%\lib.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %1>> %LINT_TEMPFILE_DIR%\files.lnt

:l4
::::::::::::::::::::::
::ִ��LINT���      ::
::::::::::::::::::::::
if %PclintMacro% equ NONE (
  %LINT_EXE_DIR%\lint-nt.exe -u %LINT_TEMPFILE_DIR%\files.lnt
) else (
  %LINT_EXE_DIR%\lint-nt.exe -d%PclintMacro%= -u %LINT_TEMPFILE_DIR%\files.lnt
)
goto :l3

:l3
::::::::::::::::::::::
:: �����ʱ�ļ�     ::
::::::::::::::::::::::
::del %LINT_DIR%\TempFile\path.tmp
::del %LINT_DIR%\TempFile\path.lnt
::del %LINT_DIR%\TempFile\lib.lnt
::del %LINT_DIR%\TempFile\files.lnt

:end