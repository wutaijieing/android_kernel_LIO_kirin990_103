// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  ar_feature_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/14
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __AR_FEATURE_DRV_PRIV_H__
#define __AR_FEATURE_DRV_PRIV_H__

/* Define the union U_TOP_CFG */
union u_top_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    mode         : 3  ; /* [2:0] */
		unsigned int    orient_en    : 1  ; /* [3] */
		unsigned int    layer        : 2  ; /* [5:4] */
		unsigned int    iter_num     : 1  ; /* [6] */
		unsigned int    first_detect : 1  ; /* [7] */
		unsigned int    rsv_0        : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_IMAGE_SIZE */
union u_image_size {
	/* Define the struct bits */
	struct {
		unsigned int    width  : 11  ; /* [10:0] */
		unsigned int    rsv_1  : 5  ; /* [15:11] */
		unsigned int    height : 11  ; /* [26:16] */
		unsigned int    rsv_2  : 5  ; /* [31:27] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_NUM_CFG */
union u_block_num_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_num : 5  ; /* [4:0] */
		unsigned int    rsv_3     : 3  ; /* [7:5] */
		unsigned int    blk_h_num : 5  ; /* [12:8] */
		unsigned int    rsv_4     : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_BLOCK_SIZE_CFG_INV */
union u_block_size_cfg_inv {
	/* Define the struct bits */
	struct {
		unsigned int    blk_v_size_inv : 16  ; /* [15:0] */
		unsigned int    blk_h_size_inv : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DETECT_NUMBER_LIMIT */
union u_detect_number_limit {
	/* Define the struct bits */
	struct {
		unsigned int    max_kpnum : 15  ; /* [14:0] */
		unsigned int    rsv_5     : 1  ; /* [15] */
		unsigned int    cur_kpnum : 15  ; /* [30:16] */
		unsigned int    rsv_6     : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_SIGMA_COEF */
union u_sigma_coef {
	/* Define the struct bits */
	struct {
		unsigned int    sigma_des : 10  ; /* [9:0] */
		unsigned int    rsv_7     : 6  ; /* [15:10] */
		unsigned int    sigma_ori : 10  ; /* [25:16] */
		unsigned int    rsv_8     : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GAUSS_ORG */
union u_gauss_org {
	/* Define the struct bits */
	struct {
		unsigned int    gauss_org_0 : 8  ; /* [7:0] */
		unsigned int    gauss_org_1 : 8  ; /* [15:8] */
		unsigned int    gauss_org_2 : 8  ; /* [23:16] */
		unsigned int    gauss_org_3 : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GSBLUR_1ST */
union u_gsblur_1st {
	/* Define the struct bits */
	struct {
		unsigned int    gauss_1st_0 : 8  ; /* [7:0] */
		unsigned int    gauss_1st_1 : 8  ; /* [15:8] */
		unsigned int    gauss_1st_2 : 8  ; /* [23:16] */
		unsigned int    rsv_9       : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GAUSS_2ND */
union u_gauss_2nd {
	/* Define the struct bits */
	struct {
		unsigned int    gauss_2nd_0 : 8  ; /* [7:0] */
		unsigned int    gauss_2nd_1 : 8  ; /* [15:8] */
		unsigned int    gauss_2nd_2 : 8  ; /* [23:16] */
		unsigned int    gauss_2nd_3 : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DOG_EDGE_THRESHOLD */
union u_dog_edge_threshold {
	/* Define the struct bits */
	struct {
		unsigned int    edge_threshold : 16  ; /* [15:0] */
		unsigned int    dog_threshold  : 15  ; /* [30:16] */
		unsigned int    rsv_10         : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DESCRIPTOR_THRESHOLD */
union u_descriptor_threshold {
	/* Define the struct bits */
	struct {
		unsigned int    mag_threshold : 8  ; /* [7:0] */
		unsigned int    rsv_11        : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_CFG */
union u_cvdr_rd_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_pixel_format    : 5  ; /* [4:0] */
		unsigned int    vprd_pixel_expansion : 1  ; /* [5] */
		unsigned int    vprd_allocated_du    : 5  ; /* [10:6] */
		unsigned int    rsv_12               : 2  ; /* [12:11] */
		unsigned int    vprd_last_page       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_LWG */
union u_cvdr_rd_lwg {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_size           : 15  ; /* [14:0] */
		unsigned int    rsv_13                   : 1  ; /* [15] */
		unsigned int    vprd_horizontal_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_14                   : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_FHG */
union u_cvdr_rd_fhg {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_frame_size        : 15  ; /* [14:0] */
		unsigned int    rsv_15                 : 1  ; /* [15] */
		unsigned int    vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_16                 : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_AXI_FS */
union u_cvdr_rd_axi_fs {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_17               : 2  ; /* [1:0] */
		unsigned int    vprd_axi_frame_start : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_AXI_LINE */
union u_cvdr_rd_axi_line {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_stride_0 : 14  ; /* [13:0] */
		unsigned int    rsv_18             : 4  ; /* [17:14] */
		unsigned int    vprd_line_wrap_0   : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_RD_IF_CFG */
union u_cvdr_rd_if_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_19                                 : 16  ; /* [15:0] */
		unsigned int    vp_rd_stop_enable_du_threshold_reached : 1  ; /* [16] */
		unsigned int    vp_rd_stop_enable_flux_ctrl            : 1  ; /* [17] */
		unsigned int    vp_rd_stop_enable_pressure             : 1  ; /* [18] */
		unsigned int    rsv_20                                 : 5  ; /* [23:19] */
		unsigned int    vp_rd_stop_ok                          : 1  ; /* [24] */
		unsigned int    vp_rd_stop                             : 1  ; /* [25] */
		unsigned int    rsv_21                                 : 5  ; /* [30:26] */
		unsigned int    vprd_prefetch_bypass                   : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PRE_CVDR_RD_FHG */
union u_pre_cvdr_rd_fhg {
	/* Define the struct bits */
	struct {
		unsigned int    pre_vprd_frame_size        : 15  ; /* [14:0] */
		unsigned int    rsv_22                     : 1  ; /* [15] */
		unsigned int    pre_vprd_vertical_blanking : 8  ; /* [23:16] */
		unsigned int    rsv_23                     : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_CFG */
union u_cvdr_wr_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_pixel_format    : 5  ; /* [4:0] */
		unsigned int    vpwr_pixel_expansion : 1  ; /* [5] */
		unsigned int    rsv_24               : 7  ; /* [12:6] */
		unsigned int    vpwr_last_page       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_AXI_FS */
union u_cvdr_wr_axi_fs {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_25                   : 2  ; /* [1:0] */
		unsigned int    vpwr_address_frame_start : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_AXI_LINE */
union u_cvdr_wr_axi_line {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_line_stride      : 14  ; /* [13:0] */
		unsigned int    vpwr_line_start_wstrb : 4  ; /* [17:14] */
		unsigned int    vpwr_line_wrap        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_WR_IF_CFG */
union u_cvdr_wr_if_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_26                                 : 16  ; /* [15:0] */
		unsigned int    vp_wr_stop_enable_du_threshold_reached : 1  ; /* [16] */
		unsigned int    vp_wr_stop_enable_flux_ctrl            : 1  ; /* [17] */
		unsigned int    vp_wr_stop_enable_pressure             : 1  ; /* [18] */
		unsigned int    rsv_27                                 : 5  ; /* [23:19] */
		unsigned int    vp_wr_stop_ok                          : 1  ; /* [24] */
		unsigned int    vp_wr_stop                             : 1  ; /* [25] */
		unsigned int    rsv_28                                 : 5  ; /* [30:26] */
		unsigned int    vpwr_prefetch_bypass                   : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_PRE_CVDR_WR_AXI_FS */
union u_pre_cvdr_wr_axi_fs {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_29                       : 2  ; /* [1:0] */
		unsigned int    pre_vpwr_address_frame_start : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_KPT_LIMIT */
union u_kpt_limit {
	struct {
		unsigned int    grid_dog_threshold : 16  ; /* [15:0] */
		unsigned int    grid_max_kpnum     : 8  ; /* [23:16] */
		unsigned int    rsv_30             : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_GRIDSTAT_1 */
union u_gridstat_1 {
	struct {
		unsigned int    min_score : 16  ; /* [15:0] */
		unsigned int    kpt_count : 12  ; /* [27:16] */
		unsigned int    rsv_31    : 4  ; /* [31:28] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_GRIDSTAT_2 */
union u_gridstat_2 {
	struct {
		unsigned int    sum_score : 28  ; /* [27:0] */
		unsigned int    rsv_32    : 4  ; /* [31:28] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_KPT_NUMBER */
union u_kpt_number {
	struct {
		unsigned int    kpt_num : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_TOTAL_KPT_NUM */
union u_total_kpt_num {
	/* Define the struct bits */
	struct {
		unsigned int    total_kpt_num : 15  ; /* [14:0] */
		unsigned int    rsv_33        : 17  ; /* [31:15] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DEBUG_0 */
union u_debug_0 {
	struct {
		unsigned int    debug0 : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_DEBUG_1 */
union u_debug_1 {
	struct {
		unsigned int    debug1 : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_DEBUG_2 */
union u_debug_2 {
	struct {
		unsigned int    debug2 : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};

/* Define the union U_DEBUG_3 */
union u_debug_3 {
	struct {
		unsigned int    debug3 : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;
};


//==============================================================================
struct s_ar_feature_regs_type {
	union u_top_cfg              top_cfg              ; /* 0 */
	union u_image_size           image_size           ; /* 4 */
	union u_block_num_cfg        block_num_cfg        ; /* 8 */
	union u_block_size_cfg_inv   block_size_cfg_inv   ; /* C */
	union u_detect_number_limit  detect_number_limit  ; /* 10 */
	union u_sigma_coef           sigma_coef           ; /* 14 */
	union u_gauss_org            gauss_org            ; /* 20 */
	union u_gsblur_1st           gsblur_1st           ; /* 24 */
	union u_gauss_2nd            gauss_2nd            ; /* 28 */
	union u_dog_edge_threshold   dog_edge_threshold   ; /* 30 */
	union u_descriptor_threshold descriptor_threshold ; /* 34 */
	union u_cvdr_rd_cfg          cvdr_rd_cfg          ; /* 40 */
	union u_cvdr_rd_lwg          cvdr_rd_lwg          ; /* 44 */
	union u_cvdr_rd_fhg          cvdr_rd_fhg          ; /* 48 */
	union u_cvdr_rd_axi_fs       cvdr_rd_axi_fs       ; /* 4C */
	union u_cvdr_rd_axi_line     cvdr_rd_axi_line     ; /* 50 */
	union u_cvdr_rd_if_cfg       cvdr_rd_if_cfg       ; /* 54 */
	union u_pre_cvdr_rd_fhg      pre_cvdr_rd_fhg      ; /* 58 */
	union u_cvdr_wr_cfg          cvdr_wr_cfg          ; /* 5C */
	union u_cvdr_wr_axi_fs       cvdr_wr_axi_fs       ; /* 60 */
	union u_cvdr_wr_axi_line     cvdr_wr_axi_line     ; /* 64 */
	union u_cvdr_wr_if_cfg       cvdr_wr_if_cfg       ; /* 68 */
	union u_pre_cvdr_wr_axi_fs   pre_cvdr_wr_axi_fs   ; /* 6C */
	union u_kpt_limit            kpt_limit[374]       ; /* 80 */
	union u_gridstat_1           gridstat_1[374]      ; /* 680 */
	union u_gridstat_2           gridstat_2[374]      ; /* C80 */
	union u_kpt_number           kpt_number[94]       ; /* 1280 */
	union u_total_kpt_num        total_kpt_num        ; /* 13F8 */
	union u_debug_0              debug_0              ; /* 1400 */
	union u_debug_1              debug_1              ; /* 1404 */
	union u_debug_2              debug_2              ; /* 1408 */
	union u_debug_3              debug_3              ; /* 140C */

};

#endif // __AR_FEATURE_DRV_PRIV_H__
