%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit

cd /d %~dp0

//@copy .\bin\Double_erase\VT3084-B16A_MICRON_120GB1L2P_NVME_3D_TLC_ASIC.bin VT3084-B16A_MICRON_120GB1L2P_NVME_3D_TLC_ASIC.bin
mptconsole.exe mpt vt3533 uart .\ VT3084-B16A_MICRON_120GB1L2P_NVME_3D_TLC_ASIC.bin 1152000 115200
