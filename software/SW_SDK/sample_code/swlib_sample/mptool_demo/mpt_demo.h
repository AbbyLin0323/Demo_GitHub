#ifndef _MPTOOL_H
#define _MPTOOL_H
typedef struct _REG{
	U32 ulAddr;
	U32 ulData;
}REG;
typedef enum _BOOT_SELECT_MODE{
	NORMAL_MODE=0,
	LLF_MODE,
	MP_MODE
}BOOT_SELECT_MODE;
#define MAX_REG_CNT_DDR 256
#define BOOTLOADER_SZ (16*1024)
#define BOOLOADER_START_ADDR (0xfff00000)
#define FW_SZ (768*1024)
#define TRANS_SZ (4*4096)//64
#define FW_START_ADDR (0x40040000)
#define NFC_REG_SZ 256
#define NFC_REG_START_ADDR 0x1ff81000
#define BBT_START_BLK 2
void mpt_rom_config_dram(U8 ucDiskIndex);
void mpt_dram_test(U8 ucDiskIndex);
void mpt_fw_stage(DISKTYPE type);
void mpt_rom_stage(DISKTYPE * type);
#endif