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
 * 1 Include HeadFile
 */
#include "diag_acore_common.h"
#include <linux/debugfs.h>
#include <linux/version.h>
#include <securec.h>
#include <vos.h>
#include <mdrv.h>
#include <mdrv_diag_system.h>
#include <msp.h>
#include <omerrorlog.h>
#include <nv_stru_sys.h>
#include <soc_socp_adapter.h>
#include <eagleye.h>
#include "diag_common.h"
#include "diag_msgbsp.h"
#include "diag_msgbbp.h"
#include "diag_msg_easyrf.h"
#include "diag_msgphy.h"
#include "diag_msgps.h"
#include "diag_msghifi.h"
#include "diag_msgapplog.h"
#include "diag_api.h"
#include "diag_cfg.h"
#include "diag_debug.h"
#include "diag_connect.h"
#include "diag_msglrm.h"
#include "diag_msgnrm.h"
#include "diag_msgapplog.h"
#include "diag_time_stamp.h"
#include "diag_message.h"
#include "diag_msg_def.h"
#include "diag_time_stamp.h"


/*
 * 2 Declare the Global Variable
 */

#define THIS_FILE_ID MSP_FILE_ID_DIAG_ACORE_COMMON_C

drv_reset_cb_moment_e g_DiagResetingCcore = MDRV_RESET_CB_INVALID;

VOS_UINT32 g_ulDebugCfg = 0;

DIAG_DUMP_INFO_STRU g_stDumpInfo = {0};

extern DIAG_TRANS_HEADER_STRU g_stPSTransHead;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
struct wakeup_source *diag_wakelock;
#else
struct wakeup_source diag_wakelock;
#endif

/*
 * 3 Function
 */

extern VOS_VOID SCM_StopAllSrcChan(VOS_VOID);

/*
 * Function Name: diag_ResetCcoreCB
 * Description: 诊断modem单独复位回调函数
 * Input: enParam
 * Output: None
 * Return: VOS_VOID
 */
VOS_INT diag_ResetCcoreCB(drv_reset_cb_moment_e enParam, int userdata)
{
    diag_trans_ind_s stResetMsg = {};
    VOS_UINT32 ret = ERR_MSP_SUCCESS;

    if (enParam == MDRV_RESET_CB_BEFORE) {
        diag_crit("Diag receive ccore reset Callback.\n");

        g_DiagResetingCcore = MDRV_RESET_CB_BEFORE;

        diag_SetPhyPowerOnState(DIAG_PHY_POWER_ON_DOING);

        if (!DIAG_IS_CONN_ON) {
            return ERR_MSP_SUCCESS;
        }

        stResetMsg.module = DRV_DIAG_GEN_MODULE_EX(DIAG_MODEM_0, DIAG_MODE_LTE, DIAG_MSG_TYPE_MSP);
        stResetMsg.pid = MSP_PID_DIAG_APP_AGENT;
        stResetMsg.msg_id = DIAG_CMD_MODEM_WILL_RESET;
        stResetMsg.reserved = 0;
        stResetMsg.length = sizeof(VOS_UINT32);
        stResetMsg.data = (void *)&ret;

        ret = mdrv_diag_report_reset_msg(&stResetMsg);
        if (ret) {
            diag_error("report reset msg fail, ret:0x%x\n", ret);
        }

        diag_crit("Diag report ccore reset to HIDP,and reset SOCP timer.\n");
        /* modem单独复位时，把中断超时时间恢复为默认值，让HIDP尽快收到复位消息 */
        mdrv_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);
#ifdef DIAG_SEC_TOOLS
        g_ulAuthState = DIAG_AUTH_TYPE_DEFAULT;
#endif
    } else if (enParam == MDRV_RESET_CB_AFTER) {
        g_DiagResetingCcore = MDRV_RESET_CB_AFTER;

    } else {
        diag_error("enParam(0x%x) error\n", enParam);
    }
    return ERR_MSP_SUCCESS;
}


VOS_UINT32 diag_AppAgentMsgProcInit(enum VOS_InitPhaseDefine ip)
{
    VOS_UINT32 ret = ERR_MSP_SUCCESS;
    VOS_CHAR *resetName = "DIAG"; /* C核单独复位的名字 */
    VOS_INT resetLevel = 49;

    if (ip == VOS_IP_LOAD_CONFIG) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        diag_wakelock = wakeup_source_register(NULL, "diag_wakelock");
#else
        wakeup_source_init(&diag_wakelock, "diag_wakelock");
#endif
        ret = (VOS_UINT32)mdrv_sysboot_register_reset_notify(resetName, (pdrv_reset_cbfun)diag_ResetCcoreCB, 0,
                                                             resetLevel);
        if (ret) {
            diag_error("diag register ccore reset fail\n");
        }

        diag_MspMsgInit();
        diag_BspMsgInit();
        diag_DspMsgInit();
        diag_EasyRfMsgInit();
        diag_BbpMsgInit();
        diag_PsMsgInit();
        diag_HifiMsgInit();
        diag_MessageInit();
        diag_AppLogMsgInit();
        diag_LRMMsgInit();
        diag_NrmMsgInit();
    } else if (ip == VOS_IP_RESTART) {
        diag_ConnReset();

        /* 都初始化结束后再设置开关 */
        diag_CfgResetAllSwt();
        /* 检测是否需要打开开机log */
        mdrv_scm_set_power_on_log();

        if (VOS_TRUE == diag_IsPowerOnLogOpen()) {
            /* 在打开开机log前，推送一个时间戳高位给工具 */
            diag_PushHighTs();
            diag_PowerOnLogHighTsTimer();
            g_ulDiagCfgInfo |= DIAG_CFG_POWERONLOG;
        }

        mdrv_scm_reg_ind_coder_dst_send_fuc();

        diag_crit("Diag PowerOnLog is %s.\n", (g_ulDiagCfgInfo & DIAG_CFG_POWERONLOG) ? "open" : "close");
    }

    return ret;
}


VOS_VOID diag_DumpMsgInfo(VOS_UINT32 ulSenderPid, VOS_UINT32 ulMsgId, VOS_UINT32 ulSize)
{
    VOS_UINT32 ulPtr = g_stDumpInfo.ulMsgCur;

    if (g_stDumpInfo.pcMsgAddr) {
        *((VOS_UINT32 *)(&g_stDumpInfo.pcMsgAddr[ulPtr])) = ulSenderPid;
        ulPtr = ulPtr + sizeof(VOS_UINT32);
        *((VOS_UINT32 *)(&g_stDumpInfo.pcMsgAddr[ulPtr])) = ulMsgId;
        ulPtr = ulPtr + sizeof(VOS_UINT32);
        *((VOS_UINT32 *)(&g_stDumpInfo.pcMsgAddr[ulPtr])) = ulSize;
        ulPtr = ulPtr + sizeof(VOS_UINT32);
        *((VOS_UINT32 *)(&g_stDumpInfo.pcMsgAddr[ulPtr])) = mdrv_timer_get_normal_timestamp();

        g_stDumpInfo.ulMsgCur = (g_stDumpInfo.ulMsgCur + 16);
        if (g_stDumpInfo.ulMsgCur >= g_stDumpInfo.ulMsgLen) {
            g_stDumpInfo.ulMsgCur = 0;
        }
    }
}

/* DUMP存储的消息的最大长度，其中64表示包含0xaa5555aa、帧头、消息内容的总的最大长度 */
#define DIAG_DUMP_MAX_FRAME_LEN (80)

/*
 * Function Name: diag_DumpDFInfo
 * Description: 保存A核最后收到的码流信息，每条码流的保存长度不超过100字节
 * Input:
 * 注意事项:
 *    不支持重入，由调用者保证不会重复进入
 */
VOS_VOID diag_DumpDFInfo(msp_diag_frame_info_s *pFrame)
{
    VOS_UINT32 ulPtr;
    VOS_UINT32 ulLen;
    errno_t err = EOK;

    if (g_stDumpInfo.pcDFAddr) {
        ulPtr = g_stDumpInfo.ulDFCur;

        *((VOS_INT32 *)(&g_stDumpInfo.pcDFAddr[ulPtr])) = (VOS_INT32)0xaa5555aa;
        *((VOS_INT32 *)(&g_stDumpInfo.pcDFAddr[ulPtr + sizeof(VOS_UINT32)])) = mdrv_timer_get_normal_timestamp();

        /* 每组数据都是16字节对齐，不会回卷 */
        g_stDumpInfo.ulDFCur = g_stDumpInfo.ulDFCur + 8;

        ulPtr = g_stDumpInfo.ulDFCur;

        ulLen = 8 + sizeof(msp_diag_frame_info_s) + pFrame->ulMsgLen;
        if (ulLen > DIAG_DUMP_MAX_FRAME_LEN) {
            ulLen = DIAG_DUMP_MAX_FRAME_LEN;
        }

        ulLen = ((ulLen + 0xf) & (~0xf)) - 8;

        if ((ulPtr + ulLen) <= g_stDumpInfo.ulDFLen) {
            err = memcpy_s(&g_stDumpInfo.pcDFAddr[ulPtr], g_stDumpInfo.ulDFLen - ulPtr, (VOS_VOID *)pFrame,
                                   ulLen);
            if (err != EOK) {
                diag_error("memcpy fail:%x\n", err);
            }

            /* 可能为0，需要取余 */
            g_stDumpInfo.ulDFCur = (g_stDumpInfo.ulDFCur + ulLen) % g_stDumpInfo.ulDFLen;
        } else {
            err = memcpy_s(&g_stDumpInfo.pcDFAddr[ulPtr], g_stDumpInfo.ulDFLen - ulPtr, (VOS_VOID *)pFrame,
                                   (g_stDumpInfo.ulDFLen - ulPtr));
            if (err != EOK) {
                diag_error("memcpy fail:%x\n", err);
            }

            ulLen = ulLen - (g_stDumpInfo.ulDFLen - ulPtr); /* 未拷贝的码流长度 */

            err = memcpy_s(&g_stDumpInfo.pcDFAddr[0], ulPtr,
                                   (((VOS_UINT8 *)pFrame) + (g_stDumpInfo.ulDFLen - ulPtr)), ulLen);
            if (err != EOK) {
                diag_error("memcpy fail:%x\n", err);
            }

            /* ulLen前面已经做了限制，不会回卷 */
            g_stDumpInfo.ulDFCur = ulLen;
        }
    }
}

VOS_VOID diag_ApAgentMsgProc(MsgBlock *pMsgBlock)
{
    DIAG_DATA_MSG_STRU *pMsgTmp = VOS_NULL;

    COVERITY_TAINTED_SET((VOS_VOID *)(pMsgBlock->value));

    pMsgTmp = (DIAG_DATA_MSG_STRU *)pMsgBlock;

    switch (pMsgTmp->ulMsgId) {
        case ID_MSG_DIAG_HSO_DISCONN_IND:
            (VOS_VOID)diag_SetChanDisconn(pMsgBlock);
            break;

        case ID_MSG_DIAG_CMD_CONNECT_REQ:
            (VOS_VOID)diag_ConnMgrProc(pMsgTmp->pContext);
            break;
        case ID_MSG_DIAG_CMD_DISCONNECT_REQ:
            (VOS_VOID)diag_DisConnMgrProc(pMsgTmp->pContext);
            break;

        default:
            break;
    }
}

VOS_VOID diag_TimerMsgProc(MsgBlock *pMsgBlock)
{
    REL_TimerMsgBlock *pTimer = NULL;


    pTimer   = (REL_TimerMsgBlock*)pMsgBlock;

    if((DIAG_DEBUG_TIMER_NAME == pTimer->name) && (DIAG_DEBUG_TIMER_PARA == pTimer->para)){
        diag_ReportMntn();
    } else if((DIAG_CONNECT_TIMER_NAME == pTimer->name) && (DIAG_CONNECT_TIMER_PARA == pTimer->para)){
        (VOS_VOID)diag_ConnTimerProc();
    } else if((DIAG_HIGH_TS_PUSH_TIMER_NAME == pTimer->name) && (DIAG_HIGH_TS_PUSH_TIMER_PARA == pTimer->para)){
        /* 推送时间戳高位 */
        diag_PushHighTs();
    }

    else if((DIAG_FLOW_ACORE_TIMER_NAME == pTimer->name) && (DIAG_ACORE_FLOW_TIMER_PARA == pTimer->para)){
        /* 推送时间戳高位 */
        DIAG_ACoreFlowPress();
    } else {
        ;
    }

    return;
}
/*
 * Function Name: diag_AppAgentMsgProc
 * Description: DIAG APP AGENT接收到的消息处理入口
 * Input: MsgBlock* pMsgBlock
 * 注意事项:
 *    由于errorlog的消息不能识别发送PID，所以需要进入errorlog的处理函数中检查
 *    当已知消息被成功处理时，则不需要再进行errorlog的消息检查
 *    通过ulErrorLog的值判断是否进行errorlog的消息检查
 *    后续函数扩展时需要注意
 */
VOS_VOID diag_AppAgentMsgProc(MsgBlock *pMsgBlock)
{
    VOS_UINT32 ulErrorLog = ERR_MSP_CONTINUE; /* 见函数头中的注意事项的描述 */

    /* 入参判断 */
    if (NULL == pMsgBlock) {
        return;
    }

    /* 任务开始处理，不允许睡眠 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_stay_awake(diag_wakelock);
#else
    __pm_stay_awake(&diag_wakelock);
#endif

    diag_DumpMsgInfo(pMsgBlock->ulSenderPid, (*(VOS_UINT32*)pMsgBlock->value), pMsgBlock->ulLength);

    /* 根据发送PID，执行不同处理 */
    switch (pMsgBlock->ulSenderPid) {
        /* 超时消息，按照超时包格式，打包回复 */
        case DOPRA_PID_TIMER:
            diag_TimerMsgProc(pMsgBlock);
            ulErrorLog = VOS_OK;
            break;

        case MSP_PID_DIAG_APP_AGENT:
            diag_ApAgentMsgProc(pMsgBlock);
            ulErrorLog = VOS_OK;
            break;
        /*lint -save -e826*/
        case MSP_PID_DIAG_AGENT:
            diag_LrmAgentMsgProc(pMsgBlock);
            break;
        case MSP_PID_DIAG_NRM_AGENT:
            diag_NrmAgentMsgProc(pMsgBlock);
            break;
        /*lint -restore +e826*/
        case DSP_PID_APM:
            ulErrorLog = ERR_MSP_CONTINUE;
            break;

        case DSP_PID_STARTUP: {
            diag_crit("receive DSP_PID_STARTUP msg\n");
            diag_GuPhyMsgProc(pMsgBlock);
        } break;


        default:
            break;
    }

    /* 任务开始结束，允许睡眠 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    __pm_relax(diag_wakelock);
#else
    __pm_relax(&diag_wakelock);
#endif
    if (ulErrorLog != VOS_OK && ulErrorLog != ERR_MSP_CONTINUE) {
        diag_error("handle msg err,sender:%x, err:%x\n", pMsgBlock->ulSenderPid, ulErrorLog);
    }

    return;
}

/*
 * Function Name: diag_TransReqProcEntry
 * Description: 该函数用于透传命令的REQ处理
 * Input: VOS_UINT8* pstReq
 * Output: None
 * Return: VOS_UINT32
 */
VOS_UINT32 diag_TransReqProcEntry(msp_diag_frame_info_s *pstReq)
{
    VOS_UINT32 ret = ERR_MSP_FAILURE;
    VOS_UINT32 ulCmdParasize;
    DIAG_TRANS_MSG_STRU *pstSendReq = NULL;
    diag_osa_msg_head_s *pstMsg = NULL;
    DIAG_COMM_CNF_STRU stCnfMsg = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};

    mdrv_diag_ptr(EN_DIAG_PTR_MSGMSP_TRANS, 1, pstReq->ulCmdId, 0);

    if (pstReq->ulMsgLen < sizeof(msp_diag_data_head_s) + sizeof(DIAG_TRANS_MSG_STRU) - sizeof(pstSendReq->para)) {
        diag_error("rev datalen error, 0x%x\n", pstReq->ulMsgLen);
        ret = ERR_MSP_DIAG_CMD_SIZE_INVALID;
        goto DIAG_ERROR;
    }

    /* 打包透传数据 */
    pstSendReq = (DIAG_TRANS_MSG_STRU *)(pstReq->aucData + sizeof(msp_diag_data_head_s));

    diag_LNR(EN_DIAG_LNR_PS_TRANS, pstReq->ulCmdId, VOS_GetSlice());

    if (VOS_PID_AVAILABLE != VOS_CheckPidValidity(pstSendReq->ulReceiverPid)) {
        diag_error("rev pid:0x%x invalid\n", pstSendReq->ulReceiverPid);
        ret = ERR_MSP_INVALID_ID;
        goto DIAG_ERROR;
    }

    if (DIAG_DEBUG_TRANS & g_ulDebugCfg) {
        diag_crit("trans req: cmdid 0x%x, pid %d, msgid 0x%x.\n", pstReq->ulCmdId, pstSendReq->ulReceiverPid,
                  pstSendReq->msgId);
    }
    
    ulCmdParasize = pstReq->ulMsgLen - sizeof(msp_diag_data_head_s);
    if (ulCmdParasize > DIAG_MEM_ALLOC_LEN_MAX) {
        diag_error("ERROR: ulMsgId=0x%x, ulCmdParasize = 0x%x.\n", pstReq->ulCmdId, ulCmdParasize);
        ret = ERR_MSP_INALID_LEN_ERROR;
        goto DIAG_ERROR;
    }
    
    pstMsg = (diag_osa_msg_head_s *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT, (ulCmdParasize - VOS_MSG_HEAD_LENGTH));
    if (pstMsg != NULL) {
        pstMsg->ulReceiverPid = pstSendReq->ulReceiverPid;

        ret = (VOS_UINT32)memcpy_s(&pstMsg->ulMsgId, ulCmdParasize - VOS_MSG_HEAD_LENGTH, &pstSendReq->msgId,
                                   (ulCmdParasize - VOS_MSG_HEAD_LENGTH));
        if (ret) {
            diag_error("memcpy fail:%x\n", ret);
        }

        ret = VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstMsg);
        if (ret != VOS_OK) {
            diag_error("VOS_SendMsg failed!\n");
        } else {
            ret = ERR_MSP_SUCCESS;
        }
        goto DIAG_ERROR;
    }
    
    ret = ERR_MSP_MALLOC_FAILUE;

DIAG_ERROR:
    stCnfMsg.ulRet = ret;
    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnfMsg, pstReq);

    stDiagInfo.ulMsgType = pstReq->stID.pri4b;

    /* 组包回复 */
    return DIAG_MsgReport(&stDiagInfo, &stCnfMsg, sizeof(stCnfMsg));
}

int diag_debug_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t diag_debug_read(struct file *file, char __user *ubuf, size_t cnt, loff_t *ppos)
{
    printk("usage:\n");
    printk("\t echo cmd [param1] [param2] ... [paramN] > /sys/kernel/debug/modem_diag/diag\n");
    printk("cmd list:\n");
    printk("\t DIAG_ALL --- save all diag debug infomation.\n");
    return 0;
}

static ssize_t diag_debug_write(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
    char buf[128] = {0};
    ssize_t ret = cnt;
    DIAG_A_DEBUG_C_REQ_STRU *pstFlag = NULL;
    VOS_UINT32 ulret = ERR_MSP_FAILURE;

    if ((!ubuf) || (!cnt)) {
        diag_error("input error\n");
        return 0;
    }

    cnt = (cnt > 127) ? 127 : cnt;
    if (copy_from_user(buf, ubuf, cnt)) {
        diag_error("copy from user fail\n");
        ret = -EFAULT;
        goto out;
    }
    buf[cnt] = 0;

    /* 配置异常目录份数 */
    if (0 == strncmp(buf, "DIAG_ALL", strlen("DIAG_ALL"))) {
        DIAG_DebugCommon();
        DIAG_DebugNoIndLog();

        pstFlag = (DIAG_A_DEBUG_C_REQ_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                                          (sizeof(DIAG_A_DEBUG_C_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

        if (pstFlag != NULL) {
            pstFlag->ulReceiverPid = MSP_PID_DIAG_AGENT;

            pstFlag->ulMsgId = DIAG_MSG_MSP_A_DEBUG_C_REQ;
            pstFlag->ulFlag = DIAG_DEBUG_DFR_BIT | DIAG_DEBUG_NIL_BIT;

            ulret = VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstFlag);
            if (ulret != VOS_OK) {
                diag_error("VOS_SendMsg failed!\n");
            }
        }
    }

out:
    return ret;
}

long diag_debug_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static const struct file_operations diag_debug_fops = {
    .open = diag_debug_open,
    .read = diag_debug_read,
    .write = diag_debug_write,
    .unlocked_ioctl = diag_debug_ioctl,
};


VOS_UINT32 MSP_AppDiagFidInit(enum VOS_InitPhaseDefine ip)
{
    VOS_UINT32 ulRelVal = 0;

    switch (ip) {
        case VOS_IP_LOAD_CONFIG:

            mdrv_ppm_reg_disconnect_cb(diag_DisconnectTLPort);

            ulRelVal = VOS_RegisterPIDInfo(MSP_PID_DIAG_APP_AGENT, (InitFunType) diag_AppAgentMsgProcInit,
                                           (MsgFunType) diag_AppAgentMsgProc);

            if (ulRelVal != VOS_OK) {
                return VOS_ERR;
            }

            ulRelVal = VOS_RegisterMsgTaskPrio(MSP_FID_DIAG_ACPU, VOS_PRIORITY_M2);
            if (ulRelVal != VOS_OK) {
                return VOS_ERR;
            }
            /* 申请8K的dump空间 */
            g_stDumpInfo.pcDumpAddr = (VOS_VOID *)mdrv_om_register_field(OM_AP_DIAG, "ap_diag",
                                                                         DIAG_DUMP_LEN, 0);

            if (VOS_NULL != g_stDumpInfo.pcDumpAddr) {
                g_stDumpInfo.pcMsgAddr = g_stDumpInfo.pcDumpAddr;
                g_stDumpInfo.ulMsgCur = 0;
                g_stDumpInfo.ulMsgLen = DIAG_DUMP_MSG_LEN;

                g_stDumpInfo.pcDFAddr = g_stDumpInfo.pcDumpAddr + DIAG_DUMP_MSG_LEN;
                g_stDumpInfo.ulDFCur = 0;
                g_stDumpInfo.ulDFLen = DIAG_DUMP_DF_LEN;
            }

            VOS_RegisterMsgGetHook((VOS_MsgHookFunc)mdrv_diag_layer_msg_report);

            return VOS_OK;
            // break;

        case VOS_IP_RESTART:
            /* ready stat */
            mdrv_ppm_pcdev_ready();
            break;
        default:
            break;
    }

    return VOS_OK;
}

VOS_VOID DIAG_DebugTransOn(VOS_UINT32 ulOn)
{
    if (0 == ulOn) {
        g_ulDebugCfg &= (~DIAG_DEBUG_TRANS);
    } else {
        g_ulDebugCfg |= DIAG_DEBUG_TRANS;
    }
}

/*
 * 函 数 名: diag_DisconnectTLPort
 * 功能描述: TL断开OM端口
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: BSP_ERROR/BSP_OK
 * 修改历史:
 *   1.日    期: 2014年05月26日
 *     作    者: h59254
 *     修改内容: Creat Function
 */
VOS_UINT32 diag_DisconnectTLPort(void)
{
    DIAG_DATA_MSG_STRU *pstMsg;

    pstMsg = (DIAG_DATA_MSG_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                                sizeof(DIAG_DATA_MSG_STRU) - VOS_MSG_HEAD_LENGTH);

    if (NULL == pstMsg) {
        return VOS_ERR;
    }

    pstMsg->ulReceiverPid = MSP_PID_DIAG_APP_AGENT;
    pstMsg->ulMsgId = ID_MSG_DIAG_HSO_DISCONN_IND;

    (void)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstMsg);

    return VOS_OK;
}
EXPORT_SYMBOL(DIAG_DebugTransOn);

