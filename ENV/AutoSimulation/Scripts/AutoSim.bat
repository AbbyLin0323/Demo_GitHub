@echo Auto Simulation Start
:::::::::::::::::::::::::::
::  Defination       
:::::::::::::::::::::::::::

@setlocal enabledelayedexpansion

@set AutoSim=%cd:~0,-8%
@set PSDriver=%cd:~0,2%
@set CommonParaFile=%cd%\Parameters\CommonParas.txt

@set SourceCode=%AutoSim%\SourceCode
@set WorkSpace=%AutoSim%\WorkSpace
@set AutoSimLog=%AutoSim%\AutoSimLog
@set Backup=%AutoSim%\Backup

@set Scripts=%AutoSim%\Scripts
@set WindowsScripts=%Scripts%\Windows
@set XtmpScripts=%Scripts%\Xtmp
@set ReportScripts=%Scripts%\Report
@set PriorActionScripts=%Scripts%\PriorAction
@set CodeCheckScripts=%Scripts%\CodeCheck

@set WinowsChecklistPath=%WorkSpace%\ENV\checklist
@set FirmwarePath=%WorkSpace%\firmware

@set GitLog=%AutoSimLog%\gitlog.txt
@set LatestGitInfo=%PriorActionScripts%\Check\latestgitinfo.txt
@set AutoRunningLogStorePath=%AutoSimLog%\AutoRunningLog
@set DispathedLog=%AutoRunningLogStorePath%\DispatchedLog.txt
@set RunningOverLog=%AutoRunningLogStorePath%\RunningOverLog.txt
@set DistributedRunningTaskList=%WindowsScripts%\Running\disrunningtasklist.txt

@set FirstLintCode=3
@set SecondLintCode=1

@set Date=%date:~0,10%
@set TimeTemp=%time:~0,5%
@if "%TimeTemp:~0,1%" EQU " " (
  set Time=%TimeTemp:~1,4%
) else (
  set Time=%TimeTemp%
)
@set LogTime=%Date%_%Time%

@for /f "delims=#" %%a in (%CommonParaFile%) do @%%a

@set RunCycleNum=%ProjectNum%
@set FirstRunEnviornmentPath=0

:::::::::::::::::::::::::::
::  Prior Action      
:::::::::::::::::::::::::::
@echo CheckVersion/DeleteCloneCode
@call %PriorActionScripts%\PriorAction.bat

:::::::::::::::::::::::::::
::  Code Check      
:::::::::::::::::::::::::::
@call %CodeCheckScripts%\CodeCheck.bat

:::::::::::::::::::::::::::
::  Windows Compile
:::::::::::::::::::::::::::
@call %WindowsScripts%\WinSim.bat

:::::::::::::::::::::::::::
::  Windows running
:::::::::::::::::::::::::::
@call %WindowsScripts%\Running\Running.bat

:::::::::::::::::::::::::::
::  Xtmp Compile
:::::::::::::::::::::::::::
@call %XtmpScripts%\XtmpMake.bat

:::::::::::::::::::::::::::
::  Report 
:::::::::::::::::::::::::::
@call %ReportScripts%\Report.bat