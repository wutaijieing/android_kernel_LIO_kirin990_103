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

#ifndef DPU_OPERATOR_WCH_H
#define DPU_OPERATOR_WCH_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"
#include "wch/dpu_wch_wdma.h"
#include "wch/dpu_wch_dither.h"
#include "wch/dpu_wch_post_clip.h"
#include "wch/dpu_wch_dfc.h"
#include "wch/dpu_wch_scf.h"
#include "wch/dpu_wch_csc.h"

struct dpu_wch_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_wch_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_operator_wch {
	struct hisi_comp_operator base;

	/* other wch data */

};

void dpu_wch_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie);
int dpu_wch_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *ov_dev,
	void *layer);

#endif /* DPU_OPERATOR_WCH_H */
