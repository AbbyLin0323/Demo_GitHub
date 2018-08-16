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
Filename    :L0_ATALibAdapter.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_SataIO.h"
#include "HAL_BufMap.h"
#include "HAL_TraceLog.h"
#include "L0_Config.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#include "L0_Pio.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"
#include "L0_Schedule.h"
#include "L0_SataErrorHandling.h"
#include "HAL_ParamTable.h"
#include "COM_Memory.h" 

//specify file name for Trace Log
#define TL_FILE_NUM     L0_ATALibAdapter_c

extern U32 g_ulSubsysNumBits;
extern U32 g_ulSysLBAMax;
extern U32 g_ulUsedLBAMax;
GLOBAL RD2HFIS g_tRFisOfSataMode; //D2H FIS, used to save result for command type: HCMD_TYPE_NEED_SPECIAL_STATUS
extern U16 g_usSecurityStatus;
extern PIOINFO gPIOInfoBlock;
extern FW_UPDATE g_tFwUpdate;
extern U32 g_ulATAGPLBuffStart;
extern void L0_UpgradeBootloader(U32 ulFwSlot);
extern void L0_UpgradeSpiRom(U32 ulFwSlot);
extern BOOL L0_FwUpgradeCheckImgValid(void);
extern GLOBAL VCM_PARAM g_VCM_Param;

void L0_AhciDataXferPrepRespFISSeq(PCB_MGR pSlot)
{
    if (SATA_PROT_PIO == pSlot->SATAProtocol)
    {
        //invoke PIO IN/OUT handling
        L0_SataSetupPIOInOut(((pSlot->IsWriteDir) ? TRUE : FALSE),
                            pSlot->PIODRQSize,
                            pSlot->HCMDLenSec);
    }

    return;
}

//dummy function for compatibility with ATA Lib.
void L0_AhciDataXferSendRespInfo(PCB_MGR pSlot)
{
    return;
}

//dummy function for compatibility with ATA Lib.
void L0_AhciDataXferUpdateStageInfo(PCB_MGR pSlot)
{
    return;
}

//dummy function for compatibility with ATA Lib.
void L0_AhciSendSimpleResp(PCB_MGR pSlot)
{
    return;
}

//host need to verify LBA&Count filed of D2H FIS for some ATA command like CheckPowerMode
//this routine is designed to convert D2H FIS format to SDC register interface when response
//completion status for those Special status
void L0_SataRespSpecialRFIS(RD2HFIS *pRD2HFis)
{
    LOCK_SHADOW_REG();//protect on
    while (FALSE == HAL_SataIsFISXferAvailable());

    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;

    //LBA[23:0]
    rSDC_LBALOW = (pRD2HFis->LBALo >> 0) & 0xFF;
    rSDC_LBAMID = (pRD2HFis->LBALo >> 8) & 0xFF;
    rSDC_LBAHIGH = (pRD2HFis->LBALo >> 16) & 0xFF;

    //LBA[47:24]
    rSDC_EXP_LBALOW = (pRD2HFis->LBAHi >> 0) & 0xFF;
    rSDC_EXP_LBAMID = (pRD2HFis->LBAHi >> 8) & 0xFF;
    rSDC_EXP_LBAHIGH = (pRD2HFis->LBAHi >> 16) & 0xFF;

    //Count[15:0]
    rSDC_SECCNT = (pRD2HFis->Count >> 0) & 0xFF;
    rSDC_EXP_SECCNT = (pRD2HFis->Count >> 8) & 0xFF;

    HAL_SataSendRegD2HFIS();
    
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    UNLOCK_SHADOW_REG();//protect off

    return;
}

/*------------------------------------------------------------------------------
Name: L0_SataAbortDataXferDoneDMACmd
Description:
If a command has been decoded as a DMA command and SendSDBFISReady bit has been
cleared before data transfer, after all data transfered, we can use this funtion
to send D2H FIS with abort status.
Input Param:
none
Output Param:
none
Return Value:
void
Usage:
Abort a DMA command after all data transfered
History:
20151217    Kristin     create
------------------------------------------------------------------------------*/
void L0_SataAbortDataXferDoneDMACmd(void)
{
    while (FALSE == HAL_SataIsFISXferAvailable())
    {
        ;
    }

    LOCK_SHADOW_REG();//protect on

    rSDC_COMMAND_STATUS = 0x51;//BSY=0,DRQ=0,ERR=1
    rSDC_FEATURE_ERROR = 4;//ABRT=1
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;//I=1

    /* set SendSDBFISReay,so HW can send D2H FIS */
    HAL_SetSendSDBFISReady(0);

    /* wait D2H FIS sent */
    while (FALSE == HAL_SataIsSdcIdle())
    {
        ;
    }
    rSDC_FEATURE_ERROR = 0;
    rSDC_COMMAND_STATUS = 0x50;//BSY=0,DRQ=0,ERR=0

    UNLOCK_SHADOW_REG();//protect off    

    return;
}

BOOL L0_SataIsRejectCmd(PCB_MGR pSlot)
{
    /* 1. check if disk is locked due to error handling */
    if (TRUE == L0_IsDiskLocked())
    {
        if ((ATA_CMD_READ_LOG_EXT == pSlot->CFis->Command) &&
            (GPL_LOGADDR_NCQERR == rSDC_LBALOW))
        {
            /* Read Log Ext - 10h */
        }
        else
        {
            L0_SataMarkCmdReject(SATA_REJECT_DISK_LOCK, pSlot->SlotNum, pSlot->CFis->Command);
            return TRUE;
        }
    }

    /* 2. Check security status, if disk is locked, some command need to reject */
    if (TRUE == L0_ATACmdCheckSecurityStatus(pSlot))
    {
        L0_SataMarkCmdReject(SATA_REJECT_SECURITY_LOCK, pSlot->SlotNum, pSlot->CFis->Command);
        return TRUE;
    }

    /* 3. Check DMA raw data command CFIS, if invalid, have to reject */
    switch (pSlot->CFis->Command)
    {
    case ATA_CMD_DATA_SET_MANAGEMENT:
        if (FALSE == L0_ATAIsDatasetMgmtCmdValid(pSlot->CFis))
        {
            L0_SataMarkCmdReject(SATA_REJECT_RAW_DATA_PARAM, pSlot->SlotNum, pSlot->CFis->Command);
            return TRUE;
        }
        break;
    case ATA_CMD_DOWNLOAD_MICROCODE_DMA:
        if (FALSE == L0_ATAIsMicroCodeCmdValid(pSlot->CFis))
        {
            L0_SataMarkCmdReject(SATA_REJECT_RAW_DATA_PARAM, pSlot->SlotNum, pSlot->CFis->Command);
            return TRUE;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL L0_SataIsCmdMalformed(PCB_MGR pSlot)
{
    /* check for malformed NCQ command */
    if (SATA_PROT_FPDMA == pSlot->SATAProtocol)
    {       
        /* check Tag overridden */
        if (rSDC_NCQOutstd & (1 << pSlot->SlotNum))
        {
            HAL_SDCHoldDmaAndCmd();
            HAL_HoldBuffMap();

            L0_SataMarkCmdError(SATAERR_TYPE_TAG, pSlot->SlotNum, pSlot->CurrentLBA, pSlot->HCMDLenSec);

            return TRUE;
        }

        /* check NCQ LBA overflow */
        if ((HCMD_TYPE_DIRECT_MEDIA_ACCESS == pSlot->CmdType) &&
            ((pSlot->CurrentLBA + pSlot->HCMDLenSec - 1) > g_ulUsedLBAMax))
        {
#ifdef SIM
            DBG_Printf("Receive NCQ Cmd 0x%x out of boundary, abort it\n", pSlot->CFis->Command);
#endif
            HAL_SDCHoldDmaAndCmd();
            HAL_HoldBuffMap();

            L0_SataMarkCmdError(SATAERR_TYPE_NCQ_LBA, pSlot->SlotNum, pSlot->CurrentLBA, pSlot->HCMDLenSec);

            return TRUE;
        }
    } //if (SATA_PROT_FPDMA == pSlot->SATAProtocol)
    else
    {
        /* check Intermix of Legacy and NCQ commands */
        if (0 != rSDC_NCQOutstd)
        {
            HAL_SDCHoldDmaAndCmd();
            HAL_HoldBuffMap();

            L0_SataMarkCmdError(SATAERR_TYPE_INTERMIX, pSlot->SlotNum, pSlot->CurrentLBA, pSlot->HCMDLenSec);

            return TRUE;
        }

        /* check DMA/PIO LBA overflow */
        if ((HCMD_TYPE_DIRECT_MEDIA_ACCESS == pSlot->CmdType) &&
            ((pSlot->CurrentLBA + pSlot->HCMDLenSec - 1) > g_ulUsedLBAMax))
        {
            /* just reject it */
            L0_SataMarkCmdReject(SATA_REJECT_LEGACY_LBA, pSlot->SlotNum, pSlot->CFis->Command);
#ifdef SIM
            DBG_Printf("Receive DMA Cmd 0x%x out of boundary, abort it\n", pSlot->CFis->Command);
#endif
            return TRUE;
        }
    }

    return FALSE;
}

/*------------------------------------------------------------------------------
Name: L0_SataParseIncomingCmd
Description: 
    1. IF disk locked, reject any command except READ_LOG_EXT(log page 0x0/0x10)
       ELSE
    2. (ATA Lib)convert CFIS to local format;
    3. for NCQ command, check if Tag is overridden and if sector length is overflowed;
    4. for non-NCQ command, check if there is NCQ command outstanding
Input Param:
    PCB_MGR pSlot: pointer to CB_MGR
Output Param:
    PCB_MGR pSlot: pointer to CB_MGR
Return Value:
    BOOL: TRUE = command can be added to local HCMD table for further process
          FALSE = we must reject the command
Usage:
    In SDC command receiving ISR, after loading CFIS from SDC reigster,
    FW call this function for further checking, and if this routine return FALSE
    we must mark the command as PIO command before quit from interrupt, and abort it
    after quiting ISR.
History:
    20150129    Gavin   create pseudo-code
    20150131    Gavin   implement for normal command
------------------------------------------------------------------------------*/
BOOL L0_SataParseIncomingCmd(PCB_MGR pSlot)
{
    if (TRUE == L0_SataIsRejectCmd(pSlot))
    {
        return FALSE;
    }

    /* call ATA Lib to parse CFIS */
    //TODO: ? backup param in CB_MGR for NCQ Tag overridden error handling ?
    L0_ATASetCmdParam(pSlot);

    if ((g_VCM_Param.CurStage == DATA_STAGE) && (g_VCM_Param.Signature != 0x3533))
    {
        DBG_Printf("Check VCM Signature fail: Signature=0x%x\n", g_VCM_Param.Signature);
        L0_SataMarkCmdReject(SATA_REJECT_NOT_SUPPORT, pSlot->SlotNum, pSlot->CFis->Command);
        return FALSE;
    }

    if (TRUE == L0_SataIsCmdMalformed(pSlot))
    {
        return FALSE;
    }

    switch (pSlot->SATAProtocol)
    {
        case SATA_PROT_FPDMA:
            HAL_SataFeedbacktoHW(pSlot->SlotNum, SDC_PROTOCOL_NCQ);
            break;

        case SATA_PROT_DMA:
            if ((ATA_CMD_DATA_SET_MANAGEMENT == pSlot->CFis->Command) ||
                (ATA_CMD_VENDER_DEFINE == pSlot->CFis->Command) ||
                (ATA_CMD_DOWNLOAD_MICROCODE_DMA == pSlot->CFis->Command))
            {
                //clear SDB_FIS_READY
                HAL_ClearSendSDBFISReady(pSlot->SlotNum);
            }
            HAL_SataFeedbacktoHW(pSlot->SlotNum, SDC_PROTOCOL_DMA);
            break;

        default:
            HAL_SataFeedbacktoHW(pSlot->SlotNum, SDC_PROTOCOL_PIO);
            break;
    }

    return TRUE;
}

/*------------------------------------------------------------------------------
Name: L0_SataCompleteCmd
Description: 
    if needed, do SATA special handling before clean up the command
Input Param:
    PCB_MGR pSlot: pointer to CB_MGR
Output Param:
    none
Return Value:
    BOOL: TRUE = command completion done
          FALSE = command completion done
Usage:
    After ATA Lib finish process a command, call this function to clean up it.
History:
    20150130    Gavin   create pseudo-code
    20150131    Gavin   implement for normal command
------------------------------------------------------------------------------*/
BOOL L0_SataCompleteCmd(PCB_MGR pSlot)
{
    U32 ulATAState;
    U8 ucSATAProtocol;
    U8 ucCmdCode;

    if (FALSE == HAL_SataIsFISXferAvailable())
    {
        return FALSE;
    }
    
    ulATAState = pSlot->ATAState;
    ucSATAProtocol = pSlot->SATAProtocol;
    ucCmdCode = pSlot->CFis->Command;

    if (ATACMD_ABORTED == ulATAState)
    {
        if ((ATA_CMD_DATA_SET_MANAGEMENT == ucCmdCode) ||
            (ATA_CMD_VENDER_DEFINE == ucCmdCode) ||
            (ATA_CMD_DOWNLOAD_MICROCODE_DMA == ucCmdCode))
        {
            L0_SataAbortDataXferDoneDMACmd();
        }
        else
        { 
            // mask command interrupt to ensure hardware DIPM correctness
            HAL_SataEnableCmdRcvInt(FALSE);

            HAL_SataSendAbortStatus();
            HAL_SataClearPIOCmdPending();

            // Unmask command interrupt
            HAL_SataEnableCmdRcvInt(TRUE);
        }
    }

    else if (ATACMD_COMPLETED == ulATAState)
    {
        switch (ucSATAProtocol)
        {
            /*
            for DMA/NCQ meidia access, the completion FIS is sent by SDC
            for PIO-In/Out, the completion FIS is sent in PIO state machine processing
            */
            case SATA_PROT_FPDMA:
            break;

            case SATA_PROT_PIO:
                // if the cmd is pio write cmd, we should send success D2H FIS
                // and clear PioCmdPending flag here
                if(FALSE == gPIOInfoBlock.bCmdRead)
                {
                    // wait SDC finish all data FIS tranferring
                    if (SATA_PIO_NOCMD != gPIOInfoBlock.ucCurrPIOState)
                    {
                        return FALSE;
                    }

                    // mask command interrupt to ensure hardware DIPM correctness
                    HAL_SataEnableCmdRcvInt(FALSE);

                    HAL_SataSendSuccessStatus();
                    HAL_SataClearPIOCmdPending();

                    // Unmask command interrupt
                    HAL_SataEnableCmdRcvInt(TRUE);
                }

            break;

            case SATA_PROT_DMA:
                if ((ATA_CMD_DATA_SET_MANAGEMENT == ucCmdCode) ||
                    (ATA_CMD_VENDER_DEFINE == ucCmdCode) ||
                    (ATA_CMD_DOWNLOAD_MICROCODE_DMA == ucCmdCode))
                {
                    // TRIM/VIA Special handling to SDC: set SDB_FIS_READY
                    HAL_SetSendSDBFISReady(pSlot->SlotNum);
                }

            break;

            case SATA_PROT_DIAG:
                // mask command interrupt to ensure hardware DIPM correctness
                HAL_SataEnableCmdRcvInt(FALSE);

                HAL_SataSignatureSendGoodStatus();
                HAL_SataClearPIOCmdPending();

                // Unmask command interrupt
                HAL_SataEnableCmdRcvInt(TRUE);
            break;

            case SATA_PROT_NONDATA:
                // mask command interrupt to ensure hardware DIPM correctness
                HAL_SataEnableCmdRcvInt(FALSE);

                if (HCMD_TYPE_NEED_SPECIAL_STATUS == pSlot->CmdType)
                {
                    L0_SataRespSpecialRFIS(&g_tRFisOfSataMode);
                }
                else
                {
                    HAL_SataSendSuccessStatus();
                }

                HAL_SataClearPIOCmdPending();

                // Unmask command interrupt
                HAL_SataEnableCmdRcvInt(TRUE);
            break;

            default:
                DBG_Printf("erroneous ata command protocol\n");
                DBG_Getch();//debug check
            break;
        }
    }

    else
    {
        DBG_Printf("erroneous ata command state\n");
        DBG_Getch();
    }
    
    return TRUE;
}

/* this function is called by ATA Lib when split Media-Access command to sub-system */
U32 L0_ATAMediaAccSplitCmd(PCB_MGR pSlot)
{
    static U32 ulHcmdStartLba;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    while(pSlot->StageRemainingBytes > 0)
    {
        // get the subsystem id for the current lba
        ulSubSysIdx = L0M_GET_SUBSYSID_FROM_LBA(pSlot->CurrentLBA, g_ulSubsysNumBits);

        // before proceeding, we have to ensure that the
        // subcommand queue is not full and the host command
        // passes the split check
        if(TRUE == L0_IsSCQFull(ulSubSysIdx) || L0_HcmdSplitCheck(pSlot) == FALSE)
        {
            break;
        }

        // get a free subcommand node from the queue
        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);

        // set the scmd type
        pCurrSCmd->ucSCmdType = (U8)SCMD_DIRECT_MEDIA_ACCESS;

        // set the host command tag number
        pCurrSCmd->ucSlotNum = pSlot->SlotNum;

        // set the subsystem lba for the current subcommand
        pCurrSCmd->tMA.ulSubSysLBA = L0M_GET_SUBSYSLBA_FROM_LBA(pSlot->CurrentLBA, g_ulSubsysNumBits);

        // set the sector length of the current subcommand
        pCurrSCmd->tMA.ucSecLen = (SEC_PER_BUF - L0M_GET_OFFSET_IN_LCT_FROM_LBA(pSlot->CurrentLBA));

        if(((U32)pCurrSCmd->tMA.ucSecLen << SEC_SIZE_BITS) > pSlot->StageRemainingBytes)
        {
            pCurrSCmd->tMA.ucSecLen = (U8)(pSlot->StageRemainingBytes >> SEC_SIZE_BITS);
        }

        // set the access type
        pCurrSCmd->tMA.ucOpType = (FALSE == pSlot->IsWriteDir) ?  (U8)DM_WRITE : (U8)DM_READ;

        // set the ncq flag
        pCurrSCmd->tMA.ucIsNCQ = ((U8)SATA_PROT_FPDMA == pSlot->SATAProtocol) ? TRUE : FALSE;

        // set the pio flag
        pCurrSCmd->tMA.ucIsPIO = (pSlot->SATAProtocol == SATA_PROT_PIO) ? TRUE : FALSE;

        // set the sequential flag based on the length of the scmd
        pCurrSCmd->tMA.ucIsSeq = (pSlot->HCMDLenSec >= SEC_PER_BUF) ? TRUE : FALSE;

        // set ulHcmdStartLba, which is a static variable,
        // if we're processing the first scmd of the host
        // command
        if(0 == pSlot->CurrentSubCmdIndex)
        {
            ulHcmdStartLba = pSlot->CurrentLBA;
        }

        // please note that both the host command start lba
        // and host command length is very important and have
        // to be set correctly, if set incorrectly, it will
        // cause failure in ASIC environment

        // set the start lba of the host command 
        pCurrSCmd->tMA.ulHostStartLBA = ulHcmdStartLba;

        // set the sector length of the host command
        pCurrSCmd->tMA.ulHCmdSecLen = pSlot->HCMDLenSec;

        // update the remaining bytes and the current lba
        pSlot->StageRemainingBytes -= ((U32)pCurrSCmd->tMA.ucSecLen << SEC_SIZE_BITS);
        pSlot->CurrentLBA += (U32)pCurrSCmd->tMA.ucSecLen;

        // set the first and last flag
        pCurrSCmd->tMA.ucFirst = (pSlot->CurrentSubCmdIndex == 0) ? TRUE : FALSE;
        pCurrSCmd->tMA.ucLast = (pSlot->StageRemainingBytes == 0) ? TRUE : FALSE;

        // set the scmd index for the current subcommand
        pCurrSCmd->tMA.usSCmdIndex = pSlot->CurrentSubCmdIndex;

        // update the scmd index
        pSlot->CurrentSubCmdIndex++;

        // update the multi-core flag, which is used to indicate
        // if subcommand queues contain subcommands originated
        // from a multi-core host command
        if(pCurrSCmd->tMA.ucLast == TRUE)
        {
            if(pCurrSCmd->tMA.ucFirst == TRUE)
            {
                g_bMultiCoreSCMDFlag = FALSE;
            }
            else
            {
                g_bMultiCoreSCMDFlag = TRUE;
            }
        }

        // push the subcommand node to the queue
        L0_PushSCmdNode(ulSubSysIdx);
    }

    // check if we're done splitting a host command
    if(0 < pSlot->StageRemainingBytes)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL L0_SATAFwCrcChk(U32 ulFwBaseAddr)
{
    U32 ulCrcCalc = 0;
    U32 *pFwDW;
    U32 ulFwDWCnt;

    pFwDW = (U32*)ulFwBaseAddr;
    ulFwDWCnt = g_tFwUpdate.ulFwSize >> 2;    //FW_IMAGE_MEM_SIZE is in byte, Calc in DW

    do 
    {
        ulCrcCalc ^= *pFwDW;
        pFwDW++;
    } while (--ulFwDWCnt);
    
    return (0 == ulCrcCalc) ? TRUE : FALSE;
}

MCU0_DRAM_TEXT BOOL L0_SATAFwCommitSaveFw(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;
    pSaveFW pSaveFWFunc;
    pSaveFWFunc = (pSaveFW)HAL_GetFTableFuncAddr(SAVE_FIRWARE);


    /* Because of NFC programming flash in page,so the source address must be page aligned(16K), 
           one big buffer should be used for backup image
        */
    COM_MemCpy( (U32*)DRAM_DATA_BUFF_MCU1_BASE,
                (U32*)g_tFwUpdate.ulFwBaseAddr,
                g_tFwUpdate.ulFwSize/sizeof(U32));
#ifndef SIM
    ulRst = pSaveFWFunc(ulFwSlot-1,DRAM_DATA_BUFF_MCU1_BASE);
#else    
    ulRst = TRUE;
#endif // SIM
    if (TRUE == ulRst)
    {
        g_tFwUpdate.ulDldSize = 0; //Set to 0 to enable a new download.
        
        DBG_Printf("SaveFW Done.\r\n");
        
        return TRUE;
    }
    else
    {
        return FAIL;
    }

    return FAIL;    //Return FAIL in default
}

MCU0_DRAM_TEXT BOOL L0_SATAFwCommitActFw(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;
    pActiveFW pActiveFWFunc;
  
    pActiveFWFunc = (pActiveFW)HAL_GetFTableFuncAddr(ACTIVE_FIRWARE);
#ifndef SIM
    HAL_HaltSubSystemMCU(TRUE);
    ulRst = pActiveFWFunc(ulFwSlot-1);
    HAL_HaltSubSystemMCU(FALSE);
#else
    ulRst = TRUE;
#endif // !SIM

    if (TRUE == ulRst)
    {
        DBG_Printf("ActiveFw Done\r\n");
        return TRUE;
    }
    else
    {
        return FAIL;
    }

    return FAIL;    //Return FAIL in default
}

MCU0_DRAM_TEXT LOCAL BOOL L0_SATASaveFwImage(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;

    DBG_Printf("SATA FW UPGRADE : Save FW Start. \n");
    ulRst = L0_SATAFwCommitSaveFw(ulFwSlot);
    DBG_Printf("SATA FW UPGRADE : Save FW End. \n");

    return ulRst;
}

MCU0_DRAM_TEXT BOOL L0_SATAFwCommit()
{
    U32 ulFinished = FALSE;
    BOOL bStsFlag = FALSE;

    /* firmware CRC check error */
    if (FALSE == L0_FwUpgradeCheckImgValid())
    {
        DBG_Printf("FwCrcChk Fail.\r\n");
    }
    else
    {
        L0_ForceAllSubSysIdle();

        /*Save FW*/
        L0_SubSystemOnlineShutdown(FALSE);
        bStsFlag = L0_SATASaveFwImage(1);
        if (TRUE == bStsFlag)
        {
            DBG_Printf("Save fw succ.\n");
            /*Active FW*/
            //bStsFlag = L0_SATAFwCommitActFw(1);
        }
        L0_SubSystemOnlineReboot();
    }

    if (TRUE == bStsFlag)
    {
        DBG_Printf("Acitive FW succ.\n");
        ulFinished = TRUE;
    }

    return ulFinished;
}
