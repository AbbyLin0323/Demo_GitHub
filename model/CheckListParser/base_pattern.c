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

//#include "simsatainc.h"
//#include "..\\satamodel\\simsatahost.h"
//#include "..\\satamodel\\simfilefetch.h"

// include FW
//#include "AHCI_ATACmdCode.h"

// include model
#include "checklist_parse.h"
#include "base_pattern.h"
#include "HostModel.h"
#include "model_config.h"
#include "win_bootloader.h"
#define PATTERN_PATH "D:\\\\pattern"
WIN32_FIND_DATA PatternFindFileData;
HANDLE hPatternFind;
BOOL bPatternFinish;
U32 g_ulMaxLBAInModel = 0;
static U32 g_ulTrimLbaEntryIndex = 0;
static U32 g_ulTrimCmdEntryIndex = 0;
static U32 g_ulDoFileTrimCmd = 0;
extern HSRW_CMD_TYPE g_HSRWCmdType;
extern U32 GetRandomLBAPartern();

#ifndef HOST_NVME
extern void ATA_GetAtaHostCmdDMAH2DFis(char *pRowCmd, U32 ulStartLba, U32 ulSectorCnt, BOOL bRead);
extern void ATA_GetAtaHostCmdNCQH2DFis(char *pRowCmd, U32 ulStartLba, U32 ulSectorCnt, BOOL bRead);
#endif

LOCAL BASE_FUNCTION l_BaseFuncArray[MAX_BASE_FUNCTION] = {0};
TRIM_CMD_ENTRY g_TrimCmdEntry[TRIM_CMD_SEC_CNT_MAX];


void SetSystemMaxLBA(U32 ulMaxLba)
{
    g_ulMaxLBAInModel = ulMaxLba;
    DBG_Printf("\n MaxLBA in System is 0x%x \n", g_ulMaxLBAInModel);
}

U32 GetSystemMaxLBA()
{

    if (0 == g_ulMaxLBAInModel)
    {
        g_ulMaxLBAInModel = MAX_LBA_IN_SYSTEM;
    }

    return g_ulMaxLBAInModel;
}


U32 GetRandomLBAPartern_Not4KAlign()
{
    U32 ulRandLBA;
    U32 ulRandStripe = 0;
    U32 count = 0;

    ulRandLBA = INVALID_8F-3;
    ulRandLBA = rand() * rand() + rand();

    if (ulRandLBA >= (g_ulMaxLBAInModel - 8))
        ulRandLBA = ulRandLBA % (g_ulMaxLBAInModel - 8);


    return (ulRandLBA);
}


U32 GetRandomLBAPartern()
{
    U32 ulRandLBA;
    U32 ulRandStripe = 0;
    U32 ulLpn;
    U32 count = 0;

    ulLpn = INVALID_8F - 3;
    ulLpn = rand() * rand() + rand();
    ulRandLBA = (ulLpn << LPN_SECTOR_BIT);

    if (ulRandLBA >= (g_ulMaxLBAInModel - 8))
        ulRandLBA = ulRandLBA % (g_ulMaxLBAInModel - 8);


    return (ulRandLBA);
}

U32 GetRandomSecCnt_Not4KAlign()
{
    U32 ulSecCnt;
    U32 MaxLBAIn4G = (4*1024*1024*2);
    ulSecCnt = rand()% MaxLBAIn4G;

    return (ulSecCnt);
}

U32 GetRandomLBAPatternInRange(U32 LbaStart, U32 LbaEnd)
{
    U32 ulRandLBA;
    U32 ulRandStripe = 0;
    U32 ulLpn;
    U32 count = 0;

    if(!LbaEnd)
    {
        ulLpn = INVALID_8F - 3;
        ulLpn = rand() * rand() + rand();
        ulRandLBA = (ulLpn << LPN_SECTOR_BIT);

        if (ulRandLBA >= (g_ulMaxLBAInModel - 8))
            ulRandLBA = ulRandLBA % (g_ulMaxLBAInModel - 8);


        return (ulRandLBA);
    }
    else
    {
        if(LbaEnd > (g_ulMaxLBAInModel - 8))
        {
            LbaEnd = g_ulMaxLBAInModel - 8;
        }
        ulRandLBA = LbaStart + rand()%(LbaEnd - LbaStart);

        return (ulRandLBA);
    }
}

U32 GetRandomSecCnt()
{
    U32 ulSecCnt;
    U32 MaxLBAIn4G = (4*1024*1024*2);
    ulSecCnt = rand()% MaxLBAIn4G;
    ulSecCnt = (ulSecCnt>>3)*8;

    return (ulSecCnt);
}


BOOL sim_trim_one_range(U32* pStartLba,U32* pLen)
{
    static U32 start_lba = 0;
    static U32 cmd_cnt = 0;
    static U32 SectorCount=0;
    static U32 ulLbaEntryIndex = 0;
    U32 LBAStart=0;
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    U16 usCmdEntryIndex = 0;
    U16 sec_cnt = 0;
    static BOOL bCmdRes = TRUE;
    HCMD_INFO HCmd;

    HCmd.CmdType = CMD_TYPE_TRIM;

    while (TRUE == bCmdRes)
    {
        start_lba = *pStartLba;

        if(*pLen <= 0xFFFF)
        {
            sec_cnt = *pLen;
        }
        else
        {
            sec_cnt = 0xFFFF;
        }

        *pLen -= sec_cnt;
        *pStartLba += sec_cnt;

        if((start_lba + sec_cnt) >= g_ulMaxLBAInModel)
        {
            start_lba = start_lba  % (g_ulMaxLBAInModel - sec_cnt);
        }


        if (start_lba > g_ulMaxLBAInModel)
            DBG_Printf("this have a error \n");


        pTrimCmdEntry = &g_TrimCmdEntry[usCmdEntryIndex];
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow = start_lba;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaHigh = 0;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength = sec_cnt;

        ulLbaEntryIndex++;
        if(*pLen == 0)
        {
            ulLbaEntryIndex = 0;
            usCmdEntryIndex++;
            SectorCount++;
            break;
        }

        if(ulLbaEntryIndex == TRIM_LBA_RANGE_ENTRY_MAX)
        {
            ulLbaEntryIndex = 0;
            usCmdEntryIndex++;
            SectorCount++;
            if (SectorCount == 8)
            {
                break;
            }
        }
    }
    HCmd.StartLba = LBAStart;
    HCmd.SecCnt = SectorCount;
    HCmd.RangeNum = TRIM_LBA_RANGE_ENTRY_MAX*SectorCount - 1;

    bCmdRes = Host_SendOneCmd(&HCmd);

    if(bCmdRes == TRUE)
    {
        cmd_cnt ++;
        SectorCount = 0;
        if(*pLen == 0)
        {
            cmd_cnt = 0;
            return TRUE;
        }

    }
    return FALSE;
}

FILE_TRIM_RETURN sim_cmd_into_trim_entry(U32 StartLba,U16 SectorCnt)
{
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    FILE_TRIM_RETURN filetrimreturn;

    if(g_ulDoFileTrimCmd == 0)
    {
        pTrimCmdEntry = &g_TrimCmdEntry[g_ulTrimCmdEntryIndex];

        if(g_ulTrimCmdEntryIndex < 2)
        {
            if(g_ulTrimLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX)
            {
                pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaLow = StartLba;
                pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaHigh = 0;
                pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].RangeLength = SectorCnt;
                g_ulTrimLbaEntryIndex++;
                filetrimreturn.ulFileTrimType = FILE_TRIM_TYPE_ADD_LBA_ENTRY_SUCCESS;
                filetrimreturn.ulTrimSecCnt = g_ulTrimCmdEntryIndex;
                return(filetrimreturn);
            }
            else if(g_ulTrimLbaEntryIndex == TRIM_LBA_RANGE_ENTRY_MAX)
            {
                g_ulTrimCmdEntryIndex++;
                g_ulTrimLbaEntryIndex = 0;
                if(g_ulTrimCmdEntryIndex >= 2)
                {
                    //g_ulTrimCmdEntryIndex = 0;
                    g_ulDoFileTrimCmd = 1;
                    filetrimreturn.ulFileTrimType = FILE_TRIM_TYPE_2_SECTOR;
                    filetrimreturn.ulTrimSecCnt = g_ulTrimCmdEntryIndex - 1;
                    return(filetrimreturn);
                }
                else
                {
                    pTrimCmdEntry = &g_TrimCmdEntry[g_ulTrimCmdEntryIndex];
                    pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaLow = StartLba;
                    pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaHigh = 0;
                    pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].RangeLength = SectorCnt;
                    g_ulTrimLbaEntryIndex++;
                    filetrimreturn.ulFileTrimType = FILE_TRIM_TYPE_ADD_LBA_ENTRY_SUCCESS;
                    filetrimreturn.ulTrimSecCnt = g_ulTrimCmdEntryIndex;
                    return(filetrimreturn);
                }
            }
        }
        else
        {
            DBG_Printf("Error trim cmd entry.");
        }
    }
    else
    {
        pTrimCmdEntry = &g_TrimCmdEntry[g_ulTrimCmdEntryIndex];
        while(g_ulTrimLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX)
        {
            pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaLow = 0;
            pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].StartLbaHigh = 0;
            pTrimCmdEntry->LbaRangeEntry[g_ulTrimLbaEntryIndex].RangeLength = 0;
            g_ulTrimLbaEntryIndex++;
        }
        filetrimreturn.ulFileTrimType = FILE_TRIM_TYPE_ADD_USELESS_ENTRY_SUCCESS;
        filetrimreturn.ulTrimSecCnt = g_ulTrimCmdEntryIndex;
        return(filetrimreturn);
    }
    filetrimreturn.ulFileTrimType = FILE_TRIM_TYPE_ADD_USELESS_ENTRY_SUCCESS;
    filetrimreturn.ulTrimSecCnt = g_ulTrimCmdEntryIndex;
    return(filetrimreturn);
}

void sim_cmd_clear_trim_entry(U32 TrimSectorCnt)
{
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    U32 Trim_Cmd_entry = 0;
    U32 Trim_Lba_entry = 0;

    while(Trim_Cmd_entry <= TrimSectorCnt)
    {
        pTrimCmdEntry = &g_TrimCmdEntry[Trim_Cmd_entry];
        while(Trim_Lba_entry < TRIM_LBA_RANGE_ENTRY_MAX)
        {
            pTrimCmdEntry->LbaRangeEntry[Trim_Lba_entry].StartLbaLow = 0;
            pTrimCmdEntry->LbaRangeEntry[Trim_Lba_entry].StartLbaHigh = 0;
            pTrimCmdEntry->LbaRangeEntry[Trim_Lba_entry].RangeLength = 0;
            Trim_Lba_entry++;
        }
        Trim_Cmd_entry++;
    }
}


BOOL sim_send_trim_cmd(void)
{
    static U32 start_lba = 0;
    static U32 cmd_cnt = 0;
    static U32 SectorCount=0;
    static U32 ulLbaEntryIndex = 0;
    U32 LBAStart=0;
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    U32 sec_cnt = 0;
    U16 usCmdEntryIndex = 0;
    static BOOL bCmdRes = TRUE;
    static BOOL bEightSend = FALSE;
    HCMD_INFO HCmd;

    while (TRUE == bCmdRes)
    {
        if (ulLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX)
        {
            start_lba = GetRandomLBAPartern();
            sec_cnt = GetRandomSecCnt();

            if (0 == sec_cnt)
            {
                sec_cnt = 256;
            }
        }
        else
        {
            start_lba = 0;
            sec_cnt = 0;
        }

        if ((start_lba + sec_cnt) > g_ulMaxLBAInModel)
            start_lba = g_ulMaxLBAInModel - sec_cnt - 1;

#ifdef HOST_NVME
        //make LBA 4K align
        start_lba = (start_lba >> SEC_PER_DATABLOCK_BITS) << SEC_PER_DATABLOCK_BITS;
        sec_cnt = max(sec_cnt,SEC_PER_DATABLOCK);

        pTrimCmdEntry = &g_TrimCmdEntry[usCmdEntryIndex];
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow = start_lba >> SEC_PER_DATABLOCK_BITS;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaHigh = 0;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength = sec_cnt >> SEC_PER_DATABLOCK_BITS;
#else
        pTrimCmdEntry = &g_TrimCmdEntry[usCmdEntryIndex];
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow = start_lba;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaHigh = 0;
        pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength = sec_cnt;
#endif

        ulLbaEntryIndex++;

        if(ulLbaEntryIndex == TRIM_LBA_RANGE_ENTRY_MAX)
        {
            ulLbaEntryIndex = 0;
            usCmdEntryIndex++;

            if (bEightSend == TRUE)
            {
                SectorCount++;
                if (SectorCount ==8)
                {
                    bEightSend = FALSE;
                    break;
                }
            }
            else
            {
                SectorCount = 1;
                bEightSend = TRUE;
                break;
            }
        }
    }
    HCmd.CmdType = CMD_TYPE_TRIM;
    HCmd.StartLba = LBAStart;
    HCmd.SecCnt = SectorCount;
    HCmd.RangeNum = TRIM_LBA_RANGE_ENTRY_MAX*SectorCount - 1;

    bCmdRes = Host_SendOneCmd(&HCmd);

    if(bCmdRes == TRUE)
    {
        cmd_cnt ++;
        SectorCount = 0;
        if(0 == (cmd_cnt % 10))
        {
            DBG_Printf("\nSend trim cmd cnt %d!\n", cmd_cnt);
            cmd_cnt = 0;
            return TRUE;

        }
    }

    return FALSE;

}

void SectorRangeSet(pFUNCTION_PARAMETER pFuncParameter)
{
    if(pFuncParameter->ulSecRange != 0)
    {
        pFuncParameter->ulSecCnt  += pFuncParameter->ulSecRange;
        if(pFuncParameter->ulSecCnt > MAX_SECTOR_CNT)
        {
            pFuncParameter->ulSecCnt = 1;
        }
        if(pFuncParameter->ulStartLba >= (g_ulMaxLBAInModel - pFuncParameter->ulSecCnt))
        {
            pFuncParameter->ulStartLba = 0;
        }
    }
}

BOOL SeqR(pFUNCTION_PARAMETER pFuncParameter)
{
    HCMD_INFO HCmd;

    HCmd.CmdType = CMD_TYPE_READ;

    HCmd.StartLba = pFuncParameter->ulStartLba;
    HCmd.SecCnt = pFuncParameter->ulSecCnt;

    if( TRUE == Host_SendOneCmd(&HCmd))
    {
        pFuncParameter->ulStartLba = HCmd.StartLba +  HCmd.SecCnt;
        SectorRangeSet(pFuncParameter);
        /*if(pFuncParameter->ulStartLba >= (g_ulMaxLBAInModel - pFuncParameter->ulSecCnt))
        {
            pFuncParameter->ulStartLba = 0;
        }*/
        if(pFuncParameter->ulEndLba)
        {
            if(pFuncParameter->ulStartLba >= pFuncParameter->ulEndLba)
            {
                pFuncParameter->ulStartLba = g_CurSec->pCondHead->pActHead->m_BpParam.ulStartLba;
            }
        }
        return TRUE;
    }
    return FALSE;
}



BOOL SeqW(pFUNCTION_PARAMETER pFuncParameter)
{
    HCMD_INFO HCmd;


    HCmd.CmdType = CMD_TYPE_WRITE;

    HCmd.StartLba = pFuncParameter->ulStartLba;
    HCmd.SecCnt = pFuncParameter->ulSecCnt;

    if( TRUE == Host_SendOneCmd(&HCmd))
    {
        pFuncParameter->ulStartLba = HCmd.StartLba +  HCmd.SecCnt;
        SectorRangeSet(pFuncParameter);
        if(pFuncParameter->ulEndLba)
        {
            if(pFuncParameter->ulStartLba >= pFuncParameter->ulEndLba)
            {
                pFuncParameter->ulStartLba = g_CurSec->pCondHead->pActHead->m_BpParam.ulStartLba;
            }
        }
        return TRUE;
    }
    return FALSE;
}

BOOL RndW(pFUNCTION_PARAMETER pFuncParameter)
{
    U32 start_lba = 0;
    U16 sec_cnt;
    HCMD_INFO HCmd;

    sec_cnt = pFuncParameter->ulSecCnt;
    if(pFuncParameter->ulEndLba >= (g_ulMaxLBAInModel - sec_cnt))
    {
        pFuncParameter->ulEndLba = g_ulMaxLBAInModel - sec_cnt;
    }
    start_lba = GetRandomLBAPatternInRange(pFuncParameter->ulStartLba,pFuncParameter->ulEndLba);

    if((start_lba + sec_cnt) >= g_ulMaxLBAInModel)
    {
        start_lba = start_lba  % (g_ulMaxLBAInModel - sec_cnt);
    }

    HCmd.CmdType = CMD_TYPE_WRITE;
    HCmd.StartLba = start_lba;
    HCmd.SecCnt = sec_cnt;

    if(TRUE == Host_SendOneCmd(&HCmd))
    {
        SectorRangeSet(pFuncParameter);
        return TRUE;
    }
    return FALSE;

}

BOOL RndR(pFUNCTION_PARAMETER pFuncParameter)
{
    U32 start_lba = 0;
    U16 sec_cnt;
    HCMD_INFO HCmd;

    sec_cnt = pFuncParameter->ulSecCnt;
    if(pFuncParameter->ulEndLba >= (g_ulMaxLBAInModel - sec_cnt))
    {
        pFuncParameter->ulEndLba = g_ulMaxLBAInModel - sec_cnt;
    }
    start_lba = GetRandomLBAPatternInRange(pFuncParameter->ulStartLba,pFuncParameter->ulEndLba);

    if((start_lba + sec_cnt) >= g_ulMaxLBAInModel)
    {
        start_lba = start_lba  % (g_ulMaxLBAInModel - sec_cnt);
    }


    HCmd.CmdType = CMD_TYPE_READ;
    HCmd.StartLba = start_lba;
    HCmd.SecCnt = sec_cnt;

    if(TRUE == Host_SendOneCmd(&HCmd))
    {
        SectorRangeSet(pFuncParameter);
        return TRUE;
    }
    return FALSE;

}

BOOL do_one_file_pattern(char * pfile_name,BOOL* bCmdDone)
{

    float time1=0.0;
    float time2=0.0;
    U32 p=0;
    char op[20];
    U32  ncq_tag=0;

    U32 cmd_cnt = 0;
    U32 cmd_cnt_read = 0;
    U32 cmd_cnt_write = 0;
    static HCMD_INFO HCmd;


    U32 test=0;
    U32 i = 0;
    U32 j = 100;

    static BOOL bCmdRes = TRUE;
    static U8 cmd_code = 0; //ATA_CMD_READ_FPDMA_QUEUED;
    static U32 LBAStart=0;
    static U32 SectorCount=0;
    static U32 total_sectorCnt;
    static U32 index=0;

    static U32 slipe_cmd_flag = 0;
    static U32 LBAStart_slipe = 0;
    static U8 Seccnt_quotient = 0;
    static U8 Seccnt_remainder = 0;

    static U8 Send_cmd_count = 0;

    static FILE * p_file = NULL;

    if (NULL == p_file)
        p_file = fopen(pfile_name,"r+b");

    if(p_file == NULL)
    {
        DBG_Printf("PatternFile %s Open Fail !\n",pfile_name);
        return FALSE;
    }

   if (slipe_cmd_flag == 0)
    {



        while (TRUE == bCmdRes)
        {
            if(!feof(p_file))
            {
                if(!fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\n", &index, &time1, &time2, &p, op,&LBAStart,&SectorCount))
                {
                     fscanf(p_file,"%*[^\n]%*c");//Skip this line
                }
            }
            else
            {
                fseek(p_file, 0, SEEK_SET);
                fclose(p_file);     //close file
                p_file = NULL;
                SectorCount = 0;
                index = 0;
                return TRUE;
            }

            if(op[0]=='R')
            {
                HCmd.CmdType = CMD_TYPE_READ;
            }
            else if(op[0]=='W')
            {
                HCmd.CmdType = CMD_TYPE_WRITE;
            }
            else
            {
                continue;
            }

            if (SectorCount > 0x800)
            {
                SectorCount = 0x800;
            }

            if (SectorCount > MAX_ONE_CMD_SEC_CNT)
            {
                Seccnt_quotient = (U32)SectorCount / MAX_ONE_CMD_SEC_CNT;
                Seccnt_remainder = SectorCount % MAX_ONE_CMD_SEC_CNT;

                if (Seccnt_quotient > 16)
                {
                    DBG_Printf("Error sector count");
                    //DBG_Break();
                }

                /*if(Seccnt_remainder == 0)
                    Cmd_total_count = Seccnt_quotient;
                else
                    Cmd_total_count = Seccnt_quotient + 1;*/

                slipe_cmd_flag = 1;
            }

            if (0 == SectorCount)
            {
                SectorCount = 65536;
            }

            if((LBAStart + SectorCount) >= g_ulMaxLBAInModel)  //MAX_LBA_IN_DISK
            {
                LBAStart = LBAStart  % (g_ulMaxLBAInModel - SectorCount);
            }
            break;
        }
    }

    if (slipe_cmd_flag != 0)
    {
        if(Send_cmd_count == 0)
        {
            //HCmd.StartLba = LBAStart;
            LBAStart_slipe = LBAStart;
            //HCmd.SecCnt = SectorCount;
        }
        if(Seccnt_remainder == 0)
        {
            if(Send_cmd_count < Seccnt_quotient)
            {
                HCmd.SecCnt = MAX_ONE_CMD_SEC_CNT;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    total_sectorCnt += MAX_ONE_CMD_SEC_CNT;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    Send_cmd_count++;
                    LBAStart_slipe += MAX_ONE_CMD_SEC_CNT;
                }

            }
            else
            {
                slipe_cmd_flag = 0;
                Send_cmd_count = 0;
            }
        }
        else
        {
            if(Send_cmd_count < Seccnt_quotient)
            {
                HCmd.SecCnt = MAX_ONE_CMD_SEC_CNT;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    total_sectorCnt += MAX_ONE_CMD_SEC_CNT;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    Send_cmd_count++;
                    LBAStart_slipe += MAX_ONE_CMD_SEC_CNT;
                }

            }
            else
            {
                HCmd.SecCnt = Seccnt_remainder;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    total_sectorCnt += Seccnt_remainder;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    slipe_cmd_flag = 0;
                    Send_cmd_count = 0;
                }
            }

        }
    }
    else
    {

        HCmd.StartLba = LBAStart;
        HCmd.SecCnt = SectorCount;

        bCmdRes = Host_SendOneCmd(&HCmd);

        if (TRUE == bCmdRes)
        {
            total_sectorCnt += SectorCount;
            if(total_sectorCnt  > (i *10000))
            {
                i++;
            }
            *bCmdDone = TRUE;
        }
    }
    return FALSE;
}

BOOL do_one_file_trim_pattern(char * pfile_name,BOOL* bCmdDone)
{

    float time1=0.0;
    float time2=0.0;
    U32 p=0;
    static char op[20];
    U32  ncq_tag=0;

    U32 cmd_cnt = 0;
    U32 cmd_cnt_read = 0;
    U32 cmd_cnt_write = 0;
    static HCMD_INFO HCmd;

    U32 test=0;
    U32 i = 0;
    U32 j = 100;

    static BOOL bCmdRes = TRUE;
    static U8 cmd_code = 0; //ATA_CMD_READ_FPDMA_QUEUED;
    static U32 LBAStart=0;
    static U32 SectorCount=0;
    static U32 total_sectorCnt;
    static U32 total_trimCnt;
    static U32 index=0;

    static U32 slipe_cmd_flag = 0;
    static U32 LBAStart_slipe = 0;
    static U8 Seccnt_quotient = 0;
    static U8 Seccnt_remainder = 0;

    static U8 Send_cmd_count = 0;

    static U8 Trim_cmd_flag = 0;
    static U8 Trim_cmd_finish = 0;
    static FILE_TRIM_RETURN filetrimreturn;

    static FILE * p_file = NULL;

    if (NULL == p_file)
        p_file = fopen(pfile_name,"r+b");

    if(p_file == NULL)
    {
        DBG_Printf("PatternFile %s Open Fail !\n",pfile_name);
        return FALSE;
    }

    if (slipe_cmd_flag == 0)
    {

        while (TRUE == bCmdRes)
        {
            if(Trim_cmd_finish == 0)
            {
                if(!feof(p_file))
                {
                    if(!fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\n", &index, &time1, &time2, &p, op,&LBAStart,&SectorCount))
                    {
                         fscanf(p_file,"%*[^\n]%*c");//Skip this line
                    }
                }
                else
                {
                    fseek(p_file, 0, SEEK_SET);
                    fclose(p_file);     //close file
                    p_file = NULL;
                    SectorCount = 0;
                    index = 0;
                    return TRUE;
                }
            }
            else if(filetrimreturn.ulFileTrimType == FILE_TRIM_TYPE_2_SECTOR)
            {
                Trim_cmd_finish = 0;
                if(!feof(p_file))
                {
                    if(!fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\n", &index, &time1, &time2, &p, op,&LBAStart,&SectorCount))
                    {
                         fscanf(p_file,"%*[^\n]%*c");//Skip this line
                    }
                }
                else
                {
                    fseek(p_file, 0, SEEK_SET);
                    fclose(p_file);     //close file
                    p_file = NULL;
                    SectorCount = 0;
                    index = 0;
                    return TRUE;
                }
            }
            if(op[0]=='R' || op[0]=='r')
            {
                if(Trim_cmd_flag == 1 && g_ulDoFileTrimCmd == 0)
                {
                    g_ulDoFileTrimCmd = 1;
                    Trim_cmd_flag = 0;
                }
                HCmd.CmdType = CMD_TYPE_READ;
            }
            else if(op[0]=='W' || op[0]=='w')
            {
                if(Trim_cmd_flag == 1 && g_ulDoFileTrimCmd == 0)
                {
                    g_ulDoFileTrimCmd = 1;
                    Trim_cmd_flag = 0;
                }
                HCmd.CmdType = CMD_TYPE_WRITE;
            }
            else if(op[0]=='T' || op[0]=='t')
            {
                 HCmd.CmdType = CMD_TYPE_TRIM;
                 if (SectorCount > LBAStart)
                 {
                     SectorCount = SectorCount - LBAStart;
                 }
                 
                 if(SectorCount > 0xFFFF)
                 {
                     SectorCount = 0xFFFF;
                 }
                 Trim_cmd_flag = 1;
            }
            else
            {
                if(Trim_cmd_flag == 1 && g_ulDoFileTrimCmd == 0)
                {
                    g_ulDoFileTrimCmd = 1;
                    Trim_cmd_flag = 0;
                }
                continue;
            }

            if(Trim_cmd_flag == 0 && g_ulDoFileTrimCmd == 0)
            {
                if (SectorCount > 0x800)
                {
                    SectorCount = 0x800;
                }

                if (SectorCount > MAX_ONE_CMD_SEC_CNT)
                {
                    Seccnt_quotient = (U32)SectorCount / MAX_ONE_CMD_SEC_CNT;
                    Seccnt_remainder = SectorCount % MAX_ONE_CMD_SEC_CNT;

                    if (Seccnt_quotient > 16)
                    {
                        DBG_Printf("Error sector count");
                        //DBG_Break();
                    }
                    slipe_cmd_flag = 1;
                }

                if (0 == SectorCount)
                {
                    SectorCount = 65536;
                }

                if((LBAStart + SectorCount) >= g_ulMaxLBAInModel)  //MAX_LBA_IN_DISK
                {
                    LBAStart = LBAStart  % (g_ulMaxLBAInModel - SectorCount);
                }
                break;
            }
            else
            {
                if((LBAStart + SectorCount) >= g_ulMaxLBAInModel)  //MAX_LBA_IN_DISK
                {
                    LBAStart = LBAStart  % (g_ulMaxLBAInModel - SectorCount);
                }
                filetrimreturn = sim_cmd_into_trim_entry(LBAStart,(U16)SectorCount);
                if(filetrimreturn.ulFileTrimType == FILE_TRIM_TYPE_2_SECTOR)
                {
                    break;
                }
                else if(filetrimreturn.ulFileTrimType == FILE_TRIM_TYPE_ADD_LBA_ENTRY_SUCCESS)
                {
                    continue;
                }
                else if(filetrimreturn.ulFileTrimType == FILE_TRIM_TYPE_ADD_USELESS_ENTRY_SUCCESS)
                {
                    break;
                }
            }

        }
    }

    if (slipe_cmd_flag != 0 && Trim_cmd_flag ==0 && g_ulDoFileTrimCmd == 0)
    {
        if(Send_cmd_count == 0)
        {
            LBAStart_slipe = LBAStart;
        }
        if(Seccnt_remainder == 0)
        {
            if(Send_cmd_count < Seccnt_quotient)
            {
                HCmd.SecCnt = MAX_ONE_CMD_SEC_CNT;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    Trim_cmd_finish = 0;
                    total_sectorCnt += MAX_ONE_CMD_SEC_CNT;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    Send_cmd_count++;
                    LBAStart_slipe += MAX_ONE_CMD_SEC_CNT;
                }

            }
            else
            {
                slipe_cmd_flag = 0;
                Send_cmd_count = 0;
            }
        }
        else
        {
            if(Send_cmd_count < Seccnt_quotient)
            {
                HCmd.SecCnt = MAX_ONE_CMD_SEC_CNT;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    Trim_cmd_finish = 0;
                    total_sectorCnt += MAX_ONE_CMD_SEC_CNT;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    Send_cmd_count++;
                    LBAStart_slipe += MAX_ONE_CMD_SEC_CNT;
                }

            }
            else
            {
                HCmd.SecCnt = Seccnt_remainder;
                HCmd.StartLba = LBAStart_slipe;

                bCmdRes = Host_SendOneCmd(&HCmd);

                if (TRUE == bCmdRes)
                {
                    Trim_cmd_finish = 0;
                    total_sectorCnt += Seccnt_remainder;
                    if(total_sectorCnt  > (i *10000))
                    {
                        i++;
                    }
                    *bCmdDone = TRUE;
                    slipe_cmd_flag = 0;
                    Send_cmd_count = 0;
                }
            }

        }
    }
    if(slipe_cmd_flag == 0)
    {

        if(g_ulDoFileTrimCmd == 0)
        {
            HCmd.StartLba = LBAStart;
            HCmd.SecCnt = SectorCount;

            bCmdRes = Host_SendOneCmd(&HCmd);

            if (TRUE == bCmdRes)
            {
                Trim_cmd_finish = 0;
                total_sectorCnt += SectorCount;
                if(total_sectorCnt  > (i *10000))
                {
                    i++;
                }
                *bCmdDone = TRUE;
            }
        }
        else
        {
            if(filetrimreturn.ulFileTrimType == FILE_TRIM_TYPE_ADD_LBA_ENTRY_SUCCESS)
            {
                filetrimreturn = sim_cmd_into_trim_entry(LBAStart,(U16)SectorCount);
            }
            HCmd.CmdType = CMD_TYPE_TRIM;
            HCmd.StartLba = 0;
            HCmd.SecCnt = filetrimreturn.ulTrimSecCnt + 1;
            HCmd.RangeNum = TRIM_LBA_RANGE_ENTRY_MAX*(filetrimreturn.ulTrimSecCnt + 1) - 1;
            bCmdRes = Host_SendOneCmd(&HCmd);

            if(TRUE == bCmdRes)
            {
                Trim_cmd_finish = 1;
                g_ulDoFileTrimCmd = 0;
                g_ulTrimCmdEntryIndex = 0;
                g_ulTrimLbaEntryIndex = 0;
                sim_cmd_clear_trim_entry(filetrimreturn.ulTrimSecCnt);
                total_trimCnt++;
                DBG_Printf("send one trim cmd. total trim cmd: %d.\n",total_trimCnt);
                *bCmdDone = TRUE;
            }

        }
    }
    return FALSE;
}


void FileNameGetInit()
{
    char sFormatFileName[256];

    sprintf(sFormatFileName, "%s\\*.*", PATTERN_PATH);

    hPatternFind =  FindFirstFile(sFormatFileName, &PatternFindFileData);

    if(hPatternFind == INVALID_HANDLE_VALUE)
    {
        DBG_Printf("Pattern path check, find no file!\n");
        DBG_Getch();
    }
    bPatternFinish = FALSE;
}

void GetFileName(char *pFileName)
{
    while(1)
    {
        if(bPatternFinish == TRUE)
        {
            *pFileName = '!';
            break;
        }

        if(!(FILE_ATTRIBUTE_DIRECTORY & PatternFindFileData.dwFileAttributes))
        {
            sprintf(pFileName, "%s\\%s", PATTERN_PATH, PatternFindFileData.cFileName);
        }
        else
        {
            *pFileName = '@';
        }

        if(!FindNextFile(hPatternFind, &PatternFindFileData))
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {

                bPatternFinish = TRUE;
            }
            else
            {
                DBG_Printf("File Find Error!\n");
                DBG_Getch();
            }
        }

        if(*pFileName != '@')
            break;
    }

}

BOOL FilePattern(pFUNCTION_PARAMETER pFuncParameter)
{
    static U32 file_cnt = 0;
    static char file_name[256] = {0};
    static U8 state = 0;
    BOOL bCmdDone = FALSE;

    switch(state)
    {
    case 0:
        FileNameGetInit();
        state++;
        break;
    case 1:
        GetFileName(file_name);
        printf("%d File Name: %s\n",file_cnt,file_name);
        state++;
        break;
    case 2:
        if ((file_name[0] != '@') && (file_name[0] != '!'))
        {
            if(TRUE == do_one_file_trim_pattern(file_name,&bCmdDone))
            {
                state = 1;
                file_cnt++;
            }
        }
        else
        {
            state = 0;
        }
        break;
    default:
        break;
    }

    if(bCmdDone)
    {
        return TRUE;
    }
    return FALSE;
}

BOOL MixRW(pFUNCTION_PARAMETER pFuncParameter)
{
    U32 start_lba = 0;
    U16 sec_cnt;
    int rw = 0;
    HCMD_INFO HCmd;
    static BOOL bCmdRes = TRUE;

    sec_cnt = rand() % 65536 + 1;
    start_lba = rand() % g_ulMaxLBAInModel + 1;

    if((start_lba + sec_cnt) >= g_ulMaxLBAInModel)
    {
        start_lba = start_lba  % (g_ulMaxLBAInModel - sec_cnt);
    }

    rw = rand() % 2;

    if(rw == 0)
    {
        HCmd.CmdType = CMD_TYPE_READ;
    }
    else
    {
        HCmd.CmdType = CMD_TYPE_WRITE;
    }

    HCmd.StartLba = start_lba;
    HCmd.SecCnt = sec_cnt;

    if(TRUE == Host_SendOneCmd(&HCmd))
    {
        return TRUE;
    }
    return FALSE;
}



pBASE_FUNCTION GetBaseFunction(U16 uBPIndex)
{
    if (uBPIndex >= MAX_BASE_FUNCTION)
    {
        DBG_Printf("Function:GetBaseFunction Base Function Index:%d > %d \n", uBPIndex, MAX_BASE_FUNCTION);
        DBG_Getch();
    }

    return &l_BaseFuncArray[uBPIndex];
}

BOOL run_base_pattern(pBASE_FUNCTION pBaseFunction)
{
    BOOL bRet = FALSE;
    pFUNCTION_PARAMETER pFuncParamter = (pFUNCTION_PARAMETER)&pBaseFunction->FuncParameter;
    bRet = pBaseFunction->pFunc(pFuncParamter);

    return bRet;
}

void ini_base_fuction()
{
    l_BaseFuncArray[0].pFunc = &SeqR;
    l_BaseFuncArray[1].pFunc = &SeqW;
    l_BaseFuncArray[2].pFunc = &RndR;
    l_BaseFuncArray[3].pFunc = &RndW;
    l_BaseFuncArray[4].pFunc = &FilePattern;
    l_BaseFuncArray[5].pFunc = &MixRW;
}


void Host_SendRWCmdByType(HCMD_INFO* pHcmd)
{

    BOOL bRead = TRUE;

    if (CMD_TYPE_WRITE == pHcmd->CmdType || CMD_TYPE_READ == pHcmd->CmdType)
    {
        if (CMD_TYPE_WRITE == pHcmd->CmdType)
        {
            bRead = FALSE;
        }

#ifndef HOST_NVME
        if (HSRW_CMD_TYPE_NCQ == g_HSRWCmdType)
        {
            ATA_GetAtaHostCmdNCQH2DFis(pHcmd->HSCmd.RowCmd, pHcmd->StartLba, pHcmd->SecCnt, bRead);
        }
        else if (HSRW_CMD_TYPE_DMA == g_HSRWCmdType)
        {
            ATA_GetAtaHostCmdDMAH2DFis(pHcmd->HSCmd.RowCmd, pHcmd->StartLba, pHcmd->SecCnt, bRead);
        }
#endif
    }

}
