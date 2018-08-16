/*************************************************
Copyright (C), 2009, VIA Tech. Co., Ltd.
Filename: L1_SataSmart.c                                          
Version: 0.9                                                 
Date: 2013-02-28                                             
Author: JackeyChai

Description: SATA SMART feature support
Others:

Modification History:
JackeyChai,       2009-08-31,      first created
Blakezhang      2013-02-28,      blakezhang porting to C0 FW
*************************************************/
#include "HAL_Inc.h"
#include "L1_SataCmd.h"
#include "L1_SataSmart.h"
#include "L1_Interface.h"
  
#ifdef SIM
#include <stdio.h>
#endif
  
#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
#endif

/*
const MEMORIGHT_SMART memoright_smart_default[] DRAM_ATTR = 
{
    {0x09, 0x00, 0x00, "Power ON hour count"},
    {0x0C, 0x00, 0x01, "Power Cycle Count"},
    {0xBF, 0x00, 0x02, "Map Rebuild Count"},
    {0xC0, 0x00, 0x03, "Power Off retract Count"},
    {0xC2, 0x00, 0x04, "SSD Temperature"},
    {0xC5, 0x00, 0x05, "ECC Event Count"},
    {0xC6, 0x00, 0x06, "Uncorrectable Read Data Count"},
    {0xC7, 0x00, 0x07, "UDMA CRC Error Count"},
    {0xFB, 0x00, 0x08, "Remaining Spare Flash Block Percentage"},
    {0xFC, 0x00, 0x09, "Total bad block Count"},
    {0xFD, 0x00, 0x0a, "Reserved for FW use"},
    {0xFE, 0x00, 0x0b, "Total Erase Flash block Count"},
    {0x00, 0x00, 0x00, "End of Table"},
};
*/
BOOL g_bSataSmartAutoSave;
BOOL g_bSataSmartEnable;
U32  g_ulSmartStatus;
U32  g_ulSmartReadDataFlag;



