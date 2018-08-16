#include "OverlayLibDef.h"
#include "Proj_Config.h"
#include "COM_Memory.h"
#include "uart.h"

#define LIB1_BACKUP_START_ADDR  0x40001000
#define LIB2_BACKUP_START_ADDR  0x40002000

#define  LIBRARY_ENRTY_ADDR    0x40000000
#define  LIBRARY_LOCATION_ADDR 0x40000000
#define  LIBRARY_SIZE          0x1000

OUTPUT_LIST  LibFuncList;
OUTPUT_LIST *pLibFuncList=&LibFuncList;
INPUT_LIST tInputParam;
void OverlayLibTestMain(void)
{
    LibEntry  *pEntry;
    U8 array[16];
    U8  i;

    uart_init();
    pEntry = (LibEntry*)(LIBRARY_ENRTY_ADDR);

    DBG_Printf("Main Thread starts\n\n");

    /* Load fixed-location lib1 */
    DBG_Printf("Library 1 Loading ... ...\n");
    COM_MemCpy((U32 *) LIBRARY_LOCATION_ADDR, (U32 *)LIB1_BACKUP_START_ADDR, LIBRARY_SIZE>>2);
    
    /* Test fixed-location lib1 */
    tInputParam.pPrintfPtr = DBG_Printf;
    pEntry(pLibFuncList, &tInputParam);
    DBG_Printf("Buffer Size of \"%s\" is %d\n", pLibFuncList->LibNameStr(), pLibFuncList->GetBufferRoom());
    for(i=0; i<16; i++)
    {
        array[i] = 0x10 + i;
    }
    pLibFuncList->WriteBuffer(array, 16);
    COM_MemZero((U32 *)array, 16>>2);
    pLibFuncList->ReadBuffer(array, 16);
    
    
    /* load fixed-location lib2 */
    DBG_Printf("Library 2 Loading ... ...\n");
    COM_MemCpy((U32 *) LIBRARY_LOCATION_ADDR, (U32 *)LIB2_BACKUP_START_ADDR, LIBRARY_SIZE>>2);
    
    /* Test the fixed-location lib2 */
    pEntry = (LibEntry*)(LIBRARY_ENRTY_ADDR);
    pEntry(pLibFuncList, &tInputParam);
    DBG_Printf("Buffer Size of \"%s\" is %d\n", pLibFuncList->LibNameStr(), pLibFuncList->GetBufferRoom());
    for(i=0; i<16; i++)
    {
        array[i] = 0x20 + i;
    }
    pLibFuncList->WriteBuffer(array, 16);
    COM_MemZero((U32 *)array, 16>>2);
    pLibFuncList->ReadBuffer(array, 16);

    return;
}
