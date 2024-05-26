/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "disp_dpu_wlt.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include "adaptor/hisi_dss.h"
#include "hisi_operator_tool.h"

#include <asm/uaccess.h>
#include "hisi_disp_iommu.h"
#include "cmdlist.h"

static void compress_slice_addr_config(struct pipeline_src_layer *src_layer)
{
	uint32_t i;
	uint32_t y_slice_row;
	uint32_t c_slice_row;
	uint32_t wlt_slice_row;

	wlt_slice_row = ALIGN_UP(src_layer->img.height / src_layer->slice_num, 64);
	disp_pr_info("wlt_slice_row = %d\n", wlt_slice_row);

	y_slice_row = wlt_slice_row / 8; /* cal hebce block */
	c_slice_row = wlt_slice_row / 2 / 8;

	disp_pr_info("hebc y_slice_row %d, c_slice_row %d\n", y_slice_row, c_slice_row);

	for (i = 0; i < WLT_SLICE_MAX_COUNT; i++) {
		src_layer->block_info.slice_planes[i][YUV_PANEL_Y].payload_addr =
			src_layer->img.hebc_planes[HEBC_PANEL_Y].payload_addr +
			y_slice_row * src_layer->img.hebc_planes[HEBC_PANEL_Y].payload_stride * i;
		src_layer->block_info.slice_planes[i][YUV_PANEL_Y].header_addr =
			src_layer->img.hebc_planes[HEBC_PANEL_Y].header_addr +
			y_slice_row * src_layer->img.hebc_planes[HEBC_PANEL_Y].header_stride * i;
		src_layer->block_info.slice_planes[i][YUV_PANEL_UV].payload_addr =
			src_layer->img.hebc_planes[HEBC_PANEL_UV].payload_addr +
			c_slice_row * src_layer->img.hebc_planes[HEBC_PANEL_UV].payload_stride * i;
		src_layer->block_info.slice_planes[i][YUV_PANEL_UV].header_addr =
			src_layer->img.hebc_planes[HEBC_PANEL_UV].header_addr +
			c_slice_row * src_layer->img.hebc_planes[HEBC_PANEL_UV].header_stride * i;
	}

	src_layer->block_info.wlt_slice_row = wlt_slice_row;
}

static void no_compress_slice_addr_config(struct pipeline_src_layer *src_layer)
{
	uint32_t i;
	uint32_t y_payload_stride;
	uint32_t y_payload_slice_row;
	uint32_t c_payload_stride;
	uint32_t c_payload_slice_row;
	uint32_t y_payload_addr;
	uint32_t c_payload_addr;

	y_payload_stride = src_layer->img.stride;
	c_payload_stride = y_payload_stride;
	y_payload_slice_row = ALIGN_UP(src_layer->img.height / src_layer->slice_num, 64); // 0-900 aligned to 8
	c_payload_slice_row = y_payload_slice_row / 2;

	disp_pr_info("y_payload_slice_row = %d\n", y_payload_slice_row);

	y_payload_addr = src_layer->img.vir_addr;
	c_payload_addr = src_layer->img.vir_addr + src_layer->img.offset_plane1;

	for (i = 0; i < WLT_SLICE_MAX_COUNT; i++) {
		src_layer->block_info.slice_planes[i][YUV_PANEL_Y].payload_addr =
			y_payload_addr + y_payload_slice_row * y_payload_stride * i;
		src_layer->block_info.slice_planes[i][YUV_PANEL_Y].header_addr =
			c_payload_addr + c_payload_slice_row * c_payload_stride * i;
		src_layer->block_info.slice_planes[i][YUV_PANEL_UV].payload_addr = 0;
		src_layer->block_info.slice_planes[i][YUV_PANEL_UV].header_addr = 0;
		disp_pr_info("payload_addr = 0x%x header_addr = 0x%x \n",
			src_layer->block_info.slice_planes[i][YUV_PANEL_Y].payload_addr,
			src_layer->block_info.slice_planes[i][YUV_PANEL_Y].header_addr);
	}

	src_layer->block_info.wlt_slice_row = y_payload_slice_row;
}

static void compress_slice_addr_config_by_vdec(struct pipeline_src_layer *src_layer)
{
	uint32_t wlt_slice_row;

	wlt_slice_row = ALIGN_UP(src_layer->img.height / src_layer->slice_num, 64);
	disp_pr_info("wlt_slice_row = %d\n", wlt_slice_row);

	src_layer->block_info.slice_planes[0][YUV_PANEL_Y].payload_addr = g_slice0_y_payload_addr;
	src_layer->block_info.slice_planes[0][YUV_PANEL_Y].header_addr = g_slice0_y_header_addr;
	src_layer->block_info.slice_planes[0][YUV_PANEL_UV].payload_addr = g_slice0_c_payload_addr;
	src_layer->block_info.slice_planes[0][YUV_PANEL_UV].header_addr = g_slice0_c_header_addr;

	src_layer->block_info.wlt_slice_row = wlt_slice_row;
}

static void no_compress_slice_addr_config_by_vdec(struct pipeline_src_layer *src_layer)
{
	uint32_t wlt_slice_row;

	wlt_slice_row = ALIGN_UP(src_layer->img.height / src_layer->slice_num, 64);
	disp_pr_info("wlt_slice_row = %d\n", wlt_slice_row);

	/* for NV12 */
	src_layer->block_info.slice_planes[0][YUV_PANEL_Y].payload_addr =
		g_slice0_y_payload_addr; // this means slice0 Y addr for linear format
	src_layer->block_info.slice_planes[0][YUV_PANEL_Y].header_addr =
		g_slice0_c_payload_addr; // this means slice0 C addr for linear format
	src_layer->block_info.slice_planes[0][YUV_PANEL_UV].payload_addr = 0;
	src_layer->block_info.slice_planes[0][YUV_PANEL_UV].header_addr = 0; // g_slice0_c_header_addr;

	src_layer->block_info.wlt_slice_row = wlt_slice_row;
}

static int prepare_cmdlist_node(struct pipeline_src_layer *src_layer)
{
	uint32_t i = 0;
	uint32_t slice_offset = 0x20;
	uint32_t dblk_offset = 0x04;
	struct cmdlist_client *node = NULL;
	struct cmdlist_client *_node_ = NULL;
	static struct cmdlist_client *client[WLT_SLICE_MAX_COUNT] = {NULL};

	for (i = 0; i < WLT_SLICE_MAX_COUNT; i++) {
		cmdlist_release_client(client[i]);
		client[i] = NULL;
	}

	for (i = 0; i < WLT_SLICE_MAX_COUNT; i++) {
		if (src_layer->slice_num > i) {
			client[i] = dpu_cmdlist_create_client(DPU_SCENE_ONLINE_0, TRANSPORT_MODE);
			if (client[i] == NULL) {
				disp_pr_err("slice%d client is NULL\n", i);
				return -1;
			}

			dpu_cmdlist_set_reg(client[i],
				SOC_DACC_WLT_SLICE0_ADDR0_H_ADDR(DACC_OFFSET + DMC_OFFSET) +
				slice_offset * i, 0x0);
			dpu_cmdlist_set_reg(client[i],
				SOC_DACC_WLT_SLICE0_ADDR0_L_ADDR(DACC_OFFSET + DMC_OFFSET) +
				slice_offset * i, src_layer->block_info.slice_planes[i][YUV_PANEL_Y].payload_addr);
			dpu_cmdlist_set_reg(client[i],
				SOC_DACC_WLT_SLICE0_ADDR1_L_ADDR(DACC_OFFSET + DMC_OFFSET) +
				slice_offset * i, src_layer->block_info.slice_planes[i][YUV_PANEL_Y].header_addr);
			dpu_cmdlist_set_reg(client[i],
				SOC_DACC_WLT_SLICE0_ADDR2_L_ADDR(DACC_OFFSET + DMC_OFFSET) +
				slice_offset * i, src_layer->block_info.slice_planes[i][YUV_PANEL_UV].payload_addr);
			dpu_cmdlist_set_reg(client[i],
				SOC_DACC_WLT_SLICE0_ADDR3_L_ADDR(DACC_OFFSET + DMC_OFFSET) +
				slice_offset * i, src_layer->block_info.slice_planes[i][YUV_PANEL_UV].header_addr);
			dpu_cmdlist_set_reg(client[i], 0x0, 0x0);
			dpu_cmdlist_set_reg(client[i],
				DPU_UVSAMP_DBLK_SLICE0_INFO_ADDR(DSS_UVSAMP0_OFFSET) +
				dblk_offset * i, 0x6F);

			list_for_each_entry_safe(node, _node_, client[i]->scene_list_header, list_node) {
				_node_->list_cmd_header->cmd_flag.ul32 = 0xa5600002;
				break;
			}
			src_layer->block_info.slice_addr[i] = client[i]->list_cmd_header->next_list;

			disp_pr_info("list_cmd_header->next_list=[0x%x]\n", client[i]->list_cmd_header->next_list);
			dpu_cmdlist_dump_client(client[i]);
		}
	}

	return 0;
}

int disp_dpu_wlt_set_writeback_addr(struct disp_wb_layer *layer)
{
	disp_check_and_return((layer == NULL), -EINVAL, err, "NULL ptr.\n");
	disp_pr_debug("wlt_dma_addr is 0x%x\n", layer->dst.wlt_dma_addr);

	layer->dst.wlt_dma_addr = (layer->dst.mmu_enable == 1) ? layer->dst.vir_addr : layer->dst.phy_addr;
	if (g_debug_bbit_venc == 1) {
		layer->dst.wlt_dma_addr = layer->dst.wlt_dma_addr + count_from_zero(layer->dst.buff_size) + Y_C_LINE_INFO_SIZE;
	} else {
		layer->dst.wlt_dma_addr = layer->dst.wlt_dma_addr + count_from_zero(layer->dst.buff_size) - Y_C_LINE_INFO_SIZE;
	}
	layer->dst.wlt_dma_addr = ALIGN_DOWN(layer->dst.wlt_dma_addr, 64);
	disp_pr_debug("set wlt_dma_addr is 0x%x\n", layer->dst.wlt_dma_addr);
	return 0;
}

int disp_dpu_wlt_set_slice_info_addr(struct pipeline_src_layer *src_layer)
{
	int ret = 0;

	disp_check_and_return((src_layer == NULL), -EINVAL, err, "NULL ptr.\n");

	disp_pr_info("g_debug_bbit_vdec=0x%x\n", g_debug_bbit_vdec);

	if (g_debug_bbit_vdec == 0) {
		if (src_layer->need_caps & CAP_HEBCD)
			compress_slice_addr_config(src_layer);
		else
			no_compress_slice_addr_config(src_layer);

		prepare_cmdlist_node(src_layer);
	} else {
		if (src_layer->need_caps & CAP_HEBCD)
			compress_slice_addr_config_by_vdec(src_layer);
		else
			no_compress_slice_addr_config_by_vdec(src_layer);

		/* physical addr */
		src_layer->block_info.slice_addr[0] = g_slice0_cmdlist_header_addr;
		src_layer->block_info.slice_addr[1] = g_slice1_cmdlist_header_addr;
		src_layer->block_info.slice_addr[2] = g_slice2_cmdlist_header_addr;
		src_layer->block_info.slice_addr[3] = g_slice3_cmdlist_header_addr;
	}

	disp_pr_info("block_info vir addr = 0x%x buffer_size = (0x%x) slice addr 0x%x 0x%x 0x%x 0x%x.\n",
		src_layer->img.vir_addr,
		src_layer->img.buff_size,
		src_layer->block_info.slice_addr[0],
		src_layer->block_info.slice_addr[1],
		src_layer->block_info.slice_addr[2],
		src_layer->block_info.slice_addr[3]);

	return ret;
}

int disp_dpu_wlt_set_dacc_cfg_wltinfo(struct pipeline_src_layer *src_layer,
	struct dpu_dacc_wlt *wlt_info)
{
	int ret = 0;
	disp_pr_info(" ++++ \n");

	wlt_info->wlt_en.value = 0;
	wlt_info->wlt_en.reg.wlt_en = 1;
	wlt_info->wlt_en.reg.wlt_layer_id = src_layer->layer_id;

	ret = disp_dpu_wlt_set_slice_info_addr(src_layer);
	if (ret)
		return -1;

	wlt_info->wlt_slice0_ddr_addr0.value = 0;
	wlt_info->wlt_slice0_ddr_addr0.reg.wlt_slice0_ddr_addr0 = src_layer->block_info.slice_addr[0];
	wlt_info->wlt_slice0_ddr_addr1.value = 0;

	if (src_layer->slice_num > 1) {
		wlt_info->wlt_slice1_ddr_addr0.value = 0;
		wlt_info->wlt_slice1_ddr_addr0.reg.wlt_slice1_ddr_addr0 = src_layer->block_info.slice_addr[1];
		wlt_info->wlt_slice1_ddr_addr1.value = 0;
	}

	if (src_layer->slice_num > 2) {
		wlt_info->wlt_slice2_ddr_addr0.value = 0;
		wlt_info->wlt_slice2_ddr_addr0.reg.wlt_slice2_ddr_addr0 = src_layer->block_info.slice_addr[2];
		wlt_info->wlt_slice2_ddr_addr1.value = 0;
	}

	if (src_layer->slice_num > 3) {
		wlt_info->wlt_slice3_ddr_addr0.value = 0;
		wlt_info->wlt_slice3_ddr_addr0.reg.wlt_slice3_ddr_addr0 = src_layer->block_info.slice_addr[3];
		wlt_info->wlt_slice3_ddr_addr1.value = 0;
	}

	wlt_info->wlt_slice0_addr0_h.value = 0;
	wlt_info->wlt_slice0_addr0_l.value = 0;
	wlt_info->wlt_slice0_addr0_l.reg.wlt_slice0_addr0_l = src_layer->block_info.slice_planes[0][YUV_PANEL_Y].payload_addr;
	wlt_info->wlt_slice0_addr1_l.value = 0;
	wlt_info->wlt_slice0_addr1_l.reg.wlt_slice0_addr1_l = src_layer->block_info.slice_planes[0][YUV_PANEL_Y].header_addr;
	wlt_info->wlt_slice0_addr2_l.value = 0;
	wlt_info->wlt_slice0_addr2_l.reg.wlt_slice0_addr2_l = src_layer->block_info.slice_planes[0][YUV_PANEL_UV].payload_addr;
	wlt_info->wlt_slice0_addr3_l.value = 0;
	wlt_info->wlt_slice0_addr3_l.reg.wlt_slice0_addr3_l = src_layer->block_info.slice_planes[0][YUV_PANEL_UV].header_addr;

	wlt_info->wlt_slice_row.value = 0;
	wlt_info->wlt_slice_row.reg.wlt_slice_row = src_layer->block_info.wlt_slice_row;

	wlt_info->wlt_cmdlist_refresh_offset.value = 0;
	wlt_info->wlt_cmdlist_refresh_offset.reg.wlt_cmdlist_refresh_offset = 24; // 8; /* empirical value */
	return 0;
}

void disp_dpu_wlt_set_dacc_cfg_wlt_regs(struct hisi_composer_device *composer_device,
	struct dpu_dacc_wlt *wlt_info, char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ \n");

	char __iomem *dacc_cfg_base = dpu_base_addr + DACC_OFFSET + DMC_OFFSET;
	struct dpu_module_desc module_desc;
	module_desc.client = composer_device->client;

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_ADDR0_H_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_addr0_h.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_ADDR0_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_addr0_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_ADDR1_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_addr1_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_ADDR2_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_addr2_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_ADDR3_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_addr3_l.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_ADDR0_H_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_addr0_h.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_ADDR0_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_addr0_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_ADDR1_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_addr1_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_ADDR2_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_addr2_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_ADDR3_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_addr3_l.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_ADDR0_H_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_addr0_h.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_ADDR0_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_addr0_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_ADDR1_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_addr1_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_ADDR2_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_addr2_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_ADDR3_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_addr3_l.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_ADDR0_H_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_addr0_h.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_ADDR0_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_addr0_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_ADDR1_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_addr1_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_ADDR2_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_addr2_l.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_ADDR3_L_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_addr3_l.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE_ROW_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice_row.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_CMDLIST_REFRESH_OFFSET_ADDR(dacc_cfg_base),
		wlt_info->wlt_cmdlist_refresh_offset.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_DDR_ADDR0_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_ddr_addr0.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE0_DDR_ADDR1_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice0_ddr_addr1.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_DDR_ADDR0_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_ddr_addr0.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE1_DDR_ADDR1_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice1_ddr_addr1.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_DDR_ADDR0_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_ddr_addr0.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE2_DDR_ADDR1_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice2_ddr_addr1.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_DDR_ADDR0_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_ddr_addr0.value);
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_SLICE3_DDR_ADDR1_ADDR(dacc_cfg_base),
		wlt_info->wlt_slice3_ddr_addr1.value);

	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_EN_ADDR(dacc_cfg_base),
		wlt_info->wlt_en.value);
}

void disp_dpu_disable_wlt_set_dacc_cfg_wltinfo(struct pipeline_src_layer *src_layer,
	struct dpu_dacc_wlt *wlt_info)
{
	disp_pr_info(" ++++ \n");
	wlt_info->wlt_en.value = 0;
	wlt_info->wlt_en.reg.wlt_en = 0;
	wlt_info->wlt_en.reg.wlt_layer_id = INVALID_LAYER_ID;
	disp_pr_info(" ---- \n");
}

void disp_dpu_disable_wlt_set_dacc_cfg_wlt_regs(struct hisi_composer_device *composer_device,
	struct dpu_dacc_wlt *wlt_info, char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ \n");
	char __iomem *dacc_cfg_base = dpu_base_addr + DACC_OFFSET + DMC_OFFSET;
	struct dpu_module_desc module_desc;
	module_desc.client = composer_device->client;
	hisi_module_set_reg(&module_desc, SOC_DACC_WLT_EN_ADDR(dacc_cfg_base),
		wlt_info->wlt_en.value);
	disp_pr_info(" ---- \n");
}

