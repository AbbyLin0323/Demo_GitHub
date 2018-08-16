#include "COM_Inc.h"

#define rHostIntSrcReg  (*(volatile U32 *)(REG_BASE_HOSTC + 0x10))

LOCAL volatile MP_REG_SET *l_pMpRegSet;
LOCAL volatile HOSTC_INTMSK_REG * const l_pHostIntMskReg = (volatile HOSTC_INTMSK_REG *)(REG_BASE_HOSTC + 0x14);
GLOBAL volatile U32 l_ulRevMpInt;

void HAL_MpPcieCmdProc(void)
{
    l_pMpRegSet->bsCmdStatus = SUCCESS;
    switch(l_pMpRegSet->bsCmdType)
    {
        case MP_CMD_MEM_WR:
        {
            *(volatile U32*)(l_pMpRegSet->ulDstAddr) = l_pMpRegSet->ulDataIn;
        }break;

        case MP_CMD_MEM_RD:
        {
            l_pMpRegSet->ulDataOut = *(volatile U32*)(l_pMpRegSet->ulSrcAddr);
        }break;
        
        case MP_CMD_MEM_EXE:
        {     
            ((PFUNC)l_pMpRegSet->ulExeEntry)();
        }break;

        default:
        {
            ;
        }
    }
    l_pMpRegSet->bsCmdTrig = TRUE;
}


void HAL_HostCIntEntry(void)
{
    if (rHostIntSrcReg & BIT_HR_INT)
    {
        rHostIntSrcReg = BIT_HR_INT;
        rAHCI_GHC |= 1;
    }
    else if (rHostIntSrcReg & BIT_MP_INT)
    {
        rHostIntSrcReg = BIT_MP_INT;
        l_ulRevMpInt = TRUE;
    }
    else
    {
        ;
    }    
}

void HAL_MpForceGen1(void)
{
#ifndef FPGA
    // Cfg to Gen1
    // Enable RW
    *(volatile U32 *)(0x1ff837A4) |= (0x1<<28);
    // Set LKMAXLS
    *(volatile U32 *)(0x1ff836D0) &= ~(0xF);
    *(volatile U32 *)(0x1ff836D0) |= (0x1);
    // Set LKMLS
    *(volatile U32 *)(0x1ff836F0) &= ~(0xF);
    *(volatile U32 *)(0x1ff836F0) |= (0x2);
    // Set LKTGLS
    *(volatile U32 *)(0x1ff836F4) &= ~(0xF);
    *(volatile U32 *)(0x1ff836F4) |= (0x1);
    // Disable RW
    *(volatile U32 *)(0x1ff837A4) &= ~(0x1<<28);
#endif
}

void HAL_MpInitInt(void)
{
    rHostIntSrcReg = BIT_MP_INT;   // clear int
    rHostIntSrcReg = BIT_HR_INT;
    l_pHostIntMskReg->bsIntMGhcHrSet = FALSE;   
    l_pHostIntMskReg->bsIntMMptTrig  = FALSE;
    l_ulRevMpInt = FALSE;
    HAL_InitInterrupt(TOP_INTSRC_HOSTC,BIT_ORINT_HOSTC);    
}

void HAL_MpGlbSetting(void)
{
    rGLB(0x3a38) = 0;          // disable opt rom
    rGLB(0x18) |= 0x20000000;  //disable pcie 
    HAL_DelayCycle(2);         //wait for reset completed
    rGLB(0x18) &= ~0x20000000; // enable pcie [29]    
}

void HAL_MpModeSel(void)
{
    if (FALSE == HAL_StrapNfcIsNVMe())
    {
        l_pMpRegSet = (volatile MP_REG_SET *)REG_BASE_AHCI_MP;
    }
    else
    {
        l_pMpRegSet = (volatile MP_REG_SET *)REG_BASE_NVME_MP;
    }
}

void HAL_MpInit(void)
{   
    HAL_MpModeSel();
    HAL_MpInitInt();
    HAL_MpGlbSetting();
    HAL_MpForceGen1();
}

void HAL_MpPcieMain(void)
{
    U32 ulCurrRegValue,ulPreRegValue;
    DBG_TRACE(TRACE_PCIE_MP);
    HAL_MpInit();
    DBG_Printf("PCIE MP ENABLED.\n");

    ulPreRegValue = rP0_CMD;
    while(1)
    {
        ulCurrRegValue = rP0_CMD;
        if ( TEST_BIT( ( ulCurrRegValue ^ ulPreRegValue ), FRE_BIT ) )
        {
            if ( TEST_BIT( ulCurrRegValue, FRE_BIT) )
            {
                ulCurrRegValue |= FR_BIT;
            }
            else
            {
                ulCurrRegValue &= ~FR_BIT;
            }
            rP0_CMD = ulCurrRegValue;
            ulPreRegValue = ulCurrRegValue;
        }

        if (TRUE == l_ulRevMpInt)
        {
            l_ulRevMpInt = FALSE;
            HAL_MpPcieCmdProc();
        }
        
        HAL_UartDBG();
    }
}


