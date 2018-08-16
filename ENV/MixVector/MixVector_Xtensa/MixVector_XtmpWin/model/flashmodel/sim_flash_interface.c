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
#ifdef SIM_XTENSA
#include "XTMP_common.h"
#endif

ST_BMID_LVL flash_bmid_lvl[CE_MAX][NFCQ_DEPTH];

extern CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
extern CRITICAL_SECTION g_PuAccCriticalSection;
#ifdef HOST_SATA
extern CRITICAL_SECTION g_BUFMAPCriticalSection;
#endif
extern ST_CE_ERROR_ENTRY pu_err[CE_MAX];
extern U32 g_CACHE_STATUS_BASE;
extern U32 g_SSU_BASE;
extern U32 g_SSU1_BASE;
extern NF_PPUST_REG *p_flash_ppust_reg;
extern void SDC_SetFirstReadDataReady(U8 uRegValue);
extern CRITICAL_SECTION g_PuBitmapCriticalSection;
U32 sim_bufmap[64];

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
    if((regaddr >= NFC_CMD_STS_BASE)&&(regaddr < (NFC_CMD_STS_BASE+CE_MAX*sizeof(NFC_CMD_STS_REG)+CE_MAX*sizeof(NF_CQ_REG_CMDTYPE))))
    {
        bWtThru = TRUE;//update to lcoal memory
        pu = (regaddr - NFC_CMD_STS_BASE) / sizeof(NFC_CMD_STS_REG);
        offset = (regaddr - NFC_CMD_STS_BASE) % sizeof(NFC_CMD_STS_REG);
        tNfcqReg = p_flash_cq_dptr[pu];
        pNfcqReg = (U32 *)&tNfcqReg;
        //write dw0
        if( (offset >= 0) && (offset <4) )
        {
            //~0x0 << (offset*8):wipe off low bits;  (~0x0 >> ((4-offset-len)*8))?¨ºowipe off high bits;
            ulRealValue = regvalue & (~0x0 << (offset*8));
            ulRealValue <<= ((4-offset-len)*8);
            ulRealValue >>= ((4-offset-len)*8);
            ulRealValue = ulRealValue | (*pNfcqReg);
            pCqRegDw = (NFC_CMD_STS_REG *)&ulRealValue;
            if(pCqRegDw->bsIntSts)
            {
                EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
                p_flash_cq_dptr[pu].bsIntSts = 0;
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
            NFC_InterfaceUpdateLogicPuSts();
        }
    }
    //trig register
    else if((regaddr >= REG_BASE_NDC_TRIG) &&
        (regaddr < (REG_BASE_NDC_TRIG+CE_MAX*sizeof(U16)*NFCQ_DEPTH)))
    {
        bWtThru = FALSE;

        //command trigger register
        pu = (regaddr - REG_BASE_NDC_TRIG) / (NFCQ_DEPTH * sizeof(NFC_TRIGGER_REG));
        wp = (regaddr - REG_BASE_NDC_TRIG) % (NFCQ_DEPTH * sizeof(NFC_TRIGGER_REG));
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
    U8 pu;
    EnterCriticalSection(&g_PuAccCriticalSection);
    //default hw pu status
    p_flash_ppust_reg->ulIdleBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulErrorBitMap = 0;
    p_flash_ppust_reg->ulFullBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulEmptyBitMap = 0;

    for(pu = 0; pu < CE_MAX; pu++)
    {
        if(p_flash_pucr_reg->aNfcLogicPUReg[pu%4][pu/4].bsPuEnable)
        {
            p_flash_ppust_reg->ulIdleBitMap &= ~(1<<pu);
            p_flash_ppust_reg->ulIdleBitMap |= ((p_flash_cq_dptr[pu].bsIdle&0x1)<<pu);

            p_flash_ppust_reg->ulErrorBitMap &= ~(1<<pu);
            p_flash_ppust_reg->ulErrorBitMap |= ((p_flash_cq_dptr[pu].bsErrh&0x1)<<pu);

            p_flash_ppust_reg->ulFullBitMap &= ~(1<<pu);
            p_flash_ppust_reg->ulFullBitMap |= ((p_flash_cq_dptr[pu].bsFull&0x1)<<pu);

            p_flash_ppust_reg->ulEmptyBitMap &= ~(1<<pu);
            p_flash_ppust_reg->ulEmptyBitMap |= ((p_flash_cq_dptr[pu].bsEmpty&0x1)<<pu);
        }
    }

    p_flash_hpust_reg->bsAnyError = (p_flash_ppust_reg->ulErrorBitMap != 0)? 1 : 0;
    p_flash_hpust_reg->bsAllDone = (p_flash_ppust_reg->ulIdleBitMap == 0xFFFFFFFF)? 1 : 0;
    p_flash_hpust_reg->bsAllFull = (p_flash_ppust_reg->ulFullBitMap == 0xFFFFFFFF)? 1 : 0;

    LeaveCriticalSection(&g_PuAccCriticalSection);
    return;
}
/*
this function is called when FW trigger a register to make nfc update logic pu status


Note:
1) for fw convenience, not enabled pu report full status
*/

void NFC_InterfaceUpdateLogicPuSts(void)
{
    U8 pu, logicPu;

    return;
    //default value of logic pu status
    p_flash_lpust_reg->ulIdleBitMap = 0xFFFFFFFF;
    p_flash_lpust_reg->ulErrorBitMap = 0;
    p_flash_lpust_reg->ulFullBitMap = 0xFFFFFFFF;
    p_flash_lpust_reg->ulEmptyBitMap = 0;
    //default value of pu finish bitmap
    p_flash_pufsb_reg->ulLogPufsbReg= 0xffffffff;
        
    for(pu = 0; pu < CE_MAX; pu++)
    {
        if(p_flash_pucr_reg->aNfcLogicPUReg[pu%4][pu/4].bsPuEnable)
        {
            logicPu = p_flash_pucr_reg->aNfcLogicPUReg[pu%4][pu/4].bsLogicPU;
            p_flash_lpust_reg->ulIdleBitMap &= ~(1<<logicPu);
            p_flash_lpust_reg->ulIdleBitMap |= ((p_flash_cq_dptr[pu].bsIdle&0x1)<<logicPu);

            p_flash_lpust_reg->ulErrorBitMap &= ~(1<<logicPu);
            p_flash_lpust_reg->ulErrorBitMap |= ((p_flash_cq_dptr[pu].bsErrh&0x1)<<logicPu);

            p_flash_lpust_reg->ulFullBitMap &= ~(1<<logicPu);
            p_flash_lpust_reg->ulFullBitMap |= ((p_flash_cq_dptr[pu].bsFull&0x1)<<logicPu);

            p_flash_lpust_reg->ulEmptyBitMap &= ~(1<<logicPu);
            p_flash_lpust_reg->ulEmptyBitMap |= ((p_flash_cq_dptr[pu].bsEmpty&0x1)<<logicPu);

            p_flash_pufsb_reg->ulLogPufsbReg &= ~(1<<logicPu);
            #ifndef VT3514_C0
            p_flash_pufsb_reg->ulLogPufsbReg |= ((p_flash_cq_dptr[pu].bsFsLv0 | p_flash_cq_dptr[pu].bsFsLv1) << logicPu);
            #else
            p_flash_pufsb_reg->ulLogPufsbReg |= ((p_flash_cq_dptr[pu].bsFsLv0 | p_flash_cq_dptr[pu].bsFsLv1 | p_flash_cq_dptr[pu].bsFsLv2 | p_flash_cq_dptr[pu].bsFsLv3) << logicPu);
            #endif
        }
    }

    return;
}

void NFC_UpdateLogicPuSts(U8 uPhyPu)
{
// will be tested local
#if 1
    U8 logicPu = 0xFF;
    EnterCriticalSection(&g_PuBitmapCriticalSection);
    if(p_flash_pucr_reg->aNfcLogicPUReg[uPhyPu%4][uPhyPu/4].bsPuEnable)
    {
        logicPu = p_flash_pucr_reg->aNfcLogicPUReg[uPhyPu%4][uPhyPu/4].bsLogicPU;
        p_flash_lpust_reg->ulIdleBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulIdleBitMap |= ((p_flash_cq_dptr[uPhyPu].bsIdle&0x1)<<logicPu);

        p_flash_lpust_reg->ulErrorBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulErrorBitMap |= ((p_flash_cq_dptr[uPhyPu].bsErrh&0x1)<<logicPu);

        p_flash_lpust_reg->ulFullBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulFullBitMap |= ((p_flash_cq_dptr[uPhyPu].bsFull&0x1)<<logicPu);

        p_flash_lpust_reg->ulEmptyBitMap &= ~(1<<logicPu);
        p_flash_lpust_reg->ulEmptyBitMap |= ((p_flash_cq_dptr[uPhyPu].bsEmpty&0x1)<<logicPu);

        p_flash_pufsb_reg->ulLogPufsbReg &= ~(1<<logicPu);
        #ifndef VT3514_C0
        p_flash_pufsb_reg->ulLogPufsbReg |= ((p_flash_cq_dptr[uPhyPu].bsFsLv0 | p_flash_cq_dptr[uPhyPu].bsFsLv1) << logicPu);
        #else
        p_flash_pufsb_reg->ulLogPufsbReg |= ((p_flash_cq_dptr[uPhyPu].bsFsLv0 | p_flash_cq_dptr[uPhyPu].bsFsLv1 | p_flash_cq_dptr[uPhyPu].bsFsLv2 | p_flash_cq_dptr[uPhyPu].bsFsLv3) << logicPu);
        #endif
    }
    LeaveCriticalSection(&g_PuBitmapCriticalSection);
#endif
}

void NFC_InterfaceRecordErr(U8 pu, U8 errtype)
{
    pu_err[pu].err_type = errtype;
    pu_err[pu].err_flag = TRUE;
}

BOOL NFC_GetErrFlag(U8 pu)
{
    return (pu_err[pu].err_flag);
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
    U8 WrPtr;
    WrPtr = p_flash_cq_dptr[pu].bsWrPtr;

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
void NFC_InterfaceResetCQ(U8 pu, U8 level)
{
    EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
    p_flash_cq_dptr[pu].bsEmpty = 1;
    p_flash_cq_dptr[pu].bsErrh = 0;
    p_flash_cq_dptr[pu].bsErrSts = 0;
    p_flash_cq_dptr[pu].bsIntSts = 0;
    p_flash_cq_dptr[pu].bsIdle = 1;
    p_flash_cq_dptr[pu].bsRdPtr = level;
    p_flash_cq_dptr[pu].bsFull = 0;
    p_flash_cq_dptr[pu].bsWrPtr = level;
    p_flash_cq_dptr[pu].bsFsLv0 = 1;
    p_flash_cq_dptr[pu].bsFsLv1 = 1;
    #ifdef VT3514_C0
    p_flash_cq_dptr[pu].bsFsLv2 = 1;
    p_flash_cq_dptr[pu].bsFsLv3 = 1;
    #endif
        
    LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);

    NFC_InterfaceUpdateHwPuSts();
    NFC_UpdateLogicPuSts(pu);
}

/************************************************
Function Name: NFC_InterfaceIsCQEmpty
Input:     pu - physical pu number 
Output:     None 
Description:
check CQ empty
*************************************************/
BOOL NFC_InterfaceIsCQEmpty(U8 pu)
{
    BOOL bempty;


    if((FALSE == p_flash_cq_dptr[pu].bsErrh) &&
        (FALSE == p_flash_cq_dptr[pu].bsIntSts))
    {
        bempty = p_flash_cq_dptr[pu].bsEmpty;
    }
    else
    {
        bempty = TRUE;//FALSE;
    }

    return bempty;
}

/************************************************
Function Name: NFC_InterfaceCQRP
Input:     pu - physical pu number 
Output:     None 
Description:
get cq read pointer
*************************************************/
U8 NFC_InterfaceCQRP(U8 pu)
{
    return (U8)(p_flash_cq_dptr[pu].bsRdPtr);
}

/************************************************
Function Name: NFC_InterfaceCQWP
Input:     pu - physical pu number 
Output:     None 
Description:
get cq write pointer
*************************************************/
U8 NFC_InterfaceCQWP(U8 pu)
{
    return (U8)(p_flash_cq_dptr[pu].bsWrPtr);
}

U8 NFC_InterfaceGetFullStatus(U8 pu)
{
    U8 nRegValue = p_flash_cq_dptr[pu].bsFull;

    return nRegValue;
}

U8 NFC_InterfaceGetErrHold(U8 pu)
{
    U8 nRegValue = p_flash_cq_dptr[pu].bsErrh;

    return nRegValue;
}

U8 NFC_InterfaceGetCmdType(U8 pu, U8 level)
{
    U8 nRegValue;

    nRegValue = p_flash_cq_dptr_cmdtype[pu].CmdType[level];

    return nRegValue;
}

/************************************************
Function Name: NFC_InterfaceJumpCQRP
Input:     pu - physical pu number 
Output:     None 
Description:
jump read pointer and reset full empty status
*************************************************/
void NFC_InterfaceJumpCQRP(U8 pu)
{
    switch (p_flash_cq_dptr[pu].bsRdPtr)
    {
        case 0:
        {
            p_flash_cq_dptr[pu].bsFsLv0 = 1;
            break;
        }
        case 1:
        {
            p_flash_cq_dptr[pu].bsFsLv1 = 1;
            break;
        }
        #ifdef VT3514_C0
        case 2:
        {
            p_flash_cq_dptr[pu].bsFsLv2 = 1;
            break;
        }
        case 3:
        {
            p_flash_cq_dptr[pu].bsFsLv3 = 1;
            break;
        }
        #endif
        default:
        {
            DBG_Getch();
        }
    }
        
    p_flash_cq_dptr[pu].bsRdPtr = (p_flash_cq_dptr[pu].bsRdPtr+1)%NFCQ_DEPTH;

    if(p_flash_cq_dptr[pu].bsRdPtr == p_flash_cq_dptr[pu].bsWrPtr)
    {
        p_flash_cq_dptr[pu].bsEmpty = 1;
        p_flash_cq_dptr[pu].bsIdle = 1;
    }
    else
    {
        p_flash_cq_dptr[pu].bsEmpty = 0;
        p_flash_cq_dptr[pu].bsIdle = 0;
    }
    p_flash_cq_dptr[pu].bsFull = 0;// moved from else{} above

    NFC_InterfaceUpdateHwPuSts();
    NFC_UpdateLogicPuSts(pu);
}

/************************************************
Function Name: NFC_InterfaceJumpCQWP
Input:     pu - physical pu number 
Output:     None 
Description:
jump CQ wp
*************************************************/
void NFC_InterfaceJumpCQWP(U8 pu)
{
    switch (p_flash_cq_dptr[pu].bsWrPtr)
    {
        case 0:
        {
            p_flash_cq_dptr[pu].bsFsLv0 = 0;
            break;
        }
        case 1:
        {
            p_flash_cq_dptr[pu].bsFsLv1 = 0;
            break;
        }
        #ifdef VT3514_C0
        case 2:
        {
            p_flash_cq_dptr[pu].bsFsLv2 = 0;
            break;
        }
        case 3:
        {
            p_flash_cq_dptr[pu].bsFsLv3 = 0;
            break;
        }
        #endif
        default:
        {
            DBG_Getch();
        }
    }
          
    p_flash_cq_dptr[pu].bsWrPtr = (p_flash_cq_dptr[pu].bsWrPtr+1)%NFCQ_DEPTH;

    if(p_flash_cq_dptr[pu].bsWrPtr == p_flash_cq_dptr[pu].bsRdPtr)
    {
        p_flash_cq_dptr[pu].bsFull= 1;
    }
    else
    {
        p_flash_cq_dptr[pu].bsFull= 0;
    }

    p_flash_cq_dptr[pu].bsEmpty= 0;// moved from else{} above
    p_flash_cq_dptr[pu].bsIdle= 0;
}

/************************************************
Function Name: NFC_InterfaceSetErrParameter
Input:     pu - physical pu number 
nErrType - flash error type
Output:     None 
Description:
input error parameter to CQ REG
*************************************************/
void NFC_InterfaceSetErrParameter(U8 nPU, U8 nErrType)
{
    EnterCriticalSection(&g_CHCriticalSection[nPU&NFC_CH_MSK]);
    p_flash_cq_dptr[nPU].bsErrSts = nErrType;
    p_flash_cq_dptr[nPU].bsIntSts = 1;

    //cq rp do not jump when error injection
    p_flash_cq_dptr[nPU].bsErrh = 1;
    p_flash_cq_dptr[nPU].bsIdle = 1;   
    LeaveCriticalSection(&g_CHCriticalSection[nPU&NFC_CH_MSK]);
    
    NFC_InterfaceUpdateHwPuSts();
    NFC_UpdateLogicPuSts(nPU);
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
void NFC_InterfaceSetBuffermap(U8 pu, U32 bufmap_value, U32 Bmid)
{
    U32 bufmap_value_old;
    //U32 sec_en;
    //U8 rp;
    U8 bufmap_id;

    //rp = p_flash_cq_dptr[pu].bsRegDW0.RP;
    //bufmap_id = flash_bmid_lvl[pu][rp].bmid;
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
void NFC_InterfaceUpdateSsu(U32 ssu_addr, U32 value,U8 index)
{
    //U32 ulVar;
    U8 ulVar;
    if(0 == index)    //SSU 0
    {
        Comm_WriteOtfb(g_SSU_BASE + (ssu_addr<<2), 1, &value);
    }
    else    // SSU1
    {
        ulVar = value & 0xff;
        Comm_WriteOtfbByByte(g_SSU1_BASE + (ssu_addr), 1, &ulVar);
    }
}

/************************************************
Function Name: NFC_InterfaceUpdateCacheStatus
Input:     cacheStatus_addr, value
Output:     None 
Description:
cache status update
*************************************************/
//void NFC_InterfaceUpdateCacheStatus(U32 cacheStatus_addr, U32 value)
//{
//    Comm_WriteOtfb(g_CACHE_STATUS_BASE + cacheStatus_addr, 1, &value);
//}

void NFC_InterfaceUpdateRedData(ST_FLASH_CMD_PARAM *p_flash_cmd_para)
{
    U8 phyPageIndex;
    NFC_RED *pLocalRed;

    pLocalRed = p_flash_cmd_para->p_local_red;
    for(phyPageIndex=0; phyPageIndex<PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    {
        if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
            p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[1] != 0)
        {
            Comm_WriteOtfb( p_flash_cmd_para->p_red_addr + sizeof(U32)*(g_RED_SZ_DW*phyPageIndex),
                g_RED_SZ_DW,
                &pLocalRed->aContent[g_RED_SZ_DW*phyPageIndex]);
        }
    }
}

/************************************************
Function Name: NFC_InterfaceSetInt
Input:     physical pu number
Output:     None 
Description:
trigger interrupt
*************************************************/
void NFC_InterfaceSetInt(U8 pu)
{
    U32 regValue;
    //wait for interrupt reg clear
    do{
        Comm_ReadReg((U32)&rNfcIntAcc, 1, &regValue);
    }while(regValue & NF_INTACC_PENDING);

    regValue |= NF_INTACC_PENDING;//set int pending
    regValue &= ~(NF_INTACC_PUMSK);
    regValue |= pu;//set pu
    #ifndef VT3514_C0
    regValue |= p_flash_cq_dptr[pu].bsRdPtr? NF_INTACC_CQPOS : 0;//set cq level
    #else    
    regValue |= p_flash_cq_dptr[pu].bsRdPtr << 5;//set cq level
    #endif
    Comm_WriteReg((U32)&rNfcIntAcc, 1, &regValue);
    
    p_flash_cq_dptr[pu].bsIntSts = 1;
    //call interrupt system function
#if defined(SIM_XTENSA)
    setInterrupt(10, 1);
#endif
    return;
}

