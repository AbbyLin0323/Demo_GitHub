/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : L2_Interface.h
* Discription  : 
* CreateAuthor : Via
* CreateDate   : 2016.3.17
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L2_INTERFACE_H
#define _L2_INTERFACE_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Disk_Config.h"
#include "L0_Interface.h"
#include "L2_Defines.h"
#include "L3_HostAdapter.h"
#include "L2_VBT.h"
#include "L2_PBIT.h"
#include "L2_StripeInfo.h"
#include "L3_Interface.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

#define BLOCK_TYPE_MIN_ERASE_CNT 0
#define BLOCK_TYPE_MAX_ERASE_CNT 1

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
//Shared Varibles Declare
extern GLOBAL MCU12_VAR_ATTR RED *g_pRedBaseAddr;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInOtfbBaseAddr;
extern GLOBAL MCU12_VAR_ATTR PuInfo *g_PuInfo[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL MCU12_VAR_ATTR U32 g_ulGBBT;

#define RED_OFFSET(McuId,TLun, Level)     ((((TLun)<<NFCQ_DEPTH_BIT) | (Level))*RED_PRG_SZ + ((McuId)-MCU1_ID)*(3<<15))
#ifdef RED_MAP_TO_DRAM
#define RED_RELATIVE_ADDR(McuId, TLun, Pri, Level)    ((RED_OFFSET((McuId), (TLun),(Level)))>>3)
#else
#define RED_RELATIVE_ADDR(TLun, Level)    ((RED_OFFSET((TLun),(Level)) + (U32)g_pRedBaseAddr - OTFB_START_ADDRESS)>>3)
#endif
#define RED_ABSOLUTE_ADDR(McuId, TLun, Level)    (RED_OFFSET((McuId), (TLun),(Level)) + (U32)g_pRedBaseAddr)
/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
U32 L3_FCmdMonitorTotalEraseCnt(void);
U32 L3_FCmdMonitorTotalWriteCnt(void);

//SharedMemMap Declare
void L1_SharedMemMap(SUBSYSTEM_MEM_BASE * pFreeMemBase);
void L2_SharedMemMap(SUBSYSTEM_MEM_BASE * pFreeMemBase);

//PBIT Interface Declare
void L2_PBIT_Set_Weak(U8 ucSuperPu, U16 VirBlockAddr, U8 bTrue);
void L2_PBIT_Set_Lock(U8 ucSuperPu, U16 VirBlockAddr, U8 bTrue);
BOOL L2_PBIT_Get_Lock(U8 ucSuperPu, U16 VirBlockAddr);
void L2_PBIT_Set_Backup(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_Error(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_Free(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_Table(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_Allocate(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Increase_EraseCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
void L2_PBIT_Decrease_AllocBlockCnt(U8 ucSuperPu, U8 ucBlkType);
U16  L2_PBIT_GetVirturlBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
void L2_PBIT_SetVirturlBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U16 VBN);
void L2_Exchange_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usSrcPhyBlock, U16 usTarPhyBlock);
void L2_Reset_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPhyBlock);
void L2_Set_PBIT_BlkSN_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U32 Value);
void L2_PBIT_Set_TLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
U8 L2_PBIT_IsTLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
void L2_VBT_Set_Free(U8 ucSuperPu, U16 VBN, U8 bTrue);
BOOL L2_VBT_Get_Free(U8 ucSuperPu, U16 VBN);
U8 L2_VBT_Get_TLC(U8 ucSuperPu, U16 VBN);

BOOL L2_IsSubPPOLUNWritten(U32 TargetOffsetBitmap, U8 ucLUNInSuperPU);
BOOL MCU12_DRAM_TEXT L2_IsTargetSuperBlkSubPageWritten(U8 ucSuperPU, U8 uLUNInSuperPU, U16 usVirBlk, U16 usVirPage);
U16  MCU12_DRAM_TEXT L2_GetTargetSuperBlkSubPPO(U8 ucTLun, U16 usVirBlk);
BOOL MCU12_DRAM_TEXT L2_IsClosedSuperBlk(U8 ucSuperPU, U16 usVirBlk);

U16  L2_VBT_GetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN);
void L2_VBT_SetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN, U16 PBN);

void L2_UpdatePMTPageNoDirtyTS(U32 PUSer, U32 PMT_TS, U32* p_DirtyBitMap);
LPNLookUPQuitCode L2_LookupPMT(PhysicalAddr* pAddr, U32 LPN, BOOL bRebuild);
LPNLookUPQuitCode L2_UpdatePMT(PhysicalAddr* pNewAddr, PhysicalAddr* pOriginalAddr, U32 ulLpn);
void L2_UpdateDirtyLpnMap(PhysicalAddr* pAddr, BOOL bValid);
void L2_IncreaseDirty(U16 ucSuperPu, U16 VirtualBlockSN, U32 DirtyCnt);
BOOL L2_LookupDirtyLpnMap(PhysicalAddr* pAddr);

void L2_BbtAddBbtBadBlk(U8 ucTLun, U16 usPhyBlk, U8 BadBlkType, U8 ucErrType);
void L2_BbtSetLunSaveBBTBitMap(U8 ucLun, BOOL BitValue);
BOOL L2_BbtIsGBbtBadBlock(U8 ucTLun, U16 usPhyBlk);

U16 L2_BM_BackUpBlockEmpty(U8 ucSuperPu, U8 ucLunInSuperPu);
U16 L2_BM_AllocateBackUpBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 AllocateType, U8 ucTLCBlk);
U16 L2_BM_AllocateFreeBlock(U8 ucSuperPu, U16 AllocateType, U8 ucTLCBlk);
U16 L2_BM_AllocateBrokenBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 AllocateType, U8 ucTLCBlk);

BOOL L2_BM_BrokenBlockEmpty(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucTLCBlk);
U32 L2_IsFlashPageSafed(PhysicalAddr* pAddr);

void L3_FCmdMonitorInit(void);

#endif
/*====================End of this head file===================================*/

