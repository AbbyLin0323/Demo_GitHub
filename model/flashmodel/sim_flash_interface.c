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

*Filename     : sim_flash_interface.c
*Version      :   Ver 1.0
*Date         :
*Author       :  Catherine Li

*Description:
*interface for visiting flash register

*Depend file:
*sim_flash_interface.h

*Modification History:
*20120209      Catherine Li 001 first create

*************************************************/
#include <windows.h>
#include "HAL_Inc.h"
#include "sim_flash_shedule.h"
#include "sim_flash_common.h"
#include "memory_access.h"
#include "sim_nfccmd.h"
#ifdef SIM_XTENSA
#include "XTMP_common.h"
#endif

#define SOFT_MAX_CW_NUM     4
#define SOFT_MAX_RD_NUM     5
#define SOFT_LLR_NUM        6
#define SOFT_DEC_FAIL_MAX   100
#define SOFT_DEC_FAIL

extern CRITICAL_SECTION g_CHCriticalSection[];
extern CRITICAL_SECTION g_PuAccCriticalSection;
extern CRITICAL_SECTION g_LogicLunStsCriticalSection;
#ifdef HOST_SATA
extern CRITICAL_SECTION g_BUFMAPCriticalSection;
#endif
//extern ST_CE_ERROR_ENTRY pu_err[CE_MAX];


extern NF_PPUST_REG *p_flash_ppust_reg;
extern void SDC_SetFirstReadDataReady(U8 uRegValue);
extern SOFT_DEC_DESCRIPTOR * MCU12_DRAM_TEXT HAL_SoftDecGetDescAddr(U32 DescID);
extern CRITICAL_SECTION g_PuBitmapCriticalSection;
extern GLOBAL volatile SOFT_MCU1_CFG_REG *g_pModelSoftMcu1CfgReg;
extern GLOBAL volatile SOFT_MCU2_CFG_REG *g_pModelSoftMcu2CfgReg;
extern GLOBAL volatile SOFT_INT_STS_REG  *g_pModelSoftIntStsReg;
extern U32 g_ulBufAddr;

U32 sim_bufmap[64];

BOOL bSoftDecorerTrigger = FALSE;
U32 g_ulModelSoftDecRptr = 0;
U32 g_ulModelSoftDecCnt = 0;
#if defined(SIM_XTENSA)
/************************************************
Function Name: nfcRegWrite
Input:     regaddr - register address
           regvalue - register value
Return:     None
Description:
nfc register write callback function
*************************************************/
BOOL nfcRegWrite(U32 regaddr, U32 regvalue, U32 len)
{
    U8 pu = 0;
    U8 offset;
    NFC_CMD_STS_REG *pCqRegDw;
    U8 wp = 0;
    BOOL bWtThru = TRUE;
    U16 *pTrigReg;
    static U32 temp_last_trigger = 0;
    static U32 temp_current_trigger = 0;
    NFC_CMD_STS_REG tNfcqReg;
    U32 * pNfcqReg;

    U32 ulRealValue;

    //dptr register
    if((regaddr >= NF_CMD_STS_REG_BASE)&&(regaddr < (NF_CMD_STS_REG_BASE + sizeof(NFC_CMD_STS) + sizeof(NFC_SIM_CMDTYPE))))
    {
        bWtThru = TRUE;//update to lcoal memory
        pu = (regaddr - NF_CMD_STS_REG_BASE) / sizeof(NFC_CMD_STS_REG);
        offset = (regaddr - NF_CMD_STS_REG_BASE) % sizeof(NFC_CMD_STS_REG);
        tNfcqReg = g_pModelNfcCmdSts->aCmdStsReg[pu];@todo
        pNfcqReg = (U32 *)&tNfcqReg;
        //write dw0
        if( (offset >= 0) && (offset <4) )
        {
            //~0x0 << (offset*8):wipe off low bits;  (~0x0 >> ((4-offset-len)*8)):wipe off high bits;
            ulRealValue = regvalue & (~0x0 << (offset*8));
            ulRealValue <<= ((4-offset-len)*8);
            ulRealValue >>= ((4-offset-len)*8);
            ulRealValue = ulRealValue | (*pNfcqReg);
            pCqRegDw = (NFC_CMD_STS_REG *)&ulRealValue;
            if(pCqRegDw->bsIntSts)
            {
                EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                g_pModelNfcCmdSts->aCmdStsReg[pu].bsIntSts = 0;@todo
                LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                bWtThru = FALSE;//update to lcoal memory
            }
            if(pCqRegDw->bsPrst)
            {
                EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                NFC_InterfaceResetCQ(pu, 0);
                p_flash_cq_dptr[pu].bsPrst = 0;
                LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                NFC_InterfaceUpdateHwPuSts();
                bWtThru = FALSE;//update to lcoal memory
            }
            if(pCqRegDw->bsRq0Pset)
            {
                EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                NFC_InterfaceResetCQ(pu, 1);
                p_flash_cq_dptr[pu].bsRq0Pset = 0;
                LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                NFC_InterfaceUpdateHwPuSts();
                bWtThru = FALSE;//update to lcoal memory
            }
        }
    }
    //logic pu status trigger register
    else if( regaddr == PUACC_TRIG_REG_BASE )
    {
        bWtThru = FALSE;

        if(regvalue&0x1)
        {
            //NFC_InterfaceUpdateLogicPuSts();
        }
    }
    //trig register
    else if((regaddr >= REG_BASE_TRIG_NDC) &&
        (regaddr < (REG_BASE_TRIG_NDC+ NFC_PU_MAX*sizeof(U16)*NFCQ_DEPTH)))
    {
        bWtThru = FALSE;

        //command trigger register
        pu = (regaddr - REG_BASE_TRIG_NDC) / (NFCQ_DEPTH * sizeof(NFC_TRIGGER_REG));
        wp = (regaddr - REG_BASE_TRIG_NDC) % (NFCQ_DEPTH * sizeof(NFC_TRIGGER_REG));
        offset = regaddr % sizeof(U32);
        pTrigReg = (U16 *)&regvalue;

      //  if(pTrigReg[offset]&0x80)//trig valid when trig bit is set
        {
            NFC_SendCMD(pu);
        }
    }
    //config register
    else if((regaddr >= REG_BASE_NDC) &&
        (regaddr < (REG_BASE_NDC + 0x80)))
    {
        bWtThru = TRUE;
        regaddr &= ~0x3;//make address 4 align
        if(regaddr == (U32)&rNfcPgCfg)
        {
            switch(regvalue & 0x03)
            {
            case 0x00:
                g_PG_SZ = 4<<10; //2048;
                break;
            case 0x01:
                g_PG_SZ = 8<<10; //4096;
                break;
            case 0x02:
                g_PG_SZ = 16<<10; //8192;
                break;
            case 0x03:
                g_PG_SZ = 32<<10; //16384;
                break;
            default:
                g_PG_SZ = 16<<10;//16384;
                break;
            }
            switch((regvalue>>8) & 0x03)
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
        }
        else if(regaddr == (U32)&rNfcSsu0Base)
        {
            g_SSU_BASE = regvalue;
        }
        else if(regaddr == (U32)&rNfcSsu1Base)
        {
            g_SSU1_BASE = regvalue;
        }
        else if(regaddr == (U32)&rNfcIntAcc)
        {
            bWtThru = FALSE;
            regvalue &= ~NF_INTACC_PENDING;
            Comm_WriteReg((U32)&rNfcIntAcc, 1, &regvalue);
        }
    }

    return bWtThru;
}
#endif
/*
this function is called when:
1) FW trigger a new command
2) nfc finish a command success
3) nfc finish a command with error

Note:
1) for fw convenience, not enabled pu report full status
*/
void NFC_InterfaceUpdateHwPuSts(void)
{
    U8 ucCh, ucPhyPuInCh;
    U8 ucPhyPuInTotalIdle  = 0;
    U8 ucPhyPuInTotalError = 0;
    U8 ucPhyPuInTotalFull  = 0;
    U8 ucPhyPuInTotalEmpty = 0;
    U8 aChAllEmpty[NFC_CH_TOTAL] = {0};
    U8 ucPhyPuInTotal;
    NFC_CMD_STS_REG *pTempCmdStsRegLun0 = NULL;

    EnterCriticalSection(&g_PuAccCriticalSection);
    //default hw pu status
    p_flash_ppust_reg->ulIdleBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulErrorBitMap = 0;
    p_flash_ppust_reg->ulNotFullBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulEmptyBitMap = 0xFFFFFFFF;

    for (ucCh = 0; ucCh < NFC_CH_TOTAL; ++ucCh)
    {
        aChAllEmpty[ucCh] = 1;

        for (ucPhyPuInCh = 0; ucPhyPuInCh < NFC_PU_PER_CH; ++ucPhyPuInCh)
        {
            if(p_flash_pucr_reg->aNfcLogicPUReg[ucCh][ucPhyPuInCh].bsPuEnable)
            {
                ucPhyPuInTotal     = ucPhyPuInCh * NFC_CH_TOTAL + ucCh;
                pTempCmdStsRegLun0 = (NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][0]);

                ucPhyPuInTotalIdle  =   pTempCmdStsRegLun0->bsIdle       & (pTempCmdStsRegLun0 + 1)->bsIdle
                                     & (pTempCmdStsRegLun0 + 2)->bsIdle  & (pTempCmdStsRegLun0 + 3)->bsIdle;
                ucPhyPuInTotalError =   pTempCmdStsRegLun0->bsErrh       | (pTempCmdStsRegLun0 + 1)->bsErrh
                                     | (pTempCmdStsRegLun0 + 2)->bsErrh  | (pTempCmdStsRegLun0 + 3)->bsErrh;
                ucPhyPuInTotalFull  =   pTempCmdStsRegLun0->bsFull       & (pTempCmdStsRegLun0 + 1)->bsFull
                                     & (pTempCmdStsRegLun0 + 2)->bsFull  & (pTempCmdStsRegLun0 + 3)->bsFull;
                ucPhyPuInTotalEmpty =   pTempCmdStsRegLun0->bsEmpty      | (pTempCmdStsRegLun0 + 1)->bsEmpty
                                     | (pTempCmdStsRegLun0 + 2)->bsEmpty | (pTempCmdStsRegLun0 + 3)->bsEmpty;

                p_flash_ppust_reg->ulIdleBitMap  &= ~(1 << ucPhyPuInTotal);
                p_flash_ppust_reg->ulIdleBitMap  |= (ucPhyPuInTotalIdle  << ucPhyPuInTotal);

                p_flash_ppust_reg->ulErrorBitMap &= ~(1 << ucPhyPuInTotal);
                p_flash_ppust_reg->ulErrorBitMap |= (ucPhyPuInTotalError << ucPhyPuInTotal);

                p_flash_ppust_reg->ulNotFullBitMap  &= ~(1 << ucPhyPuInTotal);
                p_flash_ppust_reg->ulNotFullBitMap  |= ((!ucPhyPuInTotalFull)  << ucPhyPuInTotal);

                p_flash_ppust_reg->ulEmptyBitMap &= ~(1 << ucPhyPuInTotal);
                p_flash_ppust_reg->ulEmptyBitMap |= (ucPhyPuInTotalEmpty << ucPhyPuInTotal);

                aChAllEmpty[ucCh] &= ucPhyPuInTotalEmpty;
            }
        }
    }

    p_flash_hpust_reg->bsAnyError = (p_flash_ppust_reg->ulErrorBitMap != 0)? 1 : 0;
    p_flash_hpust_reg->bsAllDone  = (p_flash_ppust_reg->ulIdleBitMap  == 0xFFFFFFFF)? 1 : 0;
    p_flash_hpust_reg->bsAllEmpty = (p_flash_ppust_reg->ulEmptyBitMap == 0xFFFFFFFF)? 1 : 0;

    p_flash_hpust_reg->bsCh0AllEmpty = aChAllEmpty[0];
    p_flash_hpust_reg->bsCh1AllEmpty = aChAllEmpty[1];
    p_flash_hpust_reg->bsCh2AllEmpty = aChAllEmpty[2];
    p_flash_hpust_reg->bsCh3AllEmpty = aChAllEmpty[3];

    LeaveCriticalSection(&g_PuAccCriticalSection);
    return;
}

void NFC_InterfaceUpdateLogicPuSts(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 ucPhyPuInTotalIdle  = 0;
    U8 ucPhyPuInTotalError = 0;
    U8 ucPhyPuInTotalFull  = 0;
    U8 ucPhyPuInTotalEmpty = 0;
    U8 logicPu = 0xFF;

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    NFC_CMD_STS_REG *pTempCmdStsRegLun0 = (NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][0]);

    EnterCriticalSection(&g_PuBitmapCriticalSection);
    if (p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsPuEnable)
    {
        ucPhyPuInTotalIdle  =   pTempCmdStsRegLun0->bsIdle       & (pTempCmdStsRegLun0 + 1)->bsIdle
                             & (pTempCmdStsRegLun0 + 2)->bsIdle  & (pTempCmdStsRegLun0 + 3)->bsIdle;
        ucPhyPuInTotalError =   pTempCmdStsRegLun0->bsErrh       | (pTempCmdStsRegLun0 + 1)->bsErrh
                             | (pTempCmdStsRegLun0 + 2)->bsErrh  | (pTempCmdStsRegLun0 + 3)->bsErrh;
        ucPhyPuInTotalFull  =   pTempCmdStsRegLun0->bsFull       & (pTempCmdStsRegLun0 + 1)->bsFull
                             & (pTempCmdStsRegLun0 + 2)->bsFull  & (pTempCmdStsRegLun0 + 3)->bsFull;
        ucPhyPuInTotalEmpty =   pTempCmdStsRegLun0->bsEmpty      | (pTempCmdStsRegLun0 + 1)->bsEmpty
                             | (pTempCmdStsRegLun0 + 2)->bsEmpty | (pTempCmdStsRegLun0 + 3)->bsEmpty;

        logicPu = p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsLogicPU;

        p_flash_lpust_reg->ulIdleBitMap  &= ~(1 << logicPu);
        p_flash_lpust_reg->ulIdleBitMap  |= (ucPhyPuInTotalIdle  << logicPu);

        p_flash_lpust_reg->ulErrorBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulErrorBitMap |= (ucPhyPuInTotalError << logicPu);

        p_flash_lpust_reg->ulNotFullBitMap  &= ~(1<<logicPu);
        p_flash_lpust_reg->ulNotFullBitMap  |= ((!ucPhyPuInTotalFull)  << logicPu);

        p_flash_lpust_reg->ulEmptyBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulEmptyBitMap |= (ucPhyPuInTotalEmpty << logicPu);
    }
    LeaveCriticalSection(&g_PuBitmapCriticalSection);
}

void NFC_UpdateLogicLunSts(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 logicPu = 0xFF;

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    NFC_CMD_STS_REG *pTempCmdStsReg = (NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu]);

    EnterCriticalSection(&g_LogicLunStsCriticalSection);
    if (p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsPuEnable)
    {
        logicPu = p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsLogicPU;

        g_pModelLogicLunStsReg->aIdleReg[logicPu / 8].ulBitMap &= ~(1 << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));
        g_pModelLogicLunStsReg->aIdleReg[logicPu / 8].ulBitMap |= (pTempCmdStsReg->bsIdle << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));

        g_pModelLogicLunStsReg->aErrorReg[logicPu / 8].ulBitMap &= ~(1 << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));
        g_pModelLogicLunStsReg->aErrorReg[logicPu / 8].ulBitMap |= (pTempCmdStsReg->bsErrh << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));

        g_pModelLogicLunStsReg->aNotFullReg[logicPu / 8].ulBitMap &= ~(1 << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));
        g_pModelLogicLunStsReg->aNotFullReg[logicPu / 8].ulBitMap |= ((!pTempCmdStsReg->bsFull) << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));

        g_pModelLogicLunStsReg->aEmptyReg[logicPu / 8].ulBitMap &= ~(1 << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));
        g_pModelLogicLunStsReg->aEmptyReg[logicPu / 8].ulBitMap |= (pTempCmdStsReg->bsEmpty << (((logicPu % 8) * NFC_LUN_MAX_PER_PU) + tNfcOrgStruct->ucLunInPhyPu));
    }
    LeaveCriticalSection(&g_LogicLunStsCriticalSection);
}

void NFC_UpdateLogicLunSwSts(volatile NFC_CMD_STS_REG *pCurCmdStsReg, U8 ucPhyPuInTotal, U8 ucLunInPhyPu, U32 ulUpdateBitmap)
{
    EnterCriticalSection(&g_aCSUptTLunSwBitmap);

    NF_LLUN_SW_BITMAP_REG tNfLunSwBitmapReg = *g_pModelLogicLunSwStsReg;

    if ((ulUpdateBitmap & 0x1) != 0)
    {
        tNfLunSwBitmapReg.aEmptyBitmap[ucLunInPhyPu] &= ~(1 << ucPhyPuInTotal);
        tNfLunSwBitmapReg.aEmptyBitmap[ucLunInPhyPu] |= (pCurCmdStsReg->bsEmpty) << ucPhyPuInTotal;
    }

    if ((ulUpdateBitmap & 0x2) != 0)
    {
        tNfLunSwBitmapReg.aNotFullBitmap[ucLunInPhyPu] &= ~(1 << ucPhyPuInTotal);
        tNfLunSwBitmapReg.aNotFullBitmap[ucLunInPhyPu] |= (!pCurCmdStsReg->bsFull) << ucPhyPuInTotal;
    }

    if ((ulUpdateBitmap & 0x4) != 0)
    {
        tNfLunSwBitmapReg.aIdleBitmap[ucLunInPhyPu] &= ~(1 << ucPhyPuInTotal);
        tNfLunSwBitmapReg.aIdleBitmap[ucLunInPhyPu] |= (pCurCmdStsReg->bsIdle) << ucPhyPuInTotal;
    }

    if ((ulUpdateBitmap & 0x8) != 0)
    {
        tNfLunSwBitmapReg.aErrorBitmap[ucLunInPhyPu] &= ~(1 << ucPhyPuInTotal);
        tNfLunSwBitmapReg.aErrorBitmap[ucLunInPhyPu] |= (pCurCmdStsReg->bsErrh) << ucPhyPuInTotal;
    }

    *g_pModelLogicLunSwStsReg = tNfLunSwBitmapReg;

    LeaveCriticalSection(&g_aCSUptTLunSwBitmap);

    return;
}
/************************************************
Function Name: NFC_InterfaceRecvBufferMapID
Input:     physical pu number
buffmap_id byte from real CQ dptr register
Output:
Description:
select bufmap id level for bufmap id setting
*************************************************/
void NFC_InterfaceRecvBufferMapID(U8 pu)
{
    //U8 WrPtr;
    //WrPtr = p_flash_cq_dptr[pu].bsWrPtr;

    //   flash_bmid_lvl[pu][wp].bmid = p_flash_cq_dptr[pu].bsBmid;
    //   flash_bmid_lvl[pu][wp].bmenw = p_flash_cq_dptr[pu].bsBmenw;
    //   flash_bmid_lvl[pu][wp].bmls = p_flash_cq_dptr[pu].bsBmls;
}

/************************************************
Function Name: NFC_InterfaceResetCQ
Input:     pu - physical pu number
Output:     None
Description:
reset command Queue
*************************************************/
void NFC_InterfaceResetCQ(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 level)
{
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U32 ulLunInTotal = ucPhyPuInTotal * NFC_LUN_PER_PU + tNfcOrgStruct->ucLunInPhyPu;

    EnterCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    volatile NFC_CMD_STS_REG *ptNfcCmdStsReg = (volatile NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu]);
    NFC_CMD_STS_REG tTempCmdStsReg = { 0 };

    tTempCmdStsReg.bsErrh = 0;
    tTempCmdStsReg.bsEmpty = 1;
    tTempCmdStsReg.bsErrSts = 0;
    tTempCmdStsReg.bsIntSts = 0;
    tTempCmdStsReg.bsIdle = 1;
    tTempCmdStsReg.bsRdPtr = level;
    tTempCmdStsReg.bsFull = 0;
    tTempCmdStsReg.bsWrPtr = level;
    tTempCmdStsReg.bsFsLv0 = 1;
    tTempCmdStsReg.bsFsLv1 = 1;
    tTempCmdStsReg.bsFsLv2 = 1;
    tTempCmdStsReg.bsFsLv3 = 1;

    *ptNfcCmdStsReg = tTempCmdStsReg;
    LeaveCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    NFC_UpdateLogicLunSwSts(ptNfcCmdStsReg, ucPhyPuInTotal, tNfcOrgStruct->ucLunInPhyPu, 0xF);

    return;
}

/************************************************
Function Name: NFC_GetEmpty
Input:     pu - physical pu number
Output:     None
Description:
check CQ empty
*************************************************/
BOOL NFC_GetEmpty(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    NFC_CMD_STS_REG *pTempCmdStsReg = (NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu]);

    return pTempCmdStsReg->bsEmpty;
}

/************************************************
Function Name: NFC_GetRP
Input:     pu - physical pu number
Output:     None
Description:
get cq read pointer
*************************************************/
U8 NFC_GetRP(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    return (U8)(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu].bsRdPtr);
}

/************************************************
Function Name: NFC_InterfaceCQWP
Input:     pu - physical pu number
Output:     None
Description:
get cq write pointer
*************************************************/
U8 NFC_InterfaceCQWP(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    ASSERT(tNfcOrgStruct != NULL);

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    return (U8)(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu].bsWrPtr);
}

U8 NFC_InterfaceGetFullStatus(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    ASSERT(tNfcOrgStruct != NULL);

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U8 nRegValue = g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu].bsFull;

    return nRegValue;
}

U8 NFC_GetErrH(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    ASSERT(tNfcOrgStruct != NULL);

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U8 nRegValue = g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu].bsErrh;

    return nRegValue;
}

/************************************************
Function Name: NFC_InterfaceJumpCQRP
Input:     pu - physical pu number
Output:     None
Description:
jump read pointer and reset full empty status
*************************************************/
void NFC_InterfaceJumpCQRP(const NFCM_LUN_LOCATION *ptNfcOrgStruct)
{
    ASSERT(NULL != ptNfcOrgStruct);

    U32 ulPhyPuInTotal = NfcM_GetPhyPuInTotal(ptNfcOrgStruct);
    U32 ulLunInTotal = ulPhyPuInTotal * NFC_LUN_PER_PU + ptNfcOrgStruct->ucLunInPhyPu;

    EnterCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    volatile NFC_CMD_STS_REG *pNfcCmdStsReg = (volatile NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ulPhyPuInTotal][ptNfcOrgStruct->ucLunInPhyPu]);
    NFC_CMD_STS_REG tTmpCmdStsReg = *pNfcCmdStsReg;

    switch (tTmpCmdStsReg.bsRdPtr)
    {
        case 0:
        {
            tTmpCmdStsReg.bsFsLv0 = 1;
            break;
        }
        case 1:
        {
            tTmpCmdStsReg.bsFsLv1 = 1;
            break;
        }
        case 2:
        {
            tTmpCmdStsReg.bsFsLv2 = 1;
            break;
        }
        case 3:
        {
            tTmpCmdStsReg.bsFsLv3 = 1;
            break;
        }
        default:
        {
            DBG_Getch();
        }
    }

    tTmpCmdStsReg.bsRdPtr = (tTmpCmdStsReg.bsRdPtr + 1) % NFCQ_DEPTH;
    tTmpCmdStsReg.bsEmpty = (tTmpCmdStsReg.bsRdPtr == tTmpCmdStsReg.bsWrPtr) ? 1 : 0;
    tTmpCmdStsReg.bsIdle = tTmpCmdStsReg.bsEmpty ? 1 : 0;
    tTmpCmdStsReg.bsFull = 0;

    *pNfcCmdStsReg = tTmpCmdStsReg;
    LeaveCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    NFC_UpdateLogicLunSwSts(pNfcCmdStsReg, ulPhyPuInTotal, ptNfcOrgStruct->ucLunInPhyPu, 0x7);
    return;
}

/************************************************
Function Name: NFC_InterfaceJumpCQWP
Input:     pu - physical pu number
Output:     None
Description:
jump CQ wp
*************************************************/
void NFC_InterfaceJumpCQWP(const NFCM_LUN_LOCATION *ptNfcOrgStruct)
{
    ASSERT(ptNfcOrgStruct != NULL);

    U32 ulPhyPuInTotal = NfcM_GetPhyPuInTotal(ptNfcOrgStruct);
    U32 ulLunInTotal = ulPhyPuInTotal * NFC_LUN_PER_PU + ptNfcOrgStruct->ucLunInPhyPu;

    EnterCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    volatile NFC_CMD_STS_REG *pNfcCmdStsReg = (volatile NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ulPhyPuInTotal][ptNfcOrgStruct->ucLunInPhyPu]);
    NFC_CMD_STS_REG tTmpCmdStsReg = *pNfcCmdStsReg;
    U8 ucCmdType = NfcM_GetTriggerReg(ptNfcOrgStruct, tTmpCmdStsReg.bsWrPtr)->bsCmdType;

    switch (tTmpCmdStsReg.bsWrPtr)
    {
        case 0:
        {
            tTmpCmdStsReg.bsFsLv0 = 0;
            tTmpCmdStsReg.bsCmdType0 = ucCmdType;
            break;
        }
        case 1:
        {
            tTmpCmdStsReg.bsFsLv1 = 0;
            tTmpCmdStsReg.bsCmdType1 = ucCmdType;
            break;
        }
        case 2:
        {
            tTmpCmdStsReg.bsFsLv2 = 0;
            tTmpCmdStsReg.bsCmdType2 = ucCmdType;
            break;
        }
        case 3:
        {
            tTmpCmdStsReg.bsFsLv3 = 0;
            tTmpCmdStsReg.bsCmdType3 = ucCmdType;
            break;
        }
        default:
        {
            DBG_Getch();
        }
    }

    tTmpCmdStsReg.bsWrPtr = (tTmpCmdStsReg.bsWrPtr + 1) % NFCQ_DEPTH;
    tTmpCmdStsReg.bsFull = (tTmpCmdStsReg.bsWrPtr == tTmpCmdStsReg.bsRdPtr) ? 1 : 0;
    tTmpCmdStsReg.bsEmpty = 0;
    tTmpCmdStsReg.bsIdle = tTmpCmdStsReg.bsErrh ? 1 : 0;

    *pNfcCmdStsReg = tTmpCmdStsReg;
    LeaveCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    NFC_UpdateLogicLunSwSts(pNfcCmdStsReg, ulPhyPuInTotal, ptNfcOrgStruct->ucLunInPhyPu, 0x7);
    return;
}

/************************************************
Function Name: NFC_InterfaceSetErrParameter
Input:     pu - physical pu number
nErrType - flash error type
Output:     None
Description:
input error parameter to CQ REG
*************************************************/
void NFC_InterfaceSetErrParameter(const NFCM_LUN_LOCATION *ptNfcOrgStruct, U8 nErrType)
{
    ASSERT(NULL != ptNfcOrgStruct);
    ASSERT(NF_SUCCESS != nErrType);

    U32 ulPhyPuInTotal = NfcM_GetPhyPuInTotal(ptNfcOrgStruct);
    U32 ulLunInTotal = ulPhyPuInTotal * NFC_LUN_PER_PU + ptNfcOrgStruct->ucLunInPhyPu;

    EnterCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    volatile NFC_CMD_STS_REG *pNfcCmdStsReg = (volatile NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ulPhyPuInTotal][ptNfcOrgStruct->ucLunInPhyPu]);
    NFC_CMD_STS_REG tTmpCmdStsReg = *pNfcCmdStsReg;

#if 0
    switch (tTmpCmdStsReg.bsRdPtr)
    {
        case 0:
        {
            tTmpCmdStsReg.bsFsLv0 = 1;
            break;
        }
        case 1:
        {
            tTmpCmdStsReg.bsFsLv1 = 1;
            break;
        }
        case 2:
        {
            tTmpCmdStsReg.bsFsLv2 = 1;
            break;
        }
        case 3:
        {
            tTmpCmdStsReg.bsFsLv3 = 1;
            break;
        }
        default:
        {
            DBG_Getch();
        }
    }
#endif

    tTmpCmdStsReg.bsErrh = TRUE;
    tTmpCmdStsReg.bsErrSts = nErrType;
    tTmpCmdStsReg.bsIdle = 1;

    *pNfcCmdStsReg = tTmpCmdStsReg;
    LeaveCriticalSection(&g_aCSUptTLunCmdSts[ulLunInTotal]);

    NFC_UpdateLogicLunSwSts(pNfcCmdStsReg, ulPhyPuInTotal, ptNfcOrgStruct->ucLunInPhyPu, 0xC);
    return;
}

/************************************************
Function Name: NFC_InterfaceSetFirstDataReady
Input:     tag -NCQ tag
Output:     None
Description:
set first data ready
*************************************************/
void NFC_InterfaceSetFirstDataReady(U8 tag)
{
#ifdef HOST_SATA
    SDC_SetFirstReadDataReady(((1<<7) | tag));
#endif
}

/************************************************
Function Name: NFC_InterfaceSetBuffermap
Input:    pu - physical pu number
            bufmap_value - buffer map value
Output:     None
Description:
set bufmap
*************************************************/
#ifdef HOST_SATA
void NFC_InterfaceSetBuffermap(U8 ucLunInTotal, U32 bufmap_value, U32 Bmid)
{
    U32 bufmap_value_old;
    U8 bufmap_id;

    bufmap_id = Bmid;

    EnterCriticalSection(&g_BUFMAPCriticalSection);
    bufmap_value_old = SDC_GetReadBufferMap(bufmap_id);
    bufmap_value |= bufmap_value_old;
    SDC_SetReadBufferMapWin(bufmap_id, bufmap_value);
    LeaveCriticalSection(&g_BUFMAPCriticalSection);
}
#endif

/************************************************
Function Name: NFC_InterfaceUpdateSsu
Input:     ssu_addr
ssu_value
Output:     None
Description:
ssu status update
*************************************************/
void NFC_InterfaceUpdateSsu(U16 ssu_addr, U8 value,U8 index, BOOL ontf)
{
    ASSERT(index < 2);
    ASSERT((ontf == TRUE) || (ontf == FALSE));

    U32 OtfbOffset;
    U32 DramOffset;

    if (ontf == TRUE)
    {
        OtfbOffset = g_ulModelSSUOffSetInOtfb + ssu_addr;
        Comm_WriteOtfbByByte(OtfbOffset, 1, &value);
    }
    else
    {
        DramOffset = g_ulModelSSUOffSetInDram + ssu_addr;
        Comm_WriteDramByByte(DramOffset, 1, &value);
    }

    return;
}

void NFC_InterfaceUpdateRedData(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 phyPageIndex;
    SIM_NFC_RED *pLocalRed;

    pLocalRed = (SIM_NFC_RED *)pFlashCmdParam->p_local_red;

    if (TRUE != pFlashCmdParam->p_nf_cq_entry->bsRawReadEn)
    {
        if (TRUE != pFlashCmdParam->red_only)
        {
            for (phyPageIndex = 0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
            {
                if (pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
                    pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[1] != 0)
                {
                    if (pFlashCmdParam->p_nf_cq_entry->bsRedOntf == TRUE)
                    {
                        Comm_WriteOtfb(pFlashCmdParam->p_red_addr + sizeof(U32) * (g_RED_SZ_DW * phyPageIndex),
                            g_RED_SZ_DW, &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]);

                    }
                    else
                    {
                        Comm_WriteDram(g_ulModelRedOffSetInDram + pFlashCmdParam->p_red_addr + sizeof(U32) * (g_RED_SZ_DW * phyPageIndex),
                            g_RED_SZ_DW, &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]);

                    }
                }
            }
        }
        else
        {
            for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->p_nf_cq_entry->bsDmaTotalLength + 1; phyPageIndex++)
            {
                if (pFlashCmdParam->p_nf_cq_entry->bsRedOntf == TRUE)
                {
                    Comm_WriteOtfb(pFlashCmdParam->p_red_addr + sizeof(U32)*(g_RED_SZ_DW * phyPageIndex),
                        g_RED_SZ_DW, &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]);

                }
                else
                {
                    Comm_WriteDram(g_ulModelRedOffSetInDram + pFlashCmdParam->p_red_addr + sizeof(U32) * (g_RED_SZ_DW * phyPageIndex),
                        g_RED_SZ_DW, &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]);
                }
            }
        }
    }
    else
    {
        for (phyPageIndex = 0; phyPageIndex < pFlashCmdParam->bsPlnNum; phyPageIndex++)
        {
            if (pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
                pFlashCmdParam->phy_page_req[phyPageIndex].sec_en[1] != 0)
            {
                Comm_WriteDram(g_ulBufAddr + g_PG_SZ, g_RED_SZ_DW, &pLocalRed->m_Content[g_RED_SZ_DW * phyPageIndex]);
            }
        }
    }

    return;
}

/************************************************
Function Name: NFC_InterfaceSetInt
Input:     physical pu number
Output:     None
Description:
trigger interrupt
*************************************************/
void NFC_InterfaceSetInt(const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    ASSERT(tNfcOrgStruct != NULL);

    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    NFC_CMD_STS_REG *pTempCmdStsReg = (NFC_CMD_STS_REG *)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][tNfcOrgStruct->ucLunInPhyPu]);

    U32 regValue;
    //wait for interrupt reg clear
    do{
        Comm_ReadReg((U32)&rNfcIntAcc, 1, &regValue);
    }while(regValue & NF_INTACC_PENDING);

    regValue |= NF_INTACC_PENDING;//set int pending
    regValue &= ~(NF_INTACC_PUMSK);

    regValue |= pTempCmdStsReg->bsRdPtr << 7;         //set RP of current PhyPU
    regValue |= tNfcOrgStruct->ucLunInPhyPu << 5;  //set LUN of current PhyPU
    regValue |= p_flash_pucr_reg->aNfcLogicPUReg[tNfcOrgStruct->ucCh][tNfcOrgStruct->ucPhyPuInCh].bsLogicPU; //logic PU

    Comm_WriteReg((U32)&rNfcIntAcc, 1, &regValue);

    pTempCmdStsReg->bsIntSts = 1;
    //call interrupt system function
#if defined(SIM_XTENSA)
    setInterrupt(10, 1);
#endif
    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcSetCmdType
Description:
    set corresponding cmd code before trigger, this is only need on XTMP and SIM ENV.
Input Param:
    U8 ucPhyPU: physical PU number.
    U8 ucWp: Wp
    U8 ucReqType: cmd request type
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set corresponding cmd code.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20151030    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
void NfcM_ISetCmdCode(U32 ulPhyPuInTotal, U32 ulLunInPhyPu, U32 ulLevel, U32 ulCommandCode)
{
    ASSERT(ulPhyPuInTotal < NFC_PU_MAX);
    ASSERT(ulLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);
    ASSERT(ulCommandCode < FLASH_PRCQ_CNT);

    g_aNfcModelCmdType[ulPhyPuInTotal][ulLunInPhyPu][ulLevel] = ulCommandCode;

    return;
}

void NfcM_ISetCmdMode(U32 ulPhyPuInTotal, U32 ulLunInPhyPu, U32 ulLevel, BOOL bSLCMode)
{
    ASSERT(ulPhyPuInTotal < NFC_PU_MAX);
    ASSERT(ulLunInPhyPu < NFC_LUN_PER_PU);
    ASSERT(ulLevel < NFCQ_DEPTH);

    g_aNfcModelCmdMode[ulPhyPuInTotal][ulLunInPhyPu][ulLevel] = bSLCMode;

    return;
}

//soft decoder trigger in sim
void NFC_SoftDecTrigger(U8 pu)
{
    EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
    //NFC_InterfaceJumpSoftDecCQWP(pu);
    bSoftDecorerTrigger = TRUE;
    LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
}
BOOL NFC_LLRValueCheck(SOFT_DEC_DESCRIPTOR *pSoftDecDesc)
{
    U8 ucIndex = 0, ucCnt = 0;
    U32 aArray[SOFT_LLR_NUM];

    aArray[0] = pSoftDecDesc->bsValueLLR0;
    aArray[1] = pSoftDecDesc->bsValueLLR1;
    aArray[2] = pSoftDecDesc->bsValueLLR2;
    aArray[3] = pSoftDecDesc->bsValueLLR3;
    aArray[4] = pSoftDecDesc->bsValueLLR4;
    aArray[5] = pSoftDecDesc->bsValueLLR5;

    while(ucIndex < (SOFT_LLR_NUM -1))
    {
        if(aArray[ucIndex] == aArray[ucIndex + 1])
        {
            DBG_Printf("LLR%d == LLR%d, value %d, %d", ucIndex, ucIndex + 1, aArray[ucIndex], aArray[ucIndex + 1]);
            DBG_Getch();
        }
        else
        {
            DBG_Printf("LLR value %d, %d", aArray[ucIndex], aArray[ucIndex + 1]);
        }
        ucIndex++;
        if((SOFT_LLR_NUM -2) == ucIndex)
        {
            ucCnt++;
            ucIndex = ucCnt;
        }
    }
    return TRUE;
}

/************************************************
Function Name: NFC_SoftDoderAnalyze
Input:     soft decoder descriptorr
Output:     NONE
Description: abalyze soft decoder descriptor
*************************************************/
void NFC_SoftDoderAnalyze(volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc)
{
    U32 ulSrcDramAddr, ulDstDramAddr;
    U32 ulCwLength = pSoftDecDesc->bsCwNum * CW_INFO_SZ;

    if(TRUE != pSoftDecDesc->bsValid)
    {
        DBG_Printf("pSoftDecDesc->bsValid = %d\n", pSoftDecDesc->bsValid);
        DBG_Getch();
    }

    if ((pSoftDecDesc->bsInfoLen1 < CW_INFO_SZ) || (pSoftDecDesc->bsInfoLen2 < CW_INFO_SZ) || (pSoftDecDesc->bsInfoLen3 < CW_INFO_SZ) || (pSoftDecDesc->bsInfoLen4 < CW_INFO_SZ))
    {
        DBG_Printf("Soft Descriptor Info Length is wrong.\n");
        DBG_Getch();
    }

    // CW number check
    if(pSoftDecDesc->bsCwNum > SOFT_MAX_CW_NUM)
    {
        DBG_Printf("Soft descriptor CWs overflow, CW count:%d\n", pSoftDecDesc->bsCwNum);
        DBG_Getch();
    }

    //LLR value check
    //NFC_LLRValueCheck(pSoftDecDesc);

#ifdef SOFT_DEC_FAIL
    if (20 == pSoftDecDesc->bsScrShfNum && g_ulModelSoftDecCnt < SOFT_DEC_FAIL_MAX)
    {

        pSoftDecDesc->bsFailed = TRUE;  // Fail
        pSoftDecDesc->bsDecSts = 0x8;   // 1110; 1:success, 0:fail
        pSoftDecDesc->bsBitMap = 0xF;   // 1111; mean 4cw are finish

        if (SOFT_MAX_RD_NUM == pSoftDecDesc->bsRdNum)
        {
            g_ulModelSoftDecCnt++;
        }
    }
    else
#endif
    {
        /* Note:
            If to do soft decode success, when the soft decode model need to copy data to target buffer.
            We need copy data first, because copy data need some time.
            Second, set fail bit is true before setting Cw bitmap.
            Because the firmware check soft decode descriptor done or not, will look Cw bitmap first.
            So Cw bitmap need to setting at the end.
        */

        // Data
        ulSrcDramAddr = pSoftDecDesc->aReadAddr[0] + DRAM_START_ADDRESS;
        ulDstDramAddr = pSoftDecDesc->ulWADDR + DRAM_START_ADDRESS;

        COM_MemCpy((U32*)ulDstDramAddr, (U32*)ulSrcDramAddr, ulCwLength / sizeof(U32));

        // Redundant
        if (pSoftDecDesc->bsCwId + pSoftDecDesc->bsCwNum >= (CW_PER_PLN - 1))
        {
            if (TRUE == pSoftDecDesc->bsRedunEn1 || TRUE == pSoftDecDesc->bsRedunEn2 || TRUE == pSoftDecDesc->bsRedunEn3 || TRUE == pSoftDecDesc->bsRedunEn4)
            {
                COM_MemCpy((U32*)(ulDstDramAddr + ulCwLength), (U32*)(ulSrcDramAddr + ulCwLength), g_RED_SZ_DW);
            }
        }

        pSoftDecDesc->bsCrcStatus1 = TRUE;
        pSoftDecDesc->bsCrcStatus2 = TRUE;
        pSoftDecDesc->bsCrcStatus3 = TRUE;
        pSoftDecDesc->bsCrcStatus4 = TRUE;

        pSoftDecDesc->bsFailed = FALSE;     // Success
        pSoftDecDesc->bsDecSts = 0xF >> (4 - pSoftDecDesc->bsCwNum);    // 1111
        pSoftDecDesc->bsBitMap = 0xF >> (4 - pSoftDecDesc->bsCwNum);    // 1111 mean 4cw are finish
    }

    return;
}

void NFC_LdpcSoftDecode(const NFCM_LUN_LOCATION *pNfcOrgStruct)
{
    volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc = NULL;

    if (FALSE == NFC_GetErrH(pNfcOrgStruct) &&
        (g_ulModelSoftDecRptr != g_pModelSoftMcu2CfgReg->bsWptr2))
    {
        pSoftDecDesc = HAL_LdpcSGetDescAddr(g_ulModelSoftDecRptr);//pSoftMcu2CfgReg->bsWptr2 - 1 //Should not use WPTR to get DescID
        NFC_SoftDoderAnalyze(pSoftDecDesc);
        g_ulModelSoftDecRptr = (g_ulModelSoftDecRptr + 1) % DESCRIPTOR_MAX_NUM;
    }

    return;
}
