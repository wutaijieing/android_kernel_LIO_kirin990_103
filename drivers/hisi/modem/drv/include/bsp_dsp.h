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

#ifndef __BSP_DSP_H__
#define __BSP_DSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <bsp_shared_ddr.h>
#if defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__) ||defined(__OS_RTOSCK_TVP__) ||defined(__OS_RTOSCK_TSP__)
#include <bsp_dpm.h>
#endif

#define DSP_IMAGE_NAME "DSP"
#define DSP_IMAGE_STATE_OK (0x5312ABCD)

#define SAVE_TCM_BEGIN (0x12345678)
#define SAVE_TCM_END  (0x90abcdef)

struct dsp_pm_om_qos_log {
    unsigned int load_start;
    unsigned int load_end;
    unsigned int msp_store;
    unsigned int dpm_store;
    unsigned int dsp_store;
    unsigned int dsp_store_end;
    unsigned int poweroff;
    unsigned int poweron;
    unsigned int dpm_wait_store_ok;
    unsigned int dpm_restore;
    unsigned int dsp_restore;
    unsigned int msp_restore;
    unsigned int msp_restore_ok;
};

struct dsp_dump_proc_flag
{
    unsigned int dsp_dump_flag;         /* 异常流程中，记录dsp保存tcm的过程标志 */
};

#ifndef CONFIG_DSP
#ifdef __KERNEL__
static inline int his_load_tldsp_image(void)
{
    return 0;
}
#endif

#else
#ifdef __KERNEL__
int his_load_tldsp_image(void);
#endif

int bsp_bbe_load_muti(void);

int bsp_bbe_is_muti_loaded(void);

int bsp_bbe_run(void);

int bsp_bbe_is_run(void);

int bsp_bbe_stop(void);

int bsp_bbe_store(void);

int bsp_bbe_wait_store_ok(void);

int bsp_bbe_restore(void);

int bsp_bbe_wait_restore_ok(void);

int bsp_bbe_clock_enable(void);

int bsp_bbe_clock_disable(void);

int bsp_bbe_unreset(void);

int bsp_bbe_reset(void);

int bsp_bbe_is_unreset(void);

int bsp_bbe_power_on(void);

int bsp_bbe_power_off(void);

int bsp_bbe_is_clock_enable(void);

int bsp_bbe_is_power_on(void);

int bsp_bbe_power_status(void);

int bsp_bbe_power_up(void);

int bsp_bbe_power_down(void);

int bsp_bbe_waiti_enable(void);

void bsp_bbe_refclk_enable(void);

void bsp_bbe_refclk_disable(void);

void bsp_bbe_peri_refclk_enable(void);

void bsp_bbe_peri_refclk_disable(void);

int bsp_bbe_is_tcm_accessible(void);

int bsp_dsp_clock_enable(void);

int bsp_dsp_clock_disable(void);

int bsp_dsp_unreset(void);

int bsp_dsp_reset(void);

int bsp_dsp_pll_status(void);

int bsp_dsp_pll_enable(void);

int bsp_dsp_pll_disable(void);

void bsp_bbe_chose_pll(u32 flag);

int bsp_dsp_dfs_pll_status(void);

int bsp_dsp_dfs_pll_enable(void);

int bsp_dsp_dfs_pll_disable(void);

int bsp_msp_bbe_store(void);

int bsp_msp_bbe_is_stored(void);

int bsp_msp_bbe_restore(void);

int bsp_msp_bbe_is_restored(void);

int bsp_msp_wait_edma_ok(void);

int bsp_msp_is_edma_ok(void);

int bsp_bbe_tcm_accessible(void);

void bsp_dsp_set_pll_div(u32 dsp_pll_freq, u32 req_value);

#if defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__) ||defined(__OS_RTOSCK_TVP__) ||defined(__OS_RTOSCK_TSP__)
int bsp_bbe_dpm_prepare(struct dpm_device *dev);

int bsp_bbe_dpm_suspend_late(struct dpm_device *dev);

int bsp_bbe_dpm_resume_early(struct dpm_device *dev);

int bsp_bbe_dpm_complete(struct dpm_device *dev);
#endif

int bsp_dsp_info(void);

int bsp_dsp_store_tcm(void);

#endif

int bsp_load_modem(void);
int bsp_load_modem_dsp(void);

/* the interface below is just for myself */

#ifdef __cplusplus
}
#endif

#endif

