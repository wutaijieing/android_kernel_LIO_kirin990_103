/******************************************************************
 * Copyright    Copyright (c) 2015- Hisilicon Technologies CO., Ltd.
 * File name    df_cvdr_opt.h
 * Description:
 *
 * Date         2019-05-24 11:23:22
 *******************************************************************/

#ifndef _CVDR_OPT_CS_H
#define _CVDR_OPT_CS_H

#include "cvdr_drv.h"
#include "cvdr.h"
#include "cfg_table_cvdr.h"

#define IPP_CVDR_VP_WR_NBR    (IPP_VP_WR_MAX)
#define IPP_CVDR_VP_RD_NBR    (IPP_VP_RD_MAX)
#define IPP_CVDR_NR_WR_NBR    1
#define IPP_CVDR_NR_RD_NBR    2
#define IPP_CVDR_SPARE_NUM    0

struct cvdr_opt_bw_t {
	unsigned int       srt;
	unsigned int       pclk;
	unsigned int       throughput;
};

struct cvdr_opt_fmt_t {
	unsigned int       id;
	unsigned int       width;
	unsigned int       height;
	unsigned int       line_size;
	unsigned int       addr;
	unsigned int       expand;
	unsigned int       full_width;
	enum cvdr_pix_fmt_e      pix_fmt;
};

struct pix_fmt_info_t {
	char name[64];
	int  compact_size;
	int  expand_size;
	int  pix_num;
	int  pix_fmt;
};

static const struct pix_fmt_info_t pix_fmt_info[DF_FMT_INVALID] = {
	{ "1PF8",   8,  8, 1,  8 },
	{ "2PF8",  16, 16, 2,  8 },
	{ "0",  24, 32, 3,  8 },
	{ "0",  32, 32, 4, 8 },
	{ "1PF10", 10, 16, 1, 10 }, // 4
	{ "0", 20, 32, 2, 10 },
	{ "0", 30, 32, 3, 10 },
	{ "0", 40, 64, 4, 10 },
	{ "0", 12, 16, 1, 12 },
	{ "0", 24, 32, 2, 12 },
	{ "0", 36, 64, 3, 12 },
	{ "0", 48, 64, 4, 12 },
	{ "0", 14, 16, 1, 14 },
	{ "2PF14", 28, 32, 2, 14 }, // 13
	{ "0", 42, 64, 3, 14 },
	{ "0", 56, 64, 4, 14 },
	{ "0",   8,  8, 1,  8 },
	{ "0",  16, 16, 2,  8 },
	{ "0",  24, 32, 3,  8 },
	{ "0",  32, 32, 4, 8 },
	{ "0", 10, 16, 1, 10 },

	{ "0", 20, 32, 2, 10 },
	{ "0", 30, 32, 3, 10 },
	{ "0", 40, 64, 4, 10 },
	{ "0", 12, 16, 1, 12 },
	{ "3PF8",  24, 32, 3,  8 }, // 25
	{ "0", 36, 64, 3, 12 },
	{ "0", 48, 64, 4, 12 },
	{ "D32",   32, 32, 1, 32 }, // 28
	{ "D48",   48, 64, 1, 48 },
	{ "D64",   64, 64, 1, 64 },
	{ "D128",  128, 128, 1, 128}, // 31
};

int cfg_tbl_cvdr_nr_wr_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id);
int cfg_tbl_cvdr_nr_rd_cfg(struct cfg_tab_cvdr_t *p_cvdr_cfg, int id);
int dataflow_cvdr_wr_cfg_vp(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt,
							unsigned int line_stride, unsigned int isp_clk, enum pix_format_e format);
int dataflow_cvdr_rd_cfg_vp(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt,
							unsigned int line_stride, unsigned int isp_clk, enum pix_format_e format);
int dataflow_cvdr_wr_cfg_d32(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							 struct cvdr_opt_fmt_t *cfmt,
							 unsigned int line_stride, unsigned int total_bytes);
int cfg_tbl_cvdr_rd_cfg_d64(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt,
							unsigned int total_bytes, unsigned int line_stride);
int cfg_tbl_cvdr_wr_cfg_d64(struct cfg_tab_cvdr_t *p_cvdr_cfg,
							struct cvdr_opt_fmt_t *cfmt, unsigned int total_bytes);

#endif /* _CVDR_OPT_CS_H */

/* *******************************end************************************ */

