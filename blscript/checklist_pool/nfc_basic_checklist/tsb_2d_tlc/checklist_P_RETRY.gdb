#chklist  setting+++++++++++++++++++++++++++++++++++++++++++++++++
echo \r\n
echo chklist P_RETRY init
echo \r\n

#SET_DW_VALUE Flag Value Addr Offset
#define SET_DW_VALUE
#set $arg0 |= $arg1
#set $arg2 = ($chklist_start_addr + $arg3)
#p/x *($arg2) = $arg0
#end

#DW0 - File Attr 0: basic file 1: extend file(LOCAL_FCMD)
set $bsFileType = 0
set $bsLastFile = 0
set $ulDW0Addr  = ($chklist_start_addr)
p/x *($ulDW0Addr) = (($bsLastFile << 4) | $bsFileType)

#DW1
#bsPuStart Bit0-7;bsPuEnd Bit8-15;bsLunStart Bit16-23;bsLunEnd Bit24-31
set $bsPuStart = 0
set $bsPuEnd = 8
set $bsLunStart = 0
set $bsLunEnd = 1
set $ulDW1Addr = ($chklist_start_addr+4)
set $ulDW1Value = 0
set $ulDW1Value = ($bsPuStart|($bsPuEnd<<8)|($bsLunStart<<16)|($bsLunEnd<<24))
p/x *($ulDW1Addr) = ($ulDW1Value)

#DW2
#bsBlkStart Bit0-15;bsBlkEnd Bit16-31
set $bsBlkStart = 24
set $bsBlkEnd = 26
set $ulDW2Addr = ($chklist_start_addr+4*2)
set $ulDW2Value = 0
set $ulDW2Value = ($bsBlkStart|($bsBlkEnd<<16))
p/x *($ulDW2Addr) = ($ulDW2Value)

#DW3
#bsPageStart Bit0-15;bsPageEnd Bit16-31
set $bsPageStart = 0
set $bsPageEnd = 0xFF
set $ulDW3Addr = ($chklist_start_addr+4*3)
set $ulDW3Value = 0
set $ulDW3Value = ($bsPageStart|($bsPageEnd<<16))
p/x *($ulDW3Addr) = ($ulDW3Value)

#DW4
#bsPatternId Bit0-7;bsDataCheckEn Bit8;bsSsuEn Bit9;bsCacheStsEn Bit10;bsSsu0DramEn Bit11;bsSsu1DramEn Bit12
#bsRedDramEn Bit13;bsLbaChk Bit14;bsDecFifoEn Bit15;bsTlcMode Bit16;bsSinglePln Bit17;bsRawDataRead Bit18
#bsLowPageMode Bit19;bsErrInjEn Bit20;bsDsIndex Bit21-23;bsRsv1 Bit24-31

#set pattern type
set $P_RESET = 0
set $P_READ_ID = 1 
set $P_NORMAL_SET_FEAT = 2
set $P_NORMAL_GET_FEAT = 3
set $P_PIO_SET_FEAT = 4
set $P_PIO_GET_FEAT = 5
set $P_SINGLE_PLN = 6    
set $P_MULTI_PLN = 7
set $P_PART_READ = 8      
set $P_SING_PLN_CCL_READ = 9
set $P_CHANGE_COL_READ = 10
set $P_RED_ONLY_READ = 11
set $P_SSU_CS = 12         
set $P_RED_UPDATE = 13
set $P_SSU_UPDATE = 14
set $P_PU_BITMAP = 15
set $P_LUN_BITMAP = 16   
set $P_RETRY = 17
set $P_TLC_COPY = 18
set $P_READ_STS = 19
set $P_ERR_INJ_DEC_REP = 20

set $bsPatternId = $P_RETRY
set $bsDataCheckEn = 1
set $bsSsuEn = 1
set $bsCacheStsEn = 0
set $bsSsu0DramEn = 1
set $bsSsu1DramEn = 1
set $bsRedDramEn = 1
set $bsLbaChk = 0
set $bsDecFifoEn = 0
set $bsTlcMode = 1
set $bsSinglePln = 0
set $bsRawDataRead = 0
set $bsLowPageMode = 1
set $bsErrInjEn = 0
set $bsDsIndex = 0
set $bsRsv1 = 0

set $bsDataCheckEn = ($bsDataCheckEn<<8)
set $bsSsuEn = ($bsSsuEn<<9)
set $bsCacheStsEn = ($bsCacheStsEn<<10)
set $bsSsu0DramEn = ($bsSsu0DramEn<<11)
set $bsSsu1DramEn = ($bsSsu1DramEn<<12)
set $bsRedDramEn = ($bsRedDramEn<<13)
set $bsLbaChk = ($bsLbaChk<<14)
set $bsDecFifoEn = ($bsDecFifoEn<<15)
set $bsTlcMode = ($bsTlcMode<<16)
set $bsSinglePln = ($bsSinglePln<<17) 
set $bsRawDataRead = ($bsRawDataRead<<18)
set $bsLowPageMode = ($bsLowPageMode<<19)
set $bsErrInjEn = ($bsErrInjEn<<20)
set $bsDsIndex = ($bsDsIndex<<21) 
set $bsRsv1 = ($bsRsv1<<24)
set $ulDW4Addr = ($chklist_start_addr+4*4)
set $ulDW4Value = 0
set $ulDW4Value = ($bsPatternId|$bsDataCheckEn|$bsSsuEn|$bsCacheStsEn|$bsSsu0DramEn|$bsSsu1DramEn|$bsRedDramEn|$bsLbaChk|$bsDecFifoEn|$bsTlcMode|$bsSinglePln|$bsRawDataRead|$bsLowPageMode|$bsErrInjEn|$bsDsIndex|$bsRsv1)
p/x *($ulDW4Addr) = ($ulDW4Value)

#DW5
#bsInjCwStart Bit0-7;bsInjCwLen Bit16-23;bsInjErrBitPerCw Bit24-31
set $bsInjCwStart = 0
set $bsInjCwLen = 0
set $bsInjCwLen=($bsInjCwLen<<16)
set $bsInjErrBitPerCw = 0
set $bsInjErrBitPerCw = ($bsInjErrBitPerCw<<24)
set $ulDW5Addr = ($chklist_start_addr+5*4)
set $ulDW5Value = 0
set $ulDW5Value = ($bsInjCwStart|$bsInjCwLen|$bsInjErrBitPerCw)
p/x *($ulDW5Addr) = ($ulDW5Value)

#DW6
#bsInjErrType Bit0-3;bsRsv3 Bit4-31
set $bsInjErrType = 0
set $bsRsv3 = 0
set $ulDW6Addr = ($chklist_start_addr+6*4)
set $ulDW6Value = 0
set $ulDW6Value = ($bsInjErrType|($bsRsv3<<4))
p/x *($ulDW6Addr) = ($ulDW6Value)