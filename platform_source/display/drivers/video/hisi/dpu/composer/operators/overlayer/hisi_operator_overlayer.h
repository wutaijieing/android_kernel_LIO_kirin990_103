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
#ifndef HISI_OPERATOR_OVERLAYER_H
#define HISI_OPERATOR_OVERLAYER_H

#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

struct hisi_ov_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_ov_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_ov {
	struct hisi_comp_operator base;

	/* other overlay data */

};

void hisi_overlayer_init(struct hisi_operator_type *type_operators, struct dpu_module_ops ops, void *cookie);
//int hisi_overlayer_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device, struct hisi_disp_cmdlist *cmdlist, void *layer);


#endif /* HISI_OPERATOR_OVERLAYER_H */
