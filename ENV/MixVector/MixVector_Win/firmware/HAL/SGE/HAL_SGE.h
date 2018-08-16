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
Filename     : HAL_SGE.h                                    
Version      : Ver 1.0                                               
Date         :                                         
Author       : Gavin

Description: 
     Scatter gather engine,we named it as SGE,is a module to indicate the 
     host/device memory assignments and to control the data transferring 
     between the host/device memory.
     For VT3514B0 ASIC ,the SGE supports DRQ for completing the data stream 
     from device(DRAM) to host ,and DWQ for host to device(DRAM).And SGQ for 
     OTFB to host memory. 
Modification History:
20130909    Gavin   created
20140915    Victor  modified according to new coding style 
*******************************************************************************/

#ifndef __HAL_SGE_H__
#define __HAL_SGE_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define DW_LENGTH                   (sizeof(U32))

#define DRQ_DEPTH_MCU0              (32)
#define DRQ_DEPTH_MCU1              (32)
#define DRQ_DEPTH_MCU2              (32)

#define DWQ_DEPTH_MCU0              (32)
#define DWQ_DEPTH_MCU1              (64)
#define DWQ_DEPTH_MCU2              (64)


#define DRQ_BASE_ADDR_MCU1          (DRQ_BASE_ADDR)
#define DRQ_BASE_ADDR_MCU2          (DRQ_BASE_ADDR_MCU1 + DRQ_DEPTH_MCU1*DW_LENGTH)          
#define DWQ_BASE_ADDR_MCU1          (DRQ_BASE_ADDR_MCU2 + DRQ_DEPTH_MCU2*DW_LENGTH)
#define DWQ_BASE_ADDR_MCU2          (DWQ_BASE_ADDR_MCU1 + DWQ_DEPTH_MCU1*DW_LENGTH)
#define DRQ_BASE_ADDR_MCU0          (DWQ_BASE_ADDR_MCU2 + DWQ_DEPTH_MCU2*DW_LENGTH)
#define DWQ_BASE_ADDR_MCU0          (DRQ_BASE_ADDR_MCU0 + DRQ_DEPTH_MCU0*DW_LENGTH)

/* SGE ENTRY 1DW */
typedef struct _SGE_ENTRY_{
    U32 bsDsgPtr:9;         // First dsg id of dsg chain 
    U32 bsWriteEn:1;        // set for write , clear for read
    U32 bsType:2;           // reserve
    U32 bsMeta:1;           // reserve
    U32 bsDChainInvalid:1;  // set   : CHNUM_DIS not count in this chain in finish chain number
                            // clear : Count in this chain in chain number.
    U32 Res:2; 
    U32 bsHsgPtr:10;        // First hsg id of hsg chain
    U32 bsHID:6;            // command tag
}SGE_ENTRY;

/* DRQ/DWQ STATUS REGISTER 1DW */
typedef union _DRQ_DWQ_REG_{
    struct{
    U32 bsTrig: 1;
    U32 bsFull: 1;
    U32 bsEmpty: 1;
    U32 bsRsvd: 13;
    U32 bsRdPtr: 8;
    U32 bsWtPtr: 8;
    };
    U32 ulValue;
}DRQ_DWQ_REG;

typedef union _CHAIN_NUM_REG_{    
    struct {
        U32 bsValid:1;
        U32 bsClear:1;
        U32 bsHID:6;
        U32 bsChainNum:16;
        U32 res:8;
    };
    U32 ulChainNumReg;
}CHAIN_NUM_REG;

typedef struct _HID_FINISH_STATUS_{
    U32 bsMcu1Valid:1;
    U32 bsMcu2Valid:1;
    U32 bsFinishChainNum:17;
    U32 Res1:13;
    U32 bsTotalChainNum:16;
    U32 bsHid:6;
    U32 Res2:10;
}HID_FINISH_STATUS;


typedef struct _SGE_STATUS_REG_{
    U32 bsDsgEmpty:1;
    U32 bsDsgFull:1;
    U32 Res1:2;

    U32 bsHsgEmpty:1;
    U32 bsHsgFull:1;
    U32 Res2:2;

    U32 bsDwqEmpty:1;
    U32 bsDwqFull:1;
    U32 Res3:2;
    
    U32 bsDrqEmpty:1;
    U32 bsDrqFull:1;
    U32 Res4:2;

    U32 bsSgqEmpty:1;
    U32 bsSgqFull:1;
    U32 Res5:14;
}SGE_STATUS_REG;

typedef union _SGE_MODE_CLKEN_REG_
{
    struct{
    U32 bsCprsMode       :1;
    U32 bsRes0           :7;
    U32 bsClkEn          :8;    // Prepare for patch SGE bug 
    U32 bsAllEngIdle     :1;
    U32 bsAllTransFsh    :1;
    U32 bsRes1           :14;   
    };
    U32 ulValue;
}SGE_MODE_CLKEN_REG;
/* hardware register interface */
/******************************************************
    REG_BASE_SGE (0x1c800 + DSRAM_START_ADDR)
******************************************************/

//#define rSgqBaseAddr     (*(volatile U32 *)(REG_BASE_SGE + 0x0))
#define rModeClkEn       (*(volatile U32 *)(REG_BASE_SGE + 0x0))   

#define rDwqBaseAddr     (*(volatile U32 *)(REG_BASE_SGE + 0xc))
#define rDrqBaseAddr     (*(volatile U32 *)(REG_BASE_SGE + 0x10))
/******************************************************
REG: SG Status Register   (for test)
bit[0]  :   DSG Empty   RO
bit[1]  :   DSG Full       RO
bit[3:2]  : res
bit[4]  :   HSG Empty   RO
bit[5]  :   HSG Full        RO
bit[7:6]  : res
bit[8]  :   DWQ Empty   RO
bit[9]  :   DWQ Full   RO
bit[11:10]  :   res
bit[12] :   DRQ Empty   RO
bit[13] :   DRQ Full    RO
bit[15:14]  :   res
bit[16] :   SGQ Empty   RO
bit[17] :   SGQ Full    RO
bit[31:18]  res
******************************************************/
#define rSgStatus       (*(volatile U32 *)(REG_BASE_SGE + 0x20))
/******************************************************
REG:    DRQ Status  for mcu1 

31:24      RW    8'b0        DRQ write pointer for MCU1     MCU1_DRQ_wpt
23:16      RW    8'b0        DRQ read pointer for MCU1      MCU1_DRQ_rpt
15:3       RW    Reserved    Reserved                       Reserved
2          RO    1'b0        DRQ empty                      MCU1_DRQ_Empty
1          RO    1'b0        DRQ full                       MCU1_DRQ_full
0          RW    1'b0        DRQ trigger                    MCU1_DRQ_trigger

******************************************************/
#define DRQ_MCU1_STS_REG_ADDR   (REG_BASE_SGE + 0x24)
#define rDrqMcu1Status (*(volatile U32 *)DRQ_MCU1_STS_REG_ADDR)

/******************************************************
REG:    DWQ status for MCU1 set Registers

Bit         Attribute   Default     Description                 Mnemonic
31:24       RO          8'b0        DWQ write pointer for MCU1  MCU1_DWQ_wpt
23:16       RO          8'b0        DWQ read pointer for MCU1   MCU1_DWQ_rpt
15:3        RW          Reserved    Reserved                    Reserved
2           RO          1'b0        DWQ empty                   MCU1_DWQ_Empty
1           RO          1'b0        DWQ full                    MCU1_DWQ_full
0           RW          1'b0        DWQ trigger                 MCU1_DWQ_trigger


******************************************************/
#define DWQ_MCU1_STS_REG_ADDR   (REG_BASE_SGE + 0x28)
#define rDwqMcu1Status (*(volatile U32 *)DWQ_MCU1_STS_REG_ADDR)

/******************************************************
DRQ status for MCU2 set Registers (2F-2Ch)
Bit     Attribute   Default     Description                 Mnemonic
31:24   RO          8'b0        DRQ write pointer for MCU2  MCU2_DRQ_wpt
23:16   RO          8'b0        DRQ read pointer for MCU2   MCU2_DRQ_rpt
15:3    Reserved    Reserved    Reserved                    Reserved
2       RO          1'b0        DRQ empty                   MCU2_DRQ_Empty
1       RO          1'b0        DRQ full                    MCU2_DRQ_full
0       RW          1'b0        DRQ trigger                 MCU2_DRQ_trigger


******************************************************/
#define DRQ_MCU2_STS_REG_ADDR   (REG_BASE_SGE + 0x2c)
#define rDrqMcu2Status (*(volatile U32 *)DRQ_MCU2_STS_REG_ADDR)

/******************************************************
DWQ status for MCU2 set Registers (33-30h)
Bit     Attribute   Default     Description                 Mnemonic
31:24   RO          8'b0        DWQ write pointer for MCU2  MCU2_DWQ_wpt
23:16   RO          8'b0        DWQ read pointer for MCU2   MCU2_DWQ_rpt
15:3    RW          Reserved    Reserved                    Reserved
2       RO          1'b0        DWQ empty                   MCU2_DWQ_Empty
1       RO          1'b0        DWQ full                    MCU2_DWQ_full
0       RW          1'b0        DWQ trigger                 MCU2_DWQ_trigger
******************************************************/
#define DWQ_MCU2_STS_REG_ADDR   (REG_BASE_SGE + 0x30)
#define rDwqMcu2Status (*(volatile U32 *)DWQ_MCU2_STS_REG_ADDR)

/******************************************************
SGQ status Registers 2(4B-48h)
Bit    Attribute    Default    Description    Mnemonic
31:0    RO    32'b0    SGQ entry 32~63 busy; 1: busy 0: idle     SGQ_busy[63:32]

******************************************************/
#define rSgqStatusLo     (*(volatile U32 *)(REG_BASE_SGE + 0x44))
#define rSgqStatusHi     (*(volatile U32 *)(REG_BASE_SGE + 0x48))



/******************************************************
CHN_NUM Registers (4F-4Ch)
Bit     Attribute   Default Description                                                     Mnemonic
31£º24  Reserved    0       Reserved                                                        RSV1
23:8    RW          0       total chain number built by MCU1                                CHN_NUM1
7:2     RW          0       HID                                                             HID1
1       RW          1'b0    FW write 1 to set corresponding HID status clear to initial     SET_CHN_NUM_CLEAR1
0       RW          1'b0    FW write 1 to set total chain number of HID built by MCU1       SET_CHN_NUM_VALID1
                            to valid (after HW finish total data transfer for HID
                            , HW set to invalid internally)    

******************************************************/
#ifndef VT3514_C0
#define CHAIN_NUM1_REG_ADDR     (REG_BASE_SGE + 0x4c)
#define CHAIN_NUM2_REG_ADDR     (REG_BASE_SGE + 0x50)
#else
#define rSgqStatusExLo   (*(volatile U32 *)(REG_BASE_SGE + 0x4c))
#define rSgqStatusExHi   (*(volatile U32 *)(REG_BASE_SGE + 0x50))

#define CHAIN_NUM1_REG_ADDR     (REG_BASE_SGE + 0x54)
#define CHAIN_NUM2_REG_ADDR     (REG_BASE_SGE + 0x58)

#define DRQ_MCU0_STS_REG_ADDR   (REG_BASE_SGE + 0x80)
#define DWQ_MCU0_STS_REG_ADDR   (REG_BASE_SGE + 0x84)

#define rDrqMcu0Status   (*(volatile U32 *)DRQ_MCU0_STS_REG_ADDR)
#define rDwqMcu0Status   (*(volatile U32 *)DWQ_MCU0_STS_REG_ADDR)
#endif 
#define rChainNumMcu1    (*(volatile U32 *)CHAIN_NUM1_REG_ADDR)
#define rChainNumMcu2    (*(volatile U32 *)CHAIN_NUM2_REG_ADDR)

/* DRQ interface */
void HAL_DrqInit(void);
BOOL HAL_DrqIsFull(void);
BOOL HAL_DrqIsEmpty(void);
BOOL HAL_DrqBuildEntry(U8 HID, U16 FirstHsgId, U16 FirstDsgId);
void HAL_DwqInit(void);
BOOL HAL_DwqIsFull(void);
BOOL HAL_DwqIsEmpty(void);
BOOL HAL_DwqBuildEntry(U8 HID, U16 FirstHsgId, U16 FirstDsgId);

/* SGQ interface */
BOOL HAL_SgqInit(void);
BOOL HAL_SgqIsBusy(U8 ucPU);
//BOOL HAL_SgqBuildEntry(U8 HID, U16 FirstHsgId, U8 PU, U8 Level);
BOOL HAL_SgqBuildEntry(U8 HID, U16 FirstHsgId, U8 PU, BOOL bWriteEn);

/* SGE total chain number( DRQ/DWQ + SGQ ) */

void HAL_SgeInitChainCnt(void);
void HAL_SgeFinishChainCnt(U8 HID, U16 TotalChain);
void HAL_SgeHelpFinishChainCnt(U8 HID);
void HAL_SgeStopChainCnt(U8 HID);

/* On the fly buffer status report */
BOOL HAL_OtfbIsBusy(U8 PU);

void HAL_DrqWaitForCompleted();
void HAL_DwqWaitForCompleted();


#endif

