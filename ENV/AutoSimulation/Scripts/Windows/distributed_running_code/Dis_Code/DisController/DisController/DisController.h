
struct DRC_PROTOCOL_UNIT
{
    UINT32 ulProtocolLabel;
    UINT32 ulUintNum;
    UINT8 ucExecutorId;
    UINT8 ucCmdCode;
    UINT8 aMsgString[114];
    DRC_PROTOCOL_UNIT *pNextProtocolUnit;
};

typedef struct _RUNNING_TASK
{
    UINT8 ucExecutingState;
    char aChecklistFolder[MAXBYTE];
    char aExeName[MAXBYTE];
    char aTypeofWholeProject[MAXBYTE];
    char aProjectName[MAXBYTE];
    char aProjectPath[MAXBYTE];
    char aChecklistVersion[MAXBYTE];
	char aFlashType[MAXBYTE];
}RUNNINGTASK;

struct EXECUTOR_INFORMATION
{
    UINT8 ucState;
    UINT8 ucExecutorId;
    UINT8 ucTime;
    UINT8 ucChecklistNum;
    UINT8 ucLastCommandCode;
    char aExecutorName[32];
    SOCKET ExecutorSock;
    sockaddr_in ExecutorAddr;
    EXECUTOR_INFORMATION *pNextExecutorInformation;
};

struct CONTROLLER_INFORMATION
{
    char *pIpAddresses;
    char *pControllerName;
    char *pRunningTaskList;
    char *pDispatchedLogFile;
    char *pRunningOverLogFile;
};

#define UNIQUE_ID_FOR_CONTROLLER       "Dis_Contorller"

#define TimeOut                 (UINT8)30

#define InvalidChecklistNum     (UINT8)255

//auto
#define ProtocolLabel           (((UINT32)97) | ((UINT32)117<<8) | ((UINT32)116<<16) | (UINT32)111<<24)

//CmdCode类型
#define InvalidCmdcode          (UINT8)0
#define AssignId                        InvalidCmdcode+(UINT8)1
#define DispatchChecklist          AssignId+(UINT8)1
#define TypeofWholeProject    DispatchChecklist+(UINT8)1
#define ControllerName            TypeofWholeProject+(UINT8)1
#define ExeName                     ControllerName+(UINT8)1
#define ProjectName                 ExeName+(UINT8)1
#define ProjectPath                 ProjectName+(UINT8)1
#define ChecklistVersion           ProjectPath+(UINT8)1
#define FlashType					ChecklistVersion+(UINT8)1
#define AllTheParas                 FlashType+(UINT8)1
#define CheckParameter          AllTheParas+(UINT8)1
#define RunChecklist                CheckParameter+(UINT8)1
#define ReportStatus                RunChecklist+(UINT8)1
#define NextCycle                   ReportStatus+(UINT8)1

//Executor state
#define Idle                    (UINT8)0
#define AskingForPara           (UINT8)1
#define StandbyforChecklist     (UINT8)2
#define PreparedforRunning      (UINT8)3
#define Running                 (UINT8)4
#define Done                    (UINT8)5
#define OverForMe               (UINT8)6

//Checklist State类型
#define NoOwner                 (UINT8)1
#define SentOneTime             (UINT8)2
#define SentTwice               (UINT8)3
#define SentThreeTimes          (UINT8)4
#define Dispatched              (UINT8)5

//MsgString类型
#define Ok                      "Ok"
#define NotOk                   "Not Ok"
#define NoMoreChecklist         "No More Checklist"
#define StateRunning            "Running"
#define StateOver               "Over"

