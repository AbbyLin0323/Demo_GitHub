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
  File Name     : L2_Sata.c
  Version       : Initial Draft
  Author        : henryluo
  Created       : 2014/10/21
  Description   : L2 SATA protocol reference
  Description   :
  Function List :
  History       :
  1.Date        : 2014/10/21
    Author      : henryluo
    Modification: Created file

*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_BufMap.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "FW_BufAddr.h"
#include "L2_Sata.h"
#include "L2_FCMDQ.h"
#include "L1_CacheStatus.h"
#include "L2_StripeInfo.h"
#include "HAL_Dmae.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/
extern U32  L2_GetPuFromLPN(U32 LPN);

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

void L2_HostDramMap( U32 *pFreeDramBase )
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    COM_MemAddr16DWAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return ;
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
    U8  ucPuNum;
    U32 ulBufAddr;
    U32 ulBufMapValue;
    int i;

    /* merge read */
    if(TRUE == pReq->bReqLocalREQFlag)
    {
        /* handle merge read without write */
        ulBufAddr =  COM_GetMemAddrByBufferID(pReq->usPhyBufferID, TRUE, BUF_SIZE_BITS);
        ulBufAddr += (LPNOffset  << LPN_SIZE_BITS);
        COM_MemClearSectorData(ulBufAddr, (U8)pReq->ulLPNBitMap);

        *(U8 *)pReq->ulReqStatusAddr = SUBSYSTEM_STATUS_SUCCESS;

#ifdef A0_B0_DSG_SSU_STATUS_PATCH
        *(U8 *)pReq->ReqStatusAddrDSG = SUBSYSTEM_STATUS_SUCCESS;
#endif
    }
    /* normal read */
    else
    {
        ulBufAddr = COM_GetMemAddrByBufferID(pReq->usPhyBufferID, TRUE, BUF_SIZE_BITS);
        ulBufAddr += (LPNOffset << LPN_SIZE_BITS);
#ifdef SIM
        HAL_DMAESetValue(ulBufAddr, LPN_SIZE* ucReqLPNCnt, INVALID_8F);
#else
        HAL_DMAESetValue(ulBufAddr, LPN_SIZE* ucReqLPNCnt, 0);
#endif        

        /* prefetch read */
        if (TRUE == pReq->bReadPreFetch)
        {
            for (i = 0; i < ucReqLPNCnt; i++)
            {
                L1_L2SubPrefetchCacheStatus(pReq->usPhyBufferID);
            }

#ifdef A0_B0_DSG_SSU_STATUS_PATCH
            L1_L2SubPrefetchCacheStatus(pReq->PhyBufferID);
#endif
        }
        /* normal host read */
        else
        {            
            /* wait CE_FIFO empty to avoid bufmap overwrited */
            ucPuNum = L2_GetSuperPuFromLPN(pReq->aLPN[LPNOffset]);
            if(TRUE != L2_FCMDQIsEmpty(ucPuNum, 0))
            {
                return FALSE;
            }            

            ulBufMapValue = COM_GETBITMASK(LPNOffset, LPNOffset + ucReqLPNCnt -1);
            HAL_UpdateBufMapValue(pReq->tBufReqHostInfo.ucDSGId, ulBufMapValue, ~ulBufMapValue);
            if (TRUE == bFirstCMDEn)
            {
                HAL_SetFirstReadDataReady(pReq->tBufReqHostInfo.ucTag);
            }
        }
    }

    return TRUE;
}

