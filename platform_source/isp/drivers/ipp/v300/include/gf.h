#ifndef _GF_CS_H__
#define _GF_CS_H__

#include "ipp.h"
#include "cfg_table_gf.h"

enum gf_buf_usage_e {
	BI_GF_GUI_I = 0,
	BI_GF_SRC_P = 1,
	BO_GF_LF_A = 2,
	BO_GF_HF_B = 3,
	GF_STREAM_MAX,
};

struct mode_cfg_t {
	unsigned int mode_in;
	unsigned int mode_out;
};

struct filter_coeff_cfg_t {
	unsigned int radius;
	unsigned int eps;
};

struct gf_hw_config_t {
	struct mode_cfg_t     mode_cfg;
	struct filter_coeff_cfg_t coeff_cfg;
};

struct msg_req_gf_request_t {
	unsigned int  frame_number;
	struct ipp_stream_t streams[GF_STREAM_MAX];
	struct gf_hw_config_t gf_hw_cfg;
	unsigned int gf_rate_value;
};

enum y_gf_in_mode_e {
	SINGLE_INPUT = 0,
	DUAL_INPUT = 1,
};

enum y_gf_out_mode_e {
	LF_ONLY = 0,
	A_B = 1,
	LF_HF = 2,
};

enum mcf_buf_usage_e {
	BI_MONO_DS4 = 0,
	BI_WARP_0_UVC,
	BI_WARP_1_UVC,
	BI_WARP_2_UVC,
	BI_MONO,
	BI_WARP_COL_Y_DS4,
	BO_RESULT_UV,
	BO_RESULT_C,
	BO_RESULT_Y,
	MCF_STREAM_MAX,
};

enum mcf_mode_e {
	MCF_FULL_MODE = 0,
	MCF_PARTIAL_MODE = 1,
	MCF_Y_GF_MODE = 2,
};

struct mcf_config_t {
	unsigned int  uv_pack;
	enum mcf_mode_e mode;
};

struct get_can_t {
	unsigned int    th_u;
	unsigned int    th_v;
	unsigned int    const_value;
	unsigned int    input_cnt;
};

struct get_final_uv_t {
	unsigned int    ratio_dist_uv_to_conf;
	unsigned int    ratio_dist_to_conf;
	unsigned int    max_u_diff_const_inv;
	unsigned int    max_v_diff_const_inv;
};

struct get_con_t {
	unsigned int    hybrid_diff_hue_shift;
	unsigned int    hybrid_diff_hue_coff;
	unsigned int    hybrid_diff_hue_shift_ex;
	unsigned int    max_diff;
	unsigned int    max_diff_inv;
};

struct fill_conflict_cfg_t {
	unsigned int    luma_th;
	unsigned int    conflict_th;
};

struct smooth_blend_cfg_t {
	unsigned int    gauss_radius;
	unsigned int    mask_blend;
};

struct smooth_blend_gf_cfg_t{
	unsigned int    uv_gf_radius;
	unsigned int    uv_gf_eps;
};

struct adjust_col_gf_t {
	unsigned int    y_gf_radius;
	unsigned int    y_gf_eps;
};

struct adjust_get_mask_t {
	unsigned int    mask_sub;
	unsigned int    mask_mult;
};

struct adjust_get_ratio_t {
	unsigned int    ratio_par_down;
	unsigned int    ratio_par_up;
};

struct uv_fusion_t {
	struct get_can_t      get_can;
	struct get_final_uv_t get_final_uv;
	struct get_con_t      get_con;
	struct fill_conflict_cfg_t fill_conflict_cfg;
};

struct mcf_uv_t {
	struct smooth_blend_cfg_t  smooth_blend_cfg;
	struct smooth_blend_gf_cfg_t smooth_blend_gf;
	struct adjust_get_mask_t adjust_get_mask;
	struct adjust_get_ratio_t adjust_get_ratio;
};

struct adjust_y_t {
	struct adjust_col_gf_t   adjust_col_gf;
};

struct full_mode_t {
	struct uv_fusion_t    uv_fusion;
	struct mcf_uv_t mcf_uv_cfg;
	struct adjust_y_t     adjust_y;
};

struct partial_mode_t {
	struct mcf_uv_t mcf_uv_cfg;
	struct adjust_y_t adjust_y;
};

struct y_gf_mode_t {
	enum y_gf_in_mode_e input_mode;
	enum y_gf_out_mode_e output_mode;
	struct adjust_y_t     adjust_y;
};

union mcf_u {
	struct full_mode_t full_mode_param;
	struct partial_mode_t partial_mode_param;
	struct y_gf_mode_t y_lf_gf_mode_param;
};

struct msg_req_mcf_request_t {
	unsigned int  frame_number;
	struct mcf_config_t mcf_cfg;
	struct ipp_stream_t streams[MCF_STREAM_MAX];
	union mcf_u  u;
	unsigned int mcf_rate_value;
};

int gf_request_handler(struct msg_req_mcf_request_t *gf_request);

#endif /* _GF_CS_H__ */
