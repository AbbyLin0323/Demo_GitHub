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
* File Name    : MixVector_Flash.c
* Discription  : common interface for read/write/erase command to flash
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/

#include "BaseDef.h"
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "MixVector_Flash.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_HSG.h"

GLOBAL BOOL l_bNfcErr;
GLOBAL U8 l_ucErrPU;
GLOBAL U8 *g_pSSU1;
GLOBAL U32 g_ulSSU1Addr;
GLOBAL HSG_ENTRY g_tHsg = {0};
GLOBAL FLASH_REQ_HSG_MONITOR g_DiskCHSGMonitor[MAX_SLOT_NUM];

GLOBAL FLASH_REQ_MONITOR g_aFlashReqMonitor[PU_NUM];

//GLOBAL MCU12_VAR_ATTR SYSTEM_GLOBAL_INFO *g_pSubSystemGlobalInfo;// only for compiling in MixVector
/*------------------------------------------------------------------------------
Name: IsPuError
Description: 
set SSU to busy before trig command to NFC. SSU will be updated by NFC if no error happen
Input Param:
void
Output Param:
BOOL    TRUE  -- an pu error occurred 
FALSE -- no error occurred
Return Value:
void
Usage:
Firstly , the pu accelarating bitmap should be updated. 
Then firmware checks the error bitmap ,if there is some exception , the HalNfcGetErrPu() 

will return the error pu number. 
Finally , a local error bit (l_bNfcErr) be set ,the error pu number be output and return TRUE if an error occurred 
History:
20131119    Victor   created
------------------------------------------------------------------------------*/

BOOL IsPuError(void)
{
    U8  ucPu = INVALID_2F;
    U32 ErrType;
    //HalNfcUpdLogicPuSts(void)();
    //ucPu = HalNfcGetErrPu();
    if(INVALID_2F != ucPu)
    {
        l_bNfcErr = TRUE;
        l_ucErrPU = ucPu;
        ErrType = HAL_NfcGetErrCode(ucPu);
        DBG_Printf("NFC err,Pu:%d,Type:%d\n",ucPu,ErrType);
        return TRUE;
    }
    return FALSE;
}
/*------------------------------------------------------------------------------
Name: SetSsuBusy
Description: 
set SSU to busy before trig command to NFC. SSU will be updated by NFC if no error happen
Input Param:
U8 ucCmdSlot: comamnd slot number
U8 ucIndex: ssu index of this slot. each slot binds to 8 SSUs
Output Param:
none
Return Value:
void
Usage:
before trig command to NFC, set SSU to busy when needed.
FW wait SSU cleared by NFC before finish command
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
LOCAL void SetSsuBusy(U8 ucCmdSlot, U8 ucIndex)
{
    g_pSSU1[ucCmdSlot * SSU1_NUM_PER_SLOT + ucIndex] = SSU_BUSY;
}

/*------------------------------------------------------------------------------
Name: ClearSsuBusy
Description: 
FW clear SSU busy to 0
Input Param:
U8 ucCmdSlot: comamnd slot number
U8 ucIndex: ssu index of this slot. each slot binds to 8 SSUs
Output Param:
none
Return Value:
void
Usage:
in flash error handling, FW call this function to clear SSU busy
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
LOCAL void ClearSsuBusy(U8 ucCmdSlot, U8 ucIndex)
{
    g_pSSU1[ucCmdSlot * SSU1_NUM_PER_SLOT + ucIndex] = SSU_FREE;
}

/*------------------------------------------------------------------------------
Name: CheckSsuBusy
Description: 
for erase command, check SSU was updated by NFC or not
Input Param:
U8 ucCmdSlot: command slot number
Output Param:
none
Return Value:
BOOL: TRUE = SSU was updated; FALSE = SSU was not updated by NFC
Usage:
for erase command to diskC, FW call this function to check all SSU updated by NFC before
build WBQ to complete host command.
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL CheckSsuBusy(U8 ucCmdSlot)
{
    U8 ucIndex;

    for (ucIndex = 0; ucIndex < SSU1_NUM_PER_SLOT; ucIndex++)
    {
        if (SSU_FREE != g_pSSU1[ucCmdSlot * SSU1_NUM_PER_SLOT + ucIndex])
        {
            return FALSE;
        }
    }

    return TRUE;
}
/*------------------------------------------------------------------------------
Name: DiskC_SetMonitor
Description: 
Back up last issued flash request and write pointer of the target PU 
Input Param:
U8 Pu  -- The target PU
U8 Wp  -- The present write pointer of the target PU
FLASH_REQ_FROM_HOST *pHostReq              -- The pointer to the last issued flash request 
Output Param:
none
Return Value:
void
Usage:
Back up the last issud flash request for error handling , if an error occured in target pu ,
the main state machine will figure the error out and process the error handling ,and in the 
error handling process ,the function will complish the pending request or not ,depending the 
back-up write pointer.Then may reset the PU for complishing the error handling . 
History:
20131119    Victor   created
------------------------------------------------------------------------------*/
void DiskC_SetMonitor(U8 Pu ,U8 Wp ,FLASH_REQ_FROM_HOST *pHostReq)
{
    g_aFlashReqMonitor[Pu].LastTrigLevel = Wp;
    COM_MemCpy((U32*)&g_aFlashReqMonitor[Pu].FlashReq[Wp],
        (U32*)pHostReq,
        sizeof(FLASH_REQ_FROM_HOST)/sizeof(U32));

}
/*------------------------------------------------------------------------------
Name:NFC_BuildHostWriteReq
Description: 
Call it when a flash write request be issued 
Input Param:
FLASH_REQ_FROM_HOST *pHostReq   -- The pointer to the issued flash request 
Output Param:
none
Return Value:
BOOL :  TRUE    --  Write flash whole page successfully
FALSE   --  Failed
Usage:
For bulid a host to flash request 
History:
20131119    Victor   created
------------------------------------------------------------------------------*/

BOOL NFC_BuildHostWriteReq(FLASH_REQ_FROM_HOST *pHostReq)
{
    U8 ucWp;
    U16 usHsgId;
    HSG_ENTRY *pHsgEntry;
    FLASH_ADDR tPhyAddr = {0};
    HMEM_DPTR tHostMemDptr = {0};

    tPhyAddr.ucPU = pHostReq->PU;
    tPhyAddr.usBlock = pHostReq->Block;
    tPhyAddr.usPage = pHostReq->Page;
    if ((TRUE == HAL_NfcGetFull(tPhyAddr.ucPU)) || (TRUE == HAL_NfcGetErrHold(tPhyAddr.ucPU)))
    {
        return FALSE;
    }

    if (FALSE == HAL_GetHsg(&usHsgId))
    {
        return FALSE;
    }

    pHsgEntry = (HSG_ENTRY *)HAL_GetHsgAddr(usHsgId);
    COM_MemZero((U32*)pHsgEntry,sizeof(HSG_ENTRY)/sizeof(U32));

    pHsgEntry->ulHostAddrHigh = pHostReq->HostAddrHigh;
    pHsgEntry->ulHostAddrLow = pHostReq->HostAddrLow;
    pHsgEntry->bsLength = pHostReq->Seclength << SEC_SIZE_BITS;
    pHsgEntry->ulLBA = pHostReq->StartSec;
    pHsgEntry->bsLast = TRUE;

    HAL_SetHsgSts(usHsgId, HSG_VALID);

    ucWp = HAL_NfcGetWP(tPhyAddr.ucPU);

    tHostMemDptr.bsCmdTag = pHostReq->HID;

    if (NFC_STATUS_SUCCESS != HAL_NfcHostPageWrite(&tPhyAddr, pHostReq->HID, usHsgId, NULL))
    {
        return FALSE;
    }
    else
    {
        DiskC_SetMonitor(tPhyAddr.ucPU,ucWp ,pHostReq);
        return TRUE;
    }
}
/*------------------------------------------------------------------------------
Name:NFC_BuildHostReadReq
Description: 
Call it when a flash read request be issued 
Input Param:
FLASH_REQ_FROM_HOST *pHostReq   -- The pointer to the issued flash request 
Output Param:
none
Return Value:
BOOL :  TRUE    --  Read flash data successfully
FALSE   --  Failed
Usage:
For bulid a flash to host request 
History:
20131119    Victor   created
------------------------------------------------------------------------------*/
BOOL NFC_BuildHostReadReq(FLASH_REQ_FROM_HOST *pHostReq,BOOL bSplit)
{
    U8 ucWp,ucHID;
    U16 usHsgId,usCurHsg,usNextHsgId;
    U32 ulLenth,ulHostReqLen;
    U32 tHostAddrLow;
    HSG_ENTRY *pHsgEntry;
    FLASH_ADDR tPhyAddr = {0};
    HMEM_DPTR tHostMemDptr = {0};
    tPhyAddr.ucPU = pHostReq->PU;
    tPhyAddr.usBlock = pHostReq->Block;
    tPhyAddr.usPage = pHostReq->Page;

    if ((TRUE == HAL_NfcGetFull(tPhyAddr.ucPU)) || (TRUE == HAL_NfcGetErrHold(tPhyAddr.ucPU)))
    {
        return FALSE;
    }

    g_DiskCHSGMonitor[pHostReq->HID].HostAddrHigh = g_aHostCmdManager[pHostReq->HID].SubDiskCCmd.HostAddrHigh;
    g_DiskCHSGMonitor[pHostReq->HID].HostAddrLow = g_aHostCmdManager[pHostReq->HID].SubDiskCCmd.HostAddrLow;

    if(TRUE == bSplit)
    {
        ucHID = pHostReq->HID;
        ulHostReqLen = pHostReq->Seclength << SEC_SIZE_BITS;        
        do
        {
            //prepare hsg chain
            if(INVALID_4F  == g_aHostCmdManager[ucHID].ContinueHSGDiskC)
            {
                if(FALSE == HAL_GetHsg(&usCurHsg))
                {
                    return FALSE;
                }
        
            }
            else
            {
                usCurHsg = g_aHostCmdManager[ucHID].ContinueHSGDiskC;
                g_aHostCmdManager[ucHID].ContinueHSGDiskC = INVALID_4F;
                HAL_TriggerHsg();
            }
        
            pHsgEntry =(HSG_ENTRY *) HAL_GetHsgAddr(usCurHsg);
            COM_MemSet((U32*)pHsgEntry,sizeof(HSG_ENTRY)/sizeof(U32),0);
            pHsgEntry->ulHostAddrHigh = g_DiskCHSGMonitor[ucHID].HostAddrHigh;
            pHsgEntry->ulHostAddrLow = g_DiskCHSGMonitor[ucHID].HostAddrLow;
        
            ulLenth = DiskC_GetHsgLenth(ucHID);

            if((ulLenth > ulHostReqLen) || (0 == ulLenth) )
            {
                ulLenth = ulHostReqLen;
            }
            pHsgEntry->bsLength = ulLenth;
        
            //DBG_Printf("Length :0x%x HostAddrLow:0x%x\n",ulLenth,pHsgEntry->ulHostAddrLow);
            if(FALSE == HAL_GetCurHsg(&usNextHsgId))
            {
                g_aHostCmdManager[ucHID].ContinueHSGDiskC = usCurHsg;
                return FALSE;
            }
            pHsgEntry->bsNextHsgId = usNextHsgId;
        
            if(FALSE == g_aHostCmdManager[ucHID].bDiskCFirstHSGGet)
            {
                usHsgId = usCurHsg;
                g_aHostCmdManager[ucHID].FirstHSGDiskC = usHsgId;
                g_aHostCmdManager[ucHID].bDiskCFirstHSGGet = TRUE;
            }
            else
            {
                usHsgId = g_aHostCmdManager[ucHID].FirstHSGDiskC;
            }
            tHostAddrLow = g_DiskCHSGMonitor[ucHID].HostAddrLow;
            g_DiskCHSGMonitor[ucHID].HostAddrLow += pHsgEntry->bsLength;
            if(g_DiskCHSGMonitor[ucHID].HostAddrLow < tHostAddrLow)
            {
                g_DiskCHSGMonitor[ucHID].HostAddrHigh ++;
            }
        
            tHostAddrLow = g_aHostCmdManager[ucHID].SubDiskCCmd.HostAddrLow;
            g_aHostCmdManager[ucHID].SubDiskCCmd.HostAddrLow += pHsgEntry->bsLength;
            if(g_aHostCmdManager[ucHID].SubDiskCCmd.HostAddrLow < tHostAddrLow)
            {
                g_aHostCmdManager[ucHID].SubDiskCCmd.HostAddrHigh ++;
            }
        
            ulHostReqLen -= pHsgEntry->bsLength;
            if(0 == ulHostReqLen)
            {
                pHsgEntry->bsLast = TRUE;//just suitable for whoel page read write.
            }
        
            HAL_SetHsgSts(usCurHsg, TRUE);
        
        }while(ulHostReqLen);


    }

    else
    {
        if (FALSE == HAL_GetHsg(&usHsgId))
        {
            return FALSE;
        }
        
        pHsgEntry = (HSG_ENTRY *)HAL_GetHsgAddr(usHsgId);
        COM_MemZero((U32*)pHsgEntry,sizeof(HSG_ENTRY)/sizeof(U32));
        
        pHsgEntry->ulHostAddrHigh = pHostReq->HostAddrHigh;
        pHsgEntry->ulHostAddrLow = pHostReq->HostAddrLow;
        pHsgEntry->bsLength = pHostReq->Seclength << SEC_SIZE_BITS;
        pHsgEntry->ulLBA = pHostReq->StartSec;
        pHsgEntry->bsLast = TRUE;
        
        HAL_SetHsgSts(usHsgId, HSG_VALID);
        
        tHostMemDptr.bsCmdTag = pHostReq->HID;

    }
    
    ucWp = HAL_NfcGetWP(tPhyAddr.ucPU);
    

    if (NFC_STATUS_SUCCESS != HAL_NfcHostPageRead(&tPhyAddr,pHostReq->HID,usHsgId,pHostReq->StartSec,pHostReq->Seclength,NULL))
    {
        return FALSE;
    }
    else
    {
        DiskC_SetMonitor(tPhyAddr.ucPU,ucWp ,pHostReq);
        g_aHostCmdManager[pHostReq->HID].bDiskCFirstHSGGet = FALSE;
        return TRUE;
    }
}
/*------------------------------------------------------------------------------
Name:NFC_BuildHostReadReq
Description: 
Call it when a flash read request be issued 
Input Param:
FLASH_REQ_FROM_HOST *pHostReq   -- The pointer to the issued flash request 
Output Param:
none
Return Value:
BOOL :  TRUE    --  Erase flash whole block successfully
FALSE   --  Failed
Usage:
For bulid a flash block erase request 
History:
20131119    Victor   created
------------------------------------------------------------------------------*/
void NFC_SetSSU1Addr(FLASH_REQ_FROM_HOST *pHostReq)
{
    g_ulSSU1Addr = (U32)&g_pSSU1[(pHostReq->HID * SSU1_NUM_PER_SLOT + pHostReq->CmdSubId)];
}

U32 NFC_GetSSU1Addr()
{
     return g_ulSSU1Addr;
}

BOOL NFC_BuildHostEraseReq(FLASH_REQ_FROM_HOST *pHostReq)
{
    FLASH_ADDR tPhyAddr = {0};
    tPhyAddr.ucPU = pHostReq->PU;
    tPhyAddr.usBlock = pHostReq->Block;

    NFC_SetSSU1Addr(pHostReq);

    if (NFC_STATUS_SUCCESS != HAL_NfcFullBlockErase(&tPhyAddr))    
    {
        return FALSE;
    }
    return TRUE;
}

/****************************************************************************
Name        :DiskC_GetHsgLenth
Input       :ptHostReq,SplitEnable,ulRemSecLen,ucCmdSlot
Output      :TRUE : HSG end; FALSE : Still have HSG
Author      :
Date        :
Description :fit HSG lenth to tHostReq.
Others      :
Modify      :
****************************************************************************/
U32 DiskC_GetHsgLenth(U32 ucCmdSlot)
{
    U32 ulHSGPtr;
    U32 bRet;

    bRet = 0;

    ulHSGPtr = g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_C];

    bRet = g_pHCmdTable[ucCmdSlot].HsgLen[ulHSGPtr] * DISK_C_UNIT;
    ulHSGPtr ++;

    g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_C] = ulHSGPtr;

    return bRet;

}

/*------------------------------------------------------------------------------
Name: ProcessNfcError
Description:
error handling entry for flash read/write/erase error
Input Param:
U8 ucPU: logic pu number
Output Param:
none
Return Value:
void
Usage:
if error flag was set, call this function to do error handling. To simplify FW, this function will
just set error in comamnd table which will be reported to host; look up flash monitor to re-build
pending command when error happen.
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
void ProcessNfcError(U8 ucPU)
{
    U8 ucErrLevel, ucPendingLevel, ucTag;
    FLASH_REQ_MONITOR *pFlashMonitor;
    FLASH_REQ_FROM_HOST *pFlashReqErr, *pFlashReqPending;
    HOST_CMD *pHostCmd;
    HOST_CMD_MANAGER *pCmdManager;

    pFlashMonitor = &g_aFlashReqMonitor[ucPU];

    //find error flash request
    ucErrLevel = HAL_NfcGetRP(ucPU);
    pFlashReqErr = &pFlashMonitor->FlashReq[ucErrLevel];

    //find host command according to error PU
    ucTag = pFlashReqErr->HID;
    pHostCmd = &g_pHCmdTable[ucTag];
    pCmdManager = &g_aHostCmdManager[ucTag];

    /* for error command, just set error flag in host command, 
    which will be reported to host */
    pHostCmd->ErrFlag |= (1 << (pFlashReqErr->CmdSubId));

    // reset HW queue to level 0
    HAL_NfcResetCmdQue(ucPU);

    if (TRUE == pCmdManager->Ssu1En)
    {
        ClearSsuBusy(ucTag, pFlashReqErr->CmdSubId);
    }

    // check if there is one command pending in NFC HW queue when eror happen
    if (ucErrLevel != pFlashMonitor->LastTrigLevel)
    {
        ucPendingLevel = (0 == ucErrLevel) ? 1 : 0;
        pFlashReqPending = &pFlashMonitor->FlashReq[ucPendingLevel];

        switch (pFlashReqPending->Type)
        {
        case HOST_REQ_READ:
            //call Flash driver interface for read
            NFC_BuildHostReadReq(pFlashReqPending,FALSE);
            break;

        case HOST_REQ_WRITE:
            //call Flash driver interface for write
            NFC_BuildHostWriteReq(pFlashReqPending);
            break;

        case HOST_REQ_ERASE:
            //call Flash driver interface for erase
            NFC_BuildHostEraseReq(pFlashReqPending);
            break;

        default:
            DBG_Getch();
            break;
        }
    }
}

/*------------------------------------------------------------------------------
Name: ProcessDiskC
Description: 
build flash command to NFC according to argument in host command. this function must update
flash monitor for error handling
if error found, set error flag
Input Param:
U8 ucCmdSlot: command slot index
Output Param:
none
Return Value:
BOOL: TRUE = build flash command success; FALSE = build flash command fail(error or full);
Usage:
if there is any request to diskC, call this function 
History:
20131105    Gavin   created
------------------------------------------------------------------------------*/
U32 ulSendLen[MAX_SLOT_NUM] = {0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
U32 ulRemLen[MAX_SLOT_NUM] = {0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
U32 ulIndex[MAX_SLOT_NUM] = {0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
BOOL ProcessDiskC(U8 ucCmdSlot)
{
    BOOL bRet = FALSE;
    BOOL bSplit;
#if 0        
    static U32 ulIndex[MAX_SLOT_NUM] = {0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

    static U32 ulRemLen[MAX_SLOT_NUM] = {0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
#endif
    
    FLASH_REQ_FROM_HOST tHostReq = {0};

    if (FALSE == g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_C].DiskEn)
    {
        return TRUE;
    }

    if (TRUE == IsPuError())
    {
        return FALSE;
    }

    if (FALSE == g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_C].DiskEn)
    {
        return TRUE;
    }  

    tHostReq.HID = ucCmdSlot;
//    g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.StartUnit = (g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.StartUnit)%SEC_PER_PG;
    g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.StartUnit = (g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.StartUnit)%SEC_PER_BUF;
    tHostReq.StartSec = g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.StartUnit;
    tHostReq.HostAddrHigh = g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.HostAddrHigh;
    tHostReq.HostAddrLow = (g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.HostAddrLow+ulSendLen[ucCmdSlot]*SEC_SIZE);
    tHostReq.Type = g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.ReqType;
    bSplit = g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.SplitEnable;

    if(0 == ulRemLen[ucCmdSlot])
    {
        ulIndex[ucCmdSlot] = 0;
        ulRemLen[ucCmdSlot] = g_aHostCmdManager[ucCmdSlot].SubDiskCCmd.UnitLength;
        //DBG_Printf("slot:%d ulRemLen[ucCmdSlot]:%d\n",ucCmdSlot,ulRemLen[ucCmdSlot]);
    }
    if (HOST_REQ_ERASE == tHostReq.Type)
    {

        while (0 != ulRemLen[ucCmdSlot])
        {
            SetSsuBusy(ucCmdSlot,ulIndex[ucCmdSlot]);
            tHostReq.CmdSubId = ulIndex[ucCmdSlot];
            tHostReq.PU = g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].PU;
            tHostReq.Block = g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].Block;
            tHostReq.Page= g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].Page;
            //DBG_Printf("ers pu:%d blk:%d\n",tHostReq.PU,tHostReq.Block);
            tHostReq.Seclength = 1;
            if (TRUE == HAL_NfcGetFull(tHostReq.PU))
            {        
                return FALSE;
            }      
            if (FALSE == NFC_BuildHostEraseReq(&tHostReq))
            {
                break;
            }

            //g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_C] += tHostReq.Seclength;
            ulRemLen[ucCmdSlot]--;
            ulIndex[ucCmdSlot]++;    
        }
    }
    else 
    {
        while (0 != ulRemLen[ucCmdSlot])
        {
            tHostReq.CmdSubId = ulIndex[ucCmdSlot];
            tHostReq.PU = g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].PU;
            tHostReq.Block = g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].Block;
            tHostReq.Page= g_pHCmdTable[ucCmdSlot].FlashAddrGroup[ulIndex[ucCmdSlot]].Page; 
            //DBG_Printf("Type:%d PU : %d,Block : %d,Page : %d\n",tHostReq.Type,tHostReq.PU,tHostReq.Block,tHostReq.Page);                    
            if (TRUE == HAL_NfcGetFull(tHostReq.PU))
            {        
                DBG_Printf("nfc full REQ:%d PU : %d,Block : %d,Page : %d\n",tHostReq.Type,tHostReq.PU,tHostReq.Block,tHostReq.Page);                
                return FALSE;
            }            
            if(tHostReq.Type==HOST_REQ_READ)
            {
                if(ulIndex[ucCmdSlot]!=0)
                {
                    tHostReq.StartSec = 0;
                }
                tHostReq.Seclength = SEC_PER_BUF-tHostReq.StartSec;
                if(tHostReq.Seclength>ulRemLen[ucCmdSlot])
                {
                    tHostReq.Seclength = ulRemLen[ucCmdSlot];
                }
            }
            if(tHostReq.Type==HOST_REQ_WRITE)
            {
                tHostReq.StartSec = 0;
                tHostReq.Seclength = SEC_PER_BUF;
            }

            #if defined(FPGA)
            DBG_Printf("slot:%d pu:%d blk:0x%x pg:%d secstart:0x%x seclen:0x%x totallen:0x%x\n",ucCmdSlot,tHostReq.PU,tHostReq.Block,tHostReq.Page,tHostReq.
            StartSec,tHostReq.Seclength,g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_C].
            UnitLength);
            #endif
            if (HOST_REQ_READ == tHostReq.Type)
            {
                bRet = NFC_BuildHostReadReq(&tHostReq,bSplit);
            }
            else if (HOST_REQ_WRITE == tHostReq.Type)
            {
                bRet = NFC_BuildHostWriteReq(&tHostReq);
            }
            else
            {
                return FALSE;
            }

            if (FALSE == bRet)
            {
                DBG_Printf("host cmd type:%d build fail tag:%d\n",tHostReq.Type,ucCmdSlot);

                break;
            }

            g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_C] += tHostReq.Seclength;

            ulRemLen[ucCmdSlot] -= tHostReq.Seclength;
            ulIndex[ucCmdSlot]++;
            ulSendLen[ucCmdSlot]+=tHostReq.Seclength;
            tHostReq.HostAddrLow+=(tHostReq.Seclength*SEC_SIZE);
            //g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_C].HostAddrLow +=(tHostReq.Seclength*SEC_SZ);
//           g_tHsg.HostAddrLow

        }

    }
    return TRUE;
}

/* end of file MixVector_Flash.c */

