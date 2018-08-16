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
*******************************************************************************/

#ifndef _SDC_COMMON_H_INCLUDED
#define _SDC_COMMON_H_INCLUDED

#include "BaseDef.h"

#ifdef    __cplusplus
extern "C" {
#endif

    typedef struct
    {
        U8 status;
        U32 start_time;
        U32 end_time;
        struct shadow_register
        {
            U8 sdc_data;
            U8 sdc_fea;
            U8 sdc_seccnt;
            U8 sdc_lbalow;
            U8 sdc_lbahight;
            U8 sdc_device;
            U8 sdc_status;
        };

        struct shadow_reggister_exp
        {
            U8 sdc_fea_exp;
            U8 sdc_seccnt_exp;
            U8 sdc_lbalow_exp;
            U8 sdc_lbahight_exp;
            U8 sdc_device_exp;
            U8 sdc_status_exp;
        };

    }sata_cmdinfo;

    sata_cmdinfo sata_cmd_entry[33];

    extern void sdcRegRead (U32 regaddr, U32 regvalue, U32 nsize);
    extern BOOL sdcRegWrite(U32 regaddr, U32 regvalue, U32 nsize);

    extern void spinLockRead(U32 regaddr, U32 regvalue, U32 nsize);
    extern BOOL spinLockWrite(U32 regaddr, U32 regvalue, U32 nsize);

    extern U32 sata_sim_clocktime();

#ifdef SIM
    void regRead(U32 addr, U32 nBytes, U8 *dst);
    void regWrite(U32 addr, U32 nBytes, const U8*src);

    void dramRead(U32 addr, U32 nWordss, U8 *buf);
    void dramWrite(U32 addr, U32 nWordss, const U8* src);
#endif

#ifdef    __cplusplus
}
#endif

#endif