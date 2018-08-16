/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    18:44:39
Description :L1 task schedule.
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"
//#include "../tracetool/Trace_Dev.h"
#include "L1_DSG.h"
#include "HAL_NormalDSG.h"
#ifdef SIM
#include "..\satamodel\simsatadev.h"
#endif

#ifdef SIM_DBG
U32   gDbgHcmdLba;
U32   gDbgHcmdLen;
#endif

#ifdef HCMD_PU_QUEUE
PU_CMD_Q gPuCmdQ[PU_NUM];
volatile U8 gPuToSelected;
volatile U8 gNextPuToSelect;
volatile U8 g_ucL1HCmdAddCnt;
volatile U8 g_ucL1HCmdHandleCnt;
#ifdef HCMD_PUQ_IN_OTFB
U32 gPuQCmdSlotBaseAddr;
#endif
#else
U32 LocalHCMDFIFO[NCQ_DEPTH + 1];
U32 LocalHCMDFIFOHead;
U32 LocalHCMDFIFOTail;
#endif

HCMD *gCurHCMD;

/* use for SubCmd */
U8      gCurSubCmd;
SUBCMD *gpCurSubCmd;
SUBCMD  gSubCmdEntry[SUBCMD_ENTRY_DEPTH];

/* use for Write Partial Hit */
SUBCMD  gPartialHitSubCmd[LPN_PER_BUF];
U8      gPartialHitCnt;
U8      gPartialHitBase;
U8      gPartialHitFlag;

/*  Buffer Status Counter */
L1_BUF_STATUS gBufStatus[PU_NUM];

U8  g_ucL1returnflag;

LOCAL U8  ucSUBCMDId;
LOCAL U8  g_ucCurrMergePU;
LOCAL U8  g_ucCurrFlushPU;
LOCAL U8  g_ucCurrRecyclePU;

U32 g_L1IdleCount;

LOCAL void L1_HostCMDOTFBMap(U32 *pFreeOTFBBase)
{

#ifdef HCMD_PUQ_IN_OTFB
  U32 ulFreeOTFBBase;  
  
  ulFreeOTFBBase = *pFreeOTFBBase;
  gPuQCmdSlotBaseAddr = ulFreeOTFBBase;
  
  //PuQ CmdSlot size:PU Total Count*(NCQ Total Count+1)*DWORD Size
  ulFreeOTFBBase += PU_NUM*(NCQ_DEPTH+1)*DWORD_SIZE;
  
  *pFreeOTFBBase = ulFreeOTFBBase;
#endif
  return;
}

void L1_HostCmdVarInit(void)
{
    U8 ucPuNum;
    U8 ucNcqDepth;
    
#ifdef HCMD_PU_QUEUE
    gPuToSelected = INVALID_2F;
      gNextPuToSelect = 0;
    g_ucL1HCmdAddCnt = 0;
    g_ucL1HCmdHandleCnt = 0;
#else
    LocalHCMDFIFOHead = 0;
    LocalHCMDFIFOTail = 0;
#endif

    gCurHCMD = NULL;
    gpCurSubCmd = NULL;
    gCurSubCmd = 0;

    g_ucCurrMergePU = 0;
    g_ucCurrFlushPU = 0;
    g_ucCurrRecyclePU = 0;

    for (ucPuNum = 0; ucPuNum < PU_NUM; ucPuNum++)
    {
        gBufStatus[ucPuNum].UsedCnt  = 0;
        gBufStatus[ucPuNum].MergeCnt = 0;
        gBufStatus[ucPuNum].FlushCnt = 0;
        gBufStatus[ucPuNum].RecycCnt = 0;

#ifdef HCMD_PU_QUEUE
            gPuCmdQ[ucPuNum].head = 0;
            gPuCmdQ[ucPuNum].tail = 0;
#ifdef HCMD_PUQ_IN_OTFB
        for (ucNcqDepth = 0; ucNcqDepth < (NCQ_DEPTH+1); ucNcqDepth++)
        {
            L1_SetPuQCmdTag(ucPuNum, ucNcqDepth, 0);
        }
#endif        
#endif
    }

    for (ucNcqDepth = 0; ucNcqDepth < NCQ_DEPTH; ucNcqDepth++)
    {
        HostCmdSlot[ucNcqDepth].ulCmdRemSector = 0;
        HostCmdSlot[ucNcqDepth].ucCmdStatus    = HCMD_STATE_NONE;
    }

    gPartialHitFlag = PARTIAL_HIT_NONE;
    
    g_L1IdleCount = 0;
}

void L1_HostCMDProcInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    L1_HostCMDOTFBMap(pFreeOTFBBase);

    L1_HostCmdVarInit();

    return;
}

/****************************************************************************
Function  : L1TaskAddNewHostCMD
Input     : HCMD *newCmd --a host cmd
         U8 ucTag -- command tag
Output    : Status of whether a HostCMD is added in the HostCMDFIFO
Return    :  Success , Fail
Purpose   : save host cmd to command array.
Reference :
The task is called in interrupt.
Modification History:
20120209   Brooke Wang create detailed code
20120118   peterxiu   001 first create function
****************************************************************************/
U8 L1_HostCMDGetCount(void)
{
    U8 HCmdCnt;

#ifdef HCMD_PU_QUEUE
    if (g_ucL1HCmdAddCnt >= g_ucL1HCmdHandleCnt)
    {
        HCmdCnt = g_ucL1HCmdAddCnt - g_ucL1HCmdHandleCnt;
    }
    else
    {
        HCmdCnt = (NCQ_DEPTH + 1) + g_ucL1HCmdAddCnt - g_ucL1HCmdHandleCnt;
    }
#else
    if(LocalHCMDFIFOTail >= LocalHCMDFIFOHead)
    {
        HCmdCnt = LocalHCMDFIFOTail - LocalHCMDFIFOHead;
    }
    else 
    {
        HCmdCnt = (NCQ_DEPTH + 1) + LocalHCMDFIFOTail - LocalHCMDFIFOHead;
    }
#endif

    return HCmdCnt;
}

BOOL  L1_AddNewHostCMD(U8 ucNewCmdTag)
{
    U32 ulNextTail ;
    //U8 ucIndex;

    //char HostCMD_debug_buffer[256];

    /* copy newcmd to slot */

    /*
    *( (U32*)(&HostCmdSlot[ucCmdTag])+0) =  *((U32*)(NewCmd)+0);
    *( (U32*)(&HostCmdSlot[ucCmdTag])+1) =  *((U32*)(NewCmd)+1);  
    *( (U32*)(&HostCmdSlot[ucCmdTag])+2) =  *((U32*)(NewCmd)+2);  
    *( (U32*)(&HostCmdSlot[ucCmdTag])+3) =  *((U32*)(NewCmd)+3);  

    */
    /* Push tag to FIFO*/
    ulNextTail = LocalHCMDFIFOTail +1 ;
    if(ulNextTail == (NCQ_DEPTH+1))
    {
        ulNextTail =0 ;
    }

    /* if the FIFO is full, return */
    if(M_HCMDFIFO_FULL(LocalHCMDFIFOHead, ulNextTail) )
    {
        /*
        #ifdef  SIM_HOST_CMD_DBG
        sprintf_s(HostCMD_debug_buffer, 200, "[L1_AddNewHostCMD ] FULL\n");
        WriteFile(hDebugLogFile,(LPVOID)(HostCMD_debug_buffer), strlen(HostCMD_debug_buffer), &haswrite,  NULL);
        #endif      
        */
        return (FAIL) ;
    }
    /*
    ucIndex = LocalHCMDFIFOHead;
    while (ucIndex != LocalHCMDFIFOTail)
    {
    if (LocalHCMDFIFO[ucIndex] == ucCmdTag)
    {
    break;
    }

    ucIndex++;
    if(ucIndex == (NCQ_DEPTH+1))
    {
    ucIndex =0 ;
    }
    }
    */
    LocalHCMDFIFO[LocalHCMDFIFOTail] = ucNewCmdTag ;

    LocalHCMDFIFOTail= ulNextTail ;

#ifdef  SIM_DBG
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "[L1_AddNewHostCMD]: add new host cmd to fifo!, Tail = %d\n", LocalHCMDFIFOTail);
#endif

    return (SUCCESS);

}


/****************************************************************************
Function  : L1TaskHostCMDSelect
Input     : none
Output    :  if HostCmd FIFO is empty,return FAIL; else return HostCmdSlot[ucCmdTag] 
Return    :
Purpose   : Select a HostCMD to serve from HostCMDFIFO
Reference :
Modification History:
20120209   Brooke Wang create detailed code
20120118   peterxiu   001 first create function
****************************************************************************/
HCMD * L1_HostCMDSelect()
{
    U8   ucCmdTag = 0;
    U32 ulNextHead ;

    /* When get CMD from HostCMDFIFO, need to Disable CPU INT first to avoid CMD is adding to FIFO simultanously */
    // CPU_DisableIntAck() ;

    /*Pop a tag from FIFO */
    /* if no CMD is the FIFO, return */  
#ifdef SIM
    EnterCriticalSection(&g_CMDQueueCriticalSection);
#endif
    if(M_HCMDFIFO_EMPTY(LocalHCMDFIFOHead, LocalHCMDFIFOTail))
    {
        // CPU_EnableIntAck() ;
#ifdef SIM
        LeaveCriticalSection(&g_CMDQueueCriticalSection);
#endif
        return (NULL) ;
    }

    ulNextHead =  LocalHCMDFIFOHead + 1;

    if( ulNextHead == (NCQ_DEPTH+1))
    {
        ulNextHead =0 ;
    }
    //  CPU_EnableIntAck() ;

    ucCmdTag = LocalHCMDFIFO[LocalHCMDFIFOHead];

#ifdef  SIM_DBG
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "\nHCMD FIFO Current head = %d, Tail = %d \n", LocalHCMDFIFOHead, LocalHCMDFIFOTail);
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "CmdLBA = 0x%xh, ulCmdSectorCnt=%d, ucCmdTag=%d \n",HostCmdSlot[ucCmdTag].ulCmdLba, HostCmdSlot[ucCmdTag].ulCmdSectorCnt, HostCmdSlot[ucCmdTag].ucCmdTag);
#endif


 //   WRITE_DBG_LOG_INFO(g_DbgLogInfo,1,(LocalHCMDFIFOHead|(LocalHCMDFIFOTail<<8)),HostCmdSlot[ucCmdTag].ulCmdLba,HostCmdSlot[ucCmdTag].ulCmdSectorCnt,HostCmdSlot[ucCmdTag].ucCmdTag);
 //   FW_L1_LogInfo_Trace(LOG_TRACE_ALL,3,LOG_LVL_INFO,&g_DbgLogInfo);
 //   RecordFlag(0,3,1,"HCMD FIFO Current head = %d[23:16], Tail = %d[31:24],CmdLBA = %x[63:32], ulCmdSectorCnt=%d[95:64], ucCmdTag=%d[127:96]\n");

    LocalHCMDFIFOHead  =  ulNextHead;

#ifdef SIM
    LeaveCriticalSection(&g_CMDQueueCriticalSection);
#endif

    return (&HostCmdSlot[ucCmdTag]);


}


/****************************************************************************
Function  : L1_SplitHCMD
Input     : pTgtHCMD
Output    : pTgtSUBCMD
Return    :  TRUE, current HCMD split done, ulCmdRemSector == 0;
                FALSE: current HCMD split ongoing, ulCmdRemSector != 0;
                
Purpose   : (1) split a host command to flash page size (16K/32K) boundary
Reference : 
Modification History:
20121121   Blake Zhang  remove SubCmd Fifo
20120209   Brooke Wang create detailed code
20120118   peterxiu   001 first create function
****************************************************************************/

SUBCMD* L1_SplitHCMD(HCMD *pTgtHCMD)
{
    U8   ucLoop;
    U8   ucSUBCMDLength;
    U16  usActalSubCmdLength;
    U8   ucVirtualSUBCMDLength;
    U8   ucCMDOffset;
    U32  ulStartLBA;
    SUBCMD* pTgtSUBCMD;

#ifdef SIM
    if(pTgtHCMD == NULL)
    {
        DBG_Printf("L1_SplitHCMD invalid input ERROR\n");
        DBG_Getch();
    }
#endif

    /*  First Check whether Host usCmdRemSector ==0 */
    if(0 == pTgtHCMD->ulCmdRemSector)
    {
        return NULL;
    }

    /* SubCmdEntry is only for debug to log recent SubCmds */
    pTgtSUBCMD = &gSubCmdEntry[gCurSubCmd];
    
    gCurSubCmd++;
    if (gCurSubCmd == SUBCMD_ENTRY_DEPTH)
    {
        gCurSubCmd = 0;
    }

    /* Then Initialize this SUBCMD */
    for (ucLoop = 0; ucLoop < SUBCMD_SIZE; ucLoop++)
    {
      *((U32*)(pTgtSUBCMD) + ucLoop) = 0;
    }

    /* (0)  Fill in the according field of subcmd from hcmd */
    pTgtSUBCMD->pHCMD = pTgtHCMD;
    pTgtSUBCMD->SubDSGId = INVALID_2F;
    pTgtSUBCMD->SubNextDsgId = INVALID_2F;

    /* (1) get the host cmd  start LBA in this subcmd */
    ulStartLBA = pTgtHCMD->ulCmdLba + pTgtHCMD->ulCmdSectorCnt - pTgtHCMD->ulCmdRemSector;

    /* (2) get the StartLBA of this SUBCMD, each SUBCMD is split by max_flash_page_size(buffer size) 16K/32K */
    pTgtSUBCMD->SubCmdAddInfo.ulSubCmdLBA = L1_CalcSubLBAForL1(ulStartLBA);
    

    /* (3) Get the SubCmdOffset & SubCmdLength  of this SUBCMD*/
    pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN = SUBCMD_OFFSET(ulStartLBA);

    ucCMDOffset = pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN;

    if ((ucCMDOffset + pTgtHCMD->ulCmdRemSector) > SEC_PER_BUF)
    {
        //SEC_PER_BUF=256, Value Overflow: ucSubCmdlengthIN:[1~255,0] ---> actualLenIN:[1~255,256]
        pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN = SEC_PER_BUF - pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN;
    }
    else
    {
        pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN = (U8)pTgtHCMD->ulCmdRemSector;
    }

    if (0 == pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN)
    {
        usActalSubCmdLength = SEC_PER_BUF;
    }
    else
    {
        usActalSubCmdLength = pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN;
    }
    
    pTgtHCMD->ulCmdRemSector -= usActalSubCmdLength; 
        
    /* (4) Get the LPNoffset & LPN count of this SUBCMD*/    
    /*first make cmdlength LPN=4K align*/

    pTgtSUBCMD->SubCmdAddInfo.ucSubLPNOffsetIN = SUBCMDLPN_OFFSET(ucCMDOffset);

    ucVirtualSUBCMDLength = SUBCMD_OFFSET_IN_LPN(ucCMDOffset) + usActalSubCmdLength;
    pTgtSUBCMD->SubCmdAddInfo.ucSubLPNCountIN = SUBCMDLPN_COUNT(ucVirtualSUBCMDLength);

    /*(4)  Configure SUBCMD  First, Last and ID */

    pTgtSUBCMD->SubCmdFirst = (ulStartLBA == pTgtHCMD->ulCmdLba);

    if(pTgtSUBCMD->SubCmdFirst)
    {
        ucSUBCMDId = 0 ;
        #ifdef SIM_DBG
        gDbgHcmdLba = pTgtSUBCMD->SubCmdAddInfo.ulSubCmdLBA + pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN;
        gDbgHcmdLen = 0;
        #endif
    }

    pTgtSUBCMD->SubCmdId = ucSUBCMDId;
    ucSUBCMDId++;  

    #ifdef SIM_DBG
    gDbgHcmdLen = gDbgHcmdLen + usActalSubCmdLength;
    #endif

    if(pTgtHCMD->ulCmdRemSector == 0)
    {
        pTgtSUBCMD->SubCmdLast = TRUE;

#ifdef SIM_DBG
        if((gDbgHcmdLen == pTgtHCMD->ulCmdSectorCnt)&&(gDbgHcmdLba == pTgtHCMD->ulCmdLba))
        {
            ;
        }
        else
        {
            DBG_Printf(" SubCmd split Error !! \n");
            DBG_Getch();
        }
#endif
    }
    else
    {
        pTgtSUBCMD->SubCmdLast = FALSE;
    }

    L1_SubCmdGetLpnSectorMap(pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN, 
            pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN, &(pTgtSUBCMD->LpnSectorBitmap[0]));

#ifdef SIM_DBG
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "L1_SplitHCMD TAG %d LBA 0x%x RW %d (SOff %d SCnt %d LOff %d LCnt %d) F %d L %d\n", 
      pTgtSUBCMD->pHCMD->ucCmdTag, pTgtSUBCMD->pHCMD->ulCmdLba, pTgtSUBCMD->pHCMD->ucCmdRW, 
      pTgtSUBCMD->SubCmdAddInfo.ucSubCmdOffsetIN, pTgtSUBCMD->SubCmdAddInfo.ucSubCmdlengthIN,
      pTgtSUBCMD->SubCmdAddInfo.ucSubLPNOffsetIN, pTgtSUBCMD->SubCmdAddInfo.ucSubLPNCountIN,
      pTgtSUBCMD->SubCmdFirst, pTgtSUBCMD->SubCmdLast);
#endif

#ifdef HCMD_PU_QUEUE
        gNextPuToSelect++;
        if (PU_NUM == gNextPuToSelect)
    {
              gNextPuToSelect = 0;
        }
#endif

    pTgtSUBCMD->SubCmdStage = SUBCMD_STAGE_SPLIT;
    return pTgtSUBCMD;
}

/****************************************************************************
Name        :L1_TaskSataIO
Input       :pSubCmd
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:30:33
Description :
Others      :
Modify      :2012.11.22  modify for L1 ramdisk desgin
****************************************************************************/
BOOL L1_TaskSataIO(SUBCMD* pSubCmd)
{
    HCMD *pHCMD;
    U16 usLogicBufId = 0;
    U8 ucDSGId;
    
    if (SATA_PIO_NOCMD != gPIOInfoBlock.ucCurrPIOState)
    {
#ifdef SIM_XTENSA
        rTraceData = TL_PIO_STAGE;
#endif
        /* PIO Step 3/3: process */
        L1_SataHandlePIODataProtocol();
    }

    if(pSubCmd == NULL)
    {
        /*no subcmd or prd resource */
        return FALSE;
    }

    if (FALSE == L1_CheckDSGResource(gpCurSubCmd))
    {
#ifdef SIM_XTENSA
        rTraceData = TL_SATAIO_CK_DSG | 0xffff;
#endif
        return FALSE;
    }

#ifdef HCMD_PU_QUEUE
    if (gPartialHitFlag == PARTIAL_HIT_NONE && pSubCmd->SubCmdFirst == TRUE)
    {
          L1_MoveHCmdPuHeader();
    }
#endif
    
#ifdef SIM_XTENSA
    rTraceData = TL_SATAIO_CK_DSG | gpCurSubCmd->SubDSGId;
#endif

    pHCMD = pSubCmd->pHCMD;
    usLogicBufId = LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID);

    if(HCMD_READ == pHCMD->ucCmdRW)
    {
        ucDSGId = HAL_SataBuildReadDSG(pSubCmd);
    }
    else if(HCMD_WRITE == pHCMD->ucCmdRW)
    {        
        ucDSGId = HAL_SataBuildWriteDSG(pSubCmd);
    }
    else
    {
        DBG_Printf("L1_TaskSataIO pHCMD->ucCmdRW %d ERROR!!\n", pHCMD->ucCmdRW);
        DBG_Getch();
    }
    
    /* PIO step 2/3: Mark the beginning of a PIO data command */
    if ((HCMD_PROTOCOL_PIO == pHCMD->ucCmdProtocol) && (TRUE == pSubCmd->SubCmdFirst)) 
    {
        gPIOInfoBlock.pCommand = pHCMD;
        gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NEWCMD;
    }

#ifdef SIM_XTENSA
    rTraceData = TL_SATAIO_BD_DSG | gpCurSubCmd->SubDSGId;
#endif

    if(INVALID_2F == ucDSGId)
    {
        if (HCMD_PROTOCOL_PIO != pHCMD->ucCmdProtocol)
        {
            DBG_Printf("L1_TaskSataIO PRD ERROR!!\n");
            DBG_Getch();
        }
    
        return FALSE;
    }

#ifdef SIM_DBG
    FIRMWARE_L1_LogInfo(LOG_FILE, 0, "L1_TaskSataIO TAG %d PRD built stage SATAIO\n", pSubCmd->pHCMD->ucCmdTag);
#endif

    g_ucL1returnflag = 5;
    pSubCmd->SubCmdStage = SUBCMD_STAGE_SATAIO;
    return TRUE;
}

/****************************************************************************
Name        :L1_TaskRecycle
Input       :pSubCmd
Output      :void
Author      :Blakezhang
Date        :2012.11.25
Description :recycle burrent subcmd.
Others      :
Modify      :
****************************************************************************/
void L1_TaskRecycle(SUBCMD* pSubCmd)
{
    return;
}

/****************************************************************************
Name        :L1_TaskMergeFlushManagement
Input       :
Output      :
Return      : TRUE, a request is send; FALSE, no request is send
Author      :Blakezhang
Date        :2012.11.22
Description :managment the cache merge and flush operations in PU Fifo
Others      :
Modify      :
****************************************************************************/
BOOL L1_TaskMergeFlushManagement(void)
{
    return TRUE;
}

/********************** FILE END ***************/

