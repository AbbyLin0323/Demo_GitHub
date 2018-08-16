/*************************************************
*Copyright (C), 2009, VIA Tech. Co., Ltd.
*Filename: defines.h                                            
*Version: 0.0.1                                                 
*Date: 2009-08-20                                            
*Author: cloudszhang
*
*Description: define all the types
*
*Modification History:
*cloudszhang,       090820,      updated from BaseDef.h
*************************************************/
#ifndef __DEFINES_H__
#define __DEFINES_H__

/*some type definitions*/
typedef unsigned char    U8;
typedef unsigned short   U16;
typedef unsigned int     U32;
typedef int              S32;
typedef short            S16;
typedef char             S8;


#ifndef BOOL
typedef S32 BOOL;
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


#ifndef NULL
#define NULL 0
#endif

#define	DBG_printf(x, ...)	printf(x, __VA_ARGS__)
#define	DBG_Getch	getchar

#endif

