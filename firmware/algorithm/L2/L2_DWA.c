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
File Name     : L2_DWA.c
Version       : Initial version
Author        : henryluo
Created       : 2014/12/4
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2014/12/4
Author      : henryluo
Modification: Created file

*******************************************************************************/

#include "L2_DWA.h"
#include "L2_StripeInfo.h"
#include "L2_PMTI.h"
#include "L2_VBT.h"
#include "L2_PMTManager.h"
#include "L2_GCManager.h"
#include "L2_Boot.h"
#include "L2_Thread.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
GLOBAL  U32 g_DWASustainWriteThs = 512;
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



#if 0
/*****************************************************************************
 Prototype      : L2_GetCurrentLS
 Description    : get current logical saturation 
 Input          : U8 ucPU
 Output         : None
 Return Value   : U32
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_GetCurrentLS( U8 ucPU )
{
    U16 usVBN;
    U16 usDirtyCnt;
    U32 ulDirtyRate;
    U32 ulTotalDirtyCnt = 0;
    U32 ulPhysicalUsedLPNCnt = 0;
    PuInfo* pInfo;
    U32 ulCurrLS;

    /* get total dirty counts in pu */
    for(usVBN = 0;  usVBN < VIR_BLK_CNT; usVBN++)
    {
        usDirtyCnt = L2_GetDirtyCnt(ucPU, usVBN);
        ulTotalDirtyCnt += usDirtyCnt;
    }

    /* calc dirty rate */
    pInfo = g_PuInfo[ucPU];
    ulPhysicalUsedLPNCnt = (pInfo->m_AllocateBlockCnt * LPN_PER_BLOCK)
        - (pInfo->m_UsedACCnt * (LPN_PER_BLOCK/2));

    if(ulTotalDirtyCnt > MAX_FLOAT_VALUE)
    {
        DBG_Printf("ulTotalDirtyCnt overflowed !!!\n");
        DBG_Getch();
    }
    ulDirtyRate = (ulTotalDirtyCnt << FLOAT_FACTOR_BIT) / ulPhysicalUsedLPNCnt;

    /* calc logical saturation */
    ulCurrLS = ((1 << FLOAT_FACTOR_BIT) - ulDirtyRate) * (pInfo->m_AllocateBlockCnt - pInfo->m_UsedACCnt/2)
        / DATA_BLOCK_PER_PU;

    return ulCurrLS;
}


/*****************************************************************************
 Prototype      : L2_DWAInit
 Description    : init structure of DWA
 Input          : None
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/10
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_DWAInit( void )
{
    U8 ucPU;
    PuInfo* pInfo;

    for ( ucPU = 0 ; ucPU < SUBSYSTEM_LUN_NUM ; ucPU++ )
    {
        pInfo = g_PuInfo[ucPU];
        pInfo->m_UsedACCnt = 0;
    }

    return;
}
#endif

/*****************************************************************************
 Prototype      : L2_GetCurrentLS
 Description    : get current logical saturation
 Input          : U8 ucPU
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_GetCurrentLS(U8 ucPU)
{
    U32  uPMTIndexInPu;
    U32 ulValidLPNCnt = 0;
    U32 ulCurrLS;

    /* get valid LPN count in this pu */
    for (uPMTIndexInPu = 0; uPMTIndexInPu < PMTPAGE_CNT_PER_PU; uPMTIndexInPu++)
    {
#ifdef ValidLPNCountSave_IN_DSRAM1
        ulValidLPNCnt += U24getValue(ucPU, uPMTIndexInPu); 
#else
        ulValidLPNCnt += g_PMTManager->m_PMTSpareBuffer[ucPU][uPMTIndexInPu]->m_ValidLPNCountSave;
#endif
    }

    /* calc logical saturation */
    ulCurrLS = (ulValidLPNCnt << FLOAT_FACTOR_BIT) / LPN_PER_PU;

    return ulCurrLS;
}

/*****************************************************************************
 Prototype      : L2_GetCurrMaxAC
 Description    : Get current MAX AC permitted by LS in PU
 Input          : float fLS
 Output         : None
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_GetCurrMaxAC(U8 ucPU)
{
    U32 ulCurrLS;

    /* Get current LS */
    ulCurrLS = L2_GetCurrentLS(ucPU);
    if (DWA_EN_MAX_LS < ulCurrLS)
    {
        return 0;
    }
    else
    {
        return MAX_AC_PER_PU;
    }
}

/*****************************************************************************
 Prototype      : L2_IsCanBeAccWrite
 Description    : check wether current write can be accelerator in this PU
 Input          : None
 Output         : None
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
#ifdef DWA
BOOL L2_IsCanBeAccWrite(U8 ucPU)
{
    U32 ulCurrMaxAC;
    PuInfo* pInfo;

    /* forbidden DWA during GC */
    if(SYSTEM_STATE_GC == L2_GetCurrState(ucPU))
    {
        return FALSE;
    }

    pInfo = g_PuInfo[ucPU];
    if(pInfo->m_SustainWritePageCnt >= g_DWASustainWriteThs)
    {
        /* Get current AC */
        ulCurrMaxAC = L2_GetCurrMaxAC(ucPU);

        /* check used accelerator capacity */
        if(pInfo->m_UsedACCnt < ulCurrMaxAC)
        {
            return TRUE;
        }
    }

    /* once DWA started, must write to clock the SLC block */
    if((PG_PER_SLC_BLK != pInfo->m_TargetPPO[TARGET_HOST_WRITE_ACC]) && (INVALID_4F != pInfo->m_TargetBlk[TARGET_HOST_WRITE_ACC]))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*****************************************************************************
 Prototype      : L2_GetRemainedAC
 Description    : get remained AC in pu
 Input          : U8 ucPU
 Output         : None
 Return Value : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_GetRemainedAC(U8 ucPU)
{
    U32 ulCurrMaxAC;
    U32 ulRemainAC;
    PuInfo* pInfo;

    pInfo = g_PuInfo[ucPU];

    /* Get current permitted max AC */
    ulCurrMaxAC = L2_GetCurrMaxAC(ucPU);

    /* get remained AC */
    if (ulCurrMaxAC >= pInfo->m_UsedACCnt)
    {
        ulRemainAC = ulCurrMaxAC - pInfo->m_UsedACCnt;
    }
    else
    {
        ulRemainAC = 0;
    }

    return ulRemainAC;
}
#endif

#if 0
/*****************************************************************************
 Prototype      : L2_IsNeedIdleGC
 Description    : check AC remained to decide wether do idle GC
 Input          : U8 ucPU
 Output         : None
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL L2_IsNeedIdleGC(void)
{
#ifdef FLASH_INTEL_3DTLC
    return FALSE;
#else
    PuInfo* pInfo;
    U32 ulCurrMaxAC;
    U16 usFreeBlockCnt;
    U16 usIdleGCThs;
    U8 ucSuperPu;
    BOOL bNeedGC = FALSE;

    if (FALSE == L2_IsBootupOK())
    {
        return FALSE;
    }

    for ( ucSuperPu = 0 ; ucSuperPu < SUBSYSTEM_SUPERPU_NUM ; ucSuperPu++ )
    {
        pInfo = g_PuInfo[ucSuperPu];

        /* Get current AC */
        ulCurrMaxAC = L2_GetCurrMaxAC(ucSuperPu);
        if(0 == ulCurrMaxAC)
        {
            /* check used accelerator capacity */
            if (pInfo->m_UsedACCnt > ulCurrMaxAC)
            {
                /* set this PU to GC state */
                L2_SetIdleGC(ucSuperPu,TRUE);
                bNeedGC = TRUE;
                continue;
            }
            else
            {
                usIdleGCThs = (GC_THRESHOLD_BLOCK_CNT * 2);
            }
        }
        else
        {
            usIdleGCThs = IDLE_GC_THRESHOLD;
        }

        /* check free block count */
        usFreeBlockCnt = pInfo->m_DataBlockCnt[0] - pInfo->m_AllocateBlockCnt[0];
        if (usFreeBlockCnt < usIdleGCThs)
        {
            /* set this PU to GC state */
            L2_SetIdleGC(ucSuperPu,TRUE);
            bNeedGC = TRUE;
        }
    }

    return bNeedGC;
#endif
}
#endif

#if 0
/*****************************************************************************
 Prototype      : L2_IdleGCSelSrc
 Description    : SLC region GC select victim block
 Input          : U8 ucPU
 Output         : None
 Return Value : U16
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/8
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U16 L2_IdleGCSelSrc( U8 ucPU )
{
    U16 usMostDirtyCnt = 0;
    U16 usVBN = INVALID_4F;

    U16 i = 0;
    PuInfo* pInfo = NULL;
    U16 usDirtyCnt = 0;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif   

    pInfo = g_PuInfo[ucPU];
    for(i = 0; i < VIR_BLK_CNT; i++)
    {
        if(ucPU != pVBT[ucPU]->m_VBT[i].StripID)
        {
            continue;
        }

        if(VBT_NOT_TARGET != pVBT[ucPU]->m_VBT[i].Target)
        {
            continue;
        }

        if(TRUE != pVBT[ucPU]->m_VBT[i].bSLC)
        {
            continue;
        }

        if(TRUE == pVBT[ucPU]->m_VBT[i].bLockedInWL)
        {
            continue;
        }

        usDirtyCnt = L2_GetDirtyCnt(ucPU, i);
        if(usDirtyCnt > usMostDirtyCnt)
        {
            usMostDirtyCnt = usDirtyCnt;
            usVBN = i;
        }
    }

    return usVBN;
}



/*****************************************************************************
 Prototype      : L2_IdleGCSLC
 Description    : MLC region idle GC
 Input          : U8 ucPU
 Output         : None
 Return Value   : void
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_IdleGCSLC( U8 ucPU )
{
    GCState* pState;
    U16 usVBN;

    pState = &g_GCDptr.m_GCState;
    switch ( *pState )
    {
    case IDLE_GC_SLC_STAGE_INIT :
        ;
        break;
    case IDLE_GC_SLC_STAGE_SEL_SRC :
        /* select victim block for SLC GC */
        usVBN = L2_IdleGCSelSrc(ucPU);
        if(INVALID_4F == usVBN)
        {
            *pState = IDLE_GC_SLC_STAGE_DONE;
        }
        else
        {
            *pState = IDLE_GC_SLC_STAGE_LOAD_RPMT;
        }
        break;

    case IDLE_GC_SLC_STAGE_LOAD_RPMT :
        if(TRUE == L3_FlashReqFifoFull(ucPU))
        {
            return FAIL;
        }
        else
        {
            SetSrcBlock(i, BlockSN, PUSer);
            LoadRPMTInGCManager(g_GCManager, ucPU, usVBN);

            *pState = IDLE_GC_SLC_STAGE_READ;
        }
        break;

    case IDLE_GC_SLC_STAGE_READ :
        ;
        break;
    case IDLE_GC_SLC_STAGE_WRITE :
        ;
        break;
    case IDLE_GC_SLC_STAGE_ERASE :
        ;
        break;
    case IDLE_GC_SLC_STAGE_DONE :
        ;
        break;

    default:
        break;
    }

    /* */
    ;
}

/*****************************************************************************
 Prototype      : L2_IdleGCMLC
 Description    : MLC region idle GC
 Input          : U8 ucPU
 Output         : None
 Return Value   : void
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_IdleGCMLC( U8 ucPU )
{
    ;

}

/*****************************************************************************
 Prototype      : L2_IdleGCEntry
 Description    : DWA idle GC
 Input          : void
 Output         : None
 Return Value   : void
 Calls          : 
 Called By      : 

 History        :
 1.Date         : 2014/12/4
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_IdleGCEntry( void )
{
    U8 ucPU;
    IDLE_GC_STAGE eGCStage;


    eGCStage = g_L2DWAManagement.m_IdleGCStage;
    switch ( eGCStage )
    {
    case IDLE_GC_STAGE_INIT :
        g_L2DWAManagement.m_IdleGCDonePuMap = 0;
        break;

    case IDLE_GC_STAGE_SLC :
        /* SLC region GC */
        for ( ucPU = 0 ; ucPU < SUBSYSTEM_LUN_NUM ; ucPU++ )
        {
            if((TRUE == L2_IsNeedIdleGC(ucPU)) && (0 != g_L2DWAManagement.m_UsedAC[ucPU]))
            {
                /* */
                L2_IdleGCSLC(ucPU);
            }
            else
            {
                g_L2DWAManagement.m_IdleGCDonePuMap |= (1 << ucPU);
            }
        }

        /* check SLC region GC done on all PU */
        if((INVALID_8F >> (32 - SUBSYSTEM_LUN_NUM)) == g_L2DWAManagement.m_IdleGCDonePuMap)
        {
            g_L2DWAManagement.m_IdleGCDonePuMap = 0;
            g_L2DWAManagement.m_IdleGCStage = IDLE_GC_STAGE_MLC;
        }

        break;

    case IDLE_GC_STAGE_MLC :
        /* MLC region GC */
        for ( ucPU = 0 ; ucPU < SUBSYSTEM_LUN_NUM ; ucPU++ )
        {
            if(TRUE == L2_IsNeedIdleGC(ucPU))
            {
                /* */
                L2_SetRefGCBlock(ucPU);
            }
            else
            {
                g_L2DWAManagement.m_IdleGCDonePuMap |= (1 << ucPU);
            }
        }

        /* check SLC region GC done on all PU */
        if((INVALID_8F >> (32 - SUBSYSTEM_LUN_NUM)) == g_L2DWAManagement.m_IdleGCDonePuMap)
        {
            g_L2DWAManagement.m_IdleGCDonePuMap = 0;
            g_L2DWAManagement.m_IdleGCStage = IDLE_GC_STAGE_DONE;
        }
        else
        {
            L2_SetSystemState(SYSTEM_STATE_GC);
        }
        break;

    case IDLE_GC_STAGE_DONE :
        ;
        break;

    default:

        break;
    }


    return;
}
#endif


