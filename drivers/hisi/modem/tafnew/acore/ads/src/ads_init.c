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
#include "vos.h"
#include "ads_init.h"
#include "ads_up_link.h"
#include "ads_down_link.h"
#include "ads_debug.h"
#include "acpu_reset.h"
#include "ads_msg_chk.h"


/*
 * 协议栈打印打点方式下的.C文件宏定义
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_INIT_C

extern VOS_MsgHookFunc vos_MsgHook;

#if (FEATURE_ACPU_PHONE_MODE == FEATURE_ON)

VOS_LONG ADS_UL_SetAffinity(VOS_VOID)
{
    cpumask_var_t mask;
    VOS_LONG      ret;
    pid_t         targetPid;

    /* 获取当前线程的Pid */
    targetPid = current->pid;

    if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
        return -ENOMEM;
    }

    /* 获取当前线程的affinity */
    ret = sched_getaffinity(targetPid, mask);
    if (ret == 0) {
        cpumask_clear_cpu(0, mask);

        if (!cpumask_empty(mask)) {
            ret = sched_setaffinity(targetPid, mask);
            if (ret != 0) {
                ADS_ERROR_LOG1(ACPU_PID_ADS_UL, "unable to set cpu affinity", ret);
            }
        }
    } else {
        ADS_ERROR_LOG1(ACPU_PID_ADS_UL, "unable to get cpu affinity", ret);
    }

    free_cpumask_var(mask);

    return ret;
}
#endif

VOS_UINT32 ADS_UL_PidInit(enum VOS_InitPhaseDefine phase)
{
    switch (phase) {
        case VOS_IP_LOAD_CONFIG:

            /* 上下文初始化 */
            ADS_InitCtx();

            /* 给低软注册回调函数，用于C核单独复位的处理 */
            mdrv_sysboot_register_reset_notify(NAS_ADS_UL_FUNC_PROC_NAME, ADS_UL_CCpuResetCallback, 0,
                                               ACPU_RESET_PRIORITY_ADS_UL);

            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_VOID ADS_UL_MsgDistr(MsgBlock *msg)
{
    if (VOS_GET_RECEIVER_ID(msg) == ACPU_PID_ADS_UL) {
        ADS_UL_ProcMsg(msg);
    } else if (VOS_GET_RECEIVER_ID(msg) == ACPU_PID_RNIC) {
        RNIC_ProcMsg(msg);
    } else {

    }
}

VOS_VOID ADS_UL_FidTask(VOS_UINT32 queueID, VOS_UINT32 fidValue, VOS_UINT32 para1, VOS_UINT32 para2)
{
    MsgBlock  *msg         = VOS_NULL_PTR;
    VOS_UINT32 event       = 0;
    VOS_UINT32 taskID      = 0;
    VOS_UINT32 rtn         = VOS_ERR;
    VOS_UINT32 eventMask   = 0;
    VOS_UINT32 expectEvent = 0;

    taskID = VOS_GetCurrentTaskID();
    if (taskID == PS_NULL_UINT32) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_FidTask: ERROR, TaskID is invalid.");
        return;
    }

    if (VOS_CreateEvent(taskID) != VOS_OK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_FidTask: ERROR, create event fail.");
        return;
    }

#if (FEATURE_ACPU_PHONE_MODE == FEATURE_ON)
    ADS_UL_SetAffinity();
#endif

    g_adsULTaskId        = taskID;
    g_adsULTaskReadyFlag = 1;

    expectEvent = ADS_UL_EVENT_DATA_PROC | VOS_MSG_SYNC_EVENT;
    eventMask   = (VOS_EVENT_ANY | VOS_EVENT_WAIT);

    for (;;) {
        rtn = VOS_EventRead(expectEvent, eventMask, 0, &event);
        if (rtn != VOS_OK) {
            ADS_ERROR_LOG(ACPU_PID_ADS_UL, "ADS_UL_FidTask: ERROR, read event error.");
            continue;
        }

        /* 事件处理 */
        if (event != VOS_MSG_SYNC_EVENT) {
            ADS_UL_ProcEvent(event);
            continue;
        }

        msg = (MsgBlock *)VOS_GetMsg(taskID);
        if (msg != VOS_NULL_PTR) {
            if (vos_MsgHook != VOS_NULL_PTR) {
                (VOS_VOID)(vos_MsgHook)((VOS_VOID *)(msg));
            }

            ADS_UL_MsgDistr(msg);

            PS_FREE_MSG(VOS_GET_RECEIVER_ID(msg), msg);  //lint !e449
        }
    }
}

VOS_UINT32 ADS_UL_FidInit(enum VOS_InitPhaseDefine ip)
{
    VOS_UINT32 rslt;

    switch (ip) {
        case VOS_IP_LOAD_CONFIG:

            /* 注册ADS_UL PID */
            rslt = VOS_RegisterPIDInfo(ACPU_PID_ADS_UL, (InitFunType)ADS_UL_PidInit, (MsgFunType)ADS_UL_ProcMsg);

            if (rslt != VOS_OK) {
                return VOS_ERR;
            }

            /* 注册RNIC PID */
            rslt = VOS_RegisterPIDInfo(ACPU_PID_RNIC, (InitFunType)RNIC_PidInit, (MsgFunType)RNIC_ProcMsg);

            if (rslt != VOS_OK) {
                return VOS_ERR;
            }

            rslt = VOS_RegisterMsgTaskEntry(ACPU_FID_ADS_UL, ADS_UL_FidTask);

            if (rslt != VOS_OK) {
                return rslt;
            }

            /* 任务优先级 */
            rslt = VOS_RegisterTaskPrio(ACPU_FID_ADS_UL, ADS_UL_TASK_PRIORITY);
            if (rslt != VOS_OK) {
                return VOS_ERR;
            }

            TAF_SortAdsUlFidChkMsgLenTbl();
            break;

        case VOS_IP_FARMALLOC:
        case VOS_IP_INITIAL:
        case VOS_IP_ENROLLMENT:
        case VOS_IP_LOAD_DATA:
        case VOS_IP_FETCH_DATA:
        case VOS_IP_STARTUP:
        case VOS_IP_RIVAL:
        case VOS_IP_KICKOFF:
        case VOS_IP_STANDBY:
        case VOS_IP_BROADCAST_STATE:
        case VOS_IP_RESTART:
        case VOS_IP_BUTT:
            break;
    }

    return VOS_OK;
}

VOS_UINT32 ADS_DL_PidInit(enum VOS_InitPhaseDefine phase)
{
    switch (phase) {
        case VOS_IP_LOAD_CONFIG:


            /* ADQ初始化 */
            ADS_DL_AllocMemForAdq();

            /* 给低软注册回调函数，用于C核单独复位的处理 */
            mdrv_sysboot_register_reset_notify(NAS_ADS_DL_FUNC_PROC_NAME, ADS_DL_CCpuResetCallback, 0,
                                               ACPU_RESET_PRIORITY_ADS_DL);

            break;

        default:
            break;
    }

    return VOS_OK;
}

VOS_VOID ADS_DL_FidTask(VOS_UINT32 queueID, VOS_UINT32 fidValue, VOS_UINT32 para1, VOS_UINT32 para2)
{
    MsgBlock  *msg         = VOS_NULL_PTR;
    VOS_UINT32 event       = 0;
    VOS_UINT32 taskID      = 0;
    VOS_UINT32 rtn         = PS_FAIL;
    VOS_UINT32 eventMask   = 0;
    VOS_UINT32 expectEvent = 0;

    taskID = VOS_GetCurrentTaskID();
    if (taskID == PS_NULL_UINT32) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FidTask: ERROR, TaskID is invalid.");
        return;
    }

    if (VOS_CreateEvent(taskID) != VOS_OK) {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FidTask: ERROR, create event fail.");
        return;
    }

    g_adsDlTaskId        = taskID;
    g_adsDlTaskReadyFlag = 1;

    expectEvent = ADS_DL_EVENT_IPF_RD_INT | ADS_DL_EVENT_IPF_ADQ_EMPTY_INT | ADS_DL_EVENT_IPF_FILTER_DATA_PROC |
                  VOS_MSG_SYNC_EVENT;
    eventMask = (VOS_EVENT_ANY | VOS_EVENT_WAIT);

    for (;;) {
        rtn = VOS_EventRead(expectEvent, eventMask, 0, &event);

        if (rtn != VOS_OK) {
            ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FidTask: ERROR, read event error.");
            continue;
        }

        /* 事件处理 */
        if (event != VOS_MSG_SYNC_EVENT) {
            ADS_DL_ProcEvent(event);
            continue;
        }

        msg = (MsgBlock *)VOS_GetMsg(taskID);
        if (msg != VOS_NULL_PTR) {
            if (vos_MsgHook != VOS_NULL_PTR) {
                (VOS_VOID)(vos_MsgHook)((VOS_VOID *)(msg));
            }

            if (VOS_GET_RECEIVER_ID(msg) == ACPU_PID_ADS_DL) {
                ADS_DL_ProcMsg(msg);
            }

            PS_FREE_MSG(ACPU_PID_ADS_DL, msg);
            msg = VOS_NULL_PTR;
        }
    }
}

VOS_UINT32 ADS_DL_FidInit(enum VOS_InitPhaseDefine ip)
{
    VOS_UINT32          rslt;
    VOS_INT32           ipfRslt;
    struct mdrv_ipf_ops ipfOps;

    ipfOps.rx_complete_cb = ADS_DL_IpfIntCB;
    ipfOps.adq_empty_cb   = ADS_DL_IpfAdqEmptyCB;

    switch (ip) {
        case VOS_IP_LOAD_CONFIG:

            /* 下行PID初始化 */
            rslt = VOS_RegisterPIDInfo(ACPU_PID_ADS_DL, (InitFunType)ADS_DL_PidInit, (MsgFunType)ADS_DL_ProcMsg);

            if (rslt != VOS_OK) {
                return VOS_ERR;
            }

            rslt = VOS_RegisterMsgTaskEntry(ACPU_FID_ADS_DL, ADS_DL_FidTask);

            if (rslt != VOS_OK) {
                return rslt;
            }

            /* 调用mdrv_ipf_register_ops注册中断处理函数,以及AD空中断处理函数 */
            ipfRslt = mdrv_ipf_register_ops(&ipfOps);
            if (ipfRslt != IPF_SUCCESS) {
                return VOS_ERR;
            }

            /* 任务优先级 */
            rslt = VOS_RegisterMsgTaskPrio(ACPU_FID_ADS_DL, VOS_PRIORITY_P6);
            if (rslt != VOS_OK) {
                return VOS_ERR;
            }

            TAF_SortAdsDlFidChkMsgLenTbl();
            break;

        case VOS_IP_FARMALLOC:
        case VOS_IP_INITIAL:
        case VOS_IP_ENROLLMENT:
        case VOS_IP_LOAD_DATA:
        case VOS_IP_FETCH_DATA:
        case VOS_IP_STARTUP:
        case VOS_IP_RIVAL:
        case VOS_IP_KICKOFF:
        case VOS_IP_STANDBY:
        case VOS_IP_BROADCAST_STATE:
        case VOS_IP_RESTART:
        case VOS_IP_BUTT:
            break;
    }

    return VOS_OK;
}

