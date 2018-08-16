@set HOST=SATA
@set FLASH=MICRON_3D_TLC_B16
@set MCUVER=A0
@set PLATFORM=ASIC
@set PCHECK=success
@set ROOTPATH=..\..

@copy %ROOTPATH%\Bin\bin\BootLoader_%FLASH%_%MCUVER%_%PLATFORM%.elf BootLoader.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU0_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu0.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU1_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu1.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU2_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu2.elf
@echo copy mcu0,mcu12,bootloader(elf) and firmware(bin) to local done.

xt-gdb -x VT3084-128GB1L2P_SATA_MICRON_3D_TLC_B16.gdb