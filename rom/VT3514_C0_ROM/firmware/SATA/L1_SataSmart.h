/*************************************************
Copyright (C), 2010, VIA Tech. Co., Ltd.
Filename: L1_SataSmart.h                                            
Version: 0.9                                                 
Date: 2013-02-28                                            
Author: HenryLuo

Description: sata smart module
Others:

Modification History:
HenryLuo,       2010-11-25,      first created
Blakezhang      2013-02-28,      blakezhang porting to C0 FW
*************************************************/

#ifndef _L1_SATASMART_H
#define _L1_SATASMART_H

#define    SMART_ATTRIBUTE_ENTRY_MAX    30

#define    SMART_ATTRIBUTE_VERSION    0xaa55
#define    SMART_THRESHOLDS_REVISION    0x55aa

#pragma pack(1)
typedef enum SMART_DATA_SECTOR_TAG
{
    SMART_DATA_SECTOR_OFF = 0,
    SMART_THRESHOLDS_SECTOR_OFF,    /* add by henryluo for smart thresholds info */
    SMART_DIRECTORY_SECTOR_OFF,
    SMART_SUMMARY_ERR_SECTOR_OFF,
    SMART_SELF_TEST_SECTOR_OFF,
    SMART_SELECTIVE_TEST_SECTOR_OFF,
    
    SMART_DATA_SECTOR_MAX
} SMART_DATA_SECTOR;

typedef struct _SMART_ATTRIBUTE_TBL
{
    U8    id;
    U8    flaglow;
    U8    flaghi;
    U8    value;
    U8    worst;
    U8    rawdata[6];
    U8    unkown;
}SMART_ATTRIBUTE_TBL, *PSMART_ATTRIBUTE_TBL;

typedef struct _SMART_INFO
{
    U16 version;
    SMART_ATTRIBUTE_TBL attribute_tbl[SMART_ATTRIBUTE_ENTRY_MAX];
    U8    wReserved[150];
} SMART_INFO, *PSMART_INFO;

typedef struct _SMART_THRESHOLDS_TBL
{
    U8    id;
    U8    thresholds;
    U8    reserved[10];
}SMART_THRESHOLDS_TBL, *PSMART_THRESHOLDS_TBL;

typedef struct _SMART_THRESHOLDS
{
    U16 revision;
    SMART_THRESHOLDS_TBL thresholds_tbl[SMART_ATTRIBUTE_ENTRY_MAX];
    U8    wReserved[150];
} SMART_THRESHOLDS, *PSMART_THRESHOLDS;

typedef struct _MEMORIGHT_SMART
{
    U8    id;
    U8    thresholds;
      U8    worst;
    U8    name[50];
}MEMORIGHT_SMART, *PMEMORIGHT_SMART;
#pragma pack()


extern void DRAM_ATTR L1_SataInitSmart(void);
extern BOOL DRAM_ATTR L1_SataCmdSmart(void);

#endif

