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
Description :
*******************************************************************************/
#ifndef __XORUT_COMMON_H__
#define __XORUT_COMMON_H__

#include "BaseDef.h"
#include "HAL_XOR.h"
#include "HAL_HostInterface.h"
#include "XorUt_Config.h"

#if defined(COSIM)
#define XORUT_ASSERT( _x_ )\
    do{\
        if ( (_x_) == FALSE )\
        {\
            g_pCosimTracerErrorReporter->bsXorUtAssertError = TRUE;\
            while(1); \
        }\
    }while(0)
#elif (defined(FPGA) || defined(ASIC))
#define XORUT_ASSERT( _x_ )\
    do{\
        if ( (_x_) == FALSE )\
        {\
            DBG_Printf("XorUt: Assert Error!\n");\
            while(1); \
        }\
    }while(0)
#elif defined(SIM)
#define XORUT_ASSERT    ASSERT
#else 
#error "Running environment type must be defined!"
#endif

#define SEC_SIZE_DW          (SEC_SIZE / sizeof(U32))
#define PLN_CNT_PER_ROW (g_ucPuCount * g_ucLunPerPu * XORUT_PLN_PER_LUN)

typedef enum _XOR_UNIT_TEST_STATE
{
    XOR_UT_INIT,
    XOR_UT_IS_NEED_ERASE_BLOCK,
    XOR_UT_ERASE_BLOCK,
    XOR_UT_PROGRAM_PAGE,
    XOR_UT_READ_PAGE,
    XOR_UT_XOR_CALCULATE,
    XOR_UT_LOAD_STORE,
    XOR_UT_WAIT_DONE,
    XOR_UT_WAIT_NFC_READ_DONE,
    XOR_UT_TLC_STORE,
    XOR_UT_STATE_ASSERT
}XOR_UT_STATE;

typedef enum _XOR_UNIT_TEST_LOG_TYPE
{
    XOR_UT_DATA_CHECK_ERROR,
    XOR_UT_NFC_CMD_ERROR,
    XOR_UT_SWITCH_CASE_ERROR,
    XOR_UT_GENERAL_ERROR,
    XOR_UT_ALL_TEST_TASK_DONE,
    XOR_UT_CONFIG_CHECK_OK,
    XOR_UT_RECOVER_READ_DATA_OK,
    XOR_UT_RECORD_CUR_TEST_PATTERN_ID,
    XOR_UT_RECORD_PASSED_TEST_PATTERN_ID,
    XOR_UT_RECORD_XORE_ID,
    XOR_UT_LOG_TYPE_ASSERT
}XOR_UT_LOG_TYPE;

// Control information of NFC protect flow.
typedef struct _XOR_NFC_PROTECT_CONTROL
{
    FLASH_ADDR tPreEraseBlkFlashAddr;
    FLASH_ADDR tCurrentFlashAddr;
    U32 ulXorPageSize;
    XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable;
    U16 usStartXorPageNumInTotal;
    U8 ucStatus;
    U8 ucXoreId;
    U8 ucCurXorPageNumInXore;
}XOR_NFC_PROTECT_CTRL;

// Control information of TLC NFC protect flow.
typedef struct _XOR_TLC_NFC_PROTECT_CONTROL
{
    FLASH_ADDR tPreEraseBlkFlashAddr;
    FLASH_ADDR tCurrentFlashAddr;
    U32 ulXorPageSize;
    XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable;
    U16 usStartXorPageNumInTotal;
    U8 ucStatus;
    U8 ucXoreId;
    U8 ucCurProgramPlaneNumInRow;
    U8 ucCurTlcProgramIndex;
    U8 ucXorParityPartNum;
}XOR_TLC_NFC_PROTECT_CTRL;

// Control information of NFC recover flow.
typedef struct _XOR_NFC_RECOVER_CONTROL
{
    U32 ulXorPageSize;
    XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable;
    U16 usStartXorPageNumInTotal;
    U16 usBrokenXorPageNumInTotal;
    U8 ucStatus;
    U8 ucXoreId;
    U8 ucCurXorPageNumInXore;
    U8 ucStoreXorPageNumInXore;
}XOR_NFC_RECOVER_CTRL;

// Control information of Bypass NFC flow.
typedef struct _XOR_BYPASS_NFC_CONTROL
{
    U32 ulXorPageSize;
    XOR_TARGET eTarget;
    union
    {
        U16 usStartXorPageNumInTotal;
        U16 usBrokenXorPageNumInTotal;
    };
    U8 ucStatus;
    U8 ucXoreId;
    U8 ucCurXorPageNumInXore;
    U8 ucXorPageCntOfLastTrigger;
    U8 ucStartXorPageNumOfLastTrigger;
}XOR_BYPASS_NFC_CTRL;

typedef struct _XOR_BYPASS_NFC_RECOVER_WITH_NFC_READ_CONTROL
{
    BOOL bIsTlcRecover;
    U32 ulXorPageSize;
    XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable;
    U16 usStartXorPageNumInTotal;
    U16 usBrokenXorPageNumInTotal;
    U8 ucStatus;
    U8 ucXoreId;
    U8 ucCurXorPageNumInXore;
    U8 ucStoreXorPageNumInXore;
    U8 ucXorPageCntOfLastTrigger;
    U8 ucStartXorPageNumOfLastTrigger;
    U8 ucPageType;
}XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL;

// Control information of Load-Store flow.
typedef struct _XOR_LOAD_STORE_CONTROL
{
    U32 ulXorPageSize;
    XOR_TARGET eTarget;
    U16 usXorPageNumInTotal;
    U8 ucStatus;
    U8 ucXoreId;
}XOR_LOAD_STORE_CTRL;

extern U8 ucLoopCnt;

void XorUt_ProtectPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal);
void XorUt_BypassRecoverPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal);
void XorUt_LoadFromDramPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal);
void XorUt_StoreToDramPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal);

BOOL XorUt_NfcProtect(XOR_NFC_PROTECT_CTRL *pNfcProtectCtrl);
BOOL XorUt_NfcProtectTlc(XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl);
BOOL XorUt_NfcProtect3dMlc(XOR_NFC_PROTECT_CTRL *p3dMlcNfcProtectCtrl);
BOOL XorUt_NfcRecover(XOR_NFC_RECOVER_CTRL *pNfcRecoverCtrl);
BOOL XorUt_BypassNfc(XOR_BYPASS_NFC_CTRL *pBypassNfcCtrl);
BOOL XorUt_BypassRecoverWithNfcRead(XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithNfcReadCtrl);
BOOL XorUt_LoadStore(XOR_LOAD_STORE_CTRL *pLoadStoreCtrl);

void XorUt_NfcProtectCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal);
void XorUt_NfcRecoverCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal);
void XorUt_BypassProtectCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal);
void XorUt_BypassRecoverCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal);
void XorUt_LoadFromDramCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal);
void XorUt_StoreToDramCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal);

void XorUt_DestPageClean(U32 ulXoreId, U32 ulXorPageSize);
void XorUt_LoadStorePageClean(U32 ulXoreId, U32 ulXorPageSize);
void XorUt_NfcRecoverCleanData(U32 ulXoreId, U32 ulXorPageSize);

U32 XorUt_GetRedunBaseAddr(void);
void XorUt_ClearNfcProtectCtrl(XOR_NFC_PROTECT_CTRL *pNfcProtectCtrl);
void XorUt_ClearTlcNfcProtectCtrl(XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl);
void XorUt_ClearNfcRecoverCtrl(XOR_NFC_RECOVER_CTRL *pNfcRecoverCtrl);
void XorUt_ClearBypassNfcCtrl(XOR_BYPASS_NFC_CTRL *pBypassNfcCtrl);
void XorUt_ClearBypassRecoverWithNfcReadCtrl(XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithNfcReadCtrl);
void XorUt_ClearLoadStoreCtrl(XOR_LOAD_STORE_CTRL *pLoadStoreCtrl);
void XorUt_Log(XOR_UT_LOG_TYPE eXorUtLogType, U32 ulRecordValue);

#endif/* __XORUT_COMMON_H__ */
