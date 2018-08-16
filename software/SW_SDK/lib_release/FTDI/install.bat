@echo off
 
Set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
 
REG.exe Query %RegQry% > checkOS.txt
 
%windir%\system32\Find /i "x86" < CheckOS.txt > StringCheck.txt

REM
REM Getting an Initial Program Compatability Problem with this on Windows-7.
REM Looking into the problem with FTDI.
REM
REM CDMUninstaller.exe 0403 6010
REM CDMUninstaller.exe 0403 cff8

If %ERRORLEVEL% == 0 (
    dpinst.exe /lm /sw
) ELSE (
    dpinst-x64.exe /lm /sw
)

