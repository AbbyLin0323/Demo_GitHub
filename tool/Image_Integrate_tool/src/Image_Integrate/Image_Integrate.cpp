// Image_Integrate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Image_Integrate.h"
#include "calcu_exp.h"

IMAGE_HDR g_tHeader;
U32 crc32_table[256];
#define CRC32_PLOY 0xEDB88320

static void GetFileFolder(TCHAR * pFolder)
{
    TCHAR * pTemp = NULL;

    assert(pFolder != NULL);

    GetModuleFileName(NULL, pFolder, MAX_PATH);
    pTemp = _tcsrchr(pFolder, '\\');
    if (NULL != pTemp)
    {
        memset(pTemp, 0, _tcslen(pTemp) * sizeof(TCHAR));
    }

    return;
}

static BOOL CopyBinFile(FILE * pDstFile, FILE * pSrcFile, UINT uiSrcLen)
{
    UINT bytes_in = 0;
    UINT bytes_out = 0;
    UINT total_len = 0;
    BYTE data[BUF_SIZE];

    assert((pSrcFile != NULL) && (pDstFile != NULL));

    while ((bytes_in = fread(data, 1, BUF_SIZE, pSrcFile)) > 0)
    {
        bytes_out = fwrite(data, 1, bytes_in, pDstFile);
        if (bytes_in != bytes_out)
        {
            _tprintf(_T("**ERROR: write size is not equal read size!\n"));
            return FALSE;
        }
        total_len += bytes_out;

        if (total_len == uiSrcLen)
        {
            break;
        }
    }
    _tprintf(_T("CopyBinFile: copy %d(0x%x) bytes.\n"), total_len, total_len);

    if (total_len != uiSrcLen)
    {
        _tprintf(_T("**ERROR: the length of source file is %d bytes, but copy %d bytes!\n"), uiSrcLen, total_len);
        return FALSE;
    }

    return TRUE;
}

//Note: after calling this function, the file pointer will shift to the beginning
static UINT GetLenOfFile(FILE * pFile)
{
    UINT uiLen;
    assert(pFile != NULL);

    fseek(pFile, 0, SEEK_END);
    uiLen = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    return uiLen;
}

//static ULONG Check64ByteAlign(ULONG ulAddr)
//{
//    ULONG ulTemp;
//
//    ulTemp = (ulAddr & 0x3f);
//    if (ulTemp != 0)
//    {
//        _tprintf(_T("address %d is not 64 Byte align, need to adjust!\n"));
//        ulAddr += (0x40 - ulTemp);
//        _tprintf(_T("the new address is %d.\n"), ulAddr);
//    }
//
//    return ulAddr;
//}

static ULONG CheckByteAlign(ULONG ulAddr, UINT uiNum)
{
    ULONG ulTemp;
    UINT uiMask;
    UINT uiMax;

    uiMask = uiNum - 1;
    uiMax = uiNum;

    ulTemp = (ulAddr & uiMask);
    if (ulTemp != 0)
    {
        _tprintf(_T("address %d(0x%x) is not 64 Byte align, need to adjust!\n"), ulAddr, ulAddr);

        ulAddr += (uiMax - ulTemp);
        _tprintf(_T("the new address is %d(0x%x).\n"), ulAddr, ulAddr);
    }

    return ulAddr;
}

//static void MakeCRC32Table(void)
//{
//    U32 c;
//    INT i = 0;
//    INT bit = 0;
//
//    for (i = 0; i < 256; i++)
//    {
//        c = (U32)i;
//
//        for (bit = 0; bit < 8; bit++)
//        {
//            if (c & 1)
//            {
//                c = (c >> 1) ^ CRC32_PLOY;
//            }
//            else
//            {
//                c = c >> 1;
//            }
//        }
//        crc32_table[i] = c;
//    }
//
//    return;
//}

//static U32 MakeCRC32(U32 crc, BYTE * pData, U32 len)
//{
//    assert(pData != NULL);
//    while (len--)
//    {
//        crc = (crc >> 8) ^ (crc32_table[(crc ^ *pData++) & 0xFF]);
//    }
//
//    return crc;
//}

//static U32 CalcCRC32ForFile(FILE * pFile, ULONG ulLen)
//{
//    BYTE buffer[1024];
//    ULONG ulRdLen, ulCount;
//    U32 crc = 0xFFFFFFFF;
//
//    ulRdLen = ulLen;
//    while (ulRdLen > 0)
//    {
//        memset(buffer, 0, sizeof(buffer));
//        ulCount = fread(buffer, 1, sizeof(buffer), pFile);
//        ulRdLen -= ulCount;
//        crc = MakeCRC32(crc, buffer, ulCount);
//    }
//
//    return crc;
//}

static U32 CalcuCRC32(FILE * pFile, ULONG ulLen)
{
    U32 wData = 0;
    ULONG ulCnt;
    UINT i;
    U32 *pBuffer;

    assert((pFile != NULL) && (ulLen != 0));

    pBuffer = NULL;

    pBuffer = (U32 *)malloc(ulLen);
    if (pBuffer)
    {
        memset((U8 *)pBuffer, 0, ulLen);
        ulCnt = fread(pBuffer, 1, ulLen, pFile);
        if (ulLen != ulCnt)
        {
            _tprintf(_T("**ERROR: want to read %d bytes from file, but read %d bytes!\n"), ulLen, ulCnt);
            __debugbreak();
            return 0;
        }

        for (i = 0; i < (ulLen / sizeof(U32)); i++)
        {
            wData ^= *(pBuffer + i);
        }

        free(pBuffer);

        return wData;
    }
    else
    {
        _tprintf(_T("**ERROR: fail to malloc buffer!\n"));
        __debugbreak();
        return 0;
    }
}

static BOOL IntegrateBlToSysFile(TCHAR * pFolder, TCHAR * pFileName, FILE * pSysFile)
{
    TCHAR filePath[MAX_PATH] = { 0 };
    FILE * pFile;
    ULONG ulFileLen = 0;
    ULONG ulLen = 0;
    U8 temp[LEN_OF_HEADER];
    BOOL res = FALSE;

    assert((pFolder != NULL) && (pFileName != NULL) && pSysFile != NULL);

    _stprintf_s(filePath, _T("%s\\%s"), pFolder, pFileName);
    if (0 == _tfopen_s(&pFile, filePath, _T("rb")))
    {
        //the original bootloader already reserve 1K from 0x3C00 ~ 0x3FFF
        //so bootloader part1 begin from 0x4000
        ulFileLen = GetLenOfFile(pFile);

        //set header info for tinyloader and bootloader_p1
        g_tHeader.tinyloader_size = LEN_OF_TINYLOADER + LEN_OF_PARAMETER;
        g_tHeader.bootloader_size = ulFileLen - (LEN_OF_TINYLOADER + LEN_OF_PARAMETER) - LEN_OF_HEADER;

        //copy bootloader part_0 to system binary file   
        fseek(pFile, 0, SEEK_SET);
        res = CopyBinFile(pSysFile, pFile, (LEN_OF_TINYLOADER + LEN_OF_PARAMETER));
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: fail to copy part_0 of %s binary to system binary file!\n"), pFileName);
            goto IntegrateBlToSysFileError;
        }

        //write 1K 0 reserve for Header in system binary
        memset(temp, 0, sizeof(temp));
        ulLen = fwrite(temp, 1, LEN_OF_HEADER, pSysFile);
        if (ulLen != LEN_OF_HEADER)
        {
            _tprintf(_T("**ERROR: fail to reserve 1K FOR Header in system binary file!\n"));
            goto IntegrateBlToSysFileError;
        }

        //copy bootloader part_1 to system binary file
        fseek(pSysFile, OFFSET_BOOTLOADER_P1_IN_SYSTEM, SEEK_SET);
        fseek(pFile, (LEN_OF_TINYLOADER + LEN_OF_PARAMETER + LEN_OF_HEADER), SEEK_SET);
        res = CopyBinFile(pSysFile, pFile, g_tHeader.bootloader_size);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: fail to copy part_1 of %s binary to system binary file!\n"), pFileName);
            goto IntegrateBlToSysFileError;
        }

        fclose(pFile);
        return res;
    }
    else
    {
        _tprintf(_T("**ERROR: Fail to open %s file!\n"), pFileName);
        return res;
    }

IntegrateBlToSysFileError:
    fclose(pFile);
    return FALSE;
}

static BOOL IntegrateBpToSysFile(TCHAR * pFolder, TCHAR * pFileName, FILE * pSysFile)
{
    TCHAR filePath[MAX_PATH] = { 0 };
    FILE * pFile = NULL;
    FTABLE tFtableInBl;
    FTABLE tFtableInBp;
    UINT uiLen;
    INT i;
    BOOL res = FALSE;

    assert((pFolder != NULL) && (pFileName != NULL) && pSysFile != NULL);

    _stprintf_s(filePath, _T("%s\\%s"), pFolder, pFileName);
    if (0 == _tfopen_s(&pFile, filePath, _T("rb")))
    {
        uiLen = GetLenOfFile(pFile);
        if (LEN_OF_PARAMETER != uiLen)
        {
            _tprintf(_T("**ERROR: the length of parameter file should be 4096 bytes, but it's %d bytes!\n"), uiLen);
            goto IntegrateBpToSysFileError;
        }

        //deal with FTALBE
        //save FTABLE from bootloader binary file
        fseek(pSysFile, OFFSET_PARAMETER_IN_SYSTEM, SEEK_SET);
        if (1 != fread(&tFtableInBl, FTABLE_SIZE, 1, pSysFile))
        {
            _tprintf(_T("**ERROR: Fail to read FTABLE from system binary file!\n"));
            goto IntegrateBpToSysFileError;
        }

        //save FTALBE from parameter bianry file
        fseek(pFile, 0, SEEK_SET);
        if (1 != fread(&tFtableInBp, FTABLE_SIZE, 1, pFile))
        {
            _tprintf(_T("**ERROR: Fail to read FTABLE from parameter binary file!\n"));
            goto IntegrateBpToSysFileError;
        }

        //copy function address from FTABLE in bootloader binary to FTABLE in parameter binary
        for (i = 0; i < FTABLE_FUNC_TOTAL; i++)
        {
            tFtableInBp.aInitFuncEntry[i].ulEntryAddr = tFtableInBl.aInitFuncEntry[i].ulEntryAddr;
        }


        //copy parameter binary file to system binary file
        uiLen = GetLenOfFile(pFile);
        fseek(pSysFile, OFFSET_PARAMETER_IN_SYSTEM, SEEK_SET);
        fseek(pFile, 0, SEEK_SET);
        res = CopyBinFile(pSysFile, pFile, uiLen);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: fail to copy %s binary to system binary file!\n"), pFileName);
            goto IntegrateBpToSysFileError;
        }

        //write complete FTABLE to parameter in system binary file
        fseek(pSysFile, OFFSET_PARAMETER_IN_SYSTEM, SEEK_SET);
        uiLen = fwrite(&tFtableInBp, 1, sizeof(FTABLE), pSysFile);
        if (uiLen != LEN_OF_FTABLE)
        {
            _tprintf(_T("**ERROR: Fail to write complete FTABLE to system binary file!\n"));
            goto IntegrateBpToSysFileError;
        }

        fclose(pFile);
        return res;
    }
    else
    {
        _tprintf(_T("**ERROR: Fail to open %s file!\n"), pFileName);
        return res;
    }

IntegrateBpToSysFileError:
    fclose(pFile);
    return FALSE;
}

static BOOL IntegrateFwToSysFile(TCHAR * pFolder, TCHAR * pFileName, FILE * pSysFile, U8 ucMcu)
{
    TCHAR filePath[MAX_PATH] = { 0 };
    FILE * pFile;
    BOOL res = FALSE;
    ULONG ulLen;

    assert((pFolder != NULL) && (pFileName != NULL) && pSysFile != NULL);

    if ((ucMcu != MCU0_IDX) && (ucMcu != MCU1_IDX) && (ucMcu != MCU2_IDX))
    {
        _tprintf(_T("**ERROR: MCU index is wrong, should be mcu0 or mcu1 or mcu2!\n"));
        return FALSE;
    }

    _stprintf_s(filePath, _T("%s\\%s"), pFolder, pFileName);
    if (0 == _tfopen_s(&pFile, filePath, _T("rb")))
    {
        ulLen = GetLenOfFile(pFile);
        //set header info for fw
        if (ucMcu == MCU0_IDX)
        {
            g_tHeader.mcu0fw_size = ulLen;//?????
            fseek(pFile, 0, SEEK_SET);
            g_tHeader.mcu0fw_crc32 = CalcuCRC32(pFile, g_tHeader.mcu0fw_size);
        }
        else if (ucMcu == MCU1_IDX)
        {
            g_tHeader.mcu1fw_size = ulLen - 8;//?????
            fseek(pFile, 0, SEEK_SET);
            g_tHeader.mcu1fw_crc32 = CalcuCRC32(pFile, g_tHeader.mcu1fw_size);
        }
        else
        {
            g_tHeader.mcu2fw_size = ulLen - 8;//?????
            fseek(pFile, 0, SEEK_SET);
            g_tHeader.mcu2fw_crc32 = CalcuCRC32(pFile, g_tHeader.mcu2fw_size);
        }

        //copy binary file to system binary file 
        fseek(pFile, 0, SEEK_SET);
        res = CopyBinFile(pSysFile, pFile, ulLen);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: fail to copy %s binary to system binary file!\n"), pFileName);
            goto IntegrateFwToSysFileError;
        }

        fclose(pFile);
        return res;
    }
    else
    {
        _tprintf(_T("**ERROR: Fail to open %s file!\n"), pFileName);
        return res;
    }

IntegrateFwToSysFileError:
    fclose(pFile);
    return FALSE;
}

static BOOL IntegrateHeaderToSysFile(FILE * pSysFile)
{
    //ULONG ulOffset;
    ULONG ulLen;

    assert(pSysFile != NULL);

    //calculate CRC32
    //the CRC32 for tinyloader should be calculated after integrate the 4K parameter
    fseek(pSysFile, 0, SEEK_SET);
    g_tHeader.tinyloader_crc32 = CalcuCRC32(pSysFile, g_tHeader.tinyloader_size);

    fseek(pSysFile, OFFSET_BOOTLOADER_P1_IN_SYSTEM, SEEK_SET);
    g_tHeader.bootloader_crc32 = CalcuCRC32(pSysFile, g_tHeader.bootloader_size);

    /*ulOffset = OFFSET_BOOTLOADER_P1_IN_SYSTEM + g_tHeader.bootloader_size;
    ulOffset = CheckByteAlign(ulOffset, 64);
    fseek(pSysFile, ulOffset, SEEK_SET);
    g_tHeader.mcu0fw_crc32 = CalcuCRC32(pSysFile, g_tHeader.mcu0fw_size);

    ulOffset += g_tHeader.mcu0fw_size;
    ulOffset = CheckByteAlign(ulOffset, 64);
    fseek(pSysFile, ulOffset, SEEK_SET);
    g_tHeader.mcu1fw_crc32 = CalcuCRC32(pSysFile, g_tHeader.mcu1fw_size);

    ulOffset += g_tHeader.mcu1fw_size;
    ulOffset = CheckByteAlign(ulOffset, 64);
    fseek(pSysFile, ulOffset, SEEK_SET);
    g_tHeader.mcu2fw_crc32 = CalcuCRC32(pSysFile, g_tHeader.mcu2fw_size);*/

    //write header to system.bin
    fseek(pSysFile, OFFSET_HEADER_IN_SYSTEM, SEEK_SET);
    ulLen = fwrite(&g_tHeader, 1, sizeof(g_tHeader), pSysFile);
    if (ulLen != sizeof(g_tHeader))
    {
        _tprintf(_T("**ERROR: fail to write Header info in system binary file!\n"));
        return FALSE;
    }

    return TRUE;
}

//delete the spaces and \t before string
static void DelSpaceBefore(TCHAR * str)
{
    INT i, iLen;
    TCHAR temp[LEN_LINE];

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

    _tcscpy_s(temp, LEN_LINE, (str + i));
    _tcscpy_s(str, LEN_LINE, temp);

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
        __debugbreak();
        return FALSE;
    }

    return TRUE;
}

static BOOL CheckInput(TCHAR * argv[])
{
    /*UINT uiLen;

    assert(argv != NULL);


    _tcsstr(argv[1], _T(".cfg"));*/

    return TRUE;
}

static BOOL GetHeaderInfo(TCHAR * pVal, U8 * pucOffset)
{
    HEADER_INFO *pHeader = HEADER_INFO_LIB;

    assert((pVal != NULL) && (pucOffset != NULL));

    while (_tcsicmp(pHeader->pDesc, _T("")))
    {
        if (!_tcsicmp(pHeader->pDesc, pVal))
        {
            *pucOffset = pHeader->ucDwOftInHeader;
            return TRUE;
        }
        pHeader++;
    }

    return FALSE;
}

static BOOL ParseOneLine(TCHAR * line, UINT uiLine)
{
    TCHAR val_1[LEN_VALUE];
    TCHAR val_2[LEN_VALUE];
    U8 ucDwOffset = 0;
    BOOL res;
    U32 * pDwOfHeader = NULL;
    BOOL bIsAuto;
    U32 uiData;
    U32 uiAddr;

    assert(line != NULL);

    memset(val_1, 0, sizeof(val_1));
    memset(val_2, 0, sizeof(val_2));

    DelSpaceAndNotes(line);

    if (2 == _stscanf_s(line, _T("%[^=]=%s"), val_1, LEN_VALUE, val_2, LEN_VALUE))
    {
        res = GetHeaderInfo(val_1, &ucDwOffset);
        if (TRUE == res)
        {
            uiData = GetOperateResult(val_2, uiLine, &bIsAuto);
            if (bIsAuto == FALSE)
            {
                pDwOfHeader = (U32 *)&g_tHeader + ucDwOffset;
                *pDwOfHeader = uiData;
            }
            else//is "auto"
            {
                if (uiData == 0)
                {
                    //the value for g_tHeader has been assigned before!
                }
                else//mcu1 and mcu2 load address
                {
                    pDwOfHeader = (U32 *)&g_tHeader + ucDwOffset - 5; //the before mcu fw size
                    uiAddr = *pDwOfHeader;
                    pDwOfHeader++;
                    uiAddr += *pDwOfHeader;//address = the before mcu fw size + the before mcu load address
                    uiAddr = CheckByteAlign(uiAddr, uiData);

                    pDwOfHeader = (U32 *)&g_tHeader + ucDwOffset;
                    *pDwOfHeader = uiAddr;
                }

            }
        }
        else
        {
            _tprintf(_T("**ERROR: line_%d input error in cfg file!\n"), uiLine);
            return FALSE;
        }
    }
    else
    {
        _tprintf(_T("**ERROR: line_%d input error in cfg file!\n"), uiLine);
        return FALSE;
    }

    return TRUE;
}

static BOOL ParseCfgAndSetHeader(TCHAR * pFolder, TCHAR * pName)
{

    TCHAR line[LEN_LINE];
    TCHAR path[MAX_PATH];
    FILE * pFile;
    UINT uiLineNo = 0;
    BOOL res, bIsContinue;

    assert((pFolder != NULL) && (pName != NULL));

    _stprintf_s(path, _T("%s\\%s"), pFolder, pName);
    if (0 == _tfopen_s(&pFile, path, _T("r")))
    {
        while (NULL != _fgetts(line, (sizeof(line) / sizeof(line[0])), pFile))
        {
            uiLineNo++;

            res = CheckWillContinue(line, &bIsContinue);
            if (res == FALSE)
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

            res = ParseOneLine(line, uiLineNo);
            if (FALSE == res)
            {
                break;
            }
        }

        fclose(pFile);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: line_%d, parse one line error!\n"), uiLineNo);
            __debugbreak();
        }
        return res;
    }
    else
    {
        _tprintf(_T("**ERROR: fail to open %s file£¡\n"), pName);
        __debugbreak();
        return FALSE;
    }
}

static BOOL AddPatchForMcu0Fw(FILE * pSysFile, TCHAR * pFolder, TCHAR * pMcu1Name, TCHAR * pMcu2Name)
{
    TCHAR filePath[MAX_PATH] = { 0 };
    FILE * pFile;
    BOOL res = FALSE;
    //ULONG ulLen;

    assert((pSysFile != NULL) && (pMcu1Name != NULL) && (pMcu2Name != NULL));

    _stprintf_s(filePath, _T("%s\\%s"), pFolder, pMcu1Name);
    if (0 == _tfopen_s(&pFile, filePath, _T("rb")))
    {
        fseek(pFile, -8, SEEK_END);
        res = CopyBinFile(pSysFile, pFile, 8);
        if (FALSE == res)
        {
            _tprintf(_T("**ERROR: fail to copy 8 bytes from %s binary to system binary file!\n"), pMcu1Name);
            goto AddPatchForMcu0FwError;
        }
        fclose(pFile);
        res = FALSE;

        _stprintf_s(filePath, _T("%s\\%s"), pFolder, pMcu2Name);
        if (0 == _tfopen_s(&pFile, filePath, _T("rb")))
        {
            fseek(pFile, -8, SEEK_END);
            res = CopyBinFile(pSysFile, pFile, 8);
            if (FALSE == res)
            {
                _tprintf(_T("**ERROR: fail to copy 8 bytes from %s binary to system binary file!\n"), pMcu2Name);
                goto AddPatchForMcu0FwError;
            }

            fclose(pFile);
            return res;
        }
        else
        {
            _tprintf(_T("**ERROR: Fail to open %s file!\n"), pMcu2Name);
            return res;
        }
    }
    else
    {
        _tprintf(_T("**ERROR: Fail to open %s file!\n"), pMcu1Name);
        return res;
    }

AddPatchForMcu0FwError:
    fclose(pFile);
    return res;
}

static void ShowInputInfo(void)
{
    _tprintf(_T("\nthere are 7 or 8 input parameters in total for the exe.\n"));
    _tprintf(_T("if parameter_1 is not the absolute root path, the total input is 7 parameters.\n"));
    _tprintf(_T("if parameter_1 is the absolute root path, the total input is 8 parameters.\n"));
    _tprintf(_T("\n:: input parameter_1: the root path for all the input and output files.\n"));
    _tprintf(_T("                    if NULL, set the root path to be the diretory of the EXE.\n"));
    _tprintf(_T(":: input parameter_2: the name of config file, such as \"layout.cfg\".\n"));
    _tprintf(_T(":: input parameter_3: the name of bootloader binary file.\n"));
    _tprintf(_T(":: input parameter_4: the name of bootloader parameter bianry file.\n"));
    _tprintf(_T(":: input parameter_5: the name of MCU_0 firmware bianry file.\n"));
    _tprintf(_T(":: input parameter_6: the name of MCU_1 firmware bianry file.\n"));
    _tprintf(_T(":: input parameter_7: the name of MCU_2 firmware bianry file.\n"));
    _tprintf(_T(":: input parameter_8: the name of system bianry file which will be generated.\n"));
}

static BOOL GetHelpInfo(TCHAR *pStr)
{
    assert(pStr != NULL);

    if ((0 == _tcsicmp(pStr, _T("/?")))
        || (0 == _tcsicmp(pStr, _T("-h")))
        || (0 == _tcsicmp(pStr, _T("-help"))))
    {
        ShowInputInfo();
        return TRUE;
    }
    else
    {
        _tprintf(_T("**ERROR: if want to get help info, please input the following:\n"));
        _tprintf(_T("         \"Image_Integrate.exe /?\" or\n"));
        _tprintf(_T("         \"Image_Integrate.exe -h\" or\n"));
        _tprintf(_T("         \"Image_Integrate.exe -help\"\n"));
        return FALSE;
    }
}

static BOOL IsAbsolutePath(const TCHAR * pPath)
{
	if (pPath == NULL)
        return FALSE;
    else if ((((*pPath >= _T('c')) && (*pPath <= _T('z'))) || ((*pPath >= _T('C')) && (*pPath <= _T('Z'))))
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

int _tmain(int argc, _TCHAR* argv[])
{
    FILE * pSysFile;
    TCHAR folderPath[MAX_PATH] = { 0 };
    TCHAR sysPath[MAX_PATH] = { 0 };
    BOOL res = FALSE;
    ULONG ulLen;
    TCHAR NameOfCfgFile[MAX_FILE_NAME];
    TCHAR NameOfBlFile[MAX_FILE_NAME];
    TCHAR NameOfBpFile[MAX_FILE_NAME];
    TCHAR NameOfMcu0File[MAX_FILE_NAME];
    TCHAR NameOfMcu1File[MAX_FILE_NAME];
    TCHAR NameOfMcu2File[MAX_FILE_NAME];
    TCHAR NameOfSysFile[MAX_FILE_NAME];
          
   /* argc = 9;
    argv[1] = _T("G:\\2016-Projects\\ImageIntegrate_20160811\\Image_Integrate");
    argv[2] = _T("layout.cfg");
    argv[3] = _T("BootLoader_L95_A0_FPGA.bin");
    argv[4] = _T("Bootloader_Parameter.bin");
    argv[5] = _T("NVME_L95_A0_FPGA_MCU0.bin");
    argv[6] = _T("NVME_L95_A0_FPGA_MCU1.bin");
    argv[7] = _T("NVME_L95_A0_FPGA_MCU2.bin");
    argv[8] = _T("System.bin");  
*/
    /*argc = 8;
    argv[1] = _T("layout.cfg");
    argv[2] = _T("BootLoader_L95_A0_FPGA.bin");
    argv[3] = _T("Bootloader_Parameter.bin");
    argv[4] = _T("NVME_L95_A0_FPGA_MCU0.bin");
    argv[5] = _T("NVME_L95_A0_FPGA_MCU1.bin");
    argv[6] = _T("NVME_L95_A0_FPGA_MCU2.bin");
    argv[7] = _T("System.bin");  */

    //show tool version
    _tprintf(_T("\n******Image Integrate tool version: v1.0.5******\n"));

    //get help info
    if (argc == 2)
    {
        if (TRUE == GetHelpInfo(argv[1]))
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }

    memset(NameOfCfgFile, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfBlFile, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfBpFile, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfMcu0File, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfMcu1File, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfMcu2File, 0, sizeof(TCHAR) * MAX_FILE_NAME);
    memset(NameOfSysFile, 0, sizeof(TCHAR) * MAX_FILE_NAME);

    //check whether have the absolute path for all files
    if ((TRUE == IsAbsolutePath(argv[1])) && (argc == 9))//8 input parameters in total
    {
        //CheckInput(argv);
        _tcscpy_s(folderPath, MAX_PATH, argv[1]);
        _tcscpy_s(NameOfCfgFile, MAX_FILE_NAME, argv[2]);
        _tcscpy_s(NameOfBlFile, MAX_FILE_NAME, argv[3]);
        _tcscpy_s(NameOfBpFile, MAX_FILE_NAME, argv[4]);
        _tcscpy_s(NameOfMcu0File, MAX_FILE_NAME, argv[5]);
        _tcscpy_s(NameOfMcu1File, MAX_FILE_NAME, argv[6]);
        _tcscpy_s(NameOfMcu2File, MAX_FILE_NAME, argv[7]);
        _tcscpy_s(NameOfSysFile, MAX_FILE_NAME, argv[8]);
        _stprintf_s(sysPath, _T("%s\\%s"), folderPath, argv[8]);
    }
    else if ((FALSE == IsAbsolutePath(argv[1])) && (argc == 8))//7 input parameters in total
    {
        //CheckInput(argv);
        GetFileFolder(folderPath);
        _tcscpy_s(NameOfCfgFile, MAX_FILE_NAME, argv[1]);
        _tcscpy_s(NameOfBlFile, MAX_FILE_NAME, argv[2]);
        _tcscpy_s(NameOfBpFile, MAX_FILE_NAME, argv[3]);
        _tcscpy_s(NameOfMcu0File, MAX_FILE_NAME, argv[4]);
        _tcscpy_s(NameOfMcu1File, MAX_FILE_NAME, argv[5]);
        _tcscpy_s(NameOfMcu2File, MAX_FILE_NAME, argv[6]);
        _tcscpy_s(NameOfSysFile, MAX_FILE_NAME, argv[7]);
        _stprintf_s(sysPath, _T("%s\\%s"), folderPath, argv[7]);
    }
    else
    {
        _tprintf(_T("**ERROR: please input 7 or 8 input parameters!\n"));
        ShowInputInfo();
        return -1;
    }

    if (0 == _tfopen_s(&pSysFile, sysPath, _T("wb+")))
    {
        //integrate bootloader binary file 
        fseek(pSysFile, 0, SEEK_SET);
        res = IntegrateBlToSysFile(folderPath, NameOfBlFile, pSysFile);
        if (res == FALSE)
        {
            goto MainError;
        }

        //integrate parameter binary file
        res = IntegrateBpToSysFile(folderPath, NameOfBpFile, pSysFile);
        if (res == FALSE)
        {
            goto MainError;
        }

        //integrate fw binary file
        ulLen = GetLenOfFile(pSysFile);
        ulLen = CheckByteAlign(ulLen, 64);
        fseek(pSysFile, ulLen, SEEK_SET);
        //if (argc == 8)//mcu0.bin, mcu1.bin and mcu2.bin

        res = IntegrateFwToSysFile(folderPath, NameOfMcu0File, pSysFile, MCU0_IDX);
        if (res == FALSE)
        {
            goto MainError;
        }

        //add a patch
        fseek(pSysFile, (ulLen + 0x40), SEEK_SET);
        res = AddPatchForMcu0Fw(pSysFile, folderPath, NameOfMcu1File, NameOfMcu2File);
        if (res == FALSE)
        {
            goto MainError;
        }

        //Calculate the mcu0fw CRC
        fseek(pSysFile, ulLen, SEEK_SET);
        g_tHeader.mcu0fw_crc32 = CalcuCRC32(pSysFile, g_tHeader.mcu0fw_size);

        ulLen = GetLenOfFile(pSysFile);
        ulLen = CheckByteAlign(ulLen, 64);
        fseek(pSysFile, ulLen, SEEK_SET);
        res = IntegrateFwToSysFile(folderPath, NameOfMcu1File, pSysFile, MCU1_IDX);
        if (res == FALSE)
        {
            goto MainError;
        }

        ulLen = GetLenOfFile(pSysFile);
        /**
         * Because MCU1&MCU2 file last 8 bytes is used to keep the binary file information
         * file length(4B) + file offset of the section description(4B)
         * This extra 8 bytes is not included into the final integration image.
         */
        ulLen -= 8;
        _tprintf(_T("Adjust MCU1(%s) address %d(0x%x):\n"), NameOfMcu1File, ulLen, ulLen);
        ulLen = CheckByteAlign(ulLen, 64);
        fseek(pSysFile, ulLen, SEEK_SET);
        res = IntegrateFwToSysFile(folderPath, NameOfMcu2File, pSysFile, MCU2_IDX);
        if (res == FALSE)
        {
            goto MainError;
        }


        //the size in g_tHeader should be filled before !!
        //if not set, the load address of mcu1 and mcu2 will be wrong !!
        //the size in cfg file will cover the size
        res = ParseCfgAndSetHeader(folderPath, NameOfCfgFile);
        if (res == FALSE)
        {
            goto MainError;
        }

        //integrate header
        //calculate the CRC32
        res = IntegrateHeaderToSysFile(pSysFile);
        if (res == FALSE)
        {
            goto MainError;
        }

        fclose(pSysFile);
    }
	else
	{
		goto MainError;
	}


    _tprintf(_T("success to integrate!\n"));
    return 0;

MainError:
    _tprintf(_T("**ERROR: fail to integrate!\n"));
	if (pSysFile != NULL)
		fclose(pSysFile);
    return -1;
}

