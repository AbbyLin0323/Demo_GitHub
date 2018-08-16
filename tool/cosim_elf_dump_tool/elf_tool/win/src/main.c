
#include <stdio.h>
#include <string.h>

#include<stdlib.h>
#include<fcntl.h>

#include "elf.h"
#include "main.h"

U32 p_base_addr[5] ;
void CreateTempFile(const char* OutFileName);


int debug[16]={0};
static void
swap(char *mem, int len)
{
	int t, i;

	for (i = 0; i < len / 2; i++)
	{
		t = mem[i];
		mem[i] = mem[len - i - 1];
		mem[len - i - 1] = t;
	}
}



static void
dump_header(Elf_Ehdr *hdr)
{
	int i;

	printf("ELF header:\n");
	printf("ident=0x");

	for (i = 0; i < EI_NIDENT; i++)
		printf("%2X.", hdr->e_ident[i]);

	printf("\n");

#ifdef LITTLE_ENDIAN
	do_swap = (hdr->e_ident[EI_DATA] == ELFDATA2MSB);
#else
	do_swap = (hdr->e_ident[EI_DATA] == ELFDATA2LSB);
#endif

	if (do_swap)
	{
		swap((char*)&hdr->e_type, sizeof hdr->e_type);
		swap((char*)&hdr->e_machine, sizeof hdr->e_machine);
		swap((char*)&hdr->e_version, sizeof hdr->e_version);
		swap((char*)&hdr->e_entry, sizeof hdr->e_entry);
		swap((char*)&hdr->e_phoff, sizeof hdr->e_phoff);
		swap((char*)&hdr->e_shoff, sizeof hdr->e_shoff);
		swap((char*)&hdr->e_flags, sizeof hdr->e_flags);
		swap((char*)&hdr->e_ehsize, sizeof hdr->e_ehsize);
		swap((char*)&hdr->e_phentsize, sizeof hdr->e_phentsize);
		swap((char*)&hdr->e_phnum, sizeof hdr->e_phnum);
		swap((char*)&hdr->e_shentsize, sizeof hdr->e_shentsize);
		swap((char*)&hdr->e_shnum, sizeof hdr->e_shnum);
		swap((char*)&hdr->e_shstrndx, sizeof hdr->e_shstrndx);
	}

	printf("type=0x%X machine=0x%X version=0x%X entry=0x%" X "\n",
			hdr->e_type, hdr->e_machine, hdr->e_version, hdr->e_entry);
	printf("phoff=0x%" X " shoff=0x%" X " flags=0x%X ehsize=0x%X\n",
			hdr->e_phoff, hdr->e_shoff, hdr->e_flags, hdr->e_ehsize);
	printf(
		"phentsize=0x%X phunm=0x%X shentsize=0x%X shnum=0x%X shstrndx=0x%X\n",
			hdr->e_phentsize, hdr->e_phnum, hdr->e_shentsize,
			hdr->e_shnum, hdr->e_shstrndx);
}

static void
dump_program_header(Elf_Phdr *p, Elf_Word i)
{
	int j;
	
	printf("program header %d:\n", i);
	p += i;

	if (do_swap)
	{
		swap((char*)&p->p_type, sizeof p->p_type);
		swap((char*)&p->p_offset, sizeof p->p_offset);
		swap((char*)&p->p_vaddr, sizeof p->p_vaddr);
		swap((char*)&p->p_paddr, sizeof p->p_paddr);
		swap((char*)&p->p_filesz, sizeof p->p_filesz);
		swap((char*)&p->p_memsz, sizeof p->p_memsz);
		swap((char*)&p->p_flags, sizeof p->p_flags);
		swap((char*)&p->p_align, sizeof p->p_align);
	}

	printf("type=0x%X offset=0x%" X " vaddr=0x%" X " paddr=0x%" X "\n",
			p->p_type, p->p_offset, p->p_vaddr, p->p_paddr);
	printf("filesz=0x%" X " memsz=0x%" X " flags=0x%X align=0x%" X "\n",
			p->p_filesz, p->p_memsz, p->p_flags, p->p_align);
}

static void
dump_section_header(Elf_Shdr *s, Elf_Word i)
{
	printf("section header %d:\n", i);
	s += i;

	if (do_swap)
	{
		swap((char*)&s->sh_name, sizeof s->sh_name);
		swap((char*)&s->sh_type, sizeof s->sh_type);
		swap((char*)&s->sh_flags, sizeof s->sh_flags);
		swap((char*)&s->sh_addr, sizeof s->sh_addr);
		swap((char*)&s->sh_offset, sizeof s->sh_offset);
		swap((char*)&s->sh_size, sizeof s->sh_size);
		swap((char*)&s->sh_link, sizeof s->sh_link);
		swap((char*)&s->sh_info, sizeof s->sh_info);
		swap((char*)&s->sh_addralign, sizeof s->sh_addralign);
		swap((char*)&s->sh_entsize, sizeof s->sh_entsize);
	}

	printf("name=%s (0x%X) type=0x%X flags=0x%" X " addr=0x%" X " offset=0x%" X "\n",
			strtab + s->sh_name,
			s->sh_name, s->sh_type, s->sh_flags, s->sh_addr, s->sh_offset);
	printf("size=0x%" X " link=0x%X info=0x%X addralign=0x%" X " entsize=0x%" X "\n",
			s->sh_size, s->sh_link, s->sh_info, s->sh_addralign, s->sh_entsize);
}

static void
dump_symbols(char *image, Elf_Ehdr *hdr, Elf_Shdr *scn)
{
	Elf_Shdr *symtab = NULL;
	Elf_Shdr *strhdr = NULL;
	Elf_Word i = 0;
	Elf_Word num;
	char *strs;
#ifdef INT64
	Elf64_Sym *syms, *s;
#else
	Elf32_Sym *syms, *s;
#endif

	for (i = 0; i < hdr->e_shnum; i++)
	{
		if (strcmp(strtab + scn[i].sh_name, ELF_SYMTAB) == 0)
			symtab = scn + i;
		else if (strcmp(strtab + scn[i].sh_name, ELF_STRTAB) == 0)
			strhdr = scn + i;
	}

	if (symtab == NULL || strhdr == NULL)
	{
		printf("NULL symtab and/or strhdr?!?\n");
		return;
	}

	if (symtab->sh_entsize != sizeof *syms)
	{
		printf("symtab->sh_entsize = " D " != sizeof *syms = %d?!?\n",
				symtab->sh_entsize, sizeof *syms);
		return;
	}

	strs = image + strhdr->sh_offset;
#if INT64
	s = syms = (Elf64_Sym*)(image + symtab->sh_offset);
#else
	s = syms = (Elf32_Sym*)(image + symtab->sh_offset);
#endif
	num = symtab->sh_size / sizeof *syms;

	for (i = 0; i < num; i++, s++)
	{
		if (do_swap)
		{
			swap((char*)&s->st_name, sizeof s->st_name);
			swap((char*)&s->st_value, sizeof s->st_value);
			swap((char*)&s->st_size, sizeof s->st_size);
			swap((char*)&s->st_shndx, sizeof s->st_shndx);
		}

		if ((ELF_ST_BIND(s->st_info) == STB_GLOBAL ||
				ELF_ST_BIND(s->st_info) == STB_LOCAL) &&
				(ELF_ST_TYPE(s->st_info) == STT_OBJECT ||
				ELF_ST_TYPE(s->st_info) == STT_FUNC))

				printf("%*" D ": %s value=0x%" X " size=0x%" X " info=0x%X shndx=0x%X\n",
				sizeof i, i, strs + s->st_name, s->st_value, s->st_size,
					s->st_info, s->st_shndx);
	}
}




static void
dumpimage(char *image, Elf_Word imagelen)
{
	Elf_Ehdr *hdr;
	Elf_Shdr *scnhdr;
	Elf_Phdr *proghdr;
	Elf_Off	sh_offset;
	Elf_Word i, j,k,m,n,len = 0;


	int haswrite_bin = 0;
	int haswrite_table = 0;

	int haswrite_isram_bin[8] = {0};
	int haswrite_dsram_bin_byte[4][4] = {0};

	int dw;
	int dw_max;

	int table_dw;
	int table_dw_max;

	hdr = (Elf_Ehdr*)image;

	if (!IS_ELF(*hdr))
	{
		printf("not ELF binary\n");
		return;
	}

#ifdef PRINT
	dump_header(hdr);
#endif
	proghdr = (Elf_Phdr*)(image + hdr->e_phoff); 
	scnhdr = (Elf_Shdr*)(image + hdr->e_shoff);
	sh_offset = scnhdr[hdr->e_shstrndx].sh_offset;

	if (do_swap)
		swap((char*)&sh_offset, sizeof sh_offset);

	strtab = image + sh_offset;

	for (i = 0; i < hdr->e_shnum; i++)
	{
		#ifdef PRINT
		dump_section_header(scnhdr, i);
		#endif

		if (scnhdr[i].sh_type != SHT_NOBITS &&
				len < scnhdr[i].sh_offset + scnhdr[i].sh_size)
			len = scnhdr[i].sh_offset + scnhdr[i].sh_size;
	}

	////////////////get the five type data from the elf file////////////////////

	for (i = 0; i < hdr->e_phnum; i++)
	{
		//get rom data
//		if((proghdr->p_vaddr >= SROM_LOCATION_START_ADDRESS) && (proghdr->p_vaddr <= SROM_LOCATION_END_ADDRESS))
        if((proghdr->p_vaddr >= g_pLocation->RomStart) && (proghdr->p_vaddr <= g_pLocation->RomEnd))
		{
			if((proghdr->p_vaddr >= Temp_Table_vaddr[0]) && (proghdr->p_filesz!=0))
			{
				Table[0].filesize = (proghdr->p_vaddr - g_pLocation->RomStart) + proghdr->p_filesz;	
				if((Table[0].filesize%16) != 0)
					Table[0].filesize = Table[0].filesize + (16 - (Table[0].filesize%16));

				Temp_Table_vaddr[0] = proghdr->p_vaddr;
			}

			rom_start_addr = rom_base_addr + (proghdr->p_vaddr - g_pLocation->RomStart);

			for(j = 0; j < proghdr->p_filesz/4; j++)
			{
				*(U32 *)rom_start_addr = *(U32 *)((U32)hdr + proghdr->p_offset);
				rom_start_addr = rom_start_addr + 4;
				proghdr->p_offset= proghdr->p_offset + 4;			
			}

			if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) != 0)
			{
				if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 1)
					*(U32 *)rom_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x000000FF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 2)
					*(U32 *)rom_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x0000FFFF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 3)
					*(U32 *)rom_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x00FFFFFF;
			}
		}	
		//get data sram data
//		else if((proghdr->p_vaddr >= DSRAM_LOCATION_START_ADDRESS) && (proghdr->p_vaddr <= DSRAM_LOCATION_END_ADDRESS))	
        else if((proghdr->p_vaddr >= g_pLocation->DsramStart) && (proghdr->p_vaddr <= g_pLocation->DsramEnd))	
		{			
			if((proghdr->p_vaddr >= Temp_Table_vaddr[1]) && (proghdr->p_filesz!=0))
			{
				Table[1].filesize = (proghdr->p_vaddr - g_pLocation->DsramStart) + proghdr->p_filesz;
				if((Table[1].filesize%4) != 0)
					Table[1].filesize = Table[1].filesize + (4 - (Table[1].filesize%4));

				Temp_Table_vaddr[1] = proghdr->p_vaddr;
			}
				
			data_sram_start_addr = data_sram_base_addr + (proghdr->p_vaddr - g_pLocation->DsramStart);
			
			for(j = 0; j < proghdr->p_filesz/4; j++)
			{
				*(U32 *)data_sram_start_addr =*(U32 *)((U32)hdr + proghdr->p_offset);
					debug[0] = *(U32 *)data_sram_start_addr;
				data_sram_start_addr = data_sram_start_addr + 4;
				proghdr->p_offset= proghdr->p_offset + 4;		
			}

			if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) != 0)
			{
				if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 1)
					*(U32 *)data_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x000000FF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 2)
					*(U32 *)data_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x0000FFFF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 3)
					*(U32 *)data_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x00FFFFFF;
			}
			

		}	
		//get instruction_sram data
///		else if((proghdr->p_vaddr >= ISRAM_LOCATION_START_ADDRESS  ) && (proghdr->p_vaddr <= ISRAM_LOCATION_END_ADDRESS	))	
        else if((proghdr->p_vaddr >= g_pLocation->IsramStart) && (proghdr->p_vaddr <= g_pLocation->IsramEnd))
		{			
            if((proghdr->p_vaddr >= Temp_Table_vaddr[2]) && (proghdr->p_filesz!=0))
			{
				Table[2].filesize = (proghdr->p_vaddr - g_pLocation->IsramStart  ) + proghdr->p_filesz;
				if((Table[2].filesize%16) != 0)
					Table[2].filesize = Table[2].filesize + (16 - (Table[2].filesize%16));

				Temp_Table_vaddr[2] = proghdr->p_vaddr;
			}
	
			instruction_sram_start_addr = instruction_sram_base_addr + (proghdr->p_vaddr - g_pLocation->IsramStart  );

			for(j = 0; j < proghdr->p_filesz/4; j++)
			{
				*(U32 *)instruction_sram_start_addr = *(U32 *)((U32)hdr + proghdr->p_offset);
				instruction_sram_start_addr = instruction_sram_start_addr + 4;
				proghdr->p_offset= proghdr->p_offset + 4;
			}

			if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) != 0)
			{
				if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 1)
					*(U32 *)instruction_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x000000FF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 2)
					*(U32 *)instruction_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x0000FFFF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 3)
					*(U32 *)instruction_sram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x00FFFFFF;
			}



		}
		//get OTFB data 
//		else if((proghdr->p_vaddr >= OTFB_LOCATION_START_ADDRESS) && (proghdr->p_vaddr <= OTFB_LOCATION_END_ADDRESS))
        else if((proghdr->p_vaddr >= g_pLocation->OtfbStart) && (proghdr->p_vaddr <= g_pLocation->OtfbEnd))
		{			
			if((proghdr->p_vaddr >= Temp_Table_vaddr[3]) && (proghdr->p_filesz!=0))
			{
				Table[3].filesize = (proghdr->p_vaddr - g_pLocation->OtfbStart) + proghdr->p_filesz;	
				if((Table[3].filesize%16) != 0)
					Table[3].filesize = Table[3].filesize + (16 - (Table[3].filesize%16));

				Temp_Table_vaddr[3] = proghdr->p_vaddr;
			}
		
			otfb_start_addr = otfb_base_addr + (proghdr->p_vaddr - g_pLocation->OtfbStart);

			for(j = 0; j < proghdr->p_filesz/4; j++)
			{
				*(U32 *)otfb_start_addr = *(U32 *)((U32)hdr + proghdr->p_offset);
				otfb_start_addr = otfb_start_addr + 4;
				proghdr->p_offset= proghdr->p_offset + 4;				
			}

			if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) != 0)
			{
				if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 1)
					*(U32 *)otfb_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x000000FF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 2)
					*(U32 *)otfb_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x0000FFFF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 3)
					*(U32 *)otfb_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x00FFFFFF;
			}
		}
		//get DRAM data
//		else if((proghdr->p_vaddr >= SDRAM_LOCATION_START_ADDRESS) && (proghdr->p_vaddr <= SDRAM_LOCATION_END_ADDRESS))
        else if((proghdr->p_vaddr >= g_pLocation->DDRStart) && (proghdr->p_vaddr <= g_pLocation->DDREnd))
		{
			if((proghdr->p_vaddr >= Temp_Table_vaddr[4]) && (proghdr->p_filesz!=0))
			{
				Table[4].filesize = (proghdr->p_vaddr - g_pLocation->DDRStart) + proghdr->p_filesz;	
				if((Table[4].filesize%16) != 0)
					Table[4].filesize = Table[4].filesize + (16 - (Table[4].filesize%16));

				Temp_Table_vaddr[4] = proghdr->p_vaddr;
			}
	
			dram_start_addr = dram_base_addr + (proghdr->p_vaddr - g_pLocation->DDRStart);

			for(j = 0; j < proghdr->p_filesz/4; j++)
			{
				*(U32 *)dram_start_addr = *(U32 *)((U32)hdr + proghdr->p_offset);
				dram_start_addr = dram_start_addr + 4;
				proghdr->p_offset= proghdr->p_offset + 4;			
			}

			if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) != 0)
			{
				if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 1)
					*(U32 *)dram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x000000FF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 2)
					*(U32 *)dram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x0000FFFF;
				else if((proghdr->p_filesz - (proghdr->p_filesz/4) * 4) == 3)
					*(U32 *)dram_start_addr = (*(U32 *)((U32)hdr + proghdr->p_offset)) & 0x00FFFFFF;
			}
		}

		proghdr = proghdr++;
	}
}

void DeleteExistFile(const char* OutFileName)
{
	int i = 0;
	remove(OutFileName);
}
void PrepareSpace()
{
	int dw;
	int dw_max;

	//initial the rom space
	rom_base_addr	= (U32)malloc(g_pLocation->RomSize);
	dw_max = g_pLocation->RomSize / 4;

	for(dw = 0;dw < dw_max;dw++)
	{
		*(((U32*)rom_base_addr) + dw) = 0; 
	}
	for(dw = 0;dw < dw_max;dw++)
	{
		if(*((char*)rom_base_addr+dw) != 0)
			printf("rom error");
	}
		
	//initial the data_sram space
	data_sram_base_addr = (U32)malloc(g_pLocation->DsramSize);
	dw_max = g_pLocation->DsramSize / 4;

	for(dw = 0; dw < dw_max; dw ++)
	{
		*(((U32*)data_sram_base_addr) + dw) = 0; 
	}

	//initial the instruction_sram space
	instruction_sram_base_addr = (U32)malloc(g_pLocation->IsramSize);
	dw_max = g_pLocation->IsramSize / 4;

	for(dw = 0; dw < dw_max; dw ++)
	{
		*(((U32*)instruction_sram_base_addr) + dw) = 0; 
	}

	//initial the otfb space
	otfb_base_addr = (U32)malloc(g_pLocation->OtfbSize);
	dw_max = g_pLocation->OtfbSize / 4;

	for(dw = 0; dw < dw_max; dw ++)
	{
		*(((U32*)otfb_base_addr) + dw) = 0; 
	}

	//initial the dram space
	dram_base_addr = (U32)malloc(DRAM_SIZE);
	dw_max = DRAM_SIZE / 4;

	for(dw = 0; dw < dw_max; dw ++)
	{
		*(((U32*)dram_base_addr) + dw) = 0; 
	}


	p_base_addr[0] = rom_base_addr;
	p_base_addr[1] = data_sram_base_addr ;
	p_base_addr[2] = instruction_sram_base_addr ;
	p_base_addr[3] = otfb_base_addr ;
	p_base_addr[4] = dram_base_addr ;
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
/*
    g_Location[0].RomStart      = 0xffe00000;
    g_Location[0].RomEnd        = 0xffe05fff;
    g_Location[0].DsramStart    = 0x1ffa4000;
    g_Location[0].DsramEnd      = 0x1ffa7fff;
    g_Location[0].IsramStart    = 0x20000000;
    g_Location[0].IsramEnd      = 0x20007fff;
    g_Location[0].OtfbStart     = 0xfff00000;
    g_Location[0].OtfbEnd       = 0xfff4ffff;
    g_Location[0].DDRStart      = 0x40000000;
    g_Location[0].DDREnd        = 0xefdfffff;

    g_Location[1].RomStart      = 0xffe00000;
    g_Location[1].RomEnd        = 0xffe05fff;
    g_Location[1].DsramStart    = 0x1ffa0000;
    g_Location[1].DsramEnd      = 0x1ffa7fff;
    g_Location[1].IsramStart    = 0x20000000;
    g_Location[1].IsramEnd      = 0x2007ffff;
    g_Location[1].OtfbStart     = 0xfff00000;
    g_Location[1].OtfbEnd       = 0xfff4ffff;
    g_Location[1].DDRStart      = 0x40000000;
    g_Location[1].DDREnd        = 0xefdfffff;

    g_Location[2].RomStart      = 0xffe00000;
    g_Location[2].RomEnd        = 0xffe05fff;
    g_Location[2].DsramStart    = 0x1ffa0000;
    g_Location[2].DsramEnd      = 0x1ffa7fff;
    g_Location[2].IsramStart    = 0x20000000;
    g_Location[2].IsramEnd      = 0x2007ffff;
    g_Location[2].OtfbStart     = 0xfff00000;
    g_Location[2].OtfbEnd       = 0xfff4ffff;
    g_Location[2].DDRStart      = 0x40000000;
    g_Location[2].DDREnd        = 0xefdfffff;
*/

    for (i=0;i<g_ulCoreNum;i++)
    {
        g_Location[i].RomSize       = g_Location[i].RomEnd      - g_Location[i].RomStart;
        g_Location[i].DsramSize     = g_Location[i].DsramEnd    - g_Location[i].DsramStart;
        g_Location[i].IsramSize     = g_Location[i].IsramEnd    - g_Location[i].IsramStart;
        g_Location[i].OtfbSize      = g_Location[i].OtfbEnd     - g_Location[i].OtfbStart;
        g_Location[i].DDRSize       = g_Location[i].DDREnd      - g_Location[i].DDRStart;
    }

}

const char * OutFileName[] = {"aMCU0","aMCU1","aMCU2","aMCU3"};
#define SINGLE_CORE_MODE    1
#define MULTI_CORE_MODE     0
int main(int argc, char *argv[])
{
	FILE *fp;
	char *image;
	int imagelen;
	int breadlength;
    int temp;
    U32 i = 0;
    U32 SCoreNum = 0;
    U32 mode = MULTI_CORE_MODE;
    const char *pOutputFileName;

    InitLocation();
    if (*argv[1]=='-')
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
                printf("Core number must be greater than 0.\n");
                exit(1);
            }
            SCoreNum = -(U32)atoi(argv[2]);
            if(SCoreNum > g_ulCoreNum - 1)
            {
                printf("Core location configuration is not existed. \n");
                printf("We limited core number under 3.\n");
                exit(1);
            }

            g_ulCoreNum = 1;
            g_pLocation += SCoreNum;
   //         strcpy(inputfile,argv[3]);
            mode = SINGLE_CORE_MODE;
        }
        else if ((*(argv[1]+1)=='m') || (*(argv[1]+1)=='M'))
        {
            if (argc > g_ulCoreNum + 2)
            {
                printf("We limited core number under 3.\n");
                exit(1);
            }
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


    for (i=0;i<g_ulCoreNum;i++)
    {
       // Table[]
        memset(Table,0,sizeof(Bin_Table)*5);
        memset(Temp_Table_vaddr,0,sizeof(int)*5);
        if(SINGLE_CORE_MODE == mode)
        {
            strcpy(inputfile,argv[3]);
            pOutputFileName = OutFileName[SCoreNum];
        }
        else
        {
            strcpy(inputfile,argv[i+2]);
            pOutputFileName = OutFileName[i];
        }
        
        fp = fopen(inputfile,"rb");
        if(fp == NULL)
        {
	        printf("Open input file error\n");
        }

        DeleteExistFile(pOutputFileName);	
        PrepareSpace();


        fseek(fp,0L,SEEK_END);
        imagelen = ftell(fp);
        fseek(fp,0L,SEEK_SET);

        image = (char *)malloc(imagelen);

        if (image == NULL)
        {
	        fprintf(stderr, "Cannot allocate space for %s!\n", argv[1]);
	        exit(3);
        }

        temp = fread(image,1,imagelen,fp);
        if(temp!=imagelen)
        {
	        fprintf(stderr, "Cannot read %s!\n", argv[1]);
	        exit(2);
        }

        dumpimage(image, imagelen);
        CreateTempFile(pOutputFileName);   
        printf ("====\n%s was completed\n\n ",pOutputFileName);
        fclose(fp);
        if (SINGLE_CORE_MODE != mode)
        {
            g_pLocation++;
        }       
    }

	exit(0);
}

char Output[5][16] = {"ROM","DSRAM","ISRAM","OTFB","DDR"};
void CreateTempFile(const char* OutFileName)
{
	FILE * pfout ;
	int filelens = 0;
	int type = -1 ;
	int i = 0 ,j = 2;

	pfout = fopen(OutFileName,"w");
	for(j=0;j<5;j++)
	{
		if(Table[j].filesize>0)
		{	
			type = j ;
			fprintf(pfout,"%08x",type);
			fprintf(pfout,"%08x",Table[j].filesize);
            printf("%s length : %d\n",&Output[j][0],Table[j].filesize);
			for(i=0;i<Table[j].filesize/4;i++)
			{
				fprintf(pfout,"%08x",*((U32*)p_base_addr[j] + i) );
			}
		}
//		fprintf(pfout,"\n================================================\n" );
	}
	fclose(pfout);
    
}