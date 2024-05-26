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

#ifndef DPU_WIDGET_SCF_LUT_H
#define DPU_WIDGET_SCF_LUT_H

#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_dpu_module.h"

void dpu_scf_lut_set_cmd(struct dpu_module_desc *module, char __iomem *base);


#endif /* DPU_WIDGET_SCF_LUT_H */
