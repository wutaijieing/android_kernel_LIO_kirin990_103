/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef DKMD_DPU_RES_MGR_H
#define DKMD_DPU_RES_MGR_H

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include "dkmd_chrdev.h"
#include "dpu_config_utils.h"

struct dpu_res_data {
	uint32_t lbuf_size;

	uint32_t offline_scene_id_count;
	uint32_t *offline_scene_ids;
};

/**
 * @brief each process must have its own data
 *
 */
struct res_process_data {
	uint64_t res_types; // detail in dkmd_res_mgr.h
	atomic_t ref_cnt;
	// others
};

struct dpu_res {
	struct platform_device *pdev;
	struct dkmd_chrdev rm_chrdev;

	struct dpu_res_data data;

	struct list_head resource_list;
};

/* resource_list node */
struct dpu_res_resouce_node {
	struct list_head list_node;
	void *data;
	uint32_t res_type;
	atomic_t res_ref_cnt;

	void* (*init)(struct dpu_res_data *rm_data);
	void (*deinit)(void *data);
	void (*reset)(void *data);
	void (*release)(void *data);
	long (*ioctl)(struct dpu_res_resouce_node *res_node, uint32_t cmd, void __user *argp);
};

extern struct dpu_res *g_rm_dev;
static inline struct device *dpu_res_get_device()
{
	return (g_rm_dev && g_rm_dev->pdev) ? &(g_rm_dev->pdev->dev) : NULL;
}

static inline void dpu_res_register_screen_info(uint32_t xres, uint32_t yres)
{
	dpu_config_set_screen_info(xres, yres);
}

#endif /* DPU_RES_MGR_H */
