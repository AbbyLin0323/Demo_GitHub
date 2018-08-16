#ifndef WIN_DEBUG_CONFIG_H
#define WIN_DEBUG_CONFIG_H

#define SEC_MSK 0x1ff
#define SEC_BIT 0x9
#define SEC_SZ 512

#define MAX_LBA_WITH_DATA (1<<27)
#define MAX_DISK_LBA  (1<<27)

//#define WITHJTAG

extern U32 syscfg_max_lba;
extern U32 syscfg_max_user_lba;
extern U32 syscfg_max_accessable_lba;

void config_init();































#endif