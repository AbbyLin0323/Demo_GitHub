/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : L2_VirtualStorage.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Yao Chen

Description: 
    A virtual storage sub-system for L1 function debugging purpose is defined here.
    It uses DDR SDRAM to simulate a L2-L3 storage sub-system which can directly
    interact with L1 buffer interface.

    Why do we place it in L1 instead of L2? L2 tasks can be scheduled to run on core 2.

Modification History:

*************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"

#include "L2_VirtualStorage.h"

/********************** FILE END ***************/

