// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  gf_reg_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/4/11
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __GF_REG_DRV_PRIV_H__
#define __GF_REG_DRV_PRIV_H__

/* Define the union U_GF_IMAGE_SIZE */
union u_gf_image_size {
	/* Define the struct bits */
	struct {
		unsigned int    width  : 9  ; /* [8:0] */
		unsigned int    rsv_0  : 7  ; /* [15:9] */
		unsigned int    height : 13  ; /* [28:16] */
		unsigned int    rsv_1  : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_MODE */
union u_gf_mode {
	/* Define the struct bits */
	struct {
		unsigned int    mode_in  : 1  ; /* [0] */
		unsigned int    mode_out : 2  ; /* [2:1] */
		unsigned int    rsv_2    : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_FILTER_COEFF */
union u_gf_filter_coeff {
	/* Define the struct bits */
	struct {
		unsigned int    radius : 4  ; /* [3:0] */
		unsigned int    eps    : 26  ; /* [29:4] */
		unsigned int    rsv_3  : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_CROP_H */
union u_gf_crop_h {
	/* Define the struct bits */
	struct {
		unsigned int    crop_left  : 9  ; /* [8:0] */
		unsigned int    rsv_4      : 7  ; /* [15:9] */
		unsigned int    crop_right : 9  ; /* [24:16] */
		unsigned int    rsv_5      : 7  ; /* [31:25] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_CROP_V */
union u_gf_crop_v {
	/* Define the struct bits */
	struct {
		unsigned int    crop_top    : 13  ; /* [12:0] */
		unsigned int    rsv_6       : 3  ; /* [15:13] */
		unsigned int    crop_bottom : 13  ; /* [28:16] */
		unsigned int    rsv_7       : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_DEBUG_0 */
union u_gf_debug_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_DEBUG_1 */
union u_gf_debug_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_EC_0 */
union u_gf_ec_0 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_EC_1 */
union u_gf_ec_1 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


//==============================================================================
/* Define the global struct */
struct s_gf_reg_regs_type {
	union u_gf_image_size   gf_image_size   ; /* 0 */
	union u_gf_mode         gf_mode         ; /* 4 */
	union u_gf_filter_coeff gf_filter_coeff ; /* 8 */
	union u_gf_crop_h       gf_crop_h       ; /* c */
	union u_gf_crop_v       gf_crop_v       ; /* 10 */
	union u_gf_debug_0      gf_debug_0      ; /* 14 */
	union u_gf_debug_1      gf_debug_1      ; /* 18 */
	union u_gf_ec_0         gf_ec_0         ; /* 1c */
	union u_gf_ec_1         gf_ec_1         ; /* 20 */

};

#endif // __GF_REG_DRV_PRIV_H__
