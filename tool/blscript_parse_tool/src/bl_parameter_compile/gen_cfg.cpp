#include "main.h"

//extern TCHAR g_fileFolder[];

static TCHAR * GetOpcodeString(UINT opcode, TCHAR *pStr)
{
    OPCODE_TYPE_DESC *pOp = OPCODE_TYPE_LIB;

    while (pOp->eOpcodeType != OPCODE_NULL)
    {
        if (pOp->eOpcodeType == opcode)
        {
            _tcsncpy_s(pStr, LEN_OPCODE, pOp->pDesc, LEN_OPCODE);
            return pStr;
        }
        pOp++;
    }

    return NULL;
}

static BOOL WriteToCfgFile(BYTE *pBuf, UINT uiLen, FILE *pFile)
{
    TCHAR * pString = NULL;
    UINT i;
    BOOL res = FALSE;
    UINT uiOp;
    UINT uiValue;
    UINT uiNum;
    U8 ucStrLen;

    if (pBuf == NULL)
    {
        _tprintf(_T("**ERROR: the pointer to buffer is NULL!\n"));
        return FALSE;
    }

    pString = (TCHAR *)malloc(sizeof(TCHAR) * LEN_OPCODE);
    if (pString == NULL)
    {
        _tprintf(_T("**ERROR: malloc buffer fail!\n"));        
        res = FALSE;
        return res;
    }

    uiOp = 0;
    memcpy(&uiOp, pBuf, NUM_OF_OPCODE_BYTE);

    pString = GetOpcodeString(uiOp, pString);
    if (NULL == pString)
    {
        _tprintf(_T("**ERROR: opcode couldn't be identified!\n"));        
        res = FALSE;
        return res;
    }

    _ftprintf(pFile, _T("%s "), pString);//printf opcode
    switch (uiOp)
    {
    case OPCODE_MEMW32:
    case OPCODE_MEMW16:
    case OPCODE_MEMW8:
    case OPCODE_SETBIT32:
    case OPCODE_SETBIT16:
    case OPCODE_SETBIT8:
    case OPCODE_CLRBIT32:
    case OPCODE_CLRBIT16:
    case OPCODE_CLRBIT8:
    case OPCODE_MWAIT32:
    case OPCODE_MWAIT16:
    case OPCODE_MWAIT8:
    case OPCODE_NFC_SETFEATURE:
        /*for (i = 1; i < uiLen; i++)
        {
            if (i == 5)
            {
                _ftprintf(pFile, _T(" "));
            }

            if ((i == 1) || (i == 5))
            {
                _ftprintf(pFile, _T("0x"));
            }

            _ftprintf(pFile, _T("%02x"), *(pBuf + i));            
        }*/

        //should be little endian
        _ftprintf(pFile, _T("0x"));
        for (i = (NUM_OF_OPCODE_BYTE + 3); i >= NUM_OF_OPCODE_BYTE; i--)
        {
            _ftprintf(pFile, _T("%02x"), *(pBuf + i));//printf addr
        }

        _ftprintf(pFile, _T(" 0x"));
        for (i = (uiLen - 1); i >= (NUM_OF_OPCODE_BYTE + 4); i--)
        {
            _ftprintf(pFile, _T("%02x"), *(pBuf + i));//printf value
        }

        res = TRUE;
        break;
    case OPCODE_UDELAY:
    case OPCODE_MDELAY:
        uiValue = 0;
        memcpy(&uiValue, (pBuf + NUM_OF_OPCODE_BYTE), 4);
        _ftprintf(pFile, _T("%d"), uiValue);//printf value
        res = TRUE;
        break;
    case OPCODE_SETSTR:
        _ftprintf(pFile, _T("0x%02x "), *(pBuf + NUM_OF_OPCODE_BYTE));//printf strid
        ucStrLen = *(pBuf + NUM_OF_OPCODE_BYTE + 1);
        for (i = (NUM_OF_OPCODE_BYTE + 2); ((i - (NUM_OF_OPCODE_BYTE + 2)) < ucStrLen) && (i < uiLen); i++)
        {
            if (i == (NUM_OF_OPCODE_BYTE + 2))
            {
                _ftprintf(pFile, _T("\""));
            }

            if (*(pBuf + i) == 0x22)
            {
                _ftprintf(pFile, _T("\\"));
            }
            _ftprintf(pFile, _T("%c"), *(pBuf + i));

            if ((i - (NUM_OF_OPCODE_BYTE + 2)) == (ucStrLen -1 ))
            {
                _ftprintf(pFile, _T("\""));
            }
        }
        res = TRUE;
        break;        
    case OPCODE_SETHEX:
        uiValue = 0;
        memcpy(&uiValue, (pBuf + NUM_OF_OPCODE_BYTE), 4);
        _ftprintf(pFile, _T("0x%04x 0x"), uiValue);//printf hexid
        /*for (i = 2; i < uiLen; i++)
        {  
            _ftprintf(pFile, _T("%02x"), *(pBuf + i));
        }*/
        //should be little endian
        for (i = (uiLen - 1); i >= (NUM_OF_OPCODE_BYTE + 4); i--)
        {
            _ftprintf(pFile, _T("%02x"), *(pBuf + i));//printf value
        }
        res = TRUE;
        break;
    case OPCODE_VER:
        for (i = (NUM_OF_OPCODE_BYTE + 1); ((i - (NUM_OF_OPCODE_BYTE + 1)) < *(pBuf + NUM_OF_OPCODE_BYTE)) && (i < uiLen); i++)
        {           
            _ftprintf(pFile, _T("%c"), *(pBuf + i));//printf str            
        }
        res = TRUE;
        break;
    case OPCODE_NFC_INTERFACE_INIT:
    case OPCODE_NFC_RESET:
    case OPCODE_NFC_UPDATE_PUMAPPING:
    case OPCODE_END:
        res = TRUE;
        break;
    case OPCODE_VENDOR:
        uiNum = 0;
        memcpy(&uiNum, (pBuf + NUM_OF_OPCODE_BYTE), 2);
        _ftprintf(pFile, _T("0x%04x "), uiNum);//printf number of value
        uiValue = 0;
        memcpy(&uiValue, (pBuf + NUM_OF_OPCODE_BYTE + 2), 2);
        _ftprintf(pFile, _T("0x%04x "), uiValue);//printf subopcode
                
        for (i = 0; i < (uiNum - 1); i++)
        {
            uiValue = 0;
            memcpy(&uiValue, (pBuf + NUM_OF_OPCODE_BYTE + 2 + 2 + i * 4), 4);
            _ftprintf(pFile, _T("0x%08x "), uiValue);//printf parameter
        }

        res = TRUE;
        break;
    default:
        res = FALSE;
        break;
    }
    _ftprintf(pFile, _T("\n"));
    
    return res;
}

BOOL GenerateCfgFile(const TCHAR *pFolder, const TCHAR * pBinFileName, const TCHAR * pCfgFileName)
{
    TCHAR binPath[MAX_PATH];
    TCHAR cfgPath[MAX_PATH];    
    FILE * pBinFile;
    FILE * pCfgFile;
    INT binErr, cfgErr;
    UINT uiTemp, uiLen;
    BYTE *pBuf = NULL;
    BOOL res = FALSE;
    UINT uiOffset;
    ULONG ulTotalByte = 0;
    ULONG ulMaxSize;
    FTABLE tFtable;
    UINT uiOp;
    UINT uiValue;

    memset(binPath, 0, sizeof(binPath));
    memset(cfgPath, 0, sizeof(cfgPath));       
     
    _stprintf_s(binPath, MAX_PATH, _T("%s\\%s"), pFolder, pBinFileName);
    _stprintf_s(cfgPath, MAX_PATH, _T("%s\\%s"), pFolder, pCfgFileName);

    binErr = _tfopen_s(&pBinFile, binPath, _T("rb+"));
    cfgErr = _tfopen_s(&pCfgFile, cfgPath, _T("w"));    

    if ((0 == binErr) && (0 == cfgErr))
    {
        _tprintf(_T("--begin to de-compile reg_list in binary file:\n"));

        //find the total bytes of REG_LIST
        uiOffset = 0;// BL_CODE_SIZE;
        fseek(pBinFile, uiOffset, SEEK_SET);
        if (1 != fread(&tFtable, FTABLE_SIZE, 1, pBinFile))
        {
            _tprintf(_T("**ERROR: fail to read bootloader binary file!\n"));            
            fclose(pBinFile);
            fclose(pCfgFile);            
            return res;
        }
        //calculate the max ftable id
        ULONG ulMaxPos = 0;
        UINT uiMaxId;
        UINT i;
        for (i = 0; i < FTABLE_FUNC_TOTAL; i++)
        {
            if (tFtable.aInitFuncEntry[i].ulRegPos >= ulMaxPos)
            {
                ulMaxPos = tFtable.aInitFuncEntry[i].ulRegPos;
                uiMaxId = i;
            }
        }
        ulMaxSize = tFtable.aInitFuncEntry[uiMaxId].ulRegPos + tFtable.aInitFuncEntry[uiMaxId].ulRegCnt;

        uiOffset = FTABLE_SIZE + PTABLE_SIZE;// BL_CODE_SIZE + FTABLE_SIZE + PTABLE_SIZE;
        fseek(pBinFile, uiOffset, SEEK_SET);

        pBuf = (BYTE *)malloc(BUF_SIZE * sizeof(BYTE));
        if (NULL == pBuf)
        {
            _tprintf(_T("**ERROR: malloc buffer error when generate cfg file!\n"));
            return res;
        }
        memset(pBuf, 0, BUF_SIZE * sizeof(BYTE));
                
        uiLen = NUM_OF_OPCODE_BYTE;
        while ((uiLen == fread_s(pBuf, BUF_SIZE, sizeof(BYTE), uiLen, pBinFile)))
            //&& (ulTotalByte < ulMaxSize)) //parse until "end"
        { 
            ulTotalByte += uiLen;

            uiOp = 0;
            memcpy(&uiOp, pBuf, NUM_OF_OPCODE_BYTE);
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
                uiLen = 8;
                if (uiLen != fread_s((pBuf + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;

                res = WriteToCfgFile(pBuf, (uiLen + NUM_OF_OPCODE_BYTE), pCfgFile);
                break;            
            case OPCODE_UDELAY:
            case OPCODE_MDELAY:
                uiLen = 4;
                if (uiLen != fread_s((pBuf + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;

                res = WriteToCfgFile(pBuf, (uiLen + NUM_OF_OPCODE_BYTE), pCfgFile);
                break;
            case OPCODE_SETSTR:
                uiLen = 2;
                if (uiLen != fread_s((pBuf + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;
                uiTemp = uiLen;

                uiLen = *(pBuf + NUM_OF_OPCODE_BYTE + uiTemp - 1);
                if ((uiLen + NUM_OF_OPCODE_BYTE + uiTemp) % 4)
                {
                    uiLen += (4 - ((uiLen + NUM_OF_OPCODE_BYTE + uiTemp) % 4));
                }
                if (uiLen != fread_s((pBuf + +uiTemp + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;

                res = WriteToCfgFile(pBuf, (uiTemp + uiLen + NUM_OF_OPCODE_BYTE), pCfgFile);
                break;           
            case OPCODE_VER:
                uiLen = 1;
                if (uiLen != fread_s((pBuf + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;
                uiTemp = uiLen;

                uiLen = *(pBuf + NUM_OF_OPCODE_BYTE);
                if ((uiLen + uiTemp + NUM_OF_OPCODE_BYTE) % 4)
                {
                    uiLen += (4 - ((uiLen + uiTemp + NUM_OF_OPCODE_BYTE) % 4));
                }
                if (uiLen != fread_s((pBuf + uiTemp + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;
                
                res = WriteToCfgFile(pBuf, (uiTemp + uiLen + NUM_OF_OPCODE_BYTE), pCfgFile);
                break;
            case OPCODE_NFC_INTERFACE_INIT:
            case OPCODE_NFC_RESET:
            case OPCODE_NFC_UPDATE_PUMAPPING:
            case OPCODE_END:
                res = WriteToCfgFile(pBuf, NUM_OF_OPCODE_BYTE, pCfgFile);
                break;
            case OPCODE_VENDOR:
                uiLen = 2;
                if (uiLen != fread_s((pBuf + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;
                uiTemp = uiLen;

                uiValue = 0;
                memcpy(&uiValue, (pBuf + NUM_OF_OPCODE_BYTE), 2);
                uiLen = 2 + (uiValue - 1) * 4;
                if (uiLen != fread_s((pBuf + uiTemp + NUM_OF_OPCODE_BYTE), BUF_SIZE, sizeof(BYTE), uiLen, pBinFile))
                {
                    _tprintf(_T("**ERROR: read bin file error!\n"));                    
                    fclose(pBinFile);
                    fclose(pCfgFile);
                    if (pBuf != NULL)
                    {
                        free(pBuf);
                        pBuf = NULL;
                    }
                    res = FALSE;
                    return res;
                }
                ulTotalByte += uiLen;

                res = WriteToCfgFile(pBuf, (uiTemp + uiLen + NUM_OF_OPCODE_BYTE), pCfgFile);
                break;
            default:
                _tprintf(_T("**ERROR: opcode couldn't be identified!\n"));
                res = FALSE;
                break;
            }

            if (uiOp == OPCODE_END)
            {
                break;
            }

            if (res == TRUE)
            {
                uiLen = NUM_OF_OPCODE_BYTE;
                memset(pBuf, 0, BUF_SIZE * sizeof(BYTE));
            }
            else
            {
                break;
            }
        }  

        if (res == TRUE)
        {
            _tprintf(_T("--end to de-compile reg_list in binary file.\n\n"));
        }
        else
        {            
            _tprintf(_T("--fail to de-compile reg_list in binary file!\n\n"));
        }
    }
    else
    {
        _tprintf(_T("**ERROR: open binary or config file fail!\n"));        
        res = FALSE;
    }

    fclose(pBinFile);
    fclose(pCfgFile);
    if (pBuf != NULL)
    {
        free(pBuf);
        pBuf = NULL;
    }
    return res;
}