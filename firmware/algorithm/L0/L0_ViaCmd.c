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
* File Name    : L0_ViaCmd.c
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2015.1.13
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#ifndef SIM
#include "Uart.h"
#include "HAL_SpiDriver.h"
#endif
#include "HAL_Dmae.h"
#include "L0_ViaCmd.h"
#include "L0_Interface.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "HAL_HostInterface.h"
#include "HAL_TraceLog.h"
#include "COM_Memory.h"
#include "Disk_Config.h"
#include "HAL_ParamTable.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/* VIA command state machine */
#define VC_STAT_IDLE                0
#define VC_STAT_NEW_CMD             1
#define VC_STAT_WAITING_SUBSYS      2
#define VC_STAT_COMPLETE            3

#define IS_DW_ALIGN(n)      (((n) & 3) == 0)
#define NOT_DW_ALIGN(n)     (((n) & 3) != 0)
#define BYTE_TO_SEC(n)      (((n) + SEC_SIZE_MSK) / SEC_SIZE)

#define CHECK_VIA_CMD_STATUS(state) \
    if (VCS_SUCCESS != (state)) \
    {\
        return (state);\
    }

#define MEM_IN_DRAM(addr)          (((addr) >= DRAM_START_ADDRESS) && ((addr) < (DRAM_START_ADDRESS + g_ulDramTotalSize)))
#define MEM_IN_DRAM_DCACHE(addr)   (((addr) >= (DRAM_START_ADDRESS + DRAM_HIGH_ADDR_OFFSET)) && ((addr) < (DRAM_START_ADDRESS + DRAM_HIGH_ADDR_OFFSET + g_ulDramTotalSize))) 

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL U32 g_ulSubsysNum;
extern GLOBAL U32 g_ulATARawBuffStart;
extern GLOBAL U32 g_ulTraceLogBuffStart;
extern GLOBAL U32 g_ulHostInfoAddr;
extern GLOBAL U32 g_ulSubSysBootOk;
extern GLOBAL U32 g_ulL0IdleTaskFinished;
extern GLOBAL U32 g_ulRawDataReadyFlag;
extern GLOBAL VIA_CMD_STATUS g_tL0ViaCmdSts;

GLOBAL VCM_PARAM g_VCM_Param;
GLOBAL U32 g_ulVarTableAddr;
GLOBAL U32 g_ulVUARTRawDataReadyFlag;

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL VIA_CMD_STATUS L0_ViaCmdMemRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam);
LOCAL VIA_CMD_STATUS L0_ViaCmdMemWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam);
LOCAL VIA_CMD_STATUS L0_ViaCmdFlashRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam);
LOCAL VIA_CMD_STATUS L0_ViaCmdFlashWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam);
LOCAL VIA_CMD_STATUS L0_ViaCmdFlashErase(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam);
LOCAL VIA_CMD_STATUS L0_ViaCmdDevCtrl(U8 ucSlot, VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam, U32* pOut);
LOCAL VIA_CMD_STATUS L0_DevCtrlVarTable(U32* pOut);
LOCAL VIA_CMD_STATUS L0_ViaCmdEnterVcm(void);
LOCAL VIA_CMD_STATUS L0_ViaCmdExitVcm(void);
LOCAL PSCMD L0_AllocNewScmdNode(U8 ucForMcuID, U32 *pSubSysIdx);

LOCAL PVAR_TABLE  l_pVarTable;
/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_CheckAddrRange(U32 ulAddr, U32 ulByteLen)
{
    VIA_CMD_STATUS eStatus = VCS_MEM_RANGE_ERROR;

    if ((ulAddr >= (DRAM_START_ADDRESS + DRAM_HIGH_ADDR_OFFSET)) && (ulAddr < ((DRAM_START_ADDRESS + DRAM_HIGH_ADDR_OFFSET) + g_ulDramTotalSize)))
    {
        if ((ulAddr + ulByteLen) <= (DRAM_START_ADDRESS + DRAM_HIGH_ADDR_OFFSET + g_ulDramTotalSize))
        {
            eStatus = VCS_SUCCESS;
        }
    }
    else if ((ulAddr >= DRAM_START_ADDRESS) && (ulAddr < (DRAM_START_ADDRESS + g_ulDramTotalSize)))
    {
        if ((ulAddr + ulByteLen) <= (DRAM_START_ADDRESS + g_ulDramTotalSize))
        {
            eStatus = VCS_SUCCESS;
        }
    }
    else if((ulAddr >= SRAM0_START_ADDRESS) && (ulAddr < (SRAM0_START_ADDRESS + DSRAM0_ALLOCATE_SIZE)))
    {
        if ((ulAddr + ulByteLen) <= (SRAM0_START_ADDRESS + DSRAM0_ALLOCATE_SIZE))
        {
            eStatus = VCS_SUCCESS;
        }
    }
    else if((ulAddr >= SRAM1_START_ADDRESS) && (ulAddr < (SRAM1_START_ADDRESS + DSRAM1_ALLOCATE_SIZE)))
    {
        if ((ulAddr + ulByteLen) <= (SRAM1_START_ADDRESS + DSRAM1_ALLOCATE_SIZE))
        {
            eStatus = VCS_SUCCESS;
        }
    }
    else if((ulAddr >= OTFB_START_ADDRESS) && (ulAddr < (OTFB_START_ADDRESS + OTFB_ALLOCATE_SIZE)))
    {
        if ((ulAddr + ulByteLen) <= (OTFB_START_ADDRESS + OTFB_ALLOCATE_SIZE))
        {
            eStatus = VCS_SUCCESS;
        }
    }
    else if((ulAddr >= APB_BASE) && (ulAddr < (APB_BASE + APB_SIZE)))
    {
        if ((ulAddr + ulByteLen) <= (APB_BASE + APB_SIZE))
        {
            eStatus = VCS_SUCCESS;
        }
    }

    /* data length check */
    if (VCS_SUCCESS == eStatus)
    {
        if (ulByteLen > BUF_SIZE)
        {
            eStatus = VCS_DATALEN_OVERFLOW;
        }
    }
    
    /* this align requirement need to confim */
    if (VCS_SUCCESS == eStatus)
    {
        if (NOT_DW_ALIGN(ulAddr) || (NOT_DW_ALIGN(ulByteLen)))
        {
            DBG_Printf("warning: addr(%x) or len(%x) not dword align\n", ulAddr, ulByteLen);
        }
    }
    
    return eStatus;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_CheckFlashParam(const VIA_CMD_PARAM *pCmdParam)
{
    /* to be continue */
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_CheckMcuID(VIA_CMD_CODE eViaCmd, U8 usMcuID)
{
    if((usMcuID < MCU0_ID) || (usMcuID > MCU2_ID))
    {
        return VCS_MCUID_ERROR;
    }

    switch(eViaCmd)
    {
        case VIA_CMD_FLASH_READ:
        case VIA_CMD_FLASH_WRITE:
        case VIA_CMD_FLASH_ERASE:
        if (MCU0_ID == usMcuID)
        {
            return VCS_MCUID_ERROR;
        }
        break;

        default:
            break;
    }

    return VCS_SUCCESS;
}



/*==============================================================================
Func Name  : L0_ViaCmdCheckParam
Input      : VIA_CMD_CODE eViaCmd            
             const VIA_CMD_PARAM *pCmdParam  
Output     : NONE
Return Val : VSC_SUCCESS(0) means paramter valid, other result means invalid.
Discription: check via vendor command parameter, if all command parameter valid
             this interface shall reture VSC_SUCCESS(0).
Usage      : for DMA command, if return 0, this command must process succussful.
             for device control command, just a reference, maybe success, maybe
             fail, but if return a non_zero_number, this command failed.
History    : 
    1. 2015.1.14 Haven Yang create function
==============================================================================*/
VIA_CMD_STATUS MCU0_DRAM_TEXT L0_ViaCmdCheckParam(VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam)
{
    VIA_CMD_STATUS eStatus = VCS_SUCCESS;

    switch(eViaCmd)
    {
        case VIA_CMD_MEM_READ:
        case VIA_CMD_MEM_WRITE:
        {
            eStatus = L0_CheckMcuID(eViaCmd, pCmdParam->tMemAccess.bsMcuID);
            CHECK_VIA_CMD_STATUS(eStatus);
        #ifndef HOST_CMD_REC
            //eStatus = L0_CheckAddrRange(pCmdParam->tMemAccess.ulDevAddr, pCmdParam->tMemAccess.bsByteLen);
            CHECK_VIA_CMD_STATUS(eStatus);
        #endif
        }
        break;
        case VIA_CMD_FLASH_READ:
        case VIA_CMD_FLASH_WRITE:
        case VIA_CMD_FLASH_ERASE:
        {
            eStatus = L0_CheckMcuID(eViaCmd, pCmdParam->tFlashAccess.bsMcuID);
            CHECK_VIA_CMD_STATUS(eStatus);
            eStatus = L0_CheckFlashParam(pCmdParam);
            CHECK_VIA_CMD_STATUS(eStatus);
        }
        break;
        case VIA_CMD_REG_READ:
        case VIA_CMD_REG_WRITE:
        {
            eStatus = L0_CheckMcuID(eViaCmd, pCmdParam->tRegAccess.bsMcuID);
            CHECK_VIA_CMD_STATUS(eStatus);
            if(NOT_DW_ALIGN(pCmdParam->tRegAccess.ulAddr))
            {
                eStatus = VCS_ADDR_NOT_DW_ALIGN;
            }
        }
        break;
        default:
            eStatus = VCS_SUCCESS;
            break;
    }

    return eStatus;
    
}

MCU0_DRAM_TEXT LOCAL void L0_SendDevCtrlToSusSys(U8 ucSlot, U8 ulSubSysIdx, U8 ucViaCmdCode, const VIA_CMD_PARAM *pCmdParam)
{
    PSCMD   pCurrSCmd;
    
    while (TRUE == L0_IsSCQFull(ulSubSysIdx))
    {
        L0_RecyleSCmd();
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
    
    pCurrSCmd->ucSlotNum  = ucSlot;
    pCurrSCmd->ucSCmdType = SCMD_VIA_DEV_CTRL;
    pCurrSCmd->tViaDevCtrl.ucViaCmdCode = ucViaCmdCode;
    pCurrSCmd->tViaDevCtrl.tViaParam = *pCmdParam;
    pCurrSCmd->tViaDevCtrl.ucCriticalFlag = FALSE;  /* to be confirm */

    L0_PushSCmdNode(ulSubSysIdx);

    L0_WaitForAllSCmdCpl(ulSubSysIdx);

}

MCU0_DRAM_TEXT LOCAL void L0_VarTableHeadInit(void)
{
    VAR_HEAD_TABLE *pVarHead;
    VAR_HEAD_L0_TABLE *pL0Table;
    TL_INFO* pTLInfo = TL_GetTLInfo(MCU0_ID);
   
    /* init var head table */
    pVarHead = &l_pVarTable->tVarHeadTable;
    pL0Table = &pVarHead->tL0Table;

    pL0Table->ucSubSysCnt = g_ulSubsysNum;
    pL0Table->ulTLInfoAddr = (U32)pTLInfo;

    /* init FW debug infos */
    pL0Table->ulHInfoBaseAddr   = g_ulHostInfoAddr;
    pL0Table->ulHInfoBaseSize   = sizeof(HOST_INFO_PAGE);
    pL0Table->ulHwTraceBaseAddr = DRAM_FW_HAL_TRACE_BASE;
    pL0Table->ulHwTraceBaseSize = DRAM_FW_HAL_TRACE_SIZE;

    return;
}

MCU0_DRAM_TEXT LOCAL void L0_VarTableSubsysInit(void)
{
    U32 ulSubSysIdx;
    PSCMD pCurrSCmd;
  
    for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
    {
        L0_WaitForAllSCmdCpl(ulSubSysIdx);
        
        if (TRUE == L0_IsSCQFull(ulSubSysIdx))
        {
            DBG_Getch();
        }
        
        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);        
        pCurrSCmd->ucSCmdType = (U8)SCMD_VIA_DEV_CTRL;
        pCurrSCmd->tViaDevCtrl.ucViaCmdCode = VIA_CMD_VAR_TABLE;
        pCurrSCmd->tViaDevCtrl.ulCmdDefine  = g_ulVarTableAddr;

        L0_PushSCmdNode(ulSubSysIdx);
    }

    for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
    {
        L0_WaitForAllSCmdCpl(ulSubSysIdx);
    }
}


/*==============================================================================
Func Name  : L0_ViaCmdInit
Input      : void  
Output     : NONE
Return Val : 
Discription: initialization for VIA vendor command resources.
Usage      : to use VIA command interfaces, call this interface at L0_Init.
History    : 
    1. 2015.1.20 Haven Yang create function
==============================================================================*/
MCU0_DRAM_TEXT void L0_ViaCmdInit(void)
{
    l_pVarTable = (PVAR_TABLE)g_ulVarTableAddr;

    L0_VarTableHeadInit();
    L0_VarTableSubsysInit();

    return;
}

/*==============================================================================
Func Name  : L0_ViaHostCmd
Input      : U8 ucSlot                          
             VIA_CMD_CODE eViaCmd               
             const VIA_CMD_PARAM *pCmdParam  
Output     : U32* pOut: command result for feedbacking to host.
Return Val : 0: cmd finished successful; other num: cmd failed or did not complete.
Discription: this function is defined for processing VIA vendor define command. 
             all VIA vendor define command must call this interface. 
             this interface shared by SATA/AHCI/NMVE mode.
Usage      : after received all VIA_CMD_PARAM, call this interface, if return
             VCS_WAITING_RESOURCE, call it again at next loop.
History    : 
    1. 2015.1.14 Haven Yang create function
==============================================================================*/
MCU0_DRAM_TEXT VIA_CMD_STATUS L0_ViaHostCmd(U8 ucSlot, VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam, U32* pOut)
{
    VIA_CMD_STATUS eStatus;

    eStatus = L0_ViaCmdCheckParam(eViaCmd, pCmdParam);
    CHECK_VIA_CMD_STATUS(eStatus);

    switch(eViaCmd)
    {
        case VIA_CMD_MEM_READ:
            eStatus = L0_ViaCmdMemRead(ucSlot, pCmdParam);
            break;
        case VIA_CMD_MEM_WRITE:
            eStatus = L0_ViaCmdMemWrite(ucSlot, pCmdParam);
            break;
        case VIA_CMD_FLASH_READ:
            eStatus = L0_ViaCmdFlashRead(ucSlot, pCmdParam);
            break;
        case VIA_CMD_FLASH_WRITE:
            eStatus = L0_ViaCmdFlashWrite(ucSlot, pCmdParam);
            break;
        case VIA_CMD_FLASH_ERASE:
            eStatus = L0_ViaCmdFlashErase(ucSlot, pCmdParam);
            break;
        case VIA_CMD_ENTER_VCM:
            eStatus = L0_ViaCmdEnterVcm();
            break;
        case VIA_CMD_EXIT_VCM:
            eStatus = L0_ViaCmdExitVcm();
            break;
        default:
            eStatus = L0_ViaCmdDevCtrl(ucSlot, eViaCmd, pCmdParam, pOut);
            break;
        
    }
    
    return eStatus;
}


MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdMemRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    PSCMD   pCurrSCmd;
    U32     ulLocalAddr;
    U32     ulDataByteLen;
    U32     ulSubSysIdx;

    pCurrSCmd = L0_AllocNewScmdNode(pCmdParam->tMemAccess.bsMcuID, &ulSubSysIdx);

    if (NULL == pCurrSCmd)
    {
        return VCS_WAITING_RESOURCE;
    }

    ulLocalAddr   = pCmdParam->tMemAccess.ulDevAddr;
    ulDataByteLen = pCmdParam->tMemAccess.bsByteLen;

    if(pCmdParam->tMemAccess.bsByteLen > 64* SEC_SIZE)
    {
        DBG_Printf("Rawdata max length: 64 Sector\n");
        DBG_Getch();
    }
    pCurrSCmd->ucSlotNum  = ucSlot;
#ifndef SIM
    #ifdef HOST_NVME
    pCurrSCmd->ucSCmdType = SCMD_VIA_UART_RAW_DATA_REQ;
    #else
    pCurrSCmd->ucSCmdType = (HAL_UartIsMp()) ? SCMD_VIA_UART_RAW_DATA_REQ : SCMD_RAW_DATA_REQ;
    #endif
#else
    pCurrSCmd->ucSCmdType = SCMD_RAW_DATA_REQ;
#endif
    pCurrSCmd->tRawData.ulBuffAddr = g_ulATARawBuffStart;
    pCurrSCmd->tRawData.ucSecLen  = BYTE_TO_SEC(ulDataByteLen);//64;
    pCurrSCmd->tRawData.ucDataDir = RAWDRQ_D2H;
    pCurrSCmd->tRawData.tViaParam = *pCmdParam;
    pCurrSCmd->tRawData.ucSATAUsePIO = FALSE;

    /* prepare data */
#ifndef SIM 
    if (HAL_UartIsMp())
    {   
        pCurrSCmd->ucSCmdSpecific = VIA_CMD_MEM_READ;
        g_ulVUARTRawDataReadyFlag = FALSE;
    }
    else if ((MCU0_ID == pCmdParam->tMemAccess.bsMcuID) || MEM_IN_DRAM(ulLocalAddr) || MEM_IN_DRAM_DCACHE(ulLocalAddr))
    {
        if ((BUF_SIZE == ulDataByteLen) && (MEM_IN_DRAM(ulLocalAddr) || MEM_IN_DRAM_DCACHE(ulLocalAddr)))
        {
            pCurrSCmd->tRawData.ulBuffAddr = ulLocalAddr;
            if (MEM_IN_DRAM_DCACHE(ulLocalAddr))
            {
                pCurrSCmd->tRawData.ulBuffAddr = ulLocalAddr - DRAM_HIGH_ADDR_OFFSET;
            }
        }
        else
        {
            COM_MemByteCopy((U8 *)g_ulATARawBuffStart, (U8 *)ulLocalAddr, ulDataByteLen);
        }
        pCurrSCmd->ucSCmdSpecific = VIA_CMD_NULL;
    }
    else
    {
        pCurrSCmd->ucSCmdSpecific = VIA_CMD_MEM_READ;
    }

    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        g_ulRawDataReadyFlag = FALSE;
    }
    /* mcu12 xfer data */
    L0_PushSCmdNode(ulSubSysIdx);

    /* waiting mcu12 process done */
    L0_WaitForAllSCmdCpl(ulSubSysIdx);
#endif    
    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        if(g_ulRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
    else
    {
        if(g_ulVUARTRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdMemWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    PSCMD   pCurrSCmd;
    U32     ulLocalAddr;
    U32     ulDataByteLen;
    U32     ulSubSysIdx;

    pCurrSCmd = L0_AllocNewScmdNode(pCmdParam->tMemAccess.bsMcuID, &ulSubSysIdx);

    if (NULL == pCurrSCmd)
    {
        return VCS_WAITING_RESOURCE;
    }
    
    ulLocalAddr   = pCmdParam->tMemAccess.ulDevAddr;
    ulDataByteLen = pCmdParam->tMemAccess.bsByteLen;

    pCurrSCmd->ucSlotNum  = ucSlot;
#ifndef SIM
    #ifdef HOST_NVME
    pCurrSCmd->ucSCmdType = SCMD_VIA_UART_RAW_DATA_REQ;
    #else
    pCurrSCmd->ucSCmdType = (HAL_UartIsMp()) ? SCMD_VIA_UART_RAW_DATA_REQ : SCMD_RAW_DATA_REQ;
    #endif
#else
    pCurrSCmd->ucSCmdType = SCMD_RAW_DATA_REQ;
#endif
    pCurrSCmd->tRawData.ulBuffAddr = g_ulATARawBuffStart;
    pCurrSCmd->tRawData.ucSecLen  = BYTE_TO_SEC(ulDataByteLen);//64;
    pCurrSCmd->tRawData.ucDataDir = RAWDRQ_H2D;
    pCurrSCmd->tRawData.tViaParam = *pCmdParam;
    pCurrSCmd->tRawData.ucSATAUsePIO = FALSE;

    if (MCU0_ID == pCmdParam->tMemAccess.bsMcuID)
    {
        /* notice subsys only receive data and do nothing */
        pCurrSCmd->ucSCmdSpecific = VIA_CMD_NULL;
    }
    else
    {
        /* notice subsys receive data and write to local dest addr */
        pCurrSCmd->ucSCmdSpecific = VIA_CMD_MEM_WRITE;
    }

    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        g_ulRawDataReadyFlag = FALSE;
    }
    else
    {
        g_ulVUARTRawDataReadyFlag = FALSE;
    }
    /* mcu12 xfer data */
    L0_PushSCmdNode(ulSubSysIdx);

    /* waiting mcu12 process done */
    L0_WaitForAllSCmdCpl(ulSubSysIdx);

    if (MCU0_ID == pCmdParam->tMemAccess.bsMcuID)
    {
        COM_MemByteCopy((U8 *)ulLocalAddr, (U8 *)g_ulATARawBuffStart, ulDataByteLen);
    }
    
    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        if(g_ulRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
    else
    {
        if(g_ulVUARTRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
}


MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdFlashAccess(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam, VIA_CMD_CODE eViaCmd)
{
    PSCMD   pCurrSCmd;
    U32     ulSubSysIdx;

    ulSubSysIdx = pCmdParam->tFlashAccess.bsMcuID - MCU1_ID;

    /* Force sub system idle */
    L0_WaitForAllSCmdCpl(ulSubSysIdx);
    L0_IssueIdleSCmd(ulSubSysIdx, IDLE_CRITICAL);
    L0_WaitForAllSCmdCpl(ulSubSysIdx);

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
    if (NULL == pCurrSCmd)
    {
        DBG_Getch();
    }

    pCurrSCmd->ucSlotNum  = ucSlot;
#ifndef SIM
    #ifdef HOST_NVME
    pCurrSCmd->ucSCmdType = SCMD_VIA_UART_RAW_DATA_REQ;
    #else
    pCurrSCmd->ucSCmdType = (HAL_UartIsMp()) ? SCMD_VIA_UART_RAW_DATA_REQ : SCMD_RAW_DATA_REQ;
    #endif
#else
    pCurrSCmd->ucSCmdType = SCMD_RAW_DATA_REQ;
#endif

    /* BuffAddr means FlashStatus,Host read status to know flash cmd finish.
       So no matter flash read/write, DataDir is always D2H */
    pCurrSCmd->tRawData.ulBuffAddr = g_ulATARawBuffStart; 
    pCurrSCmd->tRawData.ucSecLen  = 1;   

    pCurrSCmd->tRawData.ucDataDir = RAWDRQ_D2H;

    pCurrSCmd->tRawData.tViaParam = *pCmdParam;

    pCurrSCmd->tRawData.ucSATAUsePIO = FALSE;

    pCurrSCmd->ucSCmdSpecific = (U8)eViaCmd;

    if (0 == eViaCmd)
    {
        DBG_Getch();
    }

    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        g_ulRawDataReadyFlag = FALSE;
    }
    else
    {
        g_ulVUARTRawDataReadyFlag = FALSE;
    }	
    /* mcu12 xfer data */
    L0_PushSCmdNode(ulSubSysIdx);

    /* waiting mcu12 process done */
    L0_WaitForAllSCmdCpl(ulSubSysIdx);

    if(pCurrSCmd->ucSCmdType == SCMD_RAW_DATA_REQ)
    {
        if(g_ulRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
    else
    {
        if(g_ulVUARTRawDataReadyFlag == TRUE)
        {
            return VCS_SUCCESS;
        }
        else
        {
            return VCS_OPERATION_FAIL;
        }
    }
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdFlashRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    return L0_ViaCmdFlashAccess(ucSlot, pCmdParam, VIA_CMD_FLASH_READ);
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdFlashWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    return L0_ViaCmdFlashAccess(ucSlot, pCmdParam, VIA_CMD_FLASH_WRITE);
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdFlashErase(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    return L0_ViaCmdFlashAccess(ucSlot, pCmdParam, VIA_CMD_FLASH_ERASE);
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlJump(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlL2Format(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    /*MP tool don't execute L2 format in Ramdisk mode, just return SUCCESS*/
#if !(defined(L1_FAKE) || defined(L2_FAKE))
    U32 ulSubSysIdx;
    
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;
    
    DBG_Printf("L0_DevCtrlL2Format to SubSystem %d \n", ulSubSysIdx);

    /* L2 format contains L1/L2 LLF and SubSystem bootup flow */
    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_L2_FORMAT, pCmdParam);
#endif
    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlL3Format(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
    pSaveFW pSaveFWFunc;
    
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;

    if (0 == ulSubSysIdx)
    {
        //COM_MemCpy((U32 *)DRAM_BOOTLOADER_PART0_BASE, (U32 *)OTFB_BOOTLOADER_BASE, BOOTLOADER_PART0_SIZE>>2);
        //HAL_PrepareFlagsForNormalBoot();

        //patch NFC scramble seed race condition by MCU0&MUC2 while saving FW via MPT
        g_ulL0IdleTaskFinished = FALSE;

        do
        {
            L0_IssueIdleSCmd(ulSubSysIdx, (U32)IDLE_NORMAL);
            L0_WaitForAllSCmdCpl(ulSubSysIdx);
        } while (FALSE == g_ulL0IdleTaskFinished);
        
        pSaveFWFunc = (pSaveFW)HAL_GetFTableFuncAddr(SAVE_FIRWARE);
        pSaveFWFunc(0, DRAM_FW_UPDATE_BASE);
    }
    
    DBG_Printf("L0_DevCtrlL3Format to SubSystem %d done!\n", ulSubSysIdx);

    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlDebugShowAll(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;

    for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
    {
        L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_DBG_SHOWALL, pCmdParam);
    }

    return g_tL0ViaCmdSts;
}


MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlRegRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam, U32* pOut)
{
    PSCMD   pCurrSCmd;
    U32     ulLocalAddr;
    U32     ulSubSysIdx;
    U8      ucMcuID;

    ucMcuID = pCmdParam->tRegAccess.bsMcuID;
    ulLocalAddr = pCmdParam->tRegAccess.ulAddr;

    if (MCU0_ID == ucMcuID)
    {
        *pOut = *(volatile U32*)ulLocalAddr;
    }
    else
    {
        ulSubSysIdx = ucMcuID - MCU1_ID;
        if (FALSE == L0_IsSCQFull(ulSubSysIdx))
        {
            pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
        }
        else
        {
            return VCS_WAITING_RESOURCE;
        }

        pCurrSCmd->ucSlotNum  = ucSlot;
        pCurrSCmd->ucSCmdType = SCMD_VIA_DEV_CTRL;
        pCurrSCmd->tViaDevCtrl.ucViaCmdCode = VIA_CMD_REG_READ;
        pCurrSCmd->tViaDevCtrl.tViaParam = *pCmdParam;

        L0_PushSCmdNode(ulSubSysIdx);

        L0_WaitForAllSCmdCpl(ulSubSysIdx);

        *pOut = pCurrSCmd->tViaDevCtrl.aOutputValue[0];
    }
    
    return g_tL0ViaCmdSts;
}
MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFlashPreCondition(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
  
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;

    DBG_Printf("L0_DevCtrlFlashPreCondition to SubSystem %d \n", ulSubSysIdx);

    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_FLASH_PRECONDITION, pCmdParam);

    return g_tL0ViaCmdSts;

}
MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFlashSetParam(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    PSCMD   pCurrSCmd;
    U32     ulSubSysIdx;
    U8      ucMcuID;

    ucMcuID = pCmdParam->tRegAccess.bsMcuID;
    ulSubSysIdx = ucMcuID - MCU1_ID;
    if (FALSE == L0_IsSCQFull(ulSubSysIdx))
    {
        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
    }
    else
    {
        return VCS_WAITING_RESOURCE;
    }

    pCurrSCmd->ucSlotNum  = ucSlot;
    pCurrSCmd->ucSCmdType = SCMD_VIA_DEV_CTRL;
    pCurrSCmd->tViaDevCtrl.ucViaCmdCode = VIA_CMD_FLASH_SETPARAM;
    pCurrSCmd->tViaDevCtrl.tViaParam = *pCmdParam;

    L0_PushSCmdNode(ulSubSysIdx);

    L0_WaitForAllSCmdCpl(ulSubSysIdx);
    
    return g_tL0ViaCmdSts;
}
MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFlashTerminate(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
  
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;

    DBG_Printf("L0_DevCtrlFlashTerminate to SubSystem %d \n", ulSubSysIdx);

    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_FLASH_TERMINATE, pCmdParam);

    return g_tL0ViaCmdSts;

}
MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFlashReset(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
  
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;

    DBG_Printf("L0_DevCtrlFlashReset to SubSystem %d \n", ulSubSysIdx);

    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_FLASH_RESET, pCmdParam);

    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlRegWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    PSCMD   pCurrSCmd;
    U32     ulLocalAddr;
    U32     ulSubSysIdx;
    U8      ucMcuID;

    ucMcuID = pCmdParam->tRegAccess.bsMcuID;
    ulLocalAddr = pCmdParam->tRegAccess.ulAddr;

    if (MCU0_ID == ucMcuID)
    {
        *(volatile U32*)ulLocalAddr = pCmdParam->tRegAccess.ulData;
    }
    else
    {
        ulSubSysIdx = ucMcuID - MCU1_ID;
        if (FALSE == L0_IsSCQFull(ulSubSysIdx))
        {
            pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
        }
        else
        {
            return VCS_WAITING_RESOURCE;
        }

        pCurrSCmd->ucSlotNum  = ucSlot;
        pCurrSCmd->ucSCmdType = SCMD_VIA_DEV_CTRL;
        pCurrSCmd->tViaDevCtrl.ucViaCmdCode = VIA_CMD_REG_WRITE;
        pCurrSCmd->tViaDevCtrl.tViaParam = *pCmdParam;

        L0_PushSCmdNode(ulSubSysIdx);

        L0_WaitForAllSCmdCpl(ulSubSysIdx);
    }
    
#ifndef SIM
    if (((pCmdParam->aDW[1]>>16) & 0xfff) == 0x136)
    {
        DBG_Printf("L0_DevCtrlRegWrite UartMPInit\n");
		L0_UartMpInit();
   	}
#endif
    return g_tL0ViaCmdSts;
}


MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlTraceLog(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32             ulSecCnt;
    U32             ulSubSysIdx;
    VIA_TLCT        eCtrlType;
    //VIA_CMD_STATUS  eStatus = VCS_SUCCESS;
    
    eCtrlType = (VIA_TLCT)pCmdParam->aByte[0];

    switch(eCtrlType)
    {
        case TLCT_DISABLE_TL:
            TL_Disable();
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                L0_SendDevCtrlToSusSys(ucSlot, ulSubSysIdx, VIA_CMD_TRACELOG_CONTROL, pCmdParam);
            }
        break;

        case TLCT_ENABLE_TL:
            TL_Enable();
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                L0_SendDevCtrlToSusSys(ucSlot, ulSubSysIdx, VIA_CMD_TRACELOG_CONTROL, pCmdParam);
            }
        break;

        case TLCT_FLUSH_DATA:
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                L0_SendDevCtrlToSusSys(ucSlot, ulSubSysIdx, VIA_CMD_TRACELOG_CONTROL, pCmdParam);
            }
        break;

        case TLCT_INVALID_DATA:
            ulSecCnt = (pCmdParam->aDW[0] >> 16);
            if (MCU0_ID == pCmdParam->aByte[8])
            {
                TL_InvalidateTraceMemory(ulSecCnt);
            }
            else
            {
                ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;
                L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_TRACELOG_CONTROL, pCmdParam);
            }
        break;

        default:
            g_tL0ViaCmdSts = VCS_INVALID_PARAM;
            break;
    }

    
    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlBBTSave(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
  
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;

    DBG_Printf("L0_DevCtrlBBTSave to SubSystem %d \n", ulSubSysIdx);

    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_BBT_SAVE, pCmdParam);

    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlBBTLoad(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSubSysIdx;
    
    ulSubSysIdx = pCmdParam->aByte[8] - MCU1_ID;
    
    DBG_Printf("L0_DevCtrlBBTLoad to SubSystem %d \n", ulSubSysIdx);
    
    L0_SendDevCtrlToSusSys(ucSlot, (U8)ulSubSysIdx, VIA_CMD_BBT_LOAD, pCmdParam);

    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlGetIDB()
{
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFwSave(const VIA_CMD_PARAM *pCmdParam)
{
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlFwLoad(void)
{
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlActive(const VIA_CMD_PARAM *pCmdParam)
{
    return VCS_SUCCESS;
}
MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlSpiFlashWrite(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulDesAddr = pCmdParam->tSpiFlashAccess.ulSpiOffset + SPI_START_ADDRESS;
    U32 ulSrcAddr = pCmdParam->tSpiFlashAccess.ulDramAddr;
    U32 ulDataBlockNum = pCmdParam->tSpiFlashAccess.ulDataBlockNum;
    U32 ulDataBlockLen = (4<<10);
#ifndef SIM    
    while(ulDataBlockNum--)
    {
        HAL_SpiDmaWrite(ulDesAddr,ulSrcAddr); // 4K
        ulDesAddr += ulDataBlockLen;
        ulSrcAddr += ulDataBlockLen;
    }
#else
    U32 ulDataLen = ulDataBlockLen * ulDataBlockNum;  // 4k
    COM_MemCpy((U32 *)ulDesAddr, (U32 *)ulSrcAddr, ulDataLen / sizeof(U32));
#endif
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlSpiFlashRead(U8 ucSlot, const VIA_CMD_PARAM *pCmdParam)
{
    U32 ulSrcAddr = pCmdParam->tSpiFlashAccess.ulSpiOffset + SPI_START_ADDRESS;
    U32 ulDesAddr = pCmdParam->tSpiFlashAccess.ulDramAddr;
    U32 ulDataBlockNum = pCmdParam->tSpiFlashAccess.ulDataBlockNum;
    U32 ulDataLen = (ulDataBlockNum << 12);
    
#ifndef SIM    
    HAL_SpiDmaRead(ulDesAddr,ulSrcAddr,ulDataLen);
#else
    COM_MemCpy((U32 *)ulDesAddr, (U32 *)ulSrcAddr, (ulDataLen >> DWORD_SIZE_BITS));
#endif    
       
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlClearDiskLock(U8 ucSlot)
{
    pClearDiskLock pClearDiskLockFunc = (pClearDiskLock)HAL_GetFTableFuncAddr(CLEAR_DISK_LOCK);
    L0_SubSystemOnlineShutdown(FALSE);
    pClearDiskLockFunc  (DRAM_DATA_BUFF_MCU1_BASE);
    L0_SubSystemOnlineReboot();       
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdEnterVcm(void)
{
    DBG_Printf("Enter VCM\n");
    g_VCM_Param.EnterVCM = 1;
    return VCS_SUCCESS;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdExitVcm(void)
{
    DBG_Printf("Exit VCM\n");
    g_VCM_Param.EnterVCM = 0;
    return VCS_SUCCESS;
}

enum {
    FW_UPDATE_SHUTDOWN ,
    FW_UPDATE_WAIT_SHUTDOWN_DONE,
    FW_UPDATE_IDLE,
    FW_UPDATE_BOOT_SUBSYSTEM,    
    FW_UPDATE_PROC,
    FW_UPDATE_FINISH
};

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_ViaCmdDevCtrl(U8 ucSlot, VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam, U32* pOut)
{
    VIA_CMD_STATUS eStatus;

    switch(eViaCmd)
    {
        case VIA_CMD_VAR_TABLE:
            eStatus = L0_DevCtrlVarTable(pOut);
            break;
        case VIA_CMD_JUMP:
            eStatus = L0_DevCtrlJump(ucSlot, pCmdParam);
            break;
        case VIA_CMD_L2_FORMAT:
            eStatus = L0_DevCtrlL2Format(ucSlot, pCmdParam);
            break;
        case VIA_CMD_L3_FORMAT:
            eStatus = L0_DevCtrlL3Format(ucSlot, pCmdParam);
            break;
        case VIA_CMD_REG_READ:
            eStatus = L0_DevCtrlRegRead(ucSlot, pCmdParam, pOut);
            break;
        case VIA_CMD_REG_WRITE:
            eStatus = L0_DevCtrlRegWrite(ucSlot, pCmdParam);
            break;
        case VIA_CMD_TRACELOG_CONTROL:
            eStatus = L0_DevCtrlTraceLog(ucSlot, pCmdParam);
            break;
        case VIA_CMD_BBT_SAVE:
            eStatus = L0_DevCtrlBBTSave(ucSlot, pCmdParam);
            break;
        case VIA_CMD_BBT_LOAD:
            eStatus = L0_DevCtrlBBTLoad(ucSlot, pCmdParam);
            break;
        case VIA_CMD_GET_IDB:
            eStatus = L0_DevCtrlGetIDB();
            break;
        case VIA_CMD_FW_SAVE:
            eStatus = L0_DevCtrlFwSave(pCmdParam);
            break;
        case VIA_CMD_FW_LOAD:
            eStatus = L0_DevCtrlFwLoad();
            break;
        case VIA_CMD_FW_ACTIVE:
            eStatus = L0_DevCtrlActive(pCmdParam);
            break;
        case VIA_CMD_DBG_SHOWALL:
            eStatus = L0_DevCtrlDebugShowAll(ucSlot, pCmdParam);
            break;

        case VIA_CMD_SPI_FLASH_WRITE:
            eStatus = L0_DevCtrlSpiFlashWrite(ucSlot,pCmdParam);
            break;
        case VIA_CMD_SPI_FLASH_READ:
            eStatus = L0_DevCtrlSpiFlashRead(ucSlot,pCmdParam);
            break;  

        case VIA_CMD_CLEAR_DISK_LOCK:
            eStatus = L0_DevCtrlClearDiskLock(ucSlot);
            break;
        case VIA_CMD_FLASH_PRECONDITION:
            eStatus = L0_DevCtrlFlashPreCondition(ucSlot,pCmdParam);
            break;
        case VIA_CMD_FLASH_SETPARAM:
            eStatus = L0_DevCtrlFlashSetParam(ucSlot,pCmdParam);
            break;
        case VIA_CMD_FLASH_TERMINATE:
            eStatus = L0_DevCtrlFlashTerminate(ucSlot,pCmdParam);
            break;
        case VIA_CMD_FLASH_RESET:
            eStatus = L0_DevCtrlFlashReset(ucSlot,pCmdParam);
            break;
        default:
            eStatus = VCS_CMD_CODE_ERROR;
            break;
    }
    
    return eStatus;
}

MCU0_DRAM_TEXT LOCAL VIA_CMD_STATUS L0_DevCtrlVarTable(U32* pOut)
{
    L0_VarTableHeadInit();
    L0_VarTableSubsysInit();

    *pOut = g_ulVarTableAddr;
    
    return g_tL0ViaCmdSts;
}

MCU0_DRAM_TEXT LOCAL PSCMD L0_AllocNewScmdNode(U8 ucForMcuID, U32* pSubSysIdx)
{
    PSCMD pCurrSCmd = NULL;
    U32 ulSubSysIdx;
    
    if (MCU0_ID == ucForMcuID)
    {
        for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
        {
            /* Attempts to acquire one SCMD node from either subsystem. */
            if (FALSE == L0_IsSCQFull(ulSubSysIdx))
            {
                pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                *pSubSysIdx = ulSubSysIdx;
                break;
            }
        }            
    }
    else
    {
        ulSubSysIdx = ucForMcuID - MCU1_ID; 
        if (FALSE == L0_IsSCQFull(ulSubSysIdx))
        {
            pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
            *pSubSysIdx = ulSubSysIdx;
        }
    }

    return pCurrSCmd;
}


/*====================End of this file========================================*/

