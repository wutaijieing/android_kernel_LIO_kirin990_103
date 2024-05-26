/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: gpufreq file system head file
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
#ifndef __GPUFREQ_FS_H__
#define __GPUFREQ_FS_H__

#ifdef CONFIG_GPUFREQ_USER_VOTE
int gpufreq_fs_register(struct device *gpu_dev);
void gpufreq_fs_deregister(void);
#else
int gpufreq_fs_register(struct device *gpu_dev __maybe_unused){return 0;}
void gpufreq_fs_deregister(void){}
#endif /* CONFIG_GPUFREQ_USER_VOTE */

#endif /* __GPUFREQ_FS_H__ */
