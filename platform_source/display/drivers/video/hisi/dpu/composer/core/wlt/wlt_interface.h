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

#include <linux/types.h>

#ifndef WLT_INTERFACE_H
#define WLT_INTERFACE_H

extern uint64_t g_debug_bbit_vdec;
extern uint64_t g_debug_bbit_venc;
extern uint64_t g_slice0_y_payload_addr;
extern uint64_t g_slice0_y_header_addr;
extern uint64_t g_slice0_c_payload_addr;
extern uint64_t g_slice0_c_header_addr;
extern uint64_t g_slice0_cmdlist_header_addr;
extern uint64_t g_slice1_cmdlist_header_addr;
extern uint64_t g_slice2_cmdlist_header_addr;
extern uint64_t g_slice3_cmdlist_header_addr;

struct slice_cmdlist_addr {
	uint32_t slice0_ddr_addr;
	uint32_t slice1_ddr_addr;
	uint32_t slice2_ddr_addr;
	uint32_t slice3_ddr_addr;
};

struct slice_frame_addr {
	uint64_t y_header_addr;
	uint64_t y_payload_addr;
	uint64_t uv_header_addr;
	uint64_t uv_payload_addr;
};

void wlt_set_slice_info(struct slice_cmdlist_addr *slice_cmdlist_addr, struct slice_frame_addr *slice0_frame_addr);
int32_t wlt_dmabuf_map_iova(struct dma_buf *dmabuf, uint64_t iova);
int32_t wlt_dmabuf_unmap_iova(struct dma_buf *dmabuf, uint64_t iova);

#endif /* WLT_INTERFACE_H */
