/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : NfcDriver.c                                          
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Peterlgchen

Description: 
NFC driver will supply a SDK lib to do NFC hardware abstract layer. NFC driver 
will served for FTL algorithm, MPT tool, Hardware verification utilities and 
boot loader. To make the NFC driver higher efficiency, the parameters transfer 
rule use share data structure with registers mode. The whole NFC driver is 
based on a symmetrical NAND FLASH arrangement: all the chips in the system 
are the same, the same chip number for the channels. 

Depend file:
BaseDef.h
Config.h
NfcDriver.h 

Export file:
NfcDriver.h

Modification History:
20090522      Peterlgchen 001 first create
*************************************************/

#include "COM_Inc.h"

/************************************************
    Local Macro 
*************************************************/
#define NFCQ_ENTRY_SIZE_DW     (sizeof(NFCQ_ENTRY)/sizeof(U32))

/************************************************
    Raw Command table
*************************************************/

RCMD_TABLE l_aRCMD_TABLE[NF_RCMD_CNT];
const RCMD_TABLE aRCMD_TABLE[NF_RCMD_CNT] =
{
    {NF_RCMD_RESET,         3, FALSE, TRIG_CMD_OTHER, 0, INVALID_8F},
    {NF_RCMD_READID,        4, FALSE, TRIG_CMD_OTHER, 0, INVALID_8F},
    {NF_RCMD_READ_MLC_1PLN, 6, FALSE, TRIG_CMD_READ,  0, INVALID_8F},
    {NF_RCMD_READ_SLC_1PLN, 7, FALSE, TRIG_CMD_READ,  0, INVALID_8F},
};

/************************************************
    PRCQ sequence table
*************************************************/
const U8 l_aQETable[] = 
{
    // NF_RCMD_INDEX_RESET  
    // FF  | bsy | finish 
    QE_TYPE(PRCQ_ONE_CYCLE_CMD) |QE_INDEX(CQE1E_FF),
    QE_TYPE(PRCQ_READ_STATUS)   |QE_INDEX(0x0), 
    NF_PRCQ_CMD_FINISH,

    //NF_RCMD_INDEX_READID
    // 90 | addr x 1 | reg read |finish
    QE_TYPE(PRCQ_ONE_CYCLE_CMD) |QE_INDEX(CQE1E_90),
    QE_TYPE(PRCQ_ONE_CYCLE_ADDR)|QE_INDEX(0x0),
    QE_TYPE(PRCQ_REG_READ)      |QE_INDEX(0x0),
    NF_PRCQ_CMD_FINISH, 

    // NF_RCMD_INDEX_READ_1PLN
    // 00 | addr x 5 | 30  | bsy | 00 |data (pln0) | finish 

    QE_TYPE(PRCQ_TWO_PHASE_CMD)     |QE_INDEX(CQE2E_3000),
    QE_TYPE(PRCQ_FIVE_CYCLE_ADDR)   |QE_INDEX(0x0),
    QE_TYPE(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_TYPE(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_TYPE(PRCQ_DMA_READ)          |QE_INDEX(0x0),
    NF_PRCQ_CMD_FINISH,

    // NF_RCMD_INDEX_SLC_READ_1PLN
    // A2 | 00 | ROW ADDR(X3) EP  | 30 | BSY | 00 | DATA | FINISH
    QE_TYPE(PRCQ_ONE_CYCLE_CMD) |QE_INDEX(CQE1E_A2),
    QE_TYPE(PRCQ_TWO_PHASE_CMD) |QE_INDEX(CQE2E_3000),
    QE_TYPE(PRCQ_FIVE_CYCLE_ADDR) |QE_INDEX(0),
    QE_TYPE(PRCQ_READ_STATUS)   |QE_INDEX(0x0),
    QE_TYPE(PRCQ_ONE_CYCLE_CMD) |QE_INDEX(CQE1E_00),
    QE_TYPE(PRCQ_DMA_READ)      |QE_INDEX(0),
    NF_PRCQ_CMD_FINISH
} ;

const U8 ReadDataOut[] = 
{
    QE_TYPE(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    QE_TYPE(PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(CQE1E_00),
    QE_TYPE(PRCQ_DMA_READ)          |QE_INDEX(0x0),
    NF_PRCQ_CMD_FINISH
};

/************************************************
    PRCQ element entry table
*************************************************/

const U8 g_aCQE1E[QE_CMD_GROUP_DEPTH] = 
{
    0xEF,0xEE,0xFF,0xFA,
    0x90,0x00,0x30,0x60,
    0xD0,0x31,0x3F,0x85,
    0xFC,0xD4,0xD5,0xA2
};

const U16 g_aCQE2E[QE_CMD_GROUP_DEPTH] = 
{
    0x3000,0x1080,0x1181,0xD060,
    0x1180,0x1081,0xe005,0xe006,
    0x3060,0x1580,0x3200,0x3100,
    0xffff,0x3060,0xffff,0xffff
};

const U16 g_aSQEE[QE_OPRATION_GROUP_DEPTH] = 
{  
    0x3e70,0x2e70,0x1e70,0x0e70,
    0xbe70,0x7e70,0x3e70,0x3e78
};

/************************************************
    NFC initial table
*************************************************/
typedef struct NFC_REG_CONFIG_
{
    U32 NFC00 ;
    U32 NFC04 ;
    U32 NFC08 ;
    U32 NFC2C ;
    U32 NFC30 ;
    U32 NFC80 ;
    U32 NFC88 ;
    
    /* PMU 20 
     * 0x10 : HCLK 10M 
     * 0x50 : HCLK 50M
     * 0x90 : HCLK 100M 
     * 
     * */
    U32 PMU20 ;
    U32 RCmdIndex ;
}NFC_REG_CONFIG;
#ifdef VT3514_C0
const NFC_REG_CONFIG l_aNfcInitTable[8] =
{
    /* TOGGLE | MLC | SYNC */
    {
    /*
        0x60F8C92E,
        0x014C8124,  // fast mode
#ifdef FPGA        
        0x00d0e49C,
#else
        0x01d0e49C,
#endif
        0xFFFFFFFF,
        0x23000055,
        0xFFFFFFFF,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    */
        0x60F8C92E,
        0x41490BBC,
        0x44D16484,//0x44D16494,  this is good
        0xFFFFFFFF,
        0x38000000,
        0x2b2b2b2b,
        
        0x0A50Af00,
        0x90,
        NF_RCMD_READ_MLC_1PLN
    },

    /* TOGGLE | MLC | ASYNC */
    {
        0x60F8C92E,
        0x014c810d,   // fast mode 
#ifdef FPGA
        0x0010e49c,
#else
        0x0110e49c,
#endif
        0x0,
        0x23000055,
        0x0,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    },

    /* TOGGLE | TLC | SYNC */
    {
    /*
        0x60F8C94E,
        0x014C8124,
#ifdef FPGA        
        0x00d0e49C,
#else
        0x01d0e49C,
#endif        
        0xFFFFFFFF,
        0x23000055,
        0xFFFFFFFF,
        0xd0508900,
        0x50, 
        */
        0x60F8C94E,
        0x41490BBC,
        0x44D16484,//0x44D16494,  this is good
        0xFFFFFFFF,
        0x38000000,
        0x2b2b2b2b,      
        0x0A50Af00,
        0x90,

        NF_RCMD_READ_SLC_1PLN
    },
    
    /* TOGGLE | TLC | ASYNC */
    {
        0x60f8c94E,
        0x014c8125,
#ifdef FPGA         
        0x0010e49c,
#else
        0x0110e49c,
#endif        
        0x0,
        0x23000055,
        0x0,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    },

    /* ONFI | MLC | SYNC */
    {
        0x61F8C92E,
        0x014C8124,
#ifdef FPGA        
        0x00d0e49C,
#else
        0x01d0e49C,
#endif
        0x0,
        0x23000055,
        0xFFFFFFFF,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    },

    /* ONFI | MLC | ASYNC */
    {
       // 0x61F8C92E,
        0x6198cb2E,
        0x014c810d,
#ifdef FPGA
        0x0010e49c,           // FPGA clk is 25MHz , ECC clk is 25MHz
#else
        0x0110e49c,           // ASIC clk is 50MHz , ECC clk is 100MHz
#endif
        0x0,
        0x23000055,
        0x0,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN        
    },

    /* ONFI | TLC | SYNC */
    {
        0x61F8C94E,
        0x014C8124,
#ifdef FPGA        
        0x00d0e49C,
#else
        0x01d0e49C,
#endif
        0x0,
        0x23000055,
        0xFFFFFFFF,
        0xd0508900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    },
    /* ONFI | TLC | ASYNC */
    {
        0x61f8c94E,
        0x014c8125,
#ifdef FPGA  
        0x0010e49c,
#else
        0x0110e49c,
#endif        
        0x0,
        0x23000055,
        0x0,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    }
};
#else
const NFC_REG_CONFIG l_aNfcInitTable[8] =
{
    /* TOGGLE | MLC | SYNC */
    {
    /*
        0x60F8C92E,
        0x014C8124,  // fast mode
#ifdef FPGA        
        0x00d0e49C,
#else
        0x01d0e49C,
#endif
        0xFFFFFFFF,
        0x23000055,
        0xFFFFFFFF,
        0xd0d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    */
        0x60F8C92E,
        0x41490BBC,
        0x44D16484,//0x44D16494,  this is good
        0xFFFFFFFF,
        0x38000000,
        0x2b2b2b2b,
        
        0x0A50Af00,
        0x90,
        NF_RCMD_READ_MLC_1PLN

    },

    /* TOGGLE | MLC | ASYNC */
    {
        0x60F8C92E,
        0x814c810c,   // fast mode 
#ifdef FPGA
        0x00502494,
#else
        0x01502494,
#endif
        0x0,
        0x23000055,
        0x0,
        0x00d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    },

    /* TOGGLE | TLC | SYNC */
    {
        0x60F8C94E,
        0x014C8124,
        0x00d02494,
        0xFFFFFFFF,
        0x23000000,
        0xFFFFFFFF,
        0x00508900,
        0x50,  

        NF_RCMD_READ_SLC_1PLN
    },
    
    /* TOGGLE | TLC | ASYNC */
    {
        0x60f8c94E,
        0x814c8125,
        0x01502494,
        0x0,
        0x23000000,
        0x0,
        0x00d08900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    },

    /* ONFI | MLC | SYNC */
    {
        0x61F8C92E,
        0x014C8124,
        0x00902494,
        0x0,
        0x23000000,
        0xFFFFFFFF,
        0x00D08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN
    },

    /* ONFI | MLC | ASYNC */
    {
       // 0x61F8C92E,
        0x6198cb2E,
        0x814c810c,
#ifdef FPGA
        0x00102494,           // FPGA clk is 25MHz , ECC clk is 25MHz
#else
        0x01102494,           // ASIC clk is 50MHz , ECC clk is 100MHz
#endif
        0x0,
        0x23000055,
        0x0,
        0x00d08900,
        0x50,
        NF_RCMD_READ_MLC_1PLN        
    },

    /* ONFI | TLC | SYNC */
    {
        0x61F8C94E,
        0x014C8124,
        0x00902494,
        0x0,
        0x23000000,
        0xFFFFFFFF,
        0x00508900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    },
    /* ONFI | TLC | ASYNC */
    {
        0x61f8c94E,
        0x814c8125,
        0x01102494,
        0x0,
        0x23000055,
        0x0,
        0x00d08900,
        0x50,
        NF_RCMD_READ_SLC_1PLN
    }
};

#endif
/************************************************
                GLOBAL  value define
*************************************************/
volatile NFCQ_DPTR *pNFCQDptr;
volatile NFCQ_ARRAY *pNFCQArray;
volatile QEE_ENTRY_REG *pNfcQEEntry;
volatile PRCQ_ARRAY *pPrcqArray;
volatile RED_ENTRY *pRedEntry;
volatile NFCQ_TRIGGER_REG *pNfcTrigger;
GLOBAL U8 gPageReadCmdIndex;
LOCAL volatile FLASH_ID* const l_aFlashId = (volatile FLASH_ID*)(OTFB_START_ADDRESS);

/*
    extern function    
*/
extern void HAL_NormalDsgInit(void);

/************************************************
                Function define
*************************************************/
LOCAL  void cdcDelay(void)
{
    U32 i;
    i = 200;
    while(i--){
        __asm__("nop");
    }
}

LOCAL void HalNfcResetCmd(U8 Pu)
{
    pNFCQDptr->CQ_REG[Pu].bsRq0Pset = 1;
}

/************************************************
Function Name: HalNfcClearNfcq()
Input:      NFCQ_ENTRY*   point to NFCQ entry where will be cleared 
Output:  void
Description:
    Clear the NFCQ entry
*************************************************/

LOCAL void HalNfcClearNfcq(NFCQ_ENTRY* pNfcq)
{
    U32 i;
    for (i=0;i<NFCQ_ENTRY_SIZE_DW;i++)
    {
        *(((U32*)pNfcq) + i)  = 0;
    }
}


/************************************************
Function Name: HalNfcTimingInit()
Input:      void 
Output:  void  
Description:
    Config flash chip relative parameters.
    Strip pins (GLB_40[6:4]) are 3'b000 by default ,which means 
    [0] clear for Sync  ,set for async 
    [1] clear for mlc , set for tlc
    [2] clear for toshiba ,set for onfi 

    worm boot bit belongs to pmu scratch register 2 (PMU_2c),we define the bit[0]
    standing for worm boot . 
*************************************************/

void HalNfcTimingInit(void)
{
    gPageReadCmdIndex = NF_RCMD_READ_MLC_1PLN;
    if (FALSE == HAL_StrapIsSetHwFrEfuse())
    {
        NFC_REG_CONFIG* pReg ;
        U8 ulNfcIndex = HAL_StrapNfcType();

        DBG_Printf("NFC type is %d,",ulNfcIndex);
        DBG_Printf(" %s ",(TRUE == HAL_StrapNfcIsAsync())?"ASYNC":"SYNC");
        DBG_Printf(" %s ",(TRUE == HAL_StrapNfcIsOnfi())? "ONFI":"TOGGLE");
        DBG_Printf(" %s \n",(TRUE == HAL_StrapNfcIsTLC())?"TLC":"MLC");

        pReg = (NFC_REG_CONFIG*)&l_aNfcInitTable[ulNfcIndex];

        DBG_TRACE(TRACE_NFC_INIT);
        DBG_TRACE(ulNfcIndex);

        rNFC(0x0) = pReg->NFC00;
        rNFC(0x4) = pReg->NFC04;
        rNFC(0x8) = pReg->NFC08;
        rNFC(0x2c)= pReg->NFC2C;
        rNFC(0x30)= pReg->NFC30;
        rNFC(0x80)= pReg->NFC80;
        rNFC(0x88)= pReg->NFC88;
        rPMU(0x20)= pReg->PMU20;

        DBG_Printf("NFC 00 : 0x%x -- 0x%x\n",rNFC(0x0),pReg->NFC00);
        DBG_Printf("NFC 04 : 0x%x -- 0x%x\n",rNFC(0x4),pReg->NFC04);
        DBG_Printf("NFC 08 : 0x%x -- 0x%x\n",rNFC(0x8),pReg->NFC08);
        DBG_Printf("NFC 2c : 0x%x -- 0x%x\n",rNFC(0x2c),pReg->NFC2C);
        DBG_Printf("NFC 30 : 0x%x -- 0x%x\n",rNFC(0x30),pReg->NFC30);
        DBG_Printf("NFC 80 : 0x%x -- 0x%x\n",rNFC(0x80),pReg->NFC80);
        DBG_Printf("NFC 88 : 0x%x -- 0x%x\n",rNFC(0x88),pReg->NFC88);
        DBG_Printf("PMU 20 : 0x%x -- 0x%x\n",rPMU(0x20),pReg->PMU20);
        gPageReadCmdIndex = pReg->RCmdIndex;
    }    
}

/************************************************
Function Name: HalNfcInitInterface()
Input:      void 
Output:  void  
Description:
    Initialize pointers.
*************************************************/

LOCAL void HalNfcInitInterface(void)
{
    pNFCQDptr   =   (volatile NFCQ_DPTR*)   DPTR_BASE_ADDRESS;   
    pNFCQArray  =   (volatile NFCQ_ARRAY *) CQ_ENTRY_BASE;
    pPrcqArray  =   (volatile PRCQ_ARRAY *) PRCQ_ENTRY_BASE;
    //pRedEntry   =   (volatile RED_ENTRY *)  RED_BASE_ADDRESS; 
    pNfcTrigger =   (volatile NFCQ_TRIGGER_REG *) REG_BASE_NDC_TRIG;
    pNfcQEEntry =   (volatile QEE_ENTRY_REG*)PRCQ_QEE_BASE;

}

/************************************************
Function Name: HalFlashCmdTableInit()
Input:      void 
Output:  void  
Description:
    Calculate the pointers which point to the PRCQ sequence heads of commands.
*************************************************/

LOCAL void HalFlashCmdTableInit(void)
{
    U8 ucCmdIndex;
    U8 i;
    U32 ulCurQECnt = 0;
    U32 ulCurPioQECnt = 0;
    U32 Len = (sizeof(RCMD_TABLE)*NF_RCMD_CNT)/sizeof(U32);

    for (i=0; i<Len;i++)
    {
        *((U32*)l_aRCMD_TABLE + i) = *((U32*)aRCMD_TABLE + i);
    }
    
    for(ucCmdIndex = 0; ucCmdIndex <NF_RCMD_CNT; ucCmdIndex++)
    {
        if(FALSE == l_aRCMD_TABLE[ucCmdIndex].IsPIO)
        {
            l_aRCMD_TABLE[ucCmdIndex].QEPtr = (U32)&l_aQETable[ulCurQECnt];
            ulCurQECnt += l_aRCMD_TABLE[ucCmdIndex].QEPhase;
        }
    }
}

/************************************************
Function Name: HalFlashQEEInit()
Input:      void 
Output:  void  
Description:
    Initialize QEE registers 
*************************************************/

LOCAL void HalFlashQEEInit(void)
{
    U32 ulQeIndex;

    for (ulQeIndex = 0; ulQeIndex < QE_CMD_GROUP_DEPTH; ulQeIndex++)
    {
        pNfcQEEntry->PRCQ_CQE1E[ulQeIndex] = g_aCQE1E[ulQeIndex];
        pNfcQEEntry->PRCQ_CQE2E[ulQeIndex] = g_aCQE2E[ulQeIndex];
    }

    for (ulQeIndex = 0; ulQeIndex < QE_OPRATION_GROUP_DEPTH; ulQeIndex++)
    {
        pNfcQEEntry->PRCQ_SQEE[ulQeIndex] = g_aSQEE[ulQeIndex];
    }
}

/************************************************
Function Name: HalNfcInit()
Input:      void 
Output:  void  
Description:
    Initialize NFC 
    (1) Configurate page size ,redundant number,as so on
    (2) COnfigurate the NFC interface under asyn timming slow mode 
    (3) Initialize FW pointers 
    (4) Calculate the PRCQ sequence heads in PRCQ table
    (5) Initialize QEE register according to the QEE table 
*************************************************/

GLOBAL void HAL_NfcInit(void)
{
      
    HalNfcInitInterface();  
    HalFlashCmdTableInit();
    HalFlashQEEInit();
    HAL_NormalDsgInit();

#ifndef VT3514_C0
    rGLB(0x2c) &= ~0x1f;  // Remap buffer to OTFB start
#endif
}

LOCAL BOOL HalNfcGetPuEmpty(U8 Pu)
{
    return (BOOL)pNFCQDptr->CQ_REG[Pu].bsEmpty;
}



/************************************************
Function Name: HalNfcGetPuIdle()
Input:      Pu 
Output:  Status    -- TRUE  : Idle  ,FALSE : not idle  
Description:
    Get the idle bit value from access DPTR (CQ REG)
*************************************************/

LOCAL BOOL HalNfcGetPuIdle(U8 Pu)
{
    return (BOOL)pNFCQDptr->CQ_REG[Pu].bsIdle;
}

/************************************************
Function Name: HalNfcGetPuErr()
Input:      Pu 
Output:  Status    -- TRUE  : error  ,FALSE : no error  
Description:
    Get the error bit value from access DPTR (CQ REG)
*************************************************/

LOCAL BOOL  HalNfcGetPuErr(U8 Pu)
{
    return (BOOL)pNFCQDptr->CQ_REG[Pu].bsErrh;
}

/************************************************
Function Name: HalNfcGetPuFull()
Input:      Pu 
Output:  Status    -- TRUE  : Full  ,FALSE : not full
Description:
    Get the full bit value from access DPTR (CQ REG)
*************************************************/

LOCAL BOOL  HalNfcGetPuFull(U8 Pu)
{
    return (BOOL)(pNFCQDptr->CQ_REG[Pu].bsFull);
}

/************************************************
Function Name: HalNfcSinglePuStatus()
Input:      Pu 
Output:  Status    -- TRUE  : error  ,FALSE : no error  
Description:
    Waiting until idle bit be set,then return the error bit value
*************************************************/

LOCAL BOOL HalNfcSinglePuStatus(U8 Pu)
{
    while (TRUE != HalNfcGetPuIdle(Pu))
    {
        ;
    }
    cdcDelay();

    if (TRUE == HalNfcGetPuErr(Pu))
    {
        HalNfcResetCmd(Pu);
        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}

/************************************************
Function Name: HalNfcGetWp()
Input:      Pu 
Output:  Write pointer location  
Description:
    return the write pointer of target PU
*************************************************/

LOCAL U8 HalNfcGetWp(U8 Pu)
{
    return pNFCQDptr->CQ_REG[Pu].bsWrPtr;
}

/************************************************
Function Name: HalNfcGetNfcqEntry()
Input:      Pu 
Output:  pointer to available NFCQ entry   
Description:
    return the available NFCQ entry of target PU
*************************************************/

LOCAL NFCQ_ENTRY* HalNfcGetNfcqEntry(U8 Pu)
{
    U8 Wp = HalNfcGetWp(Pu);
    return (NFCQ_ENTRY*)&pNFCQArray->NfcqEntry[Pu][Wp];
}

/************************************************
Function Name: HalNfcGetPrcqEntry()
Input:      Pu 
Output:  pointer to available PRCQ entry   
Description:
    return the available PRCQ entry of target PU
*************************************************/

LOCAL PRCQ_ENTRY* HalNfcGetPrcqEntry(U8 Pu)
{
    U8 Wp = HalNfcGetWp(Pu);
    return (PRCQ_ENTRY*)&pPrcqArray->Array[Pu][Wp];
}

/************************************************
Function Name: HalSetPrcq()
Input:      Pu 
         CmdIndex   the index in raw command table 
Output:  void   
Description:
    Set the prcq entry of target PU as prcq table  
*************************************************/

LOCAL void HalSetPrcq(U8 CmdIndex,U8 Pu)
{
    U32 i,j,ulQePhase;
    U8 ucBuf[4] = {0};
    U8* pPrcqBuf = (U8*)l_aRCMD_TABLE[CmdIndex].QEPtr;
    volatile PRCQ_ENTRY* pPrcqEntry = HalNfcGetPrcqEntry(Pu);

    if ( (TRUE == HAL_StrapIsSetHwFrEfuse()) 
       &&(CmdIndex == NF_RCMD_READ_SLC_1PLN) 
       &&(CmdIndex == NF_RCMD_READ_MLC_1PLN))
    {
        
        ulQePhase = HAL_StrapGetReadPRCQ((U8*)&ucBuf);

        for (i=0;i<ulQePhase;i++)
        {
            pPrcqEntry->Entry[i] = ucBuf[i];
        }  

        for (j=0;j<sizeof(ReadDataOut);i++,j++)
        {
            pPrcqEntry->Entry[i] = ReadDataOut[j];
        }

    }
    else
    {      
        ulQePhase = l_aRCMD_TABLE[CmdIndex].QEPhase;
        for (i=0;i<ulQePhase;i++)
        {
            pPrcqEntry->Entry[i] = pPrcqBuf[i];
        }
    }
    return;
}

/************************************************
Function Name: HalGetTriggerEntry()
Input:      Pu 
Output:  U8 *   
Description:
    Get the trigger address
*************************************************/

LOCAL NFC_TRIGGER_REG* HalGetTriggerEntry(U8 Pu)
{
    U8 Wp = HalNfcGetWp(Pu);
    return (NFC_TRIGGER_REG*)&(pNfcTrigger->NfcqTriggerREG[Pu][Wp]);
}

/************************************************
Function Name: HalSetTrigger()
Input:      Pu 
         
Output:  void
Description:
    Get the trigger address
Note :
    the HW has a bug , the Trigger should be set only once for single command
    that means , if we set the trigger through pointer , as follow
    pTrigger->CmdType = 1;
    pTrigger->Trig = 1;
    the HW will be trigger twice and a exception would arise

    so here a structurt , tTrigger , be called as a buffer to solve the issue.
    
*************************************************/

LOCAL void HalSetTrigger(U8 CmdIndex,U8 Pu,U8 ucCeSel)
{
    volatile NFC_TRIGGER_REG *pTrig = HalGetTriggerEntry(Pu);
    NFC_TRIGGER_REG tTrig;
    tTrig.bsValue = 0;
    tTrig.bsCmdType = l_aRCMD_TABLE[CmdIndex].TrigType;
#ifndef VT3514_C0
    tTrig.bsTrig = TRUE;
#else
    tTrig.bsCESel = ucCeSel;
#endif  
    pTrig->bsValue = tTrig.bsValue;
    HAL_DelayCycle(5);
}
/*

*/
U8 HAL_NfcGetPhyPu(U8 ucLogPu)
{
    if (TRUE == HAL_StrapNfcIs16CeMode())
    {
        return ucLogPu >> 1;
    }
    else
    {
        return ucLogPu;
    }
}

U8 HAL_NfcGetCeSel(U8 ucLogPu)
{
    if (TRUE == HAL_StrapNfcIs16CeMode())
    {
        return ucLogPu & 0x1;
    }
    else
    {
        return 0;
    }   
}


/************************************************
Function Name: HalNfcSinglePuReset()
Input:      Pu 
Output:  BOOL  Error status    TRUE --  ERROR
                                FALSE -- NO ERROR
Description:
    Issue reset pu requeset  
*************************************************/

GLOBAL BOOL HAL_NfcResetPu(U8 ucLogPu)
{
    volatile NFCQ_ENTRY *pNfcqEntry;
    U8 ucPhyPu = HAL_NfcGetPhyPu(ucLogPu);
    U8 ucCeSel = HAL_NfcGetCeSel(ucLogPu);
    
    while (FALSE == HalNfcGetPuEmpty(ucPhyPu))
    {
        ;
    }

    pNfcqEntry = HalNfcGetNfcqEntry(ucPhyPu);
    HalNfcClearNfcq((NFCQ_ENTRY *)pNfcqEntry);

    HAL_MemoryWait();
    HalSetPrcq(NF_RCMD_RESET,ucPhyPu);
    HalSetTrigger(NF_RCMD_RESET,ucPhyPu,ucCeSel);
    return HalNfcSinglePuStatus(ucPhyPu);
}

/************************************************
Function Name: HAL_NfcReadId()
Input:   U8 Pu --- Pu Number
         U32 *pBuf --- Flash Id output buffer
Output:  Status
Description:
    Read
*************************************************/
GLOBAL BOOL HAL_NfcReadId(U8 ucLogPu)
{
    NFCQ_ENTRY *pNFCQEntry;
    BOOL bRet;

    U8 ucPhyPu = HAL_NfcGetPhyPu(ucLogPu);
    U8 ucCeSel = HAL_NfcGetCeSel(ucLogPu);

    while (FALSE == HalNfcGetPuEmpty(ucPhyPu))
    {
        ;
    }

    if (TRUE == HalNfcGetPuFull(ucPhyPu))
    {
        // FPGA only
        l_aFlashId[ucLogPu].ID[0] = 0;
        l_aFlashId[ucLogPu].ID[1] = 0;
        DBG_Printf("PU #%d Read ID: %x %x \n",ucLogPu,l_aFlashId[ucLogPu].ID[0],l_aFlashId[ucLogPu].ID[1]);
        return FAIL;
    }
    pNFCQEntry = HalNfcGetNfcqEntry(ucPhyPu);
    HalNfcClearNfcq((NFCQ_ENTRY *)pNFCQEntry);
    pNFCQEntry->bsDmaByteEn = TRUE;
    pNFCQEntry->bsByteAddr = 0x00;//For READ ID ,0x40 not use 
/*
    if (TRUE == HAL_StrapNfcIsOnfi())
    {
        pNFCQEntry->bsByteAddr = 0x00;
    }
    else
    {
        pNFCQEntry->bsByteAddr = 0x40;
    }
*/
    pNFCQEntry->bsByteLength = 8;
    HAL_MemoryWait();

    HalSetPrcq(NF_RCMD_READID,ucPhyPu);
    HalSetTrigger(NF_RCMD_READID,ucPhyPu,ucCeSel);

    while (TRUE != HalNfcGetPuIdle(ucPhyPu))
    {
        ;
    }

    if (TRUE == HalNfcGetPuErr(ucPhyPu))
    {
        return FAIL;
    }
    else
    {
        l_aFlashId[ucLogPu].ID[0] = rNfcReadID0;
        l_aFlashId[ucLogPu].ID[1] = rNfcReadID1;
        DBG_Printf("PU #%d Read ID: %x %x \n",ucLogPu,rNfcReadID0,rNfcReadID1);
        if ((0 == rNfcReadID0)&&(0 == rNfcReadID1))
        {
            return FAIL;
        }
        else
        {
            return SUCCESS;
        }
    }  
    return bRet;
}

LOCAL BOOL HAL_CheckSignature(U8 ulPu)
{
    if ((BOOT_LOADER_MAGIC_NUM_DW0 != *(U32*)BOOT_LOADER_BASE)
      ||(BOOT_LOADER_MAGIC_NUM_DW1 != *(U32*)(BOOT_LOADER_BASE+4)))
    {
        DBG_Printf("FLASH BOOT:\n");
        DBG_Printf("SIG DWORD 0:0x%x ,EXP :0x%x\n",*(U32*)BOOT_LOADER_BASE,BOOT_LOADER_MAGIC_NUM_DW0);
        DBG_Printf("SIG DWORD 1:0x%x ,EXP :0x%x\n",*(U32*)(BOOT_LOADER_BASE+4),BOOT_LOADER_MAGIC_NUM_DW1);
        DBG_TRACE(TRACE_FAIL);
        DBG_TRACE(*((U32*)OTFB_START_ADDRESS));
        DBG_TRACE(*((U32*)(OTFB_START_ADDRESS + 4)));        
        return FAIL;
    } 
    DBG_TRACE(TRACE_SUCCESS);
    DBG_TRACE(*((U32*)OTFB_START_ADDRESS));
    DBG_TRACE(*((U32*)(OTFB_START_ADDRESS + 4))); 
    return SUCCESS;
}


/*
FUNC:   BootLoaderRead
Input:  Pu  --- Physical PU 
        Dest -- into which bootloader be loaded  
Output: BOOL -- The error bit of dptr        
DES :   Read 8K data from the Page 0 in Block 0 of <Pu> into OTFB,
        by issuing the single plane read,      
*/

GLOBAL BOOL HAL_NfcReadBootloader(U8 ucLogPu)
{
    volatile NFCQ_ENTRY*       pNfcqEntry;
    volatile NORMAL_DSG_ENTRY* pDsgAddr;
    U8 ucPhyPu = HAL_NfcGetPhyPu(ucLogPu);
    U8 ucCeSel = HAL_NfcGetCeSel(ucLogPu); 
    U16 usCurDsgId;    
    U32 ulDataLen = HAL_StrapGetPageSize();
    while (FALSE == HalNfcGetPuEmpty(ucPhyPu))
    {
        ;
    }

    pNfcqEntry = (NFCQ_ENTRY*)HalNfcGetNfcqEntry(ucPhyPu);
    HalNfcClearNfcq((NFCQ_ENTRY* )pNfcqEntry);

    //pNfcqEntry->bsPuEnpMsk = TRUE;

#ifdef VT3514_C0
    pNfcqEntry->bsOtfbBypass = TRUE;
#else
    pNfcqEntry->bsTrigOmEn = TRUE;
#endif
    pNfcqEntry->bsOntfEn = TRUE;

    pNfcqEntry->aSecAddr[0].bsSecStart = 0;
    pNfcqEntry->aSecAddr[0].bsSecLength = ulDataLen >> SEC_SZ_BITS;
    pNfcqEntry->bsDmaTotalLength = ulDataLen >> SEC_SZ_BITS;
    pNfcqEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    
    pDsgAddr = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDsgAddr,sizeof(NORMAL_DSG_ENTRY));
    pDsgAddr->ulDramAddr = 0x0;
    pDsgAddr->bsXferByteLen = ulDataLen;
    pDsgAddr->bsLast = TRUE;
    HAL_MemoryWait();
    pNfcqEntry->bsFstDsgPtr = usCurDsgId;
    HAL_MemoryWait();
    HAL_SetNormalDsgSts(usCurDsgId,TRUE);
    HAL_MemoryWait();

    HalSetPrcq(gPageReadCmdIndex,ucPhyPu);
    HalSetTrigger(gPageReadCmdIndex,ucPhyPu,ucCeSel);
    HAL_MemoryWait();

    if (FAIL == HalNfcSinglePuStatus(ucPhyPu))
    {   
        DBG_TRACE(TRACE_FAIL);
        return FAIL;
    }
    else
    {
        DBG_TRACE(TRACE_SUCCESS);
        return HAL_CheckSignature(ucPhyPu);
    }
}

U32 HAL_NfcGetBootLoaderEntry(void)
{
    return BOOT_LOADER_ENTRY;
}


