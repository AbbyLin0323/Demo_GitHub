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
Filename    :L2_TblChk.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.08.07    10:33:08
Description :table check
Others      :
Modify      :
*******************************************************************************/
#include "HAL_Inc.h"
#include "HAL_FlashDriverExt.h"
#include "L2_Defines.h"
#include "L2_GCManager.h"
#include "L2_PMTManager.h"
#include "L2_TblChk.h"
#include "L2_RT.h"
#include "L2_PBIT.h"
#include "L2_VBT.h"
#include "L2_Interface.h"
#include "L2_TableBlock.h"
#include "L2_FCMDQ.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"

extern GLOBAL  U32 g_RpmtBaseAddr;
extern GLOBAL  U32 g_L2TempBufferAddr;
/****************************************************************************
Name        :TblChk_GetRpmtEntry
Input       :U8 ucCeNum, U16 usBlockInPu
Output      :RPMT addr
Author      :HenryLuo
Date        :2012.08.07    15:21:49
Description :get the RMPT addr in dram whick has been loaded accroding to the
logic block num in PU.
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT TblChk_GetRpmtEntry(U8 ucSuperPU, U16 usBlockNum)
{
    U32 ulRpmtBaseAddr;
    U32 ulOffset;

    ulOffset = (ucSuperPU * BLK_PER_PLN + usBlockNum)* sizeof(RPMT);
    ulRpmtBaseAddr = g_RpmtBaseAddr + ulOffset;

    return ulRpmtBaseAddr;
}

/****************************************************************************
Name        :L2_RPMTLoad
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.08.07    10:35:48
Description :load RPMT of the block in BMT except the target seq and rand block.
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L2_RPMTLoad()
{
    U32 ulBlockNum;
    U8 ucSuperPU;
    RPMT* pRPMT;
    FLASH_ADDR FlashAddr = { 0 };
    U16 usBufferId;
    U8  ucErrCode;
    NFC_READ_REQ_DES tRdDes = { 0 };

    /* load rpmt */
    for (ulBlockNum = 0; ulBlockNum < VIR_BLK_CNT; ulBlockNum++)
    {
        for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
        {
            /* if block is the current target block in Pu, no need to load rpmt */
            if (VBT_NOT_TARGET != pVBT[ucSuperPU]->m_VBT[ulBlockNum].Target)
            {
                continue;
            }

            /* if block have no been writen, no need to load rpmt */
            if (INVALID_4F == pVBT[ucSuperPU]->m_VBT[ulBlockNum].StripID)
            {
                continue;
            }

            pRPMT = (RPMT*)TblChk_GetRpmtEntry(ucSuperPU, ulBlockNum);

            usBufferId = COM_GetBufferIDByMemAddr((U32)pRPMT, TRUE, BUF_SIZE_BITS);

            FlashAddr.ucPU = ucSuperPU;
            FlashAddr.usBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPU, 0, ulBlockNum);
            FlashAddr.usPage = PG_PER_SLC_BLK - 1;

            tRdDes.bsSecStart = 0;
            tRdDes.bsSecLen = SEC_PER_BUF;
            tRdDes.bsRedOntf = TRUE;
            tRdDes.bsRdBuffId = usBufferId;
            tRdDes.ppNfcRed = NULL;
            tRdDes.pErrInj = NULL;

            if (NFC_STATUS_SUCCESS != HAL_NfcPageRead(&FlashAddr, &tRdDes))// l84mod
            {
                DBG_Printf("L2_RPMTLoad : status error %d\n",HAL_NfcPageRead(&FlashAddr, &tRdDes));
                DBG_Getch();
            }

            /* wait all rpmt load done */
            ucErrCode = HAL_NfcWaitStatus(FlashAddr.ucPU, FlashAddr.ucLun);
            if (NF_SUCCESS != ucErrCode)
            {
                DBG_Printf("L2_RPMTLoad : error code %d\n",ucErrCode);
                DBG_Getch();
            }
            else
            {
                if ((pRPMT->m_RPMT[0].m_SuperPU != ucSuperPU) || (pRPMT->m_RPMT[0].m_SuperBlock != ulBlockNum))
                {
                    DBG_Printf("L2_RPMTLoad : %d %d\n",pRPMT->m_RPMT[0].m_SuperPU,pRPMT->m_RPMT[0].m_SuperBlock);
                    DBG_Printf("%d %d\n",ucSuperPU,ulBlockNum);
                    
                    DBG_Getch();
                }


            }
        }
    }
}

/****************************************************************************
Name        :L2_PMTChk
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.08.07    15:45:05
Description :get the physical addr of one LPN through the PMT, then get the
refLPN through the RPMT, compare the LPN with the refLPN.
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_PMTChk(void)
{
    PhysicalAddr Addr = { 0 };
    PuInfo* pInfo;
    U32 ulLpn, ulRefLpn;
    RPMT* pRPMT;
    U8 ucPu;
    U32 ulRPMTPointer;
    U8 ucTargetType;
    U16 usPhysicalBlock;

    for (ulLpn = 0; ulLpn < MAX_LPN_IN_DISK; ulLpn++)
    {
        L2_LookupPMT(&Addr, ulLpn, FALSE);
        if (INVALID_8F == Addr.m_PPN)
        {
            continue;
        }
        else
        {
            usPhysicalBlock = L2_VBT_GetPhysicalBlockAddr(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);
            if (INVALID_4F == usPhysicalBlock)
            {
                DBG_Printf("L2_PMTChk : phyblk %d\n",usPhysicalBlock);
                DBG_Getch();
            }

            ucPu = L2_GetSuperPuFromLPN(ulLpn);
            pInfo = g_PuInfo[ucPu];

            ucTargetType = pVBT[Addr.m_PUSer]->m_VBT[Addr.m_BlockInPU].Target;
            if (VBT_TARGET_HOST_W == ucTargetType)
            {
                ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML];
                pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][ulRPMTPointer];
            }
            else if (VBT_TARGET_HOST_GC == ucTargetType)
            {
                ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_GC];
                pRPMT = pInfo->m_pRPMT[TARGET_HOST_GC][ulRPMTPointer];
            }
            else if (VBT_NOT_TARGET == ucTargetType)
            {
                pRPMT = (RPMT*)TblChk_GetRpmtEntry(Addr.m_PUSer, Addr.m_BlockInPU);
            }
            else
            {
                DBG_Printf("L2_PMTChk : target type %d\n",ucTargetType);
                DBG_Getch();
            }

            ulRefLpn = L2_LookupRPMT(pRPMT, &Addr);
            if (ulRefLpn != ulLpn)
            {
                DBG_Printf("table check error at ce %d block %d page %d lpn %d\n",
                    Addr.m_PUSer, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_LPNInPage);
                DBG_Printf("ulLpn = 0x%x, ulRefLpn = 0x%x\n", ulLpn, ulRefLpn);
                //DBG_Getch();
                return FALSE;
            }
        }
    }

    DBG_Printf("PMT table check pass !!!\n");
    return TRUE;
}

/****************************************************************************
Name        :L2_DirtyCntCheck
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.08.10    18:31:11
Description :calculate the dirty page count of the block in BMT according to the RMPT.
then check the value with the info recorded in Pu.
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_DirtyCntCheck()
{
    U8 ucSuperPU;
    U32 ulVirBlock;
    PhysicalAddr Addr = { 0 };
    PhysicalAddr RefAddr = { 0 };
    RPMT* pRPMT;
    PuInfo* pInfo;
    U32 ulPuId;
    U32 ulRPMTPointer;
    U16 ppo;
    U16 LpnInPage;
    U32 ulLpn;
    U32 ulDirtyCnt;
    U32 ulCheckPageCnt;
    U32 ulTemp;
    U8 ucTargetType;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        for (ulVirBlock = 0; ulVirBlock < VIR_BLK_CNT; ulVirBlock++)
        {
            /* if block is the GC block, no need to check, tobey update only check TLCGC DirtyCnt */
            if (ulVirBlock == g_GCManager[SLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPU])
            {
                continue;
            }

            /* if block have no been writen, no need to check */
            ulPuId = pVBT[ucSuperPU]->m_VBT[ulVirBlock].StripID;
            if (INVALID_4F == ulPuId)
            {
                continue;
            }

            pInfo = g_PuInfo[ulPuId];

            ucTargetType = pVBT[ucSuperPU]->m_VBT[ulVirBlock].Target;
            if (ucTargetType == VBT_TARGET_INVALID)
            {
                /* block not been writen */
                continue;
            }
            else if (ucTargetType == VBT_TARGET_HOST_W)
            {
                ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML];
                pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][ulRPMTPointer];
                ulCheckPageCnt = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                ulDirtyCnt = 0;
            }
            else if (ucTargetType == VBT_TARGET_HOST_GC)
            {
                ulRPMTPointer = pInfo->m_RPMTBufferPointer[TARGET_HOST_GC];
                pRPMT = pInfo->m_pRPMT[TARGET_HOST_GC][ulRPMTPointer];
                ulCheckPageCnt = pInfo->m_TargetPPO[TARGET_HOST_GC];
                ulDirtyCnt = 0;
            }
            else
            {
                pRPMT = (RPMT*)TblChk_GetRpmtEntry(ucSuperPU, ulVirBlock);
                ulCheckPageCnt = PG_PER_SLC_BLK - 1;

                /* in block that has been flushed, RPMT page same as 4 or 8 dirty lpn */
                ulDirtyCnt = LPN_PER_BUF;
            }

            for (ppo = 0; ppo < ulCheckPageCnt; ppo++)
            {
                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    Addr.m_PUSer = ucSuperPU;
                    Addr.m_BlockInPU = ulVirBlock;
                    Addr.m_PageInBlock = ppo;
                    Addr.m_LPNInPage = LpnInPage;

                    ulLpn = L2_LookupRPMT(pRPMT, &Addr);
                    if (INVALID_8F == ulLpn)
                    {
                        ulDirtyCnt++;
                        continue;
                    }

                    /* lookup PMT to get reference physical addr from logic LPN */
                    L2_LookupPMT(&RefAddr, ulLpn, FALSE);

                    /* if not equel, data is dirty */
                    if (Addr.m_PPN != RefAddr.m_PPN)
                    {
                        ulDirtyCnt++;
                    }
                }
            }

            ulTemp = L2_GetDirtyCnt(ucSuperPU, ulVirBlock);
            if (ulTemp != ulDirtyCnt)
            {
                DBG_Printf("DirtyCnt check error! ce %d, block %d\n", ucSuperPU, ulVirBlock);
                DBG_Getch();
                //return FALSE;
            }
        }
        DBG_Printf("PU %d check pass! \n", ucSuperPU);
    }

    return TRUE;
}

/****************************************************************************
Name        :L2_PMTIChk
Input       :
Output      :
Author      :HenryLuo
Date        :2012.11.29    11:40:07
Description :
Others      :
Modify      :
****************************************************************************/

BOOL MCU1_DRAM_TEXT L2_PMTIChk()
{
    U32 ulPMTIndexInPu;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
	PhysicalAddr Addr = { 0 };
    FLASH_ADDR FlashAddr = { 0 };
    U8  ucErrCode;
    U16 usBufferId;
    RED *pSpare;
    U32 ulPuNum;
    NFC_READ_REQ_DES tRdDes = { 0 };
    NFC_READ_REQ_DES tRetryDes = { 0 };

    for (ulPuNum = 0; ulPuNum < SUBSYSTEM_PU_NUM; ulPuNum++)
    {
        for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_PU; ulPMTIndexInPu++)
        {
            /* PMT page in RAM */
            if (!L2_IsPMTPageDirty(ulPuNum, ulPMTIndexInPu))
            {
                pPMTPage = g_PMTManager->m_pPMTPage[ulPuNum][ulPMTIndexInPu];
            }
            /* PMT page in flash, need to load first */
            else
            {
                /* get PMT page flash addr */
                Addr.m_PPN = L2_GetPMTPhysicalAddr(ulPuNum, 0, ulPMTIndexInPu);
                FlashAddr.ucPU = Addr.m_PUSer;
                FlashAddr.ucLun= Addr.m_OffsetInSuperPage;
                FlashAddr.usBlock = Addr.m_BlockInPU;
                FlashAddr.usPage = Addr.m_PageInBlock;

                /* load PMT page into L2 temp buffer */
#ifdef PMT_ITEM_SIZE_REDUCE
                pPMTPage = (U32*)g_L2TempBufferAddr;
#else
				pPMTPage = (PMTPage*)g_L2TempBufferAddr;
#endif
                usBufferId = COM_GetBufferIDByMemAddr((U32)pPMTPage, TRUE, BUF_SIZE_BITS);

                tRdDes.bsSecStart = 0;
                tRdDes.bsSecLen = SEC_PER_BUF;
                tRdDes.bsRedOntf = TRUE;
                tRdDes.bsRdBuffId = usBufferId;
                tRdDes.ppNfcRed = (NFC_RED **)&pSpare;
                tRdDes.pErrInj = NULL;

                if (NFC_STATUS_SUCCESS != HAL_NfcPageRead(&FlashAddr, &tRdDes))// l84mod
                {
                    DBG_Printf("L2_PMTIChk : HAL_NfcPageRead status %d\n",HAL_NfcPageRead(&FlashAddr, &tRdDes));
                    DBG_Getch();
                }

                /* wait PMT page load done */
                tRetryDes.bsSecStart = 0;
                tRetryDes.bsSecLen = SEC_PER_BUF;
                tRetryDes.bsRdBuffId = usBufferId;
                tRetryDes.bsRedOntf = TRUE;
                tRetryDes.ppNfcRed = (NFC_RED **)&pSpare;
                tRetryDes.pErrInj = NULL;

                ucErrCode = HAL_NfcWaitStatusWithRetry(&FlashAddr, &tRetryDes, 8);
                if (NF_SUCCESS != ucErrCode)
                {
                    DBG_Printf("L2_PMTIChk : errocode %d\n",ucErrCode);
                    DBG_Getch();
                }

                /* check PMT page */
                if (ulPMTIndexInPu != pSpare->m_PMTIndex)
                {
                    DBG_Printf("L2_PMTIChk : PMTI %d %d\n",ulPMTIndexInPu, pSpare->m_PMTIndex);
                    DBG_Getch();
                }
            }
        }
    }

    return TRUE;
}

/****************************************************************************
Name        :L2_TableCheck
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.08.09    18:52:57
Description :main funtion of the table check.
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_TableCheck()
{
    /* check L3 free */
    if (FALSE == L2_FCMDQIsAllEmpty())
    {
        return FAIL;
    }

    L2_RPMTLoad();

    if (FALSE == L2_PMTChk())
    {
        return FALSE;
    }

    if (FALSE == L2_DirtyCntCheck())
    {
        return FALSE;
    }

    return TRUE;
}

