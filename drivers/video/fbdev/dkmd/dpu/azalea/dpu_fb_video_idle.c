/* Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

#include "dpu_fb.h"
#include "dpu_fb_video_idle.h"

#define ONLINE_WB_TIMEOUT_COUNT 16600
#define DSS_CLEAR_TIMEOUT 1000
#define BUFFER_RELEASE_LAYER_CNT 3

static void dpufb_video_idle_feature_init(struct dpufb_video_idle_ctrl *video_idle_ctrl)
{
	video_idle_ctrl->gpu_compose_idle_frame = VIDEO_IDLE_GPU_COMPOSE_ENABLE;
	video_idle_ctrl->policy_id = DISPLAY_LB_POLICY_ID;
	video_idle_ctrl->l3cache_size = L3CACHE_AVAILABLE_SIZE;
	video_idle_ctrl->l3_request_size = L3CACHE_REQUEST_SIZE;
	video_idle_ctrl->lb_size = LB_AVAILABLE_SIZE;
}

static void dpufb_video_idle_init(struct dpu_fb_data_type *dpufd)
{
	uint32_t bpp;
	dss_rect_t rect;
	struct dpufb_video_idle_ctrl *video_idle_ctrl = NULL;

	video_idle_ctrl = &(dpufd->video_idle_ctrl);
	video_idle_ctrl->dpufd = dpufd;

	video_idle_ctrl->afbc_enable = false;
	video_idle_ctrl->mmu_enable = false;
	video_idle_ctrl->compress_enable = true;

	video_idle_ctrl->buffer_alloced = false;
	video_idle_ctrl->video_idle_wb_status = false;
	video_idle_ctrl->video_idle_rb_status = false;
	video_idle_ctrl->rb_closed_reg.need_recovery = false;
	video_idle_ctrl->idle_frame_display = false;

	video_idle_ctrl->rch_idx = DSS_RCHN_D2;
	video_idle_ctrl->wch_idx = DSS_WCHN_W0;
	video_idle_ctrl->wdma_format = DMA_PIXEL_FORMAT_ARGB_8888;
	video_idle_ctrl->ovl_idx = DSS_OVL0;

	video_idle_ctrl->buf = NULL;
	video_idle_ctrl->buf_size = 0;

	video_idle_ctrl->last_frm_num = 0;

	dpufb_video_idle_feature_init(video_idle_ctrl);

	rect.x = 0;
	rect.y = 0;
	rect.w = dpufd->panel_info.xres;
	rect.h = dpufd->panel_info.yres;

	if (video_idle_ctrl->wdma_format == DMA_PIXEL_FORMAT_RGB_565)
		bpp = 2;  /* BIT PER PIEXL, 2 - LCD_RGB565 */
	else if (video_idle_ctrl->wdma_format == DMA_PIXEL_FORMAT_ARGB_8888)
		bpp = 4;  /* BIT PER PIEXL, 4 - LCD_RGBA8888 */
	else
		bpp = 4;

	video_idle_ctrl->wb_buffer_size = ALIGN_UP((uint32_t)rect.w * bpp, DMA_STRIDE_ALIGN) * rect.h;
	DPU_FB_INFO("video_idle_ctrl->wb_buffer_size = 0x%x\n", video_idle_ctrl->wb_buffer_size);

	mipi_ifbc_get_rect(dpufd, &rect);
	video_idle_ctrl->wb_hsize = ALIGN_UP((uint32_t)rect.w, 8);
	video_idle_ctrl->wb_pad_num = ALIGN_UP((uint32_t)rect.w, 8) - rect.w;

	if (video_idle_ctrl->compress_enable)
		video_idle_ctrl->wb_pack_hsize = ALIGN_UP((uint32_t)rect.w, 8) * 3 / 4;  /* 4byte Align */
	else
		video_idle_ctrl->wb_pack_hsize = ALIGN_UP((uint32_t)rect.w, 8);

	video_idle_ctrl->wdfc_pad_hsize = ALIGN_UP(video_idle_ctrl->wb_pack_hsize, 4);
	video_idle_ctrl->wdfc_pad_num = video_idle_ctrl->wdfc_pad_hsize - video_idle_ctrl->wb_pack_hsize;
	video_idle_ctrl->wb_vsize = (uint32_t)rect.h;
}

static int dpufb_video_idle_l3cache_request
	(struct dpufb_video_idle_ctrl *video_idle_ctrl)
{
	int ret = 0;

	if (video_idle_ctrl->l3cache_size == 0)
		return ret;

#ifdef CONFIG_L3CACHE_SHARE
	video_idle_ctrl->request_params.id = DSS_IDLE;
	video_idle_ctrl->request_params.request_size =
		video_idle_ctrl->l3_request_size;
	video_idle_ctrl->request_params.type = PRIVATE;
	ret = l3_cache_request(&(video_idle_ctrl->request_params));
	if (ret)
		DPU_FB_ERR("l3_cache_request fail, ret = %d\n", ret);
#endif

	return ret;
}

static void dpufb_video_idle_l3cache_release
	(struct dpufb_video_idle_ctrl *video_idle_ctrl)
{
	if (video_idle_ctrl->l3cache_size == 0)
		return;

#ifdef CONFIG_L3CACHE_SHARE
	video_idle_ctrl->release_params.id = DSS_IDLE;
	l3_cache_release(&(video_idle_ctrl->release_params));
#endif
}

irqreturn_t dpufb_video_idle_dss_wb_isr(int irq, void *ptr)
{
	uint32_t isr_s1;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpufb_video_idle_ctrl *video_idle_ctrl = NULL;

	dpufd = (struct dpu_fb_data_type *)ptr;
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return IRQ_NONE;
	}
	video_idle_ctrl = &(dpufd->video_idle_ctrl);
	isr_s1 = inp32(dpufd->dss_base + DSS_WB_OFFSET + WB_ONLINE_ERR_INTS);
	outp32(dpufd->dss_base + DSS_WB_OFFSET + WB_ONLINE_ERR_INTS, isr_s1);
	if ((isr_s1 & BIT_WB_ONLINE_ERR_INTS) == BIT_WB_ONLINE_ERR_INTS)
		schedule_work(&(video_idle_ctrl->wb_err_work));

	return IRQ_HANDLED;
}

void dpufb_video_idle_ctrl_register(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpufb_video_idle_ctrl *vic = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "dpufd is NULL\n");
		return;
	}

	vic = &(dpufd->video_idle_ctrl);
	if (vic->idle_ctrl_created) {
		DPU_FB_INFO("video_idle_ctrl had registered!\n");
		return;
	}

	if (!is_video_idle_ctrl_mode(dpufd)) {
		DPU_FB_INFO("do not support video idle!\n");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_INFO("dpufd is not primary panel!\n");
		return;
	}

	dpufb_video_idle_init(dpufd);

	mutex_init(&(vic->video_idle_ctrl_lock));

	vic->idle_ctrl_created = 1;
}

void dpufb_video_idle_ctrl_unregister(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpufb_video_idle_ctrl *vic = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "dpufd is NULL\n");
		return;
	}

	if (!is_video_idle_ctrl_mode(dpufd))
		return;

	vic = &(dpufd->video_idle_ctrl);
	if (!vic->idle_ctrl_created)
		return;

	vic->idle_ctrl_created = 0;
}

static dss_layer_t *get_video_idle_layer_info(struct dpu_fb_data_type *dpufd)
{
	dss_overlay_t *pov_req = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;

	pov_req = &(dpufd->ov_req);
	pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	if (pov_req->ov_block_nums > 1) {
		DPU_FB_ERR("only support one block config\n");
		return NULL;
	}

	pov_h_block = &(pov_h_block_infos[0]);
	if (pov_h_block->layer_nums > 1) {
		DPU_FB_ERR("only support one layerconfig\n");
		return NULL;
	}

	return (&(pov_h_block->layer_infos[0]));
}

static void dpufb_video_idle_update_cache_size(
	struct dpufb_video_idle_ctrl *video_idle_ctrl,
	dss_layer_t *layer)
{
	/* update l3 cache size */
	video_idle_ctrl->l3cache_size = layer->img.buf_size > L3CACHE_AVAILABLE_SIZE ?
		L3CACHE_AVAILABLE_SIZE : layer->img.buf_size;

	/* update lb cache size, lb size > 0 means support lb */
	if ((LB_AVAILABLE_SIZE > 0) && (layer->img.buf_size > video_idle_ctrl->l3cache_size))
		video_idle_ctrl->lb_size = layer->img.buf_size - video_idle_ctrl->l3cache_size;
}

static uint64_t dpufb_video_idle_buffer_map(struct dpu_fb_data_type *dpufd,
	struct dma_buf *buf,
	dss_layer_t *layer)
{
	uint64_t map_addr = 0;

	DPU_FB_DEBUG("buffer map +++++\n");

	map_addr = mm_iommu_idle_display_map(__hdss_get_dev(),
		dpufd->video_idle_ctrl.policy_id, buf, layer->img.buf_size,
		dpufd->video_idle_ctrl.l3cache_size, dpufd->video_idle_ctrl.lb_size);

	DPU_FB_DEBUG("buffer map -----\n");

	return map_addr;
}

static void save_video_idle_layer_img_info(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	layer->img.vir_addr = dpufd->video_idle_ctrl.wb_vir_addr;
	layer->img.afbc_header_addr = dpufd->video_idle_ctrl.wb_vir_addr + layer->img.afbc_header_offset;
	layer->img.afbc_payload_addr = dpufd->video_idle_ctrl.wb_vir_addr + layer->img.afbc_payload_offset;

	DPU_FB_DEBUG("vir_addr=0x%lx, header=0x%lx, payload=0x%lx, size-all=0x%x, l3=0x%x, lb=0x%x\n",
		layer->img.vir_addr, layer->img.afbc_header_addr, layer->img.afbc_payload_addr, layer->img.buf_size,
		dpufd->video_idle_ctrl.l3cache_size, dpufd->video_idle_ctrl.lb_size);
}

static int dpufb_video_idle_alloc_cache(struct dpu_fb_data_type *dpufd)
{
	struct dma_buf *buf = NULL;
	dss_layer_t *layer = NULL;
	int ret;

	layer = get_video_idle_layer_info(dpufd);
	dpu_check_and_return(!layer, 0, INFO, "get_video_idle_layer_info fail\n");
	buf = dpu_get_dmabuf(layer->img.shared_fd);

	dpufb_video_idle_update_cache_size(&(dpufd->video_idle_ctrl), layer);

	if (dpufb_video_idle_l3cache_request(&(dpufd->video_idle_ctrl))) {
		DPU_FB_ERR("l3cache request failed\n");
		goto l3cache_request_fail;
	}

	dpufd->video_idle_ctrl.wb_vir_addr = dpufb_video_idle_buffer_map(dpufd, buf, layer);
	if (dpufd->video_idle_ctrl.wb_vir_addr == 0) {
		DPU_FB_ERR("dpufb_video_idle_buffer_map fail\n");
		goto map_fail;
	}

	ret = dma_buf_begin_cpu_access(buf, DMA_FROM_DEVICE);
	if (ret) {
		DPU_FB_ERR("dma_buf_begin_cpu_access fail\n");
		goto map_fail;
	}

	/* save buf */
	dpufd->video_idle_ctrl.buf = buf;
	dpufd->video_idle_ctrl.buf_size = layer->img.buf_size;

	save_video_idle_layer_img_info(dpufd, layer);

	dpufd->video_idle_ctrl.buffer_alloced = true;

	DPU_FB_DEBUG("alloc cache success\n");
	return 0;

map_fail:
	dpufb_video_idle_l3cache_release(&(dpufd->video_idle_ctrl));
l3cache_request_fail:
	dpu_put_dmabuf(buf);
	return -1;
}

int dpufb_video_idle_release_cache(struct dpu_fb_data_type *dpufd)
{
	struct dpufb_video_idle_ctrl *vic = &(dpufd->video_idle_ctrl);

	DPU_FB_DEBUG("release cache ++++\n");
	if (mutex_trylock(&(vic->video_idle_ctrl_lock))) {
		if (!vic->gpu_compose_idle_frame) {
			mutex_unlock(&(vic->video_idle_ctrl_lock));
			return 0;
		}

		if (!vic->buffer_alloced) {
			DPU_FB_DEBUG("fb%d buffer freed\n", dpufd->index);
			mutex_unlock(&(vic->video_idle_ctrl_lock));
			return 0;
		}

		(void)mm_iommu_idle_display_unmap(__hdss_get_dev(), vic->wb_vir_addr, vic->buf_size,
			vic->policy_id, vic->buf);
		dpufb_video_idle_l3cache_release(vic);

		dpu_put_dmabuf(vic->buf);
		vic->buf = NULL;
		vic->buf_size = 0;
		vic->wb_buffer_base = 0;
		vic->wb_phys_addr = 0;
		vic->wb_vir_addr = 0;
		vic->buffer_alloced = false;

		DPU_FB_DEBUG("release cache succ\n");
		mutex_unlock(&(vic->video_idle_ctrl_lock));
	}

	DPU_FB_DEBUG("release cache ------\n");

	return 0;
}

static bool is_video_idle_in_secure_mode(struct dpu_fb_data_type *dpufd)
{
	int i;
	int m;
	dss_layer_t *layer = NULL;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;

	if (dpufd->ov_req.sec_enable_status == DSS_SEC_ENABLE)
		return true;

	pov_req = &(dpufd->ov_req);
	pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < (int)pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);
		for (i = 0; i < (int)pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			if (layer->img.secure_mode == 1)
				return true;
		}
	}

	return false;
}

static int dpufb_gpu_compose_video_idle_ctrl
	(struct dpu_fb_data_type *dpufd, uint32_t video_idle_status)
{
	int ret = 0;
	static int dss_free_cache_refcount;

	if (is_video_idle_in_secure_mode(dpufd)) {
		DPU_FB_DEBUG("video idle in secure mode!\n");
		goto exit;
	}

	if (video_idle_status) {
		if (dpufd->video_idle_ctrl.buffer_alloced) {
			DPU_FB_INFO("free_refcount <= 3, release and remap\n");
			(void)dpufb_video_idle_release_cache(dpufd);
			return ret;
		}

		ret = dpufb_video_idle_alloc_cache(dpufd);
		if (ret < 0) {
			DPU_FB_ERR("alloc cache fail\n");
			return ret;
		}
		dss_free_cache_refcount = 0;
		dpufd->video_idle_ctrl.last_frm_num = dpufd->ov_req.frame_no;
		dpufd->video_idle_ctrl.idle_frame_display = true;
		DPU_FB_DEBUG("enter idle display\n");
		return ret;
	}

exit:
	if (dss_free_cache_refcount == 0)
		DPU_FB_DEBUG("exit idle display\n");

	if (dpufd->video_idle_ctrl.buffer_alloced) {
		dss_free_cache_refcount++;
		if (dss_free_cache_refcount > BUFFER_RELEASE_LAYER_CNT) {
			dss_free_cache_refcount = 1;  /* avoid overflow */
			(void)dpufb_video_idle_release_cache(dpufd);
		}
	}

	dpufd->video_idle_ctrl.idle_frame_display = false;

	return ret;
}

int dpufb_video_panel_idle_display_ctrl(struct dpu_fb_data_type *dpufd, uint32_t video_idle_status)
{
	struct dpufb_video_idle_ctrl *vic = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	DPU_FB_DEBUG("video_idle_status = %d, gpu_compose_idle_frame = %d!\n",
		video_idle_status, dpufd->video_idle_ctrl.gpu_compose_idle_frame);

	if (!is_video_idle_ctrl_mode(dpufd)) {
		DPU_FB_DEBUG("do not support video idle!\n");
		return 0;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return 0;

	if (is_lb_available() == 0)
		return 0;

	vic = &(dpufd->video_idle_ctrl);

	dpu_check_and_return((!g_enable_video_idle_l3cache && !vic->buffer_alloced), 0,
		INFO, "disable cache idle display");

	if (dpufd->video_idle_ctrl.gpu_compose_idle_frame)
		return dpufb_gpu_compose_video_idle_ctrl(dpufd, video_idle_status);

	return 0;
}

#pragma GCC diagnostic pop

