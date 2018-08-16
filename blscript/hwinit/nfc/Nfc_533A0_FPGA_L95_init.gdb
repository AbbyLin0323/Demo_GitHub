#nfc setting+++++++++++++++++++++++++++++++++++++++++++++++++
echo nfc reg init
p/x $arg_reg_list_addr
p/x $arg_reg_pos
echo \r\n
set variable $arg_nfc_reg_start = $arg_reg_pos
set variable $arg_nfc_1st_reg_start = $arg_reg_pos

#fill nfc related reg entry
Delay_Entry    0

#Page config
SetReg_Entry    ($arg_reg_base+0x1000)    0    0xD1006008

#Timing control
SetReg_Entry    ($arg_reg_base+0x1004)    0    0x538C8108
SetReg_Entry    ($arg_reg_base+0x1008)    0    0x0010248c
SetReg_Entry    ($arg_reg_base+0x100c)    0    0x89

#Red DRAM base addr
SetReg_Entry    ($arg_reg_base+0x1010)    0    0x08000000

#SSU DRAM and OTFB base addr
SetReg_Entry    ($arg_reg_base+0x101c)    0    0x09008050

#Mode config
SetReg_Entry    ($arg_reg_base+0x102c)    0    0x9a900

#EDO timing control
SetReg_Entry    ($arg_reg_base+0x1030)    0    0x0

#PIO timing control, pending on HW 
SetReg_Entry    ($arg_reg_base+0x107c)    0    0x37800000
SetReg_Entry    ($arg_reg_base+0x1080)    0    0x00f381e0
SetReg_Entry    ($arg_reg_base+0x1084)    0    0x31078400

#PMU NFCLK: 0-12.5M 1-50M 2-100M 3-200M
SetReg_Entry    ($arg_reg_base+0x1f20)    0    0x0

#interface init/reset/pumapping/set feature/
NfcInterfaceInit_Entry

NfcReset_Entry

Delay_Entry						50000

NfcPUMapping_Entry

#Driver strength
NfcSetFeature_Entry    0x80    0x1    
NfcSetFeature_Entry    0x10    0x1    

#NVDDR MODE
NfcSetFeature_Entry    0x1     0x50 

#CE hold enable in sync mode
SetReg_Entry    ($arg_reg_base+0x1000)    (~(0x1<<29))    (0x1<<29)

#DDR_CFG = 1, But DDR_HF_CFG = 0 in NVDDR mode both set to 1 in TOGGLE20
SetReg_Entry    ($arg_reg_base+0x1000)    (~(0x3<<7))     (0x2<<7) 

#DRIVE_DQS_IN_IDLE = 1 for NVDDR
#SetReg_Entry    ($arg_reg_base+0x1004)    (~(0x1<<25))   (0x1<<25) 

#RDRPH = 0
SetReg_Entry    ($arg_reg_base+0x1004)    (~(0x7<<17))    (0x0<<17)

#tCAD
SetReg_Entry    ($arg_reg_base+0x1004)    (~(0xF<<7))     (0x1<<7)

#DDR DLYCOMP config in sync mode	
SetReg_Entry    ($arg_reg_base+0x1078)    0    						0x2c2c2c2c

#PMU NFCLK: 0-12.5M 1-50M 2-100M 3-200M
SetReg_Entry    ($arg_reg_base+0x1f20)    0    						0x0
   

set $arg_nfc_1st_reg_cnt = (($arg_reg_pos-$arg_nfc_1st_reg_start)/16)
set $arg_nfc_2nd_reg_start = $arg_reg_pos

set $arg_nfc_reg_cnt = (($arg_reg_pos-$arg_nfc_reg_start)/16)
set $arg_nfc_2nd_reg_cnt = (($arg_reg_pos-$arg_nfc_2nd_reg_start)/16)
