@set PclintLog=%AutoSimLog%\Pclint
:::::::::::::::::::::
::   	Pclint
:::::::::::::::::::::
@if %PclintSwitch% equ ON (
  echo Pclint
  if exist %PclintLog% (
    rd /s /q %PclintLog%
  )
  md %PclintLog%
  if not exist %CodeCheckScripts%\Pclint\TempFile (
    md %CodeCheckScripts%\Pclint\TempFile
  )
  call %CodeCheckScripts%\Pclint\Pclint.bat %FirstLintCode% %CodeCheckScripts%\Pclint NONE

  set CheckDirRelative=adapter\L0\SATA:adapter\L1\SATA:adapter\L2\SATA:adapter\L3\SATA
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint HOST_SATA>%PclintLog%\Adapter_SATA_pclintlog.txt

  set CheckDirRelative=adapter\L0\AHCI:adapter\L1\AHCI:adapter\L2\AHCI:adapter\L3\AHCI
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint HOST_AHCI>%PclintLog%\Adapter_AHCI_pclintlog.txt

  set CheckDirRelative=adapter\L0\NVMe:adapter\L1\NVMe:adapter\L2\NVMe:adapter\L3\NVMe
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint HOST_NVME>%PclintLog%\Adapter_NVMe_pclintlog.txt

  set CheckDirRelative=algorithm\L0
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\Algorithm_L0_pclintlog.txt

  set CheckDirRelative=algorithm\L1
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\Algorithm_L1_pclintlog.txt

  set CheckDirRelative=algorithm\L2
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\Algorithm_L2_pclintlog.txt

  set CheckDirRelative=algorithm\L3
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\Algorithm_L3_pclintlog.txt

  set CheckDirRelative=HAL
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\HAL_pclintlog.txt

  set CheckDirRelative=algorithm\Misc:COM:config:MCU0_Main.c:MCU12_Main.c
  call %CodeCheckScripts%\Pclint\Pclint.bat %SecondLintCode% %CodeCheckScripts%\Pclint NONE>%PclintLog%\Others_pclintlog.txt
)

:::::::::::::::::::::
::   	Simian
:::::::::::::::::::::
@if %SimianSwitch% equ ON (
  echo Simian
  call %CodeCheckScripts%\Simian\Simian.bat
)
