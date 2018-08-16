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
Filename    : L1_Trim.c
Version     :
Author      :
Date        :
Description :L1 Trim process
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"

GLOBAL  U32 g_ulTrimStartLBA;
GLOBAL  U32 g_ulTrimEndLBA;
GLOBAL  U32 g_ulTrimReadLBA;
GLOBAL  U32 g_ulTrimReadCnt;

#ifndef LCT_TRIM_REMOVED
GLOBAL  U32 g_ulPendingTrimLctCount;
#endif
extern GLOBAL U32 g_ulPMTFlushing;

/****************************************************************************
Function  : L1_TrimProcessInit
Input     : 
Output    : 
Return    : 
Purpose   : 
init Trim SCMD process
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
extern BOOL L2_IsBootupOK();
extern BOOL L2_TrimLPN(U32 LPN);
U8 L1_TrimProcessInit(U32 ulStartLBA, U32 ulEndLBA)
{
    // 20150901 Sean Gao
    // this function initializes variables for the current trim operation,
    // return TRUE if the initialization is successful, FALSE otherwise

    // ensure L2 PMT is fully loaded before proceeding
    if(FALSE == L2_IsBootupOK())
    {
        return FALSE;
    }

    // check the trim range, ensure that both start and end LBA are legal
    // the maximum LBA trim can access is (MAX_LBA_IN_DISK-1)
    if ((MAX_LBA_IN_DISK - 1) < ulStartLBA)
    {
        // the start LBA is outside of the legal boundary, return FALSE
        return FALSE;
    }
    else if ((MAX_LBA_IN_DISK - 1) < ulEndLBA)
    {
        // adjust the end LBA to make it inside of legal boundary
        ulEndLBA = MAX_LBA_IN_DISK;
    }

    // set the start and the end of the current trim subcommand
    g_ulTrimStartLBA = ulStartLBA;
    g_ulTrimEndLBA = ulEndLBA;

    // process the current trim target LCT if it is present, this is
    // just a safety measure
    if(g_ulCurTrimTargetLct != INVALID_8F)
    {
        U8 i;
        U32 ulCurTrimLctStartLpn;

        // calculate the start LPN for the current trim target LCT
        ulCurTrimLctStartLpn = LPN_FROM_LCTINDEX(g_ulCurTrimTargetLct);

        // perform trim operations on all LPNs
        for(i = g_ulCurTrimTargetLctLpnOffset; i < LPN_PER_BUF; ++i)
        {
            if( TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
            {
                L2_TrimLPN(ulCurTrimLctStartLpn + i);
            }
        }
        
#ifndef LCT_TRIM_REMOVED
        // update the trim bitmap
        g_pulTrim[LCTTAG_FROM_INDEX(g_ulCurTrimTargetLct)] &= (~(1 << LCTMSK_FROM_INDEX(g_ulCurTrimTargetLct)));
#endif

        // reset current trim target LCT and LPN offset
        g_ulCurTrimTargetLct = INVALID_8F;
        g_ulCurTrimTargetLctLpnOffset = INVALID_8F;

#ifndef LCT_TRIM_REMOVED
        // update the number of pending trim LCT count
        g_ulPendingTrimLctCount--;
#endif
    }

    // clear the non-4k trim range
    g_ulTrimReadLBA = INVALID_8F;
    g_ulTrimReadCnt = INVALID_8F;

    return TRUE;
}

U32 L1_TrimPrerequisiteCheck(void)
{
    // Sean Gao 20150901
    // this function checks the status of L1, only when all requirements in this
    // function are meet can we start the trim operation

    U8 ucPuNum;

    // make sure all pending buffers are processed
    for(ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        if((0 != g_WriteBufInfo[ucPuNum].ucMergePendingBufCnt) 
            ||(0 != g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt)
            ||(0 != g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt))
        {
            return FALSE;
        }
    }

    // wait until all pending buffer requests are processed, we do have to wait for this since
    // PMT is looked up and modified when processing buffer requests, it is imperative that
    // these buffer requests are processed before the trim subcommand modifies the PMT
    if(FALSE == L1_BufReqEmpty())
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL L1_TrimProcessNon4KAlignedRange(void)
{
    // Sean Gao 20150902
    // this function processes the non-4k aligned range of the current trim
    // command, it returns TRUE when all non-4k aligned trim range are processed,
    // FALSE otherwise, when the process are completed, the trim range will either
    // be 4K aligned or INVALID_8F

    // check if there if a pending non-4k aligned trim range,
    // if there isn't, try get one
    if(g_ulTrimReadLBA == INVALID_8F)
    {
        // process non-4k aligned trim range in the start of the trim subcommand
        if(((g_ulTrimStartLBA & SEC_PER_LPN_MSK) != 0) && (g_ulTrimStartLBA != INVALID_8F))
        {
            // update the pending non-4k aligned trim range
            g_ulTrimReadLBA = g_ulTrimStartLBA;

            // update the start LBA of the current trim subcommand
            g_ulTrimStartLBA = (g_ulTrimStartLBA & (~SEC_PER_LPN_MSK)) + SEC_PER_LPN;

            // check if the start LBA of current trim command is still legal
            if(g_ulTrimStartLBA >= g_ulTrimEndLBA)
            {
                // the updated start LBA is greater or equal to the end LBA, this means
                // the whole trim range is less than 1 LPN
                
                // update the number of sectors to be read for the current non-4k aligned
                // operation
                g_ulTrimReadCnt = g_ulTrimEndLBA - g_ulTrimReadLBA;

                // reset the the trim start and end LBA
                g_ulTrimStartLBA = INVALID_8F;
                g_ulTrimEndLBA = INVALID_8F;
            }
            else
            {
                g_ulTrimReadCnt = g_ulTrimStartLBA - g_ulTrimReadLBA;
            }
        }
        else if(((g_ulTrimEndLBA & SEC_PER_LPN_MSK) != 0) && (g_ulTrimEndLBA != INVALID_8F))
        {
            // update the pending non-4k aligned trim range
            g_ulTrimReadLBA  = (g_ulTrimEndLBA & (~SEC_PER_LPN_MSK));

            // check if the start LBA of the pending non-4k aligned range is less or equal to
            // the start LBA of the current trim operation
            if(g_ulTrimReadLBA <= g_ulTrimStartLBA)
            {
                // the whole trim range is less than 1 LPN
                
                // set the start LBA of the pending non-4k aligned range to the start LBA
                // of the current trim operation
                g_ulTrimReadLBA = g_ulTrimStartLBA;
                g_ulTrimReadCnt = g_ulTrimEndLBA - g_ulTrimReadLBA;

                // reset the the trim start and end LBA
                g_ulTrimStartLBA = INVALID_8F;
                g_ulTrimEndLBA = INVALID_8F;
            }
            else
            {
                g_ulTrimReadCnt = g_ulTrimEndLBA - g_ulTrimReadLBA;

                // update the trim end LBA
                g_ulTrimEndLBA = g_ulTrimReadLBA;
            }
        }
        else
        {
            // at this point, either both trim start and end LBA are INVALID_8F or
            // 4K aligned

#ifdef SIM
            // check the legitimacy of the start and end LBA of the current trim operation
            if(!((g_ulTrimStartLBA == INVALID_8F && g_ulTrimEndLBA == INVALID_8F) || (((g_ulTrimStartLBA & SEC_PER_LPN_MSK) == 0) && ((g_ulTrimEndLBA & SEC_PER_LPN_MSK) == 0))))
            {
                DBG_Printf("trim process non-4k aligned trim range error\n");
                DBG_Getch();
            }

            // ensure there is no pending non-4k aligned trim range
            if((g_ulTrimReadLBA != INVALID_8F) || (g_ulTrimReadCnt != INVALID_8F))
            {
                DBG_Printf("trim process pending non-4k aligned trim range error\n");
                DBG_Getch();
            }
#endif

            // return TRUE to indicate all non-4k aligned trim range is processed
            return TRUE;
        }
    } // if(g_ulTrimReadLBA == INVALID_8F)

    // process pending non-4k aligned trim range
    if(g_ulTrimReadLBA != INVALID_8F)
    {
        U8  ucLoop;
        U8  ucSectorOffset; 
        U8  ucCacheOffset;
        U8  ucBufferLpnSectorMap;
        U16 usLogBufID;
        U16 usPhyBufID;
        U8  ucSuperPu;
        U16 usCachePhyBufID;
        U32 notAlignedStartLpn;
        LCT_HIT_RESULT HitResult;

        // calculate the not aligned LPN
        notAlignedStartLpn = g_ulTrimReadLBA >> SEC_PER_LPN_BITS;

#ifndef LCT_VALID_REMOVED
        // check if the LCT is valid, we don't have to do anything if it's not
        if(L1_GetLCTValid(LCTINDEX_FROM_LPN(notAlignedStartLpn)) == FALSE)
        {
            g_ulTrimReadLBA = INVALID_8F;
            g_ulTrimReadCnt = INVALID_8F;
            return FALSE;
        }
#endif

        // calculate the offset of trim start LBA within the LPN
        ucSectorOffset = (g_ulTrimReadLBA & SEC_PER_LPN_MSK);

        // set the buffer bitmap indicating which sectors of this LPN should be cleared
        ucBufferLpnSectorMap = 0;
        for (ucLoop = 0; ucLoop < g_ulTrimReadCnt; ucLoop++)
        {
            ucBufferLpnSectorMap |= (1 << (ucSectorOffset + ucLoop));
        }

        // search the LCT to see if notAlignedStartLpn is in buffer
        HitResult.AsU32 = L1_CacheIDLinkSearchLPN(notAlignedStartLpn, 1, FALSE);

        ucCacheOffset = HitResult.ucCacheOffset;
        usLogBufID    = HitResult.usLogBufID;
        usPhyBufID    = PHYBUFID_FROM_LGCBUFID(usLogBufID);

        if(L1_CACHE_SE_FULL_HIT == HitResult.ucHitResult)
        {
            if(BUF_STAGE_RECYCLE == gpBufTag[usLogBufID].Stage)
            {
                // the merge LPN is in read cache, invalidate it in LCT
                L1_CacheInvalidLPNInLCT(usLogBufID, ucCacheOffset, 1);
            }
            else if(BUF_STAGE_FLUSH == gpBufTag[usLogBufID].Stage)
            {
                return FALSE;
            }
            else
            {
                DBG_Printf("trim buffer status error\n");
                DBG_Getch();
            }
        } // if(L1_CACHE_SE_FULL_HIT == HitResult.ucHitResult)

        // process the non-4k aligned trim range as a random write

        // get the cacheline of the lpn to be merged
        ucSuperPu = L1_GetSuperPuFromLCT(LCTINDEX_FROM_LPN(notAlignedStartLpn));

        // check current cacheline buffer state
        // not enough space in cacheline, release the cacheline
        if (L1_STATUS_CACHE_NO_SPACE == L1_CacheGetTagStatus(ucSuperPu, 1))
        {
            L1_ReleaseCacheLine(ucSuperPu);
        }

        // no buffer has been attached to the cacheline, allocate a new write buffer
        if (L1_STATUS_CACHE_NO_BUFF == L1_CacheGetTagStatus(ucSuperPu, 1))
        {
            // allocate a new write buffer
            usCachePhyBufID = L1_AllocateWriteBuf(ucSuperPu);

            if(INVALID_4F == usCachePhyBufID)
            {
                // no write buffer is available
                return FALSE;
            }

            // attach the new buffer to the cacheline
            L1_CacheAttachBuffer(ucSuperPu, usCachePhyBufID);

            // add the lpn in the lct
            L1_CacheTrimAddNewLPN(ucSuperPu, notAlignedStartLpn, ucBufferLpnSectorMap);

            // set the buffer stage to cache
            gpBufTag[LGCBUFID_FROM_PHYBUFID(usCachePhyBufID)].Stage = BUF_STAGE_CACHE;
        }
        else
        {    
            L1_CacheTrimAddNewLPN(ucSuperPu, notAlignedStartLpn, ucBufferLpnSectorMap);
        }

        // the current pending non-4k aligned trim range is processed successfully
        g_ulTrimReadLBA = INVALID_8F;
        g_ulTrimReadCnt = INVALID_8F;
    }

    return FALSE;
}

extern void L2_TrimReqProcess(U32 TrimStartLPN, U32 TrimLPNCount);
BOOL L1_TrimProcess4KAlignedRange(void)
{
    // Sean Gao 20150902
    // this function processes all 4K-aligned but not LCT aligned trim range
    
    U32 ulTempLBA;
    U32 ulTrimStartLpn;
    U32 ulTrimLpnCount;

#ifdef SIM
    // check the legitimacy of the start and end LBA of the current trim operation,
    // at this point, the start and end LBA must be either INVALID_8F or 4K aligned
    if(!((g_ulTrimStartLBA == INVALID_8F && g_ulTrimEndLBA == INVALID_8F) || (((g_ulTrimStartLBA & SEC_PER_LPN_MSK) == 0) && ((g_ulTrimEndLBA & SEC_PER_LPN_MSK) == 0))))
    {
        DBG_Printf("trim process 4k aligned range error\n");
        DBG_Getch();
    }
#endif

    // check if the current trim operation is actually finished already, if it is,
    // just return TRUE
    if(g_ulTrimStartLBA == INVALID_8F && g_ulTrimEndLBA == INVALID_8F)
    {
        return TRUE;
    }

    // invalidate all LCT within the 4K-aligned trim range
    L1_CacheInvalidTrimLCT(g_ulTrimStartLBA, g_ulTrimEndLBA);

    // if the whole trim range is less than the size of one LCT, we will modify the L2 PMT
    // directly
    if((g_ulTrimEndLBA - g_ulTrimStartLBA) <= SEC_PER_BUF)
    {
        // calculate the start LPN of the current trim operation
        ulTrimStartLpn = g_ulTrimStartLBA >> SEC_PER_LPN_BITS;

        // calculate the number of LPNs to be trimmed
        ulTrimLpnCount = (g_ulTrimEndLBA - g_ulTrimStartLBA) >> SEC_PER_LPN_BITS;

        // trim the LPNs directly
        if(ulTrimLpnCount != 0)
        {
            if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
            {
                L2_TrimReqProcess(ulTrimStartLpn, ulTrimLpnCount);
            }
        }

        // reset the start and end LBA of the current trim operation
        g_ulTrimStartLBA = INVALID_8F;
        g_ulTrimEndLBA = INVALID_8F;

        return TRUE;
    }

    // make the start LBA LCT aligned
    if((g_ulTrimStartLBA & SEC_PER_BUF_MSK) != 0)
    {
        // update the start LBA
        ulTempLBA = (g_ulTrimStartLBA & (~SEC_PER_BUF_MSK)) + SEC_PER_BUF;

        // calculate the start LPN of the current trim operation
        ulTrimStartLpn = g_ulTrimStartLBA >> SEC_PER_LPN_BITS;

        // calculate the number of LPNs to be trimmed
        ulTrimLpnCount = (ulTempLBA - g_ulTrimStartLBA) >> SEC_PER_LPN_BITS;

        // trim the LPNs directly
        if(ulTrimLpnCount != 0)
        {
            if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
            {
                L2_TrimReqProcess(ulTrimStartLpn, ulTrimLpnCount);
            }
        }

        g_ulTrimStartLBA = ulTempLBA;
    }

    // make the end LBA LCT aligned
    if((g_ulTrimEndLBA & SEC_PER_BUF_MSK) != 0)
    {
        // update the end LBA
        ulTempLBA = g_ulTrimEndLBA & (~SEC_PER_BUF_MSK);

        // calculate the start LPN of the current trim operation
        ulTrimStartLpn = ulTempLBA >> SEC_PER_LPN_BITS;

        // calculate the number of LPNs to be trimmed
        ulTrimLpnCount = (g_ulTrimEndLBA - ulTempLBA) >> SEC_PER_LPN_BITS;

        // trim the LPNs directly
        if(ulTrimLpnCount != 0)
        {
            if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
            {
                L2_TrimReqProcess(ulTrimStartLpn, ulTrimLpnCount);
            }
        }

        g_ulTrimEndLBA = ulTempLBA;
    }

    // check if there is any trim range ramaining
    if(g_ulTrimStartLBA >= g_ulTrimEndLBA)
    {
        g_ulTrimStartLBA = INVALID_8F;
        g_ulTrimEndLBA = INVALID_8F;
    }

    return TRUE;
}

#ifndef LCT_VALID_REMOVED
extern void L1_MarkLCTValid(U32 ulStartLCT, U32 ulEndLCT);
#endif
BOOL L1_TrimProcessLctAlignedRange(void)
{
    // Sean Gao 20150903
    // this functions processes all LCT aligned trim range
    // at this point, all non-4k aligned trim range and 4k aligned trim range are processed,
    // the only remain is LCT aligned trim range
    U32 ulTrimStartLctIndex;
    U32 ulTrimEndLctIndex;
#ifndef LCT_TRIM_REMOVED
    U32 ulTrimActualStartLctIndex;
    U32 ulTrimActualEndLctIndex;
#endif

#ifdef SIM
    // check the legitimacy of the start and end LBA of the current trim operation,
    // at this point, the start and end LBA must be either INVALID_8F or 4K aligned
    if(!((g_ulTrimStartLBA == INVALID_8F && g_ulTrimEndLBA == INVALID_8F) || (((g_ulTrimStartLBA & SEC_PER_BUF_MSK) == 0) && ((g_ulTrimEndLBA & SEC_PER_BUF_MSK) == 0))))
    {
        DBG_Printf("trim process LCT aligned range error\n");
        DBG_Getch();
    }
#endif

    // check if there is ramaining trim range
    if(g_ulTrimStartLBA == INVALID_8F && g_ulTrimEndLBA == INVALID_8F)
    {
        return TRUE;
    }

    // calculate the start and end LCT index of the current trim operation
    ulTrimStartLctIndex = LCTINDEX_FROM_LBA(g_ulTrimStartLBA);
    ulTrimEndLctIndex = LCTINDEX_FROM_LBA(g_ulTrimEndLBA);

#ifdef LCT_TRIM_REMOVED
        if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
        {
            L2_TrimReqProcess(LPN_FROM_LCTINDEX(ulTrimStartLctIndex), ((ulTrimEndLctIndex - ulTrimStartLctIndex) * LPN_PER_BUF));
            g_ulTrimStartLBA = INVALID_8F;
            g_ulTrimEndLBA = INVALID_8F;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
#else
#define DIRECTLY_FLUSH_TRIM
#ifdef DIRECTLY_FLUSH_TRIM
    // the following section of code flushes certain amount of trim directly to L2 PMT
    // instead of buffering them
    if((ulTrimEndLctIndex - ulTrimStartLctIndex) <= L1_NON_BUFF_TRIM_THS)
    {
#ifndef LCT_VALID_REMOVED
        // clear the LCT valid bit for LCTs to be trimmed
        L1_MarkLCTValid(ulTrimStartLctIndex, ulTrimEndLctIndex);
#endif

        // directly trim the LPNs
        if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
        {
            L2_TrimReqProcess(LPN_FROM_LCTINDEX(ulTrimStartLctIndex), ((ulTrimEndLctIndex - ulTrimStartLctIndex) * LPN_PER_BUF));
        }

        // reset trim start and end LBA
        g_ulTrimStartLBA = INVALID_8F;
        g_ulTrimEndLBA = INVALID_8F;

        return TRUE;
    }
    else
    {
#ifndef LCT_VALID_REMOVED
        // clear the LCT valid bit for LCTs to be trimmed
        L1_MarkLCTValid(ulTrimStartLctIndex, (ulTrimStartLctIndex + L1_NON_BUFF_TRIM_THS));
#endif

        // directly trim the LPNs
        if(TRUE == L2_IsBootupOK() && FALSE == g_ulPMTFlushing)
        {
            L2_TrimReqProcess(LPN_FROM_LCTINDEX(ulTrimStartLctIndex), (L1_NON_BUFF_TRIM_THS * LPN_PER_BUF));
        }

        // move the start LCT for the current trim operation
        ulTrimStartLctIndex += L1_NON_BUFF_TRIM_THS;
    }
#endif

#define CHECK_VALID

    // reset both actual start and end LCT
    ulTrimActualStartLctIndex = INVALID_8F;
    ulTrimActualEndLctIndex = INVALID_8F;

    // set the trim bit of the LCTs with valid bit set to TRUE
    while (ulTrimStartLctIndex < ulTrimEndLctIndex)
    {
        if((LCTMSK_FROM_INDEX(ulTrimStartLctIndex) == 0) && ((ulTrimStartLctIndex + DWORD_BIT_SIZE) <= ulTrimEndLctIndex))
        {
            // process a LCT group 
            U32 ulCurrentLctTag = LCTTAG_FROM_INDEX(ulTrimStartLctIndex);

#ifndef LCT_VALID_REMOVED
            // get the valid bitmap for the current LCT group
            U32 ulValidBitmap = g_pulLCTValid[ulCurrentLctTag];

            // calculate the number of valid bits in the current LCT group
            U8 ucValidBitCount = HAL_POPCOUNT(ulValidBitmap);
#endif

#ifdef CHECK_VALID
            // set the trim bit of a LCT only if its valid bit is TRUE
#ifdef LCT_VALID_REMOVED
            g_pulTrim[ulCurrentLctTag] |= 0xFFFFFFFF;
#else
            g_pulTrim[ulCurrentLctTag] |= ulValidBitmap;
#endif

            // update the actual start LCT
#ifdef LCT_VALID_REMOVED
            if(ulTrimActualStartLctIndex == INVALID_8F)
#else
            if((ulTrimActualStartLctIndex == INVALID_8F) && (ucValidBitCount != 0))
#endif
            {
                ulTrimActualStartLctIndex = ulTrimStartLctIndex;
            }

            // update the actual end LCT
#ifndef LCT_VALID_REMOVED
            if(ucValidBitCount != 0)
            {
#endif
                ulTrimActualEndLctIndex = ulTrimStartLctIndex + DWORD_BIT_SIZE;
#ifndef LCT_VALID_REMOVED
            }
#endif
#else
            g_pulTrim[ulCurrentLctTag] = INVALID_8F;

            // update the actual start LCT
            if(ulTrimActualStartLctIndex == INVALID_8F)
            {
                ulTrimActualStartLctIndex = ulTrimStartLctIndex;
            }

            // update the actual end LCT
            ulTrimActualEndLctIndex = ulTrimStartLctIndex + DWORD_BIT_SIZE;
#endif

#ifndef LCT_VALID_REMOVED
            // clear all valid bit
            g_pulLCTValid[ulCurrentLctTag] = 0;
#endif

            // increase ulTrimStartLctIndex
            ulTrimStartLctIndex += DWORD_BIT_SIZE;

            // update the number of pending trim LCT count
#ifdef LCT_VALID_REMOVED
            g_ulPendingTrimLctCount += 32;
#else
            g_ulPendingTrimLctCount += ucValidBitCount;
#endif
        }
        else
        {
#ifdef CHECK_VALID
#ifndef LCT_VALID_REMOVED
            // set the trim bit of a LCT only if its valid bit is TRUE
            if(L1_GetLCTValid(ulTrimStartLctIndex) == TRUE)
            {
#endif
                L1_SetLCTTrim(ulTrimStartLctIndex, TRUE);

                // update the actual start LCT
                if(ulTrimActualStartLctIndex == INVALID_8F)
                {
                    ulTrimActualStartLctIndex = ulTrimStartLctIndex;
                }

                // update the actual end LCT
                ulTrimActualEndLctIndex = ulTrimStartLctIndex + 1;

                // update the number of pending trim LCT count
                g_ulPendingTrimLctCount++;
#ifndef LCT_VALID_REMOVED
            }
#endif
#else
            L1_SetLCTTrim(ulTrimStartLctIndex, TRUE);

            // update the actual start LCT
            if(ulTrimActualStartLctIndex == INVALID_8F)
            {
                ulTrimActualStartLctIndex = ulTrimStartLctIndex;
            }

            // update the actual end LCT
            ulTrimActualEndLctIndex = ulTrimStartLctIndex + 1;
#endif

#ifndef LCT_VALID_REMOVED
            // clear the LCT valid bit
            L1_SetLCTValid(ulTrimStartLctIndex, FALSE);
#endif

            // increase ulTrimStartLctIndex
            ulTrimStartLctIndex++;
        }
    } // while (ulTrimStartLctIndex < ulTrimEndLctIndex)

    // g_ulTrimSearchRangeStartLct records the first LCT in the system that needs to be trimmed
    // both g_ulTrimSearchRangeStartLct and g_ulTrimSearchRangeEndLct are used for speeding up the process of finding
    // pending trim LCTs
    
    if((ulTrimActualStartLctIndex != INVALID_8F) && (ulTrimActualEndLctIndex != INVALID_8F))
    {
        // update g_ulTrimSearchRangeStartLct
        if(g_ulTrimSearchRangeStartLct > ulTrimActualStartLctIndex)
        {
            g_ulTrimSearchRangeStartLct = ulTrimActualStartLctIndex;
        }

        // update g_ulTrimSearchRangeEndLct
        if(g_ulTrimSearchRangeEndLct < ulTrimActualEndLctIndex)
        {
            g_ulTrimSearchRangeEndLct = ulTrimActualEndLctIndex;
        }
    }

    // reset start and end LBA of the current trim operation
    g_ulTrimStartLBA = INVALID_8F;
    g_ulTrimEndLBA = INVALID_8F;

    return TRUE;
#endif
}
