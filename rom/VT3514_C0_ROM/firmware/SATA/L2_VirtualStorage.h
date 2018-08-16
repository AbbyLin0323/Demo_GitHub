#ifndef L2_VS
#define L2_VS

/* Virtual storage space size: 128MB */
#define VS_RAMSIZE (128 << 20)
#define VIRTUAL_STORAGE_MAX_LBA ( ( VS_RAMSIZE >> 9 ) - 1 )

#define MaxLBAInDisk VIRTUAL_STORAGE_MAX_LBA
void L2_VSSpaceAlloc(U32 *pulFreeDRAMStart);
void L2_VSSectorRW(U32 ulBufferID, U8 ucSecOffset, U8 bRWDir, U32 ulVSLBA);
void L1_L3ClearBufferBusy(U16 usPhyBufID);
void L2_VirtualStorage_ProcessBufferRequest(void);

#endif

