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
Filename    :BaseDef.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    11:19:39
Description : Basic data type definition and macros for enabling/disabling features supported.
Others      :updated from BaseDef.h
Modify      :
****************************************************************************/
#ifndef __BASE_DEF_H__
#define __BASE_DEF_H__

/*some type definitions*/
typedef unsigned char    U8;
typedef unsigned short   U16;
typedef unsigned int     U32;
typedef int              S32;
typedef short            S16;
typedef char             S8;

/* quaword(64bit unsigned integer) type definition */
typedef struct _QWORD
{
    U32 LowDw;
    U32 HighDw;
}QWORD;

#ifndef BOOL
typedef S32 BOOL;
#endif

#ifndef FAIL
#define    FAIL    0
#endif

#ifndef SUCCESS
#define    SUCCESS    1
#endif

#ifndef FALSE
#define FALSE   FAIL
#endif

#ifndef TRUE
#define TRUE    SUCCESS
#endif

#define    GLOBAL    
#define    LOCAL     static

/*long*/
#ifndef    INVALID_8F        
#define    INVALID_8F        0xFFFFFFFF
#endif

/*short*/
#ifndef    INVALID_4F        
#define    INVALID_4F        0xFFFF
#endif

/*char*/
#ifndef    INVALID_2F        
#define    INVALID_2F        0xFF
#endif

#ifndef     MSK_F    
#define     MSK_F        0xf

#define     MSK_1F         0xf
#define     MSK_2F         0xff
#define     MSK_3F         0xfff
#define     MSK_4F         0xffff
#define     MSK_5F         0xfffff
#define     MSK_6F         0xffffff
#define     MSK_7F         0xfffffff
#define     MSK_8F         0xffffffff
#endif


#ifndef NULL
#define NULL 0
#endif

#ifndef EVEN
#define EVEN 0
#endif

#ifndef ODD
#define ODD 1
#endif

/* DWORD size in Byte is 4 */
#define DWORD_SIZE_BITS   2
#define DWORD_SIZE        (1 << DWORD_SIZE_BITS)

/* BYTE size in Bit is 8 */
#define BYTE_BIT_SIZE_BITS    3
#define BYTE_BIT_SIZE        (1 << BYTE_BIT_SIZE_BITS)
#define BYTE_BIT_SIZE_MSK    (BYTE_BIT_SIZE - 1)

/* DWORD size in Bit is 32 */
#define DWORD_BIT_SIZE_BITS    5
#define DWORD_BIT_SIZE        (1 << DWORD_BIT_SIZE_BITS)
#define DWORD_BIT_SIZE_MSK    (DWORD_BIT_SIZE - 1)

/* 16 DWORD size in Byte is 64 */
#define   SIXTEENDW_SIZE_BITS        (6)  
#define   SIXTEENDW_SIZE             (1 << SIXTEENDW_SIZE_BITS)      
#define   SIXTEENDW_SIZE_MSK         (SIXTEENDW_SIZE - 1)

/* 32K Bytes size in Byte is 0x8000 */
#define   THIRTYTWOKB_SIZE_BITS        (15)  
#define   THIRTYTWOKB_SIZE             (1 << THIRTYTWOKB_SIZE_BITS)      
#define   THIRTYTWOKB_SIZE_MSK         (THIRTYTWOKB_SIZE - 1)

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define BIT(n)              (1<<(n))
#define ByteToDWord(n)      (((n)+3)>>2)
#define BYTE_0(dw)          ((dw) & 0xff)
#define BYTE_1(dw)          (((dw)>>8) & 0xff)
#define BYTE_2(dw)          (((dw)>>16) & 0xff)
#define BYTE_3(dw)          (((dw)>>24) & 0xff)

#define GET_BITS(_x_, _offset_, _m_)    ((( _x_ ) >> (_offset_)) & (_m_))
#define TEST_BIT(_x_, _b_)    (((_x_) & (_b_)) == (_b_))

/* include project configuration setting */
#include "Proj_Config.h"

#endif/*__BASE_DEF_H__*/
