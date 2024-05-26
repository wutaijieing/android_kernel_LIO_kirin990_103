/*
 * (C) COPYRIGHT 2014, 2018 HISI Limited. All rights reserved.
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

/*
 * Last buffer's policy management callbacks.
 *
 * The Last buffer's policy is created at platform init time based on the device tree,
 * and destroyed at platform term time. So, we can use kbdev->cache_policy_info list
 * directly without lock, you cannot use these callbacks in any other place and for
 * dynamic use. Otherwise, you have to add lock to protect it.
 */

#ifndef _HISI_CACHE_POLICY_H_
#define _HISI_CACHE_POLICY_H_

#include <mali_kbase.h>

/* The policy id of normal memory that bypass hisilicon last buffer.
 * **DO NOT EDIT!**
 */
#define KBASE_CACHE_POLICY_ID_NORMAL 0

/* struct kbase_mem_cache_policy_info - Hold the information about the cache policy
 *                                      which is saved in the dts.
 * @policy_id: The cache policy id
 * @cache_policy: The cache policy corresponds to the cache policy id
 * @next_info: Pointer to the next kbase_mem_cache_policy_info in the list
 */
struct kbase_mem_cache_policy_info {
	unsigned int policy_id;
	unsigned int cache_policy;
	struct kbase_mem_cache_policy_info *next_info;
};

/* This struct holds all of the callbacks of last buffer policy. */
struct kbase_hisi_lb_cache_policy_callbacks {

	/* This callback is used to parse the device tree and create corresponing
	 * kbase_mem_cache_policy_info object. Then, link them together.
	 */
	int  (*lb_create_policy_info)(void *dev);

	/* This callback is used to destroy all of the kbase_mem_cache_policy_info
	 * object in the list.
	 */
	void (*lb_destroy_policy_info)(void *dev);

	/* This callback is used to new a kbase_mem_cache_policy_info object
	 * and add it to list.
	 */
	int  (*lb_new_policy_info)(void *dev, unsigned int policy_id, unsigned int policy);

	/* This callback is used to delete a kbase_mem_cache_policy_info object from list. */
	int  (*lb_del_policy_info)(void *dev, unsigned int policy_id);

	/* This callback is used to get last buffer policy id from cache policy. */
	unsigned int (*lb_get_policy_id)(void *dev, u8 cache_policy);
};

typedef struct kbase_hisi_lb_cache_policy_callbacks cache_policy_callbacks;

#endif