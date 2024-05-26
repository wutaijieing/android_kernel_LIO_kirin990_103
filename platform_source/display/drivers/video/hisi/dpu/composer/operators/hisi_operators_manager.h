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
#ifndef HISI_OPERATORS_MANAGER_H
#define HISI_OPERATORS_MANAGER_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"
#include "hisi_disp_config.h"

struct hisi_comp_operator *hisi_disp_get_operator(union operator_id id_desc);
void hisi_disp_init_operators(union operator_type_desc operator_type_descs[], uint32_t count, void *cookie);
struct list_head *hisi_disp_get_operator_list(void);
int dpu_operator_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *_pre_out_data,
	struct hisi_comp_operator *next_operator);


static inline void dpu_disp_print_pipeline_data(struct hisi_pipeline_data *data)
{
	if (!data)
		return;

	disp_pr_rect(info, &data->rect);
	disp_pr_info("format=%u, rotation=%u, next_order=%u", data->format, data->rotation, data->next_order);

	/* print other information */
}

#endif /* HISI_OPERATORS_MANAGER_H */
