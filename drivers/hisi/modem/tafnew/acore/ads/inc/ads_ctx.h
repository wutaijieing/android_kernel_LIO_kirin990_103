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

#ifndef __ADSCTX_H__
#define __ADSCTX_H__

#include "vos.h"
#include "PsLogdef.h"
#include "PsTypeDef.h"
#include "ps_lib.h"
#include "ads_interface.h"
#include "ads_device_interface.h"
#include "cds_ads_interface.h"
#include "ads_nd_interface.h"
#include "ads_timer_mgmt.h"
#include "ads_log.h"
#include "mdrv.h"
#include "mdrv_nvim.h"
#include "acore_nv_stru_gucnas.h"
#include "rnic_dev_def.h"

#if (VOS_OS_VER == VOS_LINUX)
#include <asm/dma-mapping.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/pm_wakeup.h>
#include <linux/of_platform.h>
#else
#include "Linuxstub.h"
#endif /* VOS_OS_VER == VOS_LINUX */

#if (defined(CONFIG_BALONG_SPE))
#include <linux/spe/spe_interface.h>
#include "mdrv_spe_wport.h"
#endif /* CONFIG_BALONG_SPE */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (defined(CONFIG_BALONG_SPE))
#define ADS_DMA_MAC_HEADER_RES_LEN IMM_MAC_HEADER_RES_LEN
#else
#define ADS_DMA_MAC_HEADER_RES_LEN 0
#endif

/* ADS实例个数与Modem个数保持一致 */
#define ADS_INSTANCE_MAX_NUM (MODEM_ID_BUTT)

/* ADS实例索引值 */
#define ADS_INSTANCE_INDEX_0 (0)
#define ADS_INSTANCE_INDEX_1 (1)
#define ADS_INSTANCE_INDEX_2 (2)

/* 上下缓存队列的最大值 */
#define ADS_RAB_NUM_MAX (11)

/* 当前到网络的定义是5到15, 因此有一个5的偏移量 */
#define ADS_RAB_ID_OFFSET (5)

/* Rab Id的最小值 */
#define ADS_RAB_ID_MIN (5)

/* Rab Id的最大值 */
#define ADS_RAB_ID_MAX (15)

/* Rab Id的无效值 */
#define ADS_RAB_ID_INVALID (0xFF)

/* 为了避免频繁触发IPF中断，需要采用攒包机制，攒包的最大个数 */
#define ADS_UL_SEND_DATA_NUM_THREDHOLD (g_adsCtx.adsIpfCtx.thredHoldNum)

#define ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(n) (g_adsCtx.adsIpfCtx.thredHoldNum = (n))

#define ADS_UL_RX_WAKE_LOCK_TMR_LEN (g_adsCtx.adsIpfCtx.rxWakeLockTmrLen)
#define ADS_DL_TX_WAKE_LOCK_TMR_LEN (g_adsCtx.adsIpfCtx.txWakeLockTmrLen)

/* 申请的AD需要偏移14作为IPF RD的目的地址，14为MAC头的长度 */
#define ADS_DL_AD_DATA_PTR_OFFSET (14)

/* 默认的优先级加权数 */
#define ADS_UL_DEFAULT_PRI_WEIGHTED_NUM (0x01)

#define ADS_PKT_ERR_RATE_MIN_THRESHOLD (50)
#define ADS_PKT_ERR_RATE_MAX_THRESHOLD (100)
#define ADS_PKT_ERR_RATE_DEFAULT_THRESHOLD (95)

#define ADS_PKT_ERR_DETECT_MIN_DURATION (1000)
#define ADS_PKT_ERR_DETECT_MAX_DURATION (10000)
#define ADS_PKT_ERR_DETECT_DEFAULT_DURATION (2000)
#define ADS_PKT_ERR_DETECT_DEFAULT_DELTA (1000)

#define ADS_IS_PKT_ERR_RATE_THRESHOLD_VALID(val) \
    (((val) >= ADS_PKT_ERR_RATE_MIN_THRESHOLD) && ((val) <= ADS_PKT_ERR_RATE_MAX_THRESHOLD))

#define ADS_IS_PKT_ERR_DETECT_DURATION_VALID(val) \
    (((val) >= ADS_PKT_ERR_DETECT_MIN_DURATION) && ((val) <= ADS_PKT_ERR_DETECT_MAX_DURATION))

#define ADS_CURRENT_TICK (jiffies)
#define ADS_TIME_AFTER(a, b) time_after((a), (b))
#define ADS_TIME_AFTER_EQ(a, b) time_after_eq((a), (b))
#define ADS_TIME_IN_RANGE(a, b, c) time_in_range((a), (b), (c))

/* ADS ADQ的个数 */
#define ADS_DL_ADQ_MAX_NUM (2)

/* 启动ADQ空保护定时器的阈值，当可配置AD数量大于该值时则启动保护定时器 */
#define ADS_IPF_DLAD_START_TMR_THRESHOLD (IPF_DLAD0_DESC_SIZE - 6)

/* ADS使用的SPE端口号 */
#define ADS_IPF_SPE_PORT_0 (SPE_PORT_MAX + 128)
#define ADS_IPF_SPE_PORT_1 (SPE_PORT_MAX + 129)

/* AD内存队列大小 */
#define ADS_IPF_AD0_MEM_BLK_NUM (IPF_DLAD0_DESC_SIZE * 3)
#define ADS_IPF_AD1_MEM_BLK_NUM (IPF_DLAD1_DESC_SIZE * 4)
#define ADS_IPF_AD_MEM_RESV_BLK_NUM (IPF_DLAD1_DESC_SIZE * 2)

/* AD数据包的长度 */
#define ADS_IPF_AD0_MEM_BLK_SIZE (448)
#define ADS_IPF_AD1_MEM_BLK_SIZE (1536 + 14)

#define ADS_IPF_RD_BUFF_RECORD_NUM (IPF_DLRD_DESC_SIZE * 2)
#define ADS_IPF_AD_BUFF_RECORD_NUM (IPF_DLAD0_DESC_SIZE * 2)

/*
 * ADS_UL_SendPacket和ADS_DL_RegDlDataCallback的rabid为扩展的rabid，
 * 高2bit作为MODEM ID，低6bit作为RAB ID。根据扩展的RABID获取MODEM ID
 */
#define ADS_GET_MODEMID_FROM_EXRABID(i) ((i >> 6) & 0x03)

#define ADS_GET_RABID_FROM_EXRABID(i) (i & 0x3F)

#define ADS_BUILD_EXRABID(mid, rabid) ((VOS_UINT8)((((mid) << 6) & 0xC0) | ((rabid) & 0x3F)))

#define ADS_BIT16_MASK(bit) ((VOS_UINT16)(1 << (bit)))
#define ADS_BIT16_SET(val, bit) ((val) | ADS_BIT16_MASK(bit))
#define ADS_BIT16_CLR(val, bit) ((val) & ~ADS_BIT16_MASK(bit))
#define ADS_BIT16_IS_SET(val, bit) ((val) & ADS_BIT16_MASK(bit))

/* ******************************任务事件 Begin****************************** */

/* ADS上行任务事件 */
#define ADS_UL_EVENT_BASE (0x00000000)
#define ADS_UL_EVENT_DATA_PROC (ADS_UL_EVENT_BASE | 0x0001)

/* ADS下行任务事件 */
#define ADS_DL_EVENT_BASE (0x00000000)
#define ADS_DL_EVENT_IPF_RD_INT (ADS_DL_EVENT_BASE | 0x0001)
#define ADS_DL_EVENT_IPF_ADQ_EMPTY_INT (ADS_DL_EVENT_BASE | 0x0002)
#define ADS_DL_EVENT_IPF_FILTER_DATA_PROC (ADS_DL_EVENT_BASE | 0x0004)

/* ******************************任务事件 End****************************** */

/* ******************************速率统计 Begin****************************** */
/* 获取流量统计上下文 */
#define ADS_GET_DSFLOW_STATS_CTX_PTR() (&(g_adsCtx.dsFlowStatsCtx))

/* 设置当前上行速率 */
#define ADS_SET_CURRENT_UL_RATE(n) (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulCurDataRate = (n))

/* 设置当前下行速率 */
#define ADS_SET_CURRENT_DL_RATE(n) (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlCurDataRate = (n))

/* 统计上行周期性收到包的个数 */
#define ADS_RECV_UL_PERIOD_PKT_NUM(n) (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes += (n))

/* 统计下行周期性收到包的个数 */
#define ADS_RECV_DL_PERIOD_PKT_NUM(n) (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes += (n))

/* 获取上行周期性收到包的个数 */
#define ADS_GET_UL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes)

/* 获取下行周期性收到包的个数 */
#define ADS_GET_DL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes)

/* 将上行周期性收到包的个数清零 */
#define ADS_CLEAR_UL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes = 0)

/* 将下行周期性收到包的个数清零 */
#define ADS_CLEAR_DL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes = 0)

/* ******************************速率统计 End****************************** */

/* ******************************上行 Begin****************************** */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/* 获取上行Pc5队列头结点 */
#define ADS_UL_GET_PC5_QUEUE_HEAD() (&(g_adsCtx.adsPc5Ctx.ulQueue))

/* 获取上行Pc5队列最大长度 */
#define ADS_UL_GET_PC5_MAX_QUEUE_LEN() (g_adsCtx.adsPc5Ctx.ulQueueMaxLen)

/* 设置上行Pc5队列最大长度 */
#define ADS_UL_SET_PC5_MAX_QUEUE_LEN(n) (g_adsCtx.adsPc5Ctx.ulQueueMaxLen = (n))
#endif

/* 获取ADS上行实体地址 */
#define ADS_UL_GET_CTX_PTR(i) (&(g_adsCtx.adsSpecCtx[i].adsUlCtx))

/* 获取上行队列指针 */
#define ADS_UL_GET_QUEUE_LINK_PTR(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[j].adsUlLink)

/* 获取上行队列锁 */
#define ADS_UL_GET_QUEUE_LINK_SPINLOCK(i, j) (&(g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[j].spinLock))

/* 获取ADS上行BD配置参数地址 */
#define ADS_UL_GET_BD_CFG_PARA_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfUlBdCfgParam[indexNum]))

/* 获取ADS上行BD缓存地址 */
#define ADS_UL_GET_BD_BUFF_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfUlBdBuff[indexNum]))

/* 获取ADS上行发送保护定时器时长 */
#define ADS_UL_GET_PROTECT_TIMER_LEN() (g_adsCtx.adsIpfCtx.protectTmrLen)

/* 获取数据是否正在发送的标志位 */
#define ADS_UL_GET_SENDING_FLAG() (g_adsCtx.adsIpfCtx.sendingFlg)

/* 设置数据是否正在发送的标志位 */
#define ADS_UL_SET_SENDING_FLAG(flg) (g_adsCtx.adsIpfCtx.sendingFlg = flg)

/* 获取存储队列的索引 */
#define ADS_UL_GET_PRIO_QUEUE_INDEX(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.prioIndex[j])

/* 获取上行队列调度优先级 */
#define ADS_UL_GET_QUEUE_QCI(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].prio)

/* 获取上行队列信息 */
#define ADS_UL_GET_QUEUE_LINK_INFO(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].adsUlLink)

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
/* 获取IX OR HRPD的上行IPF过滤组标志 */
#define ADS_UL_GET_1X_OR_HRPD_UL_IPF_FLAG(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].uc1XorHrpdUlIpfFlag)
#endif

/* 获取上行队列一个加权数范围内记录的发送个数 */
#define ADS_UL_GET_RECORD_NUM_IN_WEIGHTED(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum)

/* 设置上行队列一个加权数范围内记录的发送个数 */
#define ADS_UL_SET_RECORD_NUM_IN_WEIGHTED(i, j, n) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum += n)

/* 清除上行队列一个加权数范围内记录的发送个数 */
#define ADS_UL_CLR_RECORD_NUM_IN_WEIGHTED(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum = 0)

/* 获取上行队列优先级的加权数 */
#define ADS_UL_GET_QUEUE_PRI_WEIGHTED_NUM(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.queuePriNv.priWeightedNum[ADS_UL_GET_QUEUE_QCI(i, j)])

/* 获取上行队列数据包的类型 */
#define ADS_UL_GET_QUEUE_PKT_TYPE(InstanceIndex, RabId) \
    (g_adsCtx.adsSpecCtx[InstanceIndex].adsUlCtx.adsUlQueue[RabId].pktType)

/* 获取ADS上行发送保护定时器时长 */
#define ADS_UL_GET_STAT_TIMER_LEN() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statTmrLen)

#define ADS_UL_ADD_STAT_PKT_NUM(n) (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum += (n))

#define ADS_UL_GET_STAT_PKT_NUM() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum)

#define ADS_UL_CLR_STAT_PKT_NUM() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum = 0)

/* 获取上行赞包控制标记 */
#define ADS_UL_GET_THRESHOLD_ACTIVE_FLAG() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.activeFlag)

/* 获取上行赞包的jiffies的时间计数 */
#define ADS_UL_GET_JIFFIES_TMR_CNT() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrCnt)

/* 设置上行赞包的jiffies的时间计数 */
#define ADS_UL_SET_JIFFIES_TMR_CNT(n) (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrCnt = (n))

/* 获取上行赞包的jiffies的超时长度 */
#define ADS_UL_GET_JIFFIES_EXP_TMR_LEN() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrExpCnt)

/* 获取上行数据包水线等级 */
#define ADS_UL_GET_WATER_LEVEL_ONE() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel1)

#define ADS_UL_GET_WATER_LEVEL_TWO() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel2)

#define ADS_UL_GET_WATER_LEVEL_THREE() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel3)

#define ADS_UL_GET_WATER_LEVEL_FOUR() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.stWaterMarkLevel.ulWaterLevel4)

/* 获取上行赞包门限 */
#define ADS_UL_DATA_THRESHOLD_ONE (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold1)

#define ADS_UL_DATA_THRESHOLD_TWO (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold2)

#define ADS_UL_DATA_THRESHOLD_THREE (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold3)

#define ADS_UL_DATA_THRESHOLD_FOUR (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold4)

/* ******************************上行 End****************************** */

/* 上行IPF内存释放队列首地址 */
#define ADS_UL_IPF_SRCMEM_FREE_QUE() (&(g_adsCtx.adsIpfCtx.ulSrcMemFreeQue))

/* IPF上行源内存释放QUEUE，定义限长为上行IPF BD描述符的2倍 */
#define ADS_UL_IPF_SRCMEM_FREE_QUE_SIZE (2 * IPF_ULBD_DESC_SIZE)

/* 通过限制ADS上行队列长度，限制A核系统内存，队列限长初始化值 */
#if (defined(CONFIG_BALONG_SPE))
#define ADS_UL_MAX_QUEUE_LENGTH (448)
#else
#define ADS_UL_MAX_QUEUE_LENGTH (500)
#endif

/* 获取上行队列限长 */
#define ADS_UL_GET_MAX_QUEUE_LENGTH(i) (g_adsCtx.adsSpecCtx[i].adsUlCtx.ulMaxQueueLength)

/* 获取ADS下行IPF AD BUFFER地址 */
#define ADS_DL_GET_IPF_AD_DESC_PTR(i, j) (&(g_adsCtx.adsIpfCtx.ipfDlAdDesc[i][j]))
#define ADS_DL_GET_IPF_AD_RECORD_PTR(i) (&(g_adsCtx.adsIpfCtx.ipfDlAdRecord[i]))

/* ******************************下行 Begin****************************** */

/* 获取ADS下行实体地址 */
#define ADS_DL_GET_CTX_PTR(i) (&(g_adsCtx.adsSpecCtx[i].adsDlCtx))

/* 获取ADS下行IPF RD BUFFER地址 */
#define ADS_DL_GET_IPF_RD_DESC_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfDlRdDesc[indexNum]))
#define ADS_DL_GET_IPF_RD_RECORD_PTR() (&(g_adsCtx.adsIpfCtx.ipfDlRdRecord))

/* 获取ADS下行RAB INFO地址 */
#define ADS_DL_GET_RAB_INFO_PTR(i, rabid) (&(ADS_DL_GET_CTX_PTR(i)->adsDlRabInfo[rabid - ADS_RAB_ID_OFFSET]))

/* 获取ADS下行RAB对应的数据包类型 */
#define ADS_DL_GET_PKT_TYPE(instance, rabid) \
    (ADS_DL_GET_CTX_PTR(instance)->adsDlRabInfo[rabid - ADS_RAB_ID_OFFSET].pktType)

/* 获取ADS下行数据回调函数指针 */
#define ADS_DL_GET_DATA_CALLBACK_FUNC(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->rcvDlDataFunc)
#define ADS_DL_GET_DATA_EXPARAM(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->exParam)

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
/* 获取ADS下行RD最后一个数据处理回调函数指针 */
#define ADS_DL_GET_RD_LST_DATA_CALLBACK_FUNC(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->rcvRdLstDataFunc)
#endif

/* get/set the last data buff ptr */
#define ADS_DL_GET_LST_DATA_PTR(mid, rabid) (ADS_DL_GET_RAB_INFO_PTR(mid, rabid)->lstPkt)
#define ADS_DL_SET_LST_DATA_PTR(mid, rabid, psPara) (ADS_DL_GET_RAB_INFO_PTR(mid, rabid)->lstPkt = (psPara))

#define ADS_DL_GET_FILTER_DATA_CALLBACK_FUNC(ucInstanceIndex, ucRabId) \
    (ADS_DL_GET_RAB_INFO_PTR(ucInstanceIndex, ucRabId)->pRcvDlFilterDataFunc)

/* 获取ADS下行流控参数信息地址 */
#define ADS_DL_GET_FC_ASSEM_INFO_PTR(i) (&(ADS_DL_GET_CTX_PTR(i)->fcAssemInfo))

/* 获取ADS下行流控组包参数调整回调函数指针 */
#define ADS_DL_GET_FC_ASSEM_TUNE_FUNC(i) (ADS_DL_GET_FC_ASSEM_INFO_PTR(i)->pFcAssemTuneFunc)

#define ADS_DL_GET_PKT_ERR_FEEDBACK_CFG_PTR() (&(g_adsCtx.pktErrFeedbackCfg))
#define ADS_DL_GET_PKT_ERR_STATS_PTR(mid, rabid) (&(g_adsCtx.pktErrStats[mid][rabid - ADS_RAB_ID_OFFSET]))

/* 获取ADS下行C核单独复位延时定时器时长 */
#define ADS_DL_GET_CCORE_RESET_DELAY_TIMER_LEN() (g_adsCtx.adsIpfCtx.cCoreResetDelayTmrLen)

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/* 获取下行Pc5分片报文队列头 */
#define ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD() (&(g_adsCtx.adsPc5Ctx.fragQue))

/* 设置下行Pc5分片队列最大长度 */
#define ADS_DL_SET_PC5_FRAGS_QUEUE_MAX_LEN(n) (g_adsCtx.adsPc5Ctx.fragQueMaxLen = (n))

/* 获取下行Pc5分片队列最大长度 */
#define ADS_DL_GET_PC5_FRAGS_QUEUE_MAX_LEN() (g_adsCtx.adsPc5Ctx.fragQueMaxLen)

/* 获取下行Pc5分片报文当前长度总和 */
#define ADS_DL_GET_PC5_FRAGS_LEN() (g_adsCtx.adsPc5Ctx.fragsLen)

/* 设置下行Pc5分片报文当前长度总和 */
#define ADS_DL_UPDATE_PC5_FRAGS_LEN(n) (g_adsCtx.adsPc5Ctx.fragsLen += (n))

/* 清空下行Pc5分片报文当前长度总和 */
#define ADS_DL_CLEAR_PC5_FRAGS_LEN() (g_adsCtx.adsPc5Ctx.fragsLen = 0)

/* 设置下一个下行Pc5分片报文序号 */
#define ADS_DL_SET_NEXT_PC5_FRAG_SEQ(n) (g_adsCtx.adsPc5Ctx.nextFragSeq = (n))

/* 获取下一个下行Pc5分片报文序号 */
#define ADS_DL_GET_NEXT_PC5_FRAG_SEQ() (g_adsCtx.adsPc5Ctx.nextFragSeq)
#endif

/* ******************************下行 End****************************** */

/* ******************************IPF Begin****************************** */
/* 获取IPF相关的上下文 */
#define ADS_GET_IPF_CTX_PTR() (&(g_adsCtx.adsIpfCtx))

#define ADS_GET_IPF_DEV() (g_dmaDev)

#define ADS_IMM_MEM_CB(immZc) ((ADS_ImmMemCb *)((immZc)->cb))

#if (defined(CONFIG_BALONG_SPE))
#define ADS_GET_IPF_SPE_WPORT() (g_adsCtx.adsIpfCtx.lSpeWPort)
#define ADS_GET_IPF_MEM_POOL_CFG_PTR() (&(g_adsCtx.adsIpfCtx.stMemPoolCfg))
#define ADS_GET_IPF_MEM_POOL_PTR(id) (&(g_adsCtx.adsIpfCtx.astMemPool[id]))
#define ADS_GET_IPF_MEM_QUE(id) (&(g_adsCtx.adsIpfCtx.astMemPool[id].que))
#define ADS_SPE_MEM_CB(immZc) ((ADS_SpeMemCb *)&((immZc)->dma))
#endif

/*lint -emacro({717}, ADS_IPF_SPE_MEM_MAP)*/
#define ADS_IPF_SPE_MEM_MAP(immZc, ulLen) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_TRUE) {       \
        ADS_IPF_MemMapByDmaRequset(immZc, ulLen, 0); \
    }                                                \
} while (0)

/*lint -emacro({717}, ADS_IPF_SPE_MEM_UNMAP)*/
#define ADS_IPF_SPE_MEM_UNMAP(immZc, ulLen) do { \
    if (ADS_IPF_IsSpeMem(immZc) == VOS_TRUE) {    \
        ADS_IPF_MemUnmapRequset(immZc, ulLen, 0); \
    }                                             \
} while (0)

/* 获取IPF模式 */
#define ADS_GET_IPF_MODE() (g_adsCtx.adsIpfCtx.ipfMode)

#define ADS_GET_IPF_FILTER_QUE() (&(g_adsCtx.adsIpfCtx.ipfFilterQue))

/* ******************************IPF End****************************** */

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#define IMM_MAX_PC5_PKT_LEN 8188
#define ADS_DL_PC5_FRAGS_QUE_MAX_LEN 10

/* Pc5大包分片信息共5个bit, bit0表示是否是尾分片, bit1表示是否是首分片, bit2-3表示分片序号 */
#define ADS_UL_SAVE_PC5_FRAG_INFO_TO_IMM(frag, isLast, fragSeq) \
    (ADS_IMM_MEM_CB(frag)->priv[0] = ((fragSeq) << 2 | ((fragSeq == 0) ? VOS_TRUE : VOS_FALSE) << 1 | (isLast)))

#define ADS_GET_PC5_FRAG_INFO(frag) (ADS_IMM_MEM_CB(frag)->priv[0])
#define ADS_GET_PC5_FRAG_SEQ(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) >> 2) & 0x07)
#define ADS_PC5_IS_FIRST_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x2)
#define ADS_PC5_IS_MID_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x0)
#define ADS_PC5_IS_LAST_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x1)
#define ADS_PC5_IS_NORM_PKT(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x3)
#endif

/* 检查MODEMID有效性 */
#define ADS_IS_MODEMID_VALID(mid) ((mid) < ADS_INSTANCE_MAX_NUM)

/* 检查RABIID有效性 */
#define ADS_IS_RABID_VALID(rabid) (((rabid) >= ADS_RAB_ID_MIN) && ((rabid) <= ADS_RAB_ID_MAX))

#define ADS_UL_IS_REACH_THRESHOLD(allUlQueueDataNum, ucSendingFlg) \
    ((allUlQueueDataNum != 0) && (allUlQueueDataNum >= ADS_UL_SEND_DATA_NUM_THREDHOLD) && (ucSendingFlg == VOS_FALSE))

#if !defined(ADS_ARRAY_SIZE)
#define ADS_ARRAY_SIZE(a) (sizeof((a)) / sizeof((a[0])))
#endif

#if (defined(LLT_OS_VER))
typedef unsigned long SPINLOCK_STRU;
#else
typedef VOS_SPINLOCK SPINLOCK_STRU;
#endif

/*
 * 枚举说明: IPF 内存队列
 */
enum ADS_IPF_MemPoolId {
    ADS_IPF_MEM_POOL_ID_AD0 = 0,
    ADS_IPF_MEM_POOL_ID_AD1,

    ADS_IPF_MEM_POOL_ID_BUTT
};
typedef VOS_UINT32 ADS_IPF_MemPoolIdUint32;


enum ADS_IPF_Mode {
    ADS_IPF_MODE_INT  = 0x00, /* 中断上下文 */
    ADS_IPF_MODE_THRD = 0x01, /* 线程上下文 */
    ADS_IPF_MODE_BUTT
};
typedef VOS_UINT8 ADS_IPF_ModeUint8;

/*
 * 结构说明: IPF CB结构
 */
typedef struct {
    dma_addr_t dmaAddr;

} ADS_SpeMemCb;

/*
 * 结构说明: IMM CB结构
 */
typedef struct {
    VOS_UINT32 priv[2];
    dma_addr_t dmaAddr;

} ADS_ImmMemCb;

/*
 * 结构说明: IPF BD Buffer结构
 */
typedef struct {
    IMM_Zc *pkt;

} ADS_IPF_BdBuff;

/*
 * 结构说明: IPF RD Buffer结构
 */
typedef struct {
    VOS_UINT32 slice;
    VOS_UINT16 attr;
    VOS_UINT16 pktLen;
    IMM_Zc    *immZc;
    VOS_VOID  *outPtr;

} ADS_IPF_RdBuff;

/*
 * 结构说明: IPF RD 描述符记录结构
 */
typedef struct {
    VOS_UINT32     pos;
    VOS_UINT8      rsv[4];
    ADS_IPF_RdBuff rdBuff[ADS_IPF_RD_BUFF_RECORD_NUM];

} ADS_IPF_RdRecord;

/*
 * 结构说明: IPF AD Buffer结构
 */
typedef struct {
    VOS_UINT32 slice;
#ifndef CONFIG_NEW_PLATFORM
    VOS_UINT32 ulPhyAddr;
#else
    VOS_UINT8 rsv[4];
    VOS_VOID *phyAddr;
#endif
    IMM_Zc *immZc;
} ADS_IPF_AdBuff;

/*
 * 结构说明: IPF AD 描述符记录结构
 */
typedef struct {
    VOS_UINT32     pos;
    VOS_UINT8      rsv[4];
    ADS_IPF_AdBuff adBuff[ADS_IPF_AD_BUFF_RECORD_NUM];
} ADS_IPF_AdRecord;


typedef struct {
    IMM_ZcHeader           *adsUlLink;           /* Rab Id对应的队列 */
    SPINLOCK_STRU           spinLock;            /* 队列锁 */
    VOS_UINT8               isQueueValid;        /* 队列是否激活，VOS_TRUE:激活，VOS_FALSE:未激活 */
    ADS_QciTypeUint8        prio;                /* 调度优先级 */
    VOS_UINT16              recordNum;           /* 记录在一个加权数范围内发送数据的个数 */
    ADS_CDS_IpfPktTypeUint8 pktType;             /* 数据包类型 */
    VOS_UINT8               uc1XorHrpdUlIpfFlag; /* 1X OR HRPD模式下的IPF过滤组 */
    VOS_UINT8               rsv[2];

} ADS_UL_Queue;

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)

typedef struct {
    IMM_ZcHeader ulQueue;       /* LTE-V Pc5的上行队列 */
    VOS_UINT32   ulQueueMaxLen; /* LTE-V Pc5上行队列限长 */
    VOS_UINT32   reserved;
    IMM_ZcHeader fragQue;       /* LTE-V Pc5下行大包分片队列 */
    VOS_UINT32   fragQueMaxLen; /* LTE-V Pc5下行大包分片队列限长 */
    VOS_UINT32   fragsLen;      /* LTE-V Pc5下行大包分片报文当前长度和 */
    VOS_UINT8    nextFragSeq;   /* LTE-V Pc5下行大包下一个分片报文序号 */
    VOS_UINT8    rsv[3];
} ADS_PC5_Ctx;
#endif


typedef struct {
    VOS_UINT32              rabId;               /* Rab Id */
    VOS_UINT32              exParam;             /* RMNET数据接收扩展参数 */
    ADS_CDS_IpfPktTypeUint8 pktType;             /* 数据包类型 */
    VOS_UINT8               rsv[7];              /* 保留 */
    RCV_DL_DATA_FUNC        rcvDlDataFunc;       /* 对应的下行接收函数 */
    RCV_DL_DATA_FUNC        rcvDlFilterDataFunc; /* 对应的下行过滤接收函数 */
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    RCV_RD_LAST_DATA_FUNC rcvRdLstDataFunc; /* 下行RD最后一个报文的处理函数 */
#endif
    IMM_Zc *lstPkt;

} ADS_DL_RabInfo;


typedef struct {
    VOS_UINT32    enableMask;
    VOS_UINT32    fcActiveFlg;
    unsigned long tmrCnt;
    VOS_UINT32    rdCnt;
    VOS_UINT32    rateUpLev;
    VOS_UINT32    rateDownLev;
    VOS_UINT32    expireTmrLen;
} ADS_DL_FcAssem;


typedef struct {
    ADS_DL_RabInfo adsDlRabInfo[ADS_RAB_NUM_MAX]; /* 下行Rab信息 */
    ADS_DL_FcAssem fcAssemInfo;

} ADS_DL_Ctx;


typedef struct {
    ADS_UL_Queue               adsUlQueue[ADS_RAB_ID_MAX + 1]; /* 上行队列管理，只用5-15 */
    ADS_UL_QueueSchedulerPriNv queuePriNv;                     /* 从NV中读取的上行队列优先级对应的加权数 */
    VOS_UINT32                 prioIndex[ADS_RAB_NUM_MAX];     /* 存储已排好优先级的上行队列的索引 */
    VOS_UINT32                 adsUlCurIndex;                  /* 记录当前调度的队列 */
    VOS_UINT32                 ulMaxQueueLength;               /* 上行队列限长 */
    VOS_UINT32                 reserved;
} ADS_UL_Ctx;


typedef struct ADS_UL_DATA_STATS {
    VOS_UINT32 ulCurDataRate;    /* 当前上行速率，保存PDP激活后2秒的速率，去激活后清空 */
    VOS_UINT32 ulPeriodSndBytes; /* 一个流量统计周期内发送的byte数 */
} ADS_UL_DataStats;


typedef struct ADS_DL_DATA_STATS {
    VOS_UINT32 dlCurDataRate;    /* 当前下行速率，保存PDP激活后2秒的速率，去激活后清空 */
    VOS_UINT32 dlPeriodRcvBytes; /* 一个流量统计周期内收到的byte数 */
} ADS_DL_DataStats;


typedef struct {
    ADS_UL_DataStats ulDataStats; /* ADS上行数据统计 */
    ADS_DL_DataStats dlDataStats; /* ADS下行数据统计 */
} ADS_StatsInfoCtx;


typedef struct {
    VOS_UINT32 statTmrLen; /* 统计定时器长度 */
    VOS_UINT32 statPktNum; /* 在单位时间内上行统计包的个数 */
} ADS_UL_ThresholdStat;


typedef struct {
    VOS_UINT32            activeFlag;        /* 使能标识: 0表示去使能,1表示使能 */
    VOS_UINT32            protectTmrExpCnt;  /* 保护定时器超时时长 */
    unsigned long         protectTmrCnt;     /* 保护定时器的计数 */
    ADS_UL_WaterMarkLevel waterMarkLevel;    /* 动态水线值 */
    ADS_UL_ThresholdLevel thresholdLevel;    /* 动态赞包门限值 */
    ADS_UL_ThresholdStat  thresholdStatInfo; /* 赞包状态统计 */
} ADS_UL_DynamicAssemInfo;


typedef struct {
    VOS_UINT16 blkNum;
    VOS_UINT16 blkSize;
    VOS_UINT8  port;
    VOS_UINT8  reserved[3];

} ADS_IPF_MemCfg;


typedef struct {
    VOS_UINT32     enable;
    VOS_UINT8      reserved[4];
    ADS_IPF_MemCfg memCfg[ADS_IPF_MEM_POOL_ID_BUTT];

} ADS_IPF_MemPoolCfg;


typedef struct {
    VOS_UINT16   blkSize;
    VOS_UINT16   blkNum;
    VOS_UINT8    port;
    VOS_UINT8    reserved[3];
    IMM_ZcHeader que;

} ADS_IPF_MemPool;


typedef struct {
    ipf_config_ulparam_s    ipfUlBdCfgParam[IPF_ULBD_DESC_SIZE]; /* 上行BD DESC */
    ipf_rd_desc_s           ipfDlRdDesc[IPF_DLRD_DESC_SIZE];     /* 下行RD DESC */
    ADS_IPF_RdRecord        ipfDlRdRecord;
    ipf_ad_desc_s           ipfDlAdDesc[ADS_DL_ADQ_MAX_NUM][IPF_DLAD0_DESC_SIZE]; /* 下行AD DESC */
    ADS_IPF_AdRecord        ipfDlAdRecord[ADS_DL_ADQ_MAX_NUM];
    ADS_IPF_BdBuff          ipfUlBdBuff[IPF_ULBD_DESC_SIZE]; /* 上行BD */
    IMM_ZcHeader            ulSrcMemFreeQue;                 /* IPF上行内存释放BUFF */
    ADS_UL_DynamicAssemInfo ulAssemParmInfo;                 /* 上行动态组包信息 */
    VOS_UINT32              thredHoldNum;                    /* 上行赞包门限值 */
    VOS_UINT32              protectTmrLen;
    VOS_UINT8               sendingFlg; /* 正在发送标志 */
    VOS_UINT8 ipfMode; /* IPF处理ADS下行数据的模式, 0: 中断上下文(默认)，1：线程上下文 */
    VOS_UINT8 rsv[2];

    VOS_UINT32 wakeLockEnable; /* wake lock 使能标识 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    struct wakeup_source *ulBdWakeLock; /* wake lock BD */
    struct wakeup_source *dlRdWakeLock; /* wake lock RD */

    struct wakeup_source *rxWakeLock; /* wake lock RX */
    struct wakeup_source *txWakeLock; /* wake lock TX */
#else
    struct wakeup_source ulBdWakeLock; /* wake lock BD */
    struct wakeup_source dlRdWakeLock; /* wake lock RD */

    struct wakeup_source rxWakeLock; /* wake lock RX */
    struct wakeup_source txWakeLock; /* wake lock TX */
#endif
    VOS_UINT32 ulBdWakeLockCnt; /* wake lock BD 计数 */
    VOS_UINT32 dlRdWakeLockCnt; /* wake lock BD 计数 */

    VOS_UINT32 rxWakeLockTimeout; /* wake lock RX 超时时间 */
    VOS_UINT32 txWakeLockTimeout; /* wake lock TX 超时时间 */

    VOS_UINT32 txWakeLockTmrLen; /* wake lock TX 超时配置 */
    VOS_UINT32 rxWakeLockTmrLen; /* wake lock RX 超时配置 */
#if (defined(CONFIG_BALONG_SPE))
    VOS_INT32          lSpeWPort;
    VOS_UINT8          aucReserved[4];
    ADS_IPF_MemPoolCfg stMemPoolCfg;
    ADS_IPF_MemPool    astMemPool[ADS_IPF_MEM_POOL_ID_BUTT];
#endif
    IMM_ZcHeader ipfFilterQue;

    SPINLOCK_STRU adqSpinLock; /* 下行AD描述符自旋锁 */

    VOS_UINT32 cCoreResetDelayTmrLen;
    VOS_UINT8  reserved1[4];

} ADS_IPF_Ctx;


typedef struct {
    ADS_UL_Ctx adsUlCtx; /* 上行上下文 */
    ADS_DL_Ctx adsDlCtx; /* 下行上下文 */

} ADS_SpecCtx;

/*
 * 结构说明: 承载数据错误统计结构
 */
typedef struct {
    VOS_ULONG minDetectExpires; /* 最小超时时间 */
    VOS_ULONG maxDetectExpires; /* 最大超时时间 */

    VOS_UINT32 errorPktNum; /* 错包数量 */
    VOS_UINT32 totalPktNum; /* 数据总量 */

} ADS_BearerPacketErrorStats;

/*
 * 结构说明: 承载数据错误反馈配置结构
 */
typedef struct {
    VOS_UINT32 enabled;           /* 使能标识 */
    VOS_UINT32 pktErrRateThres;   /* 反馈门限(错包率) */
    VOS_ULONG  minDetectDuration; /* 最小检测持续时间(TICK) */
    VOS_ULONG  maxDetectDuration; /* 最大检测持续时间(TICK) */

} ADS_PacketErrorFeedbackCfg;


typedef struct {
    ADS_SpecCtx adsSpecCtx[ADS_INSTANCE_MAX_NUM]; /* 每个实例专有的上下文 */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    ADS_PC5_Ctx adsPc5Ctx; /* LTE-V Pc5专有的上下文 */
#endif
    ADS_StatsInfoCtx dsFlowStatsCtx;              /* 流量统计 */
    ADS_IPF_Ctx      adsIpfCtx;                   /* 与IPF相关的上下文 */
    ADS_TIMER_Ctx    adsTiCtx[ADS_MAX_TIMER_NUM]; /* 定时器上下文 */
    VOS_UINT32       adsCurInstanceIndex;         /* 当前实例的index */
    VOS_UINT8        ulResetFlag;
    VOS_UINT8        rsv[3];
    VOS_SEM          hULResetSem; /* 二进制信号量，用于UL复位处理  */
    VOS_SEM          hDLResetSem; /* 二进制信号量，用于DL复位处理  */

    ADS_PacketErrorFeedbackCfg pktErrFeedbackCfg;
    ADS_BearerPacketErrorStats pktErrStats[ADS_INSTANCE_MAX_NUM][ADS_RAB_NUM_MAX];

    struct timer_list ulSendProtectTimer;
} ADS_Ctx;

extern VOS_UINT32     g_adsULTaskId;
extern VOS_UINT32     g_adsDlTaskId;
extern VOS_UINT32     g_adsULTaskReadyFlag;
extern VOS_UINT32     g_adsDlTaskReadyFlag;
extern ADS_Ctx        g_adsCtx;
extern struct device *g_dmaDev;

VOS_VOID       ADS_DL_ProcEvent(VOS_UINT32 event);
VOS_VOID       ADS_DL_SndEvent(VOS_UINT32 event);
ADS_DL_Ctx*    ADS_GetDlCtx(VOS_UINT32 instanceIndex);
ADS_TIMER_Ctx* ADS_GetTiCtx(VOS_VOID);
ADS_UL_Ctx*    ADS_GetUlCtx(VOS_UINT32 instanceIndex);
VOS_VOID       ADS_InitCtx(VOS_VOID);
VOS_VOID       ADS_InitDlCtx(VOS_UINT32 instance);
VOS_VOID       ADS_InitIpfCtx(VOS_VOID);
VOS_VOID       ADS_InitSpecCtx(VOS_VOID);
VOS_VOID       ADS_InitStatsInfoCtx(VOS_VOID);
VOS_VOID       ADS_InitTiCtx(VOS_VOID);
VOS_VOID       ADS_InitUlCtx(VOS_UINT32 instanceIndex);
VOS_UINT32     ADS_UL_CheckAllQueueEmpty(VOS_UINT32 instanceIndex);
VOS_VOID       ADS_UL_ClearQueue(IMM_ZcHeader *queue);
VOS_UINT32 ADS_UL_CreateQueue(VOS_UINT32 instanceIndex, VOS_UINT32 rabId, ADS_QciTypeUint8 prio,
                              ADS_CDS_IpfPktTypeUint8 pdpType, VOS_UINT8 uc1XorHrpdUlIpfFlag);
VOS_VOID   ADS_UL_DestroyQueue(VOS_UINT32 instanceIndex, VOS_UINT32 rabId);
VOS_UINT32 ADS_UL_GetAllQueueDataNum(VOS_VOID);
VOS_UINT32 ADS_UL_GetInsertIndex(VOS_UINT32 instanceIndex, VOS_UINT32 rabId);
VOS_UINT32 ADS_UL_GetInstanceAllQueueDataNum(VOS_UINT32 instanceIndex);
VOS_UINT32 ADS_UL_InsertQueue(VOS_UINT32 instance, IMM_Zc *data, VOS_UINT32 rabId);
VOS_UINT32 ADS_UL_IsQueueExistent(VOS_UINT32 instanceIndex, VOS_UINT32 rabId);
VOS_VOID   ADS_UL_OrderQueueIndex(VOS_UINT32 instanceIndex, VOS_UINT32 indexNum);
VOS_VOID   ADS_UL_ProcEvent(VOS_UINT32 event);
VOS_VOID ADS_UL_SetQueue(VOS_UINT32 instanceIndex, VOS_UINT32 rabId, VOS_UINT8 isQueueValid, IMM_ZcHeader *ulQueue,
                         ADS_QciTypeUint8 prio, ADS_CDS_IpfPktTypeUint8 pdpType, VOS_UINT8 uc1XorHrpdUlIpfFlag);
VOS_VOID ADS_UL_SndEvent(VOS_UINT32 event);
VOS_VOID ADS_UL_UpdateQueueInPdpDeactived(VOS_UINT32 instanceIndex, VOS_UINT32 rabId);
VOS_VOID ADS_UL_UpdateQueueInPdpModified(VOS_UINT32 instanceIndex, ADS_QciTypeUint8 prio, VOS_UINT32 rabId);
ADS_Ctx* ADS_GetAllCtx(VOS_VOID);

VOS_UINT32 ADS_UL_EnableRxWakeLockTimeout(VOS_UINT32 value);
VOS_UINT32 ADS_UL_WakeLockTimeout(VOS_VOID);
VOS_UINT32 ADS_UL_WakeLock(VOS_VOID);
VOS_UINT32 ADS_UL_WakeUnLock(VOS_VOID);
VOS_UINT32 ADS_DL_EnableTxWakeLockTimeout(VOS_UINT32 value);
VOS_UINT32 ADS_DL_WakeLockTimeout(VOS_VOID);
VOS_UINT32 ADS_DL_WakeLock(VOS_VOID);
VOS_UINT32 ADS_DL_WakeUnLock(VOS_VOID);

VOS_VOID ADS_DL_InitFcAssemParamInfo(VOS_VOID);
VOS_SEM  ADS_GetULResetSem(VOS_VOID);
VOS_SEM  ADS_GetDLResetSem(VOS_VOID);
VOS_VOID ADS_DL_ResetFcAssemParamInfo(VOS_VOID);
VOS_VOID ADS_ResetSpecUlCtx(VOS_UINT32 instance);
VOS_VOID ADS_ResetUlCtx(VOS_VOID);
VOS_VOID ADS_ResetSpecDlCtx(VOS_UINT32 instance);
VOS_VOID ADS_ResetDlCtx(VOS_VOID);
VOS_VOID ADS_ResetIpfCtx(VOS_VOID);

VOS_UINT32 ADS_UL_IsAnyQueueExist(VOS_VOID);

VOS_UINT8 ADS_GetUlResetFlag(VOS_VOID);
VOS_VOID  ADS_SetUlResetFlag(VOS_UINT8 flag);

VOS_UINT32 ADS_IPF_IsSpeMem(IMM_Zc *immZc);
IMM_Zc*    ADS_IPF_AllocMem(VOS_UINT32 poolId, VOS_UINT32 len, VOS_UINT32 reserveLen);
dma_addr_t ADS_IPF_GetMemDma(IMM_Zc *immZc);
dma_addr_t ADS_UL_GetMemDma(IMM_Zc *immZc);
VOS_VOID   ADS_IPF_SetMemDma(IMM_Zc *immZc, dma_addr_t dmaAddr);
VOS_VOID   ADS_IPF_MemMapRequset(IMM_Zc *immZc, VOS_UINT32 len, VOS_UINT8 isIn);
VOS_VOID   ADS_IPF_MemMapByDmaRequset(IMM_Zc *immZc, VOS_UINT32 len, VOS_UINT8 isIn);
VOS_VOID   ADS_IPF_MemUnmapRequset(IMM_Zc *immZc, VOS_UINT32 len, VOS_UINT8 isIn);

#if (defined(CONFIG_BALONG_SPE))
VOS_VOID ADS_IPF_InitMemPoolCfg(VOS_VOID);
VOS_VOID ADS_IPF_CreateMemPool(VOS_VOID);
#endif

VOS_VOID ADS_DL_ProcIpfFilterQue(VOS_VOID);
VOS_VOID ADS_DL_ProcIpfFilterData(IMM_Zc *immZc);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
VOS_INT32 ADS_PlatDevProbe(struct platform_device *dev);
VOS_INT32 ADS_PlatDevRemove(struct platform_device *dev);
VOS_INT32 __init ADS_PlatDevInit(void);
VOS_VOID __exit ADS_PlatDevExit(void);
#endif

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_VOID   ADS_InitPc5Ctx(VOS_VOID);
VOS_VOID   ADS_UL_ClearPc5Queue(VOS_VOID);
IMM_Zc*    ADS_UL_GetPc5QueueNode(VOS_VOID);
VOS_UINT32 ADS_UL_InsertPc5Queue(IMM_Zc *immZc);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
