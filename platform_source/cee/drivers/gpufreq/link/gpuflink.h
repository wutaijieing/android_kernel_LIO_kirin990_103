/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: gpu freq link head file
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __GPU_FLINK_H__
#define __GPU_FLINK_H__

struct list_head;
struct gpuflink_governor {
	const char *name;
	struct list_head node;
	int (*gov_init)(void);
	int (*get_target)(unsigned long gpufreq, int mode);
	int (*gov_exit)(void);
};

struct gpuflink_driver {
	const char *name;
	struct list_head node;
	void (*set_target)(unsigned int freq);
	const char *governor_name;
	struct gpuflink_governor *governor;
};

#ifdef CONFIG_GPUFLINK
int gpuflink_add_governor(struct gpuflink_governor *governor);
int gpuflink_remove_governor(struct gpuflink_governor *governor);
int gpuflink_add_driver(struct gpuflink_driver *driver);
int gpuflink_remove_driver(struct gpuflink_driver *driver);

#ifdef CONFIG_GPUFREQ_INDEPDENT
void gpuflink_notifiy(unsigned long gpufreq);
int gpuflink_init(struct devfreq *devfreq);
void gpuflink_suspend(struct devfreq *devfreq);
void gpuflink_resume(struct devfreq *devfreq);
#endif /* CONFIG_GPUFREQ_INDEPDENT */

#else
static inline int gpuflink_add_governor(struct gpuflink_governor *governor __maybe_unused){return 0;}
static inline int gpuflink_remove_governor(struct gpuflink_governor *governor __maybe_unused){return 0;}
static inline int gpuflink_add_driver(struct gpuflink_driver *driver __maybe_unused){return 0;}
static inline int gpuflink_remove_driver(struct gpuflink_driver *driver __maybe_unused){return 0;}

#ifdef CONFIG_GPUFREQ_INDEPDENT
static inline void gpuflink_notifiy(unsigned long gpufreq __maybe_unused){}
static inline int gpuflink_init(struct devfreq *devfreq __maybe_unused){return 0;}
static inline void gpuflink_suspend(struct devfreq *devfreq __maybe_unused){}
static inline void gpuflink_resume(struct devfreq *devfreq __maybe_unused){}
#endif /* CONFIG_GPUFREQ_INDEPDENT */
#endif /* CONFIG_GPUFLINK */
#endif /* __GPU_FLINK_H__ */