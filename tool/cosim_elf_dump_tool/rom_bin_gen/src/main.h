typedef unsigned char    U8;
typedef unsigned short   U16;
typedef unsigned int     U32;
typedef int              S32;
typedef short            S16;
typedef char             S8;

/*
#define MCU0
#ifdef MCU0
#define  SROM_LOCATION_START_ADDRESS    0xffe00000      // ffe0,0000 - ffe0,5fff
#define  SROM_LOCATION_END_ADDRESS		0xffe05fff

#define  DSRAM_LOCATION_START_ADDRESS   0x1ffa4000      // 
#define  DSRAM_LOCATION_END_ADDRESS		0x1ffa7fff

#define  ISRAM_LOCATION_START_ADDRESS  	0x20000000   
#define  ISRAM_LOCATION_END_ADDRESS		0x20007fff  

#define  OTFB_LOCATION_START_ADDRESS  	0xfff00000  
#define  OTFB_LOCATION_END_ADDRESS		(0xfff00000 + 0x4ffff) 

#define  SDRAM_LOCATION_START_ADDRESS 	0x40000000   
#define  SDRAM_LOCATION_END_ADDRESS		0xefdfffff  

#elif defined(MCU1)

#define  SROM_LOCATION_START_ADDRESS    0xffe00000      // ffe0,0000 - ffe0,5fff
#define  SROM_LOCATION_END_ADDRESS		0xffe05fff

#define  DSRAM_LOCATION_START_ADDRESS   0x1ffa0000      // 
#define  DSRAM_LOCATION_END_ADDRESS		0x1ffa7fff

#define  ISRAM_LOCATION_START_ADDRESS  	0x20008000   
#define  ISRAM_LOCATION_END_ADDRESS		0x2007ffff  

#define  OTFB_LOCATION_START_ADDRESS  	0xfff00000  
#define  OTFB_LOCATION_END_ADDRESS		(0xfff00000 + 0x4ffff) 

#define  SDRAM_LOCATION_START_ADDRESS 	0x40000000   
#define  SDRAM_LOCATION_END_ADDRESS		0xefdfffff 
#elif defined(MCU2) 

#define  SROM_LOCATION_START_ADDRESS    0xffe00000      // ffe0,0000 - ffe0,5fff
#define  SROM_LOCATION_END_ADDRESS		0xffe05fff

#define  DSRAM_LOCATION_START_ADDRESS   0x1ffa0000      // 
#define  DSRAM_LOCATION_END_ADDRESS		0x1ffa7fff

#define  ISRAM_LOCATION_START_ADDRESS  	0x20008000   
#define  ISRAM_LOCATION_END_ADDRESS		0x2007ffff  

#define  OTFB_LOCATION_START_ADDRESS  	0xfff00000  
#define  OTFB_LOCATION_END_ADDRESS		(0xfff00000 + 0x4ffff) 

#define  SDRAM_LOCATION_START_ADDRESS 	0x40000000   
#define  SDRAM_LOCATION_END_ADDRESS		0xefdfffff 
#endif 

#define Rom_SIZE                        (SROM_LOCATION_END_ADDRESS - SROM_LOCATION_START_ADDRESS)       
#define Data_SRAM_SIZE                  (DSRAM_LOCATION_END_ADDRESS - DSRAM_LOCATION_START_ADDRESS)     
#define Instruction_SRAM_SIZE           (ISRAM_LOCATION_END_ADDRESS - ISRAM_LOCATION_START_ADDRESS)   
#define OTFB_SIZE                       (OTFB_LOCATION_END_ADDRESS - OTFB_LOCATION_START_ADDRESS)
*/
#define DRAM_SIZE 524288		//DRAM_SIZE 0X80000   512k   The Size of the code which in DRAM is limited ,
								// so 512k is enough . 

#define CORE_NUM_MAX    8

typedef union _LOCATION_ADDRESS__
{   
    U32 m_Content[15];
    struct {
        U32 RomStart;
        U32 RomEnd;   
        U32 DsramStart;
        U32 DsramEnd;
        U32 IsramStart;
        U32 IsramEnd;
        U32 OtfbStart;
        U32 OtfbEnd;
        U32 DDRStart;
        U32 DDREnd;

        U32 RomSize;
        U32 DsramSize;
        U32 IsramSize;
        U32 OtfbSize;
        U32 DDRSize;
    };
}LOCATION_ADDRESS;

LOCATION_ADDRESS g_Location[CORE_NUM_MAX];
LOCATION_ADDRESS *g_pLocation;
U32 g_ulCoreNum;


U32 rom_base_addr;
U32 data_sram_base_addr;
U32 instruction_sram_base_addr;
U32 otfb_base_addr;
U32 dram_base_addr;


U32 rom_start_addr;
U32 data_sram_start_addr;
U32 instruction_sram_start_addr;
U32 otfb_start_addr;
U32 dram_start_addr;


/* Bin Table */
typedef struct _Bin_Table{
	int	offset;	    /* segment offset */
	int	vaddr;	    /* virtual address of segment */
	int	filesize;	/* number of bytes in file for seg. */
	int	valid;	    /* flags */
} Bin_Table;

Bin_Table Table[5];

int Temp_Table_vaddr[5];

char inputfile[512];

#define  LITTLE_ENDIAN
#define ELF_TARG_VER	1	/* The ver for which this code is intended */

static char *strtab;
int is64 = 0;
int do_swap = 0;
unsigned long elf_hash(const unsigned char *name);