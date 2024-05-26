/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_orb_enh.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#ifndef _ORB_ENH_CS_H__
#define _ORB_ENH_CS_H__

#include "ipp.h"
#include "cfg_table_orb_enh.h"

enum buf_usage_e {
	BI_ENH_YHIST = 0,
	BI_ENH_YIMAGE = 1,
	BO_ENH_OUT = 2,
	ORB_ENH_STREAM_MAX,
};

struct ctrl_cfg_t {
	unsigned int enh_en;
	unsigned int gsblur_en;
};

struct cal_hist_cfg_t {
	unsigned int blknumy;
	unsigned int blknumx;
	unsigned int blk_ysize;
	unsigned int blk_xsize;
	unsigned int edgeex_y;
	unsigned int edgeex_x;
	unsigned int climit;
};

struct adjust_hist_cfg_t {
	unsigned int lutscale;
};

struct adjust_image_cfg_t {
	unsigned int inv_ysize;
	unsigned int inv_xsize;
};

struct gsblur_coef_cfg_t {
	unsigned int coeff_gauss_0;
	unsigned int coeff_gauss_1;
	unsigned int coeff_gauss_2;
	unsigned int coeff_gauss_3;
};

struct enh_reg_cfg_t {
	struct ctrl_cfg_t  ctrl_cfg;
	struct adjust_hist_cfg_t adjust_hist_cfg;
	struct gsblur_coef_cfg_t gsblur_coef_cfg;
	struct cal_hist_cfg_t     cal_hist_cfg;
	struct adjust_image_cfg_t adjust_image_cfg;
};

struct msg_enh_req_t {
	struct ipp_stream_t        streams[ORB_ENH_STREAM_MAX];
	struct enh_reg_cfg_t     hw_config;
	unsigned int    orb_enh_rate_value;
};

void orb_enh_dump_request(struct msg_enh_req_t *req);

int orb_enh_request_handler(
	struct msg_enh_req_t *orb_enh_request);

#endif /* _ORB_ENH_ES_H__ */
