
struct DRC_PROTOCOL_UNIT
{
    UINT32 ulProtocolLabel;
    UINT32 ulUintNum;
    INT8 ucExecutorId;
    INT8 ucCmdCode;
    UINT8 aMsgString[114];
    DRC_PROTOCOL_UNIT *pNextProtocolUnit;
};

typedef struct _EXECUTOR_INFORMATION
{
    UINT8 ucTime;
    UINT8 ucExecutorState;
    UINT8 ucExecutorId;
	UINT8 ucExecuteTimes;
    char aExeName[32];
    char aProjectName[32];
    char aHostName[32];
    char aControllerName[32];
    char aChecklistFolder[32];
	char aFlashType[32];
    char aChecklistVersion[64];
    char aTypeofWholeProject[128];
    char aProjectPath[128];
    char aNetDiskPath[128];
    char aLocalPath[128];
    sockaddr *pFirstControllerAddress;
    LPDWORD lpRunningThreadId;
	HANDLE hThreadHandle;
}EXECUTOR_INFORMATION;

#define UNIQUE_ID_FOR_EXECUTOR       "Dis_Executor"

#define TimeOut                 (UINT8)15

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

//MsgString类型
#define Ok                      "Ok"
#define NotOk                   "Not Ok"
#define NoMoreChecklist         "No More Checklist"
#define StateRunning            "Running"
#define StateOver               "Over"