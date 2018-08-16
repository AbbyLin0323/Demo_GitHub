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


#include "hfmid.h"

int Empty(PBUF_MANAGE_RSV pBMR)
{
    if(pBMR->ulCurNum == 0)
        return EMPTY;
    return NOT_EMPTY_FULL;
}
unsigned long  GetOne(PBUF_MANAGE_RSV pBMR)
{
    unsigned long ulIndex;
    if(EMPTY == Full(pBMR))
        return NO_INDEX;
    else
    {
        ulIndex = pBMR->pBuf[pBMR->nHead];
        pBMR->nHead++;
        if(pBMR->nHead == pBMR->nTotal)
            pBMR->nHead = 0;
        pBMR->ulCurNum++;
        return ulIndex;
    }
}

int RsyOne(PBUF_MANAGE_RSV pBMR, unsigned long ulIndex)
{
    if(FULL == Empty(pBMR))
        return NO_INDEX;
    else{
        pBMR->pBuf[pBMR->nTail] = ulIndex;
        pBMR->nTail++;
        if(pBMR->nTail == pBMR->nTotal)
            pBMR->nTail = 0;
        pBMR->ulCurNum--;
    }
    return 1;
}

int Full(PBUF_MANAGE_RSV pBMR)
{
    if(pBMR->ulCurNum == pBMR->nTotal)
        return FULL;
    return NOT_EMPTY_FULL;
}