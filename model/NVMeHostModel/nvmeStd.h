/**
 *******************************************************************************
 ** Copyright (c) 2011-2012                                                   **
 **                                                                           **
 **   Integrated Device Technology, Inc.                                      **
 **   Intel Corporation                                                       **
 **   LSI Corporation                                                         **
 **                                                                           **
 ** All rights reserved.                                                      **
 **                                                                           **
 *******************************************************************************
 **                                                                           **
 ** Redistribution and use in source and binary forms, with or without        **
 ** modification, are permitted provided that the following conditions are    **
 ** met:                                                                      **
 **                                                                           **
 **   1. Redistributions of source code must retain the above copyright       **
 **      notice, this list of conditions and the following disclaimer.        **
 **                                                                           **
 **   2. Redistributions in binary form must reproduce the above copyright    **
 **      notice, this list of conditions and the following disclaimer in the  **
 **      documentation and/or other materials provided with the distribution. **
 **                                                                           **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   **
 ** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, **
 ** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    **
 ** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR         **
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT,INDIRECT, INCIDENTAL, SPECIAL,      **
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       **
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        **
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
 **                                                                           **
 ** The views and conclusions contained in the software and documentation     **
 ** are those of the authors and should not be interpreted as representing    **
 ** official policies, either expressed or implied, of Intel Corporation,     **
 ** Integrated Device Technology Inc., or Sandforce Corporation.              **
 **                                                                           **
 *******************************************************************************
**/

/*
 * File: nvmeStd.h
 */

#ifndef __NVME_STD_H__
#define __NVME_STD_H__

#include "NVMeHostModel/nvme.h"

#ifdef __cplusplus
extern "C"{
#endif


#define CMD_TYPE_ADMIN      (1)
#define CMD_TYPE_IO         (2)
#define CMD_TYPE_SPECAIL    (3)

#define ADMIN_QCNT      (1)
#define ADMIN_Q_DEPTH   (64)
#define IO_QCNT         (8)
#define IO_Q_DEPTH      (64)

extern U8 *g_pHostDataBuffer;
extern U16 Admin_CQHead;

void NVMeDriverInit();
BOOL NVMeInterrupt();
void MSI_ThreadInit(void);
void MSI_ThreadExit();
BOOL NVME_IsHitSQLba(U32 ulStartLba, U32 ulSecCnt);
BOOL NVMe_IsSQEmpty();
void NVMe_SetShutdownReg(U32 ulType);
BOOL NVMe_IsAdminCmdFinish();
void NVMe_SetCcEn(U32 ulEnable);
U32 NVMe_IsControllerReady(void);
void NVMeCreateCQ(ULONG QID);
void NVMeCreateSQ(ULONG QID);
#ifdef __cplusplus
}
#endif
#endif /* __NVME_STD_H__ */
