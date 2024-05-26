/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include "cmdlist_interface.h"
#include "dpu_cmdlist.h"
#include "dpu_dacc.h"
#include "scene/dpu_comp_scene.h"
#include "dpu_comp_smmu.h"

void dpu_comp_scene_switch(struct dkmd_connector_info *pinfo, struct composer_scene *scene)
{
	uint32_t primary_post_ch_offset = 0x0;
	uint32_t external_post_ch_offset = 0x0;
	uint32_t pre_itfch_offset = DPU_ITF_CH0_OFFSET + scene->scene_id * 0x100;
	uint32_t ctrl_sig_en = 0x0;

	/* dual-mipi or cphy 1+1 */
	ctrl_sig_en = (pinfo->sw_post_chn_num > EXTERNAL_CONNECT_CHN_IDX) ? 0x0 : 0x1;
	primary_post_ch_offset = pinfo->sw_post_chn_idx[PRIMARY_CONNECT_CHN_IDX] * 0x1C;

	set_reg(DPU_PIPE_SW_SIG_CTRL_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
		+ primary_post_ch_offset, BIT(scene->scene_id), 32, 0);

	set_reg(DPU_PIPE_SW_SW_POS_CTRL_SIG_EN_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
		+ primary_post_ch_offset, ctrl_sig_en, 32, 0);

	set_reg(DPU_PIPE_SW_DAT_CTRL_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
		+ primary_post_ch_offset, BIT(scene->scene_id), 32, 0);

	set_reg(DPU_PIPE_SW_SW_POS_CTRL_DAT_EN_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
		+ primary_post_ch_offset, 0x1, 32, 0);

	/* dual-mipi or cphy 1+1 */
	if (pinfo->sw_post_chn_num > EXTERNAL_CONNECT_CHN_IDX) {
		external_post_ch_offset = pinfo->sw_post_chn_idx[EXTERNAL_CONNECT_CHN_IDX] * 0x1C;

		set_reg(DPU_PIPE_SW_SIG_CTRL_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
			+ external_post_ch_offset, BIT(scene->scene_id), 32, 0);

		set_reg(DPU_PIPE_SW_SW_POS_CTRL_SIG_EN_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
			+ external_post_ch_offset, 0x1, 32, 0);

		set_reg(DPU_PIPE_SW_DAT_CTRL_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
			+ external_post_ch_offset, BIT(scene->scene_id), 32, 0);

		set_reg(DPU_PIPE_SW_SW_POS_CTRL_DAT_EN_0_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET) \
			+ external_post_ch_offset, 0x1, 32, 0);

		/* check split swap and size config */
		set_reg(DPU_PIPE_SW_SPLIT_HSIZE_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET),
			((pinfo->base.dsc_out_width / 2 - 1) << 13) | (pinfo->base.dsc_out_width - 1), 26, 0);

		set_reg(DPU_PIPE_SW_SPLIT_CTL_ADDR(scene->dpu_base + DPU_PIPE_SW_OFFSET), 0x0, 1, 0);
	}

	/* these code below may be deleted, but only kernel test case would be needed */
	if (is_dp_panel(&pinfo->base)) {
		dpu_pr_debug("xres=%d, yres=%d, fps=%d", pinfo->base.xres, pinfo->base.yres, pinfo->base.fps);

		set_reg(DPU_ITF_CH_IMG_SIZE_ADDR(scene->dpu_base + pre_itfch_offset),
			((pinfo->base.yres - 1) << 16) | (pinfo->base.xres - 1), 32, 0);
		set_reg(DPU_ITF_CH_ITFSW_DATA_SEL_ADDR(scene->dpu_base + pre_itfch_offset), 0x0, 32, 0);
		if (pinfo->colorbar_enable) {
			set_reg(DPU_ITF_CH_DPP_CLRBAR_CTRL_ADDR(scene->dpu_base + pre_itfch_offset),
				((pinfo->base.xres / 12 - 1) << 24) | 0x1, 32, 0);
			set_reg(DPU_ITF_CH_CLRBAR_START_ADDR(scene->dpu_base + pre_itfch_offset), 0x1, 32, 0);
		}
		set_reg(DPU_ITF_CH_REG_CTRL_FLUSH_EN_ADDR(scene->dpu_base + pre_itfch_offset), 0x1, 32, 0);
	}
}

static int32_t composer_scene_present(struct composer_scene *scene, uint32_t cmdlist_id)
{
	dpu_cmdlist_present_commit(scene->dpu_base, scene->scene_id, cmdlist_id);
	dpu_dacc_config_scene(scene->dpu_base, scene->scene_id, true);

	return 0;
}

/**
 * @brief Different scenarios to show interface may not be the same,
 * so need a interface initialization here
 *
 * @param scene diffrent scene struct
 * @return int32_t
 */
int32_t dpu_comp_scene_device_setup(struct composer_scene *scene)
{
	switch (scene->scene_id) {
	case DPU_SCENE_ONLINE_0:
	case DPU_SCENE_ONLINE_1:
	case DPU_SCENE_ONLINE_2:
	case DPU_SCENE_ONLINE_3:
	case DPU_SCENE_OFFLINE_0:
	case DPU_SCENE_OFFLINE_1:
	case DPU_SCENE_OFFLINE_2:
		scene->present = composer_scene_present;
		break;
	default:
		scene->present = NULL;
	}

	return 0;
}
