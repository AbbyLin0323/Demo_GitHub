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
Filename     :  HAL_DramConfig.h                                     
Version      :  Ver 1.0                                               
Date         :  20130602                                   
Author       :  Victor Zhang

Description: 
    Head of DRAM controller initialization for COSIM. 

Modification History:
20130602     Victor Zhang   Creating file referred to Susan's DRAM controller init 
20140902     Victor Zhang   Modification as new coding style
*******************************************************************************/

#ifndef __DDR_CONFIG__
#define __DDR_CONFIG__
/*------------------------------------------------------------------------------
    INCLUDING FILES 
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION 
------------------------------------------------------------------------------*/
//#define LPDDR2_MEMORY
#define BANK8_EN 0
#define GOLD 
//#define ALLONE
//#define ALLZERO
//#define NEWPMUGRD
//#define DYNCLK
#define BOOT_DDR
//#define MC6_MA0
//#define MC6_MA1
#define MC6_MA5
//#define MC6_MA6
//#define MD8B
//#define MDBL4
#define MDBL8
//#define MC6_CKE
#define DM533
//#define DM400
//#define DM333
//#define BURST_REFRESH
//#define ZQCS

/*------------------------------------------------------------------------------
   REGISTER LOCATION    
------------------------------------------------------------------------------*/
#define r_REGDRAMC_00    (*(volatile U32 *)(REG_BASE_DRAMC + 0x00))
#define r_REGDRAMC_04    (*(volatile U32 *)(REG_BASE_DRAMC + 0x04))
#define r_REGDRAMC_08    (*(volatile U32 *)(REG_BASE_DRAMC + 0x08))
#define r_REGDRAMC_0c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x0c))
#define r_REGDRAMC_10    (*(volatile U32 *)(REG_BASE_DRAMC + 0x10))
#define r_REGDRAMC_14    (*(volatile U32 *)(REG_BASE_DRAMC + 0x14))
#define r_REGDRAMC_18    (*(volatile U32 *)(REG_BASE_DRAMC + 0x18))
#define r_REGDRAMC_1c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x1c))
#define r_REGDRAMC_20    (*(volatile U32 *)(REG_BASE_DRAMC + 0x20))
#define r_REGDRAMC_24    (*(volatile U32 *)(REG_BASE_DRAMC + 0x24))
#define r_REGDRAMC_28    (*(volatile U32 *)(REG_BASE_DRAMC + 0x28))
#define r_REGDRAMC_2c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x2c))
#define r_REGDRAMC_30    (*(volatile U32 *)(REG_BASE_DRAMC + 0x30))
#define r_REGDRAMC_34    (*(volatile U32 *)(REG_BASE_DRAMC + 0x34))
#define r_REGDRAMC_38    (*(volatile U32 *)(REG_BASE_DRAMC + 0x38))
#define r_REGDRAMC_3c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x3c))
#define r_REGDRAMC_40    (*(volatile U32 *)(REG_BASE_DRAMC + 0x40))
#define r_REGDRAMC_44    (*(volatile U32 *)(REG_BASE_DRAMC + 0x44))
#define r_REGDRAMC_48    (*(volatile U32 *)(REG_BASE_DRAMC + 0x48))
#define r_REGDRAMC_4c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x4c))
#define r_REGDRAMC_50    (*(volatile U32 *)(REG_BASE_DRAMC + 0x50))
#define r_REGDRAMC_54    (*(volatile U32 *)(REG_BASE_DRAMC + 0x54))
#define r_REGDRAMC_58    (*(volatile U32 *)(REG_BASE_DRAMC + 0x58))
#define r_REGDRAMC_5c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x5c))
#define r_REGDRAMC_60    (*(volatile U32 *)(REG_BASE_DRAMC + 0x60))
#define r_REGDRAMC_64    (*(volatile U32 *)(REG_BASE_DRAMC + 0x64))
#define r_REGDRAMC_68    (*(volatile U32 *)(REG_BASE_DRAMC + 0x68))
#define r_REGDRAMC_6c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x6c))
#define r_REGDRAMC_70    (*(volatile U32 *)(REG_BASE_DRAMC + 0x70))
#define r_REGDRAMC_74    (*(volatile U32 *)(REG_BASE_DRAMC + 0x74))
#define r_REGDRAMC_78    (*(volatile U32 *)(REG_BASE_DRAMC + 0x78))
#define r_REGDRAMC_7c    (*(volatile U32 *)(REG_BASE_DRAMC + 0x7c))

#define r_REGPMU_18      (*(volatile U32 *)(REG_BASE_PMU + 0x18))
#define r_REGPMU_1c      (*(volatile U32 *)(REG_BASE_PMU + 0x1c))

/*------------------------------------------------------------------------------
   DECLARATION OF REGISTER STRUCTURE    
------------------------------------------------------------------------------*/
typedef struct _REGDRAMC_00{
    U32 HTIM:4 ;
    U32 GTIM:4 ;
    U32 VTIM:4 ;
    U32 VHTIM:4 ;
    U32 PTIM:4 ;
    U32 RDARB:2 ;
    U32 res:10 ;
}REGDRAMC_00;
 
typedef struct _REGDRAMC_04{
    U32 MR_MA:16 ;
    U32 MR_BA:3 ;
    U32 RDADSB:1 ;
    U32 res:9 ;
    U32 RAUTOADJFAIL:1 ;
    U32 RAUTOADJDONE:1 ;
    U32 RAUTOADJSTART:1 ;
}REGDRAMC_04;
 
typedef struct _REGDRAMC_08{
    U32 RDMDCLKP_DLY:7 ;
    U32 RDMDCLKP_AUTO:1 ;
    U32 RMDADCLK_DLY:7 ;
    U32 RMDADCLK_AUTO:1 ;
    U32 RSCMDADCLK_DLY:7 ;
    U32 RSCMDADCLK_AUTO:1 ;
    U32 RCTLDCLK_DLY:7 ;
    U32 RCTLDCLK_AUTO:1 ;
}REGDRAMC_08;
 
typedef struct _REGDRAMC_0C{
    U32 RDQSIN:7 ;
    U32 RDQSIN_AUTO:1 ;
    U32 RDSDLY:6 ;
    U32 res1:1 ;
    U32 RIENEXTEND:1 ;
    U32 RDRPH:4 ;
    U32 RDWADV:2 ;
    U32 res2:2 ;
    U32 RCKEAPIPE:1 ;
    U32 RD2TREQ:1 ;
    U32 RD2TCMD:1 ;
    U32 RD2TPGCLS:1 ;
    U32 RDPGPIPE:1 ;
    U32 RDSDLY_EXT:2 ;
    U32 RD2TINTCMD:1 ;
}REGDRAMC_0C;
 
typedef struct _REGDRAMC_10{
    U32 RMEMTYPE:3 ;
    U32 RDMBL8:1 ;
    U32 RMEMDW:2 ;
    U32 RMCLK0EN:1 ;
    U32 RDM32B:1 ;
    U32 res1:1 ; //0
    U32 RHS_EN:1 ;
    U32 RQTHresH:2 ;
    U32 RQNE4:1 ;
    U32 RQGT2:1 ;
    U32 REFCOMPACT0:1 ;
    U32 REFBST:1 ;
    U32 r_RDQSDIFF:1 ;
    U32 RMCLKDIFF:1 ;
    U32 RMCLKTEST:1 ;
    U32 RTESTSEL:4 ;
    U32 RDRAMCPM:1 ;
    U32 RSDM:3 ;
    U32 RMEM_RESET:1 ;
    U32 RCMPEN_ALWSON:1 ;
    U32 RCMPEN_HEAD:1 ;
    U32 RREFBSTEN:1 ;
    U32 res2:1 ;
}REGDRAMC_10;
 
typedef struct _REGDRAMC_14{
    U32 RCMD1T0:1 ;
    U32 RMA0:3 ;
    U32 RCS0EN:1 ;
    U32 RCMD1T1:1 ;
    U32 RCS1EN:1;
    U32 res1:1;

    U32 RRKSPC:2;
    U32 res2:6 ;

    U32 RBA0SEL:3 ;
    U32 res3:1 ;
    U32 RBA1SEL:3 ;
    U32 res4:1 ;
    
    U32 RBA2SEL:3 ;
    U32 RBA2EN:1 ;
    U32 r_RDDRLV_RE:2 ;
    U32 r_RDDRLV_WR:2 ;
}REGDRAMC_14;
 
typedef struct _REGDRAMC_18{
    U32 RDFW2R:3 ;
    U32 RFR2W:3 ;
    U32 RNAW_PIPE_1_0:2 ;
    U32 RR2WTARD:4 ;
    U32 RW2RTARD:4 ;
    U32 RACC1A_AXI_RDBUF_ALLOC:1 ;
    U32 RACC1A_AXI_RDBUF_VALID:1 ;
    U32 RACC1D_AXI_RDBUF_RREQ:1 ;
    U32 RACC1D_DRAMC_IF_DADS:1 ;
    U32 RREFREQ_DEASSERT_NA:1 ;
    U32 RRADSD_OPEN_CLK:1 ;
    U32 res1:2 ;
    U32 RFR2R:3 ;
    U32 RFW2W:3 ;
    U32 res2:2 ;
}REGDRAMC_18;
 
typedef struct _REGDRAMC_1C{
    U32 RTRFC:8;
    U32 RTRRD:3 ;
    U32 res4:1 ;
    U32 RCL:3 ;
    U32 RT8BKEN:1 ;
    U32 RTRAS:4 ;
    U32 RTWTR:3 ;
    U32 res3:1 ;
    U32 RTRTP:2 ;
    U32 res2:2 ;
    U32 RWRCOV:3 ;
    U32 res1:1 ;
}REGDRAMC_1C;
 
typedef struct _REGDRAMC_20{
    U32 RTXPD:1 ;
    U32 RTRP:3 ;
    U32 RTCKE:1 ;
    U32 RTRCD:3 ;
    U32 RCWL:3 ;
    U32 RFAW:5 ;
    U32 RPPB_LP:4 ;
    U32 RPAB_LP:4 ;
    U32 LPDDR2_RTRCD:4 ;
    U32 RDYNCKE:1 ;
    U32 RDMIOPWD:1 ;
    U32 RCWL_PIPE:1 ;
    U32 RCL_PIPE:1 ;
}REGDRAMC_20;
 
typedef struct _REGDRAMC_24{
    U32 RPGSCLSAL:4 ;
    U32 RPGTIM:4 ;
    U32 RPGEN:1 ;
    U32 RBKACT:1 ;
    U32 RPRPRI:1 ;
    U32 RPGDYNOPT:1 ;
    U32 RAUTOPRE:1 ;
    U32 RBKSCMB:1 ;
    U32 RBKEN:2 ;
    U32 RBKSWAP:1 ;
    U32 RBKREMIX:1 ;
    U32 res1:6 ;
    U32 res2:8 ;
}REGDRAMC_24;
 
typedef struct _REGDRAMC_28{
    U32 REFC:8 ;
    U32 RREGPIPE:1 ;
    U32 RNONPGMEN:1 ;
    U32 RDIPNRZ:1 ;
    U32 RSFREFRK0EN:1 ;
    U32 RSFREFRK1EN:1 ;
    U32 RENTERREF:3 ;
    U32 MRS_PAROP:8 ;
    U32 RPAREN:1 ;
    U32 RSRFBLOCK0:1;
    U32 res:6 ;
}REGDRAMC_28;
 
typedef struct _REGDRAMC_2C{
    U32 RODTTAR:2 ;
    U32 RODTCMD2T:1 ;
    U32 RODTEN:1 ;
    U32 res1:4 ;
    U32 RPREODT:2 ;
    U32 RPSTODTR:2 ;
    U32 RPSTODTW:2 ;
    U32 RCS0ODT:1 ;
    U32 res2:1 ;
    U32 res3:16 ;
}REGDRAMC_2C;
 
typedef struct _REGDRAMC_30{
//    U32 res1:16 ;
    
    U32 RODTNOS:4;
    U32 RODTPOS:4;        
    U32 RDQNOS:4;
    U32 RDQPOS:4;
    
    U32 r_RODTNIS:1 ;
    U32 r_RODTPIS:1 ;
    U32 res2:8 ;
}REGDRAMC_30;
 
typedef struct _REGDRAMC_34{
    U32 r_RDAUTOCMP:1 ;
    U32 r_RDCMPON:1 ;
    U32 RDCMP_CLB_SEL:2 ;
    U32 RDCMP_TRIG:1 ;
    U32 res1:3 ;
    U32 RODTAD:1 ;
    U32 RODTAU:1 ;
    U32 RPADENR:1 ;
    U32 RPADODTEN:1 ;
    U32 res2:4 ;
    U32 r_RDQSA_RS:1 ;
    U32 r_RDQA_RS:1 ;
    U32 res3:2 ;
    U32 r_RDQSA_DS:1 ;
    U32 r_RDQA_DS:1 ;
    U32 res4:2 ;
    U32 r_RCSA_DS:1 ;
    U32 r_RMAA_DS:1 ;
    U32 r_RDCLKA_DS:1 ;
    U32 res5:5 ;
}REGDRAMC_34;
 
typedef struct _REGDRAMC_38{
    U32 r_RDQSDV_AUTO:1 ;
    U32 r_RDQDV_AUTO:1 ;
    U32 r_RMAADV_AUTO:1 ;
    U32 res:5 ;
    U32 r_RDQDVN:4 ;
    U32 r_RDQDVP:4 ;
    U32 r_RMAADVN:4 ;
    U32 r_RMAADVP:4 ;
    U32 r_RCSDVN:4 ;
    U32 r_RCSDVP:4 ;
}REGDRAMC_38;
 
typedef struct _REGDRAMC_3C{
    U32 r_RDCLKP_DVN:4 ;
    U32 r_RDCLKP_DVP:4 ;
    U32 r_RDCLKN_DVN:4 ;
    U32 r_RDCLKN_DVP:4 ;
    U32 r_RDQSP_DVN:4 ;
    U32 r_RDQSP_DVP:4 ;
    U32 r_RDQSN_DVN:4 ;
    U32 r_RDQSN_DVP:4 ;
}REGDRAMC_3C;
 
typedef struct _REGDRAMC_40{
    U32 res1:8 ;
    U32 RDQM0S0R:1 ;
    U32 RDQM0S1R:1 ;
    U32 RDQM0S0F:1 ;
    U32 RDQM0S1F:1 ;
    U32 RDQM1S0R:1 ;
    U32 RDQM1S1R:1 ;
    U32 RDQM1S0F:1 ;
    U32 RDQM1S1F:1 ;
    U32 res2:8 ;
}REGDRAMC_40;
 
typedef struct _REGDRAMC_44{
    U32 RDQS0S0R:1 ;
    U32 RDQS0S1R:1 ;
    U32 RDQS0S0F:1 ;
    U32 RDQS0S1F:1 ;
    U32 RDQS1S0R:1 ;
    U32 RDQS1S1R:1 ;
    U32 RDQS1S0F:1 ;
    U32 RDQS1S1F:1 ;
    U32 res1:8 ;
    U32 RDQ0S0R:1 ;
    U32 RDQ0S1R:1 ;
    U32 RDQ0S0F:1 ;
    U32 RDQ0S1F:1 ;
    U32 RDQ1S0R:1 ;
    U32 RDQ1S1R:1 ;
    U32 RDQ1S0F:1 ;
    U32 RDQ1S1F:1 ;
    U32 res2:1 ;
}REGDRAMC_44;


typedef struct _REGDRAMC_48{
    U32 RZQCS_AUTO:1 ;
    U32 RTZQC:2 ;
    U32 RZQCS_FTST:1 ;
    U32 res:28 ;
}REGDRAMC_48;
 
typedef struct _REGDRAMC_4C{
    U32 RWLEVEN:1 ;
    U32 RWLEVTRIG:1 ;
    U32 res1:6 ;
    U32 RRLEV_SEL:1 ;
    U32 res2:7 ;
    U32 RWTWLO:4 ;
    U32 res3:12 ;
}REGDRAMC_4C;
 
typedef struct _REGDRAMC_50{

    U32 res:24 ;
    U32 REG_RSVD:8;
}REGDRAMC_50;
 
typedef struct _REGDRAMC_54{
    U32 RWLEVDLY0:7 ;
    U32 res1:1 ;
    U32 RWLEVDLY1:7 ;
    U32 res2:1 ;
    U32 res3:16 ;
}REGDRAMC_54;
 
typedef struct _REGDRAMC_58{
    U32 RWLEVVAL0_CS0:7 ;
    U32 res1:1 ;
    U32 RWLEVVAL1_CS0:7 ;
    U32 res2:1 ;
    U32 res3:1 ;
}REGDRAMC_58;
 
typedef struct _REGDRAMC_5C{
    U32 RRLEVVAL0_CS0:7 ;
    U32 res1:1 ;
    U32 RRLEVVAL1_CS0:7 ;
    U32 res2:1 ;
    U32 res3:1 ;
}REGDRAMC_5C;
 
typedef struct _REGDRAMC_60{
    U32 AUTO_MR4_ENA:1 ;
    U32 S_PD_ENA:1 ;
    U32 W_PD_ENA:2 ;
    U32 res1:1 ;
    U32 DS_ENA0:1 ;
    U32 DS_ENA1:1 ;
    U32 r_MAA_W_CTL:1 ;
    U32 LP2_RDQSCK:3 ;
    U32 res2:5 ;
    U32 NAR_MC_CLK_RATIOED:5 ;
    U32 res3:11 ;
}REGDRAMC_60;
 
typedef struct _REGDRAMC_64{
    U32 TZQCS:6 ;
    U32 res1:2 ;
    U32 RSCMDADCLK1_DLY:7 ;
    U32 RSCMDADCLK1_AUTO:1 ;
    U32 RCAS0R:1 ;
    U32 RCAS1R:1 ;
    U32 RCAS0F:1 ;
    U32 RCAS1F:1 ;
    U32 res2:12 ;
}REGDRAMC_64;
 
typedef struct _REGDRAMC_68{
    U32 RKASWAP:1 ;
    U32 res1:3 ;
    U32 RBA0_SWAP:2 ;
    U32 res2:2 ;
    U32 RBA1_SWAP:3 ;
    U32 res3:1 ;
    U32 RBA2_SWAP:2 ;
    U32 res4:2 ;
    U32 RCMD_PRE:1 ;
    U32 R2TCMD_ORG:1 ;
    U32 RHEADPIPE:1 ;
    U32 RDEBUG_SEL:1 ;
    U32 res5:4 ;
    U32 res6:8 ;
}REGDRAMC_68;
  
typedef struct _REGDRAMC_6C{
    U32 res1:15 ;
    U32 MRR_RDY:1 ;
    U32 res2:16 ;
}REGDRAMC_6C;
 
typedef struct _REGDRAMC_70{
    U32 res1:21 ;
    U32 RHFMODE_EN:1 ;
    U32 res2:9 ;
    U32 RDCLKOAPM:1 ;
}REGDRAMC_70;
 
typedef struct _REGDRAMC_74{
    U32 PASR_EN0:1 ;
    U32 PASR_EN1:1 ;
    U32 PASR_MODE:1 ;
    U32 RSIZE:2 ;
    U32 res1:3 ;
    U32 RPARTIM:4 ;
    U32 res2:4 ;
    U32 PASR_MSK0:8 ;
    U32 PASR_MSK1:8 ;
}REGDRAMC_74;
 
typedef struct _REGDRAMC_78{
    U32 RDCLKOAD_DLY:7;
    U32 res1:1 ;
    U32 res2:24 ;    
}REGDRAMC_78;

typedef struct _REGDRAMC_7C{
    U32 RWLEVVAL0_CS0_DQS:7;
    U32 res1:1 ;
    U32 RWLEVVAL1_CS0_DQS:7 ;
    U32 res2:1;
    U32 res3:16;    
}REGDRAMC_7C;    

/*------------------------------------------------------------------------------
   DECLARATION OF PARAMETER STRUCTURE   
------------------------------------------------------------------------------*/

typedef struct _MR1_ADDR_LPDDR2
{
    U32 res1:8;
    U32 MR_burst_length:3;
    U32 MR_burst_type:1;
    U32 MR_wrap:1;
    U32 MR_write_recovery:3;
    U32 res2:16;   //  16'h0001
}MR1_ADDR_LPDDR2;

typedef struct _MR2_ADDR_LPDDR2
{
    U32 res1:8;
    U32 MR_RL_WL:4;
    U32 res2:4;
    U32 res3:16;    //  16'h0002
}MR2_ADDR_LPDDR2;


typedef struct _REG_NORM
{
    U32 RDM32B_NORM:1;
    U32 RBA2EN_NORM:1;
    U32 RBA2SEL_NORM:3;
    U32 RBA1SEL_NORM:3;
    U32 RBA0SEL_NORM:3;
    U32 RMA0_NORM:3;
    U32 RCL_NORM:3;
    U32 REFC_NORM:8;
    U32 res:15;
}REG_NORM;

//    typedef struct _DDR3P_MR3_
    
typedef struct _DDR3P_MR2_
{
    U16 res0:3;
    U16 DDR3_CWL:3;
    U16 DDR3_ASR:1;
    U16 DDR3_SRT:1;
    U16 res1:1;
    U16 DDR3_RTT_WR:2;
    U16 res2:2;
    U16 res3:3;   
}DDR3P_MR2;
typedef struct _DDR3P_MR0_
{
    U16 DDR3_BL:2;
    U16 res0:1;
    U16 MR0_BT:1;
    U16 DDR3_CL:3;
    U16 res1:1;
    U16 MR0_DLR:1;
    U16 DDR3_WR:3;
    U16 DDR3_PD:1;
    U16 res2:3;
}DDR3P_MR0;

typedef struct _DDR3P_MR1_
{
    U16 MR1_DLE:1;
    U16 DDR3_ODS_0:1;
    U16 DDR3_RTT_0:1;
    U16 DDR3_AL:1;
    U16 DDR3_ODS_1:1;
    U16 DDR3_RTT_1:1;
    U16 DDR3_WL:1;
    U16 res0:1;
    
    U16 DDR3_RTT_2:1;
    U16 res1:1;
    U16 DDR3_TDS:1;
    U16 MR1_QOF:1;
    U16 res2:4;
}DDR3P_MR1;

typedef struct _RDMR_REG_
{
    U8 RDMRODS:2;   
    U8 RDMRRTT:3;   
    U8 RDMRSRT:1;   
    U8 RDMRRTTWR:2;      
}RDMR_REG;

enum{
    DRAM_CE_NUM     = 1,
    DRAM_CE0        = 0,
    DRAM_CE1
};


/*------------------------------------------------------------------------------
   FUNCTION DECLARATION  
------------------------------------------------------------------------------*/
void HAL_DramcInit(void);

#endif 


