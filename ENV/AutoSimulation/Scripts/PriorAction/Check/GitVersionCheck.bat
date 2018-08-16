@cd %SourceCode%
@git checkout %GitBranchName%
@git reset --hard origin/%GitBranchName%
@git pull
@git log -1 >%LatestGitInfo%
@if %errorlevel% equ 1 (
  exit
)
@PowerShell -command set _Executionpolicy "Remotesigned"
@PowerShell -command "& %PriorActionScripts%\Check\CheckGitVersion.ps1" %AutoSimLog% %LatestGitInfo%