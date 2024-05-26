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

#ifndef __ADSTEST_H__
#define __ADSTEST_H__

#include "vos.h"
#include "product_config.h"
#include "ps_common_def.h"
#include "ads_intra_msg.h"
#include "ads_private.h"
#include "ads_ctx.h"
#include "msp_diag_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)

#define ADS_FLOW_DL_DEFAULT_RPT_THRESHOLD (5000000)
#define ADS_FLOW_UL_DEFAULT_RPT_THRESHOLD (500000)

#define ADS_LOG_BUFF_LEN (256)

#if defined(_lint)
#define ADS_DBG_PRINT(lvl, fmt, ...) PS_PRINTF(fmt, ##__VA_ARGS__)
#else
#define ADS_DBG_PRINT(lvl, fmt, ...) \
    ADS_LogPrintf(lvl, "[slice:%u][%s] " fmt, VOS_GetSlice(), __FUNCTION__, ##__VA_ARGS__)
#endif

#define ADS_TRACE_LOW(...) ADS_DBG_PRINT(ADS_LOG_LEVEL_INFO, __VA_ARGS__)

#define ADS_TRACE_MED(...) ADS_DBG_PRINT(ADS_LOG_LEVEL_NOTICE, __VA_ARGS__)

#define ADS_TRACE_HIGH(...) ADS_DBG_PRINT(ADS_LOG_LEVEL_ERROR, __VA_ARGS__)

/* 上行IPF中断事件统计 */
#define ADS_DBG_UL_QUE_NON_EMPTY_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.ulQueNonEmptyTrigEvent += (n))
#define ADS_DBG_UL_QUE_HIT_THRES_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.ulQueHitThresTrigEvent += (n))
#define ADS_DBG_UL_TMR_HIT_THRES_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.ulTmrHitThresTrigEvent += (n))
#define ADS_DBG_UL_10MS_TMR_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.uL10MsTmrTrigEvent += (n))
#define ADS_DBG_UL_SPE_INT_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.ulSpeIntTrigEvent += (n))
#define ADS_DBG_UL_PROC_EVENT_NUM(n) (g_adsStats.ulComStatsInfo.ulProcEventNum += (n))

/* 上行数据统计 */
#define ADS_DBG_UL_RMNET_RX_PKT_NUM(n) (g_adsStats.ulComStatsInfo.ulRmnetRxPktNum += (n))
#define ADS_DBG_UL_RMNET_MODEMID_ERR_NUM(n) (g_adsStats.ulComStatsInfo.ulRmnetModemIdErrNum += (n))
#define ADS_DBG_UL_RMNET_RABID_NUM(n) (g_adsStats.ulComStatsInfo.ulRmnetRabIdErrNum += (n))
#define ADS_DBG_UL_RMNET_ENQUE_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulRmnetEnQueSuccNum += (n))
#define ADS_DBG_UL_RMNET_ENQUE_FAIL_NUM(n) (g_adsStats.ulComStatsInfo.ulRmnetEnQueFailNum += (n))
#define ADS_DBG_UL_PKT_ENQUE_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulPktEnQueSuccNum += (n))
#define ADS_DBG_UL_PKT_ENQUE_FAIL_NUM(n) (g_adsStats.ulComStatsInfo.ulPktEnQueFailNum += (n))

/* 上行PC5数据统计 */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#define ADS_DBG_UL_PC5_QUE_NON_EMPTY_TRIG_EVENT(n) (g_adsStats.ulComStatsInfo.ulPc5QueNonEmptyTrigEvent += (n))
#define ADS_DBG_UL_PC5_RX_PKT_NUM(n) (g_adsStats.ulComStatsInfo.ulRxPc5PktNum += (n))
#define ADS_DBG_UL_PC5_PKT_ENQUE_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulPc5PktEnQueSuccNum += (n))
#define ADS_DBG_UL_PC5_PKT_ENQUE_FAIL_NUM(n) (g_adsStats.ulComStatsInfo.ulPc5PktEnQueFailNum += (n))
#define ADS_DBG_UL_PC5_BDQ_CFG_BD_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulPc5BdqCfgBdSuccNum += (n))
#define ADS_DBG_UL_PC5_RX_PKT_LEN(n) (g_adsStats.ulComStatsInfo.pc5TxPktLen = (n))
#endif

/* 上行BD统计 */
#define ADS_DBG_UL_BDQ_CFG_IPF_HAVE_NO_BD_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqCfgIpfHaveNoBd += (n))
#define ADS_DBG_UL_BDQ_CFG_BD_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqCfgBdSuccNum += (n))
#define ADS_DBG_UL_BDQ_CFG_BD_FAIL_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqCfgBdFailNum += (n))
#define ADS_DBG_UL_BDQ_CFG_IPF_SUCC_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqCfgIpfSuccNum += (n))
#define ADS_DBG_UL_BDQ_CFG_IPF_FAIL_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqCfgIpfFailNum += (n))
#define ADS_DBG_UL_BDQ_SAVE_SRC_MEM_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqSaveSrcMemNum += (n))
#define ADS_DBG_UL_BDQ_FREE_SRC_MEM_NUM(n) (g_adsStats.ulComStatsInfo.ulBdqFreeSrcMemNum += (n))
#define ADS_DBG_UL_BDQ_FREE_SRC_MEM_ERR(n) (g_adsStats.ulComStatsInfo.ulBdqFreeSrcMemErr += (n))

/* 上行组包统计 */
#define ADS_DBG_UL_WM_LEVEL_1_HIT_NUM(n) (g_adsStats.ulComStatsInfo.ulWmLevel1HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_2_HIT_NUM(n) (g_adsStats.ulComStatsInfo.ulWmLevel2HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_3_HIT_NUM(n) (g_adsStats.ulComStatsInfo.ulWmLevel3HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_4_HIT_NUM(n) (g_adsStats.ulComStatsInfo.ulWmLevel4HitNum += (n))

/* 上行复位统计 */
#define ADS_DBG_UL_RESET_CREATE_SEM_FAIL_NUM(n) (g_adsStats.resetStatsInfo.ulResetCreateSemFailNum += (n))
#define ADS_DBG_UL_RESET_LOCK_FAIL_NUM(n) (g_adsStats.resetStatsInfo.ulResetLockFailNum += (n))
#define ADS_DBG_UL_RESET_SUCC_NUM(n) (g_adsStats.resetStatsInfo.ulResetSuccNum += (n))

/* 下行IPF中断事件统计 */
#define ADS_DBG_DL_RCV_IPF_RD_INT_NUM(n) (g_adsStats.dlComStatsInfo.dlRcvIpfRdIntNum += (n))
#define ADS_DBG_DL_CCORE_RESET_TRIG_EVENT(n) (g_adsStats.dlComStatsInfo.dlcCoreResetTrigEvent += (n))
#define ADS_DBG_DL_PROC_IPF_RD_EVENT_NUM(n) (g_adsStats.dlComStatsInfo.dlProcIpfRdEventNum += (n))
#define ADS_DBG_DL_RCV_IPF_ADQ_EMPTY_INT_NUM(n) (g_adsStats.dlComStatsInfo.dlRcvIpfAdqEmptyIntNum += (n))
#define ADS_DBG_DL_RECYCLE_MEM_TRIG_EVENT(n) (g_adsStats.dlComStatsInfo.dlRecycleMemTrigEvent += (n))
#define ADS_DBG_DL_PROC_IPF_AD_EVENT_NUM(n) (g_adsStats.dlComStatsInfo.dlProcIpfAdEventNum += (n))

/* 下行RD统计 */
#define ADS_DBG_DL_RDQ_RX_RD_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqRxRdNum += (n))
#define ADS_DBG_DL_RDQ_GET_RD0_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqGetRd0Num += (n))
#define ADS_DBG_DL_RDQ_TRANS_MEM_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqTransMemFailNum += (n))
#define ADS_DBG_DL_RDQ_RX_NORM_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqRxNormPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqRxNdPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_DHCP_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqRxDhcpPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_ERR_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqRxErrPktNum += (n))
#define ADS_DBG_DL_RDQ_FILTER_ERR_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRdqFilterErrNum += (n))
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#define ADS_DBG_DL_RDQ_RX_PC5_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dLRdqRxPc5PktNum += (n))
#define ADS_DBG_DL_RDQ_TX_PC5_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dLRdqTxPc5PktNum += (n))
#define ADS_DBG_DL_PC5_FRAG_ENQUE_NUM(n) (g_adsStats.dlComStatsInfo.dLPc5FragEnqueNum += (n))
#define ADS_DBG_DL_PC5_FRAG_FREE_NUM(n) (g_adsStats.dlComStatsInfo.dLPc5FragFreeNum += (n))
#define ADS_DBG_DL_PC5_FRAG_RELEASE_NUM(n) (g_adsStats.dlComStatsInfo.dLPc5FragReleaseNum += (n))
#define ADS_DBG_DL_PC5_FRAG_ASSEM_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dLPc5FragAssemSuccNum += (n))
#define ADS_DBG_DL_RDQ_RX_PC5_PKT_LEN(n) (g_adsStats.dlComStatsInfo.pc5RxPktLen = (n))
#endif

/* 下行数据统计 */
#define ADS_DBG_DL_RMNET_TX_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRmnetTxPktNum += (n))
#define ADS_DBG_DL_RMNET_MODEMID_ERR_NUM(n) (g_adsStats.dlComStatsInfo.dlRmnetModemIdErrNum += (n))
#define ADS_DBG_DL_RMNET_RABID_ERR_NUM(n) (g_adsStats.dlComStatsInfo.dlRmnetRabIdErrNum += (n))
#define ADS_DBG_DL_RMNET_NO_FUNC_FREE_PKT_NUM(n) (g_adsStats.dlComStatsInfo.dlRmnetNoFuncFreePktNum += (n))

/* 下行AD统计  */
#define ADS_DBG_DL_ADQ_ALLOC_SYS_MEM_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqAllocSysMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_SYS_MEM_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqAllocSysMemFailNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_MEM_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqAllocMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_MEM_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqAllocMemFailNum += (n))
#define ADS_DBG_DL_ADQ_FREE_MEM_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqFreeMemNum += (n))
#define ADS_DBG_DL_ADQ_RECYCLE_MEM_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqRecycleMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_RECYCLE_MEM_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqRecycleMemFailNum += (n))
#define ADS_DBG_DL_ADQ_GET_FREE_AD_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqGetFreeAdSuccNum += (n))
#define ADS_DBG_DL_ADQ_GET_FREE_AD_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqGetFreeAdFailNum += (n))
#define ADS_DBG_DL_ADQ_GET_IPF_AD0_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqGetIpfAd0FailNum += (n))
#define ADS_DBG_DL_ADQ_GET_IPF_AD1_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqGetIpfAd1FailNum += (n))
#define ADS_DBG_DL_ADQ_CFG_AD_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqCfgAdNum += (n))
#define ADS_DBG_DL_ADQ_CFG_AD0_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqCfgAd0Num += (n))
#define ADS_DBG_DL_ADQ_CFG_AD1_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqCfgAd1Num += (n))
#define ADS_DBG_DL_ADQ_FREE_AD0_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqFreeAd0Num += (n))
#define ADS_DBG_DL_ADQ_FREE_AD1_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqFreeAd1Num += (n))
#define ADS_DBG_DL_ADQ_CFG_IPF_SUCC_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqCfgIpfSuccNum += (n))
#define ADS_DBG_DL_ADQ_CFG_IPF_FAIL_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqCfgIpfFailNum += (n))
#define ADS_DBG_DL_ADQ_START_EMPTY_TMR_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqStartEmptyTmrNum += (n))
#define ADS_DBG_DL_ADQ_EMPTY_TMR_TIMEOUT_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqEmptyTmrTimeoutNum += (n))
#define ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqRcvAd0EmptyIntNum += (n))
#define ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(n) (g_adsStats.dlComStatsInfo.dlAdqRcvAd1EmptyIntNum += (n))

/* 下行流控统计 */
#define ADS_DBG_DL_FC_ACTIVATE_NUM(n) (g_adsStats.dlComStatsInfo.dlFcActivateNum += (n))
#define ADS_DBG_DL_FC_TMR_TIMEOUT_NUM(n) (g_adsStats.dlComStatsInfo.dlFcTmrTimeoutNum += (n))

/* 下行复位统计 */
#define ADS_DBG_DL_RESET_CREATE_SEM_FAIL_NUM(n) (g_adsStats.resetStatsInfo.dlResetCreateSemFailNum += (n))
#define ADS_DBG_DL_RESET_LOCK_FAIL_NUM(n) (g_adsStats.resetStatsInfo.dlResetLockFailNum += (n))
#define ADS_DBG_DL_RESET_SUCC_NUM(n) (g_adsStats.resetStatsInfo.dlResetSuccNum += (n))

/*
 * 协议表格:
 * ASN.1描述:
 * 枚举说明: ADS流量上报Debug开关
 */
enum ADS_FlowDebug {
    ADS_FLOW_DEBUG_OFF    = 0,
    ADS_FLOW_DEBUG_DL_ON  = 1,
    ADS_FLOW_DEBUG_UL_ON  = 2,
    ADS_FLOW_DEBUG_ALL_ON = 3,
    ADS_FLOW_DEBUG_BUTT
};

/*
 * 枚举说明: ADS打印LOG等级
 */
enum ADS_LogLevel {
    ADS_LOG_LEVEL_DEBUG = 0, /* 0x0: debug-level                      */
    ADS_LOG_LEVEL_INFO,      /* 0x1: informational                    */
    ADS_LOG_LEVEL_NOTICE,    /* 0x2: normal but significant condition */
    ADS_LOG_LEVEL_WARNING,   /* 0x3: warning conditions               */
    ADS_LOG_LEVEL_ERROR,     /* 0x4: error conditions                 */
    ADS_LOG_LEVEL_FATAL,     /* 0x5: fatal error condition            */

    ADS_LOG_LEVEL_BUTT
};
typedef VOS_UINT32 ADS_LogLevelUint32;


typedef struct {
    /* 上行IPF事件统计 */
    VOS_UINT32 ulQueNonEmptyTrigEvent;
    VOS_UINT32 ulQueFullTrigEvent;
    VOS_UINT32 ulQueHitThresTrigEvent;
    VOS_UINT32 ulTmrHitThresTrigEvent;
    VOS_UINT32 uL10MsTmrTrigEvent;
    VOS_UINT32 ulSpeIntTrigEvent;
    VOS_UINT32 ulProcEventNum;

    /* 上行数据统计 */
    VOS_UINT32 ulRmnetRxPktNum;
    VOS_UINT32 ulRmnetModemIdErrNum;
    VOS_UINT32 ulRmnetRabIdErrNum;
    VOS_UINT32 ulRmnetEnQueSuccNum;
    VOS_UINT32 ulRmnetEnQueFailNum;
    VOS_UINT32 ulPktEnQueSuccNum;
    VOS_UINT32 ulPktEnQueFailNum;

    /* 上行BD统计 */
    VOS_UINT32 ulBdqCfgIpfHaveNoBd;
    VOS_UINT32 ulBdqCfgBdSuccNum;
    VOS_UINT32 ulBdqCfgBdFailNum;
    VOS_UINT32 ulBdqCfgIpfSuccNum;
    VOS_UINT32 ulBdqCfgIpfFailNum;
    VOS_UINT32 ulBdqSaveSrcMemNum;
    VOS_UINT32 ulBdqFreeSrcMemNum;
    VOS_UINT32 ulBdqFreeSrcMemErr;

    /* 上行组包统计 */
    VOS_UINT32 ulWmLevel1HitNum;
    VOS_UINT32 ulWmLevel2HitNum;
    VOS_UINT32 ulWmLevel3HitNum;
    VOS_UINT32 ulWmLevel4HitNum;

    /* 上行流量统计 */
    VOS_UINT32 ulFlowDebugFlag;
    VOS_UINT32 ulFlowRptThreshold;
    VOS_UINT32 ulFlowInfo;
    VOS_UINT32 ulStartSlice;
    VOS_UINT32 ulEndSlice;

    /* PC5 上行处理统计 */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    VOS_UINT32 ulPc5QueNonEmptyTrigEvent;
    VOS_UINT32 ulRxPc5PktNum;
    VOS_UINT32 ulPc5PktEnQueSuccNum;
    VOS_UINT32 ulPc5PktEnQueFailNum;
    VOS_UINT32 ulPc5BdqCfgBdSuccNum;
    VOS_UINT32 pc5TxPktLen; /* 上行最近一次发送的PC5报文大小 */
#endif
} ADS_UL_ComStatsInfo;


typedef struct {
    /* 下行IPF事件统计 */
    VOS_UINT32 dlRcvIpfRdIntNum;
    VOS_UINT32 dlcCoreResetTrigEvent;
    VOS_UINT32 dlProcIpfRdEventNum;
    VOS_UINT32 dlRcvIpfAdqEmptyIntNum;
    VOS_UINT32 dlRecycleMemTrigEvent;
    VOS_UINT32 dlProcIpfAdEventNum;

    /* 下行RD统计 */
    VOS_UINT32 dlRdqRxRdNum;
    VOS_UINT32 dlRdqGetRd0Num;
    VOS_UINT32 dlRdqTransMemFailNum;
    VOS_UINT32 dlRdqRxNormPktNum;
    VOS_UINT32 dlRdqRxNdPktNum;
    VOS_UINT32 dlRdqRxDhcpPktNum;
    VOS_UINT32 dlRdqRxErrPktNum;
    VOS_UINT32 dlRdqFilterErrNum;
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    VOS_UINT32 dLRdqRxPc5PktNum;
    VOS_UINT32 dLRdqTxPc5PktNum;
    VOS_UINT32 dLPc5FragEnqueNum;
    VOS_UINT32 dLPc5FragFreeNum;
    VOS_UINT32 dLPc5FragReleaseNum;
    VOS_UINT32 dLPc5FragAssemSuccNum;
    VOS_UINT32 pc5RxPktLen; /* 下行最近一次接收的PC5报文大小 */
#endif

    /* 下行数据统计 */
    VOS_UINT32 dlRmnetTxPktNum;
    VOS_UINT32 dlRmnetModemIdErrNum;
    VOS_UINT32 dlRmnetRabIdErrNum;
    VOS_UINT32 dlRmnetNoFuncFreePktNum;

    /* 下行AD统计 */
    VOS_UINT32 dlAdqAllocSysMemSuccNum;
    VOS_UINT32 dlAdqAllocSysMemFailNum;
    VOS_UINT32 dlAdqAllocMemSuccNum;
    VOS_UINT32 dlAdqAllocMemFailNum;
    VOS_UINT32 dlAdqFreeMemNum;
    VOS_UINT32 dlAdqRecycleMemSuccNum;
    VOS_UINT32 dlAdqRecycleMemFailNum;
    VOS_UINT32 dlAdqGetFreeAdSuccNum;
    VOS_UINT32 dlAdqGetFreeAdFailNum;
    VOS_UINT32 dlAdqGetIpfAd0FailNum;
    VOS_UINT32 dlAdqGetIpfAd1FailNum;
    VOS_UINT32 dlAdqCfgAdNum;
    VOS_UINT32 dlAdqCfgAd0Num;
    VOS_UINT32 dlAdqCfgAd1Num;
    VOS_UINT32 dlAdqFreeAd0Num;
    VOS_UINT32 dlAdqFreeAd1Num;
    VOS_UINT32 dlAdqCfgIpfSuccNum;
    VOS_UINT32 dlAdqCfgIpfFailNum;
    VOS_UINT32 dlAdqStartEmptyTmrNum;
    VOS_UINT32 dlAdqEmptyTmrTimeoutNum;
    VOS_UINT32 dlAdqRcvAd0EmptyIntNum;
    VOS_UINT32 dlAdqRcvAd1EmptyIntNum;

    /* 下行流控统计 */
    VOS_UINT32 dlFcActivateNum;
    VOS_UINT32 dlFcTmrTimeoutNum;

    /* 下行流量统计 */
    VOS_UINT32 dlFlowDebugFlag;
    VOS_UINT32 dlFlowRptThreshold;
    VOS_UINT32 dlFlowInfo;
    VOS_UINT32 dlStartSlice;
    VOS_UINT32 dlEndSlice;

} ADS_DL_ComStatsInfo;


typedef struct {
    /* 上行复位统计 */
    VOS_UINT32 ulResetCreateSemFailNum;
    VOS_UINT32 ulResetLockFailNum;
    VOS_UINT32 ulResetSuccNum;

    /* 下行复位统计 */
    VOS_UINT32 dlResetCreateSemFailNum;
    VOS_UINT32 dlResetLockFailNum;
    VOS_UINT32 dlResetSuccNum;
} ADS_RESET_StatsInfo;


typedef struct {
    ADS_UL_ComStatsInfo ulComStatsInfo;
    ADS_DL_ComStatsInfo dlComStatsInfo;
    ADS_RESET_StatsInfo resetStatsInfo;

} ADS_StatsInfo;

extern ADS_StatsInfo g_adsStats;
extern VOS_UINT32    g_adsDlDiscardPktFlag;

VOS_VOID ADS_ResetDebugInfo(VOS_VOID);
VOS_VOID ADS_ULFlowAdd(VOS_UINT32 sduLen);
VOS_VOID ADS_LogPrintf(ADS_LogLevelUint32 level, VOS_CHAR *pcFmt, ...);
VOS_VOID ADS_ShowDLInfoStats(VOS_VOID);
VOS_VOID ADS_Help(VOS_VOID);

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_VOID ADS_ShowPc5PktProcStats(VOS_VOID);
VOS_VOID ADS_SetPc5MaxQueueLen(VOS_UINT32 length);
#endif

#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of ads_debug.h */
