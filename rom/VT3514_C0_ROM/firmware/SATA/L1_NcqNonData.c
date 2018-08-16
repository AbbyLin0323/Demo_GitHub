/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : L1_NcqNonData.c
* Discription  : for the purpose of supporting NCQ NON-DATA command in SATA V3.2
*                this file support NCQ NON-DATA command 0x63 and all of the 
*                subcommands which defined in sata 3.2 as following
*                00h: Abort NCQ Queue
*                01h: Deadline handling
*                02h: Hybrid demote by size
*                03h: Hybrid change by LBA range
*                04h: Hybrid control
*                05h: Set features
* CreateAuthor : Haven Yang
* CreateDate   : 2013.11.28
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L1_NcqNonData.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
static BOOL L1_NNDAbortNCQQ(HCMD* pCurHCMD);
static BOOL L1_NNDDeadlineHandling(HCMD* pCurHCMD);
static BOOL L1_NNDSetFeature(HCMD* pCurHCMD);

/*============================================================================*/
/* main code region: function implement                                       */


/*==============================================================================
Func Name  : L1_HandleNCQNonData
Input      : HCMD* pCurHCMD  : all information of this host command.
Output     : NONE
Return Val : process success/finish or not.
Discription: entrance of processing NCQ NON-DATA command(0x63).
Usage      : called by L1_Schedule when the host command code is NCQ NON-DATA.
History    : 
    1. 2013.11.29 Haven Yang create function
==============================================================================*/


/*====================End of this file========================================*/

