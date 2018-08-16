/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : RDT.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.4.27
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _RDT_H
#define _RDT_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define RDT_PG_NUM 1

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _RDT_HEAD{
    //dword0
    U8 ucStage;//t1 or t2
    U8 ucIdbEnable;
    U8 ucInherit;
    U8 ucFailStop;

    //dword1
    U32 ulReccValue;

    //dword2
    U16 usBeginBlk;
    U16 usTestBlkCnt;

    //dword3
    U8 ucBeginPln;
    U8 ucTestPlnCnt;
    U16 Rsv1;

    //dword4
    U16 usLoopCnt;
    U8 ucPattenCnt;
    U8 ucSlcMode;

    //dword5
    U16 usBadBlkCnt;
    U16 usBadBlkThres;

    //dword6~dword9
    U8 ucPattenVal[16];

    //dword10
    U8 ucRdtLogSizeInPage;
    U8 ucByPassRDTEn;  // Bypass RDT test to save result.
    U16 usRDTTime;

    //dword11
    U32 ulRsv;  //not used
}RDT_HEAD, *PRDT_HEAD;

typedef struct _RDT_PAYLOAD_ENTRY{
    U16 usBlk;         //Bad block position
    U8 ucPln;          //Bad block position
    U8 ucStage : 2;    //Bad block stage
    U8 ucErrType : 6;  //Bad block error type
}RDT_PAYLOAD_ENTRY, *PRDT_PAYLOAD_ENTRY;

typedef enum _ERR_TYPE{
    RDT_IDB_ERR,
    RDT_ERASE_ERR,
    RDT_WRITE_ERR,
    RDT_READ_ERR,
}ERR_TYPE;

typedef enum _RDT_TLUN_STAT{
    RDT_PASS,
    RDT_FAIL
}RDT_TLUN_STAT;

typedef enum _RDT_STAGE_
{
    RDT_INIT = 0,
    RDT_SEARCH,
    RDT_IDB,
    RDT_TEST,
    RDT_SAVE,
    RDT_CHK_RESULT,
    RDT_FORMATGB,
    RDT_FINISH
}RDT_STAGE;

typedef struct _RDT_SORT_MANAGER_
{
    U32 bsOpStage;

    U32 bsPln : 8;
    U32 bsRsvd : 24;

    U32 bsBlk : 16;
    U32 bsPage : 16;

    U32 FlashStatusBuffAddr;
    U32 DataBuffAddr;
    U32 SpareBuffAddr;
}RDT_SORT_MANAGER;

typedef struct _RDT_TEST_MANAGER
{
    U32 Blk;

    U16 Page;
    U16 PrePage;  // Recode the lower page nuber when program tlc pair page
    
    U32 Pln : 8;
    U32 CurStage : 8;
    U32 LastPage : 16;

    U16 StartBlk;
    U16 LastBlk;

    U32 FlashSLCMode  : 1;
    U32 FlashSingePln : 1;
    U32 PattenValue : 8;
    U32 Resev2 : 22;

    U32 DataBuffAddr;
    U32 SpareBuffAddr;
    U32 FlashStatusBuffAddr;

    U8  ErrhStage;
    U16 ErrhPage;
    U8  Resev1;

}RDT_TEST_MANAGER;

typedef struct _RDT_ERR_NUM_
{
    U16 EraseErrNum;
    U16 WriteErrNum;

    U16 ReccNum;
    U16 UeccNum;

    U16 AllErrNum;
    U16 ResvU16;
}RDT_ERR_NUM;

typedef enum _RDT_SORT_STAGE_
{
    RDT_SORT_INIT = 0,
    RDT_SORT_ERASE,
    RDT_SORT_ERASE_WAIT,
    RDT_SORT_WRITE,
    RDT_SORT_WRITE_WAIT,
    RDT_SORT_READ,
    RDT_SORT_READ_WAIT,
    RDT_SORT_READ_RED,
    RDT_SORT_READ_RED_WAIT,
    RDT_SORT_FINISH
}RDT_SORT_STAGE;

typedef enum _RDT_TEST_STAGE_
{
    RDT_TEST_INIT = 0,
    RDT_TEST_ERASE,
    RDT_TEST_WRITE,
    RDT_TEST_READ,
    RDT_TEST_FINISH
}RDT_TEST_STAGE;

typedef enum _RDT_CHECK_RESULT_STAGE_
{
    RDT_CHECK_RESULT_INIT = 0,
    RDT_CHECK_RESULT_FIND_TARGET,
    RDT_CHECK_RESULT_READ_TARGET,
    RDT_CHECK_RESULT_CHECK_TARGET,
    RDT_CHECK_RESULT_FINISH
}RDT_CHECK_RESULT_STAGE;

typedef struct _RDT_CTRL
{
    /*Rdt loop control*/
    U32 ulRdtRunCycle;
    U32 ulRdtRunCycleCnt;

    /*Rdt test LUN control*/
    U32 ulFinishLunNum;
    U32 ulRdtTestLunNumCnt;

    /*Rdt test block control*/
    U16 usStartBlk;
    U16 usLastBlk;

    /*Rdt patten control*/
    U32 ulRdtPatten;
    U32 ulRdtPattenCnt;
    U32 ulRdtPattenValue[16];

    /*Rdt test bad block and error handling bit map table*/
    U16 usLocalRdtOffset;
    U32 ulLocalBbtAddr[SUBSYSTEM_LUN_MAX];

    /*Rdt page location*/
    U8 usRdtPln[SUBSYSTEM_LUN_MAX];
    U16 ucRdtPage[SUBSYSTEM_LUN_MAX];

    /*Rdt bad block count and Pu status*/
    U16 usBadBlkCnt[SUBSYSTEM_LUN_MAX];
    RDT_TLUN_STAT RdtTlunStat[SUBSYSTEM_LUN_MAX];

    /*Rdt Header*/
    RDT_HEAD tRdtHead;

}RDT_CTRL;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
BOOL L3_RDTTest(void);

#endif
/*====================End of this head file===================================*/

