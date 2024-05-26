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
 * Last buffer memory pool management callbacks.
 *
 * These callbacks are used to create last buffer memory pools with differnet cache
 * policy and policy id. Currently, the LB. memory pools are created when context creating and
 * device memory init. They are destroyed when context term and device memory term.
 * Further more, every pool has its own lock, so, it is no need to add lock to protect
 * pool list during these time. However, if you want to create/term memory pools besides
 * these time, you have to add lock to protect the pool list. Now, we have no scene like this.
 */

#ifndef _HISI_MEM_POOLS_H_
#define _HISI_MEM_POOLS_H_

#include <mali_kbase.h>

/**
 * struct kbase_hisi_lb_pools - struct holds the info of lb memory pools.
 *
 * @mem_pools_lock: The spin_lock used to protect @mem_pools when
 *                  access it in multi-thread.
 * @mem_pools:      The list of last buffer memory pools of 4KB page size
 *                  with different policy id.
 * @lp_mem_pools_lock: The spin_lock used to protect @lp_mem_pools when
 *                  access it in multi-thread.
 * @lp_mem_pools:   The list of last buffer memory pools of 2MB page size
 *                  with different policy id.
 */
struct kbase_hisi_lb_pools {
	spinlock_t mem_pools_lock;
	struct list_head mem_pools;

	spinlock_t lp_mem_pools_lock;
	struct list_head lp_mem_pools;
};

/**
 * struct kbase_hisi_lb_pools_callbacks - struct holds all the callbacks of lb_pools.
 *
 * @find_pool:       Callback to find the memory pool with specific policy_id in pool_list.
 * @init_ctx_pools:  Callback to init the context lb memory pools, include the large-page pool.
 * @term_ctx_pools:  Callback to term the context lb memory pools, destroy and free it.
 * @dying_ctx_pools: Callback to mark the context lb memory pools as dying state.
 * @init_dev_pools:  Callback to init the device  lb memory pools, include the large-page pool.
 * @term_dev_pools:  Callback to term the device  lb memory pools, destroy and free it.
 * @dying_dev_pools: Callback to mark the device  lb memory pools as dying state.
 *
 */
struct kbase_hisi_lb_pools_callbacks {

	/**
	 * find_pool -- Find the memory pool.
	 *
	 * @policy_id: The cache policy id of the page in memory pool.
	 * @pool_list: The memory pools list to find.
	 *
	 * Return: The found memory pool with specific @policy_id in @pool_list.
	 */
	struct kbase_mem_pool* (*find_pool)(unsigned int policy_id, struct list_head *pool_list);

	/**
	 * init_ctx_pools -- Init the context's local lb_pools.
	 *
	 * @kctx: The context to operate.
	 */
	int  (*init_ctx_pools)( struct kbase_context *kctx);

	/**
	 * term_ctx_pools -- Term the context's local lb_pools.
	 *
	 * @kctx: The context to operate.
	 * Just like the init_ctx_pools(), this funciton will terminate the inited pools,
	 * destroy and free them.
	 */
	void (*term_ctx_pools)( struct kbase_context *kctx);

	/**
	 * dying_ctx_pools -- Mark the context's local lb_pools as dying.
	 *
	 * @kctx: The context to operate.
	 */
	void (*dying_ctx_pools)(struct kbase_context *kctx);

	/**
	 * init_dev_pools -- Init the device's lb_pools.
	 *
	 * @kbdev: The kbase_device to operate.
	 * This function will init the device's lb_pools, both 4KB page size pools
	 * and 2MB page size pools.
	 */
	int  (*init_dev_pools)(struct kbase_device  *kbdev);

	/**
	 * term_dev_pools -- Term the device's local lb_pools.
	 *
	 * @kbdev: The kbase_device to operate.
	 * Just like the init_dev_pools(), this funciton will terminate the inited pools,
	 * destroy and free them.
	 */
	void (*term_dev_pools)(struct kbase_device  *kbdev);

	/**
	 * dying_dev_pools -- Mark the device's local lb_pools as dying.
	 *
	 * @kbdev: The kbase_device to operate.
	 */
	void (*dying_dev_pools)(struct kbase_device *kbdev);
};

typedef struct kbase_hisi_lb_pools_callbacks lb_pools_callbacks;

#endif
