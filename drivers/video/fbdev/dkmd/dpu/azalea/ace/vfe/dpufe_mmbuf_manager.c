/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/genalloc.h>
#include <linux/uaccess.h>
#include "dpufe_dbg.h"
#include "dpufe_mmbuf_manager.h"

#define VFD_MMBUF_BASE (64)
#define VFD_MMBUF_ADDR_ALIGN  (64)

uint32_t g_mmbuf_max_size = 368640; // 360k
struct list_head *g_vfb_mmbuf_list;
struct gen_pool *g_vfb_mmbuf_gen_pool;
static int g_mmbuf_refcount;

DEFINE_SEMAPHORE(dpufe_mmbuf_sem);

struct dpufe_mmbuf {
	struct list_head list_node;
	uint32_t addr;
	uint32_t size;
};

void dpufe_mmbuf_sem_pend(void)
{
	down(&dpufe_mmbuf_sem);
}

void dpufe_mmbuf_sem_post(void)
{
	up(&dpufe_mmbuf_sem);
}

void *dpufe_mmbuf_init(struct dpufe_data_type *dfd)
{
	struct gen_pool *pool = NULL;
	int order = 3;  /* min alloc order */
	uint32_t addr = VFD_MMBUF_BASE;
	int prev_refcount;

	if (!dfd) {
		DPUFE_ERR("vfd is NULL\n");
		return NULL;
	}

	DPUFE_INFO("fb%d, +\n", dfd->index);
	prev_refcount = g_mmbuf_refcount++;
	if (prev_refcount == 0) {
		/* mmbuf pool */
		pool = gen_pool_create(order, 0);
		if (!pool) {
			DPUFE_ERR("fb%d, gen_pool_create failed\n", dfd->index);
			return NULL;
		}

		if (gen_pool_add(pool, addr, (size_t)g_mmbuf_max_size, 0) != 0) {
			gen_pool_destroy(pool);
			DPUFE_ERR("fb%d, gen_pool_add failed\n", dfd->index);
			return NULL;
		}

		g_vfb_mmbuf_gen_pool = pool;

		/* mmbuf list */
		if (!g_vfb_mmbuf_list) {
			g_vfb_mmbuf_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
			if (!g_vfb_mmbuf_list) {
				DPUFE_ERR("g_mmbuf_list is NULL\n");
				return NULL;
			}
			INIT_LIST_HEAD(g_vfb_mmbuf_list);
		}
	}

	dfd->mmbuf_gen_pool = g_vfb_mmbuf_gen_pool;
	dfd->mmbuf_list = g_vfb_mmbuf_list;
	DPUFE_INFO("fb%d, -, g_mmbuf_refcount=%d\n", dfd->index, g_mmbuf_refcount);

	return pool;
}

void dpufe_mmbuf_deinit(struct dpufe_data_type *dfd)
{
	int new_refcount;

	if (!dfd) {
		DPUFE_ERR("vfd is NULL\n");
		return;
	}

	DPUFE_DEBUG("fb%d, +\n", dfd->index);

	dpufe_mmbuf_mmbuf_free_all(dfd);
	dpufe_mmbuf_sem_pend();
	new_refcount = --g_mmbuf_refcount;
	if (new_refcount < 0)
		DPUFE_ERR("dss new_refcount err\n");

	if (new_refcount == 0) {
		/* mmbuf pool */
		if (g_vfb_mmbuf_gen_pool != NULL) {
			gen_pool_destroy(g_vfb_mmbuf_gen_pool);
			g_vfb_mmbuf_gen_pool = NULL;
		}

		/* mmbuf list */
		if (g_vfb_mmbuf_list != NULL) {
			kfree(g_vfb_mmbuf_list);
			g_vfb_mmbuf_list = NULL;
		}
	}

	dfd->mmbuf_gen_pool = NULL;
	dfd->mmbuf_list = NULL;
	dpufe_mmbuf_sem_post();

	DPUFE_DEBUG("fb%d, -, g_mmbuf_refcount=%d\n", dfd->index, g_mmbuf_refcount);
}

uint32_t dpufe_mmbuf_alloc(void *handle, uint32_t size)
{
	uint32_t addr = 0;
	struct dpufe_mmbuf *node = NULL;
	struct dpufe_mmbuf *mmbuf_node = NULL;
	struct dpufe_mmbuf *_node_ = NULL;

	dpufe_check_and_return(!handle, addr, "handle is NULL\n");
	dpufe_check_and_return(!g_vfb_mmbuf_list, addr, "g_mmbuf_list is NULL\n");
	dpufe_check_and_return((size == 0), addr, "mmbuf size is invalid, size=%d\n", size);

	dpufe_mmbuf_sem_pend();
	addr = gen_pool_alloc(handle, size);
	if (addr <= 0) {
		list_for_each_entry_safe(mmbuf_node, _node_, g_vfb_mmbuf_list, list_node) {
			DPUFE_DEBUG("mmbuf_node_addr[0x%x], mmbuf_node_size[0x%x]\n",
				mmbuf_node->addr, mmbuf_node->size);
		}
		addr = VFD_MMBUF_BASE;
		DPUFE_DEBUG("note: mmbuf not enough,addr=0x%x\n", addr);
	} else {
		/* node */
		node = kzalloc(sizeof(struct dpufe_mmbuf), GFP_KERNEL);
		if (node != NULL) {
			node->addr = addr;
			node->size = size;
			list_add_tail(&node->list_node, g_vfb_mmbuf_list);
		} else {
			DPUFE_ERR("kzalloc struct dpufb_mmbuf fail\n");
		}

		if ((addr & (VFD_MMBUF_ADDR_ALIGN - 1)) || (size & (VFD_MMBUF_ADDR_ALIGN - 1))) {
			DPUFE_ERR("addr[0x%x] is not %d bytes aligned, or size[0x%x] is not %d bytes aligned\n",
				addr, VFD_MMBUF_ADDR_ALIGN, size, VFD_MMBUF_ADDR_ALIGN);

			list_for_each_entry_safe(mmbuf_node, _node_, g_vfb_mmbuf_list, list_node) {
				DPUFE_ERR("mmbuf_node_addr[0x%x], mmbuf_node_size[0x%x]\n", mmbuf_node->addr,
					mmbuf_node->size);
			}
		}
	}

	dpufe_mmbuf_sem_post();
	DPUFE_DEBUG("addr=0x%x, size=%d\n", addr, size);
	return addr;
}

void dpufe_mmbuf_free(void *handle, uint32_t addr, uint32_t size)
{
	struct dpufe_mmbuf *node = NULL;
	struct dpufe_mmbuf *_node_ = NULL;

	dpufe_check_and_no_retval(!handle, "handle is NULL\n");
	dpufe_check_and_no_retval(!g_vfb_mmbuf_list, "g_mmbuf_list is NULL\n");

	dpufe_mmbuf_sem_pend();

	list_for_each_entry_safe(node, _node_, g_vfb_mmbuf_list, list_node) {
		if ((node->addr == addr) && (node->size == size)) {
			gen_pool_free(handle, addr, size);
			list_del(&node->list_node);
			kfree(node);
			node = NULL;
		}
	}

	dpufe_mmbuf_sem_post();
	DPUFE_DEBUG("addr=0x%x, size=%d\n", addr, size);
}

void dpufe_mmbuf_mmbuf_free_all(struct dpufe_data_type *dfd)
{
	struct dpufe_mmbuf *node = NULL;
	struct dpufe_mmbuf *_node_ = NULL;

	dpufe_check_and_no_retval(!dfd, "dpufd is NULL\n");
	dpufe_check_and_no_retval(!g_vfb_mmbuf_list, "g_mmbuf_list is NULL\n");

	list_for_each_entry_safe(node, _node_, g_vfb_mmbuf_list, list_node) {
		if ((node->addr > 0) && (node->size > 0)) {
			gen_pool_free(dfd->mmbuf_gen_pool, node->addr, node->size);
			DPUFE_DEBUG("dpu_mmbuf_free, addr=0x%x, size=%d\n", node->addr, node->size);
		}

		list_del(&node->list_node);
		kfree(node);
		node = NULL;
	}
}

int dpufe_mmbuf_request(struct fb_info *info, void __user *argp)
{
	int ret;
	uint32_t mmbuf_size_max;
	struct dpufe_data_type *dfd = NULL;
	dss_mmbuf_t mmbuf_info;

	dpufe_check_and_return(!info, -EINVAL, "dss mmbuf alloc info NULL Pointer\n");
	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dpufd is NULL Pointer\n");

	dpufe_check_and_return(!argp, -EINVAL, "dss mmbuf alloc argp NULL Pointer\n");

	ret = copy_from_user(&mmbuf_info, argp, sizeof(dss_mmbuf_t));
	if (ret != 0) {
		DPUFE_ERR("fb%d, copy for user failed!ret=%d\n", dfd->index, ret);
		return -EINVAL;
	}

	mmbuf_size_max = g_mmbuf_max_size;
	if ((mmbuf_info.size <= 0) || (mmbuf_info.size > mmbuf_size_max) ||
		(mmbuf_info.size & (VFD_MMBUF_ADDR_ALIGN - 1))) {
		DPUFE_ERR("fb%d, mmbuf size is invalid, size=%d\n", dfd->index, mmbuf_info.size);
		return -EINVAL;
	}

	mmbuf_info.addr = dpufe_mmbuf_alloc(dfd->mmbuf_gen_pool, mmbuf_info.size);
	if (mmbuf_info.addr < VFD_MMBUF_BASE)
		return -EINVAL;

	ret = copy_to_user(argp, &mmbuf_info, sizeof(dss_mmbuf_t));
	if (ret != 0) {
		DPUFE_ERR("fb%d, copy to user failed!ret=%d\n", dfd->index, ret);
		dpufe_mmbuf_free(dfd->mmbuf_gen_pool, mmbuf_info.addr, mmbuf_info.size);
		return -EINVAL;
	}

	return 0;
}

int dpufe_mmbuf_release(struct fb_info *info, const void __user *argp)
{
	int ret;
	struct dpufe_data_type *dfd = NULL;
	dss_mmbuf_t mmbuf_info;

	dpufe_check_and_return(!info, -EINVAL, "dss mmbuf free info NULL Pointer\n");
	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dss mmbuf free dpufd NULL Pointer\n");
	dpufe_check_and_return(!argp, -EINVAL, "dss mmbuf free argp NULL Pointer\n");

	ret = copy_from_user(&mmbuf_info, argp, sizeof(dss_mmbuf_t));
	if (ret != 0) {
		DPUFE_ERR("fb%d, copy for user failed!ret=%d\n", dfd->index, ret);
		ret = -EINVAL;
		goto err_out;
	}

	if ((mmbuf_info.addr <= 0) || (mmbuf_info.size <= 0)) {
		DPUFE_ERR("fb%d, addr=0x%x, size=%d is invalid\n",
			dfd->index, mmbuf_info.addr, mmbuf_info.size);
		ret = -EINVAL;
		goto err_out;
	}

	dpufe_mmbuf_free(dfd->mmbuf_gen_pool, mmbuf_info.addr, mmbuf_info.size);
	return 0;

err_out:
	return ret;
}

int dpufe_mmbuf_free_all(struct fb_info *info, const void __user *argp)
{
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!info, -EINVAL, "fb info is NULL Pointer\n");
	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dpufd is NULL Pointer\n");

	/* argp can be set to mmbuf owner, but now it isn't useful */
	(void)argp;

	dpufe_mmbuf_sem_pend();
	dpufe_mmbuf_mmbuf_free_all(dfd);
	dpufe_mmbuf_sem_post();

	return 0;
}
