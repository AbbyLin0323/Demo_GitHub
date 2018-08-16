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

Filename     :   AHCI_ControllerModel.c
Version      :   0.1
Date         :   2013.08.26
Author       :   bettywu

Description:  implement the function of AHCI host
Others:
Modification History:
20130826 create

*******************************************************************/


//inlcude common
#include "model_common.h"

#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#include "model_common.h"
#endif
#ifdef SIM
#include <time.h>
#endif
//include firmware
#include "HAL_HCT.h"
#include "L0_NVME.h"
#include "L0_NVMEHCT.h"

#include "HostModel.h"
#include "Proj_config.h"
#include "HAL_NVME.h"
#ifdef AF_ENABLE_M
#include "HAL_NVMECFGEX.h"
#endif
#include "NVME_ControllerModel.h"
#include "NVME_HostCommand.h"

#include "nvmeReg.h"
#include "nvme.h"
#include "nvmeStd.h"
#include "Sim_HCT.h"

extern HCT_MGR g_tHCTMgr;
extern const U8 g_uWBQDepth;
extern U32 g_ulHCTSramStartAddr;
extern volatile HCT_FCQ_REG *g_pFCQReg;
extern volatile HCT_WBQ_REG *g_pWBQReg;
extern volatile HCT_CONTROL_REG *g_pHCTControlReg;
extern PHCT_BAINC_REG g_pHCTBaIncReg;

volatile NVMe_CONTROLLER_REGISTERS *g_pCtrlReg;
extern volatile struct hal_nvmecfg *g_pNVMeCfgReg;

#ifdef SIM  //Enable AF_Model for winSim
    #ifndef AF_ENABLE_M
        #ifdef AF_ENABLE
        #error("AF_ENALBE_M is disable. But AF_ENABLE is enable")
        #endif
    #endif //NON_AF_ENALBE_M
#endif //SIM

#ifdef AF_ENABLE_M
extern volatile NVME_CFG_EX *g_pNVMeCfgExReg;   //Defined in L0_NVMe.c
extern BOOL g_bAFThreadExit;
extern HANDLE g_hContrlAFEvent;
static BOOL l_bAdminProcessing = FALSE;
static U8   l_cAdminProcessSlot = INVALID_CMD_ID;
extern U32 HCT_GetCST(U8 CSTID, U8* pCurrentValue);

enum
{
    ARB_PRI_ADMIN = 0,
    ARB_PRI_URGENT,
    ARB_PRI_HIGH,
    ARB_PRI_MEDIUM,
    ARB_PRI_LOW,
    ARB_PRI_MAX
};

typedef struct _NVME_CFG_EX_MODEL
{
    volatile struct {
        U32 ulType;
        U32 ulBondingSQBitMap;
        U32 ulSQIdLast;
    }sArbClass[ARB_PRI_MAX];
//It is possible that there is no more CQ entries for the new SQ entries to autofetch by HW.
#if (AVOID_CQ_HUNGRY)
    volatile U16 CqPreTail[MAX_CQ_NUM];    //HW will main the reserved CQ Tail Pointer
#endif //AVOID_CQ_HUNGRY

}NVME_CFG_EX_MODEL, *PNVME_CFG_EX_MODEL;
volatile NVME_CFG_EX_MODEL g_NVMeCfgExLocInfo;
#endif //AF_ENABLE_M

extern void Host_FillDataToBuffer(PLARGE_INTEGER BufferAddr,U32 DstLba);
extern void Host_ReadDataFromBuffer(PLARGE_INTEGER BufferAddr,U32 DstLba);
extern void Host_WriteToDevice(U32 ulDeviceAddr, U32 ulBytes, const U8 *pSrcBuf);
extern void Host_ReadFromDevice(U32 ulDeviceAddr, U32 ulBtyes, U8 *pDestBuf);
extern void HCTDBG_ClearCST(U32 ulSlotNum);
CRITICAL_SECTION g_CmdCriticalSection;

extern void SIM_MSITrigger(DWORD vector);


#ifdef AF_ENABLE_M
LOCAL INLINE U32 ilog2(U32 x)
{
    U8 i = 0;
    if (((x & x -1 ) != 0) && (x != 0))
    {
        DBG_Getch();
    }
    do
    {
        if ((x & 1) == 1)
        {
            return i;
        }
        else
        {
            x >>= 1;
        }
        i++;
    } while (i < 32);
    return i;
    
}


/* Init NVMeCfgEx Register ,Auto Fetch Purpose */
void NVME_ControllerCfgExModelInit(void)
{
    g_pHCTControlReg->bAUTOFCHEN = FALSE;

    DBG_Printf("Init AF Reg:%d\n", g_pHCTControlReg->bAUTOFCHEN);

    ASSERT(g_pNVMeCfgExReg != 0 );
    memset((void*)g_pNVMeCfgExReg, 0, sizeof(NVME_CFG_EX));

    g_pNVMeCfgExReg->CmdLen = sizeof(COMMAND_HEADER);

    U8 ucSQIdCnt = 0;
    for (ucSQIdCnt = 0; ucSQIdCnt < MAX_SQ_NUM; ucSQIdCnt++)
    {
        g_pNVMeCfgExReg->SQCfgAttr[ucSQIdCnt].P = PRIORITY_DEFAULT;
    }

    //Init Model internal Information
    memset((void*)&g_NVMeCfgExLocInfo, 0 ,sizeof(NVME_CFG_EX_MODEL));

    U8 ucArbPri;
    for(ucArbPri = ARB_PRI_ADMIN; ucArbPri < ARB_PRI_MAX; ucArbPri++)
    {
        g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulType = ucArbPri;
        g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast = 0;
        g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap = 0;
    }
}


static void NVME_AF_UpdateAdminProState(void)
{
    //Update AdminProcess State
    if (l_bAdminProcessing == TRUE && l_cAdminProcessSlot != INVALID_CMD_ID)
    {
        ASSERT(0 == g_pNVMeCfgExReg->CstAutoTrig);
        if (g_pNVMeCfgExReg->CstAutoTrig == HCT_GetCST(l_cAdminProcessSlot, NULL))
        {
            //Admin CMD is completed
            l_bAdminProcessing = FALSE;
            l_cAdminProcessSlot = INVALID_CMD_ID;
        }
    }
    else if (l_bAdminProcessing != TRUE && l_cAdminProcessSlot == INVALID_CMD_ID)
    {
        //No Admin CMM,no need to update AdminProcess State
    }
    else
    {
        DBG_Printf("AdminMarker is TRUE but Slot is INVALID or AdminMarker is FALSE but Slot is VALID\n");
        DBG_Getch();
    }
}


static void NVME_AF_UpdateAdminSlot(U32 ulSlotID)
{
    if (l_bAdminProcessing == FALSE)
    {
        ASSERT(l_cAdminProcessSlot == INVALID_CMD_ID)
        l_cAdminProcessSlot = ulSlotID;
        l_bAdminProcessing = TRUE;
    }
    else
    {
        DBG_Printf("The AdminMarker is TRUE and we allow to fetch a New AdminCMD, Fatal Error!\n");
        DBG_Getch();
    }
}

static BOOL NVME_HasAdminCMD(void)
{
    NVME_AF_UpdateAdminProState();
    if (SQ_NOT_EMPTY_M(0))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DBG_Printf("NVME Check AdminSQ\n");
    DBG_Getch();
    return FALSE;
}

void NVME_CheckAdminSQAF(void)
{
    U32 ulSlotID = INVALID_CMD_ID;
    if (CQ_SIZE_M(0) == 0)
    {
        DBG_Printf("AdminCQSize is 0, Fatal Error\n");
        DBG_Getch();
    }

    NVME_AF_UpdateAdminProState();

    while (SQ_NOT_EMPTY_M(0) && CQ_NOT_FULL_M(0))
    {
            if (l_bAdminProcessing == TRUE)
            {
                return;   //Fetch Admin One by One. Return to Fetch IOSQ
            }

            //Fetch CMD
            ulSlotID = SIM_HCTAutoFetch(0);

            if (INVALID_CMD_ID != ulSlotID)
            {
                PUSH_SQ_HWRP_M(0);
                //Two CQ tail:1. NVMeCfg.CQ.tail 2:CQ_PreTail
                //When enable avoid_cq_hungry:
                //The first one is only changed by WBQ. we cannot push here.
                //The second one is maintained by AF. Avoid CQ hungry purpose.
                //When disable avoid_cq_hungry:
                //Only the first CQ tail wil be used.
#if (TRUE == AVOID_CQ_HUNGRY)
                PUSH_CQ_TAIL_M(0);
#endif 
                NVME_AF_UpdateAdminSlot(ulSlotID);
                HCT_SetCSTByAF(ulSlotID, g_pNVMeCfgExReg->CstAutoTrig, g_pHCTControlReg->bsAUTOFCHCST);
            }
    }//while
}


//Verfy whether SQ is enabled or not
BOOL NVME_VerifySQ(U8 ucSQID)
{
    U8 ucCQID = 0;
    U8 ucBM = 0;
    ucCQID  = g_pNVMeCfgExReg->SQCfgAttr[ucSQID].CQMaped;
    ucBM = g_pNVMeCfgExReg->SQCfgAttr[ucSQID].BM;

    if (ucBM == 0)
    {
        //We havn't create a SQ
        return FALSE;
    }
    
    //It is not possible that ucBM != 0 and CQxSize == 0 since when we have created a SQ but the related CQ should no be zero!
    if (CQ_SIZE_M(ucCQID) == 0)
    {
        DBG_Getch();
    }
    else
    {
        return TRUE;
    }

    //Should no come to here
    DBG_Getch();
    return FALSE;
}

void NVME_CheckIOSQAF(void)
{
    U8 ucSQID = 0;
    U8 ucCQID = 0;
    U8 ucBMCnt = 0;
    U32 ulSlotID = INVALID_CMD_ID;
    BOOL bFetchMarker;

    static U8 ucSQIDLast = 1;

    for (ucSQID = ucSQIDLast; ucSQID < MAX_SQ_NUM; ucSQID++)
    {

        if (TRUE == NVME_HasAdminCMD())
        {
            ucSQIDLast = ucSQID;
            return;
        }

        ucCQID  = g_pNVMeCfgExReg->SQCfgAttr[ucSQID].CQMaped;
        ucBMCnt = 0;    //Fix Me:Should be used in SIM

        if (FALSE == NVME_VerifySQ(ucSQID))
        {
            continue;   //Search Next IOSQ
        }

#if 0  //Speep Up purpose in SIM. Discarded
        if (SQ_NOT_EMPTY_M(ucSQID) && CQ_NOT_FULL_M(ucCQID) )
        {
            //Fetch CMD
            ulSlotID = SIM_HCTAutoFetch(ucSQID);

            if (INVALID_CMD_ID != ulSlotID)
            {
                PUSH_SQ_HWRP_M(ucSQID);
                HCT_SetCSTByAF(ulSlotID, g_pNVMeCfgExReg->CstAutoTrig, g_pHCTControlReg->bsAUTOFCHCST);
#if (AVOID_CQ_HUNGRY)
                PUSH_CQ_TAIL_M(ucCQID);
#endif
                continue;
            }
            else
            {
                ucSQIDLast = ((ucSQID + 1 % MAX_SQ_NUM)== 0) ? 1 : ((ucSQID + 1) % MAX_SQ_NUM);
                return;
            }
        }//if or while
#else
        bFetchMarker = FALSE;
        while (SQ_NOT_EMPTY_M(ucSQID) && CQ_NOT_FULL_M(ucCQID) && (ucBMCnt < g_pNVMeCfgExReg->SQCfgAttr[ucSQID].BM))
        {
            //Fetch CMD
            ulSlotID = SIM_HCTAutoFetch(ucSQID);

            if (INVALID_CMD_ID != ulSlotID)
            {
                ucBMCnt++;
                PUSH_SQ_HWRP_M(ucSQID);
                HCT_SetCSTByAF(ulSlotID, g_pNVMeCfgExReg->CstAutoTrig, g_pHCTControlReg->bsAUTOFCHCST);
#if (AVOID_CQ_HUNGRY)
                PUSH_CQ_TAIL_M(ucCQID);
#endif
                bFetchMarker = TRUE;
                continue;
            }
            else //Slot Full
            {
                //If it has already fetched commands, it can fetch next SQ and return; otherwise record the ucSQID and return.
                if (TRUE == bFetchMarker)
                {
                    ucSQIDLast = ((ucSQID + 1 % MAX_SQ_NUM) == 0) ? 1 : ((ucSQID + 1) % MAX_SQ_NUM);
                }
                else
                {
                    ucSQIDLast = ucSQID;
                }
                return;
            }
        }//if or while
#endif

    }//for
    ucSQIDLast = 1;
}


//Calc the Next SQId according to the ArbClass.SQBitMap
LOCAL U8 NVME_GetArbClassNextSQId(U32 ulArbSQBitMap, U8 ucSQIdLast)
{
    U8 ucNextSQIdTmp = ucSQIdLast;
    U8 i;
    if (ulArbSQBitMap == 0)
    {
        return 0;
    }
    for (i = 0; i < MAX_SQ_NUM; i++)
    {
        ucNextSQIdTmp = (ucNextSQIdTmp + 1) % MAX_SQ_NUM;
        if (((1 << ucNextSQIdTmp) & ulArbSQBitMap) != 0)
        {
            return ucNextSQIdTmp;
        }
    }

    DBG_Getch();
    return 0;
}

LOCAL BOOL NVME_IsSQIdInArbClass(U8 ucSQId, U8 ucArbClass)
{
    ASSERT(ucArbClass != ARB_PRI_MAX);
    ASSERT((0 <= ucSQId ) && (ucSQId < MAX_SQ_NUM));
    
    if (0 != (g_NVMeCfgExLocInfo.sArbClass[ucArbClass].ulBondingSQBitMap & (1 << ucSQId)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DBG_Getch();
    return FALSE;
}

LOCAL void NVME_UpdateArbClassSQIdLast(void)
{
    U8 ucSQId;
    U8 ucArbPri;
    U32 ulSQBitMapTmp = 0;
    LOCAL U32 s_ulSQBitMap = 0;

    for (ucSQId = 0; ucSQId < MAX_SQ_NUM; ucSQId++)
    {
        if (0 != g_pNVMeCfgExReg->SQCfgAttr[ucSQId].BM)
        {
            ulSQBitMapTmp |= (1 << ucSQId);
        }
    }

    //if SQBitMap changed, sArbClass[ucArbPri].ulSQIdLast should be updated.
    if (s_ulSQBitMap != ulSQBitMapTmp)
    {
        for (ucArbPri = ARB_PRI_URGENT; ucArbPri < ARB_PRI_MAX; ucArbPri++)
        {
            g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast =
                NVME_GetArbClassNextSQId(g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap, 0);
        }
        s_ulSQBitMap = ulSQBitMapTmp;
    }
}
//Update ArbSQBondingBitMap 
LOCAL void NVME_UpdateArbClassBitMap(void)
{
    U8 ucSQId;
    U32 ucArbPri = 0;
    U32 ulSQBitMapChk = 0;

    ASSERT(g_pNVMeCfgExReg != NULL);

    for (ucArbPri = 0; ucArbPri < ARB_PRI_MAX; ucArbPri++)
    {
        g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap = 0;
    }

    for (ucSQId = 0; ucSQId < MAX_SQ_NUM; ucSQId++)
    {
        
        if (0 != g_pNVMeCfgExReg->SQCfgAttr[ucSQId].BM)
        {
            g_NVMeCfgExLocInfo.sArbClass[ilog2(g_pNVMeCfgExReg->SQCfgAttr[ucSQId].P)].ulBondingSQBitMap &= ~(1 << ucSQId);
            g_NVMeCfgExLocInfo.sArbClass[ilog2(g_pNVMeCfgExReg->SQCfgAttr[ucSQId].P)].ulBondingSQBitMap |= (1 << ucSQId);
        }
        else
        {
            //SQBitMapChk for disable SQ
            ulSQBitMapChk |= (1 << ucSQId);
        }
    }

    //SQBitMapChk
    for (ucArbPri = 0; ucArbPri < ARB_PRI_MAX; ucArbPri++)
    {
        ulSQBitMapChk += g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap;
    }

    if (ulSQBitMapChk != ((1 << MAX_SQ_NUM) - 1 ))
    {
        DBG_Printf("Arbitration SQ Mapping Error\n");
        DBG_Getch();
    }
}

LOCAL void NVME_UpdateArbClass(void)
{
    //Update BitMap Bonding information since SQCfgAttr.BM can be changed at run time.
    NVME_UpdateArbClassBitMap();

    //If the BitMap is changed, the sequence of SQs in the same ArbClass should be updated.
    NVME_UpdateArbClassSQIdLast();
}

//Check whether SQArbClass has SQ command nor not
LOCAL BOOL NVME_CheckArbClassSQSta(U8 ucArbPri)
{
    if (0 != (g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap & g_pNVMeCfgReg->cmd_fetch_helper))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DBG_Getch();
    return FALSE;
}



//Fetch Command according to the ucArbPri(ArbClass)
//return FALSE  - None of Command is fetched. Slot Full or HasAdminCMD
//       TRUE   - One or above command is fetched.
LOCAL BOOL NVME_ArbClassAF(U8 ucArbPri)
{
    U8 ucSQId;
    U8 ucCQId;
    U8 ucBMCnt;
    U8 ulSlotId = 0xFE;
    BOOL bFetchMarker;

    if (TRUE == NVME_HasAdminCMD())
    {
        return FALSE;
    }

    ucSQId = g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast;

    if (FALSE == NVME_VerifySQ(ucSQId))
    {
        DBG_Printf("Warning:VerifySQ error. May be delete SQ and fetch SQ CMD at the same time.");
        return FALSE;
        //DBG_Getch();
    }
     
    if (FALSE == NVME_IsSQIdInArbClass(ucSQId, ucArbPri))
    {
        DBG_Getch();
    }

    if (ucArbPri != ilog2(g_pNVMeCfgExReg->SQCfgAttr[ucSQId].P))
    {
        DBG_Getch();
    }

    //Fetch as many as possible according BM
    ucCQId = g_pNVMeCfgExReg->SQCfgAttr[ucSQId].CQMaped;
    ucBMCnt = 0;

    bFetchMarker = FALSE;
    while (SQ_NOT_EMPTY_M(ucSQId) && CQ_NOT_FULL_M(ucCQId) && (ucBMCnt < g_pNVMeCfgExReg->SQCfgAttr[ucSQId].BM))
    {
        //Fetch CMD
        ulSlotId = SIM_HCTAutoFetch(ucSQId);

        if (INVALID_CMD_ID != ulSlotId)
        {
            ucBMCnt++;
            PUSH_SQ_HWRP_M(ucSQId);
            HCT_SetCSTByAF(ulSlotId, g_pNVMeCfgExReg->CstAutoTrig, g_pHCTControlReg->bsAUTOFCHCST);
#if (AVOID_CQ_HUNGRY)
            PUSH_CQ_TAIL_M(ucCQId);
#endif
            bFetchMarker = TRUE;
            continue;
        }
        else
        {
            break;
        }
    }//while

    //If it has already fetched commands, it can fetch next SQ and return TRUE; otherwise check what is going on
    if (TRUE == bFetchMarker)
    {
        g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast = 
            NVME_GetArbClassNextSQId(g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap, g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast);
        return TRUE;
    }
    else // We should know why it cannot fetch command 
    {
        if (ulSlotId == INVALID_CMD_ID) //Slot full. Do not Update SqidLast. Need Pending.
        {
            return FALSE;
        }
        else //SQ empty or CQ full or SQCnt == Max. it can fetch next SQ and return TRUE;
        {
            g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast =
                NVME_GetArbClassNextSQId(g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulBondingSQBitMap, g_NVMeCfgExLocInfo.sArbClass[ucArbPri].ulSQIdLast);
            return TRUE;
        }
    }

    DBG_Getch();
    return TRUE;
}

/*
Weight Round Robin for PRI_HIGH&PRI_MIDDLE_PRI_LOW
Note:The typical WRR should set weight for each priority. But according to the HW
desgin, it doesn't have a interface for weighted setting but the arb bust(BM). 
Therefore, we set weighted value to BM when we get a set feature arb command.
*/
LOCAL void NVME_CheckARBWRRAF(void)
{
    U8 ucArbPri;
    for (ucArbPri = ARB_PRI_HIGH; ucArbPri <= ARB_PRI_LOW; ucArbPri++)
    {
        if (TRUE == NVME_CheckArbClassSQSta(ucArbPri))
        {
            (void)NVME_ArbClassAF(ucArbPri);
        }
    }
}

extern void NVME_CheckSQ(void);
//Arbitration SQ(Admin&SQ) autofetch schedule
LOCAL void NVME_CheckARBSQAF(void)
{
    U8 ucArbPri;
    U8 ucArbPriLast = ARB_PRI_ADMIN;
    /*
    1. Update fetch_helper. FW will use fetch_helper to check host idle or not.
    2. Update SQ Priority(ArbClass) information since SQ Priority(SQBitMap) will be changed at rumtime.
    3. Priority Schedule.
    */   

    //1. Update fetch_helper. FW will use fetch_helper to check host idle or not.
    (void)NVME_CheckSQ();

    /* we need to update AdminSQProcessStatus
    if (g_pNVMeCfgReg->cmd_fetch_helper == 0)
        return;
    */

    //2. Update SQ Priority(ArbClass) information since SQ Priority(SQBitMap) may be changed at rumtime.
    NVME_UpdateArbClass();
    
    //3. Priority Schedule.
    for (ucArbPri = ARB_PRI_ADMIN; ucArbPri <= ARB_PRI_HIGH; ucArbPri++)
    {
        switch(ucArbPri)
        {
        case ARB_PRI_ADMIN:
            NVME_CheckAdminSQAF();
            break;
        case ARB_PRI_URGENT:
            if (TRUE == NVME_CheckArbClassSQSta(ucArbPri))
            {
                (void)NVME_ArbClassAF(ucArbPri);
                return;
            }
            break;
        case ARB_PRI_HIGH:  
            NVME_CheckARBWRRAF(); //Select CMDs from High, Middle or Low.
            break;
        default:
            DBG_Getch();
            break;
        }
    }

    //ucArbPriLast = ARB_PRI_ADMIN;
}

void NVME_CheckSQAF(void)
{
    //NVME_CheckAdminSQAF();
    //NVME_CheckIOSQAF();
    NVME_CheckARBSQAF();
}


DWORD WINAPI SIM_HostCAFThread(LPVOID p)
{
    g_bAFThreadExit = FALSE;
    while (g_bAFThreadExit == FALSE)
    {
       WaitForSingleObject(g_hContrlAFEvent,INFINITE);
       if (g_pHCTControlReg->bAUTOFCHEN)
       {
           NVME_CheckSQAF();
           #ifndef AF_ENABLE
           DBG_Getch();
           #endif
       }
       else
       {
           NVME_CheckSQ();
       }
    }
    return 0;
}
#endif //AF_ENABLE_M

void NVME_CheckSQ(void)
{
    int i=0;
    for(i=0 ; i<(ADMIN_QCNT + IO_QCNT) ; i++)    /* Max 9 SQs*/
    {
#ifdef AF_ENABLE_M
        if (SQ_NOT_EMPTY_M(i))
#else
        if(g_pNVMeCfgReg->doorbell[i].sq_tail != g_pNVMeCfgReg->sq_ptr[i].fw_read)
#endif
        {
            g_pNVMeCfgReg->cmd_fetch_helper |= 1<<i;
        }
        else
        {
            g_pNVMeCfgReg->cmd_fetch_helper &= (~(1<<i))&0x000001ff;
        }
    }
}


void NVME_ControllerModelInit()
{
    int i;
    /*
    Set registers' default values;
    */
    g_pNVMeCfgReg = (volatile struct hal_nvmecfg *)REG_BASE_NVME;
    g_pCtrlReg = (volatile NVMe_CONTROLLER_REGISTERS *)REG_BASE_NVME;
    memset((void*)g_pNVMeCfgReg, 0, sizeof(struct hal_nvmecfg));

    //g_pNVMeCfgReg->bar.vs  = 0x00010100;        //Major version is 0x0001 Minor version is 0x0100
    for(i = 0; i < (ADMIN_QCNT + IO_QCNT); i++)
    {
        g_pNVMeCfgReg->msix_table[i].vec_ctrl = 1;    // Mask Bit
        g_pNVMeCfgReg->cq_info[i].pbit = 1;
    }
#ifdef AF_ENABLE_M
    NVME_ControllerCfgExModelInit();
#endif
}


void NVME_ControllerModelSchedule()
{
#ifdef AF_ENABLE_M
    SetEvent(g_hContrlAFEvent);
#else
    NVME_CheckSQ();    
#endif
}


U32 g_CQEntryFullCnt = 0;
BOOL NVME_ExeWBQEntry(NVME_WBQ *pWBQ,U8 CmdTag)
{
    NVMe_COMPLETION_QUEUE_ENTRY CQEntry;

    volatile U16 Ptr;
    U32 DW3;
    U64 HostAddr;

    Ptr = g_pNVMeCfgReg->cq_info[pWBQ->Cqid].tail_entry_ptr;

    if(0 == pWBQ->Cqid)
    {
        if((++Ptr) == (U16)g_pNVMeCfgReg->cq0_size + 1)
        {
            Ptr = 0;
        }
    }
    else
    {
        if((++Ptr) == (U16)g_pNVMeCfgReg->cq_size[pWBQ->Cqid-1] + 1)
        {
            Ptr = 0;
        }
    }

    //check whether CQ entry full
    if(Ptr == g_pNVMeCfgReg->doorbell[pWBQ->Cqid].cq_head)
    {
        g_CQEntryFullCnt++;
        return FALSE;
    }

    CQEntry.DW2.SQHD = g_pNVMeCfgReg->sq_ptr[pWBQ->Sqid].head; /*TODO : to be checked*/
    CQEntry.DW2.SQID = pWBQ->Sqid;
    DW3  = (pWBQ->StatusF<<17)| (g_pNVMeCfgReg->cq_info[pWBQ->Cqid].pbit<<16) |(pWBQ->CmdID);
    CQEntry.DW3 = *(NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3*)&DW3;
    CQEntry.DW0 = pWBQ->CmdSpec;

    HostAddr  = (g_pNVMeCfgReg->cq_info[pWBQ->Cqid].base_addr_h);
    HostAddr <<=32;
    HostAddr += (g_pNVMeCfgReg->cq_info[pWBQ->Cqid].base_addr_l);
    HostAddr += (g_pNVMeCfgReg->cq_info[pWBQ->Cqid].tail_entry_ptr*sizeof(NVMe_COMPLETION_QUEUE_ENTRY));

    memcpy((void*)HostAddr,(void*)&CQEntry,sizeof(NVMe_COMPLETION_QUEUE_ENTRY));

    g_pNVMeCfgReg->cq_info[pWBQ->Cqid].tail_entry_ptr = Ptr;
    if(Ptr == 0)
    {
        g_pNVMeCfgReg->cq_info[pWBQ->Cqid].pbit ^= 1;
    }

    return TRUE;
}



BOOL NVME_HandleCmdWBQ(U8 CmdTag)
{
    NVME_WBQ *pWBQ;

    pWBQ = (NVME_WBQ*)(g_pWBQReg->usWBQBA + g_ulHCTSramStartAddr + CmdTag*sizeof(NVME_WBQ));

    if(FALSE == NVME_ExeWBQEntry(pWBQ,CmdTag))
    {
        return FALSE;
    }

    //specail handling for last WBQ
    if (TRUE == pWBQ->Last)
    {
        //clear WBQ active flag
        g_tHCTMgr.ActiveWBQBitmap[ CmdTag] = 0;
        g_tHCTMgr.DataDoneBitmap[CmdTag] = 0;
        g_tHCTMgr.bsWBQOffset[ CmdTag ] = 0;

        HCTDBG_ClearCST(CmdTag);

    }

    //update CST must be at last
    if(pWBQ->Update)
    {
        HCT_SetCSTByWBQ(CmdTag, pWBQ->CST, pWBQ->NST);
    }

    return TRUE;
}

BOOL SIM_HostCWBQProcessByEvent( U32 ulEventType )
{
    U8 ucCmdTag;
    volatile NVME_WBQ *pTargetWBQEntry;
    EnterCriticalSection( &g_tHCTMgr.WBQCriticalSection );

    // handle the WBQs without waiting for data done.
    for( ucCmdTag = 0; ucCmdTag < MAX_SLOT_NUM; ucCmdTag ++)
    {
        pTargetWBQEntry  =  (PNVME_WBQ)HAL_HCTGetWBQEntry(ucCmdTag, 0);

        if(1 == g_tHCTMgr.ActiveWBQBitmap[ucCmdTag])
        {
            if((0 == pTargetWBQEntry->WSGE) ||
                (1 == pTargetWBQEntry->WSGE && 1 == g_tHCTMgr.DataDoneBitmap[ucCmdTag]))
            {
                if(TRUE == NVME_HandleCmdWBQ(ucCmdTag))
                {
                    //MSI Interrupt
                    SIM_MSITrigger(0);
                }
                else
                {
                    LeaveCriticalSection(&g_tHCTMgr.WBQCriticalSection);
                    return FALSE;
                }
            }
        }

    }

    LeaveCriticalSection( &g_tHCTMgr.WBQCriticalSection );
    return TRUE;
}


void Host_WriteToDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag)
{
    LARGE_INTEGER ullHostAddr = {0};
    LARGE_INTEGER ullStartLba = {0};
    U32 ulByteLenInPrd = 0;
    U32 ulSecLen = ByteLen>>SEC_SIZE_BITS;
    U32 ulStartLba = 0;
    U32 ulLbaIndex = 0;
    U32 ulCmdLbaLen;
    U32 ulCmdByteLen;


    ulCmdLbaLen = NVME_GetLbaLen(nTag);

    ulCmdByteLen = (ulCmdLbaLen << SEC_SIZE_BITS);

    ullHostAddr.HighPart = HostAddrHigh;
    ullHostAddr.LowPart = HostAddrLow;

    EnterCriticalSection(&g_CmdCriticalSection);

#if 1
    Host_WriteToDevice(BufferAddr, ByteLen, (U8*)ullHostAddr.QuadPart);
    NVME_XferBtyes(nTag,ByteLen);
#else
    for (ulLbaIndex = 0; ulLbaIndex < ulSecLen; ulLbaIndex++)
    {
        Host_WriteToDevice(BufferAddr, SEC_SIZE, (U8*)ullHostAddr.QuadPart);
        ullHostAddr.QuadPart += SEC_SIZE;
        BufferAddr += SEC_SIZE;

        NVME_XferOneSector(nTag);
    }
#endif

    //check if finish data transfering on current tag
    if (NVME_XferedBytes(nTag) == ulCmdByteLen)
    {
        NVME_HostCSetCmdFinishFlag(nTag);
    }
    else if (NVME_XferedBytes(nTag) > ulCmdByteLen)
    {
        DBG_Printf("Tag 0x%x XferBytes overflow!\n", nTag);
        DBG_Break();
    }

    LeaveCriticalSection(&g_CmdCriticalSection);
}


void NVME_HostPrepareData(U32 StartLBA,U32 SecCnt,U32* pDataBuffer)
{

    U16 usSecIndex;
    U32 ulCurLBA = StartLBA;
    LARGE_INTEGER ulDstHostAddr = {0};
    ulDstHostAddr.LowPart = (U32)pDataBuffer;

    for (usSecIndex = 0; usSecIndex < SecCnt; usSecIndex++)
    {
        //memset((U32*)ulDstHostAddr.QuadPart,0,SEC_SIZE);
        Host_FillDataToBuffer((PLARGE_INTEGER)ulDstHostAddr.QuadPart, ulCurLBA);
        ulDstHostAddr.QuadPart += SEC_SIZE;
        ulCurLBA++;
    }
}

void NVME_HostResetTrimData(TRIM_CMD_ENTRY *pTrimBlockEntry, U8 ucBlockCnt)
{
    U8 ucIndex;
    TRIM_CMD_ENTRY* pTrimCmdEntry;
    U32 ulLbaEntryIndex;
    U32 ulStartLba;
    U32 ulLba, ulLen;

    for (ucIndex = 0; ucIndex < ucBlockCnt; ucIndex++)
    {
        pTrimCmdEntry = &(pTrimBlockEntry[ucIndex]);

        for(ulLbaEntryIndex = 0; ulLbaEntryIndex < TRIM_LBA_RANGE_ENTRY_MAX; ulLbaEntryIndex++)
        {
            ulStartLba = (pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].StartLbaLow) << SEC_PER_DATABLOCK_BITS;
            ulLen = (pTrimCmdEntry->LbaRangeEntry[ulLbaEntryIndex].RangeLength) << SEC_PER_DATABLOCK_BITS;
            for(ulLba=ulStartLba; ulLba< (ulStartLba + ulLen); ulLba++)
            {
                Host_SaveDataCnt(ulLba,0);//set lba's write count to 0
            }
         }
     }

    return;
}
void Host_ReadFromDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag)
{
    LARGE_INTEGER ullHostAddr = {0};
    LARGE_INTEGER ulLbaStartAddr = {0};
    U32 ullStartLba = {0};
    U32 ulLeaveByteLen = ByteLen;

    U32 ulStartLba = 0;
    U32 ulEndLba = 0;
    U32 ulLba = 0;
    U32 ulCmdLbaLen;
    U32 ulCmdByteLen;

    //U8 ucCmdCode = NVME_GetCmdCode(nTag);
    BOOL bLbaCmd = NVME_HostIsLbaCmd(nTag);

    ulCmdLbaLen = NVME_GetLbaLen(nTag);

    ulCmdByteLen = (ulCmdLbaLen << SEC_SIZE_BITS);

    ullHostAddr.HighPart = HostAddrHigh;
    ullHostAddr.LowPart = HostAddrLow;

    ulLbaStartAddr.QuadPart = ullHostAddr.QuadPart;

    EnterCriticalSection(&g_CmdCriticalSection);

    Host_ReadFromDevice(BufferAddr, ByteLen, (U8*)ullHostAddr.QuadPart);

    if (TRUE == bLbaCmd)
    {
        while (0 != ulLeaveByteLen)
        {
            U32 ulEndLba = 0;
            NVMe_GetLbaFromHostCmdTable(&ullHostAddr,&ulLbaStartAddr, nTag, &ulLeaveByteLen, &ulStartLba, &ulEndLba);
            for (ulLba = ulStartLba; ulLba < ulEndLba; ulLba++) //read one sector every time
            {
                //do data check
                Host_ReadDataFromBuffer((PLARGE_INTEGER)ulLbaStartAddr.QuadPart, ulLba);
                ulLbaStartAddr.QuadPart += SEC_SIZE;
            }
        }
    }
    else
    {
         NVME_XferBtyes(nTag,ByteLen);
    }

    //chekc if finish data read on current tag
    if (NVME_XferedBytes(nTag) == ulCmdByteLen)
    {
        NVME_HostCSetCmdFinishFlag(nTag);
    }
    else if (NVME_XferedBytes(nTag) > ulCmdByteLen)
    {
        DBG_Break();
    }

    LeaveCriticalSection(&g_CmdCriticalSection);
    return;


}

void Host_ModelInitInterface()
{

    g_pHostDataBuffer = (UINT8*)malloc((DATA_BUFFER_PER_CMD * IO_Q_DEPTH * IO_QCNT) + 4095);

    if((NULL == g_pHostDataBuffer))
    {
        DBG_Printf("AHCI_HostInit: malloc host data buffer failure!\n");
        DBG_Break();
    }
    g_pHostDataBuffer = (UINT8*)(((U64)g_pHostDataBuffer) + (4096 - (((U64)g_pHostDataBuffer) & 4095)));

    InitializeCriticalSection(&g_CmdCriticalSection);
}
