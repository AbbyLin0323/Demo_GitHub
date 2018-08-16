::*********************************************************************
::PC lint 执行脚本
::**********************************************************************/
::::::::::::::::::::::
:: 关闭控制台显示   ::
::::::::::::::::::::::

@echo off

setlocal enabledelayedexpansion

set PclintDirectory=%2
set PclintMacro=%3
set LINT_DIR=%PclintDirectory%
set LINT_TEMPFILE_DIR=%LINT_DIR%\TempFile

::::::::::::::::::::::
:: 设定LINT工具路径 ::
::::::::::::::::::::::
set LINT_EXE_DIR=%LintExePath%
::::::::::::::::::::::
:: 设定编译器库路径 ::
::::::::::::::::::::::
set SYS_LIB_DIR=%SysLibPath%

::::::::::::::::::::::
:: 设定用户库根路径 ::
::::::::::::::::::::::
set LINTCODE=%1
set PROJECT_DIR=%WorkSpace%\firmware
set LOGPATH=%AutoSimLog%

if {%LINTCODE%} == {0} goto :l4

::::::::::::::::::::::
:: 清除临时文件     ::
::::::::::::::::::::::
del %LINT_TEMPFILE_DIR%\path.tmp
del %LINT_TEMPFILE_DIR%\path.lnt
del %LINT_TEMPFILE_DIR%\lib.lnt
del %LINT_TEMPFILE_DIR%\files.lnt

::::::::::::::::::::::
:: 获取全部库路径   ::
::::::::::::::::::::::
echo %SYS_LIB_DIR%>> %LINT_TEMPFILE_DIR%\path.tmp
dir %SYS_LIB_DIR% /ad /b /s >> %LINT_TEMPFILE_DIR%\path.tmp
echo %PROJECT_DIR%>> %LINT_TEMPFILE_DIR%\path.tmp
dir %PROJECT_DIR% /ad /b /s >> %LINT_TEMPFILE_DIR%\path.tmp

::::::::::::::::::::::
:: 删掉.svn路径     ::
::::::::::::::::::::::
cd %LINT_TEMPFILE_DIR%\
findstr /v ".svn" path.tmp > tmp.tmp
del path.tmp
rename tmp.tmp path.tmp

::::::::::::::::::::::
:: 生成lib.lnt文件 ::
::::::::::::::::::::::
@for /f "tokens=1,2,3 delims=" %%i in (%LINT_TEMPFILE_DIR%\path.tmp) do echo +libdir(%%i%%j%%k)>> %LINT_TEMPFILE_DIR%\lib.lnt
::::::::::::::::::::::
:: 生成path.lnt文件  ::
::::::::::::::::::::::
@for /f "tokens=1,2,3 delims=" %%i in (%LINT_TEMPFILE_DIR%\path.tmp) do echo -i%%i%%j%%k>> %LINT_TEMPFILE_DIR%\path.lnt

::pause
if {%LINTCODE%} == {1} goto :l1
if NOT {%LINTCODE%} == {1} goto :l2

:l1
:::::::::::::::::::::::::::::::::::::::::::::::
:: 生成files.lnt文件，包含制定目录下所有文件 ::
:::::::::::::::::::::::::::::::::::::::::::::::
echo %LINT_TEMPFILE_DIR%\path.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_DIR%\std.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_TEMPFILE_DIR%\lib.lnt>> %LINT_TEMPFILE_DIR%\files.lnt

::::::::::::::::::::::::
::    进行目录处理    ::
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
:: 执行LINT检查     ::
::::::::::::::::::::::
if %PclintMacro% equ NONE (
  %LINT_EXE_DIR%\lint-nt.exe -u %LINT_TEMPFILE_DIR%\files.lnt
) else (
  %LINT_EXE_DIR%\lint-nt.exe -d%PclintMacro%= -u %LINT_TEMPFILE_DIR%\files.lnt
)
goto :l3

:l2
:::::::::::::::::::::::::::::::::::::::::::::::
:: 生成files.lnt文件，仅包含当前文件         ::
:::::::::::::::::::::::::::::::::::::::::::::::
echo %LINT_TEMPFILE_DIR%\path.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_DIR%\std.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %LINT_TEMPFILE_DIR%\lib.lnt>> %LINT_TEMPFILE_DIR%\files.lnt
echo %1>> %LINT_TEMPFILE_DIR%\files.lnt

:l4
::::::::::::::::::::::
::执行LINT检查      ::
::::::::::::::::::::::
if %PclintMacro% equ NONE (
  %LINT_EXE_DIR%\lint-nt.exe -u %LINT_TEMPFILE_DIR%\files.lnt
) else (
  %LINT_EXE_DIR%\lint-nt.exe -d%PclintMacro%= -u %LINT_TEMPFILE_DIR%\files.lnt
)
goto :l3

:l3
::::::::::::::::::::::
:: 清除临时文件     ::
::::::::::::::::::::::
::del %LINT_DIR%\TempFile\path.tmp
::del %LINT_DIR%\TempFile\path.lnt
::del %LINT_DIR%\TempFile\lib.lnt
::del %LINT_DIR%\TempFile\files.lnt

:end