// ***********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  reorder_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **********************************************************

#ifndef __REORDER_DRV_PRIV_H__
#define __REORDER_DRV_PRIV_H__

/* Define the union U_REORDER_CFG */
union U_REORDER_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    reorder_en      : 1  ; /* [0] */
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

/* Define the union U_PREFETCH_CFG */
union U_TOTAL_KPT_NUMBER {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num : 15  ; /* [14:0] */
		unsigned int    rsv_0         : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PREFETCH_CFG */
union U_PREFETCH_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    first_32k_page  : 19  ; /* [18:0] */
		unsigned int    rsv_5           : 1  ; /* [19] */
		unsigned int    prefetch_enable : 1  ; /* [20] */
		unsigned int    rsv_6           : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_0 */
union U_DEBUG_0 {
	/* Define the struct bits  */
	struct {
		unsigned int debug_0                : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_DEBUG_1 */
union U_DEBUG_1 {
	/* Define the struct bits  */
	struct {
		unsigned int debug_1                : 32  ; /* [31..0]  */
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
/* Define the union U_KPT_NUMBER */
union U_KPT_NUMBER {
	/* Define the struct bits  */
	struct {
		unsigned int kpt_num                : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
// ==============================================================================
/* Define the global struct */
struct S_REORDER_REGS_TYPE {
	union U_REORDER_CFG  REORDER_CFG     ; /* 0 */
	union U_BLOCK_CFG    BLOCK_CFG       ; /* 4 */
	union U_TOTAL_KPT_NUMBER    TOTAL_KPT_NUM       ; /* 8 */
	union U_PREFETCH_CFG PREFETCH_CFG    ; /* 10 */
	union U_DEBUG_0      DEBUG_0         ; /* 20 */
	union U_DEBUG_1      DEBUG_1         ; /* 24 */
	union U_EC_0         EC_0            ; /* 28 */
	union U_EC_1         EC_1            ; /* 2C */
	union U_KPT_NUMBER   KPT_NUMBER[187] ; /* 100 */

};

#endif /* __REORDER_DRV_PRIV_H__ */
