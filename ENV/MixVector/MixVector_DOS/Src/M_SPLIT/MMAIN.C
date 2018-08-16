/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "DosBaseType.h"
#ifdef DOS_ENV
#include <i86.h>
#endif
#include <string.h>
#ifdef DOS_ENV
#include <strings.h>
#endif

#ifdef DOS_ENV
#include "pci.h"
#endif
#include "mmain.h"
#ifndef DOS_ENV
#include "Proj_COnfig.h"
#endif
#ifdef DOS_ENV
#define DBG_Printf printf
#endif
#ifndef DOS_ENV
extern BOOL l_aCommandPending[MAX_CMD_SLOT];
#endif
#define DISK_A_B_C_MULTI
//#define DISK_B_WT_ONLY
#define DISK_START_TYPE DISK_A
#define DISK_B_WT_SPLIT
#define DYNAMIC_HSG_SPLIT

#define PG_SZ 32*1024
#define PAYLOAD_INIT_VAL 0x55aa55aa
#define CHECK_CMD_FINISH_DELAY 100000
#define DATA_RANDON_OFFSET_BIT 0
#define NOPCNT 1000
//#define DISK_C_INITIAL
//#define DISK_C_ENABLE
//#define DISK_B_ENABLE
#define DISK_A_ENABLE

#define MIN_C_HSG_BYTE_LEN (2)
#define MIN_B_HSG_BYTE_LEN 2
#define HSG_CNT 64


#define TEST_B_OFFSET 0
#define TEST_B_SIZE 128*1024*1024
#define TEST_B_MAX_CMD_LEN 32

#define TEST_A_OFFSET 0
#define TEST_A_SIZE 4*1024
#define TEST_A_MAX_CMD_LEN 16

#define TEST_C_START_BLK 128
#define TEST_C_BLK_CNT 2
#define TEST_C_DISK_OFFSET 0

#define TEST_C_DISK_LEN (2*512*64)//sector
#define TEST_C_DISK_MAX_SEC 64//sector
#define DISK_C_WT_DELAY 20//ms
#define TEST_CE_NUM (1<<CE_BITS)//2

#define PAGE_PER_BLOCK  512
#define SEC_PER_PG 64
#define SEC_PER_PG_BITS 6
#define CE_BITS 1
#define PG_BITS 9
#define BLOCK_PER_CE 1024

#ifndef DOS_ENV
extern void AHCI_HostCClearCmdFinishFlag(U8 nTag);
extern void AHCI_HostCSetPxCI(U8 CMDTag);
extern int MV_Schedule(void);
extern void AHCI_HostCModelSchedule();
extern void NFC_ModelSchedule();
extern void SGE_ModelSchedule(void);
#endif

AHCI_EXT g_aAhciExt;
#if 1
ULONG HWBATable[ SUB_DISK_CNT ] = {
    DISK_A_HW_ADDRESS,
    DISK_B_HW_ADDRESS,
    DISK_C_HW_ADDRESS
};
#endif


extern unsigned char g_pHostDiskA[24<<10];    // as memory on host side. allocated in AHCI_HostInit
extern unsigned char g_pHostDiskB[128<<20];    // as memory on host side.
extern unsigned char g_pHostDiskC[128<<20];    // as memory on host side.
extern HOST_CMD g_pMvHCmdTable[MAX_CMD_SLOT];
#ifndef DOS_ENV


void SwInitWin(PAHCI_EXT AhciExt)
{
    HWBATable[SUB_DISK_A] = (ULONG)(((ULONG)(g_pHostDiskA+512)>>9)<<9);
    HWBATable[SUB_DISK_B] = (ULONG)(((ULONG)(g_pHostDiskB+512)>>9)<<9);
    HWBATable[SUB_DISK_C] = (ULONG)(((ULONG)(g_pHostDiskC+512)>>9)<<9);

    AhciExt->HostCmdTable = (PHOST_CMD)g_pMvHCmdTable;
    AhciExt->DiskAAddrSw = (UCHAR *)HWBATable[SUB_DISK_A];
    AhciExt->DiskBAddrSw = (UCHAR *)HWBATable[SUB_DISK_B];
    AhciExt->DiskCAddrSw = (UCHAR *)HWBATable[SUB_DISK_C];

}
void LogMsg( ULONG _Comp,ULONG  _Level, const char* fmt, ... )
{
    return;
}
#endif

ULONG ulCmdManager[MAX_CMD_SLOT];
ULONG ulSlotTime[MAX_CMD_SLOT];
VOID MixvSetCmdStatus(ULONG Tag,CMD_STATUS Status)
{
    ulCmdManager[Tag] = Status;
}
VOID MixvInitTimeSlot()
{
    ULONG Tag;
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
        ulSlotTime[Tag] = 0;
}
#ifdef DOS_ENV
void DBG_Getch()
{
    ULONG loop;
    loop = 0;
    while(1)
    {
    //if((loop%500000000)==0)
        delay(100);
        DBG_Printf("*");
    loop++;
    }
}
#endif
extern STATUS SwInitDos();
STATUS MixvInit()
{

#ifdef DOS_ENV
    STATUS ret;
    ret = LogOpen( LOG_FILE_NAME );

    if ( 0 != ret )
    {
        printf("Failed to open log file!\n");
        return ret;
    }
#endif

    InitAhciExt( &g_aAhciExt );
//    ret = CheckViaAhciDevice();


#ifdef DOS_ENV
    ret = SwInitDos();
    if(ret==UNSUCCESSFUL)
    {
    return ret;
    }
#else
    SwInitWin(&g_aAhciExt);
#endif
    return SUCCESS;
}

void InitAhciExt( PAHCI_EXT AhciExt )
{
    memset( (PVOID)AhciExt, 0, sizeof( AHCI_EXT ) );
    AhciExt->PciDevInfo.VendorID = VIA_AHCI_VENDOR_ID;
    AhciExt->PciDevInfo.DeviceID = VIA_AHCI_DEVICE_ID;
}

STATUS HwTerminate( PAHCI_EXT AhciExt )
{
    return SUCCESS;
}

PCHAR CmdToString( HOST_REQ_TYPE Type )
{
    switch( Type )
    {
    case HOST_REQ_READ:
        return "READ";
    case HOST_REQ_WRITE:
        return "WRITE";
    case HOST_REQ_ERASE:
        return "ERASE";
    case HOST_REQ_OTHER:
        return "OTHER";
    case HOST_REQ_INVALID:
        return "INVLD";
    default:
        return "U/A";
    }
}
void TestInit()
{
    ULONG tag;
    MixvInitTimeSlot();

    for(tag=0;tag<MAX_CMD_SLOT;tag++)
        ulCmdManager[tag] = 0;
}
STATUS BuildDiskABCmd(PHOST_CMD HostCmd,UCHAR DiskIndex,ULONG Tag, HOST_REQ_TYPE Type, ULONG Offset, ULONG Length )
{

    memset(HostCmd,0,sizeof(HOST_CMD));
    HostCmd->SubDiskCmd[ DiskIndex ].DiskEn       = 1;
    HostCmd->SubDiskCmd[ DiskIndex ].ReqType      = Type;


      HostCmd->SubDiskCmd[ DiskIndex ].UnitLength   = Length;


    HostCmd->SubDiskCmd[ DiskIndex ].HostAddrLow  = HWBATable[ DiskIndex ] +( ( DiskIndex == SUB_DISK_A ) ? (Offset) : ( Offset << 9 ) );
    HostCmd->SubDiskCmd[ DiskIndex ].HostAddrHigh  = 0;
    HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = Offset;
    memcpy(&g_aAhciExt.HostCmdTable[Tag],HostCmd,sizeof(HOST_CMD));
//    LogMsg(COMP_HW,DBG_WARNING,"SLOT:%d CMD ADDR:0x%x\n",Tag,ptr);
    HostCmd->FinishCnt[DiskIndex] = Length;
    #ifndef DOS_ENV
            MixvSetCmdStatus(Tag,CMD_BUSY);
            AHCI_HostCSetPxCI( Tag );
    #endif
    return SUCCESS;
}

//for disk c erase length :flash cmd cnt
//offset : means sector offset
STATUS BuildDiskCCmd(PHOST_CMD HostCmd,UCHAR Tag,HOST_REQ_TYPE Type, ULONG Offset,ULONG Length)
{
    HostCmd->SubDiskCmd[ SUB_DISK_A ].DiskEn       = 0;
    HostCmd->SubDiskCmd[ SUB_DISK_B ].DiskEn       = 0;
    HostCmd->SubDiskCmd[ SUB_DISK_C ].DiskEn       = 1;
    HostCmd->SubDiskCmd[ SUB_DISK_C ].ReqType      = Type;
    HostCmd->SubDiskCmd[ SUB_DISK_C ].UnitLength   = Length;//for ers: flash cnt for wt rd: sec length
        #ifndef DOS_ENV
            MixvSetCmdStatus(Tag,CMD_BUSY);
            AHCI_HostCSetPxCI( Tag );
        #endif
    HostCmd->SubDiskCmd[ SUB_DISK_C ].HostAddrLow  = HWBATable[ SUB_DISK_C ]+Tag*FLASH_ADDR_CNT*PG_SZ*TEST_CE_NUM;// + (Offset<<9);//( ( DiskIndex == SUB_DISK_A ) ? Offset : ( Offset << 9 ) );
    HostCmd->SubDiskCmd[ SUB_DISK_C ].HostAddrHigh  = 0;
//    HostCmd->SubDiskCmd[ SUB_DISK_C ].StartUnit = Offset%SEC_PER_PG;//disk offset
    if(Type == HOST_REQ_ERASE)
    {
        HostCmd->SubDiskCmd[ SUB_DISK_C ].UnitLength   = TEST_CE_NUM;
    }
    memcpy(&g_aAhciExt.HostCmdTable[Tag],HostCmd,sizeof(HOST_CMD));
    //DBG_Printf("send startunit:0x%x len:0x%x\n",Offset,g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[SUB_DISK_C].UnitLength);
    g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[SUB_DISK_C].StartUnit = Offset%SEC_PER_PG;
    return SUCCESS;
}
#ifndef DOS_ENV
extern U8 AHCI_HostCGetPxCI(U8 CMDTag);
#else
UCHAR AHCI_HostCGetPxCI(UCHAR CMDTag)
{
    ULONG res;
//    LogMsg(COMP_HW,DBG_WARNING,"Ci:0x%x\n",*g_aAhciExt.PxCIRegAddress)
//    DBG_Printf("PxCI:0x%x\n",*g_aAhciExt.PxCIRegAddress);
    res = ((*g_aAhciExt.PxCIRegAddress)&(1<<CMDTag));
    if(res)
        return 1;
    return 0;
}
#endif
#if 0
ULONG MixvGetEmptySlot()
{
    UCHAR i;
    for(i=0;i<MAX_CMD_SLOT;i++)
    {
    if(MixvGetCmdStatus(i)!=CMD_BUSY)
    {
        if(AHCI_HostCGetPxCI(i)==0)
            return i;
    }
    }
    return INVALID_8F;

}
#endif


VOID MixvSetCmdTimStamp(ULONG Tag)
{
    ulSlotTime[Tag]++;
    if(ulSlotTime[Tag]>100000)
    {
        DBG_Printf("slot %d timeout!!\n",Tag);

        DBG_Getch();
    }
    return;
}
#if 0
VOID MixvSetCmdStatus(ULONG Tag,CMD_STATUS Stat)
{
    ulCmdManager[Tag] = Stat;
}
#endif
CMD_STATUS MixvGetCmdStatus(ULONG Tag)
{
    return ulCmdManager[Tag];
}
ULONG gSlotPos=0;
#if 1

ULONG MixvGetEmptySlot()
{
    UCHAR i;
    for(i=0;i<MAX_CMD_SLOT;i++)
    {
        if(MixvGetCmdStatus(i)!=CMD_BUSY)
        {
            if((AHCI_HostCGetPxCI(i)==0))
            {
                return i;
            }
        }
    }
    return INVALID_8F;

}
#endif
VOID MixvInitCmdStatus()
{
    ULONG Tag;
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
        MixvSetCmdStatus(Tag,CMD_IDLE);
    }
}
VOID MixvCheckCmd(ULONG * ulCompleteCmd)
{
    ULONG Tag;
    *ulCompleteCmd = 0;

    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {

        if(MixvGetCmdStatus(Tag)==CMD_BUSY)
        {

            if(AHCI_HostCGetPxCI(Tag)==0)
            {

            LogMsg(COMP_HW,DBG_WARNING,"Finish cmd %d\r\n",Tag);
                MixvSetCmdStatus(Tag,CMD_FINISH);
                *ulCompleteCmd|=(1<<Tag);
        ulSlotTime[Tag] = 0;
            }
            else
            {
                MixvSetCmdTimStamp(Tag);
            }
        }
    }
    return;
}
VOID MixvZeroPayLoad(ULONG Tag,UCHAR DiskIndex,ULONG Offset,ULONG Length)
{
    ULONG Addr;
    ULONG i,Len;
    if(DiskIndex==SUB_DISK_A)
    {
        Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);
        Len = Length;
    }
    else if(DiskIndex==SUB_DISK_B)
    {
        Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
        Len = Length*SECTOR_SZ;
    }
    else if(DiskIndex==SUB_DISK_C)
    {
        Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
        Len = Length*SECTOR_SZ*TEST_CE_NUM;
    }

//    if(DiskIndex==SUB_DISK_B)
    Len=(Len>>2);
    for(i=0;i<(Len);i++)
    {
        *(volatile ULONG *)Addr = PAYLOAD_INIT_VAL;
        Addr+=4;
    }
}
VOID MixvFillPayLoad(ULONG Tag,UCHAR DiskIndex,ULONG Offset,ULONG Length)
{
    ULONG Addr;
    ULONG i,Len;

    if(DiskIndex==SUB_DISK_A)
    {
        Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);
        Len = Length;
    }
    else if(DiskIndex==SUB_DISK_B)
    {
        Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
        Len = Length*SECTOR_SZ;
    }
    else if(DiskIndex==SUB_DISK_C)
    {
        Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
        Len = Length*SECTOR_SZ;
    }


    if(DiskIndex!=SUB_DISK_C)
    {
        for(i=0;i<(Len>>2);i++)
        {
            *(volatile ULONG *)Addr = ((Offset)<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));
            Addr+=4;
        }
    }
    else
    {

//        DBG_Printf("fill payloade addr:0x%x data:0x%x\n",Addr,(Offset));
        for(i=0;i<(Len>>2);i++)
        {
            *(volatile ULONG *)Addr = (Offset+i/128);//(((Offset&0xffff)<<16)|(Offset&0xffff));
            //if((0xde23ef20==Addr))
            //    DBG_Printf("!\n");
            //if(i/128>0)
            //    DBG_Printf("!\n");
            Addr+=4;
        }

    }
 //   DBG_Printf("fill payloade addr:0x%x data:0x%x",addr,*addr);
}
#ifndef DOS_ENV
extern void sim_mix_clear_host_transbyte(U8 slot);
#endif
ULONG MixvCheck_DiskC_CmdFinish(ULONG * MskFinishCmd,UCHAR DiskIndex,HOST_CMD * HostCmdTable,HOST_REQ_TYPE Type)
{
    ULONG Addr;
    ULONG Offset,Length,FinishCnt,Tag,Ret,nop;

    Ret = 0;
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
        if(((*(volatile ULONG *)MskFinishCmd)&(1<<Tag)))
        {

            Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
            Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength;

//            FinishCnt = g_aAhciExt.HostCmdTable[Tag].FinishCnt[DiskIndex];

            FinishCnt = HostCmdTable[Tag].FinishCnt[DiskIndex];

#if 0
            if(Type!=HOST_REQ_WRITE)
                FinishCnt = g_aAhciExt.HostCmdTable[Tag].FinishCnt[DiskIndex];
            else
                FinishCnt = HostCmdTable[Tag].FinishCnt[DiskIndex];
#endif


            Ret+=FinishCnt;
            if(Type==HOST_REQ_ERASE)
            {
            //mark bad blk
            }
            else if(Type==HOST_REQ_WRITE)
            {
                //DBG_Printf("wt finish\n");
            }
            else if(Type==HOST_REQ_READ)
            {
            }
            else
            {

            }

            for(nop=0;nop<CHECK_CMD_FINISH_DELAY;nop++);
            if(FinishCnt==0)
            {
                DBG_Printf("FinishCnt==0 1st\n");
                for(nop=0;nop<CHECK_CMD_FINISH_DELAY;nop++);

                FinishCnt = g_aAhciExt.HostCmdTable[Tag].FinishCnt[DiskIndex];
                if(FinishCnt==0)
                {

                        DBG_Printf("slot:%d offset:%dsec finishcnt=0\n",Tag,Offset);

                    Offset = HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                    Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength*SECTOR_SZ;
            //DBG_Printf("Tag:%d Read C Offset:0x%x\n",Tag,Offset);
                    Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Offset*SECTOR_SZ);
            DBG_Printf("addr:0x%x val:0x%x\n",Addr,*(ULONG *)Addr);
                        DBG_Getch();
                }
            }
         }
        //Ret+=FinishCnt;

#ifndef DOS_ENV
            sim_mix_clear_host_transbyte(Tag);

#endif
#ifdef DBG_INFO
            DBG_Printf("Disk:%d cmd tag:%d offset:0x%x length:0x%x finishcnt:0x%x\n",DiskIndex,Tag,Offset,Length,FinishCnt);
#endif
            LogMsg(COMP_HW,DBG_WARNING,"cmd tag:%d finish",Tag);
        }


          return Ret;
}

ULONG MixvCheckCmdFinish(ULONG * MskFinishCmd,UCHAR DiskIndex,HOST_CMD * HostCmdTable,HOST_REQ_TYPE Type)
{
    ULONG Offset,Length,FinishCnt,Tag,Ret,nop;
    Ret = 0;
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
        if(((*(volatile ULONG *)MskFinishCmd)&(1<<Tag)))
        {

//    delay(1);
//    for(nop=0;nop<CHECK_CMD_FINISH_DELAY;nop++);
            Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
            Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength;
            FinishCnt = g_aAhciExt.HostCmdTable[Tag].FinishCnt[DiskIndex];
            #ifndef DOS_ENV
            sim_mix_clear_host_transbyte(Tag);
            #endif
            //FinishCnt = HostCmdTable[Tag].FinishCnt[DiskIndex];
            if(FinishCnt==0)
            {
                DBG_Printf("FinishCnt==0 1st\n");
                for(nop=0;nop<CHECK_CMD_FINISH_DELAY;nop++);

                 FinishCnt = g_aAhciExt.HostCmdTable[Tag].FinishCnt[DiskIndex];
                if(FinishCnt==0)
                {
                        DBG_Printf("slot:%d offset:%dsec finishcnt=0\n",Tag,Offset);
                        DBG_Getch();
                }
            }

        Ret+=FinishCnt;
         }
//        Ret+=FinishCnt;

#ifndef DOS_ENV
            sim_mix_clear_host_transbyte(Tag);

#endif
#ifdef DBG_INFO
            DBG_Printf("Disk:%d cmd tag:%d offset:0x%x length:0x%x finishcnt:0x%x\n",DiskIndex,Tag,Offset,Length,FinishCnt);
#endif
            LogMsg(COMP_HW,DBG_WARNING,"cmd tag:%d finish",Tag);
        }


    if(DiskIndex==SUB_DISK_A)
      return Ret;
    if(DiskIndex==SUB_DISK_B)
      return Ret/SECTOR_SZ;
    return INVALID_8F;
}
ULONG MixvCheckPayLoad(ULONG * MskFinishCmd,UCHAR DiskIndex,HOST_CMD * HostCmdTable/*ULONG Tag,ULONG Offset,ULONG Length*/)
{
    ULONG Tag,nop,sec;
    ULONG Addr;
    ULONG i,Offset,Length,totallen,len;
    ULONG DataSrc,DataActual;
    totallen = 0;
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
        if(((*(volatile ULONG *)MskFinishCmd)&(1<<Tag)))
        {
//        for(nop=0;nop<10000;nop++);
            if(DiskIndex==SUB_DISK_A)
            {
                Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength;

                Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);


                totallen+=Length;
            }
            else if(DiskIndex==SUB_DISK_B)
            {
                Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength*SECTOR_SZ;
                Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
                totallen+=Length;
            }
            else if(DiskIndex==SUB_DISK_C)
            {
                Offset = HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                Length =
                g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength*SECTOR_SZ;
                //DBG_Printf("Tag:%d Read C Offset:0x%x\n",Tag,Offset);
                Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
                totallen+=Length;
            }


        if(Length==0)
        {
            DBG_Printf("check len=0!!\n");
            DBG_Getch();
        }
            Length=(Length>>2);
        //DBG_Printf("check payload addr:0x%x data:0x%x\n",Addr,*(ULONG *)Addr);
        if(DiskIndex!=SUB_DISK_C)
        {
            for(i=0;i<(Length);i++)
            {
                DataSrc = *(volatile ULONG *)Addr;
                DataActual = (Offset<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));//(t<<24)|(Offset);
                if(DataSrc!=DataActual)
                {
                    DBG_Printf("check data err! 1st rd:0x%x actual:0x%x\n",DataSrc,DataActual);
                    for(nop=0;nop<1000;nop++);

                    DataSrc = *(volatile ULONG *)Addr;
                    if(DataSrc!=DataActual)
                    {
                        DBG_Printf("check data err! disk:%d hostaddr:0x%x cmd_slot:%d dword offset:0x%x err data:0x%x actual data:0x%x\n",DiskIndex,Addr,Tag,i,DataSrc,DataActual);

                        LogMsg( COMP_HW, DBG_WARNING,"check data err! cmd_slot:%d err data:0x%x actual data:0x%x\n",Tag,DataSrc,DataActual);

                        DBG_Getch();
                    }
                }
                else
                {
            //            DBG_Printf("check data ok! cmd_slot:%d err data:0x%x actual data:0x%x\n",Tag,DataSrc,DataActual);
                }
                Addr+=4;
            }
        }
        else
        {
//        DBG_Printf("c check addr:0x%x data:0x%x\n",Addr,(Offset));
            for(len=0;len<Length;len++)
            {
                DataSrc = *(volatile ULONG *)Addr;
                sec = len/128;
                DataActual = (Offset+sec);
                if(DataSrc!=DataActual)
                {
                    DBG_Printf("check data err! disk:%d hostaddr:0x%x cmd_slot:%d  err data:0x%x actual data:0x%x\n",DiskIndex,Addr,Tag,DataSrc,DataActual);
                    DBG_Getch();
                }
                Addr+=4;
            }
        }
//        #ifdef DBG_INFO
//            DBG_Printf("check data ok! cmd_slot:%d\n",Tag);
//        #endif
        }
    }
    if(DiskIndex==SUB_DISK_A)
        return totallen;
    if(DiskIndex==SUB_DISK_B)
        return totallen/SECTOR_SZ;
    if(DiskIndex==SUB_DISK_C)
        return totallen/SECTOR_SZ;
    return INVALID_8F;
}

VOID MixvShowStatus()
{
    DBG_Printf("pxci:0x%x\n",*g_aAhciExt.PxCIRegAddress);
    return;
}
VOID MixvLbaToFlash(FLASH_ADDR * p_flash_addr,U32 Lba,U32 LbaCnt)
{
    U32 StartPage,EndPage,Page,Index;

    StartPage = (((Lba>>SEC_PER_PG_BITS)<<SEC_PER_PG_BITS))>>SEC_PER_PG_BITS;

    EndPage = (((Lba+LbaCnt)>>SEC_PER_PG_BITS)<<SEC_PER_PG_BITS)>>SEC_PER_PG_BITS;

    if((Lba+LbaCnt)%(SEC_PER_PG))
    {
        EndPage++;
    }
    Index = 0;
    for(Page=StartPage;Page<(EndPage);Page++)
    {

        p_flash_addr[Index].PU = Page & (TEST_CE_NUM-1);
        p_flash_addr[Index].Block =TEST_C_START_BLK + (Page>>(CE_BITS+PG_BITS))&(BLOCK_PER_CE-1);
        p_flash_addr[Index].Page = (Page>>(CE_BITS))&(PAGE_PER_BLOCK-1);
	//if(p_flash_addr[Index].Page==0)
        //DBG_Printf("pu:%d blk:%d pg:%d\n",p_flash_addr[Index].PU,p_flash_addr[Index].Block,p_flash_addr[Index].Page);
        Index++;
        if(Index>FLASH_ADDR_CNT)
        {
            DBG_Getch();
        }
    }
    return;
}
VOID MixvDiskABRWCheck(HOST_REQ_TYPE Type,UCHAR DiskIndex,ULONG DiskOffset,ULONG DiskLen,ULONG MaxCmdLen)
{
    ULONG Disk_Length,Cmd_Cnt,Flag,Check_Len,Check_Finish_Len;
    ULONG Tag,SimTag,loop;
    ULONG Length,Offset;
    ULONG MskCmdComplete;
    HOST_CMD  HostCmdTable[MAX_CMD_SLOT];
    #ifdef DOS_ENV
    ULONG TagVal;
    #endif
#ifdef DOS_ENV
    sleep(1);
#endif
    //WRITE DISKA
    Cmd_Cnt = 0;
    Offset = DiskOffset;
    Disk_Length = DiskLen;//DISK_A_SIZE;
    MixvInitCmdStatus();
    TestInit();
    Flag = 0;
    Check_Len = 0;
    Check_Finish_Len = 0;
    loop=0;
    while(1)
    {
        MskCmdComplete = 0;
        MixvCheckCmd(&MskCmdComplete);
        if(MskCmdComplete!=0)
        {

            Check_Finish_Len+=MixvCheckCmdFinish(&MskCmdComplete,DiskIndex,HostCmdTable,Type);
            //check data
            if(Type == HOST_REQ_READ)
            {
                Check_Len+=MixvCheckPayLoad(&MskCmdComplete,DiskIndex,HostCmdTable);

                if(Check_Len==DiskLen)
                    return;

            }
        if(Type == HOST_REQ_WRITE)
        {
            LogMsg(COMP_HW,DBG_WARNING,"finish len:%d\n",Check_Finish_Len);
            if(Check_Finish_Len==DiskLen)
            {
                DBG_Printf("wt complete!!\n");
                return;
                }
            }
        }
        if(Disk_Length==0)
            continue;

        Tag = MixvGetEmptySlot();
        if(Tag!=INVALID_8F)
        {
            //BUILD CMD
            Length = (Disk_Length>MaxCmdLen?MaxCmdLen:Disk_Length);
            if(Length==0)
                continue;
            if(Type==HOST_REQ_WRITE)
            {
                MixvFillPayLoad(Tag,DiskIndex,Offset,Length);
            }
            else if(Type==HOST_REQ_READ)
            {
                MixvZeroPayLoad(Tag,DiskIndex,Offset,Length);
            }
            memset(&HostCmdTable[Tag],0,sizeof(HOST_CMD));
#ifndef DOS_ENV
            AHCI_HostCClearCmdFinishFlag(Tag);
#endif

            BuildDiskABCmd(&HostCmdTable[Tag],DiskIndex,Tag, Type,Offset,Length);
            Cmd_Cnt++;

        #ifndef DOS_ENV
            AHCI_HostCSetPxCI( Tag );
        #endif
            {

                for(SimTag=0;SimTag<MAX_CMD_SLOT;SimTag++)
                {
           #ifdef DOS_ENV

           #else
                    while(AHCI_HostCGetPxCI(SimTag))
                    {
                        MV_Schedule();
                        AHCI_HostCModelSchedule();
                        NFC_ModelSchedule();
                        SGE_ModelSchedule();
                    }
            #endif
               }
        }

    #ifdef DOS_ENV
    TagVal = (1<<Tag);

    *g_aAhciExt.PxCIRegAddress = TagVal;//(CiVal|TagVal);

    MixvSetCmdStatus(Tag,CMD_BUSY);

    LogMsg(COMP_HW,DBG_WARNING,"TRIG SLOT:%d TagVal:0x%x Ci:0x%x\n",Tag,TagVal,*g_aAhciExt.PxCIRegAddress);

    #endif
//#endif

        Offset+=Length;
        Disk_Length-=Length;
     }
  }
#ifndef DOS_ENV
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
    sim_mix_clear_host_transbyte(Tag);
    }
#endif
    }

VOID MixvDiskCRWCheck(HOST_REQ_TYPE Type,UCHAR DiskIndex,ULONG DiskOffset,ULONG DiskLen,ULONG MaxCmdLen)
{
    ULONG Disk_Length,Cmd_Cnt,Flag,Check_Len,Check_Finish_Len;
    ULONG Tag,SimTag,loop;
    ULONG Length,Offset,i;
    ULONG MskCmdComplete,tmp;
    ULONG RandCmdLen;

    HOST_CMD  HostCmdTable[MAX_CMD_SLOT];

#ifdef DOS_ENV
    ULONG TagVal;
#endif
#ifdef DOS_ENV
    sleep(1);
#endif
    //WRITE DISKA
    Cmd_Cnt = 0;
    Offset = DiskOffset;
    Disk_Length = DiskLen;//DISK_A_SIZE;
    MixvInitCmdStatus();
    TestInit();
    Flag = 0;
    Check_Len = 0;
    Check_Finish_Len = 0;
    loop=0;
    RandCmdLen = 1;
    while(1)
    {
        MskCmdComplete = 0;
        MixvCheckCmd(&MskCmdComplete);
        if(MskCmdComplete!=0)
       {
            tmp=MixvCheck_DiskC_CmdFinish(&MskCmdComplete,DiskIndex,HostCmdTable,Type);
            Check_Finish_Len+=tmp;
            //check data
            if(Type == HOST_REQ_ERASE)
            {
                DBG_Printf("erase complete:0x%x DiskLen:0x%x\n",Check_Len,DiskLen);
            }
            if(Type == HOST_REQ_READ)
            {
                Check_Len+=MixvCheckPayLoad(&MskCmdComplete,DiskIndex,HostCmdTable);
             }
            if(Type == HOST_REQ_WRITE)
            {
             }

            if(Check_Finish_Len==DiskLen)
            {
                return;
            }
        }
         if(Disk_Length==0)
        {
             continue;
        }
        if(Type!=HOST_REQ_WRITE)
        {
            Tag = MixvGetEmptySlot();
        }
        else
        {
	    Tag = INVALID_8F;
            if(MixvGetCmdStatus(0)!=CMD_BUSY)
            {
                if((AHCI_HostCGetPxCI(0)==0))
                {
                    Tag =0;
                }
                else
                {
                    Tag = INVALID_8F;
                }
            }
        }
        if(Tag!=INVALID_8F)
        {

            //BUILD CMD FOR ERASE length=flash cmd cnt

            if(Type==HOST_REQ_READ)
            {
                RandCmdLen++;
                if(RandCmdLen>MaxCmdLen)
                {
                    RandCmdLen = 1;
                }

                Length = (Disk_Length>RandCmdLen?RandCmdLen:Disk_Length);
            }
            else
            {
                Length = (Disk_Length>MaxCmdLen?MaxCmdLen:Disk_Length);
            }
            if(Length==0)
                continue;

            memset(&HostCmdTable[Tag],0,sizeof(HOST_CMD));
            if(Type==HOST_REQ_ERASE)
            {
                for(i=0;i<TEST_CE_NUM;i++)
                {
                    HostCmdTable[Tag].FlashAddrGroup[i].PU = i;//g_pumapping[i];
                    HostCmdTable[Tag].FlashAddrGroup[i].Block = Offset;
                    HostCmdTable[Tag].FlashAddrGroup[i].Page = 0;
                }
            }
            if(Type==HOST_REQ_WRITE)
            {
                MixvLbaToFlash(&HostCmdTable[Tag].FlashAddrGroup[0],Offset,Length);
//                DBG_Printf("slot:%d wt pu:%d blk:%d pg:%d\n",Tag,HostCmdTable[Tag].FlashAddrGroup[0].PU,HostCmdTable[Tag].FlashAddrGroup[0].Block,HostCmdTable[Tag].FlashAddrGroup[0].Page);

                MixvFillPayLoad(Tag,DiskIndex,Offset,Length);
            }
            else if(Type==HOST_REQ_READ)
            {
//                DBG_Printf("rd\n");
                MixvLbaToFlash(&HostCmdTable[Tag].FlashAddrGroup[0],Offset,Length);
                //for rd data check
                HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit = Offset;

                MixvZeroPayLoad(Tag,DiskIndex,Offset,Length);
            }

            HostCmdTable[Tag].FinishCnt[DiskIndex] = Length;

#ifndef DOS_ENV
            AHCI_HostCClearCmdFinishFlag(Tag);
#endif

            BuildDiskCCmd(&HostCmdTable[Tag],Tag,Type,Offset,Length);
        #ifndef DOS_ENV
            sim_mix_clear_host_transbyte(Tag);
        #endif
            Cmd_Cnt++;
        if((Cmd_Cnt%100)==0)
            DBG_Printf("Cmd Cnt:%d\n",Cmd_Cnt);
#ifndef DOS_ENV
            AHCI_HostCSetPxCI( Tag );
#endif

            {
                for(SimTag=0;SimTag<MAX_CMD_SLOT;SimTag++)
                {
#ifdef DOS_ENV
#else
                    while(AHCI_HostCGetPxCI(SimTag))
                    {
                        MV_Schedule();
                        AHCI_HostCModelSchedule();
                        NFC_ModelSchedule();
                        SGE_ModelSchedule();
                    }
#endif
             }
        }

#ifdef DOS_ENV
        TagVal = (1<<Tag);
        *g_aAhciExt.PxCIRegAddress = TagVal;//(CiVal|TagVal);
        if(Type==HOST_REQ_WRITE)
        {

        delay(DISK_C_WT_DELAY);

        }
        MixvSetCmdStatus(Tag,CMD_BUSY);
#endif
        Offset+=Length;
        Disk_Length-=Length;
     }
  }
#ifndef DOS_ENV
    for(Tag=0;Tag<MAX_CMD_SLOT;Tag++)
    {
        sim_mix_clear_host_transbyte(Tag);
    }
#endif
}

typedef struct _DOS_HOST_CMD_MANAGER
{
    HOST_CMD_STAT State;
    ULONG TrigSlot;
    ULONG TimStamp;
    HOST_CMD  HostCmdTable;
}DOS_HOST_CMD_MANAGER;
DOS_HOST_CMD_MANAGER g_HostCmdManager[MAX_CMD_SLOT];
SUB_CMD_TYPE gCurrDiskType;
typedef struct _HOST_CMD_DPTR
{
    ULONG CurrDiskWtOffset;
    ULONG CurrDiskWtLength;
    ULONG CurrDiskWtFinishLength;
    ULONG CurrDiskRdOffset;
    ULONG CurrDiskRdLength;
    ULONG CurrDiskRdFinishLength;

    ULONG CurrDiskSize;
    ULONG CurrDiskBaseCmdSize;

}HOST_CMD_DPTR;
HOST_CMD_DPTR g_cmd_dptr[SUB_DISK_CNT];
VOID MixvSplitHsg(USHORT * pHsg,UCHAR DiskIndex)
{
    U32 hsglen,hsgindex;
    USHORT *pStartHsg;
    pStartHsg = pHsg;
    if(DiskIndex==SUB_DISK_B)
    {
        hsglen = PG_SZ/HSG_CNT/MIN_B_HSG_BYTE_LEN;
    }
    if(DiskIndex==SUB_DISK_C)
    {
        hsglen = PG_SZ/HSG_CNT/MIN_C_HSG_BYTE_LEN;
    }
    for(hsgindex = 0;hsgindex<HSG_CNT;hsgindex++)
    {
        *pStartHsg++ = hsglen;
    }
}
U32 g_hsglen;
VOID MixvSplitHsgRandom(USHORT * pHsg,UCHAR DiskIndex)
{
    U32 hsglen,hsgindex,totalhsglen,grandurity;
    USHORT *pStartHsg;
    pStartHsg = pHsg;
    grandurity = (DiskIndex==SUB_DISK_B)?MIN_B_HSG_BYTE_LEN:MIN_C_HSG_BYTE_LEN;

    //hsgmaxlen = PG_SZ/hsgminlen;
    //rand(
    totalhsglen = 0;
    for(hsgindex = 0;hsgindex<HSG_CNT;hsgindex++)
    {

//        *pStartHsg = g_hsglen+hsgindex;
//        totalhsglen+=(*pStartHsg++);

        if(hsgindex==(HSG_CNT-1))
        {
            *pStartHsg = (PG_SZ/grandurity-totalhsglen);
        }
        else
        {
            *pStartHsg = g_hsglen+hsgindex;
        }

        totalhsglen+=(*pStartHsg++);
        if(totalhsglen>(PG_SZ/grandurity))
        {
            *pStartHsg--;
            *pStartHsg = (*pStartHsg-(totalhsglen-(PG_SZ/grandurity)));
            break;
        }
    }
#if 0
    g_hsglen++;
//    if(g_hsglen>(PG_SZ/grandurity))
    if(g_hsglen>(32))
        g_hsglen = 1;
#endif

//    g_hsglen++;
  //  if(g_hsglen==32)
    //    g_hsglen = 1;

}

#if 0
VOID MixvSplitHsgRandom(USHORT * pHsg,UCHAR DiskIndex)
{
    U32 hsglen,hsgindex,totalhsglen,grandurity;
    USHORT *pStartHsg;
    pStartHsg = pHsg;
    grandurity = (DiskIndex==SUB_DISK_B)?MIN_B_HSG_BYTE_LEN:MIN_C_HSG_BYTE_LEN;

    //hsgmaxlen = PG_SZ/hsgminlen;
    //rand(
    totalhsglen = 0;
    for(hsgindex = 0;hsgindex<HSG_CNT;hsgindex++)
    {

        if(hsgindex==(HSG_CNT-1))
        {
            *pStartHsg = (PG_SZ/grandurity-totalhsglen);
        }
        else
        {
            *pStartHsg = g_hsglen+hsgindex;
        }
        totalhsglen+=(*pStartHsg++);
    }
//#if 0
    g_hsglen++;
//    if(g_hsglen>(PG_SZ/grandurity))
    if(g_hsglen>(128))
        g_hsglen = 1;
//#endif

//    g_hsglen++;
  //  if(g_hsglen==32)
    //    g_hsglen = 1;

}
#endif
VOID MixvBuildDiskCmdInit(PHOST_CMD HostCmd,UCHAR DiskIndex, HOST_REQ_TYPE Type, ULONG Offset, ULONG Length )
{

    HostCmd->SubDiskCmd[ DiskIndex ].DiskEn       = 1;
    HostCmd->SubDiskCmd[ DiskIndex ].ReqType      = Type;
    if(DiskIndex==SUB_DISK_A)
    {
        HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = Offset;
          HostCmd->SubDiskCmd[ DiskIndex ].UnitLength   = Length;
    }
    if(DiskIndex==SUB_DISK_B)
    {
        HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = (Offset>>9);
          HostCmd->SubDiskCmd[ DiskIndex ].UnitLength   = (Length>>9);
#ifndef DISK_B_WT_SPLIT
        if(HostCmd->SubDiskCmd[ DiskIndex].ReqType == HOST_REQ_READ)
#endif	
        {
            HostCmd->SubDiskCmd[ DiskIndex ].SplitEnable = 1;
        //MixvSplitHsg(HostCmd,DiskIndex,&HostCmd->HsgLength[0],Offset,Length);
            #ifndef DYNAMIC_HSG_SPLIT
            MixvSplitHsg(&HostCmd->HsgLength[0],DiskIndex);
            #else
            MixvSplitHsgRandom(&HostCmd->HsgLength[0],DiskIndex);
            #endif

        }
    }
    if(DiskIndex==SUB_DISK_C)
    {
        //HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = (Offset>>9)%SEC_PER_PG;
        HostCmd->SubDiskCmd[ DiskIndex ].StartUnit = (Offset>>9);
        //OffsetDiskC[]
          HostCmd->SubDiskCmd[ DiskIndex ].UnitLength   = (Length>>9);
        HostCmd->SubDiskCmd[ DiskIndex ].SplitEnable = 1;
        if(HostCmd->SubDiskCmd[ DiskIndex].ReqType == HOST_REQ_READ)
        {
            //MixvLbaToFlash(&HostCmdTable[Tag].FlashAddrGroup[0],Offset,Length);
			MixvLbaToFlash(&HostCmd->FlashAddrGroup[0],Offset/SECTOR_SZ,Length/SECTOR_SZ);
            //MixvSplitHsg(HostCmd,DiskIndex,&HostCmd->HsgLength[0],Offset,Length);
			#ifndef DYNAMIC_HSG_SPLIT
            MixvSplitHsg(&HostCmd->HsgLength[0],DiskIndex);
			#else
            MixvSplitHsgRandom(&HostCmd->HsgLength[0],DiskIndex);
            #endif
        }
    }
//    DBG_Printf("\n");
    return;
}
#if 1
DISK_CMD_STATUS MixvDiskCSchdule(HOST_CMD_DPTR * pReqDptr,ULONG *offset,ULONG *length,HOST_REQ_TYPE * p_req)
{
    ULONG *rdOffset, *rdLen,*rdFinishLen;
    ULONG diskByteLen;
    ULONG cmdBaseByteLen;

    rdOffset = &pReqDptr->CurrDiskRdOffset;
    rdLen = &pReqDptr->CurrDiskRdLength;
    rdFinishLen = &pReqDptr->CurrDiskRdFinishLength;

    diskByteLen = pReqDptr->CurrDiskSize;
    cmdBaseByteLen = pReqDptr->CurrDiskBaseCmdSize;

    if(*rdFinishLen==diskByteLen)
    {
	DBG_Printf("rd disk:2 finish!\n");
        return RD_DISK_FINISH;
    }
    if(*rdOffset==diskByteLen)
    {
        return RD_DISK_CMD_SEND_FINISH;
    }


    *offset = *rdOffset;
    *length = *rdLen;
    *rdOffset+=*rdLen;

    if(*rdLen>(diskByteLen-(*rdOffset)))
    {
        *rdLen = (diskByteLen-*rdOffset);
    }
#if 0
    if(*rdLen>(diskByteLen-(*rdOffset)))
    {
        *rdLen = (diskByteLen-*rdOffset);
    }
    else
    {
        *rdLen+=cmdBaseByteLen;
    }
#endif


    *p_req = HOST_REQ_READ;
    return DISK_CMD_SUCCESS;

}

DISK_CMD_STATUS MixvDiskABSchdule(HOST_CMD_DPTR * pReqDptr,ULONG *offset,ULONG *length,HOST_REQ_TYPE * p_req, ULONG diskindex)
{
    ULONG *wtOffset,* wtLen,*wtFinishLen;
    ULONG *rdOffset, *rdLen,*rdFinishLen;

    ULONG diskByteLen;
    ULONG cmdBaseByteLen;
    wtOffset = &pReqDptr->CurrDiskWtOffset;
    wtLen = &pReqDptr->CurrDiskWtLength;
    wtFinishLen = &pReqDptr->CurrDiskWtFinishLength;
    rdOffset = &pReqDptr->CurrDiskRdOffset;
    rdLen = &pReqDptr->CurrDiskRdLength;
    rdFinishLen = &pReqDptr->CurrDiskRdFinishLength;

    diskByteLen = pReqDptr->CurrDiskSize;
    cmdBaseByteLen = pReqDptr->CurrDiskBaseCmdSize;
    if(*wtFinishLen==diskByteLen)
    {
#ifndef DISK_B_WT_ONLY
	if(*rdFinishLen==diskByteLen)
#endif
	{
	    
	    DBG_Printf("rd disk:%d finish!\n",diskindex);
            return RD_DISK_FINISH;
	}

        if(*rdOffset==diskByteLen)
        {
//	    DBG_Printf("rd cmd send finish disklen:0x%x\n",diskByteLen);
            return RD_DISK_CMD_SEND_FINISH;
        }
        *offset = *rdOffset;
        *length = *rdLen;
        *rdOffset+=*rdLen;
        if(*rdLen>(diskByteLen-(*rdOffset)))
        {
            *rdLen = (diskByteLen-*rdOffset);
        }
        else
        {
            
            {   
                *rdLen+=cmdBaseByteLen;
                
            }
            

        }
        *p_req = HOST_REQ_READ;
    }
    else
    {
        //disk a build wt req

        if(*wtOffset==diskByteLen)
        {
//	    DBG_Printf("wt disk:%d finish!\n",diskindex);
            return WT_DISK_CMD_SEND_FINISH;
        }
        else
        {
            *offset = *wtOffset;
            *length = *wtLen;
            *wtOffset+=*wtLen;
#if 1
            if(*wtLen>(diskByteLen-*wtOffset))
            {
                *wtLen = (diskByteLen-*wtOffset);
            }
            else
            {

        	   
                {   
                    *wtLen+=cmdBaseByteLen;
                    
                }
            }
#endif

        }

        *p_req = HOST_REQ_WRITE;

    }

    return DISK_CMD_SUCCESS;
}
#endif

#if 1
BOOL MixvGetCmdParam(ULONG * pOffset,ULONG * pLength,UCHAR DiskIndex,HOST_REQ_TYPE *Req)
{
    BOOL res;
    DISK_CMD_STATUS cmdstatus;
    res = TRUE;
    if(DiskIndex!=SUB_DISK_C)
    {
//        g_cmd_dptr[DiskIndex].CurrDiskBaseCmdSize =
        cmdstatus = MixvDiskABSchdule(&g_cmd_dptr[DiskIndex],pOffset,pLength,Req,DiskIndex);
        if(cmdstatus!=DISK_CMD_SUCCESS)
        {
            if(cmdstatus==RD_DISK_FINISH)
            {
        		DBG_Printf("wt disk:%d begin\n",DiskIndex);
                g_cmd_dptr[DiskIndex].CurrDiskRdOffset=0;
                g_cmd_dptr[DiskIndex].CurrDiskRdLength = g_cmd_dptr[DiskIndex].CurrDiskBaseCmdSize;
                g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength = 0;
                g_cmd_dptr[DiskIndex].CurrDiskWtOffset = 0;
                g_cmd_dptr[DiskIndex].CurrDiskWtLength = g_cmd_dptr[DiskIndex].CurrDiskBaseCmdSize;
                g_cmd_dptr[DiskIndex].CurrDiskWtFinishLength = 0;
            }
            res = FALSE;
        }
    }
    else
    {
        cmdstatus = MixvDiskCSchdule(&g_cmd_dptr[DiskIndex],pOffset,pLength,Req);

        if(cmdstatus!=DISK_CMD_SUCCESS)
        {
            if(cmdstatus==RD_DISK_FINISH)
            {
                g_cmd_dptr[DiskIndex].CurrDiskRdOffset=0;
                g_cmd_dptr[DiskIndex].CurrDiskRdLength = g_cmd_dptr[DiskIndex].CurrDiskBaseCmdSize;
                g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength = 0;

            }
	    res = FALSE;
        }
//        res = FALSE;
    }

    return res;
}

BOOL MixvInitSubCmd(DOS_HOST_CMD_MANAGER * p_Host_Cmd)
{
    ULONG Offset,Length;
    HOST_REQ_TYPE Req;
    BOOL res;
    res = FALSE;

    memset(&p_Host_Cmd->HostCmdTable,0,sizeof(HOST_CMD));

    res = MixvGetCmdParam(&Offset,&Length,gCurrDiskType,&Req);
    if(res==TRUE)
    {
//	DBG_Printf("Offset:0x%x Length:0x%x\n",Offset,Length);
        MixvBuildDiskCmdInit(&p_Host_Cmd->HostCmdTable,gCurrDiskType,Req,Offset,Length);
    }
//    gCurrDiskType = DISK_B;
    //gCurrDiskType = DISK_A;

#ifdef DISK_A_B_C_MULTI
    gCurrDiskType++;
    if(gCurrDiskType>DISK_C)
    {
        gCurrDiskType = DISK_A;
    }
#endif

    return res;
}

VOID MixvTrigCmd(UCHAR Tag)
{
    ULONG SimTag,TagVal,Type;
    
    
#ifndef DOS_ENV
    sim_mix_clear_host_transbyte(Tag);
    AHCI_HostCSetPxCI( Tag );
#endif
    //for(SimTag=0;SimTag<MAX_CMD_SLOT;SimTag++)
    SimTag = Tag;
    {
#ifndef DOS_ENV

         while(AHCI_HostCGetPxCI(SimTag))
         {
             MV_Schedule();
             AHCI_HostCModelSchedule();
             NFC_ModelSchedule();
             SGE_ModelSchedule();
         }
#endif
    }

#ifdef DOS_ENV
        TagVal = (1<<Tag);
        *g_aAhciExt.PxCIRegAddress = TagVal;//(CiVal|TagVal);
        if(Type==HOST_REQ_WRITE)
        {

        delay(DISK_C_WT_DELAY);

        }
        //MixvSetCmdStatus(Tag,CMD_BUSY);
#endif
}
BOOL MixvWaitCmdDone(UCHAR Tag)
{
    if(AHCI_HostCGetPxCI(Tag)==0)
    {
        return TRUE;
    }
    else
    {
        ulSlotTime[Tag]++;
        if(ulSlotTime[Tag]>1000000)
        {
            DBG_Printf("slot %d timeout!!\n",Tag);
            DBG_Getch();
        }

    }
}
BOOL MixvCheckData(DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
        ULONG Tag,nop,sec,DiskIndex;
        ULONG Addr,StartAddr;
        ULONG i,Offset,Length,len;
        ULONG DataSrc,DataActual;

        Tag = Cmd_Manager->TrigSlot;
        for(DiskIndex=0;DiskIndex<SUB_DISK_CNT;DiskIndex++)
        {

            if((Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].DiskEn==FALSE)||(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_WRITE))
                continue;


    //        for(nop=0;nop<10000;nop++);
            if(DiskIndex==SUB_DISK_A)
            {
                Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength;
//		DBG_Printf("check data bytelen:0x%x\n",g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength);
                Length=(Length>>2);
                Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);
            }
            else if(DiskIndex==SUB_DISK_B)
            {
                Offset = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].StartUnit;
                Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength*SECTOR_SZ;
                Length=(Length>>2);
                Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
            }
            else if(DiskIndex==SUB_DISK_C)
            {
                Offset = Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].StartUnit;
                Length = g_aAhciExt.HostCmdTable[Tag].SubDiskCmd[DiskIndex].UnitLength*SECTOR_SZ;
                Length=(Length>>2);
                //DBG_Printf("Tag:%d Read C Offset:0x%x\n",Tag,Offset);
                Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
            }
	    StartAddr = Addr;
//	DBG_Printf("Data Check Slot:%d Disk:%d HostAddr:0x%x ByteLen:0x%x\n",Tag,DiskIndex,Addr,Length);

        if(DiskIndex!=SUB_DISK_C)
        {
            for(i=0;i<(Length);i++)
            {
                DataSrc = *(volatile ULONG *)Addr;
                if(DiskIndex==SUB_DISK_A)
                {
                    DataActual = ((Offset)<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));//(t<<24)|(Offset);
                }
                if(DiskIndex==SUB_DISK_B)
                {
                    DataActual = ((Offset+i/128)<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));//(t<<24)|(Offset);
                }

                if(DataSrc!=DataActual)
                {

                    DBG_Printf("Data Check Slot:%d Disk:%d HostAddr:0x%x DWLen:0x%x\n",Tag,DiskIndex,StartAddr,Length);
                    DBG_Printf("check data err! 1st rd:0x%x actual:0x%x\n",DataSrc,DataActual);
                    for(nop=0;nop<100000;nop++);

                    DataSrc = *(volatile ULONG *)Addr;
                    if(DataSrc!=DataActual)
                    {
                        DBG_Printf("check data err! disk:%d hostaddr:0x%x cmd_slot:%d dword offset:0x%x err data:0x%x actual data:0x%x\n",DiskIndex,Addr,Tag,i,DataSrc,DataActual);

                        LogMsg( COMP_HW, DBG_WARNING,"check data err! cmd_slot:%d err data:0x%x actual data:0x%x\n",Tag,DataSrc,DataActual);

                        DBG_Getch();
                    }
                }
                else
                {
            //            DBG_Printf("check data ok! cmd_slot:%d err data:0x%x actual data:0x%x\n",Tag,DataSrc,DataActual);
                }
                Addr+=4;
            }
        }
        else
        {
//        DBG_Printf("c check addr:0x%x data:0x%x\n",Addr,(Offset));
            for(len=0;len<Length;len++)
            {
                DataSrc = *(volatile ULONG *)Addr;
                sec = len/128;
                DataActual = (Offset+sec);
                if(DataSrc!=DataActual)
                {

                    DBG_Printf("Data Check Slot:%d Disk:%d HostAddr:0x%x ByteLen:0x%x\n",Tag,DiskIndex,StartAddr,Length);
                    DBG_Printf("check data err! disk:%d hostaddr:0x%x cmd_slot:%d  err data:0x%x actual data:0x%x\n",DiskIndex,Addr,Tag,DataSrc,DataActual);
                    DBG_Getch();
                }
                Addr+=4;
            }
        }
//        #ifdef DBG_INFO
//              DBG_Printf("check data ok! cmd_slot:%d\n",Tag);
//        #endif

    }
//    DBG_Printf("check data ok!!\n");
    return TRUE;
}

VOID MixvFillDiscPayLoad(ULONG Tag,DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
    ULONG Addr;
    ULONG i,Len,Offset,Length;
    UCHAR DiskIndex;
//    DiskIndex =
    for(DiskIndex=0;DiskIndex<SUB_DISK_CNT;DiskIndex++)
    {
        if((Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].DiskEn == TRUE)&&(Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex].ReqType == HOST_REQ_WRITE))
        {
            Offset = Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].StartUnit;
            Length = Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].UnitLength;


            if(DiskIndex==SUB_DISK_A)
            {
                Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);
                Len = Length;
            }
            else if(DiskIndex==SUB_DISK_B)
            {
                Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
                Len = Length*SECTOR_SZ;
            }
            else if(DiskIndex==SUB_DISK_C)
            {
                Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
                Len = Length*SECTOR_SZ;
            }
            if((DiskIndex!=SUB_DISK_A))
            {
                for(i=0;i<(Len>>2);i++)
                {
                    *(volatile ULONG *)Addr = ((Offset+i/128)<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));
                    Addr+=4;
                }
            }
            else
            {
                for(i=0;i<(Len>>2);i++)
                {
                    *(volatile ULONG *)Addr = ((Offset));//(((Offset&0xffff)<<16)|(Offset&0xffff));
                    Addr+=4;
                }

            }

        }
    }
}
VOID MixvInitDiscBuffer(ULONG Tag,DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
    ULONG Addr;
    ULONG i,Len,Offset,Length;
    UCHAR DiskIndex;
//    DiskIndex =
    for(DiskIndex=0;DiskIndex<SUB_DISK_CNT;DiskIndex++)
    {
        if((Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].DiskEn == TRUE)&&(Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex].ReqType == HOST_REQ_READ))
        {
            Offset = Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].StartUnit;
            Length = Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].UnitLength;


            if(DiskIndex==SUB_DISK_A)
            {
                Addr = (ULONG)(g_aAhciExt.DiskAAddrSw+Offset);
                Len = Length;
            }
            else if(DiskIndex==SUB_DISK_B)
            {
                Addr = (ULONG)(g_aAhciExt.DiskBAddrSw+Offset*SECTOR_SZ);
                Len = Length*SECTOR_SZ;
            }
            else if(DiskIndex==SUB_DISK_C)
            {
                Addr = (ULONG)(g_aAhciExt.DiskCAddrSw+Tag*TEST_CE_NUM*FLASH_ADDR_CNT*PG_SZ);//+Offset*SECTOR_SZ;
                Len = Length*SECTOR_SZ;
            }
            if((DiskIndex!=SUB_DISK_A))
            {
                for(i=0;i<(Len>>2);i++)
                {
                    *(volatile ULONG *)Addr = PAYLOAD_INIT_VAL;//((Offset+i/128)<<DATA_RANDON_OFFSET_BIT);//(((Offset&0xffff)<<16)|(Offset&0xffff));
                    Addr+=4;
                }
            }
            else
            {
                for(i=0;i<(Len>>2);i++)
                {
                    *(volatile ULONG *)Addr = PAYLOAD_INIT_VAL;//((Offset+i/4));//(((Offset&0xffff)<<16)|(Offset&0xffff));
                    Addr+=4;
                }

            }

        }
    }
}

VOID MixvBuildSubDiskCmd(UCHAR Tag,DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
    UCHAR DiskIndex;
    ULONG Offset;
    for(DiskIndex=0;DiskIndex<SUB_DISK_CNT;DiskIndex++)
    {
        if(Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].DiskEn == TRUE)
        {
            if(DiskIndex==SUB_DISK_C)
            {
                Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].HostAddrLow = HWBATable[ DiskIndex ]+Tag*FLASH_ADDR_CNT*PG_SZ*TEST_CE_NUM;
            }
            else
            {
                Offset = Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].StartUnit;
                Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].HostAddrLow=HWBATable[ DiskIndex ] +( ( DiskIndex == SUB_DISK_A ) ? (Offset) : ( Offset << 9 ) );
            }
            Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex ].HostAddrHigh  = 0;

            if(Cmd_Manager->HostCmdTable.SubDiskCmd[ DiskIndex].ReqType == HOST_REQ_WRITE)
            {
                //fillpayload
                MixvFillDiscPayLoad(Tag,Cmd_Manager);
            }
            else
            {
                MixvInitDiscBuffer(Tag,Cmd_Manager);
            }
        }
    }
    memcpy(&g_aAhciExt.HostCmdTable[Tag],&Cmd_Manager->HostCmdTable,sizeof(HOST_CMD));
}
VOID MixvClearTagTimeStamp(DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
    ulSlotTime[Cmd_Manager->TrigSlot] = 0;
}

VOID MixvUpdateCmdDptr(DOS_HOST_CMD_MANAGER * Cmd_Manager)
{
    ULONG DiskIndex;
    for(DiskIndex=0;DiskIndex<SUB_DISK_CNT;DiskIndex++)
    {
        if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].DiskEn==FALSE)
            continue;

        if(DiskIndex==SUB_DISK_B)
        {
            if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_READ)
            {
                g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength += (Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].UnitLength<<9);
            }

            if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_WRITE)
            {
                g_cmd_dptr[DiskIndex].CurrDiskWtFinishLength += (Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].UnitLength<<9);
                //if(0x08000000==g_cmd_dptr[DiskIndex].CurrDiskWtFinishLength)
                  //  DBG_Printf("\n");
            }
        }

        if(DiskIndex==SUB_DISK_A)
        {
            if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_READ)
            {
                g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength += (Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].UnitLength);
            }

            if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_WRITE)
            {
                g_cmd_dptr[DiskIndex].CurrDiskWtFinishLength += (Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].UnitLength);
            }
        }

        if(DiskIndex==SUB_DISK_C)
        {
            if(Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].ReqType==HOST_REQ_READ)
            {
                g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength += (Cmd_Manager->HostCmdTable.SubDiskCmd[DiskIndex].UnitLength<<9);
            }
        }
#if 0	
	if(g_cmd_dptr[DiskIndex].CurrDiskRdFinishLength==g_cmd_dptr[DiskIndex].CurrDiskSize)
	    DBG_Printf("Disk %d Rd Finish!\n",DiskIndex);

	if(g_cmd_dptr[DiskIndex].CurrDiskWtFinishLength==g_cmd_dptr[DiskIndex].CurrDiskSize)
	    DBG_Printf("Disk %d Wt Finish!\n",DiskIndex);
#endif
    }
}
VOID MixvDiskAllInOnePatten()
{
    UCHAR CmdSlot,i,trigslot;
    ULONG Tag,tmpstartunit,tmpunitlen,tmpreq;
    BOOL res;
    for(CmdSlot=0;CmdSlot<MAX_CMD_SLOT;CmdSlot++)
    {
        switch(g_HostCmdManager[CmdSlot].State)
        {
            case INIT_CMD:
                res = MixvInitSubCmd(&g_HostCmdManager[CmdSlot]);
                if(res == TRUE)
                {
                    g_HostCmdManager[CmdSlot].State = GET_CMD_SLOT;
                }
                break;
            case GET_CMD_SLOT:
                Tag = MixvGetEmptySlot();
                if(Tag!=INVALID_8F)
                {
                    g_HostCmdManager[CmdSlot].State = TRIG_CMD;
                    g_HostCmdManager[CmdSlot].TrigSlot = Tag;
                    MixvBuildSubDiskCmd(Tag,&g_HostCmdManager[CmdSlot]);
                    MixvSetCmdStatus(Tag,CMD_BUSY);
                }
                break;
            case TRIG_CMD:
	    	trigslot =g_HostCmdManager[CmdSlot].TrigSlot;
		//if(gCurrDiskType==DISK_A)
		{
	    	for(i=0;i<3;i++)
		{
		tmpstartunit =g_HostCmdManager[trigslot].HostCmdTable.SubDiskCmd[i].StartUnit;

		tmpreq =g_HostCmdManager[trigslot].HostCmdTable.SubDiskCmd[i].ReqType;
		tmpunitlen =g_HostCmdManager[trigslot].HostCmdTable.SubDiskCmd[i].UnitLength;
		
		if(g_HostCmdManager[trigslot].HostCmdTable.SubDiskCmd[i].DiskEn)
		{
		    //if(i==DISK_B)
//                    DBG_Printf("trig slot:%d disk:%d reqtype:%d startunit:0x%x unitlen:0x%x\n",trigslot,i,tmpreq,tmpstartunit,tmpunitlen);
		}
		}
		}
                MixvTrigCmd(g_HostCmdManager[CmdSlot].TrigSlot);
                g_HostCmdManager[CmdSlot].State = WAIT_CMD;
                break;
            case WAIT_CMD:

                res = MixvWaitCmdDone(g_HostCmdManager[CmdSlot].TrigSlot);
                if(res==TRUE)
                {
                    g_HostCmdManager[CmdSlot].State = CHECK_CMD;
                }
                break;
            case CHECK_CMD:
                res = MixvCheckData(&g_HostCmdManager[CmdSlot]);
                if(res == TRUE)
                {
                    MixvUpdateCmdDptr(&g_HostCmdManager[CmdSlot]);

                    MixvClearTagTimeStamp(&g_HostCmdManager[CmdSlot]);


                }
                MixvSetCmdStatus(g_HostCmdManager[CmdSlot].TrigSlot,CMD_IDLE);
                memset(&g_HostCmdManager[CmdSlot],0,sizeof(DOS_HOST_CMD_MANAGER));
                g_HostCmdManager[CmdSlot].State = INIT_CMD;
                break;
            default:
                DBG_Getch();
        }
    }

}
#endif
VOID MixvAllInOnePattenInit()
{
    
    g_hsglen = 1;
    memset(g_HostCmdManager,0,MAX_CMD_SLOT*sizeof(DOS_HOST_CMD_MANAGER));
    memset(g_cmd_dptr,0,SUB_DISK_CNT*sizeof(HOST_CMD_DPTR));
    memset(ulSlotTime,0,sizeof(ulSlotTime));
    g_cmd_dptr[SUB_DISK_A].CurrDiskBaseCmdSize = 256;
    g_cmd_dptr[SUB_DISK_A].CurrDiskSize = 1024*1024;
    g_cmd_dptr[SUB_DISK_A].CurrDiskWtLength = g_cmd_dptr[SUB_DISK_A].CurrDiskBaseCmdSize;
    g_cmd_dptr[SUB_DISK_A].CurrDiskRdLength = g_cmd_dptr[SUB_DISK_A].CurrDiskBaseCmdSize;


    g_cmd_dptr[SUB_DISK_B].CurrDiskBaseCmdSize = 2048;
    g_cmd_dptr[SUB_DISK_B].CurrDiskSize = 128*1024*1024;
    g_cmd_dptr[SUB_DISK_B].CurrDiskWtLength = g_cmd_dptr[SUB_DISK_B].CurrDiskBaseCmdSize;
    g_cmd_dptr[SUB_DISK_B].CurrDiskRdLength = g_cmd_dptr[SUB_DISK_B].CurrDiskBaseCmdSize;


    g_cmd_dptr[SUB_DISK_C].CurrDiskBaseCmdSize = 31*1024;
    g_cmd_dptr[SUB_DISK_C].CurrDiskSize = TEST_C_BLK_CNT*TEST_CE_NUM*512*32*1024;

//    g_cmd_dptr[SUB_DISK_C].CurrDiskSize = 32*1024;
    g_cmd_dptr[SUB_DISK_C].CurrDiskWtLength = g_cmd_dptr[SUB_DISK_C].CurrDiskBaseCmdSize;
    g_cmd_dptr[SUB_DISK_C].CurrDiskRdLength = g_cmd_dptr[SUB_DISK_C].CurrDiskBaseCmdSize;

}

void Mixv_Test_NB_Patten()
{
#ifdef DISK_C_INITIAL
    DBG_Printf("disk c ers\n");
    MixvDiskCRWCheck(HOST_REQ_ERASE,SUB_DISK_C,TEST_C_START_BLK,TEST_C_BLK_CNT,1);
    DBG_Printf("disk c wt\n");
    MixvDiskCRWCheck(HOST_REQ_WRITE,SUB_DISK_C,TEST_C_DISK_OFFSET,TEST_C_DISK_LEN*TEST_CE_NUM,TEST_C_DISK_MAX_SEC*TEST_CE_NUM);
#endif
    MixvAllInOnePattenInit();
    while(1)
    {
        MixvDiskAllInOnePatten();
    }
}
void MixvTest()
{

    Mixv_Test_NB_Patten();
    while(1)
    {
    #ifdef DISK_C_ENABLE
    #ifdef DISK_C_INITIAL
    DBG_Printf("disk c ers\n");
    MixvDiskCRWCheck(HOST_REQ_ERASE,SUB_DISK_C,TEST_C_START_BLK,TEST_C_BLK_CNT,1);
    DBG_Printf("disk c wt\n");
    MixvDiskCRWCheck(HOST_REQ_WRITE,SUB_DISK_C,TEST_C_DISK_OFFSET,TEST_C_DISK_LEN*TEST_CE_NUM,TEST_C_DISK_MAX_SEC*TEST_CE_NUM);
    #endif
//for(;;)

    DBG_Printf("disk c rd\n");
    MixvDiskCRWCheck(HOST_REQ_READ,SUB_DISK_C,TEST_C_DISK_OFFSET,TEST_C_DISK_LEN*TEST_CE_NUM,TEST_C_DISK_MAX_SEC*TEST_CE_NUM);

//    MixvDiskCRWCheck(HOST_REQ_READ,SUB_DISK_C,TEST_C_DISK_OFFSET,TEST_C_DISK_LEN,16);
    #endif
    #ifdef DISK_A_ENABLE
    DBG_Printf("disk a wt\n");
    MixvDiskABRWCheck(HOST_REQ_WRITE,SUB_DISK_A,TEST_A_OFFSET,TEST_A_SIZE,TEST_A_MAX_CMD_LEN);
    #endif

    #ifdef DISK_B_ENABLE
    DBG_Printf("disk b wt\n");
    MixvDiskABRWCheck(HOST_REQ_WRITE,SUB_DISK_B,TEST_B_OFFSET,TEST_B_SIZE/SECTOR_SZ,TEST_B_MAX_CMD_LEN);
    #endif

    #ifdef DISK_A_ENABLE
    DBG_Printf("disk a rd\n");
    MixvDiskABRWCheck(HOST_REQ_READ,SUB_DISK_A,TEST_A_OFFSET,TEST_A_SIZE,TEST_A_MAX_CMD_LEN);
    #endif

    #ifdef DISK_B_ENABLE
    DBG_Printf("disk b rd\n");
    MixvDiskABRWCheck(HOST_REQ_READ,SUB_DISK_B,TEST_B_OFFSET,TEST_B_SIZE/SECTOR_SZ,TEST_B_MAX_CMD_LEN);
    #endif
    DBG_Printf("Test ok!\n");
    }
 //   return SUCCESS;

}
void rdhostmem(ULONG addr,ULONG dwcnt)
{
    ULONG cnt;
    for(cnt=0;cnt<dwcnt;cnt++)
    {
	DBG_Printf("hostaddr:0x%x val:0x%x\n",addr+cnt*4,*(ULONG *)(addr+cnt*4));
	if(*(ULONG *)(addr+cnt*4)!=((addr-0x8000000)/512+cnt/128))
	{
		DBG_Printf("err actual:0x%x err data:0x%x\n",((addr-0x8000000)/512+cnt/128),*(ULONG *)(addr+cnt*4));
		return;
	}
    }
    return;
}
#ifdef DOS_ENV
void main(int argc, char**argv)
#else
void mixv_host_main()
#endif
{
	  ULONG tag;
	ULONG lenth;
	#if 0
	//lenth = 0x38 * 512;
	lenth = (0x38*512)/4;
    rdhostmem(0xb2e9800,lenth);
   // rdhostmem(0xf396800,32);
    return;
    #endif
    gCurrDiskType = DISK_START_TYPE;

    if ( SUCCESS != MixvInit() )
    {
        DBG_Printf("Initialization failed!\n");
        return;
    }else{
        DBG_Printf("Initialization successfully!\n");
    }
	 	for(tag=0;tag<MAX_CMD_SLOT;tag++)
	 			ulCmdManager[tag] = 0;

    MixvTest();
}

