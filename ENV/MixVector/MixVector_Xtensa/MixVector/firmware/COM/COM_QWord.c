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
Filename     :  COM_Qword.c                                     
Version      :  Ver 1.0                                               
Date         :  20140611                                     
Author       :  Gavin

Description: 
    implement ADD/SUB/CMP operation for U64 variable

Modification History:
20140611     gavinyin     001 create file
****************************************************************************/
#include "BaseDef.h"

/*------------------------------------------------------------------------------
Name: COM_QwAddDw
Description: 
    calc result of qword + dword.
Input Param:
    QWORD *pQwIn: pointer to input qword
    U32 ulDwIn: input dword
Output Param:
    QWORD *pQwOut: pointer to output(result) qword
Return Value:
    void
Usage:
    common interface for qword + dword operation in FW
History:
    20131014    Gavin   created
------------------------------------------------------------------------------*/

void COM_QwAddDw(QWORD *pQwIn, U32 ulDwIn, QWORD *pQwOut)
{
    pQwOut->LowDw = (U32)(pQwIn->LowDw + ulDwIn);
    
    if (INVALID_8F - ulDwIn > pQwIn->LowDw)
    {
        pQwOut->HighDw = pQwIn->HighDw;
    }
    else
    {
        pQwOut->HighDw += (U32)(pQwIn->HighDw + 1);
    }
    return;
}

/*------------------------------------------------------------------------------
Name: COM_QwSubDw
Description: 
    calc result of qword - dword.
Input Param:
    QWORD *pQwIn: pointer to input qword
    U32 ulDwIn: input dword
Output Param:
    QWORD *pQwOut: pointer to output(result) qword
Return Value:
    void
Usage:
    common interface for qword - dword operation in FW
History:
    20131014    Gavin   created
------------------------------------------------------------------------------*/

void COM_QwSubDw(QWORD *pQwIn, U32 ulDwIn, QWORD *pQwOut)
{
    if(pQwIn->LowDw >= ulDwIn)
    {
        pQwOut->HighDw = pQwIn->HighDw;
        pQwOut->LowDw = pQwIn->LowDw - ulDwIn;
    }
    else
    {
        pQwOut->HighDw = pQwIn->HighDw - 1;// assume: pQwIn->HighDw > 0
        pQwOut->LowDw = INVALID_8F - (ulDwIn - pQwIn->LowDw) + 1;
    }
    return;
}

/*------------------------------------------------------------------------------
Name: COM_QwCmp
Description: 
    compare two qword.
Input Param:
    QWORD *pQwA: pointer to input qword A
    QWORD *pQwB: pointer to input qword B
Output Param:
    none
Return Value:
    BOOL: TRUE if A >= B, else FALSE
Usage:
    common interface for qword compare operation in FW
History:
    20131014    Gavin   created
------------------------------------------------------------------------------*/

BOOL COM_QwCmp(QWORD *pQwA, QWORD *pQwB)
{
    if(pQwA->HighDw > pQwB->HighDw)
    {
        return TRUE;
    }
    else if(pQwA->HighDw < pQwB->HighDw)
    {
        return FALSE;
    }
    else
    {
        return ((pQwA->LowDw >= pQwB->LowDw) ? TRUE : FALSE);
    }
}

/* end of file COM_Qword.c */ 
