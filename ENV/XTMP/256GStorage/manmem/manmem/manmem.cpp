// manmem.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
int _tmain(int argc, _TCHAR* argv[])
{
	int nArgNum ;
	HANDLE hEve;
	int quit;
	DWORD dwError;
    int i;
	char* pMap ;
		SECURITY_ATTRIBUTES attr;
	PSECURITY_DESCRIPTOR pSec;
	unsigned long ulIndex = 0;
	HANDLE hFileMapping;
	char* strShareMemName;
	char* strConsoleTitle;
	char* strShareMemSize;
	unsigned long ulSize;
	int nLen = 0;
	int nArgIndex = 0;
	nArgNum = argc;
	strConsoleTitle = (char*)malloc(128);
	memset(strConsoleTitle, 0, 128);
	GetConsoleTitle(strConsoleTitle, 128);
   // scanf("%d", &i);
	
	//hEve = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\Eve");
	//if(hEve == NULL)
	//{
	//	DWORD dwError = GetLastError();
	//	printf("heve == NULL, %d\n", dwError);
		//DBG_Getch();DBG_Getch();
	//}

	for(nArgIndex = 0; nArgIndex < argc; nArgIndex++)
	{
		//printf("%d", nArgIndex);
		if(strcmp(argv[nArgIndex], "-N") == 0)
		{
			nLen = strlen(argv[nArgIndex + 1]);
			strShareMemName = (char*)malloc(nLen + 1);
			ZeroMemory(strShareMemName, nLen + 1);
			strcpy(strShareMemName, argv[++nArgIndex]);
		}
		else if(strcmp(argv[nArgIndex], "-S") == 0)
		{
			nLen = strlen(argv[nArgIndex + 1]);
			strShareMemSize = (char*)malloc(nLen + 1);
			ZeroMemory(strShareMemSize, nLen + 1);
			strcpy(strShareMemSize, argv[++nArgIndex]);
			break;
		}
	}

	ulSize = atoi(strShareMemSize);
	printf("%s, %u\n", strShareMemName, ulSize / 1024 / 1024);
	strcat(strConsoleTitle, strShareMemName);
	SetConsoleTitle(strConsoleTitle);
pSec = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(!pSec)
	{
		return GetLastError();
	}
	if(!InitializeSecurityDescriptor(pSec, SECURITY_DESCRIPTOR_REVISION))
	{
		LocalFree(pSec);
		return GetLastError();
	}
	if(!SetSecurityDescriptorDacl(pSec, TRUE, NULL, TRUE))
	{
		LocalFree(pSec);
		return GetLastError();
	}
	
	attr.bInheritHandle = FALSE;
	attr.lpSecurityDescriptor = pSec;
	attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,NULL, PAGE_READWRITE, 0, ulSize,
		strShareMemName);
	LocalFree(pSec);
	if(hFileMapping == NULL)
	{
		printf("create file mapping failed\n");
		//DBG_Getch();DBG_Getch();
	}
	pMap = (char*)MapViewOfFile(
			hFileMapping,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			ulSize
			);

	//SetEvent(hEve);
    printf("success\n");
	scanf("%d", &quit);

	if(quit == 1)
	{
		
		dwError = GetLastError();
		if(pMap == NULL)
		{
			printf("mapping error, error code = %d\n", dwError);
			return 0;
		}
		printf("the data type string is %s\n", strShareMemName);
		//printf("%s", pMap);
		for(ulIndex = 0; ulIndex < ulSize; ulIndex++)
		{
			printf("%d ",pMap[ulIndex]); 
			if(((ulIndex + 1) % 8) == 0)
				printf("\n");
		}
	}
	printf("end process");
	while(1)
	{
	}
	//CopyMemory((PVOID)pFirst, szMsg, strlen(szMsg));
	return 0;
}

