1. ���ĵ�����������Ŀ¼����״ͼ�Լ���Ӧ˵����
2. �Ķ����ĵ�ʱ�����Ѽ��±��Ĵ�������
3. ��Ȿ�ĵ���Ҫ�߱�VT3514 FW����֧�֡���ˡ���Э�顢�����л������ı���֪ʶ

MultiMode				��ע
��  README.txt
��  
����bootloader				��bootloader��ص�source code/LSP/script
����ENV
��  ����AHCI
��  ��  ����Ramdisk_Win			AHCI mode�µ�Ramdisk Windows simulation����
��  ��  ����Ramdisk_Xtensa
��  ��  ��  ����Ramdisk_MCU12		Ramdisk MCU12��FW��Xplorer�µĹ��̣�Ramdisk MCU0��Xplorer����ͬWholeChip
��  ��  ����WholeChip_Win
��  ��  ����WholeChip_Xtensa
��  ��      ����WholeChip_MCU0
��  ��      ����WholeChip_MCU12
��  ����LSP
��  ����MixVector			MixVector�Զ���protocol��AHCI/NVMeû��������ϵ��MixVector��HWΪSATA modeʱ������
��  ��  ����MixVector_DOS		MixVector��DOS�˵�App
��  ��  ����MixVector_Win
��  ��  ����MixVector_Xtensa		ĿǰMixVector��protocol�����ֽ�֧�ֵ���ģʽ
��  ����NVMe
��  ��  ����Ramdisk_Win
��  ��  ����Ramdisk_Xtensa
��  ��  ��  ����Ramdisk_MCU12
��  ��  ����WholeChip_Win
��  ��  ����WholeChip_Xtensa
��  ��      ����WholeChip_MCU0
��  ��      ����WholeChip_MCU12
��  ����SATA				SATA�����ڶ��ģʽ�ķ�������
��      ����Ramdisk_Win
��      ����Ramdisk_Xtensa
��      ��  ����Ramdisk_MCU12
��      ����WholeChip_Win
��      ����WholeChip_Xtensa
��          ����WholeChip_MCU0
��          ����WholeChip_MCU12
����firmware
��  ����adapter				����Layer��FW����SATA/AHCI/NVMeЭ���ת����
��  ��  ����L0
��  ��  ��  ����AHCI			���ṩ���㷨��L0_Adapter.hͷ�ļ�����ʵ�ʵ�Adapter�ļ���������L0_AHCIAdapter.c/.h��
��  ��  ��  ����NVMe			���ṩ���㷨��L0_Adapter.hͷ�ļ�����ʵ�ʵ�Adapter�ļ���������L0_NVMeAdapter.c/.h��
��  ��  ��  ����SATA
��  ��  ����L1
��  ��  ��  ����AHCI
��  ��  ��  ����NVMe
��  ��  ��  ����SATA
��  ��  ����L2
��  ��  ��  ����AHCI
��  ��  ��  ����NVMe
��  ��  ��  ����SATA
��  ��  ����L3
��  ��      ����AHCI
��  ��      ����NVMe
��  ��      ����SATA
��  ����algorithm			�����㷨����HW��Э���޹�
��  ��  ����L0
��  ��  ����L1
��  ��  ����L2
��  ��  ����L3
��  ����COM				FW���ͨ�ÿ�
��  ����HAL				Driver
��      ��  HAL_Common.c		��װHAL�����ģ��ĳ�ʼ�������β�ͬmode/�����Ĳ���
��      ��  HAL_Common.h		��װHAL�����ģ��ͷ�ļ�
��      ��
��      ����DataMonitor			�ṩFW��data check��֧��
��      ����Debug			��Tag debugӲ�������ģ��
��      ����DMAE
��      ����DRAM
��      ����DSG
��      ����Flash
��      ����GLB				global register��memory map
��      ����GPIO
��      ����HCT
��      ����Head			boot�׶�reallocate FW.bin��صĴ���
��      ����HSG
��      ����Interrupt
��      ����MultiCore			spinlock��
��      ����ParamTable			bootloader��FW���������P/F-table
��      ����PM				power management
��      ����SDC				����SDC��ص�BuffMap��SATA DSG
��      ����SE				search engine
��      ����SGE
��      ����UART
��      ����Vector			�����룬boot/�ж��������
��      ����Xtensa			��װTensilica MCU�ڲ�������WAITI/NOP/I-DCACHE�ȣ�
����model				ͬʱ֧��Windows/XTMP������C model
����unit_test				����FW
��  ����HAL_Test
��  ����L0Ramdisk_FakeL1
��  ��  ����AHCI
��  ��  ����NVMe
��  ��  ����SATA
��  ����L1Ramdisk_FakeL2
��  ��  ����AHCI
��  ��  ����NVMe
��  ��  ����SATA
��  ����L3_Test
��  ����MixVector_MainFlow
��  ����MixVector_pattern
����xtmp_env				XTMP�����»���������ʱ����
