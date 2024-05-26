/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef HISI_DPU_MODULE_H_
#define HISI_DPU_MODULE_H_

#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/types.h>

#include "cmdlist.h"

struct hisi_composer_device;

struct dpu_module_ops {
	void* (* create_context) (void *cookie, int scene_id);
	void (* release_context) (void *context);
	void (* flush_context) (void *context);
};

struct dpu_module_desc {
	struct list_head list_node;

	/* link global cmdlist client, would be updated by everyframe */
	struct cmdlist_client *client;

	struct dpu_module_ops module_ops;
	void *context; /* link private cmdlist client */
	void *cookie;
};

struct dpu_module_ops *hisi_module_get_context_ops(void);

int hisi_module_init(struct dpu_module_desc *module_desc, struct dpu_module_ops ops, void *cookie);
void hisi_module_deinit(struct dpu_module_desc *module_desc);
void hisi_module_set_reg(struct dpu_module_desc *module_desc, char __iomem *addr, uint32_t value);
void hisi_module_flush(struct hisi_composer_device *device);

#endif /* HISI_DPU_MODULE_H_ */
