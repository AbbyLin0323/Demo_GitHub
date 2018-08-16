/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   io_specify.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: disk IO with DeviceIoControl with ATA_PASS_THROUGH
*              can specify the command code and command type except NCQ commands
*              all IOs response in syn mode
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 jackeychai first created
*************************************************/
#ifndef __IO_SPECIFY_H__
#define __IO_SPECIFY_H__

#include "device.h"

enum 
{
	DATA_TRANSFER_IN = 0,
	DATA_TRANSFER_OUT,
};



typedef struct _IDEREGSEX
{

    U8  bFeaturesReg;
	U8  bSectorCountReg;
    union 
    {
	    U8  bLBALowReg;//bSectorNumberReg; /*LBA low*/
        U8  bSectorNumberReg;
    };

    union
    {
	    U8  bLBAMidReg;//bCylLowReg;       /*LBA Mid*/  
        U8  bCylLowReg;
    };
    union
    {
	    U8  bLBAHighReg;//bCylHighReg;      /*LBA Hight*/ 
        U8 bCylHighReg;
    };
	U8  bDriveHeadReg;
	U8  bCommandReg;
	U8  bReserved;
    
} IDEREGSEX, *PIDEREGSEX, *LPIDEREGSEX;

#if(_WIN32_WINNT < 0x0400)
typedef IDEREGSEX IDEREGS;
#endif

BOOL io_specify_identify_data(HANDLE disk, U32 inbuf_addr);
//BOOL io_specify_smart_identify_data(HANDLE disk, U32 inbuf_addr);


BOOL sata_dma_cmd_ex(HANDLE disk,IDEREGS reg,IDEREGS exreg,U32 direction,U32 buf_addr,U32 trans_sec_cnt);
BOOL sata_pio_cmd_ex
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg,
 U32 direction,
 U32 buf_addr,
 U32 buf_len
 );

BOOL sata_nondata_cmd_ex
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg
 );
extern IDEREGS g_outreg;
extern IDEREGS g_outreg_ext;

U32 sata_register_write_cmd( HANDLE disk, IDEREGS* preg, IDEREGS* preg_Exp);

BOOL sata_vendor_read_register(HANDLE disk, U32 addr, U32 *retvaladdr);
BOOL sata_vendor_write_register(HANDLE disk, U32 addr,U32 nvalue);
void sata_vendor_pio_cmd(HANDLE disk, U32 cmd_code, U32 nDirec, U32 buf_addr, U32 buf_len);
void sata_vendor_nonedata_cmd( HANDLE disk, U32 cmd_code);
BOOL sata_vendor_dma_cmd(HANDLE disk, U32 cmd_code, U32 nDirec, U32 buf_addr, U32 trans_sec_cnt);
BOOL io_specify_pm_request(HANDLE disk, U32 command_code);

BOOL sata_cmd_dma_write_trim(HANDLE disk,U32 buf_addr,U32 trans_sec_cnt);
BOOL io_specify_trim_request(HANDLE disk);
void dataverify_set_data_before_write_trim(U32 start_lba,U32 End_lba);
BOOL io_specify_trim_request_random(HANDLE disk);


#endif