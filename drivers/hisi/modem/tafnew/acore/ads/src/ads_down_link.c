/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "ads_down_link.h"
#include "ads_debug.h"
#include "mdrv.h"
#include "acpu_reset.h"
#include "ads_filter.h"
#include "ads_dhcp_interface.h"
#include "ads_mntn.h"
#include "ads_mip_pif.h"

#include "mn_comm_api.h"
#include "securec.h"
#include "ads_msg_chk.h"

/*
 * 协议栈打印打点方式下的.C文件宏定义
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_DOWNLINK_C

VOS_UINT32 g_adsDlDiscardPktFlag = VOS_FALSE; /* ADS下行直接丢包开关 */

VOS_INT ADS_DL_CCpuResetCallback(drv_reset_cb_moment_e param, VOS_INT userData)
{
    ADS_CcpuResetInd *msg = VOS_NULL_PTR;

    /* 参数为0表示复位前调用 */
    if (param == MDRV_RESET_CB_BEFORE) {
        ADS_TRACE_HIGH("before reset enter.\n");

        /* 构造消息 */
        msg = (ADS_CcpuResetInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL, sizeof(ADS_CcpuResetInd));
        if (msg == VOS_NULL_PTR) {
            ADS_TRACE_HIGH("before reset: alloc msg failed.\n");
            return VOS_ERROR;
        }

        /* 填写消息头 */
        TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_DL, ACPU_PID_ADS_DL,
                      sizeof(ADS_CcpuResetInd) - VOS_MSG_HEAD_LENGTH);
        msg->msgId = ID_ADS_CCPU_RESET_START_IND;

        /* 发消息 */
        if (TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg) != VOS_OK) {
            ADS_TRACE_HIGH("before reset: send msg failed.\n");
            return VOS_ERROR;
        }

        /* 等待回复信号量初始为锁状态，等待消息处理完后信号量解锁。 */
        if (VOS_SmP(ADS_GetDLResetSem(), ADS_RESET_TIMEOUT_LEN) != VOS_OK) {
            ADS_TRACE_HIGH("before reset VOS_SmP failed.\n");
            ADS_DBG_DL_RESET_LOCK_FAIL_NUM(1);
            return VOS_ERROR;
        }

        ADS_TRACE_HIGH("before reset succ.\n");
        return VOS_OK;
    }
    /* 复位后 */
    else if (param == MDRV_RESET_CB_AFTER) {
        ADS_TRACE_HIGH("after reset enter.\n");

        /* 构造消息 */
        msg = (ADS_CcpuResetInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL, sizeof(ADS_CcpuResetInd));
        if (msg == VOS_NULL_PTR) {
            ADS_TRACE_HIGH("after reset: alloc msg failed.\n");
            return VOS_ERROR;
        }

        /* 填写消息头 */
        TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_DL, ACPU_PID_ADS_DL,
                      sizeof(ADS_CcpuResetInd) - VOS_MSG_HEAD_LENGTH);
        msg->msgId = ID_ADS_CCPU_RESET_END_IND;

        /* 发消息 */
        if (TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg) != VOS_OK) {
            ADS_TRACE_HIGH("after reset: send msg failed.\n");
            return VOS_ERROR;
        }

        ADS_TRACE_HIGH("after reset: succ.\n");
        ADS_DBG_DL_RESET_SUCC_NUM(1);
        return VOS_OK;
    } else {
        return VOS_ERROR;
    }
}

VOS_INT32 ADS_DL_IpfIntCB(VOS_VOID)
{
    if (ADS_GET_IPF_MODE() == ADS_IPF_MODE_INT) {
        ADS_DL_WakeLock();
        ADS_DL_ProcIpfResult();
        ADS_DL_WakeUnLock();
    } else {
        ADS_DL_SndEvent(ADS_DL_EVENT_IPF_RD_INT);
    }
    ADS_DBG_DL_RCV_IPF_RD_INT_NUM(1);

    return VOS_OK;
}

VOS_INT32 ADS_DL_IpfAdqEmptyCB(ipf_adq_empty_e eAdqEmpty)
{
    if (eAdqEmpty == IPF_EMPTY_ADQ0) {
        ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(1);
    } else if (eAdqEmpty == IPF_EMPTY_ADQ1) {
        ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(1);
    } else {
        ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(1);
        ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(1);
    }

    ADS_DL_SndEvent(ADS_DL_EVENT_IPF_ADQ_EMPTY_INT);
    ADS_DBG_DL_RCV_IPF_ADQ_EMPTY_INT_NUM(1);
    return VOS_OK;
}

VOS_VOID ADS_DL_RcvTiAdqEmptyExpired(VOS_UINT32 param, VOS_UINT32 timerName)
{
    /* 触发下行ADQ空中断处理事件 */
    ADS_DL_SndEvent(ADS_DL_EVENT_IPF_ADQ_EMPTY_INT);
    ADS_DBG_DL_ADQ_EMPTY_TMR_TIMEOUT_NUM(1);
}

VOS_VOID ADS_DL_RcvTiCcoreResetDelayExpired(VOS_UINT32 param, VOS_UINT32 timerName)
{
    ADS_DL_RcvCcpuResetStartInd();
}

VOS_VOID ADS_DL_RecordIpfAD(ipf_ad_type_e adType, ipf_ad_desc_s *adDesc, IMM_Zc *immZc, VOS_UINT32 slice)
{
    ADS_IPF_AdRecord *adRecord = VOS_NULL_PTR;

    if (adType < IPF_AD_MAX) {
        adRecord = ADS_DL_GET_IPF_AD_RECORD_PTR(adType);

        if (adRecord->pos >= ADS_IPF_AD_BUFF_RECORD_NUM) {
            adRecord->pos = 0;
        }

        adRecord->adBuff[adRecord->pos].slice = slice;
#ifndef CONFIG_NEW_PLATFORM
        adRecord->astAdBuff[adRecord->ulPos].ulPhyAddr = adDesc->out_ptr1;
#else
        adRecord->adBuff[adRecord->pos].phyAddr = (VOS_VOID *)adDesc->out_ptr1;
#endif
        adRecord->adBuff[adRecord->pos].immZc = immZc;
        adRecord->pos++;
    }
}

VOS_UINT32 ADS_DL_ConfigAdq(ipf_ad_type_e adType, VOS_UINT ipfAdNum)
{
    ipf_ad_desc_s    *adDesc   = VOS_NULL_PTR;
    IMM_Zc           *immZc    = VOS_NULL_PTR;
    VOS_UINT32        poolId;
    VOS_UINT32        tmp;
    VOS_UINT32        cnt;
    VOS_UINT32        len;
    VOS_UINT32        slice;
    VOS_INT32         rslt;

    slice = VOS_GetSlice();

    if (adType == IPF_AD_0) {
        poolId = ADS_IPF_MEM_POOL_ID_AD0;
        len    = ADS_IPF_AD0_MEM_BLK_SIZE;
    } else {
        poolId = ADS_IPF_MEM_POOL_ID_AD1;
        len    = ADS_IPF_AD1_MEM_BLK_SIZE;
    }

    for (cnt = 0; cnt < ipfAdNum; cnt++) {
        immZc = ADS_IPF_AllocMem(poolId, len, IMM_MAC_HEADER_RES_LEN);
        if (immZc == VOS_NULL_PTR) {
            ADS_DBG_DL_ADQ_ALLOC_MEM_FAIL_NUM(1);
            break;
        }

        ADS_DBG_DL_ADQ_ALLOC_MEM_SUCC_NUM(1);

        /* 填写AD描述符: OUTPUT0 ---> 目的地址; OUTPUT1 ---> SKBUFF */
        adDesc = ADS_DL_GET_IPF_AD_DESC_PTR(adType, cnt);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#ifndef CONFIG_NEW_PLATFORM
        adDesc->out_ptr0 = (VOS_UINT32)virt_to_phys((VOS_VOID *)immZc->data);
        adDesc->out_ptr1 = (VOS_UINT32)virt_to_phys((VOS_VOID *)immZc);
#else
        adDesc->out_ptr0 = (modem_phy_addr)(VOS_UINT_PTR)virt_to_phys((VOS_VOID *)immZc->data);
        adDesc->out_ptr1 = (modem_phy_addr)(VOS_UINT_PTR)virt_to_phys((VOS_VOID *)immZc);
#endif
#else
#ifndef CONFIG_NEW_PLATFORM
        adDesc->out_ptr0 = (VOS_UINT32)ADS_IPF_GetMemDma(immZc) + ADS_DMA_MAC_HEADER_RES_LEN;
        adDesc->out_ptr1 = (VOS_UINT32)virt_to_phys((VOS_VOID *)immZc);
#else
        adDesc->out_ptr0 = (modem_phy_addr)((unsigned char*)ADS_IPF_GetMemDma(immZc) + ADS_DMA_MAC_HEADER_RES_LEN);
        adDesc->out_ptr1 = (modem_phy_addr)(VOS_UINT_PTR)virt_to_phys((VOS_VOID *)immZc);
#endif
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0) */

        /* 记录描述符信息 */
        ADS_DL_RecordIpfAD(adType, adDesc, immZc, slice);
    }

    if (cnt == 0) {
        return 0;
    }

    /* 申请到AD才需要配置ADQ */
    rslt = mdrv_ipf_config_dlad(adType, cnt, ADS_DL_GET_IPF_AD_DESC_PTR(adType, 0));
    if (rslt != IPF_SUCCESS) {
        /* 配置失败，释放内存 */
        tmp = cnt;
        for (cnt = 0; cnt < tmp; cnt++) {
            adDesc = ADS_DL_GET_IPF_AD_DESC_PTR(adType, cnt);
#ifndef CONFIG_NEW_PLATFORM
            immZc = (IMM_Zc *)phys_to_virt((unsigned long)adDesc->out_ptr1);
#else
            immZc = (IMM_Zc *)phys_to_virt((VOS_UINT_PTR)adDesc->out_ptr1);  //lint !e661
#endif
            IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
            ADS_DBG_DL_ADQ_FREE_MEM_NUM(1);
        }

        ADS_DBG_DL_ADQ_CFG_IPF_FAIL_NUM(1);
        return 0;
    }

    /* 输出实际配置的AD数目 */
    ADS_DBG_DL_ADQ_CFG_AD_NUM(cnt);
    ADS_DBG_DL_ADQ_CFG_IPF_SUCC_NUM(1);
    return cnt;
}

VOS_VOID ADS_DL_AllocMemForAdq(VOS_VOID)
{
    VOS_INT32  rslt;
    VOS_UINT32 ipfAd0Num = 0;
    VOS_UINT32 ipfAd1Num = 0;
    VOS_UINT32 actAd0Num = 0;
    VOS_UINT32 actAd1Num = 0;

    /* 获取两个ADQ的空闲的AD个数 */
    rslt = mdrv_ipf_get_dlad_num(&ipfAd0Num, &ipfAd1Num);
    if (rslt != IPF_SUCCESS) {
        ADS_DBG_DL_ADQ_GET_FREE_AD_FAIL_NUM(1);
        return;
    }

    ADS_DBG_DL_ADQ_GET_FREE_AD_SUCC_NUM(1);

    /* 首先配置大内存的ADQ1 */
    if (ipfAd1Num != 0) {
        actAd1Num = ADS_DL_ConfigAdq(IPF_AD_1, ipfAd1Num);
        ADS_DBG_DL_ADQ_CFG_AD1_NUM(actAd1Num);
    }

    /* 再配置小内存的ADQ0 */
    if (ipfAd0Num != 0) {
        actAd0Num = ADS_DL_ConfigAdq(IPF_AD_0, ipfAd0Num);
        ADS_DBG_DL_ADQ_CFG_AD0_NUM(actAd0Num);
    }

    /* AD0为空或者AD1为空需要重新启动定时器 */
    if (((actAd0Num == 0) && (ipfAd0Num > ADS_IPF_DLAD_START_TMR_THRESHOLD)) ||
        ((actAd1Num == 0) && (ipfAd1Num > ADS_IPF_DLAD_START_TMR_THRESHOLD))) {
        /* 如果两个ADQ任何一个空且申请不到内存，启定时器 */
        ADS_StartTimer(TI_ADS_DL_ADQ_EMPTY, TI_ADS_DL_ADQ_EMPTY_LEN);
    }
}

VOS_VOID ADS_DL_SendNdClientDataInd(IMM_Zc *immZc)
{
    ADS_NDCLIENT_DataInd *msg  = VOS_NULL_PTR;
    VOS_UINT8            *data = VOS_NULL_PTR;
    VOS_UINT32            dataLen;
    VOS_UINT32            result;

    dataLen = IMM_ZcGetUsedLen(immZc);

    /* 申请消息 */
    msg = (ADS_NDCLIENT_DataInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL,
                                                               sizeof(ADS_NDCLIENT_DataInd) + dataLen - 2);
    if (msg == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendNdClientDataInd: pstMsg is NULL!");
        return;
    }

    /* 填写消息内容 */
    TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_DL, UEPS_PID_NDCLIENT,
                  sizeof(ADS_NDCLIENT_DataInd) + dataLen - 2 - VOS_MSG_HEAD_LENGTH);
    msg->msgId   = ID_ADS_NDCLIENT_DATA_IND;
    msg->modemId = ADS_DL_GET_MODEMID_FROM_IMM(immZc);
    msg->rabId   = ADS_DL_GET_RABID_FROM_IMM(immZc);
    msg->len     = (VOS_UINT16)dataLen;

    /* 拷贝数据内容 */
    data = IMM_ZcGetDataPtr(immZc);
    if (memcpy_s(msg->data, dataLen, data, dataLen) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendNdClientDataInd: memcpy failed!");
    }

    /* 发送消息 */
    result = TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg);
    if (result != VOS_OK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendNdClientDataInd: Send msg fail!");
    }
}

VOS_VOID ADS_DL_SendDhcpClientDataInd(IMM_Zc *immZc)
{
    ADS_DHCP_DataInd *msg  = VOS_NULL_PTR;
    VOS_UINT8        *data = VOS_NULL_PTR;
    VOS_UINT32        dataLen;
    VOS_UINT32        result;

    dataLen = IMM_ZcGetUsedLen(immZc);

    /* 申请消息 */
    msg = (ADS_DHCP_DataInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL, sizeof(ADS_DHCP_DataInd) + dataLen);

    /* 内存申请失败，返回 */
    if (msg == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendDhcpClientDataInd: pstMsg is NULL!");
        return;
    }

    /* 填写消息内容 */
    TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_DL, UEPS_PID_DHCP,
                  sizeof(ADS_DHCP_DataInd) + dataLen - VOS_MSG_HEAD_LENGTH);
    msg->msgId   = ID_ADS_DHCP_DATA_IND;
    msg->modemId = ADS_DL_GET_MODEMID_FROM_IMM(immZc);
    msg->rabId   = ADS_DL_GET_RABID_FROM_IMM(immZc);
    msg->len     = dataLen;

    /* 拷贝数据 */
    data = IMM_ZcGetDataPtr(immZc);
    if (memcpy_s(msg->data, dataLen, data, dataLen) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendDhcpClientDataInd: memcpy fail!");
    }

    /* 调用VOS发送原语 */
    result = TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg);
    if (result != VOS_OK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendDhcpClientDataInd: Send msg fail!");
    }
}

VOS_VOID ADS_DL_SendMipDataInd(IMM_Zc *immZc)
{
    ADS_MIP_DataInd *msg  = VOS_NULL_PTR;
    VOS_UINT8       *data = VOS_NULL_PTR;
    VOS_UINT32       dataLen;

    dataLen = IMM_ZcGetUsedLen(immZc);

    /* 申请消息 */
    msg = (ADS_MIP_DataInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL, sizeof(ADS_MIP_DataInd) + dataLen - 4);

    /* 内存申请失败，返回 */
    if (msg == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendMipDataInd: pstMsg is NULL!");
        return;
    }

    /* 填写消息内容 */
    TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_DL, MSPS_PID_MIP,
                  sizeof(ADS_MIP_DataInd) + dataLen - 4 - VOS_MSG_HEAD_LENGTH);
    msg->msgId   = ID_ADS_MIP_DATA_IND;
    msg->modemId = ADS_DL_GET_MODEMID_FROM_IMM(immZc);
    msg->rabId   = ADS_DL_GET_RABID_FROM_IMM(immZc);
    msg->len     = (VOS_UINT16)dataLen;

    /* 拷贝数据 */
    data = IMM_ZcGetDataPtr(immZc);
    if (memcpy_s(msg->data, dataLen, data, dataLen) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendMipDataInd: memcpy failed!");
    }

    /* 调用VOS发送原语 */
    (VOS_VOID)TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg);
}

VOS_VOID ADS_DL_FreeIpfUsedAd0(VOS_VOID)
{
    VOS_UINT32 i;
    VOS_UINT32 adNum = 0;

    ipf_ad_desc_s *adDesc = VOS_NULL_PTR;

    adDesc = (ipf_ad_desc_s *)PS_MEM_ALLOC(ACPU_PID_ADS_DL, sizeof(ipf_ad_desc_s) * IPF_DLAD0_DESC_SIZE);

    if (adDesc == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FreeIpfUsedAd0: pstAdDesc is null");
        return;
    }

    memset_s(adDesc, (sizeof(ipf_ad_desc_s) * IPF_DLAD0_DESC_SIZE), 0x00,
             (sizeof(ipf_ad_desc_s) * IPF_DLAD0_DESC_SIZE));

    if (mdrv_ipf_get_used_dlad(IPF_AD_0, (VOS_UINT32 *)&adNum, adDesc) == IPF_SUCCESS) {
        /* 释放ADQ0的内存 */
        for (i = 0; i < PS_MIN(adNum, IPF_DLAD0_DESC_SIZE); i++) {
#ifndef CONFIG_NEW_PLATFORM
            IMM_ZcFreeAny((IMM_Zc *)phys_to_virt((unsigned long)adDesc[i].out_ptr1));
#else
            IMM_ZcFreeAny((IMM_Zc *)phys_to_virt((VOS_UINT_PTR)adDesc[i].out_ptr1));
#endif
        }
    } else {
        ADS_DBG_DL_ADQ_GET_IPF_AD0_FAIL_NUM(1);
    }

    ADS_DBG_DL_ADQ_FREE_AD0_NUM(PS_MIN(adNum, IPF_DLAD0_DESC_SIZE));

    PS_MEM_FREE(ACPU_PID_ADS_DL, adDesc);
    adDesc = VOS_NULL_PTR;
}

VOS_VOID ADS_DL_FreeIpfUsedAd1(VOS_VOID)
{
    VOS_UINT32 i;
    VOS_UINT32 adNum = 0;

    ipf_ad_desc_s *adDesc = VOS_NULL_PTR;

    adDesc = (ipf_ad_desc_s *)PS_MEM_ALLOC(ACPU_PID_ADS_DL, sizeof(ipf_ad_desc_s) * IPF_DLAD1_DESC_SIZE);

    if (adDesc == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FreeIpfUsedAd1: pstAdDesc is null");
        return;
    }

    memset_s(adDesc, (sizeof(ipf_ad_desc_s) * IPF_DLAD1_DESC_SIZE), 0x00,
             (sizeof(ipf_ad_desc_s) * IPF_DLAD1_DESC_SIZE));

    if (mdrv_ipf_get_used_dlad(IPF_AD_1, (VOS_UINT32 *)&adNum, adDesc) == IPF_SUCCESS) {
        /* 释放ADQ1的内存 */
        for (i = 0; i < PS_MIN(adNum, IPF_DLAD1_DESC_SIZE); i++) {
#ifndef CONFIG_NEW_PLATFORM
            IMM_ZcFreeAny((IMM_Zc *)phys_to_virt((unsigned long)adDesc[i].out_ptr1));
#else
            IMM_ZcFreeAny((IMM_Zc *)phys_to_virt((VOS_UINT_PTR)adDesc[i].out_ptr1));
#endif
        }
    } else {
        ADS_DBG_DL_ADQ_GET_IPF_AD1_FAIL_NUM(1);
    }

    ADS_DBG_DL_ADQ_FREE_AD1_NUM(PS_MIN(adNum, IPF_DLAD1_DESC_SIZE));

    PS_MEM_FREE(ACPU_PID_ADS_DL, adDesc);
    adDesc = VOS_NULL_PTR;
}

VOS_VOID ADS_DL_SndCdsErrorInd(VOS_UINT16 modemId, VOS_UINT32 rabId)
{
    ADS_CDS_ErrInd *msg = VOS_NULL_PTR;

    /* 申请OSA消息 */
    msg = (ADS_CDS_ErrInd *)ADS_DL_ALLOC_MSG_WITH_HDR(sizeof(ADS_CDS_ErrInd));
    if (msg == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SndCdsErrorInd: Alloc MSG failed.\n");
        return;
    }

    /* 清空消息内容 */
    if (memset_s(ADS_DL_GET_MSG_ENTITY(msg), ADS_DL_GET_MSG_LENGTH(msg), 0x00, ADS_DL_GET_MSG_LENGTH(msg)) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SndCdsErrorInd: memset fail \n");
    }

    /* 填写消息头 */
    ADS_DL_CFG_CDS_MSG_HDR(msg, ID_ADS_CDS_ERR_IND);

    /* 填写消息内容 */
    msg->modemId = modemId;
    msg->rabId   = (VOS_UINT8)rabId;

    /* 发送消息 */
    TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg);
}

VOS_VOID ADS_DL_SndIntraPacketErrorInd(VOS_UINT32 instance, VOS_UINT16 bearerMask)
{
    ADS_PacketErrorInd *msg = VOS_NULL_PTR;

    /* 申请OSA消息 */
    msg = (ADS_PacketErrorInd *)ADS_DL_ALLOC_MSG_WITH_HDR(sizeof(ADS_PacketErrorInd));
    if (msg == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SndIntraPacketErrorInd: Alloc MSG failed.\n");
        return;
    }

    /* 填写消息头 */
    ADS_DL_CFG_INTRA_MSG_HDR(msg, ID_ADS_PACKET_ERROR_IND);

    /* 填写消息内容 */
    msg->modemId    = (VOS_UINT16)instance;
    msg->bearerMask = bearerMask;

    /* 发送消息 */
    ADS_DL_SEND_MSG(msg);
}

VOS_VOID ADS_DL_RecordPacketErrorStats(IMM_Zc *immZc)
{
    ADS_PacketErrorFeedbackCfg *feedbackCfg = VOS_NULL_PTR;
    ADS_BearerPacketErrorStats *pktErrStats = VOS_NULL_PTR;
    ADS_DL_ImmPrivCb           *immPriv     = VOS_NULL_PTR;

    feedbackCfg = ADS_DL_GET_PKT_ERR_FEEDBACK_CFG_PTR();
    if (feedbackCfg->enabled == VOS_FALSE) {
        return;
    }

    immPriv = ADS_DL_IMM_PRIV_CB(immZc);
    if (ADS_DL_GET_PKT_TYPE(immPriv->modemId, immPriv->rabId) == ADS_CDS_IPF_PKT_TYPE_IP) {
        pktErrStats = ADS_DL_GET_PKT_ERR_STATS_PTR(immPriv->modemId, immPriv->rabId);
        if (immPriv->u.ipfResult & ADS_DL_IPF_RESULT_PKT_ERR_FEEDBACK_MASK) {
            if (pktErrStats->errorPktNum == 0) {
                pktErrStats->minDetectExpires = ADS_CURRENT_TICK + feedbackCfg->minDetectDuration;
                pktErrStats->maxDetectExpires = ADS_CURRENT_TICK + feedbackCfg->maxDetectDuration;
            }

            pktErrStats->errorPktNum++;
        }

        if (pktErrStats->errorPktNum != 0) {
            pktErrStats->totalPktNum++;
        }
    }
}

VOS_UINT32 ADS_DL_CalcPacketErrorRate(VOS_UINT32 errorPktNum, VOS_UINT32 totalPktNum)
{
    VOS_UINT32 pktErrRate;

    if ((totalPktNum == 0) || (errorPktNum > totalPktNum)) {
        pktErrRate = 100;
    } else if (errorPktNum <= ADS_DL_ERROR_PACKET_NUM_THRESHOLD) {
        pktErrRate = errorPktNum * 100 / totalPktNum;
    } else {
        pktErrRate = errorPktNum / (totalPktNum / 100);
    }

    return pktErrRate;
}

VOS_VOID ADS_DL_FeedBackPacketErrorIfNeeded(VOS_VOID)
{
    ADS_PacketErrorFeedbackCfg *feedbackCfg = VOS_NULL_PTR;
    ADS_BearerPacketErrorStats *pktErrStats = VOS_NULL_PTR;
    VOS_UINT32                  pktErrRate;
    VOS_UINT16                  bearerMask;
    VOS_UINT32                  instance;
    VOS_UINT32                  rabId;

    feedbackCfg = ADS_DL_GET_PKT_ERR_FEEDBACK_CFG_PTR();
    if (feedbackCfg->enabled == VOS_FALSE) {
        return;
    }

    for (instance = 0; instance < ADS_INSTANCE_MAX_NUM; instance++) {
        bearerMask = 0;

        for (rabId = ADS_RAB_ID_MIN; rabId <= ADS_RAB_ID_MAX; rabId++) {
            pktErrStats = ADS_DL_GET_PKT_ERR_STATS_PTR(instance, rabId);
            if (pktErrStats->errorPktNum != 0) {
                if (ADS_TIME_IN_RANGE(ADS_CURRENT_TICK, pktErrStats->minDetectExpires, pktErrStats->maxDetectExpires)) {
                    pktErrRate = ADS_DL_CalcPacketErrorRate(pktErrStats->errorPktNum, pktErrStats->totalPktNum);

                    if (pktErrRate >= feedbackCfg->pktErrRateThres) {
                        bearerMask = ADS_BIT16_SET(bearerMask, rabId);
                    }

                    pktErrStats->errorPktNum = 0;
                    pktErrStats->totalPktNum = 0;
                }

                if (ADS_TIME_AFTER(ADS_CURRENT_TICK, pktErrStats->maxDetectExpires)) {
                    pktErrStats->errorPktNum = 0;
                    pktErrStats->totalPktNum = 0;
                    continue;
                }
            }
        }

        if (bearerMask != 0) {
            ADS_DL_SndIntraPacketErrorInd(instance, bearerMask);
        }
    }
}

VOS_UINT32 ADS_DL_ValidateImmMem(IMM_Zc *immZc)
{
    ADS_DL_ImmPrivCb *immPriv = VOS_NULL_PTR;

    immPriv = ADS_DL_IMM_PRIV_CB(immZc);

    /* 检查MODEMID */
    if (!ADS_IS_MODEMID_VALID(immPriv->modemId)) {
        ADS_DBG_DL_RMNET_MODEMID_ERR_NUM(1);
        return VOS_ERR;
    }

    /* 检查RABID */
    if (!ADS_IS_RABID_VALID(immPriv->rabId)) {
        ADS_DBG_DL_RMNET_RABID_ERR_NUM(1);
        return VOS_ERR;
    }

    return VOS_OK;
}

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)

VOS_VOID ADS_DL_FreePc5FragQueue(VOS_VOID)
{
    IMM_Zc *immZc = VOS_NULL_PTR;

    while (IMM_ZcQueueLen(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD()) > 0) {
        immZc = IMM_ZcDequeueTail(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD());
        IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
        ADS_DBG_DL_PC5_FRAG_FREE_NUM(1);
    }

    ADS_DL_CLEAR_PC5_FRAGS_LEN();
    ADS_DL_SET_NEXT_PC5_FRAG_SEQ(0);
}

VOS_UINT32 ADS_DL_InsertPc5FragInQueue(IMM_Zc *immZc)
{
    VOS_UINT32 queLen;
    VOS_UINT8  fragSeq;
    VOS_UINT8  nextSeq;

    queLen  = IMM_ZcQueueLen(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD());
    nextSeq = ADS_DL_GET_NEXT_PC5_FRAG_SEQ();
    fragSeq = ADS_GET_PC5_FRAG_SEQ(immZc);

    if ((queLen >= ADS_DL_GET_PC5_FRAGS_QUEUE_MAX_LEN()) || (fragSeq != nextSeq)) {
        IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
        ADS_DBG_DL_PC5_FRAG_FREE_NUM(1);
        ADS_DL_FreePc5FragQueue();
        return VOS_ERR;
    }

    IMM_ZcQueueTail(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD(), immZc);
    ADS_DL_UPDATE_PC5_FRAGS_LEN(IMM_ZcGetUsedLen(immZc));
    ADS_DL_SET_NEXT_PC5_FRAG_SEQ(fragSeq + 1);
    ADS_DBG_DL_PC5_FRAG_ENQUE_NUM(1);
    return VOS_OK;
}

IMM_Zc* ADS_DL_AssemblePc5Frags(VOS_VOID)
{
    IMM_Zc    *frag    = VOS_NULL_PTR;
    IMM_Zc    *newSkb  = VOS_NULL_PTR;
    VOS_UINT8 *skbData = VOS_NULL_PTR;
    VOS_UINT32 dataBufLen;
    errno_t    retVal;

    newSkb = IMM_ZcStaticAlloc(ADS_DL_GET_PC5_FRAGS_LEN());

    if (newSkb == VOS_NULL_PTR) {
        ADS_DL_FreePc5FragQueue();
        return VOS_NULL_PTR;
    }

    skbData    = IMM_ZcPut(newSkb, ADS_DL_GET_PC5_FRAGS_LEN());
    dataBufLen = IMM_ZcGetUsedLen(newSkb);

    while (IMM_ZcQueueLen(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD()) > 0) {
        frag   = IMM_ZcDequeueHead(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD());
        retVal = memcpy_s(skbData, dataBufLen, IMM_ZcGetDataPtr(frag), IMM_ZcGetUsedLen(frag));

        if (retVal != EOK) {
            IMM_ZcFreeAny(frag); /* 由操作系统接管 */
            ADS_DBG_DL_PC5_FRAG_FREE_NUM(1);
            ADS_DL_FreePc5FragQueue();
            IMM_ZcFreeAny(newSkb);
            ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_AssemblePc5Frags: memcpy fail.");
            return VOS_NULL_PTR;
        }

        skbData += IMM_ZcGetUsedLen(frag);
        dataBufLen -= IMM_ZcGetUsedLen(frag);
        IMM_ZcFreeAny(frag);
        ADS_DBG_DL_PC5_FRAG_RELEASE_NUM(1);
    }

    ADS_DL_CLEAR_PC5_FRAGS_LEN();
    ADS_DL_SET_NEXT_PC5_FRAG_SEQ(0);
    ADS_DBG_DL_PC5_FRAG_ASSEM_SUCC_NUM(1);
    return newSkb;
}

VOS_VOID ADS_DL_ProcPc5Pkt(IMM_Zc *immZc)
{
    VOS_UINT32 queLen = IMM_ZcQueueLen(ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD());
    ADS_DBG_DL_RDQ_RX_PC5_PKT_NUM(1);

    ADS_MNTN_REC_DL_PC5_INFO(immZc);

    /* 首分片 */
    if (ADS_PC5_IS_FIRST_FRAG(immZc)) {
        if (queLen != 0) {
            ADS_DL_FreePc5FragQueue();
        }

        ADS_DL_InsertPc5FragInQueue(immZc);
        return;
    }

    /* 中间分片 */
    if (ADS_PC5_IS_MID_FRAG(immZc)) {
        if (queLen == 0) {
            IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
            ADS_DBG_DL_PC5_FRAG_FREE_NUM(1);
        } else {
            ADS_DL_InsertPc5FragInQueue(immZc);
        }
        return;
    }

    /* 尾分片 */
    if (ADS_PC5_IS_LAST_FRAG(immZc)) {
        if (queLen == 0) {
            IMM_ZcFreeAny(immZc);
            ADS_DBG_DL_PC5_FRAG_FREE_NUM(1);
            return;
        }

        if (ADS_DL_InsertPc5FragInQueue(immZc) != VOS_OK) {
            return;
        }

        immZc = ADS_DL_AssemblePc5Frags();
        if (immZc == VOS_NULL_PTR) {
            return;
        }
        ADS_MNTN_REC_DL_PC5_INFO(immZc);
    }

    /* 正常报文 */
    if (ADS_PC5_IS_NORM_PKT(immZc)) {
        if (queLen != 0) {
            ADS_DL_FreePc5FragQueue();
        }
    }

    ADS_DBG_DL_RDQ_RX_PC5_PKT_LEN(IMM_ZcGetUsedLen(immZc));
    (VOS_VOID)mdrv_pcv_xmit(immZc);
    ADS_DBG_DL_RDQ_TX_PC5_PKT_NUM(1);
}
#endif

VOS_VOID ADS_DL_Xmit(IMM_Zc *immZc, VOS_UINT32 instance, VOS_UINT32 rabId)
{
    RCV_DL_DATA_FUNC rcvDlDataFunc = VOS_NULL_PTR;
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    RCV_RD_LAST_DATA_FUNC rcvRdLstDataFunc = VOS_NULL_PTR;
#endif
    IMM_Zc          *lstImmZc = VOS_NULL_PTR;
    VOS_UINT32       exParam;
    VOS_UINT16       ipfResult;
    ADS_PktTypeUint8 ipType;
    VOS_UINT8        exRabId;

    rcvDlDataFunc = ADS_DL_GET_DATA_CALLBACK_FUNC(instance, rabId);
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    rcvRdLstDataFunc = ADS_DL_GET_RD_LST_DATA_CALLBACK_FUNC(instance, rabId);
#endif

    /* 获取缓存的数据 */
    lstImmZc = ADS_DL_GET_LST_DATA_PTR(instance, rabId);
    if (lstImmZc != VOS_NULL_PTR) {
        ipfResult = ADS_DL_GET_IPF_RESULT_FORM_IMM(lstImmZc);
        ipType    = ADS_DL_GET_IPTYPE_FROM_IPF_RESULT(ipfResult);
        exParam   = ADS_DL_GET_DATA_EXPARAM(instance, rabId);
        exRabId   = ADS_BUILD_EXRABID(instance, rabId);

        rcvDlDataFunc = ADS_DL_GET_DATA_CALLBACK_FUNC(instance, rabId);
        if (rcvDlDataFunc != VOS_NULL_PTR) {
            (VOS_VOID)rcvDlDataFunc(exRabId, lstImmZc, ipType, exParam);

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
            /* 最后一个报文 */
            rcvRdLstDataFunc = ADS_DL_GET_RD_LST_DATA_CALLBACK_FUNC(instance, rabId);
            if ((immZc == VOS_NULL_PTR) && (rcvRdLstDataFunc != VOS_NULL_PTR)) {
                rcvRdLstDataFunc(exParam);
            }
#endif
            ADS_DBG_DL_RMNET_TX_PKT_NUM(1);
        } else {
            IMM_ZcFreeAny(lstImmZc); /* 由操作系统接管 */
            ADS_DBG_DL_RMNET_NO_FUNC_FREE_PKT_NUM(1);
        }
    }

    ADS_DL_SET_LST_DATA_PTR(instance, rabId, immZc);
}

VOS_VOID ADS_DL_ProcTxData(IMM_Zc *immZc)
{
    VOS_UINT32 instance;
    VOS_UINT32 rabId;

    if (immZc != VOS_NULL_PTR) {
        instance = ADS_DL_GET_MODEMID_FROM_IMM(immZc);
        rabId    = ADS_DL_GET_RABID_FROM_IMM(immZc);

        ADS_DL_Xmit(immZc, instance, rabId);
    } else {
        for (instance = 0; instance < ADS_INSTANCE_MAX_NUM; instance++) {
            for (rabId = ADS_RAB_ID_MIN; rabId <= ADS_RAB_ID_MAX; rabId++) {
                ADS_DL_Xmit(VOS_NULL_PTR, instance, rabId);
            }
        }
    }
}

IMM_Zc* ADS_DL_RdDescTransImmMem(const ipf_rd_desc_s *rdDesc, VOS_UINT32 slice)
{
    ADS_IPF_RdRecord *rdRecord = VOS_NULL_PTR;
    IMM_Zc           *immZc    = VOS_NULL_PTR;
    VOS_UINT32        cacheLen;

    /* 将OUT指针转换为IMM内存指针 */
    immZc = (IMM_Zc *)phys_to_virt((VOS_UINT_PTR)ADS_DL_GET_IPF_RD_DESC_OUT_PTR(rdDesc));
    if (immZc == VOS_NULL_PTR) {
        return VOS_NULL_PTR;
    }

    /* 记录描述符信息 */
    rdRecord = ADS_DL_GET_IPF_RD_RECORD_PTR();
    if (rdRecord->pos >= ADS_IPF_RD_BUFF_RECORD_NUM) {
        rdRecord->pos = 0;
    }

    rdRecord->rdBuff[rdRecord->pos].slice  = slice;
    rdRecord->rdBuff[rdRecord->pos].attr   = rdDesc->attribute;
    rdRecord->rdBuff[rdRecord->pos].pktLen = rdDesc->pkt_len;
    rdRecord->rdBuff[rdRecord->pos].immZc  = immZc;
    rdRecord->rdBuff[rdRecord->pos].outPtr = (VOS_VOID *)ADS_DL_GET_IPF_RD_DESC_OUT_PTR(rdDesc);
    rdRecord->pos++;

    /* 统一刷CACHE */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
    cacheLen = rdDesc->pkt_len + IMM_MAC_HEADER_RES_LEN;
#else
    cacheLen = rdDesc->pkt_len + ADS_DMA_MAC_HEADER_RES_LEN;
#endif
    ADS_IPF_DL_MEM_UNMAP(immZc, cacheLen);

    /* 设置数据真实长度 */
    IMM_ZcPut(immZc, rdDesc->pkt_len);

    /* 保存描述符信息: u16Result/u16UsrField1 */
    ADS_DL_SAVE_RD_DESC_TO_IMM(immZc, rdDesc);

    return immZc;
}

VOS_VOID ADS_DL_ProcIpfFilterData(IMM_Zc *immZc)
{
    VOS_UINT32 cacheLen;
    VOS_UINT16 ipfResult;

    /* 统一刷CACHE */
    ipfResult = ADS_DL_GET_IPF_RESULT_FORM_IMM(immZc);
    cacheLen  = IMM_ZcGetUsedLen(immZc) + IMM_MAC_HEADER_RES_LEN;

    ADS_IPF_SPE_MEM_UNMAP(immZc, cacheLen);

    /*
     * 匹配下行过滤规则的数据
     * BearId 19: NDClient包，需要转发给NDClient
     * BearId 17: DHCPv6包，需要转发给DHCP
     * [0, 15]定义为非法数据包;
     * [16, 18, 20, 21]目前直接释放
     */
    if (ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(ipfResult) == CDS_ADS_DL_IPF_BEARER_ID_ICMPV6) {
        ADS_DL_SendNdClientDataInd(immZc);
        ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(1);
    } else if (ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(ipfResult) == CDS_ADS_DL_IPF_BEARER_ID_DHCPV6) {
        ADS_DL_SendDhcpClientDataInd(immZc);
        ADS_DBG_DL_RDQ_RX_DHCP_PKT_NUM(1);
    } else if (ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(ipfResult) == CDS_ADS_DL_IPF_BEARER_ID_MIP_ADV) {
        ADS_DL_SendMipDataInd(immZc);
        ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(1);
    } else if (ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(ipfResult) == CDS_ADS_DL_IPF_BEARER_ID_MIP_REG_REPLY) {
        ADS_DL_SendMipDataInd(immZc);
        ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(1);
    } else {
        ADS_DBG_DL_RDQ_FILTER_ERR_PKT_NUM(1);
    }

    ADS_IPF_SPE_MEM_MAP(immZc, cacheLen);
    IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
}

VOS_VOID ADS_DL_ProcIpfFilterQue(VOS_VOID)
{
    IMM_Zc *immZc = VOS_NULL_PTR;

    for (;;) {
        immZc = IMM_ZcDequeueHead(ADS_GET_IPF_FILTER_QUE());
        if (immZc == VOS_NULL_PTR) {
            break;
        }

        ADS_DL_ProcIpfFilterData(immZc);
    }
}

VOS_VOID ADS_DL_ProcIpfResult(VOS_VOID)
{
    ADS_DL_IpfResult *ipfResult = VOS_NULL_PTR;
    ipf_rd_desc_s    *rdDesc    = VOS_NULL_PTR;
    IMM_Zc           *immZc     = VOS_NULL_PTR;
    VOS_UINT32        rdNum     = IPF_DLRD_DESC_SIZE;
    VOS_UINT32        txTimeout = 0;
    VOS_UINT32        cnt;
    VOS_ULONG         lockLevel;
    VOS_UINT32        rnicNapiPollQueLen;
    VOS_UINT32        slice;

    /*
     * IPF_RD_DESC_S中u16Result含义
     * [15]pkt_len_err     IP包长不在配置范围内
     * [14]bd_cd_noeq      BD中len和CD的长度不等错误提示
     * [13]pkt_parse_err   数据解析错误指示
     * [12]bd_pkt_noeq     BD中len和IP包头指示的len不等错误指示
     * [11]head_len_err    IPV4长度错误指示信号，IPV6不检查长度
     * [10]version_err     版本号错误指示
     * [9]ip_type          IP包类型，
     *                     0 表示IPV4
     *                     1 表示IPV6
     * [8]ff_type          分片包第一个分片是否包含上层头指示
     *                     0 表示分片包第一个分片包括上层头(IP包未分片时也为0)
     *                     1 表示分片包第一个分片包括上层头
     * [7:6]pf_type        IP包分片指示类型
     *                     0 表示IP包未分片
     *                     1 表示IP包分片，且为第一个分片，
     *                     2 表示分片，且为最后一个分片，
     *                     3 表示分片，且为中间分片
     * [0:5]bear_id        承载号，如果为0x3F代表所有过滤器不匹配
     */

    /* 获取RD DESC */
    rdDesc = ADS_DL_GET_IPF_RD_DESC_PTR(0);
    mdrv_ipf_get_dlrd(&rdNum, rdDesc);

    /* 获取的RD为0 */
    if (rdNum == 0) {
        ADS_DBG_DL_RDQ_GET_RD0_NUM(1);
        return;
    }

#if (FEATURE_OFF == FEATURE_LTE)
    mdrv_wdt_feed(0);
#endif

    /* 增加RD统计个数 */
    ADS_DBG_DL_RDQ_RX_RD_NUM(rdNum);

    /* 先配置AD，再处理RD */
    /*lint -e571*/
    VOS_SpinLockIntLock(&(g_adsCtx.adsIpfCtx.adqSpinLock), lockLevel);
    /*lint +e571*/
    ADS_DL_AllocMemForAdq();
    VOS_SpinUnlockIntUnlock(&(g_adsCtx.adsIpfCtx.adqSpinLock), lockLevel);

    slice = VOS_GetSlice();

    for (cnt = 0; cnt < rdNum; cnt++) {
        rdDesc = ADS_DL_GET_IPF_RD_DESC_PTR(cnt);

        /* 转换为IMM内存 */
        immZc = ADS_DL_RdDescTransImmMem(rdDesc, slice);
        if (immZc == VOS_NULL_PTR) {
            ADS_DBG_DL_RDQ_TRANS_MEM_FAIL_NUM(1);
            continue;
        }

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
        /* 获取下行PC5 IPF过滤链号 */
        if (rdDesc->fc_head == IPF_LTEV_DLFC) {
            ADS_DL_ProcPc5Pkt(immZc);
        } else
#endif
        {
            /* 校验IMM内存 */
            if (ADS_DL_ValidateImmMem(immZc) != VOS_OK) {
                IMM_ZcFreeAny(immZc); /* 由操作系统接管 */
                continue;
            }

            /* 统计异常数据 */
            ADS_DL_RecordPacketErrorStats(immZc);

            /* 统计下行周期性收到的数据字节数，用于流量查询 */
            ADS_RECV_DL_PERIOD_PKT_NUM(rdDesc->pkt_len);

            /* 待实现 */
            rnicNapiPollQueLen = 0;

            ADS_MNTN_REC_DL_IP_INFO(immZc, ADS_DL_GET_IPF_RSLT_USR_FIELD1_FROM_IMM(immZc), rdDesc->usr_field2,
                                    rdDesc->usr_field3, rnicNapiPollQueLen);

            /* 获取IPF RESULT */
            ipfResult = (ADS_DL_IpfResult *)&(rdDesc->result);

            /* BearId 0x3F: 正常下行数据包需要转发给NDIS/PPP/RNIC */
            if (ipfResult->bearedId == CDS_ADS_DL_IPF_BEARER_ID_INVALID) {
                if (rdDesc->result & ADS_DL_IPF_RESULT_PKT_ERR_MASK) {
                    ADS_DBG_DL_RDQ_RX_ERR_PKT_NUM(1);
                }

                txTimeout = ADS_DL_TX_WAKE_LOCK_TMR_LEN;

                if (g_adsDlDiscardPktFlag == VOS_TRUE) {
                    IMM_ZcFreeAny(immZc);
                    continue;
                }

                ADS_DL_ProcTxData(immZc);
                ADS_DBG_DL_RDQ_RX_NORM_PKT_NUM(1);
            } else {
                if (VOS_CheckInterrupt() != VOS_FALSE) {
                    /* 先入队列缓存 */
                    IMM_ZcQueueTail(ADS_GET_IPF_FILTER_QUE(), immZc);
                } else {
                    ADS_DL_ProcIpfFilterData(immZc);
                }
            }
        }
    }

    /* 推送最后一个数据 */
    ADS_DL_ProcTxData(VOS_NULL_PTR);

    if (IMM_ZcQueueLen(ADS_GET_IPF_FILTER_QUE()) != 0) {
        ADS_DL_SndEvent(ADS_DL_EVENT_IPF_FILTER_DATA_PROC);
    }

    /* 反馈错误提示给协议栈 */
    ADS_DL_FeedBackPacketErrorIfNeeded();

    ADS_MNTN_RPT_ALL_DL_PKT_INFO();

    ADS_DL_EnableTxWakeLockTimeout(txTimeout);
}

VOS_UINT32 ADS_DL_RegDlDataCallback(VOS_UINT8 exRabId, RCV_DL_DATA_FUNC func, VOS_UINT32 exParam)
{
    ADS_DL_RabInfo *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32      instance;
    VOS_UINT32      rabId;

    /* 检查MODEMID */
    instance = ADS_GET_MODEMID_FROM_EXRABID(exRabId);
    if (!ADS_IS_MODEMID_VALID(instance)) {
        ADS_ERROR_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RegDlDataCallback: ModemId is invalid! <ModemId>", instance);
        return VOS_ERR;
    }

    /* 检查RABID */
    rabId = ADS_GET_RABID_FROM_EXRABID(exRabId);
    if (!ADS_IS_RABID_VALID(rabId)) {
        ADS_ERROR_LOG2(ACPU_PID_ADS_DL, "ADS_DL_RegDlDataCallback: RabId is invalid! <ModemId>,<RabId>", instance,
                       rabId);
        return VOS_ERR;
    }

    /* 保存下行数据回调参数 */
    dlRabInfo                = ADS_DL_GET_RAB_INFO_PTR(instance, rabId);
    dlRabInfo->rabId         = rabId;
    dlRabInfo->exParam       = exParam;
    dlRabInfo->rcvDlDataFunc = func;

    return VOS_OK;
}

VOS_UINT32 ADS_DL_RegFilterDataCallback(VOS_UINT8 rabId, ADS_FilterIpAddrInfo *filterIpAddr, RCV_DL_DATA_FUNC func)
{
    ADS_DL_RabInfo *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32      instanceIndex;
    VOS_UINT32      realRabId;

    /* ucRabId的高2个bit表示modem id */
    instanceIndex = ADS_GET_MODEMID_FROM_EXRABID(rabId);
    realRabId     = ADS_GET_RABID_FROM_EXRABID(rabId);

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(realRabId)) {
        ADS_WARNING_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RegFilterDataCallback: ucRabId is", realRabId);
        return VOS_ERR;
    }

    dlRabInfo = ADS_DL_GET_RAB_INFO_PTR(instanceIndex, realRabId);

    /* 设置ADS下行数据过滤回调内容 */
    dlRabInfo->rabId               = realRabId;
    dlRabInfo->rcvDlFilterDataFunc = func;

    /* 保存过滤地址信息 */
    ADS_FilterSaveIPAddrInfo(filterIpAddr);

    return VOS_OK;
}

VOS_UINT32 ADS_DL_DeregFilterDataCallback(VOS_UINT32 rabId)
{
    ADS_DL_RabInfo *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32      instanceIndex;
    VOS_UINT32      realRabId;

    /* ucRabId的高2个bit表示modem id */
    instanceIndex = ADS_GET_MODEMID_FROM_EXRABID(rabId);
    realRabId     = ADS_GET_RABID_FROM_EXRABID(rabId);

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(realRabId)) {
        ADS_WARNING_LOG1(ACPU_PID_ADS_DL, "ADS_DL_DeregFilterDataCallback: ulRabId is", realRabId);
        return VOS_ERR;
    }

    dlRabInfo = ADS_DL_GET_RAB_INFO_PTR(instanceIndex, realRabId);

    /* 去注册ADS下行数据过滤回调内容 */
    dlRabInfo->rcvDlFilterDataFunc = VOS_NULL_PTR;

    /* 清除过滤信息 */
    ADS_FilterReset();

    return VOS_OK;
}

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)

VOS_UINT32 ADS_DL_RegNapiCallback(VOS_UINT8 exRabId, RCV_RD_LAST_DATA_FUNC lastDataFunc, VOS_UINT32 exParam)
{
    ADS_DL_RabInfo *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32      instance;
    VOS_UINT32      rabId;

    /* 检查MODEMID */
    instance = ADS_GET_MODEMID_FROM_EXRABID(exRabId);
    if (!ADS_IS_MODEMID_VALID(instance)) {
        ADS_ERROR_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RegRdLastDataCallback: ModemId is invalid! <ModemId>", instance);
        return VOS_ERR;
    }

    /* 检查RABID */
    rabId = ADS_GET_RABID_FROM_EXRABID(exRabId);
    if (!ADS_IS_RABID_VALID(rabId)) {
        ADS_ERROR_LOG2(ACPU_PID_ADS_DL, "ADS_DL_RegRdLastDataCallback: RabId is invalid! <ModemId>,<RabId>", instance,
                       rabId);
        return VOS_ERR;
    }

    /* 保存下行数据回调参数 */
    dlRabInfo                   = ADS_DL_GET_RAB_INFO_PTR(instance, rabId);
    dlRabInfo->rabId            = rabId;
    dlRabInfo->exParam          = exParam;
    dlRabInfo->rcvRdLstDataFunc = lastDataFunc;

    return VOS_OK;
}
#endif

VOS_UINT32 ADS_DL_RcvTafPdpStatusInd(MsgBlock *msg)
{
    ADS_PdpStatusInd *pdpStatusInd = VOS_NULL_PTR;
    ADS_DL_RabInfo         *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32              instanceIndex;
    ADS_CDS_IpfPktTypeUint8 pktType;

    pdpStatusInd = (ADS_PdpStatusInd *)msg;

    pktType       = ADS_CDS_IPF_PKT_TYPE_IP;
    instanceIndex = pdpStatusInd->modemId;

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(pdpStatusInd->rabId)) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Rab Id is invalid");
        return VOS_ERR;
    }

    if (pdpStatusInd->pdpType == ADS_PDP_PPP) {
        pktType = ADS_CDS_IPF_PKT_TYPE_PPP;
    }

    dlRabInfo = ADS_DL_GET_RAB_INFO_PTR(instanceIndex, pdpStatusInd->rabId);

    /* PDP激活 */
    if (pdpStatusInd->pdpStatus == ADS_PDP_STATUS_ACT) {
        /* 设置ADS下行数据回调的RABID */
        dlRabInfo->rabId   = pdpStatusInd->rabId;
        dlRabInfo->pktType = pktType;
    }
    /* PDP去激活  */
    else if (pdpStatusInd->pdpStatus == ADS_PDP_STATUS_DEACT) {
        /* 清除ADS下行数据回调内容 */
        dlRabInfo->rabId   = ADS_RAB_ID_INVALID;
        dlRabInfo->pktType = ADS_CDS_IPF_PKT_TYPE_IP;
        dlRabInfo->exParam = 0;

        if (pdpStatusInd->cleanRcvCbFlag == ADS_CLEAN_RCV_CB_FUNC_TURE) {
            dlRabInfo->rcvDlDataFunc = VOS_NULL_PTR;
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
            dlRabInfo->rcvRdLstDataFunc = VOS_NULL_PTR;
#endif
        } else {
            ADS_NORMAL_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Not clean ADS DL data call back func");
        }
    } else {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Pdp Status is invalid");
    }

    return VOS_OK;
}

VOS_UINT32 ADS_DL_RcvCcpuResetStartInd(VOS_VOID)
{
    VOS_UINT32 indexNum;

    ADS_TRACE_HIGH("proc reset msg: enter.\n");

    /* 停止所有启动的定时器 */
    for (indexNum = 0; indexNum < ADS_MAX_TIMER_NUM; indexNum++) {
        ADS_StopTimer(ACPU_PID_ADS_DL, indexNum, ADS_TIMER_STOP_CAUSE_USER);
    }

    /* 重置下行上下文 */
    ADS_ResetDlCtx();

    /* 释放IPF的AD */
    ADS_DL_FreeIpfUsedAd1();
    ADS_DL_FreeIpfUsedAd0();

    for (indexNum = 0; indexNum < ADS_DL_ADQ_MAX_NUM; indexNum++) {
        memset_s(ADS_DL_GET_IPF_AD_RECORD_PTR(indexNum), sizeof(ADS_IPF_AdRecord), 0x00, sizeof(ADS_IPF_AdRecord));
    }

    /* 释放信号量，使得调用API任务继续运行 */
    VOS_SmV(ADS_GetDLResetSem());

    ADS_TRACE_HIGH("proc reset msg: leave.\n");
    return VOS_OK;
}

VOS_UINT32 ADS_DL_RcvCcpuResetEndInd(MsgBlock *msg)
{
    VOS_ULONG lockLevel;
    ADS_TRACE_HIGH("proc reset msg: enter.\n");

    memset_s(ADS_DL_GET_IPF_RD_RECORD_PTR(), sizeof(ADS_IPF_RdRecord), 0x00, sizeof(ADS_IPF_RdRecord));

    /* 复位IPF */
    mdrv_ipf_reinit_dlreg();

    /* 重新初始化ADQ */
    /*lint -e571*/
    VOS_SpinLockIntLock(&(g_adsCtx.adsIpfCtx.adqSpinLock), lockLevel);
    /*lint +e571*/
    ADS_DL_AllocMemForAdq();
    VOS_SpinUnlockIntUnlock(&(g_adsCtx.adsIpfCtx.adqSpinLock), lockLevel);

    ADS_TRACE_HIGH("proc reset msg: leave.\n");
    return VOS_OK;
}

VOS_UINT32 ADS_DL_RcvTafMsg(MsgBlock *msg)
{
    MSG_Header *msgTmp;

    msgTmp = (MSG_Header *)msg;

    switch (msgTmp->msgName) {
        case ID_APS_ADS_PDP_STATUS_IND:
            ADS_DL_RcvTafPdpStatusInd(msg);
            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_DL_RcvCdsMsg(MsgBlock *msg)
{
    /* 暂时没有要处理的消息，如果收到消息可能有错误 */
    return VOS_ERR;
}

VOS_VOID ADS_DL_ProcIntraPacketErrorInd(MsgBlock *msg)
{
    ADS_PacketErrorInd *msgTemp = VOS_NULL_PTR;
    VOS_UINT32          rabId;

    msgTemp = (ADS_PacketErrorInd *)msg;

    for (rabId = ADS_RAB_ID_MIN; rabId <= ADS_RAB_ID_MAX; rabId++) {
        if (ADS_BIT16_IS_SET(msgTemp->bearerMask, rabId)) {
            ADS_DL_SndCdsErrorInd(msgTemp->modemId, rabId);
        }
    }
}

VOS_UINT32 ADS_DL_RcvAdsDlMsg(MsgBlock *msg)
{
    MSG_Header *msgTemp;

    msgTemp = (MSG_Header *)msg;

    switch (msgTemp->msgName) {
        case ID_ADS_CCPU_RESET_START_IND:
            /* 等待IPF将下行数据搬空，ADS_DL再复位，等待时长1秒 */
            ADS_StartTimer(TI_ADS_DL_CCORE_RESET_DELAY, ADS_DL_GET_CCORE_RESET_DELAY_TIMER_LEN());
            break;

        case ID_ADS_CCPU_RESET_END_IND:
            ADS_DL_RcvCcpuResetEndInd(msg);
            break;

        case ID_ADS_PACKET_ERROR_IND:
            ADS_DL_ProcIntraPacketErrorInd(msg);
            break;

        default:
            ADS_NORMAL_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RcvAdsDlMsg: rcv error msg id %d\r\n", msgTemp->msgName);
            break;
    }

    return VOS_OK;
}

VOS_VOID ADS_DL_ProcMsg(MsgBlock *msg)
{
    if (msg == VOS_NULL_PTR) {
        return;
    }

    if (TAF_ChkAdsDlFidRcvMsgLen(msg) != VOS_TRUE) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_ProcMsg: message length is invalid!");
        return;
    }
    /* 消息的分发处理 */
    switch (VOS_GET_SENDER_ID(msg)) {
        /* 来自APS的消息 */
        case I0_WUEPS_PID_TAF:
        case I1_WUEPS_PID_TAF:
        case I2_WUEPS_PID_TAF:
            ADS_DL_RcvTafMsg(msg);
            return;

        /* 来自CDS的消息 */
        case UEPS_PID_CDS:
            ADS_DL_RcvCdsMsg(msg);
            return;

        /* 来自ADS DL的消息 */
        case ACPU_PID_ADS_DL:
            ADS_DL_RcvAdsDlMsg(msg);
            return;

        default:
            return;
    }
}

