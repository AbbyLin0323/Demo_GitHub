/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_SataISR.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.03.07    15:03:31
Description :Sata module interrupt service routine.
Others      :
Modify      :
****************************************************************************/

#include "BaseDef.h"

#ifdef SIM
#include "../satamodel/simsatadev.h"
#endif

#include "HAL_Define.h"
#include "HAL_BufMap.h"
#include "HAL_SataDefine.h"
#include "HAL_SataIO.h"
#include "HAL_SataISR.h"
#include "L1_Define.h"
#include "L1_HostCmdPRO.h"

extern U8  g_ulEncounterError;

U32 g_bAutoAckHost;
U32 g_bFlagPIODataRec;
/****************************************************************************
Name        :HAL_SataRcvCmdISR
Input       :
Output      :
Author      :HenryLuo
Date        :2012.03.07    15:06:35
Description :Interrupt service routine branch for SDC register H2D FIS receiving - command
Others      :
Modify      :
****************************************************************************/
void HAL_SataRcvCmdISR(void)
{
    U8 ucNewCmdTag;
    INTCALLBACKPARAM CurrIntParam;

    g_ulEncounterError = 0;
    
    ucNewCmdTag = HAL_SataGetCmdFromSRB();

    if (0 == g_ulEncounterError)
    {
        CurrIntParam.ucIntSrc = PARAM_INTSRC_SATA;
        CurrIntParam.ucEventType = CBINT_SATACMD;
        CurrIntParam.usShortParam = (U16)ucNewCmdTag;

        L1_IntCallBackProxy(&CurrIntParam);

#ifdef FW_CTRL_ALL_SDBFISREADY
        HAL_ClearSendSDBFISReady(ucNewCmdTag);
#endif
    }
    
#ifndef RAMDISK
#ifndef L2_FORCE_VIRTUAL_STORAGE
    L2_IntCallBackProxy(&CurrIntParam);
    L3_IntCallBackProxy(&CurrIntParam);
#endif
#endif
    return;
}

/****************************************************************************
Name        :HAL_RcvPIODataISR
Input       :
Output      :
Author      :HenryLuo
Date        :2012.03.07    17:54:37
Description :Interrupt service routine branch for SDC claiming the receipt of a PIO data H2D FIS
Others      :
Modify      :
****************************************************************************/
void HAL_RcvPIODataISR(void)
{
    g_bFlagPIODataRec = TRUE;

    rSDC_IntSrcPending = BIT_SDC_INTSRC_RXPIO_DATA;
    return;
}

/****************************************************************************
Name        :HAL_COMResetISR
Input       :
Output      :
Author      :HenryLuo
Date        :2012.03.07    17:55:00
Description :COM reset
Others      :
Modify      :
****************************************************************************/
void HAL_OOBDoneISR(void)
{
    //HAL_SataInitialize();  
#ifdef SIM_XTENSA
    rTracer = TL_FW_OOBDONE;
#endif       
    rSDC_IntSrcPending = BIT_SDC_INTSRC_OOB_DONE;
    
    rSDC_PHYControl15 = 2; /* Clear the safety protection for FIS/PMREQ trigger registers */
    HAL_SataSendGoodStatus();
    
    return;
}

void HAL_COMResetISR(void)
{
#ifdef SIM_XTENSA
    rTracer = TL_FW_COMRESET;
#endif   

    HAL_SataInitialize();
    
    rSDC_IntSrcPending = BIT_SDC_INTSRC_COMRESET_RCV;
    
    rSDC_PHYControl15 = 1; /* Inform hardware to send COMINIT and continue OOB sequence */

    //bit 13
    
    //rSDC_PHYControl15 = 2;

    //HAL_SataSendGoodStatus();
    return;
}


/****************************************************************************
Name        :HAL_SoftResetISR
Input       :
Output      :
Author      :HenryLuo
Date        :2012.03.07    17:55:26
Description :Software reset
Others      :
Modify      :
****************************************************************************/
void HAL_SoftResetISR(void)
{
    HAL_SataInitialize();
    HAL_SataSendGoodStatus();
    rSDC_IntSrcPending = BIT_SDC_INTSRC_SOFTRESET;
    return;
}


/****************************************************************************
Name        :HAL_EotPendingISR
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.03.07    17:56:21
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_EotPendingISR(void)
{
    HAL_SataSendGoodStatus();
    rSDC_IntSrcPending = BIT_SDC_INTSRC_EOTPENDING;
    return;
}

/****************************************************************************
Name        :HAL_ClearBridgeIntPendingISR
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.03.07    18:04:35
Description :Clear interrupt after interrupt process finish
Others      :
Modify      :
****************************************************************************/
void HAL_ClearBridgeIntPendingISR(void)
{
    ;
}

U8 sim_CLZ(U32 dwVal)
{
    U8 i;
    for(i=0; i<32; i++)
    {
        if( (1<<(31-i)) & dwVal )
            break;
    }
    return i;
}


void HAL_NCQCmdFinishISR(void)
{
    U8  tag;
    
 //   Dbg_AutoSetFISDelay();// for hw FisDelay test only. 

#ifdef SIM
    tag = sim_CLZ(rSDC_NCQCMD_HOLD_TAG);
#else
    tag = _TIE_via_CLZ(rSDC_NCQCMD_HOLD_TAG);
#endif

#ifdef SIM_XTENSA
    rTracer = tag;
#endif

    if (32 == tag)
    {
        return;
    }
    else
    {
        tag = 31 - tag;
    }
   
    rSDC_NCQCMD_CLR_TAG |= (1 << tag);

    HAL_HCmdSataDone(tag);

}

/****************************************************************************
Name        :HAL_SataISR
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.03.07    15:03:59
Description :Interrupt service routine entry for SDC/SACMDM/SDMAC
Others      :
Modify      :
****************************************************************************/
void HAL_SataISR(void)
{
    U32 uSDC_IntSrcPending;

#ifdef SIM_XTENSA
    rTracer = TL_SATA_INTERRUPT_ENTRY;
#endif

    uSDC_IntSrcPending = rSDC_IntSrcPending;
    
#ifdef SIM_XTENSA
        rTracer = TL_SATA_INTERRUPT_ENTRY + (uSDC_IntSrcPending & 0x00ffffff);
#endif

/*
    if (uSDC_IntSrcPending & BIT_SDC_INTSRC_SERROR)
    {
        L1_ErrHandle_SataSerrISR();
    }

    if (uSDC_IntSrcPending & BIT_SDC_INTSRC_SYNC_ESCAPE)
    {
        L1_ErrHandle_SyncEscapeISR();
    }
*/
    /* clear BIT_SDC_INTSRC_PENDING before  BIT_SDC_INTSRC_FIS_COMMAND */
    if (uSDC_IntSrcPending & BIT_SDC_INTSRC_PENDING)
    {
        rSDC_IntSrcPending = BIT_SDC_INTSRC_PENDING;
    }

    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_FIS_COMMAND )
    {
        U32 delay = 10; // For cycle length of latch shr is 22ns ,and of MCU is 4ns 
                        // to make sure the shr latch completed 
        /*latch SHR */
        rSDC_SHRLCH_EN = 1;

        while (delay--);   // delay 40ns 
        
#ifdef SIM
        SDC_LchShadowReg();
#endif

        HAL_SataRcvCmdISR();
        
#ifdef SIM
        rSDC_IntSrcPending &= ~BIT_SDC_INTSRC_FIS_COMMAND;
#else
        rSDC_IntSrcPending = BIT_SDC_INTSRC_FIS_COMMAND;
#endif
        
        rSDC_SHRLCH_EN = 0;

    }   

    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_RXPIO_DATA )
        HAL_RcvPIODataISR();

    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_OOB_DONE)
    {
        HAL_OOBDoneISR();
    }

    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_COMRESET_RCV)
    {
        HAL_COMResetISR();
    }

    if (uSDC_IntSrcPending & BIT_SDC_INTSRC_NCQFINISH)
    {
        HAL_NCQCmdFinishISR();
#ifdef SIM
        rSDC_IntSrcPending &= ~BIT_SDC_INTSRC_NCQFINISH;
#else
        rSDC_IntSrcPending = BIT_SDC_INTSRC_NCQFINISH; //lily: no need to clr.
#endif
    }

    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_SOFTRESET )
        HAL_SoftResetISR();
#if 0
    if( uSDC_IntSrcPending & BIT_SDC_INTSRC_FINISH )    /* add by henryluo 2011/06/23 for VT3443C */
        HAL_PrdFinishISR();

    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_EOTPENDING)
        HAL_EotPendingISR();
#endif 
 //   if(0 == uSDC_IntSrcPending)
 //       HAL_ClearBridgeIntPendingISR();

#ifdef SIM_XTENSA
    rTracer = TL_SATA_INTERRUPT_FINISH;
#endif
    return;
}

