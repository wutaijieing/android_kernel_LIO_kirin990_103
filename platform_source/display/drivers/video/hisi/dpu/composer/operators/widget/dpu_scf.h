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

#ifndef DPU_WIDGET_SCF_H
#define DPU_WIDGET_SCF_H

#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_dpu_module.h"
#include "hisi_disp_debug.h"
#include "hisi_disp.h"

#define SCF_MIN_WIDTH  16
#define SCF_MIN_HEIGHT 16
#define SCF_INC_FACTOR 262144 /* 1 << 18 */
#define SCF_INC_OFFSET 131072 /* SCF_INC_FACTOR / 2 */

#define scf_rect_is_underflow(rect) (((rect->w) < SCF_MIN_WIDTH) && ((rect->h) < SCF_MIN_HEIGHT))
#define scf_ratio(src, acc, dst) ((((src - 1) * SCF_INC_FACTOR + SCF_INC_OFFSET) - acc) / (dst - 1))
#define scf_enable_throwing(src, dst)  ((dst) < ((src) >> 2))
#define is_scale_down(src, dst)  ((src) > (dst))
#define is_scale_up(src, dst)  ((src) < (dst))


struct dpu_scf_data {
	char __iomem *base;

	struct disp_rect src_rect;
	struct disp_rect dst_rect;
	uint32_t acc_hscl;
	uint32_t acc_vscl;
	bool has_alpha;
	uint8_t reserved[3];
};

struct scf_reg {
	DPU_ARSR_SCF_EN_HSCL_STR_UNION en_hscl_str;
	DPU_ARSR_SCF_EN_VSCL_STR_UNION en_vscl_str;
	DPU_ARSR_SCF_H_V_ORDER_UNION h_v_order;
	DPU_ARSR_SCF_INPUT_WIDTH_HEIGHT_UNION src_rect;
	DPU_ARSR_SCF_OUTPUT_WIDTH_HEIGHT_UNION dst_rect;
	DPU_ARSR_SCF_EN_HSCL_UNION en_hscl;
	DPU_ARSR_SCF_EN_VSCL_UNION en_vscl;
	DPU_ARSR_SCF_ACC_HSCL_UNION acc_hscl;
	DPU_ARSR_SCF_ACC_HSCL1_UNION acc_hscl1;
	DPU_ARSR_SCF_INC_HSCL_UNION inc_hscl;
	DPU_ARSR_SCF_ACC_VSCL_UNION acc_vscl;
	DPU_ARSR_SCF_ACC_VSCL1_UNION acc_vscl1;
	DPU_ARSR_SCF_INC_VSCL_UNION inc_vscl;
};

#define disp_pr_scf(input) do { \
		disp_pr_rect(info, &(input->src_rect)); \
		disp_pr_rect(info, &(input->dst_rect)); \
	} while (0)

void dpu_scf_set_cmd_item(struct dpu_module_desc *module, struct dpu_scf_data *input);


#endif /* DPU_WIDGET_DFC_H */
