@if %RunningorNot% equ 1 (
  echo Prepare Code Checklist
  call %WindowsScripts%\Running\PrepareCodeChecklist.bat
  cd %WindowsScripts%\Running
  echo Windows running
  DisController.exe %ServerAddresses% %DistributedRunningTaskList% %DispathedLog% %RunningOverLog%
)
