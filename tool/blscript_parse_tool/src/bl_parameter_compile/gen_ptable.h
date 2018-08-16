#ifndef _GEN_PTABLE_H
#define _GEN_PTABLE_H
#include "main.h"

//typedef struct _ptable_offset
//{
//    UINT uiDwOffset;
//    //UINT uiDwNum;
//    UINT uiBitOffsetInDw;
//    //UINT uiBitNum;
//    UINT uiMask;
//}PTABLE_OFFSET;
//
//typedef struct _ptable_member_map
//{
//    TCHAR str[LEN_VALUE];
//    PTABLE_OFFSET tOffset;
//   
//}PTABLE_MEMBER_MAP;
//
//static PTABLE_MEMBER_MAP PTABLE_MEMBER_MAP_LIB[] =
//{
//    //tBLVersion, dword 0~5
//    { _T("ucMCUType"), { 0, 0, 0x000000ff } },
//    { _T("ucHostType"), { 0, 8, 0x0000ff00 } },
//    { _T("ucFlashType"), { 0, 16, 0x00ff0000 } },
//    { _T("ucOtherConfig"), { 0, 24, 0xff000000 } },
//    { _T("ulBLParamTypes"), { 1, 0, 0xffffffff } },
//    { _T("ulFWRleaseVerion"), { 2, 0, 0xffffffff } },
//    { _T("ulGITVersion"), { 3, 0, 0xffffffff } },
//    { _T("ulDateInfo"), { 4, 0, 0xffffffff } },
//    { _T("ulTimeInfo"), { 5, 0, 0xffffffff } },
//
//    //sBootStaticFlag, dword 6
//    { _T("bsBootMethodSel"), { 6, 0, 0x00000003 } },
//    { _T("bsWarmBoot"), { 6, 2, 0x00000004 } },
//    { _T("bsLocalTest"), { 6, 3, 0x00000008 } },
//    { _T("bsRollBackECT"), { 6, 4, 0x00000010 } },
//    { _T("bsRebuildGB"), { 6, 5, 0x00000020 } },
//    { _T("bsUseDefault"), { 6, 6, 0x00000040 } },
//    { _T("bsOptionRomEn"), { 6, 7, 0x00000080 } },
//    { _T("bsDebugMode"), { 6, 8, 0x00000100 } },
//    { _T("bsUartMpMode"), { 6, 9, 0x00000200 } },
//    { _T("bsLLFMethodSel"), { 6, 10, 0x00001c00 } },
//    { _T("bsExternalSpiFlashEn"), { 6, 13, 0x00002000 } },
//    { _T("bsRdtTestEnable"), { 6,14,  0x00004000 } },
//    { _T("bsUartPrintEn"), { 6, 15, 0x00008000 } },
//    { _T("bsInheritEraseCntEn"), { 6, 16, 0x00010000 } },
//    { _T("bsStaticFlagRsv"), { 6, 17, 0xfffe0000 } },
//
//    //sBootSelFlag, dword 7
//    { _T("bsEnableMCU0"), { 7, 0, 0x00000001 } },
//    { _T("bsEnableMCU1"), { 7, 1, 0x00000002 } },
//    { _T("bsEnableMCU2"), { 7, 2, 0x00000004 } },
//    { _T("bsEnableRsv"), { 7, 3, 0xfffffff8 } },
//
//    //sHwInitFlag, dword 8
//    { _T("bsGlobalInitDone"), { 8, 0, 0x00000001 } },
//    { _T("bsDDRInitDone"), { 8, 1, 0x00000002 } },
//    { _T("bsNFCInitDone"), { 8, 2, 0x00000004 } },
//    { _T("bsPCIeInitDone"), { 8, 3, 0x00000008 } },
//    { _T("bsClockGatingInitDone"), { 8, 4, 0x00000010 } },
//    { _T("bsFlash1STDone"), { 8, 5, 0x00000020 } },
//    { _T("bsFlash2NDDone"), { 8, 6, 0x00000040 } },
//    { _T("bsHWInitRsv"), { 8, 7, 0xffffff80 } },
//
//    //aFWVersion[2], dword 9 10
//    { _T("aFWVersion[0]"), { 9, 0, 0xffffffff } },
//    { _T("aFWVersion[1]"), { 10, 0, 0xffffffff } },
//
//    //aFWDiskName[10], dword 11~20
//    { _T("aFWDiskName[0]"), { 11, 0, 0xffffffff } },
//    { _T("aFWDiskName[1]"), { 12, 0, 0xffffffff } },
//    { _T("aFWDiskName[2]"), { 13, 0, 0xffffffff } },
//    { _T("aFWDiskName[3]"), { 14, 0, 0xffffffff } },
//    { _T("aFWDiskName[4]"), { 15, 0, 0xffffffff } },
//    { _T("aFWDiskName[5]"), { 16, 0, 0xffffffff } },
//    { _T("aFWDiskName[6]"), { 17, 0, 0xffffffff } },
//    { _T("aFWDiskName[7]"), { 18, 0, 0xffffffff } },
//    { _T("aFWDiskName[8]"), { 19, 0, 0xffffffff } },
//    { _T("aFWDiskName[9]"), { 20, 0, 0xffffffff } },
//
//    //aFWSerialNum[5], dword 21-25
//    { _T("aFWSerialNum[0]"), { 21, 0, 0xffffffff } },
//    { _T("aFWSerialNum[1]"), { 22, 0, 0xffffffff } },
//    { _T("aFWSerialNum[2]"), { 23, 0, 0xffffffff } },
//    { _T("aFWSerialNum[3]"), { 24, 0, 0xffffffff } },
//    { _T("aFWSerialNum[4]"), { 25, 0, 0xffffffff } },
//
//    //ulFWSavePageCnt, dword 26
//    { _T("ulFWSavePageCnt"), { 26, 0, 0xffffffff } },
//
//    //aFWCompileDate[4], dword 27~30
//    { _T("aFWCompileDate[0]"), { 27, 0, 0xffffffff } },
//    { _T("aFWCompileDate[1]"), { 28, 0, 0xffffffff } },
//    { _T("aFWCompileDate[2]"), { 29, 0, 0xffffffff } },
//    { _T("aFWCompileDate[3]"), { 30, 0, 0xffffffff } },
//
//    //aFWCompileTime[4], dword 31~34
//    { _T("aFWCompileTime[0]"), { 31, 0, 0xffffffff } },
//    { _T("aFWCompileTime[1]"), { 32, 0, 0xffffffff } },
//    { _T("aFWCompileTime[2]"), { 33, 0, 0xffffffff } },
//    { _T("aFWCompileTime[3]"), { 34, 0, 0xffffffff } },
//
//    //ulSubSysNum, dword 35
//    { _T("ulSubSysNum"), { 35, 0, 0xffffffff } },
//
//    //ulSubSysCEMap[2], dword 36 37
//    { _T("ulSubSysCEMap[0]"), { 36, 0, 0xffffffff } },
//    { _T("ulSubSysCEMap[1]"), { 37, 0, 0xffffffff } },
//
//    //ulSubSysCeNum, dword 38
//    { _T("ulSubSysCeNum"), { 38, 0, 0xffffffff } },
//
//    //ulSubSysPlnNum, dword 39
//    { _T("ulSubSysPlnNum"), { 39, 0, 0xffffffff } },
//
//    //ulSubSysBlkNum, dword 40
//    { _T("ulSubSysBlkNum"), { 40, 0, 0xffffffff } },
//
//    //ulSubSysRsvdBlkNum, dword 41
//    { _T("ulSubSysRsvdBlkNum"), { 41, 0, 0xffffffff } },
//
//    //lSubSysFWPageNum, dword 42
//    { _T("ulSubSysFWPageNum"), { 42, 0, 0xffffffff } },
//
//    //ulFlashPageSize, dword 43
//    { _T("ulFlashPageSize"), { 43, 0, 0xffffffff } },
//
//    //ulSysMaxLBACnt, dword 44
//    { _T("ulSysMaxLBACnt"), { 44, 0, 0xffffffff } },
//
//    //ulWorldWideName[2], dword 45 46
//    { _T("ulWorldWideName[0]"), { 45, 0, 0xffffffff } },
//    { _T("ulWorldWideName[1]"), { 46, 0, 0xffffffff } },
//
//    //aFlashChipId[2], dword 47 48
//    { _T("aFlashChipId[0]"), { 47, 0, 0xffffffff } },
//    { _T("aFlashChipId[1]"), { 48, 0, 0xffffffff } },
//
//    //tSATAL0Feature, dword 49~56
//    { _T("bsSATAL0HIPMEnable"), { 49, 0, 0x00000001 } },
//    { _T("bsSATAL0DIPMEnable"), { 49, 1, 0x00000002 } },
//    { _T("bsSATAL0HPAEnable"), { 49, 2, 0x00000004 } },
//    { _T("bsSATAL0SecurityEnable"), { 49, 3, 0x00000008 } },
//    { _T("bsSATAL0SSPEnable"), { 49, 4, 0x00000010 } },
//    { _T("bsSATAL0PMUEnable"), { 49, 5, 0x00000020 } },
//    { _T("bsRsd"), { 49, 6, 0xffffffc0 } },
//    { _T("ulSATAL0FeatureDW[0]"), { 50, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[1]"), { 51, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[2]"), { 52, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[3]"), { 53, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[4]"), { 54, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[5]"), { 55, 0, 0xffffffff } },
//    { _T("ulSATAL0FeatureDW[6]"), { 56, 0, 0xffffffff } },
//
//    //tAHCIL0Feature, dword 57~64
//    { _T("bsAHCIL0HIPMEnable"), { 57, 0, 0x00000001 } },
//    { _T("bsAHCIL0DIPMEnable"), { 57, 1, 0x00000002 } },
//    { _T("bsAHCIL0HPAEnable"), { 57, 2, 0x00000004 } },
//    { _T("bsAHCIL0SecurityEnable"), { 57, 3, 0x00000008 } },
//    { _T("bsAHCIL0SSPEnable"), { 57, 4, 0x00000010 } },
//    { _T("bsAHCIL0PMUEnable"), { 57, 5, 0x00000020 } },
//    { _T("bsRsd"), { 57, 6, 0xffffffc0 } },
//    { _T("ulAHCIL0FeatureDW[0]"), { 58, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[1]"), { 59, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[2]"), { 60, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[3]"), { 61, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[4]"), { 62, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[5]"), { 63, 0, 0xffffffff } },
//    { _T("ulAHCIL0FeatureDW[6]"), { 64, 0, 0xffffffff } },
//
//    //tNVMeL0Feature, dword 65~72
//    { _T("bsNVMeL0MSIXEnable"), { 65, 0, 0x00000003 } },
//    { _T("bsNVMeL0LBA4KBEnable"), { 65, 2, 0x00000004 } },
//    { _T("bsNVMeL0PMUEnable"), { 65, 3, 0x00000008 } },
//    { _T("bsNVMeL0ForceGen2"), { 65, 4, 0x00000010 } },
//    { _T("bsNVMeL0ASPML1"), { 65, 5, 0x00000020 } },
//    { _T("bsRsd"), { 65, 6, 0xffffffc0 } },
//    { _T("ulNVMeL0FeatureDW[0]"), { 66, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[1]"), { 67, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[2]"), { 68, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[3]"), { 69, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[4]"), { 70, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[5]"), { 71, 0, 0xffffffff } },
//    { _T("ulNVMeL0FeatureDW[6]"), { 72, 0, 0xffffffff } },
//
//    //tL1Feature, dword 73~80
//    { _T("bsL1LBAHashingEnable"), { 73, 0, 0x00000001 } },
//    { _T("ulL1FeatureDW[0]"), { 74, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[1]"), { 75, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[2]"), { 76, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[3]"), { 77, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[4]"), { 78, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[5]"), { 79, 0, 0xffffffff } },
//    { _T("ulL1FeatureDW[6]"), { 80, 0, 0xffffffff } },
//
//    //tL2Feature, dword 81~88
//    { _T("ulL2TempFeature"), { 81, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[0]"), { 82, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[1]"), { 83, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[2]"), { 84, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[3]"), { 85, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[4]"), { 86, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[5]"), { 87, 0, 0xffffffff } },
//    { _T("ulL2FeatureDW[6]"), { 88, 0, 0xffffffff } },
//
//    //tL3Feature, dword 89~96
//    { _T("ulL3TempFeature"), { 89, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[0]"), { 90, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[1]"), { 91, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[2]"), { 92, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[3]"), { 93, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[4]"), { 94, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[5]"), { 95, 0, 0xffffffff } },
//    { _T("ulL3FeatureDW[6]"), { 96, 0, 0xffffffff } },
//
//    //tHALFeature, dword 97~104
//    { _T("bsHWDebugTraceEn"), { 97, 0, 0x00000001 } },
//    { _T("ulHALFeatureDW[0]"), { 98, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[1]"), { 99, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[2]"), { 100, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[3]"), { 101, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[4]"), { 102, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[5]"), { 103, 0, 0xffffffff } },
//    { _T("ulHALFeatureDW[6]"), { 104, 0, 0xffffffff } },
//
//    //tRdtTable[12], dword 105~116
//    { _T("tRdtTable[0]"), { 105, 0, 0xffffffff } },
//    { _T("tRdtTable[1]"), { 106, 0, 0xffffffff } },
//    { _T("tRdtTable[2]"), { 107, 0, 0xffffffff } },
//    { _T("tRdtTable[3]"), { 108, 0, 0xffffffff } },
//    { _T("tRdtTable[4]"), { 109, 0, 0xffffffff } },
//    { _T("tRdtTable[5]"), { 110, 0, 0xffffffff } },
//    { _T("tRdtTable[6]"), { 111, 0, 0xffffffff } },
//    { _T("tRdtTable[7]"), { 112, 0, 0xffffffff } },
//    { _T("tRdtTable[8]"), { 113, 0, 0xffffffff } },
//    { _T("tRdtTable[9]"), { 114, 0, 0xffffffff } },
//    { _T("tRdtTable[10]"), { 115, 0, 0xffffffff } },
//    { _T("tRdtTable[11]"), { 116, 0, 0xffffffff } },
//
//    //tTemperatureSensorI2C, dword 117
//    { _T("bsTemperatureSensorType"), { 117, 0, 0x0000000f } },
//    { _T("bsI2CClock"), { 117, 4, 0x000000f0 } },
//    { _T("bsI2CAddr"), { 117, 8, 0x0000ff00 } },
//
//    //end, must be at last
//    { _T(""), { 0, 0, 0x00000000 } }
//};

BOOL AssignPtableFromCfg(UINT uiVal_1, UINT uiVal_2, INT iLine);
void SetPtable(FILE *pBinFile, UINT uiOffset);

#endif