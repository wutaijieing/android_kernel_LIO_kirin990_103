/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: This file describe GPU related init
 * Create: 2014-2-24
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

#ifndef _KABSE_LB_H_
#define _KABSE_LB_H_

#include <mali_kbase.h>
#include "mali_kbase_policy_manager.h"
#include "mali_kbase_quota_manager.h"

/**
 * struct kbase_gpu_lb_callbacks - struct holds all of the callbacks
 *                                  related with last buffer.
 * @lb_init: The callback to init the last buffer's cache policy,
 *           quota manager, etc.
 * @lb_term: The callback to term the last buffer's related resources.
 * @lb_alloc: The callback to allocate the last buffer quota.
 * @lb_free:  The callback to release the last buffer quota.
 */
typedef struct kbase_gpu_lb_callbacks {
	int (*lb_init)(struct kbase_device *kbdev);
	int (*lb_term)(struct kbase_device *kbdev);
	int (*lb_alloc)(struct kbase_device *kbdev);
	int (*lb_free)(struct kbase_device *kbdev);
} gpu_lb_callbacks;

/**
 * kbase_get_lb_cbs() - Get the pointer of gpu_lb_callbacks instance.
 * @kbdev: Kbase device
 *
 * Return: Pointer to gpu_lb_callbacks object.
 */
gpu_lb_callbacks *kbase_get_lb_cbs(const struct kbase_device *kbdev);

#endif /* _KABSE_LB_H_ */
