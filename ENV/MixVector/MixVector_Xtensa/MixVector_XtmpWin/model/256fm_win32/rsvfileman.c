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
#include "hfmid.h"
#include "sim_flash_config.h"
void Init_FMR(unsigned long rsvFile, int nPgeSize)
{
    int nIndex = 0;
    __int64 rsvFileSzie = (__int64)(((__int64)rsvFile) * 1024 * 1024 * 1024I64);
    gFMR.nHWRsvIndex = rsvFileSzie / nPgeSize;
    gFMR.nTop = gFMR.nHWRsvIndex;
    gFMR.pRsvIndex = (unsigned long*)malloc(gFMR.nHWRsvIndex * sizeof(unsigned long));
    for(nIndex = 0; nIndex < gFMR.nHWRsvIndex; nIndex++)
        gFMR.pRsvIndex[nIndex] = gFMR.nHWRsvIndex - nIndex - 1;
}
unsigned long Get_FMR()
{
    if(gFMR.nTop == 0)
    {
        printf("rsv file is out of control\n");
        DBG_Getch();
    }
    gFMR.nTop--;
    return gFMR.pRsvIndex[gFMR.nTop];
}

void Set_FMR(unsigned long rsvFIndex)
{
    if(gFMR.nTop == gFMR.nHWRsvIndex)
        return;
    gFMR.pRsvIndex[gFMR.nTop] = rsvFIndex;
    gFMR.nTop++;
}

void Un_init_FMR()
{
    free(gFMR.pRsvIndex);
}
