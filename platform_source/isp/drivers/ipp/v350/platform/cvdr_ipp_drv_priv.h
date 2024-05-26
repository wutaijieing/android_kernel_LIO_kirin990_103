// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  cvdr_ipp_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2013/3/10
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.2
// History       :  xxx 2020/04/17 10:53:41 Create file
// ******************************************************************************
#ifndef __CVDR_IPP_DRV_PRIV_H__
#define __CVDR_IPP_DRV_PRIV_H__

/* Define the union U_CVDR_IPP_CVDR_CFG */
union u_cvdr_ipp_cvdr_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    axiwrite_du_threshold  : 6  ; /* [5:0] */
		unsigned int    rsv_0                  : 2  ; /* [7:6] */
		unsigned int    du_threshold_reached   : 8  ; /* [15:8] */
		unsigned int    max_axiread_id         : 6  ; /* [21:16] */
		unsigned int    axi_wr_buffer_behavior : 2  ; /* [23:22] */
		unsigned int    max_axiwrite_id        : 6  ; /* [29:24] */
		unsigned int    rsv_1                  : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_CVDR_WR_QOS_CFG */
union u_cvdr_ipp_cvdr_wr_qos_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    wr_qos_threshold_01_stop  : 4  ; /* [3:0] */
		unsigned int    wr_qos_threshold_01_start : 4  ; /* [7:4] */
		unsigned int    wr_qos_threshold_10_stop  : 4  ; /* [11:8] */
		unsigned int    wr_qos_threshold_10_start : 4  ; /* [15:12] */
		unsigned int    wr_qos_threshold_11_stop  : 4  ; /* [19:16] */
		unsigned int    wr_qos_threshold_11_start : 4  ; /* [23:20] */
		unsigned int    rsv_2                     : 2  ; /* [25:24] */
		unsigned int    wr_qos_min                : 2  ; /* [27:26] */
		unsigned int    wr_qos_max                : 2  ; /* [29:28] */
		unsigned int    wr_qos_sr                 : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_CVDR_RD_QOS_CFG */
union u_cvdr_ipp_cvdr_rd_qos_cfg {
	/* Define the struct bits */
	struct {
		unsigned int    rd_qos_threshold_01_stop  : 4  ; /* [3:0] */
		unsigned int    rd_qos_threshold_01_start : 4  ; /* [7:4] */
		unsigned int    rd_qos_threshold_10_stop  : 4  ; /* [11:8] */
		unsigned int    rd_qos_threshold_10_start : 4  ; /* [15:12] */
		unsigned int    rd_qos_threshold_11_stop  : 4  ; /* [19:16] */
		unsigned int    rd_qos_threshold_11_start : 4  ; /* [23:20] */
		unsigned int    rsv_3                     : 2  ; /* [25:24] */
		unsigned int    rd_qos_min                : 2  ; /* [27:26] */
		unsigned int    rd_qos_max                : 2  ; /* [29:28] */
		unsigned int    rd_qos_sr                 : 2  ; /* [31:30] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_FORCE_CLK */
union u_cvdr_ipp_force_clk {
	/* Define the struct bits */
	struct {
		unsigned int    force_vprd_clk_on   : 1  ; /* [0] */
		unsigned int    force_vpwr_clk_on   : 1  ; /* [1] */
		unsigned int    force_nrrd_clk_on   : 1  ; /* [2] */
		unsigned int    force_nrwr_clk_on   : 1  ; /* [3] */
		unsigned int    force_axi_rd_clk_on : 1  ; /* [4] */
		unsigned int    force_axi_wr_clk_on : 1  ; /* [5] */
		unsigned int    force_du_rd_clk_on  : 1  ; /* [6] */
		unsigned int    force_du_wr_clk_on  : 1  ; /* [7] */
		unsigned int    force_cfg_clk_on    : 1  ; /* [8] */
		unsigned int    rsv_4               : 23  ; /* [31:9] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_CVDR_DEBUG_EN */
union u_cvdr_ipp_cvdr_debug_en {
	/* Define the struct bits */
	struct {
		unsigned int    wr_peak_en : 1  ; /* [0] */
		unsigned int    rsv_5      : 7  ; /* [7:1] */
		unsigned int    rd_peak_en : 1  ; /* [8] */
		unsigned int    rsv_6      : 23  ; /* [31:9] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_CVDR_DEBUG */
union u_cvdr_ipp_cvdr_debug {
	/* Define the struct bits */
	struct {
		unsigned int    wr_peak : 8  ; /* [7:0] */
		unsigned int    rd_peak : 8  ; /* [15:8] */
		unsigned int    rsv_7   : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_DEBUG */
union u_cvdr_ipp_debug {
	/* Define the struct bits */
	struct {
		unsigned int    debug : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_OTHER_RO */
union u_cvdr_ipp_other_ro {
	/* Define the struct bits */
	struct {
		unsigned int    other_ro : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_OTHER_RW */
union u_cvdr_ipp_other_rw {
	struct {
		unsigned int    other_rw : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_NR_RD_CFG_2 */
union u_cvdr_ipp_nr_rd_cfg_2 {
	struct {
		unsigned int    rsv_8                                    : 5  ; /* [4:0] */
		unsigned int    nrrd_allocated_du_2                      : 5  ; /* [9:5] */
		unsigned int    rsv_9                                    : 6  ; /* [15:10] */
		unsigned int    nr_rd_stop_enable_du_threshold_reached_2 : 1  ; /* [16] */
		unsigned int    nr_rd_stop_enable_flux_ctrl_2            : 1  ; /* [17] */
		unsigned int    nr_rd_stop_enable_pressure_2             : 1  ; /* [18] */
		unsigned int    rsv_10                                   : 5  ; /* [23:19] */
		unsigned int    nr_rd_stop_ok_2                          : 1  ; /* [24] */
		unsigned int    nr_rd_stop_2                             : 1  ; /* [25] */
		unsigned int    rsv_11                                   : 5  ; /* [30:26] */
		unsigned int    nrrd_enable_2                            : 1  ; /* [31] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_NR_RD_LIMITER_2 */
union u_cvdr_ipp_nr_rd_limiter_2 {
	struct {
		unsigned int    nrrd_access_limiter_0_2      : 4  ; /* [3:0] */
		unsigned int    nrrd_access_limiter_1_2      : 4  ; /* [7:4] */
		unsigned int    nrrd_access_limiter_2_2      : 4  ; /* [11:8] */
		unsigned int    nrrd_access_limiter_3_2      : 4  ; /* [15:12] */
		unsigned int    rsv_12                       : 8  ; /* [23:16] */
		unsigned int    nrrd_access_limiter_reload_2 : 4  ; /* [27:24] */
		unsigned int    rsv_13                       : 4  ; /* [31:28] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_NR_RD_DEBUG_2 */
union u_cvdr_ipp_nr_rd_debug_2 {
	struct {
		unsigned int    nr_rd_debug_2 : 32  ; /* [31:0] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_NR_WR_CFG_6 */
union u_cvdr_ipp_nr_wr_cfg_6 {
	struct {
		unsigned int    rsv_38                                   : 16  ; /* [15:0] */
		unsigned int    nr_wr_stop_enable_du_threshold_reached_6 : 1  ; /* [16] */
		unsigned int    nr_wr_stop_enable_flux_ctrl_6            : 1  ; /* [17] */
		unsigned int    nr_wr_stop_enable_pressure_6             : 1  ; /* [18] */
		unsigned int    rsv_39                                   : 5  ; /* [23:19] */
		unsigned int    nr_wr_stop_ok_6                          : 1  ; /* [24] */
		unsigned int    nr_wr_stop_6                             : 1  ; /* [25] */
		unsigned int    rsv_40                                   : 5  ; /* [30:26] */
		unsigned int    nrwr_enable_6                            : 1  ; /* [31] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_NR_WR_LIMITER_6 */
union u_cvdr_ipp_nr_wr_limiter_6 {
	struct {
		unsigned int    nrwr_access_limiter_0_6      : 4  ; /* [3:0] */
		unsigned int    nrwr_access_limiter_1_6      : 4  ; /* [7:4] */
		unsigned int    nrwr_access_limiter_2_6      : 4  ; /* [11:8] */
		unsigned int    nrwr_access_limiter_3_6      : 4  ; /* [15:12] */
		unsigned int    rsv_41                       : 8  ; /* [23:16] */
		unsigned int    nrwr_access_limiter_reload_6 : 4  ; /* [27:24] */
		unsigned int    rsv_42                       : 4  ; /* [31:28] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_CFG_0 */
union u_cvdr_ipp_vp_rd_cfg_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_pixel_format_0    : 5  ; /* [4:0] */
		unsigned int    vprd_pixel_expansion_0 : 1  ; /* [5] */
		unsigned int    vprd_allocated_du_0    : 5  ; /* [10:6] */
		unsigned int    rsv_53                 : 2  ; /* [12:11] */
		unsigned int    vprd_last_page_0       : 19  ; /* [31:13] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_LWG_0 */
union u_cvdr_ipp_vp_rd_lwg_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_size_0           : 15  ; /* [14:0] */
		unsigned int    rsv_54                     : 1  ; /* [15] */
		unsigned int    vprd_horizontal_blanking_0 : 8  ; /* [23:16] */
		unsigned int    rsv_55                     : 8  ; /* [31:24] */
	} bits;

	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_FHG_0 */
union u_cvdr_ipp_vp_rd_fhg_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_frame_size_0        : 15  ; /* [14:0] */
		unsigned int    rsv_56                   : 1  ; /* [15] */
		unsigned int    vprd_vertical_blanking_0 : 8  ; /* [23:16] */
		unsigned int    rsv_57                   : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_AXI_FS_0 */
union u_cvdr_ipp_vp_rd_axi_fs_0 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_58                 : 2  ; /* [1:0] */
		unsigned int    vprd_axi_frame_start_0 : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_AXI_LINE_0 */
union u_cvdr_ipp_vp_rd_axi_line_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_stride_0 : 14  ; /* [13:0] */
		unsigned int    rsv_59             : 4  ; /* [17:14] */
		unsigned int    vprd_line_wrap_0   : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_IF_CFG_0 */
union u_cvdr_ipp_vp_rd_if_cfg_0 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_60                                   : 16  ; /* [15:0] */
		unsigned int    vp_rd_stop_enable_du_threshold_reached_0 : 1  ; /* [16] */
		unsigned int    vp_rd_stop_enable_flux_ctrl_0            : 1  ; /* [17] */
		unsigned int    vp_rd_stop_enable_pressure_0             : 1  ; /* [18] */
		unsigned int    rsv_61                                   : 5  ; /* [23:19] */
		unsigned int    vp_rd_stop_ok_0                          : 1  ; /* [24] */
		unsigned int    vp_rd_stop_0                             : 1  ; /* [25] */
		unsigned int    rsv_62                                   : 5  ; /* [30:26] */
		unsigned int    vprd_prefetch_bypass_0                   : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_LIMITER_0 */
union u_cvdr_ipp_vp_rd_limiter_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vprd_access_limiter_0_0      : 4  ; /* [3:0] */
		unsigned int    vprd_access_limiter_1_0      : 4  ; /* [7:4] */
		unsigned int    vprd_access_limiter_2_0      : 4  ; /* [11:8] */
		unsigned int    vprd_access_limiter_3_0      : 4  ; /* [15:12] */
		unsigned int    rsv_63                       : 8  ; /* [23:16] */
		unsigned int    vprd_access_limiter_reload_0 : 4  ; /* [27:24] */
		unsigned int    rsv_64                       : 4  ; /* [31:28] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_RD_DEBUG_0 */
union u_cvdr_ipp_vp_rd_debug_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vp_rd_debug_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_IF_CFG_0 */
union u_cvdr_ipp_vp_wr_if_cfg_0 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_329                                  : 16  ; /* [15:0] */
		unsigned int    vp_wr_stop_enable_du_threshold_reached_0 : 1  ; /* [16] */
		unsigned int    vp_wr_stop_enable_flux_ctrl_0            : 1  ; /* [17] */
		unsigned int    vp_wr_stop_enable_pressure_0             : 1  ; /* [18] */
		unsigned int    rsv_330                                  : 5  ; /* [23:19] */
		unsigned int    vp_wr_stop_ok_0                          : 1  ; /* [24] */
		unsigned int    vp_wr_stop_0                             : 1  ; /* [25] */
		unsigned int    rsv_331                                  : 5  ; /* [30:26] */
		unsigned int    vpwr_prefetch_bypass_0                   : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_LIMITER_0 */
union u_cvdr_ipp_vp_wr_limiter_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_access_limiter_0_0      : 4  ; /* [3:0] */
		unsigned int    vpwr_access_limiter_1_0      : 4  ; /* [7:4] */
		unsigned int    vpwr_access_limiter_2_0      : 4  ; /* [11:8] */
		unsigned int    vpwr_access_limiter_3_0      : 4  ; /* [15:12] */
		unsigned int    rsv_332                      : 8  ; /* [23:16] */
		unsigned int    vpwr_access_limiter_reload_0 : 4  ; /* [27:24] */
		unsigned int    rsv_333                      : 4  ; /* [31:28] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_SID_AXI_LINE_SID0_0 */
union u_cvdr_ipp_vp_wr_sid_axi_line_sid0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_line_stride_sid0_0      : 14  ; /* [13:0] */
		unsigned int    vpwr_line_start_wstrb_sid0_0 : 4  ; /* [17:14] */
		unsigned int    vpwr_line_wrap_sid0_0        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_SID_PREFETCH_SID0_0 */
union u_cvdr_ipp_vp_wr_sid_prefetch_sid0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vp_wr_stripe_size_bytes_sid0_0 : 16  ; /* [15:0] */
		unsigned int    rsv_334                        : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_SID_CFG_SID0_0 */
union u_cvdr_ipp_vp_wr_sid_cfg_sid0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_pixel_format_sid0_0    : 5  ; /* [4:0] */
		unsigned int    vpwr_pixel_expansion_sid0_0 : 1  ; /* [5] */
		unsigned int    vpwr_4pf_enable_sid0_0      : 1  ; /* [6] */
		unsigned int    rsv_335                     : 6  ; /* [12:7] */
		unsigned int    vpwr_last_page_sid0_0       : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_SID_AXI_FS_SID0_0 */
union u_cvdr_ipp_vp_wr_sid_axi_fs_sid0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_336                         : 2  ; /* [1:0] */
		unsigned int    vpwr_address_frame_start_sid0_0 : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IPP_VP_WR_SID_DEBUG_SID0_0 */
union u_cvdr_ipp_vp_wr_sid_debug_sid0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    vp_wr_debug_sid0_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

//==============================================================================
struct s_cvdr_ipp_regs_type {
	union u_cvdr_ipp_cvdr_cfg                   cvdr_ipp_cvdr_cfg                   ; /* 0 */
	union u_cvdr_ipp_cvdr_wr_qos_cfg            cvdr_ipp_cvdr_wr_qos_cfg            ; /* 4 */
	union u_cvdr_ipp_cvdr_rd_qos_cfg            cvdr_ipp_cvdr_rd_qos_cfg            ; /* 8 */
	union u_cvdr_ipp_force_clk                  cvdr_ipp_force_clk                  ; /* c */
	union u_cvdr_ipp_cvdr_debug_en              cvdr_ipp_cvdr_debug_en              ; /* 10 */
	union u_cvdr_ipp_cvdr_debug                 cvdr_ipp_cvdr_debug                 ; /* 14 */
	union u_cvdr_ipp_debug                      cvdr_ipp_debug                      ; /* 18 */
	union u_cvdr_ipp_other_ro                   cvdr_ipp_other_ro                   ; /* 1c */
	union u_cvdr_ipp_other_rw                   cvdr_ipp_other_rw                   ; /* 20 */
	union u_cvdr_ipp_nr_rd_cfg_2                cvdr_ipp_nr_rd_cfg_2                ; /* 40 */
	union u_cvdr_ipp_nr_rd_limiter_2            cvdr_ipp_nr_rd_limiter_2            ; /* 44 */
	union u_cvdr_ipp_nr_rd_debug_2              cvdr_ipp_nr_rd_debug_2              ; /* 48 */
	union u_cvdr_ipp_vp_rd_cfg_0                cvdr_ipp_vp_rd_cfg_0                ; /* 100 */
	union u_cvdr_ipp_vp_rd_lwg_0                cvdr_ipp_vp_rd_lwg_0                ; /* 104 */
	union u_cvdr_ipp_vp_rd_fhg_0                cvdr_ipp_vp_rd_fhg_0                ; /* 108 */
	union u_cvdr_ipp_vp_rd_axi_fs_0             cvdr_ipp_vp_rd_axi_fs_0             ; /* 10c */
	union u_cvdr_ipp_vp_rd_axi_line_0           cvdr_ipp_vp_rd_axi_line_0           ; /* 110 */
	union u_cvdr_ipp_vp_rd_if_cfg_0             cvdr_ipp_vp_rd_if_cfg_0             ; /* 114 */
	union u_cvdr_ipp_vp_rd_limiter_0            cvdr_ipp_vp_rd_limiter_0            ; /* 118 */
	union u_cvdr_ipp_vp_rd_debug_0              cvdr_ipp_vp_rd_debug_0              ; /* 11c */
	union u_cvdr_ipp_vp_wr_if_cfg_0             cvdr_ipp_vp_wr_if_cfg_0             ; /* 500 */
	union u_cvdr_ipp_vp_wr_limiter_0            cvdr_ipp_vp_wr_limiter_0            ; /* 504 */
	union u_cvdr_ipp_vp_wr_sid_axi_line_sid0_0  cvdr_ipp_vp_wr_sid_axi_line_sid0_0  ; /* 50c */
	union u_cvdr_ipp_vp_wr_sid_prefetch_sid0_0  cvdr_ipp_vp_wr_sid_prefetch_sid0_0  ; /* 510 */
	union u_cvdr_ipp_vp_wr_sid_cfg_sid0_0       cvdr_ipp_vp_wr_sid_cfg_sid0_0       ; /* 514 */
	union u_cvdr_ipp_vp_wr_sid_axi_fs_sid0_0    cvdr_ipp_vp_wr_sid_axi_fs_sid0_0    ; /* 518 */
	union u_cvdr_ipp_vp_wr_sid_debug_sid0_0     cvdr_ipp_vp_wr_sid_debug_sid0_0     ; /* 51c */
};

#endif // __CVDR_IPP_DRV_PRIV_H__
