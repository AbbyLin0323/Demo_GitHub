#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <windows.h>
#include <minwindef.h>
#include <tchar.h>
//#include <assert.h>
//#include <tchar.h>
#include <math.h>
#include <io.h>
#include "HAL_ParamTable.h"

#define MAX_CONST 512
#define MAX_STRID 256
#define MAX_HEXID 256
#define LINESZ 1024//512
#define MAX_NUM 32 //for operator and number

#define LEN_OPCODE 10
#define LEN_VALUE  500//256//60//30
#define BUF_SIZE 256  //buffer for one opcode

#define MAX_PARAMETER_LEN (4*1024)
#define NUM_OF_OPCODE_BYTE 4

//#define MAX_REG_LIST_SIZE (4096 - FTABLE_SIZE - PTABLE_SIZE)

typedef enum _opcode_type
{    
    OPCODE_MEMW32 = 0,
    OPCODE_MEMW16,//1
    OPCODE_MEMW8,//2
    OPCODE_SETBIT32,//3
    OPCODE_SETBIT16,//4
    OPCODE_SETBIT8,//5
    OPCODE_CLRBIT32,//6
    OPCODE_CLRBIT16,//7
    OPCODE_CLRBIT8,//8
    OPCODE_UDELAY,//9
    OPCODE_MDELAY,//10
    OPCODE_MWAIT32,//11
    OPCODE_MWAIT16,//12
    OPCODE_MWAIT8,//13
    OPCODE_SETSTR = 0x0F,//15
    OPCODE_SETHEX,//16
    OPCODE_SETDEC = 0x10,//16
    OPCODE_VER,//17
    OPCODE_NFC_INTERFACE_INIT,//18
    OPCODE_NFC_RESET,//19
    OPCODE_NFC_UPDATE_PUMAPPING,//20
    OPCODE_NFC_SETFEATURE,//21
    OPCODE_VENDOR,//22
    OPCODE_END = 0xFF,
    OPCODE_NULL = 0xFE
}OPCODE_TYPE;

typedef enum _directive_type
{
    DIRECTIVE_INCLUDE,
    DIRECTIVE_DEFINE,
    DIRECTIVE_FTABLEID,
    DIRECTIVE_PTMEMSET,
    DIRECTIVE_NULL = 0xFF
}DIRECTIVE_TYPE;

typedef struct _opcode_type_desc
{
    OPCODE_TYPE eOpcodeType;
    TCHAR * pDesc;
}OPCODE_TYPE_DESC;

typedef struct _directive_type_desc
{
    DIRECTIVE_TYPE eDirectType;
    TCHAR * pDesc;
}DIRECTIVE_TYPE_DESC;

static OPCODE_TYPE_DESC OPCODE_TYPE_LIB[] =
{
    { OPCODE_MEMW32, _T("memw32") },
    { OPCODE_MEMW16, _T("memw16") },
    { OPCODE_MEMW8, _T("memw8") },
    { OPCODE_SETBIT32, _T("setbit32") },
    { OPCODE_SETBIT16, _T("setbit16") },
    { OPCODE_SETBIT8, _T("setbit8") },
    { OPCODE_CLRBIT32, _T("clrbit32") },
    { OPCODE_CLRBIT16, _T("clrbit16") },
    { OPCODE_CLRBIT8, _T("clrbit8") },
    { OPCODE_UDELAY, _T("udelay") },
    { OPCODE_MDELAY, _T("mdelay") },
    { OPCODE_MWAIT32, _T("mwait32") },
    { OPCODE_MWAIT16, _T("mwait16") },
    { OPCODE_MWAIT8, _T("mwait8") },
    { OPCODE_SETSTR, _T("setstr") },
    { OPCODE_SETHEX, _T("sethex") },
    { OPCODE_SETDEC, _T("setdec") },
    { OPCODE_VER, _T("ver") },
    { OPCODE_NFC_INTERFACE_INIT, _T("nfcinit") },
    { OPCODE_NFC_RESET, _T("nfcreset") },
    { OPCODE_NFC_UPDATE_PUMAPPING, _T("nfcupdate") },
    { OPCODE_NFC_SETFEATURE, _T("nfcset") },
    { OPCODE_VENDOR, _T("vendorop") },
    { OPCODE_END, _T("end") },
    { OPCODE_NULL, _T("") }
};

static DIRECTIVE_TYPE_DESC DIRECTIVE_TYPE_LIB[] = 
{
    { DIRECTIVE_INCLUDE, _T("include") },
    { DIRECTIVE_DEFINE, _T("define") },
    { DIRECTIVE_FTABLEID, _T("ftableid") },
    { DIRECTIVE_PTMEMSET, _T("ptmemset") },
    { DIRECTIVE_NULL, _T("") }
};

typedef struct _line_parameter
{
    TCHAR value[LEN_VALUE];
}LINE_PARAMETER, *PLINE_PARAMETER;

typedef struct _line_command
{
    TCHAR type[LEN_OPCODE];
    //TCHAR value_1[LEN_VALUE];
    //TCHAR value_2[LEN_VALUE];
    UINT uiNumOfValue;
    LINE_PARAMETER *pValue;
}LINE_COMMAND, *PLINE_COMMAND;

typedef struct _constant_val
{
    TCHAR const_name[LEN_VALUE];
    TCHAR const_val[LEN_VALUE];
}CONSTANT_VAL, *PCONSTANT_VAL;

//typedef enum _str_type
//{
//    STR_SN = 0,
//    STR_MPMODE,
//    STR_NULL = 0xFF
//}STR_TYPE;
//
//typedef struct _STR_TYPE_DESC
//{
//    STR_TYPE eStrType;
//    TCHAR * pDesc;
//}STR_TYPE_DESC, *PSTR_TYPE_DESC;
//
//static STR_TYPE_DESC STR_TYPE_LIB[] = 
//{
//    { STR_SN, _T("SN") },
//    { STR_MPMODE, _T("MP_MODE") },
//    { STR_NULL, _T("") }
//};

typedef struct _ver_desc
{
    /*TCHAR ww[LEN_VALUE];
    TCHAR xx[LEN_VALUE];
    TCHAR yy[LEN_VALUE];*/
    UINT8 ww;
    UINT8 xx;
    UINT8 yy;
    TCHAR c;
}VER_DESC, *PVER_DESC;

typedef struct _InitFunc_Info
{
    UINT uiInitFuncIdx;
    UINT uiTotalByte;
    UINT uiRegCnt;
    BOOL bHasSet;
    BOOL bIsMultiple;
}INITFUNC_INFO;

typedef struct _WriteFunc_Info
{
    UINT uiFuncIdx;
    UINT uiCurOffset;
    BOOL bHasWrite;
}WRITEFUNC_INFO;

BOOL IsAbsolutePath(const TCHAR * pPath);
#endif