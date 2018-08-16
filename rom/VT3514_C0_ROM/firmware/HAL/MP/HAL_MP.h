#ifndef _HAL_MP_H_
#define _HAL_MP_H_
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define PCIE_SIG            (0x6740)
#define REG_BASE_AHCI_MP    (REG_BASE_AHCI + 0xA0)
#define REG_BASE_NVME_MP    (REG_BASE_NVME + 0x200)

typedef struct MP_REG_SET
{
    U32 bsSig :16;
    U32 bsRes0 :15;
    U32 bsMpEn :1;

    U32 bsRes1_1 :7;
    U32 bsIntEn :1;
    U32 bsCmdType :8;
    U32 bsCmdStatus :8;
    U32 bsRes1_2 :7;
    U32 bsCmdTrig :1;

    union
    {
        U32 ulParam[4];
        struct ///    write
        {
            U32 ulDstAddr;
            U32 ulDataIn;
        };

        struct
        {
            U32 ulSrcAddr;
            U32 ulDataOut;
        };

        U32 ulExeEntry;
    };
}MP_REG_SET;

typedef enum MP_CMD_TYPE {
    MP_CMD_MEM_WR,
    MP_CMD_MEM_RD,
    MP_CMD_MEM_EXE
}MP_CMD_TYPE;

#define BIT_MP_INT      (1<<8)
#define BIT_HR_INT      (1<<16)

typedef union HOSTC_INTMSK_REG_
{
    struct{
        U32 bsRes1_0 :8;
        U32 bsIntMMptTrig :1;
        U32 bsRes1_1 :7;
        U32 bsIntMGhcHrSet:1;
        U32 bsRes1_2 :15;
    };
    U32 ulValue;
}HOSTC_INTMSK_REG;

typedef struct HOSTC_CTRL_REG_SET
{
    U32 bsMpsFw :3;
    U32 bsMpsSel :1;
    U32 bsMrrsFw :3;
    U32 bsMrrsSel :1;
    U32 bsAutoSlumberEn :1;
    U32 bsPartSlumberEn :1;
    U32 bsRes0_1 :6;
    U32 bsIntALPCnt :5;
    U32 bsRes0_3 :11;

    U32 bsDevMSel :2;
    U32 bsDevIntEn :1;
    U32 bsDevMSINoMsk :1;
    U32 bsRes1 :28;

}HOSTC_CTRL_REG_SET;

#define TEST_BIT(_x_, _b_)      (((_x_) & (_b_)) == (_b_))
#define rAHCI_GHC               (*(volatile U32*)(REG_BASE_AHCI + 0x04))
#define rP0_CMD                 (*(volatile U32*)(REG_BASE_AHCI + 0x118))
#define HR_BIT                  ( 1 )
#define FR_BIT                  ( 1 << 14 )
#define FRE_BIT                 ( 1 << 4 )


#endif

