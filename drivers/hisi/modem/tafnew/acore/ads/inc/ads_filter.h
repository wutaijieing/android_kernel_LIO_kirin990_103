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

#ifndef __ADSFILTER_H__
#define __ADSFILTER_H__

#include "vos.h"
#include "ads_device_interface.h"
#include "hi_list.h"
#include "ads_tcp_ip_type_def.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)

#define ADS_FILTER_MAX_LIST_NUM (256)
#define ADS_FILTER_DEFAULT_AGING_TIMELEN (60)
#define ADS_FILTER_SECOND_TRANSFER_JIFFIES (1000)

#define ADS_FILTER_IPV4_HDR_LEN (20) /* IPv4 首部长度 */
#define ADS_FILTER_ICMP_HDR_LEN (8)  /* ICMP 首部长度 */

#define ADS_MAX_IPV4_ADDR_LEN (16) /* IPv4地址最大字符串长度 */

#define ADS_IPPROTO_ICMP (0x01) /* ICMP protocol */
#define ADS_IPPROTO_IGMP (0x02) /* IGMP protocol */
#define ADS_IPPROTO_TCP (0x06)  /* TCP protocol */
#define ADS_IPPROTO_UDP (0x11)  /* UDP protocol */

#define ADS_IP_CE (0x8000)     /* Flag: "Congestion" */
#define ADS_IP_DF (0x4000)     /* Flag: "Don't Fragment" */
#define ADS_IP_MF (0x2000)     /* Flag: "More Fragments" */
#define ADS_IP_OFFSET (0x1FFF) /* "Fragment Offset" part */

/* ICMP报文的type字段宏定义 */
#define ADS_ICMP_ECHOREPLY (0)       /* Echo Reply           */
#define ADS_ICMP_DEST_UNREACH (3)    /* Destination Unreachable  */
#define ADS_ICMP_SOURCE_QUENCH (4)   /* Source Quench        */
#define ADS_ICMP_REDIRECT (5)        /* Redirect (change route)  */
#define ADS_ICMP_ECHOREQUEST (8)     /* Echo Request         */
#define ADS_ICMP_TIME_EXCEEDED (11)  /* Time Exceeded        */
#define ADS_ICMP_PARAMETERPROB (12)  /* Parameter Problem    */
#define ADS_ICMP_TIMESTAMP (13)      /* Timestamp Request    */
#define ADS_ICMP_TIMESTAMPREPLY (14) /* Timestamp Reply      */
#define ADS_ICMP_INFO_REQUEST (15)   /* Information Request  */
#define ADS_ICMP_INFO_REPLY (16)     /* Information Reply    */
#define ADS_ICMP_ADDRESS (17)        /* Address Mask Request */
#define ADS_ICMP_ADDRESSREPLY (18)   /* Address Mask Reply   */

/* 数组元素个数计算所使用的宏 */
#define ADS_FILTER_ARRAY_SIZE(a) (sizeof((a)) / sizeof((a[0])))

#define ADS_FILTER_GET_IPV6_ADDR() (&g_adsFilterCtx.unIPv6Addr)
#define ADS_FILTER_GET_LIST(indexNum) (&(g_adsFilterCtx.lists[indexNum]))
#define ADS_FILTER_GET_AGING_TIME_LEN() (g_adsFilterCtx.agingTimeLen)
#define ADS_FILTER_SET_AGING_TIME_LEN(len) (g_adsFilterCtx.agingTimeLen = (len))

#define ADS_FILTER_GET_DL_ICMP_FUNC_TBL_ADDR(ucType) (&(g_adsFilterDecodeDlIcmpFuncTbl[ucType]))
#define ADS_FILTER_GET_DL_ICMP_FUNC_TBL_SIZE() (ADS_FILTER_ARRAY_SIZE(g_adsFilterDecodeDlIcmpFuncTbl))

/* 可维可测信息统计所用宏 */
#define ADS_FILTER_DBG_GET_STATS_ARRAY_ADDR() (&g_adsFilterStats[0])
#define ADS_FILTER_DBG_GET_STATS_BY_INDEX(enPktType) (g_adsFilterStats[enPktType])
#define ADS_FILTER_DBG_STATISTIC(enPktType, n) (g_adsFilterStats[enPktType] += (n))

#define ADS_FILTER_MALLOC(size) ADS_FilterHeapAlloc(size)
#define ADS_FILTER_FREE(addr) ADS_FilterHeapFree(addr)

/* 检查是否达到老化时间 */
#define ADS_FILTER_IS_AGED(time) \
    ((ADS_FILTER_GET_AGING_TIME_LEN() != 0) && (time_after_eq(jiffies, (ADS_FILTER_GET_AGING_TIME_LEN() + (time)))))

/* 检查IPV6地址是否相同 ADS_IPV6_ADDR_UN */
#define ADS_FILTER_IS_IPV6_ADDR_IDENTICAL(punIpv6Addr1, punIpv6Addr2)            \
    (0 == (((punIpv6Addr1)->ipAddrUint32[0] ^ (punIpv6Addr2)->ipAddrUint32[0]) | \
           ((punIpv6Addr1)->ipAddrUint32[1] ^ (punIpv6Addr2)->ipAddrUint32[1]) | \
           ((punIpv6Addr1)->ipAddrUint32[2] ^ (punIpv6Addr2)->ipAddrUint32[2]) | \
           ((punIpv6Addr1)->ipAddrUint32[3] ^ (punIpv6Addr2)->ipAddrUint32[3])))

/*
 * 过滤表索引号映射关系:
 * 对于TCP\UCP包，过滤表索引值为源端口号低8位,
 * 对于ICMP包，过滤表索引值为Sequence Num低8位,
 * 对于下行分片包(非首片)使用IP Identification低8位作为索引
 */
#define ADS_FILTER_GET_INDEX(pfilter) ((VOS_UINT8)(VOS_NTOHS(*((VOS_UINT16 *)&((pfilter)->unFilter))) & 0xFF))


enum ADS_FilterPktType {
    ADS_FILTER_PKT_TYPE_TCP,      /* 数据包类型为TCP */
    ADS_FILTER_PKT_TYPE_UDP,      /* 数据包类型为UDP */
    ADS_FILTER_PKT_TYPE_ICMP,     /* 数据包类型为ICMP */
    ADS_FILTER_PKT_TYPE_FRAGMENT, /* 数据包类型为分片包(非首片) */

    ADS_FILTER_PKT_TYPE_BUTT
};
typedef VOS_UINT32 ADS_FilterPktTypeUint32;


enum ADS_FilterOrigPkt {
    ADS_FILTER_ORIG_PKT_UL_IPV4_TCP,                /* 原始数据包类型为上行IPv4 TCP */
    ADS_FILTER_ORIG_PKT_UL_IPV4_UDP,                /* 原始数据包类型为上行IPv4 UDP */
    ADS_FILTER_ORIG_PKT_UL_IPV4_ECHOREQ,            /* 原始数据包类型为上行IPv4 ECHO REQ */
    ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_FIRST_FRAGMENT, /* 原始数据包类型为上行IPv4 分片包(非首片) */
    ADS_FILTER_ORIG_PKT_UL_IPV4_NOT_SUPPORT,        /* 原始数据包类型为上行不支持的IPv4报文 */
    ADS_FILTER_ORIG_PKT_UL_IPV6_PKT,                /* 原始数据包类型为上行IPv6报文 */

    ADS_FILTER_ORIG_PKT_DL_IPV4_TCP,                /* 原始数据包类型为下行IPv4 TCP */
    ADS_FILTER_ORIG_PKT_DL_IPV4_UDP,                /* 原始数据包类型为下行IPv4 UDP */
    ADS_FILTER_ORIG_PKT_DL_IPV4_ECHOREPLY,          /* 原始数据包类型为下行IPv4 ECHO REPLY */
    ADS_FILTER_ORIG_PKT_DL_IPV4_ICMPERROR,          /* 原始数据包类型为下行IPv4 ICMP差错报文 */
    ADS_FILTER_ORIG_PKT_DL_IPV4_FIRST_FRAGMENT,     /* 原始数据包类型为下行IPv4 分片包(首片) */
    ADS_FILTER_ORIG_PKT_DL_IPV4_NOT_FIRST_FRAGMENT, /* 原始数据包类型为下行IPv4 分片包(非首片) */
    ADS_FILTER_ORIG_PKT_DL_IPV6_PKT,                /* 原始数据包类型为下行IPv6包 */

    ADS_FILTER_ORIG_PKT_BUTT
};
typedef VOS_UINT32 ADS_FilterOrigPktUint32;


typedef struct {
    VOS_UINT32 srcAddr;     /* source address */
    VOS_UINT32 dstAddr;     /* dest address */
    VOS_UINT8  protocol;    /* protocol */
    VOS_UINT8  reserved[3]; /* reserved */
} ADS_FilterIpv4Header;


typedef struct {
    VOS_UINT16 srcPort; /* source port */
    VOS_UINT16 dstPort; /* dest port */
} ADS_FilterIpv4Tcp;


typedef ADS_FilterIpv4Tcp ADS_FILTER_IPV4_UDP_STRU;


typedef struct {
    VOS_UINT16 seqNum;     /* ICMP首部中的Sequence number */
    VOS_UINT16 identifier; /* ICMP首部中的Identifier */
} ADS_FilterIpv4Icmp;


typedef struct {
    VOS_UINT16 identification; /* IP首部中的identification */
    VOS_UINT8  reserved[2];    /* reserved */
} ADS_FilterIpv4Fragment;


typedef struct {
    ADS_FilterPktTypeUint32 pktType;
    VOS_UINT8               reserved[4];
    unsigned long           tmrCnt;
    ADS_FilterIpv4Header    ipHeader;

    union {
        ADS_FilterIpv4Tcp        tcpFilter;
        ADS_FILTER_IPV4_UDP_STRU udpFilter;
        ADS_FilterIpv4Icmp       icmpFilter;
        ADS_FilterIpv4Fragment   fragmentFilter;
    } unFilter;

} ADS_FilterIpv4Info;


typedef VOS_UINT32 (*ADS_FILTER_DECODE_DL_ICMP_FUNC)(ADS_IPV4_Hdr *pstIPv4Hdr, ADS_FilterIpv4Info *pstIPv4Filter);

typedef struct {
    ADS_FILTER_DECODE_DL_ICMP_FUNC func;
    ADS_FilterOrigPktUint32        origPktType;
    VOS_UINT8                      reserved[4];
} ADS_FilterDecodeDlIcmpFunc;


typedef struct {
    ADS_FilterIpv4Info filter;
    HI_LIST_S          list;
} ADS_FilterNode;


typedef struct {
    HI_LIST_S        lists[ADS_FILTER_MAX_LIST_NUM]; /* ADS下行数据过滤表 */
    ADS_IPV6_ADDR_UN unIPv6Addr;                     /* ADS用于过滤下行IPv6类型数据 */
    VOS_UINT32       agingTimeLen;                   /* ADS过滤节点老化时长, 单位毫秒 */
    VOS_UINT8        reserved[4];
} ADS_FilterCtx;

/* 检查IP首部里的相关信息是否匹配 ADS_FILTER_IPV4_HEADER_STRU */
static inline VOS_BOOL ADS_FilterIsIpHdrMatch(ADS_FilterIpv4Header *ipHeader1, ADS_FilterIpv4Header *ipHeader2)
{
    return (((ipHeader1->srcAddr == ipHeader2->srcAddr) && (ipHeader1->dstAddr == ipHeader2->dstAddr) &&
            (ipHeader1->protocol == ipHeader2->protocol)) ? VOS_TRUE : VOS_FALSE);
}

static inline VOS_BOOL ADS_FilterIsTcpPktMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    return (((filter1->unFilter.tcpFilter.srcPort == filter2->unFilter.tcpFilter.srcPort) &&
        (filter1->unFilter.tcpFilter.dstPort == filter2->unFilter.tcpFilter.dstPort)) ? VOS_TRUE : VOS_FALSE);
}

static inline VOS_BOOL ADS_FilterIsUdpPktMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    return (((filter1->unFilter.udpFilter.srcPort == filter2->unFilter.udpFilter.srcPort) &&
        (filter1->unFilter.udpFilter.dstPort == filter2->unFilter.udpFilter.dstPort)) ? VOS_TRUE : VOS_FALSE);
}

static inline VOS_BOOL ADS_FilterIsIcmpPktMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    return (((filter1->unFilter.icmpFilter.identifier == filter2->unFilter.icmpFilter.identifier) &&
        (filter1->unFilter.icmpFilter.seqNum == filter2->unFilter.icmpFilter.seqNum)) ? VOS_TRUE : VOS_FALSE);
}

static inline VOS_BOOL ADS_FilterIsFragmentMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2)
{
    return ((filter1->unFilter.fragmentFilter.identification == filter2->unFilter.fragmentFilter.identification) ?
        VOS_TRUE : VOS_FALSE);
}

VOS_VOID ADS_FilterResetIPv6Addr(VOS_VOID);

VOS_VOID ADS_FilterInitListsHead(VOS_VOID);

VOS_VOID ADS_FilterInitCtx(VOS_VOID);

VOS_VOID* ADS_FilterHeapAlloc(VOS_UINT32 size);

VOS_VOID ADS_FilterHeapFree(VOS_VOID *addr);

VOS_VOID ADS_FilterAddFilter(ADS_FilterIpv4Info *filter);

VOS_VOID ADS_FilterResetLists(VOS_VOID);

VOS_VOID ADS_FilterReset(VOS_VOID);

VOS_UINT32 ADS_FilterIsInfoMatch(ADS_FilterIpv4Info *filter1, ADS_FilterIpv4Info *filter2);

VOS_UINT32 ADS_FilterMatch(ADS_FilterIpv4Info *filter);

VOS_VOID ADS_FilterSaveIPAddrInfo(ADS_FilterIpAddrInfo *filterIpAddr);

VOS_UINT32 ADS_FilterDecodeUlPacket(IMM_Zc *data, ADS_FilterIpv4Info *iPv4Filter);

VOS_VOID ADS_FilterProcUlPacket(IMM_Zc *data, ADS_PktTypeUint8 ipType);

VOS_VOID ADS_FilterDecodeDlIPv4NotFirstFragPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter);

VOS_UINT32 ADS_FilterDecodeDlIPv4EchoReplyPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter);

VOS_UINT32 ADS_FilterDecodeDlIPv4IcmpErrorPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter);

VOS_VOID ADS_FilterDecodeDlIPv4TcpPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter);

VOS_VOID ADS_FilterDecodeDlIPv4UdpPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter);

VOS_UINT32 ADS_FilterDecodeDlIPv4FragPacket(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter,
                                            ADS_FilterOrigPktUint32 *origPktType);

VOS_UINT32 ADS_FilterDecodeDlIPv4Packet(ADS_IPV4_Hdr *iPv4Hdr, ADS_FilterIpv4Info *iPv4Filter,
                                        ADS_FilterOrigPktUint32 *origPktType);

VOS_UINT32 ADS_FilterProcDlIPv4Packet(IMM_Zc *data);

VOS_UINT32 ADS_FilterProcDlIPv6Packet(IMM_Zc *data);


#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of ads_filter.h */
