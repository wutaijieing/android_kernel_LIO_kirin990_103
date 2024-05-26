// ***********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  gf_reg_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **************************************************** ******

#ifndef __GF_DRV_PRIV_H__
#define __GF_DRV_PRIV_H__

/* Define the union U_GF_IMAGE_SIZE */
union U_GF_IMAGE_SIZE {
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
union U_GF_MODE {
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
union U_GF_FILTER_COEFF {
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
union U_GF_CROP_H {
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
union U_GF_CROP_V {
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
union U_GF_DEBUG_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_DEBUG_1 */
union U_GF_DEBUG_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_EC_0 */
union U_GF_EC_0 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_EC_1 */
union U_GF_EC_1 {
	/* Define the struct bits */
	struct {
		unsigned int    ec_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct S_GF_REG_REGS_TYPE {
	union U_GF_IMAGE_SIZE   GF_IMAGE_SIZE   ; /* 0 */
	union U_GF_MODE         GF_MODE         ; /* 4 */
	union U_GF_FILTER_COEFF GF_FILTER_COEFF ; /* 8 */
	union U_GF_CROP_H       GF_CROP_H       ; /* C */
	union U_GF_CROP_V       GF_CROP_V       ; /* 10 */
	union U_GF_DEBUG_0      GF_DEBUG_0      ; /* 14 */
	union U_GF_DEBUG_1      GF_DEBUG_1      ; /* 18 */
	union U_GF_EC_0         GF_EC_0         ; /* 1C */
	union U_GF_EC_1         GF_EC_1         ; /* 20 */

};

#endif // __GF_REG_DRV_PRIV_H__
