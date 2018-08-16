@set CompileLogPrefix=%1

@if not exist %WinSimLog% (
  md %WinSimLog%
)
@if not exist %WinBackup% (
  md %WinBackup%
)

@if %FirstRunEnviornmentPath% equ 0 (
  call %SetEnviormentVariablesBatPath%
  set FirstRunEnviornmentPath=1
)

@if !PreDefinations! NEQ NONE_PRED (
  PowerShell -command Set _executionPolicy "Remotesigned"
  PowerShell -command "& %WindowsScripts%\Compile\AddPreDefination.ps1" %WinWorkSpace% %WinBackup% !FlashType! %WorkSpace% !PreDefinations!
)

@%PSDriver%
@cd %WinWorkSpace%
@msbuild Win_Sim.vcxproj /t:rebuild /p:Configuration=%Compilepara1%;Platform=%Compilepara2% >%WinSimLog%\%CompileLogPrefix%_compilelog.txt