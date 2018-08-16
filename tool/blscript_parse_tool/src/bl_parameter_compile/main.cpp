
#include "main.h"
#include "gen_list.h"
#include "gen_bin.h"
#include "gen_cfg.h"

TCHAR g_exeFolder[MAX_PATH] = { 0 };


static void GetFileFolder(TCHAR * pFolder)
{
    TCHAR * pTemp = NULL;
    
    if (pFolder == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to folder is NULL!\n"));
        return;
    }

    GetModuleFileName(NULL, pFolder, MAX_PATH);
    pTemp = _tcsrchr(pFolder, '\\');
    if (NULL != pTemp)
    {        
        memset(pTemp, 0, _tcslen(pTemp) * sizeof(TCHAR));        
    }

    return;
}

static BOOL DelAllListFiles(const TCHAR *pFolder)
{
    TCHAR findPath[MAX_PATH] = { 0 };
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = NULL;
    BOOL res = FALSE;

    if (pFolder == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to folder is NULL!\n"));
        return FALSE;
    }

    _stprintf_s(findPath, MAX_PATH, _T("%s\\*.lst"), pFolder);
    hFile = FindFirstFile(findPath, &FindFileData);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return res;        
    }

    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {            
        }
        else//list files
        {   
            _stprintf_s(findPath, MAX_PATH, _T("%s\\%s"), pFolder, FindFileData.cFileName);
            res = DeleteFile(findPath);            
        }
    } while (FindNextFile(hFile, &FindFileData));

    FindClose(hFile);

    return res;
}

BOOL IsAbsolutePath(const TCHAR * pPath)
{
    if (pPath == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to path is NULL!\n"));
        return FALSE;
    }

    if ((((*pPath >= _T('c')) && (*pPath <= _T('z'))) || ((*pPath >= _T('C')) && (*pPath <= _T('Z'))))
        && (*(pPath + 1) == _T(':'))
        && ((*(pPath + 2) == _T('\\')) || (*(pPath + 2) == _T('/'))))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void ShowInputInfo(void)
{
    _tprintf(_T("\n:: the parameter_1: 0 -- transfer *.cfg file to *.bin file;\n"));
    _tprintf(_T("                    1 -- transfer *.bin file to *.cfg file.\n"));
    _tprintf(_T("for *.cfg--> *.lst:\n"));
    _tprintf(_T(":: the parameter_2: the root path.\n"));
    _tprintf(_T("                    if NULL, set the root path to be the diretory of the EXE.\n"));
    _tprintf(_T("                    if NULL, the following parameters move forward, that's parameter_2 and parameter_3.\n"));
    _tprintf(_T(":: the parameter_3: 4K parameter binary file name which will be generated.\n"));
    _tprintf(_T(":: the parameter_4: the relative or absolute path of the \"main.cfg\".\n"));
    _tprintf(_T("                    if relative path, it's relative to the root path.\n"));
    _tprintf(_T("                    if NULL, it means that the \"main.cfg\" is located in the same diretory with the root path.\n"));
    _tprintf(_T("\n"));
    _tprintf(_T("for *.lst--> *.cfg:\n"));
    _tprintf(_T(":: the parameter_2: the root path.\n"));
    _tprintf(_T("                    if NULL, set the root path to be the diretory of the EXE.\n"));
    _tprintf(_T("                    if NULL, the following parameters move forward, that's parameter_2 and parameter_3.\n"));
    _tprintf(_T(":: the parameter_3: 4K parameter binary file name which will be parsed to the config file.\n"));
    _tprintf(_T("                    the binary file must in the \"root path\\ouput\\\" diretory.\n"));
    _tprintf(_T(":: the parameter_4: the parse config file name which will be generated.\n"));
    _tprintf(_T("                    the config file will be in \"root path\\ouput\\\" diretory.\n"));    
}

int _tmain(int argc, TCHAR * argv[])
{    
    //TCHAR fileFolder[MAX_PATH] = { 0 };
    //TCHAR InFolder[MAX_PATH] = { 0 };
    TCHAR OutFolder[MAX_PATH] = { 0 };
    TCHAR NameOfBlParam[MAX_PATH] = { 0 };
    TCHAR AbsDirOfMain[MAX_PATH] = { 0 };
    TCHAR ParseCfgFile[MAX_PATH] = { 0 };
    TCHAR *p = NULL;
    BOOL res = FALSE; 
    //INT iSel = 0; 

    //argc = 5;// 3;
    //argv[1] = _T("0");
    //argv[2] = _T("G:\\2016-Projects\\blscript_20160819_vt3533\\Debug");
    //argv[3] = _T("BootLoader_Parameter.bin");
    //argv[4] = _T("\\FPGA");//the relative/absolute path of the main.cfg

   /* argc = 4;
    argv[1] = _T("1");
    argv[2] = _T("Bootloader_Parameter.bin");
    argv[3] = _T("parse.cfg");*/

    //show tool version
    _tprintf(_T("\n******tool version: v1.0.2******\n"));

    if (argc == 2)
    {
        if ((0 == _tcsicmp(argv[1], _T("/?")))
            || (0 == _tcsicmp(argv[1], _T("-h")))
            || (0 == _tcsicmp(argv[1], _T("-help"))))
        {
            ShowInputInfo();
            return 0;
        }
        else
        {
            _tprintf(_T("**ERROR: if want to get help info, please input the following:\n"));
            _tprintf(_T("         \"bl_parameter_compile.exe /?\" or\n"));
            _tprintf(_T("         \"bl_parameter_compile.exe -h\" or\n"));
            _tprintf(_T("         \"bl_parameter_compile.exe -help\"\n"));
            return -1;
        }
    }

    if ((argc != 4) && (argc != 5))
    {
        _tprintf(_T("**ERROR: please input three or four input parameters!\n"));
        ShowInputInfo();
        return -1;
    }

    if ((0 != _tcsicmp(argv[1], _T("0")))
        && (0 != _tcsicmp(argv[1], _T("1"))))
    {
        _tprintf(_T("ERROR**, please select 0 or 1 in the parameter_1!\n"));        
        return -1;
    }

    memset(OutFolder, 0, sizeof(TCHAR) * MAX_PATH);
    memset(NameOfBlParam, 0, sizeof(TCHAR) * MAX_PATH);
    memset(AbsDirOfMain, 0, sizeof(TCHAR) * MAX_PATH);
    memset(ParseCfgFile, 0, sizeof(TCHAR) * MAX_PATH);
    
    if (TRUE == IsAbsolutePath(argv[2]))
    {
        _tcscpy_s(g_exeFolder, MAX_PATH, argv[2]);
        _tcscpy_s(NameOfBlParam, MAX_PATH, argv[3]);
        if (0 == _tcsicmp(argv[1], _T("0")))
        {
            if (TRUE == IsAbsolutePath(argv[4]))
            {
                _tcscpy_s(AbsDirOfMain, MAX_PATH, argv[4]);
            }
            else
            {
                _stprintf_s(AbsDirOfMain, MAX_PATH, _T("%s%s"), g_exeFolder, argv[4]);
            }
        }
        else
        {
            _tcscpy_s(ParseCfgFile, MAX_PATH, argv[4]);
        }
    }
    else//root path is NULL
    {
        GetFileFolder(g_exeFolder);
        _tcscpy_s(NameOfBlParam, MAX_PATH, argv[2]); 
        if (0 == _tcsicmp(argv[1], _T("0")))
        {
            if (TRUE == IsAbsolutePath(argv[3]))
            {
                _tcscpy_s(AbsDirOfMain, MAX_PATH, argv[3]);
            }
            else
            {
                _stprintf_s(AbsDirOfMain, MAX_PATH, _T("%s%s"), g_exeFolder, argv[3]);
            }
        }
        else
        {
            _tcscpy_s(ParseCfgFile, MAX_PATH, argv[3]);
        }
    }

    //set input and output directory     
    //_stprintf_s(InFolder, _T("%s\\%s"), fileFolder, argv[3]);
    _stprintf_s(OutFolder, _T("%s\\output"), g_exeFolder);

    //check output dir whether exist
    //if not exist, new a output dir
    if (_taccess(OutFolder, 0) != 0)// not exist, new a dir
    {
        if (0 != _tmkdir(OutFolder))
        {
            _tprintf(_T("**ERROR: create %s diretory error!"), OutFolder);            
            return -1;
        }
    }

    //delete al list files in output diretory
    //to avoid compile error
    DelAllListFiles(OutFolder);

    if (0 == _tcsicmp(argv[1], _T("0")))
    {
        _tprintf(_T("\nconfig files ----> binary file:\n"));       

        //pre-compile        
        res = CollectAllConsts(AbsDirOfMain, _T("main.cfg"));
        if (TRUE == res)
        {
            _tprintf(_T("SUCCESS to scan all Consts.\n\n"));
        }
        else
        {
            _tprintf(_T("**ERROR: FAIL to scan all Consts!\n\n"));            
            return -1;
        }

        //generate list file for all cfg files
        //GenerateAllListFiles();
        res = GenerateListFiles(AbsDirOfMain, _T("main.cfg"));
        if (TRUE == res)
        {
            _tprintf(_T("SUCCESS to generate all list files.\n\n"));
        }
        else
        {
            _tprintf(_T("**ERROR: FAIL to generate all list files!\n\n"));            
            return -1;
        }

        //generate bin file for main.cfg        
        res = GenerateBinFile(OutFolder, NameOfBlParam);
        if (TRUE == res)
        {
            _tprintf(_T("SUCCESS to generate binary file.\n\n"));
        }
        else
        {
            _tprintf(_T("**ERROR: FAIL to generate binary file!\n\n"));           
            return -1;
        }
    }
    else if (0 == _tcsicmp(argv[1], _T("1")))
    {
        _tprintf(_T("\nbinary file  ----> config file:\n"));
                
        res = GenerateCfgFile(OutFolder, NameOfBlParam, ParseCfgFile);
        if (TRUE == res)
        {
            _tprintf(_T("SUCCESS to generate main_parse.cfg file.\n\n"));
        }
        else
        {
            _tprintf(_T("**ERROR: FAIL to generate main_parse.cfg file!\n\n"));            
            return -1;
        }
    }
    else
    {
        _tprintf(_T("\n**ERROR: BAT file argument is error!\n"));  
        return -1;        
    }

    //system("pause");
    return 0;
}