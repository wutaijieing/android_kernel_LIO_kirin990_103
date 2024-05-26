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

#ifndef __ADSUPLINK_H__
#define __ADSUPLINK_H__

#include "vos.h"
#include "ads_intra_msg.h"
#include "ads_ctx.h"
#include "product_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)

#define ADS_UL_ALLOC_MSG_WITH_HDR(ulMsgLen) TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_UL, (ulMsgLen))

/* 封装OSA消息头 */
#if (VOS_OS_VER == VOS_WIN32)
#define ADS_UL_CFG_MSG_HDR(pstMsg, ulRecvPid, ulMsgId)          \
    ((MSG_Header *)(pstMsg))->ulSenderCpuId   = VOS_LOCAL_CPUID; \
    ((MSG_Header *)(pstMsg))->ulSenderPid     = ACPU_PID_ADS_UL; \
    ((MSG_Header *)(pstMsg))->ulReceiverCpuId = VOS_LOCAL_CPUID; \
    ((MSG_Header *)(pstMsg))->ulReceiverPid   = (ulRecvPid);     \
    ((MSG_Header *)(pstMsg))->msgName         = (ulMsgId)
#else
#define ADS_UL_CFG_MSG_HDR(pstMsg, ulRecvPid, ulMsgId) \
    VOS_SET_SENDER_ID(pstMsg, ACPU_PID_ADS_UL);        \
    VOS_SET_RECEIVER_ID(pstMsg, ulRecvPid);            \
    ((MSG_Header *)(pstMsg))->msgName = (ulMsgId)
#endif
/* 获取OSA消息内容 */
#define ADS_UL_GET_MSG_ENTITY(pstMsg) ((VOS_VOID *)&(((MSG_Header *)(pstMsg))->msgName))

/* 获取OSA消息长度 */
#define ADS_UL_GET_MSG_LENGTH(pstMsg) (VOS_GET_MSG_LEN(pstMsg))

/* 构造BD的user field 1,第二个字节为Modem id,第一个字节的高4位为PktTpye,第一个字节的低4位为RabId */
#define ADS_UL_BUILD_BD_USER_FIELD_1(Instance, RabId)                                                        \
    (((((VOS_UINT16)Instance) << 8) & 0xFF00) | ((ADS_UL_GET_QUEUE_PKT_TYPE(Instance, RabId) << 4) & 0xF0) | \
     (RabId & 0x0F))

/* 过滤器组号，Modem0用0，MODEM1用1与实例号相同 */
#define ADS_UL_GET_BD_FC_HEAD(Instance) (Instance)

/*
 * 构造属性信息:
 *         bit0:   int_en  中断使能
 *         bit2:1  mode    模式控制
 *         bit3    rsv
 *         bit6:4  fc_head 过滤器组号 mfc_en控制域为1时，有效
 *         bit15:7 rsv
 * Boston对应的结构:
 *         bit0:   int_en  中断使能
 *         bit2:1  mode    模式控制
 *         bit4:3  rsv
 *         bit8:5  fc_head 过滤器组号 mfc_en控制域为1时，有效
 *         bit15:9 rsv
 */
#ifndef CONFIG_NEW_PLATFORM
#define ADS_UL_BUILD_BD_ATTRIBUTE(Flag, Mode, FcHead) \
    ((VOS_UINT16)(((Flag)&0x000F) | (((Mode) << 1) & 0x0006) | (((FcHead) << 4) & 0x0070)))

#define ADS_UL_SET_BD_ATTR_INT_FLAG(usAttr) ((usAttr) = (usAttr) | 0x1)
#endif

#define ADS_UL_IPF_1XHRPD (IPF_1XHRPD_ULFC)

/*lint -emacro({717}, ADS_UL_SAVE_MODEMID_PKTTYEP_RABID_TO_IMM)*/
#define ADS_UL_SAVE_MODEMID_PKTTYEP_RABID_TO_IMM(immZc, ulModemId, ucPktType, rabId) do { \
    ADS_IMM_MEM_CB(immZc)->priv[0] = (ulModemId);                                           \
    ADS_IMM_MEM_CB(immZc)->priv[0] = ((ADS_IMM_MEM_CB(immZc)->priv[0]) << 8) | (ucPktType); \
    ADS_IMM_MEM_CB(immZc)->priv[0] = ((ADS_IMM_MEM_CB(immZc)->priv[0]) << 8) | (rabId);     \
} while (0)

/*lint -emacro({717}, ADS_UL_SAVE_SLICE_TO_IMM)*/
#define ADS_UL_SAVE_SLICE_TO_IMM(immZc, slice) do { \
    ADS_IMM_MEM_CB(immZc)->priv[1] = (slice); \
} while (0)

/* 从IMM中获取ModemId */
#define ADS_UL_GET_MODEMID_FROM_IMM(pstImmZc) \
    ((VOS_UINT16)(((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0xFFFF0000) >> 16))

/* 从IMM中获取PktType */
#define ADS_UL_GET_PKTTYPE_FROM_IMM(pstImmZc) ((VOS_UINT8)(((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0x0000FF00) >> 8))

/* 从IMM中获取RabId */
#define ADS_UL_GET_RABIID_FROM_IMM(pstImmZc) ((VOS_UINT8)((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0x000000FF))

/* 从IMM中获取Slice */
#define ADS_UL_GET_SLICE_FROM_IMM(immZc) ((VOS_UINT32)ADS_IMM_MEM_CB(immZc)->priv[1])

/* 上行内存cache flush (map) */
/*lint -emacro({717}, ADS_IPF_UL_MEM_MAP)*/
#define ADS_IPF_UL_MEM_MAP(immZc, ulLen) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_FALSE) { \
        ADS_IPF_MemMapRequset(immZc, ulLen, 1); \
    }                                           \
} while (0)

/* 上行内存cache flush (unmap) */
/*lint -emacro({717}, ADS_IPF_UL_MEM_UNMAP)*/
#define ADS_IPF_UL_MEM_UNMAP(immZc, ulLen) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_FALSE) {   \
        ADS_IPF_MemUnmapRequset(immZc, ulLen, 1); \
    }                                             \
} while (0)

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/* 正常业务报文上行队列 */
#define LOW_PRIO_QUE (0)

/* PC5报文上行队列 */
#define HIGH_PRIO_QUE (1)

/* PC5和正常业务报文上行发送的优先级配比为10:1 */
#define ADS_UL_PC5_PKT_SEND_WEIGHT (10)

extern VOS_UINT32 g_pc5NodeCnt;
#endif

VOS_VOID ADS_UL_ConfigBD(VOS_UINT32 bdNum);
IMM_Zc*  ADS_UL_GetInstanceNextQueueNode(VOS_UINT32 instanceIndex, VOS_UINT32 *rabId, VOS_UINT8 *puc1XorHrpdUlIpfFlag);
IMM_Zc*  ADS_UL_GetNextQueueNode(VOS_UINT32 *rabId, VOS_UINT32 *instanceIndex, VOS_UINT8 *puc1XorHrpdUlIpfFlag);
VOS_VOID ADS_UL_ProcLinkData(VOS_VOID);
VOS_VOID ADS_UL_ProcMsg(MsgBlock *msg);
VOS_UINT32 ADS_UL_RcvTafMsg(MsgBlock *msg);
VOS_UINT32 ADS_UL_RcvTafPdpStatusInd(MsgBlock *msg);
VOS_UINT32 ADS_UL_ProcPdpStatusInd(ADS_PdpStatusInd *statusInd);
VOS_UINT32 ADS_UL_RcvCdsIpPacketMsg(MsgBlock *msg);
VOS_UINT32 ADS_UL_RcvCdsMsg(MsgBlock *msg);
VOS_UINT32 ADS_UL_RcvTimerMsg(MsgBlock *msg);
VOS_VOID   ADS_UL_SaveIpfSrcMem(const ADS_IPF_BdBuff *ipfUlBdBuff, VOS_UINT32 saveNum);
VOS_VOID   ADS_UL_FreeIpfSrcMem(VOS_UINT32 *allMemFree);
VOS_VOID   ADS_UL_ClearIpfSrcMem(VOS_VOID);
VOS_UINT32 ADS_UL_RcvCcpuResetStartInd(MsgBlock *msg);
VOS_VOID   ADS_UL_RcvTiDsFlowStatsExpired(VOS_UINT32 timerName, VOS_UINT32 param);
VOS_VOID   ADS_UL_StartDsFlowStats(VOS_UINT32 instance, VOS_UINT32 rabId);
VOS_VOID   ADS_UL_StopDsFlowStats(VOS_UINT32 instance, VOS_UINT32 rabId);
VOS_VOID   ADS_UL_RcvTiDataStatExpired(VOS_UINT32 timerName, VOS_UINT32 param);
VOS_INT    ADS_UL_CCpuResetCallback(drv_reset_cb_moment_e param, VOS_INT iUserData);

VOS_VOID ADS_UL_SndDplConfigInd(VOS_BOOL bDplNeededFlg);

VOS_UINT32 ADS_UL_RcvDplConfigInd(MsgBlock *msgBlock);

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_INT    ADS_Pc5DataIngress(IMM_Zc *immZc);
VOS_UINT32 ADS_UL_GetPrioQueue(VOS_VOID);
VOS_VOID   ADS_UL_PC5_FillIpfCfgParam(ipf_config_ulparam_s *ulCfgParam, ADS_IPF_BdBuff *ulBdBuff, IMM_Zc *immZc);
#endif

#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of AdsUlProcData.h */
