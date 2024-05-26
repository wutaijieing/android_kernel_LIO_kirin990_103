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

#ifndef __ADSDOWNLINK_H__
#define __ADSDOWNLINK_H__

#include "vos.h"
#include "ads_intra_msg.h"
#include "ads_ctx.h"
#include "mn_comm_api.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)

#define ADS_DL_ERROR_PACKET_NUM_THRESHOLD (VOS_NULL_DWORD / 100)

#define ADS_DL_IPF_RESULT_PKT_LEN_ERR_MASK (0x8000)    /* bit 15 */
#define ADS_DL_IPF_RESULT_BDCD_LEN_ERR_MASK (0x4000)   /* bit 14 */
#define ADS_DL_IPF_RESULT_PKT_PARSE_ERR_MASK (0x2000)  /* bit 13 */
#define ADS_DL_IPF_RESULT_BD_PKT_LEN_ERR_MASK (0x1000) /* bit 12 */
#define ADS_DL_IPF_RESULT_HDR_LEN_ERR_MASK (0x0800)    /* bit 11 */
#define ADS_DL_IPF_RESULT_IP_VER_ERR_MASK (0x0400)     /* bit 10 */

#define ADS_DL_IPF_RESULT_PKT_ERR_MASK                                            \
    (ADS_DL_IPF_RESULT_BDCD_LEN_ERR_MASK | ADS_DL_IPF_RESULT_PKT_PARSE_ERR_MASK | \
     ADS_DL_IPF_RESULT_BD_PKT_LEN_ERR_MASK | ADS_DL_IPF_RESULT_HDR_LEN_ERR_MASK | ADS_DL_IPF_RESULT_IP_VER_ERR_MASK)

#define ADS_DL_IPF_RESULT_PKT_ERR_FEEDBACK_MASK (ADS_DL_IPF_RESULT_HDR_LEN_ERR_MASK | ADS_DL_IPF_RESULT_IP_VER_ERR_MASK)

/*lint -emacro({717}, ADS_DL_SAVE_RD_DESC_TO_IMM)*/
#define ADS_DL_SAVE_RD_DESC_TO_IMM(immZc, rdDesc) do { \
    ADS_IMM_MEM_CB(immZc)->priv[0] = (rdDesc)->result;                                                  \
    ADS_IMM_MEM_CB(immZc)->priv[0] = ((ADS_IMM_MEM_CB(immZc)->priv[0]) << 16) | ((rdDesc)->usr_field1); \
} while (0)

/* 从IPF Result获取IP包类型 */
#define ADS_DL_GET_IPTYPE_FROM_IPF_RESULT(usRslt) ((ADS_PktTypeUint8)(((ADS_DL_IpfResult *)&(usRslt))->ipType))

/* 从IPF Result获取Beared ID */
#define ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(usRslt) ((ADS_PktTypeUint8)(((ADS_DL_IpfResult *)&(usRslt))->bearedId))

/* RD的user field 1，高1byte为Modem id，低1byte为Rab Id */
#define ADS_DL_GET_IPF_RESULT_FORM_IMM(immZc) ((VOS_UINT16)(((ADS_IMM_MEM_CB(immZc)->priv[0]) & 0xFFFF0000) >> 16))
#define ADS_DL_GET_MODEMID_FROM_IMM(immZc) ((VOS_UINT32)(((ADS_IMM_MEM_CB(immZc)->priv[0]) & 0x0000FF00) >> 8))
#define ADS_DL_GET_RABID_FROM_IMM(immZc) ((VOS_UINT32)((ADS_IMM_MEM_CB(immZc)->priv[0]) & 0x000000FF))

#define ADS_DL_IMM_PRIV_CB(immZc) ((ADS_DL_ImmPrivCb *)(&(ADS_IMM_MEM_CB(immZc)->priv[0])))

#define ADS_DL_GET_IPF_RSLT_USR_FIELD1_FROM_IMM(immZc) (ADS_IMM_MEM_CB(immZc)->priv[0])

#ifndef CONFIG_NEW_PLATFORM
#define ADS_DL_GET_IPF_RD_DESC_OUT_PTR(rdDesc) ((rdDesc)->u32OutPtr)
#else
#define ADS_DL_GET_IPF_RD_DESC_OUT_PTR(pstRdDesc) ((pstRdDesc)->out_ptr)
#endif

/* 下行内存cache invalidate (map) */
/*lint -emacro({717}, ADS_IPF_DL_MEM_MAP)*/
#define ADS_IPF_DL_MEM_MAP(immZc, len) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_FALSE) { \
        ADS_IPF_MemMapRequset(immZc, len, 0);   \
    }                                           \
} while (0)

/* 下行内存cache invalidate (unmap) */
/*lint -emacro({717}, ADS_IPF_DL_MEM_UNMAP)*/
#define ADS_IPF_DL_MEM_UNMAP(immZc, ulLen) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_FALSE) {   \
        ADS_IPF_MemUnmapRequset(immZc, ulLen, 0); \
    }                                             \
} while (0)

/* 封装OSA申请消息接口 */
#define ADS_DL_ALLOC_MSG_WITH_HDR(ulMsgLen) TAF_AllocMsgWithHeaderLen(ACPU_PID_ADS_DL, (ulMsgLen))

/* 封装OSA消息头 */
#if (VOS_OS_VER == VOS_WIN32)
#define ADS_DL_CFG_MSG_HDR(pstMsg, ulRecvPid, ulMsgId)          \
    ((MSG_Header *)(pstMsg))->ulSenderCpuId   = VOS_LOCAL_CPUID; \
    ((MSG_Header *)(pstMsg))->ulSenderPid     = ACPU_PID_ADS_DL; \
    ((MSG_Header *)(pstMsg))->ulReceiverCpuId = VOS_LOCAL_CPUID; \
    ((MSG_Header *)(pstMsg))->ulReceiverPid   = (ulRecvPid);     \
    ((MSG_Header *)(pstMsg))->msgName         = (ulMsgId)
#else
#define ADS_DL_CFG_MSG_HDR(pstMsg, ulRecvPid, ulMsgId) \
    VOS_SET_SENDER_ID(pstMsg, ACPU_PID_ADS_DL);        \
    VOS_SET_RECEIVER_ID(pstMsg, (ulRecvPid));          \
    ((MSG_Header *)(pstMsg))->msgName = (ulMsgId)
#endif
/* 封装OSA消息头(ADS内部消息) */
#define ADS_DL_CFG_INTRA_MSG_HDR(msg, ulMsgId) ADS_DL_CFG_MSG_HDR(msg, ACPU_PID_ADS_DL, ulMsgId)

/* 封装OSA消息头(CDS内部消息) */
#define ADS_DL_CFG_CDS_MSG_HDR(msg, ulMsgId) ADS_DL_CFG_MSG_HDR(msg, UEPS_PID_CDS, ulMsgId)

/* 获取OSA消息内容 */
#define ADS_DL_GET_MSG_ENTITY(msg) ((VOS_VOID *)&(((MSG_Header *)(msg))->msgName))

/* 获取OSA消息长度 */
#define ADS_DL_GET_MSG_LENGTH(msg) (VOS_GET_MSG_LEN(msg))

/* 封装OSA发送消息接口 */
/*lint -emacro({717}, ADS_DL_SEND_MSG)*/
#define ADS_DL_SEND_MSG(msg) do { \
    if (TAF_TraceAndSendMsg(ACPU_PID_ADS_DL, msg) != VOS_OK) {                           \
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SEND_MSG: Send message fail!\n"); \
    }                                                                            \
} while (0)


typedef struct {
    VOS_UINT16 bearedId : 6;
    VOS_UINT16 ipSegInfo : 2;
    VOS_UINT16 upperLayer : 1;
    VOS_UINT16 ipType : 1;
    VOS_UINT16 verErr : 1;
    VOS_UINT16 hdrLenErr : 1;
    VOS_UINT16 bdPktNotEq : 1;
    VOS_UINT16 pktParseErr : 1;
    VOS_UINT16 bdCdNotEq : 1;
    VOS_UINT16 pktLenErr : 1;

} ADS_DL_IpfResult;

/*
 * 结构说明: ADS下行IMM私有结构
 */
typedef struct {
    VOS_UINT8 rabId;
    VOS_UINT8 modemId;
    union {
        VOS_UINT16       ipfResult;
        ADS_DL_IpfResult ipfFliterResult;
    } u;

} ADS_DL_ImmPrivCb;

VOS_VOID   ADS_DL_ProcMsg(MsgBlock *msg);
VOS_VOID   ADS_DL_ProcTxData(IMM_Zc *immZc);
VOS_UINT32 ADS_DL_RcvTafPdpStatusInd(MsgBlock *msg);
VOS_UINT32 ADS_DL_RcvCdsMsg(MsgBlock *msg);
VOS_INT32  ADS_DL_IpfIntCB(VOS_VOID);
VOS_VOID   ADS_DL_ProcIpfResult(VOS_VOID);
VOS_INT32  ADS_DL_IpfAdqEmptyCB(ipf_adq_empty_e eAdqEmpty);
VOS_VOID   ADS_DL_RcvTiAdqEmptyExpired(VOS_UINT32 param, VOS_UINT32 timerName);
VOS_UINT32 ADS_DL_ConfigAdq(ipf_ad_type_e adType, VOS_UINT ipfAdNum);
VOS_VOID   ADS_DL_AllocMemForAdq(VOS_VOID);
VOS_INT    ADS_DL_CCpuResetCallback(drv_reset_cb_moment_e param, VOS_INT userData);
VOS_UINT32 ADS_DL_RcvCcpuResetStartInd(VOS_VOID);
VOS_UINT32 ADS_DL_RcvCcpuResetEndInd(MsgBlock *msg);
VOS_VOID   ADS_DL_FreeIpfUsedAd0(VOS_VOID);
VOS_VOID   ADS_DL_FreeIpfUsedAd1(VOS_VOID);
VOS_VOID   ADS_DL_SendNdClientDataInd(IMM_Zc *immZc);
VOS_VOID   ADS_DL_SendDhcpClientDataInd(IMM_Zc *immZc);
VOS_VOID   ADS_DL_RcvTiCcoreResetDelayExpired(VOS_UINT32 param, VOS_UINT32 timerName);
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_VOID   ADS_DL_ProcPc5Pkt(IMM_Zc *immZc);
VOS_VOID   ADS_DL_FreePc5FragQueue(VOS_VOID);
VOS_UINT32 ADS_DL_InsertPc5FragInQueue(IMM_Zc *immZc);
IMM_Zc*    ADS_DL_AssemblePc5Frags(VOS_VOID);
#endif

#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of AdsDlProcData.h */
