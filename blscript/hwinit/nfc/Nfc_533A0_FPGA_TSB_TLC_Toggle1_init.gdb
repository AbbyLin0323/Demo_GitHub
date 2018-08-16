#nfc setting+++++++++++++++++++++++++++++++++++++++++++++++++
echo nfc reg init
p/x $arg_reg_list_addr
p/x $arg_reg_pos
echo \r\n
set variable $arg_nfc_reg_start = $arg_reg_pos
set variable $arg_nfc_1st_reg_start = $arg_reg_pos

#fill nfc related reg entry
Delay_Entry    0

    #rNfcPgCfg                = 0xc1806008;
    SetReg_Entry    ($arg_reg_base+0x1000)    0    0xc1806008
    
    #rNfcMisc                 = 0x0fff0201;
    SetReg_Entry    ($arg_reg_base+0x1014)    0    0x0fff0201
    
    #rNfcEdoTCtrlECO          = 0x23000055;
    SetReg_Entry    ($arg_reg_base+0x1030)    0    0x23000055
    
    #rNfcPRCQDelayTime        = 0x70381201;
    SetReg_Entry    ($arg_reg_base+0x1034)    0    0x70381201
    
    #Timing related registers setting
    #pNfcPgCfg->bsDDRCfg = FALSE;
    SetReg_Entry    ($arg_reg_base+0x1000)    (~(0x1<<8))    (0x0<<8)
    #pNfcPgCfg->bsDDRHfCfg = FALSE;
    SetReg_Entry    ($arg_reg_base+0x1000)    (~(0x1<<7))    (0x0<<7)
    
    #rNfcTCtrl0      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl0;
    SetReg_Entry    ($arg_reg_base+0x1004)    0    0x538C8108
    
    #rNfcTCtrl1      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl1;
    SetReg_Entry    ($arg_reg_base+0x1008)    0    0x0010048c
    
    #rNfcTCtrl2      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl2;
    SetReg_Entry    ($arg_reg_base+0x100c)    0    0x89
    
    #rNfcModeConfig  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcModeConfig;
    SetReg_Entry    ($arg_reg_base+0x102c)    0    0x0BA900
    
    #rNfcIODelayCtrl = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcIODelayCtrl;
    SetReg_Entry    ($arg_reg_base+0x1038)    0    0x0
    
    #rNfcDDrDlyComp  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcDDrDlyComp;
    SetReg_Entry    ($arg_reg_base+0x1078)    0    0x0
    
    #rPMU(0x20)      = g_aAsyncTConfTab[ucIfType][ucMode].ulPMU20;
    SetReg_Entry    ($arg_reg_base+0x1f20)    0    0x0

#interface init/reset/pumapping/set feature/
NfcInterfaceInit_Entry

NfcReset_Entry

Delay_Entry						50000

NfcPUMapping_Entry

#/* switch to sync mode */
    #rNfcPgCfg                 = 0xc100c288;
    SetReg_Entry    ($arg_reg_base+0x1000)    0    0xc100c288
    
    #rNfcModeConfig            = 0x1FE9a900;
    SetReg_Entry    ($arg_reg_base+0x102c)    0    0x1FE9a900
    
    #rNfcTCtrl0                = 0x518D03BD;
    SetReg_Entry    ($arg_reg_base+0x1004)    0    0x518D03BD
    
    #rNfcTCtrl0                &= (~(0x7<<17));
    #rNfcTCtrl0                |= (0<<17);
    SetReg_Entry    ($arg_reg_base+0x1004)    (~(0x7<<17))    0x0
        
#set feature
NfcSetFeature_Entry    0x80    0x0  
NfcSetFeature_Entry    0x10    0x6  

#Timing related registers setting
#rNfcPgCfg                 = 0xc101c181; 0xc101c181;
#SetReg_Entry    ($arg_reg_base+0x1000)    0    0xC101C981
#enable CE hold
SetReg_Entry    ($arg_reg_base+0x1000)    0    0xe101C981

#rNfcTCtrl0                = 0x51808208;
SetReg_Entry    ($arg_reg_base+0x1004)    0    0x51808208

#rNfcTCtrl1                = 0x0010248c;
SetReg_Entry    ($arg_reg_base+0x1008)    0    0x10248c

#rNfcTCtrl2                = 0x2089;
SetReg_Entry    ($arg_reg_base+0x100c)    0    0x2089

#rNfcModeConfig            = 0x1F89a900;
SetReg_Entry    ($arg_reg_base+0x102c)    0    0x1F89a900
#disable insert cmd
#SetReg_Entry    ($arg_reg_base+0x102c)    0    0x1f89a901

#rNfcIODelayCtrl
SetReg_Entry    ($arg_reg_base+0x1038)    0    0

#rNfcDDrDlyComp
SetReg_Entry    ($arg_reg_base+0x1078)    0    0

#rPMU20	= 0x2
SetReg_Entry    ($arg_reg_base+0x1f20)    0    0x2

#rNfcNfdmaCfg |= (1<<18);
SetReg_Entry    ($arg_reg_base+0x10a0)    0		(0x1<<10)|(0x1<<2)|(0x1<<1)|(0x1<<3)

#rNfcOtfbAdsBase |= (1 << 30);
SetReg_Entry    ($arg_reg_base+0x1018)    0    0x41ff0200

#dbg signals
SetReg_Entry    ($arg_reg_base+0x0050)    0    0x3E1E00
SetReg_Entry    ($arg_reg_base+0x0054)    0    0x1E0E0
SetReg_Entry    ($arg_reg_base+0x1020)    0    0x1010
SetReg_Entry    ($arg_reg_base+0x1024)    0    0x00330004
SetReg_Entry    ($arg_reg_base+0x1028)    0    0x10331004

set $arg_nfc_1st_reg_cnt = (($arg_reg_pos-$arg_nfc_1st_reg_start)/16)
set $arg_nfc_2nd_reg_start = $arg_reg_pos

set $arg_nfc_reg_cnt = (($arg_reg_pos-$arg_nfc_reg_start)/16)
set $arg_nfc_2nd_reg_cnt = (($arg_reg_pos-$arg_nfc_2nd_reg_start)/16)
