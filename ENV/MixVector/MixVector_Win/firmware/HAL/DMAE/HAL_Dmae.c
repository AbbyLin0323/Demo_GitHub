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
Filename    :HAL_Dmae.c
Version     :
Author      :Kristin Wang
Date        :2014.7
Description :this file encapsulate DMAE driver interface          
Others      :
Modify      :
20140912    Kristin    1. Coding style uniform
                       2. Add A0 support
                       3. delete definition of HAL_DMAEClearTrigger, 
                           HAL_DMAEAddCmd, and HAL_DMAETriggerCmd
*******************************************************************************/
#include "COM_Memory.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "HAL_DMAE.h"
#ifndef SIM
#include "HAL_Interrupt.h"
#endif
#ifdef VT3514_A0
#include "HAL_MultiCore.h"
#endif

#ifdef SIM
extern void DMAE_ModelProcessCmdEntry(U8 ucCMDID);
#endif

/*-----------------------------------------------------------------------------
Name: HAL_DMAEIsQueueFull
Description: 
    Bit [0] of rDMAE_CMD_STATUS_FULL indicates if all DMAE command entries are busy
Input Param: 
    none
Output Param: 
    none
Return Value:
    BOOL: FALSE- there is DMAE entry idle
          TRUE- all DMAE command entries are busy
Usage:
    check if all DMAE command entries are busy(queue is full)
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
BOOL DMAE_TEXT_ATTR HAL_DMAEIsQueueFull(void)
{
    return (BOOL)(rDMAE_CMD_STATUS_FULL & 0x1);
}


/*----------------------------------------------------------------------------
Name: HAL_DMAEIsQueueEmpty
Description: 
    Bit [0] of rDMAE_CMD_STATUS_EMPTY indicates if all DMAE command entries are idle
Input Param: 
    none
Output Param: 
    none
Return Value:
    BOOL: FALSE- there is DMAE entry busy
          TRUE- all DMAE command entries are idle
Usage:
    check if all DMAE command entry are idle(queue is empty)
History:  
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
BOOL DMAE_TEXT_ATTR HAL_DMAEIsQueueEmpty(void)
{
    return (BOOL)(rDMAE_CMD_STATUS_EMPTY & 0x1);
}

/*----------------------------------------------------------------------------
Name: HAL_DMAESelMCU
Description: 
    MCU0/1/2 can use some same adrress to access its exclusive memory(actual 
  memory is different for MCU0/1/2),so we must tell DMAE which MCU it should
  select for this command entry.
    On VT3514_A0, select MCU in rGLB(0x64);
    On VT3514_B0 or higher, select MCU in DMAE command entry.
Input Param: 
    U8 ucDMAECmdID: DMAE command entry ID
Output Param: 
    none
Return Value:
    void
Usage:
    When FW fills DMAE command entry,it calls this function to select MCU.
History:  
20140912    Kristin    Coding style uniform, add A0 support
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAESelMCU(U8 ucDMAECmdID, U8 ucMCUID)
{
    U8 ucMCUSel;
    DMAE_CMDENTRY *pCmdEntry = (DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucDMAECmdID * sizeof(DMAE_CMDENTRY));

    switch (ucMCUID)
    {
    case MCU0_ID:
        ucMCUSel = DMAE_SEL_MCU0;
        break;

    case MCU1_ID:
        ucMCUSel = DMAE_SEL_MCU1;
        break;

    case MCU2_ID:
        ucMCUSel = DMAE_SEL_MCU2;
        break;

    default:
        DBG_Printf("HAL_DMAESelMCU: get MCU ID %d error", ucMCUID);
        DBG_Getch();
        break;
    }

#ifdef VT3514_A0
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_DMAE);
    rGlbTrfc &= (~MSK_DMAE_MCU_SEL);
    rGlbTrfc |= (ucMCUSel << 16);
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_DMAE);
#else
    pCmdEntry->bsMCUSel = ucMCUSel;
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_DMAEInit
Description: 
    necessary initialize for DMAE
Input Param: 
    none
Output Param: 
    none
Return Value:
    void
Usage:
    HW initialize  
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAEInit(void)
{
    rDMAE_REQ_PRIORITY &=  (~MASK_DMAE_REQ_PRIORITY);
    rDMAE_REQ_PRIORITY |= DMAE_REQ_PRIORITY_LOW;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_DMAEParseAddress
Description: 
    In DMAE command, FW must tell HW the area type of source and destination,and
  need to convert data address to the format that DMAE ask for.
Input Param: 
    U32 ulAddrIn: absolute address
Output Param:
    U8 *pAreaType: area type, 0- PIF(SRAM), 1- OTFB, 2- DRAM, 3- SPI
    U32 *pAddrOut: data address in DMAE command format
Return Value:
    BOOL: FALSE- the input address is invalid
          TRUE- the input address is valid
Usage:
    When FW fills a DMAE command entry,it calls this function to get 
  'bsDesType'/'bsSrcType' and 'ulSrcAddr'/'ulDesAddr' from the absolute 
  source/destination address.
History:  
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
BOOL DMAE_TEXT_ATTR HAL_DMAEParseAddress(U8 *pAreaType, U32 *pAddrOut, const U32 ulAddrIn)
{  
    if (ulAddrIn < SRAM0_START_ADDRESS)
    {    
        return FALSE;
    }
    else if ((ulAddrIn >= SRAM0_START_ADDRESS) && (ulAddrIn < DRAM_START_ADDRESS)) /* SRAM */
    {
        *pAddrOut = ulAddrIn;        
        *pAreaType = DMAE_SEL_AREA_PIF;     
    }
    else if ((ulAddrIn >= DRAM_START_ADDRESS) && (ulAddrIn < SPI_START_ADDRESS)) /* DRAM */
    {
        *pAddrOut = ulAddrIn;        
        *pAreaType = DMAE_SEL_AREA_DRAM;     
    }
    else if ((ulAddrIn >= SPI_START_ADDRESS) && (ulAddrIn < OTFB_START_ADDRESS)) /* SPI NOR Flash */
    {
        *pAddrOut = ulAddrIn; 
        *pAreaType = DMAE_SEL_AREA_SPI; 
    }
    else if (ulAddrIn >= OTFB_START_ADDRESS) /* OTFB */
    {
        *pAddrOut = ulAddrIn;
        *pAreaType = DMAE_SEL_AREA_OTFB;
    }
    else
    {
        return FALSE;
    }
    
    return TRUE;
}

/*----------------------------------------------------------------------------
Name: HAL_DMAEGetCmdLen
Description: 
    In DMAE command, length value n (n>=0) means 16 * (n + 1) bytes.
Input Param: 
    U32 ulLenByte: length in unit of byte
Output Param:
    none
Return Value:
    U16 : data length in DMAE command format
          0- 16 bytes
          1- 32 bytes ...
Usage:
    When FW fills a DMAE command entry, it calls this function to get 'bsLength'
  from the data length in unit of byte.
History:  
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
U16 DMAE_TEXT_ATTR HAL_DMAEGetCmdLen(U32 ulLenByte)
{
    if (0 != (ulLenByte & 0xF))
    {
        DBG_Printf("HAL_DMAEGetCmdLen: Length is not divisible by 16-byte, error!\n");
        DBG_Getch();
    }

    return (U16)( (ulLenByte >> 4) -1);
}

/*----------------------------------------------------------------------------
Name: HAL_DMAEIsEntryAvailable
Description: 
    If the status of a DMAE command entry is DONE(3) or IDLE(0),that means this
  entry is idle.
Input Param: 
    U8 ucCmdID: DMAE command entry ID, 0~7
Output Param:
    none
Return Value:
    BOOL : TRUE- this command entry is idle
           FALSE- this command entry is busy
Usage:
    check if a DMAE command entry is idle
History:  
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
BOOL DMAE_TEXT_ATTR HAL_DMAEIsEntryAvailable(U8 ucCmdID)
{
    volatile DMAE_CMDENTRY *pCmdEntry;
    U8 ucStatus;
 
    pCmdEntry =  (volatile DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucCmdID * sizeof(DMAE_CMDENTRY));
    ucStatus = pCmdEntry->bsStatus;
    
    if ((DMAE_CMDENTRY_STATUS_DONE == ucStatus) || (DMAE_CMDENTRY_STATUS_IDLE == ucStatus))
    {
        return TRUE;
    }
    
    return FALSE;
}

/*----------------------------------------------------------------------------
Name: HAL_DMAEGetNextCmdID
Description: 
    Get an usable DMAE command entry ID.
    In order to prevent conflict between MCU, FW controls MCU0 can only use
  command entry 0~1, MCU1 can only use command entry 2~4, MCU2 can only use
  cpmmand entry 5~7.
Input Param: 
    none
Output Param:
    none
Return Value:
    U8 : the usable command entry ID, INVALID means no DMAE entry can use 
         currently
Usage:
    In multi-core environment, calling this function to find an idle DMAE 
  command entry and needn't to worry about conflict with other MCUs.

History:  
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
U8 DMAE_TEXT_ATTR HAL_DMAEGetNextCmdID(void)
{
    U8 ucMCUID;
    U8 ucCmdId;
    U8 ucCmdIDRet = INVALID_2F;

    ucMCUID = HAL_GetMcuId();

    switch(ucMCUID)
    {
    case MCU0_ID:
        for (ucCmdId = DMAE_CMD_HEAD_MCU0; ucCmdId <= DMAE_CMD_TAIL_MCU0; ucCmdId++)
        {
            if (TRUE == HAL_DMAEIsEntryAvailable(ucCmdId))
            {
                ucCmdIDRet = ucCmdId;
                break;
            }
        }
        break;

    case MCU1_ID:
        for (ucCmdId = DMAE_CMD_HEAD_MCU1; ucCmdId <= DMAE_CMD_TAIL_MCU1; ucCmdId++)
        {
            if (TRUE == HAL_DMAEIsEntryAvailable(ucCmdId))
            {
                ucCmdIDRet = ucCmdId;
                break;
            }
        }
        break;

    case MCU2_ID:
        for (ucCmdId = DMAE_CMD_HEAD_MCU2; ucCmdId <= DMAE_CMD_TAIL_MCU2; ucCmdId++)
        {
            if (TRUE == HAL_DMAEIsEntryAvailable(ucCmdId))
            {
                ucCmdIDRet = ucCmdId;
                break;
            }
        }
        break;

    default:
        break;
    }

    return ucCmdIDRet;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAEIsCmdDone
Description: 
    Check if a DMAE command entry has been done
Input Param: 
    U8 ucCmdID: a DMAE command entry ID,0~7
Output Param:
    none
Return Value:
    BOOL: TRUE- done
          FALSE- not done
Usage:
    After FW triggers a DMAE command, can use this function to wait request done
History:
20140912    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
BOOL DMAE_TEXT_ATTR HAL_DMAEIsCmdDone(U8 ucCmdID)
{
    volatile DMAE_CMDENTRY *pCmdEntry;
    U8 ucStatus;
    
    pCmdEntry =  (volatile DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucCmdID * sizeof(DMAE_CMDENTRY));
    ucStatus = pCmdEntry->bsStatus;
    
    if (DMAE_CMDENTRY_STATUS_DONE == ucStatus)
    {
        return TRUE;
    }
    
    return FALSE;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAECopyOneBlockLenLimit
Description: 
    Fill a DMAE command entry once to complete a data copying, and the copying
  length is limited to 128KB.
Input Param: 
    U32 ulDesAddr: destination address,it is an absolute address
    U32 ulSrcAddr: source address, it is an absolute address
    U32 ulBlockLenInByte: data length in unit of byte, because the 
                          Lenth field of DMAE command entry is only 13-bit(max to
                          0x1FFF,128KB), it  can't be larger than 0x20000.
    U8 ucMCUID: If neither of source and destination is SRAM, this parameter is
                useless; if source or destination is SRAM, it is the MCU ID 
                which the SRAM belongs to.
Output Param:
    none
Return Value:
    BOOL: TRUE - data copying has been done
          FALSE- fail to fill this data copy request into DMAE command entry, 
                 because there is no available command entry or the input address
                 is invalid.
Usage:
    local function
History:  
20140912    Kristin    Coding style uniform
20141202    Kristin    Add a input parameter ucSramMCUID
------------------------------------------------------------------------------*/
LOCAL BOOL DMAE_TEXT_ATTR HAL_DMAECopyOneBlockLenLimit(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte, U8 ucSramMCUID)
{
    U8 ucCurCmdID;
    volatile DMAE_CMDENTRY *pCurCmdEntry;
    U8 ucDesAreaType;
    U8 ucSrcAreaType;
    U32 ulCmdDesAddr;
    U32 ulCmdSrcAddr;
    
    ucCurCmdID = HAL_DMAEGetNextCmdID();
    if (INVALID_2F == ucCurCmdID)
    {
        return FALSE;
    }

    pCurCmdEntry = (volatile DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucCurCmdID * sizeof(DMAE_CMDENTRY));
    
    if ((FALSE == HAL_DMAEParseAddress(&ucDesAreaType, &ulCmdDesAddr, ulDesAddr))
        || (FALSE == HAL_DMAEParseAddress(&ucSrcAreaType, &ulCmdSrcAddr, ulSrcAddr)))
    {
        return FALSE;
    }
    
    pCurCmdEntry->bTrigger = 0;
    
    pCurCmdEntry->bsDesType = ucDesAreaType;
    pCurCmdEntry->ulDesAddr = ulCmdDesAddr;
    pCurCmdEntry->bsSrcType = ucSrcAreaType;
    pCurCmdEntry->ulSrcAddr = ulCmdSrcAddr;
    pCurCmdEntry->bsLength = HAL_DMAEGetCmdLen(ulBlockLenInByte);
    HAL_DMAESelMCU(ucCurCmdID, ucSramMCUID);
  
    pCurCmdEntry->bTrigger = 1;
#ifdef SIM
    pCurCmdEntry->bsStatus = DMAE_CMDENTRY_STATUS_PENDING;
    DMAE_ModelProcessCmdEntry(ucCurCmdID);
#endif
    
    while (FALSE == HAL_DMAEIsCmdDone(ucCurCmdID))
    {  
    }    

    return TRUE;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAECopyOneBlock
Description: 
    Copy data from DRAM/OTFB/local-SRAM to DRAM/OTFB/local-SRAM
    if enable DMAE,use DMAE to realize.
    otherwise,call COM_MemCpy immediately.
Input Param: 
    U32 ulDesAddr: destination address,it is an absolute address
    U32 ulSrcAddr: source address, it is an absolute address
    U32 ulBlockLenInByte: data copying length in unit of byte, must be divisible 
                          by 16
Output Param:
    none
Return Value:
    void
Usage:
    When this function return, that means data copying has been completed.
    Can't be used for copying from or to other MCU's SRAM.
History:
20140912    Kristin    Coding style uniform
20141202    Kristin    1. add a input when call HAL_DMAECopyOneBlockLenLimit .
                       2. change the usage.
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAECopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte)
{
#ifdef DMAE_ENABLE
    U32 ulCurCopyLen;
    U32 ulCurDesAddr = ulDesAddr;
    U32 ulCurSrcAddr = ulSrcAddr;
    U32 ulRemainLen = ulBlockLenInByte;

    while (ulRemainLen > 0)
    {
        if ( ulRemainLen >= DMAE_ENTRY_MAX_LENGTH_IN_BYTE)
        {
            ulCurCopyLen = DMAE_ENTRY_MAX_LENGTH_IN_BYTE;
        }
        else
        {
            ulCurCopyLen = ulRemainLen;
        }

        while (FALSE == HAL_DMAECopyOneBlockLenLimit(ulCurDesAddr, ulCurSrcAddr, ulCurCopyLen, (U8)HAL_GetMcuId()))
        {
        }

        ulRemainLen -= ulCurCopyLen;
        ulCurDesAddr += ulCurCopyLen;
        ulCurSrcAddr += ulCurCopyLen;
    } 
#else
    COM_MemCpy((U32 *)ulDesAddr, (U32 *)ulSrcAddr, ulBlockLenInByte/sizeof(U32));
#endif

    return ;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAESramCopyOneBlock
Description: 
    Copy data from DRAM/OTFB/SRAM to SRAM, or from SRAM to DRAM/OTFB/SRAM using 
  DMAE. 
Input Param: 
    U32 ulDesAddr: destination address,it is an absolute address
    U32 ulSrcAddr: source address, it is an absolute address
    U32 ulBlockLenInByte: data copying length in unit of byte, must be divisible 
                          by 16
    U8 ucSramMCUID: ID of the MCU which SRAM belongs to.
Output Param:
    none
Return Value:
    void
Usage:
    When this function return, that means data copying has been completed.
    If both source and destination are SRAM, they have to belong to the same MCU.
History:
20141202    Kristin    Create
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAESramCopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte, U8 ucSramMCUID)
{
    U32 ulCurCopyLen;
    U32 ulCurDesAddr = ulDesAddr;
    U32 ulCurSrcAddr = ulSrcAddr;
    U32 ulRemainLen = ulBlockLenInByte;

    while (ulRemainLen > 0)
    {
        if ( ulRemainLen >= DMAE_ENTRY_MAX_LENGTH_IN_BYTE)
        {
            ulCurCopyLen = DMAE_ENTRY_MAX_LENGTH_IN_BYTE;
        }
        else
        {
            ulCurCopyLen = ulRemainLen;
        }

        while (FALSE == HAL_DMAECopyOneBlockLenLimit(ulCurDesAddr, ulCurSrcAddr, ulCurCopyLen, ucSramMCUID))
        {
        }

        ulRemainLen -= ulCurCopyLen;
        ulCurDesAddr += ulCurCopyLen;
        ulCurSrcAddr += ulCurCopyLen;
    } 

    return ;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAESetValueLenLimit
Description: 
    Fill a DMAE command entry once to set a memory area which is not large than 
  128KB with a 32-bit data (that is, every DWORD in this memory area will be 
  set to the data).
Input Param: 
    U32 ulDesAddr: start address of the memory to set,it is an absolute address
    U32 ulBlockLenInByte: memory length in unit of byte, because the Lenth field 
                          of DMAE command entry is only 13-bit(max to 0x1FFF,
                          128KB), it  can't be larger than 0x20000.
    U8 ucMCUID: If the memory isn't SRAM, this parameter is useless; if the 
                memory is SRAM, it is the MCU ID which the SRAM belongs to.
Output Param:
    none
Return Value:
    BOOL: TRUE - data set has been done
          FALSE- fail to fill this data set request into DMAE command entry, 
                 because there is no available command entry or the input address
                 is invalid.
Usage:
    local function
History: 
20140912    Kristin    Coding style uniform
20141202    Kristin    Add a input parameter ucMCUID
------------------------------------------------------------------------------*/
LOCAL BOOL DMAE_TEXT_ATTR HAL_DMAESetValueLenLimit(const U32 ulDesAddr,const U32 ulLenByte, U32 ulValue, U8 ucMCUID)
{
    U8 ucCurCmdID;
    volatile DMAE_CMDENTRY *pCurCmdEntry;
    U8 ucDesAreaType;
    U32 ulCmdDesAddr;
    U32 *pDataReg;
    
    ucCurCmdID = HAL_DMAEGetNextCmdID();
    if (INVALID_2F == ucCurCmdID)
    {
        return FALSE;
    }

    pCurCmdEntry = (volatile DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucCurCmdID * sizeof(DMAE_CMDENTRY));
    pDataReg = (U32 *)(DMAE_REG_DATA_BASE + sizeof(U32) * ucCurCmdID);
    
    if (FALSE == HAL_DMAEParseAddress(&ucDesAreaType, &ulCmdDesAddr, ulDesAddr))
    {
        return FALSE;
    }
    
    pCurCmdEntry->bTrigger = 0;
    
    pCurCmdEntry->bsDesType = ucDesAreaType;
    pCurCmdEntry->ulDesAddr = ulCmdDesAddr;
    pCurCmdEntry->bsSrcType = DMAE_SEL_AREA_REG;
    pCurCmdEntry->bsLength = HAL_DMAEGetCmdLen(ulLenByte);
    HAL_DMAESelMCU(ucCurCmdID, ucMCUID);
    *pDataReg = ulValue;
    
    pCurCmdEntry->bTrigger = 1;
#ifdef SIM
    pCurCmdEntry->bsStatus = DMAE_CMDENTRY_STATUS_PENDING;
    DMAE_ModelProcessCmdEntry(ucCurCmdID);
#endif

    while (FALSE == HAL_DMAEIsCmdDone(ucCurCmdID))
    {
    }
    
    return TRUE;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAESetValue
Description: 
    Set a memory area with a 32-bit data(that is, every DWORD in this memory 
  area will be set to the data).
    if enable DMAE,use DMAE to realize.
    otherwise,call COM_MemSet immediately.
Input Param: 
    U32 ulDesAddr: start address of the memory to set,it is an absolute address
    U32 ulBlockLenInByte: memory length in unit of byte, must be divisible by 16
    U32 ulValue: the data to set in memory.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function if it wants to initialize or clear a continuous memory 
  area. 
    When this function return, that means data setting has been completed.
    Can't be used for setting other MCU's SRAM.
History:  
20140912    Kristin    Coding style uniform
20141202    Kristin    1. add a input when call HAL_DMAESetValueLenLimit .
                       2. change the usage.
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAESetValue(const U32 ulDesAddr,const U32 ulLenByte, U32 ulValue)
{
#ifdef DMAE_ENABLE
    U32 ulCurSetLen;
    U32 ulCurDesAddr = ulDesAddr;
    U32 ulRemainLen = ulLenByte;

    while (ulRemainLen > 0)
    {
        if (ulRemainLen >= DMAE_ENTRY_MAX_LENGTH_IN_BYTE)
        {
            ulCurSetLen = DMAE_ENTRY_MAX_LENGTH_IN_BYTE;
        }
        else
        {
            ulCurSetLen = ulRemainLen;
        }

        while (FALSE == HAL_DMAESetValueLenLimit(ulCurDesAddr, ulCurSetLen, ulValue, (U8)HAL_GetMcuId()))
        {
        }

        ulRemainLen -= ulCurSetLen;
        ulCurDesAddr += ulCurSetLen;
    }
#else
    COM_MemSet((U32 *)ulDesAddr, ulLenByte/sizeof(U32), ulValue);
#endif

    return ;
}

/*---------------------------------------------------------------------------
Name: HAL_DMAESramSetValue
Description: 
    Set a SRAM area with a 32-bit data(that is, every DWORD in this memory 
  area will be set to the data) using DMAE.
Input Param: 
    U32 ulDesAddr: start address of the memory to set,it is an absolute address
    U32 ulBlockLenInByte: memory length in unit of byte, must be divisible by 16
    U32 ulValue: the data to set in SRAM.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function if it wants to initialize or clear a continuous memory 
  area. 
    When this function return, that means data setting has been completed.
History:  
20141202    Kristin    Create
------------------------------------------------------------------------------*/
void DMAE_TEXT_ATTR HAL_DMAESramSetValue(const U32 ulDesAddr,const U32 ulLenByte, U32 ulValue, U8 ucMCUID)
{
    U32 ulCurSetLen;
    U32 ulCurDesAddr = ulDesAddr;
    U32 ulRemainLen = ulLenByte;

    while (ulRemainLen > 0)
    {
        if (ulRemainLen >= DMAE_ENTRY_MAX_LENGTH_IN_BYTE)
        {
            ulCurSetLen = DMAE_ENTRY_MAX_LENGTH_IN_BYTE;
        }
        else
        {
            ulCurSetLen = ulRemainLen;
        }

        while (FALSE == HAL_DMAESetValueLenLimit(ulCurDesAddr, ulCurSetLen, ulValue, ucMCUID))
        {
        }

        ulRemainLen -= ulCurSetLen;
        ulCurDesAddr += ulCurSetLen;
    }

    return ;
}

/********************** FILE END ***************/

