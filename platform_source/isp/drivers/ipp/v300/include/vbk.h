

#ifndef __VBK_CS_H__
#define __VBK_CS_H__

#include "ipp.h"
#include "cfg_table_vbk.h"

#define ALIGN_VBK_WIDTH  16
#define VBK_SIGMA2ALPHA_NUM 256

enum vbk_request_mode_e {
	VBK_BOKEH_MODE = 0,
	VBK_FRONT_FACE_MODE = 1,
};

enum vbk_buf_usage_e {
	BI_VBK_SRC_Y_DS16 = 0,
	BI_VBK_SRC_UV_DS16 = 1,
	BI_VBK_SRC_UV = 2,
	BI_VBK_SIGMA = 3,
	BI_VBK_SRC_Y = 4,
	BI_VBK_FOREMASK = 5,
	BO_VBK_RESULT_UV = 6,
	BO_VBK_RESULT_Y = 7,
	BO_VBK_RESULT_F = 8,
	VBK_STREAM_MAX,
};

struct vbk_hw_config_t {
// vbk_ctrl_t
	unsigned int  op_mode;
	unsigned int  y_ds16_gauss_en;
	unsigned int  uv_ds16_gauss_en;
	unsigned int  sigma_gauss_en;
	unsigned int  sigma2alpha_en;
	unsigned int  in_width_ds4;
	unsigned int  in_height_ds4;
// vbk_y_sigma_gauss_t
	unsigned int yds16_gauss_coeff_g00;
	unsigned int yds16_gauss_coeff_g01;
	unsigned int yds16_gauss_coeff_g02;
	unsigned int yds16_gauss_coeff_g03;
	unsigned int yds16_gauss_coeff_g04;
	unsigned int yds16_gauss_coeff_inv;
	unsigned int sigma_gauss_coeff_g00;
	unsigned int sigma_gauss_coeff_g01;
	unsigned int sigma_gauss_coeff_g02;
	unsigned int sigma_gauss_coeff_g03;
	unsigned int sigma_gauss_coeff_g04;
	unsigned int sigma_gauss_coeff_inv;
// vbk_uv_gauss_t
	unsigned int uvds16_gauss_coeff_g00;
	unsigned int uvds16_gauss_coeff_g01;
	unsigned int uvds16_gauss_coeff_g02;
	unsigned int uvds16_gauss_coeff_inv;

	unsigned int addnoise_th;
	unsigned int foremask_coeff;
	unsigned int foremask_th;
	unsigned int foremask_weighted_filter_en;
	unsigned int foremask_dilation_radius;

	unsigned int sigma2alpha_lut[VBK_SIGMA2ALPHA_NUM];

};

struct msg_req_vbk_request_t {
	unsigned int frame_number;
	struct ipp_stream_t streams[VBK_STREAM_MAX];
	struct vbk_hw_config_t vbk_hw_config;
	unsigned int vbk_rate_value;
};

int vbk_request_handler(
	struct msg_req_vbk_request_t *vbk_request);

#endif

