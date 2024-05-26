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

#define GPU_DEFAULT_GOVERNOR	"gpu_scene_aware"

#ifdef CONFIG_GPU_FHSS
int fhss_init(struct device *dev);
#else
static inline int fhss_init(struct device *dev __maybe_unused){return 0;}
#endif
/* Hz */
void update_gputop_linked_freq(unsigned long freq);
/* MHz */
void ipa_gputop_freq_limit(unsigned long freq);

#ifdef CONFIG_GPUFLINK
void gpuflink_notifiy(unsigned long gpufreq);
int gpuflink_init(struct devfreq *devfreq);
void gpuflink_suspend(struct devfreq *devfreq);
void gpuflink_resume(struct devfreq *devfreq);
#else
static inline void gpuflink_notifiy(unsigned long gpufreq __maybe_unused){}
static inline int gpuflink_init(struct devfreq *devfreq __maybe_unused){return 0;}
static inline void gpuflink_suspend(struct devfreq *devfreq __maybe_unused){}
static inline void gpuflink_resume(struct devfreq *devfreq __maybe_unused){}
#endif
#endif /* _GPUFREQ_H */
