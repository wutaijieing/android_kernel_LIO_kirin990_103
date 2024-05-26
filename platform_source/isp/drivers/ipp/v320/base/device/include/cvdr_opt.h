/******************************************************************
 * Copyright    Copyright (c) 2015- Hisilicon Technologies CO., Ltd.
 * File name    df_cvdr_opt.h
 * Description:
 *
 * Version      1.0
 * Date         2019-05-24 11:23:22
 *******************************************************************/

#ifndef _CVDR_OPT_CS_H
#define _CVDR_OPT_CS_H

#include "cvdr_drv.h"
#include "cvdr.h"
#include "config_table_cvdr.h"

#define IPP_CVDR_VP_WR_NBR    (WR_VP_ELEMENT_MAX)
#define IPP_CVDR_VP_RD_NBR    (RD_VP_ELEMENT_MAX)
#define IPP_CVDR_NR_WR_NBR    1
#define IPP_CVDR_NR_RD_NBR    2
#define IPP_CVDR_SPARE_NUM    0
#define IPP_CVDR_DU_VALUE 6

enum ipp_vp_rd_e {
	RD_IPP_CMDLST = 0,
	RD_ORB_Y      = 1,
	RD_RDR_DESC   = 2,
	RD_CMP_REF    = 3,
	RD_VBK_SRC_Y = 4,
	RD_VBK_SRC_UV = 5,
	RD_VBK_SRC_Y_DS16 = 6,
	RD_VBK_SRC_UV_DS16 = 7,
	RD_VBK_SIGMA = 8,
	RD_VBK_CURR_MASK_TF = 9,
	RD_GF_SRC_P = 10,
	RD_GF_GUI_I = 11,
	RD_ANF_CURR_I0 = 12,
	RD_ANF_CURR_Q0 = 13,
	RD_ANF_PREV_I0 = 14,
	RD_ANF_PREV_Q0 = 15,
	RD_ANF_CURR_I1 = 16,
	RD_ANF_CURR_Q1 = 17,
	RD_ANF_PREV_I1 = 18,
	RD_ANF_PREV_Q1 = 19,
	RD_VP_ELEMENT_MAX
};

enum ipp_vp_wr_e {
	WR_IPP_CMDLST = 0,
	WR_ORB_Y      = 1,
	WR_ORB_DESC   = 2,
	WR_VBK_Y = 3,
	WR_VBK_UV = 4,
	WR_VBK_NEXT_MASK_TF = 5,
	WR_GF_LF_A = 6,
	WR_GF_HF_B = 7,
	WR_ANF_Q0 = 8,
	WR_ANF_I0 = 9,
	WR_ANF_Q1 = 10,
	WR_ANF_I1 = 11,
	WR_VP_ELEMENT_MAX
};

enum ipp_non_raster_rd_e {
	RD_CMP_CUR = 0,
	RD_MC_REF_CUR  = 1,
};

enum ipp_non_raster_wr_e {
	WR_RDR_DESC = 0,
};


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
	{ "3PF8",  24, 32, 3,  8 },
	{ "4PF8",  32, 32, 4, 8 },
	{ "1PF10", 10, 16, 1, 10 },
	{ "2PF10", 20, 32, 2, 10 },
	{ "3PF10", 30, 32, 3, 10 },
	{ "4PF10", 40, 64, 4, 10 },
	{ "1PF12", 12, 16, 1, 12 },
	{ "2PF12", 24, 32, 2, 12 },
	{ "3PF12", 36, 64, 3, 12 },
	{ "4PF12", 48, 64, 4, 12 },
	{ "1PF14", 14, 16, 1, 14 },
	{ "2PF14", 28, 32, 2, 14 },
	{ "3PF14", 42, 64, 3, 14 },
	{ "4PF14", 56, 64, 4, 14 },
	{ "0",   8,  8, 1,  8 },
	{ "0",  16, 16, 2,  8 },
	{ "0",  24, 32, 3,  8 },
	{ "0",  32, 32, 4, 8 },
	{ "0", 10, 16, 1, 10 },
	{ "0", 20, 32, 2, 10 },
	{ "0", 30, 32, 3, 10 },
	{ "0", 40, 64, 4, 10 },
	{ "0", 12, 16, 1, 12 },
	{ "0", 24, 32, 2, 12 },
	{ "0", 36, 64, 3, 12 },
	{ "0", 48, 64, 4, 12 },
	{ "D32",   32, 32, 1, 32 },
	{ "D48",   48, 64, 1, 48 },
	{ "D64",   64, 64, 1, 64 },
	{ "D128",  128, 128, 1, 128},
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

