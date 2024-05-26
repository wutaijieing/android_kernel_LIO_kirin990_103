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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm_iommu.h>
#include "wlt_interface.h"
#include "hisi_disp_iommu.h"
#include "hisi_disp_debug.h"

void wlt_set_slice_info(struct slice_cmdlist_addr *cmdlist_addr, struct slice_frame_addr *slice0_frame_addr)
{
	disp_pr_debug("[%s:%d]: +++\n", __func__, __LINE__);
	g_debug_bbit_vdec = 1;
	g_slice0_cmdlist_header_addr = cmdlist_addr->slice0_ddr_addr;
	g_slice1_cmdlist_header_addr = cmdlist_addr->slice1_ddr_addr;
	g_slice2_cmdlist_header_addr = cmdlist_addr->slice2_ddr_addr;
	g_slice3_cmdlist_header_addr = cmdlist_addr->slice3_ddr_addr;

	g_slice0_y_header_addr = slice0_frame_addr->y_header_addr;
	g_slice0_y_payload_addr = slice0_frame_addr->y_payload_addr;
	g_slice0_c_header_addr = slice0_frame_addr->uv_header_addr;
	g_slice0_c_payload_addr = slice0_frame_addr->uv_payload_addr;

	disp_pr_debug("[%s:%d]: %u %u %u %u\n", __func__, __LINE__, g_slice0_cmdlist_header_addr,
		g_slice0_cmdlist_header_addr, g_slice2_cmdlist_header_addr,
		g_slice3_cmdlist_header_addr);
	disp_pr_debug("[%s:%d]: ---\n", __func__, __LINE__);
}
EXPORT_SYMBOL(wlt_set_slice_info);

// DEBUG: map shared iova for vdec
int32_t wlt_dmabuf_map_iova(struct dma_buf *dmabuf, uint64_t iova)
{
	disp_pr_debug("[%s:%d]: +++\n", __func__, __LINE__);
	if (!dmabuf) {
		disp_pr_err("[%s:%d]: dmabuf is NULL\n", __func__, __LINE__);
		return -EFAULT;
	}

	int32_t ret = hisi_dss_dmabuf_map_iova(dmabuf, iova);
	if (ret) {
		disp_pr_err("[%s:%d]: dmabuf is NULL\n", __func__, __LINE__);
		return -EFAULT;
	}

	disp_pr_debug("[%s:%d]: ---\n", __func__, __LINE__);
	return 0;
}
EXPORT_SYMBOL(wlt_dmabuf_map_iova);

// DEBUG: unmap shared iova for vdec
int32_t wlt_dmabuf_unmap_iova(struct dma_buf *dmabuf, uint64_t iova)
{
	disp_pr_debug("[%s:%d]: +++\n", __func__, __LINE__);
	if (!dmabuf) {
		disp_pr_err("[%s:%d]: dmabuf is NULL\n", __func__, __LINE__);
		return -EFAULT;
	}

	int32_t ret = hisi_dss_dmabuf_unmap_iova(dmabuf, iova);
	if (ret) {
		disp_pr_err("[%s:%d]: dmabuf is NULL\n", __func__, __LINE__);
		return -EFAULT;
	}

	disp_pr_debug("[%s:%d]: ---\n", __func__, __LINE__);
	return 0;
}
EXPORT_SYMBOL(wlt_dmabuf_unmap_iova);

// test for ecall
void wlt_debug_interface(uint32_t slice0_cmd_addr, uint32_t slice1_cmd_addr,
	uint32_t slice2_cmd_addr,  uint32_t slice3_cmd_addr,uint64_t y_header_addr,
	uint64_t y_payload_addr, uint64_t uv_header_addr, uint64_t uv_payload_addr)
{
	struct slice_cmdlist_addr cmdlist_addr;
	struct slice_frame_addr slice0_frame_addr;

	disp_pr_debug("[%s:%d]: +++\n", __func__, __LINE__);
	g_debug_bbit_vdec = 1;
	cmdlist_addr.slice0_ddr_addr = slice0_cmd_addr;
	cmdlist_addr.slice1_ddr_addr = slice1_cmd_addr;
	cmdlist_addr.slice2_ddr_addr = slice2_cmd_addr;
	cmdlist_addr.slice3_ddr_addr = slice3_cmd_addr;

	slice0_frame_addr.y_header_addr = y_header_addr;
	slice0_frame_addr.y_payload_addr = y_payload_addr;
	slice0_frame_addr.uv_header_addr = uv_header_addr;
	slice0_frame_addr.uv_payload_addr = uv_payload_addr;

	wlt_set_slice_info(&cmdlist_addr, &slice0_frame_addr);

	disp_pr_debug("[%s:%d]: ---\n", __func__, __LINE__);
}
EXPORT_SYMBOL(wlt_debug_interface);
