#include "HAL_SataDSG.h"
#if defined(SIM)
#include "../sgemodel/sim_HSG.h"
#endif

#ifdef SATA_DSG_MODE
LOCAL volatile pHSG_REPORT_MCU l_pSataDsgReport1;
LOCAL volatile pHSG_REPORT_MCU l_pSataDsgReport2;
/*----------------------------------------------------------------------------
Name: HAL_SataDsgInit
Description: 
    initialize sata DSG base address;
    set sata DSG to default status.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    in bootup stage, call this function to initialize sata DSG;
    any other sata DSG functions can be called after this function finished.
------------------------------------------------------------------------------*/
void HAL_SataDsgInit(void)
{
    //SIM_XTENSA
    l_pSataDsgReport1 = (pHSG_REPORT_MCU)&rHsgReportMcu1;//HSG_REPORT_MCU0R;
    l_pSataDsgReport2 = (pHSG_REPORT_MCU)&rHsgReportMcu2;//HSG_REPORT_MCU1R;  

    /* Set Sata mode enable */
    rSGESataMode |= 1;
}

/*------------------------------------------------------------------------------
Name: HAL_GetSataDsg
Description: 
    get current valid Sata DSG and trigger next one.
Input Param:
    U8 Type£º 0 = sata type0; 1 = sata type1;
            type0:sata DSG ID = 64~127;
            type1:sata DSG ID = 0~63; 
Output Param:
    U16 *PDsgId: pointer to obtained sata DSG id;
Return Value:
    BOOL:
        0 = no sata DSG got;
        1 = sata DSG got success;
Usage:
    when build sata DSG chain, call this function to get one sata DSG.
------------------------------------------------------------------------------*/
BOOL HAL_GetSataDsg(U16 *PDsgId, U8 Type)
{
    BOOL ucStsFlag;
    ucStsFlag = HAL_GetCurSataDsg(PDsgId, Type);

    HAL_TriggerSataDsg(Type);
    return ucStsFlag; 
}

/*------------------------------------------------------------------------------
Name: HAL_IsSataDsgValid
Description: 
    check current sata DSG valid or not.
Input Param:
    U8 Type£º 0 = sata type0; 1 = sata type1;
            type0:sata DSG ID =  64~127; 
            type1:sata DSG ID = 0~63;
Output Param:
    none
Return Value:
    BOOL:
        0 = current sata DSG invalid;
        1 = current sata DSG valid;
Usage:
------------------------------------------------------------------------------*/
BOOL HAL_IsSataDsgValid(U8 Type)
{
    if(DSG_TYPE_WRITE == Type)
    {
        return l_pSataDsgReport1->HsgValidEn;            
    }
    else if(DSG_TYPE_READ ==Type)
    {
        return l_pSataDsgReport2->HsgValidEn;   
    }
    else
    {
        DBG_Getch();
    }
}

/*------------------------------------------------------------------------------
Name: HAL_GetCurSataDsg
Description: 
    get current valid Sata DSG but not trigger next one.
Input Param:
    U8 Type£º 0 = type0; 1 = type1;
            type0:sata DSG ID =  64~127; 
            type1:sata DSG ID = 0~63;
Output Param:
    U16 *PDsgId: pointer to obtained sata DSG id;
Return Value:
    BOOL:
        0 = no sata DSG got;
        1 = sata DSG got success;
Usage:
    when build sata DSG chain, call this function to get one sata DSG.
    caution:after call funtion HAL_TriggerSataDsg,current sata DSG owns to you really.
------------------------------------------------------------------------------*/
BOOL HAL_GetCurSataDsg(U16 *PDsgId, U8 Type)
{
    BOOL ucStsFlag;

    if(DSG_TYPE_WRITE == Type)
    {
        if(TRUE == l_pSataDsgReport1->HsgValidEn)
        {
            *PDsgId = l_pSataDsgReport1->HsgId;
            ucStsFlag = 1;
        }
        else
        {
            *PDsgId = INVALID_4F;
            ucStsFlag = 0;        
        }
        
    }
    else if(DSG_TYPE_READ == Type)
    {
        if(TRUE == l_pSataDsgReport2->HsgValidEn)
        {
            *PDsgId = l_pSataDsgReport2->HsgId;        
            ucStsFlag = 1;            
        }
        else
        {
            *PDsgId = INVALID_4F;
            ucStsFlag = 0;    
        }        
    }
    else
    {
        DBG_Getch();
    }
    return ucStsFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_TriggerSataDsg
Description: 
    trigger next sata DSG.
Input Param:
    U8 Type£º 0 = sata type0; 1 = sata type1;
            type0:sata DSG ID = 64~127;
            type1:sata DSG ID = 0~63; 
Output Param:
    none
Return Value:
   none
Usage:
    when build sata DSG chain, call this function to triger next sata DSG.
    caution:HAL_GetSataDsg = HAL_GetCurSataDsg + HAL_TriggerSataDsg
------------------------------------------------------------------------------*/
void HAL_TriggerSataDsg( U8 Type)
{
#if defined (SIM)
    if(DSG_TYPE_WRITE == Type)
    {
        DSG_AllocateSataType0Dsg();
    }
    else if(DSG_TYPE_READ == Type)
    {
        DSG_AllocateSataType1Dsg();
    }
    else
    {
        DBG_Getch();
    }    
#else
    //SIM_XTENSA
    if(DSG_TYPE_WRITE == Type)
    {
        l_pSataDsgReport1->HsgTrigger = TRUE;
    }
    else
    {
        l_pSataDsgReport2->HsgTrigger = TRUE;
    }

#endif    
}
/*------------------------------------------------------------------------------
Name: HAL_GetSataDsgAddr
Description: 
    get addrress of sata DSG according to DSG ID.
Input Param:
    U16 DsgId: sata DSG id
Output Param:
    none
Return Value:
    U32: address for sata DSG
Usage:
    if get one sata DSG by HAL_GetNormalDsg function, call this function to get its address.
------------------------------------------------------------------------------*/
U32 HAL_GetSataDsgAddr(U16 DsgId)
{
    //SIM_XTENSA
    if(DsgId  >= SATA_TOTAL_DSG_NUM)
    {
        DBG_Getch();
    }
    else
    {
        //return (U32)(DsgId*sizeof(SATA_DSG)+SATA_DSG_BASE+OTFB_START_ADDRESS);
        return (U32)(DsgId*sizeof(SATA_DSG)+SATA_DSG_BASE);
    }  
}

/*------------------------------------------------------------------------------
Name: HAL_SetSataDsgSts
Description: 
    set sata DSG sts(valid or not) according to StsValue
Input Param:
    U16 DsgId: sata DSG id
    U8 StsValue: 1 = valid; 0 = invalid; others not allowed;
Output Param:
    none
Return Value:
    none
Usage:
    set one sata DSG valid after allocated and filled it with contents.
    set one sata DSG invalid if it's not longer needed.
------------------------------------------------------------------------------*/
void HAL_SetSataDsgSts(U16 DsgId, U8 StsValue)
{
#if defined(SIM)
    if(DsgId >= SATA_TOTAL_DSG_NUM)
    {
        printf("something wrong!!!\n");
        DBG_Getch();
    }    

    if(0 == StsValue)
    {
        DSG_SetSataDsgInvalid(DsgId);
    }
    else if(1 == StsValue)
    {
        DSG_SetSataDsgValid(DsgId);
    }
    else
    {
        DBG_Getch();
    }

#else
    if(DsgId >= SATA_TOTAL_DSG_NUM)
    {
        DBG_Getch();
    }
    if(DsgId < SATA_TYPE1_DSG_NUM)
    {      
        l_pSataDsgReport2->HsgWrIndex = DsgId;
        l_pSataDsgReport2->HsgValue = StsValue;
        l_pSataDsgReport2->HsgWrEn = TRUE;
    }
    else
    {
        l_pSataDsgReport1->HsgWrIndex = DsgId;
        l_pSataDsgReport1->HsgValue = StsValue;
        l_pSataDsgReport1->HsgWrEn = TRUE;
    }

#endif    
}


/*==============================================================================
Func Name  : HAL_SetSataDsgValid
Input      : U16 usDsgId  
Output     : NONE
Return Val : NONE
Discription: Set one Sata Dsg Valid. to confirm hardware this DSG is used.
Usage      : 
History    : 
    1. 2013.11.6 Haven Yang create function
==============================================================================*/
void HAL_SetSataDsgValid(U16 usDsgId)
{

#ifdef SIM
    DSG_SetSataDsgValid(usDsgId);
#else
    if(usDsgId >= SATA_TOTAL_DSG_NUM)
    {
        DBG_Getch();
    }
    if(usDsgId < SATA_TYPE1_DSG_NUM)
    {      
        l_pSataDsgReport2->HsgWrIndex = usDsgId;
        l_pSataDsgReport2->HsgValue = 1;
        l_pSataDsgReport2->HsgWrEn = TRUE;
    }
    else
    {
        l_pSataDsgReport1->HsgWrIndex = usDsgId;
        l_pSataDsgReport1->HsgValue = 1;
        l_pSataDsgReport1->HsgWrEn = TRUE;
    }
#endif    
}
#endif

