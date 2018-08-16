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
File name: sim_flash_common.c
Description: this file implements XTMP/Windows dependent functions
History:
    20130507, Gavin Yin, created 
*******************************************************************************/
#include "sim_flash_common.h"
#include "model_config.h"
#ifdef SIM_XTENSA
#include "xtmp_sysmem.h"
#include "xtmp_localmem.h"
#endif
#define  G_PAGE_SIZE ((1024*1024*1024)/ PG_SZ)

volatile NFC_CMD_STS_REG *p_flash_cq_dptr;
NF_CQ_REG_CMDTYPE *p_flash_cq_dptr_cmdtype;
/**added by vigoss zhang for pu acc 2013.7.9**/
NFC_LOGIC_PU *p_flash_pucr_reg;
NF_HPUST_REG *p_flash_hpust_reg;
NF_PPUST_REG *p_flash_ppust_reg;
NF_LPUST_REG *p_flash_lpust_reg;
NF_PUACC_TRIG_REG *p_flash_puacc_trig_reg;
//NF_PUFS_REG *p_flash_pufs_reg;
NF_PUFSB_REG *p_flash_pufsb_reg;
NFCQ_ENTRY *p_flash_cq_entry;
NFC_PRCQ_ENTRY  *p_flash_prcq_entry;
#ifdef VT3514_C0
FIRST_BYTE_AREA *g_pFirstByteArea;
#endif

#if defined(SIM_XTENSA)
extern U32* g_pDataBufferIn;
extern U32* g_pDataBufferOut;
extern CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
extern CRITICAL_SECTION g_PuAccCriticalSection;
extern void Mid_Un_Init();
extern U32* g_pDataBufferIn;
extern U32* g_pDataBufferOut;
extern CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
extern CRITICAL_SECTION g_PuAccCriticalSection;
#elif defined(SIM)
HANDLE g_SimNFCEvent;
HANDLE g_hNFCThread;
extern U32* g_pDataBufferIn;
extern U32* g_pDataBufferOut;
extern CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
extern CRITICAL_SECTION g_PuAccCriticalSection;
extern volatile BOOL g_bReSetFlag;
extern U32 SIM_DevGetStatus();
#else
#error "Please #define SIM_XTENSA or SIM in BaseDef.h"
#endif

U32 g_PG_SZ;//phy page size
U32 g_RED_SZ_DW;//red size for each phy page
U32 g_CACHE_STATUS_BASE;//offset in OTFB
U32 g_SSU_BASE;//offset in OTFB
U32 g_SSU1_BASE;
CRITICAL_SECTION g_PuBitmapCriticalSection;

LOCAL volatile BOOL l_bNFCModelExit = FALSE;



/*------------------------------------------------------------------------------
function: NFCQ_ENTRY* Comm_GetCQEntry(U8 pu, U8 rp);
Description: 
    get CQ entry
Input Param:
    U8 pu: CE num
    U8 rp: read pointer to HW queue
Output Param:
    void
Return Value:
    NFCQ_ENTRY*: pointer to one CQ entry
Usage:
    get CQ entry before NFC model to analysis CQ
History:
------------------------------------------------------------------------------*/
NFCQ_ENTRY* Comm_GetCQEntry(U8 PhyPu, U8 rp)
{
    U8 ucLogicPu;
    ucLogicPu = p_flash_pucr_reg->aNfcLogicPUReg[PhyPu%4][PhyPu/4].bsLogicPU;
    return &p_flash_cq_entry[(ucLogicPu * NFCQ_DEPTH) + rp];
}

/*------------------------------------------------------------------------------
function: NF_CQ_ENTRY* Comm_GetCQEntry(U8 pu, U8 rp);
Description: 
    get CQ entry
Input Param:
    U8 pu: CE num
    U8 rp: read pointer to HW queue
Output Param:
    void
Return Value:
    NF_CQ_ENTRY*: pointer to one CQ entry
Usage:
    get CQ entry before NFC model to analysis CQ
History:
------------------------------------------------------------------------------*/
NFC_PRCQ_ENTRY * Comm_GetPRCQEntry(U8 PhyPu, U8 Rp)
{
    U8 ucLogicPu;
    ucLogicPu = p_flash_pucr_reg->aNfcLogicPUReg[PhyPu%4][PhyPu/4].bsLogicPU;
    return &p_flash_prcq_entry[(ucLogicPu * NFCQ_DEPTH) + Rp];
}

#ifdef VT3514_C0
/*------------------------------------------------------------------------------
function: FIRST_BYTE_ENTRY * Comm_GetFirstByteEntry(U8 ucCE, U8 ucRp)
Description: 
    get address of first byte value entry
Input Param:
    U8 ucCE: CE num
    U8 ucRp: read pointer to HW queue
Output Param:
    void
Return Value:
    FIRST_BYTE_ENTRY *: pointer to first byte value entry in SRAM
Usage:
    supported in VT3514 C0 design only
    get CQ first byte value entry before NFC model to check first byte value
History:
------------------------------------------------------------------------------*/
FIRST_BYTE_ENTRY * Comm_GetFirstByteEntry(U8 ucCE, U8 ucRp)
{
    U8 ucLogicPu;
    ucLogicPu = p_flash_pucr_reg->aNfcLogicPUReg[ucCE%4][ucCE/4].bsLogicPU;
    return (FIRST_BYTE_ENTRY *)&g_pFirstByteArea->aFirstByteEntry[ucLogicPu][ucRp];
}
#endif

#if 0
void Comm_GetRedSize()
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
    switch((pNfcPgCfg->RedNum) & 0x03)
    {
    case 0x00:
        g_RED_SZ_DW = 4;
        break;
    case 0x01:
        g_RED_SZ_DW = 8;
        break;
    case 0x02:
        g_RED_SZ_DW = 12;
        break;
    default:
        g_RED_SZ_DW = 16;
        break;
    }

    return;
}
#endif

#if defined(SIM_XTENSA)
void NFC_ModelThreadXtmp()
{
    U8 ch;

    while(1){
        for (ch = 0; ch < NFC_CH_TOTAL ; ch++)
            NFC_ChannelSchedule(ch);

        XTMP_wait(1);
    }
}
#elif defined(SIM)
void NFC_ModelProcess()
{
    U8 ch;

    for (ch = 0; ch < NFC_CH_TOTAL ; ch++)
        NFC_ChannelSchedule(ch);
}

void NFC_ModelSchedule()
{
#ifdef NO_THREAD
    NFC_ModelProcess();
#else
    SetEvent(g_SimNFCEvent);
#endif
}

BOOL NFC_IsExit()
{ 
    return l_bNFCModelExit;
}

DWORD WINAPI NFC_ModelThread(LPVOID p)
{
    BOOL bContinue = TRUE;
    l_bNFCModelExit = FALSE;
    while (bContinue)
    {
    
        l_bNFCModelExit = g_bReSetFlag;
        if (FALSE == g_bReSetFlag)
        {
            l_bNFCModelExit = FALSE;
            WaitForSingleObject(g_SimNFCEvent,INFINITE);
            NFC_ModelProcess();
        }
    }

    return 0;
}
#endif

BOOL  NFC_ModelIsStartUp()
{
    return (!l_bNFCModelExit);
}

void NFC_ModelStartUp(void)
{
    
    
#if defined(SIM_XTENSA)
    XTMP_userThreadNew("NFC_ModelThreadXtmp", (XTMP_userThreadFunction)NFC_ModelThreadXtmp, NULL);

    //atexit(Mid_Un_Init);
     
#elif defined(SIM)
  #ifndef NO_THREAD
    g_SimNFCEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hNFCThread = CreateThread(0, 0, NFC_ModelThread, 0, 0, 0);
  #endif
#endif
}

/*------------------------------------------------------------------------------
function: void Comm_NFCModelInit(void);
Description: 
    NFC model init
Input Param:
    void
Output Param:
    void
Return Value:
    NF_CQ_ENTRY*: pointer to one CQ entry
Usage:
    get CQ entry before NFC model to analysis CQ
History:
------------------------------------------------------------------------------*/
void Comm_NFCModelInit(void)
{
    U8 ch;
    
#if defined(SIM_XTENSA)
    p_flash_cq_dptr = (NFC_CMD_STS_REG *)GetVirtualAddrInLocalMem(NFC_CMD_STS_BASE);    
    p_flash_cq_dptr_cmdtype = (NF_CQ_REG_CMDTYPE*)GetVirtualAddrInLocalMem(NFC_CMD_STS_BASE + (CE_MAX * sizeof(NFC_CMD_STS_REG)));
    /**added by vigoss zhang for pu acc**/
    p_flash_pucr_reg = (NFC_LOGIC_PU *)GetVirtualAddrInLocalMem(LOGIC_PU_REG_BASE);
    p_flash_hpust_reg = (NF_HPUST_REG *)GetVirtualAddrInLocalMem(HPUST_REG_BASE);
    p_flash_ppust_reg = (NF_PPUST_REG *)GetVirtualAddrInLocalMem(PPUST_REG_BASE);
    p_flash_lpust_reg = (NF_LPUST_REG *)GetVirtualAddrInLocalMem(LPUST_REG_BASE);
    p_flash_puacc_trig_reg = (NF_PUACC_TRIG_REG *)GetVirtualAddrInLocalMem(PUACC_TRIG_REG_BASE);
    p_flash_pufsb_reg = (NF_PUFSB_REG *)GetVirtualAddrInLocalMem(PUFSB_REG_BASE);     
    p_flash_prcq_entry= (NFC_PRCQ_ENTRY*)GetVirtualAddrInLocalMem(PRCQ_ENTRY_BASE);
    p_flash_cq_entry = (NFCQ_ENTRY *)GetVirtualAddrInLocalMem(CQ_ENTRY_BASE);
#ifdef VT3514_C0
    g_pFirstByteArea = (FIRST_BYTE_AREA *)GetVirtualAddrInLocalMem(EM_LBA_BASE);
#endif
#elif defined(SIM)
    p_flash_cq_dptr = (NFC_CMD_STS_REG *)NFC_CMD_STS_BASE;//in DRAM    
    p_flash_cq_dptr_cmdtype = (NF_CQ_REG_CMDTYPE*)(NFC_CMD_STS_BASE + (CE_MAX * sizeof(NFC_CMD_STS_REG)));
    /**added by vigoss zhang for pu acc at 2013.7.9**/ //in DRAM
    p_flash_pucr_reg = (NFC_LOGIC_PU *)LOGIC_PU_REG_BASE;
    p_flash_hpust_reg = (NF_HPUST_REG *)HPUST_REG_BASE;
    p_flash_ppust_reg = (NF_PPUST_REG *)(PPUST_REG_BASE);
    p_flash_lpust_reg = (NF_LPUST_REG *)LPUST_REG_BASE;
    p_flash_puacc_trig_reg = (NF_PUACC_TRIG_REG *)PUACC_TRIG_REG_BASE;
//  p_flash_pufs_reg = (NF_PUFS_REG *)PUFS_REG_BASE;
    p_flash_pufsb_reg = (NF_PUFSB_REG *)PUFSB_REG_BASE;
    
    //p_flash_raw_cmdq = (NF_RAWQ_ENTRY*)RAWQ_BASE;
    //p_flash_raw_cmdq_ext = (NF_RAWQ_ENTRY_EXT*)RAWQ_EXT_BASE;
    p_flash_prcq_entry= (NFC_PRCQ_ENTRY*)PRCQ_ENTRY_BASE;
    p_flash_cq_entry = (NFCQ_ENTRY*)CQ_ENTRY_BASE;
#ifdef VT3514_C0
    g_pFirstByteArea = (FIRST_BYTE_AREA *)EM_LBA_BASE;
#endif
    g_RED_SZ_DW = RED_SZ_DW;
    g_PG_SZ = PG_SZ;
    //g_CACHE_STATUS_BASE = (CACHE_STATUS_BASE_OTFB-OTFB_START_ADDRESS);//&~0xFFFF;
    //g_SSU_BASE = (SSU0_BASE_OTFB-OTFB_START_ADDRESS)&~0x3FF;
    //g_SSU1_BASE = (SSU1_BASE_OTFB-OTFB_START_ADDRESS)&~0xffff;
    g_SSU_BASE  = OTFB_SSU0_MCU1_BASE-OTFB_START_ADDRESS;            
    g_SSU1_BASE = OTFB_SSU1_MCU1_BASE-OTFB_START_ADDRESS;
    Mid_Init_Ex();
#endif

    //page size config
    g_pDataBufferIn = (U32 *)malloc(PIPE_PG_SZ);
    g_pDataBufferOut = (U32 *)malloc(PIPE_PG_SZ);

   
    InitializeCriticalSection(&g_PuBitmapCriticalSection);
    for(ch = 0;ch < NFC_CH_TOTAL;ch++)
    {
        InitializeCriticalSection(&g_CHCriticalSection[ch]);
    }
    InitializeCriticalSection(&g_PuAccCriticalSection);

    NFC_ModelParamInit();

#ifndef SIM
    {
        int nPu = CE_SUM;
        int nPln = PLN_PER_PU;
        int nBlk = BLK_PER_PLN;
        int nPge = PG_PER_BLK;

        unsigned long ulPgeSize = PG_SZ;
        unsigned long ulCapacity = ((CE_SUM * PLN_PER_PU * BLK_PER_PLN * PG_PER_BLK )/G_PAGE_SIZE);

        int nRsvPer = 2;

        Mid_Init(nPu, nPln, nBlk, nPge, (UINT32)ulCapacity, ulPgeSize, nRsvPer);
    }
#endif 

    NFC_ModelStartUp();
}

