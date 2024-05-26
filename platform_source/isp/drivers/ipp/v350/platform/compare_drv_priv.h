// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  slam_compare_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __SLAM_COMPARE_DRV_PRIV_H__
#define __SLAM_COMPARE_DRV_PRIV_H__

/* Define the union U_COMPARE_CFG */
union u_compare_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    compare_en : 1  ; /* [0] */
		unsigned int    rsv_0      : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_CFG */
union u_compare_block_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_num : 5  ; /* [4:0] */
		unsigned int    rsv_1     : 3  ; /* [7:5] */
		unsigned int    blk_h_num : 5  ; /* [12:8] */
		unsigned int    rsv_2     : 3  ; /* [15:13] */
		unsigned int    blk_num   : 10  ; /* [25:16] */
		unsigned int    rsv_3     : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_SEARCH_CFG */
union u_search_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    v_radius      : 2  ; /* [1:0] */
		unsigned int    h_radius      : 2  ; /* [3:2] */
		unsigned int    dis_ratio     : 13  ; /* [16:4] */
		unsigned int    dis_threshold : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_STAT_CFG */
union u_stat_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    bin_num    : 6  ; /* [5:0] */
		unsigned int    rsv_4      : 2  ; /* [7:6] */
		unsigned int    bin_factor : 8  ; /* [15:8] */
		unsigned int    max3_ratio : 5  ; /* [20:16] */
		unsigned int    rsv_5      : 3  ; /* [23:21] */
		unsigned int    stat_en    : 1  ; /* [24] */
		unsigned int    rsv_6      : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PREFETCH_CFG */
union u_compare_prefetch_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    first_4k_page   : 24  ; /* [23:0] */
		unsigned int    prefetch_enable : 1  ; /* [24] */
		unsigned int    rsv_7           : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_OFFSET_CFG */
union u_offset_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    cenv_offset : 4  ; /* [3:0] */
		unsigned int    cenh_offset : 4  ; /* [7:4] */
		unsigned int    rsv_8       : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_TOTAL_KPT_NUM */
union u_compare_total_kpt_num {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num : 15  ; /* [14:0] */
		unsigned int    rsv_9         : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_0 */
union u_debug_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_1 */
union u_debug_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug1 : 32  ; /* [31:0] */
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

/* Define the union U_REF_KPT_NUMBER */
union u_ref_kpt_number {
	/* Define the struct bits */
	struct {
		unsigned int    ref_kpt_num : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CUR_KPT_NUMBER */
union u_cur_kpt_number {
	/* Define the struct bits */
	struct {
		unsigned int    cur_kpt_num : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_MATCH_POINTS */
union u_match_points {
	/* Define the struct bits */
	struct {
		unsigned int    matched_kpts : 12  ; /* [11:0] */
		unsigned int    rsv_10       : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INDEX */
union u_index {
	/* Define the struct bits */
	struct {
		unsigned int    cur_index : 12  ; /* [11:0] */
		unsigned int    ref_index : 12  ; /* [23:12] */
		unsigned int    rsv_11    : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DISTANCE */
union u_distance {
	/* Define the struct bits */
	struct {
		unsigned int    distance : 15  ; /* [14:0] */
		unsigned int    rsv_12   : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

// ==============================================================================
/* define the global struct */
struct s_slam_compare_regs_type {
	union u_compare_cfg    compare_cfg        ; /* 0 */
	union u_compare_block_cfg      block_cfg          ; /* 4 */
	union u_search_cfg     search_cfg         ; /* 8 */
	union u_stat_cfg       stat_cfg           ; /* c */
	union u_compare_prefetch_cfg   prefetch_cfg       ; /* 10 */
	union u_offset_cfg     offset_cfg         ; /* 14 */
	union u_compare_total_kpt_num  total_kpt_num      ; /* 18 */
	union u_debug_0        debug_0            ; /* 20 */
	union u_debug_1        debug_1            ; /* 24 */
	union u_ec_0           ec_0               ; /* 28 */
	union u_ec_1           ec_1               ; /* 2c */
	union u_ref_kpt_number ref_kpt_number[94] ; /* 100 */
	union u_cur_kpt_number cur_kpt_number[94] ; /* 300 */
	union u_match_points   match_points       ; /* 4f0 */
	union u_index          index[3200]        ; /* 500 */
	union u_distance       distance[3200]     ; /* 504 */

};

#endif // __SLAM_COMPARE_DRV_PRIV_H__
