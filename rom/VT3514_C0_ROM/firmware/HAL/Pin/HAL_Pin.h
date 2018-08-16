/*------------------------------------------------------------------------------
Copyright (c) 2014 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :   HAL StripePin.h                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :   Victor Zhang

Description: 
    Interface of hardware stripe pins , ROM boot program may select boot/init 
    mode by these pins.
    
Modification History:
20141023    Victor Zhang   created
------------------------------------------------------------------------------*/

#ifndef HAL_STRIP_H
#define HAL_STRIP_H

typedef union STRAP_PIN_
{ 
    struct
    {
        U32 bsBootSrc0      :1;
        U32 bsBootSrc1      :1;
        U32 bsBootSrc2      :1;
        U32 bsSataPcieSel   :1;       
        U32 bsAsyncSyncSel  :1;
        U32 bsTlcMlcSel     :1;
        U32 bsToggleOnfiSel :1;
        U32 bsNvmeAhciSel   :1;    
        U32 bsEPRomSel      :1;
        U32 bs16CeMode      :1;
    };
    U8  ucGLB40;
    U32 ulStrapPin;
}STRAP_PIN;

enum STRIP_PARAM_
{
    STRAP_UART_DIS  = 1,
    STRAP_UART_EN   = 0,

    STRAP_BOOT_NORMAL = 0x00,
    STRAP_BOOT_MP     = 0x3,

    STRAP_SATA      = 0,
    STRAP_PCIE      = 1,

    STRAP_AHCI      = 0,
    STRAP_NVME      = 1, 

    STRAP_FLASH     = 0,
    STRAP_SPI       = 1,

    STRAP_SYNC      = 0,
    STRAP_ASYNC     = 1,

    STRAP_MLC       = 0,
    STRAP_TLC       = 1,

    STRAP_TOGGLE    = 0,
    STRAP_ONFI      = 1    
};

#define SET_EPHY_REG(_base_,_off_) *(volatile U8*)((_base_)+(_off_))
#define EPHY_REG_NUM (7)
typedef struct EPHY_REG_
{
    U16 bsAddr:7;
    U16 bsValid:1;
    U16 bsData:8;
}EPHY_REG;

typedef struct EFUSE_FILE_
{
    union 
    {
        U32 ulKey;
        U32 ulDW0;
    };
    
    //  Read flash command prcq
    union 
    {
        struct {
            U8 ucQE[4]; 
        };
        U32 ulDW1;
    };

    union 
    {
        struct{
            /* DW0 byte 0 */
            U32 bsPMU_NFCLKSEL:2;        // Set this field to 0x1ff81f20[7:6]
            U32 bsIsEfuseSet:1;         // 1: Init HW from EFUSE 0: Use ROM default setting                           
            U32 bsIsONFI:1;             // 1: ONFI 0:Toggle (if it is ONFI, set 0x1ff81000[24] =1)
            U32 bsIsASYNC:1;            //1: ASYNC 0: SYNC (if it is SYNC, set 0x1ff8102c = 0x3f)
            U32 bsNFC_ECCMode:2;        //00-ECC24, 01-ECC40,10-ECC64, 11-ECC72
            U32 bsNFC_CLAddrByte:1;     //0: 2byte Addr in CL read 1: 5byte Addr in CL read
            //if it is Toggle & it is Sync, set 0x1ff81088[23] =0

            /* DW0 byte 1 */
            U32 bsNFC_PageSize:2;       //00-4KB,01-8KB,10-16KB,11-32KB
            U32 bsNFC_TWHRLD:3;         //Set this field to 0x1ff81004[30:28] 
            U32 bsNFC_TDBS:2;           // Set this field to 0x1ff81004[27:26]
            U32 bsNFC_TCAD_3:1;         //Set this field to 0x1ff81004[25]

            /* DW0 byte 2 */
            U32 bsNFC_TCS:1;            // Set this field to 0x1ff81004[24]
            U32 bsNFC_TCLHZ:2;          // Set this field to 0x1ff81004[21:20]
            U32 bsNFC_RDRPH:3;          // Set this field to 0x1ff81004[19:17]
            U32 bsNFC_TRPST:2;          // Set this field to 0x1ff81004[15:14]

            /* DW0 byte 3 */
            U32 bsNFC_TRPSTH:1;         // Set this field to 0x1ff81004[16]
            U32 bsNFC_TCAD:3;           // Set this field to 0x1ff81004[9:7]
            U32 bsNFC_TCCS_LDV:4;       // Set this field to 0x1ff81004[6:3]
        };
        U32 ulDW2;
    };

    union 
    {
        struct {
            /* DW1 byte 0 */
            U32 bsNFC_TRPRE:2;          // Set this field to 0x1ff81004[13:12]
            U32 bsNFC_SlowRead:1;       // Set this field to 0x1ff81004[0]
            U32 bsNFC_TRHWLD:5;         // Set this field to 0x1ff81008[31:27]

            /* DW1 byte 1 */
            U32 bsNFC_DDR_CFG:1;        // Set this field to 0x1ff81008[23]
            U32 bsNFC_DDR_HF_CFG:1;     // Set this field to 0x1ff81008[22] 
            U32 bsNFC_TDQSRE:3;         // Set this field to 0x1ff81008[2:0]
            U32 bsNFC_DQSHZ_TH:3;       // Set this field to 0x1ff81088[27:25]
            //If it is ONFI & bsNFC_DDR_HF_CFG ==1, set 0x1ff81088[23] to 0
            //If it is ONFI & bsNFC_DDR_HF_CFG ==0, set 0x1ff81088[23] to 1

            /* DW2 byte 2 */
            U32 bsNFC_CHxNFCKGCMPCNT:6;  // Set this field to 0x1ff81080[6:1][14:9][22:17][30:25]
            U32 bsNFC_DQSHZ_CHK_DIS:1;   // Set this field to 0x1ff81088[24]
            U32 bsHostIF_Reset:1;        //If Reset == 1,
                                         //If it is PCIE mode, set 0x1ff80018 bit 29 = 0
                                         //If it is SATA mode, set 0x1ff80018 bit[11:8] to 4'b=0
                                         //If reset == 0 do nothing
            /* DW3 byte 3 */
            U32 bsNormal_REG0_DATA:8 ;  
        };
        U32 ulDW3;
    };

    // DW 4~ 7
    EPHY_REG usEPHY_REG[EPHY_REG_NUM]; 
    U16 bsNormal_REG0_ADDR:8;
    U16 bsCheckSum:8; 

}EFUSE_FILE;

#define EFUSE_WDATA_DEPTH   (8)
#define EFUSE_RDATA_DEPTH   (8)

typedef struct EFUSE_REG_SET_
{
    U32 aDATA[EFUSE_WDATA_DEPTH];
    U32 Res[EFUSE_RDATA_DEPTH];
    struct {
        U32 bsProgramed :1;
        U32 bsDataValid :1;
        U32 Res         :30;
    };
}EFUSE_REG_SET;

typedef struct NFC_REG_00{
    U32 PG_SIZE:2;         // 00-4k ,01-8k ,10-16k ,11-32k
    U32 FINISH_MODE:1;   // 0: set FINISH before update SSU.
                        // 1: set FINISH after update SSU    
    U32 Res:1;             
    U32 CL_ADDR_BYTE:1;  // 1- 5byte ,0 - 2byte    
    U32 ECC_SELECT:2;       // 00-Ecc24,01-Ecc40,10-Ecc64
    U32 NF_INT_MSK:1;     // 1-mask ,0-not mask
    
    U32 REDNUM:2;       // 00-16byte,01-32byte,10-48byte,11-64byte    
    U32 CP_DEPTH:2;    // 00-4DW,01-8DW,10-16DW,11-32DW.
    U32 SCR_RO_EN:1;     // 0 - Scramble seed not rotate ,1 - rotate 
    U32 RbLoc:3;        // RB bit location in byte.(defined in SQEE. Will remove)

    U32 SF_LOC:3;        // Success/Fail bit location in a byte.
    U32 ECC_TH:5;        // ECC error counter threshold. In bit.
                        // [4]: means X4 or not. If it is 1, {ECC_TH[3:0],2'b00} is the real threshold.
                        // [3:0]: Basic value of threshold.
                            
    U32 IS_MICRON:1;     // It is ONFI device or not. 1-MICRON device, 0-Not MICRON device (if DDR_MODE=1, indicate it is toggle NAND).
    U32 BCHECCET_EN:2;     // BCHECC early termination enable
    U32 MUL_LUN:2;       // Multi-LUN number:
                        // 00: not multi-LUN
                        // 01: 2 LUN in one target
                        // 10: 4 LUN in one target
                        // 11: 8 LUN in one target
    U32 SKIP_RED:1;      // 1: enable skip redundant data for LDPC
    U32 CHARB_EN:1;      // 1: Arbitrate in 1K data. 0: Arbitrate in 256 byte data.
    U32 DISHP_CMD:1;     // Disable command higher priority. 1-disable, 0-not disable.
}NFC_REG_00;

typedef struct NFC_REG_04{
    U32 SLOWRD :1;            
    U32 SLOWRD_CNTH_STSRD_CNT:2;
    U32 TCCS_LDV:4;
    U32 TCAD:3;
    U32 TWPRE:2;
    U32 TRPRE:2;
    U32 TRPST:2;
        
    U32 TRPSTH:1;
    U32 RDRPH:3;
    U32 TCLHZ:2;
    U32 TWPRE_2:1;
    U32 RSTAG_HOLD_EN:1;
    U32 TCS:1;
    U32 TCAD_3:1;
    U32 TDBS:2;
    U32 TWHRLD:3;
    U32 CE_HOLD_EN:1;       
}NFC_REG_04;

typedef struct NFC_REG_08
{
    U32 TDQSRE     :3;
    U32 TCWAWEN    :1;
    U32 ENP_BPS    :1;
    U32 TMR_TH1    :3;
    U32 TMR_TH0    :3;
    U32 Reserved   :3;
    U32 TADL_LD    :3;
    U32 TCWAW      :5;
    U32 DDR_HF_CFG :1;
    U32 DDR_CFG    :1;
    U32 ECC_ACC    :2;
    U32 ODT_EN     :1;
    U32 TRHWLD     :5;
}NFC_REG_08;

typedef struct NFC_REG_80
{ 
/*
    U32 CH0NFCKGCMPCNT   :7;
    U32 Res0             :1;
    U32 CH1NFCKGCMPCNT   :7;
    U32 Res1             :1;
    U32 CH2NFCKGCMPCNT   :7;
    U32 Res2             :1;
    U32 CH3NFCKGCMPCNT   :7;
    U32 Res3             :1;
*/
    U32 Res              :1;
    U32 CH0NFCKGCMPCNT   :6;
    U32 Res0             :2;
    U32 CH1NFCKGCMPCNT   :6;
    U32 Res1             :2;
    U32 CH2NFCKGCMPCNT   :6;
    U32 Res2             :2;
    U32 CH3NFCKGCMPCNT   :6;
    U32 Res3             :1;


}NFC_REG_80;

typedef struct NFC_REG_88
{                           
    U32 CH0DLYCNT           :2;
    U32 CH1DLYCNT           :2;
    U32 CH2DLYCNT           :2;
    U32 CH3DLYCNT           :2;
    
    U32 CDQSSTH             :5;
    U32 WPSTH_EN            :1;
    U32 WPSTHTH             :2;
    
    U32 RES2                :1;
    U32 RES1                :1;
    U32 CDQSS_CHK_DIS       :1;
    U32 CDQSSHTH            :2;
    U32 CDQSSH_CHK_DIS      :1;
    U32 CDQSSH_CHK_CMD_ONLY :1;
    U32 DRIVE_DQS_IN_IDLE   :1;
    
    U32 DQSHZ_CHK_DIS       :1;
    U32 DQSHZ_TH            :3;
    U32 TO_CNT              :2;
    U32 TO_EN               :1;
    U32 DELAY_ACC_EN        :1;
}NFC_REG_88;

U32 HAL_StrapGetPageSize(void);
BOOL HAL_StrapIsEfuse(void);
BOOL HAL_StrapIsSetHwFrEfuse(void);
void HAL_StrapEfuseSetHW(void);
void HAL_StrapInit(void)           ;        
BOOL HAL_StrapUartEn(void)         ;
BOOL HAL_StrapMpBoot(void)         ;
BOOL HAL_StrapSpiBoot(void)        ;
BOOL HAL_StrapIsPcie(void)         ;
U8  HAL_StrapNfcType(void)         ;
BOOL HAL_StrapNfcIsTLC(void)       ;
BOOL HAL_StrapNfcIsOnfi(void)      ;
BOOL HAL_StrapNfcIsAsync(void)     ;
U32 HAL_StrapGetWarmBootEntry(void);
BOOL HAL_StrapNfcIsNVMe(void)      ;

void HAL_StrapCheckEfuse(void);

#endif 

