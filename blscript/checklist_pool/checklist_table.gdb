echo set checklist load control reg
echo \r\n

#set chklist load control reg address
set $chklist_load_base = (0xfff90000)

#0-disable; 1-auto load from flash; 2-manual load from gdb file(first load must 2)
set $bsLoadMethod = 0

#0-basic file; 1-extend file
set $bsFileType = 1

set $value = 0
set $value = (($bsFileType<<8)|($bsLoadMethod))
set *($chklist_load_base) = $value