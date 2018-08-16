/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :BaseDef.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    11:19:39
Description :
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


#ifndef BOOL
typedef S32 BOOL;
typedef S32 bool;
#endif

#ifndef FAIL
#define	FAIL    0
#endif

#ifndef SUCCESS
#define	SUCCESS	1
#endif

#ifndef FALSE
#define FALSE   FAIL
#endif

#ifndef TRUE
#define TRUE    SUCCESS
#endif

#define	GLOBAL	
#define	LOCAL	 static

/*long*/
#ifndef	INVALID_8F		
#define	INVALID_8F		0xFFFFFFFF
#endif

/*short*/
#ifndef	INVALID_4F		
#define	INVALID_4F		0xFFFF
#endif

/*char*/
#ifndef	INVALID_2F		
#define	INVALID_2F		0xFF
#endif

#ifndef 	MSK_F	
#define 	MSK_F		0xf

#define 	MSK_1F 		0xf
#define 	MSK_2F 		0xff
#define 	MSK_3F 		0xfff
#define 	MSK_4F 		0xffff
#define 	MSK_5F 		0xfffff
#define 	MSK_6F 		0xffffff
#define 	MSK_7F 		0xfffffff
#define 	MSK_8F 		0xffffffff
#endif

#define		SIM


#endif

