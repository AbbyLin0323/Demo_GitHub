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
#include "simsatainc.h"
#ifdef SIM
#include "flash_meminterface.h"
#else
#include "../model/256fm_win32/hfmid.h"
#endif
//#include "L2_Interface.h"   // for Low level format LLF


#include "SimATACmd.h"
#include "checklist_parse.h"
#include "hscmd_parse.h"
#include "model_common.h"

BOOL Host_IsCMDEmptyInterface();

volatile SIM_SATA_CMD g_SimHostCmd[SIM_HOST_CMD_MAX];
SIM_SATA_HOST_MGR l_stSataHostMGR = {0};

TRIM_CMD_ENTRY g_TrimCmdEntry[TRIM_CMD_SEC_CNT_MAX];//DMA out sector cnt max is 256
U32 g_TrimCmdEntryHead = 0;
U32 g_TrimCmdEntryTail = 0;

#define G_PAGE_SIZE ((1024 * 1024 * 1024)/LOGIC_PG_SZ)
#define MAX_SEND_SEC (256)


void Host_ModelInitInterface()
{
    U8 i;

    for( i = 0; i < 32; i++ )
    {
        g_SimHostCmd[i].cmd_status = SIM_CMD_NONE;
    }
}


void SATA_HCmdToRowCmd(HCMD_INFO* pHcmd)
{
    PSATA_H2D_REGISTER_FIS pCmdFis = (PSATA_H2D_REGISTER_FIS)&pHcmd->HSCmd.RowCmd;
    HSCMD_INFO *pHSCmd = &pHcmd->HSCmd;
    U32 StartLba = pHcmd->StartLba;
    U32 SecCnt = pHcmd->SecCnt;
    U32 CmdCode;

    switch(pHcmd->CmdType)
    {
    case CMD_TYPE_READ:
    case CMD_TYPE_WRITE:
        //Do nothing, fis is set before
        break;
    case CMD_TYPE_TRIM:
        pCmdFis->Command = ATA_CMD_DATA_SET_MANAGEMENT;
        strcpy(pHSCmd->CmdDsc, "Trim");
        pHSCmd->DataSecCnt = SecCnt;
        pHSCmd->Protocal = PROT_DMA_OUT;
        pHSCmd->pDataBuffer = (U8 *)g_TrimCmdEntry;
        break;
    case CMD_TYPE_STANDBY:
        pCmdFis->Command = ATA_CMD_STANDBY_IMMEDIATE;
        strcpy(pHSCmd->CmdDsc, "Standby");
        pHSCmd->DataSecCnt = 0;
        pHSCmd->Protocal = PROT_NONDATA;
        break;
    case CMD_TYPE_HSCMD:
        if(PROT_NONDATA != pHSCmd->Protocal)
        {
            SecCnt = pHSCmd->DataSecCnt;
        }
        else
        {
            SecCnt = pCmdFis->Count7_0 + (pCmdFis->Count15_8 << 8);
        }
        break;
    default:
        DBG_Printf("HCmd CmdType error\n");
        DBG_Break();
        break;
    }

    CmdCode = pCmdFis->Command;

    if(CMD_TYPE_TRIM == pHcmd->CmdType)
    {
        pCmdFis->LBA7_0 = StartLba & 0xFF;
        pCmdFis->LBA15_8 = (StartLba >> 8) & 0xFF;
        pCmdFis->LBA23_16 = (StartLba >> 16) & 0xFF;
        pCmdFis->LBA31_24 = (StartLba >>24) & 0xFF;
    }

    if(CMD_TYPE_READ != pHcmd->CmdType &&
        CMD_TYPE_WRITE != pHcmd->CmdType)
    {
        pCmdFis->Count7_0 = (UCHAR)(SecCnt & 0x000000ff);
        pCmdFis->Count15_8 = (UCHAR)((SecCnt>>8)&0x000000ff);
    }

    if (ATA_CMD_DATA_SET_MANAGEMENT == CmdCode)
    {
        pCmdFis->Feature7_0 |= 0x1;
    }

    // if want to send SOFT_RESET command, clear CFIS.C to zero,
    // else set CFIS.C to one
    pCmdFis->C = TRUE;
}



BOOL SATA_HostLbaIsHit(U32 StartLba, U32 SectorCnt)
{
    U32 ulNewLbaStart = StartLba;
    U32 ulNewLbaEnd = 0;
    U32 ulPendingLbaStart = 0;
    U32 ulPendingLbaEnd = 0;
    U8 uCmdTag = 0;
    BOOL bHit = FALSE;

    ulNewLbaEnd = ulNewLbaStart + SectorCnt - 1;

    for (uCmdTag = 0; uCmdTag < 32; uCmdTag++)
    {
        if (SIM_CMD_NONE != g_SimHostCmd[uCmdTag].cmd_status)
        {
            bHit = TRUE;
            ulPendingLbaStart = g_SimHostCmd[uCmdTag].start_lba;
            ulPendingLbaEnd = ulPendingLbaStart + g_SimHostCmd[uCmdTag].sector_cnt - 1;
            if ((ulNewLbaStart > ulPendingLbaEnd) || (ulNewLbaEnd < ulPendingLbaStart))
            {
                bHit = FALSE;
            }

            if (TRUE == bHit)
            {
                break;
            }
        }
    }

    return bHit;
}

BOOL SATA_SendHCmdToDevice(U8 ucCmdTag, U32 ulStartLba, U32 ulSecCnt, HSCMD_INFO *pHSCmd)
{
    PSATA_H2D_REGISTER_FIS pCmdFis = (PSATA_H2D_REGISTER_FIS)&(pHSCmd->RowCmd);

    if (SIM_CMD_NONE != g_SimHostCmd[ucCmdTag].cmd_status)
    {
        DBG_Getch();
    }

    memcpy((void *)&g_SimHostCmd[ucCmdTag].FCMDFis, pCmdFis, sizeof(SATA_H2D_REGISTER_FIS));

    g_SimHostCmd[ucCmdTag].cmd_code = pCmdFis->Command;
    g_SimHostCmd[ucCmdTag].start_lba = ulStartLba;
    g_SimHostCmd[ucCmdTag].sector_cnt = ulSecCnt;
    g_SimHostCmd[ucCmdTag].trans_cnt = 0;
    g_SimHostCmd[ucCmdTag].ncq_subcmd = 0;
    g_SimHostCmd[ucCmdTag].ncq_subcmdspec = 0;
    g_SimHostCmd[ucCmdTag].cmd_status = SIM_CMD_PENDING;
    g_SimHostCmd[ucCmdTag].pDataBuffer = pHSCmd->pDataBuffer;

    //if (ulSecCnt != ATA_HostGetNCQSecCnt(&g_SimHostCmd[ucCmdTag].FCMDFis))
    //{
    //    DBG_Getch();
    //}

    //if (ulStartLba != ATA_HostGetCmdLba(&g_SimHostCmd[ucCmdTag].FCMDFis))
    //{
    //    DBG_Getch();
    //}



    return TRUE;
}

BOOL Host_GetEmptySlot(U8 *pCmdTag)
{
    U8 uCmdTag = 0;
    static U8 nNextCMDIndex = 0;
    U32 OutstandingCount;

    /*allocate a new cmd tag for new command*/
#ifndef  SIM_SATA_CMD_FIFO
    nNextCMDIndex = 0;
#endif
    for( uCmdTag = 0;uCmdTag < 32; uCmdTag++ )
    {
        if( g_SimHostCmd[nNextCMDIndex].cmd_status == SIM_CMD_NONE )
        {
            /*g_SimHostCmd[nNextCMDIndex].cmd_code = cmd_code;
            g_SimHostCmd[nNextCMDIndex].start_lba = start_lba;
            g_SimHostCmd[nNextCMDIndex].sector_cnt = sector_cnt;
            g_SimHostCmd[nNextCMDIndex].trans_cnt = 0;
            g_SimHostCmd[nNextCMDIndex].ncq_subcmd = subcmd_info & 0xFF;
            g_SimHostCmd[nNextCMDIndex].ncq_subcmdspec = (U8)(subcmd_info >> 8)&0xFF;
            g_SimHostCmd[nNextCMDIndex].cmd_status = SIM_CMD_PENDING;
            g_SimHostCmd[nNextCMDIndex].pDataBuffer = 0;*/

            *pCmdTag = nNextCMDIndex;
            break;
        }

        nNextCMDIndex += 1;
        if (nNextCMDIndex == 32)
        {
            nNextCMDIndex = 0;
        }
    }

    if(uCmdTag == 32)
    {
        return FALSE;
    }
    else
    {
        nNextCMDIndex += 1;
        if (nNextCMDIndex == 32)
            nNextCMDIndex = 0;
    }

    OutstandingCount = 0;

    for( uCmdTag = 0; uCmdTag < 32; uCmdTag++ )
    {
        if( g_SimHostCmd[uCmdTag].cmd_status == SIM_CMD_SENT )
            OutstandingCount++;
    }
    if(OutstandingCount > 3)
    {
        //DBG_Printf("outstanding IO %d\n",OutstandingCount);
        if(OutstandingCount == 32)
        {
            //DBG_Printf("outstanding IO %d\n",OutstandingCount);

        }
    }

    return TRUE;
}


BOOL SATA_SendNCQCmd(HCMD_INFO* pHCmd)
{

    PSATA_H2D_REGISTER_FIS pCmdFis = (PSATA_H2D_REGISTER_FIS)&(pHCmd->HSCmd.RowCmd);

    //U32 uSendSecCnt = 0;
    //U32 uSendStartLba = 0;
    U32 uSectorCnt = pHCmd->SecCnt;
    U32 uStartLba = pHCmd->StartLba;

    BOOL bRtn = FALSE;
    BOOL bLBAHit = FALSE;
    U8 ucCmdTag = 0;
    U8 ucCmdCode = pCmdFis->Command;

    if (TRUE == l_stSataHostMGR.bHandleNonNCQCmd)
    {
        return FALSE;
    }

    // if equal, this is a new cmd
   /* if (uSendSecCnt == uRemSecCnt)
    {
        uRemSecCnt = uSectorCnt;
        uRemStartLba = uStartLba;
        uSendStartLba = uRemStartLba;
        uSendSecCnt = uRemSecCnt;

    }
    else
    {
        uSendSecCnt = uRemSecCnt;
        uSendStartLba = uRemStartLba;
    }*/

    //uSendSecCnt = (uSendSecCnt > MAX_SEND_SEC)? MAX_SEND_SEC:uSendSecCnt;

    if (ATA_CMD_NCQ_NON_DATA != ucCmdCode)
    {
        bLBAHit = SATA_HostLbaIsHit(pHCmd->StartLba, pHCmd->SecCnt);
    }

    if (FALSE == bLBAHit)
    {
        bRtn = Host_GetEmptySlot(&ucCmdTag);

        if (TRUE == bRtn)
        {
            if (g_SimHostCmd[ucCmdTag].cmd_status != SIM_CMD_NONE)
            {
                DBG_Getch();
            }
            pCmdFis->SectorCount = (ucCmdTag << 3);
            bRtn = SATA_SendHCmdToDevice(ucCmdTag, pHCmd->StartLba, pHCmd->SecCnt, &pHCmd->HSCmd);
        }
    }

   /* if (TRUE == bRtn)
    {
        uRemSecCnt -= uSendSecCnt;
        uRemStartLba = uSendStartLba + uSendSecCnt;
    }*/

    /*if (0 != uRemSecCnt)
        bRtn = FALSE;*/


    return bRtn;
}

BOOL SATA_HostSendHSCMD(HSCMD_INFO *pHSCmd)
{
    PSATA_H2D_REGISTER_FIS pCmdFis = (PSATA_H2D_REGISTER_FIS)&(pHSCmd->RowCmd);
    U32 ulStartLba = ATA_HostGetCmdLba(pCmdFis);
    U32 ulSecCnt = 0;
    U8 ucProtocol = pHSCmd->Protocal;

    if (TRUE == ATA_HostIsLbaCmd(pCmdFis->Command))
    {
        ulSecCnt = ATA_HostGetLbaCmdSecCnt(pCmdFis);
    }
    else
    {
        ulSecCnt = pHSCmd->DataSecCnt;
    }
    /* command queue must be empty */
    if (FALSE == Host_IsCMDEmptyInterface())
    {
        return FALSE;
    }

    /* Fill command */
    SATA_SendHCmdToDevice(0, ulStartLba, ulSecCnt, pHSCmd);

    while(FALSE == Host_IsCMDEmptyInterface())
    {
        /* wait HSCMD has been done,then return to callback*/
       Sleep(10);
    }

   /* if ((PROT_PIO_IN== ucProtocol) || (PROT_DMA_IN == ucProtocol))
    {
        if (1 == g_tPortRFis[ucCmdTag].RD2HFis.Error)
        {
            pHSCmd->bDataValid = TRUE;
        }
        AHCI_HostCopyDataBack(ucCmdTag, pHSCmd->pDataBuffer);
    }*/

    return TRUE;
}


BOOL Host_SendOneCmdInterface(HCMD_INFO* pHCmd)
{
    BOOL bRtn = FALSE;
    PSATA_H2D_REGISTER_FIS pCmdFis = (PSATA_H2D_REGISTER_FIS)&pHCmd->HSCmd.RowCmd;
    U8 ucCmdCode = 0;

    SATA_HCmdToRowCmd(pHCmd);
    ucCmdCode = pCmdFis->Command;
    if (TRUE == ATA_IsNCQCMD(ucCmdCode))
    {
        bRtn = SATA_SendNCQCmd(pHCmd);
    }
    else
    {
        bRtn = SATA_HostSendHSCMD(&pHCmd->HSCmd);
    }

    return bRtn;
}



void Host_CMDFinish(U8 cmdtag)
{
    static U32 per_cmd_end_time;
    static U32 host_cmd_num;

    //U16 sdc_intsrcpending;

    if (g_SimHostCmd[cmdtag].cmd_status != SIM_CMD_SUCCESS &&
        g_SimHostCmd[cmdtag].cmd_status != SIM_CMD_FAIL &&
        g_SimHostCmd[cmdtag].cmd_status != SIM_CMD_READ_CRCERR
        )
    {
        DBG_Getch();
        //HOST_LogInfo(LOG_FILE, 0, "[Err]Host_CMDFinish : start_lba=%d, tag=%d \n", g_SimHostCmd[cmdtag].start_lba, cmdtag);

    }
    g_SimHostCmd[cmdtag].cmd_status = SIM_CMD_NONE;

    if(host_cmd_num > 0)//Ignore first host cmd process time
    {
        g_TransTotalTime += sata_sim_clocktime() - per_cmd_end_time;
        g_TransSecCnt += g_SimHostCmd[cmdtag].sector_cnt;
    }
    per_cmd_end_time = sata_sim_clocktime();
    host_cmd_num++;

    /*HOST_LogInfo(LOG_FILE, 0, "[Info]Host_CMDFinish : start_lba=%d\t tag=%d\t opCode=%d\t secCnt=%d\n",
    g_SimHostCmd[cmdtag].start_lba, cmdtag, g_SimHostCmd[cmdtag].cmd_code, g_SimHostCmd[cmdtag].sector_cnt);*/

#ifndef IGNORE_PERFORMANCE
    /*HOST_LogInfo(LOG_FILE, 0, "[Info]Host_CMDFinish : start_lba=%d\t tag=%d\t opCode=%d\t secCnt=%d\t start_time=%d\t end_time=%d\t\n",
    g_SimHostCmd[cmdtag].start_lba, cmdtag, g_SimHostCmd[cmdtag].cmd_code, g_SimHostCmd[cmdtag].sector_cnt,g_SimHostCmd[cmdtag].start_time,g_SimHostCmd[cmdtag].end_time);*/
#endif
    return;
}

//void Host_NoneDataCMDFinishInterface()
//{
//    if (g_SimHostCmd[0].cmd_status != SIM_CMD_SENT)
//    {
//        DBG_Getch();
//
//    }
//    g_SimHostCmd[0].cmd_status = SIM_CMD_NONE;
//    DBG_Printf("Host none data finish\n");
//}

void Host_SimClearHCmdQueueInterface(void)
{
    int i = 0;
    for( i = 0; i < 32; i++ )
    {
        if (SIM_CMD_NONE != g_SimHostCmd[i].cmd_status)
        {
            g_SimHostCmd[i].cmd_status = SIM_CMD_NONE;
            g_SimHostCmd[i].trans_cnt = 0;
        }
    }
}

BOOL Host_IsHitHCmdTable(U32 ulSystemLba)
{
    U8 uSlotIndex = 0;
    U32 ulStartLba,ulEndLba;

    for (uSlotIndex = 0; uSlotIndex < MAX_SLOT_NUM; uSlotIndex++)
    {
        ulStartLba = g_SimHostCmd[uSlotIndex].start_lba;
        ulEndLba = ulStartLba + g_SimHostCmd[uSlotIndex].sector_cnt - 1;
        if(ulSystemLba >= ulStartLba && ulSystemLba <= ulEndLba)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void Host_GetPerformance()
{
    double perf;
    perf = (double)g_TransSecCnt*1000*1000/1024/g_TransTotalTime; //0.5us per loop
    HOST_LogInfo(LOG_FILE, 0, "Host_GetPerformance :%.2f MBps\n",perf);

}

BOOL Host_IsCMDEmptyInterface()
{
    U8 i = 0;
    for( i = 0; i < 32; i++)
    {
        if( g_SimHostCmd[i].cmd_status != SIM_CMD_NONE)
        {
            return FALSE;
        }
    }

    return TRUE;
}


// Data Check Section

void Host_DataFileDel()
{
    DeleteFileA("data.bin");
}


//void Host_InitData(DWORD dwHowManyArray)
//{
//    DWORD dwArray = dwHowManyArray;
//    g_SC.dwLength = dwArray;
//    g_SC.dwCnt = (DWORD*)malloc(dwArray * sizeof(DWORD));
//    if (!g_SC.dwCnt)
//    {
//        printf("memory alloc failed for the g_SC\n");
//        DBG_Getch();
//    }
//    memset(g_SC.dwCnt, 0, dwArray * sizeof(DWORD));
//}

//void Host_OpenDataFile()
//{
//    g_hSimHostFileData =  CreateFile("data.bin",
//        GENERIC_READ|GENERIC_WRITE,
//        FILE_SHARE_READ|FILE_SHARE_WRITE,
//        NULL,
//        CREATE_ALWAYS,
//        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
//        NULL);
//
//    if(g_hSimHostFileData == INVALID_HANDLE_VALUE)
//    {
//        HOST_LogInfo(LOG_PRINT, 0, "%s file create error %d\r\n", "log.txt", GetLastError());
//        getch();
//    }
//
//    {
//
//        int nPu = CE_NUM;
//        int nPln = PLN_PER_LUN;
//        int nBlk = BLK_PER_PLN;
//        int nPge = PG_PER_BLK;
//        unsigned long ulPgeSize = LOGIC_PG_SZ;
//        unsigned long ulCapacity = ((CE_NUM * PLN_PER_LUN * BLK_PER_PLN * PG_PER_BLK )/G_PAGE_SIZE);
//#ifdef SIM
//        Mid_init_SATA();
//#else
//        Mid_Init_SATA((UINT32)ulCapacity);  // get from chip parameter
//#endif
//    }
//
//}


/* this function is for debug, do not used in formal validation. haven yang */
BOOL Host_SendACommand(U8 cmd_code, U32 start_lba, U16 sector_cnt)
{
    U8 i;
    static U8 nNextCMDIndex = 0;

    /*sending one host command to device, allocate a NCQ command tag for*/
    if( cmd_code == ATA_CMD_READ_FPDMA_QUEUED ||
        cmd_code == ATA_CMD_WRITE_FPDMA_QUEUED )
    {
        /*check if non ncq command existed*/
        if( g_SimHostCmd[0].cmd_status != SIM_CMD_NONE )
        {
            if(g_SimHostCmd[0].cmd_code != ATA_CMD_READ_FPDMA_QUEUED &&
                g_SimHostCmd[0].cmd_code != ATA_CMD_WRITE_FPDMA_QUEUED )
                return FALSE;
        }

        /*allocate a new cmd tag for new command*/
        for( i = 0; i < 32; i++ )
        {
            if( g_SimHostCmd[nNextCMDIndex].cmd_status == SIM_CMD_NONE )
            {
                g_SimHostCmd[nNextCMDIndex].cmd_code = cmd_code;
                g_SimHostCmd[nNextCMDIndex].start_lba = start_lba;
                g_SimHostCmd[nNextCMDIndex].sector_cnt = sector_cnt;
                g_SimHostCmd[nNextCMDIndex].trans_cnt = 0;
                g_SimHostCmd[nNextCMDIndex].cmd_status = SIM_CMD_PENDING;
                nNextCMDIndex += 1;
                nNextCMDIndex %= 32;
                break;
            }
        }

        if(i == 32)
        {
            return FALSE;
        }
    }
    else
    {
        for ( i = 0; i < 32; i++ )
        {
            if ( SIM_CMD_NONE == g_SimHostCmd[i].cmd_status )
                continue;
            else
                return FALSE;
        }

        if( g_SimHostCmd[0].cmd_status == SIM_CMD_NONE )
        {
            g_SimHostCmd[0].cmd_code = cmd_code;
            g_SimHostCmd[0].start_lba = start_lba;
            g_SimHostCmd[0].sector_cnt = sector_cnt;
            g_SimHostCmd[0].trans_cnt = 0;
            g_SimHostCmd[0].cmd_status = SIM_CMD_PENDING;
        }

    }

    return TRUE;
}

