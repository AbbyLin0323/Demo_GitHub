/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
  File Name     : iniparser.c
  Version       : Initial Draft
  Author        : Nina Yang
  Created       : 2014/4/13
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2014/4/13
    Author      : Nina Yang
    Modification: Created file

******************************************************************************/
//#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <ctype.h>
#include <conio.h>
#include <Windows.h>
#include "iniparser.h"

extern void add_section(char* section);
extern void add_condlist(char* sec,char* key,char* value);
extern void add_hscmdlist(char* scmd_name);
extern void add_hscmd_define(char* scmd_name,char* key,char* value);

WIN32_FIND_DATA FindFileData; 
HANDLE hFind; 
BOOL bFinish;

char * strstrip(char * s)
{
    static char l[255];
    char * last ;
    
    if (s==NULL) return NULL ;
    
    while (isspace((int)*s) && *s) 
        s++;
    memset(l, 0, 255);
    strcpy(l, s);
    last = l + strlen(l);
    while (last > l) {
        if (!isspace((int)*(last-1)))
            break ;
        last -- ;
    }
    *last = (char)0;
    return (char*)l ;
}
static char * strlwc(const char * s)
{
    static char l[LINESZ+1];
    int i ;

    if (s==NULL) return NULL ;
    memset(l, 0, LINESZ+1);
    i=0 ;
    while (s[i] && i<LINESZ) {
        l[i] = (char)tolower((int)s[i]);
        i++ ;
    }
    l[LINESZ]=(char)0;
    return l ;
}

static line_status iniparser_line(
    char * input_line,
    char * section,
    char * key,
    char * value)
{   
    line_status sta ;
    char        line[1024];
    int         len ;

    strcpy(line, strstrip(input_line));
    len = (int)strlen(line);

    sta = LINE_UNPROCESSED ;
    if (len<1) {
        /* Empty line */
        sta = LINE_EMPTY ;
    } else if (line[0]=='#') {
        /* Comment line */
        sta = LINE_COMMENT ; 
    } else if (line[0]=='[' && line[len-1]==']') {
        /* Section name */
        sscanf(line, "[%[^]]", section);
        strcpy(section, strstrip(section));
        //strcpy(section, strlwc(section));
        sta = LINE_SECTION ;
    } else if (sscanf (line, "%[^=] = \"%[^\"]\"", key, value) == 2
           ||  sscanf (line, "%[^=] = '%[^\']'",   key, value) == 2
           ||  sscanf (line, "%[^=] = %[^;#]",     key, value) == 2) {
        /* Usual key=value, with or without comments */
        strcpy(key, strstrip(key));
        //strcpy(key, strlwc(key));
        strcpy(value, strstrip(value));
        /*
         * sscanf cannot handle '' or "" as empty values
         * this is done here
         */
        if (!strcmp(value, "\"\"") || (!strcmp(value, "''"))) {
            value[0]=0 ;
        }
        sta = LINE_VALUE ;
    } else if (sscanf(line, "%[^=] = %[;#]", key, value)==2
           ||  sscanf(line, "%[^=] %[=]", key, value) == 2) {
        /*
         * Special cases:
         * key=
         * key=;
         * key=#
         */
        strcpy(key, strstrip(key));
        //strcpy(key, strlwc(key));
        value[0]=0 ;
        sta = LINE_VALUE ;
    } else {
        /* Generate syntax error */
        sta = LINE_ERROR ;
    }
    return sta ;
}

BOOL iniparser_load(char* ininame)
{
    FILE *in_stream;
    char line[LINESZ];
    char section[255];  
    char key[255];
    char value[255];
    int  len ;
    int  lineno=0 ;
    int  last=0 ;

    strcpy(section,"");

    if((in_stream = fopen(ininame, "r" )) != NULL )
    {
        while(fgets(line,sizeof(line),in_stream) != NULL)
        {
            lineno++ ;
            len = (int)strlen(line)-1;
            /* Safety check against buffer overflows */
            if (line[len]!='\n') {
                fprintf(stderr,
                    "iniparser: input line too long in %s (%d)\n",
                    ininame,
                    lineno);
                fclose(in_stream);
                return FALSE ;
            }
            /* Get rid of \n and spaces at end of line */
            while ((len>=0) &&
                ((line[len]=='\n') || (isspace(line[len])))) {
                    line[len]=0 ;
                    len-- ;
            }
            /* Detect multi-line */
            if (line[len]=='\\') {
                /* Multi-line value */
                last=len ;
                continue ;
            } else {
                last=0 ;
            }
            switch (iniparser_line(line, section, key, value)) 
            {
            case LINE_EMPTY:
            case LINE_COMMENT:
                break ;

            case LINE_SECTION:
                add_section(section);
                break ;

            case LINE_VALUE:
                add_condlist(section,key,value);
                break ;

            case LINE_ERROR:
                fprintf(stderr, "iniparser: syntax error in %s (%d):\n",
                    ininame,
                    lineno);
                fprintf(stderr, "-> %s\n", line);
                break;

            default:
                break ;
            }
            memset(line, 0, LINESZ);
            last=0;
        }
        fclose(in_stream);
    }
    else
    {       
        return FALSE;
    }
    return TRUE;
}

BOOL scmd_parser_load(char* ininame)
{
    FILE *in_stream;
    char line[LINESZ];
    char section[255];  
    char key[255];
    char value[255];
    int  len ;
    int  lineno=0 ;
    int  last=0 ;

    strcpy(section,"");

    if((in_stream = fopen(ininame, "r" )) != NULL )
    {
        while(fgets(line,sizeof(line),in_stream) != NULL)
        {
            lineno++ ;
            len = (int)strlen(line)-1;
            /* Safety check against buffer overflows */
            if (line[len]!='\n') {
                fprintf(stderr,
                    "iniparser: input line too long in %s (%d)\n",
                    ininame,
                    lineno);
                fclose(in_stream);
                return FALSE ;
            }
            /* Get rid of \n and spaces at end of line */
            while ((len>=0) &&
                ((line[len]=='\n') || (isspace(line[len])))) {
                    line[len]=0 ;
                    len-- ;
            }
            /* Detect multi-line */
            if (line[len]=='\\') {
                /* Multi-line value */
                last=len ;
                continue ;
            } else {
                last=0 ;
            }
            switch (iniparser_line(line, section, key, value)) 
            {
            case LINE_EMPTY:
            case LINE_COMMENT:
                break ;

            case LINE_SECTION:
                add_hscmdlist(section);
                break ;

            case LINE_VALUE:
                add_hscmd_define(section,key,value);
                break ;

            case LINE_ERROR:
                fprintf(stderr, "iniparser: syntax error in %s (%d):\n",
                    ininame,
                    lineno);
                fprintf(stderr, "-> %s\n", line);
                break;

            default:
                break ;
            }
            memset(line, 0, LINESZ);
            last=0;
        }
        fclose(in_stream);
    }
    else
    {       
        return FALSE;
    }
    return TRUE;
}
U16 get_word_num(char* line)
{
    U16 word_num = INVALID_4F;
    char num[3] = {'0'};

    if(!strncmp(line,"Word",4))
    {
        strncpy(num,line+5,3);
        word_num = (U16)strtoul(num,NULL,10); 
    }
    return (U16)word_num;
}
U16 parse_hex(U16* pBuffer,U32 word_num,char* line,char* new_line,BOOL* bHex)
{
    U32 BufIndex = 0;
    char* FlagStr = NULL;   
    U8 num = 0;
    char BufVal[16] = {'0'};
    char buffer[1024] = "";
    char *s;

    U32 ValueIndex = 0;
    U32 j;

    FlagStr = line;
    while(*FlagStr) 
    {
        if(*FlagStr != '#')
        {
            buffer[BufIndex] = *FlagStr;       
            BufIndex++;
        }
        else
        {
            *bHex = TRUE;
            num++;
            if(num % 4 == 0)
            {
                j = (U32)strlen(buffer);
                sprintf(BufVal,"%4X",*(pBuffer+word_num+ValueIndex)); 
                s = BufVal;
                while(*s) 
                {
                    if(*s == ' ') 
                    {
                        *s = '0';
                    }
                    s++;
                }
                j += sprintf(buffer+j,BufVal);
                ValueIndex++;
                BufIndex += 4;
            }
        }
        FlagStr++;
    }
    memcpy(new_line,buffer,strlen(buffer)+1);
    return (word_num+ValueIndex);
}
U16 parse_binary(U16* pBuffer,U32 word_num,U16 bitIndex,char* line,char* new_line)
{
    U32 BufIndex = 0;
    char* FlagStr = NULL;   
    U8 num = 0;
    char BufVal[16] = {'0'};
    char buffer[1024] = "";
    U32 j;
    U16 Value = 0;

    FlagStr = line;
    while(*FlagStr) 
    {
        if(*FlagStr != '&')
        {
            buffer[BufIndex] = *FlagStr;       
            BufIndex++;
        }
        else
        {
            j = (U32)strlen(buffer);
            Value = *(pBuffer+word_num);
            sprintf(BufVal,"%d",(Value&(1<<bitIndex)) >> bitIndex);  
            j += sprintf(buffer+j,BufVal);
            if(bitIndex > 0)
            {
                bitIndex--;
            }
            BufIndex++;          
        }
        FlagStr++;
    }
    memcpy(new_line,buffer,strlen(buffer)+1);
    return (bitIndex);    
}
void Parse_IdentifyData(U16* pBuffer)
{
    FILE*  pFile;
    FILE* pFileDst;
    char line[1024];
    char new_line[1024];
    U16 word_num;
    U16 next_line_word_num;
    U16 cur_line_word_num;
    U16 bitIndex = 15;
    BOOL bHex = FALSE;
    char file_name[128] = "checklist/special_cmd_list/IdfyInfo.txt";
    char file_name_dst[128] = "VT3514_IdfyInfo.txt";

    pFileDst = fopen(file_name_dst, "w" );

    if((pFile = fopen(file_name, "r" )) != NULL )
    {
        while(fgets(line,sizeof(line),pFile) != NULL)
        {
            word_num = get_word_num(line);
            if(INVALID_4F != word_num)
            {
                cur_line_word_num = word_num;
                if(0 == bitIndex)
                {
                    bitIndex = 15;
                }
                next_line_word_num = parse_hex(pBuffer,word_num,line,new_line,&bHex);
                fprintf(pFileDst,new_line);
            }
            else
            {    
                bHex = FALSE;
                next_line_word_num = parse_hex(pBuffer,next_line_word_num,line,new_line,&bHex);
                if(FALSE == bHex)
                {
                    bitIndex = parse_binary(pBuffer,cur_line_word_num,bitIndex,line,new_line);
                }
                fprintf(pFileDst,new_line);
            }
        }
        fclose(pFileDst);
        fclose(pFile);
    }

}
void GetChecklistInit(char *FilePath)
{
    hFind =  FindFirstFile((LPCSTR)FilePath, &FindFileData);

    if(hFind == INVALID_HANDLE_VALUE)
    {
        printf("Checklist path check, find no file!\n");
        _getch();
    }
    bFinish = FALSE;
}

void GetChecklistName(char *pFileName)
{
    while(1)
    {
        if(bFinish == TRUE)
        {
            *pFileName = '!';
            break;
        }
        if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
        {
            sprintf(pFileName, "%s", FindFileData.cFileName);
        }
        else
        {
            *pFileName = '@';
        }

        if(!FindNextFile(hFind, &FindFileData))  
        { 
            if (GetLastError() == ERROR_NO_MORE_FILES)  
            {               
                bFinish = TRUE;  
            }  
            else  
            { 
                printf("File Find Error!\n");
                _getch();
            }  
        } 
        if(*pFileName != '@')
            break;
    }
}
