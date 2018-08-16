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

Filename     :   Sim_HCT.c
Version      :   0.1
Date         :   2014.11.10
Author       :   NinaYang

Description:  implement the function of HCT
Others:
Modification History:
2014.11.10   create
*******************************************************************/
#include "windows.h"
#include "BaseDef.h"
#include "HAL_HCT.h"
#include "HAL_MemoryMap.h"
#include "HAL_HostInterface.h"
#ifdef HOST_NVME
#include "NVME_ControllerModel.h"
#include "NVME_HostCommand.h"
#ifdef AF_ENABLE_M
#include "HAL_NVMECFGEX.h"
#endif
#else
#include "AHCI_HostModelVar.h"
#endif
#include "model_common.h"
#include "Sim_HCT.h"
#include "system_statistic.h"
#include "COM_Memory.h"

#ifdef AF_ENABLE_M
extern volatile NVME_CFG_EX *g_pNVMeCfgExReg;
#endif
//define for HCT's base address
U32 g_ulCmdHeaderBaseAddr;
U32 g_ulCFisBaseAddr;

U32 g_ulCSTBaseAddr;
U32 g_ulHCTSramStartAddr;


//define for FCQ
U8 g_uFCQReadPointer;
U8 g_uFCQWritePointer;
U8 g_uFCQEmptyCnt;

//define for WBQ
const U8 g_uWBQDepth = WBQ_N;
U8 g_uWBQWritePointer[MAX_SLOT_NUM];
//U8 g_uWBQReadPointer[MAX_SLOT_NUM];

U8 g_uCMDFinishFlag[MAX_SLOT_NUM];
//U8 g_uHostCState = AHCI_HOSTC_STATE_FCQ;

// FCQ registers critical section
CRITICAL_SECTION g_csFCQCriticalSection;

// CST registers critical section
CRITICAL_SECTION g_csCSTCriticalSection;

// AHCI registers (except FCQ, WBQ, CST) critical section
CRITICAL_SECTION g_csRegCriticalSection;

HANDLE  g_hControlFCQThread;
HANDLE  g_hContrlWBQThread;

#ifdef AF_ENABLE_M
HANDLE  g_hContrlAFThread;
HANDLE  g_hContrlAFEvent;
BOOL g_bAFThreadExit;
extern DWORD WINAPI SIM_HostCAFThread(LPVOID p);
#endif


// indicate FCQ thread to terminate.
BOOL g_bFCQThreadExit;
// indicate FCQ thread to handle an FCQ entry or terminate.
HANDLE  g_hContrlFCQEvent;


volatile HCT_FCQ_REG *g_pFCQReg = 0;
volatile HCT_WBQ_REG *g_pWBQReg = 0;
extern volatile HCT_CONTROL_REG *g_pHCTControlReg;
PHCT_BAINC_REG g_pHCTBaIncReg = 0;


HCT_MGR g_tHCTMgr;
U32 g_ulSlotFinishTime[MAX_SLOT_NUM];
U8 g_ucSlotWBQOffset[MAX_SLOT_NUM];
BOOL g_bSlotWBQActive[MAX_SLOT_NUM];
U32 g_ulTimeCnt = 0;
BOOL g_bTimeCount = FALSE;

LOCAL HCT_FCQ_WBQ *l_pFCQAddr = NULL;

LOCAL DebugCST_Trace l_tDebugCSTTrace[MAX_SLOT_NUM] = {{0}};
LOCAL DebugCST_Trace l_tDebugCSTTraceBackup[MAX_SLOT_NUM] = {{0}};


LOCAL void HCTDBG_RecordCST(U32 ulSlotNum, U8 ucCSTValue)
{
#ifdef HOST_NVME
    U8 ucPtr;

    if (ulSlotNum > MAX_SLOT_NUM)
    {
        DBG_Printf("slot num overflow\n");
        DBG_Break();
    }
    ucPtr = l_tDebugCSTTrace[ulSlotNum].ucPtr;

    if (ucPtr >= CST_STATUS_COUNT)
    {
        DBG_Printf("slot = %d, ucPrt= 0x%x, CSTValue = %d", ulSlotNum, ucPtr, ucCSTValue);
        DBG_Getch();
    }

    l_tDebugCSTTrace[ulSlotNum].tCSTValue[ucPtr]= ucCSTValue;
    l_tDebugCSTTrace[ulSlotNum].ucPtr++;

    if (l_tDebugCSTTrace[ulSlotNum].ucPtr >= CST_STATUS_COUNT)
    {
        l_tDebugCSTTrace[ulSlotNum].ucPtr = 0;
    }

#endif
}

void HCTDBG_ClearCST(U32 ulSlotNum)
{
    U8 ucPtr;
    if (ulSlotNum > MAX_SLOT_NUM)
    {
        DBG_Printf("slot num overflow\n");
        DBG_Break();
    }

    memcpy ((void *)&l_tDebugCSTTraceBackup[ulSlotNum],
                (void *)&l_tDebugCSTTrace[ulSlotNum],
                sizeof(DebugCST_Trace));

    for(ucPtr = 0; ucPtr < CST_STATUS_COUNT; ucPtr++)
    {
        l_tDebugCSTTrace[ulSlotNum].tCSTValue[ucPtr] = 0;
    }

    l_tDebugCSTTrace[ulSlotNum].ucPtr = 0;
}

U32 HCT_GetCST(U8 CSTID, U8* pCurrentValue)
{
    U32 CSTAddr = g_ulCSTBaseAddr + CSTID;
    if (pCurrentValue != NULL)
    {
    *pCurrentValue = *((char*)CSTAddr);
    }

    return *((char*)CSTAddr);
}


void HCT_SearchCST()
{
#ifdef SIM_XTENSA
    PHCT_CSCS_REG pHctCSTReg = (PHCT_CSCS_REG)GetVirtualAddrInLocalMem((U32)&rHCT_CSCS_REG);
#else
    PHCT_CSCS_REG pHctCSTReg = (PHCT_CSCS_REG)&rHCT_CSCS_REG;
#endif

    U32 CSTAddr = 0;
    U32 ulCSTIndex = 0;
    static U32 ulCSTNextIndexRecord = 0;
    static U32 pCST = 0;


    if (0 == pCST)
    {
        pCST = g_ulCSTBaseAddr;
    }
    //EnterCriticalSection( &g_csCSTCriticalSection );
    pHctCSTReg->bsCSTRDY = 0;

    //search latter part
    pHctCSTReg->bsCSTNOID = 1;
    for (ulCSTIndex = 0; ulCSTIndex < MAX_SLOT_NUM; ulCSTIndex++)
    {
        //CSTAddr = g_ulCSTBaseAddr + ulCSTNextIndexRecord;
        if (pHctCSTReg->bsCST == (*(U8*)pCST))
        {
            pHctCSTReg->bsCSTCID = pCST - g_ulCSTBaseAddr;
            pHctCSTReg->bsCSTNOID = 0;
        }

        pCST++;
        if(pCST ==  (MAX_SLOT_NUM + g_ulCSTBaseAddr))
        {
            pCST = g_ulCSTBaseAddr;
        }

       /* ulCSTNextIndexRecord++;
        if(ulCSTNextIndexRecord == MAX_SLOT_NUM)
        {
            ulCSTNextIndexRecord = 0;
        }*/

        if(0 == pHctCSTReg->bsCSTNOID)
        {
            break;
        }
    }

    pHctCSTReg->bsCST = 0;
    pHctCSTReg->bsCSTTRI = 0;
    pHctCSTReg->bsCSTRDY = 1;

    //LeaveCriticalSection( &g_csCSTCriticalSection );
}

U32 HCT_GetDeviceAddr(U8 IDBaseEnable, U8 SN, U16 Offset, U8 ID)
{
    U32 ulDeviceAddr = 0;
    PHCT_BAINC_REG pBaseAddrInc = 0;
    U32 ulAddrOffset = 0;

    if (IDBaseEnable)
    {
        pBaseAddrInc = (PHCT_BAINC_REG)(g_pHCTBaIncReg + SN);
        ulAddrOffset = pBaseAddrInc->bsINC * ID;
    }
    else
    {
        pBaseAddrInc = (PHCT_BAINC_REG)(g_pHCTBaIncReg);
        ulAddrOffset = Offset;
    }

    ulDeviceAddr = (pBaseAddrInc->usBA & 0xFFFF) + g_ulHCTSramStartAddr + ulAddrOffset;

    return ulDeviceAddr;
}

void HCT_SetCSTByFw(U8 CSTID, U8 CurrentValue, U8 NextValue)
{
    U32 CSTAddr;
    U8 CSTValue;

    //EnterCriticalSection( &g_csCSTCriticalSection );

    // set cst value
    CSTAddr = g_ulCSTBaseAddr + CSTID;
    CSTValue = *((char*)CSTAddr);
    *((char*)CSTAddr) = NextValue;

    if (CSTValue != CurrentValue)
    {
        DBG_Printf("CST Value error in HCT_SetCSTByFw()\n");
        DBG_Break();
    }

    //RecLogFile("FW set CST[%d] from %d to %d\n", CSTID, CSTValue, NextValue);
    HCTDBG_RecordCST(CSTID, NextValue);

    //set active WBQ flag
    if ( g_pWBQReg->bsWBQTRI == NextValue )
    {
        SIM_HostCWBQStateTrigger( CSTID );
    }

    //LeaveCriticalSection( &g_csCSTCriticalSection );

}

void HCT_SetCSTByFCQ(U8 CSTID, U8 CurrentValue, U8 NextValue)
{
    U32 CSTAddr;
    U8 CSTValue;

    //EnterCriticalSection( &g_csCSTCriticalSection );

    CSTAddr = g_ulCSTBaseAddr + CSTID;
    CSTValue = *((char*)CSTAddr);

    if(CSTValue != CurrentValue)
    {
        DBG_Printf("CSTID 0x%x CSTValue %d CurrentValue %d\n",CSTID,CSTValue,CurrentValue);
        DBG_Break();
    }

    //RecLogFile("FCQ set CST[%d] from %d to %d\n", CSTID, CSTValue, NextValue);
    HCTDBG_RecordCST(CSTID, NextValue);

    *((char*)CSTAddr) = NextValue;
    if ( g_pWBQReg->bsWBQTRI == NextValue )
    {
        // temporarily assume FCQ thread doesn't get here.
        // by Charles Zhou
        DBG_Break();
    }

    //LeaveCriticalSection( &g_csCSTCriticalSection );
}

void HCT_SetCSTByWBQ(U8 CSTID, U8 CurrentValue, U8 NextValue)
{
    U32 CSTAddr;
    U8 CSTValue;

    //EnterCriticalSection( &g_csCSTCriticalSection );

    CSTAddr = g_ulCSTBaseAddr + CSTID;
    CSTValue = *((char*)CSTAddr);

    /*if (CSTValue != CurrentValue)
    {
        DBG_Printf("CST value error!\n");
        DBG_Break();
    }*/

    //RecLogFile("WBQ set CST[%d] from %d to %d\n", CSTID, CSTValue, NextValue);
    HCTDBG_RecordCST(CSTID, NextValue);

    *((char*)CSTAddr) = NextValue;

    if ( g_pWBQReg->bsWBQTRI == NextValue )
    {
        DBG_Break();
    }

    //LeaveCriticalSection( &g_csCSTCriticalSection );
    return;
}

void HCT_CSTInit()
{
    U32 i;
    for(i = 0; i < MAX_SLOT_NUM/sizeof(U32); i++)
    {
        *(U32*)(g_ulCSTBaseAddr + i*sizeof(U32)) = 0;
    }
}

void HCT_ModelInit()
{
    U32 i;
    InitializeCriticalSection(&g_csFCQCriticalSection);
    InitializeCriticalSection(&g_csCSTCriticalSection);
    InitializeCriticalSection(&g_csRegCriticalSection);
    InitializeCriticalSection( &g_tHCTMgr.WBQCriticalSection );

#ifdef SIM
    g_ulCSTBaseAddr = HCT_CS_REG;
    g_ulHCTSramStartAddr = HCT_SRAM_BASE;

    // get Reg addr
    g_pFCQReg = (HCT_FCQ_REG *)&rHCT_FCQ_REG;
    g_pWBQReg = (HCT_WBQ_REG *)&rHCT_WBQ_REG;

    g_pHCTControlReg = (HCT_CONTROL_REG *)&rHCT_CONTROL_REG;
    g_pHCTBaIncReg = (PHCT_BAINC_REG)HCT_DSBA_REG_BASE;
#ifdef AF_ENABLE_M
    g_pNVMeCfgExReg = (PNVME_CFG_EX)REG_BASE_EXT_NVME_AF;
#endif //AF_ENABLE_M
#else
    //g_ulCmdHeaderBaseAddr = (U32)GetVirtualAddrInLocalMem((U32)HCT_S0_BASE);
    //g_ulCFisBaseAddr = (U32)GetVirtualAddrInLocalMem((U32)HCT_S1_BASE);
    g_ulCSTBaseAddr = (U32)GetVirtualAddrInLocalMem((U32)HCT_CS_REG);
    g_ulHCTSramStartAddr = (U32)GetVirtualAddrInLocalMem((U32)HCT_SRAM_BASE);

    g_pFCQReg = (HCT_FCQ_REG *)GetVirtualAddrInLocalMem((U32)&rHCT_FCQ_REG);
    g_pWBQReg = (HCT_WBQ_REG *)GetVirtualAddrInLocalMem((U32)&rHCT_WBQ_REG);
    g_pHCTControlReg = (HCT_CONTROL_REG *)GetVirtualAddrInLocalMem((U32)&rHCT_CONTROL_REG);
    g_pHCTBaIncReg = (PHCT_BAINC_REG)GetVirtualAddrInLocalMem((U32)HCT_DSBA_REG_BASE);
#ifdef AF_ENABLE_M
        //Fix Me: Reserved for NVME_Ex_AF
        //g_pNVMeCfgExReg = (PNVME_CFG_EX)GetVirtualAddrInLocalMem();
#endif //AF_ENABLE_M
#endif

    // init FCQ REG
    SIM_HCTRegInit();

    for(i = 0; i < 4; i++)
    {
        g_tHCTMgr.EventTable[ i ] = CreateEvent( NULL, TRUE, FALSE, NULL );
    }

    HCT_CSTInit();

#ifdef SIM
#ifndef NO_THREAD
    g_bFCQThreadExit = FALSE;
    g_hContrlFCQEvent = CreateSemaphore( NULL, 0, 256, NULL ); 
#ifdef AF_ENABLE_M
    g_bAFThreadExit = FALSE;
    g_hContrlAFEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
#endif //AF_ENABLE_M
#endif //NO_THREAD
#endif //SIM
}
/*HCT end*/

/*FCQ begin*/

/* Should be called by AHCI firmware to activate FCQ handle thread
to handle a new FCQ entry.*/
void SIM_HostCFCQWriteTrigger()
{
    EnterCriticalSection(&g_csFCQCriticalSection);

    if (1 == g_pFCQReg->bsFCQFULL)
    {
        DBG_Printf("AHCI_HostCFCQWriteTriger: g_pFCQReg->bsFCQFULL = 1 \n");
        DBG_Break();
    }

    g_pFCQReg->bsFCQWP = g_pFCQReg->bsFCQWP + 1;
    if (MAX_FCQ == g_pFCQReg->bsFCQWP)
    {
        g_pFCQReg->bsFCQWP = 0;
    }

    g_uFCQEmptyCnt--;
    g_pFCQReg->bsFCQEMPT = 0;
    g_pFCQReg->bsFCQPUSH = 0;

    if (g_pFCQReg->bsFCQWP == g_uFCQReadPointer)
    {
        g_pFCQReg->bsFCQFULL = 1;
    }

    SIM_HostCModelTriggerFCQWrite();

    LeaveCriticalSection(&g_csFCQCriticalSection);

    return;
}

void SIM_HostCFCQReadTrigger()
{
    // if the FCQ is empty
    if ((!g_pFCQReg->bsFCQFULL) && (g_uFCQReadPointer == g_pFCQReg->bsFCQWP))
    {
        DBG_Printf("AHCI_HostCFCQWriteTriger: g_uFCQReadPointer = g_pFCQReg->bsFCQWP \n");
        DBG_Break();
    }

    g_uFCQReadPointer++;
    if (MAX_FCQ == g_uFCQReadPointer)
        g_uFCQReadPointer = 0;

    g_uFCQEmptyCnt++;
    if (MAX_FCQ == g_uFCQEmptyCnt)
        g_pFCQReg->bsFCQEMPT = 1;

    g_pFCQReg->bsFCQFULL = 0;

    return;
}

void SIM_FCQThreadHandle()
{
    U8 uFCQPointer = g_uFCQReadPointer;

    U32 ulDeviceAddr;
    LARGE_INTEGER ullHostAddr;

#ifndef IGNORE_PERFORMANCE
    U32 ulCurTime;
#endif
    EnterCriticalSection(&g_csFCQCriticalSection);
    while (1 != g_pFCQReg->bsFCQEMPT)
    {
#ifdef IGNORE_PERFORMANCE
        l_pFCQAddr = (HCT_FCQ_WBQ *)(g_pFCQReg->usFCQBA + g_ulHCTSramStartAddr); //+ g_uFCQReadPointer*sizeof(HCT_FCQ_WBQ);
        l_pFCQAddr += g_uFCQReadPointer;
        ulDeviceAddr = HCT_GetDeviceAddr(l_pFCQAddr->bsIDB,
            l_pFCQAddr->bsSN,
            l_pFCQAddr->bsOffset,
            l_pFCQAddr->bsID);

        ullHostAddr.HighPart = l_pFCQAddr->ulHostAddrHigh;
        ullHostAddr.LowPart = l_pFCQAddr->ulHostAddrLow;

        if (0 != (ullHostAddr.LowPart & 0x03))
        {
            DBG_Printf("SIM_FCQThreadHandle: Host addr_Low_part = 0x%x, don't align 1DW", ullHostAddr.LowPart);
            DBG_Getch();
        }

        if (0 != (ulDeviceAddr & 0x3))
        {
            DBG_Printf("SIM_FCQThreadHandle: local DeviceAddr = 0x%x, don't align 1DW", ulDeviceAddr);
            DBG_Getch();
        }

        if (0 != (l_pFCQAddr->bsLength & 0x3))
        {
            DBG_Printf("SIM_FCQThreadHandle: Data Length = 0x%x, don't align 1DW", l_pFCQAddr->bsLength);
            DBG_Getch();
        }

        memcpy((void*)ulDeviceAddr, (void*)ullHostAddr.QuadPart, l_pFCQAddr->bsLength);




#ifdef HOST_NVME
        NVME_FillHostCmdTable(l_pFCQAddr->bsID, l_pFCQAddr->ulHostAddrHigh, l_pFCQAddr->ulHostAddrLow, l_pFCQAddr->bsCST);
#endif

        /* Note: updating CST must be the last operation when process one FCQ
        * Because FW assume that the FCQ was totally finished when CST updated
        */
        HCT_SetCSTByFCQ(l_pFCQAddr->bsID, l_pFCQAddr->bsCST, l_pFCQAddr->bsNST);

        SIM_HostCFCQReadTrigger();
#else
        switch (l_ucFcqBusSts)
        {
        case FCQ_STS_WAIT_CMD:
            {
                l_pFCQAddr = (HCT_FCQ_WBQ *)(g_pFCQReg->usFCQBA + g_ulHCTSramStartAddr); //+ g_uFCQReadPointer*sizeof(HCT_FCQ_WBQ);
                l_pFCQAddr += g_uFCQReadPointer;
                ulDeviceAddr = HCT_GetDeviceAddr(l_pFCQAddr->bsIDB,
                    l_pFCQAddr->bsSN,
                    l_pFCQAddr->bsOffset,
                    l_pFCQAddr->bsID);

                ullHostAddr.HighPart = l_pFCQAddr->ulHostAddrHigh;
                ullHostAddr.LowPart = l_pFCQAddr->ulHostAddrLow;

                memcpy((void*)ulDeviceAddr, (void*)ullHostAddr.QuadPart, l_pFCQAddr->Length * 4);

                l_ucFcqBusSts = FCQ_STS_WAIT_BUS;
            }
            break;

        case FCQ_STS_WAIT_BUS:
            {
                EnterCriticalSection(&g_PcieHostToDevCriticalSection);
                if (PCIE_STS_FREE == l_ucPcieBusHostToDevSts)
                {
                    l_ucPcieBusHostToDevSts = PCIE_STS_BUSY;
                    l_ucFcqBusSts = FCQ_STS_PCIE_TRANS;

                    l_tFcqTimeRecord.time_start = (U32)GET_TIME();
                    l_tFcqTimeRecord.time_busy = (U32)(BUSY_TIME_HOST_TO_DEV_PER_BYTE * l_pFCQAddr->Length * 4);
                }
                LeaveCriticalSection(&g_PcieHostToDevCriticalSection);
            }
            break;

        case FCQ_STS_PCIE_TRANS:
            {
                ulCurTime = (U32)GET_TIME();
                if( (ulCurTime - l_tFcqTimeRecord.time_start) >= l_tFcqTimeRecord.time_busy)
                {
                    SIM_HostCFCQReadTrigger();
                    /* Note: updating CST must be the last operation when process one FCQ
                    * Because FW assume that the FCQ was totally finished when CST updated
                    */
                    HCT_SetCSTByFCQ(l_pFCQAddr->bsID, l_pFCQAddr->bsCST, l_pFCQAddr->bsNST);
                    l_ucFcqBusSts = FCQ_STS_WAIT_CMD;
                    EnterCriticalSection(&g_PcieHostToDevCriticalSection);
                    l_ucPcieBusHostToDevSts = PCIE_STS_FREE;
                    LeaveCriticalSection(&g_PcieHostToDevCriticalSection);
                }
            }
            break;

        default:
            break;
        }
#endif
    }
    LeaveCriticalSection(&g_csFCQCriticalSection);
    return;
}


/*
 The function is used to indicate FCQ thread to exit.
 by Charles Zhou
 */
BOOL SIM_HostCFCQThreadExit()
{
    g_bFCQThreadExit = TRUE;
    ReleaseSemaphore( g_hContrlFCQEvent, 1, NULL );
    WaitForSingleObject(g_hControlFCQThread, INFINITE);
    CloseHandle(g_hControlFCQThread);

    return TRUE;
}
/*
FCQ process thread.
*/
DWORD WINAPI SIM_HostCFCQThread(LPVOID p)
{
    while ( g_bFCQThreadExit == FALSE )
    {
        WaitForSingleObject(g_hContrlFCQEvent,INFINITE);
        SIM_FCQThreadHandle();
    }
    return 0;
}
/*FCQ end*/


void SIM_WBQWriteBack(const HCT_FCQ_WBQ *pWBQ)
{
    U32 ulDeviceAddr;
    LARGE_INTEGER ullHostAddr;

    ulDeviceAddr = HCT_GetDeviceAddr(pWBQ->bsIDB,
                                           pWBQ->bsSN,
                                           pWBQ->bsOffset,
                                           pWBQ->bsID);

    ullHostAddr.HighPart = pWBQ->ulHostAddrHigh;
    ullHostAddr.LowPart = pWBQ->ulHostAddrLow;

    if (0 != (ullHostAddr.LowPart & 0x03))
    {
        DBG_Printf("SIM_WBQWriteBack: Host addr_Low_part = 0x%x, don't align 1DW", ullHostAddr.LowPart);
        DBG_Getch();
    }

    if (0 != (ulDeviceAddr & 0x3))
    {
        DBG_Printf("SIM_WBQWriteBack: local DeviceAddr = 0x%x, don't align 1DW", ulDeviceAddr);
        DBG_Getch();
    }

    if (0 != (pWBQ->bsLength & 0x3))
    {
        DBG_Printf("SIM_WBQWriteBack: Data Length = 0x%x, don't align 1DW", pWBQ->bsLength*4);
        DBG_Getch();
    }

    memcpy((void*)ullHostAddr.QuadPart, (void*)ulDeviceAddr, pWBQ->bsLength);


}

BOOL SIM_WBQThreadHandle()
{
    DWORD ret;
    BOOL bContinue = TRUE;
    ret = WaitForMultipleObjects( 4, g_tHCTMgr.EventTable, FALSE, 0);
    switch( ret )
    {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
        ResetEvent( g_tHCTMgr.EventTable[ ret - WAIT_OBJECT_0 ] );
        SIM_HostCWBQProcessByEvent( ret - WAIT_OBJECT_0 );

        break;
    case WAIT_OBJECT_0 + 2:
        ResetEvent( g_tHCTMgr.EventTable[ ret - WAIT_OBJECT_0 ] );
        bContinue = FALSE;
        break;
    case WAIT_OBJECT_0 + 3:
        SIM_FCQThreadHandle();
    case WAIT_TIMEOUT:
        bContinue = TRUE;
        break;
    default:
        /*DBG_Break();*/
        bContinue = FALSE;
        break;
    }

    return bContinue;
}

DWORD WINAPI SIM_HostCWBQThread(LPVOID p)
{
    // by Charles Zhou
    BOOL bContinue = TRUE;
    while (bContinue)
    {
       bContinue = SIM_WBQThreadHandle();
    }
    return 0;
}

#ifdef SIM_XTENSA
void AHCI_HOSTCWBQThread_XTENSA()
{
    //only process event type = 1; data transfer done event.
    U32 ulEventType = 1;
    BOOL bContinue = TRUE;

    while (bContinue)
    {
        SIM_HostCWBQProcessByEvent(ulEventType);
        XTMP_wait(2);
    }
    return;
}

void AHCI_HOSTCFCQThread_XTENSA()
{
     BOOL bContinue = TRUE;
    while (bContinue)
    {
        //WaitForSingleObject(g_SimSDCEvent,INFINITE);
        SIM_FCQThreadHandle();

        XTMP_wait(2);
    }
    return;
}
#endif

void SIM_HCTRegInit()
{

    int i = 0;
    // init FCQ REG
    g_pFCQReg->bsFCQEMPT = 1;
    g_pFCQReg->bsFCQFULL = 0;
    g_pFCQReg->bsFCQWP = 0;
    g_pWBQReg->bsWBQTRI = 7;//SLOT_TRIGGER_WBQ;//0x6;

    g_uFCQReadPointer = 0;
    g_uFCQWritePointer = 0;
    g_uFCQEmptyCnt = MAX_FCQ;


    for(i = 0; i < MAX_SLOT_NUM; i++)
    {
        g_tHCTMgr.bsWBQOffset[ i ] = 0;
        g_tHCTMgr.ActiveWBQBitmap[i] = 0;
        g_tHCTMgr.DataDoneBitmap[i] = 0;

        g_uWBQWritePointer[i] = 0;
    }
}

void SIM_HostCModelPowerUp()
{
    g_bFCQThreadExit = FALSE;
    //g_hControlFCQThread = CreateThread(0, 0, SIM_HostCFCQThread, 0, 0, 0);
    g_hContrlWBQThread = CreateThread(0, 0, SIM_HostCWBQThread, 0, 0, 0);

#ifdef AF_ENABLE_M
    g_bAFThreadExit = FALSE;
    g_hContrlAFThread = CreateThread(0, 0, SIM_HostCAFThread, 0, 0, 0);
#endif
}

void SIM_HostCModelSchedule()
{
#ifdef NO_THREAD
    SIM_FCQThreadHandle();
    SIM_WBQThreadHandle();
#else
    //no need to trigger FCQ/WBQ thread, as the Event was fired somewhere else
    return;
#endif
}
/*
The callcer should first setup an FCQ entry and then calls this routine to indicate
HostCModel that an new FCQ arrives.

by Charles Zhou
*/
void SIM_HostCModelTriggerFCQWrite()
{
    //ReleaseSemaphore( g_hContrlFCQEvent, 1, NULL );
    SetEvent(g_tHCTMgr.EventTable[3]);
}

/*
 by Charles Zhou
 Indicate WBQ thread to handle the corresponding WBQs.
 The caller should make sure that the thread has entered WBQ critical section
 before calling.
  */
void SIM_HostCWBQStateTrigger( U8 ucCmdTag )
{
        g_tHCTMgr.ActiveWBQBitmap[ucCmdTag] = 1;
    SetEvent( g_tHCTMgr.EventTable[ 0 ] );
}
/*
 by Charles Zhou

 Once SGE completes data transfer for one command, the function
 shoule be called to indicate WBQ thread to handle the rest WBQs of the corresponding command.
 */
void SIM_HostCWBQDataDoneTrigger( U8 ucCmdTag )
{
    EnterCriticalSection( &g_tHCTMgr.WBQCriticalSection );

        g_tHCTMgr.DataDoneBitmap[ucCmdTag] = 1;
    SetEvent( g_tHCTMgr.EventTable[ 1 ] );
    LeaveCriticalSection( &g_tHCTMgr.WBQCriticalSection );
}
/*
 by Charles Zhou

 The function is used to terminate WBQ thread.
 */
BOOL SIM_HostCWBQThreadExit()
{
    SetEvent( g_tHCTMgr.EventTable[ 2 ] );
    WaitForSingleObject( g_hContrlWBQThread, INFINITE );
    CloseHandle(g_hContrlWBQThread);
    return TRUE;
}

#ifdef AF_ENABLE_M 

BOOL SIM_HostCAFThreadExit()
{
    g_bAFThreadExit = TRUE;
    SetEvent(g_hContrlAFEvent);
    WaitForSingleObject(g_hContrlAFThread, INFINITE);
    CloseHandle(g_hContrlAFThread);

    return TRUE;
}

static U32 SIM_HCTSearchCST(U32 ulState)
{
    U8 *pHCTCstState;
    pHCTCstState = (U8*)g_ulCSTBaseAddr;

    U8 ucCstCnt;    //Mark one round.
    U32 ulCstID;    //Return CstID
    static U32 ulCstIDLa = 0;   //Record last CstID

    ulCstID = (ulCstIDLa + 1) % MAX_SLOT_NUM;
    
    for (ucCstCnt = 0; ucCstCnt < MAX_SLOT_NUM; ucCstCnt++)
    {
        if (ulState == pHCTCstState[ulCstID])
        {
            ulCstIDLa = ulCstID;
            return ulCstIDLa;
        }
        else
        {
            ulCstID++;
            ulCstID %= MAX_SLOT_NUM;
        }

    }

    if (MAX_SLOT_NUM >= ucCstCnt)
    {
        return INVALID_CMD_ID;
    }

    //Should Not Come here
    ASSERT(FAIL);
    return INVALID_CMD_ID;
}

void HCT_SetCSTByAF(U8 CSTID, U8 CurrentValue, U8 NextValue)
{
    /*
            if (g_pHCTControlReg->bAUTOFCHEN == FALSE)
                return;
                */
    U32 CSTAddr;
    U8 CSTValue;
    //EnterCriticalSection( &g_csCSTCriticalSection );

    CSTAddr = g_ulCSTBaseAddr + CSTID;
    CSTValue = *((char*)CSTAddr);

    if(CSTValue != CurrentValue)
    {
        DBG_Printf("CSTID 0x%x CSTValue %d CurrentValue %d\n",CSTID,CSTValue,CurrentValue);
        DBG_Break();
    }

    //RecLogFile("FCQ set CST[%d] from %d to %d\n", CSTID, CSTValue, NextValue);
    HCTDBG_RecordCST(CSTID, NextValue);

    *((char*)CSTAddr) = NextValue;
    if ( g_pWBQReg->bsWBQTRI == NextValue )
    {
        // temporarily assume FCQ thread doesn't get here.
        // by Charles Zhou
        DBG_Break();
    }

    //LeaveCriticalSection( &g_csCSTCriticalSection );
}

static void SIM_HCTSlotSQBonding(U32 ulSlotID, U32 SQID)
{
    PSQ_ENTRY pSQEntry;
    pSQEntry = (PSQ_ENTRY)g_pNVMeCfgExReg->SQEntry;

    pSQEntry[ulSlotID].HWRP = SQ_HWRP_M(SQID);
    pSQEntry[ulSlotID].SQID = SQID;
    
}

//AutoFetch One Cmd in Spcial SQID
//Return Which Slot has fetched Cmd
U32 SIM_HCTAutoFetch(U32 ulSQID)
{
    U32 ulSlotID;
    
    /*SN*/
#define SN_COMMAND_HEADER       (0)
#define SN_PRPT                 (1)

    //Search IDLE SLot
    //ulSlotID = SIM_HCTSearchCST(g_pNVMeCfgExReg->CstAutoTrig);

    /*
    if (g_pHCTControlReg->bAUTOFCHEN == FALSE)
        return INVALID_CMD_ID;
        */

    //ulSlotID = SIM_HCTSearchCST(0);
    ASSERT(g_pNVMeCfgExReg->CstAutoTrig == 0);
    ulSlotID = SIM_HCTSearchCST(g_pNVMeCfgExReg->CstAutoTrig);

    if (ulSlotID == INVALID_CMD_ID)
    {
        return  ulSlotID;
    }

    //Bonding Information
    SIM_HCTSlotSQBonding(ulSlotID, ulSQID);

    //Fetch
    U32 ulDeviceAddr;
    LARGE_INTEGER ullHostAddr;
    ulDeviceAddr = HCT_GetDeviceAddr(TRUE, SN_COMMAND_HEADER, 0, ulSlotID);

    ullHostAddr.LowPart = SQ_HOST_BAL_M(ulSQID)+ SQ_HWRP_M(ulSQID) * g_pNVMeCfgExReg->SQCfgAttr[ulSQID].StepAddr;
    ullHostAddr.HighPart = (ullHostAddr.LowPart < SQ_HOST_BAL_M(ulSQID)) ? (SQ_HOST_BAH_M(ulSQID) + 1):SQ_HOST_BAH_M(ulSQID);
    
    if (0 != (ullHostAddr.LowPart & 0x03))
    {
        DBG_Printf("SIM_HCTAutoFetch: Host addr_Low_part = 0x%x, don't align 1DW", ullHostAddr.LowPart);
        DBG_Getch();
    }

    if (0 != (ulDeviceAddr & 0x3))
    {
        DBG_Printf("SIM_HCTAutoFetch: local DeviceAddr = 0x%x, don't align 1DW", ulDeviceAddr);
        DBG_Getch();
    }

    if (0 != (g_pNVMeCfgExReg->CmdLen & 0x3))
    {
        DBG_Printf("SIM_HCTAutoFetch: Data Length = 0x%x, don't align 1DW", g_pNVMeCfgExReg->CmdLen);
        DBG_Getch();
    }

    //Clear S0 section
    memset((void*)ulDeviceAddr, 0, g_pNVMeCfgExReg->CmdLen);
    memcpy((void*)ulDeviceAddr, (void*)ullHostAddr.QuadPart, g_pNVMeCfgExReg->CmdLen);
    
    NVME_FillHostCmdTable(ulSlotID, ullHostAddr.HighPart, ullHostAddr.LowPart, g_pHCTControlReg->bsAUTOFCHCST);

    return ulSlotID;
}

#endif //AF_ENABLE_M



