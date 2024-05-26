/* Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
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
#include "dpufe_ion.h"
#include "dpufe.h"
#include "dpufe_dbg.h"
#include "dpufe_enum.h"
#include "dpufe_panel_def.h"
#include "dpufe_ov_play.h"
#include "dpufe_fb_buf_sync.h"
#include "dpufe_ov_utils.h"

#define BITS_PER_BYTE 8
#define DMA_STRIDE_ALIGN  (128 / BITS_PER_BYTE)

#ifndef array_size
#define array_size(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef align_up
#define align_up(val, al)    (((val) + ((al)-1)) & ~((typeof(val))(al)-1))
#endif

typedef struct dpu_fb_fix_var_screeninfo {
	uint32_t fix_type;
	uint32_t fix_xpanstep;
	uint32_t fix_ypanstep;
	uint32_t var_vmode;

	uint32_t var_blue_offset;
	uint32_t var_green_offset;
	uint32_t var_red_offset;
	uint32_t var_transp_offset;

	uint32_t var_blue_length;
	uint32_t var_green_length;
	uint32_t var_red_length;
	uint32_t var_transp_length;

	uint32_t var_blue_msb_right;
	uint32_t var_green_msb_right;
	uint32_t var_red_msb_right;
	uint32_t var_transp_msb_right;
	uint32_t bpp;
} dpu_fb_fix_var_screeninfo_t;

static dpu_fb_fix_var_screeninfo_t g_dpu_fb_fix_var_screeninfo[DPU_FB_PIXEL_FORMAT_MAX] = {
	{0}, {0}, {0}, {0}, {0}, {0}, {0},
	/* for DPU_FB_PIXEL_FORMAT_BGR_565 */
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 0, 4, 4, 4, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 12, 4, 4, 4, 4, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 0, 5, 5, 5, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 15, 5, 5, 5, 1, 0, 0, 0, 0, 2 },
	{0},
	/* DPU_FB_PIXEL_FORMAT_BGRA_8888 */
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 8, 16, 24, 8, 8, 8, 8, 0, 0, 0, 0, 4 },
	{ FB_TYPE_INTERLEAVED_PLANES, 2, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2 },
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};

void dpufe_free_reserve_buffer(struct dpufe_data_type *dfd)
{
	if (dfd == NULL) {
		DPUFE_ERR("NULL Pointer!\n");
		return;
	}

	if (dfd->index == PRIMARY_PANEL_IDX) {
		if (!dfd->fb_mem_free_flag) {
			dpufe_free_fb_buffer(dfd);
			dfd->fb_mem_free_flag = true;
		}
	}
}

uint32_t dpufe_line_length(int index, uint32_t xres, uint32_t bpp)
{
	return align_up(xres * bpp, DMA_STRIDE_ALIGN);
}

void dpufe_fb_init_screeninfo_base(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var)
{
	dpufe_check_and_no_retval((fix == NULL || var == NULL), "fix or var is NULL\n");

	fix->type_aux = 0;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;

	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	var->nonstd = 0;
	var->activate = FB_ACTIVATE_VBL;
	var->accel_flags = 0;
	var->sync = 0;
	var->rotate = 0;
}

void dpufe_fb_init_sreeninfo_by_img_type(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var,
	uint32_t fb_img_type, uint32_t *bpp)
{
	dpufe_check_and_no_retval((fix == NULL || var == NULL), "fix or var is NULL\n");

	fix->type = g_dpu_fb_fix_var_screeninfo[fb_img_type].fix_type;
	fix->xpanstep = g_dpu_fb_fix_var_screeninfo[fb_img_type].fix_xpanstep;
	fix->ypanstep = g_dpu_fb_fix_var_screeninfo[fb_img_type].fix_ypanstep;
	var->vmode = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_vmode;

	var->blue.offset = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_blue_offset;
	var->green.offset = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_green_offset;
	var->red.offset = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_red_offset;
	var->transp.offset = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_transp_offset;

	var->blue.length = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_blue_length;
	var->green.length = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_green_length;
	var->red.length = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_red_length;
	var->transp.length = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_transp_length;

	var->blue.msb_right = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_blue_msb_right;
	var->green.msb_right = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_green_msb_right;
	var->red.msb_right = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_red_msb_right;
	var->transp.msb_right = g_dpu_fb_fix_var_screeninfo[fb_img_type].var_transp_msb_right;
	*bpp = g_dpu_fb_fix_var_screeninfo[fb_img_type].bpp;
}

void dpufe_fb_init_sreeninfo_by_panel_info(struct fb_var_screeninfo *var, struct panel_base_info panel_info,
	uint32_t fb_num, uint32_t bpp)
{
	dpufe_check_and_no_retval((var == NULL), "var or panel_info is NULL\n");

	var->xres = panel_info.xres;
	var->yres = panel_info.yres;

	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * fb_num;
	var->bits_per_pixel = bpp * 8;

	var->height = panel_info.height;
	var->width = panel_info.width;

	DPUFE_INFO("xres %u, yres %u, xres_virtual %u, yres_virtual %u, "
		"bpp %u, var->pixclock %u, var->height %u, var->width %u\n",
		var->xres,
		var->yres,
		var->xres_virtual,
		var->yres_virtual,
		bpp,
		var->pixclock,
		var->height,
		var->width);
}

static int dpufe_get_hal_format_16_bpp(const int *hal_format_table, int table_len, uint32_t offset)
{
	int i;

	for (i = 0; i < table_len - 1; i += 2) {
		if (offset == hal_format_table[i])
			return hal_format_table[i + 1];
	}

	return DPU_FB_PIXEL_FORMAT_MAX;
}

static int dpufe_get_hal_format(struct fb_info *info)
{
	struct fb_var_screeninfo *var = NULL;
	int hal_format = 0;
	/* hal_fromat_blue[0, 2, 4] is offset of Blue bitfield
	 * hal_fromat_blue[1, 3, 5] is hal_format
	 */
	int hal_fromat_blue[] = {
		8, DPU_FB_PIXEL_FORMAT_RGBX_4444,
		10, DPU_FB_PIXEL_FORMAT_RGBX_5551,
		11, DPU_FB_PIXEL_FORMAT_BGR_565
	};
	/* hal_fromat_red[0, 2, 4] is offset of Red bitfield
	 * hal_fromat_red[1, 3, 5] is hal_format
	 */
	int hal_fromat_red[] = {
		8, DPU_FB_PIXEL_FORMAT_BGRX_4444,
		10, DPU_FB_PIXEL_FORMAT_BGRX_5551,
		11, DPU_FB_PIXEL_FORMAT_RGB_565
	};

	dpufe_check_and_return((info == NULL), -EINVAL, "info is NULL\n");

	var = &info->var;

	switch (var->bits_per_pixel) {
	case 16:  /* 16bits per pixel */
		if (var->transp.offset == 12) {  /* offset of transp */
			hal_fromat_blue[1] =  DPU_FB_PIXEL_FORMAT_RGBA_4444;
			hal_fromat_blue[3] = DPU_FB_PIXEL_FORMAT_RGBA_5551;
			hal_fromat_red[1] = DPU_FB_PIXEL_FORMAT_BGRA_4444;
			hal_fromat_red[3] = DPU_FB_PIXEL_FORMAT_BGRA_5551;
		}

		if (var->blue.offset == 0)
			hal_format = dpufe_get_hal_format_16_bpp(hal_fromat_red, array_size(hal_fromat_red),
				var->red.offset);
		else
			hal_format = dpufe_get_hal_format_16_bpp(hal_fromat_blue, array_size(hal_fromat_blue),
				var->blue.offset);

		if (hal_format == DPU_FB_PIXEL_FORMAT_MAX)
			goto err_return;
		break;
	case 32:  /* 32bits per pixel */
		if (var->blue.offset == 0)
			hal_format = (var->transp.length == 8) ?  /* length of transp */
				DPU_FB_PIXEL_FORMAT_BGRA_8888 : DPU_FB_PIXEL_FORMAT_BGRX_8888;
		else
			hal_format = (var->transp.length == 8) ?  /* length of transp */
				DPU_FB_PIXEL_FORMAT_RGBA_8888 : DPU_FB_PIXEL_FORMAT_RGBX_8888;
		break;

	default:
		goto err_return;
	}

	return hal_format;

err_return:
	DPUFE_ERR("not support this bits_per_pixel:%d!\n", var->bits_per_pixel);
	return -1;
}

static void dpufe_ov_pan_display_pack_data(
	struct dpufe_data_type *dfd,
	dss_overlay_t *pov_req,
	struct fb_info *fbi,
	int hal_format,
	uint32_t offset)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;

	pov_req->ov_block_infos_ptr = (uint64_t)(uintptr_t)(dfd->ov_block_infos);
	pov_req->ov_block_nums = 1;
	pov_req->ovl_idx = DPU_OVL0;
	pov_req->release_fence = -1;

	pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	pov_h_block = &(pov_h_block_infos[0]);
	pov_h_block->layer_nums = 1;

	layer = &(pov_h_block->layer_infos[0]);
	layer->img.format = hal_format;
	layer->img.width = fbi->var.xres;
	layer->img.height = fbi->var.yres;
	layer->img.bpp = fbi->var.bits_per_pixel >> 3;
	layer->img.stride = fbi->fix.line_length;
	layer->img.buf_size = layer->img.stride * layer->img.height;
	layer->img.phy_addr = fbi->fix.smem_start + offset;
	layer->img.vir_addr = fbi->fix.smem_start + offset;
	layer->img.mmu_enable = 1;
	layer->img.shared_fd = -1;
	layer->src_rect.x = 0;
	layer->src_rect.y = 0;
	layer->src_rect.w = fbi->var.xres;
	layer->src_rect.h = fbi->var.yres;
	layer->dst_rect.x = 0;
	layer->dst_rect.y = 0;
	layer->dst_rect.w = fbi->var.xres;
	layer->dst_rect.h = fbi->var.yres;
	layer->transform = DPU_FB_TRANSFORM_NOP;
	layer->blending = DPU_FB_BLENDING_NONE;
	layer->glb_alpha = 0xFF;
	layer->color = 0x0;
	layer->layer_idx = 0x0;
	layer->chn_idx = DPU_RCHN_V0;
	layer->need_cap = 0;
	layer->acquire_fence = -1;
}

static int dpufe_ov_pan_display_prepare(
	struct dpufe_data_type *dfd,
	struct fb_info *fbi)
{
	dss_overlay_t *pov_req = NULL;
	uint32_t offset;
	int hal_format;

	offset = fbi->var.xoffset * (fbi->var.bits_per_pixel >> 3) + fbi->var.yoffset * fbi->fix.line_length;
	if (!fbi->fix.smem_start) {
		DPUFE_ERR("fb%d, smem_start is null!\n", dfd->index);
		return -EINVAL;
	}

	if (fbi->fix.smem_len <= 0) {
		DPUFE_ERR("fb%d, smem_len = %d is out of range!\n", dfd->index, fbi->fix.smem_len);
		return -EINVAL;
	}

	hal_format = dpufe_get_hal_format(fbi);
	if (hal_format < 0) {
		DPUFE_ERR("fb%d, not support this fb_info's format!\n", dfd->index);
		return -EINVAL;
	}

	pov_req = &(dfd->ov_req);
	memset(pov_req, 0, sizeof(dss_overlay_t));

	dpufe_ov_pan_display_pack_data(dfd, pov_req, fbi, hal_format, offset);

	return 0;
}

static int dpufe_ov_pan_display_config_handle(struct dpufe_data_type *dfd)
{
	int ret;

	ret = dpufe_play_pack_data(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("online play block config failed!\n");
		return ret;
	}

	ret = dpufe_play_send_info(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("online play commit failed!\n");
		return ret;
	}

	return 0;
}

int dpufe_overlay_pan_display(struct dpufe_data_type *dfd)
{
	int ret;
	struct fb_info *fbi = NULL;

	dpufe_check_and_return(dfd == NULL, -EINVAL, "dfd is nullptr!\n");
	fbi = dfd->fbi;
	dpufe_check_and_return(fbi == NULL, -EINVAL, "fbi is nullptr!\n");
	dpufe_check_and_return(!dfd->panel_power_on, 0, "fb%d, panel is power off!\n", dfd->index);

	ret = dpufe_ov_pan_display_prepare(dfd, fbi);
	if (ret != 0) {
		DPUFE_INFO("fb%d, pan display prepare failed!\n", dfd->index);
		return -1;
	}

	ret = dpufe_ov_pan_display_config_handle(dfd);
	if (ret != 0) {
		DPUFE_INFO("fb%d, pan display config handle failed!\n", dfd->index);
		return -1;
	}

	dfd->frame_count++;
	return 0;
}

int dpufe_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct dpufe_data_type *dfd = NULL;

	if (info == NULL) {
		DPUFE_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (var == NULL) {
		DPUFE_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dfd = (struct dpufe_data_type *)info->par;
	if (dfd == NULL) {
		DPUFE_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPUFE_INFO("fb%d, +\n", dfd->index);

	if (var->rotate != FB_ROTATE_UR) {
		DPUFE_ERR("error rotate %d!\n", var->rotate);
		return -EINVAL;
	}

	if (var->grayscale != info->var.grayscale) {
		DPUFE_DEBUG("error grayscale %d!\n", var->grayscale);
		return -EINVAL;
	}

	if ((var->xres_virtual <= 0) || (var->yres_virtual <= 0)) {
		DPUFE_ERR("xres_virtual=%d yres_virtual=%d out of range!\n",
			var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}

	if ((var->xres == 0) || (var->yres == 0)) {
		DPUFE_ERR("xres=%d, yres=%d is invalid!\n", var->xres, var->yres);
		return -EINVAL;
	}

	if (var->xoffset > (var->xres_virtual - var->xres)) {
		DPUFE_ERR("xoffset=%d(xres_virtual=%d, xres=%d) out of range!\n",
			var->xoffset, var->xres_virtual, var->xres);
		return -EINVAL;
	}

	if (var->yoffset > (var->yres_virtual - var->yres)) {
		DPUFE_ERR("yoffset=%d(yres_virtual=%d, yres=%d) out of range!\n",
			var->yoffset, var->yres_virtual, var->yres);
		return -EINVAL;
	}

	DPUFE_INFO("fb%d, -\n", dfd->index);
	return 0;
}

int dpufe_fb_set_par(struct fb_info *info)
{
	struct dpufe_data_type *dfd = NULL;
	struct fb_var_screeninfo *var = NULL;

	if (info == NULL) {
		DPUFE_ERR("set par info NULL Pointer\n");
		return -EINVAL;
	}

	dfd = (struct dpufe_data_type *)info->par;
	if (dfd == NULL) {
		DPUFE_ERR("set par dfd NULL Pointer\n");
		return -EINVAL;
	}

	DPUFE_INFO("fb%d, +\n", dfd->index);

	var = &info->var;

	dfd->fbi->fix.line_length = dpufe_line_length(dfd->index, var->xres_virtual,
		var->bits_per_pixel >> 3);

	DPUFE_INFO("fb%d, -\n", dfd->index);

	return 0;
}

int dpufe_fb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	int ret = 0;
	struct dpufe_data_type *dfd = NULL;

	if (var == NULL || info == NULL) {
		DPUFE_ERR("pan display var or info NULL Pointer!\n");
		return -EINVAL;
	}

	dfd = (struct dpufe_data_type *)info->par;
	if (dfd == NULL) {
		DPUFE_ERR("pan display dfd NULL Pointer!\n");
		return -EINVAL;
	}

	down(&dfd->blank_sem);

	if (!dfd->panel_power_on) {
		DPUFE_INFO("fb%d, panel power off!\n", dfd->index);
		goto err_out;
	}

	DPUFE_INFO("fb%d, +\n", dfd->index);

	if (var->xoffset > (info->var.xres_virtual - info->var.xres))
		goto err_out;

	if (var->yoffset > (info->var.yres_virtual - info->var.yres))
		goto err_out;

	if (info->fix.xpanstep)
		info->var.xoffset =
		(var->xoffset / info->fix.xpanstep) * info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset =
		(var->yoffset / info->fix.ypanstep) * info->fix.ypanstep;

	dpufe_overlay_pan_display(dfd);

	up(&dfd->blank_sem);
	DPUFE_INFO("fb%d, -\n", dfd->index);

	return ret;

err_out:
	up(&dfd->blank_sem);
	return 0;
}
