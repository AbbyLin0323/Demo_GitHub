#include "main.h"

void DelSpace(TCHAR * str)
{    
    INT i;

    if (str == NULL)
    {
        return;
    }

    while (*str != _T('\0'))
    {
        while (_istspace(*str))
        {
            for (i = 0; *(str + i) != _T('\0'); i++)
            {
                *(str + i) = *(str + i + 1);
            }
        }
        str++;
    }

    return;
}

static BOOL CheckExp(TCHAR * pCh, UINT uiLine)
{
    int bracket = 0;

    while (*pCh != _T('\0'))
    {
        //if (uiRadix == 16)
        {
            if (((*pCh >= _T('0')) && (*pCh <= _T('9')))
                || ((*pCh >= _T('a')) && (*pCh <= _T('f')))
                || ((*pCh >= _T('A')) && (*pCh <= _T('F')))
                || (*pCh == _T('&')) || (*pCh == _T('|')) || (*pCh == _T('~'))
                || (*pCh == _T('(')) || (*pCh == _T(')'))
                || (*pCh == _T('<')) || (*pCh == _T('>'))
                || (*pCh == _T('+')) || (*pCh == _T('-'))
                || (*pCh == _T('x')) || (*pCh == _T('X')))
            {

            }
            else
            {
                _tprintf(_T("**ERROR: line_%d, expression input error!\n"), uiLine);                
                return FALSE;
            }
        }
        /*else if (uiRadix == 10)
        {
            if (((*pCh >= _T('0')) && (*pCh <= _T('9')))                
                || (*pCh == _T('&')) || (*pCh == _T('|')) || (*pCh == _T('~'))
                || (*pCh == _T('(')) || (*pCh == _T(')'))
                || (*pCh == _T('<')) || (*pCh == _T('>')))               
            {

            }
            else
            {
                _tprintf(_T("**ERROR: line_%d, expression input error!\n"), uiLine);                
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }*/

        if (*pCh == _T('('))
        {
            bracket++;
        }
        if (*pCh == _T(')'))
        {
            bracket--;
        }

        pCh++;
    }

    if (bracket != 0)
    {
        _tprintf(_T("**ERROR: line_%d, brackets don't match!\n"), uiLine);        
        return FALSE;
    }

    return TRUE;
}

static void UpdateOperAndData(TCHAR *pOper, UINT *pData, INT p)
{
    int i;
    int iLen;

    if ((pOper == NULL) || (pData == NULL))
    {
        _tprintf(_T("**ERROR: the pointer is NULL!\n"));
        return;
    }

    iLen = _tcslen(pOper);
    for (i = p; i < iLen; i++)
    {
        pOper[i] = pOper[i + 1];
        pData[i] = pData[i + 1];
    }

    pOper[i] = _T('\0');
    pData[i] = pData[i + 1];
}

UINT ConvertValue(TCHAR *pCh, UINT uiLine)
{
    int i, iStart;
    UINT uiData = 0;
    UINT temp;
    UINT uiRadix;
    INT iLen = _tcslen(pCh);

    if (pCh == NULL)
    {
        _tprintf(_T("**ERROF: the pointer is NULL!\n"));
        return uiData;
    }
    
    if ((*pCh == _T('0')) && ((*(pCh + 1) == _T('x')) || (*(pCh + 1) == _T('X'))))
    {
        uiRadix = 16;
        iStart = 2;
        
        if (iLen > 10)
        {
            _tprintf(_T("**ERROR: line_%d, hex number is more than 0xFFFFFFFF!\n"), uiLine);            
            return uiData;
        }
    }
    else
    {
        uiRadix = 10;
        iStart = 0;

        if (iLen > 10)
        {
            _tprintf(_T("**ERROR: line_%d, dec number is more than 4294967295!\n"), uiLine);           
            return uiData;
        }
    }    

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
            _tprintf(_T("**ERROR: line_%d, data is not 0-9 for 10 radix or 0-9/a-f/A-F for 16 radix!\n"), uiLine);
            break;
        }
    }

    return uiData;
}

static BOOL CheckData(TCHAR Ch)
{
    int bracket = 0;

    if (Ch == _T('\0'))
    {
        _tprintf(_T("**ERROR: char is NULL!\n"));
        return FALSE;
    }

    if (((Ch >= _T('0')) && (Ch <= _T('9')))
        || ((Ch >= _T('a')) && (Ch <= _T('f')))
        || ((Ch >= _T('A')) && (Ch <= _T('F'))))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static UINT CalcuExp(TCHAR *pCh, UINT uiLine)
{
    TCHAR oper[MAX_NUM];//stack for all operators
    UINT data[MAX_NUM];//stack for all values
    INT oper_i = 0;
    INT i;//
    INT data_i = 0;
    TCHAR value[LEN_VALUE];//save one value
    INT value_i = 0;
    TCHAR temp[LEN_VALUE];
    INT temp_i = 0;
    INT bracket = 0;    

    memset(oper, 0, MAX_NUM * sizeof(TCHAR));
    memset(data, 0, MAX_NUM * sizeof(TCHAR));
    memset(value, 0, LEN_VALUE * sizeof(TCHAR));
    memset(temp, 0, LEN_VALUE * sizeof(TCHAR));

    while (*pCh != _T('\0'))
    {
        switch (*pCh)
        {
        case _T('&'):
        case _T('|'):
        case _T('+'):
        case _T('-'):
            oper[oper_i] = *pCh;
            oper_i++;

            //if ((*(pCh - 1) != _T(')')) && (*(pCh - 1) != _T('(')))
            if (*(pCh - 1) != _T(')'))
            {
                value[value_i] = _T('\0');
                value_i = 0;

                data[data_i] = ConvertValue(value, uiLine);
                data_i++;
                memset(value, 0, LEN_VALUE * sizeof(TCHAR));
            }
            break;
        case _T('~'):// must be '(' or operator before '~'
            oper[oper_i] = *pCh;
            oper_i++;
            break;
        case _T('<')://_T('<<') :
        case _T('>')://_T('>>') :
            if (((*pCh == _T('<')) && (*(pCh + 1) == _T('<')))
                || ((*pCh == _T('>')) && (*(pCh + 1) == _T('>'))))
            {
                oper[oper_i] = *pCh;
                oper_i++;

                //if ((*(pCh - 1) != _T(')')) && (*(pCh - 1) != _T('(')))
                if (*(pCh - 1) != _T(')'))
                {
                    value[value_i] = _T('\0');
                    value_i = 0;

                    data[data_i] = ConvertValue(value, uiLine);
                    data_i++;
                    memset(value, 0, LEN_VALUE * sizeof(TCHAR));
                }
                pCh++;
            }
            else
            {
                _tprintf(_T("**ERROR: line_%d, operator '<<' or '>>' is error!\n"), uiLine);                
            }
            break;
        case _T('('):
            bracket++;
            while (bracket > 0)
            {
                pCh++;
                temp[temp_i] = *pCh;
                temp_i++;

                if (temp_i >= LEN_VALUE)
                {
                    _tprintf(_T("**ERROR: line_%d, expression is too long!\n"), uiLine);                    
                    break;
                }

                if (*pCh == _T('('))
                {
                    bracket++;
                }
                if (*pCh == _T(')'))
                {
                    bracket--;
                }
            }
            temp[temp_i] = _T('\0');
            temp_i = 0;

            data[data_i] = CalcuExp(temp, uiLine);
            data_i++;
            memset(temp, 0, LEN_VALUE * sizeof(TCHAR));
            break;
        case _T(')')://data end ?????
            if (TRUE == CheckData(*(pCh - 1)))
            {
                value[value_i] = _T('\0');
                value_i = 0;

                data[data_i] = ConvertValue(value, uiLine);
                data_i++;
                memset(value, 0, LEN_VALUE * sizeof(TCHAR));
            }
            break;
        default://other is  a value
            value[value_i] = *pCh;
            value_i++;

            if (*(pCh + 1) == _T('\0'))
            {
                value[value_i] = _T('\0');
                value_i = 0;

                data[data_i] = ConvertValue(value, uiLine);
                data_i++;
                memset(value, 0, LEN_VALUE * sizeof(TCHAR));
            }
            break;
        }
        pCh++;
    }

    oper[oper_i] = _T('\0');

    i = 0;
    while (oper_i > 0)
    {
        switch (oper[i])
        {
        case _T('&'):
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            data[i + 1] = data[i] & data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        case _T('|'):
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            data[i + 1] = data[i] | data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        case _T('~'):
            oper_i--;
            data[i + 1] = ~(data[i]);//???????
            UpdateOperAndData(oper, data, i);
            break;
        case _T('>')://_T('>>') :
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            data[i + 1] = data[i] >> data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        case _T('<')://_T('<<') :
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            data[i + 1] = data[i] << data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        case _T('+'):
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            data[i + 1] = data[i] + data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        case _T('-'):
            oper_i--;
            if (oper[i + 1] == _T('~'))
            {
                data[i + 1] = ~(data[i + 1]);
                oper[i + 1] = oper[i + 2];
                oper_i--;
            }
            if (data[i] < data[i + 1])
            {
                _tprintf(_T("**ERROR: line_%d, after the subtraction operation, will get a negative number!\n"), uiLine);
                break;
            }
            data[i + 1] = data[i] - data[i + 1];
            UpdateOperAndData(oper, data, i);
            break;
        default:
            _tprintf(_T("**ERROR: line_%d, the operator is not supported!\n"), uiLine);            
            break;
        }
    }

    return data[0];
}

UINT GetOperateResult(TCHAR * pCh, UINT uiLine)
{
    UINT data = 0;

    if (pCh == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to char is NULL!\n"));
        return data;
    }

    DelSpace(pCh);
    
    if (TRUE == CheckExp(pCh, uiLine))
    {
        data = CalcuExp(pCh, uiLine);
    }
    else
    {
        _tprintf(_T("**ERROR: expression input error!\n"));        
    }

    return data;
}