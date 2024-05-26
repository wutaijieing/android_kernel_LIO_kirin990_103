/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include "hisi_display_effect_test.h"
#include "hisi_disp_config.h"
#include "hisi_disp_dpp.h"
#include "hisi_disp_gadgets.h"
#include "hisi_dpp_hiace_test.h"
#include "hisi_dpp_gmp_test.h"
#include "hisi_dpp_xcc_test.h"
#include "hisi_dpp_gamma_test.h"
#include "hisi_operator_tool.h"

unsigned hisi_fb_display_effect_test_time = 10;
module_param_named(debug_display_effect_test_time,
hisi_fb_display_effect_test_time, int, 0644);
MODULE_PARM_DESC(debug_display_effect_test_time,
"hisi_fb_display_effect_test_time");

unsigned hisi_fb_display_effect_test_chn = 0x6a;
module_param_named(debug_display_effect_test_chn,
hisi_fb_display_effect_test_chn, int, 0644);
MODULE_PARM_DESC(debug_display_effect_test_chn,
"hisi_fb_display_effect_test_chn");

void dpp_effect_test(char __iomem *dpu_base)
{
	disp_pr_info("[effect] enter");
	if (NULL == dpu_base) {
		disp_pr_err("dpu_base is NULL!\n");
		return;
	}

	disp_pr_info("[dpp] hisi_fb_display_effect_test_time 0x%x\n",
							 hisi_fb_display_effect_test_time);
	if (hisi_fb_display_effect_test_time < 50) {
		if (hisi_fb_display_effect_test_chn & BIT(1)) {
			dpp_effect_gamma_test(dpu_base);
		} else if (hisi_fb_display_effect_test_chn & BIT(3)) {
			dpp_effect_degamma_test(dpu_base);
		} else if (hisi_fb_display_effect_test_chn & BIT(5)) {
			dpp_effect_gmp_test(dpu_base);
		} else if (hisi_fb_display_effect_test_chn & BIT(6)) {
			dpp_effect_xcc_test(dpu_base + DPU_DPP_CH0_XCC_OFFSET);
		} else if (hisi_fb_display_effect_test_chn & BIT(8)) {
			dpp_effect_xcc_test(dpu_base + DPU_DPP_CH0_NL_XCC_OFFSET);
		}
	} else if (hisi_fb_display_effect_test_time == 0x33) {
		disp_pr_info("[effect] hiace test\n");
		dpp_hiace_test(dpu_base);
	}
	return;
}
