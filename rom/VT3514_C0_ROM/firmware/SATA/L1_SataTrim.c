/*************************************************
Copyright (C), 2009, VIA Tech. Co., Ltd.
Filename: L1_SataTrim.c                                          
Version: 0.9                                                 
Date: 2013-03-12                                             
Author: Blakezhang

Description: SATA TRIM feature support
Others:

Modification History:
Blakezhang      2013-03-12,      first created
*************************************************/
#include "HAL_Inc.h"
#include "L1_SataCmd.h"
#include "L1_SataTrim.h"
#include "L1_Interface.h"
  
#ifdef SIM
#include <stdio.h>
#endif
  
#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
#endif



