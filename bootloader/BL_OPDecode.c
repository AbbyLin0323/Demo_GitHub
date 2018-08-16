/****************************************************************************
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
*****************************************************************************
Filename    : BL_OPDecode.c
Version     : Ver 1.0
Author      : Roger.Rao
Date        :  2016.04
Description :
Others      :
Modify      :
20160429    Roger     Create file
****************************************************************************/
#include "BaseDef.h"
#include "BL_OPDefine.h"
#include "Proj_Config.h"
#include "Bootloader.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "HAL_ParamTable.h"



LOCAL U32 BL_CalStrLenWithPadding(U8 ucStrLen, U32 ulPrefixLen);

LOCAL void BL_ExecOpFormatNoParam(OPCODE_TYPE Op);
LOCAL void BL_ExecOpFormatVal(OPCODE_TYPE Op, U32 ulVal);
LOCAL void BL_ExecOpFormatAddr4Val4(OPCODE_TYPE Op, U32 ulAddr, U32 ulVal);


LOCAL void BL_ExecOpSetString(U8 ucStrId, U8 ucStrLen, U32 ulStrAddr);
LOCAL void BL_ExecOpSetHex(U32 ulHexId, U32 ulValueAddr);

LOCAL S32 BL_DecodeOpSegment(const U32 *pOPStart, const U32 leftOPLength, OPCODE_TYPE desireOp);

/*

*/
void BL_DecodeOpsInRAM(const U8 *pOPCodeStart, U32 ulOPCodeLength, OPCODE_TYPE desireOp)
{
    S32 unDecodedLen = 0;
    S32 move_offset= 0;
    const U8 *pCurOpCodeStart = NULL;
    //DBG_Printf("Dec-start\n");
    /*
         normally we set desireOp to OP_ALL which indicates that all Ops are needs to be executed.
            if desireOp == OP_ALL, it will exec all Op.
            otherwise, it will only find and exec the desireOp until hits OPCODE_END.
    */
    if (!BL_CheckAlignU32((U32)pOPCodeStart) || pOPCodeStart == NULL || ulOPCodeLength > 4096)
    {
        DBG_Printf("ERR %s:%d 0x%x:%d\n", __FUNCTION__, __LINE__, pOPCodeStart, ulOPCodeLength);
        DBG_Getch();
    }

    pCurOpCodeStart = pOPCodeStart;
    unDecodedLen = ulOPCodeLength;

     while (unDecodedLen > 0) {
        move_offset = BL_DecodeOpSegment((const U32*)pCurOpCodeStart, unDecodedLen, desireOp);
            if(move_offset > 0)
        {
            // normal case.
            unDecodedLen -= move_offset;
            pCurOpCodeStart += move_offset;
            //DBG_Printf("%d %d:%x:%d\n", __LINE__, move_offset, pCurOpCodeStart, unDecodedLen);
        }else if(move_offset == 0) // meets OPCODE_END.
        {
            //DBG_Printf("OPCODE_END\n");
            break;
        }
        else//move offset < 0 -->error
        {
            DBG_Printf("%d:%d:%x:%d\n", __LINE__, move_offset, pOPCodeStart, unDecodedLen);
            DBG_Getch();
        }
    }

    if (unDecodedLen < 0)
    {
        DBG_Printf("%d EOF unDecodedLen: %d\n", __LINE__, unDecodedLen);
        DBG_Getch();
    }
    else
    {
        // unDecodedLen should be >=0
        if(unDecodedLen > 0)
        {
            DBG_Printf("decdone:%d\n", unDecodedLen);
        }
    }
    //DBG_Printf("Dec-end\n");
}


/*
decode a Op Segment and do the jobs.
return value:
if Ret > 0 : indicates the processed OpCode size.
if Ret == 0: meets OPCODE_END. decode should finish immediately.
if Ret < 0 : there is error happend.
such as Invalid OP Code, etc.
*/
LOCAL S32 BL_DecodeOpSegment(const U32 *pOPStart, const U32 leftOPLength, OPCODE_TYPE desireOp)
{
    U32 Op = 0xFF;
    U8 ucId = 0;
    U8 ucStrLen = 0;
    U32 ulId = 0;
    const U32 *pValue = NULL;
    const U32 *pAddr = NULL;
    const char *pChar = NULL;
    S32 moved_offset = 0;
    BOOL bSkipExec = FALSE;
    PTABLE *pPTable;
    BOOTLOADER_FILE *pBootLoaderFile;
    pBootLoaderFile = BL_GetBootLoaderFile();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    if(!BL_CheckAlignU32((U32)pOPStart))
    {
        DBG_Getch();
    }

    //1. Regconize OpCode
    //2. Exec the Op
    Op = *pOPStart;
    if(desireOp != OPCODE_ALL && Op != desireOp)
    {
        bSkipExec = TRUE;
    }

    //DBG_Printf("Op:%d 0x%x 0x%x\n", Op, *(pOPStart + 1), *(pOPStart + 2));

    switch (Op)
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
        {
            if (leftOPLength < TYPE_OP_ADDR4_VAL4_LEN)
            {
                DBG_Printf("Op:%d LeftOpLen:%d\n", Op, leftOPLength);
                DBG_Getch();
            }

            if(!bSkipExec)
            {
                pAddr = pOPStart + 1;
                pValue = pAddr + 1;
                //DBG_Printf("Op:%d 0x%x 0x%x\n", Op, *pAddr, *(U32*)pValue);
                BL_ExecOpFormatAddr4Val4(Op, *pAddr, *(U32*)pValue);
            }
            moved_offset = TYPE_OP_ADDR4_VAL4_LEN;
        }break;

        case OPCODE_NFC_SETFEATURE:
        {
            if (leftOPLength < TYPE_OP_ADDR4_VAL4_LEN)
            {
                DBG_Getch();
            }
            if(!bSkipExec)
            {
                pAddr = pOPStart + 1;
                pValue = pAddr + 1;
                /* if it is Micron 3D TLC, skip set feature 0x85 for changing to TLC state, only Intel need */
                if (!((U8)((*pAddr)&0xff) == 0x85 && (pPTable->aFlashChipId[0]&0xFF) == 0x2C))
                {
                    BL_SetFeatureAllFlash((U8)((*pAddr)&0xff), *(U32*)pValue);//original: BL_SetFeatureAllFlash((ulAddr&0xff), ulOrMsk);
                    /* if script asserted OPT_NFC_SETFEATURE, then NFC needed, then NfcInit should be done before */
                    if ((U8)((*pAddr)&0xff) == 0x85)
                        BL_ResetAllFlashWaitStatus();
                }
                l_bNfcInitDoneFlag = TRUE;
            }
            moved_offset = TYPE_OP_ADDR4_VAL4_LEN;
        }break;
        case OPCODE_UDELAY:
        case OPCODE_MDELAY:
        {
            if (leftOPLength < TYPE_OP_VAL4_LEN)
            {
                DBG_Getch();
            }
            if(!bSkipExec)
            {
                pValue = pOPStart + 1;
                //DBG_Printf("[%d]:%d:%d\n", __LINE__, Op, *(U32*)pValue);
                BL_ExecOpFormatVal(Op, *(U32*)pValue);
            }
            moved_offset = TYPE_OP_VAL4_LEN;
        }break;

        case OPCODE_NFC_INTERFACE_INIT:
        case OPCODE_NFC_RESET:
        case OPCODE_NFC_UPDATE_PUMAPPING:
        {
            if (leftOPLength < TYPE_OP_NOPARAM_LEN)
            {
                DBG_Getch();
            }
            if(!bSkipExec)
            {
                BL_ExecOpFormatNoParam(Op);
            }
            moved_offset = TYPE_OP_NOPARAM_LEN;
        }break;

        case OPCODE_END:
        {
            DBG_Printf("Op End\n");
            moved_offset = 0;
        }break;

        case OPCODE_SETSTR:
        {
            //set pointer
            ucId = *(U8*)(pOPStart + 1);
            ucStrLen = *((U8*)(pOPStart + 1) + 1);
            pChar = (char*)(pOPStart + 1) + 2;
            moved_offset = BL_CalStrLenWithPadding(ucStrLen, 4+1+1);
            if(!bSkipExec)
            {
                DBG_Printf("sstr:%d:%d:%s\n", ucId, ucStrLen, pChar);
                BL_ExecOpSetString(ucId, ucStrLen, (U32)pChar);
            }


        }break;
        case OPCODE_SETHEX:
        {
            //set pointer
            pAddr = pOPStart + 1;
            ulId = *pAddr;
            pValue = pOPStart + 2;
            if(!bSkipExec)
            {
                DBG_Printf("sethex:%d:pOPStart:0x%x\n", ulId, pOPStart);
                BL_ExecOpSetHex(ulId, (U32)pValue);
            }
            moved_offset = TYPE_OP_ADDR4_VAL4_LEN;
        }break;
        case OPCODE_VER:
        {
            //DBG_Printf("BLVer:pOPStart:%x\n", pOPStart);
            pAddr = pOPStart + 1;
            ucStrLen = *(U8*)pAddr;
            pChar = (U8*)pAddr + 1;
            DBG_Printf("BLVer:%s\n", (char*)pChar);
            moved_offset = BL_CalStrLenWithPadding(ucStrLen, 4+1);
            if(!bSkipExec)
            {
                //empty.
            }
            //DBG_Printf("moved_offset=%d ucStrLen:%d\n", moved_offset, ucStrLen);
        }break;
        case OPCODE_VENDOR_OP:
        {
            DBG_Printf("VenderOp\n");
            DBG_Getch();
            if(!bSkipExec)
            {
                //empty.
            }
            moved_offset = 0;//TODO
        }break;
        default:
            DBG_Printf("BadOP:%d\n",*pOPStart);
            DBG_Getch();
            break;
    }
    //DBG_Printf("OpSegDec:e moved:%d\n", moved_offset);
    return moved_offset;
}



LOCAL void BL_ExecOpFormatNoParam(OPCODE_TYPE Op)
{
    U32 ulSys0CEMap;
    U32 ulSys1CEMap;

    switch (Op)
    {
        case OPCODE_NFC_RESET:
        {
            //DBG_Printf("NFC_RESET\n");
            BL_ResetAllFlash();
        }break;
        /*
        case OPCODE_NFC_SYNC_RESET:
        {
            //DBG_Printf("NFC_SYNC_RESET\n");
            BL_SyncResetAllFlash();
        }break;
        */
        case OPCODE_NFC_UPDATE_PUMAPPING:
        {
            //DBG_Printf("NFC_MAPPING\n");
            BL_NFC_UpdatePuMapping();
        }break;
        case OPCODE_NFC_INTERFACE_INIT:
        {
            //DBG_Printf("NFC_IFINIT\n");
            BL_NFC_InterfaceInit();
        }break;
        default:
            DBG_Printf("Invalid Op:%d\n", Op);
            DBG_Getch();
        break;

    }

}
LOCAL void BL_ExecOpFormatVal(OPCODE_TYPE Op, U32 ulVal)
{
    U8 i;
    switch (Op)
    {
        case OPCODE_UDELAY:
        {
            HAL_DelayUs(ulVal);
        }break;

        case OPCODE_MDELAY:
        {
            for(i = 0; i < ulVal; i++)
            {
                HAL_DelayUs(1000);
            }

        }break;

        default:
            DBG_Printf("[%d]Op:%d\n", __LINE__, Op);
            DBG_Getch();
            break;
    }
}
LOCAL void BL_ExecOpFormatAddr4Val4(OPCODE_TYPE Op, U32 ulAddr, U32 ulVal)
{
    volatile U32 *pAddr = (U32*)ulAddr;
    volatile U16 *pAddr16 = (U16*)ulAddr;
    volatile U8 *pAddr8 = (U8*)ulAddr;

    U32 ulUartMP;
    /*get GPIO8 value, if low, uart mp mode*/
    ulUartMP = rGPIO(0x18);
    if (!(ulUartMP & 0x00000100)) {
        if ((ulAddr >= 0x1ff83600) && (ulAddr <= 0x1ff83aff)) {
            return;
        }
    }
    switch (Op)
    {
        case OPCODE_MEMW32:
            *(U32*)pAddr = ulVal;
            break;
        case OPCODE_SETBIT32:
            *(U32*)pAddr |= ulVal;
            break;
        case OPCODE_CLRBIT32:
            *(U32*)pAddr &= ~ulVal;
            break;
        case OPCODE_MWAIT32:
            while( (*pAddr & ulVal) != ulVal)
            {
                ;
            }
            break;

        case OPCODE_MEMW16:
            *(U16*)pAddr = (U16)(ulVal & 0xffff);
            break;
        case OPCODE_SETBIT16:
            *(U16*)pAddr |= (U16)(ulVal & 0xffff);
            break;
        case OPCODE_CLRBIT16:
            *(U16*)pAddr &= ~((U16)(ulVal & 0xffff));
            break;
        case OPCODE_MWAIT16:
            while( (*pAddr16 & ((U16)(ulVal & 0xffff))) != (U16)(ulVal & 0xffff))
            {
                ;
            }
            break;

        case OPCODE_MEMW8:
            *(U8*)pAddr = (U8)(ulVal & 0xff);
            break;
        case OPCODE_SETBIT8:
            *(U8*)pAddr |= (U8)(ulVal & 0xff);
            break;
        case OPCODE_CLRBIT8:
            *(U8*)pAddr &= ~((U8)(ulVal & 0xff));
            break;
        case OPCODE_MWAIT8:
            while( (*pAddr8 & ((U8)(ulVal & 0xff))) != (U8)(ulVal & 0xff))
            {
                ;
            }
            break;
        default:
            DBG_Printf("%d Op:%d\n", __LINE__,Op);
            DBG_Getch();
            break;
    }
}


LOCAL void BL_ExecOpSetString(U8 ucStrId, U8 ucStrLen, U32 ulStrAddr)
{
    g_StrDataIndexTable[ucStrId] = (U16)(ulStrAddr - (OTFB_BOOTLOADER_BASE + BL_CODE_SIZE));
    //DBG_Printf("%d:0x%x\n", ucStrId, g_StrDataIndexTable[ucStrId]);
}

LOCAL void BL_ExecOpSetHex(U32 ulHexId, U32 ulValueAddr)
{
    //DBG_Printf("sethex:%d:0x%x\n", ulHexId, ulValueAddr);
    g_HexDataIndexTable[ulHexId] = (U16)(ulValueAddr - (OTFB_BOOTLOADER_BASE + BL_CODE_SIZE));
    //DBG_Printf("%d:0x%x\n", ulHexId, g_HexDataIndexTable[ulHexId]);

}

BOOL BL_CheckAlignU32(U32 ulAddr)
{
    if(ulAddr & 0x3 !=0)
    {
        DBG_Printf("0x%x Not U32 Algin\n", ulAddr);
        return FALSE;
    }
    return TRUE;
}

/*
since Xtensa CPU's limitation, we want every U32 Access is U32 Align for simplicity.
We need to calculate the actual start of next OpCode, which is U32 Align.
*/
LOCAL U32 BL_CalStrLenWithPadding(U8 ucStrLen, U32 ulPrefixLen)
{
    U32 ulLengthInBytesAlign;
    U32 ulActualLen = ulPrefixLen + ucStrLen + 1;//'\0'
    if(ulActualLen % sizeof(U32))
    {
        //not U32 align.
        ulLengthInBytesAlign = (ulActualLen / 4 + 1) * 4;
    }else{
        //is U32 align
        ulLengthInBytesAlign = ulActualLen;
    }
    //DBG_Printf("padding:%d\n", ulLengthInBytesAlign);
    return ulLengthInBytesAlign;
}
