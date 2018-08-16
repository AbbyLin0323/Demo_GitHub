/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */

/* Program to dump out headers from an ELF binary to aid in building
   an executable/loader.
   ELF definitions copied from OpenBSD.  Original copyrights follow:
 */

/*  $OpenBSD: exec_elf.h,v 1.10 1996/12/11 05:55:33 imp Exp $   */
/*
 * Copyright (c) 1995, 1996 Erik Theisen.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//#define PRINT

#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include<stdlib.h>
#include<fcntl.h>
#include <time.h>

#pragma warning(disable:4996)

typedef unsigned char    U8;
typedef unsigned int     U32;

U8 zero = 0;


#define  LITTLE_ENDIAN
//#include <sys/types.h>
//#include <sys/stat.h>

/*
 * These typedefs need to be handled better -
 *  doesn't work on 64-bit machines.  Note:
 *  there currently isn't a 64-bit ABI.
    [However, there appears to be ad-hoc support for 64-bits, so this
    file has been carefully hacked to be compiled for elf64.  --Parag]
 */
typedef unsigned int    Elf32_Addr;     /* Unsigned program address */
typedef unsigned int    Elf32_Off;      /* Unsigned file offset */
typedef int             Elf32_Sword;    /* Signed large integer */
typedef unsigned int    Elf32_Word;     /* Unsigned large integer */
typedef unsigned short  Elf32_Half;     /* Unsigned medium integer */

#ifdef INT64
/* 64-bit extras */
typedef unsigned long long  Elf64_Addr; /* Unsigned program address */
typedef unsigned long long  Elf64_Off;  /* Unsigned file offset */
typedef long long       Elf64_Sword;    /* Signed large integer */
typedef unsigned long long  Elf64_Word; /* Unsigned large integer */
typedef Elf64_Addr  Elf_Addr;       /* Unsigned program address */
typedef Elf64_Off   Elf_Off;        /* Unsigned file offset */
typedef Elf64_Word  Elf_Word;       /* Unsigned large integer */
typedef Elf64_Sword Elf_Sword;      /* Unsigned large integer */
#define X "qX"
#define D "qd"
#else
typedef Elf32_Addr  Elf_Addr;       /* Unsigned program address */
typedef Elf32_Off   Elf_Off;        /* Unsigned file offset */
typedef Elf32_Word  Elf_Word;       /* Unsigned large integer */
typedef Elf32_Sword Elf_Sword;      /* Unsigned large integer */
#define X "X"
#define D "d"
#endif
char inputfilefw_core0[0x100000];
char inputfilefw_core12[0x100000];
char inputfile_rom[0x100000];
char outputfilefw_bin[0x100000];
/* e_ident[] identification indexes */
#define EI_MAG0     0       /* file ID */
#define EI_MAG1     1       /* file ID */
#define EI_MAG2     2       /* file ID */
#define EI_MAG3     3       /* file ID */
#define EI_CLASS    4       /* file class */
#define EI_DATA     5       /* data encoding */
#define EI_VERSION  6       /* ELF header version */
#define EI_PAD      7       /* start of pad bytes */
#define EI_NIDENT   16      /* Size of e_ident[] */

/* e_ident[] magic number */
#define ELFMAG0     0x7f        /* e_ident[EI_MAG0] */
#define ELFMAG1     'E'     /* e_ident[EI_MAG1] */
#define ELFMAG2     'L'     /* e_ident[EI_MAG2] */
#define ELFMAG3     'F'     /* e_ident[EI_MAG3] */
#define ELFMAG      "\177ELF"   /* magic */
#define SELFMAG     4       /* size of magic */

/* e_ident[] file class */
#define ELFCLASSNONE    0       /* invalid */
#define ELFCLASS32      1       /* 32-bit objs */
#define ELFCLASS64      2       /* 64-bit objs */
#define ELFCLASSNUM     3       /* number of classes */

/* e_ident[] data encoding */
#define ELFDATANONE 0       /* invalid */
#define ELFDATA2LSB 1       /* Little-Endian */
#define ELFDATA2MSB 2       /* Big-Endian */
#define ELFDATANUM  3       /* number of data encode defines */

/* e_ident */
#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                      (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                      (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                      (ehdr).e_ident[EI_MAG3] == ELFMAG3)

/* ELF Header */
typedef struct elfhdr{
    unsigned char   e_ident[EI_NIDENT]; /* ELF Identification */
    Elf32_Half  e_type;         /* object file type */
    Elf32_Half  e_machine;      /* machine */
    Elf32_Word  e_version;      /* object file version */
    Elf_Addr    e_entry;        /* virtual entry point */
    Elf_Off     e_phoff;        /* program header table offset */
    Elf_Off     e_shoff;        /* section header table offset */
    Elf32_Word  e_flags;        /* processor-specific flags */
    Elf32_Half  e_ehsize;       /* ELF header size */
    Elf32_Half  e_phentsize;    /* program header entry size */
    Elf32_Half  e_phnum;        /* number of program header entries */
    Elf32_Half  e_shentsize;    /* section header entry size */
    Elf32_Half  e_shnum;        /* number of section header entries */
    Elf32_Half  e_shstrndx;     /* section header table's "section 
                                   header string table" entry offset */
} Elf_Ehdr;

/* e_type */
#define ET_NONE     0       /* No file type */
#define ET_REL      1       /* relocatable file */
#define ET_EXEC     2       /* executable file */
#define ET_DYN      3       /* shared object file */
#define ET_CORE     4       /* core file */
#define ET_NUM      5       /* number of types */
#define ET_LOPROC   0xff00      /* reserved range for processor */
#define ET_HIPROC   0xffff      /*  specific e_type */

/* e_machine */
#define EM_NONE     0       /* No Machine */
#define EM_M32      1       /* AT&T WE 32100 */
#define EM_SPARC    2       /* SPARC */
#define EM_386      3       /* Intel 80386 */
#define EM_68K      4       /* Motorola 68000 */
#define EM_88K      5       /* Motorola 88000 */
#define EM_486      6       /* Intel 80486 - unused? */
#define EM_860      7       /* Intel 80860 */
#define EM_MIPS     8       /* MIPS R3000 Big-Endian only */
/* 
 * Don't know if EM_MIPS_RS4_BE,
 * EM_SPARC64, EM_PARISC,
 * or EM_PPC are ABI compliant
 */
#define EM_MIPS_RS4_BE  10      /* MIPS R4000 Big-Endian */
#define EM_SPARC64  11      /* SPARC v9 64-bit unoffical */
#define EM_PARISC   15      /* HPPA */
#define EM_PPC      20      /* PowerPC */
#define EM_NUM      13      /* number of machine types */

/* Version */
#define EV_NONE     0       /* Invalid */
#define EV_CURRENT  1       /* Current */
#define EV_NUM      2       /* number of versions */

/* Section Header */
typedef struct {
    Elf32_Word  sh_name;        /* name - index into section header
                                   string table section */
    Elf32_Word  sh_type;        /* type */
    Elf_Word    sh_flags;       /* flags */
    Elf_Addr    sh_addr;        /* address */
    Elf_Off     sh_offset;      /* file offset */
    Elf_Word    sh_size;        /* section size */
    Elf32_Word  sh_link;        /* section header table index link */
    Elf32_Word  sh_info;        /* extra information */
    Elf_Word    sh_addralign;   /* address alignment */
    Elf_Word    sh_entsize;     /* section entry size */
} Elf_Shdr;

/* Special Section Indexes */
#define SHN_UNDEF   0       /* undefined */
#define SHN_LORESERVE   0xff00      /* lower bounds of reserved indexes */
#define SHN_LOPROC  0xff00      /* reserved range for processor */
#define SHN_HIPROC  0xff1f      /*   specific section indexes */
#define SHN_ABS     0xfff1      /* absolute value */
#define SHN_COMMON  0xfff2      /* common symbol */
#define SHN_HIRESERVE   0xffff      /* upper bounds of reserved indexes */

/* sh_type */
#define SHT_NULL    0       /* inactive */
#define SHT_PROGBITS    1       /* program defined information */
#define SHT_SYMTAB  2       /* symbol table section */
#define SHT_STRTAB  3       /* string table section */
#define SHT_RELA    4       /* relocation section with addends*/
#define SHT_HASH    5       /* symbol hash table section */
#define SHT_DYNAMIC 6       /* dynamic section */
#define SHT_NOTE    7       /* note section */
#define SHT_NOBITS  8       /* no space section */
#define SHT_REL     9       /* relation section without addends */
#define SHT_SHLIB   10      /* reserved - purpose unknown */
#define SHT_DYNSYM  11      /* dynamic symbol table section */
#define SHT_NUM     12      /* number of section types */
#define SHT_LOPROC  0x70000000  /* reserved range for processor */
#define SHT_HIPROC  0x7fffffff  /*  specific section header types */
#define SHT_LOUSER  0x80000000  /* reserved range for application */
#define SHT_HIUSER  0xffffffff  /*  specific indexes */

/* Section names */
#define ELF_BSS         ".bss"      /* uninitialized data */
#define ELF_DATA        ".data"     /* initialized data */
#define ELF_DEBUG       ".debug"    /* debug */
#define ELF_DYNAMIC     ".dynamic"  /* dynamic linking information */
#define ELF_DYNSTR      ".dynstr"   /* dynamic string table */
#define ELF_DYNSYM      ".dynsym"   /* dynamic symbol table */
#define ELF_FINI        ".fini"     /* termination code */
#define ELF_GOT         ".got"      /* global offset table */
#define ELF_HASH        ".hash"     /* symbol hash table */
#define ELF_INIT        ".init"     /* initialization code */
#define ELF_REL_DATA    ".rel.data" /* relocation data */
#define ELF_REL_FINI    ".rel.fini" /* relocation termination code */
#define ELF_REL_INIT    ".rel.init" /* relocation initialization code */
#define ELF_REL_DYN     ".rel.dyn"  /* relocaltion dynamic link info */
#define ELF_REL_RODATA  ".rel.rodata"   /* relocation read-only data */
#define ELF_REL_TEXT    ".rel.text" /* relocation code */
#define ELF_RODATA      ".rodata"   /* read-only data */
#define ELF_SHSTRTAB    ".shstrtab" /* section header string table */
#define ELF_STRTAB      ".strtab"   /* string table */
#define ELF_SYMTAB      ".symtab"   /* symbol table */
#define ELF_TEXT        ".text"     /* code */


/* Section Attribute Flags - sh_flags */
#define SHF_WRITE   0x1     /* Writable */
#define SHF_ALLOC   0x2     /* occupies memory */
#define SHF_EXECINSTR   0x4     /* executable */
#define SHF_MASKPROC    0xf0000000  /* reserved bits for processor */
                    /*  specific section attributes */

/* Symbol Table Entry */
typedef struct elf32_sym {
    Elf32_Word  st_name;    /* name - index into string table */
    Elf32_Addr  st_value;   /* symbol value */
    Elf32_Word  st_size;    /* symbol size */
    unsigned char   st_info;    /* type and binding */
    unsigned char   st_other;   /* 0 - no defined meaning */
    Elf32_Half  st_shndx;   /* section header index */
} Elf32_Sym;

typedef struct elf64_sym {
    Elf32_Word  st_name;        /* name - index into string table */
    unsigned char   st_info;    /* type and binding */
    unsigned char   st_other;   /* 0 - no defined meaning */
    Elf32_Half  st_shndx;       /* section header index */
    Elf_Addr    st_value;       /* symbol value */
    Elf_Word    st_size;        /* symbol size */
} Elf64_Sym;

/* Symbol table index */
#define STN_UNDEF   0       /* undefined */

/* Extract symbol info - st_info */
#define ELF_ST_BIND(x)  ((x) >> 4)
#define ELF_ST_TYPE(x)  (((unsigned int) x) & 0xf)
#define ELF_ST_INFO(b,t)    (((b) << 4) + ((t) & 0xf))

/* Symbol Binding - ELF_ST_BIND - st_info */
#define STB_LOCAL   0       /* Local symbol */
#define STB_GLOBAL  1       /* Global symbol */
#define STB_WEAK    2       /* like global - lower precedence */
#define STB_NUM     3       /* number of symbol bindings */
#define STB_LOPROC  13      /* reserved range for processor */
#define STB_HIPROC  15      /*  specific symbol bindings */

/* Symbol type - ELF_ST_TYPE - st_info */
#define STT_NOTYPE  0       /* not specified */
#define STT_OBJECT  1       /* data object */
#define STT_FUNC    2       /* function */
#define STT_SECTION 3       /* section */
#define STT_FILE    4       /* file */
#define STT_NUM     5       /* number of symbol types */
#define STT_LOPROC  13      /* reserved range for processor */
#define STT_HIPROC  15      /*  specific symbol types */

/* Relocation entry with implicit addend */
typedef struct 
{
    Elf_Addr    r_offset;   /* offset of relocation */
    Elf_Word    r_info;     /* symbol table index and type */
} Elf_Rel;

/* Relocation entry with explicit addend */
typedef struct 
{
    Elf_Addr    r_offset;   /* offset of relocation */
    Elf_Word    r_info;     /* symbol table index and type */
    Elf_Sword   r_addend;
} Elf_Rela;

/* Extract relocation info - r_info */
#define ELF32_R_SYM(i)      ((i) >> 8)
#define ELF32_R_TYPE(i)     ((i) & 0xFF)
#define ELF32_R_INFO(s,t)   (((s) << 8) + ((t) & 0xFF))

/* 64-bit Extract relocation info - r_info */
#define ELF64_R_SYM(i)      ((i) >> 32)
#define ELF64_R_TYPE(i)     ((i) & 0xffffffff)
#define ELF64_R_INFO(s,t)   (((s) << 32) + ((t) & 0xFFFFFFFF))

#ifdef INT64
/* Program Header */
typedef struct {
    Elf32_Word  p_type;     /* segment type */
    Elf32_Word  p_flags;    /* flags */
    Elf_Off     p_offset;   /* segment offset */
    Elf_Addr    p_vaddr;    /* virtual address of segment */
    Elf_Addr    p_paddr;    /* physical address - ignored? */
    Elf_Word    p_filesz;   /* number of bytes in file for seg. */
    Elf_Word    p_memsz;    /* number of bytes in mem. for seg. */
    Elf_Word    p_align;    /* memory alignment */
} Elf_Phdr;
#else
/* Program Header */
typedef struct {
    Elf32_Word  p_type;     /* segment type */
    Elf32_Off   p_offset;   /* segment offset */
    Elf_Addr    p_vaddr;    /* virtual address of segment */
    Elf_Addr    p_paddr;    /* physical address - ignored? */
    Elf_Word    p_filesz;   /* number of bytes in file for seg. */
    Elf_Word    p_memsz;    /* number of bytes in mem. for seg. */
    Elf_Word    p_flags;    /* flags */
    Elf_Word    p_align;    /* memory alignment */
} Elf_Phdr;
#endif

/* Segment types - p_type */
#define PT_NULL     0       /* unused */
#define PT_LOAD     1       /* loadable segment */
#define PT_DYNAMIC  2       /* dynamic linking section */
#define PT_INTERP   3       /* the RTLD */
#define PT_NOTE     4       /* auxiliary information */
#define PT_SHLIB    5       /* reserved - purpose undefined */
#define PT_PHDR     6       /* program header */
#define PT_NUM      7       /* Number of segment types */
#define PT_LOPROC   0x70000000  /* reserved range for processor */
#define PT_HIPROC   0x7fffffff  /*  specific segment types */

/* Segment flags - p_flags */
#define PF_X        0x1     /* Executable */
#define PF_W        0x2     /* Writable */
#define PF_R        0x4     /* Readable */
#define PF_MASKPROC 0xf0000000  /* reserved bits for processor */
                    /*  specific segment flags */

/* Dynamic structure */
typedef struct 
{
    Elf_Sword   d_tag;      /* controls meaning of d_val */
    union 
    {
        Elf_Word    d_val;  /* Multiple meanings - see d_tag */
        Elf_Addr    d_ptr;  /* program virtual address */
    } d_un;
} Elf_Dyn;

extern Elf_Dyn  _DYNAMIC[];

/* Dynamic Array Tags - d_tag */
#define DT_NULL     0       /* marks end of _DYNAMIC array */
#define DT_NEEDED   1       /* string table offset of needed lib */
#define DT_PLTRELSZ 2       /* size of relocation entries in PLT */
#define DT_PLTGOT   3       /* address PLT/GOT */
#define DT_HASH     4       /* address of symbol hash table */
#define DT_STRTAB   5       /* address of string table */
#define DT_SYMTAB   6       /* address of symbol table */
#define DT_RELA     7       /* address of relocation table */
#define DT_RELASZ   8       /* size of relocation table */
#define DT_RELAENT  9       /* size of relocation entry */
#define DT_STRSZ    10      /* size of string table */
#define DT_SYMENT   11      /* size of symbol table entry */
#define DT_INIT     12      /* address of initialization func. */
#define DT_FINI     13      /* address of termination function */
#define DT_SONAME   14      /* string table offset of shared obj */
#define DT_RPATH    15      /* string table offset of library
                       search path */
#define DT_SYMBOLIC 16      /* start sym search in shared obj. */
#define DT_REL      17      /* address of rel. tbl. w addends */
#define DT_RELSZ    18      /* size of DT_REL relocation table */
#define DT_RELENT   19      /* size of DT_REL relocation entry */
#define DT_PLTREL   20      /* PLT referenced relocation entry */
#define DT_DEBUG    21      /* bugger */
#define DT_TEXTREL  22      /* Allow rel. mod. to unwritable seg */
#define DT_JMPREL   23      /* add. of PLT's relocation entries */
#define DT_NUM      24      /* Number used. */
#define DT_LOPROC   0x70000000  /* reserved range for processor */
#define DT_HIPROC   0x7fffffff  /*  specific dynamic array tags */

/* Standard ELF hashing function */
unsigned long elf_hash(const unsigned char *name);

#define ELF_TARG_VER    1   /* The ver for which this code is intended */


static char *strtab;

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

int is64 = 0;
int do_swap = 0;

static void
dump_header(Elf_Ehdr *hdr)
{
    int i;

    //printf("ELF header:\n");
    //printf("ident=0x");

    for (i = 0; i < EI_NIDENT; i++)
        //printf("%2X.", hdr->e_ident[i]);

    //printf("\n");

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

#if 0
    printf("type=0x%X machine=0x%X version=0x%X entry=0x%" X "\n",
            hdr->e_type, hdr->e_machine, hdr->e_version, hdr->e_entry);
    printf("phoff=0x%" X " shoff=0x%" X " flags=0x%X ehsize=0x%X\n",
            hdr->e_phoff, hdr->e_shoff, hdr->e_flags, hdr->e_ehsize);
    printf(
        "phentsize=0x%X phunm=0x%X shentsize=0x%X shnum=0x%X shstrndx=0x%X\n",
            hdr->e_phentsize, hdr->e_phnum, hdr->e_shentsize,
            hdr->e_shnum, hdr->e_shstrndx);
#endif
}

static void
dump_program_header(Elf_Phdr *p, Elf_Word i)
{
    
    
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

typedef enum __FILE_TYPE
{
    ELF_MCU0,
    ELF_MCU12,
    ELF_MCU2,
    ROM_FILE,
    VER_INFO,
    BOOT_ROM,
}FILE_TYPE;

typedef struct __FW_SEC_DES_ELE
{
    U32 m_sec_offset; // the section offset to the base of the fw bin file
    U32 m_sec_len; // the section len
    U32 m_sec_phy_addr_off; // this section offset to the real start position of the fw in DRAM
    U32 m_b_duplite_section_in_bin;

    //these fields need not copy to bin
    struct __FW_SEC_DES_ELE *m_next;
    FILE_TYPE file_type;
    U32 m_sec_off_to_input_file;
}FW_SEC_DES_ELE;

typedef struct __FW_FILE_HDR
{
    U32 m_bin_total_len; // The total length of the fw bin file
    U32 m_sec_des_off; //The file offset of the section description
}FW_FILE_HDR;

typedef struct __FW_HDR_INFO
{
    FW_FILE_HDR m_fw_file_hdr;

    FW_SEC_DES_ELE *m_sec_des_list;
    U32 m_sec_des_ele_cnt;

}FW_HDR_INFO;

FW_HDR_INFO g_fw_hdr_info = { { 0 }, NULL, 0 };
FW_SEC_DES_ELE *gp_cur_fw_sec_des_ele = NULL;
U32 g_file_offset = 0;

//DRAM_BOOTLOADER_LLF_START_ADDR
//DRAM_PARAM_TABLE_START_ADDR
//DRAM_HEAD_MCU0_START_ADDR
//DRAM_DATA_TEXT_MCU0_START_ADDR
//DRAM_RODATA_MCU12_SHARE_START_ADDR
//DRAM_TEXT_MCU12_SHARE_START_ADDR
//DSRAM0_MCU0_START_ADDR
//DSRAM0_MCU1_START_ADDR
//DSRAM0_MCU2_START_ADDR
//ISRAM_MCU0_START_ADDR
//ISRAM_MCU12_SHARE_START_ADDR
//OPTION_ROM
//DSRAM1_START_ADDR

#define DRAM_BASE 0x40000000

//#define DRAM_BOOTLOADER_LLF_START_ADDR (DRAM_BASE+256*1024)
//#define DRAM_BOOTLOADER_LLF_LEN 16*1024
//#define DRAM_BOOTLOADER_LLF_BIN_OFFSET 0//(DRAM_STATIC_PARAM_BIN_OFFSET+DRAM_STATIC_PARAM_LEN)

//#define DRAM_PARAM_TABLE_START_ADDR (DRAM_BOOTLOADER_LLF_START_ADDR + DRAM_BOOTLOADER_LLF_LEN)
//#define DRAM_PARAM_TABLE_LEN 16*1024
//#define DRAM_PARAM_TABLE_BIN_OFFSET (DRAM_BOOTLOADER_LLF_BIN_OFFSET+DRAM_BOOTLOADER_LLF_LEN)

//#define DRAM_HEAD_MCU0_START_ADDR (DRAM_PARAM_TABLE_START_ADDR+DRAM_PARAM_TABLE_LEN)
#define DRAM_HEAD_MCU0_START_ADDR   (DRAM_BASE+288*1024)
#define DRAM_HEAD_MCU0_LEN (16*1024)
#define DRAM_HEAD_MCU0_BIN_OFFSET 0//(DRAM_PARAM_TABLE_BIN_OFFSET+DRAM_PARAM_TABLE_LEN)

//DRAM_DATA_TEXT_MCU0 128*1024
#define DRAM_DATA_TEXT_MCU0_START_ADDR (DRAM_HEAD_MCU0_START_ADDR+DRAM_HEAD_MCU0_LEN)
#define DRAM_DATA_TEXT_MCU0_LEN (128*1024)
#define DRAM_DATA_TEXT_MCU0_BIN_OFFSET (DRAM_HEAD_MCU0_BIN_OFFSET+DRAM_HEAD_MCU0_LEN)


//DRAM_RODATA_MCU12_SHARE 24*1024
#define DRAM_RODATA_MCU12_SHARE_START_ADDR (DRAM_DATA_TEXT_MCU0_START_ADDR+DRAM_DATA_TEXT_MCU0_LEN)
#define DRAM_RODATA_MCU12_SHARE_LEN (24*1024)
#define DRAM_RODATA_MCU12_SHARE_BIN_OFFSET (DRAM_DATA_TEXT_MCU0_BIN_OFFSET+DRAM_DATA_TEXT_MCU0_LEN)

//DRAM_RODATA_MCU2_SHARE 8*1024
#define DRAM_RODATA_MCU2_SHARE_START_ADDR (DRAM_RODATA_MCU12_SHARE_START_ADDR+DRAM_RODATA_MCU12_SHARE_LEN)
#define DRAM_RODATA_MCU2_SHARE_LEN (8*1024)
#define DRAM_RODATA_MCU2_SHARE_BIN_OFFSET (DRAM_RODATA_MCU12_SHARE_BIN_OFFSET+DRAM_RODATA_MCU12_SHARE_LEN)


//DRAM_TEXT_MCU12_SHARE 160*1024
#define DRAM_TEXT_MCU12_SHARE_START_ADDR (DRAM_RODATA_MCU2_SHARE_START_ADDR+DRAM_RODATA_MCU2_SHARE_LEN)
#define DRAM_TEXT_MCU12_SHARE_LEN (160*1024)
#define DRAM_TEXT_MCU12_SHARE_BIN_OFFSET (DRAM_RODATA_MCU2_SHARE_BIN_OFFSET+DRAM_RODATA_MCU2_SHARE_LEN)

//DRAM_TEXT_MCU2_SHARE 96*1024
#define DRAM_TEXT_MCU2_SHARE_START_ADDR (DRAM_TEXT_MCU12_SHARE_START_ADDR+DRAM_TEXT_MCU12_SHARE_LEN)
#define DRAM_TEXT_MCU2_SHARE_LEN (96*1024)
#define DRAM_TEXT_MCU2_SHARE_BIN_OFFSET (DRAM_TEXT_MCU12_SHARE_BIN_OFFSET+DRAM_TEXT_MCU12_SHARE_LEN)

//DSRAM0_MCU0 16*1024
#define DSRAM0_MCU0_START_ADDR 0x1ffa4000
#define DSRAM0_MCU0_LEN (16*1024)
#define DSRAM0_MCU0_BIN_OFFSET (DRAM_TEXT_MCU2_SHARE_BIN_OFFSET+DRAM_TEXT_MCU2_SHARE_LEN)

//DSRAM0_MCU1 32*1024
#define DSRAM0_MCU1_START_ADDR 0x1ffa0000
#define DSRAM0_MCU1_LEN (32*1024)
#define DSRAM0_MCU1_BIN_OFFSET (DSRAM0_MCU0_BIN_OFFSET+DSRAM0_MCU0_LEN)

//DSRAM0_MCU1 32*1024
#define DSRAM0_MCU2_START_ADDR 0x1ffa0000
#define DSRAM0_MCU2_LEN (32*1024)
#define DSRAM0_MCU2_BIN_OFFSET (DSRAM0_MCU1_BIN_OFFSET+DSRAM0_MCU1_LEN)

//DSRAM0_MCU2 32*1024
//#define DSRAM0_MCU2_START_ADDR DSRAM0_MCU1_START_ADDR+DSRAM0_MCU1_LEN//0x1ffa0000
//#define DSRAM0_MCU2_LEN 32*1024
//#define DSRAM0_MCU2_BIN_OFFSET (DSRAM0_MCU1_BIN_OFFSET+DSRAM0_MCU1_LEN)

#if 0
//DSRAM0_MCU1 32*1024 for mcu12
#define DSRAM0_MCU1_CORE12_START_ADDR 0x1ffa0000
#define DSRAM0_MCU1_CORE12_LEN 2*32*1024
#define DSRAM0_MCU1_CORE12_BIN_OFFSET (DSRAM0_MCU0_BIN_OFFSET+DSRAM0_MCU0_LEN)


//DSRAM0_MCU2 32*1024 for mcu12
#define DSRAM0_MCU2_CORE12_START_ADDR 0x1ffa0000
#define DSRAM0_MCU2_CORE12_LEN 32*1024
#define DSRAM0_MCU2_CORE12_BIN_OFFSET (DSRAM0_MCU1_BIN_OFFSET+DSRAM0_MCU1_LEN)
#endif

//ISRAM_MCU0 40*1024
#define ISRAM_MCU0_START_ADDR 0x20000000//note for mcu0
#define ISRAM_MCU0_LEN (40*1024)
#define ISRAM_MCU0_BIN_OFFSET (DSRAM0_MCU2_BIN_OFFSET+DSRAM0_MCU2_LEN)

//ISRAM_MCU12_SHARE 128*1024
#define ISRAM_MCU12_SHARE_START_ADDR 0x20000000//note for mcu12
#define ISRAM_MCU12_SHARE_LEN (128*1024)
#define ISRAM_MCU12_SHARE_BIN_OFFSET (ISRAM_MCU0_BIN_OFFSET+ISRAM_MCU0_LEN)

//ISRAM_MCU2_SHARE 64*1024
#define ISRAM_MCU2_SHARE_START_ADDR 0x20000000//note for mcu12
#define ISRAM_MCU2_SHARE_LEN (64*1024)
#define ISRAM_MCU2_SHARE_BIN_OFFSET (ISRAM_MCU12_SHARE_BIN_OFFSET+ISRAM_MCU12_SHARE_LEN)

#define OPTION_ROM_LEN (64*1024)
#define OPTION_ROM_BIN_OFFSET (ISRAM_MCU2_SHARE_BIN_OFFSET + ISRAM_MCU2_SHARE_LEN)

#if 0
#define DSRAM1_START_ADDR 0x1ffc8000//note for mcu12
#define DSRAM1_LEN 96*1024
#define DSRAM1_BIN_OFFSET (OPTION_ROM_BIN_OFFSET+OPTION_ROM_LEN)
#endif

#define FW_BIN_SIZE (DRAM_HEAD_MCU0_LEN + \
DRAM_DATA_TEXT_MCU0_LEN + \
DRAM_RODATA_MCU12_SHARE_LEN + \
DRAM_TEXT_MCU12_SHARE_LEN + \
DSRAM0_MCU0_LEN + \
(DSRAM0_MCU1_LEN*2) + \
ISRAM_MCU0_LEN+ \
ISRAM_MCU12_SHARE_LEN+ \
OPTION_ROM_LEN)
/*
DSRAM1_SUSPEND(DSRAM1_MCU1
            DSRAM1_MCU01_SHARE
            DSRAM1_MCU2
            DSRAM1_MCU02_SHARE
            DSRAM1_MCU012_SHARE) 96*1024
*/

#define OTFB_BASE 0xfff00000

#define OTFB_SIGNATURE_START_ADDR (OTFB_BASE)
#define OTFB_SIGNATURE_LEN 0x8
#define OTFB_SIGNATURE_BIN_OFFSET 0

#define OTFB_ENTRY_START_ADDR (OTFB_SIGNATURE_START_ADDR + OTFB_SIGNATURE_LEN)
#define OTFB_ENTRY_LEN 0x8
#define OTFB_ENTRY_BIN_OFFSET (OTFB_SIGNATURE_BIN_OFFSET + OTFB_SIGNATURE_LEN)

#define OTFB_TEXT_START_ADDR (OTFB_ENTRY_START_ADDR + OTFB_ENTRY_LEN)
#define OTFB_TEXT_LEN 0x2ff0
#define OTFB_TEXT_BIN_OFFSET (OTFB_ENTRY_BIN_OFFSET + OTFB_ENTRY_LEN)

#define OTFB_FTABLE_HEAD_START_ADDR (OTFB_TEXT_START_ADDR + OTFB_TEXT_LEN)
#define OTFB_FTABLE_HEAD_LEN 0x10
#define OTFB_FTABLE_HEAD_BIN_OFFSET (OTFB_TEXT_BIN_OFFSET + OTFB_TEXT_LEN)

#define OTFB_FTABLE_FUNCENTRY_START_ADDR (OTFB_FTABLE_HEAD_START_ADDR + OTFB_FTABLE_HEAD_LEN)
#define OTFB_FTABLE_FUNCENTRY_LEN 0xf0
#define OTFB_FTABLE_FUNCENTRY_BIN_OFFSET (OTFB_FTABLE_HEAD_BIN_OFFSET + OTFB_FTABLE_HEAD_LEN)

#define OTFB_PTABLE_START_ADDR (OTFB_FTABLE_FUNCENTRY_START_ADDR + OTFB_FTABLE_FUNCENTRY_LEN)
#define OTFB_PTABLE_LEN 0x200
#define OTFB_PTABLE_BIN_OFFSET (OTFB_FTABLE_FUNCENTRY_BIN_OFFSET + OTFB_FTABLE_FUNCENTRY_LEN)

#define OTFB_REGLIST_START_ADDR (OTFB_PTABLE_START_ADDR + OTFB_PTABLE_LEN)
#define OTFB_REGLIST_LEN 0xd00
#define OTFB_REGLIST_BIN_OFFSET (OTFB_PTABLE_BIN_OFFSET + OTFB_PTABLE_LEN)

#define DRAM_BOOTLOADER_TEXT_START_ADDR 0x40044000
#define DRAM_BOOTLOADER_TEXT_LEN 0x4000
#define DRAM_BOOTLOADER_TEX_BIN_OFFSET (OTFB_REGLIST_BIN_OFFSET + OTFB_REGLIST_LEN)

#define BL_BIN_SIZE (OTFB_SIGNATURE_LEN + \
OTFB_ENTRY_LEN +\
OTFB_TEXT_LEN +\
OTFB_FTABLE_HEAD_LEN +\
OTFB_FTABLE_FUNCENTRY_LEN +\
OTFB_PTABLE_LEN +\
OTFB_REGLIST_LEN +\
DRAM_BOOTLOADER_TEXT_LEN)

#define FW_PARAM_NUM        5
#define BL_PARAM_NUM        4
#define BR_PARAM_NUM        3
#define VERSION_INFO_LEN    (4*8)   /* 4DW string, exclude date&time */
//#define FW_VERSION_OFFSET   (12 * 1024)
#define FW_VERSION_OFFSET   (32)
#define BL_VERSION_OFFSET   (12 * 1024 + 256)

#define VERSION_TOTAL_LEN_DW 6
#define VERSION_TOTAL_LEN (VERSION_TOTAL_LEN_DW*4)
#define PG_SIZE_PER_PLN (16*1024)

U32 g_fw_total_len_location = FW_VERSION_OFFSET + VERSION_TOTAL_LEN;// 32+ VERSION_TOTAL_LEN
U32 g_fw_unpack_hdr_offset = 0x50;
U32 g_fw_compact_start_addr = 12*1024;
U8 g_fw_total_len_has_buf = true;

inline U32 up_bound_dw_align(U32 value)
{
    if ( (value & 0x3) == 0)
    {
        return value;
    }
    else
    {
        return ((value >> 2) << 2) + 4;
    }
}

void add_fw_sec_des_ele(FILE_TYPE type, U32 offset_to_input, U32 phy_offset, U32 sec_len, U8 b_duplite_section_in_bin)
{
    FW_SEC_DES_ELE *lp_fw_sec_des_ele_tmp = new FW_SEC_DES_ELE;
#if 0
    U32 sec_len_tmp;

    //reserve sizeof(FW_FILE_HDR) for storing fw bin file length info
    if ((false == g_fw_total_len_has_buf) && g_file_offset + sec_len + sizeof(FW_FILE_HDR) > PG_SIZE_PER_PLN)
    {
        sec_len_tmp = PG_SIZE_PER_PLN - g_file_offset - sizeof(FW_FILE_HDR);
        add_fw_sec_des_ele(type, offset_to_input, phy_offset, sec_len_tmp);

        offset_to_input += sec_len_tmp;
        phy_offset += sec_len_tmp;
        sec_len -= sec_len_tmp;


    }
#endif

    lp_fw_sec_des_ele_tmp->file_type = type;
    lp_fw_sec_des_ele_tmp->m_sec_off_to_input_file = offset_to_input;
    lp_fw_sec_des_ele_tmp->m_next = NULL;
    lp_fw_sec_des_ele_tmp->m_sec_len = sec_len;
    //lp_fw_sec_des_ele_tmp->m_sec_offset = g_file_offset;
    lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off = phy_offset;
    lp_fw_sec_des_ele_tmp->m_b_duplite_section_in_bin = b_duplite_section_in_bin;
    if (NULL == g_fw_hdr_info.m_sec_des_list)
    {
        g_fw_hdr_info.m_sec_des_list = lp_fw_sec_des_ele_tmp;
        gp_cur_fw_sec_des_ele = lp_fw_sec_des_ele_tmp;
    }
    else
    {
        gp_cur_fw_sec_des_ele->m_next = lp_fw_sec_des_ele_tmp;
        gp_cur_fw_sec_des_ele = lp_fw_sec_des_ele_tmp;
    }

    g_fw_hdr_info.m_sec_des_ele_cnt++;
    if ((b_duplite_section_in_bin == false) && (phy_offset>=g_fw_compact_start_addr))
    {
        //g_file_offset += sec_len; // duplicate section does not consume space in bin file
        g_file_offset += up_bound_dw_align(sec_len); // duplicate section does not consume space in bin file
    }
    

#if 0
    if ((false == g_fw_total_len_has_buf) && g_file_offset + sizeof(FW_FILE_HDR) == PG_SIZE_PER_PLN)
    {
        g_fw_total_len_location = PG_SIZE_PER_PLN - sizeof(FW_FILE_HDR);
        g_fw_total_len_has_buf = true;
        g_file_offset += sizeof(FW_FILE_HDR);
    }
#endif
}

static void dump_rom_file(char *p_rombuf, char * outputbuf, int romfilelen)
{
    int BinOffset;

    BinOffset = OPTION_ROM_BIN_OFFSET;
    //memcpy(outputbuf + BinOffset, p_rombuf, romfilelen);
    //memcpy(outputbuf + g_file_offset, p_rombuf, romfilelen);

    add_fw_sec_des_ele(ROM_FILE, 0, BinOffset, romfilelen,false);

    return;
}

/* identical to FW memory map */
#define DRAM_DSRAM0_MCU0_BASE           (0x400b4000)
#define DRAM_DSRAM0_MCU1_BASE           (0x400b8000)
#define DRAM_DSRAM0_MCU2_BASE           (0x400c0000)
#define DRAM_ISRAM_MCU0_BASE            (0x400c8000)
#define DRAM_ISRAM_MCU1_BASE            (0x400d2000)
#define DRAM_ISRAM_MCU2_BASE            (0x400f2000)

#define HEAD_BEGIN (DRAM_BASE + 0x400000)
#define HEAD_END (HEAD_BEGIN+ 12*1024)

static void
dump_fw_elf(char *p_fw_elf,char * outputbuf,int elf_num)
{
    Elf_Ehdr *hdr;
    bool valid_sec = false;
    U32 phy_offset,i,sec;
    Elf_Phdr *proghdr;
    hdr = (Elf_Ehdr *)p_fw_elf;
    //#ifdef PRINT
    dump_header(hdr);
    //#endif
    proghdr = (Elf_Phdr*)((U32)p_fw_elf + (U32)hdr->e_phoff);
    sec = 0;
    for (i = 0; i < hdr->e_phnum; i++)
    {
        //printf("sec:0x%x     p_offset:0x%x             p_filesz:0x%x             p_memsz:0x%x             p_paddr:0x%x\n",sec,proghdr->p_offset,proghdr->p_filesz,proghdr->p_memsz,proghdr->p_paddr);
        proghdr = proghdr++;
        sec++;
    }

    /*printf("DRAM_BOOTLOADER_LLF_START_ADDR:0x%x DRAM_BOOTLOADER_LLF_BIN_OFFSET:0x%x\n",DRAM_BOOTLOADER_LLF_START_ADDR,DRAM_BOOTLOADER_LLF_BIN_OFFSET);
    printf("DRAM_PARAM_TABLE_START_ADDR:0x%x DRAM_PARAM_TABLE_BIN_OFFSET:0x%x\n",DRAM_PARAM_TABLE_START_ADDR,DRAM_PARAM_TABLE_BIN_OFFSET);
    printf("DRAM_HEAD_MCU0_START_ADDR:0x%x DRAM_HEAD_MCU0_BIN_OFFSET:0x%x\n",DRAM_HEAD_MCU0_START_ADDR,DRAM_HEAD_MCU0_BIN_OFFSET);

    printf("DRAM_DATA_TEXT_MCU0_START_ADDR:0x%x DRAM_DATA_TEXT_MCU0_BIN_OFFSET:0x%x\n",DRAM_DATA_TEXT_MCU0_START_ADDR,DRAM_DATA_TEXT_MCU0_BIN_OFFSET);
    printf("DRAM_RODATA_MCU12_SHARE_START_ADDR:0x%x DRAM_RODATA_MCU12_SHARE_BIN_OFFSET:0x%x\n",DRAM_RODATA_MCU12_SHARE_START_ADDR,DRAM_RODATA_MCU12_SHARE_BIN_OFFSET);
    printf("DRAM_TEXT_MCU12_SHARE_START_ADDR:0x%x DRAM_TEXT_MCU12_SHARE_BIN_OFFSET:0x%x\n",DRAM_TEXT_MCU12_SHARE_START_ADDR,DRAM_TEXT_MCU12_SHARE_BIN_OFFSET);
    printf("DSRAM0_MCU0_START_ADDR:0x%x DSRAM0_MCU0_BIN_OFFSET:0x%x\n",DSRAM0_MCU0_START_ADDR,DSRAM0_MCU0_BIN_OFFSET);
    printf("DSRAM0_MCU1_START_ADDR:0x%x DSRAM0_MCU1_BIN_OFFSET:0x%x\n",DSRAM0_MCU1_START_ADDR,DSRAM0_MCU1_BIN_OFFSET);
    printf("ISRAM_MCU0_START_ADDR:0x%x ISRAM_MCU0_BIN_OFFSET:0x%x\n",ISRAM_MCU0_START_ADDR,ISRAM_MCU0_BIN_OFFSET);*/
    
    phy_offset = 0;
    proghdr = (Elf_Phdr*)(p_fw_elf + hdr->e_phoff);
	for (i = 0; i < hdr->e_phnum; i++)
	{
			/* Now MCU0 head enter this case */
        if ((proghdr->p_paddr >= HEAD_BEGIN) && (proghdr->p_paddr < HEAD_END))
			{
				valid_sec = false;
				memcpy(outputbuf + proghdr->p_paddr - HEAD_BEGIN, p_fw_elf + proghdr->p_offset, proghdr->p_filesz);
				printf("======0x%x %x %x\n", proghdr->p_paddr, proghdr->p_offset,proghdr->p_filesz);
			}
            else
            {
                phy_offset = proghdr->p_paddr;                
                valid_sec = true;

                /*DSRAM*/
                {
                    if (elf_num == 0)
                    {
                        //DSRAM0_MCU0_START_ADDR
                        /*MCU0 DSRAM0 enter this case */
                        if ((proghdr->p_paddr >= DSRAM0_MCU0_START_ADDR) && ((proghdr->p_paddr < (DSRAM0_MCU0_START_ADDR + DSRAM0_MCU0_LEN))))
                        {
                            phy_offset = DRAM_DSRAM0_MCU0_BASE + (proghdr->p_paddr - DSRAM0_MCU0_START_ADDR);                            
                            valid_sec = true;
                        }
                    }
                    if (elf_num == 1)
                    {
                        //DSRAM0_MCU1_START_ADDR
                        if ((proghdr->p_paddr >= DSRAM0_MCU1_START_ADDR) && ((proghdr->p_paddr < (DSRAM0_MCU1_START_ADDR + DSRAM0_MCU1_LEN))))
                        {
                            phy_offset = DRAM_DSRAM0_MCU1_BASE + (proghdr->p_paddr - DSRAM0_MCU1_START_ADDR);                            
                            valid_sec = true;
                        }
                    }

                    if (elf_num == 2)
                    {
                        //DSRAM0_MCU1_START_ADDR
                        if ((proghdr->p_paddr >= DSRAM0_MCU2_START_ADDR) && ((proghdr->p_paddr < (DSRAM0_MCU2_START_ADDR + DSRAM0_MCU2_LEN))))
                        {
                            phy_offset = DRAM_DSRAM0_MCU2_BASE + (proghdr->p_paddr - DSRAM0_MCU2_START_ADDR);                            
                            valid_sec = true;
                        }
                    }

                }

                /* ISRAM */
                {
                    if (elf_num == 0)
                    {
                        //printf("ISRAM_MCU0_BIN_OFFSET:0x%x\n",ISRAM_MCU0_BIN_OFFSET);
                        //ISRAM_MCU0_START_ADDR
                        if ((proghdr->p_paddr >= ISRAM_MCU0_START_ADDR) && ((proghdr->p_paddr < (ISRAM_MCU0_START_ADDR + ISRAM_MCU0_LEN))))
                        {
                            phy_offset = DRAM_ISRAM_MCU0_BASE + (proghdr->p_paddr - ISRAM_MCU0_START_ADDR);                            
                            valid_sec = true;
                        }
                    }
                    if (elf_num == 1)
                    {
                        //ISRAM_MCU12_SHARE_START_ADDR
                        if ((proghdr->p_paddr >= ISRAM_MCU12_SHARE_START_ADDR) && ((proghdr->p_paddr < (ISRAM_MCU12_SHARE_START_ADDR + ISRAM_MCU12_SHARE_LEN))))
                        {
                            phy_offset = DRAM_ISRAM_MCU1_BASE + (proghdr->p_paddr - ISRAM_MCU12_SHARE_START_ADDR);                            
                            valid_sec = true;
                        }
                    }
                    if (elf_num == 2)
                    {
                        //ISRAM_MCU2_SHARE_START_ADDR
                        if ((proghdr->p_paddr >= ISRAM_MCU2_SHARE_START_ADDR) && ((proghdr->p_paddr < (ISRAM_MCU2_SHARE_START_ADDR + ISRAM_MCU2_SHARE_LEN))))
                        {
                            phy_offset = DRAM_ISRAM_MCU2_BASE + (proghdr->p_paddr - ISRAM_MCU2_SHARE_START_ADDR);                            
                            valid_sec = true;
                        }
                    }
                }
            }

			
#if 0
			//DSRAM1_START_ADDR
			if((proghdr->p_paddr>=DSRAM1_START_ADDR)&&((proghdr->p_paddr<(DSRAM1_START_ADDR+DSRAM1_LEN))))
			{
				offset = DSRAM1_BIN_OFFSET+(proghdr->p_paddr-DSRAM1_START_ADDR);
			}
#endif
			/*
			if(offset!=0xffffffff)
				memcpy(outputbuf+offset,p_fw_elf+proghdr->p_offset,proghdr->p_filesz);
			offset = 0xffffffff;
			*/
			if (true == valid_sec && proghdr->p_filesz !=0 )
			{
				//memcpy(outputbuf + g_file_offset, p_fw_elf + proghdr->p_offset, proghdr->p_filesz);

				/* todo : update g_fw_hdr_info */
				//g_fw_hdr_info.m_fw_file_hdr.m_bin_total_len += proghdr->p_filesz;
				add_fw_sec_des_ele((FILE_TYPE)elf_num, proghdr->p_offset, phy_offset, proghdr->p_filesz, false);
			}
			valid_sec = false;

			proghdr++;
			//I_SRAM_SEG_SIZE
	}

	return;
}

void CreateFwBinFile(char * p_buff,char * out_bin_name,U32 cnt)
{
    HANDLE pfile_outbin;
    int haswrite_dram = 0;

    //open  or create dram file
    pfile_outbin = CreateFile(out_bin_name,
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
                        NULL);

    if(pfile_outbin == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "hello", GetLastError());
        while(1)
            ;
    }

    //wrtie data to dram file

    WriteFile(pfile_outbin,(LPDWORD)p_buff,cnt,(LPDWORD)&haswrite_dram,NULL);

    CloseHandle(pfile_outbin);  
}

static void GetFileInform(HANDLE * pHandle, char * pFileName, char * * pLocalBuff, int * pFileLen)
{
    int breadlength;

    *pHandle =  CreateFile(pFileName,//"dram_vec",//"","rom","ramdisk", inst_ram_vec
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL, 
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if(*pHandle == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "hello", GetLastError());
        while(1)
            ;
    }
    
    *pFileLen = GetFileSize(*pHandle, NULL);//1M file
    *pLocalBuff = (char *)malloc(*pFileLen);

    if (*pLocalBuff == NULL)
    {
        fprintf(stderr, "Cannot allocate space for %s!\n",pFileName);
        exit(3);
    }

    if (!ReadFile(*pHandle,(LPVOID)*pLocalBuff, *pFileLen,(LPDWORD)&breadlength,NULL))
    {
        fprintf(stderr, "Cannot read %s!\n", pFileName);
        exit(2);

    }

    return;
}

void SetFWBinCheckCode(int * p_buf)
{
    int index;
    int checkCode;

    checkCode = 0;
    for (index = 1; index < (FW_BIN_SIZE>>2); index++)
    {
        checkCode ^= p_buf[index];
    }

    p_buf[0] = checkCode;
    return;
}


void usage(const char *filename)
{
    printf("input parameter error, usage:\n");
    //printf("1. %s FW [mcu0elf] [mcu12elf] [optionrom] [version-string(4DW+)] [output-filename.bin]\n");
    printf("1. %s FW  MCU0     [mcu0elf]  [version-string(4DW+)] [output-filename.bin]\n",filename);
    printf("2. %s BL [bootloaderelf] [version-string(4DW+)] [output-filename.bin]\n",filename);
    printf("3. %s BR [romelf] [output-filename.bin]",filename);
    getchar();
}


static void
dump_bootloader_elf(char *p_elf,char * outputbuf)
{
    Elf_Ehdr *hdr;
    U32 offset,i;
    Elf_Phdr *proghdr;
    hdr = (Elf_Ehdr *)p_elf;
    //#ifdef PRINT
    dump_header(hdr);
    //#endif
    proghdr = (Elf_Phdr*)((U32)p_elf + (U32)hdr->e_phoff);
    
    for (i = 0; i < hdr->e_phnum; i++)
    {
        if (proghdr->p_paddr < DRAM_BOOTLOADER_TEXT_START_ADDR)
        {
            proghdr++;
            continue;
        }
        
        if((proghdr->p_paddr>=OTFB_SIGNATURE_START_ADDR)&&(proghdr->p_paddr<(OTFB_SIGNATURE_START_ADDR+OTFB_SIGNATURE_LEN)))
        {
            offset = OTFB_SIGNATURE_BIN_OFFSET+(proghdr->p_paddr-OTFB_SIGNATURE_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_ENTRY_START_ADDR)&&(proghdr->p_paddr<(OTFB_ENTRY_START_ADDR+OTFB_ENTRY_LEN)))
        {
            offset = OTFB_ENTRY_BIN_OFFSET+(proghdr->p_paddr-OTFB_ENTRY_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_TEXT_START_ADDR)&&(proghdr->p_paddr<(OTFB_TEXT_START_ADDR+OTFB_TEXT_LEN)))
        {
            offset = OTFB_TEXT_BIN_OFFSET+(proghdr->p_paddr-OTFB_TEXT_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_FTABLE_HEAD_START_ADDR)&&(proghdr->p_paddr<(OTFB_FTABLE_HEAD_START_ADDR+OTFB_FTABLE_HEAD_LEN)))
        {
            offset = OTFB_FTABLE_HEAD_BIN_OFFSET+(proghdr->p_paddr-OTFB_FTABLE_HEAD_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_FTABLE_FUNCENTRY_START_ADDR)&&(proghdr->p_paddr<(OTFB_FTABLE_FUNCENTRY_START_ADDR+OTFB_FTABLE_FUNCENTRY_LEN)))
        {
            offset = OTFB_FTABLE_FUNCENTRY_BIN_OFFSET+(proghdr->p_paddr-OTFB_FTABLE_FUNCENTRY_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_PTABLE_START_ADDR)&&(proghdr->p_paddr<(OTFB_PTABLE_START_ADDR+OTFB_PTABLE_LEN)))
        {
            offset = OTFB_PTABLE_BIN_OFFSET+(proghdr->p_paddr-OTFB_PTABLE_START_ADDR);
        }
        else if((proghdr->p_paddr>=OTFB_REGLIST_START_ADDR)&&(proghdr->p_paddr<(OTFB_REGLIST_START_ADDR+OTFB_REGLIST_LEN)))
        {
            offset = OTFB_REGLIST_BIN_OFFSET+(proghdr->p_paddr-OTFB_REGLIST_START_ADDR);
        }
        else if ((proghdr->p_paddr>=DRAM_BOOTLOADER_TEXT_START_ADDR) && (proghdr->p_paddr < (DRAM_BOOTLOADER_TEXT_START_ADDR + DRAM_BOOTLOADER_TEXT_LEN)))
        {
            offset = DRAM_BOOTLOADER_TEX_BIN_OFFSET+(proghdr->p_paddr-DRAM_BOOTLOADER_TEXT_START_ADDR);
        }
        else
        {
            offset = 0xffffffff;
        }

        if(offset!=0xffffffff)
        {
            memcpy(outputbuf+offset,p_elf+proghdr->p_offset,proghdr->p_filesz);
        }

        proghdr++;
        //I_SRAM_SEG_SIZE
    }
    

    return;
}

static unsigned int endian_swap(unsigned int input)
{
    unsigned int output;

    output = 0;
    output += ((input & 0xff000000) >> 24);
    output += ((input & 0x00ff0000) >> 8);
    output += ((input & 0x0000ff00) << 8);
    output += ((input & 0x000000ff) << 24);

    return output;
}


typedef union _FW_VERSION_INFO
{
    struct
    {
        /* BYTE 0: MCU Core Types */
        U8 ucMCUType;

        /* BYTE 1: Host Types */
        U8 ucHostType;

        /* BYTE 2: Flash Types */
        U8 ucFlashType;

        /* BYTE 3: Other Configurations */
        U8 ucOtherConfig;

        /* BYTE 4-7: Bootloader Parameter Types (Only for BL, FW reserved) */
        U32 ulBLParamTypes;

        /* BYTE 8-11: FW release version */
        U32 ulFWRleaseVerion;

        /* BYTE 12-15: GIT release version */
        U32 ulGITVersion;

        /* BYTE 16-19: Compile Date */
        U32 ulDateInfo;  /* 20150528 */

        /* BYTE 20-23: Compile Time */
        U32 ulTimeInfo;  /* 00163410 */
    };

    U32 ulFWVersion[6];
} FW_VERSION_INFO;

static void add_version(char *output, const char *input_version_info)
{
    int i;
    unsigned int ul_temp_dw;
    unsigned int ul_date;
    unsigned int ul_time;

    time_t timep;
    struct tm *ptm;

    char dw_str[10] = {0};
    unsigned int *p_curr_dw = (unsigned int*)output;
    FW_VERSION_INFO *p_version = (FW_VERSION_INFO *)output;

    for (i = 0; i < VERSION_INFO_LEN/8; i++)
    {
        memcpy(dw_str, &input_version_info[i*8], 8);
        //*p_curr_dw = atoi(dw_str);
        sscanf(dw_str, "%x", &ul_temp_dw);
        *p_curr_dw = ul_temp_dw;//endian_swap(ul_temp_dw);
        p_curr_dw++;
    }

    p_version->ulFWRleaseVerion = endian_swap(p_version->ulFWRleaseVerion);

    time(&timep);
    ptm = localtime(&timep);

    ul_date = ((ptm->tm_year + 1900)<<16) + ((ptm->tm_mon + 1)<<8) + (ptm->tm_mday);
    ul_time = (ptm->tm_hour << 16) + (ptm->tm_min << 8) + (ptm->tm_sec);

    *p_curr_dw = ul_date;
    p_curr_dw++;
    *p_curr_dw = ul_time;

    printf("add firmware version\n");
    p_curr_dw = (unsigned int*)output;
    for (i = 0; i < VERSION_TOTAL_LEN_DW; i++)
    {
        printf("0x%08x  ", p_curr_dw[i]);
    }
    printf("\n");
}

static void add_bootlaoder_version(char *p_out_bin_buff, char *input_version_info)
{
    add_version(p_out_bin_buff + BL_VERSION_OFFSET, input_version_info);
}

#define DRAM_FW_STATIC_INFO_BASE       (0x40048000)

static void add_firmware_version(char *p_out_bin_buff, char *input_version_info)
{
    add_version(p_out_bin_buff + FW_VERSION_OFFSET, input_version_info);
    //add_version(p_out_bin_buff + g_file_offset, input_version_info);

    add_fw_sec_des_ele(VER_INFO, 0, DRAM_FW_STATIC_INFO_BASE, VERSION_TOTAL_LEN, false);
}

int dump_firmware_bin(int argc, const char *argv[])
{
    char * p_buf;
    HANDLE pfile_core0;
//    HANDLE pfile_core12;
//  HANDLE pfile_rom;
    int imagelen_core0;
//    int imagelen_core12;
//    int imagelen_rom;
    char *image_core0 = NULL;
    char *image_core12 = NULL;
    char *image_rom = NULL;
    int tempdata = 0x12345678;
    int mcu_num = 0;

    char input_version_info[VERSION_INFO_LEN];

    #if 0
    if (VERSION_INFO_LEN != strlen(argv[5]))
    {
        printf("version info error!\n");
        //while(1);
        return 0;
    }
    #endif
    if (0 == strcmp("MCU0", argv[2]))
    {
        mcu_num = 0;
    }
    else if (0 == strcmp("MCU1", argv[2]))
    {
        mcu_num = 1;
    }
    else if (0 == strcmp("MCU2", argv[2]))
    {
        mcu_num = 2;
    }
    strcpy(inputfilefw_core0,argv[3]);
    //strcpy(inputfilefw_core12,argv[3]);
    //strcpy(inputfile_rom,argv[4]);
    strncpy(input_version_info,argv[4],VERSION_INFO_LEN);
    strcpy(outputfilefw_bin,argv[5]);

    GetFileInform(&pfile_core0, inputfilefw_core0, &image_core0, &imagelen_core0);
    //GetFileInform(&pfile_core12, inputfilefw_core12, &image_core12, &imagelen_core12);
    //GetFileInform(&pfile_rom, inputfile_rom, &image_rom, &imagelen_rom);
//     if(imagelen_rom > OPTION_ROM_LEN)
//     {
//         fprintf(stderr, "rom file is lager than expected!\n");
//         //while(1);
//         return 0;
//     }

    p_buf = (char *)malloc(FW_BIN_SIZE);
    memset(p_buf, 0, FW_BIN_SIZE);
    if (0 == mcu_num)
    {
        g_file_offset = g_fw_compact_start_addr;
    }
    else
    {
        g_file_offset = 0;
    }
    //g_file_offset += sizeof(FW_FILE_HDR);

    dump_fw_elf(image_core0, p_buf, mcu_num);
    if (0 == mcu_num)
    {
        add_firmware_version(p_buf, input_version_info);
    }
    //dump_fw_elf(image_core12, p_buf, 1);
    //dump_rom_file(image_rom, p_buf, imagelen_rom);
    g_fw_hdr_info.m_fw_file_hdr.m_sec_des_off = g_file_offset;
    U32 g_file_offset_bak = g_file_offset;
    {
        U32 l_file_offset ;

        if (0 == mcu_num)
        {
            l_file_offset = g_fw_compact_start_addr;
        }
        else
        {
            l_file_offset = 0;
        }
        U8 *lp_sec_ele_inbuf = new U8[g_fw_hdr_info.m_sec_des_ele_cnt];
        memset(lp_sec_ele_inbuf, 0, g_fw_hdr_info.m_sec_des_ele_cnt);
        FW_SEC_DES_ELE *lp_fw_sec_des_ele_tmp = NULL, *lp_fw_sec_des_ele_to_process = NULL;
        U32 phy_addr_to_process,ele_to_use_num;
        for (unsigned int i = 0; i < g_fw_hdr_info.m_sec_des_ele_cnt; i++)
        {
            lp_fw_sec_des_ele_tmp = g_fw_hdr_info.m_sec_des_list;
            lp_fw_sec_des_ele_to_process = NULL;
            phy_addr_to_process = 0xFFFFFFFF;

            //select one elemnt to place in bin file
            int ele_num = 0;
            while (lp_fw_sec_des_ele_tmp != NULL)
            {
                if (false == lp_sec_ele_inbuf[ele_num] && (lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off < phy_addr_to_process))
                {
                    phy_addr_to_process = lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off;
                    lp_fw_sec_des_ele_to_process = lp_fw_sec_des_ele_tmp;
                    ele_to_use_num = ele_num;
                }
                lp_fw_sec_des_ele_tmp = lp_fw_sec_des_ele_tmp->m_next;
                ele_num++;
            }
			//process the selected element
			lp_sec_ele_inbuf[ele_to_use_num] = true;
            if (lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off >= HEAD_BEGIN && lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off < HEAD_END)
            {
                lp_fw_sec_des_ele_to_process->m_sec_offset = lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off-HEAD_BEGIN;
            }
            else
            {
                lp_fw_sec_des_ele_to_process->m_sec_offset = l_file_offset;
            }			

			if (lp_fw_sec_des_ele_to_process->m_b_duplite_section_in_bin == false)
			{
				printf("0x%x 0x%x 0x%x %d\n", lp_fw_sec_des_ele_to_process->m_sec_offset,
					lp_fw_sec_des_ele_to_process->m_sec_off_to_input_file,
					lp_fw_sec_des_ele_to_process->m_sec_len,g_file_offset);

                if (lp_fw_sec_des_ele_to_process->file_type == ELF_MCU0 || lp_fw_sec_des_ele_to_process->file_type == ELF_MCU12 || lp_fw_sec_des_ele_to_process->file_type == ELF_MCU2)
                {
                    memcpy(p_buf + lp_fw_sec_des_ele_to_process->m_sec_offset,
                        (char *)(image_core0 + lp_fw_sec_des_ele_to_process->m_sec_off_to_input_file), lp_fw_sec_des_ele_to_process->m_sec_len);
                }               
                else if (lp_fw_sec_des_ele_to_process->file_type == VER_INFO)
                {
                    add_version(p_buf + lp_fw_sec_des_ele_to_process->m_sec_offset, input_version_info);
                }
                else
                {
                    printf("process segment element error \n");
                    getchar();
                }
            }
            else
            {
                //duplicate should adjast the fw bin file write pointer back
                lp_fw_sec_des_ele_to_process->m_sec_offset -= up_bound_dw_align(lp_fw_sec_des_ele_to_process->m_sec_len);
                printf("process duplicate section\n");
            }


            //add segment description to the end of bin file
            U32 len_temp = sizeof(FW_SEC_DES_ELE) - 12;  //more see FW_SEC_DES_ELE
            lp_fw_sec_des_ele_to_process->m_sec_len = up_bound_dw_align(lp_fw_sec_des_ele_to_process->m_sec_len);
            memcpy(p_buf + g_file_offset, (char *)lp_fw_sec_des_ele_to_process, len_temp);
            g_file_offset += len_temp;

            if ((lp_fw_sec_des_ele_to_process->m_b_duplite_section_in_bin == false)&&(lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off>=g_fw_compact_start_addr))
            {
                l_file_offset += lp_fw_sec_des_ele_to_process->m_sec_len;
            }

        }
        if (g_file_offset_bak != l_file_offset)
        {
            printf("length calc error(%d)\n",g_file_offset_bak-l_file_offset);
            getchar();
        }
        printf("00000====%d \n",g_file_offset);
        {
            if (0 == mcu_num)
            {
                // place file hdr size in its right location
                g_fw_hdr_info.m_fw_file_hdr.m_bin_total_len = g_file_offset;
                FW_FILE_HDR *lp_fw_file_hdr = &g_fw_hdr_info.m_fw_file_hdr;
                memcpy(p_buf + g_fw_total_len_location, (char *)lp_fw_file_hdr, sizeof(FW_FILE_HDR));               
            }           
            else
            {
                // add to the end of the bin file
                g_fw_hdr_info.m_fw_file_hdr.m_bin_total_len = g_file_offset;
                FW_FILE_HDR *lp_fw_file_hdr = &g_fw_hdr_info.m_fw_file_hdr;
                memcpy(p_buf + g_file_offset, (char *)lp_fw_file_hdr, sizeof(FW_FILE_HDR));
                g_file_offset += sizeof(FW_FILE_HDR);
                printf("%x %x",lp_fw_file_hdr->m_bin_total_len,lp_fw_file_hdr->m_sec_des_off);
            }

            // delete the dyn alloc space
            delete[]lp_sec_ele_inbuf;

            lp_fw_sec_des_ele_tmp = g_fw_hdr_info.m_sec_des_list;
            FW_SEC_DES_ELE *lp_fw_sec_des_ele_del = NULL;
            while (lp_fw_sec_des_ele_tmp != NULL)
            {
                lp_fw_sec_des_ele_del = lp_fw_sec_des_ele_tmp;
                lp_fw_sec_des_ele_tmp = lp_fw_sec_des_ele_tmp->m_next;
                delete lp_fw_sec_des_ele_del;
            }
        }
    }
    //CreateFwBinFile(p_buf,outputfilefw_bin,FW_BIN_SIZE);
    printf("%d --\n", g_file_offset);
    CreateFwBinFile(p_buf, outputfilefw_bin, g_file_offset);
    free(p_buf);

    printf("FW.bin Generate Done!!!,%d", g_file_offset);    
    exit(0);
}

int dump_bootloader_bin(int argc, const char *argv[])
{
    char in_elf_name[128];
    char out_bin_name[128];
    char *p_in_elf_buff = NULL;
    char *p_out_bin_buff = NULL;

    char input_version_info[VERSION_INFO_LEN];

    HANDLE pfile_bl_elf;
    int bl_elf_len;

    #if 0
    if (VERSION_INFO_LEN != strlen(argv[3]))
    {
        printf("version info error!\n");
        //while(1);
        return 0;
    }
    #endif

    strcpy(in_elf_name,argv[2]);
    strncpy(input_version_info, argv[3], VERSION_INFO_LEN);
    strcpy(out_bin_name,argv[4]);

    GetFileInform(&pfile_bl_elf, in_elf_name, &p_in_elf_buff, &bl_elf_len);

    p_out_bin_buff = (char*)malloc(BL_BIN_SIZE);

    if (NULL == p_out_bin_buff)
    {
        printf("malloc memory failed\n");
        if (NULL != p_in_elf_buff)
        {
            free(p_in_elf_buff);
        }
        return 0;
    }

    memset(p_out_bin_buff,0,BL_BIN_SIZE);

    dump_bootloader_elf(p_in_elf_buff, p_out_bin_buff);
    add_bootlaoder_version(p_out_bin_buff, input_version_info);

    CreateFwBinFile(p_out_bin_buff,out_bin_name,BL_BIN_SIZE);
    free(p_in_elf_buff);
    free(p_out_bin_buff);

    printf("bootloader.bin Generate Done!!!");
    exit(0);
    
}


#define BR_UNPACK_HDR_BASE_ADDR (0xffe00000)
#define BR_UNPACK_HDR_SIZE (0x100)

#define BR_ISRAM_BASE_ADDR (0x20000000)
#define BR_ISRAM_CODE_LEN (0x7000)

#define BR_DSRAM_BASE_ADDR (0x1ffa4800)
#define BR_DSRAM_LEN (0x3800)

#define BR_BIN_SIZE (BR_UNPACK_HDR_SIZE+BR_ISRAM_CODE_LEN+BR_DSRAM_LEN)
U32 g_br_compact_start_addr = BR_UNPACK_HDR_SIZE;


void add_br_sec_des_ele(FILE_TYPE type, U32 offset_to_input, U32 phy_offset, U32 sec_len, U8 b_duplite_section_in_bin)
{
    FW_SEC_DES_ELE *lp_fw_sec_des_ele_tmp = new FW_SEC_DES_ELE;

    lp_fw_sec_des_ele_tmp->file_type = type;
    lp_fw_sec_des_ele_tmp->m_sec_off_to_input_file = offset_to_input;
    lp_fw_sec_des_ele_tmp->m_next = NULL;
    lp_fw_sec_des_ele_tmp->m_sec_len = sec_len;
    lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off = phy_offset;
    lp_fw_sec_des_ele_tmp->m_b_duplite_section_in_bin = b_duplite_section_in_bin;
    if (NULL == g_fw_hdr_info.m_sec_des_list)
    {
        g_fw_hdr_info.m_sec_des_list = lp_fw_sec_des_ele_tmp;
        gp_cur_fw_sec_des_ele = lp_fw_sec_des_ele_tmp;
    }
    else
    {
        gp_cur_fw_sec_des_ele->m_next = lp_fw_sec_des_ele_tmp;
        gp_cur_fw_sec_des_ele = lp_fw_sec_des_ele_tmp;
    }

    g_fw_hdr_info.m_sec_des_ele_cnt++;
    if (b_duplite_section_in_bin == false)
    {
        //g_file_offset += sec_len; // duplicate section does not consume space in bin file
        g_file_offset += up_bound_dw_align(sec_len); // duplicate section does not consume space in bin file
    }
}

static void
dump_br_elf(char *p_fw_elf, char * outputbuf, int elf_num)
{
    Elf_Ehdr *hdr;
    bool valid_sec = false;
    U32 phy_offset, i, sec;
    Elf_Phdr *proghdr;
    hdr = (Elf_Ehdr *)p_fw_elf;
    //#ifdef PRINT
    dump_header(hdr);
    //#endif
    proghdr = (Elf_Phdr*)((U32)p_fw_elf + (U32)hdr->e_phoff);
    sec = 0;
#if 0
	for (i = 0; i < hdr->e_phnum; i++)
	{		
		printf("sec:0x%x	p_offset:0x%x	p_filesz:0x%x	p_memsz:0x%x	p_paddr:0x%x\n",sec,proghdr->p_offset,proghdr->p_filesz,proghdr->p_memsz,proghdr->p_paddr);
		proghdr = proghdr++;
		sec++;
	}
#endif

	phy_offset = 0;
	proghdr = (Elf_Phdr*)(p_fw_elf + hdr->e_phoff);
	for (i = 0; i < hdr->e_phnum; i++)
	{					

		/* BR unpack hdr enter this case */
		if ((proghdr->p_paddr >= (BR_UNPACK_HDR_BASE_ADDR)) && ((proghdr->p_paddr < (BR_UNPACK_HDR_BASE_ADDR + BR_UNPACK_HDR_SIZE))))
		{
			valid_sec = false;
			memcpy(outputbuf + proghdr->p_paddr - (BR_UNPACK_HDR_BASE_ADDR), p_fw_elf + proghdr->p_offset, proghdr->p_filesz);
			//printf("======0x%x %x %x\n", proghdr->p_paddr, proghdr->p_offset, proghdr->p_filesz);
		}
        else
        {
            phy_offset = proghdr->p_paddr;
            valid_sec = true;
        }
		

#if 0
		/* BR isram segments enter this case */
		if ((proghdr->p_paddr >= (BR_ISRAM_BASE_ADDR)) && ((proghdr->p_paddr<(BR_ISRAM_BASE_ADDR + BR_ISRAM_CODE_LEN))))
		{
			phy_offset = (proghdr->p_paddr - BR_DSRAM_BASE_ADDR);
			valid_sec = true;
		}

		/* BR data segments enter this case */
		if ((proghdr->p_paddr >= (BR_DSRAM_BASE_ADDR)) && ((proghdr->p_paddr < (BR_DSRAM_BASE_ADDR + BR_DSRAM_LEN))))
		{
			phy_offset = (proghdr->p_paddr - BR_DSRAM_BASE_ADDR);
			valid_sec = true;
		}
#endif

		if (true == valid_sec && proghdr->p_filesz != 0)
		{			
			add_br_sec_des_ele((FILE_TYPE)elf_num, proghdr->p_offset, phy_offset, proghdr->p_filesz, false);
        }
        valid_sec = false;

        proghdr++;      
    }

    return;
}

U32 g_br_total_len_location = 0x50;

int dump_rom_bin(int argc, const char *argv[])
{
    printf("dump rom bin\n");
    g_fw_compact_start_addr = g_br_compact_start_addr;  

    char * p_buf;
    HANDLE pfile_core0;
    int imagelen_core0; 
    char *image_core0 = NULL;
    int mcu_num = 0;

    strcpy(inputfilefw_core0, argv[2]);
    strcpy(outputfilefw_bin, argv[3]);

    GetFileInform(&pfile_core0, inputfilefw_core0, &image_core0, &imagelen_core0);

    p_buf = (char *)malloc(BR_BIN_SIZE);
    memset(p_buf, 0, BR_BIN_SIZE);
    g_file_offset = g_fw_compact_start_addr;    

    dump_br_elf(image_core0, p_buf, BOOT_ROM);
    
    g_fw_hdr_info.m_fw_file_hdr.m_sec_des_off = g_file_offset;
    U32 g_file_offset_bak = g_file_offset;
    {
        U32 l_file_offset;
        
        l_file_offset = g_fw_compact_start_addr;
        
        U8 *lp_sec_ele_inbuf = new U8[g_fw_hdr_info.m_sec_des_ele_cnt];
        memset(lp_sec_ele_inbuf, 0, g_fw_hdr_info.m_sec_des_ele_cnt);
        FW_SEC_DES_ELE *lp_fw_sec_des_ele_tmp = NULL, *lp_fw_sec_des_ele_to_process = NULL;
        U32 phy_addr_to_process, ele_to_use_num;
        for (unsigned int i = 0; i < g_fw_hdr_info.m_sec_des_ele_cnt; i++)
        {
            lp_fw_sec_des_ele_tmp = g_fw_hdr_info.m_sec_des_list;
            lp_fw_sec_des_ele_to_process = NULL;
            phy_addr_to_process = 0xFFFFFFFF;

            //select one elemnt to place in bin file
            int ele_num = 0;
            while (lp_fw_sec_des_ele_tmp != NULL)
            {
                if (false == lp_sec_ele_inbuf[ele_num] && (lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off < phy_addr_to_process))
                {
                    phy_addr_to_process = lp_fw_sec_des_ele_tmp->m_sec_phy_addr_off;
                    lp_fw_sec_des_ele_to_process = lp_fw_sec_des_ele_tmp;
                    ele_to_use_num = ele_num;
                }
                lp_fw_sec_des_ele_tmp = lp_fw_sec_des_ele_tmp->m_next;
                ele_num++;
			}
			//process the selected element
			lp_sec_ele_inbuf[ele_to_use_num] = true;
            if (lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off >= BR_UNPACK_HDR_BASE_ADDR &&
                lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off < (BR_UNPACK_HDR_BASE_ADDR + BR_UNPACK_HDR_SIZE))
            {
                lp_fw_sec_des_ele_to_process->m_sec_offset = lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off - BR_UNPACK_HDR_BASE_ADDR;
            }
            else
            {
                lp_fw_sec_des_ele_to_process->m_sec_offset = l_file_offset;
            }


			if (lp_fw_sec_des_ele_to_process->m_b_duplite_section_in_bin == false)
			{
#if 0
				printf("0x%x phy_off:0x%x file_off:0x%x  0x%x pos:0x%x\n", lp_fw_sec_des_ele_to_process->m_sec_offset,
					lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off,
					lp_fw_sec_des_ele_to_process->m_sec_off_to_input_file,
					lp_fw_sec_des_ele_to_process->m_sec_len, g_file_offset);
#endif

				if (lp_fw_sec_des_ele_to_process->file_type == BOOT_ROM)
				{
					memcpy(p_buf + lp_fw_sec_des_ele_to_process->m_sec_offset,
						(char *)(image_core0 + lp_fw_sec_des_ele_to_process->m_sec_off_to_input_file), lp_fw_sec_des_ele_to_process->m_sec_len);
				}				
				else
				{
					printf("process segment element error \n");
                    getchar();
                }
            }
            else
            {
                //duplicate should adjast the fw bin file write pointer back
                lp_fw_sec_des_ele_to_process->m_sec_offset -= up_bound_dw_align(lp_fw_sec_des_ele_to_process->m_sec_len);
                printf("process duplicate section\n");
            }


            //add segment description to the end of bin file
            U32 len_temp = sizeof(FW_SEC_DES_ELE) - 12;  //more see FW_SEC_DES_ELE
            lp_fw_sec_des_ele_to_process->m_sec_len = up_bound_dw_align(lp_fw_sec_des_ele_to_process->m_sec_len);
            memcpy(p_buf + g_file_offset, (char *)lp_fw_sec_des_ele_to_process, len_temp);
            g_file_offset += len_temp;

			if ((lp_fw_sec_des_ele_to_process->m_b_duplite_section_in_bin == false) && 
				(lp_fw_sec_des_ele_to_process->m_sec_phy_addr_off <= BR_ISRAM_CODE_LEN+(BR_ISRAM_BASE_ADDR)))
			{
				l_file_offset += lp_fw_sec_des_ele_to_process->m_sec_len;
            }

        }
        if (g_file_offset_bak != l_file_offset)
        {
            printf("length calc error(%d)\n", g_file_offset_bak - l_file_offset);
            getchar();
        }
        //printf("00000====%d \n", g_file_offset);
        {           
            // place file hdr size in its right location
            g_fw_hdr_info.m_fw_file_hdr.m_bin_total_len = g_file_offset;
            FW_FILE_HDR *lp_fw_file_hdr = &g_fw_hdr_info.m_fw_file_hdr;
            memcpy(p_buf + g_br_total_len_location, (char *)lp_fw_file_hdr, sizeof(FW_FILE_HDR));
            
            // delete the dyn alloc space
            delete[]lp_sec_ele_inbuf;

            lp_fw_sec_des_ele_tmp = g_fw_hdr_info.m_sec_des_list;
            FW_SEC_DES_ELE *lp_fw_sec_des_ele_del = NULL;
            while (lp_fw_sec_des_ele_tmp != NULL)
            {
                lp_fw_sec_des_ele_del = lp_fw_sec_des_ele_tmp;
                lp_fw_sec_des_ele_tmp = lp_fw_sec_des_ele_tmp->m_next;
                delete lp_fw_sec_des_ele_del;
            }
        }
    }
    //CreateFwBinFile(p_buf,outputfilefw_bin,FW_BIN_SIZE);
    //printf("%d --\n", g_file_offset);
    CreateFwBinFile(p_buf, outputfilefw_bin, g_file_offset);
    free(p_buf);

    printf("BR.bin Generate Done!!! Total: %d bytes\n", g_file_offset); 

    exit(0);    

    return 0;
}



int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        usage(argv[0]);
        return 0;
    }

    if (0 == strcmp(argv[1], "BL"))
    {
        if ((BL_PARAM_NUM + 1) == argc)
        {
            dump_bootloader_bin(argc, argv);
        }
        else
        {
            usage(argv[0]);
        }
    }
    else if(0 == strcmp(argv[1], "FW"))
    {
        if ((FW_PARAM_NUM + 1) == argc)
        {
            dump_firmware_bin(argc, argv);
        }
        else
        {
            usage(argv[0]);
        }
    }
    else if (0 == strcmp(argv[1], "BR"))
    {
        if ((BR_PARAM_NUM + 1) == argc)
        {
            dump_rom_bin(argc, argv);
        }
        else
        {
            usage(argv[0]);
        }
    }
    else
    {
        usage(argv[0]);
    }

    return 0;
}

