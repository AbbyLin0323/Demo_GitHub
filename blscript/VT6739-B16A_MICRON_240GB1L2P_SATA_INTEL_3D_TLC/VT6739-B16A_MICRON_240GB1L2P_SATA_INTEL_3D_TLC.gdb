target remote localhost:20000
set pagination off

#patch OTFB default can't write
p/x *0x1ff8002c=0

#dump system bin to dev OTFB (offset 1K, first 1K is for MP tool)
restore ..\..\Bin\bin\VT6739-B16A_MICRON_240GB1L2P_SATA_3D_TLC_ASIC.bin binary 0xfff00400
echo restore VT6739-B16A_MICRON_240GB1L2P_SATA_3D_TLC_ASIC.bin
echo \r\n

set $pc=0xfff00408