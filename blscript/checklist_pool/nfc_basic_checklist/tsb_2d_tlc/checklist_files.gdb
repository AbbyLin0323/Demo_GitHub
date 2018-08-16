############################################################################
#add basic checklist files directory
dir ..\\checklist_pool\\nfc_basic_checklist
############################################################################


############################################################################
#set chklist base address,pending
set $fileSize = (28)
set $chklist_start_addr = (0xfff04000)
############################################################################


############################################################################
#check list, add custom files here

source checklist_P_SINGLE_PLN.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_MULTI_PLN.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_PART_READ.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_SING_PLN_CCL_READ.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_CHANGE_COL_READ.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_RED_ONLY_READ.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_SSU_CS.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_SSU_UPDATE.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_RED_UPDATE.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_PU_BITMAP.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_LUN_BITMAP.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_RETRY.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_READ_STS.gdb

#set $chklist_start_addr = ($chklist_start_addr + $fileSize)
#source checklist_P_ERR_INJ_DEC_REP.gdb

set $chklist_start_addr = ($chklist_start_addr + $fileSize)
source checklist_P_TLC_COPY.gdb
############################################################################
   
