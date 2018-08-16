// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

// TODO: reference additional headers your program requires here

#define MAX_NUM_DEVICES 50


#define Boolean uint32_t
#define false 0
#define true 1


extern int serverPort;
extern int server_fd;

extern int vpi_fd; // should be the descriptor for our connection to the VPI server

extern int current_chain;
extern int dbg_chain;

#define DBGCHAIN_SIZE           4 // Renamed from DC_SIZE due to definition clash with something in <windows.h> --jb 090302
#define DC_STATUS_SIZE    4

#define DC_WISHBONE       0
#define DC_CPU0           1
#define DC_CPU1           2

// Manually figure the 5-bit reversed values again if they change
#define DI_GO          0
#define DI_READ_CMD    1
#define DI_WRITE_CMD   2
#define DI_READ_CTRL   3
#define DI_WRITE_CTRL  4

#define DBG_CRC_SIZE      32
#define DBG_CRC_POLY      0x04c11db7

#define DBG_ERR_OK        0
#define DBG_ERR_INVALID_ENDPOINT 3
#define DBG_ERR_CRC       8

#define NUM_SOFT_RETRIES  3
#define NUM_HARD_RETRIES  3
#define NUM_ACCESS_RETRIES 10

// TODO: reference additional headers your program requires here
