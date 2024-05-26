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

#ifndef __ADS_MNTN_H__
#define __ADS_MNTN_H__

#include "vos.h"
#include "taf_diag_comm.h"
#include "imm_interface.h"
#include "ips_mntn.h"
#include "ads_tcp_ip_type_def.h"
#include "msp_diag.h"
#include "ads_up_link.h"
#include <linux/static_key.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)

#define ADS_MNTN_HOOK_PKT_MAX_SIZE (52)

#define ADS_MNTN_COMM_MOUDLE_ID (MDRV_DIAG_GEN_MODULE(MODEM_ID_0, DIAG_MODE_COMM))

#define ADS_MNTN_REC_UL_PKT_MAX_NUM (IPF_ULBD_DESC_SIZE)      /* ����IP���ļ�¼������ */
#define ADS_MNTN_REC_DL_PKT_MAX_NUM (IPF_DLRD_DESC_SIZE)      /* ����IP���ļ�¼������ */
#define ADS_MNTN_UL_RKT_REC_INFO_ARRAY (&(g_adsUlPktRecInfo)) /* ��¼����IP���ĵ�����   */
#define ADS_MNTN_DL_RKT_REC_INFO_ARRAY (&(g_adsDlPktRecInfo)) /* ��¼����IP���ĵ�����   */

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
#define ADS_MNTN_UL_PC5_RKT_REC_INFO_ARRAY (&(g_adsUlPc5PktRecInfo)) /* ��¼����PC5���ĵ����� */
#define ADS_MNTN_DL_PC5_RKT_REC_INFO_ARRAY (&(g_adsDlPc5PktRecInfo)) /* ��¼����PC5���ĵ����� */
#endif

/*lint -emacro({717}, ADS_MNTN_REC_UL_IP_INFO)*/
#define ADS_MNTN_REC_UL_IP_INFO(pkt, p1, p2, p3) do { \
    if (static_key_enabled(&g_adsDplNeeded)) {           \
        ADS_MntnRecULIpPktInfo((pkt), (p1), (p2), (p3)); \
    }                                                    \
} while (0)

/*lint -emacro({717}, ADS_MNTN_REC_DL_IP_INFO)*/
#define ADS_MNTN_REC_DL_IP_INFO(pkt, p1, p2, p3, p4) do { \
    if (static_key_enabled(&g_adsDplNeeded)) {                 \
        ADS_MntnRecDLIpPktInfo((pkt), (p1), (p2), (p3), (p4)); \
    }                                                          \
} while (0)

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/*lint -emacro({717}, ADS_MNTN_REC_UL_PC5_INFO)*/
#define ADS_MNTN_REC_UL_PC5_INFO(pkt) do { \
    if (static_key_enabled(&g_adsDplNeeded)) { \
        ADS_MntnRecPc5ULPktInfo((pkt));        \
    }                                          \
} while (0)

/*lint -emacro({717}, ADS_MNTN_REC_DL_PC5_INFO)*/
#define ADS_MNTN_REC_DL_PC5_INFO(pkt) do { \
    if (static_key_enabled(&g_adsDplNeeded)) { \
        ADS_MntnRecPc5DLPktInfo((pkt));        \
    }                                          \
} while (0)
#endif

/*lint -emacro({717}, ADS_MNTN_RPT_ALL_UL_PKT_INFO)*/
#define ADS_MNTN_RPT_ALL_UL_PKT_INFO() do { \
    if (static_key_enabled(&g_adsDplNeeded)) { \
        ADS_MntnReportAllULPktInfo();          \
    }                                          \
} while (0)

/*lint -emacro({717}, ADS_MNTN_RPT_ALL_DL_PKT_INFO)*/
#define ADS_MNTN_RPT_ALL_DL_PKT_INFO() do { \
    if (static_key_enabled(&g_adsDplNeeded)) { \
        ADS_MntnReportAllDLPktInfo();          \
    }                                          \
} while (0)

extern struct static_key g_adsDplNeeded;

/*
 * �ṹ˵��: ADS��ά�ɲ⹫��ͷ�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 reserved;

    VOS_UINT16 reserved0;
    VOS_UINT16 reserved1;
    VOS_UINT16 reserved2;

} ADS_MntnCommHeader;

/*
 * �ṹ˵��: ������IPFͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    ADS_MntnCommHeader commHeader;

    VOS_UINT32 ulQueNonEmptyTrigEvent;
    VOS_UINT32 ulQueFullTrigEvent;
    VOS_UINT32 ulQueHitThresTrigEvent;
    VOS_UINT32 ulTmrHitThresTrigEvent;
    VOS_UINT32 uL10MsTmrTrigEvent;
    VOS_UINT32 ulSpeIntTrigEvent;
    VOS_UINT32 ulProcEventNum;

    VOS_UINT32 ulBdqCfgIpfHaveNoBd;
    VOS_UINT32 ulBdqCfgBdSuccNum;
    VOS_UINT32 ulBdqCfgBdFailNum;
    VOS_UINT32 ulBdqCfgIpfSuccNum;
    VOS_UINT32 ulBdqCfgIpfFailNum;
    VOS_UINT32 ulBdqSaveSrcMemNum;
    VOS_UINT32 ulBdqFreeSrcMemNum;
    VOS_UINT32 ulBdqFreeSrcMemErr;

    VOS_UINT32 ulBuffThresholdCurrent;

    VOS_UINT32 ulBuffThreshold1;
    VOS_UINT32 ulBuffThreshold2;
    VOS_UINT32 ulBuffThreshold3;
    VOS_UINT32 ulBuffThreshold4;

    VOS_UINT32 ulWmLevel1HitNum;
    VOS_UINT32 ulWmLevel2HitNum;
    VOS_UINT32 ulWmLevel3HitNum;
    VOS_UINT32 ulWmLevel4HitNum;

} ADS_MntnUlIpfProcStats;

/*
 * �ṹ˵��: ����IPFͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    ADS_MntnCommHeader commHeader;

    VOS_UINT32 dlRcvIpfRdIntNum;
    VOS_UINT32 dlProcIpfRdEventNum;
    VOS_UINT32 dlRcvIpfAdqEmptyIntNum;
    VOS_UINT32 dlProcIpfAdEventNum;
    VOS_UINT32 dlcCoreResetTrigEvent;

    VOS_UINT32 dlRdqRxRdNum;
    VOS_UINT32 dlRdqGetRd0Num;
    VOS_UINT32 dlRdqTransMemFailNum;
    VOS_UINT32 dlRdqRxNormPktNum;
    VOS_UINT32 dlRdqRxNdPktNum;
    VOS_UINT32 dlRdqRxDhcpPktNum;
    VOS_UINT32 dlRdqRxErrPktNum;
    VOS_UINT32 dlRdqFilterErrNum;

    VOS_UINT32 dlAdqAllocSysMemSuccNum;
    VOS_UINT32 dlAdqAllocSysMemFailNum;
    VOS_UINT32 dlAdqAllocMemSuccNum;
    VOS_UINT32 dlAdqAllocMemFailNum;
    VOS_UINT32 dlAdqFreeMemNum;
    VOS_UINT32 dlAdqRecycleMemSuccNum;
    VOS_UINT32 dlAdqRecycleMemFailNum;
    VOS_UINT32 dlAdqGetFreeAdSuccNum;
    VOS_UINT32 dlAdqGetFreeAdFailNum;
    VOS_UINT32 dlAdqCfgAdNum;
    VOS_UINT32 dlAdqCfgAd0Num;
    VOS_UINT32 dlAdqCfgAd1Num;
    VOS_UINT32 dlAdqCfgIpfSuccNum;
    VOS_UINT32 dlAdqCfgIpfFailNum;
    VOS_UINT32 dlAdqStartEmptyTmrNum;
    VOS_UINT32 dlAdqEmptyTmrTimeoutNum;
    VOS_UINT32 dlAdqRcvAd0EmptyIntNum;
    VOS_UINT32 dlAdqRcvAd1EmptyIntNum;

} ADS_MntnDlIpfProcStats;

/*
 * �ṹ˵��: ��������ͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    ADS_MntnCommHeader commHeader;

    VOS_UINT32 ulRmnetRxPktNum;
    VOS_UINT32 ulRmnetModemIdErrNum;
    VOS_UINT32 ulRmnetRabIdErrNum;
    VOS_UINT32 ulRmnetEnQueSuccNum;
    VOS_UINT32 ulRmnetEnQueFailNum;
    VOS_UINT32 ulPktEnQueSuccNum;
    VOS_UINT32 ulPktEnQueFailNum;

} ADS_MntnUlPktProcStats;

/*
 * �ṹ˵��: ��������ͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    ADS_MntnCommHeader commHeader;

    VOS_UINT32 dlRmnetTxPktNum;
    VOS_UINT32 dlRmnetNoFuncFreePktNum;
    VOS_UINT32 dlRmnetRabIdErrNum;

} ADS_MntnDlPktProcStats;

/*
 * �ṹ˵��: ����ͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    ADS_MntnCommHeader commHeader;

    VOS_UINT32 ulDataRate;
    VOS_UINT32 dlDataRate;

} ADS_MntnThroughputStats;

/*
 * �ṹ˵��: SPE��ά�ɲ⹫��ͷ�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 reserved;

    VOS_UINT16 reserved0;
    VOS_UINT16 reserved1;
    VOS_UINT16 reserved2;

} SPE_MntnCommHeader;

/*
 * �ṹ˵��: SPE�˿�ͳ����Ϣ�ϱ��ṹ
 */
typedef struct {
    SPE_MntnCommHeader commHeader;

    VOS_UINT32 tdInputRate;
    VOS_UINT32 rdOutputRate;

    VOS_UINT32 tdPtrA;
    VOS_UINT32 tdUsingNum;
    VOS_UINT32 tdFullH;
    VOS_UINT32 tdEmptyH;
    VOS_UINT32 tdFullS;

    VOS_UINT32 rdPtrA;
    VOS_UINT32 rdUsingNum;
    VOS_UINT32 rdFullH;
    VOS_UINT32 rdEmptyH;
    VOS_UINT32 rdFullS;

} SPE_MntnPortProcStats;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ����IP�ؼ���Ϣ
 */
typedef struct {
    VOS_UINT8 ipVer;    /* IP Э��汾 */
    VOS_UINT8 l4Proto;  /* Э������ICMP,TCP,UDP */
    VOS_UINT8 icmpType; /* ICMP�������� */
    VOS_UINT8 rsv[1];

    VOS_UINT16 dataLen; /* IP ���ݰ����� */
    VOS_UINT16 ip4Id;   /* IPv4 IDENTIFY�ֶ� */
    VOS_UINT16 srcPort; /* IP Դ�˿ں� */
    VOS_UINT16 dstPort; /* IP Ŀ�Ķ˿ں� */

    VOS_UINT32 l4Id;      /* ��4��IDENTIFY,ICMP IDENTIFY+SN,TCP SEQ */
    VOS_UINT32 tcpAckSeq; /* TCP ACK SEQ */
    VOS_UINT32 param1;    /* �Զ������ */
    VOS_UINT32 param2;    /* �Զ������ */
    VOS_UINT32 param3;    /* �Զ������ */
    VOS_UINT32 param4;    /* �Զ������ */
} ADS_MntnIpPktInfo;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ADS����IP���ļ�¼�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 rsv[3];

    VOS_UINT32        rptNum;
    ADS_MntnIpPktInfo ipPktRecInfo[ADS_MNTN_REC_UL_PKT_MAX_NUM];
} ADS_MntnUlIpPktRec;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ADS����IP���ļ�¼�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 rsv[3];

    VOS_UINT32        rptNum;
    ADS_MntnIpPktInfo ipPktRecInfo[ADS_MNTN_REC_DL_PKT_MAX_NUM];
} ADS_MntnDlIpPktRec;


#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ����PC5�����������ͷ��Ϣ;
 */
typedef struct {
    VOS_UINT8  srcAddr[3]; /* ԴMAC��ַ */
    VOS_UINT8  dstAddr[3]; /* Ŀ��MAC��ַ */
    VOS_UINT8  priority;   /* 8�����ȼ�������ȡֵ��Χ1~8 */
    VOS_UINT8  usrId;      /* ��������͵�USER ID */
    VOS_UINT16 usFrmNo;    /* ���տտ�֡�� */
    VOS_UINT8  subFrmNo;   /* ���տտ���֡�� */
    VOS_UINT8  resv;       /* �����ͷ��Ԥ���ֶ� */
    VOS_UINT8  protoType;  /* �����ͷ�ṹ��Protocal Type���� */
    VOS_UINT8  rev[3];     /* �ṹ��Ԥ���ֶ� */
    VOS_UINT32 fragInfo;   /* ��Ƭ��Ϣ */
} ADS_MntnDlPc5PktHead;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ��������PC5�����������ͷ��Ϣ;
 */
typedef struct {
    VOS_UINT16 aid;        /* AIDӦ�÷����̵�Ӧ�ñ�ʾ�ţ���������ͬ��Ӧ�� */
    VOS_UINT8  srcAddr[3]; /* ԴMAC��ַ */
    VOS_UINT8  dstAddr[3]; /* Ŀ��MAC��ַ */
    VOS_UINT8  priority;   /* 8�����ȼ�������ȡֵ��Χ1~8 */
    /* ���ϲ����ݰ�ΪIP���ݰ�ʱ����ֵ����ΪIP(ֵΪ0);���ϲ����ݰ�ΪDSMP���ݰ���DME���ݰ�ʱ����ֵ����ΪNon-IP(ֵΪ1) */
    VOS_UINT8 pduType;
    VOS_UINT8 resv[2];   /* �����ͷ��Ԥ���ֶ� */
    VOS_UINT8 protoType; /* �����ͷ�ṹ��Protocal Type���� */
    VOS_UINT8 resv1[3];  /* �ṹ��Ԥ���ֶ� */
    VOS_UINT32 fragInfo; /* ��Ƭ��Ϣ */
} ADS_MntnUlPc5PktHead;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ADS����PC5���ļ�¼�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 rsv[3];

    VOS_UINT32           rptNum;
    ADS_MntnUlPc5PktHead pc5UlPktRecInfo[ADS_MNTN_REC_UL_PKT_MAX_NUM];
} ADS_MntnUlPc5PktRec;

/*
 * Э����:
 * ASN.1����:
 * �ṹ˵��: ADS����PC5���ļ�¼�ṹ
 */
typedef struct {
    VOS_UINT8 ver;
    VOS_UINT8 rsv[3];

    VOS_UINT32           rptNum;
    ADS_MntnDlPc5PktHead pc5DlPktRecInfo[ADS_MNTN_REC_DL_PKT_MAX_NUM];
} ADS_MntnDlPc5PktRec;
#endif

VOS_VOID ADS_MntnReportAllStatsInfo(VOS_VOID);
#if (defined(CONFIG_BALONG_SPE))
VOS_VOID SPE_MntnReportAllStatsInfo(VOS_VOID);
#endif

VOS_VOID ADS_MntnDecodeIpPktInfo(VOS_UINT8 *ipPkt, ADS_MntnIpPktInfo *pktInfo);
VOS_VOID ADS_MntnRecULIpPktInfo(IMM_Zc *immZc, VOS_UINT32 param1, VOS_UINT32 param2, VOS_UINT32 param3);
VOS_VOID ADS_MntnRecDLIpPktInfo(IMM_Zc *immZc, VOS_UINT32 param1, VOS_UINT32 param2, VOS_UINT32 param3,
                                VOS_UINT32 param4);
VOS_VOID RNIC_MntnReportAllStatsInfo(VOS_VOID);

#pragma pack(pop)

#if (FEATURE_PC5_DATA_CHANNEL == FEATURE_ON)
VOS_VOID ADS_MntnRecPc5ULPktInfo(IMM_Zc *immZc);
VOS_VOID ADS_MntnRecPc5DLPktInfo(IMM_Zc *immZc);
#endif

VOS_VOID ADS_MntnReportAllULPktInfo(VOS_VOID);
VOS_VOID ADS_MntnReportAllDLPktInfo(VOS_VOID);
VOS_VOID ADS_MntnDiagConnStateNotifyCB(mdrv_diag_state_e state);
VOS_VOID ADS_MntnTraceCfgNotifyCB(VOS_BOOL bTraceEnable);
VOS_VOID ADS_MntnConfigDpl(VOS_BOOL bAdsDplFlg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of ads_mntn.h */
