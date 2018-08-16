#ifndef __COMM_DEF_H__
#define __COMM_DEF_H__

#include "BaseDef.h"

typedef U32 (LibEntry)(void *pParamOut, void *pParamIn); //the start function prototype

typedef struct _OUTPUT_LIST
{
    U8*   (*LibNameStr)(void);
    U32   (*WriteBuffer)(U8 *pIn, U32 ulSize);
    U32   (*ReadBuffer)(U8 *pOut, U32 ulSize);
    U32   (*GetBufferRoom)(void);
} OUTPUT_LIST;

typedef struct _INPUT_LIST
{
    void (*pPrintfPtr)(const char *fmt, ...);
}INPUT_LIST;

enum CommandType
{
    Idle = 1,
    Terminate,
    Loaded,
    LibDone
};

typedef struct _command_struct
{
    enum CommandType   type;
    void              *arg;
} Command;


#endif /* __COMM_DEF_H__ */
