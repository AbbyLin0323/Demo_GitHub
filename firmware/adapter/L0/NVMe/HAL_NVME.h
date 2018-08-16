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


#ifndef HAL_NVME_H_
#define HAL_NVME_H_
#ifdef __cplusplus
extern "C"{
#endif
#include "BaseDef.h"


/* Copied and modified from Linux Source code*/
struct nvme_bar {
    volatile U32 capl;    /* Controller Capabilities */
    volatile U32 caph;
    volatile U32 vs;    /* Version */
    volatile U32 intms;    /* Interrupt Mask Set */
    volatile U32 intmc;    /* Interrupt Mask Clear */
    volatile U32 cc;    /* Controller Configuration */
    volatile U32 rsvd1;    /* Reserved */
    volatile U32 csts;    /* Controller Status */
    volatile U32 rsvd2;    /* Reserved */
    volatile U32 aqa;    /* Admin Queue Attributes */
    volatile U32 asql;    /* Admin SQ Base Address */
    volatile U32 asqh;
    volatile U32 acql;    /* Admin CQ Base Address */
    volatile U32 acqh;
};

#define NVME_CAP_MQES(cap)    ((cap) & MSK_4F)
#define NVME_CAP_CQR(cap)    (((cap) >> 16) & 1u)
#define NVME_CAP_AMS(cap)    (((cap) >> 17) & 3u)
#define NVME_CAP_TIMEOUT(cap)    (((cap) >> 24) & MSK_2F)
#define NVME_CAP_STRIDE(cap)    (((cap) >> 32) & MSK_1F)
#define NVME_CAP_MPSMIN(cap)    (((cap) >> 48) & MSK_1F)
#if 0
enum {
    NVME_CC_ENABLE        = 1 << 0,
    NVME_CC_CSS_NVM        = 0 << 4,
    NVME_CC_MPS_SHIFT    = 7,
    NVME_CC_ARB_RR        = 0 << 11,
    NVME_CC_ARB_WRRU    = 1 << 11,
    NVME_CC_ARB_VS        = 7 << 11,
    NVME_CC_SHN_NONE    = 0 << 14,
    NVME_CC_SHN_NORMAL    = 1 << 14,
    NVME_CC_SHN_ABRUPT    = 2 << 14,
    NVME_CC_SHN_MASK    = 3 << 14,
    NVME_CC_IOSQES        = 6 << 16,
    NVME_CC_IOCQES        = 4 << 20,
    NVME_CSTS_RDY        = 1 << 0,
    NVME_CSTS_CFS        = 1 << 1,
    NVME_CSTS_SHST_NORMAL    = 0 << 2,
    NVME_CSTS_SHST_OCCUR    = 1 << 2,
    NVME_CSTS_SHST_CMPLT    = 2 << 2,
    NVME_CSTS_SHST_MASK    = 3 << 2,
};

#endif

/*
 * 9 Doorbells
 * */

struct hal_nvme_doorbell{
    volatile U32 sq_tail;
    volatile U32 cq_head;
};


/*
 * 9 MSI-X Table Entry
 * */

struct hal_nvme_msix_tab_entry{
    volatile U32 msg_addr;
    volatile U32 msg_addr_h;
    volatile U32 msg_data;        // Message Data
    volatile U32 vec_ctrl;        // Vector Control
};

struct hal_nvme_sq_pointer
{
    U16 fw_read;    /* Indicate the firmware read pointer of sq*/
    U16 head;        /* Indicate the head of SQ*/
};

struct hal_nvme_cq_info
{
    U32 base_addr_l;
    U32 base_addr_h;

    union
    {
        struct
        {
            U32 tail_entry_ptr:16;
            U32 pbit:1;
            U32 rsvd0:15;
        };

        U32 dword1;
    };

    U32 rsvd1;
};

struct hal_nvme_cq_msix_vector{
    U32 cq8:4;
    U32 cq7:4;
    U32 cq6:4;
    U32 cq5:4;
    U32 cq4:4;
    U32 cq3:4;
    U32 cq2:4;
    U32 cq1:4;
};

struct hal_wbq_time_cfg{
    U32 timeout_en:1;
    U32 timeout_any:1;
    U32 rsvd_0x11c8:30;

    U32 threshold;
    U32 start[2];
    U32 timeout[2];
};

struct hal_nvmecfg{
    struct nvme_bar bar;            // 0x0000-0x0037 NVMe BAR Space
    U8 rsvd_0x0038_0x0fff[4040];   // 0x0038-0x1000
    
    /*Doorbell 0 to doorbell 8. doorbell 0 is admin doorbel */
    struct hal_nvme_doorbell doorbell[9];    /* 0x1000 - 0x1047*/

    U8 rsvd_0x1048_0x104f[8];   // 0x01048-0x1050

    struct hal_nvme_msix_tab_entry msix_table[9];/* 0x1050 - 0x10df*/

    U32 msix_pba_pending_l;    /* 0x10e0*/
    U32 msix_pba_pending_h;
    U32 rsvd_0x10e8[2];

    U32 int_status;            /* 0x10f0 NVMe Interrupt Status*/
    U32 int_mask;                /* 0x10f4 NVMe Interrupt Mask*/
    U32 intvec_int_status;     /* 0x10f8 NVME Interrupt Vector Interrupt Status */

    U32 rsvd_0x10fc[1];        /* 0x10fc*/

    struct hal_nvme_sq_pointer sq_ptr[9];    /*0x1100*/
    U32 cq0_size;
    U32 cq_addr_inc_l;
    U32 cq_addr_inc_h:16;
    U32 rsvd_0x112e:16;

    struct hal_nvme_cq_info cq_info[9];        /*0x1130*/

    //struct hal_nvme_cq_msix_vector msix_vector; /* 0x11c0*/
    U32 msix_vector;

    U32 cmd_fetch_helper;              /* 0x11c4*/
    struct hal_wbq_time_cfg wbq_time_cfg;
    U16 cq_size[8];
    U16  cq_clr;
    U16  sq_clr;
};

#define NORMAL_SHN              (0x1)
#define ABRUPT_SHN              (0x2)

#define SHN_PROCESS_OCCURING    (0x1)
#define SHN_PROCESS_COMPLETE    (0x2)

typedef union _NVMe_CC
{
    struct
    {
        U32 EN         :1; 

        /* Bits 1-3 */
        U32 Reserved1  :3;
        U32 CSS        :3; 
        U32 MPS        :4; 
        U32 AMS        :3; 
        U32 SHN        :2; 

        U32 IOSQES     :4; 
        U32 IOCQES     :4; 

        /* Bits 24-31 */
        U32 Reserved2  :8;
    };
    U32 AsUlong;
} NVMe_CC, *PNVMe_CC;


typedef union _NVMe_CSTS
{
    struct
    {
        U32 RDY        :1; 
        U32 CFS        :1; 
        U32 SHST       :2; 

        /* Bits 4-31 */
        U32 Reserved   :28;
    };

    U32 AsUlong;
} NVMe_CSTS, *PNVMe_CSTS;


#ifdef __cplusplus
}
#endif
#endif /* HAL_AHCI_H_ */
