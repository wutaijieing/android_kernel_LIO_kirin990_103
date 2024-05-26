/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2019. All rights reserved.
 * Description: This file describe HISI GPU related init
 * Author: Huawei Technologies Co., Ltd.
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

#ifndef _KBASE_HISI_LB_POLICY_H_
#define _KBASE_HISI_LB_POLICY_H_

#include <mali_kbase.h>

/* The maximum length of the policy name. */
#define PLC_NAME_SIZE   16

/* struct kbase_lb_policy_info - Hold the information about the lb policy
 *                               which is derived from the device tree.
 * @name: The cache policy name
 * @policy_id: The cache policy id. Currently, the cache policy id is same
 *                                  as the memory group's group id.
 * @cache_policy: The cache policy corresponds to the cache policy id.
 *                We will use the cache policy specified by the user space
 *                to get the policy id.
 */
struct kbase_lb_policy_info {
	char name[PLC_NAME_SIZE];
	unsigned int policy_id;
	unsigned int cache_policy;
};

/* struct kbase_policy_manager - Contains all the cache policy info of
 *                               last buffer.
 * @policy: The array of kbase_lb_policy_info.
 *
 * Currently, the maximum nr of kbase_lb_policy_info the driver supported is
 * BASE_MEM_GROUP_COUNT and hisilicon implementaion won't exceed this.
 */
struct kbase_policy_manager {
	struct kbase_lb_policy_info policy[BASE_MEM_GROUP_COUNT];
};

/**
 * kbase_mem_init_lb_policy() - Init the last buffer cache policy.
 * @kbdev: Kbase device
 *
 * This funciton is used to parse the specific device tree and get all of
 * the GPU's policy id and cache policy, we create a kbase_policy_manager
 * object to hold these.
 *
 * Return: 0 for success, error code otherwise.
 */
int kbase_mem_init_lb_policy(struct kbase_device *kbdev);

/**
 * kbase_mem_term_lb_policy() - Term the last buffer cache policy.
 * @kbdev: Kbase device
 *
 * Corresponding to kbase_mem_init_lb_policy(), this function is used to
 * term the use of cache policy of last buffer.
 */
void kbase_mem_term_lb_policy(const struct kbase_device *kbdev);

#endif /* _KBASE_HISI_LB_POLICY_H_ */
