// **********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  ipp_top_drv_priv.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **********************************************************

#ifndef __IPP_TOP_DRV_PRIV_H__
#define __IPP_TOP_DRV_PRIV_H__

/* Define the union U_DMA_CRG_CFG0 */
union U_DMA_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    jpg_dw_axi_gatedclock_en : 1; /* [0] */
		unsigned int    rsv_0                     : 15; /* [15:1] */
		unsigned int    control_disable_axi_data_packing : 1;
		unsigned int    mst_priority_cvdr         : 2 ; /* [18:17] */
		unsigned int    mst_priority_pac0         : 2 ; /* [20:19] */
		unsigned int    mst_priority_pac1          : 2 ; /* [22:21] */
		unsigned int    rsv_1                 : 9 ; /* [31:23] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DMA_CRG_CFG1 */
union U_DMA_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_clken     : 1  ; /* [0] */
		unsigned int    tbu_cvdr_clken : 1  ; /* [1] */
		unsigned int    rsv_2          : 1  ; /* [2] */
		unsigned int    cmdlst_clken   : 1  ; /* [3] */
		unsigned int    dpm_clken      : 1  ; /* [4] */
		unsigned int    rsv_3          : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DMA_CRG_CFG2 */
union U_DMA_CRG_CFG2 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_soft_rst     : 1  ; /* [0] */
		unsigned int    tbu_cvdr_soft_rst : 1  ; /* [1] */
		unsigned int    rsv_4             : 1  ; /* [2] */
		unsigned int    cmdlst_soft_rst   : 1  ; /* [3] */
		unsigned int    dpm_soft_rst      : 1  ; /* [4] */
		unsigned int    rsv_5             : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG0 */
union U_CVDR_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_clr : 8  ; /* [7:0] */
		unsigned int    rsv_6        : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG1 */
union U_CVDR_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_mask : 8  ; /* [7:0] */
		unsigned int    rsv_7         : 24  ; /* [31:8] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_IRQ_REG2 */
union U_CVDR_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_irq_state_mask : 8  ; /* [7:0] */
		unsigned int    rsv_8               : 8  ; /* [15:8] */
		unsigned int    cvdr_irq_state_raw  : 8  ; /* [23:16] */
		unsigned int    rsv_9               : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_MEM_CFG0 */
union U_CVDR_MEM_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    cvdr_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    mem_ctrl_sp      : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CVDR_MEM_CFG1 */
union U_CVDR_MEM_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_10      : 3  ; /* [2:0] */
		unsigned int    mem_ctrl_tp : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_DPM_CTRL */
union U_DPM_CTRL {
	/* Define the struct bits */
	struct {
		unsigned int    dpm_freq_sel : 3  ; /* [2:0] */
		unsigned int    rsv_11       : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_IPP_RESERVED */
union U_IPP_RESERVED {
	/* Define the struct bits */
	struct {
		unsigned int    reserved_ec : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_FLUX_CTRL0_0 */
union U_JPG_FLUX_CTRL0_0 {
	/* Define the struct bits */
	struct {
		unsigned int    flux_ctrl0_cvdr_r : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_FLUX_CTRL0_1 */
union U_JPG_FLUX_CTRL0_1 {
	/* Define the struct bits */
	struct {
		unsigned int    flux_ctrl1_cvdr_r : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_FLUX_CTRL1_0 */
union U_JPG_FLUX_CTRL1_0 {
	/* Define the struct bits */
	struct {
		unsigned int    flux_ctrl0_cvdr_w : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_FLUX_CTRL1_1 */
union U_JPG_FLUX_CTRL1_1 {
	/* Define the struct bits */
	struct {
		unsigned int    flux_ctrl1_cvdr_w : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_BWC_CHN_AR */
union U_JPG_BWC_CHN_AR {
	/* Define the struct bits */
	struct {
		unsigned int    jpg_bwc_cfg : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_RO_STATE */
union U_JPG_RO_STATE {
	/* Define the struct bits */
	struct {
		unsigned int    rsv_12            : 16  ; /* [15:0] */
		unsigned int    jpg_axi_dlock_irq : 1  ; /* [16] */
		unsigned int    jpg_axi_dlock_wr  : 1  ; /* [17] */
		unsigned int    jpg_axi_dlock_slv : 1  ; /* [18] */
		unsigned int    jpg_axi_dlock_mst : 1  ; /* [19] */
		unsigned int    jpg_axi_dlock_id  : 8  ; /* [27:20] */
		unsigned int    rsv_13            : 4  ; /* [31:28] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_CRG_CFG0 */
union U_JPGENC_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_clken        : 1  ; /* [0] */
		unsigned int    rsv_14              : 15  ; /* [15:1] */
		unsigned int    jpgenc_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_15              : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_CRG_CFG1 */
union U_JPGENC_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_16          : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_MEM_CFG */
union U_JPGENC_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_17             : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_IRQ_REG0 */
union U_JPGENC_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_irq_clr : 5  ; /* [4:0] */
		unsigned int    rsv_18         : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_IRQ_REG1 */
union U_JPGENC_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_irq_mask : 5  ; /* [4:0] */
		unsigned int    rsv_19          : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGENC_IRQ_REG2 */
union U_JPGENC_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgenc_irq_state_mask : 5  ; /* [4:0] */
		unsigned int    rsv_20                : 11  ; /* [15:5] */
		unsigned int    jpgenc_irq_state_raw  : 5  ; /* [20:16] */
		unsigned int    rsv_21                : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_CRG_CFG0 */
union U_JPGDEC_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_clken        : 1  ; /* [0] */
		unsigned int    rsv_22              : 15  ; /* [15:1] */
		unsigned int    jpgdec_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_23              : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_CRG_CFG1 */
union U_JPGDEC_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_24          : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_MEM_CFG */
union U_JPGDEC_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_25             : 1  ; /* [3] */
		unsigned int    jpgdec_mem_ctrl_tp : 3  ; /* [6:4] */
		unsigned int    rsv_26             : 25  ; /* [31:7] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_IRQ_REG0 */
union U_JPGDEC_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_irq_clr : 4  ; /* [3:0] */
		unsigned int    rsv_27         : 28  ; /* [31:4] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_IRQ_REG1 */
union U_JPGDEC_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_irq_mask : 4  ; /* [3:0] */
		unsigned int    rsv_28          : 28  ; /* [31:4] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPGDEC_IRQ_REG2 */
union U_JPGDEC_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    jpgdec_irq_state_mask : 4  ; /* [3:0] */
		unsigned int    rsv_29                : 12  ; /* [15:4] */
		unsigned int    jpgdec_irq_state_raw  : 4  ; /* [19:16] */
		unsigned int    rsv_30                : 12  ; /* [31:20] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_CRG_CFG0 */
union U_GF_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_clken        : 1  ; /* [0] */
		unsigned int    rsv_31          : 15  ; /* [15:1] */
		unsigned int    gf_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_32          : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_CRG_CFG1 */
union U_GF_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_33      : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_MEM_CFG */
union U_GF_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    gf_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_34         : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG0 */
union U_GF_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_clr : 13  ; /* [12:0] */
		unsigned int    rsv_35     : 19  ; /* [31:13] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG1 */
union U_GF_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_mask  : 13  ; /* [12:0] */
		unsigned int    rsv_36       : 3  ; /* [15:13] */
		unsigned int    gf_irq_outen : 2  ; /* [17:16] */
		unsigned int    rsv_37       : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_GF_IRQ_REG2 */
union U_GF_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    gf_irq_state_raw  : 13  ; /* [12:0] */
		unsigned int    rsv_38            : 3  ; /* [15:13] */
		unsigned int    gf_irq_state_mask : 13  ; /* [28:16] */
		unsigned int    rsv_39            : 3  ; /* [31:29] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_CRG_CFG0 */
union U_ORB_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    orb_clken        : 1  ; /* [0] */
		unsigned int    rsv_40           : 15  ; /* [15:1] */
		unsigned int    orb_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_41           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_CRG_CFG1 */
union U_ORB_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    orb_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_42       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_MEM_CFG */
union U_ORB_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    orb_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_43          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_IRQ_REG0 */
union U_ORB_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    orb_irq_clr : 16  ; /* [15:0] */
		unsigned int    rsv_44      : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_IRQ_REG1 */
union U_ORB_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    orb_irq_mask  : 16  ; /* [15:0] */
		unsigned int    orb_irq_outen : 2  ; /* [17:16] */
		unsigned int    rsv_45        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ORB_IRQ_REG2 */
union U_ORB_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    orb_irq_state_mask : 16  ; /* [15:0] */
		unsigned int    orb_irq_state_raw  : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_CRG_CFG0 */
union U_RDR_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_clken        : 1  ; /* [0] */
		unsigned int    rsv_46           : 15  ; /* [15:1] */
		unsigned int    rdr_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_47           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_CRG_CFG1 */
union U_RDR_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_48       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_MEM_CFG */
union U_RDR_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_49          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_IRQ_REG0 */
union U_RDR_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_irq_clr : 5  ; /* [4:0] */
		unsigned int    rsv_50      : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_IRQ_REG1 */
union U_RDR_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_irq_mask  : 5  ; /* [4:0] */
		unsigned int    rsv_51        : 11  ; /* [15:5] */
		unsigned int    rdr_irq_outen : 2  ; /* [17:16] */
		unsigned int    rsv_52        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_RDR_IRQ_REG2 */
union U_RDR_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    rdr_irq_state_mask : 5  ; /* [4:0] */
		unsigned int    rsv_53             : 11  ; /* [15:5] */
		unsigned int    rdr_irq_state_raw  : 5  ; /* [20:16] */
		unsigned int    rsv_54             : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_CRG_CFG0 */
union U_CMP_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_clken        : 1  ; /* [0] */
		unsigned int    rsv_55           : 15  ; /* [15:1] */
		unsigned int    cmp_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_56           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_CRG_CFG1 */
union U_CMP_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_57       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_MEM_CFG */
union U_CMP_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_58          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_IRQ_REG0 */
union U_CMP_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_irq_clr : 5  ; /* [4:0] */
		unsigned int    rsv_59      : 27  ; /* [31:5] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_IRQ_REG1 */
union U_CMP_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_irq_mask  : 5  ; /* [4:0] */
		unsigned int    rsv_60        : 11  ; /* [15:5] */
		unsigned int    cmp_irq_outen : 2  ; /* [17:16] */
		unsigned int    rsv_61        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMP_IRQ_REG2 */
union U_CMP_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    cmp_irq_state_mask : 5  ; /* [4:0] */
		unsigned int    rsv_62             : 11  ; /* [15:5] */
		unsigned int    cmp_irq_state_raw  : 5  ; /* [20:16] */
		unsigned int    rsv_63             : 11  ; /* [31:21] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_CRG_CFG0 */
union U_ENH_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_clken        : 1  ; /* [0] */
		unsigned int    rsv_64           : 15  ; /* [15:1] */
		unsigned int    enh_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_65           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_CRG_CFG1 */
union U_ENH_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_66       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_MEM_CFG */
union U_ENH_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    enh_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_67          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_VPB_CFG */
union U_ENH_VPB_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    enh_vpbg_en : 2  ; /* [1:0] */
		unsigned int    rsv_68      : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG0 */
union U_ENH_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_clr : 10  ; /* [9:0] */
		unsigned int    rsv_69      : 22  ; /* [31:10] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG1 */
union U_ENH_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_mask  : 10  ; /* [9:0] */
		unsigned int    rsv_70        : 6  ; /* [15:10] */
		unsigned int    enh_irq_outen : 2  ; /* [17:16] */
		unsigned int    rsv_71        : 14  ; /* [31:18] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ENH_IRQ_REG2 */
union U_ENH_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    enh_irq_state_mask : 10  ; /* [9:0] */
		unsigned int    rsv_72             : 6  ; /* [15:10] */
		unsigned int    enh_irq_state_raw  : 10  ; /* [25:16] */
		unsigned int    rsv_73             : 6  ; /* [31:26] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_CRG_CFG0 */
union U_VBK_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_clken        : 1  ; /* [0] */
		unsigned int    rsv_74           : 15  ; /* [15:1] */
		unsigned int    vbk_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_75           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_CRG_CFG1 */
union U_VBK_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_76       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_MEM_CFG */
union U_VBK_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_77          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_IRQ_REG0 */
union U_VBK_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_irq_clr : 24  ; /* [23:0] */
		unsigned int    rsv_78      : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_IRQ_REG1 */
union U_VBK_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_irq_mask : 24  ; /* [23:0] */
		unsigned int    rsv_79       : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_IRQ_REG2 */
union U_VBK_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_80        : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_IRQ_REG3 */
union U_VBK_IRQ_REG3 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_irq_state_mask : 24  ; /* [23:0] */
		unsigned int    rsv_81             : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_VBK_IRQ_REG4 */
union U_VBK_IRQ_REG4 {
	/* Define the struct bits */
	struct {
		unsigned int    vbk_irq_state_raw : 24  ; /* [23:0] */
		unsigned int    rsv_82            : 8  ; /* [31:24] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_CRG_CFG0 */
union U_ANF_CRG_CFG0 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_clken        : 1  ; /* [0] */
		unsigned int    rsv_83           : 15  ; /* [15:1] */
		unsigned int    anf_force_clk_on : 1  ; /* [16] */
		unsigned int    rsv_84           : 15  ; /* [31:17] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_CRG_CFG1 */
union U_ANF_CRG_CFG1 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_soft_rst : 1  ; /* [0] */
		unsigned int    rsv_85       : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_MEM_CFG */
union U_ANF_MEM_CFG {
	/* Define the struct bits */
	struct {
		unsigned int    anf_mem_ctrl_sp : 3  ; /* [2:0] */
		unsigned int    rsv_86          : 29  ; /* [31:3] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_IRQ_REG0 */
union U_ANF_IRQ_REG0 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_irq_clr : 31  ; /* [30:0] */
		unsigned int    rsv_87      : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_IRQ_REG1 */
union U_ANF_IRQ_REG1 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_irq_mask : 31  ; /* [30:0] */
		unsigned int    rsv_88       : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_IRQ_REG2 */
union U_ANF_IRQ_REG2 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_irq_outen : 2  ; /* [1:0] */
		unsigned int    rsv_89        : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_IRQ_REG3 */
union U_ANF_IRQ_REG3 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_irq_state_mask : 31  ; /* [30:0] */
		unsigned int    rsv_90             : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_ANF_IRQ_REG4 */
union U_ANF_IRQ_REG4 {
	/* Define the struct bits */
	struct {
		unsigned int    anf_irq_state_raw : 31  ; /* [30:0] */
		unsigned int    rsv_91            : 1  ; /* [31] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_CTRL_MAP */
union U_CMDLST_CTRL_MAP {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_ctrl_chn : 4  ; /* [3:0] */
		unsigned int    rsv_92          : 28  ; /* [31:4] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_CTRL_PM */
union U_CMDLST_CTRL_PM {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_post_mask : 2  ; /* [1:0] */
		unsigned int    rsv_93           : 30  ; /* [31:2] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_CLR */
union U_CMDLST_IRQ_CLR {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_clr : 16  ; /* [15:0] */
		unsigned int    rsv_94         : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_MSK */
union U_CMDLST_IRQ_MSK {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_mask : 16  ; /* [15:0] */
		unsigned int    rsv_95          : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_CMDLST_IRQ_STA */
union U_CMDLST_IRQ_STA {
	/* Define the struct bits */
	struct {
		unsigned int    cmdlst_irq_state_mask : 16  ; /* [15:0] */
		unsigned int    cmdlst_irq_state_raw  : 16  ; /* [31:16] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_DEBUG_0 */
union U_JPG_DEBUG_0 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_info_0 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_DEBUG_1 */
union U_JPG_DEBUG_1 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_info_1 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_DEBUG_2 */
union U_JPG_DEBUG_2 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_info_2 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_DEBUG_3 */
union U_JPG_DEBUG_3 {
	/* Define the struct bits */
	struct {
		unsigned int    debug_info_3 : 32  ; /* [31:0] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};

/* Define the union U_JPG_SEC_CTRL_S */
union U_JPG_SEC_CTRL_S {
	/* Define the struct bits */
	struct {
		unsigned int    top_tz_secure_n : 1  ; /* [0] */
		unsigned int    rsv_96          : 31  ; /* [31:1] */
	} bits;

	/* Define an unsigned member */
	unsigned int    u32;

};


// ==============================================================================
/* Define the global struct */
struct S_IPP_TOP_REGS_TYPE {
	union U_DMA_CRG_CFG0     DMA_CRG_CFG0        ; /* 0 */
	union U_DMA_CRG_CFG1     DMA_CRG_CFG1        ; /* 4 */
	union U_DMA_CRG_CFG2     DMA_CRG_CFG2        ; /* 8 */
	union U_CVDR_IRQ_REG0    CVDR_IRQ_REG0       ; /* 10 */
	union U_CVDR_IRQ_REG1    CVDR_IRQ_REG1       ; /* 14 */
	union U_CVDR_IRQ_REG2    CVDR_IRQ_REG2       ; /* 18 */
	union U_CVDR_MEM_CFG0    CVDR_MEM_CFG0       ; /* 20 */
	union U_CVDR_MEM_CFG1    CVDR_MEM_CFG1       ; /* 24 */
	union U_DPM_CTRL         DPM_CTRL            ; /* 28 */
	union U_IPP_RESERVED     IPP_RESERVED        ; /* 7C */
	union U_JPG_FLUX_CTRL0_0 JPG_FLUX_CTRL0_0    ; /* 80 */
	union U_JPG_FLUX_CTRL0_1 JPG_FLUX_CTRL0_1    ; /* 84 */
	union U_JPG_FLUX_CTRL1_0 JPG_FLUX_CTRL1_0    ; /* 88 */
	union U_JPG_FLUX_CTRL1_1 JPG_FLUX_CTRL1_1    ; /* 8C */
	union U_JPG_BWC_CHN_AR   JPG_BWC_CHN_AR      ; /* F0 */
	union U_JPG_RO_STATE     JPG_RO_STATE        ; /* FC */
	union U_JPGENC_CRG_CFG0  JPGENC_CRG_CFG0     ; /* 100 */
	union U_JPGENC_CRG_CFG1  JPGENC_CRG_CFG1     ; /* 104 */
	union U_JPGENC_MEM_CFG   JPGENC_MEM_CFG      ; /* 108 */
	union U_JPGENC_IRQ_REG0  JPGENC_IRQ_REG0     ; /* 110 */
	union U_JPGENC_IRQ_REG1  JPGENC_IRQ_REG1     ; /* 114 */
	union U_JPGENC_IRQ_REG2  JPGENC_IRQ_REG2     ; /* 118 */
	union U_JPGDEC_CRG_CFG0  JPGDEC_CRG_CFG0     ; /* 180 */
	union U_JPGDEC_CRG_CFG1  JPGDEC_CRG_CFG1     ; /* 184 */
	union U_JPGDEC_MEM_CFG   JPGDEC_MEM_CFG      ; /* 188 */
	union U_JPGDEC_IRQ_REG0  JPGDEC_IRQ_REG0     ; /* 190 */
	union U_JPGDEC_IRQ_REG1  JPGDEC_IRQ_REG1     ; /* 194 */
	union U_JPGDEC_IRQ_REG2  JPGDEC_IRQ_REG2     ; /* 198 */
	union U_GF_CRG_CFG0      GF_CRG_CFG0         ; /* 200 */
	union U_GF_CRG_CFG1      GF_CRG_CFG1         ; /* 204 */
	union U_GF_MEM_CFG       GF_MEM_CFG          ; /* 208 */
	union U_GF_IRQ_REG0      GF_IRQ_REG0         ; /* 210 */
	union U_GF_IRQ_REG1      GF_IRQ_REG1         ; /* 214 */
	union U_GF_IRQ_REG2      GF_IRQ_REG2         ; /* 218 */
	union U_ORB_CRG_CFG0     ORB_CRG_CFG0        ; /* 280 */
	union U_ORB_CRG_CFG1     ORB_CRG_CFG1        ; /* 284 */
	union U_ORB_MEM_CFG      ORB_MEM_CFG         ; /* 288 */
	union U_ORB_IRQ_REG0     ORB_IRQ_REG0        ; /* 290 */
	union U_ORB_IRQ_REG1     ORB_IRQ_REG1        ; /* 294 */
	union U_ORB_IRQ_REG2     ORB_IRQ_REG2        ; /* 298 */
	union U_RDR_CRG_CFG0     RDR_CRG_CFG0        ; /* 300 */
	union U_RDR_CRG_CFG1     RDR_CRG_CFG1        ; /* 304 */
	union U_RDR_MEM_CFG      RDR_MEM_CFG         ; /* 308 */
	union U_RDR_IRQ_REG0     RDR_IRQ_REG0        ; /* 310 */
	union U_RDR_IRQ_REG1     RDR_IRQ_REG1        ; /* 314 */
	union U_RDR_IRQ_REG2     RDR_IRQ_REG2        ; /* 318 */
	union U_CMP_CRG_CFG0     CMP_CRG_CFG0        ; /* 380 */
	union U_CMP_CRG_CFG1     CMP_CRG_CFG1        ; /* 384 */
	union U_CMP_MEM_CFG      CMP_MEM_CFG         ; /* 388 */
	union U_CMP_IRQ_REG0     CMP_IRQ_REG0        ; /* 390 */
	union U_CMP_IRQ_REG1     CMP_IRQ_REG1        ; /* 394 */
	union U_CMP_IRQ_REG2     CMP_IRQ_REG2        ; /* 398 */
	union U_ENH_CRG_CFG0     ENH_CRG_CFG0        ; /* 400 */
	union U_ENH_CRG_CFG1     ENH_CRG_CFG1        ; /* 404 */
	union U_ENH_MEM_CFG      ENH_MEM_CFG         ; /* 408 */
	union U_ENH_VPB_CFG      ENH_VPB_CFG         ; /* 40C */
	union U_ENH_IRQ_REG0     ENH_IRQ_REG0        ; /* 410 */
	union U_ENH_IRQ_REG1     ENH_IRQ_REG1        ; /* 414 */
	union U_ENH_IRQ_REG2     ENH_IRQ_REG2        ; /* 418 */
	union U_VBK_CRG_CFG0     VBK_CRG_CFG0        ; /* 480 */
	union U_VBK_CRG_CFG1     VBK_CRG_CFG1        ; /* 484 */
	union U_VBK_MEM_CFG      VBK_MEM_CFG         ; /* 488 */
	union U_VBK_IRQ_REG0     VBK_IRQ_REG0        ; /* 48C */
	union U_VBK_IRQ_REG1     VBK_IRQ_REG1        ; /* 490 */
	union U_VBK_IRQ_REG2     VBK_IRQ_REG2        ; /* 494 */
	union U_VBK_IRQ_REG3     VBK_IRQ_REG3        ; /* 498 */
	union U_VBK_IRQ_REG4     VBK_IRQ_REG4        ; /* 49C */
	union U_ANF_CRG_CFG0     ANF_CRG_CFG0        ; /* 4C0 */
	union U_ANF_CRG_CFG1     ANF_CRG_CFG1        ; /* 4C4 */
	union U_ANF_MEM_CFG      ANF_MEM_CFG         ; /* 4C8 */
	union U_ANF_IRQ_REG0     ANF_IRQ_REG0        ; /* 4CC */
	union U_ANF_IRQ_REG1     ANF_IRQ_REG1        ; /* 4D0 */
	union U_ANF_IRQ_REG2     ANF_IRQ_REG2        ; /* 4D4 */
	union U_ANF_IRQ_REG3     ANF_IRQ_REG3        ; /* 4D8 */
	union U_ANF_IRQ_REG4     ANF_IRQ_REG4        ; /* 4DC */
	union U_CMDLST_CTRL_MAP  CMDLST_CTRL_MAP[16] ; /* 500 */
	union U_CMDLST_CTRL_PM   CMDLST_CTRL_PM[16]  ; /* 504 */
	union U_CMDLST_IRQ_CLR   CMDLST_IRQ_CLR[6]   ; /* 600 */
	union U_CMDLST_IRQ_MSK   CMDLST_IRQ_MSK[6]   ; /* 604 */
	union U_CMDLST_IRQ_STA   CMDLST_IRQ_STA[6]   ; /* 608 */
	union U_JPG_DEBUG_0      JPG_DEBUG_0         ; /* 700 */
	union U_JPG_DEBUG_1      JPG_DEBUG_1         ; /* 704 */
	union U_JPG_DEBUG_2      JPG_DEBUG_2         ; /* 708 */
	union U_JPG_DEBUG_3      JPG_DEBUG_3         ; /* 70C */
	union U_JPG_SEC_CTRL_S   JPG_SEC_CTRL_S      ; /* 800 */

};

#endif // __IPP_TOP_DRV_PRIV_H__
