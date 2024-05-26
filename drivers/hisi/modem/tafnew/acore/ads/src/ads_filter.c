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

#include "ads_ctx.h"
#include "ads_private.h"
#include "ads_log.h"
#include "ads_filter.h"
#include "ads_interface.h"
#include "mdrv_nvim.h"
#include "nv_stru_gucnas.h"
#include "securec.h"
/* TAF_ACORE_NV_READ���װͷ�ļ����� */
#include "taf_type_def.h"


/*
 * Э��ջ��ӡ��㷽ʽ�µ�.C�ļ��궨��
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_FILTER_C

/* ADS���˹��������� */
ADS_FilterCtx g_adsFilterCtx;

/*
 * ADS���˹���ͳ����Ϣ: �����и�Ԫ���������ͳ������������:
 * ��������        ͳ����
 *    0         ����IPv4 TCP
 *    1         ����IPv4 UDP
 *    2         ����IPv4 ECHO REQ
 *    3         ���в�֧�ֵ�IPv4����
 *    4         ����IPv6����
 *    5         ����IPv4 TCP
 *    6         ����IPv4 UDP
 *    7         ����IPv4 ECHO REPLY
 *    8         ����IPv4 ICMP�����
 *    9         ����IPv4 ��Ƭ��(��Ƭ)
 *    10        ����IPv4 ��Ƭ��(����Ƭ)
 *    11        ����IPv6��
 */
VOS_UINT32 g_adsFilterStats[ADS_FILTER_ORIG_PKT_BUTT] = {0};

/* ADS��������ICMP�������õĺ���ָ��ṹ�壬��ǰֻ֧��Echo Reply��ICMP����� */
ADS_FilterDecodeDlIcmpFunc g_adsFilterDecodeDlIcmpFuncTbl[] = {
    /* ICMP Type */ /* pFunc */ /* enOrigPktType */ /* aucReserved[4] */
    { ADS_FilterDecodeDlIPv4EchoReplyPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ECHOREPLY, { 0, 0, 0, 0 }}, /* 0:Echo Reply */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 1:Reserve */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 2:Reserve */
    /* 3:Destination Unreachable */
    { ADS_FilterDecodeDlIPv4IcmpErrorPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR, { 0, 0, 0, 0 }},
    /* 4:Source Quench */
    { ADS_FilterDecodeDlIPv4IcmpErrorPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR, { 0, 0, 0, 0 }},
    /* 5:Redirect (change route) */
    { ADS_FilterDecodeDlIPv4IcmpErrorPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR, { 0, 0, 0, 0 }},
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 6:Reserve      */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 7:Reserve      */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 8:Echo Request */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 9:Reserve      */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 10:Reserve     */
    /* 11:Time Exceeded */
    { ADS_FilterDecodeDlIPv4IcmpErrorPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR, { 0, 0, 0, 0 }},
    /* 12:Parameter Problem */
    { ADS_FilterDecodeDlIPv4IcmpErrorPacket, ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR, { 0, 0, 0, 0 }},
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 13:Timestamp Request    */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 14:Timestamp Reply      */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 15:Information Request  */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 16:Information Reply    */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}, /* 17:Address Mask Request */
    { VOS_NULL_PTR, ADS_FILTER_ORIG_PKT_BUTT, { 0, 0, 0, 0 }}  /* 18:Address Mask Reply   */
};

VOS_VOID ADS_FilterResetIPv6Addr(VOS_VOID)
{
    (VOS_VOID)memset_s(ADS_FILTER_GET_IPV6_ADDR(), sizeof(ADS_IPV6_ADDR_UN), 0x00, sizeof(ADS_IPV6_ADDR_UN));
}

VOS_VOID ADS_FilterInitListsHead(VOS_VOID)
{
    VOS_UINT32 loop;
    HI_LIST_S *listHead = VOS_NULL_PTR;

    /* ѭ����ʼ��ÿһ�����˱��ͷ�ڵ� */
    for (loop = 0; loop < ADS_FILTER_MAX_LIST_NUM; loop++) {
        listHead = ADS_FILTER_GET_LIST(loop);

        HI_INIT_LIST_HEAD(listHead);
    }
}

VOS_VOID ADS_FilterInitCtx(VOS_VOID)
{
    TAF_NVIM_SharePdpInfo sharePdp;
    VOS_UINT32            timeLen;

    memset_s(&g_adsFilterCtx, sizeof(g_adsFilterCtx), 0x00, sizeof(g_adsFilterCtx));

    /* ��ʼ�����˱�ͷ�ڵ� */
    ADS_FilterInitListsHead();

    /* ��ʼ���ϻ����� */
    /* ��ȡNV���ȡ���˽ڵ��ϻ�ʱ�� */
    (VOS_VOID)memset_s(&sharePdp, sizeof(sharePdp), 0x00, sizeof(sharePdp));

    if (TAF_ACORE_NV_READ(MODEM_ID_0, NV_ITEM_SHARE_PDP_INFO, &sharePdp, sizeof(sharePdp)) != NV_OK) {
        ADS_WARNING_LOG(ACPU_PID_ADS_DL, "ADS_FilterInitCtx: NV Read Failed!");
        sharePdp.agingTimeLen = ADS_FILTER_DEFAULT_AGING_TIMELEN;
    }
    /* ����������ϻ�ʱ��Ϊ0��������ΪĬ��ֵ */
    sharePdp.agingTimeLen = sharePdp.agingTimeLen ? sharePdp.agingTimeLen : ADS_FILTER_DEFAULT_AGING_TIMELEN;

    timeLen = ADS_FILTER_SECOND_TRANSFER_JIFFIES * sharePdp.agingTimeLen;
    ADS_FILTER_SET_AGING_TIME_LEN(timeLen);

    /* ��ʼ��IPv6���˵�ַ */
    ADS_FilterResetIPv6Addr();
}

VOS_VOID* ADS_FilterHeapAlloc(VOS_UINT32 size)
{
    VOS_VOID *ret = VOS_NULL_PTR;

    if ((size == 0) || (size > 1024)) {
        return VOS_NULL_PTR;
    }

#if (VOS_LINUX == VOS_OS_VER)
    ret = (VOS_VOID *)kmalloc(size, GFP_KERNEL);
#else
    ret = (VOS_VOID *)malloc(size);
#endif

    return ret;
}

VOS_VOID ADS_FilterHeapFree(VOS_VOID *addr)
{
    if (addr == NULL) {
        return;
    }

#if (VOS_LINUX == VOS_OS_VER)
    kfree(addr);
#else
    free(addr);
#endif
}

VOS_VOID ADS_FilterAddFilter(ADS_FilterIpv4Info *filter)
{
    ADS_FilterNode *node     = VOS_NULL_PTR;
    HI_LIST_S      *listHead = VOS_NULL_PTR;
    VOS_UINT8       indexNum;

    /* ������˱�ڵ��ڴ� */
    node = (ADS_FilterNode *)ADS_FILTER_MALLOC(sizeof(ADS_FilterNode));
    if (node == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_FilterAddFilter: Malloc Failed!");
        return;
    }

    (VOS_VOID)memset_s(node, sizeof(ADS_FilterNode), 0x00, sizeof(ADS_FilterNode));
    node->filter = *filter;

    /* ��ȡ��ʾ��Ϣ��Ӧ�Ĺ��˱������� */
    indexNum = ADS_FILTER_GET_INDEX(filter);

    /* ͨ�������Ż�ȡ��Ӧ���������ͷ��� */
    listHead = ADS_FILTER_GET_LIST(indexNum);

    /* ���ڵ����ӵ����˱������� */
    msp_list_add_tail(&node->list, listHead);
}

VOS_VOID ADS_FilterResetLists(VOS_VOID)
{
    VOS_UINT32      loop;
    HI_LIST_S      *me       = VOS_NULL_PTR;
    HI_LIST_S      *tmp      = VOS_NULL_PTR;
    HI_LIST_S      *listHead = VOS_NULL_PTR;
    ADS_FilterNode *node     = VOS_NULL_PTR;

    /* ѭ���������й��˱����ͷŹ��˱�����нڵ� */
    for (loop = 0; loop < ADS_FILTER_MAX_LIST_NUM; loop++) {
        listHead = ADS_FILTER_GET_LIST(loop);
        msp_list_for_each_safe(me, tmp, listHead)
        {
            node = msp_list_entry(me, ADS_FilterNode, list);

            /* �ӹ��˱���ɾ���ڵ� */
            msp_list_del(&node->list);

            /* �ͷŽڵ��ڴ� */
            ADS_FILTER_FREE(node);
            node = VOS_NULL_PTR;
        }
    }
}

VOS_VOID ADS_FilterReset(VOS_VOID)
{
    /* ����IPv6���˵�ַ */
    ADS_FilterResetIPv6Addr();

    /* ���ù��˱� */
    ADS_FilterResetLists();
}

VOS_UINT32 ADS_FilterIsInfoMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    VOS_UINT32 ret = VOS_FALSE;

    /* ƥ��Դ��ַ��Ŀ�ĵ�ַ��Э������ */
    if (!ADS_FilterIsIpHdrMatch(&filter1->ipHeader, &filter2->ipHeader)) {
        return ret;
    }

    /* ���ձ������ͽ���ƥ�� */
    switch (filter1->pktType) {
        case ADS_FILTER_PKT_TYPE_TCP:
            /* ����TCP���ͽ���ƥ�� */
            ret = ADS_FilterIsTcpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_UDP:
            /* ����UDP���ͽ���ƥ�� */
            ret = ADS_FilterIsUdpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_ICMP:
            /* ����ICMP���ͽ���ƥ�� */
            ret = ADS_FilterIsIcmpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_FRAGMENT:
            /* ���շ�Ƭ�����ͽ���ƥ�� */
            ret = ADS_FilterIsFragmentMatch(filter1, filter2);
            break;
        default:
            break;
    }

    return ret;
}

VOS_UINT32 ADS_FilterMatch(ADS_FilterIpv4Info *filter)
{
    ADS_FilterNode *node     = VOS_NULL_PTR;
    HI_LIST_S      *me       = VOS_NULL_PTR;
    HI_LIST_S      *listTmp  = VOS_NULL_PTR;
    HI_LIST_S      *listHead = VOS_NULL_PTR;
    VOS_UINT32      ret = VOS_FALSE;
    VOS_UINT32      indexNum;

    /* ��ȡ��ʾ��Ϣ��Ӧ�Ĺ��˱������� */
    indexNum = ADS_FILTER_GET_INDEX(filter);

    /* ͨ�������Ż�ȡ��Ӧ���������ͷ��� */
    listHead = ADS_FILTER_GET_LIST(indexNum);

    /* ѭ��������������ƥ���� */
    msp_list_for_each_safe(me, listTmp, listHead)
    {
        node = msp_list_entry(me, ADS_FilterNode, list);

        /* �жϽڵ��Ƿ���� */
        if (ADS_FILTER_IS_AGED(node->filter.tmrCnt)) {
            /* �������в���ýڵ� */
            msp_list_del(&node->list);

            /* �ͷŽڵ��ڴ� */
            ADS_FILTER_FREE(node);
            node = VOS_NULL_PTR;
            continue;
        }

        /* �жϱ��������Ƿ�ƥ�� */
        if ((filter->pktType != node->filter.pktType) || (ret == VOS_TRUE)) {
            /* �����Ͳ�ƥ����Ѿ��ҵ�ƥ���������������һ���ڵ� */
            continue;
        }

        /* ���Ĺ�����Ϣƥ�� */
        ret = ADS_FilterIsInfoMatch(filter, &node->filter);

        /* ������Ϣƥ�䣬ˢ���ϻ�ʱ�� */
        if (ret == VOS_TRUE) {
            node->filter.tmrCnt = ADS_CURRENT_TICK;
            if (node->filter.pktType == ADS_FILTER_PKT_TYPE_ICMP) {
                /* �������в���ýڵ� */
                msp_list_del(&node->list);

                /* �ͷŽڵ��ڴ� */
                ADS_FILTER_FREE(node);
                node = VOS_NULL_PTR;
            }
        }
    }

    return ret;
}

VOS_VOID ADS_FilterSaveIPAddrInfo(ADS_FilterIpAddrInfo *filterIpAddr)
{
    /* IPv6���ͣ�����Ҫ��IPv6��ַ���浽ȫ�ֹ�����Ϣ�� */
    if (filterIpAddr->opIpv6Addr == VOS_TRUE) {
        memcpy_s(ADS_FILTER_GET_IPV6_ADDR()->ipAddrUint8, ADS_IPV6_ADDR_LEN, filterIpAddr->ipv6Addr, ADS_IPV6_ADDR_LEN);
    }
}

VOS_UINT32 ADS_FilterDecodeUlPacket(IMM_Zc *data, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_IPV4_Hdr *iPv4Hdr = VOS_NULL_PTR;
    ADS_UdpHdr   *udpHdr  = VOS_NULL_PTR;
    ADS_TcpHdr   *tcpHdr  = VOS_NULL_PTR;
    ADS_ICMP_Hdr *icmpHdr = VOS_NULL_PTR;

    /* ����IP�ײ� */
    iPv4Hdr = (ADS_IPV4_Hdr *)IMM_ZcGetDataPtr(data);

    /*
     * ��Ƭ���ķ���Ƭ����ֱ�ӷ��ͣ�����Ҫ���й�����Ϣ��ȡ���������б���������(��Ƭ������Ƭ�жϷ���: OffsetΪ��0):
     *                  MF      Offset
     * �Ƿ�Ƭ��         0         0
     * ��Ƭ����Ƭ       1         0
     * ��Ƭ���м�Ƭ     1         ��0
     * ��Ƭ��βƬ       0         ��0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_OFFSET)) != 0) {
        /* ��ά�ɲ�ͳ�� */
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_FIRST_FRAGMENT, 1);
        return VOS_ERR;
    }

    /* ��ȡ��ǰϵͳʱ�� */
    iPv4Filter->tmrCnt = ADS_CURRENT_TICK;

    iPv4Filter->ipHeader.srcAddr  = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr  = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol = iPv4Hdr->protocol;

    /* �жϱ��������Ƿ�ΪTCP\UDP\ICMP��ע��:��Ƭ����ƬΪTCP��UDP��ͳһ����TCP/UDP�����ͽ��д��� */
    switch (iPv4Hdr->protocol) {
        case ADS_IPPROTO_ICMP:
            icmpHdr = (ADS_ICMP_Hdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

            /* Ŀǰ����ֻ֧��PING ECHO REQ����ICMP���ģ����յ���������ICMP���ģ��������˴���ֱ�ӷ��� */
            if (icmpHdr->type != ADS_ICMP_ECHOREQUEST) {
                /* ��������ICMP���ģ���������ά�ɲ�ͳ�� */
                ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_SUPPORT, 1);
                return VOS_ERR;
            }

            iPv4Filter->pktType                        = ADS_FILTER_PKT_TYPE_ICMP;
            iPv4Filter->unFilter.icmpFilter.identifier = icmpHdr->unIcmp.icmpEcho.identifier;
            iPv4Filter->unFilter.icmpFilter.seqNum     = icmpHdr->unIcmp.icmpEcho.seqNum;

            /* ��ά�ɲ�ͳ�� */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_ECHOREQ, 1);
            return VOS_OK;

        case ADS_IPPROTO_TCP:
            iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_TCP;
            tcpHdr                                 = (ADS_TcpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
            iPv4Filter->unFilter.tcpFilter.srcPort = tcpHdr->srcPort;
            iPv4Filter->unFilter.tcpFilter.dstPort = tcpHdr->dstPort;

            /* ��ά�ɲ�ͳ�� */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_TCP, 1);
            return VOS_OK;

        case ADS_IPPROTO_UDP:
            iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_UDP;
            udpHdr                                 = (ADS_UdpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
            iPv4Filter->unFilter.udpFilter.srcPort = udpHdr->srcPort;
            iPv4Filter->unFilter.udpFilter.dstPort = udpHdr->dstPort;

            /* ��ά�ɲ�ͳ�� */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_UDP, 1);
            return VOS_OK;

        default:
            /* ��֧�ֵ�IPv4���������� */
            /* ��ά�ɲ�ͳ�� */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_SUPPORT, 1);
            break;
    }

    return VOS_ERR;
}

VOS_VOID ADS_FilterProcUlPacket(IMM_Zc *data, ADS_PktTypeUint8 ipType)
{
    ADS_FilterIpv4Info iPv4Filter;
    VOS_UINT32         decodeRet;
    VOS_UINT32         ret;

    /* ��ʼ�� */
    (VOS_VOID)memset_s(&iPv4Filter, sizeof(iPv4Filter), 0x00, sizeof(iPv4Filter));

    /* ������IPv4�������ݰ���IPv6����ֱ�ӷ��� */
    if (ipType != ADS_PKT_TYPE_IPV4) {
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV6_PKT, 1);
        return;
    }

    /* �����������ݰ�������ȡ��Ӧ���������еĹ��˱�ʾ��Ϣ */
    decodeRet = ADS_FilterDecodeUlPacket(data, &iPv4Filter);
    if (decodeRet != VOS_OK) {
        /* ����ʧ�ܻ�֧�ֵı������Ͳ����� */
        return;
    }

    /* �ڹ��˱���ƥ���ʾ��Ϣ */
    ret = ADS_FilterMatch(&iPv4Filter);
    if (ret != VOS_TRUE) {
        /* ûƥ�䵽�ڵ㣬��IP��ʾ��Ϣ��ӵ����˱� */
        ADS_FilterAddFilter(&iPv4Filter);
    }
}

VOS_VOID ADS_FilterDecodeDlIPv4NotFirstFragPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    (VOS_VOID)memset_s(iPv4Filter, sizeof(ADS_FilterIpv4Info), 0x00, sizeof(ADS_FilterIpv4Info));

    /* ��ȡ��Ƭ��������Ϣ */
    iPv4Filter->pktType                                = ADS_FILTER_PKT_TYPE_FRAGMENT;
    iPv4Filter->tmrCnt                                 = ADS_CURRENT_TICK;
    iPv4Filter->ipHeader.srcAddr                       = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr                       = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol                      = iPv4Hdr->protocol;
    iPv4Filter->unFilter.fragmentFilter.identification = iPv4Hdr->identification;
}

VOS_UINT32 ADS_FilterDecodeDlIPv4EchoReplyPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_ICMP_Hdr *icmpHdr = VOS_NULL_PTR;

    /* ��ȡICMP�����ײ� */
    icmpHdr = (ADS_ICMP_Hdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

    iPv4Filter->pktType                        = ADS_FILTER_PKT_TYPE_ICMP;
    iPv4Filter->tmrCnt                         = ADS_CURRENT_TICK;
    iPv4Filter->ipHeader.srcAddr               = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr               = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol              = iPv4Hdr->protocol;
    iPv4Filter->unFilter.icmpFilter.identifier = icmpHdr->unIcmp.icmpEcho.identifier;
    iPv4Filter->unFilter.icmpFilter.seqNum     = icmpHdr->unIcmp.icmpEcho.seqNum;

    return VOS_OK;
}

VOS_UINT32 ADS_FilterDecodeDlIPv4IcmpErrorPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_IPV4_Hdr *icmpIPv4Hdr = VOS_NULL_PTR;
    ADS_TcpHdr   *tcpHdr      = VOS_NULL_PTR;
    ADS_UdpHdr   *udpHdr      = VOS_NULL_PTR;

    /* ��ȡICMP������������ԭʼ���ݰ���IP�ײ�ָ�� */
    icmpIPv4Hdr = (ADS_IPV4_Hdr *)(((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN + ADS_FILTER_ICMP_HDR_LEN));

    iPv4Filter->tmrCnt            = ADS_CURRENT_TICK;
    iPv4Filter->ipHeader.srcAddr  = icmpIPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr  = icmpIPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol = icmpIPv4Hdr->protocol;

    if (icmpIPv4Hdr->protocol == ADS_IPPROTO_TCP) {
        iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_TCP;
        tcpHdr                                 = (ADS_TcpHdr *)((VOS_UINT8 *)icmpIPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
        iPv4Filter->unFilter.tcpFilter.srcPort = tcpHdr->srcPort;
        iPv4Filter->unFilter.tcpFilter.dstPort = tcpHdr->dstPort;
    } else if (icmpIPv4Hdr->protocol == ADS_IPPROTO_UDP) {
        iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_UDP;
        udpHdr                                 = (ADS_UdpHdr *)((VOS_UINT8 *)icmpIPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
        iPv4Filter->unFilter.udpFilter.srcPort = udpHdr->srcPort;
        iPv4Filter->unFilter.udpFilter.dstPort = udpHdr->dstPort;
    } else {
        /* ��TCP/UDP�����˳���ֱ�ӽ���HOST���� */
        return VOS_ERR;
    }

    return VOS_OK;
}

VOS_VOID ADS_FilterDecodeDlIPv4TcpPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_TcpHdr *tcpHdr = VOS_NULL_PTR;

    /* ��ȡTCP�����ײ� */
    tcpHdr = (ADS_TcpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

    iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_TCP;
    iPv4Filter->tmrCnt                     = ADS_CURRENT_TICK;
    iPv4Filter->ipHeader.srcAddr           = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr           = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol          = iPv4Hdr->protocol;
    iPv4Filter->unFilter.tcpFilter.srcPort = tcpHdr->dstPort;
    iPv4Filter->unFilter.tcpFilter.dstPort = tcpHdr->srcPort;
}

VOS_VOID ADS_FilterDecodeDlIPv4UdpPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_UdpHdr *udpHdr = VOS_NULL_PTR;

    /* ��ȡUDP�����ײ� */
    udpHdr = (ADS_UdpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

    iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_UDP;
    iPv4Filter->tmrCnt                     = ADS_CURRENT_TICK;
    iPv4Filter->ipHeader.srcAddr           = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr           = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol          = iPv4Hdr->protocol;
    iPv4Filter->unFilter.udpFilter.srcPort = udpHdr->dstPort;
    iPv4Filter->unFilter.udpFilter.dstPort = udpHdr->srcPort;
}

VOS_UINT32 ADS_FilterDecodeDlIPv4FragPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter,
                                            ADS_FilterOrigPktUint32 *origPktType)
{
    VOS_UINT32 ret;

    /*
     * �жϷ�Ƭ���Ƿ�Ϊ��Ƭ(��Ƭ�����Ƭ����������Offset�Ƿ�Ϊ0).
     * �������б���������:
     *                  MF      Offset
     * ��Ƭ����Ƭ       1         0
     * ��Ƭ���м�Ƭ     1         ��0
     * ��Ƭ��βƬ       0         ��0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_OFFSET)) == 0) {
        /* ��Ƭ�Ĵ���(��ǰֻ֧��TCP��UDP����Э������) */
        switch (iPv4Hdr->protocol) {
            case ADS_IPPROTO_TCP:
                /* ��¼����ԭʼ���� */
                *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT;

                /* ��ȡTCP���Ĺ�����Ϣ */
                ADS_FilterDecodeDlIPv4TcpPacket(iPv4Hdr, iPv4Filter);

                ret = VOS_OK;
                break;

            case ADS_IPPROTO_UDP:
                /* ��¼����ԭʼ���� */
                *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT;

                /* ��ȡUDP���Ĺ�����Ϣ */
                ADS_FilterDecodeDlIPv4UdpPacket(iPv4Hdr, iPv4Filter);

                ret = VOS_OK;
                break;

            default:
                /* �������ͱ��ģ�������ֱ�ӽ���HOST���� */
                ret = VOS_ERR;
                break;
        }
    } else {
        /* ����Ϊ"����Ƭ",��¼����ԭʼ���� */
        *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_NOT_FIRST_FRAGMENT;

        /* ��ȡ����Ƭ�Ĺ�����Ϣ */
        ADS_FilterDecodeDlIPv4NotFirstFragPacket(iPv4Hdr, iPv4Filter);

        ret = VOS_OK;
    }

    return ret;
}

VOS_UINT32 ADS_FilterDecodeDlIPv4Packet(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter,
                                        ADS_FilterOrigPktUint32 *origPktType)
{
    ADS_ICMP_Hdr                  *icmpHdr             = VOS_NULL_PTR;
    ADS_FILTER_DECODE_DL_ICMP_FUNC decodeDlIcmpFunc    = VOS_NULL_PTR;
    ADS_FilterDecodeDlIcmpFunc    *decodeDlIcmpFuncTbl = VOS_NULL_PTR;
    VOS_UINT32                     ret                 = VOS_ERR;

    /*
     * ����"��Ƭ��".�������б���������:
     *                  MF      Offset
     * �Ƿ�Ƭ��         0         0
     * ��Ƭ����Ƭ       1         0
     * ��Ƭ���м�Ƭ     1         ��0
     * ��Ƭ��βƬ       0         ��0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_MF | ADS_IP_OFFSET)) != 0) {
        return ADS_FilterDecodeDlIPv4FragPacket(iPv4Hdr, iPv4Filter, origPktType);
    }

    /* ����TCP\UDP\ICMP(ECHO REPLY��ICMP�����) */
    switch (iPv4Hdr->protocol) {
        case ADS_IPPROTO_ICMP:
            icmpHdr = (ADS_ICMP_Hdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

            if (icmpHdr->type < ADS_FILTER_GET_DL_ICMP_FUNC_TBL_SIZE()) {
                decodeDlIcmpFuncTbl = ADS_FILTER_GET_DL_ICMP_FUNC_TBL_ADDR(icmpHdr->type);
                decodeDlIcmpFunc    = decodeDlIcmpFuncTbl->func;
                if (decodeDlIcmpFunc != VOS_NULL_PTR) {
                    /* ��¼����ԭʼ���� */
                    *origPktType = decodeDlIcmpFuncTbl->origPktType;

                    /* ��ȡICMP���Ĺ�����Ϣ */
                    ret = decodeDlIcmpFunc(iPv4Hdr, iPv4Filter);
                }
            }
            break;

        case ADS_IPPROTO_TCP:
            /* ��¼����ԭʼ���� */
            *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_TCP;

            /* ��ȡTCP���Ĺ�����Ϣ */
            ADS_FilterDecodeDlIPv4TcpPacket(iPv4Hdr, iPv4Filter);

            ret = VOS_OK;
            break;

        case ADS_IPPROTO_UDP:
            /* ��¼����ԭʼ���� */
            *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_UDP;

            /* ��ȡUDP���Ĺ�����Ϣ */
            ADS_FilterDecodeDlIPv4UdpPacket(iPv4Hdr, iPv4Filter);

            ret = VOS_OK;
            break;

        default:
            break;
    }

    return ret;
}

VOS_UINT32 ADS_FilterProcDlIPv4Packet(IMM_Zc *data)
{
    ADS_IPV4_Hdr           *iPv4Hdr = VOS_NULL_PTR;
    ADS_FilterIpv4Info      iPv4Filter;
    VOS_UINT32              decodeRet;
    VOS_UINT32              ret;
    ADS_FilterOrigPktUint32 origPktType;

    /* ��ȡIPV4�����ײ���ַ */
    iPv4Hdr = (ADS_IPV4_Hdr *)IMM_ZcGetDataPtr(data);

    /* �жϱ������Ͳ���ȡ���˱�ʾ��Ϣ */
    (VOS_VOID)memset_s(&iPv4Filter, sizeof(iPv4Filter), 0x00, sizeof(iPv4Filter));
    decodeRet = ADS_FilterDecodeDlIPv4Packet(iPv4Hdr, &iPv4Filter, &origPktType);
    if (decodeRet != VOS_OK) {
        /* ����ʧ�ܻ�֧�ֵı������Ͳ���������HOST���� */
        return VOS_ERR;
    }

    /* �ڹ��˱���ƥ���ʾ��Ϣ */
    ret = ADS_FilterMatch(&iPv4Filter);
    if (ret == VOS_TRUE) {
        /*
         * �ҵ���ƥ��ڵ㣬����Ƿ�Ƭ����Ƭ������Ҫ����Ƭ�е������Ϣ��ȡ��������ӵ����˱��У�
         * ��Ϊ��������Ƭ��Ƭ���Ĺ�������
         */
        if (origPktType == ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT) {
            ADS_FilterDecodeDlIPv4NotFirstFragPacket(iPv4Hdr, &iPv4Filter);

            /* ����Ƭ��������Ϣ��ӵ����˱��� */
            ADS_FilterAddFilter(&iPv4Filter);
        }

        ADS_FILTER_DBG_STATISTIC(origPktType, 1);

        return VOS_OK;
    }

    return VOS_ERR;
}

VOS_UINT32 ADS_FilterProcDlIPv6Packet(IMM_Zc *data)
{
    ADS_IPV6_Hdr *iPv6Hdr = VOS_NULL_PTR;
    VOS_UINT32    ret;

    /* ��ȡIPV6�ײ�ָ�� */
    iPv6Hdr = (ADS_IPV6_Hdr *)IMM_ZcGetDataPtr(data);

    /* �ж����а��е�Ŀ�ĵ�ַ�Ƿ���DEVICE�˵�ַ��ͬ */
    if (ADS_FILTER_IS_IPV6_ADDR_IDENTICAL(ADS_FILTER_GET_IPV6_ADDR(), &(iPv6Hdr->unDstAddr))) {
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_DL_IPV6_PKT, 1);

        ret = VOS_OK;
    } else {
        ret = VOS_ERR;
    }

    return ret;
}

