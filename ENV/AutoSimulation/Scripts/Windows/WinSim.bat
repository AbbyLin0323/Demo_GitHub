@echo Windows Simulation Start

@set WinSimLog=%AutoSimLog%\Windows

@if %RunningorNot% equ 0 (
  set FlashType=L85
  set WinWorkSpace=%WorkSpace%\ENV\SATA\WholeChip_Win
  set WinBackup=%Backup%\SATA_WholeChip_Win
  set PreDefinations=NONE_PRED
  call %WindowsScripts%\Compile\Compile.bat SATA_WholeChip
  if %L1_Fake_Switch% equ ON (
    set PreDefinations=/d L1_FAKE
    call %WindowsScripts%\Compile\Compile.bat SATA_WholeChip_l1_fake
  )
  if %L2_Fake_Switch% equ ON (
    set PreDefinations=/d L2_FAKE
    call %WindowsScripts%\Compile\Compile.bat SATA_WholeChip_l2_fake
  )

  set WinWorkSpace=%WorkSpace%\ENV\AHCI\WholeChip_Win
  set WinBackup=%Backup%\AHCI_WholeChip_Win
  set PreDefinations=NONE_PRED
  call %WindowsScripts%\Compile\Compile.bat AHCI_WholeChip
  if %L1_Fake_Switch% equ ON (
    set PreDefinations=/d L1_FAKE
    call %WindowsScripts%\Compile\Compile.bat AHCI_WholeChip_l1_fake
  )
  if %L2_Fake_Switch% equ ON (
    set PreDefinations=/d L2_FAKE
    call %WindowsScripts%\Compile\Compile.bat AHCI_WholeChip_l2_fake
  )

  set WinWorkSpace=%WorkSpace%\ENV\NVMe\WholeChip_Win
  set WinBackup=%Backup%\NVMe_WholeChip_Win
  set PreDefinations=NONE_PRED
  call %WindowsScripts%\Compile\Compile.bat NVMe_WholeChip
  if %L1_Fake_Switch% equ ON (
    set PreDefinations=/d L1_FAKE
    call %WindowsScripts%\Compile\Compile.bat NVMe_WholeChip_l1_fake
  )
  if %L2_Fake_Switch% equ ON (
    set PreDefinations=/d L2_FAKE
    call %WindowsScripts%\Compile\Compile.bat NVMe_WholeChip_l2_fake
  )
) else (
  @for %%i in (%Scripts%\Parameters\WinCompileRunParas_*.txt) do @(
    for /f "tokens=1,2,3,4,5 delims=#" %%a in (%%i) do @(
      set TypeofProject=%%a
      set LogNameforCompileandRunning=%%b
      set FlashType=%%c
      set ChecklistFolderNames=%%d
      set PreDefinations=%%e
      set WinWorkSpace=%WorkSpace%\ENV\!TypeofProject!\WholeChip_Win
      set WinBackup=%Backup%\!TypeofProject!_WholeChip_Win
      call %WindowsScripts%\Compile\Compile.bat !TypeofProject!_!LogNameforCompileandRunning!
      call %WindowsScripts%\PrepareRunning\PrepareTask.bat
    )
    call %WindowsScripts%\PrepareRunning\PrepareSolution.bat
  )
)

:notexecute