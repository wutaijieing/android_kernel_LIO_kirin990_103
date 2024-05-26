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
#include "ads_timer_mgmt.h"
#include "ads_ctx.h"
#include "ads_down_link.h"
#include "ads_up_link.h"
#include "ads_debug.h"
#include "mn_comm_api.h"


/*
 * 协议栈打印打点方式下的.C文件宏定义
 */
#define THIS_FILE_ID PS_FILE_ID_ADS_TIMERMGMT_C

ADS_TIMER_Operate g_adsTmrOperateTbl[] = {
    { ACPU_PID_ADS_UL, TI_ADS_UL_SEND, VOS_TIMER_NO_PRECISION, {0}, VOS_NULL_PTR },
    { ACPU_PID_ADS_UL, TI_ADS_DSFLOW_STATS, VOS_TIMER_NO_PRECISION, {0}, VOS_NULL_PTR },
    { ACPU_PID_ADS_DL, TI_ADS_DL_ADQ_EMPTY, VOS_TIMER_PRECISION_0, {0}, ADS_DL_RcvTiAdqEmptyExpired },
    { ACPU_PID_ADS_UL, TI_ADS_UL_DATA_STAT, VOS_TIMER_NO_PRECISION, {0}, VOS_NULL_PTR },
    { ACPU_PID_ADS_DL, TI_ADS_DL_CCORE_RESET_DELAY, VOS_TIMER_PRECISION_0, {0}, ADS_DL_RcvTiCcoreResetDelayExpired }
};

VOS_VOID ADS_MntnTraceTimerOperation(VOS_UINT32 pid, ADS_TIMER_IdUint32 timerId, VOS_UINT32 timerLen,
                                     ADS_TIMER_OperationTypeUint8 timerAction, ADS_TIMER_StopCauseUint8 stopCause)
{
    ADS_TIMER_Info msg = {0};

    TAF_CfgMsgHdr((MsgBlock *)(&msg), pid, VOS_PID_TIMER, sizeof(ADS_TIMER_Info) - VOS_MSG_HEAD_LENGTH);

    msg.timerId        = timerId;
    msg.timerLen       = timerLen;
    msg.timerAction    = timerAction;
    msg.timerStopCause = stopCause;
    msg.reserved[0]    = 0;
    msg.reserved[1]    = 0;

    mdrv_diag_trace_report(&msg);
}

VOS_VOID ADS_StartTimer(ADS_TIMER_IdUint32 timerId, VOS_UINT32 len)
{
    ADS_TIMER_Ctx     *tiCtx      = VOS_NULL_PTR;
    ADS_TIMER_Operate *tmrOperate = VOS_NULL_PTR;
    VOS_UINT32         ret;

    /* 不在使用的定时器范围内 */
    if (timerId >= ADS_MAX_TIMER_NUM) {
        ADS_ERROR_LOG1(ACPU_PID_ADS_UL, "ADS_StartTimer: timer id is invalid. <enTimerId>", timerId);
        return;
    }

    /* 获取定时器上下文 */
    tmrOperate = &(g_adsTmrOperateTbl[timerId]);
    tiCtx      = &(g_adsCtx.adsTiCtx[timerId]);

    /* 定时器长度检查 */
    if (len == 0) {
        ADS_ERROR_LOG(tmrOperate->pid, "ADS_StartTimer: timer len is 0,");
        return;
    }

    /* 定时器已运行 */
    if (tiCtx->hTimer != VOS_NULL_PTR) {
        return;
    }

    /* 启动定时器 */
    if (tmrOperate->pfnTimerStartCallBack == VOS_NULL_PTR) {
        ret = VOS_StartRelTimer(&(tiCtx->hTimer), tmrOperate->pid, len, timerId, 0, VOS_RELTIMER_NOLOOP,
                                tmrOperate->precision);
    } else {
        ret = VOS_StartCallBackRelTimer(&(tiCtx->hTimer), tmrOperate->pid, len, timerId, 0, VOS_RELTIMER_NOLOOP,
                                        tmrOperate->pfnTimerStartCallBack, tmrOperate->precision);
    }

    if (ret != VOS_OK) {
        ADS_ERROR_LOG1(tmrOperate->pid, "ADS_StartTimer: timer start failed! <ret>", ret);
        return;
    }

    /* 勾包ADS_TIMER_Info */
    ADS_MntnTraceTimerOperation(tmrOperate->pid, timerId, len, ADS_TIMER_OPERATION_START,
                                ADS_TIMER_STOP_CAUSE_ENUM_BUTT);
}

VOS_VOID ADS_StopTimer(VOS_UINT32 pid, ADS_TIMER_IdUint32 timerId, ADS_TIMER_StopCauseUint8 stopCause)
{
    ADS_TIMER_Ctx *tiCtx = VOS_NULL_PTR;

    /* 不在使用的定时器范围内 */
    if (timerId >= ADS_MAX_TIMER_NUM) {
        ADS_ERROR_LOG1(ACPU_PID_ADS_UL, "ADS_StopTimer: timer id is invalid. <enTimerId>", timerId);
        return;
    }

    /* 获取定时器上下文 */
    tiCtx = &(g_adsCtx.adsTiCtx[timerId]);

    /* 停止定时器 */
    if (tiCtx->hTimer != VOS_NULL_PTR) {
        (VOS_VOID)VOS_StopRelTimer(&(tiCtx->hTimer));
    }

    /* 勾包ADS_TIMER_Info */
    ADS_MntnTraceTimerOperation(pid, timerId, 0, ADS_TIMER_OPERATION_STOP, stopCause);
}

ADS_TIMER_StatusUint8 ADS_GetTimerStatus(VOS_UINT32 pid, ADS_TIMER_IdUint32 timerId)
{
    ADS_TIMER_Ctx *tiCtx = VOS_NULL_PTR;

    /* 不在使用的定时器范围内 */
    if (timerId >= ADS_MAX_TIMER_NUM) {
        return ASD_TIMER_STATUS_BUTT;
    }

    /* 获取定时器上下文 */
    tiCtx = &(g_adsCtx.adsTiCtx[timerId]);

    /* 检查定时器句柄 */
    if (tiCtx->hTimer != VOS_NULL_PTR) {
        return ADS_TIMER_STATUS_RUNNING;
    }

    return ADS_TIMER_STATUS_STOP;
}

void ADS_StartUlSendProtectTimer(void)
{
    if (!timer_pending(&g_adsCtx.ulSendProtectTimer)) {
        mod_timer(&g_adsCtx.ulSendProtectTimer, jiffies + ADS_UL_GET_PROTECT_TIMER_LEN());
        ADS_INFO_LOG(ACPU_PID_ADS_UL, "ADS_StartUlSendProtectTimer");
    }
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
void ADS_UlSendProtectTimerExpire(struct timer_list *t)
#else
void ADS_UlSendProtectTimerExpire(unsigned long data)
#endif
{
    ADS_UL_SndEvent(ADS_UL_EVENT_DATA_PROC);
    ADS_DBG_UL_10MS_TMR_TRIG_EVENT(1);
}

