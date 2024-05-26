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

#ifndef DPU_WIDGET_POST_CLIP_H
#define DPU_WIDGET_POST_CLIP_H

#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_dpu_module.h"

struct dpu_post_clip_data {
	char __iomem *base;

	/* TODO: other data */
};
void dpu_post_clip_set_cmd_item(struct dpu_module_desc *module, struct dpu_post_clip_data *input);


#endif /* DPU_WIDGET_POST_CLIP_H */
