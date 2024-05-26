﻿/*
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
 * 1 Include HeadFile
 */
#include <securec.h>
#include "diag_connect.h"
#include <product_config.h>
#include <mdrv.h>
#include <msp.h>
#include <vos.h>
#include "diag_common.h"
#include "diag_cfg.h"
#include "diag_msgmsp.h"
#include "diag_msgphy.h"
#include "diag_api.h"
#include "diag_debug.h"
#include "msp_errno.h"
#include "soc_socp_adapter.h"
#include "diag_time_stamp.h"
#include "diag_msgps.h"
#include "diag_api.h"

#define THIS_FILE_ID MSP_FILE_ID_DIAG_CONNECT_C

#ifdef DIAG_SEC_TOOLS
VOS_UINT32 g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
HTIMER g_AuthTimer = VOS_NULL;
#endif

HTIMER g_TransCnfTimer = VOS_NULL;

/*
 * Function Name: diag_GetModemInfo
 * Description: 获取modem信息(用于工具未连接时，工具后台查询单板信息)
 * Input: VOS_VOID
 * Output: None
 * Return: VOS_UINT32
 */
VOS_VOID diag_GetModemInfo(DIAG_CONNECT_FRAME_INFO_STRU *pstDiagHead)
{
    VOS_UINT32 ulCnfRst;
    DIAG_CMD_HOST_CONNECT_CNF_STRU stCnf = {0};
    const modem_ver_info_s *pstVerInfo = VOS_NULL;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo;
    errno_t err = EOK;

    /* 处理结果 */
    stCnf.ulAuid = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulAuid;
    stCnf.ulSn = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulSn;

    /* 获取版本信息 */
    pstVerInfo = mdrv_ver_get_info();
    if (pstVerInfo != NULL) {
        /* 获取数采基地址 */
        stCnf.ulChipBaseAddr = (VOS_UINT32)pstVerInfo->product_info.chip_type;
    }

    /* 获取IMEI号 */

    /* 获取软件版本号 */
    err = memset_s(&stCnf.stUeSoftVersion, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU), 0,
                           (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU));
    if (err != EOK) {
        diag_error("memset fail:%x\n", err);
    }

    /* 路测信息获取 */
    (VOS_VOID)mdrv_nv_read(EN_NV_ID_AGENT_FLAG, &(stCnf.stAgentFlag), (VOS_UINT32)sizeof(NV_ITEM_AGENT_FLAG_STRU));

    stCnf.diag_cfg.UintValue = 0;

    /* 010: OM通道融合的版本 */
    /* 110: OM融合GU未融合的版本 */
    /* 100: OM完全融合的版本 */
    stCnf.diag_cfg.CtrlFlag.ulDrxControlFlag = 0; /* 和HIDS确认此处不再使用,打桩处理即可 */
    stCnf.diag_cfg.CtrlFlag.ulPortFlag = 0;
    stCnf.diag_cfg.CtrlFlag.ulOmUnifyFlag = 1;

    stCnf.ulLpdMode = 0x5a5a5a5a;
    stCnf.ulRc = ERR_MSP_SUCCESS;

    err = memset_s(stCnf.szProduct, (VOS_UINT32)sizeof(stCnf.szProduct), 0,
                           (VOS_UINT32)sizeof(stCnf.szProduct));
    if (err != EOK) {
        diag_error("memset fail:%x\n", err);
    }
    err = memcpy_s(stCnf.szProduct, (VOS_UINT32)sizeof(stCnf.szProduct), PRODUCT_FULL_VERSION_STR,
                           VOS_StrNLen(PRODUCT_FULL_VERSION_STR, sizeof(stCnf.szProduct) - 1));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    stDiagInfo.ulSSId = DIAG_SSID_CPU;
    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;
    stDiagInfo.ulMode = pstDiagHead->stID.mode4b;
    stDiagInfo.ulSubType = pstDiagHead->stID.sec5b;
    stDiagInfo.ulDirection = DIAG_MT_CNF;
    stDiagInfo.ulModemid = 0;
    stDiagInfo.ulMsgId = pstDiagHead->ulCmdId;

    stCnf.ulAuid = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulAuid;
    stCnf.ulSn = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulSn;

    ulCnfRst = mdrv_diag_report_cnf((diag_cnf_info_s *)&stDiagInfo, (VOS_VOID *)&stCnf, sizeof(stCnf));
    if (ERR_MSP_SUCCESS != ulCnfRst) {
        diag_error("diag_GetModemInfo failed.\n");
    } else {
        diag_crit("diag_GetModemInfo success.\n");
    }

    return;
}
/*
 * Function Name: diag_ConnProc
 * Description: 该函数用于处理ConnProcEntry传进来的HSO连接命令
 * Input: pstReq 待处理数据
 * Output: None
 * Return: VOS_UINT32
 */
VOS_UINT32 diag_ConnProc(VOS_UINT8 *pstReq)
{
    VOS_UINT32 ulCnfRst = ERR_MSP_UNAVAILABLE;
    DIAG_MSG_MSP_CONN_STRU *pstConn = VOS_NULL;
    DIAG_CMD_HOST_CONNECT_CNF_STRU stCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    const modem_ver_info_s *pstVerInfo = VOS_NULL;
    mdrv_diag_cmd_reply_set_req_s stReplay = {0};
    msp_diag_frame_info_s *pstDiagHead = VOS_NULL;
    DIAG_CMD_GET_MDM_INFO_REQ_STRU *pstInfo = VOS_NULL;
    errno_t err = EOK;

    pstDiagHead = (msp_diag_frame_info_s *)pstReq;

    mdrv_diag_ptr(EN_DIAG_PTR_MSGMSP_CONN_IN, 1, pstDiagHead->ulCmdId, 0);
	
    if (ERR_MSP_SUCCESS != diag_CheckModemStatus()) {
        return ERR_MSP_UNAVAILABLE;
    }	

    /* 新增获取modem信息的命令用于工具查询单板信息 */
    if (pstDiagHead->ulMsgLen >= sizeof(DIAG_CMD_GET_MDM_INFO_REQ_STRU)) {
        pstInfo = (DIAG_CMD_GET_MDM_INFO_REQ_STRU *)pstDiagHead->aucData;
        if (DIAG_GET_MODEM_INFO_BIT == (pstInfo->ulInfo & DIAG_GET_MODEM_INFO_BIT)) {
            diag_GetModemInfo((DIAG_CONNECT_FRAME_INFO_STRU *)pstDiagHead);
            return ERR_MSP_SUCCESS;
        }
#ifdef DIAG_SEC_TOOLS
        if (DIAG_VERIFY_SIGN_BIT == (pstInfo->ulInfo & DIAG_VERIFY_SIGN_BIT)) {
            diag_ConnAuth((DIAG_CONNECT_FRAME_INFO_STRU *)pstDiagHead);
            return ERR_MSP_SUCCESS;
        }
#endif
    }

    if (pstDiagHead->ulMsgLen < sizeof(msp_diag_data_head_s)) {
        diag_error("rev tool msg len error:0x%x\n", pstDiagHead->ulMsgLen);
        return ERR_MSP_INALID_LEN_ERROR;
    }

    diag_crit("Receive tool connect cmd!\n");

#ifdef DIAG_SEC_TOOLS
    g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
#endif
    pstConn =
        (DIAG_MSG_MSP_CONN_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                               (VOS_UINT32)(sizeof(DIAG_MSG_MSP_CONN_STRU) - VOS_MSG_HEAD_LENGTH));
    if (VOS_NULL == pstConn) {
        ulCnfRst = ERR_MSP_DIAG_ALLOC_MALLOC_FAIL;
        goto DIAG_ERROR;
    }

    /* 设置连接状态开关值 */
    ulCnfRst = diag_CfgSetGlobalBitValue(&g_ulDiagCfgInfo, DIAG_CFG_CONN_BIT, DIAG_CFG_SWT_OPEN);
    if (ulCnfRst) {
        diag_error("Open DIAG_CFG_CONN_BIT failed.\n");
        goto DIAG_ERROR;
    }

    /* 获取版本信息 */
    pstVerInfo = mdrv_ver_get_info();
    if (pstVerInfo != NULL) {
        /* 获取数采基地址 */
        pstConn->stConnInfo.ulChipBaseAddr = (VOS_UINT32)pstVerInfo->product_info.chip_type;
    }

    /* 获取IMEI号 */

    /* 获取软件版本号 */
    err = memset_s(&pstConn->stConnInfo.stUeSoftVersion, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU), 0,
                           (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU));
    if (err != EOK) {
        diag_error("memset fail:%x\n", err);
    }

    /* 路测信息获取 */
    (VOS_VOID)mdrv_nv_read(EN_NV_ID_AGENT_FLAG, &(pstConn->stConnInfo.stAgentFlag),
                           (VOS_UINT32)sizeof(NV_ITEM_AGENT_FLAG_STRU));

    pstConn->stConnInfo.diag_cfg.UintValue = 0;

    /* 010: OM通道融合的版本 */
    /* 110: OM融合GU未融合的版本 */
    /* 100: OM完全融合的版本 */
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulDrxControlFlag = 0; /* 和HIDS确认此处不再使用,打桩处理即可 */
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulPortFlag = 0;
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulOmUnifyFlag = 1;
#ifdef DIAG_SEC_TOOLS
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulAuthFlag = 1;
#else
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulAuthFlag = 0;
#endif
    pstConn->stConnInfo.ulLpdMode = 0x5a5a5a5a;

    err = memset_s(pstConn->stConnInfo.szProduct, (VOS_UINT32)sizeof(pstConn->stConnInfo.szProduct), 0,
                           (VOS_UINT32)sizeof(pstConn->stConnInfo.szProduct));
    if (err != EOK) {
        diag_error("memset fail:%x\n", err);
    }

    err = memcpy_s(pstConn->stConnInfo.szProduct, (VOS_UINT32)sizeof(pstConn->stConnInfo.szProduct),
                           PRODUCT_FULL_VERSION_STR,
                           VOS_StrNLen(PRODUCT_FULL_VERSION_STR, sizeof(pstConn->stConnInfo.szProduct) - 1));
    if (err != EOK) {
        diag_error("memcpy fail:%x\n", err);
    }

    ulCnfRst = diag_SendMsg(MSP_PID_DIAG_APP_AGENT, PS_PID_MM, ID_MSG_DIAG_CMD_REPLAY_TO_PS, (VOS_UINT8*)&stReplay,\
                            (VOS_UINT32)sizeof(mdrv_diag_cmd_reply_set_req_s));
    if (ulCnfRst) {
        ulCnfRst = ERR_MSP_DIAG_SEND_MSG_FAIL;
        goto DIAG_ERROR;
    }

    /* 处理结果 */
    pstConn->stConnInfo.ulAuid = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulAuid;
    pstConn->stConnInfo.ulSn = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulSn;
    pstConn->stConnInfo.ulRc = ERR_MSP_SUCCESS;

    pstConn->ulReceiverPid = MSP_PID_DIAG_AGENT;
    pstConn->ulSenderPid = MSP_PID_DIAG_APP_AGENT;
    pstConn->ulCmdId = pstDiagHead->ulCmdId;
    pstConn->ulMsgId = DIAG_MSG_MSP_CONN_REQ;

    ulCnfRst = VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstConn);
    if (ERR_MSP_SUCCESS == ulCnfRst) {
        /* 复位维测信息记录 */
        mdrv_diag_reset_mntn_info(DIAGLOG_SRC_MNTN);
        mdrv_diag_reset_mntn_info(DIAGLOG_DST_MNTN);

        mdrv_applog_conn();

        mdrv_hds_printlog_conn();

        mdrv_hds_translog_conn();

        DIAG_NotifyConnState(DIAG_STATE_CONN);

#ifndef DIAG_SEC_TOOLS
        if (!DIAG_IS_POLOG_ON) {
            mdrv_socp_send_data_manager(SOCP_CODER_DST_OM_IND, SOCP_DEST_DSM_ENABLE);
        }
#endif
        diag_crit("Diag send ConnInfo to Modem success.\n");

        return ulCnfRst;
    }

DIAG_ERROR:

    if (pstConn != VOS_NULL) {
        if (VOS_FreeMsg(MSP_PID_DIAG_APP_AGENT, pstConn)) {
            diag_error("free mem fail\n");
        }
    }

    stCnf.ulRc = ulCnfRst;

    stDiagInfo.ulSSId = DIAG_SSID_CPU;
    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;
    stDiagInfo.ulMode = pstDiagHead->stID.mode4b;
    stDiagInfo.ulSubType = pstDiagHead->stID.sec5b;
    stDiagInfo.ulDirection = DIAG_MT_CNF;
    stDiagInfo.ulModemid = 0;
    stDiagInfo.ulMsgId = pstDiagHead->ulCmdId;

    stCnf.ulAuid = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulAuid;
    stCnf.ulSn = ((msp_diag_data_head_s *)(pstDiagHead->aucData))->ulSn;

    ulCnfRst = mdrv_diag_report_cnf((diag_cnf_info_s *)&stDiagInfo, &stCnf, (VOS_UINT32)sizeof(stCnf));

    diag_error("diag connect failed.\n");

    return ulCnfRst;
}


VOS_UINT32 diag_SetChanDisconn(MsgBlock *pMsgBlock)
{
#ifdef DIAG_SEC_TOOLS
    g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
#endif

    if (!DIAG_IS_CONN_ON) {
        return 0;
    } else {
        diag_ConnReset();
        diag_CfgResetAllSwt();

        /* 删除定时器 */
        diag_StopMntnTimer();

        diag_StopTransCnfTimer();

        mdrv_hds_printlog_disconn();

        mdrv_hds_translog_disconn();

        /* 将状态发送给C核 */
        (VOS_VOID)diag_SendMsg(MSP_PID_DIAG_APP_AGENT, MSP_PID_DIAG_AGENT, ID_MSG_DIAG_HSO_DISCONN_IND, VOS_NULL, 0);

        mdrv_socp_send_data_manager(SOCP_CODER_DST_OM_IND, SOCP_DEST_DSM_DISABLE);

        DIAG_NotifyConnState(DIAG_STATE_DISCONN);
    }

    return 0;
}

/*
 * Function Name: diag_DisConnProc
 * Description: 该函数用于处理ConnProcEntry传进来的HSO断开命令
 * Input: pstReq 待处理数据
 * Output: None
 * Return: VOS_UINT32
 */
VOS_UINT32 diag_DisConnProc(VOS_UINT8 *pstReq)
{
    VOS_UINT32 ret;
    DIAG_CMD_HOST_DISCONNECT_CNF_STRU stCnfDisConn = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    msp_diag_frame_info_s *pstDiagHead = VOS_NULL;
    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo = VOS_NULL;

    diag_crit("Receive tool disconnect cmd!\n");

#ifdef DIAG_SEC_TOOLS
    /* 清空鉴权状态 */
    g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
#endif

    pstDiagHead = (msp_diag_frame_info_s *)pstReq;

    if (pstDiagHead->ulMsgLen < sizeof(msp_diag_data_head_s)) {
        diag_error("rev tool msg len error:0x%x\n", pstDiagHead->ulMsgLen);
        return ERR_MSP_INALID_LEN_ERROR;
    }

    mdrv_diag_ptr(EN_DIAG_PTR_MSGMSP_DISCONN_IN, 1, pstDiagHead->ulCmdId, 0);

    /* 重置所有开关状态为未打开 */
    diag_ConnReset();
    diag_CfgResetAllSwt();

    /* 删除定时器 */
    diag_StopMntnTimer();

    diag_StopTransCnfTimer();

    DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);

    mdrv_hds_printlog_disconn();

    mdrv_hds_translog_disconn();

    mdrv_socp_send_data_manager(SOCP_CODER_DST_OM_IND, SOCP_DEST_DSM_DISABLE);

    DIAG_NotifyConnState(DIAG_STATE_DISCONN);

    return ret;

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnfDisConn, pstDiagHead);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;

    stCnfDisConn.ulRc = ret;

    ret = DIAG_MsgReport(&stDiagInfo, &stCnfDisConn, (VOS_UINT32)sizeof(stCnfDisConn));

    diag_error("diag disconnect failed.\n");

    return ret;
}
#ifdef DIAG_SEC_TOOLS
VOS_VOID diag_ConnAuth(DIAG_CONNECT_FRAME_INFO_STRU *pstDiagHead)
{
    VOS_UINT32 ulRet = 0;

    g_ulAuthState = DIAG_AUTH_TYPE_AUTHING;

    /* 鉴权消息DIAG 三级头+ 鉴权key */
    if ((pstDiagHead->ulMsgLen < sizeof(msp_diag_data_head_s)) || (pstDiagHead->ulMsgLen > DIAG_FRAME_SUM_LEN)) {
        diag_error("rev msglen is too small, len:0x%x\n", pstDiagHead->ulMsgLen);
        g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
        (VOS_VOID)diag_FailedCmdCnf((msp_diag_frame_info_s *)pstDiagHead, ERR_MSP_INALID_LEN_ERROR);
        return;
    }

    ulRet = diag_SendMsg(MSP_PID_DIAG_APP_AGENT, MSP_PID_DIAG_AGENT, DIAG_MSG_MSP_AUTH_REQ, pstDiagHead->aucData,
                         pstDiagHead->ulMsgLen);
    if (ulRet) {
        g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
        diag_FailedCmdCnf((msp_diag_frame_info_s *)pstDiagHead, ulRet);
    }

    return;
}
VOS_VOID diag_ConnAuthRst(MsgBlock *pMsgBlock)
{
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {};
    DIAG_CMD_AUTH_CNF_STRU stCmdAuthCnf = {};
    DIAG_DATA_MSG_STRU *pstMsg = (DIAG_DATA_MSG_STRU *)pMsgBlock;
    DIAG_AUTH_CNF_STRU *pstAuthRst = NULL;

    /* 不是在鉴权中 */
    if (g_ulAuthState != DIAG_AUTH_TYPE_AUTHING) {
        diag_error("no auth req, g_ulAuthState:0x%x\n", g_ulAuthState);
        return;
    }

    if (pstMsg->ulLength < sizeof(DIAG_DATA_MSG_STRU) - VOS_MSG_HEAD_LENGTH + sizeof(DIAG_AUTH_CNF_STRU)) {
        diag_error("rev datalen is too small,len:0x%x\n", pstMsg->ulLength);
        return;
    }

    pstAuthRst = (DIAG_AUTH_CNF_STRU *)(pstMsg->pContext);

    /* success */
    if (pstAuthRst->ulRet == ERR_MSP_SUCCESS) {
        g_ulAuthState = DIAG_AUTH_TYPE_SUCCESS;

        if (!DIAG_IS_POLOG_ON) {
            mdrv_socp_send_data_manager(SOCP_CODER_DST_OM_IND, SOCP_DEST_DSM_ENABLE);
        }
        diag_crit("tools auth success\n");
    } else {
        diag_error("tools auth fail, ret:0x%x\n", pstAuthRst->ulRet);
    }

    /* CNF to tools */
    stCmdAuthCnf.ulAuid = pstAuthRst->stMdmInfo.ulAuid;
    stCmdAuthCnf.ulSn = pstAuthRst->stMdmInfo.ulSn;
    stCmdAuthCnf.ulRc = pstAuthRst->ulRet;

    stDiagInfo.ulSSId = DIAG_SSID_CPU;
    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;
    stDiagInfo.ulMode = DIAG_MODE_LTE;
    stDiagInfo.ulSubType = DIAG_MSG_CMD;
    stDiagInfo.ulDirection = DIAG_MT_CNF;
    stDiagInfo.ulModemid = 0;
    stDiagInfo.ulMsgId = DIAG_CMD_HOST_CONNECT;
    stDiagInfo.ulTransId = 0;

    (VOS_VOID)DIAG_MsgReport(&stDiagInfo, (VOS_VOID *)&stCmdAuthCnf, sizeof(stCmdAuthCnf));
    return;
}
#endif
VOS_UINT32 diag_ConnTimerProc(VOS_VOID)
{
    return ERR_MSP_SUCCESS;
}
VOS_UINT32 diag_DisConnMgrProc(VOS_UINT8 *pstReq)
{
    return ERR_MSP_SUCCESS;
}
VOS_UINT32 diag_ConnMgrProc(VOS_UINT8 *pstReq)
{
    return ERR_MSP_SUCCESS;
}
VOS_UINT32 diag_ConnMgrSendFuncReg(VOS_UINT32 ulConnId, VOS_UINT32 ulChannelNum, VOS_UINT32 *ulChannel,
                                   DIAG_SEND_PROC_FUNC pSendFuc)
{
    return ERR_MSP_SUCCESS;
}
VOS_UINT32 diag_ConnCnf (VOS_UINT32  ulConnId,  VOS_UINT32 ulSn, VOS_UINT32 ulCount, mdrv_diag_connect_s *pstResult)
{
    return ERR_MSP_SUCCESS;
}


/*
 * Function Name: diag_CheckModemStatus
 * Description: check modem status is ready or not
 * Input: None
 * Output: None
 * Return: VOS_UINT32
 */
VOS_UINT32 diag_CheckModemStatus(VOS_VOID)
{
    VOS_UINT32 result = 0;

    result = VOS_CheckPidValidity(MSP_PID_DIAG_AGENT);
    if (VOS_PID_AVAILABLE != result) {
        diag_error("LR PID Table is not ready,ulResult=0x%x\n", result);
        return ERR_MSP_UNAVAILABLE;
    }
    return ERR_MSP_SUCCESS;
}

mdrv_diag_conn_state_notify_fun g_ConnStateNotifyFun[DIAG_CONN_STATE_FUN_NUM] = { VOS_NULL, VOS_NULL, VOS_NULL, VOS_NULL,
                                                                          VOS_NULL };

unsigned int mdrv_diag_conn_state_notify_fun_reg(mdrv_diag_conn_state_notify_fun pfun)
{
    VOS_UINT32 ulFunIndex;

    for (ulFunIndex = 0; ulFunIndex < sizeof(g_ConnStateNotifyFun) / sizeof(mdrv_diag_conn_state_notify_fun); ulFunIndex++) {
        if (g_ConnStateNotifyFun[ulFunIndex] == VOS_NULL) {
            g_ConnStateNotifyFun[ulFunIndex] = pfun;
            return ERR_MSP_SUCCESS;
        }
    }

    diag_error("no space to register new func\n");
    return ERR_MSP_NOT_FREEE_SPACE;
}

VOS_VOID DIAG_NotifyConnState(unsigned int state)
{
    VOS_UINT32 ulFunIndex;

    for (ulFunIndex = 0; ulFunIndex < sizeof(g_ConnStateNotifyFun) / sizeof(mdrv_diag_conn_state_notify_fun); ulFunIndex++) {
        if (g_ConnStateNotifyFun[ulFunIndex] != VOS_NULL) {
            g_ConnStateNotifyFun[ulFunIndex](state);
        }
    }
}

VOS_VOID DIAG_ShowConnStateNotifyFun(VOS_VOID)
{
    VOS_UINT32 ulFunIndex;

    for (ulFunIndex = 0; ulFunIndex < sizeof(g_ConnStateNotifyFun) / sizeof(mdrv_diag_conn_state_notify_fun); ulFunIndex++) {
        if (g_ConnStateNotifyFun[ulFunIndex] != VOS_NULL) {
            diag_crit("%d %s\n", ulFunIndex, g_ConnStateNotifyFun[ulFunIndex]);
        }
    }

    return;
}

VOS_VOID diag_StopTransCnfTimer(VOS_VOID)
{
    if (g_TransCnfTimer != VOS_NULL) {
        (VOS_VOID)VOS_StopRelTimer(&g_TransCnfTimer);
    }
}

#define DIAG_NV_IMEI_LEN 15

VOS_UINT32 diag_GetImei(VOS_CHAR szimei[16])
{
    VOS_UINT32 ret;
    VOS_UINT32 uslen;
    VOS_UINT32 subscript = 0;
    VOS_CHAR auctemp[DIAG_NV_IMEI_LEN + 1] = {0};

    uslen = DIAG_NV_IMEI_LEN + 1;

    ret = mdrv_nv_read(0, auctemp, uslen);

    if (ret != 0) {
        return ret;
    } else {
        for (subscript = 0; subscript < uslen; subscript++) {
            *(szimei + subscript) = *(auctemp + subscript) + 0x30; /* 字符转换 */
        }
        szimei[DIAG_NV_IMEI_LEN] = 0;
    }

    return 0;
}
