@if %RunningorNot% equ 1 (
  echo Prepare Solution
  if exist %NetDiskPath%\ENV\!TypeofProject!\WholeChip_Win (
    rd /s /q %NetDiskPath%\ENV\!TypeofProject!\WholeChip_Win
  )
  md %NetDiskPath%\ENV\!TypeofProject!\WholeChip_Win
  xcopy %WinWorkSpace% %NetDiskPath%\ENV\!TypeofProject!\WholeChip_Win /s /e /y /q
)