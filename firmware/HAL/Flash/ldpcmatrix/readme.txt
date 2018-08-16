The ldpc matrix include two set:
                          0      1    2      3          
set:LDPC_MAT_MODE(0):    136    72    88    104
set:LDPC_MAT_MODE(1):    136    112   120   128


Note:
1. LDPC_MAT_MODE select which set to be used.
2. LDPC_MAT_SEL_FST_15K/LDPC_MAT_SEL_LASE_1K select which mamber to be used.
3. Different NAND Flash support Matrixes.

                   LDPC_MAT_MODE    LDPC_MAT_SEL_FST_15K    LDPC_MAT_SEL_LASE_1K
   Intel 3D MLC/TLC         1                    3                       0
   L95                      0                    3                       0
   TSB 3D TLC               1                    1                       0
   TSB 3D MLC               0                    1                       2
   FourPln                  0                    1                       0
   TSB 2D TLC               1                    1                       0

