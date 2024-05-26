/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "../../dpu_display_effect.h"
#include "../../dpu_dpe_utils.h"
#include "../../dpu_mmbuf_manager.h"

struct dss_dump_data_type *dpufb_alloc_dumpdss(void)
{
	struct dss_dump_data_type *dumpdss = NULL;

	dumpdss = kmalloc(sizeof(*dumpdss), GFP_KERNEL);
	if (dumpdss == NULL) {
		DPU_FB_ERR("alloc dumpDss failed!\n");
		return NULL;
	}
	memset(dumpdss, 0, sizeof(*dumpdss));

	dumpdss->dss_buf_len = 0;
	dumpdss->dss_buf = kmalloc(DUMP_BUF_SIZE, GFP_KERNEL);
	if (dumpdss->dss_buf == NULL) {
		DPU_FB_ERR("alloc dss_buf failed!\n");
		kfree(dumpdss);
		dumpdss = NULL;
		return NULL;
	}
	memset(dumpdss->dss_buf, 0, DUMP_BUF_SIZE);

	DPU_FB_INFO("alloc dumpDss succeed!\n");

	return dumpdss;
}

void dpufb_free_dumpdss(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (dpufd->dumpDss != NULL) {
		if (dpufd->dumpDss->dss_buf != NULL) {
			kfree(dpufd->dumpDss->dss_buf);
			dpufd->dumpDss->dss_buf = NULL;
		}

		kfree(dpufd->dumpDss);
		dpufd->dumpDss = NULL;
	}
}

void dump_dss_overlay(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	uint32_t i;
	uint32_t k;
	dss_layer_t const *layer = NULL;
	dss_overlay_block_t *pov_block_info = NULL;

	dpu_check_and_no_retval((!dpufd), ERR, "dpufd is NULL\n");
	dpu_check_and_no_retval((!pov_req), ERR, "pov_req is NULL\n");
	dpu_check_and_no_retval((pov_req->ovl_idx < DSS_OVL0) || (pov_req->ovl_idx >= DSS_OVL_IDX_MAX),
		ERR, "ovl_idx is NULL\n");

	DPU_FB_DEBUG("frame_no=%u ovl_idx=%d release_fence=%d ov_block_nums=%u\n",
		pov_req->frame_no, pov_req->ovl_idx, pov_req->release_fence, pov_req->ov_block_nums);

	for (i = 0; i < pov_req->ov_block_nums; i++) {
		pov_block_info = &(pov_req->ov_block_infos[i]);

		DPU_FB_DEBUG("ov_block_rect[%d,%d, %d,%d] layer_nums=%d\n",
			pov_block_info->ov_block_rect.x,
			pov_block_info->ov_block_rect.y,
			pov_block_info->ov_block_rect.w,
			pov_block_info->ov_block_rect.h,
			pov_block_info->layer_nums);

		for (k = 0; k < pov_block_info->layer_nums; k++) {
			layer = &(pov_block_info->layer_infos[k]);
			DPU_FB_DEBUG("LayerInfo[%d]:\n"
				"format=%d  width=%d  height=%d  bpp=%d  buf_size=%d  stride=%d\n"
				"afbc_header_stride=%d  afbc_payload_stride=%d iova:0x%lx\n"
				"mmbuf_base=0x%x  mmbuf_size=%d  mmu_enable=%d shared_fd=%d\n"
				"src_rect[%d,%d, %d,%d]  dst_rect[%d,%d, %d,%d]\n"
				"transform=%d  blending=%d  glb_alpha=0x%x  color=0x%x\n"
				"layer_idx=%d  chn_idx=%d  need_cap=0x%x\n",
				k,
				layer->img.format,
				layer->img.width,
				layer->img.height,
				layer->img.bpp,
				layer->img.buf_size,
				layer->img.stride,
				layer->img.afbc_header_stride,
				layer->img.afbc_payload_stride,
				layer->img.vir_addr,
				layer->img.mmbuf_base,
				layer->img.mmbuf_size,
				layer->img.mmu_enable,
				layer->img.shared_fd,
				layer->src_rect.x,
				layer->src_rect.y,
				layer->src_rect.w,
				layer->src_rect.h,
				layer->dst_rect.x,
				layer->dst_rect.y,
				layer->dst_rect.w,
				layer->dst_rect.h,
				layer->transform,
				layer->blending,
				layer->glb_alpha,
				layer->color,
				layer->layer_idx,
				layer->chn_idx,
				layer->need_cap);
		}
	}
}

void dump_core_disp_info(struct dpu_core_disp_data *ov_info)
{
	uint32_t i;
	uint32_t k;
	struct block_info *block = NULL;
	struct layer_info *layer = NULL;

	dpu_check_and_no_retval(!ov_info, ERR, "ov_info is null\n");

	DPU_FB_DEBUG("frame_no=%d ovl_idx=%d ov_block_nums=%d disp_id:%u magic:0x%x\n",
		ov_info->frame_no, ov_info->ovl_idx, ov_info->ov_block_nums, ov_info->disp_id, ov_info->magic_num);

	for (i = 0; i < ov_info->ov_block_nums; i++) {
		block = &(ov_info->ov_block_infos[i]);

		DPU_FB_DEBUG("ov_block_rect[%d,%d, %d,%d] layer_nums=%d\n",
			block->ov_block_rect.x,
			block->ov_block_rect.y,
			block->ov_block_rect.w,
			block->ov_block_rect.h,
			block->layer_nums);

		for (k = 0; k < block->layer_nums; k++) {
			layer = &(block->layer_infos[k]);
			DPU_FB_DEBUG("LayerInfo[%d]:\n"
				"format=%d  width=%d  height=%d  bpp=%d  buf_size=%d  stride=%d\n"
				"afbc_header_stride=%d  afbc_payload_stride=%d\n"
				"mmbuf_base=0x%x  mmbuf_size=%d  mmu_enable=%d shared_fd=%d\n"
				"src_rect[%d,%d, %d,%d]  dst_rect[%d,%d, %d,%d]\n"
				"transform=%d  blending=%d  glb_alpha=0x%x  color=0x%x\n"
				"layer_idx=%d  chn_idx=%d  need_cap=0x%x source=%d\n",
				k,
				layer->format,
				layer->width,
				layer->height,
				layer->bpp,
				layer->buf_size,
				layer->stride,
				layer->afbc_header_stride,
				layer->afbc_payload_stride,
				layer->mmbuf_base,
				layer->mmbuf_size,
				layer->mmu_enable,
				layer->shared_fd,
				layer->src_rect.x,
				layer->src_rect.y,
				layer->src_rect.w,
				layer->src_rect.h,
				layer->dst_rect.x,
				layer->dst_rect.y,
				layer->dst_rect.w,
				layer->dst_rect.h,
				layer->transform,
				layer->blending,
				layer->glb_alpha,
				layer->color,
				layer->layer_idx,
				layer->chn_idx,
				layer->need_cap,
				layer->disp_source);
		}
	}
}

void dpu_debug_func(struct work_struct *work)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval(!work, ERR, "work is NULL\n");
	dpufd = container_of(work, struct dpu_fb_data_type, dss_debug_work);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL point!\n");
		return;
	}

	dpu_dump_current_info(dpufd);
	dump_dss_overlay(dpufd, &dpufd->ov_req);
}
