/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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
#include "at_ltev_ttf_set_cmd_proc.h"
#include "securec.h"

#include "AtParse.h"
#include "at_cmd_proc.h"
#include "at_snd_msg.h"
#include "at_ltev_comm.h"


/*
 * Э��ջ��ӡ��㷽ʽ�µ�.C�ļ��궨��
 */
#define THIS_FILE_ID PS_FILE_ID_AT_LTEV_TTF_SET_CMD_PROC_C

#define AT_PTRRPT_PARA_NUM 1

#define AT_MCGCFG_PARA_MAX_NUM       15
#define AT_MCGCFG_PARA_GROUP_ID      0 /* MCGCFG�ĵ�һ������GROUP_ID */
#define AT_MCGCFG_PARA_SELF_RSU_ID   1 /* MCGCFG�ĵڶ�������SELF_RSU_ID */
#define AT_MCGCFG_PARA_SELF_RSU_TYPE 2 /* MCGCFG�ĵ���������SELF_RSU_TYPE */
#define AT_MCGCFG_PARA_INDOOR_RSU_ID 3 /* MCGCFG�ĵ��ĸ�����INDOOR_RSU_ID */
#define AT_MCGCFG_PARA_RSU_LIST_NUM  4 /* MCGCFG�ĵ��������RSU_LIST_NUM */
#define AT_MCGCFG_PARA_RSU_ID_LIST   5 /* MCGCFG�ĵ���������RSU_ID_LIST */

#if (FEATURE_LTEV == FEATURE_ON)

VOS_UINT32 VPDCP_SetPtrRpt(MN_CLIENT_ID_T clientId, MN_OPERATION_ID_T opId)
{
    /* ������Ϣ��VRRCģ�� */
    VOS_UINT32       rslt;
    AT_VPDCP_ReqMsg *msg = VOS_NULL_PTR;
    VOS_UINT32       length;

    /* Allocating memory for message */
    length = sizeof(AT_VPDCP_ReqMsg) - VOS_MSG_HEAD_LENGTH;
    msg    = (AT_VPDCP_ReqMsg *)TAF_AllocMsgWithoutHeaderLen(WUEPS_PID_AT, length);
    if (msg == VOS_NULL_PTR) {
        return VOS_ERR;
    }

    (VOS_VOID)memset_s((VOS_UINT8 *)msg + VOS_MSG_HEAD_LENGTH, length, 0x00, length);

    msg->clientId      = clientId;
    msg->opId          = opId;
    msg->msgName       = VPDCP_MSG_RSU_PTRRPT_SET_REQ;
    msg->ulSenderPid   = WUEPS_PID_AT;
    msg->ulReceiverPid = AT_GetDestPid(clientId, I0_PS_PID_VPDCP_UL);
    msg->content[0]    = 1;

    rslt = TAF_TraceAndSendMsg(WUEPS_PID_AT, msg);
    if (rslt != VOS_OK) {
        AT_ERR_LOG("VPDCP_SetPtrRpt: SEND MSG FAIL");
        return VOS_ERR;
    }

    return VOS_OK;
}

VOS_UINT32 AT_SetPtrRpt(VOS_UINT8 indexNum)
{
    VOS_UINT32 rst;

    if (g_atParseCmd.cmdOptType != AT_CMD_OPT_SET_PARA_CMD) {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* �������� */
    if (g_atParaIndex != AT_PTRRPT_PARA_NUM) {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (g_atParaList[0].paraLen == 0) {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* ִ��������� */
    rst = VPDCP_SetPtrRpt(g_atClientTab[indexNum].clientId, g_atClientTab[indexNum].opId);

    if (rst != VOS_OK) {
        return AT_ERROR;
    }
    g_atClientTab[indexNum].cmdCurrentOpt = AT_CMD_PTRRPT_SET;
    return AT_WAIT_ASYNC_RETURN; /* ������������״̬ */
}

VOS_UINT32 AT_GetOptionCmdParaByParaIndex(VOS_UINT8 paraIndex)
{
    if (g_atParaList[paraIndex].paraLen == 0) {
        return 0;
    }

    return g_atParaList[paraIndex].paraValue;
}

#endif

