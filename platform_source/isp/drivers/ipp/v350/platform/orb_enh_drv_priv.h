// **********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  orb_enh_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// ***********************************************************

#ifndef __ORB_ENH_DRV_PRIV_H__
#define __ORB_ENH_DRV_PRIV_H__

/* Define the union U_ENABLE_CFG */
union u_enable_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    enh_en      : 1  ; /* [0] */
		unsigned int    gsblur_en   : 1  ; /* [1] */
		unsigned int    rsv_0       : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLK_NUM */
union u_blk_num {
	/* Define the struct bits */
	struct {
		unsigned int    blknumx : 4  ; /* [3:0] */
		unsigned int    blknumy : 4  ; /* [7:4] */
		unsigned int    rsv_1   : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLK_SIZE */
union u_blk_size {
	/* Define the struct bits */
	struct {
		unsigned int    blk_xsize : 8  ; /* [7:0] */
		unsigned int    blk_ysize : 8  ; /* [15:8] */
		unsigned int    rsv_2     : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INV_BLK_SIZE_Y */
union u_inv_blk_size_y {
	/* Define the struct bits */
	struct {
		unsigned int    inv_ysize : 17  ; /* [16:0] */
		unsigned int    rsv_3     : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INV_BLK_SIZE_X */
union u_inv_blk_size_x {
	/* Define the struct bits */
	struct {
		unsigned int    inv_xsize : 17  ; /* [16:0] */
		unsigned int    rsv_4     : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_EXTEND_CFG */
union u_extend_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    edgeex_x : 11  ; /* [10:0] */
		unsigned int    rsv_5    : 5  ; /* [15:11] */
		unsigned int    edgeex_y : 11  ; /* [26:16] */
		unsigned int    rsv_6    : 5  ; /* [31:27] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_HIST_CLIMIT */
union u_hist_climit {
	/* Define the struct bits */
	struct {
		unsigned int    climit : 11  ; /* [10:0] */
		unsigned int    rsv_7  : 21  ; /* [31:11] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_LUT_SCALE */
union u_lut_scale {
	/* Define the struct bits */
	struct {
		unsigned int    lutscale : 18  ; /* [17:0] */
		unsigned int    rsv_8    : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_COEF_01 */
union u_gsblur_coef_01 {
	/* Define the struct bits */
	struct {
		unsigned int    gauss_coeff_1 : 10  ; /* [9:0] */
		unsigned int    rsv_9         : 6  ; /* [15:10] */
		unsigned int    gauss_coeff_0 : 10  ; /* [25:16] */
		unsigned int    rsv_10        : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_COEF_23 */
union u_gsblur_coef_23 {
	/* Define the struct bits */
	struct {
		unsigned int    gauss_coeff_3 : 10  ; /* [9:0] */
		unsigned int    rsv_11        : 6  ; /* [15:10] */
		unsigned int    gauss_coeff_2 : 10  ; /* [25:16] */
		unsigned int    rsv_12        : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct s_orb_enh_regs_type {
	union u_enable_cfg     enable_cfg     ; /* 0 */
	union u_blk_num        blk_num        ; /* 8 */
	union u_blk_size       blk_size       ; /* 10 */
	union u_inv_blk_size_y inv_blk_size_y ; /* 14 */
	union u_inv_blk_size_x inv_blk_size_x ; /* 18 */
	union u_extend_cfg     extend_cfg     ; /* 1c */
	union u_hist_climit    hist_climit    ; /* 20 */
	union u_lut_scale      lut_scale      ; /* 24 */
	union u_gsblur_coef_01 gsblur_coef_01 ; /* 30 */
	union u_gsblur_coef_23 gsblur_coef_23 ; /* 34 */

};

#endif // __ORB_ENH_DRV_PRIV_H__

