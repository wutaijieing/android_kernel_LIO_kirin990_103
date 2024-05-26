// ******************************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_cvdr.h
// Description:
//
// Date         2017-09-30 16:42:26
// ******************************************************************

#ifndef __CONFIG_TABLE_CVDR_CS_H_INCLUDED__
#define __CONFIG_TABLE_CVDR_CS_H_INCLUDED__

#include "cvdr.h"

enum cvdr_prefetch_mode_e {
	OLD_CVDR_PREFETCH_EN,
	OLD_CVDR_PREFETCH_BYPASS,
};

enum pix_type_e {
	STD_PIX = 0,
	EXP_PIX = 1,
}; // expanded or not

enum cvdr_pix_fmt_e {
	DF_1PF8  = 0,
	DF_2PF8  = 1,
	DF_1PF10 = 4,
	DF_2PF14 = 13,
	DF_3PF8  = 25,
	DF_D32   = 28,
	DF_D48   = 29,
	DF_D64   = 30,
	DF_D128  = 31,
	DF_FMT_INVALID,
};

struct cvdr_bw_cfg_t {
	unsigned char       bw_limiter0;
	unsigned char       bw_limiter1;
	unsigned char       bw_limiter2;
	unsigned char       bw_limiter3;
	unsigned char       bw_limiter_reload;
};

struct cvdr_wr_fmt_desc_t {
	unsigned int        fs_addr;
	unsigned int        last_page;

	enum cvdr_pix_fmt_e  pix_fmt;
	unsigned char       pix_expan;
	unsigned short      line_stride;
	unsigned short      line_wrap;
	unsigned short      line_shart_wstrb;
	unsigned short      mono_mode;
};

struct cvdr_rd_fmt_desc_t {
	unsigned int        fs_addr;
	unsigned int        last_page;
	enum cvdr_pix_fmt_e      pix_fmt;
	unsigned char       pix_expan;
	unsigned short      allocated_du;
	unsigned short      line_size;
	unsigned short      hblank;
	unsigned short      frame_size;
	unsigned short      vblank;
	unsigned short      line_stride;
	unsigned short      line_wrap;
};

struct cvdr_vp_wr_cfg_t {
	unsigned char           to_use;
	unsigned char           id;
	struct cvdr_wr_fmt_desc_t  fmt;
	struct cvdr_bw_cfg_t  bw;
};

struct cvdr_vp_rd_cfg_t {
	unsigned char  to_use;
	unsigned char  id;
	struct cvdr_rd_fmt_desc_t fmt;
	struct cvdr_bw_cfg_t  bw;
};

struct cvdr_nr_wr_cfg_t {
	unsigned char       to_use;
	unsigned char       nr_wr_stop_en_du_thr;
	unsigned char       nr_wr_stop_en_flux_ctrl;
	unsigned char       nr_wr_stop_en_pressure;
	unsigned char       nr_wr_stop_ok;
	unsigned char       nr_wr_stop;
	unsigned char       en;
	struct cvdr_bw_cfg_t  bw;
};

struct cvdr_nr_rd_cfg_t {
	unsigned char       to_use;
	unsigned short      allocated_du;
	unsigned char       nr_rd_stop_en_du_thr;
	unsigned char       nr_rd_stop_en_flux_ctrl;
	unsigned char       nr_rd_stop_en_pressure;
	unsigned char       nr_rd_stop_ok;
	unsigned char       nr_rd_stop;
	unsigned char       en;
	struct cvdr_bw_cfg_t  bw;
};

struct cvdr_global_config_t {
	unsigned char   to_use;

	unsigned short   axi_wr_du_thr;
	unsigned short   du_thr_rch;
	unsigned short   max_axi_rd_id;
	unsigned short   max_axi_wr_id;
};

struct cvdr_smmu_cfg_t {
	unsigned char   to_use;

	unsigned int   num;
	unsigned int   smr_nscfg;
};

struct cfg_tab_cvdr_t {
	struct cvdr_vp_wr_cfg_t vp_wr_cfg[IPP_VP_WR_MAX];
	struct cvdr_vp_rd_cfg_t vp_rd_cfg[IPP_VP_RD_MAX];
	struct cvdr_nr_wr_cfg_t nr_wr_cfg[IPP_NR_WR_MAX];
	struct cvdr_nr_rd_cfg_t nr_rd_cfg[IPP_NR_RD_MAX];

	unsigned int    rdr_cvdr_frame_size_addr; // address in cmdlst_buffer
	unsigned int    cmp_cvdr_frame_size_addr; // address in cmdlst_buffer
	unsigned int    arf_cvdr_wr_addr; // address in cmdlst_buffer
};

#endif /* __CONFIG_TABLE_CVDR_CS_H_INCLUDED__ */

/* ********************************* END ********************************* */
