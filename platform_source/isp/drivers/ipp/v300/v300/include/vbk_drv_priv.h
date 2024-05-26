// *********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  vbk_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **********************************************************

#ifndef __VBK_DRV_PRIV_H__
#define __VBK_DRV_PRIV_H__

/* Define the union U_VBK_CFG */
union U_VBK_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    op_mode          : 1  ; /* [0] */
		unsigned int    y_ds16_gauss_en  : 1  ; /* [1] */
		unsigned int    uv_ds16_gauss_en : 1  ; /* [2] */
		unsigned int    sigma_gauss_en   : 1  ; /* [3] */
		unsigned int    sigma2alpha_en   : 1  ; /* [4] */
		unsigned int    in_width_ds4     : 10  ; /* [14:5] */
		unsigned int    rsv_0            : 1  ; /* [15] */
		unsigned int    in_height_ds4    : 10  ; /* [25:16] */
		unsigned int    rsv_1            : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_Y_DS16_GAUSS_COEFF0_CFG */
union U_VBK_Y_DS16_GAUSS_COEFF0_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    y_ds16_g00 : 5  ; /* [4:0] */
		unsigned int    rsv_2      : 3  ; /* [7:5] */
		unsigned int    y_ds16_g01 : 5  ; /* [12:8] */
		unsigned int    rsv_3      : 3  ; /* [15:13] */
		unsigned int    y_ds16_g02 : 5  ; /* [20:16] */
		unsigned int    rsv_4      : 3  ; /* [23:21] */
		unsigned int    y_ds16_g03 : 5  ; /* [28:24] */
		unsigned int    rsv_5      : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_Y_DS16_GAUSS_COEFF1_CFG */
union U_VBK_Y_DS16_GAUSS_COEFF1_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    y_ds16_g04       : 5  ; /* [4:0] */
		unsigned int    rsv_6            : 3  ; /* [7:5] */
		unsigned int    y_ds16_gauss_inv : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_UV_DS16_GAUSS_COEFF1_CFG */
union U_VBK_UV_DS16_GAUSS_COEFF1_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_7             : 8  ; /* [7:0] */
		unsigned int    uv_ds16_gauss_inv : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_UV_DS16_GAUSS_COEFF0_CFG */
union U_VBK_UV_DS16_GAUSS_COEFF0_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    uv_ds16_g00 : 5  ; /* [4:0] */
		unsigned int    rsv_8       : 3  ; /* [7:5] */
		unsigned int    uv_ds16_g01 : 5  ; /* [12:8] */
		unsigned int    rsv_9       : 3  ; /* [15:13] */
		unsigned int    uv_ds16_g02 : 5  ; /* [20:16] */
		unsigned int    rsv_10      : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_SIGMA_GAUSS_COEFF0_CFG */
union U_VBK_SIGMA_GAUSS_COEFF0_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    sigma_g00 : 5  ; /* [4:0] */
		unsigned int    rsv_11    : 3  ; /* [7:5] */
		unsigned int    sigma_g01 : 5  ; /* [12:8] */
		unsigned int    rsv_12    : 3  ; /* [15:13] */
		unsigned int    sigma_g02 : 5  ; /* [20:16] */
		unsigned int    rsv_13    : 3  ; /* [23:21] */
		unsigned int    sigma_g03 : 5  ; /* [28:24] */
		unsigned int    rsv_14    : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_SIGMA_GAUSS_COEFF1_CFG */
union U_VBK_SIGMA_GAUSS_COEFF1_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    sigma_g04       : 5  ; /* [4:0] */
		unsigned int    rsv_15          : 3  ; /* [7:5] */
		unsigned int    sigma_gauss_inv : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_SIGMA2ALPHA_LUT */
union U_VBK_SIGMA2ALPHA_LUT {
	/* Define the struct bits */
	struct {
		unsigned int    sigma2alpha : 8  ; /* [7:0] */
		unsigned int    rsv_16      : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_ADDNOISE_CFG */
union U_VBK_ADDNOISE_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    addnoise_th : 3  ; /* [2:0] */
		unsigned int    rsv_17      : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_FOREMASK_CFG */
union U_VBK_FOREMASK_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    foremask_dilation_radius    : 3  ; /* [2:0] */
		unsigned int    rsv_18                      : 1  ; /* [3] */
		unsigned int    foremask_weighted_filter_en : 1  ; /* [4] */
		unsigned int    rsv_19                      : 3  ; /* [7:5] */
		unsigned int    foremask_th                 : 8  ; /* [15:8] */
		unsigned int    foremask_coeff              : 8  ; /* [23:16] */
		unsigned int    rsv_20                      : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_TOP_DEBUG */
union U_VBK_TOP_DEBUG {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_top_debug : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_Y_DS16_GAUSS_DEBUG */
union U_VBK_Y_DS16_GAUSS_DEBUG {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_y_ds16_debug : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_UV_DS16_GAUSS_DEBUG */
union U_VBK_UV_DS16_GAUSS_DEBUG {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_uv_ds16_debug : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_SIGMA_GAUSS_DEBUG */
union U_VBK_SIGMA_GAUSS_DEBUG {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_sigma_debug : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_Y_CROP_H */
union U_VBK_Y_CROP_H {
	/* Define the struct bits */
	struct {
		unsigned int    y_crop_left  : 13  ; /* [12:0] */
		unsigned int    rsv_21       : 3  ; /* [15:13] */
		unsigned int    y_crop_right : 13  ; /* [28:16] */
		unsigned int    rsv_22       : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_Y_CROP_V */
union U_VBK_Y_CROP_V {
	/* Define the struct bits */
	struct {
		unsigned int    y_crop_top    : 13  ; /* [12:0] */
		unsigned int    rsv_23        : 3  ; /* [15:13] */
		unsigned int    y_crop_bottom : 13  ; /* [28:16] */
		unsigned int    rsv_24        : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_UV_CROP_H */
union U_VBK_UV_CROP_H {
	/* Define the struct bits */
	struct {
		unsigned int    uv_crop_left  : 13  ; /* [12:0] */
		unsigned int    rsv_25        : 3  ; /* [15:13] */
		unsigned int    uv_crop_right : 13  ; /* [28:16] */
		unsigned int    rsv_26        : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_UV_CROP_V */
union U_VBK_UV_CROP_V {
	/* Define the struct bits */
	struct {
		unsigned int    uv_crop_top    : 13  ; /* [12:0] */
		unsigned int    rsv_27         : 3  ; /* [15:13] */
		unsigned int    uv_crop_bottom : 13  ; /* [28:16] */
		unsigned int    rsv_28         : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_TF_CROP_H */
union U_VBK_TF_CROP_H {
	/* Define the struct bits */
	struct {
		unsigned int    tf_crop_left  : 11  ; /* [10:0] */
		unsigned int    rsv_29        : 5  ; /* [15:11] */
		unsigned int    tf_crop_right : 11  ; /* [26:16] */
		unsigned int    rsv_30        : 5  ; /* [31:27] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_TF_CROP_V */
union U_VBK_TF_CROP_V {
	/* Define the struct bits */
	struct {
		unsigned int    tf_crop_top    : 11  ; /* [10:0] */
		unsigned int    rsv_31         : 5  ; /* [15:11] */
		unsigned int    tf_crop_bottom : 11  ; /* [26:16] */
		unsigned int    rsv_32         : 5  ; /* [31:27] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct S_VBK_REGS_TYPE {
	union U_VBK_CFG  VBK_CFG; /* 0 */
	union U_VBK_Y_DS16_GAUSS_COEFF0_CFG  VBK_Y_DS16_GAUSS_COEFF0_CFG;
	union U_VBK_Y_DS16_GAUSS_COEFF1_CFG  VBK_Y_DS16_GAUSS_COEFF1_CFG;
	union U_VBK_UV_DS16_GAUSS_COEFF1_CFG VBK_UV_DS16_GAUSS_COEFF1_CFG;
	union U_VBK_UV_DS16_GAUSS_COEFF0_CFG VBK_UV_DS16_GAUSS_COEFF0_CFG;
	union U_VBK_SIGMA_GAUSS_COEFF0_CFG   VBK_SIGMA_GAUSS_COEFF0_CFG;
	union U_VBK_SIGMA_GAUSS_COEFF1_CFG   VBK_SIGMA_GAUSS_COEFF1_CFG;
	union U_VBK_SIGMA2ALPHA_LUT          VBK_SIGMA2ALPHA_LUT[256]; /* 1C */
	union U_VBK_ADDNOISE_CFG             VBK_ADDNOISE_CFG; /* 41C */
	union U_VBK_FOREMASK_CFG             VBK_FOREMASK_CFG; /* 420 */
	union U_VBK_TOP_DEBUG                VBK_TOP_DEBUG; /* 424 */
	union U_VBK_Y_DS16_GAUSS_DEBUG       VBK_Y_DS16_GAUSS_DEBUG; /* 428 */
	union U_VBK_UV_DS16_GAUSS_DEBUG      VBK_UV_DS16_GAUSS_DEBUG; /* 42C */
	union U_VBK_SIGMA_GAUSS_DEBUG   VBK_SIGMA_GAUSS_DEBUG; /* 430 */
	union U_VBK_Y_CROP_H       VBK_Y_CROP_H                 ; /* 434 */
	union U_VBK_Y_CROP_V       VBK_Y_CROP_V                 ; /* 438 */
	union U_VBK_UV_CROP_H    VBK_UV_CROP_H                ; /* 43C */
	union U_VBK_UV_CROP_V    VBK_UV_CROP_V                ; /* 440 */
	union U_VBK_TF_CROP_H    VBK_TF_CROP_H                ; /* 444 */
	union U_VBK_TF_CROP_V    VBK_TF_CROP_V                ; /* 448 */

};

/* Declare the struct pointor of the module VBK */
extern struct S_VBK_REGS_TYPE *gopVBKAllReg;


#endif /* __VBK_DRV_PRIV_H__ */
