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
  File Name     : L2_Sata.h
  Version       : Initial Draft
  Author        : henryluo
  Created       : 2014/10/22
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2014/10/22
    Author      : henryluo
    Modification: Created file

*******************************************************************************/

#ifndef __L2_SATA_H__
#define __L2_SATA_H__

#include "BaseDef.h"
#include "L1_Interface.h"

extern BOOL L2_FtlHandleReadWithoutWrite(U8 ucSuperPu, BUF_REQ_READ* pReq, BUFREQ_HOST_INFO* pBufReqHostInfo, U8 LPNOffset, U8 ucReqLenth, U8 ucReqLPNCnt, BOOL bFirstCMDEn);


#endif

