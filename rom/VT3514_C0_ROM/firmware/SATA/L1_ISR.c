/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_ISR.c
Version     :Ver 1.0
Author      :Blakezhang
Date        :2013.01.07
Description :
Others      :
Modify      :2013.01.07     blakezhang     001 first create
****************************************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"

U8 L1_GetPUFromLba(U32 LbaAddr)//temp function. just for compile in ramdisk without cache.
{
    return (LbaAddr>>SEC_PER_BUF_BITS)%PU_NUM;
}


void L1_IntCallBackProxy(INTCALLBACKPARAM *pCBParam)
{
    U8 ucIntSrc, ucIntEvent;
    U8 ucNewCmdTag;
#ifdef HCMD_PU_QUEUE
    U8 ucTargetPu, ucCmdCode;
#endif

    ucIntSrc = pCBParam->ucIntSrc;
    ucIntEvent = pCBParam->ucEventType;

    switch ( ucIntSrc ) {
        case PARAM_INTSRC_SATA:
            if ( CBINT_SATACMD == ucIntEvent ) 
            {
#ifdef HCMD_PU_QUEUE
                /* Acquire command tag from event parameter. */
                ucNewCmdTag = (U8) (pCBParam->usShortParam & 0xFF);
                ucCmdCode   = HostCmdSlot[ucNewCmdTag].ucCmdCode;

                if ( (ATA_CMD_READ_FPDMA_QUEUED    == ucCmdCode) 
                   ||(ATA_CMD_WRITE_FPDMA_QUEUED   == ucCmdCode))
                {
                    ucTargetPu = L1_GetPUFromLba(HostCmdSlot[ucNewCmdTag].ulCmdLba);
                }
                else
                {
                    ucTargetPu = 0;
                }
                
                /* add host cmd to cmd slot */
                L1_AddHCmdtoPu(ucNewCmdTag, ucTargetPu);

#else
                /* Acquire command tag from event parameter. */
                ucNewCmdTag = (U8)pCBParam->usShortParam;

                /* add host cmd to cmd slot */
                L1_AddNewHostCMD(ucNewCmdTag);
#endif
                
            }

            break;

        default:
            break;
    }

    return;
}

/********************** FILE END ***************/

