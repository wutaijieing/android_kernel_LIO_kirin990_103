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
#include "ads_up_link.h"
#include "acpu_reset.h"
#include "ads_filter.h"
#include "ads_debug.h"
#include "ads_mntn.h"
#include "ads_ndis_interface.h"
#include "mn_comm_api.h"
#include "securec.h"
#if (defined(CONFIG_HUAWEI_BASTET) || defined(CONFIG_HW_DPIMARK_MODULE))
#include <net/inet_sock.h>
#include <linux/version.h>
#endif

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#include <linux/skbuff.h>
#endif
#include "ads_msg_chk.h"


/*
 * Э��ջ��ӡ��㷽ʽ�µ�.C�ļ��궨��
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_UPLINK_C

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_UINT32 g_pc5NodeCnt = 0;
#endif

VOS_INT ADS_UL_CCpuResetCallback(drv_reset_cb_moment_e param, VOS_INT iUserData)
{
    ADS_CcpuResetInd *msg = VOS_NULL_PTR;

    /* ����Ϊ0��ʾ��λǰ���� */
    if (param == MDRV_RESET_CB_BEFORE) {
        ADS_TRACE_HIGH("before reset: enter.\n");

        ADS_SetUlResetFlag(VOS_TRUE);

        /* ������Ϣ */
        msg = (ADS_CcpuResetInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_UL, sizeof(ADS_CcpuResetInd));
        if (msg == VOS_NULL_PTR) {
            ADS_TRACE_HIGH("before reset: alloc msg failed.\n");
            return VOS_ERROR;
        }

        /* ��д��Ϣͷ */
        TAF_CfgMsgHdr((MsgBlock *)msg, ACPU_PID_ADS_UL, ACPU_PID_ADS_UL,
                      sizeof(ADS_CcpuResetInd) - VOS_MSG_HEAD_LENGTH);
        msg->msgId = ID_ADS_CCPU_RESET_START_IND;

        /* ����Ϣ */
        if (TAF_TraceAndSendMsg(ACPU_PID_ADS_UL, msg) != VOS_OK) {
            ADS_TRACE_HIGH("before reset: send msg failed.\n");
            return VOS_ERROR;
        }

        /* �ȴ��ظ��ź�����ʼΪ��״̬���ȴ���Ϣ��������ź��������� */
        if (VOS_SmP(ADS_GetULResetSem(), ADS_RESET_TIMEOUT_LEN) != VOS_OK) {
            ADS_TRACE_HIGH("before reset: VOS_SmP failed.\n");
            ADS_DBG_UL_RESET_LOCK_FAIL_NUM(1);
            return VOS_ERROR;
        }

        ADS_TRACE_HIGH("before reset: succ.\n");
        return VOS_OK;
    }
    /* ��λ�� */
    else if (param == MDRV_RESET_CB_AFTER) {
        ADS_TRACE_HIGH("after reset enter.\n");

        ADS_SetUlResetFlag(VOS_FALSE);

        ADS_TRACE_HIGH("after reset: succ.\n");
        ADS_DBG_UL_RESET_SUCC_NUM(1);
        return VOS_OK;
    } else {
        return VOS_ERROR;
    }
}

VOS_VOID ADS_UL_SndDplConfigInd(VOS_BOOL bDplNeededFlg)
{
    ADS_DplConfigInd *dplCfgInd = VOS_NULL_PTR;

    /* ������Ϣ */
    dplCfgInd = (ADS_DplConfigInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_UL, sizeof(ADS_DplConfigInd));
    if (dplCfgInd == VOS_NULL_PTR) {
        ADS_TRACE_HIGH("alloc msg failed.\n");
        return;
    }

    /* �����Ϣ���� */
    if (memset_s(ADS_UL_GET_MSG_ENTITY(dplCfgInd), ADS_UL_GET_MSG_LENGTH(dplCfgInd), 0x00,
                 ADS_UL_GET_MSG_LENGTH(dplCfgInd)) != EOK) {
        ADS_TRACE_HIGH("memset faild.\n");
    }

    /* ��д��Ϣͷ */
    ADS_UL_CFG_MSG_HDR(dplCfgInd, ACPU_PID_ADS_UL, ID_ADS_DPL_CONFIG_IND);

    dplCfgInd->dplNeededFlg = bDplNeededFlg;

    /* ����Ϣ */
    (VOS_VOID)TAF_TraceAndSendMsg(ACPU_PID_ADS_UL, dplCfgInd);
}

VOS_VOID ADS_UL_StartDsFlowStats(VOS_UINT32 instance, VOS_UINT32 rabId)
{
    /* ������ж��д���, ����������ͳ�ƶ�ʱ�� */
    if (ADS_UL_IsQueueExistent(instance, rabId) == VOS_OK) {
        ADS_StartTimer(TI_ADS_DSFLOW_STATS, TI_ADS_DSFLOW_STATS_LEN);
    }
}

VOS_VOID ADS_UL_StopDsFlowStats(VOS_UINT32 instance, VOS_UINT32 rabId)
{
    /* ����������ж����Ѳ����ڣ���ֹͣ����ͳ�ƶ�ʱ�����������ͳ����Ϣ */
    if (ADS_UL_IsAnyQueueExist() == VOS_FALSE) {
        ADS_StopTimer(ACPU_PID_ADS_UL, TI_ADS_DSFLOW_STATS, ADS_TIMER_STOP_CAUSE_USER);
        ADS_InitStatsInfoCtx();
    }
}

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)

VOS_INT ADS_Pc5DataIngress(IMM_Zc *immZc)
{
    IMM_Zc    *fragSkb = VOS_NULL_PTR;
    VOS_UINT32 pktLen;
    VOS_UINT8  fragSeq = 0;

    /* �ж��Ƿ�Ϊ�����ݰ� */
    if (immZc == VOS_NULL_PTR) {
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "pkt is null.");
        return VOS_ERROR;
    }

    ADS_DBG_UL_PC5_RX_PKT_NUM(1);
    pktLen = IMM_ZcGetUsedLen(immZc);
    ADS_DBG_UL_PC5_RX_PKT_LEN(pktLen);

    if ((pktLen == 0) || (pktLen > IMM_MAX_PC5_PKT_LEN)) {
        IMM_ZcFreeAny(immZc); /* �ɲ���ϵͳ�ӹ� */
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "pkt len err.");
        return VOS_ERROR;
    }

    while (IMM_ZcGetUsedLen(immZc) > IMM_MAX_ETH_FRAME_LEN) {
        fragSkb = skb_clone(immZc, GFP_ATOMIC);

        if (fragSkb == VOS_NULL_PTR) {
            IMM_ZcFreeAny(immZc);
            ADS_WARNING_LOG(ACPU_PID_ADS_UL, "pkt clone fail.");
            return VOS_ERROR;
        }

        skb_trim(fragSkb, IMM_MAX_ETH_FRAME_LEN);
        ADS_UL_SAVE_PC5_FRAG_INFO_TO_IMM(fragSkb, VOS_FALSE, fragSeq);

        if (ADS_UL_InsertPc5Queue(fragSkb) != VOS_OK) {
            IMM_ZcFreeAny(fragSkb);
            IMM_ZcFreeAny(immZc);
            return VOS_ERROR;
        }

        IMM_ZcPull(immZc, IMM_MAX_ETH_FRAME_LEN);
        fragSeq++;
    }

    ADS_UL_SAVE_PC5_FRAG_INFO_TO_IMM(immZc, VOS_TRUE, fragSeq);

    /* ��pstImmZc���뵽PC5��������� */
    if (ADS_UL_InsertPc5Queue(immZc) != VOS_OK) {
        IMM_ZcFreeAny(immZc);
        return VOS_ERROR;
    }

    return VOS_OK;
}

VOS_VOID ADS_UL_PC5_FillIpfCfgParam(ipf_config_ulparam_s *ulCfgParam, ADS_IPF_BdBuff *ulBdBuff, IMM_Zc *immZc)
{
    if (immZc == VOS_NULL_PTR) {
        return;
    }

    ulBdBuff->pkt = immZc;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
    /*lint -e64 */
    ulCfgParam->data = (modem_phy_addr)(VOS_UINT_PTR)virt_to_phys((VOS_VOID *)immZc->data);
    /*lint +e64 */
    ulCfgParam->mode    = IPF_MODE_FILTERANDTRANS;
    ulCfgParam->fc_head = IPF_LTEV_ULFC;
    ulCfgParam->len     = (VOS_UINT16)immZc->len;
#endif

    ADS_MNTN_REC_UL_PC5_INFO(immZc);

    /* ˢCAHCE */
    ADS_IPF_UL_MEM_MAP(immZc, immZc->len);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
    ulCfgParam->data    = (modem_phy_addr)(VOS_UINT_PTR)ADS_IPF_GetMemDma(immZc);
    ulCfgParam->mode    = IPF_MODE_FILTERANDTRANS;
    ulCfgParam->fc_head = IPF_LTEV_ULFC;
    ulCfgParam->len     = (VOS_UINT16)immZc->len;
#endif

    ulCfgParam->usr_field1 = (VOS_UINT16)ADS_GET_PC5_FRAG_INFO(immZc);

    ADS_DBG_UL_PC5_BDQ_CFG_BD_SUCC_NUM(1);
}

VOS_UINT32 ADS_UL_GetPrioQueue(VOS_VOID)
{
    VOS_UINT32 instanceIndex;
    VOS_UINT32 comQueTotalLen = 0;
    VOS_UINT32 pc5QueLen;

    pc5QueLen      = IMM_ZcQueueLen(ADS_UL_GET_PC5_QUEUE_HEAD());

    /* PC5����Ϊ�գ�ֱ�ӷ���LOW */
    if (pc5QueLen == 0) {
        g_pc5NodeCnt = 0;
        return LOW_PRIO_QUE;
    }

    /* PC5���в�Ϊ���Ҽ���С��10������HIGH */
    if (g_pc5NodeCnt < ADS_UL_PC5_PKT_SEND_WEIGHT) {
        g_pc5NodeCnt++;
        return HIGH_PRIO_QUE;
    }

    /* ������ͨ���еı��ĸ��� */
    for (instanceIndex = 0; instanceIndex < ADS_INSTANCE_MAX_NUM; instanceIndex++) {
        comQueTotalLen = comQueTotalLen + ADS_UL_GetInstanceAllQueueDataNum(instanceIndex);
    }

    /* ��ͨ����Ϊ�գ�ֱ�ӷ���HIGH */
    if (comQueTotalLen == 0) {
        g_pc5NodeCnt++;
        return HIGH_PRIO_QUE;
    }

    /* ��ͨ���в�Ϊ�գ�PC5���в�Ϊ������ȡ��10����������LOW */
    g_pc5NodeCnt = 0;
    return LOW_PRIO_QUE;
}
#endif

VOS_INT ADS_UL_SendPacket(IMM_Zc *immZc, VOS_UINT8 exRabId)
{
    VOS_UINT32 instance;
    VOS_UINT32 rabId;

    /* �ж��Ƿ�Ϊ�����ݰ� */
    if (immZc == VOS_NULL_PTR) {
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "ADS_UL_SendPacket: pstImmZc is null!");
        return VOS_ERR;
    }

    if (IMM_ZcGetUsedLen(immZc) == 0) {
        IMM_ZcFreeAny(immZc);
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "ADS_UL_SendPacket: len is 0!");
        return VOS_ERR;
    }

    /* �������н�������ͳ�Ƹ��� */
    ADS_DBG_UL_RMNET_RX_PKT_NUM(1);

    /* ͳ�������������յ��������ֽ���������������ѯ */
    ADS_RECV_UL_PERIOD_PKT_NUM(immZc->len);

    /* ���MODEMID */
    instance = ADS_GET_MODEMID_FROM_EXRABID(exRabId);
    if (instance >= ADS_INSTANCE_MAX_NUM) {
        IMM_ZcFreeAny(immZc); /* �ɲ���ϵͳ�ӹ� */
        ADS_DBG_UL_RMNET_MODEMID_ERR_NUM(1);
        return VOS_ERR;
    }

    /* ���RABID */
    rabId = ADS_GET_RABID_FROM_EXRABID(exRabId);
    if (!ADS_IS_RABID_VALID(rabId)) {
        IMM_ZcFreeAny(immZc);
        ADS_DBG_UL_RMNET_RABID_NUM(1);
        return VOS_ERR;
    }

    /* ��pstData���뵽ucRabId��Ӧ�Ļ�������� */
    if (ADS_UL_InsertQueue(instance, immZc, rabId) != VOS_OK) {
        IMM_ZcFreeAny(immZc);
        ADS_DBG_UL_RMNET_ENQUE_FAIL_NUM(1);
        return VOS_ERR;
    }

    ADS_DBG_UL_RMNET_ENQUE_SUCC_NUM(1);
    return VOS_OK;
}

VOS_INT ADS_UL_SendPacketEx(IMM_Zc *immZc, ADS_PktTypeUint8 ipType, VOS_UINT8 exRabId)
{
    ADS_DL_RabInfo *dlRabInfo = VOS_NULL_PTR;
    VOS_UINT32      instance;
    VOS_UINT32      rabId;

    /* �ж��Ƿ�Ϊ�����ݰ� */
    if (immZc == VOS_NULL_PTR) {
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "ADS_UL_SendPacketEx: pstImmZc is null!");
        return VOS_ERR;
    }

    if (IMM_ZcGetUsedLen(immZc) == 0) {
        IMM_ZcFreeAny(immZc); /* �ɲ���ϵͳ�ӹ� */
        ADS_WARNING_LOG(ACPU_PID_ADS_UL, "ADS_UL_SendPacketEx: len is 0!");
        return VOS_ERR;
    }

    /* �������н�������ͳ�Ƹ��� */
    ADS_DBG_UL_RMNET_RX_PKT_NUM(1);

    /* ͳ�������������յ��������ֽ���������������ѯ */
    ADS_RECV_UL_PERIOD_PKT_NUM(immZc->len);

    /* ���MODEMID */
    instance = ADS_GET_MODEMID_FROM_EXRABID(exRabId);
    if (instance >= ADS_INSTANCE_MAX_NUM) {
        IMM_ZcFreeAny(immZc);
        ADS_DBG_UL_RMNET_MODEMID_ERR_NUM(1);
        return VOS_ERR;
    }

    /* ���RABID */
    rabId = ADS_GET_RABID_FROM_EXRABID(exRabId);
    if (!ADS_IS_RABID_VALID(rabId)) {
        IMM_ZcFreeAny(immZc);
        ADS_DBG_UL_RMNET_RABID_NUM(1);
        return VOS_ERR;
    }

    /* �ж��Ƿ��Ѿ�ע������й��˻ص���������ע���������Ҫ���й�����Ϣ����ȡ������ֱ�ӷ��ͱ��� */
    dlRabInfo = ADS_DL_GET_RAB_INFO_PTR(instance, rabId);
    if (dlRabInfo->rcvDlFilterDataFunc != VOS_NULL_PTR) {
        /* ���ù����������ݰ������� */
        ADS_FilterProcUlPacket(immZc, ipType);
    }

    /* ��pstData���뵽ucRabId��Ӧ�Ļ�������� */
    if (ADS_UL_InsertQueue(instance, immZc, rabId) != VOS_OK) {
        IMM_ZcFreeAny(immZc);
        ADS_DBG_UL_RMNET_ENQUE_FAIL_NUM(1);
        return VOS_ERR;
    }

    ADS_DBG_UL_RMNET_ENQUE_SUCC_NUM(1);
    return VOS_OK;
}

IMM_Zc* ADS_UL_GetInstanceNextQueueNode(VOS_UINT32 instanceIndex, VOS_UINT32 *rabId, VOS_UINT8 *puc1XorHrpdUlIpfFlag)
{
    VOS_UINT32  i;
    VOS_UINT32 *curIndex;
    ADS_UL_Ctx *adsUlCtx;
    IMM_Zc     *node;

    adsUlCtx = ADS_GetUlCtx(instanceIndex);

    curIndex = &adsUlCtx->adsUlCurIndex;

    node = VOS_NULL_PTR;

    /* ���ȼ���ʽ */
    for (i = 0; i < ADS_RAB_NUM_MAX; i++) {
        /*
         * ��Ϊ�����Ѿ����򣬵�ǰ������Ч�����������ж��ж���Ч
         * ����������������Ч���У�������ͷ����
         */
        if (ADS_UL_GET_QUEUE_LINK_INFO(instanceIndex, *curIndex) == VOS_NULL_PTR) {
            i += ADS_RAB_NUM_MAX - (*curIndex + 1U);

            *curIndex = 0;

            continue;
        }

        /* ����Ϊ��Ч���е�������ʱ������������ */
        if (ADS_UL_GET_QUEUE_LINK_INFO(instanceIndex, *curIndex)->qlen == 0) {
            /* ������һ�����е�����ʱ���������м�¼����� */
            ADS_UL_CLR_RECORD_NUM_IN_WEIGHTED(instanceIndex, *curIndex);

            *curIndex = (*curIndex + 1) % ADS_RAB_NUM_MAX;

            continue;
        }

        /* �������ȼ��ȼ���Ӧ�ļ�Ȩ���������� */
        /* ���ȼ��ߵ��ȷ��� */
        if (ADS_UL_GET_RECORD_NUM_IN_WEIGHTED(instanceIndex, *curIndex) <
            ADS_UL_GET_QUEUE_PRI_WEIGHTED_NUM(instanceIndex, *curIndex)) {
            /* ��ȡ����ͷ��� */
            node = IMM_ZcDequeueHead(ADS_UL_GET_QUEUE_LINK_INFO(instanceIndex, *curIndex));

            /* ��ȡ�ý���RabId */
            *rabId = ADS_UL_GET_PRIO_QUEUE_INDEX(instanceIndex, *curIndex);

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
            /* ��ȡ�ýڵ��Ƿ���1X����HRPD�İ� */
            *puc1XorHrpdUlIpfFlag = ADS_UL_GET_1X_OR_HRPD_UL_IPF_FLAG(instanceIndex, *curIndex);
#else
            *puc1XorHrpdUlIpfFlag = VOS_FALSE;
#endif

            /* �����м�¼������ 1 */
            ADS_UL_SET_RECORD_NUM_IN_WEIGHTED(instanceIndex, *curIndex, 1);

            /* ����Ѿ������걾���е����ȼ���Ȩ�����������ݣ��������¸����з������� */
            if (ADS_UL_GET_RECORD_NUM_IN_WEIGHTED(instanceIndex, *curIndex) ==
                ADS_UL_GET_QUEUE_PRI_WEIGHTED_NUM(instanceIndex, *curIndex)) {
                /* ������һ�����е�����ʱ���������м�¼����� */
                ADS_UL_CLR_RECORD_NUM_IN_WEIGHTED(instanceIndex, *curIndex);

                *curIndex = (*curIndex + 1) % ADS_RAB_NUM_MAX;
            }

            break;
        }
    }

    return node;
}

IMM_Zc* ADS_UL_GetNextQueueNode(VOS_UINT32 *rabId, VOS_UINT32 *instanceIndex, VOS_UINT8 *puc1XorHrpdUlIpfFlag)
{
    ADS_Ctx   *adsCtx = VOS_NULL_PTR;
    IMM_Zc    *node   = VOS_NULL_PTR;
    VOS_UINT32 i;
    VOS_UINT32 curInstanceIndex;

    adsCtx = ADS_GetAllCtx();

    curInstanceIndex = adsCtx->adsCurInstanceIndex;

    for (i = 0; i < ADS_INSTANCE_MAX_NUM; i++) {
        curInstanceIndex = curInstanceIndex % ADS_INSTANCE_MAX_NUM;

        node = ADS_UL_GetInstanceNextQueueNode(curInstanceIndex, rabId, puc1XorHrpdUlIpfFlag);

        if (node != VOS_NULL_PTR) {
            break;
        }

        curInstanceIndex++;
    }

    /* ����ʵ������������BDʱ��modem id */
    *instanceIndex = curInstanceIndex;

    /* ��¼�´δ��ĸ�ʵ����ȥ���� */
    adsCtx->adsCurInstanceIndex = (curInstanceIndex + 1) % ADS_INSTANCE_MAX_NUM;

    return node;
}

VOS_VOID ADS_UL_SaveIpfSrcMem(const ADS_IPF_BdBuff *ipfUlBdBuff, VOS_UINT32 saveNum)
{
    VOS_UINT32 cnt;

    if (saveNum > IPF_ULBD_DESC_SIZE) {
        return;
    }

    for (cnt = 0; cnt < saveNum; cnt++) {
        IMM_ZcQueueTail(ADS_UL_IPF_SRCMEM_FREE_QUE(), ipfUlBdBuff[cnt].pkt);
        ADS_DBG_UL_BDQ_SAVE_SRC_MEM_NUM(1);
    }
}

VOS_VOID ADS_UL_FreeIpfSrcMem(VOS_UINT32 *allMemFree)
{
    IMM_Zc    *immZc = VOS_NULL_PTR;
    VOS_UINT32 idleBD;
    VOS_UINT32 busyBD;
    VOS_UINT32 canFree;
    VOS_UINT32 queCnt;
    VOS_UINT32 cnt;

    /* que is empty */
    queCnt = IMM_ZcQueueLen(ADS_UL_IPF_SRCMEM_FREE_QUE());
    if (queCnt == 0) {
        *allMemFree = VOS_TRUE;
        return;
    }

    /* get busy bd num */
    idleBD = mdrv_ipf_get_uldesc_num();
    busyBD = IPF_ULBD_DESC_SIZE - idleBD;

    if (idleBD == IPF_ULBD_DESC_SIZE) {
        *allMemFree = VOS_TRUE;
    }

    if (queCnt >= busyBD) {
        canFree = queCnt - busyBD;
    } else {
        ADS_ERROR_LOG3(ACPU_PID_ADS_UL,
                       "ADS_UL_FreeIpfUlSrcMem: Buff Num Less IPF Busy BD Num. ulQueCnt, ulBusyBD, ulIdleBD", queCnt,
                       busyBD, idleBD);
        ADS_DBG_UL_BDQ_FREE_SRC_MEM_ERR(1);
        return;
    }

    /* free src mem */
    for (cnt = 0; cnt < canFree; cnt++) {
        immZc = IMM_ZcDequeueHead(ADS_UL_IPF_SRCMEM_FREE_QUE());
        if (immZc == VOS_NULL_PTR) {
            break;
        }

        /* ��ˢCACHE, ���ͷŻ��� */
        ADS_IPF_UL_MEM_UNMAP(immZc, IMM_ZcGetUsedLen(immZc));
        IMM_ZcFreeAny(immZc); /* �ɲ���ϵͳ�ӹ� */
        ADS_DBG_UL_BDQ_FREE_SRC_MEM_NUM(1);
    }
}

VOS_VOID ADS_UL_ClearIpfSrcMem(VOS_VOID)
{
    IMM_Zc    *immZc = VOS_NULL_PTR;
    VOS_UINT32 queCnt;
    VOS_UINT32 cnt;
    VOS_UINT32 ipfUlBdNum;
    VOS_UINT32 i;

    queCnt = IMM_ZcQueueLen(ADS_UL_IPF_SRCMEM_FREE_QUE());
    if (queCnt == 0) {
        return;
    }

    /* ���е�PDP��ȥ����󣬲���BD�Ѿ�ȫ�����У�����������ȫ�����꣬���������Դ�ڴ���� */
    for (i = 0; i < ADS_INSTANCE_MAX_NUM; i++) {
        if (ADS_UL_CheckAllQueueEmpty(i) == VOS_FALSE) {
            return;
        }
    }

    ipfUlBdNum = mdrv_ipf_get_uldesc_num();
    /* ����BD���63�� */
    if (ipfUlBdNum != IPF_ULBD_DESC_SIZE) {
        return;
    }

    /* free src mem */
    for (cnt = 0; cnt < queCnt; cnt++) {
        immZc = IMM_ZcDequeueHead(ADS_UL_IPF_SRCMEM_FREE_QUE());
        if (immZc == VOS_NULL_PTR) {
            break;
        }

        /* ��ˢCACHE, ���ͷŻ��� */
        ADS_IPF_UL_MEM_UNMAP(immZc, immZc->len);
        IMM_ZcFreeAny(immZc); /* �ɲ���ϵͳ�ӹ� */
        ADS_DBG_UL_BDQ_FREE_SRC_MEM_NUM(1);
    }
}

VOS_UINT8 ADS_UL_GetBdFcHead(VOS_UINT32 instance, VOS_UINT8 uc1XorHrpdUlIpfFlag)
{
    VOS_UINT8 tempBdFcHead;

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
    tempBdFcHead = ((uc1XorHrpdUlIpfFlag == VOS_TRUE) ? (VOS_UINT8)ADS_UL_IPF_1XHRPD : (VOS_UINT8)instance);
#else
    tempBdFcHead = (VOS_UINT8)instance;
#endif

    return tempBdFcHead;
}

VOS_UINT32 ADS_UL_CalcBuffTime(VOS_UINT32 beginSlice, VOS_UINT32 endSlice)
{
    if (endSlice > beginSlice) {
        return (endSlice - beginSlice);
    } else {
        return (VOS_NULL_DWORD - beginSlice + endSlice + 1);
    }
}

VOS_UINT32 ADS_UL_BuildBdUserField2(IMM_Zc *immZc)
{
#if (defined(CONFIG_HUAWEI_BASTET) || defined(CONFIG_HW_DPIMARK_MODULE))
    struct sock *sk = VOS_NULL_PTR;
#ifdef CONFIG_HW_DPIMARK_MODULE
    VOS_UINT8 *tmp = VOS_NULL_PTR;
#endif
    VOS_UINT32 userField2;

    if (immZc == VOS_NULL_PTR) {
        return 0;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 23))
    sk = skb_to_full_sk(immZc);
#else
    sk = immZc->sk;
#endif
    if (sk != VOS_NULL_PTR) {
        userField2 = 0;
#ifdef CONFIG_HUAWEI_BASTET
        /* ��һ���ֽ�bit0Ϊ�����ȼ�״̬ �ڶ����ֽ�Ϊ��ʱ������������ */
        userField2 |= (sk->acc_state & 0x01);
        userField2 |= (((VOS_UINT32)sk->discard_duration << 8) & 0xFF00);
#endif
#ifdef CONFIG_HW_DPIMARK_MODULE
        tmp = (VOS_UINT8 *)&(sk->sk_hwdpi_mark);
        /* sk_hwdpi_mark��1,2�ֽھ����õ�userfield2�ĵ�һ�ֽ� */
        userField2 |= tmp[0];
        userField2 |= tmp[1];
#endif
        return userField2;
    }
#endif

    return 0;
}

dma_addr_t ADS_UL_GetMemDma(IMM_Zc *immZc)
{
    return ADS_IPF_GetMemDma(immZc);
}

VOS_VOID ADS_UL_ConfigBD(VOS_UINT32 bdNum)
{
    ipf_config_ulparam_s *ulCfgParam = VOS_NULL_PTR;
    ADS_IPF_BdBuff       *ulBdBuff   = VOS_NULL_PTR;
    IMM_Zc               *immZc      = VOS_NULL_PTR;
    VOS_UINT32            beginSlice;
    VOS_UINT32            endSlice;
    VOS_UINT32            tmp;
    VOS_UINT32            cnt;
    VOS_INT32             rslt;
    VOS_UINT32            instance;
    VOS_UINT32            rabId;
    VOS_UINT8             uc1XorHrpdUlIpfFlag;
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    VOS_UINT32 quePrio;
#endif

    endSlice = VOS_GetSlice();

    for (cnt = 0; cnt < bdNum; cnt++) {
        ulBdBuff   = ADS_UL_GET_BD_BUFF_PTR(cnt);
        ulCfgParam = ADS_UL_GET_BD_CFG_PARA_PTR(cnt);

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
        quePrio = ADS_UL_GetPrioQueue();

        if (quePrio == HIGH_PRIO_QUE) {
            immZc = ADS_UL_GetPc5QueueNode();
            ADS_UL_PC5_FillIpfCfgParam(ulCfgParam, ulBdBuff, immZc);
            continue;
        } else {
            immZc = ADS_UL_GetNextQueueNode(&rabId, &instance, &uc1XorHrpdUlIpfFlag);
        }
#else
        immZc = ADS_UL_GetNextQueueNode(&rabId, &instance, &uc1XorHrpdUlIpfFlag);
#endif

        if (immZc == VOS_NULL_PTR) {
            break;
        }

        ulBdBuff->pkt = immZc;

        beginSlice = ADS_UL_GET_SLICE_FROM_IMM(immZc);
        /* Attribute: �ж�ʹ�ܣ����˼Ӱ��ƣ����������modem0��0��modem1��1 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#ifndef CONFIG_NEW_PLATFORM
        ulCfgParam->data      = (VOS_UINT32)virt_to_phys((VOS_VOID *)immZc->data);
        ulCfgParam->attribute = ADS_UL_BUILD_BD_ATTRIBUTE(VOS_FALSE, IPF_MODE_FILTERANDTRANS,
                                                          ADS_UL_GetBdFcHead(instance, uc1XorHrpdUlIpfFlag));
#else
        ulCfgParam->data    = (modem_phy_addr)(VOS_UINT_PTR)virt_to_phys((VOS_VOID *)immZc->data);
        ulCfgParam->mode    = IPF_MODE_FILTERANDTRANS;
        ulCfgParam->fc_head = ADS_UL_GetBdFcHead(instance, uc1XorHrpdUlIpfFlag);
#endif /* CONFIG_NEW_PLATFORM */
        ulCfgParam->len = (VOS_UINT16)immZc->len;
#endif
        ulCfgParam->usr_field1 = (VOS_UINT16)ADS_UL_BUILD_BD_USER_FIELD_1(instance, rabId);
        ulCfgParam->usr_field3 = (VOS_UINT32)ADS_UL_CalcBuffTime(beginSlice, endSlice);
        ulCfgParam->usr_field2 = ADS_UL_BuildBdUserField2(immZc);

        ADS_MNTN_REC_UL_IP_INFO(immZc, ulCfgParam->usr_field1, ulCfgParam->usr_field2, ulCfgParam->usr_field3);

        /* ˢCAHCE */
        ADS_IPF_UL_MEM_MAP(immZc, immZc->len);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
#ifndef CONFIG_NEW_PLATFORM
        ulCfgParam->data      = (VOS_UINT32)ADS_UL_GetMemDma(immZc);
        ulCfgParam->attribute = ADS_UL_BUILD_BD_ATTRIBUTE(VOS_FALSE, IPF_MODE_FILTERANDTRANS,
                                                          ADS_UL_GetBdFcHead(instance, uc1XorHrpdUlIpfFlag));
#else
        ulCfgParam->data    = (modem_phy_addr)ADS_UL_GetMemDma(immZc);
        ulCfgParam->mode    = IPF_MODE_FILTERANDTRANS;
        ulCfgParam->fc_head = ADS_UL_GetBdFcHead(instance, uc1XorHrpdUlIpfFlag);
#endif
        ulCfgParam->len = (VOS_UINT16)immZc->len;
#endif
    }

    /* �����������ݸ���Ϊ0 */
    if (cnt == 0) {
        return;
    }

    /* ���һ��BD�����ж�ʹ�� */
    ulCfgParam = ADS_UL_GET_BD_CFG_PARA_PTR(cnt - 1);
#ifndef CONFIG_NEW_PLATFORM
    ADS_UL_SET_BD_ATTR_INT_FLAG(ulCfgParam->u16Attribute);
#else
    ulCfgParam->int_en = 1;
#endif
    /* ����IPF����BD */
    rslt = mdrv_ipf_config_ulbd(cnt, ADS_UL_GET_BD_CFG_PARA_PTR(0));
    if (rslt != IPF_SUCCESS) {
        /* IPF����ʧ��, ��Ҫ�ͷ�Դ�ڴ� */
        tmp = cnt;
        for (cnt = 0; cnt < tmp; cnt++) {
            ulBdBuff = ADS_UL_GET_BD_BUFF_PTR(cnt);
            IMM_ZcFreeAny(ulBdBuff->pkt); /* �ɲ���ϵͳ�ӹ� */
            ADS_DBG_UL_BDQ_CFG_BD_FAIL_NUM(1);
        }

        ADS_DBG_UL_BDQ_CFG_IPF_FAIL_NUM(1);
        return;
    }

    /* �������õ�BDԴ�ڴ汣�浽Դ�ڴ���� */
    ADS_UL_SaveIpfSrcMem(ADS_UL_GET_BD_BUFF_PTR(0), cnt);
    ADS_DBG_UL_BDQ_CFG_BD_SUCC_NUM(cnt);
    ADS_DBG_UL_BDQ_CFG_IPF_SUCC_NUM(1);

    ADS_MNTN_RPT_ALL_UL_PKT_INFO();

    ADS_UL_EnableRxWakeLockTimeout(ADS_UL_RX_WAKE_LOCK_TMR_LEN);
}

VOS_VOID ADS_UL_ProcLinkData(VOS_VOID)
{
    VOS_UINT32 allUlQueueDataNum;
    VOS_UINT32 ipfUlBdNum;
    VOS_UINT32 sndBdNum;
    VOS_UINT32 allMemFree;

    /* �������ʱ�е����� */
    for (;;) {
        /* ���ڸ�λ������ֱ���˳� */
        if (ADS_GetUlResetFlag() == VOS_TRUE) {
            ADS_PR_LOGI("in ccore reset");

            return;
        }

        /* ��ȡ���п��Է��͵�BD���� */
        ipfUlBdNum = mdrv_ipf_get_uldesc_num();
        if (ipfUlBdNum == 0) {
            ADS_DBG_UL_BDQ_CFG_IPF_HAVE_NO_BD_NUM(1);

            /* ���÷��ͽ�����־ */
            ADS_UL_SET_SENDING_FLAG(VOS_FALSE);

            /* ������ʱ���˳� */
            ADS_StartUlSendProtectTimer();
            break;
        }

        /* �������ڷ��ͱ�־ */
        ADS_UL_SET_SENDING_FLAG(VOS_TRUE);

        /* ��ȡ��ǰ���ж����е����ݰ����� */
        allUlQueueDataNum = ADS_UL_GetAllQueueDataNum();

        /* ���㵱ǰ�ɷ��͵�BD��Ŀ */
        sndBdNum = PS_MIN(ipfUlBdNum, allUlQueueDataNum);

        /* �ͷű����Դ�ڴ� */
        allMemFree = VOS_FALSE;
        ADS_UL_FreeIpfSrcMem(&allMemFree);

        /* ����BD��д��IPF */
        ADS_UL_ConfigBD(sndBdNum);

        /* ��ȡ��ǰ���ж����е����ݰ����� */
        allUlQueueDataNum = ADS_UL_GetAllQueueDataNum();

        /* ��ǰ������û�����ݣ��˳����ȴ��´ζ����ɿձ�Ϊ�ǿմ��� */
        if (allUlQueueDataNum == 0) {
            /* ulSndBdNumΪ0����������bd��Ϊ0�������д��ͷ��ڴ��ͷ��꣬�����ݷ�����ɣ�����������ʱ�� */
            if (sndBdNum == 0 && allMemFree == VOS_TRUE) {
                /* ���÷��ͽ�����־ */
                ADS_UL_SET_SENDING_FLAG(VOS_FALSE);
                break;
            } else {
                ADS_StartUlSendProtectTimer();

                /* ���÷��ͽ�����־ */
                ADS_UL_SET_SENDING_FLAG(VOS_FALSE);
                break;
            }
        }
        /* ��ǰ�����������ݣ�������Ҫ�����ܰ� */
        else if (allUlQueueDataNum <= ADS_UL_SEND_DATA_NUM_THREDHOLD) {
            ADS_StartUlSendProtectTimer();

            /* ���÷��ͽ�����־ */
            ADS_UL_SET_SENDING_FLAG(VOS_FALSE);
            break;
        } else {
            continue;
        }
    }
}

VOS_UINT32 ADS_UL_ProcPdpStatusInd(ADS_PdpStatusInd *statusInd)
{
    VOS_UINT32              instanceIndex;
    ADS_CDS_IpfPktTypeUint8 pktType = ADS_CDS_IPF_PKT_TYPE_IP;
    ADS_QciTypeUint8        prio;

    instanceIndex = statusInd->modemId;

    prio = statusInd->qciType;

    /* RabId�Ϸ��Լ�� */
    if (!ADS_IS_RABID_VALID(statusInd->rabId)) {
        ADS_WARNING_LOG1(ACPU_PID_ADS_UL, "ADS_UL_ProcPdpStatusInd: ulRabId is ", statusInd->rabId);
        return VOS_ERR;
    }

    /* ������������ȼ������޸�����PDP��QCIΪ��ͬ���ȼ������������㷨��������ʹ�ȼ����PDP���ȴ��� */
    if (g_adsCtx.adsSpecCtx[instanceIndex].adsUlCtx.queuePriNv.status == VOS_FALSE) {
        prio = ADS_QCI_TYPE_QCI9_NONGBR;
    }

    if (statusInd->pdpType == ADS_PDP_PPP) {
        pktType = ADS_CDS_IPF_PKT_TYPE_PPP;
    }

    /* ����PDP״̬�ֱ���д��� */
    switch (statusInd->pdpStatus) {
        /* PDP���� */
        case ADS_PDP_STATUS_ACT:

            /* ����������� */
            ADS_UL_CreateQueue(instanceIndex, statusInd->rabId, prio, pktType, statusInd->xorHrpdUlIpfFlag);

            /* ��������ͳ�� */
            ADS_UL_StartDsFlowStats(instanceIndex, statusInd->rabId);

            break;

        /* PDP�޸� */
        case ADS_PDP_STATUS_MODIFY:

            /* ���޸ĵĶ�����Ϣ���µ����ж��й����� */
            ADS_UL_UpdateQueueInPdpModified(instanceIndex, prio, statusInd->rabId);

            break;

        /* PDPȥ���� */
        case ADS_PDP_STATUS_DEACT:

            /* ���ٻ������ */
            ADS_UL_DestroyQueue(instanceIndex, statusInd->rabId);

            /* �����е�PDP��ȥ��������Դ�ڴ���� */
            ADS_UL_ClearIpfSrcMem();

            /* ֹͣ����ͳ�� */
            ADS_UL_StopDsFlowStats(instanceIndex, statusInd->rabId);

            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvTafPdpStatusInd(MsgBlock *msg)
{
    ADS_PdpStatusInd *pdpStatusInd;
    VOS_UINT32        rslt;

    pdpStatusInd = (ADS_PdpStatusInd *)msg;

    rslt = ADS_UL_ProcPdpStatusInd(pdpStatusInd);

    return rslt;
}

VOS_UINT32 ADS_UL_RcvCdsIpPacketMsg(MsgBlock *msg)
{
    VOS_UINT32        result;
    ADS_NDIS_DataInd *adsNdisDataInd = VOS_NULL_PTR;
    IMM_Zc           *zcData         = VOS_NULL_PTR;
    CDS_ADS_DataInd  *dataInd        = VOS_NULL_PTR;
    VOS_CHAR         *zcPutData      = VOS_NULL_PTR;

    dataInd = (CDS_ADS_DataInd *)msg;

    /* ������Ϣ  */
    adsNdisDataInd = (ADS_NDIS_DataInd *)TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_UL, sizeof(ADS_NDIS_DataInd));

    if (adsNdisDataInd == VOS_NULL_PTR) {
        return VOS_ERR;
    }

    /* ��д��Ϣ���� */
    TAF_CfgMsgHdr((MsgBlock *)adsNdisDataInd, ACPU_PID_ADS_UL, PS_PID_APP_NDIS,
                  sizeof(ADS_NDIS_DataInd) - VOS_MSG_HEAD_LENGTH);
    adsNdisDataInd->msgId        = ID_ADS_NDIS_DATA_IND;
    adsNdisDataInd->modemId      = dataInd->modemId;
    adsNdisDataInd->rabId        = dataInd->rabId;
    adsNdisDataInd->ipPacketType = dataInd->ipPacketType;

    zcData = (IMM_Zc *)IMM_ZcStaticAlloc((VOS_UINT32)dataInd->len);

    if (zcData == VOS_NULL_PTR) {
        (VOS_VOID)VOS_FreeMsg(ACPU_PID_ADS_UL, adsNdisDataInd);

        return VOS_ERR;
    }

    /* �˲��費���٣�����ƫ������βָ�� */
    zcPutData = (VOS_CHAR *)IMM_ZcPut(zcData, dataInd->len);

    if (memcpy_s(zcPutData, dataInd->len, dataInd->data, dataInd->len) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_RcvCdsIpPacketMsg: Memcpy Fail!");
    }

    adsNdisDataInd->data = zcData;

    /* ����VOS����ԭ�� */
    result = TAF_TraceAndSendMsg(ACPU_PID_ADS_UL, adsNdisDataInd);

    if (result != VOS_OK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_RcvCdsIpPacketMsg: Send Msg Fail!");

        IMM_ZcFreeAny(zcData); /* �ɲ���ϵͳ�ӹ� */

        return VOS_ERR;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvCcpuResetStartInd(MsgBlock *msg)
{
    VOS_UINT32 insIndex;
    VOS_UINT32 rabIndex;
    VOS_UINT32 tiIndex;
    ADS_Ctx   *adsCtx = VOS_NULL_PTR;

    ADS_TRACE_HIGH("enter.\n");

    adsCtx = ADS_GetAllCtx();

    /* ����������л������ */
    for (insIndex = 0; insIndex < ADS_INSTANCE_MAX_NUM; insIndex++) {
        for (rabIndex = 0; rabIndex < ADS_RAB_ID_MAX + 1; rabIndex++) {
            ADS_UL_DestroyQueue(insIndex, rabIndex);
        }
    }

    /* ���Դ�ڴ���� */
    ADS_UL_ClearIpfSrcMem();

    /* ֹͣ���������Ķ�ʱ�� */
    for (tiIndex = 0; tiIndex < ADS_MAX_TIMER_NUM; tiIndex++) {
        ADS_StopTimer(ACPU_PID_ADS_UL, tiIndex, ADS_TIMER_STOP_CAUSE_USER);
    }

    /* �������������� */
    ADS_ResetUlCtx();

    /* ����IPF��ص������� */
    ADS_ResetIpfCtx();

    /* ���õ�ǰʵ������ֵ */
    adsCtx->adsCurInstanceIndex = ADS_INSTANCE_INDEX_0;

    /* ����ADS Filter���������� */
    ADS_FilterReset();

    /* �ͷ��ź�����ʹ�õ���API����������� */
    VOS_SmV(ADS_GetULResetSem());

    ADS_TRACE_HIGH("leave.\n");
    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvDplConfigInd(MsgBlock *msg)
{
    ADS_DplConfigInd *dplCfgInd = VOS_NULL_PTR;

    dplCfgInd = (ADS_DplConfigInd *)msg;

    ADS_MntnConfigDpl(dplCfgInd->dplNeededFlg);

    return VOS_OK;
}

VOS_VOID ADS_UL_RcvTiDsFlowStatsExpired(VOS_UINT32 timerName, VOS_UINT32 param)
{
    VOS_UINT32 taBytes;
    VOS_UINT32 rate;

    /* ���û�����ж��д���, ����ͳ������ */
    if (ADS_UL_IsAnyQueueExist() == VOS_FALSE) {
        ADS_NORMAL_LOG(ACPU_PID_ADS_DL, "ADS_UL_RcvTiDsFlowStatsExpired: no queue is exist!");
        return;
    }

    /* ��ȡ1����������ݸ��� */
    taBytes = ADS_GET_DL_PERIOD_PKT_NUM();

    /* ÿ1���Ӽ���һ��,��λΪ:byte/s */
    rate = taBytes;
    ADS_SET_CURRENT_DL_RATE(rate);

    /* ��ȡ1����������� */
    taBytes = ADS_GET_UL_PERIOD_PKT_NUM();

    /* ÿ1���Ӽ���һ��,��λΪ:byte/s */
    rate = taBytes;
    ADS_SET_CURRENT_UL_RATE(rate);

    /* ÿ������ͳ�����ڽ�������Ҫ������ͳ��Byte����� */
    ADS_CLEAR_UL_PERIOD_PKT_NUM();
    ADS_CLEAR_DL_PERIOD_PKT_NUM();

    /* ��������ͳ�ƶ�ʱ�� */
    ADS_StartTimer(TI_ADS_DSFLOW_STATS, TI_ADS_DSFLOW_STATS_LEN);
}

VOS_VOID ADS_UL_RcvTiDataStatExpired(VOS_UINT32 timerName, VOS_UINT32 param)
{
    VOS_UINT32 statPktNum;

    statPktNum = ADS_UL_GET_STAT_PKT_NUM();

    /* �������ݰ����������ް����� */
    if (statPktNum < ADS_UL_GET_WATER_LEVEL_ONE()) {
        ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(ADS_UL_DATA_THRESHOLD_ONE);
        ADS_DBG_UL_WM_LEVEL_1_HIT_NUM(1);
    } else if (statPktNum < ADS_UL_GET_WATER_LEVEL_TWO()) {
        ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(ADS_UL_DATA_THRESHOLD_TWO);
        ADS_DBG_UL_WM_LEVEL_2_HIT_NUM(1);
    } else if (statPktNum < ADS_UL_GET_WATER_LEVEL_THREE()) {
        ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(ADS_UL_DATA_THRESHOLD_THREE);
        ADS_DBG_UL_WM_LEVEL_3_HIT_NUM(1);
    } else {
        ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(ADS_UL_DATA_THRESHOLD_FOUR);
        ADS_DBG_UL_WM_LEVEL_4_HIT_NUM(1);
    }

    /* 100ms��û�����ݰ���ö�ʱ���������� */
    if (statPktNum != 0) {
        /* ������������ͳ�ƶ�ʱ�� */
        ADS_StartTimer(TI_ADS_UL_DATA_STAT, ADS_UL_GET_STAT_TIMER_LEN());
    }

    /* ���ͳ�ư��ĸ��� */
    ADS_UL_CLR_STAT_PKT_NUM();
}

VOS_UINT32 ADS_UL_RcvTafMsg(MsgBlock *msg)
{
    MSG_Header *msgTemp;

    msgTemp = (MSG_Header *)msg;

    switch (msgTemp->msgName) {
        case ID_APS_ADS_PDP_STATUS_IND:
            ADS_UL_RcvTafPdpStatusInd(msg);
            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvCdsMsg(MsgBlock *msg)
{
    MSG_Header *msgTemp;

    msgTemp = (MSG_Header *)msg;

    switch (msgTemp->msgName) {
        case ID_CDS_ADS_DATA_IND:
            ADS_UL_RcvCdsIpPacketMsg(msg);
            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvTimerMsg(MsgBlock *msg)
{
    REL_TimerMsgBlock *timerMsg;

    timerMsg = (REL_TimerMsgBlock *)msg;

    /* ֹͣ�ö�ʱ�� */
    ADS_StopTimer(ACPU_PID_ADS_UL, timerMsg->name, ADS_TIMER_STOP_CAUSE_TIMEOUT);

    switch (timerMsg->name) {
        case TI_ADS_DSFLOW_STATS:
            ADS_UL_RcvTiDsFlowStatsExpired(timerMsg->name, timerMsg->para);
            ADS_MntnReportAllStatsInfo();
            RNIC_MntnReportAllStatsInfo();
            break;

        case TI_ADS_UL_DATA_STAT:
            ADS_UL_RcvTiDataStatExpired(timerMsg->name, timerMsg->para);
            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_UL_RcvAdsUlMsg(MsgBlock *msg)
{
    MSG_Header *msgTemp;

    msgTemp = (MSG_Header *)msg;

    switch (msgTemp->msgName) {
        case ID_ADS_CCPU_RESET_START_IND:
            ADS_UL_RcvCcpuResetStartInd(msg);
            break;

        case ID_ADS_CCPU_RESET_END_IND:
            /* do nothing */
            ADS_NORMAL_LOG(ACPU_PID_ADS_UL, "ADS_DL_RcvAdsDlMsg: rcv ID_CCPU_ADS_UL_RESET_END_IND");
            break;

        case ID_ADS_DPL_CONFIG_IND:
            ADS_UL_RcvDplConfigInd(msg);
            break;

        default:
            ADS_NORMAL_LOG1(ACPU_PID_ADS_UL, "ADS_UL_RcvAdsUlMsg: rcv error msg id %d\r\n", msgTemp->msgName);
            break;
    }

    return VOS_OK;
}

VOS_VOID ADS_UL_ProcMsg(MsgBlock *msg)
{
    if (msg == VOS_NULL_PTR) {
        return;
    }

    if (TAF_ChkAdsUlFidRcvMsgLen(msg) != VOS_TRUE) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_ProcMsg: message length is invalid!");
        return;
    }

    /* ��Ϣ�ķַ����� */
    switch (VOS_GET_SENDER_ID(msg)) {
        /* ����Timer����Ϣ */
        case VOS_PID_TIMER:
            ADS_UL_RcvTimerMsg(msg);
            return;

        /* ����TAF����Ϣ */
        case I0_WUEPS_PID_TAF:
        case I1_WUEPS_PID_TAF:
        case I2_WUEPS_PID_TAF:
            ADS_UL_RcvTafMsg(msg);
            return;

        /* ����CDS����Ϣ */
        case UEPS_PID_CDS:
            ADS_UL_RcvCdsMsg(msg);
            return;

        /* ����ADS UL����Ϣ */
        case ACPU_PID_ADS_UL:
            ADS_UL_RcvAdsUlMsg(msg);
            return;

        default:
            return;
    }
}

