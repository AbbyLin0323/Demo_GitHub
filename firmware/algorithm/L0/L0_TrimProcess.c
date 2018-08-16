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
*******************************************************************************/


#include "BaseDef.h"
#include "HAL_MultiCore.h"
#include "L0_Config.h"
#include "L0_Interface.h"
#include "L0_TrimProcess.h"

extern U32 g_ulATARawBuffStart;
extern U32 g_ulSysLBAMax;
extern U32 g_ulSubsysNum;
extern U32 g_ulSubsysNumBits;

LOCAL void L0_TrimSegIntlvToSubsys(const U32 ulSegStart, const U32 ulSegLen, PLBA_LONGENTRY pSubsysLBA);
LOCAL U32 L0_TrimIntlvFirstLane(U32 ulSegStart, U32 ulSegLen, PSUBSYSRANGE_TABLE pSubRng);
LOCAL U32 L0_TrimIntlvLastLane(U32 ulSegStart, U32 ulSegLen, PSUBSYSRANGE_TABLE pSubRng);

U32 L0_TrimProcLBAEntry(U32 ulStartEntry, U32 ulMaxEntry, PLBA_LONGENTRY pSegLBA)
{
    PLBA_ENTRY pCurrLBAEntry;
    U32 ulProcessedEntryNum;
    U32 ulCurrSegStart, ulCurrSegEnd;
    U32 ulCurrEntryStart, ulCurrEntryEnd;

    if (ulStartEntry > ulMaxEntry)
    {
        return 0;
    }

    ulProcessedEntryNum = 0;
    pCurrLBAEntry = (PLBA_ENTRY)g_ulATARawBuffStart + ulStartEntry;
    ulCurrSegStart = pCurrLBAEntry->ulLBALow;
    ulCurrSegEnd = ulCurrSegStart + pCurrLBAEntry->bsRgLen - 1;

    do
    {
#ifdef HOST_SATA
        if (0 == pCurrLBAEntry->bsRgLen)
        {
            /* Entire range entry list ended. */
            break;
        }
#else
        if (0 != pCurrLBAEntry->bsRgLen)
#endif
        {
            ulCurrEntryStart = pCurrLBAEntry->ulLBALow;
            ulCurrEntryEnd = ulCurrEntryStart + pCurrLBAEntry->bsRgLen - 1;

            if (ulCurrEntryEnd > g_ulSysLBAMax)
            {
                /* Invalid entry detected. */
                return INVALID_8F;
                break;
            }

            if (((ulCurrEntryEnd + 1) < ulCurrSegStart) || ((ulCurrSegEnd + 1) < ulCurrEntryStart))
            {
                /* Segment range has no overlapping with current LBA range in entry. */
                /* This indicates a new segment starts. */
                break;
            }

            else
            {
                /* Merges current segment with the LBA range indicated by current LBA entry. */
                if (ulCurrEntryStart < ulCurrSegStart)
                {
                    ulCurrSegStart = ulCurrEntryStart;
                }

                if (ulCurrEntryEnd > ulCurrSegEnd)
                {
                    ulCurrSegEnd = ulCurrEntryEnd;
                }
            }
        }

        ulStartEntry++;
        pCurrLBAEntry++;
        ulProcessedEntryNum++;
    } while (ulStartEntry <= ulMaxEntry);

    if (0 != ulProcessedEntryNum)
    {
        /* A valid LBA segment has been processed. We shall update the result. */
        pSegLBA->ulStartLBA = ulCurrSegStart;
        pSegLBA->ulRegionLen = ulCurrSegEnd + 1 - ulCurrSegStart;
    }

    return ulProcessedEntryNum;
}


void L0_TrimProcSegment(const LBA_LONGENTRY *pSegLBA, PLBA_LONGENTRY pSubsysLBA)
{
    U32 ulSegStartLBA, ulSegSecLen;

    ulSegStartLBA = pSegLBA->ulStartLBA;
    ulSegSecLen = pSegLBA->ulRegionLen;

    if (1 == g_ulSubsysNum)
    {
        /* In a single-subsystem system environment... */
        pSubsysLBA[0].ulStartLBA = pSegLBA->ulStartLBA;
        pSubsysLBA[0].ulRegionLen = pSegLBA->ulRegionLen;
    }

    else
    {
        /* For a system with multiple subsystem existing, we shall calculate for the interleaving. */
        L0_TrimSegIntlvToSubsys(ulSegStartLBA, ulSegSecLen, pSubsysLBA);
    }

    return;
}

LOCAL void L0_TrimSegIntlvToSubsys(const U32 ulSegStart, const U32 ulSegLen, PLBA_LONGENTRY pSubsysLBA)
{
    /* For a system containing multiple subsystems:
         Some synonyms and concepts:
         1. Sector - the smallest logic block allocatable in a storage;

         2. LCT - a larger block whose size is matching our buffer size (32KB currently).
                       Our storage space is interleaved in LCT size to each subsystem.

         3. Lane - A stripe composed of a bundle of contiguous LCTs. It contains one LCT from each subsystem.

         In a system that is made up of four subsystems:
         Lane 0: |LCT0 LCT1 LCT2 LCT3|
         Lane 3: |LCT12 LCT13 LCT14 LCT15|
         LCT0/4/8/12... belong to Subsystem 0;
         LCT1/5/9/13... belong to Subsystem 1;
         ... ...

         Therefore a single LBA address for the system can be decomposed into the following segments.
         |31 - 9| |8 - 7| |6 - 0|
          ------   -----   -----
            |               |            |--->  Offset in LCT
            |               |--------->  Subsystem Number (as LCT offset in lane)
            |---------------->   Lane number
                           -----------
                             |
                             |--------->  Offset in lane
          ------------
            |
            |---------------->  LCT number

            4. First lane - The lane in which the specified LBA range starts.
                                      A given LBA range may only hit the rear part of the whole lane.
            Lane 4: |LCT16|LCT17|LCT18|LCT19|
                                                     -----------
                                                        |<------->|Start LBA = 0x4A0, Sector Length = 0xA0

            5. Last lane - The lane in which the specified LBA range ends.
                                      A given LBA range may only hit the front part of the whole lane.
            Lane 17: |LCT68|LCT69|LCT70|LCT71|
                            |<----->|Start LBA = 0x1080, Sector Length = 0x100
    */

    /* Intermediate results and recorders declaration: */
    SUBSYSRANGE_TABLE a_tSubRng[SUBSYSTEM_NUM_MAX];

    U32 ulCurrSubsysNum;
    U32 ulCmplSecLen;
    U32 ulSecLenInWholeLanes;;

    /* 1. Clears total calculated sectors of all subsystem to 0 first. */
    for (ulCurrSubsysNum = 0; ulCurrSubsysNum < g_ulSubsysNum; ulCurrSubsysNum++)
    {
        a_tSubRng[ulCurrSubsysNum].ulSecLen = 0;
    }
    
    /* 2. Analyzes the first lane of given LBA range. */
    ulCmplSecLen = L0_TrimIntlvFirstLane(ulSegStart, ulSegLen, a_tSubRng);

    /* 3. Analyzes the last lane of given LBA range. */
    if (ulCmplSecLen < ulSegLen)
    {
        ulCmplSecLen += L0_TrimIntlvLastLane(ulSegStart, ulSegLen, a_tSubRng);

        /* 4. Accumulates the left range that are lane-aligned. */
        if (ulCmplSecLen < ulSegLen)
        {
            ulSecLenInWholeLanes = ulSegLen - ulCmplSecLen;

            for (ulCurrSubsysNum = 0; ulCurrSubsysNum < g_ulSubsysNum; ulCurrSubsysNum++)
            {
                a_tSubRng[ulCurrSubsysNum].ulSecLen += (ulSecLenInWholeLanes >> g_ulSubsysNumBits);
            }
        }
    }

    /* 5. Updates target LBA range data. */
    for (ulCurrSubsysNum = 0; ulCurrSubsysNum < g_ulSubsysNum; ulCurrSubsysNum++)
    {
        pSubsysLBA[ulCurrSubsysNum].ulRegionLen = a_tSubRng[ulCurrSubsysNum].ulSecLen;

        if (0 != a_tSubRng[ulCurrSubsysNum].ulSecLen)
        {
            pSubsysLBA[ulCurrSubsysNum].ulStartLBA = (a_tSubRng[ulCurrSubsysNum].ulStartLaneNum << SEC_PER_BUF_BITS)
                + a_tSubRng[ulCurrSubsysNum].ulStartOffsetInLCT;
        }
    }

    return;
}

LOCAL U32 L0_TrimIntlvFirstLane(U32 ulSegStart, U32 ulSegLen, PSUBSYSRANGE_TABLE pSubRng)
{
    U32 ulSegStartLane;
    U32 ulSegStartSubsysNum;
    U32 ulCurrSubsysNum;
    U32 ulCurrLCTOffset, ulCurrLCTLen;
    U32 ulFirstLaneSecLen, ulFirstLaneSecLeft;

    ulSegStartLane = L0M_GET_LANE_FROM_LBA(ulSegStart, g_ulSubsysNumBits);
    ulSegStartSubsysNum = L0M_GET_SUBSYSID_FROM_LBA(ulSegStart, g_ulSubsysNumBits);
    ulFirstLaneSecLen = SEC_PER_LANE(g_ulSubsysNumBits)
        - L0M_GET_OFFSET_IN_LANE_FROM_LBA(ulSegStart, g_ulSubsysNumBits);

    if (ulFirstLaneSecLen > ulSegLen)
    {
        ulFirstLaneSecLen = ulSegLen;
    }

    ulFirstLaneSecLeft = ulFirstLaneSecLen;

    /* For each LCT inside the start lane: */
    for (ulCurrSubsysNum = 0;
        ((0 != ulFirstLaneSecLeft) && (ulCurrSubsysNum < g_ulSubsysNum));
        ulCurrSubsysNum++)
    {
        if (ulCurrSubsysNum < ulSegStartSubsysNum)
        {
            /* For "blank" LCTs before segment starting position. */
            /* 1) Start lane number shall be the next lane. */
            pSubRng[ulCurrSubsysNum].ulStartLaneNum = ulSegStartLane + 1;

            /* 2) Starting offset in the lane shall be 0 if it exists. */
            pSubRng[ulCurrSubsysNum].ulStartOffsetInLCT = 0;

            /* 3) Clears sector length accumulated in this stage to 0. */
            /* 
                 ulCurrSubsysSecInLCT = 0;
                 pSubRng[ulCurrSubsysNum].ulSecLen += ulCurrSubsysSecInLCT;
                 ulFirstLaneSecLeft -= ulCurrSubsysSecInLCT;
            */
        }

        else
        {
            /* For the "hit" LCTs after segment starting position. */
            /* 1) Start lane number shall be the current lane. */
            pSubRng[ulCurrSubsysNum].ulStartLaneNum = ulSegStartLane;

             /* 2) Starting offset in the LCT shall be segment start offset for the "segment start" LCT,
                 and 0 for all LCTs following it. */
             if (ulCurrSubsysNum == ulSegStartSubsysNum)
             {
                ulCurrLCTOffset = L0M_GET_OFFSET_IN_LCT_FROM_LBA(ulSegStart);
             }

             else
             {
                ulCurrLCTOffset = 0;
             }

            pSubRng[ulCurrSubsysNum].ulStartOffsetInLCT = ulCurrLCTOffset;

            /* 3) Calculates sector length accumulated in this stage. */
            ulCurrLCTLen = SEC_PER_BUF - ulCurrLCTOffset;

            if (ulCurrLCTLen > ulFirstLaneSecLeft)
            {
                ulCurrLCTLen = ulFirstLaneSecLeft;
            }

            pSubRng[ulCurrSubsysNum].ulSecLen += ulCurrLCTLen;
            ulFirstLaneSecLeft -= ulCurrLCTLen;
        }
    }

#ifdef L0_DBG_STATE
        ASSERT(0 == ulFirstLaneSecLeft);
#endif

    return ulFirstLaneSecLen;
}

LOCAL U32 L0_TrimIntlvLastLane(U32 ulSegStart, U32 ulSegLen, PSUBSYSRANGE_TABLE pSubRng)
{
    U32 ulSegLastLBA;
    U32 ulSegLastSubsysNum;
    U32 ulCurrSubsysNum;
    U32 ulCurrLCTLen;
    U32 ulLastLaneSecLen, ulLastLaneSecLeft;

    ulSegLastLBA = ulSegStart + ulSegLen - 1;
    ulSegLastSubsysNum = L0M_GET_SUBSYSID_FROM_LBA(ulSegLastLBA, g_ulSubsysNumBits);
    ulLastLaneSecLen = L0M_GET_OFFSET_IN_LANE_FROM_LBA(ulSegLastLBA, g_ulSubsysNumBits) + 1;

    ulLastLaneSecLeft = ulLastLaneSecLen;

    /* For each LCT inside the last lane: */
    for (ulCurrSubsysNum = 0; ulCurrSubsysNum <= ulSegLastSubsysNum; ulCurrSubsysNum++)
    {
        /* Sector length is the full LCT for all LCTs before the last LCT. */
        if (ulLastLaneSecLeft > SEC_PER_BUF)
        {
            ulCurrLCTLen = SEC_PER_BUF;
        }

        /* Sector length is the full LCT for all LCTs before the last LCT. */
        else
        {
            ulCurrLCTLen = ulLastLaneSecLeft;
        }

        /* Records the sector length hit current LCT. */
        pSubRng[ulCurrSubsysNum].ulSecLen += ulCurrLCTLen;
        ulLastLaneSecLeft -= ulCurrLCTLen;
    }

#ifdef L0_DBG_STATE
    ASSERT(0 == ulLastLaneSecLeft);
#endif

    return ulLastLaneSecLen;
}

