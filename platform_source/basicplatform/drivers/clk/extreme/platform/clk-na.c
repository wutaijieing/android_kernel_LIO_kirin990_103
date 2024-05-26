/*
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#include "clk-na.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>

static const struct fixed_rate_clock fixed_clks[] = {
	{ CLKIN_SYS, "clkin_sys", NULL, 0, 38400000, "clkin_sys" },
	{ CLKIN_REF, "clkin_ref", NULL, 0, 32764, "clkin_ref" },
	{ CLK_FLL_SRC, "clk_fll_src", NULL, 0, 184000000, "clk_fll_src" },
	{ CLK_PPLL1, "clk_ppll1", NULL, 0, 2133000000, "clk_ppll1" },
	{ CLK_PPLL2, "clk_ppll2", NULL, 0, 1920000000, "clk_ppll2" },
	{ CLK_PPLL3, "clk_ppll3", NULL, 0, 1200000000, "clk_ppll3" },
	{ CLK_PPLL4, "clk_ppll4", NULL, 0, 1440000000, "clk_ppll4" },
	{ CLK_PPLL5, "clk_ppll5", NULL, 0, 2100000000, "clk_ppll5" },
	{ CLK_AUPLL, "clk_aupll", NULL, 0, 1179648000, "clk_aupll" },
	{ CLK_SPLL, "clk_spll", NULL, 0, 1622016000, "clk_spll" },
	{ CLK_MODEM_BASE, "clk_modem_base", NULL, 0, 49152000, "clk_modem_base" },
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
	{ CLK_DIV_SYSBUS, "div_sysbus_pll", "clk_spll", 1, 7, 0, "div_sysbus_pll" },
	{ CLK_DIV_HSDT_BUS, "div_hsdtbus_pll", "clk_spll", 1, 21, 0, "div_hsdtbus_pll" },
	{ CLK_DIV_HSDT1_BUS, "div_hsdt1bus_pll", "clk_spll", 1, 4, 0, "div_hsdt1bus_pll" },
	{ CLK_GATE_WD0_HIGH, "clk_wd0_high", "sc_div_cfgbus", 1, 1, 0, "clk_wd0_high" },
	{ CLK_FACTOR_TCXO, "clk_factor_tcxo", "clk_sys_ini", 1, 4, 0, "clk_factor_tcxo" },
	{ CLK_GATE_ULPI_REF, "clk_ulpi_ref", "clkin_sys", 1, 1, 0, "clk_ulpi_ref" },
	{ CLK_USB2PHY_REF_DIV, "clkdiv_usb2phy_ref", "clk_abb_192", 1, 2, 0, "clkdiv_usb2phy_ref" },
	{ CLKIN_SYS_DIV, "clkdiv_in_sys", "clkin_sys", 1, 2, 0, "clkdiv_in_sys" },
	{ CLK_DIV_HSDT1_USBDP, "div_hsdt1_usbdp", "clkgt_hsdt1_usbdp", 1, 7, 0, "div_hsdt1_usbdp" },
	{ CLK_USB3PHY_REF_DIV, "clkdiv_usb3phy_ref", "clkin_sys", 1, 2, 0, "clkdiv_usb3phy_ref" },
	{ ACLK_GATE_USB3OTG, "aclk_usb3otg", "clk_sys_ini", 1, 1, 0, "aclk_usb3otg" },
	{ HCLK_GATE_USB3OTG, "hclk_usb3otg", "clk_sys_ini", 1, 1, 0, "hclk_usb3otg" },
	{ CLK_GATE_USB3OTG_REF, "clk_usb3otg_ref", "clk_sys_ini", 1, 1, 0, "clk_usb3otg_ref" },
	{ CLK_GATE_HSDT1_EUSB, "clk_hsdt1_eusb", "clk_sys_ini", 1, 1, 0, "clk_hsdt1_eusb" },
	{ CLK_GATE_PCIE1AUX, "clk_pcie1aux", "clk_sys_ini", 1, 1, 0, "clk_pcie1aux" },
	{ ATCLK, "clk_at", "clk_atdvfs", 1, 1, 0, "clk_at" },
	{ TRACKCLKIN, "clk_track", "clkdiv_track", 1, 1, 0, "clk_track" },
	{ PCLK_DBG, "pclk_dbg", "pclkdiv_dbg", 1, 1, 0, "pclk_dbg" },
	{ CLK_DIV_CSSYSDBG, "clk_cssys_div", "autodiv_sysbus", 1, 1, 0, "clk_cssys_div" },
	{ CLK_GATE_CSSYSDBG, "clk_cssysdbg", "clk_dmabus_div", 1, 1, 0, "clk_cssysdbg" },
	{ CLK_GATE_DMA_IOMCU, "clk_dma_iomcu", "clk_fll_src", 1, 1, 0, "clk_dma_iomcu" },
	{ CLK_DIV_A53HPM, "clk_a53hpm_div", "clk_a53hpm_gt", 1, 4, 0, "clk_a53hpm_div" },
	{ CLK_DIV_320M, "sc_div_320m", "gt_clk_320m_pll", 1, 5, 0, "sc_div_320m" },
	{ CLK_GATE_UART1, "clk_uart1", "clk_sys_ini", 1, 1, 0, "clk_uart1" },
	{ CLK_GATE_UART4, "clk_uart4", "clk_sys_ini", 1, 1, 0, "clk_uart4" },
	{ PCLK_GATE_UART1, "pclk_uart1", "clk_sys_ini", 1, 1, 0, "pclk_uart1" },
	{ CLK_GATE_UART2, "clk_uart2", "clk_sys_ini", 1, 1, 0, "clk_uart2" },
	{ CLK_GATE_UART5, "clk_uart5", "clk_sys_ini", 1, 1, 0, "clk_uart5" },
	{ PCLK_GATE_UART2, "pclk_uart2", "clk_sys_ini", 1, 1, 0, "pclk_uart2" },
	{ CLK_GATE_UART0, "clk_uart0", "sc_div_320m", 1, 2, 0, "clk_uart0" },
	{ CLK_FACTOR_UART0, "clk_uart0_fac", "clkmux_uart0", 1, 1, 0, "clk_uart0_fac" },
	{ CLK_GATE_I2C9, "clk_i2c9", "clk_sys_ini", 1, 1, 0, "clk_i2c9" },
	{ PCLK_GATE_I2C9, "pclk_i2c9", "clk_sys_ini", 1, 1, 0, "pclk_i2c9" },
	{ CLK_GATE_I3C4, "clk_i3c4", "clk_sys_ini", 1, 1, 0, "clk_i3c4" },
	{ PCLK_GATE_I3C4, "pclk_i3c4", "clk_sys_ini", 1, 1, 0, "pclk_i3c4" },
	{ CLK_GATE_UFSPHY_REF, "clk_ufsphy_ref", "clkin_sys", 1, 1, 0, "clk_ufsphy_ref" },
	{ CLK_GATE_UFSIO_REF, "clk_ufsio_ref", "clkin_sys", 1, 1, 0, "clk_ufsio_ref" },
	{ CLK_GATE_PWM, "clk_pwm", "clk_fll_src", 1, 2, 0, "clk_pwm" },
	{ CLK_GATE_BLPWM, "clk_blpwm", "clk_fll_src", 1, 2, 0, "clk_blpwm" },
	{ CLK_SYSCNT_DIV, "clk_syscnt_div", "clk_sys_ini", 1, 10, 0, "clk_syscnt_div" },
	{ CLK_GATE_GPS_REF, "clk_gps_ref", "clk_sys_ini", 1, 1, 0, "clk_gps_ref" },
	{ CLK_MUX_GPS_REF, "clkmux_gps_ref", "clk_sys_ini", 1, 1, 0, "clkmux_gps_ref" },
	{ CLK_GATE_MDM2GPS0, "clk_mdm2gps0", "clk_modem_base", 1, 1, 0, "clk_mdm2gps0" },
	{ CLK_GATE_MDM2GPS1, "clk_mdm2gps1", "clk_modem_base", 1, 1, 0, "clk_mdm2gps1" },
	{ CLK_GATE_MDM2GPS2, "clk_mdm2gps2", "clk_modem_base", 1, 1, 0, "clk_mdm2gps2" },
	{ CLKDIV_ASP_CODEC, "clkdiv_asp_codec", "clkdiv_asp_codec_pll", 1, 2, 0, "clkdiv_asp_codec" },
	{ PCLK_GATE_DSS, "pclk_dss", "clk_sys_ini", 1, 1, 0, "pclk_dss" },
	{ ACLK_GATE_DSS, "aclk_dss", "clk_sys_ini", 1, 1, 0, "aclk_dss" },
	{ PCLK_GATE_DSI0, "pclk_dsi0", "clk_sys_ini", 1, 1, 0, "pclk_dsi0" },
	{ PCLK_GATE_DSI1, "pclk_dsi1", "clk_sys_ini", 1, 1, 0, "pclk_dsi1" },
	{ CLK_GATE_LDI0, "clk_ldi0", "clk_invalid", 1, 1, 0, "clk_ldi0" },
	{ CLK_GATE_LDI1, "clk_ldi1", "clk_invalid", 1, 1, 0, "clk_ldi1" },
	{ VENC_VOLT_HOLD, "venc_volt_hold", "peri_volt_hold", 1, 1, 0, "venc_volt_hold" },
	{ VDEC_VOLT_HOLD, "vdec_volt_hold", "peri_volt_hold", 1, 1, 0, "vdec_volt_hold" },
	{ EDC_VOLT_HOLD, "edc_volt_hold", "peri_volt_hold", 1, 1, 0, "edc_volt_hold" },
	{ EFUSE_VOLT_HOLD, "efuse_volt_hold", "peri_volt_middle", 1, 1, 0, "efuse_volt_hold" },
	{ CLK_GATE_DPCTRL_16M, "clk_dpctrl_16m", "clk_invalid", 1, 1, 0, "clk_dpctrl_16m" },
	{ PCLK_GATE_DPCTRL, "pclk_dpctrl", "clk_invalid", 1, 1, 0, "pclk_dpctrl" },
	{ ACLK_GATE_DPCTRL, "aclk_dpctrl", "clk_invalid", 1, 1, 0, "aclk_dpctrl" },
	{ CLK_GATE_ISP_SYS, "clk_isp_sys", "clk_invalid", 1, 1, 0, "clk_isp_sys" },
	{ CLK_GATE_ISP_I3C, "clk_isp_i3c", "clk_invalid", 1, 1, 0, "clk_isp_i3c" },
	{ CLK_GATE_ISPFUNC2, "clk_ispfunc2", "clk_spll", 1, 1, 0, "clk_ispfunc2" },
	{ CLK_GATE_ISPFUNC3, "clk_ispfunc3", "clk_spll", 1, 1, 0, "clk_ispfunc3" },
	{ CLK_GATE_ISP_SNCLK2, "clk_isp_snclk2", "clk_mux_ispsn0", 1, 4, 0, "clk_isp_snclk2" },
	{ CLK_GATE_ISP_SNCLK3, "clk_isp_snclk3", "clk_mux_ispsn0", 1, 4, 0, "clk_isp_snclk3" },
	{ CLK_ISP_SNCLK_FAC, "clk_fac_ispsn", "clk_ispsn_gt", 1, 14, 0, "clk_fac_ispsn" },
	{ CLK_ISP_SNCLK_ANGT, "clk_ispsn_gt", "sc_div_320m", 1, 4, 0, "clk_ispsn_gt" },
	{ CLK_GATE_RXDPHY2_CFG, "clk_rxdphy2_cfg", "clk_rxdcfg_mux", 1, 1, 0, "clk_rxdphy2_cfg" },
	{ CLK_GATE_RXDPHY3_CFG, "clk_rxdphy3_cfg", "clk_rxdcfg_mux", 1, 1, 0, "clk_rxdphy3_cfg" },
	{ CLK_GATE_TXDPHY0_REF, "clk_txdphy0_ref", "clk_sys_ini", 1, 1, 0, "clk_txdphy0_ref" },
	{ CLK_GATE_TXDPHY1_CFG, "clk_txdphy1_cfg", "clk_invalid", 1, 1, 0, "clk_txdphy1_cfg" },
	{ CLK_GATE_TXDPHY1_REF, "clk_txdphy1_ref", "clk_invalid", 1, 1, 0, "clk_txdphy1_ref" },
	{ CLK_FACTOR_RXDPHY, "clk_rxdcfg_fac", "clk_rxdcfg_gt", 1, 6, 0, "clk_rxdcfg_fac" },
	{ PCLK_GATE_AO_LOADMONITOR, "pclk_ao_loadmonitor", "clk_sys_ini", 1, 1, 0, "pclk_ao_loadmonitor" },
	{ CLK_GATE_AO_LOADMONITOR, "clk_ao_loadmonitor", "clk_sys_ini", 1, 1, 0, "clk_ao_loadmonitor" },
	{ PCLK_GATE_LOADMONITOR, "pclk_loadmonitor", "clk_sys_ini", 1, 1, 0, "pclk_loadmonitor" },
	{ CLK_GATE_LOADMONITOR, "clk_loadmonitor", "clk_sys_ini", 1, 1, 0, "clk_loadmonitor" },
	{ PCLK_GATE_LOADMONITOR_L, "pclk_loadmonitor_l", "clk_sys_ini", 1, 1, 0, "pclk_loadmonitor_l" },
	{ CLK_GATE_LOADMONITOR_L, "clk_loadmonitor_l", "clk_sys_ini", 1, 1, 0, "clk_loadmonitor_l" },
	{ CLK_GATE_FD_FUNC, "clk_fd_func", "clk_invalid", 1, 1, 0, "clk_fd_func" },
	{ CLK_GATE_JPG_FUNC, "clk_jpg_func", "clk_spll", 1, 16, 0, "clk_jpg_func" },
	{ ACLK_GATE_JPG, "aclk_jpg", "clk_spll", 1, 16, 0, "aclk_jpg" },
	{ PCLK_GATE_JPG, "pclk_jpg", "clk_spll", 1, 16, 0, "pclk_jpg" },
	{ ACLK_GATE_NOC_ISP, "aclk_noc_isp", "clk_spll", 1, 16, 0, "aclk_noc_isp" },
	{ PCLK_GATE_NOC_ISP, "pclk_noc_isp", "clk_spll", 1, 16, 0, "pclk_noc_isp" },
	{ ACLK_GATE_ASC, "aclk_asc", "clk_spll", 1, 16, 0, "aclk_asc" },
	{ CLK_GATE_DSS_AXI_MM, "clk_dss_axi_mm", "clk_spll", 1, 16, 0, "clk_dss_axi_mm" },
	{ CLK_GATE_MMBUF, "clk_mmbuf", "clk_spll", 1, 16, 0, "clk_mmbuf" },
	{ PCLK_GATE_MMBUF, "pclk_mmbuf", "clk_spll", 1, 16, 0, "pclk_mmbuf" },
	{ CLK_GATE_I2C0, "clk_i2c0", "clk_fll_src", 1, 2, 0, "clk_i2c0" },
	{ CLK_GATE_I2C1, "clk_i2c1", "clk_i2c1_gt", 1, 2, 0, "clk_i2c1" },
	{ CLK_GATE_I2C2, "clk_i2c2", "clk_fll_src", 1, 2, 0, "clk_i2c2" },
	{ CLK_GATE_SPI0, "clk_spi0", "clk_fll_src", 1, 2, 0, "clk_spi0" },
	{ CLK_FAC_180M, "clkfac_180m", "clk_spll", 1, 8, 0, "clkfac_180m" },
	{ CLK_GATE_IOMCU_PERI0, "clk_gate_peri0", "clk_spll", 1, 1, 0, "clk_gate_peri0" },
	{ CLK_GATE_SPI2, "clk_spi2", "clk_fll_src", 1, 2, 0, "clk_spi2" },
	{ CLK_GATE_UART3, "clk_uart3", "clk_gate_peri0", 1, 6, 0, "clk_uart3" },
	{ CLK_GATE_UART8, "clk_uart8", "clk_gate_peri0", 1, 6, 0, "clk_uart8" },
	{ CLK_GATE_UART7, "clk_uart7", "clk_gate_peri0", 1, 6, 0, "clk_uart7" },
	{ CLK_GNSS_ABB, "clk_gnss_abb", "clk_abb_192", 1, 1, 0, "clk_gnss_abb" },
	{ AUTODIV_ISP_DVFS, "autodiv_isp_dvfs", "autodiv_sysbus", 1, 1, 0, "autodiv_isp_dvfs" },
};

static const struct gate_clock crgctrl_gate_clks[] = {
	{ CLK_GATE_PPLL0_MEDIA, "clk_ppll0_media", "clk_spll", 0x410, 0x40000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_media" },
	{ CLK_GATE_PPLL2_MEDIA, "clk_ppll2_media", "clk_ap_ppll2", 0x410, 0x8000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_media" },
	{ CLK_GATE_PPLL4_MEDIA, "clk_ppll4_media", "clk_ap_ppll4", 0x410, 0x4000000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll4_media" },
	{ CLK_GATE_PPLL0_MEDIA2, "clk_spll_media2", "clk_spll", 0x410, 0x20000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spll_media2" },
	{ CLK_GATE_PPLL2_MEDIA2, "clk_ppll2_media2", "clk_ap_ppll2", 0x410, 0x40000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_media2" },
	{ CLK_GATE_PPLL4_MEDIA2, "clk_ppll4_media2", "clk_ap_ppll4", 0x410, 0x100000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll4_media2" },
	{ PCLK_GPIO0, "pclk_gpio0", "sc_div_cfgbus", 0x010, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio0" },
	{ PCLK_GPIO1, "pclk_gpio1", "sc_div_cfgbus", 0x010, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio1" },
	{ PCLK_GPIO2, "pclk_gpio2", "sc_div_cfgbus", 0x010, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio2" },
	{ PCLK_GPIO3, "pclk_gpio3", "sc_div_cfgbus", 0x010, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio3" },
	{ PCLK_GPIO4, "pclk_gpio4", "sc_div_cfgbus", 0x010, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio4" },
	{ PCLK_GPIO5, "pclk_gpio5", "sc_div_cfgbus", 0x010, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio5" },
	{ PCLK_GPIO6, "pclk_gpio6", "sc_div_cfgbus", 0x010, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio6" },
	{ PCLK_GPIO7, "pclk_gpio7", "sc_div_cfgbus", 0x010, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio7" },
	{ PCLK_GPIO8, "pclk_gpio8", "sc_div_cfgbus", 0x010, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio8" },
	{ PCLK_GPIO9, "pclk_gpio9", "sc_div_cfgbus", 0x010, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio9" },
	{ PCLK_GPIO10, "pclk_gpio10", "sc_div_cfgbus", 0x010, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio10" },
	{ PCLK_GPIO11, "pclk_gpio11", "sc_div_cfgbus", 0x010, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio11" },
	{ PCLK_GPIO12, "pclk_gpio12", "sc_div_cfgbus", 0x010, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio12" },
	{ PCLK_GPIO13, "pclk_gpio13", "sc_div_cfgbus", 0x010, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio13" },
	{ PCLK_GPIO14, "pclk_gpio14", "sc_div_cfgbus", 0x010, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio14" },
	{ PCLK_GPIO15, "pclk_gpio15", "sc_div_cfgbus", 0x010, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio15" },
	{ PCLK_GPIO16, "pclk_gpio16", "sc_div_cfgbus", 0x010, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio16" },
	{ PCLK_GPIO17, "pclk_gpio17", "sc_div_cfgbus", 0x010, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio17" },
	{ PCLK_GPIO18, "pclk_gpio18", "sc_div_cfgbus", 0x010, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio18" },
	{ PCLK_GPIO19, "pclk_gpio19", "sc_div_cfgbus", 0x010, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio19" },
	{ PCLK_GATE_WD0_HIGH, "pclk_wd0_high", "sc_div_cfgbus", 0x020, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0_high" },
	{ PCLK_GATE_WD0, "pclk_wd0", "clkin_ref", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0" },
	{ PCLK_GATE_WD1, "pclk_wd1", "div_sysbus_pll", 0x20, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd1" },
	{ CLK_GATE_HSDT1_USBDP, "clk_hsdt1_usbdp", "div_hsdt1_usbdp", 0x040, 0x2, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt1_usbdp" },
	{ CLK_ATDVFS, "clk_atdvfs", "clk_cssys_div", 0, 0x0, 0,
		NULL, 1, {0,0,0}, {0,1,2,3}, 3, 19, 0, "clk_atdvfs" },
	{ ACLK_GATE_PERF_STAT, "aclk_perf_stat", "clk_dmabus_div", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_perf_stat" },
	{ PCLK_GATE_PERF_STAT, "pclk_perf_stat", "clk_dmabus_div", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_perf_stat" },
	{ CLK_GATE_PERF_STAT, "clk_perf_stat", "clk_perf_div", 0x040, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat" },
	{ CLK_GATE_PERF_STAT_SUBSYS, "clk_perf_stat_subsys", "clk_perf_div", 0x040, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat_subsys" },
	{ CLK_GATE_DATA_STAT_MEDIA1, "clk_data_stat_media1", "clk_perf_div", 0x040, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_data_stat_media1" },
	{ CLK_GATE_DATA_STAT_MEDIA2, "clk_data_stat_media2", "clk_perf_div", 0x040, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_data_stat_media2" },
	{ CLK_GATE_DATA_STAT_TMSS, "clk_data_stat_tmss", "clk_perf_div", 0x040, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_data_stat_tmss" },
	{ CLK_GATE_DMAC, "clk_dmac", "div_sysbus_pll", 0x030, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dmac" },
	{ PCLK_GATE_UART4, "pclk_uart4", "clkmux_uarth", 0x020, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart4" },
	{ PCLK_GATE_UART5, "pclk_uart5", "clkmux_uartl", 0x020, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart5" },
	{ PCLK_GATE_UART0, "pclk_uart0", "clkmux_uart0", 0x020, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart0" },
	{ CLK_GATE_I2C3, "clk_i2c3", "clkmux_i2c", 0x020, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c3" },
	{ CLK_GATE_I2C4, "clk_i2c4", "clkmux_i2c", 0x020, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c4" },
	{ CLK_GATE_I2C6_ACPU, "clk_i2c6_acpu", "clkmux_i2c", 0x010, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c6_acpu" },
	{ CLK_GATE_I2C7, "clk_i2c7", "clkmux_i2c", 0x010, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c7" },
	{ PCLK_GATE_I2C3, "pclk_i2c3", "clkmux_i2c", 0x020, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c3" },
	{ PCLK_GATE_I2C4, "pclk_i2c4", "clkmux_i2c", 0x020, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c4" },
	{ PCLK_GATE_I2C6_ACPU, "pclk_i2c6_acpu", "clkmux_i2c", 0x010, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c6_acpu" },
	{ PCLK_GATE_I2C7, "pclk_i2c7", "clkmux_i2c", 0x010, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c7" },
	{ CLK_GATE_SPI1, "clk_spi1", "clkmux_spi1", 0x020, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi1" },
	{ CLK_GATE_SPI4, "clk_spi4", "clkmux_spi4", 0x040, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi4" },
	{ PCLK_GATE_SPI1, "pclk_spi1", "clkmux_spi1", 0x020, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi1" },
	{ PCLK_GATE_SPI4, "pclk_spi4", "clkmux_spi4", 0x040, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi4" },
	{ PCLK_GATE_PCTRL, "pclk_pctrl", "clk_ptp_div", 0x020, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pctrl" },
	{ CLK_GATE_AO_ASP, "clk_ao_asp", "clk_ao_asp_div", 0x000, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_asp" },
	{ PERI_VOLT_HOLD, "peri_volt_hold", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0,0,0}, {0,1,2,3}, 3, 18, 0, "peri_volt_hold" },
	{ PERI_VOLT_MIDDLE, "peri_volt_middle", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0,0}, {0,1,2}, 2, 22, 0, "peri_volt_middle" },
	{ PERI_VOLT_LOW, "peri_volt_low", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0}, {0,1}, 1, 28, 0, "peri_volt_low" },
	{ CLK_GATE_ISP_I2C_MEDIA, "clk_isp_i2c_media", "clk_div_isp_i2c", 0x030, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i2c_media" },
	{ CLK_GATE_ISP_SNCLK0, "clk_isp_snclk0", "clk_mux_ispsn0", 0x050, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk0" },
	{ CLK_GATE_ISP_SNCLK1, "clk_isp_snclk1", "clk_mux_ispsn1", 0x050, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk1" },
	{ CLK_GATE_RXDPHY0_CFG, "clk_rxdphy0_cfg", "clk_rxdcfg_mux", 0x030, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy0_cfg" },
	{ CLK_GATE_RXDPHY1_CFG, "clk_rxdphy1_cfg", "clk_rxdcfg_mux", 0x030, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy1_cfg" },
	{ CLK_GATE_TXDPHY0_CFG, "clk_txdphy0_cfg", "clk_sys_ini", 0x030, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_cfg" },
	{ CLK_GATE_MEDIA_TCXO, "clk_media_tcxo", "clk_sys_ini", 0x040, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_tcxo" },
	{ CLK_PMU32KC, "clk_pmu32kc", "clkin_ref", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pmu32kc" },
	{ AUTODIV_ISP, "autodiv_isp", "autodiv_isp_dvfs", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "autodiv_isp" },
	{ CLK_GATE_ATDIV_DMA, "clk_atdiv_dma", "clk_dmabus_div", 0x410, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_dma" },
	{ CLK_GATE_ATDIV_CFG, "clk_atdiv_cfg", "sc_div_cfgbus", 0x410, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_cfg" },
	{ CLK_GATE_ATDIV_SYS, "clk_atdiv_sys", "div_sysbus_pll", 0x410, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_sys" },
};

static const struct pll_clock pll_clks[] = {
	{ CLK_GATE_PPLL2, "clk_ap_ppll2", "clk_ppll2", 0x2,
		{ 0x954, 5 }, { 0x958, 5 }, { 0x95C, 5 }, { 0x808, 26 }, 2, 0, "clk_ap_ppll2" },
	{ CLK_GATE_PPLL4, "clk_ap_ppll4", "clk_ppll4", 0x4,
		{ 0x960, 0 }, { 0x964, 0 }, { 0x968, 0 }, { 0x818, 26 }, 2, 0, "clk_ap_ppll4" },
};

static const struct pll_clock sctrl_pll_clks[] = {
	{ CLK_GATE_AUPLL, "clk_ap_aupll", "clk_aupll", 0xC,
		{ 0x144, 0 }, { 0x148, 26 }, { 0x144, 1 }, { 0x158, 0 }, 1, 0, "clk_ap_aupll" },
};

static const struct scgt_clock crgctrl_scgt_clks[] = {
	{ AUTODIV_SYSBUS, "autodiv_sysbus", "div_sysbus_pll",
		0x410, 6, CLK_GATE_HIWORD_MASK, "autodiv_sysbus" },
	{ CLK_HSDT1_USBDP_ANDGT, "clkgt_hsdt1_usbdp", "clk_spll",
		0x70C, 2, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_hsdt1_usbdp" },
	{ CLK_ANDGT_PERF_STAT, "clk_perf_stat_gt", "clk_sw_perf_stat",
		0x0F0, 5, CLK_GATE_HIWORD_MASK, "clk_perf_stat_gt" },
	{ CLK_PERF_DIV_GT, "clk_perf_gt", "clk_perf_stat_div",
		0x0F0, 3, CLK_GATE_HIWORD_MASK, "clk_perf_gt" },
	{ CLK_GATE_CSSYS_ATCLK, "clk_cssys_atclk", "clk_dmabus_div",
		0x1A0, 9, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_cssys_atclk" },
	{ CLK_GATE_TIME_STAMP_GT, "clk_timestp_gt", "clk_sys_ini",
		0x0F0, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_timestp_gt" },
	{ CLK_GATE_TIME_STAMP, "clk_time_stamp", "clk_timestp_div",
		0x1A0, 11, CLK_GATE_HIWORD_MASK, "clk_time_stamp" },
	{ CLK_A53HPM_ANDGT, "clk_a53hpm_gt", "clk_a53hpm_mux",
		0x0F4, 7, CLK_GATE_HIWORD_MASK, "clk_a53hpm_gt" },
	{ CLK_320M_PLL_GT, "gt_clk_320m_pll", "sc_sel_320m_pll",
		0x0F8, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "gt_clk_320m_pll" },
	{ CLK_ANDGT_UARTH, "clkgt_uarth", "sc_div_320m",
		0x0F4, 11, CLK_GATE_HIWORD_MASK, "clkgt_uarth" },
	{ CLK_ANDGT_UARTL, "clkgt_uartl", "sc_div_320m",
		0x0F4, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uartl" },
	{ CLK_ANDGT_UART0, "clkgt_uart0", "sc_div_320m",
		0x0F4, 9, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uart0" },
	{ CLK_ANDGT_SPI1, "clkgt_spi1", "sc_div_320m",
		0x0F4, 13, CLK_GATE_HIWORD_MASK, "clkgt_spi1" },
	{ CLK_ANDGT_SPI4, "clkgt_spi4", "sc_div_320m",
		0x0F4, 14, CLK_GATE_HIWORD_MASK, "clkgt_spi4" },
	{ CLK_ANDGT_PTP, "clk_ptp_gt", "sc_div_320m",
		0x0F8, 5, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_ptp_gt" },
	{ CLK_DIV_AO_ASP_GT, "clk_ao_asp_gt", "clkmux_ao_asp",
		0x0F4, 4, CLK_GATE_HIWORD_MASK, "clk_ao_asp_gt" },
	{ CLK_GT_ISP_I2C, "clk_gt_isp_i2c", "sc_div_320m",
		0x0F8, 4, CLK_GATE_HIWORD_MASK, "clk_gt_isp_i2c" },
	{ CLK_ANDGT_RXDPHY, "clk_rxdcfg_gt", "clk_a53hpm_div",
		0x0F0, 12, CLK_GATE_HIWORD_MASK, "clk_rxdcfg_gt" },
	{ AUTODIV_CFGBUS, "autodiv_cfgbus", "autodiv_sysbus",
		0x404, 4, CLK_GATE_HIWORD_MASK, "autodiv_cfgbus" },
	{ AUTODIV_DMABUS, "autodiv_dmabus", "autodiv_sysbus",
		0x404, 3, CLK_GATE_HIWORD_MASK, "autodiv_dmabus" },
};

static const struct div_clock crgctrl_div_clks[] = {
	{ CLK_DIV_CFGBUS, "sc_div_cfgbus", "div_sysbus_pll",
		0x0EC, 0x3, 4, 1, 0, 0, 0, "sc_div_cfgbus" },
	{ PCLK_DIV_DBG, "pclkdiv_dbg", "clk_cssys_div",
		0x1A0, 0x6000, 4, 1, 0, 0, 0, "pclkdiv_dbg" },
	{ TRACKCLKIN_DIV, "clkdiv_track", "clk_cssys_div",
		0x128, 0x3000, 4, 1, 0, 0, 0, "clkdiv_track" },
	{ CLK_PERF_STAT_DIV, "clk_perf_stat_div", "clk_perf_stat_gt",
		0x0D0, 0x600, 4, 1, 0, 0, 0, "clk_perf_stat_div" },
	{ CLK_DIV_PERF_STAT, "clk_perf_div", "clk_perf_gt",
		0x0D0, 0xf000, 16, 1, 0, 0, 0, "clk_perf_div" },
	{ CLK_DIV_DMABUS, "clk_dmabus_div", "autodiv_dmabus",
		0x0EC, 0x8000, 2, 1, 0, 0, 0, "clk_dmabus_div" },
	{ CLK_DIV_TIME_STAMP, "clk_timestp_div", "clk_timestp_gt",
		0x1A0, 0x1c0, 8, 1, 0, 0, 0, "clk_timestp_div" },
	{ CLK_DIV_UARTH, "clkdiv_uarth", "clkgt_uarth",
		0x0B0, 0xf000, 16, 1, 0, 0, 0, "clkdiv_uarth" },
	{ CLK_DIV_UARTL, "clkdiv_uartl", "clkgt_uartl",
		0x0B0, 0xf00, 16, 1, 0, 0, 0, "clkdiv_uartl" },
	{ CLK_DIV_UART0, "clkdiv_uart0", "clkgt_uart0",
		0x0B0, 0xf0, 16, 1, 0, 0, 0, "clkdiv_uart0" },
	{ CLK_DIV_I2C, "clkdiv_i2c", "sc_div_320m",
		0x0E8, 0xf0, 16, 1, 0, 0, 0, "clkdiv_i2c" },
	{ CLK_DIV_SPI1, "clkdiv_spi1", "clkgt_spi1",
		0x0C4, 0xf000, 16, 1, 0, 0, 0, "clkdiv_spi1" },
	{ CLK_DIV_SPI4, "clkdiv_spi4", "clkgt_spi4",
		0x0D0, 0xf0, 16, 1, 0, 0, 0, "clkdiv_spi4" },
	{ CLK_DIV_PTP, "clk_ptp_div", "clk_ptp_gt",
		0x0D8, 0xf, 16, 1, 0, 0, 0, "clk_ptp_div" },
	{ CLK_DIV_AO_ASP, "clk_ao_asp_div", "clk_ao_asp_gt",
		0x108, 0x3c0, 16, 1, 0, 0, 0, "clk_ao_asp_div" },
	{ CLK_DIV_ISP_I2C, "clk_div_isp_i2c", "clk_gt_isp_i2c",
		0x0B8, 0x780, 16, 1, 0, 0, 0, "clk_div_isp_i2c" },
	{ CLK_ISP_SNCLK_DIV0, "clk_div_ispsn0", "clk_fac_ispsn",
		0x108, 0x3, 4, 1, 0, 0, 0, "clk_div_ispsn0" },
	{ CLK_ISP_SNCLK_DIV1, "clk_div_ispsn1", "clk_fac_ispsn",
		0x10C, 0xc000, 4, 1, 0, 0, 0, "clk_div_ispsn1" },
};

static const char * const clk_perf_stat_sw_p [] = { "clk_spll", "clk_ap_ppll2" };
static const char * const clk_mux_a53hpm_p [] = { "clk_spll", "clk_ap_ppll2" };
static const char * const clk_mux_320m_p [] = { "clk_ap_ppll2", "clk_spll" };
static const char * const clk_mux_uarth_p [] = { "clk_sys_ini", "clkdiv_uarth" };
static const char * const clk_mux_uartl_p [] = { "clk_sys_ini", "clkdiv_uartl" };
static const char * const clk_mux_uart0_p [] = { "clk_sys_ini", "clkdiv_uart0" };
static const char * const clk_mux_i2c_p [] = { "clk_sys_ini", "clkdiv_i2c" };
static const char * const clk_mux_spi1_p [] = { "clk_sys_ini", "clkdiv_spi1" };
static const char * const clk_mux_spi4_p [] = { "clk_sys_ini", "clkdiv_spi4" };
static const char * const clk_mux_ao_asp_p [] = { "clk_ap_ppll2", "clk_spll", "clk_invalid", "clk_invalid" };
static const char * const clk_isp_snclk_mux0_p [] = { "clk_sys_ini", "clk_div_ispsn0" };
static const char * const clk_isp_snclk_mux1_p [] = { "clk_sys_ini", "clk_div_ispsn1" };
static const char * const clk_mux_rxdphy_cfg_p [] = { "clk_rxdcfg_fac", "clk_sys_ini" };
static const struct mux_clock crgctrl_mux_clks[] = {
	{ CLK_PERF_STAT_SW, "clk_sw_perf_stat", clk_perf_stat_sw_p,
		ARRAY_SIZE(clk_perf_stat_sw_p), 0x0D0, 0x800, CLK_MUX_HIWORD_MASK, "clk_sw_perf_stat" },
	{ CLK_MUX_A53HPM, "clk_a53hpm_mux", clk_mux_a53hpm_p,
		ARRAY_SIZE(clk_mux_a53hpm_p), 0x0D4, 0x200, CLK_MUX_HIWORD_MASK, "clk_a53hpm_mux" },
	{ CLK_MUX_320M, "sc_sel_320m_pll", clk_mux_320m_p,
		ARRAY_SIZE(clk_mux_320m_p), 0x100, 0x1, CLK_MUX_HIWORD_MASK, "sc_sel_320m_pll" },
	{ CLK_MUX_UARTH, "clkmux_uarth", clk_mux_uarth_p,
		ARRAY_SIZE(clk_mux_uarth_p), 0x0AC, 0x10, CLK_MUX_HIWORD_MASK, "clkmux_uarth" },
	{ CLK_MUX_UARTL, "clkmux_uartl", clk_mux_uartl_p,
		ARRAY_SIZE(clk_mux_uartl_p), 0x0AC, 0x8, CLK_MUX_HIWORD_MASK, "clkmux_uartl" },
	{ CLK_MUX_UART0, "clkmux_uart0", clk_mux_uart0_p,
		ARRAY_SIZE(clk_mux_uart0_p), 0x0AC, 0x4, CLK_MUX_HIWORD_MASK, "clkmux_uart0" },
	{ CLK_MUX_I2C, "clkmux_i2c", clk_mux_i2c_p,
		ARRAY_SIZE(clk_mux_i2c_p), 0x0AC, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_i2c" },
	{ CLK_MUX_SPI1, "clkmux_spi1", clk_mux_spi1_p,
		ARRAY_SIZE(clk_mux_spi1_p), 0x0AC, 0x100, CLK_MUX_HIWORD_MASK, "clkmux_spi1" },
	{ CLK_MUX_SPI4, "clkmux_spi4", clk_mux_spi4_p,
		ARRAY_SIZE(clk_mux_spi4_p), 0xAC, 0x1, CLK_MUX_HIWORD_MASK, "clkmux_spi4" },
	{ CLK_MUX_AO_ASP, "clkmux_ao_asp", clk_mux_ao_asp_p,
		ARRAY_SIZE(clk_mux_ao_asp_p), 0x100, 0x30, CLK_MUX_HIWORD_MASK, "clkmux_ao_asp" },
	{ CLK_ISP_SNCLK_MUX0, "clk_mux_ispsn0", clk_isp_snclk_mux0_p,
		ARRAY_SIZE(clk_isp_snclk_mux0_p), 0x108, 0x8, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn0" },
	{ CLK_ISP_SNCLK_MUX1, "clk_mux_ispsn1", clk_isp_snclk_mux1_p,
		ARRAY_SIZE(clk_isp_snclk_mux1_p), 0x10C, 0x2000, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn1" },
	{ CLK_MUX_RXDPHY_CFG, "clk_rxdcfg_mux", clk_mux_rxdphy_cfg_p,
		ARRAY_SIZE(clk_mux_rxdphy_cfg_p), 0x0C4, 0x100, CLK_MUX_HIWORD_MASK, "clk_rxdcfg_mux" },
};

static const char *const clkin_serdes_mux_p [] = { "clk_usb2phy_ref_div", "clk_gate_abb_192" };
static const char *const clk_mux_usb2phy_ref_p [] = { "clkin_serdes_mux", "clkin_sys_div" };
static const struct mux_clock hsdtctrl_mux_clks[] = {
	{ CLKIN_SERDES_MUX, "clkin_serdes_mux", clkin_serdes_mux_p,
		ARRAY_SIZE(clkin_serdes_mux_p), 0x0A8, 0x20, CLK_MUX_HIWORD_MASK, "clkin_serdes_mux" },
	{ CLK_MUX_USB2PHY_REF, "clk_mux_usb2phy_ref", clk_mux_usb2phy_ref_p,
		ARRAY_SIZE(clk_mux_usb2phy_ref_p), 0x0A8, 0x10, CLK_MUX_HIWORD_MASK, "clk_mux_usb2phy_ref" },
};

static const struct gate_clock hsdtctrl_gate_clks[] = {
	{ CLK_GATE_USB20PHY_REF, "clk_usb20phy_ref", "clk_mux_usb2phy_ref", 0x000, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb20phy_ref" },
	{ CLK_GATE_USB21PHY_REF, "clk_usb21phy_ref", "clk_mux_usb2phy_ref", 0x000, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb21phy_ref" },
	{ CLK_GATE_USB22PHY_REF, "clk_usb22phy_ref", "clk_mux_usb2phy_ref", 0x000, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb22phy_ref" },
	{ PCLK_USB20PHY, "pclk_usb20phy", "div_hsdtbus_pll", 0x000, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb20phy" },
	{ PCLK_USB21PHY, "pclk_usb21phy", "div_hsdtbus_pll", 0x000, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb21phy" },
	{ PCLK_USB22PHY, "pclk_usb22phy", "div_hsdtbus_pll", 0x000, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb22phy" },
	{ PCLK_USB20_BUS, "pclk_usb20_bus", "div_hsdtbus_pll", 0x000, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb20_bus" },
	{ PCLK_USB21_BUS, "pclk_usb21_bus", "div_hsdtbus_pll", 0x000, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb21_bus" },
	{ CLK_USB22_BUS, "clk_usb22_bus", "div_hsdtbus_pll", 0x000, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb22_bus" },
	{ PCLK_USB20_SYSCTRL, "pclk_usb20_sysctrl", "div_hsdtbus_pll", 0x000, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb20_sysctrl" },
	{ PCLK_USB21_SYSCTRL, "pclk_usb21_sysctrl", "div_hsdtbus_pll", 0x000, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb21_sysctrl" },
	{ CLK_USB22_SYS, "clk_usb22_sys", "div_hsdtbus_pll", 0x000, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb22_sys" },
	{ CLK_SNP_USB20_SP, "clk_snp_usb20_sp", "clkin_ref", 0x000, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_snp_usb20_sp" },
	{ CLK_SNP_USB21_SP, "clk_snp_usb21_sp", "clkin_ref", 0x000, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_snp_usb21_sp" },
	{ CLK_HIS_USB22_SP, "clk_his_usb22_sp", "clkin_sys", 0x000, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_his_usb22_sp" },
	{ CLK_USB20CTRL_REF, "clk_usb20ctrl_ref", "clkdiv_in_sys", 0x000, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb20ctrl_ref" },
	{ CLK_USB21CTRL_REF, "clk_usb21ctrl_ref", "clkdiv_in_sys", 0x000, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb21ctrl_ref" },
	{ CLK_USB22CTRL_REF, "clk_usb22ctrl_ref", "clkdiv_in_sys", 0x000, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb22ctrl_ref" },
};

static const struct pll_clock hsdt1crg_pll_clks[] = {
	{ CLK_GATE_PCIE0PLL, "clk_ap_pcie0pll", "clk_ppll_pcie", 0xD,
		{ 0x228, 0 }, { 0, 0 }, { 0x228, 1 }, { 0x204, 4 }, 12, 1, "clk_ap_pcie0pll" },
};

static const struct gate_clock hsdt1ctrl_gate_clks[] = {
	{ CLK_USB3PHY_AUX, "clk_usb3phy_aux", "clkin_sys", 0x000, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3phy_aux" },
	{ PCLK_USB3PHY, "pclk_usb3phy", "clk_hsdt1_usbdp", 0x000, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3phy" },
	{ PCLK_USB2PHY, "pclk_usb2phy", "clk_hsdt1_usbdp", 0x000, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy" },
	{ CLK_USB3_HBUS, "clk_usb3_hbus", "clk_hsdt1_usbdp", 0x000, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_hbus" },
	{ CLK_USB3_SBUS, "clk_usb3_sbus", "clk_hsdt1_usbdp", 0x000, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_sbus" },
	{ CLK_HIS_USB3_SP, "clk_his_usb3_sp", "clkin_sys", 0x000, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_his_usb3_sp" },
	{ CLK_SNP_USB3_SP, "clk_snp_usb3_sp", "clkin_ref", 0x000, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_snp_usb3_sp" },
	{ CLK_USB3SCTRL_REF, "clk_usb3sctrl_ref", "clkdiv_usb3phy_ref", 0x000, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3sctrl_ref" },
	{ CLK_USB3HCTRL_REF, "clk_usb3hctrl_ref", "clkdiv_usb3phy_ref", 0x000, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3hctrl_ref" },
	{ PCLK_USB3_SYSCTRL, "pclk_usb3_sysctrl", "clk_hsdt1_usbdp", 0x000, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_sysctrl" },
	{ CLK_USB3_SYS, "clk_usb3_sys", "clk_hsdt1_usbdp", 0x000, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_sys" },
	{ CLK_USB3_UCE, "clk_usb3_uce", "clk_hsdt1_usbdp", 0x000, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_uce" },
	{ CLK_USB3_UBUS, "clk_usb3_ubus", "clk_hsdt1_usbdp", 0x000, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_ubus" },
	{ PCLK_USB3_DJTAG, "pclk_usb3_djtag", "clk_hsdt1_usbdp", 0x000, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_djtag" },
	{ CLK_USB3_M32K, "clk_usb3_m32k", "clkin_ref", 0x000, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_m32k" },
	{ CLK_USB3_M19P2, "clk_usb3_m19p2", "clkdiv_usb3phy_ref", 0x000, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_m19p2" },
	{ CLK_USB2PHY_REF, "clk_usb2phy_ref", "clkdiv_usb3phy_ref", 0x000, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref" },
	{ CLK_GATE_PCIEPHY_REF, "clk_pciephy_ref", "clk_ppll_pcie", 0x020, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pciephy_ref" },
	{ CLK_GATE_PCIE1PHY_REF, "clk_pcie1phy_ref", "clk_ppll_pcie", 0x020, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie1phy_ref" },
	{ PCLK_GATE_PCIE_SYS, "pclk_pcie_sys", "clk_hsdt1_usbdp", 0x020, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_sys" },
	{ PCLK_GATE_PCIE1_SYS, "pclk_pcie1_sys", "clk_hsdt1_usbdp", 0x020, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_sys" },
	{ PCLK_GATE_PCIE_PHY, "pclk_pcie_phy", "clk_hsdt1_usbdp", 0x020, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie_phy" },
	{ PCLK_GATE_PCIE1_PHY, "pclk_pcie1_phy", "clk_hsdt1_usbdp", 0x020, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pcie1_phy" },
	{ CLK_GATE_PCIEPHY_AUX, "clk_pciephy_aux", "clk_pcieaux", 0x020, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pciephy_aux" },
	{ CLK_GATE_PCIE1PHY_AUX, "clk_pcie1phy_aux", "clk_pcieaux", 0x020, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie1phy_aux" },
	{ ACLK_GATE_PCIE, "aclk_pcie", "clk_hsdt1_usbdp", 0x020, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie" },
	{ ACLK_GATE_PCIE1, "aclk_pcie1", "clk_hsdt1_usbdp", 0x020, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_pcie1" },
	{ CLK_GATE_HSDT_TBU, "clk_hsdt_tbu", "clk_hsdt1_usbdp", 0x020, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt_tbu" },
	{ CLK_GATE_PCIE1_TBU, "clk_pcie1_tbu", "clk_hsdt1_usbdp", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie1_tbu" },
	{ CLK_GATE_HSDT_TCU, "clk_hsdt_tcu", "clk_hsdt1_usbdp", 0x020, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt_tcu" },
	{ CLK_GATE_PCIE0_MCU, "clk_pcie0_mcu", "clk_hsdt1_usbdp", 0x020, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie0_mcu" },
	{ CLK_GATE_PCIE0_MCU_BUS, "clk_pcie0_mcu_bus", "clk_hsdt1_usbdp", 0x020, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie0_mcu_bus" },
	{ CLK_GATE_PCIE0_MCU_19P2, "clk_pcie0_mcu_19p2", "clk_sys_ini", 0x020, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie0_mcu_19p2" },
	{ CLK_GATE_PCIE0_MCU_32K, "clk_pcie0_mcu_32k", "clkin_ref", 0x020, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcie0_mcu_32k" },
};

static const struct div_clock sctrl_div_clks[] = {
	{ CLK_DIV_AOBUS, "sc_div_aobus", "clk_spll",
		0x254, 0x3f, 64, 1, 0, 0, 0, "sc_div_aobus" },
	{ CLK_PCIE_AUX_DIV, "clk_pcie_aux_div", "clk_pcie_aux_andgt",
		0x254, 0xf00, 16, 1, 0, 0, 0, "clk_pcie_aux_div" },
	{ CLK_DIV_IOPERI, "sc_div_ioperi", "clk_gt_ioperi",
		0x270, 0x3f, 64, 1, 0, 0, 0, "sc_div_ioperi" },
	{ CLK_DIV_SPI5, "clkdiv_spi5", "clkgt_spi5",
		0x2B0, 0x3f, 64, 1, 0, 0, 0, "clkdiv_spi5" },
	{ CLKDIV_ASP_CODEC_PLL, "clkdiv_asp_codec_pll", "clkgt_asp_codec",
		0x298, 0xfc0, 64, 1, 0, 0, 0, "clkdiv_asp_codec_pll" },
};

static const char * const clk_pcie_aux_mux_p [] = { "clk_sys_ini", "clk_pcie_aux_div" };
static const char * const clk_mux_ioperi_p [] = { "clk_fll_src", "clk_spll" };
static const char * const clk_mux_spi5_p [] = { "clk_fll_src", "clk_spll" };
static const char * const clkmux_syscnt_p [] = { "clk_syscnt_div", "clkin_ref" };
static const char * const clk_mux_asp_codec_p [] = { "clk_ap_aupll", "clk_spll" };
static const char * const clk_mux_asp_pll_p [] = {
		"clk_invalid", "clk_spll", "clk_fll_src", "clk_invalid",
		"sel_ao_asp", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_sys_ini", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_ao_asp_mux_p [] = { "clk_ao_asp", "clk_ap_aupll" };
static const struct mux_clock sctrl_mux_clks[] = {
	{ CLK_PCIE_AUX_MUX, "clk_pcie_aux_mux", clk_pcie_aux_mux_p,
		ARRAY_SIZE(clk_pcie_aux_mux_p), 0x264, 0x4000, CLK_MUX_HIWORD_MASK, "clk_pcie_aux_mux" },
	{ CLK_MUX_IOPERI, "clk_ioperi_mux", clk_mux_ioperi_p,
		ARRAY_SIZE(clk_mux_ioperi_p), 0x294, 0x4000, CLK_MUX_HIWORD_MASK, "clk_ioperi_mux" },
	{ CLK_MUX_SPI5, "clkmux_spi5", clk_mux_spi5_p,
		ARRAY_SIZE(clk_mux_spi5_p), 0x294, 0x8000, CLK_MUX_HIWORD_MASK, "clkmux_spi5" },
	{ CLKMUX_SYSCNT, "clkmux_syscnt", clkmux_syscnt_p,
		ARRAY_SIZE(clkmux_syscnt_p), 0x264, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_syscnt" },
	{ CLK_MUX_ASP_CODEC, "sel_asp_codec", clk_mux_asp_codec_p,
		ARRAY_SIZE(clk_mux_asp_codec_p), 0x260, 0x8000, CLK_MUX_HIWORD_MASK, "sel_asp_codec" },
	{ CLK_MUX_ASP_PLL, "clk_asp_pll_sel", clk_mux_asp_pll_p,
		ARRAY_SIZE(clk_mux_asp_pll_p), 0x280, 0xf, CLK_MUX_HIWORD_MASK, "clk_asp_pll_sel" },
	{ CLK_AO_ASP_MUX, "sel_ao_asp", clk_ao_asp_mux_p,
		ARRAY_SIZE(clk_ao_asp_mux_p), 0x250, 0x1000, CLK_MUX_HIWORD_MASK, "sel_ao_asp" },
};

static const struct gate_clock sctrl_gate_clks[] = {
	{ PCLK_GPIO20, "pclk_gpio20", "sc_div_aobus", 0x1B0, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio20" },
	{ PCLK_GPIO21, "pclk_gpio21", "sc_div_aobus", 0x1B0, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio21" },
	{ CLK_GATE_TIMER5, "clk_timer5", "clk_fll_src", 0x170, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_timer5" },
	{ CLK_GATE_PCIEAUX, "clk_pcieaux", "clk_pcie_aux_mux", 0x1B0, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_pcieaux" },
	{ CLK_GATE_SPI, "clk_spi3", "sc_div_ioperi", 0x1B0, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi3" },
	{ PCLK_GATE_SPI, "pclk_spi3", "sc_div_ioperi", 0x1B0, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi3" },
	{ CLK_GATE_SPI5, "clk_spi5", "clkdiv_spi5", 0x1B0, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi5" },
	{ PCLK_GATE_SPI5, "pclk_spi5", "clkdiv_spi5", 0x1B0, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi5" },
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
	{ CLK_ASP_CODEC, "clk_asp_codec", "clkdiv_asp_codec", 0x170, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_codec" },
	{ CLK_GATE_ASP_SUBSYS, "clk_asp_subsys", "clk_asp_pll_sel", 0x170, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_subsys" },
};

static const struct scgt_clock sctrl_scgt_clks[] = {
	{ CLK_PCIE_AUX_ANDGT, "clk_pcie_aux_andgt", "clk_fll_src",
		0x254, 7, CLK_GATE_HIWORD_MASK, "clk_pcie_aux_andgt" },
	{ CLK_ANDGT_IOPERI, "clk_gt_ioperi", "clk_ioperi_mux",
		0x270, 6, CLK_GATE_HIWORD_MASK, "clk_gt_ioperi" },
	{ CLK_ANDGT_SPI5, "clkgt_spi5", "clkmux_spi5",
		0x2B0, 6, CLK_GATE_HIWORD_MASK, "clkgt_spi5" },
	{ CLKGT_ASP_CODEC, "clkgt_asp_codec", "sel_asp_codec",
		0x274, 14, CLK_GATE_HIWORD_MASK, "clkgt_asp_codec" },
};

static const struct gate_clock iomcu_gate_clks[] = {
	{ CLK_I2C1_GATE_IOMCU, "clk_i2c1_gt", "clk_fll_src", 0x010, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c1_gt" },
};

static const struct div_clock media1crg_div_clks[] = {
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_vivobus_gt",
		0x060, 0x3f, 64, 1, 0, 0, 0, "clk_vivobus_div" },
	{ CLK_DIV_EDC0, "clk_edc0_div", "clk_edc0_gt",
		0x070, 0x3f, 64, 1, 0, 0, 0, "clk_edc0_div" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_ispcpu_gt",
		0x068, 0x3f, 64, 1, 0, 0x0A0, 5, "clk_ispcpu_div" },
	{ CLK_DIV_ISPFUNC, "clkdiv_ispfunc", "clkgt_ispfunc",
		0x064, 0x3f, 64, 1, 0, 0x0A0, 2, "clkdiv_ispfunc" },
};

static const char * const clk_mux_vivobus_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_ppll2_media", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_edc0_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_ppll2_media", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispi2c_p [] = { "clk_media_tcxo", "clk_isp_i2c_media" };
static const char * const clk_mux_ispcpu_p [] = {
		"clk_invalid", "clk_ppll0_media", "clk_ppll2_media", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char * const clk_mux_ispfunc_p [] = {
		"clk_invalid", "clk_invalid", "clk_ppll0_media", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_ppll4_media",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media1crg_mux_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", clk_mux_vivobus_p,
		ARRAY_SIZE(clk_mux_vivobus_p), 0x060, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vivobus_mux" },
	{ CLK_MUX_EDC0, "sel_edc0_pll", clk_mux_edc0_p,
		ARRAY_SIZE(clk_mux_edc0_p), 0x070, 0x3c0, CLK_MUX_HIWORD_MASK, "sel_edc0_pll" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", clk_mux_ispi2c_p,
		ARRAY_SIZE(clk_mux_ispi2c_p), 0x064, 0x400, CLK_MUX_HIWORD_MASK, "clk_ispi2c_mux" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", clk_mux_ispcpu_p,
		ARRAY_SIZE(clk_mux_ispcpu_p), 0x068, 0x3c0, CLK_MUX_HIWORD_MASK, "sel_ispcpu" },
	{ CLK_MUX_ISPFUNC, "clkmux_ispfunc", clk_mux_ispfunc_p,
		ARRAY_SIZE(clk_mux_ispfunc_p), 0x064, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_ispfunc" },
};

static const struct gate_clock media1crg_gate_clks[] = {
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_vivobus_div", 0x010, 0x4000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vivobus" },
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", "clk_edc0_div", 0x010, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_edc0freq" },
	{ ACLK_GATE_ISP, "aclk_isp", "clk_vivobus_div", 0x000, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_ispi2c_mux", 0x000, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispi2c" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_ispcpu_div", 0x000, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispcpu" },
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clkdiv_ispfunc", 0x000, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_vivobus_div", 0x000, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vivo" },
};

static const struct scgt_clock media1crg_scgt_clks[] = {
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_vivobus_mux",
		0x06c, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vivobus_gt" },
	{ CLK_ANDGT_EDC0, "clk_edc0_gt", "sel_edc0_pll",
		0x06C, 3, CLK_GATE_HIWORD_MASK, "clk_edc0_gt" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "sel_ispcpu",
		0x06C, 2, CLK_GATE_HIWORD_MASK, "clk_ispcpu_gt" },
	{ CLK_ANDGT_ISPFUNC, "clkgt_ispfunc", "clkmux_ispfunc",
		0x06C, 0, CLK_GATE_HIWORD_MASK, "clkgt_ispfunc" },
};

static const struct div_clock media2crg_div_clks[] = {
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_vcodbus_gt",
		0x088, 0x3f, 64, 1, 0, 0, 0, "clk_vcodbus_div" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clkgt_vdec",
		0x080, 0x3f, 64, 1, 0, 0x118, 13, "clkdiv_vdec" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clkgt_venc",
		0x084, 0x3f, 64, 1, 0, 0x118, 12, "clkdiv_venc" },
};

static const char *const clk_mux_vcodecbus_p [] = {
		"clk_invalid", "clk_spll_media2", "clk_ppll2_media2", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_vdec_p [] = {
		"clk_invalid", "clk_spll_media2", "clk_ppll2_media2", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_venc_p [] = {
		"clk_invalid", "clk_spll_media2", "clk_ppll2_media2", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll4_media2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media2crg_mux_clks[] = {
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", clk_mux_vcodecbus_p,
		ARRAY_SIZE(clk_mux_vcodecbus_p), 0x088, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vcodbus_mux" },
	{ CLK_MUX_VDEC, "clkmux_vdec", clk_mux_vdec_p,
		ARRAY_SIZE(clk_mux_vdec_p), 0x080, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_vdec" },
	{ CLK_MUX_VENC, "clkmux_venc", clk_mux_venc_p,
		ARRAY_SIZE(clk_mux_venc_p), 0x084, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_venc" },
};

static const struct gate_clock media2crg_gate_clks[] = {
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_vcodbus_div", 0x000, 0x4000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clkdiv_vdec", 0x000, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vdecfreq" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clkdiv_venc", 0x000, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vencfreq" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_vcodbus_div", 0x000, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clkdiv_vdec", 0x000, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vdec" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clkdiv_venc", 0x000, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_venc" },
};

static const struct scgt_clock media2crg_scgt_clks[] = {
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_vcodbus_mux",
		0x08c, 2, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vcodbus_gt" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clkmux_vdec",
		0x08c, 0, CLK_GATE_HIWORD_MASK, "clkgt_vdec" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clkmux_venc",
		0x08c, 1, CLK_GATE_HIWORD_MASK, "clkgt_venc" },
};

static const struct xfreq_clock xfreq_clks[] = {
	{ CLK_CLUSTER0, "cpu-cluster.0", 0, 0, 0, 0x41C, { 0x0001030A, 0x0 }, { 0x0001020A, 0x0 }, "cpu-cluster.0" },
	{ CLK_CLUSTER1, "cpu-cluster.1", 1, 0, 0, 0x41C, { 0x0002030A, 0x0 }, { 0x0002020A, 0x0 }, "cpu-cluster.1" },
	{ CLK_G3D, "clk_g3d", 2, 0, 0, 0x41C, { 0x0003030A, 0x0 }, { 0x0003020A, 0x0 }, "clk_g3d" },
	{ CLK_DDRC_FREQ, "clk_ddrc_freq", 3, 0, 0, 0x41C, { 0x00040309, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_freq" },
	{ CLK_DDRC_MAX, "clk_ddrc_max", 5, 1, 0x8000, 0x250, { 0x00040308, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_max" },
	{ CLK_DDRC_MIN, "clk_ddrc_min", 4, 1, 0x8000, 0x270, { 0x00040309, 0x0 }, { 0x0004020A, 0x0 }, "clk_ddrc_min" },
	{ CLK_DMSS_MIN, "clk_dmss_min", 6, 1, 0x8000, 0x230, { 0x00040309, 0x0 }, { 0x0004020A, 0x0 }, "clk_dmss_min" },
};

static const struct pmu_clock pmu_clks[] = {
	{ CLK_GATE_ABB_192, "clk_abb_192", "clkin_sys", 0x049, 0, 9, 0, "clk_abb_192" },
	{ CLK_PMU32KA, "clk_pmu32ka", "clkin_ref", 0x051, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32ka" },
	{ CLK_PMU32KB, "clk_pmu32kb", "clkin_ref", 0x050, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kb" },
	{ CLK_PMUAUDIOCLK, "clk_pmuaudioclk", "clk_sys_ini", 0x04E, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmuaudioclk" },
	{ CLK_EUSB_38M4, "clk_eusb_38m4", "clk_sys_ini", 0x48, 0, INVALID_HWSPINLOCK_ID, 0, "clk_eusb_38m4" },
};

static const struct dvfs_clock dvfs_clks[] = {
	{ CLK_GATE_EDC0, "clk_edc0", "clk_edc0freq", 20, -1, 3, 1,
		{ 231700, 325000, 406000 }, { 0, 1, 2, 3 }, 406000, 1, "clk_edc0" },
	{ CLK_GATE_VDEC, "clk_vdec", "clk_vdecfreq", 21, -1, 2, 1,
		{ 271000, 406000, 540000 }, { 0, 1, 2 }, 406000, 1, "clk_vdec" },
	{ CLK_GATE_VENC, "clk_venc", "clk_vencfreq", 24, -1, 3, 1,
		{ 325000, 480000, 640000 }, { 0, 1, 2, 3 }, 640000, 1, "clk_venc" },
	{ CLK_GATE_ISPFUNC, "clk_ispfunc", "clk_ispfuncfreq", 26, -1, 2, 1,
		{ 181000, 325000, 480000 }, { 0, 1, 2 }, 325000, 1, "clk_ispfunc" },
};

static const struct fixed_factor_clock media1_stub_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", "clk_sys_ini", 1, 1, 0, "clk_vivobus_mux" },
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_sys_ini", 1, 1, 0, "clk_vivobus_gt" },
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_sys_ini", 1, 1, 0, "clk_vivobus_div" },
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_sys_ini", 1, 1, 0, "clk_vivobus" },
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", "clk_sys_ini", 1, 1, 0, "clk_edc0freq" },
	{ CLK_DIV_EDC0, "clk_edc0_div", "clk_sys_ini", 1, 1, 0, "clk_edc0_div" },
	{ CLK_ANDGT_EDC0, "clk_edc0_gt", "clk_sys_ini", 1, 1, 0, "clk_edc0_gt" },
	{ CLK_MUX_EDC0, "sel_edc0_pll", "clk_sys_ini", 1, 1, 0, "sel_edc0_pll" },
	{ ACLK_GATE_ISP, "aclk_isp", "clk_sys_ini", 1, 1, 0, "aclk_isp" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", "clk_sys_ini", 1, 1, 0, "clk_ispi2c_mux" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_sys_ini", 1, 1, 0, "clk_ispi2c" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", "clk_sys_ini", 1, 1, 0, "sel_ispcpu" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_gt" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_div" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_sys_ini", 1, 1, 0, "clk_ispcpu" },
	{ CLK_MUX_ISPFUNC, "clkmux_ispfunc", "clk_sys_ini", 1, 1, 0, "clkmux_ispfunc" },
	{ CLK_ANDGT_ISPFUNC, "clkgt_ispfunc", "clk_sys_ini", 1, 1, 0, "clkgt_ispfunc" },
	{ CLK_DIV_ISPFUNC, "clkdiv_ispfunc", "clk_sys_ini", 1, 1, 0, "clkdiv_ispfunc" },
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clk_sys_ini", 1, 1, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vivo" },
};

static const struct fixed_factor_clock media2_stub_clks[] = {
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_sys_ini", 1, 1, 0, "clk_vcodecbus" },
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_div" },
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_gt" },
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_mux" },
	{ CLK_MUX_VDEC, "clkmux_vdec", "clk_sys_ini", 1, 1, 0, "clkmux_vdec" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clk_sys_ini", 1, 1, 0, "clkgt_vdec" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clk_sys_ini", 1, 1, 0, "clkdiv_vdec" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clk_sys_ini", 1, 1, 0, "clk_vdecfreq" },
	{ CLK_MUX_VENC, "clkmux_venc", "clk_sys_ini", 1, 1, 0, "clkmux_venc" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clk_sys_ini", 1, 1, 0, "clkgt_venc" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clk_sys_ini", 1, 1, 0, "clkdiv_venc" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clk_sys_ini", 1, 1, 0, "clk_vencfreq" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vdec" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clk_sys_ini", 1, 1, 0, "clk_atdiv_venc" },
};

static void clk_crgctrl_init(struct device_node *np)
{
	struct clock_data *clk_crgctrl_data = NULL;
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
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(hsdtctrl_mux_clks) +
		ARRAY_SIZE(hsdtctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_mux(hsdtctrl_mux_clks,
		ARRAY_SIZE(hsdtctrl_mux_clks), clk_data);

	plat_clk_register_gate(hsdtctrl_gate_clks,
		ARRAY_SIZE(hsdtctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdtcrg,
	"hisilicon,clk-extreme-hsdt-crg", clk_hsdt_init);

static void clk_hsdt1_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(hsdt1crg_pll_clks) +
		ARRAY_SIZE(hsdt1ctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_pll(hsdt1crg_pll_clks,
		ARRAY_SIZE(hsdt1crg_pll_clks), clk_data);

	plat_clk_register_gate(hsdt1ctrl_gate_clks,
		ARRAY_SIZE(hsdt1ctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdt1crg,
	"hisilicon,clk-extreme-hsdt1-crg", clk_hsdt1_init);

static void clk_sctrl_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(sctrl_pll_clks) +
		ARRAY_SIZE(sctrl_scgt_clks) +
		ARRAY_SIZE(sctrl_div_clks) +
		ARRAY_SIZE(sctrl_mux_clks) +
		ARRAY_SIZE(sctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_pll(sctrl_pll_clks,
		ARRAY_SIZE(sctrl_pll_clks), clk_data);

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

static void clk_media1_init(struct device_node *np)
{
	int nr;
	struct clock_data *clk_data = NULL;
	int no_media = media_check(MEDIA1);
	if (no_media)
		nr = ARRAY_SIZE(media1_stub_clks);
	else
		nr = ARRAY_SIZE(media1crg_scgt_clks) +
		ARRAY_SIZE(media1crg_div_clks) +
		ARRAY_SIZE(media1crg_mux_clks) +
		ARRAY_SIZE(media1crg_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	if (no_media) {
		pr_err("[%s] media1 clock won't initialize!\n", __func__);
		plat_clk_register_fixed_factor(media1_stub_clks,
			ARRAY_SIZE(media1_stub_clks), clk_data);
	} else {
		plat_clk_register_divider(media1crg_div_clks,
			ARRAY_SIZE(media1crg_div_clks), clk_data);

		plat_clk_register_mux(media1crg_mux_clks,
			ARRAY_SIZE(media1crg_mux_clks), clk_data);

		plat_clk_register_scgt(media1crg_scgt_clks,
			ARRAY_SIZE(media1crg_scgt_clks), clk_data);

		plat_clk_register_gate(media1crg_gate_clks,
			ARRAY_SIZE(media1crg_gate_clks), clk_data);
	}
}
CLK_OF_DECLARE_DRIVER(clk_media1crg,
	"hisilicon,clk-extreme-media1-crg", clk_media1_init);

static void clk_media2_init(struct device_node *np)
{
	int nr;
	struct clock_data *clk_data = NULL;
	int no_media = media_check(MEDIA2);
	if (no_media)
		nr = ARRAY_SIZE(media2_stub_clks);
	else
		nr = ARRAY_SIZE(media2crg_scgt_clks) +
			ARRAY_SIZE(media2crg_div_clks) +
			ARRAY_SIZE(media2crg_mux_clks) +
			ARRAY_SIZE(media2crg_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	if (no_media) {
		pr_err("[%s] media2 clock won't initialize!\n", __func__);
		plat_clk_register_fixed_factor(media2_stub_clks,
			ARRAY_SIZE(media2_stub_clks), clk_data);
	} else {
		plat_clk_register_divider(media2crg_div_clks,
			ARRAY_SIZE(media2crg_div_clks), clk_data);

		plat_clk_register_mux(media2crg_mux_clks,
			ARRAY_SIZE(media2crg_mux_clks), clk_data);

		plat_clk_register_scgt(media2crg_scgt_clks,
			ARRAY_SIZE(media2crg_scgt_clks), clk_data);

		plat_clk_register_gate(media2crg_gate_clks,
			ARRAY_SIZE(media2crg_gate_clks), clk_data);
	}
}
CLK_OF_DECLARE_DRIVER(clk_media2crg,
	"hisilicon,clk-extreme-media2-crg", clk_media2_init);

static void clk_iomcu_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
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
	struct clock_data *clk_data = NULL;
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
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(pmu_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_clkpmu(pmu_clks,
		ARRAY_SIZE(pmu_clks), clk_data);
}
CLK_OF_DECLARE_DRIVER(clk_pmu,
	"hisilicon,clk-extreme-pmu", clk_pmu_init);

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
CLK_OF_DECLARE_DRIVER(clk_dvfs,
	"hisilicon,clk-extreme-dvfs", clk_dvfs_init);
