// ***********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    df_cvdr_opt.c
// Description:
//
// Date         2019-05-24 11:17:22
// ***********************************************************

#include "ipp.h"
#include "cfg_table_cvdr.h"
#include "cvdr_opt.h"
#include <linux/printk.h>

#define LOG_TAG LOG_MODULE_CVDR_OPT


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

static struct cvdr_opt_bw_t cvdr_vp_wr_bw[IPP_CVDR_VP_WR_NBR] = {
	[IPP_VP_WR_CMDLST] = {CVDR_SRT, ISP_CLK, (float)DERATE * ISP_CLK},
	[IPP_VP_WR_ORB_ENH_Y] = {CVDR_SRT, ISP_CLK, ISP_CLK}, // need confirm throughput
	[IPP_VP_WR_ARF_PRY_1] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_ARF_PRY_2] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_ARF_DOG_0] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_ARF_DOG_1] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_ARF_DOG_2] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_ARF_DOG_3] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_HIOF_SPARSE_MD] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_HIOF_DENSE_TNR] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_GF_LF_A] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_WR_GF_HF_B] = {CVDR_SRT, ISP_CLK, ISP_CLK},
};

static struct cvdr_opt_bw_t cvdr_vp_rd_bw[IPP_CVDR_VP_RD_NBR] = {
	[IPP_VP_RD_CMDLST] = {CVDR_SRT, ISP_CLK, (float)DERATE * ISP_CLK},
	[IPP_VP_RD_ORB_ENH_Y_HIST] = {CVDR_SRT, ISP_CLK, ISP_CLK}, // need confirm throughput
	[IPP_VP_RD_ORB_ENH_Y_IMAGE] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_ARF_0] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_ARF_1] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_ARF_2] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_ARF_3] = {CVDR_SRT, ISP_CLK,  ISP_CLK},
	[IPP_VP_RD_HIOF_REF_Y] = {CVDR_SRT, ISP_CLK, 720},
	[IPP_VP_RD_HIOF_CUR_Y] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_RDR] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_CMP] = {CVDR_SRT, ISP_CLK,  ISP_CLK},
	[IPP_VP_RD_GF_SRC_P] = {CVDR_SRT, ISP_CLK, ISP_CLK},
	[IPP_VP_RD_GF_GUI_I] = {CVDR_SRT, ISP_CLK, ISP_CLK},
};

static struct cvdr_opt_bw_t cvdr_nr_wr_bw[IPP_CVDR_NR_WR_NBR] = {
	[IPP_NR_WR_RDR]    = {CVDR_SRT, ISP_CLK, ISP_CLK},
};

static struct cvdr_opt_bw_t cvdr_nr_rd_bw[IPP_CVDR_NR_RD_NBR] = {
	[IPP_NR_RD_CMP]    = {CVDR_SRT, ISP_CLK, ISP_CLK}, // CMP
	[IPP_NR_RD_MC]  = {CVDR_SRT, ISP_CLK, ISP_CLK}, // MC
};

int calculate_cvdr_bw_limiter(struct cvdr_bw_cfg_t *bw,
							  unsigned int throughput)
{
	// int num_75mbytes; // v350 no use
	// num_75mbytes  = (throughput + 74) / 75;
	bw->bw_limiter0  = 0xF;
	bw->bw_limiter1  = 0xF;
	bw->bw_limiter2  = 0xF;
	bw->bw_limiter3  = 0xF;
	bw->bw_limiter_reload = 0xF;
	logd("bw_limiter: %d, %d, %d, %d, reload: %x, throughput: %d",
		 bw->bw_limiter0, bw->bw_limiter1, bw->bw_limiter2,
		 bw->bw_limiter3, bw->bw_limiter_reload, throughput);
	return ISP_IPP_OK;
}

int calculate_cvdr_allocated_du(
	unsigned int pclk, unsigned int throughput, int is_dgen)
{
	unsigned int num_du;

	if (throughput > 0)
		num_du = 6;
	else
		num_du = 0;

	return num_du;
}

int cfg_tbl_cvdr_nr_wr_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id)
{
	p_cvdr_cfg->nr_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->nr_wr_cfg[id].en     = 1;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->nr_wr_cfg[id].bw),
							  cvdr_nr_wr_bw[id].throughput);
	return ISP_IPP_OK;
}

int cfg_tbl_cvdr_nr_rd_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id)
{
	p_cvdr_cfg->nr_rd_cfg[id].to_use = 1;
	p_cvdr_cfg->nr_rd_cfg[id].en     = 1;
	p_cvdr_cfg->nr_rd_cfg[id].allocated_du = 6;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->nr_rd_cfg[id].bw),
							  cvdr_nr_rd_bw[id].throughput);
	logd("allocated_du: %d, pclk: %d", p_cvdr_cfg->nr_rd_cfg[id].allocated_du,
		 cvdr_nr_rd_bw[id].pclk);
	return ISP_IPP_OK;
}

static int cvdr_wr_cfg_vp_format_convert(enum pix_format_e format, enum cvdr_pix_fmt_e *pix_fmt,
		unsigned int *config_num)
{
	switch (format) {
	case PIXEL_FMT_IPP_Y8:
	case PIXEL_FMT_IPP_1PF8:
		*pix_fmt    = DF_1PF8;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_2PF8:
		*pix_fmt    = DF_2PF8;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_2PF14:
		*pix_fmt    = DF_2PF14;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_3PF8:
		*pix_fmt    = DF_3PF8;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_D32:
		*pix_fmt    = DF_D32;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_D64:
		*pix_fmt    = DF_D64;
		*config_num = 1;
		break;

	case PIXEL_FMT_IPP_D128:
		*pix_fmt    = DF_D128;
		*config_num = 1;
		break;

	default:
		loge("Failed:pix_fmt unsupported! format = %d", format);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

int dataflow_cvdr_wr_cfg_vp(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt, unsigned int line_stride,
							unsigned int isp_clk, enum pix_format_e format)
{
	unsigned int pix_size;
	unsigned int i;
	unsigned int config_num     = 0;
	unsigned int total_bytes[2] = {0};
	unsigned int config_width;
	enum cvdr_pix_fmt_e pix_fmt = DF_1PF8;
	struct cvdr_vp_wr_cfg_t *vp_wr_cfg = NULL;
	dataflow_check_para(p_cvdr_cfg);
	dataflow_check_para(cfmt);
	cvdr_wr_cfg_vp_format_convert(format, &pix_fmt, &config_num);
	cfmt->pix_fmt = pix_fmt;
	pix_size = (cfmt->expand == EXP_PIX) ?
			   pix_fmt_info[pix_fmt].expand_size :
			   pix_fmt_info[pix_fmt].compact_size;
	config_width = align_up(cfmt->width * pix_size / 8, CVDR_ALIGN_BYTES);
	total_bytes[0] = align_up(config_width * cfmt->height, CVDR_TOTAL_BYTES_ALIGN);
	total_bytes[1] = align_up(config_width * (cfmt->height), CVDR_TOTAL_BYTES_ALIGN);

	for (i = cfmt->id; i < (cfmt->id + config_num); i++) {
		vp_wr_cfg = &(p_cvdr_cfg->vp_wr_cfg[i]);
		vp_wr_cfg->to_use = 1;
		vp_wr_cfg->id = i;
		vp_wr_cfg->fmt.fs_addr = cfmt->addr + (i - cfmt->id) * (config_width * cfmt->height);
		vp_wr_cfg->fmt.last_page = (p_cvdr_cfg->vp_wr_cfg[i].fmt.fs_addr + align_up(align_up(cfmt->full_width * pix_size / 8,
									CVDR_ALIGN_BYTES) * cfmt->height, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
		vp_wr_cfg->fmt.pix_fmt = pix_fmt;
		vp_wr_cfg->fmt.pix_expan = cfmt->expand;
		vp_wr_cfg->fmt.line_stride = align_up(cfmt->full_width * pix_size / 8, CVDR_ALIGN_BYTES) / CVDR_ALIGN_BYTES - 1;
		vp_wr_cfg->fmt.line_wrap = DEFAULT_LINE_WRAP;
		calculate_cvdr_bw_limiter(&(vp_wr_cfg->bw), cvdr_vp_wr_bw[i].throughput);

		if (line_stride != 0) {
			unsigned int cvdr_align = align_up(line_stride, CVDR_ALIGN_BYTES);
			unsigned int cvdr_total_bytes = cvdr_align * cfmt->height;
			cvdr_total_bytes = align_up(cvdr_total_bytes, CVDR_TOTAL_BYTES_ALIGN);
			vp_wr_cfg->fmt.line_stride = cvdr_align / CVDR_ALIGN_BYTES - 1;
			vp_wr_cfg->fmt.last_page = (vp_wr_cfg->fmt.fs_addr + align_up
										(cvdr_total_bytes, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
		}

		logd("fs_addr = 0x%x, id = %d, line_stride = %d, cfmt->full_width=%d", p_cvdr_cfg->vp_wr_cfg[i].fmt.fs_addr, i,
			 p_cvdr_cfg->vp_wr_cfg[i].fmt.line_stride, cfmt->full_width);
		logd("line_wrap = %d, config_width  = %d", cfmt->height, config_width);
		logd("last_page = 0x%x, total_byte = %d, pix_fmt = %d", p_cvdr_cfg->vp_wr_cfg[i].fmt.last_page,
			 total_bytes[i - cfmt->id], p_cvdr_cfg->vp_wr_cfg[i].fmt.pix_fmt);
	}

	return ISP_IPP_OK;
}

static int cvdr_rd_cfg_vp_format_convert(enum pix_format_e format, enum cvdr_pix_fmt_e *pix_fmt)
{
	switch (format) {
	case PIXEL_FMT_IPP_Y8:
	case PIXEL_FMT_IPP_1PF8:
		*pix_fmt    = DF_1PF8;
		break;

	case PIXEL_FMT_IPP_1PF10:
		*pix_fmt    = DF_1PF10;
		break;

	case PIXEL_FMT_IPP_2PF8:
		*pix_fmt    = DF_2PF8;
		break;

	case PIXEL_FMT_IPP_3PF8:
		*pix_fmt    = DF_3PF8;
		break;

	case PIXEL_FMT_IPP_D32:
		*pix_fmt    = DF_D32;
		break;

	case PIXEL_FMT_IPP_D64:
		*pix_fmt    = DF_D64;
		break;

	case PIXEL_FMT_IPP_D128:
		*pix_fmt    = DF_D128;
		break;

	default:
		loge("Failed:pix_fmt unsupported! format = %d", format);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}


int dataflow_cvdr_rd_cfg_vp(struct cfg_tab_cvdr_t *p_cvdr_cfg, struct cvdr_opt_fmt_t *cfmt, unsigned int line_stride,
							unsigned int isp_clk, enum pix_format_e format)
{
	unsigned int pix_size;
	unsigned int config_width;
	unsigned int id = 0;
	unsigned int line_bytes;
	unsigned int du = 0;
	enum cvdr_pix_fmt_e pix_fmt     = DF_FMT_INVALID;
	dataflow_check_para(p_cvdr_cfg);
	dataflow_check_para(cfmt);
	id = cfmt->id;
	cvdr_rd_cfg_vp_format_convert(format, &pix_fmt);
	cfmt->pix_fmt = pix_fmt;
	pix_size = (cfmt->expand == EXP_PIX) ?
			   pix_fmt_info[pix_fmt].expand_size :
			   pix_fmt_info[pix_fmt].compact_size;
	line_bytes = cfmt->width * pix_size / 8;
	config_width = align_up(line_bytes, CVDR_ALIGN_BYTES);
	du = calculate_cvdr_allocated_du(cvdr_vp_rd_bw[id].pclk,
					  cvdr_vp_rd_bw[id].throughput, 0);
	p_cvdr_cfg->vp_rd_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_rd_cfg[id].id = id;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page = (cfmt->addr + align_up(align_up(cfmt->full_width * pix_size / 8,
			CVDR_ALIGN_BYTES) * cfmt->height, CVDR_TOTAL_BYTES_ALIGN)) >> 15;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_fmt = pix_fmt; // v350 must be DF_2PF8
	p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_expan = cfmt->expand;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.allocated_du = du;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_size = cfmt->line_size - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.hblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.frame_size = cfmt->height - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.vblank = 0;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride = align_up(cfmt->full_width *
			pix_size / 8, CVDR_ALIGN_BYTES) / CVDR_ALIGN_BYTES - 1;
	p_cvdr_cfg->vp_rd_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;

	if (line_stride != 0) {
		p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride = align_up(line_stride, CVDR_ALIGN_BYTES) / CVDR_ALIGN_BYTES - 1;
		p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page =
			(cfmt->addr + align_up(align_up(line_stride, CVDR_ALIGN_BYTES) * cfmt->height,
				CVDR_TOTAL_BYTES_ALIGN)) >> 15;
	}

	logd("fs_addr = 0x%x, id = %d, line_stride = %d, line_size  =%d, frame_size = %d",
		 p_cvdr_cfg->vp_rd_cfg[id].fmt.fs_addr, id, p_cvdr_cfg->vp_rd_cfg[id].fmt.line_stride,
		 p_cvdr_cfg->vp_rd_cfg[id].fmt.line_size, p_cvdr_cfg->vp_rd_cfg[id].fmt.frame_size);
	logd("pix_fmt = 0x%x, pix_expan = 0x%x, allocated_du = 0x%x, last_page = 0x%x",
		 p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_fmt, p_cvdr_cfg->vp_rd_cfg[id].fmt.pix_expan,
		 p_cvdr_cfg->vp_rd_cfg[id].fmt.allocated_du, p_cvdr_cfg->vp_rd_cfg[id].fmt.last_page);
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_rd_cfg[id].bw), cvdr_vp_rd_bw[id].throughput);
	return ISP_IPP_OK;
}

int dataflow_cvdr_wr_cfg_d32(
	struct cfg_tab_cvdr_t *p_cvdr_cfg, struct cvdr_opt_fmt_t *cfmt,
	unsigned int line_stride, unsigned int total_bytes)
{
	unsigned int id = 0;
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		loge("Failed:input param is NULL!");
		return ISP_IPP_ERR;
	}

	id = cfmt->id;
	p_cvdr_cfg->vp_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_wr_cfg[id].id = id;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.last_page =
		(p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr + total_bytes) >> 15;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_fmt = DF_D32;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_expan = 1;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_stride = line_stride;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_wr_cfg[id].bw),
							  cvdr_vp_wr_bw[id].throughput);
	return 0;
}


int cfg_tbl_cvdr_rd_cfg_d64(
	struct cfg_tab_cvdr_t *p_cvdr_cfg, struct cvdr_opt_fmt_t *cfmt,
	unsigned int total_bytes, unsigned int line_stride)
{
	unsigned int id;
	unsigned int allocated_du = 2;
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		logd("input param is NULL!");
		return -1;
	}

	id = cfmt->id;
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
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_rd_cfg[id].bw),
							  cvdr_vp_rd_bw[id].throughput);
	return 0;
}

int cfg_tbl_cvdr_wr_cfg_d64(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt, unsigned int total_bytes)
{
	unsigned int id = 0;
	if (p_cvdr_cfg == NULL || cfmt == NULL) {
		loge("Failed:input param is NULL!");
		return -1;
	}

	id = cfmt->id;
	p_cvdr_cfg->vp_wr_cfg[id].to_use = 1;
	p_cvdr_cfg->vp_wr_cfg[id].id = id;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr = cfmt->addr;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.last_page =
		(p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr + total_bytes) >> 15;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_fmt = DF_D64;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.pix_expan = 1;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_stride = 0;
	p_cvdr_cfg->vp_wr_cfg[id].fmt.line_wrap = DEFAULT_LINE_WRAP;
	calculate_cvdr_bw_limiter(&(p_cvdr_cfg->vp_wr_cfg[id].bw),
							  cvdr_vp_wr_bw[id].throughput);
	logd("fs_addr = 0x%x, id = %d, last_page = 0x%x, total_byte = %d",
		 p_cvdr_cfg->vp_wr_cfg[id].fmt.fs_addr, id,
		 p_cvdr_cfg->vp_wr_cfg[id].fmt.last_page,
		 total_bytes);
	return 0;
}

/* ***************************end************************************** */

