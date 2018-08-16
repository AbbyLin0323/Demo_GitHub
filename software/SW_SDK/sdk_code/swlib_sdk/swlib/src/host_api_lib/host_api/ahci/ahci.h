#ifndef _AHCI
#define AHCI
#include <ntddscsi.h>
typedef struct 
{
	ATA_PASS_THROUGH_EX apt;
	U32 filler;
	U8 data_buf[128*1024];
} ATA_PASS_THROUGH_EX_WITH_BUFFERS;
// IDE��ID����ص�����
// ��512�ֽ�(256��WORD)�������������һЩ����Ȥ����(����������ATA/ATAPI-4)
typedef struct _IDINFO
{
    U16  wGenConfig;                 // WORD 0: ������Ϣ��
    U16  wNumCyls;                   // WORD 1: ������
    U16  wReserved2;                 // WORD 2: ����
    U16  wNumHeads;                  // WORD 3: ��ͷ��
    U16  wReserved4;                 // WORD 4: ����
    U16  wReserved5;                 // WORD 5: ����
    U16  wNumSectorsPerTrack;        // WORD 6: ÿ�ŵ�������
    U16  wVendorUnique[3];           // WORD 7-9: �����趨ֵ
    U8   sSerialNumber[20];          // WORD 10-19:���к�
    U16  wBufferType;                // WORD 20: ��������
    U16  wBufferSize;                // WORD 21: �����С
    U16  wECCSize;                   // WORD 22: ECCУ���С
    U8   sFirmwareRev[8];            // WORD 23-26: �̼��汾
    U8   sModelNumber[40];           // WORD 27-46: �ڲ��ͺ�
    U16  wMoreVendorUnique;          // WORD 47: �����趨ֵ
    U16  wReserved48;                // WORD 48: ����
    struct 
    {
        U16  reserved1:8;
        U16  DMA:1;                  // 1=֧��DMA
        U16  LBA:1;                  // 1=֧��LBA
        U16  DisIORDY:1;             // 1=�ɲ�ʹ��IORDY
        U16  IORDY:1;                // 1=֧��IORDY
        U16  SoftReset:1;            // 1=��ҪATA������
        U16  Overlap:1;              // 1=֧���ص�����
        U16  Queue:1;                // 1=֧���������
        U16  InlDMA:1;               // 1=֧�ֽ����ȡDMA
    } wCapabilities;                    // WORD 49: һ������
    U16  wReserved1;                 // WORD 50: ����
    U16  wPIOTiming;                 // WORD 51: PIOʱ��
    U16  wDMATiming;                 // WORD 52: DMAʱ��
    struct 
    {
        U16  CHSNumber:1;            // 1=WORD 54-58��Ч
        U16  CycleNumber:1;          // 1=WORD 64-70��Ч
        U16  UnltraDMA:1;            // 1=WORD 88��Ч
        U16  reserved:13;
    } wFieldValidity;                   // WORD 53: �����ֶ���Ч�Ա�־
    U16  wNumCurCyls;                // WORD 54: CHS��Ѱַ��������
    U16  wNumCurHeads;               // WORD 55: CHS��Ѱַ�Ĵ�ͷ��
    U16  wNumCurSectorsPerTrack;     // WORD 56: CHS��Ѱַÿ�ŵ�������
    U16  wCurSectorsLow;             // WORD 57: CHS��Ѱַ����������λ��
    U16  wCurSectorsHigh;            // WORD 58: CHS��Ѱַ����������λ��
    struct 
    {
        U16  CurNumber:8;            // ��ǰһ���Կɶ�д������
        U16  Multi:1;                // 1=��ѡ���������д
        U16  reserved1:7;
    } wMultSectorStuff;                 // WORD 59: ��������д�趨
    U32  dwTotalSectors;              // WORD 60-61: LBA��Ѱַ��������
    U16  wSingleWordDMA;             // WORD 62: ���ֽ�DMA֧������
    struct 
    {
        U16  Mode0:1;                // 1=֧��ģʽ0 (4.17Mb/s)
        U16  Mode1:1;                // 1=֧��ģʽ1 (13.3Mb/s)
        U16  Mode2:1;                // 1=֧��ģʽ2 (16.7Mb/s)
        U16  Reserved1:5;
        U16  Mode0Sel:1;             // 1=��ѡ��ģʽ0
        U16  Mode1Sel:1;             // 1=��ѡ��ģʽ1
        U16  Mode2Sel:1;             // 1=��ѡ��ģʽ2
        U16  Reserved2:5;
    } wMultiWordDMA;                    // WORD 63: ���ֽ�DMA֧������
    struct 
    {
        U16  AdvPOIModes:8;          // ֧�ָ߼�POIģʽ��
        U16  reserved:8;
    } wPIOCapacity;                     // WORD 64: �߼�PIO֧������
    U16  wMinMultiWordDMACycle;      // WORD 65: ���ֽ�DMA�������ڵ���Сֵ
    U16  wRecMultiWordDMACycle;      // WORD 66: ���ֽ�DMA�������ڵĽ���ֵ
    U16  wMinPIONoFlowCycle;         // WORD 67: ��������ʱPIO�������ڵ���Сֵ
    U16  wMinPOIFlowCycle;           // WORD 68: ��������ʱPIO�������ڵ���Сֵ
    U16  wReserved69[11];            // WORD 69-79: ����
    struct 
    {
        U16  Reserved1:1;
        U16  ATA1:1;                 // 1=֧��ATA-1
        U16  ATA2:1;                 // 1=֧��ATA-2
        U16  ATA3:1;                 // 1=֧��ATA-3
        U16  ATA4:1;                 // 1=֧��ATA/ATAPI-4
        U16  ATA5:1;                 // 1=֧��ATA/ATAPI-5
        U16  ATA6:1;                 // 1=֧��ATA/ATAPI-6
        U16  ATA7:1;                 // 1=֧��ATA/ATAPI-7
        U16  ATA8:1;                 // 1=֧��ATA/ATAPI-8
        U16  ATA9:1;                 // 1=֧��ATA/ATAPI-9
        U16  ATA10:1;                // 1=֧��ATA/ATAPI-10
        U16  ATA11:1;                // 1=֧��ATA/ATAPI-11
        U16  ATA12:1;                // 1=֧��ATA/ATAPI-12
        U16  ATA13:1;                // 1=֧��ATA/ATAPI-13
        U16  ATA14:1;                // 1=֧��ATA/ATAPI-14
        U16  Reserved2:1;
    } wMajorVersion;                    // WORD 80: ���汾
    U16  wMinorVersion;              // WORD 81: ���汾
    //U16  wReserved82[6];             // WORD 82-87: ����
    struct 
    {
        //WORD82
        U16 Smart:1;                //SMART support
        U16 Security:1;             //SECURITY support
        U16 obsolete822:1;
        U16 MandPowerMgt:1;           //mandatory power management support
        U16 Packet:1;               //PACKET support
        U16 WriteCache:1;           //volatile write cache support
        U16 ReadAhead:1;            //read look-ahead support
        U16 ReleaseU32:1;           //release U32errupt support
        U16 Service:1;              //SERVICE U32errupt support
        U16 DeviceReset:1;          //DEVICE RESET command support
        U16 HPA:1;                  //HPA support
        U16 obsolete8211:1;
        U16 WriteBuffer:1;          //Write buffer command support
        U16 ReadBuffer:1;           //Read buffer command support
        U16 NOP:1;                  //NOP command support
        U16 obsolete8215:1;

        //WORD83
        U16 DownloadMicrocode:1;     //Download MicroCode command support
        U16 TCQ:1;                   //TCQ feature support
        U16 CFA:1;                   //CFA support
        U16 APM:1;                   //APM support
        U16 obsolete834:1;
        U16 PUIS:1;                  //PUIS support
        U16 SetFeatures:1;           //Set Features sub command is required to spin-up after power up
        U16 reserved837:1;
        U16 SetMaxSecurity:1;        //SET MAX security extension support
        U16 AAM:1;                   //AAM support
        U16 Addr48bit:1;             //48bit address support
        U16 DCO:1;                   //DCO support
        U16 FlushCache:1;            //Flush Cache command support
        U16 FlushCacheExt:1;          //Flush Cache EXT command support
        U16 rsvd8314:1;
        U16 rsvd8315:1;

        //WORD84
        U16 SmartErrLog:1;          //SMART error logging support
        U16 SmartSelfTest:1;        //SMART self test support
        U16 MediaSn:1;              //Media serial number support
        U16 MediaCardPassThrough:1; //Media Card Pass Through support
        U16 Streaming:1;            //Streaming support
        U16 GPL:1;                  //GPL support
        U16 WdfeWmfe:1;             //Write DMA FUA EXT and Write Multiple FUA Ext command support
        U16 Wdqfe:1;                //Write DMA QUEUED FUA EXT command support
        U16 WWN64bit:1;              //64bits world wide name support
        U16 obsolete849:1;
        U16 obsolete8410:1;
        U16 rsvd8411:1;
        U16 rsvd8412:1;
        U16 Unload:1;                //IDLE IMMEDIATE command with unload feature support
        U16 rsvd8414:1;
        U16 rsvd8415:1;       
    } wCmdFeatureSupport;             //WORD82-84: command and feature support

    struct
    {
        //WORD85
        U16 Smart:1;                //SMART enable
        U16 Security:1;             //SECURITY enable
        U16 obsolete852:1;
        U16 MandPowerMgt:1;           //mandatory power management support
        U16 Packet:1;               //PACKET support
        U16 WriteCache:1;           //volatile write cache enable
        U16 ReadAhead:1;            //read look-ahead enable
        U16 ReleaseU32:1;           //release U32errupt enable
        U16 Service:1;              //SERVICE U32errupt enable
        U16 DeviceReset:1;          //DEVICE RESET command support
        U16 HPA:1;                  //HPA support
        U16 obsolete8511:1;
        U16 WriteBuffer:1;          //Write buffer command support
        U16 ReadBuffer:1;           //Read buffer command support
        U16 NOP:1;                  //NOP command support
        U16 obsolete8515:1;

        //WORD86
        U16 DownloadMicrocode:1;     //Download MicroCode command support
        U16 TCQ:1;                   //TCQ feature support
        U16 CFA:1;                   //CFA support
        U16 APM:1;                   //APM enable
        U16 obsolete864:1;
        U16 PUIS:1;                  //PUIS enable
        U16 SetFeatures:1;           //Set Features sub command is required to spin-up after power up
        U16 reserved867:1;
        U16 SetMaxSecurity:1;        //SET MAX security extension enable
        U16 AAM:1;                   //AAM enable
        U16 Addr48bit:1;             //48bit address support
        U16 DCO:1;                   //DCO support
        U16 FlushCache:1;            //Flush Cache command support
        U16 FlushCacheExt:1;          //Flush Cache EXT command support
        U16 rsvd8614:1;
        U16 Word119120Valid:1;         //Words 119..120 valid   

        //WORD87
        U16 SmartErrLog:1;          //SMART error logging support
        U16 SmartSelfTest:1;        //SMART self test support
        U16 MediaSn:1;              //Media serial number is valid
        U16 MediaCardPassThrough:1; //Media Card Pass Through support
        U16 obsolete874:1;            
        U16 GPL:1;                  //GPL support
        U16 WdfeWmfe:1;             //Write DMA FUA EXT and Write Multiple FUA Ext command support
        U16 Wdqfe:1;                //Write DMA QUEUED FUA EXT command support
        U16 WWN64bit:1;              //64bits world wide name support
        U16 obsolete879:1;
        U16 obsolete8710:1;
        U16 rsvd8711:1;
        U16 rsvd8712:1;
        U16 Unload:1;                //IDLE IMMEDIATE command with unload feature support
        U16 rsvd8714:1;
        U16 rsvd8715:1;       
    } wCmdFeatureEnable;              //WORD85-87: command and feature enable
    
    struct 
    {
        U16  Mode0:1;                // 1=֧��ģʽ0 (16.7Mb/s)
        U16  Mode1:1;                // 1=֧��ģʽ1 (25Mb/s)
        U16  Mode2:1;                // 1=֧��ģʽ2 (33Mb/s)
        U16  Mode3:1;                // 1=֧��ģʽ3 (44Mb/s)
        U16  Mode4:1;                // 1=֧��ģʽ4 (66Mb/s)
        U16  Mode5:1;                // 1=֧��ģʽ5 (100Mb/s)
        U16  Mode6:1;                // 1=֧��ģʽ6 (133Mb/s)
        U16  Mode7:1;                // 1=֧��ģʽ7 (166Mb/s) ???
        U16  Mode0Sel:1;             // 1=��ѡ��ģʽ0
        U16  Mode1Sel:1;             // 1=��ѡ��ģʽ1
        U16  Mode2Sel:1;             // 1=��ѡ��ģʽ2
        U16  Mode3Sel:1;             // 1=��ѡ��ģʽ3
        U16  Mode4Sel:1;             // 1=��ѡ��ģʽ4
        U16  Mode5Sel:1;             // 1=��ѡ��ģʽ5
        U16  Mode6Sel:1;             // 1=��ѡ��ģʽ6
        U16  Mode7Sel:1;             // 1=��ѡ��ģʽ7
    } wUltraDMA;                        // WORD 88:  Ultra DMA֧������
    U16 wReserved89[11];         // WORD 89-99
    U32 dwTotalSectors48bit;      //WORD 100-103, total sectors for 48bits command
    U32 dwTotalSector48bitsHigh;
    U16 wReserved104[152];             //WORD104~255
} IDINFO, *PIDINFO;
#endif