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
* File Name    : MixVector_DRAM.c
* Discription  : common interface for read/write DRAM interface; cache status interface
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/

#include "BaseDef.h"
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "MixVector_DRAM.h"
#include "HAL_NormalDSG.h"
#include "HAL_HSG.h"
#include "HAL_SGE.h"

GLOBAL U8 *g_pCacheStatus;

/*------------------------------------------------------------------------------
Name: SetCacheStatusBusy
Description: 
    Set cache status when g_aHostCmdManager[ucCmdSlot].CacheStatusEn be set  
Input Param:
    U8 ucCmdSlot: command slot number
Output Param:
    none
Return Value:
    void
Usage:
    if manager enable cache status, FW call this function to set cachestatus of 
    target buffers in disk b
History:
    20131119    Victor   created
------------------------------------------------------------------------------*/

LOCAL void SetCacheStatusBusy(U8 ucCmdSlot)
{
    U32 i = 0 ;    
    U32 ulStartBufId = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit >> SEC_PER_BUF_BITS;
    U32 ulEndBufId = ((g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit)  
                        + g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].UnitLength - 1) 
                        >> SEC_PER_BUF_BITS;
    for (i=0;i<(ulEndBufId - ulStartBufId + 1);i++)
    {
        g_pCacheStatus[i + ulStartBufId] = CS_BUSY;
    }    
}


/*------------------------------------------------------------------------------
Name: CheckCacheStatusBusy
Description: 
    check if cache status is updated by SGE
Input Param:
    U8 ucCmdSlot: command slot number
Output Param:
    none
Return Value:
    BOOL: TRUE = CacheStatus was updated; FALSE = CacheStatus was not updated
Usage:
    if manager enable cache status, FW call this function to check all CacheStatus updated before
    build WBQ to complete host command.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
BOOL CheckCacheStatusBusy(U8 ucCmdSlot)
{
    U32 i = 0 ;    
    U32 ulStartBufId = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit >> SEC_PER_BUF_BITS;
    U32 ulEndBufId = ((g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit)
                        + g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].UnitLength - 1)
                        >> SEC_PER_BUF_BITS;
    for (i=0;i<(ulEndBufId - ulStartBufId + 1);i++)
    {
        if (CS_FREE != g_pCacheStatus[i + ulStartBufId])
        {
            return FALSE;
        }
    }
    return TRUE;
}
/****************************************************************************
Name        :DiskB_GetHsgLenth
Input       :ptHostReq,SplitEnable,ulRemSecLen,ucCmdSlot
Output      :TRUE : HSG end; FALSE : Still have HSG
Author      :
Date        :
Description :fit HSG lenth to tHostReq.
Others      :
Modify      :
****************************************************************************/
U32 DiskB_GetHsgLenth(HOST_MEM_REQ *pHostReq,U32 ucCmdSlot)
{
    U32 ulHSGPtr;
    U32 bRet;

    bRet = 0;

    ulHSGPtr = g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_B];

    bRet = g_pHCmdTable[ucCmdSlot].HsgLen[ulHSGPtr] * DISK_B_UNIT;
    ulHSGPtr ++;

    g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_B] = ulHSGPtr;

    return bRet;
}
/*------------------------------------------------------------------------------
Name: DiskB_BuildHsgChain
Description:
    bulid
Input Param:
    U8 ucCmdSlot: command slot index
Output Param:
    none
Return Value:
    BOOL: TRUE = build DRQ/DWQ success(not full); FALSE = build DRQ/DWQ fail(full)
Usage:
    when FW find there is any request to diskB, call this fucntion to build DRQ or DWQ.
    data size processed every time in this function  <= page size.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/

BOOL DiskB_BuildHsgChain(HOST_MEM_REQ *pHostReq,U16 *pFirstHsgId,BOOL bSplitEnable)
{
    U16 usCurHsgId = 0 , usNextHsgId = 0;
    HSG_ENTRY tHsgEntry = {0};
    HSG_ENTRY *pHsgEntry = NULL;
    U32 ulRemLenth;
    U16 usHSGLenth,print_hsg;
    U32 tHostAddrLow;
    if(TRUE == bSplitEnable)
    {
        ulRemLenth = pHostReq->ByteLength;
        print_hsg = 0;
        if(DiskB_GetHsgLenth(pHostReq,pHostReq->HID)==1)
            print_hsg = 1;
        while(0 != ulRemLenth)
        {
            if(INVALID_4F  == g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB)
            {
                if(FALSE == HAL_GetHsg(&usCurHsgId))
                {
                    return FALSE;
                }

            }
            else
            {
                usCurHsgId = g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB;
                g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB = INVALID_4F;
                HAL_TriggerHsg();
            }


            COM_MemZero((U32*)&tHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );
            //   Get a hsg entry
            tHsgEntry.ulHostAddrHigh = pHostReq->HostAddrHigh;
            tHsgEntry.ulHostAddrLow = pHostReq->HostAddrLow;
            //GetHSGLenth from Hcmd
            usHSGLenth = DiskB_GetHsgLenth(pHostReq,pHostReq->HID);

            if((usHSGLenth >= ulRemLenth) || (0 == usHSGLenth))
            {
                usHSGLenth = ulRemLenth;
                tHsgEntry.bsLast = TRUE;

            }
            tHsgEntry.bsLength = usHSGLenth;

            if(FALSE == HAL_GetCurHsg(&usNextHsgId))
            {
                g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB = usCurHsgId;
                return FALSE;
            }

            tHsgEntry.bsNextHsgId = usNextHsgId;

            if(FALSE == g_aHostCmdManager[pHostReq->HID].bDiskBFirstHSGGet)
            {
                // Output HSG ID
                *pFirstHsgId = usCurHsgId;
                g_aHostCmdManager[pHostReq->HID].bDiskBFirstHSGGet = TRUE;
            }

            ulRemLenth -= tHsgEntry.bsLength;
            pHostReq->HostAddrLow += tHsgEntry.bsLength;

            g_aHostCmdManager[pHostReq->HID].FinishCnt[SUB_DISK_B] += tHsgEntry.bsLength;

            if(g_aHostCmdManager[pHostReq->HID].FinishCnt[SUB_DISK_B] >=
                g_pHCmdTable[ pHostReq->HID ].SubDiskCmd[ SUB_DISK_B ].UnitLength << SEC_SIZE_BITS)
            {
                tHsgEntry.bsLast = TRUE;
                ulRemLenth = 0;
            }
            tHostAddrLow = g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow;

            g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow += tHsgEntry.bsLength;
            if(g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow < tHostAddrLow)
            {
                g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrHigh ++;
            }

            g_aHostCmdManager[pHostReq->HID ].StartUnitInByte[SUB_DISK_B] += tHsgEntry.bsLength;
            //if tHsgEntry.Length < SEC_SZ ???
            g_aHostCmdManager[pHostReq->HID ].UnitLenthInByte[SUB_DISK_B] -= tHsgEntry.bsLength;

            pHsgEntry = (HSG_ENTRY*)HAL_GetHsgAddr(usCurHsgId);
            COM_MemZero((U32*)pHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );
            COM_MemCpy((U32*)pHsgEntry,(U32*)&tHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );
            if(print_hsg == 1)
            DBG_Printf("Hid:%d Length :0x%x HostAddrLow:0x%x\n",pHostReq->HID,pHsgEntry->bsLength,pHsgEntry->ulHostAddrLow);
            HAL_SetHsgSts(usCurHsgId,TRUE);


        }
    }
    else
    {
        if(INVALID_4F  == g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB)
        {
            if(FALSE == HAL_GetHsg(&usCurHsgId))
            {
                return FALSE;
            }

        }
        else
        {
            usCurHsgId = g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB;
            g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB = INVALID_4F;
            HAL_TriggerHsg();
        }


        COM_MemZero((U32*)&tHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );
        if(FALSE == g_aHostCmdManager[pHostReq->HID].bDiskBFirstHSGGet)
        {
            // Output HSG ID
            *pFirstHsgId = usCurHsgId;
            g_aHostCmdManager[pHostReq->HID].bDiskBFirstHSGGet = TRUE;
        }


        //   Get a hsg entry
        tHsgEntry.ulHostAddrHigh = pHostReq->HostAddrHigh;
        tHsgEntry.ulHostAddrLow = pHostReq->HostAddrLow;
        tHsgEntry.bsLength = pHostReq->ByteLength;
        //tHsgEntry.LBA = pHostReq->LBA;
        tHsgEntry.bsLast = TRUE;

        if(FALSE == HAL_GetCurHsg(&usNextHsgId))
        {
            g_aHostCmdManager[pHostReq->HID].ContinueHSGDiskB = usCurHsgId;
            return FALSE;
        }


        tHsgEntry.bsNextHsgId = usNextHsgId;

        //   config  hsg
        pHsgEntry = (HSG_ENTRY*)HAL_GetHsgAddr(usCurHsgId);
        COM_MemZero((U32*)pHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );
        COM_MemCpy((U32*)pHsgEntry,(U32*)&tHsgEntry, ( sizeof(HSG_ENTRY) / sizeof( U32 ) ) );

        pHostReq->HostAddrLow += tHsgEntry.bsLength;
        g_aHostCmdManager[pHostReq->HID].FinishCnt[SUB_DISK_B] += tHsgEntry.bsLength;

        tHostAddrLow = g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow;

        g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow += tHsgEntry.bsLength;
        if(g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrLow < tHostAddrLow)
        {
            g_aHostCmdManager[ pHostReq->HID ].SubDiskBCmd.HostAddrHigh ++;
        }

        g_aHostCmdManager[pHostReq->HID ].StartUnitInByte[SUB_DISK_B] += tHsgEntry.bsLength;
        //if tHsgEntry.Length < SEC_SZ ???
        g_aHostCmdManager[pHostReq->HID ].UnitLenthInByte[SUB_DISK_B] -= tHsgEntry.bsLength;


        HAL_SetHsgSts(usCurHsgId,TRUE);
    }

    return TRUE;
}

/*------------------------------------------------------------------------------
Name: DiskB_BuildDsgChain
Description:
    Build Dsg chain , in this version , just build one DSG entry
Input Param:
    HOST_MEM_REQ *pHostReq   -- Host memory request
Output Param:
    U16 **pFirstDsgId  -- point to first DSG ID
Return Value:
    BOOL:   TRUE  --- build DSG successfully
            FALSE --- no free DSG
Usage:
    Build DSG entry according host request
History:
    20131119    Victor   created
------------------------------------------------------------------------------*/

BOOL DiskB_BuildDsgChain(HOST_MEM_REQ *pHostReq,U16 *pFirstDsgId,BOOL bSplitEnable)
{
    U16 usCurDsgId = 0, usNextDsgId = 0;
    NORMAL_DSG_ENTRY tDsgEntry = {0};
    NORMAL_DSG_ENTRY *pDsgEntry = NULL;

    if (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        return FALSE;
    }
    *pFirstDsgId = usCurDsgId;
    tDsgEntry.ulDramAddr = pHostReq->LocalAddr - DRAM_START_ADDRESS;
    //tDsgEntry.XferByteLen= pHostReq->ByteLength;
    tDsgEntry.bsXferByteLen= pHostReq->DSGByteLength;
    if (TRUE == pHostReq->CacheStsEn)
    {
        tDsgEntry.bsCacheStsEn = TRUE;
        tDsgEntry.bsCacheStatusAddr = (U32)(g_pCacheStatus + (pHostReq->LBA >> SEC_PER_BUF_BITS))
                                    - OTFB_START_ADDRESS;
        tDsgEntry.bsCacheStsData = CS_FREE;
    }

    tDsgEntry.bsLast = TRUE;

    if (FALSE == HAL_GetCurNormalDsg(&usNextDsgId))
    {
        //DBG_Getch();
        return FALSE;
    }

    pHostReq->LocalAddr += tDsgEntry.bsXferByteLen;
    tDsgEntry.bsNextDsgId = usNextDsgId;
    pDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDsgEntry, ( sizeof(NORMAL_DSG_ENTRY) / sizeof( U32 ) ) );
    COM_MemCpy((U32*)pDsgEntry,(U32*)&tDsgEntry, ( sizeof(NORMAL_DSG_ENTRY) / sizeof( U32 ) ) );
    g_pHCmdTable[pHostReq->HID].SubDiskCmd[SUB_DISK_B].StartUnit += (tDsgEntry.bsXferByteLen/SEC_SIZE);

    HAL_SetNormalDsgSts(usCurDsgId,TRUE);
    return TRUE;

}

/*------------------------------------------------------------------------------
Name: DWQ_BuildHostWriteReq
Description:
    Call it build a host write request
Input Param:
    HOST_MEM_REQ *pHostReq   -- Host memory request
Output Param:
    none
Return Value:
    BOOL:   TRUE  --- build DWQ successfully
            FALSE --- failed
Usage:
    1.check DWQ full
    2.build hsg
    3.build dsg
    4.build dwq
History:
    20131119    Victor   created
------------------------------------------------------------------------------*/

BOOL DWQ_BuildHostWriteReq(HOST_MEM_REQ *pHostReq,BOOL bSplitEnable)
{
    U16 usFirstDsgId = 0;
    U16 * FirstHSGDiskB;
    FirstHSGDiskB = &g_aHostCmdManager[pHostReq->HID].FirstHSGDiskB;
    if (TRUE == HAL_DwqIsFull())
    {
        return FALSE;
    }

    if (FALSE == DiskB_BuildHsgChain(pHostReq,FirstHSGDiskB,bSplitEnable))
    {
        return FALSE;
    }

    if (FALSE == DiskB_BuildDsgChain(pHostReq,&usFirstDsgId,bSplitEnable))
    {
        return FALSE;
    }

    if (FALSE == HAL_DwqBuildEntry(pHostReq->HID,*FirstHSGDiskB,usFirstDsgId))
    {
        return FALSE;
    }

    return TRUE;
}

/*------------------------------------------------------------------------------
Name: DWQ_BuildHostReadReq
Description:
    Call it build a host read request
Input Param:
    HOST_MEM_REQ *pHostReq   -- Host memory request
Output Param:
    none
Return Value:
    BOOL:   TRUE  --- build DRQ successfully
            FALSE --- failed
Usage:
    1.check DRQ full
    2.build hsg
    3.build dsg
    4.build drq
History:
    20131119    Victor   created
------------------------------------------------------------------------------*/

BOOL DRQ_BuildHostReadReq(HOST_MEM_REQ *pHostReq,BOOL bSplitEnable)     ///  return TRUE  for building drq successfully ,FALSE for failed
{
    U16 usFirstDsgId = 0;
    U16 * FirstHSGDiskB;
    FirstHSGDiskB = &g_aHostCmdManager[pHostReq->HID].FirstHSGDiskB;

    if (TRUE == HAL_DrqIsFull())
    {
        return FALSE;
    }

    if (FALSE == DiskB_BuildHsgChain(pHostReq,FirstHSGDiskB,bSplitEnable))
    {
        return FALSE;
    }

    if (FALSE == DiskB_BuildDsgChain(pHostReq,&usFirstDsgId,bSplitEnable))
    {
        return FALSE;
    }

    if (FALSE == HAL_DrqBuildEntry(pHostReq->HID,*FirstHSGDiskB,usFirstDsgId))
    {
        return FALSE;
    }

    return TRUE;
}
/*------------------------------------------------------------------------------
Name: ProcessDiskB
Description:
    procees host read/write request from/to diskB. for read, build DRQ; for write, build DWQ
Input Param:
    U8 ucCmdSlot: command slot index
Output Param:
    none
Return Value:
    BOOL: TRUE = build DRQ/DWQ success(not full); FALSE = build DRQ/DWQ fail(full)
Usage:
    when FW find there is any request to diskB, call this fucntion to build DRQ or DWQ.
    data size processed every time in this function  <= page size.
History:
    20131105    Gavin   created
------------------------------------------------------------------------------*/
#if 0
BOOL ProcessDiskB(U8 ucCmdSlot)
{
    BOOL bRet = FALSE;
    HOST_MEM_REQ tHostReq ={0};
    U32 ulRemSecLen = 0;
    U32 ulStartSec = 0;
    U32 ulOffInByte;
    BOOL bSplitEnable = FALSE;
    if (FALSE == g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].DiskEn)
    {
        return TRUE;
    }
    //if ( g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_B] > 0 )
    //{
    //    DBG_Break();
    //}
    if(ucCmdSlot==5)
        DBG_Printf("\n");
    g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_B] = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_B ].UnitLength << SEC_SIZE_BITS;

    tHostReq.HID = ucCmdSlot;
    tHostReq.HostAddrHigh = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].HostAddrHigh;
    tHostReq.HostAddrLow = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].HostAddrLow;
    tHostReq.LocalAddr = (g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit << SEC_SIZE_BITS) + DISK_B_DRAM_BASE;  /// base address??
    tHostReq.ReqType = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].ReqType;

    tHostReq.LBA = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit;


    ulRemSecLen = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].UnitLength << SEC_SIZE_BITS;
    ulOffInByte = ((g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit) % SEC_PER_BUF ) << SEC_SIZE_BITS ;
    ulStartSec = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit;
    tHostReq.CacheStsEn = g_aHostCmdManager[ucCmdSlot].CacheStatusEn;
    bSplitEnable = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].SplitEnable;

    if (TRUE == tHostReq.CacheStsEn)
    {
        SetCacheStatusBusy(ucCmdSlot);
    }
    while (0 != ulRemSecLen)
    {
        //if ( (ulOffInByte + ulRemSecLen) > BUF_SIZE)
        if(ulOffInByte!=0)
        {
            //tHostReq.ByteLength = BUF_SIZE - ((tHostReq.LBA % SEC_PER_BUF) << SEC_SIZE_BITS);
            tHostReq.ByteLength = (BUFF_SIZE>ulRemSecLen?(ulRemSecLen):(BUFF_SIZE-ulOffInByte));

        }
        else
        {
            tHostReq.ByteLength = ulRemSecLen ;
        }
        switch (tHostReq.ReqType)
        {
            case HOST_REQ_WRITE:
                bRet = DWQ_BuildHostWriteReq(&tHostReq,bSplitEnable);
                break;
            case HOST_REQ_READ:
                bRet = DRQ_BuildHostReadReq(&tHostReq,bSplitEnable);
                break;
            default:
                DBG_Getch();
        }

        /* if fw failed to build drq/dwq entry ,just try last entry again.*/
        if (FALSE == bRet)
        {
            return FALSE;
            //break;
        }

        // by Charles Zhou
        // remove the following instruction to Pre
        g_aHostCmdManager[ucCmdSlot].FinishCnt[SUB_DISK_B] += tHostReq.ByteLength;

        tHostReq.CmdSubId++;
        ulRemSecLen -= tHostReq.ByteLength;
        tHostReq.HostAddrLow += tHostReq.ByteLength;
        tHostReq.LocalAddr += tHostReq.ByteLength;
        tHostReq.LBA += tHostReq.ByteLength >> SEC_SIZE_BITS;
        g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_B] = 0;
    }

    return TRUE;
}
#endif
BOOL ProcessDiskB(U8 ucCmdSlot)
{
    BOOL bRet = FALSE;
    HOST_MEM_REQ tHostReq ={0};
    U32 ulRemSecLen = 0;

    U32 ulOffInByte,ulOffInByteForDSG;
    BOOL bSplitEnable = FALSE;

    if (FALSE == g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.DiskEn)
    {
        return TRUE;
    }
    if(g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.UnitLength == 0x0)
    {
        DBG_Printf("slot:%d unitlen:%d\n",ucCmdSlot,g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.UnitLength);
    }
    if((0x5==ucCmdSlot)&&(HOST_REQ_READ==g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.ReqType)&&
        (g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.UnitLength == 0x00000304))
    {
        DBG_Printf("g_pHCmdTable[ucCmdSlot] 0x%x\n",&g_pHCmdTable[ucCmdSlot]);
        DBG_Printf("g_aHostCmdManager[ucCmdSlot] 0x%x\n",&g_aHostCmdManager[ucCmdSlot]);

    }

    g_pHCmdTable[ucCmdSlot].FinishCnt[SUB_DISK_B] = g_pHCmdTable[ ucCmdSlot ].SubDiskCmd[ SUB_DISK_B ].UnitLength << SEC_SIZE_BITS;

    tHostReq.HID = ucCmdSlot;
    tHostReq.HostAddrHigh = g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.HostAddrHigh;
    tHostReq.HostAddrLow = g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.HostAddrLow;
    tHostReq.LocalAddr = (g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit << SEC_SIZE_BITS) + DISK_B_DRAM_BASE;  /// base address??
    tHostReq.ReqType = g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.ReqType;

    tHostReq.LBA = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit;


    ulRemSecLen = g_aHostCmdManager[ucCmdSlot].UnitLenthInByte[SUB_DISK_B];


    tHostReq.CacheStsEn = g_aHostCmdManager[ucCmdSlot].CacheStatusEn;
    bSplitEnable = g_aHostCmdManager[ucCmdSlot].SubDiskBCmd.SplitEnable;


    if (TRUE == tHostReq.CacheStsEn)
    {
        SetCacheStatusBusy(ucCmdSlot);
    }
    while (0 != ulRemSecLen)
    {
        ulOffInByte = g_aHostCmdManager[ucCmdSlot].StartUnitInByte[SUB_DISK_B] % BUF_SIZE;
        tHostReq.ByteLength = ((BUF_SIZE-ulOffInByte)>ulRemSecLen?(ulRemSecLen):(BUF_SIZE-ulOffInByte));

        ulOffInByteForDSG = ((g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit) % SEC_PER_BUF ) << SEC_SIZE_BITS ;

        if(ulOffInByte != ulOffInByteForDSG)
        {
            tHostReq.DSGByteLength = tHostReq.ByteLength + ulOffInByte - ulOffInByteForDSG;
        }
        else
        {
            tHostReq.DSGByteLength = ((BUF_SIZE-ulOffInByteForDSG)>ulRemSecLen?(ulRemSecLen):(BUF_SIZE-ulOffInByteForDSG));
        }


        tHostReq.LBA = g_pHCmdTable[ucCmdSlot].SubDiskCmd[SUB_DISK_B].StartUnit;

        switch (tHostReq.ReqType)
        {
            case HOST_REQ_WRITE:
                bRet = DWQ_BuildHostWriteReq(&tHostReq,bSplitEnable);
                break;
            case HOST_REQ_READ:
                bRet = DRQ_BuildHostReadReq(&tHostReq,bSplitEnable);
                break;
            default:
                DBG_Getch();
        }

        /* if fw failed to build drq/dwq entry ,just try last entry again.*/
        if (FALSE == bRet)
        {
            return FALSE;
            //break;
        }

        // by Charles Zhou
        // remove the following instruction to Pre
        //g_aHostCmdManager[ucCmdSlot].FinishCnt[SUB_DISK_B] += tHostReq.ByteLength;

        tHostReq.CmdSubId++;
        ulRemSecLen -= tHostReq.ByteLength;

        g_aHostCmdManager[ucCmdSlot].HSGPtr[SUB_DISK_B] = 0;
        g_aHostCmdManager[ucCmdSlot].bDiskBFirstHSGGet = FALSE;
    }

    return TRUE;
}
/* end of file MixVector_DRAM.c */

