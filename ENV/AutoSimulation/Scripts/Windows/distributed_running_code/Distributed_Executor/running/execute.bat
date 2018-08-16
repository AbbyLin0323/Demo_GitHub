@set NetDiskPath=%1
@set ProjectPath=%2
@set ExeName=%3
@set TypeofWholeProject=%4
@set ChecklistVersion=%5
@set ChecklistFolder=%6
@set FlashType=%7

if exist %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win (
  rd /s /q %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win
)
md %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win
xcopy %NetDiskPath%\ENV\%TypeofWholeProject%\WholeChip_Win %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win /y /s /q

del /q %ProjectPath%\firmware\HAL\Flash\*.c
del /q %ProjectPath%\firmware\HAL\Flash\*.h
xcopy %ProjectPath%\firmware\HAL\Flash\%FlashType%\*.c %ProjectPath%\firmware\HAL\Flash /y /q
xcopy %ProjectPath%\firmware\HAL\Flash\%FlashType%\*.h %ProjectPath%\firmware\HAL\Flash /y /q
del /q %ProjectPath%\ENV\checklist\*.ini
xcopy %ProjectPath%\ENV\checklist\normal_release_pattern\%ChecklistFolder% %ProjectPath%\ENV\checklist /y /q
xcopy %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win\%ChecklistVersion%_%FlashType% %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win /y /q

cd %ProjectPath%\ENV\%TypeofWholeProject%\WholeChip_Win
%ExeName%