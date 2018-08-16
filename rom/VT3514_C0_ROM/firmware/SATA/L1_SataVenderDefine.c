/*************************************************
Copyright (C), 2009, VIA Tech. Co., Ltd.
Filename: L1_SataVenderDefine.c                                          
Version: 0.1                                                 
Date: 2009-08-31                                            
Author: JackeyChai

Description: SATA vender define commands support
Others:

Modification History:
JackeyChai,       2009-08-31,      first created
Blakezhang      2013-03-04,      blakezhang porting to C0 FW
*************************************************/
#include "HAL_Inc.h"
#include "L1_SataCmd.h"
#include "L1_Interface.h"
#include "L1_SataVenderDefine.h"
    
#ifdef SIM
#include <stdio.h>
#endif
    
#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
#endif

IDEREGSx sata_status;
U32 g_ulVenderDefineStatus;



/*Begin: add by henryluo 2010-12-23 for FAE tool */
BOOL DRAM_ATTR satap_vd_read_reg(void)
{

    U32  reg_addr;
    U32  reg_value;
    U32  sector_cnt;
    U32  dma_low;
    U32  dma_mid;
    U32  dma_high;

    /* get reg addr */
    sector_cnt = rSDC_SECCNT;
    dma_low = rSDC_LBALOW;
    dma_mid = rSDC_LBAMID;
    dma_high = rSDC_LBAHIGH;
    reg_addr = (dma_high << 24) | (dma_mid << 16) | (dma_low << 8) | sector_cnt;
    reg_value = *(U32 *)reg_addr;

    rSDC_SECCNT = (U8)(reg_value & 0xff);
    rSDC_LBALOW = (U8)((reg_value >> 8) & 0xff);
    rSDC_LBAMID = (U8)((reg_value >> 16) & 0xff);
    rSDC_LBAHIGH = (U8)((reg_value >> 24) & 0xff);
        
    /*return OK status*/
    while( FALSE == HAL_SataIsFISXferAvailable() );
    HAL_SataSendSuccessStatus();

    return TRUE;
}

BOOL DRAM_ATTR satap_vd_write_reg(void)
{
    U32  reg_addr;
    U32  reg_value;
    U32  sector_cnt;
    U32  dma_low;
    U32  dma_mid;
    U32  dma_high;
    U32  sector_cnt_exp;
    U32  dma_low_exp;
    U32  dma_mid_exp;
    U32  dma_high_exp;

    /* get reg addr */
    sector_cnt = rSDC_SECCNT;
    dma_low = rSDC_LBALOW;
    dma_mid = rSDC_LBAMID;
    dma_high = rSDC_LBAHIGH;
    reg_addr = (dma_high << 24) | (dma_mid << 16) | (dma_low << 8) | sector_cnt;

    /* get reg value */
    sector_cnt_exp = rSDC_EXP_SECCNT;
    dma_low_exp = rSDC_EXP_LBALOW;
    dma_mid_exp = rSDC_EXP_LBAMID;
    dma_high_exp = rSDC_EXP_LBAHIGH;
    reg_value = (dma_high_exp << 24) | (dma_mid_exp << 16) | (dma_low_exp << 8) | sector_cnt_exp;

    /* write the register */
    *(U32 *)reg_addr = reg_value;
        
    /*return OK status*/
    while( FALSE == HAL_SataIsFISXferAvailable() );
    HAL_SataSendSuccessStatus();

    return TRUE;
}

BOOL DRAM_ATTR satap_vd_firmware_run(void)
{
    U32  running_addr;
    U32  sector_cnt;
    U32  dma_low;
    U32  dma_mid;
    U32  dma_high;
    func_t ptr;

    /* get running addr */
    sector_cnt = rSDC_SECCNT;
    dma_low = rSDC_LBALOW;
    dma_mid = rSDC_LBAMID;
    dma_high = rSDC_LBAHIGH;
    running_addr = (dma_high << 24) | (dma_mid << 16) | (dma_low << 8) | sector_cnt;
    DBG_Printf("running_addr = 0x%x\n", running_addr);
    /*return OK status*/
    while( FALSE == HAL_SataIsFISXferAvailable() );
    HAL_SataSendSuccessStatus();
    
    /* jump to addr */
    ptr = (func_t)running_addr;
    (*ptr)();
    
    return TRUE;
}

BOOL DRAM_ATTR satap_vd_read_status(void)
{

    DBG_Printf("[satap_vd_read_status]\n");
    rSDC_SECCNT = sata_status.bSectorCountReg;
    rSDC_LBALOW = sata_status.bLBALowReg;
    rSDC_LBAMID = sata_status.bLBAMidReg;
    rSDC_LBAHIGH = sata_status.bLBAHighReg;
    rSDC_DEVICE_HEAD = sata_status.bDriveHeadReg;
    
    /*return OK status*/
    while( FALSE == HAL_SataIsFISXferAvailable() );
    HAL_SataSendSuccessStatus();
    return TRUE;
}
/*End: add by henryluo 2010-12-23 for FAE tool */


BOOL DRAM_ATTR L1_SataCmdVenderDefine(void)
{
    U8 vd_fea;
    
    vd_fea = rSDC_FEATURE_ERROR;
    DBG_Printf("[vencmd]: vencmd code = 0x%x. \n", vd_fea);
    switch( vd_fea )
    {
  /*      case VD_FEA_GET_DRAM_SIZE:
            satap_vd_get_dram_size();
            break;
        case VD_FEA_READ_DRAM_DMA:
            satap_vd_read_dram_dma();
            break;
        case VD_FEA_DISK_STATUS_CHK:
            satap_vd_disk_status_chk();
            break; */
        case VD_READ_REG:
            satap_vd_read_reg();
            break;
        case VD_WRITE_REG:
            satap_vd_write_reg();
            break;
 /*       case VD_WISHBONE_READ:
            satap_vd_wishbone_read();
            break;
        case VD_WISHBONE_WRITE:
            satap_vd_wishbone_write();
            break; */
        case VD_FIRMWARE_RUN:
            satap_vd_firmware_run();
            break;
        case VD_READ_STATUS:
            satap_vd_read_status();
            break;
        default:
            DBG_Printf("[vencmd]: invalid feature 0x%x.\n", vd_fea);
            while( FALSE == HAL_SataIsFISXferAvailable() );
            HAL_SataSendAbortStatus();
    }

    return TRUE;
}

/*End: add by henryluo 2010-12-17 for system enhancement tool */
