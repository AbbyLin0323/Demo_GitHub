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
  File Name     : iniparser.h
  Version       : Initial Draft
  Author        : Nina Yang
  Created       : 2014/4/13
  Last Modified :
  Description   :
  Function List :
  History       :
  1.Date        : 2014/4/13
    Author      : Nina Yang
    Modification: Created file

******************************************************************************/
#ifndef _INIPARSER_H_
#define _INIPARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/COM/BaseDef.h"

#define LINESZ 1024

typedef enum _line_status_ {
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
} line_status ;

BOOL scmd_parser_load(char* ininame);
BOOL iniparser_load(char* ininame);
void GetChecklistName(char *pFileName);
void GetChecklistInit(char *FilePath);
char * strstrip(char * s);

#endif