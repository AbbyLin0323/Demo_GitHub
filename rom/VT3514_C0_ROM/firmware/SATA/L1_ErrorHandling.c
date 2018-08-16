/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : L1_ErrorHandling.c
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2013.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_GLBReg.h"
#include "HAL_SataIO.h"
#include "L1_ErrorHandling.h"
#include "COM_Event.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define ET_NOTSUPPORTCMD    1
#define ET_SERROR           2
#define ET_SYNCESCAPE       3
#define ET_REDUNDANTTAG     4

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
/* g_ulEncounterError: when FW Encounter a error occur at ISR, it may be a 
 * HW error (Sata link), or FW not support this command/or subcommand, 
 * then we set this flag to 1, indicate that we did't input this tag to HCMD_Q.
 */
U8  g_ulEncounterError = 0;

#if 0
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
static void FW_ResetDSG(void)
{
    
}

static void FW_ResetHostCmdQueue(void)
{
    L1_HostCmdVarInit();
}

static void FW_ResetSubCmdQueue(void)
{
    
}

static void FW_ResetBufReqQueue(void)
{
    
}

void FW_Reset(void)
{
    // Reset DSG 
    FW_ResetDSG();
    
    // reset L1 host command/sub-command queue
    FW_ResetHostCmdQueue();

    // reset L1 buffer request queue

    // reset L2/L3 related queues
}

void L1_ErrHandle_NotSupportCmd(void)
{
    // reset GLB Register
    /*rGLB_18 |= R_RST_SDC_CMD; // to be confirm
    rGLB_18 &= ~R_RST_SDC_CMD;*/

    // Send Abort FIS
    HAL_SataSendAbortStatus();

    // Reinitialize configuration parameters
    HAL_SataInitialize();

    // Firmware Reset
    FW_Reset();

    g_ulEncounterError = 1;

#ifdef SIM_XTENSA
    DBG_Printf("L1_ErrHandle_NotSupportCmd()\n");
    //DBG_Getch();
#endif
    
}

void L1_ErrHandle_SataLinkErr(void)
{
    // reset GLB Register
    rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; // to be confirm
    rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);

    // Send Abort FIS
    HAL_SataSendAbortStatus();

    // Reinitialize SDMAC configuration parameters


    // Firmware Reset
    //FW_Reset();
}


/* error handle flow for device response(receive)R-ERR to(from) host. jasonguo*/
void L1_ErrHandle_SataSerrISR(void)
{
    /* Transmitting Register FIS Failed . 
       Work Flow: 
       IF fail occurred when tx DataFIS, 
            THEN sdc send S-Err interrupt to fw, and fw do something.
       ENDIF;
       IF fail occurred when tx NonDataFIS, 
            THEN sdc auto retrey, 
            IF repeat 3 times fail, 
                THEN sdc send S-Err interrupt to fw, and fw do something.
            ENDIF, 
       ENDIF;
    */
    U16 usErrorStatus = rSDC_ErrorStatus;
    if (usErrorStatus & (BIT_SDC_ERROR_STATUS_RXNRPE|BIT_SDC_ERROR_STATUS_RXNRTE))
    {
        /* Fw reset Hw DMAC and CMD model */
        rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
        rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
       
        /* set Err */
        Lock_ShadowRegister();
        rSDC_FEATURE_ERROR  = 0;
        rSDC_COMMAND_STATUS = BIT_SDC_COMMAND_STATUS_DRDY | BIT_SDC_COMMAND_STATUS_CD;//0x50;
        UnLock_ShadowRegister();

        /* send D2HFIS/SDBFIS to Host */
        while (FAIL == HAL_SataIsFISXferAvailable());
        rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
        if (rSDMAC_CmdType == HCMD_PROTOCOL_NCQ)
        {
            HAL_SataSendSetDevBitFIS();
        }
        else
        {
            HAL_SataSendRegD2HFIS();
        }
    }
    /* Receiving Register FIS Failed .
       Work Flow: 
       IF fail occurred when rx DataFIS, 
            THEN sdc send S-Err interrupt to fw, and fw do something.
       ENDIF;
       IF fail occurred when rx NonDataFIS, 
            THEN sdc send S-Err interrupt to fw, and fw clr int and return. 
       ENDIF;
    */
    else
    {
        /* Error Occurred in Link Layer When Receiving Data Fis */
        if (usErrorStatus & BIT_SDC_ERROR_STATUS_RXNRDE)
        {
            /* Fw reset Hw DMAC and CMD model */
            rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
            rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
        
            /* set Err */
            Lock_ShadowRegister();
            rSDC_FEATURE_ERROR  = 0;
            rSDC_COMMAND_STATUS = BIT_SDC_COMMAND_STATUS_DRDY | BIT_SDC_COMMAND_STATUS_CD;
            UnLock_ShadowRegister();

            /* send D2HFIS/SDBFIS to Host */
            while (FAIL == HAL_SataIsFISXferAvailable());
            rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
            if (rSDMAC_CmdType == HCMD_PROTOCOL_NCQ)
            {
                HAL_SataSendSetDevBitFIS();
            }
            else
            {
                HAL_SataSendRegD2HFIS();
            }
        }  
        /* Error Occurred in Link Layer When Receiving NonData Fis, do nothing except clr int */
    }

    /* Firmware Reset */
    //FW_Reset();
    L1_SetErrorEvent(ET_SERROR);

    /* clear Serr status bits */
    rSDC_ErrorStatus = usErrorStatus;
    
    /* clear Serr Interrupt*/
    rSDC_IntSrcPending = BIT_SDC_INTSRC_SERROR;
}

/* error handle flow for SYNC-ESCAPE. jasonguo*/
void L1_ErrHandle_SyncEscapeISR(void)
{
    /* Fw reset Hw DMAC and CMD model */
    rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
    rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
    
    /* Firmware Reset */
    FW_Reset();

    /* clear Serr Interrupt*/
    rSDC_IntSrcPending = BIT_SDC_INTSRC_SYNC_ESCAPE;
}

/* error handle for host send NCQ cmd with overridden tag . jasonguo*/
BOOL L1_ErrHandle_CheckOverriddenCmdTag(U8 curCmdTag)
{
    if (rSDC_NCQOutstd & (1<<curCmdTag))
    {
        /* Fw reset Hw DMAC and CMD model */
        rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
        rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);

        /* Send Abort FIS */
        HAL_SataSendAbortStatus();

        /* Firmware Reset */
        FW_Reset();
        
        return TRUE;
    }
    
    return FALSE;
}

void L1_Sim_FwTrigDevicePowerMgt(void)
{
    U16 usLinkStatus;
    U8  ucSataPwrState;
    
    static U32 s_ulCnt = 0;
    if (++s_ulCnt%50)
    {
        return;
    }
    
    /* ucSataPwrState[15:8]:
    00h: Device not present or communication not established.
    01h: Interface in active state
    02h: Interface in Partial power management state.
    06h: Interface in Slumber power management state.
    */
    usLinkStatus = rSDC_LinkStatus;
    switch(usLinkStatus >> 8)
    {
        case DPM_PWR_STATE_IF_IN_ACTIVE:
            ucSataPwrState = DPM_PWR_STATE_IF_IN_PARTIAL;
            break;
        case DPM_PWR_STATE_IF_IN_PARTIAL:
            ucSataPwrState = DPM_PWR_STATE_IF_IN_SLUMBER;
            break;
        case DPM_PWR_STATE_IF_IN_SLUMBER:
            ucSataPwrState = DPM_PWR_STATE_IF_IN_ACTIVE;
            break;
        default:
            ucSataPwrState = DPM_PWR_STATE_IF_IN_ACTIVE;
            break;
    }
    
    rSDC_LinkStatus = (ucSataPwrState << 8) | (usLinkStatus & 0x00FF); 
}




void L1_SetErrorEvent(U32 ulEventType)
{
    COMM_EVENT_PARAMETER * pParameter;

    //get parameter from comm event
    CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);

    pParameter->EventParameterNormal[0] = ulEventType;

    switch (ulEventType)
    {
        case ET_SERROR:
             //pParameter->EventParameterNormal[1] = rSDC_ErrorStatus;
             break;
        case ET_NOTSUPPORTCMD:
        case ET_REDUNDANTTAG:
        case ET_SYNCESCAPE:
             break;
        default:
             break;
    }

    //set errorhandling event
    CommSetEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_ERR);
}
#endif 
void L1_ErrorHandling(void)
{
/*
    COMM_EVENT_PARAMETER * pParameter;
    U32 ulEventType;
    
    //get parameter from comm event
    CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);

    //DWORD0 free dram address,DW1 free otfb sram address
    ulEventType = pParameter->EventParameterNormal[0];

    switch (ulEventType)
    {
        case ET_SERROR:
        case ET_NOTSUPPORTCMD:
        case ET_REDUNDANTTAG:
        case ET_SYNCESCAPE:
             FW_Reset();
             break;
        default:
             break;
    }


    //clear event,init finished!
    CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_ERR);
*/    
}

/*====================End of this file========================================*/

