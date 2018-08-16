/*************************************************************************


*************************************************************************/
#include "BaseDef.h"
#include "Proj_Config.h"
#include "HAL_PIN.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"

typedef BOOL (*PFUNC_NORMAL_BOOT)(U8);

#define ROM_FUNC_TABLE __attribute__((section("rom_func_table")))

extern BOOL HAL_NfcResetPu(U8 ulPu);
extern BOOL HAL_NfcReadId(U8 ulPu);
extern BOOL HAL_NfcReadBootloader(U8 ulPu);
extern U32 HAL_NfcGetBootLoaderEntry();
extern void HAL_MpPcieMain(void);
extern void HAL_MpSataMain(void);
GLOBAL U32 g_ulDbgEnable;

PFUNC_NORMAL_BOOT const l_pNFBoot[] = {
    HAL_NfcResetPu,
    HAL_NfcReadId,
    HAL_NfcReadBootloader
};

enum {
    FUNC_INDEX_RESET_PU = 0,
    FUNC_INDEX_READ_ID,
    FUNC_INDEX_READ_FLASH,
    FUNC_INDEX_NUM
};

char const BootString[FUNC_INDEX_NUM][16] =
{
    "RESET PU",
    "READ ID",
    "READ FLASH",
};

LOCAL U8 l_ucCeMax;
PFUNC l_pBootEntry;
LOCAL U32 l_ulVaildPuBM[2];

/*******************************************************************************
FUNC : HAL_ActivateDRAM
INPUT:
OUTPUT:
DESCRIPTION:
    In warm boot processing , ROM program will jump to the vector located in DRAM,
    while the DRAM may be in the self-refresh mode, and unvailable to be read.
    So ROM should try to write the DRAM firstly to make sure the DRAM resume into 
    the normal status.
History:
    2014/10/28  Victor Zhang .Created

*******************************************************************************/
void HAL_ActivateDRAM(void)
{
    U32 ulAddr = DRAM_START_ADDRESS;
    U32 i;
//#if 1  //ndef COSIM    
    for (i=0;i<4;i++)
    {
        *(U32*)(ulAddr + i*4) = 0;
    }
//#endif    
}
/************************************************
Function Name: HAL_SetEPHY()
Input:
Output:
Description:
    set ephy default value if under SATA ,
    do nothing if under PCIE

*************************************************/

void HAL_SetEPHY(void)
{
#ifndef FPGA
#ifndef VT3514_C0
    // set sata ephy
    if(FALSE == HAL_StrapIsPcie())
    {
        rGLB(0x3c60) = 0x34000     ;
        rGLB(0x3c64) = 0xe60015    ;
        rGLB(0x3c68) = 0x157001b   ;
        rGLB(0x3c14) = 0x0f80e808  ;
        rGLB(0x3c20) = 0x0c6c0000  ;
        rGLB(0x3c24) = 0x0         ;
        rGLB(0x3c2c) = 0xaf50faaa  ;
        rGLB(0x3c34) = 0x0c6c0000  ;
        rGLB(0x3c38) = 0x0         ;
        rGLB(0x3c40) = 0xa550faaa  ;
        rGLB(0x3c48) = 0x0c6c0000  ;
        rGLB(0x3c4c) = 0x092       ;
        rGLB(0x3c54) = 0xa050ffff  ;
    }
#else
    if(FALSE == HAL_StrapIsPcie())
    {
        rGLB(0x3c10) = 0x00000000  ;

        rGLB(0x3c2c) = 0xfa50c5fa  ;
        rGLB(0x3c40) = 0xf550c5fa  ;
        rGLB(0x3c54) = 0xf550c5fa  ;

        rGLB(0x3c28) = 0x22441144  ;
        rGLB(0x3c3c) = 0x22441144  ;
        rGLB(0x3c50) = 0x22441144  ;

        rGLB(0x3c14) = 0x07802000  ;
        rGLB(0x3c60) = 0x00850000  ;
        rGLB(0x3c64) = 0x00850023  ;
        rGLB(0x3c68) = 0x0085003f  ;
        rGLB(0x3c20) = 0x06770000  ;
        rGLB(0x3c24) = 0x00000000  ;
        rGLB(0x3c34) = 0x06770000  ;
        rGLB(0x3c38) = 0x00000000  ;
        rGLB(0x3c48) = 0x07F80000  ;
        rGLB(0x3c4c) = 0x000002D0  ;
    }
#endif
#endif 
}

/*******************************************************************************
FUNC : HAL_RomInit
INPUT:
OUTPUT:
DESCRIPTION:
    Main roution of the ROM boot program.
    1. remap entire SRAM back to OTFB
    2. fetch strap pin info 
    3. init uart if enable 
    4. enable trace buffer 
    5. set SATA ephy default value
    6. double CE number if under 16 ce mode  
History:
    2014/10/28  Victor Zhang .Created

*******************************************************************************/
void HAL_RomInit(void)
{
	g_ulDbgEnable = 'V';
    l_ulVaildPuBM[0] = 0;
    l_ulVaildPuBM[1] = 0;
    rGLB(0x24) = 0x2f; // enable entire OTFB
    HAL_StrapInit();

#ifdef FPGA
    // FPGA only , PCIE reset en 
    if (TRUE == HAL_StrapIsPcie())
    {
        rGLB(0x1f1c) |= (1<<24);
    }
#endif 

    uart_init();           
    DBG_Printf("VT3514 C0 ROM BOOT PROCESSING ...\n");
    DBG_Printf("UART enabled\n");
    
#ifdef VT3514_C0
    HAL_Trax3CoreConfig(MCU0_ID);
#endif
    l_ucCeMax = (TRUE == HAL_StrapNfcIs16CeMode())?CE_MAX*2:CE_MAX;
}
/************************************************
Function Name: HAL_ScanPu()
Input: 
Output:    
Description:
    scan all pu id for MP , MP thread may evoke it through function pointer
*************************************************/

void HAL_ScanPu(void)
{
    U32 i;
    for (i=0;i<l_ucCeMax;i++)
    {
        HAL_NfcResetPu(i);
        HAL_NfcReadId(i);
    }
}


/************************************************
Function Name: HAL_HwInit
Input: 
Output:    
Description:
    1. check efuse be valid and set HW if valid
    2. Init NFC module
*************************************************/

void HAL_HwInit(void)
{
    HAL_StrapCheckEfuse();    
    
}

/************************************************
Function Name: TEST_FuncReturn
Input: 
Output:    
Description:
    FOR test
*************************************************/

void TEST_FuncReturn(void)
{
    DBG_Printf("TEST FUNCTION RETURE \n");
    return;
}

void HAL_Jump(void)
{
    l_pBootEntry();
}

BOOL HAL_NfcGetFlashValid(U32 ulPu)
{
    BOOL bSts = 0;
    if(ulPu<32)
    {
        bSts = ( l_ulVaildPuBM[0] |(1<<ulPu)) ? TRUE:FALSE;
    }
    else
    {
        bSts =  (l_ulVaildPuBM[1] | (1<<(ulPu-32))) ? TRUE:FALSE;
    }
    return bSts;
}

void HAL_NfcSetFlashValid(U32 ulPu)
{
    if(ulPu<32)
    {
        l_ulVaildPuBM[0] |= (1<<ulPu);
    }
    else
    {
        l_ulVaildPuBM[1] |= (1<<(ulPu-32));
    }    
}

/************************************************
Function Name: HAL_FlashBoot
Input: 
Output:    
Description:
    flash boot
        <1> reset pu -> read id -> read flash -(success)-> <2>
                                              -(failed)--> <3>
        <2> start pcie/sata if required then execute bootloader 
        <3> accumulate pu then try <1> 
*************************************************/

void HAL_FlashBoot(void)
{
    U32 ulPu,ulFuncId,ulStatus,ulBootFuncNum;
    U32 ulVaildPuBM[2],ulBmId ;
    U32 *pBM;

    BOOL bToggleSync;

    bToggleSync = (FALSE == HAL_StrapNfcIsOnfi())&&(FALSE == HAL_StrapNfcIsAsync());
    
    HAL_NfcInit();
   /* 
    if((FALSE == HAL_StrapNfcIsOnfi())&&(FALSE == HAL_StrapNfcIsAsync()))
    {
        for (ulPu=0;ulPu<l_ucCeMax;ulPu++)
        {
            HAL_NfcResetPu(ulPu);
        } 
        HAL_DelayCycle(400000);
    }
    
    HalNfcTimingInit();

    for (ulPu=0;ulPu<l_ucCeMax;ulPu++)
    {
        for (ulFuncId=0;ulFuncId<FUNC_INDEX_NUM;ulFuncId++)
        {
            DBG_TRACE(TRACE_NFC_PRO(ulFuncId) + ulPu);
            ulStatus = l_pNFBoot[ulFuncId](ulPu);
            if (FAIL == ulStatus)
            {
                DBG_Printf("PU %d %s failed\n",ulPu,BootString[ulFuncId]);
                break;
            }
        }

        if (SUCCESS == ulStatus)
        {
            DBG_TRACE(TRACE_SUCCESS);
            l_pBootEntry = (PFUNC)HAL_NfcGetBootLoaderEntry();
            HAL_StrapResetHostIF();
            DBG_Printf("Flash Boot successfully , boot entry is 0x%x\n",l_pBootEntry);
            break;
        }
    }
    */

   DBG_Printf("Enter Flash boot.\n");

    if(FALSE == bToggleSync)
    {
        HalNfcTimingInit();
    }
    else// bToggleSync == TRUE,  TSB Toggle only chip
    {
    #if 0
        //set Async timing 
		rGLB(0x54)=0x0;
        rGLB(0x1000)=0x60f8ca2e;   
        rGLB(0x1004)=0x814c8125;
        rGLB(0x1008)=0x00102494;     
        rGLB(0x102c)=0x0;
        rGLB(0x1030)=0x30000055;  
        //rGLB(0x1038)=0x0;     
        rGLB(0x1080)=0x0;
        //rGLB(0x1088)=0xd0508900;        
        rGLB(0x1f20)=0x50;
        rGLB(0x1078)=0x0;
        rGLB(0x107c)=0x0;
    #endif

        rGLB(0x54)=0x0;
        rGLB(0x1000)=0x60f8ca2e;   
        rGLB(0x1004)=0x814c8125;
        rGLB(0x1008)=0x00102494;     
        rGLB(0x102c)=0x0;
        rGLB(0x1030)=0x30000055;  
        rGLB(0x1080)=0x0;
        rGLB(0x1f20)=0x50;
        rGLB(0x1078)=0x0;
        rGLB(0x107c)=0x0;
    }
    

   
    for (ulPu=0;ulPu<l_ucCeMax;ulPu++)
    {
        HAL_NfcResetPu(ulPu);
    } 
    //HAL_DelayCycle(400000);
    DBG_Printf("Rom boot all flash reset finish\n");

    for (ulPu=0;ulPu<l_ucCeMax;ulPu++)
    {
        if(TRUE == HAL_NfcReadId(ulPu))
        {
            HAL_NfcSetFlashValid(ulPu);
        }
    } 

    if(TRUE == bToggleSync)
    {
        HalNfcTimingInit();
    }

    for (ulPu=0;ulPu<l_ucCeMax;ulPu++)
    {
        if(TRUE == HAL_NfcGetFlashValid(ulPu))
        {
            if(TRUE == HAL_NfcReadBootloader(ulPu))
            {
                l_pBootEntry = (PFUNC)HAL_NfcGetBootLoaderEntry();
                HAL_StrapResetHostIF();
                break;
            }
        }
    }
}

/************************************************
Function Name: HAL_WarmBoot
Input: 
Output:    
Description:
    check if wram boot or not 
            <1> get warm boot entry 
            <2> activate dram before warm boot
            <3> jump to fw entry

*************************************************/

void HAL_WarmBoot(void)
{
    l_pBootEntry = (PFUNC)HAL_StrapGetWarmBootEntry();

    if (NULL != l_pBootEntry)
    {   
        DBG_TRACE(TRACE_WARM_BOOT);
        HAL_ActivateDRAM();
        HAL_Jump();
    }    
}

/************************************************
Function Name: HAL_WarmBoot
Input: 
Output:    
Description:
    check if wram boot or not 
            <1> get warm boot entry 
            <2> activate dram before warm boot
            <3> jump to fw entry

*************************************************/

void HAL_SpiBoot(void)
{
    if (TRUE == HAL_StrapSpiBoot())
    {
        HAL_SpiBootProcess();
    }        
}

/************************************************
Function Name: HAL_SelMpEntry
Input: 
Output:    
Description:
    check if under PCIE or SATA mode 
            <1> select mp entry 
*************************************************/

void HAL_SelMpEntry(void)
{
    if (TRUE == HAL_StrapIsPcie())
    {
        l_pBootEntry = HAL_MpPcieMain;
    }
    else    
    {
        l_pBootEntry = HAL_MpSataMain;
    }
}

/************************************************
Function Name: HAL_ForceMpBoot
Input: 
Output:    
Description:
         jump to mp entry if force to mp 
*************************************************/

void HAL_ForceMpBoot(void)
{
    if (TRUE == HAL_StrapMpBoot())
    {
        HAL_Jump();
    }
}

/************************************************
Function Name: main
Input: 
Output:    
Description:
    main of rom process
    1. ROM init 
        <1> remap entire SRAM back to OTFB
        <2> fetch strap pin info 
        <3> init uart if enable 
        <4> enable trace buffer 
        <5> set EPHY default value if sata mode 
        <6> double CE number if under 16 ce mode 
    2. check if wram boot or not 
        <1> get warm boot entry 
        <2> activate dram before warm boot
        <3> jump to fw entry
    3. check if under PCIE or SATA mode 
        <1> select mp entry
        <2> jump to mp entry if force to mp 
    4. check if spi nor flash boot
        <1> jump to spi boot entry if enabled
        <2> return to flash boot if SPI boot failed 
    5. HW init
        <1> init hw through EFUSE if EFUSE data valid
        <2> init nfc 
    6. flash boot
        <1> reset pu -> read id -> read flash -(success)-> <2>
                                              -(failed)--> <3>
        <2> start pcie/sata if required then execute bootloader 
        <3> accumulate pu then try <1>        
        
*************************************************/
int main(void)
{    
    HAL_RomInit();

    HAL_WarmBoot();

    HAL_SetEPHY();

    HAL_SpiBoot();

    HAL_SelMpEntry();

    HAL_ForceMpBoot();

    HAL_HwInit();
    
    HAL_FlashBoot();

    HAL_Jump();
    return 0;

}


/*
Function table for MP calling member function of ROM 
location is 0xffe05fc0
*/
ROM_FUNC_TABLE const PFUNC aRomFunc[] =
{
    TEST_FuncReturn,
    HAL_ScanPu,
    HAL_HwInit,
    0x123543
};


