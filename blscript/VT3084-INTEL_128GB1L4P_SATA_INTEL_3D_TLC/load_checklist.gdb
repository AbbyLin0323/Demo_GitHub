#source checklist of im_3d_tlc into memory online

source ..\checklist_pool\nfc_basic_checklist\im_3d_tlc\checklist_files.gdb
#source ..\checklist_pool\nfc_extend_checklist\im_3d_tlc\checklist_files.gdb
echo source checklist done
echo \r\n

set $value = *($chklist_load_base)
set *($chklist_load_base) = ((0x1<<16)|$value)

c