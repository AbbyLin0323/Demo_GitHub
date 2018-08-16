#include "BaseDef.h"
//#include "SSD_Config.h"
#include "HAL_Define.h"
#include "HAL_Cache.h"



void HAL_CacheInit(void)
{
    U32 ulBuf = 0;
    
    // 512M  = 4bit
    //1: write thru
    //2: not cacheble
    //4: write back
    /* init icache and dcache in C code, modified by henryluo */
    xthal_icache_all_unlock();
    xthal_icache_all_invalidate();
    xthal_set_icacheattr(0x22222211);   
    
    xthal_dcache_all_unlock();
    xthal_dcache_all_invalidate();
    xthal_set_dcacheattr(0x22221122);
    
     
    
 //   ulBuf |= 1 << 21; // wire RMCU2_RDCache_EN              = REGGLB_40[21];                            
 //   ulBuf |= 1 << 20; // wire RMCU2_RICache_EN              = REGGLB_40[20];                            
 //   ulBuf |= 1 << 19; // wire RMCU1_RDCache_EN              = REGGLB_40[19];                            
//    ulBuf |= 1 << 18; // wire RMCU1_RICache_EN              = REGGLB_40[18];                            
    ulBuf |= 1 << 17; // wire RMCU0_RDCache_EN              = REGGLB_40[17];                            
    ulBuf |= 1 << 16; // wire RMCU0_RICache_EN              = REGGLB_40[16];   
    
    rREG_GLB_40 |= ulBuf;  

}
