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
Filename    :HAL_SearchEngine.h
Version     :
Author      :Kristin Wang
Date        :2014.9
Description :header file for Search Engine driver.
Others      :
Modify      :
20140915    Kristin    Coding style uniform
*******************************************************************************/
#ifndef _HAL_SEARCHENGINE_H_
#define _HAL_SEARCHENGINE_H_
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

typedef enum _SE_ID
{
    SE0 = 0,
    SE1,
#ifndef VT3514_A0
    SE2,
#endif
    SE_NUM
}SE_ID;

typedef enum _SE_MSK_GROUP
{
    SE_1ST_MSK_GROUP = 0,
    SE_2ND_MSK_GROUP,
    SE_MSK_GROUP_NUM
}SE_MSK_GROUP;

#define SE_SEL_DRAM    0
#define SE_SEL_OTFB    1

#define SE_WAIT_TIME   40 //wait HW clear status register after triggering

/* SE registers define */
/* _n_ = 0 : SE0 */
/* _n_ = 1 : SE1 */
/* _n_ = 2 : SE2 */
#ifdef VT3514_A0
#define SE_BASE_ADDRESS(_n_)          (REG_BASE_SE0 + 0x100 * (_n_))
#else
#define SE_BASE_ADDRESS(_n_)          (((_n_) == SE2) ? REG_BASE_SE2 : (REG_BASE_SE0 + 0x100 * (_n_)))
#endif

#define rSE_START_ADDR(_n_)           (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x00))) //2-QW boundary
#define rSE_SEARCH_LEN_QW(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x04))) //in unit of QW
/* distance between two adjacent item
 * 0 - 1 QWORD.for example,item0 address is 0x40000000,item1 address is 0x40000008
 * 0xF - 16 QWORD
 * */
#define rSE_PITCH_SIZE(_n_)           (*((volatile U8*)(SE_BASE_ADDRESS(_n_) + 0x08)))

/* First group mask of SEn */
/* Selection Mask(MSK1):
 * if ( 0 == MSK1 )
 *     go on searching this item
 * else
 *     if ( 0 == (item & MSK1) )
 *         ignore this item
 *     else
 *         go on searching this item 
 * */
#define rSE_FMSK_SELECTION_LOW(_n_)      (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x0c)))
#define rSE_FMSK_SELECTION_HIGH(_n_)     (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x10)))

/* Condition Mask(MSK2):
 * if ( 0 == MSK2 )
 *     go on searching this item
 * else
 *     if ( (0 == condition type) && ( 0 != (item & MSK2) ) )
 *         ignore this item
 *     else if ( (1 == condition type) && ( 0 == (item & MSK2) ) )
 *         ignore this item
 *     else
 *         go on searching this item 
 * */
#define rSE_FMASK_CONDITION_LOW(_n_)     (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x14)))
#define rSE_FMASK_CONDITION_HIGH(_n_)    (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x18)))

/* Field Mask(MSK3):     
 * the search field is (item & MSK3) */
#define rSE_FMASK_FIELD_LOW(_n_)         (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x1c)))
#define rSE_FMASK_FIELD_HIGH(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x20)))

/* used for find-the-same, the value we want to search for */
#define rSE_FVALUE_SEARCHED_LOW(_n_)     (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x24)))
#define rSE_FVALUE_SEARCHED_HIGH(_n_)    (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x28)))

/* Second group mask of SEn*/
/* similar to First group mask */
#define rSE_SMSK_SELECTION_LOW(_n_)      (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x2c)))
#define rSE_SMSK_SELECTION_HIGH(_n_)     (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x30)))
#define rSE_SMASK_CONDITION_LOW(_n_)     (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x34)))
#define rSE_SMASK_CONDITION_HIGH(_n_)    (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x38)))
#define rSE_SMASK_FIELD_LOW(_n_)         (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x3c)))
#define rSE_SMASK_FIELD_HIGH(_n_)        (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x40)))
#define rSE_SVALUE_SEARCHED_LOW(_n_)     (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x44)))
#define rSE_SVALUE_SEARCHED_HIGH(_n_)    (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x48)))

/* bit 0 : trigger
 * bit [4:1] : item size  
 * bit [7:5] : DBG signal select
 * */
#define rSE_TRIGGER_SIZE_REG(_n_)        (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0x4c)))
typedef struct _SE_TRIGGER_SIZE_REG
{
    U8 bsTrigger: 1;
    U8 bsItemSize: 4;
    U8 bsDbgSel: 3;
}SE_TRIGGER_SIZE_REG;
typedef enum _SE_ITEM_SIZE
{
    SE_ITEM_SIZE_QWORD = 0,
    SE_ITEM_SIZE_DWORD,
    SE_SET_BIT0,
    SE_SET_BIT1,
    SE_SEARCH_MEM_BIT0,
    SE_SEARCH_MEM_BIT1,
    SE_SEARCH_MEM_BIT0_LOAD,
    SE_SEARCH_MEM_BIT1_LOAD,
    SE_SEARCH_REG_BIT0,
    SE_SEARCH_REG_BIT1,
    SE_SEARCH_REG_BIT0_LOAD,
    SE_SEARCH_REG_BIT1_LOAD,
    SE_SEARCH_OVERLAP
}SE_ITEM_SIZE;

/* bit [2:0] : search type of first group
 * bit 3 : condition type of first group
 * bit [6:4] : search type of second group
 * bit 7 : condition type of second group
 * */
#define rSE_TYPE_REG(_n_)                (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0x4d)))
typedef struct _SE_TYPE_REG
{
    U8 bsFGroupSType: 3;
    U8 bsFGroupCType: 1;
    U8 bsSGroupSType: 3;
    U8 bsSGroupCType: 1;
}SE_TYPE_REG;
#define SEARCH_TYPE_MAX                 (0)
#define SEARCH_TYPE_MIN                 (1)
#define SEARCH_TYPE_SAME                (2)

/* bit 0: 1-stable
 * bit 1: 1-find for first group
 * bit 2: 1-find for second group
 * bit 3: 1-bit search done
 * bit 4: 1-bit set done
 * bit 5 :search overlap done
 * bit [7:6]: reserved
 * */
#define rSE_STATUS_REG(_n_)              (*((volatile U8*)(SE_BASE_ADDRESS(_n_) + 0x4e)))
typedef struct _SE_STATUS_REG
{
    U8 bStable: 1;
    U8 bFGroupFind: 1;
    U8 bSGroupFind: 1;
    U8 bBitSearchDone: 1;
    U8 bBitSetDone: 1;
    U8 bOverlapDone: 1;
    U8 bsRsv: 2;
}SE_STATUS_REG;

/* Search result of find max/min/same */
#define rSE_FINDEX(_n_)                 (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x50)))    //in unit of item
#define rSE_RETURN_FVALUE_LOW(_n_)      (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x54)))
#define rSE_RETURN_FVALUE_HIGH(_n_)     (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x58)))
#define rSE_SINDEX(_n_)                 (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x60)))
#define rSE_RETURN_SVALUE_LOW(_n_)      (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x64)))
#define rSE_RETURN_SVALUE_HIGH(_n_)     (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x68)))

/* bit 0: 1 - enable XOR mask for first group
 * bit 1: 1 - enable XOR mask for second group */
#define rSE_XOR_MASK_ENABLE(_n_)        (*( (volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x6c)))
#define MSK_FGROUP_XOR_EN               (1 << 0)
#define MSK_SGROUP_XOR_EN               (1 << 1)

/* XOR Mask(MSK4, MSK5):
 * when XOR enable, the XOR field is (MSK4 & item)
 * if ( 0 == (item-XOR-field ^ MSK5) )
 *     go on searching this item
 * else
 *     ignore this item
 * */
#define rSE_FMASK_XOR_FIELD_LOW(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x70)))
#define rSE_FMASK_XOR_FIELD_HIGH(_n_)       (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x74)))
#define rSE_FMASK_XOR_VALUE_LOW(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x78)))
#define rSE_FMASK_XOR_VALUE_HIGH(_n_)       (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x7c)))
#define rSE_SMASK_XOR_FIELD_LOW(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x80)))
#define rSE_SMASK_XOR_FIELD_HIGH(_n_)       (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x84)))
#define rSE_SMASK_XOR_VALUE_LOW(_n_)        (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x88)))
#define rSE_SMASK_XOR_VALUE_HIGH(_n_)       (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x8c)))

/* parameter for bit search/set */
#define rSE_BIT_SET_ADDRESS(_n_)            (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x90)))    //search in memory
#define rSE_BIT_SEARCH_ADDRESS(_n_)         (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x94)))    //search in memory
#define rSE_BIT_SEARCH_DATA(_n_)            (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0x98)))    //search from register
#define rSE_BIT_SET_INDEX(_n_)              (*((volatile U16*)(SE_BASE_ADDRESS(_n_) + 0x9c)))    //in unit of bit
#define rSE_BIT_SEARCH_SIZE(_n_)            (*((volatile U16*)(SE_BASE_ADDRESS(_n_) + 0x9e)))    //in unit of QWORD

/* result of bit search */
#define rSE_MEM_BIT1_INDEX(_n_)             (*( (volatile U16*)(SE_BASE_ADDRESS(_n_) + 0xa0)))   //start from the search address,in unit of bit
#define rSE_MEM_BIT1_FIND(_n_)              (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xa2)))    //bit 0: 1-find
#define rSE_MEM_BIT0_INDEX(_n_)             (*( (volatile U16*)(SE_BASE_ADDRESS(_n_) + 0xa4)))
#define rSE_MEM_BIT0_FIND(_n_)              (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xa6)))
#define rSE_REG_BIT0_INDEX(_n_)             (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xa8)))
#define rSE_REG_BIT0_FIND(_n_)              (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xa9)))
#define rSE_REG_BIT1_INDEX(_n_)             (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xaa)))
#define rSE_REG_BIT1_FIND(_n_)              (*( (volatile U8*)(SE_BASE_ADDRESS(_n_) + 0xab)))
#define MSK_BIT_FIND                        (1 << 0)

/* search overlap related registers */
#define rSE_LIST_BASE_ADDRESS(_n_)          (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0xac)))
#define rSE_LIST_ITEM_NUM(_n_)              (*((volatile U16*)(SE_BASE_ADDRESS(_n_) + 0xb0)))    //in unit of QWORD
#define rSE_KEY_START_VALUE(_n_)            (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0xb4)))
#define rSE_KEY_ACCESS_LEN(_n_)             (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0xb8)))
#define SE_OVERLAP_BITMAP_LEMDW             (4)
#define rSE_OVERLAP_BITMAP(_n_,_m_)         (*((volatile U32*)(SE_BASE_ADDRESS(_n_) + 0xbc + (_m_)*sizeof(U32))))

typedef struct _SE_SEARCH_VALUE_PARAM
{
    /* DWORD 0 */
    U32 ulStartAddr;         //absolute address

    /* DWORD 1 */
    U32 ulItemCnt;          

    /* DWORD 2 */
    U32 ulSelMaskLow;        //Selection Mask(MSK1) low

    /* DWORD 3 */
    U32 ulSelMaskHigh;       //Selection Mask(MSK1) high

    /* DWORD 4 */
    U32 ulCondMaskLow;       //Condition Mask(MSK2) low

    /* DWORD 5 */
    U32 ulCondMaskHigh;      //Condition Mask(MSK2) high

    /* DWORD 6 */
    U32 ulFieldMaskLow;      //Field Mask(MSK3) low

    /* DWORD 7 */
    U32 ulFieldMaskHigh;     //Field Mask(MSK3) high

    /* DWORD 8 */
    U32 ulXorFieldMaskLow;   //the field of XOR Mask(MSK4) low

    /* DWORD 9 */
    U32 ulXorFieldMaskHigh;  //the field of XOR Mask(MSK4) high

    /* DWORD 10 */
    U32 ulXorValueMaskLow;   //the value of XOR Mask(MSK5) low

    /* DWORD 11 */
    U32 ulXorValueMaskHigh;  //the value of XOR Mask(MSK5) high

    /* DWORD 12 */
    U32 ulValToSearchLow;    //the value to search same low

    /* DWORD 13 */
    U32 ulValToSearchHigh;   //the value to search same high

    /* DWORD 14 */
    U8 ucPitchQW;
    U8 bsSearchType: 3;
    U8 bsCondType: 1;
    U8 bsItemSizeDW: 1;       //0 - item size is QWORD, 1 - item size is DWORD
    U8 bsXorEn: 1;
    U8 bsOTFB: 1;             //0 - DRAM, 1 - OTFB
    U8 bsRsv: 1;
    U8 aRsv[3];
}SE_SEARCH_VALUE_PARAM;

typedef struct _SE_SEARCH_VALUE_RESULT
{
    /* DWORD 0 */
    U32 ulIndex;

    /* DWORD 1 */
    U32 ulRtnValueLow;

    /* DWORD 2 */
    U32 ulRtnValueHigh;

    /* DWORD 3 */
    U16 bsFind: 1;
    U16 bsRsv: 15;
    U16 usRsv;
}SE_SEARCH_VALUE_RESULT;

typedef struct _SE_SEARCH_BIT_PARAM
{
    /* DWORD 0 */
    union
    {
        U32 ulStartAddr;     //absolute address,for search in memory
        U32 ulData;          //for search data in register
    };

    /* DWORD 1 */
    U16 usSizeQW;
    U16 bBitValue: 1;
    U16 bReg: 1;             //0 - search in memory, 1 - search in register
    U16 bOTFB: 1;            //if search in memory, 0 - DRAM, 1- OTFB
    U16 bLoad: 1;
    U16 bsRsv: 12;
}SE_SEARCH_BIT_PARAM;

typedef struct _SE_SEARCH_BIT_RESULT
{
    U16 usBitIndex;
    U16 bFind: 1;
    U16 bsRsv: 15;
}SE_SEARCH_BIT_RESULT;

typedef struct _SE_SET_BIT_PARAM
{
    /* DWORD 0 */
    U32 ulSetAddr;

    /* DWORD 1 */
    U16 usSetIndex;
    U16 bBitValue: 1;
    U16 bOTFB: 1;
    U16 bsRsv: 14;
}SE_SET_BIT_PARAM;

typedef struct _SE_SEARCH_OVERLAP_ENTRY
{
    /* DWORD 0 */
    U32 ulListAddr;                              //absolute address of list

    /* DWORD 1 */
    U32 ulListLenQW;

    /* DWORD 2 */
    U32 ulKeyStartVal;  

    /* DWORD 3 */
    U32 ulKeyLen;
    
    /* DWORD 4 ~ 7 */
    U32 aOverlapBitMap[SE_OVERLAP_BITMAP_LEMDW];  //the result, bit 1 means the item overlap with key range
}SE_SEARCH_OVERLAP_ENTRY;


void HAL_SEInit(void);
void HAL_SESearchValue( SE_SEARCH_VALUE_PARAM *ptValParam, SE_SEARCH_VALUE_RESULT *ptValResult );
void HAL_SESearchBit(SE_SEARCH_BIT_PARAM *pBitParam, SE_SEARCH_BIT_RESULT *pBitResult);
void HAL_SESetBit( SE_SET_BIT_PARAM *ptSetParam );
void HAL_SESearchOverlap(SE_SEARCH_OVERLAP_ENTRY *pOverlapEntry, BOOL bOTFB);

#endif //_HAL_SEARCHENGINE_H_
