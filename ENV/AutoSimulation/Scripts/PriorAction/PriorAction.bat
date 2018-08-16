@echo Create WorkSpace/AutoSimLog/Backup
@if exist %WorkSpace% (
  rd /s /q %WorkSpace%
)
@if exist %Backup% (
  rd /s /q %Backup%
)
@md %WorkSpace%
@md %Backup%

@if exist %SourceCode% (
  call %PriorActionScripts%\Check\GitVersionCheck.bat
  for /f "delims=" %%a in (%LatestGitInfo%) do @(
    if %%a EQU 1 exit
  )
  if exist %AutoSimLog% (
    rd /s /q %AutoSimLog%
  )
  md %AutoSimLog%
  call %PriorActionScripts%\Git\Git.bat 1
) else (
  if exist %AutoSimLog% (
    rd /s /q %AutoSimLog%
  )
  md %AutoSimLog%
  call %PriorActionScripts%\Git\Git.bat 0
)

@xcopy %SourceCode% %WorkSpace% /y /s /q

@if exist %DistributedRunningTaskList% (
  del /q %DistributedRunningTaskList%
)

@if exist %AutoRunningLogStorePath% (
  rd /s /q %AutoRunningLogStorePath%
)
@if %RunningorNot% equ 1 (
  md %AutoRunningLogStorePath%
)