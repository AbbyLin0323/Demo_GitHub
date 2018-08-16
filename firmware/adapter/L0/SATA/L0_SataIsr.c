/****************************************************************************
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
*****************************************************************************
Filename    :L0_SataIsr.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "HAL_Interrupt.h"
#include "HAL_BufMap.h"
#include "HAL_SataIO.h"
#include "HAL_TraceLog.h"
#ifdef SIM
#include "simsatadev.h"
#endif
#include "L0_Event.h"
#include "L0_Interface.h"
#include "L0_SataIsr.h"
#include "L0_HcmdChain.h"
#include "L0_Pio.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"
#include "L0_SataErrorHandling.h"

//specify file name for Trace Log
#define TL_FILE_NUM     L0_SataIsr_c

extern U32 g_ulATAInfoIdfyPage;
extern U32 g_ulHostInfoAddr;
extern U16 g_usSecurityStatus;
extern U32 g_ulPowerStatus;
extern volatile U32 g_ulSataBistMode;

void L0_SataReceiveCmdIsr(void)
{
    // this function should be called when the sata command
    // interrupt is raised
    U32 ulCmdCode;
    U32 ulCmdTag;
    HCMD* pNewCmd;

    // fetch the host command code from latch shadow register
#ifdef SIM
    ulCmdCode = (rSDC_AtaCFisDW0 >> 16) & MSK_2F;
#else
    ulCmdCode = rSDC_SHRLCH_COMMAND;
#endif

    // set the host command tag number for ncq commands
    if (TRUE == L0_ATACheckNCQCmdCode(ulCmdCode))
    {
#ifdef SIM
        ulCmdTag = (rSDC_AtaCFisDW3 & MSK_2F) >> 3;
#else
        ulCmdTag = rSDC_SHRLCH_NCQTAG;
#endif
    }
    else
    {
        // the tag number for non-ncq commands is always 0
        // (including trim)
        ulCmdTag = 0;
    }

    // fetch the entry of host command slots based on the tag
    pNewCmd = &HostCmdSlot[ulCmdTag];

    // save the current host command tag
    pNewCmd->tCbMgr.SlotNum = ulCmdTag;

    // save original CFIS for ATA Lib processing, the CFIS will be
    // parsed later
    pNewCmd->tCbMgr.CFis = &pNewCmd->tCFis;
    HAL_SataGetOrigCFIS((U32 *)(pNewCmd->tCbMgr.CFis));
    pNewCmd->tCbMgr.Ch = &pNewCmd->tCh; // just for adaptation to ATA Lib, this variable is useless in sata mode

    // try parsing the incoming host command, a serious of
    // tests will be conducted to ensure the validity of the
    // incoming command
    if(TRUE == L0_SataParseIncomingCmd(&pNewCmd->tCbMgr))
    {
        // set the start LBA for the incoming host command
        pNewCmd->ulStartLba = pNewCmd->tCbMgr.CurrentLBA;

        // the incoming command has passed all tests, add it to
        // L0 host command chain
        L0_AddNewHostCmd(ulCmdTag);
    }
    else
    {
        // the incoming command has failed to pass all tests,
        // mark the command as PIO type, and abort it by sending
        // D2H FIS later
        HAL_SataFeedbacktoHW(pNewCmd->tCbMgr.SlotNum, SDC_PROTOCOL_PIO);
    }

    return;
}

BOOL L0_SataIsCommandFIS(void)
{
    CFIS tH2DFis;

    HAL_SataGetOrigCFIS((U32*)&tH2DFis);

    return (BOOL)(tH2DFis.C);
}

void L0_SataISR(void)
{
    // this is the major function for handling interrupts
    // in sata mode, all interrupts should be processed in
    // this function
    U32 uSDC_IntSrcPending;

    // get the pending interrupts bitmap from the sdc register,
    // every bit in this register denotes a specific interrupt
    uSDC_IntSrcPending = (rSDC_IntSrcPending) & ~(rSDC_IntMask);


    // handle all kinds of interrupts accordingly

    // s-error interrupt
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_SERROR)
    {
        L0_SataSerrISR();
    }

    /*
     [1st SRST INT]: Check (Control INT status = 1) & (1st SRST INT status = 1)
     [2nd SRST INT]: 
       When OOB status is sent between the asserting of soft-reset and de-assert of soft-reset, the interrupt of de-assert of soft reset will lose
       Workaround: Check (Control INT status =1 ) & ((1st SRST INT status = 0) | (SRST bit = 0))
     */
    if (uSDC_IntSrcPending & BIT_SDC_INTSRC_FIS_CONTROL)
    {
        rSDC_IntSrcPending = BIT_SDC_INTSRC_FIS_CONTROL;
        if (rSDC_IntSrcPending & BIT_SDC_INTSRC_SOFTRESET_FIRST)
        {
            // 1st SRST INT
            rSDC_IntSrcPending = BIT_SDC_INTSRC_SOFTRESET_FIRST;
            if ((rSDC_CONTROL_ALTSTATUS & BIT_SDC_CONTROL_SRST) == 0)
            {
                // 2nd SRST INT
                L0_SoftResetISR();
                rSDC_IntSrcPending = BIT_SDC_INTSRC_FIS_CONTROL;
            }
        }
        else
        {
            // 2nd SRST INT
            L0_SoftResetISR();
        }
    }

    // incoming host command
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_FIS_COMMAND)
    {
        // if the device status is sleep, then firmware can't do any command
        if(SATA_POWER_SLEEP == g_ulPowerStatus)
        {
            HAL_SataFeedbacktoHW(0, SDC_PROTOCOL_PIO);
            rSDC_IntSrcPending = BIT_SDC_INTSRC_FIS_COMMAND;
            return;
        }

#ifndef SIM
        // latch the shadow register
        rSDC_SHRLCH_EN = TRUE;

        // wait until the shadow register is 1
        while(FALSE == rSDC_SHRLCH_EN)
        {
            ;
        }
#endif
        // receiving the incoming host command
        L0_SataReceiveCmdIsr();

        // clear the incoming command interrupt bit
#ifdef SIM
        rSDC_IntSrcPending &= ~BIT_SDC_INTSRC_FIS_COMMAND;
#else
        rSDC_IntSrcPending = BIT_SDC_INTSRC_FIS_COMMAND;

        // release the SDC latch
        rSDC_SHRLCH_EN = FALSE;
#endif
    }   

    // clear BIT_SDC_INTSRC_PENDING before BIT_SDC_INTSRC_FIS_COMMAND
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_PENDING)
    {
        rSDC_IntSrcPending = BIT_SDC_INTSRC_PENDING;
    }

    // LPM interrupt, please note that both HIPM abd DIPM
    // mechanisms will raise an interrupt here
    // Yao Chen: Firmware does not need to be aware of this event.
#if 0
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_LPM)
    {
        DBG_Printf("BIT_SDC_INTSRC_LPM\n");
        rSDC_IntSrcPending = BIT_SDC_INTSRC_LPM;
    }
#endif

    // pio data receipt interrupt, this interrupt will be raised
    // when pio data is sent from the host
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_RXPIO_DATA)
    {
        L0_ReceivePioDataIsr();

        rSDC_IntSrcPending = BIT_SDC_INTSRC_RXPIO_DATA;
    }

    // OOB interrupt
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_OOB_DONE)
    {
        //DBG_Printf("BIT_SDC_INTSRC_OOB_DONE\n");bruce
        g_ulSataBistMode = 0;
        L0_OOBDoneISR();
    }

    // com reset interrupt
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_COMRESET_RCV)
    {
        //DBG_Printf("BIT_SDC_INTSRC_COMRESET_RCV\n");
        if (g_ulSataBistMode == 1)
            L0_BIST_L();
#if 1
        L0_SataMarkReset(SATAERR_TYPE_COMRESET_FIRST);
        rSDC_IntSrcPending = BIT_SDC_INTSRC_COMRESET_RCV;
#else
        //stop SDC from sending any FIS due to some BufMap/FDR is set
        HAL_HoldBuffMap();
        HAL_SDCHoldDmaAndCmd(); //add hold DMAANDCMD bruce data Fis
        L0_COMResetISR();
#endif
    }

    // ncq command finish interrupt, please note that this
    // interrupt shouldn't be raised under normal circumstance
    // and ncq commands should be completed automatically
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_NCQFINISH)
    {
        //DBG_Printf("BIT_SDC_INTSRC_NCQFINISH\n");bruce
        L0_NCQCmdFinishISR();
    }

    // eot pending isr
    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_EOTPENDING)
    {
        L0_EotPendingISR();
    }

    if(uSDC_IntSrcPending & BIT_SDC_INTSRC_BISTRCV)
    {
        L0_BIST_L();
        rSDC_IntSrcPending = BIT_SDC_INTSRC_BISTRCV;
    }

#if 0  //closed by blakezhang: DBG_Print every time when enter SATA INT
    if(rSDC_IntSrcPending != 0)
    {
        // at this point, all interrupts should be handled and
        // cleared, if there are still pending interrupts, we
        // might have a potential error here
        DBG_Printf("unhandled pending interrupts, rSDC_IntSrcPending: %x\n", rSDC_IntSrcPending);
    }
#endif

    return;
}

void L0_OOBDoneISR(void)
{
    L0_SataMarkReset(SATAERR_TYPE_COMRESET);

    // clear the OOB interrupt
#ifndef SIM
    rSDC_IntSrcPending = BIT_SDC_INTSRC_OOB_DONE;
#else
    rSDC_IntSrcPending &= ~BIT_SDC_INTSRC_OOB_DONE;
#endif
    return;
}

void L0_COMResetISR(void)
{
    rSDC_IntSrcPending = BIT_SDC_INTSRC_COMRESET_RCV;

    //FW is ready and inform HW to send cominit to host
    *(volatile U32 *)(REG_BASE_PMU + 0x10) |= (1 << 6);

    return;
}

void L0_SoftResetISR(void)
{
    //DBG_Printf("Software Reset interrupt 2 received\n");
    
    //Giga 2015.8.20 : Marked this DealyCycle() & Set SDC Offset 0xB8 bit 11 CONTROL_INT_MSK to 1.
    //HAL_DelayCycle(50000);//bruce        
    rSDC_IntMask |= BIT_SDC_INTSRC_FIS_CONTROL;
    
    L0_SataMarkReset(SATAERR_TYPE_SOFTWARE_RESET);

    // clear the soft reset interrupt
    rSDC_IntSrcPending = BIT_SDC_INTSRC_SOFTRESET;
    return;
}

void L0_NCQCmdFinishISR(void)
{
    U8  ucTag;
    U32 ulLBA;
    U32 ulSecCnt;

    /* Clear NCQ Command Tag */
    ucTag = HAL_CLZ(rSDC_NCQCMD_HOLD_TAG);
    if (32 != ucTag)
    {
        ucTag = 31 - ucTag;
    }   
    rSDC_NCQCMD_CLR_TAG |= (1 << ucTag);
    //DBG_Printf("Rcv NFInt\n");

    //if (ucTag == L0_SataIsUECCTag())
    if (TRUE == L0_SataIsEncounterUECC())
    {
        ulLBA = HostCmdSlot[ucTag].ulStartLba;
        ulSecCnt = HostCmdSlot[ucTag].tCbMgr.HCMDLenSec;

        L0_SataMarkCmdError(SATAERR_TYPE_NCQ_UECC, ucTag, ulLBA, ulSecCnt);
        L0_SataClearUECCMsgRcv();

        HAL_SataClearAllSendSDBFISReady();
        HAL_SataDisableNcqFinishInt();
    }
    else
    {
        //DBG_Printf("L0_NCQCmdFinishISR:not this UECC tag\n");
    }
    //HAL_SataDisableNcqFinishInt();

    rSDC_IntSrcPending = BIT_SDC_INTSRC_NCQFINISH;

    return;
}

void L0_SataSerrISR(void)
{
    U32 ulSerialErrorStatus = (rSDC_ErrorStatus & 0x1FFF);

    L0_SataMarkSError((U16)ulSerialErrorStatus);
    
    if ((ulSerialErrorStatus & (BIT_SDC_ERROR_STATUS_RXNRTE | BIT_SDC_ERROR_STATUS_RXNRDE)) &&
        ((ulSerialErrorStatus & BIT_SDC_ERROR_STATUS_RXPCLE) == 0))
    {
        //HAL_SataClearHWHoldErr();
        HAL_SDCHoldDmaAndCmd();
        HAL_HoldBuffMap();
    }

    /* Clears serial error status for hardware. */
    rSDC_ErrorStatus = ulSerialErrorStatus;

    /* Clears SDC interrupt pending status. */
    rSDC_IntSrcPending = BIT_SDC_INTSRC_SERROR;

    return;
}

void L0_EotPendingISR(void)
{
    rSDC_IntSrcPending = BIT_SDC_INTSRC_EOTPENDING;
    return;
}

void L0_BIST_L(void)
{
    U8 CurrentSpeed;
    static U8 ucSDC_18, ucSDC_07;
    static U32 ulEPHY_60, ulEPHY_64, ulEPHY_68;
    static U8 ucSDC_00, ucSDC_06, ucEPHY_07, ucEPHY_0B, ucSDC_01;
    static U32 ulEPHY_30, ulEPHY_14, ulEPHY_58, ulEPHY_5C, ulEPHY_24;
    static U32 ulEPHY_00, ulEPHY_04, ulEPHY_34, ulEPHY_38, ulEPHY_40;
    static U32 ulSDC_08, ulSDC_10;
    static U32 ulEPHY_1C, ulEPHY_44, ulEPHY_20;
    static U32 ulEPHY_2C, ulEPHY_48, ulEPHY_4C, ulEPHY_54;

    if (g_ulSataBistMode == 0)
    {
        DBG_Printf("BIST FIS\n");
        ucSDC_18 = *(volatile U8 *)0x1ff80818;
        ulEPHY_60 = *(volatile U32 *)0x1ff83c60;
        ulEPHY_64 = *(volatile U32 *)0x1ff83c64;
        ulEPHY_68 = *(volatile U32 *)0x1ff83c68;

        //SATA Release
        *(volatile U32 *)0x1ff80018 = 0x20000000;
        //SATA TX EPHY Driving
        *(volatile U32 *)0x1ff83c60 = 0x00048000;
        *(volatile U32 *)0x1ff83c64 = 0x00650012;
        *(volatile U32 *)0x1ff83c68 = 0x0065003F;

        ucSDC_00 = *(volatile U8 *)0x1ff80800;
        ucSDC_06 = *(volatile U8 *)0x1ff80806;
        ucEPHY_07 = *(volatile U8 *)0x1ff83c07;
        ucEPHY_0B = *(volatile U8 *)0x1ff83c0b;
        ulEPHY_14 = *(volatile U32 *)0x1ff83c14;
        ulEPHY_58 = *(volatile U32 *)0x1ff83c58;
        ulEPHY_5C = *(volatile U32 *)0x1ff83c5C;
        ulEPHY_00 = *(volatile U32 *)0x1ff83c00;
        ulEPHY_04 = *(volatile U32 *)0x1ff83c04;
        ulEPHY_38 = *(volatile U32 *)0x1ff83c38;

        CurrentSpeed = (U8)((rSDC_FW_Ctrl16 & 0x1800) >> 11);

        if (CurrentSpeed== 0x00)
        {
            DBG_Printf("Gen1 Speed\n");
            //SATA GEN1
            ulEPHY_1C = *(volatile U32 *)0x1ff83c1c; //Gen1
            ulEPHY_20 = *(volatile U32 *)0x1ff83c20; //Gen1
            ulEPHY_2C = *(volatile U32 *)0x1ff83c2c; //Gen1
            ucSDC_07 = *(volatile U8 *)0x1ff80807;

            //set the SATA work at test mode and at GEN1(C0), GEN2(D0) or GEN3(E0). HFTP(00),MFTP(01),LFTP(02)
            *(volatile U8 *)0x1ff80800 = 0x0;
            //Force PhyRdy LB_RX_Retime EnInsert2Align ForecGen3
            *(volatile U8 *)0x1ff80806 = 0x28;

            //yccdebug 0314 disable Gen2 and Gen3 speed
            *(volatile U8 *)0x1ff80807 = 0x018;
            //set EPHY the work at GEN1(4)
            *(volatile U8 *)0x1ff83c07 = 0x20;
            *(volatile U8 *)0x1ff83c0b = 0x80;

            *(volatile U32 *)0x1ff83c1c = 0x11001119;
            *(volatile U32 *)0x1ff83c14 = 0x00002000;

            //yccdebug 0328
            *(volatile U32 *)0x1ff83c58=0xAAA8000F;
            *(volatile U32 *)0x1ff83c5C=0xAAA90164;
            *(volatile U32 *)0x1ff83c38=0x003FFC00;

            //yccdebug RX setting
            *(volatile U32 *)0x1ff83c00 = 0x63041003;
            *(volatile U32 *)0x1ff83c04 = 0x00333EA0;

            *(volatile U32 *)0x1ff83c20 = 0x06A70060;
            *(volatile U32 *)0x1ff83c24=0x003FFC05;
            *(volatile U32 *)0x1ff83c2c=0xffa5c5fa;
        }
        else if (CurrentSpeed== 0x01)
        {
            DBG_Printf("Gen2 Speed\n");
            //SATA GEN2 
            ulEPHY_30 = *(volatile U32 *)0x1ff83c30;
            ulEPHY_24 = *(volatile U32 *)0x1ff83c24;
            ulEPHY_34 = *(volatile U32 *)0x1ff83c34;
            ulEPHY_40 = *(volatile U32 *)0x1ff83c40;

            //set the SATA work at test mode and at GEN1(C0), GEN2(D0) or GEN3(E0). HFTP(00),MFTP(01),LFTP(02)
            *(volatile U8 *)0x1ff80800 = 0x20;
            //Force PhyRdy LB_RX_Retime EnInsert2Align ForecGen3
            *(volatile U8 *)0x1ff80806 = 0x68;
            //set EPHY the work at GEN2(5)
            *(volatile U8 *)0x1ff83c07 = 0x20;
            *(volatile U8 *)0x1ff83c0b = 0xa0;

            *(volatile U32 *)0x1ff83c30 = 0x11001119;

            *(volatile U32 *)0x1ff83c14 = 0x00002000;

            //yccdebug 0328
            *(volatile U32 *)0x1ff83c58=0xAAA8000F;
            *(volatile U32 *)0x1ff83c5C=0xAAA90164;
            *(volatile U32 *)0x1ff83c24=0x003FFC95;
            //yccdebug RX setting
            *(volatile U32 *)0x1ff83c00 = 0x63041003;
            *(volatile U32 *)0x1ff83c04 = 0x00333EA0;

            *(volatile U32 *)0x1ff83c34 = 0x06A70060;
            *(volatile U32 *)0x1ff83c38 = 0x003FFC00;
            *(volatile U32 *)0x1ff83c40 = 0xF550C5FA;
        }
        else if (CurrentSpeed== 0x02)
        {
            DBG_Printf("Gen3 Speed\n");
            //SATA GEN3 
            ucSDC_01 = *(volatile U8 *)0x1ff80801; //Gen3
            ulEPHY_44 = *(volatile U32 *)0x1ff83c44; //Gen3
            ulSDC_08 = *(volatile U32 *)0x1ff80808; //Gen3
            ulSDC_10 = *(volatile U32 *)0x1ff80810; //Gen3
            ulEPHY_24 = *(volatile U32 *)0x1ff83c24; //Gen2 //Gen3
            ulEPHY_48 = *(volatile U32 *)0x1ff83c48; //Gen3
            ulEPHY_4C = *(volatile U32 *)0x1ff83c4c; //Gen3
            ulEPHY_54 = *(volatile U32 *)0x1ff83c54; //Gen3

            //set the SATA work at test mode and at GEN1(C0), GEN2(D0) or GEN3(E0). HFTP(00),MFTP(01),LFTP(02)
            *(volatile U8 *)0x1ff80800 = 0x10;
            //Force PhyRdy LB_RX_Retime EnInsert2Align ForecGen3
            *(volatile U8 *)0x1ff80806 = 0xa8;
            //set Test  pattern
            *(volatile U8 *)0x1ff80801 = 0x60;
            //set EPHY the work at GEN3(6)
            *(volatile U8 *)0x1ff83c07 = 0x20;
            *(volatile U8 *)0x1ff83c0b = 0xc0;

            *(volatile U32 *)0x1ff83c44 = 0x11001119;

            *(volatile U32 *)0x1ff83c14 = 0x00002000;

            //OOB enhance
            *(volatile U32 *)0x1ff80808 = 0x10;
            //E-IDLEDET sel 
            *(volatile U32 *)0x1ff80810 = 0x480200;
            //MPLL delay = 00
            *(volatile U32 *)0x1ff83c00 = 0x63041003 ;

            //yccdebug 0328
            *(volatile U32 *)0x1ff83c58=0xAAA8000F;
            *(volatile U32 *)0x1ff83c5C=0xAAA90164;
            *(volatile U32 *)0x1ff83c38=0x003FFED0;
            *(volatile U32 *)0x1ff83c24=0x003FFC95;

            //yccdebug RX setting
            *(volatile U32 *)0x1ff83c00 = 0x63041003;
            *(volatile U32 *)0x1ff83c04 = 0x00333EA0;

            *(volatile U32 *)0x1ff83c48 = 0x07A80060;
            *(volatile U32 *)0x1ff83c4c = 0x2D0;
            *(volatile U32 *)0x1ff83c54 = 0xF050C5FA;
        }
        else
        {
            DBG_Printf("Unknown speed\n");
        }
        DBG_Printf("BIST_L ...\n");
        g_ulSataBistMode = 1;
    }
    else
    {
        *(volatile U8 *)0x1ff80800 = ucSDC_00;
        *(volatile U8 *)0x1ff80806 = ucSDC_06;
        *(volatile U8 *)0x1ff83c07 = ucEPHY_07;
        *(volatile U8 *)0x1ff83c0b = ucEPHY_0B;
        *(volatile U32 *)0x1ff83c14 = ulEPHY_14;
        *(volatile U32 *)0x1ff83c58 = ulEPHY_58;
        *(volatile U32 *)0x1ff83c5C = ulEPHY_5C;
        *(volatile U32 *)0x1ff83c00 = ulEPHY_00;
        *(volatile U32 *)0x1ff83c04 = ulEPHY_04;
        *(volatile U32 *)0x1ff83c38 = ulEPHY_38;
        DBG_Printf("Restore non-BIST_L setting completed\n");

        while (!(rSDC_IntSrcPending & BIT_SDC_INTSRC_OOB_DONE))
            ;
        CurrentSpeed = (U8)((rSDC_FW_Ctrl16 & 0x1800) >> 11);

        if (CurrentSpeed== 0x00)
        {
            DBG_Printf("Gen1 Speed\n");
            *(volatile U32 *)0x1ff83c1c = ulEPHY_1C; //Gen1
            *(volatile U32 *)0x1ff83c20 = ulEPHY_20; //Gen1
            *(volatile U32 *)0x1ff83c2c = ulEPHY_2C; //Gen1
            *(volatile U8 *)0x1ff80807 = ucSDC_07;

        }
        else if (CurrentSpeed== 0x01)
        {
            DBG_Printf("Gen2 Speed\n");
            *(volatile U32 *)0x1ff83c30 = ulEPHY_30; //Gen2
            *(volatile U32 *)0x1ff83c24 = ulEPHY_24;
            *(volatile U32 *)0x1ff83c34 = ulEPHY_34;
            *(volatile U32 *)0x1ff83c40 = ulEPHY_40;
        }
        else if (CurrentSpeed== 0x02)
        {
            DBG_Printf("Gen3 Speed\n");
            *(volatile U8 *)0x1ff80801 = ucSDC_01; //Gen3
            *(volatile U32 *)0x1ff83c44 = ulEPHY_44; //Gen3
            *(volatile U32 *)0x1ff80808 = ulSDC_08; //Gen3
            *(volatile U32 *)0x1ff80810 = ulSDC_10; //Gen3
            *(volatile U32 *)0x1ff83c24 = ulEPHY_24;
            *(volatile U32 *)0x1ff83c48 = ulEPHY_48; //Gen3
            *(volatile U32 *)0x1ff83c4c = ulEPHY_4C; //Gen3
            *(volatile U32 *)0x1ff83c54 = ulEPHY_54; //Gen3
        }
        else
        {
            DBG_Printf("Unknown speed\n");
        }
        *(volatile U8 *)0x1ff80818 = ucSDC_18;
        *(volatile U32 *)0x1ff83c60 = ulEPHY_60;
        *(volatile U32 *)0x1ff83c64 = ulEPHY_64;
        *(volatile U32 *)0x1ff83c68 = ulEPHY_68;
    }
}

