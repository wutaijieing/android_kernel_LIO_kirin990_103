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

/* ADSʵ��������Modem��������һ�� */
#define ADS_INSTANCE_MAX_NUM (MODEM_ID_BUTT)

/* ADSʵ������ֵ */
#define ADS_INSTANCE_INDEX_0 (0)
#define ADS_INSTANCE_INDEX_1 (1)
#define ADS_INSTANCE_INDEX_2 (2)

/* ���»�����е����ֵ */
#define ADS_RAB_NUM_MAX (11)

/* ��ǰ������Ķ�����5��15, �����һ��5��ƫ���� */
#define ADS_RAB_ID_OFFSET (5)

/* Rab Id����Сֵ */
#define ADS_RAB_ID_MIN (5)

/* Rab Id�����ֵ */
#define ADS_RAB_ID_MAX (15)

/* Rab Id����Чֵ */
#define ADS_RAB_ID_INVALID (0xFF)

/* Ϊ�˱���Ƶ������IPF�жϣ���Ҫ�����ܰ����ƣ��ܰ��������� */
#define ADS_UL_SEND_DATA_NUM_THREDHOLD (g_adsCtx.adsIpfCtx.thredHoldNum)

#define ADS_UL_SET_SEND_DATA_NUM_THREDHOLD(n) (g_adsCtx.adsIpfCtx.thredHoldNum = (n))

#define ADS_UL_RX_WAKE_LOCK_TMR_LEN (g_adsCtx.adsIpfCtx.rxWakeLockTmrLen)
#define ADS_DL_TX_WAKE_LOCK_TMR_LEN (g_adsCtx.adsIpfCtx.txWakeLockTmrLen)

/* �����AD��Ҫƫ��14��ΪIPF RD��Ŀ�ĵ�ַ��14ΪMACͷ�ĳ��� */
#define ADS_DL_AD_DATA_PTR_OFFSET (14)

/* Ĭ�ϵ����ȼ���Ȩ�� */
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

/* ADS ADQ�ĸ��� */
#define ADS_DL_ADQ_MAX_NUM (2)

/* ����ADQ�ձ�����ʱ������ֵ����������AD�������ڸ�ֵʱ������������ʱ�� */
#define ADS_IPF_DLAD_START_TMR_THRESHOLD (IPF_DLAD0_DESC_SIZE - 6)

/* ADSʹ�õ�SPE�˿ں� */
#define ADS_IPF_SPE_PORT_0 (SPE_PORT_MAX + 128)
#define ADS_IPF_SPE_PORT_1 (SPE_PORT_MAX + 129)

/* AD�ڴ���д�С */
#define ADS_IPF_AD0_MEM_BLK_NUM (IPF_DLAD0_DESC_SIZE * 3)
#define ADS_IPF_AD1_MEM_BLK_NUM (IPF_DLAD1_DESC_SIZE * 4)
#define ADS_IPF_AD_MEM_RESV_BLK_NUM (IPF_DLAD1_DESC_SIZE * 2)

/* AD���ݰ��ĳ��� */
#define ADS_IPF_AD0_MEM_BLK_SIZE (448)
#define ADS_IPF_AD1_MEM_BLK_SIZE (1536 + 14)

#define ADS_IPF_RD_BUFF_RECORD_NUM (IPF_DLRD_DESC_SIZE * 2)
#define ADS_IPF_AD_BUFF_RECORD_NUM (IPF_DLAD0_DESC_SIZE * 2)

/*
 * ADS_UL_SendPacket��ADS_DL_RegDlDataCallback��rabidΪ��չ��rabid��
 * ��2bit��ΪMODEM ID����6bit��ΪRAB ID��������չ��RABID��ȡMODEM ID
 */
#define ADS_GET_MODEMID_FROM_EXRABID(i) ((i >> 6) & 0x03)

#define ADS_GET_RABID_FROM_EXRABID(i) (i & 0x3F)

#define ADS_BUILD_EXRABID(mid, rabid) ((VOS_UINT8)((((mid) << 6) & 0xC0) | ((rabid) & 0x3F)))

#define ADS_BIT16_MASK(bit) ((VOS_UINT16)(1 << (bit)))
#define ADS_BIT16_SET(val, bit) ((val) | ADS_BIT16_MASK(bit))
#define ADS_BIT16_CLR(val, bit) ((val) & ~ADS_BIT16_MASK(bit))
#define ADS_BIT16_IS_SET(val, bit) ((val) & ADS_BIT16_MASK(bit))

/* ******************************�����¼� Begin****************************** */

/* ADS���������¼� */
#define ADS_UL_EVENT_BASE (0x00000000)
#define ADS_UL_EVENT_DATA_PROC (ADS_UL_EVENT_BASE | 0x0001)

/* ADS���������¼� */
#define ADS_DL_EVENT_BASE (0x00000000)
#define ADS_DL_EVENT_IPF_RD_INT (ADS_DL_EVENT_BASE | 0x0001)
#define ADS_DL_EVENT_IPF_ADQ_EMPTY_INT (ADS_DL_EVENT_BASE | 0x0002)
#define ADS_DL_EVENT_IPF_FILTER_DATA_PROC (ADS_DL_EVENT_BASE | 0x0004)

/* ******************************�����¼� End****************************** */

/* ******************************����ͳ�� Begin****************************** */
/* ��ȡ����ͳ�������� */
#define ADS_GET_DSFLOW_STATS_CTX_PTR() (&(g_adsCtx.dsFlowStatsCtx))

/* ���õ�ǰ�������� */
#define ADS_SET_CURRENT_UL_RATE(n) (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulCurDataRate = (n))

/* ���õ�ǰ�������� */
#define ADS_SET_CURRENT_DL_RATE(n) (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlCurDataRate = (n))

/* ͳ�������������յ����ĸ��� */
#define ADS_RECV_UL_PERIOD_PKT_NUM(n) (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes += (n))

/* ͳ�������������յ����ĸ��� */
#define ADS_RECV_DL_PERIOD_PKT_NUM(n) (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes += (n))

/* ��ȡ�����������յ����ĸ��� */
#define ADS_GET_UL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes)

/* ��ȡ�����������յ����ĸ��� */
#define ADS_GET_DL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes)

/* �������������յ����ĸ������� */
#define ADS_CLEAR_UL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.ulDataStats.ulPeriodSndBytes = 0)

/* �������������յ����ĸ������� */
#define ADS_CLEAR_DL_PERIOD_PKT_NUM() (g_adsCtx.dsFlowStatsCtx.dlDataStats.dlPeriodRcvBytes = 0)

/* ******************************����ͳ�� End****************************** */

/* ******************************���� Begin****************************** */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/* ��ȡ����Pc5����ͷ��� */
#define ADS_UL_GET_PC5_QUEUE_HEAD() (&(g_adsCtx.adsPc5Ctx.ulQueue))

/* ��ȡ����Pc5������󳤶� */
#define ADS_UL_GET_PC5_MAX_QUEUE_LEN() (g_adsCtx.adsPc5Ctx.ulQueueMaxLen)

/* ��������Pc5������󳤶� */
#define ADS_UL_SET_PC5_MAX_QUEUE_LEN(n) (g_adsCtx.adsPc5Ctx.ulQueueMaxLen = (n))
#endif

/* ��ȡADS����ʵ���ַ */
#define ADS_UL_GET_CTX_PTR(i) (&(g_adsCtx.adsSpecCtx[i].adsUlCtx))

/* ��ȡ���ж���ָ�� */
#define ADS_UL_GET_QUEUE_LINK_PTR(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[j].adsUlLink)

/* ��ȡ���ж����� */
#define ADS_UL_GET_QUEUE_LINK_SPINLOCK(i, j) (&(g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[j].spinLock))

/* ��ȡADS����BD���ò�����ַ */
#define ADS_UL_GET_BD_CFG_PARA_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfUlBdCfgParam[indexNum]))

/* ��ȡADS����BD�����ַ */
#define ADS_UL_GET_BD_BUFF_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfUlBdBuff[indexNum]))

/* ��ȡADS���з��ͱ�����ʱ��ʱ�� */
#define ADS_UL_GET_PROTECT_TIMER_LEN() (g_adsCtx.adsIpfCtx.protectTmrLen)

/* ��ȡ�����Ƿ����ڷ��͵ı�־λ */
#define ADS_UL_GET_SENDING_FLAG() (g_adsCtx.adsIpfCtx.sendingFlg)

/* ���������Ƿ����ڷ��͵ı�־λ */
#define ADS_UL_SET_SENDING_FLAG(flg) (g_adsCtx.adsIpfCtx.sendingFlg = flg)

/* ��ȡ�洢���е����� */
#define ADS_UL_GET_PRIO_QUEUE_INDEX(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.prioIndex[j])

/* ��ȡ���ж��е������ȼ� */
#define ADS_UL_GET_QUEUE_QCI(i, j) (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].prio)

/* ��ȡ���ж�����Ϣ */
#define ADS_UL_GET_QUEUE_LINK_INFO(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].adsUlLink)

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
/* ��ȡIX OR HRPD������IPF�������־ */
#define ADS_UL_GET_1X_OR_HRPD_UL_IPF_FLAG(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].uc1XorHrpdUlIpfFlag)
#endif

/* ��ȡ���ж���һ����Ȩ����Χ�ڼ�¼�ķ��͸��� */
#define ADS_UL_GET_RECORD_NUM_IN_WEIGHTED(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum)

/* �������ж���һ����Ȩ����Χ�ڼ�¼�ķ��͸��� */
#define ADS_UL_SET_RECORD_NUM_IN_WEIGHTED(i, j, n) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum += n)

/* ������ж���һ����Ȩ����Χ�ڼ�¼�ķ��͸��� */
#define ADS_UL_CLR_RECORD_NUM_IN_WEIGHTED(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.adsUlQueue[ADS_UL_GET_PRIO_QUEUE_INDEX(i, j)].recordNum = 0)

/* ��ȡ���ж������ȼ��ļ�Ȩ�� */
#define ADS_UL_GET_QUEUE_PRI_WEIGHTED_NUM(i, j) \
    (g_adsCtx.adsSpecCtx[i].adsUlCtx.queuePriNv.priWeightedNum[ADS_UL_GET_QUEUE_QCI(i, j)])

/* ��ȡ���ж������ݰ������� */
#define ADS_UL_GET_QUEUE_PKT_TYPE(InstanceIndex, RabId) \
    (g_adsCtx.adsSpecCtx[InstanceIndex].adsUlCtx.adsUlQueue[RabId].pktType)

/* ��ȡADS���з��ͱ�����ʱ��ʱ�� */
#define ADS_UL_GET_STAT_TIMER_LEN() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statTmrLen)

#define ADS_UL_ADD_STAT_PKT_NUM(n) (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum += (n))

#define ADS_UL_GET_STAT_PKT_NUM() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum)

#define ADS_UL_CLR_STAT_PKT_NUM() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdStatInfo.statPktNum = 0)

/* ��ȡ�����ް����Ʊ�� */
#define ADS_UL_GET_THRESHOLD_ACTIVE_FLAG() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.activeFlag)

/* ��ȡ�����ް���jiffies��ʱ����� */
#define ADS_UL_GET_JIFFIES_TMR_CNT() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrCnt)

/* ���������ް���jiffies��ʱ����� */
#define ADS_UL_SET_JIFFIES_TMR_CNT(n) (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrCnt = (n))

/* ��ȡ�����ް���jiffies�ĳ�ʱ���� */
#define ADS_UL_GET_JIFFIES_EXP_TMR_LEN() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.ulProtectTmrExpCnt)

/* ��ȡ�������ݰ�ˮ�ߵȼ� */
#define ADS_UL_GET_WATER_LEVEL_ONE() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel1)

#define ADS_UL_GET_WATER_LEVEL_TWO() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel2)

#define ADS_UL_GET_WATER_LEVEL_THREE() (g_adsCtx.adsIpfCtx.ulAssemParmInfo.waterMarkLevel.waterLevel3)

#define ADS_UL_GET_WATER_LEVEL_FOUR() (g_adsCtx.adsIpfCtx.stUlAssemParmInfo.stWaterMarkLevel.ulWaterLevel4)

/* ��ȡ�����ް����� */
#define ADS_UL_DATA_THRESHOLD_ONE (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold1)

#define ADS_UL_DATA_THRESHOLD_TWO (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold2)

#define ADS_UL_DATA_THRESHOLD_THREE (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold3)

#define ADS_UL_DATA_THRESHOLD_FOUR (g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold4)

/* ******************************���� End****************************** */

/* ����IPF�ڴ��ͷŶ����׵�ַ */
#define ADS_UL_IPF_SRCMEM_FREE_QUE() (&(g_adsCtx.adsIpfCtx.ulSrcMemFreeQue))

/* IPF����Դ�ڴ��ͷ�QUEUE�������޳�Ϊ����IPF BD��������2�� */
#define ADS_UL_IPF_SRCMEM_FREE_QUE_SIZE (2 * IPF_ULBD_DESC_SIZE)

/* ͨ������ADS���ж��г��ȣ�����A��ϵͳ�ڴ棬�����޳���ʼ��ֵ */
#if (defined(CONFIG_BALONG_SPE))
#define ADS_UL_MAX_QUEUE_LENGTH (448)
#else
#define ADS_UL_MAX_QUEUE_LENGTH (500)
#endif

/* ��ȡ���ж����޳� */
#define ADS_UL_GET_MAX_QUEUE_LENGTH(i) (g_adsCtx.adsSpecCtx[i].adsUlCtx.ulMaxQueueLength)

/* ��ȡADS����IPF AD BUFFER��ַ */
#define ADS_DL_GET_IPF_AD_DESC_PTR(i, j) (&(g_adsCtx.adsIpfCtx.ipfDlAdDesc[i][j]))
#define ADS_DL_GET_IPF_AD_RECORD_PTR(i) (&(g_adsCtx.adsIpfCtx.ipfDlAdRecord[i]))

/* ******************************���� Begin****************************** */

/* ��ȡADS����ʵ���ַ */
#define ADS_DL_GET_CTX_PTR(i) (&(g_adsCtx.adsSpecCtx[i].adsDlCtx))

/* ��ȡADS����IPF RD BUFFER��ַ */
#define ADS_DL_GET_IPF_RD_DESC_PTR(indexNum) (&(g_adsCtx.adsIpfCtx.ipfDlRdDesc[indexNum]))
#define ADS_DL_GET_IPF_RD_RECORD_PTR() (&(g_adsCtx.adsIpfCtx.ipfDlRdRecord))

/* ��ȡADS����RAB INFO��ַ */
#define ADS_DL_GET_RAB_INFO_PTR(i, rabid) (&(ADS_DL_GET_CTX_PTR(i)->adsDlRabInfo[rabid - ADS_RAB_ID_OFFSET]))

/* ��ȡADS����RAB��Ӧ�����ݰ����� */
#define ADS_DL_GET_PKT_TYPE(instance, rabid) \
    (ADS_DL_GET_CTX_PTR(instance)->adsDlRabInfo[rabid - ADS_RAB_ID_OFFSET].pktType)

/* ��ȡADS�������ݻص�����ָ�� */
#define ADS_DL_GET_DATA_CALLBACK_FUNC(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->rcvDlDataFunc)
#define ADS_DL_GET_DATA_EXPARAM(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->exParam)

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
/* ��ȡADS����RD���һ�����ݴ���ص�����ָ�� */
#define ADS_DL_GET_RD_LST_DATA_CALLBACK_FUNC(i, j) (ADS_DL_GET_RAB_INFO_PTR(i, j)->rcvRdLstDataFunc)
#endif

/* get/set the last data buff ptr */
#define ADS_DL_GET_LST_DATA_PTR(mid, rabid) (ADS_DL_GET_RAB_INFO_PTR(mid, rabid)->lstPkt)
#define ADS_DL_SET_LST_DATA_PTR(mid, rabid, psPara) (ADS_DL_GET_RAB_INFO_PTR(mid, rabid)->lstPkt = (psPara))

#define ADS_DL_GET_FILTER_DATA_CALLBACK_FUNC(ucInstanceIndex, ucRabId) \
    (ADS_DL_GET_RAB_INFO_PTR(ucInstanceIndex, ucRabId)->pRcvDlFilterDataFunc)

/* ��ȡADS�������ز�����Ϣ��ַ */
#define ADS_DL_GET_FC_ASSEM_INFO_PTR(i) (&(ADS_DL_GET_CTX_PTR(i)->fcAssemInfo))

/* ��ȡADS��������������������ص�����ָ�� */
#define ADS_DL_GET_FC_ASSEM_TUNE_FUNC(i) (ADS_DL_GET_FC_ASSEM_INFO_PTR(i)->pFcAssemTuneFunc)

#define ADS_DL_GET_PKT_ERR_FEEDBACK_CFG_PTR() (&(g_adsCtx.pktErrFeedbackCfg))
#define ADS_DL_GET_PKT_ERR_STATS_PTR(mid, rabid) (&(g_adsCtx.pktErrStats[mid][rabid - ADS_RAB_ID_OFFSET]))

/* ��ȡADS����C�˵�����λ��ʱ��ʱ��ʱ�� */
#define ADS_DL_GET_CCORE_RESET_DELAY_TIMER_LEN() (g_adsCtx.adsIpfCtx.cCoreResetDelayTmrLen)

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/* ��ȡ����Pc5��Ƭ���Ķ���ͷ */
#define ADS_DL_GET_PC5_FRAGS_QUEUE_HEAD() (&(g_adsCtx.adsPc5Ctx.fragQue))

/* ��������Pc5��Ƭ������󳤶� */
#define ADS_DL_SET_PC5_FRAGS_QUEUE_MAX_LEN(n) (g_adsCtx.adsPc5Ctx.fragQueMaxLen = (n))

/* ��ȡ����Pc5��Ƭ������󳤶� */
#define ADS_DL_GET_PC5_FRAGS_QUEUE_MAX_LEN() (g_adsCtx.adsPc5Ctx.fragQueMaxLen)

/* ��ȡ����Pc5��Ƭ���ĵ�ǰ�����ܺ� */
#define ADS_DL_GET_PC5_FRAGS_LEN() (g_adsCtx.adsPc5Ctx.fragsLen)

/* ��������Pc5��Ƭ���ĵ�ǰ�����ܺ� */
#define ADS_DL_UPDATE_PC5_FRAGS_LEN(n) (g_adsCtx.adsPc5Ctx.fragsLen += (n))

/* �������Pc5��Ƭ���ĵ�ǰ�����ܺ� */
#define ADS_DL_CLEAR_PC5_FRAGS_LEN() (g_adsCtx.adsPc5Ctx.fragsLen = 0)

/* ������һ������Pc5��Ƭ������� */
#define ADS_DL_SET_NEXT_PC5_FRAG_SEQ(n) (g_adsCtx.adsPc5Ctx.nextFragSeq = (n))

/* ��ȡ��һ������Pc5��Ƭ������� */
#define ADS_DL_GET_NEXT_PC5_FRAG_SEQ() (g_adsCtx.adsPc5Ctx.nextFragSeq)
#endif

/* ******************************���� End****************************** */

/* ******************************IPF Begin****************************** */
/* ��ȡIPF��ص������� */
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

/* ��ȡIPFģʽ */
#define ADS_GET_IPF_MODE() (g_adsCtx.adsIpfCtx.ipfMode)

#define ADS_GET_IPF_FILTER_QUE() (&(g_adsCtx.adsIpfCtx.ipfFilterQue))

/* ******************************IPF End****************************** */

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#define IMM_MAX_PC5_PKT_LEN 8188
#define ADS_DL_PC5_FRAGS_QUE_MAX_LEN 10

/* Pc5�����Ƭ��Ϣ��5��bit, bit0��ʾ�Ƿ���β��Ƭ, bit1��ʾ�Ƿ����׷�Ƭ, bit2-3��ʾ��Ƭ��� */
#define ADS_UL_SAVE_PC5_FRAG_INFO_TO_IMM(frag, isLast, fragSeq) \
    (ADS_IMM_MEM_CB(frag)->priv[0] = ((fragSeq) << 2 | ((fragSeq == 0) ? VOS_TRUE : VOS_FALSE) << 1 | (isLast)))

#define ADS_GET_PC5_FRAG_INFO(frag) (ADS_IMM_MEM_CB(frag)->priv[0])
#define ADS_GET_PC5_FRAG_SEQ(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) >> 2) & 0x07)
#define ADS_PC5_IS_FIRST_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x2)
#define ADS_PC5_IS_MID_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x0)
#define ADS_PC5_IS_LAST_FRAG(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x1)
#define ADS_PC5_IS_NORM_PKT(frag) (((ADS_IMM_MEM_CB(frag)->priv[0]) & 0x3) == 0x3)
#endif

/* ���MODEMID��Ч�� */
#define ADS_IS_MODEMID_VALID(mid) ((mid) < ADS_INSTANCE_MAX_NUM)

/* ���RABIID��Ч�� */
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
 * ö��˵��: IPF �ڴ����
 */
enum ADS_IPF_MemPoolId {
    ADS_IPF_MEM_POOL_ID_AD0 = 0,
    ADS_IPF_MEM_POOL_ID_AD1,

    ADS_IPF_MEM_POOL_ID_BUTT
};
typedef VOS_UINT32 ADS_IPF_MemPoolIdUint32;


enum ADS_IPF_Mode {
    ADS_IPF_MODE_INT  = 0x00, /* �ж������� */
    ADS_IPF_MODE_THRD = 0x01, /* �߳������� */
    ADS_IPF_MODE_BUTT
};
typedef VOS_UINT8 ADS_IPF_ModeUint8;

/*
 * �ṹ˵��: IPF CB�ṹ
 */
typedef struct {
    dma_addr_t dmaAddr;

} ADS_SpeMemCb;

/*
 * �ṹ˵��: IMM CB�ṹ
 */
typedef struct {
    VOS_UINT32 priv[2];
    dma_addr_t dmaAddr;

} ADS_ImmMemCb;

/*
 * �ṹ˵��: IPF BD Buffer�ṹ
 */
typedef struct {
    IMM_Zc *pkt;

} ADS_IPF_BdBuff;

/*
 * �ṹ˵��: IPF RD Buffer�ṹ
 */
typedef struct {
    VOS_UINT32 slice;
    VOS_UINT16 attr;
    VOS_UINT16 pktLen;
    IMM_Zc    *immZc;
    VOS_VOID  *outPtr;

} ADS_IPF_RdBuff;

/*
 * �ṹ˵��: IPF RD ��������¼�ṹ
 */
typedef struct {
    VOS_UINT32     pos;
    VOS_UINT8      rsv[4];
    ADS_IPF_RdBuff rdBuff[ADS_IPF_RD_BUFF_RECORD_NUM];

} ADS_IPF_RdRecord;

/*
 * �ṹ˵��: IPF AD Buffer�ṹ
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
 * �ṹ˵��: IPF AD ��������¼�ṹ
 */
typedef struct {
    VOS_UINT32     pos;
    VOS_UINT8      rsv[4];
    ADS_IPF_AdBuff adBuff[ADS_IPF_AD_BUFF_RECORD_NUM];
} ADS_IPF_AdRecord;


typedef struct {
    IMM_ZcHeader           *adsUlLink;           /* Rab Id��Ӧ�Ķ��� */
    SPINLOCK_STRU           spinLock;            /* ������ */
    VOS_UINT8               isQueueValid;        /* �����Ƿ񼤻VOS_TRUE:���VOS_FALSE:δ���� */
    ADS_QciTypeUint8        prio;                /* �������ȼ� */
    VOS_UINT16              recordNum;           /* ��¼��һ����Ȩ����Χ�ڷ������ݵĸ��� */
    ADS_CDS_IpfPktTypeUint8 pktType;             /* ���ݰ����� */
    VOS_UINT8               uc1XorHrpdUlIpfFlag; /* 1X OR HRPDģʽ�µ�IPF������ */
    VOS_UINT8               rsv[2];

} ADS_UL_Queue;

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)

typedef struct {
    IMM_ZcHeader ulQueue;       /* LTE-V Pc5�����ж��� */
    VOS_UINT32   ulQueueMaxLen; /* LTE-V Pc5���ж����޳� */
    VOS_UINT32   reserved;
    IMM_ZcHeader fragQue;       /* LTE-V Pc5���д����Ƭ���� */
    VOS_UINT32   fragQueMaxLen; /* LTE-V Pc5���д����Ƭ�����޳� */
    VOS_UINT32   fragsLen;      /* LTE-V Pc5���д����Ƭ���ĵ�ǰ���Ⱥ� */
    VOS_UINT8    nextFragSeq;   /* LTE-V Pc5���д����һ����Ƭ������� */
    VOS_UINT8    rsv[3];
} ADS_PC5_Ctx;
#endif


typedef struct {
    VOS_UINT32              rabId;               /* Rab Id */
    VOS_UINT32              exParam;             /* RMNET���ݽ�����չ���� */
    ADS_CDS_IpfPktTypeUint8 pktType;             /* ���ݰ����� */
    VOS_UINT8               rsv[7];              /* ���� */
    RCV_DL_DATA_FUNC        rcvDlDataFunc;       /* ��Ӧ�����н��պ��� */
    RCV_DL_DATA_FUNC        rcvDlFilterDataFunc; /* ��Ӧ�����й��˽��պ��� */
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    RCV_RD_LAST_DATA_FUNC rcvRdLstDataFunc; /* ����RD���һ�����ĵĴ����� */
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
    ADS_DL_RabInfo adsDlRabInfo[ADS_RAB_NUM_MAX]; /* ����Rab��Ϣ */
    ADS_DL_FcAssem fcAssemInfo;

} ADS_DL_Ctx;


typedef struct {
    ADS_UL_Queue               adsUlQueue[ADS_RAB_ID_MAX + 1]; /* ���ж��й���ֻ��5-15 */
    ADS_UL_QueueSchedulerPriNv queuePriNv;                     /* ��NV�ж�ȡ�����ж������ȼ���Ӧ�ļ�Ȩ�� */
    VOS_UINT32                 prioIndex[ADS_RAB_NUM_MAX];     /* �洢���ź����ȼ������ж��е����� */
    VOS_UINT32                 adsUlCurIndex;                  /* ��¼��ǰ���ȵĶ��� */
    VOS_UINT32                 ulMaxQueueLength;               /* ���ж����޳� */
    VOS_UINT32                 reserved;
} ADS_UL_Ctx;


typedef struct ADS_UL_DATA_STATS {
    VOS_UINT32 ulCurDataRate;    /* ��ǰ�������ʣ�����PDP�����2������ʣ�ȥ�������� */
    VOS_UINT32 ulPeriodSndBytes; /* һ������ͳ�������ڷ��͵�byte�� */
} ADS_UL_DataStats;


typedef struct ADS_DL_DATA_STATS {
    VOS_UINT32 dlCurDataRate;    /* ��ǰ�������ʣ�����PDP�����2������ʣ�ȥ�������� */
    VOS_UINT32 dlPeriodRcvBytes; /* һ������ͳ���������յ���byte�� */
} ADS_DL_DataStats;


typedef struct {
    ADS_UL_DataStats ulDataStats; /* ADS��������ͳ�� */
    ADS_DL_DataStats dlDataStats; /* ADS��������ͳ�� */
} ADS_StatsInfoCtx;


typedef struct {
    VOS_UINT32 statTmrLen; /* ͳ�ƶ�ʱ������ */
    VOS_UINT32 statPktNum; /* �ڵ�λʱ��������ͳ�ư��ĸ��� */
} ADS_UL_ThresholdStat;


typedef struct {
    VOS_UINT32            activeFlag;        /* ʹ�ܱ�ʶ: 0��ʾȥʹ��,1��ʾʹ�� */
    VOS_UINT32            protectTmrExpCnt;  /* ������ʱ����ʱʱ�� */
    unsigned long         protectTmrCnt;     /* ������ʱ���ļ��� */
    ADS_UL_WaterMarkLevel waterMarkLevel;    /* ��̬ˮ��ֵ */
    ADS_UL_ThresholdLevel thresholdLevel;    /* ��̬�ް�����ֵ */
    ADS_UL_ThresholdStat  thresholdStatInfo; /* �ް�״̬ͳ�� */
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
    ipf_config_ulparam_s    ipfUlBdCfgParam[IPF_ULBD_DESC_SIZE]; /* ����BD DESC */
    ipf_rd_desc_s           ipfDlRdDesc[IPF_DLRD_DESC_SIZE];     /* ����RD DESC */
    ADS_IPF_RdRecord        ipfDlRdRecord;
    ipf_ad_desc_s           ipfDlAdDesc[ADS_DL_ADQ_MAX_NUM][IPF_DLAD0_DESC_SIZE]; /* ����AD DESC */
    ADS_IPF_AdRecord        ipfDlAdRecord[ADS_DL_ADQ_MAX_NUM];
    ADS_IPF_BdBuff          ipfUlBdBuff[IPF_ULBD_DESC_SIZE]; /* ����BD */
    IMM_ZcHeader            ulSrcMemFreeQue;                 /* IPF�����ڴ��ͷ�BUFF */
    ADS_UL_DynamicAssemInfo ulAssemParmInfo;                 /* ���ж�̬�����Ϣ */
    VOS_UINT32              thredHoldNum;                    /* �����ް�����ֵ */
    VOS_UINT32              protectTmrLen;
    VOS_UINT8               sendingFlg; /* ���ڷ��ͱ�־ */
    VOS_UINT8 ipfMode; /* IPF����ADS�������ݵ�ģʽ, 0: �ж�������(Ĭ��)��1���߳������� */
    VOS_UINT8 rsv[2];

    VOS_UINT32 wakeLockEnable; /* wake lock ʹ�ܱ�ʶ */

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
    VOS_UINT32 ulBdWakeLockCnt; /* wake lock BD ���� */
    VOS_UINT32 dlRdWakeLockCnt; /* wake lock BD ���� */

    VOS_UINT32 rxWakeLockTimeout; /* wake lock RX ��ʱʱ�� */
    VOS_UINT32 txWakeLockTimeout; /* wake lock TX ��ʱʱ�� */

    VOS_UINT32 txWakeLockTmrLen; /* wake lock TX ��ʱ���� */
    VOS_UINT32 rxWakeLockTmrLen; /* wake lock RX ��ʱ���� */
#if (defined(CONFIG_BALONG_SPE))
    VOS_INT32          lSpeWPort;
    VOS_UINT8          aucReserved[4];
    ADS_IPF_MemPoolCfg stMemPoolCfg;
    ADS_IPF_MemPool    astMemPool[ADS_IPF_MEM_POOL_ID_BUTT];
#endif
    IMM_ZcHeader ipfFilterQue;

    SPINLOCK_STRU adqSpinLock; /* ����AD������������ */

    VOS_UINT32 cCoreResetDelayTmrLen;
    VOS_UINT8  reserved1[4];

} ADS_IPF_Ctx;


typedef struct {
    ADS_UL_Ctx adsUlCtx; /* ���������� */
    ADS_DL_Ctx adsDlCtx; /* ���������� */

} ADS_SpecCtx;

/*
 * �ṹ˵��: �������ݴ���ͳ�ƽṹ
 */
typedef struct {
    VOS_ULONG minDetectExpires; /* ��С��ʱʱ�� */
    VOS_ULONG maxDetectExpires; /* ���ʱʱ�� */

    VOS_UINT32 errorPktNum; /* ������� */
    VOS_UINT32 totalPktNum; /* �������� */

} ADS_BearerPacketErrorStats;

/*
 * �ṹ˵��: �������ݴ��������ýṹ
 */
typedef struct {
    VOS_UINT32 enabled;           /* ʹ�ܱ�ʶ */
    VOS_UINT32 pktErrRateThres;   /* ��������(�����) */
    VOS_ULONG  minDetectDuration; /* ��С������ʱ��(TICK) */
    VOS_ULONG  maxDetectDuration; /* ��������ʱ��(TICK) */

} ADS_PacketErrorFeedbackCfg;


typedef struct {
    ADS_SpecCtx adsSpecCtx[ADS_INSTANCE_MAX_NUM]; /* ÿ��ʵ��ר�е������� */
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    ADS_PC5_Ctx adsPc5Ctx; /* LTE-V Pc5ר�е������� */
#endif
    ADS_StatsInfoCtx dsFlowStatsCtx;              /* ����ͳ�� */
    ADS_IPF_Ctx      adsIpfCtx;                   /* ��IPF��ص������� */
    ADS_TIMER_Ctx    adsTiCtx[ADS_MAX_TIMER_NUM]; /* ��ʱ�������� */
    VOS_UINT32       adsCurInstanceIndex;         /* ��ǰʵ����index */
    VOS_UINT8        ulResetFlag;
    VOS_UINT8        rsv[3];
    VOS_SEM          hULResetSem; /* �������ź���������UL��λ����  */
    VOS_SEM          hDLResetSem; /* �������ź���������DL��λ����  */

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
