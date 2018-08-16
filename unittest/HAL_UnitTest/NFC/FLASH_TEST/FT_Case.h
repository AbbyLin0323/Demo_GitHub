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
Filename    : FT_Case.h
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : 
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_FLASH_TEST_CASE_H_
#define _HAL_FLASH_TEST_CASE_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    DATA STRUCTURES DEFINITION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void FT_BasicEWR(void);
void FT_MultiPuEWR(void);
void FT_SnapRead(void);
void FT_ReadIDB(void);
void FT_EraseAllBlk(void);
void FT_OTPRead(void);
void FT_N0Read(void);
void FT_HealthChkWholeDisk(void);
void FT_N0ChkWholeDisk(void);
void FT_MLCSinglePageProg(void);
void FT_SLCShiftAddrTest(void);
void FT_GetDefaultFeat(void);

#endif

/* end of this file */
