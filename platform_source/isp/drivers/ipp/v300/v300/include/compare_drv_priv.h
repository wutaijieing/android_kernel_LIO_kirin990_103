// ***********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  compare_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// ***********************************************************

#ifndef __COMPARE_DRV_PRIV_H__
#define __COMPARE_DRV_PRIV_H__

/* Define the union U_COMPARE_CFG */
union U_COMPARE_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    compare_en      : 1  ; /* [0] */
		unsigned int    rsv_0           : 3  ; /* [3:1] */
		unsigned int    descriptor_type : 1  ; /* [4] */
		unsigned int    rsv_1           : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_CFG */
union U_BLOCK_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_num : 5  ; /* [4:0] */
		unsigned int    rsv_2     : 3  ; /* [7:5] */
		unsigned int    blk_h_num : 5  ; /* [12:8] */
		unsigned int    rsv_3     : 3  ; /* [15:13] */
		unsigned int    blk_num   : 10  ; /* [25:16] */
		unsigned int    rsv_4     : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_SEARCH_CFG */
union U_SEARCH_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    v_radius      : 2  ; /* [1:0] */
		unsigned int    rsv_5         : 2  ; /* [3:2] */
		unsigned int    h_radius      : 2  ; /* [5:4] */
		unsigned int    rsv_6         : 2  ; /* [7:6] */
		unsigned int    dis_ratio     : 7  ; /* [14:8] */
		unsigned int    rsv_7         : 1  ; /* [15] */
		unsigned int    dis_threshold : 8  ; /* [23:16] */
		unsigned int    rsv_8         : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_STAT_CFG */
union U_STAT_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    bin_num    : 6  ; /* [5:0] */
		unsigned int    rsv_9      : 2  ; /* [7:6] */
		unsigned int    bin_factor : 8  ; /* [15:8] */
		unsigned int    max3_ratio : 5  ; /* [20:16] */
		unsigned int    rsv_10     : 3  ; /* [23:21] */
		unsigned int    stat_en    : 1  ; /* [24] */
		unsigned int    rsv_11     : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

union U_TOTAL_KPTNUM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num    : 15  ; /* [14:0] */
		unsigned int    rsv_17           : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PREFETCH_CFG */
union U_PREFETCH_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    first_32k_page  : 19  ; /* [18:0] */
		unsigned int    rsv_12          : 1  ; /* [19] */
		unsigned int    prefetch_enable : 1  ; /* [20] */
		unsigned int    rsv_13          : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_OFFSET_CFG */
union U_OFFSET_CFG {
	/* Define the struct bits */
	struct {
		signed int    cenv_offset : 4  ; /* [3:0] */
		signed int    cenh_offset : 4  ; /* [7:4] */
		signed int    rsv_14      : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	signed int    u32;

};

/* Define the union U_DEBUG_0 */
union U_DEBUG_0 {
	/* Define the struct bits  */
	struct {
		unsigned int debug0                 : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_DEBUG_1 */
union U_DEBUG_1 {
	/* Define the struct bits  */
	struct {
		unsigned int debug1                 : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_EC_0 */
union U_EC_0 {
	/* Define the struct bits  */
	struct {
		unsigned int ec_0                   : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_EC_1 */
union U_EC_1 {
	/* Define the struct bits  */
	struct {
		unsigned int ec_1                   : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_REF_KPT_NUMBER */
union U_REF_KPT_NUMBER {
	/* Define the struct bits  */
	struct {
		unsigned int ref_kpt_num            : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_CUR_KPT_NUMBER */
union U_CUR_KPT_NUMBER {
	/* Define the struct bits  */
	struct {
		unsigned int cur_kpt_num            : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_MATCH_POINTS */
union U_MATCH_POINTS {
	/* Define the struct bits */
	struct {
		unsigned int    matched_kpts : 12  ; /* [11:0] */
		unsigned int    rsv_15       : 20  ; /* [31:12] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INDEX */
union U_INDEX {
	/* Define the struct bits */
	struct {
		unsigned int    cur_index : 12  ; /* [11:0] */
		unsigned int    ref_index : 12  ; /* [23:12] */
		unsigned int    best_dist : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

// ==============================================================================
/* Define the global struct */
struct S_COMPARE_REGS_TYPE {
	union U_COMPARE_CFG    COMPARE_CFG         ; /* 0 */
	union U_BLOCK_CFG      BLOCK_CFG           ; /* 4 */
	union U_SEARCH_CFG     SEARCH_CFG          ; /* 8 */
	union U_STAT_CFG       STAT_CFG            ; /* C */
	union U_PREFETCH_CFG   PREFETCH_CFG        ; /* 10 */
	union U_OFFSET_CFG     OFFSET_CFG          ; /* 14 */
	union U_TOTAL_KPTNUM_CFG  TOTAL_KPTNUM_CFG   ; /* 18 */
	union U_DEBUG_0        DEBUG_0             ; /* 20 */
	union U_DEBUG_1        DEBUG_1             ; /* 24 */
	union U_EC_0           EC_0                ; /* 28 */
	union U_EC_1           EC_1                ; /* 2C */
	union U_REF_KPT_NUMBER REF_KPT_NUMBER[187] ; /* 100 */
	union U_CUR_KPT_NUMBER CUR_KPT_NUMBER[187] ; /* 400 */
	union U_MATCH_POINTS   MATCH_POINTS        ; /* 6F0 */
	union U_INDEX          INDEX[3200]         ; /* 700 */

};

#endif /* __COMPARE_DRV_PRIV_H__ */
