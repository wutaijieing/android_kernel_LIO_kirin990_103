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

#include "ads_debug.h"
#include "ads_mntn.h"
#include "securec.h"


/*
 * 协议栈打印打点方式下的.C文件宏定义
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_MNTN_C


ADS_MntnUlIpPktRec g_adsUlPktRecInfo = {0};
ADS_MntnDlIpPktRec g_adsDlPktRecInfo = {0};

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
ADS_MntnUlPc5PktRec g_adsUlPc5PktRecInfo = {0};
ADS_MntnDlPc5PktRec g_adsDlPc5PktRecInfo = {0};
#endif

struct static_key g_adsDplNeeded __read_mostly = STATIC_KEY_INIT_FALSE;  //lint !e10 !e86

VOS_VOID ADS_MntnTransReport(VOS_UINT32 msgId, VOS_VOID *data, VOS_UINT32 len)
{
    mdrv_diag_trans_ind_s diagTransInd;

    diagTransInd.ulModule  = ADS_MNTN_COMM_MOUDLE_ID;
    diagTransInd.ulPid     = ACPU_PID_ADS_UL;
    diagTransInd.ulMsgId   = msgId;
    diagTransInd.ulReserve = 0;
    diagTransInd.ulLength  = len;
    diagTransInd.pData     = data;

    (VOS_VOID)mdrv_diag_trans_report(&diagTransInd);
}

VOS_VOID ADS_MntnSndULIpfProcStatsInd(VOS_VOID)
{
    ADS_MntnUlIpfProcStats stats;

    stats.commHeader.ver      = 100;
    stats.commHeader.reserved = 0;
    stats.commHeader.reserved0 = 0;
    stats.commHeader.reserved1 = 0;
    stats.commHeader.reserved2 = 0;

    stats.ulQueNonEmptyTrigEvent = g_adsStats.ulComStatsInfo.ulQueNonEmptyTrigEvent;
    stats.ulQueFullTrigEvent     = g_adsStats.ulComStatsInfo.ulQueFullTrigEvent;
    stats.ulQueHitThresTrigEvent = g_adsStats.ulComStatsInfo.ulQueHitThresTrigEvent;
    stats.ulTmrHitThresTrigEvent = g_adsStats.ulComStatsInfo.ulTmrHitThresTrigEvent;
    stats.uL10MsTmrTrigEvent     = g_adsStats.ulComStatsInfo.uL10MsTmrTrigEvent;
    stats.ulSpeIntTrigEvent      = g_adsStats.ulComStatsInfo.ulSpeIntTrigEvent;
    stats.ulProcEventNum         = g_adsStats.ulComStatsInfo.ulProcEventNum;

    stats.ulBdqCfgIpfHaveNoBd = g_adsStats.ulComStatsInfo.ulBdqCfgIpfHaveNoBd;
    stats.ulBdqCfgBdSuccNum   = g_adsStats.ulComStatsInfo.ulBdqCfgBdSuccNum;
    stats.ulBdqCfgBdFailNum   = g_adsStats.ulComStatsInfo.ulBdqCfgBdFailNum;
    stats.ulBdqCfgIpfSuccNum  = g_adsStats.ulComStatsInfo.ulBdqCfgIpfSuccNum;
    stats.ulBdqCfgIpfFailNum  = g_adsStats.ulComStatsInfo.ulBdqCfgIpfFailNum;
    stats.ulBdqSaveSrcMemNum  = g_adsStats.ulComStatsInfo.ulBdqSaveSrcMemNum;
    stats.ulBdqFreeSrcMemNum  = g_adsStats.ulComStatsInfo.ulBdqFreeSrcMemNum;
    stats.ulBdqFreeSrcMemErr  = g_adsStats.ulComStatsInfo.ulBdqFreeSrcMemErr;

    stats.ulBuffThresholdCurrent = g_adsCtx.adsIpfCtx.thredHoldNum;

    stats.ulBuffThreshold1 = g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold1;
    stats.ulBuffThreshold2 = g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold2;
    stats.ulBuffThreshold3 = g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold3;
    stats.ulBuffThreshold4 = g_adsCtx.adsIpfCtx.ulAssemParmInfo.thresholdLevel.threshold4;

    stats.ulWmLevel1HitNum = g_adsStats.ulComStatsInfo.ulWmLevel1HitNum;
    stats.ulWmLevel2HitNum = g_adsStats.ulComStatsInfo.ulWmLevel2HitNum;
    stats.ulWmLevel3HitNum = g_adsStats.ulComStatsInfo.ulWmLevel3HitNum;
    stats.ulWmLevel4HitNum = g_adsStats.ulComStatsInfo.ulWmLevel4HitNum;

    ADS_MntnTransReport(ID_DIAG_ADS_UL_IPF_PROC_STATS_IND, (VOS_VOID *)&stats, sizeof(ADS_MntnUlIpfProcStats));
}

VOS_VOID ADS_MntnSndDLIpfProcStatsInd(VOS_VOID)
{
    ADS_MntnDlIpfProcStats stats;

    stats.commHeader.ver      = 100;
    stats.commHeader.reserved = 0;
    stats.commHeader.reserved0 = 0;
    stats.commHeader.reserved1 = 0;
    stats.commHeader.reserved2 = 0;

    stats.dlRcvIpfRdIntNum       = g_adsStats.dlComStatsInfo.dlRcvIpfRdIntNum;
    stats.dlProcIpfRdEventNum    = g_adsStats.dlComStatsInfo.dlProcIpfRdEventNum;
    stats.dlRcvIpfAdqEmptyIntNum = g_adsStats.dlComStatsInfo.dlRcvIpfAdqEmptyIntNum;
    stats.dlProcIpfAdEventNum    = g_adsStats.dlComStatsInfo.dlProcIpfAdEventNum;
    stats.dlcCoreResetTrigEvent  = g_adsStats.dlComStatsInfo.dlcCoreResetTrigEvent;

    stats.dlRdqRxRdNum         = g_adsStats.dlComStatsInfo.dlRdqRxRdNum;
    stats.dlRdqGetRd0Num       = g_adsStats.dlComStatsInfo.dlRdqGetRd0Num;
    stats.dlRdqTransMemFailNum = g_adsStats.dlComStatsInfo.dlRdqTransMemFailNum;
    stats.dlRdqRxNormPktNum    = g_adsStats.dlComStatsInfo.dlRdqRxNormPktNum;
    stats.dlRdqRxNdPktNum      = g_adsStats.dlComStatsInfo.dlRdqRxNdPktNum;
    stats.dlRdqRxDhcpPktNum    = g_adsStats.dlComStatsInfo.dlRdqRxDhcpPktNum;
    stats.dlRdqRxErrPktNum     = g_adsStats.dlComStatsInfo.dlRdqRxErrPktNum;
    stats.dlRdqFilterErrNum    = g_adsStats.dlComStatsInfo.dlRdqFilterErrNum;

    stats.dlAdqAllocSysMemSuccNum = g_adsStats.dlComStatsInfo.dlAdqAllocSysMemSuccNum;
    stats.dlAdqAllocSysMemFailNum = g_adsStats.dlComStatsInfo.dlAdqAllocSysMemFailNum;
    stats.dlAdqAllocMemSuccNum    = g_adsStats.dlComStatsInfo.dlAdqAllocMemSuccNum;
    stats.dlAdqAllocMemFailNum    = g_adsStats.dlComStatsInfo.dlAdqAllocMemFailNum;
    stats.dlAdqFreeMemNum         = g_adsStats.dlComStatsInfo.dlAdqFreeMemNum;
    stats.dlAdqRecycleMemSuccNum  = g_adsStats.dlComStatsInfo.dlAdqRecycleMemSuccNum;
    stats.dlAdqRecycleMemFailNum  = g_adsStats.dlComStatsInfo.dlAdqRecycleMemFailNum;
    stats.dlAdqGetFreeAdSuccNum   = g_adsStats.dlComStatsInfo.dlAdqGetFreeAdSuccNum;
    stats.dlAdqGetFreeAdFailNum   = g_adsStats.dlComStatsInfo.dlAdqGetFreeAdFailNum;
    stats.dlAdqCfgAdNum           = g_adsStats.dlComStatsInfo.dlAdqCfgAdNum;
    stats.dlAdqCfgAd0Num          = g_adsStats.dlComStatsInfo.dlAdqCfgAd0Num;
    stats.dlAdqCfgAd1Num          = g_adsStats.dlComStatsInfo.dlAdqCfgAd1Num;
    stats.dlAdqCfgIpfSuccNum      = g_adsStats.dlComStatsInfo.dlAdqCfgIpfSuccNum;
    stats.dlAdqCfgIpfFailNum      = g_adsStats.dlComStatsInfo.dlAdqCfgIpfFailNum;
    stats.dlAdqStartEmptyTmrNum   = g_adsStats.dlComStatsInfo.dlAdqStartEmptyTmrNum;
    stats.dlAdqEmptyTmrTimeoutNum = g_adsStats.dlComStatsInfo.dlAdqEmptyTmrTimeoutNum;
    stats.dlAdqRcvAd0EmptyIntNum  = g_adsStats.dlComStatsInfo.dlAdqRcvAd0EmptyIntNum;
    stats.dlAdqRcvAd1EmptyIntNum  = g_adsStats.dlComStatsInfo.dlAdqRcvAd1EmptyIntNum;

    ADS_MntnTransReport(ID_DIAG_ADS_DL_IPF_PROC_STATS_IND, (VOS_VOID *)&stats, sizeof(ADS_MntnDlIpfProcStats));
}

VOS_VOID ADS_MntnSndULPktProcStatsInd(VOS_VOID)
{
    ADS_MntnUlPktProcStats stats;

    stats.commHeader.ver      = 100;
    stats.commHeader.reserved = 0;
    stats.commHeader.reserved0 = 0;
    stats.commHeader.reserved1 = 0;
    stats.commHeader.reserved2 = 0;

    stats.ulRmnetRxPktNum      = g_adsStats.ulComStatsInfo.ulRmnetRxPktNum;
    stats.ulRmnetModemIdErrNum = g_adsStats.ulComStatsInfo.ulRmnetModemIdErrNum;
    stats.ulRmnetRabIdErrNum   = g_adsStats.ulComStatsInfo.ulRmnetRabIdErrNum;
    stats.ulRmnetEnQueSuccNum  = g_adsStats.ulComStatsInfo.ulRmnetEnQueSuccNum;
    stats.ulRmnetEnQueFailNum  = g_adsStats.ulComStatsInfo.ulRmnetEnQueFailNum;
    stats.ulPktEnQueSuccNum    = g_adsStats.ulComStatsInfo.ulPktEnQueSuccNum;
    stats.ulPktEnQueFailNum    = g_adsStats.ulComStatsInfo.ulPktEnQueFailNum;

    ADS_MntnTransReport(ID_DIAG_ADS_UL_PKT_PROC_STATS_IND, (VOS_VOID *)&stats, sizeof(ADS_MntnUlPktProcStats));
}

VOS_VOID ADS_MntnSndDLPktProcStatsInd(VOS_VOID)
{
    ADS_MntnDlPktProcStats stats;

    stats.commHeader.ver      = 100;
    stats.commHeader.reserved = 0;
    stats.commHeader.reserved0 = 0;
    stats.commHeader.reserved1 = 0;
    stats.commHeader.reserved2 = 0;

    stats.dlRmnetTxPktNum         = g_adsStats.dlComStatsInfo.dlRmnetTxPktNum;
    stats.dlRmnetNoFuncFreePktNum = g_adsStats.dlComStatsInfo.dlRmnetNoFuncFreePktNum;
    stats.dlRmnetRabIdErrNum      = g_adsStats.dlComStatsInfo.dlRmnetRabIdErrNum;

    ADS_MntnTransReport(ID_DIAG_ADS_DL_PKT_PROC_STATS_IND, (VOS_VOID *)&stats, sizeof(ADS_MntnDlPktProcStats));
}

VOS_VOID ADS_MntnSndThroughputStatsInd(VOS_VOID)
{
    ADS_MntnThroughputStats stats;

    stats.commHeader.ver      = 100;
    stats.commHeader.reserved = 0;
    stats.commHeader.reserved0 = 0;
    stats.commHeader.reserved1 = 0;
    stats.commHeader.reserved2 = 0;

    stats.ulDataRate = g_adsCtx.dsFlowStatsCtx.ulDataStats.ulCurDataRate << 3;
    stats.dlDataRate = g_adsCtx.dsFlowStatsCtx.dlDataStats.dlCurDataRate << 3;

    ADS_MntnTransReport(ID_DIAG_ADS_THROUGHPUT_STATS_IND, (VOS_VOID *)&stats, sizeof(ADS_MntnThroughputStats));
}

VOS_VOID ADS_MntnReportAllStatsInfo(VOS_VOID)
{
    ADS_MntnSndULIpfProcStatsInd();
    ADS_MntnSndDLIpfProcStatsInd();
    ADS_MntnSndULPktProcStatsInd();
    ADS_MntnSndDLPktProcStatsInd();
    ADS_MntnSndThroughputStatsInd();
}


STATIC VOS_VOID ADS_MntnReportULPktInfo(VOS_VOID)
{
    ADS_MntnUlIpPktRec *recStru    = VOS_NULL_PTR;
    VOS_UINT16          reportSize = 0;

    recStru = ADS_MNTN_UL_RKT_REC_INFO_ARRAY;

    /* 没有数据不上报 */
    if (recStru->rptNum == 0) {
        return;
    }

    reportSize =
        (VOS_UINT16)(sizeof(ADS_MntnIpPktInfo) * (recStru->rptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    recStru->ver = 100;

    ADS_MntnTransReport(ID_DIAG_ADS_UL_PKT_INFO_STATS_IND, (VOS_VOID *)recStru, reportSize);

    recStru->rptNum = 0;
}

STATIC VOS_VOID ADS_MntnReportDLPktInfo(VOS_VOID)
{
    ADS_MntnDlIpPktRec *recStru    = VOS_NULL_PTR;
    VOS_UINT16          reportSize = 0;

    recStru = ADS_MNTN_DL_RKT_REC_INFO_ARRAY;

    /* 没有数据不上报 */
    if (recStru->rptNum == 0) {
        return;
    }

    reportSize =
        (VOS_UINT16)(sizeof(ADS_MntnIpPktInfo) * (recStru->rptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    recStru->ver = 101;

    ADS_MntnTransReport(ID_DIAG_ADS_DL_PKT_INFO_STATS_IND, (VOS_VOID *)recStru, reportSize);

    recStru->rptNum = 0;
}

VOS_VOID ADS_MntnDecodeL4PktInfo(VOS_UINT8 *ipPkt, ADS_MntnIpPktInfo *pktInfo)
{
    ADS_TcpHdr   *tcpHdr  = VOS_NULL_PTR;
    ADS_UdpHdr   *udpHdr  = VOS_NULL_PTR;
    ADS_ICMP_Hdr *icmpHdr = VOS_NULL_PTR;

    switch (pktInfo->l4Proto) {
        case ADS_IP_PROTO_TCP:
            tcpHdr             = (ADS_TcpHdr *)ipPkt;
            pktInfo->srcPort   = VOS_NTOHS(tcpHdr->srcPort);
            pktInfo->dstPort   = VOS_NTOHS(tcpHdr->dstPort);
            pktInfo->l4Id      = VOS_NTOHL(tcpHdr->seqNum);
            pktInfo->tcpAckSeq = VOS_NTOHL(tcpHdr->ackNum);
            break;

        case ADS_IP_PROTO_UDP:
            udpHdr           = (ADS_UdpHdr *)ipPkt;
            pktInfo->srcPort = VOS_NTOHS(udpHdr->srcPort);
            pktInfo->dstPort = VOS_NTOHS(udpHdr->dstPort);
            break;

        case ADS_IPV4_PROTO_ICMP:
        case ADS_IPV6_PROTO_ICMP:
            icmpHdr           = (ADS_ICMP_Hdr *)ipPkt;
            pktInfo->icmpType = icmpHdr->type;
            if ((pktInfo->icmpType == ADS_IPV4_ICMP_ECHO_REQUEST) || (pktInfo->icmpType == ADS_IPV4_ICMP_ECHO_REPLY) ||
                (pktInfo->icmpType == ADS_IPV6_ICMP_ECHO_REQUEST) || (pktInfo->icmpType == ADS_IPV6_ICMP_ECHO_REPLY)) {
                pktInfo->l4Id      = VOS_NTOHS(icmpHdr->unIcmp.icmpEcho.identifier);
                pktInfo->tcpAckSeq = VOS_NTOHS(icmpHdr->unIcmp.icmpEcho.seqNum);
            }
            break;

        default:
            pktInfo->l4Id      = VOS_NTOHL(((VOS_UINT32 *)ipPkt)[0]);
            pktInfo->tcpAckSeq = VOS_NTOHL(((VOS_UINT32 *)ipPkt)[1]);
            break;
    }
}

VOS_VOID ADS_MntnDecodeIpv6PktInfo(VOS_UINT8 *ipPkt, ADS_MntnIpPktInfo *pktInfo)
{
    ADS_IPV6_Hdr *iPv6Hdr = VOS_NULL_PTR;

    iPv6Hdr          = (ADS_IPV6_Hdr *)ipPkt;
    pktInfo->ipVer   = ADS_IP_VERSION_V6;
    pktInfo->l4Proto = iPv6Hdr->nextHdr;
    pktInfo->ip4Id   = 0;
    pktInfo->dataLen = VOS_NTOHS(iPv6Hdr->payloadLen) + ADS_IPV6_HDR_LEN;

    ipPkt += ADS_IPV6_HDR_LEN;
    ADS_MntnDecodeL4PktInfo(ipPkt, pktInfo);
}

VOS_VOID ADS_MntnDecodeIpv4PktInfo(VOS_UINT8 *ipPkt, ADS_MntnIpPktInfo *pktInfo)
{
    ADS_IPV4_Hdr *iPv4Hdr = VOS_NULL_PTR;

    iPv4Hdr          = (ADS_IPV4_Hdr *)ipPkt;
    pktInfo->ipVer   = ADS_IP_VERSION_V4;
    pktInfo->l4Proto = iPv4Hdr->protocol;
    pktInfo->ip4Id   = VOS_NTOHS(iPv4Hdr->identification);
    pktInfo->dataLen = VOS_NTOHS(iPv4Hdr->totalLen);

    ipPkt += ADS_IPV4_HDR_LEN;
    ADS_MntnDecodeL4PktInfo(ipPkt, pktInfo);
}

VOS_VOID ADS_MntnDecodeIpPktInfo(VOS_UINT8 *ipPkt, ADS_MntnIpPktInfo *pktInfo)
{
    if (ADS_GET_IP_VERSION(ipPkt) == ADS_IP_VERSION_V4) {
        ADS_MntnDecodeIpv4PktInfo(ipPkt, pktInfo);
    } else {
        ADS_MntnDecodeIpv6PktInfo(ipPkt, pktInfo);
    }
}

VOS_VOID ADS_MntnRecULIpPktInfo(IMM_Zc *immZc, VOS_UINT32 param1, VOS_UINT32 param2, VOS_UINT32 param3)
{
    ADS_MntnUlIpPktRec *recStru = VOS_NULL_PTR;
    ADS_MntnIpPktInfo  *pktInfo = VOS_NULL_PTR;
    VOS_UINT32          dataLen;
    VOS_UINT32          cacheLen;

    recStru = ADS_MNTN_UL_RKT_REC_INFO_ARRAY;

    if (recStru->rptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM) {
        return;
    }

    pktInfo         = &(recStru->ipPktRecInfo[recStru->rptNum]);
    pktInfo->param1 = param1;
    pktInfo->param2 = param2;
    pktInfo->param3 = param3;

    dataLen  = IMM_ZcGetUsedLen(immZc);
    cacheLen = (dataLen < ADS_MNTN_HOOK_PKT_MAX_SIZE) ? (IMM_MAC_HEADER_RES_LEN + dataLen) :
                                                        (IMM_MAC_HEADER_RES_LEN + ADS_MNTN_HOOK_PKT_MAX_SIZE);

    ADS_IPF_SPE_MEM_UNMAP(immZc, cacheLen);
    ADS_MntnDecodeIpPktInfo(IMM_ZcGetDataPtr(immZc), pktInfo);
    ADS_IPF_SPE_MEM_MAP(immZc, cacheLen);

    (recStru->rptNum)++;
    if (recStru->rptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM) {
        ADS_MntnReportULPktInfo();
        recStru->rptNum = 0;
    }
}

VOS_VOID ADS_MntnRecDLIpPktInfo(IMM_Zc *immZc, VOS_UINT32 param1, VOS_UINT32 param2, VOS_UINT32 param3,
                                VOS_UINT32 param4)
{
    ADS_MntnDlIpPktRec *recStru = VOS_NULL_PTR;
    ADS_MntnIpPktInfo  *pktInfo = VOS_NULL_PTR;
    VOS_UINT32          dataLen;
    VOS_UINT32          cacheLen;

    recStru = ADS_MNTN_DL_RKT_REC_INFO_ARRAY;

    if (recStru->rptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM) {
        return;
    }

    pktInfo         = &(recStru->ipPktRecInfo[recStru->rptNum]);
    pktInfo->param1 = param1;
    pktInfo->param2 = param2;
    pktInfo->param3 = param3;
    pktInfo->param4 = param4;

    dataLen  = IMM_ZcGetUsedLen(immZc);
    cacheLen = (dataLen < ADS_MNTN_HOOK_PKT_MAX_SIZE) ? (IMM_MAC_HEADER_RES_LEN + dataLen) :
                                                        (IMM_MAC_HEADER_RES_LEN + ADS_MNTN_HOOK_PKT_MAX_SIZE);

    ADS_IPF_SPE_MEM_UNMAP(immZc, cacheLen);
    ADS_MntnDecodeIpPktInfo(IMM_ZcGetDataPtr(immZc), pktInfo);
    ADS_IPF_SPE_MEM_MAP(immZc, cacheLen);

    (recStru->rptNum)++;
    if (recStru->rptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM) {
        ADS_MntnReportDLPktInfo();
        recStru->rptNum = 0;
    }
}

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)

STATIC VOS_VOID ADS_MntnReportPc5ULPktInfo(VOS_VOID)
{
    ADS_MntnUlPc5PktRec *recStru    = VOS_NULL_PTR;
    VOS_UINT16           reportSize = 0;

    recStru = ADS_MNTN_UL_PC5_RKT_REC_INFO_ARRAY;

    /* 没有数据不上报 */
    if (recStru->rptNum == 0) {
        return;
    }

    reportSize =
        (VOS_UINT16)(sizeof(ADS_MntnUlPc5PktHead) * (recStru->rptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    recStru->ver = 100;

    ADS_MntnTransReport(ID_DIAG_ADS_UL_PC5_INFO_STATS_IND, (VOS_VOID *)recStru, reportSize);

    recStru->rptNum = 0;
}

STATIC VOS_VOID ADS_MntnReportPc5DLPktInfo(VOS_VOID)
{
    ADS_MntnDlPc5PktRec *recStru    = VOS_NULL_PTR;
    VOS_UINT16           reportSize = 0;

    recStru = ADS_MNTN_DL_PC5_RKT_REC_INFO_ARRAY;

    /* 没有数据不上报 */
    if (recStru->rptNum == 0) {
        return;
    }

    reportSize =
        (VOS_UINT16)(sizeof(ADS_MntnDlPc5PktHead) * (recStru->rptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    recStru->ver = 100;

    ADS_MntnTransReport(ID_DIAG_ADS_DL_PC5_INFO_STATS_IND, (VOS_VOID *)recStru, reportSize);

    recStru->rptNum = 0;
}

VOS_VOID ADS_MntnRecPc5ULPktInfo(IMM_Zc *immZc)
{
    ADS_MntnUlPc5PktRec  *recStru = VOS_NULL_PTR;
    ADS_MntnUlPc5PktHead *pktInfo = VOS_NULL_PTR;
    VOS_UINT32            pc5HeadLen;
    VOS_UINT32            dataLen;

    pc5HeadLen = sizeof(ADS_MntnUlPc5PktHead) - sizeof(VOS_UINT32);
    dataLen = IMM_ZcGetUsedLen(immZc);

    recStru = ADS_MNTN_UL_PC5_RKT_REC_INFO_ARRAY;

    if (recStru->rptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM) {
        return;
    }

    pktInfo = &(recStru->pc5UlPktRecInfo[recStru->rptNum]);

    if (memcpy_s(pktInfo, sizeof(ADS_MntnUlPc5PktHead), IMM_ZcGetDataPtr(immZc),
                 PS_MIN(dataLen, pc5HeadLen)) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_MntnRecPc5ULPktInfo: memcpy failed!");
    }

    pktInfo->fragInfo = ADS_IMM_MEM_CB(immZc)->priv[0];
    (recStru->rptNum)++;
    if (recStru->rptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM) {
        ADS_MntnReportPc5ULPktInfo();
        recStru->rptNum = 0;
    }
}

VOS_VOID ADS_MntnRecPc5DLPktInfo(IMM_Zc *immZc)
{
    ADS_MntnDlPc5PktRec  *recStru = VOS_NULL_PTR;
    ADS_MntnDlPc5PktHead *pktInfo = VOS_NULL_PTR;
    VOS_UINT32            pc5HeadLen;
    VOS_UINT32            dataLen;

    pc5HeadLen = sizeof(ADS_MntnDlPc5PktHead) - sizeof(VOS_UINT32);
    dataLen = IMM_ZcGetUsedLen(immZc);

    recStru = ADS_MNTN_DL_PC5_RKT_REC_INFO_ARRAY;

    if (recStru->rptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM) {
        return;
    }

    pktInfo = &(recStru->pc5DlPktRecInfo[recStru->rptNum]);

    if (memcpy_s(pktInfo, sizeof(ADS_MntnDlPc5PktHead), IMM_ZcGetDataPtr(immZc),
                 PS_MIN(dataLen, pc5HeadLen)) != EOK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_MntnRecPc5DLPktInfo: memcpy failed!");
    }

    pktInfo->fragInfo = ADS_IMM_MEM_CB(immZc)->priv[0];
    (recStru->rptNum)++;
    if (recStru->rptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM) {
        ADS_MntnReportPc5DLPktInfo();
        recStru->rptNum = 0;
    }
}
#endif

VOS_VOID ADS_MntnReportAllULPktInfo(VOS_VOID)
{
    ADS_MntnReportULPktInfo();
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    ADS_MntnReportPc5ULPktInfo();
#endif
}

VOS_VOID ADS_MntnReportAllDLPktInfo(VOS_VOID)
{
    ADS_MntnReportDLPktInfo();
#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
    ADS_MntnReportPc5DLPktInfo();
#endif
}

VOS_VOID ADS_MntnDiagConnStateNotifyCB(mdrv_diag_state_e state)
{
    if (state == DIAG_STATE_DISCONN) {
        ADS_UL_SndDplConfigInd(VOS_FALSE);
    }
}

VOS_VOID ADS_MntnTraceCfgNotifyCB(VOS_BOOL bTraceEnable)
{
    ADS_UL_SndDplConfigInd(bTraceEnable);
}

VOS_VOID ADS_MntnConfigDpl(VOS_BOOL bAdsDplFlg)
{
    if (bAdsDplFlg == VOS_TRUE) {
        ADS_TRACE_HIGH("g_adsDplNeeded enable");
        static_key_enable(&g_adsDplNeeded);
    } else {
        ADS_TRACE_HIGH("g_adsDplNeeded disable");
        static_key_disable(&g_adsDplNeeded);
    }
}

