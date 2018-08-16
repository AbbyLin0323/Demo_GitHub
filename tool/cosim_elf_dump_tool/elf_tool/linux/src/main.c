#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define uchar unsigned char
#define uint  unsigned int 
#define NAME_LEN        (128)
#define MEM_TYPE        (5)
#define MAX_FILE_NUM    (32)
#define MAX_TOTAL_FILE_NUM  (128)

#define bool  unsigned char 
#define true  1
#define false 0

enum mcu_id {MCU0_ID,MCU1_ID,MCU2_ID,MCU_NUM};



typedef struct PRELOAD_DPTR_IN
{
    char PreloadFileName[NAME_LEN];       
    uint  FileSize;                 // size by kb   -- eg: 24 -> 24k
    uint  MemType;                  // 0 - ROM ,1 - DSRAM ,2 - ISRAM ,3 - OTFB , 4 - DRAM
    uint  Alignment;                // 32 - 1 DW align ,64 - 2 DW align
}PRELOAD_DPTR_IN;

typedef struct PRELOAD_DPTR
{
    char PreloadFileName[NAME_LEN];
    uint  FileSize;
    bool  DWAlign;
}PRELOAD_DPTR;

PRELOAD_DPTR_IN g_aTempFile[MAX_TOTAL_FILE_NUM] = {0}; 
PRELOAD_DPTR g_aFile[MEM_TYPE][MAX_FILE_NUM] = {0};
int g_FileNum[MEM_TYPE] = {0};


int  chtoi(char ch)
{
	if((ch>='0')&&(ch<='9'))
		return (ch - '0');
	else if((ch>='a')&&(ch<='f'))
		return (ch - 'a' + 10);
	else if((ch>='A')&&(ch<='F'))
		return (ch - 'A' + 10);
	else	
		return 0;
}

int stoi(int num,char* s)
{
	int i = 0;
	int res = 0;
	
	for(i=0;i<num;i++)
	{
		res = chtoi(s[i]) + res*16;
	}
	return res;
}

int LoadConfFile(uchar ucMcuId)
{
    int i,depth;
    char McuFileName[MCU_NUM][NAME_LEN] = {"mcu0.df","mcu1.df","mcu2.df"};
    FILE *fp = fopen(McuFileName[ucMcuId],"r");
    
    if(NULL == fp)
    {
        printf("File mcu%d open failed.\n",ucMcuId);
        exit(1);
    }
    memset((char*)g_aTempFile,0,sizeof(PRELOAD_DPTR_IN)*MAX_TOTAL_FILE_NUM);
    fseek(fp,0,SEEK_END);
    depth = ftell(fp);
    fseek(fp,0,SEEK_SET);
    for (i=0;depth != ftell(fp);i++)
    {
        fscanf(fp,"%s%d%d%d",&g_aTempFile[i].PreloadFileName,&g_aTempFile[i].FileSize
                            ,&g_aTempFile[i].MemType,&g_aTempFile[i].Alignment) ;
    }
    fclose(fp);
    i--;
    printf("Total create file number is %d\n",i);
    return i;
}

void DraftPreload(int fnum)
{
    int i,j,fsize,falignch;
    FILE *fp;
    for (i=0;i<fnum;i++)
    {
        fp = fopen(g_aTempFile[i].PreloadFileName,"w");
        if (NULL == fp)
        {
            printf("Can not open file :%s \n",g_aTempFile[i].PreloadFileName);
            exit(1);
        }

        fsize = g_aTempFile[i].FileSize<<10;            //  file size * 1k  ---  24(input) --- 24*1024 (output)
        falignch = (g_aTempFile[i].Alignment/4);         //  8 byte per line --  32 bits(input) -- 8byte (output)
        for(j=0;j<(fsize/falignch);j++)
        {
            if(8 == falignch)
            {
                fprintf(fp,"00000000\r\n");
            }
            else if(16 == falignch)
            {
                fprintf(fp,"0000000000000000\r\n");
            }
            else
            {
                printf("Align error\n");
                getchar();
            }
        }
        fclose(fp);
    }    
}
void FileDptrInit(uchar line)
{
    int i,j,type,index[MEM_TYPE] = {0}; 
    memset((char*)&g_aFile[0][0],0,sizeof(PRELOAD_DPTR)*MEM_TYPE*MAX_FILE_NUM);
    memset((int*)g_FileNum,0,sizeof(g_FileNum));
    printf("sizeof g_FileNum %d\n",sizeof(g_FileNum));
    for (i=0;i<line;i++)
    {
        type = g_aTempFile[i].MemType;
        strcpy(g_aFile[type][index[type]].PreloadFileName,g_aTempFile[i].PreloadFileName);
        g_aFile[type][index[type]].FileSize = g_aTempFile[i].FileSize << 10;
        g_aFile[type][index[type]].DWAlign =  (g_aTempFile[i].Alignment == 32);
        g_FileNum[type]++;
        index[type]++;
        
    }

    for(i=0;i<MEM_TYPE;i++)
    {
        for(j=0;j< g_FileNum[i];j++)
        {
            printf("%s\n",g_aFile[i][j].PreloadFileName);
        }
        printf("\n");
    }

}
/*

int main ()
{


    LoadConfFile(0);
    return 0;
}
*/

bool CheckMcuInputFile(bool bMulCore,uchar id,char *argv[])
{
    FILE *fp_Conf[MCU_NUM] = {NULL},*fp_Txt[MCU_NUM] = {NULL};
    char Conf[MCU_NUM][NAME_LEN] = {"mcu0.df","mcu1.df","mcu2.df"};
    char i;
    if (true == bMulCore)
    {
        for(i=0;i<MCU_NUM;i++)
        {
            fp_Conf[i] = fopen(Conf[i],"r");
            if(NULL == fp_Conf[i])
            {
                printf("Cannot open mcu%d.df !!\n",i);
            }
            else
            {
                fclose(fp_Conf[i]);
            }

            fp_Txt[i] = fopen(argv[2+i],"r");
            if(NULL == fp_Txt[i])
            {
                printf("Cannot open elf of mcu&d\n",i);
            }
            else
            {
                fclose(fp_Txt[i]);
            }
        }      
        return false;
    }
    else
    {
        fp_Conf[id] = fopen(Conf[id],"r");
        if(NULL == fp_Conf[id])
        {
            printf("Cannot open mcu%d.df !!\n",id);
        }
        else
        {
            fclose(fp_Conf[id]);
        }

        fp_Txt[id] = fopen(argv[3],"r");
        if(NULL == fp_Txt[id])
        {
            printf("Cannot open text file of mcu%d\n",id);
        }  
        else
        {
            fclose(fp_Txt[id]);
        }
        return false;
    }
    return true;
}

void DeletePreload(int filenum)
{
    int i;
    for(i=0;i<filenum;i++)
    {
       remove(g_aTempFile[i].PreloadFileName);
    }
    printf("Complete remove exist preload.\n");
}

int GetFileLen(FILE *fp)
{
    int len;
    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    fseek(fp,0,SEEK_SET);   
    return len;
}
void CreatePreload(FILE* fp,int InFileLen)
{
	char *strtype = (char*)malloc(9) ;
	char *strlens = (char*)malloc(9) ;
    char *txt;
    int *base,*start;
    uchar *p_out;
	int type = 0 ,lens = 0;
    int i,j,k,remlens,filesize,MaxFileSzDw,ChPerLine;
    FILE *fout;
	static int uuu = 1;
	int m = 0;

    while (InFileLen>1)
    {
	    memset(strtype,'\0',9);
	    memset(strlens,'\0',9);
 		fread(strtype,8,1,fp);
		fread(strlens,8,1,fp);  
        type = stoi(8,strtype);
        lens = stoi(8,strlens)*2;

        InFileLen -= lens + 16;
        txt = (char*)malloc(lens+4);
        memset(txt,0,(lens+4));
        fread(txt,1,lens,fp);
        base = (int*)malloc(lens/2+4);
        remlens = (lens%8)?(lens/8+1):(lens/8);
        for(i=0;i<remlens;i++)
	    {
		    base[i] = stoi(8,&txt[i*8]);	
	    }
        start = base;
        for (i=0;i<g_FileNum[type];i++)
        {
            MaxFileSzDw = g_aFile[type][i].FileSize >> 2;
            p_out = (uchar*)malloc(g_aFile[type][i].FileSize);
            memset(p_out,0,g_aFile[type][i].FileSize);
            
            if (remlens > MaxFileSzDw)
            {
                filesize = MaxFileSzDw;
            }
            else
            {
                filesize = remlens;
            }
            memcpy(p_out,start,filesize*sizeof(int));
            start += filesize;
            remlens -= filesize;

            fout = fopen(g_aFile[type][i].PreloadFileName,"w");
            if (NULL == fout)
            {
                printf("Create preload file %s failed.\n",g_aFile[type][i].PreloadFileName);
                exit(1);
            }

            if (true == g_aFile[type][i].DWAlign)
            {
                ChPerLine = 4;         
            }
            else
            {
                ChPerLine = 8;
                MaxFileSzDw = MaxFileSzDw/2;
            }

            for (j=0;j<MaxFileSzDw;j++)
            {
                for(k=0;k<ChPerLine;k++)
                {
                    fprintf(fout,"%.2x",*(p_out+j*ChPerLine+(ChPerLine-1-k))); 
                }
                fprintf(fout,"\r\n");
            }
            fclose(fout);
            free(p_out);
        }
        free(txt);
    }
}

void Generate(char *FileInName,uchar id)
{
    int TotalFileNum = 0;
    int InFileLen = 0;
    int PrgLen[MEM_TYPE] = {0};
    FILE *fp_Input = fopen(FileInName,"r");
    if (NULL == fp_Input)
    {
        printf("Can not open file : %s\n",FileInName);
        exit(3);
    }
    printf("Open file %s\n",FileInName);
    TotalFileNum = LoadConfFile(id);
    DeletePreload(TotalFileNum);
    DraftPreload(TotalFileNum);
    FileDptrInit(TotalFileNum);
    InFileLen = GetFileLen(fp_Input);
    CreatePreload(fp_Input,InFileLen); 
    fclose(fp_Input);
}    

int main(int argc,char* argv[])
{
    if (argc < 3)
    {
        printf("Input error!\n");
        printf("<mode> (mcu id) <file1> (<file2> <file3>)\n");
        printf(" -s single core. eg : -s -0 aMCU0\n");
        printf(" -m multi core. eg : -m aMCU0 aMCU1 aMCU2\n");
        exit(1);
    }

    if((*(argv[1]+1)=='s') || (*(argv[1]+1) =='S'))
    {
        int id = -(int)atoi(argv[2]);
        if ((id>2)||(id<0))
        {
            printf("Please select mcu 0~2.\n");
            exit(1);
        }    
        CheckMcuInputFile(false,id,argv);
        printf("Create MCU %d preload\n",id);
        Generate(argv[3],id);
    }
    else if ((*(argv[1]+1)=='m') || (*(argv[1]+1) =='M'))
    {
        int i;
        if (argc<5)
        {
            printf("Input error!\n");
            printf(" -m multi core. eg : -m aMCU0 aMCU1 aMCU2\n");
            exit(2);
        }
        CheckMcuInputFile(true,-1,argv);
        printf("Create MCU 0~2 preload\n");
        for(i=0;i<MCU_NUM;i++)
        {
            Generate(argv[2+i],i);
        }
    }
	printf("Create preload file successfully\n");
	return 1;
}

