// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  mc_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __MC_DRV_PRIV_H__
#define __MC_DRV_PRIV_H__

/* Define the union U_MC_CFG */
union u_mc_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    mc_en           : 1  ; /* [0] */
		unsigned int    cfg_mode        : 1  ; /* [1] */
		unsigned int    h_cal_en        : 1  ; /* [2] */
		unsigned int    push_inliers_en : 1  ; /* [3] */
		unsigned int    rsv_0           : 1  ; /* [4] */
		unsigned int    flag_10_11      : 1  ; /* [5] */
		unsigned int    rsv_1           : 2  ; /* [7:6] */
		unsigned int    max_iterations  : 12  ; /* [19:8] */
		unsigned int    svd_iterations  : 5  ; /* [24:20] */
		unsigned int    rsv_2           : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_THRESHOLD_CFG */
union u_threshold_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    inlier_th : 20  ; /* [19:0] */
		unsigned int    rsv_3     : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_MATCH_POINTS */
union u_match_points {
	/* Define the struct bits */
	struct {
		unsigned int    matched_kpts : 12  ; /* [11:0] */
		unsigned int    rsv_4        : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_INLIER_NUMBER */
union u_inlier_number {
	/* Define the struct bits */
	struct {
		unsigned int    inlier_num : 12  ; /* [11:0] */
		unsigned int    rsv_5      : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_H_MATRIX */
union u_h_matrix {
	/* Define the struct bits */
	struct {
		unsigned int    h_matrix : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_REF_PREFETCH_CFG */
union u_ref_prefetch_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    ref_first_4k_page   : 24  ; /* [23:0] */
		unsigned int    ref_prefetch_enable : 1  ; /* [24] */
		unsigned int    rsv_6               : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_CUR_PREFETCH_CFG */
union u_cur_prefetch_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    cur_first_4k_page   : 24  ; /* [23:0] */
		unsigned int    cur_prefetch_enable : 1  ; /* [24] */
		unsigned int    rsv_7               : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_TABLE_CLEAR_CFG */
union u_table_clear_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    table_clear : 1  ; /* [0] */
		unsigned int    rsv_8       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_INDEX_CFG */
union u_index_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    cfg_cur_index : 12  ; /* [11:0] */
		unsigned int    cfg_ref_index : 12  ; /* [23:12] */
		unsigned int    cfg_best_dist : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_INDEX_PAIRS */
union u_index_pairs {
	/* Define the struct bits */
	struct {
		unsigned int    cur_index : 12  ; /* [11:0] */
		unsigned int    ref_index : 12  ; /* [23:12] */
		unsigned int    best_dist : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_COORDINATE_CFG */
union u_coordinate_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    cfg_coordinate_x : 16  ; /* [15:0] */
		unsigned int    cfg_coordinate_y : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_COORDINATE_PAIRS */
union u_coordinate_pairs {
	/* Define the struct bits */
	struct {
		unsigned int    coordinate_x : 16  ; /* [15:0] */
		unsigned int    coordinate_y : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_DEBUG_0 */
union u_debug_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_DEBUG_1 */
union u_debug_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_EC_0 */
union u_ec_0 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

/* Define the union U_EC_1 */
union u_ec_1 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;
};

// ==============================================================================
/* Define the global struct */
struct s_mc_regs_type {
	union u_mc_cfg           mc_cfg                 ; /* 0 */
	union u_threshold_cfg    threshold_cfg          ; /* 4 */
	union u_match_points     match_points           ; /* 8 */
	union u_inlier_number    inlier_number          ; /* c */
	union u_h_matrix         h_matrix[18]           ; /* 10 */
	union u_ref_prefetch_cfg ref_prefetch_cfg       ; /* 60 */
	union u_cur_prefetch_cfg cur_prefetch_cfg       ; /* 64 */
	union u_table_clear_cfg  table_clear_cfg        ; /* 70 */
	union u_index_cfg        index_cfg[3200]        ; /* 80 */
	union u_index_pairs      index_pairs[3200]      ; /* 90 */
	union u_coordinate_cfg   coordinate_cfg[3200]   ; /* a0 */
	union u_coordinate_pairs coordinate_pairs[3200] ; /* b0 */
	union u_debug_0          debug_0                ; /* c0 */
	union u_debug_1          debug_1                ; /* c4 */
	union u_ec_0             ec_0                   ; /* c8 */
	union u_ec_1             ec_1                   ; /* cc */
};

#endif // __MC_DRV_PRIV_H__
