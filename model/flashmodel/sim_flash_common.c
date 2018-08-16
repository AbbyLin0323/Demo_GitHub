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
#include "Disk_Config.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_DecStsReport.h"
#include <XORModel/Sim_XOR_Interface.h>


#ifdef SIM_XTENSA
#include "xtmp_sysmem.h"
#include "xtmp_localmem.h"
#endif
#define  G_PAGE_SIZE ((1024*1024*1024)/ LOGIC_PG_SZ)

GLOBAL volatile NFC_CMD_STS     *g_pModelNfcCmdSts;
GLOBAL volatile PG_CONF_REG     *g_pModelNfcPageConfigReg;
GLOBAL volatile NF_LLUNST_REG   *g_pModelLogicLunStsReg;
GLOBAL volatile NF_LLUN_SW_BITMAP_REG   *g_pModelLogicLunSwStsReg;
GLOBAL volatile NFCQ *g_pModelNfcq;
GLOBAL volatile NFC_TRIGGER *g_pModelNfcTriggerReg;
GLOBAL volatile DEC_STATUS_SRAM *g_pModelNfcDecSts;//Store LDPC status & flash ID & flash status in DEC STATUS SRAM
GLOBAL volatile DEC_FIFO_STATUS *g_pModelDecFifoSts;  //Store flash management status in DEC FIFO
GLOBAL volatile XOR_DEC_FIFO_CFG_REG *g_pModelDecFifoCfg;
GLOBAL volatile SOFT_MCU1_CFG_REG *g_pModelSoftMcu1CfgReg;
GLOBAL volatile SOFT_MCU2_CFG_REG *g_pModelSoftMcu2CfgReg;
GLOBAL volatile SOFT_INT_STS_REG  *g_pModelSoftIntStsReg;

/*Leo add for DEC & error injection driver bug */
GLOBAL NFC_CMD_STS_REG     g_pCurNfcCmdSts;

/* cmd type for nfc model use, SIM and XTMP env need */
U8 g_aNfcModelCmdType[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH];
BOOL g_aNfcModelCmdMode[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH];
U32 *g_pDataBufferIn;
U32 g_aDataBufferOut[PU_SUM][NFC_LUN_PER_PU][LOGIC_PIPE_PG_SZ / sizeof(U32)] = {0};

/**added by vigoss zhang for pu acc 2013.7.9**/
NFC_LOGIC_PU *p_flash_pucr_reg;
NF_HPUST_REG *p_flash_hpust_reg;
NF_PPUST_REG *p_flash_ppust_reg;
NF_LPUST_REG *p_flash_lpust_reg;
//NF_PUACC_TRIG_REG *p_flash_puacc_trig_reg;
//NF_PUFS_REG *p_flash_pufs_reg;
//NF_PUFSB_REG *p_flash_pufsb_reg;


#if defined(SIM_XTENSA)
extern CRITICAL_SECTION g_CHCriticalSection[];
extern CRITICAL_SECTION g_PuAccCriticalSection;
extern void Mid_Un_Init();
#elif defined(SIM)
HANDLE g_SimNFCEvent;
HANDLE g_hNFCThread;
extern CRITICAL_SECTION g_CHCriticalSection[];
extern CRITICAL_SECTION g_PuAccCriticalSection;
extern volatile BOOL g_bReSetFlag;
extern U32 SIM_DevGetStatus();
extern void NFC_LdpcSoftDecode(const NFCM_LUN_LOCATION *pNfcOrgStruct);
#else
#error "Please #define SIM_XTENSA or SIM in BaseDef.h"
#endif

U32 g_PG_SZ;//phy page size
U32 g_RED_SZ_DW;//red size for each phy page

CRITICAL_SECTION g_PuBitmapCriticalSection;
CRITICAL_SECTION g_LogicLunStsCriticalSection;


LOCAL volatile BOOL l_bNFCModelExit = FALSE;


void NfcM_OtherEmbeddedEngine(const NFCM_LUN_LOCATION *pNfcOrgStruct)
{
#if defined(LDPC_SOFT_DEC_TEST) || defined(UECC_SOFT_DECODE_EN)
    NFC_LdpcSoftDecode(pNfcOrgStruct);
#endif
    return;
}

BOOL NfcM_HaveCmdNeedProcess(const NFCM_LUN_LOCATION* pNfcOrgStruct)
{
    if (FALSE == NFC_GetErrH(pNfcOrgStruct) && FALSE == NFC_GetEmpty(pNfcOrgStruct))
    {
        if (NfcM_IsXorParityWriteCmd(pNfcOrgStruct, NFC_GetRP(pNfcOrgStruct)) == TRUE)
        {
            U32 ulXoreId = NfcM_GetXoreId(pNfcOrgStruct, NFC_GetRP(pNfcOrgStruct));

            return (TRUE == XORM_IIsParityReady(ulXoreId)) ? TRUE : FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}

/*------------------------------------------------------------------------------
function: NFCQ_ENTRY* COM_GetNFCQEntry(U8 pu, U8 rp);
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
NFCQ_ENTRY* COM_GetNFCQEntry(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 rp)
{
    U8 ucLogicPu = p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsLogicPU;
    U8 ucNfcLunMode  = 1 << g_pModelNfcPageConfigReg->bsMulLun;
    U8 ucNfcqDepthPerLun = ucNfcLunMode == 4 ? 2 : 4;

    if (ucNfcLunMode > 4)
    {
        DBG_Getch();
    }

    return (NFCQ_ENTRY*)&(g_pModelNfcq->aNfcqEntry[ucLogicPu][tNfcOrgStruct->ucLunInPhyPu][rp]);
}

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
    U8 ucCh, ucPhyPuInCh, ucLunInPhyPu;
    NFCM_LUN_LOCATION tNfcOrgStruct;

    for (ucLunInPhyPu = 0; ucLunInPhyPu < NFC_LUN_PER_PU; ++ucLunInPhyPu)
    {
        tNfcOrgStruct.ucLunInPhyPu = ucLunInPhyPu;

        for (ucPhyPuInCh = 0; ucPhyPuInCh < NFC_PU_PER_CH; ++ucPhyPuInCh)
        {
            tNfcOrgStruct.ucPhyPuInCh = ucPhyPuInCh;

            for (ucCh = 0; ucCh < NFC_CH_TOTAL; ++ucCh)
            {
                tNfcOrgStruct.ucCh = ucCh;

                NfcM_OtherEmbeddedEngine(&tNfcOrgStruct);

                if (TRUE == NfcM_HaveCmdNeedProcess(&tNfcOrgStruct))
                {
                    NfcM_CmdProcess(&tNfcOrgStruct);
                }
            }
        }
    }

    return;
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

BOOL NFC_IsReSet()
{
    return g_bReSetFlag;
}

DWORD WINAPI NFC_ModelThread(LPVOID p)
{
    while (TRUE)
    {
        if (FALSE == g_bReSetFlag)
        {
            l_bNFCModelExit = FALSE;
            WaitForSingleObject(g_SimNFCEvent,INFINITE);
            NFC_ModelProcess();
        }
        else
        {
            l_bNFCModelExit = TRUE;
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
    g_pModelNfcPageConfigReg = (NF_PG_CFG_REG *)GetVirtualAddrInLocalMem(NF_PG_CFG_REG_BASE);
    g_pModelNfcCmdSts = (NFC_CMD_STS *)GetVirtualAddrInLocalMem(NF_CMD_STS_REG_BASE);
    /**added by vigoss zhang for pu acc**/
    p_flash_pucr_reg = (NFC_LOGIC_PU *)GetVirtualAddrInLocalMem(LOGIC_PU_REG_BASE);
    p_flash_hpust_reg = (NF_HPUST_REG *)GetVirtualAddrInLocalMem(HPUST_REG_BASE);
    p_flash_ppust_reg = (NF_PPUST_REG *)GetVirtualAddrInLocalMem(PPUST_REG_BASE);
    p_flash_lpust_reg = (NF_LPUST_REG *)GetVirtualAddrInLocalMem(LPUST_REG_BASE);
    //p_flash_puacc_trig_reg = (NF_PUACC_TRIG_REG *)GetVirtualAddrInLocalMem(PUACC_TRIG_REG_BASE);
    //p_flash_pufsb_reg = (NF_PUFSB_REG *)GetVirtualAddrInLocalMem(PUFSB_REG_BASE);
    g_pModelLogicLunStsReg = (NF_LLUNST_REG *)GetVirtualAddrInLocalMem(LLUNST_REG_BASE);
    g_pModelLogicLunSwStsReg = (NF_LLUN_SW_BITMAP_REG*)GetVirtualAddrInLocalMem(NF_LLUNST_SW_REG_BASE);
    p_flash_prcq_entry= (NFC_PRCQ_ENTRY*)GetVirtualAddrInLocalMem(PRCQ_ENTRY_BASE);
    g_pModelNfcq = (NFCQ *)GetVirtualAddrInLocalMem(CQ_ENTRY_BASE);
    g_pModelNfcTriggerReg = (NFC_TRIGGER *)GetVirtualAddrInLocalMem(TRIGGER_REG_BASE);
    g_pModelNfcDecSts = (DEC_STATUS_SRAM *)GetVirtualAddrInLocalMem(DEC_STATUS_BASE);

#elif defined(SIM)

    g_pModelNfcPageConfigReg = (PG_CONF_REG *)NF_PUBLIC_REG_BASE;
    g_pModelNfcCmdSts = (NFC_CMD_STS *)NF_CMD_STS_REG_BASE;//in DRAM
    /**added by vigoss zhang for pu acc at 2013.7.9**/ //in DRAM
    p_flash_pucr_reg = (NFC_LOGIC_PU *)NF_LOGIC_PU_REG_BASE;
    p_flash_hpust_reg = (NF_HPUST_REG *)NF_HPUST_REG_BASE;
    p_flash_ppust_reg = (NF_PPUST_REG *)NF_PPUST_REG_BASE;
    p_flash_lpust_reg = (NF_LPUST_REG *)NF_LPUST_REG_BASE;
    //p_flash_puacc_trig_reg = (NF_PUACC_TRIG_REG *)NF_LPS_TRIG_REG_BASE;
    g_pModelLogicLunStsReg = (NF_LLUNST_REG *)NF_LLUNST_REG_BASE;
    g_pModelLogicLunSwStsReg = (NF_LLUN_SW_BITMAP_REG*)NF_LLUNST_SW_REG_BASE;

    g_pModelNfcq = (NFCQ *)NFCQ_ENTRY_BASE;
    g_pModelNfcTriggerReg = (NFC_TRIGGER *)NF_TRIGGER_REG_BASE;
    g_pModelNfcDecSts = (DEC_STATUS_SRAM *)DEC_STATUS_BASE;
    g_pModelDecFifoSts = (DEC_FIFO_STATUS *)DEC_STS_FIFO_BASE;
    g_pModelDecFifoCfg = (XOR_DEC_FIFO_CFG_REG *)DEC_FIFO_CFG_REG;

    g_pModelSoftMcu1CfgReg = (SOFT_MCU1_CFG_REG *)LDPC_SOFT_MCU1_CFG_REG_BASE;
    g_pModelSoftMcu2CfgReg = (SOFT_MCU2_CFG_REG *)LDPC_SOFT_MCU2_CFG_REG_BASE;
    g_pModelSoftIntStsReg = (SOFT_INT_STS_REG  *)LDPC_SOFT_INT_STS_REG;

    g_RED_SZ_DW = RED_SZ_DW;
    g_PG_SZ = LOGIC_PG_SZ;

    Mid_Init_Ex();
#endif

    //page size config
    g_pDataBufferIn = (U32 *)malloc(LOGIC_PIPE_PG_SZ);

    InitializeCriticalSection(&g_PuBitmapCriticalSection);
    for(ch = 0;ch < NFC_CH_TOTAL;ch++)
    {
        InitializeCriticalSection(&g_CHCriticalSection[ch]);
    }
    InitializeCriticalSection(&g_PuAccCriticalSection);
    InitializeCriticalSection(&g_LogicLunStsCriticalSection);

    U32 ulTLun;
    for (ulTLun = 0; ulTLun < NFC_MODEL_LUN_SUM; ulTLun++)
    {
        InitializeCriticalSection(&g_aCSUptTLunCmdSts[ulTLun]);
    }
    InitializeCriticalSection(&g_aCSUptTLunSwBitmap);

    NFC_ModelParamInit();

#ifndef SIM
    {
        int nPu = NFC_MODEL_LUN_SUM;
        int nPln = PLN_PER_LUN;
        int nBlk = BLK_PER_PLN;
        int nPge = PG_PER_BLK;

        unsigned long ulPgeSize = LOGIC_PG_SZ;
        unsigned long ulCapacity = ((NFC_MODEL_LUN_SUM * PLN_PER_LUN * BLK_PER_PLN * PG_PER_BLK )/G_PAGE_SIZE);

        int nRsvPer = 2;

        Mid_Init(nPu, nPln, nBlk, nPge, (UINT32)ulCapacity, ulPgeSize, nRsvPer);
    }
#endif

    NFC_ModelStartUp();

}

const volatile NFC_TRIGGER_REG * NfcM_GetTriggerReg(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel)
{
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    U32 ulNfcPuInTotal = pNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + pNfcOrgStruct->ucCh;

    return &(g_pModelNfcTriggerReg->aNfcTriggerReg[ulNfcPuInTotal][pNfcOrgStruct->ucLunInPhyPu][ulLevel]);
}

const U8 NFC_GetCmdCode(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel)
{
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    U8 ucPhyPuInTotal = pNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + pNfcOrgStruct->ucCh;

    return g_aNfcModelCmdType[ucPhyPuInTotal][pNfcOrgStruct->ucLunInPhyPu][ulLevel];
}

const BOOL NFC_GetCmdMode(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel)
{
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    U8 ucPhyPuInTotal = pNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + pNfcOrgStruct->ucCh;

    return g_aNfcModelCmdMode[ucPhyPuInTotal][pNfcOrgStruct->ucLunInPhyPu][ulLevel];
}

void NfcM_CheckFirmwareError(NFCM_CHK_FW_ERR_TYPE eCheckFirmwareErrorType, const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel)
{
    ASSERT((eCheckFirmwareErrorType >= 0) && (eCheckFirmwareErrorType < NFCM_CHK_FW_ERR_TYPE_ASSERT));
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    switch (eCheckFirmwareErrorType)
    {
    case NFCM_CHK_XOR_ON:
    {
        NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pNfcOrgStruct, ulLevel);

        if (pNfcqEntry->bsXorEn == FALSE)
        {
            DBG_Getch();    // Firmware config error, XOR enable bit should be set 1.
        }

        break;
    }
    case NFCM_CHK_XOR_OFF:
    {
        NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pNfcOrgStruct, ulLevel);

        if (pNfcqEntry->bsXorEn == TRUE)
        {
            DBG_Getch();    // Firmware config error, XOR enable bit should be set 0.
        }

        break;
    }
    default:
        DBG_Getch();
    }

    return;
}
// For write command, if all its data or part of its data is XOR parity of page,
// then we call it a XOR parity write command.
BOOL NfcM_IsXorParityWriteCmd(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel)
{
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    U32 ulChannelNum = pNfcOrgStruct->ucCh;
    U32 ulNfcPuNumInCh = pNfcOrgStruct->ucPhyPuInCh;
    U32 ulLunNumInNfcPu = pNfcOrgStruct->ucLunInPhyPu;
    U32 ulNfcPuInTotal = ulNfcPuNumInCh * NFC_CH_TOTAL + ulChannelNum;

    U32 ulCmdType = NfcM_GetTriggerReg(pNfcOrgStruct, ulLevel)->bsCmdType;
    U32 ulExtCmd = NfcM_GetTriggerReg(pNfcOrgStruct, ulLevel)->bsExtCmd;

    if ((ulCmdType == TRIG_CMD_WRITE) && (ulExtCmd == 1))
    {
        NfcM_CheckFirmwareError(NFCM_CHK_XOR_ON, pNfcOrgStruct, ulLevel);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Plane count of the specific command equal to 1 when the command is single plane write, equal to
// 2, 4 or other valid value when the command is full plane write.
U32 NfcM_GetPlaneCount(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level)
{
    ASSERT(lun_location != NULL);
    return (1 << COM_GetNFCQEntry(lun_location, cmd_level)->bsTlcPlnNum);
}

U32 NfcM_Get1stAtomRedunOffset(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level)
{
    ASSERT(lun_location != NULL);
    return (COM_GetNFCQEntry(lun_location, cmd_level)->bsRedAddr << 3);
}

BOOL NfcM_IsXorParityPageInWriteCmd(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level, U32 plane_index)
{
    ASSERT(lun_location != NULL);
    ASSERT(lun_location->ucCh < NFC_CH_TOTAL);
    ASSERT(lun_location->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(lun_location->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(cmd_level < NFCQ_DEPTH);
    ASSERT(plane_index < PLN_PER_LUN * INTRPG_PER_PGADDR);

    U32 plane_count = NfcM_GetPlaneCount(lun_location, cmd_level);

    if (NfcM_IsXorParityWriteCmd(lun_location, cmd_level) == TRUE)
    {
        // XOR parity page always is the last plane of LUN.
        if (plane_index == (plane_count - 1))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

// Data size of this write command is a single plane page, and all of data is XOR parity.
BOOL NfcM_Is1PlnXorParityWrite(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level)
{
    ASSERT(lun_location != NULL);
    ASSERT(lun_location->ucCh < NFC_CH_TOTAL);
    ASSERT(lun_location->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(lun_location->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(cmd_level < NFCQ_DEPTH);

    U32 plane_count = NfcM_GetPlaneCount(lun_location, cmd_level);

    if ((NfcM_IsXorParityWriteCmd(lun_location, cmd_level) == TRUE) && (plane_count == 1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL NfcM_IsFullPlaneXorParityPageInWriteCmd(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level) 
{
    ASSERT(lun_location != NULL);
    ASSERT(lun_location->ucCh < NFC_CH_TOTAL);
    ASSERT(lun_location->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(lun_location->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(cmd_level < NFCQ_DEPTH);

    U32 plane_count = NfcM_GetPlaneCount(lun_location, cmd_level);

    if ((NfcM_IsXorParityWriteCmd(lun_location, cmd_level) == TRUE) && (plane_count > 1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

NFCM_PAGE_DATA_LOCATION NfcM_GetPageDataLocation(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel, U32 ulPlaneIndex)
{
    ASSERT(pNfcOrgStruct != NULL);
    ASSERT(pNfcOrgStruct->ucCh < NFC_CH_TOTAL);
    ASSERT(pNfcOrgStruct->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(pNfcOrgStruct->ucLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);
    ASSERT(ulPlaneIndex < (PLN_PER_LUN * INTRPG_PER_PGADDR));

    NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(pNfcOrgStruct, ulLevel);

    if (TRUE == NfcM_IsXorParityPageInWriteCmd(pNfcOrgStruct, ulLevel, ulPlaneIndex))
    {
        return NFCM_PAGE_DATA_IN_OTFB;
    }
    else
    {
        if (pNfcqEntry->bsOntfEn == TRUE)
        {
            // Nowadays, if user data of command is stored in OTFB SRAM, then this command can't use XOR.
            NfcM_CheckFirmwareError(NFCM_CHK_XOR_OFF, pNfcOrgStruct, ulLevel);
            return NFCM_PAGE_DATA_IN_OTFB;
        }
        else
        {
            return NFCM_PAGE_DATA_IN_DRAM;
        }
    }
}

U32 NfcM_GetXoreId(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level)
{
    ASSERT(lun_location != NULL);
    ASSERT(lun_location->ucCh < NFC_CH_TOTAL);
    ASSERT(lun_location->ucPhyPuInCh < NFC_PU_PER_CH);
    ASSERT(lun_location->ucLunInPhyPu < NFC_LUN_PER_PU);

    U32 ulXoreId = 0;

    if (NfcM_IsXorParityWriteCmd(lun_location, cmd_level) == TRUE)
    {
        ulXoreId = NfcM_GetTriggerReg(lun_location, cmd_level)->bsExtCmdSel;
    }
    else
    {
        ulXoreId = COM_GetNFCQEntry(lun_location, cmd_level)->bsXorId;
    }

    return ulXoreId;
}

U32 NfcM_GetPhyPuInTotal(const NFCM_LUN_LOCATION *lun_location)
{
    return (lun_location->ucPhyPuInCh * NFC_CH_TOTAL + lun_location->ucCh);
}

//#ifdef FLASH_TLC
BOOL NfcM_Is6DsgIssueCmd(U32 ulCmdCode)
{
    if ((ulCmdCode >= NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN) && (ulCmdCode <= NF_PRCQ_TLC_PRG_1ST_UP_MULTIPLN))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
//#endif
