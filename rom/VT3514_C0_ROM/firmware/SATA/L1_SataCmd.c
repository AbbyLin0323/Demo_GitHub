/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_SataCmd.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.28    14:31:50
Description :SATA command layer functions
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_SataCmd.h"
#include "L1_SataTrim.h"
#include "L1_SataSmart.h"
#include "L1_SataVenderDefine.h"
#include "L1_Cache.h"

#ifdef SIM
#include <stdio.h>
#endif

//#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
//#endif
#ifdef __cplusplus
extern "C" {
#endif
///* PAGE define */
//U32 g_ulSyscfgPageSize;
//U32 g_ulSyscfgPageSizeBit;
//U32 g_ulSyscfgPageSizeMsk;

extern void HAL_SataSignatureSendGoodStatus(void);
extern void HAL_SataTriggerWritePRD(void);
extern BOOL HAL_SataIsWritePRDEmpty(void);

U32 g_ulPIORemainSectorCount;
U8  g_ucPIODRQBlkLen;
U8  g_ucPIOCurrDataFISLen;
U8  g_ucPIOFirstReadPRD;
PIOINFO gPIOInfoBlock;

U16* g_pSataIdentifyData;
U32 g_ulSyscfgMaxAccessableLba;
U32 g_FirmwareVersion;
U32 g_SataTempBufBaseAddr;
U32 g_SataDataSetBufBaseAddr;
U32 g_SataSmartDataBaseAddr;
U32 g_SataVenderDefineBaseAddr;

BOOL g_bSataFlagAutoActive;
BOOL g_bSataFlagDipmEnabled;
BOOL g_bSataFlagAPTS_Enabled;
BOOL g_bMultipleDataOpen;
U32 g_ulSataPowerMode;

//#define SATA_ONLY_SUPPORT_PIO

#ifdef SIM
static const  U16 s_IdentifyDefault[256] DRAM_ATTR =
#else
static const  U16 s_IdentifyDefault[256] DRAM_ATTR __attribute__ ((aligned (32))) =
#endif
{
    0x0000, 0x0041, 0xC837, 0x0010, 0x0000, 0x0000, 0x003F, 0x0000,  // 000~007
    0x0000, 0x0000, 0x3030, 0x3037, 0x3434, 0x3030, 0x3031, 0x2020,  // 008~015
    0x2020, 0x2020, 0x2020, 0x2020, 0x0000, 0x0000, 0x0000, 0x5665,  // 016~023
    0x725F, 0x302E, 0x3930, 0x5654, 0x3334, 0x3932, 0x2D41, 0x3020,  // 024~031
    0x4657, 0x302E, 0x3930, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020,  // 032~039
    0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x8010,  // 040~047
#ifndef SATA_ONLY_SUPPORT_PIO
    0x0000, 0x2F00, 0x4000, 0x0000, 0x0000, 0x0006, 0x0041, 0x0010,  // 048~055 for NCQ/DMA
    0x003F, 0xFFF0, 0x0000, 0x0110, 0x0000, 0x0001, 0x0000, 0x0007,  // 056~063 for NCQ/DMA
    0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x4000, 0x0000, 0x0000,  // 064~071
    0x0000, 0x0000, 0x0000, 0x001F, 0x0106, 0x0000, 0x0000, 0x0000,  // 072~079 for NCQ/DMA
    0x01FE, 0x0028, 0x0068, 0x7408, 0x4120, 0x0008, 0x3400, 0x4120,  // 080~087
    0x007F, 0x0000, 0x0000, 0x00FE, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095 for NCQ/DMA
#else
    0x0000, 0x2E00, 0x4000, 0x0000, 0x0000, 0x0006, 0x0041, 0x0010,  // 048~055 for PIO
    0x003F, 0xFFF0, 0x0000, 0x0110, 0x0000, 0x0001, 0x0000, 0x0000,  // 056~063 for PIO
    0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x4000, 0x0000, 0x0000,  // 064~071
    0x0000, 0x0000, 0x0000, 0x0000, 0x0006, 0x0000, 0x0000, 0x0000,  // 072~079 for PIO
    0x01FE, 0x0028, 0x0068, 0x7408, 0x4120, 0x0008, 0x3400, 0x4120,  // 080~087
    0x0000, 0x0000, 0x0000, 0x00FE, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095 for PIO
#endif
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000,  // 096~103
    0x0000, 0x0000, 0x4000, 0x0000, 0x5000, 0xCCA3, 0x05CF, 0x150A,  // 104~111
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000, 0x0000,  // 112~119
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 120~127
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 128~135
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 136~143
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 144~151
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 152~159
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 160~167
    0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 168~175
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 176~183
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 184~191
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 192~199
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 200~207
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 208~215
    0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x101F, 0x0051,  // 216~223
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 224~231
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 232~239
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 240~247
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A5   // 248~255
};


/****************************************************************************
Identify default data from 492 boot rom
****************************************************************************/
GLOBAL const U16 Default_Idfy_Data[256]  =
{
  0x0000, 0x0008, 0xC837, 0x0002, 0x0000, 0x0000, 0x0010, 0x0000,  // 000~007
  0x0000, 0x0000, 0x3030, 0x3037, 0x3434, 0x3030, 0x3031, 0x2020,  // 008~015
  0x2020, 0x2020, 0x2020, 0x2020, 0x0000, 0x0000, 0x0000, 0x5665,  // 016~023
  0x722E, 0x3030, 0x3031, 0x5649, 0x4120, 0x5353, 0x4420, 0x5241,  // 024~031
  0x4D44, 0x4953, 0x4B20, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020,  // 032~039
  0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x8010,  // 040~047
  0x0000, 0x2F00, 0x4000, 0x0000, 0x0000, 0x0006, 0x0008, 0x0002,  // 048~055
  0x0010, 0x0100, 0x0000, 0x0110, 0x0200, 0x0000, 0x0000, 0x0007,  // 056~063
  0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x0000, 0x0000, 0x0000,  // 064~071
  0x0000, 0x0000, 0x0000, 0x0000, 0x0006, 0x0000, 0x0000, 0x0000,  // 072~079
  0x03FE, 0x0110, 0x0028, 0x5000, 0x4100, 0x0028, 0x1000, 0x4100,  // 080~087
  0x007F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 096~103
  0x0000, 0x0000, 0x0000, 0x0000, 0x5000, 0xCCA3, 0x05CF, 0x150A,  // 104~111
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 112~119
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 120~127
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 128~135
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 136~143
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 144~151
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 152~159
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 160~167
  0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 168~175
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 176~183
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 184~191
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 192~199
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 200~207
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 208~215
  0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x103F, 0x0051,  // 216~223
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 224~231
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 232~239
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 240~247
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A5   // 248~255
};




void DRAM_ATTR L1_SataCmdInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    g_ulSyscfgMaxAccessableLba =  MAX_ACCESS_LBA ;
    g_bSataFlagAutoActive = FALSE;
    g_bSataFlagDipmEnabled = FALSE;
    g_bSataFlagAPTS_Enabled = FALSE;
    g_bMultipleDataOpen = FALSE;
    g_ulSataPowerMode = SATA_POWER_ACTIVEORIDLE;
    gPIOInfoBlock.pCommand = NULL;
    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NOCMD;
    gPIOInfoBlock.ucCurrRemLenth = 0;
    gPIOInfoBlock.ucRemReadPRD   = 0;
    gPIOInfoBlock.ucReadPRDCnt   = 0;
    g_ucPIOCurrDataFISLen = 0;
    g_ucPIOFirstReadPRD   = FALSE;

    //init identify data for cosim/fpga (otfb version)
    g_pSataIdentifyData = (U16*)OTFB_IDENTIFY_ADDR;
    HAL_MemCpy((U32 *)g_pSataIdentifyData, (U32 *)(&Default_Idfy_Data[0]), SEC_SZ/sizeof(U32));

    return;
}

void DRAM_ATTR L1_SataCmdSetIdentifyDataCheckSum(void)
{
    U8  *pucIdentifyData;
    U8  ucSum;
    U32 ulSum;
    U32 ulLoop;

    ulSum = 0;
    pucIdentifyData = (U8*)g_pSataIdentifyData;
    g_pSataIdentifyData[255] = 0x00A5;

    for (ulLoop = 0; ulLoop < 511; ulLoop++)
    {
        ulSum += pucIdentifyData[ulLoop];
    }

    ucSum = (ulSum & 0xFF);
    g_pSataIdentifyData[255] |= (0xFF - ucSum + 1) << 8;

    DBG_Printf("g_pSataIdentifyData[255] = 0x%x\n", g_pSataIdentifyData[255]);

    ulSum = 0;
    for (ulLoop = 0; ulLoop < 512; ulLoop++)
    {
        ulSum += pucIdentifyData[ulLoop];
    }

    if ((ulSum & 0xFF) == 0)
    {
        DBG_Printf("IdentifyDataCheckSum OK!!\n");
    }
    else
    {
        DBG_Printf("IdentifyDataCheckSum (ulSum & 0xFF) = %d ERROR!!\n", ulSum & 0xFF);
    }
    
    return;
}


/****************************************************************************
Name        :L1_SataSetDefaultIdentifyData
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.04.05    14:16:07
Description :
Others      :
Modify      :
****************************************************************************/
void DRAM_ATTR L1_SataSetDefaultIdentifyData(void)
{

    /*The default identify data shows that:
    all support features are enabled in default identify data, if we don't want
    to enable it, we shall update associated enable bit after boot up.
    1. [80..81]: support ATA7 T13 1532D version 4D
    2. [76]: support 32NCQ, GEN1, GEN2
    3. [78]: support in order data deliverly, DMA setup auto-activetion
    4. [82]: support NOP, HPA, read look-ahead, write cache, mandatory PM
    security.0x446a
    Read buffer and write buffer is not supported
    5. [83]: support flush cache and flush cache ext, support 48bits
    APM is not supported
    6. [84]: GPL support, SMART self-test and error logging support
    7. [85]: SMART is enabled
    8. [86]: words 119..120 is not valid
    9. [87]: SMART self-test supported and error logging supported
    10. 
    */
#ifndef DMAE_ENABLE
    HAL_MemZero((U32 *)g_pSataIdentifyData, SEC_SZ/sizeof(U32));
    //HAL_MemCpy((U32 *)g_pSataIdentifyData, (U32 *)(&s_IdentifyDefault[0]), SEC_SZ/sizeof(U32));
    HAL_MemCpy((U32 *)g_pSataIdentifyData, (U32 *)(&Default_Idfy_Data[0]), SEC_SZ/sizeof(U32));
    
#else
   // (void)HAL_DMAECopyOneBlock((U32)g_pSataIdentifyData, (U32)s_IdentifyDefault, SEC_SZ);
#endif

    return;
}

/****************************************************************************
Name        :L1_SataInitIdentifyData
Input       :
Output      :
Author      :HenryLuo
Date        :2012.04.05    14:32:41
Description :
Others      :
Modify      :
****************************************************************************/
void DRAM_ATTR L1_SataInitIdentifyData(void)
{
//    U32 i;
    U32 ulCHSSecTotal;

    /*copy Identify Default data from sram to DRAM*/
    L1_SataSetDefaultIdentifyData();

    //if support hpa udpate max_access_lba to parameters 

    /*set max accessable address from g_ulSyscfgMaxAccessableLba*/
    /*set WORDS[60..61]*/
    DBG_Printf("[sata]: MAX LBA 0x%x.\n", g_ulSyscfgMaxAccessableLba);

    if( g_ulSyscfgMaxAccessableLba > 0xFFFFFFF )
    {
        g_pSataIdentifyData[61] = 0x0FFF;
        g_pSataIdentifyData[60] = 0xFFFF;
    }
    else
    {
        g_pSataIdentifyData[61] = 
            (U16)((((g_ulSyscfgMaxAccessableLba+1) >> 16) & 0x0FFF));
        g_pSataIdentifyData[60] = (U16)((g_ulSyscfgMaxAccessableLba+1) & 0xFFFF);
    }

    /*set WORDS[100..103]*/
    g_pSataIdentifyData[103] = 0x00;
    g_pSataIdentifyData[102] = 0x00;
    g_pSataIdentifyData[101] = (U16)((g_ulSyscfgMaxAccessableLba+1) >> 16);
    g_pSataIdentifyData[100] = (U16)((g_ulSyscfgMaxAccessableLba+1) & 0xFFFF);

    /*set WORD[1] to default cylinder address*/
    /*
    WORD[1] cylinder address max 16383
    WORD[3] head max 16
    WORD[6] sector max 65
    */

    g_pSataIdentifyData[1] = g_ulSyscfgMaxAccessableLba/
        (g_pSataIdentifyData[3]*g_pSataIdentifyData[6]); 
    if( g_pSataIdentifyData[1] > 0x3FFF )
    {
        g_pSataIdentifyData[1] = 0x3FFF;
    }

    DBG_Printf("[sata]: [60] = 0x%x, [61] = 0x%x\n", g_pSataIdentifyData[60], 
        g_pSataIdentifyData[61]);

    /*WORD[54..56] current CHS address can adjust by command*/
    g_pSataIdentifyData[54] = g_pSataIdentifyData[1];
    g_pSataIdentifyData[55] = g_pSataIdentifyData[3]; 
    g_pSataIdentifyData[56] = g_pSataIdentifyData[6];

    ulCHSSecTotal = g_pSataIdentifyData[1] * g_pSataIdentifyData[3] * g_pSataIdentifyData[6];

    /*WORD[57 58] current capcity of sector*/
    g_pSataIdentifyData[57] = ulCHSSecTotal & 0xffff; 
    g_pSataIdentifyData[58] = (ulCHSSecTotal >> 16) & 0xffff;

    DBG_Printf("[sata]: [100] = 0x%x, [101] = 0x%x, [102] = 0x%x, [103] = 0x%x.\n",
        g_pSataIdentifyData[100], g_pSataIdentifyData[101],
        g_pSataIdentifyData[102], g_pSataIdentifyData[103]);
    
    /*set security status and other features from log page in identify
    data, this is done by each sub module's initialize, so no need to 
    update here.
    In default identify data, we shows that all these features are supported
    but not enabled, if we do not want to support it, we can update identify
    data in each moduel to not support it.
    */

    //mem_cpy(&g_pSataIdentifyData[27],p_startup_page->sata_serial_number,10);
    /*
    L1_SataStrCpy(&g_pSataIdentifyData[27],p_startup_page->sata_module_name,40);
    L1_SataStrCpy(&g_pSataIdentifyData[10],p_startup_page->sata_serial_number,20);
    L1_SataStrCpy(&g_pSataIdentifyData[23],&g_firmware_version,8);
    */

}

/****************************************************************************
Name        :L1_SataCmdIdentifyDevice
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.02.28    14:35:29
Description :
Others      :
Modify      :
****************************************************************************/
void DRAM_ATTR L1_SataCmdIdentifyDevice()
{
    DBG_Printf("g_pSataIdentifyData: 0x%x\n",g_pSataIdentifyData);

    L1_SataCmdSetIdentifyDataCheckSum();
    L1_SataHandleSpecialPIODataIn(1, (U32)g_pSataIdentifyData);

    return;
}

/****************************************************************************
Name        :L1_SataCmdSetFeatures
Input       :void
Output      :BOOL
Author      :HenryLuo
Date        :2012.04.05    15:39:30
Description :
Others      :
Modify      :
****************************************************************************/
void DRAM_ATTR L1_SataCmdSetFeatures(void)
{
    U8 trans_mode;
    BOOL    berror = FALSE;
    U8 feature_code;

    feature_code = rSDC_FEATURE_ERROR;
    switch(feature_code)
    {
    case ENABLE_WRITE_CACHE:
        DBG_Printf("[set features]: enable write cache.\n");
        g_pSataIdentifyData[85] |= 0x0020;        
        break;

    case DISABLE_WRITE_CACHE:
        DBG_Printf("[set features]: disable write cache.\n");
        g_pSataIdentifyData[85] &= (~0x0020);    
        break;

    case SET_TRANSFER_MODE:
        trans_mode = rSDC_SECCNT;
        //TODO: transfer mode setup
        DBG_Printf("[set features]: set transfer mode to 0x%x.\n", trans_mode);
        switch( trans_mode & 0xF8 )
        {
        case    0 << 3:    /*PIO default mode*/
            /*set PIO mode to default mode*/
            /* Nothing is needed to do */
            break;

        case    1 << 3:    //PIO transfer mode
            // YaoChen: PIO flow control transfer mode, bit 7:3 = 00001b  2008/12/13
            trans_mode &= 0x7;
            //check if target mode is supported or not
            if( trans_mode > 4 )
            {
                DBG_Printf("[set features]: target mode 0x%x is not support.\n", trans_mode);
                berror = TRUE;    //PIO mode 4 is maximum supported mode
            }
            //set PIO mode to mode (transMode & 0x7)
            break;

        case    4 << 3:    //multiword DMA
            // YaoChen: bit 7:3 = 00100b 2008/12/13
            trans_mode &= 0x7;
            //set MultitWord DMA mode to mode (transMode & 0x7)
            //check if target mode is supported or not
            if( trans_mode > 2 )    
            {
                DBG_Printf("[set features]: target mode 0x%x is not support.\n", trans_mode);
                berror = TRUE;    //Multiple word DMA mode 2 is maximum supported mode
                break;
            }
            g_pSataIdentifyData[63] &= ~((1 << 8) | (1 << 9) | (1 << 10));    //Clear current Multiword DMA mode field
            g_pSataIdentifyData[63] |= (1 << (8 + trans_mode));    //Set identify device information field bit
            // according to current Multiword DMA mode
            g_pSataIdentifyData[88] &= 0xFF;    //Clear any Ultra DMA mode field;
            break;

        case    8 << 3:    //ultraDMA mode
            // YaoChen: bit 7:3 = 01000b 2008/12/13
            trans_mode &= 0x7;
            //check if target mode is supported or not
            if( trans_mode > 6 )    
            {
                DBG_Printf("[set features]: target mode 0x%x is not support.\n", trans_mode);
                berror = TRUE;    //Ultra DMA mode 6 is maximum supported mode
                break;
            }
            g_pSataIdentifyData[63] &= ~((1 << 8) | (1 << 9) | (1 << 10));    //Clear current Multiword DMA mode field
            g_pSataIdentifyData[88] &= 0x80FF;    //Clear any Ultra DMA mode field;
            g_pSataIdentifyData[88] |= (1 << (8 + trans_mode));    //Set identify device information field bit according to current Ultra DMA mode
            break;

        default:
            DBG_Printf("[set features]: target mode 0x%x is not support.\n", trans_mode);
            berror = TRUE;
            break;
        }
        break;

    case ENABLE_APM:
        trans_mode = rSDC_SECCNT;
        if(0 == trans_mode)
        {
            berror = TRUE;
        }

        else if((trans_mode >= 0x01) && (trans_mode <= 0x7F))
        {
            g_ulSataPowerMode = SATA_POWER_STANDBY;
        }

        else
        {
            g_ulSataPowerMode = SATA_POWER_ACTIVE;
        }

        break;

    case DISABLE_APM:
        g_ulSataPowerMode = SATA_POWER_ACTIVE;
        break;

    case ENABLE_SATA_FEATURE:
        DBG_Printf("[set features]: enable sata features 0x%x.\n", rSDC_SECCNT);
        switch( rSDC_SECCNT )
        {
        case SATA_FEATURE_NONZERO_OFFSET:                       
            g_pSataIdentifyData[79] |= 0x0002;
            break;

        case SATA_FEATURE_AUTO_ACTIVATE:
            g_bSataFlagAutoActive = TRUE;
            g_pSataIdentifyData[79] |= 0x0004;
            break;

        case SATA_FEATURE_DIPM:
            g_bSataFlagDipmEnabled = TRUE;
            g_pSataIdentifyData[79] |= 0x0008;
            break;

        case SATA_FEATURE_DIAPTS:
            if ( TRUE == g_bSataFlagDipmEnabled )
                g_bSataFlagAPTS_Enabled = TRUE;
            else
                berror = TRUE;
            break;

        case SATA_FEATURE_GUARANTEED_INORDER:
            g_pSataIdentifyData[79] |= 0x0010;
            break;

        default:
            berror = TRUE;
            break;
        }
        break;

    case DISABLE_SATA_FEATURE:
        DBG_Printf("[set features]: disable sata features 0x%x.\n", rSDC_SECCNT);
        switch( rSDC_SECCNT )
        {
        case SATA_FEATURE_NONZERO_OFFSET:
            g_pSataIdentifyData[79] &= (~0x0002);
            break;

        case SATA_FEATURE_AUTO_ACTIVATE:
            g_bSataFlagAutoActive = FALSE;
            g_pSataIdentifyData[79] &= (~0x0004);
            break;

        case SATA_FEATURE_DIPM:
            g_bSataFlagDipmEnabled = FALSE;
            g_pSataIdentifyData[79] &= ~(0x0008);
            break;

        case SATA_FEATURE_DIAPTS:
            g_bSataFlagAPTS_Enabled = FALSE;
            break;

        case SATA_FEATURE_GUARANTEED_INORDER:
            g_pSataIdentifyData[79] &= (~0x0010);
            break;

        default:
            berror = TRUE;
            break;
        }
        break;

    default :
        DBG_Printf("[set features]: invalid feature code 0x%x.\n", feature_code);
        berror = TRUE;
        break;
    }

    if( TRUE == berror )
    {
        while( FAIL == HAL_SataIsFISXferAvailable() );
        HAL_SataSendAbortStatus();    
    }
    else
    {
        while( FAIL == HAL_SataIsFISXferAvailable() );
        HAL_SataSendSuccessStatus();    
    }

    //DBG_Printf("[set features]: done.\n");

    return;
}

/*===================================================================
Function   :     ATA_Cmd_SetMultipleMode
Input      :     None
Output     :     None 
Description :  Host command process routine, this function will not return until the commands
process finish
Note: 
Modify History:
20090607    JackeyChai    001: first created
===================================================================*/
void DRAM_ATTR L1_SataCmdSetMultipleMode(void)
{
    U8    sectorCount;
    U8     ShiftCount;
    BOOL bSetMultipleError = TRUE;

    //update identify device data;
    sectorCount = rSDC_SECCNT;
    if( sectorCount <= (U8) (g_pSataIdentifyData[47] & 0xff))
    {
        if(sectorCount == 0)
        {
            g_bMultipleDataOpen = FALSE;
            bSetMultipleError = FALSE;
        }

        else
        {    
            ShiftCount = 0;
            while(ShiftCount < 5)
            {
                if((1<<ShiftCount)  == sectorCount)
                {
                    g_bMultipleDataOpen = TRUE;
                    g_pSataIdentifyData[59] = (0x0100 | sectorCount);
                    bSetMultipleError = FALSE;
                    break;
                }
                ShiftCount ++;
            }
        }
    }

    if(bSetMultipleError == FALSE)
    {
        while( FAIL == HAL_SataIsFISXferAvailable() );
        HAL_SataSendSuccessStatus();
    }
    else
    {
        //Do not support the transfer count set by host, abort this command
        while( FAIL == HAL_SataIsFISXferAvailable() );
        HAL_SataSendAbortStatus();
    }

    return;
}


/*===================================================================
Function   :     ATA_Cmd_ExecuteDeviceDiagnostic
Input      :     None
Output     :     None 
Description :  Host command process routine, this function will not return until the commands
process finish
Note: 
Modify History:
20090604    JackeyChai    001:    first created
===================================================================*/
void DRAM_ATTR L1_SataCmdExecuteDeviceDiagnostic(void)
{
    //do hardware initialize and execute diagnostic

    //send good signature
  //  HAL_SataSendGoodStatus();
    HAL_SataSignatureSendGoodStatus();

    DBG_Printf("L1_SataCmdExecuteDeviceDiagnostic end\n");

    return;
}



/****************************************************************************
Name:    L1_SataHandleSpecialPIODataIn
Input:    ucSecCnt ulStartDRamAddr
Output:    No output required.
Author:    Blake Zhang
Date:    2013.02.25
Description: The routine for executing an ATA data access command which follows the PIO in/out protocol.
Others:
Modify:
****************************************************************************/
void DRAM_ATTR L1_SataHandleSpecialPIODataIn(U8 ucSecCnt, U32 ulStartDRamAddr)
{
    volatile SATA_DSG *pCurDSG; //[P] volatile
    U8    ucDSGId;
    U32   ulBuffMapValue;

    if (ucSecCnt == 0 || ulStartDRamAddr < DRAM_START_ADDRESS)
    {
        DBG_Printf("L1_SataHandleSpecialPIODataIn Input ERROR!\n");
        DBG_Getch();
    }

    ucDSGId = L1_GetSpecialReadDSG();
    if (INVALID_2F == ucDSGId)
    {
        DBG_Printf("L1_SataHandleSpecialPIODataIn No free ReadPRD ERROR!\n");
        DBG_Getch();
    }
    
    pCurDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);//&g_pSataReadPRD[ucRPRDId];

    // clear PRD area to 0
    HAL_MemZero((U32*)pCurDSG, DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
    pCurDSG->AtaProtInfo.AutoActiveEn    = BIT_ENABLE;
    pCurDSG->AtaProtInfo.IsWriteCmd      = BIT_FALSE;
    pCurDSG->AtaProtInfo.EcpEn           = BIT_DISABLE;
    pCurDSG->AtaProtInfo.EcpKeySel       = 0;
    pCurDSG->AtaProtInfo.IsNonDataCmd    = BIT_FALSE;
    pCurDSG->AtaProtInfo.FwAckHostEn     = BIT_DISABLE;
    pCurDSG->AtaProtInfo.ProtSel         = PROT_PIO;
    pCurDSG->AtaProtInfo.CmdTag          = 0;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = ucSecCnt;
    pCurDSG->AtaProtInfo.XferEndIntEn    = SDC_NOT_INT_MCU;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    pCurDSG->XferCtrlInfo.Reserved       = 0;
    pCurDSG->XferCtrlInfo.DummyDataEn    = BIT_DISABLE;
    pCurDSG->CacheStsLocSel = CS_IN_SRAM;
#ifdef OTFB_VERSION
    pCurDSG->XferCtrlInfo.DataLocSel     = DATA_IN_SRAM;
    pCurDSG->DataAddr        = ulStartDRamAddr - OTFB_START_ADDRESS;
#else
    pCurDSG->XferCtrlInfo.DataLocSel     = DATA_IN_DRAM;
    pCurDSG->DataAddr        = ulStartDRamAddr - DRAM_START_ADDRESS;
#endif
    pCurDSG->XferCtrlInfo.BuffLen        = ucSecCnt;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.BuffOffset     = 0;
    pCurDSG->XferCtrlInfo.CacheStsEn     = BIT_DISABLE;
    pCurDSG->XferCtrlInfo.BuffMapEn      = BIT_ENABLE;
    pCurDSG->XferCtrlInfo.Eot            = BIT_TRUE;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    pCurDSG->NextDsgId       = INVALID_2F;
    pCurDSG->CacheStsData    = 0;
    pCurDSG->CmdLbaHigh      = 0;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    pCurDSG->CacheStsAddr    = 0;
    pCurDSG->CmdLbaLow       = 0;

    ulBuffMapValue = (0xffffffff << (0 >> SEC_PER_LPN_BITS))
                   & (0xffffffff >> (31 - ((ucSecCnt - 1) >> SEC_PER_LPN_BITS)));
                   
    HAL_SetBufMapInitValue(pCurDSG->XferCtrlInfo.BuffMapId, ulBuffMapValue);

    HAL_SetLastDataReady(0);
    HAL_UsedSataDSG(ucDSGId);
    HAL_SetFirstDSGID(0, ucDSGId);
    HAL_SetFirstReadDataReady(0);

    //6. Send a PIO setup FIS to host
    Lock_ShadowRegister();
    rSDC_PIOXferCountLow = (U8)(ucSecCnt << SEC_SZ_BITS);
    rSDC_PIOXferCountHigh = (U8)(ucSecCnt << (SEC_SZ_BITS - 8));
    rSDC_COMMAND_STATUS = 0x58;
    rSDC_END_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_FISDirInt = BIT_SDC_FIS_DIRFLAG | BIT_SDC_FIS_INTFLAG;
    HAL_SataSendPIOSetupFIS();

    while (FALSE == HAL_SataIsFISXferAvailable());
    
    UnLock_ShadowRegister();

    //7. Send a PIO data FIS to host
    HAL_SataSendPIODataFIS();

    while (FALSE == HAL_SataIsFISXferAvailable()); //wait Data FIS transfer finish.
    
    HAL_SetSendSDBFISReady(0);

    return;
}

/****************************************************************************
Name:    L1_SataHandleSpecialPIODataOut
Input:    ucSecCnt ulStartDRamAddr
Output:    No output required.
Author:    Blake Zhang
Date:    2013.02.25
Description: The routine for executing an ATA data access command which follows the PIO in/out protocol.
Others:
Modify:
****************************************************************************/
BOOL DRAM_ATTR L1_SataHandleSpecialPIODataOut(U8 ucSecCnt, U32 ulStartDRamAddr)
{
    volatile SATA_DSG *pCurDSG; //[P] volatile
    U8    ucDSGId;
    U32   ulBuffStartAddrSect;

    if (ucSecCnt == 0 || ulStartDRamAddr < DRAM_START_ADDRESS)
    {
        DBG_Printf("L1_SataHandleSpecialPIODataOut Input ERROR!\n");
        DBG_Getch();
    }

    ucDSGId = L1_GetSpecialWriteDSG();
    if (INVALID_2F == ucDSGId)
    {
        DBG_Printf("L1_SataHandleSpecialPIODataIn No free ReadPRD ERROR!\n");
        DBG_Getch();
    }
    
    pCurDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);//&g_pSataReadPRD[ucRPRDId];

    // clear PRD area to 0
    HAL_MemZero((U32*)pCurDSG, DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
    pCurDSG->AtaProtInfo.AutoActiveEn    = BIT_ENABLE;
    pCurDSG->AtaProtInfo.IsWriteCmd      = BIT_TRUE;
    pCurDSG->AtaProtInfo.EcpEn           = BIT_DISABLE;
    pCurDSG->AtaProtInfo.EcpKeySel       = 0;
    pCurDSG->AtaProtInfo.IsNonDataCmd    = BIT_FALSE;
    pCurDSG->AtaProtInfo.FwAckHostEn     = BIT_DISABLE;
    pCurDSG->AtaProtInfo.ProtSel         = PROT_PIO;
    pCurDSG->AtaProtInfo.CmdTag          = 0;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = ucSecCnt;
    pCurDSG->AtaProtInfo.XferEndIntEn    = SDC_NOT_INT_MCU;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    pCurDSG->XferCtrlInfo.Reserved       = 0;
    pCurDSG->XferCtrlInfo.DummyDataEn    = BIT_DISABLE;
    pCurDSG->CacheStsLocSel = CS_IN_SRAM;
#ifdef OTFB_VERSION
    pCurDSG->XferCtrlInfo.DataLocSel     = DATA_IN_SRAM;
    pCurDSG->DataAddr        = ulStartDRamAddr - OTFB_START_ADDRESS;
#else
    pCurDSG->XferCtrlInfo.DataLocSel     = DATA_IN_DRAM;
    pCurDSG->DataAddr        = ulStartDRamAddr - DRAM_START_ADDRESS;
#endif
    pCurDSG->XferCtrlInfo.BuffLen        = ucSecCnt;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.BuffOffset     = 0;
    pCurDSG->XferCtrlInfo.CacheStsEn     = BIT_DISABLE;
    pCurDSG->XferCtrlInfo.BuffMapEn      = BIT_DISABLE;
    pCurDSG->XferCtrlInfo.Eot            = BIT_TRUE;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    pCurDSG->NextDsgId       = INVALID_2F;
    pCurDSG->CacheStsData    = 0;
    pCurDSG->CmdLbaHigh      = 0;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    pCurDSG->CacheStsAddr    = 0;
    pCurDSG->CmdLbaLow       = 0;

    HAL_SetLastDataReady(0);
    HAL_UsedSataDSG(ucDSGId);
    HAL_SetFirstDSGID(0, ucDSGId);
    HAL_SetFirstReadDataReady(0);

    //6. Send a PIO setup FIS to host
    Lock_ShadowRegister();
    rSDC_PIOXferCountLow = (U8)(ucSecCnt << SEC_SZ_BITS);
    rSDC_PIOXferCountHigh = (U8)(ucSecCnt << (SEC_SZ_BITS - 8));
    rSDC_COMMAND_STATUS = 0x58;
    rSDC_END_STATUS = 0x50;
    rSDC_FEATURE_ERROR = 0;
    rSDC_FISDirInt = 0;
    HAL_SataSendPIOSetupFIS();
    UnLock_ShadowRegister();

    /* 
    7. Receive a PIO data FIS from host. check if current request finish or not 
    */
    while(1)
    {
        if( TRUE == g_bFlagPIODataRec ) 
        {
            g_bFlagPIODataRec = FALSE;
            break;
        }
    }

    while( FALSE == HAL_SataIsFISXferAvailable() );
    HAL_SataSendSuccessStatus();

    return TRUE;
}

U8 L1_GetDRQBlkLen(U8 ucHCmdCode)
{
    U8 ucDRQBlkLen;
    
    /* Decide the DRQ block length according to command code. */
    switch (ucHCmdCode)
    {
        /* Sector based command. */
        case ATA_CMD_READ_SECTOR:
        case ATA_CMD_WRITE_SECTOR:
        case ATA_CMD_READ_SECTOR_EXT:
        case ATA_CMD_WRITE_SECTOR_EXT:
            ucDRQBlkLen = 1;
            break;

        /* Multiple based command. */
        default:
            ucDRQBlkLen = (U8)g_pSataIdentifyData[59];
            break;
    }

    return ucDRQBlkLen;
}


/****************************************************************************
Name:    L1_SataHandlePIODataProtocol
Input:    No input required.
Global data structure gPIOInfoBlock is providing information as input.
Output:    No output required.
Author:    Yao Chen
Date:    2012.09.18
Description: The routine for executing an ATA data access command which follows the PIO in/out protocol.
Others:
Modify:
****************************************************************************/
void DRAM_ATTR L1_SataHandlePIODataProtocol(void)
{
    BOOL bFirstDRQBlk, bLastDRQBlk;

    static HCMD *pCurrHostCmd;
    static U32 ulRemainSectorCount;
    static U8 ucDRQBlkLen, ucCurrDataFISLen;

    /* No PIO command pending? It is an error assertion. */
    if (NULL == gPIOInfoBlock.pCommand )
    {
        return;
    }
    
    /* Entering PIO state machine. */
    switch ( gPIOInfoBlock.ucCurrPIOState ) 
    {
        /* 1. A new host command is pending and we shall record its parameters and initialize our data record. */
        case SATA_PIO_NEWCMD:
        
            g_bFlagPIODataRec   = FALSE;
            pCurrHostCmd        = gPIOInfoBlock.pCommand;
            ulRemainSectorCount = pCurrHostCmd->ulCmdSectorCnt;
            ucDRQBlkLen         = L1_GetDRQBlkLen(pCurrHostCmd->ucCmdCode);
            gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_NEWCMD<<24) | ulRemainSectorCount;
#endif
            break;
            /* 2. PIO setup stage. The parameters of a DRQ block shall be calculated,
            and a PIO setup FIS shall be sent in this stage. */
        case SATA_PIO_SETUP:
        
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_SETUP<<24) | 1;
#endif

            /* If there is any pending FIS, we shall neither trigger another FIS nor modify PIO state now. */
            if (FAIL == HAL_SataIsFISXferAvailable())
            {
                break;
            }
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_SETUP<<24) | 2;
#endif            
            /* Decide whether current DRQ block is the first one within current host command. */
            if ( ulRemainSectorCount == pCurrHostCmd->ulCmdSectorCnt )
            {
                bFirstDRQBlk = TRUE;
            }
            else
            {
                bFirstDRQBlk = FALSE;
            }
            
            /* Calculate current DRQ block length and decide whether
            current DRQ block is the last one within current host command. */
            if ( ulRemainSectorCount > (U32)ucDRQBlkLen ) 
            {
                ucCurrDataFISLen = ucDRQBlkLen;
                bLastDRQBlk = FALSE;
            }
            else 
            {
                ucCurrDataFISLen = ulRemainSectorCount;
                bLastDRQBlk = TRUE;
            }

            /* Construct the PIO setup FIS through our information on current DRQ block. */
            //            dbg_printf("PIO Setup\n\r");
            HAL_SataConstructAndSendPIOSetupFIS(bFirstDRQBlk, bLastDRQBlk, ucCurrDataFISLen, pCurrHostCmd->ucCmdRW);
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_SETUP<<24) | 3;
#endif

            /* Select the next stage according to the data direction. */
            if ( HCMD_READ == pCurrHostCmd->ucCmdRW )
            {
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_DATA_IN;
            }
            else
            {
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_DATA_OUT;
            }
            break;

            /* 3. PIO data in stage. The current DRQ block shall be sent to host in this stage. */
        case SATA_PIO_DATA_IN:
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_IN<<24) | 1;
#endif

            /* If there is any pending FIS, we shall neither trigger another FIS nor modify PIO state now. */
            if( FAIL == HAL_SataIsFISXferAvailable() )
            {
                break;
            }
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_IN<<24) | 2;
#endif

            /* Now we can trigger the data FIS. */
            //dbg_printf("PIOR\n\r");
            HAL_SataSendPIODataFIS();
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_IN<<24) | 3;
#endif

            /* Update our global data record of current command. */
            ulRemainSectorCount -= ucCurrDataFISLen;

            /* Select the next stage according to the remaining data length. */
            if ( 0 == ulRemainSectorCount ) 
            {                
                gPIOInfoBlock.pCommand = NULL;
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NOCMD;

                while ( FALSE == HAL_SataIsFISXferAvailable() ); //wait Data FIS transfer finish.
                
                rSDC_FW_Ctrl  |= CLR_PIOCMD_DATA; //VT3514, clear small busy
                //dbg_printf("PIORFin\n\r");
#ifdef SIM_XTENSA
                rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_IN<<24) | 4;
#endif
            }
            else
            {
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;
            }
            break;

            /* 4. PIO data out stage. The current DRQ block shall be received to a buffer. */
        case SATA_PIO_DATA_OUT:
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_OUT<<24) | 1;
#endif

            /* Check whether a data FIS from host has been received. */
            if ( TRUE == g_bFlagPIODataRec ) 
            {
                /* Clear the corresponding global flag. */
                g_bFlagPIODataRec = FALSE;
                //dbg_printf("PIOW\n\r");

                /* Update our global data record of current command. */
                ulRemainSectorCount -= ucCurrDataFISLen;

                /* Select the next stage according to the remaining data length. */
                if ( 0 == ulRemainSectorCount )
                {
                    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_FINISH;
#ifdef SIM_XTENSA
                    rTracer = TL_PIO_STAGE | (SATA_PIO_DATA_OUT<<24) | 2;
#endif
                }
                else
                {
                    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;
                }
            }
#ifndef SIM
            HAL_EnableMCUIntAck();
#endif
            /* The data FIS has not been received yet and we shall keep waiting in current state. */
            break;

            /* 5. PIO finishing stage dedicate for a data out command. The execution status shall be reported to host. */
        case SATA_PIO_FINISH:
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_FINISH<<24) | 1;
#endif

            /* If there is any pending FIS, we shall neither trigger another FIS nor modify PIO state now. */
            if( FAIL == HAL_SataIsFISXferAvailable() )
            {
                break;
            }
            
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_FINISH<<24) | 2;
#endif

            /* Send the status report to host. */
            HAL_SataSendSuccessStatus();
#ifdef SIM_XTENSA
            rTracer = TL_PIO_STAGE | (SATA_PIO_FINISH<<24) | 3;
#endif

            gPIOInfoBlock.pCommand = NULL;
            gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NOCMD;
            //dbg_printf("PIOWFin\n\r");
            
            /* Now we can clear the PIO command pending flag. */
            rSDC_FW_Ctrl  |= CLR_PIOCMD_DATA; //VT3514, clear small busy

            break;

            /* 6. The dummy branch for a incorrect PIO state. */
        default:
            DBG_Printf("L1_SataHandlePIODataProtocol State ERROR!!\n");
            DBG_Getch();
            break;
    }

    return;
}


/****************************************************************************
Name:    L1_SataCheckPIOMultipleEnable
Input:    pCurCmd - the pointer to the command slot which contains the PIO command to be checked.
Output:    SUCCESS - command is valid with current multiple enable state.
FAIL - command is not valid with current multiple enable state.
Author:    Yao Chen
Date:    2012.09.18
Description: The routine checks whether current host command is valid with current PIO multiple
access setting. For commands not implementing PIO protocol, it always returns SUCCESS.
Others:
Modify:
****************************************************************************/
BOOL DRAM_ATTR L1_SataCheckPIOMultipleEnable(HCMD *pCurCmd)
{
    BOOL bResult;
    U8 ucCmdCode = pCurCmd->ucCmdCode;

    switch ( ucCmdCode ) {

        /* For commands implementing PIO data in/out protocol, we shall check the multiple enable flag. */
        case ATA_CMD_READ_MULTIPLE:
        case ATA_CMD_WRITE_MULTIPLE:
        case ATA_CMD_READ_MULTIPLE_EXT:
        case ATA_CMD_WRITE_MULTIPLE_EXT:
            bResult = g_bMultipleDataOpen;
            break;

            /* For commands not implementing PIO data in/out protocol, we shall not perform the check. */
        default:
            bResult = SUCCESS;
            break;
    }

    return bResult;
}




#ifdef __cplusplus
}
#endif
