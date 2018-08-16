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
Filename    : HAL_HCT.h 
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.07.14
Description : header file for defining HCT related HW interface in VT3514 
              AHCI mode
Others      : 
Modify      :
20140714    Gavin     Create file
20140912    Kristin    1. Coding style uniform
                       2. delete AHCI_REG_BASE_ADDRESS definition
*******************************************************************************/
#ifndef _HAL_HCT_H__
#define _HAL_HCT_H__

#ifdef HOST_NVME
#define WBQ_N    1
#else
#define WBQ_N    4
#endif

#define INVALID_CMD_ID    0xFF

#define HCT_DSRAM_BASE_ADDRESS  HCT_SRAM_BASE

/* HCT Register Definition */
#define HCT_DSBA_REG_BASE       REG_BASE_HCT
#define rHCT_CONTROL_REG        (*(volatile HCT_CONTROL_REG *)(REG_BASE_HCT + 0x14))
#define rHCT_FCQ_REG            (*(volatile HCT_FCQ_REG *)(REG_BASE_HCT + 0x20))
#define rHCT_WBQ_REG            (*(volatile HCT_WBQ_REG *)(REG_BASE_HCT + 0x24))
#define rHCT_CSCS_REG           (*(volatile HCT_CSCS_REG *)(REG_BASE_HCT + 0x28))
#define HCT_CS_REG              (REG_BASE_HCT + 0x30)
#define HCT_CS_REG_ADDRESS(_n_) (REG_BASE_HCT + 0x30 + (_n_))
#define rHCT_CS_REG             ((volatile U8 *)(REG_BASE_HCT + 0x30))
#define rHCT_CAFR_REG           (*(volatile U32 *)(REG_BASE_HCT + 0x70))  //PxCI auto fetch result

/* Register for unlock HCT/HostC reset in PCIe reset
    (Residing in PMU area), designed for C0 hardware only. */
#define rPMU_HCT_CLRRST         (*(volatile U8 *)(REG_BASE_PMU + 0x1D))
#define HOSTC_RESET_PENDING     (1 << 1)
#define HCT_RESET_PENDING       (1 << 2)

/* AHCI Register Definition */
#define rAHCI_CAP               (*(volatile U32*)(REG_BASE_AHCI + 0x00))
#define rAHCI_GHC               (*(volatile U32*)(REG_BASE_AHCI + 0x04))
#define rAHCI_IS                (*(volatile U32*)(REG_BASE_AHCI + 0x08))
#define rAHCI_PI                (*(volatile U32*)(REG_BASE_AHCI + 0x0C))
#define rAHCI_VS                (*(volatile U32*)(REG_BASE_AHCI + 0x10))
#define rAHCI_CCC_CTL           (*(volatile U32*)(REG_BASE_AHCI + 0x14))
#define rAHCI_CCC_PORTS         (*(volatile U32*)(REG_BASE_AHCI + 0x18))
#define rAHCI_EM_LOC            (*(volatile U32*)(REG_BASE_AHCI + 0x1C))
#define rAHCI_EM_CTL            (*(volatile U32*)(REG_BASE_AHCI + 0x20))
#define rAHCI_CAP2              (*(volatile U32*)(REG_BASE_AHCI + 0x24))
#define rAHCI_BOHC              (*(volatile U32*)(REG_BASE_AHCI + 0x28))

#define rP0_CLB                 (*(volatile U32*)(REG_BASE_AHCI + 0x100))
#define rP0_CLBU                (*(volatile U32*)(REG_BASE_AHCI + 0x104))
#define rP0_FB                  (*(volatile U32*)(REG_BASE_AHCI + 0x108))
#define rP0_FBU                 (*(volatile U32*)(REG_BASE_AHCI + 0x10C))
#define rP0_IS                  (*(volatile U32*)(REG_BASE_AHCI + 0x110))
#define rP0_IE                  (*(volatile U32*)(REG_BASE_AHCI + 0x114))
#define rP0_CMD                 (*(volatile U32*)(REG_BASE_AHCI + 0x118))
#define rP0_TFD                 (*(volatile U32*)(REG_BASE_AHCI + 0x120))
#define rP0_SIG                 (*(volatile U32*)(REG_BASE_AHCI + 0x124))
#define rP0_SSTS                (*(volatile U32*)(REG_BASE_AHCI + 0x128))
#define rP0_SCTL                (*(volatile U32*)(REG_BASE_AHCI + 0x12C))
#define rP0_SERR                (*(volatile U32*)(REG_BASE_AHCI + 0x130))
#define rP0_SACT                (*(volatile U32*)(REG_BASE_AHCI + 0x134))
#define rP0_CI                  (*(volatile U32*)(REG_BASE_AHCI + 0x138))
#define rP0_SNTF                (*(volatile U32*)(REG_BASE_AHCI + 0x13C))
#define rP0_FBS                 (*(volatile U32*)(REG_BASE_AHCI + 0x140))
#define rP0_VS                  (*(volatile U32*)(REG_BASE_AHCI + 0x170))

typedef union _HCT_BAINC_REG
{
    struct
    {
        U16 usBA;
        U16 bsINC   : 10;
        U16 bsRsvd  : 6;
    };

    U32 ulAsU32;
} HCT_BAINC_REG, *PHCT_BAINC_REG;

typedef union _HCT_CONTROL_REG
{
    struct
    {
        U32 bsRsvd0         : 8;
        U32 bsAUTOFCHCST    : 4;
        U32 bAUTOFCHEN      : 1;
        U32 bsRsvd1         : 3;
        U32 bCQFullChkPlcy  : 1;
        U32 bsRsvd2         : 15;
    };

    U32 ulAsU32;
} HCT_CONTROL_REG, *PHCT_CONTROL_REG;

typedef union _HCT_FCQ_REG
{
    struct
    {
        U16 usFCQBA;
        U8  bsFCQWP     : 5;
        U8  bsFCQEMPT   : 1;
        U8  bsFCQFULL   : 1;
        U8  bsFCQPUSH   : 1;  //RW1S
        U8  ucRsvd;
    };

    U32 ulAsU32;
} HCT_FCQ_REG, *PHCT_FCQ_REG;

typedef union _HCT_WBQ_REG
{
    struct
    {
        U16 usWBQBA;
        U8  bsWBQTRI    : 4;
        U8  bsRsvd      : 4;
        U8  ucWBQINC;
    };

    U32 ulAsU32;
} HCT_WBQ_REG, *PHCT_WBQ_REG;

typedef union _HCT_CSCS_REG
{
    struct
    {
        U8  bsCST       : 4;
        U8  bsCSTTRI    : 1;
        U8  bsRsvd0     : 3;
        U8  bsCSTCID    : 6;
        U8  bsRsvd1     : 2;
        U16 bsCSTRDY    : 1;
        U16 bsCSTNOID   : 1;
        U16 bsRsvd2     : 14;
    };

    U32 ulAsU32;
} HCT_CSCS_REG, *PHCT_CSCS_REG;

typedef struct _HCT_FCQ_WBQ
{
    /* DWORD 0 */
    U32 bsID    : 6;    //command slot id
    U32 bsRsvd  : 1;
    U32 bsIDB   : 1;    //ID base address enable
    U32 bsSN    : 3;
    U32 bsRsvd1 : 5;
    U32 bsOffset: 16;

    /* DWORD 1 */
    U32 ulHostAddrLow;

    /* DWORD 2 */
    U32 ulHostAddrHigh;  
                       
    /* DWORD 3 */      
    U32 bsNST       : 4;    //next status of corresponding entry
    U32 bsCST       : 4;    //current status of corresponding entry
    U32 bsClrCI     : 1;    //1:clear PxCI related bit (WBQ Only)
    U32 bsClrSACT   : 1;    //1:clear PxSACT related bit (WBQ Only)
    U32 bsUpdCCS    : 1;    //1:update PxCMD.CC (WBQ Only)
    U32 bsAsstIntr  : 1;    //update PxIS[3:0] (WBQ Only)
    U32 bsIntrType  : 2;    //WBQ Only
    U32 bsRegFirst  : 1;    //1:register update at first;0:register update at last (WBQ Only)
    U32 bsLast      : 1;
    U32 bsLength    : 12;   //in units of DWORD
    U32 bsWaitNfc   : 1;    //wait NFC program OK
    U32 bsWait      : 1;    //1:wait data transfer done (WBQ Only)
    U32 bsUpdate    : 1;    //1:update HCT status
    U32 bsCheck     : 1;    //1:check CST
}HCT_FCQ_WBQ, *PHCT_FCQ_WBQ;


typedef struct _NVME_WBQ
{
    /*DWORD 0*/
    U32 Id      : 6;    /*host command entry id*/
    U32 Rsvd    : 2;
    U32 Cqid    : 4; 
    U32 Rsvd1   : 4;
    U32 Sqid    : 16;

    /*DWORD 1*/
    U32 CmdSpec;

    /*DWORD 2*/
    //U32 HostAddrHigh;
    U16 CmdID;
    U16 Rsv1    : 1;
    U16 StatusF : 15;

    /*DWORD 3*/
    U32 NST     : 4;    /*next status of corresponding entry*/
    U32 CST     : 4;    /*current status of corresponding entry*/
    U32 RSV3    : 3;    /*1:clear PxCI related bit (WBQ Only)*/
    U32 R3      : 1;    /*1:clear PxSACT related bit (WBQ Only)*/
    U32 Rsv4    : 2;    /*1:update PxCMD.CC (WBQ Only)S*/
    U32 Rf      : 1;    /*update PxIS[3:0] (WBQ Only)*/
    U32 Last    : 1;    /* (WBQ Only) */

    U32 Rsv5    : 12;    
    U32 WNFC    : 1;
    U32 WSGE    : 1;
    U32 Update  : 1;    /*1:update HCT status*/
    U32 Rsv6    : 1;    /*1:check CST*/
}NVME_WBQ, *PNVME_WBQ;

typedef struct _HCT_INIT_PARAM
{
    /* DWORD 0 */
    struct
    {
        U32 ulBaseAddr;
    } tFCQParam;

    struct
    {
        /* DWORD 1 */
        U32 ulBaseAddr;

        /* DWORD 2 */
        U16 usTriggerState;
        U16 usIncrement;
    } tWBQParam;

    struct
    {
        /* DWORD 3 */
        U32 ulBaseAddr;

        /* DWORD 4 */
        U32 ulIncrement;
    } aSRAMTableParam[4];
} HCT_INIT_PARAM, *PHCT_INIT_PARAM;

typedef  volatile HCT_FCQ_WBQ (*PWBQTABLE)[WBQ_N];

void HAL_HCTInit(const HCT_INIT_PARAM *);
void HAL_HCTReset(void);
void HAL_HCTAssertReset(void);
void HAL_HCTReleaseReset(void);
void HAL_HCTClearAutoReset(void);
U32 HAL_HCTSetCST(U32, U32);
U32 HAL_HCTGetCST(U32);
U32 HAL_HCTSearchCST(U32);
volatile HCT_FCQ_WBQ *HAL_HCTGetFCQEntry(void);
volatile HCT_FCQ_WBQ *HAL_HCTGetWBQEntry(U32, U32);
void HAL_HCTPushFCQEntry(void);
void HAL_HCTTriggerWBQ(U32);
void HAL_TrigPCIeVDMsg(U32 ulMsgData);

#endif//_HAL_HCT_H__
