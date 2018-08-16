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
Description :
*******************************************************************************/
#ifndef __HGE_UNITTEST_H__
#define __HGE_UNITTEST_H__

//#define TEST_HGE_NORMAL

typedef enum _HGE_TEST_MODE_
{
    HGE_SLC_NORMAL = 0,
    HGE_MLC_NORMAL,
    HGE_TLC_NORMAL,
    HGE_SLC_NO_READ,
    HGE_MLC_NO_READ,
    HGE_TLC_NO_READ,
    HGE_MLC_NO_READ_FIFO,
    HGE_SLC_NO_READ_ERROR_INJ,
    HGE_MLC_NO_READ_ERROR_INJ,
    HGE_TLC_NO_READ_ERROR_INJ,
    HGE_MLC_NO_READ_ERROR_INJ_FIFO,
    HGE_TOTAL_MODE
}HGE_TEST_MODE;

//void UT_HgeMain();

#define TEST_HGE_NORMAL

#endif/* __HGE_UNITTEST_H__ */

