##########################
##  ELF ��������ʹ��˵�� 
##########################
��������window�˺�linux������������ɡ�
 windows ELF tool 
path��~/win/tools
��1����SVN��check out�������ߡ�ͬʱȷ��ʹ��ʱ�������ߣ�multi_core_elf_win.exe���������ļ�(Addr*.df)����ͬһĿ¼�¡�
��2������ELF�ļ�
     multi coreģʽ
     eg��multi_core_elf_win.exe -m ELF_CORE0 ELF_CORE1 ELF_CORE2 //����3���ı��ļ� aMCU0 aMCU1 aMCU2
     
     single coreģʽ
     eg��multi_core_elf_win.exe -s -0 ELF_CORE0  //����1���ı��ļ� aMCU0
     eg��multi_core_elf_win.exe -s -1 ELF_CORE1  //����1���ı��ļ� aMCU1
     eg��multi_core_elf_win.exe -s -2 ELF_CORE2  //����1���ı��ļ� aMCU2
 
 linux ELF tool
 path: /logic/victorzh/multi_core_elf_tool/
 (1) copy����Ŀ¼�����أ�ȷ���������ߣ�multi_core_elf_linux���������ļ���mcu*.df������ͬһĿ¼�¡�
 (2) ��win elf�����������ɵ� aMCU0 aMCU1 aMCU2 ���Ƶ�LinuxĿ¼�� �ļ�����Ȼ��  aMCU0 aMCU1 aMCU2
 (3) ����preload�ļ�
     multi coreģʽ
     eg��multi_core_elf_linux -m aMCU0 aMCU1 aMCU2 //����ȫ��preload�ļ�
     
     single coreģʽ
     eg��multi_core_elf_linux -s -0 aMCU0  //����core0 project ������preload �ļ�
     eg��multi_core_elf_linux -s -1 aMCU1  //����core1 project ������preload �ļ�
     eg��multi_core_elf_linux -s -2 aMCU2  //����core2 project ������preload �ļ�   


