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
Filename    : HAL_Xtensa.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.02
Description : this file encapsulate assemble interface to Tensilica MCU core 
Others      : some of functions in this file are not available in Windows ENV.
Modify      :
20140902    Gavin     Create file
20140903    Gavin     add cycle count / cache / exception handling
20141028    Gavin     fix spelling mistake. add "INLINE" attribute
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"

#ifndef SIM
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_timer.h>
#include <xtensa/tie/via.h>
extern U32 _bss_end;//the variable '_bss_end' is defined by Xtensa Linker
#else
#include "xt_library.h"
#endif

/*------------------------------------------------------------------------------
Name: HAL_GetMcuId
Description: 
    get MCU id from MCU internal register PRID.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: MCU id. In VT3514, MCU0's id = 1, MCU1's id = 2, MCU2's id = 3
Usage:
    FW call this function if it wants MCU id to do some judgment;
History:
    20140902    Gavin   moved from HAL_MultiCore.h
    20141028    Gavin   change attribute to "INLINE"
------------------------------------------------------------------------------*/
INLINE U32 HAL_GetMcuId(void)
{
    U32 ulMcuId;
    ulMcuId = XT_RSR_PRID();

    return ulMcuId;
}

/*------------------------------------------------------------------------------
Name: HAL_DisableMCUIntAck
Description: 
    disable MCU acknowledge interrupt by setting interrupt level to the highest.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to disable MCU acknowledge interrupt.
History:
    20140902    Gavin   moved from HAL_Interrupt.c
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_DisableMCUIntAck(void)
{
#ifndef SIM
    XT_RSIL(15);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EnableMCUIntAck
Description: 
    Enable MCU acknowledge interrupt by setting interrupt level to the lowest.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to enable MCU acknowledge interrupt.
History:
    20140902    Gavin   moved from HAL_Interrupt.c
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_EnableMCUIntAck(void)
{
#ifndef SIM
    XT_RSIL(0);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_DisableMCUIntSrc
Description: 
    Disable interrupt source which are specified by bitmap in argument.
Input Param:
    U32 ulDisableMap: each bit in this DW represent a interrupt source
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to disable some interrupt source.
History:
    20140902    Gavin   moved from HAL_Interrupt.c
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_DisableMCUIntSrc(U32 ulDisableMap)
{
#ifndef SIM
    U32 ulNewIntEn = (XT_RSR_INTENABLE() & (~ulDisableMap));

    XT_WSR_INTENABLE(ulNewIntEn);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EnableMCUIntSrc
Description: 
    Enable interrupt source which are specified by bitmap in argument.
Input Param:
    U32 ulEnableMap: each bit in this DW represent a interrupt source
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to enable some interrupt source.
History:
    20140902    Gavin   moved from HAL_Interrupt.c
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_EnableMCUIntSrc(U32 ulEnableMap)
{
#ifndef SIM
    U32 ulNewIntEn = (XT_RSR_INTENABLE() | ulEnableMap);

    XT_WSR_INTENABLE(ulNewIntEn);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_MCUWaitForInt
Description: 
    halt MCU until interrupt come to it.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    If FW want to halt MCU to save power, this function can be called.
History:
    20140903    Gavin   create
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_MCUWaitForInt(void)
{
#ifndef SIM
    XT_WAITI(0);
#else
    BOOL bHaltMcu = TRUE;

    while (TRUE == bHaltMcu)
    {
        ;//make MCU in dead loop for Windows ENV.
    }
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetMCUIntSrc
Description: 
    get current interrupt source of MCU.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: interrupt source bitmap
Usage:
    In ISR, FW call this function to get current interrupt source of MCU.
History:
    20140903    Gavin   created
    20141028    Gavin   fix bug. we should call "XT_RSR_INTERRUPT"
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE U32 HAL_GetMCUIntSrc(void)
{
#ifndef SIM
    return XT_RSR_INTERRUPT();
#else
    return 0;//in windows ENV, EXCCAUSE is not available, we just return zero.
#endif
}

/*------------------------------------------------------------------------------
Name: HAL_GetMCUExcCause
Description: 
    get current exception cause of MCU.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: exception cause
Usage:
    In exception vector, FW call this function to get exception cause of MCU.
History:
    20140903    Gavin   created
    20141028    Gavin   fix bug. we should call "XT_RSR_EXCCAUSE"
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE U32 HAL_GetMCUExcCause(void)
{
#ifndef SIM
    return XT_RSR_EXCCAUSE();
#else
    return 0;//in windows ENV, INTERRUPT is not available, we just return zero.
#endif
}

/*------------------------------------------------------------------------------
Name: HAL_SaveMCUExcCause
Description: 
    Just records exception cause and source instruction address at the start of
    OTFB and hangs..
Input Param:
    U32 ulExcCause: exception cause value
Output Param:
    none
Return Value:
    none
Usage:
    When unpredictable exception happens, call this function to save exception
    cause for debug.
History:
    20140903    Gavin   created
    20141028    Gavin   fix spelling mistake
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_SaveMCUExcCause(U32 ulExcCause)
{
#ifndef SIM
    U32 ulEPC1;

    asm ( "rsr.epc1 %0\n\t \
          s32i %2, %1, 0\n\t \
          s32i %0, %1, 4\n\t \
          waiti 4\n\t"
          : "+a"(ulEPC1) : "a"(OTFB_START_ADDRESS), "a"(ulExcCause) : "memory" );
#else
    //for Windows ENV, we can not get here
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_MemoryWait
Description: 
    wait memory access finsh.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    For some FW/HW interaction, FW need to wait previous access to HW/memory finish
    before any other operation. In that case, this function can be called.    
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_MemoryWait(void)
{
#ifndef SIM
    XT_MEMW();
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EnableICache
Description: 
    enable MCU instruction cache.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to enable MCU I-cache.    
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_EnableICache(U32 ulMcuId)
{
    U32 ulBuf = 0;
 
    /* Icache open */
    if (MCU0_ID == ulMcuId)
    {
       ulBuf |= 1 << 16; // wire RMCU0_RICache_EN = REGGLB_40[16];
    }
    else if (MCU1_ID == ulMcuId)
    {
       ulBuf |= 1 << 18; // wire RMCU1_RICache_EN = REGGLB_40[18];
    }
    else //MCU2_ID
    {
       ulBuf |= 1 << 20; // wire RMCU2_RICache_EN = REGGLB_40[20];  
    } 
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_GLB40);
    rGlbMCUMisc |= ulBuf;
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_GLB40);
#ifndef SIM
   xthal_icache_all_unlock();
   xthal_icache_all_invalidate();
   
   /* The 4GB address space is divided into 8*512MB, and cache attribute for each
    512MB is assigned by a 4-bit value.
    1 : cached, write thru
    2 : bypass cache
    4 : cached, write back
    */
   xthal_set_icacheattr(0x22221122);
   xthal_icache_sync();
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EnableDCache
Description: 
    enable MCU data cache.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to enable MCU D-cache.    
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_EnableDCache(U32 ulMcuId)
{
    U32 ulBuf = 0;
    
    /* Dcache open */                                                           
    if (MCU0_ID == ulMcuId)
    {
        ulBuf |= 1 << 17; // wire RMCU0_RDCache_EN = REGGLB_40[17]; 
    }
    else if (MCU1_ID == ulMcuId)
    {
        ulBuf |= 1 << 19; // wire RMCU1_RDCache_EN = REGGLB_40[19];  
    }
    else //MCU2_ID
    {
        ulBuf |= 1 << 21; // wire RMCU2_RDCache_EN = REGGLB_40[21];
    }
    ulBuf |= 1 << 24;
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_GLB40);
    rGlbMCUMisc |= ulBuf;
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_GLB40);
#ifndef SIM
    xthal_dcache_all_unlock();
    xthal_dcache_all_invalidate();

    /* The 4GB address space is divided into 8*512MB, and cache attribute for each
    512MB is assigned by a 4-bit value.
    1 : cached, write thru
    2 : bypass cache
    4 : cached, write back
    */
    xthal_set_dcacheattr(0x22112222);
    xthal_dcache_sync();
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_InvalidateDCache
Description: 
    invalidate MCU data cache.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to invalidate MCU D-cache for FW/HW interaction.    
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_InvalidateDCache(void)
{
#ifndef SIM
    xthal_dcache_all_invalidate();   
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetMCUCycleCount
Description: 
    get current cycle count of MCU.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: current cycle count
Usage:
    FW call this function to get current cycle count of MCU.
History:
    20140902    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE U32 HAL_GetMCUCycleCount(void)
{
#ifndef SIM
    return XT_RSR_CCOUNT();
#else
    return 0;//in windows ENV, CCOUNT is not available, we just return zero.
#endif
}

/*------------------------------------------------------------------------------
Name: HAL_DelayCycle
Description: 
    delay by cycle count.
Input Param:
    U32 ulCycleCnt: cycle count to delay
Output Param:
    none
Return Value:
    none
Usage:
    Anywhere need to insert delay, this function can be called.
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
    20141112    Gavin   change XT_NOP() to asm("nop") for compiler optimization issue
------------------------------------------------------------------------------*/
INLINE void HAL_DelayCycle(U32 ulCycleCnt)
{
    while (ulCycleCnt--)
    {
        #ifndef SIM
            asm("nop"); // not use XT_NOP() to prevent compiler optimizing it
        #endif
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_WSRVecBase
Description: 
    Write special register VECBASE.
Input Param:
    U32 ulNewVecBase: new VECBASE
Output Param:
    none
Return Value:
    none
Usage:
    when FW want to change VECBASE. support Xtensa ENV only
History:
    20140903    Gavin   created
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE void HAL_WSRVecBase(U32 ulNewVecBase)
{
#ifndef SIM
    XT_WSR_VECBASE(ulNewVecBase);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetBssSegEndAddr
Description: 
    get .bss segment ending address.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: the end address of .bss segment.
  NOTE:
    In Windows ENV, the return value only indicates base address of some free memory.
Usage:
    In Xtensa run-time, if FW wants to use free memory following .bss segment, it should
    call this function to get the starting address of available space.
History:
    20150303    Gavin   created
------------------------------------------------------------------------------*/
INLINE U32 HAL_GetBssSegEndAddr(void)
{
#ifndef SIM
    return (U32)&(_bss_end);
#else
    U32 ulMcuId = HAL_GetMcuId();

    return (SIM_BSS_BASE + (ulMcuId - MCU0_ID) * SIM_PER_MCU_BSS_SIZE);
#endif
}

#ifdef SIM
LOCAL MCU12_VAR_ATTR U32 l_SCLZ_Data;

/*------------------------------------------------------------------------------
Name: CLZ
Description: 
    count leading zeros in a DWord.
Input Param:
    U32 ulDw: the dword in which need to count leading zeros
Output Param:
    none
Return Value:
    U8: number of leading zeros.
Usage:
    This function is available on windows platform only.
    In Xtensa ENV, CLZ is a instruction provided by Tensilica ISA
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U8 CLZ(U32 ulDw)
{
    U8 i;
    if (0 == ulDw)
    {
        i = 32;
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            if (0 != ((1 << (31-i)) & ulDw))
            {
                break;
            }
        }
    }
    
    return i;
}

/****************************************************************************
Name        :POPCOUNT
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    count the number of "bit 1" in a input U32
    example: 
    input 0x0             output 0
    input 0xFFFFFFFF  output 32
    input 0xF0F0F0F0  output 16

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
LOCAL U8 POPCOUNT(U32 ulDw)
{
    U8 i;
    U8 ucCount;

    ucCount = 0;

    for (i = 0; i < 32; i++)
    {
        if (0 != ((1 << (31-i)) & ulDw))
        {
            ucCount++;
        }
    }

    return ucCount;
}

/****************************************************************************
Name        :Wclzstate
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    register a Mask for SCLZ()

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
LOCAL void Wclzstate(U32 ulDw)
{
    l_SCLZ_Data = ulDw;
    return;
}

/****************************************************************************
Name        :SCLZ
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    output each "bit 1" offset in a registered Mask 
    by Wclzstate() for each call.
    From the highest bit to lowest bit, if all zero, return 32
    example:

    1, first call Wclzstate(0x87654321)
    2, call SCLZ() each times output:
       0 -> 5 -> 6 -> 7 -> 9 -> 10 -> 
       13 -> 15 -> 17 -> 22 -> 23 -> 
       26 -> 31 -> 32 -> 32 -> 32 ....

    3, call Wclzstate() again will reset SCLZ()

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
LOCAL U8 SCLZ(void)
{
    U8 ucRet;
  
    ucRet = CLZ(l_SCLZ_Data);

    if (32 != ucRet)
    {
        l_SCLZ_Data &= ~(1<<(31 - ucRet));
    }

    return ucRet;
}
#endif

/*------------------------------------------------------------------------------
Name: HAL_CLZ
Description: 
    count leading zeros in a DWord.
Input Param:
    U32 ulDw: the dword in which need to count leading zeros
Output Param:
    none
Return Value:
    U8: number of leading zeros.
Usage:
    the function support all ENVs
History:
    20141028    Gavin   add description. the original author is victorzhang
    20141028    Gavin   add "INLINE" attribute
------------------------------------------------------------------------------*/
INLINE U8 HAL_CLZ(U32 ulDw)
{
    return CLZ(ulDw);
}

/****************************************************************************
Name        :HAL_POPCOUNT
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    count the number of "bit 1" in a input U32
    example: 
    input 0x0             output 0
    input 0xFFFFFFFF  output 32
    input 0xF0F0F0F0  output 16

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
INLINE U8 HAL_POPCOUNT(U32 ulDw)
{
    return POPCOUNT(ulDw);
}

/****************************************************************************
Name        :HAL_Wclzstate
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    register a Mask for SCLZ()

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
INLINE void HAL_Wclzstate(U32 ulDw)
{
    Wclzstate(ulDw);
    return;
}

/****************************************************************************
Name        :HAL_SCLZ
Input       :
Output      :
Author      :
Date        :
Description :
    XTMP assemble instruction:
    output each "bit 1" offset in a registered Mask 
    by Wclzstate() for each call.
    From the highest bit to lowest bit, if all zero, return 32
    example:

    1, first call Wclzstate(0x87654321)
    2, call SCLZ() each times output:
       0 -> 5 -> 6 -> 7 -> 9 -> 10 -> 
       13 -> 15 -> 17 -> 22 -> 23 -> 
       26 -> 31 -> 32 -> 32 -> 32 ....

    3, call Wclzstate() again will reset SCLZ()

Others      :
Modify      :  2014/5/31   BlakeZhang   first Created
****************************************************************************/
INLINE U8 HAL_SCLZ(void)
{
    return SCLZ();
}

/* end of file HAL_Xtensa.c */

