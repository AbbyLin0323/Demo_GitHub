set $bsFileType = $EXT_CHKLIST
set $bsLastFile = 0
set $bsTLun = 0
set $bsReqType = $FCMD_REQ_TYPE_READ
set $bsReqSubType = $FCMD_REQ_SUBTYPE_NORMAL
set $bsForceCCL = 0

set $bsVirBlk = 457
set $bsVirPage = 247

set $bsSecStart = 0
set $bsSecLen = 128
set $bsPlnNum = 0
set $bsShiftRdEn = 0
set $bsRdRedOnly = 0
set $bsMergeRdEn = 0
set $bsLpnBitmap = 0

################################# config FCMD end ####################################


############################### reserve FCMD element #################################
#set $bsTBRebuilding = 0
#set $bsBootUpOk = 0
#set $bsPrePgEn = 0
#set $bsTableReq = 0
#set $bsRDTEn = 0
#set $bsContainXorData = 0
#set $bsReqPtr = 0
#set $bsXorStripeId = 0
#set $bsXorEn = 0
#set $bsDecFifoEn = 0
#set $bsFCmdPri = 0
#set $ulReqStsAddr = 0
#set $ulSpareadr = 0
#set $bsPhyBlk = 0
#set $bsHostRdEn = 0
#set $bsBufID = 0
#set $bsBlkMod = $FCMD_REQ_SLC_BLK
#set $bsSecStart0 = 0
#set $bsSecLen0 = 32
#set $bsSecStart1 = 0
#set $bsSecLen1 = 0
#set $bsSecStart2 = 0
#set $bsSecLen2 = 0
#############################################################################################


############################### set value into memory start #################################
#DW0
set $ulDW0Addr  = ($chklist_start_addr)
set *($ulDW0Addr) = (($bsLastFile << 4) | $bsFileType)

#DW1
set $ulDW1Addr  = ($chklist_start_addr+4)
set $ulDW1Value = 0
set $ulDW1Value = (($bsForceCCL<<31)|($bsReqSubType<<21)|($bsReqType<<18)|$bsTLun)
set *($ulDW1Addr) = ($ulDW1Value)

#DW2/3 rsv, need clr
set $ulDW2Addr  = ($chklist_start_addr+4*2)
set $ulDW2Value = 0
set *($ulDW2Addr) = ($ulDW2Value)

set $ulDW3Addr  = ($chklist_start_addr+4*3)
set $ulDW3Value = 0
set *($ulDW3Addr) = ($ulDW3Value)

#DW4/5 - FCMD_FLASH_DESC
set $ulDW4Addr  = ($chklist_start_addr+4*4)
set $ulDW4Value = 0
set $ulDW4Value = (($bsVirPage<<20)|$bsVirBlk)
set *($ulDW4Addr) = ($ulDW4Value)

set $ulDW5Addr  = ($chklist_start_addr+4*5)
set $ulDW5Value = 0
set $ulDW5Value = (($bsLpnBitmap<<24)|($bsMergeRdEn<<23)|($bsRdRedOnly<<22)|($bsShiftRdEn<<20)|($bsPlnNum<<18)|($bsSecLen<<8)|$bsSecStart)
set *($ulDW5Addr) = ($ulDW5Value)

#DW6/7/8 - FCMD_BUF_DESC,rsv
set $ulDW6Addr  = ($chklist_start_addr+4*6)
set $ulDW6Value = 0
set *($ulDW6Addr) = ($ulDW6Value)

set $ulDW7Addr  = ($chklist_start_addr+4*7)
set $ulDW7Value = 0
set *($ulDW7Addr) = ($ulDW7Value)

set $ulDW8Addr  = ($chklist_start_addr+4*8)
set $ulDW8Value = 0
set *($ulDW8Addr) = ($ulDW8Value)

#DW9/10/11/12 - FCMD_HOST_DESC, rsv
set $ulDW9Addr  = ($chklist_start_addr+4*9)
set $ulDW9Value = 0
set *($ulDW9Addr) = ($ulDW9Value)

set $ulDW10Addr  = ($chklist_start_addr+4*10)
set $ulDW10Value = 0
set *($ulDW10Addr) = ($ulDW10Value)

set $ulDW11Addr  = ($chklist_start_addr+4*11)
set $ulDW11Value = 0
set *($ulDW11Addr) = ($ulDW11Value)

set $ulDW12Addr  = ($chklist_start_addr+4*12)
set $ulDW12Value = 0
set *($ulDW12Addr) = ($ulDW12Value)
############################### set value into memory end ###################################

