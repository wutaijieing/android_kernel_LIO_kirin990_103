/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU custom device frequency schedule method
 * Create: 2014-2-24
 */
#ifndef GPU_DEV_FREQ_H
#define GPU_DEV_FREQ_H
#ifdef CONFIG_DRG
#include <linux/drg.h>
#endif
#ifdef CONFIG_PM_DEVFREQ
void kbase_gpu_devfreq_resume(const struct devfreq *df);
void kbase_gpu_devfreq_init(struct kbase_device *kbdev);
#else
static inline void kbase_gpu_devfreq_init(struct kbase_device *kbdev)
{
	(void)kbdev;
}

static inline void kbase_gpu_devfreq_resume(const struct devfreq *df)
{
	(void)df;
}
#endif /* CONFIG_PM_DEVFREQ */
#endif
