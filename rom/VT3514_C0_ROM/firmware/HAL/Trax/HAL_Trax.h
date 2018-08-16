/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    : HAL_Trax.h
Version     : Ver 1.0
Author      : tobey
Date        : 2014.11.20
Description : this file declare Trax driver interface. 
Others      : 
Modify      :
20141120    Tobey     Create file
****************************************************************************/

#ifndef __HAL_TRAX_H__
#define __HAL_TRAX_H__

#define ERI_ADDR_BASE (0x100000)

#define PWRCTL        (ERI_ADDR_BASE + 0x3020)
#define PWRSTAT       (ERI_ADDR_BASE + 0x3024)
#define ERISTAT       (ERI_ADDR_BASE + 0x3028)

#define TRAX_ID        (ERI_ADDR_BASE + 0x0000)
#define TRAX_CTRL      (ERI_ADDR_BASE + 0x0004)
#define TRAX_STAT      (ERI_ADDR_BASE + 0x0008)
#define TRAX_DATA      (ERI_ADDR_BASE + 0x000C)
#define TRAX_ADDR      (ERI_ADDR_BASE + 0x0010)
#define TRAX_TRIGGERPC     (ERI_ADDR_BASE + 0x0014)
#define TRAX_PCMATCHCTRL   (ERI_ADDR_BASE + 0x0018)
#define TRAX_DELAYCOUNT    (ERI_ADDR_BASE + 0x001C)
#define TRAX_MEMSTARTADDR  (ERI_ADDR_BASE + 0x0020)
#define TRAX_MEMENDADDR    (ERI_ADDR_BASE + 0x0024)

/*  below registers are internal only!:  */ 
#define  TRAX_DEBUGPC    (ERI_ADDR_BASE + 0x003C)    /*[0x0F] Debug PC */
#define  TRAX_P4CHANGE   (ERI_ADDR_BASE + 0x0040)    /*[0x10] X */
#define  TRAX_P4REV      (ERI_ADDR_BASE + 0x0044)    /*[0x11] X */
#define  TRAX_P4DATE     (ERI_ADDR_BASE + 0x0048)    /*[0x12] X */
#define  TRAX_P4TIME     (ERI_ADDR_BASE + 0x004C)    /*[0x13] X */
#define  TRAX_PDSTATUS   (ERI_ADDR_BASE + 0x0050)    /*[0x14] Sample of PDebugStatus */
#define  TRAX_PDDATA     (ERI_ADDR_BASE + 0x0054)    /*[0x15] Sample of PDebugData */
#define  TRAX_STOP_PC    (ERI_ADDR_BASE + 0x0058)    /*[0x16] X */
#define  TRAX_STOP_ICNT  (ERI_ADDR_BASE + 0x005C)    /*[0x16] X */
#define  TRAX_MSG_STATUS (ERI_ADDR_BASE + 0x0060)    /*[0x17] X */
#define  TRAX_FSM_STATUS (ERI_ADDR_BASE + 0x0064)    /*[0x18] X */
#define  TRAX_IB_STATUS  (ERI_ADDR_BASE + 0x0068)    /*[0x19] X */
#define  TRAX_STOPCNT    (ERI_ADDR_BASE + 0x006C)    /*[0x1A] X */


#define TRACERAMSIZE (32<<10) 
#define m 13 //TRACERAMSIZE determine the MEMSIZE, m = MEMSIZE - 2; may be 7~16

#define TRAXRAM_SIZE_MIN (128)
#define TRAXRAM_SIZE_MAX (1024*1024*1024)

/* Minimum size between start and end addresses */
#define    TRAX_MIN_TRACEMEM    64

#define POSTSIZE_ADJUST 32      /* max number of bytes of trace output after
                                   post-trigger countdown reaches 0, for CNTU
                   setup to count words; must be multiple of 4
                 */

#define TRAX_READABLE_REG_NUM 18

#if 0
typedef struct _PWRCTLREG
{
   U32 bsShutCoreOffOnPWait:1; // R/W (PSO) Core request to shut off upon WAITI (that is,when asserting PWaitMode)
   U32 bsRsv0:7;
   U32 bsMemWakeup:1;          // R/W (PSO) Request power to Memory domain
   U32 bsRsv1:3;
   U32 bsDebugWakeup:1;        // R/W PCM (PSO) Request power to Debug Module
   U32 bsRsv2:15;   
   U32 bsDebugReset:1;         // R/W Setting to 1 asserts reset to the Xtensa Debug Module!
   U32 bsRsv3:3;
}PWRCTLREG;

typedef struct _PWRSTATREG
{
   U32 bsCoreDomainOn:1;      // RO (PSO) Set if core is reported as powered on; although
                              // only readable by the core, thus normally always 1, it
                              // reads 0 after wake-up before software clear PWRCTL.ShutProcOffOnPWait
   U32 bsWakeupReset:1;       // RO (PSO) Distinguishes core reset caused by PSO
                              // wakeup (1) vs. cold start (0)
   U32 bsCachesLostPower:1;   // RO (PSO) Set if memory domain was powered down while the core was shut off
   U32 bsRsv0:1;
   U32 bsCoreStillNeeded:1;   // RO (PSO) Set if any core wakeup signal is set; indicates
                              // some agent still requires power to the core
   U32 bsRsv1:3;
   U32 bsMemDomainOn:1;       // RO (PSO) Set if memory domain is powered on
   U32 bsRsv2:3;
   U32 bsDebugDomainOn:1;     // RO (PSO) Set if debug domain is powered on
   U32 bsRsv3:15;
   U32 bsDebugWasReset:1;     // R/clr Debug Module was reset since last time this bit was cleared
   U32 bsRsv4:3;
}PWRSTATREG;

typedef struct _ERISTATREG
{
   U32 bsWRSUC:1;  //R/W ERI write success indication
   U32 bsRsv:31;
}ERISTATREG;
#endif

typedef struct _TRAXIDREG
{
    U32 usCFGID:16;  //Configuration ID. The value of this field is unique for each possible configuration of the TRAX unit
    U32 bsSTDCFG:1;  //set:Tensilica Standard configuration; clear:custom-configured by the customer;
    U32 bsMINVER:3;  //Minor version number. For example, for version 2.0, MINVER is 0.
    U32 bsMAJVER:4;  //Major version number. For example, for version 2.0, MAJVER is 2.
    U32 bsPRODOPT:4; //Product specific options.
    U32 bsPRODNO:4;  //Product number (identifies device class/type). 0:TRAX; 1~15:rsv;  
}TRAXIDREG;

typedef struct _TRAXCTRLREG
{
    /*byte0*/
    U32 bsTREN:1;    //Trace enable
    U32 bsTRSTP:1;   //Trace stop. When this bit is set while TRAXSTAT.TRACT is set and TRAXSTAT.TRIG is clear, trace stop is triggered.
    U32 bsPCMEN:1;   //PC match enable
    U32 bsRsv5:1;    
    U32 bsPTIEN:1;   //Processor trigger input enable
    U32 bsCTIEN:1;   //Cross-trigger input enable.
    U32 bsRsv4:1;    
    U32 bsTMEN:1;    //TraceRAM enable. Enables local trace memory. Always set.   

    /*byte1*/
    U32 bsRsv3:1;
    U32 bsCNTU:1;    //Post-stop-trigger countdown units, 0, DelayCount counts down once for each 32-bit word written to the TraceRAM
                     //1, DelayCount counts down once for each processor instruction executed and for each exception and interrupt taken.
    U32 bsRsv2:1;     
    U32 bsTSEN:1;    
    U32 bsSMPER:3;   //Synchronization message period.0,none; 1,1 every 256; 2,1 every 128; 
                     //3,1 every 64; 4,1 every 32; 6,1 every 8; 7,(reserved);
    U32 bsRsv1:1;

    /*byte2*/
    U32 bsPTOWHEN:2;   //Processor Trigger Output (PTO) is enabled when stop triggered
                       //Processor Trigger Output (PTO) is enabled when trace stop completes
    U32 bsRsv0:2;    
    U32 bsCTOWHEN:2;   //Cross-Trigger Output (CTO) is enabled when trace stop completes.
                     //Cross-Trigger Output (CTO) is enabled when trace stop completes.
    U32 bsITCTO:1;   //Integration mode: cross-trigger output (CrossTriggerOut).   
    U32 bsITCTIA:1;  //Integration mode: cross-trigger input acknowledge (CrossTriggerInAck).

    /*byte3*/
    U32 bsATIDOrITATV:7; //ATB source ID.Output directly as the ATID signal (if ATB configured).
                         //In integration mode, this field becomes ITATV (ATID[0]): the ATVALID output value. 
                         //The ITMODE bit of the ITCTRL register controls this as described in Section 4.1.
    U32 bsATEN:1;        //ATB enable. When set, trace output is sent out on the ATB interface (if ATB configured).
}TRAXCTRLREG;

typedef struct _TRAXSTATREG
{
    /*byte0*/
    U32 bsTRACT:1;
    U32 bsTRIG:1;
    U32 bsPCMTG:1;
    U32 bsPJTR:1;    
    U32 bsPTITG:1;
    U32 bsCTITG:1;
    U32 bsRsv0:2;

    /*byte1*/
    U32 bsMEMSZ:5; 
    U32 bsRsv1:3;

    /*byte2*/
    U32 bsPTO:1;
    U32 bsCTO:1;
    U32 bsRsv2:4;     
    U32 bsITCTOA:1;
    U32 bsITCTI:1;

    /*byte3*/
    U32 bsITATR:1;
    U32 bsRsv3:1;
}TRAXSTATREG;

typedef struct _TRAXADDRREG
{
    U32 bsTADDR:m;
    U32 bsRSV:21-m;
    U32 bsTWRAP:10;
    U32 bsTWSAT:1;
}TRAXADDRREG;

typedef struct _PCMATCHCTRLREG
{
    U32 bsPCML:5;//PC Match Mask Length. Indicates how many of the lower bits of the Trigger PC register to ignore.
    U32 bsRsv:26;
    U32 bsPCMS:1;//0, match when the processor¡¯s PC is in-range of the Trigger PC register and mask.
                 //1, match when the processor¡¯s PC is out-of-range of Trigger PC register and mask.
}PCMATCHCTRLREG;

#if 0
typedef struct _TRAX_CONTEXT
{
  U16   usTraxVersion;        /* TRAX PC version information */
  U32   ulTraxTramSize;        /* If trace RAM is present,size of it */
  #if 0
  int        hflags;            /* Flags that can be used to debug, 
                         print info, etc. */
  int        address_read_last;    /* During saving of the trace, this
                         indicates the address from which
                       the current trace reading must
                       resume */
  unsigned long    bytes_read;        /* bytes read uptil now */
  unsigned long total_memlen;        /* Total bytes to be read based on the 
                             trace collected in the trace RAM */
  bool        get_trace_started;    /* indicates that the first chunk of
                         bytes (which include the header) has
                       been read */
  #endif
} TRAX_CONTEXT;
#endif

typedef struct _TRAX_CONTEXT
{
  U16   usTraxVersion;        /* TRAX PC version information */
  U32   ulTraxTramSize;        /* If trace RAM is present,size of it */
  
  U32   ulFlags;            /* Flags that can be used to debug, 
                         print info, etc. */
  U16   usAddrReadLast;    /* During saving of the trace, this
                         indicates the address from which
                       the current trace reading must
                       resume */
  U32   ulTotalMemlen;        /* Total bytes to be read based on the 
                             trace collected in the trace RAM */
  U16 usStartAddr;
  U16 usEndAddr;

} TRAX_CONTEXT;


#define TRAX_FHEAD_MAGIC    "TRAXdmp"
#define TRAX_FHEAD_VERSION    1

/*  Header flags:  */
#define TRAX_FHEADF_OCD_ENABLED         0x00000001    /* set if OCD was enabled while capturing trace */
#define TRAX_FHEADF_TESTDUMP            0x00000002    /* set if is a test file 
                                                                   (from 'memsave' instead of 'save') */
#define TRAX_FHEADF_OCD_ENABLED_WHILE_EXIT     0x00000004    /* set if OCD was enabled while capturing trace and
                                   we were exiting the OCD mode */

#define TRAX_NUM_PER_READ    256  //by Bytes
/*  Header at the start of a TRAX dump file.  */
typedef struct _TRAX_FILE_HEAD
{
    U8    aMagic[8];    /* 00: "TRAXdmp\0" (TRAX_FHEAD_MAGIC) */
    U8    ucEndianness;    /* 08: 0=little-endian, 1=big-endian */
    U8    ucVersion;    /* 09: TRAX_FHEAD_VERSION */
    U8    ucRsv0[2];    /* 0A: ... */
    U32   ulFileSize;    /* 0C: size of the trace file, including this header in bytes */
    U32   ulTraceOffset;    /* 10: start of trace output, byte offset from start of header */
    U32   ulTraceSize;    /* 14: size of trace output in bytes */
    U32   ulDumpTime;    /* 18: date/time of capture save (secs since 1970-01-01), 0 if unknown */
    U32   ulFlags;        /* 1C: misc flags (TRAX_FHEAD_F_xxx) */
    U8    ucUserName[16];    /* 20: user doing the capture/save (up to 15 chars) */
    U8    ucToolVer[24];    /* 30: tool + version used for capture/save (up to 23 chars) */
    U8    ucRsv2[40];    /* 48: (reserved - could be hostname used for dump (up to 39 chars)) */
    U32   ulConfigId[2];    /* 70: processor ConfigID values, 0 if unknown */
    U32   ulRsv3[2];    /* 78: (reserved) */
    U32   ulId;        /* 80: TRAX registers at time of save (0 if not read) */
    U32   ulControl;
    U32   ulStatus; 
    U32   ulRsv4;    /* Data register (should not be read) */
    U32   ulAddress;
    U32   ulTrigger; 
    U32   ulMatch;
    U32   ulDelay;
    U32   aTraxRegs[24];    /*100: (total size) -- dummy allocation (FIXME) */
} TRAX_FILE_HEAD;


#define TRACE_MXDSRAM1_3  //when defined:max trace memory size 8kB, if not max trace use MxDSRAM1_2 memory, size 32kB

#define TRACE_RAM_SIZE_MULTICORE  (2<<10) //(8192)
#define TRACE_RAM_SIZE_SINGLECORE (8<<10)
#endif

