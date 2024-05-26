// ***********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    df_cvdr_opt.c
// Description:
//
// Version      1.0
// Date         2019-05-24 11:17:22
// ***********************************************************

#include "ipp.h"
#include "config_table_cvdr.h"
#include "cvdr_opt.h"
#include <linux/printk.h>

#define LOG_TAG LOG_MODULE_CVDR_OPT

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

enum cvdr_dev_e {
	CVDR_RT = 0,
	CVDR_SRT = 1,
};

#define ISP_CLK   600
#define DERATE    1.2
#define ISP_BASE_ADDR 0xE8400000
#define ISP_BASE_ADDR_CVDR_RT (ISP_BASE_ADDR + 0x00022000)
#define CVDR_RT_LIMITER_VP_RD_10_REG  0x8A8
#define CVDR_RT_LIMITER_VP_WR_26_REG  0x468
#define CVDR_VP_WR_NBR 58
#define ISP_BASE_ADDR_CVDR_SRT  (ISP_BASE_ADDR + 0x0002E000)
#define CVDR_SRT_VP_WR_IF_CFG_0_REG   0x28
#define CVDR_VP_RD_NBR  28
#define CVDR_SRT_VP_RD_IF_CFG_0_REG 0x514
#define CVDR_RT_CVDR_CFG_REG  0x0


/*lint +e524*/
int calculate_cvdr_bw_limiter(struct cvdr_bw_cfg_t *bw)
{
	bw->bw_limiter0  = 0xF;
	bw->bw_limiter1  = 0xF;
	bw->bw_limiter2  = 0xF;
	bw->bw_limiter3  = 0xF;
	bw->bw_limiter_reload = 0xF;
	D("bw_limiter: %d, %d, %d, %d, reload: %x, ",
	  bw->bw_limiter0, bw->bw_limiter1, bw->bw_limiter2,
	  bw->bw_limiter3, bw->bw_limiter_reload);
	return ISP_IPP_OK;
}

int cfg_tbl_cvdr_nr_wr_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id)
{
	p_cvdr_cfg->nr_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->nr_wr_cfg[id].en     = 1;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->nr_wr_cfg[id].bw));
	return ISP_IPP_OK;
}

int cfg_tbl_cvdr_nr_rd_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id)
{
	p_cvdr_cfg->nr_rd_cfg[id].to_use = 1;
	p_cvdr_cfg->nr_rd_cfg[id].en     = 1;
	p_cvdr_cfg->nr_rd_cfg[id].allocated_du = 6;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->nr_rd_cfg[id].bw));
	return ISP_IPP_OK;
}

int dataflow_cvdr_wr_cfg_vp(
	struct cfg_tab_cvdr_t *p_cvdr_cfg,
	struct cvdr_opt_fmt_t *cfmt, unsigned int line_stride,
	unsigned int isp_clk, enum pix_format_e format)
{
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		E("input param is NULL!");
		return ISP_IPP_ERR;
	}

	if (cfmt->id >= IPP_VP_WR_MAX) {
		E("input id is overflow!");
		return ISP_IPP_ERR;
	}

	unsigned int i              = 0;
	unsigned int pix_size;
	unsigned int id             = cfmt->id;
	unsigned int config_num     = 0;
	unsigned int total_bytes[2] = {0};
	unsigned int config_width;
	unsigned int line_bytes;
	enum cvdr_pix_fmt_e pix_fmt = DF_1PF8;

	switch (format) {
	case PIXEL_FMT_IPP_Y8:
	case PIXEL_FMT_IPP_1PF8:
		pix_fmt    = DF_1PF8;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_2PF8:
		pix_fmt    = DF_2PF8;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_3PF8:
		pix_fmt    = DF_3PF8;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_D32:
		pix_fmt    = DF_D32;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_D48:
		pix_fmt    = DF_D48;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_D64:
		pix_fmt    = DF_D64;
		config_num = 1;
		break;

	case PIXEL_FMT_IPP_D128:
		pix_fmt    = DF_D128;
		config_num = 1;
		break;

	default:
		E("pix_fmt unsupported! format = %d", format);
		return ISP_IPP_ERR;
	}

	cfmt->pix_fmt = pix_fmt;

	pix_size = (cfmt->expand == EXP_PIX) ?
			   pix_fmt_info[pix_fmt].expand_size :
			   pix_fmt_info[pix_fmt].compact_size;
	line_bytes   = cfmt->width * pix_size / 8;
	config_width = ALIGN_UP(line_bytes, CVDR_ALIGN_BYTES);
	total_bytes[0] = ALIGN_UP(config_width * cfmt->height,
		CVDR_TOTAL_BYTES_ALIGN);
	total_bytes[1] = ALIGN_UP(config_width * (cfmt->height),
		CVDR_TOTAL_BYTES_ALIGN);
	struct cvdr_vp_wr_cfg_t *vp_wr_cfg = NULL;

	for (i = id; i < (id + config_num); i++) {
		vp_wr_cfg = &(p_cvdr_cfg->vp_wr_cfg[i]);
		vp_wr_cfg->to_use = 1;
		vp_wr_cfg->id = i;
		vp_wr_cfg->fmt.fs_addr = cfmt->addr + (i - id) *
			(config_width * cfmt->height);
		vp_wr_cfg->fmt.last_page =
		(p_cvdr_cfg->vp_wr_cfg[i].fmt.fs_addr +
		ALIGN_UP(ALIGN_UP(cfmt->full_width * pix_size / 8,
		CVDR_ALIGN_BYTES) * cfmt->height,
		CVDR_TOTAL_BYTES_ALIGN)) >> 15;
		vp_wr_cfg->fmt.pix_fmt = pix_fmt;
		vp_wr_cfg->fmt.pix_expan = cfmt->expand;
		vp_wr_cfg->fmt.line_stride =
		ALIGN_UP(cfmt->full_width * pix_size / 8, CVDR_ALIGN_BYTES) /
		CVDR_ALIGN_BYTES - 1;
		vp_wr_cfg->fmt.line_wrap = DEFAULT_LINE_WRAP;
		calculate_cvdr_bw_limiter(&(vp_wr_cfg->bw));

		if (line_stride != 0) {
			unsigned int cvdr_align =
			ALIGN_UP(line_stride, CVDR_ALIGN_BYTES);
			unsigned int cvdr_total_bytes =
			cvdr_align * cfmt->height;

			cvdr_total_bytes =
			ALIGN_UP(cvdr_total_bytes,
			CVDR_TOTAL_BYTES_ALIGN);

			vp_wr_cfg->fmt.line_stride =
			cvdr_align / CVDR_ALIGN_BYTES - 1;
			vp_wr_cfg->fmt.last_page =
			(vp_wr_cfg->fmt.fs_addr + ALIGN_UP
			(cvdr_total_bytes, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
		}
		D("fs_addr = 0x%x, id = %d, line_stride = %d\n",
			p_cvdr_cfg->vp_wr_cfg[i].fmt.fs_addr, i,
			p_cvdr_cfg->vp_wr_cfg[i].fmt.line_stride);
		D("line_wrap = %d, config_width  = %d,\n",
		  cfmt->height, config_width);
		D("last_page = 0x%x, total_byte = %d, pix_fmt = %d\n",
		  p_cvdr_cfg->vp_wr_cfg[i].fmt.last_page,
		  total_bytes[i - id], p_cvdr_cfg->vp_wr_cfg[i].fmt.pix_fmt);
	}

	return ISP_IPP_OK;
}

int dataflow_cvdr_rd_cfg_vp(
		struct cfg_tab_cvdr_t *p_cvdr_cfg,
		struct cvdr_opt_fmt_t *cfmt, unsigned int line_stride,
		unsigned int isp_clk, enum pix_format_e format)
{
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		E("input param is NULL!");
		return ISP_IPP_ERR;
	}

	if (cfmt->id >= IPP_VP_RD_MAX) {
		E("input id is overflow!");
		return ISP_IPP_ERR;
	}

	unsigned int pix_size;
	unsigned int config_width;
	unsigned int id            = cfmt->id;
	unsigned int line_bytes;
	enum cvdr_pix_fmt_e pix_fmt     = DF_FMT_INVALID;

	switch (format) {
	case PIXEL_FMT_IPP_Y8:
	case PIXEL_FMT_IPP_1PF8:
		pix_fmt    = DF_1PF8;
		break;

	case PIXEL_FMT_IPP_2PF8:
		pix_fmt    = DF_2PF8;
		break;

	case PIXEL_FMT_IPP_3PF8:
		pix_fmt    = DF_3PF8;
		break;

	case PIXEL_FMT_IPP_D32:
		pix_fmt    = DF_D32;
		break;

	case PIXEL_FMT_IPP_D48:
		pix_fmt    = DF_D48;
		break;

	case PIXEL_FMT_IPP_D64:
		pix_fmt    = DF_D64;
		break;

	case PIXEL_FMT_IPP_D128:
		pix_fmt    = DF_D128;
		break;

	default:
		E("pix_fmt unsupported! format = %d", format);
		return ISP_IPP_ERR;
	}

	cfmt->pix_fmt = pix_fmt;
	pix_size = (cfmt->expand == EXP_PIX) ?
			   pix_fmt_info[pix_fmt].expand_size :
			   pix_fmt_info[pix_fmt].compact_size;
	line_bytes = cfmt->width * pix_size / 8;
	config_width = ALIGN_UP(line_bytes, CVDR_ALIGN_BYTES);
	unsigned int du = IPP_CVDR_DU_VALUE;

	p_cvdr_cfg->vp_rd_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_rd_cfg[id].id = id;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page =
	(cfmt->addr + ALIGN_UP(ALIGN_UP(cfmt->full_width * pix_size / 8,
	CVDR_ALIGN_BYTES) * cfmt->height, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_fmt = pix_fmt;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_expan = cfmt->expand;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.allocated_du = du;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_size = cfmt->line_size - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.hblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.frame_size = cfmt->height - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.vblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride = ALIGN_UP(cfmt->full_width *
		pix_size / 8, CVDR_ALIGN_BYTES) / CVDR_ALIGN_BYTES - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;

	if (line_stride != 0) {
		p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride =
		ALIGN_UP(line_stride, CVDR_ALIGN_BYTES) / CVDR_ALIGN_BYTES - 1;
		p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page =
		(cfmt->addr + ALIGN_UP(ALIGN_UP(line_stride, CVDR_ALIGN_BYTES) *
		cfmt->height, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
	}

	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_rd_cfg[id].bw));
	return ISP_IPP_OK;
}

int dataflow_cvdr_wr_cfg_d32(
	struct cfg_tab_cvdr_t *p_cvdr_cfg, struct cvdr_opt_fmt_t *cfmt,
	unsigned int line_stride, unsigned int total_bytes)
{
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		E("input param is NULL!");
		return ISP_IPP_ERR;
	}

	unsigned int id = cfmt->id;

	p_cvdr_cfg->vp_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_wr_cfg[id].id = id;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.last_page =
		(p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr + total_bytes) >> 15;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_fmt = DF_D32;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_expan = 1;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_stride = line_stride;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_wr_cfg[id].bw));
	return 0;
}


int cfg_tbl_cvdr_rd_cfg_d64(
	struct cfg_tab_cvdr_t *p_cvdr_cfg, struct cvdr_opt_fmt_t *cfmt,
	unsigned int total_bytes, unsigned int line_stride)
{
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		D("input param is NULL!");
		return -1;
	}

	unsigned int id = cfmt->id;
	unsigned int allocated_du = 2;

	p_cvdr_cfg->vp_rd_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_rd_cfg[id].id = cfmt->id;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page =
		(cfmt->addr + total_bytes) >> 15;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_fmt = DF_D64;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_expan = 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.allocated_du = allocated_du;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_size = cfmt->line_size - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.hblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.frame_size = cfmt->height - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.vblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride =
		(line_stride > 0) ? (line_stride - 1) : 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_rd_cfg[id].bw));
	return 0;
}

int cfg_tbl_cvdr_wr_cfg_d64(struct cfg_tab_cvdr_t *p_cvdr_cfg,
	struct cvdr_opt_fmt_t *cfmt, unsigned int total_bytes)
{
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		E("input param is NULL!");
		return -1;
	}

	unsigned int id = cfmt->id;

	p_cvdr_cfg->vp_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_wr_cfg[id].id = id;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.last_page =
		(p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr + total_bytes) >> 15;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_fmt = DF_D64;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_expan = 1;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_stride = 0;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_wr_cfg[id].bw));

	return 0;
}

#pragma GCC diagnostic pop
/* ***************************end************************************** */

