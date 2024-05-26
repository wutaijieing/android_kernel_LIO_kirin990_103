// *******************************************************
// Copyright     :  Copyright (C) 2018, Hisilicon Technologies Co. Ltd.
// File name     :  ipp_cvdr_drv_priv.h
// Project line  :  ISP
// Department    :
// Author        :  Anthony Sixta
// Version       :  1.0
// Date          :  2011/11/29
// Description   :  Image Sensor processing
// Others        :  Generated automatically by nManager V4.0.2.5
// History       :  Anthony Sixta 2018/06/30 09:23:10 Create file
// *************************************************** ****

#ifndef __IPP_CVDR_DRV_PRIV_H__
#define __IPP_CVDR_DRV_PRIV_H__

/* Define the union U_AXI_IPP_CVDR_CFG */
union U_AXI_IPP_CVDR_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    axiwrite_du_threshold : 6   ; /* [5..0]  */
		unsigned int    reserved_0            : 2   ; /* [7..6]  */
		unsigned int    du_threshold_reached  : 8   ; /* [15..8]  */
		unsigned int    max_axiread_id        : 6   ; /* [21..16]  */
		unsigned int    reserved_1            : 2   ; /* [23..22]  */
		unsigned int    max_axiwrite_id       : 5   ; /* [28..24]  */
		unsigned int    reserved_2            : 3   ; /* [31..29]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_CVDR_DEBUG_EN */
union U_AXI_IPP_CVDR_DEBUG_EN{
	/* Define the struct bits */
	struct {
		unsigned int    wr_peak_en            : 1   ; /* [0]  */
		unsigned int    reserved_0            : 7   ; /* [7..1]  */
		unsigned int    rd_peak_en            : 1   ; /* [8]  */
		unsigned int    reserved_1            : 23  ; /* [31..9]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_CVDR_DEBUG */
union U_AXI_IPP_CVDR_DEBUG{
	/* Define the struct bits */
	struct {
		unsigned int    wr_peak               : 8   ; /* [7..0]  */
		unsigned int    rd_peak               : 8   ; /* [15..8]  */
		unsigned int    reserved_0            : 16  ; /* [31..16]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_CVDR_WR_QOS_CFG */
union U_AXI_IPP_CVDR_WR_QOS_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    wr_qos_threshold_01_stop : 4; /* [3..0] */
		unsigned int    wr_qos_threshold_01_start : 4; /* [7..4] */
		unsigned int    wr_qos_threshold_10_stop : 4; /* [11..8] */
		unsigned int    wr_qos_threshold_10_start : 4; /* [15..12] */
		unsigned int    wr_qos_threshold_11_stop : 4; /* [19..16] */
		unsigned int    wr_qos_threshold_11_start : 4; /* [23..20] */
		unsigned int    reserved_0            : 2   ; /* [25..24]  */
		unsigned int    wr_qos_min            : 2   ; /* [27..26]  */
		unsigned int    wr_qos_max            : 2   ; /* [29..28]  */
		unsigned int    wr_qos_sr             : 2   ; /* [31..30]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_CVDR_RD_QOS_CFG */
union U_AXI_IPP_CVDR_RD_QOS_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    rd_qos_threshold_01_stop : 4 ; /* [3..0] */
		unsigned int    rd_qos_threshold_01_start : 4 ; /* [7..4] */
		unsigned int    rd_qos_threshold_10_stop : 4 ; /* [11..8] */
		unsigned int    rd_qos_threshold_10_start : 4 ; /* [15..12] */
		unsigned int    rd_qos_threshold_11_stop : 4 ; /* [19..16] */
		unsigned int    rd_qos_threshold_11_start : 4 ; /* [23..20] */
		unsigned int    reserved_0            : 2   ; /* [25..24]  */
		unsigned int    rd_qos_min            : 2   ; /* [27..26]  */
		unsigned int    rd_qos_max            : 2   ; /* [29..28]  */
		unsigned int    rd_qos_sr             : 2   ; /* [31..30]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_FORCE_CLK */
union U_AXI_IPP_FORCE_CLK{
	/* Define the struct bits */
	struct {
		unsigned int    force_vprd_clk_on     : 1   ; /* [0]  */
		unsigned int    force_vpwr_clk_on     : 1   ; /* [1]  */
		unsigned int    force_nrrd_clk_on     : 1   ; /* [2]  */
		unsigned int    force_nrwr_clk_on     : 1   ; /* [3]  */
		unsigned int    force_axi_rd_clk_on   : 1   ; /* [4]  */
		unsigned int    force_axi_wr_clk_on   : 1   ; /* [5]  */
		unsigned int    force_du_rd_clk_on    : 1   ; /* [6]  */
		unsigned int    force_du_wr_clk_on    : 1   ; /* [7]  */
		unsigned int    force_cfg_clk_on      : 1   ; /* [8]  */
		unsigned int    reserved_0            : 23  ; /* [31..9]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_OTHER_RO */
union U_AXI_IPP_OTHER_RO{
	/* Define the struct bits  */
	struct {
		unsigned int other_ro               : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};
/* Define the union U_AXI_IPP_OTHER_RW */
union U_AXI_IPP_OTHER_RW{
	/* Define the struct bits  */
	struct {
		unsigned int other_rw               : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_WR_CFG */
union U_AXI_IPP_VP_WR_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_pixel_format      : 5   ; /* [4..0]  */
		unsigned int    vpwr_pixel_expansion   : 1   ; /* [5]  */
		unsigned int    reserved_0             : 7   ; /* [12..6]  */
		unsigned int    vpwr_last_page         : 19  ; /* [31..13]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_WR_AXI_FS */
union U_AXI_IPP_VP_WR_AXI_FS{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 2   ; /* [1..0]  */
		unsigned int    vpwr_address_frame_start : 30  ; /* [31..2]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_WR_AXI_LINE */
union U_AXI_IPP_VP_WR_AXI_LINE{
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_line_stride      : 14  ; /* [13..0]  */
		unsigned int    vpwr_line_start_wstrb : 4   ; /* [17..14]  */
		unsigned int    vpwr_line_wrap        : 14  ; /* [31..18]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_WR_IF_CFG */
union U_AXI_IPP_VP_WR_IF_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 16  ; /* [15..0]  */
		unsigned int    vp_wr_stop_enable_du_threshold_reached : 1;
		unsigned int    vp_wr_stop_enable_flux_ctrl : 1   ; /* [17]  */
		unsigned int    vp_wr_stop_enable_pressure : 1   ; /* [18]  */
		unsigned int    reserved_1            : 5   ; /* [23..19]  */
		unsigned int    vp_wr_stop_ok       : 1   ; /* [24]  */
		unsigned int    vp_wr_stop          : 1   ; /* [25]  */
		unsigned int    reserved_2            : 5   ; /* [30..26]  */
		unsigned int    vpwr_prefetch_bypass : 1   ; /* [31]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_LIMITER_VP_WR */
union U_AXI_IPP_LIMITER_VP_WR{
	/* Define the struct bits */
	struct {
		unsigned int    vpwr_access_limiter_0 : 4   ; /* [3..0]  */
		unsigned int    vpwr_access_limiter_1 : 4   ; /* [7..4]  */
		unsigned int    vpwr_access_limiter_2 : 4   ; /* [11..8]  */
		unsigned int    vpwr_access_limiter_3 : 4   ; /* [15..12]  */
		unsigned int    reserved_0            : 8   ; /* [23..16]  */
		unsigned int    vpwr_access_limiter_reload : 4; /* [27..24] */
		unsigned int    reserved_1            : 4   ; /* [31..28]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_CFG */
union U_AXI_IPP_VP_RD_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    vprd_pixel_format   : 5   ; /* [4..0]  */
		unsigned int    vprd_pixel_expansion : 1   ; /* [5]  */
		unsigned int    vprd_allocated_du   : 5   ; /* [10..6]  */
		unsigned int    reserved_0            : 2   ; /* [12..11]  */
		unsigned int    vprd_last_page      : 19  ; /* [31..13]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_LWG */
union U_AXI_IPP_VP_RD_LWG{
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_size      : 13  ; /* [12..0]  */
		unsigned int    reserved_0            : 3   ; /* [15..13]  */
		unsigned int    vprd_horizontal_blanking : 8   ; /* [23..16]  */
		unsigned int    reserved_1            : 8   ; /* [31..24]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_FHG */
union U_AXI_IPP_VP_RD_FHG{
	/* Define the struct bits */
	struct {
		unsigned int    vprd_frame_size     : 13  ; /* [12..0]  */
		unsigned int    reserved_0            : 3   ; /* [15..13]  */
		unsigned int    vprd_vertical_blanking : 8   ; /* [23..16]  */
		unsigned int    reserved_1            : 8   ; /* [31..24]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_AXI_FS */
union U_AXI_IPP_VP_RD_AXI_FS{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 2   ; /* [1..0]  */
		unsigned int    vprd_axi_frame_start : 30  ; /* [31..2]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_AXI_LINE */
union U_AXI_IPP_VP_RD_AXI_LINE{
	/* Define the struct bits */
	struct {
		unsigned int    vprd_line_stride    : 14  ; /* [13..0]  */
		unsigned int    reserved_0            : 4   ; /* [17..14]  */
		unsigned int    vprd_line_wrap      : 14  ; /* [31..18]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_IF_CFG */
union U_AXI_IPP_VP_RD_IF_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 16  ; /* [15..0]  */
		unsigned int    vp_rd_stop_enable_du_threshold_reached : 1;
		unsigned int    vp_rd_stop_enable_flux_ctrl : 1   ; /* [17]  */
		unsigned int    vp_rd_stop_enable_pressure : 1   ; /* [18]  */
		unsigned int    reserved_1            : 5   ; /* [23..19]  */
		unsigned int    vp_rd_stop_ok       : 1   ; /* [24]  */
		unsigned int    vp_rd_stop          : 1   ; /* [25]  */
		unsigned int    reserved_2            : 5   ; /* [30..26]  */
		unsigned int    vprd_prefetch_bypass : 1   ; /* [31]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_RD_DEBUG */
union U_AXI_IPP_VP_RD_DEBUG{
	/* Define the struct bits  */
	struct {
		unsigned int vp_rd_debug          : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_LIMITER_VP_RD */
union U_AXI_IPP_LIMITER_VP_RD{
	/* Define the struct bits */
	struct {
		unsigned int    vprd_access_limiter_0 : 4   ; /* [3..0]  */
		unsigned int    vprd_access_limiter_1 : 4   ; /* [7..4]  */
		unsigned int    vprd_access_limiter_2 : 4   ; /* [11..8]  */
		unsigned int    vprd_access_limiter_3 : 4   ; /* [15..12]  */
		unsigned int    reserved_0            : 8   ; /* [23..16]  */
		unsigned int    vprd_access_limiter_reload : 4; /* [27..24] */
		unsigned int    reserved_1            : 4   ; /* [31..28]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_NR_WR_CFG */
union U_AXI_IPP_NR_WR_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 16  ; /* [15..0]  */
		unsigned int    nr_wr_stop_enable_du_threshold_reached : 1;
		unsigned int    nr_wr_stop_enable_flux_ctrl : 1   ; /* [17]  */
		unsigned int    nr_wr_stop_enable_pressure : 1   ; /* [18]  */
		unsigned int    reserved_1            : 5   ; /* [23..19]  */
		unsigned int    nr_wr_stop_ok       : 1   ; /* [24]  */
		unsigned int    nr_wr_stop          : 1   ; /* [25]  */
		unsigned int    reserved_2            : 5   ; /* [30..26]  */
		unsigned int    nrwr_enable         : 1   ; /* [31]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_NR_WR_DEBUG */
union U_AXI_IPP_NR_WR_DEBUG{
	/* Define the struct bits  */
	struct {
		unsigned int nr_wr_debug          : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_LIMITER_NR_WR */
union U_AXI_IPP_LIMITER_NR_WR{
	/* Define the struct bits */
	struct {
		unsigned int    nrwr_access_limiter_0 : 4   ; /* [3..0]  */
		unsigned int    nrwr_access_limiter_1 : 4   ; /* [7..4]  */
		unsigned int    nrwr_access_limiter_2 : 4   ; /* [11..8]  */
		unsigned int    nrwr_access_limiter_3 : 4   ; /* [15..12]  */
		unsigned int    reserved_0            : 8   ; /* [23..16]  */
		unsigned int    nrwr_access_limiter_reload : 4; /* [27..24] */
		unsigned int    reserved_1            : 4   ; /* [31..28]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_NR_RD_CFG */
union U_AXI_IPP_NR_RD_CFG{
	/* Define the struct bits */
	struct {
		unsigned int    reserved_0            : 5   ; /* [4..0]  */
		unsigned int    nrrd_allocated_du   : 5   ; /* [9..5]  */
		unsigned int    reserved_1            : 6   ; /* [15..10]  */
		unsigned int    nr_rd_stop_enable_du_threshold_reached : 1;
		unsigned int    nr_rd_stop_enable_flux_ctrl : 1   ; /* [17]  */
		unsigned int    nr_rd_stop_enable_pressure : 1   ; /* [18]  */
		unsigned int    reserved_2            : 5   ; /* [23..19]  */
		unsigned int    nr_rd_stop_ok       : 1   ; /* [24]  */
		unsigned int    nr_rd_stop          : 1   ; /* [25]  */
		unsigned int    reserved_3            : 5   ; /* [30..26]  */
		unsigned int    nrrd_enable         : 1   ; /* [31]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

} ;

/* Define the union U_AXI_IPP_NR_RD_DEBUG */
union U_AXI_IPP_NR_RD_DEBUG{
	/* Define the struct bits  */
	struct {
		unsigned int nr_rd_debug          : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_LIMITER_NR_RD */
union U_AXI_IPP_LIMITER_NR_RD{
	/* Define the struct bits */
	struct {
		unsigned int    nrrd_access_limiter_0 : 4   ; /* [3..0]  */
		unsigned int    nrrd_access_limiter_1 : 4   ; /* [7..4]  */
		unsigned int    nrrd_access_limiter_2 : 4   ; /* [11..8]  */
		unsigned int    nrrd_access_limiter_3 : 4   ; /* [15..12]  */
		unsigned int    reserved_0            : 8   ; /* [23..16]  */
		unsigned int    nrrd_access_limiter_reload : 4; /* [27..24] */
		unsigned int    reserved_1            : 4   ; /* [31..28]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_SPARE */
union U_AXI_IPP_SPARE{
	/* Define the struct bits  */
	struct {
		unsigned int spare                : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_VP_WR_DEBUG */
union U_AXI_IPP_VP_WR_DEBUG{
	/* Define the struct bits  */
	struct {
		unsigned int vp_wr_debug          : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_AXI_IPP_DEBUG */
union U_AXI_IPP_DEBUG{
	/* Define the struct bits  */
	struct {
		unsigned int debug                : 32  ; /* [31..0]  */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

struct S_IPP_VP_WR{
	union U_AXI_IPP_VP_WR_CFG        IPP_CVDR_VP_WR_CFG;
	union U_AXI_IPP_VP_WR_AXI_FS     IPP_CVDR_VP_WR_AXI_FS;
	union U_AXI_IPP_VP_WR_AXI_LINE   IPP_CVDR_VP_WR_AXI_LINE;
	union U_AXI_IPP_VP_WR_IF_CFG     IPP_CVDR_VP_WR_IF_CFG;
};

struct S_IPP_VP_RD{
	union U_AXI_IPP_VP_RD_CFG        IPP_CVDR_VP_RD_CFG;
	union U_AXI_IPP_VP_RD_LWG        IPP_CVDR_VP_RD_LWG;
	union U_AXI_IPP_VP_RD_FHG        IPP_CVDR_VP_RD_FHG;
	union U_AXI_IPP_VP_RD_AXI_FS     IPP_CVDR_VP_RD_AXI_FS;
	union U_AXI_IPP_VP_RD_AXI_LINE   IPP_CVDR_VP_RD_AXI_LINE;
	union U_AXI_IPP_VP_RD_IF_CFG     IPP_CVDR_VP_RD_IF_CFG;
	unsigned int               reserved_10;
	union U_AXI_IPP_VP_RD_DEBUG      IPP_CVDR_VP_RD_DEBUG;
};

struct S_IPP_NR_WR{
	union U_AXI_IPP_NR_WR_CFG        IPP_CVDR_NR_WR_CFG;
	union U_AXI_IPP_NR_WR_DEBUG   IPP_CVDR_NR_WR_DEBUG;
	union U_AXI_IPP_LIMITER_NR_WR  IPP_CVDR_LIMITER_NR_WR;
};

struct S_IPP_NR_RD{
	union U_AXI_IPP_NR_RD_CFG        IPP_CVDR_NR_RD_CFG;
	union U_AXI_IPP_NR_RD_DEBUG      IPP_CVDR_NR_RD_DEBUG;
	union U_AXI_IPP_LIMITER_NR_RD    IPP_CVDR_LIMITER_NR_RD;
	unsigned int   reserved_0;
};

// ===================================
/* Define the global struct */
struct S_IPP_CVDR_REGS_TYPE{
	union U_AXI_IPP_CVDR_CFG         IPP_CVDR_CVDR_CFG           ; /* 0x0 */
	union U_AXI_IPP_CVDR_DEBUG_EN    IPP_CVDR_CVDR_DEBUG_EN      ; /* 0x4 */
	union U_AXI_IPP_CVDR_DEBUG       IPP_CVDR_CVDR_DEBUG         ; /* 0x8 */
	union U_AXI_IPP_CVDR_WR_QOS_CFG  IPP_CVDR_CVDR_WR_QOS_CFG    ; /* 0xc */
	union U_AXI_IPP_CVDR_RD_QOS_CFG  IPP_CVDR_CVDR_RD_QOS_CFG    ; /* 0x10 */
	union U_AXI_IPP_FORCE_CLK        IPP_CVDR_FORCE_CLK          ; /* 0x14 */
	unsigned int     reserved_0[2]; /* 0x18~0x1c */
	union U_AXI_IPP_OTHER_RO         IPP_CVDR_OTHER_RO; /* 0x20 */
	union U_AXI_IPP_OTHER_RW         IPP_CVDR_OTHER_RW; /* 0x24 */
	unsigned int               reserved_1[2]; /* 0x28~0x2c */
	struct S_IPP_VP_WR                IPP_CVDR_VP_WR[31]; /* 0x30 */
	unsigned int    reserved_2[388]; /* 0x220~0x82c */
	union U_AXI_IPP_LIMITER_VP_WR    IPP_CVDR_LIMITER_VP_WR[31]  ; /* 0x830 */
	unsigned int      reserved_3[97]; /* 0x8ac~0xa2c */
	struct S_IPP_VP_RD    IPP_CVDR_VP_RD[19]; /* 0xa30 */
	unsigned int      reserved_4[360]; /* 0xc90~0x122c */
	union U_AXI_IPP_LIMITER_VP_RD    IPP_CVDR_LIMITER_VP_RD[19]  ; /* 0x1230 */
	unsigned int               reserved_5[45]; /* 0x127c~0x132c */
	struct S_IPP_NR_WR                IPP_CVDR_NR_WR[10]          ; /* 0x1330 */
	unsigned int               reserved_6[88]; /* 0x13d0~0x152c */
	struct S_IPP_NR_RD   IPP_CVDR_NR_RD[10]; /* 0x1530 */
	unsigned int  reserved_7[472]; /* 0x15d0~0x1d2c */
	union U_AXI_IPP_SPARE   IPP_CVDR_SPARE[4]; /* 0x1d30 */
	union U_AXI_IPP_VP_WR_DEBUG      IPP_CVDR_VP_WR_DEBUG[31]; /* 0x1d40 */
	unsigned int  reserved_8[97] ; /* 0x1dbc~0x1f3c */
	union U_AXI_IPP_DEBUG   IPP_CVDR_DEBUG[16]; /* 0x1f40 */
};

#endif /* __IPP_CVDR_DRV_PRIV_H__ */
