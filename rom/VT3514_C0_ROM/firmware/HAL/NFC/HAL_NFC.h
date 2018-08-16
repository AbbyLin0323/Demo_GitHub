/*************************************************
Copyright (c) 2014 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : NfcDriver.h                                          
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Peterlgchen

Description: 

Modification History:
20140221  Victor Zhang   Created   
*************************************************/


#ifndef _NFC_DRIVER_H
#define _NFC_DRIVER_H

/************    Definde SYNC MODE TEST or ASYNC MODE TEST  *******/

#ifndef VT3514_C0
#define HARDWARE_QUEUE_LEVEL_NUM    2
#else
#define HARDWARE_QUEUE_LEVEL_NUM    4
#endif

#define NFCQ_ENTRY_DW  16   
#define NFCQ_SEC_ADDR_COUNT 4
#define NFCQ_ROW_ADDR_COUNT 8
#define NFCQ_DMA_ENTRY_COUNT 32
#define NFCQ_DEPTH  HARDWARE_QUEUE_LEVEL_NUM

/******************************************************************************
    RED  definition 
******************************************************************************/

#define RED_NUM  16            //16,32,64byte mode,dual plan double
//REDUNDENT Byte entry
typedef struct _RED_ENTRY
{
    U32 RedDw[RED_NUM];    
}RED_ENTRY;

typedef struct _NF_CQ_REG_CMDTYPE // only for modle test
{
    U8 CmdType[NFCQ_DEPTH];
}NF_CQ_REG_CMDTYPE;

typedef struct _NFC_SEC_ADDR
{
    U16 bsSecLength:8;
    U16 bsSecStart:8;
}NFC_SEC_ADDR;

#ifndef VT3514_C0
/* NFCQ register */
typedef struct _NFCQ_ENTRY
{
    union
    {
        struct//for real NFC design
        {
            /* DW0 byte 0 */
            U32 bsOntfEn:1;       // set - otfb. clear -  dram
            U32 bsDmaByteEn:1;    // set - byte. clear - sector
            U32 bsIntEn:1;        // set - enable NFC generate interrrupt to mcu after cmd process done. clear - disable it
            U32 bsPuEnpMsk:1;     // set - mask = disable scramble; clear - enable scramble
            U32 bsPuEccMsk:1;     // set - mask = disable ECC; clear - enable ECC( ECC valid when PuLdpcEn = 0 )
            U32 bsPuLdpcEn:1;     // set - enable LDPC
            U32 bsRedEn:1;        // enable red
            U32 bsRedOnly:1;      // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase 

            /* DW0 byte 1 */
            U32 bsSsu0En:1;       // set - enable ssu0 , clear - disable ssu0.
            U32 bsSsu1En:1;       // set - enable ssu1 , clear - disable ssu1. used for cache status
            U32 bsTrigOmEn:1;     // set - Need to trig OTFBM before data xfer.  clear -  Not need 
            U32 bsDsgEn:1;        // set - fetch dsg ,clear - not fetch dsg
            U32 bsInjEn:1;        // set - enable error injection  , clear -disableerror injection
            U32 bsErrTypeS:3;     // 8 kinds of error types
    
            /* DW0 byte 2~3 */
            U32 bsNcqMode:1;      // 0 = First data ready set after the first 4K data transfer done regardless of BCH decode result;
                                  // 1 = First data ready set after the first 1K data transfer done regardless of BCH decode result;
            U32 bsNcqNum:5;       // 32 tag, if we have more than 32 tags, fill bit[5] to TagExt in DW3
            U32 bsN1En:1;         // ? 
            U32 bsFstDsgPtr:9;    // Record First DSG ID for incidental case if need release the DSG 
        };

        struct//for Fake NFC design
        {
            U32 bsFakeNfcOntfEn: 1;  // 1 = Message data is stored in on-the-fly SRAM.
            U32 bsFakeNfcTrigOmEn: 1;// need trig OTFBM before data transfer.
            U32 bsFakeNfcDW0Rsvd: 6;
            U32 ucFakeNfcTotalSec: 8;//As Total data length in this command. The unit is sector.
            U32 usFakeNfcDw0Rsvd: 16;
        };
    };

    /* DW1~2 */
    union
    {  
        NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];     // for DMA mode ,DW 1 means sec start and length
        
        /* for dma byte mode, DW1 redefined, DW2 not used */
        struct                                         // for byte mode ,DW1 means byte addr and length
        {
            U32 bsByteAddr:16;                         //
            U32 bsByteLength:16;                       // 
            U32 ulByteRev1;                            // Not Used In Byte Mode
        };

        struct//for Fake NFC design
        {
            U32 ulFakeNfcDRAMAddr; // NFCQ DW1, used for Fake NFC as DRAM address for DMA.
            U32 ulFakeNFcRsvd;     // NFCQ DW2~15 is not used by Fake NFC
        };
    };
    
    /* DW3 */  
    U32 bsDmaTotalLength:8;       //in sector
    U32 bsRedAddr:16;             // 
    U32 bsBmEn:1;                 // buffer map enable
    U32 bsFstDataRdy:1;           // first data ready enable
    U32 bsTagExt:1;               // bit[5] of bsNcqNum field if we have more than 32 tags
    U32 bsAddSgqCntEn:1;          // 1 = enable NFC add chain cnt after on-the-fly program ok
    U32 bsRsv3:4;                 //

    /* DW4~5 */
    U8 bsScrambleSeed[8];         // 

    /* DW6 */
    U32 bsSsuAddr0:8;             //  ssu addr[2:9]   dw mode
    U32 bsSsuData0:8;
    U32 bsRev6:16;
    
    /* DW7 */
    U32 bsSsuAddr1:16;            // original cachestatus , reserved    byte mode
    U32 bsSsuData1:8;             // reserved
    U32 bsRev7:8;
    
    /* DW8~15 */
    U32 aRowAddr[8];             // QE Addr group   (AQEE)
}NFCQ_ENTRY;

/* NFCQ control register */
typedef struct _PG_CONF_REG
{
    U32 PgSz:2;      // 00-4k ,01-8k ,10-16k ,11-32k
    U32 BlkSz:3;     // 000-64 ,001-128 ,010-256,011-512,100-1k,101-2k,110-4k,111-8k
    U32 EccSel:2;    // 00-Ecc24,01-Ecc40,10-Ecc64
    U32 IntMskEn:1;  // 1-mask ,0-not mask
    
    U32 RedNum:2;    // 00-16byte,01-32byte,10-48byte,11-64byte
    U32 PrcqDepth:3; // 000-4dw ,001-8dw,010-16dw,011-32dw,100-64dw
    U32 RbLoc:3;     // RB bit location in byte.(defined in SQEE. Will remove)
    
    U32 SFLoc:3;     // Success/Fail bit location in a byte.
    U32 EccTh:5;     // ECC error counter threshold. In bit.
                     // [4]: means X4 or not. If it is 1, {ECC_TH[3:0],2'b00} is the real threshold.
                     // [3:0]: Basic value of threshold.
    
    U32 IsMicron:1;  // It is ONFI device or not. 1-MICRON device, 0-Not MICRON device (if DDR_MODE=1, indicate it is toggle NAND).
    U32 BchEccET:2;  // BCHECC early termination enable
    U32 MulLun:2;    // Multi-LUN number:
                     // 00: not multi-LUN
                     // 01: 2 LUN in one target
                     // 10: 4 LUN in one target
                     // 11: 8 LUN in one target
    U32 SkipRed:1;   // 1: enable skip redundant data for LDPC
    U32 ChArbEn:1;   // 1: Arbitrate in 1K data. 0: Arbitrate in 256 byte data.
    U32 DisCmdHP:1;  // Disable command higher priority. 1-disable, 0-not disable.
    
}PG_CONF_REG;

/* NFC trigger register */
typedef union _NFC_TRIGGER_REG
{
    struct{
        U8 bsCmdType:2;  // 00 - write, 01 - read, 10 - erase, 11 - other
        U8 bsOtfb:1;     // 1 - data xfer through otfb , through dram
        U8 bsPio:1;      // 1 - PIO command, 0 - Normal command
        U8 bsRev:3;      // reserved
        U8 bsTrig:1;     // MCU write this register to inform command is filled.
    };
    U8 bsValue;
}NFC_TRIGGER_REG;

/* NF CQ register*/
typedef struct _NFC_CMD_STS_REG
{
    U8 bsIdle:1;   // 1 - idle, 0 - not idle, idle indicate empty or hold by error
    U8 bsErrh:1;   // 1 - error hold, 0 - no error
    U8 bsEmpty:1;  // 1 - empty, 0 - not empty
    U8 bsFull:1;   // 1 - full, 0 - not full 
    U8 bsRdPtr:1;  // read pointer, NFCLK domain.
    U8 bsWrPtr:1;  // write pointer. HCLK domain.
    U8 bsIntSts:1; // interrupt status, MCU can write this bit or public interrupt bit to clear the status.
    U8 bsPrst:1;   // When read, it returns "ERRH" bit. When write, trun current PU's all level into available
                   // status.when ERRH is cleared. Only software could write this bit.    

    U8 bsErrSts:4; // when Errh == 1, thi indicate error type
    U8 bsFsLv0:1;  // Level 0 finishing status 
    U8 bsFsLv1:1;  // Level 1 finishing status 
    U8 bsRq0Pset:1;// When write, current PU's Wp and Rp is set to 1. ERRH is clear. 
    U8 bsRes:1;    // reserved

    U8 ucCmdTypeRsv[2]; // reserved
}NFC_CMD_STS_REG;

#else
/* NFCQ register */
typedef struct _NFCQ_ENTRY
{
    union
    {
        struct//for real NFC design
        {
            /* DW0 byte 0 */
            U32 bsOntfEn:1;       // set - otfb. clear -  dram
            U32 bsDmaByteEn:1;    // set - byte. clear - sector
            U32 bsIntEn:1;        // set - enable NFC generate interrrupt to mcu after cmd process done. clear - disable it
            U32 bsPuEnpMsk:1;     // set - mask = disable scramble; clear - enable scramble
            U32 bsPuEccMsk:1;     // set - mask = disable ECC; clear - enable ECC( ECC valid when PuLdpcEn = 0 )
            U32 bsPuLdpcEn:1;     // set - enable LDPC
            U32 bsRedEn:1;        // enable red
            U32 bsRedOnly:1;      // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase 

            /* DW0 byte 1 ~ 3*/
            U32 bsSsu0En:1;       // set - enable ssu0 , clear - disable ssu0.
            U32 bsSsu1En:1;       // set - enable ssu1 , clear - disable ssu1. used for cache status
            U32 bsTrigOmEn:1;     // set - Need to trig OTFBM before data xfer.  clear -  Not need 
            U32 bsDsgEn:1;        // set - fetch dsg ,clear - not fetch dsg
            U32 bsInjEn:1;        // set - enable error injection  , clear -disableerror injection
            U32 bsErrTypeS:4;     // 8 kinds of error types
            U32 bsNcqNum:6;       // 32 tag, if we have more than 32 tags, fill bit[5] to TagExt in DW3
            U32 bsFstDsgPtr:9;    // Record First DSG ID for incidental case if need release the DSG 
        };

        struct//for Fake NFC design
        {
            U32 bsFakeNfcOntfEn: 1;  // 1 = Message data is stored in on-the-fly SRAM.
            U32 bsFakeNfcTrigOmEn: 1;// need trig OTFBM before data transfer.
            U32 bsFakeNfcDW0Rsvd: 6;
            U32 ucFakeNfcTotalSec: 8;//As Total data length in this command. The unit is sector.
            U32 usFakeNfcDw0Rsvd: 16;
        };
    };

    /* DW1~2 */
    union
    {  
        NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];     // for DMA mode ,DW 1 means sec start and length
        
        /* for dma byte mode, DW1 redefined, DW2 not used */
        struct                                         // for byte mode ,DW1 means byte addr and length
        {
            U32 bsByteAddr:16;                         //
            U32 bsByteLength:16;                       // 
            U32 ulByteRev1;                            // Not Used In Byte Mode
        };

        struct//for Fake NFC design
        {
            U32 ulFakeNfcDRAMAddr; // NFCQ DW1, used for Fake NFC as DRAM address for DMA.
            U32 ulFakeNFcRsvd;     // NFCQ DW2~15 is not used by Fake NFC
        };
    };
    
    /* DW3 */  
    U32 bsDmaTotalLength:8;       // in sector
    U32 bsRedAddr:16;             // 16 byte align
    U32 bsBmEn:1;                 // buffer map enable
    U32 bsFstDataRdy:1;           // first data ready enable
    U32 bsN1En:1;                 // calculate N1 enable
    U32 bsAddSgqCntEn:1;          // enable count program success command number for AHCI OTFB write error handling
    U32 bsFstByteCheck:1;         // check first byte data every sector in read command
    U32 bsEMEn:1;                 // get AES seed for every sector, from redundant data in wrie command; form AES_SED SRAM in read command
    U32 bsNcqMode:1;              // 0 = First data ready set after the first 4K data transfer done regardless of BCH decode result;
                                  // 1 = First data ready set after the first 1K data transfer done regardless of BCH decode result;
    U32 bsOtfbBypass:1;           // OTFBM bypass data transfer to OTFB; set:by pass; Clear:not bypass

    /* DW4~5 */
    U8 bsScrambleSeed[8];         

    /* DW6 */
    U32 bsSsuAddr0:8;             // DW align
    U32 bsSsuData0:8;
    U32 bsRev6:16;
    
    /* DW7 */
    U32 bsSsuAddr1:16;            // byte align
    U32 bsSsuData1:8;             
    U32 bsRev7:8;
    
    /* DW8~15 */
    U32 aRowAddr[8];             // QE Addr group   (AQEE)
}NFCQ_ENTRY;


/* NFC trigger register */
typedef union _NFC_TRIGGER_REG
{
    struct {
        U16 bsCmdType:2;   // 00 - write, 01 - read, 10 - erase, 11 - other
        U16 bsInsert:1;    // 1 - Indicate command in this level could insert into other level's command
                           // 0 - Disable insert feature. 
        U16 bsPio:1;       // 1 - PIO command ,0 -Normal command
        U16 bsLun:2;       // Lun Index  0~3
        U16 bsLunEn:1;     // 1 -  Indicate command in this level is a multi-LUN command and could jump to other level.
                           // 0 -  Other command.    
        U16 bsCacheOp:1;   // 1-    Indicate command in this level is a cache read/write command. 0 - other command

        U16 bsCESel:1;
        U16 bsRsv:7;
    };
    U16 bsValue;

}NFC_TRIGGER_REG;

/* NF CQ register*/
typedef struct _NFC_CMD_STS_REG
{
    U8 bsIdle:1;     // 1 - idle, 0 - not idle, idle indicate all HW levels cmd all done
    U8 bsErrh:1;     // 1 - error hold, 0 - no error
    U8 bsEmpty:1;    // 1 - empty, 0 - not empty
    U8 bsFull:1;     // 1 - full, 0 - not full 
    U8 bsRdPtr:2;    // Indicate which HW level the next read cmd pointer directing
    U8 bsWrPtr:2;    // Indicate which HW level the next write cmd pointer directing
    
    U8 bsErrSts:4;   // when Errh == 1, thi indicate error type
    U8 bsIntSts:1;   // interrupt status, MCU can write this bit or public interrupt bit to clear the status.
    U8 bsPrst:1;     // When read, it returns "ERRH" bit. When write, trun current PU's all level into available
                     // status.when ERRH is cleared. Only software could write this bit. 
    U8 bsRq0Pset:1;  // When write, current PU's Wp and Rp is set to 1. ERRH is clear.
    U8 bsres:1;      // reserved
    
    U8 bsCmdType0:3; // Level 0 Cmd type
    U8 bsFsLv0:1;    // Level 0 finishing status 
    U8 bsCmdType1:3; // Level 1 Cmd type
    U8 bsFsLv1:1;    // Level 1 finishing status 
    U8 bsCmdType2:3; // Level 2 Cmd type
    U8 bsFsLv2:1;    // Level 2 finishing status 
    U8 bsCmdType3:3; // Level 3 Cmd type
    U8 bsFsLv3:1;    // Level 3 finishing status 
}NFC_CMD_STS_REG;

#endif


/******************************************************************************
    DPTR  definition 
******************************************************************************/

typedef struct NFCQ_DPTR_
{
    NFC_CMD_STS_REG  CQ_REG[CE_MAX];
}NFCQ_DPTR;

typedef struct NFCQ_ARRAY_
{
    NFCQ_ENTRY NfcqEntry[CE_MAX][NFCQ_DEPTH];
}NFCQ_ARRAY;

/******************************************************************************
    PRCQ  definition 
******************************************************************************/
#define PRCQ_DEPTH  64
#define PRCQ_LEVEL  HARDWARE_QUEUE_LEVEL_NUM

#define QE_INDEX_LEVEL  16
#define QE_CMD_GROUP_DEPTH  16
#define QE_OPRATION_GROUP_DEPTH 8

typedef struct PRCQ_ELEM_
{
    U8 Index:4;    
    U8 Attr:2;
    U8 Group:2;
}PRCQ_ELEM,*P_PRCQ_ELEM;

typedef struct PRCQ_ENTRY_
{
    U8   Entry[PRCQ_DEPTH];
}PRCQ_ENTRY;

typedef struct PRCQ_ARRAY_
{
    PRCQ_ENTRY   Array[CE_MAX][PRCQ_LEVEL];
}PRCQ_ARRAY;

typedef struct _QEE_ENTRY_REG
{
    /* 1 cycle cmd  group */
    U8 PRCQ_CQE1E[QE_CMD_GROUP_DEPTH] ;  

    /* 2 cycle cmd group */
    U16 PRCQ_CQE2E[QE_CMD_GROUP_DEPTH] ;

    U32 res[8];
    /* status group */
    U16 PRCQ_SQEE[QE_OPRATION_GROUP_DEPTH] ;

}QEE_ENTRY_REG;


typedef enum _PRCQ_QE
{
    PRCQ_ONE_CYCLE_CMD = 0,
    PRCQ_TWO_CYCLE_CMD,
    PRCQ_TWO_PHASE_CMD,
    PRCQ_THREE_PHASE_CMD,

    PRCQ_ONE_CYCLE_ADDR,
    PRCQ_COL_ADDR,
    PRCQ_ROW_ADDR,
    PRCQ_FIVE_CYCLE_ADDR,

    PRCQ_REG_READ,
    PRCQ_REG_WRITE,
    PRCQ_DMA_READ,
    PRCQ_DMA_WRITE,

    PRCQ_READ_STATUS,
    PRCQ_READ_STATUS_EH,
    PRCQ_IDLE,
    PRCQ_FINISH
}PRCQ_QE;

typedef enum _CQE1E_INDEX  
{
    CQE1E_EF,CQE1E_EE,CQE1E_FF,CQE1E_FA,
    CQE1E_90,CQE1E_00,CQE1E_30,CQE1E_60,
    CQE1E_D0,CQE1E_31,CQE1E_3F,CQE1E_85,
    CQE1E_FC,CQE1E_D4,CQE1E_D5,CQE1E_A2
}CQE1E_INDEX;

typedef enum _CQE2E_INDEX  
{
    CQE2E_3000,CQE2E_1080,CQE2E_1181,CQE2E_D060,
    CQE2E_1180,CQE2E_1081,CQE2E_E005,CQE2E_E006,
    CQE2E_3060,CQE2E_1580,CQE2E_3200,CQE2E_3100,
    CQE2E_FFFF
}CQE2E_INDEX;

typedef enum _SQEE_INDEX  
{
    SQEE_3E70,SQEE_2870,SQEE_1870,SQEE_0870,
    SQEE_B870,SQEE_7870,SQEE_3C70
}SQEE_INDEX;

// PRCQ mode   : modified by victor zhang     2013.8.14
// QE    7      6 |    5    4 |    3    2    1    0| 
//        Group |    Attr  |    Index         |
#define QE_INDEX(x)         (x & 0xf)
#define QE_TYPE(_Type_)     (_Type_ << 4)   

#define NF_PRCQ_CMD_BSY        (QE_TYPE(PRCQ_READ_STATUS)  | QE_INDEX(0x0))  //c1 : read status 
#define NF_PRCQ_CMD_FINISH    (QE_TYPE(PRCQ_FINISH)       | QE_INDEX(0xF))      //ff : finish or end


/******************************************************************************
    TRIGGER  definition 
******************************************************************************/

#define TRIGGER_LEVEL   HARDWARE_QUEUE_LEVEL_NUM

typedef struct _NFCQ_TRIGGER_REG
{
    NFC_TRIGGER_REG NfcqTriggerREG[CE_MAX][TRIGGER_LEVEL];
}NFCQ_TRIGGER_REG;

typedef struct FLASH_ID
{
    U32 ID[2];
}FLASH_ID;

/* trigger command type define*/
typedef enum _NFC_TRIGGER_CMD_TYPE
{
    TRIG_CMD_WRITE = 0,
    TRIG_CMD_READ,
    TRIG_CMD_ERASE,
    TRIG_CMD_OTHER    
}NFC_TRIGGER_CMD_TYPE;

/******************************************************************************
    Command table definition 
******************************************************************************/

typedef struct _RCMD_PARAM_TABLE
{
    U32 CmdCode:8;
    U32 QEPhase:8;
    U32 IsPIO:1;
    U32 TrigType:2;
    U32 Rsv:13;
    U32 QEPtr;
}RCMD_TABLE;


/************************************************
        NFC GLOBAL Function declare and extern
************************************************/
GLOBAL void HAL_NfcInit(void);
GLOBAL BOOL HAL_NfcReadId(U8 ucPu);
GLOBAL BOOL HAl_NfcSinglePuStatus(U8 ucPu);
GLOBAL BOOL HAL_NfcResetPu(U8 ucPu);
GLOBAL BOOL HAL_NfcReadBootloader(U8 ucPu);
#endif// #ifndef _NFC_DIRVER_H

/****************    FILE         END *************/


