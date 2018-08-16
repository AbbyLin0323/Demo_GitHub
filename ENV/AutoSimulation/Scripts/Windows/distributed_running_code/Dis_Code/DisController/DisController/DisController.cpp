// DisController.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "stdlib.h"
#include <time.h>
#include <string.h>
#pragma comment (lib,"ws2_32")
#include "DisController.h"

DRC_PROTOCOL_UNIT *pCSendProtocolUnitMain;
DRC_PROTOCOL_UNIT *pCSendProtocolUnitComm;
DRC_PROTOCOL_UNIT *pCRecvProtocolUnitComm;
DRC_PROTOCOL_UNIT *pCRecvProtocolUnitMain;

DWORD WINAPI CommunicationThread(LPVOID lpParam)
{
    WSADATA WsaData;
    int AddrLen = sizeof(SOCKADDR);
    int Sendflag = 1;
    int lSendRet = 0;
    int lRecvRet = 0;
    UINT32 ulUnitNum = 0;
    UINT32 ulSelectNum = 1;
   
    fd_set Readfd;
    INT32 lRet = 0;
    timeval SelectTimeOut;
    DRC_PROTOCOL_UNIT *pTempRecvProtocolUnit = NULL;
    DRC_PROTOCOL_UNIT *pTempSendProtocolUnit = NULL;
    EXECUTOR_INFORMATION *pExecutorInformation = (EXECUTOR_INFORMATION *)lpParam;
    EXECUTOR_INFORMATION *pNowExecutorInformation = (EXECUTOR_INFORMATION *)lpParam;

    if (WSAStartup(MAKEWORD(2,2),&WsaData))
    { 
        printf("Winsock initialize failed!\n"); 
        WSACleanup();
        return -1;
    }

    SelectTimeOut.tv_sec=3;
    SelectTimeOut.tv_usec=0;

    while(1)
    {
        if (NULL != pCSendProtocolUnitComm)
        {
            if ((Sendflag < 3) && (Sendflag > 0))
            {
                pCSendProtocolUnitComm->ulUintNum = ulUnitNum++;
                lSendRet = sendto((pExecutorInformation + pCSendProtocolUnitComm->ucExecutorId - 1)->ExecutorSock,(char *)pCSendProtocolUnitComm,sizeof(DRC_PROTOCOL_UNIT),0,
                (sockaddr *)&((pExecutorInformation + pCSendProtocolUnitComm->ucExecutorId - 1)->ExecutorAddr),AddrLen);
                //printf("send %d\n",pCSendProtocolUnitComm->ucCmdCode);
                if (lSendRet == SOCKET_ERROR)
                {
                    //printf("Send to executor %d cmd code %d failed!!!\n",pCSendProtocolUnitComm->ucExecutorId,pCSendProtocolUnitComm->ucCmdCode);
                    Sendflag++;
                }
                else
                {
                    Sendflag = 0;
                }
            }
            if ((NULL != pCSendProtocolUnitComm->pNextProtocolUnit) && ((Sendflag == 0) || (Sendflag >= 3)))
            {
                Sendflag = 1;
                pTempSendProtocolUnit = pCSendProtocolUnitComm;
                pCSendProtocolUnitComm = pCSendProtocolUnitComm->pNextProtocolUnit;
                free(pTempSendProtocolUnit);
            }
        }

        //轮询结构体链表
        pNowExecutorInformation = pExecutorInformation;
        FD_ZERO(&Readfd);
        ulSelectNum = 1;
        while (NULL != pNowExecutorInformation)
        {
            FD_SET(pNowExecutorInformation->ExecutorSock, &Readfd);
            pNowExecutorInformation = pNowExecutorInformation->pNextExecutorInformation;
            ulSelectNum++;
        }
        
        lRet = select(0, &Readfd, 0, 0, &SelectTimeOut);
        if (lRet > 0)
        {
            pNowExecutorInformation = pExecutorInformation;
            while (NULL != pNowExecutorInformation)
            {
                if(FD_ISSET(pNowExecutorInformation->ExecutorSock,&Readfd))
                {
                    pTempRecvProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                    memset(pTempRecvProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                    lRecvRet = recvfrom(pNowExecutorInformation->ExecutorSock,(char *)pTempRecvProtocolUnit,sizeof(DRC_PROTOCOL_UNIT),0,
                        (SOCKADDR *)&(pNowExecutorInformation->ExecutorAddr),&AddrLen);
                    /*if (lRecvRet == SOCKET_ERROR)
                    {
                        printf("Receive error!!!\n");
                        printf("Error code:%d\n",WSAGetLastError());
                    }*/
                    //只处理协议报文
                    if (ProtocolLabel == pTempRecvProtocolUnit->ulProtocolLabel)
                    {
                        //printf("receive %d\n",pTempRecvProtocolUnit->ucCmdCode);
                        pTempRecvProtocolUnit->pNextProtocolUnit = NULL;
                        if (NULL == pCRecvProtocolUnitComm)
                        {
                            pCRecvProtocolUnitComm = pTempRecvProtocolUnit;
                            pCRecvProtocolUnitMain = pTempRecvProtocolUnit;
                        }
                        else
                        {
                            pCRecvProtocolUnitComm->pNextProtocolUnit = pTempRecvProtocolUnit;
                            pCRecvProtocolUnitComm = pTempRecvProtocolUnit;
                        }
                    }
                    else
                    {
                        //printf("Free non-protocol unit!!!\n");
                        free(pTempRecvProtocolUnit);
                    }
                }
                pNowExecutorInformation = pNowExecutorInformation->pNextExecutorInformation;
            }
        }
        
    }
    return 0;
}
EXECUTOR_INFORMATION *BuildInformationStructforExecutor(char *pIpAddresses)
{
    char aIpAddress[10][20] = {0};
    char *pStill = pIpAddresses,Signal = ':';
    UINT32 ulPositionInSingleAddress = 0,ulCount = 0,i = 0;
    EXECUTOR_INFORMATION *pHeadExecutorInformation = NULL;
    EXECUTOR_INFORMATION *pNowExecutorInformation = NULL;
    EXECUTOR_INFORMATION *pTempExecutorInformation = NULL;
    u_long FAR Argp = 1;

    for (i = 0; '\0' != pStill[i]; i++)
    {
        if(Signal != pStill[i])
        {
            aIpAddress[ulCount][ulPositionInSingleAddress] = pStill[i];
            ulPositionInSingleAddress++;
        }
        else
        {
            ulPositionInSingleAddress = 0;
            ulCount++;
        }
    }

    pHeadExecutorInformation = (EXECUTOR_INFORMATION *)malloc((ulCount + 1) * sizeof(EXECUTOR_INFORMATION));
    memset(pHeadExecutorInformation,0,(ulCount + 1) * sizeof(EXECUTOR_INFORMATION));
    
    for (i = 0; i <= ulCount; i++)
    {
        pNowExecutorInformation = pHeadExecutorInformation + i;
        
        pNowExecutorInformation->ExecutorSock = socket(AF_INET,SOCK_DGRAM,0);
        ioctlsocket(pNowExecutorInformation->ExecutorSock,FIONBIO,&Argp);

        pNowExecutorInformation->ExecutorAddr.sin_family = AF_INET;
        pNowExecutorInformation->ExecutorAddr.sin_port = htons(9000);
        pNowExecutorInformation->ExecutorAddr.sin_addr.S_un.S_addr = inet_addr(aIpAddress[i]);
        pNowExecutorInformation->ucState = Idle;

        pNowExecutorInformation->ucExecutorId = i + 1;//ExecutorId从1开始，0认为是无效Id
        printf("Executor %d address:%s\n",pNowExecutorInformation->ucExecutorId,aIpAddress[i]);
        pNowExecutorInformation->ucLastCommandCode = InvalidCmdcode;
        pNowExecutorInformation->ucChecklistNum = InvalidChecklistNum;
        pNowExecutorInformation->ucTime = 0;

        if (i == 0)
        {
            pTempExecutorInformation = pNowExecutorInformation;
        }
        else
        {
            pTempExecutorInformation->pNextExecutorInformation = pNowExecutorInformation;
            pTempExecutorInformation = pNowExecutorInformation;
        }
    }
    return pHeadExecutorInformation;
}
void SendAllTheParas(UINT8 ucExecutorID,CONTROLLER_INFORMATION *pControllerInformation,
                                            RUNNINGTASK *pRunningTask)
{
    DRC_PROTOCOL_UNIT *pTempProtocolUnit = NULL;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = ExeName;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pRunningTask->aExeName);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = ProjectPath;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pRunningTask->aProjectPath);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = ChecklistVersion;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pRunningTask->aChecklistVersion);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

	pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
	memset(pTempProtocolUnit, 0, sizeof(DRC_PROTOCOL_UNIT));
	pTempProtocolUnit->ucExecutorId = ucExecutorID;
	pTempProtocolUnit->ucCmdCode = FlashType;
	pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
	strcpy((char *)(pTempProtocolUnit->aMsgString), pRunningTask->aFlashType);

	pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
	pCSendProtocolUnitMain = pTempProtocolUnit;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = ProjectName;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pRunningTask->aProjectName);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = TypeofWholeProject;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pRunningTask->aTypeofWholeProject);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
    pTempProtocolUnit->ucExecutorId = ucExecutorID;
    pTempProtocolUnit->ucCmdCode = ControllerName;
    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
    strcpy((char *)(pTempProtocolUnit->aMsgString),pControllerInformation->pControllerName);

    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
    pCSendProtocolUnitMain = pTempProtocolUnit;

    return;
}
int MaintainExecutorState(EXECUTOR_INFORMATION *pExecutorInformation,void *pChecklistPara,int ChecklistNum
                          ,CONTROLLER_INFORMATION *pControllerInformation)
{
    
    //如果checklist全都处理完毕，则退出,用所有Executor的状态为Idle或OverForMe来判断

    
    return 0;
}
void ProcessProtocolUnit(EXECUTOR_INFORMATION *pExecutorInformation,void *pChecklistPara
                         ,CONTROLLER_INFORMATION *pControllerInformation)
{
    
    
    return;
}
INT32 GetRunningTasks(RUNNINGTASK *pRunningTask,char *pRunningTaskList,UINT32 *pChecklistNum)
{

    FILE *pFilePointer;
    char aRunningTask[10*MAXBYTE] = {0};
	char *pFirst = NULL, *pSecond = NULL, *pThird = NULL, *pForth = NULL, *pFifth = NULL, *pSixth = NULL, Signal1 = ':', Signal2 = '#';
    char *pChecklistFolder = NULL,*pTemp = NULL;
    char *pSignalPath = "+";
    UINT32 ulPositionInSingleName = 0,ulCount = 0;
    UINT32 i = 0;
	
    if (0 != fopen_s(&pFilePointer,pRunningTaskList,"rb"))
    {
        printf("Running task file open error!\n");
        return -1;
    }
    while(1)
    {
        if (NULL == pTemp)
        {
            memset(aRunningTask,0,10*MAXBYTE);
            
            if (NULL == fgets(aRunningTask,10*MAXBYTE,pFilePointer))
            {
                break;
            }
            //printf("read one line\n");
            //获取Running参数
            pFirst = strchr(aRunningTask,Signal2);
            pSecond = strchr(pFirst + 1,Signal2);
            pThird = strchr(pSecond + 1,Signal2);
            pForth = strchr(pThird + 1,Signal2);
            pFifth = strchr(pForth + 1,Signal2);
			pSixth = strchr(pFifth + 1, Signal2);
			if ((NULL == pFirst) || (NULL == pSecond) || (NULL == pThird) || (NULL == pForth) || (NULL == pFifth) || (NULL == pSixth))
            {
                printf("Error when searching signal!\n");
                return -1;
            }
            
            strncpy((pRunningTask + i)->aProjectPath,aRunningTask,pFirst - aRunningTask);
            strncpy((pRunningTask + i)->aExeName,pFirst + 1,pSecond - pFirst - 1);
            strncpy((pRunningTask + i)->aProjectName,pSecond + 1,pThird - pSecond - 1);
            strncpy((pRunningTask + i)->aTypeofWholeProject,pThird + 1,pForth - pThird - 1);
            strncpy((pRunningTask + i)->aChecklistVersion,pForth + 1,pFifth - pForth - 1);
			strncpy((pRunningTask + i)->aFlashType, pFifth + 1, pSixth - pFifth - 1);

            printf("ProjectPath:%s\n",(pRunningTask + i)->aProjectPath);
            printf("ExeName:%s\n",(pRunningTask + i)->aExeName);
            printf("ProjectName:%s\n",(pRunningTask + i)->aProjectName);
            printf("TypeofWholeProject:%s\n",(pRunningTask + i)->aTypeofWholeProject);
            printf("ChecklistVersion:%s\n",(pRunningTask + i)->aChecklistVersion);
			printf("FlashType:%s\n", (pRunningTask + i)->aFlashType);

			pChecklistFolder = strchr(pSixth + 1, Signal1);
            if (NULL == pChecklistFolder)
            {
				for (ulCount = 0; (('\0' != pSixth[ulCount + 1]) && ('\r' != pSixth[ulCount + 1]) && ('\n' != pSixth[ulCount + 1])); ulCount++)
                {
					(pRunningTask + i)->aChecklistFolder[ulCount] = pSixth[ulCount + 1];
                }
            }
            else
            {
				strncpy((pRunningTask + i)->aChecklistFolder, pSixth + 1, pChecklistFolder - pSixth - 1);
                printf("checklist folder:%s\n",(pRunningTask + i)->aChecklistFolder);
                pTemp = pChecklistFolder;
            }
        }
        else
        {
            memcpy(pRunningTask + i,pRunningTask + i -1,sizeof(RUNNINGTASK));
            memset((pRunningTask + i)->aChecklistFolder,0,MAXBYTE);
            
            pChecklistFolder = strchr(pTemp + 1,Signal1);
            if (NULL == pChecklistFolder)
            {
                for(ulCount = 0; (('\0' != pTemp[ulCount + 1]) && ('\r' != pTemp[ulCount + 1]) && ('\n' != pTemp[ulCount + 1])); ulCount++)
                {
                    (pRunningTask + i)->aChecklistFolder[ulCount] = pTemp[ulCount + 1];
                }
                pTemp = NULL;
            }
            else
            {
                strncpy((pRunningTask + i)->aChecklistFolder,pTemp + 1,pChecklistFolder - pTemp - 1);
                pTemp = strchr(pChecklistFolder + 1,Signal1);
            }
            printf("checklist folder:%s\n",(pRunningTask + i)->aChecklistFolder);
        }
        (pRunningTask + i)->ucExecutingState = NoOwner;
        i++;
    }
    *pChecklistNum = i;
    
    return 0;
}
int main(int argc, char* argv[])
{
    UINT8 RunningOverflag = 0;
    UINT8 ChecklistSentFlag = 0;
    UINT8 AnyOtherChecklistWaitingFlag = 0;
    UINT8 Recvflag = 1;
    UINT32 ChecklistNum = 0;
    UINT32 Cycle = 0;
    
    LPDWORD lpThreadId = 0;
    WSADATA WsaData;
    
    EXECUTOR_INFORMATION *pNowExecutorInformation = NULL;
    EXECUTOR_INFORMATION *pExecutorInformation = NULL;
    CONTROLLER_INFORMATION *pControllerInformation = NULL;
    
    DRC_PROTOCOL_UNIT *pTempProtocolUnit = NULL;
    DRC_PROTOCOL_UNIT *pTempRecvProtocolUnit = NULL;
    RUNNINGTASK *pRunningTask = NULL;
    FILE *pDispatchedLogFile = NULL;
    FILE *pRunningOverLogFile = NULL;

    char aHostName[MAXBYTE] = {0};

    CreateMutex(NULL,FALSE,(LPCWSTR)UNIQUE_ID_FOR_CONTROLLER);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("One controller already exits!\n");
        system("pause");
        return -1;
    }
    
    if (WSAStartup(MAKEWORD(2,2),&WsaData))
    { 
        printf("Winsock initialize failed!\n"); 
        WSACleanup();
        system("pause");
        return -1;
    }
    
    gethostname(aHostName,sizeof(aHostName));

    pControllerInformation = (CONTROLLER_INFORMATION *)malloc(sizeof(CONTROLLER_INFORMATION));
    pControllerInformation->pRunningOverLogFile = argv[4];
    pControllerInformation->pDispatchedLogFile= argv[3];
    pControllerInformation->pRunningTaskList = argv[2];
    pControllerInformation->pIpAddresses = argv[1];
    pControllerInformation->pControllerName = aHostName;
    
    pRunningTask = (RUNNINGTASK *)malloc(100 * sizeof(RUNNINGTASK));
    memset(pRunningTask,0,100*sizeof(RUNNINGTASK));

    pCSendProtocolUnitMain = NULL;
    pCSendProtocolUnitComm = NULL;
    pCRecvProtocolUnitComm = NULL;
    pCRecvProtocolUnitMain = NULL;

    //获取其他running时候的参数
    if (0 != GetRunningTasks(pRunningTask,pControllerInformation->pRunningTaskList,&ChecklistNum))
    {
        return -1;
    }
    
    printf("checklistnum:%d\n",ChecklistNum);

    //为所有Executor建立相应的信息结构体
    pExecutorInformation = BuildInformationStructforExecutor(pControllerInformation->pIpAddresses);
    if (NULL  == pExecutorInformation)
    {
        printf("Build executor information failed!\n");
        system("pause");
        return -1;
    }
    
    //创建communication线程
    if (FALSE == CreateThread(NULL,0,CommunicationThread,pExecutorInformation,0,lpThreadId))
    {
        printf("Create communication thread failed\n");
        system("pause");
        return -1;
    }

    while (1)
    {
        pNowExecutorInformation = pExecutorInformation;
        if (1 == RunningOverflag)
        {
            while (NULL != pNowExecutorInformation)
            {
                if ((Idle != pNowExecutorInformation->ucState) && (OverForMe != pNowExecutorInformation->ucState))
                {
                    RunningOverflag = 2;
                }
                pNowExecutorInformation = pNowExecutorInformation->pNextExecutorInformation;
            }
        }

        if (1 == RunningOverflag)
        {
            printf("All the checklist have run done!\n");
            break;
        }

        RunningOverflag = 0;
        pNowExecutorInformation = pExecutorInformation;

        while (NULL != pNowExecutorInformation)
        {
            if (0 == pNowExecutorInformation->ucTime)
            {
                switch (pNowExecutorInformation->ucState)
                {
                    case Idle:
                        //printf("Executor %d is Idle\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;

                        pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                        pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                        pTempProtocolUnit->ucCmdCode = AssignId;
                        pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                        if (pCSendProtocolUnitMain == NULL)
                        {
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            pCSendProtocolUnitComm = pTempProtocolUnit;
                        }
                        else
                        {
                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                        }
                        break;
                    case StandbyforChecklist:
                        //printf("Executor %d is StandbyforChecklist\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;

                        if (InvalidChecklistNum != pNowExecutorInformation->ucChecklistNum)
                        {
                            //已有相应checklist
                            if ((pRunningTask + pNowExecutorInformation->ucChecklistNum)->ucExecutingState == SentThreeTimes)
                            {
                                //将这个Executor设置为OverForMe状态
                                printf("Lose connect with executor %d when sending checklist\n",pNowExecutorInformation->ucExecutorId);
                                (pRunningTask + pNowExecutorInformation->ucChecklistNum)->ucExecutingState = NoOwner;
                                pNowExecutorInformation->ucState = OverForMe;
                                pNowExecutorInformation->ucChecklistNum = InvalidChecklistNum;
                            }
                            else
                            {
                                pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                                memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                                pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                                pTempProtocolUnit->ucCmdCode = DispatchChecklist;
                                pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                                strcpy((char *)(pTempProtocolUnit->aMsgString), 
                                    (pRunningTask + pNowExecutorInformation->ucChecklistNum)->aChecklistFolder);
                                //printf("ready to send checklistfolder %s again\n",(char *)(pTempProtocolUnit->aMsgString));
                                pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                                pCSendProtocolUnitMain = pTempProtocolUnit;

                                (pRunningTask + pNowExecutorInformation->ucChecklistNum)->ucExecutingState++;
                            }
                        }
                        else
                        {
                            //分配checklist
                            for (Cycle = 0; Cycle < ChecklistNum; Cycle++)
                            {
                                if ((pRunningTask + Cycle)->ucExecutingState == NoOwner)
                                {
                                    pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                                    memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                                    pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                                    pTempProtocolUnit->ucCmdCode = DispatchChecklist;
                                    pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                                    strcpy((char *)(pTempProtocolUnit->aMsgString) , (pRunningTask + Cycle)->aChecklistFolder);
                                    //printf("get checklistfolder:%s\n",(char *)(pTempProtocolUnit->aMsgString));
                                    pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                                    pCSendProtocolUnitMain = pTempProtocolUnit;

                                    (pRunningTask + Cycle)->ucExecutingState++;
                                    pNowExecutorInformation->ucChecklistNum = Cycle;

                                    ChecklistSentFlag = 1;
                                    break;
                                }
                                else if ((pRunningTask + Cycle)->ucExecutingState < Dispatched)
                                {
                                    AnyOtherChecklistWaitingFlag = 1;
                                }
                            }
                            if ((ChecklistSentFlag == 0) && (AnyOtherChecklistWaitingFlag == 0))
                            {
                                //在本轮测试中将不再联系该Executor
                                printf("no more checklist\n");
                                pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                                memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                                pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                                pTempProtocolUnit->ucCmdCode = DispatchChecklist;
                                pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                                strcpy((char *)(pTempProtocolUnit->aMsgString) , NoMoreChecklist);
                                pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                                pCSendProtocolUnitMain = pTempProtocolUnit;

                                pNowExecutorInformation->ucState = OverForMe;
                                pNowExecutorInformation->ucChecklistNum = InvalidChecklistNum;
                                pNowExecutorInformation->ucTime = 0;
                            }
                            ChecklistSentFlag = 0;
                            AnyOtherChecklistWaitingFlag = 0;
                        }
                        break;
                    case AskingForPara:
                        //printf("Executor %d is AskingForPara\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;
                        break;
                    case PreparedforRunning:
                        //printf("Executor %d is PreparedforRunning\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;

                        pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                        pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                        pTempProtocolUnit->ucCmdCode = RunChecklist;
                        pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                        pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                        pCSendProtocolUnitMain = pTempProtocolUnit;
                        break;
                    case Running:
                        //printf("Executor %d is Running\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;
                        //Running状态下等待Executor定时自动汇报状态
                        break;
                    case Done:
                        //printf("Executor %d is Done\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;
                        //命令Executor开始下一轮running
                        pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                        memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                        pTempProtocolUnit->ucExecutorId = pNowExecutorInformation->ucExecutorId;
                        pTempProtocolUnit->ucCmdCode = NextCycle;
                        pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                        pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                        pCSendProtocolUnitMain = pTempProtocolUnit;
                        pRunningOverLogFile = fopen(pControllerInformation->pRunningOverLogFile,"a");
                        if (NULL == pRunningOverLogFile)
                        {
                            printf("open runningoverflagfile error when %s with %s on %s",(pRunningTask + pNowExecutorInformation
                            ->ucChecklistNum)->aChecklistVersion,(pRunningTask + pNowExecutorInformation
                            ->ucChecklistNum)->aChecklistFolder,pNowExecutorInformation->aExecutorName);
                            break;
                        }
                        fprintf(pRunningOverLogFile,"%s with %s on %s over\n",(pRunningTask + pNowExecutorInformation
                            ->ucChecklistNum)->aChecklistVersion,(pRunningTask + pNowExecutorInformation
                            ->ucChecklistNum)->aChecklistFolder,pNowExecutorInformation->aExecutorName);
                        fclose(pRunningOverLogFile);
                        break;
                    case OverForMe:
                        //printf("Executor %d is OverForMe\n",pNowExecutorInformation->ucExecutorId);
                        pNowExecutorInformation->ucTime = TimeOut;
                        RunningOverflag = 1;
                        //do nothing
                        break;
                    default:
                        printf("Executor %d is default!!!\n",pNowExecutorInformation->ucExecutorId);
                        break;
                    }
                }
                else
                {
                    pNowExecutorInformation->ucTime--;
                }
                pNowExecutorInformation = pNowExecutorInformation->pNextExecutorInformation;
        }
            
        //延时1s
        Sleep(1000);

        if (NULL != pCRecvProtocolUnitMain)
        {
            if (1 == Recvflag)
            {
                //处理协议报文,如果是OverForMe状态则不作处理了
                if ((pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState != OverForMe)
                {
                    if (((pCRecvProtocolUnitMain->ucCmdCode == ReportStatus) && (((pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState == PreparedforRunning) 
                        || (((pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState == Running))))
                        || ((pCRecvProtocolUnitMain->ucCmdCode != ReportStatus) && (pCRecvProtocolUnitMain->ucCmdCode > (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode)) )
                    {
                        switch (pCRecvProtocolUnitMain->ucCmdCode)
                        {
                        case AssignId:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = AssignId;

                            strcpy((pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->aExecutorName,
                                    (char *)(pCRecvProtocolUnitMain->aMsgString));
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = StandbyforChecklist;
                            break;
                        case DispatchChecklist:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = DispatchChecklist;
                            (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)
                                    ->ucChecklistNum)->ucExecutingState = Dispatched;
                            pDispatchedLogFile = fopen(pControllerInformation->pDispatchedLogFile,"a");
                            if (NULL == pDispatchedLogFile)
                            {
                                printf("DispatchedLogfile open error and the task is %s with %s\n!\n",(pRunningTask + 
                                    (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aChecklistVersion,
                                    (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aChecklistFolder);
                                (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = AskingForPara;
                                break;
                            }
                            fprintf(pDispatchedLogFile,"%s with %s on %s\n",(pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)
                                ->ucChecklistNum)->aChecklistVersion,(pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)
                                ->ucChecklistNum)->aChecklistFolder,(pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->aExecutorName);
                            fclose(pDispatchedLogFile);

                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = AskingForPara;
                            break;
                        case AllTheParas:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = AllTheParas;
                            SendAllTheParas(pCRecvProtocolUnitMain->ucExecutorId,pControllerInformation,
                                    pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum);

                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = AllTheParas;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case TypeofWholeProject:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = TypeofWholeProject;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = TypeofWholeProject;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aTypeofWholeProject);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case ControllerName:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ControllerName;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ControllerName;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),pControllerInformation->pControllerName);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case ExeName:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ExeName;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ExeName;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aExeName);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case ProjectName:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ProjectName;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ProjectName;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aProjectName);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case ProjectPath:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ProjectPath;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ProjectPath;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aProjectPath);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case ChecklistVersion:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ChecklistVersion;
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ChecklistVersion;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
                            strcpy((char *)(pTempProtocolUnit->aMsgString),
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aChecklistVersion);

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
						case FlashType:
							(pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = FlashType;
							pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
							memset(pTempProtocolUnit, 0, sizeof(DRC_PROTOCOL_UNIT));
							pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
							pTempProtocolUnit->ucCmdCode = FlashType;
							pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;
							strcpy((char *)(pTempProtocolUnit->aMsgString),
								(pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->aFlashType);

							pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
							pCSendProtocolUnitMain = pTempProtocolUnit;
							break;
                        case CheckParameter:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = CheckParameter;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;
                            
                            //printf("Executor %d CheckParameter ok\n",pCRecvProtocolUnitMain->ucExecutorId);
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = PreparedforRunning;

                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = CheckParameter;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case RunChecklist:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = RunChecklist;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;

                            if (0 == strcmp((char *)(pCRecvProtocolUnitMain->aMsgString),Ok))
                            {
                                //printf("Executor %d RunChecklist ok\n",pCRecvProtocolUnitMain->ucExecutorId);
                                (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = Running;
                            }
                            else
                            {
                                //printf("Executor %d return RunChecklist failed!!!\n",pCRecvProtocolUnitMain->ucExecutorId);
                                (pRunningTask + (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum)->ucExecutingState = NoOwner;
                                (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = OverForMe;
                                (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum = InvalidChecklistNum;
                            }
                            break;
                        case ReportStatus:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = ReportStatus;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;

                            if (0 == strcmp((char *)(pCRecvProtocolUnitMain->aMsgString),StateRunning))
                            {
                                //printf("Executor %d is running\n",pCRecvProtocolUnitMain->ucExecutorId);
                            }
                            else if (0 == strcmp((char *)(pCRecvProtocolUnitMain->aMsgString),StateOver))
                            {
                                //更新状态
                                //printf("Executor %d is done\n",pCRecvProtocolUnitMain->ucExecutorId);
                                (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = Done;
                            }
                            else
                            {
                                printf("Executor %d running status is error\n",pCRecvProtocolUnitMain->ucExecutorId);
                            }
                            pTempProtocolUnit = (DRC_PROTOCOL_UNIT *)malloc(sizeof(DRC_PROTOCOL_UNIT));
                            memset(pTempProtocolUnit,0,sizeof(DRC_PROTOCOL_UNIT));
                            pTempProtocolUnit->ucExecutorId = pCRecvProtocolUnitMain->ucExecutorId;
                            pTempProtocolUnit->ucCmdCode = ReportStatus;
                            pTempProtocolUnit->ulProtocolLabel = ProtocolLabel;

                            pCSendProtocolUnitMain->pNextProtocolUnit = pTempProtocolUnit;
                            pCSendProtocolUnitMain = pTempProtocolUnit;
                            break;
                        case NextCycle:
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = NextCycle;
                            printf("Executor %d NextCycle ok\n",pCRecvProtocolUnitMain->ucExecutorId);
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucTime = 0;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucState = StandbyforChecklist;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucChecklistNum = InvalidChecklistNum;
                            (pExecutorInformation + pCRecvProtocolUnitMain->ucExecutorId - 1)->ucLastCommandCode = InvalidCmdcode;

                            break;
                        default:
                            printf("Protocol cmd code to default with %d\n",pCRecvProtocolUnitMain->ucCmdCode);
                            break;
                        }
                    }
                    Recvflag = 0;
                }
            }

            if (NULL != pCRecvProtocolUnitMain->pNextProtocolUnit)
            {
                Recvflag = 1;
                pTempRecvProtocolUnit = pCRecvProtocolUnitMain;
                pCRecvProtocolUnitMain = pCRecvProtocolUnitMain->pNextProtocolUnit;
                free(pTempRecvProtocolUnit);
            }
        }
    }

    //需要在最后释放收发FIFO各自的最后一块内存
    free(pCRecvProtocolUnitMain);
    free(pCSendProtocolUnitComm);
    
    //释放所有结构体
    free(pControllerInformation);
    free(pExecutorInformation);

    return 0;
}
