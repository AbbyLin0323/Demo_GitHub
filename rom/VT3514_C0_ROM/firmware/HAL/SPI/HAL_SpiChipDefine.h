#ifndef _HAL_SPI_CHIP_DEF_MX_H_
#define _HAL_SPI_CHIP_DEF_MX_H_
#include "HAL_SpiDriver.h"

#define SPI_RCMD_DUMMY(_cycle_)     (_cycle_)
typedef struct SPI_RCMD_TABLE
{
    U32 bsCmdCode   :8;
    U32 bsSpiMode   :1;
    U32 bsDataIO    :2;
    U32 bsAddrMode  :1;
    U32 bsCmdIO     :2;
    U32 bsDummy     :8;
    U32 bsRes       :10;
}SPI_RWCMD_TABLE;

#define SPI_CHIP_TYPE_WB            1
#define SPI_CHIP_TYPE_MX            2

//#define SPI_CHIP_TYEP_CURR          SPI_CHIP_TYPE_MX
#define SPI_CHIP_TYEP_CURR          SPI_CHIP_TYPE_WB

#if SPI_CHIP_TYEP_CURR == SPI_CHIP_TYPE_MX  //defined(SPI_CHIP_TYPE_MX)
enum SPI_CMD_CODE_
{

    SPI_CMD_RDSR1   = (0x05), // read status register
    SPI_CMD_RDSR2   = (0x15),// read config register
    SPI_CMD_RDSR3   = (0x2B),// read security register
                    
    SPI_CMD_WRSR1   = (0x01),// write status register
    SPI_CMD_WRSR2   = (0x2F),// write security register
    SPI_CMD_WRSR3   = (0x01),// default

    SPI_CMD_WREN    = (0x06),
    SPI_CMD_EN4B    = (0xB7),
    SPI_CMD_EX4B    = (0xE9),
    SPI_CMD_EQIO    = (0x35),
    SPI_CMD_EXQIO   = (0xF5),
    SPI_CMD_RDID    = (0x9F),
    SPI_CMD_RSTEN   = (0x66),
    SPI_CMD_RST     = (0x99),
    SPI_CMD_READ    = (0x03),
    SPI_CMD_READ4B  = (0x13),
    SPI_CMD_DREAD   = (0xBB),// 2X INPUT IO read
    SPI_CMD_DREAD4B = (0xBC),
    SPI_CMD_QREAD_T = (0xEA),
    SPI_CMD_QREAD   = (0xEB),
    SPI_CMD_QREAD4B = (0xEC),
    SPI_CMD_PP      = (0x02),
    SPI_CMD_PP4B    = (0x12),
    SPI_CMD_4PP     = (0x38),
    SPI_CMD_4PP4B   = (0x3E),
    SPI_CMD_BE_64   = (0xD8),
    SPI_CMD_BE4B    = (0xDC),
    SPI_CMD_SE      = (0x20),
    SPI_CMD_SE4B    = (0x21),
    SPI_CMD_CE      = (0xC7)
};

typedef enum SPI_RCMD_INDEX_
{
    SPI_RCMD_INDEX_READ, // normal read SPI          Cmd X1 Addr X1 DX1
    SPI_RCMD_INDEX_DREAD,// dual input IO read SPI   Cmd X1 Addr X2 DX2
    SPI_RCMD_INDEX_QREAD,// quad input IO read QPI   Cmd X4 Addr X4 DX4
    SPI_RCMD_INDEX_READ4B,// normal read SPI(4Byte)          Cmd X1 Addr X1 DX1
    SPI_RCMD_INDEX_DREAD4B,// dual input IO read SPI (4Byte)  Cmd X1 Addr X2 DX2
    SPI_RCMD_INDEX_QREAD4B,// quad input IO read QPI (4Byte)  Cmd X4 Addr X4 DX4
    SPI_RCMD_INDEX_CNT
}SPI_RCMD_INDEX;

#define RCMD_ENTRY_0    {SPI_CMD_READ ,      SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_3B,   SPI_CMDIO_1BIT,    SPI_RCMD_DUMMY(0)}
#define RCMD_ENTRY_1    {SPI_CMD_DREAD,      SPI_MODE,   SPI_DATAIO_2BIT,    SPI_ADDR_3B,   SPI_CMDIO_2BIT,    SPI_RCMD_DUMMY(8)}
#define RCMD_ENTRY_2    {SPI_CMD_QREAD,      QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_3B,   SPI_CMDIO_4BIT,    SPI_RCMD_DUMMY(8)}
#define RCMD_ENTRY_3    {SPI_CMD_READ4B ,    SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_4B,   SPI_CMDIO_1BIT,    SPI_RCMD_DUMMY(0)}
#define RCMD_ENTRY_4    {SPI_CMD_DREAD4B,    SPI_MODE,   SPI_DATAIO_2BIT,    SPI_ADDR_4B,   SPI_CMDIO_2BIT,    SPI_RCMD_DUMMY(8)}
#define RCMD_ENTRY_5    {SPI_CMD_QREAD4B,    QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_4B,   SPI_CMDIO_4BIT,    SPI_RCMD_DUMMY(8)}
#define RCMD_ENTRY_6    {}
#define RCMD_ENTRY_7    {}

typedef enum SPI_WCMD_INDEX_
{
    SPI_WCMD_INDEX_PP, // Page program SPI
    SPI_WCMD_INDEX_QPP,// Page program QPI
    SPI_WCMD_INDEX_PP4B,// Page program SPI 4 byte address
    SPI_WCMD_INDEX_QPP4B,// Page program QPI 4 byte address
    SPI_WCMD_INDEX_CNT
}SPI_WCMD_INDEX;

#define WCMD_ENTRY_0    {SPI_CMD_PP,    SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_3B,    SPI_CMDIO_1BIT}
#define WCMD_ENTRY_1    {SPI_CMD_PP,    QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_3B,    SPI_CMDIO_4BIT}
#define WCMD_ENTRY_2    {SPI_CMD_PP4B,  SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_4B,    SPI_CMDIO_1BIT}
#define WCMD_ENTRY_3    {SPI_CMD_PP4B,  QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_4B,    SPI_CMDIO_4BIT}

#elif SPI_CHIP_TYEP_CURR == SPI_CHIP_TYPE_WB

enum SPI_CMD_CODE_
{
    SPI_CMD_RDSR1       = (0x05), // read status register-1
    SPI_CMD_RDSR2       = (0x35),// read status register-2
    SPI_CMD_RDSR3       = (0x15),// read status register-3
                        
    SPI_CMD_WRSR1       = (0x01),// write status register-1
    SPI_CMD_WRSR2       = (0x31),// write status register-2
    SPI_CMD_WRSR3       = (0x11),// write status register-3
                        
    SPI_CMD_WREN        = (0x06),// write enable
                        
    SPI_CMD_EN4B        = (0xB7),// 3 byte mode ---- current chip
    SPI_CMD_EX4B        = (0xE9),// 4 byte mode
                        
    SPI_CMD_EQIO        = (0x38),// Enter QPI Mode
    SPI_CMD_EXQIO       = (0xFF),//Exit QPI Mode
                        
    SPI_CMD_RDID        = (0xAB),//Release Power-down/Device ID
    SPI_CMD_RMorDID     = (0x90),//Read Manufacturer/Device ID
    SPI_CMD_DRMorDID    = (0x92),//Read Manufacturer/Device ID Dual I/O
    SPI_CMD_QRMorDID    = (0x94),//Read Manufacturer/Device ID Quad I/O
    SPI_CMD_RUniqueID   = (0x4B),//Read Unique ID Number
    SPI_CMD_RJEDECID    = (0x4B),//Read JEDEC ID

    SPI_CMD_RSTEN       = (0x66),//Enable Reset
    SPI_CMD_RST         = (0x99),//Reset Device

    SPI_CMD_READ        = (0x03),//Read Data
    SPI_CMD_FREAD       = (0x0B),//Fast Read
    SPI_CMD_DREAD       = (0x3B),//Fast Read Dual Output
    SPI_CMD_QREAD       = (0x6B),//Fast Read Qual Output
    SPI_CMD_DREAD_IO    = (0xBB),//Fast Read Dual I/O
    SPI_CMD_QREAD_IO    = (0xEB),//Fast Read Qual I/O

    SPI_CMD_SRP         = (0xC0),//Set Read Parameters

    SPI_CMD_PP          = (0x02),//Page Program
    SPI_CMD_QIPP        = (0x32),//Quad Input Page Program

    SPI_CMD_BE_32       = (0x52),//32KB Block Erase
    SPI_CMD_BE_64       = (0xD8),//64KB Block Erase
    SPI_CMD_CE          = (0xC7),//Whole chip Erase

    SPI_CMD_BE4B        = (0xDC),//Erase Chip

    SPI_CMD_SE          = (0x20),//Sector Erase
    SPI_CMD_SE4B        = (0x21)
};

typedef struct _SPI_STATUS_REG2_
{
    U8 bsSRP2   :1; // status register protect 1
    U8 bsQE     :1;// Quad Enable
    U8 bsRes    :6;
}SPI_STATUS_REG2;

typedef enum SPI_RCMD_INDEX_
{
    SPI_RCMD_INDEX_READ, // normal read SPI          Cmd X1 Addr X1 DX1
    SPI_RCMD_INDEX_DREAD,// dual input IO read SPI   Cmd X1 Addr X2 DX2
    SPI_RCMD_INDEX_QREAD,// quad input IO read QPI   Cmd X4 Addr X4 DX4
    SPI_RCMD_INDEX_CNT
}SPI_RCMD_INDEX;

#define RCMD_ENTRY_0    {SPI_CMD_READ ,  SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_3B,   SPI_CMDIO_1BIT,    SPI_RCMD_DUMMY(0)}
#define RCMD_ENTRY_1    {SPI_CMD_DREAD,  SPI_MODE,   SPI_DATAIO_2BIT,    SPI_ADDR_3B,   SPI_CMDIO_2BIT,    SPI_RCMD_DUMMY(8)}
#define RCMD_ENTRY_2    {SPI_CMD_FREAD,  QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_3B,   SPI_CMDIO_4BIT,    SPI_RCMD_DUMMY(4)}
#define RCMD_ENTRY_3    {}
#define RCMD_ENTRY_4    {}
#define RCMD_ENTRY_5    {}
#define RCMD_ENTRY_6    {}
#define RCMD_ENTRY_7    {}

typedef enum SPI_WCMD_INDEX_
{
    SPI_WCMD_INDEX_PP, // Page program SPI
    SPI_WCMD_INDEX_QPP,// Page program QPI
    SPI_WCMD_INDEX_CNT
}SPI_WCMD_INDEX;

#define WCMD_ENTRY_0    {SPI_CMD_PP,    SPI_MODE,   SPI_DATAIO_1BIT,    SPI_ADDR_3B,    SPI_CMDIO_1BIT}
#define WCMD_ENTRY_1    {SPI_CMD_PP,    QPI_MODE,   SPI_DATAIO_4BIT,    SPI_ADDR_3B,    SPI_CMDIO_4BIT}
#define WCMD_ENTRY_2    {}
#define WCMD_ENTRY_3    {}

#endif 

typedef union _SPI_STATUS_REG_
{
    struct
    {
        U8 bsWIP :1; // write in process
        U8 bsWEL :1;// write enable latch
        U8 bsRes :6;
    };
    U8 ucValue;
}SPI_STATUS_REG;

#endif
