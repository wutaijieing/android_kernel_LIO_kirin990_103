/*******************************************************************
 * Copyright    Copyright (c) 2014- Hisilicon Technologies CO., Ltd.
 * File name    segment_arfeature.h
 * Description:
 *
 * Date        2020-04-23
 ******************************************************************/

#ifndef _ARFEATURE_CS_H
#define _ARFEATURE_CS_H

#include "ipp.h"
#include "segment_common.h"

#define MAX_BLK_H_NUM   (22)
#define MAX_BLK_V_NUM   (17)
#define GRID_NUM_RANG   (MAX_BLK_H_NUM*MAX_BLK_V_NUM) // 374
#define GRID_NUM_RANG_DS4   (94)

#define ARFEARURE_DESCRIPTOR_SIZE (align_up((144*4095), CVDR_ALIGN_BYTES))
#define ARFEARURE_MINSCR_KPTCNT_SIZE  (align_up((GRID_NUM_RANG*4), CVDR_ALIGN_BYTES)) // minscore_kptcount_buf
#define ARFEARURE_SUM_SCORE_SIZE  (align_up((GRID_NUM_RANG*4), CVDR_ALIGN_BYTES)) // sum_score_buf
#define ARFEARURE_KPT_NUM_SIZE  (align_up((GRID_NUM_RANG_DS4*4), CVDR_ALIGN_BYTES)) // kpt_num_buf
#define ARFEARURE_TOTAL_KPT_NUM_SIZE    (align_up((1*4), CVDR_ALIGN_BYTES))

/* AR case:mode = 1,2,2,3,4,4,4,4,4,4; total mode num is 10. ) DMAP case:mode = 0,3,4; total mode num is 3.) */
#define ARFEATURE_TOTAL_MODE_MAX    (10)

typedef enum _arfeature_buf_usage_e {
	BI_ARFEATURE_0 = 0,
	BI_ARFEATURE_1,
	BI_ARFEATURE_2,
	BI_ARFEATURE_3,
	BO_ARFEATURE_PYR_1,
	BO_ARFEATURE_PYR_2,
	BO_ARFEATURE_DOG_0,
	BO_ARFEATURE_DOG_1,
	BO_ARFEATURE_DOG_2,
	BO_ARFEATURE_DOG_3,
	ARFEATURE_STREAM_MAX, // 10
} arfeature_buf_usage_e;

typedef struct _arfeature_req_ctrl_cfg_t {
	unsigned int   mode;
	unsigned int   orient_en;
	unsigned int   layer;
	unsigned int   iter_num;
	unsigned int   first_detect;
} arfeature_req_ctrl_cfg_t;

typedef struct _arfeature_req_size_cfg_t {
	unsigned int   width;
	unsigned int   height;
} arfeature_req_size_cfg_t;

typedef struct _arfeature_req_blk_cfg_t {
	unsigned int   blk_h_num;
	unsigned int   blk_v_num;
	unsigned int   blk_h_size_inv;
	unsigned int   blk_v_size_inv;
} arfeature_req_blk_cfg_t;

typedef struct _arfeature_req_detect_num_lmt_t {
	unsigned int   cur_kpnum;
	unsigned int   max_kpnum;
} arfeature_req_detect_num_lmt_t;

typedef struct _arfeature_req_sigma_coef_cfg_t {
	unsigned int   sigma_ori;
	unsigned int   sigma_des;
} arfeature_req_sigma_coef_cfg_t;

typedef struct _arfeature_req_gauss_coef_cfg_t {
	unsigned int   gauss_org_0; // GAUSS_ORG
	unsigned int   gauss_org_1;
	unsigned int   gauss_org_2;
	unsigned int   gauss_org_3;

	unsigned int   gsblur_1st_0; // GSBLUR_1ST
	unsigned int   gsblur_1st_1;
	unsigned int   gsblur_1st_2;

	unsigned int   gauss_2nd_0; // GAUSS_2ND
	unsigned int   gauss_2nd_1;
	unsigned int   gauss_2nd_2;
	unsigned int   gauss_2nd_3;
} arfeature_req_gauss_coef_cfg_t;

typedef struct _arfeature_req_dog_edge_thd_t {
	unsigned int   dog_threshold;
	unsigned int   edge_threshold;
} arfeature_req_dog_edge_thd_t;

typedef struct _arfeature_req_descriptor_thd_t {
	unsigned int   mag_threshold;
} arfeature_req_descriptor_thd_t;

typedef struct _arfeature_req_kpt_lmt_t {
	unsigned int   grid_max_kpnum[GRID_NUM_RANG];
	unsigned int   grid_dog_threshold[GRID_NUM_RANG];
} arfeature_req_kpt_lmt_t;

typedef struct _arfeature_reg_cfg_t {
	arfeature_req_ctrl_cfg_t	ctrl;
	arfeature_req_size_cfg_t	size_cfg;
	arfeature_req_blk_cfg_t	blk_cfg;
	arfeature_req_detect_num_lmt_t	detect_num_lmt;
	arfeature_req_sigma_coef_cfg_t	sigma_coef;
	arfeature_req_gauss_coef_cfg_t	gauss_coef;
	arfeature_req_dog_edge_thd_t	dog_edge_thd;
	arfeature_req_descriptor_thd_t	req_descriptor_thd;
	arfeature_req_kpt_lmt_t	kpt_lmt;
} arfeature_reg_cfg_t;

typedef struct _req_arfeature_t {
	struct ipp_stream_t	streams[ARFEATURE_TOTAL_MODE_MAX][ARFEATURE_STREAM_MAX];
	arfeature_reg_cfg_t	reg_cfg[ARFEATURE_TOTAL_MODE_MAX];
	unsigned int    arfeature_stat_buff[STAT_BUFF_MAX];
} req_arfeature_t;


typedef struct _seg_arf_stripe_num_t {
	unsigned int detect_total_stripe;
	unsigned int gauss_total_stripe;
	unsigned int layer_stripe_num; // stripes num per Detect Layer;
} seg_arf_stripe_num_t;

struct msg_req_arfeature_request_t {
	unsigned int    frame_num;
	unsigned int    mode_cnt; // same as v300_ORB's pyramid layer
	unsigned int    secure_flag;
	unsigned int    work_mode; // 0 : single stripe.  1:multi stripe
	unsigned int    arfeature_rate_value;
	req_arfeature_t    req_arfeature;
};

int arfeature_request_handler(struct msg_req_arfeature_request_t *msg);
#endif
