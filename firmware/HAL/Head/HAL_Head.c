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
Filename     :  COM_head.c                                    
Version      :  Ver 1.0                                               
Date         :                                           
Author         :    AddisonSu

Description: Firmware entry code. It sets up runtime environment
    for main firmware.

Modification History:
20120903    AddisonSu    001 first create
20140612    VictorZhang  002 Recreate for 3514 FW 1.060.0.0
*******************************************************************************/
#include "HAL_GLBReg.h"
#include "HAL_Head.h"
#include "HAL_DMAE.h"
#include "HAL_Memorymap.h"
#ifndef SIM
#include "HAL_PM.h"
#endif
#ifdef HOST_SATA
#include "HAL_SataDSG.h"
#endif

LOCAL const HEAD_MAP_TABLE l_aMapTable[] = 
{
    {MCU0_DSRAM0_BASE + STACK_SIZE, DRAM_DSRAM0_MCU0_BASE + STACK_SIZE, MCU0_DSRAM0_SIZE - STACK_SIZE, MCU0_ID},
    {MCU1_DSRAM0_BASE + STACK_SIZE, DRAM_DSRAM0_MCU1_BASE + STACK_SIZE, MCU1_DSRAM0_SIZE - STACK_SIZE, MCU1_ID},
    {MCU2_DSRAM0_BASE + STACK_SIZE, DRAM_DSRAM0_MCU2_BASE + STACK_SIZE, MCU2_DSRAM0_SIZE - STACK_SIZE, MCU2_ID},
    {MCU0_ISRAM_BASE,               DRAM_ISRAM_MCU0_BASE,               MCU0_ISRAM_SIZE, MCU0_ID},
    {MCU1_ISRAM_BASE,               DRAM_ISRAM_MCU1_BASE,               MCU1_ISRAM_SIZE, MCU1_ID},
    {MCU2_ISRAM_BASE,               DRAM_ISRAM_MCU2_BASE,               MCU2_ISRAM_SIZE, MCU2_ID}    
};

LOCAL const HEAD_MAP_TABLE l_aMapTableEx[] = 
{
    {OTFB_START_ADDRESS ,DRAM_OTFB_SUSPEND_BASE     ,OTFB_SUSPEND_SIZE  ,MCU0_ID},
    {MCU012_DSRAM1_BASE ,DRAM_DSRAM1_SUSPEND_BASE   ,MCU012_DSRAM1_SIZE ,MCU0_ID}
};

/********************************************************************************
FUNC :  HEAD_ChkWarmBootEntry
INPUT:  None 
OUTPUT: Whether warm boot entry address saved in PMU scratch register is zero.
Description :
    Reading out the warm boot entry saved in PMU scratch register. It can indicate whether system
    is resumed from Suspending to DRAM state.
********************************************************************************/
INLINE U32 HEAD_ChkWarmBootEntry(void)
{
#ifndef SIM
    const volatile PMUREGSET *pPMURegBlk =
            (const volatile PMUREGSET *)REG_BASE_PMU;

    U32 ulWarmStartFlag;

    if (0 == pPMURegBlk->ulResumeEntry)
    {
      ulWarmStartFlag = FALSE;
    }

    else
    {
      ulWarmStartFlag = TRUE;
    }

    return ulWarmStartFlag;
#else
    return FALSE;
#endif
}

INLINE LOCAL void HEAD_ReconfigGLB(void)
{
    rGLB(0x40) |= (1 << 27);
    rGLB(0x64) = (rGLB(0x64) & ~(1 << 26)) | (1 << 25);
    rGLB(0x68) = (rGLB(0x68) & ~(1 << 29)) | (1 << 28);
    rGLB(0x3C) = 4;
    rGLB(0xA0) |= (1 << 18);

    return;
}

INLINE void HEAD_RelocationSramToDram(BOOL bToSleep)
{
    U32 i;
    for (i = 0; i < HEAD_SRAM_SEG_NUM; i++)
    {
        HAL_DMAESramCopyOneBlock(l_aMapTable[i].ulDramAddr,
                                 l_aMapTable[i].ulSramAddr,
                                 l_aMapTable[i].ulSize,
                                 l_aMapTable[i].ulMcuId);
    }

    if (TRUE == bToSleep)
    {
        for (i = 0; i < HEAD_OTFB_SEG_NUM; i++)
        {
            HAL_DMAESramCopyOneBlock(l_aMapTableEx[i].ulDramAddr,
                                     l_aMapTableEx[i].ulSramAddr,
                                     l_aMapTableEx[i].ulSize,
                                     l_aMapTableEx[i].ulMcuId);
        }    
    }

    return;
}

/* DW align memmove */
INLINE void HEAD_Simple_memmove(char *des, char *src, U32 count)
{
    U32 count_dw = count/4;
    U32 count_org = count_dw;
    int *p_des = (int *)des;
    int *p_src = (int *)src;

    if(p_des == p_src)
        return;

    while(count_dw>0)
    {
        
        if(p_des>p_src)
        {
            count_dw--;
            *(p_des+count_dw) = *(p_src+count_dw);
        }
        else if(p_des<p_src)
        {
            *(p_des+count_org-count_dw) = *(p_src+count_org-count_dw);
            count_dw--;
        }
    }
}

/* DW align memset */
INLINE void HEAD_Simple_memset(char *addr, char value, U32 count)
{
    U32 count_dw = count/4;
    int *p_src = (int *)addr;
    while(count_dw>0)
    {
        *(p_src+count_dw) = value;
        count_dw--;
    }
}

MCU0_HEAD_ATTR U32 HEAD_RelocationSectionToRightDram(U32 uOffset, U32 uMCU)
{
     char * input_buf = NULL;
     input_buf =  (char*)(0x40400000+uOffset);   

    //char * output_buf =  (char*)(DRAM_HEAD_MCU0_BASE);
    U32 fw_total_len_location = FW_VERSION_OFFSET+VERSION_TOTAL_LEN_DW*4;
    FW_FILE_HDR *p_fw_file_hdr = (FW_FILE_HDR *)(0x40400000+fw_total_len_location)+uMCU;
    FW_SEC_DES_ELE_META *p_fw_sec_des_ele_meta = (FW_SEC_DES_ELE_META *)(input_buf + p_fw_file_hdr->m_bin_total_len -
                                                                         sizeof(FW_SEC_DES_ELE_META));
    U32 file_len = p_fw_file_hdr->m_bin_total_len;
    U32 data_len = p_fw_file_hdr->m_sec_des_off;
    U32 seg_num = (file_len-data_len)/sizeof(FW_SEC_DES_ELE_META);
    U32 seg_num_bak = seg_num;
    U32 zero_len = 0;

    // copy the compated fw bin to another location for saving
    //HEAD_Simple_memmove((char *)DRAM_DATA_BUFF_MCU1_BASE, input_buf, file_len);

    do{
        seg_num--;

        if(p_fw_sec_des_ele_meta->m_sec_phy_addr_off != p_fw_sec_des_ele_meta->m_sec_offset)
        {
            HEAD_Simple_memmove((char *)(p_fw_sec_des_ele_meta->m_sec_phy_addr_off),
                input_buf+p_fw_sec_des_ele_meta->m_sec_offset,
                p_fw_sec_des_ele_meta->m_sec_len);
        }

         p_fw_sec_des_ele_meta -= 1;
    }while(seg_num>0);


    return file_len+uOffset;
}

INLINE void HEAD_RelocationDramToSram(U32 ulWarmBoot)
{
    U32 i;
    for (i = 0; i < HEAD_SRAM_SEG_NUM; i++)
    {
        HAL_DMAESramCopyOneBlock(l_aMapTable[i].ulSramAddr,
                                 l_aMapTable[i].ulDramAddr,
                                 l_aMapTable[i].ulSize,
                                 l_aMapTable[i].ulMcuId);
    }

    if (TRUE == ulWarmBoot)
    {
        for (i = 0;i < HEAD_OTFB_SEG_NUM; i++)
        {
            HAL_DMAESramCopyOneBlock(l_aMapTableEx[i].ulSramAddr,
                                     l_aMapTableEx[i].ulDramAddr,                                        
                                     l_aMapTableEx[i].ulSize,
                                     l_aMapTableEx[i].ulMcuId);
        }    
    }

    return;
}

MCU0_DRAM_TEXT void HEAD_StallMcu12(U32 bStall)
{
    if (FALSE == bStall)
    {
        rGlbMCUCtrl &= ~(RMCU1_EXEC_STALL | RMCU2_EXEC_STALL);
    }
    else
    {
        rGlbMCUCtrl |= (RMCU1_EXEC_STALL | RMCU2_EXEC_STALL);
    }

    return;
}

MCU0_DRAM_TEXT void HEAD_RstMcu12(U32 bRst)
{
    if (FALSE == bRst)
    {
        rGlbMcuSgeRst &= ~(R_RST_MCU1 | R_RST_MCU1IF | R_RST_MCU2 | R_RST_MCU2IF);
    }
    else
    {
        rGlbMcuSgeRst |= (R_RST_MCU1 | R_RST_MCU1IF | R_RST_MCU2 | R_RST_MCU2IF);
    }

    return;
}

INLINE void HEAD_AlterMcu12StatVec(U32 bAlt)
{
    if (FALSE == bAlt)
    {
        rGlbMCUCtrl &= ~(RMCU1_USE_ALTRESETVECT | RMCU2_USE_ALTRESETVECT);
    }
    else
    {
        rGlbMCUCtrl |= (RMCU1_USE_ALTRESETVECT | RMCU2_USE_ALTRESETVECT);
    }

    return;
}

/*********************************************************************************
 *FUNC : HEAD_Main
 *Description:
 *     Relocate DSRAM/ISRAM of MCU12
 *     Relocate vector base of MCU0
 *     Call cmain of MCU0
 * *******************************************************************************/
MCU0_DRAM_TEXT void HEAD_Main(void)
{
#ifndef SIM
    U32 ulWarmBootFlag;
    U32 uOffset = 0;

    ulWarmBootFlag = HEAD_ChkWarmBootEntry();

    if (TRUE == ulWarmBootFlag)
    {
        HEAD_ReconfigGLB();
        uart_init();
    }

    HAL_DMAEInit();
    HEAD_StallMcu12(TRUE);          //  stall mcu 1 & 2 
    HEAD_RstMcu12(FALSE);           //  start mcu 1 & 2 and mcu 1 & 2 do not fetch cmd 
    HEAD_RelocationDramToSram(ulWarmBootFlag);    //  relocate data from dram to sram 
    HEAD_AlterMcu12StatVec(TRUE);
    HEAD_RstMcu12(TRUE);            //  reset mcu 1 & 2
    HEAD_StallMcu12(FALSE);         //  release mcu 1 & 2
    HAL_WSRVecBase(MCU0_ISRAM_BASE);
    _clear_bss();                   // clear .bss and then jump to c main()

#endif
}

MCU0_HEAD_ATTR void HEAD_FirstRelocateEntry(void)
{
    U32 ulOffset;
    ulOffset = HEAD_RelocationSectionToRightDram(0, 0);
    ulOffset = HEAD_RelocationSectionToRightDram(ulOffset,1);
    ulOffset = HEAD_RelocationSectionToRightDram(ulOffset,2);

    return;
}

/********************** FILE END ***************/

