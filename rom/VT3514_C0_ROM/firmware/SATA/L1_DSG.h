/******************************************************************************
*                   Copyright (C), 2012, VIA Tech. Co., Ltd.                  *
*        Information in this file is the intellectual property of VIA         *
*    Technologies, Inc., It may contains trade secrets and must be stored     *
*                          and viewed confidentially.                         *
 ******************************************************************************
  File Name     : L1_DSG.h
  Version       : Initial Draft
  Author        : Haven Yang
  Created       : 2013/8/29
  Description   : L1 DSG head file
  Description   : 
  Function List :
  History       :
  1.Date        : 2013/8/29
    Author      : Haven Yang
    Modification: Created file

******************************************************************************/
#ifndef _L1_DSG_H
#define _L1_DSG_H

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

 

enum
{
    CURRENT_DSG = 0,
    NEXT_DSG    = 1
};

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


 
extern U8 L1_GetBuffMapID(U8 DsgID);
extern U8 L1_GetReadDSGID(U8 CurOrNext);
extern U8 L1_GetWriteDSGID(U8 CurOrNext);
extern U8 L1_ProtocalSelect(U8 Protocol);

extern U32 L1_GetMemAddrByLBA(U32 LBA);

extern U32 HAL_SetFirstDSGID(U8 CmdTag, U8 DSGID);

extern BOOL L1_CheckDSGResource(SUBCMD* pSubCmd);
extern void HAL_UsedSataDSG(U8 ucDSGID);

extern U8  L1_GetSpecialReadDSG(void);
extern U8  L1_GetSpecialWriteDSG(void);

#endif


