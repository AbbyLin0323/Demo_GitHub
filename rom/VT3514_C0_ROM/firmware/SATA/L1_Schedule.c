/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : FlashFile.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"


#ifdef SIM
#include "../satamodel/simsatahost.h"
#include "../satamodel/simsatadev.h"
#else
#include <xtensa/tie/xt_timer.h>
#endif

//#include "../tracetool/Trace_Dev.h"

U32    LocalL1LoopCount;
U32    LocalL3LoopCount;
U32    LocalL2LoopCount;
U32    g_L1LoopCount;
U32    g_L3LoopCount;
U32    g_L2LoopCount;
U32    LocalPMLoopCount;
U32    LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_IDLE;
extern U16 g_CurrUsedReadDSGID ;//= INVALID_4F;
extern U16 g_CurrUsedWriteDSGID;// = INVALID_4F;

#ifdef SIM_DBG

U32       TaskStatus  ;
#endif

#ifdef TRACE_PERFORMANCE_AISC
extern volatile U32 gCurrentTime;
extern volatile U32 gIntvalTime;
extern volatile U32 gLastPCTriggerTime;

#define L1_PERFORMANCE_TRACE(FlagId) do {\
    gCurrentTime = XT_RSR_CCOUNT();\
    gIntvalTime = gCurrentTime - gLastPCTriggerTime;\
    gLastPCTriggerTime = gCurrentTime;\
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,(FlagId),0, gCurrentTime, gIntvalTime, 0);\
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,3,LOG_LVL_INFO,&g_DbgLogInfo);\
    }while(0)
#endif


#ifdef TRACE_PERFORMANCE_AISC
volatile U32 gCurrentTime;
volatile U32 gIntvalTime;
volatile U32 gLastPCTriggerTime = 0;
#endif
/****************************************************************************
Name        :L1_TaskSchedule
Input       :
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:46:30
Description :L1 task schedule.
Others      :
Modify      :
****************************************************************************/
U8 L1_TaskSchedule(void)
{
    if(COMM_EVENT_STATUS_BLOCKING == L1_TaskEventHandle())
        return g_ucL1returnflag;

    if(NULL == gCurHCMD)
    {
        gCurHCMD = L1_HostCMDSelect();
    }

    if(NULL != gCurHCMD)
    {
        g_ucL1returnflag = 0;
        HAL_HCmdProcessing(gCurHCMD->ucCmdTag);
        
        if(HCMD_TYPE_NONDATA == gCurHCMD->ucCmdType)
        {
            if ( TRUE == L1_TaskScheduleNoneData(gCurHCMD) )
            {
#ifndef FW_CTRL_ALL_SDBFISREADY
                HAL_SetSendSDBFISReady(gCurHCMD->ucCmdTag);
#endif
                gCurHCMD = NULL;
            }

            if(gpCurSubCmd != NULL)
            {
                DBG_Printf("HCmdType NONDATA gpCurSubCmd != NULL ERROR!!\n");
                DBG_Getch();
            }
        }
        else
        {
            /* Check multiple DRQ enable status for READ MULTIPLE and similar command */
            if ( FAIL == L1_SataCheckPIOMultipleEnable(gCurHCMD) ) {
                gCurHCMD->ulCmdRemSector = 0;
                HAL_SataSendAbortStatus();
            }

            if (gpCurSubCmd == NULL)
            {
                /* Split a SubCmd */
                gpCurSubCmd = L1_SplitHCMD(gCurHCMD);

                if (0 == gCurHCMD->ulCmdRemSector)
                {
                    gCurHCMD = NULL;
                }
            }
        }
    }
    else 
    {
        g_ucL1returnflag = 1;
    }

    L1_TaskSataIO(gpCurSubCmd);

    if (NULL != gpCurSubCmd && SUBCMD_STAGE_SATAIO == gpCurSubCmd->SubCmdStage)
    {
        gpCurSubCmd = NULL;
    }


    return g_ucL1returnflag;
}


BOOL L1_TaskScheduleNoneData(HCMD* pCurHCMD)
{
    BOOL bRtn = TRUE;
    
    switch( pCurHCMD->ucCmdCode )
    {
    case    ATA_CMD_IDENTIFY_DEVICE:
        L1_SataCmdIdentifyDevice();
        break;

    case    ATA_CMD_SET_FEATURES:
        L1_SataCmdSetFeatures();
        break;

    case    ATA_CMD_SET_MULTIPLE_MODE:
        L1_SataCmdSetMultipleMode();
        break;

    case    ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC:
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;            
        
        L1_SataCmdExecuteDeviceDiagnostic();
        break;

    case    ATA_CMD_CHECK_POWER_MODE:    //power management related commands
        L1_SataSendPowerMode(0xff);
        break;
/*
    case    ATA_CMD_INITIALIZE_DEVICE_PARAMETERS:
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;            
        
        L1_SataCmdInitializeDeviceParameters();
        break;

    case    ATA_CMD_CHECK_POWER_MODE:    //power management related commands
        L1_SataSendPowerMode();
        break;

    case    ATA_CMD_IDLE:
    case    ATA_CMD_IDLE_IMMEDIATE:
        L1_SataSetPowerMode(SATA_POWER_IDLE);
        break;

    case    ATA_CMD_STANDBY:
    case    ATA_CMD_STANDBY_IMMEDIATE:

        //blakezhang: for flush L1 buffers
        pCurHCMD->ucCmdRW = HCMD_SHUTDOWN;

        if(TRUE == MCU0PowerOffControl())
        {
#ifdef SIM
            Host_NoneDataCMDFinish();
#else
            HAL_SataSendSuccessStatus();
#endif
        }
        else
        {
            return FALSE;
        }

        break;

    case    ATA_CMD_SLEEP:
        L1_SataSetPowerMode(SATA_POWER_SLEEP);
        break;
*/
    #if 0   /* temp mask for fpga, 2014,1,2 haven */
    case    ATA_CMD_SMART:
        if (FALSE == L1_SataCmdSmart())
        {
            return FALSE;
        }
        break;
    #endif
/*
    case    ATA_CMD_DATA_SET_MANAGEMENT:
        if (FALSE == L1_SataCmdDataSetManagement())
        {
            return FALSE;
        }
        break;
*/
    case    ATA_CMD_VENDER_DEFINE:
        if (FALSE == L1_SataCmdVenderDefine())
        {
            return FALSE;
        }
        break;
/*
    case    ATA_CMD_SEEK:
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;  

        HAL_SataSendSuccessStatus();
        DBG_Printf("[sata]: seek command done\n");
        break;

    case    ATA_CMD_READ_VERIFY_SECTOR:
    case    ATA_CMD_READ_VERIFY_SECTOR_EXT:
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;            
        
        HAL_SataSendSuccessStatus();      
        break;

    case    ATA_CMD_FLUSH_CACHE:
    case    ATA_CMD_FLUSH_CACHE_EXT:
        if(FALSE == L1_CacheFlushAll())
            return FALSE;
        else
        {

#ifdef SIM
            Host_NoneDataCMDFinish();
#else
            HAL_SataSendSuccessStatus();
#endif


        }

        break;

    case    ATA_CMD_RECALIBRATE:
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;            

        HAL_SataSendSuccessStatus();

        break;

    case ATA_CMD_NCQ_NON_DATA:
        bRtn = L1_HandleNCQNonData(pCurHCMD);
        break;
        
    case ATA_CMD_SEND_FPDMA_QUEUED:
        bRtn = L1_HandleNCQSendFPDMA(pCurHCMD);
        break;
        
    case ATA_CMD_RECEIVE_FPDMA_QUEUED:
        bRtn = L1_HandleNCQReceiveFPDMA(pCurHCMD);
        break;
*/
    default:    //unsupported commands
        if( FALSE == HAL_SataIsFISXferAvailable() )
            return FALSE;        

        HAL_SataSendAbortStatus();

        break;
    }

    if (TRUE == bRtn)
    {
        rSDC_FW_Ctrl  |= CLR_PIOCMD_DATA;   //VT3514, clear small busy
        //HAL_SetSendSDBFISReady(pCurHCMD->ucCmdTag);//VT3514, allow hardware reporting good status to host 
    }
    
    return    bRtn;
}


/****************************************************************************
Name        :L1_TaskEventHandle
Input       :
Output      :
Author      :Peter Xiu
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
//extern U32 L1_DbgEventHandler(void);

U32 L1_TaskEventHandle(void)
{
    COMMON_EVENT L1_Event;
    U32 ulFreeDramBase;
    U32 ulFreeOTFBBase;
    COMM_EVENT_PARAMETER * pParameter;

    U32 Ret;

    Ret = COMM_EVENT_STATUS_BLOCKING;

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L1,&L1_Event))
    {
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

     if (L1_Event.EventInit)
    {
        //get parameter from comm event
        CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);

        //DWORD0 free dram address,DW1 free otfb sram address
        ulFreeDramBase = pParameter->EventParameterNormal[0];
        ulFreeOTFBBase = pParameter->EventParameterNormal[1];

        L1_TaskInit(&ulFreeDramBase,&ulFreeOTFBBase);
        HAL_Init(&ulFreeDramBase,&ulFreeOTFBBase);

        //update the free dram/sram address to parameter(status) buffer
        pParameter->EventParameterNormal[0] = ulFreeDramBase;
        pParameter->EventParameterNormal[1] = ulFreeOTFBBase;

        //clear event,init finished!
        CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_INIT);

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (L1_Event.EventErrorHandling)
    {
      //  L1_ErrorHandling();

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (L1_Event.EventLLF)
    {

    }
    else if (L1_Event.EventReset)
    {

    }
    else if (L1_Event.EventShutDown)
    {
     //   if (FALSE ==L1_TaskPMCheckBusy())
        {
            //clear event,L1 shutdown finished.
     //       CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_SHUTDOWN);
        }

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else
    {

    }

    return Ret;
}

void DRAM_ATTR MCU0TaskPM(U32 time,U32 idle)
{
    U32 ulSystemIdleCounter;

    //working stage only MCU do period clock-gating to save power
    if(idle == FALSE)
    {
        //working stage period sleep
        //HalPMSleep(time,FALSE,FALSE);
        return;
    }

    //idle stage

    rPM_TIMER1_START = 1;

    //system idle
    ulSystemIdleCounter = rPM_TIMER1_COUNTER;

    if(ulSystemIdleCounter< HAL_THRESHOLD_D0)
        return;
    else if(ulSystemIdleCounter < HAL_THRESHOLD_D1)
    {
        //idle 0
        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_IDLE);
        HalPMPLLOff();
        HalPMSleep(HAL_THRESHOLD_D2,FALSE,TRUE);

    }
    else if(ulSystemIdleCounter < HAL_THRESHOLD_D2)
    {
        //idle 1
        HalPMPLLOn();

        //prepare STR

        //take action
        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_IDLE);
        HalPMPLLOff();
        HalPMSleep(HAL_THRESHOLD_D3,FALSE,HAL_THRESHOLD_D3);
    }
    else if(ulSystemIdleCounter < HAL_THRESHOLD_D3)
    {
        //take action

        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_STANDBY);
        HalPMPowerOff();
    }
    else if(ulSystemIdleCounter < HAL_THRESHOLD_D4)
    {
        //Standby stage
        HalPMPLLOn();

        //perpare standby

        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_STANDBY);

        HalPMPLLOff();
        HalPMPowerOff();
    }
    else
    {
        //Sleep stage
        //not arrival here
        HalPMPLLOn();

        //wait ack
        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_STANDBY);

        HalPMPLLOff();
        HalPMPowerOff();
    }
}

void DRAM_ATTR MCU0SataPM()
{
    if(SATA_POWER_MODE_ACTIVE == HalSataGetPowerMode())
    {
        rPM_TIMER1_STOP = 1;
        rPM_TIMER1_COUNTER = 0;
    }
    else
    {
        HalPMSataPMSchedule();
    }

}
/****************************************************************************
Function  : CheckL1InputResource
Input     :  
Output    : PUFifoCnt
Return    :        
Purpose   :  Since SubCmd Fifo has been removed from L1 Ramdisk design, 
****************************************************************************/

void CheckL1InputResource(U16* L1HCmdCnt)
{
    *L1HCmdCnt = L1_HostCMDGetCount();

    return;
}

/****************************************************************************
Function  : CheckL2InputResource
Input     :  
Output    : BufReqNum
Return    :        
Purpose   :  
****************************************************************************/

void CheckL2InputResource(U16* BufReqNum)
{
    *BufReqNum = L1_PrioFifoGetPendingCnt();

    return;
}

/****************************************************************************
Function  : MCU0TaskRatio
Input     :  
Output    : none
Return    : 
           
Purpose   :  Assign L1, L3 task ration according to the Used random/sequential buffer count
Reference :


****************************************************************************/
void MCU0TaskRatio()
{
#if 0
    U16  HostCmdNum;
    U16  BufReqNum;
    U16  FlashReqNum;

    // Default value
    LocalL1LoopCount =  TaskSpeed_NORMAL;
    LocalL2LoopCount =  TaskSpeed_NORMAL;
    LocalL3LoopCount =  TaskSpeed_SLOW;

    //fetch resources
    CheckL1InputResource(&HostCmdNum);
    CheckL2InputResource(&BufReqNum);
    CheckL3InputResource(&FlashReqNum);

    //tune L1 task ratio - based on L1 input: SubCmdNum and HostCmdNum
    if (HOSTCMD_COUNT_UPTHS <= HostCmdNum)
    {
        LocalL1LoopCount = TaskSpeed_FAST;
    }

    if (HostCmdNum < HOSTCMD_COUNT_LWTHS)
    {
        LocalL1LoopCount = TaskSpeed_SLOW;
    }

    //tune L2 task ratio - based on L2 input: BufReqNum
    if (BUF_REQ_COUNT_LWTHS > BufReqNum)
    {
        LocalL2LoopCount = TaskSpeed_SLOW;
    }
    else if (BUF_REQ_COUNT_UPTHS <= BufReqNum)
    {
        LocalL2LoopCount = TaskSpeed_FAST;
    }

    //tune L3 task ratio - based on L3 input: FlashReqNum and RecycleNum    
    if (CE_FLUSH_COUNT_UPTHS < FlashReqNum)
    {
        LocalL3LoopCount = TaskSpeed_NORMAL;
    }


#else
    LocalL1LoopCount = 8;
    LocalL2LoopCount = 8;
    LocalL3LoopCount = 1;
#endif

    return; 

    /*PM task schedule counter add by peterxiu,default close*/
    LocalPMLoopCount = 0;

}

void MCU0TaskSchedule()
{
    L1_TaskSchedule();
}

void L1_TaskInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    L1_HostCMDProcInit(pFreeDramBase,pFreeOTFBBase);
    /*L1_CacheInit(pFreeDramBase,pFreeOTFBBase);
    L1_BufReqFifoInit(pFreeDramBase,pFreeOTFBBase);*/
    L1_SataCmdInit(pFreeDramBase,pFreeOTFBBase);
    /*L1_BufferInit(pFreeDramBase,pFreeOTFBBase) ;
    L1_DbgInit();*/
#ifdef L2_FORCE_VIRTUAL_STORAGE
    L2_VSSpaceAlloc(pFreeDramBase);
#endif
    gCurHCMD = NULL ;
    gpCurSubCmd = NULL;
    gCurSubCmd = 0;

    g_CurrUsedReadDSGID = INVALID_4F;
    g_CurrUsedWriteDSGID = INVALID_4F; 
    
}

U32 DRAM_ATTR L1_TaskPMCheckBusy()
{
//    U8 PuNum;
    
    /* current cmd must be processed done except standby cmd, add by henryluo for normal shutdown */
    if((L1_HostCMDGetCount() == 1) && (NULL != gCurHCMD) && 
        (ATA_CMD_STANDBY_IMMEDIATE == gCurHCMD->ucCmdCode || ATA_CMD_STANDBY == gCurHCMD->ucCmdCode))
    {
        ;
    }
    else
    {
        return TRUE;
    }

    //has subcmd not finishing,busy
    if (NULL != gpCurSubCmd)
        return TRUE;


    //has BufReq not finished, busy
    if (FALSE == L1_BufReqFifoEmpty())
        return TRUE;

    return FALSE;

}

U32 L2_TaskPMCheckBusy()
{
    return TRUE;
}

U32 L3_TaskPMCheckBusy()
{
    return TRUE;
}

U32 DRAM_ATTR MCU0PowerOffControl()
{
    COMMON_EVENT Event;

    switch (LocalPMPowerOffStage)
    {
    case L1_MCUPOWEROFF_STAGE_IDLE:
        LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_FLASHCACHE;
        break;

    case L1_MCUPOWEROFF_STAGE_FLASHCACHE:
        /*if(TRUE == L1_CacheFlushAll())*/
            LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L1_TRIGGER;
        break;

    case L1_MCUPOWEROFF_STAGE_L1_TRIGGER:
        CommSetEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_SHUTDOWN);
        LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L1_WAIT;
        break;

    case L1_MCUPOWEROFF_STAGE_L1_WAIT:
        CommCheckEvent(COMM_EVENT_OWNER_L1,&Event);
        if (FALSE == Event.EventShutDown)
        {
            LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L2_TRIGGER;
        }
        break;

    case L1_MCUPOWEROFF_STAGE_L2_TRIGGER:
        CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_SHUTDOWN);
        LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L2_WAIT;
        break;

    case L1_MCUPOWEROFF_STAGE_L2_WAIT:
        CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);
        if (FALSE == Event.EventShutDown)
        {
            LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L3_TRIGGER;
        }
        break;

    case L1_MCUPOWEROFF_STAGE_L3_TRIGGER:
        CommSetEvent(COMM_EVENT_OWNER_L3,COMM_EVENT_OFFSET_SHUTDOWN);
        LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_L3_WAIT;
        break;

    case L1_MCUPOWEROFF_STAGE_L3_WAIT:
        CommCheckEvent(COMM_EVENT_OWNER_L3,&Event);
        if (FALSE == Event.EventShutDown)
        {
            LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_DONE;
        }
        break;

    case L1_MCUPOWEROFF_STAGE_DONE:
        LocalPMPowerOffStage = L1_MCUPOWEROFF_STAGE_IDLE;
        return TRUE;
        break;

    default:
        break;
    }

    return FALSE;
}
/****************************************************************************
Function  : MCU0TaskIDLE
Input     : none
Output    : none
Return    :  if system idle return TRUE;
if system busy return FALSE;
Purpose   : flush cache,idle action,system pm idle status maintain
Reference :


****************************************************************************/
BOOL DRAM_ATTR MCU0TaskIDLE()
{
    U32 bBusy;
    U32 bIdle;
    U32 PowerStage;

    /*no host command,no subcmd ,idle*/
    bBusy = L1_TaskPMCheckBusy() | L2_TaskPMCheckBusy() | L3_TaskPMCheckBusy();

    if(TRUE == bBusy)
    {
        //working
        HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_WORKING);
        rPM_TIMER0_STOP = 1;
        rPM_TIMER0_COUNTER = 0;
        return FALSE;
    }

    //system idle

    //only do idle task in PM stage WORKING,if other stage,stop idle action
    //so if host issue STAND_BY or Sleep command,IDLE action is blocked

    PowerStage = HalPMGetSystemPowerStage();

    if(SYSTEM_POWER_MODE_WORKING == PowerStage)
    {
        bIdle = TRUE;

        /*//idle task 1
        if(L1_CacheNeedIdleFlush())
        {
            //idle task 1
            //idle flush cache
            L1_CacheIdleFlush();
            bIdle = FALSE;
        }*/

        if(bIdle == FALSE)
        {
            HalPMSetSystemPowerStage(SYSTEM_POWER_MODE_WORKING);
            rPM_TIMER1_STOP = 1;
            rPM_TIMER1_COUNTER = 0;
            return FALSE;
        }
        else
        {
            rPM_TIMER1_STOP = 0;
            rPM_TIMER1_START = 1;
        }
    }

    return TRUE;
}

/*===============================================================
                            FakeL1
=================================================================*/
#ifdef FAKE_L1
DRAM_ATTR void Fake_L1_Schedule(void)
{
    L2_FakeL1PtnGenRand();
}

#endif
/********************** FILE END ***************/

