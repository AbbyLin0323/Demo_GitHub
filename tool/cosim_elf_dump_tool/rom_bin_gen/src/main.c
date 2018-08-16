
#include <stdio.h>
#include <string.h>

#include<stdlib.h>
#include<fcntl.h>

#include "elf.h"
#include "main.h"

U8 g_ElfName[64];
U8 g_BinName[64];
U8 g_ElfNum;

U8 *g_pElfImg;
U32 g_ulElfImgLen;
U8 *g_pRomDataBase;
U32 g_ulRomSize;
#if 0
U8 get_argument(int argc,char *argv[])
{
    U8 *pModeSel = argv[1];
    U8 ret = FALSE;
    
    if (argc <= 1)
    {
        printf("Input error!\n");
        printf("<mode> (mcu id) <elf0> (<elf1> <elf2>)\n");
        printf("-s single core . eg: '-s -1 ELF1' , aMCU1 will be created.\n  ");
        printf("-m multi core . eg: '-m ELF0 ELF1 ELF2', aMCU0 aMCU1 aMCU2 will be created.\n");
        exit(0);        
    }

    if (pModeSel[0]=='-')
    {
        if (argc < 3)
        {
            printf("Not enough parameter.\n");
            exit(1);
        }

        if ((*(argv[1]+1)=='s') || (*(argv[1]+1)=='S'))
        {
            
            if(atoi(argv[2]) > 0)
            {
                printf("Select MCU ID, -0/MCU0, -1/MCU1, -2/MCU2.\n");
                exit(1);
            }
            SCoreNum = -(U32)atoi(argv[2]);
 /*           if(SCoreNum > g_ulCoreNum - 1)
            {
                printf("Core location configuration is not existed. \n");
                printf("We limited core number under 3.\n");
                exit(1);
            } */

            g_ulCoreNum = 1;
            g_pLocation += SCoreNum;
            ret = FALSE;
        }
        else if ((*(argv[1]+1)=='m') || (*(argv[1]+1)=='M'))
        {
            if (argc > g_ulCoreNum + 2)
            {
                printf("We limited core number under 3.\n");
                exit(1);
            }
            ret = TRUE;
        }
        else
        {
            printf("Option error. '-s' for single core mode. \n");
            exit(1);
        }
    }
    else
    {
        printf("Input error!\n");
        printf("<mode> (mcu id) <elf0> (<elf1> <elf2>)\n");
        printf("-s single core . eg: '-s -1 ELF1' , aMCU1 will be created.\n  ");
        printf("-m multi core . eg: '-m ELF0 ELF1 ELF2', aMCU0 aMCU1 aMCU2 will be created.\n");
        exit(2);
    }    
    return ret;
}

void InitLocation(void)
{
    U32 i = 0,j = 0,depth;
    char AddrFileName[4][128] = {"Addr0.df","Addr1.df","Addr2.df","Addr3.df"};
    FILE *fp;
    g_ulCoreNum = 3;

    g_pLocation = &g_Location[0];

    for (j=0;j<g_ulCoreNum;j++)
    {
        fp = fopen(AddrFileName[j],"r");
        if (NULL == fp)
        {
            printf("Address file %d open failed.\n",j);
            exit(-1);
        }

        memset ((U8*)&g_Location[j],0,sizeof(LOCATION_ADDRESS));
        fseek(fp,0,SEEK_END);
        depth = ftell(fp);
        fseek(fp,0,SEEK_SET);

        for(i=0;depth != ftell(fp);i++)
        {
            fscanf(fp,"%x",&g_Location[j].m_Content[i]); 
        }
        fclose(fp);
    }

    for (i=0;i<g_ulCoreNum;i++)
    {
        g_Location[i].RomSize       = g_Location[i].RomEnd      - g_Location[i].RomStart;
        g_Location[i].DsramSize     = g_Location[i].DsramEnd    - g_Location[i].DsramStart;
        g_Location[i].IsramSize     = g_Location[i].IsramEnd    - g_Location[i].IsramStart;
        g_Location[i].OtfbSize      = g_Location[i].OtfbEnd     - g_Location[i].OtfbStart;
        g_Location[i].DDRSize       = g_Location[i].DDREnd      - g_Location[i].DDRStart;
    }

}
#endif

void open_elf(int argc, char *argv[])
{
    FILE *fp;
    U32 temp;

    if (argc<3)
    {
        printf("Wrong arguments number !!! Please enter filename again. as 'elf2bin.exe <ELF_NAME> <BIN_NAME>'\n");
        exit(0);
    }
    
    strcpy(g_ElfName,argv[1]);
    strcpy(g_BinName,argv[2]);
    fp = fopen(g_ElfName,"rb");
    if(fp == NULL)
    {
        printf("Open input file error\n");
    }

    fseek(fp,0L,SEEK_END);
    g_ulElfImgLen = ftell(fp);
    fseek(fp,0L,SEEK_SET);

    g_pElfImg = (char *)malloc(g_ulElfImgLen);

    if (g_pElfImg == NULL)
    {
        fprintf(stderr, "Cannot allocate space for %s!\n", argv[1]);
        exit(0);
    }

    temp = fread(g_pElfImg,1,g_ulElfImgLen,fp);
    if(temp!=g_ulElfImgLen)
    {
        fprintf(stderr, "Cannot read %s!\n", argv[1]);
        exit(0);
    }    

    fclose(fp);
}

#define ROM_START_ADDRESS   (0xffe00000)
#define ROM_END_ADDRESS     (0xffe06000)
#define ROM_SIZE            (0x6000)
#define IS_ROM(_addr_)      ((ROM_START_ADDRESS <=_addr_)&&(_addr_ <= ROM_END_ADDRESS))
 
void dump_image(void)
{
    U32 ucNum,i;
    U32 ulTempAddr = 0;
    
    Elf_Ehdr *pElfHead = (Elf_Ehdr*)g_pElfImg;
    Elf_Phdr *pProgHead = (Elf_Phdr*)(g_pElfImg + pElfHead->e_phoff);

    U8 *pRomDataBase;
    U8 *pRomDataStart;
    
    if(!IS_ELF(*pElfHead)) 
    {
        printf("Not a ELF file!!!\n");
        exit(0);
    }
    g_ulRomSize = 0;
    g_pRomDataBase = (U8*)malloc(ROM_SIZE);
    for (i=0;i<ROM_SIZE;i++)
    {
        *(g_pRomDataBase+i) = 0;
    }
	pRomDataStart = g_pRomDataBase;
    for (ucNum=0;ucNum<pElfHead->e_phnum;ucNum++)
    {
        if((ROM_START_ADDRESS <=pProgHead->p_vaddr)&&(pProgHead->p_vaddr <= ROM_END_ADDRESS))
        {
            if ((pProgHead->p_vaddr >= ulTempAddr)&&(0 != pProgHead->p_filesz))
            {
                g_ulRomSize = pProgHead->p_vaddr - ROM_START_ADDRESS + pProgHead->p_filesz;
                ulTempAddr = pProgHead->p_vaddr;
            }
        }
        pRomDataStart = g_pRomDataBase + pProgHead->p_vaddr - ROM_START_ADDRESS;
        for (i=0;i<pProgHead->p_filesz;i++)
        {
            *(U8*)(pRomDataStart+i) = *(U8*)(g_pElfImg + pProgHead->p_offset+i);
        }

        pProgHead++;
    }
    return;
}


void create_rom_bin(void)
{
    FILE *fp;
    U32 i;
    
    fp = fopen(g_BinName,"wb");
    if(fp == NULL)
    {
        printf("Open output file error\n");
    }

    fwrite(g_pRomDataBase,1,0x6000,fp);

    fclose(fp);

    free(g_pRomDataBase);

    return;        
}

void main(int argc, char *argv[])
{
    open_elf(argc,argv);
    dump_image();
    create_rom_bin();
    return;
}