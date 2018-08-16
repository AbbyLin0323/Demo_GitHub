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
* File Name    : L2_NVMe.c
* Discription  :
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.2
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_NormalDSG.h"
#include "HAL_HSG.h"
#include "HAL_SGE.h"
#include "HAL_FlashDriverExt.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "L1_CacheStatus.h"
#include "L2_NVMe.h"
#include "HAL_Dmae.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL  U32 g_ulL2DummyDataAddr;
extern GLOBAL U32 g_L1ErrorHandleStatus;

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
extern U8 L2_GetSuperPuFromLPN(U32 LPN);

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
MCU12_VAR_ATTR RdWithoutWtDptr *l_RdWithoutWtDptr;
LOCAL MCU12_VAR_ATTR U16 l_usL2PreFetchHsgId[SUBSYSTEM_LUN_MAX];

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

void L2_HostDramMap( U32 *pFreeDramBase )
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    // l_RdWithoutWtDptr 704B/16PU
    l_RdWithoutWtDptr = ( RdWithoutWtDptr * )ulFreeDramBase;
    COM_MemIncBaseAddr( &ulFreeDramBase, COM_MemSize16DWAlign( SUBSYSTEM_SUPERPU_NUM * sizeof( l_RdWithoutWtDptr[0] ) ) );

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    *pFreeDramBase = ulFreeDramBase;

    return ;
}

void L2_NVMeInitRdWithoutWtDptr()
{
    U8 ucPuNum;

    for(ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        l_usL2PreFetchHsgId[ucPuNum] = INVALID_4F;
        COM_MemZero((U32*)&l_RdWithoutWtDptr[ucPuNum], sizeof(RdWithoutWtDptr)/4);
    }


    return;
}

BOOL L2_CheckHsgResource(RdWithoutWtDptr *pDptr, BOOL BLast)
{
    U8 ucPuNum;
    BOOL bCheckSuccess = FALSE;

    if(FALSE == HAL_IsHsgValid())
    {
        HAL_TriggerHsg();
    }

    ucPuNum = L2_GetSuperPuFromLPN((pDptr->HmemDptr.ulLBA) >> SEC_PER_LPN_BITS);

    //if we have prefetched DSG
    if(INVALID_4F != l_usL2PreFetchHsgId[ucPuNum])
    {
        //last sub command only need one DSG
        if(TRUE == BLast)
        {
            pDptr->HsgId = l_usL2PreFetchHsgId[ucPuNum];
            pDptr->NextHsgId = INVALID_4F;
            bCheckSuccess = TRUE;
        }
        else
        {
            if(TRUE == HAL_IsHsgValid())
            {
                pDptr->HsgId = l_usL2PreFetchHsgId[ucPuNum];
                HAL_GetHsg(&pDptr->NextHsgId);
                bCheckSuccess = TRUE;
            }
        }
    }
    else
    {
        if(TRUE == HAL_IsHsgValid())
        {
            //save HSG id reported by SGE
            HAL_GetCurHsg(&l_usL2PreFetchHsgId[ucPuNum]);

            //last sub command only need one HSG
            if(TRUE == BLast)
            {
                pDptr->HsgId = l_usL2PreFetchHsgId[ucPuNum];
                pDptr->NextHsgId = INVALID_4F;
                bCheckSuccess = TRUE;
            }            

            //Trig Sge to get NewHsg;
            HAL_TriggerHsg();

            if(FALSE == BLast && TRUE == HAL_IsHsgValid())
            {
                if(TRUE == HAL_IsHsgValid())
                {
                    pDptr->HsgId = l_usL2PreFetchHsgId[ucPuNum];
                    HAL_GetHsg(&pDptr->NextHsgId);
                    bCheckSuccess = TRUE;
                }        
            }
        }
    }

    return bCheckSuccess;
}

BOOL HandleHostReadWithoutWrite(U8 ucPuNum, BUFREQ_HOST_INFO* pBufReqHostInfo, U8 ucStartLPNPos, U8 ucReqLPNCnt, U8 ucReqLenth)
{ 
    /* Aborts read request when error handling is in progress. Avoids blocking L1 error handling flow. */
    if (L1_ERRORHANDLE_INIT != g_L1ErrorHandleStatus)
    {
        l_RdWithoutWtDptr[ucPuNum].Initialized = FALSE;
        return TRUE;
    }    

    // if we come here firstly
     if(FALSE == l_RdWithoutWtDptr[ucPuNum].Initialized)
     {
        //initialize DPTR
        COM_MemCpy((U32*)&l_RdWithoutWtDptr[ucPuNum].HmemDptr,
                   (U32 *)&pBufReqHostInfo->HmemDptr,
                   sizeof(HMEM_DPTR)>>2);
        l_RdWithoutWtDptr[ucPuNum].ReqByteLen = ucReqLenth << SEC_SIZE_BITS;
        l_RdWithoutWtDptr[ucPuNum].RemainByteLen = l_RdWithoutWtDptr[ucPuNum].ReqByteLen;
        l_RdWithoutWtDptr[ucPuNum].bHsgDone = FALSE;
        l_RdWithoutWtDptr[ucPuNum].bDsgDone = FALSE;
        l_RdWithoutWtDptr[ucPuNum].Initialized = TRUE;
        l_RdWithoutWtDptr[ucPuNum].LPNStartPos = ucStartLPNPos;
        l_RdWithoutWtDptr[ucPuNum].LPNNextPos = ucStartLPNPos + ucReqLPNCnt -1;
        l_RdWithoutWtDptr[ucPuNum].pBufReqHostInfo = pBufReqHostInfo;
     }
     else
     {
        ucReqLenth = l_RdWithoutWtDptr[ucPuNum].ReqByteLen >> SEC_SIZE_BITS;        
     }

    // build HSG
    if(FALSE == l_RdWithoutWtDptr[ucPuNum].bHsgDone)
    {        
        while(0 != l_RdWithoutWtDptr[ucPuNum].RemainByteLen)
        {
            BOOL bHsgCheckResult;
            BOOL bLast;
            U32 ulNextHsgLen;

            bLast = HAL_CheckForLastHsg(&l_RdWithoutWtDptr[ucPuNum].HmemDptr,
                                        l_RdWithoutWtDptr[ucPuNum].RemainByteLen);

            bHsgCheckResult = L2_CheckHsgResource(&l_RdWithoutWtDptr[ucPuNum], bLast);

            if(TRUE == bHsgCheckResult)
            {
                if(l_RdWithoutWtDptr[ucPuNum].RemainByteLen == l_RdWithoutWtDptr[ucPuNum].ReqByteLen)
                {
                    l_RdWithoutWtDptr[ucPuNum].FirstHsgId = l_RdWithoutWtDptr[ucPuNum].HsgId;
                }

                ulNextHsgLen = (U32)l_RdWithoutWtDptr[ucPuNum].RemainByteLen;
                HAL_BuildHsg(&l_RdWithoutWtDptr[ucPuNum].HmemDptr,
                            &ulNextHsgLen,//value may be changed after function return
                            l_RdWithoutWtDptr[ucPuNum].HsgId,
                            l_RdWithoutWtDptr[ucPuNum].NextHsgId);
                HAL_SetHsgSts(l_RdWithoutWtDptr[ucPuNum].HsgId, 1);
                l_usL2PreFetchHsgId[ucPuNum] = l_RdWithoutWtDptr[ucPuNum].NextHsgId;

                l_RdWithoutWtDptr[ucPuNum].RemainByteLen = ulNextHsgLen;

                if(0 == l_RdWithoutWtDptr[ucPuNum].RemainByteLen)
                {                    
                    l_RdWithoutWtDptr[ucPuNum].bHsgDone = TRUE;
                }
                else
                {
                    if(HPAGE_SIZE == l_RdWithoutWtDptr[ucPuNum].HmemDptr.bsOffset)
                    {
                        pBufReqHostInfo = l_RdWithoutWtDptr[ucPuNum].pBufReqHostInfo++;
                        //initialize DPTR with next HMEM recorded
                        COM_MemCpy((U32*)&l_RdWithoutWtDptr[ucPuNum].HmemDptr,
                                   (U32 *)&pBufReqHostInfo->HmemDptr,
                                   sizeof(HMEM_DPTR)>>2);
                        l_RdWithoutWtDptr[ucPuNum].pBufReqHostInfo = pBufReqHostInfo;   
                    }
                }
            }
            else
            {                
                break;//skip building HSG
            }
        }
    }    

    // build DSG
    if(FALSE == l_RdWithoutWtDptr[ucPuNum].bDsgDone)
    {
        U16 usDsgId;
        NORMAL_DSG_ENTRY *pDsg;

        if(FALSE == HAL_GetNormalDsg(&usDsgId))
        {
            return FALSE;
        }

        pDsg = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usDsgId);
        COM_MemZero((U32 *)pDsg, sizeof(NORMAL_DSG_ENTRY)/sizeof(U32));
        pDsg->bsLast = TRUE;
        pDsg->bsDramAddr = (g_ulL2DummyDataAddr - DRAM_START_ADDRESS) >> 1;
        //pDsg->bsDramAddr = (g_ulL2DummyDataAddr + ucLunInSPU*LOGIC_PIPE_PG_SZ - DRAM_START_ADDRESS) >> 1;
        pDsg->bsXferByteLen = l_RdWithoutWtDptr[ucPuNum].ReqByteLen;
        HAL_SetNormalDsgSts(usDsgId, 1);

        //record first DSG id
        l_RdWithoutWtDptr[ucPuNum].FirstDsgId = usDsgId;
        l_RdWithoutWtDptr[ucPuNum].bDsgDone = TRUE;
    }

    //build DRQ
    if(TRUE == l_RdWithoutWtDptr[ucPuNum].bHsgDone
        && TRUE == l_RdWithoutWtDptr[ucPuNum].bDsgDone)
    {
        if(TRUE == HAL_DrqIsFull())
        {
            //DBG_Printf("!!!!!!!!!!!!!!!Drq is full\n");
            return FALSE;
        }

        /*add finished request length and chain num, if equal to total request length, get total chain num*/
        HAL_AddFinishReqLength(l_RdWithoutWtDptr[ucPuNum].HmemDptr.bsCmdTag, ucReqLenth);


        HAL_DrqBuildEntry(l_RdWithoutWtDptr[ucPuNum].HmemDptr.bsCmdTag,
                        l_RdWithoutWtDptr[ucPuNum].FirstHsgId,
                        l_RdWithoutWtDptr[ucPuNum].FirstDsgId);

        //set Initialized to FALSE
        l_RdWithoutWtDptr[ucPuNum].Initialized = FALSE;

        return TRUE;
    }

    return FALSE;
}

void L2_MoveHostPointer(HMEM_DPTR *PHostInfo, U32 SecLen)
{
    U16 usPrpIndex = PHostInfo->bsPrdOrPrpIndex;
    U32 ulCurPRPOffset = PHostInfo->bsOffset;
    U32 ulLengthByte  = SecLen<<SEC_SIZE_BITS;
    U32 ulCurPRPXferBytes = 0;


    while (ulLengthByte > 0)
    {
        ulCurPRPXferBytes = min((HPAGE_SIZE - ulCurPRPOffset) , ulLengthByte);
        if (ulCurPRPXferBytes == ulLengthByte)
        {
            ulCurPRPOffset += ulLengthByte;
            if(ulCurPRPOffset >= HPAGE_SIZE)
            {
                usPrpIndex++;
                ulCurPRPOffset = ulCurPRPOffset - HPAGE_SIZE;
            }
            ulLengthByte = 0;
        }
        else if (ulCurPRPXferBytes < ulLengthByte)
        {
            usPrpIndex++;
            ulCurPRPOffset = 0;
            ulLengthByte -= ulCurPRPXferBytes;

        }
    }
    //updata HMEM_DPTR
    PHostInfo->bsOffset = ulCurPRPOffset;
    PHostInfo->bsPrdOrPrpIndex = usPrpIndex;
}

/*****************************************************************************
 Prototype      : L2_FtlHandleReadWithoutWrite
 Description    : L2 FTL handle read without write
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/10/21
   Author       : henryluo
   Modification : Created function

*****************************************************************************/
BOOL L2_FtlHandleReadWithoutWrite(U8 ucSuperPu, BUF_REQ_READ* pReq, BUFREQ_HOST_INFO* pBufReqHostInfo, U8 LPNOffset, U8 ucReqLenth, U8 ucReqLPNCnt, BOOL bFirstCMDEn)
{
    U32 ulBufAddr;

    /* merge read */
    if(TRUE == pReq->bReqLocalREQFlag)
    {
        /* handle merge read without write */
        ulBufAddr =  COM_GetMemAddrByBufferID(pReq->usPhyBufferID, TRUE, BUF_SIZE_BITS);
        ulBufAddr += (LPNOffset  << LPN_SIZE_BITS);

        COM_MemClearSectorData(ulBufAddr, (U8)pReq->ulLPNBitMap);
        *(U8*)pReq->ulReqStatusAddr = SUBSYSTEM_STATUS_SUCCESS;

        l_RdWithoutWtDptr[ucSuperPu].LPNNextPos = LPNOffset + ucReqLPNCnt -1;

        return TRUE;
    }
    /* normal read */
    else
    {
        /* prefetch read (include host read from DRAM) */
        if (TRUE == pReq->bReadPreFetch)
        {
            int i;
            ulBufAddr =  COM_GetMemAddrByBufferID(pReq->usPhyBufferID, TRUE, BUF_SIZE_BITS);
            ulBufAddr += (LPNOffset << LPN_SIZE_BITS);
            
#ifdef SIM
            HAL_DMAESetValue(ulBufAddr, LPN_SIZE* ucReqLPNCnt, INVALID_8F);
#else
            HAL_DMAESetValue(ulBufAddr, LPN_SIZE* ucReqLPNCnt, 0);
#endif

            for (i = 0; i < ucReqLPNCnt; i++)
            {
                //COM_MemClearSectorData(ulBufAddr + i*(1<<LPN_SIZE_BITS), 0xFF);
                L1_L2SubPrefetchCacheStatus(pReq->usPhyBufferID);
            }
            l_RdWithoutWtDptr[ucSuperPu].LPNNextPos = LPNOffset + ucReqLPNCnt -1;

            return TRUE;
        }
        /* normal host read (host read from OTFB) */
        else
        {
            if (TRUE == HandleHostReadWithoutWrite(ucSuperPu, pBufReqHostInfo, LPNOffset, ucReqLPNCnt, ucReqLenth))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

}

/*====================End of this file========================================*/

