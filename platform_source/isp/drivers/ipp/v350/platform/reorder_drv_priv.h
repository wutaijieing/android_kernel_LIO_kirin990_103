// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  slam_reorder_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __SLAM_REORDER_DRV_PRIV_H__
#define __SLAM_REORDER_DRV_PRIV_H__

/* Define the union U_REORDER_CFG */
union u_reorder_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    reorder_en : 1  ; /* [0] */
		unsigned int    rsv_0      : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_CFG */
union u_reorder_block_cfg {
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

/* Define the union U_TOTAL_KPT_NUM */
union u_reorder_total_kpt_num {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num : 15  ; /* [14:0] */
		unsigned int    rsv_4         : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PREFETCH_CFG */
union u_reorder_prefetch_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    first_4k_page   : 24  ; /* [23:0] */
		unsigned int    prefetch_enable : 1  ; /* [24] */
		unsigned int    rsv_5           : 7  ; /* [31:25] */
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

/* Define the union U_KPT_NUMBER */
union u_kpt_number {
	/* Define the struct bits */
	struct {
		unsigned int    kpt_num : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


//==============================================================================
/* Define the global struct */
struct s_slam_reorder_regs_type {
	union u_reorder_cfg   reorder_cfg    ; /* 0 */
	union u_reorder_block_cfg     block_cfg      ; /* 4 */
	union u_reorder_total_kpt_num total_kpt_num  ; /* 8 */
	union u_reorder_prefetch_cfg  prefetch_cfg   ; /* 10 */
	union u_debug_0       debug_0        ; /* 20 */
	union u_debug_1       debug_1        ; /* 24 */
	union u_ec_0          ec_0           ; /* 28 */
	union u_ec_1          ec_1           ; /* 2c */
	union u_kpt_number    kpt_number[94] ; /* 100 */

};

#endif // __SLAM_REORDER_DRV_PRIV_H__
