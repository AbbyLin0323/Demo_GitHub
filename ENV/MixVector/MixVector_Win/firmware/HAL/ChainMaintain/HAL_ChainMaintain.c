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
Filename    : HAL_ChainMaintain.c
Version     : Ver 1.0
Author      : Gavin
Date        : 20140611
Description : maintain total number of SGE chain which was built by L1/L2/L3
Others      :
Modify      :
20140611     gavinyin     001 create file
20140909     gavinyin     002 modify to meet coding stytle
*******************************************************************************/
#include "BaseDef.h"
#include "COM_QWord.h"
#include "COM_Memory.h"
#include "HAL_HSG.h"
#include "HAL_HostInterface.h"
#include "HAL_ChainMaintain.h"

LOCAL MCU12_VAR_ATTR U16 l_aChainNumRcd[MAX_SLOT_NUM];

/* manager to record host request length and firmwarw processed length */
LOCAL MCU12_VAR_ATTR CHAIN_NUM_MGR l_aChainNumMgr[MAX_SLOT_NUM];

extern void HAL_SgeFinishChainCnt(U8 HID, U16 TotalChain);

/*------------------------------------------------------------------------------
Name: HAL_ChainMaintainInit
Description: 
    init chain num manager and record.
Note:
    In Xtensa ENV, global/static variable should be initilized in run-time.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in boot stage;
History:
    20140716    Gavin   created
------------------------------------------------------------------------------*/
void HAL_ChainMaintainInit(void)
{
    COM_MemZero((U32 *)&l_aChainNumRcd[0], sizeof(l_aChainNumRcd)/sizeof(U32));
    COM_MemZero((U32 *)&l_aChainNumMgr[0], sizeof(l_aChainNumMgr)/sizeof(U32));
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_L1AddHostReqLength
Description: 
    add new request length to chain num manager.
Input Param:
    U8 ucHID: host comamnd id(NCQ tag)
    BOOL bFirst: TRUE means the request is the first one
    BOOL bLast: TRUE means the request is the last one
    U16 usSecCnt: sector length to add
Output Param:
    none
Return Value:
    void
Usage:
    when L1 received a new sub command from L0, it calls this function to add host request
    length
History:
    20131031    Gavin   created
------------------------------------------------------------------------------*/
void HAL_L1AddHostReqLength(U8 ucHID, BOOL bFirst, BOOL bLast, U16 usSecCnt)
{
    CHAIN_NUM_MGR *pChainNumMgr = &l_aChainNumMgr[ucHID];

    if (TRUE == bFirst)
    {
        pChainNumMgr->bsTotalSecCntValid = FALSE;
        pChainNumMgr->usTotalSecCnt = 0;
        pChainNumMgr->usFinishSecCnt = 0;
        pChainNumMgr->usFinishChainCnt = 0;

        //debug mode code
        if (TRUE == pChainNumMgr->bsStartProcess)
        {
            DBG_Printf("HAL_L1AddHostReqLength ucHID 0x%x StartProcess TRUE ERROR\n", ucHID);
            DBG_Getch();
        }
        pChainNumMgr->bsStartProcess = TRUE;
    }

    //debug mode code
    if (FALSE == pChainNumMgr->bsStartProcess)
    {
        DBG_Printf("HAL_L1AddHostReqLength ucHID 0x%x StartProcess FALSE ERROR\n", ucHID);
        DBG_Getch();
    }

    // add total cnt
    pChainNumMgr->usTotalSecCnt += usSecCnt;
    
    if (TRUE == bLast)
    {
        pChainNumMgr->bsTotalSecCntValid = TRUE;
    }
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_AddFinishReqLength
Description: 
    add processed request length to chain num manager.
Input Param:
    U8 ucHID: host comamnd id(NCQ tag)
    U16 usSecCnt: sector length to add
Output Param:
    none
Return Value:
    U16: 0: means some request have not been processed by FW.
            others: means all request data have been processed by FW, the return value indicates
            total chain num that has been built to SGE
Usage:
    after build a DRQ/DWQ/SGQ, FW should call this function to check host request finish or not.
History:
    20131031    Gavin   created
------------------------------------------------------------------------------*/
void HAL_AddFinishReqLength(U8 ucHID, U16 usSecCnt)
{
    CHAIN_NUM_MGR *pChainNumMgr = &l_aChainNumMgr[ucHID];

    //debug mode code
    if (FALSE == pChainNumMgr->bsStartProcess)
    {
        DBG_Printf("HAL_AddFinishReqLength ucHID 0x%x StartProcess ERROR\n", ucHID);
        DBG_Getch();
    }

    // add finish sector cnt and chain num
    pChainNumMgr->usFinishSecCnt += usSecCnt;
    pChainNumMgr->usFinishChainCnt++;

    //wait all request received and processed 
    if (TRUE == pChainNumMgr->bsTotalSecCntValid
        && pChainNumMgr->usTotalSecCnt == pChainNumMgr->usFinishSecCnt)
    {
        //debug mode code
        pChainNumMgr->bsStartProcess = FALSE;

        /*set total chain num*/
        HAL_SgeFinishChainCnt(ucHID, pChainNumMgr->usFinishChainCnt);
    }

    return;
}

#ifdef HOST_AHCI
/*------------------------------------------------------------------------------
Name: HAL_GetLocalPrdEntryAddr
Description: 
    Get PRD address according command tag and PRD index.
Input Param:
    U8 ucCmdTag: command tag
    U16 usPrdIndex: PRD index in PRDT of this tag
Output Param:
    none
Return Value:
    void
Usage:
    when build HSG according to PRD, FW need call this function to get PRD's address
History:
    20140611    Gavin   created
------------------------------------------------------------------------------*/
U32 HAL_GetLocalPrdEntryAddr(U8 ucCmdTag, U16 usPrdIndex)
{
    return GET_PRD_ADDR(ucCmdTag, usPrdIndex);
}
#endif// #ifdef HOST_AHCI

#ifdef HOST_NVME
/*------------------------------------------------------------------------------
Name: HAL_GetLocalPrpEntryAddr
Description: 
    Get PRP address according command tag and PRP index.
Input Param:
    U8 ucCmdTag: command tag of host command
    U16 usPrpIndex: the PRP index. Note: L0 prepared all PRPs in continuous SRAM, no
                    matter the PRP is from command entry or PRP page
Output Param:
    none
Return Value:
    void
Usage:
    when build HSG according to PRP, FW need call this function to get PRP's address
History:
    20141110    Nina   created
    20141114    Gavin  remove PRPList support, because L0 cover it
------------------------------------------------------------------------------*/
U32 HAL_GetLocalPrpEntryAddr(U8 ucCmdTag, U16 usPrpIndex)
{
    return GET_PRP_ADDR(ucCmdTag, usPrpIndex);
}
#endif //#ifdef HOST_NVME

#ifndef VT3514_C0
/*------------------------------------------------------------------------------
Name: HAL_HostAddrMeetHwLimit
Description: 
    In VT3514 B0 HW design, there is limitation on On-the-fly data path: the host
    address and transfer length in HSG must be 8-byte align. This function check
    if host memory meet the limitation or not.
    For VT3514 A0, the on-the-fly path are not supported.
    For VT3514 C0, HW plan to fix the limitation, so C0 FW should not call this function.
Input Param:
    HMEM_DPTR *pHmemDptr: pointer to host memory descriptor
    U32 ulByteLen: byte length for the check
Output Param:
    none
Return Value:
    BOOL: TRUE = meet HW limitation; FALSE = not meet.
Usage:
    Before enable one sub command go through on-the-fly, FW call this function.
History:
    20140818    Gavin   created
------------------------------------------------------------------------------*/
BOOL HAL_HostAddrMeetHwLimit(HMEM_DPTR *pHmemDptr, U32 ulByteLen)
{
    U32 ulPrdtAddr;
    PRD tPrdt;
    QWORD tWordIn;
    QWORD tStartHostAddr;
    U32 ulLenInHsg;
    U32 ulLenInPrd;
    U32 ulLeftByteLen = ulByteLen;
    U32 ulPrdIndex = pHmemDptr->bsPrdOrPrpIndex;

    do
    {
        /* we only take account of AHCI mode, because NVMe mode is supported on C0 design only,
        and C0 HW has no address limitation, so C0 FW will not come here */
        ulPrdtAddr = HAL_GetLocalPrdEntryAddr(pHmemDptr->bsCmdTag, ulPrdIndex);

        COM_MemCpy((U32 *)&tPrdt, (U32 *)ulPrdtAddr, sizeof(PRD)/sizeof(U32));
        
        if (ulLeftByteLen == ulByteLen)
        {            
            tWordIn.HighDw = tPrdt.DBAHi;
            tWordIn.LowDw = tPrdt.DBALo;
            COM_QwAddDw(&tWordIn, pHmemDptr->bsOffset, &tStartHostAddr);
            ulLenInPrd = (tPrdt.DBC + 1) - pHmemDptr->bsOffset;
        }
        else
        {
            tStartHostAddr.HighDw = tPrdt.DBAHi;
            tStartHostAddr.LowDw = tPrdt.DBALo;
            ulLenInPrd = (tPrdt.DBC + 1);
        }

        ulLenInHsg = (ulLenInPrd > ulLeftByteLen) ? ulLeftByteLen : ulLenInPrd;

        if ((0 != (tStartHostAddr.LowDw & SEC_SIZE_MSK)) || (0 != (ulLenInHsg & SEC_SIZE_MSK)))
        {
            return FALSE;
        }
        else
        {
            ulPrdIndex++;
            ulLeftByteLen -= ulLenInHsg;
        }
    }while (0 != ulLeftByteLen);

    return TRUE;
}
#endif// #ifndef VT3514_C0

/*------------------------------------------------------------------------------
Name: HAL_BuildHsg
Description: 
    build a HSG entry according host memory descriptor.
Input Param:
    HMEM_DPTR *pHmemDptr: pointer to host memory descriptor
    U32* pByteLen: pointer of length which desires to build HSG.
    U16 usCurHsgId: Hsg id to build
    U16 usNextHsgId: next HSG to use( 0xFFFF means CurHsgId is the last in HSG chain )
Output Param:
    HMEM_DPTR *pHmemDptr: updated host memory descriptor( host memory descriptor will move ahead )
    U32* pByteLen: left lenth which still need to build HSG.
Return Value:
    void
Usage:
    when L1/L2/L3 need to build one HSG entry, FW call this function
History:
    20131014    Gavin   created
    20131117    Gavin   add support for NVMe mode
    20141212    Nina    modify for NVMe mode
                        (1) AHCI:pHmemDptr->bsOffset means Host memory addr offset
                        (2) NVMe:pHmemDptr->bsOffset means Host memory addr 4K align offset                        
------------------------------------------------------------------------------*/
void HAL_BuildHsg(HMEM_DPTR *pHmemDptr, U32* pByteLen, U16 usCurHsgId, U16 usNextHsgId)
{
    U32 ulHsgAddr;
    HSG_ENTRY * pHsgEntry;
    U32 ulRemLenInEntry;
    U32 ulHostReqRemainLen;
    QWORD tWordIn;
    QWORD tWordOut;
#ifdef HOST_AHCI
    volatile PRD *pPrd;
    pPrd = (volatile PRD *)HAL_GetLocalPrdEntryAddr(pHmemDptr->bsCmdTag, pHmemDptr->bsPrdOrPrpIndex);
    ulRemLenInEntry = (pPrd->DBC + 1) - pHmemDptr->bsOffset;
    tWordIn.HighDw = pPrd->DBAHi;
    tWordIn.LowDw = pPrd->DBALo;
#else // NVMe mode
    volatile QWORD *pHostMemAddr;
    pHostMemAddr = (volatile QWORD *)HAL_GetLocalPrpEntryAddr(pHmemDptr->bsCmdTag, pHmemDptr->bsPrdOrPrpIndex);
    ulRemLenInEntry = HPAGE_SIZE - pHmemDptr->bsOffset;
    tWordIn.HighDw = pHostMemAddr->HighDw;
    tWordIn.LowDw = pHostMemAddr->LowDw & (~HPAGE_SIZE_MSK);
#endif

    ulHostReqRemainLen = *pByteLen;
    COM_QwAddDw(&tWordIn, pHmemDptr->bsOffset, &tWordOut);

    //fill HSG
    ulHsgAddr = HAL_GetHsgAddr(usCurHsgId);
    pHsgEntry = (HSG_ENTRY *)ulHsgAddr;

    pHsgEntry->ulHostAddrHigh = tWordOut.HighDw;
    pHsgEntry->ulHostAddrLow = tWordOut.LowDw;
    pHsgEntry->bsNextHsgId = usNextHsgId;

    if(INVALID_4F == usNextHsgId)
    {
        pHsgEntry->bsLast = TRUE;
    }
    else
    {
        pHsgEntry->bsLast = FALSE;
    }

    if(ulHostReqRemainLen > ulRemLenInEntry)
    {
        pHsgEntry->bsLength = ulRemLenInEntry;  
        pHmemDptr->bsOffset = 0;
        pHmemDptr->bsPrdOrPrpIndex++;
    }
    else
    {
        pHsgEntry->bsLength = ulHostReqRemainLen;    
        pHmemDptr->bsOffset += ulHostReqRemainLen;
#ifdef HOST_NVME
        if(pHmemDptr->bsOffset >= HPAGE_SIZE)
        {
            pHmemDptr->bsOffset -= HPAGE_SIZE;
            pHmemDptr->bsPrdOrPrpIndex++;
        }
#endif
    }

    //return remain length
    ulHostReqRemainLen -= pHsgEntry->bsLength;
    *pByteLen = ulHostReqRemainLen;

    //update pHmemDptr
    if(0 != pHmemDptr->bsLastSecRemain)
    {
        if( ( pHsgEntry->bsLength - pHmemDptr->bsLastSecRemain) > 0)
        {
            pHsgEntry->ulLBA = pHmemDptr->ulLBA + 1;
            if(( pHsgEntry->bsLength - pHmemDptr->bsLastSecRemain)/SEC_SIZE )
            {
                pHmemDptr->ulLBA++;
            }     
        }
    }
    else
    {
        pHsgEntry->ulLBA = pHmemDptr->ulLBA;
    }

    if(0 != pHmemDptr->bsLastSecRemain)
    {
        pHmemDptr->bsLastSecRemain = SEC_SIZE - (pHsgEntry->bsLength - pHmemDptr->bsLastSecRemain)%SEC_SIZE;
    }
    else
    {
        pHmemDptr->bsLastSecRemain = SEC_SIZE - pHsgEntry->bsLength%SEC_SIZE;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_CheckForLastHsg
Description: 
    check if the next HSG will be the last or not.
Input Param:
    HMEM_DPTR *pHmemDptr: pointer to host memory descriptor
    U32 ulByteLenRemain: byte length need to build
Output Param:
    none
Return Value:
    BOOL: TRUE: byte length need to build only need one HSG. FALSE = need at least 2 HSGs
Usage:
    as finishing a request need several HSG, FW need to check if one HSG is enough to finish remaining byte
History:
    20131014    Gavin   created
    20141117    Gavin   Add support for NVMe mode
    20141212    Nina    modify for NVMe mode
                        (1) AHCI:pHmemDptr->bsOffset means Host memory addr offset
                        (2) NVMe:pHmemDptr->bsOffset means Host memory addr 4K align offset                        
------------------------------------------------------------------------------*/
BOOL HAL_CheckForLastHsg(HMEM_DPTR *pHmemDptr, U32 ulByteLenRemain)
{
    BOOL bLastFlag = FALSE;
    U32 ulRemLenInEntry;
#ifdef HOST_AHCI
    volatile PRD *pPrd;
    pPrd = (volatile PRD *)HAL_GetLocalPrdEntryAddr(pHmemDptr->bsCmdTag, pHmemDptr->bsPrdOrPrpIndex);
    ulRemLenInEntry = pPrd->DBC + 1 - pHmemDptr->bsOffset;
#else //NVMe mode
    volatile QWORD *pHostMemAddr;
    pHostMemAddr = (volatile QWORD *)HAL_GetLocalPrpEntryAddr(pHmemDptr->bsCmdTag, pHmemDptr->bsPrdOrPrpIndex);
    ulRemLenInEntry = HPAGE_SIZE - pHmemDptr->bsOffset;
#endif

    if(ulByteLenRemain <= ulRemLenInEntry)
    {
        bLastFlag = TRUE;
    }
    else
    {
        bLastFlag = FALSE;
    }
    return bLastFlag;
}

/* end of file HAL_ChainMaintain.c */
