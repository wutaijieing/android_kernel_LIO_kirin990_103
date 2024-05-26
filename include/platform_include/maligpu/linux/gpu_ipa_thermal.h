/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Description: This file describe GPU  thermal related data structs
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2021-05-11
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#ifndef _GPU_IPA_THERMAL_H_
#define _GPU_IPA_THERMAL_H_

#ifdef CONFIG_GPU_IPA_THERMAL
int ithermal_get_ipa_gpufreq_limit(unsigned long freq, unsigned long *freq_limit);
struct thermal_cooling_device *ithermal_get_gpu_cdev(void);
void ithermal_gpu_cdev_register(struct device_node *df_node, struct devfreq *df);
void ithermal_gpu_cdev_unregister(void);
#else
static inline int ithermal_get_ipa_gpufreq_limit(unsigned long freq, unsigned long *freq_limit)
{
	(void)freq;
	*freq_limit = 0;
	return 0;
}

static inline struct thermal_cooling_device *ithermal_get_gpu_cdev(void)
{
	return NULL;
}

static inline void ithermal_gpu_cdev_register(struct device_node *df_node,
					      struct devfreq *df)
{
	(void)df_node;
	(void)df;
}

static inline void ithermal_gpu_cdev_unregister(void)
{
}
#endif

#endif /* _GPU_IPA_THERMAL_H_ */
