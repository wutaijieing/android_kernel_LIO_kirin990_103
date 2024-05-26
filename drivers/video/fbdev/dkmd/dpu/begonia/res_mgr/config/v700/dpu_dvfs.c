/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <soc_pmctrl_interface.h>
#include <soc_media1_crg_interface.h>
#include <dpu/soc_dpu_define.h>

#include "dkmd_log.h"
#include "dkmd_object.h"
#include "dpu_config_utils.h"

#define SW_DVFS_OPT_NUM  8
#define HW_DVFS_OPT_NUM  32

enum {
	OPT_VOLT_UP_INDEX = 0,
	OPT_VOLT_DOWN_INDEX = 1,
	OPT_VOLT_MAX_INDEX,
};

typedef union {
	uint16_t value;
	/* data include END, WAIT_CNT, WAIT_CHG, VOL_CFG, CLK_SW, CLK_DIV, PLL_EN, PLL_GT, NOP */
	struct dvfs_instruction {
		uint32_t op     : 4;     /* bit[3:0]  : OP code for dvfs */
		uint32_t halt   : 1;     /* bit[4]    : halt flag for dvfs */
		uint32_t instr  : 11;    /* bit[15:5] : instruction code for dvfs */
	} cmd;
} dpu_dvfs_cmd;

struct dvfs_vote_info {
	uint16_t vote_vol;
	uint16_t vote_clk_sel;
	uint16_t vote_clk_div;
};

static uint64_t s_dpu_core_rate_tbl_v110[] = {
	DPU_CORE_FREQ_OFF, /* L0_POWER_OFF */
	DPU_CORE_FREQ0, /* L1_060V */
	DPU_CORE_FREQ1, /* L2_065V */
	DPU_CORE_FREQ2, /* L3_070V */
	DPU_CORE_FREQ3  /* L4_080V */
};

static uint64_t dpu_config_get_core_rate(uint32_t level)
{
	if (level >= ARRAY_SIZE(s_dpu_core_rate_tbl_v110))
		return s_dpu_core_rate_tbl_v110[4];

	return s_dpu_core_rate_tbl_v110[level];
}

/*
 * clk sw pll 0b0001: ppll2_b 0b0010: ppll0 0b0100: ppll1 0b1000: ppll2
 * vol_cfg 0b0011: 0.8v 0b0010: 0.7v 0b0001: 0.65v 0b0000: 0.6v
 */
static struct dvfs_vote_info g_vote_info[] = {
	/* level0 0.6  - 104M, ppll0, 16div  */
	{
		.vote_vol = 0b0000,
		.vote_clk_sel = 0b0010,
		.vote_clk_div = 0b1111,
	},
	/* level1 0.6  - 278M, ppll0, 6div  */
	{
		.vote_vol = 0b0000,
		.vote_clk_sel = 0b0010,
		.vote_clk_div = 0b0101,
	},
	/* level2 0.65  - 418M, ppll0, 4div  */
	{
		.vote_vol = 0b0001,
		.vote_clk_sel = 0b0010,
		.vote_clk_div = 0b0011,
	},
	/* level3 0.7  - 480M, ppll1, 3div  */
	{
		.vote_vol = 0b0010,
		.vote_clk_sel = 0b0100,
		.vote_clk_div = 0b0010,
	},
	/* level4 0.8  - 640M, ppll2_b, 2div  */
	{
		.vote_vol = 0b0011,
		.vote_clk_sel = 0b0001,
		.vote_clk_div = 0b0001,
	},
};

static void dpu_hw_dvfs_init(char __iomem *pmctrl_base)
{
	SOC_PMCTRL_DSS_DVFS_CNT_CFG1_UNION sctrl_cfg1;
	SOC_PMCTRL_DSS_DVFS_CNT_CFG2_UNION sctrl_cfg2;
	SOC_PMCTRL_DSS_DVFS_CNT_CFG3_UNION sctrl_cfg3;
	SOC_PMCTRL_APB_DSS_DVFS_HW_EN_UNION dvfs_en_cfg;

	sctrl_cfg1.reg.apb_pclk_cnt_us = 0x54;
	sctrl_cfg1.reg.apb_vol_stable_cnt = 0x1;
	sctrl_cfg1.reg.apb_sw_stable_cnt = 0x1;
	sctrl_cfg1.reg.apb_sw_ack_cnt = 0x6;
	sctrl_cfg1.reg.apb_div_stable_cnt = 0x1;

	outp32(SOC_PMCTRL_DSS_DVFS_CNT_CFG1_ADDR(pmctrl_base), sctrl_cfg1.value);

	sctrl_cfg2.reg.apb_div_ack_cnt = 0x6;
	sctrl_cfg2.reg.apb_pll_lock_mode = 0x1;
	sctrl_cfg2.reg.apb_lock_time = 0x32;
	sctrl_cfg2.reg.apb_pll_gt_stable_cnt = 0x1;
	outp32(SOC_PMCTRL_DSS_DVFS_CNT_CFG2_ADDR(pmctrl_base), sctrl_cfg2.value);

	sctrl_cfg3.reg.apb_reload_instr_stable_cnt = 0x1E;
	outp32(SOC_PMCTRL_DSS_DVFS_CNT_CFG3_ADDR(pmctrl_base), sctrl_cfg3.value);

	dvfs_en_cfg.reg.apb_hw_ch_fps_cnt = 0x2;
	dvfs_en_cfg.reg.apb_hw_ch_fps_cnt_dss_bypass = 0x0;
	dvfs_en_cfg.reg.apb_hw_ch_fps_cnt_dss2qice_bypass = 0x0;
	dvfs_en_cfg.reg.apb_hw_ch_fps_cnt_dss2ddr_bypass = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), dvfs_en_cfg.value);
}

static void dpu_sw_channel_dvfs_init(char __iomem *pmctrl_base)
{
	SOC_PMCTRL_HW_DSS_DVFS_APB_SW_CFG_UNION sw_cfg;
	SOC_PMCTRL_HW_DSS_DVFS_APB_DIV_CFG_UNION div_cfg;
	SOC_PMCTRL_APB_DSS_DVFS_PLL_CFG_UNION pll_cfg;
	SOC_PMCTRL_HW_DSS_DVFS_APB_VF_CFG_UNION vf_cfg;

	sw_cfg.reg.apb_sw_init = 0x1;
	sw_cfg.reg.apb_sw = 0b0010; /* select pll0 */
	outp32(SOC_PMCTRL_HW_DSS_DVFS_APB_SW_CFG_ADDR(pmctrl_base), sw_cfg.value);
	vf_cfg.reg.apb_sw_chg_init_pulse = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_APB_VF_CFG_ADDR(pmctrl_base), vf_cfg.value);

	div_cfg.reg.apb_div_init = 0x1;
	div_cfg.reg.apb_div = 0x5; /* default 278M div 0x5 */
	outp32(SOC_PMCTRL_HW_DSS_DVFS_APB_DIV_CFG_ADDR(pmctrl_base), div_cfg.value);
	vf_cfg.reg.apb_div_chg_init_pulse = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_APB_VF_CFG_ADDR(pmctrl_base), vf_cfg.value);

	pll_cfg.reg.apb_pll_gt3 = 0x0; /* ppll2 disable   */
	pll_cfg.reg.apb_pll_en3 = 0x0;
	pll_cfg.reg.apb_pll_gt2 = 0x0; /* ppll1 disable   */
	pll_cfg.reg.apb_pll_en2 = 0x0;
	pll_cfg.reg.apb_pll_gt1 = 0x1; /* ppll0 enable    */
	pll_cfg.reg.apb_pll_en1 = 0x1;
	pll_cfg.reg.apb_pll_gt0 = 0x0; /* ppll2_b disable */
	pll_cfg.reg.apb_pll_en0 = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_PLL_CFG_ADDR(pmctrl_base), pll_cfg.value | (0xff << 16));

	vf_cfg.reg.apb_pll_gt3_init = 0x1;
	vf_cfg.reg.apb_pll_en3_init = 0x1;
	vf_cfg.reg.apb_pll_gt2_init = 0x1;
	vf_cfg.reg.apb_pll_en2_init = 0x1;
	vf_cfg.reg.apb_pll_gt1_init = 0x1;
	vf_cfg.reg.apb_pll_en1_init = 0x1;
	vf_cfg.reg.apb_pll_gt0_init = 0x1;
	vf_cfg.reg.apb_pll_en0_init = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_APB_VF_CFG_ADDR(pmctrl_base), vf_cfg.value);
}

static void dpu_dvfs_sw_channel_cfg(char __iomem *pmctrl_base, dpu_dvfs_cmd *cmd, int cmd_size, int opt_type)
{
	int i = 0;
	uint32_t value = 0;
	SOC_PMCTRL_APB_DSS_DVFS_SF_EN_UNION sf_en;
	SOC_PMCTRL_HW_DSS_DVFS_SF_CH_UNION sf_ch;
	SOC_PMCTRL_APB_DSS_DVFS_HW_EN_UNION hw_en_cfg;

	/*
	 * OPT_VOLT_UP_INDEX: vol --> pll_en&pll_gt --> clk_sw --> clk_div --> pll_en&pll_gt --> end
	 * OPT_VOLT_DOWN_INDEX: pll_en&pll_gt --> clk_div --> clk_sw --> pll_en&pll_gt --> vol--> end
	 */
	int opt_table[OPT_VOLT_MAX_INDEX][SW_DVFS_OPT_NUM] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 1, 2, 4, 3, 5, 6, 0, 7 }
	};

	/* could be deleted when test ok */
	sf_en.value = 0;
	sf_en.reg.apb_sf_instr_lock = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_SF_EN_ADDR(pmctrl_base), sf_en.value | (0x1 << 16));

	/* could be deleted when test ok */
	sf_ch.value = 0;
	sf_ch.reg.apb_sf_instr_clr = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_SF_CH_ADDR(pmctrl_base), sf_ch.value);

	/* sw command config */
	for (i = 0; i < SW_DVFS_OPT_NUM; i += 2) {
		dpu_pr_debug("high - low: %#x, %#x",
				cmd[opt_table[opt_type][i]].value, cmd[opt_table[opt_type][i + 1]].value);

		outp32(SOC_PMCTRL_APB_SF_INSTR_LIST_ADDR(pmctrl_base, i / 2),
				(cmd[opt_table[opt_type][i]].value << 16) | cmd[opt_table[opt_type][i + 1]].value);

		dpu_pr_debug("dvfs cmd-%d: %#x --> %#x \n", i/2, SOC_PMCTRL_APB_SF_INSTR_LIST_ADDR(0xFFB81000, i / 2),
				inp32(SOC_PMCTRL_APB_SF_INSTR_LIST_ADDR(pmctrl_base, i / 2)));
	}

	/* could be deleted when test ok */
	sf_en.value = 0;
	sf_en.reg.apb_sf_instr_lock = 0x1;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_SF_EN_ADDR(pmctrl_base), sf_en.value | (0x1 << 16));

	/* v120 don't need config */
	hw_en_cfg.value = 0;
	hw_en_cfg.reg.apb_hw_dvfs_enable = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), hw_en_cfg.value | (0x1 << 16));

	sf_ch.value = 0;
	sf_ch.reg.apb_sf_dvfs_en = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_SF_CH_ADDR(pmctrl_base), sf_ch.value);

	i = 0;
	while ((i++ < 30) && ((value & BIT(16)) != BIT(16))) {
		value = inp32(SOC_PMCTRL_HW_DSS_DVFS_SF_RD_ADDR(pmctrl_base));
		dpu_pr_debug("apb_rd_intr_sf_dvfs_raw_stat = %#x", value);
		udelay(10);
	}

	sf_ch.value = 0;
	sf_ch.reg.apb_sf_instr_clr = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_SF_CH_ADDR(pmctrl_base), sf_ch.value);
}

void dpu_sw_dvfs_enable(void)
{
	char __iomem *pmctrl_base = dpu_config_get_ip_base(DISP_IP_BASE_PMCTRL);
	char __iomem *media1_crg = dpu_config_get_ip_base(DISP_IP_BASE_MEDIA1_CRG);

	/* hw init */
	dpu_hw_dvfs_init(pmctrl_base);

	/* enable media1 crg dvfs en */
	outp32(SOC_MEDIA1_CRG_DSS_DVFS_ADDR(media1_crg), 0x1);

	/* init select pll0 and div 0xf for power off by 104M */
	dpu_sw_channel_dvfs_init(pmctrl_base);
}

static void dpu_sw_dvfs_vote_level(uint32_t vote_level)
{
	int opt_type = OPT_VOLT_UP_INDEX;
	static uint32_t last_vote_level = 0;
	char __iomem *pmctrl_base = dpu_config_get_ip_base(DISP_IP_BASE_PMCTRL);

	/* When the frequency is increased, the voltage is increased first,
	 * and then the frequency is decreased.
	 * When the PLL is switched, the DIV is switched first,
	 * and the SW is switched first when the frequency is increased.
	 */
	dpu_dvfs_cmd sf_dvfs_instr[] = {
		/* vol crg */
		{ .cmd.op = 0b0011, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_vol },

		/* pll en && pll gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0,
			.cmd.instr = (g_vote_info[vote_level].vote_clk_sel | g_vote_info[last_vote_level].vote_clk_sel) },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0,
			.cmd.instr = (g_vote_info[vote_level].vote_clk_sel | g_vote_info[last_vote_level].vote_clk_sel) },

		/* clk sw && clk div */
		{ .cmd.op = 0b0100, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },
		{ .cmd.op = 0b0101, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_div },

		/* pll en && pll gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },

		/* end */
		{ .cmd.op = 0b1111, .cmd.halt = 0b0, .cmd.instr = 0b0000 },
	};

	opt_type = vote_level > last_vote_level ? OPT_VOLT_UP_INDEX : OPT_VOLT_DOWN_INDEX;

	dpu_dvfs_sw_channel_cfg(pmctrl_base, sf_dvfs_instr, ARRAY_SIZE(sf_dvfs_instr), opt_type);

	last_vote_level = vote_level;
}

static void dpu_dvfs_hw_channel_cfg(dpu_dvfs_cmd *cmd, int cmd_size)
{
	int i = 0;
	char __iomem *pmctrl_base = dpu_config_get_ip_base(DISP_IP_BASE_PMCTRL);
	SOC_PMCTRL_HW_DSS_DVFS_HW_CH_UNION hw_ch;
	SOC_PMCTRL_APB_DSS_DVFS_HW_EN_UNION hw_en_cfg;

	/* could be deleted when test ok */
	hw_en_cfg.value = 0;
	hw_en_cfg.reg.apb_hw_instr_lock = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), hw_en_cfg.value | (0b10 << 16));

	/* could be deleted when test ok */
	hw_ch.value = 0;
	hw_ch.reg.apb_hw_instr_clr = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_HW_CH_ADDR(pmctrl_base), hw_ch.value);

	/* sw command config */
	for (i = 0; i < cmd_size; i += 2) {
		dpu_pr_debug("high - low: %#x, %#x", cmd[i].value, cmd[i + 1].value);

		outp32(SOC_PMCTRL_APB_HW_INSTR_LIST_ADDR(pmctrl_base, i / 2),
				(cmd[i].value << 16) | cmd[i + 1].value);

		dpu_pr_debug("dvfs cmd-%d: %#x --> %#x \n", i/2, SOC_PMCTRL_APB_SF_INSTR_LIST_ADDR(0xFFB81000, i / 2),
				inp32(SOC_PMCTRL_APB_SF_INSTR_LIST_ADDR(pmctrl_base, i / 2)));
	}

	hw_en_cfg.value = 0;
	hw_en_cfg.reg.apb_hw_instr_lock = 0x1;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), hw_en_cfg.value | (0b10 << 16));

	hw_en_cfg.value = 0;
	hw_en_cfg.reg.apb_hw_dvfs_enable = 0x0;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), hw_en_cfg.value | (0b01 << 16));

	hw_en_cfg.value = 0;
	hw_en_cfg.reg.apb_hw_dvfs_enable = 0x1;
	outp32(SOC_PMCTRL_APB_DSS_DVFS_HW_EN_ADDR(pmctrl_base), hw_en_cfg.value | (0b01 << 16));

	hw_ch.value = 0;
	hw_ch.reg.apb_hw_flush_pls_init = 0x1;
	outp32(SOC_PMCTRL_HW_DSS_DVFS_HW_CH_ADDR(pmctrl_base), hw_ch.value);
}

static void dpu_pmctrl_dvfs_vote_trajectory_build(uint32_t idle_level, uint32_t vote_level,
		uint32_t time_wait, uint32_t active_time, uint32_t last_idle_level)
{
	/* wait porch time --> vote freq --> wait active time --> vote idle freq --> end  */
	dpu_dvfs_cmd hw_dvfs_instr[] = {
		/* time_wait */
		{ .cmd.op = 0b0001, .cmd.halt = 0b0, .cmd.instr = (time_wait >> 11) },
		{ .cmd.op = 0b0000, .cmd.halt = 0b0, .cmd.instr = (time_wait & 0x7ff) },

		/* vol_cfg 0b0011: 0.8v 0b0010: 0.7v 0b0001: 0.65v 0b0000: 0.6v */
		{ .cmd.op = 0b0011, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_vol },
		/* pll_en && pll_gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0,
			.cmd.instr = g_vote_info[vote_level].vote_clk_sel | g_vote_info[last_idle_level].vote_clk_sel },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0,
			.cmd.instr = g_vote_info[vote_level].vote_clk_sel | g_vote_info[last_idle_level].vote_clk_sel },

		/* clk_div & clk sw */
		{ .cmd.op = 0b0101, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_div },
		{ .cmd.op = 0b0100, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },

		/* pll_en && pll_gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0, .cmd.instr = g_vote_info[vote_level].vote_clk_sel },

		/* nop */
		{ .cmd.op = 0b1110, .cmd.halt = 0b0, .cmd.instr = 0b0000 },

		/* active_time */
		{ .cmd.op = 0b0001, .cmd.halt = 0b0, .cmd.instr = (active_time >> 11) },
		{ .cmd.op = 0b0000, .cmd.halt = 0b0, .cmd.instr = (active_time & 0x7ff) },

		/* pll_en && pll_gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0,
			.cmd.instr = g_vote_info[vote_level].vote_clk_sel | g_vote_info[idle_level].vote_clk_sel },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0,
			.cmd.instr = g_vote_info[vote_level].vote_clk_sel | g_vote_info[idle_level].vote_clk_sel },

		/* clk_div & clk sw */
		{ .cmd.op = 0b0101, .cmd.halt = 0b0, .cmd.instr = g_vote_info[idle_level].vote_clk_div },
		{ .cmd.op = 0b0100, .cmd.halt = 0b0, .cmd.instr = g_vote_info[idle_level].vote_clk_sel },

		/* pll en & pll gt */
		{ .cmd.op = 0b0110, .cmd.halt = 0b0, .cmd.instr = g_vote_info[idle_level].vote_clk_sel },
		{ .cmd.op = 0b0111, .cmd.halt = 0b0, .cmd.instr = g_vote_info[idle_level].vote_clk_sel },

		/* vol_cfg */
		{ .cmd.op = 0b0011, .cmd.halt = 0b0, .cmd.instr = g_vote_info[idle_level].vote_vol },
		/* nop */
		{ .cmd.op = 0b1110, .cmd.halt = 0b0, .cmd.instr = 0b0000 },

		/* nop */
		{ .cmd.op = 0b1110, .cmd.halt = 0b0, .cmd.instr = 0b0000 },
		/* end */
		{ .cmd.op = 0b1111, .cmd.halt = 0b0, .cmd.instr = 0b0000 }
	};

	dpu_dvfs_hw_channel_cfg(hw_dvfs_instr, ARRAY_SIZE(hw_dvfs_instr));
}

/* for test */
void dpu_pmctrl_dvfs_vote_level(uint32_t vote_level)
{
	dpu_check_and_no_retval((vote_level > 4), err, "don't support perf level%d!", vote_level);

	dpu_pr_info("config clk rate=%llu", dpu_config_get_core_rate(vote_level));

	dpu_pmctrl_dvfs_vote_trajectory_build(1, vote_level, 1, 15000, 1);
}
EXPORT_SYMBOL(dpu_pmctrl_dvfs_vote_level);

bool is_dpu_dvfs_enable(void)
{
	return true;
}
EXPORT_SYMBOL(is_dpu_dvfs_enable);

void dpu_intra_frame_dvfs_vote(struct intra_frame_dvfs_info *info)
{
	static uint32_t last_idle_level = 0;
	uint32_t time_wait = info->time_wait;
	uint32_t perf_level = info->perf_level;
	uint32_t active_time = info->active_time;
	uint32_t idle_level = info->idle_level;

	dpu_check_and_no_retval((perf_level > 4), err, "don't support perf level%d!", perf_level);

	dpu_pr_info("config clk rate=%llu", dpu_config_get_core_rate(perf_level));

	dpu_pmctrl_dvfs_vote_trajectory_build(idle_level, perf_level, time_wait, active_time, last_idle_level);

	last_idle_level = idle_level;
}

void dpu_legacy_inter_frame_dvfs_vote(uint32_t level)
{
	dpu_check_and_no_retval((level > 4), err, "don't support perf level%d!", level);

	dpu_pr_info("config clk rate=%llu", dpu_config_get_core_rate(level));

	if (dpu_config_enable_pmctrl_dvfs())
		dpu_sw_dvfs_vote_level(level);
	else
		dpu_config_dvfs_vote_exec(dpu_config_get_core_rate(level));
}
EXPORT_SYMBOL(dpu_legacy_inter_frame_dvfs_vote);

void dpu_enable_core_clock(void)
{
	if (g_dpu_config_data.clk_gate_edc == NULL)
		return;

	if (dpu_config_enable_pmctrl_dvfs()) {
		if (clk_prepare_enable(g_dpu_config_data.clk_gate_edc) != 0)
			dpu_pr_err("enable clk_gate_edc failed!");

		dpu_sw_dvfs_enable();
	} else {
		dpu_legacy_inter_frame_dvfs_vote(1);
		if (clk_prepare_enable(g_dpu_config_data.clk_gate_edc) != 0)
			dpu_pr_err("enable clk_gate_edc failed!");
	}
}

void dpu_disable_core_clock(void)
{
	dpu_config_dvfs_vote_exec(dpu_config_get_core_rate(0));
	dpu_legacy_inter_frame_dvfs_vote(0);

	if (g_dpu_config_data.clk_gate_edc != NULL)
		clk_disable_unprepare(g_dpu_config_data.clk_gate_edc);
}

