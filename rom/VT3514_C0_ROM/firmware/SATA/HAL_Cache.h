#ifndef _HAL_CHCHE_

#define _HAL_CHCHE_

#define rREG_GLB_40        *(volatile U32*)(REG_BASE_GLB + 0x40)



extern void HAL_CacheInit(void);
#endif
