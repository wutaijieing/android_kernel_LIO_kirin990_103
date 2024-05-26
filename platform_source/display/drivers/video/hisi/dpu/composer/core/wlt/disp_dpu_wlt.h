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

#ifndef DISP_DPU_WLT_H
#define DISP_DPU_WLT_H

#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "cmdlist.h"
#define Y_C_LINE_INFO_SIZE 64
#define SLICE_ADDR_INFO_SIZE 64

struct dpu_dacc_wlt {
	SOC_DACC_WLT_EN_UNION wlt_en;

	SOC_DACC_WLT_SLICE0_DDR_ADDR0_UNION wlt_slice0_ddr_addr0;
	SOC_DACC_WLT_SLICE0_DDR_ADDR1_UNION wlt_slice0_ddr_addr1;
	SOC_DACC_WLT_SLICE1_DDR_ADDR0_UNION wlt_slice1_ddr_addr0;
	SOC_DACC_WLT_SLICE1_DDR_ADDR1_UNION wlt_slice1_ddr_addr1;
	SOC_DACC_WLT_SLICE2_DDR_ADDR0_UNION wlt_slice2_ddr_addr0;
	SOC_DACC_WLT_SLICE2_DDR_ADDR1_UNION wlt_slice2_ddr_addr1;
	SOC_DACC_WLT_SLICE3_DDR_ADDR0_UNION wlt_slice3_ddr_addr0;
	SOC_DACC_WLT_SLICE3_DDR_ADDR1_UNION wlt_slice3_ddr_addr1;

	SOC_DACC_WLT_SLICE0_ADDR0_H_UNION wlt_slice0_addr0_h;
	SOC_DACC_WLT_SLICE0_ADDR0_L_UNION wlt_slice0_addr0_l;
	SOC_DACC_WLT_SLICE0_ADDR1_L_UNION wlt_slice0_addr1_l;
	SOC_DACC_WLT_SLICE0_ADDR2_L_UNION wlt_slice0_addr2_l;
	SOC_DACC_WLT_SLICE0_ADDR3_L_UNION wlt_slice0_addr3_l;

	SOC_DACC_WLT_SLICE1_ADDR0_H_UNION wlt_slice1_addr0_h;
	SOC_DACC_WLT_SLICE1_ADDR0_L_UNION wlt_slice1_addr0_l;
	SOC_DACC_WLT_SLICE1_ADDR1_L_UNION wlt_slice1_addr1_l;
	SOC_DACC_WLT_SLICE1_ADDR2_L_UNION wlt_slice1_addr2_l;
	SOC_DACC_WLT_SLICE1_ADDR3_L_UNION wlt_slice1_addr3_l;

	SOC_DACC_WLT_SLICE2_ADDR0_H_UNION wlt_slice2_addr0_h;
	SOC_DACC_WLT_SLICE2_ADDR0_L_UNION wlt_slice2_addr0_l;
	SOC_DACC_WLT_SLICE2_ADDR1_L_UNION wlt_slice2_addr1_l;
	SOC_DACC_WLT_SLICE2_ADDR2_L_UNION wlt_slice2_addr2_l;
	SOC_DACC_WLT_SLICE2_ADDR3_L_UNION wlt_slice2_addr3_l;

	SOC_DACC_WLT_SLICE3_ADDR0_H_UNION wlt_slice3_addr0_h;
	SOC_DACC_WLT_SLICE3_ADDR0_L_UNION wlt_slice3_addr0_l;
	SOC_DACC_WLT_SLICE3_ADDR1_L_UNION wlt_slice3_addr1_l;
	SOC_DACC_WLT_SLICE3_ADDR2_L_UNION wlt_slice3_addr2_l;
	SOC_DACC_WLT_SLICE3_ADDR3_L_UNION wlt_slice3_addr3_l;

	SOC_DACC_WLT_SLICE_ROW_UNION wlt_slice_row;
	SOC_DACC_WLT_CMDLIST_REFRESH_OFFSET_UNION wlt_cmdlist_refresh_offset;
};

struct wlt_write_back_dma_addr{
	unsigned int  y_line : 16;
	unsigned int  reserved0[3];
	unsigned int  reserved1 : 13;
	unsigned int  y_slice_num : 3;
	unsigned int  c_line : 16;
	unsigned int  reserved2[3];
	unsigned int  reserved3 : 13;
	unsigned int  c_slice_num : 3;
	unsigned int  reserved4[8];
};

int disp_dpu_wlt_set_writeback_addr(struct disp_wb_layer *layer);
int disp_dpu_wlt_set_slice_addr(struct pipeline_src_layer *src_layer);
void disp_dpu_wlt_set_dacc_cfg_wlt_regs(struct hisi_composer_device *composer_device,
	struct dpu_dacc_wlt *wlt_info, char __iomem *dpu_base_addr);
int disp_dpu_wlt_set_dacc_cfg_wltinfo(struct pipeline_src_layer *src_layer,
	struct dpu_dacc_wlt *wlt_info);

void disp_dpu_disable_wlt_set_dacc_cfg_wltinfo(struct pipeline_src_layer *src_layer,
	struct dpu_dacc_wlt *wlt_info);
void disp_dpu_disable_wlt_set_dacc_cfg_wlt_regs(struct hisi_composer_device *composer_device,
	struct dpu_dacc_wlt *wlt_info, char __iomem *dpu_base_addr);

#endif /* DISP_DPU_WLT_H */

