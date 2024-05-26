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
/* TAF_ACORE_NV_READ宏封装头文件包含 */
#include "taf_type_def.h"


/*
 * 协议栈打印打点方式下的.C文件宏定义
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_FILTER_C

/* ADS过滤功能上下文 */
ADS_FilterCtx g_adsFilterCtx;

/*
 * ADS过滤功能统计信息: 数组中各元素所代表的统计量意义如下:
 * 数组索引        统计量
 *    0         上行IPv4 TCP
 *    1         上行IPv4 UDP
 *    2         上行IPv4 ECHO REQ
 *    3         上行不支持的IPv4报文
 *    4         上行IPv6报文
 *    5         下行IPv4 TCP
 *    6         下行IPv4 UDP
 *    7         下行IPv4 ECHO REPLY
 *    8         下行IPv4 ICMP差错报文
 *    9         下行IPv4 分片包(首片)
 *    10        下行IPv4 分片包(非首片)
 *    11        下行IPv6包
 */
VOS_UINT32 g_adsFilterStats[ADS_FILTER_ORIG_PKT_BUTT] = {0};

/* ADS解析下行ICMP报文所用的函数指针结构体，当前只支持Echo Reply和ICMP差错报文 */
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

    /* 循环初始化每一个过滤表的头节点 */
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

    /* 初始化过滤表头节点 */
    ADS_FilterInitListsHead();

    /* 初始化老化周期 */
    /* 读取NV项，获取过滤节点老化时长 */
    (VOS_VOID)memset_s(&sharePdp, sizeof(sharePdp), 0x00, sizeof(sharePdp));

    if (TAF_ACORE_NV_READ(MODEM_ID_0, NV_ITEM_SHARE_PDP_INFO, &sharePdp, sizeof(sharePdp)) != NV_OK) {
        ADS_WARNING_LOG(ACPU_PID_ADS_DL, "ADS_FilterInitCtx: NV Read Failed!");
        sharePdp.agingTimeLen = ADS_FILTER_DEFAULT_AGING_TIMELEN;
    }
    /* 如果读到的老化时间为0，则设置为默认值 */
    sharePdp.agingTimeLen = sharePdp.agingTimeLen ? sharePdp.agingTimeLen : ADS_FILTER_DEFAULT_AGING_TIMELEN;

    timeLen = ADS_FILTER_SECOND_TRANSFER_JIFFIES * sharePdp.agingTimeLen;
    ADS_FILTER_SET_AGING_TIME_LEN(timeLen);

    /* 初始化IPv6过滤地址 */
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

    /* 申请过滤表节点内存 */
    node = (ADS_FilterNode *)ADS_FILTER_MALLOC(sizeof(ADS_FilterNode));
    if (node == VOS_NULL_PTR) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_FilterAddFilter: Malloc Failed!");
        return;
    }

    (VOS_VOID)memset_s(node, sizeof(ADS_FilterNode), 0x00, sizeof(ADS_FilterNode));
    node->filter = *filter;

    /* 获取标示信息对应的过滤表索引号 */
    indexNum = ADS_FILTER_GET_INDEX(filter);

    /* 通过索引号获取对应过滤链表的头结点 */
    listHead = ADS_FILTER_GET_LIST(indexNum);

    /* 将节点增加到过滤表链表中 */
    msp_list_add_tail(&node->list, listHead);
}

VOS_VOID ADS_FilterResetLists(VOS_VOID)
{
    VOS_UINT32      loop;
    HI_LIST_S      *me       = VOS_NULL_PTR;
    HI_LIST_S      *tmp      = VOS_NULL_PTR;
    HI_LIST_S      *listHead = VOS_NULL_PTR;
    ADS_FilterNode *node     = VOS_NULL_PTR;

    /* 循环遍历所有过滤表，并释放过滤表的所有节点 */
    for (loop = 0; loop < ADS_FILTER_MAX_LIST_NUM; loop++) {
        listHead = ADS_FILTER_GET_LIST(loop);
        msp_list_for_each_safe(me, tmp, listHead)
        {
            node = msp_list_entry(me, ADS_FilterNode, list);

            /* 从过滤表中删除节点 */
            msp_list_del(&node->list);

            /* 释放节点内存 */
            ADS_FILTER_FREE(node);
            node = VOS_NULL_PTR;
        }
    }
}

VOS_VOID ADS_FilterReset(VOS_VOID)
{
    /* 重置IPv6过滤地址 */
    ADS_FilterResetIPv6Addr();

    /* 重置过滤表 */
    ADS_FilterResetLists();
}

VOS_UINT32 ADS_FilterIsInfoMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    VOS_UINT32 ret = VOS_FALSE;

    /* 匹配源地址、目的地址、协议类型 */
    if (!ADS_FilterIsIpHdrMatch(&filter1->ipHeader, &filter2->ipHeader)) {
        return ret;
    }

    /* 按照报文类型进行匹配 */
    switch (filter1->pktType) {
        case ADS_FILTER_PKT_TYPE_TCP:
            /* 按照TCP类型进行匹配 */
            ret = ADS_FilterIsTcpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_UDP:
            /* 按照UDP类型进行匹配 */
            ret = ADS_FilterIsUdpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_ICMP:
            /* 按照ICMP类型进行匹配 */
            ret = ADS_FilterIsIcmpPktMatch(filter1, filter2);
            break;
        case ADS_FILTER_PKT_TYPE_FRAGMENT:
            /* 按照分片包类型进行匹配 */
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

    /* 获取标示信息对应的过滤表索引号 */
    indexNum = ADS_FILTER_GET_INDEX(filter);

    /* 通过索引号获取对应过滤链表的头结点 */
    listHead = ADS_FILTER_GET_LIST(indexNum);

    /* 循环遍历链表，查找匹配项 */
    msp_list_for_each_safe(me, listTmp, listHead)
    {
        node = msp_list_entry(me, ADS_FilterNode, list);

        /* 判断节点是否过期 */
        if (ADS_FILTER_IS_AGED(node->filter.tmrCnt)) {
            /* 从链表中拆出该节点 */
            msp_list_del(&node->list);

            /* 释放节点内存 */
            ADS_FILTER_FREE(node);
            node = VOS_NULL_PTR;
            continue;
        }

        /* 判断报文类型是否匹配 */
        if ((filter->pktType != node->filter.pktType) || (ret == VOS_TRUE)) {
            /* 若类型不匹配或已经找到匹配项则继续处理下一个节点 */
            continue;
        }

        /* 报文过滤信息匹配 */
        ret = ADS_FilterIsInfoMatch(filter, &node->filter);

        /* 过滤信息匹配，刷新老化时间 */
        if (ret == VOS_TRUE) {
            node->filter.tmrCnt = ADS_CURRENT_TICK;
            if (node->filter.pktType == ADS_FILTER_PKT_TYPE_ICMP) {
                /* 从链表中拆出该节点 */
                msp_list_del(&node->list);

                /* 释放节点内存 */
                ADS_FILTER_FREE(node);
                node = VOS_NULL_PTR;
            }
        }
    }

    return ret;
}

VOS_VOID ADS_FilterSaveIPAddrInfo(ADS_FilterIpAddrInfo *filterIpAddr)
{
    /* IPv6类型，则需要将IPv6地址保存到全局过滤信息中 */
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

    /* 解析IP首部 */
    iPv4Hdr = (ADS_IPV4_Hdr *)IMM_ZcGetDataPtr(data);

    /*
     * 分片包的非首片，则直接发送，不需要进行过滤信息提取。各类型判别条件如下(分片包非首片判断方法: Offset为非0):
     *                  MF      Offset
     * 非分片包         0         0
     * 分片包首片       1         0
     * 分片包中间片     1         非0
     * 分片包尾片       0         非0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_OFFSET)) != 0) {
        /* 可维可测统计 */
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_FIRST_FRAGMENT, 1);
        return VOS_ERR;
    }

    /* 获取当前系统时间 */
    iPv4Filter->tmrCnt = ADS_CURRENT_TICK;

    iPv4Filter->ipHeader.srcAddr  = iPv4Hdr->unSrcAddr.ipAddrUint32;
    iPv4Filter->ipHeader.dstAddr  = iPv4Hdr->unDstAddr.ipAddrUint32;
    iPv4Filter->ipHeader.protocol = iPv4Hdr->protocol;

    /* 判断报文类型是否为TCP\UDP\ICMP。注意:分片包首片为TCP或UDP，统一按照TCP/UDP包类型进行处理 */
    switch (iPv4Hdr->protocol) {
        case ADS_IPPROTO_ICMP:
            icmpHdr = (ADS_ICMP_Hdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

            /* 目前上行只支持PING ECHO REQ类型ICMP报文，若收到其他类型ICMP报文，不做过滤处理，直接发送 */
            if (icmpHdr->type != ADS_ICMP_ECHOREQUEST) {
                /* 其他类型ICMP报文，不处理，可维可测统计 */
                ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_SUPPORT, 1);
                return VOS_ERR;
            }

            iPv4Filter->pktType                        = ADS_FILTER_PKT_TYPE_ICMP;
            iPv4Filter->unFilter.icmpFilter.identifier = icmpHdr->unIcmp.icmpEcho.identifier;
            iPv4Filter->unFilter.icmpFilter.seqNum     = icmpHdr->unIcmp.icmpEcho.seqNum;

            /* 可维可测统计 */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_ECHOREQ, 1);
            return VOS_OK;

        case ADS_IPPROTO_TCP:
            iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_TCP;
            tcpHdr                                 = (ADS_TcpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
            iPv4Filter->unFilter.tcpFilter.srcPort = tcpHdr->srcPort;
            iPv4Filter->unFilter.tcpFilter.dstPort = tcpHdr->dstPort;

            /* 可维可测统计 */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_TCP, 1);
            return VOS_OK;

        case ADS_IPPROTO_UDP:
            iPv4Filter->pktType                    = ADS_FILTER_PKT_TYPE_UDP;
            udpHdr                                 = (ADS_UdpHdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);
            iPv4Filter->unFilter.udpFilter.srcPort = udpHdr->srcPort;
            iPv4Filter->unFilter.udpFilter.dstPort = udpHdr->dstPort;

            /* 可维可测统计 */
            ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV4_UDP, 1);
            return VOS_OK;

        default:
            /* 不支持的IPv4包，不处理 */
            /* 可维可测统计 */
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

    /* 初始化 */
    (VOS_VOID)memset_s(&iPv4Filter, sizeof(iPv4Filter), 0x00, sizeof(iPv4Filter));

    /* 仅处理IPv4类型数据包，IPv6包则直接发送 */
    if (ipType != ADS_PKT_TYPE_IPV4) {
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_UL_IPV6_PKT, 1);
        return;
    }

    /* 解码上行数据包，并提取对应报文类型中的过滤标示信息 */
    decodeRet = ADS_FilterDecodeUlPacket(data, &iPv4Filter);
    if (decodeRet != VOS_OK) {
        /* 解析失败或不支持的报文类型不处理 */
        return;
    }

    /* 在过滤表中匹配标示信息 */
    ret = ADS_FilterMatch(&iPv4Filter);
    if (ret != VOS_TRUE) {
        /* 没匹配到节点，则将IP标示信息添加到过滤表 */
        ADS_FilterAddFilter(&iPv4Filter);
    }
}

VOS_VOID ADS_FilterDecodeDlIPv4NotFirstFragPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    (VOS_VOID)memset_s(iPv4Filter, sizeof(ADS_FilterIpv4Info), 0x00, sizeof(ADS_FilterIpv4Info));

    /* 提取分片包过滤信息 */
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

    /* 获取ICMP报文首部 */
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

    /* 获取ICMP报文中所带的原始数据包的IP首部指针 */
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
        /* 非TCP/UDP包则退出，直接交由HOST处理 */
        return VOS_ERR;
    }

    return VOS_OK;
}

VOS_VOID ADS_FilterDecodeDlIPv4TcpPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter)
{
    ADS_TcpHdr *tcpHdr = VOS_NULL_PTR;

    /* 获取TCP报文首部 */
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

    /* 获取UDP报文首部 */
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
     * 判断分片包是否为首片(首片与非首片的区别在于Offset是否为0).
     * 各类型判别条件如下:
     *                  MF      Offset
     * 分片包首片       1         0
     * 分片包中间片     1         非0
     * 分片包尾片       0         非0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_OFFSET)) == 0) {
        /* 首片的处理(当前只支持TCP和UDP两种协议类型) */
        switch (iPv4Hdr->protocol) {
            case ADS_IPPROTO_TCP:
                /* 记录报文原始类型 */
                *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT;

                /* 提取TCP报文过滤信息 */
                ADS_FilterDecodeDlIPv4TcpPacket(iPv4Hdr, iPv4Filter);

                ret = VOS_OK;
                break;

            case ADS_IPPROTO_UDP:
                /* 记录报文原始类型 */
                *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT;

                /* 提取UDP报文过滤信息 */
                ADS_FilterDecodeDlIPv4UdpPacket(iPv4Hdr, iPv4Filter);

                ret = VOS_OK;
                break;

            default:
                /* 其他类型报文，不处理，直接交由HOST处理 */
                ret = VOS_ERR;
                break;
        }
    } else {
        /* 报文为"非首片",记录报文原始类型 */
        *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_NOT_FIRST_FRAGMENT;

        /* 提取非首片的过滤信息 */
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
     * 处理"分片包".各类型判别条件如下:
     *                  MF      Offset
     * 非分片包         0         0
     * 分片包首片       1         0
     * 分片包中间片     1         非0
     * 分片包尾片       0         非0
     */
    if ((iPv4Hdr->flagsFo & VOS_HTONS(ADS_IP_MF | ADS_IP_OFFSET)) != 0) {
        return ADS_FilterDecodeDlIPv4FragPacket(iPv4Hdr, iPv4Filter, origPktType);
    }

    /* 处理TCP\UDP\ICMP(ECHO REPLY或ICMP差错报文) */
    switch (iPv4Hdr->protocol) {
        case ADS_IPPROTO_ICMP:
            icmpHdr = (ADS_ICMP_Hdr *)((VOS_UINT8 *)iPv4Hdr + ADS_FILTER_IPV4_HDR_LEN);

            if (icmpHdr->type < ADS_FILTER_GET_DL_ICMP_FUNC_TBL_SIZE()) {
                decodeDlIcmpFuncTbl = ADS_FILTER_GET_DL_ICMP_FUNC_TBL_ADDR(icmpHdr->type);
                decodeDlIcmpFunc    = decodeDlIcmpFuncTbl->func;
                if (decodeDlIcmpFunc != VOS_NULL_PTR) {
                    /* 记录报文原始类型 */
                    *origPktType = decodeDlIcmpFuncTbl->origPktType;

                    /* 提取ICMP报文过滤信息 */
                    ret = decodeDlIcmpFunc(iPv4Hdr, iPv4Filter);
                }
            }
            break;

        case ADS_IPPROTO_TCP:
            /* 记录报文原始类型 */
            *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_TCP;

            /* 提取TCP报文过滤信息 */
            ADS_FilterDecodeDlIPv4TcpPacket(iPv4Hdr, iPv4Filter);

            ret = VOS_OK;
            break;

        case ADS_IPPROTO_UDP:
            /* 记录报文原始类型 */
            *origPktType = ADS_FILTER_ORIG_PKT_DL_IPV4_UDP;

            /* 提取UDP报文过滤信息 */
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

    /* 获取IPV4报文首部地址 */
    iPv4Hdr = (ADS_IPV4_Hdr *)IMM_ZcGetDataPtr(data);

    /* 判断报文类型并提取过滤标示信息 */
    (VOS_VOID)memset_s(&iPv4Filter, sizeof(iPv4Filter), 0x00, sizeof(iPv4Filter));
    decodeRet = ADS_FilterDecodeDlIPv4Packet(iPv4Hdr, &iPv4Filter, &origPktType);
    if (decodeRet != VOS_OK) {
        /* 解析失败或不支持的报文类型不处理，交给HOST处理 */
        return VOS_ERR;
    }

    /* 在过滤表中匹配标示信息 */
    ret = ADS_FilterMatch(&iPv4Filter);
    if (ret == VOS_TRUE) {
        /*
         * 找到了匹配节点，如果是分片包首片，则需要将首片中的相关信息提取出来并添加到过滤表中，
         * 作为后续非首片分片包的过滤条件
         */
        if (origPktType == ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT) {
            ADS_FilterDecodeDlIPv4NotFirstFragPacket(iPv4Hdr, &iPv4Filter);

            /* 将分片包过滤信息添加到过滤表中 */
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

    /* 获取IPV6首部指针 */
    iPv6Hdr = (ADS_IPV6_Hdr *)IMM_ZcGetDataPtr(data);

    /* 判断下行包中的目的地址是否与DEVICE端地址相同 */
    if (ADS_FILTER_IS_IPV6_ADDR_IDENTICAL(ADS_FILTER_GET_IPV6_ADDR(), &(iPv6Hdr->unDstAddr))) {
        ADS_FILTER_DBG_STATISTIC(ADS_FILTER_ORIG_PKT_DL_IPV6_PKT, 1);

        ret = VOS_OK;
    } else {
        ret = VOS_ERR;
    }

    return ret;
}

