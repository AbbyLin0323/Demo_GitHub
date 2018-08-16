#ifndef _FLASH_DEFINE_ 
#define _FLASH_DEFINE_

#define     BOOTLOADER_BUF_SIZE_BITS       14       
#define     BOOTLOADER_BUF_SIZE (1 << BOOTLOADER_BUF_SIZE_BITS)

/*
    Memory Base Address
*/
#define SPI_NOR_FLASH_START_ADDRESS 0xc0000000

/*
    Register Base Address
*/

#define DPTR_BASE_ADDRESS         (REG_BASE_NDC     + 0x100)
#define PRCQ_QEE_BASE             (REG_BASE_NDC     + 0x300)

//#define REG_BASE_UART   (0xffe07000)  
/*
    OTFB Memory
*/
#define FLASH_ID_LOCATION       (OTFB_START_ADDRESS)
#define IDENTIFY_BUFF_BASE      (OTFB_START_ADDRESS + 0x1000)
#define SATA_BUF_BASE           (IDENTIFY_BUFF_BASE + 0x1000)

/*
    Special reg
*/

#define WARM_BOOT_BIT   (0x1 << 0)    
#define WARM_BOOT   (FALSE != (rPMU(0x2c) & WARM_BOOT_BIT))

/*
Base on NFC Interface Register Set  (Rev 0.2 ,2nd Draft,2013/11/22)
*/
#define rNfcPgCfg           (*((volatile U32*)(REG_BASE_NDC + 0x00)))
#define rNfcTCtrl0          (*((volatile U32*)(REG_BASE_NDC + 0x04)))
#define rNfcTCtrl1          (*((volatile U32*)(REG_BASE_NDC + 0x08)))
#define rNfcSsu0Base         (*((volatile U32*)(REG_BASE_NDC + 0x0c)))
#define rNfcSsu1Base         (*((volatile U32*)(REG_BASE_NDC + 0x10)))
#define rNfcRdEccErrInj     (*((volatile U32*)(REG_BASE_NDC + 0x14)))
#define rNfcPrgEccErrInj    (*((volatile U32*)(REG_BASE_NDC + 0x18)))
#define rNfcBypassCtl       (*((volatile U32*)(REG_BASE_NDC + 0x1c)))


#define rNfcDbgSigGrpChg0   (*((volatile U32*)(REG_BASE_NDC + 0x20)))
#define rNfcDbgSigGrpChg1   (*((volatile U32*)(REG_BASE_NDC + 0x24)))
#define rNfcDbgSigGrpChg2   (*((volatile U32*)(REG_BASE_NDC + 0x28)))
#define rNfcToggelModeInDDR         (*((volatile U32*)(REG_BASE_NDC + 0x2c)))
#define rNfcEdoTCtrlECO         (*((volatile U32*)(REG_BASE_NDC + 0x30)))
#define rNfcByPass5         (*((volatile U8*)(REG_BASE_NDC + 0x34)))
#define rNfcDelayCtrl         (*((volatile U32*)(REG_BASE_NDC + 0x38)))
#define rNfcIntAcc          (*((volatile U32*)(REG_BASE_NDC + 0x3c)))
#define rNfcReadID0         (*((volatile U32*)(REG_BASE_NDC + 0x40)))
#define rNfcReadID1         (*((volatile U32*)(REG_BASE_NDC + 0x44)))
#define rNfcSetFeature      (*((volatile U32*)(REG_BASE_NDC + 0x48)))

#define rNfcLogicPuNumCh0Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x50)))
#define rNfcLogicPuNumCh0Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x54)))
#define rNfcLogicPuNumCh1Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x58)))
#define rNfcLogicPuNumCh1Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x5c)))
#define rNfcLogicPuNumCh2Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x60)))
#define rNfcLogicPuNumCh2Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x64)))
#define rNfcLogicPuNumCh3Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x68)))
#define rNfcLogicPuNumCh3Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x6c)))


#define rNfcClkGateCtrl0         (*((volatile U32*)(REG_BASE_NDC + 0x78)))
#define rNfcClkGateCtrl1         (*((volatile U32*)(REG_BASE_NDC + 0x7c)))
#define rNfcDDrDlyComp0          (*((volatile U32*)(REG_BASE_NDC + 0x80))) 
#define rNfcDDrDlyComp1          (*((volatile U32*)(REG_BASE_NDC + 0x84))) 
#define rNfcMisc                (*((volatile U32*)(REG_BASE_NDC + 0x88)))
#define rNfcProgIOTimingCtrl1       (*((volatile U32*)(REG_BASE_NDC + 0x90)))
#define rNfcProgIOTimingCtrl2       (*((volatile U32*)(REG_BASE_NDC + 0x94)))
#define rNfcProgIOTimingCtrl3       (*((volatile U32*)(REG_BASE_NDC + 0x98)))
// reserver  9c
#define rNfcReadDataOutputPIO1       (*((volatile U32*)(REG_BASE_NDC + 0xa0)))
#define rNfcReadDataOutputPIO2       (*((volatile U32*)(REG_BASE_NDC + 0xa4)))



/*NFC interrupt acc*/
#define NF_INTACC_PENDING       0x100
#define NF_INTACC_BWCQ            0x80
#define NF_INTACC_CQPOS            0x40
#define NF_INTACC_PUMSK          0x3f



typedef enum _RAW_CMD_INDEX 
{
    NF_RCMD_RESET = 0,        
    NF_RCMD_READID,
    NF_RCMD_READ_MLC_1PLN,
    NF_RCMD_READ_SLC_1PLN,    
    NF_RCMD_CNT
}RAW_CMD_INDEX;


//*********************normal read 2**************************


#endif

