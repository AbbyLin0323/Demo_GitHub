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
*******************************************************************************/

#include <math.h>
#include "SimATACmd.h"
#include "model_common.h"
#include "simsataloc.h"
#include "simsatadev.h"
#include "base_pattern.h"
#include "HAL_HostInterface.h"
#include "HAL_SataIO.h"
#include "HAL_BufMap.h"
#include "HAL_Interrupt.h"

#include "system_statistic.h"
#include "checklist_parse.h"

extern void L0_SataISR(void);
extern U32 Host_GetDataCntHighBit(U32 lba);

//extern TRIM_CMD_ENTRY g_TrimCmdEntry[TRIM_CMD_SEC_CNT_MAX];
//extern U32 g_TrimCmdEntryHead;
//extern U32 g_TrimCmdEntryTail;

HANDLE  g_SimSDCEvent;
HANDLE  g_hSDCThread;

CRITICAL_SECTION g_CMDQueueCriticalSection;
CRITICAL_SECTION g_BUFMAPCriticalSection;

U8 sim_sata_dev_status;
U8 sim_sata_dev_pending_prd_bread = 1;    // 1:first try to check write prd; 0 : first try to check read prd
U8 sim_sata_dev_force_write;
U8 sim_cur_cmd_tag = SIM_HOST_CMD_MAX;
U8 sim_cur_dsg_id = 0;
BOOL sim_sata_dev_b_handle_cmd = FALSE;        // TRUE : a cmd is handle. FALSE : have no cmd be doing

U8 g_HandleCMDStatus = 0;    // 0 , have no cmd to be handled, 1, now do write cmd, 2, now do read prd
/*
    g_SendSDBFISReady:
    VT3514: fw ctrl hw state machine, last FIS send or not.
    32bits for 32 command tag. each tag one bit.
*/
//U32 g_SendSDBFISReady = 0xFFFFFFFF;
//rSDC_SendSDBFISReady;
U32 *l_pSDBFisReady = 0;

BOOL g_bSDCThreadExit = FALSE;
U16 *l_prGLG_58 = 0;
extern ERR_INJ_PARRM CRCErrParamSlot[ERR_INJ_SLOT_MAX];

#define SATA_SIM_IDLE 6
//#define SATA_SIM_READ_CMD 1
//#define SATA_SIM_WRITE_CMD 2
#define SATA_SIM_CHECK 3
#define SATA_SIM_READ_WAIT 4
#define SATA_SIM_WRITE_WAIT 5
#define SATA_SIM_START 0
#define SATA_SIM_WAIT_TIME 7
#define SATA_SIM_HANDLE_CMD 1

#define SATA_SPLIT_MODE_4K    0
#define SATA_SPLIT_MODE_8K  1
#define SATA_SPLIT_MODE_16K 2
#define SATA_SPLIT_MODE_32K 3

#define SATA_4K  0x1000

#define SATA_SEC_PER_4K_BITS 3
#define SATA_SEC_PER_4K      (1 << SATA_SEC_PER_4K_BITS)
#define SATA_SEC_PER_4K_MSK     (SATA_SEC_PER_4K - 1)


#define SATA_SEC_MAP_BITS 3
#define SATA_SEC_PER_MAP      (1 << SATA_SEC_MAP_BITS)
#define SATA_SEC_PER_MAP_MSK     (SATA_SEC_PER_4K - 1)

#define SATA_READ_CRC_ERR 1

extern U32 Host_GetDataCnt(U32 lba);
extern U32 Host_SaveDataCnt(U32 lba, U32 cnt);
extern void Host_CMDFinish(U8 cmdtag);
extern void DSG_ReleaseSataDsg(U16 DsgId);
extern BOOL DSG_IsSataDsgValid(U16 DsgId);
extern U32 Host_UpdateDataCnt(U32 lba);
extern void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);
extern void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);
extern void DSG_FetchSataDsg(U16 DsgId, SATA_DSG *PSataDsg);
extern U32 Host_SaveDataCnt(U32 lba, U32 cnt);
extern BOOL ATA_HostIsWriteCmd(U8 ucCmdCode);
extern BOOL ATA_HostIsLbaCmd(U8 ucCmdCode);
extern void Host_SimClearHCmdQueueInterface(void);




//#define SATA_SIM_SEC_TRANSFER_TIME  1     //cycles
float SATA_SIM_SEC_TRANSFER_TIME = 52;//26.04us per 16K,600M/s

U32    SATA_SIM_TIMER_INTERVAL = 1;
SATA_DSG g_CurrentPrd = {0};

U32 SDC_DataCacl(U32 lba, U32 cnt)
{
    return (lba << 8) + cnt;
}

BOOL SDC_CheckSDBFISReady(U8 cmd_tag)
{
    U32 ulRegVal;
    regRead((U32)&rSDC_SendSDBFISReady, sizeof(U32), (U8 *)&ulRegVal);
    return ((ulRegVal >> cmd_tag) & 1);

}

void SDC_SendCOMReset(void)
{
    U32 ulStatus = INVALID_2F;
    U32 ulIntPending;

    regRead((U32)&rSDC_IntSrcPending, 2, (U8 *)&ulIntPending);
    ulIntPending |= BIT_SDC_INTSRC_OOB_DONE;

#ifdef SIM
    EnterCriticalSection(&g_CMDQueueCriticalSection);
    regWrite((U32)&rSDC_COMMAND_STATUS, 1, (U8 *)&ulStatus);
    regWrite((U32)&rSDC_IntSrcPending, 2, (U8 *)&ulIntPending);
    L0_SataISR();
    LeaveCriticalSection(&g_CMDQueueCriticalSection);
#endif
}

BOOL SDC_IsDeviceReady(void)
{
    U32 ulStatus;

    regRead((U32)&rSDC_COMMAND_STATUS, 1, (U8 *)&ulStatus);

    if (0 == (ulStatus & BIT_SDC_COMMAND_STATUS_BSY) >> 7)
    {
        return TRUE;
    }

    else
    {
        return FALSE;
    }
}

/****************************************************************************
Name        :SDC_DataCmp
Input       :
Output      :
Author      :
Date        :
Description :compare data from dram with which in memory
Others      :
Modify      :
****************************************************************************/
BOOL SDC_DataCmp(U32 lba, U32 nReadCnt, U32 nReadLba)
{
    int err_code = 0;
    int hasread = 0;
    U32 cnt;
    U8 bRtn = FALSE;
    ERR_INJ_PARRM* pCRCErrParam;
    U8 ucIndex;
    
    cnt = Host_GetDataCnt(lba);

    if( cnt == 0)
    {
        /* TRIM could be discard during normal/abnormal power off */
#ifdef TRIM_DATA_CHECK
        if ((nReadCnt != INVALID_8F) || (nReadLba != INVALID_8F))
        {
            DBG_Printf("SDC_DataCmp: TRIM Lba 0x%x, nReadLba 0x%x, nReadCnt 0x%x ERROR\n",
                        lba, nReadLba, nReadCnt);
            DBG_Break();
        }
#endif
        return TRUE;
    }

#ifdef DBG_TABLE_REBUILD
    if(0 != Host_GetDataCntHighBit(lba))
    {
        if(lba == nReadLba)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
#endif
    
    if (((lba == nReadLba) && (nReadCnt == cnt)) ||
        ((nReadCnt == INVALID_8F) && (nReadLba == INVALID_8F))) //UECC NFC will clear data to INVALID_8F,so ignore check
    {
        bRtn = TRUE;
    }
    else
    {
        DBG_Printf("lba:0x%x, cnt:0x%x, read_lba:0x%x, read_cnt:0x%x\n",
            lba, cnt, nReadLba, nReadCnt);
        DBG_Break();

        bRtn = FALSE;
    }

    /* check is hit Read CRC error */
    for(ucIndex = 0; ucIndex < g_InjCRCErrTail; ucIndex++)    
    {        
        pCRCErrParam = &CRCErrParamSlot[ucIndex];        

        if (nReadLba == pCRCErrParam->LBA)
        {
            if(SATA_READ_CRC_ERR == pCRCErrParam->CmdCode)
            {
                g_HandleCMDStatus = SDC_HANDLE_READ_CRC_ERR;
                DBG_Printf("Read CRC error triggered; lba: 0x%x\n", lba);
                break;
            }
        }
    }


    //else if(read_val_from_ssd == SDC_DataCacl(lba, cnt))
    //    return TRUE;
    //else
    //{
    //    DBG_Printf("lba:0x%x, cnt:0x%x, rightval:0x%x valreadback:0x%x\n",
    //        lba, cnt, SDC_DataCacl(lba, cnt), read_val_from_ssd);

    //    //SDC_LogInfo(LOG_FILE, 0,  "data error : lba=0x%x, cnt=0x%x, rightval=0x%x valreadback=0x%x\n",
    //    //    lba, cnt, sim_data_cacl(lba, cnt), read_val_from_ssd);
    //    return FALSE;
    //}

    return bRtn;
}


/****************************************************************************
Name        :SDC_LchShadowReg
Input       :
Output      :copy cmd data from shadow register to Latch shadow register
Author      :
Date        :
Description :copy cmd data from shadow register to Latch shadow register
Others      : it will be called when the rSDC_SHRLCH_EN trigger
Modify      :
****************************************************************************/
#if 0
void SDC_LchShadowReg()
{

    SHADOW_REG shadow_reg = {0};
    SHADOW_REG_EXP shadow_reg_exp = {0};
    LATCH_SHADOW_REG latch_shadow_reg = {0};

    U32 start_lba = 0;
    U16 sec_cnt = 0;

    regRead((U32)&rSDC_DATA, sizeof(SHADOW_REG), (U8*)&shadow_reg);
    regRead((U32)&rSDC_EXP_FEATURE, sizeof(SHADOW_REG_EXP), (U8*)&shadow_reg_exp);

    switch(shadow_reg.sdc_status)
    {
    case ATA_CMD_READ_FPDMA_QUEUED:
    case ATA_CMD_WRITE_FPDMA_QUEUED:
        start_lba = shadow_reg.sdc_lbalow + (shadow_reg.sdc_lbamid << 8) +
            (shadow_reg.sdc_lbahight << 16) + (shadow_reg_exp.sdc_lbalow_exp << 24);
        latch_shadow_reg.sdc_shrlch_lba48 = start_lba;
        latch_shadow_reg.sdc_shrlch_seccnt = shadow_reg.sdc_fea +  (shadow_reg_exp.sdc_fea_exp << 8);
        latch_shadow_reg.sdc_shrlch_feature = shadow_reg.sdc_fea +  (shadow_reg_exp.sdc_fea_exp << 8);
        latch_shadow_reg.sdc_shrlch_cmd = shadow_reg.sdc_status;
        latch_shadow_reg.sdc_shrlch_ncqtag = shadow_reg.sdc_seccnt >> 3;

        break;

    case ATA_CMD_SEND_FPDMA_QUEUED:
    case ATA_CMD_RECEIVE_FPDMA_QUEUED:
    case ATA_CMD_NCQ_NON_DATA:
        start_lba = shadow_reg.sdc_lbalow + (shadow_reg.sdc_lbamid << 8) +
            (shadow_reg.sdc_lbahight << 16) + (shadow_reg_exp.sdc_lbalow_exp << 24);
        latch_shadow_reg.sdc_shrlch_lba48 = start_lba;
        latch_shadow_reg.sdc_shrlch_seccnt = shadow_reg.sdc_seccnt +  (shadow_reg_exp.sdc_seccnt_exp << 8);
        latch_shadow_reg.sdc_shrlch_feature = shadow_reg.sdc_fea +  (shadow_reg_exp.sdc_fea_exp << 8);
        latch_shadow_reg.sdc_shrlch_cmd = shadow_reg.sdc_status;
        latch_shadow_reg.sdc_shrlch_ncqtag = shadow_reg.sdc_seccnt >> 3;
        break;

    case ATA_CMD_READ_DMA:
    case ATA_CMD_WRITE_DMA:
        start_lba = shadow_reg.sdc_lbalow + (shadow_reg.sdc_lbamid << 8) +
            (shadow_reg.sdc_lbahight << 16) + ((shadow_reg_exp.sdc_lbalow_exp  & 0x0F )<< 24);
        latch_shadow_reg.sdc_shrlch_lba28 = start_lba;
        latch_shadow_reg.sdc_shrlch_seccnt = shadow_reg.sdc_seccnt;
        latch_shadow_reg.sdc_shrlch_cmd = shadow_reg.sdc_status;

        if (shadow_reg.sdc_seccnt == 0)
            latch_shadow_reg.sdc_shrlch_seccnt = 256;

        break;

    case ATA_CMD_READ_DMA_EXT:
    case ATA_CMD_WRITE_DMA_EXT:
        start_lba = shadow_reg.sdc_lbalow + (shadow_reg.sdc_lbamid << 8) +
            (shadow_reg.sdc_lbahight << 16) + (shadow_reg_exp.sdc_lbalow_exp << 24);
        latch_shadow_reg.sdc_shrlch_lba48 = start_lba;
        latch_shadow_reg.sdc_shrlch_seccnt = shadow_reg.sdc_seccnt + (shadow_reg_exp.sdc_seccnt_exp << 8);
        latch_shadow_reg.sdc_shrlch_cmd = shadow_reg.sdc_status;
        break;

    default:
        //peterxiu add for none data command
        latch_shadow_reg.sdc_shrlch_cmd = shadow_reg.sdc_status;
        break;
    }

    latch_shadow_reg.sdc_shrch_dev6 = 1;

    regWrite((U32)&rSDC_SHRLCH_LBA28, sizeof(LATCH_SHADOW_REG), (U8*)&latch_shadow_reg);
}
#endif

BOOL SDC_ProcessPrd(SATA_DSG *DSG)
{
    BOOL bRtn = FALSE;
    U8 PrdID = INVALID_2F;
    U8 status = SIM_CMD_FAIL;
    BOOL bFirstPrd = FALSE;
    U32 cmd_tag_mask = 0;

    if (DSG == NULL)
    {
        return FALSE;
    }

    if (TRUE == DSG->AtaProtInfo.IsNonDataCmd)
    {
        bRtn = TRUE;
    }
    else
    {
        bRtn = SDC_ParsingOnePrd(DSG, sim_cur_cmd_tag);
    }

    if (TRUE == bRtn)
    {
        // check if the prd end flag have been setted
        if ((g_SimHostCmd[sim_cur_cmd_tag].sector_cnt == g_SimHostCmd[sim_cur_cmd_tag].trans_cnt) &&
            !(SDC_IsLastPrd(DSG)))
        {
            DBG_Printf("cmd have finished, but the end flag have not be setted!\n");
            DBG_Getch();
        }

        // modify host cmd queue status if the prd is a last prd
        /* release current DSG */
        DSG_ReleaseSataDsg(sim_cur_dsg_id);

        if(SDC_HANDLE_READ_CRC_ERR == g_HandleCMDStatus)
        {
            return bRtn;
        }
        
        if (SDC_IsLastPrd(DSG))
        {
            g_HandleCMDStatus = SDC_HANDLE_WAIT_SDBFIS;
        }
        else
        {
            /* get next DSG id */
            sim_cur_dsg_id = DSG->NextDsgId;
        }
    }

    return bRtn;
}

// if have prd which waitting to be handled, return 1
BOOL SDC_CheckPRD()
{
    BOOL bRtn = FALSE;
    U8 PrdID = INVALID_2F;
    U8 status = SIM_CMD_FAIL;
    U8 cmd_tag = 0;
    BOOL bFirstPrd = FALSE;
    U32 uBufMap = 0;
    U32 uBufMapGet;
    U8 ucBuffMapID;

    /* find a comamnd to process */
    if (SDC_HANDLE_NONE == g_HandleCMDStatus)
    {
        if (TRUE == SDC_GetFirstDataReady(&cmd_tag))
        {
            if (TRUE == SDC_IsLastDataReady(cmd_tag))
            {
                sim_cur_cmd_tag = cmd_tag;

                regRead((U32)&rFIRST_DSG_ID[cmd_tag], 1, &sim_cur_dsg_id);
                g_HandleCMDStatus = SDC_HANDLE_PROCESS;
            }
        }
    }

    if (SDC_HANDLE_PROCESS == g_HandleCMDStatus)
    {
        if (TRUE == DSG_IsSataDsgValid(sim_cur_dsg_id))
        {
            DSG_FetchSataDsg(sim_cur_dsg_id, &g_CurrentPrd);
            //HOST_LogInfo("[M] fetch DSG %d\n", sim_cur_dsg_id);//ywn

            /* Non-Data command not need to check buffer map */
            if (TRUE == g_CurrentPrd.AtaProtInfo.IsNonDataCmd)
            {
                bRtn = TRUE;
            }/* read command must enable buffer map */
            else if (FALSE == g_CurrentPrd.AtaProtInfo.IsWriteCmd)
            {
                if( TRUE != g_CurrentPrd.XferCtrlInfo.BuffMapEn)
                {
                    DBG_Getch();
                }
                uBufMap = GetBitMap(&g_CurrentPrd);
                uBufMapGet = SDC_GetReadBufferMap(g_CurrentPrd.XferCtrlInfo.BuffMapId);
                if( uBufMapGet != uBufMap )
                {
                    if(uBufMapGet&~uBufMap)//this condition should not happen
                    {
                        printf("check buffer map error\n");
                        DBG_Getch();
                    }

                    return FALSE;
                }
                else
                {
                    ucBuffMapID = g_CurrentPrd.XferCtrlInfo.BuffMapId;
                    regWrite((U32)&rSDMAC_CurBufMapId, 1, (U8*)&ucBuffMapID);
                    bRtn = TRUE;
                }
            }
            else
            {
                /* do not check buffer map for write command  */
                bRtn = TRUE;
            }
        }
    }

    return bRtn;
}



//BOOL SDC_ProcessReadPrd(U16* psecnt)
//{
//    BOOL btrn = FALSE;
//    U8 prd_id;
//    U8 status = SIM_CMD_FAIL;
//    U8 cmd_tag = 0;
//    BOOL bFirstPrd = FALSE;
//    SATA_PRD_ENTRY rprd = {0};
//
//    *psecnt = 0;
//
//    SDC_GetPrdBaseAddr();
//
//
//    prd_id = SDC_GetCurRPTRForReadPrd();
//
//    dramRead((U32)&g_pSDCReadPrd[prd_id], sizeof(SATA_PRD_ENTRY)/4, (U8*)&rprd);
//
//    if (FALSE == sim_sata_dev_b_handle_cmd)
//    {
//        bFirstPrd = TRUE;
//        sim_cur_cmd_tag = rprd.ucTag;
//    }
//
//    //SDC_LogInfo(LOG_FILE, 0, "SDC_ProcessReadPrd() nTag = %d, prdid = %d\n", sim_cur_cmd_tag, prd_id);
//
//    if( INVALID_2F != prd_id )
//    {
//        if( TRUE == SDC_ParsingOneReadPrd(&rprd, sim_cur_cmd_tag))
//        {
//            SDC_AccumulateRPTRReadPrd();
//            sim_sata_dev_b_handle_cmd = TRUE;
//            status = SIM_CMD_SUCCESS;
//            bFirstPrd = FALSE;
//            sim_sata_dev_pending_prd_bread = 0;
//
//            if (0 == rprd.ucSecCnt)
//                *psecnt = (U16)SDC_GetSectorsInPage();
//            else
//                *psecnt = (U16)rprd.ucSecCnt;
//
//            if(SDC_IsLastPrd(&rprd))
//            {
//                g_SimHostCmd[sim_cur_cmd_tag].end_time = sata_sim_clocktime() + ceil((rprd.ucSecCnt * SATA_SIM_SEC_TRANSFER_TIME)/32);
//                g_SimHostCmd[sim_cur_cmd_tag].cmd_status = SIM_CMD_SUCCESS;
//                Host_CMDFinish(sim_cur_cmd_tag);
//                sim_cur_cmd_tag = SIM_HOST_CMD_MAX;
//                sim_sata_dev_b_handle_cmd = FALSE;
//                sim_sata_dev_pending_prd_bread = 1;
//
//                //SDC_LogInfo(LOG_FILE, 0, "SDC_ProcessReadPrd(), nTag = %d end cmd \n", sim_cur_cmd_tag);
//
//                return TRUE;
//
//            }
//        }
//        else
//        {
//            if(bFirstPrd)
//            {
//                sim_cur_cmd_tag = SIM_HOST_CMD_MAX;
//                sim_sata_dev_pending_prd_bread = 1;                // first read prd not ready, then try to check write prd
//                sim_sata_dev_b_handle_cmd = FALSE;
//
//                return FALSE;
//            }
//            else
//                return TRUE;
//        }
//
//    }
//
//    return btrn;
//}
//

BOOL SDC_SpecialCMDDataTransfer(U8 uCmdTag, U32 ulSecCnt, U32 pDramAddr, BOOL bWrite)
{
    U8 *pHostDataBuffer = (U8*)g_SimHostCmd[uCmdTag].pDataBuffer;

    if (TRUE == bWrite)
    {
        memcpy((void*)pDramAddr, pHostDataBuffer, ulSecCnt << SEC_SIZE_BITS);
    }
    else
    {
        memcpy(pHostDataBuffer, (void*)pDramAddr, ulSecCnt << SEC_SIZE_BITS);
    }

    return TRUE;

}

BOOL SDC_TrimDataTransfer(U8 uCmdTag, U32 ulSecCnt, U32 pDramAddr)
{
    U32 ulSecIndex = 0;
    TRIM_CMD_ENTRY *pTrimCmdEntry = (TRIM_CMD_ENTRY*)g_SimHostCmd[uCmdTag].pDataBuffer;
    U32 ulLbaEntryIndex = 0;
    U32 ulStartLba;
    U32 ulLba, ulLen;

    memcpy((void*)pDramAddr, g_SimHostCmd[uCmdTag].pDataBuffer, ulSecCnt << SEC_SIZE_BITS);
    g_SimHostCmd[uCmdTag].trans_cnt += (U16)ulSecCnt;

    for (ulSecIndex = 0; ulSecIndex < ulSecCnt; ulSecIndex++)
    {
        for(ulLbaEntryIndex = 0; ulLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX; ulLbaEntryIndex++)
        {
            ulStartLba = pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow;
            ulLen = pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength;
            for(ulLba=ulStartLba; ulLba< (ulStartLba + ulLen); ulLba++)
            {
                Host_SaveDataCnt(ulLba,0);//set lba's write count to 0
            }
         }
        pTrimCmdEntry++;


    }

    return TRUE;
}

BOOL SDC_LBADataTransfer(U8 uCmdTag, U32 ulSecCnt, U32 pDramAddr)
{
    U32 ulSecIndex = 0;
    TRIM_CMD_ENTRY *pTrimCmdEntry = 0;
    U32 ulLbaEntryIndex = 0;
    //U32 ulStartLba;
    //U32 ulLba, ulLen;
    U32 ulDramAddr = 0;
    BOOL bWrtie = FALSE;
    U32 ulCmdTransLba = g_SimHostCmd[uCmdTag].start_lba + g_SimHostCmd[uCmdTag].trans_cnt;

    bWrtie = ATA_HostIsWriteCmd(g_SimHostCmd[uCmdTag].cmd_code);
    for (ulSecIndex = 0; ulSecIndex < ulSecCnt; ulSecIndex++)
    {
        ulDramAddr = pDramAddr + (ulSecIndex << 9);
        if( FALSE == bWrtie)
        {
            U32 nReadLba;
            U32 nWriteCnt;
            dramRead(ulDramAddr, 1, (U8*)&nWriteCnt);
            dramRead((ulDramAddr+4), 1, (U8*)&nReadLba);

            if(FALSE == SDC_DataCmp(ulCmdTransLba,nWriteCnt, nReadLba))
            {
                //DBG_Getch();
            }

            if(SDC_HANDLE_READ_CRC_ERR == g_HandleCMDStatus)
            {
                /* CRC error triggered, report status to FW side */
                break;
            }
        }
        else
        {
            U32 nWriteCnt = (U32)Host_UpdateDataCnt(ulCmdTransLba);
            dramWrite(ulDramAddr, 1, (U8*)&nWriteCnt);
            dramWrite((ulDramAddr+4), 1, (U8*)&ulCmdTransLba);
        }

        g_SimHostCmd[uCmdTag].trans_cnt++;
        ulCmdTransLba++;
        //nTotalTransCnt++;
    }

    return TRUE;
}

U32 WriteCmd[2][1024] = { 0 };
U32 WriteCatchSatus[2][1024] = { 0 };
U32 l_WriteCmdIndex = 0;

/****************************************************************************
Name        :SDC_DataTransfer
Input       :   fk_bit_msk -- which 4K in PRD which have data to be moved.
cmd_tag    -- the PRD belongs to which cmd. the tag is the index of CMD queue.
p_prd      -- pointer point to the PRD address.
Output      :
Author      :
Date        :
Description :if WRITE PRD is not empty,then handle the WRITE PRD which read pointer pointed.
Others      :
Modify      :
****************************************************************************/
BOOL SDC_DataTransfer(U32 uBufMap, U8 uCmdTag, SATA_DSG *pPrd)
{

    U32 nCmdTransLba = g_SimHostCmd[uCmdTag].start_lba + g_SimHostCmd[uCmdTag].trans_cnt;
    BOOL bClear_Chache_Status = FALSE;
    U8  bRead = !pPrd->AtaProtInfo.IsWriteCmd;
    U8 uBufQueue[4] = {0};
    U32 nLoop = 0;
    U32 nTransCnt = 0;
    U32 nTotalTransCnt = 0;
    U32 uDramAddr = 0;
    U32 uBufAddr = 0;

    if ((g_SimHostCmd[uCmdTag].cmd_code == ATA_CMD_READ_FPDMA_QUEUED && bRead == 0)
        || (g_SimHostCmd[uCmdTag].cmd_code == ATA_CMD_WRITE_FPDMA_QUEUED && bRead == 1))
    {
        SDC_LogInfo(LOG_FILE, 0, "SDC_DataTransfer have error!");
    }


    if (g_SimHostCmd[uCmdTag].trans_cnt == g_SimHostCmd[uCmdTag].sector_cnt)
    {
        return TRUE;
    }

    nTransCnt = (pPrd->XferCtrlInfo.BuffLen == 0) ? 256 : pPrd->XferCtrlInfo.BuffLen;
    uDramAddr = pPrd->DataAddr + DRAM_START_ADDRESS;

    if (bRead == FALSE)
    {
        //SystemStatisticRecord("SDC_DataTransfer: Tag = %d, uDramAdd = 0x%x \n", uCmdTag, uDramAddr);
        WriteCmd[0][l_WriteCmdIndex] = uCmdTag;
        WriteCmd[0][l_WriteCmdIndex] = uDramAddr;

    }
    switch (g_SimHostCmd[uCmdTag].cmd_code)
    {
        case ATA_CMD_DATA_SET_MANAGEMENT:
        {
            SDC_TrimDataTransfer(uCmdTag, nTransCnt, uDramAddr);
        }
            break;
        case ATA_CMD_WRITE_DMA:
        case ATA_CMD_READ_DMA:
        case ATA_CMD_WRITE_DMA_EXT:
        case ATA_CMD_READ_DMA_EXT:
        case ATA_CMD_READ_FPDMA_QUEUED:
        case ATA_CMD_WRITE_FPDMA_QUEUED:
        {
            SDC_LBADataTransfer(uCmdTag, nTransCnt, uDramAddr);
        }
            break;

        default:
        {
            SDC_SpecialCMDDataTransfer(uCmdTag, nTransCnt, uDramAddr, pPrd->AtaProtInfo.IsWriteCmd);
        }
            break;

    }

    if (TRUE == pPrd->XferCtrlInfo.CacheStsEn)
    {
        U32 ulCacheStsData = (U32)(pPrd->CacheStsData);
        if (bRead == FALSE)
        {
            //SystemStatisticRecord("SDC_DataTransfer: Tag = %d, uDramAdd = 0x%x , ulCacheStsData = %d, CacheStsLocSel = %d\n", uCmdTag, uDramAddr, pPrd->CacheStsData, pPrd->CacheStsLocSel);
            WriteCatchSatus[0][l_WriteCmdIndex] = pPrd->CacheStsData << 8 | pPrd->CacheStsLocSel;
            WriteCatchSatus[0][l_WriteCmdIndex] = pPrd->CacheStsAddr;
        }

        if(FALSE == pPrd->CacheStsLocSel)
        {
            Comm_WriteOtfbByByte(pPrd->CacheStsAddr, 1, (U8 *)(&ulCacheStsData));
        }
        else
        {
            Comm_WriteDramByByte(pPrd->CacheStsAddr, 1, (U8 *)(&ulCacheStsData));
        }
    }

    if (bRead == FALSE)
    {
        l_WriteCmdIndex++;
        l_WriteCmdIndex &= 0x400;
    }

    if (g_SimHostCmd[uCmdTag].trans_cnt > g_SimHostCmd[uCmdTag].sector_cnt)
    {
        DBG_Printf("xfer error Tag 0x%x trans_cnt 0x%x\n", uCmdTag, g_SimHostCmd[uCmdTag].trans_cnt);
        DBG_Getch();
    }
    else if (g_SimHostCmd[uCmdTag].trans_cnt == g_SimHostCmd[uCmdTag].sector_cnt)
    {
#ifdef FW_CTRL_ALL_SDBFISREADY
#ifdef SIM_XTENSA
        U32 cmd_tag_mask;
        U32 sdc_intsrcpending;
        cmd_tag_mask = (1<<sim_cur_cmd_tag);
        regWrite((U32)&rSDC_NCQCMD_HOLD_TAG, 4, (U8*)&cmd_tag_mask);

        sdc_intsrcpending = BIT_SDC_INTSRC_NCQFINISH;
        regWrite((U32)&rSDC_IntSrcPending, 4, (U8*)&sdc_intsrcpending);

        setInterrupt(9, 1);
#else
        //HAL_NCQCmdFinish();
#endif
#endif
    }

    return TRUE;
}




/****************************************************************************
Name        :SDC_ParsingOneReadPrd
Input       :
cmd_tag    -- the PRD belongs to which cmd. the tag is the index of CMD queue.
p_prd      -- pointer point to the PRD address.
Output      :
Author      :
Date        :
Description :check the PRD's data if ready.
Others      :
Modify      :
****************************************************************************/
BOOL SDC_ParsingOnePrd(SATA_DSG * pDsg, U8 uCmdTag)
{
    /*we will not simulate our of order for read operations*/
    U32 uSecCnt = 0;
    U32 uBufMap = 0;
    U32 nLoop = 0;
    U8 nBufMapID = 0;
    SGLENTRY *pBufInfo = NULL;

    if (NULL == pDsg)
    {
        //SDC_LogInfo(LOG_ALL, 0, "sata sim: SDC_ParsingOneReadPrd--- p_rprd == NULL \n");
        return FALSE;
    }

    SDC_DataTransfer(uBufMap,uCmdTag, pDsg);

    if(pDsg->AtaProtInfo.IsWriteCmd && pDsg->XferCtrlInfo.BuffMapEn)
    {
        uBufMap = GetBitMap(pDsg);
        SDC_SetWriteBufferMap(pDsg->XferCtrlInfo.BuffMapId, uBufMap);
    }

    return TRUE;
}

U32 GetBitMap(SATA_DSG *pBufInfo)
{
    U8 uSecOffset = pBufInfo->XferCtrlInfo.BuffOffset;
    U8 uMapSizeSec = SATA_SEC_PER_MAP;
    U16 uSecLen = pBufInfo->XferCtrlInfo.BuffLen;

    U32 uBitMap = 0;
    U32 uSecIndex = 0;
    U32 nLoop = 0;



    U8 uSecOffsetMap = uSecOffset - (uSecOffset >> SATA_SEC_MAP_BITS) * SATA_SEC_PER_MAP;

    if (0 == uSecLen)
        uSecLen = 256;

    for (uSecIndex = uSecOffset - uSecOffsetMap;uSecIndex < (U32)(uSecOffset + uSecLen); uSecIndex +=8)
    {
        uBitMap |= (1<< nLoop);
        nLoop++;
    }

    uBitMap = uBitMap << (uSecOffset / SATA_SEC_PER_MAP);

    return uBitMap;
}

//BOOL SDC_ParsingOneWritePrd(SATA_PRD_ENTRY * p_wprd, U8 cmd_tag)
//{
//    U32 start_4K, end4_K;
//    U32 req_4K_msk;
//    U32 start_Off, end_Off;
//    U8 sec_cnt;
//
//    if (NULL == p_wprd)
//    {
//        return FALSE;
//    }
//
//    start_Off = p_wprd->ucBuffOffset;
//
//    if (0 == p_wprd->ucSecCnt)
//        sec_cnt = (U8)SDC_GetSectorsInPage();
//    else
//        sec_cnt = p_wprd->ucSecCnt;
//
//    end_Off = start_Off + sec_cnt - 1;
//
//    start_4K = start_Off >> SATA_SEC_PER_4K_BITS;
//    end4_K = end_Off >> SATA_SEC_PER_4K_BITS;
//
//    req_4K_msk = Get4KBitMsk(start_4K, end4_K);
//
//    SDC_DataTransfer(req_4K_msk, cmd_tag, p_wprd);
//
//    if(p_wprd->bUpdateWriteBufmapEn)
//        SDC_SetWriteBufferMap(p_wprd->ucBufMapID,req_4K_msk);
//
//    return TRUE;
//}


/****************************************************************************
Name        :SDC_CmdTrigger
Input       :
Output      :
Author      :
Date        :
Description :put a cmd to shadow register,when SDC is in idle and rSDC_IntSrcPending is not set.
Others      :
Modify      :
****************************************************************************/
BOOL SDC_CmdTrigger(void)
{
    U8 ntag = 0;
    U8 ucDeviceStatus;
    U32 sdc_intsrcpending;
    U32 sdc_intmask;

    static U32 nNextCmdTag = 0;

    SATA_H2D_REGISTER_FIS *pCmdFis = NULL;

    //trigger command only after device clears BSY flag.
    regRead((U32)&rSDC_IntMask, 2, (U8*)&sdc_intmask);
    regRead((U32)&rSDC_IntSrcPending, 2, (U8*)&sdc_intsrcpending);
    regRead((U32)&rSDC_COMMAND_STATUS, 1, &ucDeviceStatus);

    if (((ucDeviceStatus & BIT_SDC_COMMAND_STATUS_BSY) != 0) ||
        ((sdc_intsrcpending & MSK_4F) != 0) ||
        ((sdc_intmask & BIT_SDC_INTSRC_FIS_COMMAND) != 0))
    {
        return FALSE;
    }

#ifndef  SIM_SATA_CMD_FIFO
    nNextCmdTag = 0;
#endif

    for (ntag = 0; ntag < SIM_HOST_CMD_MAX; ntag++)
    {

        if (SIM_CMD_PENDING == g_SimHostCmd[nNextCmdTag].cmd_status)
        {
            pCmdFis = (SATA_H2D_REGISTER_FIS*)&g_SimHostCmd[nNextCmdTag].FCMDFis;

            g_SimHostCmd[nNextCmdTag].cmd_status = SIM_CMD_SENT;
            g_SimHostCmd[nNextCmdTag].start_time = sata_sim_clocktime();

            //copy original CFIS to SDC register
            regWrite((U32)&rSDC_AtaCFisDW0, sizeof(U32), (U8*)(0x0 + (U32)pCmdFis));
            regWrite((U32)&rSDC_AtaCFisDW1, sizeof(U32), (U8*)(0x4 + (U32)pCmdFis));
            regWrite((U32)&rSDC_AtaCFisDW2, sizeof(U32), (U8*)(0x8 + (U32)pCmdFis));
            regWrite((U32)&rSDC_AtaCFisDW3, sizeof(U32), (U8*)(0xC + (U32)pCmdFis));
            regWrite((U32)&rSDC_AtaCFisDW4, sizeof(U32), (U8*)(0x10 + (U32)pCmdFis));

            ucDeviceStatus = (BIT_SDC_COMMAND_STATUS_BSY | BIT_SDC_COMMAND_STATUS_DRDY | BIT_SDC_COMMAND_STATUS_CD);
            sdc_intsrcpending = BIT_SDC_INTSRC_FIS_COMMAND;
            regWrite((U32)&rSDC_IntSrcPending, 2, (U8*)&sdc_intsrcpending);
            regWrite((U32)&rSDC_COMMAND_STATUS, 1, &ucDeviceStatus);

            //printf("sata SDC_CmdTrigger startlba = %d, seccnt = %d, cmdcode = %d \n", start_lba, sec_cnt, g_SimHostCmd[ntag].cmd_code);
            //SDC_LogInfo(LOG_FILE, 0, "sata SDC_CmdTrigger startlba = %d, seccnt = %d, cmdcode = %d  tag = %d\n", start_lba, sec_cnt, g_SimHostCmd[ntag].cmd_code, ntag);
#ifdef SIM
            EnterCriticalSection(&g_CMDQueueCriticalSection);
            L0_SataISR();
            LeaveCriticalSection(&g_CMDQueueCriticalSection);
#endif
#ifdef SIM_XTENSA
            setInterrupt(9, 1);
#endif

            nNextCmdTag++;
            if (nNextCmdTag == SIM_HOST_CMD_MAX)
                nNextCmdTag = 0;

            break;
        }

        nNextCmdTag += 1;
        if (nNextCmdTag == SIM_HOST_CMD_MAX)
        {
            nNextCmdTag = 0;
        }
    }

    return FALSE;
}

#ifdef SIM
void SDC_ModelPowerUp()
{
    g_bSDCThreadExit = FALSE;
    sim_sata_dev_status = SATA_SIM_IDLE;
    g_HandleCMDStatus = 0;
    //rSDC_SendSDBFISReady;
    l_pSDBFisReady = (U32 *)(&rSDC_SendSDBFISReady);
    l_prGLG_58 = (U16*)(&rGlbIntMsk0);
    *(l_prGLG_58) |= TOP_INTSRC_SDC;
    SDC_InitFirstDataRead();
    g_hSDCThread = CreateThread(0, 0, SDC_ModelThread, 0, 0, 0);

}
#endif

void SDC_ModelInit(void)
{
    U16 usIntMskRegVal = 0;
    //SDC_IniReadPrd();



    InitializeCriticalSection(&g_CMDQueueCriticalSection);
    InitializeCriticalSection(&g_ReadPrdCriticalSection);
    InitializeCriticalSection(&g_WritePrdCriticalSection);
    InitializeCriticalSection(&g_BUFMAPCriticalSection);
    InitializeCriticalSection(&g_FirstDataReadyCriticalSection);


#ifndef SIM_XTENSA
    //InitializeCriticalSection(&g_CMDQueueCriticalSection);
    //InitializeCriticalSection(&g_ReadPrdCriticalSection);
    //InitializeCriticalSection(&g_WritePrdCriticalSection);

#else
    DSG_InitSataDsg();

    g_bSDCThreadExit = FALSE;
    sim_sata_dev_status = SATA_SIM_IDLE;
    g_HandleCMDStatus = 0;
    //rSDC_SendSDBFISReady;
    //l_pSDBFisReady = (U32 *)(&rSDC_SendSDBFISReady);
    //l_prGLG_58 = (U16*)(&rGlbIntMsk0);
    //*(l_prGLG_58) |= TOP_INTSRC_SDC;
    usIntMskRegVal |= TOP_INTSRC_SDC;
    regWrite(&rGlbIntMsk0, 2, (U8 *)&usIntMskRegVal);
    SDC_InitFirstDataRead();
#endif
}

void SDC_ModelSchedule(void)
{
#ifdef NO_THREAD
    SDC_ModelProcess();
#else
    SetEvent(g_SimSDCEvent);
#endif
}

#ifdef SIM_XTENSA
void SDC_ModelThread_XTENSA(void)
{
    BOOL bContinue = TRUE;
    while (bContinue)
    {
        //WaitForSingleObject(g_SimSDCEvent,INFINITE);
        SDC_ModelProcess();

        XTMP_wait(1);
    }
    return 0;
}
#endif

#ifdef SIM
BOOL SDC_ModeleThreadExit()
{
    g_bSDCThreadExit = TRUE;
    WaitForSingleObject(g_hSDCThread, INFINITE);
    CloseHandle(g_hSDCThread);

    return TRUE;
}

DWORD WINAPI SDC_ModelThread(LPVOID p)
{
    while (FALSE == g_bSDCThreadExit)
    {
        WaitForSingleObject(g_SimSDCEvent,INFINITE);
        SDC_ModelProcess();
    }
    return 0;
}
#endif

void SDC_SetD2HFis()
{
    U8 bAbortTag = 0;
    if (4 == rSDC_FEATURE_ERROR)
    {
        // about all host cmds
        Host_SimClearHCmdQueueInterface();
        return;
    }
    else if (g_SimHostCmd[0].cmd_status != SIM_CMD_SENT) // if sim SoftReset and ComReset,this condition will be rewrite
    {
        DBG_Getch();

    }

    // this will read reg to set Success,fail and abort;
    g_SimHostCmd[0].cmd_status = SIM_CMD_SUCCESS;
    Host_CMDFinish(0);
    //g_SimHostCmd[0].cmd_status = SIM_CMD_NONE;
    DBG_Printf("Host none data finish\n");
}

void SDC_CheckCmdFinish(void)
{
    U8 ucCmdCode = g_SimHostCmd[sim_cur_cmd_tag].FCMDFis.Command;
    U8 ucDeviceStatus;

    if (SDC_HANDLE_WAIT_SDBFIS == g_HandleCMDStatus)
    {
        if (ATA_HostIsPIOReadCmd(ucCmdCode))
        {
            g_SimHostCmd[sim_cur_cmd_tag].cmd_status = SIM_CMD_SUCCESS;
            Host_CMDFinish(sim_cur_cmd_tag);
            g_HandleCMDStatus = SDC_HANDLE_NONE;
        }
        else if (ATA_HostIsPIOWriteCmd(ucCmdCode))
        {
            // do nothing to wait D2H FIS to end the cmd
            g_HandleCMDStatus = SDC_HANDLE_NONE;
        }
        else
        {
            if(TRUE == SDC_CheckSDBFISReady(sim_cur_cmd_tag))
            {
                ucDeviceStatus = (BIT_SDC_COMMAND_STATUS_DRDY | BIT_SDC_COMMAND_STATUS_CD);
                regWrite((U32)&rSDC_COMMAND_STATUS, 1, &ucDeviceStatus);
                g_SimHostCmd[sim_cur_cmd_tag].cmd_status = SIM_CMD_SUCCESS;
                Host_CMDFinish(sim_cur_cmd_tag);
                g_HandleCMDStatus = SDC_HANDLE_NONE;
            }
        }

        if (SDC_HANDLE_NONE == g_HandleCMDStatus)
        {
             /* release resource and set sdc status */
            g_SimHostCmd[sim_cur_cmd_tag].end_time = sata_sim_clocktime(); // + ceil((rprd.ucSecCnt * SATA_SIM_SEC_TRANSFER_TIME)/32);
            /* clear current tag's last data ready and move to next tag */
            SDC_ClearFirstReadDataReady(sim_cur_cmd_tag);
            SDC_ClearLastDataReady(sim_cur_cmd_tag);

            sim_cur_cmd_tag++;
            sim_cur_cmd_tag %= SIM_HOST_CMD_MAX;
        }
    }

    if( SDC_HANDLE_READ_CRC_ERR == g_HandleCMDStatus)              
    {
        if(TRUE == SDC_CheckSDBFISReady(sim_cur_cmd_tag))
        {
            g_SimHostCmd[sim_cur_cmd_tag].cmd_status = SIM_CMD_READ_CRCERR;
            Host_CMDFinish(sim_cur_cmd_tag);
            g_HandleCMDStatus = SDC_HANDLE_NONE;
        }
        
        if (SDC_HANDLE_NONE == g_HandleCMDStatus)
        {
             /* release resource and set sdc status */
            g_SimHostCmd[sim_cur_cmd_tag].end_time = sata_sim_clocktime(); // + ceil((rprd.ucSecCnt * SATA_SIM_SEC_TRANSFER_TIME)/32);
            /* clear current tag's last data ready and move to next tag */
            SDC_ClearFirstReadDataReady(sim_cur_cmd_tag);
            SDC_ClearLastDataReady(sim_cur_cmd_tag);

            sim_cur_cmd_tag++;
            sim_cur_cmd_tag %= SIM_HOST_CMD_MAX;
        }
    }
            
    return;
}

/****************************************************************************
Name        :SDC_ModelProcess
Input       :
Output      :
Author      :
Date        :
Description :SDC main process
Others      :
Modify      :
****************************************************************************/
void SDC_ModelProcess(void)
{
    U16 usIntMskRegVal;
    static float trans_start_time = 0;
    static float trans_busy_time = 0;
    //U32 cur_time;

    // Device's interrupt don't be enable, so return.
    regRead((U32)&rGlbIntMsk0, 2, (U8 *)&usIntMskRegVal);
    if (TOP_INTSRC_SDC == (usIntMskRegVal & TOP_INTSRC_SDC))
    {
        return;
    }

#ifdef IGNORE_PERFORMANCE

    SDC_CmdTrigger();
    if (SDC_CheckPRD())
    {
        SDC_ProcessPrd(&g_CurrentPrd);
    }
    SDC_CheckCmdFinish();



#else
    switch(sim_sata_dev_status)
    {
    case SATA_SIM_HANDLE_CMD:
        SDC_ProcessPrd(&g_CurrentPrd);
        sim_sata_dev_status = SATA_SIM_CHECK;
        break;
    case SATA_SIM_IDLE:
        SDC_CmdTrigger();
        sim_sata_dev_status = SATA_SIM_CHECK;//Note: SATA_SIM_IDLE & SATA_SIM_CHECK is done in a loop;
        break;
    case SATA_SIM_CHECK:
        // first try to check write prd
        if (SDC_CheckPRD())
            sim_sata_dev_status = SATA_SIM_HANDLE_CMD;
        else
            sim_sata_dev_status = SATA_SIM_IDLE;

        //nWait = 1;
        break;
    default:
        break;
    }//end switch

#endif
}
