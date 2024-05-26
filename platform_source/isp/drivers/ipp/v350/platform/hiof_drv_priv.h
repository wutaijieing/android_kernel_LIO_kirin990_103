// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  hiof_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/14
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __HIOF_DRV_PRIV_H__
#define __HIOF_DRV_PRIV_H__

/* Define the union U_HIOF_CFG */
union u_hiof_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    spena         : 1  ; /* [0] */
		unsigned int    iter_check_en : 1  ; /* [1] */
		unsigned int    mode          : 2  ; /* [3:2] */
		unsigned int    iter_num      : 4  ; /* [7:4] */
		unsigned int    rsv_0         : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_SIZE_CFG */
union u_size_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    height : 10  ; /* [9:0] */
		unsigned int    rsv_1  : 6  ; /* [15:10] */
		unsigned int    width  : 10  ; /* [25:16] */
		unsigned int    rsv_2  : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_COEFF_CFG */
union u_coeff_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    coeff0 : 9  ; /* [8:0] */
		unsigned int    rsv_3  : 7  ; /* [15:9] */
		unsigned int    coeff1 : 9  ; /* [24:16] */
		unsigned int    rsv_4  : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIOF_THRESHOLD_CFG */
union u_hiof_threshold_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    flat_thr     : 12  ; /* [11:0] */
		unsigned int    ismotion_thr : 4  ; /* [15:12] */
		unsigned int    confilv_thr  : 12  ; /* [27:16] */
		unsigned int    att_max      : 4  ; /* [31:28] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_RO1 */
union u_debug_ro1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_ro1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_RO2 */
union u_debug_ro2 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_ro2 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_RW */
union u_debug_rw {
	/* Define the struct bits */
	struct {
		unsigned int    debug_rw : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


//==============================================================================
/* Define the global struct */
struct s_hiof_regs_type {
	union u_hiof_cfg      hiof_cfg      ; /* 4 */
	union u_size_cfg      size_cfg      ; /* 8 */
	union u_coeff_cfg     coeff_cfg     ; /* c */
	union u_hiof_threshold_cfg threshold_cfg ; /* 10 */
	union u_debug_ro1     debug_ro1     ; /* 20 */
	union u_debug_ro2     debug_ro2     ; /* 24 */
	union u_debug_rw      debug_rw      ; /* 28 */

};

#endif // __HIOF_DRV_PRIV_H__
