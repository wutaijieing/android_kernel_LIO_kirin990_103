/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: This file describe GPU virtual dev frequency in low load scenario
 * Create: 2021-1-24
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
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */
#ifndef GPU_VIRTUAL_DEVFREQ_H
#define GPU_VIRTUAL_DEVFREQ_H
#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#include "mali_kbase.h"

#ifdef CONFIG_MALI_BORR
#define VFREQ_OPP_COUNT_MAX	6
#define CORE_MASK_ALL	0x77777777ULL
#endif

struct gpu_virtual_devfreq {
	u64 desired_core_mask;
	u64 current_core_mask;
	struct kbase_devfreq_opp vfreq_table[VFREQ_OPP_COUNT_MAX];
	int num_virtual_opps;
	unsigned long devfreq_min;
	struct work_struct set_coremask_work;
	bool virtual_devfreq_mode;
	bool virtual_devfreq_enable;
	bool virtual_devfreq_support;
	struct mutex lock;
};

extern struct devfreq_dev_profile mali_kbase_devfreq_profile;
int gpu_get_vfreq_num(void);
void gpu_get_vfreq_val(int num, u64 *opp_freq, u64 *core_mask);
struct gpu_virtual_devfreq *kbase_get_virtual_devfreq_data(void);
void mali_kbase_devfreq_set_coremask_worker(struct work_struct *work);
#endif
#endif
