#ifndef _L2_READDISTURB_H
#define _L2_READDISTURB_H

#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"

//#define TARGET_BLK_READ_CNT_DEC                  (10)
#define CLOSED_BLK_READ_CNT_DEC                  (1)
#define BLK_READ_DISTURB_THS                     0//(TARGET_BLK_READ_CNT_DEC * PG_PER_SLC_BLK * 2)

#define READ_DISTURB_ERASE_CNT_THS_0            (500)
#define READ_DISTURB_ERASE_CNT_THS_1            (1000)
#define READ_DISTURB_ERASE_CNT_THS_2            (1500)
#define READ_DISTURB_ERASE_CNT_THS_3            (2000)
#define READ_DISTURB_ERASE_CNT_THS_4            (2500)
#define READ_DISTURB_ERASE_CNT_THS_5            (3000)
#define READ_DISTURB_ERASE_CNT_THS_6            (3500)

#ifdef SIM
#define PAGEPERBLK_DECIMAL_1HUNDRED_AMP         (PG_PER_BLK)
#define BLK_READ_CNT_INIT_VALUE_THS_0           (8 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_1           (7 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_2           (6 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_3           (5 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_4           (4 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_5           (3 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_6           (2 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_7           (1 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#else
#define PAGEPERBLK_DECIMAL_1HUNDRED_AMP         (100 * PG_PER_BLK)//Bics2/3: (PG_PER_BLK * PG_PER_WL); B0KB/B16A/B17A: PG_PER_BLK;
#define BLK_READ_CNT_INIT_VALUE_THS_0           (100 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_1           (80 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_2           (70 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_3           (60 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_4           (43 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_5           (30 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_6           (15 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#define BLK_READ_CNT_INIT_VALUE_THS_7           (8 * PAGEPERBLK_DECIMAL_1HUNDRED_AMP)
#endif

#define FORCE_GC_QUEUE_SIZE                     (BLK_PER_PLN)

typedef enum _BLKQ_LINK_TYPE
{
    LINK_TYPE_HOST = 0,
    LINK_TYPE_TLCW,
    LINK_TYPE_FREE
} BLKQ_LINK_TYPE;

typedef struct _BLK_QUEUE_ENTRY
{
    U16 VBN;
    U16 NextIndex;
}BLK_QUEUE_ENTRY;

typedef struct _BLK_QUEUE
{
    U16 usLength;
    BLK_QUEUE_ENTRY aVBN[FORCE_GC_QUEUE_SIZE];
    
    U16 usHOSTStartIndex;    /* VBT_TYPE_HOST BLK Link */
    U16 usHOSTEndIndex;
    U16 usHOSTCnt;

    U16 usTLCWStartIndex;    /* VBT_TYPE_TLCW BLK Link */
    U16 usTLCWEndIndex;      
    U16 usTLCWCnt;
    
    U16 usFreeStartIndex;           /* Free Link */
    U16 usFreeEndIndex; 
    U16 usFreeCnt;   
}BLK_QUEUE;

#if 0
typedef enum _TargetBlkGCState
{
    TARGET_BLK_GC_STATE_PREPARE,
    TARGET_BLK_GC_STATE_READ,
    TARGET_BLK_GC_STATE_WRITE,
    TARGET_BLK_GC_STATE_ERASE,
    TARGET_BLK_GC_STATE_WAIT_ERASE,
    TARGET_BLK_GC_STATE_ALL
}TargetBlkGCState;
#endif

extern GLOBAL  MCU12_VAR_ATTR BLK_QUEUE* g_pForceGCSrcBlkQueue[SUBSYSTEM_SUPERPU_MAX];

#ifdef SIM
extern GLOBAL  U32 g_aDbgForceGCTargetBlkCnt[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  U32 g_aDbgForceGCClosedBlkCnt[SUBSYSTEM_SUPERPU_MAX];
#endif

void L2_AddBlkQEntryToLinkTail(BLK_QUEUE* pQueue, BLKQ_LINK_TYPE BlkLInkType, U16 TargetLogIndex);
U16 L2_RemoveBlkQEntry(U8 ucSuperPu, BLK_QUEUE* pQueue, BLKQ_LINK_TYPE BlkLInkType, BOOL bDirtySLC);
void L2_BlkQueueInit(BLK_QUEUE *pQueue, U32 ulLength);
BOOL L2_BlkQueueIsEmpty(BLK_QUEUE *pQueue,  BLKQ_LINK_TYPE BlkQLinkType);
BOOL L2_BlkQueueIsFull(BLK_QUEUE *pQueue);
BOOL L2_BlkQueuePushBlock(U8 ucSuperPu, BLK_QUEUE *pQueue, U8 ucBlkType, U16 usVBN);
U16 L2_BlkQueuePopBlock(U8 ucSuperPu, BLK_QUEUE *pQueue, U8 ucBlkType, BOOL bDirtySLC);
void L2_BlkQueueRemoveAll(U8 ucSuperPu, BLK_QUEUE *pQueue);
void L2_ForceGCInit(void);
void L2_UpdateBlkReadCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usVBN);
void L2_ResetBlkReadCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPBN);
//BOOL L2_GCTargetBlkProcess(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucGCMode);
#endif


