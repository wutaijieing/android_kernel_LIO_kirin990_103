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

#ifndef __PM_OM_DEBUG_H__
#define __PM_OM_DEBUG_H__

#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/notifier.h>
#include <linux/version.h>
#include <bsp_pm_om.h>
#include "pm_om_nrccpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pm_om_debug_error(fmt, ...) (bsp_err("<%s> " fmt, __FUNCTION__, ##__VA_ARGS__))
#define pm_om_debug_info(fmt, ...) (bsp_info("<%s> " fmt, __FUNCTION__, ##__VA_ARGS__))
#define pm_om_debug_err(fmt, ...) (bsp_err(fmt, ##__VA_ARGS__))
#define PM_OM_PRINT_HRESHOLD_CHECK_SIZE 25
struct pm_om_stat {
    u32 wakeup_cnt;    /* ����acore�Ĵ��� */
    u32 waket_prev;    /* ����acore����һ�ε�ʱ�� */
    u32 waket_min;     /* ����acore�����ʱ���� */
    u32 waket_max;     /* ����acore���ʱ���� */
    u32 logt_max;      /* ���λ���acore��ʱ����ֵ */
    u32 logt_print_sw; /* ��ӡ���� */
    u32 wrs_flag;      /* ����Դ�ֽ�ʱ��ӡ���� */
};

struct pm_om_debug {
    struct pm_om_stat stat;
    struct pm_om_ctrl *ctrl;
    struct notifier_block pm;
    char *cdrx_dump_addr;
    char *cdrx_dump_corepm_addr;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    struct wakeup_source *wakelock_debug;
#else
    struct wakeup_source wakelock_debug;
#endif
    u32 max_log_buffer_size;
};

int pm_om_debug_init(void);
u32 pm_om_log_time_rec(u32 time_log_start);
static inline void pm_om_log_time_print(u32 log_sw)
{
}
extern void hac_dump_init(void);
extern int hacpm_suspend_prepare(void);
extern void print_hac_lowpower_info(void);

#ifdef __cplusplus
}
#endif

#endif /* __PM_OM_DEBUG_H__ */
