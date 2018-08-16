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
  File Name     : logtrace_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha,away,nina,blake
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines logtrace demo function.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include "..\include\sw_lib.h"
#include ".\logtrace_function.h"
#include ".\logtrace_parser.h"
U8  g_ulSubsystemPuNum;
U32 gPlnPerPu;
U32 gPagePerBlock;
U32 gBlkPerPln;
U32 gPhyPageSize;
U32 gRsvdBlkPerCE;
U16 l_aSLCPageMap[512];

U8 *pBBT[MAX_FLASH_SUBSYS_CNT];
RT* pRT[MAX_FLASH_SUBSYS_CNT];
U8 gBlockSel[MAX_FLASH_SUBSYS_CNT];
U16 g_usRTBlock[MAX_FLASH_SUBSYS_CNT][SUBSYSTEM_PU_MAX];
U16 g_usAT0Block[MAX_FLASH_SUBSYS_CNT][SUBSYSTEM_PU_MAX][AT0_BLOCK_COUNT];
U16 g_usAT1Block[MAX_FLASH_SUBSYS_CNT][SUBSYSTEM_PU_MAX][AT1_BLOCK_COUNT];
U16 g_usTraceBlock[MAX_FLASH_SUBSYS_CNT][SUBSYSTEM_PU_MAX][TRACE_BLOCK_COUNT];
U16 gLogTraceCurrPage[MAX_FLASH_SUBSYS_CNT][SUBSYSTEM_PU_MAX][TRACE_BLOCK_COUNT];
STATUS Api_Ext_Get_TraceLogInfo(PDEVICE_OBJECT pDevObj, TL_INFO *pTraceLogInfoBuf)
{
    STATUS Status;

    Status = Api_Read_Dram_Sram(pDevObj, MCU0, pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ulTLInfoAddr, TL_INFO_SIZE, (U8 *)pTraceLogInfoBuf);

    return Status;
}
STATUS Api_Ext_Get_TraceLogData(PDEVICE_OBJECT pDevObj, FILE *fLogFile, CPUID MCUID, TL_INFO *pTraceLogInfoBuf)
{
    UCHAR ucMCUID = MCUID - 1;
    TL_INFO *pTLInfo = pTraceLogInfoBuf;

    ULONG ulReadSecCnt;
    ULONG ulRemainSecCnt;
    ULONG ulTlMemSecCnt = (pTLInfo->ulTlMemSize >> 9);
    ULONG ulStartReadSec;
    ULONG ulHostBufSizeSec = 64;
    ULONG ulCurSecCnt;
    ULONG ulCurReadSec;
    ULONG ulReadDevAddr;

    BYTE *pHostReadBuf;

    /* get read length */
    ulReadSecCnt = pTLInfo->ulValidSecCnt;

    /* get read start sector location */
    ulStartReadSec = pTLInfo->ulReadSec;

    printf("MCU %d: TlMemBase = 0x%x, TlMemSize = 0x%x, ValidSecCnt = 0x%x, ReadSec = 0x%x\n",
        ucMCUID, pTLInfo->ulTlMemBase, pTLInfo->ulTlMemSize, pTLInfo->ulValidSecCnt, pTLInfo->ulReadSec);
    printf("StartSecID = 0x%x, ReadSecCnt = 0x%x\n", ulStartReadSec, ulReadSecCnt);

    /* read TL data from device */
    pHostReadBuf = (U8 *)malloc(ulHostBufSizeSec << 9); //32K
    ulRemainSecCnt = ulReadSecCnt;
    ulCurReadSec = ulStartReadSec;
    while (ulRemainSecCnt > 0)
    {
        memset(pHostReadBuf, 0, ulHostBufSizeSec << 9);

        ulReadDevAddr = pTLInfo->ulTlMemBase + ((ulCurReadSec % ulTlMemSecCnt) << 9);

        ulCurSecCnt = (ulRemainSecCnt < ulHostBufSizeSec) ? ulRemainSecCnt : ulHostBufSizeSec;
        if (((ulCurReadSec + 1) <= ulTlMemSecCnt)
            && ((ulCurReadSec + ulCurSecCnt + 1) > ulTlMemSecCnt))
        {
            ulCurSecCnt = ulTlMemSecCnt - ulCurReadSec;
        }
        printf("read from Sector 0x%x, SecCnt = 0x%x\n", ulCurReadSec % ulTlMemSecCnt, ulCurSecCnt);

        Api_Read_Dram_Sram(pDevObj, ucMCUID + 1, ulReadDevAddr, ulCurSecCnt << 9, pHostReadBuf);
        fwrite(pHostReadBuf, 512, ulCurSecCnt, fLogFile);
        Api_Ext_TraceLogControl(pDevObj, MCUID, TL_UPDATE_INVALID, (U16)ulCurSecCnt);

        ulCurReadSec += ulCurSecCnt;
        ulRemainSecCnt -= ulCurSecCnt;
    }

    free(pHostReadBuf);

    return RETURN_SUCCESS;
}
STATUS Api_Ext_GetTraceLog(PDEVICE_OBJECT pDevObj, char *pReportFileDir)
{
    FILE *fLogFile;
    char aFileName[128];
    UCHAR ucMCUID;
    TL_INFO *pTraceLogInfoBuf = (TL_INFO *)malloc(TL_INFO_SIZE);

    memset(pTraceLogInfoBuf, 0, TL_INFO_SIZE);

    Api_Ext_TraceLogControl(pDevObj, MCU0, TL_CTL_DISABLE, 0);
    Api_Ext_Get_TraceLogInfo(pDevObj, pTraceLogInfoBuf);

    /* open file */
    for (ucMCUID = 0; ucMCUID < 3; ucMCUID++)
    {
        memset(aFileName, 0, 128);
        sprintf_s(aFileName, 128, "%s\\MCU%dTL", pReportFileDir, ucMCUID);
        fopen_s(&fLogFile, aFileName, "wb+");
        if (NULL == fLogFile)
        {
            printf("Can't open file %s with w", aFileName);
            system("pause");
            return RETURN_FAIL;
        }
        Api_Ext_Get_TraceLogData(pDevObj, fLogFile, (CPUID)(ucMCUID + 1), (TL_INFO *)&pTraceLogInfoBuf[ucMCUID]);

        fclose(fLogFile);
    }
    //Api_Ext_TraceLogControl(pDevObj, MCU0, TL_CTL_ENABLE, 0);

    free(pTraceLogInfoBuf);
    return RETURN_SUCCESS;
}
STATUS Api_Ext_Get_TraceLogReport(PDEVICE_OBJECT pDevObj, char *pReportFileDir, char *pFormatFile0, char *pFormatFile12, char *pTraceLogHeaderFile)
{
    char mcuid;
    char aDataFileName[128];
    Api_Ext_GetTraceLog(pDevObj, pReportFileDir);
    for (mcuid = 0; mcuid < 3; mcuid++)
    {
        memset(aDataFileName, 0, 128);
        sprintf_s(aDataFileName, 128, "%s\\MCU%dTL", pReportFileDir, mcuid);
        if (mcuid == 0)
        {
            Decoder(pFormatFile0, aDataFileName, pTraceLogHeaderFile, pReportFileDir, mcuid);
        }
        else
        {
            Decoder(pFormatFile12, aDataFileName, pTraceLogHeaderFile, pReportFileDir, mcuid);
        }
    }
    return RETURN_SUCCESS;
}

STATUS Api_Ext_Get_HostInfo(PDEVICE_OBJECT pDevObj)
{
    U32 ulHostInfoStartAddr, ulHostInfoLenth;
    HOST_INFO_PAGE *pHostInfoBuf;

    ulHostInfoStartAddr = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ulHInfoBaseAddr;
    ulHostInfoLenth = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ulHInfoBaseSize;

    pHostInfoBuf = (HOST_INFO_PAGE *)malloc(ulHostInfoLenth);
    memset(pHostInfoBuf, 0, ulHostInfoLenth);

    Api_Read_Dram_Sram(pDevObj, 1, ulHostInfoStartAddr, ulHostInfoLenth, (U8 *)pHostInfoBuf);

    printf("SErrorCnt = 0x%x\n", pHostInfoBuf->SErrorCnt);

    printf("PowerOnHours = 0x%x, PowerOnMins = 0x%x,PowerOnSecs = 0x%x\n", pHostInfoBuf->PowerOnHours,
        pHostInfoBuf->PowerOnMins, pHostInfoBuf->PowerOnSecs);

    printf("TotalLBAWrittenHigh = 0x%x, TotalLBAWrittenLow = 0x%x\n", pHostInfoBuf->TotalLBAWrittenHigh,
        pHostInfoBuf->TotalLBAWrittenLow);

    printf("TotalLBAReadHigh = 0x%x, TotalLBAReadLow = 0x%x\n", pHostInfoBuf->TotalLBAReadHigh,
        pHostInfoBuf->TotalLBAReadLow);

    free(pHostInfoBuf);
    return RETURN_SUCCESS;
}

STATUS Api_Ext_Get_DeviceInfo(PDEVICE_OBJECT pDevObj)
{
    U32 ulDeviceInfoStartAddr, ulDeviceInfoLenth, cpuid, ucSubSysCnt;
    DEVICE_PARAM_PAGE *pDeviceInfoBuf;

    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
    for (cpuid = MCU1; cpuid < (MCU1 + ucSubSysCnt); cpuid++)
    {
        printf("MCU = 0x%x DeviceInfo\n", cpuid);

        ulDeviceInfoStartAddr = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulDParamBaseAddr;
        ulDeviceInfoLenth = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulDParamBaseSize;

        pDeviceInfoBuf = (DEVICE_PARAM_PAGE *)malloc(ulDeviceInfoLenth);

        memset(pDeviceInfoBuf, 0, ulDeviceInfoLenth);

        Api_Read_Dram_Sram(pDevObj, cpuid, ulDeviceInfoStartAddr, ulDeviceInfoLenth, (U8 *)pDeviceInfoBuf);

        printf("PowerCycleCnt = 0x%x,SafeShutdownCnt = 0x%x\n", pDeviceInfoBuf->PowerCycleCnt,
            pDeviceInfoBuf->SafeShutdownCnt);
        printf("EraseFailCnt = 0x%x,ProgramFailCnt = 0x%x,SYSUECCCnt = 0x%x\n", pDeviceInfoBuf->EraseFailCnt,
            pDeviceInfoBuf->ProgramFailCnt, pDeviceInfoBuf->SYSUECCCnt);
        free(pDeviceInfoBuf);

    }


    return RETURN_SUCCESS;
}

STATUS Api_Ext_Get_FlashMonitorInfo(PDEVICE_OBJECT pDevObj)
{
    U32 ulDeviceInfoStartAddr, ulDeviceInfoLenth, cpuid, ucSubSysCnt, ulPu, ulSubSysPu;
    FCMD_ENTRY_MONITOR *pDeviceInfoBuf;

    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;

    for (cpuid = MCU1; cpuid < (MCU1 + ucSubSysCnt); cpuid++)
    {
        ulSubSysPu = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ucPuNum;
        printf("MCU = 0x%x FCMD_ENTRY_MONITOR\n", cpuid);

        for (ulPu = 0; ulPu < ulSubSysPu; ulPu++)
        {
            ulDeviceInfoStartAddr = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulFlashMonitorAddr[ulPu];
            ulDeviceInfoLenth = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulFlashMonitorSize[ulPu];

            pDeviceInfoBuf = (FCMD_ENTRY_MONITOR *)malloc(ulDeviceInfoLenth);

            memset(pDeviceInfoBuf, 0, ulDeviceInfoLenth);

            Api_Read_Dram_Sram(pDevObj, cpuid, ulDeviceInfoStartAddr, ulDeviceInfoLenth, (U8 *)pDeviceInfoBuf);

            printf("Pu = 0x%x\n", ulPu);

            printf("bsFlashAddrBlk = 0x%x,bsFlashAddrPage = 0x%x,bsCmdType = 0x%x\n", pDeviceInfoBuf->bsFlashAddrBlk,
                pDeviceInfoBuf->bsFlashAddrPage, pDeviceInfoBuf->bsCmdType);
            printf("bsERSErrCnt = 0x%x,bsPRGErrCnt = 0x%x\n", pDeviceInfoBuf->bsERSErrCnt,
                pDeviceInfoBuf->bsPRGErrCnt);
            printf("bsRECCErrCnt = 0x%x,bsUECCErrCnt = 0x%x\n", pDeviceInfoBuf->bsRECCErrCnt,
                pDeviceInfoBuf->bsUECCErrCnt);
            printf("EraseTime = 0x%x,ProgramTime = 0x%x,ReadTime = 0x%x\n", pDeviceInfoBuf->EraseTime,
                pDeviceInfoBuf->ProgramTime, pDeviceInfoBuf->ReadTime);

            free(pDeviceInfoBuf);
        }


    }


    return RETURN_SUCCESS;
}

STATUS Api_Ext_Get_PBITInfo(PDEVICE_OBJECT pDevObj, char *pReportFileDir)
{
    U32 ulDeviceInfoStartAddr, ulDeviceInfoLenth, cpuid, ucSubSysCnt, ulSubSysPu, ulSubSysSuperPu, ulLunInSuperPu;
    PBIT_ENTRY *pDeviceInfoBuf;
    FILE *fpexcel = NULL;
    U16 ulTotalBlockPerCE;
    U8 ucPuNum, ucLunNum;
    U16 usBlock;
    char tmp_fileName[128] = "";
    SYSTEMTIME sys1;
    PBIT_ENTRY* pPBIT;

    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;

    for (cpuid = MCU1; cpuid < (MCU1 + ucSubSysCnt); cpuid++)
    {
        ulSubSysPu = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ucPuNum;
        ulSubSysSuperPu = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulSuperPuNum;
        ulLunInSuperPu = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulLunInSuperPu;

        printf("MCU = 0x%x PBITInfo\n", cpuid);

        for (ucPuNum = 0; ucPuNum < ulSubSysSuperPu; ucPuNum++)
        {
            ulDeviceInfoStartAddr = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulPBITBaseAddr[ucPuNum];
            ulDeviceInfoLenth = pDevObj->VarTable.SubSysTable[cpuid - MCU1].ulPBITBaseSize[ucPuNum];

            pDeviceInfoBuf = (PBIT_ENTRY *)malloc(ulDeviceInfoLenth);

            memset(pDeviceInfoBuf, 0, ulDeviceInfoLenth);

            Api_Read_Dram_Sram(pDevObj, cpuid, ulDeviceInfoStartAddr, ulDeviceInfoLenth, (U8 *)pDeviceInfoBuf);

            GetLocalTime(&sys1);
            memset(tmp_fileName, 0, sizeof(tmp_fileName));

            ulTotalBlockPerCE = BLK_PER_CE + RSVD_BLK_PER_CE;
            for (ucLunNum = 0; ucLunNum < ulLunInSuperPu; ucLunNum++)
            {
                pPBIT = (PBIT_ENTRY*)(pDeviceInfoBuf + ucLunNum*sizeof(PBIT_ENTRY)*ulTotalBlockPerCE);

                sprintf_s(tmp_fileName, "%sPBIT_Statistic_Sys%d_SuperPu%d_Lun%d_%4u-%02u-%02u_%02u_%02u_%02u.xls", pReportFileDir, (cpuid - MCU1), ucPuNum, ucLunNum,sys1.wYear, sys1.wMonth, sys1.wDay, sys1.wHour, sys1.wMinute, sys1.wSecond);
                
                fopen_s(&fpexcel, tmp_fileName, "wb+");
                if (fpexcel == NULL)
                {
                    return RETURN_FAIL;
                }

                fprintf(fpexcel, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "Virtual Block", "EraseCnt", "bFree", "bError", "bAllocated", "bBackup", "bTable", "BlockType", "bWL", "bLock", "bReserved");
                for (usBlock = 0; usBlock < ulTotalBlockPerCE; usBlock++)
                {
                    fprintf(fpexcel, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                        pPBIT[usBlock].VirtualBlockAddr,
                        pPBIT[usBlock].EraseCnt,
                        pPBIT[usBlock].bFree,
                        pPBIT[usBlock].bError,
                        pPBIT[usBlock].bAllocated,
                        pPBIT[usBlock].bBackup,
                        pPBIT[usBlock].bTable,
                        pPBIT[usBlock].BlockType,
                        pPBIT[usBlock].bWL,
                        pPBIT[usBlock].bLock,
                        pPBIT[usBlock].bReserved);
                }

                fclose(fpexcel);
            }
            free(pDeviceInfoBuf);
        }
    }
    return RETURN_SUCCESS;
}

int Api_Ext_Get_SysInfo(PDEVICE_OBJECT pDevObj, char *pReportFileDir)
{
    U8 ch;

    while (1)
    {
        printf("\nSysInfo related CMD MENU:\n");
        printf("1. read host info page\n");
        printf("2. read device info page\n");
        printf("3. read flash monitor info page\n");
        printf("4. read PBIT info page\n");
        printf("5. dbg showall\n");
        printf("0. EXIT\n");
        printf("\nPlease Press Key 1 - 5 to select command: ");

        ch = DBG_Getch();
        ch -= 0x30;

        DBG_printf("%d\n", ch);

        switch (ch)
        {
        case 0:
            return 0;
        case 1:
            Api_Ext_Get_HostInfo(pDevObj);
            break;
        case 2:
            Api_Ext_Get_DeviceInfo(pDevObj);
            break;
        case 3:
            Api_Ext_Get_FlashMonitorInfo(pDevObj);
            break;
        case 4:
            Api_Ext_Get_PBITInfo(pDevObj, pReportFileDir);
            break;
        case 5:
            Api_Ext_Dbg_ShowAll(pDevObj, MCU0);
            break;
        default:
            break;
        }

    }

    return 0;
}


FILE* FAE_OpenFile(char* fileName)
{
    FILE* fLogFile;
#if 0
    SYSTEMTIME sys1;
    char tmp_fileName[128] = "";

    GetLocalTime(&sys1);

    memset(tmp_fileName, 0, sizeof(tmp_fileName));
    sprintf_s(tmp_fileName, ".\\output\\%s_%4u-%02u-%02u_%02u_%02u_%02u.txt", fileName, sys1.wYear, sys1.wMonth, sys1.wDay, sys1.wHour, sys1.wMinute, sys1.wSecond);

    fopen_s(&fLogFile, tmp_fileName, "wb+");
#else
    fopen_s(&fLogFile, fileName, "wb+");
    if (NULL == fLogFile)
    {
        DBG_printf("FAE_OpenFile %s fail!\n", fileName);
        DBG_Getch();
    }
#endif

    return fLogFile;
}


void FAE_SaveFile(FILE* pFile, U8* pBuffer, U32 ulBytes)
{
    U32 ulRealWrite = 0;

    if (NULL == pFile)
    {
        DBG_printf("FAE_SaveFile pFile == NULL\n", GetLastError());
        DBG_Getch();
    }

    ulRealWrite = fwrite(pBuffer, 1, ulBytes, pFile);
    if (ulRealWrite != ulBytes)
    {
        DBG_printf("FAE_SaveFile Error:%d\n", GetLastError());
        DBG_Getch();
    }
    return;
}

void FAE_CloseFile(FILE* pFile)
{
    fclose(pFile);
}

void Api_Ext_AddPuToMsk(U8 ucLogicPu, U32* pulPuMask)
{
    U32 ulLogicPuMsk;

    ulLogicPuMsk = *pulPuMask;
    ulLogicPuMsk |= (1 << ucLogicPu);
    *pulPuMask = ulLogicPuMsk;

    return;
}


void Init_L95_SLCMapping(void)
{
    U32 ulLogicPage;

    for (ulLogicPage = 0; ulLogicPage < PG_PER_BLK / 2; ulLogicPage++)
    {
        if (ulLogicPage < 6)
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage;
            continue;
        }

        if ((ulLogicPage == 6) || (ulLogicPage == 7))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage + 1;
            continue;
        }

        if (0 == (ulLogicPage % 2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage * 2 - 6;
        }
        else if (1 == (ulLogicPage % 2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage * 2 - 7;
        }
    }
}

void Init_L85_SLCMapping(void)
{
    U32 ulLogicPage;

    for (ulLogicPage = 0; ulLogicPage < PG_PER_BLK / 2; ulLogicPage++)
    {
        if (ulLogicPage < 6)
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage;
            continue;
        }

        if (0 == (ulLogicPage % 2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage * 2 - 4;
        }
        else if (1 == (ulLogicPage % 2))
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage * 2 - 5;
        }
    }

    return;
}

void Init_TSB_SLCMapping(void)
{
    U32 ulLogicPage;
    for (ulLogicPage = 0; ulLogicPage < PG_PER_BLK / 2; ulLogicPage++)
    {
        if (ulLogicPage == 0)
        {
            l_aSLCPageMap[ulLogicPage] = 0;
        }
        else
        {
            l_aSLCPageMap[ulLogicPage] = ulLogicPage * 2 - 1;
        }
    }
}
/*
#L85

#set $aFlashChipId0Val = 0x3c64842c

#set $aFlashChipId1Val  = 0xa5



#L95

set $aFlashChipId0Val = 0x54e5a42c

set $aFlashChipId1Val  = 0xa9



#set $aFlashChipId0Val = 0x5464842c

#set $aFlashChipId1Val  = 0xa9



#TSB

#set $aFlashChipId0Val = 0x93953a98

#set $aFlashChipId1Val  = 0x0408577a



#TSB_FOURPLN

#set $aFlashChipId0Val = 0x93a43a98

#set $aFlashChipId1Val = 0x0408d07a

#TSB_FOURPLN

#set $aFlashChipId0Val = 0x93a43a98

#set $aFlashChipId1Val = 0x0408d17a



#TSB_FOURPLN_A19_MULTI_LUN

#set $aFlashChipId0Val = 0x93a53c98

#set $aFlashChipId1Val = 0x0408d17e


#TSB_FOURPLN_A19_MULTI_LUN

#set $aFlashChipId0Val = 0x93a53c98

#set $aFlashChipId1Val = 0x0408d07e
*/

void Api_Ext_InitFlashSLCMapping(PDEVICE_OBJECT pDevObj)
{
    U32 ulFlashChipId[2];

    g_ulSubsystemPuNum = pDevObj->VarTable.SubSysTable[0].ucPuNum;
    gPlnPerPu = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
    gBlkPerPln = pDevObj->VarTable.SubSysTable[0].ucBlkNum;
    gRsvdBlkPerCE = pDevObj->VarTable.SubSysTable[0].ulRsvBlkNum;
    gPagePerBlock = pDevObj->VarTable.SubSysTable[0].ucPageNum;
    gPhyPageSize = pDevObj->VarTable.SubSysTable[0].ucPhyPageSize;

    ulFlashChipId[0] = pDevObj->VarTable.SubSysTable[0].ulFlashId[0];
    ulFlashChipId[1] = pDevObj->VarTable.SubSysTable[0].ulFlashId[1];
	
    //L95
    if (0x5464842c == ulFlashChipId[0] && 0xa9 == ulFlashChipId[1])
    {
        Init_L95_SLCMapping();
    }
	/*
	#L95
	set $aFlashChipId0Val = 0x54e5a42c
	set $aFlashChipId1Val = 0xa9
	*/
	else if (0x54e5a42c == ulFlashChipId[0] && 0xa9 == ulFlashChipId[1])
	{
		Init_L95_SLCMapping();
	}
    //L85
    else if (0x3c64842c == ulFlashChipId[0] && 0xa5 == ulFlashChipId[1])
    {
        Init_L85_SLCMapping();
    }
    //TSB
	/*
	#TSB

	#set $aFlashChipId0Val = 0x93953a98

	#set $aFlashChipId1Val  = 0x0408577a
	*/
    else if (0x93953a98 == ulFlashChipId[0])
    {
        Init_TSB_SLCMapping();
    }
    //TSB 4Pln
	/*
	#TSB_FOURPLN

	#set $aFlashChipId0Val = 0x93a43a98

	#set $aFlashChipId1Val = 0x0408d07a
	*/
	else if (0x93a43a98 == ulFlashChipId[0])
    {
        Init_TSB_SLCMapping();
    }
	//TSB a19
	/*
	#TSB_FOURPLN_A19_MULTI_LUN

	#set $aFlashChipId0Val = 0x93a53c98

	#set $aFlashChipId1Val = 0x0408d07e
	*/
	else if (0x93a53c98 == ulFlashChipId[0])
	{
		Init_TSB_SLCMapping();
	}
    else
    {
        DBG_printf("FlashId0 0x%x,FlashId1 0x%x\n", ulFlashChipId[0], ulFlashChipId[1]);
        DBG_Getch();
    }
} 

U16 Api_Ext_GetSLCPage(U16 usLogicPage)
{
    return l_aSLCPageMap[usLogicPage];
}

void Api_Ext_Find_LatestBBT(PDEVICE_OBJECT pDevObj, U8 ucSubsysId, U8* pucSubPuNum, U16* pusBlock, U16* pusPPO)
{
    U8   ucSubPuNum;
    U8   ucLogicPu;
    U16  usBlock;
    U16  usPage;
    U16  usPPO;
    U32 ulLogicPuMsk;
    U32 ulMaxSN;
    U8* pRedbuf = NULL;
    RED* pRed;
    U8 * pReadBuf = NULL;
    U32 ulStatus;

    U8 ucPlnMode = 0xff;
    U32 ulLogicPuMskStatus = 0;

    usBlock = 1;
    ulMaxSN = 0;

    for (ucSubPuNum = 0; ucSubPuNum < SUBSYSTEM_PU_NUM; ucSubPuNum++)
    {
        ucLogicPu = ucSubPuNum + ucSubsysId * SUBSYSTEM_PU_NUM;

        ulLogicPuMsk = 0;
        Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

        for (usPPO = 0; usPPO < PG_PER_BLK / 2; usPPO += 2)
        {
            usPage = Api_Ext_GetSLCPage(usPPO);

            /* get latest BBT by BBT SN */
            Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);

            ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
            if (NF_SUCCESS != ulStatus)
            {
                if ((NF_ERR_TYPE_EMPTY_PG == ulStatus) || (NF_ERR_TYPE_EMPTY_PG_E0 == ulStatus))
                {
                    /* page 0 is empty page means this PU cannot store BBT */
                    if (usPage == 0)
                    {
                        break;
                    }
                }
                else
                {
                    DBG_printf("Api_Ext_Find_LatestBBT Read Pu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
                        ucLogicPu, usBlock, usPage, ulStatus);
                    DBG_Getch();
                }
            }
            else
            {
                Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadBuf);
                Api_Flash_Get_Red_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, (RED **)(&pRedbuf));

                pRed = (RED*)pRedbuf;

                if (PAGE_TYPE_BBT != pRed->m_RedComm.bcPageType)
                {
                    DBG_printf("Api_Ext_Find_LatestBBT BBT Type 0x%x ERROR!\n", pRed->m_RedComm.bcPageType);
                    DBG_Getch();
                }
                else
                {
                    if (ulMaxSN <= pRed->bbt_sn)
                    {
                        *pucSubPuNum = ucSubPuNum;
                        *pusBlock = usBlock;
                        *pusPPO = usPPO;
                        ulMaxSN = pRed->bbt_sn;
                    }
                }

            }
        }
    }

    DBG_printf("Find_LatestBBT Subid %d SubPu %d(LogicPu %d) Block 0x%x PPO 0x%x MaxSN 0x%x done!\n",
        ucSubsysId, *pucSubPuNum, (*pucSubPuNum + ucSubsysId * SUBSYSTEM_PU_NUM), *pusBlock, *pusPPO, ulMaxSN);

    return;
}
U8 BBT_ChkBadBlk(U8 ucTLun, U16 usBlock, U8 ucPln, U32 ulGBBTAddr)
{
    U32 ulBitPos, ulBytePos;
    BOOL bBadBlk;

    ulBitPos = ucTLun * PLN_PER_LUN * BBT_BLK_PER_PLN + ucPln * BBT_BLK_PER_PLN + usBlock;
    ulBytePos = ulGBBTAddr + ulBitPos / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (ulBitPos % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

BOOL BBT_IsBadBlockBBT(U32 BBTAddr, U8 Pu, U16 BlkAddr)
{
    if ((TRUE == BBT_ChkBadBlk(Pu, BlkAddr, 0, BBTAddr)) || (TRUE == BBT_ChkBadBlk(Pu, BlkAddr, 1, BBTAddr)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void BBT_Print_AllBadBlk(U8 ucSubsysId)
{
    U8 ucSubsysPu;
    U16 usBlk;
    U8 ucPln;

    for (ucSubsysPu = 0; ucSubsysPu < SUBSYSTEM_PU_NUM; ucSubsysPu++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            DBG_printf("SubId:%d SubPu:%d Pln:%d BadBlk:\n", ucSubsysId, ucSubsysPu, ucPln);
            for (usBlk = 0; usBlk < BBT_BLK_PER_PLN; usBlk++)
            {
                if (TRUE == BBT_ChkBadBlk(ucSubsysPu, usBlk, ucPln, (U32)&pBBT[ucSubsysId][0]))
                {
                    DBG_printf(" %d ", usBlk);
                }
            }
            DBG_printf("\n");
        }
    }
}

U16 L2_RT_Search_BlockBBT(U8 ucSubsysId, U8 ucSubsysPu, U16 usNoneBadPos)
{
    U16 usNoneBadCnt;
    U16 PhyBlockAddr;

    usNoneBadCnt = 0;

    for (PhyBlockAddr = BBT_BLOCK_ADDR_BASE; PhyBlockAddr < (BLK_PER_CE + RSVD_BLK_PER_CE); PhyBlockAddr++)
    {
        //Search none bad block from BBT
        if (TRUE == BBT_IsBadBlockBBT((U32)&pBBT[ucSubsysId][0], ucSubsysPu, PhyBlockAddr))
        {
            DBG_printf("Subid %d SubPu %d Badblk 0x%x\n", ucSubsysId, ucSubsysPu, PhyBlockAddr);
            continue;
        }
        else
        {
            usNoneBadCnt++;
        }

        if (usNoneBadCnt == usNoneBadPos)
        {
            return PhyBlockAddr;
        }
    }

    return INVALID_4F;
}

void Api_Ext_MarkTableBlock(U8 ucSubsysId, U8 ucSubsysPu)
{
    U16 usBlock;
    U16 usBlockPos;
    U16 usBlockIndex;

    usBlockPos = (GLOBAL_BLOCK_COUNT + BBT_BLOCK_COUNT);

    /* mark RT block */
    usBlock = L2_RT_Search_BlockBBT(ucSubsysId, ucSubsysPu, usBlockPos);
    g_usRTBlock[ucSubsysId][ucSubsysPu] = usBlock;

    usBlockPos += RT_BLOCK_COUNT;

    /* mark AT0 block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT0_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSubsysId, ucSubsysPu, usBlockIndex);
        g_usAT0Block[ucSubsysId][ucSubsysPu][(usBlockIndex - usBlockPos)] = usBlock;
    }

    usBlockPos += AT0_BLOCK_COUNT;

    /* mark AT1 block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT1_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSubsysId, ucSubsysPu, usBlockIndex);
        g_usAT1Block[ucSubsysId][ucSubsysPu][(usBlockIndex - usBlockPos)] = usBlock;
    }

    usBlockPos += AT1_BLOCK_COUNT;

    /* mark Trace block */
    for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + TRACE_BLOCK_COUNT); usBlockIndex++)
    {
        usBlock = L2_RT_Search_BlockBBT(ucSubsysId, ucSubsysPu, usBlockIndex);
        g_usTraceBlock[ucSubsysId][ucSubsysPu][(usBlockIndex - usBlockPos)] = usBlock;
    }

    return;
}

void Api_Ext_Find_LatestRT(PDEVICE_OBJECT pDevObj, U8 ucSubsysId, U8* pucSubPuNum, U16* pusBlock, U16* pusPage)
{
    U8   ucLogicPu;
    U8   ucSubPuNum;
    U16  usBlock;
    U16  usPPO, usPage;
    U16  usLastOkPage;
    U32 ulLogicPuMsk = 0;
    U8* pRedbuf = NULL;
    RED* pRed = NULL;
    U32 ulStatus = 0;

    *pucSubPuNum = INVALID_2F;
    *pusBlock = INVALID_4F;
    *pusPage = INVALID_4F;
    U8 ucPlnMode = 0xff;

    /* i. find Log Page CE from FLASH */
    for (ucSubPuNum = 0; ucSubPuNum < SUBSYSTEM_PU_NUM; ucSubPuNum++)
    {
        ucLogicPu = ucSubPuNum + ucSubsysId * SUBSYSTEM_PU_NUM;

        ulLogicPuMsk = 0;
        Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);
        Api_Read_Flash(pDevObj, ucPlnMode, g_usRTBlock[ucSubsysId][ucSubPuNum], 0, ulLogicPuMsk, 0);

        ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
        if (NF_SUCCESS != ulStatus)
        {
            if ((NF_ERR_TYPE_EMPTY_PG == ulStatus) || (NF_ERR_TYPE_EMPTY_PG_E0 == ulStatus))
            {
                continue;
            }
            else
            {
                DBG_printf("Api_Ext_Find_LatestRT Read Pu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
                    ucLogicPu, g_usRTBlock[ucSubsysId][ucSubPuNum], 0, ulStatus);
                DBG_Getch();
            }
        }
        else
        {
            Api_Flash_Get_Red_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, (RED**)&pRed);
            if (PAGE_TYPE_RT == pRed->m_RedComm.bcPageType)
            {
                //DBG_printf("Api_Ext_Find_LatestRT SubId %d SubPu 0x%x Block 0x%x Success!\n",ucSubsysId, ucSubPuNum, g_usRTBlock[ucSubsysId][ucSubPuNum]);
                *pucSubPuNum = ucSubPuNum;
                *pusBlock = g_usRTBlock[ucSubsysId][ucSubPuNum];
                *pusPage = 0;
                break;
            }
            else
            {
                continue;
            }
        }
    }

    /* find latest Log Page from FLASH */
    if (0 != *pusPage)
    {
        DBG_printf("Api_Ext_Find_LatestRT Log Page Find BLock 0x%x Pu 0x%x ERROR!\n", g_usRTBlock[ucLogicPu], ucLogicPu);
        DBG_Getch();
    }
    else
    {
        ulLogicPuMsk = 0;
        ucLogicPu = ucSubPuNum + ucSubsysId * SUBSYSTEM_PU_NUM;
        usBlock = *pusBlock;
        usLastOkPage = 0;

        Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

        for (usPPO = 1; usPPO < PG_PER_BLK / 2; usPPO++)
        {
            usPage = Api_Ext_GetSLCPage(usPPO);

            Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);

            ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
            if (NF_SUCCESS != ulStatus)
            {
                if ((NF_ERR_TYPE_EMPTY_PG != ulStatus) && (NF_ERR_TYPE_EMPTY_PG_E0 != ulStatus))
                {
                    DBG_printf("Api_Ext_Find_LatestRT Read SubPu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
                        ucSubPuNum, usBlock, usPage, ulStatus);
                    DBG_Getch();
                }
            }
            else
            {
                usLastOkPage = usPage;
            }

            if (usPPO == (PG_PER_BLK / 2 - 1))
            {
                //DBG_printf("Api_Ext_Find_LatestRT Page 0x%x Success!\n", usLastOkPage);
                *pusPage = usLastOkPage;
                return;
            }
        }
    }
    return;
}

void Analysis_RT(U8 ucSubsysId, U16* pusTracePhyPos)
{
    /* get Log Trace Info */
    *pusTracePhyPos = pRT[ucSubsysId]->m_TraceBlockManager.m_TracePhyPos;

    return;
}

U8 Api_Ext_GetTraceLog_BlockSel(PDEVICE_OBJECT pDevObj, U8 ucSubsysId)
{
    U16  usBlock;
    U32  i;
    U32 ulLogicPuMsk;
    RED* pRed;
    U8 ucBlockSel;
    U32 ulSNLow;
    U32 ulSNHigh;
    U8 bFoundLow;
    U8 bFoundHigh;
    U8 ucPlnMode = 0xff;
    U32 ulStatus = 0;
    U8 ucLogicPu;

    bFoundLow = FALSE;
    bFoundHigh = FALSE;
    ulSNLow = INVALID_8F;
    ulSNHigh = INVALID_8F;

    ucLogicPu = 0;
    ulLogicPuMsk = 0;
    Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

    for (i = 0; i < TRACE_BLOCK_COUNT; i++)
    {
        usBlock = g_usTraceBlock[ucSubsysId][0][i];

        Api_Read_Flash(pDevObj, ucPlnMode, usBlock, 0, ulLogicPuMsk, 0);
        ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
        if (NF_SUCCESS != ulStatus)
        {
            if ((NF_ERR_TYPE_EMPTY_PG != ulStatus) && (NF_ERR_TYPE_EMPTY_PG_E0 != ulStatus))
            {
                DBG_printf("Api_Ext_GetTraceLog_BlockSel Read Pu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
                    ucLogicPu, usBlock, 0, ulStatus);
                DBG_Getch();
            }
        }
        else
        {
            Api_Flash_Get_Red_Addr_By_Pu(pDevObj, 0, ucPlnMode, &pRed);
            if (PAGE_TYPE_TRACE == pRed->m_RedComm.bcPageType)
            {
                if (0 == i)
                {
                    ulSNLow = pRed->m_TraceSN;
                    bFoundLow = TRUE;
                }
                else
                {
                    ulSNHigh = pRed->m_TraceSN;
                    bFoundHigh = TRUE;
                }
            }
        }
    }

    if (FALSE == bFoundLow)
    {
        if (FALSE == bFoundHigh)
        {
            ucBlockSel = LOG_TRACE_BLOCK_SEL_NONE;
        }
        else
        {
            ucBlockSel = LOG_TRACE_BLOCK_SEL_HIGH;
        }
    }
    else
    {
        if (FALSE == bFoundHigh)
        {
            ucBlockSel = LOG_TRACE_BLOCK_SEL_LOW;
        }
        else
        {
            if (ulSNLow >= ulSNHigh)
            {
                ucBlockSel = LOG_TRACE_BLOCK_SEL_LOW;
            }
            else
            {
                ucBlockSel = LOG_TRACE_BLOCK_SEL_HIGH;
            }
        }
    }
    return ucBlockSel;
}

BOOL Api_Ext_CheckLogTrace_FLASH(PDEVICE_OBJECT pDevObj, U8 ucSubsysId, U8 ucSubsysPu, U32* pMcuId, U32* pSecCnt, TL_INFO** pTLInfo)
{
    U8  ucBlockPos;
    U16 usBlock;
    U16 usPage;
    U32 ulLogicPuMsk;
    U8* pRedbuf = NULL;
    RED* pRed;
    U8 ucPlnMode = 0xff;
    U32 ulStatus;
    U8 ucLogicPu;

    *pSecCnt = 0;

    if (LOG_TRACE_BLOCK_SEL_LOW == gBlockSel[ucSubsysId])
    {
        if (gLogTraceCurrPage[ucSubsysId][ucSubsysPu][0] >= PG_PER_BLK)
        {
            ucBlockPos = 1;
        }
        else
        {
            ucBlockPos = 0;
        }
    }
    else
    {
        if (gLogTraceCurrPage[ucSubsysId][ucSubsysPu][1] >= PG_PER_BLK)
        {
            ucBlockPos = 0;
        }
        else
        {
            ucBlockPos = 1;
        }
    }

    ulLogicPuMsk = 0;
    ucLogicPu = ucSubsysPu + ucSubsysId * SUBSYSTEM_PU_NUM;
    usBlock = g_usTraceBlock[ucSubsysId][ucSubsysPu][ucBlockPos];

    Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

    for (usPage = gLogTraceCurrPage[ucSubsysId][ucSubsysPu][ucBlockPos]; usPage < PG_PER_BLK; usPage++)
    {
        Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);

        ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
        if (NF_SUCCESS != ulStatus)
        {
            if ((NF_ERR_TYPE_EMPTY_PG == ulStatus) || (NF_ERR_TYPE_EMPTY_PG_E0 == ulStatus))
            {
                /* Page 0 is Empty page means this block don't have any Log Trace saved */
                if (usPage == 0)
                {
                    break;
                }
            }
            else
            {
                DBG_printf("Api_Ext_CheckLogTrace_FLASH Read Pu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
                    ucLogicPu, usBlock, usPage, ulStatus);
                DBG_Getch();
            }
        }
        else
        {
            Api_Flash_Get_Red_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pRed);
            if (PAGE_TYPE_TRACE != pRed->m_RedComm.bcPageType)
            {
                DBG_printf("FAE_Check_Log_Trace_FLASH Page Type 0x%x ERROR!\n", pRed->m_RedComm.bcPageType);
                DBG_Getch();
            }
            else
            {
                *pSecCnt = pRed->m_TraceItemCNT;
                *pMcuId = pRed->m_McuId;
                *pTLInfo = &pRed->m_TLInfo;
                gLogTraceCurrPage[ucSubsysId][ucSubsysPu][ucBlockPos] = usPage + 1;

                DBG_printf("LoadTraceLog SubId %d SubPu %d Block 0x%x Page 0x%x(MCUId %d SecCnt %d Trace,SN %d)\n",
                    ucSubsysId, ucSubsysPu, usBlock, usPage, pRed->m_McuId, pRed->m_TraceItemCNT, pRed->m_TraceSN);

                return TRUE;
            }
        }
    }

    gLogTraceCurrPage[ucSubsysId][ucSubsysPu][ucBlockPos] = PG_PER_BLK;
    return FALSE;
}

BOOL Is_All_Log_Trace_Read(U8 ucSubsysId)
{
    U8 ucBlockPos;
    U8 ucSubsysPu;

    for (ucSubsysPu = 0; ucSubsysPu < SUBSYSTEM_PU_NUM; ucSubsysPu++)
    {
        for (ucBlockPos = 0; ucBlockPos < TRACE_BLOCK_COUNT; ucBlockPos++)
        {
            if (gLogTraceCurrPage[ucSubsysId][ucSubsysPu][ucBlockPos] < PG_PER_BLK)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void Api_Ext_ReadDramBBT(PDEVICE_OBJECT pDevObj, U8 ucSubsysId, U8* pHostReadBuf)
{
    U32 ulBBTAddr = pDevObj->VarTable.SubSysTable[ucSubsysId].ulL3BbtTableAddr;
    U32 ulBytes = PAGE_SIZE;

    Api_Read_Dram_Sram(pDevObj, ucSubsysId + 2, ulBBTAddr, ulBytes, pHostReadBuf);
}


void Api_Ext_ReadTraceLog_Flash(PDEVICE_OBJECT pDevObj, char *pReportFileDir, char *pFormatFile0, char *pFormatFile12, char *pTraceLogHeaderFile)
{
    U8   ucLogicPu;
    U16  usBlock;
    U16  usPPO = 0, usPage = 0;
    U32 ulLogicPuMsk;
    U8* pRTBuf[MAX_FLASH_SUBSYS_CNT] = { NULL };
    U8* pReadbuf = NULL;
    FILE* pFile;
    U16 usTracePhyPos[MAX_FLASH_SUBSYS_CNT];
    U32 ulTotalPageRead[MCU_MAX] = { 0 };
    U32 ulReceivedTraceSecCnt[MCU_MAX] = { 0 };
    U8 ucPlnMode = 0xff;
    U8 ucSubsysId;
    U8 ucSubsysPu;
    char fileName[128];
    U32 ulMcuId;
    FILE* pFileTL[MCU_MAX];
    U32 ulSecCnt;
    TL_INFO* pTLInfo;
    U8* pMcu0SrcBuf = (U8*)malloc(L0_TL_MEM_SIZE);
    U8* pMcu0DstBuf = (U8*)malloc(L0_TL_MEM_SIZE);
    BOOL bNeedMergeBuffer = FALSE;
    U32 ulOffset;

    /* open file */
    for (ulMcuId = 0; ulMcuId < MCU_MAX; ulMcuId++)
    {
        memset(fileName, 0, 128);
        sprintf_s(fileName, 128, "%s\\MCU%d_FlashTL", pReportFileDir, ulMcuId);
        pFileTL[ulMcuId] = FAE_OpenFile(fileName);

        ulTotalPageRead[ulMcuId] = 0;
        ulReceivedTraceSecCnt[ulMcuId] = 0;
    }

    for (ucSubsysId = 0; ucSubsysId < MAX_FLASH_SUBSYS_CNT; ucSubsysId++)
    {
        pBBT[ucSubsysId] = (U8 *)malloc(PAGE_SIZE);
        pRTBuf[ucSubsysId] = (U8 *)malloc(PAGE_SIZE);

        memset(g_usTraceBlock[ucSubsysId], 0, SUBSYSTEM_PU_MAX*TRACE_BLOCK_COUNT*sizeof(g_usTraceBlock));
        memset(gLogTraceCurrPage[ucSubsysId], 0, SUBSYSTEM_PU_MAX*TRACE_BLOCK_COUNT*sizeof(gLogTraceCurrPage));

#if 0
        /* i. load latest BBT from FLASH */
        Api_Ext_Find_LatestBBT(pDevObj, ucSubsysId, &ucSubsysPu, &usBlock, &usPPO);

        if ((usBlock >= BLK_PER_CE) || (usPPO >= PG_PER_BLK / 2))
        {
            DBG_printf("Api_Ext_ReadTraceLog_Flash BBT Find BLock 0x%x Page 0x%x ERROR!\n", usBlock, usPPO);
            DBG_Getch();
        }
        else
        {
            ulLogicPuMsk = 0;
            ucLogicPu = ucSubsysPu + ucSubsysId * SUBSYSTEM_PU_NUM;

            Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

            /* BBT Size is 2*PageSize */
            usPage = Api_Ext_GetSLCPage(usPPO);
            Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);
            Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadbuf);
            memcpy(&pBBT[ucSubsysId][0], pReadbuf, PAGE_SIZE);

            usPage = Api_Ext_GetSLCPage(usPPO + 1);
            Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);
            Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadbuf);
            memcpy(&pBBT[ucSubsysId][PAGE_SIZE], pReadbuf, PAGE_SIZE);

            DBG_printf("Load_LatestBBT Subid %d SubPu %d(LogicPu %d) BLock 0x%x usPPO 0x%x Success!\n",
                ucSubsysId, ucSubsysPu, ucLogicPu, usBlock, usPPO);
        }
#else
        Api_Ext_Bbt_Load(pDevObj, ucSubsysId + 2);
        Api_Ext_ReadDramBBT(pDevObj, ucSubsysId, pBBT[ucSubsysId]);
#endif
        memset(fileName, 0, 128);
        sprintf_s(fileName, "%s\\SubSys%d_BBT_FLASH", pReportFileDir, ucSubsysId);
        pFile = FAE_OpenFile(fileName);
        FAE_SaveFile(pFile, pBBT[ucSubsysId], GBBT_BUF_SIZE);
        FAE_CloseFile(pFile);
        BBT_Print_AllBadBlk(ucSubsysId);

        /* ii. mark all table blocks from BBT */
        for (ucSubsysPu = 0; ucSubsysPu < SUBSYSTEM_PU_NUM; ucSubsysPu++)
        {
            Api_Ext_MarkTableBlock(ucSubsysId, ucSubsysPu);
        }

        /* iii. load latest RT Page from FLASH */
        Api_Ext_Find_LatestRT(pDevObj, ucSubsysId, &ucSubsysPu, &usBlock, &usPage);
        if ((usBlock >= BLK_PER_CE) || (usPage >= PG_PER_BLK) || (ucSubsysPu >= SUBSYSTEM_PU_NUM*MAX_FLASH_SUBSYS_CNT))
        {
            DBG_printf("Api_Ext_ReadTraceLog_Flash RT Find BLock 0x%x Page 0x%x Pu 0x%x ERROR!\n", usBlock, usPage, ucSubsysPu);
            DBG_Getch();
        }
        else
        {
            ulLogicPuMsk = 0;
            ucLogicPu = ucSubsysPu + ucSubsysId * SUBSYSTEM_PU_NUM;

            Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

            Api_Read_Flash(pDevObj, ucPlnMode, usBlock, usPage, ulLogicPuMsk, 0);
            Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadbuf);
            memcpy(pRTBuf[ucSubsysId], pReadbuf, PAGE_SIZE);

            DBG_printf("Load_LatestRT SubId %d SubPu %d(LogicPu %d) BLock 0x%x Page 0x%x Success!\n",
                ucSubsysId, ucSubsysPu, ucLogicPu, usBlock, usPage);
        }

        memset(fileName, 0, 128);
        sprintf_s(fileName, "%s\\SubSys%d_RT_FLASH", pReportFileDir, ucSubsysId);
        pFile = FAE_OpenFile(fileName);
        FAE_SaveFile(pFile, pRTBuf[ucSubsysId], PAGE_SIZE);
        FAE_CloseFile(pFile);

        /* analysis LB: 1. dump GlobalInfo and ECT; 2. get log trace related info */
        pRT[ucSubsysId] = (RT*)pRTBuf[ucSubsysId];
        Analysis_RT(ucSubsysId, &usTracePhyPos[ucSubsysId]);

        gBlockSel[ucSubsysId] = Api_Ext_GetTraceLog_BlockSel(pDevObj, ucSubsysId);
        if (LOG_TRACE_BLOCK_SEL_NONE == gBlockSel[ucSubsysId])
        {
            DBG_printf("Api_Ext_ReadTraceLog_Flash No Log Trace saved in FLASH. Done!\n");
        }
        else
        {
            /* iv. load and save trace from FLASH */
            ucSubsysPu = 0;
            while (1)
            {
                /* check if log page valid and read the log page to Dram buffer */
                if (TRUE == Api_Ext_CheckLogTrace_FLASH(pDevObj, ucSubsysId, ucSubsysPu, &ulMcuId, &ulSecCnt, &pTLInfo))
                {
                    ucLogicPu = ucSubsysPu + ucSubsysId * SUBSYSTEM_PU_NUM;
                    Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadbuf);
                    ulTotalPageRead[ulMcuId - 1]++;

                    if (MCU0_ID == ulMcuId)
                    {
                        ulOffset = (ulTotalPageRead[0] - 1) % (L0_TL_MEM_SIZE / PAGE_SIZE);
                        memcpy(pMcu0SrcBuf + ulOffset*PAGE_SIZE, pReadbuf, PAGE_SIZE);

                        if (0 == ulOffset && pTLInfo->ulFlushSec > pTLInfo->ulWriteSec
                            && pTLInfo->ulValidFlushCnt == pTLInfo->ulTlMemSize / SEC_SIZE)
                        {
                            bNeedMergeBuffer = TRUE;
                        }

                        if (0 != ulTotalPageRead[0] && (ulTotalPageRead[0] % (L0_TL_MEM_SIZE / PAGE_SIZE) == 0))
                        {
                            if (TRUE == bNeedMergeBuffer)
                            {
                                memset(pMcu0DstBuf, 0, L0_TL_MEM_SIZE);
                                memcpy(pMcu0DstBuf, pMcu0SrcBuf + pTLInfo->ulFlushSec*SEC_SIZE, pTLInfo->ulTlMemSize - pTLInfo->ulFlushSec*SEC_SIZE);
                                memcpy(pMcu0DstBuf + pTLInfo->ulTlMemSize - pTLInfo->ulFlushSec*SEC_SIZE, pMcu0SrcBuf, pTLInfo->ulWriteSec*SEC_SIZE);

                                FAE_SaveFile(pFileTL[MCU0_ID - 1], pMcu0DstBuf, L0_TL_MEM_SIZE);
                                DBG_printf("MergeMCU0 TraceBuffer,WritePointer 0x%x,FlusnPointer 0x%x,TLSize 0x%x\n",
                                    pTLInfo->ulWriteSec, pTLInfo->ulFlushSec, pTLInfo->ulTlMemSize);

                                DBG_printf("   ###Save MCU%d Trace,TLSize %dK ###\n", ulMcuId, pTLInfo->ulTlMemSize / 1024);
                                bNeedMergeBuffer = FALSE;
                            }
                            else
                            {
                                memcpy(pMcu0DstBuf, pMcu0SrcBuf + pTLInfo->ulFlushSec*SEC_SIZE, pTLInfo->ulValidFlushCnt*SEC_SIZE);

                                FAE_SaveFile(pFileTL[MCU0_ID - 1], pMcu0DstBuf, pTLInfo->ulValidFlushCnt*SEC_SIZE);
                                DBG_printf("MCU0 TraceBuffer WritePointer 0x%x,FlusnPointer 0x%x,ValidFlushCnt 0x%x\n",
                                    pTLInfo->ulWriteSec, pTLInfo->ulFlushSec, pTLInfo->ulValidFlushCnt);

                                DBG_printf("   ###Save MCU%d Trace,SecCnt %d ###\n", ulMcuId, pTLInfo->ulValidFlushCnt);
                            }
                        }

                    }
                    else
                    {
                        FAE_SaveFile(pFileTL[ulMcuId - 1], pReadbuf, ulSecCnt * 512);
                        DBG_printf("   ###Save MCU%d Trace,SecCnt %d ###\n", ulMcuId, ulSecCnt);
                    }

                    ulReceivedTraceSecCnt[ulMcuId - 1] += ulSecCnt;
                }
                else
                {
                    if (TRUE == Is_All_Log_Trace_Read(ucSubsysId))
                    {
                        break;
                    }
                }

                ucSubsysPu++;
                if (ucSubsysPu >= SUBSYSTEM_PU_NUM)
                {
                    ucSubsysPu = 0;
                }
            }
        }
        free(pBBT[ucSubsysId]);
        free(pRTBuf[ucSubsysId]);
    }

    /* close file */
    for (ulMcuId = 0; ulMcuId < MCU_MAX; ulMcuId++)
    {
        DBG_printf("###MCUID %d TotalPageRead 0x%x ReceivedTraceSecCnt 0x%x done!###\n",
            ulMcuId, ulTotalPageRead[ulMcuId], ulReceivedTraceSecCnt[ulMcuId]);

        FAE_CloseFile(pFileTL[ulMcuId]);

        /* Decode trace */
        memset(fileName, 0, 128);
        sprintf_s(fileName, 128, "%s\\MCU%d_FlashTL", pReportFileDir, ulMcuId);
        if (ulMcuId == 0)
        {
            Decoder(pFormatFile0, fileName, pTraceLogHeaderFile, pReportFileDir, ulMcuId);
        }
        else
        {
            Decoder(pFormatFile12, fileName, pTraceLogHeaderFile, pReportFileDir, ulMcuId);
        }
    }

    free(pMcu0SrcBuf);
    free(pMcu0DstBuf);

    return;
}

void Api_Ext_ChangeTraceRecord(PDEVICE_OBJECT pDevObj)
{
    U32 ulInput;

    printf("Please input 0/1 to Disable/Enable: ");
    scanf_s("%u", &ulInput);

    if (0 == ulInput)
    {
        Api_Ext_TraceLogControl(pDevObj, MCU0, TL_CTL_DISABLE, 0);
    }
    else if (1 == ulInput)
    {
        Api_Ext_TraceLogControl(pDevObj, MCU0, TL_CTL_ENABLE, 0);
    }
    else if (ulInput >= 2)
    {
        DBG_printf("ucEnable 0x%x Wrong, valid value is 0 or 1\n", ulInput);
        return;
    }

    DBG_printf("Api_Ext_ChangeTraceRecord done!\n");

    return;
}
STATUS Api_Ext_Get_Pu_Msk(PDEVICE_OBJECT pDevObj, U32 * ulPuMskLow, U32 * ulPuMskHigh, U8 *ucLogicPuCnt)
{
    U8 ucSubSysCnt, ucPuCnt, ucPuTotal;

    *ulPuMskLow = *ulPuMskHigh = 0;
    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
    ucPuCnt = pDevObj->VarTable.SubSysTable[0].ucPuNum;

    ucPuTotal = ucSubSysCnt*ucPuCnt;

    if (ucPuTotal <= 32)
    {
        *ulPuMskLow = ((1 << ucPuTotal) - 1);
        *ulPuMskHigh = 0;
    }
    else
    {
        *ulPuMskLow = ((1 << 32) - 1);
        *ulPuMskHigh = ((1 << (ucPuTotal - 32)) - 1);
    }

    *ucLogicPuCnt = ucPuTotal;
    return RETURN_SUCCESS;
}
STATUS Api_Ext_Get_SubSys_Pu(PDEVICE_OBJECT pDevObj, U8 ucLogicPu, U8 *ucSubSys, U8 *ucSubSysPuNum)
{
    *ucSubSysPuNum = pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu].ucPuInSubSys;
    *ucSubSys = pDevObj->FlashInfo.LogicPuInfoEntry[ucLogicPu].ucSubFlashSysId;

    return RETURN_SUCCESS;
}

/* HwDebug Trace related functions, added by blakezhang */
void Api_Ext_HwDebug_Show_HCT_FCQ(FILE* HwDebugFile, HCT_FCQ_WBQ *FCQ)
{
    fprintf(HwDebugFile, "\nFCQ:\n");
    fprintf(HwDebugFile, "ID:%#0x\n", FCQ->bsID);
    fprintf(HwDebugFile, "IDB:%#0x\n", FCQ->bsIDB);
    fprintf(HwDebugFile, "SN:%#0x\n", FCQ->bsSN);
    fprintf(HwDebugFile, "Offset:%#0x\n", FCQ->bsOffset);
    fprintf(HwDebugFile, "HostAddrLow:%#0x\n", FCQ->ulHostAddrLow);
    fprintf(HwDebugFile, "HostAddrHigh:%#0x\n", FCQ->ulHostAddrHigh);
    fprintf(HwDebugFile, "NST:%#0x\n", FCQ->bsNST);
    fprintf(HwDebugFile, "CST:%#0x\n", FCQ->bsCST);
    fprintf(HwDebugFile, "Last:%#0x\n", FCQ->bsLast);
    fprintf(HwDebugFile, "Length:%#0x\n", FCQ->bsLength);
    fprintf(HwDebugFile, "Update:%#0x\n", FCQ->bsUpdate);
    fprintf(HwDebugFile, "Check:%#0x\n", FCQ->bsCheck);

}
void Api_Ext_HwDebug_Show_HCT_WBQ(FILE* HwDebugFile, HCT_FCQ_WBQ *WBQ)
{
    fprintf(HwDebugFile, "\nWBQ:\n");
    fprintf(HwDebugFile, "ID:%#0x\n", WBQ->bsID);
    fprintf(HwDebugFile, "IDB:%#0x\n", WBQ->bsIDB);
    fprintf(HwDebugFile, "SN:%#0x\n", WBQ->bsSN);
    fprintf(HwDebugFile, "Offset:%#0x\n", WBQ->bsOffset);
    fprintf(HwDebugFile, "HostAddrLow:%#0x\n", WBQ->ulHostAddrLow);
    fprintf(HwDebugFile, "HostAddrHigh:%#0x\n", WBQ->ulHostAddrHigh);
    fprintf(HwDebugFile, "NST:%#0x\n", WBQ->bsNST);
    fprintf(HwDebugFile, "CST:%#0x\n", WBQ->bsCST);
    fprintf(HwDebugFile, "ClrCI:%#0x\n", WBQ->bsClrCI);
    fprintf(HwDebugFile, "ClrSACT:%#0x\n", WBQ->bsClrSACT);
    fprintf(HwDebugFile, "UpdCCS:%#0x\n", WBQ->bsUpdCCS);
    fprintf(HwDebugFile, "AsstIntr:%#0x\n", WBQ->bsAsstIntr);
    fprintf(HwDebugFile, "IntrType:%#0x\n", WBQ->bsIntrType);
    fprintf(HwDebugFile, "RegFirst:%#0x\n", WBQ->bsRegFirst);
    fprintf(HwDebugFile, "Last:%#0x\n", WBQ->bsLast);
    fprintf(HwDebugFile, "Length:%#0x\n", WBQ->bsLength);
    fprintf(HwDebugFile, "Wait:%#0x\n", WBQ->bsWait);
    fprintf(HwDebugFile, "Update:%#0x\n", WBQ->bsUpdate);
    fprintf(HwDebugFile, "Check:%#0x\n", WBQ->bsCheck);

}

void Api_Ext_HwDebug_Show_SGE_Entry(FILE*HwDebugFile, SGE_ENTRY *SgeEntry)
{
    fprintf(HwDebugFile, "\nSGE Entry:\n");
    fprintf(HwDebugFile, "DsgPtr:%#0x\n", SgeEntry->bsDsgPtr);
    fprintf(HwDebugFile, "WriteEn:%#0x\n", SgeEntry->bsWriteEn);
    fprintf(HwDebugFile, "Type:%#0x\n", SgeEntry->bsType);
    fprintf(HwDebugFile, "Meta:%#0x\n", SgeEntry->bsMeta);
    fprintf(HwDebugFile, "Res:%#0x\n", SgeEntry->Res);
    fprintf(HwDebugFile, "HsgPtr:%#0x\n", SgeEntry->bsHsgPtr);
    fprintf(HwDebugFile, "HID:%#0x\n", SgeEntry->bsHID);
}

void Api_Ext_HwDebug_Show_HSG_Entry(FILE *HwDebugFile, HSG_ENTRY *HsgEntry)
{
    fprintf(HwDebugFile, "\nHSG Entry:\n");
    fprintf(HwDebugFile, "NextHsgId:%#0x\n", HsgEntry->bsNextHsgId);
    fprintf(HwDebugFile, "Last:%#0x\n", HsgEntry->bsLast);
    fprintf(HwDebugFile, "Length:%#0x\n", HsgEntry->bsLength);
    fprintf(HwDebugFile, "HostAddrLow:%#0x\n", HsgEntry->ulHostAddrLow);
    fprintf(HwDebugFile, "HostAddrHigh:%#0x\n", HsgEntry->ulHostAddrHigh);
    fprintf(HwDebugFile, "LBA:%#0x\n", HsgEntry->ulLBA);
}

void Api_Ext_HwDebug_Show_DSG_Entry(FILE *HwDebugFile, NORMAL_DSG_ENTRY *DsgEntry)
{
    fprintf(HwDebugFile, "\nDSG Entry:\n");
    fprintf(HwDebugFile, "NextDsgId:%#0x\n", DsgEntry->bsNextDsgId);
    fprintf(HwDebugFile, "bLast:%#0x\n", DsgEntry->bsLast);
    fprintf(HwDebugFile, "XferByteLen:%#0x\n", DsgEntry->bsXferByteLen);
    fprintf(HwDebugFile, "DramAddr:%#0x\n", DsgEntry->ulDramAddr);
    fprintf(HwDebugFile, "CacheStatusAddr:%#0x\n", DsgEntry->bsCacheStatusAddr);
    fprintf(HwDebugFile, "CsInDram", DsgEntry->bsCsInDram);
    fprintf(HwDebugFile, "CacheStsData:%#0x\n", DsgEntry->bsCacheStsData);
    fprintf(HwDebugFile, "bCacheStsEn:%#0x\n", DsgEntry->bsCacheStsEn);
    fprintf(HwDebugFile, "BuffMapId:%#0x\n", DsgEntry->bsBuffMapId);
    fprintf(HwDebugFile, "MapSecLen:%#0x\n", DsgEntry->bsMapSecLen);
    fprintf(HwDebugFile, "MapSecOff:%#0x\n", DsgEntry->bsMapSecOff);

}

void Api_Ext_HwDebug_Show_NfcqEntryDw0(FILE *HwDebugFile, NF_CQ_ENTRY_DW0 NfcqEntryDW0)
{
    fprintf(HwDebugFile, "\nNfcqEntryDw0->OntfEn:%#0x\n", NfcqEntryDW0.OntfEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->DmabyteEn:%#0x\n", NfcqEntryDW0.DmabyteEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->IntEn:%#0x\n", NfcqEntryDW0.IntEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->PuEnpMsk:%#0x\n", NfcqEntryDW0.PuEnpMsk);
    fprintf(HwDebugFile, "NfcqEntryDw0->PuEccMsk:%#0x\n", NfcqEntryDW0.PuEccMsk);
    fprintf(HwDebugFile, "NfcqEntryDw0->PuLdpcEn:%#0x\n", NfcqEntryDW0.PuLdpcEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->RedEn:%#0x\n", NfcqEntryDW0.RedEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->RedOnly:%#0x\n", NfcqEntryDW0.RedOnly);
    fprintf(HwDebugFile, "NfcqEntryDw0->Ssu0En:%#0x\n", NfcqEntryDW0.Ssu0En);
    fprintf(HwDebugFile, "NfcqEntryDw0->Ssu1En:%#0x\n", NfcqEntryDW0.Ssu1En);
    fprintf(HwDebugFile, "NfcqEntryDw0->TrigOmEn:%#0x\n", NfcqEntryDW0.TrigOmEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->DsgEn:%#0x\n", NfcqEntryDW0.DsgEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->InjEn:%#0x\n", NfcqEntryDW0.InjEn);
    fprintf(HwDebugFile, "NfcqEntryDw0->ErrTypeS:%#0x\n", NfcqEntryDW0.ErrTypeS);
    fprintf(HwDebugFile, "NfcqEntryDw0->NcqMode:%#0x\n", NfcqEntryDW0.NcqMode);
    fprintf(HwDebugFile, "NfcqEntryDw0->NcqNum:%#0x\n", NfcqEntryDW0.NcqNum);
    fprintf(HwDebugFile, "NfcqEntryDw0->N1En:%#0x\n", NfcqEntryDW0.N1En);
    fprintf(HwDebugFile, "NfcqEntryDw0->FstDsgPtr:%#0x\n", NfcqEntryDW0.FstDsgPtr);
}


void Api_Ext_HwDebug_Show_NFCQ_Entry(FILE *HwDebugFile, NF_CQ_ENTRY *NfCqEntry)
{
    U32 Nfcq_Sec_Count;
    U8 i;
    fprintf(HwDebugFile, "\n---NF_CQ Entry:---\n");
    /*This part is used to clear NF_CQ_ENTRY, shouldn't be reported.
    for(i=0;i<NFCQ_ENTRY_DW;i++)
    {
    fprintf(HwDebugFile,"mNfcqEntry[%u]:%#0x\n",i,NfCqEntry->mNfcqEntry[i]);
    }
    */
    Api_Ext_HwDebug_Show_NfcqEntryDw0(HwDebugFile, NfCqEntry->NfcqEntryDW0);
    //mode judge
    if (NfCqEntry->NfcqEntryDW0.DmabyteEn)
    {
        fprintf(HwDebugFile, "Byte Mode:\n");
        fprintf(HwDebugFile, "ByteAddr:%#0x\n", NfCqEntry->ByteAddr);
        fprintf(HwDebugFile, "ByteLength:%#0x\n", NfCqEntry->ByteLength);
        //fprintf(HwDebugFile,"ByteRev:%#0x\n",NfCqEntry->ByteRev1);	

    }
    else
    {
        for (Nfcq_Sec_Count = 0; Nfcq_Sec_Count < NFCQ_SEC_ADDR_COUNT; Nfcq_Sec_Count++)
        {
            fprintf(HwDebugFile, "SecLength:%#0x\n", NfCqEntry->SecAddr[Nfcq_Sec_Count].SecLength);
            fprintf(HwDebugFile, "SecStart:%#0x\n", NfCqEntry->SecAddr[Nfcq_Sec_Count].SecStart);
        }
    }

    fprintf(HwDebugFile, "DmaTotalLength:%#0x\n", NfCqEntry->DmaTotalLength);
    fprintf(HwDebugFile, "RedAddr:%#0x\n", NfCqEntry->RedAddr);
    fprintf(HwDebugFile, "BmEn:%#0x\n", NfCqEntry->BmEn);
    fprintf(HwDebugFile, "FstDataRdy:%#0x\n", NfCqEntry->FstDataRdy);
    fprintf(HwDebugFile, "Rsv3:%#0x\n", NfCqEntry->Rsv3);
    for (i = 0; i < 8; i++)
    {
        fprintf(HwDebugFile, "ScrambleSeed[%u]:%#0x\n", i, NfCqEntry->ScrambleSeed[i]);
    }

    fprintf(HwDebugFile, "SsuAddr0:%#0x\n", NfCqEntry->SsuAddr0);
    fprintf(HwDebugFile, "SsuData0:%#0x\n", NfCqEntry->SsuData0);
    fprintf(HwDebugFile, "rev6:%#0x\n", NfCqEntry->rev6);
    fprintf(HwDebugFile, "SsuAddr1:%#0x\n", NfCqEntry->SsuAddr1);
    fprintf(HwDebugFile, "SsuData1:%#0x\n", NfCqEntry->SsuData1);
    fprintf(HwDebugFile, "rev7:%#0x\n", NfCqEntry->rev7);
    for (i = 0; i < 8; i++)
    {
        fprintf(HwDebugFile, "RowAddr[%u]:%#0x\n", i, NfCqEntry->RowAddr[i]);
    }
}

/*------------------------------------------------------------------------------
Name:Show_HCT_Info
Description:
report and record HCT Information of one tag
Input Param:
FILE *HwDebugFile:file pointer of the record file
TRACE_HCT_INFO * HCT_Info:pointer which points SGE Information of one tag
Output Param:
none
Return Value:
void
Usage:
analysis function call this function when need to record HCT_Info
History:
20140820    ZihengChen   create function
------------------------------------------------------------------------------*/
void Api_Ext_HwDebug_Show_HCT_Info(FILE *HwDebugFile, TRACE_HCT_INFO * HCT_Info)
{
    U32 Index;
    U32 EntryOffset;
    HCT_DESC *HctDesc;
    HCT_FCQ_WBQ *FCQ_WBQ;

    fprintf(HwDebugFile, "\n<<HCT Information>>\n");
    fprintf(HwDebugFile, "Total Number of HCT Descriptor: %u.\n", HCT_Info->ulHctDescIndex);
    for (Index = 0; Index < (HCT_Info->ulHctDescIndex); Index++)
    {
        HctDesc = &(HCT_Info->aHctDesc[Index]);
        EntryOffset = HctDesc->ulFcqWbqEntryOffset;
        FCQ_WBQ = (HCT_FCQ_WBQ*)&HCT_Info->ucFcqWbqBuff[EntryOffset];//Look out for disorder
        //record HCT descriptor by type
        switch (HctDesc->eEntryType)
        {
        case RCD_FCQ:
            fprintf(HwDebugFile, "\n---HCT Descriptor%u(FCQ)---:\n", Index);
            fprintf(HwDebugFile, "usEntryId:%#0x\n", HctDesc->usEntryId);
            Api_Ext_HwDebug_Show_HCT_FCQ(HwDebugFile, FCQ_WBQ);
            break;
        case RCD_WBQ:
            fprintf(HwDebugFile, "\n---HCT Descriptor%u(WCQ)---:\n", Index);
            fprintf(HwDebugFile, "usEntryId:%#0x\n", HctDesc->usEntryId);
            Api_Ext_HwDebug_Show_HCT_WBQ(HwDebugFile, FCQ_WBQ);
            break;
        default:
            fprintf(HwDebugFile, "\n---HCT Descriptor%u(Unknown Entry Type)---:\n", Index);
            break;
        }
    }
}

/*------------------------------------------------------------------------------
Name:Show_SGE_Info
Description:
report and record SGE Information of one tag
Input Param:
FILE *HwDebugFile:file pointer of the record file
TRACE_SGE_INFO* SGE_Info:pointer which points SGE Information of one tag
Output Param:
none
Return Value:
void
Usage:
analysis function call this function when need to record SGE_Info
History:
20140820    ZihengChen   create function
------------------------------------------------------------------------------*/
void Api_Ext_HwDebug_Show_SGE_Info(FILE *HwDebugFile, TRACE_SGE_INFO* SGE_Info)
{
    U32 Index;
    U32 EntryOffset = 0;
    SGE_DESC *SgeDesc;
    SGE_ENTRY *SgeEntry;
    HSG_ENTRY *HsgEntry;
    NORMAL_DSG_ENTRY *DsgEntry;
    NF_CQ_ENTRY * NfCqEntry;

    fprintf(HwDebugFile, "\n<<SGE information>>\n");
    fprintf(HwDebugFile, "Total Number of SGE Descriptor: %u.\n", SGE_Info->ulSgeDescIndex);
    if ((SGE_Info->bAllSgeChainBuilt) == TRUE)
    {
        fprintf(HwDebugFile, "FW has finished building SGE chain!\n");
    }
    else
    {
        fprintf(HwDebugFile, "FW hasn't finished building SGE chain!\n");
    }

    if (SGE_Info->bAllNfcChainBuilt == TRUE)
        fprintf(HwDebugFile, "FW has finished building NFC on-the-fly program chain!\n");
    else
        fprintf(HwDebugFile, "FW hasn't finished building NFC on-the-fly program chain!\n");

    fprintf(HwDebugFile, "Total number of SGE chain:%u\n", SGE_Info->usSgeTotalChain);
    fprintf(HwDebugFile, "Total number of NFC on-the-fly chain:%u\n", SGE_Info->usNfcTotalChain);

    for (Index = 0; Index < (SGE_Info->ulSgeDescIndex); Index++)
    {
        SgeDesc = &(SGE_Info->aSgeDesc[Index]);
        //record HCT descriptor by type
        switch (SgeDesc->eEntryType)
        {
        case RCD_DRQ:
            fprintf(HwDebugFile, "\n---SGE Descriptor%u(DRQ)---:\n", Index);
            break;
        case RCD_DWQ:
            fprintf(HwDebugFile, "\n---SGE Descriptor%u(DWQ)---:\n", Index);
            break;
        case RCD_SGQ_R:
            fprintf(HwDebugFile, "\n---SGE Descriptor%u(SGQ_R)---:\n", Index);
            break;
        case RCD_SGQ_W:
            fprintf(HwDebugFile, "\n---SGE Descriptor%u(SGQ_W)---:\n", Index);
            break;
        default:
            fprintf(HwDebugFile, "\n---SGE Descriptor%u(Unknown Entry Type!)---:\n", Index);
            break;
        }

        fprintf(HwDebugFile, "usEntryId:%#0x\n", SgeDesc->usEntryId);
        SgeEntry = &SgeDesc->tSgeEntry;
        Api_Ext_HwDebug_Show_SGE_Entry(HwDebugFile, SgeEntry);
        EntryOffset = SgeDesc->ulHsgEntryOffset;
        do
        {
            HsgEntry = (HSG_ENTRY*)&(SGE_Info->ucEntryBuff[EntryOffset]);
            Api_Ext_HwDebug_Show_HSG_Entry(HwDebugFile, HsgEntry);
            EntryOffset += sizeof(HSG_ENTRY);
            if (HsgEntry->bsLast == TRUE)
            {
                break;
            }
        } while (1);

        if (SgeDesc->eEntryType == RCD_DRQ || SgeDesc->eEntryType == RCD_DWQ)
        {

            EntryOffset = SgeDesc->ulDsgEntryOffset;
            do
            {
                DsgEntry = (NORMAL_DSG_ENTRY *)&SGE_Info->ucEntryBuff[EntryOffset];
                Api_Ext_HwDebug_Show_DSG_Entry(HwDebugFile, DsgEntry);
                EntryOffset += sizeof(NORMAL_DSG_ENTRY);
                if (TRUE == DsgEntry->bsLast)
                {
                    break;
                }
            } while (1);
        }
        else
        {
            EntryOffset = SgeDesc->ulNfcqEntryOffset;
            NfCqEntry = (NF_CQ_ENTRY*)&SGE_Info->ucEntryBuff[EntryOffset];
            Api_Ext_HwDebug_Show_NFCQ_Entry(HwDebugFile, NfCqEntry);
        }
    }
}

STATUS Api_Ext_HwDebug_Log_Decode(PDEVICE_OBJECT pDevObj, char *pReportFileDir)
{
    U8 ucTag;
    U8 SubSystem_Index;
    U8 ucSubSysCnt;
    TRACE_HCT_INFO *TraceHctInfo_De;
    TRACE_SGE_INFO *TraceSgeInfo_De;
    TRACE_TAG_INFO *TraceTagInfo_De;
    U32 ulHwTraceBaseAddr;
    U32 ulHwTraceBaseSize;
    U8* pBuffer;
    HW_DEBUG_INFO *DebugInfo;


    FILE* HwDebugFile;
    SYSTEMTIME sys1;
    char Report_Name[128];

    GetLocalTime(&sys1);
    memset(Report_Name, 0, sizeof(Report_Name));
    sprintf(Report_Name, "%s\\HwDebugReport_%4u-%02u-%02u_%02u_%02u_%02u.txt", pReportFileDir, sys1.wYear, sys1.wMonth, sys1.wDay, sys1.wHour, sys1.wMinute, sys1.wSecond);
    HwDebugFile = fopen(Report_Name, "a");

    if (HwDebugFile == NULL)
    {
        printf("Api_Ext_HwDebug_Log_Decode Creat File Failed!\n");
        fclose(HwDebugFile);
        return RETURN_FAIL;
    }

    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
    ulHwTraceBaseAddr = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ulHwTraceBaseAddr;
    ulHwTraceBaseSize = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ulHwTraceBaseSize;

    pBuffer = (U8 *)malloc(ulHwTraceBaseSize + 512);
    memset(pBuffer, 0, ulHwTraceBaseSize + 512);

    Api_Read_Dram_Sram(pDevObj, MCU0_ID, ulHwTraceBaseAddr, ulHwTraceBaseSize, (U8 *)pBuffer);

    DebugInfo = (HW_DEBUG_INFO*)pBuffer;

    for (ucTag = 0; ucTag < 32; ucTag++)
    {
        TraceTagInfo_De = &(DebugInfo->aTraceTagInfo[ucTag]);

        fprintf(HwDebugFile, "\n**********Tag[%u]**********\n", ucTag);

        TraceHctInfo_De = &(TraceTagInfo_De->tTraceHctInfo);
        if ((TraceHctInfo_De->ulHctDescIndex) > 0)
        {
            Api_Ext_HwDebug_Show_HCT_Info(HwDebugFile, TraceHctInfo_De);//
        }

        for (SubSystem_Index = 0; SubSystem_Index < ucSubSysCnt; SubSystem_Index++)
        {
            TraceSgeInfo_De = &(TraceTagInfo_De->aTraceSgeInfo[SubSystem_Index]);
            if ((TraceSgeInfo_De->ulSgeDescIndex) > 0)
            {
                fprintf(HwDebugFile, "\n||-- SubSystem %u--||\n", SubSystem_Index);
                Api_Ext_HwDebug_Show_SGE_Info(HwDebugFile, TraceSgeInfo_De);
            }
        }
    }

    printf("HwDebug Trace Decoded successfully!\n");
	free(pBuffer);
    fclose(HwDebugFile);
    HwDebugFile = NULL;

    return RETURN_SUCCESS;
}

STATUS Api_Ext_HwDebug_Log_Decode_Bin(PDEVICE_OBJECT pDevObj, char *pReportFileDir)
{
    U8 ucTag;
    U8 SubSystem_Index;
    U8 ucSubSysCnt;
    TRACE_HCT_INFO *TraceHctInfo_De;
    TRACE_SGE_INFO *TraceSgeInfo_De;
    TRACE_TAG_INFO *TraceTagInfo_De;
    U32 real_read_bytes;
    U8* pBuffer;
    HW_DEBUG_INFO *DebugInfo;

    FILE* HwDebugFile;
    FILE* SourceFile;
    SYSTEMTIME sys1;
    char Report_Name[128];

    SourceFile = fopen(".\\HwDebug.bin", "rb");
    if (NULL == SourceFile)
    {
        printf("Open File HwDebug.bin Fail, please check your file and its name !\n");
        fclose(SourceFile);
        return RETURN_FAIL;
    }

    GetLocalTime(&sys1);
    memset(Report_Name, 0, sizeof(Report_Name));
    sprintf(Report_Name, "%s\\HwDebugReport_%4u-%02u-%02u_%02u_%02u_%02u.txt", pReportFileDir, sys1.wYear, sys1.wMonth, sys1.wDay, sys1.wHour, sys1.wMinute, sys1.wSecond);
    HwDebugFile = fopen(Report_Name, "a");

    if (NULL == HwDebugFile)
    {
        printf("Api_Ext_HwDebug_Log_Decode Creat File Failed!\n");
        fclose(HwDebugFile);
        fclose(SourceFile);
        return RETURN_FAIL;
    }

    ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;

    pBuffer = (U8 *)malloc(sizeof(HW_DEBUG_INFO) + 512);
    memset(pBuffer, 0, sizeof(HW_DEBUG_INFO) + 512);

    real_read_bytes = fread(pBuffer, 1, sizeof(HW_DEBUG_INFO), SourceFile);

    if (real_read_bytes != sizeof(HW_DEBUG_INFO))
    {
        printf("Read Source File Failed!\n");
        fclose(HwDebugFile);
        fclose(SourceFile);
        free(pBuffer);
        return RETURN_FAIL;
    }

    DebugInfo = (HW_DEBUG_INFO*)pBuffer;

    for (ucTag = 0; ucTag < 32; ucTag++)
    {
        TraceTagInfo_De = &(DebugInfo->aTraceTagInfo[ucTag]);

        fprintf(HwDebugFile, "\n**********Tag[%u]**********\n", ucTag);

        TraceHctInfo_De = &(TraceTagInfo_De->tTraceHctInfo);
        if ((TraceHctInfo_De->ulHctDescIndex) > 0)
        {
            Api_Ext_HwDebug_Show_HCT_Info(HwDebugFile, TraceHctInfo_De);//
        }

        for (SubSystem_Index = 0; SubSystem_Index < ucSubSysCnt; SubSystem_Index++)
        {
            TraceSgeInfo_De = &(TraceTagInfo_De->aTraceSgeInfo[SubSystem_Index]);
            if ((TraceSgeInfo_De->ulSgeDescIndex) > 0)
            {
                fprintf(HwDebugFile, "\n||-- SubSystem %u--||\n", SubSystem_Index);
                Api_Ext_HwDebug_Show_SGE_Info(HwDebugFile, TraceSgeInfo_De);
            }
        }
    }

    printf("HwDebug Trace Decoded successfully!\n");

    fclose(HwDebugFile);
    fclose(SourceFile);
    free(pBuffer);
    HwDebugFile = NULL;

    return RETURN_SUCCESS;
}

void Api_Ext_Read_Flash_Addr(PDEVICE_OBJECT pDevObj, char* pReportFileDir)
{
    U32 PhyBlk, PhyPage;
    U32 ulLogicPuMsk;
    U8* pReadbuf = NULL;
    U32 ucSubsysPu, ucLogicPu;
    U32 ucPlnMode = 0xff;
    char fileName[128];
    FILE* pFile;
    U32 ulStatus;
    RED* pRed = NULL;
    U32 ulMcuId;
    U32 ulPgSize;

    DBG_printf("Please Input McuId(MCU0:1,MCU1:2,MCU2:3):\n");
    scanf("%d", &ulMcuId);
    if (ulMcuId != MCU1_ID && ulMcuId != MCU2_ID)
    {
        DBG_printf("Invalid McuID!\n");
        DBG_Getch();
    }

    DBG_printf("Please Input ucSubsysPu:\n");
    scanf("%d", &ucSubsysPu);

    if (ucSubsysPu >= g_ulSubsystemPuNum)
    {
        DBG_printf("ucSubsysPu %d is invalid,max is %d\n", ucSubsysPu, g_ulSubsystemPuNum);
        return;
    }

    DBG_printf("Please Input PhyBlk:\n");
    scanf("%d", &PhyBlk);
    if (PhyBlk >= (gBlkPerPln + gRsvdBlkPerCE))
    {
        DBG_printf("PhyBlk %d is invalid,max is %d\n", PhyBlk, (gBlkPerPln + gRsvdBlkPerCE));
        return;
    }

    DBG_printf("Please Input PhyPage:\n");
    scanf("%d", &PhyPage);
    if (PhyPage >= gPagePerBlock)
    {
        DBG_printf("PhyBlk %d is invalid,max is %d\n", PhyBlk, gPagePerBlock);
        return;
    }

    DBG_printf("Please Input PlnMode(SinglePln:0,MultiPln:1):\n");
    scanf("%d", &ucPlnMode);
    if (0 != ucPlnMode && 1 != ucPlnMode)
    {
        DBG_printf("ucPlnMode %d is invalid\n", ucPlnMode);
        return;
    }

    ucPlnMode = (ucPlnMode == 0) ? (0) : (MULTI_PLN);

    ulLogicPuMsk = 0;
    ucLogicPu = ucSubsysPu + (ulMcuId - 2) * SUBSYSTEM_PU_NUM;

    Api_Ext_AddPuToMsk(ucLogicPu, &ulLogicPuMsk);

    Api_Read_Flash(pDevObj, ucPlnMode, PhyBlk, PhyPage, ulLogicPuMsk, 0);
    ulStatus = Api_Flash_Get_Status_By_Pu(pDevObj, ucLogicPu);
    if (NF_SUCCESS != ulStatus)
    {
        DBG_printf("Api_Ext_Read_Flash_Addr Read Pu 0x%x Block 0x%x Page 0x%x Status 0x%x ERROR!\n",
            ucLogicPu, PhyBlk, PhyPage, ulStatus);
        return;
    }
    else
    {
        Api_Flash_Get_Red_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, (RED**)&pRed);
        Api_Flash_Get_Data_Addr_By_Pu(pDevObj, ucLogicPu, ucPlnMode, &pReadbuf);

        ulPgSize = (ucPlnMode == MULTI_PLN) ? (PAGE_SIZE) : PHY_PG_SZ;
        memset(fileName, 0, 128);
        sprintf_s(fileName, "%s\\MCU%d_SubSysPu%d_PhyBlk%d_PhyPage%d.bin", pReportFileDir, ulMcuId - 1, ucSubsysPu, PhyBlk, PhyPage);
        pFile = FAE_OpenFile(fileName);
        FAE_SaveFile(pFile, pReadbuf, ulPgSize);
        FAE_CloseFile(pFile);

        memset(fileName, 0, 128);
        sprintf_s(fileName, "%s\\MCU%d_SubSysPu%d_PhyBlk%d_PhyPage%d_Red.bin", pReportFileDir, ulMcuId - 1, ucSubsysPu, PhyBlk, PhyPage);
        pFile = FAE_OpenFile(fileName);
        FAE_SaveFile(pFile, (U8*)pRed, sizeof(RED));
        FAE_CloseFile(pFile);

        DBG_printf("Read Pu 0x%x Block 0x%x Page 0x%x Success!\n", ucLogicPu, PhyBlk, PhyPage);
    }
    return;
}

void Api_Ext_Read_Fw_Runtime_Info(PDEVICE_OBJECT pDevObj)
{
    U8 smartbuf[4096];
    U8 ucFwVersion[16], ucBlVersion[16];
    FW_RUNTIME_INFO tFirmwareInfo; 
    SMART_INFO tSmartInfo;
    
    if (RETURN_FAIL == Api_Get_SmartInfo(pDevObj, smartbuf))
    {
        return;
    }
     
    memcpy(&tSmartInfo, smartbuf, 512);  
    tFirmwareInfo = tSmartInfo.FWRuntimeInfo;

    //Transport FW, BL release version
    Api_DwordToString(tFirmwareInfo.FWVersionInfo.ulFWRleaseVerion, ucFwVersion);
    Api_DwordToString(tFirmwareInfo.BLVersionInfo.ulFWRleaseVerion, ucBlVersion);

    printf("=====FW Version Info=====\n");
    if (FW_VERSION_INFO_MCU_TYPE_B0 == tFirmwareInfo.FWVersionInfo.ucMCUType)
    {
        printf("MCU Core Type: B0\n");
    }
    else if (FW_VERSION_INFO_MCU_TYPE_C0 == tFirmwareInfo.FWVersionInfo.ucMCUType)
    {
        printf("MCU Core Type: C0\n");
    }
    else
    {
        printf("MCU Core Type: None\n");
    }
    
    if (FW_VERSION_INFO_HOST_TYPE_SATA == tFirmwareInfo.FWVersionInfo.ucHostType)
    {
        printf("Host Type: SATA\n");
    }
    else if (FW_VERSION_INFO_HOST_TYPE_AHCI == tFirmwareInfo.FWVersionInfo.ucHostType)
    {
        printf("Host Type: AHCI\n");
    }
    else if (FW_VERSION_INFO_HOST_TYPE_NVME == tFirmwareInfo.FWVersionInfo.ucHostType)
    {
        printf("Host Type: NVME\n");
    }
    else
    {
        printf("Host Type: None\n");
    }

    if (FW_VERSION_INFO_FLASH_TYPE_L85 == tFirmwareInfo.FWVersionInfo.ucFlashType)
    {
        printf("Flash Type: L85\n");
    }
    else if (FW_VERSION_INFO_FLASH_TYPE_L95 == tFirmwareInfo.FWVersionInfo.ucFlashType)
    {
        printf("Flash Type: L95\n");
    }
    else if (FW_VERSION_INFO_FLASH_TYPE_A19 == tFirmwareInfo.FWVersionInfo.ucFlashType)
    {
        printf("Flash Type: A19\n");
    }
    else if (FW_VERSION_INFO_FLASH_TYPE_A19_4PLN == tFirmwareInfo.FWVersionInfo.ucFlashType)
    {
        printf("Flash Type: A19_4PLN\n");
    }
    else
    {
        printf("Flash Type: None\n");
    }
    
    printf("FW release version: %s\n", ucFwVersion);
    printf("GIT release version: %x\n", tFirmwareInfo.FWVersionInfo.ulGITVersion);
    printf("Compile Date: ");
    Api_ShowDate(tFirmwareInfo.FWVersionInfo.ulDateInfo);

    printf("Compile Time: ");
    Api_ShowTime(tFirmwareInfo.FWVersionInfo.ulTimeInfo);
    printf("\n");

    printf("=====Bootloader Version Info=====\n");
    printf("BL release version: %s\n", ucBlVersion);
    printf("GIT release version: %x\n", tFirmwareInfo.BLVersionInfo.ulGITVersion);
    printf("Compile Date: ");
    Api_ShowDate(tFirmwareInfo.BLVersionInfo.ulDateInfo);

    printf("Compile Time: ");
    Api_ShowTime(tFirmwareInfo.BLVersionInfo.ulTimeInfo);
    printf("\n");

    printf("=====Flash Detail info=====\n");
    printf("Pln Num: %d\n", tFirmwareInfo.ucPlnNum);
    printf("LUN Num: %d\n", tFirmwareInfo.ucLunNum);
    printf("Physical Page Size: %dKB\n", tFirmwareInfo.usPhyPageSize/1024);
    printf("Flash ID: 0x%x, 0x%x\n", tFirmwareInfo.ulFlashId[0], tFirmwareInfo.ulFlashId[1]);
    printf("Physical CE Map: 0x%x, 0x%x\n", tFirmwareInfo.ulPhyCeMap[0], tFirmwareInfo.ulPhyCeMap[1]);
    printf("\n");

    printf("=====Disk Config info=====\n");
    printf("CE count: %d\n", tFirmwareInfo.ucCeCount);
    printf("MCU count: %d\n", tFirmwareInfo.ucMcuCount);
    printf("DRAM size: %dMB\n", tFirmwareInfo.ulDRAMSize/1024/1024);

	return;
}
