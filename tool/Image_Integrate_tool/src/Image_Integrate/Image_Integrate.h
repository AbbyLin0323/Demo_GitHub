#ifndef _IMAGE_INTEGRATE_H
#define _IMAGE_INTEGRATE_H

#include <windows.h>
#include <assert.h>

#include "HAL_ParamTable.h"

#define MAX_FILE_NAME 80

#define BUF_SIZE 256

#define LEN_VALUE 20
#define LEN_LINE 512

#define MAX_NUM 32 //for operator and number

#define LEN_OF_TINYLOADER 0x2C00 
#define LEN_OF_PARAMETER 4096
#define LEN_OF_HEADER    1024
#define LEN_OF_FTABLE    256

#define OFFSET_TINYLOADER_IN_SYSTEM     0
#define OFFSET_PARAMETER_IN_SYSTEM      0x2C00
#define OFFSET_HEADER_IN_SYSTEM         0x3C00
#define OFFSET_BOOTLOADER_P1_IN_SYSTEM  0x4000

#define BOOT_MAGIC_SIZE 16
#define OFFSET_HEADER_INFO (BOOT_MAGIC_SIZE / sizeof(U32))

#define MCU0_IDX 0
#define MCU1_IDX 1
#define MCU2_IDX 2

typedef struct _IMAGE_HDR
{
    /* VT3533 FW IMAGE */
    U8 magic[BOOT_MAGIC_SIZE];

    U32 tinyloader_size; /* size in bytes */
    U32 tinyloader_addr; /* physical load addr */
    U32 tinyloader_exec; /* tinyloader exec addr */
    U32 tinyloader_crc32;

    U32 bootloader_size; /* size in bytes */
    U32 bootloader_addr; /* physical load addr */
    U32 bootloader_exec; /* bootloader exec addr */
    U32 bootloader_crc32;

    U32 mcu0fw_size; /* size in bytes */
    U32 mcu0fw_addr; /* physical load addr */
    U32 mcu0fw_exec; /* mcu0 exec addr */
    U32 mcu0fw_crc32;

    U32 mcu1fw_size; /* size in bytes */
    U32 mcu1fw_addr; /* physical load addr */
    U32 mcu1fw_exec; /* mcu1 exec addr */
    U32 mcu1fw_crc32;

    U32 mcu2fw_size; /* size in bytes */
    U32 mcu2fw_addr; /* physical load addr */
    U32 mcu2fw_exec; /* mcu2 exec addr */
    U32 mcu2fw_crc32;

    U32 rom_size; /* size in bytes */
    U32 rom_addr; /* physical load addr */
    U32 rom_exec; /* rom exec addr */
    U32 rom_crc32;
}IMAGE_HDR, * PIMAGE_HDR;

typedef struct _header_info
{
    TCHAR * pDesc;
    U8 ucDwOftInHeader;
}HEADER_INFO, * PHEADER_INFO;

static HEADER_INFO HEADER_INFO_LIB[] =
{
    { _T("tinyloader_size"), OFFSET_HEADER_INFO },
    { _T("tinyloader_addr"), (OFFSET_HEADER_INFO +1 ) },
    { _T("tinyloader_exec"), (OFFSET_HEADER_INFO + 2 ) },

    { _T("bootloader_size"), (OFFSET_HEADER_INFO + 4 ) },
    { _T("bootloader_addr"), (OFFSET_HEADER_INFO + 5 ) },
    { _T("bootloader_exec"), (OFFSET_HEADER_INFO + 6 ) },

    { _T("mcu0fw_size"), (OFFSET_HEADER_INFO + 8 ) },
    { _T("mcu0fw_addr"), (OFFSET_HEADER_INFO + 9 ) },
    { _T("mcu0fw_exec"), (OFFSET_HEADER_INFO + 10 ) },

    { _T("mcu1fw_size"), (OFFSET_HEADER_INFO + 12 ) },
    { _T("mcu1fw_addr"), (OFFSET_HEADER_INFO + 13 ) },
    { _T("mcu1fw_exec"), (OFFSET_HEADER_INFO + 14 ) },

    { _T("mcu2fw_size"), (OFFSET_HEADER_INFO + 16 ) },
    { _T("mcu2fw_addr"), (OFFSET_HEADER_INFO + 17 ) },
    { _T("mcu2fw_exec"), (OFFSET_HEADER_INFO + 18 ) },

    {_T(""), 0}
};

typedef struct _fw_binary_info
{
    ULONG ulMcu0Offset;
    ULONG ulMcu0Len;
    ULONG ulMcu1Offset;
    ULONG ulMcu1Len;
    ULONG ulMcu2Offset;
    ULONG ulMcu2Len;
}FW_BINARY_INFO, * PFW_BINARY_INFO;

#endif