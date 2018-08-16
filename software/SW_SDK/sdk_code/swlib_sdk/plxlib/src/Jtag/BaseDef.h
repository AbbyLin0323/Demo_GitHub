/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : BaseDef.h                                           
Version      :  v0.1                                                
Date         :   2008/5/21                                           
Author       :  Sue Wang

Description: A basic header file includes UBS and NFC interface functions.
Others: 
Modification History:
20080603	JackeyChai	001: Add define of GLOBAL and LOCAL
20080617 	peterlgchen	001: add define S32, S16, S8 for signed data
20081126	peterlgchen 	002: Change File Name to BaseDef.h
*************************************************/
//some type definitions
#ifndef __SSDDEF_H
#define __SSDDEF_H

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	 U32;

typedef long int S32;
typedef int 	S16;
typedef char S8;

/*
#ifndef BOOL
#define	BOOL	unsigned int
#endif

#ifndef FAIL
#define	FAIL		0
#endif

#ifndef SUCCESS
#define	SUCCESS	1
#endif

#ifndef FALSE
#define FALSE 	FAIL
#endif

#ifndef TRUE
#define TRUE	SUCCESS
#endif

#define	GLOBAL	
#define	LOCAL	 static

#ifndef	INVALID_8F		//long
#define	INVALID_8F		0xFFFFFFFF
#endif

#ifndef	INVALID_4F		//short
#define	INVALID_4F		0xFFFF
#endif

#ifndef	INVALID_2F		//char
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

//#ifdef	SIMULATION
#define	DBG_//dbg_printf(x, ...)	//dbg_printf(x, __VA_ARGS__)
//#else
//#define	DBG_//dbg_printf(x,...)		
//#endif

// avoid to include <windows.h>
/*  
typedef void *	HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (HANDLE)-1
#endif
*/
#define	GLOBAL	
#define	LOCAL	 static

#define SIM_WIN
void dbg_printf(const char *fmt, ...);
#endif//#ifndef __SSDDEF_H
