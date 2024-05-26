/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _ARPP_V100_H_
#define _ARPP_V100_H_

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_wakeup.h>

/* CRG REG OFFSET */
#define PERI_CLK_ENABLE             0x0000
#define PERI_CLK_DISABLE            0x0004
#define PERI_SRST                   0x0034

/* PCTRL */
#define FUNC0_SPRAM_CTRL            0x0000
#define FUNC0_TPRAM_CTRL            0x0004
#define FUNC1_SPRAM_CTRL            0x0030
#define FUNC1_TPRAM_CTRL            0x0034
#define DW_BUS_TOP_PERI_CG_EN       0x0088
#define DW_BUS_TOP_PERI_STAT        0x008C

#define INTR_CLEAR0                 0x0160
#define INTR_CLEAR1                 0x0174
#define INTR_CLEAR2                 0x0188
#define INTR_CLEAR3                 0x019C
#define INTR_CLEAR4                 0x01B0
#define INTR_CLEAR5                 0x01C4
#define INTR_CLEAR6                 0x01D8
#define INTR_CLEAR7                 0x01EC
#define INTR_CLEAR8                 0x0200
#define INTR_CLEAR9                 0x0214

#define INTR_MASK0                  0x0168
#define INTR_MASK1                  0x017C
#define INTR_MASK2                  0x0190
#define INTR_MASK3                  0x01A4
#define INTR_MASK4                  0x01B8
#define INTR_MASK5                  0x01CC
#define INTR_MASK6                  0x01E0
#define INTR_MASK7                  0x01F4
#define INTR_MASK8                  0x0208
#define INTR_MASK9                  0x021C

/* SCTRL */
#define RCH0_ID_ATTR_S              0x0000
#define RCH1_ID_ATTR_S              0x0004
#define RCH2_ID_ATTR_S              0x0008
#define RCH3_ID_ATTR_S              0x000C
#define RCH4_ID_ATTR_S              0x0010
#define RCH5_ID_ATTR_S              0x0014
#define RCH6_ID_ATTR_S              0x0018
#define RCH7_ID_ATTR_S              0x001C
#define RCH8_ID_ATTR_S              0x0020
#define WCH0_ID_ATTR_S              0x0024
#define WCH1_ID_ATTR_S              0x0028
#define WCH2_ID_ATTR_S              0x002C
#define WCH3_ID_ATTR_S              0x0030
#define WCH4_ID_ATTR_S              0x0034
#define WCH5_ID_ATTR_S              0x0038
#define WCH6_ID_ATTR_S              0x003C
#define WCH7_ID_ATTR_S              0x0040
#define WCH8_ID_ATTR_S              0x0044
#define DW_BUS_TOP_DATA_CG_EN       0x0128
#define DW_BUS_TOP_DATA_STAT        0x012C
#define DW_BUS_TOP_QIC_CG_EN        0x0244

/* TBU */
#define SMMU_TBU_CR                 0x0000
#define SMMU_TBU_CRACK              0x0004
#define SMMU_TBU_SCR                0x1000

/* HWACC CFG REG OFFSET */
#define DEBUG_REG3                  0x0028
#define CRTL_EN0                    0x0120
#define CTRL_DIS0                   0x0124
#define DW_BUS_LITE_CG_EN           0x0408

/* HWACC INI CFG REG OFFSET */
#define DAT0_SRC_ADDR_L             0x0000
#define DAT0_SRC_ADDR_H             0x0004
#define HWACC_INI_SAT               0x0008
#define HWACC_INI_MOD               0x000C
#define HWACC_INI_CFG_EN            0x0010
#define HWACC_CFG0_EN0_PRV_RST      0x0020

/* LBA OFFSET */
#define HWACC_WORK_MODE             0x0020
#define HWACC_PROC_STAT             0x0024
#define LBA_NUM_KF                  0x0028
#define LBA_NUM_FF                  0x002C
#define LBA_NUM_MP                  0x0030
#define LBA_NUM_EDGE                0x0034
#define LBA_EDGE_IDP_DC_NUM         0x0038
#define LBA_EDGE_IDP_FC_NUM         0x003C
#define LBA_EDGE_IDP_SC_NUM         0x0040
#define LBA_EDGE_IDP_NC_NUM         0x0044
#define LBA_NUM_KF_EDGE             0x0048
#define LBA_NUM_PARAMS              0x004C
#define LBA_NUM_EFF_PARAMS          0x0050
#define LBA_NUM_RESIDUALS           0x0054
#define LBA_MAX_NUM_ITER            0x0058
#define LBA_JACOBI_SCALING          0x005C
#define LBA_USE_NONM_STEPS          0x0060
#define LBA_MAX_CON_NONM_STEPS      0x0064
#define LBA_MAX_CON_INVLD_STEPS     0x0068
#define LBA_NRB                     0x006C
#define LBA_NPB                     0x0070
#define LBA_MAX_SS_RETRIES          0x0074
#define LBA_PARAMETER_ADDR          0x0080
#define LBA_PARAM_IDP_DC_ADDR       0x0088
#define LBA_PARAM_IDP_FC_ADDR       0x0090
#define LBA_PARAM_IDP_SC_ADDR       0x0098
#define LBA_PARAM_IDP_NC_ADDR       0x00A0
#define LBA_PARAM_PRV_ADDR          0x00A8
#define LBA_PARAM_BIAS_ADDR         0x00B0
#define LBA_EDGEVERTEX_PRVB_ADDR    0x00B8
#define LBA_EDGEVERTEX_IDP_DC_ADDR  0x00C0
#define LBA_EDGEVERTEX_IDP_FC_ADDR  0x00C8
#define LBA_EDGEVERTEX_IDP_SC_ADDR  0x00D0
#define LBA_EDGEVERTEX_IDP_NC_ADDR  0x00D8
#define LBA_JBR_OFFSET_DC_ADDR      0x00E0
#define LBA_JBR_OFFSET_FC_ADDR      0x00E8
#define LBA_JBR_OFFSET_SC_ADDR      0x00F0
#define LBA_JBR_OFFSET_NC_ADDR      0x00F8
#define LBA_JBL_OFFSET2_FC_ADDR     0x0100
#define LBA_JBL_OFFSET1_SC_ADDR     0x0108
#define LBA_JBL_OFFSET1_NC_ADDR     0x0110
#define LBA_JBL_OFFSET2_NC_ADDR     0x0118
#define LBA_IS_CONST_ADDR           0x0120
#define LBA_OFFSET_ADDR             0x0128
#define LBA_EFFECTIVE_OFFSET_ADDR   0x0130
#define LBA_SWAP_BASE_ADDR          0x0138
#define LBA_FREE_BASE_ADDR          0x0140
#define LBA_OUT_BASE_ADDR           0x0148
#define LBA_LOSS_PRV                0x0150
#define LBA_LOSS_BIAS               0x0158
#define LBA_LOSS_IDP                0x0160
#define LBA_GRAD_TOLERANCE          0x0168
#define LBA_PARAM_TOLERANCE         0x0170
#define LBA_FUNC_TOLERANCE          0x0178
#define LBA_MIN_RELATIVE_DECREASE   0x0180
#define LBA_MIN_TRUST_REGION_RADIUS 0x0188
#define LBA_RADIUS                  0x0190
#define LBA_MAX_RADIUS              0x0198
#define LBA_MIN_DIAGONAL            0x01A0
#define LBA_MAX_DIAGONAL            0x01A8
#define LBA_DECREASE_DIAGONAL       0x01B0
#define LBA_FIXED_COST              0x01B8
#define LBA_CALIB_INT               0x01C0
#define LBA_CALIB_EXT               0x01E0
#define LBA_MSK_HBL_ADDR            0x0240
#define LBA_FC_MP_STA_ADDR          0x0248
#define LBA_SC_MP_STA_ADDR          0x0250
#define LBA_NC_MP_STA_ADDR          0x0258
#define LBA_EDGEVERTEX_RELPOS_ADDR  0x0260
#define LBA_PARAM_PRIOR_ADDR        0x0268
#define LBA_PARAM_VEL_ADDR          0x0270
#define LBA_PARAM_GRAV_ADDR         0x0278
#define LBA_PARAM_RELPOS_ADDR       0x0280
#define LBA_PARAM_ABSPOS_ADDR       0x0288
#define LBA_M2_DC_PCB0_ADDR         0x0290
#define LBA_M2_FC_PCB0_ADDR         0x0298
#define LBA_M2_SC_PCB0_ADDR         0x02A0
#define LBA_M2_NC_PCB0_ADDR         0x02A8
#define LBA_M2_DC_PCBI_ADDR         0x02B0
#define LBA_M2_FC_PCBI_ADDR         0x02B8
#define LBA_M2_SC_PCBI_ADDR         0x02C0
#define LBA_M2_NC_PCBI_ADDR         0x02C8
#define LBA_M2_DC_RCB0_ADDR         0x02D0
#define LBA_M2_FC_RCB0_ADDR         0x02D8
#define LBA_M2_SC_RCB0_ADDR         0x02E0
#define LBA_M2_NC_RCB0_ADDR         0x02E8
#define LBA_M2_DC_RCBI_ADDR         0x02F0
#define LBA_M2_FC_RCBI_ADDR         0x02F8
#define LBA_M2_SC_RCBI_ADDR         0x0300
#define LBA_M2_NC_RCBI_ADDR         0x0308

typedef union {
	unsigned int value;
	struct {
		unsigned int dw_bus_top_data_s1_st  : 1;
		unsigned int dw_bus_top_data_s2_st  : 1;
		unsigned int reserved_0             : 14;
		unsigned int dw_bus_top_data_m1_st  : 1;
		unsigned int dw_bus_top_data_m2_st  : 1;
		unsigned int dw_bus_top_data_m3_st  : 1;
		unsigned int reserved_1             : 13;
	} reg;
} dw_bus_data_state;

typedef union {
	unsigned int value;
	struct {
		unsigned int dw_bus_top_peri_s1_st  : 1;
		unsigned int dw_bus_top_peri_s2_st  : 1;
		unsigned int dw_bus_top_peri_s3_st  : 1;
		unsigned int dw_bus_top_peri_s4_st  : 1;
		unsigned int dw_bus_top_peri_s5_st  : 1;
		unsigned int reserved_0             : 11;
		unsigned int dw_bus_top_peri_m1_st  : 1;
		unsigned int dw_bus_top_peri_m2_st  : 1;
		unsigned int dw_bus_top_peri_m3_st  : 1;
		unsigned int reserved_1             : 13;
	} reg;
} dw_bus_peri_state;

typedef union {
	unsigned int value;
	struct {
		unsigned int reserved_0 : 25;
		unsigned int chip_bits  : 3;
		unsigned int reserved_1 : 4;
	} reg;
} efuse_chip_sel;

typedef union {
	unsigned int value;
	struct {
		unsigned int reserved_0 : 2;
		unsigned int arpp_bits  : 1;
		unsigned int reserved_1 : 29;
	} reg;
} efuse_arpp_sel;

struct arpp_iomem {
	char __iomem *lba0_base;
	char __iomem *hwacc_ini0_cfg_base;
	char __iomem *hwacc1_cfg_base;
	char __iomem *hwacc0_cfg_base;
	char __iomem *dpm_base;
	char __iomem *ahc_s_base;
	char __iomem *ahc_ns_base;
	char __iomem *crg_base;
	char __iomem *tzpc_base;
	char __iomem *pctrl_base;
	char __iomem *sctrl_base;
	char __iomem *asc0_base;
	char __iomem *asc1_base;
	char __iomem *dmmu_base;
	char __iomem *tbu_base;
	char __iomem *actrl_base;
	char __iomem *hwacc_ini1_cfg_base;
};

struct arpp_device {
	/* power */
	struct regulator *media2_supply;
	struct regulator *arpp_supply;
	struct regulator *smmu_tcu_supply;

	/* clock */
	const char *clk_name;
	struct clk *clk;
	int clk_level;
	u32 clk_rate_low;
	u32 clk_rate_medium;
	u32 clk_rate_high;
	u32 clk_rate_turbo;
	u32 clk_rate_lowtemp;
	u32 clk_rate_pd;
	u32 clk_rate_lp;

	/* smmu info */
	uint32_t sid;
	uint32_t ssid;

	/* IPC interrupt */
	uint32_t ahc_normal_irq;
	uint32_t ahc_error_irq;

	/* reg info */
	struct arpp_iomem io_info;

	/* pm */
	struct wakeup_source *wake_lock;
};

enum arpp_clk_level {
	CLK_LEVEL_LOW = 0,
	CLK_LEVEL_MEDIUM,
	CLK_LEVEL_HIGH,
	CLK_LEVEL_TURBO,
	CLK_LEVEL_LP,
};

int is_arpp_available(void);
int get_regulator(struct arpp_device *arpp_dev, struct device *dev);
int put_regulator(struct arpp_device *arpp_dev);
int get_clk(struct arpp_device *arpp_dev, struct device *dev, struct device_node *dev_node);
int put_clk(struct arpp_device *arpp_dev, struct device *dev);
int power_up(struct arpp_device *arpp_dev);
int power_down(struct arpp_device *device);
int get_all_registers(struct arpp_device *device, struct device_node *dev_node);
void put_all_registers(struct arpp_device *device);
int get_irqs(struct arpp_device *arpp_dev, struct device_node *dev_node);
int get_sid_ssid(struct arpp_device *arpp_dev, struct device_node *dev_node);
void init_and_reset(struct arpp_device *arpp_dev);
void open_auto_lp_mode(struct arpp_device *arpp_dev);
void mask_and_clear_isr(struct arpp_device *arpp_dev);
int change_clk_level(struct arpp_device *arpp_dev, int level);

#endif /* _ARPP_V100_H_ */
