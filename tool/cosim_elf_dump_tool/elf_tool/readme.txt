##########################
##  ELF 解析工具使用说明 
##########################
本工具由window端和linux端两个工具组成。
 windows ELF tool 
path：~/win/tools
（1）从SVN中check out解析工具。同时确保使用时解析工具（multi_core_elf_win.exe）和配置文件(Addr*.df)处于同一目录下。
（2）解析ELF文件
     multi core模式
     eg：multi_core_elf_win.exe -m ELF_CORE0 ELF_CORE1 ELF_CORE2 //生成3个文本文件 aMCU0 aMCU1 aMCU2
     
     single core模式
     eg：multi_core_elf_win.exe -s -0 ELF_CORE0  //生成1个文本文件 aMCU0
     eg：multi_core_elf_win.exe -s -1 ELF_CORE1  //生成1个文本文件 aMCU1
     eg：multi_core_elf_win.exe -s -2 ELF_CORE2  //生成1个文本文件 aMCU2
 
 linux ELF tool
 path: /logic/victorzh/multi_core_elf_tool/
 (1) copy以上目录到本地，确保解析工具（multi_core_elf_linux）和配置文件（mcu*.df）处于同一目录下。
 (2) 将win elf解析工具生成的 aMCU0 aMCU1 aMCU2 复制到Linux目录下 文件名依然是  aMCU0 aMCU1 aMCU2
 (3) 生成preload文件
     multi core模式
     eg：multi_core_elf_linux -m aMCU0 aMCU1 aMCU2 //生成全部preload文件
     
     single core模式
     eg：multi_core_elf_linux -s -0 aMCU0  //生成core0 project 产生的preload 文件
     eg：multi_core_elf_linux -s -1 aMCU1  //生成core1 project 产生的preload 文件
     eg：multi_core_elf_linux -s -2 aMCU2  //生成core2 project 产生的preload 文件   


