@set JustGenerateLog=%1

@if %JustGenerateLog% equ 0 (
  git clone %GitPath% %SourceCode%
  if %errorlevel% equ 1 (
    exit
  )
)
@cd %SourceCode%
@git checkout %GitBranchName%
@git log -1>%GitLog%
@if %errorlevel% equ 1 (
  exit
)