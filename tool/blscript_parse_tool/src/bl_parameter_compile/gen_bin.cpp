#include "main.h"
#include "gen_ptable.h"
#include "calcu_exp.h"

static FTABLE g_tFtable;

static BOOL FindInitFuncIdx(TCHAR *pPath, UINT * pIdx)
{
    UINT uiFuncIdx = 0;    
    INT lstErr;
    FILE * pLstFile;
    TCHAR line[LINESZ];
    TCHAR val_1[LEN_VALUE];
      
    if (pPath == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to path is NULL!\n"));
        return FALSE;
    }

    lstErr = _tfopen_s(&pLstFile, pPath, _T("r"));
    if (0 == lstErr)
    {
        while (NULL != _fgetts(line, sizeof(line) / sizeof(line[0]), pLstFile))
        {
            if ((_stscanf_s(line, _T("%s %d"), val_1, LEN_VALUE, &uiFuncIdx, LEN_VALUE) == 2)
                && (!_tcsicmp(val_1, _T("ftableid"))))
            {
                *pIdx = uiFuncIdx;
                fclose(pLstFile);
                return TRUE;
            }
        }

        fclose(pLstFile);
    }

    return FALSE;
}

//1ff81234 -> 3412f81f
static BOOL ReverseString(TCHAR * pStr)
{
    TCHAR * pTemp = pStr;
    UINT uiLen;
    UINT i;
    TCHAR temp[3] = { 0 };

    if (pTemp == NULL)
    {
        return FALSE;
    }

    uiLen = _tcslen(pStr);
    for (i = 0; i < (uiLen/2); i += 2)
    {        
        //_tcsncpy_s(temp, 3, (pTemp + i), 2);
        temp[0] = *(pTemp + i);
        temp[1] = *(pTemp + i + 1);
        //_tcsncpy_s((pTemp + i), 3, (pTemp + uiLen - 2 - i), 2);
        *(pTemp + i) = *(pTemp + uiLen - 2 - i);
        *(pTemp + i + 1) = *(pTemp + uiLen - 1 - i);
        //_tcsncpy_s((pTemp + uiLen - 2 - i), 3, temp, 2);
        *(pTemp + uiLen - 2 - i) = temp[0];
        *(pTemp + uiLen - 1 - i) = temp[1];
    }

    return TRUE;
}

//"1ff81234" will be 0x1ff81234
static UINT StringToUint(const TCHAR *pCh)
{
    int i, iStart;
    UINT uiData = 0;
    UINT temp;
    UINT uiRadix;
    INT iLen = _tcslen(pCh);

    uiRadix = 16;
    iStart = 0;

    temp = 1;
    for (i = (iLen - 1); i >= iStart; i--)
    {
        if ((*(pCh + i) >= _T('0')) && (*(pCh + i) <= _T('9')))
        {
            uiData = uiData + (*(pCh + i) - 48) * temp;
            temp *= uiRadix;
        }
        else if ((*(pCh + i) >= _T('a')) && (*(pCh + i) <= _T('f')) && (uiRadix == 16))
        {
            uiData = uiData + (*(pCh + i) - 97 + 10) * temp;
            temp *= uiRadix;
        }
        else if ((*(pCh + i) >= _T('A')) && (*(pCh + i) <= _T('F')) && (uiRadix == 16))
        {
            uiData = uiData + (*(pCh + i) - 65 + 10) * temp;
            temp *= uiRadix;
        }
        else
        {
            _tprintf(_T("**ERROR: %s, data is not 0-9 for 10 radix or 0-9/a-f/A-F for 16 radix!\n"), pCh);            
            break;
        }
    }

    return uiData;
}

static BOOL InsertString(TCHAR * pDstStr, const TCHAR * pSrcStr, UINT uiOffset, BOOL bIsLast)
{
    UINT uiDstLen, uiSrcLen;
    UINT i;

    if ((pDstStr == NULL) || (pSrcStr == NULL))
    {
        _tprintf(_T("**ERROR: the input string is NULL!\n"));
        return FALSE;
    }

    uiDstLen = _tcslen(pDstStr);
    uiSrcLen = _tcslen(pSrcStr);

    if (uiOffset > uiDstLen)
    {
        _tprintf(_T("**ERROR: insert offset %d is bigger than the string len %d!\n"), uiOffset, uiDstLen);
        return FALSE;
    }

    if (uiSrcLen + uiOffset > (LINESZ - 1))
    {
        _tprintf(_T("**ERROR: fail to insert string, the source string is too long!\n"));
        return FALSE;
    }

    for (i = 0; i < uiSrcLen; i++)
    {
        *(pDstStr + uiOffset + i) = *(pSrcStr + i);
    }

    if (((i + uiOffset) >= uiDstLen) || (bIsLast == TRUE))
    {
        *(pDstStr + i + uiOffset) = _T('\n');
        *(pDstStr + i + uiOffset + 1) = _T('\0');
    }

    return TRUE;
}

static BOOL BigToLittleEndian(TCHAR * pStr)
{
    TCHAR * pIndex = pStr;
    TCHAR temp[LEN_VALUE];
    UINT uiLen;
    UINT uiOp = OPCODE_NULL;
    UINT uiData = 0;
    UINT i, uiOffset;

    uiLen = _tcslen(pStr);
   
    if (pIndex == NULL)
    {
        _tprintf(_T("**ERROR: malloc fail in BigToLittleEndian() function!\n"));
        return FALSE;
    }
    
    memset(temp, 0, sizeof(TCHAR) * LEN_VALUE);

    if (_stscanf_s(pIndex, _T("%s"), temp, LEN_OPCODE) == 1)
    {
        uiOp = StringToUint(temp);
        uiLen = _tcslen(temp);        
        ReverseString(temp);
        //_tcsncpy_s(pStr, uiLen, temp, uiLen);
        if (FALSE == InsertString(pStr, temp, 0, FALSE))
        {
            return FALSE;
        }
        pIndex += uiLen;
        memset(temp, 0, sizeof(TCHAR) * LEN_VALUE);

        switch (uiOp)
        {
        case OPCODE_MEMW32:
        case OPCODE_SETBIT32:
        case OPCODE_CLRBIT32:
        case OPCODE_MWAIT32:
        case OPCODE_MEMW16:
        case OPCODE_SETBIT16:
        case OPCODE_CLRBIT16:
        case OPCODE_MWAIT16:
        case OPCODE_MEMW8:
        case OPCODE_SETBIT8:
        case OPCODE_CLRBIT8:
        case OPCODE_MWAIT8:
        case OPCODE_SETHEX:
        case OPCODE_NFC_SETFEATURE:
            pIndex++;//skip the first space
            if (_stscanf_s(pIndex, _T("%s"), temp, LEN_VALUE) == 1)
            {
                uiLen = _tcslen(temp);
                ReverseString(temp);
                //_tcsncpy_s(pStr, uiLen, temp, uiLen);
                if (FALSE == InsertString(pStr, temp, 8, FALSE))
                {
                    return FALSE;
                }
                pIndex += (uiLen + 1);
                memset(temp, 0, sizeof(TCHAR) * LEN_VALUE);

                if (_stscanf_s(pIndex, _T("%s"), temp, LEN_VALUE) == 1)
                {
                    uiLen = _tcslen(temp);
                    ReverseString(temp);
                    //_tcsncpy_s(pStr, uiLen, temp, uiLen);
                    if (FALSE == InsertString(pStr, temp, 16, TRUE))
                    {
                        return FALSE;
                    }
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
            break;
        case OPCODE_UDELAY:
        case OPCODE_MDELAY:
            pIndex++;//skip the first space
            if (_stscanf_s(pIndex, _T("%s"), temp, LEN_VALUE) == 1)
            {
                uiLen = _tcslen(temp);
                ReverseString(temp);
                //_tcsncpy_s(pStr, uiLen, temp, uiLen);
                if (FALSE == InsertString(pStr, temp, 8, TRUE))
                {
                    return FALSE;
                }               
            }
            else
            {
                return FALSE;
            }
            break;
        case OPCODE_SETSTR:
        case OPCODE_VER:
            //also should delete the first space
            //not need to reverse the string, only need to delete the space
            DelSpace(pIndex);
            uiLen = _tcslen(pIndex);
            pIndex += uiLen;
            *(pIndex) = _T('\n');
            *(pIndex + 1) = _T('\0');
            break; 
        case OPCODE_NFC_INTERFACE_INIT:
        case OPCODE_NFC_RESET:
        case OPCODE_NFC_UPDATE_PUMAPPING:
        case OPCODE_END:
            break;
        case OPCODE_VENDOR:
            pIndex++;//skip the first space
            if (_stscanf_s(pIndex, _T("%s"), temp, LEN_VALUE) == 1)
            {
                uiData = StringToUint(temp);
                uiLen = _tcslen(temp);
                ReverseString(temp);
                //_tcsncpy_s(pStr, uiLen, temp, uiLen);
                if (FALSE == InsertString(pStr, temp, 8, FALSE))
                {
                    return FALSE;
                }
                pIndex += (uiLen + 1);
                memset(temp, 0, sizeof(TCHAR) * LEN_VALUE);

                uiOffset = 12;
                i = 0;
                while (uiData > 0)
                {
                    if (_stscanf_s(pIndex, _T("%s"), temp, LEN_VALUE) == 1)
                    {
                        uiLen = _tcslen(temp);
                        ReverseString(temp);
                        //_tcsncpy_s(pStr, uiLen, temp, uiLen);
                        
                        
                        if (uiData > 1)
                        {
                            if (FALSE == InsertString(pStr, temp, uiOffset, FALSE))
                            {
                                return FALSE;
                            }
                            pIndex += (uiLen + 1);
                        }
                        else
                        {
                            if (FALSE == InsertString(pStr, temp, uiOffset, TRUE))
                            {
                                return FALSE;
                            }                            
                        }
                        memset(temp, 0, sizeof(TCHAR) * LEN_VALUE);
                        if (i == 0)
                        {
                            uiOffset += 4;
                        }
                        else
                        {
                            uiOffset += 8;
                        }

                        uiData--;
                        i++;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
            }
            else
            {
                return FALSE;
            }
            break;
        default:
            _tprintf(_T("**ERROR: opcode couldn't be identified!\n"));
            return FALSE;
            break;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }   
}

static BOOL WriteToBinFile(FILE * pFile, TCHAR * pFolder, TCHAR * pData, BOOL isInclude, UINT * pRegPos)
{
    TCHAR filePath[MAX_PATH];
    UINT uiLen;
    FILE * pLstFile;
    UINT err;
    UINT uiLineNo = 0;
    TCHAR line[LINESZ];
    BYTE *pBuf = NULL;
    UINT i;
    TCHAR data[5];
    TCHAR * pTemp;
    TCHAR * pDot;
    TCHAR val_1[LEN_VALUE];
    TCHAR val_2[LEN_VALUE];     

    if ((pFile == NULL) || (pData == NULL))
    {
        _tprintf(_T("**ERROR: the pointer to file OR data is NULL!\n"));
        return FALSE;
    }


    if (TRUE == isInclude)
    {        
        _stprintf_s(filePath, MAX_PATH, _T("%s\\%s"), pFolder, pData);
        err = _tfopen_s(&pLstFile, filePath, _T("r+"));
        if (0 == err)
        {
            while (NULL != _fgetts(line, sizeof(line) / sizeof(line[0]), pLstFile))
            {
                uiLineNo++;
                uiLen = _tcslen(line) - 1;
                if ((uiLen == 0) && (line[uiLen] == _T('\n')))
                {
                    continue;
                }

                //\r\r¿Õ¸ñ
                /*if ((line[uiLen] != _T('\n')) && (line[uiLen] != _T('\r')))
                {
                    _tprintf(_T("**ERROR: input line too long in %s (%d)\n"), filePath, uiLineNo);
                    fclose(pLstFile);
                    return FALSE;
                }*/

                if ((line[0] == _T('/')) && (line[1] == _T('/')))
                {
                    continue;
                }

                //if ((line[0] == _T('1')) && (line[1] == _T('4')))
                if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                    && (!_tcsicmp(val_1, _T("include"))))
                {
                    pTemp = _tcsrchr(val_2, _T('\\'));
                    if (NULL != pTemp) // "\dram\file.cfg"
                    {
                        pTemp++;
                    }
                    else //"file.cfg"
                    {
                        pTemp = _tcschr(val_2, _T('\"'));
                        if (NULL != pTemp)
                        {
                            pTemp++;
                        }
                        else
                        {
                            _tprintf(_T("**ERROR: line_%d include format is error!\n"), uiLineNo);
                            fclose(pLstFile);                           
                            
                            return FALSE;
                        }
                    }

                    //change .cfg to .lst
                    pDot = _tcsrchr(pTemp, _T('.'));
                    if (NULL != pDot)
                    {
                        memcpy(pDot, _T(".lst"), sizeof(_T(".lst")));
                    }
                    else
                    {
                        _tprintf(_T("**ERROR: line_%d include format is error!\n"), uiLineNo);
                        fclose(pLstFile);                        
                        return FALSE;
                    }
                    WriteToBinFile(pFile, pFolder, pTemp, TRUE, pRegPos);
                }
                else if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                    && (!_tcsicmp(val_1, _T("ftableid"))))
                {

                }
                else
                {                         
                    WriteToBinFile(pFile, pFolder, line, FALSE, pRegPos);  
                    *pRegPos += (_tcslen(line) - 1) / 2;                    
                }

            }
            
            fclose(pLstFile);             
        }
        else
        {
            _tprintf(_T("**ERROR: open %s fail!\n"), filePath);            
            return FALSE;
        }
    }
    else
    {
        pBuf = (BYTE *)malloc(BUF_SIZE * sizeof(BYTE));
        if (NULL == pBuf)
        {
            _tprintf(_T("**ERROR: malloc buffer error when generate bin file!\n"));
            return FALSE;
        }
        memset(pBuf, 0, BUF_SIZE * sizeof(BYTE));
         

        if (FALSE == BigToLittleEndian(pData))
        {
            _tprintf(_T("**ERROR: fail to transfer %s from big to little endian!\n"), pData);
            return FALSE;
        }
        uiLen = (_tcslen(pData) - 1) / 2;
        for (i = 0; i < uiLen; i++)
        {
            memset(data, 0, sizeof(data));
            _stprintf_s(data, 3, _T("%c%c"), *pData, *(pData+1));
            pData = pData + 2;

            pBuf[i] = (BYTE)_tcstoul(data, NULL, 16);            
        }

        fseek(pFile, *pRegPos, SEEK_SET);
       
        fwrite(pBuf, sizeof(BYTE), uiLen, pFile);
        free(pBuf);
    }

    return TRUE;
}

static BOOL SetFtable(TCHAR *pPath, INITFUNC_INFO * ptFuncInfo)
{
    UINT uiFuncIdx;
    BOOL bIsInitFunc = FALSE;        
    INT lstErr;
    FILE * pLstFile; 
    TCHAR line[LINESZ];    
    TCHAR val_1[LEN_VALUE];    
    UINT uiLen;    
    BOOL bIsEmptyLine;
    UINT i;
    BOOL bIsMultiple = 0;

    if (pPath == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to path is NULL!\n"));
        return FALSE;
    }

    lstErr = _tfopen_s(&pLstFile, pPath, _T("r"));
    if (0 == lstErr)
    {
        //search ftableid in list file
        while (NULL != _fgetts(line, sizeof(line) / sizeof(line[0]), pLstFile))
        {
            if ((_stscanf_s(line, _T("%s %d %d"), val_1, LEN_VALUE, &uiFuncIdx, &bIsMultiple) >= 2)
                && (!_tcsicmp(val_1, _T("ftableid"))))
            {
                if (uiFuncIdx >= FTABLE_FUNC_TOTAL)
                {
                    _tprintf(_T("**ERROR: ftableid is too big!\n"));                    
                    fclose(pLstFile);
                    return FALSE;
                } 
                
                //check if have multiple config file
                if (bIsMultiple != 0)
                {
                    ptFuncInfo[uiFuncIdx].bIsMultiple = TRUE;
                }
                   

                if ((ptFuncInfo[uiFuncIdx].bHasSet == TRUE) && (ptFuncInfo[uiFuncIdx].bIsMultiple == FALSE))
                {
                    _tprintf(_T("**ERROR: ftableid %d has been re-set!\n"), uiFuncIdx);                    
                    fclose(pLstFile);
                    return FALSE;
                }

                bIsInitFunc = TRUE;

                //if have multiple config file for the same function index, should accumulate
                if (ptFuncInfo[uiFuncIdx].bHasSet != TRUE)
                {
                    ptFuncInfo[uiFuncIdx].uiInitFuncIdx = uiFuncIdx;
                    ptFuncInfo[uiFuncIdx].uiTotalByte = 0;
                    ptFuncInfo[uiFuncIdx].uiRegCnt = 0;
                }
                ptFuncInfo[uiFuncIdx].bHasSet = TRUE;

                break;
            }
        }
         
        //calculate the total bytes and regcnt for the current init func.
        if (bIsInitFunc)
        {
            fseek(pLstFile, 0, SEEK_SET);
            while (NULL != _fgetts(line, sizeof(line) / sizeof(line[0]), pLstFile))
            {                  
                uiLen = _tcslen(line);
                if (uiLen == 0)//empty line with nothing
                {
                    continue;
                }
               
                //empty line but len is not 0
                //include "\n", " \n", "\r\r "
                bIsEmptyLine = TRUE;
                for (i = 0; i < uiLen; i++)
                {
                    if ((line[i] != _T(' ')) && (line[i] != _T('\n')) && (line[i] != _T('\r')))
                    {
                        bIsEmptyLine = FALSE;
                        break;
                    }
                }
                if (bIsEmptyLine == TRUE)
                {
                    continue;
                }

                if ((line[0] == _T('/')) && (line[1] == _T('/')))
                {
                    continue;
                }

                if ((_stscanf_s(line, _T("%s"), val_1, LEN_VALUE) == 1)
                    && (!_tcsicmp(val_1, _T("include"))))
                {
                    continue;
                }
                else if ((_stscanf_s(line, _T("%s"), val_1, LEN_VALUE) == 1)
                    && (!_tcsicmp(val_1, _T("ftableid"))))
                {
                    continue;
                }
                else
                {
                    //must delete space, \t, \n, or calculate length will be error
                    DelSpace(line);
                    uiLen = _tcslen(line);//not include '\n'
                    ptFuncInfo[uiFuncIdx].uiTotalByte += uiLen/ 2; //(uiLen - 1) / 2;
                    ptFuncInfo[uiFuncIdx].uiRegCnt++;
                }
            }
        }       

        fclose(pLstFile);
    }
    else
    {
        _tprintf(_T("**ERROR: couldn't open '%s'!\n"));
        return FALSE;
    }    

    return TRUE;
}

static BOOL ScanFtidAndSetFt(TCHAR * pFolder, FILE * pBinFile, UINT uiOffset)
{
    TCHAR findPath[MAX_PATH] = { 0 };
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile;
    BOOL res = TRUE;
    TCHAR *p = NULL;
    INITFUNC_INFO tFuncInfo[FTABLE_FUNC_TOTAL];
    UINT uiMaxId = 0;
    UINT i, j, ulRegPos;
    UINT ulTotalSize = 0;;

    if (pFolder == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to folder is NULL!\n"));
        return FALSE;
    }

    memset(&tFuncInfo, 0, sizeof(INITFUNC_INFO)*FTABLE_FUNC_TOTAL);    
        
    _stprintf_s(findPath, MAX_PATH, _T("%s\\*.lst"), pFolder);

    hFile = FindFirstFile(findPath, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        _tprintf(_T("**ERROR: fine no *.lst file in %s folder!\n"), pFolder);
        res = FALSE;
        return res;
    }

    do
    {
        if (FALSE == (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            p = _tcsrchr(FindFileData.cFileName, _T('.'));
            if (p == NULL)
            {
                continue;
            }
            if (0 != _tcsicmp(p, _T(".lst")))
            {
                continue;
            }

            _stprintf_s(findPath, MAX_PATH, _T("%s\\%s"), pFolder, FindFileData.cFileName);
            res = SetFtable(findPath, tFuncInfo);
            if (FALSE == res)
            {
                _tprintf(_T("*ERROR: scan %s file error, stop!\n"), FindFileData.cFileName);                
                break;
            }
        }
    } while (FindNextFile(hFile, &FindFileData));

    FindClose(hFile);

    //calculate the max ftable id
    for (i = 0; i < FTABLE_FUNC_TOTAL; i++)
    {
        if (tFuncInfo[i].uiInitFuncIdx > uiMaxId)
        {
            uiMaxId = i;
        }
    }

    //calculate regpos for the current init func
    for (i = 0; i <= uiMaxId; i++)
    {
        g_tFtable.aInitFuncEntry[i].ulRegCnt = tFuncInfo[i].uiTotalByte;// tFuncInfo[i].uiRegCnt;

        ulRegPos = 0;
        for (j = 0; j < i; j++)
        {
            ulRegPos += tFuncInfo[j].uiTotalByte;
        }
        g_tFtable.aInitFuncEntry[i].ulRegPos = ulRegPos;

        ulTotalSize += tFuncInfo[i].uiTotalByte;
    }

    //check FTABLE + PTABLE + REG_LIST <= 4k
    ulTotalSize += FTABLE_SIZE + PTABLE_SIZE;
    if (ulTotalSize > MAX_PARAMETER_LEN)
    {
        _tprintf(_T("**ERROR: Ftable + Ptable + Reg_List is too long(Max is 4096 byte, but now is %d byte!\n)"), ulTotalSize);        
        res = FALSE;
        return res;
    }

    fseek(pBinFile, uiOffset, SEEK_SET);
    fwrite(&g_tFtable, sizeof(FTABLE), 1, pBinFile);

    return res;
}

BOOL GenerateBinFile(TCHAR *pFolder, TCHAR * pBinFileName)
{
    TCHAR lstPath[MAX_PATH];
    TCHAR binPath[MAX_PATH];
    //TCHAR binFileName[MAX_PATH];
    INT lstErr, binErr;
    FILE * pLstFile;
    FILE * pBinFile;    
    UINT uiLen;
    TCHAR line[LINESZ];
    UINT uiLineNo = 0;
    TCHAR val_1[LEN_VALUE];
    TCHAR val_2[LEN_VALUE];    
    TCHAR * pTemp = NULL;
    TCHAR * pDot = NULL;
    INT i;
    UINT uiOffset, uiInc = 0;
    BOOL res = FALSE;  
    U8 * pParam = NULL;
    UINT uiFuncIdx;
    WRITEFUNC_INFO tWtInfo[FTABLE_FUNC_TOTAL];

    if ((pFolder == NULL) || (pBinFileName == NULL))
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    memset(tWtInfo, 0, sizeof(WRITEFUNC_INFO) * FTABLE_FUNC_TOTAL);

    _tprintf(_T("--begin to generate bin file:\n"));
            
    _stprintf_s(lstPath, MAX_PATH, _T("%s\\main.lst"), pFolder);    
    _stprintf_s(binPath, MAX_PATH, _T("%s\\%s"), pFolder, pBinFileName);
   
    lstErr = _tfopen_s(&pLstFile, lstPath, _T("r"));
    binErr = _tfopen_s(&pBinFile, binPath, _T("wb+"));

    if ((0 == lstErr) && (0 == binErr))
    {  
        //write ftable into bin file
        memset(&g_tFtable, 0, sizeof(FTABLE));
        uiOffset = 0;        
        res = ScanFtidAndSetFt(pFolder, pBinFile, uiOffset);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: scan ftable id or set ftable error!\n"));            
            fclose(pLstFile);
            fclose(pBinFile);
            return res;
        }        

        //write ptable into bin file;  
        uiOffset = FTABLE_SIZE;
        SetPtable(pBinFile, uiOffset);                    

        //write reg_list into bin file
        uiOffset = FTABLE_SIZE + PTABLE_SIZE;
        //clear (4096-256-512)byte REG_LIST in bootloader.bin before write
        pParam = (U8 *)malloc(sizeof(U8) * (MAX_PARAMETER_LEN - FTABLE_SIZE - PTABLE_SIZE));
        memset(pParam, 0, (sizeof(U8) * (MAX_PARAMETER_LEN - FTABLE_SIZE - PTABLE_SIZE)));
        fseek(pBinFile, uiOffset, SEEK_SET);
        fwrite(pParam, sizeof(BYTE), (MAX_PARAMETER_LEN - FTABLE_SIZE - PTABLE_SIZE), pBinFile);
        free(pParam);

        while (NULL != _fgetts(line, sizeof(line) / sizeof(line[0]), pLstFile))
        {
            uiLineNo++;
            uiLen = _tcslen(line) - 1;
            if ((uiLen == 0) && (line[uiLen] == _T('\n')))
            {
                continue;
            }

            if (line[uiLen] != _T('\n'))
            {
                _tprintf(_T("**ERROR: input line too long in %s (%d)\n"), lstPath, uiLineNo);
                //fclose(pLstFile);
                //fclose(pBinFile);                
                res = FALSE;  
                break;
            }

            if ((line[0] == _T('/')) && (line[1] == _T('/')))
            {
                continue;
            }

            //if ((line[0] == _T('1')) && (line[1] == _T('4')))
            if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                && (!_tcsicmp(val_1, _T("include"))))
            {                
                pTemp = _tcsrchr(val_2, _T('\\'));
                if (NULL != pTemp) // "\dram\file.cfg"
                {
                    pTemp++;
                }
                else //"file.cfg"
                {
                    pTemp = _tcschr(val_2, _T('\"'));
                    if (NULL != pTemp)
                    {
                        pTemp++;
                    }
                    else
                    {
                        _tprintf(_T("**ERROR: line_%d include format is error!\n"), uiLineNo);
                        //fclose(pLstFile);
                        //fclose(pBinFile);                        
                        res = FALSE;
                        break;
                    }
                }

                pDot = _tcsrchr(pTemp, _T('.'));
                if (NULL != pDot)
                {
                    memcpy(pDot, _T(".lst"), sizeof(_T(".lst")));
                }
                else
                {
                    _tprintf(_T("**ERROR: line_%d include format is error!\n"), uiLineNo);
                    //fclose(pLstFile);
                    //fclose(pBinFile);                    
                    res = FALSE;
                    break;
                }
                
                _stprintf_s(lstPath, MAX_PATH, _T("%s\\%s"), pFolder, pTemp);
                if (TRUE == FindInitFuncIdx(lstPath, &uiFuncIdx))
                {
                    tWtInfo[uiFuncIdx].uiFuncIdx = uiFuncIdx;
                    if (FALSE == tWtInfo[uiFuncIdx].bHasWrite)
                    {                        
                        tWtInfo[uiFuncIdx].uiCurOffset = FTABLE_SIZE + PTABLE_SIZE + g_tFtable.aInitFuncEntry[uiFuncIdx].ulRegPos;
                    }                    
                    tWtInfo[uiFuncIdx].bHasWrite = TRUE;
                }
                else
                {
                    //uiOffset = uiRegPos;
                    //if include file don't have ftable_id, shouldn't write it into REG_LIST
                    _tprintf(_T("**WARNING: \"%s doesn't have ftableid, so will not write it into REG_LIST!\n"), pTemp);
                    res = TRUE;
                    continue;
                }
               
                res = WriteToBinFile(pBinFile, pFolder, pTemp, TRUE, &(tWtInfo[uiFuncIdx].uiCurOffset));
            }
            else if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                && (!_tcsicmp(val_1, _T("ftableid"))))
            {

            }
            else
            {      
                //error, main.cfg should not have opcodes ???
                //where I put opcodes in main.cfg?
                //after REG_LIST
                UINT uiLstIdx = 0;
                UINT uiMaxPos = 0;
                UINT uiTotalSize;
                                
                for (i = 0; i < FTABLE_FUNC_TOTAL; i++)
                {
                    if (g_tFtable.aInitFuncEntry[i].ulRegPos >= uiMaxPos)
                    {
                        uiMaxPos = g_tFtable.aInitFuncEntry[i].ulRegPos;
                        uiLstIdx = i;
                    }
                }
                uiOffset = FTABLE_SIZE + PTABLE_SIZE + uiMaxPos + g_tFtable.aInitFuncEntry[uiLstIdx].ulRegCnt + uiInc;
                
                uiTotalSize = (uiOffset + ((_tcslen(line) - 1) / 2));
                if (uiTotalSize > MAX_PARAMETER_LEN)
                {
                    _tprintf(_T("**ERROR: Ftable + Ptable + Reg_List + opcode in main.cfg is too long(Max is 4096 byte, but now is %d byte!\n)"), uiTotalSize);
                    res = FALSE;
                    break;
                }
                
                res = WriteToBinFile(pBinFile, pFolder, line, FALSE, &uiOffset);
                uiInc += (_tcslen(line) - 1) / 2;
            }
        }
        
        fclose(pLstFile);
        fclose(pBinFile);
        if (TRUE == res)
        {
            _tprintf(_T("--end to generate bin file.\n"));
        }
        else
        {
            _tprintf(_T("--fail to generate bin file.\n"));
        }
    }
    else
    {
        _tprintf(_T("--fail to open list or bin file!\n"));
    }
    
    return res;    
}