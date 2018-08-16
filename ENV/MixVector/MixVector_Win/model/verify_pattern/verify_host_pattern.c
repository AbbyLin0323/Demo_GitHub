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
Filename    :Verify_host_pattern.c
Version     :Ver 1.0
Author      :Brookewang
Date        :2012.07.11
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "model_common.h"
#include "HAL_MemoryMap.h"
#include "AHCI_ControllerInterface.h"
#include "AHCI_HostModelVar.h"
#include "HostInc.h"
#include "Verify_host_pattern.h"
#include "HostModel.h"
#include "model_config.h"
#include "MixVector_HostApp.h"

extern CMD_TABLE g_tHostCmdTable[];

U8 g_SimHostFunction = 0;//sim_host_function_code

#ifdef NO_THREAD
void sim_mix_clear_host_transbyte(U8 slot)
{
	g_tHostCmdTable[slot].nTransferByte = 0;
    g_tHostCmdTable[slot].nTransferLba = 0;
}
BOOL sim_mix_vector()
{
    U8 ucCmdSlot;
    static U8 l_ucCurSlot = 0;

    for (ucCmdSlot = 0; ucCmdSlot < MAX_SLOT_NUM; ucCmdSlot++)
    {
        if (TRUE == MV_CmdSlotIsIdle(l_ucCurSlot))
        {
            AHCI_HostCClearCmdFinishFlag(l_ucCurSlot);
            g_tHostCmdTable[l_ucCurSlot].nTransferByte = 0;
            g_tHostCmdTable[l_ucCurSlot].nTransferLba = 0;
            MV_SendCmd( l_ucCurSlot );
            DBG_Printf("send comamnd on tag %d\n", l_ucCurSlot);
            
            l_ucCurSlot++;
            if (MAX_SLOT_NUM == l_ucCurSlot)
            {
                l_ucCurSlot = 0;
            }

            //only send one command in every loop
            return TRUE;
        }

        l_ucCurSlot++;
        if (MAX_SLOT_NUM == l_ucCurSlot)
        {
            l_ucCurSlot = 0;
        }
    }

    return FALSE;
}
#else
BOOL sim_mix_vector()
{
    U32 i;
    DWORD ulVal;
    static U8 ucCmdIndex = 0;

    for( i = 0; i < g_ucCmdNum; i++)
    {
        g_hCmdCompletionEventTable[ i ] = CreateEvent( NULL, TRUE, TRUE, NULL );
    }

    while( 1 )
    {
        ulVal = WaitForMultipleObjects( g_ucCmdNum,
            g_hCmdCompletionEventTable,
            FALSE,
            INFINITE );

        if( ( ulVal >= WAIT_OBJECT_0 ) && ( ulVal < ( WAIT_OBJECT_0 + g_ucCmdNum ) ) )
        {
            ResetEvent( g_hCmdCompletionEventTable[ ulVal - WAIT_OBJECT_0 ] );
            MV_SendCmd( (U8)( ulVal - WAIT_OBJECT_0 ) );
            DBG_Printf("send comamnd on tag %d\n", ulVal - WAIT_OBJECT_0);
        }
        else
        {
            DBG_Break();
        }
    }

    for( i = 0; i < g_ucCmdNum; i++)
    {
        CloseHandle( g_hCmdCompletionEventTable[ i ] );
    }

    return FALSE;
}
#endif
