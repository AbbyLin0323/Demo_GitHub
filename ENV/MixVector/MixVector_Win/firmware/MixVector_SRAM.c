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
* File Name    : MixVector_SRAM.c
* Discription  : interface for fetch host commnad and write back status;
                 common interface for read/write HCT SRAM
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#include "COM_Memory.h"
#include "BaseDef.h"
#include "HAL_HCT.h"
#include "MixVector_SRAM.h"
#include "HAL_HwDebug.h"
#ifdef SIM
#include "HostModel.h"
#endif
void InitHCTReg()
{
    PHCT_BAINC_REG  pbaInc;
    volatile HCT_CONTROL_REG *  pHctCtrlReg;
    volatile HCT_FCQ_REG*       pFcqReg;
    volatile HCT_WBQ_REG *      pWbqReg;

    pHctCtrlReg = (volatile HCT_CONTROL_REG *)&rHCT_CONTROL_REG;

    pHctCtrlReg->bsRSTALL = 1;
    HW_RESET_OPT;
    while( pHctCtrlReg->bsRSTALL == 1 )
    {
    }

    pbaInc = (PHCT_BAINC_REG)HCT_DSBA_REG_BASE;
    // to store command
    pbaInc[ 0 ].ulAsU32   = 0;
    pbaInc[ 0 ].usBA      = (U16)( HCT_HCMD_BASE - HCT_DSRAM_BASE_ADDRESS );
    pbaInc[ 0 ].bsINC     = sizeof( HOST_CMD );
    
    // to work as Disk A

    pbaInc[ 1 ].ulAsU32   = 0;
    pbaInc[ 1 ].usBA      = (U16)( HCT_DISK_A_BASE - HCT_DSRAM_BASE_ADDRESS );
    pbaInc[ 1 ].bsINC     = 1;

    pbaInc[ 2 ].ulAsU32   = 0;
    pbaInc[ 3 ].ulAsU32   = 0;
    pbaInc[ 4 ].ulAsU32   = 0;

    pFcqReg              = (volatile HCT_FCQ_REG *)&rHCT_FCQ_REG;
    pFcqReg->usFCQBA        = (U16)( HCT_FCQ_BASE - HCT_DSRAM_BASE_ADDRESS );

    pWbqReg               = (volatile HCT_WBQ_REG *)&rHCT_WBQ_REG;
    pWbqReg->usWBQBA        = (U16)( HCT_WBQ_BASE - HCT_DSRAM_BASE_ADDRESS );
    pWbqReg->bsWBQTRI       = CST_STATUS_TRIG_WBQ;
    pWbqReg->ucWBQINC       = sizeof(HCT_FCQ_WBQ) * WBQ_N;

    g_pHCmdTable = (HOST_CMD *)HCT_HCMD_BASE;
}
/*------------------------------------------------------------------------------
Name: FetchHostCmd
Description: 
    fetch host command arguments to local sram.
    FCQ should update CST to CST_STATUS_CMD_RCV after command completed.

Input Param:
    U8 ucCmdSlot: command slot index
Output Param:
    none
Return Value:
    BOOL: TRUE = build FCQ success(FCQ not full);
          FALSE = build FCQ fail(FCQ is full)
Usage:
    when FW check PxCI and find new command was sent by host,
    call this function to build a FCQ
    to fetch command argument.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL FetchHostCmd(U8 ucCmdSlot)
{
    volatile HCT_FCQ_WBQ *pFcq;
    HCT_FCQ_REG fcqReg;

    fcqReg =  rHCT_FCQ_REG;
    if ( 0 == fcqReg.bsFCQFULL )
    {
        pFcq                = (HCT_FCQ_WBQ *)(HCT_FCQ_BASE + fcqReg.bsFCQWP * sizeof( HCT_FCQ_WBQ) );
        COM_MemZero((U32 *)pFcq, sizeof(HCT_FCQ_WBQ) / sizeof(U32));

        pFcq->bsID            = ucCmdSlot;
        pFcq->bsIDB           = 1;
        pFcq->bsSN            = SN_COMMAND;
        pFcq->ulHostAddrHigh  = rP0_CLBU;
        pFcq->ulHostAddrLow   = rP0_CLB + ucCmdSlot * sizeof(HOST_CMD);
        pFcq->bsNST           = CST_STATUS_CMD_RCV;
        pFcq->bsCST           = CST_STATUS_INIT;
        pFcq->bsLength        = sizeof(HOST_CMD) / sizeof(U32);
        pFcq->bsUpdate        = 1;

        HAL_HwDebugTrace(ucCmdSlot, RCD_FCQ, (void *)pFcq, fcqReg.bsFCQWP, NULL);
        fcqReg.bsFCQPUSH = 1;
        FCQ_REG_WRITE_TRIGGER;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
BOOL ReturnDiskADataAndCompleteHostCmd(U8 ucCmdSlot, BOOL bWaitSge, BOOL bWaitNfc)
{
    U8 index;
    index = 0;
    if ( ( TRUE == g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].DiskEn )
        && ( HOST_REQ_READ == g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].ReqType ) )
    {
        SetupWBQForReadingDiskA( ucCmdSlot, index );
        g_pHCmdTable[ ucCmdSlot ].FinishCnt[ SUB_DISK_A ] = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].UnitLength;
        index++;
    }
    g_pHCmdTable[ ucCmdSlot ].FinishCnt[ SUB_DISK_C ] = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_C ].UnitLength;
    //DBG_Printf("FinishCnt[ SUB_DISK_C ]:0x%x\n",g_pHCmdTable[ ucCmdSlot ].FinishCnt[ SUB_DISK_C ]);
    CompleteHostCmd( ucCmdSlot, bWaitSge, bWaitNfc, index );

    return TRUE;
}
/*
 * Setup the WBQ specifed by ucWbqIndex to read disk A.
 * don't set CST to CST_STATUS_TRIG_WBQ.
 *
 * ucCmdSlot : indicates which command should be handled.
 * ucWbqIndex: indicates which WBQ entry should be set.
 *
 */
BOOL SetupWBQForReadingDiskA( U8 ucCmdSlot, U8 ucWbqIndex )
{
    PHCT_FCQ_WBQ   pWbq;

    pWbq = (PHCT_FCQ_WBQ)( HCT_WBQ_BASE + ( ucCmdSlot * WBQ_N + ucWbqIndex ) * sizeof( HCT_FCQ_WBQ ) );

    COM_MemZero( (U32*)pWbq, sizeof( HCT_FCQ_WBQ ) / sizeof (U32 ) );

    pWbq->bsSN            = SN_DISK_A;
    pWbq->bsID            = ucCmdSlot;
    pWbq->bsOffset        = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].StartUnit
                          + HCT_DISK_A_BASE - HCT_DSRAM_BASE_ADDRESS;
    pWbq->ulHostAddrLow   = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].HostAddrLow;
    pWbq->ulHostAddrHigh  = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].HostAddrHigh;

    pWbq->bsLast          = 0;
    pWbq->bsLength        = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].UnitLength / sizeof(U32);
    pWbq->bsWait          = 0;
    pWbq->bsWaitNfc       = 0;
    pWbq->bsCheck         = 0;    // don't check CST
    pWbq->bsUpdate        = 0;   // don't update

    HAL_HwDebugTrace(ucCmdSlot, RCD_WBQ, pWbq, ucWbqIndex, NULL);
    return TRUE;
}
/*--------------------  ----------------------------------------------------------
Name: CompleteHostCmd
Description: 
    build WBQ to return diskA data to host; clear CI and return status
Input Param:
    U8 ucCmdSlot: command slot index
    BOOL bWaitSge: TRUE = need to enable wait 'SGE data done' in WBQ
    BOOL bWaitNfc: TRUE = need to enable wait 'NFC program OK' in WBQ
    U8 ucWbqIndex: the slot for build this WBQ
Output Param:
    none
Return Value:
    BOOL: TRUE = build WBQ success; NOTE: in current design, return value always be TRUE
Usage:
    when command received, FCQ done, or cache status and SSU1 updated, call this
    function to return diskA data, clear CI and return status
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL CompleteHostCmd(U8 ucCmdSlot, BOOL bWaitSge, BOOL bWaitNfc, U8 ucWbqIndex )
{
    PHCT_FCQ_WBQ   pWbq;

    U32 ulOffsetReturnValue;
    
    pWbq = (PHCT_FCQ_WBQ)( HCT_WBQ_BASE + ( ucCmdSlot * WBQ_N + ucWbqIndex ) * sizeof( HCT_FCQ_WBQ ) );
    ulOffsetReturnValue = ucCmdSlot * sizeof( HOST_CMD ) + (U32)( ((HOST_CMD *)0)->FinishCnt );

    COM_MemZero( (U32*)pWbq, sizeof( HCT_FCQ_WBQ ) / sizeof (U32 ) );

    pWbq->bsID            = ucCmdSlot;
    pWbq->bsSN            = SN_COMMAND; // write back FinishCnt and ErrFlag
    pWbq->bsOffset        = ulOffsetReturnValue;
    pWbq->ulHostAddrHigh  = rP0_CLBU;
    pWbq->ulHostAddrLow   = rP0_CLB + ulOffsetReturnValue;

    pWbq->bsNST           = CST_STATUS_INIT;
    pWbq->bsCST           = 0;
    pWbq->bsClrCI         = 1;   // update PxCI
    pWbq->bsLast          = 1;
    pWbq->bsRegFirst      = 1; // update PxCI before CST because FW check CST before PxCI
    pWbq->bsLength        = SUB_DISK_CNT + 1;//return 4 DW (3 FinishCnt + 1 ErrFlag)
    pWbq->bsWait          = bWaitSge;
    pWbq->bsWaitNfc       = bWaitNfc;
    pWbq->bsUpdate        = 1;   // update the state of slot with NST.
    pWbq->bsCheck         = 0;    // don't check CST

    HAL_HwDebugTrace(ucCmdSlot, RCD_WBQ, pWbq, ucWbqIndex, NULL);
    SetCST( ucCmdSlot, CST_STATUS_TRIG_WBQ );

    return TRUE;
}

/*------------------------------------------------------------------------------
Name: ProcessWriteDiskA
Description: 
    for write diskA, build FCQ and enable update CST to CST_STATUS_DATA_DONE
Input Param:
    U8 ucCmdSlot: command slot index
Output Param:
    none
Return Value:
    BOOL: TRUE = build FCQ success(not full); FALSE = build FCQ  fail(full)
Usage:
    when FW find there is write request to diskA, call this fucntion to build FCQ.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL ProcessWriteDiskA(U8 ucCmdSlot)
{
    BOOL ret;
    HOST_MEM_REQ    hostMemReq;
    
    hostMemReq.ReqType      = HOST_REQ_WRITE;
    hostMemReq.HID          = ucCmdSlot;
    hostMemReq.UpdateCstEn  = 1;
    hostMemReq.NextCst      = CST_STATUS_DATA_DONE;
    hostMemReq.ClearCI      = 0;
    hostMemReq.WaitSge      = 0;
    hostMemReq.CmdSubId     = 0;
    hostMemReq.CacheStsEn   = 0;
    hostMemReq.Dw0Rsvd      = 0;
    hostMemReq.LBA          = 0;
    hostMemReq.ByteLength   = g_pHCmdTable[ ucCmdSlot].SubDiskCmd[ SUB_DISK_A ].UnitLength;
    hostMemReq.LocalAddr    = g_pHCmdTable[ ucCmdSlot].SubDiskCmd[ SUB_DISK_A ].StartUnit + HCT_DISK_A_BASE;
    hostMemReq.HostAddrLow  = g_pHCmdTable[ ucCmdSlot].SubDiskCmd[ SUB_DISK_A ].HostAddrLow;
    hostMemReq.HostAddrHigh = g_pHCmdTable[ ucCmdSlot].SubDiskCmd[ SUB_DISK_A ].HostAddrHigh;

    ret = FCQ_BuildHostWriteReq( &hostMemReq );
    if ( TRUE == ret )
    {
        g_pHCmdTable[ ucCmdSlot ].FinishCnt[ SUB_DISK_A ] = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_A ].UnitLength;
    }
    return ret;
}


/*------------------------------------------------------------------------------
Name: FCQ_BuildHostWriteReq
Description: 
    common interface for write HCT sram
Input Param:
    HOST_MEM_REQ *pHostReq: pointer to descriptor of memory request
Output Param:
    none
Return Value:
    BOOL: TRUE = build FCQ success(not full); FALSE = build FCQ  fail(full)
Usage:
    call this fucntion to process write HCT sram request.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL FCQ_BuildHostWriteReq(HOST_MEM_REQ *pHostReq)
{
    PHCT_FCQ_WBQ   pFcq;
    HCT_FCQ_REG fcqReg;

    fcqReg = rHCT_FCQ_REG;
    if ( 0 == fcqReg.bsFCQFULL )
    {
        pFcq                = (PHCT_FCQ_WBQ)(HCT_FCQ_BASE + fcqReg.bsFCQWP * sizeof( HCT_FCQ_WBQ) );
        COM_MemZero( (U32*)pFcq, sizeof( HCT_FCQ_WBQ ) / sizeof (U32 ) );

        pFcq->bsID            = pHostReq->HID;
        pFcq->bsSN            = SN_DISK_A;
        pFcq->bsOffset        = pHostReq->LocalAddr - HCT_DSRAM_BASE_ADDRESS;
        //DBG_Printf("Write Offset:0x%x\n",pFcq->Offset );
        pFcq->ulHostAddrHigh  = pHostReq->HostAddrHigh;
        pFcq->ulHostAddrLow   = pHostReq->HostAddrLow;
        pFcq->bsNST           = pHostReq->NextCst;
        pFcq->bsLength        = pHostReq->ByteLength / sizeof(U32);
        pFcq->bsCheck         = 0;
        pFcq->bsUpdate        = pHostReq->UpdateCstEn;

        HAL_HwDebugTrace(pHostReq->HID, RCD_FCQ, pFcq, fcqReg.bsFCQWP, NULL);
        fcqReg.bsFCQPUSH = 1;
        FCQ_REG_WRITE_TRIGGER;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*------------------------------------------------------------------------------
Name: WBQ_BuildHostReadReq
Description: 
    common interface for read HCT sram
Input Param:
    HOST_MEM_REQ *pHostReq: pointer to descriptor of memory request
Output Param:
    none
Return Value:
    void
Usage:
    call this fucntion to process read HCT sram request.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
void WBQ_BuildHostReadReq(HOST_MEM_REQ *pHostReq)
{
    //TODO
}
CST_STATUS GetCST( U8 ucCmdSlot )
{
    volatile U8 * ret;
    ret = ( (volatile U8 *)HCT_CS_REG_ADDRESS( ucCmdSlot ) );
    return (CST_STATUS)(*ret);
}
void SetCST( U8 ucCmdSlot, CST_STATUS Status )
{
    //SET_CMD_CST(ucCmdSlot, Status);
    HAL_HCTSetCST(ucCmdSlot,Status);
}
/* end of file MixVector_SRAM.c */

