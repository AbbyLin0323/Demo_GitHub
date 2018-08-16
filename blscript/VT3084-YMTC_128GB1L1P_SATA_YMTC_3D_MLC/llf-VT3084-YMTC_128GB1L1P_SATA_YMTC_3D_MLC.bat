@set HOST=SATA
@set FLASH=YMTC_3D_MLC
@set MCUVER=A0
@set PLATFORM=ASIC
@set PCHECK=success
@set ROOTPATH=..\..

@copy %ROOTPATH%\Bin\bin\BootLoader_%FLASH%_%MCUVER%_%PLATFORM%.elf BootLoader.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU0_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu0.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU1_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu1.elf
@copy %ROOTPATH%\Bin\bin\%HOST%_MCU2_%FLASH%_%MCUVER%_%PLATFORM%.elf WholeChip_Mcu2.elf
@echo copy mcu0,mcu12,bootloader(elf) and firmware(bin) to local done.

xt-gdb -x VT3084-YMTC_128GB1L1P_SATA_YMTC_3D_MLC.gdb