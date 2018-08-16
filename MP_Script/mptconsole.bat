%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit

cd /d %~dp0

::mptconsole.exe mpt vt3533 uart E:\debug-fw\mpt\ VT3084-256G1L2P_NVME_TSB_2D_TLC_ASIC-FW-UART-MP-Solution3.bin 1152000 115200
mptconsole.exe mpt vt3533 uart .\ VT3084-MICRON_128GB1L4P_NVME_3D_TLC_ASIC.bin 1152000 115200
::mptconsole.exe mpt vt3514 uart E:\debug-fw\fw-update-vt3514\v2.0.1\nvme-vt6707c-48-addGetCmdEffect\ NVME_TSB_FOURPLN_C0_ASIC.bin BootLoader_TSB_FOURPLN_C0_ASIC_TABLE.bin preDram.gdb 1152000 115200
::mptconsole.exe mpt vt3533 uart E:\SsdFw\SSD-VT3533\Bin\bin\ VT3084-128G1L2P_NVME_TSB_2D_TLC_ASIC.bin 1152000 128000