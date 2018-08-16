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
Filename     :  HAL_DramConfig.c                                     
Version      :  Ver 1.0                                               
Date         :  20130602                                   
Author       :  Victor Zhang

Description: 
    Completing DRAM controller initialization for COSIM. 

Modification History:
20130602     Victor Zhang   Creating file referred to Susan's DRAM controller init 
20140902     Victor Zhang   Modification as new coding style
20151119     Regina Wang    Modification 
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION 
------------------------------------------------------------------------------*/

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "Proj_Config.h"
#include "HAL_DramConfig.h"

#ifdef COSIM
/*------------------------------------------------------------------------------
    LOCAL PARAMETER DECLARATION 
------------------------------------------------------------------------------*/
LOCAL REGDRAMC_00 rREGDRAMC_00 = {0};
LOCAL REGDRAMC_04 rREGDRAMC_04 = {0};
LOCAL REGDRAMC_08 rREGDRAMC_08 = {0};
LOCAL REGDRAMC_0C rREGDRAMC_0c = {0};
LOCAL REGDRAMC_10 rREGDRAMC_10 = {0};
LOCAL REGDRAMC_14 rREGDRAMC_14 = {0};
LOCAL REGDRAMC_18 rREGDRAMC_18 = {0};
LOCAL REGDRAMC_1C rREGDRAMC_1c = {0};
LOCAL REGDRAMC_20 rREGDRAMC_20 = {0};
LOCAL REGDRAMC_24 rREGDRAMC_24 = {0};
LOCAL REGDRAMC_28 rREGDRAMC_28 = {0};
LOCAL REGDRAMC_2C rREGDRAMC_2c = {0};
LOCAL REGDRAMC_30 rREGDRAMC_30 = {0};
LOCAL REGDRAMC_34 rREGDRAMC_34 = {0};
LOCAL REGDRAMC_38 rREGDRAMC_38 = {0};
LOCAL REGDRAMC_3C rREGDRAMC_3c = {0};
LOCAL REGDRAMC_40 rREGDRAMC_40 = {0};
LOCAL REGDRAMC_44 rREGDRAMC_44 = {0};
LOCAL REGDRAMC_48 rREGDRAMC_48 = {0};
LOCAL REGDRAMC_4C rREGDRAMC_4c = {0};
LOCAL REGDRAMC_50 rREGDRAMC_50 = {0};
LOCAL REGDRAMC_54 rREGDRAMC_54 = {0};
LOCAL REGDRAMC_58 rREGDRAMC_58 = {0};
LOCAL REGDRAMC_5C rREGDRAMC_5c = {0};
LOCAL REGDRAMC_60 rREGDRAMC_60 = {0};
LOCAL REGDRAMC_64 rREGDRAMC_64 = {0};
LOCAL REGDRAMC_68 rREGDRAMC_68 = {0};
LOCAL REGDRAMC_6C rREGDRAMC_6c = {0};
LOCAL REGDRAMC_70 rREGDRAMC_70 = {0};
LOCAL REGDRAMC_74 rREGDRAMC_74 = {0};
LOCAL REGDRAMC_78 rREGDRAMC_78 = {0};
LOCAL REGDRAMC_7C rREGDRAMC_7c = {0};
LOCAL U32 rREGPMU_18 = 0;
LOCAL U32 rREGPMU_1c = 0;
LOCAL MR1_ADDR_LPDDR2 rMR1_ADDR_LPDDR2 = {0};
LOCAL MR2_ADDR_LPDDR2 rMR2_ADDR_LPDDR2 = {0};
LOCAL REG_NORM rREG_NORM = {0};
LOCAL RDMR_REG rRDMR_REG = {0};
LOCAL DDR3P_MR0 rDDR3P_MR0 = {0};
LOCAL DDR3P_MR1 rDDR3P_MR1 = {0};
LOCAL DDR3P_MR2 rDDR3P_MR2 = {0};

/*------------------------------------------------------------------------------
    EXTERNAL FUNCTION DECLARATION 
------------------------------------------------------------------------------*/
extern U32 HAL_GetMCUCycleCount(void);

/*------------------------------------------------------------------------------
    MACRO DECLARATION 
------------------------------------------------------------------------------*/
#define COUNT_INoneUS   300 /* CPU CLK 300MHz */

/*------------------------------------------------------------------------------
Name: HalDramInitLPDDR2
Description: 
    DRAM LPDDR2 initialization for cosim
Note:
    
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    
History:
    20130905    Haven Yang      create 
    20140905    Victor Zhang    Replace  HAL_GetMCUCycleCount to XT_RSR_CCOUNT
------------------------------------------------------------------------------*/

void Delay_us(U32 ulUsCount)
{
    U32 ulLastTimeTag;
    U32 ulCurrTimeTag;
    U32 ulTargetTime;
    U8  ucTargetOvevflow = FALSE;
    U8  ucCurrOverflow   = FALSE;

    ulLastTimeTag = HAL_GetMCUCycleCount();
    ulTargetTime = ulLastTimeTag + ulUsCount * COUNT_INoneUS;

    if (ulTargetTime < ulLastTimeTag)
    {
        ucTargetOvevflow = TRUE;
    }

    for(;;)
    {
        ulCurrTimeTag = HAL_GetMCUCycleCount();

        if (ulCurrTimeTag < ulLastTimeTag)
        {
            ucCurrOverflow = TRUE;
        }
        
        /* target unoverload */
        if (FALSE == ucTargetOvevflow)
        {
            if ((TRUE == ucCurrOverflow) || (ulCurrTimeTag >= ulTargetTime))
            {
                break;
            }
        }
        /* target overload */
        else
        {
            if ((TRUE == ucCurrOverflow) && (ulCurrTimeTag >= ulTargetTime))
            {
                break;
            }
        }
    }
}
/*
FUNC    :   HAL_DramCeEn
INPUT   :   ucCE -- DRAM CE ID
OUTPUT  :   NONE
DES     : 
    Support multiple dram ce 

History :
    2014/11/27 Victor Zhang Create
*/



/*------------------------------------------------------------------------------
Name: HalDramInitLPDDR2
Description: 
    DRAM LPDDR2 initialization for cosim
Note:
    
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    
History:
    20140902    Victor Zhang   created
------------------------------------------------------------------------------*/

void HalDramInitLPDDR2(void)
{
    Delay_us(2);    
    rREGDRAMC_20.RDYNCKE = 0;      
    rREGDRAMC_10.RMCLK0EN = 1;     
    rREGDRAMC_60.DS_ENA0 = 0;      
    rREGDRAMC_60.DS_ENA1 = 0;      
    rREGDRAMC_24.RPGEN = 0;        
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;   
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;   
    r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;   
    r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;   

    rREGDRAMC_14.RBA2EN = 1;       
    rREGDRAMC_14.RMA0 = 7;         
    rREGDRAMC_14.RBA2SEL = 2;        
    rREGDRAMC_14.RBA1SEL = 2;        
    rREGDRAMC_14.RBA0SEL = 3; 
    
    r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;   

    rMR1_ADDR_LPDDR2.MR_burst_type = 0;    
    rMR1_ADDR_LPDDR2.MR_wrap = 0;          
                                           
#ifdef MDBL8
    rMR1_ADDR_LPDDR2.MR_burst_length = 3;  
#else
    rMR1_ADDR_LPDDR2.MR_burst_length = 2;  
#endif                                     
                                     
#ifdef DM533                               
    rMR2_ADDR_LPDDR2.MR_RL_WL = 6;         
    rMR1_ADDR_LPDDR2.MR_write_recovery = 4;
#elif DM400                                
    rMR2_ADDR_LPDDR2.MR_RL_WL = 4;         
    rMR1_ADDR_LPDDR2.MR_write_recovery = 4;
#elif DM333                                
    rMR2_ADDR_LPDDR2.MR_RL_WL = 3;         
    rMR1_ADDR_LPDDR2.MR_write_recovery = 4;
#elif DM200                                
    rMR2_ADDR_LPDDR2.MR_RL_WL = 1;         
    rMR1_ADDR_LPDDR2.MR_write_recovery = 4;
#endif                                     
                                        
    Delay_us(2);
                                           
    rREG_NORM.RCL_NORM = rREGDRAMC_1c.RCL; 
    rREGDRAMC_1c.RCL = 1;                  
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;       
                                      
    /// step 5  MRESET CMD    

        rREGDRAMC_10.RSDM = 3;                   
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;       
        rREGDRAMC_04.MR_MA = 0x3f00;             
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        rREGDRAMC_04.RDADSB = 1;                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;                 
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
                               
        Delay_us(5);                     

        // DAI MRR CMD                                      
        rREGDRAMC_10.RSDM = 5;                   
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;       
        rREGDRAMC_04.MR_MA &= ~(0xff00);           
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        rREGDRAMC_04.RDADSB = 1;          
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;            
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
                                           
        Delay_us(1);                       
                                          
        rREGDRAMC_6c.MRR_RDY = 0;            
        r_REGDRAMC_6c = *(volatile U32 *)&rREGDRAMC_6c;       
                
        Delay_us(2);        

        // ZQ Init MRW CMD                                   
        rREGDRAMC_10.RSDM = 3;             
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;      
        rREGDRAMC_04.MR_MA = (0xaff);       
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;      
        rREGDRAMC_04.RDADSB = 1;           
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;      
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;            
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;     
                                                                                   
        Delay_us(5);         
                                         
        rREGDRAMC_10.RSDM = 3;             
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;

        // MRW CMD for MR1
        rREGDRAMC_04.MR_MA = (*(volatile U32 *)&rMR1_ADDR_LPDDR2 & (0x00ffff00)) >> 8;     
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;     
        rREGDRAMC_04.RDADSB = 1;            
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;     
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;           
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       

        // MRW CMD for MR2                                                                    
        rREGDRAMC_04.MR_MA = (*(volatile U32 *)&rMR2_ADDR_LPDDR2 & (0x00ffff00)) >> 8;  
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        rREGDRAMC_04.RDADSB = 1;             
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;             
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;       
        Delay_us(1);   
    
    // step 6
    rREGDRAMC_10.RSDM = 0;             
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;       
                                        
    rREGDRAMC_24.RPGEN = 1;             
    r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;       
                           
#ifdef MC6_CKE                        
    rREGDRAMC_20.RDYNCKE = 1;                            
    rREGDRAMC_60.DS_ENA0 = 0;                                 
    rREGDRAMC_60.DS_ENA1 = 0;                                 
    rREGDRAMC_70.RDCLKOAPM = 1;         
#endif                                   
#ifdef DYNCLK                            
    rREGDRAMC_10.RDRAMCPM = 1;          
    Delay_us(2);                        
#else                                    
    rREGDRAMC_10.RDRAMCPM = 0;
    Delay_us(2); 
#endif            
              
    rREGDRAMC_1c.RCL = rREG_NORM.RCL_NORM; 
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;        
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;        
    r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;        
    r_REGDRAMC_70 = *(volatile U32 *)&rREGDRAMC_70;        
                                     
}

/*------------------------------------------------------------------------------
Name: HalDramInitDDR3
Description: 
    DRAM DDR3 initialization for cosim.
Note:
    
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    
History:
    20140902    Victor Zhang   created
------------------------------------------------------------------------------*/

void HalDramInitDDR3(void)
{     
    Delay_us(1);                       
    rREGDRAMC_20.RDYNCKE = 0;          
    rREGDRAMC_10.RMCLK0EN = 1;         
    rREGDRAMC_60.DS_ENA0 = 0;          
    rREGDRAMC_60.DS_ENA1 = 0;     
    rREGDRAMC_24.RPGEN = 0;                                 
                                                            
    rREG_NORM.REFC_NORM =  rREGDRAMC_28.REFC;               
    rREGDRAMC_28.REFC = 0;                                  
    rREG_NORM.RDM32B_NORM = rREGDRAMC_10.RDM32B;            
    rREGDRAMC_10.RDM32B = 0;                                
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;         
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;         
    r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;         
    r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;         
    r_REGDRAMC_28 = *(volatile U32 *)&rREGDRAMC_28;         
    rREGDRAMC_14.RBA2EN = 1;                                

    rREGDRAMC_14.RMA0 = 7;
                              
    rREGDRAMC_14.RBA2SEL = 3;                               
    rREGDRAMC_14.RBA1SEL = 3;                               
    rREGDRAMC_14.RBA0SEL = 3;  
    r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;                                                                   
    Delay_us(1);                                                                                     
    rREGDRAMC_10.RMEM_RESET = 0;                            
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;         
    Delay_us(1);                                            
    rREGDRAMC_10.RMEM_RESET = 1;                            
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;         
                                                            
    Delay_us(1);                               
    //HAL_DelayCycle(7);  //Regina for COSIM
                                                         

        // set mode register 2
        rREGDRAMC_10.RSDM = 3;                                  
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;     
        *(U16 *)&rDDR3P_MR2 = 0x208;
        rREGDRAMC_04.MR_MA = *(U16*)&rDDR3P_MR2;                
        rREGDRAMC_04.MR_BA = 2;                                 
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        rREGDRAMC_04.RDADSB = 1;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;                                                                                                                              
        Delay_us(1);                                                                                                        

        // set mode register 3                                                        
        rREGDRAMC_04.MR_MA = 0;                          
        rREGDRAMC_04.MR_BA = 3;                                 
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        rREGDRAMC_04.RDADSB = 1;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;                                                                                                                              
        Delay_us(1);                                            

        // set mode register 1 Enable DLL                                                                                                                   
        rREGDRAMC_04.MR_MA = *(U16*)&rDDR3P_MR1;                
        rREGDRAMC_04.MR_BA = 1;                                 
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        rREGDRAMC_04.RDADSB = 1;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;                                                                     
                                                                
        Delay_us(1);                                            
        // set mode register 0 Enable DLL                                                        
        *(U16 *)&rDDR3P_MR0 = 0x1918;        
        rREGDRAMC_04.MR_MA = *(U16*)&rDDR3P_MR0;                
        rREGDRAMC_04.MR_BA = 0;                                 
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        rREGDRAMC_04.RDADSB = 1;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        Delay_us(3);
        rREGDRAMC_04.RDADSB = 0;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;                                                                                                                               
        Delay_us(1);                                            
        // ZQCL CMD                                                                                                                  
        rREGDRAMC_10.RSDM = 6;                                  
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;         
        rREGDRAMC_04.RDADSB = 1;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;         
        Delay_us(3);//Delay(3000);
        rREGDRAMC_04.RDADSB = 0;                                
        r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;                                                                                                                                
        Delay_us(1);                                            

                                                                                                                      
    
        rREGDRAMC_10.RSDM = 0;                                  
        rREGDRAMC_10.RDM32B =  rREG_NORM.RDM32B_NORM;           
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;                                                                    
        rREGDRAMC_24.RPGEN  = 1;                                
        rREGDRAMC_28.REFC = rREG_NORM.REFC_NORM;                
        r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;         
        r_REGDRAMC_28 = *(volatile U32 *)&rREGDRAMC_28;                                                                    
        rREGDRAMC_14.RMA0 = 5;                                  
        rREGDRAMC_14.RBA2SEL = 1;                               
        rREGDRAMC_14.RBA1SEL = 1;                               
        rREGDRAMC_14.RBA0SEL = 1;                               
        r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;                                                                                                                                 
        Delay_us(1);                                                                                      
                                                            
#ifdef MC6_CKE                                              
        rREGDRAMC_20.RDYNCKE = 1;                               
        rREGDRAMC_60.DS_ENA0 = 0;                               
        rREGDRAMC_60.DS_ENA1 = 0;                               
        rREGDRAMC_70.RDCLKOAPM = 1;                             
#endif                                                      
                                                            
#ifdef DYNCLK                                               
        rREGDRAMC_10.RDRAMCPM = 1; 
        Delay_us(2);
#else                                                       
        rREGDRAMC_10.RDRAMCPM = 0;
        Delay_us(2);
#endif                                                      
        r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;         
        r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;         
        r_REGDRAMC_70 = *(volatile U32 *)&rREGDRAMC_70;  
    
}

/*------------------------------------------------------------------------------
Name: HalDramcRegConfig
Description: 
    DRAM controller basic setting initialization. The setting is refer to   
    verilog code of DRAMC
Note:
    
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    
History:
    20140902    Victor Zhang   created
------------------------------------------------------------------------------*/

void HalDramcRegConfig(void)
{
    int i =0;
    
    //  Initial value                      
    rREGDRAMC_00.HTIM = 3;
    rREGDRAMC_00.GTIM = 3;
    rREGDRAMC_00.VTIM = 3;
    rREGDRAMC_00.VHTIM = 3;
    rREGDRAMC_00.PTIM = 3; 

    rREGDRAMC_08.RDMDCLKP_AUTO = 1;
    rREGDRAMC_08.RMDADCLK_AUTO = 1;

    rREGDRAMC_0c.RDQSIN_AUTO = 1;

    rREGDRAMC_10.RMCLK0EN = 1;
    rREGDRAMC_10.RMEMDW = 2;
    rREGDRAMC_10.RMEMTYPE = 3;
    rREGDRAMC_10.REFCOMPACT0 = 1;
    rREGDRAMC_10.RHS_EN = 1;
    rREGDRAMC_10.RMCLKDIFF = 1;
    rREGDRAMC_10.r_RDQSDIFF = 1;
    rREGDRAMC_10.RMEM_RESET = 1; 
    rREGDRAMC_10.RREFBSTEN = 1;
    
    rREGDRAMC_14.RCS0EN = 1;
    rREGDRAMC_14.RMA0 = 1;
    rREGDRAMC_14.RBA1SEL = 3;
    rREGDRAMC_14.RBA0SEL = 4;
    rREGDRAMC_14.RBA2SEL = 2;
    rREGDRAMC_14.RRKSPC = 2;

    rREGDRAMC_18.RNAW_PIPE_1_0 = 2;
    rREGDRAMC_18.RW2RTARD = 2;
    rREGDRAMC_18.RR2WTARD = 1;
    rREGDRAMC_18.RRADSD_OPEN_CLK = 1;
    rREGDRAMC_18.RREFREQ_DEASSERT_NA = 1;
    rREGDRAMC_18.RACC1D_DRAMC_IF_DADS = 1;
    rREGDRAMC_18.RACC1D_AXI_RDBUF_RREQ = 1;
    rREGDRAMC_18.RACC1A_AXI_RDBUF_VALID = 1;
    rREGDRAMC_18.RACC1A_AXI_RDBUF_ALLOC = 1;

    rREGDRAMC_20.RDMIOPWD = 1;
#ifdef ALLONE
    rREGDRAMC_24.RPGTIM = 0xf;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_24.RPGTIM = 0;
    Delay_us(2);
#endif
   
#ifdef NEWPMUGRD
    rREGDRAMC_24.RPGSCLSAL = 0;
    Delay_us(1);
#endif
#ifdef ALLONE
    rREGDRAMC_24.RPGSCLSAL = 0xf;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_24.RPGSCLSAL = 0;
    Delay_us(2);
#endif
    
#ifdef NEWPMUGRD
    rREGDRAMC_24.RAUTOPRE = 0;
    Delay_us(1);
#endif   
#ifdef ALLONE
    rREGDRAMC_24.RAUTOPRE = 0x1;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_24.RAUTOPRE = 0;
    Delay_us(2);
#endif
    
#ifdef NEWPMUGRD
    rREGDRAMC_24.RPRPRI = 0;
    Delay_us(1);
#endif
#ifdef ALLONE
    rREGDRAMC_24.RPRPRI = 0x1;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_24.RPRPRI = 0;
    Delay_us(2);
#endif
    
#ifdef NEWPMUGRD
    rREGDRAMC_24.RPGEN = 0;
    Delay_us(1);
#endif
#ifdef ALLONE
    rREGDRAMC_24.RPGEN = 0x1;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_24.RPGEN = 0;
    Delay_us(2);
#endif
    
#ifdef NEWPMUGRD
    rREGDRAMC_28.RENTERREF = 0;
    Delay_us(1);
#endif
#ifdef ALLONE
    rREGDRAMC_28.RENTERREF = 7;
    Delay_us(2);
#elif ALLZERO 
    rREGDRAMC_28.RENTERREF = 1;
    Delay_us(2);
#else //GOLD 
    rREGDRAMC_28.RENTERREF = 0;
    Delay_us(2);
#endif
    
    rREGDRAMC_28.RDIPNRZ = 1; 
    
#ifdef NEWPMUGRD
    rREGDRAMC_28.RNONPGMEN = 0;
    Delay_us(1);
#endif
#ifdef ALLONE
    rREGDRAMC_28.RNONPGMEN = 1;
    Delay_us(2);
#else //(GOLD or ALLZERO) 
    rREGDRAMC_28.RNONPGMEN = 0;
    Delay_us(2);
#endif   
    
    rREGDRAMC_28.RREGPIPE = 1;  

    rREGDRAMC_34.r_RDCMPON = 1;
  
    rREGDRAMC_4c.RWTWLO = 0xf;

#ifdef DYNCLK
    rREGDRAMC_60.DS_ENA1 = 1;
    Delay_us(2);
    rREGDRAMC_60.DS_ENA0 = 1;
    Delay_us(2);
#endif
       
    rREGDRAMC_64.RSCMDADCLK1_AUTO = 1; 
    
    rREGDRAMC_60.LP2_RDQSCK = 4; 
    rREGDRAMC_64.RSCMDADCLK1_AUTO = 1; 
#ifdef DYNCLK
    rREGDRAMC_70.RDCLKOAPM = 1;
    Delay_us(2);
#else
    rREGDRAMC_70.RDCLKOAPM = 0;
    Delay_us(2);
#endif

    rMR1_ADDR_LPDDR2.res2 = 1;  // 16'h0001
    rMR2_ADDR_LPDDR2.res3 = 2;  // 16'h0002

    rRDMR_REG.RDMRRTTWR = 1;                                              
    rDDR3P_MR0.MR0_BT = 1;            
    rDDR3P_MR0.MR0_DLR = 1;           
                                   
    if(1 == rREGDRAMC_10.RDMBL8)
    {
     rDDR3P_MR0.DDR3_BL = 0;       
    }
    else
    {
     rDDR3P_MR0.DDR3_BL = 2;
    }
    rDDR3P_MR0.DDR3_CL = rREGDRAMC_1c.RCL;                   
    rDDR3P_MR0.DDR3_WR = rREGDRAMC_1c.RWRCOV + 1;            
    rDDR3P_MR0.DDR3_PD = 1;                                  
                                    //
    rDDR3P_MR1.MR1_DLE = 0;                                  
    rDDR3P_MR1.MR1_QOF = 0;                                  

    rDDR3P_MR1.DDR3_ODS_0 = rRDMR_REG.RDMRODS & 0x1;         
    rDDR3P_MR1.DDR3_ODS_1 = (rRDMR_REG.RDMRODS >> 1) & 0x1; 
    rDDR3P_MR1.DDR3_RTT_0 = rRDMR_REG.RDMRRTT & 0x1;         
    rDDR3P_MR1.DDR3_RTT_1 = (rRDMR_REG.RDMRRTT >> 1) & 0x1;                           
    rDDR3P_MR1.DDR3_RTT_2 = (rRDMR_REG.RDMRRTT >> 2) & 0x1;                           

    rDDR3P_MR2.DDR3_CWL = rREGDRAMC_20.RCWL - 1;         
                                    //wire            
    rDDR3P_MR2.DDR3_SRT = rRDMR_REG.RDMRSRT;             
    rDDR3P_MR2.DDR3_RTT_WR = rRDMR_REG.RDMRRTTWR;        

    Delay_us(1);
    rREGDRAMC_68.RCMD_PRE = 1;     
    rREGDRAMC_14.RCMD1T0 = 1;      
#ifdef BOOT_DDR   
    rREGDRAMC_10.RMCLK0EN = 1;     
    rREGDRAMC_10.RMEMDW = 1;       
    
#ifdef LPDDR2_MEMORY    
    //DRAMC Register Config with mode DDR2                    
    rREGDRAMC_10.RMEMTYPE = 3; 
    
    rREGDRAMC_18.RR2WTARD = 2;      
    rREGDRAMC_0c.RDRPH = 7;         
    rREGDRAMC_0c.RDSDLY = 0x1c;     
    rREGDRAMC_20.LPDDR2_RTRCD = 0xa;
    rREGDRAMC_20.RPAB_LP = 5;       
    rREGDRAMC_1c.RWRCOV = 7;        
    rREGDRAMC_1c.RTRTP = 3;         
    rREGDRAMC_1c.RTWTR = 4;         
    rREGDRAMC_1c.RTRAS = 0xf;       
    rREGDRAMC_1c.RT8BKEN = 0;       
    //rREGDRAMC_1c.RCL = 3;         
    rREGDRAMC_1c.RCL = 6;           
    rREGDRAMC_1c.RTRRD = 5;         
    rREGDRAMC_1c.RTRFC = 0x20;      
    rREGDRAMC_20.RFAW = 0xf;        
    rREGDRAMC_20.RCWL = 4;          
    rREGDRAMC_20.RTRCD = 7;         
    rREGDRAMC_20.RTCKE = 1;         
    rREGDRAMC_20.RTRP = 5;          
    rREGDRAMC_20.RTXPD = 0;         
    rREGDRAMC_60.LP2_RDQSCK = 3;    
    rREGDRAMC_20.RPPB_LP = 6;       
    rREGDRAMC_20.RPAB_LP = 8;       
    rREGDRAMC_20.LPDDR2_RTRCD = 0xa;
   
    rREGDRAMC_64.TZQCS = 0x30;      
    rREGDRAMC_28.REFC = 0;          
    rREGDRAMC_0c.RIENEXTEND = 1;    

    rREGDRAMC_0c.RDSDLY_EXT = 1;    

    
#else    

    //DRAMC Register Config with mode DDR3                   
    rREGDRAMC_10.RMEMTYPE = 2;
        
    rREGDRAMC_0c.RDRPH = 0x7;
    rREGDRAMC_0c.RDSDLY = 0x1c;
    
    rREGDRAMC_20.RFAW = 0x15;
    rREGDRAMC_20.RCWL = 0x2;
    rREGDRAMC_20.RTRCD = 0x7;
    rREGDRAMC_20.RTCKE = 0x1;
    rREGDRAMC_20.RTRP = 0x7;
    rREGDRAMC_20.RTXPD = 0;
    
    rREGDRAMC_1c.RWRCOV = 0x3;
    rREGDRAMC_1c.RTRTP = 0x2;
    rREGDRAMC_1c.RTWTR = 0x3;
    rREGDRAMC_1c.RTRAS = 0x5;
    rREGDRAMC_1c.RT8BKEN = 0;
    rREGDRAMC_1c.RCL = 0x1;
    rREGDRAMC_1c.RTRRD = 0x4;
    rREGDRAMC_1c.RTRFC = 0xb0;
    
    rREGDRAMC_28.REFC = 0x80;
#endif   
    //DRAMC Register Config with mode common                   
    rREGDRAMC_10.RQGT2 = 1;

    if(0 == BANK8_EN)
    {
        rREGDRAMC_24.RBKEN = 2;
        rREGDRAMC_14.RBA2EN = 0;
    }
    else
    {
        rREGDRAMC_24.RBKEN = 3;
        rREGDRAMC_14.RBA2EN = 1;
        rREGDRAMC_14.RMA0 = 5;
        rREGDRAMC_14.RBA2SEL = 1;
        rREGDRAMC_14.RBA1SEL = 1;
        rREGDRAMC_14.RBA0SEL = 1;        
    }
    //rREGDRAMC_1c.RT8BKEN = 1;
      rREGDRAMC_04.RAUTOADJSTART = 1;
      rREGDRAMC_14.RBA1SEL = 1;
      rREGDRAMC_14.RBA0SEL = 1;  
      rREGDRAMC_10.RMEM_RESET = 1;
      rREGDRAMC_10.r_RDQSDIFF = 1;
      
      rREGDRAMC_24.RBKACT = 1;
      rREGDRAMC_0c.RIENEXTEND = 1;
      rREGDRAMC_1c.RT8BKEN = 1;
      rREGDRAMC_14.RBA2EN = 1;
      rREGDRAMC_14.RMA0 = 7;
      rREGDRAMC_14.RBA2SEL = 2;
      rREGDRAMC_14.RBA1SEL = 2;
      rREGDRAMC_14.RBA0SEL = 3;  
#endif //#ifdef BOOT_DDR
      
#ifdef MC6_MA0
      Delay_us(1);
      rREGDRAMC_14.RBA2EN = 0;
      rREGDRAMC_14.RMA0 = 0;
      rREGDRAMC_14.RBA1SEL = 1;
      rREGDRAMC_14.RBA0SEL = 1;  
      rREGDRAMC_24.RBKEN = 2;
#endif
      
#ifdef MC6_MA1
      Delay_us(1);
      rREGDRAMC_14.RBA2EN = 0;
      rREGDRAMC_14.RMA0 = 1;
      rREGDRAMC_14.RBA1SEL = 2;
      rREGDRAMC_14.RBA0SEL = 2;  
      rREGDRAMC_24.RBKEN = 2;
#endif
      
#ifdef MC6_MA5
      Delay_us(1);
      rREGDRAMC_14.RBA2EN = 1;
      rREGDRAMC_14.RMA0 = 5;
      rREGDRAMC_14.RBA0SEL = 1;  
      rREGDRAMC_14.RBA1SEL = 1;
      rREGDRAMC_14.RBA2SEL = 1;
      rREGDRAMC_24.RBKEN = 3;
#endif
      
#ifdef MC6_MA6
      Delay_us(1);
      rREGDRAMC_14.RBA2EN = 1;
      rREGDRAMC_14.RMA0 = 6;
      rREGDRAMC_14.RBA0SEL = 3;  
      rREGDRAMC_14.RBA1SEL = 3;
      rREGDRAMC_14.RBA2SEL = 3;
      rREGDRAMC_24.RBKEN = 3;
#endif
      
      Delay_us(1);
#ifdef MD8B
      rREGDRAMC_10.RDM32B = 1;
      rREGDRAMC_10.RDMBL8 = 1;
#else
      rREGDRAMC_10.RDM32B = 0;
    #ifdef MDBL4
      rREGDRAMC_10.RDMBL8 = 0;
    #else//MDBL8
      rREGDRAMC_10.RDMBL8 = 1;
    #endif       
#endif
      
    // MD16B & MDBL8
    //rREGDRAMC_10.RDM32B = 0;
    //rREGDRAMC_10.RDMBL8 = 1;
#ifdef MC6_CKE
      Delay_us(1);
      rREGDRAMC_20.RDYNCKE = 1;
#endif
      
#ifdef BURST_REFRESH
      Delay_us(1);
      rREGDRAMC_10.REFBST = 1;
      rREGDRAMC_28.RSFREFRK1EN = 1;
      rREGDRAMC_28.RSFREFRK0EN = 1;
      
      rREGDRAMC_28.RENTERREF = 3;
      rREGDRAMC_10.RREFBSTEN = 1;
#endif
      
#ifdef ZQCS
      Delay_us(1);
      rREGDRAMC_48.RZQCS_FTST = 1;
      rREGDRAMC_48.RZQCS_AUTO = 1;
#endif
      
    rREGDRAMC_10.RHS_EN = 1;
    rREGDRAMC_68.RCMD_PRE = 1;
    rREGDRAMC_60.S_PD_ENA = 1;
    rREGDRAMC_60.W_PD_ENA = 3;
    rREGDRAMC_14.RCMD1T0 = 1;
    rREGDRAMC_14.RCMD1T1 = 1;
    rREGDRAMC_10.RCMPEN_HEAD = 1;
    rREGDRAMC_60.S_PD_ENA = 1;
     
    //write to phy memory                            
#ifdef GOLD 
    Delay_us(1);
    r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;  
    Delay_us(1);
    r_REGDRAMC_0c = *(volatile U32 *)&rREGDRAMC_0c;  
    Delay_us(1);
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;  
    Delay_us(1);
    r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;  
    Delay_us(1);
    r_REGDRAMC_18 = *(volatile U32 *)&rREGDRAMC_18;  
    Delay_us(1);
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;  
    Delay_us(1);
    r_REGDRAMC_20 = *(volatile U32 *)&rREGDRAMC_20;  
    Delay_us(1);
    r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;  
    Delay_us(1);
    r_REGDRAMC_28 = *(volatile U32 *)&rREGDRAMC_28;  
    Delay_us(1);
    r_REGDRAMC_48 = *(volatile U32 *)&rREGDRAMC_48;  
    Delay_us(1);
    r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;  
    Delay_us(1);
    r_REGDRAMC_64 = *(volatile U32 *)&rREGDRAMC_64;  
    Delay_us(1);
    r_REGDRAMC_68 = *(volatile U32 *)&rREGDRAMC_68;  
    Delay_us(1);
    r_REGDRAMC_70 = *(volatile U32 *)&rREGDRAMC_70;  
    Delay_us(1);
#else
    r_REGDRAMC_00 = *(volatile U32 *)&rREGDRAMC_00;  
    Delay_us(1);
    r_REGDRAMC_04 = *(volatile U32 *)&rREGDRAMC_04;  
    Delay_us(1);
    r_REGDRAMC_08 = *(volatile U32 *)&rREGDRAMC_08;  
    Delay_us(1);
    r_REGDRAMC_0c = *(volatile U32 *)&rREGDRAMC_0c;  
    Delay_us(1);
    r_REGDRAMC_10 = *(volatile U32 *)&rREGDRAMC_10;  
    Delay_us(1);
    r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;  
    Delay_us(1);
    r_REGDRAMC_18 = *(volatile U32 *)&rREGDRAMC_18;  
    Delay_us(1);
    r_REGDRAMC_1c = *(volatile U32 *)&rREGDRAMC_1c;  
    Delay_us(1);
    r_REGDRAMC_20 = *(volatile U32 *)&rREGDRAMC_20;  
    Delay_us(1);
    r_REGDRAMC_24 = *(volatile U32 *)&rREGDRAMC_24;  
    Delay_us(1);
    r_REGDRAMC_28 = *(volatile U32 *)&rREGDRAMC_28;  
    Delay_us(1);
    r_REGDRAMC_2c = *(volatile U32 *)&rREGDRAMC_2c;  
    Delay_us(1);
    r_REGDRAMC_30 = *(volatile U32 *)&rREGDRAMC_30;  
    Delay_us(1);
    r_REGDRAMC_34 = *(volatile U32 *)&rREGDRAMC_34;  
    Delay_us(1);
    r_REGDRAMC_38 = *(volatile U32 *)&rREGDRAMC_38;  
    Delay_us(1);
    r_REGDRAMC_3c = *(volatile U32 *)&rREGDRAMC_3c;  
    Delay_us(1);
    r_REGDRAMC_40 = *(volatile U32 *)&rREGDRAMC_40;  
    Delay_us(1);
    r_REGDRAMC_44 = *(volatile U32 *)&rREGDRAMC_44;  
    Delay_us(1);
    r_REGDRAMC_48 = *(volatile U32 *)&rREGDRAMC_48;  
    Delay_us(1);
    r_REGDRAMC_4c = *(volatile U32 *)&rREGDRAMC_4c;  
    Delay_us(1);
    r_REGDRAMC_54 = *(volatile U32 *)&rREGDRAMC_54;  
    Delay_us(1);
    r_REGDRAMC_58 = *(volatile U32 *)&rREGDRAMC_58;  
    Delay_us(1);
    r_REGDRAMC_5c = *(volatile U32 *)&rREGDRAMC_5c;  
    Delay_us(1);
    r_REGDRAMC_60 = *(volatile U32 *)&rREGDRAMC_60;  
    Delay_us(1);
    r_REGDRAMC_64 = *(volatile U32 *)&rREGDRAMC_64;  
    Delay_us(1);
    r_REGDRAMC_68 = *(volatile U32 *)&rREGDRAMC_68;  
    Delay_us(1);
    r_REGDRAMC_70 = *(volatile U32 *)&rREGDRAMC_70;  
    Delay_us(1);
    r_REGDRAMC_74 = *(volatile U32 *)&rREGDRAMC_74;  
    Delay_us(1);
    r_REGDRAMC_78 = *(volatile U32 *)&rREGDRAMC_78;  
    Delay_us(1);
    r_REGDRAMC_7c = *(volatile U32 *)&rREGDRAMC_7c;  
    Delay_us(1);
#endif
}


void HAL_DramCeEn(U32 ucCE)
{
    if (DRAM_CE0 == ucCE)
    {
        rREGDRAMC_14.RCS0EN = 1;
        rREGDRAMC_14.RCS1EN = 0;
    }
    else if (DRAM_CE1 == ucCE)
    {
        rREGDRAMC_14.RCS0EN = 0;
        rREGDRAMC_14.RCS1EN = 1;
    }
    else
    {
        ; // default value  1 CE 
    }    
    r_REGDRAMC_14 = *(volatile U32 *)&rREGDRAMC_14;
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_DramInit
Description: 
    DRAM controller register initialization interface.
Note:
    
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function before accessing the DRAM.
History:
    20140902    Victor Zhang   created
------------------------------------------------------------------------------*/


void HAL_DramcInit(void)
{
    U32 ulPatch,i;
    //*(volatile U32 *)(REG_BASE_PMU + 0x18) = 0x00120191;        
    //*(volatile U32 *)(REG_BASE_PMU + 0x1c) = 0;              
                             
    Delay_us(5);                      
    HalDramcRegConfig();
    Delay_us(1); 

    for (i=0;i<DRAM_CE_NUM;i++)
    {
        HAL_DramCeEn(i);
#ifdef LPDDR2_MEMORY              
        HalDramInitLPDDR2();   
#else                           
        HalDramInitDDR3();                          
#endif
    }

    //init RDSDLY_EXT
    r_REGDRAMC_0c |= (1<<29);    

}
#endif

/*================================= FILE END  ================================*/

