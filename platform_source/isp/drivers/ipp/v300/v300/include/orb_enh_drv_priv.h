// **********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  orb_enh_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// ***********************************************************

#ifndef __ORB_ENH_DRV_PRIV_H__
#define __ORB_ENH_DRV_PRIV_H__

/* Define the union U_ENABLE_CFG */
union U_ENABLE_CFG {
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
union U_BLK_NUM {
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
union U_BLK_SIZE {
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
union U_INV_BLK_SIZE_Y {
	/* Define the struct bits */
	struct {
		unsigned int    inv_ysize : 17  ; /* [16:0] */
		unsigned int    rsv_3     : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INV_BLK_SIZE_X */
union U_INV_BLK_SIZE_X {
	/* Define the struct bits */
	struct {
		unsigned int    inv_xsize : 17  ; /* [16:0] */
		unsigned int    rsv_4     : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_EXTEND_CFG */
union U_EXTEND_CFG {
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
union U_HIST_CLIMIT {
	/* Define the struct bits */
	struct {
		unsigned int    climit : 11  ; /* [10:0] */
		unsigned int    rsv_7  : 21  ; /* [31:11] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_LUT_SCALE */
union U_LUT_SCALE {
	/* Define the struct bits */
	struct {
		unsigned int    lutscale : 18  ; /* [17:0] */
		unsigned int    rsv_8    : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_COEF_01 */
union U_GSBLUR_COEF_01 {
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
union U_GSBLUR_COEF_23 {
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
struct S_ORB_ENH_REGS_TYPE {
	union U_ENABLE_CFG     ENABLE_CFG     ; /* 0 */
	union U_BLK_NUM        BLK_NUM        ; /* 8 */
	union U_BLK_SIZE       BLK_SIZE       ; /* 10 */
	union U_INV_BLK_SIZE_Y INV_BLK_SIZE_Y ; /* 14 */
	union U_INV_BLK_SIZE_X INV_BLK_SIZE_X ; /* 18 */
	union U_EXTEND_CFG     EXTEND_CFG     ; /* 1C */
	union U_HIST_CLIMIT    HIST_CLIMIT    ; /* 20 */
	union U_LUT_SCALE      LUT_SCALE      ; /* 24 */
	union U_GSBLUR_COEF_01 GSBLUR_COEF_01 ; /* 30 */
	union U_GSBLUR_COEF_23 GSBLUR_COEF_23 ; /* 34 */

};

#endif // __ORB_ENH_DRV_PRIV_H__

