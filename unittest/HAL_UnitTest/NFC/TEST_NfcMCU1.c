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
Filename    : TEST_NfcMCU1.c
Version     : Ver 1.0
Author      : abby
Date        : 20160905
Description : This file is compiled by MCU1
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcMCU1.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    EXTERN DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR U32 *g_pFileBaseInDram;
extern GLOBAL MCU12_VAR_ATTR BOOL g_bLocalBootUpOk;
extern GLOBAL MCU12_VAR_ATTR volatile U32 l_ulBbtStatusAddr;

/*------------------------------------------------------------------------------
    EXTERN FUNCTIONS
------------------------------------------------------------------------------*/
extern void MCU1_DRAM_TEXT L2_RT_Init_Clear_All(void);
extern void MCU12_DRAM_TEXT L2_VBT_Init_Clear_All(void);
extern BOOL TEST_NfcIsLastCheckListFile(CHECK_LIST_FILE_ATTR *pFileAttr);
extern void MCU1_DRAM_TEXT TEST_NfcLocalLLF(void);
extern void MCU1_DRAM_TEXT TEST_NfcCopyChecklist2SafeDram(void);
extern void MCU1_DRAM_TEXT TEST_NfcExtSharedMemMap(SUBSYSTEM_MEM_BASE *pFreeMemBase);

/*------------------------------------------------------------------------------
    FUNCTION DEFINITION
------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
Name:   TEST_NfcPattGenInit        
Description:
    Initialize pattern gen
AuthorName:      AbbyLin
-------------------------------------------------------------------------*/
LOCAL void MCU1_DRAM_TEXT TEST_NfcPattGenInit(void)
{
    U8 ucFileType;
    
    ucFileType = TEST_NfcGetCheckListFileType();

    if (BASIC_CHK_LIST == ucFileType)
    {
        TEST_NfcBasicPattGenInit();        
    }
    else//ext file
    {
        TEST_NfcExtPattGenInit();
    }
}

void MCU1_DRAM_TEXT TEST_NfcTaskInit(SUBSYSTEM_MEM_BASE * pFreeMemBase)
{
    L1_SharedMemMap(&g_FreeMemBase);
    L1_SetDefaultDeviceParam();
    
    L2_SharedMemMap(&g_FreeMemBase);
    L2_FCMDQReqInit();
    L2_Sram0Map(&g_FreeMemBase.ulFreeSRAM0Base);
    L2_Sram1Map(&g_FreeMemBase.ulFreeSRAM1Base);

    //Allocate for RT,need 32K align
    U32 ulFreeDramBase = g_FreeMemBase.ulDRAMBase;
    pRT = (RT *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RT)));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    DBG_Printf("NFC UT Alloc pRT DRAM memory, size %dKB\n",(ulFreeDramBase - g_FreeMemBase.ulDRAMBase)/1024);
    g_FreeMemBase.ulDRAMBase = ulFreeDramBase;
    
    L2_BbtDisablePbnBindingTable();
    L2_RT_Init_Clear_All();
    L2_VBT_Init_Clear_All();

    TEST_NfcPattGenInit();
}

LOCAL U32 TEST_NfcStartOneCMDPerfTimer(void)
{
#ifdef PATT_GEN_ONE_CMD_CYC_CNT
    return TEST_NfcStartTimer();
#else
    return 0;
#endif
}

LOCAL void TEST_NfcEndOneCMDPerfTimer(U32 ulStartTimer, U32 ulEndTimer)
{
#ifdef PATT_GEN_ONE_CMD_CYC_CNT
    U32 ulTimeDelta = 0;
    U32 ulTimeDeltaInUs = TEST_NfcEndTimer(ulStartTimer, ulEndTimer, &ulTimeDelta);
    DBG_Printf("PATT_GEN_ONE_CMD_PERF: cycle cnt 0x%x, cost time %d us\n", ulTimeDelta, ulTimeDeltaInUs);
#endif
}

LOCAL BOOL TEST_MCU1EventCheck(void)
{
    /* blocking before backend init done */
    COMMON_EVENT L2_Event;
    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event))
    {
        if (!L2_Event.EventInit)//should not exist other event now
        {
            DBG_Printf("MCU1 Event Invalid!\n");
        }
        return FALSE;
    }
    return TRUE;
}

#ifdef EXT_PERFORMANCE_TEST
/****************************************************************************
Function      : TEST_NfcMCU1Main
Input         :
Output        :
Description   : only for EXT performance test
Reference     :
History       :
    20160904    abby    create
****************************************************************************/
#ifdef P_DBG_1
extern U32 g_DbgHostRd;
#endif
void TEST_NfcMCU1Main(void)
{ 
    if (FALSE == TEST_MCU1EventCheck())//blocking event
    {
        return;
    }
    
    TEST_NfcCopyChecklist2SafeDram();

    /*  clr MCU1 event to enable MCU2 */
    CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_BOOT);

    U8  ucPerfCase = TLC_SEQ_WT_PERF;
    U32 ulTimeDelta = 0, ulTimeDeltaInUs = 0;
    U32 a_ulCmdCnt[PERF_TYPE_CNT] = {0};
    CHECK_LIST_EXT_FILE *pChecklist;

    /*  init llf table: BBT/VBT/PBIT, only do when boot up  */
    if (FALSE == g_bLocalBootUpOk)
    {
        TEST_NfcLocalLLF();  
    }
    
    while (ucPerfCase < SLC2TLC_PERF)
    {
        switch (ucPerfCase)
        {
            case SLC_SEQ_WT_PERF:
            {
                g_bTlcMode = FALSE;
                a_ulCmdCnt[SLC_SEQ_WT_PERF] = 0;
                
                /* fill into fake checklist */   
                pChecklist = (CHECK_LIST_EXT_FILE*)g_pCheckListPtr;
                pChecklist += TEST_NfcFakeSeqW(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;

                /* start SLC_SEQ_RD_PERF timer */
                g_aStartTimer[SLC_SEQ_WT_PERF] = TEST_NfcStartTimer();
                        
                while(TRUE)
                {  
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();

                    TEST_NfcExtPattGen();
                    
                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);

                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[SLC_SEQ_WT_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end SLC_SEQ_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[SLC_SEQ_WT_PERF], g_aEndTimer[SLC_SEQ_WT_PERF], &ulTimeDelta);
                DBG_Printf("SLC_SEQ_WT_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[SLC_SEQ_WT_PERF], ulTimeDelta, ulTimeDeltaInUs);

                ucPerfCase = SLC_SEQ_RD_PERF;
            }break;

            case SLC_SEQ_RD_PERF:
            {
                g_bTlcMode = FALSE;
                a_ulCmdCnt[SLC_SEQ_RD_PERF] = 0;
                
                /* reset checklist pointer to avoid over-flow, and fill into new checklist */ 
                pChecklist = (CHECK_LIST_EXT_FILE *)g_pFileBaseInDram;
                g_pCheckListPtr = (U32*)pChecklist;
                pChecklist += TEST_NfcFakeSeqR(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;
                
                #ifdef P_DBG_1
                g_DbgHostRd = TRUE;
                #endif             

                /* start SLC_SEQ_RD_PERF timer */
                g_aStartTimer[SLC_SEQ_RD_PERF] = TEST_NfcStartTimer();
                
                while(TRUE)
                {     
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
                    
                    TEST_NfcExtPattGen();

                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);
                
                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[SLC_SEQ_RD_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end SLC_SEQ_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[SLC_SEQ_RD_PERF], g_aEndTimer[SLC_SEQ_RD_PERF], &ulTimeDelta);
                //DBG_Printf("SLC_SEQ_RD_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[SLC_SEQ_RD_PERF], ulTimeDelta, ulTimeDeltaInUs);

                ucPerfCase = TLC_SEQ_WT_PERF;
                
                #ifdef P_DBG_1
                g_DbgHostRd = FALSE;
                #endif

            }break;

            case SLC_RAND4K_RD_PERF:
            {
                g_bTlcMode = FALSE;
            
                /* fill into fake checklist */   
                pChecklist = (CHECK_LIST_EXT_FILE*)g_pCheckListPtr;
                pChecklist += TEST_NfcFakeRandR(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;

                /* start SLC_RAND4K_RD_PERF timer */
                g_aStartTimer[SLC_RAND4K_RD_PERF] = TEST_NfcStartTimer();
                
                #ifdef P_DBG_1
                g_DbgHostRd = TRUE;
                #endif
                
                while(TRUE)
                {  
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
                
                    TEST_NfcExtPattGen();

                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);

                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[SLC_RAND4K_RD_PERF])
                        break;
                    
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);   
                }
                /* end SLC_RAND4K_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[SLC_RAND4K_RD_PERF], g_aEndTimer[SLC_RAND4K_RD_PERF], &ulTimeDelta);
                DBG_Printf("SLC_RAND4K_RD_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[SLC_RAND4K_RD_PERF], ulTimeDelta, ulTimeDeltaInUs);            

                #ifdef P_DBG_1
                g_DbgHostRd = FALSE;
                #endif

                ucPerfCase = TLC_SEQ_WT_PERF;//TLC_SEQ_WT_PERF;//TLC_SEQ_WT_PERF;//TLC_SEQ_WT_PERF;//SLC2TLC_PERF;

            }break;
            
            case TLC_SEQ_WT_PERF:
            {
                g_bTlcMode = TRUE;
                a_ulCmdCnt[TLC_SEQ_WT_PERF] = 0;
                
                /* reset checklist pointer to avoid over-flow, and fill into new checklist */ 
                pChecklist = (CHECK_LIST_EXT_FILE *)g_pFileBaseInDram;
                g_pCheckListPtr = (U32*)pChecklist;
                pChecklist += TEST_NfcFakeSeqW(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;
                
                #ifdef P_DBG_1
                g_DbgHostRd = TRUE;
                #endif             

                /* start TLC_SEQ_RD_PERF timer */
                g_aStartTimer[TLC_SEQ_WT_PERF] = TEST_NfcStartTimer();
                    
                while(TRUE)
                {     
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
                    
                    TEST_NfcExtPattGen();

                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);
                
                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[TLC_SEQ_WT_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end SLC_SEQ_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[TLC_SEQ_WT_PERF], g_aEndTimer[TLC_SEQ_WT_PERF], &ulTimeDelta);
                DBG_Printf("TLC_SEQ_WT_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[TLC_SEQ_WT_PERF], ulTimeDelta, ulTimeDeltaInUs);

                #ifdef P_DBG_1
                g_DbgHostRd = FALSE;
                #endif

                ucPerfCase = TLC_SEQ_RD_PERF;

            }break;

            case TLC_SEQ_RD_PERF:
            {
                g_bTlcMode = TRUE;
                a_ulCmdCnt[TLC_SEQ_RD_PERF] = 0;

                /* reset checklist pointer to avoid over-flow, and fill into new checklist */ 
                pChecklist = (CHECK_LIST_EXT_FILE *)g_pFileBaseInDram;
                g_pCheckListPtr = (U32*)pChecklist;
                pChecklist += TEST_NfcFakeSeqR(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;

                /* start TLC_SEQ_RD_PERF timer */
                g_aStartTimer[TLC_SEQ_RD_PERF] = TEST_NfcStartTimer();
                    
                while(TRUE)
                {     
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
                    
                    TEST_NfcExtPattGen();

                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);
                
                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[TLC_SEQ_RD_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end SLC_SEQ_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[TLC_SEQ_RD_PERF], g_aEndTimer[TLC_SEQ_RD_PERF], &ulTimeDelta);
                //DBG_Printf("TLC_SEQ_RD_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[TLC_SEQ_RD_PERF], ulTimeDelta, ulTimeDeltaInUs);

                ucPerfCase = TLC_SEQ_RD_PERF;
                
            }break;

            case TLC_RAND4K_RD_PERF:
            {
                g_bTlcMode = TRUE;
                a_ulCmdCnt[TLC_RAND4K_RD_PERF] = 0;
            
                /* fill into fake checklist */   
                pChecklist = (CHECK_LIST_EXT_FILE*)g_pCheckListPtr;
                pChecklist += TEST_NfcFakeRandR(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;

                /* start TLC_RAND4K_RD_PERF timer */
                g_aStartTimer[TLC_RAND4K_RD_PERF] = TEST_NfcStartTimer();
                        
                while(TRUE)
                {  
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();

                    TEST_NfcExtPattGen();
                    
                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);

                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[TLC_RAND4K_RD_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end TLC_RAND4K_RD_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[TLC_RAND4K_RD_PERF], g_aEndTimer[TLC_RAND4K_RD_PERF], &ulTimeDelta);
                DBG_Printf("TLC_RAND4K_RD_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[TLC_RAND4K_RD_PERF], ulTimeDelta, ulTimeDeltaInUs);

                ucPerfCase = SLC2TLC_PERF;
                
            }break;
            
            case SLC2TLC_PERF:
            {
                a_ulCmdCnt[SLC2TLC_PERF] = 0;
                
                /* step1: prepare data for copy */
                pChecklist = (CHECK_LIST_EXT_FILE *)g_pFileBaseInDram;
                g_pCheckListPtr = (U32*)pChecklist;
                pChecklist += TEST_NfcFakeExtCopyPrepare(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;
                  
                while(TRUE)
                {     
                    TEST_NfcExtPattGen();

                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }

                /* step2: copy performance test */
                /* reset checklist pointer to avoid over-flow, and fill into new checklist */ 
                pChecklist = (CHECK_LIST_EXT_FILE *)g_pFileBaseInDram;
                g_pCheckListPtr = (U32*)pChecklist;
                pChecklist += TEST_NfcFakeExtCopy(pChecklist);
                (pChecklist - 1)->tFlieAttr.bsLastFile = 1;

                /* start SLC2TLC_PERF timer */
                g_aStartTimer[SLC2TLC_PERF] = TEST_NfcStartTimer();
                    
                while(TRUE)
                {     
                    /* ONE CMD statistic timer, optional */
                    g_aStartTimer[PATT_GEN_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
                    
                    TEST_NfcExtPattGen();

                    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[PATT_GEN_ONE_CMD_PERF], g_aEndTimer[PATT_GEN_ONE_CMD_PERF]);
                
                    if (CHECK_LIST_FILE_NUM_MAX == ++a_ulCmdCnt[SLC2TLC_PERF])
                        break;
                
                    if (TRUE == TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr))
                    {
                        break;
                    }
                    
                    /* go to next check list if not last */
                    TEST_NfcGetNextChklistFile(EXT_CHK_LIST);       
                }
                
                /* end SLC2TLC_PERF timer */
                ulTimeDeltaInUs = TEST_NfcEndTimer(g_aStartTimer[SLC2TLC_PERF], g_aEndTimer[SLC2TLC_PERF], &ulTimeDelta);
                DBG_Printf("SLC2TLC_PERF: CmdCnt %d, cycle cnt 0x%x, cost time %d us\n\n", a_ulCmdCnt[SLC2TLC_PERF], ulTimeDelta, ulTimeDeltaInUs);

                ucPerfCase = SLC_SEQ_WT_PERF;

            }break;

            default:
            {
                DBG_Printf("Not support this performance test case %d!\n", ucPerfCase);
                DBG_Getch();
            }
        }
    }
    DBG_Printf("Performance Test Done!\n");
    while(1);

    return;
}

#else
/****************************************************************************
Function      : TEST_NfcMCU1Main
Input         :
Output        :
Description   : NFC driver test main function in MCU1
Reference     :
History       :
    20160904    abby    create
****************************************************************************/
void TEST_NfcMCU1Main(void)
{ 
    if (FALSE == TEST_MCU1EventCheck())//blocking event
    {
        return;
    }

    TEST_NfcCopyChecklist2SafeDram();

    /*  clr MCU1 event to enable MCU2 */
    CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_BOOT);

    U8 ucFileType;
    U32 ulCnt = 0;
    BOOL bLastFile = FALSE;
    LOCAL U32 l_ucBurnInLoop = 0;
 
    ucFileType = TEST_NfcGetCheckListFileType();
   
    /*  init llf table: BBT/VBT/PBIT, only do when boot up  */
    if ((EXT_CHK_LIST == ucFileType)&&(FALSE == g_bLocalBootUpOk))
    {
        TEST_NfcLocalLLF();
    }

#ifdef FAKE_BASIC_CHKLIST
    TEST_NfcFakeBasicChecklist((CHECK_LIST_BASIC_FILE*)g_pCheckListPtr);
#elif (defined(FAKE_EXT_CHKLIST))
    TEST_NfcFakeExtChecklist((CHECK_LIST_EXT_FILE*)g_pCheckListPtr);
#endif

    while(TRUE)
    {  
        if (BASIC_CHK_LIST == ucFileType)
        {
            TEST_NfcBasicPattGen();
        }
        else//ext file
        {
            TEST_NfcExtPattGen();
      
            if (CHECK_LIST_FILE_NUM_MAX == ++ulCnt)
                break;
        }
        
        bLastFile = TEST_NfcIsLastCheckListFile((CHECK_LIST_FILE_ATTR *)g_pCheckListPtr);
        if (bLastFile)
        {
        #ifndef ACC_EXT_TEST
            if (EXT_CHK_LIST == ucFileType)
            {
                /* handle remaining pending FCMD */
                TEST_NfcIsRemainPendingFCmd();

                /* handle remaining data check */
                TEST_NfcIsRemainDataCheckFCMDQ();
            }
        #endif
            break;
        }
        
        /* go to next check list if not last */
        TEST_NfcGetNextChklistFile(ucFileType);       
    }
    //DBG_Printf("NFC UT PATTERN GEN LOOP %d COMPLETE!\n",l_ucBurnInLoop++);

#ifdef BURN_IN_FOREVER
    g_pCheckListPtr = g_pFileBaseInDram;
#else
    while(1);
#endif

    return;
}
#endif


/* end of this file */
