VT3514_C0：
    指计划于2015年1月tape out的HW
VT3514_B0：
    指计划于2014年09月tape out的HW
VT3514：
    指已经于2014年5月tape out的HW，又被称为VT3514_A0

VT3514_C0_FW_LSP.xws：
    与VT3514_C0对应的WholeChip FW的LSP，包含MCU0的LSP，以及MCU1/2共用的LSP
VT3514_C0_COSIM_MCU0_LSP.xws：
    与VT3514_C0对应的run COSIM的LSP，仅包含MCU0的LSP
VT3514_C0_COSIM_MCU12_LSP.xws：
    与VT3514_C0对应的run COSIM的LSP，仅包含MCU1/2共用的LSP

VT3514_FW_1_060_LSP.xws：
    与VT3514（VT3514_A0/B0）对应的WholeChip FW的LSP，包含MCU0的LSP，以及MCU1/2共用的LSP
VT3514_COSIM_MCU0_LSP.xws：
    与VT3514（VT3514_A0/B0）对应的run COSIM的LSP，仅包含MCU0的LSP
VT3514_COSIM_MCU12_LSP.xws：
    与VT3514（VT3514_A0/B0）对应的run COSIM的LSP，仅包含MCU1/2共用的LSP

注：
1. VT3514_B0在VT3514_A0修正了已知的HW bug，B0的address mapping和A0是一致的，所以B0的LSP和A0是一样的
2. viatie_win32.tgz是A0与B0使用的MCU package；rfviatieloop16_win32.tgz是C0使用的MCU package。
3. LSP与MCU package基于Xplorer-6.0.1创建，请使用Xplorer-6.0.1导入