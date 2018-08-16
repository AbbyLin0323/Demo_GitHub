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
* NON-INFRINGEMENT.
********************************************************************************
Description :
*******************************************************************************/
#ifndef _HAL_HGE_H_
#define _HAL_HGE_H_

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "COM_Memory.h"


#define HGE_ENTRY_MAX 6     //256/40 = 6 (per MCU)
#define HGE_DES_ADDR_BASE1 HGE_ENTRY_BASE   //total 0x200?
#define HGE_DES_ADDR_BASE2 HGE_ENTRY_BASE + 0x100
#define HGE_PG_SIZE 32768       //tmp (depend on pipe page size)
#define RETRY_TIME_MAX 10       //tmp (should get from HAL_FlashChipDefine.h)
#define VT_MAX 7                //tmp (MLC=3,TLC=7 ,should get from HAL_FlashChipDefine.h)
#define HGE_QW_ALIGN_BITS 3     //RD NUM need to be a multiple of 8 (QW as unit)
#define HGE_TEST_LOOP 10        //For HGE test without flash read (just prepare fake dram data)

typedef enum _HGE_VT
{
    AR = 1,
    BR,
    CR,
    DR,
    ER,
    FR,
    GR
    //and more TBD for TLC & 3D Nand
} HGE_VT;


typedef enum _HGE_PATTERN_IDX
{
    PATTERN_IDX_MLC_BR = 0,
    PATTERN_IDX_MLC_AR_CR,
    PATTERN_IDX_TLC_AR_BR_CR_DR,
    PATTERN_IDX_TLC_ER_FR_GR
    //and more TBD for TLC & 3D Nand
} HGE_PATTERN_IDX;


typedef enum _HGE_VT_CNT
{
    HGE_SLC_TYPE_VT_CNT = 1,
    HGE_MLC_TYPE_VT_CNT = 3,
    HGE_TLC_TYPE_VT_CNT = 7
    
    //and more TBD for TLC & 3D Nand
} HGE_VT_CNT;


typedef enum _HGE_LOOPTIME
{
    HGE_SLC_TYPE_LOOPTIME = 1,
    HGE_MLC_TYPE_LOOPTIME = 2,
    HGE_TLC_TYPE_LOOPTIME = 2
    
    //and more TBD for TLC & 3D Nand
} HGE_LOOPTIME;


/* Descriptor id with MCU id*/
typedef struct _HGE_DES_ID
{
    U8 ulDescMcu;   //MCU id
    U32 ulDescID;   //Descriptor id
} HGE_DES_ID;


/* For calculation of final results */
typedef struct _HGE_FIN_RESULTS
{
    U32 ulResults;          //The results of A -B
    U8 ucVtValueA;          //Vt value or retry time
    U8 ucVtValueB;          //Vt value or retry time
    U8 ucDone;              //both got A & B or not (1:yes ; 0:no)
} HGE_FIN_RESULTS;


/* Storing the results from HGE engine */
typedef struct _HGE_DES_RESULTS
{
    HGE_DES_ID ulDescID;    //Descriptor ID (Maybe no need)
    U32 ulResults;          //18bit results
    HGE_VT ucHgeVt;         // AR,BR,CR ¡K
    U8 ucVtValue;           //Vt value or retry time
} HGE_DES_RESULTS;


/* Storing the results from HGE engine */
typedef struct _HGE_RESULTS
{
    HGE_DES_RESULTS ulHgeDesResults[RETRY_TIME_MAX*VT_MAX];
} HGE_RESULTS;


/* Patterns information when setting HGE */
typedef struct _HGE_PATTERN_INFO
{
    U8 ucHgePattern0 : 4;
    U8 ucHgePatternMask0 : 4;
    U8 ucHgePattern1 : 4;
    U8 ucHgePatternMask1 : 4;
    U8 ucHgePattern2 : 4;
    U8 ucHgePatternMask2 : 4;
    U8 ucHgePattern3 : 4;
    U8 ucHgePatternMask3 : 4;
} HGE_PATTERN_INFO;


/* Read adddress information when setting HGE */
typedef struct _HGE_READ_ADDRESS_INFO
{
    U32 ulHgeReadAddr0;
    U32 ulHgeReadAddr1;
    U32 ulHgeReadAddr2;
    U32 ulHgeReadAddr3;
} HGE_READ_ADDRESS_INFO;

/* FIFO for HGE fifo behavior test */
typedef struct _FIFO_Queue
{
    U8 ucDesId;
    U8 ucRetryTime;
}FIFO_Queue;


/* Histogram engine register */
typedef struct _HISTOGRAM_REG
{
    /* DW0 */
    U32 ulReadDescAddrBase1;        // RDESADDR_BASE1

    /* DW1 */
    U32 ulReadDescAddrBase2;        // RDESADDR_BASE1

    /* DW2 */
    U32 bsRptr1 : 8;                // Hardware RPTR of Descriptor1
    U32 bsRptr2 : 8;                // Hardware RPTR of Descriptor2
    U32 bsWptr1 : 8;                // Hardware WPTR of Descriptor1
    U32 bsWptr2 : 8;                // Hardware WPTR of Descriptor2

    /* DW3 */
    U32 bsHgeRst : 1;               // Engine reset
    U32 bsIntEn : 1;                // Interrupt enable  1:decode done
    U32 bsHgeCkgEn : 1;             // Histogram Engine clk gating enable
                                    // 0:disable
                                    // 1:enable
    U32 bsDbgSel : 2;               // Debug signal select
    U32 bsIntSts : 1;               // Interrupt status 1:decode done (DONE)
    U32 bsRev0 : 2;
    U32 bsEntrySize : 8;            // Descriptor number
    U32 bsRev1 : 16;
}HISTOGRAM_REG;


/* Histogram Descriptor */
typedef struct _HGE_DESC
{
    union
    {
        struct
        {
            U32 ulDW[10];
        };

        struct
        {
            /* DW0 */
            U32 bsValidBit : 1;     // 0 to indicate the descriptor is invalid.
            // 1 to indicate the descriptor is valid and wait for processing
            // HW writes 0 after finishing task of this descriptor
            U32 bsRdNum : 2;            // Number of read times for histogram engine (1,2,3,4)
            U32 bsIntStsEn : 1;     // Interrupt status enable bits
            // 1 : enable DONE status
            U32 bsRev1 : 12;
            U32 bsRdLength : 16;        // # of QW for each read

            /* DW1 */
            U32 bsPattern0 : 4;     // Pattern 0
            U32 bsPattern1 : 4;     // Pattern 1
            U32 bsPattern2 : 4;     // Pattern 2
            U32 bsPattern3 : 4;     // Pattern 3
            U32 bsPatternMask0 : 4; // Pattern Mask 0
            U32 bsPatternMask1 : 4; // Pattern Mask 1
            U32 bsPatternMask2 : 4; // Pattern Mask 2
            U32 bsPatternMask3 : 4; // Pattern Mask 3

            /* DW2 */
            U32 ulR0Addr;   // Read address of 1st read data

            /* DW3 */
            U32 ulR1Addr;   // Read address of 2nd read data

            /* DW4*/
            U32 ulR2Addr;   // Read address of 3rd read data

            /* DW5*/
            U32 ulR3Addr;   // Read address of 4th read data

            /* DW6*/
            U32 bsPat0AccumuResult : 18;    // Accumulation Results for Pattern 0
            U32 bsRev2 : 14;

            /* DW7*/
            U32 bsPat1AccumuResult : 18;    // Accumulation Results for Pattern 1
            U32 bsRev3 : 14;

            /* DW8*/
            U32 bsPat2AccumuResult : 18;    // Accumulation Results for Pattern 2
            U32 bsRev4 : 14;

            /* DW9*/
            U32 bsPat3AccumuResult : 18;    // Accumulation Results for Pattern 3
            U32 bsRev5 : 14;
        };
    };
} HGE_DESC;

void HAL_HgeCheckPatternMatchSLC(U32* ulLowPageData, U32 ulCompareSize, U32 ulRetryTime, HGE_DESC * HgeDescriptor);
void HAL_HgeCheckPatternMatchMLC(U32* ulLowPageData, U32* ulHighPageData, U32 ulCompareSize, U32 ulRetryTime, HGE_DESC * HgeDescriptor);
void HAL_HgeCheckPatternMatchTLC(U32* ulLowPageData, U32* ulMidPageData, U32* ulHighPageData, U32 ulCompareSize, U32 ulRetryTime, U8 ucIndex, HGE_DESC * HgeDescriptor);
void HAL_HgeDsAlloc(U32 *pDramBase);
void HAL_HgeRegInit(void);
void HAL_HgeRegReset(void);
BOOL HAL_HgeCheckIdle(U32 ulMcuId);
U32 HAL_HgeGetValidDescriptor(U32 ulMcuId);
U8 HAL_HgeGetCurrentWp(U32 ulMcuId);
U32 HAL_HgeGetDescriptorAddress(HGE_DES_ID DesId);
void HAL_HgeSetDescriptor(HGE_DESC *HgeDescriptor, U8 ucPattern_idx, HGE_READ_ADDRESS_INFO ReadAddrInfo, U8 ucRdNum, U32 ulSize);
void HAL_HgeRegTrigger(U32 McuId);
void HAL_HgeWaitDescriptorDone(volatile HGE_DESC *HgeDescriptor);
void HAL_HgeGetDescriptorResultsMlc(HGE_DESC *HgeDescriptor, U8 ucHgeVt);
void HAL_HgeGetDescriptorResultsTlc(HGE_DESC *HgeDescriptor, U8 ucHgeVt, U8 ucIndex);
void HAL_HgeSetReadAddrInfo(HGE_READ_ADDRESS_INFO *ReadAddrInfo, U32 AddrInfo, U32 index);
U8 HAL_HgeDataAnalysis(U8 ucType);

#endif // _HAL_HGE_H_

