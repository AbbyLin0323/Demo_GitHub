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
Filename    :HAL_SataIO.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Sata interface function
Others      :
Modify      :
*******************************************************************************/

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_HostInterface.h"
#include "HAL_GLBReg.h"
#include "HAL_SataDSG.h"
#include "HAL_BufMap.h"
#include "HAL_SataIO.h"
#ifdef SIM
#include "simsatadev.h"
#else
#include "HAL_PM.h"
#endif
#include "HAL_ParamTable.h"
#include "COM_BitMask.h"

extern PTABLE *g_pBootParamTable;

void HAL_SataEnableNcqFinishInt(void)
{
    rSDC_IntMask &= ~(BIT_SDC_INTSRC_NCQFINISH);
    rSDC_SHR_LockControl |= BIT_FW_CMDINT_EN;
    //DBG_Printf("Enable NFInt\n");
    return;
}

void HAL_SataDisableNcqFinishInt(void)
{
    rSDC_SHR_LockControl &= (~BIT_FW_CMDINT_EN);
    rSDC_IntMask |= BIT_SDC_INTSRC_NCQFINISH;

    //DBG_Printf("Disable NFInt\n");
    return;
}

void HAL_SataEnableCmdRcvInt(U32 ulEnable)
{
    if (FALSE == ulEnable)
    {
        rSDC_IntMask |= BIT_SDC_INTSRC_FIS_COMMAND;
    }

    else
    {
        rSDC_IntMask &= ~BIT_SDC_INTSRC_FIS_COMMAND;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SDCResetDmaAndCmd
Description: 
    reset DMA and CMD module of SDC to initial status, and then make them normal run.
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in boot/init stage and error handling.
History:
    20141016    Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SDCResetDmaAndCmd(void)
{
    rGlbSoftRst |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
    rGlbSoftRst &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
}

/*------------------------------------------------------------------------------
Name: HAL_SDCHoldDmaAndCmd
Description: 
    hold/keep DMA and CMD module of SDC in initial status
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in error handling.
History:
    20141016    Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SDCHoldDmaAndCmd(void)
{
    rGlbSoftRst |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
}

/*------------------------------------------------------------------------------
Name: HAL_SDCReleaseDmaAndCmd
Description: 
    make DMA and CMD module of SDC to normal status
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called after error handling.
History:
    20141016    Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SDCReleaseDmaAndCmd(void)
{
    rGlbSoftRst &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
}

/*------------------------------------------------------------------------------
Name: HAL_SDCResetICB
Description: 
    reset ICB interface of SDC
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    called in error handling.
History:
    20141016    Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SDCResetICB(void)
{
    rGlbSoftRst |= R_RST_SDC_ICB;
    rGlbSoftRst &= ~R_RST_SDC_ICB;
}

/*------------------------------------------------------------------------------
Name: HAL_SDCFeatureInit
Description: 
    Init SDC features:
    1. FIS delay parameter
    2. FW flag for HW start COM_INIT
    3. HW do not send interrupt to MCU when finish one command
Input Param:
    U8 ucBuffSizeBits: bits of buff size. (e.g. 15 means 32K (2 ^ 15 = 32K))
Output Param:
    none
Return Value:
    none
Usage:
    called in boot stage before release reset of SDC.
History:
    20141204  Gavin   ported from HAL_SataInitialize
    20150603  Gavin   add buff size as input param
------------------------------------------------------------------------------*/
void HAL_SDCFeatureInit(U8 ucBuffSizeBits)
{
#ifndef SIM
    volatile PMUREGSET *pPMURegBlk = (volatile PMUREGSET *)REG_BASE_PMU;
#endif

#ifdef FIS_Delay
    rSDC_FISDelayControl |= TXSETDEVCOMPL_DLY_EN | 0xC0;
#endif
    
    /* enable HW to wait rSDC_SendSDBFISReady when DMA command */
    rSDC_ControlRegister |= FW_DMAEXE_STE_EN | DIPM_EN | HW_SLUMBER_EN /*| HW_PARTIAL_EN */;
    
    /* SDB FIS ready, default all valid */
    rSDC_SendSDBFISReady = 0xffffffff;

    /*  PMU_Regitser_Base: 0x1ff81f00
        REG10[0] : ASR OOB done, set second interrupt or not.
        REG10[1] : HW reset TP/Link when COMReset.
        REG10[4] : SDCBLKDMACMD_EN, HW will block SDC_DMAC when COMReset interrupt FW or Serror.
                    need to clear HD_Err_clr(SDC_offset_0xCC:bit18).
        REG10[5] : 0 = FW_COMINIT_RDY is from PMU register FW_COMINIT_RDY_SUS= REG10[22];
                    1 = FW_COMINIT_RDY is from SATA register 1ff80812[0]
        REG10[6] : need to set COMINIT_EN after COMreset interrupt.  Inform hardware to send COMINIT and continue OOB sequence

        REG10[7] : FW Decode enable
        REG10[9] : when FW ready, set 1, HW send COMINIT.  total switch for REG10[21]/REG10[22]
       */
#ifndef SIM
    //*(volatile U32 *)(REG_BASE_PMU + 0x10) |= 0x2df;
    pPMURegBlk->ulSATAMiscCfg |= 0x2DF;
#endif

    /* Initialize SDMAC */
    rSDMAC_DataBuffBase = (ucBuffSizeBits - SIZE_4K_BITS) | DUMMYDATA | HOLDXFER;

    /* Initialize SGE to SATA mode */
    rSGESataMode |= SGE_SATAMODE_ENABLE;

    /* If HIPM feature is not enabled by P-Table, disables hardware from acknowledging HIPM requests from host. */
    if (FALSE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0HIPMEnable)
    {
        rSDC_LinkControl |= (0x3 << 8);
    }

    rSDC_PHYControl1 |= (1<<5);   // SATA internal OOB register 

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataInitialize
Description: 
    initialize all SATA HW interface
Input Param:
    U8 ucBuffSizeBits: bits of buff size. (e.g. 15 means 32K (2 ^ 15 = 32K))
Output Param:
    none
Return Value:
    none
Usage:
    called in boot/init and error handling.
History:
    20140211  kristinwang  BUFFSIZE_4K->BUFFSIZE_32K,different from 0.99
    20141016  Gavin   modify for coding style
    20150603  Gavin   add buff size as input param
    20151116  Nina    remove ASR enable code in FW,ASR enable or not controlled by HW_init script.
------------------------------------------------------------------------------*/
void HAL_SataInitialize(U8 ucBuffSizeBits)
{
    //SDC internal function selection
    rSDC_PHYControl14 |= R_ERR_SRC_SEL;
    rSDC_LKControl |= LK_ROK_DELAY_SEL;

    //ASR feature
    rSDC_PHYControl3 &= ~EN_STOP_ASR;
    //rSDC_PHYControl5 |= ASR_SUPPORT_EN;//AsynRecoverSupport
    rSDC_PHYControl6 |= INC_OOB_BURST;
    rSDC_PHYControl14 |= EPHY_IDLE_DELAY_SEL;//ELECIDLE delay

    /* Initialize SDC interrupt mask */
    rSDC_IntMask |= (BIT_SDC_INTSRC_TXOP |
                        BIT_SDC_INTSRC_RXDMACMPL |
                        BIT_SDC_INTSRC_EOTPENDING |
                        BIT_SDC_INTSRC_LPM |
                        BIT_SDC_INTSRC_NCQFINISH |
                        BIT_SDC_INTSRC_FIFOFULL |
                        BIT_SDC_INTSRC_SOFTRESET_FIRST |
                        BIT_SDC_INTSRC_SOFTRESET |
                        BIT_SDC_INTSRC_INFER_EIDLE);// add msk FIFOfull bruce

    /* Enable COMRESET holding BuffMap module hardware patch */
    rSDC_PTDef |= (1 << 2);

    rSDC_SHR_LockControl |= BIT_SOFTWARE_RST_SUPPORT;

    /* Clr NCQ_CMD state when CMD reset */
    rSDC_PHYControl3 |= CLR_NCQ_CMD_EN;

    HAL_SDCFeatureInit(ucBuffSizeBits);

    /* Clears possible PIO command pending status inherited from ROM code in MP procedure. */
    HAL_SataClearPIOCmdPending();

    /* Release reset signal for SATA */
    rGlbSoftRst &= ~(R_RST_SDC_DMA | R_RST_LPHY | R_RST_SDC_CMD | R_RST_SDC);

    return;
}

/*==============================================================================
Func Name  : HAL_SataClearBigBusy
Input      : NONE
Output     : NONE
Return Val : 
Discription: Inform hardware that command receiving has been completed
Usage      : 
History    : 
    1. 2015.01.13 Kristin create
==============================================================================*/
void HAL_SataClearBigBusy(void)
{
    rSDC_FW_Ctrl = FW_CFGCMD_DONE;
    return;
}
/*==============================================================================
Func Name  : HAL_SataClearPIOCmdPending
Input      : void
Output     : NONE
Return Val : 
Discription: clear SDC internal pending flag for PIO command.
Usage      : 
             call this function after complete a PIO/NONDATA command
             Note:
             for DMA/NCQ command, SDC clear the pending flag by itself
History    : 
    1. 2015.02.02 Gavin create function
==============================================================================*/
void HAL_SataClearPIOCmdPending(void)
{
    rSDC_FW_Ctrl = CLR_PIOCMD_DATA; 
}

/*==============================================================================
Func Name  : HAL_SataFeedbacktoHW
Input      : 
        U8 ucCmdTag: command tag
        U8 ucCmdProtocol: protocol: PIO/DMA/NCQ/NONDATA
Output     : NONE
Return Val : 
Discription: when receive a host command, fw shall feedback to the hardware
             several information. including:
             1. decode what protocol of this host command.
             2. which tag of this host command used.
             3. clear big busy. 
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
    2. 2014.10.15 Gavin modfiy input param format
==============================================================================*/
void HAL_SataFeedbacktoHW(U8 ucCmdTag, U8 ucCmdProtocol)
{
    U8 ucClearHwValue;

    /* FW Decode program : command tag & command protocol */
    rSDC_FW_DECODE = (ucCmdTag << 3) | ucCmdProtocol;


    /* Reset hardware NCQ state machine to idle */
    if (SDC_PROTOCOL_NCQ != ucCmdProtocol)
    {
        ucClearHwValue = (FW_CFGCMD_DONE | FW_CLR_NCQEXE);
    }
    else
    {
#ifdef SIM
        /* Clears BSY bit in Device Status register. */
        rSDC_COMMAND_STATUS = 0x50;
#endif
        ucClearHwValue = FW_CFGCMD_DONE;
    }

    /* clear big busy, inform hardware that command receiving has been completed */
    rSDC_FW_Ctrl = ucClearHwValue;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetNcqOutStandingCmd
Description: 
    get the bitmap of current outstanding NCQ command
Input Param:
    void
Output Param:
    none
Return Value:
    U32: bit0~31 indicates command tag0~31
Usage:
    called in error handling case.
History:
    20141016  Gavin   created
------------------------------------------------------------------------------*/
U32 HAL_GetNcqOutStandingCmd(void)
{
    return rSDC_NCQOutstd;
}

/*------------------------------------------------------------------------------
Name: HAL_SataClearNcqOutstdReg
Description: 
    clear rSDC_NCQOutstd
Input Param:
    void
Output Param:
    none
Return Value:
    void
Usage:
    called in error handling case.
History:
    20150115  Kristin   created
------------------------------------------------------------------------------*/
void HAL_SataClearAllNcqOutstd(void)
{
    rSDC_NCQOutstd = 0;
    return;
}

void HAL_SataSetAllSendSDBFISReady(void)
{
    rSDC_SendSDBFISReady = INVALID_8F;
    return;
}

void HAL_SataClearAllSendSDBFISReady(void)
{
    rSDC_SendSDBFISReady = 0;
    return;
}

void HAL_SataClearSendSDBFISReady(U8 ucCmdTag)
{
    rSDC_SendSDBFISReady &= (~(1 << ucCmdTag));
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataClearHWHoldErr
Description: 
    Set HD_Err_clr to release DMA and CMD module held by HW
Input Param:
    void
Output Param:
    none
Return Value:
    void
Usage:
    called in COMRESET, software reset and SError handling
History:
    20150115  Kristin   created
------------------------------------------------------------------------------*/
void HAL_SataClearHWHoldErr(void)
{
    rSDC_FW_Ctrl = HD_Err_clr;
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataClearComResetBlock
Description: 
    Set FW_CLR_COMRESET_BLOCK to clear FW Trigger Register Block Signal
Input Param:
    void
Output Param:
    none
Return Value:
    void
Usage:
    called in COMRESET, software reset and SError handling
History:
    20150115  Kristin   created
------------------------------------------------------------------------------*/
void HAL_SataClearComResetBlock(void)
{
    rSDC_PHYControl15 |= FW_CLR_COMRESET_BLOCK;
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataIsDmaBusy
Description: 
    Bit 3 of rSDMAC_ConfigReg4 indicates the SDC DMA Module is busy or free
Input Param:
    void
Output Param:
    none
Return Value:
    BOOL: TRUE - the SDC DMA Module is busy
          FALSE - the SDC DMA Module is free
Usage:
    called in error handling case.
History:
    20150114  Kristin   created
------------------------------------------------------------------------------*/
BOOL HAL_SataIsDmaBusy(void)
{
    return (0 != (rSDMAC_ConfigReg4 & (1 << 3)));
}

/*------------------------------------------------------------------------------
Name: HAL_SataIsSdcIdle
Description: 
    check if SDC finish transaction for all commands
Input Param:
    void
Output Param:
    none
Return Value:
    BOOL: TRUE - SDC is idle
          FALSE - There is command pending in SDC
Usage:
    called in error handling/power management case.
History:
    20150318  Gavin   ported from L0_SataErrorHandling.c
    20150319  Gavin   add referring to rSDC_PwrAndCmdStatus
------------------------------------------------------------------------------*/
BOOL HAL_SataIsSdcIdle(void)
{
    BOOL bIdle;

#ifndef SIM
    if ((0 == (rSDC_PwrAndCmdStatus & SDC_ALL_CMD_DONE)) ||
        (0 != rSDC_IOControl) )
    {
        bIdle = FALSE;
    }
    else
#endif
    {
        bIdle = TRUE;
    }

    return bIdle;
}

#if 0
/*------------------------------------------------------------------------------
Name: HAL_SataRejectCommandOnReceiving
Description: 
    reject a host command after receive CMD FIS
Input Param:
    U8 ucCmdTag: the command tag which need to reject
Output Param:
    none
Return Value:
    none
Usage:
    called in error handling case.
History:
    20141016  Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SataRejectCommandOnReceiving(U8 ucCmdTag)
{
    rSDC_FW_DECODE = (ucCmdTag<<3) | SDC_PROTOCOL_PIO;

    rSDC_FW_Ctrl  = (FW_CFGCMD_DONE | FW_CLR_NCQEXE);

    HAL_SataSendAbortStatus();
    HAL_SataClearPIOCmdPending();

    return;
}
#endif

/*===================================================================
Function   :     ATA_TP_SendPIODataFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a PIO data D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendPIODataFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDPIODATA_FIS;
    return;
}

/*===================================================================
Function   :     ATA_TP_SendPIOSetupFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a PIO setup D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendPIOSetupFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDPIOSETUP_FIS;
    return;
}

/*===================================================================
Function   :     HAL_SataSendRegD2HFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a register D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendRegD2HFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDREGD2H_FIS;
#ifdef SIM
    SDC_SetD2HFis();
#endif
    return;
}

/*===================================================================
Function   :     ATA_TP_SendSetDevBitFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a set device bits D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendSetDevBitFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDSDB_FIS;
    return;
}

/*===================================================================
Function   :    ATA_TP_IsFISXferAvailable
Input      :     none
Output     :    The status of whether hardware is ready to send a FIS
Description :   Return a TRUE status when hardware is capable for sending out a FIS 
Modify History:
20090703         Yao Chen 001: first created
===================================================================*/
BOOL HAL_SataIsFISXferAvailable(void)
{
#ifndef SIM
    if(0 == rSDC_IOControl)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return TRUE;
#endif
}

/*===================================================================
Function   :     HAL_SataSendGoodStatus
Input      :     none
Output     :     none
Description :   send good status in COMRESET and software reset protocol, 
                or Device Reset command protocol
Note: 
Modify History:
20090528    Yao Chen    001: first created
20150115    Kristin     modify code comments
===================================================================*/
void HAL_SataSendGoodStatus(void)
{
    LOCK_SHADOW_REG();//protect on
    
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 1;
    rSDC_LBALOW = 1;
    rSDC_LBAMID = 0;
    rSDC_LBAHIGH = 0;
    rSDC_SECCNT = 1;
    rSDC_DEVICE_HEAD = 0;
    rSDC_FISDirInt &= (~BIT_SDC_FIS_INTFLAG);
#ifndef SIM
    HAL_SataSendRegD2HFIS();
    while (FALSE == HAL_SataIsFISXferAvailable());
#endif
    rSDC_FEATURE_ERROR = 0;
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*===================================================================
Function   :     HAL_SataSignatureSendGoodStatus
Input      :     none
Output     :     none
Description :   send good status in EXECUTE DEVICE DIAGNOSTIC command 
                protocol, I = 1
Note: 
Modify History:
20090528    Yao Chen    001: first created
20150115    Kristin     modify code comments
===================================================================*/
void HAL_SataSignatureSendGoodStatus(void)
{
    LOCK_SHADOW_REG();//protect on
    
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 1;
    rSDC_LBALOW = 1;
    rSDC_LBAMID = 0;
    rSDC_LBAHIGH = 0;
    rSDC_SECCNT = 1;
    rSDC_DEVICE_HEAD = 0;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;

    HAL_SataSendRegD2HFIS();
    while (FALSE == HAL_SataIsFISXferAvailable());

    rSDC_FEATURE_ERROR = 0;
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataSendAbortStatus
Description: 
    send D2H FIS to host to abort a command 
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    1. PIO,DMA command not supported, address outside of the range of user-accessible,
       or error occurs during data transfer
    2. NCQ command malformed
    3. reject command when disk locked
History:
    20120405    Henry       Create
    20150115    Kristin     modify code comments
------------------------------------------------------------------------------*/

void HAL_SataSendAbortStatus(void)
{
    while (FALSE == HAL_SataIsFISXferAvailable())
    {
        ;
    }

    LOCK_SHADOW_REG();//protect on
    
    rSDC_COMMAND_STATUS = 0x51;//BSY=0,DRQ=0,ERR=1
    rSDC_FEATURE_ERROR = 4;//ABRT=1
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;//I=1
    HAL_SataSendRegD2HFIS();

    while ( FALSE == HAL_SataIsFISXferAvailable() )
    {
        ;
    }
    rSDC_FEATURE_ERROR = 0;
    rSDC_COMMAND_STATUS = 0x50;//BSY=0,DRQ=0,ERR=0
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/****************************************************************************
Name        :HAL_SataSendSuccessStatus
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.04.05    15:58:25
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_SataSendSuccessStatus(void)
{
    while (FALSE == HAL_SataIsFISXferAvailable())
    {
        ;
    }

    LOCK_SHADOW_REG();//protect on
    
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
    HAL_SataSendRegD2HFIS();

    while (FALSE == HAL_SataIsFISXferAvailable())
    {
        ;
    }
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataSendSDBUncorrectableError
Description: 
    send SDB FIS to host if NCQ command data contains an uncorrectable error 
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    Sata L0 error handling for UECC
History:
    20120405    Henry       Create
    20150115    Kristin     001 modify function name HAL_SataSendErrorStatus ->
                                HAL_SataSendSDBUncorrectableError
                            002 modify code comments
------------------------------------------------------------------------------*/
void HAL_SataSendSDBUncorrectableError(void)
{
    while (FALSE == HAL_SataIsFISXferAvailable())
    {
    }

    LOCK_SHADOW_REG();//protect on
    
    rSDC_FEATURE_ERROR = 0x40;//Uncorrectable Error
    rSDC_COMMAND_STATUS = 0x51;//ERR=1
    rSDC_NCQActive = 0;//ACT = 0
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;//I=1
    HAL_SataSendSetDevBitFIS();

    while (FALSE == HAL_SataIsFISXferAvailable())
    {
    }
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataSendSDBQueueCleanACT
Description: 
    send SDB FIS to host to clean queue ACT
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    error handling after receiving a Read Log Ext -10h, called before processing
    Read Log Ext -10h
History:
    20150115    Kristin       Create
------------------------------------------------------------------------------*/
void HAL_SataSendSDBQueueCleanACT(void)
{
    /* wait other FIS send done */
    while (FALSE == HAL_SataIsFISXferAvailable())
    {
    }

    LOCK_SHADOW_REG();//protect on
    
    rSDC_FEATURE_ERROR = 0;
    rSDC_COMMAND_STATUS = 0x50;//ERR=0
    rSDC_NCQActive = INVALID_8F;//ACT = 0xFFFFFFFF
    rSDC_FISDirInt = 0;//I=0
    HAL_SataSendSetDevBitFIS();

    while (FALSE == HAL_SataIsFISXferAvailable())
    {
    }
    
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SataGetOrigCFIS
Description: 
    copy CFIS from SDC IRS to local.
Input Param:
    none
Output Param:
    U32 *pTargetCFIS: pointer to memory for saving CFIS(5 DWs)
Return Value:
    void
Usage:
    called in interrupt due to SDC receive new command
History:
    20150121    Gavin   create function
------------------------------------------------------------------------------*/
void HAL_SataGetOrigCFIS(U32 *pTargetCFIS)
{
    *pTargetCFIS++ = rSDC_AtaCFisDW0;
    *pTargetCFIS++ = rSDC_AtaCFisDW1;
    *pTargetCFIS++ = rSDC_AtaCFisDW2;
    *pTargetCFIS++ = rSDC_AtaCFisDW3;
    *pTargetCFIS = rSDC_AtaCFisDW4;

    return;
}

/****************************************************************************
Name:    HAL_SataConstructAndSendPIOSetupFIS
Input:    4 parameters
bFirstDRQ - The DRQ block is the first one within a host command;
bLastDRQ - The DRQ block is the last one within a host command;
ucDRQLen - The length of current DRQ block in sector;
bRead - TRUE: The current host command uses PIO data in;
Output:    No output required.
Author:    Yao Chen
Date:    2012.09.18
Description: Build and send a PIO setup FIS for a DRQ block with information carried by 4 parameters.
Others:
Modify:
****************************************************************************/
void HAL_SataConstructAndSendPIOSetupFIS (
    const BOOL bFirstDRQ,
    const BOOL bLastDRQ,
    const U8 ucDRQLen,
    const BOOL bRead )
{
    /* 1. Enable firmware programming for shadow register block. */
    LOCK_SHADOW_REG();

    /* 2. Status when data is ready shall has BUSY = 0, DRDY = 1, DRQ = 1 and ERR = 0.
    Error shall never be reported by a PIO Setup FIS. */
    rSDC_COMMAND_STATUS = 0x58;
    rSDC_FEATURE_ERROR = 0;

    /* 3. Program transfer length field in the PIO Setup FIS. */
    rSDC_PIOXferCountLow =  (U8)(ucDRQLen << SEC_SIZE_BITS);
    rSDC_PIOXferCountHigh = (U8)(ucDRQLen << (SEC_SIZE_BITS - 8));

    /* 4. Decide the ending status and interrupt request/data direction according to IN/OUT protocol and first/last flag. */
    if ( TRUE == bRead ) {
        /* Interrupt bit and direction bit are always required for a read command. */
        rSDC_FISDirInt = BIT_SDC_FIS_DIRFLAG | BIT_SDC_FIS_INTFLAG;

        if ( TRUE == bLastDRQ )
            /* If last data block is ready, busy shall be cleared after data transfer according to PIO data in protocol. */
            rSDC_END_STATUS = 0x50;

        else
            /* Busy shall be set after data transfer if all data blocks are not finished according to PIO data in protocol. */
            rSDC_END_STATUS = 0xD0;
    }

    else {
        /* Busy shall always be set after one data block transfer in a write command
        because device needs to flush data into its media. */
        rSDC_END_STATUS = 0xD0;

        if ( TRUE == bFirstDRQ )
            /* There is no need for device to interrupt host when it gets ready for receiving the first data block. */
            rSDC_FISDirInt = 0;

        else
            /* In subsequent data transfer, getting ready for receiving a data block
            also means the previous data block has been written into media. So host needs to be notified. */
            rSDC_FISDirInt = BIT_SDC_FIS_INTFLAG;
    }

    /* 5. Trigger TP layer to send out the PIO Setup FIS. */
    HAL_SataSendPIOSetupFIS();

    /* 6. Check the transfer status and disable firmware programming for shadow register block. */
    //while ( FALSE == HAL_SataIsFISXferAvailable() );
    UNLOCK_SHADOW_REG();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_SetSendSDBFISReady
Description: 
    set Send SDB FIS ready.
Input Param:
    U8 ucCmdTag: the command tag
Output Param:
    none
Return Value:
    none
Usage:
    not very clear
History:
    20141016  Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_SetSendSDBFISReady(U8 ucCmdTag)
{
    //SDC wait corresponding bit before send SDB FIS
    rSDC_SendSDBFISReady |= (1<<ucCmdTag);

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_ClearSendSDBFISReady
Description: 
    clear Send SDB FIS ready
Input Param:
    U8 ucCmdTag: the command tag
Output Param:
    none
Return Value:
    none
Usage:
    not very clear
History:
    20141016  Gavin   modify for coding style
------------------------------------------------------------------------------*/
void HAL_ClearSendSDBFISReady(U8 ucCmdTag)
{

    rSDC_SendSDBFISReady &= (~(1 << ucCmdTag));

}

/****************************************************************************
Name        :HAL_GetSATALinkState
Input       :None.
Output      :Current SATA link layer state machine state.
Author      :Yao Chen
Date        :
Description : This routine reads SATA link layer state machine state from hardware register.
Others      :
Modify      :
20141016    Gavin   ported from HAL_PM.c
****************************************************************************/
LOCAL INLINE U8 HAL_GetSATALinkState(void)
{
    /* Top 6 bits in Reg 0xC8 */
    return ( rSDC_LinkStatus >> 8 );
}

/****************************************************************************
Name        :HAL_GetSDMACStatus
Input       :None.
Output      :SATA DMA controller is busy or not.
Author      :Yao Chen
Date        :
Description : This routine checks whether SDMAC is busy.
Others      :
Modify      :
20141016    Gavin   ported from HAL_PM.c
****************************************************************************/
LOCAL INLINE BOOL HAL_GetSDMACStatus(void)
{
    /* LSB of SDMAC Reg 0x30 */
    return ( rSDMAC_Status & 0x1 );
}

/****************************************************************************
Name        :HAL_GetHostAck
Input       :None.
Output      :Receiving PM_ACK/PM_NAK status from host.
Author      :Yao Chen
Date        :
Description : This routine checks link PM acknowledgement from host.
Others      :
Modify      :
20141016    Gavin   ported from HAL_PM.c
****************************************************************************/
LOCAL INLINE U8 HAL_GetHostAck(void)
{
    /* Bottom 2 bits in Reg 0xC8 */
    return (rSDC_LinkStatus & 0x3);
}

/****************************************************************************
Name        :HAL_SendLPMRequest
Input       :ucPMReq - Specifies the request type that hardware shall issue.
Output      :None.
Author      :Yao Chen
Date        :
Description : This routine invokes SATA link layer to send out a DIPM request or COMWAKE
                    signal.
Others      :
Modify      :
20141016    Gavin   ported from HAL_PM.c
****************************************************************************/
INLINE void HAL_SendLPMRequest(U8 ucPMReq)
{
    /* Bit 15:12 -- Request state */
    rSDC_LinkControl = ( rSDC_LinkControl & ~( 0xF << 12 ) ) | ( ucPMReq << 12 );

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_SataIsSlumber
Description: 
    check if sata link entered into slumber 
Input Param:
    void
Output Param:
    none
Return Value:
    BOOL: TRUE - Slumber
             FALSE - Normal or Partial
Usage:
    called in error handling/power management case.
History:
    20150427 Victor Zhang Create 
------------------------------------------------------------------------------*/
BOOL HAL_SataIsSlumber(void)
{
    return (DPM_PWR_STATE_IF_IN_SLUMBER == HAL_GetSATALinkState());
}


/*--- end of file HAL_SataIO.c ---*/

