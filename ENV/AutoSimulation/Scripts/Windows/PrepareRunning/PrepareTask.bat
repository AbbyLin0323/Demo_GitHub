@if %Compilepara2% equ x64 (
  if not exist %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType! (
    md %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType!
  )
  xcopy %WinWorkSpace%\x64\%Compilepara1% %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType! /y /s /q
) else (
  if not exist %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType! (
    md %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType!
  )
  xcopy %WinWorkSpace%\%Compilepara1% %WinWorkSpace%\!LogNameforCompileandRunning!_!FlashType! /y /s /q
)
@if %RunningorNot% equ 1 (
  echo Prepare Task
  echo %WorkSpace%#Win_Sim.exe#WholeChip_Win#!TypeofProject!#!LogNameforCompileandRunning!#!FlashType!#!ChecklistFolderNames!>>%DistributedRunningTaskList%
)