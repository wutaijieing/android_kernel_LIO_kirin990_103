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

#include "ppp_data_interface.h"
#include "ads_device_interface.h"

__STATIC_ PPP_DlPacketRxFun g_dlPacketReceiver = VOS_NULL_PTR;

/* IP���Ͳ��ţ����յ�Um/Uu�ڵ����� */
__STATIC_ VOS_INT PPP_PushPacketEvent(VOS_UINT8 rabId, IMM_Zc *immZc, ADS_PktTypeUint8 pktType, VOS_UINT32 exParam)
{
    PPP_Id pppId = 0;

    if ((!PPP_RAB_TO_PPPID(&pppId, rabId)) || (g_dlPacketReceiver == VOS_NULL_PTR)) {
        PPP_MemFree(immZc);
        PPP_MNTN_LOG2(PS_PRINT_NORMAL, "PPP, PPP_PushPacketEvent, NORMAL, Can not get PPP Id, RabId <1> <2>",
            rabId, (VOS_INT32)(g_dlPacketReceiver == VOS_NULL_PTR));

        return VOS_ERR;
    }

    return g_dlPacketReceiver(pppId, immZc);
}

VOS_UINT32 PPP_RegRxDlDataHandler(PPP_Id pppId, PPP_DlPacketRxFun fun)
{
    VOS_UINT8  rabId = 0;
    VOS_UINT32 result;

    /* ͨ��usPppId��Ѱ�ҵ�usRabId */
    if (!PPP_PPPID_TO_RAB(pppId, &rabId)) {
        PPP_MNTN_LOG2(PS_PRINT_NORMAL, "Can not get PPP Id", pppId, rabId);
        return VOS_ERR;
    }

    /* ���ʱ��PDP�Ѿ����ע���������ݽ��սӿ� */
    result = ADS_DL_RegDlDataCallback(rabId, PPP_PushPacketEvent, 0);
    if (result != VOS_OK) {
        PPP_MNTN_LOG1(PS_PRINT_WARNING, "Register DL CB failed", rabId);
        return VOS_ERR;
    }
    
    g_dlPacketReceiver = fun;
    return VOS_OK;
}

VOS_UINT32 PPP_TxUlData(VOS_UINT16 pppId, PPP_Zc *immZc, VOS_UINT16 proto)
{
    VOS_UINT8 rabId = 0;

    /* ͨ��pppId��Ѱ�ҵ�rabId */
    if (!PPP_PPPID_TO_RAB(pppId, &rabId)) {
        PPP_MemFree(immZc);
        PPP_MNTN_LOG2(PS_PRINT_NORMAL, "PPP, PPP_PushPacketEvent, WARNING, Can not get PPP Id <1>, RabId <2>", pppId, rabId);
        return VOS_ERR;
    }

    /* ���ݷ��͸�ADS�����ʧ����ADS�ͷ��ڴ� */
    if (ADS_UL_SendPacket(immZc, rabId) != VOS_OK) {
        return VOS_ERR;
    }
    return VOS_OK;
}

VOS_UINT32 PPP_ReserveHeaderInBypassMode(IMM_Zc **mem)
{
    return VOS_OK;
}

