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
* File Name    : action_define.c
* Discription  :
* CreateAuthor :
* CreateDate   : 2014.6.26
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include <Windows.h>
#include <io.h>
#include "BaseDef.h"
#include "HostModel.h"
#include "HAL_TraceLog.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "Model_config.h"
#include "hscmd_parse.h"
#include "action_define.h"
#ifndef HOST_NVME
#include "SimATA_HSCMDCallBack.h"
#endif

#include "COM_Memory.h"
#ifdef SIM_XTENSA
#include "system_statistic.h"
#endif

#ifdef DBG_TABLE_REBUILD
#include "L2_SimTablecheck.h"
#include "L2_SimRebuildTable.h"
#endif

#ifdef HOST_NVME
void NVMe_SetShutdownReg(U32 ulType);
#endif

/* #define region: constant & MACRO defined here                              */
#ifdef SIM
//idle time
#define SIM_IDLE_TIME_MIN      2000//(1000)//ms
#define SIM_IDLE_TIME_MAX      3000 //(3000)//ms
#else //SIM_XTENSA
//idle cycle
#define SIM_IDLE_TIME_MIN   (500000)   //ms
#define SIM_IDLE_TIME_MAX   (600000)   //ms
#endif

GLOBAL StatisticDptr l_StatisticDptr;

/* extern region: extern global variable & function prototype                 */
extern void SIM_DevSetStatus(U32 ulStatus);
extern BOOL SIM_DevIsBootUp(void);
extern U8 SIM_DevGetStatus(void);
extern void NFC_InterfaceResetCQ(U8 pu, U8 level);
extern U32 SystemCaculateTimePass(U32 ulStartTime, U32 ulEndTime);
extern void Sim_ClearDram();
extern HSCMD_INFO* lookup_hscmdlist(U32 hscmd_id);
extern BOOL Host_IsCMDEmptyInterface(void);
extern void ResetTableRebuildFlag();
extern void SIM_SetReportWLstatistic();
extern BOOL SIM_CheckReportWLstatistic();

extern char l_strCheckListFolder[255];

extern U32 TestCaseSN;
extern BOOL volatile g_bReSetFlag;

U8 g_TableRebuildType;
BOOL g_bInjectError;

BOOL g_bWearLevelingStatistic = FALSE;
U32 g_ulDevVarTableAddr = 0;
U32 g_ulDevFlashRedAddr = 0;
U32 g_ulTLInfoAddr = 0;
U32 g_ulDevFlashDataAddr[MCU_MAX - 1][32] = {{0}};

/* local region:  declare local variable & local function prototype           */

/* main code region: function implement */

BOOL IsRealPowerDown()
{
    return TRUE;

#ifdef HOST_NVME
    return TRUE;
#endif
    if (0 != (TestCaseSN % 10))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Sim_SendStandby(void)
{
#ifdef HOST_NVME
    return TRUE;
#else
    BOOL ret = FALSE;
    HCMD_INFO HCmd;

    memset(&HCmd, 0, sizeof(HCMD_INFO));
    HCmd.CmdType = CMD_TYPE_STANDBY;
    HCmd.StartLba = 0;
    HCmd.SecCnt = 0;
    if (TRUE == Host_SendOneCmd(&HCmd))
    {
        ret = TRUE;
    }

    return ret;
#endif
}

BOOL Sim_NormalBootSchdedule(void)
{
    static U32 test_state = 0;
    static U32 ulNormalBootCnt = 0;
    BOOL ret = FALSE;
    HCMD_INFO HCmd;

    memset(&HCmd,0,sizeof(HCMD_INFO));
    HCmd.CmdType = CMD_TYPE_STANDBY;
    HCmd.StartLba = 0;
    HCmd.SecCnt = 0;

    switch(test_state)
    {
    case 0:
        if(TRUE == Host_SendOneCmd(&HCmd))
        {
            if (TRUE == IsRealPowerDown())
            {
                test_state++;
            }
            else
            {
                test_state = 2;
            }
            TestCaseSN++;
        }
        break;
        /* wait shutdown cmd to finish */
    case 1:
        if (Host_IsCMDEmpty() == TRUE)
        {
            SIM_DevSetStatus(HOST_STATUS_WAIT_L3_IDLE);

            DBG_Printf("############Normal power down Loop %d Start!############\n",TestCaseSN);
            test_state++;

        }
        break;
    case 2:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            test_state = 0;
            ret = TRUE;
        }
        break;
    default:
        break;
    }

    return ret;
}

BOOL Sim_AbnormalBoot_WaitL3Empty()
{
    static U32 test_state = 0;
    BOOL ret = FALSE;

#ifndef DBG_TABLE_REBUILD
    DBG_Printf("Run table rebuild pattern,should define DBG_TABLE_REBUILD & DBG_PMT\n");
    DBG_Getch();
#endif

    switch(test_state)
    {
    case 0:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            TestCaseSN++;
            DBG_Printf("############Table Rebuild Loop %d Start!############\n",TestCaseSN);
            g_TableRebuildType = TABLE_REBUILD_WAIT_L3_EMPTY;
        #ifdef HOST_NVME
            NVMe_SetShutdownReg(0x2);
        #endif
            SIM_DevSetStatus(HOST_STATUS_WAIT_L3_IDLE);
            test_state++;
        }
        break;

    case 1:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            test_state = 0;
            g_TableRebuildType = TABLE_REBUILD_NONE;
            ret = TRUE;
        }
        break;
    default:
        break;
    }
    return ret;
}

BOOL Sim_AbnormalBoot_NotWaitL3Empty()
{
    static U32 test_state = 0;
    BOOL ret = FALSE;

#ifndef DBG_TABLE_REBUILD
    DBG_Printf("Run table rebuild pattern,should define DBG_TABLE_REBUILD & DBG_PMT\n");
    DBG_Getch();
#endif

    switch(test_state)
    {
    case 0:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            TestCaseSN++;
            DBG_Printf("############Table Rebuild Not Wait L3 Empty Loop %d Start!############\n",TestCaseSN);
            g_TableRebuildType = TABLE_REBUILD_NOT_WAIT_L3_EMPTY;
            SIM_DevSetStatus(HOST_STATUS_DO_POWERDOWN);
            test_state++;
        }
        break;

    case 1:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            test_state = 0;
            g_TableRebuildType = TABLE_REBUILD_NONE;

            ret = TRUE;
        }
        break;
    default:
        break;
    }
    return ret;
}
BOOL Sim_Redo_LLF()
{
    static U32 test_state = 0;
    BOOL ret = FALSE;
    switch(test_state)
    {
    case 0:
        if (TRUE == Host_IsCMDEmptyInterface())
        {
            test_state++;
        }
        break;
    case 1:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            DBG_Printf("############Sim_Redo_LLF############\n");
            ResetTableRebuildFlag();
            SIM_DevSetStatus(HOST_STATUS_REDO_LLF);
            test_state++;
        }
        break;
    case 2:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            test_state = 0;
            ret = TRUE;
        }
        break;
    default:
        break;
    }
    return ret;
}

BOOL Sim_SystemIdle(void)
{
    BOOL ret = FALSE;
    static U32 ulRandomIdleTime;
#ifdef SIM_XTENSA
    static BOOL bFirstIn = TRUE;
    static U32 ulStartCycle = 0;
    U32 ulCurCycle;
    U32 ulPassedCycle;
#endif

#ifdef SIM
    ulRandomIdleTime = rand()% SIM_IDLE_TIME_MAX;
    if(ulRandomIdleTime < SIM_IDLE_TIME_MIN)
    {
        ulRandomIdleTime = SIM_IDLE_TIME_MIN;
    }

    if (HOST_STATUS_RUNNING != SIM_DevGetStatus())
    {
        return ret;
    }

    if(Host_IsCMDEmpty() == TRUE)
    {
        Sleep(ulRandomIdleTime);
        ret = TRUE;
    }
#else


    if(Host_IsCMDEmpty() == TRUE)
    {
        if(TRUE == bFirstIn)
        {
            ulRandomIdleTime = rand()% SIM_IDLE_TIME_MAX;
            if(ulRandomIdleTime < SIM_IDLE_TIME_MIN)
            {
                ulRandomIdleTime = SIM_IDLE_TIME_MIN;
            }
            ulStartCycle = (U32)GET_TIME();
            bFirstIn = FALSE;
        }
        else
        {
            ulCurCycle = (U32)GET_TIME();
            ulPassedCycle = SystemCaculateTimePass(ulStartCycle, ulCurCycle);
            if(ulPassedCycle >= ulRandomIdleTime)
            {
                bFirstIn = TRUE;
                ret = TRUE;
            }
        }
    }
#endif
    return ret;
}
BOOL Sim_Send_HSCMD(U32 HSCmdId)
{
    U32 start_lba = 0;
    U32 sec_cnt = 0;
    HCMD_INFO HCmd;

    HSCMD_INFO* pHSCmd = lookup_hscmdlist(HSCmdId);

    memcpy(&HCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));
    HCmd.CmdType = CMD_TYPE_HSCMD;
    HCmd.McuId = 0;

    if(TRUE == Host_SendOneCmd(&HCmd))
    {
        DBG_Printf("Send %s done\n",pHSCmd->CmdDsc);
        if(NULL == pHSCmd->pHSCmdCallBackFunc)
        {
            return TRUE;
        }
        else if (TRUE == ((*pHSCmd->pHSCmdCallBackFunc)((U8*)pHSCmd->pDataBuffer)))
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

U8 Sim_GetTableRebuildType(void)
{
    return g_TableRebuildType;
}

void SendGetTLInfoCmd(U8 *pTLInfoBuf)
{
    U8* pReadBuf;

    if(0 == g_ulTLInfoAddr)
    {
        DBG_Getch();
    }

    pReadBuf = SendReadMemoryCmd(g_ulTLInfoAddr,SEC_SIZE,MCU0_ID);
    memcpy(pTLInfoBuf,pReadBuf,SEC_SIZE);
}

BOOL parse_trace_log_info(TL_INFO *pMcuTLInfo, U16 usMaxReadSec, U16 *pReadSecCnt, U32 *pStartSec, U16 *pFirstReadSecCnt)
{
    U32 ulTlMemSecCnt;
    U16 usReadSecCnt;

    if (0x351400 != pMcuTLInfo->ulTlVersion)
    {
        return FALSE;
    }

    ulTlMemSecCnt = (pMcuTLInfo->ulTlMemSize >> 9);
    if (pMcuTLInfo->ulValidSecCnt > ulTlMemSecCnt)
    {
        printf("MCU TL_INFO error: TlMemSizeSec = 0x%x, ValidSecCnt = 0x%x\n", ulTlMemSecCnt, pMcuTLInfo->ulValidSecCnt);
        DBG_Getch();
    }

    if (pMcuTLInfo->ulValidSecCnt < pMcuTLInfo->ulWriteSec)
    {
        printf("MCU TL_INFO error: ulValidSecCnt = 0x%x, ulWriteSec = 0x%x\n", pMcuTLInfo->ulValidSecCnt, pMcuTLInfo->ulWriteSec);
        DBG_Getch();
    }

    /* calculate the sector count to read */
    if (usMaxReadSec < pMcuTLInfo->ulValidSecCnt)
    {
         usReadSecCnt = usMaxReadSec;
    }
    else
    {
         usReadSecCnt = pMcuTLInfo->ulValidSecCnt;
    }

    /* calculate the sector count and start sector firstly to read */
    if ((pMcuTLInfo->ulWriteSec + 1) >= usReadSecCnt)
    {
        *pFirstReadSecCnt = usReadSecCnt;
        *pStartSec = pMcuTLInfo->ulWriteSec + 1 - usReadSecCnt;
    }
    else
    {
        *pFirstReadSecCnt = usReadSecCnt - (pMcuTLInfo->ulWriteSec + 1);
        *pStartSec = ulTlMemSecCnt - *pFirstReadSecCnt;
    }

    *pReadSecCnt = usReadSecCnt;

    return TRUE;
}

void SendGetTLDataCmd(U32 ulBufAddr, U32 ulReadDevAddr, U16 usSecCnt, U8 ucMCUID)
{
    U8* pReadBuf = SendReadMemoryCmd(ulReadDevAddr,usSecCnt*SEC_SIZE,ucMCUID);
    memcpy((U8*)ulBufAddr,pReadBuf,usSecCnt*SEC_SIZE);

    return;
}

void SendInvalidTLData(U32 ulMcuId,U32 ulSecCnt)
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("InvalidReadTLData");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;
    tHCmd.SecCnt = ulSecCnt;
    tHCmd.McuId = ulMcuId;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }
    return;
}
void SendDisableTL()
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("DisableTL");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }
    return;
}

void SendEnableTL()
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("EnableTL");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }
    return;
}

U8* SendReadMemoryCmd(U32 ulReadDevAddr, U16 usLength, U8 ucMCUID)
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("VIA_ReadMemory");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    if(NULL == pHSCmd->pDataBuffer)
    {
        DBG_Getch();
    }

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;
    tHCmd.SecCnt = usLength;
    tHCmd.StartLba = ulReadDevAddr;
    tHCmd.McuId = ucMCUID;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }

    return (pHSCmd->pDataBuffer);
}

U8* SendWriteMemoryCmd(U32 ulWriteDevAddr, U16 usLength, U8 ucMCUID)
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("VIA_WriteMemory");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    if(NULL == pHSCmd->pDataBuffer)
    {
        DBG_Getch();
    }

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;
    tHCmd.SecCnt = usLength;
    tHCmd.StartLba = ulWriteDevAddr;
    tHCmd.McuId = ucMCUID;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }

    return (pHSCmd->pDataBuffer);
}

void GetMcuTraceLog(U8 ucMCUID, U32 ulSecCnt, TL_INFO *pTLInfo)
{
    FILE *fLogFile;
    U32 ulReadSecCnt;
    U32 ulRemainSecCnt;
    U32 ulTlMemSecCnt = (pTLInfo->ulTlMemSize >> SEC_SIZE_BITS);
    U32 ulStartReadSec;
    U32 ulHostBufSizeSec = 64;//32K
    U16 usCurSecCnt;
    U32 ulCurReadSec;
    U32 ulReadDevAddr;
    char aFileName[128];
    U8 *pHostReadBuf = NULL;

    /* check TL Info */
    if (0x351400 != pTLInfo->ulTlVersion)
    {
        printf("MCU %d doesn't enable trace log\n", ucMCUID);
        return;
    }

    /* get read length */
    if ((0 == ulSecCnt) || (ulSecCnt > pTLInfo->ulValidSecCnt))
    {
        ulReadSecCnt = pTLInfo->ulValidSecCnt;
    }
    else
    {
         ulReadSecCnt = ulSecCnt;
    }

    /* get read start sector location */
    ulStartReadSec = pTLInfo->ulReadSec;

    printf("MCU %d: TlMemBase 0x%x,TlMemSize 0x%x,ValidSecCnt 0x%x,ReadSec 0x%x\n",
        ucMCUID - 1, pTLInfo->ulTlMemBase, pTLInfo->ulTlMemSize, pTLInfo->ulValidSecCnt, pTLInfo->ulReadSec);
    printf("  StartSecID 0x%x, ReadSecCnt 0x%x\n", ulStartReadSec, ulReadSecCnt);

    /* open file */
    memset(aFileName, 0, 128);
    sprintf(aFileName, "output\\MCU%dTL", ucMCUID - 1);
    fLogFile = fopen(aFileName, "w");
    if(NULL == fLogFile)
    {
        printf("Can't open file %s with w", aFileName);
        system("pause");
        return;
    }
    fclose(fLogFile);
    fLogFile = fopen(aFileName, "ab+");
    if(NULL == fLogFile)
    {
        printf("Can't open file %s with ab+\n", aFileName);
        system("pause");
        return;
    }

    /* read TL data from device */
    pHostReadBuf = (U8 *)malloc(ulHostBufSizeSec << SEC_SIZE_BITS); //60K
    memset(pHostReadBuf,0,(ulHostBufSizeSec << SEC_SIZE_BITS));

    ulRemainSecCnt = ulReadSecCnt;
    ulCurReadSec = ulStartReadSec;
    while (ulRemainSecCnt > 0)
    {
        ulReadDevAddr = pTLInfo->ulTlMemBase + ((ulCurReadSec % ulTlMemSecCnt) << SEC_SIZE_BITS);

        usCurSecCnt = (ulRemainSecCnt < ulHostBufSizeSec) ? ulRemainSecCnt : ulHostBufSizeSec;
        if (((ulCurReadSec + 1) <= ulTlMemSecCnt)
            && ((ulCurReadSec + usCurSecCnt + 1) > ulTlMemSecCnt))
        {
            usCurSecCnt = ulTlMemSecCnt - ulCurReadSec;
        }

        SendGetTLDataCmd((U32)pHostReadBuf, ulReadDevAddr, usCurSecCnt, ucMCUID);
        SendInvalidTLData(ucMCUID,usCurSecCnt);
        fwrite(pHostReadBuf, SEC_SIZE, usCurSecCnt, fLogFile);

        ulCurReadSec += usCurSecCnt;
        ulRemainSecCnt -= usCurSecCnt;
        printf("  ReadDevAddr 0x%x, read Sector 0x%x, ulRemainSecCnt 0x%x\n",
            ulReadDevAddr,usCurSecCnt,ulRemainSecCnt);
    }

    fclose(fLogFile);
    free(pHostReadBuf);
    pHostReadBuf = NULL;

    return;
}

BOOL Sim_LoadTraceLog(void)
{
    U8 ucMCUID = 3;
    U32 ulReadLogSec = 0;
    TL_INFO *pTLInfoBuf = NULL;

    pTLInfoBuf = (TL_INFO *)malloc(SEC_SIZE);

    SendDisableTL();

    SendGetTLInfoCmd((U8 *)pTLInfoBuf);

    if ((ucMCUID >= 0) && (ucMCUID < 3))
    {
        GetMcuTraceLog(ucMCUID, ulReadLogSec, &(pTLInfoBuf[ucMCUID]));
    }
    else if (3 == ucMCUID)
    {
        GetMcuTraceLog(MCU0_ID, ulReadLogSec, &(pTLInfoBuf[0]));
        GetMcuTraceLog(MCU1_ID, ulReadLogSec, &(pTLInfoBuf[1]));
        GetMcuTraceLog(MCU2_ID, ulReadLogSec, &(pTLInfoBuf[2]));
    }
    else
    {
        printf("Sim_LoadTraceLog: Input wrong MCUID!\n");
    }

    free(pTLInfoBuf);
    pTLInfoBuf = NULL;

    SendEnableTL();

    return TRUE;
}
BOOL CompareReadData(U8* pWriteBuf,U8* pReadBuf,U32 ulBytes)
{
    U32 i;
    for(i = 0; i < ulBytes; i++)
    {
        if(*pWriteBuf != *pReadBuf)
        {
            DBG_Printf("CompareReadData Error!\n");
            DBG_Getch();
        }
    }
    return TRUE;
}
U32 GetDevFlashDataAddr(U32 ulMcuId,U8 ucPuNum)
{
    return g_ulDevFlashDataAddr[ulMcuId - 2][ucPuNum];
}

BOOL Sim_FlashHandle(void)
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;
    static U8* pWriteBuf;
    static U8* pReadBuf;

    static U8 State = 0;
    static U8 ucPuMsk = 0;
    U16 usBlk = 0x10;
    U16 usPage = 0x3;
    U16 usPln = 0;
    static U16 usPlnMode = 0;
    BOOL bRet = FALSE;
    U8 ucPuNum = 0;

    switch(State)
    {
    case 0:
        if(0 == GetDevFlashDataAddr(MCU2_ID,ucPuNum))
        {
            DBG_Getch();
        }
        pWriteBuf = SendWriteMemoryCmd(GetDevFlashDataAddr(MCU2_ID,ucPuNum),64*SEC_SIZE,MCU2_ID);
        State++;
        break;
    case 1:
        ucPuMsk |= (1 << ucPuNum);

        ucHSCmdId = get_hscmd_id("VIA_FlashWrite");
        pHSCmd = lookup_hscmdlist(ucHSCmdId);

        memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));
        tHCmd.CmdType = CMD_TYPE_HSCMD;
        tHCmd.McuId = MCU2_ID;
        tHCmd.StartLba = ucPuMsk;
        tHCmd.SecCnt = (usBlk) | (usPage << 16) | (usPln << 25) | (usPlnMode << 29);
        while (FALSE == Host_SendOneCmd(&tHCmd));
        State++;
        break;
    case 2:
        ucHSCmdId = get_hscmd_id("VIA_FlashRead");
        pHSCmd = lookup_hscmdlist(ucHSCmdId);

        memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));
        tHCmd.CmdType = CMD_TYPE_HSCMD;
        tHCmd.McuId = MCU2_ID;
        tHCmd.StartLba = ucPuMsk;
        tHCmd.SecCnt = (usBlk) | (usPage << 16) | (usPln << 25) | (usPlnMode << 29);
        while (FALSE == Host_SendOneCmd(&tHCmd));
        State++;
        break;
    case 3:
        pReadBuf = SendReadMemoryCmd(GetDevFlashDataAddr(MCU2_ID,ucPuNum),64*SEC_SIZE,MCU2_ID);
        if(TRUE == CompareReadData(pWriteBuf,pReadBuf,16*4))
        {
            DBG_Printf("MCU %d read flash Pu %d Blk 0x%x Pg 0x%x(PlnMode %d) is right.\n",MCU2_ID,ucPuNum,usBlk,usPage,usPlnMode);
        }
        State++;
        break;
    case 4:
        ucHSCmdId = get_hscmd_id("VIA_FlashErase");
        pHSCmd = lookup_hscmdlist(ucHSCmdId);

        memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));
        tHCmd.CmdType = CMD_TYPE_HSCMD;
        tHCmd.McuId = MCU2_ID;
        tHCmd.StartLba = ucPuMsk;
        tHCmd.SecCnt = (usBlk) | (usPage << 16) | (usPln << 25) | (usPlnMode << 29);
        while (FALSE == Host_SendOneCmd(&tHCmd));
        State = 0;
        //change PlnMode and Pu
        usPlnMode ^= 1;
        ucPuMsk = 0x2;
        if(usPlnMode == 0)
        {
            bRet = TRUE;
        }
        break;
    default:
        DBG_Getch();
        break;
    }

    return bRet;
}

LOCAL HANDLE l_hTraceFile;
void OpenTraceFile(U32 ulMucId)
{
    char fileName[128]="";
    memset(fileName,0,sizeof(fileName));
    sprintf(fileName,"MCU%d_Trace.bin",ulMucId-1);

    l_hTraceFile = CreateFile(fileName,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(l_hTraceFile == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "fileName", GetLastError());
        DBG_Getch();
    }
}

void SaveTraceToFile(U8* pBuffer,U32 ulBytes)
{
    U32 ulWriteBytes;
    WriteFile(l_hTraceFile,(LPVOID)(pBuffer),ulBytes, (LPDWORD)&ulWriteBytes, NULL);
}

void CloseTraceFile()
{
    CloseHandle(l_hTraceFile);
}

BOOL Sim_LoadTraceFromFlash(void)
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;
    static U32 State = 0;
    static U8 ucPuMsk = 0x0;
    U16 usBlk = 0xd; //TraceBlock 0xd,0xe
    U16 usPage = 0x0;
    U16 usPln = 0;
    static U16 usPlnMode = 0;
    BOOL bRet = FALSE;
    static U8 ucPuNum;
    static U8* pWriteBuf;
    static U8* pReadBuf;
    static U32 ulMcuId = MCU1_ID;

    switch(State)
    {
    case 0:
        OpenTraceFile(ulMcuId);
        State++;
    case 1:
        if(0 == GetDevFlashDataAddr(ulMcuId,ucPuNum))
        {
            DBG_Getch();
        }
        ucPuMsk |= (1 << ucPuNum);

        ucHSCmdId = get_hscmd_id("VIA_FlashRead");
        pHSCmd = lookup_hscmdlist(ucHSCmdId);

        memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));
        tHCmd.CmdType = CMD_TYPE_HSCMD;
        tHCmd.McuId = ulMcuId;
        tHCmd.StartLba = ucPuMsk;
        tHCmd.SecCnt = (usBlk) | (usPage << 16) | (usPln << 25) | (usPlnMode << 29);
        while (FALSE == Host_SendOneCmd(&tHCmd));
        State++;
        break;
    case 2:
        pReadBuf = SendReadMemoryCmd(GetDevFlashDataAddr(ulMcuId,ucPuNum),64*SEC_SIZE,ulMcuId);

        /* save trace to file */
        SaveTraceToFile(pReadBuf,64*SEC_SIZE);
        DBG_Printf("MCU %d read flash Pu %d Blk 0x%x Pg 0x%x(PlnMode %d) is right.\n",ulMcuId,ucPuNum,usBlk,usPage,usPlnMode);
        if(ulMcuId == MCU1_ID)
        {
            CloseTraceFile();
            ulMcuId = MCU2_ID;
            State = 0;
        }
        else
        {
            State = 3;
        }
        break;
    case 3:
        if(ulMcuId == MCU2_ID)
        {
            CloseTraceFile();
            bRet = TRUE;
        }
        break;
    default:
        DBG_Getch();
        break;
    }

    return bRet;
}

#ifndef MIX_VECTOR
#include "L0_ViaCmd.h"
BOOL Sim_GetVarTableData()
{
    U32 ulMcuId = MCU1_ID;
    VAR_TABLE* pVarTable;
    U32 i;

    if(0 == g_ulDevVarTableAddr)
    {
        DBG_Printf("g_ulDevVarTableAddr 0x%x,should send VIA_GetVarTableAddr before!\n",g_ulDevVarTableAddr);
        DBG_Getch();
    }

    for(i = 0; i < MCU_MAX - 1; i++)
    {
        pVarTable = (VAR_TABLE*)SendReadMemoryCmd(g_ulDevVarTableAddr,sizeof(VAR_TABLE),ulMcuId);

        g_ulDevFlashRedAddr = pVarTable->aSubSysTable[ulMcuId - 2].ulDevFlashRedAddr;

        memcpy(&g_ulDevFlashDataAddr[ulMcuId - 2][0],pVarTable->aSubSysTable[ulMcuId - 2].ulDevFlashDataAddr,32*sizeof(U32));

        g_ulTLInfoAddr = pVarTable->tVarHeadTable.tL0Table.ulTLInfoAddr;

        ulMcuId = MCU2_ID;
    }

    return TRUE;
}
#endif

#define    FW_IMAGE_SIZE    (768*1024)
#define    FW_IMAGE_RELATIVE_PATH   "special_cmd_list\\FW_Image.bin"

BOOL Sim_FirmwareImageDL()
{
    U32 i;
    U32 ulLen,ulOffset = 0;

    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    FILE *pFWImg;
    char strFWImgPath[255] = { 0 };
    U32 uFileSeeker = 0;
    U32 uFileCnt;

    memset(strFWImgPath, 0x00, sizeof(strFWImgPath));
    sprintf(strFWImgPath, "%s\\%s", l_strCheckListFolder, FW_IMAGE_RELATIVE_PATH);

    /*FileOpen(FWImg.bin)*/
    pFWImg = fopen(strFWImgPath, "rb");
    if (NULL == pFWImg)
    {
        DBG_Printf("OpenFile:FWImage.bin faile\r\n");
    }

    if (0 != fseek(pFWImg, 0, SEEK_SET))
    {
        DBG_Printf("Seek error\r\n");
    }

    ulLen = (32*1024)/sizeof(U32);

    for(i = 0; i< FW_IMAGE_SIZE/(32*1024); i++)
    {
        ucHSCmdId = get_hscmd_id("FirmwareImageDownload");
        pHSCmd = lookup_hscmdlist(ucHSCmdId);

        uFileCnt = (U32)fread(pHSCmd->pDataBuffer, sizeof(U32), ulLen, pFWImg);
        if (uFileCnt != ulLen)
        {
            DBG_Printf("Cannot Read 32K in FWImg.bin one time, EOF?\r\n");
        }

        uFileSeeker += ulLen;
        if (0 != fseek(pFWImg, uFileSeeker << 2, SEEK_SET)) //FAILE
        {
            DBG_Printf("Seek error\r\n");
        }


        if(NULL == pHSCmd->pDataBuffer)
        {
            DBG_Getch();
        }

        *((U32*)pHSCmd->RowCmd + 10) = ulLen - 1; //CDW10
        *((U32*)pHSCmd->RowCmd + 11) = ulOffset;  //CDW11

        memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

        tHCmd.CmdType = CMD_TYPE_HSCMD;

        while (FALSE == Host_SendOneCmd(&tHCmd))
        {
        }
        ulOffset += ulLen;
    }

    if (0 != fclose(pFWImg))//FAILE
    {
        DBG_Printf("Close File FWImg error.\r\n");
    }

    return TRUE;
}
BOOL Sim_SendFWActive()
{
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("FirmwareActive");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    memcpy(&tHCmd.HSCmd,pHSCmd,sizeof(HSCMD_INFO));

    tHCmd.CmdType = CMD_TYPE_HSCMD;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }

    return TRUE;
}

BOOL Sim_FirmwareUpdate()
{
    static U32 test_state = 0;
    BOOL ret = FALSE;

    switch(test_state)
    {
    case 0:
        if (Host_IsCMDEmpty() == TRUE)
        {
            test_state++;
        }
        break;
    case 1:
        if(TRUE == Sim_FirmwareImageDL())
        {
            test_state++;
        }
        break;
    case 2:
        if (TRUE == Sim_SendFWActive())
        {
            test_state = 0;
            ret = TRUE;
        }
        break;
    default:
        break;
    }

    return ret;
}

BOOL Sim_WearLevelingStatus(void)
{
#ifdef SIM

    static U32 test_state = 0;
    BOOL ret = FALSE;
    switch(test_state)
    {
    case 0:
        if (HOST_STATUS_RUNNING == SIM_DevGetStatus())
        {
            DBG_Printf("############Sim_Report_Wear_Leveling_Statistic############\n");
            SIM_SetReportWLstatistic();
            Dbg_ReportStatistic();
            test_state++;
        }
        break;
    case 1:
        if (TRUE == SIM_CheckReportWLstatistic())
        {
            test_state = 0;
            ret = TRUE;
        }
        break;
    default:
        break;
    }
    return ret;


#endif // SIM

}

BOOL Sim_SecurityStatusCheck(U8 ucSecurityStatus)
{
#ifndef HOST_NVME
    U32 start_lba = 0;
    U32 sec_cnt = 0;
    U8 ucHSCmdId;
    HCMD_INFO HCmd;
    HSCMD_INFO* pHSCmd;
    BOOL bStatus = FALSE;
    PIDENTIFY_DEVICE_DATA pIdentifyData;

    ucHSCmdId = get_hscmd_id("IdentifyDevice");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);

    memcpy(&HCmd.HSCmd, pHSCmd, sizeof(HSCMD_INFO));
    HCmd.CmdType = CMD_TYPE_HSCMD;
    HCmd.McuId = 0;
    pIdentifyData = (PIDENTIFY_DEVICE_DATA)pHSCmd->pDataBuffer;

    if (TRUE == Host_SendOneCmd(&HCmd))
    {
        DBG_Printf("Send %s done\n", pHSCmd->CmdDsc);
        if (NULL == pHSCmd->pHSCmdCallBackFunc)
        {
            return TRUE;
        }
        else if (TRUE == ((*pHSCmd->pHSCmdCallBackFunc)((U8*)pHSCmd->pDataBuffer)))
        {
            if (TRUE != pIdentifyData->SecurityStatus.SecuritySupported)
            {
                DBG_Printf("No support security feature\n");
                DBG_Getch();
            }

            switch (ucSecurityStatus)
            {
                case ACT_TYPE_SECURITY_LOCK_CHECK:
                    if ((TRUE == pIdentifyData->SecurityStatus.SecurityEnabled) && (TRUE == pIdentifyData->SecurityStatus.SecurityLocked))
                    {
                        bStatus = TRUE;
                    }
                    break;
                case ACT_TYPE_SECURITY_ULOCK_CHECK:
                    if ((FALSE == pIdentifyData->SecurityStatus.SecurityEnabled) &&
                        (FALSE == pIdentifyData->SecurityStatus.SecurityLocked)  &&
                        (FALSE == pIdentifyData->SecurityStatus.SecurityFrozen))
                    {
                        bStatus = TRUE;
                    }
                    break;
                case ACT_TYPE_SECURITY_FREEZE_CHECK:
                    if (TRUE == pIdentifyData->SecurityStatus.SecurityFrozen)
                    {
                        bStatus = TRUE;
                    }
                    break;
                case ACT_TYPE_SECURITY_ENABLE_CHECK:
                    if (TRUE == pIdentifyData->SecurityStatus.SecurityEnabled)
                    {
                        bStatus = TRUE;
                    }
                    break;
                default:
                    break;
            }

            if (FALSE == bStatus)
            {
                DBG_Printf("Error Security Status:0x%x\n", pIdentifyData->SecurityStatus);
                DBG_Getch();
            }

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
#else
    return FALSE;
#endif
}

typedef struct _WZ_CMD_PARAM{
    U32 ulStartLbaL;
    U32 ulStartLbaH;
    U32 ulBlockNum;
}WZ_CMD_PARAM, *PWZ_CMD_PARAM;

BOOL Sim_NVMeWriteZeroSendWZ(PWZ_CMD_PARAM pWzCmdParam)
{
    BOOL bFinish = FALSE;
    HCMD_INFO tHCmd;
    U8 ucHSCmdId;
    HSCMD_INFO* pHSCmd;

    ucHSCmdId = get_hscmd_id("WriteZero");
    pHSCmd = lookup_hscmdlist(ucHSCmdId);
    if ((ucHSCmdId == INVALID_8F) || (pHSCmd == NULL))
    {
        DBG_Printf("Can not found WriteZero hscmd_id or pHSCmd of WriteZero is NULL\n");
        DBG_Getch();
    }

    *((U32*)pHSCmd->RowCmd + 10) = pWzCmdParam->ulStartLbaL; //CDW10,start LBAL
    *((U32*)pHSCmd->RowCmd + 11) = pWzCmdParam->ulStartLbaH; //CDW11,start LBAH
    *((U32*)pHSCmd->RowCmd + 12) = (pWzCmdParam->ulBlockNum - 1) & 0xFFFF;//CDW12

    memcpy(&tHCmd.HSCmd, pHSCmd, sizeof(HSCMD_INFO));
    tHCmd.CmdType = CMD_TYPE_HSCMD;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }
    return TRUE;
}

BOOL Sim_NVMeWriteZeroSendRD(PWZ_CMD_PARAM pWzCmdParam, U8* pDataBufFix)
{
    HCMD_INFO tHCmd;

    ASSERT(pDataBufFix != NULL);

    tHCmd.CmdType = CMD_TYPE_READ_IO;
    tHCmd.StartLba = pWzCmdParam->ulStartLbaL;
    if (pWzCmdParam->ulStartLbaH != 0)
    {
        DBG_Printf("Warning: LBAHigh is unsupported in HCmd type\n");
    }
    tHCmd.SecCnt = pWzCmdParam->ulBlockNum;
    tHCmd.HSCmd.pDataBuffer = pDataBufFix;
    tHCmd.HSCmd.DataSecCnt = pWzCmdParam->ulBlockNum;

    while (FALSE == Host_SendOneCmd(&tHCmd))
    {
    }

    return TRUE;
}

BOOL Sim_NVMeWriteZeroChkData(PWZ_CMD_PARAM pWzCmdParam, U8* pDataBufFix)
{
    U32 i;
    BOOL bFinish = TRUE;

    ASSERT(pDataBufFix != NULL);
    U8 *pBuffer = pDataBufFix;
    U32 ulSecCnt = pWzCmdParam->ulBlockNum;

    DBG_Printf("WriteZero Zero Check:\n");
    //Since we only store the first 8 byte of one sector, Just check those 8 bytes of each sector.
    for (i = 0; i < ulSecCnt; i++)
    {
        if (*(ULONGLONG*)pBuffer != 0)
        {
            printf("0x%llx is not Zero. Value(8Byte):0x%llx\n", (ULONGLONG)pBuffer, *(ULONGLONG*)pBuffer);
            bFinish = FALSE;
        }
       pBuffer += SEC_SIZE;
    }

    if (FALSE == bFinish)
    {
        DBG_Getch();
    }
    else
    {
        DBG_Printf("WriteZero Check Pass\n");
    }
    return bFinish;
}

BOOL Sim_NVMeWriteZero(void)
{
    typedef enum NVMeWriteZeroTestStatus{
        WRITE_ZERO_PREPARING,
        WRITE_ZERO_SEND_WZ_CMD,
        WRITE_ZERO_SEND_RD_CMD,
        WRITE_ZERO_CHECK,
        WRITE_ZERO_COMPLETED
    };
    WZ_CMD_PARAM tWzCmdParam = { 0, 0, 64 };
    LOCAL U32 t_ulTestStatus = WRITE_ZERO_PREPARING;
    BOOL bFinish = FALSE;
    LOCAL U8* pDataBuf = NULL;
    LOCAL U8* pDataBufFix = NULL;


    switch(t_ulTestStatus)
    {
    case WRITE_ZERO_PREPARING:
        if (FALSE == Host_IsCMDEmpty())
        {
            break;
        }
        ASSERT(pDataBuf == NULL);
        pDataBuf = (U8*)malloc(tWzCmdParam.ulBlockNum * SEC_SIZE + HPAGE_SIZE*2 - 1);
        pDataBufFix = (U8*)(((ULONGLONG)(pDataBuf + HPAGE_SIZE)) & (~HPAGE_SIZE_MSK));
        if (pDataBuf == NULL)
        {
            t_ulTestStatus = WRITE_ZERO_COMPLETED;
            DBG_Printf("Cannot Malloc WriteZero CMD Data Buffer. Ignore CMD.\n");
            break;
        }
        else
        {
            printf("pDataAddr:0x%llx\n", (ULONGLONG)pDataBuf);
        }
        
        t_ulTestStatus = WRITE_ZERO_SEND_WZ_CMD;
        break;
        
    case WRITE_ZERO_SEND_WZ_CMD:
        if (TRUE == Sim_NVMeWriteZeroSendWZ(&tWzCmdParam))
        {
            t_ulTestStatus = WRITE_ZERO_SEND_RD_CMD;  
        }
        
    case WRITE_ZERO_SEND_RD_CMD:
        if (TRUE == Host_IsCMDEmpty())
        {
            if (TRUE == Sim_NVMeWriteZeroSendRD(&tWzCmdParam, pDataBufFix))
            {
                t_ulTestStatus = WRITE_ZERO_CHECK;
            }
        }
        break;

    case WRITE_ZERO_CHECK:
        if (TRUE == Host_IsCMDEmpty())
        {
            (VOID)Sim_NVMeWriteZeroChkData(&tWzCmdParam, pDataBufFix);
            t_ulTestStatus = WRITE_ZERO_COMPLETED;
        }
        break;
        
    case WRITE_ZERO_COMPLETED:
        if (pDataBuf != NULL)
        {
            free(pDataBuf);
            pDataBuf = NULL;
        }
        t_ulTestStatus = WRITE_ZERO_PREPARING;
        bFinish = TRUE;
    default:
        break;
    }

    return bFinish;
}

void DBG_ResetCounter()
{
    l_StatisticDptr.m_HostWriteSecCnt = 0;
    l_StatisticDptr.m_HostWritePgCnt = 0;
    l_StatisticDptr.m_DeviceWritePgCnt = 0;
}

void Dbg_InitStatistic()
{
    l_StatisticDptr.m_GlobalSN = 0;
    DBG_ResetCounter();
}

void Dbg_IncGlobalSN(int Cnt)
{
    l_StatisticDptr.m_GlobalSN += Cnt;

}

void Dbg_IncHostWriteCnt(int SecCnt)
{
    l_StatisticDptr.m_HostWriteSecCnt += SecCnt;
    if (l_StatisticDptr.m_HostWriteSecCnt >= (SEC_PER_LOGIC_PG))
    {
        l_StatisticDptr.m_HostWritePgCnt += (unsigned __int64)(l_StatisticDptr.m_HostWriteSecCnt / (unsigned __int64)(SEC_PER_LOGIC_PG));
        l_StatisticDptr.m_HostWriteSecCnt = l_StatisticDptr.m_HostWriteSecCnt % (unsigned __int64)(SEC_PER_LOGIC_PG);
    }
}

void Dbg_IncDevWriteCnt(int PageCnt)
{
    if (TRUE == g_bWearLevelingStatistic)
    {
        l_StatisticDptr.m_DeviceWritePgCnt += PageCnt;
        if (l_StatisticDptr.m_DeviceWritePgCnt >= 18446744073709551615)
        {
            DBG_ResetCounter();
        }
    }

}

void Dbg_IncDevTableWriteCnt(int PageCnt)
{
    if (TRUE == g_bWearLevelingStatistic)
    {
        l_StatisticDptr.m_DevWTablePgCnt += PageCnt;
        Dbg_IncDevWriteCnt(PageCnt);
    }
}

void Dbg_IncDevDataWriteCnt(int PageCnt)
{
    if (TRUE == g_bWearLevelingStatistic)
    {
        l_StatisticDptr.m_DevWDataPgCnt += PageCnt;
        Dbg_IncDevWriteCnt(PageCnt);
    }
}

void Dbg_IncDevTableReadCnt(int PageCnt)
{
    if (TRUE == g_bWearLevelingStatistic)
    {
        l_StatisticDptr.m_DevRTablePgCnt += PageCnt;
    }
}

void Dbg_IncDevDataReadCnt(int PageCnt)
{
    if (TRUE == g_bWearLevelingStatistic)
    {
        l_StatisticDptr.m_DevRDataPgCnt += PageCnt;
    }
}

void Dbg_ReportStatistic()
{
    FILE* pFile;
    SYSTEMTIME sys_time;
    char filename[256];

    if(_access(".\\WLstatistic", 0)!=0) system("md .\\WLstatistic");

    GetLocalTime( &sys_time );
    sprintf(filename,".\\WLstatistic\\statistic_%d_%d_%d_%d_%d_%d.txt",sys_time.wYear,sys_time.wMonth,sys_time.wDay,sys_time.wHour,sys_time.wMinute,sys_time.wSecond);


    pFile = fopen(filename, "w");
    if(pFile == NULL)
        return;

    fprintf(pFile, "TableWritePg:%I64u, \t DataWritePg:%I64u, \t TableReadPg:%I64u \t, DataReadPg:%I64u\t",
        l_StatisticDptr.m_DevWTablePgCnt, l_StatisticDptr.m_DevWDataPgCnt, l_StatisticDptr.m_DevRTablePgCnt, l_StatisticDptr.m_DevRDataPgCnt);
    fprintf(pFile, "HostWritePg:\t");
    fprintf(pFile, "%I64u\t", l_StatisticDptr.m_HostWritePgCnt);
    fprintf(pFile, "DeviceWritePg:\t");
    fprintf(pFile, "%I64u\t", l_StatisticDptr.m_DeviceWritePgCnt);
    fprintf(pFile, "WA:\t");
    fprintf(pFile, "%f\t", (float)l_StatisticDptr.m_DeviceWritePgCnt / l_StatisticDptr.m_HostWritePgCnt);

    fprintf(pFile, "\n");

    fclose(pFile);
}