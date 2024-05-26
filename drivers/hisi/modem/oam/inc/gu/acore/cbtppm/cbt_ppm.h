/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
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

/*
 * 版权所有（c）华为技术有限公司 2001-2011
 * 功能描述: cbt_ppm.c 的头文件
 * 修改历史:
 * 1.日    期: 2014年5月31日
 *   修改内容: 创建文件
 */

#ifndef __CBT_PPM_H__
#define __CBT_PPM_H__

#include "vos.h"
#include "nv_stru_pam.h"
#include "mdrv.h"

#if (FEATURE_ACORE_MODULE_TO_CCORE == FEATURE_OFF)
#if (defined(FEATURE_HISOCKET) && (FEATURE_HISOCKET == FEATURE_ON) && (VOS_WIN32 != VOS_OS_VER))
#include "mdrv_socket.h"
#endif
#else
#include "mdrv_vcom.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(push, 4)
#if defined(PAM_CT_ENABLED) || defined(PAM_BT_ENABLED) || defined(CBT_ENABLED)

#define BIT_LEFTMOVE_N(num) (0x01 << (num))
#define CBT_ACPU_VCOM_CB BIT_LEFTMOVE_N(0)
#define CBT_ACPU_USB_CB BIT_LEFTMOVE_N(2)

#define CBT_PRINT_RETURN_VAL(ret, strFunc, strErr) CBT_PrintReturnVal(ret, strFunc, strErr)

#if (FEATURE_ACORE_MODULE_TO_CCORE == FEATURE_OFF)
#if (defined(FEATURE_HISOCKET) && (FEATURE_HISOCKET == FEATURE_ON) && (VOS_WIN32 != VOS_OS_VER))
#define CBTPPM_SOCK_MSG_LEN 1000

#define CBTPPM_SOCK_PORT_NUM 20250

#define CBTPPM_SOCKET_NUM_MAX 1

#if (VOS_LINUX == VOS_OS_VER)
#define CBTPPM_SOCK_NULL (-1)

#define CBTPPM_SOCKET_ERROR (VOS_ERROR)

#define CBT_IP_NULL 0
#define CBT_Ip_sockaddr sockaddr
#define CBT_Ip_socklen_t int
#define CBT_Ip_fd int
#define CBT_Ip_fd_set hi_fd_set

#define CBT_socket(domain, type, protocol) mdrv_socket(domain, type, protocol)

#define CBT_shutdown(fd, how) do { \
    mdrv_shutdown((CBT_Ip_fd)fd, how); \
    mdrv_close((CBT_Ip_fd)fd);         \
} while (0)

#define CBT_closesocket(s) CBT_shutdown(s, SHUT_RDWR)

#define CBT_bind(fd, addr, addrlen) mdrv_bind((CBT_Ip_fd)fd, (struct CBT_Ip_sockaddr *)addr, (CBT_Ip_socklen_t)addrlen)
#define CBT_accept(fd, addr, addrlenp) \
    mdrv_accept((CBT_Ip_fd)fd, (struct CBT_Ip_sockaddr *)addr, (CBT_Ip_socklen_t *)addrlenp)
#define CBT_listen(fd, backlog) mdrv_listen((CBT_Ip_fd)fd, backlog)
#define CBT_select(nfds, rf, wf, ef, t) mdrv_select(nfds, (CBT_Ip_fd_set *)rf, (CBT_Ip_fd_set *)wf, CBT_IP_NULL, t)
#define CBT_recv(fd, buf, len, flags) mdrv_recv((CBT_Ip_fd)fd, (void *)buf, len, flags)
#define CBT_send(fd, msg, len, flags) mdrv_send((CBT_Ip_fd)fd, (void *)msg, len, flags)
#else
#define CBTPPM_SOCK_NULL 0

#define CBT_Ip_fd_set fd_set

#endif

#endif

#endif

/* Cbt Usb Debug Info 上报定时器 */
#define CBT_USB_DEBUG_INFO_TIMER_LENTH 5000
#define CBT_USB_DEBUG_INFO_TIMER_NAME  0x00001001

#define OM_ICC_BUFFER_SIZE (16 * 1024)

#define OM_DRV_MAX_IO_COUNT 8 /* 一次提交给底软接口的最大数目 */

/* UDI设备句柄 */
enum OM_ProtHandle {
    OM_USB_CBT_PORT_HANDLE = 0,
    OM_PORT_HANDLE_BUTT
};

typedef VOS_UINT32 OM_ProtHandleUint32;

enum OM_LogicChannel {
    OM_LOGIC_CHANNEL_CBT = 0,
    OM_LOGIC_CHANNEL_BUTT
};

typedef VOS_UINT32 OM_LogicChannelUint32;

enum {
    CBTCPM_SEND_OK,
    CBTCPM_SEND_FUNC_NULL,
    CBTCPM_SEND_PARA_ERR,
    CBTCPM_SEND_ERR,
    CBTCPM_SEND_AYNC,
    CBTCPM_SEND_BUTT
};

/* 用于记录VCOM发送信息 */
typedef struct {
    VOS_UINT32 vcomSendSn;
    VOS_UINT32 vcomSendNum;
    VOS_UINT32 vcomSendLen;
    VOS_UINT32 vcomSendErrNum;
    VOS_UINT32 vcomSendErrLen;

    VOS_UINT32 vcomRcvSn;
    VOS_UINT32 vcomRcvNum;
    VOS_UINT32 vcomRcvLen;
    VOS_UINT32 vcomRcvErrNum;
    VOS_UINT32 vcomRcvErrLen;
} CBT_AcpuVcomDebugInfo;

#if (FEATURE_ACORE_MODULE_TO_CCORE == FEATURE_OFF)
#if (defined(FEATURE_HISOCKET) && (FEATURE_HISOCKET == FEATURE_ON) && (VOS_WIN32 != VOS_OS_VER))

#if (VOS_LINUX == VOS_OS_VER)

typedef int CBTPPM_SOCKET;

#else

typedef unsigned int CBTPPM_SOCKET;

#endif

typedef struct {
    CBTPPM_SOCKET socket;
    CBTPPM_SOCKET listener;
    VOS_SEM       smClose;
    VOS_CHAR      buf[CBTPPM_SOCK_MSG_LEN];
} CBTPPM_SocketCtrlInfo;

typedef struct {
    VOS_UINT32 listernNum1;
    VOS_UINT32 listernNum2;
    VOS_UINT32 rcvData;
    VOS_UINT32 failToProcess;
    VOS_UINT32 sndData;
    VOS_UINT32 failToSend;
} CBTPPM_SocketDebugInfo;
#endif
#endif

/* 用于记录USB发送信息 */
typedef struct {
    VOS_UINT32 usbOpenNum;
    VOS_UINT32 usbOpenSlice;
    VOS_UINT32 usbOpenOkNum;
    VOS_UINT32 usbOpenOkSlice;
    VOS_UINT32 usbOpenOk2Num;
    VOS_UINT32 usbOpenOk2Slice;

    VOS_UINT32 usbReadNum1;
    VOS_UINT32 usbReadInSlice;
    VOS_UINT32 usbReadNum2;
    VOS_UINT32 usbReadOutSlice;
    VOS_UINT32 usbReadMaxTime;

    VOS_UINT32 usbReadErrNum;
    VOS_UINT32 usbReadErrValue;
    VOS_UINT32 usbReadErrTime;
    VOS_UINT32 usbDecodeErrNum;

    VOS_UINT32 usbWriteNum1;
    VOS_UINT32 usbWriteInSlice;
    VOS_UINT32 usbWriteNum2;
    VOS_UINT32 usbWriteOutSlice;
    VOS_UINT32 usbWriteMaxTime;

    VOS_UINT32 usbPseudoSyncSemInSlice;
    VOS_UINT32 usbPseudoSyncSemOutSlice;
    VOS_UINT32 usbPseudoSyncSemNum;
    VOS_UINT32 usbPseudoSyncSemErrNum;

    VOS_UINT32 usbWriteErrNum;
    VOS_UINT32 usbWriteErrLen;
    VOS_UINT32 usbWriteErrValue;
    VOS_UINT32 usbWriteErrTime;
} CBT_AcpuUsbDebugInfo;

typedef struct {
    CBT_AcpuVcomDebugInfo cbtVcomAcpuDebugInfo;
    CBT_AcpuUsbDebugInfo  cbtUsbAcpuDebugInfo;
} CBT_ACPU_DEBUG_INFO;

/* 操作命令和操作参数的匹配表 */
typedef struct {
    VOS_UINT32 u32Cmd;
    VOS_VOID *pParam;
} CBT_AcpuCmdStru;

VOS_UINT32 CBTPPM_OamCbtPortDataSnd(VOS_UINT8 *virAddr, VOS_UINT32 dataLen);

VOS_VOID CBTPPM_OamUsbCbtReadDataCB(VOS_VOID);

VOS_VOID CBTPPM_OamCbtPortDataInit(OM_ProtHandleUint32 handle, VOS_VOID *readCb, VOS_VOID *writeCb, VOS_VOID *stateCb);

VOS_VOID CBTPPM_OamCbtPortInit(VOS_VOID);

VOS_UINT32 CBTPPM_SocketTaskInit(VOS_VOID);

VOS_VOID CBTPPM_SocketPortInit(VOS_VOID);

VOS_VOID CBT_PrintReturnVal(VOS_UINT32 ret, const VOS_CHAR *funcName, const VOS_CHAR *errInfo);

#endif /* end if CBT_ENABLED */

#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of cbt_ppm.h */
