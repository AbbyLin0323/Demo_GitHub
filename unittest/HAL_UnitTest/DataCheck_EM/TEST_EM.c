/****************************************************************************
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
*****************************************************************************
Filename    : TEST_NfcDataCheck.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description : this file provide test pattern for NFC first byte check function
Others      : 
Modify      :
20141104    Gavin     Create file
20160110    SparkSun  Porting to VT3533
****************************************************************************/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_HostInterface.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_NfcDataCheck.h"
#include "HAL_EncriptionModule.h"
#include "FW_BufAddr.h"
#include "HAL_MultiCore.h"
#include "Disk_Config.h"

#include "TEST_NfcFuncBasic.h"

extern void TEST_NfcBasicPattRunInit(void);

BOOL UT_EmMain(void)
{
    //TEST_NfcBasicInit();
    TEST_NfcBasicPattRunInit();
    
    #ifdef TLC_MODE_TEST
    g_bTlcMode = TRUE;
    #endif

    U8 ucEmKeySize = 0;
    U8 ucEmModeSel;

    /* Select a EM KeySize */
    while (ucEmKeySize <= EM_KEY_SIZE_256BIT)
    {
        /* Select a EM Mode */
        ucEmModeSel = 0;
        while (TRUE)
        {
            switch (ucEmModeSel)
            {
                case 0:
                {
                    ucEmModeSel = EM_MODE_XTS;
                }
                break;
                
                case EM_MODE_XTS:
                {
                    ucEmModeSel = EM_MODE_CBC_ESSIV;
                }
                break;

                case EM_MODE_CBC_ESSIV:
                {
                    ucEmModeSel = EM_MODE_CTR_ESSIV;
                }
                break;

                case EM_MODE_CTR_ESSIV:
                {
                    ucEmModeSel = EM_MODE_ECB;
                }
                break;

                case EM_MODE_ECB:
                {
                    ucEmModeSel = EM_MODE_CTR;
                }
                break;

                default:
                {
                    DBG_Printf("Wrong EM Mode, ModeSel=%d!",ucEmModeSel);
                    DBG_Getch();
                }
                break;
            }
            
            /* Change EM engine with KeySize & Mode setting */
            HAL_EMInit(ucEmKeySize, ucEmModeSel,NULL);

            /* Enable NFC EM Function  */
            g_bEmEnable = TRUE;

            /* Test EM with NFC Multi-Plane E/W/R operation  */
            TEST_NfcSinglePlnEWR();
            DBG_Printf("EM KeySizeSel=%d, ModeSel=%d Test Pass!!\n",ucEmKeySize, ucEmModeSel);

            if (ucEmModeSel == EM_MODE_CTR)
            {
                break;// break while(TRUE) circle
            }
        }
        DBG_Printf("EM function test KeySize=%d all mode test pass!\n",ucEmKeySize);
        ucEmKeySize++;
    }

    
    return TRUE;
}


