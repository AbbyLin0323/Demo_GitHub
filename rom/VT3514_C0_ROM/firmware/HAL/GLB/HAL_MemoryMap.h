#ifndef HAL_MEMORYMAP_H___
#define HAL_MEMORYMAP_H___

/* DRAM start address */

#define SRAM0_START_ADDRESS       0x1ff80000
#define SRAM1_START_ADDRESS       0x1ffc0000
#define DRAM_START_ADDRESS        0x40000000
#define DRAM_HIGH_START_ADDRESS   0x80000000
#define DRAM_HIGH_ADDR_OFFSET     (DRAM_HIGH_START_ADDRESS - DRAM_START_ADDRESS)
#define DRAM_ALLCOATE_SIZE        (512*1024*1024)
#define OTFB_START_ADDRESS        0xFFF00000
#define SPI_START_ADDRESS         0xc0000000
/* HW register start address */
#define REG_START_ADDRESS         SRAM0_START_ADDRESS

/* FW/HW interface( DSG, HSG, DRQ, etc. ) SRAM start address */
#define IF_SRAM_BASE     (REG_START_ADDRESS + 0x4000)

/* detail of HW register layout */
#define REG_BASE_GLB            (REG_START_ADDRESS)
#define REG_BASE_SPI            (REG_BASE_GLB       + 0xE0)
#define REG_BASE_SDC            (REG_BASE_GLB       + 0x800)//0x1ff80800
#define REG_BASE_DRAMC          (REG_BASE_SDC       + 0x400)
#define REG_BASE_NDC            (REG_BASE_DRAMC     + 0x400)//0x1ff81000
#define REG_BASE_EM             (REG_BASE_NDC       + 0x800)
#define REG_BASE_LDPC           (REG_BASE_EM        + 0x100) 
#define REG_BASE_BUFM           (REG_BASE_LDPC      + 0x100)//0x1ff81a00
#define REG_BASE_SE0            (REG_BASE_BUFM      + 0x100)
#define REG_BASE_SE1            (REG_BASE_SE0       + 0x100)
#define REG_BASE_DMAE           (REG_BASE_SE1       + 0x100)
#define REG_BASE_TEST           (REG_BASE_DMAE      + 0x100)
#define REG_BASE_PMU            (REG_BASE_TEST      + 0x100)//0x1ff81f00
#define REG_BASE_NVME           (REG_BASE_PMU       + 0x100) //0x1ff82000
#define REG_BASE_PCIE_NVME      (REG_BASE_NVME      + 0x1200) //0x1ff83200
#define REG_BASE_AHCI           (REG_BASE_PCIE_NVME + 0x100) //0x1ff83300
#define REG_BASE_HCT            (REG_BASE_AHCI      + 0x180) //0x1ff83480
#define REG_BASE_PCIE_AHCI      (REG_BASE_HCT       + 0x80)  //0x1ff83500
#define REG_BASE_PCIE           (REG_BASE_PCIE_AHCI + 0x100) //0x1ff83600    
#define REG_BASE_HOSTC          (REG_BASE_PCIE      + 0x400) //0x1ff83a00
#define REG_BASE_SGE            (REG_BASE_HOSTC     + 0x100)//0x1ff83b00
#define REG_BASE_EPHY           (REG_BASE_SGE       + 0x100) //0x1ff83c00       
#define REG_BASE_SE2            (REG_BASE_EPHY      + 0x100)//0x1ff83d00
#define REG_BASE_XOR            (REG_BASE_SE2       + 0x100) //0x1ff83e00
#define REG_BASE_EFUSE          (REG_BASE_XOR       + 0x80)  // 0x1ff83e80
#define REG_BASE_NDC_TRIG       (REG_BASE_XOR       + 0x100) //0x1ff83f00

// for 2x lane system
#define REG_BASE_EPHY_LANE0     (0x1ff838A0) 
#define REG_BASE_EPHY_LANE1     (0x1ff83948) 


/* detail of FW/HW interface( DSG, HSG, DRQ, etc. ) SRAM */

#ifdef VT3514_C0
//in AHCI mode, HSG memory share sata DSG
#define SATA_DSG_BASE             (IF_SRAM_BASE)//0x1FF84000
#define SGEQ_ENTRY_BASE           (SATA_DSG_BASE+0x3000)//0x1FF87000
#define DRQ_BASE_ADDR             (SGEQ_ENTRY_BASE)//0x1FF87000
#define DWQ_BASE_ADDR             (SGEQ_ENTRY_BASE + 0x100)//0x1FF87100
#define SGQ_BASE_ADDR             (SGEQ_ENTRY_BASE + 0x400)//0x1FF87400
#define LDPC_N1_BASE              (SGEQ_ENTRY_BASE+0x800)//0x1FF87800
#define NORMAL_DSG_BASE           (LDPC_N1_BASE+0x800)//0x1FF88000
#define CQ_ENTRY_BASE             (NORMAL_DSG_BASE+0x2000)//0x1FF8A000
#define PRCQ_ENTRY_BASE           (CQ_ENTRY_BASE+0x2000)//0x1FF8C000   
#define EM_LBA_BASE               (PRCQ_ENTRY_BASE+0x4000)//0x1FF90000
#define LDPC_VALUE_BASE           (EM_LBA_BASE+0x4000)//0x1FF94000
#else
//in AHCI mode, HSG memory share sata DSG
#define SATA_DSG_BASE             (IF_SRAM_BASE)//0x1FF84000
#define CQ_ENTRY_BASE             (SATA_DSG_BASE+0x3000)//0x1FF87000
#define NORMAL_DSG_BASE           (CQ_ENTRY_BASE+0x1000)//0x1FF88000
#define PRCQ_ENTRY_BASE           (NORMAL_DSG_BASE+0x2000)//0x1FF8A000   
#define SGEQ_ENTRY_BASE           (PRCQ_ENTRY_BASE+0x2000)//0x1FF8C000
#define DRQ_BASE_ADDR             (SGEQ_ENTRY_BASE)//0x1FF8C000
#define DWQ_BASE_ADDR             (DRQ_BASE_ADDR + 0x100)//0x1FF8C100
#define SGQ_BASE_ADDR             (DWQ_BASE_ADDR + 0x200)//0x1FF8C300
#define LDPC_N1_BASE              (SGEQ_ENTRY_BASE+0x400)//0x1FF8C400
#define DECOMP_BASE               (LDPC_N1_BASE+0x400)//0x1FF8C800
#define LDPC_VALUE_BASE           (DECOMP_BASE+0x800)//0x1FF8D000
#endif


#define APB_BASE        0xFFE07000
#define APB_SIZE        0x9000
#define REG_BASE_UART   (APB_BASE)

#define   SEC_SZ_BITS           9
#define   SEC_SZ                (1<<SEC_SZ_BITS)
#define   SEC_SZ_MSK            (SEC_SZ - 1)

#define rGLB(_x_)   *((volatile U32*)(REG_BASE_GLB + _x_))
#define rNFC(_x_)   *((volatile U32*)(REG_BASE_NDC + _x_))
#define rSDC(_x_)   *((volatile U32*)(REG_BASE_SDC + _x_))
#define rPMU(_x_)   *((volatile U32*)(REG_BASE_PMU + _x_))

#define BOOT_LOADER_BASE    (OTFB_START_ADDRESS)
#define BOOT_LOADER_ENTRY   (BOOT_LOADER_BASE + 0x8)
#define BOOT_LOADER_MAGIC_NUM_DW0   (0x35335456)   //'5''3''T''V'
#define BOOT_LOADER_MAGIC_NUM_DW1   (0x30433431)   //'0''C''4''1'

#endif 

