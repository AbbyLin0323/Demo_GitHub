#ifndef __HEAD_H__
#define __HEAD_H__
#include "BaseDef.h"

//#define BOOT_TEST_FALAG
//#define BOOT_DBG
//#define BOOT_DATA_MANUAL

#define DDR_OFFSET 0x24000   //136K+8k=144K   16K align  

//#define  REG_START_ADDRESS         0x10000
#define D_SRAM_START_ADDRESS       0x1ff80000
#define I_SRAM_START_ADDRESS       0x20000000
//#define OTFB_START_ADDRESS        0x30000
//#define DRAM_START_ADDRESS      0x10000000

#define HEAD_SEG_SIZE        16*1024       //16K    
#define DDR_SEG_SIZE          64*1024      //64K
#define D_SRAM_SEG_SIZE    40*1024     //40K
#define I_SRAM_SEG_SIZE     64*1024      //64K
#define OTFB_SEG_SIZE         136*1024     //136K

#define TABLE_HEAD_SEG_ADDR                (DRAM_START_ADDRESS+DDR_OFFSET)
#define TABLE_DDR_SEG_ADDR                      ( TABLE_HEAD_SEG_ADDR+HEAD_SEG_SIZE )
#define TABLE_D_SRAM_SEG_ADDR           ( TABLE_DDR_SEG_ADDR+DDR_SEG_SIZE )
#define TABLE_I_SRAM_SEG_ADDR                ( TABLE_D_SRAM_SEG_ADDR+D_SRAM_SEG_SIZE )
#define TABLE_OTFB_SEG_ADDR                 ( TABLE_I_SRAM_SEG_ADDR+I_SRAM_SEG_SIZE )
#define TOTAL_TABLE_SIZE                          ( HEAD_SEG_SIZE+DDR_SEG_SIZE+D_SRAM_SEG_SIZE+I_SRAM_SEG_SIZE+OTFB_SEG_SIZE+8*1024 )    //0x5,0000=320K  16K Allign
#define BOOT_LOADER_SIZE                        0x4000   //16K


#define PU0          0
#define PU1           1
#define BLK0           0
#define PAGE0      0
#define PAGE1       1
#define LEVEL0      0
#define BOOTLOADER_START_PAGE    PAGE0
#define FIRMWARE_START_PAGE         PAGE1

void PrepareFirmwareRun();
void PrepareFirmwareSleep();
U32 head();
//extern void set_stack();

#ifdef BOOT_TEST_FALAG
extern U32 g_bBootUpFlag;
void BootTest();
void CopyVerification();
void BootVefification(U32 uPuNum,U32 uAddr);
void MemSetDW(U32 addr,U32 value,U32 size);
#endif
int ReadVerification(U8 uPuNum, U32 uStartPg);

#endif

