/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DPU_OVERLAY_UTILS_PLATFORM_H_
#define _DPU_OVERLAY_UTILS_PLATFORM_H_

#include "../dpu_display_effect_dssv510.h"
#include "../dpu_smmu_base.h"

#define CONFIG_DSS_LP_USED

static uint32_t g_dss_module_base[DSS_CHN_MAX_DEFINE][MODULE_CHN_MAX] = {
	/* D0 */
	{
	MIF_CH0_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH0_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH0_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH0,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH0_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH0_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH0_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD0_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_D0_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_D0_DFC_OFFSET,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	DSS_RCH_D0_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* D1 */
	{
	0,   /* MODULE_MIF_CHN */
	0,  /* MODULE_AIF0_CHN */
	0,  /* MODULE_AIF1_CHN */
	0,  /* MODULE_MCTL_CHN_MUTEX */
	0,  /* MODULE_MCTL_CHN_FLUSH_EN */
	0,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	0,  /* MODULE_DMA */
	0,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
	/* V0 */
	{
	MIF_CH2_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH2_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH2_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH2,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH2_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH2_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH2_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD2_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_VG0_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_VG0_DFC_OFFSET,  /* MODULE_DFC */
	DSS_RCH_VG0_SCL_OFFSET,  /* MODULE_SCL */
	DSS_RCH_VG0_SCL_LUT_OFFSET,  /* MODULE_SCL_LUT */
	DSS_RCH_VG0_ARSR_OFFSET,  /* MODULE_ARSR2P */
	DSS_RCH_VG0_ARSR_LUT_OFFSET,  /* MODULE_ARSR2P_LUT */
	DSS_RCH_VG0_POST_CLIP_OFFSET,  /* MODULE_POST_CLIP */
	DSS_RCH_VG0_PCSC_OFFSET,  /* MODULE_PCSC */
	DSS_RCH_VG0_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* G0 */
	{
	MIF_CH3_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH3_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH3_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH3,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH3_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH3_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH3_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD3_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_G0_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_G0_DFC_OFFSET,  /* MODULE_DFC */
	DSS_RCH_G0_SCL_OFFSET,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	DSS_RCH_G0_POST_CLIP_OFFSET,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
	/* V1 */
	{
	MIF_CH4_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH4_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH4_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH4,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH4_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH4_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH4_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD4_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_VG1_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_VG1_DFC_OFFSET,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	DSS_RCH_VG1_POST_CLIP_OFFSET,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	DSS_RCH_VG1_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* G1 */
	{
	0,   /* MODULE_MIF_CHN */
	0,  /* MODULE_AIF0_CHN */
	0,  /* MODULE_AIF1_CHN */
	0,  /* MODULE_MCTL_CHN_MUTEX */
	0,  /* MODULE_MCTL_CHN_FLUSH_EN */
	0,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	0,  /* MODULE_DMA */
	0,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
	/* D2 */
	{
	MIF_CH6_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH6_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH6_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH6,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH6_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH6_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH6_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD6_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_D2_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_D2_DFC_OFFSET,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	DSS_RCH_D2_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* D3 */
	{
	MIF_CH7_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH7_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH7_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_RCH7,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH7_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH7_OV_OEN,  /* MODULE_MCTL_CHN_OV_OEN */
	DSS_MCTRL_SYS_OFFSET + MCTL_RCH7_STARTY,  /* MODULE_MCTL_CHN_STARTY */
	DSS_MCTRL_SYS_OFFSET + MCTL_MOD7_DBG,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_RCH_D3_DMA_OFFSET,  /* MODULE_DMA */
	DSS_RCH_D3_DFC_OFFSET,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	DSS_RCH_D3_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* W0 */
	{
	0,   /* MODULE_MIF_CHN */
	0,  /* MODULE_AIF0_CHN */
	0,  /* MODULE_AIF1_CHN */
	0,  /* MODULE_MCTL_CHN_MUTEX */
	0,  /* MODULE_MCTL_CHN_FLUSH_EN */
	0,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	0,  /* MODULE_DMA */
	0,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
	/* W1 */
	{
	MIF_CH9_OFFSET,   /* MODULE_MIF_CHN */
	AIF0_CH9_OFFSET,  /* MODULE_AIF0_CHN */
	AIF1_CH9_OFFSET,  /* MODULE_AIF1_CHN */
	MCTL_CTL_MUTEX_WCH1,  /* MODULE_MCTL_CHN_MUTEX */
	DSS_MCTRL_SYS_OFFSET + MCTL_WCH1_FLUSH_EN,  /* MODULE_MCTL_CHN_FLUSH_EN */
	DSS_MCTRL_SYS_OFFSET + MCTL_WCH1_OV_IEN,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	DSS_WCH1_DMA_OFFSET,  /* MODULE_DMA */
	DSS_WCH1_DFC_OFFSET,  /* MODULE_DFC */
	DSS_WCH1_SCL_OFFSET,  /* MODULE_SCL */
	DSS_WCH1_SCL_LUT_OFFSET,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	DSS_WCH1_POST_CLIP_OFFSET,  /* MODULE_POST_CLIP */
	DSS_WCH1_PCSC_OFFSET,  /* MODULE_PCSC */
	DSS_WCH1_CSC_OFFSET,  /* MODULE_CSC */
	},
	/* V2 */
	{
	0,  /* MODULE_MIF_CHN */
	0,  /* MODULE_AIF0_CHN */
	0,  /* MODULE_AIF1_CHN */
	0,  /* MODULE_MCTL_CHN_MUTEX */
	0,  /* MODULE_MCTL_CHN_FLUSH_EN */
	0,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	0,  /* MODULE_DMA */
	0,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
	/* W2 */
	{
	0,  /* MODULE_MIF_CHN */
	0,  /* MODULE_AIF0_CHN */
	0,  /* MODULE_AIF1_CHN */
	0,  /* MODULE_MCTL_CHN_MUTEX */
	0,  /* MODULE_MCTL_CHN_FLUSH_EN */
	0,  /* MODULE_MCTL_CHN_OV_OEN */
	0,  /* MODULE_MCTL_CHN_STARTY */
	0,  /* MODULE_MCTL_CHN_MOD_DBG */
	0,  /* MODULE_DMA */
	0,  /* MODULE_DFC */
	0,  /* MODULE_SCL */
	0,  /* MODULE_SCL_LUT */
	0,  /* MODULE_ARSR2P */
	0,  /* MODULE_ARSR2P_LUT */
	0,  /* MODULE_POST_CLIP */
	0,  /* MODULE_PCSC */
	0,  /* MODULE_CSC */
	},
};

static uint32_t g_dss_module_ovl_base[DSS_MCTL_IDX_MAX][MODULE_OVL_MAX] = {
	{ DSS_OVL0_OFFSET, DSS_MCTRL_CTL0_OFFSET },
	{ 0, 0 },
	{ DSS_OVL2_OFFSET, DSS_MCTRL_CTL2_OFFSET },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
};

static uint32_t g_dss_module_cap[DSS_CHN_MAX_DEFINE][MODULE_CAP_MAX] = {
	/* D2 */
	{0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
	/* D3 */
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
	/* V0 */
	{0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1},
	/* G0 */
	{0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	/* V1 */
	{0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1},
	/* G1 */
	{0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	/* D0 */
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
	/* D1 */
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},

	/* W0 */
	{1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1},
	/* W1 */
	{1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1},

	/* V2 */
	{0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1},
	/* W2 */
	{1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1},
};

/* number of smrx idx for each channel */
static uint32_t g_dss_chn_sid_num[DSS_CHN_MAX_DEFINE] = {
	4, 1, 4, 4, 4, 4, 1, 1, 3, 4, 3, 3
};

static uint32_t g_dss_smmu_smrx_idx[DSS_CHN_MAX_DEFINE] = {
	0, 4, 5, 9, 13, 17, 21, 22, 26, 29, 23, 36
};

/* GPIO */
#define GPIO_PG_SEL_A 168  /* GPIO_21_0 */
#define GPIO_TX_RX_A 170  /* GPIO_21_2 */
#define GPIO_PG_SEL_B 133  /*  GPIO_16_5 */
#define GPIO_TX_RX_B 135  /*  GPIO_16_7 */

#define CRGPERI_PLL0_CLK_RATE 1622016000UL /* SPLL */
#define CRGPERI_PLL2_CLK_RATE 1920000000UL
#define CRGPERI_PLL3_CLK_RATE 1200000000UL

#define DEFAULT_DSS_CORE_CLK_RATE_L4 541000000UL
#define DEFAULT_DSS_CORE_CLK_RATE_L3 406000000UL
#define DEFAULT_DSS_CORE_CLK_RATE_L2 325000000UL
#define DEFAULT_DSS_CORE_CLK_RATE_L1 214000000UL
#define TEMP_DSS_CORE_CLK_RATE 232000000UL

#define DEFAULT_MDC_CORE_CLK_RATE_L4 541000000UL
#define DEFAULT_MDC_CORE_CLK_RATE_L3 406000000UL
#define DEFAULT_MDC_CORE_CLK_RATE_L2 325000000UL
#define DEFAULT_MDC_CORE_CLK_RATE_L1 214000000UL

/* pix0_clk: 0.60v-213M, 0.65v-324M, 0.70-405M, 0.80-540M */
#define DEFAULT_DSS_PXL0_CLK_RATE_L4 541000000UL
#define DEFAULT_DSS_PXL0_CLK_RATE_L3 406000000UL
#define DEFAULT_DSS_PXL0_CLK_RATE_L2 325000000UL
#define DEFAULT_DSS_PXL0_CLK_RATE_L1 214000000UL

/* mm_axi_clk, aclk_mmbuf */
#define DEFAULT_DSS_MMBUF_CLK_RATE_L4 480000000UL
#define DEFAULT_DSS_MMBUF_CLK_RATE_L3 325000000UL
#define DEFAULT_DSS_MMBUF_CLK_RATE_L2 325000000UL
#define DEFAULT_DSS_MMBUF_CLK_RATE_L1 325000000UL

/* pix1_clk: clk_ldi1_ini */
#define DEFAULT_DSS_PXL1_CLK_RATE_L4 600000000UL
#define DEFAULT_DSS_PXL1_CLK_RATE_L3 415000000UL
#define DEFAULT_DSS_PXL1_CLK_RATE_L2 332000000UL
#define DEFAULT_DSS_PXL1_CLK_RATE_L1 213000000UL

/* dss clk power off */
#define DEFAULT_DSS_CORE_CLK_RATE_POWER_OFF 102000000UL
#define DEFAULT_DSS_MMBUF_CLK_RATE_POWER_OFF 102000000UL
#define DEFAULT_DSS_PXL1_CLK_RATE_POWER_OFF 102000000UL

#define DEFAULT_PCLK_PCTRL_RATE 80000000UL
#define DEFAULT_PCLK_DSS_RATE 232000000UL


#define OVL_LAYER_NUM_MAX 8

/* 10BIT: 0x3ff, 8BIT: 0xFF  RATIO = 0x3ff/0xff */
#define OVL_PATTERN_RATIO 4

/* the channel id which support arsr pre scaler */
#define ARSR_PRE_CH DSS_RCHN_V0

#define DPU_CMDLIST_MAX 16
#define DPU_CMDLIST_IDXS_MAX 0xFFFF  /* 16 cmdlist, 16bit, 1111,1111,1111,1111=0xFFFF */
#define DPU_COPYBIT_CMDLIST_IDXS 0xC000  /* bit14, bit15 */
#define DPU_MEDIACOMMON_CMDLIST_IDXS 0x204  /* bit2, bit9 */
#define DPU_DPP_MAX_SUPPORT_BIT 0x1fff  /* 10 dpp module, 10bit, contrast to enmu dpp_module_idx */
#define DPUFB_PLATFORM_TYPE  (FB_ACCEL_DSSV346 | FB_ACCEL_PLATFORM_TYPE_ASIC)

/* scl */
#define SCF_LINE_BUF 2560

/* DSS global */
#define DSS_GLB_MODULE_CLK_SEL_DEFAULT_VAL 0x00000000

/* DISP global */
#define DSS_MODULE_CORE_CLK_SEL_DEFAULT_VAL 0x00000000

/* AIF */
#define DSS_AIF0_CH_LS 0x40081
#define DSS_AIF1_CH_LS 0x40081
#define DSS_ONLINE_AXI0 AXI_CHN0
#define DSS_ONLINE_AXI1 AXI_CHN1
#define DSS_OFFLINE_AXI0 AXI_CHN0
#define DSS_OFFLINE_AXI1 AXI_CHN1

/* MIF */
#define DSS_MIF_CTRL2_INVAL_SEL3_STRIDE_MASK 0xc

/* AFBCE */
#define DSS_AFBCE_ENC_OS_CFG_DEFAULT_VAL 0x7

#define TUI_SEC_RCH DSS_RCHN_D3

typedef struct dss_mctl_ch_base {
	char __iomem *chn_mutex_base;
	char __iomem *chn_flush_en_base;
	char __iomem *chn_ov_en_base;
	char __iomem *chn_starty_base;
	char __iomem *chn_mod_dbg_base;
} dss_mctl_ch_base_t;

typedef struct dss_arsr2p {
	uint32_t arsr_input_width_height;
	uint32_t arsr_output_width_height;
	uint32_t ihleft;
	uint32_t ihright;
	uint32_t ivtop;
	uint32_t ivbottom;
	uint32_t ihinc;
	uint32_t ivinc;
	uint32_t offset;
	uint32_t mode;
	struct arsr2p_info arsr2p_effect;
	struct arsr2p_info arsr2p_effect_scale_up;
	struct arsr2p_info arsr2p_effect_scale_down;
	uint32_t ihleft1;
	uint32_t ihright1;
	uint32_t ivbottom1;
} dss_arsr2p_t;

typedef struct dirty_region_updt {
	uint32_t dbuf_frm_size;
	uint32_t dbuf_frm_hsize;
	uint32_t dpp_img_size_bef_sr;
	uint32_t dpp_img_size_aft_sr;
	uint32_t dpp_img_size_aft_ifbcsw;
	uint32_t hiace_img_size;
	uint32_t ldi_dpi0_hrz_ctrl0;
	uint32_t ldi_dpi0_hrz_ctrl1;
	uint32_t ldi_dpi0_hrz_ctrl2;
	uint32_t ldi_dpi1_hrz_ctrl0;
	uint32_t ldi_dpi1_hrz_ctrl1;
	uint32_t ldi_dpi1_hrz_ctrl2;
	uint32_t ldi_vrt_ctrl0;
	uint32_t ldi_vrt_ctrl1;
	uint32_t ldi_vrt_ctrl2;
	uint32_t ldi_ctrl;
	uint32_t ifbc_size;
	uint32_t edpi_cmd_size;
	spr_dirty_region spr_ditry;
	dss_arsr1p_t s_arsr1p;
} dirty_region_updt_t;

/*
 * dss module reg
 */
typedef struct dss_module_reg {
	char __iomem *mif_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *aif_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *aif1_ch_base[DSS_CHN_MAX_DEFINE];
	dss_mctl_ch_base_t mctl_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *dma_base[DSS_CHN_MAX_DEFINE];
	char __iomem *dfc_base[DSS_CHN_MAX_DEFINE];
	char __iomem *scl_base[DSS_CHN_MAX_DEFINE];
	char __iomem *scl_lut_base[DSS_CHN_MAX_DEFINE];
	char __iomem *arsr2p_base[DSS_CHN_MAX_DEFINE];
	char __iomem *arsr2p_lut_base[DSS_CHN_MAX_DEFINE];
	char __iomem *post_clip_base[DSS_CHN_MAX_DEFINE];
	char __iomem *pcsc_base[DSS_CHN_MAX_DEFINE];
	char __iomem *csc_base[DSS_CHN_MAX_DEFINE];

	char __iomem *ov_base[DSS_OVL_IDX_MAX];
	char __iomem *mctl_base[DSS_MCTL_IDX_MAX];
	char __iomem *mctl_sys_base;
	char __iomem *smmu_base;
	char __iomem *post_scf_base;
#if defined(CONFIG_RCH_CLD)
	char __iomem *cld_base;
#endif

	dss_mif_t mif[DSS_CHN_MAX_DEFINE];
	dss_aif_t aif[DSS_CHN_MAX_DEFINE];
	dss_aif_t aif1[DSS_CHN_MAX_DEFINE];
	dss_aif_bw_t aif_bw[DSS_CHN_MAX_DEFINE];
	dss_aif_bw_t aif1_bw[DSS_CHN_MAX_DEFINE];
	dss_rdma_t rdma[DSS_CHN_MAX_DEFINE];
	dss_wdma_t wdma[DSS_CHN_MAX_DEFINE];
	dss_dfc_t dfc[DSS_CHN_MAX_DEFINE];
	dss_scl_t scl[DSS_CHN_MAX_DEFINE];
	dss_arsr2p_t arsr2p[DSS_CHN_MAX_DEFINE];
	dss_post_clip_t post_clip[DSS_CHN_MAX_DEFINE];
	dss_csc_t pcsc[DSS_CHN_MAX_DEFINE];
	dss_csc_t csc[DSS_CHN_MAX_DEFINE];
	dss_ovl_t ov[DSS_OVL_IDX_MAX];
	dss_mctl_t mctl[DSS_MCTL_IDX_MAX];
	dss_mctl_ch_t mctl_ch[DSS_CHN_MAX_DEFINE];
	dss_mctl_sys_t mctl_sys;
	dss_smmu_t smmu;
	dirty_region_updt_t dirty_region_updt;
	dss_arsr1p_t post_scf;
#if defined(CONFIG_RCH_CLD)
	struct dss_cld cld;
#endif

	uint8_t mif_used[DSS_CHN_MAX_DEFINE];
	uint8_t aif_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t aif1_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t dma_used[DSS_CHN_MAX_DEFINE];
	uint8_t dfc_used[DSS_CHN_MAX_DEFINE];
	uint8_t scl_used[DSS_CHN_MAX_DEFINE];
	uint8_t arsr2p_used[DSS_CHN_MAX_DEFINE];
	uint8_t arsr2p_effect_used[DSS_CHN_MAX_DEFINE];
	uint8_t post_clip_used[DSS_CHN_MAX_DEFINE];
	uint8_t pcsc_used[DSS_CHN_MAX_DEFINE];
	uint8_t csc_used[DSS_CHN_MAX_DEFINE];
	uint8_t ov_used[DSS_OVL_IDX_MAX];
	uint8_t ch_reg_default_used[DSS_CHN_MAX_DEFINE];
	uint8_t mctl_used[DSS_MCTL_IDX_MAX];
	uint8_t mctl_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t mctl_sys_used;
	uint8_t smmu_used;
	uint8_t dirty_region_updt_used;
	uint8_t post_scf_used;
#if defined(CONFIG_RCH_CLD)
	uint8_t cld_used;
#endif
} dss_module_reg_t;

typedef struct dss_mmbuf_info {
	uint32_t mm_base[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size[DSS_CHN_MAX_DEFINE];
	uint32_t mm_base0_y8[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size0_y8[DSS_CHN_MAX_DEFINE];
	uint32_t mm_base1_c8[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size1_c8[DSS_CHN_MAX_DEFINE];
	uint32_t mm_base2_y2[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size2_y2[DSS_CHN_MAX_DEFINE];
	uint32_t mm_base3_c2[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size3_c2[DSS_CHN_MAX_DEFINE];

	uint8_t mm_used[DSS_CHN_MAX_DEFINE];
} dss_mmbuf_info_t;

#define inheritor_dss_wr_reg(addr, value) outp32(addr, value)


#endif
