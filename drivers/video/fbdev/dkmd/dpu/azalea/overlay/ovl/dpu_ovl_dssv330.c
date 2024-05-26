/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "../dpu_overlay_utils.h"
#include "../../dpu_mmbuf_manager.h"

void dpu_ovl_init(const char __iomem *ovl_base, dss_ovl_t *s_ovl, int ovl_idx)
{
	int i;

	dpu_check_and_no_retval(!ovl_base, ERR, "ovl_base is NULL!\n");
	dpu_check_and_no_retval(!s_ovl, ERR, "s_ovl is NULL!\n");

	memset(s_ovl, 0, sizeof(dss_ovl_t));

	s_ovl->ovl_size = inp32(ovl_base + OVL_SIZE);
	s_ovl->ovl_bg_color = inp32(ovl_base + OVL_BG_COLOR);
	s_ovl->ovl_dst_startpos = inp32(ovl_base + OVL_DST_STARTPOS);
	s_ovl->ovl_dst_endpos = inp32(ovl_base + OVL_DST_ENDPOS);
	s_ovl->ovl_gcfg = inp32(ovl_base + OVL_GCFG);


	for (i = 0; i < OVL_6LAYER_NUM; i++) {
		s_ovl->ovl_layer[i].layer_pos =
			inp32(ovl_base + OVL_LAYER0_POS + i * 0x3C);
		s_ovl->ovl_layer[i].layer_size =
			inp32(ovl_base + OVL_LAYER0_SIZE + i * 0x3C);
		s_ovl->ovl_layer[i].layer_pattern =
			inp32(ovl_base + OVL_LAYER0_PATTERN + i * 0x3C);
		s_ovl->ovl_layer[i].layer_alpha =
			inp32(ovl_base + OVL_LAYER0_ALPHA + i * 0x3C);
		s_ovl->ovl_layer[i].layer_cfg =
			inp32(ovl_base + OVL_LAYER0_CFG + i * 0x3C);

		s_ovl->ovl_layer_pos[i].layer_pspos =
			inp32(ovl_base + OVL_LAYER0_PSPOS + i * 0x3C);
		s_ovl->ovl_layer_pos[i].layer_pepos =
			inp32(ovl_base + OVL_LAYER0_PEPOS + i * 0x3C);
	}

	s_ovl->ovl_block_size = inp32(ovl_base + OVL6_BLOCK_SIZE);
}

void dpu_ovl_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *ovl_base, dss_ovl_t *s_ovl, int ovl_idx)
{
	int i;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");
	dpu_check_and_no_retval(!ovl_base, ERR, "ovl_base is NULL!\n");
	dpu_check_and_no_retval(!s_ovl, ERR, "s_ovl is NULL!\n");

	if ((ovl_idx == DSS_OVL1) || (ovl_idx == DSS_OVL3)) {
		DPU_FB_ERR("ovl_idx not surport:%d\n", ovl_idx);
		return;
	}

	dpufd->set_reg(dpufd, ovl_base + OVL6_REG_DEFAULT, 0x1, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL6_REG_DEFAULT, 0x0, 32, 0);

	dpufd->set_reg(dpufd, ovl_base + OVL_SIZE, s_ovl->ovl_size, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL_BG_COLOR, s_ovl->ovl_bg_color, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL_DST_STARTPOS, s_ovl->ovl_dst_startpos, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL_DST_ENDPOS, s_ovl->ovl_dst_endpos, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL_GCFG, s_ovl->ovl_gcfg, 32, 0);

	for (i = 0; i < OVL_6LAYER_NUM; i++) {
		if (s_ovl->ovl_layer_used[i] == 1) {
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_POS + i * 0x3C,
				s_ovl->ovl_layer[i].layer_pos, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_SIZE + i * 0x3C,
				s_ovl->ovl_layer[i].layer_size, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_PATTERN + i * 0x3C,
				s_ovl->ovl_layer[i].layer_pattern, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_ALPHA + i * 0x3C,
				s_ovl->ovl_layer[i].layer_alpha, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_CFG + i * 0x3C,
				s_ovl->ovl_layer[i].layer_cfg, 32, 0);

			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_PSPOS + i * 0x3C,
				s_ovl->ovl_layer_pos[i].layer_pspos, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_PEPOS + i * 0x3C,
				s_ovl->ovl_layer_pos[i].layer_pepos, 32, 0);
		} else {
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_POS + i * 0x3C,
				s_ovl->ovl_layer[i].layer_pos, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_SIZE + i * 0x3C,
				s_ovl->ovl_layer[i].layer_size, 32, 0);
			dpufd->set_reg(dpufd, ovl_base + OVL_LAYER0_CFG + i * 0x3C,
				s_ovl->ovl_layer[i].layer_cfg, 32, 0);
		}
	}

	dpufd->set_reg(dpufd, ovl_base + OVL6_BLOCK_SIZE, s_ovl->ovl_block_size, 32, 0);
}

void dpu_ov_set_reg_default_value(struct dpu_fb_data_type *dpufd,
	char __iomem *ovl_base, int ovl_idx)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is null\n");
		return;
	}

	if (!ovl_base) {
		DPU_FB_ERR("ovl_base is null\n");
		return;
	}

	dpufd->set_reg(dpufd, ovl_base + OVL6_REG_DEFAULT, 0x1, 32, 0);
	dpufd->set_reg(dpufd, ovl_base + OVL6_REG_DEFAULT, 0x0, 32, 0);
}

