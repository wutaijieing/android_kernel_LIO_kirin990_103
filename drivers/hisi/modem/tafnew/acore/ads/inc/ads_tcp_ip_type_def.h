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

#ifndef __ADS_TCPIPTYPEDEF_H__
#define __ADS_TCPIPTYPEDEF_H__

#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 1)

/* IP 数据包可维可测 */
#define ADS_IP_VERSION_V4 (4)          /* IPV4的版本号 */
#define ADS_IPV4_HDR_LEN (20)          /* IPV4的头部长度 */
#define ADS_IPV4_PROTO_ICMP (1)        /* IPV4的ICMP协议号 */
#define ADS_IPV4_ICMP_ECHO_REQUEST (8) /* IPV4的ICMP的TYPE ECHO REQ */
#define ADS_IPV4_ICMP_ECHO_REPLY (0)   /* IPV4的ICMP的TYPE ECHO REPLY */

#define ADS_IP_VERSION_V6 (6)            /* IPV6的版本号 */
#define ADS_IPV6_HDR_LEN (40)            /* IPV6的头部长度 */
#define ADS_IPV6_PROTO_ICMP (58)         /* IPV6的ICMP协议号 */
#define ADS_IPV6_ICMP_ECHO_REQUEST (128) /* IPV6的ICMP的TYPE ECHO REQ */
#define ADS_IPV6_ICMP_ECHO_REPLY (129)   /* IPV6的ICMP的TYPE ECHO REPLY */

#define ADS_IP_PROTO_TCP (6)  /* TCP协议号 */
#define ADS_IP_PROTO_UDP (17) /* UDP协议号 */

#define ADS_IPV4_ADDR_LEN (4)  /* IPV4地址长度 */
#define ADS_IPV6_ADDR_LEN (16) /* IPV6地址长度 */
#define ADS_IPV6_ADDR_HALF_LEN (8)
#define ADS_IPV6_ADDR_QUARTER_LEN (4)

#define ADS_GET_IP_VERSION(ipPkt) ((ipPkt)[0] >> 4) /* 获取IP version */

/* 大小字节序转换 */
#ifndef VOS_NTOHL
#if VOS_BYTE_ORDER == VOS_BIG_ENDIAN
#define VOS_NTOHL(x) (x)
#define VOS_HTONL(x) (x)
#define VOS_NTOHS(x) (x)
#define VOS_HTONS(x) (x)
#else
#define VOS_NTOHL(x) \
    ((((x) & 0x000000ffU) << 24) | (((x) & 0x0000ff00U) << 8) | (((x) & 0x00ff0000U) >> 8) | (((x) & 0xff000000U) >> 24))

#define VOS_HTONL(x) \
    ((((x) & 0x000000ffU) << 24) | (((x) & 0x0000ff00U) << 8) | (((x) & 0x00ff0000U) >> 8) | (((x) & 0xff000000U) >> 24))

#define VOS_NTOHS(x) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))

#define VOS_HTONS(x) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#endif /* _BYTE_ORDER==_LITTLE_ENDIAN */
#endif


typedef union {
    VOS_UINT8  ipAddrUint8[4];
    VOS_UINT32 ipAddrUint32;
} ADS_IPV4_ADDR_UN;


typedef union {
    VOS_UINT8  ipAddrUint8[16];
    VOS_UINT16 ipAddrUint16[8];
    VOS_UINT32 ipAddrUint32[4];
} ADS_IPV6_ADDR_UN;


typedef struct {
    VOS_UINT8        ipHdrLen : 4;   /* header length */
    VOS_UINT8        ipVersion : 4;  /* version */
    VOS_UINT8        typeOfService;  /* type of service */
    VOS_UINT16       totalLen;       /* total length */
    VOS_UINT16       identification; /* identification */
    VOS_UINT16       flagsFo;        /* flags and fragment offset field */
    VOS_UINT8        ttl;            /* time to live */
    VOS_UINT8        protocol;       /* protocol */
    VOS_UINT16       checkSum;       /* checksum */
    ADS_IPV4_ADDR_UN unSrcAddr;      /* source address */
    ADS_IPV4_ADDR_UN unDstAddr;      /* dest address */
} ADS_IPV4_Hdr;


typedef struct {
    VOS_UINT8        priority : 4;  /* Priority  */
    VOS_UINT8        ipVersion : 4; /* ip version, to be 6 */
    VOS_UINT8        flowLabel[3];  /* flow lable */
    VOS_UINT16       payloadLen;    /* not include ipv6 fixed hdr len 40bytes */
    VOS_UINT8        nextHdr;       /* for l4 protocol or ext hdr */
    VOS_UINT8        hopLimit;
    ADS_IPV6_ADDR_UN unSrcAddr;
    ADS_IPV6_ADDR_UN unDstAddr;
} ADS_IPV6_Hdr;


typedef struct {
    VOS_UINT16 srcPort;  /* 源端口 */
    VOS_UINT16 dstPort;  /* 目的端口 */
    VOS_UINT16 len;      /* UDP包长度 */
    VOS_UINT16 checksum; /* UDP校验和 */
} ADS_UdpHdr;


typedef struct {
    VOS_UINT16 srcPort;
    VOS_UINT16 dstPort;
    VOS_UINT32 seqNum;
    VOS_UINT32 ackNum;
    VOS_UINT16 reserved : 4;
    VOS_UINT16 offset : 4;
    VOS_UINT16 fin : 1;
    VOS_UINT16 syn : 1;
    VOS_UINT16 rst : 1;
    VOS_UINT16 psh : 1;
    VOS_UINT16 ack : 1;
    VOS_UINT16 urg : 1;
    VOS_UINT16 ece : 1;
    VOS_UINT16 cwr : 1;
    VOS_UINT16 windowSize;
    VOS_UINT16 checkSum;
    VOS_UINT16 urgentPoint;
} ADS_TcpHdr;


typedef struct {
    VOS_UINT16 identifier;
    VOS_UINT16 seqNum;

} ADS_ICMP_EchoHdr;


typedef struct {
    VOS_UINT32 unUsed;

} ADS_ICMP_ErrorHdr;


typedef struct {
    VOS_UINT8  type;
    VOS_UINT8  code;
    VOS_UINT16 checkSum;

    union {
        ADS_ICMP_EchoHdr  icmpEcho;
        ADS_ICMP_ErrorHdr icmpError;
    } unIcmp;

} ADS_ICMP_Hdr;


#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of ads_tcp_ip_type_def.h */
