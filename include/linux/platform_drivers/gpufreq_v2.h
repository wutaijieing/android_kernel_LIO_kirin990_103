/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: gpufreq share
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __GPUFREQ_H__
#define __GPUFREQ_H__

#include <linux/devfreq.h>

/*
 * struct devfreq_data :
 * Private data given from devfreq user device to governors.
 *
 * @vsync_hit: Indicate whether vsync signal is hit or miss.
 * @cl_boost: Indicate whether to boost the freq in OpenCL scene.
 */
struct devfreq_data {
	int vsync_hit;
	int cl_boost;
};

enum gpufreq_cb_type {
	GF_CB_BUSYTIME = 0,
	GF_CB_VSYNC_HIT = 1,
	GF_CB_CL_BOOST = 2,
	GF_CB_USER_TARGET = 3,
	GF_CB_CORE_MASK = 4,
	GF_CB_MAX
};

int gpufreq_register_callback(unsigned int type, int (*cb)(void));
int gpufreq_start(void);
int gpufreq_term(void);
unsigned long gpufreq_get_cur_freq(void);
void gpufreq_devfreq_suspend(void);
void gpufreq_devfreq_resume(void);

struct dev_pm_qos_request;
#ifdef CONFIG_GPUFREQ_VOTE
/* khz */
int gpufreq_register_min_req(struct dev_pm_qos_request *req, int value);
int gpufreq_register_max_req(struct dev_pm_qos_request *req, int value);
int gpufreq_unregister_request(struct dev_pm_qos_request *req);
int gpufreq_vote_request(struct dev_pm_qos_request *req, int value);
#else
static inline int gpufreq_register_min_req(struct dev_pm_qos_request *req __maybe_unused,
					   int value __maybe_unused)
{
	return 0;
}
static inline int gpufreq_register_max_req(struct dev_pm_qos_request *req __maybe_unused,
					   int value __maybe_unused)
{
	return 0;
}
static inline int gpufreq_unregister_request(struct dev_pm_qos_request *req __maybe_unused)
{
	return 0;
}
static inline int gpufreq_vote_request(struct dev_pm_qos_request *req __maybe_unused,
				       int value __maybe_unused)
{
	return 0;
}
#endif /* CONFIG_GPUFREQ_VOTE */
#endif /* __GPUFREQ_H__ */
