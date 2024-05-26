// ***********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  orb_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2018/12/28
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// ***********************************************************

#ifndef __ORB_DRV_PRIV_H__
#define __ORB_DRV_PRIV_H__

/* Define the union U_ORB_CFG */
union U_ORB_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    pyramid_en   : 1  ; /* [0] */
		unsigned int    gsblur_en    : 1  ; /* [1] */
		unsigned int    fast_en      : 1  ; /* [2] */
		unsigned int    nms_en       : 1  ; /* [3] */
		unsigned int    orient_en    : 1  ; /* [4] */
		unsigned int    brief_en     : 1  ; /* [5] */
		unsigned int    freak_en     : 1  ; /* [6] */
		unsigned int    gridstat_en  : 1  ; /* [7] */
		unsigned int    undistort_en : 1  ; /* [8] */
		unsigned int    first_stripe : 1  ; /* [9] */
		unsigned int    first_layer  : 1  ; /* [10] */
		unsigned int    rsv_0        : 21  ; /* [31:11] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_IMAGE_SIZE */
union U_IMAGE_SIZE {
	/* Define the struct bits */
	struct {
		unsigned int    width  : 10  ; /* [9:0] */
		unsigned int    rsv_1  : 6  ; /* [15:10] */
		unsigned int    height : 11  ; /* [26:16] */
		unsigned int    rsv_2  : 5  ; /* [31:27] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_STAT_RANGE */
union U_STAT_RANGE {
	/* Define the struct bits */
	struct {
		unsigned int    stat_right : 10  ; /* [9:0] */
		unsigned int    rsv_3      : 6  ; /* [15:10] */
		unsigned int    stat_left  : 10  ; /* [25:16] */
		unsigned int    rsv_4      : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_SIZE_CFG_INV */
union U_BLOCK_SIZE_CFG_INV {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_size_inv : 16  ; /* [15:0] */
		unsigned int    blk_h_size_inv : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PYRAMID_INC_CFG */
union U_PYRAMID_INC_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    scl_inc : 20  ; /* [19:0] */
		unsigned int    rsv_5   : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PYRAMID_VCROP_CFGB */
union U_PYRAMID_VCROP_CFGB {
	/* Define the struct bits */
	struct {
		unsigned int    scl_vbottom : 29  ; /* [28:0] */
		unsigned int    rsv_6       : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PYRAMID_VCROP_CFGT */
union U_PYRAMID_VCROP_CFGT {
	/* Define the struct bits */
	struct {
		unsigned int    scl_vtop : 29  ; /* [28:0] */
		unsigned int    rsv_7    : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PYRAMID_HCROP_CFGR */
union U_PYRAMID_HCROP_CFGR {
	/* Define the struct bits */
	struct {
		unsigned int    scl_hright : 29  ; /* [28:0] */
		unsigned int    rsv_8      : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PYRAMID_HCROP_CFGL */
union U_PYRAMID_HCROP_CFGL {
	/* Define the struct bits */
	struct {
		unsigned int    scl_hleft : 29  ; /* [28:0] */
		unsigned int    rsv_9     : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_COEF_01 */
union U_GSBLUR_COEF_01 {
	/* Define the struct bits */
	struct {
		unsigned int    coeff_gauss_1 : 10  ; /* [9:0] */
		unsigned int    rsv_10        : 6  ; /* [15:10] */
		unsigned int    coeff_gauss_0 : 10  ; /* [25:16] */
		unsigned int    rsv_11        : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_COEF_23 */
union U_GSBLUR_COEF_23 {
	/* Define the struct bits */
	struct {
		unsigned int    coeff_gauss_3 : 10  ; /* [9:0] */
		unsigned int    rsv_12        : 6  ; /* [15:10] */
		unsigned int    coeff_gauss_2 : 10  ; /* [25:16] */
		unsigned int    rsv_13        : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_THRESHOLD_CFG */
union U_THRESHOLD_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    min_th : 8  ; /* [7:0] */
		unsigned int    ini_th : 8  ; /* [15:8] */
		unsigned int    rsv_14 : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_NMS_WIN_CFG */
union U_NMS_WIN_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    nmscell_v : 4  ; /* [3:0] */
		unsigned int    nmscell_h : 5  ; /* [8:4] */
		unsigned int    rsv_15    : 23  ; /* [31:9] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_LAYER0_START_H */
union U_LAYER0_START_H {
	/* Define the struct bits */
	struct {
		unsigned int    start_blk_h_offset : 11  ; /* [10:0] */
		unsigned int    rsv_16             : 1  ; /* [11] */
		unsigned int    start_blk_h_num    : 5  ; /* [16:12] */
		unsigned int    rsv_17             : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_LAYER0_START_V */
union U_LAYER0_START_V {
	/* Define the struct bits */
	struct {
		unsigned int    start_blk_v_offset : 11  ; /* [10:0] */
		unsigned int    rsv_18             : 1  ; /* [11] */
		unsigned int    start_blk_v_num    : 5  ; /* [16:12] */
		unsigned int    rsv_19             : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_NUM_CFG */
union U_BLOCK_NUM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_num : 5  ; /* [4:0] */
		unsigned int    rsv_20    : 3  ; /* [7:5] */
		unsigned int    blk_h_num : 5  ; /* [12:8] */
		unsigned int    rsv_21    : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_SIZE_CFG */
union U_BLOCK_SIZE_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_size : 11  ; /* [10:0] */
		unsigned int    rsv_22     : 1  ; /* [11] */
		unsigned int    blk_h_size : 11  ; /* [22:12] */
		unsigned int    rsv_23     : 9  ; /* [31:23] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_OCTREE_CFG */
union U_OCTREE_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    grid_max_kpnum : 10  ; /* [9:0] */
		unsigned int    rsv_24         : 2  ; /* [11:10] */
		unsigned int    max_kpnum      : 15  ; /* [26:12] */
		unsigned int    rsv_25         : 1  ; /* [27] */
		unsigned int    flag_10_11     : 1  ; /* [28] */
		unsigned int    rsv_26         : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_INC_LUT_CFG */
union U_INC_LUT_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    inc_level : 20  ; /* [19:0] */
		unsigned int    rsv_27    : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_CX */
union U_UNDISTORT_CX {
	/* Define the struct bits */
	struct {
		unsigned int    cx     : 21  ; /* [20:0] */
		unsigned int    rsv_28 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_CY */
union U_UNDISTORT_CY {
	/* Define the struct bits */
	struct {
		unsigned int    cy     : 21  ; /* [20:0] */
		unsigned int    rsv_29 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_FX */
union U_UNDISTORT_FX {
	/* Define the struct bits */
	struct {
		unsigned int    fx     : 21  ; /* [20:0] */
		unsigned int    rsv_30 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_FY */
union U_UNDISTORT_FY {
	/* Define the struct bits */
	struct {
		unsigned int    fy     : 21  ; /* [20:0] */
		unsigned int    rsv_31 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_INVFX */
union U_UNDISTORT_INVFX {
	/* Define the struct bits */
	struct {
		unsigned int    invfx  : 20  ; /* [19:0] */
		unsigned int    rsv_32 : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_INVFY */
union U_UNDISTORT_INVFY {
	/* Define the struct bits */
	struct {
		unsigned int    invfy  : 20  ; /* [19:0] */
		unsigned int    rsv_33 : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_K1 */
union U_UNDISTORT_K1 {
	/* Define the struct bits */
	struct {
		unsigned int    k1     : 21  ; /* [20:0] */
		unsigned int    rsv_34 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_K2 */
union U_UNDISTORT_K2 {
	/* Define the struct bits */
	struct {
		unsigned int    k2     : 21  ; /* [20:0] */
		unsigned int    rsv_35 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_K3 */
union U_UNDISTORT_K3 {
	/* Define the struct bits */
	struct {
		unsigned int    k3     : 21  ; /* [20:0] */
		unsigned int    rsv_36 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_P1 */
union U_UNDISTORT_P1 {
	/* Define the struct bits */
	struct {
		unsigned int    p1     : 21  ; /* [20:0] */
		unsigned int    rsv_37 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_UNDISTORT_P2 */
union U_UNDISTORT_P2 {
	/* Define the struct bits */
	struct {
		unsigned int    p2     : 21  ; /* [20:0] */
		unsigned int    rsv_38 : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_CFG */
union U_CVDR_RD_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_pixel_format    : 5  ; /* [4:0] */
		unsigned int    vprd_pixel_expansion : 1  ; /* [5] */
		unsigned int    vprd_allocated_du    : 5  ; /* [10:6] */
		unsigned int    rsv_39               : 2  ; /* [12:11] */
		unsigned int    vprd_last_page       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_LWG */
union U_CVDR_RD_LWG {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_size           : 13  ; /* [12:0] */
		unsigned int    rsv_40                   : 3  ; /* [15:13] */
		unsigned int    vprd_horizontal_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_41                   : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_FHG */
union U_CVDR_RD_FHG {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_frame_size        : 13  ; /* [12:0] */
		unsigned int    rsv_42                 : 3  ; /* [15:13] */
		unsigned int    vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_43                 : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_AXI_FS */
union U_CVDR_RD_AXI_FS {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_44               : 2  ; /* [1:0] */
		unsigned int    vprd_axi_frame_start : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_AXI_LINE */
union U_CVDR_RD_AXI_LINE {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_stride : 11  ; /* [10:0] */
		unsigned int    rsv_45           : 5  ; /* [15:11] */
		unsigned int    vprd_line_wrap   : 13  ; /* [28:16] */
		unsigned int    rsv_46           : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_IF_CFG */
union U_CVDR_RD_IF_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_47    : 16; /* [15:0] */
		unsigned int    vp_rd_stop_enable_du_threshold_reached : 1;
		unsigned int    vp_rd_stop_enable_flux_ctrl  : 1; /* [17] */
		unsigned int    vp_rd_stop_enable_pressure : 1; /* [18] */
		unsigned int    rsv_48       : 5  ; /* [23:19] */
		unsigned int    vp_rd_stop_ok       : 1; /* [24] */
		unsigned int    vp_rd_stop              : 1; /* [25] */
		unsigned int    rsv_49                       : 5; /* [30:26] */
		unsigned int    vprd_prefetch_bypass   : 1; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PRE_CVDR_RD_FHG */
union U_PRE_CVDR_RD_FHG {
	/* Define the struct bits */
	struct {
		unsigned int    pre_vprd_frame_size        : 13  ; /* [12:0] */
		unsigned int    rsv_50                     : 3  ; /* [15:13] */
		unsigned int    pre_vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_51                     : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_CFG */
union U_CVDR_WR_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_pixel_format    : 5  ; /* [4:0] */
		unsigned int    vpwr_pixel_expansion : 1  ; /* [5] */
		unsigned int    rsv_52               : 7  ; /* [12:6] */
		unsigned int    vpwr_last_page       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_AXI_FS */
union U_CVDR_WR_AXI_FS {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_53                   : 2  ; /* [1:0] */
		unsigned int    vpwr_address_frame_start : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_AXI_LINE */
union U_CVDR_WR_AXI_LINE {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_line_stride      : 11  ; /* [10:0] */
		unsigned int    vpwr_line_start_wstrb : 4  ; /* [14:11] */
		unsigned int    vpwr_line_wrap        : 14  ; /* [28:15] */
		unsigned int    rsv_54                : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_IF_CFG */
union U_CVDR_WR_IF_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_55                      : 16; /* [15:0] */
		unsigned int    vp_wr_stop_enable_du_threshold_reached : 1;
		unsigned int    vp_wr_stop_enable_flux_ctrl   : 1  ; /* [17] */
		unsigned int    vp_wr_stop_enable_pressure  : 1  ; /* [18] */
		unsigned int    rsv_56                       : 5; /* [23:19] */
		unsigned int    vp_wr_stop_ok        : 1; /* [24] */
		unsigned int    vp_wr_stop              : 1; /* [25] */
		unsigned int    rsv_57                       : 5; /* [30:26] */
		unsigned int    vpwr_prefetch_bypass   : 1; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PRE_CVDR_WR_AXI_FS */
union U_PRE_CVDR_WR_AXI_FS {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_58                       : 2  ; /* [1:0] */
		unsigned int    pre_vpwr_address_frame_start : 30;
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG 0 */
union U_DEBUG0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG 1 */
union U_DEBUG1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BRIEF_PATTERN */
union U_BRIEF_PATTERN {
	/* Define the struct bits */
	struct {
		unsigned int    pattern_x0 : 6  ; /* [5:0] */
		unsigned int    rsv_59     : 2  ; /* [7:6] */
		unsigned int    pattern_y0 : 6  ; /* [13:8] */
		unsigned int    rsv_60     : 2  ; /* [15:14] */
		unsigned int    pattern_x1 : 6  ; /* [21:16] */
		unsigned int    rsv_61     : 2  ; /* [23:22] */
		unsigned int    pattern_y1 : 6  ; /* [29:24] */
		unsigned int    rsv_62     : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_SCORE_THESHOLD */
union U_SCORE_THESHOLD {
	/* Define the struct bits */
	struct {
		unsigned int    score_th : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_KPT_NUMBER */
union U_KPT_NUMBER {
	/* Define the struct bits */
	struct {
		unsigned int    kpt_num : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_TOTAL_KPT_NUM */
union U_TOTAL_KPT_NUM {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num : 15  ; /* [14:0] */
		unsigned int    rsv_63        : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_KPNUM_NMS */
union U_KPNUM_NMS {
	/* Define the struct bits */
	struct {
		unsigned int    kpnum_nms : 16  ; /* [15:0] */
		unsigned int    rsv_64    : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GRIDSTAT */
union U_GRIDSTAT {
	/* Define the struct bits */
	struct {
		unsigned int    grid_score : 18  ; /* [17:0] */
		unsigned int    rsv_65     : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct S_ORB_REGS_TYPE {
	union U_ORB_CFG           ORB_CFG           ; /* 0 */
	union U_IMAGE_SIZE         IMAGE_SIZE         ; /* 4 */
	union U_STAT_RANGE         STAT_RANGE         ; /* 8 */
	union U_BLOCK_SIZE_CFG_INV BLOCK_SIZE_CFG_INV ; /* C */
	union U_PYRAMID_INC_CFG    PYRAMID_INC_CFG    ; /* 10 */
	union U_PYRAMID_VCROP_CFGB PYRAMID_VCROP_CFGB ; /* 14 */
	union U_PYRAMID_VCROP_CFGT PYRAMID_VCROP_CFGT ; /* 18 */
	union U_PYRAMID_HCROP_CFGR PYRAMID_HCROP_CFGR ; /* 1C */
	union U_PYRAMID_HCROP_CFGL PYRAMID_HCROP_CFGL ; /* 20 */
	union U_GSBLUR_COEF_01     GSBLUR_COEF_01     ; /* 30 */
	union U_GSBLUR_COEF_23     GSBLUR_COEF_23     ; /* 34 */
	union U_THRESHOLD_CFG      THRESHOLD_CFG      ; /* 40 */
	union U_NMS_WIN_CFG        NMS_WIN_CFG        ; /* 44 */
	union U_LAYER0_START_H     LAYER0_START_H     ; /* 48 */
	union U_LAYER0_START_V     LAYER0_START_V     ; /* 4C */
	union U_BLOCK_NUM_CFG      BLOCK_NUM_CFG      ; /* 50 */
	union U_BLOCK_SIZE_CFG     BLOCK_SIZE_CFG     ; /* 54 */
	union U_OCTREE_CFG         OCTREE_CFG         ; /* 58 */
	union U_INC_LUT_CFG        INC_LUT_CFG        ; /* 5C */
	union U_UNDISTORT_CX       UNDISTORT_CX       ; /* 60 */
	union U_UNDISTORT_CY       UNDISTORT_CY       ; /* 64 */
	union U_UNDISTORT_FX       UNDISTORT_FX       ; /* 68 */
	union U_UNDISTORT_FY       UNDISTORT_FY       ; /* 6C */
	union U_UNDISTORT_INVFX    UNDISTORT_INVFX    ; /* 70 */
	union U_UNDISTORT_INVFY    UNDISTORT_INVFY    ; /* 74 */
	union U_UNDISTORT_K1       UNDISTORT_K1       ; /* 78 */
	union U_UNDISTORT_K2       UNDISTORT_K2       ; /* 7C */
	union U_UNDISTORT_K3       UNDISTORT_K3       ; /* 80 */
	union U_UNDISTORT_P1       UNDISTORT_P1       ; /* 84 */
	union U_UNDISTORT_P2       UNDISTORT_P2       ; /* 88 */
	union U_CVDR_RD_CFG        CVDR_RD_CFG        ; /* 90 */
	union U_CVDR_RD_LWG        CVDR_RD_LWG        ; /* 94 */
	union U_CVDR_RD_FHG        CVDR_RD_FHG        ; /* 98 */
	union U_CVDR_RD_AXI_FS     CVDR_RD_AXI_FS     ; /* 9C */
	union U_CVDR_RD_AXI_LINE   CVDR_RD_AXI_LINE   ; /* A0 */
	union U_CVDR_RD_IF_CFG     CVDR_RD_IF_CFG     ; /* A4 */
	union U_PRE_CVDR_RD_FHG    PRE_CVDR_RD_FHG    ; /* A8 */
	union U_CVDR_WR_CFG        CVDR_WR_CFG        ; /* B0 */
	union U_CVDR_WR_AXI_FS     CVDR_WR_AXI_FS     ; /* B4 */
	union U_CVDR_WR_AXI_LINE   CVDR_WR_AXI_LINE   ; /* B8 */
	union U_CVDR_WR_IF_CFG     CVDR_WR_IF_CFG     ; /* BC */
	union U_PRE_CVDR_WR_AXI_FS PRE_CVDR_WR_AXI_FS ; /* C0 */
	union U_DEBUG0            DEBUG0            ; /* D0 */
	union U_DEBUG1            DEBUG1            ; /* D4 */
	union U_BRIEF_PATTERN      BRIEF_PATTERN[256] ; /* 100 */
	union U_SCORE_THESHOLD     SCORE_THESHOLD[94] ; /* 500 */
	union U_KPT_NUMBER         KPT_NUMBER[187]    ; /* 700 */
	union U_TOTAL_KPT_NUM      TOTAL_KPT_NUM      ; /* 9EC */
	union U_KPNUM_NMS          KPNUM_NMS          ; /* 9F0 */
	union U_GRIDSTAT           GRIDSTAT[748]      ; /* A00 */

};

#endif // __ORB_DRV_PRIV_H__
