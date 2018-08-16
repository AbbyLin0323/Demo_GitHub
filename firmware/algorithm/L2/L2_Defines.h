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
Filename    :L2_Defines.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :defines for algorithm firmware.
Others      :
Modify      :
****************************************************************************/
#ifndef __L2_DEFINES_H__
#define __L2_DEFINES_H__
#include "BaseDef.h"
#include "Disk_Config.h"
#include "HAL_MemoryMap.h"
#include "HAL_TraceLog.h"


typedef enum BlkType_Tag
{
    BLKTYPE_SLC = 0,    //Total free blk cnt also use this field
    BLKTYPE_TLC,
    BLKTYPE_ALL
}BlkType;

typedef enum TargetType_Tag
{
    TARGET_HOST_WRITE_NORAML = 0,   //SLC
    TARGET_HOST_GC,                 //SLC
    TARGET_HOST_ALL,
    TARGET_TLC_WRITE = TARGET_HOST_ALL,               //TLC
    TARGET_ALL
}TargetType;

typedef enum FTL_THREAD_TYPE_TAG
{
    FTL_THREAD_SLCGC = 0,
    FTL_THREAD_TLCGC,
    FTL_THREAD_GC_2TLC,
    FTL_THREAD_WL,
    FTL_THREAD_WRITE,
    FTL_THREAD_TYPE_ALL,
    FTL_THREAD_NO_TLC_FREEBLK
}FTL_THREAD_TYPE;

//status for write entyr
typedef enum WriteStatus_Tag
{
    WRITE_STATUS_OK = 0,
    WRITE_STATUS_CE_ALL_BUSY,
    WRITE_STATUS_NO_DATA_PAGE,
    WRITE_STATUS_ALL
}WriteStatus;

//firmware state
typedef enum SYSTEM_STATE_TAG
{
    SYSTEM_STATE_NORMAL,
    SYSTEM_STATE_BOOT_BLOCKING,
    SYSTEM_STATE_BOOT_PENDING,
    SYSTEM_STATE_ERROR_HANDLING,
    SYSTEM_STATE_ALL
}SYSTEM_STATE;

typedef enum SYS_THREAD_TYPE_TAG
{
    SYS_THREAD_FTL = 0,                         //host write, data GC, data SWL
    SYS_THREAD_ERROR_HANDLING,      //error handling
    SYS_THREAD_TABLE_PMT,           //table write or GC/WL
    SYS_THREAD_BOOT,                //table rebuild or boot up
    SYS_THREAD_TYPE_ALL
}SYS_THREAD_TYPE;

typedef enum CurSWLStatus_Tage
{
    CURSTATUS_SWL_PENDING,
    CURSTATUS_SWL_RUNNING,
    CURSTATUS_SWL_FREE
}CurSWLStatus;

typedef enum SWLMode_Tag
{
    SWL_TLC_COPYALL,
    SWL_MODE_ALL
}SWLMode;

//status for SWL state
typedef enum SWLState_Tag
{
    SWL_STATE_PERPARE,
    SWL_STATE_LOAD_RPMT,
    SWL_STATE_WAIT_RPMT_STATUS,
    SWL_STATE_READ,
    SWL_STATE_WAIT_READ_STATUS,
    SWL_STATE_REBUILD_RPMT,
    SWL_STATE_WRITE,
    SWL_STATE_WAIT_WRITE_STATUS,
    SWL_STATE_ERASE,
    SWL_STATE_WAIT_ERASE,
    SWL_STATE_TLC_GC2SLC,
    SWL_STATE_TLC_WAIT_HOSTGC2TLC,
    SWL_STATE_TLC_WRITE,
    SWL_STAGE_ERRH_PROG,
    SWL_STATE_TLC_DONE,
    SWL_STATE_ALL
}SWLState;

//status for GC state
typedef enum GCState_Tag
{
    GC_STATE_PREPARE_GC,
    GC_STATE_SELECT_TLCGC_SRC,
    //GC_STATE_TARGET_BLK_PRO,
    GC_STATE_LOAD_RPMT,
    GC_STATE_ERROR_HANDLING,
    GC_STATE_COPY_VALID,
    GC_STATE_WAIT_READ_STATUS,
    GC_STATE_WRITE,
    GC_STATE_WAIT_WRITE_STATUS,
    GC_STATE_ERRH_PROG,
    GC_STATE_ERASE,
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    GC_STATE_SHUTDOWN_SHARED_PAGE_CLOSED,
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    GC_STATE_SHUTDOWN_TLCBLK_ERASE,
    GC_STATE_SHUTDOWN_WAIT_TLCBLK_ERASE,
#endif
#ifdef L2_HANDLE_UECC
    GC_STAGE_ERRH_UECC,
#endif
    GC_STATE_ALL
}GCState;

typedef enum BootStatus_Tag
{
    BOOT_STATUS_REBUILD,
    BOOT_STATUS_NORMAL,
    BOOT_STATUS_ALL
}BootStatus;

typedef enum L2_FTL_STATUS_TAG
{
    FTL_STATUS_NO_CMD = 0,
    FTL_STATUS_NO_DATA_IN_BUFFER,//if all the LPN in one buffer is INVALID_8F, return this value.
    FTL_STATUS_NO_FREE_PAGE,//if no free page left when write, return this value
    FTL_STATUS_CMD_PENDING,
    FTL_STATUS_NEED_SCHEDULER,
    FTL_STATUS_TABLE_MANAGEMENT, //Table Miss
    FTL_STATUS_ROLL_BACK,
    FTL_STATUS_OK,
    FTL_STATUS_ALL
}L2_FTL_STATUS;


typedef enum L2_GC_STATUS_TAG
{
    GC_STATUS_L3_QUEUE_BUSY,
    GC_STATUS_NO_NEED_GC,
    GC_STATUS_PROCESS_READ,
    GC_STATUS_DONE,
    GC_STATUS_OK,
    GC_STATUS_ALL
}L2_GC_STATUS;

typedef enum _TLCStage
{
    TLC_STAGE_PRPARE,
    TLC_STAGE_ERRH_RPMT,    // read RPMT fail error handling
    TLC_STAGE_ERRH_PROG,    // TLC program error handling
    TLC_STAGE_LOADRPMT,
    TLC_STAGE_CHECK_RPMTSTATUS,
    TLC_STAGE_READSRC,
    TLC_STAGE_CHECK_READSTATUS,
    TLC_STAGE_WRITETLC,
    TLC_STAGE_DONE,
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    TLC_STAGE_SHUTDOWN_SHARED_PAGE_CLOSED,
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    TLC_STAGE_SHUTDOWN_TLCBLK_ERASE,
    TLC_STAGE_SHUTDOWN_WAIT_TLCBLK_ERASE,
#endif
#ifdef L2_HANDLE_UECC
    TLC_STAGE_ERRH_UECC,
#endif
    TLC_STAGE_ALL
}TLCStage;

typedef enum _TLCErrStage
{
    TLC_ERRH_ALLOCATE_NEWBLK,
    TLC_ERRH_SAVE_BBT,
    TLC_ERRH_WAIT_BBT,
    TLC_ERRH_ERASE,
    TLC_ERRH_ALL
}TLCErrH;

typedef enum _TLCWriteType
{
    TLC_WRITE_HOSTWRITE,
    TLC_WRITE_SWL,
    TLC_WRITE_TLCGC,
    TLC_WRITE_ALL
}TLCWriteType;

typedef enum TableRWType_Tag
{
    TABLE_READ = 0,
    TABLE_WRITE,
    TABLE_RW_ALL
} TableRWType;

typedef enum TableRWStatus_Tag
{
    TABLE_STATUS_BEGIN = 0,
    TABLE_STATUS_GC,
    TABLE_STATUS_WL,
    TABLE_STATUS_LOAD,
    TABLE_STATUS_FLUSH,
    TABLE_STATUS_WAIT,
    TABLE_STATUS_POP_STATE,
    TABLE_STATUS_OVER
} TableRWStatus;

typedef enum TableQuitStatus_Tag
{
    TABLE_QUIT_STAGE_DONE = 0,
    TABLE_QUIT_PREPROCESS_UNDONE,
    TABLE_QUIT_GC_UNDONE,
    TABLE_QUIT_WL_UNDONE,
    TABLE_QUIT_RW_UNDONE,
    TABLE_QUIT_WAIT_UNDONE,
    TABLE_QUIT_FLASH_ERROR
} TableQuitStatus;

typedef enum LPNLookUPQuitCode_Tag
{
    LPN_LOOKUP_SUCCESS = 0,
    LPN_LOOKUP_FAIL
} LPNLookUPQuitCode;


typedef enum RebuildState_Tag
{
    Rebuild_State_Init = 0,
    Rebuild_State_RebuildTB,
    Rebuild_State_RebuildPMTI,
    Rebuild_State_RebuildPMTManager,
    Rebuild_State_RebuildPu,
    Rebuild_State_RebuildPMT,
    Rebuild_State_FullfillPartialPMTPPO,
    Rebuild_State_ErrHandling,
    Rebuild_State_Done,
    Rebuild_State_RebuildDirtyCnt,
    Rebuild_State_All
}RebuildState;

typedef enum
{
    Rebuild_FullfillPartialPMTPPO_Write,
    Rebuild_FullfillPartialPMTPPO_WaitStatus
}RebuildFullfillPartialPMTPPOState;

typedef enum RebuildPMTIState_Tag
{
    Rebuild_PMTI_Prepare = 0,
    Rebuild_PMTI_LoadPMTSpare,
    Rebuild_PMTI_CheckUECCCnt,
    Rebuild_PMTI_EraseTooManyUECCBlock,
    Rebuild_PMTI_UpdatePMTI,
    Rebuild_PMTI_Done,
    Rebuild_PMTI_All
}RebuildPMTIState;

typedef enum RebuildPMTState_Tag
{
    Rebuild_PMT_Prepare = 0,
    Rebuild_PMT_LoadPMT,
    Rebuild_PMT_LoadFirstRPMT,
    Rebuild_PMT_RebuildPMT,
    Rebuild_PMT_RebuildErrTLC,
    Rebuild_PMT_Done,
    Rebuild_PMT_All
}RebuildPMTState;

typedef enum REBUILD_PMT_STAGE_TAG
{
    REBUILD_PMT_STAGE_LOAD_RPMT = 0,
    REBUILD_PMT_STAGE_WAIT_RPMT,
    REBUILD_PMT_STAGE_REBUILD,
    REBUILD_PMT_STAGE_DONE,
    REBUILD_PMT_STAGE_ALL
}REBUILD_PMT_STAGE;

typedef enum RebuildPuState_Tag
{
    Rebuild_Pu_Prepare = 0,
    Rebuild_SeqPPO,
    Rebuild_RndPPO,
    Rebuild_Pu_Done
}RebuildPuState;

typedef enum RebuildErrTLC_Tag
{
    Rebuild_ErrTLC_LoadTLCPage = 0,
    Rebuild_ErrTLC_WaitStautsOfTLCPage,
    Rebuild_ErrTLC_LookUpOldMapping,
    Rebuild_ErrTLC_WaitLoadDestBlk,
    Rebuild_ErrTLC_RebuildPMT,
    Rebuild_ErrTLC_LoadNextTLCPage,
    Rebuild_ErrTLC_Done
}RebuildErrTLCState;

typedef enum RebuildPuPPOState_Tag
{
    Rebuild_PuPPO_Read = 0,
    Rebuild_PuPPO_WaitStatus,
}RebuildPuPPOState;


typedef enum RebuildErrHandleState_Tag
{
    Rebuild_ErrHandle_Prepare = 0,
    Rebuild_ErrHandle_MoveBlkProcess,
    Rebuild_ErrHandle_HandleRndTargetBlkProcess,
    Rebuild_ErrHandle_Done
}RebuildErrHandleState;

typedef enum RebuildMoveBlkStage_Tag
{
    Rebuild_MoveBlk_Read = 0,
    Rebuild_MoveBlk_WaitRStatus,
    Rebuild_MoveBlk_ReadPreLowPage,
    Rebuild_MoveBlk_WaitReadPreLowPageStatus,
    Rebuild_MoveBlk_ReadNextPageRed,
    Rebuild_MoveBlk_WaitReadNextPageRedStatus,
    Rebuild_MoveBlk_Write,
    Rebuild_MoveBlk_WaitWStatus,
    Rebuild_MoveBlk_Erase,
    Rebuild_MoveBlk_Done
}RebuildMoveBlkStage;

typedef enum RebuildHandleTargetBlkStage_Tag
{
    Rebuild_HandleTargetBlk_Prepare = 0,
    Rebuild_HandleTargetBlk_Read,
    Rebuild_HandleTargetBlk_WaitRStatus,
    Rebuild_HandleTargetBlk_Write,
    Rebuild_HandleTargetBlk_WaitWStatus,
    Rebuild_HandleTargetBlk_Done
}RebuildHandleTargetBlkStage;

typedef enum RebuildCalcDirtyCnt_Tag
{
    Rebuild_CalcDirtyCnt_Prepare = 0,
    Rebuild_CalcDirtyCnt_Process,
    Rebuild_CalcDirtyCnt_Done
}RebuildCalcDirtyCntState;

typedef enum _RebuildCalcDirtyCntStage_
{
    Rebuild_DirtyCnt_Bootup = 0,
    Rebuild_DirtyCnt_Wait_L3_Idle,
    Rebuild_DirtyCnt_Adjust,
    Rebuild_DirtyCnt_Flush_PMT
}RebuildCalcDirtyCntStage;

typedef enum RebuildReadRPMT_Tag
{
    Rebuild_ReadRPMT = 0,
    Rebuild_ReadRPMT_WaitStatus,
    Rebuild_ReadRPMT_ErrHandle,
    Rebuild_ReadRPMT_Done,
    Rebuild_Calc_One_Blk_Done,
}RebuildReadRPMTState;

typedef enum RebuildLoadSpare_Tag
{
    RebuildLoadSpare_Read = 0,
    RebuildLoadSpare_WaitStatus,
    RebuildLoadSpare_Done,
}RebuildLoadSpareState;

typedef enum NeedRebuildFlag_Tag
{
    NOT_BYPASS = 0,    //lun need to join rebuild
    BYPASS,            //lun don't need to join rebuild
    RPMT_UECC_BYPASSS        //TLC lun any one RPMT UECC, by pass and rebuild later
}NeedRebuildFlag;

#define CEIL(x,y)                   (((x)+((y)-1))/(y))
#define BBT_TLUN_MSK_SIZE           CEIL(SUBSYSTEM_LUN_MAX,32)
typedef struct _GBBT_
{
    U32 ulBbtMark;   //record the bbt version
    U32 bsGBbtTLun : 8;
    U32 bsGBbtPage : 16;
    U32 bsGBbtBlk : 7;
    U32 bsFormatGBBT : 1;
    U32 ulMaxBbtSn;
    U32 aLunMskBitMap[BBT_TLUN_MSK_SIZE];
}GBBT_INFO;

/* Redundant defines */
typedef union _REDUNDANT
{
    U32 m_Content[RED_SW_SZ_DW];
    struct
    {
        /* Common Info : 3 DW */
        RedComm m_RedComm;

        /* private data */
        union
        {
            /* Global Block */
            U32 SubPageType;

            /* BBT */
            struct
            {
                U32 bbt_sn;
                U32 pu_msk[2];
            };

            /* Root Table */
            struct
            {
                BOOL bPowerCycle;
            };

            /* AT0 Table */
            U32 m_PageIndex;

            /* AT1 PMT Table */
            struct
            {
                U32 m_PMTIndex;
#ifdef PMT_ITEM_SIZE_REDUCE
                U32 m_DirtyBitMap[PMT_DIRTY_BM_SIZE_MAX];
#else
				U32 m_DirtyBitMap[PMT_DIRTY_BM_SIZE];
#endif
                U32 m_ValidLPNCountSave;
            };

            /* Trace Block */
            struct
            {
                U32 m_McuId;
                U32 m_TraceSN;
                U32 m_TraceItemCNT;
                TL_INFO m_TLInfo;
            };

            /* BBT Red */
            GBBT_INFO m_tGBbtInfo;

            /* Data Block */
            DataRed m_DataRed;            
        };
    };
}RED;

typedef union PhysicalAddr_Tag
{
    U32 m_PPN;
	U8 m_BytePPN[4];
    struct
    {
#if defined (FLASH_TLC)
    #define BLOCK_PER_PU_BITS 12
    #define PAGE_PER_BLOCK_BITS 9
    #define LPN_PER_PAGE_BITS 3
		U32 m_LPNInPage : 3;
		U32 m_PageInBlock : 9;
        U32 m_BlockInPU : 12;        
        U32 m_OffsetInSuperPage : 2;       
		U32 m_PUSer : 6;

#elif defined(FLASH_INTEL_3DTLC)

#ifdef FLASH_IM_3DTLC_GEN2

#ifdef PMT_ITEM_SIZE_REDUCE
    #define BLOCK_PER_PU_BITS 9 //here only support 0-500(virtual address range) for L2, but actual physical address range is 0~504
    #define PAGE_PER_BLOCK_BITS 12
#ifdef FLASH_IM_3DTLC_GEN2_B17A
    #define LPN_PER_PAGE_BITS 4
#else
    #define LPN_PER_PAGE_BITS 3
#endif
#endif
    #ifdef FLASH_IM_3DTLC_GEN2_B17A
    U32 m_LPNInPage : 4;//4PLN - 64KB (4KB*16)
    #else
    U32 m_LPNInPage : 3;//4PLN - 32KB (4KB*8)
    #endif
    U32 m_PageInBlock : 12;//0~2303
    U32 m_BlockInPU : 9;//here support actual physical address range is 0~504
    #ifdef FLASH_IM_3DTLC_GEN2_B17A
    U32 m_OffsetInSuperPage : 3;//0~7 (1:8 LUNs == 64PLNs for XOR Operation)
    #else
    U32 m_OffsetInSuperPage : 4;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
    #endif
    U32 m_PUSer : 4;//SuperPU 0~15

#else //end of FLASH_IM_3DTLC_GEN2

#ifdef PMT_ITEM_SIZE_REDUCE
    #define BLOCK_PER_PU_BITS 9 //here only support 0-512(virtual address range) for L2, but actual physical address range is 0~547
    #define PAGE_PER_BLOCK_BITS 11
    #define LPN_PER_PAGE_BITS 4
#endif
		U32 m_LPNInPage : 4;//4PLN - 64KB (4KB*16)
		U32 m_PageInBlock : 11;//0~1535
#if(LUN_NUM_PER_SUPERPU == 3)
        U32 m_OffsetInSuperPage : 2;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
        U32 m_BlockInPU : 10;//here support actual physical address range is 0~547
		U32 m_PUSer : 1;//SuperPU 0~7
        U32 m_Rsvd : 4;
#elif(LUN_NUM_PER_SUPERPU == 6)
        U32 m_OffsetInSuperPage : 3;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
        U32 m_BlockInPU : 10;//here support actual physical address range is 0~547
        U32 m_PUSer : 1;//SuperPU 0~7
        U32 m_Rsvd : 3;
#elif(LUN_NUM_PER_SUPERPU == 8)
        U32 m_OffsetInSuperPage : 3;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
        U32 m_BlockInPU : 10;//here support actual physical address range is 0~547
        U32 m_PUSer : 1;//SuperPU 0~7
        U32 m_Rsvd : 3;
#elif((LUN_NUM_PER_SUPERPU == 12) || (LUN_NUM_PER_SUPERPU == 16))
#ifndef IM_3D_TLC_1TB
        U32 m_OffsetInSuperPage : 4;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
        U32 m_BlockInPU : 10;//here support actual physical address range is 0~547
        U32 m_PUSer : 1;//SuperPU 0~7
        U32 m_Rsvd : 2;
#else
        U32 m_OffsetInSuperPage : 4;//0~15 (1:16 LUNs == 64PLNs for XOR Operation)
        U32 m_PUSer : 1;//SuperPU 0~7
        U32 m_BlockInPU : 10;//here support actual physical address range is 0~547
        U32 m_Rsvd : 2;
#endif
#endif
#endif //end of else of FLASH_IM_3DTLC_GEN2
#else
    #if (((BLK_PER_PLN + RSV_BLK_PER_PLN) > 1024) && (LOGIC_PG_PER_BLK_BITS <= 8)) //For TSB chip 

        #if (LOGIC_PIPE_PG_SZ == (32*1024)) // For TSB 2pln,15nm has 2048 Blocks
		U32 m_LPNInPage : 3;
		U32 m_PageInBlock : 8;
		U32 m_BlockInPU : 12;
        U32 m_OffsetInSuperPage : 1;

    #define BLOCK_PER_PU_BITS 12
    #define PAGE_PER_BLOCK_BITS 8
    #define LPN_PER_PAGE_BITS 3
#elif (LOGIC_PIPE_PG_SZ == (64*1024)) //For TSB 4pln
		U32 m_LPNInPage : 4;
		U32 m_PageInBlock : 8;
		U32 m_BlockInPU : 11;
        U32 m_OffsetInSuperPage : 1;

    #define BLOCK_PER_PU_BITS 11
    #define PAGE_PER_BLOCK_BITS 8
    #define LPN_PER_PAGE_BITS 4
#else //For YMTC
		U32 m_LPNInPage : 2;
		U32 m_PageInBlock : 8;
		U32 m_BlockInPU : 12;
        U32 m_OffsetInSuperPage : 2;

    #define BLOCK_PER_PU_BITS 12
    #define PAGE_PER_BLOCK_BITS 8
    #define LPN_PER_PAGE_BITS 2
        #endif

    #else //For L85/L95 flash,L95 has 24 Rsv Blk.
		U32 m_LPNInPage : 3;
		U32 m_PageInBlock : 9;
		U32 m_BlockInPU : 11;
        U32 m_OffsetInSuperPage : 1;

#define BLOCK_PER_PU_BITS 11
#define PAGE_PER_BLOCK_BITS 9
#define LPN_PER_PAGE_BITS 3
    #endif
		U32 m_PUSer : 8;
#endif

    };
}PhysicalAddr;

typedef struct _XOR_PARAMETER
{
    U32 bsXorEn          : 1;
    U32 bsXorStripeId    : 4;
    U32 bsContainXorData : 1;
    U32 bsReserved       : 26;
}XOR_PARAM;
/*----------------------------------------------*
 * L2 feature macros define                     *
 *----------------------------------------------*/

#define TrimBufferSize 16

typedef struct L2TrimShareData_t
{
    /* DW0 */
    U8 ucL2TrimStatus;
    U8 ucL3TrimStatus;
    S8  sWritePoint;
    S8  sReadPoint;

    /*DW1-2*/
    U32 ulL2TrimTotal;
    U32 ulL3TrimTotal;

    /*DW3-19*/
    U32 ulTrimPhyAddr[TrimBufferSize];//12

    U16 usPMTIIndexInPu[TrimBufferSize];

    U8 ucPu[TrimBufferSize];
    U32 ulTrimLPNMap[4];
    U32 ulDirtyBitMapAddr[4];

#ifdef DirtyLPNCnt_IN_DSRAM1
    U32 ulDirtyLPNCntAddr;
#endif
#ifdef ValidLPNCountSave_IN_DSRAM1
    U32 ulValidLPNCountSaveL_addr; 
    U32 ulValidLPNCountSaveH_addr; 
#else
    U32 ulValidLPNCountSaveAddr[4];
#endif
}L2TrimShareData;

typedef enum TrimStatus_Tag
{
    Trim_L2_Start = 0,
    Trim_L2_Run,//1
    Trim_L2_Wait_L3,//2
    Trim_L2_Finish,//3
    Trim_L3_Run,//4
    Trim_L3_Wait_L2,//5
    Trim_L3_Finish,//6
    Trim_Finish//7
}TrimStatus;

#ifdef SIM
extern MCU12_VAR_ATTR volatile L2TrimShareData *g_pShareData;
#else
extern MCU12_VAR_ATTR L2TrimShareData *g_pShareData;
#endif

#endif
