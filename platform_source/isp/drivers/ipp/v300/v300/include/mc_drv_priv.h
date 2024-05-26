// **********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  mc_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2018/12/28
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **********************************************************

#ifndef __MC_DRV_PRIV_H__
#define __MC_DRV_PRIV_H__

/* Define the union U_MC_CFG */
union U_MC_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    mc_en           : 1  ; /* [0] */
		unsigned int    cfg_mode        : 1  ; /* [1] */
		unsigned int    h_cal_en        : 1  ; /* [2] */
		unsigned int    push_inliers_en : 1  ; /* [3] */
		unsigned int    bf_mode         : 1  ; /* [4] */
		unsigned int    flag_10_11      : 1  ; /* [5] */
		unsigned int    rsv_0           : 2  ; /* [7:6] */
		unsigned int    max_iterations  : 12  ; /* [19:8] */
		unsigned int    svd_iterations  : 5  ; /* [24:20] */
		unsigned int    rsv_1           : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_THRESHOLD_CFG */
union U_THRESHOLD_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    inlier_th : 20  ; /* [19:0] */
		unsigned int    rsv_2     : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_MATCH_POINTS */
union U_MATCH_POINTS {
	/* Define the struct bits */
	struct {
		unsigned int    matched_kpts : 12  ; /* [11:0] */
		unsigned int    rsv_3        : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INLIER_NUMBER */
union U_INLIER_NUMBER {
	/* Define the struct bits */
	struct {
		unsigned int    inlier_num : 12  ; /* [11:0] */
		unsigned int    rsv_4      : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_H_MATRIX */
union U_H_MATRIX {
	/* Define the struct bits */
	struct {
		unsigned int    h_matrix : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_REF_PREFETCH_CFG */
union U_REF_PREFETCH_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    ref_first_32k_page  : 19  ; /* [18:0] */
		unsigned int    rsv_5               : 1  ; /* [19] */
		unsigned int    ref_prefetch_enable : 1  ; /* [20] */
		unsigned int    rsv_6               : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CUR_PREFETCH_CFG */
union U_CUR_PREFETCH_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    cur_first_32k_page  : 19  ; /* [18:0] */
		unsigned int    rsv_7               : 1  ; /* [19] */
		unsigned int    cur_prefetch_enable : 1  ; /* [20] */
		unsigned int    rsv_8               : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_TABLE_CLEAR_CFG */
union U_TABLE_CLEAR_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    table_clear : 1  ; /* [0] */
		unsigned int    rsv_9       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INDEX_CFG */
union U_INDEX_CFG {
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
union U_INDEX_PAIRS {
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
union U_COORDINATE_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    cfg_coordinate_x : 16  ; /* [15:0] */
		unsigned int    cfg_coordinate_y : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_COORDINATE_PAIRS */
union U_COORDINATE_PAIRS {
	/* Define the struct bits */
	struct {
		unsigned int    coordinate_x : 16  ; /* [15:0] */
		unsigned int    coordinate_y : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_0 */
union U_DEBUG_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_1 */
union U_DEBUG_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_EC_0 */
union U_EC_0 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_EC_1 */
union U_EC_1 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct S_MC_REGS_TYPE {
	union U_MC_CFG           MC_CFG                 ; /* 0 */
	union U_THRESHOLD_CFG    THRESHOLD_CFG          ; /* 4 */
	union U_MATCH_POINTS     MATCH_POINTS           ; /* 8 */
	union U_INLIER_NUMBER    INLIER_NUMBER          ; /* C */
	union U_H_MATRIX         H_MATRIX[18]           ; /* 10 */
	union U_REF_PREFETCH_CFG REF_PREFETCH_CFG       ; /* 60 */
	union U_CUR_PREFETCH_CFG CUR_PREFETCH_CFG       ; /* 64 */
	union U_TABLE_CLEAR_CFG  TABLE_CLEAR_CFG        ; /* 70 */
	union U_INDEX_CFG        INDEX_CFG[3200]        ; /* 80 */
	union U_INDEX_PAIRS      INDEX_PAIRS[3200]      ; /* 90 */
	union U_COORDINATE_CFG   COORDINATE_CFG[3200]   ; /* A0 */
	union U_COORDINATE_PAIRS COORDINATE_PAIRS[3200] ; /* B0 */
	union U_DEBUG_0          DEBUG_0                ; /* C0 */
	union U_DEBUG_1          DEBUG_1                ; /* C4 */
	union U_EC_0             EC_0                   ; /* C8 */
	union U_EC_1             EC_1                   ; /* CC */

};

#endif // __MC_DRV_PRIV_H__
