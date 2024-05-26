/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_dpp_local_dimming.h"
#include "hisi_disp_composer.h"
#include "hisi_composer_operator.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_debug.h"

#define DIM_HVZME_LUT_SIZE 257
#define DIM_LUT_DATA_SIZE 7
#define DIM_DIV_LUT_SIZE 1024

#define LED_NUM_H_MAX_SIZE 64
#define LED_NUM_V_MAX_SIZE 32

static unsigned int dim_div_luts[1024] = {
	#include "ld_test_params/dim_div_luts.txt"
};

static unsigned int dim_hzme_luts[257][7] = {
	#include "ld_test_params/dim_hzme_luts.txt"
};

static unsigned int dim_vzme_luts[257][7] = {
	#include "ld_test_params/dim_vzme_luts.txt"
};

static void get_ld_dither_info(struct hisi_composer_data *ov_data, struct hisi_ld_dither_info *ld_dither_info)
{
	disp_pr_debug(" ++++ \n");
	ld_dither_info->dither_ctl1.value = 0;
	ld_dither_info->dither_ctl1.reg.dither_mode = 1; // 12bits to 10bits
	ld_dither_info->dither_ctl1.reg.dither_sel = 1;
	ld_dither_info->dither_ctl1.reg.dither_auto_sel_en = 0;

	ld_dither_info->dither_ctl0.value = 0;
	ld_dither_info->dither_ctl0.reg.dither_en = get_ld_dither_en();
	ld_dither_info->dither_ctl0.reg.dither_hifreq_noise_mode = 1;
	ld_dither_info->dither_ctl0.reg.dither_rgb_shift_mode = 1;
	ld_dither_info->dither_ctl0.reg.dither_uniform_en = 0;

	ld_dither_info->hifreq_reg_ini_cfg_en.value = 0;
	ld_dither_info->hifreq_reg_ini_cfg_en.reg.hifreq_reg_ini_cfg_en = 1;

	ld_dither_info->dither_frc_ctl.value = 0;
	ld_dither_info->dither_frc_ctl.reg.frc_en = 1;
	ld_dither_info->dither_frc_ctl.reg.frc_findex_mode = 0;

	disp_pr_debug("xres:%d, yres:%d\n", ov_data->fix_panel_info->xres, ov_data->fix_panel_info->yres);
	ld_dither_info->dither_img_size.value = 0; // todo
	ld_dither_info->dither_img_size.reg.dpp_dither_img_width = ov_data->fix_panel_info->xres - 1;
	ld_dither_info->dither_img_size.reg.dpp_dither_img_height = ov_data->fix_panel_info->yres - 1;
}

static void set_ld_dither(struct hisi_comp_operator *operator,
	struct hisi_composer_data *ov_data, char __iomem *ld_base_addr)
{
	struct hisi_ld_dither_info ld_dither_info;
	get_ld_dither_info(ov_data, &ld_dither_info);

	hisi_module_set_reg(&operator->module_desc, DPU_LD_DITHER_CTL1_ADDR(ld_base_addr),
		ld_dither_info.dither_ctl1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DITHER_CTL0_ADDR(ld_base_addr),
		ld_dither_info.dither_ctl0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_HIFREQ_REG_INI_CFG_EN_ADDR(ld_base_addr),
		ld_dither_info.hifreq_reg_ini_cfg_en.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DITHER_FRC_CTL_ADDR(ld_base_addr),
		ld_dither_info.dither_frc_ctl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DPP_DITHER_IMG_SIZE_ADDR(ld_base_addr),
		ld_dither_info.dither_img_size.value);
}

static void get_ld_core_info(struct hisi_composer_data *ov_data, char __iomem *ld_base_addr,
	struct pipeline_src_layer *src_layer, struct hisi_ld_core_info *ld_core)
{
	disp_pr_debug(" ++++ \n");
	disp_pr_debug("xres:%d, yres:%d\n", ov_data->fix_panel_info->xres, ov_data->fix_panel_info->yres);

	ld_core->ch_clk_sel.value = 0;

	ld_core->glb_ctrl.value = inp32(DPU_LD_DIM_GLB_CTRL_ADDR(ld_base_addr));
	// ld_core->glb_ctrl.reg.mode_init_led = 700; // todo
	ld_core->glb_ctrl.reg.dim2d_en = 1;
	ld_core->glb_ctrl.reg.led_en = 1;
	ld_core->glb_ctrl.reg.scd_en = 0;
	ld_core->glb_ctrl.reg.lcd_en = 1;
	ld_core->glb_ctrl.reg.dimming_en = 1;
	ld_core->glb_ctrl.reg.dim_stt_en = 1;

	ld_core->led_num.value = 0;
	ld_core->led_num.reg.led_numv = get_ld_led_numv() - 1;
	ld_core->led_num.reg.led_numh = get_ld_led_numh() - 1;

	ld_core->seg_metrics.value = 0;
	uint32_t seg_height = (ov_data->fix_panel_info->yres / 2) / get_ld_led_numv();
	uint32_t seg_width = (ov_data->fix_panel_info->xres / 2) / get_ld_led_numh();
	if (seg_height < 4) {
		disp_pr_err("seg_height bellow 4:%d", seg_height);
	}
	if (seg_width < 16) {
		disp_pr_err("seg_height bellow 16:%d", seg_width);
	}
	ld_core->seg_metrics.reg.seg_height =  seg_height; // [4,1280]
	ld_core->seg_metrics.reg.seg_width = seg_width;  // 12 / [16,1920]

	ld_core->stat_metrics.value = 0;
	ld_core->stat_metrics.reg.stat_height = ld_core->seg_metrics.reg.seg_height * 3 / 2; // 1.5 * seg
	ld_core->stat_metrics.reg.stat_width = ld_core->seg_metrics.reg.seg_width * 3 / 2; // 1.5 * seg

	ld_core->glb_norm_unit.value = 0;
	ld_core->glb_norm_unit.reg.glb_norm_unit = (ov_data->fix_panel_info->xres * ov_data->fix_panel_info->yres) / 4;

	ld_core->seg_norm_unit.value = 0;
	ld_core->seg_norm_unit.reg.seg_norm_unit =
		ld_core->stat_metrics.reg.stat_width * ld_core->stat_metrics.reg.stat_height;

	ld_core->dim_scl_ratio_v.value = inp32(DPU_LD_DIM_SCL_RATIO_V_ADDR(ld_base_addr));
	ld_core->dim_scl_ratio_v.reg.lvfir_scl_dec = get_ld_led_numv() * 65536 / ov_data->fix_panel_info->yres;

	ld_core->dim_scl_ratio_h.value = inp32(DPU_LD_DIM_SCL_RATIO_H_ADDR(ld_base_addr));
	ld_core->dim_scl_ratio_h.reg.lhfir_scl_dec = get_ld_led_numh() * 65536 / ov_data->fix_panel_info->xres;

	ld_core->demo_ireso.value = 0;
	ld_core->demo_ireso.reg.demon_width = ov_data->fix_panel_info->xres / 2 - 1; // -1
	ld_core->demo_ireso.reg.demon_height = ov_data->fix_panel_info->yres / 2 - 1; // -1

	ld_core->led_panel_reg1.value = 0;

	ld_core->input_resolution.value = 0;
	ld_core->input_resolution.reg.input_height = ov_data->fix_panel_info->yres - 1;
	ld_core->input_resolution.reg.input_width = ov_data->fix_panel_info->xres - 1;

	ld_core->dma_ctrl1.value = inp32(DPU_LD_DIM_DMA_CTRL1_ADDR(ld_base_addr)); // todo
	uint32_t stride = ALIGN_UP(get_ld_led_numh() * 2, 16); // align 16Byte
	stride = stride < (2 * (ld_core->led_num.reg.led_numh + 1)) ? (2 * (ld_core->led_num.reg.led_numh + 1)) : stride;
	ld_core->dma_ctrl1.reg.stride = stride;

	ld_core->addr2d_reg1.value = 0; // todo
	ld_core->addr2d_reg1.reg.addr_2d_low = src_layer->img.vir_addr + src_layer->img.buff_size; // todo
	disp_pr_debug("src_layer->img.vir_addr:0x%x, addr_2d_low:0x%x\n",
		src_layer->img.vir_addr, ld_core->addr2d_reg1.reg.addr_2d_low);
	disp_pr_debug("g_ld_frm_cnt:%d, led_num_h:%d, led_num_v:%d",
		get_ld_frm_cnt(), get_ld_led_numh(), get_ld_led_numv());

	if (get_ld_frm_cnt() == 0)
		set_ld_wch_last_vaddr(get_ld_wch_vaddr());

	ld_core->addr2d_reg1.reg.addr_2d_low = get_ld_wch_last_vaddr(); // todo
	if (get_ld_continue_frm_wb()) // next frame addr
		set_ld_wch_last_vaddr(get_ld_wch_last_vaddr() + ALIGN_UP(2 * get_ld_led_numh(), 16) * get_ld_led_numv());
	disp_pr_debug("g_ld_wch_vaddr:0x%x, g_ld_wch_last_vaddr:0x%x\n", get_ld_wch_vaddr(), get_ld_wch_last_vaddr());

	ld_core->output_ctrl.value = 0;
	ld_core->output_ctrl.reg.low_bit = 0;
	ld_core->output_ctrl.reg.stt2d_width = get_ld_stt2d_width(); // 0:12bit, 1:14bit

	ld_core->wb_sec_ctrl1.value = 0;
	ld_core->wb_sec_ctrl1.reg.wb_en_ns = 1;

	ld_core->ld_sync_ctrl.value = 0;
	ld_core->ld_sync_ctrl.reg.sync_sel = get_ld_sync_sel(); // 0-DPRX2DSS VACT_START, 1-DPTX FRM_START
	ld_core->ld_sync_ctrl.reg.sync_en = get_ld_sync_en();

	ld_core->ld_sync_flag_interval.value = 0;
	ld_core->ld_sync_flag_interval.reg.flag_interval = 13107; // todo

	ld_core->ld_sync_pulse_width.value = 0;
	ld_core->ld_sync_pulse_width.reg.ld_sync_pulse_width = 10; // todo
}

static void set_ld_core(struct hisi_comp_operator *operator, struct hisi_composer_data *ov_data,
	char __iomem *ld_base_addr, struct pipeline_src_layer *src_layer)
{
	disp_pr_debug(" ++++ \n");
	struct hisi_ld_core_info ld_core;
	get_ld_core_info(ov_data, ld_base_addr, src_layer, &ld_core);
	if (get_ld_continue_frm_wb())
		set_ld_frm_cnt(get_ld_frm_cnt() + 1);

	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_GLB_CTRL_ADDR(ld_base_addr), ld_core.glb_ctrl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_LED_NUM_ADDR(ld_base_addr), ld_core.led_num.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_SEG_METIRCS_ADDR(ld_base_addr), ld_core.seg_metrics.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_STAT_METRICS_ADDR(ld_base_addr),
		ld_core.stat_metrics.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_GLB_NORM_UNIT_ADDR(ld_base_addr),
		ld_core.glb_norm_unit.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_SEG_NORM_UNIT_ADDR(ld_base_addr),
		ld_core.seg_norm_unit.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_SCL_RATIO_V_ADDR(ld_base_addr),
		ld_core.dim_scl_ratio_v.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_SCL_RATIO_H_ADDR(ld_base_addr),
		ld_core.dim_scl_ratio_h.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_DEMO_IRESO_ADDR(ld_base_addr), ld_core.demo_ireso.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_LED_PANEL_REG1_ADDR(ld_base_addr),
		ld_core.led_panel_reg1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_INPUT_RESOLUTION_ADDR(ld_base_addr),
		ld_core.input_resolution.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_DMA_CTRL1_ADDR(ld_base_addr), ld_core.dma_ctrl1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_ADDR2D_REG1_ADDR(ld_base_addr), ld_core.addr2d_reg1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_CH_CLK_SEL_ADDR(ld_base_addr), ld_core.ch_clk_sel.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_DIM_OUTPUT_CTRL_ADDR(ld_base_addr), ld_core.output_ctrl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_WB_SEC_CTRL1_ADDR(ld_base_addr), ld_core.wb_sec_ctrl1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_SYNC_CTRL_ADDR(ld_base_addr), ld_core.ld_sync_ctrl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_SYNC_FLAG_INTERVAL_ADDR(ld_base_addr),
		ld_core.ld_sync_flag_interval.value);
	hisi_module_set_reg(&operator->module_desc, DPU_LD_SYNC_PULSE_WIDTH_ADDR(ld_base_addr),
		ld_core.ld_sync_pulse_width.value);

	disp_pr_debug(" ---- \n");
}

static void set_ld_lut_div_coff(struct hisi_comp_operator *operator, char __iomem *ld_base_addr)
{
	uint32_t i;
	for (i = 0; i < DIM_DIV_LUT_SIZE; i++) {
		hisi_module_set_reg(&operator->module_desc, DPU_LD_U_LD_DIV_COEF_ADDR(ld_base_addr, i), dim_div_luts[i]);
	}
}

static void set_ld_lut_h_coff(struct hisi_comp_operator *operator, char __iomem *ld_base_addr)
{
	uint32_t ld2_index = 0;
	uint32_t low_value = 0;
	uint32_t high_value = 0;
	uint32_t merge_value = 0;
	uint32_t i;
	uint32_t j;
	for (i = 0; i < DIM_HVZME_LUT_SIZE; i++) {
		for (j = 0; j < DIM_LUT_DATA_SIZE - 1; j += 2) {
			low_value = dim_hzme_luts[i][j];
			high_value = dim_hzme_luts[i][j + 1];
			merge_value = (high_value << 16) | low_value;
			hisi_module_set_reg(&operator->module_desc, DPU_LD_U_LD_H_COEF_ADDR(ld_base_addr, ld2_index++), merge_value);
		}
		low_value = dim_hzme_luts[i][DIM_LUT_DATA_SIZE - 1];
		hisi_module_set_reg(&operator->module_desc, DPU_LD_U_LD_H_COEF_ADDR(ld_base_addr, ld2_index++), low_value);
	}
	disp_pr_info("ld2_index:%d\n", ld2_index);
}

static void set_ld_lut_v_coff(struct hisi_comp_operator *operator, char __iomem *ld_base_addr)
{
	uint32_t ld2_index = 0;
	uint32_t low_value = 0;
	uint32_t high_value = 0;
	uint32_t merge_value = 0;
	uint32_t i;
	uint32_t j;
	for (i = 0; i < DIM_HVZME_LUT_SIZE; i++) {
		for (j = 0; j < DIM_LUT_DATA_SIZE - 1; j += 2) {
			low_value = dim_vzme_luts[i][j];
			high_value = dim_vzme_luts[i][j + 1];
			merge_value = (high_value << 16) | low_value;
			hisi_module_set_reg(&operator->module_desc, DPU_LD_U_LD_V_COEF_ADDR(ld_base_addr, ld2_index++), merge_value);
		}
		low_value = dim_vzme_luts[i][DIM_LUT_DATA_SIZE - 1];
		hisi_module_set_reg(&operator->module_desc, DPU_LD_U_LD_V_COEF_ADDR(ld_base_addr, ld2_index++), low_value);
	}
	disp_pr_info("ld2_index:%d\n", ld2_index);
}

static void set_ld_lut(struct hisi_comp_operator *operator, char __iomem *ld_base_addr)
{
	disp_pr_info(" ++++ \n");
	set_ld_lut_div_coff(operator, ld_base_addr);
	set_ld_lut_h_coff(operator, ld_base_addr);
	set_ld_lut_v_coff(operator, ld_base_addr);
	disp_pr_info(" ---- \n");
}

static void set_ld_en(struct hisi_comp_operator *operator, char __iomem *dpp_base_addr)
{
	hisi_module_set_reg(&operator->module_desc, DPU_DPP_LD_EN_ADDR(dpp_base_addr), 1);
}

static int g_ld_init_flag = 0;
void hisi_local_dimming_set_regs(struct hisi_comp_operator *operator, struct hisi_composer_data *ov_data,
	char __iomem *dpu_base_addr, struct pipeline_src_layer *src_layer)
{
	disp_pr_debug(" ++++ \n");
	char __iomem *dpp_base_addr;
	char __iomem *ld_base_addr;
	dpp_base_addr = dpu_base_addr + DSS_DPP0_OFFSET;
	ld_base_addr = dpu_base_addr + DPU_LD_OFFSET;
	disp_pr_debug("xres:%d, yres:%d\n", ov_data->fix_panel_info->xres, ov_data->fix_panel_info->yres);

	outp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C4, 0); // unmask ld interrupt
	outp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0, 0xF); // clear ld interrupt

	set_ld_en(operator, dpp_base_addr);

	if (g_ld_init_flag == 0) {
		set_ld_lut(operator, ld_base_addr);
		g_ld_init_flag = 1;
	}

	set_ld_core(operator, ov_data, ld_base_addr, src_layer);
	set_ld_dither(operator, ov_data, ld_base_addr);

	disp_pr_debug(" ---- \n");
}
