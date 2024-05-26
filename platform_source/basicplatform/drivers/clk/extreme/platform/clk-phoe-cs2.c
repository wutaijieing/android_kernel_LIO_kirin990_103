/*
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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
#include "clk-phoe-cs2.h"
#include "../clk-mclk.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>

static const struct fixed_rate_clock fixed_clks[] = {
	{ CLKIN_SYS, "clkin_sys", NULL, 0, 38400000, "clkin_sys" },
	{ CLKIN_REF, "clkin_ref", NULL, 0, 32764, "clkin_ref" },
	{ CLK_FLL_SRC, "clk_fll_src", NULL, 0, 180000000, "clk_fll_src" },
	{ CLK_PPLL0, "clk_ppll0", NULL, 0, 1660000000, "clk_ppll0" },
	{ CLK_PPLL1, "clk_ppll1", NULL, 0, 1866000000, "clk_ppll1" },
	{ CLK_PPLL2, "clk_ppll2", NULL, 0, 1920000000, "clk_ppll2" },
	{ CLK_PPLL3, "clk_ppll3", NULL, 0, 1200000000, "clk_ppll3" },
	{ CLK_PPLL5, "clk_ppll5", NULL, 0, 1500000000, "clk_ppll5" },
	{ CLK_SCPLL, "clk_scpll", NULL, 0, 393216000, "clk_scpll" },
	{ CLK_PPLL6, "clk_ppll6", NULL, 0, 1720000000, "clk_ppll6" },
	{ CLK_PPLL7, "clk_ppll7", NULL, 0, 1290000000, "clk_ppll7" },
	{ CLK_SPLL, "clk_spll", NULL, 0, 1572864000, "clk_spll" },
	{ CLK_MODEM_BASE, "clk_modem_base", NULL, 0, 49152000, "clk_modem_base" },
	{ CLK_LBINTPLL, "clk_lbintpll", NULL, 0, 100000000, "clk_lbintpll" },
	{ CLK_LBINTPLL_1, "clk_lbintpll_1", NULL, 0, 49146757, "clk_lbintpll_1" },
	{ CLK_PPLL_PCIE, "clk_ppll_pcie", NULL, 0, 100000000, "clk_ppll_pcie" },
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
	{ CLK_SYS_INI, "clk_sys_ini", "clkin_sys", 1, 2, 0, "clk_sys_ini" },
	{ CLK_DIV_SYSBUS, "div_sysbus_pll", "clk_ap_ppll0", 1, 7, 0, "div_sysbus_pll" },
	{ CLK_PPLL_EPS, "clk_ppll_eps", "clk_ap_ppll2", 1, 1, 0, "clk_ppll_eps" },
	{ CLK_GATE_WD0_HIGH, "clk_wd0_high", "sc_div_cfgbus", 1, 1, 0, "clk_wd0_high" },
	{ PCLK_GATE_DSI0, "pclk_dsi0", "sc_div_cfgbus", 1, 1, 0, "pclk_dsi0" },
	{ PCLK_GATE_DSI1, "pclk_dsi1", "sc_div_cfgbus", 1, 1, 0, "pclk_dsi1" },
	{ CLK_FACTOR_TCXO, "clk_factor_tcxo", "clk_sys_ini", 1, 4, 0, "clk_factor_tcxo" },
	{ CLK_GATE_TIMER5_A, "clk_timer5_a", "clkmux_timer5_a", 1, 1, 0, "clk_timer5_a" },
	{ ATCLK, "clk_at", "clk_atdvfs", 1, 1, 0, "clk_at" },
	{ CLK_DIV_CSSYSDBG, "clk_cssys_div", "autodiv_sysbus", 1, 1, 0, "clk_cssys_div" },
	{ CLK_GATE_CSSYSDBG, "clk_cssysdbg", "clk_dmabus_div", 1, 1, 0, "clk_cssysdbg" },
	{ NPU_PLL6_FIX, "npu_pll6_fix", "clk_ap_ppll6", 1, 4, 0, "npu_pll6_fix" },
	{ CLK_GATE_DMA_IOMCU, "clk_dma_iomcu", "clk_fll_src", 1, 4, 0, "clk_dma_iomcu" },
	{ CLK_SD_SYS, "clk_sd_sys", "clk_sd_sys_gt", 1, 6, 0, "clk_sd_sys" },
	{ CLK_SDIO_SYS, "clk_sdio_sys", "clk_sdio_sys_gt", 1, 6, 0, "clk_sdio_sys" },
	{ CLK_DIV_A53HPM, "clk_a53hpm_div", "clk_a53hpm_gt", 1, 4, 0, "clk_a53hpm_div" },
	{ CLK_DIV_320M, "sc_div_320m", "gt_clk_320m_pll", 1, 5, 0, "sc_div_320m" },
	{ PCLK_GATE_UART0, "pclk_uart0", "clk_uart0", 1, 1, 0, "pclk_uart0" },
	{ CLK_FACTOR_UART0, "clk_uart0_fac", "clkmux_uart0", 1, 1, 0, "clk_uart0_fac" },
	{ CLK_FACTOR_USB3PHY_PLL, "clkfac_usb3phy", "clk_ap_ppll0", 1, 60, 0, "clkfac_usb3phy" },
	{ CLKIN_SYS_DIV, "clkin_sys_div", "clkin_sys", 1, 2, 0, "clkin_sys_div" },
	{ CLK_GATE_ABB_USB, "clk_abb_usb", "clk_usb_tcxo_en", 1, 1, 0, "clk_abb_usb" },
	{ CLK_GATE_UFSPHY_REF, "clk_ufsphy_ref", "clkin_sys", 1, 1, 0, "clk_ufsphy_ref" },
	{ CLK_GATE_UFSIO_REF, "clk_ufsio_ref", "clkin_sys", 1, 1, 0, "clk_ufsio_ref" },
	{ CLK_GATE_BLPWM, "clk_blpwm", "clk_fll_src", 1, 2, 0, "clk_blpwm" },
	{ CLK_SYSCNT_DIV, "clk_syscnt_div", "clk_sys_ini", 1, 10, 0, "clk_syscnt_div" },
	{ CLK_GATE_GPS_REF, "clk_gps_ref", "clk_modem_base", 1, 1, 0, "clk_gps_ref" },
	{ CLK_MUX_GPS_REF, "clkmux_gps_ref", "clk_modem_base", 1, 1, 0, "clkmux_gps_ref" },
	{ CLK_GATE_MDM2GPS0, "clk_mdm2gps0", "clk_modem_base", 1, 1, 0, "clk_mdm2gps0" },
	{ CLK_GATE_MDM2GPS1, "clk_mdm2gps1", "clk_modem_base", 1, 1, 0, "clk_mdm2gps1" },
	{ CLK_GATE_MDM2GPS2, "clk_mdm2gps2", "clk_modem_base", 1, 1, 0, "clk_mdm2gps2" },
	{ CLK_GATE_LDI0, "clk_ldi0", "clk_sys_ini", 1, 1, 0, "clk_ldi0" },
	{ EPS_VOLT_HIGH, "eps_volt_high", "peri_volt_hold", 1, 1, 0, "eps_volt_high" },
	{ EPS_VOLT_MIDDLE, "eps_volt_middle", "peri_volt_middle", 1, 1, 0, "eps_volt_middle" },
	{ EPS_VOLT_LOW, "eps_volt_low", "peri_volt_low", 1, 1, 0, "eps_volt_low" },
	{ VENC_VOLT_HOLD, "venc_volt_hold", "peri_volt_hold", 1, 1, 0, "venc_volt_hold" },
	{ VDEC_VOLT_HOLD, "vdec_volt_hold", "peri_volt_hold", 1, 1, 0, "vdec_volt_hold" },
	{ EDC_VOLT_HOLD, "edc_volt_hold", "peri_volt_hold", 1, 1, 0, "edc_volt_hold" },
	{ EFUSE_VOLT_HOLD, "efuse_volt_hold", "peri_volt_middle", 1, 1, 0, "efuse_volt_hold" },
	{ LDI0_VOLT_HOLD, "ldi0_volt_hold", "peri_volt_hold", 1, 1, 0, "ldi0_volt_hold" },
	{ SEPLAT_VOLT_HOLD, "seplat_volt_hold", "peri_volt_hold", 1, 1, 0, "seplat_volt_hold" },
	{ CLK_FIX_DIV_DPCTRL, "clk_fix_div_dpctrl", "clk_gt_16m_dp", 1, 10, 0, "clk_fix_div_dpctrl" },
	{ CLK_ISP_SNCLK_FAC, "clk_fac_ispsn", "clk_ispsn_gt", 1, 14, 0, "clk_fac_ispsn" },
	{ CLK_FACTOR_RXDPHY, "clk_rxdcfg_fac", "clk_rxdcfg_gt", 1, 6, 0, "clk_rxdcfg_fac" },
	{ CLK_DIV_HIFD_PLL, "clk_hifd_pll_div", "clkgt_hifd_pll", 1, 5, 0, "clk_hifd_pll_div" },
	{ CLK_GATE_I2C0, "clk_i2c0", "clk_fll_src", 1, 2, 0, "clk_i2c0" },
	{ CLK_GATE_I2C1, "clk_i2c1", "clk_i2c1_gt", 1, 2, 0, "clk_i2c1" },
	{ CLK_GATE_I2C2, "clk_i2c2", "clk_fll_src", 1, 2, 0, "clk_i2c2" },
	{ CLK_GATE_SPI0, "clk_spi0", "clk_fll_src", 1, 2, 0, "clk_spi0" },
	{ CLK_FAC_180M, "clkfac_180m", "clk_ppll0", 1, 8, 0, "clkfac_180m" },
	{ CLK_GATE_IOMCU_PERI0, "clk_gate_peri0", "clk_spll", 1, 1, 0, "clk_gate_peri0" },
	{ CLK_GATE_SPI2, "clk_spi2", "clk_fll_src", 1, 2, 0,  "clk_spi2" },
	{ CLK_GATE_UART3, "clk_uart3", "clk_gate_peri0", 1, 8, 0, "clk_uart3" },
	{ CLK_GATE_UART8, "clk_uart8", "clk_gate_peri0", 1, 8, 0, "clk_uart8" },
	{ CLK_GATE_UART7, "clk_uart7", "clk_gate_peri0", 1, 8, 0, "clk_uart7" },
	{ AUTODIV_ISP_DVFS, "autodiv_isp_dvfs", "autodiv_sysbus", 1, 1, 0, "autodiv_isp_dvfs" },
};

static const struct gate_clock crgctrl_gate_clks[] = {
	{ CLK_GATE_PPLL0_MEDIA, "clk_ppll0_media", "clk_ap_ppll0", 0x410, 0x40000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_media" },
	{ CLK_GATE_PPLL2_MEDIA, "clk_ppll2_media", "clk_ap_ppll2", 0x410, 0x8000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_media" },
	{ CLK_GATE_PPLL3_MEDIA, "clk_ppll3_media", "clk_ap_ppll3", 0x410, 0x10000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll3_media" },
	{ CLK_GATE_PPLL7_MEDIA, "clk_ppll7_media", "clk_ap_ppll7", 0x410, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll7_media" },
	{ PCLK_GPIO0, "pclk_gpio0", "sc_div_cfgbus", 0x10, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio0" },
	{ PCLK_GPIO1, "pclk_gpio1", "sc_div_cfgbus", 0x10, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio1" },
	{ PCLK_GPIO2, "pclk_gpio2", "sc_div_cfgbus", 0x10, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio2" },
	{ PCLK_GPIO3, "pclk_gpio3", "sc_div_cfgbus", 0x10, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio3" },
	{ PCLK_GPIO4, "pclk_gpio4", "sc_div_cfgbus", 0x10, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio4" },
	{ PCLK_GPIO5, "pclk_gpio5", "sc_div_cfgbus", 0x10, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio5" },
	{ PCLK_GPIO6, "pclk_gpio6", "sc_div_cfgbus", 0x10, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio6" },
	{ PCLK_GPIO7, "pclk_gpio7", "sc_div_cfgbus", 0x10, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio7" },
	{ PCLK_GPIO8, "pclk_gpio8", "sc_div_cfgbus", 0x10, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio8" },
	{ PCLK_GPIO9, "pclk_gpio9", "sc_div_cfgbus", 0x10, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio9" },
	{ PCLK_GPIO10, "pclk_gpio10", "sc_div_cfgbus", 0x10, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio10" },
	{ PCLK_GPIO11, "pclk_gpio11", "sc_div_cfgbus", 0x10, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio11" },
	{ PCLK_GPIO12, "pclk_gpio12", "sc_div_cfgbus", 0x10, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio12" },
	{ PCLK_GPIO13, "pclk_gpio13", "sc_div_cfgbus", 0x10, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio13" },
	{ PCLK_GPIO14, "pclk_gpio14", "sc_div_cfgbus", 0x10, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio14" },
	{ PCLK_GPIO15, "pclk_gpio15", "sc_div_cfgbus", 0x10, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio15" },
	{ PCLK_GPIO16, "pclk_gpio16", "sc_div_cfgbus", 0x10, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio16" },
	{ PCLK_GPIO17, "pclk_gpio17", "sc_div_cfgbus", 0x10, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio17" },
	{ PCLK_GPIO18, "pclk_gpio18", "sc_div_cfgbus", 0x10, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio18" },
	{ PCLK_GPIO19, "pclk_gpio19", "sc_div_cfgbus", 0x10, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio19" },
	{ PCLK_GATE_WD0_HIGH, "pclk_wd0_high", "sc_div_cfgbus", 0x20, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0_high" },
	{ PCLK_GATE_WD0, "pclk_wd0", "clk_wd0_mux", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0" },
	{ PCLK_GATE_WD1, "pclk_wd1", "sc_div_cfgbus", 0x20, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd1" },
	{ CLK_GATE_CODECSSI, "clk_codecssi", "sel_codeccssi", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_codecssi" },
	{ PCLK_GATE_CODECSSI, "pclk_codecssi", "sc_div_cfgbus", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_codecssi" },
	{ CLK_GATE_MMC_USBDP, "clk_mmc_usbdp", "div_mmc_usbdp", 0x040, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mmc_usbdp" },
	{ PCLK_GATE_IOC, "pclk_ioc", "clk_sys_ini", 0x20, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ioc" },
	{ PCLK_GATE_MMC1_PCIE, "pclk_mmc1_pcie", "pclk_pcie_div", 0x30, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_mmc1_pcie" },
	{ CLK_ATDVFS, "clk_atdvfs", "clk_cssys_div", 0, 0x0, 0,
		NULL, 1, { 0, 0, 0 }, { 0, 1, 2, 3 }, 3, 19, 0, "clk_atdvfs" },
	{ ACLK_GATE_PERF_STAT, "aclk_perf_stat", "clk_dmabus_div", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_perf_stat" },
	{ PCLK_GATE_PERF_STAT, "pclk_perf_stat", "clk_dmabus_div", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_perf_stat" },
	{ CLK_GATE_PERF_STAT, "clk_perf_stat", "clk_perf_div", 0x040, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat" },
	{ CLK_GATE_DMAC, "clk_dmac", "div_sysbus_pll", 0x30, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dmac" },
	{ CLK_GATE_SOCP_ACPU, "clk_socp_acpu", "clk_dmabus_div", 0x10, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_socp_acpu" },
	{ CLK_GATE_SOCP_DEFLATE, "clk_socp_deflat", "clk_socpdef_div", 0, 0x0, 0,
		"clk_socp_acpu", 0, {0}, {0}, 0, 0, 0, "clk_socp_deflat" },
	{ CLK_GATE_TCLK_SOCP, "tclk_socp", "clk_sys_ini", 0, 0x0, 0,
		"clk_socp_acpu", 0, {0}, {0}, 0, 0, 0, "tclk_socp" },
	{ CLK_GATE_IPF, "clk_ipf", "div_sysbus_pll", 0x0, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ipf" },
	{ CLK_GATE_IPF_PSAM, "clk_ipf_psam", "div_sysbus_pll", 0x0, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ipf_psam" },
	{ACLK_GATE_MAA, "aclk_maa", "div_sysbus_pll", 0x470, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_maa"},
	{CLK_GATE_MAA_REF, "clk_maa_ref", "clkin_ref", 0x470, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_maa_ref"},
	{CLK_GATE_SPE, "clk_spe", "clk_spe_div", 0x470, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spe"},
	{HCLK_GATE_SPE, "hclk_spe", "div_sysbus_pll", 0x470, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_spe"},
	{CLK_GATE_SPE_REF, "clk_spe_ref", "clkin_ref", 0x470, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spe_ref"},
	{ACLK_GATE_AXI_MEM, "aclk_axi_mem", "div_sysbus_pll", 0x470, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_axi_mem"},
	{CLK_GATE_AXI_MEM, "clk_axi_mem", "div_sysbus_pll", 0x470, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_axi_mem"},
	{CLK_GATE_AXI_MEM_GS, "clk_axi_mem_gs", "div_sysbus_pll", 0x470, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_axi_mem_gs"},
	{ CLK_GATE_VCODECBUS2DDR, "clk_vcodecbus2", "clk_vcodbus_div", 0x0, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus2" },
	{ CLK_GATE_SD, "clk_sd", "clk_sd_muxsys", 0x40, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 2, 1, "clk_sd" },
	{ CLK_GATE_UART1, "clk_uart1", "clkmux_uarth", 0x20, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart1" },
	{ CLK_GATE_UART4, "clk_uart4", "clkmux_uarth", 0x20, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart4" },
	{ PCLK_GATE_UART1, "pclk_uart1", "clkmux_uarth", 0x20, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart1" },
	{ PCLK_GATE_UART4, "pclk_uart4", "clkmux_uarth", 0x20, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart4" },
	{ CLK_GATE_UART2, "clk_uart2", "clkmux_uartl", 0x20, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart2" },
	{ CLK_GATE_UART5, "clk_uart5", "clkmux_uartl", 0x20, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart5" },
	{ PCLK_GATE_UART2, "pclk_uart2", "clkmux_uartl", 0x20, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart2" },
	{ PCLK_GATE_UART5, "pclk_uart5", "clkmux_uartl", 0x20, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart5" },
	{ CLK_GATE_UART0, "clk_uart0", "clkmux_uart0", 0x20, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart0" },
	{ CLK_GATE_I2C3, "clk_i2c3", "clkmux_i2c", 0x20, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c3" },
	{ CLK_GATE_I2C4, "clk_i2c4", "clkmux_i2c", 0x20, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c4" },
	{ CLK_GATE_I2C6_ACPU, "clk_i2c6_acpu", "clkmux_i2c", 0x10, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c6_acpu" },
	{ CLK_GATE_I2C7, "clk_i2c7", "clkmux_i2c", 0x10, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c7" },
	{ PCLK_GATE_I2C3, "pclk_i2c3", "clkmux_i2c", 0x20, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c3" },
	{ PCLK_GATE_I2C4, "pclk_i2c4", "clkmux_i2c", 0x20, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c4" },
	{ PCLK_GATE_I2C6_ACPU, "pclk_i2c6_acpu", "clkmux_i2c", 0x10, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c6_acpu" },
	{ PCLK_GATE_I2C7, "pclk_i2c7", "clkmux_i2c", 0x10, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c7" },
	{ CLK_GATE_I3C4, "clk_i3c4", "clkdiv_i3c4", 0x470, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i3c4" },
	{ PCLK_GATE_I3C4, "pclk_i3c4", "clkdiv_i3c4", 0x470, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i3c4" },
	{ CLK_GATE_SPI1, "clk_spi1", "clkmux_spi", 0x20, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi1" },
	{ CLK_GATE_SPI4, "clk_spi4", "clkmux_spi", 0x40, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi4" },
	{ PCLK_GATE_SPI1, "pclk_spi1", "clkmux_spi", 0x20, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi1" },
	{ PCLK_GATE_SPI4, "pclk_spi4", "clkmux_spi", 0x40, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi4" },
	{ CLK_GATE_USB3OTG_REF, "clk_usb3otg_ref", "clk_sys_ini", 0x050, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3otg_ref" },
	{ CLK_GATE_AO_ASP, "clk_ao_asp", "clk_ao_asp_div", 0x0, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_asp" },
	{ PCLK_GATE_PCTRL, "pclk_pctrl", "clk_ptp_div", 0x20, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pctrl" },
	{ CLK_GATE_PWM, "clk_pwm", "clk_ptp_div", 0x20, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pwm" },
	{ PCLK_GATE_BLPWM_PERI, "pclk_blpwm_peri", "div_blpwm_peri", 0x20, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_blpwm_peri" },
	{ PERI_VOLT_HOLD, "peri_volt_hold", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, { 0, 0, 0 }, { 0, 1, 2, 3 }, 3, 18, 0, "peri_volt_hold" },
	{ PERI_VOLT_MIDDLE, "peri_volt_middle", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, { 0, 0 }, { 0, 1, 2 }, 2, 22, 0, "peri_volt_middle" },
	{ PERI_VOLT_LOW, "peri_volt_low", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0}, { 0, 1 }, 1, 28, 0, "peri_volt_low" },
	{ CLK_GATE_DPCTRL_16M, "clk_dpctrl_16m", "div_dpctrl16m", 0x470, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dpctrl_16m" },
	{ CLK_GATE_ISP_I2C_MEDIA, "clk_isp_i2c_media", "clk_div_isp_i2c", 0x30, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i2c_media" },
	{ CLK_GATE_ISP_SNCLK0, "clk_isp_snclk0", "clk_mux_ispsn0", 0x50, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk0" },
	{ CLK_GATE_ISP_SNCLK1, "clk_isp_snclk1", "clk_mux_ispsn1", 0x50, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk1"},
	{ CLK_GATE_ISP_SNCLK2, "clk_isp_snclk2", "clk_mux_ispsn2", 0x50, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk2" },
	{ CLK_GATE_ISP_SNCLK3, "clk_isp_snclk3", "clk_mux_ispsn3", 0x40, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk3" },
	{ CLK_GATE_RXDPHY0_CFG, "clk_rxdphy0_cfg", "clk_rxdcfg_mux", 0x030, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy0_cfg" },
	{ CLK_GATE_RXDPHY1_CFG, "clk_rxdphy1_cfg", "clk_rxdcfg_mux", 0x030, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy1_cfg" },
	{ CLK_GATE_RXDPHY2_CFG, "clk_rxdphy2_cfg", "clk_rxdcfg_mux", 0x030, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy2_cfg" },
	{ CLK_GATE_RXDPHY3_CFG, "clk_rxdphy3_cfg", "clk_rxdcfg_mux", 0x030, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy3_cfg" },
	{ CLK_GATE_TXDPHY0_CFG, "clk_txdphy0_cfg", "clk_sys_ini", 0x030, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_cfg" },
	{ CLK_GATE_TXDPHY0_REF, "clk_txdphy0_ref", "clk_sys_ini", 0x030, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_ref" },
	{ CLK_GATE_TXDPHY1_CFG, "clk_txdphy1_cfg", "clk_sys_ini", 0x030, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy1_cfg" },
	{ CLK_GATE_TXDPHY1_REF, "clk_txdphy1_ref", "clk_sys_ini", 0x030, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy1_ref" },
	{ PCLK_GATE_LOADMONITOR, "pclk_loadmonitor", "sc_div_cfgbus", 0x20, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor" },
	{ CLK_GATE_LOADMONITOR, "clk_loadmonitor", "clk_loadmonitor_div", 0x20, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor" },
	{ PCLK_GATE_LOADMONITOR_L, "pclk_loadmonitor_l", "sc_div_cfgbus", 0x20, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor_l" },
	{ CLK_GATE_LOADMONITOR_L, "clk_loadmonitor_l", "clk_loadmonitor_div", 0x20, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_l" },
	{PCLK_GATE_LOADMONITOR_2, "pclk_loadmonitor_2", "sc_div_cfgbus", 0x30, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor_2"},
	{CLK_GATE_LOADMONITOR_2, "clk_loadmonitor_2", "clk_loadmonitor_div", 0x30, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_2"},
	{ CLK_GATE_MEDIA_TCXO, "clk_media_tcxo", "clk_sys_ini", 0x40, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_tcxo" },
	{ CLK_GATE_AO_HIFD, "clk_ao_hifd", "clk_ao_hifd_div", 0x50, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_hifd" },
	{ AUTODIV_ISP, "autodiv_isp", "autodiv_isp_dvfs", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "autodiv_isp" },
	{ CLK_GATE_ATDIV_MMC0, "clk_atdiv_mmc0", "clk_div_mmc0bus", 0x410, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_mmc0" },
	{ CLK_GATE_ATDIV_DMA, "clk_atdiv_dma", "clk_dmabus_div", 0x410, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_dma" },
	{ CLK_GATE_ATDIV_CFG, "clk_atdiv_cfg", "sc_div_cfgbus", 0x410, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_cfg" },
	{ CLK_GATE_ATDIV_SYS, "clk_atdiv_sys", "div_sysbus_pll", 0x410, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_sys" },
};

static const struct scgt_clock crgctrl_scgt_clks[] = {
	{ CLK_ANDGT_MMC_USBDP, "clk_mmc_usbdp_andgt", "clk_ap_ppll0",
		0x70C, 2, CLK_GATE_HIWORD_MASK, "clk_mmc_usbdp_andgt" },
	{ AUTODIV_SYSBUS, "autodiv_sysbus", "div_sysbus_pll",
		0x404, 5, CLK_GATE_HIWORD_MASK, "autodiv_sysbus" },
	{ AUTODIV_EMMC0BUS, "autodiv_emmc0bus", "autodiv_sysbus",
		0x404, 1, CLK_GATE_HIWORD_MASK, "autodiv_emmc0bus" },
	{ PCLK_ANDGT_MMC1_PCIE, "pclk_pcie_andgt", "sc_div_320m",
		0xf8, 13, CLK_GATE_HIWORD_MASK, "pclk_pcie_andgt" },
	{ CLK_PERF_DIV_GT, "clk_perf_gt", "sc_div_320m",
		0xF0, 3, CLK_GATE_HIWORD_MASK, "clk_perf_gt" },
	{ CLK_SOCP_DEFLATE_GT, "clk_socp_def_gt", "clk_ap_ppll0",
		0x0fc, 9, CLK_GATE_HIWORD_MASK, "clk_socp_def_gt" },
	{ CLK_GATE_TIME_STAMP_GT, "clk_timestp_gt", "clk_sys_ini",
		0xF0, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_timestp_gt" },
	{CLK_GATE_SPE_GT, "clk_spe_gt", "sc_div_320m",
		0x70C, 9, CLK_GATE_HIWORD_MASK, "clk_spe_gt"},
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_vcodbus_mux",
		0x0F0, 8, CLK_GATE_HIWORD_MASK, "clk_vcodbus_gt" },
	{ CLK_ANDGT_SD, "clk_sd_gt", "clk_sd_muxpll",
		0xF4, 3, CLK_GATE_HIWORD_MASK, "clk_sd_gt" },
	{ CLK_SD_SYS_GT, "clk_sd_sys_gt", "clk_sys_ini",
		0xF4, 5, CLK_GATE_HIWORD_MASK, "clk_sd_sys_gt" },
	{ CLK_A53HPM_ANDGT, "clk_a53hpm_gt", "clk_a53hpm_mux",
		0x0F4, 7, CLK_GATE_HIWORD_MASK, "clk_a53hpm_gt" },
	{ CLK_320M_PLL_GT, "gt_clk_320m_pll", "sc_sel_320m_pll",
		0xF8, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "gt_clk_320m_pll" },
	{ CLK_ANDGT_UARTH, "clkgt_uarth", "sc_div_320m",
		0xF4, 11, CLK_GATE_HIWORD_MASK, "clkgt_uarth" },
	{ CLK_ANDGT_UARTL, "clkgt_uartl", "sc_div_320m",
		0xF4, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uartl" },
	{ CLK_ANDGT_UART0, "clkgt_uart0", "sc_div_320m",
		0xF4, 9, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uart0" },
	{ CLK_ANDGT_I3C4, "clkgt_i3c4", "sc_div_320m",
		0x70C, 5, CLK_GATE_HIWORD_MASK, "clkgt_i3c4" },
	{ CLK_ANDGT_SPI, "clkgt_spi", "sc_div_320m",
		0xF4, 13, CLK_GATE_HIWORD_MASK, "clkgt_spi" },
	{ CLK_DIV_AO_ASP_GT, "clk_ao_asp_gt", "clkmux_ao_asp",
		0xF4, 4, CLK_GATE_HIWORD_MASK, "clk_ao_asp_gt" },
	{ CLK_ANDGT_PTP, "clk_ptp_gt", "sc_div_320m",
		0xF8, 5, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_ptp_gt"},
	{ CLK_BLPWM_PERI_GT, "clk_blpwm_peri_gt", "sel_blpwm",
		0xFC, 1, CLK_GATE_HIWORD_MASK, "clk_blpwm_peri_gt" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clkmux_vdec",
		0xF0, 13, CLK_GATE_HIWORD_MASK, "clkgt_vdec" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clkmux_venc",
		0xF0, 9, CLK_GATE_HIWORD_MASK, "clkgt_venc" },
	{ CLK_GT_DPCTRL_16M, "clk_gt_16m_dp", "clk_a53hpm_div",
		0xfc, 12, CLK_GATE_HIWORD_MASK, "clk_gt_16m_dp" },
	{ CLK_GT_ISP_I2C, "clk_gt_isp_i2c", "sc_div_320m",
		0xf8, 4, CLK_GATE_HIWORD_MASK, "clk_gt_isp_i2c" },
	{ CLK_ANDGT_VENC2, "clkgt_venc2", "clkmux_venc2",
		0x0F0, 14, CLK_GATE_HIWORD_MASK, "clkgt_venc2" },
	{ CLK_ISP_SNCLK_ANGT, "clk_ispsn_gt", "sc_div_320m",
		0x108, 2, CLK_GATE_HIWORD_MASK, "clk_ispsn_gt" },
	{ CLK_ANDGT_RXDPHY, "clk_rxdcfg_gt", "clk_a53hpm_div",
		0x0F0, 12, CLK_GATE_HIWORD_MASK, "clk_rxdcfg_gt" },
	{ CLK_GT_LOADMONITOR, "clk_loadmonitor_gt", "sc_div_320m",
		0x0f0, 5, CLK_GATE_HIWORD_MASK, "clk_loadmonitor_gt" },
	{ CLK_ANDGT_IVP32DSP_CORE, "clkgt_ivp32dsp", "clkmux_ivp32dsp",
		0x0F0, 7, CLK_GATE_HIWORD_MASK, "clkgt_ivp32dsp" },
	{ CLK_ANDGT_AO_HIFD, "clkgt_ao_hifd", "clkmux_ao_hifd",
		0x100, 10, CLK_GATE_HIWORD_MASK, "clkgt_ao_hifd" },
	{ AUTODIV_CFGBUS, "autodiv_cfgbus", "autodiv_sysbus",
		0x404, 4, CLK_GATE_HIWORD_MASK, "autodiv_cfgbus" },
	{ AUTODIV_DMABUS, "autodiv_dmabus", "autodiv_sysbus",
		0x404, 3, CLK_GATE_HIWORD_MASK, "autodiv_dmabus" },
};

static const struct pll_clock pll_clks[] = {
	{ CLK_GATE_PPLL0, "clk_ap_ppll0", "clk_ppll0", 0x0,
		{0x96C, 0}, {0x970, 0}, {0x974, 0}, {0x4F0, 26}, HS_PMCTRL, 0, "clk_ap_ppll0" },
	{ CLK_GATE_PPLL1, "clk_ap_ppll1", "clk_ppll1", 0x1,
		{0x954, 0}, {0x958, 0}, {0x95C, 0}, {0x500, 26}, HS_PMCTRL, 0, "clk_ap_ppll1" },
	{ CLK_GATE_PPLL2, "clk_ap_ppll2", "clk_ppll2", 0x2,
		{0x954, 5}, {0x958, 5}, {0x95C, 5}, {0x508, 26}, HS_PMCTRL, 0, "clk_ap_ppll2" },
	{ CLK_GATE_PPLL3, "clk_ap_ppll3", "clk_ppll3", 0x3,
		{0x954, 10}, {0x958, 10}, {0x95C, 10}, {0x510, 26}, HS_PMCTRL, 0, "clk_ap_ppll3" },
	{ CLK_GATE_PPLL5, "clk_ap_ppll5", "clk_ppll5", 0x5,
		{0x96C, 5}, {0x970, 5}, {0x974, 5}, {0x520, 26}, HS_PMCTRL, 0, "clk_ap_ppll5" },
	{ CLK_GATE_PPLL6, "clk_ap_ppll6", "clk_ppll6", 0x6,
		{0x960, 5}, {0x964, 5}, {0x968, 5}, {0x528, 26}, HS_PMCTRL, 0, "clk_ap_ppll6" },
	{ CLK_GATE_PPLL7, "clk_ap_ppll7", "clk_ppll7", 0x7,
		{0x960, 10}, {0x964, 10}, {0x968, 10}, {0x530, 26}, HS_PMCTRL, 0, "clk_ap_ppll7" },
};

static const struct div_clock crgctrl_div_clks[] = {
	{ CLK_DIV_CFGBUS, "sc_div_cfgbus", "div_sysbus_pll",
		0xEC, 0x3, 4, 1, 0, "sc_div_cfgbus" },
	{ CLK_DIV_MMC_USBDP, "div_mmc_usbdp", "clk_mmc_usbdp_andgt",
		0x710, 0xf, 16, 1, 0, "div_mmc_usbdp" },
	{ PCLK_DIV_MMC1_PCIE, "pclk_pcie_div", "pclk_pcie_andgt",
		0xb4, 0x3c0, 16, 1, 0, "pclk_pcie_div" },
	{ CLK_DIV_PERF_STAT, "clk_perf_div", "clk_perf_gt",
		0x0D0, 0xf000, 16, 1, 0, "clk_perf_div" },
	{ CLK_DIV_DMABUS, "clk_dmabus_div", "autodiv_dmabus",
		0x0EC, 0x8000, 2, 1, 0, "clk_dmabus_div" },
	{ CLK_DIV_SOCP_DEFLATE, "clk_socpdef_div", "clk_socp_def_gt",
		0x700, 0x3e00, 32, 1, 0, "clk_socpdef_div" },
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_vcodbus_gt",
		0x0BC, 0x3f, 64, 1, 0, "clk_vcodbus_div" },
	{ CLK_DIV_MMC0BUS, "clk_div_mmc0bus", "div_sysbus_pll",
		0x0EC, 0x4000, 2, 1, 0, "clk_div_mmc0bus" },
	{ CLK_DIV_SD, "clk_sd_div", "clk_sd_gt",
		0xB8, 0xf, 16, 1, 0, "clk_sd_div" },
	{ CLK_DIV_UARTH, "clkdiv_uarth", "clkgt_uarth",
		0xB0, 0xf000, 16, 1, 0, "clkdiv_uarth" },
	{ CLK_DIV_UARTL, "clkdiv_uartl", "clkgt_uartl",
		0xB0, 0xf00, 16, 1, 0, "clkdiv_uartl" },
	{ CLK_DIV_UART0, "clkdiv_uart0", "clkgt_uart0",
		0xB0, 0xf0, 16, 1, 0, "clkdiv_uart0" },
	{ CLK_DIV_I2C, "clkdiv_i2c", "sc_div_320m",
		0xE8, 0xf0, 16, 1, 0, "clkdiv_i2c" },
	{ CLK_DIV_I3C4, "clkdiv_i3c4", "clkgt_i3c4",
		0x710, 0xc00, 4, 1, 0, "clkdiv_i3c4" },
	{ CLK_DIV_SPI, "clkdiv_spi", "clkgt_spi",
		0xC4, 0xf000, 16, 1, 0, "clkdiv_spi" },
	{ CLK_DIV_AO_ASP, "clk_ao_asp_div", "clk_ao_asp_gt",
		0x108, 0x3c0, 16, 1, 0, "clk_ao_asp_div" },
	{ CLK_DIV_PTP, "clk_ptp_div", "clk_ptp_gt",
		0xD8, 0xf, 16, 1, 0, "clk_ptp_div" },
	{ CLK_BLPWM_PERI_DIV, "div_blpwm_peri", "clk_blpwm_peri_gt",
		0xEC, 0x30, 4, 1, 0, "div_blpwm_peri" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clkgt_vdec",
		0xC4, 0x3f, 64, 1, 0, "clkdiv_vdec" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clkgt_venc",
		0xC0, 0x3f00, 64, 1, 0, "clkdiv_venc" },
	{ CLK_DIV_DPCTRL_16M, "div_dpctrl16m", "clk_fix_div_dpctrl",
		0x700, 0xc000, 4, 1, 0, "div_dpctrl16m" },
	{ CLK_DIV_ISP_I2C, "clk_div_isp_i2c", "clk_gt_isp_i2c",
		0xb8, 0x780, 16, 1, 0, "clk_div_isp_i2c" },
	{ CLK_DIV_VENC2, "clkdiv_venc2", "clkgt_venc2",
		0x0C0, 0x3f, 64, 1, 0, "clkdiv_venc2" },
	{ CLK_ISP_SNCLK_DIV0, "clk_div_ispsn0", "clk_fac_ispsn",
		0x108, 0x3, 4, 1, 0, "clk_div_ispsn0" },
	{ CLK_ISP_SNCLK_DIV1, "clk_div_ispsn1", "clk_fac_ispsn",
		0x10C, 0xc000, 4, 1, 0, "clk_div_ispsn1" },
	{ CLK_ISP_SNCLK_DIV2, "clk_div_ispsn2", "clk_fac_ispsn",
		0x10C, 0x1800, 4, 1, 0, "clk_div_ispsn2" },
	{ CLK_ISP_SNCLK_DIV3, "clk_div_ispsn3", "clk_fac_ispsn",
		0x100, 0x300, 4, 1, 0, "clk_div_ispsn3" },
	{ CLK_DIV_LOADMONITOR, "clk_loadmonitor_div", "clk_loadmonitor_gt",
		0x0b8, 0xc000, 4, 1, 0, "clk_loadmonitor_div" },
	{ CLK_DIV_IVP32DSP_CORE, "clkdiv_ivpdsp", "clkgt_ivp32dsp",
		0x0B4, 0x3f, 64, 1, 0, "clkdiv_ivpdsp" },
	{ CLK_DIV_AO_HIFD, "clk_ao_hifd_div", "clkgt_ao_hifd",
		0x104, 0xc000, 4, 1, 0, "clk_ao_hifd_div" },
};

static const char * const clk_mux_wd0_p [] = { "clkin_ref", "pclk_wd0_high" };
static const char * const codeccssi_mux_p [] = { "clk_sys_ini", "clk_sys_ini" };
static const char * const clk_mux_vcodecbus_p [] = {
		"clk_invalid", "clk_ap_ppll0", "clk_sys_ini", "clk_invalid",
		"clk_ap_ppll2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ap_ppll3", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_sd_sys_p [] = { "clk_sd_sys", "clk_sd_div" };
static const char * const clk_mux_sd_pll_p [] = { "clk_ap_ppll0", "clk_ap_ppll3", "clk_ap_ppll2", "clk_ap_ppll2" };
static const char * const clk_mux_a53hpm_p [] = { "clk_ap_ppll0", "clk_ap_ppll2" };
static const char * const clk_mux_320m_p [] = { "clk_ap_ppll2", "clk_ap_ppll0" };
static const char * const clk_mux_uarth_p [] = { "clk_sys_ini", "clkdiv_uarth" };
static const char * const clk_mux_uartl_p [] = { "clk_sys_ini", "clkdiv_uartl" };
static const char * const clk_mux_uart0_p [] = { "clk_sys_ini", "clkdiv_uart0" };
static const char * const clk_mux_i2c_p [] = { "clk_sys_ini", "clkdiv_i2c" };
static const char * const clk_mux_spi_p [] = { "clk_sys_ini", "clkdiv_spi" };
static const char * const clk_mux_ao_asp_p [] = { "clk_ap_ppll2", "clk_ap_ppll0", "clk_ap_ppll3", "clk_ap_ppll3" };
static const char * const clk_blpwm_peri_mux_p [] = { "sc_div_320m", "clk_fll_src" };
static const char * const clk_mux_vdec_p [] = {
		"clk_invalid", "clk_invalid", "clk_ap_ppll0", "clk_invalid",
		"clk_ap_ppll2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ap_ppll3", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_venc_p [] = {
		"clk_invalid", "clk_invalid", "clk_ap_ppll0", "clk_invalid",
		"clk_ap_ppll2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ap_ppll3", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_venc2_p [] = {
		"clk_invalid", "clk_invalid", "clk_ap_ppll0", "clk_invalid",
		"clk_ap_ppll2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ap_ppll3", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_isp_snclk_mux0_p [] = { "clk_sys_ini", "clk_div_ispsn0" };
static const char * const clk_isp_snclk_mux1_p [] = { "clk_sys_ini", "clk_div_ispsn1" };
static const char * const clk_isp_snclk_mux2_p [] = { "clk_sys_ini", "clk_div_ispsn2" };
static const char * const clk_isp_snclk_mux3_p [] = { "clk_sys_ini", "clk_div_ispsn3" };
static const char * const clk_mux_rxdphy_cfg_p [] = { "clk_rxdcfg_fac", "clk_sys_ini" };
static const char * const clk_mux_ivp32dsp_core_p [] = {
		"clk_invalid", "clk_invalid", "clk_ap_ppll0", "clk_invalid",
		"clk_ap_ppll2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ap_ppll3", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_ao_hifd_mux_p [] = { "clk_ap_ppll0", "clk_ap_ppll2" };
static const struct mux_clock crgctrl_mux_clks[] = {
	{ CLK_MUX_WD0, "clk_wd0_mux", clk_mux_wd0_p,
		ARRAY_SIZE(clk_mux_wd0_p), 0x140, 0x20000, 0, "clk_wd0_mux" },
	{ CODECCSSI_MUX, "sel_codeccssi", codeccssi_mux_p,
		ARRAY_SIZE(codeccssi_mux_p), 0x0AC, 0x80, CLK_MUX_HIWORD_MASK, "sel_codeccssi" },
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", clk_mux_vcodecbus_p,
		ARRAY_SIZE(clk_mux_vcodecbus_p), 0x0C8, 0xf, CLK_MUX_HIWORD_MASK, "clk_vcodbus_mux" },
	{ CLK_MUX_SD_SYS, "clk_sd_muxsys", clk_mux_sd_sys_p,
		ARRAY_SIZE(clk_mux_sd_sys_p), 0x0B8, 0x40, CLK_MUX_HIWORD_MASK, "clk_sd_muxsys" },
	{ CLK_MUX_SD_PLL, "clk_sd_muxpll", clk_mux_sd_pll_p,
		ARRAY_SIZE(clk_mux_sd_pll_p), 0x0B8, 0x30, CLK_MUX_HIWORD_MASK, "clk_sd_muxpll" },
	{ CLK_MUX_A53HPM, "clk_a53hpm_mux", clk_mux_a53hpm_p,
		ARRAY_SIZE(clk_mux_a53hpm_p), 0x0D4, 0x200, CLK_MUX_HIWORD_MASK, "clk_a53hpm_mux" },
	{ CLK_MUX_320M, "sc_sel_320m_pll", clk_mux_320m_p,
		ARRAY_SIZE(clk_mux_320m_p), 0x100, 0x1, CLK_MUX_HIWORD_MASK, "sc_sel_320m_pll" },
	{ CLK_MUX_UARTH, "clkmux_uarth", clk_mux_uarth_p,
		ARRAY_SIZE(clk_mux_uarth_p), 0xAC, 0x10, CLK_MUX_HIWORD_MASK, "clkmux_uarth" },
	{ CLK_MUX_UARTL, "clkmux_uartl", clk_mux_uartl_p,
		ARRAY_SIZE(clk_mux_uartl_p), 0xAC, 0x8, CLK_MUX_HIWORD_MASK, "clkmux_uartl" },
	{ CLK_MUX_UART0, "clkmux_uart0", clk_mux_uart0_p,
		ARRAY_SIZE(clk_mux_uart0_p), 0xAC, 0x4, CLK_MUX_HIWORD_MASK, "clkmux_uart0" },
	{ CLK_MUX_I2C, "clkmux_i2c", clk_mux_i2c_p,
		ARRAY_SIZE(clk_mux_i2c_p), 0xAC, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_i2c" },
	{ CLK_MUX_SPI, "clkmux_spi", clk_mux_spi_p,
		ARRAY_SIZE(clk_mux_spi_p), 0xAC, 0x100, CLK_MUX_HIWORD_MASK, "clkmux_spi" },
	{ CLK_MUX_AO_ASP, "clkmux_ao_asp", clk_mux_ao_asp_p,
		ARRAY_SIZE(clk_mux_ao_asp_p), 0x100, 0x30, CLK_MUX_HIWORD_MASK, "clkmux_ao_asp" },
	{ CLK_BLPWM_PERI_MUX, "sel_blpwm", clk_blpwm_peri_mux_p,
		ARRAY_SIZE(clk_blpwm_peri_mux_p), 0x10C, 0x20, CLK_MUX_HIWORD_MASK, "sel_blpwm" },
	{ CLK_MUX_VDEC, "clkmux_vdec", clk_mux_vdec_p,
		ARRAY_SIZE(clk_mux_vdec_p), 0xC8, 0xf00, CLK_MUX_HIWORD_MASK, "clkmux_vdec" },
	{ CLK_MUX_VENC, "clkmux_venc", clk_mux_venc_p,
		ARRAY_SIZE(clk_mux_venc_p), 0xC8, 0xf0, CLK_MUX_HIWORD_MASK, "clkmux_venc" },
	{ CLK_MUX_VENC2, "clkmux_venc2", clk_mux_venc2_p,
		ARRAY_SIZE(clk_mux_venc2_p), 0x0C8, 0xf000, CLK_MUX_HIWORD_MASK, "clkmux_venc2" },
	{ CLK_ISP_SNCLK_MUX0, "clk_mux_ispsn0", clk_isp_snclk_mux0_p,
		ARRAY_SIZE(clk_isp_snclk_mux0_p), 0x108, 0x8, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn0" },
	{ CLK_ISP_SNCLK_MUX1, "clk_mux_ispsn1", clk_isp_snclk_mux1_p,
		ARRAY_SIZE(clk_isp_snclk_mux1_p), 0x10C, 0x2000, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn1" },
	{ CLK_ISP_SNCLK_MUX2, "clk_mux_ispsn2", clk_isp_snclk_mux2_p,
		ARRAY_SIZE(clk_isp_snclk_mux2_p), 0x10C, 0x400, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn2" },
	{ CLK_ISP_SNCLK_MUX3, "clk_mux_ispsn3", clk_isp_snclk_mux3_p,
		ARRAY_SIZE(clk_isp_snclk_mux3_p), 0x100, 0x80, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn3" },
	{ CLK_MUX_RXDPHY_CFG, "clk_rxdcfg_mux", clk_mux_rxdphy_cfg_p,
		ARRAY_SIZE(clk_mux_rxdphy_cfg_p), 0x0C4, 0x100, CLK_MUX_HIWORD_MASK, "clk_rxdcfg_mux" },
	{ CLK_MUX_IVP32DSP_CORE, "clkmux_ivp32dsp", clk_mux_ivp32dsp_core_p,
		ARRAY_SIZE(clk_mux_ivp32dsp_core_p), 0x708, 0xf000, CLK_MUX_HIWORD_MASK, "clkmux_ivp32dsp" },
	{ CLK_AO_HIFD_MUX, "clkmux_ao_hifd", clk_ao_hifd_mux_p,
		ARRAY_SIZE(clk_ao_hifd_mux_p), 0x100, 0x40, CLK_MUX_HIWORD_MASK, "clkmux_ao_hifd" },
};

static const struct div_clock hsdtctrl_div_clks[] = {
	{ CLK_DIV_SDIO, "clk_sdio_div", "clk_sdio_gt",
		0x0A8, 0xf, 16, 1, 0, "clk_sdio_div" },
	{ CLKDIV_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao_div", "clk_dp_audio_pll_ao_gt",
		0xB0, 0xf, 16, 1, 0, "clk_dp_audio_pll_ao_div" },
};

static const char * const clk_mux_sdio_sys_p [] = { "clk_sdio_sys", "clk_sdio_div" };
static const struct mux_clock hsdtctrl_mux_clks[] = {
	{ CLK_MUX_SDIO_SYS, "clk_sdio_muxsy", clk_mux_sdio_sys_p,
		ARRAY_SIZE(clk_mux_sdio_sys_p), 0x0A8, 0x2000, CLK_MUX_HIWORD_MASK, "clk_sdio_muxsy" },
};

static const struct gate_clock hsdtctrl_gate_clks[] = {
	{ CLK_GATE_PCIEPHY_REF, "clk_pciephy_ref", "clk_ppll_pcie", 0x010, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pciephy_ref" },
	{ CLK_GATE_PCIE1PHY_REF, "clk_pcie1phy_ref", "clk_ppll_pcie", 0x010, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie1phy_ref" },
	{ PCLK_GATE_PCIE_SYS, "pclk_pcie_sys", "clk_spll", 0x010, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_sys" },
	{ PCLK_GATE_PCIE1_SYS, "pclk_pcie1_sys", "clk_spll", 0x010, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_sys" },
	{ PCLK_GATE_PCIE_PHY, "pclk_pcie_phy", "pclk_mmc1_pcie", 0x010, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_phy" },
	{ PCLK_GATE_PCIE1_PHY, "pclk_pcie1_phy", "pclk_mmc1_pcie", 0x010, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_phy" },
	{ HCLK_GATE_SDIO, "hclk_sdio", "clk_spll", 0x000, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sdio" },
	{ CLK_GATE_SDIO, "clk_sdio", "clk_sdio_muxsy", 0x00, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 2, "clk_sdio" },
	{ CLK_GATE_PCIEAUX, "clk_pcieaux", "clk_sys_ini", 0x010, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcieaux" },
	{ CLK_GATE_PCIEAUX1, "clk_pcieaux1", "clk_sys_ini", 0x010, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcieaux1" },
	{ ACLK_GATE_PCIE, "aclk_pcie", "clk_spll", 0x010, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie" },
	{ ACLK_GATE_PCIE1, "aclk_pcie1", "clk_spll", 0x010, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie1" },
	{ CLK_GATE_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao", "clk_dp_audio_pll_ao_div", 0x00, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dp_audio_pll_ao" },
};

static const struct pll_clock hsdtctrl_pll_clks[] = {
	{ CLK_GATE_SCPLL, "clk_ap_scpll", "clk_scpll", 0x8,
		{0x200, 0}, {0x204, 26}, {0x200, 1}, {0x208, 0}, HS_HSDTCRG, 1, "clk_ap_scpll" },
};

static const struct scgt_clock hsdtctrl_scgt_clks[] = {
	{ CLK_ANDGT_SDIO, "clk_sdio_gt", "clk_spll",
		0x0AC, 1, CLK_GATE_HIWORD_MASK, "clk_sdio_gt" },
	{ CLK_SDIO_SYS_GT, "clk_sdio_sys_gt", "clk_sys_ini",
		0x0AC, 0, CLK_GATE_HIWORD_MASK, "clk_sdio_sys_gt" },
	{ CLKGT_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao_gt", "clk_ap_scpll",
		0xAC, 4, CLK_GATE_HIWORD_MASK, "clk_dp_audio_pll_ao_gt" },
};

static const struct gate_clock mmc0ctrl_gate_clks[] = {
	{ HCLK_GATE_USB3OTG, "hclk_usb3otg", "clk_mmc_usbdp", 0x0, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_usb3otg" },
	{ ACLK_GATE_USB3OTG, "aclk_usb3otg", "clk_mmc_usbdp", 0x0, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_usb3otg" },
	{ HCLK_GATE_SD, "hclk_sd", "clk_div_mmc0bus", 0x0, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sd" },
	{ PCLK_GATE_DPCTRL, "pclk_dpctrl", "clk_div_mmc0bus", 0x00, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl" },
	{ ACLK_GATE_DPCTRL, "aclk_dpctrl", "clk_mmc_usbdp", 0x00, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_dpctrl" },
};

static const struct div_clock sctrl_div_clks[] = {
	{ CLK_DIV_AOBUS, "sc_div_aobus", "clk_spll",
		0x254, 0x3f, 64, 1, 0, "sc_div_aobus" },
	{ CLK_DIV_IOPERI, "sc_div_ioperi", "clk_gt_ioperi",
		0x270, 0x3f, 64, 1, 0, "sc_div_ioperi" },
	{ CLKDIV_ASP_CODEC, "clkdiv_asp_codec", "clkgt_asp_codec",
		0x254, 0xf80, 32, 1, 0, "clkdiv_asp_codec" },
	{ CLK_DIV_AO_LOADMONITOR, "clk_ao_loadmonitor_div", "clk_ao_loadmonitor_gt",
		0x26c, 0x3c00, 16, 1, 0, "clk_ao_loadmonitor_div" },
	{ CLK_DIV_HIFD_FLL, "clk_hifd_fll_div", "clkgt_hifd_fll",
		0x284, 0x30, 4, 1, 0, "clk_hifd_fll_div" },
};

static const char * const clk_mux_timer5_a_p [] = { "clkin_ref", "apb_pclk", "clk_factor_tcxo", "apb_pclk" };
static const char * const clk_mux_timer5_b_p [] = { "clkin_ref", "apb_pclk", "clk_factor_tcxo", "apb_pclk" };
static const char * const clk_usb2phy_ref_mux_p [] = { "clkin_sys_div", "clk_lbintpll" };
static const char * const clkmux_syscnt_p [] = { "clk_syscnt_div", "clkin_ref" };
static const char * const clk_mux_asp_codec_p [] = { "clkdiv_asp_codec", "clk_asp_backup" };
static const char * const clk_mux_asp_pll_p [] = {
		"clk_invalid", "clk_spll", "clk_fll_src", "clk_invalid",
		"sel_ao_asp", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_sys_ini", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_ao_asp_mux_p [] = { "clk_ao_asp", "clk_lbintpll_1" };
static const char *const clk_mux_ao_camera_p [] = {"clkin_sys_div", "clk_hifd_fll", "clk_hifd_pll", "clk_isp_snclk1"};
static const char * const clk_sw_ao_loadmonitor_p [] = { "clk_fll_src", "clk_spll" };
static const struct mux_clock sctrl_mux_clks[] = {
	{ CLK_MUX_TIMER5_A, "clkmux_timer5_a", clk_mux_timer5_a_p,
		ARRAY_SIZE(clk_mux_timer5_a_p), 0x3c4, 0x30, 0, "clkmux_timer5_a" },
	{ CLK_MUX_TIMER5_B, "clkmux_timer5_b", clk_mux_timer5_b_p,
		ARRAY_SIZE(clk_mux_timer5_b_p), 0x3c4, 0xc0, 0, "clkmux_timer5_b" },
	{ CLK_USB2PHY_REF_MUX, "clk_usb2phy_ref_mux", clk_usb2phy_ref_mux_p,
		ARRAY_SIZE(clk_usb2phy_ref_mux_p), 0x264, 0x4000, CLK_MUX_HIWORD_MASK, "clk_usb2phy_ref_mux" },
	{ CLKMUX_SYSCNT, "clkmux_syscnt", clkmux_syscnt_p,
		ARRAY_SIZE(clkmux_syscnt_p), 0x264, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_syscnt" },
	{ CLK_MUX_ASP_CODEC, "sel_asp_codec", clk_mux_asp_codec_p,
		ARRAY_SIZE(clk_mux_asp_codec_p), 0x260, 0x8000, CLK_MUX_HIWORD_MASK, "sel_asp_codec" },
	{ CLK_MUX_ASP_PLL, "clk_asp_pll_sel", clk_mux_asp_pll_p,
		ARRAY_SIZE(clk_mux_asp_pll_p), 0x280, 0xf, CLK_MUX_HIWORD_MASK, "clk_asp_pll_sel" },
	{ CLK_AO_ASP_MUX, "sel_ao_asp", clk_ao_asp_mux_p,
		ARRAY_SIZE(clk_ao_asp_mux_p), 0x250, 0x1000, CLK_MUX_HIWORD_MASK, "sel_ao_asp" },
	{CLK_MUX_AO_CAMERA, "mux_ao_camera", clk_mux_ao_camera_p,
		ARRAY_SIZE(clk_mux_ao_camera_p), 0x290, 0x30, CLK_MUX_HIWORD_MASK, "mux_ao_camera"},
	{ CLK_SW_AO_LOADMONITOR, "clk_ao_loadmonitor_sw", clk_sw_ao_loadmonitor_p,
		ARRAY_SIZE(clk_sw_ao_loadmonitor_p), 0x26c, 0x100, CLK_MUX_HIWORD_MASK, "clk_ao_loadmonitor_sw" },
};

static const struct gate_clock sctrl_gate_clks[] = {
	{ PCLK_GPIO20, "pclk_gpio20", "sc_div_aobus", 0x1B0, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio20" },
	{ PCLK_GPIO21, "pclk_gpio21", "sc_div_aobus", 0x1B0, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio21" },
	{ CLK_GATE_TIMER5_B, "clk_timer5_b", "clkmux_timer5_b", 0, 0x0, 0,
		"clk_timer5", 0, {0}, {0}, 0, 0, 0, "clk_timer5_b" },
	{ CLK_GATE_TIMER5, "clk_timer5", "sc_div_aobus", 0x170, 0x3000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5" },
	{ CLK_GATE_SPI, "clk_spi3", "sc_div_ioperi", 0x1B0, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 84, 0, "clk_spi3" },
	{ PCLK_GATE_SPI, "pclk_spi3", "sc_div_ioperi", 0x1B0, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi3" },
	{ CLK_GATE_USB2PHY_REF, "clk_usb2phy_ref", "clk_usb2phy_ref_mux", 0x1B0, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref" },
	{ PCLK_GATE_RTC, "pclk_rtc", "sc_div_aobus", 0x160, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_rtc" },
	{ PCLK_GATE_RTC1, "pclk_rtc1", "sc_div_aobus", 0x160, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_rtc1" },
	{ PCLK_AO_GPIO0, "pclk_ao_gpio0", "sc_div_aobus", 0x160, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio0" },
	{ PCLK_AO_GPIO1, "pclk_ao_gpio1", "sc_div_aobus", 0x160, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio1" },
	{ PCLK_AO_GPIO2, "pclk_ao_gpio2", "sc_div_aobus", 0x160, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio2" },
	{ PCLK_AO_GPIO3, "pclk_ao_gpio3", "sc_div_aobus", 0x160, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio3" },
	{ PCLK_AO_GPIO4, "pclk_ao_gpio4", "sc_div_aobus", 0x160, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio4" },
	{ PCLK_AO_GPIO5, "pclk_ao_gpio5", "sc_div_aobus", 0x160, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio5" },
	{ PCLK_AO_GPIO6, "pclk_ao_gpio6", "sc_div_aobus", 0x160, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio6" },
	{ PCLK_AO_GPIO29, "pclk_ao_gpio29", "sc_div_aobus", 0x1B0, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio29" },
	{ PCLK_AO_GPIO30, "pclk_ao_gpio30", "sc_div_aobus", 0x1B0, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio30" },
	{ PCLK_AO_GPIO31, "pclk_ao_gpio31", "sc_div_aobus", 0x1B0, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio31" },
	{ PCLK_AO_GPIO32, "pclk_ao_gpio32", "sc_div_aobus", 0x1B0, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio32" },
	{ PCLK_AO_GPIO33, "pclk_ao_gpio33", "sc_div_aobus", 0x1B0, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio33" },
	{ PCLK_AO_GPIO34, "pclk_ao_gpio34", "sc_div_aobus", 0x1B0, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio34" },
	{ PCLK_AO_GPIO35, "pclk_ao_gpio35", "sc_div_aobus", 0x1B0, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio35" },
	{ PCLK_AO_GPIO36, "pclk_ao_gpio36", "sc_div_aobus", 0x1B0, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio36" },
	{ PCLK_GATE_SYSCNT, "pclk_syscnt", "sc_div_aobus", 0x160, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_syscnt" },
	{ CLK_GATE_SYSCNT, "clk_syscnt", "clkmux_syscnt", 0x160, 0x100000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_syscnt" },
	{ CLK_ASP_BACKUP, "clk_asp_backup", "clk_lbintpll_1", 0x160, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_backup" },
	{ CLK_ASP_CODEC, "clk_asp_codec", "sel_asp_codec", 0x170, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_codec" },
	{ CLK_GATE_ASP_SUBSYS, "clk_asp_subsys", "clk_asp_pll_sel", 0x170, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_subsys" },
	{ CLK_GATE_ASP_TCXO, "clk_asp_tcxo", "clk_sys_ini", 0x160, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_tcxo" },
	{ CLK_GATE_DP_AUDIO_PLL, "clk_dp_audio_pll", "clk_dp_audio_pll_ao", 0x1B0, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dp_audio_pll" },
	{CLK_GATE_AO_CAMERA, "clk_ao_camera", "div_ao_camera", 0x170, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_camera"},
	{ PCLK_GATE_AO_LOADMONITOR, "pclk_ao_loadmonitor", "sc_div_aobus", 0x1b0, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_loadmonitor" },
	{ CLK_GATE_AO_LOADMONITOR, "clk_ao_loadmonitor", "clk_ao_loadmonitor_div", 0x1b0, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_loadmonitor" },
	{ CLK_GATE_HIFD_TCXO, "clk_hifd_tcxo", "clk_sys_ini", 0x190, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_tcxo" },
	{ CLK_GATE_HIFD_FLL, "clk_hifd_fll", "clk_hifd_fll_div", 0x170, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_fll" },
	{ CLK_GATE_HIFD_PLL, "clk_hifd_pll", "clk_hifd_pll_div", 0x170, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_pll" },
};

static const struct scgt_clock sctrl_scgt_clks[] = {
	{ CLK_ANDGT_IOPERI, "clk_gt_ioperi", "clk_spll",
		0x270, 6, CLK_GATE_HIWORD_MASK, "clk_gt_ioperi" },
	{ CLKGT_ASP_CODEC, "clkgt_asp_codec", "clk_spll_vote",
		0x274, 14, CLK_GATE_HIWORD_MASK, "clkgt_asp_codec" },
	{CLK_ANDGT_AO_CAMERA, "clkgt_ao_camera", "mux_ao_camera",
		0x290, 14, CLK_GATE_HIWORD_MASK, "clkgt_ao_camera"},
	{ CLK_GT_AO_LOADMONITOR, "clk_ao_loadmonitor_gt", "clk_ao_loadmonitor_sw",
		0x26c, 9, CLK_GATE_HIWORD_MASK, "clk_ao_loadmonitor_gt" },
	{ CLK_ANDGT_HIFD_FLL, "clkgt_hifd_fll", "clk_fll_src",
		0x254, 15, CLK_GATE_HIWORD_MASK, "clkgt_hifd_fll" },
	{CLK_ANDGT_HIFD_PLL, "clkgt_hifd_pll", "clk_spll",
		0x258, 5, CLK_GATE_HIWORD_MASK, "clkgt_hifd_pll" },
};

static const struct gate_clock iomcu_gate_clks[] = {
	{ CLK_I2C1_GATE_IOMCU, "clk_i2c1_gt", "clk_fll_src", 0x0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c1_gt" },
};

static const struct div_clock media1crg_div_clks[] = {
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_vivobus_gt",
		0x74, 0x3f, 64, 1, 0, "clk_vivobus_div" },
	{ CLK_DIV_EDC0, "clk_edc0_div", "clk_edc0_gt",
		0x68, 0x3f, 64, 1, 0, "clk_edc0_div" },
	{ CLK_DIV_LDI1, "clkdiv_ldi1", "clkgt_ldi1",
		0x64, 0x3f, 64, 1, 0, "clkdiv_ldi1" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_ispcpu_gt",
		0x70, 0x3f, 64, 1, 0, "clk_ispcpu_div" },
	{ CLK_DIV_ISPFUNC, "clkdiv_ispfunc", "clkgt_ispfunc",
		0x6c, 0x3f, 64, 1, 0, "clkdiv_ispfunc" },
	{ CLK_DIV_ISPFUNC2, "clkdiv_ispfunc2", "clkgt_ispfunc2",
		0x88, 0x3f0, 64, 1, 0, "clkdiv_ispfunc2" },
	{ CLK_DIV_ISPFUNC3, "clkdiv_ispfunc3", "clkgt_ispfunc3",
		0x88, 0xfc00, 64, 1, 0, "clkdiv_ispfunc3" },
	{ CLK_DIV_ISPFUNC4, "clkdiv_ispfunc4", "clkgt_ispfunc4",
		0x74, 0xfc00, 64, 1, 0, "clkdiv_ispfunc4" },
	{ CLK_DIV_ISP_I3C, "clkdiv_isp_i3c", "clkgt_isp_i3c",
		0x064, 0xfc00, 64, 1, 0, "clkdiv_isp_i3c" },
	{ CLK_MEDIA_COMMON_DIV, "clkdiv_media_common", "clkgt_media_common",
		0x80, 0xfc0, 64, 1, 0, "clkdiv_media_common" },
	{ CLK_DIV_FD_FUNC, "clk_fd_func_div", "clkgt_fd_func",
		0x7C, 0x3f0, 64, 1, 0, "clk_fd_func_div" },
	{ CLK_DIV_JPG_FUNC, "clk_jpg_func_div", "clkgt_jpg_func",
		0x80, 0x3f, 64, 1, 0, "clk_jpg_func_div" },
	{ CLK_DIV_FDAI_FUNC, "clk_fdai_func_div", "clkgt_fdai_func",
		0x98, 0x3f, 64, 1, 0, "clk_fdai_func_div" },
	{ ACLK_DIV_MMBUF, "aclk_mmbuf_div", "clk_mmbuf_gt",
		0x7C, 0xfc00, 64, 1, 0, "aclk_mmbuf_div" },
	{ PCLK_DIV_MMBUF, "pclk_mmbuf_div", "pclk_mmbuf_gt",
		0x78, 0x3, 4, 1, 0, "pclk_mmbuf_div" },
};

static const char * const clk_mux_vivobus_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_edc0_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ldi1_p [] = {
		"clk_invalid", "clk_ppll7_media", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispi2c_p [] = {"clk_media_tcxo", "clk_isp_i2c_media"};
static const char * const clk_mux_ispcpu_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc2_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc3_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc4_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_media_common_sw_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_fd_func_mux_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_ppll2_media", "clk_ppll3_media"
};
static const char * const clk_jpg_func_mux_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_fdai_func_mux_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_ppll2_media", "clk_ppll3_media"
};
static const char * const clk_sw_mmbuf_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll3_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media1crg_mux_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", clk_mux_vivobus_p,
		ARRAY_SIZE(clk_mux_vivobus_p), 0x74, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vivobus_mux" },
	{ CLK_MUX_EDC0, "sel_edc0_pll", clk_mux_edc0_p,
		ARRAY_SIZE(clk_mux_edc0_p), 0x68, 0x3c0, CLK_MUX_HIWORD_MASK, "sel_edc0_pll" },
	{ CLK_MUX_LDI1, "clkmux_ldi1", clk_mux_ldi1_p,
		ARRAY_SIZE(clk_mux_ldi1_p), 0x64, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_ldi1" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", clk_mux_ispi2c_p,
		ARRAY_SIZE(clk_mux_ispi2c_p), 0x78, 0x4, CLK_MUX_HIWORD_MASK, "clk_ispi2c_mux" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", clk_mux_ispcpu_p,
		ARRAY_SIZE(clk_mux_ispcpu_p), 0x60, 0x3c00, CLK_MUX_HIWORD_MASK, "sel_ispcpu" },
	{ CLK_MUX_ISPFUNC, "clkmux_ispfunc", clk_mux_ispfunc_p,
		ARRAY_SIZE(clk_mux_ispfunc_p), 0x6c, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_ispfunc" },
	{ CLK_MUX_ISPFUNC2, "clkmux_ispfunc2", clk_mux_ispfunc2_p,
		ARRAY_SIZE(clk_mux_ispfunc2_p), 0x6c, 0x3c00, CLK_MUX_HIWORD_MASK, "clkmux_ispfunc2" },
	{ CLK_MUX_ISPFUNC3, "clkmux_ispfunc3", clk_mux_ispfunc3_p,
		ARRAY_SIZE(clk_mux_ispfunc3_p), 0x70, 0x3c00, CLK_MUX_HIWORD_MASK, "clkmux_ispfunc3" },
	{ CLK_MUX_ISPFUNC4, "clkmux_ispfunc4", clk_mux_ispfunc4_p,
		ARRAY_SIZE(clk_mux_ispfunc4_p), 0x78, 0x78, CLK_MUX_HIWORD_MASK, "clkmux_ispfunc4" },
	{ CLK_MEDIA_COMMON_SW, "sel_media_common_pll", clk_media_common_sw_p,
		ARRAY_SIZE(clk_media_common_sw_p), 0x80, 0xf000, CLK_MUX_HIWORD_MASK, "sel_media_common_pll" },
	{ CLK_FD_FUNC_MUX, "clkmux_fd_func", clk_fd_func_mux_p,
		ARRAY_SIZE(clk_fd_func_mux_p), 0x70, 0x300, CLK_MUX_HIWORD_MASK, "clkmux_fd_func" },
	{ CLK_JPG_FUNC_MUX, "clkmux_jpg_func", clk_jpg_func_mux_p,
		ARRAY_SIZE(clk_jpg_func_mux_p), 0x60, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_jpg_func" },
	{ CLK_FDAI_FUNC_MUX, "clkmux_fdai_func", clk_fdai_func_mux_p,
		ARRAY_SIZE(clk_fdai_func_mux_p), 0x98, 0xc0, CLK_MUX_HIWORD_MASK, "clkmux_fdai_func" },
	{ CLK_SW_MMBUF, "aclk_mmbuf_sw", clk_sw_mmbuf_p,
		ARRAY_SIZE(clk_sw_mmbuf_p), 0x88, 0xf, CLK_MUX_HIWORD_MASK, "aclk_mmbuf_sw" },
};

static const struct gate_clock media1crg_gate_clks[] = {
	{ PCLK_GATE_ISP_NOC_SUBSYS, "pclk_isp_noc_subsys", "div_sysbus_pll", 0x10, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_isp_noc_subsys" },
	{ ACLK_GATE_ISP_NOC_SUBSYS, "aclk_isp_noc_subsys", "clk_ppll0", 0x10, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp_noc_subsys" },
	{ ACLK_GATE_MEDIA_COMMON, "aclk_media_common", "aclk_disp_noc_subsys", 0x10, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_media_common" },
	{ ACLK_GATE_NOC_DSS, "aclk_noc_dss", "aclk_disp_noc_subsys", 0x10, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_noc_dss" },
	{ PCLK_GATE_MEDIA_COMMON, "pclk_media_common", "pclk_disp_noc_subsys", 0x10, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media_common" },
	{ PCLK_GATE_NOC_DSS_CFG, "pclk_noc_dss_cfg", "pclk_disp_noc_subsys", 0x10, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_noc_dss_cfg" },
	{ PCLK_GATE_MMBUF_CFG, "pclk_mmbuf_cfg", "pclk_disp_noc_subsys", 0x20, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_mmbuf_cfg" },
	{ PCLK_GATE_DISP_NOC_SUBSYS, "pclk_disp_noc_subsys", "div_sysbus_pll", 0x10, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_disp_noc_subsys" },
	{ ACLK_GATE_DISP_NOC_SUBSYS, "aclk_disp_noc_subsys", "clk_ppll0", 0x10, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_disp_noc_subsys" },
	{ PCLK_GATE_DSS, "pclk_dss", "pclk_disp_noc_subsys", 0x00, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dss" },
	{ACLK_GATE_DSS, "aclk_dss", "aclk_disp_noc_subsys", 0x00, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 8, 0, "aclk_dss" },
	{ ACLK_GATE_ISP, "aclk_isp", "aclk_isp_noc_subsys", 0x00, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 7, 0, "aclk_isp" },
	{ CLK_GATE_VIVOBUS_TO_DDRC, "clk_vivobus2ddr", "clk_vivobus_div", 0x00, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vivobus2ddr" },
	{ CLK_GATE_EDC0, "clk_edc0", "clk_edc0_div", 0x00, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 20, 0, "clk_edc0" },
	{ CLK_GATE_LDI1, "clk_ldi1", "clkdiv_ldi1", 0x00, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 23, 0, "clk_ldi1" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_ispi2c_mux", 0x00, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispi2c" },
	{ CLK_GATE_ISP_SYS, "clk_isp_sys", "clk_media_tcxo", 0x00, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_sys" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_ispcpu_div", 0x00, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispcpu" },
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clkdiv_ispfunc", 0x00, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 26, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_ISPFUNC2FREQ, "clk_ispfunc2freq", "clkdiv_ispfunc2", 0x00, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 38, 0, "clk_ispfunc2freq" },
	{ CLK_GATE_ISPFUNC3FREQ, "clk_ispfunc3freq", "clkdiv_ispfunc3", 0x00, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 39, 0, "clk_ispfunc3freq" },
	{ CLK_GATE_ISPFUNC4FREQ, "clk_ispfunc4freq", "clkdiv_ispfunc4", 0x00, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 40, 0, "clk_ispfunc4freq" },
	{ CLK_GATE_ISP_I3C, "clk_isp_i3c", "clkdiv_isp_i3c", 0x010, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i3c" },
	{ CLK_GATE_MEDIA_COMMONFREQ, "clk_media_commonfreq", "clkdiv_media_common", 0x10, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 34, 0, "clk_media_commonfreq" },
	{ CLK_GATE_BRG, "clk_brg", "clkdiv_media_common", 0x00, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_brg" },
	{ PCLK_GATE_MEDIA1_LM, "pclk_media1_lm", "clk_ptp_div", 0x00, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media1_lm" },
	{ CLK_GATE_LOADMONITOR_MEDIA1, "clk_loadmonitor_media1", "clk_ptp_div", 0x00, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_media1" },
	{ CLK_GATE_FD_FUNC, "clk_fd_func", "clk_fd_func_div", 0x10, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fd_func" },
	{ CLK_GATE_JPG_FUNCFREQ, "clk_jpg_funcfreq", "clk_jpg_func_div", 0x10, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 36, 0, "clk_jpg_funcfreq" },
	{ ACLK_GATE_JPG, "aclk_jpg", "aclk_isp_noc_subsys", 0x10, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_jpg" },
	{ PCLK_GATE_JPG, "pclk_jpg", "pclk_isp_noc_subsys", 0x10, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_jpg" },
	{ ACLK_GATE_NOC_ISP, "aclk_noc_isp", "aclk_isp_noc_subsys", 0x10, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_noc_isp" },
	{ PCLK_GATE_NOC_ISP, "pclk_noc_isp", "pclk_isp_noc_subsys", 0x10, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_noc_isp" },
	{ CLK_GATE_FDAI_FUNCFREQ, "clk_fdai_funcfreq", "clk_fdai_func_div", 0x10, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 35, 0, "clk_fdai_funcfreq" },
	{ ACLK_GATE_ASC, "aclk_asc", "clk_mmbuf", 0x20, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_asc" },
	{ CLK_GATE_DSS_AXI_MM, "clk_dss_axi_mm", "clk_mmbuf", 0x20, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dss_axi_mm" },
	{ CLK_GATE_MMBUF, "clk_mmbuf", "aclk_mmbuf_div", 0x20, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mmbuf" },
	{ PCLK_GATE_MMBUF, "pclk_mmbuf", "pclk_mmbuf_div", 0x20, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 58, 0, "pclk_mmbuf" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_vivobus_div", 0x010, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vivo" },
	{ CLK_GATE_ATDIV_ISPCPU, "clk_atdiv_ispcpu", "clk_ispcpu_div", 0x010, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_ispcpu" },
};

static const struct scgt_clock media1crg_scgt_clks[] = {
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_vivobus_mux",
		0x84, 3, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vivobus_gt" },
	{ CLK_ANDGT_EDC0, "clk_edc0_gt", "sel_edc0_pll",
		0x84, 7, CLK_GATE_HIWORD_MASK, "clk_edc0_gt" },
	{ CLK_ANDGT_LDI1, "clkgt_ldi1", "clkmux_ldi1",
		0x84, 8, CLK_GATE_HIWORD_MASK, "clkgt_ldi1" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "sel_ispcpu",
		0x84, 5, CLK_GATE_HIWORD_MASK, "clk_ispcpu_gt" },
	{ CLK_ANDGT_ISPFUNC, "clkgt_ispfunc", "clkmux_ispfunc",
		0x84, 6, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc" },
	{ CLK_ANDGT_ISPFUNC2, "clkgt_ispfunc2", "clkmux_ispfunc2",
		0x84, 2, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc2" },
	{ CLK_ANDGT_ISPFUNC3, "clkgt_ispfunc3", "clkmux_ispfunc3",
		0x84, 1, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc3" },
	{ CLK_ANDGT_ISPFUNC4, "clkgt_ispfunc4", "clkmux_ispfunc4",
		0x84, 0, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc4" },
	{ CLK_ANDGT_ISP_I3C, "clkgt_isp_i3c", "clk_ppll0_media",
		0x084, 9, CLK_GATE_HIWORD_MASK, "clkgt_isp_i3c" },
	{ CLKANDGT_MEDIA_COMMON, "clkgt_media_common", "sel_media_common_pll",
		0x84, 11, CLK_GATE_HIWORD_MASK, "clkgt_media_common" },
	{ CLK_ANDGT_FD_FUNC, "clkgt_fd_func", "clkmux_fd_func",
		0x84, 13, CLK_GATE_HIWORD_MASK, "clkgt_fd_func" },
	{ CLK_ANDGT_JPG_FUNC, "clkgt_jpg_func", "clkmux_jpg_func",
		0x84, 12, CLK_GATE_HIWORD_MASK, "clkgt_jpg_func" },
	{ CLK_ANDGT_FDAI_FUNC, "clkgt_fdai_func", "clkmux_fdai_func",
		0x84, 10, CLK_GATE_HIWORD_MASK, "clkgt_fdai_func" },
	{ CLK_MMBUF_PLL_ANDGT, "clk_mmbuf_gt", "aclk_mmbuf_sw",
		0x84, 14, CLK_GATE_HIWORD_MASK, "clk_mmbuf_gt" },
	{ PCLK_MMBUF_ANDGT, "pclk_mmbuf_gt", "aclk_mmbuf_div",
		0x84, 15, CLK_GATE_HIWORD_MASK, "pclk_mmbuf_gt" },
};

static const struct gate_clock media2crg_gate_clks[] = {
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clkdiv_vdec", 0x00, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 21, 0, "clk_vdecfreq" },
	{ PCLK_GATE_VDEC, "pclk_vdec", "div_sysbus_pll", 0x00, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_vdec" },
	{ ACLK_GATE_VDEC, "aclk_vdec", "clk_vcodbus_div", 0x00, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_vdec" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clkdiv_venc", 0x00, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 24, 0, "clk_vencfreq" },
	{ PCLK_GATE_VENC, "pclk_venc", "div_sysbus_pll", 0x00, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_venc" },
	{ ACLK_GATE_VENC, "aclk_venc", "clk_vcodbus_div", 0x00, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_venc" },
	{ ACLK_GATE_VENC2, "aclk_venc2", "clk_vcodbus_div", 0x00, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_venc2" },
	{ CLK_GATE_VENC2FREQ, "clk_venc2freq", "clkdiv_venc2", 0x00, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 25, 0, "clk_venc2freq" },
	{ PCLK_GATE_MEDIA2_LM, "pclk_media2_lm", "clk_ptp_div", 0x00, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media2_lm" },
	{ CLK_GATE_LOADMONITOR_MEDIA2, "clk_loadmonitor_media2", "clk_ptp_div", 0x00, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_media2" },
	{ CLK_GATE_IVP32DSP_TCXO, "clk_ivpdsp_tcxo", "clk_media_tcxo", 0x00, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ivpdsp_tcxo" },
	{ CLK_GATE_IVP32DSP_COREFREQ, "clk_ivpdsp_corefreq", "clkdiv_ivpdsp", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 27, 0, "clk_ivpdsp_corefreq" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_vcodbus_div", 0x00, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clkdiv_vdec", 0x00, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vdec" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clkdiv_venc", 0x00, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_venc" },
};

static const struct scgt_clock pctrl_scgt_clks[] = {
	{ CLK_GATE_USB_TCXO_EN, "clk_usb_tcxo_en", "clk_abb_192",
		0x10, 0, CLK_GATE_HIWORD_MASK, "clk_usb_tcxo_en" },
};

static const struct xfreq_clock xfreq_clks[] = {
	{ CLK_CLUSTER0, "cpu-cluster.0", 0, 0, 0, 0x41C, {0x0001030A, 0x0}, {0x0001020A, 0x0}, "cpu-cluster.0" },
	{ CLK_CLUSTER1, "cpu-cluster.1", 1, 0, 0, 0x41C, {0x0002030A, 0x0}, {0x0002020A, 0x0}, "cpu-cluster.1" },
	{ CLK_G3D0, "clk_g3d", 2, 0, 0, 0x41C, {0x0003030A, 0x0}, {0x0003020A, 0x0}, "clk_g3d" },
	{ CLK_DDRC_FREQ0, "clk_ddrc_freq", 3, 0, 0, 0x41C, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_freq" },
#ifndef CONFIG_LIBLINUX
	{ CLK_DDRC_MAX0, "clk_ddrc_max", 5, 1, 0x8000, 0x250, {0x00040308, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_max" },
	{ CLK_DDRC_MIN0, "clk_ddrc_min", 4, 1, 0x8000, 0x270, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_min" },
#else
	{ CLK_DDRC_MAX0, "clk_ddrc_max", 5, 1, 0x8000, 0x25C, {0x00040308, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_max" },
	{ CLK_DDRC_MIN0, "clk_ddrc_min", 4, 1, 0x8000, 0x28C, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_min" },
#endif
};

static const struct pmu_clock pmu_clks[] = {
	{ CLK_GATE_ABB_192, "clk_abb_192", "clkin_sys", 0x03C, 0, 9, 0, "clk_abb_192" },
	{ CLK_PMU32KA, "clk_pmu32ka", "clkin_ref", 0x47, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32ka" },
	{ CLK_PMU32KB, "clk_pmu32kb", "clkin_ref", 0x46, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kb" },
	{ CLK_PMU32KC, "clk_pmu32kc", "clkin_ref", 0x45, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kc" },
	{ CLK_PMUAUDIOCLK, "clk_pmuaudioclk", "clk_sys_ini", 0x42, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmuaudioclk" },
};

static const struct mailbox_clock mailbox_clks[] = {
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_ppll0", 0,
		{0x000D0002, 0xC}, {0x000D0102, 0xC}, 0, "clk_vivobus" },
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_vcodbus_div", 0,
		{0x000D0002, 0xE}, {0x000D0102, 0xE}, 0, "clk_vcodecbus" },
	{ CLK_SPLL_VOTE, "clk_spll_vote", "clk_spll", 0,
		{0x000D0002, 0x0}, {0x000D0102, 0x0}, 0, "clk_spll_vote" },
};

static const struct dvfs_clock dvfs_clks[] = {
	{ CLK_GATE_VDEC, "clk_vdec", "clk_vdecfreq", 21, 5, 3, 1,
		{185000, 277000, 332000}, {0, 1, 2, 3}, 332000, 1, "clk_vdec" },
	{ CLK_GATE_VENC, "clk_venc", "clk_vencfreq", 24, 4, 3, 1,
		{207500, 332000, 480000}, {0, 1, 2, 3}, 480000, 1, "clk_venc" },
	{ CLK_GATE_VENC2, "clk_venc2", "clk_venc2freq", 25, 7, 3, 1,
		{207500, 332000, 480000}, {0, 1, 2, 3}, 480000, 1, "clk_venc2" },
	{ CLK_GATE_ISPFUNC, "clk_ispfunc", "clk_ispfuncfreq", 26, 2, 3, 1,
		{207500, 332000, 480000}, {0, 1, 2, 3}, 480000, 1, "clk_ispfunc" },
	{ CLK_GATE_JPG_FUNC, "clk_jpg_func", "clk_jpg_funcfreq", 36, -1, 3, 1,
		{238000, 332000, 415000}, {0, 1, 2, 3}, 415000, 1, "clk_jpg_func" },
	{ CLK_GATE_FDAI_FUNC, "clk_fdai_func", "clk_fdai_funcfreq", 35, -1, 3, 1,
		{384000, 480000, 554000}, {0, 1, 2, 3}, 554000, 1, "clk_fdai_func" },
	{ CLK_GATE_MEDIA_COMMON, "clk_media_common", "clk_media_commonfreq", 34, -1, 3, 1,
		{238000, 332000, 415000}, {0, 1, 2, 3}, 415000, 1, "clk_media_common" },
	{ CLK_GATE_IVP32DSP_CORE, "clk_ivpdsp_core", "clk_ivpdsp_corefreq", 27, 6, 3, 1,
		{480000, 554000, 640000}, {0, 1, 2, 3}, 640000, 1, "clk_ivpdsp_core" },
	{ CLK_GATE_ISPFUNC2, "clk_ispfunc2", "clk_ispfunc2freq", 38, -1, 3, 1,
		{207500, 332000, 480000}, {0, 1, 2, 3}, 480000, 1, "clk_ispfunc2" },
	{ CLK_GATE_ISPFUNC3, "clk_ispfunc3", "clk_ispfunc3freq", 39, -1, 3, 1,
		{166000, 300000, 400000}, {0, 1, 2, 3}, 400000, 1, "clk_ispfunc3" },
	{ CLK_GATE_ISPFUNC4, "clk_ispfunc4", "clk_ispfunc4freq", 40, -1, 3, 1,
		{185000, 300000, 400000}, {0, 1, 2, 3}, 400000, 1, "clk_ispfunc4" },
	{ CLK_GATE_HIFACE, "clk_hiface", "apb_pclk", 41, -1, 3, 1,
		{600000, 650000, 700000}, {0, 1, 2, 3}, 0, 0, "clk_hiface" },
};

static void clk_crgctrl_init(struct device_node *np)
{
	struct clock_data *clk_crgctrl_data;
	int nr = ARRAY_SIZE(fixed_clks) +
		ARRAY_SIZE(pll_clks) +
		ARRAY_SIZE(crgctrl_scgt_clks) +
		ARRAY_SIZE(fixed_factor_clks) +
		ARRAY_SIZE(crgctrl_div_clks) +
		ARRAY_SIZE(crgctrl_mux_clks) +
		ARRAY_SIZE(crgctrl_gate_clks);

	clk_crgctrl_data = clk_init(np, nr);
	if (!clk_crgctrl_data)
		return;

	plat_clk_register_fixed_rate(fixed_clks,
		ARRAY_SIZE(fixed_clks), clk_crgctrl_data);

	plat_clk_register_pll(pll_clks,
		ARRAY_SIZE(pll_clks), clk_crgctrl_data);

	plat_clk_register_scgt(crgctrl_scgt_clks,
		ARRAY_SIZE(crgctrl_scgt_clks), clk_crgctrl_data);

	plat_clk_register_fixed_factor(fixed_factor_clks,
		ARRAY_SIZE(fixed_factor_clks), clk_crgctrl_data);

	plat_clk_register_divider(crgctrl_div_clks,
		ARRAY_SIZE(crgctrl_div_clks), clk_crgctrl_data);

	plat_clk_register_mux(crgctrl_mux_clks,
		ARRAY_SIZE(crgctrl_mux_clks), clk_crgctrl_data);

	plat_clk_register_gate(crgctrl_gate_clks,
		ARRAY_SIZE(crgctrl_gate_clks), clk_crgctrl_data);
}

CLK_OF_DECLARE_DRIVER(clk_peri_crgctrl,
	"hisilicon,clk-extreme-crgctrl", clk_crgctrl_init);

static void clk_hsdt_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(hsdtctrl_pll_clks) +
		ARRAY_SIZE(hsdtctrl_scgt_clks) +
		ARRAY_SIZE(hsdtctrl_div_clks) +
		ARRAY_SIZE(hsdtctrl_mux_clks) +
		ARRAY_SIZE(hsdtctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_pll(hsdtctrl_pll_clks,
		ARRAY_SIZE(hsdtctrl_pll_clks), clk_data);

	plat_clk_register_scgt(hsdtctrl_scgt_clks,
		ARRAY_SIZE(hsdtctrl_scgt_clks), clk_data);

	plat_clk_register_divider(hsdtctrl_div_clks,
		ARRAY_SIZE(hsdtctrl_div_clks), clk_data);

	plat_clk_register_mux(hsdtctrl_mux_clks,
		ARRAY_SIZE(hsdtctrl_mux_clks), clk_data);

	plat_clk_register_gate(hsdtctrl_gate_clks,
		ARRAY_SIZE(hsdtctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdtcrg,
	"hisilicon,clk-extreme-hsdt-crg", clk_hsdt_init);

static void clk_mmc0_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(mmc0ctrl_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_gate(mmc0ctrl_gate_clks,
		ARRAY_SIZE(mmc0ctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_mmc0crg,
	"hisilicon,clk-extreme-mmc0-crg", clk_mmc0_init);

static void clk_sctrl_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(sctrl_scgt_clks) +
		ARRAY_SIZE(sctrl_div_clks) +
		ARRAY_SIZE(sctrl_mux_clks) +
		ARRAY_SIZE(sctrl_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_scgt(sctrl_scgt_clks,
		ARRAY_SIZE(sctrl_scgt_clks), clk_data);

	plat_clk_register_divider(sctrl_div_clks,
		ARRAY_SIZE(sctrl_div_clks), clk_data);

	plat_clk_register_mux(sctrl_mux_clks,
		ARRAY_SIZE(sctrl_mux_clks), clk_data);

	plat_clk_register_gate(sctrl_gate_clks,
		ARRAY_SIZE(sctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_sctrl,
	"hisilicon,clk-extreme-sctrl", clk_sctrl_init);

static void clk_pctrl_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(pctrl_scgt_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_scgt(pctrl_scgt_clks,
		ARRAY_SIZE(pctrl_scgt_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_pctrl,
	"hisilicon,clk-extreme-pctrl", clk_pctrl_init);

static void clk_media1_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(media1crg_scgt_clks) +
		ARRAY_SIZE(media1crg_div_clks) +
		ARRAY_SIZE(media1crg_mux_clks) +
		ARRAY_SIZE(media1crg_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_scgt(media1crg_scgt_clks,
		ARRAY_SIZE(media1crg_scgt_clks), clk_data);

	plat_clk_register_divider(media1crg_div_clks,
		ARRAY_SIZE(media1crg_div_clks), clk_data);

	plat_clk_register_mux(media1crg_mux_clks,
		ARRAY_SIZE(media1crg_mux_clks), clk_data);

	plat_clk_register_gate(media1crg_gate_clks,
		ARRAY_SIZE(media1crg_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_media1crg,
	"hisilicon,clk-extreme-media1-crg", clk_media1_init);

static void clk_media2_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(media2crg_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_gate(media2crg_gate_clks,
		ARRAY_SIZE(media2crg_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_media2crg,
	"hisilicon,clk-extreme-media2-crg", clk_media2_init);

static void clk_iomcu_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(iomcu_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_gate(iomcu_gate_clks,
		ARRAY_SIZE(iomcu_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_iomcu,
	"hisilicon,clk-extreme-iomcu-crg", clk_iomcu_init);

static void clk_xfreq_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(xfreq_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_xfreq(xfreq_clks,
		ARRAY_SIZE(xfreq_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_xfreq,
	"hisilicon,clk-extreme-xfreq", clk_xfreq_init);

static void clk_pmu_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(pmu_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_clkpmu(pmu_clks,
		ARRAY_SIZE(pmu_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_pmu,
	"hisilicon,clk-extreme-pmu", clk_pmu_init);

static void clk_mailbox_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(mailbox_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_mclk(mailbox_clks,
		ARRAY_SIZE(mailbox_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_mailbox,
	"hisilicon,clk-extreme-mailbox", clk_mailbox_init);

static void clk_dvfs_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(dvfs_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_dvfs_clk(dvfs_clks,
		ARRAY_SIZE(dvfs_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_dvfs,
	"hisilicon,clk-extreme-dvfs", clk_dvfs_init);

