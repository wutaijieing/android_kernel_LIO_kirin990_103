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
#ifndef DPU_OPERATOR_HDR_H
#define DPU_OPERATOR_HDR_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

struct dpu_hdr_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_hdr_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_operator_hdr {
	struct hisi_comp_operator base;

	/* other overlay data */
};

struct dpu_hdr_params {
};

void dpu_hdr_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie);
#endif
