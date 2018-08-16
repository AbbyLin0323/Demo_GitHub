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

Filename     : sim_nfccmd.c                                          
Version       : Ver 1.0                                               
Date           :                                         
Author        : Peterlgchen

Description: 

Modification History:
20090522    peterlgchen 01 first create
20120214    PengfeiYu   002 modified
20120409    GavinYin    003 modified
*************************************************/
#include <stdio.h>
#include <stddef.h>
#include "BaseDef.h"
#include "sim_flash_config.h"
#include "sim_NormalDSG.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_MemoryMap.h"
#include "Disk_Config.h"
#include "memory_access.h"
#include "sim_flash_common.h"
#include "sim_SGE.h"
#include "sim_flash_interface.h"
#include "sim_nfccmd.h"
#include "model_config.h"
#include "action_define.h"


//#ifndef SIM
//#include "flash2file.h"
//#include "ffile.h"
//#endif

#ifdef SIM_XTENSA
    #include "hfmid.h"
#endif

//tobey start
NORMAL_DSG_ENTRY NfcDsg;
NORMAL_DSG_ENTRY * l_pNfcDsg;
U16 l_usCurDsgID;
U16 l_usNextDsgID;
U8 l_ucNcqMode;
//tobey end

extern U32* g_pDataBufferIn;
extern U32* g_pDataBufferOut;
extern ST_CE_ERROR_ENTRY   pu_err[NFC_CH_TOTAL][NFC_PU_PER_CH];

extern volatile BOOL g_bReSetFlag;
extern BOOL IsDuringTableRebuild();
extern BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId);
extern CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
extern U8 NF_GetCmdType(U8 ucCmdCode);

extern BOOL g_bWearLevelingStatistic;


#ifdef DBG_TABLE_REBUILD
extern U32 Host_GetDataCntHighBit(U32 lba);
extern void Host_ClearDataCntHighBit(U32 lba,U32 ulWriteCnt);
extern U32 Host_GetDataCnt(U32 lba);
extern BOOL Host_IsHitHCmdTable(U32 ulSystemLba);
#endif

U8 g_LPNInRedOffSet = offsetof(SIM_NFC_RED, m_DataRed);

/*==============================================================================
Func Name  : NFC_CheckReadErrOccur
Input      : U8 ucppu  
             U8 pg_type
             ST_FLASH_CMD_PARAM *p_flash_cmd_para
             ST_FLASH_READ_RETRY_PARAM *p_flash_readrety_param
Output     : None
Return Val : TRUE:Need report the error
             FALSE:don't need to reprot the error
Discription: Check Err Injection Status, to skip set buf map or add chain number.
Usage      : 
History    : 
    1. 2014.10.4 JasonGuo create function
    2. 2015.01.16 NinaYang Modify 
       check whether read error occur:
        if occur, record err_type and err_flag,and check the error whether need to report.
        if not,return false.
==============================================================================*/
BOOL NFC_CheckReadErrOccur(U8 ucppu, U8 pg_type, ST_FLASH_CMD_PARAM *p_flash_cmd_para,ST_FLASH_READ_RETRY_PARAM *p_flash_readrety_param)
{
    BOOL bNfcErr = FALSE;//default no err.

    if (FALSE == p_flash_cmd_para->bsPuEccMsk)
    {
        //1.EMPTY PAGE error,genarated by NFC Model
        if (PG_TYPE_FREE == pg_type)
        {
            NFC_InterfaceRecordErr(ucppu, NF_ERR_TYPE_EMPTY_PG); 
            bNfcErr = TRUE;
        }
        //2.Other errors injected by CheckList
        else if (TRUE == p_flash_cmd_para->module_inj_en)
        {
            //Check UECC whether occur
            if (NF_ERR_TYPE_UECC == p_flash_cmd_para->module_inj_type)
            {
                if (TRUE != p_flash_readrety_param->read_retry_en)
                {
                    NFC_InterfaceRecordErr(ucppu, NF_ERR_TYPE_UECC);
                    bNfcErr = TRUE;
                }
                else if ((TRUE == p_flash_readrety_param->read_retry_en) &&
                    (p_flash_readrety_param->read_retry_current_time < p_flash_readrety_param->read_retry_success_time))
                {                
                    NFC_InterfaceRecordErr(ucppu, NF_ERR_TYPE_UECC); 
                    bNfcErr = TRUE;
                }
                else if ((TRUE == p_flash_readrety_param->read_retry_en) &&
                    (p_flash_readrety_param->read_retry_current_time == p_flash_readrety_param->read_retry_success_time))
                {                
                    FLASH_PHY flash_phy;
                    NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[0].row_addr,
                        p_flash_cmd_para->phy_page_req[0].part_in_wl,ucppu,&flash_phy);

                    DBG_Printf("PU %d Blk %d Pg %d Retry %d times,retry success!\n",ucppu,flash_phy.nBlock,flash_phy.nPage,
                        p_flash_readrety_param->read_retry_success_time);
                }
            }
        }
        else if (TRUE == p_flash_cmd_para->inj_en)
        {
            NFC_InterfaceRecordErr(ucppu, p_flash_cmd_para->err_type);
            bNfcErr = TRUE;
        }
    }
    
    return bNfcErr;
}

U32 L1_CalcLPNForMoudle(U32 phyLPN)
{
#ifdef LPN_TO_CE_REMAP
    U8 targetPU;
    U32 pageNum;
    U32 lpnOffset;

    pageNum = phyLPN>>LPN_PER_BUF_BITS;
    lpnOffset = phyLPN & LPN_PER_BUF_MSK;
    
    targetPU =( ( pageNum )^
                ( pageNum/CE_SUM )^ 
                ( pageNum/CE_SUM/CE_SUM )^
                ( pageNum/CE_SUM/CE_SUM/CE_SUM )^
                ( pageNum/CE_SUM/CE_SUM/CE_SUM/CE_SUM ) )%CE_SUM;
    
    pageNum = ( pageNum & ~(CE_SUM-1) ) + targetPU;

    return ( (pageNum<<LPN_PER_BUF_BITS) + lpnOffset );
#else
    return phyLPN;
#endif
}

void NFC_GetSparePhyAddr(U32* pRedData, ST_FLASH_ADDR *pFlashAddr, U8 nLPNInPage)
{
#ifdef DBG_PMT_SPARE
    pFlashAddr->pu = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_PUSer;
    pFlashAddr->pbn = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_BlockInPU;
    pFlashAddr->ppo = ((SpareArea *)pRedData)->m_PMTDebug[nLPNInPage].m_PageInBlock;

    pFlashAddr->pu = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_PUSer;
    pFlashAddr->pbn = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_BlockInPU;
    pFlashAddr->ppo = ((SIM_NFC_RED *)pRedData)->m_PMTDebug[nLPNInPage].m_PageInBlock;
#endif
}

void initDmaAddr(NFCQ_ENTRY *p_nf_cq_entry)
{
    g_remainSecToXfer = p_nf_cq_entry->bsDmaTotalLength;
    g_finishSecInDmaEntry = 0;

    l_usCurDsgID = 0;
    l_usNextDsgID = 0;
    memset(&NfcDsg,0,sizeof(NORMAL_DSG_ENTRY));
    l_pNfcDsg = NULL;
}

/*
    input: p_flash_cmd_para
    output: pointer of next DMA target address. when return value is TRUE, which means should update buff map after
            data transfer finishing, pointer to the buff map bit also passed to caller 
*/
BOOL NFC_GetDMATargetAddr(ST_FLASH_CMD_PARAM* p_flash_cmd_para, U32 **pAddr, U8 *pBmBitPos, U32 *Bmid,BOOL *pNcq1stEn, U16 *pReleaseDsgFlag)
{
    U32 curDmaAddr;
    //U32 ulNextDmaAddr;
    BOOL bUpdateBm;
    BOOL bBmEn;
    U8    usDMATotalLength;
    U8 ucSecLenInFirst4K;

    bUpdateBm = FALSE;
    usDMATotalLength =  p_flash_cmd_para->p_nf_cq_entry->bsDmaTotalLength;

    if(NULL == l_pNfcDsg)
    {
        l_ucNcqMode = p_flash_cmd_para->ncq_mode;
        l_usCurDsgID = p_flash_cmd_para->p_nf_cq_entry->bsFstDsgPtr;
        DSG_FetchNormalDsg(l_usCurDsgID, &NfcDsg);
        l_pNfcDsg = &NfcDsg;
        g_bmBitPos = l_pNfcDsg->bsMapSecOff >> SEC_PER_LPN_BITS;
        l_usNextDsgID = l_pNfcDsg->bsNextDsgId;
    }
    else
    {
        if(0 == g_finishSecInDmaEntry)
        {
            if(1 != l_pNfcDsg->bsLast)
            {
                DSG_FetchNormalDsg(l_usCurDsgID, &NfcDsg);
                l_pNfcDsg = &NfcDsg;
                g_bmBitPos = l_pNfcDsg->bsMapSecOff >> SEC_PER_LPN_BITS;
                l_usNextDsgID = l_pNfcDsg->bsNextDsgId;
            }
            else 
            {
                return bUpdateBm;
            }
        }
    }

    curDmaAddr = l_pNfcDsg->ulDramAddr + g_finishSecInDmaEntry*SEC_SIZE;

    bBmEn = p_flash_cmd_para->p_nf_cq_entry->bsBmEn;
    *Bmid = l_pNfcDsg->bsBuffMapId;
    g_finishSecInDmaEntry++;

    //ulNextDmaAddr = l_pNfcDsg->DramAddr + g_finishSecInDmaEntry*SEC_SIZE;

    //when 4k finish, current is the last sector in current DSG or last DSG, we need to update buffmap 
    //if(((0 == g_finishSecInDmaEntry%8) || (1 == g_remainSecToXfer) || (g_finishSecInDmaEntry == l_pNfcDsg->XferLen))
    //if(((0 == ulNextDmaAddr%DATA_4KB_SIZE) || (1 == g_remainSecToXfer) || ((g_finishSecInDmaEntry << SEC_SIZE_BITS) == l_pNfcDsg->XferByteLen))
    if(((0 == (l_pNfcDsg->bsMapSecOff + g_finishSecInDmaEntry)%8) || (1 == g_remainSecToXfer) || ((g_finishSecInDmaEntry << SEC_SIZE_BITS) == l_pNfcDsg->bsXferByteLen))
        && (TRUE == bBmEn))
    {
        bUpdateBm = TRUE;
        *pBmBitPos = g_bmBitPos;
        g_bmBitPos++;
    }

    //move forwared when current DSG finished
    if((g_finishSecInDmaEntry  << SEC_SIZE_BITS) == l_pNfcDsg->bsXferByteLen)
    {
        g_finishSecInDmaEntry = 0;
        //DSG_ReleaseNormalDsg(l_usCurDsgID);
        *pReleaseDsgFlag = (1<<15) | (l_usCurDsgID);// bit15 = 1, need to release DSG; bit[14:0] = DSG id to release
        l_usCurDsgID = l_usNextDsgID;
    }

     g_remainSecToXfer--;
     *pAddr = (U32*)curDmaAddr;

    //when 4k finished or current is the last sector in last DSG report ncq_1st
    if (TRUE == p_flash_cmd_para->ncq_1st_en)
    {
        if (NCQMD_4KB_FINISH == l_ucNcqMode)
        {
            if ((p_flash_cmd_para->p_nf_cq_entry->aSecAddr[0].bsSecStart >> SEC_PER_LPN_BITS)
              == ((p_flash_cmd_para->p_nf_cq_entry->aSecAddr[0].bsSecStart
                + p_flash_cmd_para->p_nf_cq_entry->aSecAddr[0].bsSecLength - 1) >> SEC_PER_LPN_BITS))
            {
                ucSecLenInFirst4K = (U8)p_flash_cmd_para->p_nf_cq_entry->aSecAddr[0].bsSecLength;
            }
            else
            {
                ucSecLenInFirst4K = SEC_PER_LPN - (p_flash_cmd_para->p_nf_cq_entry->aSecAddr[0].bsSecStart & SEC_PER_LPN_MSK);
            }

            if ((usDMATotalLength - g_remainSecToXfer) == ucSecLenInFirst4K)
            {
                *pNcq1stEn = TRUE;
            }
        }
        else
        {
            if (0 == g_remainSecToXfer)
            {
                *pNcq1stEn = TRUE;
            }
        }
    }

    return bUpdateBm;
}

U32 NFC_GetSpareLPN(U32* pRedData,U8 nLPNInPage)
{
    return ((SIM_NFC_RED *)pRedData)->m_DataRed.aCurrLPN[nLPNInPage];
}


/*********************************************************************
Function     :NFC_PGReadCMD     
Input         : 
Output       :none 
Description :  
Note: 
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
BOOL NFC_PGReadCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    FLASH_PHY flash_phy;
    NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    int nType;
    U32 dwToDram;
    U32 secIndex;
    U32* p_buf;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    BOOL bUpdateBm;
    U8 bitPosOfBm;
    BOOL ubNcq1stEn = FALSE;
    U32 ulBmid;
    BOOL bNfcErr;
#ifdef VT3514_C0
    U8 ucChkFstByteSecIdx = 0;
    U8 ucFirstByteData = 0;
#endif

    //read whole logic-page out with redundant
    pLocalRed = p_flash_cmd_para->p_local_red;
    p_buf = g_pDataBufferOut;
    //just for judge DataType
    p_flash_cmd_para->phy_page_req[0].row_addr = p_flash_cmd_para->p_nf_cq_entry->aRowAddr[0];
#ifdef SIM_XTENSA
    nType = RAND_DATA_TYPE;
#endif
    for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    {
     NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                     p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                     ucppu,
                     &flash_phy);
     p_addr_red = &pLocalRed->aContent[g_RED_SZ_DW*(p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl*PLN_PER_PU+phyPageIndex)];
#ifdef SIM_XTENSA
     Mid_Read(&flash_phy, nType, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
#else
     Mid_Read(&flash_phy, (char*) p_buf+phyPageIndex*g_PG_SZ, (char*) p_addr_red, g_PG_SZ);
#endif
    }

    //judge data type
    pg_type = NFC_GetSparePGType((SIM_NFC_RED *)pLocalRed);

    if(pg_type == PG_TYPE_DATA_SIMNFC)//A: read compressed data
    {
        nType = COM_DATA_TYPE;
        dwToDram = 2;
    }
    else//B: read whole page
    {
        nType = RSV_DATA_TYPE;
        dwToDram = SEC_SIZE/sizeof(U32);
    }

    //copy data to DMA_buffer
    initDmaAddr(p_flash_cmd_para->p_nf_cq_entry);

    bNfcErr = NFC_CheckReadErrOccur(ucppu, pg_type, p_flash_cmd_para, p_flash_readrety_param);

    for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    {
        if((p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[0] != 0 
            || p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[1] != 0)
            && p_flash_cmd_para->red_only != 1)
        {
            p_buf = g_pDataBufferOut+phyPageIndex*g_PG_SZ/sizeof(U32);
        #ifdef SIM_XTENSA
            NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                ucppu,
                &flash_phy);
            Mid_Read(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
        #endif
            secIndex = 0;
            while(secIndex<64)
            {
                bSecHighDW = (secIndex>31)? 1 : 0;
                bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                {
                    U16 usReleaseDsgFlag;
                    usReleaseDsgFlag = 0;// bit15 = 1, need to release DSG; bit[14:0] = DSG id to release
                    ubNcq1stEn = FALSE;
                    bUpdateBm = NFC_GetDMATargetAddr(p_flash_cmd_para, &p_addr, &bitPosOfBm, &ulBmid, &ubNcq1stEn, &usReleaseDsgFlag);
                    #ifdef VT3514_C0
                    if (TRUE == p_flash_cmd_para->bsFirstByteCheckEn)
                    {
                        ucFirstByteData = *(U8 *)((U32)p_buf + SEC_SIZE * secIndex);
                        if (ucFirstByteData != p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx])
                        {
                            DBG_Printf("Nfc read check first byte value fail. value in first byte entry: 0x%x, value in page: 0x%x\n",
                                p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx],
                                ucFirstByteData);
                            DBG_Getch();
                        }
                        ucChkFstByteSecIdx++;
                    }
                    #endif
                    if(p_flash_cmd_para->ontf_en)
                    {
                        Comm_WriteOtfb((U32)p_addr, dwToDram, p_buf+(SEC_SIZE*secIndex)/sizeof(U32));
                    }
                    else
                    {
                        Comm_WriteDram((U32)p_addr, dwToDram, p_buf+(SEC_SIZE*secIndex)/sizeof(U32));
                    } 
                    
                    #ifdef HOST_SATA                    
                    if(TRUE == bUpdateBm) //update buffmap
                    {
                        if (FALSE == bNfcErr)
                        {
                            p_flash_cmd_para->buffMapValue = 0;
                            p_flash_cmd_para->buffMapValue |= (1<<bitPosOfBm);
                            NFC_InterfaceSetBuffermap(ucppu, p_flash_cmd_para->buffMapValue, ulBmid);
                        }
                    }
                    #endif
                    
                    if (0 != (usReleaseDsgFlag&(1<<15)))
                    {
                        DSG_ReleaseNormalDsg(usReleaseDsgFlag & (~(1<<15)));
                    }

                    //update Ncq1st
                    if(TRUE == ubNcq1stEn)
                    {
                        NFC_InterfaceSetFirstDataReady(p_flash_cmd_para->ncq_num);
                    }
                }
                secIndex++;
            }
        }
    }
    
    return TRUE;
}


U32 NFC_GetLBAInSubSystem(U32 ulSrcLBA)
{
    U8 ulSubSysNumBits = 0;
    U32 ulLanePerBuf = 0;

#ifdef SINGLE_SUBSYSTEM
    ulSubSysNumBits = 0;
#else
    ulSubSysNumBits = 1;
#endif

    ulLanePerBuf = ulSrcLBA >> (SEC_PER_BUF_BITS + ulSubSysNumBits);
    ulSrcLBA = (ulLanePerBuf << SEC_PER_BUF_BITS) + (ulSrcLBA & SEC_PER_BUF_MSK);

    return ulSrcLBA;
}

U32 NFC_GetLBAInSystem(U32 SubSystemLpn,U32 ulMcuID)
{
    U32 SubSystemLba,ulSystemLba;
    U32 ulSubSysNumBits;

#ifdef SINGLE_SUBSYSTEM
    ulSubSysNumBits = 0;
#else
    ulSubSysNumBits = 1;
#endif

    if ((MCU1_ID != ulMcuID) && (MCU2_ID != ulMcuID))
    {
        DBG_Break();
    }

    SubSystemLba = SubSystemLpn*SEC_PER_LPN;

    ulSystemLba = (SubSystemLba & SEC_PER_BUF_MSK) + ((SubSystemLba >> SEC_PER_BUF_BITS) << ( SEC_PER_BUF_BITS + ulSubSysNumBits)) 
        + ((ulMcuID - 2) << SEC_PER_BUF_BITS);

    return ulSystemLba;
}
U32 g_DebugCnt = 0;
BOOL NFC_CheckData(U32 *pData, U32 LPNInSpare,U32 ulMcuID,U32 SecOffsetInLPN)
{
    U32 ulLBA = INVALID_8F ;
    U32 ulWriteCnt = INVALID_8F;
    U32 ulLBAInSubSys = INVALID_8F; 
    BOOL bEqual = TRUE;

    if (NULL == pData)
    {
        DBG_Break();
    }

    ulWriteCnt = *pData;
    ulLBA = *(pData + 1);

    if((INVALID_8F == ulLBA) || (INVALID_8F == LPNInSpare))
    {
        return TRUE;
    }

#ifdef DBG_TABLE_REBUILD
    if((TRUE != g_bReSetFlag) && (TRUE != IsDuringTableRebuild()))
    {
        if(0 != Host_GetDataCntHighBit(ulLBA))
        { 
            Host_ClearDataCntHighBit(ulLBA,ulWriteCnt);
        }
    }
#endif

    ulLBAInSubSys = NFC_GetLBAInSubSystem(ulLBA);

    if ((ulLBAInSubSys/SEC_PER_LPN) != LPNInSpare)
    {
        /*add by nina 2014-10-13,because this corner case will be checked error
        1.write LPN0 to Addr1;
        2.Trim LPN0,PMT is INVALID_8F;
        3.Addr1 is erased and written new data;
        4.abnormal shutdown,
        a)rebuild PMT,LPN0 -> old Addr1 
        b)Host write LPN0,and need merge some lba,
        because Addr1 is writtern new data,nfc will check data error.
        So add this to ignore this check,and clear buffer to INVALID_8F*/ 
#ifdef DBG_TABLE_REBUILD
        U32 ulSystemLba = NFC_GetLBAInSystem(LPNInSpare,ulMcuID) + SecOffsetInLPN;
        if((0 == Host_GetDataCnt(ulSystemLba)) ||
            (TRUE == Host_IsHitHCmdTable(ulSystemLba) && (Host_GetDataCnt(ulSystemLba) > 0)) ||
            IsDuringRebuildDirtyCnt(ulMcuID))
        {
            DBG_Printf("Ignore check LPNInSpare 0x%x MCUId %d ulSystemLba 0x%x LBAInBuffer 0x%x\n",
                LPNInSpare,ulMcuID,ulSystemLba,ulLBA);
            *pData = INVALID_8F;
            *(pData + 1) = INVALID_8F;
            g_DebugCnt++;
            return TRUE;
        }
        else
        {
            DBG_Printf("MCUId %d ulSystemLba 0x%x WriteCnt %d\n",ulMcuID,ulSystemLba,Host_GetDataCnt(ulSystemLba));
        }
#endif
        DBG_Printf("LBAInBuffer is 0x%x, LBAinSubSys is 0x%x, LPN is 0x%x \n", ulLBA, ulLBAInSubSys, LPNInSpare);
        DBG_Break();
        bEqual = FALSE;
    }

    return bEqual;
}

/*********************************************************************
Function        :NFC_PGWriteCMD     
Input        : 
Output        :none 
Description :  
Note: 
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
BOOL NFC_PGWriteCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para)
{
    FLASH_PHY flash_phy;
    SIM_NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    int nType;
    U32  dwFromDram;
    U32 secIndex;
    U32* p_buf;
    //U32* p_buf_temp;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    BOOL bUpdateBm;
    U8 bitPosOfBm;
    BOOL bNcq1stEn = FALSE;
    U32 ulBmid = 0;
#ifdef VT3514_C0
    U8 ucChkFstByteSecIdx = 0;
    U8 ucFirstByteData;
#endif


    if(g_bWearLevelingStatistic)
    {
            Dbg_IncDevWriteCnt(1);
    } 
    //copy redundant data to local memory
    //Comm_GetRedSize();
    if (g_RED_SZ_DW != RED_SZ_DW)
    {
        DBG_Printf("NFC_PGWriteCMD: RedSize not same with Register \n");
        DBG_Getch();
    }

    pLocalRed = (SIM_NFC_RED*)p_flash_cmd_para->p_local_red;
    Comm_ReadOtfb(p_flash_cmd_para->p_red_addr, sizeof(SIM_NFC_RED)/sizeof(U32), (U32 *)pLocalRed);

    pg_type = NFC_GetSparePGType((SIM_NFC_RED *)pLocalRed);
    
    if( PG_TYPE_FREE !=  pg_type)
    {
        if(PG_TYPE_DATA_SIMNFC == pg_type)//A : write whole page
        {
            nType = COM_DATA_TYPE;
            dwFromDram = 2;
        }
        else//B : write compressed data : 2 DW for 1 sector
        {
            nType = RSV_DATA_TYPE;           
            dwFromDram = SEC_SIZE/sizeof(U32);        
        }
        
        p_buf = g_pDataBufferIn;
        //p_buf_temp = g_pDataBufferIn;
        initDmaAddr(p_flash_cmd_para->p_nf_cq_entry);

        for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
        {
            if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
                p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[1] != 0)
            {
                NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                    p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                    ucppu,
                    &flash_phy);

                //write redundant data
                p_addr_red = (U32*)&pLocalRed->m_Content[g_RED_SZ_DW*(p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl*PLN_PER_PU+phyPageIndex)];
            #ifdef SIM_XTENSA
                Mid_Write(&flash_phy, RAND_DATA_TYPE, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
            #endif
                secIndex = 0;
                while(secIndex < 64)
                {
                    bSecHighDW = (secIndex>31)? 1 : 0;
                    bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                    if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                    {
                        U16 usReleaseDsgFlag;
                        usReleaseDsgFlag = 0;// bit15 = 1, need to release DSG; bit[14:0] = DSG id to release
                        bUpdateBm = NFC_GetDMATargetAddr(p_flash_cmd_para, &p_addr, &bitPosOfBm, &ulBmid, &bNcq1stEn, &usReleaseDsgFlag);
                        
                        if(p_flash_cmd_para->ontf_en)
                        {
                            Comm_ReadOtfb((U32)p_addr, dwFromDram, p_buf+(SEC_SIZE*secIndex)/sizeof(U32));
                        }
                        else
                        {
                            Comm_ReadDram((U32)p_addr, dwFromDram,p_buf+(SEC_SIZE*secIndex)/sizeof(U32));
                        }

                        #ifdef VT3514_C0
                        if (TRUE == p_flash_cmd_para->bsFirstByteCheckEn)
                        {
                            ucFirstByteData = *(U8 *)((U32)p_buf + SEC_SIZE * secIndex);
                            if (ucFirstByteData != p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx])
                            {
                                DBG_Printf("Nfc write check first byte value fail. value in first byte entry: 0x%x, value in system memory: 0x%x\n",
                                    p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx],
                                    ucFirstByteData);
                                DBG_Getch();
                            }
                            ucChkFstByteSecIdx++;
                        }
                        #endif
                         //check lba in buffer with LPN in spare
                        if (COM_DATA_TYPE == nType)
                        {
                        #ifdef SIM
                            if (FALSE == NFC_CheckData(p_buf+(SEC_SIZE*secIndex)/sizeof(U32),pLocalRed->m_DataRed.aCurrLPN[((phyPageIndex*SEC_PER_PG)+secIndex)/SEC_PER_LPN],
                                pLocalRed->m_RedComm.ulMCUId,secIndex % SEC_PER_LPN))
                        #else
                            if (FALSE == NFC_CheckData(p_buf+(SEC_SIZE*secIndex)/sizeof(U32),pLocalRed->m_DataRed.aCurrLPN[((phyPageIndex*SEC_PER_PG)+secIndex)/SEC_PER_LPN],0,secIndex % SEC_PER_LPN))
                        #endif
                            {
                                DBG_Printf("NFC_PGWriteCMD: check Data Error!\n");
                                DBG_Break();
                            }
                        }

                        if (0 != (usReleaseDsgFlag&(1<<15)))
                        {
                            DSG_ReleaseNormalDsg(usReleaseDsgFlag & (~(1<<15)));
                        }

                        #ifdef HOST_SATA                        
                        if(TRUE == bUpdateBm)//update buffmap
                        {
                            p_flash_cmd_para->buffMapValue |= (1<<bitPosOfBm);
                            NFC_InterfaceSetBuffermap(ucppu, p_flash_cmd_para->buffMapValue, ulBmid);
                        }
                        #endif
                    }
                    secIndex++;
                }
                #ifdef SIM_XTENSA
                    Mid_Write(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
                #else
                    Mid_Write(&flash_phy, nType, (char*) p_buf, (char *) p_addr_red, g_PG_SZ);
                #endif
            }
            p_buf += g_PG_SZ/sizeof(U32);
        }
    }
    else
    {
        DBG_Printf("Function:NFC_PGWriteCMD-- invalid page type(PG_TYPE_FREE)\n");
        DBG_Getch();
    }
    return TRUE;
}

/*********************************************************************
Function        :NFC_BlkEreaseCMD     
Input        : 
Output        :none 
Description :  
Note: 
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
BOOL NFC_BlkEreaseCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para)
{
    FLASH_PHY flash_phy;
    U32 rowAddr;
    U8 planeIndex;
    U8 wl;
    U8 partInWl;

    //rowAddr = p_flash_cmd_para->p_nf_cq_entry->RowAddrLow[0];
    //rowAddr = ((p_flash_cmd_para->p_nf_cq_entry->RowAddrHigh[0])<<16) + rowAddr;
    rowAddr = p_flash_cmd_para->p_nf_cq_entry->aRowAddr[0];
    NFC_GetFlashAddr(rowAddr, 0, ucppu, &flash_phy);
    wl = flash_phy.nPage/PAGE_PER_WL;

    //read redundant data for check page type
    for(partInWl = 0; partInWl < PAGE_PER_WL; partInWl++)
    {
        for(planeIndex = 0; planeIndex < PLN_PER_PU; planeIndex++)
        {
            flash_phy.nPln = planeIndex;
            flash_phy.nPage = wl*PAGE_PER_WL + partInWl;
            Mid_Erase(&flash_phy);
        }
    }
    
    NFC_ClearInjErrEntry((U8)(flash_phy.nPU),flash_phy.nBlock,INVALID_4F);

    return TRUE;
}

/*********************************************************************
Function        :NFC_GetFlashAddr()
Input        : 
Output        :none 
Description :get the cq addr from NFC phy address to FTL phy address  
Note: 
Modify History:
20120208    Pengfei Yu    001: first created
**********************************************************************/
void NFC_GetFlashAddr(U32 row_addr, U8 part_in_wl, U8 ucpu, FLASH_PHY *pFlashAddr)
{
    U16 pbn = 0;
    U16  ppo = 0;
    U8  pln = 0;

    ppo = (row_addr & WL_ADDR_MSK)*PAGE_PER_WL + part_in_wl;
    pbn = ((row_addr >>(WL_ADDR_BITS+PLN_PER_PU_BITS)) & BLK_REAL_MSK);
    pln =  ((row_addr >> WL_ADDR_BITS) & PLN_PER_PU_MSK);

    pFlashAddr->nBlock = pbn;
    pFlashAddr->nPln = pln;
    pFlashAddr->nPage = ppo;
    pFlashAddr->nPU = ucpu;
}

U8 NFC_GetSparePGType(SIM_NFC_RED * p_red)
{
    return p_red->m_RedComm.bcPageType;
}

U32 NFC_GetSpareOpType(U32* pRedData)
{
    return ((SIM_NFC_RED *)pRedData)->m_RedComm.eOPType;
}

void mem_set
(
 U32 dst_addr, 
 U32 cnt_dw, 
 U32 val
 )
{
    U32 dw;
    for( dw = 0; dw < cnt_dw; dw++ )
    {
        *((U32 *)dst_addr +dw) = val;
    }
}



/*
Function : NFC_OtfbPGReadCMD
Input : U8 ucppu   PU number
        ST_FLASH_CMD_PARAM* p_flash_cmd_para   Pointer to flash command parameters 
Output : BOOL   TRUE for success ,FALSE for failed
Description :
    1.Get otfb addr
    2.Read from flash to otfb
    3.Get sgq 
    4.Xfer data from otfb to host
*/
BOOL NFC_OtfbPGReadCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    FLASH_PHY flash_phy;
    NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    int nType;
    U32 dwToOtfb;
    U32 secIndex;
    U32 ulSecCnt;
    U32* p_buf;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    U8  rp;
    U32 bNfcErr = FALSE;
#ifdef VT3514_C0
    U8 ucChkFstByteSecIdx = 0;
    U8 ucFirstByteData;
#endif

    //U32 ulBmid; 
    rp = NFC_InterfaceCQRP(ucppu);
    //read whole logic-page out with redundant
    pLocalRed = p_flash_cmd_para->p_local_red;
    p_buf = g_pDataBufferOut;
    //just for judge DataType
    p_flash_cmd_para->phy_page_req[0].row_addr = p_flash_cmd_para->p_nf_cq_entry->aRowAddr[0];
#ifdef SIM_XTENSA
    nType = RAND_DATA_TYPE;
#endif
    for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    {
     NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                     p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                     ucppu,
                     &flash_phy);
     p_addr_red = &pLocalRed->aContent[g_RED_SZ_DW*(p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl*PLN_PER_PU+phyPageIndex)];
#ifdef SIM_XTENSA
     Mid_Read(&flash_phy, nType, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
#else
     Mid_Read(&flash_phy, (char*) p_buf+phyPageIndex*g_PG_SZ, (char*) p_addr_red, g_PG_SZ);
#endif
    }

    //judge data type
    pg_type = NFC_GetSparePGType((SIM_NFC_RED *)pLocalRed);
    if(pg_type == PG_TYPE_DATA_SIMNFC)//A: read compressed data
    {
        nType = COM_DATA_TYPE;
        dwToOtfb = 2;
    }
    else//B: read whole page
    {
        nType = RSV_DATA_TYPE;
        dwToOtfb = SEC_SIZE/sizeof(U32);
    }

    //copy data to DMA_buffer
    ulSecCnt = 0;

#if (TRUE == SGE_ENABLE)
    p_addr = (U32*)SGE_GetOtfbAddr(ucppu);
#endif

    for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
    {
        if((p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[0] != 0 
            || p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[1] != 0)
            && p_flash_cmd_para->red_only != 1)
        {
            p_buf = g_pDataBufferOut+phyPageIndex*g_PG_SZ/sizeof(U32);
        #ifdef SIM_XTENSA
            NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                ucppu,
                &flash_phy);
            Mid_Read(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
        #endif
            secIndex = 0;
            while(secIndex<64)
            {
                bSecHighDW = (secIndex>31)? 1 : 0;
                bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                {
                    #ifdef VT3514_C0
                    if (TRUE == p_flash_cmd_para->bsFirstByteCheckEn)
                    {
                        ucFirstByteData = *(U8 *)((U32)p_buf + SEC_SIZE * secIndex);
                        if (ucFirstByteData != p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx])
                        {
                            DBG_Printf("Nfc on-the-fly read check first byte value fail. value in first byte entry: 0x%x, value in page: 0x%x\n",
                                p_flash_cmd_para->pFirstByteEntry->aFirstByteVal[ucChkFstByteSecIdx],
                                ucFirstByteData);
                            DBG_Getch();
                        }
                        ucChkFstByteSecIdx++;
                    }
                    #endif
                    Comm_WriteOtfb((U32)(p_addr + (SEC_SIZE*ulSecCnt)/sizeof(U32)),
                                   dwToOtfb, 
                                   p_buf+(SEC_SIZE*secIndex)/sizeof(U32));
                    ulSecCnt++;
                }
                secIndex++;
            }
        }     
    } 

    bNfcErr = NFC_CheckReadErrOccur(ucppu, pg_type, p_flash_cmd_para, p_flash_readrety_param);
    
#if (TRUE == SGE_ENABLE)
    SGE_OtfbToHost(ucppu,rp,bNfcErr);
#endif
    
    return TRUE;    
}

BOOL NFC_OtfbPGWriteCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para)
{
    FLASH_PHY flash_phy;
    SIM_NFC_RED *pLocalRed;
    U8 pg_type;
    U8 bSecHighDW,bitPosInDW;
    int nType;
    U32  ulDLenOtfb;
    U32 secIndex;
    U32* p_buf;
    //U32* p_buf_temp;
    U32* p_addr = NULL;
    U32* p_addr_red = NULL;
    U16 phyPageIndex;
    BOOL bNcq1stEn = FALSE;

    U32 ulBmid = 0;
    U8 ucRp = NFC_InterfaceCQRP(ucppu);
    
    if(g_bWearLevelingStatistic)
    {
         Dbg_IncDevWriteCnt(1);
    } 
    
    //copy redundant data to local memory
    pLocalRed = (SIM_NFC_RED*)p_flash_cmd_para->p_local_red;
    Comm_ReadOtfb(p_flash_cmd_para->p_red_addr, sizeof(NFC_RED)/sizeof(U32), (U32 *)pLocalRed);

    pg_type = NFC_GetSparePGType((SIM_NFC_RED *)pLocalRed);
    
    if( PG_TYPE_FREE !=  pg_type)
    {
        if(PG_TYPE_DATA_SIMNFC == pg_type)//A : write whole page
        {
            nType = COM_DATA_TYPE;
            ulDLenOtfb = 2;
        }
        else//B : write compressed data : 2 DW for 1 sector
        {
            nType = RSV_DATA_TYPE;           
            ulDLenOtfb = SEC_SIZE/sizeof(U32);        
        }
        
        p_buf = g_pDataBufferIn;
#if (TRUE == SGE_ENABLE)
        SGE_HostToOtfb(ucppu,ucRp);
        p_addr = (U32*)SGE_GetOtfbAddr(ucppu); 
#endif
        for(phyPageIndex =0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
        {
            if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[0] != 0 ||
                p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[1] != 0)
            {
                NFC_GetFlashAddr(p_flash_cmd_para->phy_page_req[phyPageIndex].row_addr,
                    p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl,
                    ucppu,
                    &flash_phy);
                //write redundant data
                p_addr_red = (U32*)&pLocalRed->m_Content[g_RED_SZ_DW*(p_flash_cmd_para->phy_page_req[phyPageIndex].part_in_wl*PLN_PER_PU+phyPageIndex)];
            #ifdef SIM_XTENSA
                Mid_Write(&flash_phy, RAND_DATA_TYPE, (char*) p_addr_red, g_RED_SZ_DW*sizeof(U32));
            #endif
                secIndex = 0;
                while(secIndex<64)
                {
                    bSecHighDW = (secIndex>31)? 1 : 0;
                    bitPosInDW = (bSecHighDW==1)?(secIndex-32):secIndex;
                    if(p_flash_cmd_para->phy_page_req[phyPageIndex].sec_en[bSecHighDW] & (1<<bitPosInDW))
                    {
                        Comm_ReadOtfb((U32)(p_addr + (phyPageIndex*g_PG_SZ)/sizeof(U32) + (SEC_SIZE*secIndex)/sizeof(U32)),
                                      ulDLenOtfb,
                                      p_buf+(SEC_SIZE*secIndex)/sizeof(U32));

                         //check lba in buffer with LPN in spare
                        if (COM_DATA_TYPE == nType)
                        {
                        #ifdef SIM
                            if (FALSE == NFC_CheckData(p_buf+(SEC_SIZE*secIndex)/sizeof(U32),pLocalRed->m_DataRed.aCurrLPN[secIndex/SEC_PER_LPN],
                                pLocalRed->m_RedComm.ulMCUId,secIndex % SEC_PER_LPN))
                        #else
                            if (FALSE == NFC_CheckData(p_buf+(SEC_SIZE*secIndex)/sizeof(U32),pLocalRed->m_DataRed.aCurrLPN[secIndex/SEC_PER_LPN],0,secIndex % SEC_PER_LPN))
                        #endif
                            {
                                DBG_Printf("NFC_PGWriteCMD: check Data Error!\n");
                                DBG_Break();
                            }
                        }
                    }

                    secIndex++;
                }
                #ifdef SIM_XTENSA
                    Mid_Write(&flash_phy, nType, (char*) p_buf, g_PG_SZ);
                #else
                    Mid_Write(&flash_phy, nType, (char*) p_buf, (char *) p_addr_red, g_PG_SZ);
                #endif
            }
            p_buf += g_PG_SZ/sizeof(U32);
        }
    }
    else
    {
        DBG_Printf("Function:NFC_PGWriteCMD-- invalid page type(PG_TYPE_FREE)\n");
        DBG_Getch();
    }

    return TRUE;
}

BOOL NFC_IsAddrUECC(FLASH_PHY* pFlash_phy)
{
    U32 nLoop;
    BOOL bRtn = FALSE;

    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        if (!g_stFlashErrInjEntry[nLoop].valid)
            continue;
        else
        {
            if ((NF_ERR_TYPE_UECC == g_stFlashErrInjEntry[nLoop].err_type)  &&
                (pFlash_phy->nPU == g_stFlashErrInjEntry[nLoop].pu)         &&
                (pFlash_phy->nBlock== g_stFlashErrInjEntry[nLoop].block)    &&
                (pFlash_phy->nPage == g_stFlashErrInjEntry[nLoop].page))
            {
                if(g_stFlashErrInjEntry[nLoop].retry_times > HAL_FLASH_READRETRY_CNT)
                {
                    bRtn = TRUE;
                    break;
                }
            }
        }
    }
    return bRtn;
}

/*****************************************************************************
 Prototype      : NFC_GetPendingLPN
 Description    : Get CQ Entry Write Pending LPN
 Input          : U32* pPendingLPN  
                  U32* pLPNCnt      
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2014/6/17
   Author       : NinaYang
   Modification : Created function

*****************************************************************************/
#ifndef SIM_XTENSA
void NFC_GetPendingLPN(U32* pPendingLPN,U32* pLPNCnt)
{
    U32 nPu = 0;
    U32 i = 0;
    U32 nPendingLpn = 0;
    ST_FLASH_CMD_PARAM flash_param[2*SUBSYSTEM_PU_MAX];
    ST_FLASH_CMD_PARAM* p_flash_param;
    SIM_NFC_RED RedEntry;
    U8 PageType = 0;
    U8 rp;
    U8 cmd_code;
    NFCQ_ENTRY * p_cq_entry;
    NFC_PRCQ_ENTRY * p_prcq_entry;
    U8 ucNFCmdType;
    U8 ucSubSystemPu;
    U32 ulRedAddr = 0;

    *pLPNCnt = 0;
    for (ucSubSystemPu=0; ucSubSystemPu< SUBSYSTEM_PU_NUM; ucSubSystemPu++)
    {   
        nPu = HAL_NfcGetCE(ucSubSystemPu);
        p_flash_param = &flash_param[nPu];

        while(FALSE == NFC_InterfaceIsCQEmpty(nPu))
        {
            rp = NFC_InterfaceCQRP(nPu);
            p_cq_entry = Comm_GetCQEntry(nPu, rp);
            p_prcq_entry = Comm_GetPRCQEntry(nPu, rp);
            cmd_code = NFC_InterfaceGetCmdType(nPu, rp);
            ucNFCmdType = NF_GetCmdType(cmd_code);
            ulRedAddr = p_cq_entry->bsRedAddr << 4;
            if(CMD_CODE_PROGRAM != ucNFCmdType)
            {
                EnterCriticalSection(&g_CHCriticalSection[nPu&NFC_CH_MSK]);
                NFC_InterfaceJumpCQRP(nPu);
                LeaveCriticalSection(&g_CHCriticalSection[nPu&NFC_CH_MSK]);
                continue;
            }
            memset(&RedEntry,0xFF,sizeof(NFC_RED));
            Comm_ReadOtfb(ulRedAddr, sizeof(NFC_RED)/sizeof(U32), (U32*)&RedEntry);
            PageType = (U8)RedEntry.m_RedComm.bcPageType;

            if(PAGE_TYPE_PMT == PageType)
            {
                U32 ulPMTIndex = RedEntry.m_PMTIndex;
                DBG_Printf("MCU %d NFC Pending PMTI %d\n",HAL_GetMcuId(),ulPMTIndex);
            }
            else if(PG_TYPE_DATA_SIMNFC == PageType)
            {
                for(i=0; i<LPN_PER_BUF;i++)
                {
                    nPendingLpn = RedEntry.m_DataRed.aCurrLPN[i];
                    if(nPendingLpn != INVALID_8F)
                    {
                        DBG_Printf("MCU %d Pu %d NFC Pending LPN 0x%x TS 0x%x VirBlk 0x%x\n",HAL_GetMcuId(),nPu,nPendingLpn,RedEntry.m_RedComm.ulTimeStamp,
                            RedEntry.m_RedComm.bsVirBlockAddr);
                        *pPendingLPN = nPendingLpn;
                        pPendingLPN++;
                        (*pLPNCnt)++;
                    }
                }
            }
            EnterCriticalSection(&g_CHCriticalSection[nPu&NFC_CH_MSK]);
            NFC_InterfaceJumpCQRP(nPu);
            LeaveCriticalSection(&g_CHCriticalSection[nPu&NFC_CH_MSK]);
        }
        p_flash_param = NULL;
    }        
}
#endif

/********************** FILE END ***************/

