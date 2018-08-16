/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_SataIO.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Sata interface function
Others      :
Modify      :
****************************************************************************/

#include "BaseDef.h"

#ifndef SIM
#include <xtensa/tie/via.h>
#include <xtensa/tie/xt_core.h>
#endif
#include "HAL_SataDSG.h"
#include "HAL_Define.h"
#include "HAL_BufMap.h"
#include "HAL_SataDefine.h"
#include "HAL_SataIO.h"
#include "HAL_Buffer.h"
#include "HAL_SataCmdDefine.h"
#include "HAL_GLBReg.h"
#include "L1_SataCmd.h"
#include "L1_Define.h"
#include "L1_HostCmdPRO.h"
#include "L1_DSG.h"
#include "L1_NcqNonData.h"
#include "L1_ErrorHandling.h"
//#include "L1_HostCmdPRO.h"
//#include "HAL_SataIO.h"


volatile SATA_PRD_ENTRY * g_pSataReadPRD;
volatile SATA_PRD_ENTRY * g_pSataWritePRD;

volatile U32 * g_pCacheStatus;

volatile SDC_PRD_CONTROL* g_pSataPRDControlReg;

HCMD HostCmdSlot[NCQ_DEPTH];
extern U8  g_ulEncounterError;
extern U32 g_ulDramFreeAddr;
extern PIOINFO gPIOInfoBlock;
/*
SATA Phy 
Based on VT3514B0 ASIC TEST
*/
static void Sata_PHYConfig(void)
{
    // PHY 
    rGLB(0x3c60) = 0x00034000;
    rGLB(0x3c64) = 0x00e60015;
    rGLB(0x3c68) = 0x0157001b;
    rGLB(0x3c14) = 0x0f80e808;

    // GEN1 Rx 
    rGLB(0x3c20) = 0x0c6c0000;
    rGLB(0x3c24) = 0x00000000;
    rGLB(0x3c28) = 0xaf50faaa;

    // GEN2 Rx 
    rGLB(0x3c34) = 0x0c6c0000;
    rGLB(0x3c38) = 0x00000000;
    rGLB(0x3c40) = 0xa550faaa;

    // GEN3 Rx
    rGLB(0x3c48) = 0x0c6c0000;
    rGLB(0x3c4c) = 0x00000092;
    rGLB(0x3c54) = 0xa050ffff;

    // SDC setting 
    rGLB(0x804) = 0x0;
    rGLB(0x800) = (1<<29) ;
    rGLB(0x804) &= 0x18000000;
    rGLB(0x804) |= (1<<17);
    rGLB(0x810) = (1<<2);

    return;
}

void HAL_ResetSDC(void)
{
    // reset GLB Register
    rGLB_18 |= R_RST_SDC_DMA | R_RST_SDC_CMD; 
    rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_SDC_CMD);
}

void HAL_ResetBuffMap(void)
{
    rGLB_18 |= R_RST_BUFM; 
    rGLB_18 &= ~R_RST_BUFM;
}

void HAL_HoldBuffMap(void)
{
    rGLB_18 |= R_RST_BUFM; 
}

void HAL_ReleaseBuffMap(void)
{
    rGLB_18 &= ~R_RST_BUFM;
}

/****************************************************************************
Name        :HAL_SataInitialize
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.15    15:11:36
Description :Initialize sata.
Others      :
Modify      :
****************************************************************************/
void HAL_SataInitialize(void)
{
    /*
    SDC_BASE_ADDRESS  0x14[7] = 1 :receive error command fis, do not interrupt FW.
    SDC_BASE_ADDRESS  0x11[0] = 1 :receive error command fis, interrupt FW.
    */
    *(volatile U8 *)(SDC_BASE_ADDRESS + 0x11) |= 1;
    *(volatile U8 *)(SDC_BASE_ADDRESS + 0x14) |= (1<<7);

    /* COM_Reset Block disable */
   // *(volatile U32*)(0x1ff81f10) &= ~(0x1 << 20); 
    
    /* Initialize SDMAC */
    rSDMAC_DataBuffBase = BUFFSIZE_32K | DUMMYDATA | HOLDXFER | BYPASSEN;/*|SDBFISwDMA */

    /* Set Sata mode enable */
    rSGESataMode |= 1;


    /* DMAEXE_STE_EN(HW will wait SDBFISReady when DMA command )*/
    rSDC_ControlRegister = FW_DMAEXE_STE_EN;

    //rSDC_FW_Ctrl |= HD_Err_clr;

    //rSDC_SHR_LockControl |= BIT_FW_CMDINT_EN;
    
#ifndef FW_CTRL_ALL_SDBFISREADY
    /* SDB FIS ready, default all valid */
    rSDC_SendSDBFISReady = 0xffffffff;
#endif

#ifdef FIS_Delay
/*************FISDelay set***********************************/
    rSDC_FISDelayControl |= TXSETDEVCOMPL_DLY_EN | 0xC0;

#endif
    //g_pSataPRDControlReg = (SDC_PRD_CONTROL*)SDC_PRD_CONTROL_BASE;

    /* Initialize SACMDM */
    /* config r/w PRD in SRAM */
    //rSACMDM_RPRDBaseAddr = RPRD_BASE - OTFB_START_ADDRESS;
    //rSACMDM_WPRDBaseAddr = WPRD_BASE - OTFB_START_ADDRESS;
    //g_pSataReadPRD = (SATA_PRD_ENTRY*)RPRD_BASE;
    //g_pSataWritePRD = (SATA_PRD_ENTRY*)WPRD_BASE;
    //enable PRD in SRAM
    rTOP_REGGLB40 |= (1 << 26);

    /* Initialize SDC */
#if 0
    rSDC_IntMask |= (BIT_SDC_INTSRC_FIS_CONTROL | 
                    BIT_SDC_INTSRC_TXOP|
                    BIT_SDC_INTSRC_RXDMACMPL|
                    BIT_SDC_INTSRC_SERROR);
#endif
    rSDC_IntMask |= (BIT_SDC_INTSRC_FIS_CONTROL | 
                        BIT_SDC_INTSRC_TXOP|
                        BIT_SDC_INTSRC_RXDMACMPL|
                        BIT_SDC_INTSRC_LPM);

    /* Disables hardware report PMACK to HIPM requests */
    rSDC_LinkControl |= 0x0300;


    /* cache status in OTFB */
    //rSACMDM_CACHE_STATUS_BASE_ADDRESS_SRAM = (U32)(CACHE_STATUS_BASE_OTFB - OTFB_START_ADDRESS);
    //g_pCacheStatus  = (U32*)OTFB_RAMDISK_CS;//CACHE_STATUS_BASE_OTFB;
#ifdef VT3514_C0
    *(volatile U32*)(0x1ff80804) |= (3<<27); //sata force GEN1 0x1ff80804 bit [28:27] to 2'b11  
#endif
    /* Release reset signal for SATA */
    rGLB_18 &= ~(R_RST_SDC_DMA | R_RST_LPHY | R_RST_SDC_CMD | R_RST_SDC);

    return;
}


#if 0
void HAL_SataGetPRDControl()
{

#ifdef HW_TIE_ENABLE
    U32 uControlReg;    
    uControlReg = *(U32*)g_pSataPRDControlReg;
    _TIE_via_WUR_tiereg_str_prdreg_dw0(uControlReg);
#endif

    return;
}
#endif


/****************************************************************************
Name        :HAL_SataIsReadPRDFull
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.14    17:23:26
Description :
Others      :
Modify      :
****************************************************************************/
BOOL HAL_SataIsReadPRDFull(void)
{
    return FALSE;
}

/****************************************************************************
Name        :HAL_SataIsReadPRDEmpty
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.14    19:51:31
Description :
Others      :
Modify      :
****************************************************************************/
BOOL HAL_SataIsReadPRDEmpty(void)
{
    return FALSE;
}

/****************************************************************************
Name        :HAL_SataGetNextReadPRD
Input       :void
Output      :Read PRD Id.
Author      :HenryLuo
Date        :2012.02.14    17:18:30
Description :get next read prd id.
Others      :
Modify      :
****************************************************************************/
U8    HAL_SataGetNextReadPRD(void)
{
    return 0;
}

/****************************************************************************
Name        :HAL_SataTriggerReadPRD
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.02.14    19:17:08
Description :trigger read PRD.
Others      :
Modify      :
****************************************************************************/
void HAL_SataTriggerReadPRD(void)
{

#ifdef SIM
    //SDC_AllocateReadPrd();
#else
    //rSACMDM_RPRDReadPtr_Trigger = 0x5A;
    XT_MEMW();
#endif
    return;
}

/****************************************************************************
Name        :HAL_SataTriggerWritePRD
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.02.20    19:05:57
Description :trigger write PRD.
Others      :
Modify      :
****************************************************************************/
void HAL_SataTriggerWritePRD(void)
{

#ifdef SIM
    //SDC_AllocateWritePrd();
#else
    //rSACMDM_WPRDWritePtr_Trigger = 0x5A;
    XT_MEMW();
#endif
    return;
}

/****************************************************************************
Name        :HAL_SataIsWritePRDFull
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.14    19:29:56
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/
BOOL HAL_SataIsWritePRDFull(void)
{
#ifdef HW_TIE_ENABLE
    return _TIE_via_VIATIE_SATA_WPRD_FULL();
#else
    /*if( 0 == ( rSACMDM_WPRDWritePtr & BIT_SACMDM_WPRD_LIST_FULL_FLAG ))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }*/
    return FALSE;
#endif
}

/****************************************************************************
Name        :HAL_SataIsWritePRDEmpty
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.14    19:52:39
Description :
Others      :I am hungry now. 
Modify      :
****************************************************************************/
BOOL HAL_SataIsWritePRDEmpty(void)
{
    return FALSE;
}

/****************************************************************************
Name        :HAL_SataGetNextWritePRD
Input       :void
Output      :
Author      :HenryLuo
Date        :2012.02.14    19:29:00
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/
U8 HAL_SataGetNextWritePRD(void)
{
    return 0;
}
/*
void HAL_SetParamNCQNonData(HCMD* pHostCmd)
{
    U8  ucSubCmd;

    ucSubCmd = rSDC_SHRLCH_FEATURE8;

    pHostCmd->ucNCQSubCmd = ucSubCmd & 0xF;

    switch(pHostCmd->ucNCQSubCmd)
    {
        case NND_SUBCMD_ABORT_NCQQ:
        case NND_SUBCMD_DL_HANDLING:
        case NND_SUBCMD_SET_FEATURES:
             pHostCmd->ulCmdLba       = rSDC_SHRLCH_LBA48;
             pHostCmd->ulSHRFeature16 = rSDC_SHRLCH_FEATURE16;
             pHostCmd->ulSHRCount16   = rSDC_SHRLCH_SECCNT16;
             break;
        case NND_SUBCMD_HB_DEMOTE_BY_SIZE:
        case NND_SUBCMD_HB_CHANGE_BY_LBA:
        case NND_SUBCMD_HB_CONTROL:
        default:
             L1_ErrHandle_NotSupportCmd();
             //pHostCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;   //for hanging HW's NCQ state machine             
             break;
    }
    return;
}


void HAL_SetParamSendFPDMA(HCMD* pHostCmd)
{
    U16  usSubCmd;

    usSubCmd = rSDC_SHRLCH_SECCNT16;

    pHostCmd->ucNCQSubCmd = (usSubCmd >> 8) & 0x1F;

    switch(pHostCmd->ucNCQSubCmd)
    {
        case NCQ_SEND_SUBCMD_SET_MANAGEMENT:
        case NCQ_SEND_SUBCMD_WRITE_LOG:
             pHostCmd->ulCmdLba       = rSDC_SHRLCH_LBA48;
             pHostCmd->ulCmdSectorCnt = rSDC_SHRLCH_FEATURE16;
             pHostCmd->ulSHRCount16   = rSDC_SHRLCH_SECCNT16;
             pHostCmd->ucSubCmdSpec   = (U8)rSDC_SHRLCH_AUXILIARY;
             break;
        case NCQ_SEND_SUBCMD_HB_EVICT:
        default:
             L1_ErrHandle_NotSupportCmd();
             //pHostCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;   //for hanging HW's NCQ state machine             
             break;
    }
    return;
}


void HAL_SetParamReceiveFPDMA(HCMD* pHostCmd)
{
    U16  usSubCmd;

    usSubCmd = rSDC_SHRLCH_SECCNT16;

    pHostCmd->ucNCQSubCmd = (usSubCmd >> 8) & 0x1F;

    switch(pHostCmd->ucNCQSubCmd)
    {
        case NCQ_RECV_SUBCMD_READ_LOG:
             pHostCmd->ulCmdLba       = rSDC_SHRLCH_LBA48;
             pHostCmd->ulCmdSectorCnt = rSDC_SHRLCH_FEATURE16;
             pHostCmd->ulSHRCount16   = rSDC_SHRLCH_SECCNT16;
             break;
        case NCQ_RECV_SUBCMD_RESERVED:
        default:
             L1_ErrHandle_NotSupportCmd();
             //pHostCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;   //for hanging HW's NCQ state machine             
             break;
    }
    return;
}
*/


/****************************************************************************
Name        :HAL_SataSetTransferParamNCQ
Input       :HCMD* pHostCmd
Output      :void
Author      :HenryLuo
Date        :2012.03.07    16:23:12
Description :Set parameters for NCQ cmd.
Others      :
Modify      :
****************************************************************************/
void HAL_SataSetTransferParamNCQ(HCMD* pHostCmd)
{
    U32 ulActualSecCount;

    ulActualSecCount = rSDC_SHRLCH_FEATURE16;
    if(0 == ulActualSecCount)
    {
        ulActualSecCount = 65536;
    }

    pHostCmd->ulCmdLba = rSDC_SHRLCH_LBA48;
    pHostCmd->ulCmdSectorCnt = ulActualSecCount;
    pHostCmd->ulCmdRemSector = ulActualSecCount;

    //    DBG_Printf("%x HCMD LBA\n",rSDC_SHRLCH_LBA48);

    return;
}

/****************************************************************************
Name        :HAL_SataSetTransferParam48
Input       :HCMD* pHostCmd
Output      :void
Author      :HenryLuo
Date        :2012.03.07    16:29:41
Description :Set parameters for 48 bit LBA cmd.
Others      :
Modify      :
****************************************************************************/
void HAL_SataSetTransferParam48(HCMD* pHostCmd)
{
    U32 ulActualSecCount;

    ulActualSecCount = rSDC_SHRLCH_SECCNT16;

    if(0 == ulActualSecCount)
    {
        ulActualSecCount = 65536;
    }
    pHostCmd->ulCmdLba = rSDC_SHRLCH_LBA48;
    pHostCmd->ulCmdSectorCnt = ulActualSecCount;
    pHostCmd->ulCmdRemSector = ulActualSecCount;

    return;
}

/****************************************************************************
Name        :HAL_SataSetTransferParam28
Input       :HCMD* pHostCmd
Output      :void
Author      :HenryLuo
Date        :2012.03.07    16:37:27
Description :Set parameters for 28 bit LBA cmd.
Others      :
Modify      :
****************************************************************************/
void HAL_SataSetTransferParam28(HCMD* pHostCmd)
{
    U32 ulActualLBA;
    U32 ulActualSecCount;
    U8 ucHeadNum, ucSecNum;
    U16 usCylinNum;

    if(0 != rSDC_SHRLCH_DEV6)
    {
        ulActualLBA = rSDC_SHRLCH_LBA28;
    }
    else
    {
        //For CHS command format
        ucHeadNum = rSDC_DEVICE_HEAD & 0xF;
        ucSecNum = rSDC_LBALOW;
        usCylinNum = (rSDC_LBAHIGH << 8) + rSDC_LBAMID;
        ulActualLBA = ( usCylinNum * g_pSataIdentifyData[55] + ucHeadNum ) * g_pSataIdentifyData[56] + ucSecNum - 1;
    }

    ulActualSecCount = (0 == rSDC_SHRLCH_SECCNT8) ? 256 : rSDC_SECCNT;

    pHostCmd->ulCmdLba = ulActualLBA;
    pHostCmd->ulCmdSectorCnt = ulActualSecCount;
    pHostCmd->ulCmdRemSector = ulActualSecCount;

    return;
}

/*==============================================================================
Func Name  : HAL_SataFeedbacktoHW
Input      : HCMD* pHCmd  
Output     : NONE
Return Val : 
Discription: when receive a host command, fw shall feedback to the hardware
             several information. including:
             1. decode what protocol of this host command.
             2. which tag of this host command used.
             3. clear big busy. 
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
==============================================================================*/
void HAL_SataFeedbacktoHW(HCMD* pHCmd)
{
    /* FW Decode program : command tag & command protocol */
    if (0 == g_ulEncounterError)
    {
        rSDC_FW_DECODE = (pHCmd->ucCmdTag << 3) | pHCmd->ucCmdProtocol;
    }

    /* clear big busy, inform hardware that command receiving has been completed */
    rSDC_FW_Ctrl |= FW_CFGCMD_DONE;

    /* Reset hardware NCQ state machine to idle */
    if (HCMD_PROTOCOL_NCQ != pHCmd->ucCmdProtocol)
    {
        rSDC_FW_Ctrl |= FW_CLR_NCQEXE;
    }
    
}

/****************************************************************************
Name        :HAL_SataGetCmdFromSRB
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.03.07    15:33:52
Description :get cmd para from Shadow Register Block registers.
Others      :
Modify      :
****************************************************************************/
U8 HAL_SataGetCmdFromSRB()
{
    U8 ucCmdCode;
    U8 ucCmdTag;
    HCMD* pNewCmd;

    ucCmdCode = rSDC_SHRLCH_COMMAND;
/*
    if (  (ATA_CMD_READ_FPDMA_QUEUED == ucCmdCode) 
        ||(ATA_CMD_WRITE_FPDMA_QUEUED == ucCmdCode)
        ||(ATA_CMD_NCQ_NON_DATA == ucCmdCode)
        ||(ATA_CMD_SEND_FPDMA_QUEUED == ucCmdCode)
        ||(ATA_CMD_RECEIVE_FPDMA_QUEUED == ucCmdCode))
    {
        ucCmdTag = rSDC_SHRLCH_NCQTAG;
        
        if (L1_ErrHandle_CheckOverriddenCmdTag(ucCmdTag))
        {
        #ifdef SIM_XTENSA
            rTracer = TL_SATA_INTERRUPT_ENTRY + (1<<25) + (ucCmdCode<<8) + ucCmdTag;
        #endif
            return 0;
        }
    }
    else   */
    {
        ucCmdTag = 0;
    }
    
#ifdef SIM_XTENSA
    rTracer = TL_SATA_INTERRUPT_ENTRY + (1<<24) + (ucCmdCode<<8) + ucCmdTag;
#endif

    pNewCmd = &HostCmdSlot[ucCmdTag];

#ifdef SIM
    if (HCMD_STATE_NONE != pNewCmd->ucCmdStatus)
    {
        DBG_Getch();
    }
#endif

    pNewCmd->ucCmdStatus = HCMD_STATE_RECEIVED;
    pNewCmd->ucCmdCode = ucCmdCode;
    pNewCmd->ucCmdTag = ucCmdTag;

    switch( ucCmdCode )
    {
    case ATA_CMD_READ_FPDMA_QUEUED:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_NCQ;
        pNewCmd->ucCmdRW = HCMD_READ;
        HAL_SataSetTransferParamNCQ(pNewCmd);
        break;

    case ATA_CMD_WRITE_FPDMA_QUEUED:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_NCQ;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SataSetTransferParamNCQ(pNewCmd);
        break;

    case ATA_CMD_READ_DMA:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_DMA;
        pNewCmd->ucCmdRW = HCMD_READ;
        HAL_SataSetTransferParam28(pNewCmd);
        break;

    case ATA_CMD_WRITE_DMA:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_DMA;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SataSetTransferParam28(pNewCmd);
        break;

    case ATA_CMD_READ_DMA_EXT:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_DMA;
        pNewCmd->ucCmdRW = HCMD_READ;
        HAL_SataSetTransferParam48(pNewCmd);
        break;

    case ATA_CMD_WRITE_DMA_EXT:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_DMA;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SataSetTransferParam48(pNewCmd);
        break;

    case ATA_CMD_READ_SECTOR:
    case ATA_CMD_READ_MULTIPLE:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        pNewCmd->ucCmdRW = HCMD_READ;
        HAL_SataSetTransferParam28(pNewCmd);
        break;

    case ATA_CMD_WRITE_SECTOR:
    case ATA_CMD_WRITE_MULTIPLE:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SataSetTransferParam28(pNewCmd);
        break;

    case ATA_CMD_READ_SECTOR_EXT:
    case ATA_CMD_READ_MULTIPLE_EXT:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        pNewCmd->ucCmdRW = HCMD_READ;
        HAL_SataSetTransferParam48(pNewCmd);
        break;

    case ATA_CMD_WRITE_SECTOR_EXT:
    case ATA_CMD_WRITE_MULTIPLE_EXT:
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SataSetTransferParam48(pNewCmd);
        break;
/*
    //VT3514, support SATA3.2
    case ATA_CMD_NCQ_NON_DATA:
        pNewCmd->ucCmdType = HCMD_TYPE_NONDATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_NCQ;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        HAL_SetParamNCQNonData(pNewCmd);
        break;        
    case ATA_CMD_SEND_FPDMA_QUEUED:
        pNewCmd->ucCmdType = HCMD_TYPE_NONDATA;
        pNewCmd->ucCmdRW = HCMD_WRITE;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_NCQ;
        HAL_SetParamSendFPDMA(pNewCmd);
        break;
    case ATA_CMD_RECEIVE_FPDMA_QUEUED:
        pNewCmd->ucCmdType = HCMD_TYPE_NONDATA;
        pNewCmd->ucCmdRW = HCMD_READ;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_NCQ;
        HAL_SetParamReceiveFPDMA(pNewCmd);
        break;
*/
    /* temp patch, validate identify device command */
    /*case ATA_CMD_IDENTIFY_DEVICE:   
        pNewCmd->ucCmdType = HCMD_TYPE_DATA;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        pNewCmd->ucCmdRW = HCMD_READ;
        pNewCmd->ulCmdLba = 0;
        pNewCmd->ulCmdSectorCnt = 1;
        pNewCmd->ulCmdRemSector = 1;
        break;  */

    default:
        pNewCmd->ucCmdType = HCMD_TYPE_NONDATA;
        pNewCmd->ucCmdRW = HCMD_READ;
        pNewCmd->ucCmdProtocol = HCMD_PROTOCOL_PIO;
        break;
    }

    HAL_SataFeedbacktoHW(pNewCmd);

    return ucCmdTag;
}

/*===================================================================
Function   :     ATA_TP_SendPIODataFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a PIO data D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendPIODataFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDPIODATA_FIS;
    return;
}

/*===================================================================
Function   :     ATA_TP_SendPIOSetupFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a PIO setup D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendPIOSetupFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDPIOSETUP_FIS;
    return;
}

/*===================================================================
Function   :     HAL_SataSendRegD2HFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a register D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendRegD2HFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDREGD2H_FIS;
    return;
}

/*===================================================================
Function   :     ATA_TP_SendSetDevBitFIS
Input      :     none
Output     :     none 
Description :  Invoke hardware to transfer a set device bits D2H FIS
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendSetDevBitFIS(void)
{
    rSDC_IOControl = BIT_SDC_IOCTRL_SENDSDB_FIS;
    return;
}

/*===================================================================
Function   :    ATA_TP_IsFISXferAvailable
Input      :     none
Output     :    The status of whether hardware is ready to send a FIS
Description :   Return a TRUE status when hardware is capable for sending out a FIS 
Modify History:
20090703         Yao Chen 001: first created
===================================================================*/
BOOL HAL_SataIsFISXferAvailable(void)
{
    if(0 == rSDC_IOControl)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*===================================================================
Function   :     ATA_TP_Signature_SendGoodStatus
Input      :     none
Output     :     none
Description :   Invokes hardware to send a signature FIS to report good status in COMRESET and
software reset protocol, or Execute Device Diagnostic protocol
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendGoodStatus(void)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 1;
    rSDC_LBALOW = 1;
    rSDC_LBAMID = 0;
    rSDC_LBAHIGH = 0;
    rSDC_SECCNT = 1;
    rSDC_DEVICE_HEAD = 0;
    rSDC_FISDirInt &= (~BIT_SDC_FIS_INTFLAG);
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    rSDC_FEATURE_ERROR = 0;
    UnLock_ShadowRegister();
    return;
}

void HAL_SataSignatureSendGoodStatus(void)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 1;
    rSDC_LBALOW = 1;
    rSDC_LBAMID = 0;
    rSDC_LBAHIGH = 0;
    rSDC_SECCNT = 1;
    rSDC_DEVICE_HEAD = 0;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    rSDC_FEATURE_ERROR = 0;
    UnLock_ShadowRegister();
    return;
}

/*===================================================================
Function   :     ATA_TP_Signature_SendBadStatus
Input      :     none
Output     :     none
Description :   Invokes hardware to send a signature FIS to report bad status in COMRESET and
software reset protocol, or Execute Device Diagnostic protocol
Note: 
Modify History:
20090528    Yao Chen    001: first created
===================================================================*/
void HAL_SataSendBadStatus(void)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_LBALOW = 1;
    rSDC_LBAMID = 0;
    rSDC_LBAHIGH = 0;
    rSDC_SECCNT = 1;
    rSDC_DEVICE_HEAD = 0;
    rSDC_FISDirInt &= (~BIT_SDC_FIS_INTFLAG);
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    UnLock_ShadowRegister();
    return;
}

/****************************************************************************
Name        :HAL_SataSendAbortStatus
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.04.05    15:54:54
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_SataSendAbortStatus(void)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x51;
    rSDC_FEATURE_ERROR = 4;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    rSDC_FEATURE_ERROR = 0;
    rSDC_COMMAND_STATUS = 0x50;
    UnLock_ShadowRegister();

    return;
}

/****************************************************************************
Name        :HAL_SataSendSuccessStatus
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.04.05    15:58:25
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_SataSendSuccessStatus(void)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    UnLock_ShadowRegister();

    return;
}

void HAL_SataSendErrorStatus(void)
{
    Lock_ShadowRegister();
    
    rSDC_FEATURE_ERROR   = 0x40;
    rSDC_COMMAND_STATUS  = 0x51;
    HAL_SataSendSetDevBitFIS();
    while (FALSE == HAL_SataIsFISXferAvailable());
    
    UnLock_ShadowRegister();
}

void L1_SataSendPowerMode(U8 mode)
{
    Lock_ShadowRegister();
    rSDC_COMMAND_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_SECCNT = mode;
    rSDC_FISDirInt |= BIT_SDC_FIS_INTFLAG;
    HAL_SataSendRegD2HFIS();
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    UnLock_ShadowRegister();
    return;
}

/****************************************************************************
Name:    HAL_SataConstructAndSendPIOSetupFIS
Input:    4 parameters
bFirstDRQ - The DRQ block is the first one within a host command;
bLastDRQ - The DRQ block is the last one within a host command;
ucDRQLen - The length of current DRQ block in sector;
ucProtocol - The current host command uses PIO data in or out protocol;
Output:    No output required.
Author:    Yao Chen
Date:    2012.09.18
Description: Build and send a PIO setup FIS for a DRQ block with information carried by 4 parameters.
Others:
Modify:
****************************************************************************/
void HAL_SataConstructAndSendPIOSetupFIS (
    const BOOL bFirstDRQ,
    const BOOL bLastDRQ,
    const U8 ucDRQLen,
    const U8 ucProtocol )
{
    /* 1. Enable firmware programming for shadow register block. */
    Lock_ShadowRegister();

    /* 2. Status when data is ready shall has BUSY = 0, DRDY = 1, DRQ = 1 and ERR = 0.
    Error shall never be reported by a PIO Setup FIS. */
    rSDC_COMMAND_STATUS = 0x58;
    rSDC_FEATURE_ERROR = 0;

    /* 3. Program transfer length field in the PIO Setup FIS. */
    rSDC_PIOXferCountLow =  (U8)(ucDRQLen << SEC_SZ_BITS);
    rSDC_PIOXferCountHigh = (U8)(ucDRQLen << (SEC_SZ_BITS - 8));

    /* 4. Decide the ending status and interrupt request/data direction according to IN/OUT protocol and first/last flag. */
    if ( HCMD_READ == ucProtocol ) {
        /* Interrupt bit and direction bit are always required for a read command. */
        rSDC_FISDirInt = BIT_SDC_FIS_DIRFLAG | BIT_SDC_FIS_INTFLAG;

        if ( TRUE == bLastDRQ )
            /* If last data block is ready, busy shall be cleared after data transfer according to PIO data in protocol. */
            rSDC_END_STATUS = 0x50;

        else
            /* Busy shall be set after data transfer if all data blocks are not finished according to PIO data in protocol. */
            rSDC_END_STATUS = 0xD0;
    }

    else {
        /* Busy shall always be set after one data block transfer in a write command
        because device needs to flush data into its media. */
        rSDC_END_STATUS = 0xD0;

        if ( TRUE == bFirstDRQ )
            /* There is no need for device to interrupt host when it gets ready for receiving the first data block. */
            rSDC_FISDirInt = 0;

        else
            /* In subsequent data transfer, getting ready for receiving a data block
            also means the previous data block has been written into media. So host needs to be notified. */
            rSDC_FISDirInt = BIT_SDC_FIS_INTFLAG;
    }

    /* 5. Trigger TP layer to send out the PIO Setup FIS. */
    HAL_SataSendPIOSetupFIS();

    /* 6. Check the transfer status and disable firmware programming for shadow register block. */
    while ( FALSE == HAL_SataIsFISXferAvailable() );
    UnLock_ShadowRegister();

    return;
}


/****************************************************************************
Name        :HAL_SataBuildReadDSG
Input       :SUBCMD* pSubCmd
Output      :read DSG id.
Author      :Haven Yang
Date        :2013.8.23
Description :build read DSG. full fill DSG Register bits & trigger it.
Others      :
Modify      :
****************************************************************************/
U8 HAL_SataBuildReadDSG(SUBCMD* pSubCmd)
{
    SATA_DSG tSataDsg;
    HCMD    *pHostCmd;
    U32      ulBuffMapValue;
    U16      usActualBuffLen;

    pHostCmd  = pSubCmd->pHCMD;

    HAL_MemSet((U32*)&tSataDsg, SATA_DSG_SIZE_DW, 0);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
    tSataDsg.AtaProtInfo.AutoActiveEn    = BIT_DISABLE;
    tSataDsg.AtaProtInfo.IsWriteCmd      = BIT_FALSE;
    tSataDsg.AtaProtInfo.EcpEn           = BIT_DISABLE;
    tSataDsg.AtaProtInfo.EcpKeySel       = 0;
    tSataDsg.AtaProtInfo.IsNonDataCmd    = BIT_FALSE;
    tSataDsg.AtaProtInfo.FwAckHostEn     = BIT_DISABLE;
    tSataDsg.AtaProtInfo.ProtSel         = PROTOCAL_SELECT(pHostCmd->ucCmdProtocol);//PROT_DMA_FPDMA;
    tSataDsg.AtaProtInfo.CmdTag          = pHostCmd->ucCmdTag;
    tSataDsg.AtaProtInfo.CmdXferSecCnt   = pHostCmd->ulCmdSectorCnt;
    tSataDsg.AtaProtInfo.XferEndIntEn    = SDC_NOT_INT_MCU;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    tSataDsg.XferCtrlInfo.Reserved       = 0;
    tSataDsg.XferCtrlInfo.DummyDataEn    = BIT_DISABLE;
    tSataDsg.CacheStsLocSel = CS_IN_SRAM;
#ifdef OTFB_VERSION
    tSataDsg.XferCtrlInfo.DataLocSel     = DATA_IN_SRAM;
#else
    tSataDsg.XferCtrlInfo.DataLocSel     = DATA_IN_DRAM;
#endif
    tSataDsg.XferCtrlInfo.BuffLen        = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;// for 128k bufsize: [0(256),255~1]
    tSataDsg.XferCtrlInfo.BuffMapId      = BUFMAP_ID(pSubCmd->SubDSGId);
    tSataDsg.XferCtrlInfo.BuffOffset     = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;// for 128k bufsize: [0     ,1~255]
    tSataDsg.XferCtrlInfo.CacheStsEn     = BIT_DISABLE;//((BIT_TRUE == pSubCmd->SubCmdLast) ? BIT_ENABLE : BIT_DISABLE);    // for cosim validate cs
    tSataDsg.XferCtrlInfo.BuffMapEn      = BIT_ENABLE;
    tSataDsg.XferCtrlInfo.Eot            = pSubCmd->SubCmdLast;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    tSataDsg.NextDsgId       = pSubCmd->SubNextDsgId;
    tSataDsg.CacheStsData    = 0;
    tSataDsg.CmdLbaHigh      = 0;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    tSataDsg.CacheStsAddr    = ((U32)g_pCacheStatus) + (pHostCmd->ucCmdTag * 4) - OTFB_START_ADDRESS; //pSubCmd->CacheStatusAddr; //???
    tSataDsg.DataAddr        = L1_GetMemAddrByLBA(pSubCmd->SubCmdAddInfo.ulSubCmdLBA + pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN);
    tSataDsg.CmdLbaLow       = pHostCmd->ulCmdLba;

    /*========================================================================*/
    /* copy DSG entry from local to DRAM/ASIC  & Trigger it                   */
    /*========================================================================*/
    HAL_MemCpy((U32 *)HAL_GetSataDsgAddr(pSubCmd->SubDSGId), (U32 *)&tSataDsg, SATA_DSG_SIZE_DW);

    usActualBuffLen = ((0==tSataDsg.XferCtrlInfo.BuffLen)? SEC_PER_BUF:tSataDsg.XferCtrlInfo.BuffLen);

    ulBuffMapValue = (0xffffffff << (tSataDsg.XferCtrlInfo.BuffOffset >> SEC_PER_LPN_BITS))
                   & (0xffffffff >> (31 - ((tSataDsg.XferCtrlInfo.BuffOffset + usActualBuffLen - 1) >> SEC_PER_LPN_BITS)));

    HAL_SetBufMapInitValue(tSataDsg.XferCtrlInfo.BuffMapId, ulBuffMapValue);
    
    if(BIT_TRUE == pSubCmd->SubCmdLast)
    {
        HAL_SetLastDataReady(pSubCmd->pHCMD->ucCmdTag);
 //       cosim_SetCacheStsAddr(pSubCmd->pHCMD->ucCmdTag, tSataDsg.CacheStsAddr);
 //       cosim_SetCacheStatus(tSataDsg.CacheStsAddr);
    }

    HAL_UsedSataDSG(pSubCmd->SubDSGId);
    
    if (BIT_TRUE == pSubCmd->SubCmdFirst)
    {
        HAL_SetFirstDSGID(pSubCmd->pHCMD->ucCmdTag, pSubCmd->SubDSGId);
        HAL_SetFirstReadDataReady(pSubCmd->pHCMD->ucCmdTag);
    }
    
    return pSubCmd->SubDSGId;
}

/*****************************************************************************
 Prototype      : HAL_SataBuildWriteDSG
 Description    : build write DSG. full fill DSG Register bits & trigger it.
 Input          : SUBCMD* SubCmd  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/8/28
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
U8 HAL_SataBuildWriteDSG(SUBCMD* pSubCmd)
{
    SATA_DSG    tSataDsg;
    HCMD       *pHostCmd;

    pHostCmd  = pSubCmd->pHCMD;

    HAL_MemSet((U32*)&tSataDsg, SATA_DSG_SIZE_DW, 0);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
#ifdef ONLY_ODD_CMD_TAG_DMA_AUTO_ACTIVE_EN
    tSataDsg.AtaProtInfo.AutoActiveEn    = (((HCMD_PROTOCOL_NCQ == pHostCmd->ucCmdProtocol) && (pHostCmd->ucCmdTag & 0x01)) ? BIT_ENABLE : BIT_DISABLE);
#else
    tSataDsg.AtaProtInfo.AutoActiveEn    = (HCMD_PROTOCOL_NCQ == pHostCmd->ucCmdProtocol) ? BIT_ENABLE : BIT_DISABLE;
#endif
    tSataDsg.AtaProtInfo.IsWriteCmd      = BIT_TRUE;
    tSataDsg.AtaProtInfo.EcpEn           = BIT_DISABLE;
    tSataDsg.AtaProtInfo.EcpKeySel       = 0;
    tSataDsg.AtaProtInfo.IsNonDataCmd    = BIT_FALSE;
    tSataDsg.AtaProtInfo.FwAckHostEn     = BIT_DISABLE;
    tSataDsg.AtaProtInfo.ProtSel         = PROTOCAL_SELECT(pHostCmd->ucCmdProtocol);
    tSataDsg.AtaProtInfo.CmdTag          = pHostCmd->ucCmdTag;
    tSataDsg.AtaProtInfo.CmdXferSecCnt   = pHostCmd->ulCmdSectorCnt;
    tSataDsg.AtaProtInfo.XferEndIntEn    = SDC_NOT_INT_MCU;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    tSataDsg.XferCtrlInfo.Reserved       = 0;
    tSataDsg.XferCtrlInfo.DummyDataEn    = BIT_DISABLE;
    tSataDsg.CacheStsLocSel = CS_IN_SRAM;
#ifdef OTFB_VERSION
    tSataDsg.XferCtrlInfo.DataLocSel     = DATA_IN_SRAM;
#else
    tSataDsg.XferCtrlInfo.DataLocSel     = DATA_IN_DRAM;
#endif
    tSataDsg.XferCtrlInfo.BuffLen        = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    tSataDsg.XferCtrlInfo.BuffMapId      = BUFMAP_ID(pSubCmd->SubDSGId);
    tSataDsg.XferCtrlInfo.BuffOffset     = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
    tSataDsg.XferCtrlInfo.CacheStsEn     = BIT_DISABLE;//((BIT_TRUE == pSubCmd->SubCmdLast) ? BIT_ENABLE : BIT_DISABLE); // for cosim validate cs //ramdisk must disable cs, and wholechip write enable.
    tSataDsg.XferCtrlInfo.BuffMapEn      = BIT_DISABLE;
    tSataDsg.XferCtrlInfo.Eot            = pSubCmd->SubCmdLast;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    tSataDsg.NextDsgId       = pSubCmd->SubNextDsgId;
    tSataDsg.CacheStsData    = 0;
    tSataDsg.CmdLbaHigh      = 0;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    tSataDsg.CacheStsAddr    = ((U32)g_pCacheStatus) + (pHostCmd->ucCmdTag * 4) - OTFB_START_ADDRESS; //pSubCmd->CacheStatusAddr - OTFB_START_ADDRESS; 
    tSataDsg.DataAddr        = L1_GetMemAddrByLBA(pSubCmd->SubCmdAddInfo.ulSubCmdLBA + pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN);
    tSataDsg.CmdLbaLow       = pHostCmd->ulCmdLba;

    /*========================================================================*/
    /* copy DSG entry from local to DRAM/ASIC  & Trigger it                   */
    /*========================================================================*/
    HAL_MemCpy((U32 *)HAL_GetSataDsgAddr(pSubCmd->SubDSGId), (U32 *)&tSataDsg, SATA_DSG_SIZE_DW);
    
    if(pSubCmd->SubCmdLast)
    {
        HAL_SetLastDataReady(pSubCmd->pHCMD->ucCmdTag);
//        cosim_SetCacheStsAddr(pSubCmd->pHCMD->ucCmdTag, tSataDsg.CacheStsAddr);
//        cosim_SetCacheStatus(tSataDsg.CacheStsAddr);
    }

    HAL_UsedSataDSG(pSubCmd->SubDSGId);

    if (BIT_TRUE == pSubCmd->SubCmdFirst)
    {
        HAL_SetFirstDSGID(pSubCmd->pHCMD->ucCmdTag, pSubCmd->SubDSGId);
        HAL_SetFirstReadDataReady(pSubCmd->pHCMD->ucCmdTag);
    }

    return pSubCmd->SubDSGId;
}

void HAL_SetSendSDBFISReady(U8 ucCmdTag)
{
#ifdef SIM
    SDC_SetSDBFISReady(ucCmdTag);
#else
    rSDC_SendSDBFISReady |= (1<<ucCmdTag);
#endif
}

void HAL_ClearSendSDBFISReady(U8 ucCmdTag)
{
#ifdef SIM
    SDC_ClearSDBFISReady(ucCmdTag);
#else
    rSDC_SendSDBFISReady    &= (~(1 << ucCmdTag));
#endif
}

static U32 cosim_cs_addr[NCQ_DEPTH] = {0};

void cosim_SetCacheStsAddr(U8 ucCmdTag, U32 ulCSAddress)
{
    cosim_cs_addr[ucCmdTag] = ulCSAddress;
}

void cosim_SetCacheStatus(U32 ulCSAddress)
{
    volatile U8* pCSAddress;// = OTFB_START_ADDRESS + ulCSAddress;
    pCSAddress = (volatile U8*)(OTFB_START_ADDRESS + ulCSAddress);
    *pCSAddress = 1;
}

U8 cosim_CheckCS_IDLE(U8 ucCmdTag)
{
    U8 cs;
    
    cs = *(volatile U8*)(OTFB_START_ADDRESS + cosim_cs_addr[ucCmdTag]);

    if ((0 == cs)||(2 == cs))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void HAL_FWCtrlSDBFISReady(void)
{
    U8 ucCmdTag;
    
    for (ucCmdTag = 0; ucCmdTag < NCQ_DEPTH; ucCmdTag++)
    {
        if (HCMD_STATE_SATA_DONE == HostCmdSlot[ucCmdTag].ucCmdStatus)
        {
            //check cache status, and send SDBFISReady
            if (TRUE == cosim_CheckCS_IDLE(ucCmdTag))
            {
                HAL_SetSendSDBFISReady(ucCmdTag);
                HAL_HCmdNone(ucCmdTag);
            }
        }
    }
}


/********************** FILE END ***************/
