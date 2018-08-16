#include "gen_ptable.h"

//static PTABLE g_tPtable;
static U8 g_PTABLE[PTABLE_SIZE] = {0};

BOOL AssignPtableFromCfg(UINT uiByteOffset, UINT uiVal, INT iLine)
{   
    UINT uiSizeOfPt;   

    //uiDwOfPtable = (PTABLE_SIZE >> 2) - 1;
    uiSizeOfPt = PTABLE_SIZE;
    if (uiByteOffset >= PTABLE_SIZE)
    {
        _tprintf(_T("**ERROR: line_%d, BYTE offset %d is too big, max is %d!\n"), iLine, uiByteOffset, uiSizeOfPt);
        return FALSE;
    }

    g_PTABLE[uiByteOffset] = uiVal & 0xff;
    g_PTABLE[uiByteOffset + 1] = ((uiVal & 0xff00) >> 8);
    g_PTABLE[uiByteOffset + 2] = ((uiVal & 0xff0000 )>> 16);
    g_PTABLE[uiByteOffset + 3] = ((uiVal & 0xff000000 )>> 24);
       
    return TRUE;
}

void SetPtable(FILE *pBinFile, UINT uiOffset)
{
    //PTABLE tPtable;
    //U32 ulFWSavePageCntVal = 46;
    //U32 ulSubSysNumVal = 2;
    //U32 ulSubSysCeNumVal = 4;
    //U32 ulSubSysPlnNumVal = 2;
    //U32 ulSubSysBlkNumVal = 1024;
    //U32 ulSubSysRsvdBlkNumVal = 32;    
    //U32 ulSubSysFWPageNumVal = 512;
    //U32 ulSubSysFWPageSizeVal = 16384;
    //U32 ulSysMaxLBACntVal = 0;

    //U32 aFwDiskName[10] = { 0x41205649,
    //                        0x44205353,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020,
    //                        0x20202020 };
    //U32 aFwSerialNum[5] = { 0x30373030,
    //                        0x30303434,
    //                        0x20203031,
    //                        0x20202020,
    //                        0x20202020 };
    //U32 ulWordWideName[2] = { 0x5001ff20,
    //                          0x6730 };
    //U32 aFlashChipId[2] = { 0x5464842c,
    //                        0xa9 };
    //U32 ulSubSysCEMap[2] = { 0x0, 0x0 };

    ////static flag
    //BOOL bsBootMethodSel = 1;
    //BOOL bsLocalTest = 0;
    //BOOL bsRollBackECT = 0;
    //BOOL bsRebuildGB = 1;
    //BOOL bsUseDefault = 1;
    //BOOL bsOptionRomEn = 0;
    //BOOL bsUartMpMode = 0;
    //BOOL bsLLFMethodSel = 0;
    //BOOL bsExternalSpiFlashValid = 0;
    //BOOL bsUartPrintEn = 1;
    //BOOL bsInheritEraseCntEn = 1;

    ////MCU flag
    //BOOL bsEnableMCU0 = 1;
    //BOOL bsEnableMCU1 = 1;
    //BOOL bsEnableMCU2 = 1;

    ////HW init flag
    //BOOL bsGlobalInitDone = 0;
    //BOOL bsDDRInitDone = 1;
    //BOOL bsNFCInitDone = 0;
    //BOOL bsPCIeOrSDCInitDone = 0;
    //BOOL bsFlash1STDone = 0;
    //BOOL bsFlash2NDDone = 0;
    //BOOL bsClockGatingInitDone = 0;

    ////SATA L0 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsSATAL0HIPMEnable = 0;
    //BOOL bsSATAL0DIPMEnable = 1;
    //BOOL bsSATAL0HPAEnable = 1;
    //BOOL bsSATAL0SecurityEnable = 1;
    //BOOL bsSATAL0SSPEnable = 1;
    //BOOL bsSATAL0PMUEnable = 1;
    //U32 ulSATAL0FeatureDW[7] = { 0 };    

    ////AHCI L0 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsAHCIL0HIPMEnable = 0;
    //BOOL bsAHCIL0DIPMEnable = 1;
    //BOOL bsAHCIL0HPAEnable = 1;
    //BOOL bsAHCIL0SecurityEnable = 1;
    //BOOL bsAHCIL0SSPEnable = 1;
    //BOOL bsAHCIL0PMUEnable = 0;
    //U32 ulAHCIL0FeatureDW[7] = { 0 };   

    ////NVME L0 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsNVMeL0MSIXEnable = 0;
    //BOOL bsNVMeL0LBA4KBEnable = 0;
    //BOOL bsNVMeL0PMUEnable = 1;
    //BOOL bsNVMeL0ForceGen2 = 0;
    //BOOL bsNVMeL0ASPML1 = 1;
    //U32 ulNVMeL0FeatureDW[7] = { 0 };
    //

    ////FW L1 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsL1LBAHashingEnable = 1;
    //U32 ulL1FeatureDW[7] = { 0 };    

    ////FW L2 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //U32 ulL2TempFeature = 0;
    //U32 ulL2FeatureDW[7] = { 0xC64, 5, 20, 512, 0, 0, 0 };

    ////FW L3 Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //U32 ulL3TempFeature = 0;
    //U32 ulL3FeatureDW[7] = { 0 };   

    ////FW HAL Param Init Flag == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsHWDebugTraceEn = 0;        
    //U32 ulHALFeatureDW[7] = { 0x08000000, 0x100000, 0, 0, 0, 0, 0 };   

    ////Temperature sensor setting == == == == == == == == == == == == == == == == == == == == == == == =
    //BOOL bsTemperatureSensorType = 1;   
    //BOOL bsI2CClock = 1;        
    //BOOL bsI2CAddr = 0x48;   

    //memset(&tPtable, 0, sizeof(PTABLE));
   
    ////static flag
    //tPtable.sBootStaticFlag.ulStaticFlag = 0;
    //tPtable.sBootStaticFlag.ulStaticFlag |= (bsBootMethodSel | (bsLocalTest << 3)
    //                                     | (bsRollBackECT << 4) | (bsRebuildGB << 5) 
    //                                     | (bsUseDefault << 6) | (bsOptionRomEn << 7) 
    //                                     | (bsUartMpMode << 9) | (bsLLFMethodSel << 10) 
    //                                     | (bsExternalSpiFlashValid << 13) | (bsUartPrintEn << 15)
    //                                     | (bsInheritEraseCntEn << 16));
    ////enable mcu flag
    //tPtable.sBootSelFlag.ulEnableMCUFlag = 0;
    //tPtable.sBootSelFlag.ulEnableMCUFlag |= (bsEnableMCU0 | (bsEnableMCU1 << 1) 
    //                                     | (bsEnableMCU2 << 2));
    ////hw init flag
    //tPtable.sHwInitFlag.ulHWInitFlag = 0;
    //tPtable.sHwInitFlag.ulHWInitFlag |= ((bsGlobalInitDone) | (bsDDRInitDone << 1)
    //                                  | (bsNFCInitDone << 2) | (bsPCIeOrSDCInitDone << 3)
    //                                  | (bsClockGatingInitDone << 4) | (bsFlash1STDone << 5)
    //                                  | (bsFlash2NDDone << 6));
    ////disk name
    //memcpy(tPtable.aFWDiskName, aFwDiskName, sizeof(U32) * 10);
    ////serial num
    //memcpy(tPtable.aFWSerialNum, aFwSerialNum, sizeof(U32) * 5);
    ////fw save page cnt
    //tPtable.ulFWSavePageCnt = ulFWSavePageCntVal;
    ////subsys num val
    //tPtable.ulSubSysNum = ulSubSysNumVal;
    ////subsys CE MAP
    //memcpy(tPtable.ulSubSysCEMap, ulSubSysCEMap, sizeof(U32) * 2);
    ////subsys ce num
    //tPtable.ulSubSysCeNum = ulSubSysCeNumVal;
    ////subsys palne num
    //tPtable.ulSubSysPlnNum = ulSubSysPlnNumVal;
    ////subsys block num
    //tPtable.ulSubSysBlkNum = ulSubSysBlkNumVal;
    ////subsys rsv block num
    //tPtable.ulSubSysRsvdBlkNum = ulSubSysRsvdBlkNumVal;
    ////subsys fw page num
    //tPtable.ulSubSysFWPageNum = ulSubSysFWPageNumVal;
    ////fw page size
    //tPtable.ulFlashPageSize = ulSubSysFWPageSizeVal;
    ////max LBA cnt
    //tPtable.ulSysMaxLBACnt = ulSysMaxLBACntVal;
    ////world wid name
    //memcpy(tPtable.ulWorldWideName, ulWordWideName, sizeof(U32) * 2);
    ////flash id
    //memcpy(tPtable.aFlashChipId, aFlashChipId, sizeof(U32) * 2);
    ////sata L0 feature
    //tPtable.tSATAL0Feature.ulFeatureBitMap = 0;
    //tPtable.tSATAL0Feature.ulFeatureBitMap |= ((bsSATAL0HIPMEnable) | (bsSATAL0DIPMEnable << 1)
    //                                       | (bsSATAL0HPAEnable << 2) | (bsSATAL0SecurityEnable << 3) 
    //                                       | (bsSATAL0SSPEnable << 4) | (bsSATAL0PMUEnable << 5));
    //memcpy(tPtable.tSATAL0Feature.aFeatureDW, ulSATAL0FeatureDW, sizeof(U32) * 7);
    ////AHCI L0 feature
    //tPtable.tAHCIL0Feature.ulFeatureBitMap = 0;
    //tPtable.tAHCIL0Feature.ulFeatureBitMap |= ((bsAHCIL0HIPMEnable) | (bsAHCIL0DIPMEnable << 1)
    //                                       | (bsAHCIL0HPAEnable << 2) | (bsAHCIL0SecurityEnable << 3) 
    //                                       | (bsAHCIL0SSPEnable << 4) | (bsAHCIL0PMUEnable << 5));
    //memcpy(tPtable.tAHCIL0Feature.aFeatureDW, ulAHCIL0FeatureDW, sizeof(U32) * 7);
    ////NVMe L0 feature
    //tPtable.tNVMeL0Feature.ulFeatureBitMap = 0;
    //tPtable.tNVMeL0Feature.ulFeatureBitMap |= ((bsNVMeL0MSIXEnable) | (bsNVMeL0LBA4KBEnable << 2) 
    //                                       | (bsNVMeL0PMUEnable << 3) | (bsNVMeL0ForceGen2 << 4)) 
    //                                       | (bsNVMeL0ASPML1 << 5);
    //memcpy(tPtable.tNVMeL0Feature.aFeatureDW, ulNVMeL0FeatureDW, sizeof(U32) * 7);
    ////L1 feature
    //tPtable.tL1Feature.ulFeatureBitMap = 0;
    //tPtable.tL1Feature.ulFeatureBitMap |= (bsL1LBAHashingEnable);
    //memcpy(tPtable.tL1Feature.aFeatureDW, ulL1FeatureDW, sizeof(U32) * 7);
    ////L2 feature
    //tPtable.tL2Feature.ulFeatureBitMap = 0;
    //tPtable.tL2Feature.ulFeatureBitMap |= ulL2TempFeature;
    //memcpy(tPtable.tL2Feature.aFeatureDW, ulL2FeatureDW, sizeof(U32) * 7);
    ////L3 feature
    //tPtable.tL3Feature.ulFeatureBitMap = 0;
    //tPtable.tL3Feature.ulFeatureBitMap |= ulL3TempFeature;
    //memcpy(tPtable.tL3Feature.aFeatureDW, ulL3FeatureDW, sizeof(U32) * 7);
    ////HAL feature
    //tPtable.tHALFeature.ulFeatureBitMap = 0;
    //tPtable.tHALFeature.ulFeatureBitMap |= bsHWDebugTraceEn;
    //memcpy(tPtable.tHALFeature.aFeatureDW, ulHALFeatureDW, sizeof(U32) * 7);
    ////temperature sensor
    //tPtable.tTemperatureSensorI2C.bsI2CAddr = bsI2CAddr;
    //tPtable.tTemperatureSensorI2C.bsI2CClock = bsI2CClock;
    //tPtable.tTemperatureSensorI2C.bsTemperatureSensorType = bsTemperatureSensorType;

    fseek(pBinFile, uiOffset, SEEK_SET);
    fwrite(g_PTABLE, sizeof(PTABLE), 1, pBinFile);

    return;
}