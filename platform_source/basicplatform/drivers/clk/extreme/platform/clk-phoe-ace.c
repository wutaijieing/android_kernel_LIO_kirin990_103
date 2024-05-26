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
#include "clk-phoe-ace.h"
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

static const struct gate_clock fixed_factor_clks[] = {
	{CLK_SYS_INI, "clk_sys_ini", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sys_ini"},
	{CLK_DIV_SYSBUS, "div_sysbus_pll", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "div_sysbus_pll"},
	{CLK_PPLL_EPS, "clk_ppll_eps", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll_eps"},
	{CLK_GATE_WD0_HIGH, "clk_wd0_high", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_wd0_high"},
	{PCLK_GATE_DSI0, "pclk_dsi0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dsi0"},
	{PCLK_GATE_DSI1, "pclk_dsi1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dsi1"},
	{CLK_FACTOR_TCXO, "clk_factor_tcxo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_factor_tcxo"},
	{CLK_GATE_TIMER5_A, "clk_timer5_a", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5_a"},
	{ATCLK, "clk_at", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_at"},
	{CLK_DIV_CSSYSDBG, "clk_cssys_div", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_cssys_div"},
	{CLK_GATE_CSSYSDBG, "clk_cssysdbg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_cssysdbg"},
	{NPU_PLL6_FIX, "npu_pll6_fix", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "npu_pll6_fix"},
	{CLK_GATE_DMA_IOMCU, "clk_dma_iomcu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dma_iomcu"},
	{CLK_SD_SYS, "clk_sd_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sd_sys"},
	{CLK_SDIO_SYS, "clk_sdio_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sdio_sys"},
	{CLK_DIV_A53HPM, "clk_a53hpm_div", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_a53hpm_div"},
	{CLK_DIV_320M, "sc_div_320m", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "sc_div_320m"},
	{PCLK_GATE_UART0, "pclk_uart0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart0"},
	{CLK_FACTOR_UART0, "clk_uart0_fac", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart0_fac"},
	{CLK_FACTOR_USB3PHY_PLL, "clkfac_usb3phy", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clkfac_usb3phy"},
	{CLKIN_SYS_DIV, "clkin_sys_div", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clkin_sys_div"},
	{CLK_GATE_ABB_USB, "clk_abb_usb", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_abb_usb"},
	{CLK_GATE_UFSPHY_REF, "clk_ufsphy_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ufsphy_ref"},
	{CLK_GATE_UFSIO_REF, "clk_ufsio_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ufsio_ref"},
	{CLK_GATE_BLPWM, "clk_blpwm", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_blpwm"},
	{CLK_SYSCNT_DIV, "clk_syscnt_div", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_syscnt_div"},
	{CLK_GATE_GPS_REF, "clk_gps_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_gps_ref"},
	{CLK_MUX_GPS_REF, "clkmux_gps_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clkmux_gps_ref"},
	{CLK_GATE_MDM2GPS0, "clk_mdm2gps0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mdm2gps0"},
	{CLK_GATE_MDM2GPS1, "clk_mdm2gps1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mdm2gps1"},
	{CLK_GATE_MDM2GPS2, "clk_mdm2gps2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mdm2gps2"},
	{CLK_GATE_LDI0, "clk_ldi0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ldi0"},
	{EPS_VOLT_HIGH, "eps_volt_high", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "eps_volt_high"},
	{EPS_VOLT_MIDDLE, "eps_volt_middle", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "eps_volt_middle"},
	{EPS_VOLT_LOW, "eps_volt_low", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "eps_volt_low"},
	{VENC_VOLT_HOLD, "venc_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "venc_volt_hold"},
	{VDEC_VOLT_HOLD, "vdec_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "vdec_volt_hold"},
	{EDC_VOLT_HOLD, "edc_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "edc_volt_hold"},
	{EFUSE_VOLT_HOLD, "efuse_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "efuse_volt_hold"},
	{LDI0_VOLT_HOLD, "ldi0_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "ldi0_volt_hold"},
	{SEPLAT_VOLT_HOLD, "seplat_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "seplat_volt_hold"},
	{CLK_FIX_DIV_DPCTRL, "clk_fix_div_dpctrl", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fix_div_dpctrl"},
	{CLK_ISP_SNCLK_FAC, "clk_fac_ispsn", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fac_ispsn"},
	{CLK_FACTOR_RXDPHY, "clk_rxdcfg_fac", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdcfg_fac"},
	{CLK_DIV_HIFD_PLL, "clk_hifd_pll_div", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_pll_div"},
	{CLK_GATE_I2C0, "clk_i2c0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c0"},
	{CLK_GATE_I2C1, "clk_i2c1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c1"},
	{CLK_GATE_I2C2, "clk_i2c2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c2"},
	{CLK_GATE_SPI0, "clk_spi0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi0"},
	{CLK_FAC_180M, "clkfac_180m", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clkfac_180m"},
	{CLK_GATE_IOMCU_PERI0, "clk_gate_peri0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_gate_peri0"},
	{CLK_GATE_SPI2, "clk_spi2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi2"},
	{CLK_GATE_UART3, "clk_uart3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart3"},
	{CLK_GATE_UART8, "clk_uart8", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart8"},
	{CLK_GATE_UART7, "clk_uart7", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart7"},
	{AUTODIV_ISP_DVFS, "autodiv_isp_dvfs", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "autodiv_isp_dvfs"},
};

static const struct gate_clock crgctrl_gate_clks[] = {
	{CLK_GATE_PPLL0_MEDIA, "clk_ppll0_media", "NULL", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_media"},
	{CLK_GATE_PPLL2_MEDIA, "clk_ppll2_media", "NULL", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_media"},
	{CLK_GATE_PPLL3_MEDIA, "clk_ppll3_media", "NULL", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll3_media"},
	{CLK_GATE_PPLL7_MEDIA, "clk_ppll7_media", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll7_media"},
	{PCLK_GPIO0, "pclk_gpio0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio0"},
	{PCLK_GPIO1, "pclk_gpio1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio1"},
	{PCLK_GPIO2, "pclk_gpio2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio2"},
	{PCLK_GPIO3, "pclk_gpio3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio3"},
	{PCLK_GPIO4, "pclk_gpio4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio4"},
	{PCLK_GPIO5, "pclk_gpio5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio5"},
	{PCLK_GPIO6, "pclk_gpio6", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio6"},
	{PCLK_GPIO7, "pclk_gpio7", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio7"},
	{PCLK_GPIO8, "pclk_gpio8", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio8"},
	{PCLK_GPIO9, "pclk_gpio9", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio9"},
	{PCLK_GPIO10, "pclk_gpio10", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio10"},
	{PCLK_GPIO11, "pclk_gpio11", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio11"},
	{PCLK_GPIO12, "pclk_gpio12", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio12"},
	{PCLK_GPIO13, "pclk_gpio13", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio13"},
	{PCLK_GPIO14, "pclk_gpio14", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio14"},
	{PCLK_GPIO15, "pclk_gpio15", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio15"},
	{PCLK_GPIO16, "pclk_gpio16", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio16"},
	{PCLK_GPIO17, "pclk_gpio17", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio17"},
	{PCLK_GPIO18, "pclk_gpio18", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio18"},
	{PCLK_GPIO19, "pclk_gpio19", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio19"},
	{PCLK_GATE_WD0_HIGH, "pclk_wd0_high", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0_high"},
	{PCLK_GATE_WD0, "pclk_wd0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0"},
	{PCLK_GATE_WD1, "pclk_wd1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd1"},
	{CLK_GATE_CODECSSI, "clk_codecssi", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_codecssi"},
	{PCLK_GATE_CODECSSI, "pclk_codecssi", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_codecssi"},
	{CLK_GATE_MMC_USBDP, "clk_mmc_usbdp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mmc_usbdp"},
	{PCLK_GATE_IOC, "pclk_ioc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ioc"},
	{PCLK_GATE_MMC1_PCIE, "pclk_mmc1_pcie", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_mmc1_pcie"},
	{CLK_ATDVFS, "clk_atdvfs", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdvfs"},
	{ACLK_GATE_PERF_STAT, "aclk_perf_stat", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_perf_stat"},
	{PCLK_GATE_PERF_STAT, "pclk_perf_stat", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_perf_stat"},
	{CLK_GATE_PERF_STAT, "clk_perf_stat", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat"},
	{CLK_GATE_DMAC, "clk_dmac", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dmac"},
	{CLK_GATE_SOCP_ACPU, "clk_socp_acpu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_socp_acpu"},
	{CLK_GATE_SOCP_DEFLATE, "clk_socp_deflat", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_socp_deflat"},
	{CLK_GATE_TCLK_SOCP, "tclk_socp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "tclk_socp"},
	{CLK_GATE_TIME_STAMP_GT, "clk_timestp_gt", "NULL", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timestp_gt"},
	{CLK_GATE_IPF, "clk_ipf", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ipf"},
	{CLK_GATE_IPF_PSAM, "clk_ipf_psam", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ipf_psam"},
	{ACLK_GATE_MAA, "aclk_maa", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_maa"},
	{CLK_GATE_MAA_REF, "clk_maa_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_maa_ref"},
	{CLK_GATE_SPE, "clk_spe", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spe"},
	{HCLK_GATE_SPE, "hclk_spe", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_spe"},
	{CLK_GATE_SPE_REF, "clk_spe_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spe_ref"},
	{ACLK_GATE_AXI_MEM, "aclk_axi_mem", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_axi_mem"},
	{CLK_GATE_AXI_MEM, "clk_axi_mem", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_axi_mem"},
	{CLK_GATE_AXI_MEM_GS, "clk_axi_mem_gs", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_axi_mem_gs"},
	{CLK_GATE_VCODECBUS2DDR, "clk_vcodecbus2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus2"},
	{CLK_GATE_SD, "clk_sd", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sd"},
	{CLK_GATE_UART1, "clk_uart1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart1"},
	{CLK_GATE_UART4, "clk_uart4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart4"},
	{PCLK_GATE_UART1, "pclk_uart1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart1"},
	{PCLK_GATE_UART4, "pclk_uart4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart4"},
	{CLK_GATE_UART2, "clk_uart2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart2"},
	{CLK_GATE_UART5, "clk_uart5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart5"},
	{PCLK_GATE_UART2, "pclk_uart2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart2"},
	{PCLK_GATE_UART5, "pclk_uart5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart5"},
	{CLK_GATE_UART0, "clk_uart0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart0"},
	{CLK_GATE_I2C3, "clk_i2c3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c3"},
	{CLK_GATE_I2C4, "clk_i2c4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c4"},
	{CLK_GATE_I2C6_ACPU, "clk_i2c6_acpu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c6_acpu"},
	{CLK_GATE_I2C7, "clk_i2c7", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c7"},
	{PCLK_GATE_I2C3, "pclk_i2c3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c3"},
	{PCLK_GATE_I2C4, "pclk_i2c4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c4"},
	{PCLK_GATE_I2C6_ACPU, "pclk_i2c6_acpu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c6_acpu"},
	{PCLK_GATE_I2C7, "pclk_i2c7", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c7"},
	{CLK_GATE_I3C4, "clk_i3c4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i3c4"},
	{PCLK_GATE_I3C4, "pclk_i3c4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i3c4"},
	{CLK_GATE_SPI1, "clk_spi1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi1"},
	{CLK_GATE_SPI4, "clk_spi4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi4"},
	{PCLK_GATE_SPI1, "pclk_spi1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi1"},
	{PCLK_GATE_SPI4, "pclk_spi4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi4"},
	{CLK_GATE_USB3OTG_REF, "clk_usb3otg_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3otg_ref"},
	{CLK_GATE_AO_ASP, "clk_ao_asp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_asp"},
	{PCLK_GATE_PCTRL, "pclk_pctrl", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pctrl"},
	{CLK_GATE_PWM, "clk_pwm", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pwm"},
	{PCLK_GATE_BLPWM_PERI, "pclk_blpwm_peri", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_blpwm_peri"},
	{PERI_VOLT_HOLD, "peri_volt_hold", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "peri_volt_hold"},
	{PERI_VOLT_MIDDLE, "peri_volt_middle", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "peri_volt_middle"},
	{PERI_VOLT_LOW, "peri_volt_low", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "peri_volt_low"},
	{CLK_GATE_DPCTRL_16M, "clk_dpctrl_16m", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dpctrl_16m"},
	{CLK_GATE_ISP_I2C_MEDIA, "clk_isp_i2c_media", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i2c_media"},
	{CLK_GATE_ISP_SNCLK0, "clk_isp_snclk0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk0"},
	{CLK_GATE_ISP_SNCLK1, "clk_isp_snclk1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk1"},
	{CLK_GATE_ISP_SNCLK2, "clk_isp_snclk2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk2"},
	{CLK_GATE_ISP_SNCLK3, "clk_isp_snclk3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk3"},
	{CLK_GATE_RXDPHY0_CFG, "clk_rxdphy0_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy0_cfg"},
	{CLK_GATE_RXDPHY1_CFG, "clk_rxdphy1_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy1_cfg"},
	{CLK_GATE_RXDPHY2_CFG, "clk_rxdphy2_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy2_cfg"},
	{CLK_GATE_RXDPHY3_CFG, "clk_rxdphy3_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy3_cfg"},
	{CLK_GATE_TXDPHY0_CFG, "clk_txdphy0_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_cfg"},
	{CLK_GATE_TXDPHY0_REF, "clk_txdphy0_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_ref"},
	{CLK_GATE_TXDPHY1_CFG, "clk_txdphy1_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy1_cfg"},
	{CLK_GATE_TXDPHY1_REF, "clk_txdphy1_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy1_ref"},
	{PCLK_GATE_LOADMONITOR, "pclk_loadmonitor", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor"},
	{CLK_GATE_LOADMONITOR, "clk_loadmonitor", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor"},
	{PCLK_GATE_LOADMONITOR_L, "pclk_loadmonitor_l", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor_l"},
	{CLK_GATE_LOADMONITOR_L, "clk_loadmonitor_l", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_l"},
	{PCLK_GATE_LOADMONITOR_2, "pclk_loadmonitor_2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor_2"},
	{CLK_GATE_LOADMONITOR_2, "clk_loadmonitor_2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_2"},
	{CLK_GATE_MEDIA_TCXO, "clk_media_tcxo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_tcxo"},
	{CLK_GATE_AO_HIFD, "clk_ao_hifd", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_hifd"},
	{AUTODIV_ISP, "autodiv_isp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "autodiv_isp"},
	{CLK_GATE_ATDIV_MMC0, "clk_atdiv_mmc0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_mmc0"},
	{CLK_GATE_ATDIV_DMA, "clk_atdiv_dma", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_dma"},
	{CLK_GATE_ATDIV_CFG, "clk_atdiv_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_cfg"},
	{CLK_GATE_ATDIV_SYS, "clk_atdiv_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_sys"},
};

static const struct gate_clock pll_clks[] = {
	{CLK_GATE_PPLL0, "clk_ap_ppll0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll0"},
	{CLK_GATE_PPLL1, "clk_ap_ppll1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll1"},
	{CLK_GATE_PPLL2, "clk_ap_ppll2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll2"},
	{CLK_GATE_PPLL3, "clk_ap_ppll3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll3"},
	{CLK_GATE_PPLL5, "clk_ap_ppll5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll5"},
	{CLK_GATE_PPLL6, "clk_ap_ppll6", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll6"},
	{CLK_GATE_PPLL7, "clk_ap_ppll7", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_ppll7"},
};

static const struct gate_clock hsdtctrl_pll_clks[] = {
	{CLK_GATE_SCPLL, "clk_ap_scpll", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ap_scpll"},
};

static const struct gate_clock hsdtctrl_gate_clks[] = {
	{CLK_GATE_PCIEPHY_REF, "clk_pciephy_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pciephy_ref"},
	{CLK_GATE_PCIE1PHY_REF, "clk_pcie1phy_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie1phy_ref"},
	{PCLK_GATE_PCIE_SYS, "pclk_pcie_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_sys"},
	{PCLK_GATE_PCIE1_SYS, "pclk_pcie1_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_sys"},
	{PCLK_GATE_PCIE_PHY, "pclk_pcie_phy", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_phy"},
	{PCLK_GATE_PCIE1_PHY, "pclk_pcie1_phy", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_phy"},
	{HCLK_GATE_SDIO, "hclk_sdio", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sdio"},
	{CLK_GATE_SDIO, "clk_sdio", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sdio"},
	{CLK_GATE_PCIEAUX, "clk_pcieaux", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcieaux"},
	{CLK_GATE_PCIEAUX1, "clk_pcieaux1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcieaux1"},
	{ACLK_GATE_PCIE, "aclk_pcie", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie"},
	{ACLK_GATE_PCIE1, "aclk_pcie1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie1"},
	{CLK_GATE_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dp_audio_pll_ao"},
};

static const struct gate_clock sctrl_gate_clks[] = {
	{PCLK_GPIO20, "pclk_gpio20", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio20"},
	{PCLK_GPIO21, "pclk_gpio21", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio21"},
	{CLK_GATE_TIMER5_B, "clk_timer5_b", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5_b"},
	{CLK_GATE_TIMER5, "clk_timer5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5"},
	{CLK_GATE_SPI, "clk_spi3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi3"},
	{PCLK_GATE_SPI, "pclk_spi3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi3"},
	{CLK_GATE_USB2PHY_REF, "clk_usb2phy_ref", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref"},
	{PCLK_GATE_RTC, "pclk_rtc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_rtc"},
	{PCLK_GATE_RTC1, "pclk_rtc1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_rtc1"},
	{PCLK_AO_GPIO0, "pclk_ao_gpio0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio0"},
	{PCLK_AO_GPIO1, "pclk_ao_gpio1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio1"},
	{PCLK_AO_GPIO2, "pclk_ao_gpio2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio2"},
	{PCLK_AO_GPIO3, "pclk_ao_gpio3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio3"},
	{PCLK_AO_GPIO4, "pclk_ao_gpio4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio4"},
	{PCLK_AO_GPIO5, "pclk_ao_gpio5", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio5"},
	{PCLK_AO_GPIO6, "pclk_ao_gpio6", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio6"},
	{PCLK_AO_GPIO29, "pclk_ao_gpio29", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio29"},
	{PCLK_AO_GPIO30, "pclk_ao_gpio30", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio30"},
	{PCLK_AO_GPIO31, "pclk_ao_gpio31", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio31"},
	{PCLK_AO_GPIO32, "pclk_ao_gpio32", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio32"},
	{PCLK_AO_GPIO33, "pclk_ao_gpio33", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio33"},
	{PCLK_AO_GPIO34, "pclk_ao_gpio34", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio34"},
	{PCLK_AO_GPIO35, "pclk_ao_gpio35", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio35"},
	{PCLK_AO_GPIO36, "pclk_ao_gpio36", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio36"},
	{PCLK_GATE_SYSCNT, "pclk_syscnt", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_syscnt"},
	{CLK_GATE_SYSCNT, "clk_syscnt", "NULL", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_syscnt"},
	{CLK_ASP_BACKUP, "clk_asp_backup", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_backup"},
	{CLK_ASP_CODEC, "clk_asp_codec", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_codec"},
	{CLK_GATE_ASP_SUBSYS, "clk_asp_subsys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_subsys"},
	{CLK_GATE_ASP_TCXO, "clk_asp_tcxo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_tcxo"},
	{CLK_GATE_DP_AUDIO_PLL, "clk_dp_audio_pll", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dp_audio_pll"},
	{CLK_GATE_AO_CAMERA, "clk_ao_camera", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_camera"},
	{PCLK_GATE_AO_LOADMONITOR, "pclk_ao_loadmonitor", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_loadmonitor"},
	{CLK_GATE_AO_LOADMONITOR, "clk_ao_loadmonitor", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_loadmonitor"},
	{CLK_GATE_HIFD_TCXO, "clk_hifd_tcxo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_tcxo"},
	{CLK_GATE_HIFD_FLL, "clk_hifd_fll", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_fll"},
	{CLK_GATE_HIFD_PLL, "clk_hifd_pll", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hifd_pll"},
};

static const struct gate_clock mmc0ctrl_gate_clks[] = {
	{HCLK_GATE_USB3OTG, "hclk_usb3otg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_usb3otg"},
	{ACLK_GATE_USB3OTG, "aclk_usb3otg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_usb3otg"},
	{HCLK_GATE_SD, "hclk_sd", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sd"},
	{PCLK_GATE_DPCTRL, "pclk_dpctrl", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl"},
	{ACLK_GATE_DPCTRL, "aclk_dpctrl", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_dpctrl"},
};

static const struct gate_clock iomcu_gate_clks[] = {
	{CLK_I2C1_GATE_IOMCU, "clk_i2c1_gt", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c1_gt"},
};

static const struct gate_clock media1crg_gate_clks[] = {
	{PCLK_GATE_ISP_NOC_SUBSYS, "pclk_isp_noc_subsys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_isp_noc_subsys"},
	{ACLK_GATE_ISP_NOC_SUBSYS, "aclk_isp_noc_subsys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp_noc_subsys"},
	{ACLK_GATE_MEDIA_COMMON, "aclk_media_common", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_media_common"},
	{ACLK_GATE_NOC_DSS, "aclk_noc_dss", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_noc_dss"},
	{PCLK_GATE_MEDIA_COMMON, "pclk_media_common", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media_common"},
	{PCLK_GATE_NOC_DSS_CFG, "pclk_noc_dss_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_noc_dss_cfg"},
	{PCLK_GATE_MMBUF_CFG, "pclk_mmbuf_cfg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_mmbuf_cfg"},
	{PCLK_GATE_DISP_NOC_SUBSYS, "pclk_disp_noc_subsys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_disp_noc_subsys"},
	{ACLK_GATE_DISP_NOC_SUBSYS, "aclk_disp_noc_subsys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_disp_noc_subsys"},
	{PCLK_GATE_DSS, "pclk_dss", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dss"},
	{ACLK_GATE_DSS, "aclk_dss", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_dss"},
	{ACLK_GATE_ISP, "aclk_isp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp"},
	{CLK_GATE_VIVOBUS_TO_DDRC, "clk_vivobus2ddr", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vivobus2ddr"},
	{CLK_GATE_EDC0, "clk_edc0", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_edc0"},
	{CLK_GATE_LDI1, "clk_ldi1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ldi1"},
	{CLK_GATE_ISPI2C, "clk_ispi2c", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispi2c"},
	{CLK_GATE_ISP_SYS, "clk_isp_sys", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_sys"},
	{CLK_GATE_ISPCPU, "clk_ispcpu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispcpu"},
	{CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfuncfreq"},
	{CLK_GATE_ISPFUNC2FREQ, "clk_ispfunc2freq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc2freq"},
	{CLK_GATE_ISPFUNC3FREQ, "clk_ispfunc3freq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc3freq"},
	{CLK_GATE_ISPFUNC4FREQ, "clk_ispfunc4freq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc4freq"},
	{CLK_GATE_ISP_I3C, "clk_isp_i3c", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i3c"},
	{CLK_GATE_MEDIA_COMMONFREQ, "clk_media_commonfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_commonfreq"},
	{CLK_GATE_BRG, "clk_brg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_brg"},
	{PCLK_GATE_MEDIA1_LM, "pclk_media1_lm", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media1_lm"},
	{CLK_GATE_LOADMONITOR_MEDIA1, "clk_loadmonitor_media1", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_media1"},
	{CLK_GATE_FD_FUNC, "clk_fd_func", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fd_func"},
	{CLK_GATE_JPG_FUNCFREQ, "clk_jpg_funcfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_jpg_funcfreq"},
	{ACLK_GATE_JPG, "aclk_jpg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_jpg"},
	{PCLK_GATE_JPG, "pclk_jpg", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_jpg"},
	{ACLK_GATE_NOC_ISP, "aclk_noc_isp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_noc_isp"},
	{PCLK_GATE_NOC_ISP, "pclk_noc_isp", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_noc_isp"},
	{CLK_GATE_FDAI_FUNCFREQ, "clk_fdai_funcfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fdai_funcfreq"},
	{ACLK_GATE_ASC, "aclk_asc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_asc"},
	{CLK_GATE_DSS_AXI_MM, "clk_dss_axi_mm", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dss_axi_mm"},
	{CLK_GATE_MMBUF, "clk_mmbuf", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mmbuf"},
	{PCLK_GATE_MMBUF, "pclk_mmbuf", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_mmbuf"},
	{CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vivo"},
	{CLK_GATE_ATDIV_ISPCPU, "clk_atdiv_ispcpu", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_ispcpu"},
};

static const struct gate_clock media2crg_gate_clks[] = {
	{CLK_GATE_VDECFREQ, "clk_vdecfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vdecfreq"},
	{PCLK_GATE_VDEC, "pclk_vdec", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_vdec"},
	{ACLK_GATE_VDEC, "aclk_vdec", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_vdec"},
	{CLK_GATE_VENCFREQ, "clk_vencfreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vencfreq"},
	{PCLK_GATE_VENC, "pclk_venc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_venc"},
	{ACLK_GATE_VENC, "aclk_venc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_venc"},
	{ACLK_GATE_VENC2, "aclk_venc2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_venc2"},
	{CLK_GATE_VENC2FREQ, "clk_venc2freq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_venc2freq"},
	{PCLK_GATE_MEDIA2_LM, "pclk_media2_lm", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_media2_lm"},
	{CLK_GATE_LOADMONITOR_MEDIA2, "clk_loadmonitor_media2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_media2"},
	{CLK_GATE_IVP32DSP_TCXO, "clk_ivpdsp_tcxo", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ivpdsp_tcxo"},
	{CLK_GATE_IVP32DSP_COREFREQ, "clk_ivpdsp_corefreq", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ivpdsp_corefreq"},
	{CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vcbus"},
	{CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vdec"},
	{CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_venc"},
};

static const struct gate_clock pctrl_scgt_clks[] = {
	{CLK_GATE_USB_TCXO_EN, "clk_usb_tcxo_en", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb_tcxo_en"},
};

static const struct xfreq_clock xfreq_clks[] = {
	{CLK_CLUSTER0, "clk_cluster0", 0, 0, 0, 0x41C, {0x0001030A, 0x0}, {0x0001020A, 0x0}, "cpu-cluster.0"},
	{CLK_CLUSTER1, "clk_cluster1", 1, 0, 0, 0x41C, {0x0002030A, 0x0}, {0x0002020A, 0x0}, "cpu-cluster.1"},
	{CLK_G3D, "clk_g3d", 2, 0, 0, 0x41C, {0x0003030A, 0x0}, {0x0003020A, 0x0}, "clk_g3d"},
	{CLK_DDRC_FREQ, "clk_ddrc_freq", 3, 0, 0, 0x41C, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_freq"},
	{CLK_DDRC_MAX, "clk_ddrc_max", 5, 1, 0x8000, 0x250, {0x00040308, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_max"},
	{CLK_DDRC_MIN, "clk_ddrc_min", 4, 1, 0x8000, 0x270, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_min"},
};

static const struct gate_clock pmu_clks[] = {
	{CLK_GATE_ABB_192, "clk_abb_192", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_abb_192"},
	{CLK_PMU32KA, "clk_pmu32ka", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pmu32ka"},
	{CLK_PMU32KB, "clk_pmu32kb", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pmu32kb"},
	{CLK_PMU32KC, "clk_pmu32kc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pmu32kc"},
	{CLK_PMUAUDIOCLK, "clk_pmuaudioclk", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pmuaudioclk"},
};

static const struct gate_clock dvfs_clks[] = {
	{CLK_GATE_VDEC, "clk_vdec", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vdec"},
	{CLK_GATE_VENC, "clk_venc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_venc"},
	{CLK_GATE_VENC2, "clk_venc2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_venc2"},
	{CLK_GATE_ISPFUNC, "clk_ispfunc", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc"},
	{CLK_GATE_JPG_FUNC, "clk_jpg_func", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_jpg_func"},
	{CLK_GATE_FDAI_FUNC, "clk_fdai_func", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_fdai_func"},
	{CLK_GATE_MEDIA_COMMON, "clk_media_common", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_common"},
	{CLK_GATE_IVP32DSP_CORE, "clk_ivpdsp_core", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ivpdsp_core"},
	{CLK_GATE_ISPFUNC2, "clk_ispfunc2", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc2"},
	{CLK_GATE_ISPFUNC3, "clk_ispfunc3", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc3"},
	{CLK_GATE_ISPFUNC4, "clk_ispfunc4", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfunc4"},
	{CLK_GATE_HIFACE, "clk_hiface", "NULL", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hiface"},
};

static void clk_crgctrl_init(struct device_node *np)
{
	struct clock_data *clk_crgctrl_data;
	int nr = ARRAY_SIZE(fixed_clks) +
		ARRAY_SIZE(pll_clks) +
		ARRAY_SIZE(fixed_factor_clks) +
		ARRAY_SIZE(crgctrl_gate_clks);

	clk_crgctrl_data = clk_init(np, nr);
	if (!clk_crgctrl_data)
		return;

	plat_clk_register_fixed_rate(fixed_clks,
		ARRAY_SIZE(fixed_clks), clk_crgctrl_data);

	ace_clk_register_gate(pll_clks,
		ARRAY_SIZE(pll_clks), clk_crgctrl_data);

	ace_clk_register_gate(fixed_factor_clks,
		ARRAY_SIZE(fixed_factor_clks), clk_crgctrl_data);

	ace_clk_register_gate(crgctrl_gate_clks,
		ARRAY_SIZE(crgctrl_gate_clks), clk_crgctrl_data);
}

CLK_OF_DECLARE_DRIVER(clk_peri_crgctrl,
	"hisilicon,clk-extreme-crgctrl", clk_crgctrl_init);

static void clk_hsdt_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(hsdtctrl_pll_clks) +
		ARRAY_SIZE(hsdtctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	ace_clk_register_gate(hsdtctrl_pll_clks,
		ARRAY_SIZE(hsdtctrl_pll_clks), clk_data);

	ace_clk_register_gate(hsdtctrl_gate_clks,
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

	ace_clk_register_gate(mmc0ctrl_gate_clks,
		ARRAY_SIZE(mmc0ctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_mmc0crg,
	"hisilicon,clk-extreme-mmc0-crg", clk_mmc0_init);

static void clk_sctrl_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(sctrl_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	ace_clk_register_gate(sctrl_gate_clks,
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

	ace_clk_register_gate(pctrl_scgt_clks,
		ARRAY_SIZE(pctrl_scgt_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_pctrl,
	"hisilicon,clk-extreme-pctrl", clk_pctrl_init);

static void clk_media1_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(media1crg_gate_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	ace_clk_register_gate(media1crg_gate_clks,
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

	ace_clk_register_gate(media2crg_gate_clks,
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

	ace_clk_register_gate(iomcu_gate_clks,
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

	ace_clk_register_gate(pmu_clks,
		ARRAY_SIZE(pmu_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_pmu,
	"hisilicon,clk-extreme-pmu", clk_pmu_init);

static void clk_dvfs_init(struct device_node *np)
{
	struct clock_data *clk_data;
	int nr = ARRAY_SIZE(dvfs_clks);
	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	ace_clk_register_gate(dvfs_clks,
		ARRAY_SIZE(dvfs_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_dvfs,
	"hisilicon,clk-extreme-dvfs", clk_dvfs_init);

