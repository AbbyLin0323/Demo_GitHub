#ifndef _BL_OP_DEFINE_
#define _BL_OP_DEFINE_
#include "BaseDef.h"

#define TYPE_OP_ADDR4_VAL4_LEN (4*3)
#define TYPE_OP_VAL4_LEN (4*2)
#define TYPE_OP_NOPARAM_LEN (4*1)

typedef enum _opcode_type
{
    OPCODE_MEMW32 = 0,
    OPCODE_MEMW16, // 1
    OPCODE_MEMW8,// 2
    OPCODE_SETBIT32,// 3
    OPCODE_SETBIT16,//4 // 4
    OPCODE_SETBIT8,//5
    OPCODE_CLRBIT32,//6
    OPCODE_CLRBIT16,//7
    OPCODE_CLRBIT8,//8
    OPCODE_UDELAY,//9
    OPCODE_MDELAY,//10
    OPCODE_MWAIT32,//11
    OPCODE_MWAIT16,//12
    OPCODE_MWAIT8,//13
    //14 skipped.
    OPCODE_SETSTR = 0x0F,//15
    OPCODE_SETHEX,//16
    OPCODE_SETDEC = 0x10,//16
    OPCODE_VER,// 17
    OPCODE_NFC_INTERFACE_INIT,// 18
    OPCODE_NFC_RESET,// 19
    OPCODE_NFC_UPDATE_PUMAPPING,// 20
    OPCODE_NFC_SETFEATURE,// 21
    OPCODE_VENDOR_OP, // 22
    OPCODE_END = 0xFF,
    OPCODE_NULL = 0xFE,
    OPCODE_ALL = 0xAA
}OPCODE_TYPE;

typedef struct _opcode_type_desc
{
    OPCODE_TYPE eOpcodeType;
    const char * pDesc;
}OPCODE_TYPE_DESC;

BOOL BL_CheckAlignU32(U32 ulAddr);
void BL_DecodeOpsInRAM(const U8 *pOPCodeStart, U32 ulOPCodeLength, OPCODE_TYPE desireOp);

static OPCODE_TYPE_DESC OPCODE_TYPE_LIB[] =
{
    /*
    和OPCODE的Enum值顺序保持一致为佳
    bootloader中的代码不需要这张表，只是为了在PC上做单元测试，反解OPCode 用。
    */
    { OPCODE_MEMW32, "memw32" },
    { OPCODE_MEMW16, "memw16" },
    { OPCODE_MEMW8, "memw8" },
    { OPCODE_SETBIT32, "setbit32" },
    { OPCODE_SETBIT16, "setbit16" },
    { OPCODE_SETBIT8, "setbit8" },
    { OPCODE_CLRBIT32, "clrbit32" },
    { OPCODE_CLRBIT16, "clrbit16" },
    { OPCODE_CLRBIT8, "clrbit8" },
    { OPCODE_UDELAY, "udelay" },
    { OPCODE_MDELAY, "mdelay" },
    { OPCODE_MWAIT32, "mwait32" },
    { OPCODE_MWAIT16, "mwait16" },
    { OPCODE_MWAIT8, "mwait8" },
    { OPCODE_NULL, ""},///////////////////14在定义时就没有
    { OPCODE_SETSTR, "setstr" },
    { OPCODE_SETHEX, "sethex" },
    { OPCODE_VER, "ver" },
    { OPCODE_NFC_INTERFACE_INIT, "nfcinit" }, //nfc soft init
    { OPCODE_NFC_RESET, "nfcreset" },//nfc reset
    { OPCODE_NFC_UPDATE_PUMAPPING, "nfcupdate" }, //update pu mapping
    { OPCODE_NFC_SETFEATURE, "nfcset" },//nfc set feature
    { OPCODE_END, "end" },
    { OPCODE_NULL, "" }
};
#endif
