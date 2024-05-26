/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_COMPOSER_OPERATORS_H
#define HISI_COMPOSER_OPERATORS_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_connector_utils.h"
#include "hisi_composer_core.h"
#include "hisi_dpu_module.h"

struct hisi_operator_type;
struct hisi_comp_operator;
struct hisi_pipeline_data;

typedef int (*set_cmd_item_cb)(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
		void *layer);

typedef int (*build_data_cb)(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *pre_out_data, \
		struct hisi_comp_operator *next_operator);

typedef void (*init_operator_cb)(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);


#define disp_pr_edit_rect(rect) disp_pr_info("%s (left=%u, right=%u, top=%u, bottom=%u)",\
									#rect, rect.left, rect.right, rect.top, rect.bottom)

/* used for clip or padding, such as:
 * clip left pixel num, clip right pixel num.
 */
struct dpu_edit_rect {
	uint16_t left;
	uint16_t right;
	uint16_t top;
	uint16_t bottom;
};

struct dpu_operator_offset {
	uint32_t reg_offset;
	uint32_t dm_offset;
};

/* public: rdma->rot->scl->overlay->dpp->mipi
 * this is base struct for operators, other operators can inherit this struct
 */
struct hisi_pipeline_data {
	struct disp_rect rect;
	uint32_t rotation;
	enum DPU_FORMAT format;

	uint32_t next_order;
	/* other public data */
};

struct hisi_comp_operator {
	union operator_id id_desc;

	/* it was used to dm count operator nums, when the operator was reused,
	 * it can't be counted repeated
	 */
	bool be_dm_counted;
	uint32_t scene_id;
	uint32_t operator_offset;
	uint32_t dm_offset;
	enum PIPELINE_TYPE operator_pos;

	struct semaphore operator_sem;
	struct hisi_pipeline_data *out_data;
	struct hisi_pipeline_data *in_data;

	struct dpu_module_desc module_desc;

	// need verify interface
	set_cmd_item_cb set_cmd_item;
	build_data_cb build_data;
};

struct hisi_operator_type {
	struct list_head list_node;

	uint32_t type;
	uint32_t operator_count;
	struct hisi_comp_operator **operators;
};

static inline int hisi_operator_init(struct hisi_comp_operator *operator, struct dpu_module_ops ops, void *cookie)
{
	if (operator == NULL) {
		disp_pr_err(" operator is null \n");
		return -1;
	}

	disp_pr_info(" ++++++ \n");
	return hisi_module_init(&operator->module_desc, ops, cookie);
}

static inline void hisi_operator_deinit(struct hisi_comp_operator *operator)
{
	if (operator == NULL) {
		disp_pr_err(" operator is null \n");
		return;
	}

	hisi_module_deinit(&operator->module_desc);
}

#endif /* HISI_COMPOSE_OPERATORS_H */
