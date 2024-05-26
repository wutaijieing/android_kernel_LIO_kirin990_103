/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: This file describe HISI GPU DPM&PCR feature.
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2019-10-1
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef MALI_KBASE_DPM_H
#define MALI_KBASE_DPM_H
#include "mali_kbase.h"

/*
 * register config Difference of mali_trym,mali_borr.
 * note: mali_trym do not need read and report top acc_energy info
 */
#if defined CONFIG_MALI_TRYM
#define DPM_GPU_TOP_OFFSET          (0x0400)
#define DPM_GPU_CORE_OFFSET         (0x0600)
#define DPM_GPU_CORE_OFFSET_STEP    (0x0200)
#elif defined CONFIG_MALI_BORR
#define DPM_GPU_TOP_OFFSET          (0x0400)
#define DPM_GPU_CORE_OFFSET         (0x1000)
#define DPM_GPU_CORE_OFFSET_STEP    (0x0200)
#endif

/*
 * dpm register addr and offset;
 * case1: top ENERGY_ADDR = DPM_ACC_ENERGY_ADDR + DPM_GPU_TOP_OFFSET
 * case2: core_x ENERGY_ADDR =
 * DPM_ACC_ENERGY_ADDR + DPM_GPU_CORE_OFFSET + DPM_GPU_CORE_OFFSET_STEP * x
 */

#define DPM_POWER_PARAM0_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x060))
#define DPM_VOLT_PARAM_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0A4))
#define DPM_FREQ_PARAM_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0C4))
#define DPM_SIG_LEV_MODE_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0E4))
#define DPM_SIGNAL_INVERSE_EN_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0EC))
#define DPM_CLK_INVERSE_EN_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0F8))
#define DPM_SENSOR_EN_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x008))
#define DPM_SOFT_SAMPLE_PULSE_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x0E8))
#define DPM_CTRL_EN_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x000))
#define DPM_BUSY_CNT0_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x014))
#define DPM_ACC_ENERGY_ADDR(DPM_GPU_BASE_VIR_ADDR) \
		((DPM_GPU_BASE_VIR_ADDR) + (0x054))

#define DPM_PCTRL_SOFT_RST_GPU_ADDR(PCTRL_BASE_VIR_ADDR) \
		((PCTRL_BASE_VIR_ADDR) + (0x260))
#define DPM_PCTRL_CLK_EN_GPU_ADDR(PCTRL_BASE_VIR_ADDR) \
		((PCTRL_BASE_VIR_ADDR) + (0x25C))
#define DPM_PCTRL_VOLT_FREQ_SEL_ADDR(PCTRL_BASE_VIR_ADDR) \
		((PCTRL_BASE_VIR_ADDR) + (0x25C))

#define DPM_SIG_AND_CONST_POWER_PARA_NUM  16
#define DPM_VOLT_AND_FREQ_PARA_NUM  8
#define DPM_BUSY_COUNT_NUM  16
#define DPM_ACC_ENERGY_NUM  1
#define DPM_BUSY_COUNT_BUF_SIZE   ((DPM_BUSY_COUNT_NUM) * (GPU_DPM_NUM_MAX + 1))
#define NUM_OF_BYTES  4

#define VOLT_SEL  6U
#define FREQ_SEL  6U

#define ALL_DPM_NO_RESET 0
#define ALL_DPM_CLK_DISABLE 0

#if defined CONFIG_MALI_TRYM
#define GPU_DPM_NUM_MAX  8
#define ALL_DPM_RESET 0xFF
#define ALL_DPM_CLK_ENABLE 0xFF
#define POWER_PARAM 0x003000
#define SYNC_CG_OFF_ENABLE 0x00020000
#define VOLT_PARAM  0x300
#define FREQ_PARAM  0x300

/* REPORT_CORE_MASK represents which dpm cores are choosed to report
 * trym platform has 8 core dpms, 1 top dpm
 * bit 0 represents top dpm
 * bit 1~8 represent core dpm 1~8
 * set bitx 1 when you want to enable this dpm
 */
#define REPORT_CORE_MASK 0x0F
#elif defined CONFIG_MALI_BORR
#define GPU_DPM_NUM_MAX  24
#define ALL_DPM_RESET 0x1FFFF
#define ALL_DPM_CLK_ENABLE 0x1FFFF
#define POWER_PARAM 0x003000
#define SYNC_CG_OFF_ENABLE 0x00020000
#define VOLT_PARAM  0x300
#define FREQ_PARAM  0x300

/* REPORT_CORE_MASK represents which dpm cores are choosed to report
 * borr platform has 24 core dpms, 1 top dpm
 * bit 0 represents top dpm
 * bit 1~24 represent core dpm 1~24
 * set bitx 1 when you want to enable this dpm
 */
#define REPORT_CORE_MASK 0x00FF
#endif

#define SIG_LEV_MODE_TOGGLE  0
#define SIG_LEV_MODE_LEVEL   0xFFFF
#define ALL_MONITOR_SENSOR_ENABLE 0xFFFF
#define MONITOR_CTRL_ENABLE  0x00010001
#define MONITOR_CTRL_DISABLE 0x00010000
#define SOFT_SAMPLE_PULSE    0x1
#define CORE_FREQ_SEL_OFFSET 25
#define CORE_VOLT_SEL_OFFSET 28
#define TOP_FREQ_SEL_OFFSET  28
#define PULSE_DELAY_TIME  1

/* time for hrtimer */
#define GPU_DPM_CHECK_PERIOD  1000 // ms
#define GPU_DPM_TIMER_SEC  (GPU_DPM_CHECK_PERIOD / 1000)
#define GPU_DPM_TIMER_NSEC ((GPU_DPM_CHECK_PERIOD % 1000) * 1000000)

/* util method for register bit set */
static inline void gpu_set_bits(unsigned int mask, volatile void __iomem *addr)
{
	(*(volatile unsigned int *) (addr)) |= mask;
}
static inline void gpu_clr_bits(unsigned int mask, volatile void __iomem *addr)
{
	(*(volatile unsigned int *) (addr)) &= ~(mask);
}

void dpm_pcr_resource_init(struct kbase_device *kbdev);
void dpm_pcr_resource_exit(struct kbase_device *kbdev);
void dpm_pcr_enable(struct kbase_device *kbdev);
void dpm_pcr_disable(struct kbase_device *kbdev);
#endif

