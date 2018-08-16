#include "BaseDef.h"
#include "Disk_Config.h"
#include "COM_BitMask.h"
#include "L2_PMTManager.h"
#include "L2_TLCMerge.h"
#include "L2_Interface.h"
#include "L2_StripeInfo.h"
#include "L2_VBT.h"
#include "L2_PBIT.h"
#include "L2_FTL.h"
#include "L2_Schedule.h"
#include "L2_ErrorHandling.h"
#include "L2_Erase.h"
#include "L2_StaticWL.h"
#include "L2_TableBBT.h"
#include "FW_Event.h"

#if defined(L2MEASURE) || (defined(SWL_EVALUATOR))
#include "L2_Evaluater.h"
#endif

extern GLOBAL  LC lc;

extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
extern GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_TLCMergeErrHandle;
extern TLCGCSrcBlkRecord * g_pTLCGCSrcBlkRecord;

extern U32 LookupRPMTInGC(U8 ucSuperPu, PhysicalAddr* pAddr, U8 ucRPMTNum, BOOL bTLCWrite, GCComStruct *ptCom);
extern BOOL L2_BbtSave(U8 ucTLun, U8 ucErrHTLun);
extern void L2_LoadSpare(PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, BOOL bSLCMode);

#if (defined(SWL_EVALUATOR) && (!defined(SWL_OFF)))
extern void SWLRecordTLCW(U8 ucSuperPu);
#endif

#if (!defined(NEW_SWL) && defined(DBG_LC))
extern GLOBAL  LC lc;
#endif

GLOBAL MCU12_VAR_ATTR INVERSE_TLC_SHARED_PAGE l_aTLCInverseProgOrder[PG_PER_WL * PG_PER_SLC_BLK];

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
GLOBAL MCU12_VAR_ATTR U16 m_ShutdownCalculateStartPage[2];
#endif

INVERSE_TLC_SHARED_PAGE MCU1_DRAM_TEXT L2_Calculate3DTLCSharedPage(U16 usPageNum)
#ifdef FLASH_IM_3DTLC_GEN2 // for B16A and B17A
{
    U8 ucType = 0xFF;
    U8 ucPageCnt = 2;
    U16  ulPairPage = 0xFFFF;
    
    INVERSE_TLC_SHARED_PAGE InverseTLCShare;

    if (usPageNum < 12)
    {
        ucPageCnt = 1;
        ucType = 3;
    }
    else if(usPageNum >= 12 && usPageNum <= 35)
    {
        if ((usPageNum - 12)%2)
        {
            ucType = 1;
            ulPairPage = usPageNum - 1;
        }
        else
        {
            ucType = 0;
            ulPairPage = usPageNum + 1;
        }
    }
    else if (usPageNum >= 36 && usPageNum <= 59)
    {
        ucType = 3;
        ucPageCnt = 1;
        ulPairPage = (usPageNum - 36)*2 + 25 + usPageNum;
    }
    else if (usPageNum >= 60 && usPageNum <= 2269)
    {
        if ((usPageNum >=62) && (usPageNum <= 2219)&& ((usPageNum - 62)%3) == 0)
        {
            ucType = 3;
            ucPageCnt = 1;
            if (usPageNum <= 2150)
                ulPairPage = usPageNum + 71;
            else if (usPageNum <= 2186)
                ulPairPage = usPageNum + 72 + (usPageNum - 2153)/3;
            else //if (usPageNum <= 2219)
                ulPairPage = usPageNum + 82 - (usPageNum - 2189)/3;
        } 
        else if (usPageNum >= 60 && usPageNum <= 2218)
        {
            if (((usPageNum >=60) && ((usPageNum - 60)%12) == 0) || ((usPageNum >=63) && ((usPageNum - 63)%12) == 0)
             || ((usPageNum >=66) && ((usPageNum - 66)%12) == 0) || ((usPageNum >=69) && ((usPageNum - 69)%12) == 0))
            {
                ucType = 2;
                ulPairPage = usPageNum + 1;
            }
            else if (((usPageNum >=61) && ((usPageNum - 61)%12) == 0) || ((usPageNum >=64) && ((usPageNum - 64)%12) == 0)
                 || ((usPageNum >=67) && ((usPageNum - 67)%12) == 0) || ((usPageNum >=70) && ((usPageNum - 70)%12) == 0))
            {
                ucType = 1;
                ulPairPage = usPageNum - 1;
            }
        }
        else if (usPageNum >= 2220 && usPageNum <= 2269 && ((usPageNum%4) == 0 || (usPageNum%4) == 1)) 
        {
            if (((usPageNum >=2220) && ((usPageNum - 2220)%16) == 0) || ((usPageNum >=2224) && ((usPageNum - 2224)%16) == 0)
             || ((usPageNum >=2228) && ((usPageNum - 2228)%16) == 0) || ((usPageNum >=2232) && ((usPageNum - 2232)%16) == 0))
            {
                ucType = 2;
                ulPairPage = usPageNum + 1;
            }
            else if (((usPageNum >=2221) && ((usPageNum - 2221)%16) == 0) || ((usPageNum >=2225) && ((usPageNum - 2225)%16) == 0)
                  || ((usPageNum >=2229) && ((usPageNum - 2229)%16) == 0) || ((usPageNum >=2233) && ((usPageNum - 2233)%16) == 0))
            {
                ucType = 1;
                ulPairPage = usPageNum - 1;
            }
        }
        else if (usPageNum >= 2222 && usPageNum <= 2267 && ((usPageNum%4) == 2 || (usPageNum%4) == 3))
        {
            if (((usPageNum >=2222) && ((usPageNum - 2222)%16) == 0) || ((usPageNum >=2226) && ((usPageNum - 2226)%16) == 0)
             || ((usPageNum >=2230) && ((usPageNum - 2230)%16) == 0) || ((usPageNum >=2234) && ((usPageNum - 2234)%16) == 0))
            {
                ucType = 0;
                ulPairPage = usPageNum + 1;
            }
            else if (((usPageNum >=2223) && ((usPageNum - 2223)%16) == 0) || ((usPageNum >=2227) && ((usPageNum - 2227)%16) == 0)
                  || ((usPageNum >=2231) && ((usPageNum - 2231)%16) == 0) || ((usPageNum >=2235) && ((usPageNum - 2235)%16) == 0))
            {
                ucType = 1;
                ulPairPage = usPageNum - 1;
            }
        }
    }
    else if(usPageNum >= 2270 && usPageNum <= 2291)
    {
        if ((usPageNum - 2272)%2)
        {
            ucType = 1;
            ulPairPage = usPageNum - 1;
        }
        else
        {
            ucType = 2;
            ulPairPage = usPageNum + 1;
        }
    }
    else if (usPageNum >= 2292 && usPageNum <= 2303)
    {
        ucPageCnt = 1;
        ucType = 3;
    }

    InverseTLCShare.m_PageType = ucType;//Lower/Upper/Extra/LowerWOUpper Page
    InverseTLCShare.m_ProgPageCnt = ucPageCnt; // 0 ~2
    InverseTLCShare.m_2ndPageNum = (0xFFF & ulPairPage); //0 ~2303

    return InverseTLCShare;
}
#else //for FLASH_IM_3DTLC_GEN1 : B0KB
{
    U8 ucType = 0xFF;
    U8 ucPageCnt = 2;
    U16  ulPairPage = 0xFFFF;
    BOOL bWLClosed = FALSE;
     
    INVERSE_TLC_SHARED_PAGE InverseTLCShare;

    if (usPageNum < 16)
    {
        ucPageCnt = 1;
        ucType = 3;
        
        if(usPageNum == 15)
           bWLClosed = TRUE;
    }
    else if (usPageNum >= 16 && usPageNum < 112)
    {
        if ((usPageNum - 16) % 2)
        {
            ucType = 1;
        }
        else
        {
            ucType = 0;
        }

        if(usPageNum == 47)
           bWLClosed = TRUE;
    }
    else if (usPageNum >= 112 && usPageNum <= 1505)
    {
        if (((usPageNum >= 112) && ((usPageNum - 112) % 12) == 0) || ((usPageNum >= 115) && ((usPageNum - 115) % 12) == 0)
         || ((usPageNum >= 118) && ((usPageNum - 118) % 12) == 0) || ((usPageNum >= 121) && ((usPageNum - 121) % 12) == 0))
        {
            ucType = 2;

            if((usPageNum >= 157) && (((usPageNum - 157) % 48) == 0))
                bWLClosed = TRUE;
        }
        else if (((usPageNum >= 113) && ((usPageNum - 113) % 12) == 0) || ((usPageNum >= 116) && ((usPageNum - 116) % 12) == 0)
              || ((usPageNum >= 119) && ((usPageNum - 119) % 12) == 0) || ((usPageNum >= 122) && ((usPageNum - 122) % 12) == 0))
        {
            ucType = 0;
            if (usPageNum == 1505)
            {
                ucPageCnt = 1;
                ucType = 3;
            }
        }
        else if ((((usPageNum - 113) % 12) == 1) || (((usPageNum - 116) % 12) == 1)
              || (((usPageNum - 119) % 12) == 1) || (((usPageNum - 122) % 12) == 1))
        {
            ucType = 1;
        }

       //if(usPageNum == 1503)
       //   bWLClosed = TRUE;        
        
    }
    else if (usPageNum >= 1506 && usPageNum <= 1535)
    {
        if (((usPageNum >= 1506) && ((usPageNum - 1506) % 8) == 0) || ((usPageNum >= 1508) && ((usPageNum - 1508) % 8) == 0)
         || ((usPageNum >= 1510) && ((usPageNum - 1510) % 8) == 0) || ((usPageNum >= 1512) && ((usPageNum - 1512) % 8) == 0))
        {
            ucType = 2;
        }
        else if (usPageNum % 2)
        {
            ucPageCnt = 1;
            ucType = 3;
        }
    }

    if (ucType == 1)
    {
        ucPageCnt = 0;
        ulPairPage = usPageNum - 1;
    }
    else if (ucType == 0 && ucPageCnt == 2)
    {
        ulPairPage = usPageNum + 1;
    }
    else if (ucType == 2)
    {
        if (usPageNum >= 112 && usPageNum <= 202)
            ulPairPage = usPageNum - 63 - ((usPageNum - 112) / 3);
        else if (usPageNum >= 1506 && usPageNum <= 1534)
            ulPairPage = usPageNum - 93 + ((usPageNum - 1506) >> 1);
        else
            ulPairPage = usPageNum - 94;
    }

    InverseTLCShare.m_PageType = ucType;//Lower/Upper/Extra/LowerWOUpper Page
    InverseTLCShare.m_ProgPageCnt = ucPageCnt; // 0 ~2
    InverseTLCShare.m_2ndPageNum = (0xFFF & ulPairPage); //0 ~1535    

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    InverseTLCShare.m_WLClosed = bWLClosed;
#endif

    return InverseTLCShare;
}  
#endif

void MCU1_DRAM_TEXT L2_InitTLCInverseProgOrderTable(void)
{
    U32 i;
  
    for (i = 0; i < (PG_PER_SLC_BLK * PG_PER_WL); i++)
    {
        l_aTLCInverseProgOrder[i] = L2_Calculate3DTLCSharedPage(i);
    }

    return;
}

void L2_ResetTLCManager(U8 ucSuperPu)
{
    U8 j, i;

    for (j = 0; j < PG_PER_WL; j++)
    {
        g_TLCManager->ausSrcBlk[ucSuperPu][j] = INVALID_4F;
    }
    g_TLCManager->ausDesTLCBlk[ucSuperPu] = INVALID_4F;

    g_TLCManager->ausTLCProgOrder[ucSuperPu] = 0;
    g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_PRPARE;
    g_TLCManager->aeTLCResetStage[ucSuperPu] = TLCRESET_UPDATE_RPMT;
    g_TLCManager->aucSLCReadNum[ucSuperPu] = 0;
    g_TLCManager->aeTLCWriteType[ucSuperPu] = TLC_WRITE_ALL;
    g_TLCManager->aucTLCReadBitMap[ucSuperPu] = 0;
    g_TLCManager->aucRPMTNum[ucSuperPu] = 0;
    g_TLCManager->aeErrHandleStage[ucSuperPu] = TLC_ERRH_ALL;
    g_TLCManager->aulErrProgBitMap[ucSuperPu] = 0;
    g_TLCManager->bTLCInternalW[ucSuperPu] = TRUE;
#ifdef L2_HANDLE_UECC
    g_TLCManager->aeErrHandleUECCStage[ucSuperPu] = TLC_ERRH_ALL;
#endif

    g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] = 0;

    for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
    {
        for (i = 0; i < (TLC_BUF_CNT + 1); i++)
        {
            g_TLCManager->aucTLCBufStatus[ucSuperPu][j][i] = SUBSYSTEM_STATUS_SUCCESS;
        }

        for (i = 0; i < PG_PER_WL; i++)
        {
            g_TLCManager->aucRPMTStatus[ucSuperPu][j][i] = SUBSYSTEM_STATUS_SUCCESS;
        }
#ifdef L2_HANDLE_UECC
        for (i = 0; i < L2_HANDLE_UECCBLK_PERLUN_MAX; i++)
        {
            g_TLCManager->ausUECCBlk[ucSuperPu][j][i] = INVALID_4F;
        }
        g_TLCManager->ausUECCBlkCnt[ucSuperPu][j] = 0;
#endif
    }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        m_ShutdownCalculateStartPage[ucSuperPu] = 0;
#endif

    return;
}

BOOL L2_SetTLCMergeInfo(U8 ucSuperPu, U8 ucSrcVBTType, TLCWriteType eWriteType)
{
    U8 i;
    U16 PBN;
    U32 j;
    U16 usGCSrcBlk;
    COMMON_EVENT L2_Event;

#ifdef SIM
    if (TLC_WRITE_ALL != g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        DBG_Printf("[%s] TLC_WRITE_ALL != g_TLCManager->aeTLCWriteType\n", __FUNCTION__);
#ifndef SHUTDOWN_IMPROVEMENT_STAGE2
        DBG_Getch();
#endif
    }
#endif

    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);

    if (L2_Event.EventShutDown)
    {
       //DBG_Printf("L2_SetTLCMergeInfo(EventShutDown) L2_ResetTLCManager(ucSuperPu = %d)\n", ucSuperPu);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
       if (TLC_WRITE_ALL == g_TLCManager->aeTLCWriteType[ucSuperPu])
#endif
       L2_ResetTLCManager(ucSuperPu);       
       
       if (TLC_WRITE_HOSTWRITE == eWriteType)
       {
           L2_FTLTaskTLCMergeClear(ucSuperPu);
           L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC);
       }
       else if (TLC_WRITE_TLCGC == eWriteType)
       {
           L2_FTLTaskTLCGCClear(ucSuperPu);
           L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
           if ((TLC_WRITE_TLCGC != g_TLCManager->aeTLCWriteType[ucSuperPu]) &&
               (TLC_WRITE_SWL != g_TLCManager->aeTLCWriteType[ucSuperPu]))
#endif
               L2_InitGCManager(ucSuperPu, TLCGC_MODE);
       }
       else if (TLC_WRITE_SWL == eWriteType)
       {
           L2_FTLTaskTLCSWLClear(ucSuperPu);
           L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);
           
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
           if ((TLC_WRITE_TLCGC != g_TLCManager->aeTLCWriteType[ucSuperPu]) &&
               (TLC_WRITE_SWL != g_TLCManager->aeTLCWriteType[ucSuperPu]))
#endif
               g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_PREPARE_GC; //reset m_GCStage

           //here keep the current dst blk and save it in PBIT
           if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu])
           {
               L2_PBIT_Set_Lock(ucSuperPu, gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu], FALSE);
               gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
           }
#ifdef NEW_SWL
           if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
           {
               L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
               gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
           }
#endif
       }
       
       return TRUE;
    }

    if (TLC_WRITE_HOSTWRITE== eWriteType)
    {
        for (i = 0; i < PG_PER_WL; i++)
        {
#ifdef SIM
            if (INVALID_4F != g_TLCManager->ausSrcBlk[ucSuperPu][i])
            {
                DBG_Printf("[%s] INVALID_4F != g_TLCManager->ausSrcBlk\n", __FUNCTION__);
                DBG_Getch();
            }
#endif

            /* Select GCSrcBlk from forceGC Queue first */
            usGCSrcBlk = L2_BlkQueuePopBlock(ucSuperPu, g_pForceGCSrcBlkQueue[ucSuperPu], ucSrcVBTType, FALSE);

            /* If forceGC Queue have not needed blk, follow select MinSN blk */
            if (INVALID_4F == usGCSrcBlk)
            {
                usGCSrcBlk = L2_SelectGCSrcBlkMinSN(ucSuperPu, FALSE, ucSrcVBTType);
            }

            /* Exit TLC Merge : if we couldn't figure out three SLC source blocks */
            if (INVALID_4F == usGCSrcBlk)
            {
                U16 j;
                DBG_Printf("TLC Merge Exit : SrcVBTType %d : INVALID_4F == usGCSrcBlk\n", ucSrcVBTType);
                for (j = 0; j < i; j++)
                {
                    L2_PBIT_Set_Lock(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][j], FALSE);
                }

                L2_ResetTLCManager(ucSuperPu);
                L2_FTLTaskTLCMergeClear(ucSuperPu);
                L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC);
                return TRUE;
            }

            g_TLCManager->ausSrcBlk[ucSuperPu][i] = usGCSrcBlk;
            /* lock GC src block */
            L2_PBIT_Set_Lock(ucSuperPu, usGCSrcBlk, TRUE);
        }

        g_TLCManager->ausDesTLCBlk[ucSuperPu] = g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE];
        //FIRMWARE_LogInfo("Merge SuperPU %d TLCWRITE BLK 0x%x_0x%x \n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], pVBT[ucSuperPu]->m_VBT[g_TLCManager->ausDesTLCBlk[ucSuperPu]].PhysicalBlockAddr[0]);

        //DBG_Printf("L2_SetTLCMergeInfo(TLC_WRITE_HOSTWRITE) SRC[%d][%d][%d] DEST[%d]\n",
        //            g_TLCManager->ausSrcBlk[ucSuperPu][0], g_TLCManager->ausSrcBlk[ucSuperPu][1], g_TLCManager->ausSrcBlk[ucSuperPu][2], g_TLCManager->ausDesTLCBlk[ucSuperPu]);

        //GigaHsu 2016.9.13 : 3D_TLC : Use ExternalCopy instead of InternalCopy first.
        g_TLCManager->bTLCInternalW[ucSuperPu] = FALSE;
        
    }
    else if (TLC_WRITE_SWL== eWriteType)
    {
        g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
#ifdef NEW_SWL
        U8 ucLunInSuperPu = 0;
        U16 usBuffDstBlkEC, ulCurDstBlkEC;
        U32 ulPhyBlkDst = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
        U32 ulPhyBlkDstBuf = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
        if (INVALID_4F != gwl_info->nDstBlkBuf[ucSuperPu])
        {
            usBuffDstBlkEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][ulPhyBlkDstBuf].EraseCnt;
            ulCurDstBlkEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][ulPhyBlkDst].EraseCnt;
            if (usBuffDstBlkEC > ulCurDstBlkEC)
            {
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
                gwl_info->nDstBlk[ucSuperPu] = gwl_info->nDstBlkBuf[ucSuperPu];
                gwl_info->nDstBlkBuf[ucSuperPu] = INVALID_4F;
            }
        }
        g_TLCManager->ausDesTLCBlk[ucSuperPu] = gwl_info->nDstBlk[ucSuperPu];
        for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
        {
            PBN = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[j];
            pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bFree = FALSE;
            pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bAllocated = TRUE;
            L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, PBN, INVALID_8F);
        }
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].StripID = ucSuperPu;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].Target = VBT_TARGET_TLC_W;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].VBTType = VBT_TYPE_TLCW;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].bFree = FALSE;
        g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC]++;
        L2_PBIT_Increase_AllocBlockCnt(ucSuperPu, BLKTYPE_TLC);
#else
        g_TLCManager->ausDesTLCBlk[ucSuperPu] = gwl_info->nDstBlk[ucSuperPu];
        for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
        {
            PBN = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[j];
            pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bFree = FALSE;
            pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bAllocated = TRUE;
            L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, PBN, INVALID_8F);
        }
        g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC]++;
        L2_PBIT_Increase_AllocBlockCnt(ucSuperPu, BLKTYPE_TLC);
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].StripID = ucSuperPu;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].Target = VBT_TARGET_TLC_W;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].VBTType = VBT_TYPE_TLCW;
        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].bFree = FALSE;
#endif
        g_TLCManager->bTLCInternalW[ucSuperPu] = FALSE;
        //DBG_Printf("L2_SetTLCMergeInfo(TLC_WRITE_SWL) SRC[%d] DEST[%d]\n", g_TLCManager->ausSrcBlk[ucSuperPu][0], g_TLCManager->ausDesTLCBlk[ucSuperPu]);
        FIRMWARE_LogInfo("SWL SuperPU %d TLCWRITE BLK 0x%x_0x%x \n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], pVBT[ucSuperPu]->m_VBT[g_TLCManager->ausDesTLCBlk[ucSuperPu]].PhysicalBlockAddr[0]);
    }
    else//TLC_WRITE_TLCGC
    {
        g_TLCManager->ausSrcBlk[ucSuperPu][0] = INVALID_4F;
        g_TLCManager->ausDesTLCBlk[ucSuperPu] = g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE];
        g_TLCManager->bTLCInternalW[ucSuperPu] = FALSE;
        //DBG_Printf("L2_SetTLCMergeInfo(TLC_WRITE_TLCGC) DEST[%d]\n", g_TLCManager->ausDesTLCBlk[ucSuperPu]);
        FIRMWARE_LogInfo("TLCGC SuperPU %d TLCWRITE BLK 0x%x_0x%x \n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], pVBT[ucSuperPu]->m_VBT[g_TLCManager->ausDesTLCBlk[ucSuperPu]].PhysicalBlockAddr[0]);
    }

    g_TLCManager->aeTLCWriteType[ucSuperPu] = eWriteType;

    g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_LOADRPMT;

    return FALSE;
}

#ifdef L2_HANDLE_UECC
#if 0
BOOL L2_TLCCheckUECCStatus(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;
    U8 ucBlkIndex;
    U16 ulSrcBlk;
    U16 ulUECCBlkIndex;
    BOOL bRet = TRUE;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (0 != g_TLCManager->aulErrUECCLUNBitMap[ucSuperPu][ucLunInSuperPu])
        {
            for (ucBlkIndex = 0; ucBlkIndex < PG_PER_WL; ucBlkIndex++)
            {
                if (0 != (g_TLCManager->aulErrUECCLUNBitMap[ucSuperPu][ucLunInSuperPu] & (1 << ucBlkIndex)))
                {
                    if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPu])
                    {
                        ulSrcBlk = g_TLCManager->ausSrcBlk[ucSuperPu][ucBlkIndex];
                    }
                    else
                    {
                        ulSrcBlk = g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] - 1];
                    }

                    for (ulUECCBlkIndex = 0; ulUECCBlkIndex < g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunInSuperPu]; ulUECCBlkIndex++)
                    {
                        if (ulSrcBlk == g_TLCManager->ausUECCBlk[ucSuperPu][ucLunInSuperPu][ulUECCBlkIndex])
                        {
                            break;
                        }
                    }
                    if ((ulUECCBlkIndex == g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunInSuperPu]) && (ulUECCBlkIndex < L2_HANDLE_UECCBLK_PERLUN_MAX))
                    {
                        DBG_Printf("L2_TLCCheckUECCStatus:  Spu=%d LUN %d Blk %d UECC \n", ucSuperPu, ucLunInSuperPu, ulSrcBlk);
                        g_TLCManager->ausUECCBlk[ucSuperPu][ucLunInSuperPu][ulUECCBlkIndex] = ulSrcBlk;
                        g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunInSuperPu]++;
                    }
                    g_TLCManager->aulErrUECCLUNBitMap[ucSuperPu][ucLunInSuperPu] &= ~(1 << ucBlkIndex);
                }
            }
            bRet = FALSE;
        }
    }

    return bRet;
}
#endif
void L2_AddUECCBlock(U8 ucSuperPu, U8 ucLun, U16 usBlk)
{
    U16 ulUECCBlkIndex;

    for (ulUECCBlkIndex = 0; ulUECCBlkIndex < g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLun]; ulUECCBlkIndex++)
    {
        if (usBlk == g_TLCManager->ausUECCBlk[ucSuperPu][ucLun][ulUECCBlkIndex])
        {
            break;
        }
    }
    if ((ulUECCBlkIndex == g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLun]) && (ulUECCBlkIndex < L2_HANDLE_UECCBLK_PERLUN_MAX))
    {
        DBG_Printf("L2_AddUECCBlock:  Spu=%d LUN %d Blk %d UECC \n", ucSuperPu, ucLun, usBlk);
        g_TLCManager->ausUECCBlk[ucSuperPu][ucLun][ulUECCBlkIndex] = usBlk;
        g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLun]++;
    }

    return;
}

BOOL L2_NeedHandleUECC(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (0 != g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunInSuperPu])
        {
            return TRUE;
        }
    }

    return FALSE;
}

void L2_HandleUECC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucErrLun;
    U8 ucLunOffset;
    U8 ucBlkIndex;
    U8 ucTLun;
    U8 ucReplaceLUN;
    TLCErrH *pErrStage;
    BOOL bTLCBlk;
    BOOL bAllDirtyBlk;
    BOOL bReplaceBlk = FALSE;
    U16 usAllDirtyBlkIndex;
    U16 usSrcBlk;
    U16 usUECCBlkCnt;

    pErrStage = &g_TLCManager->aeErrHandleUECCStage[ucSuperPu];

    switch (*pErrStage)
    {
    case TLC_ERRH_ALLOCATE_NEWBLK:

        for (ucLunOffset = 0; ucLunOffset < LUN_NUM_PER_SUPERPU; ucLunOffset++)
        {
            usUECCBlkCnt = g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunOffset];
            if (0 != usUECCBlkCnt)
            {
                ucErrLun = ucLunOffset;
                for (ucBlkIndex = 0; ucBlkIndex < usUECCBlkCnt; ucBlkIndex++)
                {
                    usSrcBlk = g_TLCManager->ausUECCBlk[ucSuperPu][ucLunOffset][ucBlkIndex];
                    if (((g_TLCManager->aeTLCWriteType[ucSuperPu] == TLC_WRITE_TLCGC) || (g_TLCManager->aeTLCWriteType[ucSuperPu] == TLC_WRITE_SWL)) && (ucBlkIndex == (usUECCBlkCnt - 1)))
                    {
                        bAllDirtyBlk = FALSE;
                        for (usAllDirtyBlkIndex = 0; usAllDirtyBlkIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; usAllDirtyBlkIndex++)
                        {
                            if (usSrcBlk == g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][usAllDirtyBlkIndex])
                            {
                                bAllDirtyBlk = TRUE;
                            }
                        }
                    }
                    else
                    {
                        bAllDirtyBlk = TRUE;
                    }
                    if (bAllDirtyBlk == TRUE)
                    {
                        /* allocate new phy block */
                        bTLCBlk = L2_VBT_Get_TLC(ucSuperPu, usSrcBlk);
                        L2_ErrHReplaceBLK(ucSuperPu, ucErrLun, usSrcBlk, bTLCBlk, READ_ERR);
                        bReplaceBlk = TRUE;
                        ucReplaceLUN = ucErrLun;
                    }
                    g_TLCManager->ausUECCBlkCnt[ucSuperPu][ucLunOffset]--;
                }
            }
        }
        *pErrStage = TLC_ERRH_SAVE_BBT;

    case TLC_ERRH_SAVE_BBT:
        if (FALSE == bReplaceBlk)
        {
            *pErrStage = TLC_ERRH_ALL;
        }
        else
        {
            ucTLun = L2_GET_TLUN(ucSuperPu, ucReplaceLUN);
            L2_BbtSetLunSaveBBTBitMap(ucTLun, TRUE);
            CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
            *pErrStage = TLC_ERRH_WAIT_BBT;
        }
        break;

    case TLC_ERRH_WAIT_BBT:
        if (TRUE == L2_BbtIsSavedDone())
        {
            *pErrStage = TLC_ERRH_ALL;
        }
        else
        {
            break;
        }
        break;

    case TLC_ERRH_ALL:
    default:
        DBG_Printf("L2_HandleUECC ErrH stage error\n");
        DBG_Getch();
        break;
    }

    return;
}
#endif

/****************************************************************************
Name        :L2_TLCLoadRPMT
Input       :U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap
Output      :
Description : TLC merge, read 3 SLC block's RPMT page to g_TLCManager->pRPMT buffer
History    :
1. 2015.05.22 zoewen create
****************************************************************************/
TLCStage L2_TLCLoadRPMT(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 *pucRPMTNum;
    U16 usSrcVBN;

    pucRPMTNum = &g_TLCManager->aucRPMTNum[ucSuperPu];

    usSrcVBN = g_TLCManager->ausSrcBlk[ucSuperPu][(*pucRPMTNum)];

    L2_LoadRPMT(ucSuperPu, ulLunAllowToSendFcmdBitmap, *pucRPMTNum, usSrcVBN, VBT_TARGET_TLC_W, FALSE);

    if (SUPERPU_LUN_NUM_BITMSK == g_TLCManager->aucTLCReadBitMap[ucSuperPu])
    {
        g_TLCManager->aucTLCReadBitMap[ucSuperPu] = 0;

        (*pucRPMTNum)++;
        if ((*pucRPMTNum) >= PG_PER_WL)
        {
            return TLC_STAGE_CHECK_RPMTSTATUS;
        }
        else
        {
            return TLC_STAGE_LOADRPMT;
        }
    }
    else
    {
        return TLC_STAGE_LOADRPMT;
    }
}

TLCStage L2_TLCCheckRPMTStatus(U8 ucSuperPu)
{    
#ifndef L3_UNIT_TEST
    U8 ucLunInSuperPu, ucPGO;
    TLCWriteType eWriteSrc = g_TLCManager->aeTLCWriteType[ucSuperPu];

    /* check whether the RPMT has been loaded successfully at the first copy beginning */
    if ((0 == g_TLCManager->ausTLCProgOrder[ucSuperPu]))
    {
        for (ucPGO = 0; ucPGO < PG_PER_WL; ucPGO++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (SUBSYSTEM_STATUS_PENDING == g_TLCManager->aucRPMTStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
                {
                    // return immediately if one of the RPMTs hasn't been loaded yet
                    return TLC_STAGE_CHECK_RPMTSTATUS;
                }
                else if (SUBSYSTEM_STATUS_FAIL == g_TLCManager->aucRPMTStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
                {
                    /* trigger error handling to rebuild RPMT for TLC write */
                    g_TLCMergeErrHandle[ucSuperPu].m_ErrPU = ucSuperPu;
                    g_TLCMergeErrHandle[ucSuperPu].m_ucLun = ucLunInSuperPu;
                    g_TLCMergeErrHandle[ucSuperPu].m_ucErrRPMTnum = ucPGO;
                    g_TLCMergeErrHandle[ucSuperPu].m_ErrBlock = g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO];
                    
                    DBG_Printf("ucSuperPu %d ucLunInSuperPu %d RPMTnum %d block %d load rpmt fail in TLC write, start to do L2 error handling.\n", 
                        ucSuperPu, ucLunInSuperPu, ucPGO, g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO]);
                    g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_ERRH_RPMT;
#ifdef L2_HANDLE_UECC
                    L2_AddUECCBlock(ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO]);
#endif
                    return TLC_STAGE_ERRH_RPMT;
                }

                // check the content of the load RPMT to ensure its validity
                if ((g_TLCManager->pRPMT[ucSuperPu][ucPGO]->m_RPMT[ucLunInSuperPu].m_SuperPU != ucSuperPu) || (g_TLCManager->pRPMT[ucSuperPu][ucPGO]->m_RPMT[ucLunInSuperPu].m_SuperBlock != g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO]))
                {
                    U16 usPhyBlock;
                    usPhyBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO]);
                    DBG_Printf("[L2_TLCMerge] PU %d block 0x%x (PBN 0x%x)WL 0x%x Load wrong RPMT  !!!\n", ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucPGO], usPhyBlock, ucPGO);
                    DBG_Getch();
                }
            }
        }
    } 
#endif

    if (FALSE == g_TLCManager->bTLCInternalW[ucSuperPu])
    {
        return TLC_STAGE_READSRC;
    }
    else
    {
        return TLC_STAGE_WRITETLC;
    }
}

void L2_UpdateTLCRedandRPMT(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucPageType, U8 ucProgPageCnt, U16 uc1stPageNum, U16 uc2ndPageNum, RED *pSpare, BOOL bUpdateRed)
{
    U8 ucTLun, ucInValidLPNCnt, ucLPNO;
    U8 i, j;
    BOOL Ret = FALSE;
    RED *pCurSpare = pSpare;   
    U8 ucSrcBlkIndex;
    U16 usSrcPgIndex, usPageNum, usSLCBlk;
    U32 ulTimeStamp,ulLPNMap;
    TLCWriteType tTLCWritetype;
    GCComStruct* ptCom;
    PhysicalAddr RefAddr,SrcAddr;
    U32 LPN,DirtyCnt = 0;
    U32* pRPMTAddr;
    U32 ulTargetOffsetTS;
#ifdef SIM
    U32 ulTLCGCSrcIndex;
#endif
    //in this interface, take SWL(copyvalid) as TLCGC
    if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        tTLCWritetype = TLC_WRITE_TLCGC;
    }
    else
    {
        tTLCWritetype = g_TLCManager->aeTLCWriteType[ucSuperPu];
    }
    
    ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
    ulTimeStamp = L2_GetTimeStampInPu(ucSuperPu);
    ulTargetOffsetTS = L2_GetTargetOffsetTS(&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE]);

    for (i = 0; i < ucProgPageCnt; i++)
    {
        if (i == 0)
        {
            usPageNum = uc1stPageNum;
        }
        else
        {
            usPageNum = uc2ndPageNum;
        }
        
        if (TLC_WRITE_HOSTWRITE == tTLCWritetype)
        {
#ifndef FLASH_IM_3DTLC_GEN2
            if ((ucPageType != L2_EXTRA_PAGE) || (i != 1))
            {
#endif
                if (usPageNum < TLC_FIRST_RPMT_PAGE_NUM)
                {
                    ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                    usSLCBlk = g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex];
                    usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);
                
                    // calculate the number of invalid LPNs in the current word line of the SLC source block
#if (LPN_PER_BUF == 8)
                    ulLPNMap = ~(0xffffff00 | pDPBM->m_LpnMap[ucSuperPu][usSLCBlk].m_LpnMapPerPage[ucLunInSuperPu][usSrcPgIndex]);
#elif (LPN_PER_BUF == 16)                
                    ulLPNMap = ~(0xffff0000 | pDPBM->m_LpnMap[ucSuperPu][usSLCBlk].m_LpnMapPerPage[ucLunInSuperPu][usSrcPgIndex]);
#endif                
                    ucInValidLPNCnt = HAL_POPCOUNT(ulLPNMap);

                    if (0 != ucInValidLPNCnt)
                    {
                        HAL_Wclzstate(ulLPNMap);
                        for (j = 0; j < ucInValidLPNCnt; j++)
                        {
                            ucLPNO = (31 - HAL_SCLZ());
                            if (TRUE == bUpdateRed)
                            {
                                pCurSpare->m_DataRed.aCurrLPN[ucLPNO] = INVALID_8F;
                            }
                            g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_RPMTItems[usSrcPgIndex* LPN_PER_BUF + ucLPNO] = INVALID_8F;
                        }
                    }

                    g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usSrcPgIndex] = ulTimeStamp;
                    g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usSrcPgIndex] = ulTargetOffsetTS;
                }
                // 127 word line process
                // Page 1533/1534/1535
                else
                {              
                    g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperPU = ucSuperPu;
                    g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperBlock = g_TLCManager->ausDesTLCBlk[ucSuperPu];
                    g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[PG_PER_SLC_BLK - 1] = ulTimeStamp;
                    g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[PG_PER_SLC_BLK - 1] = ulTargetOffsetTS;
                  
                    COM_MemSet((U32 *)&g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_RPMTItems[(PG_PER_SLC_BLK - 1) * LPN_PER_BUF], LPN_PER_BUF, INVALID_8F);
                }
#ifndef FLASH_IM_3DTLC_GEN2
            }
            else
            {
                if (usPageNum < TLC_FIRST_RPMT_PAGE_NUM)
                {
                    ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                    usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);
                }
            }
#endif
        }
        else if (TLC_WRITE_TLCGC == tTLCWritetype)
        {
#ifndef FLASH_IM_3DTLC_GEN2
            if((ucPageType != L2_EXTRA_PAGE) || (i != 1))
            {
#endif
               if(usPageNum < TLC_FIRST_RPMT_PAGE_NUM)
               {
                  ptCom = &g_GCManager[TLCGC_MODE]->tGCCommon;

                  ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);                
                  usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);
                
                  for (j = ucLunInSuperPu*LPN_PER_BUF; j < (ucLunInSuperPu + 1)*LPN_PER_BUF; j++)
                  {
                      LPN = ptCom->m_LPNInBuffer[ucSuperPu][(uc1stPageNum + i) % TLC_BUF_CNT][j];
                      if (LPN == INVALID_8F)
                      {
                        DirtyCnt++;
                        continue;
                      }
                      L2_LookupPMT(&RefAddr, LPN, FALSE);

                      SrcAddr.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][ucSrcBlkIndex][usSrcPgIndex*LPN_PER_SUPERBUF + j].m_PPN;
                      if (RefAddr.m_PPN != SrcAddr.m_PPN)
                      {
                          ptCom->m_LPNInBuffer[ucSuperPu][(uc1stPageNum + i) % TLC_BUF_CNT][j] = INVALID_8F;
                          DirtyCnt++;
#ifdef SIM
                          for (ulTLCGCSrcIndex = 0; ulTLCGCSrcIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; ulTLCGCSrcIndex++)
                          {
                              if (SrcAddr.m_BlockInPU == g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulTLCGCSrcIndex])
                              {
                                  g_pTLCGCSrcBlkRecord->m_NewAddDirtyCnt[ucSuperPu][ulTLCGCSrcIndex]++;
                              }
                          }
#endif
                      }
                  }

                  pRPMTAddr = (U32 *)&g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_RPMTItems[usSrcPgIndex * LPN_PER_BUF];
                  COM_MemCpy(pRPMTAddr, &ptCom->m_LPNInBuffer[ucSuperPu][(uc1stPageNum + i) % TLC_BUF_CNT][ucLunInSuperPu*LPN_PER_BUF], LPN_PER_BUF);                
                
                  if (LPN_PER_BUF == DirtyCnt)
                  {
                    //DBG_Printf("TLCGC WriteBuffer WL0x%x PGInWL%d all dirty when write\n", ucWL, i);
                  }
                  g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usSrcPgIndex] = ulTimeStamp;
                  g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usSrcPgIndex] = ulTargetOffsetTS;                  
               }
               else// Page 1533/1534/1535
               {                
                   g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperPU = ucSuperPu;
                   g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperBlock = g_TLCManager->ausDesTLCBlk[ucSuperPu];
                   g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[PG_PER_SLC_BLK - 1] = ulTimeStamp;
                   g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[PG_PER_SLC_BLK - 1] = ulTargetOffsetTS;
                 
                   COM_MemSet((U32 *)&g_TLCManager->pRPMT[ucSuperPu][usPageNum - TLC_FIRST_RPMT_PAGE_NUM]->m_RPMT[ucLunInSuperPu].m_RPMTItems[(PG_PER_SLC_BLK - 1) * LPN_PER_BUF], LPN_PER_BUF, INVALID_8F);
               }
#ifndef FLASH_IM_3DTLC_GEN2
            }
            else
            {
                if (usPageNum < TLC_FIRST_RPMT_PAGE_NUM)
                {
                    ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                    usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);
                }
            }
#endif
        }
        else
        {
            DBG_Printf("L2_UpdateTLCRedandRPMT WriteType error\n");
            DBG_Getch();
        }

        if (TRUE == bUpdateRed)
        {
#ifndef FLASH_IM_3DTLC_GEN2
            if ((ucPageType == L2_EXTRA_PAGE) && (i == 1))
            {
                //Extra's upper page need rollback Spare TimeStamp from RPMT data structure
                pCurSpare->m_RedComm.ulTimeStamp = g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usSrcPgIndex];
                pCurSpare->m_RedComm.ulTargetOffsetTS = g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_LunOrderTS[usSrcPgIndex];
            }
            else
#endif
            {
                pCurSpare->m_RedComm.ulTimeStamp = ulTimeStamp;
                pCurSpare->m_RedComm.ulTargetOffsetTS = ulTargetOffsetTS;
            }
         
            pCurSpare->m_RedComm.bsVirBlockAddr = g_TLCManager->ausDesTLCBlk[ucSuperPu];

            pCurSpare->m_RedComm.bcBlockType = (TLC_WRITE_TLCGC != tTLCWritetype) ? BLOCK_TYPE_TLC_W : BLOCK_TYPE_TLC_GC;
            if (usPageNum < TLC_FIRST_RPMT_PAGE_NUM)
            {
                /*update Redundant LPN info*/
                if (TLC_WRITE_TLCGC == tTLCWritetype)
                {
#ifndef FLASH_IM_3DTLC_GEN2
                    if ((ucPageType == L2_EXTRA_PAGE) && (i == 1))
                    {
                        //Extra's upper page need rollback Spare LPN data from RPMT data structure
                        COM_MemCpy(pCurSpare->m_DataRed.aCurrLPN, &g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_RPMTItems[usSrcPgIndex * LPN_PER_BUF], LPN_PER_BUF);
                    }
                    else
#endif
                    {
                        COM_MemCpy(pCurSpare->m_DataRed.aCurrLPN, &ptCom->m_LPNInBuffer[ucSuperPu][(uc1stPageNum + i) % TLC_BUF_CNT][ucLunInSuperPu*LPN_PER_BUF], LPN_PER_BUF);
                    }
                }
                pCurSpare->m_RedComm.bcPageType = PAGE_TYPE_DATA;
                pCurSpare->m_RedComm.eOPType = OP_TYPE_GC_WRITE;          
            }
            else
            {
                    for (j = 0; j < LPN_PER_BUF; j++)
                    {
                        pCurSpare->m_DataRed.aCurrLPN[j] = INVALID_8F;
                    }

                    pCurSpare->m_DataRed.m_uTLCFirstPageTS = g_TLCManager->aFirstPageRealTS[ucSuperPu];
                    pCurSpare->m_RedComm.bcPageType = PAGE_TYPE_RPMT;
                    pCurSpare->m_RedComm.eOPType = OP_TYPE_RPMT_WRITE;
            }

            if (TLC_WRITE_TLCGC == tTLCWritetype)
            {
#ifndef FLASH_IM_3DTLC_GEN2
                if (i == 1)
                {
                    //Table Rebuild:Please don't refer Upper Page [bsTLCGC1stSrcBlock, ulTLCGC1stSrcBlockTS]
                    pCurSpare->m_DataRed.bsTLCGC1stSrcBlock = 0;
                    pCurSpare->m_DataRed.ulTLCGC1stSrcBlockTS = 0;
                }
                else
#endif
                {
                    pCurSpare->m_DataRed.bsTLCGC1stSrcBlock = (0 == g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]) ? g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] : g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][0];
                    pCurSpare->m_DataRed.ulTLCGC1stSrcBlockTS = g_pTLCGCSrcBlkRecord->m_TLCGC1stSrcBlkTS[ucSuperPu];
                }
            }
        }
        
        if (NULL != pCurSpare)
        {
            pCurSpare++;
            if (TLC_WRITE_HOSTWRITE == tTLCWritetype)
            {
                bUpdateRed = FALSE;
            }
        }
    }
    return;
}


BOOL L2_TLCWaitReadStatus(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;
    U16 ucCurPage;
    U8 usPGO;
    U8 ucPageType, ucProgPageCnt;
    U16 uc1stPageNum, uc2ndPageNum;
    U32 ulPage = 0;
    U8 ucLPNOffset;
    U16 usPageIndex = 0;
    U32 ulLPN = 0;
    RED * pSpare;
    PhysicalAddr Addr = { 0 };
    PhysicalAddr RefAddr = { 0 };

    uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);
            
    if (ucProgPageCnt == 2)
    {
        uc2ndPageNum = L2_Get2ndPageNumByProgOrder(ucSuperPu);
    }
   
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (usPGO = 0; usPGO < ucProgPageCnt; usPGO++)
        {
            if (usPGO == 0)
                ucCurPage = uc1stPageNum;
            else
                ucCurPage = uc2ndPageNum;

            pSpare = g_TLCManager->atSpare[ucSuperPu][ucLunInSuperPu][usPGO];

            /* if RPMT red read fail rebuild redundant info*/
            if ((ucCurPage >= TLC_FIRST_RPMT_PAGE_NUM) &&
                g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][usPGO] == SUBSYSTEM_STATUS_FAIL)
            {
                for (ucLPNOffset = 0; ucLPNOffset < LPN_PER_BUF; ucLPNOffset++)
                {
                    pSpare->m_DataRed.aCurrLPN[ucLPNOffset] = INVALID_8F;
                }
                pSpare->m_RedComm.bsVirBlockAddr = g_TLCManager->ausSrcBlk[ucSuperPu][0];
                pSpare->m_RedComm.bcBlockType = BLOCK_TYPE_ERROR;
                pSpare->m_RedComm.bcPageType = PAGE_TYPE_RPMT;
                pSpare->m_RedComm.ulTimeStamp = 0;
                pSpare->m_RedComm.ulTargetOffsetTS = 0;
            }

            /*if DataPage & RED both UECC */
            if ((ucCurPage < TLC_FIRST_RPMT_PAGE_NUM) &&
                (g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][usPGO] == SUBSYSTEM_STATUS_FAIL))
            {        
#if 0
                U8 ucPrefix;

                Addr.m_PUSer = ucSuperPu;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][0];
                Addr.m_PageInBlock = ucCurPage;
                ucPrefix = L2_GetInterCopySrcBlkIndex(ucCurPage);
                pRPMT = g_TLCManager->pRPMT[ucSuperPu][ucPrefix];
                usPageIndex = L2_GetInterCopyPgIndex(ucCurPage);

                for (ucLPNOffset = 0; ucLPNOffset < LPN_PER_BUF; ucLPNOffset++)
                {
                    Addr.m_LPNInPage = ucLPNOffset;
                    ulLPN = pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[usPageIndex*LPN_PER_BUF + ucLPNOffset];
                    if (INVALID_8F == ulLPN)
                    {
                        /*Skip UECC Pages which are prcessed in RPMT Uecc ErrorHanding*/
                        continue;
                    }
                    else
                    {
                        /*meet DataPage UECC && this page UECC info didn't record in RPMT when ErrorHanding*/
                        L2_LookupPMT(&RefAddr, ulLPN, FALSE);
                        if (RefAddr.m_PPN == Addr.m_PPN)
                        {
                            if (TRUE == L2_LookupDirtyLpnMap(&RefAddr))
                            {
                                PhysicalAddr NewAddr = { 0 };

                                L2_UpdateDirtyLpnMap(&Addr, B_DIRTY);
                                L2_IncreaseDirty(Addr.m_PUSer, Addr.m_BlockInPU, 1);

                                NewAddr.m_PPN = INVALID_8F;
                                L2_UpdatePMT(&NewAddr, NULL, ulLPN);
                            #ifdef DBG_PMT
                                L2_UpdateDebugPMT(&NewAddr, ulLPN);
                            #endif

                            #ifdef SIM
                                DBG_Printf("MCU#%d L2_TLCWaitReadStatus Invalid LPN 0x%x SuperPU %d LUNOffset %d Blk 0x%x PPO 0x%x LPNInBuf %d DirtyCnt %d\n", HAL_GetMcuId(), ulLPN, Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock, RefAddr.m_LPNInPage, L2_GetDirtyCnt(Addr.m_PUSer, Addr.m_BlockInPU));
                            #endif
                            }
                        }
                    }

                    /*set Redundant LPN info INVALID*/
                    pSpare->m_DataRed.aCurrLPN[ucLPNOffset] = INVALID_8F;
                }
#endif
                DBG_Printf("TLC SWL read src data UECC SrcBlk 0x%x Pg %d\n", g_TLCManager->ausSrcBlk[ucSuperPu][0], ucCurPage);
                pSpare->m_RedComm.bsVirBlockAddr = g_TLCManager->ausSrcBlk[ucSuperPu][0];
                pSpare->m_RedComm.bcBlockType = BLOCK_TYPE_ERROR;
                pSpare->m_RedComm.bcPageType = PAGE_TYPE_DATA;
                pSpare->m_RedComm.ulTimeStamp = 0;
                pSpare->m_RedComm.ulTargetOffsetTS = 0;
            }

            if (SUBSYSTEM_STATUS_PENDING == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][usPGO])
            {
                return FAIL;
            }

            if (SUBSYSTEM_STATUS_EMPTY_PG == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][usPGO])
            {
                DBG_Printf("SPU %d WL read EmptyPage\n", ucSuperPu);
                DBG_Getch();
            }
        }
    }

    return SUCCESS;
}

BOOL L2_TLCCheckWriteStatus(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;

    /* check write status */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
         if (SUBSYSTEM_STATUS_PENDING == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT])
         {
             return FALSE;
         }
    }

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (SUBSYSTEM_STATUS_FAIL == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT])
        {
            // TLC program fails, we have to initiate the TLC program error handling process
            DBG_Printf("L2_TLCCheckWriteStatus FAIL SuperPu %d LunInSuperPu %d Blk %d TLCWriteType %d\n", ucSuperPu, ucLunInSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], g_TLCManager->aeTLCWriteType[ucSuperPu]);

            g_TLCManager->aulErrProgBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
        }
    }

    if (0 != g_TLCManager->aulErrProgBitMap[ucSuperPu])
    {
        g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_ERRH_PROG;
        g_TLCManager->aeErrHandleStage[ucSuperPu] = TLC_ERRH_ALLOCATE_NEWBLK;

        return FALSE;
    }

    return TRUE;
}

BOOL MCU1_DRAM_TEXT L2_TLCCheckEraseStatus(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;
    U16 usTLCGCSrcMax, usTLCGCSrcVBN, i;
    TLCWriteType eWriteType;

    /* check write status */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
         if (SUBSYSTEM_STATUS_PENDING == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT])
         {
             return FALSE;
         }
    }

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (SUBSYSTEM_STATUS_FAIL == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT])
        {           
            DBG_Printf("L2_TLCCheckEraseStatus(Failed) SuperPu %d LunInSuperPu %d Blk %d TLCWriteType %d\n", ucSuperPu, ucLunInSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], g_TLCManager->aeTLCWriteType[ucSuperPu]);

            g_TLCManager->aulErrProgBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
        }
    }

    if (0 != g_TLCManager->aulErrProgBitMap[ucSuperPu])
    {
       //L3 : Handle erase failed case.
       //return FALSE;
    }

    eWriteType = g_TLCManager->aeTLCWriteType[ucSuperPu];

    /* unlock source blk */
    if (TLC_WRITE_HOSTWRITE == eWriteType)
    {
        for (i = 0; i < PG_PER_WL; i++)
        {
            L2_PBIT_Set_Lock(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][i], FALSE);
        }
    }
    else if ((TLC_WRITE_TLCGC == eWriteType) || (TLC_WRITE_SWL == eWriteType))
    {
        usTLCGCSrcMax = g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu];

        for (i = 0; i < usTLCGCSrcMax; i++)
        {
            usTLCGCSrcVBN = g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][i];
            if (INVALID_4F == usTLCGCSrcVBN)
            {
                continue;
            }

            g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][i] = INVALID_4F;
            L2_PBIT_Set_Lock(ucSuperPu, usTLCGCSrcVBN, FALSE);
        }

        usTLCGCSrcVBN = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu];
        if (INVALID_4F != usTLCGCSrcVBN)
        {
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
            L2_PBIT_Set_Lock(ucSuperPu, usTLCGCSrcVBN, FALSE);
        }
        g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] = 0;
    }
    return TRUE;
}

/****************************************************************************
Name        :L2_TLCHandleProgFail
Input       :U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap
Output      :
Description : Error handling for Program fail
History    :
1. 2015.08.13 zoewen create
2. 20170510 TobeyTAN no need to allocate one new TLC Target, just finish coresponding task.
****************************************************************************/
void L2_TLCErrHPutTargetIntoEraseQueue(U8 ucSuperPu)
{
    U16 usNewDes = INVALID_4F;
    U32 ulSrcBlkIndex;

    /* SWL special process */
    if (g_TLCManager->ausDesTLCBlk[ucSuperPu] == gwl_info->nDstBlk[ucSuperPu])
    {
        L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
        gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;

        if (INVALID_4F != gwl_info->nDstBlkBuf[ucSuperPu])
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlkBuf[ucSuperPu], FALSE);
            gwl_info->nDstBlkBuf[ucSuperPu] = INVALID_4F;
        }

        if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
        }
    }

    /* TLC Merge special: unlock 3 SrcSLCBlock */
    for (ulSrcBlkIndex = 0; ulSrcBlkIndex < PG_PER_WL; ulSrcBlkIndex++)
    {
        L2_PBIT_Set_Lock(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ulSrcBlkIndex], FALSE);
    }

    /* put current Des block into erase queue */
    pVBT[ucSuperPu]->m_VBT[g_TLCManager->ausDesTLCBlk[ucSuperPu]].Target = VBT_NOT_TARGET;

    /*Note: erase open block may not cleanly from flash management view*/
    L2_InsertBlkIntoEraseQueue(ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], FALSE); 

#ifdef L2MEASURE
    L2MeasureLogIncECTyepCnt(ucSuperPu, L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, g_TLCManager->ausDesTLCBlk[ucSuperPu]), L2MEASURE_ERASE_TLCERR);
#endif    

    g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE] = 0;
    g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE] = INVALID_4F;
    g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE] = 0;
    return;
}


BOOL L2_TLCErrHSetNewDesBlkSWL(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U8 *pStatus;
    BOOL bRet = FALSE;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (TRUE == COM_BitMaskGet(gwl_info->tSWLCommon.m_FinishBitMap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }

        pStatus = &gwl_info->tSWLCommon.m_FlushStatus[ucSuperPu][0][ucLunInSuperPu];
        L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, gwl_info->nDstBlk[ucSuperPu], pStatus, FALSE, FALSE, TRUE);

        gwl_info->tSWLCommon.m_FinishBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }

    if (SUPERPU_LUN_NUM_BITMSK == gwl_info->tSWLCommon.m_FinishBitMap[ucSuperPu])
    {
        /* Wait erase status */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pStatus = &gwl_info->tSWLCommon.m_FlushStatus[ucSuperPu][0][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == (*pStatus))
            {
                return bRet;
            }
        }

        gwl_info->tSWLCommon.m_FinishBitMap[ucSuperPu] = 0;
        L2_BM_CollectFreeBlock_WL(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]);
        L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
        L2_VBT_ResetBlk(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]);

#ifdef DBG_LC
        lc.uLockCounter[ucSuperPu][STATIC_WL]--;
        if (lc.uLockCounter[ucSuperPu][STATIC_WL] >= 3)
        {
            DBG_Printf("ucSuperPu %d lock block counter >= 3 !\n", ucSuperPu);
            DBG_Getch();
        }
#endif
        bRet = TRUE;
    }
    return bRet;
}


void L2_TLCHandleProgFail(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucErrLun;// = g_TLCManager->aucErrLun[ucSuperPu];
    U8 ucLunOffset;
    U8 ucTLun;
    TLCErrH *pErrStage;
    U16 usNewDes = INVALID_4F;

    /* record current program order for thread decision to increase quota for TLC thread */
#ifdef SIM
    if (0 == g_TLCManager->ausTLCProgOrder[ucSuperPu])
    {
        DBG_Printf("TLC program err, current program order is 0\n");
        DBG_Getch();
    }
#endif

    /*get current process ErrLUN*/
    for (ucLunOffset = 0; ucLunOffset <LUN_NUM_PER_SUPERPU; ucLunOffset++)
    {
        if (0 != (g_TLCManager->aulErrProgBitMap[ucSuperPu] & (1 << ucLunOffset)))
        {
            ucErrLun =  ucLunOffset;
            break;
        }
    }

    pErrStage = &g_TLCManager->aeErrHandleStage[ucSuperPu];

    switch (*pErrStage)
    {
    case TLC_ERRH_ALLOCATE_NEWBLK:

        /* allocate new phy block */
        ASSERT(FALSE != L2_VBT_Get_TLC(ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu]));
        L2_ErrHReplaceBLK(ucSuperPu, ucErrLun, g_TLCManager->ausDesTLCBlk[ucSuperPu], TRUE, WRITE_ERR);
        *pErrStage = TLC_ERRH_SAVE_BBT;
        //break;

    case TLC_ERRH_SAVE_BBT:
        ucTLun = L2_GET_TLUN(ucSuperPu, ucErrLun);
        L2_BbtSetLunSaveBBTBitMap(ucTLun, TRUE);
        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
        *pErrStage = TLC_ERRH_WAIT_BBT;
        break;

    case TLC_ERRH_WAIT_BBT:
        ucTLun = L2_GET_TLUN(ucSuperPu, ucErrLun);
        if (TRUE == L2_BbtIsSavedDone())
        {
            g_TLCManager->aucTLCBufStatus[ucSuperPu][ucErrLun][TLC_BUF_CNT] = SUBSYSTEM_STATUS_SUCCESS;
            g_TLCManager->aulErrProgBitMap[ucSuperPu] &= ~(1 << ucErrLun);

            if (0 == g_TLCManager->aulErrProgBitMap[ucSuperPu])
            {
                *pErrStage = TLC_ERRH_ERASE;
            }
            else
            {
                *pErrStage = TLC_ERRH_ALLOCATE_NEWBLK;
                break;
            }
        }
        else
        {
            break;
        }

    case TLC_ERRH_ERASE:
        L2_TLCErrHPutTargetIntoEraseQueue(ucSuperPu); //GC write & SWL
        *pErrStage = TLC_ERRH_ALL;

        break;

    case TLC_ERRH_ALL:
    default:
        DBG_Printf("TLC ErrH stage error\n");
        DBG_Getch();
        break;
    }

    return;
}

U8 L2_GetInterCopySrcBlkIndex(U16 usPageNum)
{
    U8 ucSrcBlkNum = usPageNum / (PG_PER_SLC_BLK - 1);
    return ucSrcBlkNum;
}

U16 L2_GetInterCopyPgIndex(U16 usPageNum)
{
    U16 usPgIndex = usPageNum % (PG_PER_SLC_BLK - 1);
    return usPgIndex;
}

void L2_SetInterCopySrcAddr(U8 ucSuperPu, U8 ucLunInSuperPu,PhysicalAddr *pSrcAddr, U8 ucProgPageCnt, U16 uc1stPageNum, U16 uc2ndPageNum)
{
    U32 i;
    U8 ucSrcBlkIndex;
    U16 usPageNum;

    for (i = 0; i < ucProgPageCnt; i++)
    {
        if (i == 0)
        {
            usPageNum = uc1stPageNum;
        }
        else
        {
            usPageNum = uc2ndPageNum;
        }

        ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);

        pSrcAddr[i].m_PUSer = ucSuperPu;
        pSrcAddr[i].m_OffsetInSuperPage = ucLunInSuperPu;
        pSrcAddr[i].m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex];
        pSrcAddr[i].m_PageInBlock = L2_GetInterCopyPgIndex(usPageNum); 
        pSrcAddr[i].m_LPNInPage = 0;

        //FIRMWARE_LogInfo("\t Pu %d InternalCopySrcAddr ucTLCWL %d SrcAddr[%d] %d Pg %d \n", ucSuperPu,ucTLCWL, i, pSrcAddr[i].m_BlockInPU, pSrcAddr[i].m_PageInBlock);
    }

}

U8 L2_GetCurTLCWLByProgOrder(U8 ucSuperPu)
{
    DBG_Printf("3D_TLC : Don't use this funciton : L2_GetCurTLCWLByProgOrder()\n");
    DBG_Getch();
    return 0;// (l_aTLCInverseProgOrder[g_TLCManager->ausTLCProgOrder[ucSuperPu]] >> TLC_PROGCYCLE_BIT);
}

U8 L2_GetProgCycleByProgOrder(U8 ucSuperPu)
{
    DBG_Printf("3D_TLC : Don't use this funciton : L2_GetProgCycleByProgOrder()\n");
    DBG_Getch();
    return 0;// (l_aTLCInverseProgOrder[g_TLCManager->ausTLCProgOrder[ucSuperPu]] & TLC_PROGCYCLE_MSK);
}

U8 L2_GetPageTypeByProgOrder(U8 ucSuperPu)
{
    return (U8)l_aTLCInverseProgOrder[g_TLCManager->ausTLCProgOrder[ucSuperPu]].m_PageType;
}

U8 L2_GetProgPageCntByProgOrder(U8 ucSuperPu)
{
    return (U8)l_aTLCInverseProgOrder[g_TLCManager->ausTLCProgOrder[ucSuperPu]].m_ProgPageCnt;
}

U16 L2_Get1stPageNumByProgOrder(U8 ucSuperPu)
{
    return g_TLCManager->ausTLCProgOrder[ucSuperPu];
}

U16 L2_Get2ndPageNumByProgOrder(U8 ucSuperPu)
{
    return l_aTLCInverseProgOrder[g_TLCManager->ausTLCProgOrder[ucSuperPu]].m_2ndPageNum;
}

U8 L3_GetPageTypeByProgOrder(U16 usPage)
{
    return (U8)l_aTLCInverseProgOrder[usPage].m_PageType;
}

U8 L3_GetProgPageCntByProgOrder(U16 usPage)
{
    return (U8)l_aTLCInverseProgOrder[usPage].m_ProgPageCnt;
}

U16 L3_Get2ndPageNumByProgOrder(U16 usPage)
{
    return l_aTLCInverseProgOrder[usPage].m_2ndPageNum;
}

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
BOOL L2_GetWordLineClose(U16 usCurPageNum)
{
#ifdef FLASH_IM_3DTLC_GEN2 // for B16A and B17A

    //(Lower + Upper) or (Extra + Upper)
    if(usCurPageNum <= 35)
    {
       if((usCurPageNum == 11) || (usCurPageNum == 35))
          return TRUE;
    }
    else if((usCurPageNum > 35)  && (usCurPageNum <= 2218))
    {
	   if (l_aTLCInverseProgOrder[usCurPageNum].m_PageType == L2_UPPER_PAGE)
       {
          if((usCurPageNum >= 94) && ((usCurPageNum - 94) % 36) == 0)
              return TRUE;
       }
    }

    return FALSE;

#else

    return l_aTLCInverseProgOrder[usCurPageNum].m_WLClosed;

#endif
}
#else
BOOL L2_GetSharedPageClose(U16 usCurPageNum)
{
    if((usCurPageNum <= 11) || (usCurPageNum >= 2292))
    {
       return TRUE;
    }
    else if((usCurPageNum > 11)  && (usCurPageNum < 2292))
    {
	   if (l_aTLCInverseProgOrder[usCurPageNum].m_PageType == L2_UPPER_PAGE)
       {
          return TRUE;
       }
    }

    return FALSE;
}
#endif
#endif

/****************************************************************************
Name        :L2_TLCWriteTLC
Input       :U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap
Output      :
Description : write data from TLCManager's buffer to TLC's WL.
History       :
1. 2015.06.09 zoewen create
****************************************************************************/
BOOL L2_TLCWriteTLC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 i;
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U8 ucBufO;
   PhysicalAddr tTLCAddr, tSLCAddr[TLC_PROGPAGE_CNT];
    U8 *pStatus;
    U32 aulBuffer[TLC_PROGPAGE_CNT] = { 0 };
    RED *pSpare, *pCurSpare;
    BOOL Ret = FALSE;
    TLCStage eNextStage = TLC_STAGE_WRITETLC;
    U32 *pulTargetOffsetBitMap;
    U8 ucPageType;
    U8 usProgPageCnt;
    U16 uc1stPageNum, uc2ndPageNum;   
    TLCWriteType eWriteType;
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    RED *pFirstPageSpare;    
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
    BOOL bWordLineClose = FALSE;
#else
    BOOL bSharedPageClose = FALSE;
#endif        
#endif    
    
    //in this interface, take SWL(copyvalid) as TLCGC
    if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        eWriteType = TLC_WRITE_TLCGC;
    }
    else
    {
        eWriteType = g_TLCManager->aeTLCWriteType[ucSuperPu];
    }

#ifdef L3_UNIT_TEST
    U32 ulTargetBitMap = 0;
#endif
    COMMON_EVENT L2_Event;

    // get the LUN bitmap
#ifdef L3_UNIT_TEST
    pulTargetOffsetBitMap = &ulTargetBitMap;    //not support multi-lun
#else
        pulTargetOffsetBitMap = &g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE];
#endif

    uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    usProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);

    if(usProgPageCnt == 2)
    {
       uc2ndPageNum = L2_Get2ndPageNumByProgOrder(ucSuperPu);
    }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef FLASH_IM_3DTLC_GEN2
    
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
    if(usProgPageCnt == 2)
       bWordLineClose = L2_GetWordLineClose(uc2ndPageNum);
#else
    if(usProgPageCnt == 2)
       bSharedPageClose = L2_GetSharedPageClose(uc2ndPageNum); 
    else
       bSharedPageClose = L2_GetSharedPageClose(uc1stPageNum);   
#endif
#else
       bWordLineClose = L2_GetWordLineClose(uc1stPageNum);
#endif
#endif

    if (TLC_WRITE_TLCGC == eWriteType)
        ucBufO = uc1stPageNum % TLC_BUF_CNT;
    else
        ucBufO = 0;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }
        if (TRUE == COM_BitMaskGet(*pulTargetOffsetBitMap, ucLunInSuperPu))
        {
            continue;
        }
#ifdef SWL_EVALUATOR
        if (uc1stPageNum == TLC_FIRST_RPMT_PAGE_NUM)
        {
            SWL_TotalTLCWPageCntInc(ucSuperPu, TLC_TOTAL_RPMT_PAGE_NUM);
        }
        else if (uc1stPageNum < TLC_FIRST_RPMT_PAGE_NUM)
        {
            if (ucPageType == L2_LOWER_PAGE)
            {
                SWL_TotalTLCWPageCntInc(ucSuperPu,usProgPageCnt);
            }
            else if (ucPageType == L2_EXTRA_PAGE)
            {
                SWL_TotalTLCWPageCntInc(ucSuperPu,1);
            }
        }
#endif
        /* status address */
        pStatus = &g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT];

        /* Setting TLC address */
        tTLCAddr.m_PPN = 0;
        tTLCAddr.m_PUSer = ucSuperPu;
        tTLCAddr.m_BlockInPU = g_TLCManager->ausDesTLCBlk[ucSuperPu];
        // please note that what we pass to L3 is the current program order
        tTLCAddr.m_PageInBlock = g_TLCManager->ausTLCProgOrder[ucSuperPu];
        tTLCAddr.m_OffsetInSuperPage = ucLunInSuperPu;
        tTLCAddr.m_LPNInPage = 0;

        if ((ucPageType == L2_LOWER_PAGE) || (ucPageType == L2_LOW_PAGE_WITHOUT_HIGH) || (ucPageType == L2_EXTRA_PAGE))
        {
            if ((TRUE == g_TLCManager->bTLCInternalW[ucSuperPu]) && (uc1stPageNum < TLC_FIRST_RPMT_PAGE_NUM))
            {
                L2_SetInterCopySrcAddr(ucSuperPu, ucLunInSuperPu, &tSLCAddr[0], usProgPageCnt, uc1stPageNum, uc2ndPageNum);               

#ifndef L3_UNIT_TEST
                /* follow the general rule of L2, whenever we write a new page,we increase timestamp by 1 */
                if (0 == *pulTargetOffsetBitMap)
                {
                    L2_IncTimeStampInPu(ucSuperPu);
                }
#endif                   
                g_TLCManager->aFirstPageRealTS[ucSuperPu] = L2_GetTimeStampInPu(ucSuperPu);
                   
                L2_UpdateTLCRedandRPMT(ucSuperPu, ucLunInSuperPu, ucPageType, usProgPageCnt, uc1stPageNum, uc2ndPageNum, NULL, FALSE);

                //FIRMWARE_LogInfo("InternalWrite SuperPU %d TLCBlk 0x%x_%d Pg %d PrgOrder %d SLCBlk 0x%x_0x%x_0x%x ucPageType %d \n", ucSuperPu, 
                //    tTLCAddr.m_BlockInPU, pVBT[ucSuperPu]->m_VBT[tTLCAddr.m_BlockInPU].PhysicalBlockAddr[ucLunInSuperPu], tTLCAddr.m_PageInBlock, 
                //    g_TLCManager->ausTLCProgOrder[ucSuperPu], tSLCAddr[0].m_BlockInPU, tSLCAddr[1].m_BlockInPU, tSLCAddr[2].m_BlockInPU, ucPageType);

                L2_FtlTLCInternalWriteLocal(&tSLCAddr[0], &tTLCAddr, pStatus);
            }
            else
            {
                // 3DTLC only need to set first two atSpare[0] & atSpare[1] structure.
                pSpare = g_TLCManager->atSpare[ucSuperPu][ucLunInSuperPu][0];

                pCurSpare = pSpare;

                /* data buffer address */
                // we have to check if we're programming the last word line of the TLC block
#ifndef L3_UNIT_TEST
                if (uc1stPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
                {
                    // we're programming the last word line of the TLC block, select RPMT buffer as the data source                   
                    if (uc1stPageNum == TLC_FIRST_RPMT_PAGE_NUM)
                    {
                        aulBuffer[0] = ((U32)&g_TLCManager->pRPMT[ucSuperPu][0]->m_RPMT[ucLunInSuperPu]);
                    }
                    else if (uc1stPageNum == (TLC_FIRST_RPMT_PAGE_NUM + 1))
                    {
                        aulBuffer[0] = ((U32)&g_TLCManager->pRPMT[ucSuperPu][1]->m_RPMT[ucLunInSuperPu]);
#ifndef FLASH_IM_3DTLC_GEN2
                        if (TLC_WRITE_TLCGC == eWriteType)
                        {
                            aulBuffer[1] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][TLC_BUF_CNT][ucLunInSuperPu];
                        }
                        else
                        {
                            aulBuffer[1] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][1][ucLunInSuperPu];
                        }
#endif
                    }
                    else if (uc1stPageNum == (TLC_FIRST_RPMT_PAGE_NUM + 2))
                    {
                        aulBuffer[0] = ((U32)&g_TLCManager->pRPMT[ucSuperPu][2]->m_RPMT[ucLunInSuperPu]);
                    }
                }
                else
#endif
                {
                    // we're programming an usual data word line, select the data buffer as the data source
                    for (i = 0; i < usProgPageCnt; i++)
                    {
                        aulBuffer[i] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][(ucBufO + i) % TLC_BUF_CNT][ucLunInSuperPu];
                    }
#ifndef FLASH_IM_3DTLC_GEN2
                    /*TLCGC use SubstituteBuff Confirm*/
                    if (TLC_WRITE_TLCGC == eWriteType)
                    {                                 
                        if (ucPageType == L2_EXTRA_PAGE)
                        {
                            aulBuffer[1] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][TLC_BUF_CNT][ucLunInSuperPu];
                        }
                    }
#endif
                }

               /* check: update redundant info */
#ifndef L3_UNIT_TEST
                    /* here we follow the general rule of L2, whenever we write a new page, we increase the timestamp by 1 */
                if (0 == *pulTargetOffsetBitMap)
                {
                    L2_IncTimeStampInPu(ucSuperPu);
                }
#endif
                if (0 == uc1stPageNum)
                {
                    g_TLCManager->aFirstPageRealTS[ucSuperPu] = L2_GetTimeStampInPu(ucSuperPu);
                }

                if (TLC_WRITE_TLCGC == eWriteType)
                {
                    L2_UpdateTLCRedandRPMT(ucSuperPu, ucLunInSuperPu, ucPageType, usProgPageCnt, uc1stPageNum, uc2ndPageNum, pSpare, TRUE);
                }
                else
                {
                    if (uc1stPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
                    {
                        L2_UpdateTLCRedandRPMT(ucSuperPu, ucLunInSuperPu, ucPageType, usProgPageCnt, uc1stPageNum, uc2ndPageNum, pSpare, TRUE);
                    }
                    else
                    {
                        L2_UpdateTLCRedandRPMT(ucSuperPu, ucLunInSuperPu, ucPageType, usProgPageCnt, uc1stPageNum, uc2ndPageNum, NULL, FALSE);
                    }
                }                

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                if (0 == tTLCAddr.m_PageInBlock)
                {
                    if(TLC_WRITE_HOSTWRITE == eWriteType)
                    {
                       pFirstPageSpare = g_TLCManager->atSpare[ucSuperPu][ucLunInSuperPu][2];
                       pFirstPageSpare->m_RedComm.bcBlockType = BLOCK_TYPE_TLC_W;
                       pFirstPageSpare->m_RedComm.bcPageType = PAGE_TYPE_DATA;
                       pFirstPageSpare->m_RedComm.eOPType = OP_TYPE_HOST_WRITE;
                       pFirstPageSpare->m_RedComm.ulTimeStamp =  g_TLCManager->aFirstPageRealTS[ucSuperPu];
                       
                       L2_Set_DataBlock_PBIT_Info(tTLCAddr.m_PUSer, tTLCAddr.m_OffsetInSuperPage, tTLCAddr.m_BlockInPU, pFirstPageSpare);               
                    }
                    else
                    {
                        L2_Set_DataBlock_PBIT_Info(tTLCAddr.m_PUSer, tTLCAddr.m_OffsetInSuperPage, tTLCAddr.m_BlockInPU, pSpare);
                    }
                }
#else
                if ((0 == tTLCAddr.m_PageInBlock) || (TLC_WRITE_HOSTWRITE == eWriteType))
                {
                    L2_Set_DataBlock_PBIT_Info(tTLCAddr.m_PUSer, tTLCAddr.m_OffsetInSuperPage, tTLCAddr.m_BlockInPU, pSpare);
                }
#endif
        
                if (uc1stPageNum == (TLC_FIRST_RPMT_PAGE_NUM + 2))
                {
                    L2_Set_DataBlock_PBIT_InfoLastTimeStamp(tTLCAddr.m_PUSer, tTLCAddr.m_OffsetInSuperPage, tTLCAddr.m_BlockInPU, pSpare);
                }

                //FIRMWARE_LogInfo("ExternalWrite LUN %d TLCBlk 0x%x_%d Pg %d PrgOrder %d PageType %d ProgPageCnt %d SrcBlk 0x%x_0x%x_0x%x buffer 0x%x_0x%x_0x%x Spare 0x%x_0x%x_0x%x\n", ucLunInSuperPu, tTLCAddr.m_BlockInPU, 
                //    pVBT[ucSuperPu]->m_VBT[tTLCAddr.m_BlockInPU].PhysicalBlockAddr[ucLunInSuperPu], tTLCAddr.m_PageInBlock,
                //    g_TLCManager->ausTLCProgOrder[ucSuperPu], ucPageType, usProgPageCnt, g_TLCManager->ausSrcBlk[ucSuperPu][0], g_TLCManager->ausSrcBlk[ucSuperPu][1], g_TLCManager->ausSrcBlk[ucSuperPu][2],
                //    aulBuffer[0], aulBuffer[1], aulBuffer[2], &pSpare[0], &pSpare[1], &pSpare[2]);

                /* TLC write command */
                L2_FtlTLCExternalWriteLocal(&tTLCAddr, (U32 *)aulBuffer, (U32*)pSpare, pStatus, NULL);
            }
        }

            //Update TLC PPO
#ifndef FLASH_IM_3DTLC_GEN2
            if (ucPageType != L2_LOWER_PAGE)
                L2_SetTLCPPO(ucSuperPu, tTLCAddr.m_PageInBlock + 1);
            else
#endif
                L2_SetTLCPPO(ucSuperPu, tTLCAddr.m_PageInBlock + usProgPageCnt);

#ifdef L2MEASURE
        if(uc1stPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
        {
            L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_RPMT_SLC);
        }
        else
        {
            if (TLC_WRITE_HOSTWRITE == eWriteType)
            {
                L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_TLCMERGE_TLC);
            }
            else// (TLC_WRITE_SWL == eWriteSrc)
            {
                L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_SWLALL_TLC);
            }
        }
#endif          

        /* update BitMap */
        (*pulTargetOffsetBitMap) |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);

        if (SUPERPU_LUN_NUM_BITMSK == *pulTargetOffsetBitMap)
        {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPu);
#endif            
            *pulTargetOffsetBitMap = 0;

#ifndef FLASH_IM_3DTLC_GEN2
            if (ucPageType != L2_LOWER_PAGE) //means L2_EXTRA_PAGE or L2_LOW_PAGE_WITHOUT_HIGH
            {
               g_TLCManager->ausTLCProgOrder[ucSuperPu]++;
            }
            else
#endif
            {
               g_TLCManager->ausTLCProgOrder[ucSuperPu] += usProgPageCnt;
            }

            CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
            
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            if((L2_Event.EventShutDown) && (m_ShutdownCalculateStartPage[ucSuperPu] == 0))
            {
                m_ShutdownCalculateStartPage[ucSuperPu] = g_TLCManager->ausTLCProgOrder[ucSuperPu];
                
                if(g_TLCManager->ausTLCProgOrder[ucSuperPu] >= TLC_FORCE_TO_CLOSE_PAGE_NUM)
                   DBG_Printf("SuperPU %d TLC_Merge : StartPage(%d) : Force to close TLC Blk VBN(%d).\n", ucSuperPu, m_ShutdownCalculateStartPage[ucSuperPu], g_TLCManager->ausDesTLCBlk[ucSuperPu]);                
            }
#endif

            /* Need to evaluate cost time of shutdown improvement : Force to close TLC open block. */
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
            if ((L2_Event.EventShutDown) && bWordLineClose && (g_TLCManager->ausTLCProgOrder[ucSuperPu] < TLC_FORCE_TO_CLOSE_PAGE_NUM))
#else
            if ((L2_Event.EventShutDown) && bSharedPageClose && (g_TLCManager->ausTLCProgOrder[ucSuperPu] < TLC_FORCE_TO_CLOSE_PAGE_NUM))
#endif                
            {
                eNextStage = TLC_STAGE_SHUTDOWN_SHARED_PAGE_CLOSED;
            }
#else//SHUTDOWN_IMPROVEMENT_STAGE1
            if (L2_Event.EventShutDown)
            {
                eNextStage = TLC_STAGE_SHUTDOWN_TLCBLK_ERASE;
            }
#endif
            else
            {
                if ((FALSE == g_TLCManager->bTLCInternalW[ucSuperPu]) &&
                    ((PG_PER_SLC_BLK * PG_PER_WL) > g_TLCManager->ausTLCProgOrder[ucSuperPu]))
                {
                    eNextStage = TLC_STAGE_READSRC;
                }
            }
                        
            Ret = TRUE;
            goto out;
        }

    }

out:
    g_TLCManager->aeTLCStage[ucSuperPu] = eNextStage;
    return Ret;
}


#ifndef SHUTDOWN_IMPROVEMENT_STAGE2
void MCU1_DRAM_TEXT L2_EraseTLCWriteBlk(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U32 *pulTargetOffsetBitMap;   
    U8 ucLunInSuperPu, ucTLun;
    U8 *pStatus;
 
    TLCWriteType eWriteType = g_TLCManager->aeTLCWriteType[ucSuperPu];

    // get the LUN bitmap
    pulTargetOffsetBitMap = &g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE];
 
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }
        if (TRUE == COM_BitMaskGet(*pulTargetOffsetBitMap, ucLunInSuperPu))
        {
            continue;
        }

        /* status address */
        pStatus = &g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][TLC_BUF_CNT];

        L2_EraseTLCBlock(ucSuperPu, ucLunInSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], pStatus, TRUE);

        /* update BitMap */
        (*pulTargetOffsetBitMap) |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);

        if (SUPERPU_LUN_NUM_BITMSK == *pulTargetOffsetBitMap)
        {
            *pulTargetOffsetBitMap = 0;
                        
            if (TLC_WRITE_HOSTWRITE == eWriteType)
            {
                g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_SHUTDOWN_WAIT_TLCBLK_ERASE;
                DBG_Printf("SuperPU %d TLC_Merge VBN(%d) PG(%d); meanwhile, receive shutdown event.\n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], g_TLCManager->ausTLCProgOrder[ucSuperPu]); 
            }
            else if ((TLC_WRITE_TLCGC == eWriteType) || (TLC_WRITE_SWL == eWriteType))
            {
                g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_SHUTDOWN_WAIT_TLCBLK_ERASE;
                DBG_Printf("SuperPU %d TLC_GC VBN(%d) PG(%d); meanwhile, receive shutdown event.\n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], g_TLCManager->ausTLCProgOrder[ucSuperPu]);
            }
        }
    }
}
#endif

TLCStage L2_TLCCheckReadStatus(U8 ucSuperPu)
{
    U8 ucLunInSuperPu, ucPGO;
    U16 uc1stPageNum, uc2ndPageNum;
    U8 ucPageType, ucProgPageCnt;
    U16 usPageNum;
    U8 ucSrcBlkIndex;
         
    uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);
            
    if (ucProgPageCnt == 2)
    {
        uc2ndPageNum = L2_Get2ndPageNumByProgOrder(ucSuperPu);
    }
          
    /* check read src status */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        for (ucPGO = 0; ucPGO < ucProgPageCnt; ucPGO++)
        {
            if (SUBSYSTEM_STATUS_PENDING == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
            {
                return TLC_STAGE_CHECK_READSTATUS;
            }
            else if (SUBSYSTEM_STATUS_FAIL == g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
            {
                if(g_TLCManager->aucSLCReadNum[ucSuperPu] == ucPGO)
                {
                    usPageNum = uc1stPageNum;//LowerPage or ExtraPage
                }
                else
                {
                    usPageNum = uc2ndPageNum;//UpperPage
                }

                if (usPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
                {
                    ucSrcBlkIndex = usPageNum - TLC_FIRST_RPMT_PAGE_NUM;
                    DBG_Printf("PU %d LUN %d block %d read src rpmt %d redundant fail in TLC merge, skip, no data is needed.\n", ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex], ucSrcBlkIndex);
#ifdef L2_HANDLE_UECC
                    L2_AddUECCBlock(ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex]);
#endif
                    continue;
                }

                ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                DBG_Printf("PU %d LUN %d block %d read src %d fail in TLC merge.\n", ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex], ucSrcBlkIndex);
#ifdef L2_HANDLE_UECC
                L2_AddUECCBlock(ucSuperPu, ucLunInSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex]);
#endif
            }
        }
    }

    return TLC_STAGE_WRITETLC;
}

/****************************************************************************
Name        :L2_TLCReadSrcBlk
Input       :U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap
Output      :
Description : Read page from SLC block to TLCManager's buffer
History    :
1. 2015.06.09 zoewen create
****************************************************************************/
TLCStage L2_TLCReadSrcBlk(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U16 usPageNum;
    U8 ucPageType, ucProgPageCnt;
    U16 uc1stPageNum, uc2ndPageNum;
    U8 ucSrcBlkIndex;
    U8 ucBufO;
    PhysicalAddr tSLCAddr;
    U8 *pStatus;
    U32 *pBuffer;
    RED *pSpare;
    TLCWriteType eWriteSrc = g_TLCManager->aeTLCWriteType[ucSuperPu];
    BOOL bSrcTLC = FALSE;

    #ifdef SIM
    // check the validity of the current program order
    if ((PG_PER_SLC_BLK * PG_PER_WL) <= g_TLCManager->ausTLCProgOrder[ucSuperPu])
    {
        DBG_Printf("L2_TLCReadSrcBlk, WL >= %d error!!\n", PG_PER_SLC_BLK * PG_PER_WL);
        DBG_Getch();
    }
    #endif

    uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);
            
    if (ucProgPageCnt == 2)
    {
        uc2ndPageNum = L2_Get2ndPageNumByProgOrder(ucSuperPu);
    }
               
    /* read data from SLC block */
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (TRUE == COM_BitMaskGet(g_TLCManager->aucTLCReadBitMap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }

        // determine the buffer to be used to store source data for the current read operation
        ucBufO = g_TLCManager->aucSLCReadNum[ucSuperPu];
        
        /* setting Source Blk address */
        tSLCAddr.m_PPN = 0;
        tSLCAddr.m_PUSer = ucSuperPu;

        if(g_TLCManager->aucSLCReadNum[ucSuperPu] == 0)
           usPageNum = uc1stPageNum;//LowerPage or ExtraPage
        else
           usPageNum = uc2ndPageNum;//UpperPage                
            
        if (usPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
        {
            tSLCAddr.m_PageInBlock = PG_PER_SLC_BLK - 1; 
            
            if (uc1stPageNum == TLC_FIRST_RPMT_PAGE_NUM)
            {
                tSLCAddr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][0];
            }
            else if (uc1stPageNum == (TLC_FIRST_RPMT_PAGE_NUM + 1))
            {
                tSLCAddr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][1];
            }
            else if (uc1stPageNum == (TLC_FIRST_RPMT_PAGE_NUM + 2))
            {
                tSLCAddr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][2];
            }
        }
        else
        {
            tSLCAddr.m_PageInBlock = L2_GetInterCopyPgIndex(usPageNum);
            ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
            tSLCAddr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex];
        }

        tSLCAddr.m_OffsetInSuperPage = ucLunInSuperPu;
        tSLCAddr.m_LPNInPage = 0;

        pStatus = &g_TLCManager->aucTLCBufStatus[ucSuperPu][ucLunInSuperPu][ucBufO];
        pSpare = g_TLCManager->atSpare[ucSuperPu][ucLunInSuperPu][ucBufO];
        pBuffer = (U32 *)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][ucBufO][ucLunInSuperPu];

#ifndef L3_UNIT_TEST
        if (usPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
        {
            // for the last word line of the source block, we only need to load the spare
            // of the page since the data it stores is RPMT and we've already loaded it
            L2_LoadSpare(&tSLCAddr, pStatus, (U32*)pSpare, TRUE);
        }
        else
#endif
        {
            L2_FtlReadLocal(pBuffer, &tSLCAddr, pStatus, (U32*)pSpare, LPN_PER_BUF, 0, FALSE, !(bSrcTLC));
        }

        /* update BitMap */
        g_TLCManager->aucTLCReadBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);

        if (SUPERPU_LUN_NUM_BITMSK == g_TLCManager->aucTLCReadBitMap[ucSuperPu])
        {
            // the data of the current word line has been loaded on all LUNs of the source block
            // advance to the next source block if we have to

            g_TLCManager->aucTLCReadBitMap[ucSuperPu] = 0;
            g_TLCManager->aucSLCReadNum[ucSuperPu]++;

            if (ucProgPageCnt == g_TLCManager->aucSLCReadNum[ucSuperPu])
            {
                g_TLCManager->aucSLCReadNum[ucSuperPu] = 0;
                return TLC_STAGE_CHECK_READSTATUS;
            }
        }
    }

    return TLC_STAGE_READSRC;
}

void L2_TLCUpdatePMT(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;
    U8 ucPartInWL, ucLPNO;
    U16 ucWLO;
    U8 ucSrcBlkIndex;
    U32 ulTLCGCSrcIndex;
    U32 usTLCGCSrcBlk;
    U16 usSrcPgIndex;
    U16 usPageNum;
    U32 ulLPN;
    PhysicalAddr tOrgAddr, tSrcAddr, tDesAddr;
    U32 ulLPNOffset;

#ifdef SIM
    U32 ulUpdataPMTLPNCnt[TLCGC_SRCBLK_MAX] = { 0 };
    U32 ulOrgDC;
#endif

    if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
#ifdef SIM
        ulOrgDC = L2_GetDirtyCnt(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][0]) +
        L2_GetDirtyCnt(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][1]) +
        L2_GetDirtyCnt(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][2]);
#endif

    /* Update PMT Table info */
    for (ucWLO = 0; ucWLO < PG_PER_SLC_BLK; ucWLO++)
    {
        for (ucPartInWL = 0; ucPartInWL < PG_PER_WL; ucPartInWL++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {                
                usPageNum = ucWLO * PG_PER_WL + ucPartInWL;
                
                if(usPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
                {
                    continue;
                }

                ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);
                
                tSrcAddr.m_PPN = 0;
                tSrcAddr.m_PUSer = ucSuperPu;
                tSrcAddr.m_BlockInPU = g_TLCManager->ausSrcBlk[ucSuperPu][ucSrcBlkIndex];
                tSrcAddr.m_PageInBlock = usSrcPgIndex;
                tSrcAddr.m_OffsetInSuperPage = ucLunInSuperPu;

                tDesAddr.m_PPN = 0;
                tDesAddr.m_PUSer = ucSuperPu;
                tDesAddr.m_BlockInPU = g_TLCManager->ausDesTLCBlk[ucSuperPu];
                tDesAddr.m_PageInBlock = usPageNum;
                tDesAddr.m_OffsetInSuperPage = ucLunInSuperPu;

                for (ucLPNO = 0; ucLPNO < LPN_PER_BUF; ucLPNO++)
                {
                    tSrcAddr.m_LPNInPage = ucLPNO;
                    tDesAddr.m_LPNInPage = ucLPNO;

                    /* lookup RPMT to get logic LPN from physical addr */
                    ulLPN = LookupRPMTInGC(ucSuperPu, &tSrcAddr, ucSrcBlkIndex, TRUE, NULL);
                    if (INVALID_8F != ulLPN)
                    {
                        L2_LookupPMT(&tOrgAddr, ulLPN, FALSE);
                    }

                    //only the valid data need to update it's PMT information
                    if ((INVALID_8F != ulLPN) && (tOrgAddr.m_PPN == tSrcAddr.m_PPN))
                    {
                        L2_UpdatePMT(&tDesAddr, NULL, ulLPN);

                        //FIRMWARE_LogInfo("TLCMerge LPN 0x%x -> Addr 0x%x (Blk %d_%d Pg %d) WL %d PartInWL %d TS %d\n", 
                        //  ulLPN, tDesAddr.m_PPN, tDesAddr.m_BlockInPU, pVBT[ucSuperPu]->m_VBT[tDesAddr.m_BlockInPU].PhysicalBlockAddr[0], tDesAddr.m_PageInBlock, ucWLO, ucPartInWL,
                        //    g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usSrcPgIndex]);
                    
                        L2_UpdateDirtyLpnMap(&tDesAddr, B_VALID);

                        /* increase the gc source block dirty count */
                        L2_IncreaseDirty(ucSuperPu, tSrcAddr.m_BlockInPU, 1);
                        L2_UpdateDirtyLpnMap(&tSrcAddr, B_DIRTY);
#ifdef DBG_PMT
                        /* for L2 PMT debug */
                        L2_UpdateDebugPMT(&tDesAddr, ulLPN);
#endif
                    }
                    else
                    {
                        L2_IncreaseDirty(ucSuperPu, tDesAddr.m_BlockInPU, 1);
                    }
                }

            }
        }
    }

#ifdef SIM
    for (ucPartInWL = 0; ucPartInWL < PG_PER_WL; ucPartInWL++)
    {
        if (LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT != L2_GetDirtyCnt(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucPartInWL]))
        {
            DBG_Printf("SLC Blk :%d DC(%d) != LPN_PER_SUPER_SLCBLK(%d)\n", g_TLCManager->ausSrcBlk[ucSuperPu][ucPartInWL]
                , L2_GetDirtyCnt(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][ucPartInWL]), LPN_PER_SUPER_SLCBLK);
            DBG_Getch();
        }
    }

    if (ulOrgDC != L2_GetDirtyCnt(ucSuperPu, tDesAddr.m_BlockInPU))
    {
        DBG_Printf("TLC Dirty Cnt != sum of 3 SLC Dirty Cnt\n");
        DBG_Getch();
    }
#endif
    }
    else if ((TLC_WRITE_TLCGC == g_TLCManager->aeTLCWriteType[ucSuperPu]) || (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu]))
    {
#ifdef SIM
        for (ulTLCGCSrcIndex = 0; ulTLCGCSrcIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; ulTLCGCSrcIndex++)
        {
            usTLCGCSrcBlk = g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulTLCGCSrcIndex];

            //FIRMWARE_LogInfo("SuperPU %d TLCGCSrcBlk 0x%x DirtyCnt 0x%x when before update PMT\n", ucSuperPu, usTLCGCSrcBlk, L2_GetDirtyCnt(ucSuperPu, usTLCGCSrcBlk));
        }
#endif

        for (ucWLO = 0; ucWLO < PG_PER_SLC_BLK; ucWLO++)
        {
            for (ucPartInWL = 0; ucPartInWL < PG_PER_WL; ucPartInWL++)
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    usPageNum = ucWLO * PG_PER_WL + ucPartInWL;
                    
                    if(usPageNum >= TLC_FIRST_RPMT_PAGE_NUM)
                    {
                        continue;
                    }

                    ucSrcBlkIndex = L2_GetInterCopySrcBlkIndex(usPageNum);
                    usSrcPgIndex = L2_GetInterCopyPgIndex(usPageNum);

                    tDesAddr.m_PPN = 0;
                    tDesAddr.m_PUSer = ucSuperPu;
                    tDesAddr.m_BlockInPU = g_TLCManager->ausDesTLCBlk[ucSuperPu];
                    tDesAddr.m_PageInBlock = usPageNum;
                    tDesAddr.m_OffsetInSuperPage = ucLunInSuperPu;

                    for (ucLPNO = 0; ucLPNO < LPN_PER_BUF; ucLPNO++)
                    {
                        tDesAddr.m_LPNInPage = ucLPNO;
                        ulLPNOffset = usSrcPgIndex*LPN_PER_SUPERBUF + ucLunInSuperPu*LPN_PER_BUF + ucLPNO;
                        ulLPN = g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_RPMTItems[usSrcPgIndex*LPN_PER_BUF + ucLPNO];

                        if (INVALID_8F != ulLPN)
                        {
                            L2_LookupPMT(&tOrgAddr, ulLPN, FALSE);
                            tSrcAddr = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][ucSrcBlkIndex][ulLPNOffset];
                        }

                        //only the valid data need to update it's PMT information
                        if ((INVALID_8F != ulLPN) && (tOrgAddr.m_PPN == tSrcAddr.m_PPN))
                        {
                            L2_UpdatePMT(&tDesAddr, NULL, ulLPN);

                            //FIRMWARE_LogInfo("TLCGC LPN 0x%x -> Addr 0x%x (Blk %d_%d Pg %d) WL %d PartInWL %d TS %d\n",
                            //  ulLPN, tDesAddr.m_PPN, tDesAddr.m_BlockInPU, pVBT[ucSuperPu]->m_VBT[tDesAddr.m_BlockInPU].PhysicalBlockAddr[0],
                            //    tDesAddr.m_PageInBlock, ucWLO, ucPartInWL, g_TLCManager->pRPMT[ucSuperPu][ucSrcBlkIndex]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usSrcPgIndex]);

                            L2_UpdateDirtyLpnMap(&tDesAddr, B_VALID);

                            /* increase the gc source block dirty count */
                            L2_IncreaseDirty(ucSuperPu, tSrcAddr.m_BlockInPU, 1);
                            L2_UpdateDirtyLpnMap(&tSrcAddr, B_DIRTY);
                        #ifdef SIM
                            for (ulTLCGCSrcIndex = 0; ulTLCGCSrcIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; ulTLCGCSrcIndex++)
                            {
                                if (tSrcAddr.m_BlockInPU == g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulTLCGCSrcIndex])
                                {
                                    ulUpdataPMTLPNCnt[ulTLCGCSrcIndex]++;
                                }
                            }
                        #endif
                        #ifdef DBG_PMT
                            /* for L2 PMT debug */
                            L2_UpdateDebugPMT(&tDesAddr, ulLPN);
                        #endif
                        }
                        else
                        {
                            L2_IncreaseDirty(ucSuperPu, tDesAddr.m_BlockInPU, 1);
                        }
                    }
                }
            }
        }

#ifdef SIM
        for (ulTLCGCSrcIndex = 0; ulTLCGCSrcIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; ulTLCGCSrcIndex++)
        {
            usTLCGCSrcBlk = g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulTLCGCSrcIndex];
            if (LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT != L2_GetDirtyCnt(ucSuperPu, usTLCGCSrcBlk))
            {
                DBG_Printf("TLCGC SPU %d BLOCK 0x%x DC %d != LPN_PER_SUPER_BLOCK %d\n", ucSuperPu, usTLCGCSrcBlk, L2_GetDirtyCnt(ucSuperPu, usTLCGCSrcBlk), LPN_PER_SUPER_BLOCK);
                DBG_Getch();
            }
        }
#endif
    }
    else
    {
        DBG_Printf("other kind of TLCWrite Type shouldn't reach here!\n");
        DBG_Getch();
    }

    return;
}

BOOL L2_TLCReset(U8 ucSuperPu)
{
    U32 i;
    PuInfo *pInfo;
    U16 usTLCGCSrcVBN;
    U16 usTLCGCSrcMax;
    BOOL bRet = FALSE;
    GCComStruct *ptCom;

    switch (g_TLCManager->aeTLCResetStage[ucSuperPu])
    {
        case TLCRESET_UPDATE_RPMT:
        {
            /* Update PMT */
            L2_TLCUpdatePMT(ucSuperPu);
#ifdef READ_DISTURB_OPEN
            L1_ForL2SetReLookupPMTFlag(ucSuperPu);
#endif
            g_TLCManager->aeTLCResetStage[ucSuperPu] = TLCRESET_BLKERASE_CHECK;
        }

        case TLCRESET_BLKERASE_CHECK:
        {
            if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPu])
            {
                /* Put SLC blk into erase queue */
                for (i = 0; i < PG_PER_WL; i++)
                {
                    L2_PBIT_Set_Lock(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][i], FALSE);
                    L2_InsertBlkIntoEraseQueue(ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][i], TRUE);
                    //FIRMWARE_LogInfo("TLCMerger SrcBlk SuperPU %d BLK 0x%x into EreaseQue\n", ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][i]);
#ifdef L2MEASURE
                    L2MeasureLogIncECTyepCnt(ucSuperPu, L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, g_TLCManager->ausSrcBlk[ucSuperPu][i]), L2MEASURE_ERASE_SLC2TLC);
#endif
                }
                //FIRMWARE_LogInfo("TLCMerger SuperPU %d SrcBLK:0x%x_0x%x_0x%x TargetBlk 0x%x\n", ucSuperPu, g_TLCManager->ausSrcBlk[ucSuperPu][0],
                //    g_TLCManager->ausSrcBlk[ucSuperPu][1], g_TLCManager->ausSrcBlk[ucSuperPu][2], g_TLCManager->ausDesTLCBlk[ucSuperPu]);
            }
            else if ((TLC_WRITE_TLCGC == g_TLCManager->aeTLCWriteType[ucSuperPu]) || (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu]))
            {
                ptCom = &g_GCManager[TLCGC_MODE]->tGCCommon;

                /*Add all diry block into EraseQue*/
                usTLCGCSrcMax = g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu];

        #ifdef SIM
                if (0 == usTLCGCSrcMax)
                {
                    DBG_Printf("SuperPU %d TLCGC SrcTargetCnt should be bigger than 1\n", ucSuperPu);
                    //DBG_Getch();
                }
        #endif
                for (i = 0; i < usTLCGCSrcMax; i++)
                {
                    usTLCGCSrcVBN = g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][i];
                    if (INVALID_4F == usTLCGCSrcVBN)
                    {
                        continue;
                    }

                    if ((LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT) == L2_GetDirtyCnt(ucSuperPu, usTLCGCSrcVBN))
                    {
                        g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][i] = INVALID_4F;

                        L2_PBIT_Set_Lock(ucSuperPu, usTLCGCSrcVBN, FALSE);
                        L2_InsertBlkIntoEraseQueue(ucSuperPu, usTLCGCSrcVBN, FALSE);
                    }
                    else
                    {
                        DBG_Printf("TLCGC SuperPU %d SrcBlk 0x%x not all dirty when TLCGC done\n", ucSuperPu, usTLCGCSrcVBN);
                        DBG_Getch();
                    }
                }

                if (g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] == g_pTLCGCSrcBlkRecord->l_aTLC1stGCSrcBlk[ucSuperPu])
                {
                    usTLCGCSrcVBN = INVALID_4F;
                    g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
                }
                else
                {
                    usTLCGCSrcVBN = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu];
                }

                if (INVALID_4F != usTLCGCSrcVBN)
                {
                    g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
                    if ((LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT) != L2_GetDirtyCnt(ucSuperPu, usTLCGCSrcVBN))
					{
						L2_PBIT_Set_Lock(ucSuperPu, usTLCGCSrcVBN, FALSE);
						//FIRMWARE_LogInfo("TLCGC SuperPu %d Release BLK 0x%x\n", ucSuperPu, usTLCGCSrcVBN);
					}
					else//Current all dirty blk is the last one
					{
						L2_PBIT_Set_Lock(ucSuperPu, usTLCGCSrcVBN, FALSE);
                        L2_InsertBlkIntoEraseQueue(ucSuperPu, usTLCGCSrcVBN, FALSE);
                    }
                }
            }
            else
            {
                DBG_Printf("L2_TLCReset TLCManager WriteType error\n");
                DBG_Getch();
            }
            g_TLCManager->aeTLCResetStage[ucSuperPu] = TLCRESET_RESET_MANAGER;
        }

        case TLCRESET_RESET_MANAGER:
        {
            L2_Set_PBIT_BlkSN(ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu]);

            if (g_TLCManager->ausDesTLCBlk[ucSuperPu] != gwl_info->nDstBlk[ucSuperPu])
            {
 #ifdef SIM
                if ((TLC_WRITE_HOSTWRITE != g_TLCManager->aeTLCWriteType[ucSuperPu])
                    && (TLC_WRITE_TLCGC != g_TLCManager->aeTLCWriteType[ucSuperPu]))
                {
                    DBG_Printf("TLC write Type:%d\n", g_TLCManager->aeTLCWriteType[ucSuperPu]);
                    DBG_Getch();
                }
 #endif

                pInfo = g_PuInfo[ucSuperPu];
                if ((pInfo->m_TargetBlk[TARGET_TLC_WRITE] == g_TLCManager->ausDesTLCBlk[ucSuperPu])
                    && (g_TLCManager->ausTLCProgOrder[ucSuperPu] == (PG_PER_WL*PG_PER_SLC_BLK)))
                {
                    /* Allocate TLC write target new block */
                    pVBT[ucSuperPu]->m_VBT[pInfo->m_TargetBlk[TARGET_TLC_WRITE]].Target = VBT_NOT_TARGET;
 
                    pInfo->m_TargetBlk[TARGET_TLC_WRITE] = INVALID_4F;
                    pInfo->m_TargetPPO[TARGET_TLC_WRITE] = 0;
                }
                else
                {
                    DBG_Printf("SuperPU %d L2_TLCReset condition error\n", ucSuperPu);
                    DBG_Getch();
                }
            }

            else
            {
                //SWL Reset is different from GC
                U16 PhyDstBlk;
                U8 ucLunInSuperPu;
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    PhyDstBlk = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];

                    L2_PBIT_Set_Free(ucSuperPu, ucLunInSuperPu, PhyDstBlk, FALSE);
                    L2_PBIT_Set_Allocate(ucSuperPu, ucLunInSuperPu, PhyDstBlk, TRUE);
                }
                pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].Target = VBT_NOT_TARGET;
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
#ifdef NEW_SWL
                //here need to release the unused SrcBlk
                if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
                {
                    L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
                    gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
                }
#endif
                gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
                gwl_info->nDstPPO[ucSuperPu] = 0;
                g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE] = 0;
#if (!defined(NEW_SWL) && defined(DBG_LC))
                lc.uLockCounter[ucSuperPu][STATIC_WL]--;
#endif
            }

            //DBG_Printf("L2_TLCReset() => L2_ResetTLCManager(ucSuperPu=%d)\n", ucSuperPu);
            L2_ResetTLCManager(ucSuperPu);
            bRet = TRUE;
        }
    }
 
    return bRet;
 }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
void MCU1_DRAM_TEXT L2_TLCSharedPageClosedSave(U8 ucSuperPuNum)
{
    U32 ulDataAddr;
    U32 ulLength;
    U32 ulBufferAddr;
    U32 i;
        
    ulBufferAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPuNum][0][0];            
    ulDataAddr = (U32)g_TLCManager->m_TLCGCSrcAddr[ucSuperPuNum];

#ifdef FLASH_IM_3DTLC_GEN2
    ulDataAddr += ((LUN_NUM_PER_SUPERPU * BUF_SIZE) << 1);
    ulLength = sizeof(g_TLCManager->m_TLCGCSrcAddr[0]) - ((LUN_NUM_PER_SUPERPU * BUF_SIZE) << 1);
#else
    ulDataAddr += LUN_NUM_PER_SUPERPU * BUF_SIZE;
    ulLength = sizeof(g_TLCManager->m_TLCGCSrcAddr[0]) - LUN_NUM_PER_SUPERPU * BUF_SIZE;
#endif

    COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
    ulBufferAddr += ulLength;

    ulDataAddr = (U32)g_TLCManager->atSpare[ucSuperPuNum][0][0];
    ulLength = LUN_NUM_PER_SUPERPU * sizeof(RED) * 3;
    COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
    ulBufferAddr += ulLength;
    
    if(ucSuperPuNum == 0)
    {
       ulDataAddr = (U32)g_TLCManager;
       ulLength = sizeof(TLCMerge);
       COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;    

       ulDataAddr = (U32)g_GCManager[TLCGC_MODE];
       ulLength = sizeof(GCManager);
       COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;

       ulDataAddr = (U32)g_pTLCGCSrcBlkRecord;

#ifdef SIM
	   ulLength = sizeof(TLCGCSrcBlkRecord);
#else
       ulLength = sizeof(TLCGCSrcBlkRecordStage2);
#endif

       COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;

       for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
       {
           if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[i])
           {
               L2_PBIT_Set_Lock(i, gwl_info->tSWLCommon.m_SrcPBN[i], FALSE);
               gwl_info->tSWLCommon.m_SrcPBN[i] = INVALID_4F;
           }
#ifdef NEW_SWL
           if (INVALID_4F != gwl_info->nSrcBlkBuf[i])
           {
               L2_PBIT_Set_Lock(i, gwl_info->nSrcBlkBuf[i], FALSE);
               gwl_info->nSrcBlkBuf[i] = INVALID_4F;
           }
#endif
       }

       for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
       { 
           if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[i])
           {
               ulDataAddr = (U32)gwl_info;
               ulLength = sizeof(WL_INFO);
               COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ulLength >> DWORD_SIZE_BITS);
               i = SUBSYSTEM_SUPERPU_NUM;
           }
       }
    }

    if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
    {
        DBG_Printf("SuperPU %d TLC_Merge VBN(%d) Start(%d)-PPO(%d); meanwhile, receive shutdown event.\n", 
                    ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], m_ShutdownCalculateStartPage[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
    }
    else if (TLC_WRITE_TLCGC == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
    {
        DBG_Printf("SuperPU %d TLC_GC VBN(%d) Start(%d)-PPO(%d); meanwhile, receive shutdown event.\n",
                    ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], m_ShutdownCalculateStartPage[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
    }
    else if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
    {
        DBG_Printf("SuperPU %d TLC_SWL VBN(%d) Start(%d)-PPO(%d); meanwhile, receive shutdown event.\n", 
                    ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], m_ShutdownCalculateStartPage[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
    }   
}
          
void MCU1_DRAM_TEXT L2_TLCSharedPageClosedLoad(U8 ucSuperPuNum)
{
    U32 ulDataAddr;
    U32 ulLength;
    U32 ulBufferAddr;
    U32 i;

    ulBufferAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPuNum][0][0];
    ulDataAddr = (U32)g_TLCManager->m_TLCGCSrcAddr[ucSuperPuNum];

#ifdef FLASH_IM_3DTLC_GEN2
    ulDataAddr += ((LUN_NUM_PER_SUPERPU * BUF_SIZE) << 1);   
    ulLength = sizeof(g_TLCManager->m_TLCGCSrcAddr[0]) - ((LUN_NUM_PER_SUPERPU * BUF_SIZE) << 1);
#else
    ulDataAddr += LUN_NUM_PER_SUPERPU * BUF_SIZE;   
    ulLength = sizeof(g_TLCManager->m_TLCGCSrcAddr[0]) - LUN_NUM_PER_SUPERPU * BUF_SIZE;
#endif 

    COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
    ulBufferAddr += ulLength;
    
    ulDataAddr = (U32)g_TLCManager->atSpare[ucSuperPuNum][0][0];
    ulLength = LUN_NUM_PER_SUPERPU * sizeof(RED) * 3;
    COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
    ulBufferAddr += ulLength;

    if(ucSuperPuNum == 0)
    {
       ulDataAddr = (U32)g_TLCManager;
       ulLength = sizeof(TLCMerge);
       COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;

       ulDataAddr = (U32)g_GCManager[TLCGC_MODE];
       ulLength = sizeof(GCManager);
       COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;

       ulDataAddr = (U32)g_pTLCGCSrcBlkRecord;
#ifdef SIM
	   ulLength = sizeof(TLCGCSrcBlkRecord);
#else
	   ulLength = sizeof(TLCGCSrcBlkRecordStage2);
#endif
       COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
       ulBufferAddr += ulLength;
 
       for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
       {
           if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[i])
           {
               ulDataAddr = (U32)gwl_info;
               ulLength = sizeof(WL_INFO);
               COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength >> DWORD_SIZE_BITS);
               i = SUBSYSTEM_SUPERPU_NUM;
           }
       }
    }

    if (TLC_STAGE_SHUTDOWN_SHARED_PAGE_CLOSED == g_TLCManager->aeTLCStage[ucSuperPuNum])
    {
        g_TLCManager->aeTLCStage[ucSuperPuNum] = TLC_STAGE_PRPARE;
	    
        if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
        {
            g_TLCManager->aeTLCStage[ucSuperPuNum] = TLC_STAGE_READSRC;
            DBG_Printf("SuperPU %d TLC_Merge VBN(%d) PPO(%d) : To be continue...\n", ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
        }
        else if (TLC_WRITE_TLCGC == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
        {
            if (g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCDummyWrite[ucSuperPuNum] == TRUE)
            {
               g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPuNum] = GC_STATE_COPY_VALID;
            }
            else
            {
               g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPuNum] = GC_STATE_LOAD_RPMT;
               g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPuNum] = TRUE;
            }
            
            g_GCManager[TLCGC_MODE]->tGCCommon.m_ucRPMTNum[ucSuperPuNum] = 0;
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcWLO[ucSuperPuNum] = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcWLO_Extra[ucSuperPuNum];
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcOffset[ucSuperPuNum] = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcOffset_Extra[ucSuperPuNum];
            g_GCManager[TLCGC_MODE]->tGCCommon.m_GCReadOffset[ucSuperPuNum] = 0;
            g_GCManager[TLCGC_MODE]->tGCCommon.m_GCUpOfExtraReadOffset[ucSuperPuNum] = 0;

            DBG_Printf("SuperPU %d TLC_GC VBN(%d) PPO(%d) : To be continue...\n", ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
        }
        else if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPuNum])
        {
            if (g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCDummyWrite[ucSuperPuNum] == TRUE)
            {
               g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPuNum] = GC_STATE_COPY_VALID;
            }
            else
            {
               g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[ucSuperPuNum] = GC_STATE_LOAD_RPMT;
               g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPuNum] = TRUE;
            }
            
            g_GCManager[TLCGC_MODE]->tGCCommon.m_ucRPMTNum[ucSuperPuNum] = 0;
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcWLO[ucSuperPuNum] = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcWLO_Extra[ucSuperPuNum];
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcOffset[ucSuperPuNum] = g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcOffset_Extra[ucSuperPuNum];
            g_GCManager[TLCGC_MODE]->tGCCommon.m_GCReadOffset[ucSuperPuNum] = 0;
            g_GCManager[TLCGC_MODE]->tGCCommon.m_GCUpOfExtraReadOffset[ucSuperPuNum] = 0;

            DBG_Printf("SuperPU %d TLC_SWL VBN(%d) PPO(%d) : To be continue...\n", ucSuperPuNum, g_TLCManager->ausDesTLCBlk[ucSuperPuNum], g_TLCManager->ausTLCProgOrder[ucSuperPuNum]);
        }
    }
}

void MCU1_DRAM_TEXT L2_TLCSharedPageClosedReset(U8 ucSuperPu)
{
   TLCWriteType eWriteType = g_TLCManager->aeTLCWriteType[ucSuperPu]; 

   if (TLC_WRITE_HOSTWRITE == eWriteType)
   {
       L2_FTLTaskTLCMergeClear(ucSuperPu);
       L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC); 
   }
   else if (TLC_WRITE_TLCGC == eWriteType)
   {
       L2_FTLTaskTLCGCClear(ucSuperPu);
       L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC); 
   }
   else if (TLC_WRITE_SWL == eWriteType)
   {
       L2_FTLTaskTLCSWLClear(ucSuperPu);
       L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);
       //here keep the current dst blk and save it in PBIT
       if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu])
       {
           L2_PBIT_Set_Lock(ucSuperPu, gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu], FALSE);
           gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
       }
#ifdef NEW_SWL
       if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
       {
           L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
           gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
       }
#endif
   }
}
#else//SHUTDOWN_IMPROVEMENT_STAGE1
void MCU1_DRAM_TEXT L2_TLCShutdownReset(U8 ucSuperPu)
{ 
   PuInfo *pInfo;
   TLCWriteType eWriteType = g_TLCManager->aeTLCWriteType[ucSuperPu];
   
   pVBT[ucSuperPu]->m_VBT[g_TLCManager->ausDesTLCBlk[ucSuperPu]].Target = VBT_NOT_TARGET;
   L2_RecycleBlock(ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu]);
   L2_BM_CollectFreeBlock(ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu]);

   pInfo = g_PuInfo[ucSuperPu];
   pInfo->m_TargetPPO[TARGET_TLC_WRITE] = 0;

   if (g_TLCManager->ausDesTLCBlk[ucSuperPu] == gwl_info->nDstBlk[ucSuperPu])
   {
       L2_FTLTaskTLCSWLClear(ucSuperPu);  

       /* Unlock nSrcBlkBuf here,because L2_StaticWLEntry will call L2_SWLClear after SWL Task cleared */
    #ifdef NEW_SWL
       if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
       {
           L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
           gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
       }
    #endif

   }
   else
   {
       if (TLC_WRITE_HOSTWRITE == eWriteType)
       {
           pInfo->m_TargetBlk[TARGET_TLC_WRITE] = INVALID_4F;
           L2_FTLTaskTLCMergeClear(ucSuperPu);
           L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC);
       }
       else if (TLC_WRITE_TLCGC == eWriteType)
       {
           pInfo->m_TargetBlk[TARGET_TLC_WRITE] = INVALID_4F;
           L2_FTLTaskTLCGCClear(ucSuperPu);
           L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC);
       }
   }
   L2_ResetTLCManager(ucSuperPu);

   return;
}
#endif

/****************************************************************************
Name        :L2_TLCWrite
Input       :U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap
Output      :
Description : TLC merge, merge 3 SLC block into 1 TLC block
target is TLC write
History    :
1. 2015.05.22 zoewen create
****************************************************************************/
BOOL L2_TLCWrite(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    TLCStage *pStage;
    BOOL Ret = FALSE;


#ifdef SIM
    if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
    {
        FIRMWARE_LogInfo("SuperPU %d TLCMERGE process TaskInfo 0x%x error\n", ucSuperPu, L2_FTLGetTaskBitInfo(ucSuperPu));
        DBG_Getch();
    }
#endif

    // get the current stage of the TLC merge operation
    pStage = &g_TLCManager->aeTLCStage[ucSuperPu];

    switch (*pStage)
    {
    case TLC_STAGE_PRPARE:
        Ret = L2_SetTLCMergeInfo(ucSuperPu, VBT_TYPE_HOST, TLC_WRITE_HOSTWRITE);
        break;

    case TLC_STAGE_ERRH_RPMT:
        /* RPMT load fail */
        if (TRUE == L2_ErrorHandlingEntry(ucSuperPu, FALSE, NULL))
        {
            *pStage = TLC_STAGE_CHECK_RPMTSTATUS;
        }
        break;

    case TLC_STAGE_LOADRPMT:
        // this stage loads RPMTs of all 3 source SLC blocks
        *pStage = L2_TLCLoadRPMT(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        break;

    case TLC_STAGE_CHECK_RPMTSTATUS:
        *pStage = L2_TLCCheckRPMTStatus(ucSuperPu);
        break;

    case TLC_STAGE_ERRH_PROG:
        L2_TLCHandleProgFail(ucSuperPu, ulLunAllowToSendFcmdBitmap);

        if (TLC_ERRH_ALL == g_TLCManager->aeErrHandleStage[ucSuperPu])
        {
            L2_ResetTLCManager(ucSuperPu);

            L2_FTLTaskTLCMergeClear(ucSuperPu);
            L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC);
        }
        break;

    case TLC_STAGE_READSRC:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            *pStage = L2_TLCReadSrcBlk(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        }
        break;

    case  TLC_STAGE_CHECK_READSTATUS:
          *pStage = L2_TLCCheckReadStatus(ucSuperPu);
        break;

    case TLC_STAGE_WRITETLC:
        /* check write status */
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            if (TRUE == L2_TLCWriteTLC(ucSuperPu, ulLunAllowToSendFcmdBitmap))
            {
                if (g_TLCManager->ausTLCProgOrder[ucSuperPu] >= (PG_PER_SLC_BLK * PG_PER_WL))
                {
                    // all word lines of the current TLC blocks has been programmed successfully
                    *pStage = TLC_STAGE_DONE;
                    
                    //DBG_Printf("TLCMerge SuperPU %d ausSrcBlk[0](%d) [1](%d) [2](%d) ausDesTLCBlk[](%d)\n", ucSuperPu,g_TLCManager->ausSrcBlk[ucSuperPu][0], 
                    //    g_TLCManager->ausSrcBlk[ucSuperPu][1],g_TLCManager->ausSrcBlk[ucSuperPu][2],  g_TLCManager->ausDesTLCBlk[ucSuperPu]);
                }
                else
                {
                    Ret = TRUE;
                }
            }
        }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case TLC_STAGE_SHUTDOWN_SHARED_PAGE_CLOSED:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            L2_TLCSharedPageClosedReset(ucSuperPu);
            g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt++;
            Ret = TRUE;
        }
        break;
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    case TLC_STAGE_SHUTDOWN_TLCBLK_ERASE:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            L2_EraseTLCWriteBlk(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        }
        break;

    case TLC_STAGE_SHUTDOWN_WAIT_TLCBLK_ERASE:
        if (TRUE == L2_TLCCheckEraseStatus(ucSuperPu))
        {
            L2_TLCShutdownReset(ucSuperPu);
            g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt++;
            Ret = TRUE;
        }
        break;
#endif

#ifdef L2_HANDLE_UECC
    case TLC_STAGE_ERRH_UECC:
        L2_HandleUECC(ucSuperPu, ulLunAllowToSendFcmdBitmap);

        if (TLC_ERRH_ALL == g_TLCManager->aeErrHandleUECCStage[ucSuperPu])
        {
            *pStage = TLC_STAGE_DONE;
        }
        break;
#endif

    case TLC_STAGE_DONE:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
#if (defined(SWL_EVALUATOR) && (!defined(SWL_OFF)))
                SWLRecordTLCW(ucSuperPu);            
#endif
#ifdef L2_HANDLE_UECC
            if (TRUE == L2_NeedHandleUECC(ucSuperPu))
            {
                g_TLCManager->aeTLCStage[ucSuperPu] = TLC_STAGE_ERRH_UECC;
                g_TLCManager->aeErrHandleUECCStage[ucSuperPu] = TLC_ERRH_ALLOCATE_NEWBLK;
                break;
            }
#endif
            L2_TLCReset(ucSuperPu);
            L2_FTLTaskTLCMergeClear(ucSuperPu);
            L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_GC_2TLC);
        }
        break;

    case TLC_STAGE_ALL:
    default:
        DBG_Printf("TLC Stage Error\n");
        DBG_Getch();
        break;

    }
    return Ret;
}


void L2_SetTLCPPO(U32 uSuperPu, U32 PPO)
{
    g_PuInfo[uSuperPu]->m_TargetPPO[TARGET_TLC_WRITE] = PPO;
}
void L2_Set_WL_TLCPPO(U32 uSuperPu, U32 PPO)
{
    gwl_info->nDstPPO[uSuperPu] = PPO;
}

U32 L2_GetTLCPPO(U32 uSuperPu)
{
    return g_PuInfo[uSuperPu]->m_TargetPPO[TARGET_TLC_WRITE];
}

BOOL L2_IsTLCProLastWordLine(U32 uSuperPu)
{
    if (L2_GetTLCPPO(uSuperPu) >= TLC_FIRST_RPMT_PAGE_NUM)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


