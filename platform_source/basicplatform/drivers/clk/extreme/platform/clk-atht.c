/*
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "../clk.h"
#include "clk-atht.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>

static const struct fixed_rate_clock fixed_clks[] = {
	{ CLKIN_SYS, "clkin_sys", NULL, 0, 38400000, "clkin_sys" },
	{ CLKIN_REF, "clkin_ref", NULL, 0, 32764, "clkin_ref" },
	{ CLK_FLL_SRC, "clk_fll_src", NULL, 0, 368633447, "clk_fll_src" },
	{ CLK_SPLL, "clk_spll", NULL, 0, 983040000, "clk_spll" },
	{ CLK_FNPLL1, "clk_fnpll1", NULL, 0, 1200000000, "clk_fnpll1" },
	{ CLK_MODEM_BASE, "clk_modem_base", NULL, 0, 49152000, "clk_modem_base" },
	{ PCLK, "apb_pclk", NULL, 0, 20000000, "apb_pclk" },
	{ CLK_UART0_DBG, "uart0clk_dbg", NULL, 0, 19200000, "uart0clk_dbg" },
	{ CLK_UART6, "uart6clk", NULL, 0, 19200000, "uart6clk" },
	{ OSC32K, "osc32khz", NULL, 0, 32764, "osc32khz" },
	{ OSC19M, "osc19mhz", NULL, 0, 19200000, "osc19mhz" },
	{ CLK_480M, "clk_480m", NULL, 0, 480000000, "clk_480m" },
	{ CLK_INVALID, "clk_invalid", NULL, 0, 10000000, "clk_invalid" },
	{ CLK_FPGA_1P92, "clk_fpga_1p92", NULL, 0, 1920000, "clk_fpga_1p92" },
	{ CLK_FPGA_2M, "clk_fpga_2m", NULL, 0, 2000000, "clk_fpga_2m" },
	{ CLK_FPGA_10M, "clk_fpga_10m", NULL, 0, 10000000, "clk_fpga_10m" },
	{ CLK_FPGA_19M, "clk_fpga_19m", NULL, 0, 19200000, "clk_fpga_19m" },
	{ CLK_FPGA_20M, "clk_fpga_20m", NULL, 0, 20000000, "clk_fpga_20m" },
	{ CLK_FPGA_24M, "clk_fpga_24m", NULL, 0, 24000000, "clk_fpga_24m" },
	{ CLK_FPGA_26M, "clk_fpga_26m", NULL, 0, 26000000, "clk_fpga_26m" },
	{ CLK_FPGA_27M, "clk_fpga_27m", NULL, 0, 27000000, "clk_fpga_27m" },
	{ CLK_FPGA_32M, "clk_fpga_32m", NULL, 0, 32000000, "clk_fpga_32m" },
	{ CLK_FPGA_40M, "clk_fpga_40m", NULL, 0, 40000000, "clk_fpga_40m" },
	{ CLK_FPGA_48M, "clk_fpga_48m", NULL, 0, 48000000, "clk_fpga_48m" },
	{ CLK_FPGA_50M, "clk_fpga_50m", NULL, 0, 50000000, "clk_fpga_50m" },
	{ CLK_FPGA_57M, "clk_fpga_57m", NULL, 0, 57000000, "clk_fpga_57m" },
	{ CLK_FPGA_60M, "clk_fpga_60m", NULL, 0, 60000000, "clk_fpga_60m" },
	{ CLK_FPGA_64M, "clk_fpga_64m", NULL, 0, 64000000, "clk_fpga_64m" },
	{ CLK_FPGA_80M, "clk_fpga_80m", NULL, 0, 80000000, "clk_fpga_80m" },
	{ CLK_FPGA_100M, "clk_fpga_100m", NULL, 0, 100000000, "clk_fpga_100m" },
	{ CLK_FPGA_160M, "clk_fpga_160m", NULL, 0, 160000000, "clk_fpga_160m" },
};

static const struct fixed_factor_clock fixed_factor_clks[] = {
	{ CLK_PPLL_EPS, "clk_ppll_eps", "clk_spll", 1, 1, 0, "clk_ppll_eps" },
	{ CLK_SYS_INI, "clk_sys_ini", "clkin_sys", 1, 2, 0, "clk_sys_ini" },
	{ CLK_DIV_SYSBUS, "div_sysbus_pll", "clk_fnpll1", 1, 7, 0, "div_sysbus_pll" },
	{ CLK_DIV_HSDTBUS, "div_hsdtbus_pll", "clk_spll", 1, 5, 0, "div_hsdtbus_pll" },
	{ PCLK_GATE_WD0, "pclk_wd0", "clk_spll", 1, 10, 0, "pclk_wd0" },
	{ CLK_DIV_AOBUS, "clk_div_aobus", "clk_spll", 1, 22, 0, "clk_div_aobus" },
	{ CLK_USB2_DIV, "clkdiv_usb2", "sc_gt_clk_usb2_div", 1, 2, 0, "clkdiv_usb2" },
	{ ATCLK, "clk_at", "clkin_sys", 1, 1, 0, "clk_at" },
	{ CLK_GATE_DMA_IOMCU, "clk_dma_iomcu", "clk_fll_src", 1, 4, 0, "clk_dma_iomcu" },
	{ CLK_GATE_PFA_TFT, "clk_pfa_tft", "clk_sys_ini", 1, 1, 0, "clk_pfa_tft" },
	{ ACLK_GATE_DSS, "aclk_dss", "clk_sys_ini", 1, 1, 0, "aclk_dss" },
	{ PCLK_GATE_DSI0, "pclk_dsi0", "clk_sys_ini", 1, 1, 0, "pclk_dsi0" },
	{ CLK_GATE_HIFACE, "clk_hiface", "clk_sys_ini", 1, 1, 0, "clk_hiface" },
	{ CLK_DIV_A53HPM, "clk_a53hpm_div", "clk_a53hpm_gt", 1, 15, 0, "clk_a53hpm_div" },
	{ CLK_DIV_320M, "sc_div_320m", "gt_clk_320m_pll", 1, 3, 0, "sc_div_320m" },
	{ CLK_FACTOR_UART0, "clk_uart0_fac", "clkmux_uart0", 1, 1, 0, "clk_uart0_fac" },
	{ CLK_GATE_BLPWM, "clk_blpwm", "clk_fll_src", 1, 2, 0, "clk_blpwm" },
	{ CLK_SYSCNT_DIV, "clk_syscnt_div", "clk_sys_ini", 1, 10, 0, "clk_syscnt_div" },
	{ CLK_GATE_GPS_REF, "clk_gps_ref", "clk_sys_ini", 1, 1, 0, "clk_gps_ref" },
	{ CLK_MUX_GPS_REF, "clkmux_gps_ref", "clk_sys_ini", 1, 1, 0, "clkmux_gps_ref" },
	{ CLK_GATE_MDM2GPS0, "clk_mdm2gps0", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps0" },
	{ CLK_GATE_MDM2GPS1, "clk_mdm2gps1", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps1" },
	{ CLK_GATE_MDM2GPS2, "clk_mdm2gps2", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps2" },
	{ VENC_VOLT_HOLD, "venc_volt_hold", "peri_volt_hold", 1, 1, 0, "venc_volt_hold" },
	{ VDEC_VOLT_HOLD, "vdec_volt_hold", "peri_volt_hold", 1, 1, 0, "vdec_volt_hold" },
	{ EDC_VOLT_HOLD, "edc_volt_hold", "peri_volt_hold", 1, 1, 0, "edc_volt_hold" },
	{ CLK_ISP_SNCLK_FAC, "clk_fac_ispsn", "clk_ispsn_gt", 1, 14, 0, "clk_fac_ispsn" },
	{ CLK_GATE_ISPFUNC2, "clk_ispfunc2freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc2freq" },
	{ CLK_GATE_ISPFUNC3, "clk_ispfunc3freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc3freq" },
	{ CLK_GATE_ISPFUNC4, "clk_ispfunc4freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc4freq" },
	{ CLK_GATE_ISPFUNC5, "clk_ispfunc5freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc5freq" },
	{ CLK_GATE_PFA_REF, "clk_pfa_ref", "clk_sys_ini", 1, 1, 0, "clk_pfa_ref" },
	{ HCLK_GATE_PFA, "hclk_pfa", "clk_sys_ini", 1, 1, 0, "hclk_pfa" },
	{ CLK_GATE_I2C0, "clk_i2c0", "clk_fll_src", 1, 2, 0, "clk_i2c0" },
	{ CLK_GATE_I2C1, "clk_i2c1", "clk_fll_src", 1, 2, 0, "clk_i2c1" },
	{ CLK_GATE_I2C2, "clk_i2c2", "clk_fll_src", 1, 2, 0, "clk_i2c2" },
	{ CLK_FAC_180M, "clkfac_180m", "clk_spll", 1, 8, 0, "clkfac_180m" },
	{ CLK_GATE_IOMCU_PERI0, "clk_gate_peri0", "clk_spll", 1, 1, 0, "clk_gate_peri0" },
	{ CLK_GATE_UART3, "clk_uart3", "clk_gate_peri0", 1, 9, 0, "clk_uart3" },
	{ AUTODIV_ISP_DVFS, "autodiv_isp_dvfs", "autodiv_sysbus", 1, 1, 0, "autodiv_isp_dvfs" },
};

static const struct gate_clock aoncrg_gate_clks[] = {
	{ CLK_GATE_TIMER5, "clk_timer5", "clk_div_aobus", 0x000, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5" },
	{ PCLK_GATE_SYSCNT, "pclk_syscnt", "clk_div_aobus", 0x000, 0x4, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_syscnt" },
	{ CLK_GATE_SYSCNT, "clk_syscnt", "clkmux_syscnt", 0x000, 0x8, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_syscnt" },
};

static const char * const clkmux_syscnt_p [] = { "clk_syscnt_div", "clkin_ref" };
static const struct mux_clock aoncrg_mux_clks[] = {
	{ CLKMUX_SYSCNT, "clkmux_syscnt", clkmux_syscnt_p,
		ARRAY_SIZE(clkmux_syscnt_p), 0x304, 0x1, CLK_MUX_HIWORD_MASK, "clkmux_syscnt" },
};

#ifdef CONFIG_FSM_PPLL_VOTE
static const struct fsm_pll_clock litecrg_pll_clks[] = {
	{ CLK_GATE_FNPLL1, "clk_ap_fnpll1", "clk_fnpll1", 0xA,
		{ 0x0D8, 0 }, { 0x60C, 9 }, "clk_ap_fnpll1" },
};
#else
static const struct pll_clock litecrg_pll_clks[] = {
	{ CLK_GATE_FNPLL1, "clk_ap_fnpll1", "clk_fnpll1", 0xA,
		{ 0x0D8, 0 }, { 0x238, 0 }, { 0x248, 0 }, { 0x0CC, 7 }, 0xE, 0, "clk_ap_fnpll1" },
};
#endif

static const struct gate_clock litecrg_gate_clks[] = {
	{ CLK_GATE_PPLL0_MEDIA, "clk_ppll0_media", "clk_spll", 0x010, 0x2000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_media" },
	{ CLK_GATE_FNPLL1_MEDIA, "clk_fnpll1_media", "clk_ap_fnpll1", 0x010, 0x1000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fnpll1_media" },
	{ CLK_GATE_ULPLL_MEDIA, "clk_ulpll_media", "clk_fll_src", 0x010, 0x800000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ulpll_media" },
	{ CLK_GATE_ODT_ACPU, "clk_odt_acpu", "clk_spll", 0x010, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_odt_acpu" },
	{ CLK_GATE_ODT_DEFLATE, "clk_odt_deflat", "clk_spll", 0, 0x0, 0,
		"clk_odt_acpu", 0, {0}, {0}, 0, 0, 0, "clk_odt_deflat" },
	{ CLK_GATE_TCLK_ODT, "tclk_odt", "clk_sys_ini", 0, 0x0, 0,
		"clk_odt_acpu", 0, {0}, {0}, 0, 0, 0, "tclk_odt" },
	{ CLK_GATE_EMMC_HF_SD, "clk_emmc_hf_sd", "clkmux_emmc_hf_sd", 0x010, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_emmc_hf_sd" },
	{ CLK_ASP_CODEC, "clk_asp_codec", "sel_asp_codec", 0x020, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_codec" },
	{ CLK_GATE_ASP_SUBSYS, "clk_asp_subsys", "clk_asp_pll_sel", 0x020, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_subsys" },
	{ CLK_GATE_ASP_TCXO, "clk_asp_tcxo", "clk_sys_ini", 0x000, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_tcxo" },
	{ CLK_GATE_TXDPHY0_CFG, "clk_txdphy0_cfg", "clk_sys_ini", 0x020, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_cfg" },
	{ CLK_GATE_TXDPHY0_REF, "clk_txdphy0_ref", "clk_sys_ini", 0x020, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_ref" },
};

static const struct scgt_clock litecrg_scgt_clks[] = {
	{ CLKANDGT_EMMC_HF_SD_FLL, "clkgt_emmc_hf_sd_fll", "clk_fll_src",
		0x340, 0, CLK_GATE_HIWORD_MASK, "clkgt_emmc_hf_sd_fll" },
	{ CLKANDGT_EMMC_HF_SD_PLL, "clkgt_emmc_hf_sd_pll", "clk_spll",
		0x340, 1, CLK_GATE_HIWORD_MASK, "clkgt_emmc_hf_sd_pll" },
	{ CLKGT_ASP_CODEC, "clkgt_asp_codec", "clk_spll",
		0x30C, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_asp_codec" },
	{ CLKGT_ASP_CODEC_FLL, "clkgt_asp_codec_fll", "clk_fll_src",
		0x30C, 0, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_asp_codec_fll" },
};

static const struct div_clock litecrg_div_clks[] = {
	{ CLK_EMMC_HF_SD_FLL_DIV, "clk_emmc_hf_sd_fll_div", "clkgt_emmc_hf_sd_fll",
		0x340, 0xfc, 64, 1, 0, "clk_emmc_hf_sd_fll_div" },
	{ CLK_EMMC_HF_SD_PLL_DIV, "clk_emmc_hf_sd_pll_div", "clkgt_emmc_hf_sd_pll",
		0x340, 0x3f00, 64, 1, 0, "clk_emmc_hf_sd_pll_div" },
	{ CLKDIV_ASP_CODEC, "clkdiv_asp_codec", "clkgt_asp_codec",
		0x30C, 0x3f00, 64, 1, 0, "clkdiv_asp_codec" },
	{ CLKDIV_ASP_CODEC_FLL, "clkdiv_asp_codec_fll", "clkgt_asp_codec_fll",
		0x30C, 0xfc, 64, 1, 0, "clkdiv_asp_codec_fll" },
};

static const char * const clk_mux_emmc_hf_sd_p [] = { "clk_emmc_hf_sd_fll_div", "clk_emmc_hf_sd_pll_div" };
static const char * const clk_mux_asp_codec_p [] = { "clkdiv_asp_codec_fll", "clkdiv_asp_codec" };
static const char * const clk_mux_asp_pll_p [] = {
		"clk_invalid", "clk_spll", "clk_fll_src", "clk_invalid",
		"clk_ap_fnpll1", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_sys_ini", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock litecrg_mux_clks[] = {
	{ CLK_MUX_EMMC_HF_SD, "clkmux_emmc_hf_sd", clk_mux_emmc_hf_sd_p,
		ARRAY_SIZE(clk_mux_emmc_hf_sd_p), 0x340, 0x4000, CLK_MUX_HIWORD_MASK, "clkmux_emmc_hf_sd" },
	{ CLK_MUX_ASP_CODEC, "sel_asp_codec", clk_mux_asp_codec_p,
		ARRAY_SIZE(clk_mux_asp_codec_p), 0x30C, 0x4000, CLK_MUX_HIWORD_MASK, "sel_asp_codec" },
	{ CLK_MUX_ASP_PLL, "clk_asp_pll_sel", clk_mux_asp_pll_p,
		ARRAY_SIZE(clk_mux_asp_pll_p), 0x308, 0x7800, CLK_MUX_HIWORD_MASK, "clk_asp_pll_sel" },
};

static const struct gate_clock crgctrl_gate_clks[] = {
	{ CLK_GATE_PPLL0_M2, "clk_ppll0_m2", "clk_spll", 0x410, 0x40, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_m2" },
	{ CLK_GATE_FNPLL1_M2, "clk_fnpll1_m2", "clk_ap_fnpll1", 0x410, 0x80, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fnpll1_m2" },
	{ PCLK_GATE_DSS, "pclk_dss", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dss" },
	{ PCLK_GATE_IOC, "pclk_ioc", "clkin_sys", 0x020, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ioc" },
	{ ACLK_GATE_PERF_STAT, "aclk_perf_stat", "clk_spll", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_perf_stat" },
	{ PCLK_GATE_PERF_STAT, "pclk_perf_stat", "clk_spll", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_perf_stat" },
	{ CLK_GATE_PERF_STAT, "clk_perf_stat", "clk_perf_div", 0x040, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat" },
	{ CLK_GATE_PERF_CTRL, "clk_perf_ctrl", "sc_div_320m", 0x040, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_ctrl" },
	{ CLK_GATE_DMAC, "clk_dmac", "clk_spll", 0x030, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dmac" },
	{ CLK_GATE_TIME_STAMP, "clk_time_stamp", "clk_timestp_div", 0x000, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_time_stamp" },
	{ CLK_GATE_SDIO_HF_SD, "clk_sdio_hf_sd", "clkmux_sdio_hf_sd", 0x410, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sdio_hf_sd" },
	{ CLK_GATE_UART1, "clk_uart1", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart1" },
	{ CLK_GATE_UART4, "clk_uart4", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart4" },
	{ PCLK_GATE_UART1, "pclk_uart1", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart1" },
	{ PCLK_GATE_UART4, "pclk_uart4", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart4" },
	{ CLK_GATE_UART2, "clk_uart2", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart2" },
	{ CLK_GATE_UART5, "clk_uart5", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart5" },
	{ PCLK_GATE_UART2, "pclk_uart2", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart2" },
	{ PCLK_GATE_UART5, "pclk_uart5", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart5" },
	{ PCLK_GATE_UART0, "pclk_uart0", "clkmux_uart0", 0x020, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart0" },
	{ CLK_GATE_I2C2_ACPU, "clk_i2c2_acpu", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c2_acpu" },
	{ CLK_GATE_I2C3, "clk_i2c3", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c3" },
	{ CLK_GATE_I2C4, "clk_i2c4", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c4" },
	{ CLK_GATE_I2C6_ACPU, "clk_i2c6_acpu", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c6_acpu" },
	{ CLK_GATE_I2C7, "clk_i2c7", "clkmux_i2c", 0x000, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c7" },
	{ PCLK_GATE_I2C2, "pclk_i2c2", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c2" },
	{ PCLK_GATE_I2C3, "pclk_i2c3", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c3" },
	{ PCLK_GATE_I2C4, "pclk_i2c4", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c4" },
	{ PCLK_GATE_I2C6_ACPU, "pclk_i2c6_acpu", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c6_acpu" },
	{ PCLK_GATE_I2C7, "pclk_i2c7", "clkmux_i2c", 0x000, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c7" },
	{ CLK_GATE_SPI1, "clk_spi1", "clkmux_spi1", 0x000, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi1" },
	{ CLK_GATE_SPI2, "clk_spi2", "clkmux_spi2", 0x000, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi2" },
	{ CLK_GATE_SPI, "clk_spi3", "clkmux_spi3", 0x000, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi3" },
	{ PCLK_GATE_PCTRL, "pclk_pctrl", "clk_spll", 0x020, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pctrl" },
	{ PERI_VOLT_HOLD, "peri_volt_hold", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0 , 0 , 0}, {0 , 1 , 2 , 3}, 3, 18, 0, "peri_volt_hold" },
	{ PERI_VOLT_MIDDLE, "peri_volt_middle", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0 , 0}, {0 , 1 , 2}, 2, 22, 0, "peri_volt_middle" },
	{ PERI_VOLT_LOW, "peri_volt_low", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0}, {0 , 1}, 1, 28, 0, "peri_volt_low" },
	{ CLK_GATE_MEDIA2_TCXO, "clk_media2_tcxo", "clk_sys_ini", 0x050, 0x40000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media2_tcxo" },
	{ CLK_GATE_ISP_I2C_MEDIA, "clk_isp_i2c_media", "clk_div_isp_i2c", 0x030, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i2c_media" },
	{ CLK_GATE_ISP_SYS, "clk_isp_sys", "clk_media2_tcxo", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_sys" },
	{ CLK_GATE_ISP_I3C, "clk_isp_i3c", "clk_sys_ini", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i3c" },
	{ CLK_GATE_ISP_SNCLK0, "clk_isp_snclk0", "clk_mux_ispsn0", 0x050, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk0" },
	{ CLK_GATE_ISP_SNCLK1, "clk_isp_snclk1", "clk_mux_ispsn1", 0x050, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk1" },
	{ CLK_GATE_RXDPHY0_CFG, "clk_rxdphy0_cfg", "clk_rxdphy_cfg", 0x050, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy0_cfg" },
	{ CLK_GATE_RXDPHY1_CFG, "clk_rxdphy1_cfg", "clk_rxdphy_cfg", 0x050, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy1_cfg" },
	{ CLK_GATE_RXDPHY_CFG, "clk_rxdphy_cfg", "clk_rxdcfg_mux", 0x050, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy_cfg" },
	{ PCLK_GATE_LOADMONITOR, "pclk_loadmonitor", "clk_spll", 0x020, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor" },
	{ CLK_GATE_LOADMONITOR, "clk_loadmonitor", "clk_loadmonitor_div", 0x020, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor" },
	{ CLK_GATE_PFA, "clk_pfa", "clk_spll", 0x410, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pfa" },
	{ ACLK_GATE_DRA, "aclk_dra", "clk_spll", 0x470, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_dra" },
	{ CLK_GATE_ATDIV_CFG, "clk_atdiv_cfg", "autodiv_cfgbus", 0xE00, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_cfg" },
	{ CLK_GATE_ATDIV_SYS, "clk_atdiv_sys", "div_sysbus_pll", 0xE20, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_sys" },
};

static const struct scgt_clock crgctrl_scgt_clks[] = {
	{ AUTODIV_SYSBUS, "autodiv_sysbus", "div_sysbus_pll",
		0x750, 5, CLK_GATE_HIWORD_MASK, "autodiv_sysbus" },
	{ CLK_PERF_DIV_GT, "clk_perf_gt", "sc_div_320m",
		0x0F0, 3, CLK_GATE_HIWORD_MASK, "clk_perf_gt" },
	{ CLK_GATE_TIME_STAMP_GT, "clk_timestp_gt", "clk_sys_ini",
		0x0F0, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_timestp_gt" },
	{ CLKANDGT_SDIO_HF_SD_FLL, "clkgt_sdio_hf_sd_fll", "clk_fll_src",
		0x704, 6, CLK_GATE_HIWORD_MASK, "clkgt_sdio_hf_sd_fll" },
	{ CLKANDGT_SDIO_HF_SD_PLL, "clkgt_sdio_hf_sd_pll", "clk_spll",
		0x704, 5, CLK_GATE_HIWORD_MASK, "clkgt_sdio_hf_sd_pll" },
	{ CLK_A53HPM_ANDGT, "clk_a53hpm_gt", "clk_a53hpm_mux",
		0x0F4, 7, CLK_GATE_HIWORD_MASK, "clk_a53hpm_gt" },
	{ CLK_320M_PLL_GT, "gt_clk_320m_pll", "clk_spll",
		0x0F8, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "gt_clk_320m_pll" },
	{ CLK_ANDGT_UART0, "clkgt_uart0", "sc_div_320m",
		0x0F4, 9, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uart0" },
	{ CLK_ANDGT_I2C, "clkgt_i2c", "sc_div_320m",
		0x0AC, 0, CLK_GATE_HIWORD_MASK, "clkgt_i2c" },
	{ CLK_ANDGT_SPI1, "clkgt_spi1", "sc_div_320m",
		0x0A8, 11, CLK_GATE_HIWORD_MASK, "clkgt_spi1" },
	{ CLK_ANDGT_SPI2, "clkgt_spi2", "sc_div_320m",
		0x0A8, 12, CLK_GATE_HIWORD_MASK, "clkgt_spi2" },
	{ CLK_ANDGT_SPI3, "clkgt_spi3", "sc_div_320m",
		0x0A8, 13, CLK_GATE_HIWORD_MASK, "clkgt_spi3" },
	{ CLK_ANDGT_PTP, "clk_ptp_gt", "sc_div_320m",
		0x0F8, 5, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_ptp_gt" },
	{ CLK_GT_ISP_I2C, "clk_gt_isp_i2c", "sc_div_320m",
		0x0F8, 4, CLK_GATE_HIWORD_MASK, "clk_gt_isp_i2c" },
	{ CLK_ISP_SNCLK_ANGT, "clk_ispsn_gt", "sc_div_320m",
		0x108, 2, CLK_GATE_HIWORD_MASK, "clk_ispsn_gt" },
	{ CLKANDGT_RXDPHY_CFG_FLL, "clkgt_rxdphy_cfg_fll", "clk_fll_src",
		0x0FC, 10, CLK_GATE_HIWORD_MASK, "clkgt_rxdphy_cfg_fll" },
	{ CLK_GT_LOADMONITOR, "clk_loadmonitor_gt", "sc_div_320m",
		0x0F0, 5, CLK_GATE_HIWORD_MASK, "clk_loadmonitor_gt" },
	{ AUTODIV_CFGBUS, "autodiv_cfgbus", "autodiv_sysbus",
		0x750, 5, CLK_GATE_HIWORD_MASK, "autodiv_cfgbus" },
};

static const struct div_clock crgctrl_div_clks[] = {
	{ CLK_DIV_PERF_STAT, "clk_perf_div", "clk_perf_gt",
		0x0D0, 0xf000, 16, 1, 0, "clk_perf_div" },
	{ CLK_DIV_TIME_STAMP, "clk_timestp_div", "clk_timestp_gt",
		0x0B8, 0x70, 8, 1, 0, "clk_timestp_div" },
	{ CLK_SDIO_HF_SD_FLL_DIV, "clk_sdio_hf_sd_fll_div", "clkgt_sdio_hf_sd_fll",
		0x714, 0x3f, 64, 1, 0, "clk_sdio_hf_sd_fll_div" },
	{ CLK_SDIO_HF_SD_PLL_DIV, "clk_sdio_hf_sd_pll_div", "clkgt_sdio_hf_sd_pll",
		0x718, 0xfc00, 64, 1, 0, "clk_sdio_hf_sd_pll_div" },
	{ CLK_DIV_UART0, "clkdiv_uart0", "clkgt_uart0",
		0x0B0, 0xf0, 16, 1, 0, "clkdiv_uart0" },
	{ CLK_DIV_I2C, "clkdiv_i2c", "clkgt_i2c",
		0x0AC, 0xf00, 16, 1, 0, "clkdiv_i2c" },
	{ CLK_DIV_SPI1, "clkdiv_spi1", "clkgt_spi1",
		0x0A8, 0x78, 16, 1, 0, "clkdiv_spi1" },
	{ CLK_DIV_SPI2, "clkdiv_spi2", "clkgt_spi2",
		0x0A8, 0x780, 16, 1, 0, "clkdiv_spi2" },
	{ CLK_DIV_SPI3, "clkdiv_spi3", "clkgt_spi3",
		0x0AC, 0xf0, 64, 1, 0, "clkdiv_spi3" },
	{ CLK_DIV_PTP, "clk_ptp_div", "clk_ptp_gt",
		0x0D8, 0xf, 16, 1, 0, "clk_ptp_div" },
	{ CLK_DIV_ISP_I2C, "clk_div_isp_i2c", "clk_gt_isp_i2c",
		0x0B8, 0x780, 16, 1, 0, "clk_div_isp_i2c" },
	{ CLK_ISP_SNCLK_DIV0, "clk_div_ispsn0", "clk_fac_ispsn",
		0x108, 0x3, 4, 1, 0, "clk_div_ispsn0" },
	{ CLK_ISP_SNCLK_DIV1, "clk_div_ispsn1", "clk_fac_ispsn",
		0x10C, 0xc000, 4, 1, 0, "clk_div_ispsn1" },
	{ CLK_RXDPHY_FLL_DIV, "clk_rxdphy_fll_div", "clkgt_rxdphy_cfg_fll",
		0x0DC, 0x3, 4, 1, 0, "clk_rxdphy_fll_div" },
	{ CLK_DIV_LOADMONITOR, "clk_loadmonitor_div", "clk_loadmonitor_gt",
		0x0B8, 0xc000, 4, 1, 0, "clk_loadmonitor_div" },
};

static const char * const clk_mux_sdio_hf_sd_p [] = { "clk_sdio_hf_sd_pll_div", "clk_sdio_hf_sd_fll_div" };
static const char * const clk_mux_a53hpm_p [] = { "clk_spll", "clk_ap_fnpll1" };
static const char * const clk_mux_uart0_p [] = { "clk_sys_ini", "clkdiv_uart0" };
static const char * const clk_mux_i2c_p [] = { "clk_sys_ini", "clkdiv_i2c" };
static const char * const clk_mux_spi1_p [] = { "clk_sys_ini", "clkdiv_spi1" };
static const char * const clk_mux_spi2_p [] = { "clk_sys_ini", "clkdiv_spi2" };
static const char * const clk_mux_spi3_p [] = { "clk_sys_ini", "clkdiv_spi3" };
static const char * const clk_isp_snclk_mux0_p [] = { "clk_sys_ini", "clk_div_ispsn0", "clkin_sys", "clkin_sys" };
static const char * const clk_isp_snclk_mux1_p [] = { "clk_sys_ini", "clk_div_ispsn1", "clkin_sys", "clkin_sys" };
static const char * const clk_mux_rxdphy_cfg_p [] = { "clk_a53hpm_div", "clk_a53hpm_div", "clk_sys_ini", "clk_rxdphy_fll_div" };
static const struct mux_clock crgctrl_mux_clks[] = {
	{ CLK_MUX_SDIO_HF_SD, "clkmux_sdio_hf_sd", clk_mux_sdio_hf_sd_p,
		ARRAY_SIZE(clk_mux_sdio_hf_sd_p), 0x704, 0x80, CLK_MUX_HIWORD_MASK, "clkmux_sdio_hf_sd" },
	{ CLK_MUX_A53HPM, "clk_a53hpm_mux", clk_mux_a53hpm_p,
		ARRAY_SIZE(clk_mux_a53hpm_p), 0x0D4, 0x200, CLK_MUX_HIWORD_MASK, "clk_a53hpm_mux" },
	{ CLK_MUX_UART0, "clkmux_uart0", clk_mux_uart0_p,
		ARRAY_SIZE(clk_mux_uart0_p), 0x0AC, 0x4, CLK_MUX_HIWORD_MASK, "clkmux_uart0" },
	{ CLK_MUX_I2C, "clkmux_i2c", clk_mux_i2c_p,
		ARRAY_SIZE(clk_mux_i2c_p), 0x0AC, 0x8, CLK_MUX_HIWORD_MASK, "clkmux_i2c" },
	{ CLK_MUX_SPI1, "clkmux_spi1", clk_mux_spi1_p,
		ARRAY_SIZE(clk_mux_spi1_p), 0x0A8, 0x1, CLK_MUX_HIWORD_MASK, "clkmux_spi1" },
	{ CLK_MUX_SPI2, "clkmux_spi2", clk_mux_spi2_p,
		ARRAY_SIZE(clk_mux_spi2_p), 0x0A8, 0x2, CLK_MUX_HIWORD_MASK, "clkmux_spi2" },
	{ CLK_MUX_SPI3, "clkmux_spi3", clk_mux_spi3_p,
		ARRAY_SIZE(clk_mux_spi3_p), 0x0A8, 0x4, CLK_MUX_HIWORD_MASK, "clkmux_spi3" },
	{ CLK_ISP_SNCLK_MUX0, "clk_mux_ispsn0", clk_isp_snclk_mux0_p,
		ARRAY_SIZE(clk_isp_snclk_mux0_p), 0x108, 0x18, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn0" },
	{ CLK_ISP_SNCLK_MUX1, "clk_mux_ispsn1", clk_isp_snclk_mux1_p,
		ARRAY_SIZE(clk_isp_snclk_mux1_p), 0x10C, 0x300, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn1" },
	{ CLK_MUX_RXDPHY_CFG, "clk_rxdcfg_mux", clk_mux_rxdphy_cfg_p,
		ARRAY_SIZE(clk_mux_rxdphy_cfg_p), 0x0DC, 0xc, CLK_MUX_HIWORD_MASK, "clk_rxdcfg_mux" },
};

static const struct gate_clock hsdtcrg_gate_clks[] = {
	{ PCLK_USB2_SYSCTRL, "pclk_usb2_sysctrl", "div_hsdtbus_pll", 0x000, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2_sysctrl" },
	{ PCLK_USB2PHY, "pclk_usb2phy", "div_hsdtbus_pll", 0x000, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy" },
	{ HCLK_USB2DRD, "hclk_usb2drd", "div_hsdtbus_pll", 0x000, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_usb2drd" },
	{ CLK_USB2_REF, "clk_usb2_ref", "clkdiv_usb2", 0x000, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_ref" },
	{ CLK_USB2PHY_REF, "clk_usb2phy_ref", "clk_mux_usb2phy_ref", 0x000, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref" },
	{ CLK_GATE_SDIO_32K, "clk_sdio_32k", "clkin_ref", 0x000, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sdio_32k" },
	{ ACLK_GATE_SDIO, "aclk_sdio", "clk_spll", 0x000, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_sdio" },
};

static const struct scgt_clock hsdtcrg_scgt_clks[] = {
	{ CLK_ANDGT_USB2_DIV, "sc_gt_clk_usb2_div", "clkin_sys",
		0x300, 0, CLK_GATE_HIWORD_MASK, "sc_gt_clk_usb2_div" },
};

static const char * const clk_mux_usb2phy_ref_p [] = { "clkdiv_usb2", "clkin_sys" };
static const struct mux_clock hsdtcrg_mux_clks[] = {
	{ CLK_MUX_USB2PHY_REF, "clk_mux_usb2phy_ref", clk_mux_usb2phy_ref_p,
		ARRAY_SIZE(clk_mux_usb2phy_ref_p), 0x304, 0x1, CLK_MUX_HIWORD_MASK, "clk_mux_usb2phy_ref" },
};

static const struct div_clock media1crg_div_clks[] = {
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_vivobus_gt",
		0x060, 0x3f, 64, 1, 0, "clk_vivobus_div" },
	{ CLK_DIV_EDC0, "clk_edc0_div", "clk_edc0_gt",
		0x070, 0x3f, 64, 1, 0, "clk_edc0_div" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clkgt_vdec",
		0x064, 0x3f, 64, 1, 0, "clkdiv_vdec" },
};

static const char * const clk_mux_vivobus_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_fnpll1_media", "clk_invalid",
		"clk_ulpll_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_edc0_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_fnpll1_media", "clk_invalid",
		"clk_ulpll_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_vdec_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_fnpll1_media", "clk_invalid",
		"clk_ulpll_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media1crg_mux_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", clk_mux_vivobus_p,
		ARRAY_SIZE(clk_mux_vivobus_p), 0x060, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vivobus_mux" },
	{ CLK_MUX_EDC0, "sel_edc0_pll", clk_mux_edc0_p,
		ARRAY_SIZE(clk_mux_edc0_p), 0x070, 0x3c0, CLK_MUX_HIWORD_MASK, "sel_edc0_pll" },
	{ CLK_MUX_VDEC, "clkmux_vdec", clk_mux_vdec_p,
		ARRAY_SIZE(clk_mux_vdec_p), 0x064, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_vdec" },
};

static const struct gate_clock media1crg_gate_clks[] = {
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_vivobus_div", 0x010, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vivobus" },
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", "clk_edc0_div", 0x800, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_edc0freq" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clkdiv_vdec", 0x000, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vdecfreq" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_vivobus_div", 0x000, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vivo" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clkdiv_vdec", 0x000, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vdec" },
};

static const struct scgt_clock media1crg_scgt_clks[] = {
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_vivobus_mux",
		0x06C, 0, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vivobus_gt" },
	{ CLK_ANDGT_EDC0, "clk_edc0_gt", "sel_edc0_pll",
		0x06C, 3, CLK_GATE_HIWORD_MASK, "clk_edc0_gt" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clkmux_vdec",
		0x06C, 1, CLK_GATE_HIWORD_MASK, "clkgt_vdec" },
};

static const struct div_clock media2crg_div_clks[] = {
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_vcodbus_gt",
		0x08C, 0x3f, 64, 1, 0, "clk_vcodbus_div" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clkgt_venc",
		0x088, 0x3f, 64, 1, 0, "clkdiv_venc" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_ispcpu_gt",
		0x084, 0x3f, 64, 1, 0, "clk_ispcpu_div" },
	{ CLKDIV_ISPFUNC, "clkdiv_ispfunc", "clkgt_ispfunc",
		0x080, 0x3f, 64, 1, 0, "clkdiv_ispfunc" },
};

static const char * const clk_mux_vcodecbus_p [] = {
		"clk_invalid", "clk_ppll0_m2", "clk_fnpll1_m2", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_venc_p [] = {
		"clk_invalid", "clk_ppll0_m2", "clk_fnpll1_m2", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispi2c_p [] = { "clk_media2_tcxo", "clk_isp_i2c_media" };
static const char * const clk_mux_ispcpu_p [] = {
		"clk_invalid", "clk_ppll0_m2", "clk_fnpll1_m2", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll0_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_fnpll1_media", "clk_invalid",
		"clk_ppll0_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll0_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media2crg_mux_clks[] = {
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", clk_mux_vcodecbus_p,
		ARRAY_SIZE(clk_mux_vcodecbus_p), 0x08C, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vcodbus_mux" },
	{ CLK_MUX_VENC, "clkmux_venc", clk_mux_venc_p,
		ARRAY_SIZE(clk_mux_venc_p), 0x088, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_venc" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", clk_mux_ispi2c_p,
		ARRAY_SIZE(clk_mux_ispi2c_p), 0x080, 0x400, CLK_MUX_HIWORD_MASK, "clk_ispi2c_mux" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", clk_mux_ispcpu_p,
		ARRAY_SIZE(clk_mux_ispcpu_p), 0x084, 0x3c0, CLK_MUX_HIWORD_MASK, "sel_ispcpu" },
	{ CLK_MUX_ISPFUNC, "clk_mux_ispfunc", clk_mux_ispfunc_p,
		ARRAY_SIZE(clk_mux_ispfunc_p), 0x080, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_mux_ispfunc" },
};

static const struct gate_clock media2crg_gate_clks[] = {
	{ ACLK_GATE_ISP, "aclk_isp", "clk_sys_ini", 0x900, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp" },
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_vcodbus_div", 0x000, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clkdiv_venc", 0x000, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vencfreq" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_ispi2c_mux", 0x900, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispi2c" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_ispcpu_div", 0x900, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispcpu" },
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clkdiv_ispfunc", 0x900, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_vcodbus_div", 0x000, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clkdiv_venc", 0x000, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_venc" },
};

static const struct scgt_clock media2crg_scgt_clks[] = {
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_vcodbus_mux",
		0x090, 0, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vcodbus_gt" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clkmux_venc",
		0x090, 1, CLK_GATE_HIWORD_MASK, "clkgt_venc" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "sel_ispcpu",
		0x090, 2, CLK_GATE_HIWORD_MASK, "clk_ispcpu_gt" },
	{ CLKANDGT_ISPFUNC, "clkgt_ispfunc", "clk_mux_ispfunc",
		0x090, 3, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc" },
};

static const struct xfreq_clock xfreq_clks[] = {
	{ CLK_CLUSTER0, "cpu-cluster.0", 0, 0, 0, 0x41C, { 0x0001030A, 0x0 }, { 0x0001020A, 0x0 }, "cpu-cluster.0" },
	{ CLK_CLUSTER1, "cpu-cluster.1", 1, 0, 0, 0x41C, { 0x0002030A, 0x0 }, { 0x0002020A, 0x0 }, "cpu-cluster.1" },
	{ CLK_G3D0, "clk_g3d", 2, 0, 0, 0x41C, { 0x0003030A, 0x0 }, { 0x0003020A, 0x0 }, "clk_g3d" },
	{ CLK_DDRC_FREQ0, "clk_ddrc_freq", 3, 0, 0, 0x41C, { 0x00040309, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_freq" },
	{ CLK_DDRC_MAX0, "clk_ddrc_max", 5, 1, 0x8000, 0x250, { 0x00040308, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_max" },
	{ CLK_DDRC_MIN0, "clk_ddrc_min", 4, 1, 0x8000, 0x270, { 0x00040309, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_min" },
};

static const struct pmu_clock pmu_clks[] = {
	{ CLK_PMU32KA, "clk_pmu32ka", "clkin_ref", 0x04B, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32ka" },
	{ CLK_PMU32KB, "clk_pmu32kb", "clkin_ref", 0x04A, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kb" },
	{ CLK_PMU32KC, "clk_pmu32kc", "clkin_ref", 0x049, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kc" },
	{ CLK_PMUAUDIOCLK, "clk_pmuaudioclk", "clk_sys_ini", 0x046, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmuaudioclk" },
};

static const struct dvfs_clock dvfs_clks[] = {
	{ CLK_GATE_EDC0, "clk_edc0", "clk_edc0freq", 20, -1, 3, 1,
		{ 238000, 400000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_edc0" },
	{ CLK_GATE_VDEC, "clk_vdec", "clk_vdecfreq", 21, -1, 3, 1,
		{ 207500, 332000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_vdec" },
	{ CLK_GATE_VENC, "clk_venc", "clk_vencfreq", 24, -1, 3, 1,
		{ 277000, 415000, 640000 }, { 0, 1, 2, 3 }, 640000, 1, "clk_venc" },
	{ CLK_GATE_ISPFUNC, "clk_ispfunc", "clk_ispfuncfreq", 26, -1, 3, 1,
		{ 207500, 332000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_ispfunc" },
};

static const struct fixed_factor_clock media1crg_stub_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", "clk_sys_ini", 1, 1, 0, "clk_vivobus_mux" },
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_sys_ini", 1, 1, 0, "clk_vivobus_gt" },
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_sys_ini", 1, 1, 0, "clk_vivobus_div" },
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_sys_ini", 1, 1, 0, "clk_vivobus" },
	{ CLK_MUX_EDC0, "sel_edc0_pll", "clk_sys_ini", 1, 1, 0, "sel_edc0_pll" },
	{ CLK_ANDGT_EDC0, "clk_edc0_gt", "clk_sys_ini", 1, 1, 0, "clk_edc0_gt" },
	{ CLK_DIV_EDC0, "clk_edc0_div", "clk_sys_ini", 1, 1, 0, "clk_edc0_div" },
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", "clk_sys_ini", 1, 1, 0, "clk_edc0freq" },
	{ CLK_MUX_VDEC, "clkmux_vdec", "clk_sys_ini", 1, 1, 0, "clkmux_vdec" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clk_sys_ini", 1, 1, 0, "clkgt_vdec" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clk_sys_ini", 1, 1, 0, "clkdiv_vdec" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clk_sys_ini", 1, 1, 0, "clk_vdecfreq" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vivo" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vdec" },
};

static const struct fixed_factor_clock media2crg_stub_clks[] = {
	{ ACLK_GATE_ISP, "aclk_isp", "clk_sys_ini", 1, 1, 0, "aclk_isp" },
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_sys_ini", 1, 1, 0, "clk_vcodecbus" },
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_div" },
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_gt" },
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_mux" },
	{ CLK_MUX_VENC, "clkmux_venc", "clk_sys_ini", 1, 1, 0, "clkmux_venc" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clk_sys_ini", 1, 1, 0, "clkgt_venc" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clk_sys_ini", 1, 1, 0, "clkdiv_venc" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clk_sys_ini", 1, 1, 0, "clk_vencfreq" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", "clk_sys_ini", 1, 1, 0, "clk_ispi2c_mux" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_sys_ini", 1, 1, 0, "clk_ispi2c" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", "clk_sys_ini", 1, 1, 0, "sel_ispcpu" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_gt" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_div" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_sys_ini", 1, 1, 0, "clk_ispcpu" },
	{ CLK_MUX_ISPFUNC, "clk_mux_ispfunc", "clk_sys_ini", 1, 1, 0, "clk_mux_ispfunc" },
	{ CLKANDGT_ISPFUNC, "clkgt_ispfunc", "clk_sys_ini", 1, 1, 0, "clkgt_ispfunc" },
	{ CLKDIV_ISPFUNC, "clkdiv_ispfunc", "clk_sys_ini", 1, 1, 0, "clkdiv_ispfunc" },
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clk_sys_ini", 1, 1, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clk_sys_ini", 1, 1, 0, "clk_atdiv_venc" },
};

static void clk_aoncrg_init(struct device_node *np)
{
	struct clock_data *clk_aoncrg_data = NULL;

	int nr = ARRAY_SIZE(aoncrg_mux_clks) +
		ARRAY_SIZE(aoncrg_gate_clks);

	clk_aoncrg_data = clk_init(np, nr);
	if (!clk_aoncrg_data)
		return;

	plat_clk_register_mux(aoncrg_mux_clks, ARRAY_SIZE(aoncrg_mux_clks), clk_aoncrg_data);

	plat_clk_register_gate(aoncrg_gate_clks, ARRAY_SIZE(aoncrg_gate_clks), clk_aoncrg_data);
}

CLK_OF_DECLARE_DRIVER(clk_aoncrg, "hisilicon,clk-extreme-aoncrg", clk_aoncrg_init);

static void clk_litecrg_init(struct device_node *np)
{
	struct clock_data *clk_litecrg_data = NULL;
	int nr = ARRAY_SIZE(litecrg_pll_clks) +
		ARRAY_SIZE(litecrg_scgt_clks) +
		ARRAY_SIZE(litecrg_div_clks) +
		ARRAY_SIZE(litecrg_mux_clks) +
		ARRAY_SIZE(litecrg_gate_clks);

	clk_litecrg_data = clk_init(np, nr);
	if (!clk_litecrg_data)
		return;

#ifdef CONFIG_FSM_PPLL_VOTE
	plat_clk_register_fsm_pll(litecrg_pll_clks, ARRAY_SIZE(litecrg_pll_clks), clk_litecrg_data);
#else
	plat_clk_register_pll(litecrg_pll_clks, ARRAY_SIZE(litecrg_pll_clks), clk_litecrg_data);
#endif

	plat_clk_register_scgt(litecrg_scgt_clks, ARRAY_SIZE(litecrg_scgt_clks), clk_litecrg_data);

	plat_clk_register_divider(litecrg_div_clks, ARRAY_SIZE(litecrg_div_clks), clk_litecrg_data);

	plat_clk_register_mux(litecrg_mux_clks, ARRAY_SIZE(litecrg_mux_clks), clk_litecrg_data);

	plat_clk_register_gate(litecrg_gate_clks, ARRAY_SIZE(litecrg_gate_clks), clk_litecrg_data);
}

CLK_OF_DECLARE_DRIVER(clk_litecrg, "hisilicon,clk-extreme-litecrg", clk_litecrg_init);


static void clk_crgctrl_init(struct device_node *np)
{
	struct clock_data *clk_crgctrl_data = NULL;
	int nr = ARRAY_SIZE(fixed_clks) +
		ARRAY_SIZE(crgctrl_scgt_clks) +
		ARRAY_SIZE(fixed_factor_clks) +
		ARRAY_SIZE(crgctrl_div_clks) +
		ARRAY_SIZE(crgctrl_mux_clks) +
		ARRAY_SIZE(crgctrl_gate_clks);

	clk_crgctrl_data = clk_init(np, nr);
	if (!clk_crgctrl_data)
		return;

	plat_clk_register_fixed_rate(fixed_clks, ARRAY_SIZE(fixed_clks), clk_crgctrl_data);

	plat_clk_register_scgt(crgctrl_scgt_clks, ARRAY_SIZE(crgctrl_scgt_clks), clk_crgctrl_data);

	plat_clk_register_fixed_factor(fixed_factor_clks, ARRAY_SIZE(fixed_factor_clks), clk_crgctrl_data);

	plat_clk_register_divider(crgctrl_div_clks, ARRAY_SIZE(crgctrl_div_clks), clk_crgctrl_data);

	plat_clk_register_mux(crgctrl_mux_clks, ARRAY_SIZE(crgctrl_mux_clks), clk_crgctrl_data);

	plat_clk_register_gate(crgctrl_gate_clks, ARRAY_SIZE(crgctrl_gate_clks), clk_crgctrl_data);
}

CLK_OF_DECLARE_DRIVER(clk_peri_crgctrl, "hisilicon,clk-extreme-crgctrl", clk_crgctrl_init);

static void clk_hsdt_init(struct device_node *np)
{
	struct clock_data *clk_hsdtcrg_data = NULL;
	int nr = ARRAY_SIZE(hsdtcrg_scgt_clks) +
		ARRAY_SIZE(hsdtcrg_mux_clks) +
		ARRAY_SIZE(hsdtcrg_gate_clks);

	clk_hsdtcrg_data = clk_init(np, nr);
	if (!clk_hsdtcrg_data)
		return;

	plat_clk_register_scgt(hsdtcrg_scgt_clks, ARRAY_SIZE(hsdtcrg_scgt_clks), clk_hsdtcrg_data);

	plat_clk_register_mux(hsdtcrg_mux_clks, ARRAY_SIZE(hsdtcrg_mux_clks), clk_hsdtcrg_data);

	plat_clk_register_gate(hsdtcrg_gate_clks, ARRAY_SIZE(hsdtcrg_gate_clks), clk_hsdtcrg_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdtcrg, "hisilicon,clk-extreme-hsdt-crg", clk_hsdt_init);

static void clk_media1_init(struct device_node *np)
{
	int nr;
	struct clock_data *clk_media1crg_data = NULL;

#ifdef CONFIG_CLK_DEBUG
	int no_media = is_no_media();
#else
	int no_media = 0;
#endif

	if (no_media)
		nr = ARRAY_SIZE(media1crg_stub_clks);
	else
		nr = ARRAY_SIZE(media1crg_scgt_clks) +
			ARRAY_SIZE(media1crg_div_clks) +
			ARRAY_SIZE(media1crg_mux_clks) +
			ARRAY_SIZE(media1crg_gate_clks);

	clk_media1crg_data = clk_init(np, nr);
	if (!clk_media1crg_data)
		return;

	if (no_media) {
		pr_err("[%s] media1 clock won't initialize!\n", __func__);
		plat_clk_register_fixed_factor(media1crg_stub_clks,
			ARRAY_SIZE(media1crg_stub_clks), clk_media1crg_data);
	} else {
		plat_clk_register_mux(media1crg_mux_clks,
			ARRAY_SIZE(media1crg_mux_clks), clk_media1crg_data);

		plat_clk_register_divider(media1crg_div_clks,
			ARRAY_SIZE(media1crg_div_clks), clk_media1crg_data);

		plat_clk_register_scgt(media1crg_scgt_clks,
			ARRAY_SIZE(media1crg_scgt_clks), clk_media1crg_data);

		plat_clk_register_gate(media1crg_gate_clks,
			ARRAY_SIZE(media1crg_gate_clks), clk_media1crg_data);
	}
}
CLK_OF_DECLARE_DRIVER(clk_media1crg, "hisilicon,clk-extreme-media1-crg", clk_media1_init);

static void clk_media2_init(struct device_node *np)
{
	int nr;
	struct clock_data *clk_media2crg_data = NULL;

#ifdef CONFIG_CLK_DEBUG
	int no_media = is_no_media();
#else
	int no_media = 0;
#endif

	if (no_media || is_fpga())
		nr = ARRAY_SIZE(media2crg_stub_clks);
	else
		nr = ARRAY_SIZE(media2crg_scgt_clks) +
			ARRAY_SIZE(media2crg_div_clks) +
			ARRAY_SIZE(media2crg_mux_clks) +
			ARRAY_SIZE(media2crg_gate_clks);

	clk_media2crg_data = clk_init(np, nr);
	if (!clk_media2crg_data)
		return;

	if (no_media || is_fpga()) {
		pr_err("[%s] media2 clock won't initialize!\n", __func__);
		plat_clk_register_fixed_factor(media2crg_stub_clks,
			ARRAY_SIZE(media2crg_stub_clks), clk_media2crg_data);
	} else {
		plat_clk_register_divider(media2crg_div_clks,
			ARRAY_SIZE(media2crg_div_clks), clk_media2crg_data);

		plat_clk_register_mux(media2crg_mux_clks,
			ARRAY_SIZE(media2crg_mux_clks), clk_media2crg_data);

		plat_clk_register_scgt(media2crg_scgt_clks,
			ARRAY_SIZE(media2crg_scgt_clks), clk_media2crg_data);

		plat_clk_register_gate(media2crg_gate_clks,
			ARRAY_SIZE(media2crg_gate_clks), clk_media2crg_data);
	}
}
CLK_OF_DECLARE_DRIVER(clk_media2crg, "hisilicon,clk-extreme-media2-crg", clk_media2_init);

static void clk_xfreq_init(struct device_node *np)
{
	struct clock_data *clk_xfreq_data = NULL;
	int nr = ARRAY_SIZE(xfreq_clks);

	clk_xfreq_data = clk_init(np, nr);
	if (!clk_xfreq_data)
		return;

	plat_clk_register_xfreq(xfreq_clks, ARRAY_SIZE(xfreq_clks), clk_xfreq_data);
}

CLK_OF_DECLARE_DRIVER(clk_xfreq, "hisilicon,clk-extreme-xfreq", clk_xfreq_init);

static void clk_pmu_init(struct device_node *np)
{
	struct clock_data *clk_pmu_data = NULL;
	int nr = ARRAY_SIZE(pmu_clks);

	clk_pmu_data = clk_init(np, nr);
	if (!clk_pmu_data)
		return;

	plat_clk_register_clkpmu(pmu_clks, ARRAY_SIZE(pmu_clks), clk_pmu_data);
}

CLK_OF_DECLARE_DRIVER(clk_pmu, "hisilicon,clk-extreme-pmu", clk_pmu_init);

static void clk_dvfs_init(struct device_node *np)
{
	struct clock_data *clk_dvfs_data = NULL;
	int nr = ARRAY_SIZE(dvfs_clks);

	clk_dvfs_data = clk_init(np, nr);
	if (!clk_dvfs_data)
		return;

	plat_clk_register_dvfs_clk(dvfs_clks,
		ARRAY_SIZE(dvfs_clks), clk_dvfs_data);
}

CLK_OF_DECLARE_DRIVER(clk_dvfs, "hisilicon,clk-extreme-dvfs", clk_dvfs_init);
