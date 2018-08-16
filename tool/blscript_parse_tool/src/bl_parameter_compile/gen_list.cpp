#include "main.h"
#include "calcu_exp.h"
#include "gen_ptable.h"

//#define COMPARE 

extern TCHAR g_exeFolder[MAX_PATH];

static CONSTANT_VAL g_tConst[MAX_CONST] = { 0 };
static UINT g_uiNumOfConst = 0;
static UINT g_uiStrId[MAX_STRID] = { 0 };
static UINT g_uiHexId[MAX_HEXID] = { 0 };
static UINT g_uiNumOfStrId = 0;
static UINT g_uiNumOfHexId = 0;

static BOOL GetOpcodeType(TCHAR * str, OPCODE_TYPE * pOpcode)
{
    OPCODE_TYPE_DESC *pOp = OPCODE_TYPE_LIB;

    while (_tcsicmp(pOp->pDesc, _T("")))
    {
        if (!_tcsicmp(pOp->pDesc, str))
        {
            *pOpcode = pOp->eOpcodeType;
            return TRUE;
        }
        pOp++;
    }

    return FALSE;
}

static BOOL GetDirectiveType(TCHAR * str, DIRECTIVE_TYPE * pType)
{
    DIRECTIVE_TYPE_DESC *pOp = DIRECTIVE_TYPE_LIB;

    while (_tcsicmp(pOp->pDesc, _T("")))
    {
        if (!_tcsicmp(pOp->pDesc, str))
        {
            *pType = pOp->eDirectType;
            return TRUE;
        }
        pOp++;
    }

    return FALSE;
}

static BOOL UiToString(UINT uiData, UINT uiRadix, TCHAR * pStr, UINT uiStrLen)
{
    const TCHAR string[] = _T("0123456789ABCDEF");
    UINT i, j;
    UINT uiLen;
    TCHAR temp;

    if (pStr == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to the string is NULL!\n"));
        return FALSE;
    }

    i = 0;
    while (uiData > 0)
    {
        *(pStr + i) = string[uiData % uiRadix];
        uiData /= uiRadix;
        i++;
    }

    if (i < uiStrLen)
    {
        for (j = i; j < uiStrLen; j++)
        {
            *(pStr + j) = _T('0');
        }
    }
    *(pStr + uiStrLen) = _T('\0');

    //reverse the string, 12345678 -> 21436587, little endian
    /*uiLen = _tcslen(pStr);
    for (i = 0; i < uiLen; i = i + 2)
    {
        temp = *(pStr + i);
        *(pStr + i) = *(pStr + i + 1);
        *(pStr + i + 1) = temp;
    }*/

    //reverse the string, 12345678 -> 87654321
    //0x1ff80068 should be "1ff80068" in list file
    uiLen = _tcslen(pStr);
    for (i = 0; i < (uiLen/2); i++)
    {
        temp = *(pStr + i);
        *(pStr + i) = *(pStr + uiLen - 1 - i);
        *(pStr + uiLen - 1 - i) = temp;
    }

    return TRUE;
}

static void WriteToLstfile(TCHAR * pBuf, UINT uiStrcnt, FILE * pLstfile)
{
    LINE_COMMAND *pLine = NULL;
    OPCODE_TYPE eType = OPCODE_NULL;
    UINT i;
    //STR_TYPE eStrType = STR_NULL;
    //INT iData;
    UINT uiNumByte;
    TCHAR opStr[LEN_OPCODE];

    if ((pBuf == NULL) || (pLstfile == NULL))
    {
        _tprintf(_T("**ERROR: the pointer to buffer OR FILE is NULL!\n"));
        return;
    }

    if (uiStrcnt != 0)
    {
        pLine = (LINE_COMMAND *)pBuf;
        GetOpcodeType(pLine->type, &eType);
    }

    //write opcode as 4-BYTE format, little endian
    memset(opStr, 0, sizeof(opStr));
    UiToString(eType, 16, opStr, 8);

    if (uiStrcnt == 0)
    {
        //for comments, already have "\n", not need to add
        _ftprintf(pLstfile, _T("%s"), pBuf);
    }
    else
    {
        if (eType == OPCODE_VENDOR)
        {
            _ftprintf(pLstfile, _T("%s"), opStr);
            //write uiNumOfValue as 2-BYTE alginment, little endian
            memset(opStr, 0, sizeof(opStr));
            UiToString(pLine->uiNumOfValue, 16, opStr, 4);
            _ftprintf(pLstfile, _T(" %s"), opStr);            
            for (i = 0; i < pLine->uiNumOfValue; i++)
            {
                _ftprintf(pLstfile, _T(" %s"), (pLine->pValue + i)->value);
            }

            _ftprintf(pLstfile, _T("\n"));
        }
        else
        {
            if (eType == OPCODE_VER)
            {
                uiNumByte = 0;
                _ftprintf(pLstfile, _T("%s"), opStr);
                uiNumByte += 4;
                _ftprintf(pLstfile, _T(" %02x "), _tcslen((pLine->pValue)->value));
                uiNumByte += 1;
                for (i = 0; i < _tcslen((pLine->pValue)->value); i++)
                {
                    _ftprintf(pLstfile, _T("%02x"), (pLine->pValue)->value[i]);
                    uiNumByte += 1;
                }

                //patch for 4-BYTE alignment
                if (uiNumByte % 4)
                {
                    for (i = 0; i < (4 - (uiNumByte % 4)); i++)
                    {
                        _ftprintf(pLstfile, _T("%02x"), 0);
                    }
                }

                _ftprintf(pLstfile, _T("\n"));
            }
            else if (eType == OPCODE_SETSTR)
            {
                uiNumByte = 0;
                _ftprintf(pLstfile, _T("%s"), opStr);
                uiNumByte += 4;
                _ftprintf(pLstfile, _T(" %s"), (pLine->pValue)->value);
                uiNumByte += 1;
                _ftprintf(pLstfile, _T( " %02x "), _tcslen((pLine->pValue + 1)->value));
                uiNumByte += 1;
                for (i = 0; i < _tcslen((pLine->pValue + 1)->value); i++)
                {
                    _ftprintf(pLstfile, _T("%02x"), (pLine->pValue + 1)->value[i]);
                    uiNumByte += 1;
                }

                //patch for 4-BYTE alignment
                if (uiNumByte % 4)
                {
                    for (i = 0; i < (4 - (uiNumByte % 4)); i++)
                    {
                        _ftprintf(pLstfile, _T("%02x"), 0);
                    }
                }

                _ftprintf(pLstfile, _T("\n"));
            }
            else
            {
                _ftprintf(pLstfile, _T("%s"), opStr);

                for (i = 0; i < pLine->uiNumOfValue; i++)
                {
                    _ftprintf(pLstfile, _T(" %s"), (pLine->pValue + i)->value);
                }
                _ftprintf(pLstfile, _T("\n"));
            }            
        }
    }
}

//check whether have const letter that need to be replaced
//a~f/A~F may be hex data
//but just like ABC may be defined as const string, how to deal?
static BOOL IsHaveLetter(TCHAR * pCh)
{
    while (*pCh != _T('\0'))
    {

        if (((*pCh >= _T('g')) && (*pCh <= _T('z')))
            || ((*pCh >= _T('G')) && (*pCh <= _T('Z'))))

        {
            return TRUE;
        }  
        pCh++;
    }
    
    return FALSE;
}

static void ReplaceConst(TCHAR * pVal)
{
    UINT i;
    TCHAR * p = NULL;
    TCHAR reststr[LEN_VALUE];
    UINT uiLen;

    if (pVal == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return;
    }

    memset(reststr, 0, LEN_VALUE * sizeof(TCHAR));

    for (i = 0; i < g_uiNumOfConst; i++)
    {
        /*if (0 == _tcsicmp(g_tConst[i].const_name, pVal))
        {
            memcpy(pVal, g_tConst[i].const_val, sizeof(g_tConst[i].const_val));
            break;
        }*/
        //maybe part of exp is define micro
        p = _tcsstr(pVal, g_tConst[i].const_name);
        if (p != NULL)
        {
            uiLen = _tcslen(g_tConst[i].const_name);   
            //to avoid PTABLE_DW30/PTABLE_DW31.../PTALBE_DW39 be replaced with the value of PTABLE_DW3
            if (((*(p + uiLen) >= _T('a')) && (*(p + uiLen) <= _T('z')))
                || ((*(p + uiLen) >= _T('A')) && (*(p + uiLen) <= _T('Z')))
                || ((*(p + uiLen) >= _T('0')) && (*(p + uiLen) <= _T('9')))
                || ((*(p + uiLen)) == _T('_')))
            {
                continue;
            }
            _tcscpy_s(reststr, LEN_VALUE, (p + uiLen));            

            _tcscpy_s(p, (LEN_VALUE - (p - pVal)), g_tConst[i].const_val);            
            _tcscat_s(pVal, LEN_VALUE, reststr);

            //check whether have other const
            //if have, continue to replace the const
            //else, exit
            if (FALSE == IsHaveLetter(pVal))
            {
                break;
            }           
        }
    }

    return;
}

static BOOL IsHaveConst(TCHAR * pConst)
{
    UINT i;
    BOOL res = FALSE;

    if (pConst == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    for (i = 0; i < g_uiNumOfConst; i++)
    {
        if (0 == _tcsicmp(g_tConst[i].const_name, pConst))
        {
            res = TRUE;
            break;
        }
    }

    return res;
}

static BOOL IsHaveStrId(UINT uiId)
{
    UINT i;
    BOOL res = FALSE;

    for (i = 0; i < g_uiNumOfStrId; i++)
    {
        if (uiId == g_uiStrId[i])
        {
            res = TRUE;
            break;
        }
    }

    return res;
}

static BOOL IsHaveHexId(UINT uiId)
{
    UINT i;
    BOOL res = FALSE;

    for (i = 0; i < g_uiNumOfHexId; i++)
    {
        if (uiId == g_uiHexId[i])
        {
            res = TRUE;
            break;
        }
    }

    return res;
}

//in addition vendorop, calculate the number of value in one line
//use the space as seperator for one value
//opcode is not a value, so how much space, how much value
static U8 GetNumOfValue(TCHAR * line)
{
    UINT uiSpaceNo = 0;    
    TCHAR * p = NULL;

    p = line;

    while (*p != _T('\0'))
    {
        //space only in the middle of the string
        //the spaces before and after the string and have been deleted
        if (((*p == _T(' ')) || (*p == _T('\t')))
            && ((*(p + 1) != _T(' ')) && (*(p + 1) != _T('\t'))))
        {
            uiSpaceNo++;            
        }

        p++;
    }

    return uiSpaceNo;
}

//use the space as seperator for one param
//uiIdx: the first value is after opcode, uiIdx = 1
static TCHAR * GetOneValue(const TCHAR *line, UINT uiIdx, TCHAR * pParam)
{
    UINT uiSpaceNo = 0;
    UINT i;
    const TCHAR * p = NULL;
    UINT uiLen;

    p = line;
    uiLen = _tcslen(line);

    while (*p != _T('\0'))
    {
        //space only in the middle of the string
        //the spaces before and after the string and have been deleted
        if (((*p == _T(' ')) || (*p == _T('\t')))
            && ((*(p + 1) != _T(' ')) && (*(p + 1) != _T('\t'))))
        {
            uiSpaceNo++;

            //copy string until space
            if (uiSpaceNo == uiIdx)
            {
                //p point to the space
                //p+1 point to the string
                for (i = 1; ((*(p + i) != _T(' ')) && (*(p + i) != _T('\t')) && (*(p + i) != _T('\n')) && (*(p + i) != _T('\r')) && (i < uiLen)); i++)
                {
                    *(pParam + i - 1) = *(p + i);

                }
                *(pParam + i) = _T('\0');

                return pParam;
            }
        }

        p++;
    }

    return NULL;

}

static BOOL ParseLine(TCHAR * line, LINE_COMMAND * ptLinecmd, UINT8 * pIsDirective, UINT uiLineNo)
{
    //size_t uiLen;
    OPCODE_TYPE eOpcode = OPCODE_NULL;
    DIRECTIVE_TYPE eDirective = DIRECTIVE_NULL;
    BOOL sts = FALSE;
    TCHAR aNumOfParam[3];
    UINT i;
    TCHAR aValue[LEN_VALUE];
   
    //uiLen = _tcslen(line);

    if (_stscanf_s(line, _T("%s"), ptLinecmd->type, LEN_OPCODE) == 1)
    {
        if ((FALSE == GetOpcodeType(ptLinecmd->type, &eOpcode))
            && (FALSE == GetDirectiveType(ptLinecmd->type, &eDirective)))
        {
            sts = FALSE;
            return sts;
        }

        if (eOpcode == OPCODE_VENDOR)
        {
            if (_stscanf_s(line, _T("%s %s"), ptLinecmd->type, LEN_OPCODE, aNumOfParam, 3) == 2)
            {
                ptLinecmd->uiNumOfValue = (U8)ConvertValue(aNumOfParam, uiLineNo);
                ptLinecmd->pValue = (LINE_PARAMETER *)malloc(ptLinecmd->uiNumOfValue * LEN_VALUE * sizeof(TCHAR));

                for (i = 0; i < ptLinecmd->uiNumOfValue; i++)
                {
                    memset(aValue, 0, sizeof(aValue));
                    if (NULL == GetOneValue(line, (i + 2), aValue))
                    {
                        _tprintf(_T("**ERROR: vendor define opcode not have the %d parameter!\n"), i);
                        sts = FALSE;
                        return sts;
                    }
                    _tcscpy_s((ptLinecmd->pValue + i)->value, LEN_VALUE, aValue);

                    ReplaceConst((ptLinecmd->pValue + i)->value);
                }

                sts = TRUE;
            }
        }
        else
        {
            ptLinecmd->uiNumOfValue = GetNumOfValue(line);
            ptLinecmd->pValue = (LINE_PARAMETER *)malloc(ptLinecmd->uiNumOfValue * LEN_VALUE* sizeof(TCHAR));

            for (i = 0; i < ptLinecmd->uiNumOfValue; i++)
            {
                memset(aValue, 0, sizeof(aValue));
                if (NULL == GetOneValue(line, (i + 1), aValue))
                {
                    break;
                }
                                
                _tcscpy_s((ptLinecmd->pValue + i)->value, LEN_VALUE, aValue);

                if (eOpcode != OPCODE_NULL)
                {
                    ReplaceConst((ptLinecmd->pValue + i)->value);
                }
            }

            sts = TRUE;
        }
    }
   
    if (eDirective != DIRECTIVE_NULL)
    {
        *pIsDirective = TRUE;
    }
    else
    {
        *pIsDirective = FALSE;
    }

    return sts;
}

static BOOL CheckIntValue(UINT uiValue, UINT uiNumOfByte)
{
    if (uiNumOfByte == 1)
    {
        if ((uiValue >= 0) && (uiValue <= 0xFF))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (uiNumOfByte == 2)
    {
        if ((uiValue >= 0) && (uiValue <= 0xFFFF))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (uiNumOfByte == 4)
    {
        if ((uiValue >= 0) && (uiValue <= 0xFFFFFFFF))
        {
            return TRUE;
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
}

static BOOL CheckVerDec(UINT8 data)
{
    if ((data >= 0) && (data <= 99))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

static BOOL CheckVerChar(TCHAR c)
{
    if ((c >= _T('a')) && (c <= _T('z')))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static BOOL CheckOpAndWriteToLfile(UINT line, LINE_COMMAND * ptLinecmd, FILE * pLstfile)
{
    UINT uiVal_1, uiVal_2;
    OPCODE_TYPE eOptype = OPCODE_NULL;
    //STR_TYPE eStrtype = STR_NULL;
    BOOL sts = FALSE;
    VER_DESC tVer;
    //UINT uiLen;
    //UINT uiId;
    UINT uiLen, i, j;
    TCHAR temp[LEN_VALUE] = { 0 };

    if (ptLinecmd == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    if (FALSE == GetOpcodeType(ptLinecmd->type, &eOptype))
    {
        _tprintf(_T("**ERROR: line_%d, couldn't find the opcode!\n"), line);        
        return FALSE;
    }       

    switch (eOptype)
    {
    case OPCODE_MEMW32:
    case OPCODE_SETBIT32:
    case OPCODE_CLRBIT32:
    case OPCODE_MWAIT32:
    case OPCODE_NFC_SETFEATURE:
        //uiVal_1 = _tcstoul(ptLinecmd->value_1, NULL, 16);        
        //uiVal_2 = _tcstoul(ptLinecmd->value_2, NULL, 16);
        uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
        uiVal_2 = GetOperateResult((ptLinecmd->pValue + 1)->value, line);
        if ((FALSE == CheckIntValue(uiVal_1, 4)) || (FALSE == CheckIntValue(uiVal_2, 4))
            || (ptLinecmd->uiNumOfValue > 2))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);            
            return FALSE;
        }

        memset((ptLinecmd->pValue)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        memset((ptLinecmd->pValue + 1)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 8);
        UiToString(uiVal_2, 16, (ptLinecmd->pValue + 1)->value, 8);

        WriteToLstfile((TCHAR *)ptLinecmd, 3, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_MEMW16:
    case OPCODE_SETBIT16:
    case OPCODE_CLRBIT16:
    case OPCODE_MWAIT16:
        //uiVal_1 = _tcstoul(ptLinecmd->value_1, NULL, 16);
        //uiVal_2 = _tcstoul(ptLinecmd->value_2, NULL, 16);
        uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
        uiVal_2 = GetOperateResult((ptLinecmd->pValue + 1)->value, line);
        if ((FALSE == CheckIntValue(uiVal_1, 4)) || (FALSE == CheckIntValue(uiVal_2, 2))
            || (ptLinecmd->uiNumOfValue > 2))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);            
            return FALSE;
        }

        memset((ptLinecmd->pValue)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        memset((ptLinecmd->pValue + 1)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 8);
        UiToString(uiVal_2, 16, (ptLinecmd->pValue + 1)->value, 8);

        WriteToLstfile((TCHAR *)ptLinecmd, 3, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_MEMW8:
    case OPCODE_SETBIT8:
    case OPCODE_CLRBIT8:
    case OPCODE_MWAIT8:
        //uiVal_1 = _tcstoul(ptLinecmd->value_1, NULL, 16);
        //uiVal_2 = _tcstoul(ptLinecmd->value_2, NULL, 16);
        uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
        uiVal_2 = GetOperateResult((ptLinecmd->pValue + 1)->value, line);
        if ((FALSE == CheckIntValue(uiVal_1, 4)) || (FALSE == CheckIntValue(uiVal_2, 1))
            || (ptLinecmd->uiNumOfValue > 2))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);            
            return FALSE;
        }

        memset((ptLinecmd->pValue)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        memset((ptLinecmd->pValue + 1)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 8);
        UiToString(uiVal_2, 16, (ptLinecmd->pValue + 1)->value, 8);

        WriteToLstfile((TCHAR *)ptLinecmd, 3, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_UDELAY:
    case OPCODE_MDELAY:
        //uiVal_1 = _tcstoul(ptLinecmd->value_1, NULL, 10);
        uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
        if ((FALSE == CheckIntValue(uiVal_1, 4)) || (ptLinecmd->uiNumOfValue > 1))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);            
            return FALSE;
        }

        memset((ptLinecmd->pValue)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 8);

        WriteToLstfile((TCHAR *)ptLinecmd, 2, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_SETSTR:
        if ((ptLinecmd->pValue)->value != _T(""))
            //&&(1 == _stscanf_s(ptLinecmd->value_1, _T("%d"), &uiId)))// dec or hex?
        {
            uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);

            if ((FALSE == CheckIntValue(uiVal_1, 1))
                || (FALSE == CheckIntValue(_tcslen((ptLinecmd->pValue + 1)->value), 1)))
            {
                _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);               
                return FALSE;
            }
        }
        else
        {
            _tprintf(_T("**ERROR: line_%d, define %s is empty!\n"), line, (ptLinecmd->pValue)->value);            
            return FALSE;
        }

        //to be sure not access g_uiStrId[MAX_STRID] 
        if (g_uiNumOfStrId >= MAX_STRID)
        {
            _tprintf(_T("**ERROR: line_%d, string IDs are more than %d!\n"), line, MAX_STRID);            
            return FALSE;
        }

        if (TRUE == IsHaveStrId(uiVal_1))
        {
            _tprintf(_T("**ERROR: line_%d, SETSTR/SETHEX/SETDEC for StrId_%d has already existed!\n"), line, uiVal_1);            
            return FALSE;
        }

        g_uiStrId[g_uiNumOfStrId] = uiVal_1;
        g_uiNumOfStrId++;

        //deal with string, should use " "
        uiLen = _tcslen((ptLinecmd->pValue + 1)->value);
        if (((ptLinecmd->pValue + 1)->value[0] != _T('\"'))
            || ((ptLinecmd->pValue + 1)->value[uiLen - 1] != _T('\"')))
        {
            _tprintf(_T("**ERROR: line_%d, string should be within double quotations!\n"), line);            
            return FALSE;
        }
        //\" within double quotations should be written in bin file
        _tcscpy_s(temp, uiLen, ((ptLinecmd->pValue + 1)->value + 1));
        temp[uiLen - 2] = _T('\0');
        memset((ptLinecmd->pValue + 1)->value, 0, LEN_VALUE * sizeof(TCHAR));
        for (i = 0, j = 0; i < uiLen, j < uiLen; i++)
        {
            if (temp[i] != _T('\\'))
            {
                (ptLinecmd->pValue + 1)->value[j] = temp[i];
                j++;
            }
        }

        memset((ptLinecmd->pValue)->value, 0, LEN_VALUE * sizeof(TCHAR));
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 2);
        WriteToLstfile((TCHAR *)ptLinecmd, 3, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_SETHEX:
        //case OPCODE_SETDEC:
        if ((ptLinecmd->pValue)->value != _T(""))
            // && (1 == _stscanf_s(ptLinecmd->value_1, _T("%d"), &uiId)))// dec or hex?
        {
            uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
            uiVal_2 = GetOperateResult((ptLinecmd->pValue + 1)->value, line);

            if ((FALSE == CheckIntValue(uiVal_1, 4))
                || (FALSE == CheckIntValue(uiVal_2, 4)))
            {
                _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);                
                return FALSE;
            }
        }
        else
        {
            _tprintf(_T("**ERROR: line_%d, define %s is empty!\n"), line, (ptLinecmd->pValue)->value);            
            return FALSE;
        }

        //to be sure not access g_uiHexId[MAX_STRID] 
        if (g_uiNumOfHexId >= MAX_STRID)
        {
            _tprintf(_T("**ERROR: line_%d, string IDs are more than %d!\n"), line, MAX_STRID);            
            return FALSE;
        }

        if (TRUE == IsHaveHexId(uiVal_1))
        {
            _tprintf(_T("**ERROR: line_%d, SETSTR/SETHEX/SETDEC for StrId_%d has already existed!\n"), line, uiVal_1);            
            return FALSE;
        }

        g_uiHexId[g_uiNumOfHexId] = uiVal_1;
        g_uiNumOfHexId++;
                
        //int to string
        memset((ptLinecmd->pValue)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        memset((ptLinecmd->pValue + 1)->value, 0, sizeof(TCHAR) * LEN_VALUE);
        UiToString(uiVal_1, 16, (ptLinecmd->pValue)->value, 8);
        UiToString(uiVal_2, 16, (ptLinecmd->pValue + 1)->value, 8);

        WriteToLstfile((TCHAR *)ptLinecmd, 3, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_VER:
        if ((0 == _tcsicmp((ptLinecmd->pValue)->value, _T(""))) || (ptLinecmd->uiNumOfValue > 1)
            || (FALSE == CheckIntValue(_tcslen((ptLinecmd->pValue)->value), 1)))//length shouldn't more than 1 byte
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);            
            return FALSE;
        }

        memset(&tVer, 0, sizeof(VER_DESC));

        if (_stscanf_s((ptLinecmd->pValue)->value, _T("%d.%d.%d%c"), &(tVer.ww), &(tVer.xx), &(tVer.yy), &(tVer.c)) != 4)
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter format is error!\n"), line, eOptype);           
            return FALSE;
        }

        //uiLen = _tcslen(tVer.yy);
        //tVer.c = tVer.yy[uiLen];
        //tVer.yy[uiLen] = 0;

        //check 0=< ww/xx/yy <= 99
        //check c should be 'a'~'z'
        if ((FALSE == CheckVerDec(tVer.ww))
            || (FALSE == CheckVerDec(tVer.xx))
            || (FALSE == CheckVerDec(tVer.yy))
            || (FALSE == CheckVerChar(tVer.c)))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter format is error!\n"), line, eOptype);            
            return FALSE;
        }

        WriteToLstfile((TCHAR *)ptLinecmd, 2, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_NFC_INTERFACE_INIT:
    case OPCODE_NFC_RESET:    
    case OPCODE_NFC_UPDATE_PUMAPPING:
    case OPCODE_END:
        if (ptLinecmd->uiNumOfValue > 0)
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d should no parameter!\n"), line, eOptype);            
            return FALSE;
        }

        WriteToLstfile((TCHAR *)ptLinecmd, 1, pLstfile);
        sts = TRUE;
        break;
    case OPCODE_VENDOR:
        if (FALSE == CheckIntValue(ptLinecmd->uiNumOfValue, 2))
        {
            _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);           
            return FALSE;
        }

        for (i = 0; i < ptLinecmd->uiNumOfValue; i++)
        {
            uiVal_1 = GetOperateResult((ptLinecmd->pValue + i)->value, line);
            if (i == 0)//subcode 2 BYTE
            {
                if (FALSE == CheckIntValue(uiVal_1, 2))
                {
                    _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);                    
                    return FALSE;
                }
                UiToString(uiVal_1, 16, (ptLinecmd->pValue + i)->value, 4);
            }
            else//other values 4 BYTE
            {
                if (FALSE == CheckIntValue(uiVal_1, 4))
                {
                    _tprintf(_T("**ERROR: line_%d, opcode%d parameter is error!\n"), line, eOptype);                    
                    return FALSE;
                }
                UiToString(uiVal_1, 16, (ptLinecmd->pValue + i)->value, 8);
            }
        }

        WriteToLstfile((TCHAR *)ptLinecmd, (ptLinecmd->uiNumOfValue + 2), pLstfile);
        sts = TRUE;
        break;
    default:
        sts = FALSE;
        break;
    }

    return sts;
}

static BOOL CheckDirective(INT line, LINE_COMMAND * ptLinecmd, FILE * pLstfile)
{
    //UINT uiVal_1, uiVal_2;
    DIRECTIVE_TYPE eDirective = DIRECTIVE_NULL;
    UINT uiVal_1, uiVal_2;
    TCHAR * pTemp;    
    BOOL sts = FALSE;

    if (ptLinecmd == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    if (FALSE == GetDirectiveType(ptLinecmd->type, &eDirective))
    {
        _tprintf(_T("**ERROR: line_%d, couldn't find the directive!\n"), line);        
        return FALSE;
    }

    switch (eDirective)
    {
    case DIRECTIVE_INCLUDE:
        if ((0 == _tcsicmp((ptLinecmd->pValue)->value, _T(""))) || (ptLinecmd->uiNumOfValue > 1))
        {
            _tprintf(_T("**ERROR: line_%d, INCLUDE parameter is error!\n"), line);            
            return FALSE;
        }

        //check whether the format is right:
        //include "file.cfg"
        //include "\dir\file.cfg"
        if ((ptLinecmd->pValue)->value[0] != _T('\"'))
        {
            _tprintf(_T("**ERROR: line_%d INCLUDE format is error!\n"), line);
            return FALSE;
        }
        else
        {
            pTemp = _tcsrchr((ptLinecmd->pValue)->value, _T('.'));
            if (pTemp != NULL)
            {
                if (0 != _tcsicmp(pTemp, _T(".cfg\"")))//the end should be .cfg"
                {
                    _tprintf(_T("**ERROR: line_%d INCLUDE format is error!\n"), line);                    
                    return FALSE;
                }
            }
            else
            {
                _tprintf(_T("**ERROR: line_%d INCLUDE format is error!\n"), line);                
                return FALSE;
            }
        }     

        //WriteToLstfile((TCHAR *)ptLinecmd, 2, pLstfile);
        _ftprintf(pLstfile, _T("%s %s\n"), ptLinecmd->type, (ptLinecmd->pValue)->value);
        sts = TRUE;
        break;
    case DIRECTIVE_DEFINE:  
        sts = TRUE;
        break;
    case DIRECTIVE_FTABLEID:        
        if (ptLinecmd->uiNumOfValue == 1)
        {
            if (0 == _tcsicmp((ptLinecmd->pValue)->value, _T("")))
            {
                _tprintf(_T("**ERROR: line_%d, FTABLEID parameter is error!\n"), line);                
                return FALSE;
            }
        }
        else if (ptLinecmd->uiNumOfValue == 2)
        {
            if ((0 == _tcsicmp((ptLinecmd->pValue)->value, _T(""))) || (0 == _tcsicmp((ptLinecmd->pValue + 1)->value, _T(""))))
            {
                _tprintf(_T("**ERROR: line_%d, FTABLEID parameter is error!\n"), line);                
                return FALSE;
            }
        }
        else
        {
            _tprintf(_T("**ERROR: line_%d, FTABLEID should have 1 or 2 parameter!\n"), line);            
            return FALSE;
        }

        ReplaceConst((ptLinecmd->pValue)->value);
        if (ptLinecmd->uiNumOfValue == 1)
        {
            _ftprintf(pLstfile, _T("%s %s\n"), ptLinecmd->type, (ptLinecmd->pValue)->value);
        }
        else//2 parameters
        {
            ReplaceConst((ptLinecmd->pValue + 1)->value);
            _ftprintf(pLstfile, _T("%s %s %s\n"), ptLinecmd->type, (ptLinecmd->pValue)->value, (ptLinecmd->pValue + 1)->value);
        }        

        sts = TRUE;
        break;
    case DIRECTIVE_PTMEMSET:        
        if ((0 == _tcsicmp((ptLinecmd->pValue)->value, _T(""))) || (0 == _tcsicmp((ptLinecmd->pValue + 1)->value, _T(""))))
        {
            _tprintf(_T("**ERROR: line_%d, PTMEMSET parameter is error!\n"), line);           
            return FALSE;
        }

        ReplaceConst((ptLinecmd->pValue)->value);
        ReplaceConst((ptLinecmd->pValue + 1)->value);

        uiVal_1 = GetOperateResult((ptLinecmd->pValue)->value, line);
        uiVal_2 = GetOperateResult((ptLinecmd->pValue + 1)->value, line);

        sts = AssignPtableFromCfg(uiVal_1, uiVal_2, line);
        break;
    default:
        sts = FALSE;
        break;
    }

    return sts;
}

//delete the spaces and \t before string
static void DelSpaceBefore(TCHAR * str)
{
    INT i, iLen;
    TCHAR temp[LINESZ];

    if (str == NULL)
    {
        return;
    }

    iLen = _tcslen(str);
    for (i = 0; (i < iLen); i++)
    {
        if ((*(str + i) != _T(' ')) && (*(str + i) != _T('\t')))
        {
            break;
        }
        //*(str + i) = *(str + i + 1);
    }

    _tcscpy_s(temp, LINESZ, (str + i));
    _tcscpy_s(str, LINESZ, temp);

    return;
}

//delete the space and \t after string
static void DelSpaceAfter(TCHAR * str)
{
    INT i, iLen;

    if (str == NULL)
    {
        return;
    }
        
    iLen = _tcslen(str);
    if ((*(str + iLen - 1) == _T('\r')) || (*(str + iLen - 1) == _T('\n')))
    {
        for (i = (iLen - 2); (i > 0); i--)
        {
            if ((*(str + i) != _T(' ')) && (*(str + i) != _T('\t'))
                && (*(str + i) != _T('\r')) && (*(str + i) != _T('\n')))
            {
                break;
            }

            if (((*(str + i) == _T(' ')) || (*(str + i) == _T('\t')))
                && ((*(str + i + 1) == _T('\r')) || (*(str + i + 1) == _T('\n'))))
            {
                *(str + i) = *(str + i + 1);
                *(str + i + 1) = _T('\0');
            }
        }
    }
    else//no \r or \n, the last line
    {
        for (i = (iLen - 1); (i > 0); i--)
        {
            if ((*(str + i) != _T(' ')) && (*(str + i) != _T('\t'))
                && (*(str + i) != _T('\r')) && (*(str + i) != _T('\n')))
            {
                break;
            }

            if ((*(str + i) == _T(' ')) || (*(str + i) == _T('\t')))                
            {
                *(str + i) = _T('\0');                
            }
        }
    }    

    return;
}

//check whether continue to read next line or deal with the current line
static BOOL CheckWillContinue(TCHAR * pLine, BOOL * pbContinue)
{
    INT iLen, i;
    BOOL bIsEmptyLine = TRUE;

    *pbContinue = FALSE;

    //for empty line with all ' ' or '\t'
    DelSpaceBefore(pLine);
    iLen = _tcslen(pLine);
    if (iLen == 0)//empty line with nothing
    {
        *pbContinue = TRUE;
    }

    //delete space after the string
    DelSpaceAfter(pLine);
    iLen = _tcslen(pLine);//string length change after delete space after the string

    ////empty line with '\n'
    //if ((iLen == 1) && line[iLen - 1] == _T('\n'))
    //{
    //    continue;
    //}
    ////from ubuntu to windows, add a empty line  0x20 0x0A
    //if ((iLen == 2) && line[iLen - 1] == _T('\n'))
    //{
    //    continue;
    //}

    //empty line but len is not 0
    //include "\n", " \n", "\r\r "   
    for (i = 0; i < iLen; i++)
    {
        if ((*(pLine + i) != _T(' ')) && (*(pLine + i) != _T('\n')) && (*(pLine + i) != _T('\r')))
        {
            bIsEmptyLine = FALSE;
            break;
        }
    }
    if (bIsEmptyLine == TRUE)
    {
        *pbContinue = TRUE;
    }

    //UTF-8 format
    if ((*(pLine) == 0xEF) && (*(pLine + 1) == 0xBB) && (*(pLine + 2) == 0xBF))
    {
        _tprintf(_T("**ERROR: please transfer UTF-8 format to ASCII format!\n"));        
        return FALSE;        
    }

    return TRUE;
}

//free the memory which malloc for one line in ParseLine()
static void FreeValueMemory(LINE_PARAMETER * ptParam)
{
    if (ptParam != NULL)
    {
        free(ptParam);
        ptParam = NULL;
    }
}

static BOOL IsExistTheListFile(const TCHAR * pPath)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile;

    if (pPath == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    hFile = FindFirstFile(pPath, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("**ERROR: %s has already existed in OUTPUT directory, please don't include the same name cfg file!\n"), pPath);
        FindClose(hFile);        
        return TRUE;
    }

    return FALSE;
}

static BOOL LoadOneCfgFile(const TCHAR *pCfgName, FILE * pCfgFile)
{
    TCHAR lstPath[MAX_PATH];
    TCHAR fileName[MAX_PATH];
    INT iLstErr;    
    FILE * pLstFile;
    TCHAR line[LINESZ];    
    UINT uiLineNo = 0;
    LINE_COMMAND tLineCmd;
    TCHAR *p = NULL;
    BOOL res = TRUE;
    BOOL bIsExist = FALSE;
    UINT8 isDirective = FALSE;    
    BOOL bIsContinue;
#ifdef COMPARE
    FILE * pTxtFile;
    INT iTxtErr;
#endif

    if (pCfgFile == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }
       
    memset(lstPath, 0, sizeof(lstPath));
    memset(fileName, 0, sizeof(fileName));   

    //generate the same name list file    
    _tcscpy_s(fileName, MAX_PATH, pCfgName);
    p = _tcsrchr(fileName, _T('.'));
    if (NULL != p)
    {
        memcpy(p, _T(".lst"), sizeof(_T(".lst")));
    }
    else
    {
        _tprintf(_T("**ERROR: couldn't gerenrate the same name list file for %s cfg file!\n"), pCfgName);        
        return FALSE;
    }
    
    _stprintf_s(lstPath, MAX_PATH, _T("%s\\output\\%s"), g_exeFolder, fileName);
    bIsExist = IsExistTheListFile(lstPath);
    if (TRUE == bIsExist)
    {        
        return FALSE;
    }

    iLstErr = _tfopen_s(&pLstFile, lstPath, _T("w"));    

#ifdef COMPARE
    //for debug
    p = _tcsrchr(lstPath, _T('.'));
    if (NULL != p)
    {
        memcpy(p, _T(".txt"), sizeof(_T(".txt")));
    }
    iTxtErr = _tfopen_s(&pTxtFile, lstPath, _T("w"));
#endif

    if (0 == iLstErr)
    {
        _tprintf(_T("--begin to compile '%s':\n"), pCfgName);
        while (NULL != _fgetts(line, (sizeof(line) / sizeof(line[0])), pCfgFile))
        {
            memset(&tLineCmd, 0, sizeof(tLineCmd));

            uiLineNo++;

            res = CheckWillContinue(line, &bIsContinue);
            if (FALSE == res)
            {                
                break;
            }
            else
            {
                if (bIsContinue == TRUE)
                {
                    continue;
                }
            }            

            if ((line[0] == _T('/')) && (line[1] == _T('/')))
            {
                //comments, just write to .lst file                
                WriteToLstfile(line, 0, pLstFile);                
                continue;
            }
#ifdef COMPARE
            else
            {    
                //for debug
                if (iTxtErr == 0)
                    _ftprintf(pTxtFile, _T("%s"), line);
            }
#endif

            //parse line
            res = ParseLine(line, &tLineCmd, &isDirective, uiLineNo);
            if (FALSE == res)
            {
                _tprintf(_T("**ERROR: line_%d opcode or directive couldn't be identified!\n"), uiLineNo);
                FreeValueMemory(tLineCmd.pValue);                
                //fclose(pCfgFile);
                //fclose(pLstFile);                
                break;
            }

            //check directive
            if (isDirective == TRUE)
            {
                res = CheckDirective(uiLineNo, &tLineCmd, pLstFile);
                if (TRUE == res)
                {
                    FreeValueMemory(tLineCmd.pValue);
                    continue;
                }
            }
            else
            {
                //check the parameter and write into *.bin            
                res = CheckOpAndWriteToLfile(uiLineNo, &tLineCmd, pLstFile);
                if (FALSE == res)
                {
                    FreeValueMemory(tLineCmd.pValue);
                    break;
                }
            }

            FreeValueMemory(tLineCmd.pValue);
        }

        if ((TRUE == res) && (uiLineNo > 0))
        {
            _tprintf(_T("generate '%s'.\n"), fileName);
        }
        if (uiLineNo == 0)
        {
            _tprintf(_T("the '%s' is an empty file!\n"), pCfgName);
        }
        //fclose(pCfgFile);
        fclose(pLstFile);
#ifdef COMPARE
        fclose(pTxtFile);
#endif

        if (TRUE == res)
        {
            _tprintf(_T("--end to compile '%s'.\n\n"), pCfgName);
        }
        else
        {
            _tprintf(_T("--fail to compile '%s'.\n\n"), pCfgName);
        }
    }
    else
    {
        _tprintf(_T("--fail to open config and list file!\n"));
        res = FALSE;
    }

    return res;
}

static BOOL CollectConstInOneFile(FILE * pCfgFile, const TCHAR * pFileName)
{
    
    TCHAR line[LINESZ];    
    UINT uiLineNo = 0;
    LINE_COMMAND tLineCmd;
    TCHAR *p = NULL;
    BOOL res = TRUE;
    UINT8 isDirective = FALSE;
    DIRECTIVE_TYPE eDirective;    
    BOOL bIsContinue;
       
    if (pCfgFile == NULL)
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return FALSE;
    }

    while (NULL != _fgetts(line, (sizeof(line) / sizeof(line[0])), pCfgFile))
    {
        memset(&tLineCmd, 0, sizeof(tLineCmd));

        uiLineNo++;

        res = CheckWillContinue(line, &bIsContinue);
        if (FALSE == res)
        {
            break;
        }
        else
        {
            if (bIsContinue == TRUE)
            {
                continue;
            }
        }

        if ((line[0] == _T('/')) && (line[1] == _T('/')))
        {
            continue;
        }

        //parse line
        res = ParseLine(line, &tLineCmd, &isDirective, uiLineNo);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: %s, line_%d opcode or directive couldn't be identified!\n"), pFileName, uiLineNo);
            FreeValueMemory(tLineCmd.pValue);            
            res = FALSE;
            break;
        }

        //check directive
        if (isDirective == TRUE)
        {
            res = GetDirectiveType(tLineCmd.type, &eDirective);
            if ((TRUE == res) && (DIRECTIVE_DEFINE == eDirective))
            {
                //if ((0 == _tcsicmp((tLineCmd.pValue)->value, _T(""))) || (0 == _tcsicmp((tLineCmd.pValue + 1)->value, _T(""))))
                if (tLineCmd.uiNumOfValue != 2)
                {
                    _tprintf(_T("**ERROR: %s, line_%d, \"%s\" parameter is error!\n"), pFileName, uiLineNo, tLineCmd.type);
                    FreeValueMemory(tLineCmd.pValue);                    
                    res = FALSE;
                    break;
                }

                if (g_uiNumOfConst >= MAX_CONST)
                {
                    _tprintf(_T("**ERROR: %s, line_%d, consts are more than %d!\n"), pFileName, uiLineNo, MAX_CONST);
                    FreeValueMemory(tLineCmd.pValue);                    
                    res = FALSE;
                    break;
                }

                if (TRUE == IsHaveConst((tLineCmd.pValue)->value))
                {
                    _tprintf(_T("**ERROR: %s, line_%d, micro %s has been defined!\n"), pFileName, uiLineNo, (tLineCmd.pValue)->value);
                    FreeValueMemory(tLineCmd.pValue);                    
                    res = FALSE;
                    break;
                }

                memcpy((g_tConst[g_uiNumOfConst].const_name), (tLineCmd.pValue)->value, sizeof((tLineCmd.pValue)->value));
                memcpy((g_tConst[g_uiNumOfConst].const_val), (tLineCmd.pValue + 1)->value, sizeof((tLineCmd.pValue + 1)->value));
                g_uiNumOfConst++;
            }
            else
            {
                FreeValueMemory(tLineCmd.pValue);
                continue;
            }
        }
        FreeValueMemory(tLineCmd.pValue);
    }   

    return res;
}

//delete the double quotation before and after the string
//"\dir\file.cfg" --> \dir\file.cfg
static void DelQuotation(TCHAR * str)
{
    TCHAR temp[LEN_VALUE];
    TCHAR * pTemp;

    if (str == NULL)
    {
        return;
    }

    //delete the double quotation before    
    _tcscpy_s(temp, (_tcslen(str) + 1), (str + 1));
    _tcscpy_s(str, (_tcslen(temp) + 1), temp);//should include '\0'

    pTemp = _tcsrchr(str, _T('\"'));
    if (pTemp != NULL)
    {
        *pTemp = _T('\0');
    }

    return;
}

BOOL CollectAllConsts(const TCHAR * pFolder, const TCHAR * pFileName)
{
    TCHAR findPath[MAX_PATH] = { 0 };
    TCHAR cfgPath[MAX_PATH] = { 0 };
    FILE * pCfgFile;
    TCHAR line[LINESZ];
    TCHAR val_1[LEN_VALUE];
    TCHAR val_2[LEN_VALUE];
    TCHAR fileName[MAX_PATH] = { 0 };
    TCHAR * pTemp;
    BOOL res = TRUE;

    ////////////////////////////////////////////////////////////////////////////////////////
    //for main.cfg, the pFolder is already absolute path
    if (0 == _tcsicmp(pFileName, _T("main.cfg")))
    {
        _stprintf_s(findPath, MAX_PATH, _T("%s\\%s"), pFolder, pFileName);
    }
    else//for include file, pFolder maybe absolute or relative path
    {
        if (NULL != pFolder)
        {
            if (TRUE == IsAbsolutePath(pFolder))
            {
                _stprintf_s(findPath, MAX_PATH, _T("%s\\%s"), pFolder, pFileName);
            }
            else//relative to the root path
            {
                _stprintf_s(findPath, MAX_PATH, _T("%s%s\\%s"), g_exeFolder, pFolder, pFileName);
            }
        }
        else//folder is NULL, file is relative to the root path
        {
            _stprintf_s(findPath, MAX_PATH, _T("%s\\%s"), g_exeFolder, pFileName);
        }
    }

    if (0 == _tfopen_s(&pCfgFile, findPath, _T("r+")))
    {
        //collect the define const in the file
        res = CollectConstInOneFile(pCfgFile, pFileName); 
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: collect consts in %s file error, stop!\n"), pFileName);            
            return res;
        }

        fseek(pCfgFile, 0, SEEK_SET);
        while (NULL != _fgetts(line, (sizeof(line) / sizeof(line[0])), pCfgFile))
        {
            if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                && (!_tcsicmp(val_1, _T("include"))))
            {
                //去掉前后双引号
                DelQuotation(val_2);

                pTemp = _tcschr(val_2, _T('\\'));
                if (NULL != pTemp) //val_2 include sub directory, such like \dir\file.cfg
                {
                    pTemp = _tcsrchr(val_2, _T('\\'));
                    if (pTemp != NULL)
                    {
                        _tcscpy_s(fileName, MAX_PATH, (pTemp + 1));
                        *pTemp = _T('\0');//val_2 change to be \dir                       

                        res = CollectAllConsts(val_2, fileName);
                    }                    
                }
                else//val_2 is only the file name, the file is in the parent folder
                {
                    _tcscpy_s(fileName, (_tcslen(val_2) + 1), val_2);                   

                    res = CollectAllConsts(val_2, fileName);
                }

                pTemp = NULL;
                if (FALSE == res)
                {
                    _tprintf(_T("**ERROR: collect consts in %s file error, stop!\n"), val_2);
                    return res;
                }
            }
        }

        fclose(pCfgFile);
    }
    else
    {
        res = FALSE;
        _tprintf(_T("**ERROR: couldn't open %s file!\n"), pFileName);        
    }

    return res;
}

//recursion will lead to "stack around xx was corrupted" ??
BOOL GenerateListFiles(const TCHAR * pFolder, const TCHAR * pFileName)
{
    TCHAR cfgPath[MAX_PATH] = { 0 };    
    FILE * pCfgFile;
    TCHAR line[LINESZ];
    TCHAR val_1[LEN_VALUE];
    TCHAR val_2[LEN_VALUE];   
    TCHAR fileName[MAX_PATH] = { 0 };    
    TCHAR * pTemp;
    BOOL res = TRUE; 

    ///////////////////////////////////////////////////////////////////////////////
    //for main.cfg, the pFolder is already absolute path
    if (0 == _tcsicmp(pFileName, _T("main.cfg")))
    {
        _stprintf_s(cfgPath, MAX_PATH, _T("%s\\%s"), pFolder, pFileName);
    }
    else//for include file, pFolder maybe absolute or relative path
    {
        if (NULL != pFolder)
        {
            if (TRUE == IsAbsolutePath(pFolder))
            {
                _stprintf_s(cfgPath, MAX_PATH, _T("%s\\%s"), pFolder, pFileName);
            }
            else//relative to the root path
            {
                _stprintf_s(cfgPath, MAX_PATH, _T("%s%s\\%s"), g_exeFolder, pFolder, pFileName);
            }
        }
        else//folder is NULL, file is relative to the root path
        {
            _stprintf_s(cfgPath, MAX_PATH, _T("%s\\%s"), g_exeFolder, pFileName);
        }
    }
       
    if (0 == _tfopen_s(&pCfgFile, cfgPath, _T("r+")))
    {  
        //generate *.lst file
        res = LoadOneCfgFile(pFileName, pCfgFile);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: load %s file error, stop!\n"), pFileName);                       
            return res;
        }

        fseek(pCfgFile, 0, SEEK_SET);
        while (NULL != _fgetts(line, (sizeof(line) / sizeof(line[0])), pCfgFile))
        {
            if ((_stscanf_s(line, _T("%s %s"), val_1, LEN_VALUE, val_2, LEN_VALUE) == 2)
                && (!_tcsicmp(val_1, _T("include"))))
            {
               //去掉前后双引号
                DelQuotation(val_2);
                
                pTemp = _tcschr(val_2, _T('\\'));
                if (NULL != pTemp) //val_2 include sub directory, such like \dir\file.cfg
                {
                    pTemp = _tcsrchr(val_2, _T('\\'));
                    if (pTemp != NULL)
                    {                        
                        _tcscpy_s(fileName, MAX_PATH, (pTemp + 1));
                        *pTemp = _T('\0');//val_2 change to be \dir                       

                        res = GenerateListFiles(val_2, fileName);
                    }
                    /*else
                    {
                        return FALSE;
                    }*/
                }
                else//val_2 is only the file name, the file is in the parent folder
                {                    
                    _tcscpy_s(fileName, (_tcslen(val_2) + 1), val_2);

                    res = GenerateListFiles(NULL, fileName);
                }  

                pTemp = NULL;                
                if (FALSE == res)
                {
                    _tprintf(_T("**ERROR: load %s file error, stop!\n"), val_2);                                      
                    return res;
                }
            }
        }
        
        fclose(pCfgFile);
    }
    else
    {
        res = FALSE;
        _tprintf(_T("**ERROR: couldn't open %s file!"), pFileName);
    }

    return res;   
}
