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

Filename     :   AHCI_HostModel.h                                         
Version      :   0.1                                              
Date         :   2013.08.26                                         
Author       :   bettywu

Description:  the interface to other model
Others: 
Modification History:
20130826 create

******************************************************************************/

#ifndef _H_AHCI_CONTROLLERMODEL
#define _H_AHCI_CONTROLLERMODEL


typedef enum _WBQ_IS_FLAG
{
    WBQ_IS_D2H_FLAG          = 0,
    WBQ_IS_PIO_SETUP_FLAG    = 1,
    WBQ_IS_DMA_SETUP_FLAG    = 2,
    WBQ_IS_SDB_FLAG          = 3
} WBQ_IS_FLAG;



// by Charles Zhou for re-designing WBQ process model.
void SIM_HostCWBQDataDoneTrigger( U8 ucCmdTag );
BOOL SIM_HostCWBQThreadExit();
BOOL SIM_HostCWBQProcessByEvent( U32 ucEventType );
void AHCI_HostCWBQEnterCS( void );
void AHCI_HostCWBQLeaveCS( void );


#ifndef IGNORE_PERFORMANCE
typedef enum _FCQ_STS_TAG
{
    FCQ_STS_WAIT_CMD,
    FCQ_STS_WAIT_BUS,
    FCQ_STS_PCIE_TRANS
}FCQ_STS_TAG;

typedef enum _WBQ_STS_TAG
{
    WBQ_STS_WAIT_BUS,
    WBQ_STS_PCIE_TRANS
}WBQ_STS_TAG;
#endif

#endif