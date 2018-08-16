
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "BaseDef.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <windows.h>
#include <ctype.h>

HANDLE hFile[4];


#define TRACE_PATH ".\\input"
WIN32_FIND_DATA FindFileData;
HANDLE hFind;
BOOL bFinish;

//#define BLK_PER_PLN  548

/*Physics block information table*/
typedef union _PBIT_ENTRY
{
    U32 Value[2];
    struct
    {
        U32 VirtualBlockAddr : 16;
        U32 EraseCnt : 16;

        U32 bFree : 1;
        U32 bError : 1;
        U32 bAllocated : 1;
        U32 bBackup : 1;        //should be recovery when full revovery
        U32 bTable : 1;
        U32 BlockType : 4;
        U32 bPatrolRead : 1;
        U32 bWL : 1;
        U32 bLock : 1;
        U32 bTLC : 1;             // 1: TLC, 0:MLC or SLC
        U32 bBroken : 1;
        U32 bWeak : 1;  // Weak block, marked by l3 when normal read recc or retry-success, checked by l2 when petrol-read or gc-src-block-selection.- added by jasonguo 20150817
        U32 bRetryFail : 1;
        U32 bReserved : 1;
        U32 bRsv : 15;
    };
}PBIT_ENTRY;

void FileNameGetInit()
{
    char sFormatFileName[256];
    sprintf(sFormatFileName, "%s\\*.*", TRACE_PATH);
    hFind = FindFirstFile(sFormatFileName, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("input path check, find no file!\n");
        _getch();
    }
    bFinish = FALSE;
}

void GetFileName(char *pFileName)
{
    while (1)
    {
        if (bFinish == TRUE)
        {
            *pFileName = '!';
            break;
        }
        if (!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
        {
            sprintf(pFileName, "%s\\%s", TRACE_PATH, FindFileData.cFileName);
        }
        else
        {
            *pFileName = '@';
        }

        if (!FindNextFile(hFind, &FindFileData))
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                bFinish = TRUE;
            }
            else
            {
                printf("File Find Error!\n");
                _getch();
            }
        }
        if (*pFileName != '@')
            break;
    }

}

void PBITBinParser(char* file_name)
{
    U8* pBuffer;
    PBIT_ENTRY* pPBIT;
    FILE *fpexcel = NULL;
    char dst_fileName[256] = "";
    char temp_fileName[128] = "";
    HANDLE hfile_in;
    U32 read_bytes, real_read_bytes;
    U32  ret;
    U32 usBlock,usTotalBlkCnt;

    hfile_in = CreateFile(file_name, // file to open
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
        NULL);                      // no attr. template
    if (INVALID_HANDLE_VALUE == hfile_in)
    {
        printf("Create file failed\n");
        system("pause");
    }

    pBuffer = (U8 *)malloc(sizeof(PBIT_ENTRY));
    read_bytes = sizeof(PBIT_ENTRY);

    memcpy(temp_fileName, file_name + strlen(".\\input\\"), strlen(file_name) - strlen(".\\input\\") - strlen(".bin"));

    sprintf(dst_fileName, ".\\output\\%s.xls", temp_fileName);
    fopen_s(&fpexcel, dst_fileName, "wb+");
    if (fpexcel == NULL)
    {
        return;
    }

    fprintf(fpexcel, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "Virtual Block", "EraseCnt", "bFree", "bError", "bTLC", "bAllocated", "bBackup", "bTable", "BlockType", "bWL", "bLock","bWeak","bRetryFail");

    while (1)
    {
        ret = ReadFile(hfile_in, pBuffer, read_bytes, &real_read_bytes, NULL);

        usTotalBlkCnt = real_read_bytes / (sizeof(PBIT_ENTRY));

        if (0 != (real_read_bytes % (sizeof(PBIT_ENTRY))))
        {
            printf("warning,your input bin is not PBIT_ENTRY align\n");
            _getch();
        }

        pPBIT = (PBIT_ENTRY*)pBuffer;

        for (usBlock = 0; usBlock < usTotalBlkCnt; usBlock++)
        {
            fprintf(fpexcel, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                pPBIT[usBlock].VirtualBlockAddr,
                pPBIT[usBlock].EraseCnt,
                pPBIT[usBlock].bFree,
                pPBIT[usBlock].bError,
                pPBIT[usBlock].bTLC,
                pPBIT[usBlock].bAllocated,
                pPBIT[usBlock].bBackup,
                pPBIT[usBlock].bTable,
                pPBIT[usBlock].BlockType,
                pPBIT[usBlock].bWL,
                pPBIT[usBlock].bLock,
                pPBIT[usBlock].bWeak,
                pPBIT[usBlock].bRetryFail);
        }

        if (ret)
        {
            if (real_read_bytes == 0)
            {
                break;
            }
        }
        else
        {
            printf("ReadFile failed:%d\n", GetLastError());
            system("pause");
        }
    }
    free(pBuffer);
    fclose(fpexcel);

    return;
}


int main(void)
{
    char file_name[128];

    FileNameGetInit();
    while (1)
    {
        GetFileName(file_name);

        if ((file_name[0] != '@') && (file_name[0] != '!'))
        {
            printf("File Name: %s\n", file_name);
            PBITBinParser(file_name);
        }
        else
        {
            printf("all input bin parser done\n");
            break;
        }
    }

    system("pause");

    return 0;
}

