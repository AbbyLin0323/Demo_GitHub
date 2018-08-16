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
  File Name     : host_api_misc.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the log trace functions of host access to device.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/

#include "logtrace_parser.h"

UINT32 TransferChartoNum(char *pStart, char *pEnd)
{
    UINT32 Num = 0;
    int i = 0;
    while (pStart + i != pEnd)
    {
        Num = Num * 10 + (*(pStart + i)) - 48;
        i++;
    }
    return Num;
}
int SplitFormatInformation(char *pHead, char *pEnd, TRACEINFORMATION *pTraceInformation, int Num)
{
    int j = 0;
    char *pStart = NULL;
    char Signal = '"';
    UINT32 Data = 0;

    if (5 == Num)
    {
        //用户备注部分，特殊处理
        pStart = strchr(pHead, Signal);
        pStart++;
        //将引号中内容取出存放，若引号内为空则不作存储
        for (j = 0; pEnd - 1 != (pStart + j); j++)
        {
            pTraceInformation->NoteInfo[j] = pStart[j];
        }
    }
    else if ((3 == Num) || (4 == Num))
    {
        return 0;
    }
    else
    {
        pStart = pHead;
        while ((' ' == *pStart) || ('\r' == *pStart) || ('\n' == *pStart))
        {
            pStart++;
        }
        Data = TransferChartoNum(pStart, pEnd);
        switch (Num)
        {
        case 0:
            pTraceInformation->FileNum = Data;
            break;
        case 1:
            pTraceInformation->LineNum = Data;
            break;
        case 2:
            //DataSize是以bit为单位的
            pTraceInformation->DataSize = Data;
            break;
        default:
            break;
        }
    }
    return 0;
}
void SplitGetMember(char *pHead, MEMBERINFORMATION **ppFirstMemberInformation, char *pEnd)
{
    char *pTemp = NULL;
    char *pStart = pHead;
    char Signal = ',';
    MEMBERINFORMATION *pNowMember = NULL;
    MEMBERINFORMATION **ppBeforeMember = ppFirstMemberInformation;
    int i = 0;
    UINT StartBit = 0;

    while (pStart != pEnd)
    {
        pNowMember = (MEMBERINFORMATION *)malloc(sizeof(MEMBERINFORMATION));
        memset(pNowMember, 0, sizeof(MEMBERINFORMATION));

        //变量类型
        pTemp = strchr(pStart, Signal);
        while (' ' == *pStart)
        {
            pStart++;
        }
        for (i = 0; (pStart + i) != pTemp; i++)
        {
            pNowMember->MemberType[i] = pStart[i];
        }
        pStart = pTemp + 1;

        //变量名字
        pTemp = strchr(pStart, Signal);
        while (' ' == *pStart)
        {
            pStart++;
        }
        for (i = 0; (pStart + i) != pTemp; i++)
        {
            pNowMember->MemberName[i] = pStart[i];
        }
        pStart = pTemp + 1;

        //变量大小
        //此时pTemp指向最后一个逗号
        pTemp = strchr(pStart, Signal);
        while (' ' == *pStart)
        {
            pStart++;
        }
        //MemberSize是以bit为单位的，因为存在以bit为单位获取数据的情况
        for (i = 0; (pStart + i) != pTemp; i++)
        {
            pNowMember->MemberSize = pNowMember->MemberSize * 10 + int(pStart[i]) - 48;
        }
        pStart = pTemp + 1;

        if ((0 == strcmp(pNowMember->MemberType, TYPE_LOW_BOOL)) || (0 == strcmp(pNowMember->MemberType, TYPE_UP_BOOL))
            || (0 == strcmp(pNowMember->MemberType, TYPE_BYTE)))
        {
            pNowMember->ShouldbeSize = 8;
        }
        else if (0 == strcmp(pNowMember->MemberType, TYPE_CHAR))
        {
            pNowMember->ShouldbeSize = 8;
        }
        else if (0 == strcmp(pNowMember->MemberType, TYPE_UNSIGNED_CHAR) || (0 == strcmp(pNowMember->MemberType, TYPE_UBYTE)))
        {
            pNowMember->ShouldbeSize = 8;
        }
        else if ((0 == strcmp(pNowMember->MemberType, TYPE_SHORT)) || (0 == strcmp(pNowMember->MemberType, TYPE_WORD)))
        {
            pNowMember->ShouldbeSize = 16;
        }
        else if ((0 == strcmp(pNowMember->MemberType, TYPE_UNSIGNED_SHORT)) || (0 == strcmp(pNowMember->MemberType, TYPE_UWORD)))
        {
            pNowMember->ShouldbeSize = 16;
        }
        else if ((0 == strcmp(pNowMember->MemberType, TYPE_INT)) || (0 == strcmp(pNowMember->MemberType, TYPE_DWORD)))
        {
            pNowMember->ShouldbeSize = 32;
        }
        else if ((0 == strcmp(pNowMember->MemberType, TYPE_UNSIGNED_INT)) || (0 == strcmp(pNowMember->MemberType, TYPE_UDWORD))
            || (0 == strcmp(pNowMember->MemberType, TYPE_PTR)))
        {
            pNowMember->ShouldbeSize = 32;
        }
        else if (0 == strcmp(pNowMember->MemberType, TYPE_LONG))
        {
            pNowMember->ShouldbeSize = 32;
        }
        else if (0 == strcmp(pNowMember->MemberType, TYPE_UNSIGNED_LONG))
        {
            pNowMember->ShouldbeSize = 32;
        }
        else if ((0 == strcmp(pNowMember->MemberType, TYPE_LONGLONG)) || (0 == strcmp(pNowMember->MemberType, TYPE_QWORD)))
        {
            pNowMember->ShouldbeSize = 64;
        }
        else if (0 == strcmp(pNowMember->MemberType, TYPE_UNSIGNED_LONGLONG))
        {
            pNowMember->ShouldbeSize = 64;
        }
        else
        {
            printf("Error: member %s get a zero size!\n", pNowMember->MemberType);
        }

        if (pNowMember->MemberSize < pNowMember->ShouldbeSize)
        {
            //printf("---TRUE---\n");
            pNowMember->SameWord = TRUE;
            pNowMember->StartBit = StartBit;
            StartBit += pNowMember->MemberSize;
            if (StartBit >= pNowMember->ShouldbeSize)
            {
                StartBit = 0;
                //printf("continued bit size word\n");
            }
        }
        else
        {
            //printf("---FALSE---\n");
            pNowMember->SameWord = FALSE;
            pNowMember->StartBit = 0;
            StartBit = 0;
        }

        pNowMember->pNextMember = NULL;
        if (NULL == (*ppBeforeMember))
        {
            *ppBeforeMember = pNowMember;
        }
        else
        {
            (*ppBeforeMember)->pNextMember = pNowMember;
            ppBeforeMember = &((*ppBeforeMember)->pNextMember);
        }
    }

    return;
}
TRACEINFORMATION *GetFormatInformation(char *pFormatFile)
{
    FILE *pFilePointer;
    char *pFinalReadFormatInformation = NULL;
    char Signal1 = ',', Signal2 = '"';
    char *pSignal3 = "!@#$";
    char *pTemp = NULL, *pStart = NULL, *pInfoStart = NULL, *pEnd = NULL;
    int i = 0, ReadTime = 0, FileLen = 0;
    int FlagFirst = 1;
    TRACEINFORMATION *pNowTraceInformation = NULL;
    TRACEINFORMATION *pHeadTraceInformation = NULL;
    TRACEINFORMATION *pTraceInformation = NULL;

    if (0 != fopen_s(&pFilePointer, pFormatFile, "rb"))
    {
        printf("Format file open error!\n");
        return NULL;
    }

    fseek(pFilePointer, 0, SEEK_END);
    FileLen = ftell(pFilePointer);
    fseek(pFilePointer, 0, SEEK_SET);
    pFinalReadFormatInformation = (char *)malloc(FileLen);
    memset(pFinalReadFormatInformation, 0, FileLen);
    fread(pFinalReadFormatInformation, FileLen, 1, pFilePointer);
    pStart = pFinalReadFormatInformation;

    while (1)
    {
        pEnd = strstr(pStart, pSignal3);
        if (NULL == pEnd)
        {
            break;
        }

        //获取format信息
        pTraceInformation = (TRACEINFORMATION *)malloc(sizeof(TRACEINFORMATION));
        if (NULL == pTraceInformation)
        {
            printf("malloc memory for Trace Information failed!\n");
            return NULL;
        }
        memset(pTraceInformation, 0, sizeof(TRACEINFORMATION));

        for (i = 0; i < 5; i++)
        {
            pTemp = strchr(pStart, Signal1);
            SplitFormatInformation(pStart, pTemp, pTraceInformation, i);
            pStart = pTemp + 1;
        }
        pInfoStart = pStart;

        //找到起始和终止引号
        while (1)
        {
            pTemp = strchr(pInfoStart, Signal2);
            if ((pTemp > pEnd) || (pTemp == NULL))
            {
                pTemp = pInfoStart;
                break;
            }
            else
            {
                pInfoStart = pTemp + 1;
            }
        }

        SplitFormatInformation(pStart, pTemp, pTraceInformation, i);
        pStart = pTemp + 1;
        if ('\0' != *pStart)
        {
            SplitGetMember(pStart, &(pTraceInformation->pMember), pEnd);
        }
        if (1 == FlagFirst)
        {
            pHeadTraceInformation = pTraceInformation;
            pNowTraceInformation = pTraceInformation;
            pNowTraceInformation->pNextTRACEINFORMATION = NULL;
            FlagFirst = 0;
        }
        else
        {
            pNowTraceInformation->pNextTRACEINFORMATION = pTraceInformation;
            pNowTraceInformation = pTraceInformation;
            pNowTraceInformation->pNextTRACEINFORMATION = NULL;
        }

        pStart = pEnd + 4;
    }

    free(pFinalReadFormatInformation);

    return pHeadTraceInformation;
}
UINT32 GetValueforMembers(TRACEINFORMATION *pTraceInformation, char *pValue)
{
    MEMBERINFORMATION *pNowMember = pTraceInformation->pMember;
    UINT32 TotalSize = 0;
    UINT32 ValueOffset = 0;
    UINT32 WordNowSize = 0;

    if (NULL == pNowMember)
    {
        printf("Member is Null!\n");
        return pTraceInformation->DataSize / 8;
    }

    while (1)
    {
        if (((FALSE == pNowMember->SameWord) && (0 != WordNowSize))
            || ((TRUE == pNowMember->SameWord) && (0 == pNowMember->StartBit)))
        {
            ValueOffset += WordNowSize / 8;
        }
        TotalSize += pNowMember->MemberSize;
        if (TotalSize <= pTraceInformation->DataSize)
        {
            //printf("should be size:%d\n",(pNowMember->ShouldbeSize/8));
            if (0 == (ValueOffset % (pNowMember->ShouldbeSize / 8)))
            {
                pNowMember->pMemberValue = (void *)(pValue + ValueOffset);
                //printf("ValueOffset:%d\n",ValueOffset);
            }
            else
            {
                pNowMember->pMemberValue = (void *)(pValue + ValueOffset + pNowMember->ShouldbeSize / 8 - ValueOffset % (pNowMember->ShouldbeSize / 8));
                //printf("ValueOffset:%d\n",ValueOffset + pNowMember->ShouldbeSize/8 - ValueOffset % (pNowMember->ShouldbeSize/8));
            }
        }
        else
        {
            printf("Error with Value getting!\n");
            break;
        }

        if (TRUE == pNowMember->SameWord)
        {
            if (0 == pNowMember->StartBit)
            {
                WordNowSize = 0;
                WordNowSize += pNowMember->MemberSize;
            }
            else
            {
                WordNowSize += pNowMember->MemberSize;
            }
        }
        else
        {
            WordNowSize = 0;
            ValueOffset += pNowMember->MemberSize / 8;
        }

        if (NULL != pNowMember->pNextMember)
        {
            pNowMember = pNowMember->pNextMember;
        }
        else
        {
            break;
        }
    }
    //printf("add:%d\n",pTraceInformation->DataSize/8);
    return pTraceInformation->DataSize / 8;
}
UINT32 GetOneDataBlock(FILE **ppDataFilePointer, char **ppData,
    UINT32 DecodeCycle)
{
    FILE *pFilePointer = *ppDataFilePointer;
    char *pData = NULL;

    pData = (char *)malloc(DATABLOCK_SIZE);//在process中释放了
    if (NULL == pData)
    {
        printf("malloc Data block failed!\n");
        return 1;
    }

    *ppData = pData;

    memset(pData, 0, DATABLOCK_SIZE);

    if (0 != feof(pFilePointer))
    {
        free(pData);
        pData = NULL;
        printf("Error:no more sector!\n");
        return 1;
    }

    //确定文件偏移
    fseek(pFilePointer, DecodeCycle*SECTOR_SIZE, SEEK_SET);

    //按照固定格式读取一个sector
    fread(pData, DATABLOCK_SIZE, 1, pFilePointer);

    if (0 != ferror(pFilePointer))
    {
        free(pData);
        pData = NULL;
        printf("Read dat file sector error!\n");
        return 1;
    }

    return 0;
}
void GetOneReportFileName(char *pReportFileDir, char *pReportFileName, char mcuid)
{
    SYSTEMTIME SysTime;
    char *pFileBasicName = "TraceLogReport";

    GetLocalTime(&SysTime);
    memset(pReportFileName, 0, MAXBYTE);
    sprintf_s(pReportFileName, MAXBYTE, "%s\\%s_%s_%u_%4u_%02u_%02u_%02u_%02u_%02u.txt", pReportFileDir, pFileBasicName, "MCU", mcuid, SysTime.wYear, SysTime.wMonth,
        SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond);

    return;
}
void GenerateReport(TRACEINFORMATION *pTraceInformation, char pFileNames[MAXBYTE][MAXBYTE / 4],
    FILE *fp)
{
    UINT8 u8BitOffset = 0;
    UINT16 u16BitOffset = 0;
    UINT32 u32BitOffset = 0;
    UINT64 u64BitOffset = 0;
    UINT32 uCount = 0;
    UINT64 TempleValue = 0;
    MEMBERINFORMATION *pNowMember = pTraceInformation->pMember;

    char *pMemberName = NULL;
    char *pMemberType = NULL;

    if ('\0' != pTraceInformation->NoteInfo[0])
    {
        fprintf(fp, "%s %4d %s : ", pFileNames[pTraceInformation->FileNum + 1], pTraceInformation->LineNum
            , pTraceInformation->NoteInfo);
    }
    else
    {
        fprintf(fp, "%s %4d ", pFileNames[pTraceInformation->FileNum + 1], pTraceInformation->LineNum);
    }
    while (NULL != pNowMember)
    {
        pMemberName = pNowMember->MemberName;
        pMemberType = pNowMember->MemberType;
        memcpy(&TempleValue, pNowMember->pMemberValue, 8);
        if ((0 == strcmp(pMemberType, TYPE_LOW_BOOL)) || (0 == strcmp(pMemberType, TYPE_UP_BOOL)))
        {
            fprintf(fp, "%s = 0x%x ", pMemberName, *(INT8 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_BYTE))
        {
            fprintf(fp, "%s = 0x%x ", pMemberName, *(INT8 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UBYTE))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%x ", pMemberName, *(UINT8 *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u8BitOffset += VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT8 *)(&TempleValue))&u8BitOffset)
                    >> pNowMember->StartBit);
                u8BitOffset = 0;
            }
        }
        else if (0 == strcmp(pMemberType, TYPE_CHAR))
        {
            fprintf(fp, "%s = %c ", pMemberName, *(CHAR *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UNSIGNED_CHAR))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%x ", pMemberName, *(UCHAR *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u8BitOffset += VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT8 *)(&TempleValue))&u8BitOffset)
                    >> pNowMember->StartBit);
                u8BitOffset = 0;
            }
        }
        else if ((0 == strcmp(pMemberType, TYPE_SHORT)) || (0 == strcmp(pMemberType, TYPE_WORD)))
        {
            fprintf(fp, "%s = 0x%x ", pMemberName, *(INT16 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UNSIGNED_SHORT) || (0 == strcmp(pMemberType, TYPE_UWORD)))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%x ", pMemberName, *(UINT16 *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u16BitOffset += VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT16 *)(&TempleValue))&u16BitOffset)
                    >> pNowMember->StartBit);
                u16BitOffset = 0;
            }
        }
        else if ((0 == strcmp(pMemberType, TYPE_INT)) || (0 == strcmp(pMemberType, TYPE_WORD)))
        {
            fprintf(fp, "%s = 0x%x ", pMemberName, *(INT32 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UNSIGNED_INT) || (0 == strcmp(pMemberType, TYPE_UDWORD))
            || (0 == strcmp(pMemberType, TYPE_PTR)))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%x ", pMemberName, *(UINT32 *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u32BitOffset += VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT32 *)(&TempleValue))&u32BitOffset)
                    >> pNowMember->StartBit);
                u32BitOffset = 0;
            }
        }
        else if (0 == strcmp(pMemberType, TYPE_LONG))
        {
            fprintf(fp, "%s = 0x%x ", pMemberName, *(INT32 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UNSIGNED_LONG))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%x ", pMemberName, *(UINT32 *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u32BitOffset += VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT32 *)(&TempleValue))&u32BitOffset)
                    >> pNowMember->StartBit);
                u32BitOffset = 0;
            }
        }
        else if (0 == strcmp(pMemberType, TYPE_LONGLONG))
        {
            fprintf(fp, "%s = 0x%lx ", pMemberName, *(INT64 *)(&TempleValue));
        }
        else if (0 == strcmp(pMemberType, TYPE_UNSIGNED_LONGLONG) || (0 == strcmp(pMemberType, TYPE_QWORD)))
        {
            if (FALSE == pNowMember->SameWord)
            {
                fprintf(fp, "%s = 0x%lx ", pMemberName, *(UINT64 *)(&TempleValue));
            }
            else
            {
                for (uCount = 0; uCount < pNowMember->MemberSize; uCount++)
                {
                    u64BitOffset += (UINT64)VALUE_ONE << (pNowMember->StartBit + uCount);
                }
                fprintf(fp, "%s = 0x%x ", pMemberName, ((*(UINT64 *)(&TempleValue))&u64BitOffset)
                    >> pNowMember->StartBit);
                u64BitOffset = 0;
            }
        }
        else
        {
            printf("Warning:This variable did nothing in report!\n");
        }
        pNowMember = pNowMember->pNextMember;
    }
    fprintf(fp, "\n");
    return;
}
void ReleaseMemory(TRACEINFORMATION *pDictoryTraceInformation, FILE *pDataFilePointer, FILE *fp)
{
    TRACEINFORMATION *pNowTraceInformation = pDictoryTraceInformation;
    TRACEINFORMATION *pNextTraceInformation = NULL;
    MEMBERINFORMATION *pNowMember = NULL;
    MEMBERINFORMATION *pNextMember = NULL;

    printf("Release memory ...\n");
    while (NULL != pNowTraceInformation)
    {
        pNowMember = pNowTraceInformation->pMember;
        while (NULL != pNowMember)
        {
            pNextMember = pNowMember->pNextMember;
            free(pNowMember);
            pNowMember = pNextMember;
        }
        pNextTraceInformation = pNowTraceInformation->pNextTRACEINFORMATION;
        free(pNowTraceInformation);
        pNowTraceInformation = pNextTraceInformation;
    }

    if (NULL != pDataFilePointer)
    {
        fclose(pDataFilePointer);
        pDataFilePointer = NULL;
    }

    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }
    return;
}
int GetFileNames(char *pTraceLogHeaderFile, char pFileNames[MAXBYTE][MAXBYTE / 4])
{
    char aTemp[MAXBYTE] = { 0 };
    char *pStartSignal = "//start of name files";
    char *pEndSignal = "}C_FILE_NAME_ENUM;";
    FILE *pFilePointer;
    int i = 0, j = 0, k = 0;

    if (0 != fopen_s(&pFilePointer, pTraceLogHeaderFile, "rb"))
    {
        return -1;
    }

    //找到开始点后再偏移一行才是文件名的开始
    while (1)
    {
        fgets(aTemp, MAXBYTE, pFilePointer);
        if (NULL != strstr(aTemp, pStartSignal))
        {
            fgets(aTemp, MAXBYTE, pFilePointer);
            break;
        }
        memset(aTemp, 0, MAXBYTE);
    }

    //按行读取文件名，去掉空格和逗号
    while (1)
    {
        fgets(pFileNames[i], MAXBYTE / 4, pFilePointer);
        for (j = 0, k = 0; '\0' != pFileNames[i][j]; j++)
        {
            if ((' ' != pFileNames[i][j]) && (',' != pFileNames[i][j]) && ('\n' != pFileNames[i][j])
                && ('\t' != pFileNames[i][j]) && ('\r' != pFileNames[i][j]))
            {
                pFileNames[i][k] = pFileNames[i][j];
                k++;
            }
        }
        for (; '\0' != pFileNames[i][k]; k++)
        {
            pFileNames[i][k] = '\0';
        }
        if (NULL != strstr(pFileNames[i], pEndSignal))
        {
            for (j = 0; '\0' != pFileNames[i][j]; j++)
            {
                pFileNames[i][j] = '\0';
            }
            break;
        }
        i++;
    }

    return 0;
}
UINT32 OpenandGetSizeofDataFile(char *pDataFileName, FILE **ppDataFilePointer)
{
    UINT32 SizeofDataFile = 0;

    if (0 != fopen_s(ppDataFilePointer, pDataFileName, "rb"))
    {
        printf("Dat file open error!\n");
        return 0;
    }

    fseek(*ppDataFilePointer, 0, SEEK_END);
    SizeofDataFile = ftell(*ppDataFilePointer);
    fseek(*ppDataFilePointer, 0, SEEK_SET);

    return SizeofDataFile / SECTOR_SIZE;
}
TRACEINFORMATION *GetRightDictory(TRACEINFORMATION *pHeadTraceInformation, char *pData, UINT32 *pOffset, TRACEINFORMATION **ppCacheDictory)
{
    UINT32 FileNum = 0;
    UINT32 LineNum = 0;
    UINT32 TempleLogHead = 0;
    UINT32 Offset = *pOffset;

    TRACEINFORMATION *pNowTraceInformation = pHeadTraceInformation;
    TRACEINFORMATION *pTempCache = *ppCacheDictory;

    memcpy((void *)&TempleLogHead, (void *)(pData + Offset), 4);

    //printf("offset:%d\n",Offset);
    FileNum = ((TL_LOG_HEADER *)&TempleLogHead)->bsFileNum;
    //printf("FileNum:%d\n",FileNum);
    LineNum = ((TL_LOG_HEADER *)&TempleLogHead)->bsLineNum;
    //printf("LineNum:%d\n",LineNum);
    if (0 == LineNum)
    {
        printf("Line num is zero! ...\n");
        return NULL;
    }
    Offset += 4;
    if (NULL != pTempCache)
    {
        while (1)
        {
            if ((pTempCache->FileNum == FileNum) && (pTempCache->LineNum == LineNum))
            {
                Offset += GetValueforMembers(pTempCache, pData + Offset);
                //####测试代码3
                *pOffset = Offset;
                return pTempCache;
            }
            else
            {
                pTempCache = pTempCache->pNextTRACEINFORMATION;
                if (NULL == pTempCache)
                {
                    break;
                }
            }
        }
    }
    while (1)
    {
        if ((pNowTraceInformation->FileNum == FileNum) && (pNowTraceInformation->LineNum == LineNum))
        {
            pTempCache = (TRACEINFORMATION *)malloc(sizeof(TRACEINFORMATION));
            memcpy(pTempCache, pNowTraceInformation, sizeof(TRACEINFORMATION));
            pTempCache->pNextTRACEINFORMATION = *ppCacheDictory;
            *ppCacheDictory = pTempCache;
            Offset += GetValueforMembers(pNowTraceInformation, pData + Offset);
            //####测试代码3
            break;
        }
        else
        {
            pNowTraceInformation = pNowTraceInformation->pNextTRACEINFORMATION;
            if (NULL == pNowTraceInformation)
            {
                printf("Warning:No Dictory can matter!\n");
                break;
            }
        }
    }
    *pOffset = Offset;
    return pNowTraceInformation;
}
UINT32 ProcessOneDataBlock(char **ppData, char aFileNames[MAXBYTE][MAXBYTE / 4],
    FILE *fp, TRACEINFORMATION *pHeadTraceInformation, TRACEINFORMATION **ppCacheDictory)
{
    char *pData = *ppData;
    UINT32 Offset = 0;
    UINT16 TagHead = 0;
    UINT32 Cycle = 0;
    UINT32 AddLineNum = 0;

    TRACEINFORMATION *pTraceInformation = NULL;

    TagHead = ((TL_SEC_HEADER *)(pData + Offset))->usLogNum;
    Offset += 4;
    //printf("TagHead:%d\n",TagHead);

    for (Cycle = 0; Cycle < TagHead; Cycle++)
    {
        pTraceInformation = GetRightDictory(pHeadTraceInformation, pData, &Offset, ppCacheDictory);

        if (NULL == pTraceInformation)
        {
            printf("Error:this log will not generate a report!\n");
        }
        else
        {
            GenerateReport(pTraceInformation, aFileNames, fp);
            AddLineNum++;
        }
    }
    free(pData);

    return AddLineNum;
}

int Decoder(char *pFormatFile, char *pDataFileName, char *pTraceLogHeaderFile, char *pReportFileDir, char mcuid)
{
#if 0
    char *pFormatFile = "D:\\TraceLogTool\\Parser\\temple\\log_format_file.ini";
    char *pDataFileName = "D:\\TraceLogTool\\MCU0TL";
    char *pTraceLogHeaderFile = "D:\\TraceLogTool\\Parser\\temple\\firmware\\HAL_TraceLog.h";
    char *pReportFileDir = "D:\\TraceLogTool";
#endif

    TRACEINFORMATION *pDictoryTraceInformation = NULL;
    TRACEINFORMATION *pCacheDictory = NULL;
    char aFileNames[MAXBYTE][MAXBYTE / 4] = { 0 };
    UINT32 SizeofDataFileInHalfM = 0;
    UINT32 DecodeCycle = 0;
    char aReportFileName[MAXBYTE] = { 0 };
    UINT32 CurrentLineNum = 0;

    char *pData = NULL;
    FILE *pDataFilePointer = NULL;
    FILE *fp = NULL;

    printf("Decode log format file ...\n");
    pDictoryTraceInformation = GetFormatInformation(pFormatFile);

    if (NULL == pDictoryTraceInformation)
    {
        printf("DECODE FORMAT FILE FAILED!\n");
        return -1;
    }

    printf("Decode Trace log H file ...\n");
    if (0 != GetFileNames(pTraceLogHeaderFile, aFileNames))
    {
        printf("DECODE TRACE LOG H FILE FAILED!\n");
        ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp);
        return -1;
    }
    //#####测试代码1

    SizeofDataFileInHalfM = OpenandGetSizeofDataFile(pDataFileName, &pDataFilePointer);
    printf("sector num:%d\n", SizeofDataFileInHalfM);
    if (0 == SizeofDataFileInHalfM)
    {
        printf("Error:the size of Data is zero!\n");
        ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp);
        return -1;
    }
    else
    {
        printf("Start to decode data file and generate report , this might need a few minutes ...\n");
    }

    for (DecodeCycle = 0; DecodeCycle < SizeofDataFileInHalfM; DecodeCycle++)
    {
        if (1 == GetOneDataBlock(&pDataFilePointer, &pData, DecodeCycle))
        {
            printf("DECODE DAT FILE FAILED!\n");
            ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp);
            return -1;
        }

        if ('\0' == aReportFileName[0])
        {
            GetOneReportFileName(pReportFileDir, aReportFileName, mcuid);
            fopen_s(&fp, aReportFileName, "a");
            if (NULL == fp)
            {
                printf("Report file open failed!\n");
                ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp);
                return -1;
            }
        }

        if (CurrentLineNum >= ONEFILEMAXLINENUM)
        {
            if (NULL != fp)
            {
                fclose(fp);
            }
            GetOneReportFileName(pReportFileDir, aReportFileName, mcuid);
            fopen_s(&fp, aReportFileName, "a");
            CurrentLineNum = 0;
        }

        CurrentLineNum += ProcessOneDataBlock(&pData, aFileNames, fp, pDictoryTraceInformation, &pCacheDictory);

        //####测试代码2
    }

    ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp);

    printf("Decoder running ok!");
    return 0;
}

/*//format.file测试输出代码  #####1
TRACEINFORMATION *pNowT = pDictoryTraceInformation;
MEMBERINFORMATION *pNowM = pDictoryTraceInformation->pMember;

while(NULL != pNowT)
{
printf("%u %5d %5d %s : ",pNowT->FileNum,pNowT->LineNum,pNowT->DataSize,pNowT->NoteInfo);
pNowM = pNowT->pMember;
while(NULL != pNowM)
{
printf("%s %s %d %d",pNowM->MemberName,pNowM->MemberType,pNowM->MemberSize,pNowM->pMemberValue);
pNowM = pNowM->pNextMember;
}
printf("\n");
pNowT = pNowT->pNextTRACEINFORMATION;
}

//Trace log Header file 测试输出代码
for(int i = 0;'\0' != *aFileNames[i];i++)
{
printf("%s",aFileNames[i]);
}*/

/*//format.file测试输出代码 #####2
TRACEINFORMATION *pNowT = pRealTraceInformation;
MEMBERINFORMATION *pNowM = pRealTraceInformation->pMember;

while(NULL != pNowT)
{
printf("%3d %5d %5d %s : ",pNowT->FileNum,pNowT->LineNum,pNowT->DataSize,pNowT->NoteInfo);
pNowM = pNowT->pMember;
while(NULL != pNowM)
{
printf("%s %s %d %d",pNowM->MemberName,pNowM->MemberType,pNowM->MemberSize,pNowM->pMemberValue);
pNowM = pNowM->pNextMember;
}
printf("\n");
pNowT = pNowT->pNextTRACEINFORMATION;
}*/

/*//测试输出代码  #####3
MEMBERINFORMATION *pNowMember = NULL;
pNowMember = pNowTraceInformation->pMember;
while(NULL != pNowMember)
{
printf("%s %s %d %d\n",pNowMember->MemberName,pNowMember->MemberType,pNowMember->MemberSize,pNowMember->pMemberValue);
pNowMember = pNowMember->pNextMember;
}
if (NULL != pTempTraceInformation)
{
pNowMember = pTempTraceInformation->pMember;
while(NULL != pNowMember)
{
printf("relative %s %s %d %d\n",pNowMember->MemberName,pNowMember->MemberType,pNowMember->MemberSize,pNowMember->pMemberValue);
pNowMember = pNowMember->pNextMember;
}
}*/

/*按照size写文件
if ((CurrentSize >= REPORT_FILE_SIZE_MAX - 1024*1024) || (DecodeCycle == SizeofDataFileInHalfM - 1))
{
GetOneReportFileName(pReportFileDir, aReportFileName);
fopen_s(&fp, aReportFileName, "a");
if (NULL == fp)
{
printf("Report file open failed!\n");
ReleaseMemory(pDictoryTraceInformation, pDataFilePointer, fp, pTempleStore);
return -1;
}
fprintf(fp, pTempleStore);
fclose(fp);
fp = NULL;
CurrentSize = 0;
memset(pTempleStore, 0, REPORT_FILE_SIZE_MAX);
}
*/

