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
Filename    :L0_Pio.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_SataIO.h"
#include "L0_Pio.h"
#include "L0_Interface.h"

PIOINFO gPIOInfoBlock;
volatile BOOL g_bPioDataReceived;

void L0_PioInit(void)
{
    COM_MemZero((U32 *)&gPIOInfoBlock, sizeof(PIOINFO) / sizeof(U32));
    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NOCMD;
    return;
}

void L0_SataSetupPIOInOut(BOOL bPIOIn, U8 ucDRQLen, U32 ulCmdTotalSecLen)
{
    gPIOInfoBlock.bCmdRead = (TRUE == bPIOIn) ? TRUE : FALSE;
    gPIOInfoBlock.ucDrqLen = ucDRQLen;
    gPIOInfoBlock.ulCmdTotalSecLen = ulCmdTotalSecLen;
    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NEWCMD;

    return;
}

void L0_HandlePioProtocol(void)
{
    BOOL bFirstDRQBlk, bLastDRQBlk;

    static U32 ulRemainSectorCount;
    static U32 ulTotalSectorCount;
    static U8 ucDRQBlkLen, ucCurrDataFISLen;

    // process the pio protocol based on the state
    switch(gPIOInfoBlock.ucCurrPIOState) 
    {
        // there's an incoming new pio host command, initialize
        // related structures
        case SATA_PIO_NEWCMD:
            // reset the g_bPioDataReceived flag,
            // which indicate if a pio data from the
            // host has been received
            g_bPioDataReceived = FALSE;
                    
            // set the remaining sector count and the
            // total sector count
            ulRemainSectorCount = gPIOInfoBlock.ulCmdTotalSecLen;
            ulTotalSectorCount = ulRemainSectorCount;

            // set the DRQ block length
            ucDRQBlkLen = gPIOInfoBlock.ucDrqLen;
#ifndef SIM
            /* PIO DATA OUT patch: wait SDC get first write DSG before trigger first PIO Setup FIS */
            if(FALSE == gPIOInfoBlock.bCmdRead)
            {
                if (1 != (rSDMAC_ConfigReg4 & 0x1))
                {
                      break;
                }
            }
#endif
            // advance the state to pio setup
            gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;

            break;
            
        // pio setup stage, a pio setup fis will be sent to the
        // host at the end of this stage
        case SATA_PIO_SETUP:
            // first check if there's any pending fis in the
            // system, if there is, do nothing and break
            if (FALSE == HAL_SataIsFISXferAvailable())
            {
                break;
            }
            
            // check if the current drq block is the first one of
            // this pio host command
            bFirstDRQBlk = (ulRemainSectorCount == ulTotalSectorCount) ? TRUE : FALSE;
            
            // determine the length of the current drq block based
            // on the remaining sector count of the current pio
            // host command
            if(ulRemainSectorCount > (U32)ucDRQBlkLen) 
            {
                ucCurrDataFISLen = ucDRQBlkLen;
                bLastDRQBlk = FALSE;
            }
            else 
            {
                // the current drq block is the last one
                ucCurrDataFISLen = ulRemainSectorCount;
                bLastDRQBlk = TRUE;
            }

            // construct and send the pio setup fis based on the
            // information we've gathered
            HAL_SataConstructAndSendPIOSetupFIS(bFirstDRQBlk, bLastDRQBlk, ucCurrDataFISLen, gPIOInfoBlock.bCmdRead);

            // set the next stage based on the command type
            if(TRUE == gPIOInfoBlock.bCmdRead)
            {
                // pio host read
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_DATA_IN;
            }
            else
            {
                // pio host write
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_DATA_OUT;
            }
            break;

        // pio data in(pio host read), a pio data fis will be sent
        // to the host in this stage
        case SATA_PIO_DATA_IN:
            // check if there's any pending fis, if there is, do
            // nothing and break;
            if(FALSE == HAL_SataIsFISXferAvailable())
            {
                break;
            }

            // mask command interrupt to ensure hardware DIPM correctness
            HAL_SataEnableCmdRcvInt(FALSE);

            // send the pio data fis
            HAL_SataSendPIODataFIS();

            // wait until the FIS sending OK
            while (FALSE == HAL_SataIsFISXferAvailable())
            {
                ;
            }

            // update the remaining sector count of the current
            // pio host command
            ulRemainSectorCount -= ucCurrDataFISLen;
            
            if(0 == ulRemainSectorCount) 
            {
                // all data has been transfered, move to the
                // finish state
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_FINISH;

                // clear PioCmdPending
                HAL_SataClearPIOCmdPending();
            }
            else
            {
                // data fis has been sent and the command hasn't
                // finished yet, move back to pio setup stage
                gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;
            }

            // Unmask command interrupt
            HAL_SataEnableCmdRcvInt(TRUE);
            break;           

        // pio data out(pio host write)
        case SATA_PIO_DATA_OUT:
#ifndef SIM
            HAL_DisableMCUIntAck();
#endif
            // check if a pio data fis from the host has been
            // received
#ifndef SIM
            if(TRUE == g_bPioDataReceived) 
#endif
            {
                // clear the flag
                g_bPioDataReceived = FALSE;

                // update the remaining sector count
                ulRemainSectorCount -= ucCurrDataFISLen;

                // set the next stage based on the remaining
                // sector count
                if(0 == ulRemainSectorCount)
                {
                    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_FINISH;
                }
                else
                {
                    gPIOInfoBlock.ucCurrPIOState = SATA_PIO_SETUP;
                }
            }
#ifndef SIM
            HAL_EnableMCUIntAck();
#endif
            // note that we will stay in the data out stage until
            // a pio data fis from the host is received
            break;
        
        case SATA_PIO_FINISH:
            // wait SDC finish all data FIS tranferring before going to next stage
            // cope with the case that FIS isn't sent successfully but COMRESET clear IOControl
            if ((FALSE == HAL_SataIsFISXferAvailable()) || (FALSE == L0_CheckSCQAllEmpty()))
            {
                break;
            }

            // clear the pio info block
            gPIOInfoBlock.ucCurrPIOState = SATA_PIO_NOCMD;

            // TODO:
            // for PIO-Out, send success D2H FIS and clear PioCmdPending in L0_SataCompleteCmd()

            break;

        default:
            DBG_Printf("pio state error\n");
            DBG_Getch();
            break;
    } // switch(gPIOInfoBlock.ucCurrPIOState) 

    return;
}

void L0_ReceivePioDataIsr(void)
{
    g_bPioDataReceived = TRUE;
    return;
}


