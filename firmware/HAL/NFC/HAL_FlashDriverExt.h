/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
Filename    : HAL_FlashDriverExt.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.12
Description : this file declare interface for operation to Tensilica MCU core
Others      : HAL_FlashDriverExt.h depends on HAL_FlashDriverBasic.h.
Modify      :
20140912    Gavin    Create file
20140916    Gavin    add function declaration
20151204    abby     modify to meet VT3533
*******************************************************************************/
#ifndef __HAL_FLASHDRVIER_EXT_H__
#define __HAL_FLASHDRVIER_EXT_H__

/*------------------------------------------------------------------------------
    INCLUDING FILES
------------------------------------------------------------------------------*/
#include "HAL_FlashDriverBasic.h"
#include "HAL_NormalDSG.h"
#ifndef HOST_SATA
#include "HAL_ChainMaintain.h"
#endif

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
/*NFC interrupt acc*/
#define NF_INTACC_PENDING           0x200
#define NF_INTACC_CQPOS             0x40
#define NF_INTACC_PUMSK             0x1FF

/*  move from HAL_FlashDriverExt.c    */
#define NFC_TIMING_IFTYPE_MAX       2
#define NFC_TIMING_MODE_MAX         4


/*------------------------------------------------------------------------------
   DECLARATION OF REGISTER STRUCTURE
------------------------------------------------------------------------------*/
typedef struct _NF_HPUST_REG{
    /*DW 0 : NF Status Registers for all Channels (60-63h)  */
    U32 bsAllEmpty:1;
    U32 bsCh0AllEmpty:1;
    U32 bsCh1AllEmpty:1;
    U32 bsCh2AllEmpty:1;
    U32 bsCh3AllEmpty:1;
    U32 bsres0_0:3;
    U32 bsAnyError:1;
    U32 bsres0_1:7;
    U32 bsAllDone:1;
    U32 bsres0_2:15;
}NF_HPUST_REG;

typedef struct _NF_PPUST_REG{

    /*Physical PU status registers (70-7Fh)*/
    U32 ulEmptyBitMap;
    U32 ulNotFullBitMap;
    U32 ulIdleBitMap;
    U32 ulErrorBitMap;
}NF_PPUST_REG;

/* Logic PU status registers (80-8Fh) */
typedef struct _NF_LPUST_REG{
   U32 ulEmptyBitMap;  // Logic Pu empty bitmap register
   U32 ulNotFullBitMap;// Logic Pu not full bitmap register
   U32 ulIdleBitMap;   // Logic Pu idle bitmap register
   U32 ulErrorBitMap;  // Logic Pu error bitmap register
}NF_LPUST_REG;

// Logic Lun status registers
typedef union _NF_LLUN_BITMAP_REG
{
    U32 ulBitMap;
    struct
    {
        U32 bsLPU0:4;
        U32 bsLPU1:4;
        U32 bsLPU2:4;
        U32 bsLPU3:4;
        U32 bsLPU4:4;
        U32 bsLPU5:4;
        U32 bsLPU6:4;
        U32 bsLPU7:4;
    };
}NFC_LLUN_EMPTY_REG, NFC_LLUN_NOTFULL_REG, NFC_LLUN_IDLE_REG, NFC_LLUN_ERROR_REG;

typedef struct _NFC_LLUNST_REG_
{
    NFC_LLUN_EMPTY_REG    aEmptyReg[4];
    NFC_LLUN_NOTFULL_REG  aNotFullReg[4];
    NFC_LLUN_IDLE_REG     aIdleReg[4];
    NFC_LLUN_ERROR_REG    aErrorReg[4];
}NF_LLUNST_REG;

typedef struct _NF_LLUN_SW_BITMAP_REG_
{
    U32 aEmptyBitmap[NFC_LUN_MAX_PER_PU];
    U32 aNotFullBitmap[NFC_LUN_MAX_PER_PU];
    U32 aIdleBitmap[NFC_LUN_MAX_PER_PU];
    U32 aErrorBitmap[NFC_LUN_MAX_PER_PU];
}NF_LLUN_SW_BITMAP_REG;

typedef struct _NF_PUACC_TRIG_REG{
    U32 bsTrig:1;   // when FW write 1 to bit 0, update all PUs status to LPUST
    U32 bsRsv:31;
}NF_PUACC_TRIG_REG;

typedef union _NFC_CHAIN_NUM_REG_{
    U32 ulChainNumReg;
    struct {
        U32 bsValid:1;        //FW write 1 to set total chain number of HID to valid
                              //after HW finish total data transfer for HID, HW set to invalid internally
        U32 bsClear:1;        //FW write 1 to set corresponding HID status clear to initial
        U32 bsHID:6;
        U32 bsChainNum:13;    //total chain number
        U32 bsTagDis:1;       //Trig HID disable
        U32 bsRsv:10;
    };
}NFC_CHAIN_NUM_REG;

/*    modify to meet VT3533    */
typedef struct ASYNC_TIMING_CONFIG__
{
    U32 ulNfcTCtrl0 ;
    U32 ulNfcTCtrl1 ;
    U32 ulNfcTCtrl2 ;
    U32 ulNfcModeConfig ;
    U32 ulNfcIODelayCtrl ;
    U32 ulNfcDDrDlyComp ;
    U32 ulPMU20 ;
}ASYNC_TIMING_CONFIG;

typedef enum _NFC_LOGIC_PU_BITMAP_TYPE_
{
    LOGIC_PU_BITMAP_ERROR = 0,
    LOGIC_PU_BITMAP_IDLE,
    LOGIC_PU_BITMAP_NOTFULL,
    LOGIC_PU_BITMAP_EMPTY
}PU_BITMAP_TYPE;

/*  Logic LUN bitmap   */
typedef enum _NFC_LOGIC_LUN_BITMAP_TYPE_
{
    LOGIC_LUN_BITMAP_ERROR = 0,
    LOGIC_LUN_BITMAP_IDLE,
    LOGIC_LUN_BITMAP_NOTFULL,
    LOGIC_LUN_BITMAP_EMPTY
}LOGIC_LUN_BITMAP_TYPE;

/*------------------------------------------------------------------------------
   FUNCTION DECLARATION
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT BL_NfcInit_Substitute(void);
void MCU12_DRAM_TEXT HalNfcInterfaceInitExt(void);
void MCU12_DRAM_TEXT HalNfcFeatureInitExt(void);
void MCU12_DRAM_TEXT HAL_FlashInit(void);
void MCU12_DRAM_TEXT HAL_NfcAsyncTimingInit(U8 ucIfType, U8 ucMode);
void MCU12_DRAM_TEXT HAL_NfcSyncTimingInit(U8 ucIfType, U8 ucMode);
U32  HAL_NfcGetLogicPuBitMap(PU_BITMAP_TYPE BitMapType);
U32  HAL_NfcGetLLunStsBitMap(U8 ucPU, U8 ucBitMapType);
U32  HAL_NfcGetLLunSwBitMap(U8 ucLunInPU, U8 ucBitMapType);
void HAL_ConfigSsu0(NFCQ_ENTRY *pNFCQEntry, U16 usSsu0Addr, BOOL bOntf);
void HAL_ConfigSsu1(NFCQ_ENTRY *pNFCQEntry, U16 usSsu1Addr, BOOL bOntf);
void HAL_ConfigCacheStatus(NORMAL_DSG_ENTRY *pDSG, U32 ulCsAddr);
void HAL_NfcSetFullPageRowAddr(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr, BOOL bMultiPageMode);
U32  HAL_NfcFullBlockErase(FLASH_ADDR *pFlashAddr, BOOL bTLCMode);
U32  MCU12_DRAM_TEXT HAL_NfcFullPageWrite(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq);
U32  MCU12_DRAM_TEXT HAL_NfcPageRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq);
U32  MCU12_DRAM_TEXT HAL_NfcRedOnlyRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq);
U32  MCU12_DRAM_TEXT HAL_NfcChangeColRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq);
U8   MCU12_DRAM_TEXT HAL_NfcWaitStatusWithRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, U8 ReqType);

#ifndef HOST_SATA
void HAL_NfcInitChainCnt(void);
void HAL_NfcFinishChainCnt(U8 HID, U16 TotalChain);
void HAL_NfcHelpFinishChainCnt(U8 HID);
void HAL_NfcStopChainCnt(U8 HID);
U32  MCU12_DRAM_TEXT HAL_NfcHostPageWrite(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_PRG_REQ_DES *pWrReq);
U32  MCU12_DRAM_TEXT HAL_NfcHostPageRead(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_READ_REQ_DES *pRdReq);
#endif

#if (defined(FLASH_TLC) && !defined(FLASH_TSB_3D))
U32 MCU12_DRAM_TEXT HAL_SlcCopyToSlcExt(FLASH_ADDR *pSrcFlashAddr, FLASH_ADDR *pDesFlashAddr, NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq);
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcExt(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr, NFC_READ_REQ_DES *pSlcRdReq, NFC_PRG_REQ_DES *pTlcWrReq);
U32 MCU12_DRAM_TEXT HAL_SlcCopyToSlcInt(FLASH_ADDR *pSrcAddr, FLASH_ADDR *pDesAddr, NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq);
//#ifdef TRI_STAGE_COPY
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcInt(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr, NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq);
//#endif
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcIntOnce(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr, NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq);
#endif

U32 MCU12_DRAM_TEXT HAL_NfcSinglePlnReadSts(FLASH_ADDR *pFlashAddr);

void HAL_NfcXorInit(U32 ulXorRedunBaseAddr);

#endif//__HAL_FLASHDRVIER_EXT_H__



