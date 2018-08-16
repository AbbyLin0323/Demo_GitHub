// DisExecutor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "stdlib.h"
#include <time.h>
#pragma comment (lib,"ws2_32")
#include "DisExecutor.h"

DRC_PROTOCOL_UNIT *pESendProtocolUnitMain;
DRC_PROTOCOL_UNIT *pESendProtocolUnitComm;
DRC_PROTOCOL_UNIT *pERecvProtocolUnitComm;
DRC_PROTOCOL_UNIT *pERecvProtocolUnitMain;

int RunningPhase = 0;

DWORD WINAPI CommunicationThread(LPVOID lpParam)
{
    sockaddr_in LocalAddr;
    sockaddr ControllerAddr;
    int ControllerAddrLen = sizeof(SOCKADDR);
    SOCKET ExecutorSock;
    u_long FAR Argp = 1;
    fd_set Readfd;
    INT32 lRet = 0;
    timeval SelectTimeOut;
    DRC_PROTOCOL_UNIT *pTempRecvProtocolUnit = NULL;
    DRC_PROTOCOL_UNIT *pTempSendProtocolUnit = NULL;
    int Sendflag = 1;
    int lSendRet = 0;
    UINT32 ulUnitNum = 0;

    SelectTimeOut.tv_sec=1;
    SelectTimeOut.tv_usec=0;

    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_port = htons(9000);
    LocalAddr.sin_addr.s_addr = INADDR_ANY;
    ExecutorSock = socket(AF_INET,SOCK_DGRAM,0);
    if (ExecutorSock < 0)
    {
        printf("Socket initialization failed!\n");
        return -1;
    }

    if (SOCKET_ERROR == bind(ExecutorSock,(SOCKADDR*)&LocalAddr,sizeof(SOCKADDR)))
    {
        printf("Bind failed with error code:%d\n",WSAGetLastError());
        return -1;
    }
    ioctlsocket(ExecutorSock,FIONBIO,&Argp);

    EXECUTOR_INFORMATION *pstExecutorInformation = (EXECUTOR_INFORMATION *)lpParam;

    while (1)
    {
        FD_ZERO(&Readfd);
        FD_SET(ExecutorSock, &Readfd);
        lRet = select(0, &Readfd, 0, 0, &SelectTimeOut);
    
        if (lRet > 0)
        {
            if(FD_ISSET(ExecutorSock,&Readfd))
            {
                pTempRecvProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                memset(pTempRecvProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                recvfrom(ExecutorSock,(char *)pTempRecvProtocolUnit,sizeof(DRC_PROTOCOL_UNIT),0,
                    &ControllerAddr,&ControllerAddrLen);
                //只处理协议报文
                if (ProtocolLabel == pTempRecvProtocolUnit->ulProtocolLabel)
                {
                    pTempRecvProtocolUnit->pNextProtocolUnit = NULL;//发送过来的这个字段非空
                    if ((NULL == pstExecutorInformation->pFirstControllerAddress) && (pTempRecvProtocolUnit->ucCmdCode == AssignId))
                    {
                        pstExecutorInformation->pFirstControllerAddress = &ControllerAddr;
                        if (NULL == pERecvProtocolUnitComm)
                        {
                            pERecvProtocolUnitComm = pTempRecvProtocolUnit;
                            pERecvProtocolUnitMain = pTempRecvProtocolUnit;
                        }
                        else
                        {
                            pERecvProtocolUnitComm->pNextProtocolUnit = pTempRecvProtocolUnit;
                            pERecvProtocolUnitComm = pTempRecvProtocolUnit;
                        }
                    }
                    else if (pstExecutorInformation->pFirstControllerAddress != NULL)
                    {
                        if ((0 == strcmp(pstExecutorInformation->pFirstControllerAddress->sa_data,ControllerAddr.sa_data)))
                        {
                            if (NULL == pERecvProtocolUnitComm)
                            {
                                pERecvProtocolUnitComm = pTempRecvProtocolUnit;
                                pERecvProtocolUnitMain = pTempRecvProtocolUnit;
                            }
                            else
                            {
                                pERecvProtocolUnitComm->pNextProtocolUnit = pTempRecvProtocolUnit;
                                pERecvProtocolUnitComm = pTempRecvProtocolUnit;
                            }
                        }
                    }
                }
                else
                {
                    free(pTempRecvProtocolUnit);
                }
            }
        }

        if (NULL != pESendProtocolUnitComm)
        {
            if ((Sendflag > 0) && (Sendflag < 3))
            {
                pESendProtocolUnitComm->ulUintNum = ulUnitNum++;
                lSendRet = sendto(ExecutorSock,(char *)pESendProtocolUnitComm,sizeof(DRC_PROTOCOL_UNIT),0,
                    pstExecutorInformation->pFirstControllerAddress,ControllerAddrLen);
                //printf("send unit num:%x\n",pESendProtocolUnitComm->ulUintNum);
                if (lSendRet == SOCKET_ERROR)
                {
                    //printf("Send to controller cmd code %d failed!!!\n",pESendProtocolUnitComm->ucCmdCode);
                    Sendflag++;
                }
                else
                {
                    Sendflag = 0;
                }
            }
            if ((NULL != pESendProtocolUnitComm->pNextProtocolUnit) && ((Sendflag == 0) || (Sendflag >= 3)))
            {
                Sendflag = 1;
                pTempSendProtocolUnit = pESendProtocolUnitComm;
                pESendProtocolUnitComm = pESendProtocolUnitComm->pNextProtocolUnit;
                free(pTempSendProtocolUnit);
            }
        }
    }
    return 0;
}
int CheckAllParameters(EXECUTOR_INFORMATION *pstExecutorInformation)
{
    DRC_PROTOCOL_UNIT *pTempProtocolUnit = NULL;
    int lRet = 0;
    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    pTempProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;

    if ((pstExecutorInformation->aTypeofWholeProject[0] == '\0') && (pstExecutorInformation->aControllerName[0] == '\0')
        && (pstExecutorInformation->aExeName[0] == '\0') && (pstExecutorInformation->aProjectName[0] == '\0')
		&& (pstExecutorInformation->aProjectPath[0] == '\0') && (pstExecutorInformation->aFlashType[0] == '\0')
		&& (pstExecutorInformation->aChecklistVersion[0] == '\0'))
    {
        pTempProtocolUnit->ucCmdCode = AllTheParas;
        pESendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
        pESendProtocolUnitMain = pTempProtocolUnit;
        lRet = -8;
        return lRet;
    }

    if (pstExecutorInformation->aTypeofWholeProject[0] == '\0')
    {
        printf("TypeofWholeProject is invalid!\n");
        pTempProtocolUnit->ucCmdCode = TypeofWholeProject;
        lRet = -1;
    }
    else if (pstExecutorInformation->aControllerName[0] == '\0')
    {
        printf("ControllerName is invalid!\n");
        pTempProtocolUnit->ucCmdCode = ControllerName;
        lRet = -2;
    }
    else if (pstExecutorInformation->aExeName[0] == '\0')
    {
        printf("ExeName is invalid!\n");
        pTempProtocolUnit->ucCmdCode = ExeName;
        lRet = -3;
    }
    else if (pstExecutorInformation->aProjectName[0] == '\0')
    {
        printf("ProjectName is invalid!\n");
        pTempProtocolUnit->ucCmdCode = ProjectName;
        lRet = -4;
    }
    else if (pstExecutorInformation->aProjectPath[0] == '\0')
    {
        printf("ProjectPath is invalid!\n");
        pTempProtocolUnit->ucCmdCode = ProjectPath;
        lRet = -5;
    }
	else if (pstExecutorInformation->aFlashType[0] == '\0')
	{
		printf("Flashtype is invalid!\n");
		pTempProtocolUnit->ucCmdCode = FlashType;
		lRet = -6;
	}
	else if (pstExecutorInformation->aChecklistVersion[0] == '\0')
	{
		printf("ChecklistVersion is invalid!\n");
		pTempProtocolUnit->ucCmdCode = ChecklistVersion;
		lRet = -7;
	}

    if (lRet != 0)
    {
        strcpy((char *)(pTempProtocolUnit->aMsgString),NotOk);
        pESendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
        pESendProtocolUnitMain = pTempProtocolUnit;
    }
    else
    {
        free(pTempProtocolUnit);
    }
    return lRet;
}
void PriorExecute(EXECUTOR_INFORMATION *pstExecutorInformation)
{
    char aPriorExecute[2*MAXBYTE] = {0};
    char *pFolderName = "\\running\\";
    char *pPriorBatName = "priorexecute.bat";
    char *pSpace = " ";
    char *pPriorExecuteOrder = NULL;

    if ((0 != strcmp(pstExecutorInformation->aControllerName,pstExecutorInformation->aHostName))
			&&(pstExecutorInformation->ucExecuteTimes == 0))
    {
        (void)strcat(aPriorExecute,pstExecutorInformation->aLocalPath);
        (void)strcat(aPriorExecute,pFolderName);
        (void)strcat(aPriorExecute,pPriorBatName);
        (void)strcat(aPriorExecute,pSpace);
        (void)strcat(aPriorExecute,pstExecutorInformation->aProjectPath);
        (void)strcat(aPriorExecute,pSpace);
        pPriorExecuteOrder = strcat(aPriorExecute,pstExecutorInformation->aNetDiskPath);

        printf("Prior:%s\n",pPriorExecuteOrder);
        system(pPriorExecuteOrder);

		pstExecutorInformation->ucExecuteTimes++;
    }

    return;
}
void Execute(EXECUTOR_INFORMATION *pstExecutorInformation)
{
    int i = 0;
    char aExecute[2*MAXBYTE] = {0};
    char *pFolderName = "\\running\\";
    char *pPriorBatName = "priorexecute.bat";
    char *pBatName = "execute.bat ";
    char *pSpace = " ";
    char *pExecuteOrder = NULL;
    
    (void)strcat(aExecute,pstExecutorInformation->aLocalPath);
    (void)strcat(aExecute,pFolderName);
    (void)strcat(aExecute,pBatName);
	(void)strcat(aExecute,pstExecutorInformation->aNetDiskPath);
	(void)strcat(aExecute,pSpace);
	(void)strcat(aExecute,pstExecutorInformation->aProjectPath);
	(void)strcat(aExecute,pSpace);
    (void)strcat(aExecute,pstExecutorInformation->aExeName);
    (void)strcat(aExecute,pSpace);
    (void)strcat(aExecute,pstExecutorInformation->aTypeofWholeProject);
    (void)strcat(aExecute,pSpace);
    (void)strcat(aExecute,pstExecutorInformation->aChecklistVersion);
    (void)strcat(aExecute,pSpace);
	(void)strcat(aExecute, pstExecutorInformation->aChecklistFolder);
	(void)strcat(aExecute, pSpace);
	pExecuteOrder = strcat(aExecute, pstExecutorInformation->aFlashType);
    printf("Execute:%s\n",pExecuteOrder);
    system(pExecuteOrder);
	
    return;
}
void GetCharArray(int Number,char *pChar1,char *pChar2)
{
    int s = 0,p = Number,i,j;
    for(i = 0; p != 0; i++)
    {
        s = p % 10;
        p = p / 10;
        pChar1[i] = char(s + 48); 
    }
    i--;
    for(j = 0; i >= 0; j++)
    {
        pChar2[j] = pChar1[i];
        i--;
    }
    pChar2[j] = '\0';

    return;
}
char *GetDate(char *pDate)
{
    char aChar1[10];
    char aChar2[10];
    char *pUnder = "_";

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    GetCharArray(1900 + timeinfo->tm_year, aChar1, aChar2);
    strcat(pDate,aChar2);
    strcat(pDate,pUnder);

    GetCharArray(timeinfo->tm_mon + 1, aChar1, aChar2);
    strcat(pDate,aChar2);
    strcat(pDate,pUnder);

    GetCharArray(timeinfo->tm_mday, aChar1, aChar2);
    strcat(pDate,aChar2);
    strcat(pDate,pUnder);

    GetCharArray(timeinfo->tm_hour, aChar1, aChar2);
    strcat(pDate,aChar2);
    strcat(pDate,pUnder);

    GetCharArray(timeinfo->tm_min, aChar1, aChar2);
    strcat(pDate,aChar2);

    //printf("%s\n",pDate);
    return pDate;
}
DWORD WINAPI ComeOnLetsRunning(LPVOID lpParam)
{
    char aChecklistName[MAXBYTE] = {0};
    char aDate[MAXBYTE] = {0};
	char *pUnderline = "#";
    char *pLogNameEnd = "_runninglog.txt";

    EXECUTOR_INFORMATION *pstExecutorInformation = (EXECUTOR_INFORMATION *)lpParam;
    
    gethostname(pstExecutorInformation->aHostName,sizeof(pstExecutorInformation->aHostName));
	RunningPhase = 1;
    PriorExecute(pstExecutorInformation);
    Execute(pstExecutorInformation);

    //running结束，改变Executor状态
    pstExecutorInformation->ucExecutorState = Done;
    pstExecutorInformation->ucTime = 0;
	RunningPhase = 0;
    printf("Running thread is done\n");
    return 0;
}

int main(int argc, char* argv[])
{
    UINT8 Recvflag = 1;
    LPDWORD lpCommunicationThreadId = 0;
    WSADATA WsaData;
    
    EXECUTOR_INFORMATION *pstExecutorInformation = NULL;
    DRC_PROTOCOL_UNIT *pTempSendProtocolUnit = NULL;
    DRC_PROTOCOL_UNIT *pTempRecvProtocolUnit = NULL;
    
    char aNetDiskPath[128];
    char aLocalPath[128];
    char aHostName[MAXBYTE] = {0};
	char aReboot[128] = "taskkill /f /t /im ";

    CreateMutex(NULL,FALSE,(LPCWSTR)UNIQUE_ID_FOR_EXECUTOR);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("One executor already exits!\n");
        system("pause");
        return -1;
    }

    pESendProtocolUnitMain = NULL;
    pESendProtocolUnitComm = NULL;
    pERecvProtocolUnitComm = NULL;
    pERecvProtocolUnitMain = NULL;
    
    pstExecutorInformation = (EXECUTOR_INFORMATION *)malloc(sizeof(EXECUTOR_INFORMATION));
    memset(pstExecutorInformation,0,sizeof(EXECUTOR_INFORMATION));
    pstExecutorInformation->ucExecutorState = Idle;
    pstExecutorInformation->ucTime = TimeOut;
    
    strcpy(pstExecutorInformation->aLocalPath,argv[2]);
    strcpy(pstExecutorInformation->aNetDiskPath,argv[1]);
    strcpy(aLocalPath,argv[2]);
    strcpy(aNetDiskPath,argv[1]);

    //设置为无缓冲区模式
    //setvbuf(stdout,NULL,_IONBF,0);

    if (WSAStartup(MAKEWORD(2,2),&WsaData))
    { 
        printf("Winsock initialization failed!\n"); 
        WSACleanup(); 
        return -1;
    }

    //创建通信线程
    if (FALSE == CreateThread(NULL,0,CommunicationThread,pstExecutorInformation,0,lpCommunicationThreadId))
    {
        printf("create communication thread failed\n");
    }

    gethostname(aHostName,sizeof(aHostName));

    //轮循处理报文并维护状态机
    while(1)
    {
        if (NULL != pERecvProtocolUnitMain)
        {
            if (1 == Recvflag)
            {
                //处理协议报文
                switch (pERecvProtocolUnitMain->ucCmdCode)
                {
                case AssignId:
                    //只要收到AssignID，就从头开始，相当于重新初始化
                    printf("AssignID\n");
					if (RunningPhase != 0)
					{
						RunningPhase = 0;
						strcpy(aReboot, "taskkill /f /t /im ");
						(void)strcat(aReboot, pstExecutorInformation->aExeName);
						system(aReboot);
						printf("%s\n", aReboot);
						if (FALSE == TerminateThread(pstExecutorInformation->hThreadHandle,-1))
						{
							printf("terminate thread failed!\n");
							system("pause");
						}
						if (FALSE == CloseHandle(pstExecutorInformation->hThreadHandle))
						{
							printf("Close handle error\n");
							system("pause");
						}
					}

					memset(pstExecutorInformation->aExeName, 0, 32);
					memset(pstExecutorInformation->aProjectName, 0, 32);
					memset(pstExecutorInformation->aControllerName, 0, 32);
					memset(pstExecutorInformation->aChecklistFolder, 0, 32);
					memset(pstExecutorInformation->aFlashType, 0, 32);
					memset(pstExecutorInformation->aChecklistVersion, 0, 64);
					memset(pstExecutorInformation->aTypeofWholeProject, 0, 128);
					memset(pstExecutorInformation->aProjectPath, 0, 128);

                    pstExecutorInformation->ucExecutorId = pERecvProtocolUnitMain->ucExecutorId;
                    pstExecutorInformation->ucTime = 0;
                    pstExecutorInformation->ucExecutorState = StandbyforChecklist;

                    pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                    memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                    pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                    pTempSendProtocolUnit->ucCmdCode = AssignId;
                    pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;
                    strcpy((char *)(pTempSendProtocolUnit->aMsgString),aHostName);
                    //有可能是初次使用,也有可能不是
                    if (NULL != pESendProtocolUnitMain)
                    {
                        pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                        pESendProtocolUnitMain = pTempSendProtocolUnit;
                    }
                    else
                    {
                        pESendProtocolUnitMain = pTempSendProtocolUnit;
                        pESendProtocolUnitComm = pTempSendProtocolUnit;
                    }
                    break;
                case DispatchChecklist:
                    printf("checklist:%s\n",(char *)(pERecvProtocolUnitMain->aMsgString));
                    if (0 == strcmp((char *)(pERecvProtocolUnitMain->aMsgString),NoMoreChecklist))
                    {
                        printf("No more checklist\n");
                        //进入Idle状态,清空结构体
                        memset(pstExecutorInformation,0,sizeof(EXECUTOR_INFORMATION));
                        pstExecutorInformation->ucExecutorState = Idle;
                        pstExecutorInformation->ucTime = 0;
                        pstExecutorInformation->pFirstControllerAddress = NULL;
                        strcpy(pstExecutorInformation->aLocalPath,aLocalPath);
                        strcpy(pstExecutorInformation->aNetDiskPath,aNetDiskPath);
                    }
                    else
                    {
                        pstExecutorInformation->ucTime = 0;
                        pstExecutorInformation->ucExecutorState = AskingForPara;
                        memset(pstExecutorInformation->aChecklistFolder,0,32);
                        memcpy(pstExecutorInformation->aChecklistFolder,(char *)(pERecvProtocolUnitMain->aMsgString),32);

                        pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));

                        pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                        pTempSendProtocolUnit->ucCmdCode = DispatchChecklist;
                        pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;

                        pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                        pESendProtocolUnitMain = pTempSendProtocolUnit;
                    }
                    break;
                case ExeName:
                    printf("ExeName:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aExeName,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
                case ProjectPath:
                    printf("ProjectPath:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aProjectPath,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
                case ProjectName:
                    printf("ProjectName:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aProjectName,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
                case ChecklistVersion:
                    printf("ChecklistVersion:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aChecklistVersion,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
				case FlashType:
					printf("FlashType:%s\n", (char *)pERecvProtocolUnitMain->aMsgString);
					strcpy((char *)pstExecutorInformation->aFlashType, (char *)pERecvProtocolUnitMain->aMsgString);
					break;
                case TypeofWholeProject:
                    printf("TypeofWholeProject:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aTypeofWholeProject,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
                case ControllerName:
                    printf("ControllerName:%s\n",(char *)pERecvProtocolUnitMain->aMsgString);
                    strcpy((char *)pstExecutorInformation->aControllerName,(char *)pERecvProtocolUnitMain->aMsgString);
                    break;
                case AllTheParas:
                    pstExecutorInformation->ucTime = 0;
                    break;
                case CheckParameter:
                    printf("CheckParameter\n");
                    pstExecutorInformation->ucTime = 0;
                    pstExecutorInformation->ucExecutorState = PreparedforRunning;
                    break;
                case RunChecklist:
                    //起running线程
                    if (pstExecutorInformation->ucExecutorState != Running)
                    {
                        pstExecutorInformation->ucTime = 0;
                        pstExecutorInformation->ucExecutorState = Running;

                        pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                        pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                        pTempSendProtocolUnit->ucCmdCode = RunChecklist;
                        pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;
						
						pstExecutorInformation->hThreadHandle = CreateThread(NULL,0,ComeOnLetsRunning,pstExecutorInformation,0,pstExecutorInformation->lpRunningThreadId);

                        if (FALSE == pstExecutorInformation->hThreadHandle)
                        {
                            printf("Create running thread failed\n");
                            strcpy((char *)(pTempSendProtocolUnit->aMsgString),NotOk);
                        }
                        else
                        {
                            strcpy((char *)(pTempSendProtocolUnit->aMsgString),Ok);
                        }

                        pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                        pESendProtocolUnitMain = pTempSendProtocolUnit;
                    }
                    else
                    {
                        pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                        pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                        pTempSendProtocolUnit->ucCmdCode = RunChecklist;
                        pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;
                        strcpy((char *)(pTempSendProtocolUnit->aMsgString),Ok);
                        pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                        pESendProtocolUnitMain = pTempSendProtocolUnit;
                    }
                    break;
                case ReportStatus:
                    break;
                case NextCycle:
                    pstExecutorInformation->ucTime = 0;
                    //准备进入下一轮循环
                    pstExecutorInformation->ucExecutorState = StandbyforChecklist;
                    memset(pstExecutorInformation->aExeName,0,32);
                    memset(pstExecutorInformation->aProjectName,0,32);
                    memset(pstExecutorInformation->aControllerName,0,32);
                    memset(pstExecutorInformation->aChecklistFolder,0,32);
					memset(pstExecutorInformation->aFlashType,0,32);
                    memset(pstExecutorInformation->aChecklistVersion,0,64);
                    memset(pstExecutorInformation->aTypeofWholeProject,0,128);
                    memset(pstExecutorInformation->aProjectPath,0,128);

                    pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                    memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                    pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                    pTempSendProtocolUnit->ucCmdCode = NextCycle;
                    pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;

                    pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                    pESendProtocolUnitMain = pTempSendProtocolUnit;
                    break;
                default:
                    printf("Protocol cmd code to default\n");
                    break;
                }
                Recvflag = 0;
            }
        
            if (NULL != pERecvProtocolUnitMain->pNextProtocolUnit)
            {
                Recvflag = 1;
                pTempRecvProtocolUnit = pERecvProtocolUnitMain;
                pERecvProtocolUnitMain = pERecvProtocolUnitMain->pNextProtocolUnit;
                free(pTempRecvProtocolUnit);
            }
        
        }
        //延时1s
        Sleep(1000);
        if (pstExecutorInformation->ucTime == 0)
        {
            switch (pstExecutorInformation->ucExecutorState)
            {
            case Idle:
                pstExecutorInformation->ucTime = TimeOut;
                //等待Controller来连接
                break;
            case StandbyforChecklist:
                pstExecutorInformation->ucTime = TimeOut;
                //等待Controller发送checklist
                break;
            case AskingForPara:
                pstExecutorInformation->ucTime = TimeOut;

                if (0 ==  CheckAllParameters(pstExecutorInformation))
                {
                    pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                    memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));

                    pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                    pTempSendProtocolUnit->ucCmdCode = CheckParameter;
                    pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;

                    pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                    pESendProtocolUnitMain = pTempSendProtocolUnit;
                }
                break;
            case PreparedforRunning:
                pstExecutorInformation->ucTime = TimeOut;
                //等待Controller发送Runchecklist命令
                break;
            case Running:
                //printf("running checklist\n");
                //为保持链路畅通，持续回报running状态
                pstExecutorInformation->ucTime = TimeOut;
                pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));

                pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                pTempSendProtocolUnit->ucCmdCode = ReportStatus;
                pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;
                strcpy((char *)(pTempSendProtocolUnit->aMsgString),StateRunning);

                pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                pESendProtocolUnitMain = pTempSendProtocolUnit;
                break;
            case Done:
				if (FALSE == CloseHandle(pstExecutorInformation->lpRunningThreadId))
				{
					printf("Close handle error\n");
				}
                pstExecutorInformation->ucTime = TimeOut;
                pTempSendProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                memset(pTempSendProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));

                pTempSendProtocolUnit->ucExecutorId = pstExecutorInformation->ucExecutorId;
                pTempSendProtocolUnit->ucCmdCode = ReportStatus;
                pTempSendProtocolUnit->ulProtocolLabel = ProtocolLabel;
                strcpy((char *)(pTempSendProtocolUnit->aMsgString),StateOver);

                pESendProtocolUnitMain->pNextProtocolUnit = pTempSendProtocolUnit;
                pESendProtocolUnitMain = pTempSendProtocolUnit;
                break;
            default:
                printf("Executor state to default\n");
                break;
            }
        }
        else
        {
            pstExecutorInformation->ucTime--;
        }
    }

    WSACleanup();

    //需要在最后释放收发FIFO各自的最后一块内存
    free(pESendProtocolUnitComm);
    pESendProtocolUnitComm = NULL;
    free(pERecvProtocolUnitMain);

    //释放结构体
    free(pstExecutorInformation);
    return 0;
}

