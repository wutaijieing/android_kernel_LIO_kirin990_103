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
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include "dpu_block_algorithm.h"

static int g_debug_ovl_block_composer = 1;

struct ov_block_rect_param {
	struct disp_rect wb_block_rect;
	struct block_rect_info blk_rect;
};

int rect_across_rect(struct disp_rect rect1, struct disp_rect rect2, struct disp_rect *cross_rect)
{
	uint32_t center_x;
	uint32_t center_y;

	disp_check_and_return(!cross_rect, -EINVAL, err, "cross_rect is NULL\n");

	memset(cross_rect, 0x0, sizeof(struct disp_rect));

	if (rect1.w == 0 || rect1.h == 0 || rect2.w == 0 || rect2.h == 0)
		return 0;

	center_x = abs(rect2.x + rect2.w - 1 + rect2.x - (rect1.x + rect1.w - 1 + rect1.x));
	center_y = abs(rect2.y + rect2.h - 1 + rect2.y - (rect1.y + rect1.h - 1 + rect1.y));

	if ((center_x < rect2.w + rect1.w) && (center_y < rect2.h + rect1.h)) {  /*lint !e574*/
		/* rect cross case */
		cross_rect->x = MAX(rect1.x, rect2.x);
		cross_rect->y = MAX(rect1.y, rect2.y);
		cross_rect->w = MIN(rect1.x + rect1.w - 1, rect2.x + rect2.w - 1) - cross_rect->x + 1;
		cross_rect->h = MIN(rect1.y + rect1.h - 1, rect2.y + rect2.h - 1) - cross_rect->y + 1;

		return 1;
	}

	return 0;
}

static bool is_calc_dest_block(struct pipeline_src_layer *layer)
{
	/* sharpenss line buffer is 1600 for austin and dallas, but 2560 for chicago
	 * when hwc set CAP_2D_SHARPNESS or copybit use arsr pre do scaler, then calc block
	 */
	/* TODO */
	return false;
}

static bool is_calc_dest_rect_restrict(struct pipeline_src_layer *layer, int32_t scf_line_buffer)
{
	if (layer->src_rect.h != layer->dst_rect.h) {
		if (((layer->src_rect.w >= layer->dst_rect.w) && (layer->dst_rect.w > scf_line_buffer)) ||
			((layer->src_rect.w < layer->dst_rect.w) && (layer->src_rect.w > scf_line_buffer)))
			return true;
	}

	return false;
}

static uint32_t calc_dest_block_width(struct disp_overlay_block *pov_h_block, bool *src_invalid)
{
	struct pipeline_src_layer *layer = NULL;
	uint32_t block_width = BLOCK_WIDTH_MAX;
	int32_t scf_line_buffer = SCF_LINE_BUF;
	uint32_t i;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);

		if (layer->need_caps & (CAP_DIM | CAP_BASE)) {
			disp_pr_info("cap = %d continue", layer->need_caps);
			continue;
		}

		if (is_calc_dest_block(layer))
			block_width = MIN(block_width, SHARPNESS_LINE_BUF);


		if (layer->need_caps & CAP_HEBCD)
			block_width = MIN(block_width, HEBCE_LINEBUF);

		/* scaler line buffer, default value is 2560, but line buffer of rchn_v2 is 512
		 * scaler line buffer should be subtracted by 32 according to scale algorithm
		 */
		scf_line_buffer = SCF_LINE_BUF; // no rchn_v2

		scf_line_buffer = scf_line_buffer - 32;

		if (is_calc_dest_rect_restrict(layer, scf_line_buffer))
			block_width = MIN(block_width, scf_line_buffer);  /*lint !e574*/

		if (layer->src_rect.w > BLOCK_WIDTH_MAX)
			*src_invalid = true;
	}

	return block_width;
}

static void wb_calc_dest_block_width(struct ov_req_infos *pov_req, bool *wb_invalid, uint32_t *block_width)
{
	struct disp_wb_layer *wb_layer = NULL;
	uint32_t i;

	for (i = 0; i < pov_req->wb_layer_nums; i++) {
		wb_layer = &(pov_req->wb_layers[i]);

		if (wb_layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			if (wb_layer->need_caps & CAP_HEBCE)
				*block_width = MIN(*block_width, HEBCE_ROT_LINEBUF);
			else
				/* maximum of rot linebuffer is 480 */
				*block_width = MIN(*block_width, WDMA_ROT_LINEBUF);
		}

		/* maximum of hebce linebuffer is 512 */
		if (wb_layer->need_caps & CAP_HEBCE)
			*block_width = MIN(*block_width, HEBCE_LINEBUF);

		/* maximum of wch scl linebuffer is 512 */
		if (wb_layer->need_caps & CAP_SCL)
			*block_width = MIN(*block_width, WCH_SCF_LINE_BUF);

		if (wb_layer->dst_rect.w <= WCH_SCF_LINE_BUF)
			*wb_invalid = true;
	}
}

uint32_t calc_dest_block_size(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block)
{
	bool src_invalid = false;
	bool wb_invalid = false;
	uint32_t block_width;

	disp_check_and_return((!pov_req), BLOCK_SIZE_INVALID, err, "pov_req is NULL\n");

	disp_check_and_return((!pov_h_block), BLOCK_SIZE_INVALID, err, "pov_h_block is NULL\n");

	block_width = calc_dest_block_width(pov_h_block, &src_invalid);

	wb_calc_dest_block_width(pov_req, &wb_invalid, &block_width);

	/* if src layer width is greater than 8192 and wb layer width is less than or equal to 512
	 * we need to divide layer into vertical blocks with 256 width
	 */
	if (src_invalid && wb_invalid)
		block_width = MIN(block_width, BLOCK_WIDTH_MIN);

	disp_pr_info("block_width = %d", block_width);

	return block_width;
}

/* according wb src and dst calc ratio in h direction */
static uint32_t calc_h_ratio(struct ov_req_infos *pov_req)
{
	uint32_t h_ratio = SCF_INC_FACTOR;

	if (g_debug_block_scl_rot) {
		uint32_t acc_hscl = 0;
		int scf_src_w;
		int scf_dst_w;

		if (pov_req->wb_type == WB_COMPOSE_COPYBIT) {
			scf_src_w = pov_req->wb_layers[0].src_rect.w;
			scf_dst_w = pov_req->wb_layers[0].dst_rect.w;
			if (pov_req->wb_layers[0].transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
				scf_dst_w = pov_req->wb_layers[0].dst_rect.h;
				disp_pr_info("ADJUST[rot+scl] scf_dst_w = %d, scf_src_w = %d", scf_dst_w, scf_src_w);
			}
			disp_pr_info("ADJUST[scl] scf_dst_w = %d, scf_src_w = %d", scf_dst_w, scf_src_w);
			if (scf_src_w != scf_dst_w)
				h_ratio = (DPU_WIDTH(scf_src_w) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - acc_hscl) /
					DPU_WIDTH(scf_dst_w);
		}
	} else {
		(void)pov_req;
	}

	return h_ratio;
}

static void get_dst_rect_for_wb_scl(struct ov_req_infos *pov_req, struct ov_req_infos *pov_req_h_v,
	struct disp_rect ov_block_rect, int block_num)
{
	if (pov_req->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
		if (block_num > 1) {
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.x =
				pov_req->wb_ov_rect.x + ov_block_rect.x;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.y =
				pov_req->wb_ov_rect.y;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.w = ov_block_rect.w;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.h = ov_block_rect.h;
			disp_pr_info("ADJUST[scl] pov_req_h_v->wb_layers[0].wb_block.dst_rect w = %d", pov_req_h_v->wb_layers[0].wb_block.dst_rect.w,
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.h);
		} else {
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.x =
				pov_req_h_v->wb_layers[0].dst_rect.x;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.y =
				pov_req_h_v->wb_layers[0].dst_rect.y;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.w =
				pov_req_h_v->wb_layers[0].dst_rect.w;
			pov_req_h_v->wb_layers[0].wb_block.dst_rect.h =
				pov_req_h_v->wb_layers[0].dst_rect.h;
			disp_pr_info("ADJUST[scl] pov_req_h_v->wb_layers[0].wb_block.dst_rect w = %d", pov_req_h_v->wb_layers[0].wb_block.dst_rect.w,
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.h);
		}

        if (g_debug_ovl_block_composer)
			disp_pr_info("wb_layer:dst_rect[%d:%d:%d:%d]!\n",
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.x,
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.y,
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.w,
				pov_req_h_v->wb_layers[0].wb_block.dst_rect.h);
	}
}

int scf_output_suitable(uint32_t x_start, uint32_t x_end, uint32_t pos)
{
	if ((x_start > pos) || (x_end < pos))
		return 0;

	/* if distance between layer start/end and pos, return 1 for adjust */
	if ((pos - x_start < SCF_MIN_OUTPUT) || (x_end - pos + 1 < SCF_MIN_OUTPUT))
		return 1;

	return 0;
}

int block_fix_scf_constraint(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block,
	uint32_t block_size, uint32_t end_pos, uint32_t *fix_size)
{
	struct pipeline_src_layer *layer = NULL;
	uint32_t scf_layer_num = 0;
	uint32_t end = end_pos;
	uint32_t i;
	struct disp_rect scf_dst_rect[MAX_OFFLINE_LAYER_NUMBER];

	disp_check_and_return(!pov_req, -1, err, "pov_req is NULL.\n");

	disp_check_and_return(!pov_h_block, -1, err, "pov_h_block is NULL.\n");

	disp_check_and_return(!fix_size, -1, err, "fix_size is NULL.\n");

	*fix_size = block_size;

	disp_check_and_return((block_size <= SCF_MIN_OUTPUT), -1, err, "block size[%d] is too small!\n", block_size);

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);
		if (layer->need_caps & (CAP_BASE | CAP_DIM /*| CAP_PURE_COLOR*/))
			continue;

		if (0 /*layer->need_cap & CAP_2D_SHARPNESS */) {
			disp_check_and_return((scf_layer_num >= MAX_OFFLINE_LAYER_NUMBER), -1, err,
				"layer number in offline [%d] is more than scf moudle [%d]\n",
				scf_layer_num, MAX_OFFLINE_LAYER_NUMBER);
		} else {
			if ((layer->src_rect.w == layer->dst_rect.w) && (layer->src_rect.h == layer->dst_rect.h))
				continue;

			disp_check_and_return((scf_layer_num >= MAX_OFFLINE_SCF), -1, err,
				"scf layer in offline [%d] is more than scf moudle [%d]\n",
				scf_layer_num, MAX_OFFLINE_SCF);
		}

		/* get all scaler layers */
		scf_dst_rect[scf_layer_num].x = layer->dst_rect.x;
		scf_dst_rect[scf_layer_num].y = layer->dst_rect.y;
		scf_dst_rect[scf_layer_num].w = layer->dst_rect.w;
		scf_dst_rect[scf_layer_num].h = layer->dst_rect.h;
		scf_layer_num++;
	}

	if (scf_layer_num == 0)
		return 0;

REDO:
	for (i = 0; i < scf_layer_num; i++) {
		if (scf_output_suitable(scf_dst_rect[i].x,
			scf_dst_rect[i].x + scf_dst_rect[i].w - 1, pov_req->wb_ov_rect.x + end)) {
			end = end - SCF_MIN_OUTPUT;
			goto REDO;
		}
	}

	*fix_size = block_size - (end_pos - end);

	return 0;
}

static void wb_adjust_layers_cap(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block,
	struct disp_wb_layer *wb_layer, bool has_rot)
{
	uint32_t i = 0;
	int temp = 0;
	struct pipeline_src_layer *layer = NULL;
	uint32_t stretch_line_num = 0;
	struct disp_rect temp_rect;

	/* 2 wb layer */
	if (has_rot) {
		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layers[i]);
			temp = wb_layer->src_rect.w;
			wb_layer->src_rect.w = wb_layer->src_rect.h;
			wb_layer->src_rect.h = temp;
			disp_pr_info("ADJUST[rot] wblayer_info layer->src_rect.w = %d, h = %d", wb_layer->src_rect.w, wb_layer->src_rect.h);
			wb_layer->transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);
		}
	}

	if ((pov_h_block->layer_nums == 1) && (pov_req->wb_layer_nums == 1)) {
		/* adjust wb_layer src_rect from layer src_rect when wch scl */
		if (wb_layer->need_caps & CAP_SCL) {
			layer = &pov_h_block->layer_infos[0];
			stretch_line_num = 0; //get_sdma_stretch_line_num(layer);
			temp_rect.w = layer->src_rect.w;
			temp_rect.h = layer->src_rect.h;

			if (stretch_line_num > 0)
				temp_rect.h = (layer->src_rect.h / stretch_line_num) +
					((layer->src_rect.h % stretch_line_num) ? 1 : 0);

			wb_layer->src_rect.w = temp_rect.w;
			wb_layer->src_rect.h = temp_rect.h;

            disp_pr_info("[DATA] wb_layer->src_rect.w = %d, h = %d", wb_layer->src_rect.w, wb_layer->src_rect.h);
		}
	}
}

int adjust_layers_cap(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block, struct disp_wb_layer *wb_layer)
{
	uint32_t i;
	int temp = 0;
	struct pipeline_src_layer *layer = NULL;
	bool has_rot = false;

	disp_check_and_return((!pov_req), -EINVAL, err, "pov_req is NULL.\n");

	disp_check_and_return((!pov_h_block), -EINVAL, err, "pov_h_block is NULL.\n");

	disp_check_and_return((!wb_layer), -EINVAL, err, "wb_layer is NULL.\n");

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &pov_h_block->layer_infos[i];

		if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
			temp = layer->dst_rect.x;
			layer->dst_rect.x = pov_req->wb_ov_rect.x + (layer->dst_rect.y - pov_req->wb_ov_rect.y);
			layer->dst_rect.y = pov_req->wb_ov_rect.y + temp - pov_req->wb_ov_rect.x;

			temp = layer->dst_rect.w;
			layer->dst_rect.w = layer->dst_rect.h;
			layer->dst_rect.h = temp;

			disp_pr_info("ADJUST[rot] layer_info layer->dst_rect.w = %d, h = %d", layer->dst_rect.w, layer->dst_rect.h);
			if (layer->transform == HISI_FB_TRANSFORM_ROT_90)
				layer->transform = HISI_FB_TRANSFORM_FLIP_V;
			else if (layer->transform == HISI_FB_TRANSFORM_ROT_270)
				layer->transform = HISI_FB_TRANSFORM_FLIP_H;
			else if (layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_H))
				layer->transform = HISI_FB_TRANSFORM_ROT_180;
			else if (layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V))
				layer->transform = HISI_FB_TRANSFORM_NOP;
			else
				;  /* do nothing */

			has_rot = true;
		}
	}

	wb_adjust_layers_cap(pov_req, pov_h_block, wb_layer, has_rot);

	return 0;
}

static bool get_copybit_wch_scl(uint32_t need_caps, uint32_t wb_type)
{
	if ((need_caps & CAP_SCL) && (wb_type == WB_COMPOSE_COPYBIT) && g_debug_block_scl_rot)
		return true;
	else
		return false;
}

static void get_wb_block_rect(bool has_wb_scl, struct disp_wb_layer *wb_layer,
	struct block_rect_info *blk_rect, struct disp_rect *wb_block_rect)
{
	uint32_t transform = (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V);

	if (has_wb_scl || blk_rect->copybit_wch_scl) {
		if (wb_layer->transform == transform) {
			wb_block_rect->x = wb_layer->dst_rect.x;
			wb_block_rect->y = wb_layer->dst_rect.y;
			wb_block_rect->w = wb_layer->dst_rect.h;
			wb_block_rect->h = wb_layer->dst_rect.w;
			disp_pr_info("ADJUST[rot+scl] wb_block_rect wb_block_rect.w = %d, h = %d", wb_block_rect->w, wb_block_rect->h);
		} else {
			*wb_block_rect = wb_layer->dst_rect;
			disp_pr_info("ADJUST[scl] wb_block_rect wb_block_rect.w = %d, h = %d", wb_block_rect->w, wb_block_rect->h);
		}
	} else {
		*wb_block_rect = wb_layer->src_rect;
	}

	blk_rect->w = wb_block_rect->w;
	blk_rect->h = wb_block_rect->h;
}

static void get_ov_block_rect_size(struct disp_rect *ov_block_rects[], int block_num,
	struct disp_wb_layer *wb_layer, struct ov_block_rect_param blk_rect_param)
{
	if (blk_rect_param.blk_rect.copybit_wch_scl) {
		ov_block_rects[block_num]->x = wb_layer->dst_rect.x;
		ov_block_rects[block_num]->y = wb_layer->dst_rect.y;
		ov_block_rects[block_num]->w = wb_layer->dst_rect.w;
		ov_block_rects[block_num]->h = wb_layer->dst_rect.h;
		disp_pr_info("ADJUST[copybit_scl] wb_block_rect wb_block_rect.w = %d, h = %d", ov_block_rects[block_num]->w, ov_block_rects[block_num]->h);
	} else {
		ov_block_rects[block_num]->x = blk_rect_param.wb_block_rect.x;
		ov_block_rects[block_num]->y = blk_rect_param.wb_block_rect.y;
		ov_block_rects[block_num]->w = blk_rect_param.wb_block_rect.w;
		ov_block_rects[block_num]->h = blk_rect_param.wb_block_rect.h;
	}
}

static int recalculate_dst_blocks(struct ov_req_infos *pov_req,
	struct disp_overlay_block *pov_h_block, int *block_num, struct disp_rect *ov_block_rects[],
	struct ov_block_rect_param *blk_rect_param)
{
	struct pipeline_src_layer *layer = NULL;
	struct disp_rect *wb_block_rect = &(blk_rect_param->wb_block_rect);
	struct block_rect_info *blk_rect = &(blk_rect_param->blk_rect);
	uint32_t i;

	/* recalculate the block size, the final value */
	blk_rect->current_offset = blk_rect->current_offset - (blk_rect->block_size - blk_rect->fix_scf_span);
	blk_rect->block_has_layer = 0;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);

		if (((blk_rect->last_offset + pov_req->wb_ov_rect.x) <= (layer->dst_rect.x + layer->dst_rect.w - 1)) &&  /*lint !e574*/
			(layer->dst_rect.x < (blk_rect->current_offset + pov_req->wb_ov_rect.x))) {  /*lint !e574*/
			blk_rect->block_has_layer = 1;
			if ((*block_num) >= HISI_DPU_CMDLIST_BLOCK_MAX)
				return -5; /* error code */

			/* get the block rectangles */
			ov_block_rects[*block_num]->x = wb_block_rect->x + blk_rect->last_offset;
			ov_block_rects[*block_num]->y = wb_block_rect->y;
			ov_block_rects[*block_num]->w = MIN(blk_rect->current_offset - blk_rect->last_offset,
				blk_rect->w - blk_rect->last_offset);
			ov_block_rects[*block_num]->h = blk_rect->h;
			disp_pr_info("ADJUST[block_has_layer] ov_block_rects[*block_num].w = %d, h = %d, block_num = %d", ov_block_rects[*block_num]->w, ov_block_rects[*block_num]->h, *block_num);
			(*block_num)++;
			break;
		}
	}

	if (blk_rect->block_has_layer == 0) {
		if ((*block_num) >= HISI_DPU_CMDLIST_BLOCK_MAX)
			return -6; /* error code */

		ov_block_rects[*block_num]->x = wb_block_rect->x + blk_rect->last_offset;
		ov_block_rects[*block_num]->y = wb_block_rect->y;
		ov_block_rects[*block_num]->w = MIN(blk_rect->current_offset - blk_rect->last_offset,
			blk_rect->w - blk_rect->last_offset);
		ov_block_rects[*block_num]->h = blk_rect->h;
		disp_pr_info("ADJUST[block_no_layer] ov_block_rects[*block_num].w = %d, h = %d, block_num = %d", ov_block_rects[*block_num]->w, ov_block_rects[*block_num]->h, *block_num);
		(*block_num)++;
	}

	if (g_debug_ovl_block_composer)
		disp_pr_info("ov_block_rects[%d]:[%d:%d:%d:%d], wb_block_rect[%d:%d:%d:%d], current_offset=%d,"
			"fix_scf_span=%d, last_offset=%d, w=%d!\n",
			*block_num, ov_block_rects[*block_num - 1]->x, ov_block_rects[*block_num - 1]->y,
			ov_block_rects[*block_num - 1]->w, ov_block_rects[*block_num - 1]->h, wb_block_rect->x,
			wb_block_rect->y, wb_block_rect->w, wb_block_rect->h, blk_rect->current_offset,
			blk_rect->fix_scf_span, blk_rect->last_offset, blk_rect->w);

	return 0;
}

int get_ov_block_rect(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block,
	int *block_num, struct disp_rect *ov_block_rects[], bool has_wb_scl)
{
	int ret = 0;
	struct disp_wb_layer *wb_layer = NULL;
	struct ov_block_rect_param blk_rect_param;

	if (!pov_req || !pov_h_block || !ov_block_rects || !block_num) {
		disp_pr_err("input ptr is NULL\n");
		return -EINVAL;
	}

	wb_layer = &(pov_req->wb_layers[0]);

	memset(&blk_rect_param, 0, sizeof(blk_rect_param));

	*block_num = 0;

	blk_rect_param.blk_rect.copybit_wch_scl = get_copybit_wch_scl(wb_layer->need_caps, pov_req->wb_type);

	/* adjust layer transform cap, source layer dst_rect and writeback layer src_rect */
	adjust_layers_cap(pov_req, pov_h_block, wb_layer);

	get_wb_block_rect(has_wb_scl, wb_layer, &blk_rect_param.blk_rect, &blk_rect_param.wb_block_rect);

	/* init block size according to source layer dst_rect */
	blk_rect_param.blk_rect.block_size = calc_dest_block_size(pov_req, pov_h_block);

	/* if block size is invalid or larger than write back width, block is not needed. */
	/* Then block num is set to 1, and block rect is set to write back layer rect */
	if ((blk_rect_param.blk_rect.block_size == BLOCK_SIZE_INVALID) || /*lint !e574*/
		(blk_rect_param.blk_rect.block_size >= (uint32_t)blk_rect_param.blk_rect.w)) {
		get_ov_block_rect_size(ov_block_rects, *block_num, wb_layer, blk_rect_param);
		*block_num = 1;
		return ret;
	}

	blk_rect_param.blk_rect.fix_scf_span = blk_rect_param.blk_rect.block_size;

	for (blk_rect_param.blk_rect.current_offset = blk_rect_param.blk_rect.block_size;
		blk_rect_param.blk_rect.last_offset < blk_rect_param.blk_rect.w;  /*lint !e574*/
		blk_rect_param.blk_rect.last_offset = blk_rect_param.blk_rect.current_offset,
		blk_rect_param.blk_rect.current_offset += blk_rect_param.blk_rect.block_size) {
		/* make sure each block of scaler layer is larger than 16 */
		if (!has_wb_scl && !blk_rect_param.blk_rect.copybit_wch_scl) {
			if (block_fix_scf_constraint(pov_req, pov_h_block, blk_rect_param.blk_rect.block_size,
				blk_rect_param.blk_rect.current_offset, &blk_rect_param.blk_rect.fix_scf_span) != 0) {
				disp_pr_err("block_fix_scf_constraint err!\n");
				return -3; /* error code */
			}
		}

		/* dst devided by block_size */
		ret = recalculate_dst_blocks(pov_req, pov_h_block, block_num, ov_block_rects, &blk_rect_param);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int get_vscfh_ch(void)
{
	return 2; //DSS_RCHN_V0;
}

static void arsr_pre_scf_para_process(struct pipeline_src_layer *h_layer,
	struct pipeline_src_layer *h_v_layer, struct block_layer_info *blk)
{
	/* h_ratio=(arsr_input_width*65536+65536-ihleft) / (arsr_output_width+1) */
	blk->h_ratio = (DPU_WIDTH(blk->src_rect.w) * ARSR2P_INC_FACTOR + ARSR2P_INC_FACTOR - blk->acc_hscl) /
		h_layer->dst_rect.w;

	/* starti = ceil( starto *((double)(ihinc))/(65536.0) - overlapH */
	blk->scf_int = blk->output_startpos * blk->h_ratio / ARSR2P_INC_FACTOR;
	blk->scf_rem = blk->output_startpos * blk->h_ratio % ARSR2P_INC_FACTOR;
	blk->scf_in_start = (blk->scf_rem > 0) ? (blk->scf_int + 1) : blk->scf_int;

	/* endi = ceil((endo+1)*((double)(ihinc))/(65536.0) + overlapH -1 */
	blk->scf_int = (blk->output_startpos + blk->output_span) * blk->h_ratio / ARSR2P_INC_FACTOR;
	blk->scf_rem = (blk->output_startpos + blk->output_span) * blk->h_ratio % ARSR2P_INC_FACTOR;
	blk->scf_in_end = (blk->scf_rem > 0) ? (blk->scf_int + 1) : blk->scf_int;

	/* the layer is included only in this block */
	if ((blk->first_block == 1) && (blk->last_block == 1)) {
		blk->scf_read_start = 0;
		blk->scf_read_end = DPU_WIDTH(blk->src_rect.w);
		h_v_layer->block_info.last_tile = 1;
	} else if (blk->first_block == 1) {  /* first block of this layer */
		blk->scf_read_start = 0;
		blk->scf_read_end = blk->scf_in_end + ARSR2P_OVERLAPH - 1;
	} else {
		blk->scf_read_start = 0;
		if (blk->scf_in_start > ARSR2P_OVERLAPH)
			blk->scf_read_start = blk->scf_in_start - ARSR2P_OVERLAPH;

		if (blk->last_block == 1) {  /* last block of this layer */
			blk->scf_read_end = DPU_WIDTH(blk->src_rect.w);
			h_v_layer->block_info.last_tile = 1;
		} else {  /* middle block of this layer */
			blk->scf_read_end = blk->scf_in_end + ARSR2P_OVERLAPH - 1;
		}
	}
}

static void arsr_pre_layer_block_process(
	struct pipeline_src_layer *h_layer, struct pipeline_src_layer *h_v_layer,
	struct disp_rect *dst_cross_rect, struct block_layer_info *blk)
{
	disp_pr_info("no need so return\n");
	return;
	if (h_layer->src_rect.w < h_layer->dst_rect.w ||
		h_layer->src_rect.h != h_layer->dst_rect.h /*||
		(h_layer->need_cap & CAP_2D_SHARPNESS)*/) {

		/* not the first block and start position is not aligned to 2 */
		if ((!blk->first_block) && (blk->output_startpos % 2)) {
			/* start position subtracted by 1 */
			h_v_layer->block_info.arsr2p_left_clip = 1;
			dst_cross_rect->x = dst_cross_rect->x - h_v_layer->block_info.arsr2p_left_clip;
			dst_cross_rect->w = dst_cross_rect->w + h_v_layer->block_info.arsr2p_left_clip;

			blk->output_startpos = blk->output_startpos - h_v_layer->block_info.arsr2p_left_clip;
			blk->output_span = dst_cross_rect->w;
		}

		if (h_layer->src_rect.w > h_layer->dst_rect.w) {
			blk->src_rect.x = h_layer->src_rect.x;
			blk->src_rect.y = h_layer->src_rect.y;
			/* scaling down in horizental direction, set arsr2p input width to dst width */
			blk->src_rect.w = h_layer->dst_rect.w;
			blk->src_rect.h = h_layer->src_rect.h;
		} else {
			blk->src_rect = h_layer->src_rect;
		}

		arsr_pre_scf_para_process(h_layer, h_v_layer, blk);

		if (blk->scf_read_end > DPU_WIDTH(blk->src_rect.w))  /*lint !e574*/
			blk->scf_read_end = DPU_WIDTH(blk->src_rect.w);

		blk->input_startpos = blk->scf_read_start;
		blk->input_span = blk->scf_read_end - blk->scf_read_start + 1;
		h_v_layer->block_info.h_ratio_arsr2p = blk->h_ratio;
		h_v_layer->block_info.arsr2p_src_x = h_layer->src_rect.x;
		h_v_layer->block_info.arsr2p_src_y = h_layer->src_rect.y;
		h_v_layer->block_info.arsr2p_dst_x = h_layer->dst_rect.x;
		h_v_layer->block_info.arsr2p_dst_y = h_layer->dst_rect.y;
		h_v_layer->block_info.arsr2p_dst_w = h_layer->dst_rect.w;

		/* handle ROT */
		blk->rect_transform.x = h_layer->src_rect.x + blk->input_startpos;
		blk->rect_transform.y = h_layer->src_rect.y;
		blk->rect_transform.w = blk->input_span;
		blk->rect_transform.h = h_layer->src_rect.h;
		h_v_layer->block_info.arsr2p_in_rect = blk->rect_transform;
		h_v_layer->src_rect = blk->rect_transform; /* arsr2p input rect */
		(void)rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);
		h_v_layer->dst_rect = *dst_cross_rect; /* arsr2p output rect */
	}
}

static void scaling_scf_para_process(struct pipeline_src_layer *h_layer,
	struct block_layer_info *blk)
{
	blk->h_ratio = (DPU_WIDTH(h_layer->src_rect.w) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2 - blk->acc_hscl) /
		DPU_WIDTH(h_layer->dst_rect.w);

	blk->scf_in_start = blk->output_startpos * blk->h_ratio / SCF_INC_FACTOR;
	blk->scf_in_end = DPU_WIDTH(blk->output_startpos + blk->output_span) * blk->h_ratio / SCF_INC_FACTOR;

	/* the layer is included only in this block */
	if ((blk->first_block == 1) && (blk->last_block == 1)) {
		blk->acc_hscl = 0;
		blk->scf_read_start = 0;
		blk->scf_read_end = DPU_WIDTH(h_layer->src_rect.w);
	} else if (blk->first_block == 1) {  /* first block of this layer */
		blk->acc_hscl = 0;
		blk->scf_read_start = 0;
		blk->scf_read_end = blk->scf_in_end + SCF_INPUT_OV;
	} else {
		blk->scf_read_start = 0;
		if (blk->scf_in_start > SCF_INPUT_OV)
			blk->scf_read_start = blk->scf_in_start - SCF_INPUT_OV;

		blk->acc_hscl = blk->output_startpos * blk->h_ratio - blk->scf_read_start * SCF_INC_FACTOR;

		if (blk->last_block == 1)  /* last block of this layer */
			blk->scf_read_end = DPU_WIDTH(h_layer->src_rect.w);
		else  /* middle block of this layer */
			blk->scf_read_end = blk->scf_in_end + SCF_INPUT_OV;
	}
}

static void scaling_block_process(struct pipeline_src_layer *h_layer,
	struct pipeline_src_layer *h_v_layer, struct block_layer_info *blk)
{
	//disp_pr_info("no need so return\n");
	//return;
	if (0 /* h_layer->chn_idx == blk->chn_idx_temp */) {
		if (h_layer->src_rect.w <= h_layer->dst_rect.w)
			return;
	} else {
		if (h_layer->src_rect.w == h_layer->dst_rect.w)
			return;
	}

    disp_pr_info("enter+++");
	/* check if arsr2p input has already extened width */
	if (h_v_layer->block_info.h_ratio_arsr2p) {
		/* h_v_layer->block_info.arsr2p_in_rect = rect_transform; */
		blk->dst_rect = blk->rect_transform;
		h_v_layer->block_info.both_vscfh_arsr2p_used = 1;

		blk->output_startpos = blk->input_startpos;
		/* output_startpos = dst_rect.x - h_layer->src_rect.x */
		blk->output_span = blk->dst_rect.w;
	}

	scaling_scf_para_process(h_layer, blk);

	if (blk->scf_read_end > DPU_WIDTH(h_layer->src_rect.w))  /*lint !e574*/
		blk->scf_read_end = DPU_WIDTH(h_layer->src_rect.w);

	blk->input_startpos = blk->scf_read_start;
	blk->input_span = blk->scf_read_end - blk->scf_read_start + 1;
	h_v_layer->block_info.h_ratio = blk->h_ratio;
	h_v_layer->block_info.acc_hscl = blk->acc_hscl;

	if (g_debug_ovl_block_composer)
		disp_pr_info("first_block=%d, last_block=%d, output_startpos=%d, output_span=%d, "
			"h_ratio=%d, acc_hscl=%d, scf_read_start=%d, scf_read_end=%d, input_startpos=%d, input_span=%d\n",
			blk->first_block, blk->last_block, blk->output_startpos, blk->output_span,
			blk->h_ratio, blk->acc_hscl, blk->scf_read_start, blk->scf_read_end,
			blk->input_startpos, blk->input_span);
}

static int create_h_v_block_layer(struct pipeline_src_layer *h_layer, struct pipeline_src_layer *h_v_layer,
	struct disp_rect dst_cross_rect, struct disp_rect ov_block_rect)
{
	struct block_layer_info blk;

	disp_check_and_return(!h_layer || !h_v_layer, -EINVAL, err, "h_layer or h_v_layer is NULL\n");

	memset(&blk, 0, sizeof(struct block_layer_info));
	blk.chn_idx_temp = get_vscfh_ch();
	blk.first_block = (h_layer->dst_rect.x >= ov_block_rect.x) ? 1 : 0;
	blk.last_block = ((ov_block_rect.x + ov_block_rect.w) >= (h_layer->dst_rect.x + h_layer->dst_rect.w)) ? 1 : 0;
	blk.output_startpos = dst_cross_rect.x - h_layer->dst_rect.x;
	blk.output_span = dst_cross_rect.w;
	blk.input_startpos = blk.output_startpos;
	blk.input_span = blk.output_span;

	/* handle arsr2p layer */
	arsr_pre_layer_block_process(h_layer, h_v_layer, &dst_cross_rect, &blk);

	/* scaling not in rchn v0 or scaling down in rchn v0 */
	scaling_block_process(h_layer, h_v_layer, &blk);

	/* handle ROT */
	switch (h_v_layer->transform) {
	case HISI_FB_TRANSFORM_NOP:
	case HISI_FB_TRANSFORM_FLIP_V:
		blk.rect_transform.x = h_layer->src_rect.x + blk.input_startpos;
		blk.rect_transform.y = h_layer->src_rect.y;
		blk.rect_transform.w = blk.input_span;
		blk.rect_transform.h = h_layer->src_rect.h;
		break;
	case HISI_FB_TRANSFORM_ROT_180:
	case HISI_FB_TRANSFORM_FLIP_H:
		blk.rect_transform.x = h_layer->src_rect.x + h_layer->src_rect.w - blk.input_startpos - blk.input_span;
		blk.rect_transform.y = h_layer->src_rect.y;
		blk.rect_transform.w = blk.input_span;
		blk.rect_transform.h = h_layer->src_rect.h;
		break;
	default:
		disp_pr_err("unknow h_v_layer->transform=%d!\n", h_v_layer->transform);
		return -EINVAL;
	}

	h_v_layer->src_rect = blk.rect_transform;
	(void)rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);
	h_v_layer->dst_rect = dst_cross_rect;

	return 0;
}

static void wb_block_print_info(struct wb_block_layer_info *wb_blk)
{
    if (g_debug_ovl_block_composer)
		disp_pr_info("first_block=%d, last_block=%d, output_startpos=%d, output_span=%d, "
			"h_ratio=%d, acc_hscl=%d, scf_read_start=%d, scf_read_end=%d, input_startpos=%d, input_span=%d\n",
			wb_blk->first_block, wb_blk->last_block, wb_blk->output_startpos, wb_blk->output_span,
			wb_blk->h_ratio, wb_blk->acc_hscl, wb_blk->scf_read_start, wb_blk->scf_read_end,
			wb_blk->input_startpos, wb_blk->input_span);
}

static void set_wb_blk_scf(struct ov_req_infos *pov_req_h_v, struct pipeline_src_layer *h_layer,
	struct wb_block_layer_info *wb_blk)
{
	if ((wb_blk->first_block == 1) && (wb_blk->last_block == 1)) {
		wb_blk->acc_hscl = 0;
		wb_blk->scf_read_start = 0;
		wb_blk->scf_read_end = DPU_WIDTH(h_layer->src_rect.w);
	} else if (wb_blk->first_block == 1) { /* first block of this layer */
		wb_blk->acc_hscl = 0;
		wb_blk->scf_read_start = 0;
		wb_blk->scf_read_end = wb_blk->scf_in_end + SCF_INPUT_OV;
	} else {
		wb_blk->scf_read_start = 0;
		if (wb_blk->scf_in_start > SCF_INPUT_OV)
			wb_blk->scf_read_start = wb_blk->scf_in_start - SCF_INPUT_OV;

		wb_blk->acc_hscl = wb_blk->output_startpos * wb_blk->h_ratio -
			wb_blk->scf_read_start * SCF_INC_FACTOR;

		/* output_startpos is the size before scaler, so no need *h_ratio */
		if (pov_req_h_v->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
			wb_blk->acc_hscl = wb_blk->output_startpos * SCF_INC_FACTOR -
				wb_blk->scf_read_start * SCF_INC_FACTOR;
			disp_pr_info("ADJUST[scl] wb_blk->acc_hscl = %d", wb_blk->acc_hscl);
		}

		wb_blk->scf_read_end = (wb_blk->last_block == 1) ?
			DPU_WIDTH(h_layer->src_rect.w) : (wb_blk->scf_in_end + SCF_INPUT_OV);
	}
}

static void wb_block_scf_process(struct ov_req_infos *pov_req_h_v,
	struct pipeline_src_layer *h_layer, struct disp_rect ov_block_rect, struct wb_block_layer_info *wb_blk)
{
	/* scf dst is not src layer dst, but wb layer dst */
	if (pov_req_h_v->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
		wb_blk->last_block = (((ov_block_rect.x + ov_block_rect.w) + 5) >=
			(h_layer->dst_rect.x + h_layer->dst_rect.w)) ? 1 : 0;
		wb_blk->scf_dst_w = pov_req_h_v->wb_layers[0].dst_rect.w;
		disp_pr_info("ADJUST[scl] scf_dst_w = %d", wb_blk->scf_dst_w);
		if (pov_req_h_v->wb_layers[0].transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
			wb_blk->scf_dst_w = pov_req_h_v->wb_layers[0].dst_rect.h;
			disp_pr_info("ADJUST[scl+rot] scf_dst_w = %d", wb_blk->scf_dst_w);
		}
	}

	if (h_layer->src_rect.w != wb_blk->scf_dst_w) {
		wb_blk->h_ratio = (DPU_WIDTH(h_layer->src_rect.w) * SCF_INC_FACTOR +
			SCF_INC_FACTOR / 2 - wb_blk->acc_hscl) / DPU_WIDTH(wb_blk->scf_dst_w);

		wb_blk->scf_in_start = wb_blk->output_startpos * wb_blk->h_ratio / SCF_INC_FACTOR;
		wb_blk->scf_in_end = DPU_WIDTH(wb_blk->output_startpos + wb_blk->output_span) *
			wb_blk->h_ratio / SCF_INC_FACTOR;

		/* scf in start is already calc outside, no need calc anymore */
		if (pov_req_h_v->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
			wb_blk->scf_in_start = wb_blk->output_startpos;
			wb_blk->scf_in_end = wb_blk->output_startpos + wb_blk->output_span;
			disp_pr_info("ADJUST[scl] wb_blk->scf_in_start = %d, wb_blk->scf_in_end = %d", wb_blk->scf_in_start, wb_blk->scf_in_end);
		}

		/* the layer is included only in this block */
		set_wb_blk_scf(pov_req_h_v, h_layer, wb_blk);

		if (wb_blk->scf_read_end > DPU_WIDTH(h_layer->src_rect.w))  /*lint !e574*/
			wb_blk->scf_read_end = DPU_WIDTH(h_layer->src_rect.w);

		wb_blk->input_startpos = wb_blk->scf_read_start;
		wb_blk->input_span = wb_blk->scf_read_end - wb_blk->scf_read_start + 1;

		pov_req_h_v->wb_layers[0].wb_block.h_ratio = wb_blk->h_ratio;
		pov_req_h_v->wb_layers[0].wb_block.acc_hscl = wb_blk->acc_hscl;

		wb_block_print_info(wb_blk);
	}
}

static int get_rect_rot_transform(uint32_t transform,
	struct pipeline_src_layer *h_layer, struct wb_block_layer_info *wb_blk)
{
	switch (transform) {
	case HISI_FB_TRANSFORM_NOP:
	case HISI_FB_TRANSFORM_FLIP_V:
		wb_blk->rect_transform.x = h_layer->src_rect.x + wb_blk->input_startpos;
		wb_blk->rect_transform.y = h_layer->src_rect.y;
		wb_blk->rect_transform.w = wb_blk->input_span;
		wb_blk->rect_transform.h = h_layer->src_rect.h;
		disp_pr_info("ADJUST[v mirror] wb_blk->rect_transform w = %d, wb_blk->rect_transform h = %d", wb_blk->rect_transform.w, wb_blk->rect_transform.h);
		break;
	case HISI_FB_TRANSFORM_ROT_180:
	case HISI_FB_TRANSFORM_FLIP_H:
		wb_blk->rect_transform.x = h_layer->src_rect.x + h_layer->src_rect.w -
			wb_blk->input_startpos - wb_blk->input_span;
		wb_blk->rect_transform.y = h_layer->src_rect.y;
		wb_blk->rect_transform.w = wb_blk->input_span;
		wb_blk->rect_transform.h = h_layer->src_rect.h;
		disp_pr_info("ADJUST[h mirror] wb_blk->rect_transform w = %d, wb_blk->rect_transform h = %d", wb_blk->rect_transform.w, wb_blk->rect_transform.h);
		break;
	default:
		disp_pr_err("unknow h_v_layer->transform=%d!\n", transform);
		return -EINVAL;
	}

	return 0;
}

static void  h_v_block_layer_print_info(struct ov_req_infos *pov_req_h_v, struct pipeline_src_layer *h_v_layer)
{
	disp_pr_info("layer:src_rect[%d:%d:%d:%d], dst_rect[%d:%d:%d:%d]!\n"
		"wb_layer:src_rect[%d:%d:%d:%d], dst_rect[%d:%d:%d:%d]!\n",
		h_v_layer->src_rect.x, h_v_layer->src_rect.y, h_v_layer->src_rect.w, h_v_layer->src_rect.h,
		h_v_layer->dst_rect.x, h_v_layer->dst_rect.y, h_v_layer->dst_rect.w, h_v_layer->dst_rect.h,
		pov_req_h_v->wb_layers[0].wb_block.src_rect.x,
		pov_req_h_v->wb_layers[0].wb_block.src_rect.y,
		pov_req_h_v->wb_layers[0].wb_block.src_rect.w,
		pov_req_h_v->wb_layers[0].wb_block.src_rect.h,
		pov_req_h_v->wb_layers[0].wb_block.dst_rect.x,
		pov_req_h_v->wb_layers[0].wb_block.dst_rect.y,
		pov_req_h_v->wb_layers[0].wb_block.dst_rect.w,
		pov_req_h_v->wb_layers[0].wb_block.dst_rect.h);
}

static int wb_create_h_v_block_layer(struct ov_req_infos *pov_req_h_v, struct pipeline_src_layer *h_layer,
	struct pipeline_src_layer *h_v_layer, struct disp_rect dst_cross_rect, struct disp_rect ov_block_rect)
{
	int ret;
	struct wb_block_layer_info wb_blk;

	disp_check_and_return(!h_layer || !h_v_layer, -EINVAL, err, "NULL ptr\n");

	memset(&wb_blk, 0, sizeof(struct wb_block_layer_info));

	wb_blk.first_block = (h_layer->dst_rect.x >= ov_block_rect.x) ? 1 : 0;
	wb_blk.last_block = ((ov_block_rect.x + ov_block_rect.w) >=
		(h_layer->dst_rect.x + h_layer->dst_rect.w)) ? 1 : 0;
	wb_blk.output_startpos = dst_cross_rect.x - h_layer->dst_rect.x;
	wb_blk.output_span = dst_cross_rect.w;
	wb_blk.input_startpos = wb_blk.output_startpos;
	wb_blk.input_span = wb_blk.output_span;
	wb_blk.scf_dst_w = h_layer->dst_rect.w;

	wb_block_scf_process(pov_req_h_v, h_layer, ov_block_rect, &wb_blk);

	ret = get_rect_rot_transform(h_v_layer->transform, h_layer, &wb_blk);
	if (ret < 0)
		return ret;

	h_v_layer->src_rect = wb_blk.rect_transform;
	(void)rect_across_rect(h_v_layer->src_rect, h_v_layer->src_rect_mask, &h_v_layer->src_rect_mask);

	h_v_layer->dst_rect = dst_cross_rect;
	pov_req_h_v->wb_layers[0].wb_block.dst_rect = dst_cross_rect;

	pov_req_h_v->wb_layers[0].wb_block.src_rect.x = pov_req_h_v->wb_layers[0].src_rect.x +
		wb_blk.input_startpos;
	pov_req_h_v->wb_layers[0].wb_block.src_rect.y = pov_req_h_v->wb_layers[0].src_rect.y;
	pov_req_h_v->wb_layers[0].wb_block.src_rect.w = wb_blk.input_span;
	pov_req_h_v->wb_layers[0].wb_block.src_rect.h = pov_req_h_v->wb_layers[0].src_rect.h;

    disp_pr_info("[DATA] src w = %d, h = %d dst w = %d h = %d",
                pov_req_h_v->wb_layers[0].wb_block.src_rect.w, pov_req_h_v->wb_layers[0].wb_block.src_rect.h,
                pov_req_h_v->wb_layers[0].wb_block.dst_rect.w, pov_req_h_v->wb_layers[0].wb_block.dst_rect.h);

	/* rewrite when copybit can support wch scl */
	if (pov_req_h_v->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
		h_v_layer->dst_rect.x = pov_req_h_v->wb_layers[0].src_rect.x + wb_blk.input_startpos;
		h_v_layer->dst_rect.y = pov_req_h_v->wb_layers[0].src_rect.y;
		h_v_layer->dst_rect.w = wb_blk.input_span;
		h_v_layer->dst_rect.h = pov_req_h_v->wb_layers[0].src_rect.h;
		disp_pr_info("ADJUST[scl] h_v_layer->dst_rect.w = %d, h_v_layer->dst_rect.h = %d", h_v_layer->dst_rect.w,
			h_v_layer->dst_rect.h);
	}

	if (g_debug_ovl_block_composer)
		h_v_block_layer_print_info(pov_req_h_v, h_v_layer);

	return 0;
}

static int init_pov_req_v_block(struct ov_req_infos *pov_req, struct ov_req_infos *pov_req_h_v,
	struct disp_overlay_block *pov_h_block, struct disp_overlay_block *pov_h_v_block)
{
	memcpy(pov_req_h_v, pov_req, sizeof(struct ov_req_infos));
	pov_req_h_v->ov_block_infos_ptr = (uint64_t)(uintptr_t)(pov_h_v_block);

	if (calc_dest_block_size(pov_req, pov_h_block) == BLOCK_SIZE_INVALID) {
		pov_req_h_v->ov_block_nums = 1;
		memcpy(pov_h_v_block, pov_h_block, sizeof(struct disp_overlay_block));
		return -1;
	}

	pov_h_v_block->layer_nums = 0;
	memcpy(&pov_h_v_block->ov_block_rect, &pov_h_block->ov_block_rect, sizeof(struct disp_rect));

	return 0;
}

/* cal src blocks by h_ratio and dst blocks */
static void calc_wb_block_size(struct ov_req_infos *pov_req, struct disp_rect ov_block_rect,
	struct disp_rect *wb_ov_rect)
{
	uint32_t h_ratio;

	h_ratio = calc_h_ratio(pov_req);
	wb_ov_rect->x = (int32_t)(pov_req->wb_ov_rect.x + ((int64_t)ov_block_rect.x * h_ratio) / SCF_INC_FACTOR);
	wb_ov_rect->y = pov_req->wb_ov_rect.y;

	wb_ov_rect->w = (int32_t)(((int64_t)ov_block_rect.w * h_ratio) / SCF_INC_FACTOR);
	wb_ov_rect->h = ov_block_rect.h;
	disp_pr_info("ADJUST[block_has_layer] wb_ov_rect.w = %d, h = %d, x = %d, y = %d", wb_ov_rect->w, wb_ov_rect->h,
	wb_ov_rect->x, wb_ov_rect->y);
}

static bool get_wb_creat_value(struct ov_req_infos *pov_req)
{
	bool is_wb_creat = false;

	/* when copybit support wch scl use wb creat */
	if (pov_req->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot)
		is_wb_creat = true;

	return is_wb_creat;
}

static void h_layer_print_info(struct pipeline_src_layer *h_layer, struct disp_rect wb_ov_rect, int i)
{
	disp_pr_info("h_layer[%d](transform[%d], wb_ov_rect[%d,%d,%d,%d], src_rect[%d,%d,%d,%d], dst_rect[%d,%d,%d,%d])!\n",
		i, h_layer->transform, wb_ov_rect.x, wb_ov_rect.y, wb_ov_rect.w, wb_ov_rect.h,
		h_layer->src_rect.x, h_layer->src_rect.y, h_layer->src_rect.w, h_layer->src_rect.h,
		h_layer->dst_rect.x, h_layer->dst_rect.y, h_layer->dst_rect.w, h_layer->dst_rect.h);
}

static void h_v_layer_print_info(struct pipeline_src_layer *h_v_layer, struct disp_rect dst_cross_rect,
	int h_v_layer_idx, struct ov_req_infos *pov_req_h_v)
{
	disp_pr_info(
		"h_v_layer[%d](transform[%d], src_rect[%d,%d,%d,%d], dst_rect[%d,%d,%d,%d], dst_cross_rect[%d,%d,%d,%d]),"
		"wb_block.h_ratio=%d, wb_block.acc_hscl=%d!\n",
		h_v_layer_idx, h_v_layer->transform, h_v_layer->src_rect.x, h_v_layer->src_rect.y,
		h_v_layer->src_rect.w, h_v_layer->src_rect.h, h_v_layer->dst_rect.x, h_v_layer->dst_rect.y,
		h_v_layer->dst_rect.w, h_v_layer->dst_rect.h, dst_cross_rect.x, dst_cross_rect.y,
		dst_cross_rect.w, dst_cross_rect.h, pov_req_h_v->wb_layers[0].wb_block.h_ratio,
		pov_req_h_v->wb_layers[0].wb_block.acc_hscl);
}

int get_block_layers(struct ov_req_infos *pov_req, struct disp_overlay_block *pov_h_block,
	struct disp_rect ov_block_rect, struct ov_req_infos *pov_req_h_v, int block_num)
{
	uint32_t i;
	int ret;
	struct disp_rect dst_cross_rect = {0};
	struct disp_rect wb_ov_rect = {0};
	struct disp_overlay_block *pov_h_v_block = NULL;
	struct pipeline_src_layer *h_layer = NULL;
	struct pipeline_src_layer *h_v_layer = NULL;
	int h_v_layer_idx = 0;
	bool is_wb_creat = false;

	disp_check_and_return((!pov_req), -EINVAL, err, "pov_req is NULL.\n");

	disp_check_and_return((!pov_h_block), -EINVAL, err, "pov_h_block is NULL.\n");

	disp_check_and_return((!pov_req_h_v), -EINVAL, err, "pov_req_h_v is NULL.\n");

	disp_check_and_return((!ov_block_rect.w || !ov_block_rect.h), -1, err,
		"invaild args, ov_block_rect[%d,%d,%d,%d]!\n",
		ov_block_rect.x, ov_block_rect.y, ov_block_rect.w, ov_block_rect.y);

	/* init pov_req_v_block */
	pov_h_v_block = (struct disp_overlay_block *)(uintptr_t)pov_req_h_v->ov_block_infos_ptr;
	ret = init_pov_req_v_block(pov_req, pov_req_h_v, pov_h_block, pov_h_v_block);
	disp_check_and_return((ret != 0), 0, info, "calc_dest_block_size is invalid.\n");

	/* wb scl enable need calculation of OV block size before scaling based on dst size */
	calc_wb_block_size(pov_req, ov_block_rect, &wb_ov_rect);

	is_wb_creat = get_wb_creat_value(pov_req);

	disp_pr_info("layer_num = %d\n", pov_h_block->layer_nums);

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		h_layer = &(pov_h_block->layer_infos[i]);

		ret = rect_across_rect(h_layer->dst_rect, wb_ov_rect, &dst_cross_rect);
		if (ret == 0)
			continue;

		h_v_layer = &(pov_h_v_block->layer_infos[h_v_layer_idx]);
		memcpy(h_v_layer, h_layer, sizeof(struct pipeline_src_layer));
		h_v_layer->layer_id = h_v_layer_idx;

		disp_pr_info("h_v_layer dma_id = %d, h_layer dma_id = %d", h_v_layer->dma_sel, h_layer->dma_sel);

		if (is_wb_creat)
			ret = wb_create_h_v_block_layer(pov_req_h_v, h_layer, h_v_layer, dst_cross_rect, wb_ov_rect);
		else
			ret = create_h_v_block_layer(h_layer, h_v_layer, dst_cross_rect, wb_ov_rect);

		get_dst_rect_for_wb_scl(pov_req, pov_req_h_v, ov_block_rect, block_num);

		if (ret != 0 || g_debug_ovl_block_composer) {
			// TODO: add debug info
			h_layer_print_info(h_layer, wb_ov_rect, i);
			h_v_layer_print_info(h_v_layer, dst_cross_rect, h_v_layer_idx, pov_req_h_v);
		}

		if (ret != 0) {
			disp_pr_err("create_h_v_block_layer failed, h_layer[%d], h_v_layer[%d]!\n", i, h_v_layer_idx);
			break;
		}

		h_v_layer_idx++;
		pov_h_v_block->layer_nums = h_v_layer_idx;
	}

	return ret;
}

int get_wb_layer_block_rect(struct disp_wb_layer *wb_layer, bool has_wb_scl, struct disp_rect *wb_layer_block_rect)
{
	disp_check_and_return(!wb_layer, -1, err, "wb_layer is NULL point!\n");
	disp_check_and_return(!wb_layer_block_rect, -1, err, "wb_layer_block_rect is NULL point!\n");

	if (has_wb_scl) {
		if (wb_layer->transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
			wb_layer_block_rect->x = wb_layer->dst_rect.x;
			wb_layer_block_rect->y = wb_layer->dst_rect.y;
			wb_layer_block_rect->w = wb_layer->dst_rect.h;
			wb_layer_block_rect->h = wb_layer->dst_rect.w;
		} else {
			*wb_layer_block_rect = wb_layer->dst_rect;
		}
	} else {
		*wb_layer_block_rect = wb_layer->src_rect;
	}

	return 0;
}

int get_wb_ov_block_rect(struct disp_rect ov_block_rect, struct disp_rect wb_src_rect,
	struct disp_rect *wb_ov_block_rect, struct ov_req_infos *pov_req_h_v, uint32_t wb_type)
{
	int ret;
	struct disp_overlay_block *pov_h_v_block = NULL;

	ret = rect_across_rect(ov_block_rect, wb_src_rect, wb_ov_block_rect);

	if (wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
		/* when wb scaler, ov block is rch dst rect */
		pov_h_v_block = (struct disp_overlay_block *)(pov_req_h_v->ov_block_infos_ptr);

		wb_ov_block_rect->x = pov_h_v_block->layer_infos[0].dst_rect.x;
		wb_ov_block_rect->y = pov_h_v_block->layer_infos[0].dst_rect.y;
		wb_ov_block_rect->w = pov_h_v_block->layer_infos[0].dst_rect.w;
		wb_ov_block_rect->h = pov_h_v_block->layer_infos[0].dst_rect.h;
		disp_pr_info("ADJUST[scl] wb_ov_block_rect w = %d h = %d", wb_ov_block_rect->w,
				wb_ov_block_rect->h);
		ret = 1;
	}

	return ret;
}


