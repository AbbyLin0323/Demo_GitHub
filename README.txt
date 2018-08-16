1. 本文档描述各级子目录的树状图以及相应说明。
2. 阅读本文档时尽量把记事本的窗口拉宽
3. 理解本文档需要具备VT3514 FW关于支持“多核、多协议、多运行环境”的背景知识

MultiMode				备注
│  README.txt
│  
├─bootloader				与bootloader相关的source code/LSP/script
├─ENV
│  ├─AHCI
│  │  ├─Ramdisk_Win			AHCI mode下的Ramdisk Windows simulation工程
│  │  ├─Ramdisk_Xtensa
│  │  │  └─Ramdisk_MCU12		Ramdisk MCU12的FW在Xplorer下的工程，Ramdisk MCU0的Xplorer工程同WholeChip
│  │  ├─WholeChip_Win
│  │  └─WholeChip_Xtensa
│  │      ├─WholeChip_MCU0
│  │      └─WholeChip_MCU12
│  ├─LSP
│  ├─MixVector			MixVector自定义protocol与AHCI/NVMe没有依赖关系，MixVector在HW为SATA mode时不可用
│  │  ├─MixVector_DOS		MixVector在DOS端的App
│  │  ├─MixVector_Win
│  │  └─MixVector_Xtensa		目前MixVector的protocol处理部分仅支持单核模式
│  ├─NVMe
│  │  ├─Ramdisk_Win
│  │  ├─Ramdisk_Xtensa
│  │  │  └─Ramdisk_MCU12
│  │  ├─WholeChip_Win
│  │  └─WholeChip_Xtensa
│  │      ├─WholeChip_MCU0
│  │      └─WholeChip_MCU12
│  └─SATA				SATA运行在多核模式的方案待定
│      ├─Ramdisk_Win
│      ├─Ramdisk_Xtensa
│      │  └─Ramdisk_MCU12
│      ├─WholeChip_Win
│      └─WholeChip_Xtensa
│          ├─WholeChip_MCU0
│          └─WholeChip_MCU12
├─firmware
│  ├─adapter				各个Layer的FW适配SATA/AHCI/NVMe协议的转换层
│  │  ├─L0
│  │  │  ├─AHCI			放提供给算法的L0_Adapter.h头文件，和实际的Adapter文件（类似于L0_AHCIAdapter.c/.h）
│  │  │  ├─NVMe			放提供给算法的L0_Adapter.h头文件，和实际的Adapter文件（类似于L0_NVMeAdapter.c/.h）
│  │  │  └─SATA
│  │  ├─L1
│  │  │  ├─AHCI
│  │  │  ├─NVMe
│  │  │  └─SATA
│  │  ├─L2
│  │  │  ├─AHCI
│  │  │  ├─NVMe
│  │  │  └─SATA
│  │  └─L3
│  │      ├─AHCI
│  │      ├─NVMe
│  │      └─SATA
│  ├─algorithm			核心算法，与HW、协议无关
│  │  ├─L0
│  │  ├─L1
│  │  ├─L2
│  │  └─L3
│  ├─COM				FW软件通用库
│  └─HAL				Driver
│      │  HAL_Common.c		封装HAL层各子模块的初始化，屏蔽不同mode/环境的差异
│      │  HAL_Common.h		封装HAL层各子模块头文件
│      │
│      ├─DataMonitor			提供FW做data check的支持
│      ├─Debug			按Tag debug硬件问题的模块
│      ├─DMAE
│      ├─DRAM
│      ├─DSG
│      ├─Flash
│      ├─GLB				global register，memory map
│      ├─GPIO
│      ├─HCT
│      ├─Head			boot阶段reallocate FW.bin相关的代码
│      ├─HSG
│      ├─Interrupt
│      ├─MultiCore			spinlock等
│      ├─ParamTable			bootloader与FW交互定义的P/F-table
│      ├─PM				power management
│      ├─SDC				包含SDC相关的BuffMap、SATA DSG
│      ├─SE				search engine
│      ├─SGE
│      ├─UART
│      ├─Vector			汇编代码，boot/中断向量入口
│      └─Xtensa			封装Tensilica MCU内部操作（WAITI/NOP/I-DCACHE等）
├─model				同时支持Windows/XTMP环境的C model
├─unit_test				测试FW
│  ├─HAL_Test
│  ├─L0Ramdisk_FakeL1
│  │  ├─AHCI
│  │  ├─NVMe
│  │  └─SATA
│  ├─L1Ramdisk_FakeL2
│  │  ├─AHCI
│  │  ├─NVMe
│  │  └─SATA
│  ├─L3_Test
│  ├─MixVector_MainFlow
│  └─MixVector_pattern
└─xtmp_env				XTMP环境下基本的运行时环境
