/******************************************************************************
*                   Copyright (C), 2012, VIA Tech. Co., Ltd.                  *
*        Information in this file is the intellectual property of VIA         *
*    Technologies, Inc., It may contains trade secrets and must be stored     *
*                          and viewed confidentially.                         *
 ******************************************************************************
  File Name     : L1_DSG.c
  Version       : Initial Draft
  Author        : Haven Yang
  Created       : 2013/8/26
  Description   : stat DSG operation
  Function List :
  History       :
  1.Date        : 2013/8/26
    Author      : Haven Yang
    Modification: Created file

******************************************************************************/


#include "HAL_SataDSG.h"
#include "HAL_SataIO.h"
#include "L1_DSG.h"

/******************************************************************************
l_aReadDSGQ: used for buffer of ReadDSG IDs. the Q number is READ_DSG_NUM + 1,
              so the Q can storage all READ DSG, and the extra one is 
              INVALID_2F, which confirm the Q is all used, need refreshing.
Called by :
            L1_RefreshReadDSGQ : update free DSG to the Q from Register.
            L1_GetReadDSGID: Get Current or Next DSGID when firmware need it.

l_aWriteDSGQ: same of the l_aReadDSGQ
Q_Value:
            l_aReaDDSGQ : 1 ~ READ_DSG_NUM
            l_aWriteDSGQ: READ_DSG_NUM+1 ~ 128
******************************************************************************/

#define BUFFER_OFFSET_FROM_DRAM (128<<20)    //128M

U16 g_CurrUsedReadDSGID ;//= INVALID_4F;
U16 g_CurrUsedWriteDSGID;// = INVALID_4F;


U32 HAL_SetFirstDSGID(U8 CmdTag, U8 DSGID)
{
    rFIRST_DSG_ID[CmdTag] = DSGID;
    return 0;
}

void HAL_UsedSataDSG(U8 ucDSGID)
{
    HAL_SetSataDsgValid(ucDSGID);

#ifdef SIM
    if(ucDSGID >= SATA_TOTAL_DSG_NUM)
    {
        printf("something wrong!!!\n");
        DBG_Getch();
    }

    if(ucDSGID < SATA_TYPE1_DSG_NUM)
    {
        if (ucDSGID != g_CurrUsedReadDSGID)
        {
            printf("something wrong!!!usDsgId != g_CurrUsedReadDSGID\n");
            DBG_Getch();
        }
    }
    else
    {
        if (ucDSGID != g_CurrUsedWriteDSGID)
        {
            printf("something wrong!!!usDsgId != g_CurrUsedWriteDSGID\n");
            DBG_Getch();
        }
    }

#endif

    if(ucDSGID < SATA_TYPE1_DSG_NUM)
    {
        g_CurrUsedReadDSGID = INVALID_4F;
    }
    else
    {
        g_CurrUsedWriteDSGID = INVALID_4F;
    }
}


/*****************************************************************************
 Prototype      : HAL_GetMemAddr
 Description    : Get memory address (in DRAM) by Buffer ID 
 Input          : U32 BufferID  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/8/26
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
U32 HAL_GetMemAddr(U32 BufferID)
{
    return ((BufferID << BUF_SIZE_BITS) + DRAM_START_ADDRESS);
}

/*****************************************************************************
 Prototype      : HAL_GetBufferID
 Description    : Get Buffer ID by real buffer data memory address
 Input          : U32 MemAddr  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/8/26
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
U32 HAL_GetBufferID(U32 MemAddr)
{
    return ((MemAddr - DRAM_START_ADDRESS) >> BUF_SIZE_BITS);
}



U32 L1_GetMemAddrByLBA(U32 LBA)
{
#ifdef OTFB_VERSION   
    return ((OTFB_RAMDISK_BASE - OTFB_START_ADDRESS)+ (LBA << 9));
#else
    return (BUFFER_OFFSET_FROM_DRAM + (LBA << 9));
#endif
}


BOOL L1_CheckReadDSGResource(SUBCMD* pSubCmd)
{
    U16  usCurrDSG;
    U16  usNextDSG;

    /*========================================================================*/
    /* 1. Get Current DSG ID                                                  */
    /*========================================================================*/
    if (INVALID_4F == g_CurrUsedReadDSGID)
    {
        HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_READ);
        HAL_TriggerSataDsg(DSG_TYPE_READ);
        
#ifdef SIM
        if (INVALID_4F != usCurrDSG)
        {
            if (usCurrDSG >= 64)
            {
                printf("alloc read dsg wrong 1\n");
                DBG_Getch();
            }       
         }
#endif
    }
    else
    {
        usCurrDSG = g_CurrUsedReadDSGID;
    }

    if (INVALID_4F == usCurrDSG)
    {
        return FALSE;
    }

    g_CurrUsedReadDSGID = usCurrDSG;

    /*========================================================================*/
    /* 2. Check Next DSG ID                                                   */
    /*========================================================================*/
    if (TRUE != pSubCmd->SubCmdLast)
    {
        HAL_GetCurSataDsg(&usNextDSG, DSG_TYPE_READ);
        
        if (INVALID_4F == usNextDSG)
        {
            HAL_TriggerSataDsg(DSG_TYPE_READ);
            return FALSE;
        }
    }
    else    /* Last DSG */
    {
        usNextDSG = INVALID_4F;
    }

    /*========================================================================*/
    /* 3. set subcmd and return                                               */
    /*========================================================================*/
#ifdef SIM
    if (usCurrDSG >= 64)
    {
        printf("alloc read dsg wrong 3\n");
        DBG_Getch();
    }       
#endif
    
    pSubCmd->SubDSGId     = (U8)usCurrDSG;
    pSubCmd->SubNextDsgId = (U8)usNextDSG;

    return TRUE;
    
}



BOOL L1_CheckWriteDSGResource(SUBCMD* pSubCmd)
{
    U16  usCurrDSG;
    U16  usNextDSG;

    /*========================================================================*/
    /* 1. Get Current DSG ID                                                  */
    /*========================================================================*/
    if (INVALID_4F == g_CurrUsedWriteDSGID)
    {
        HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_WRITE);
        HAL_TriggerSataDsg(DSG_TYPE_WRITE);
        
#ifdef SIM
        if (INVALID_4F != usCurrDSG)
        {
            if ((usCurrDSG >= 128) || (usCurrDSG < 64))
            {
                printf("alloc read dsg wrong 1\n");
                DBG_Getch();
            }       
         }
#endif
    }
    else
    {
        usCurrDSG = g_CurrUsedWriteDSGID;
    }

    if (INVALID_4F == usCurrDSG)
    {
        return FALSE;
    }

    g_CurrUsedWriteDSGID = usCurrDSG;

    /*========================================================================*/
    /* 2. Check Next DSG ID                                                   */
    /*========================================================================*/
    if (TRUE != pSubCmd->SubCmdLast)
    {
        HAL_GetCurSataDsg(&usNextDSG, DSG_TYPE_WRITE);
        
        if (INVALID_4F == usNextDSG)
        {
            HAL_TriggerSataDsg(DSG_TYPE_WRITE);
            return FALSE;
        }
    }
    else    /* Last DSG */
    {
        usNextDSG = INVALID_4F;
    }

    /*========================================================================*/
    /* 3. set subcmd and return                                               */
    /*========================================================================*/
#ifdef SIM
    if ((usCurrDSG >= 128) || (usCurrDSG < 64))
    {
        printf("alloc read dsg wrong 3\n");
        DBG_Getch();
    }       
#endif
    
    pSubCmd->SubDSGId     = (U8)usCurrDSG;
    pSubCmd->SubNextDsgId = (U8)usNextDSG;

    return TRUE;
    
}


/*****************************************************************************
 Prototype      : L1_CheckDSGResource
 Description    : Check is there free DSGs to use? and fill the DSG id to
                  the subcmd structure
 Input          : SUBCMD* pSubCmd  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/9/10
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
BOOL L1_CheckDSGResource(SUBCMD* pSubCmd)
{
    if (HCMD_WRITE == pSubCmd->pHCMD->ucCmdRW)
    {
        return L1_CheckWriteDSGResource(pSubCmd);
    }
    else
    {
        return L1_CheckReadDSGResource(pSubCmd);
    }    
}

/*==============================================================================
Func Name  : L1_GetSpecialReadDSG
Input      : void  
Output     : NONE
Return Val : 
Discription: when a NON-DATA command need to Xfer data, allocate DSG via this interface.
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
==============================================================================*/
U8  L1_GetSpecialReadDSG(void)
{
    U16  usCurrDSG;

    if (INVALID_4F == g_CurrUsedReadDSGID)
    {
        HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_READ);
        HAL_TriggerSataDsg(DSG_TYPE_READ);

        if (INVALID_4F == usCurrDSG)
        {
            HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_READ);
            HAL_TriggerSataDsg(DSG_TYPE_READ);
        }

        g_CurrUsedReadDSGID = usCurrDSG;        
    }

    return g_CurrUsedReadDSGID;
}

/*==============================================================================
Func Name  : L1_GetSpecialWriteDSG
Input      : void  
Output     : NONE
Return Val : 
Discription: when a NON-DATA command need to Xfer data, allocate DSG via this interface.
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
==============================================================================*/
U8  L1_GetSpecialWriteDSG(void)
{
    U16  usCurrDSG;

    if (INVALID_4F == g_CurrUsedWriteDSGID)
    {
        HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_WRITE);
        HAL_TriggerSataDsg(DSG_TYPE_WRITE);

        if (INVALID_4F == usCurrDSG)
        {
            HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_WRITE);
            HAL_TriggerSataDsg(DSG_TYPE_WRITE);
        }

        g_CurrUsedWriteDSGID = usCurrDSG;        
    }

    return g_CurrUsedWriteDSGID;
}


