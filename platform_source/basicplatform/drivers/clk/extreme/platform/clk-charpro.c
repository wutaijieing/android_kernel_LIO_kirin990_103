/*
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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
#include "clk-charpro.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>

static const struct fixed_rate_clock fixed_clks[] = {
	{ CLKIN_SYS, "clkin_sys", NULL, 0, 38400000, "clkin_sys" },
	{ CLK_38M4_HSDT, "clk_hsdt_38m4", NULL, 0, 38400000, "clk_hsdt_38m4" },
	{ CLK_SYSTEM_HSDT0, "clk_system_hsdt0", NULL, 0, 38400000, "clk_system_hsdt0" },
	{ CLKIN_SYS_SERDES, "clkin_sys_serdes", NULL, 0, 38400000, "clkin_sys_serdes" },
	{ CLKIN_REF, "clkin_ref", NULL, 0, 32764, "clkin_ref" },
	{ CLK_FLL_SRC, "clk_fll_src", NULL, 0, 208906485, "clk_fll_src" },
	{ CLK_PPLL1, "clk_ppll1", NULL, 0, 1440000000, "clk_ppll1" },
	{ CLK_PPLL2, "clk_ppll2", NULL, 0, 964000000, "clk_ppll2" },
	{ CLK_PPLL2_B, "clk_ppll2_b", NULL, 0, 1928000000, "clk_ppll2_b" },
	{ CLK_PPLL3, "clk_ppll3", NULL, 0, 1066000000, "clk_ppll3" },
	{ CLK_PPLL5, "clk_ppll5", NULL, 0, 960000000, "clk_ppll5" },
	{ CLK_PPLL7, "clk_ppll7", NULL, 0, 1290000000, "clk_ppll7" },
	{ CLK_SPLL, "clk_spll", NULL, 0, 1671168000, "clk_spll" },
	{ CLK_MODEM_BASE, "clk_modem_base", NULL, 0, 49152000, "clk_modem_base" },
	{ CLK_ULPPLL_1, "clk_ulppll_1", NULL, 0, 300024573, "clk_ulppll_1" },
	{ CLK_AUPLL, "clk_aupll", NULL, 0, 1572864000, "clk_aupll" },
	{ CLK_SCPLL, "clk_scpll", NULL, 0, 100000000, "clk_scpll" },
	{ CLK_FNPLL, "clk_fnpll", NULL, 0, 100000000, "clk_fnpll" },
	{ CLK_PPLL_PCIE, "clk_ppll_pcie", NULL, 0, 100000000, "clk_ppll_pcie" },
	{ CLK_PPLL_PCIE1, "clk_ppll_pcie1", NULL, 0, 100000000, "clk_ppll_pcie1" },
	{ CLK_SDPLL, "clk_sdpll", NULL, 0, 964000000, "clk_sdpll" },
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
	{ CLK_PPLL_EPS, "clk_ppll_eps", "clk_ap_ppll2", 1, 1, 0, "clk_ppll_eps" },
	{ CLK_GATE_HIFACE, "clk_hiface", "clk_sys_ini", 1, 1, 0, "clk_hiface" },
	{ CLK_SYS_INI, "clk_sys_ini", "clkin_sys", 1, 2, 0, "clk_sys_ini" },
	{ CLK_DIV_SYSBUS, "div_sysbus_pll", "clk_spll", 1, 7, 0, "div_sysbus_pll" },
	{ CLK_DIV_CFGBUS, "sc_div_cfgbus", "div_sysbus_pll", 1, 1, 0, "sc_div_cfgbus" },
	{ CLK_GATE_WD0_HIGH, "clk_wd0_high", "div_sysbus_pll", 1, 1, 0, "clk_wd0_high" },
	{ ACLK_GATE_MEDIA_COMMON, "aclk_media_common", "clk_spll", 1, 1, 0, "aclk_media_common" },
	{ ACLK_GATE_QIC_DSS, "aclk_qic_dss", "clk_spll", 1, 1, 0, "aclk_qic_dss" },
	{ PCLK_GATE_MEDIA_COMMON, "pclk_media_common", "clk_spll", 1, 1, 0, "pclk_media_common" },
	{ PCLK_GATE_QIC_DSS_CFG, "pclk_qic_dss_cfg", "clk_spll", 1, 1, 0, "pclk_qic_dss_cfg" },
	{ PCLK_GATE_MMBUF_CFG, "pclk_mmbuf_cfg", "clk_spll", 1, 1, 0, "pclk_mmbuf_cfg" },
	{ PCLK_GATE_DISP_QIC_SUBSYS, "pclk_disp_qic_subsys", "clk_spll", 1, 1, 0, "pclk_disp_qic_subsys" },
	{ ACLK_GATE_DISP_QIC_SUBSYS, "aclk_disp_qic_subsys", "clk_spll", 1, 1, 0, "aclk_disp_qic_subsys" },
	{ PCLK_GATE_DSS, "pclk_dss", "clk_spll", 1, 3, 0, "pclk_dss" },
	{ CLK_DIV_AOBUS_334M, "div_aobus_334M", "clk_spll", 1, 5, 0, "div_aobus_334M" },
	{ CLK_DIV_AOBUS, "sc_div_aobus", "div_aobus_334M", 1, 6, 0, "sc_div_aobus" },
	{ CLK_FACTOR_TCXO, "clk_factor_tcxo", "clk_sys_ini", 1, 4, 0, "clk_factor_tcxo" },
	{ CLK_GATE_TIMER5_B, "clk_timer5_b", "clkmux_timer5_b", 1, 1, 0, "clk_timer5_b" },
	{ CLK_GATE_TIMER5, "clk_timer5", "sc_div_aobus", 1, 1, 0, "clk_timer5" },
	{ HCLK_GATE_USB3OTG, "hclk_usb3otg", "clk_hsdt1_usbdp", 1, 1, 0, "hclk_usb3otg" },
	{ ACLK_GATE_USB3OTG, "aclk_usb3otg", "clkmux_usb_dock", 1, 1, 0, "aclk_usb3otg" },
	{ ACLK_GATE_USB3OTG_ASYNCBRG, "aclk_asyncbrg", "clkmux_usb_dock", 1, 1, 0, "aclk_asyncbrg" },
	{ CLK_MUX_USB_DOCK, "clkmux_usb_dock", "clk_hsdt1_usbdp", 1, 1, 0, "clkmux_usb_dock" },
	{ ATCLK, "clk_at", "clk_atdvfs", 1, 1, 0, "clk_at" },
	{ PCLK_DBG, "pclk_dbg", "pclkdiv_dbg", 1, 1, 0, "pclk_dbg" },
	{ CLK_DIV_CSSYSDBG, "clk_cssys_div", "autodiv_sysbus", 1, 1, 0, "clk_cssys_div" },
	{ CLK_GATE_CSSYSDBG, "clk_cssysdbg", "div_sysbus_pll", 1, 1, 0, "clk_cssysdbg" },
	{ CLK_GATE_DMAC, "clk_dmac", "div_sysbus_pll", 1, 1, 0, "clk_dmac" },
	{ ACLK_GATE_DSS, "aclk_dss", "clk_spll", 1, 3, 0, "aclk_dss" },
	{ CLK_GATE_SDIO, "clk_sdio", "clk_sdpll", 1, 1, 0, "clk_sdio" },
	{ PCLK_GATE_DSI0, "pclk_dsi0", "clk_sys_ini", 1, 1, 0, "pclk_dsi0" },
	{ PCLK_GATE_DSI1, "pclk_dsi1", "clk_sys_ini", 1, 1, 0, "pclk_dsi1" },
	{ CLK_GATE_LDI0, "clk_ldi0", "clk_sys_ini", 1, 1, 0, "clk_ldi0" },
	{ CLK_GATE_VENC2, "clk_venc2", "clk_sys_ini", 1, 1, 0, "clk_venc2" },
	{ CLK_GATE_PPLL5, "clk_ap_ppll5", "clk_sys_ini", 1, 1, 0, "clk_ap_ppll5" },
	{ CLK_GATE_FD_FUNC, "clk_fd_func", "clk_sys_ini", 1, 1, 0, "clk_fd_func" },
	{ CLK_GATE_FDAI_FUNC, "clk_fdai_func", "clk_sys_ini", 1, 1, 0, "clk_fdai_func" },
	{ CLK_DIV_HSDT1BUS, "clk_div_hsdt1bus", "div_sysbus_pll", 1, 1, 0, "clk_div_hsdt1bus" },
	{ CLK_ANDGT_SDPLL, "clkgt_sdpll", "clk_sdpll", 1, 1, 0, "clkgt_sdpll" },
	{ CLK_MUX_SD_PLL, "clk_sd_muxpll", "clk_sys_ini", 1, 1, 0, "clk_sd_muxpll" },
	{ CLK_DIV_SD, "clk_sd_div", "clk_sd_gt", 1, 1, 0, "clk_sd_div" },
	{ CLK_ANDGT_SD, "clk_sd_gt", "clk_sd_muxpll", 1, 1, 0, "clk_sd_gt" },
	{ CLK_SD_SYS, "clk_sd_sys", "clk_sd_sys_gt", 1, 6, 0, "clk_sd_sys" },
	{ CLK_DIV_320M, "sc_div_320m", "gt_clk_320m_pll", 1, 5, 0, "sc_div_320m" },
	{ PCLK_GATE_UART0, "pclk_uart0", "clk_uart0", 1, 1, 0, "pclk_uart0" },
	{ CLK_FACTOR_UART0, "clk_uart0_fac", "clkmux_uart0", 1, 1, 0, "clk_uart0_fac" },
	{ CLK_GATE_UART3, "clk_uart3", "clk_gate_peri0", 1, 9, 0, "clk_uart3" },
	{ CLK_GATE_UART7, "clk_uart7", "clk_gate_peri0", 1, 9, 0, "clk_uart7" },
	{ CLK_GATE_I2C0, "clk_i2c0", "clk_fll_src", 1, 2, 0, "clk_i2c0" },
	{ CLK_GATE_I2C1, "clk_i2c1", "clk_i2c1_gt", 1, 2, 0, "clk_i2c1" },
	{ CLK_GATE_I2C2, "clk_i2c2", "clk_fll_src", 1, 2, 0, "clk_i2c2" },
	{ CLK_GATE_SPI0, "clk_spi0", "clk_fll_src", 1, 2, 0, "clk_spi0" },
	{ CLK_FAC_180M, "clkfac_180m", "clk_spll", 1, 8, 0, "clkfac_180m" },
	{ CLK_GATE_IOMCU_PERI0, "clk_gate_peri0", "clk_spll", 1, 1, 0, "clk_gate_peri0" },
	{ CLK_GATE_SPI2, "clk_spi2", "clk_fll_src", 1, 2, 0, "clk_spi2" },
	{ CLK_GATE_USB3OTG_REF, "clk_usb3otg_ref", "clk_sys_ini", 1, 1, 0, "clk_usb3otg_ref" },
	{ CLK_USB3_32K_DIV, "clkdiv_usb3_32k", "clkin_ref", 1, 586, 0, "clkdiv_usb3_32k" },
	{ CLK_GATE_SNP_USB3_SUSPEND, "snp_usb3_32k", "clkin_ref", 1, 1, 0, "snp_usb3_32k" },
	{ ACLK_GATE_SNP_USB3, "aclk_snp_usb3", "clkmux_usb_dock", 1, 1, 0, "aclk_snp_usb3" },
	{ CLK_GATE_SNP_USB3CTRL_REF, "snp_usb3crtl", "clkdiv_usb2phy_ref", 1, 1, 0, "snp_usb3crtl" },
	{ PCLK_GATE_USB_SCTRL, "pclk_usb_sctrl", "clk_div_hsdt1bus", 1, 1, 0, "pclk_usb_sctrl" },
	{ CLK_FACTOR_USB3PHY_PLL, "clkfac_usb3phy", "clk_spll", 1, 60, 0, "clkfac_usb3phy" },
	{ CLK_ANDGT_USB2PHY_REF, "clkgt_usb2phy_ref", "clk_abb_192", 1, 1, 0, "clkgt_usb2phy_ref" },
	{ CLK_USB2PHY_REF_DIV, "clkdiv_usb2phy_ref", "clkin_sys", 1, 2, 0, "clkdiv_usb2phy_ref" },
	{ CLK_MUX_ULPI, "sel_ulpi_ref", "clk_abb_192", 1, 1, 0, "sel_ulpi_ref" },
	{ CLK_GATE_ULPI_REF, "clk_ulpi_ref", "clk_abb_192", 1, 1, 0, "clk_ulpi_ref" },
	{ CLK_HSDT1_EUSB_MUX, "sel_hsdt1_eusb", "clk_sys_ini", 1, 1, 0, "sel_hsdt1_eusb" },
	{ CLK_GATE_HSDT1_EUSB, "clk_hsdt1_eusb", "clk_sys_ini", 1, 1, 0, "clk_hsdt1_eusb" },
	{ CLK_GATE_ABB_USB, "clk_abb_usb", "clk_abb_192", 1, 1, 0, "clk_abb_usb" },
	{ CLK_PCIE1PLL_SERDES, "clk_pcie1pll_serdes", "clk_abb_192", 1, 1, 0, "clk_pcie1pll_serdes" },
	{ CLK_HSDT_SUBSYS_INI, "clk_hsdt_subsys_ini", "clk_spll", 1, 7, 0, "clk_hsdt_subsys_ini" },
	{ PCLK_GATE_PCIE_SYS, "pclk_pcie_sys", "clk_hsdt_subsys_ini", 1, 1, 0, "pclk_pcie_sys" },
	{ PCLK_GATE_PCIE1_SYS, "pclk_pcie1_sys", "clk_hsdt_subsys_ini", 1, 1, 0, "pclk_pcie1_sys" },
	{ PCLK_GATE_PCIE_PHY, "pclk_pcie_phy", "clk_hsdt_subsys_ini", 1, 1, 0, "pclk_pcie_phy" },
	{ PCLK_GATE_PCIE1_PHY, "pclk_pcie1_phy", "clk_hsdt_subsys_ini", 1, 1, 0, "pclk_pcie1_phy" },
	{ PCLK_GATE_HSDT0_PCIE, "pclk_hsdt0_pcie", "pclk_pcie_div", 1, 1, 0, "pclk_hsdt0_pcie" },
	{ PCLK_GATE_HSDT1_PCIE1, "pclk_hsdt1_pcie1", "pclk_pcie_div", 1, 1, 0, "pclk_hsdt1_pcie1" },
	{ PCLK_DIV_PCIE, "pclk_pcie_div", "pclk_pcie_andgt", 1, 1, 0, "pclk_pcie_div" },
	{ PCLK_ANDGT_PCIE, "pclk_pcie_andgt", "sc_div_320m", 1, 1, 0, "pclk_pcie_andgt" },
	{ CLK_GATE_PCIEAUX, "clk_pcie0_aux", "clk_hsdt0_pcie_aux", 1, 1, 0, "clk_pcie0_aux" },
	{ ACLK_GATE_PCIE, "aclk_pcie", "clk_hsdt_subsys_ini", 1, 1, 0, "aclk_pcie" },
	{ CLK_GATE_HSDT_TBU, "clk_hsdt_tbu", "clk_hsdt_subsys_ini", 1, 1, 0, "clk_hsdt_tbu" },
	{ CLK_GATE_PCIE1_TBU, "clk_pcie1_tbu", "clk_hsdt_subsys_ini", 1, 1, 0, "clk_pcie1_tbu" },
	{ CLK_GATE_PCIE1AUX, "clk_pcie1_aux", "clk_hsdt0_pcie_aux", 1, 1, 0, "clk_pcie1_aux" },
	{ ACLK_GATE_PCIE1, "aclk_pcie1", "clk_hsdt_subsys_ini", 1, 1, 0, "aclk_pcie1" },
	{ CLK_ANDGT_HSDT0PRE, "gt_clk_hsdt0pre", "clkin_sys", 1, 1, 0, "gt_clk_hsdt0pre" },
	{ DIV_HSDT0BUS_PRE, "hsdt0bus_div_pre", "gt_clk_hsdt0pre", 1, 4, 0, "hsdt0bus_div_pre" },
	{ CLK_GATE_HSDT0BUS_ROOT, "clk_hsdt0_bus_root", "hsdt0bus_div_pre", 1, 1, 0, "clk_hsdt0_bus_root" },
	{ CLKINDIVSYS_INI, "clkinsys_ini_tmp", "clkin_sys", 1, 2, 0, "clkinsys_ini_tmp" },
	{ CLK_GATE_UFSPHY_REF, "clk_ufsphy_ref", "clkin_sys", 1, 1, 0, "clk_ufsphy_ref" },
	{ CLK_GATE_UFSIO_REF, "clk_ufsio_ref", "clkin_sys", 1, 1, 0, "clk_ufsio_ref" },
	{ PCLK_GATE_FER, "pclk_fer", "clk_ptp_div", 1, 1, 0, "pclk_fer" },
	{ CLK_GATE_SFC_BUS, "clk_sfc_bus", "div_clk_sfc2x", 1, 1, 0, "clk_sfc_bus" },
	{ CLK_GATE_SYSBUS2SFC, "clk_sys_bus2sfc", "div_clk_sfc2x", 1, 1, 0, "clk_sys_bus2sfc" },
	{ CLK_GATE_BLPWM, "clk_blpwm", "clk_fll_src", 1, 2, 0, "clk_blpwm" },
	{ CLK_GATE_GPS_REF, "clk_gps_ref", "clk_sys_ini", 1, 1, 0, "clk_gps_ref" },
	{ CLK_MUX_GPS_REF, "clkmux_gps_ref", "clk_sys_ini", 1, 1, 0, "clkmux_gps_ref" },
	{ CLK_GATE_MDM2GPS0, "clk_mdm2gps0", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps0" },
	{ CLK_GATE_MDM2GPS1, "clk_mdm2gps1", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps1" },
	{ CLK_GATE_MDM2GPS2, "clk_mdm2gps2", "clk_sys_ini", 1, 1, 0, "clk_mdm2gps2" },
	{ CLK_GATE_LDI1, "clk_ldi1", "clk_sys_ini", 1, 1, 0, "clk_ldi1" },
	{ EPS_VOLT_HIGH, "eps_volt_high", "peri_volt_hold", 1, 1, 0, "eps_volt_high" },
	{ EPS_VOLT_MIDDLE, "eps_volt_middle", "peri_volt_middle", 1, 1, 0, "eps_volt_middle" },
	{ EPS_VOLT_LOW, "eps_volt_low", "peri_volt_low", 1, 1, 0, "eps_volt_low" },
	{ VENC_VOLT_HOLD, "venc_volt_hold", "peri_volt_hold", 1, 1, 0, "venc_volt_hold" },
	{ VDEC_VOLT_HOLD, "vdec_volt_hold", "peri_volt_hold", 1, 1, 0, "vdec_volt_hold" },
	{ EDC_VOLT_HOLD, "edc_volt_hold", "peri_volt_hold", 1, 1, 0, "edc_volt_hold" },
	{ EFUSE_VOLT_HOLD, "efuse_volt_hold", "peri_volt_middle", 1, 1, 0, "efuse_volt_hold" },
	{ LDI0_VOLT_HOLD, "ldi0_volt_hold", "peri_volt_hold", 1, 1, 0, "ldi0_volt_hold" },
	{ SEPLAT_VOLT_HOLD, "seplat_volt_hold", "peri_volt_hold", 1, 1, 0, "seplat_volt_hold" },
	{ PCLK_GATE_DPCTRL, "pclk_dpctrl", "clkdiv_usb2phy_ref", 1, 1, 0, "pclk_dpctrl" },
	{ ACLK_GATE_DPCTRL, "aclk_dpctrl", "clk_sys_ini", 1, 1, 0, "aclk_dpctrl" },
	{ CLK_GATE_ISPFUNC3FREQ, "clk_ispfunc3freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc3freq" },
	{ CLK_GATE_ISPFUNC4FREQ, "clk_ispfunc4freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc4freq" },
	{ CLK_GATE_ISPFUNC5FREQ, "clk_ispfunc5freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc5freq" },
	{ CLK_GATE_ISPFUNC3, "clk_ispfunc3", "clk_ispfunc3freq", 1, 1, 0, "clk_ispfunc3" },
	{ CLK_GATE_ISPFUNC4, "clk_ispfunc4", "clk_ispfunc4freq", 1, 1, 0, "clk_ispfunc4" },
	{ CLK_GATE_ISPFUNC5, "clk_ispfunc5", "clk_ispfunc5freq", 1, 1, 0, "clk_ispfunc5" },
	{ CLK_ISP_SNCLK_FAC, "clk_fac_ispsn", "clk_ispsn_gt", 1, 14, 0, "clk_fac_ispsn" },
	{ CLK_DIV_RXDPHY_CFG, "div_rxdphy_cfg", "clkgt_rxdphy_cfg", 1, 1, 0, "div_rxdphy_cfg" },
	{ CLKANDGT_RXDPHY_CFG, "clkgt_rxdphy_cfg", "clk_rxdphy_mux", 1, 1, 0, "clkgt_rxdphy_cfg" },
	{ CLK_GATE_TXDPHY1_CFG, "clk_txdphy1_cfg", "clk_sys_ini", 1, 1, 0, "clk_txdphy1_cfg" },
	{ CLK_GATE_TXDPHY1_REF, "clk_txdphy1_ref", "clk_sys_ini", 1, 1, 0, "clk_txdphy1_ref" },
	{ CLK_MEDIA_COMMON_SW, "sel_media_common_pll", "clk_sys_ini", 1, 1, 0, "sel_media_common_pll" },
	{ CLKANDGT_MEDIA_COMMON, "clkgt_media_common", "sel_media_common_pll", 1, 1, 0, "clkgt_media_common" },
	{ CLK_MEDIA_COMMON_DIV, "clkdiv_media_common", "clk_sys_ini", 1, 1, 0, "clkdiv_media_common" },
	{ CLK_GATE_MEDIA_COMMONFREQ, "clk_media_commonfreq", "clk_spll", 1, 16, 0, "clk_media_commonfreq" },
	{ CLK_GATE_MEDIA_COMMON, "clk_media_common", "clk_media_commonfreq", 1, 16, 0, "clk_media_common" },
	{ CLK_GATE_BRG, "clk_brg", "clk_spll", 1, 16, 0, "clk_brg" },
	{ CLK_SW_AO_LOADMONITOR, "clk_ao_loadmonitor_sw", "clk_fll_src", 1, 1, 0, "clk_ao_loadmonitor_sw" },
	{ CLK_GATE_IVP32DSP_COREFREQ, "clk_ivpdsp_corefreq", "clk_sys_ini", 1, 1, 0, "clk_ivpdsp_corefreq" },
	{ CLK_GATE_IVP32DSP_CORE, "clk_ivpdsp_core", "clk_ivpdsp_corefreq", 1, 1, 0, "clk_ivpdsp_core" },
	{ CLK_GATE_IVP1DSP_COREFREQ, "clk_ivp1dsp_corefreq", "clk_sys_ini", 1, 1, 0, "clk_ivp1dsp_corefreq" },
	{ CLK_GATE_IVP1DSP_CORE, "clk_ivp1dsp_core", "clk_ivp1dsp_corefreq", 1, 1, 0, "clk_ivp1dsp_core" },
	{ CLK_GATE_ARPPFREQ, "clk_arpp_topfreq", "clk_sys_ini", 1, 1, 0, "clk_arpp_topfreq" },
	{ CLK_GATE_ARPP, "clk_arpp_top", "clk_arpp_topfreq", 1, 1, 0, "clk_arpp_top" },
	{ CLK_ADE_FUNC_MUX, "clkmux_ade_func", "clk_sys_ini", 1, 1, 0, "clkmux_ade_func" },
	{ CLK_ANDGT_ADE_FUNC, "clkgt_ade_func", "clk_sys_ini", 1, 1, 0, "clkgt_ade_func" },
	{ CLK_DIV_ADE_FUNC, "clk_ade_func_div", "clk_sys_ini", 1, 1, 0, "clk_ade_func_div" },
	{ CLK_GATE_ADE_FUNCFREQ, "clk_ade_funcfreq", "clk_spll", 1, 16, 0, "clk_ade_funcfreq" },
	{ CLK_GATE_ADE_FUNC, "clk_ade_func", "clk_ade_funcfreq", 1, 16, 0, "clk_ade_func" },
	{ ACLK_GATE_ASC, "aclk_asc", "clk_mmbuf", 1, 1, 0, "aclk_asc" },
	{ CLK_GATE_DSS_AXI_MM, "clk_dss_axi_mm", "clk_spll", 1, 3, 0, "clk_dss_axi_mm" },
	{ CLK_GATE_MMBUF, "clk_mmbuf", "clk_spll", 1, 3, 0, "clk_mmbuf" },
	{ PCLK_GATE_MMBUF, "pclk_mmbuf", "clk_spll", 1, 3, 0, "pclk_mmbuf" },
	{ CLK_SW_MMBUF, "aclk_mmbuf_sw", "clk_sys_ini", 1, 1, 0, "aclk_mmbuf_sw" },
	{ ACLK_DIV_MMBUF, "aclk_mmbuf_div", "clk_sys_ini", 1, 1, 0, "aclk_mmbuf_div" },
	{ PCLK_DIV_MMBUF, "pclk_mmbuf_div", "clk_sys_ini", 1, 1, 0, "pclk_mmbuf_div" },
	{ AUTODIV_ISP_DVFS, "autodiv_isp_dvfs", "autodiv_sysbus", 1, 1, 0, "autodiv_isp_dvfs" },
};

static const struct gate_clock crgctrl_gate_clks[] = {
	{ CLK_GATE_PPLL0_MEDIA, "clk_ppll0_media", "clk_spll", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_media" },
	{ CLK_GATE_PPLL1_MEDIA, "clk_ppll1_media", "clk_ap_ppll1", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll1_media" },
	{ CLK_GATE_PPLL2_MEDIA, "clk_ppll2_media", "clk_ap_ppll2", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_media" },
	{ CLK_GATE_PPLL2B_MEDIA, "clk_ppll2b_media", "clk_ap_ppll2_b", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2b_media" },
	{ CLK_GATE_PPLL7_MEDIA, "clk_ppll7_media", "clk_ap_ppll7", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll7_media" },
	{ CLK_GATE_PPLL7_MEDIA2, "clk_ppll7_media2", "clk_ap_ppll7", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll7_media2" },
	{ CLK_GATE_PPLL0_M2, "clk_ppll0_m2", "clk_spll", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll0_m2" },
	{ CLK_GATE_PPLL1_M2, "clk_ppll1_m2", "clk_ap_ppll1", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll1_m2" },
	{ CLK_GATE_PPLL2_M2, "clk_ppll2_m2", "clk_ap_ppll2", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2_m2" },
	{ CLK_GATE_PPLL2B_M2, "clk_ppll2b_m2", "clk_ap_ppll2_b", 0, 0x0, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ppll2b_m2" },
	{ PCLK_GPIO0, "pclk_gpio0", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio0" },
	{ PCLK_GPIO1, "pclk_gpio1", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio1" },
	{ PCLK_GPIO2, "pclk_gpio2", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio2" },
	{ PCLK_GPIO3, "pclk_gpio3", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio3" },
	{ PCLK_GPIO4, "pclk_gpio4", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio4" },
	{ PCLK_GPIO5, "pclk_gpio5", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio5" },
	{ PCLK_GPIO6, "pclk_gpio6", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio6" },
	{ PCLK_GPIO7, "pclk_gpio7", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio7" },
	{ PCLK_GPIO8, "pclk_gpio8", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio8" },
	{ PCLK_GPIO9, "pclk_gpio9", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio9" },
	{ PCLK_GPIO10, "pclk_gpio10", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio10" },
	{ PCLK_GPIO11, "pclk_gpio11", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio11" },
	{ PCLK_GPIO12, "pclk_gpio12", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio12" },
	{ PCLK_GPIO13, "pclk_gpio13", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio13" },
	{ PCLK_GPIO14, "pclk_gpio14", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio14" },
	{ PCLK_GPIO15, "pclk_gpio15", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio15" },
	{ PCLK_GPIO16, "pclk_gpio16", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio16" },
	{ PCLK_GPIO17, "pclk_gpio17", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio17" },
	{ PCLK_GPIO18, "pclk_gpio18", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio18" },
	{ PCLK_GPIO19, "pclk_gpio19", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio19" },
	{ PCLK_GATE_WD0_HIGH, "pclk_wd0_high", "div_sysbus_pll", 0x20, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0_high" },
	{ PCLK_GATE_WD0, "pclk_wd0", "clkin_ref", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd0" },
	{ PCLK_GATE_WD1, "pclk_wd1", "div_sysbus_pll", 0x20, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_wd1" },
	{ CLK_GATE_CODECSSI, "clk_codecssi", "sel_codeccssi", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_codecssi" },
	{ PCLK_GATE_CODECSSI, "pclk_codecssi", "div_sysbus_pll", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_codecssi" },
	{ PCLK_GATE_IOC, "pclk_ioc", "clk_sys_ini", 0x20, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ioc" },
	{ CLK_ATDVFS, "clk_atdvfs", "clk_cssys_div", 0, 0x0, 0,
		NULL, 1, { 0, 0, 0 }, { 0, 1, 2, 3 }, 3, 19, 0, "clk_atdvfs" },
	{ CLK_GATE_TIME_STAMP, "clk_time_stamp", "clk_timestp_div", 0x00, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_time_stamp" },
	{ ACLK_GATE_PERF_STAT, "aclk_perf_stat", "div_sysbus_pll", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_perf_stat" },
	{ PCLK_GATE_PERF_STAT, "pclk_perf_stat", "div_sysbus_pll", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_perf_stat" },
	{ CLK_GATE_PERF_STAT, "clk_perf_stat", "clk_perf_div", 0x040, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_stat" },
	{ CLK_GATE_PERF_CTRL, "clk_perf_ctrl", "sc_div_320m", 0x040, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_perf_ctrl" },
	{ CLK_GATE_SD_PERI, "clk_sd_peri", "clk_div_sd_peri", 0x020, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sd_peri" },
	{ CLK_GATE_UART4, "clk_uart4", "clkmux_uarth", 0x20, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart4" },
	{ PCLK_GATE_UART4, "pclk_uart4", "clkmux_uarth", 0x20, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart4" },
	{ CLK_GATE_UART5, "clk_uart5", "clkmux_uartl", 0x20, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart5" },
	{ PCLK_GATE_UART5, "pclk_uart5", "clkmux_uartl", 0x20, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart5" },
	{ CLK_GATE_UART0, "clk_uart0", "clkmux_uart0", 0x20, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart0" },
	{ CLK_GATE_UART8, "clk_uart8", "clkmux_uart8", 0x40, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_uart8" },
	{ PCLK_GATE_UART8, "pclk_uart8", "clkmux_uart8", 0x40, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_uart8" },
	{ CLK_GATE_I2C2_ACPU, "clk_i2c2_acpu", "clkmux_i2c", 0x20, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c2_acpu" },
	{ CLK_GATE_I2C3, "clk_i2c3", "clkmux_i2c", 0x20, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c3" },
	{ CLK_GATE_I2C4, "clk_i2c4", "clkmux_i2c", 0x20, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c4" },
	{ CLK_GATE_I2C6_ACPU, "clk_i2c6_acpu", "clkmux_i2c", 0x10, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c6_acpu" },
	{ CLK_GATE_I2C7, "clk_i2c7", "clkmux_i2c", 0x10, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c7" },
	{ PCLK_GATE_I2C2, "pclk_i2c2", "clkmux_i2c", 0x20, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c2" },
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
	{ CLK_GATE_SPI1, "clk_spi1", "clkmux_spi1", 0x20, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi1" },
	{ CLK_GATE_SPI4, "clk_spi4", "clkmux_spi4", 0x40, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi4" },
	{ PCLK_GATE_SPI1, "pclk_spi1", "clkmux_spi1", 0x20, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi1" },
	{ PCLK_GATE_SPI4, "pclk_spi4", "clkmux_spi4", 0x40, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi4" },
	{ CLK_GATE_SPI5, "clk_spi5", "clkmux_spi5", 0x040, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi5" },
	{ PCLK_GATE_SPI5, "pclk_spi5", "clkmux_spi5", 0x040, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi5" },
	{ CLK_GATE_SPI6, "clk_spi6", "clkmux_spi6", 0x040, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi6" },
	{ PCLK_GATE_SPI6, "pclk_spi6", "clkmux_spi6", 0x040, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi6" },
	{ CLK_GATE_HSDT0_PCIEAUX, "clk_hsdt0_pcie_aux", "clkmux_pcieaux", 0x010, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt0_pcie_aux" },
	{ CLK_GATE_PERI2HSDT0, "clk_peri2hsdt0", "clk_div_peri2hsdt0", 0x420, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_peri2hsdt0" },
	{ CLK_GATE_XGE_480, "clk_xge_480", "clkdiv_xge_480", 0x420, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge_480" },
	{ CLK_GATE_XGE_360, "clk_xge_360", "clkdiv_xge_360", 0x420, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge_360" },
	{ CLK_GATE_AO_ASP, "clk_ao_asp", "clk_ao_asp_div", 0x0, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_asp" },
	{ PCLK_GATE_PCTRL, "pclk_pctrl", "div_sysbus_pll", 0x20, 0x80000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_pctrl" },
	{ CLK_GATE_BLPWM5, "pclk_blpwm5", "clk_ptp_div", 0x30, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_blpwm5" },
	{ CLK_GATE_BLPWM4, "pclk_blpwm4", "clk_ptp_div", 0x30, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_blpwm4" },
	{ CLK_GATE_BLPWM3, "pclk_blpwm3", "clk_ptp_div", 0x30, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_blpwm3" },
	{ CLK_GATE_SFC_2X, "clk_sfc_x2", "div_clk_sfc2x", 0x40, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sfc_x2" },
	{ CLK_GATE_BLPWM1, "clk_blpwm1", "clk_ptp_div", 0x20, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_blpwm1" },
	{ PERI_VOLT_HOLD, "peri_volt_hold", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0,0,0}, {0,1,2,3}, 3, 18, 0, "peri_volt_hold" },
	{ PERI_VOLT_MIDDLE, "peri_volt_middle", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0,0}, {0,1,2}, 2, 22, 0, "peri_volt_middle" },
	{ PERI_VOLT_LOW, "peri_volt_low", "clk_sys_ini", 0, 0x0, 0,
		NULL, 1, {0}, {0,1}, 1, 28, 0, "peri_volt_low" },
	{ CLK_GATE_DPCTRL_16M, "clk_dpctrl_16m", "div_dpctrl16m", 0x470, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dpctrl_16m" },
	{ CLK_GATE_DPCTRL_PIXEL, "dpctrl_pixel", "div_dpctr_pixel", 0x470, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "dpctrl_pixel" },
	{ CLK_GATE_ISP_I2C_MEDIA, "clk_isp_i2c_media", "clk_div_isp_i2c", 0x30, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i2c_media" },
	{ CLK_GATE_ISP_SNCLK0, "clk_isp_snclk0", "clk_mux_ispsn0", 0x50, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk0" },
	{ CLK_GATE_ISP_SNCLK1, "clk_isp_snclk1", "clk_mux_ispsn1", 0x50, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk1" },
	{ CLK_GATE_ISP_SNCLK2, "clk_isp_snclk2", "clk_mux_ispsn2", 0x50, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk2" },
	{ CLK_GATE_ISP_SNCLK3, "clk_isp_snclk3", "clk_mux_ispsn3", 0x40, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_snclk3" },
	{ CLK_GATE_RXDPHY0_CFG, "clk_rxdphy0_cfg", "clk_rxdphy_cfg", 0x050, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy0_cfg" },
	{ CLK_GATE_RXDPHY1_CFG, "clk_rxdphy1_cfg", "clk_rxdphy_cfg", 0x050, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy1_cfg" },
	{ CLK_GATE_RXDPHY2_CFG, "clk_rxdphy2_cfg", "clk_rxdphy_cfg", 0x050, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy2_cfg" },
	{ CLK_GATE_RXDPHY3_CFG, "clk_rxdphy3_cfg", "clk_rxdphy_cfg", 0x050, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy3_cfg" },
	{ CLK_GATE_RXDPHY4_CFG, "clk_rxdphy4_cfg", "clk_rxdphy_cfg", 0x050, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy4_cfg" },
	{ CLK_GATE_RXDPHY5_CFG, "clk_rxdphy5_cfg", "clk_rxdphy_cfg", 0x050, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy5_cfg" },
	{ CLK_GATE_RXDPHY_CFG, "clk_rxdphy_cfg", "clk_rxdphy_mux", 0x050, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_rxdphy_cfg" },
	{ CLK_GATE_TXDPHY0_CFG, "clk_txdphy0_cfg", "clk_sys_ini", 0x030, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_cfg" },
	{ CLK_GATE_TXDPHY0_REF, "clk_txdphy0_ref", "clk_sys_ini", 0x030, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_txdphy0_ref" },
	{ PCLK_GATE_LOADMONITOR, "pclk_loadmonitor", "div_sysbus_pll", 0x20, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor" },
	{ CLK_GATE_LOADMONITOR, "clk_loadmonitor", "clk_loadmonitor_div", 0x20, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor" },
	{ PCLK_GATE_LOADMONITOR_L, "pclk_loadmonitor_l", "div_sysbus_pll", 0x20, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_loadmonitor_l" },
	{ CLK_GATE_LOADMONITOR_L, "clk_loadmonitor_l", "clk_loadmonitor_div", 0x20, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_loadmonitor_l" },
	{ CLK_GATE_MEDIA_TCXO, "clk_media_tcxo", "clk_sys_ini", 0x40, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_media_tcxo" },
	{ AUTODIV_ISP, "autodiv_isp", "autodiv_isp_dvfs", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "autodiv_isp" },
	{ CLK_GATE_ATDIV_HSDT1BUS, "clk_atdiv_hsdt1bus", "clk_div_hsdt1bus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_hsdt1bus" },
	{ CLK_GATE_ATDIV_DMA, "clk_atdiv_dma", "clk_dmabus_div", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_dma" },
	{ CLK_GATE_ATDIV_CFG, "clk_atdiv_cfg", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_cfg" },
	{ CLK_GATE_ATDIV_SYS, "clk_atdiv_sys", "div_sysbus_pll", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_sys" },
};

static const struct fsm_pll_clock fsm_pll_clks[] = {
	{ CLK_GATE_PPLL1, "clk_ap_ppll1", "clk_ppll1", 0x1,
		{ 0x950, 0 }, { 0x958, 0 }, "clk_ap_ppll1" },
	{ CLK_GATE_PPLL2, "clk_ap_ppll2", "clk_ppll2", 0x2,
		{ 0x950, 5 }, { 0x958, 1 },  "clk_ap_ppll2" },
	{ CLK_GATE_PPLL2_B, "clk_ap_ppll2_b", "clk_ppll2_b", 0x9,
		{ 0x954, 5 }, { 0x958, 2 }, "clk_ap_ppll2_b" },
	{ CLK_GATE_PPLL3, "clk_ap_ppll3", "clk_ppll3", 0x3,
		{ 0x950, 10 }, { 0x958, 3 }, "clk_ap_ppll3" },
	{ CLK_GATE_PPLL7, "clk_ap_ppll7", "clk_ppll7", 0x7,
		{ 0x954, 0 }, { 0x958, 4 }, "clk_ap_ppll7" },
};

static const struct fsm_pll_clock aocrg_fsm_pll_clks[] = {
	{ CLK_GATE_AUPLL, "clk_ap_aupll", "clk_aupll", 0xC,
		{ 0x1D0, 0 }, { 0x1EC, 10 }, "clk_ap_aupll" },
};

static const struct scgt_clock crgctrl_scgt_clks[] = {
	{ CLK_GATE_TIME_STAMP_GT, "clk_timestp_gt", "clk_sys_ini",
		0xF0, 1, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_timestp_gt" },
	{ CLK_PERF_DIV_GT, "clk_perf_gt", "sc_div_320m",
		0xF0, 3, CLK_GATE_HIWORD_MASK, "clk_perf_gt" },
	{ CLK_A53HPM_ANDGT, "clk_a53hpm_gt", "clk_a53hpm_mux",
		0x0F4, 7, CLK_GATE_HIWORD_MASK, "clk_a53hpm_gt" },
	{ CLK_320M_PLL_GT, "gt_clk_320m_pll", "clk_spll",
		0xF8, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "gt_clk_320m_pll" },
	{ CLK_ANDGT_UARTH, "clkgt_uarth", "sc_div_320m",
		0xF4, 11, CLK_GATE_HIWORD_MASK, "clkgt_uarth" },
	{ CLK_ANDGT_UARTL, "clkgt_uartl", "sc_div_320m",
		0xF4, 10, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uartl" },
	{ CLK_ANDGT_UART0, "clkgt_uart0", "sc_div_320m",
		0xF4, 9, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uart0" },
	{ CLK_ANDGT_UART8, "clkgt_uart8", "sc_div_320m",
		0xD0, 4, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clkgt_uart8" },
	{ CLK_ANDGT_I3C4, "clkgt_i3c4", "sc_div_320m",
		0x70C, 5, CLK_GATE_HIWORD_MASK, "clkgt_i3c4" },
	{ CLK_ANDGT_SPI1, "clkgt_spi1", "sc_div_320m",
		0xF4, 13, CLK_GATE_HIWORD_MASK, "clkgt_spi1" },
	{ CLK_ANDGT_SPI4, "clkgt_spi4", "sc_div_320m",
		0xF4, 12, CLK_GATE_HIWORD_MASK, "clkgt_spi4" },
	{ CLK_ANDGT_SPI5, "clkgt_spi5", "clk_spll",
		0x0CC, 4, CLK_GATE_HIWORD_MASK, "clkgt_spi5" },
	{ CLK_ANDGT_SPI6, "clkgt_spi6", "clk_spll",
		0x0CC, 9, CLK_GATE_HIWORD_MASK, "clkgt_spi6" },
	{ CLK_ANDGT_32KPLL_PCIEAUX, "clkgt_32kpll_pcieaux", "clk_fll_src",
		0x0fc, 6, CLK_GATE_HIWORD_MASK, "clkgt_32kpll_pcieaux" },
	{ CLK_ANDGT_PERI2HSDT0, "gt_clk_peri2hsdt0", "clk_mux_peri2hsdt0",
		0x0BC, 14, CLK_GATE_HIWORD_MASK, "gt_clk_peri2hsdt0" },
	{ CLK_ANDGT_XGE480, "clkgt_xge_480", "clk_ap_ppll1",
		0x0BC, 4, CLK_GATE_HIWORD_MASK, "clkgt_xge_480" },
	{ CLK_ANDGT_XGE360, "clkgt_xge_360", "clk_ap_ppll1",
		0x0BC, 9, CLK_GATE_HIWORD_MASK, "clkgt_xge_360" },
	{ CLK_DIV_AO_ASP_GT, "clk_ao_asp_gt", "clkmux_ao_asp",
		0xF4, 4, CLK_GATE_HIWORD_MASK, "clk_ao_asp_gt" },
	{ CLK_ANDGT_PTP, "clk_ptp_gt", "sc_div_320m",
		0xF8, 5, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_ptp_gt" },
	{ CLK_GT_SFC_2X, "clk_andgt_sfc2x", "clk_ptp_div",
		0x0D0, 9, CLK_GATE_HIWORD_MASK, "clk_andgt_sfc2x" },
	{ CLK_GT_DPCTRL_16M, "clk_gt_16m_dp", "clk_mux_16m_dp",
		0x0FC, 12, CLK_GATE_HIWORD_MASK, "clk_gt_16m_dp" },
	{ CLK_ANDGT_DPCTR_PIXEL, "gt_dpctr_pixel", "mux_dpctr_pixel",
		0xF0, 0, CLK_GATE_HIWORD_MASK, "gt_dpctr_pixel" },
	{ CLK_GT_ISP_I2C, "clk_gt_isp_i2c", "sc_div_320m",
		0xF8, 4, CLK_GATE_HIWORD_MASK, "clk_gt_isp_i2c" },
	{ CLK_ISP_SNCLK_ANGT, "clk_ispsn_gt", "sc_div_320m",
		0x108, 2, CLK_GATE_HIWORD_MASK, "clk_ispsn_gt" },
	{ CLKANDGT_RXDPHY_FLL, "clkgt_rxdphy_fll", "clk_fll_src",
		0xFC, 10, CLK_GATE_HIWORD_MASK, "clkgt_rxdphy_fll" },
	{ CLK_GT_LOADMONITOR, "clk_loadmonitor_gt", "sc_div_320m",
		0x0f0, 5, CLK_GATE_HIWORD_MASK, "clk_loadmonitor_gt" },
	{ AUTODIV_SYSBUS, "autodiv_sysbus", "div_sysbus_pll",
		0xF90, 5, CLK_GATE_HIWORD_MASK, "autodiv_sysbus" },
	{ AUTODIV_HSDT1BUS, "autodiv_hsdt1bus", "autodiv_sysbus",
		0xF90, 1, CLK_GATE_HIWORD_MASK, "autodiv_hsdt1bus" },
	{ AUTODIV_CFGBUS, "autodiv_cfgbus", "autodiv_sysbus",
		0xF90, 4, CLK_GATE_HIWORD_MASK, "autodiv_cfgbus" },
	{ AUTODIV_DMABUS, "autodiv_dmabus", "autodiv_sysbus",
		0xF90, 3, CLK_GATE_HIWORD_MASK, "autodiv_dmabus" },
};

static const struct div_clock crgctrl_div_clks[] = {
	{ PCLK_DIV_DBG, "pclkdiv_dbg", "clk_cssys_div",
		0xFF4, 0x7800, 4, 1, 0, "pclkdiv_dbg" },
	{ CLK_DIV_TIME_STAMP, "clk_timestp_div", "clk_timestp_gt",
		0x0B8, 0x70, 8, 1, 0, "clk_timestp_div" },
	{ CLK_DIV_PERF_STAT, "clk_perf_div", "clk_perf_gt",
		0x0D0, 0xf000, 16, 1, 0, "clk_perf_div" },
	{ CLK_DIV_SD_PERI, "clk_div_sd_peri", "clk_ap_ppll2",
		0x700, 0x1e0, 64, 1, 0, "clk_div_sd_peri" },
	{ CLK_DIV_A53HPM, "clk_a53hpm_div", "clk_a53hpm_gt",
		0x0E0, 0xfc00, 64, 1, 0, "clk_a53hpm_div" },
	{ CLK_DIV_UARTH, "clkdiv_uarth", "clkgt_uarth",
		0xB0, 0xf000, 16, 1, 0, "clkdiv_uarth" },
	{ CLK_DIV_UARTL, "clkdiv_uartl", "clkgt_uartl",
		0xB0, 0xf00, 16, 1, 0, "clkdiv_uartl" },
	{ CLK_DIV_UART0, "clkdiv_uart0", "clkgt_uart0",
		0xB0, 0xf0, 16, 1, 0, "clkdiv_uart0" },
	{ CLK_DIV_UART8, "clkdiv_uart8", "clkgt_uartl",
		0xD0, 0xf, 16, 1, 0, "clkdiv_uart8" },
	{ CLK_DIV_I2C, "clkdiv_i2c", "sc_div_320m",
		0xE8, 0xf0, 16, 1, 0, "clkdiv_i2c" },
	{ CLK_DIV_I3C4, "clkdiv_i3c4", "clkgt_i3c4",
		0x710, 0xc00, 4, 1, 0, "clkdiv_i3c4" },
	{ CLK_DIV_SPI1, "clkdiv_spi1", "clkgt_spi1",
		0xC4, 0xf000, 16, 1, 0, "clkdiv_spi1" },
	{ CLK_DIV_SPI4, "clkdiv_spi4", "clkgt_spi4",
		0xC4, 0xf00, 16, 1, 0, "clkdiv_spi4" },
	{ CLK_DIV_SPI5, "clkdiv_spi5", "clkgt_spi5",
		0x0CC, 0xf, 64, 1, 0, "clkdiv_spi5" },
	{ CLK_DIV_SPI6, "clkdiv_spi6", "clkgt_spi6",
		0x0CC, 0x1e0, 64, 1, 0, "clkdiv_spi6" },
	{ CLK_DIV_32KPLL_PCIEAUX, "div_32kpll_pcieaux", "clkgt_32kpll_pcieaux",
		0x0fc, 0x3f, 64, 1, 0, "div_32kpll_pcieaux" },
	{ DIV_PERI2HSDT0, "clk_div_peri2hsdt0", "gt_clk_peri2hsdt0",
		0x0BC, 0x3c00, 16, 1, 0, "clk_div_peri2hsdt0" },
	{ CLK_DIV_XGE_480, "clkdiv_xge_480", "clkgt_xge_480",
		0x0BC, 0xf, 16, 1, 0, "clkdiv_xge_480" },
	{ CLK_DIV_XGE_360, "clkdiv_xge_360", "clkgt_xge_360",
		0x0BC, 0x1e0, 16, 1, 0, "clkdiv_xge_360" },
	{ CLK_DIV_AO_ASP, "clk_ao_asp_div", "clk_ao_asp_gt",
		0x108, 0x3c0, 16, 1, 0, "clk_ao_asp_div" },
	{ CLK_DIV_PTP, "clk_ptp_div", "clk_ptp_gt",
		0xD8, 0xf, 16, 1, 0, "clk_ptp_div" },
	{ CLK_DIV_SFC2X, "div_clk_sfc2x", "clk_andgt_sfc2x",
		0x0D0, 0x1e0, 16, 1, 0, "div_clk_sfc2x" },
	{ CLK_DIV_DPCTRL_16M, "div_dpctrl16m", "clk_gt_16m_dp",
		0x0B4, 0xfc0, 64, 1, 0, "div_dpctrl16m" },
	{ CLK_DIV_DPCTR_PIXEL, "div_dpctr_pixel", "gt_dpctr_pixel",
		0xC0, 0x3f00, 64, 1, 0, "div_dpctr_pixel" },
	{ CLK_DIV_ISP_I2C, "clk_div_isp_i2c", "clk_gt_isp_i2c",
		0xB8, 0x780, 16, 1, 0, "clk_div_isp_i2c" },
	{ CLK_ISP_SNCLK_DIV0, "clk_div_ispsn0", "clk_fac_ispsn",
		0x108, 0x3, 4, 1, 0, "clk_div_ispsn0" },
	{ CLK_ISP_SNCLK_DIV1, "clk_div_ispsn1", "clk_fac_ispsn",
		0x10C, 0xc000, 4, 1, 0, "clk_div_ispsn1" },
	{ CLK_ISP_SNCLK_DIV2, "clk_div_ispsn2", "clk_fac_ispsn",
		0x10C, 0x1800, 4, 1, 0, "clk_div_ispsn2" },
	{ CLK_ISP_SNCLK_DIV3, "clk_div_ispsn3", "clk_fac_ispsn",
		0x100, 0x300, 4, 1, 0, "clk_div_ispsn3" },
	{ CLK_RXDPHY_FLL_DIV, "div_rxdphy_fll", "clkgt_rxdphy_fll",
		0x0DC, 0x3, 4, 1, 0, "div_rxdphy_fll" },
	{ CLK_DIV_LOADMONITOR, "clk_loadmonitor_div", "clk_loadmonitor_gt",
		0x0b8, 0xc000, 4, 1, 0, "clk_loadmonitor_div" },
};

static const char *const codeccssi_mux_p [] = { "clk_sys_ini", "clk_sys_ini" };
static const char *const clk_mux_a53hpm_p [] = { "clk_spll", "clk_ap_ppll1" };
static const char *const clk_mux_uarth_p [] = { "clk_sys_ini", "clkdiv_uarth" };
static const char *const clk_mux_uartl_p [] = { "clk_sys_ini", "clkdiv_uartl" };
static const char *const clk_mux_uart0_p [] = { "clk_sys_ini", "clkdiv_uart0" };
static const char *const clk_mux_uart8_p [] = { "clk_sys_ini", "clkdiv_uart8" };
static const char *const clk_mux_i2c_p [] = { "clk_sys_ini", "clkdiv_i2c" };
static const char *const clk_mux_spi1_p [] = { "clk_sys_ini", "clkdiv_spi1" };
static const char *const clk_mux_spi4_p [] = { "clk_sys_ini", "clkdiv_spi4" };
static const char *const clk_mux_spi5_p [] = { "clk_sys_ini", "clkdiv_spi5" };
static const char *const clk_mux_spi6_p [] = { "clk_sys_ini", "clkdiv_spi6" };
static const char *const clk_mux_pcieaux_p [] = { "clk_sys_ini", "div_32kpll_pcieaux" };
static const char *const sel_peri2hsdt0_p [] = { "clk_spll", "clk_ppll2_b" };
static const char *const clk_mux_ao_asp_p [] = { "clk_ap_ppll1", "clk_ap_ppll2_b", "clk_ap_ppll3", "clk_ap_ppll3" };
static const char *const clk_mux_dpctrl_16m_p [] = { "clk_fll_src", "clk_a53hpm_div" };
static const char *const clk_mux_dpctr_pixel_p [] = { "clk_spll", "clk_ap_ppll1", "clk_ap_ppll7", "clk_ap_ppll7" };
static const char *const clk_isp_snclk_mux0_p [] = { "clk_sys_ini", "clk_div_ispsn0" };
static const char *const clk_isp_snclk_mux1_p [] = { "clk_sys_ini", "clk_div_ispsn1" };
static const char *const clk_isp_snclk_mux2_p [] = { "clk_sys_ini", "clk_div_ispsn2" };
static const char *const clk_isp_snclk_mux3_p [] = { "clk_sys_ini", "clk_div_ispsn3" };
static const char *const clk_mux_rxdphy_cfg_p [] = { "clk_a53hpm_div", "clk_a53hpm_div",
						"clk_sys_ini", "div_rxdphy_fll" };
static const struct mux_clock crgctrl_mux_clks[] = {
	{ CODECCSSI_MUX, "sel_codeccssi", codeccssi_mux_p,
		ARRAY_SIZE(codeccssi_mux_p), 0x0AC, 0x80, CLK_MUX_HIWORD_MASK, "sel_codeccssi" },
	{ CLK_MUX_A53HPM, "clk_a53hpm_mux", clk_mux_a53hpm_p,
		ARRAY_SIZE(clk_mux_a53hpm_p), 0x0D4, 0x200, CLK_MUX_HIWORD_MASK, "clk_a53hpm_mux" },
	{ CLK_MUX_UARTH, "clkmux_uarth", clk_mux_uarth_p,
		ARRAY_SIZE(clk_mux_uarth_p), 0xAC, 0x10, CLK_MUX_HIWORD_MASK, "clkmux_uarth" },
	{ CLK_MUX_UARTL, "clkmux_uartl", clk_mux_uartl_p,
		ARRAY_SIZE(clk_mux_uartl_p), 0xAC, 0x8, CLK_MUX_HIWORD_MASK, "clkmux_uartl" },
	{ CLK_MUX_UART0, "clkmux_uart0", clk_mux_uart0_p,
		ARRAY_SIZE(clk_mux_uart0_p), 0xAC, 0x4, CLK_MUX_HIWORD_MASK, "clkmux_uart0" },
	{ CLK_MUX_UART8, "clkmux_uart8", clk_mux_uart8_p,
		ARRAY_SIZE(clk_mux_uart8_p), 0xAC, 0x800, CLK_MUX_HIWORD_MASK, "clkmux_uart8" },
	{ CLK_MUX_I2C, "clkmux_i2c", clk_mux_i2c_p,
		ARRAY_SIZE(clk_mux_i2c_p), 0xAC, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_i2c" },
	{ CLK_MUX_SPI1, "clkmux_spi1", clk_mux_spi1_p,
		ARRAY_SIZE(clk_mux_spi1_p), 0xAC, 0x100, CLK_MUX_HIWORD_MASK, "clkmux_spi1" },
	{ CLK_MUX_SPI4, "clkmux_spi4", clk_mux_spi4_p,
		ARRAY_SIZE(clk_mux_spi4_p), 0xAC, 0x40, CLK_MUX_HIWORD_MASK, "clkmux_spi4" },
	{ CLK_MUX_SPI5, "clkmux_spi5", clk_mux_spi5_p,
		ARRAY_SIZE(clk_mux_spi5_p), 0x0AC, 0x200, CLK_MUX_HIWORD_MASK, "clkmux_spi5" },
	{ CLK_MUX_SPI6, "clkmux_spi6", clk_mux_spi6_p,
		ARRAY_SIZE(clk_mux_spi6_p), 0x0AC, 0x400, CLK_MUX_HIWORD_MASK, "clkmux_spi6" },
	{ CLK_MUX_PCIEAUX, "clkmux_pcieaux", clk_mux_pcieaux_p,
		ARRAY_SIZE(clk_mux_pcieaux_p), 0x0d8, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_pcieaux" },
	{ SEL_PERI2HSDT0, "clk_mux_peri2hsdt0", sel_peri2hsdt0_p,
		ARRAY_SIZE(sel_peri2hsdt0_p), 0x0BC, 0x8000, CLK_MUX_HIWORD_MASK, "clk_mux_peri2hsdt0" },
	{ CLK_MUX_AO_ASP, "clkmux_ao_asp", clk_mux_ao_asp_p,
		ARRAY_SIZE(clk_mux_ao_asp_p), 0x100, 0x30, CLK_MUX_HIWORD_MASK, "clkmux_ao_asp" },
	{ CLK_MUX_DPCTRL_16M, "clk_mux_16m_dp", clk_mux_dpctrl_16m_p,
		ARRAY_SIZE(clk_mux_dpctrl_16m_p), 0x0B4, 0x1000, CLK_MUX_HIWORD_MASK, "clk_mux_16m_dp" },
	{ CLK_MUX_DPCTR_PIXEL, "mux_dpctr_pixel", clk_mux_dpctr_pixel_p,
		ARRAY_SIZE(clk_mux_dpctr_pixel_p), 0x0C0, 0xc000, CLK_MUX_HIWORD_MASK, "mux_dpctr_pixel" },
	{ CLK_ISP_SNCLK_MUX0, "clk_mux_ispsn0", clk_isp_snclk_mux0_p,
		ARRAY_SIZE(clk_isp_snclk_mux0_p), 0x108, 0x18, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn0" },
	{ CLK_ISP_SNCLK_MUX1, "clk_mux_ispsn1", clk_isp_snclk_mux1_p,
		ARRAY_SIZE(clk_isp_snclk_mux1_p), 0x10C, 0x2000, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn1" },
	{ CLK_ISP_SNCLK_MUX2, "clk_mux_ispsn2", clk_isp_snclk_mux2_p,
		ARRAY_SIZE(clk_isp_snclk_mux2_p), 0x10C, 0x400, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn2" },
	{ CLK_ISP_SNCLK_MUX3, "clk_mux_ispsn3", clk_isp_snclk_mux3_p,
		ARRAY_SIZE(clk_isp_snclk_mux3_p), 0x100, 0xc0, CLK_MUX_HIWORD_MASK, "clk_mux_ispsn3" },
	{ CLK_MUX_RXDPHY_CFG, "clk_rxdphy_mux", clk_mux_rxdphy_cfg_p,
		ARRAY_SIZE(clk_mux_rxdphy_cfg_p), 0x0DC, 0xc, CLK_MUX_HIWORD_MASK, "clk_rxdphy_mux" },
};

static const struct div_clock hsdtctrl_div_clks[] = {
	{ DIV_SATA_OBB, "clk_div_sata_obb", "gt_clk_sata_obb_tmp",
		0x0AC, 0x3c00, 5, 1, 0, "clk_div_sata_obb" },
	{ DIV_XGE_AXI, "clk_div_xge_axi", "gt_aclk_xge",
		0x0AC, 0xf, 16, 1, 0, "clk_div_xge_axi" },
};

static const struct gate_clock hsdtctrl_gate_clks[] = {
	{ CLK_GATE_HSDT_TCU, "clk_hsdt_tcu", "clk_hsdt_subsys_ini", 0x010, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt_tcu" },
	{ ACLK_GATE_SATA2QIC_ABRG, "aclk_sata2qic_abrg", "clk_hsdt_subsys_ini", 0x010, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_sata2qic_abrg" },
	{ CLK_GATE_SATA_TBU, "clk_sata_tbu", "clk_hsdt_subsys_ini", 0x010, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata_tbu" },
	{ HCLK_GATE_SATA, "hclk_sata", "clk_hsdt_subsys_ini", 0x030, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sata" },
	{ CLK_GATE_SATA1_AUX, "clk_sata1_aux", "clkinsys_ini_tmp", 0x030, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata1_aux" },
	{ CLK_GATE_SATA0_AUX, "clk_sata0_aux", "clkinsys_ini_tmp", 0x030, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata0_aux" },
	{ CLK_GATE_SATA_PHY1REF, "clk_sata_phy1_ref", "clkin_sys", 0x030, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata_phy1_ref" },
	{ CLK_GATE_SATA_PHY0REF, "clk_sata_phy0_ref", "clkin_sys", 0x030, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata_phy0_ref" },
	{ CLK_GATE_SATA_OBB, "clk_sata_obb", "clk_div_sata_obb", 0x030, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sata_obb" },
	{ ACLK_GATE_XGE12QIC_ABRG, "aclk_xge12qic_abrg", "clk_div_xge_axi", 0x010, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_xge12qic_abrg" },
	{ ACLK_GATE_XGE02QIC_ABRG, "aclk_xge02qic_abrg", "clk_div_xge_axi", 0x010, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_xge02qic_abrg" },
	{ CLK_GATE_XGE_TBU, "clk_xge_tbu", "clk_div_xge_axi", 0x010, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge_tbu" },
	{ ACLK_GATE_XGE1, "aclk_xge1", "clk_div_xge_axi", 0x030, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_xge1" },
	{ ACLK_GATE_XGE0, "aclk_xge0", "clk_div_xge_axi", 0x030, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_xge0" },
	{ CLK_GATE_XGE1_CTRL, "clk_xge1_ctrl", "clkin_sys", 0x030, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge1_ctrl" },
	{ CLK_GATE_XGE0_CTRL, "clk_xge0_ctrl", "clkin_sys", 0x030, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge0_ctrl" },
	{ CLK_GATE_XGE1_PHY_REF, "clk_xge1_phy_ref", "clkin_sys", 0x030, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge1_phy_ref" },
	{ CLK_GATE_XGE0_PHY_REF, "clk_xge0_phy_ref", "clkin_sys", 0x030, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge0_phy_ref" },
	{ CLK_GATE_XGE1_CORE, "clk_xge1_core", "clk_xge_360", 0x030, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge1_core" },
	{ CLK_GATE_XGE0_CORE, "clk_xge0_core", "clk_xge_360", 0x030, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge0_core" },
	{ CLK_GATE_XGE1_PTP_REF, "clk_xge1_ptp_ref", "clk_xge_480", 0x030, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge1_ptp_ref" },
	{ CLK_GATE_XGE0_PTP_REF, "clk_xge0_ptp_ref", "clk_xge_480", 0x030, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_xge0_ptp_ref" },
};

static const struct scgt_clock hsdtctrl_scgt_clks[] = {
	{ CLK_ANDGT_CLK_SATA_OBBTMP, "gt_clk_sata_obb_tmp", "clk_xge_480",
		0x0AC, 14, CLK_GATE_HIWORD_MASK, "gt_clk_sata_obb_tmp" },
	{ ACLK_ANDGT_XGE, "gt_aclk_xge", "clk_hsdt_subsys_ini",
		0x0AC, 4, CLK_GATE_HIWORD_MASK, "gt_aclk_xge" },
};

static const struct div_clock hsdt1ctrl_div_clks[] = {
	{ CLK_DIV_USB2_ULPI, "clk_div_usb2_ulpi", "clkgt_usb2_ulpi",
		0x0A8, 0x1e0, 16, 1, 0, "clk_div_usb2_ulpi" },
};

static const char *const clk_mux_sd_sys_p [] = { "clkdiv_sdpll", "clk_sd_peri" };
static const char *const sel_hsdt1_suspend_p [] = { "clkin_sys", "clkin_ref" };
static const struct mux_clock hsdt1ctrl_mux_clks[] = {
	{ CLK_MUX_SD_SYS, "clk_sd_muxsys", clk_mux_sd_sys_p,
		ARRAY_SIZE(clk_mux_sd_sys_p), 0x0AC, 0x20, CLK_MUX_HIWORD_MASK, "clk_sd_muxsys" },
	{ SEL_HSDT1_SUSPEND, "clk_mux_hsdt1_suspend", sel_hsdt1_suspend_p,
		ARRAY_SIZE(sel_hsdt1_suspend_p), 0x0A8, 0x1, CLK_MUX_HIWORD_MASK, "clk_mux_hsdt1_suspend" },
};

static const struct gate_clock hsdt1ctrl_gate_clks[] = {
	{ CLK_GATE_SD, "clk_sd", "clk_sd_muxsys", 0x010, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 1, "clk_sd" },
	{ HCLK_GATE_SD, "hclk_sd", "clk_div_hsdt1bus", 0x0, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_sd" },
	{ CLK_GATE_SD32K, "clk_sd32k", "clkin_ref", 0x010, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_sd32k" },
	{ CLK_GATE_USB31PHY_APB, "usb31phy_apb", "clkmux_usb_dock", 0x000, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "usb31phy_apb" },
	{ HCLK_GATE_DCDR_MUX, "hclk_dcdr_mux", "clkmux_usb_dock", 0x000, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_dcdr_mux" },
	{ HGATE_HSDT1_MISCAHBS, "hclk_hsdt1_misc_ahb_s", "clkmux_usb_dock", 0x040, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hclk_hsdt1_misc_ahb_s" },
	{ AGATE_HIUSB3_BUS0, "aclk_hi_usb3_bus0", "clkmux_usb_dock", 0x000, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_hi_usb3_bus0" },
	{ AGATE_HIUSB3_BUS1, "aclk_hi_usb3_bus1", "clkmux_usb_dock", 0x000, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_hi_usb3_bus1" },
	{ AGATE_HIUSB3_BUS2, "aclk_hi_usb3_bus2", "clkmux_usb_dock", 0x020, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_hi_usb3_bus2" },
	{ AGATE_HIUSB3_BUS3, "aclk_hi_usb3_bus3", "clkmux_usb_dock", 0x030, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_hi_usb3_bus3" },
	{ CLK_GATE_USB2_BUS0, "clk_usb2_bus0", "clkmux_usb_dock", 0x020, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_bus0" },
	{ CLK_GATE_USB2_BUS1, "clk_usb2_bus1", "clkmux_usb_dock", 0x020, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_bus1" },
	{ CLKGATE_QICUSB2MAIN0, "clk_qic_usb2_main0", "clkmux_usb_dock", 0x020, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_qic_usb2_main0" },
	{ CLKGATE_QICUSB2MAIN1, "clk_qic_usb2_main1", "clkmux_usb_dock", 0x020, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_qic_usb2_main1" },
	{ CLK_GATE_USB2_BC0, "clk_usb2_bc0", "clkmux_usb_dock", 0x030, 0x400000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_bc0" },
	{ CLK_GATE_USB2_BC1, "clk_usb2_bc1", "clkmux_usb_dock", 0x030, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_bc1" },
	{ CLK_USB3_BC0, "clk_usb3_bc0", "clkmux_usb_dock", 0x030, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_bc0" },
	{ CLK_USB3_BC1, "clk_usb3_bc1", "clkmux_usb_dock", 0x030, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_bc1" },
	{ CLK_USB3_BC2, "clk_usb3_bc2", "clkmux_usb_dock", 0x030, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_bc2" },
	{ CLK_USB3_BC3, "clk_usb3_bc3", "clkmux_usb_dock", 0x030, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb3_bc3" },
	{ ACLK_GATE_TBU_USB0, "aclk_tbu_usb0", "clkmux_usb_dock", 0x040, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_tbu_usb0" },
	{ ACLK_GATE_TBU_USB1, "aclk_tbu_usb1", "clkmux_usb_dock", 0x040, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_tbu_usb1" },
	{ ACLK_GATE_TBU_USB2, "aclk_tbu_usb2", "clkmux_usb_dock", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_tbu_usb2" },
	{ PHSDT1GATE_MISC_APBM, "pclkhsdt1_miscapb_m", "clkmux_usb_dock", 0x040, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclkhsdt1_miscapb_m" },
	{ CLK_GATE_HIUSB3_SUSPEND0, "hi_usb3_suspend0", "clk_mux_hsdt1_suspend", 0x000, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3_suspend0" },
	{ CLK_GATE_HIUSB3_SUSPEND1, "hi_usb3_suspend1", "clk_mux_hsdt1_suspend", 0x000, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3_suspend1" },
	{ CLK_GATE_HIUSB3_SUSPEND2, "hi_usb3_suspend2", "clk_mux_hsdt1_suspend", 0x020, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3_suspend2" },
	{ CLK_GATE_HIUSB3_SUSPEND3, "hi_usb3_suspend3", "clk_mux_hsdt1_suspend", 0x030, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3_suspend3" },
	{ CLK_GATE_USB2_SUSPEND0, "clk_usb2_suspend0", "clk_mux_hsdt1_suspend", 0x020, 0x20000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_suspend0" },
	{ CLK_GATE_USB2_SUSPEND1, "clk_usb2_suspend1", "clk_mux_hsdt1_suspend", 0x020, 0x40000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_suspend1" },
	{ CLK_GATE_USBDP_MCU_32K, "usbdp_mcu_32k", "clkin_ref", 0x010, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "usbdp_mcu_32k" },
	{ CLK_GATE_HSDT1_USBDP_MCU, "hsdt1_usbdp_mcu", "clkmux_usb_dock", 0x010, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hsdt1_usbdp_mcu" },
	{ CLK_GATE_HSDT1_USBDP_MCU_BUS, "usbdp_mcu_bus", "clkmux_usb_dock", 0x010, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "usbdp_mcu_bus" },
	{ CLK_GATE_HSDT1_USBDP_MCU_19P2, "usbdp_mcu_19p2", "clkdiv_usb2phy_ref", 0x010, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "usbdp_mcu_19p2" },
	{ CLK_GATE_HI_USB3CTRL_REF0, "hi_usb3crtlref0", "clkdiv_usb2phy_ref", 0x000, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3crtlref0" },
	{ CLK_GATE_USB3CTRL_REF1, "hi_usb3crtlref1", "clkdiv_usb2phy_ref", 0x000, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3crtlref1" },
	{ CLK_GATE_USB3CTRL_REF2, "hi_usb3crtlref2", "clkdiv_usb2phy_ref", 0x020, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3crtlref2" },
	{ CLK_GATE_USB3CTRL_REF3, "hi_usb3crtlref3", "clkdiv_usb2phy_ref", 0x030, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "hi_usb3crtlref3" },
	{ CLK_GATE_USB2PHY0_REF, "clk_usb2phy0_ref", "clkdiv_usb2phy_ref", 0x020, 0x2000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy0_ref" },
	{ CLK_GATE_USB2PHY1REF, "clk_usb2phy1_ref", "clkdiv_usb2phy_ref", 0x020, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy1_ref" },
	{ CLK_GATE_USB2CTRL0REF, "clk_usb2ctrl0_ref", "clkdiv_usb2phy_ref", 0x020, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2ctrl0_ref" },
	{ CLK_GATE_USB2CTRL1REF, "clk_usb2ctrl1_ref", "clkdiv_usb2phy_ref", 0x020, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2ctrl1_ref" },
	{ GATE_USB2PHY_REFUDC0, "clk_usb2phy_ref_udc0", "clkdiv_usb2phy_ref", 0x020, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref_udc0" },
	{ GATE_USB2PHY_REFUDC1, "clk_usb2phy_ref_udc1", "clkdiv_usb2phy_ref", 0x020, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref_udc1" },
	{ GATE_USB2PHY_REFUDC2, "clk_usb2phy_ref_udc2", "clkdiv_usb2phy_ref", 0x020, 0x10000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref_udc2" },
	{ GATE_USB2PHY_REFUDC3, "clk_usb2phy_ref_udc3", "clkdiv_usb2phy_ref", 0x030, 0x1000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2phy_ref_udc3" },
	{ PCLK_GATE_USB2PHY0, "pclk_usb2phy0", "clkdiv_usb2phy_ref", 0x020, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy0" },
	{ PCLK_GATE_USB2PHY1, "pclk_usb2phy1", "clkdiv_usb2phy_ref", 0x020, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy1" },
	{ PCLK_GATEUSB2_SYSCTRL0, "pclk_usb2_sysctrl0", "clkdiv_usb2phy_ref", 0x020, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2_sysctrl0" },
	{ PCLK_GATEUSB2_SYSCTRL1, "pclk_usb2_sysctrl1", "clkdiv_usb2phy_ref", 0x020, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2_sysctrl1" },
	{ PCLK_GATE_USB2PHY_UDC0, "pclk_usb2phy_udc0", "clkdiv_usb2phy_ref", 0x020, 0x8000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy_udc0" },
	{ PCLK_GATE_USB2PHY_UDC1, "pclk_usb2phy_udc1", "clkdiv_usb2phy_ref", 0x020, 0x4000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy_udc1" },
	{ PCLK_GATE_USB2PHY_UDC2, "pclk_usb2phy_udc2", "clkdiv_usb2phy_ref", 0x020, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy_udc2" },
	{ PCLK_GATE_USB2PHY_UDC3, "pclk_usb2phy_udc3", "clkdiv_usb2phy_ref", 0x030, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb2phy_udc3" },
	{ CLK_GATE_USB31PHY_APB0, "clk_usb31phy_apb0", "clkdiv_usb2phy_ref", 0x000, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb31phy_apb0" },
	{ CLK_GATE_USB31PHY_APB1, "clk_usb31phy_apb1", "clkdiv_usb2phy_ref", 0x000, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb31phy_apb1" },
	{ CLK_GATE_USB31PHY_APB2, "clk_usb31phy_apb2", "clkdiv_usb2phy_ref", 0x020, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb31phy_apb2" },
	{ CLK_GATE_USB31PHY_APB3, "clk_usb31phy_apb3", "clkdiv_usb2phy_ref", 0x030, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb31phy_apb3" },
	{ PCLK_GATE_USB3_SYSCTRL0, "pclk_usb3_sysctrl0", "clkdiv_usb2phy_ref", 0x020, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_sysctrl0" },
	{ PCLK_GATE_USB3_SYSCTRL1, "pclk_usb3_sysctrl1", "clkdiv_usb2phy_ref", 0x020, 0x40000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_sysctrl1" },
	{ PCLK_GATE_USB3_SYSCTRL2, "pclk_usb3_sysctrl2", "clkdiv_usb2phy_ref", 0x020, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_sysctrl2" },
	{ PCLK_GATE_USB3_SYSCTRL3, "pclk_usb3_sysctrl3", "clkdiv_usb2phy_ref", 0x030, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_usb3_sysctrl3" },
	{ CLK_GATE_USBDP_AUX0, "clk_usbdp_aux0", "clkin_sys", 0x000, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usbdp_aux0" },
	{ CLK_GATE_USBDP_AUX1, "clk_usbdp_aux1", "clkin_sys", 0x000, 0x800000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usbdp_aux1" },
	{ CLK_GATE_USBDP_AUX2, "clk_usbdp_aux2", "clkin_sys", 0x020, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usbdp_aux2" },
	{ CLK_GATE_USBDP_AUX3, "clk_usbdp_aux3", "clkin_sys", 0x030, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usbdp_aux3" },
	{ CLK_GATE_USB2_ULPI, "clk_usb2_ulpi", "clk_div_usb2_ulpi", 0x000, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_usb2_ulpi" },
	{ PCLK_GATE_DPCTRL0, "pclk_dpctrl0", "clkdiv_usb2phy_ref", 0x00, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl0" },
	{ PCLK_GATE_DPCTRL1, "pclk_dpctrl1", "clkdiv_usb2phy_ref", 0x10, 0x4000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl1" },
	{ PCLK_GATE_DPCTRL2, "pclk_dpctrl2", "clkdiv_usb2phy_ref", 0x10, 0x2000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl2" },
	{ PCLK_GATE_DPCTRL3, "pclk_dpctrl3", "clkdiv_usb2phy_ref", 0x10, 0x1000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_dpctrl3" },
};

static const struct scgt_clock hsdt1ctrl_scgt_clks[] = {
	{ CLK_SD_SYS_GT, "clk_sd_sys_gt", "clk_sys_ini",
		0x0B0, 0, CLK_GATE_HIWORD_MASK, "clk_sd_sys_gt" },
	{ CLKGT_USB2_ULPI, "clkgt_usb2_ulpi", "clkmux_usb_dock",
		0x0A8, 4, CLK_GATE_HIWORD_MASK, "clkgt_usb2_ulpi" },
};

static const struct div_clock aocrg_div_clks[] = {
	{ CLK_DIV_HSDT1_USBDP, "clk_hsdt1_usbdp_div", "clk_andgt_hsdt1_usbdp",
		0x1B4, 0xf, 16, 1, 0, "clk_hsdt1_usbdp_div" },
	{ CLK_DIVHSDT1_USBDP_ULPLL, "clk_hsdt1_usbdp_ulpll_div", "clk_andgt_hsdt1_usbdp_ulpll",
		0x1B4, 0xc0, 4, 1, 0, "clk_hsdt1_usbdp_ulpll_div" },
	{ CLK_DIV_I2C9, "clkdiv_i2c9", "clkgt_i2c9",
		0x198, 0x1f, 32, 1, 0, "clkdiv_i2c9" },
	{ CLK_DIV_SPI3_ULPPLL, "div_spi3_ulppll", "gt_spi3_ulppll",
		0x1A4, 0xf, 16, 1, 0, "div_spi3_ulppll" },
	{ CLK_DIV_SPI3, "clkdiv_spi3", "clkgt_spi3",
		0x1A0, 0x1f80, 64, 1, 0, "clkdiv_spi3" },
	{ CLK_DIV_BLPWM2, "clkdiv_blpwm2", "clkgt_blpwm2",
		0x1A0, 0x60, 4, 1, 0, "clkdiv_blpwm2" },
	{ CLK_SYSCNT_DIV, "clk_syscnt_div", "clk_sys_ini",
		0x168, 0xf0, 10, 1, 0, "clk_syscnt_div" },
	{ CLK_SPLL_ASP_PLLDIV, "clkdiv_pll_asp_spll", "clkgt_spll_asp_pll",
		0x17c, 0x3f, 11, 1, 0, "clkdiv_pll_asp_spll" },
	{ CLKDIV_ASP_CODEC, "clkdiv_asp_codec", "clk_spll_asp",
		0x17c, 0x3f, 64, 1, 0, "clkdiv_asp_codec" },
	{ CLK_DIV_I2S5, "clk_i2s5_div", "clkgt_i2s5",
		NULL, 0x0, 32, 1, 0, "clk_i2s5_div" },
	{ CLKDIV_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao_div", "clk_dp_audio_pll_ao_gt",
		0x1A0, 0xf, 16, 1, 0, "clk_dp_audio_pll_ao_div" },
	{ DIV_MAD_FLL, "clk_mad_flldiv", "clkgt_mad_fll",
		0x18C, 0x1f, 32, 1, 0, "clk_mad_flldiv" },
	{ DIV_MAD_SPLL, "clk_mad_splldiv", "clkgt_mad_spll",
		0x168, 0xf, 16, 1, 0, "clk_mad_splldiv" },
	{ CLK_DIV_AO_CAMERA, "clkdiv_ao_camera", "clkgt_ao_camera",
		0x160, 0x3f, 64, 1, 0, "clkdiv_ao_camera" },
	{ CLK_DIV_AO_LOADMONITOR, "clk_ao_loadmonitor_div", "clk_ao_loadmonitor_gt",
		0x16C, 0x3c00, 16, 1, 0, "clk_ao_loadmonitor_div" },
};

static const char *const sel_clk_hsdt1usbdp_p [] = { "clk_hsdt1_usbdp_ulpll_div", "div_hsdt1_usbdp" };
static const char *const clk_mux_i2c9_p [] = {
		"clk_invalid", "clk_spll", "clk_sys_ini", "clk_invalid",
		"clk_fll_src", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ulppll_1", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_spi3_p [] = { "clkdiv_spi3", "div_spi3_ulppll" };
static const char *const clkmux_syscnt_p [] = { "clk_syscnt_div", "clkin_ref" };
static const char *const clk_mux_asp_pll_p [] = {
		"clk_invalid", "clk_spll", "sel_ao_asp_32kpll", "clk_invalid",
		"clk_ao_asp", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_sys_ini", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_ao_asp_32kpll_mux_p [] = { "clk_fll_src", "clk_ulppll_1" };
static const char *const sel_mad_mux_p [] = { "clk_mad_splldiv", "clk_mad_flldiv" };
static const char *const clk_mux_ao_camera_p [] = { "clk_sys_ini", "clk_fll_src", "sc_div_320m", "clk_isp_snclk1" };
static const struct mux_clock aocrg_mux_clks[] = {
	{ SEL_CLK_HSDT1USBDP, "clk_sel_hsdt1usbdp", sel_clk_hsdt1usbdp_p,
		ARRAY_SIZE(sel_clk_hsdt1usbdp_p), 0x1B4, 0x20, CLK_MUX_HIWORD_MASK, "clk_sel_hsdt1usbdp" },
	{ CLK_MUX_I2C9, "clkmux_i2c9", clk_mux_i2c9_p,
		ARRAY_SIZE(clk_mux_i2c9_p), 0x194, 0xf, CLK_MUX_HIWORD_MASK, "clkmux_i2c9" },
	{ CLK_MUX_SPI3, "clkmux_spi3", clk_mux_spi3_p,
		ARRAY_SIZE(clk_mux_spi3_p), 0x1A0, 0x8000, CLK_MUX_HIWORD_MASK, "clkmux_spi3" },
	{ CLKMUX_SYSCNT, "clkmux_syscnt", clkmux_syscnt_p,
		ARRAY_SIZE(clkmux_syscnt_p), 0x164, 0x2000, CLK_MUX_HIWORD_MASK, "clkmux_syscnt" },
	{ CLK_MUX_ASP_PLL, "clk_asp_pll_sel", clk_mux_asp_pll_p,
		ARRAY_SIZE(clk_mux_asp_pll_p), 0x178, 0xf, CLK_MUX_HIWORD_MASK, "clk_asp_pll_sel" },
	{ CLK_AO_ASP_32KPLL_MUX, "sel_ao_asp_32kpll", clk_ao_asp_32kpll_mux_p,
		ARRAY_SIZE(clk_ao_asp_32kpll_mux_p), 0x150, 0x2000, CLK_MUX_HIWORD_MASK, "sel_ao_asp_32kpll" },
	{ SEL_MAD_MUX, "clk_mad_sel", sel_mad_mux_p,
		ARRAY_SIZE(sel_mad_mux_p), 0x170, 0x80, CLK_MUX_HIWORD_MASK, "clk_mad_sel" },
	{ CLK_MUX_AO_CAMERA, "clkmux_ao_camera", clk_mux_ao_camera_p,
		ARRAY_SIZE(clk_mux_ao_camera_p), 0x188, 0x30, CLK_MUX_HIWORD_MASK, "clkmux_ao_camera" },
};

static const struct gate_clock aocrg_gate_clks[] = {
	{ PCLK_GPIO20, "pclk_gpio20", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio20" },
	{ PCLK_GPIO21, "pclk_gpio21", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_gpio21" },
	{ CLK_GATE_HSDT1_USBDP, "clk_hsdt1_usbdp", "clk_sel_hsdt1usbdp", 0x040, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_hsdt1_usbdp" },
	{ CLK_GATE_I2C9, "clk_i2c9", "clkdiv_i2c9", 0x50, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c9" },
	{ PCLK_GATE_I2C9, "pclk_i2c9", "clkdiv_i2c9", 0x50, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_i2c9" },
	{ CLK_GATE_SPI, "clk_spi3", "clkmux_spi3", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spi3" },
	{ CLK_GATE_QIC_SPI3DMA, "clk_qic_spi3dma", "clkmux_spi3", 0x040, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_qic_spi3dma" },
	{ PCLK_GATE_SPI, "pclk_spi3", "clkmux_spi3", 0x040, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_spi3" },
	{ PCLK_GATE_RTC, "pclk_rtc", "sc_div_aobus", 0x000, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_rtc" },
	{ PCLK_AO_GPIO0, "pclk_ao_gpio0", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio0" },
	{ PCLK_AO_GPIO1, "pclk_ao_gpio1", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio1" },
	{ PCLK_AO_GPIO2, "pclk_ao_gpio2", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio2" },
	{ PCLK_AO_GPIO3, "pclk_ao_gpio3", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio3" },
	{ PCLK_AO_GPIO4, "pclk_ao_gpio4", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio4" },
	{ PCLK_AO_GPIO5, "pclk_ao_gpio5", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio5" },
	{ PCLK_AO_GPIO6, "pclk_ao_gpio6", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio6" },
	{ PCLK_AO_GPIO29, "pclk_ao_gpio29", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio29" },
	{ PCLK_AO_GPIO30, "pclk_ao_gpio30", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio30" },
	{ PCLK_AO_GPIO31, "pclk_ao_gpio31", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio31" },
	{ PCLK_AO_GPIO32, "pclk_ao_gpio32", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio32" },
	{ PCLK_AO_GPIO33, "pclk_ao_gpio33", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio33" },
	{ PCLK_AO_GPIO34, "pclk_ao_gpio34", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio34" },
	{ PCLK_AO_GPIO35, "pclk_ao_gpio35", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio35" },
	{ PCLK_AO_GPIO36, "pclk_ao_gpio36", "sc_div_aobus", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_gpio36" },
	{ CLK_GATE_BLPWM2, "clk_blpwm2", "clkdiv_blpwm2", 0x0, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_blpwm2" },
	{ PCLK_GATE_SYSCNT, "pclk_syscnt", "sc_div_aobus", 0x0, 0x80000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_syscnt" },
	{ CLK_GATE_SYSCNT, "clk_syscnt", "clkmux_syscnt", 0x0, 0x100000, ALWAYS_ON,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_syscnt" },
	{ CLK_GATE_SPLL_ASP, "clk_spll_asp", "clkdiv_pll_asp_spll", 0x010, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_spll_asp" },
	{ CLK_GATE_ASP_SUBSYS, "clk_asp_subsys", "clk_asp_pll_sel", 0x10, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_subsys" },
	{ CLK_GATE_I2S5_CDC, "clk_i2s5_cdc", "clk_i2s5_div", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2s5_cdc" },
	{ CLK_GATE_I2S5_CFG_CDC, "clk_i2s5_cfg_cdc", "clk_i2s5_div", 0, 0x0, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2s5_cfg_cdc" },
	{ CLK_GATE_ASP_TCXO, "clk_asp_tcxo", "clk_sys_ini", 0x0, 0x8000000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_asp_tcxo" },
	{ CLK_GATE_DP_AUDIO_PLL, "clk_dp_audio_pll", "clk_dp_audio_pll_ao_div", 0x40, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dp_audio_pll" },
	{ CLK_GATE_MAD, "clk_mad", "clk_mad_sel", 0x000, 0x20000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_mad" },
	{ CLK_GATE_AO_CAMERA, "clk_ao_camera", "clkdiv_ao_camera", 0x010, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_camera" },
	{ PCLK_GATE_AO_LOADMONITOR, "pclk_ao_loadmonitor", "sc_div_aobus", 0x40, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_ao_loadmonitor" },
	{ CLK_GATE_AO_LOADMONITOR, "clk_ao_loadmonitor", "clk_ao_loadmonitor_div", 0x40, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ao_loadmonitor" },
};

static const struct scgt_clock aocrg_scgt_clks[] = {
	{ GT_CLK_HSDT1_USBDP, "clk_andgt_hsdt1_usbdp", "clk_spll",
		0x1B4, 4, CLK_GATE_HIWORD_MASK, "clk_andgt_hsdt1_usbdp" },
	{ GT_CLK_HSDT1_USBDP_ULPLL, "clk_andgt_hsdt1_usbdp_ulpll", "clk_fll_src",
		0x1B4, 8, CLK_GATE_HIWORD_MASK, "clk_andgt_hsdt1_usbdp_ulpll" },
	{ CLK_ANDGT_I2C9, "clkgt_i2c9", "clkmux_i2c9",
		0x194, 13, CLK_GATE_HIWORD_MASK, "clkgt_i2c9" },
	{ CLK_ANDGT_SPI3_ULPPLL, "gt_spi3_ulppll", "clk_fll_src",
		0x1A0, 13, CLK_GATE_HIWORD_MASK, "gt_spi3_ulppll" },
	{ CLK_ANDGT_SPI3, "clkgt_spi3", "clk_spll",
		0x1A0, 14, CLK_GATE_HIWORD_MASK, "clkgt_spi3" },
	{ CLK_ANDGT_BLPWM2, "clkgt_blpwm2", "clk_fll_src",
		0x164, 14, CLK_GATE_HIWORD_MASK, "clkgt_blpwm2" },
	{ GT_CLK_SPLL_ASP_PLL, "clkgt_spll_asp_pll", "clk_spll",
		0x174, 14, CLK_GATE_HIWORD_MASK, "clkgt_spll_asp_pll" },
	{ CLK_ANDGT_I2S5, "clkgt_i2s5", "clkdiv_pll_asp_spll",
		NULL, NULL, CLK_GATE_HIWORD_MASK, "clkgt_i2s5" },
	{ CLKGT_DP_AUDIO_PLL_AO, "clk_dp_audio_pll_ao_gt", "clk_ap_aupll",
		0x18C, 14, CLK_GATE_HIWORD_MASK, "clk_dp_audio_pll_ao_gt" },
	{ GT_CLK_MAD_FLL, "clkgt_mad_fll", "clk_fll_src",
		0x164, 15, CLK_GATE_HIWORD_MASK, "clkgt_mad_fll" },
	{ GT_CLK_MAD_SPLL, "clkgt_mad_spll", "clkdiv_pll_asp_spll",
		0x154, 6, CLK_GATE_HIWORD_MASK, "clkgt_mad_spll" },
	{ CLK_ANDGT_AO_CAMERA, "clkgt_ao_camera", "clkmux_ao_camera",
		0x188, 14, CLK_GATE_HIWORD_MASK, "clkgt_ao_camera" },
	{ CLK_GT_AO_LOADMONITOR, "clk_ao_loadmonitor_gt", "clk_fll_src",
		0x16C, 9, CLK_GATE_HIWORD_MASK, "clk_ao_loadmonitor_gt" },
};

static const struct gate_clock iomcu_gate_clks[] = {
	{ CLK_I2C1_GATE_IOMCU, "clk_i2c1_gt", "clk_fll_src", 0x10, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_i2c1_gt" },
};

static const struct div_clock media1crg_div_clks[] = {
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_vivobus_gt",
		0x70, 0x3f, 64, 1, 0, "clk_vivobus_div" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clkgt_vdec",
		0x64, 0x3f, 64, 1, 0, "clkdiv_vdec" },
	{ CLK_DIV_JPG_FUNC, "clk_jpg_func_div", "clkgt_jpg_func",
		0x68, 0x3f, 64, 1, 0, "clk_jpg_func_div" },
};

static const char *const clk_mux_vivobus_p [] = {
		"clk_invalid", "clk_ppll2b_media", "clk_ppll0_media", "clk_invalid",
		"clk_ppll1_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_vdec_p [] = {
		"clk_invalid", "clk_ppll2b_media", "clk_ppll0_media", "clk_invalid",
		"clk_ppll1_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll7_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_jpg_func_mux_p [] = {
		"clk_invalid", "clk_ppll2b_media", "clk_ppll0_media", "clk_invalid",
		"clk_ppll1_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll2_media", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media1crg_mux_clks[] = {
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", clk_mux_vivobus_p,
		ARRAY_SIZE(clk_mux_vivobus_p), 0x70, 0x3c0, CLK_MUX_HIWORD_MASK, "clk_vivobus_mux" },
	{ CLK_MUX_VDEC, "clkmux_vdec", clk_mux_vdec_p,
		ARRAY_SIZE(clk_mux_vdec_p), 0x64, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_vdec" },
	{ CLK_JPG_FUNC_MUX, "clkmux_jpg_func", clk_jpg_func_mux_p,
		ARRAY_SIZE(clk_jpg_func_mux_p), 0x68, 0x3c0, CLK_MUX_HIWORD_MASK, "clkmux_jpg_func" },
};

static const struct gate_clock media1crg_gate_clks[] = {
	{ CLK_DSS_TCXO, "clk_dss_tcxo", "clk_media_tcxo", 0x10, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dss_tcxo" },
	{ CLK_GATE_DSSLITE1_TCXO, "clk_dss_lite1_tcxo", "clk_media_tcxo", 0x700, 0x100000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_dss_lite1_tcxo" },
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_spll", 0x10, 0x10000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vivobus" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clkdiv_vdec", 0x00, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vdecfreq" },
	{ PCLK_GATE_VDEC, "pclk_vdec", "div_sysbus_pll", 0x00, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_vdec" },
	{ ACLK_GATE_VDEC, "aclk_vdec", "clk_vcodbus_div", 0x00, 0x1, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_vdec" },
	{ CLK_GATE_JPG_FUNCFREQ, "clk_jpg_funcfreq", "clk_jpg_func_div", 0x000, 0x400, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_jpg_funcfreq" },
	{ ACLK_GATE_JPG, "aclk_jpg", "clk_vcodbus_div", 0x000, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_jpg" },
	{ PCLK_GATE_JPG, "pclk_jpg", "div_sysbus_pll", 0x000, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_jpg" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_vivobus_div", 0x020, 0x80, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vivo" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clkdiv_vdec", 0x020, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vdec" },
};

static const struct scgt_clock media1crg_scgt_clks[] = {
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_vivobus_mux",
		0x84, 4, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vivobus_gt" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clkmux_vdec",
		0x84, 1, CLK_GATE_HIWORD_MASK, "clkgt_vdec" },
	{ CLK_ANDGT_JPG_FUNC, "clkgt_jpg_func", "clkmux_jpg_func",
		0x84, 2, CLK_GATE_HIWORD_MASK, "clkgt_jpg_func" },
};

static const struct div_clock media2crg_div_clks[] = {
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_vcodbus_gt",
		0x09C, 0x3f00, 64, 1, 0, "clk_vcodbus_div" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clkgt_venc",
		0x98, 0x3f00, 64, 1, 0, "clkdiv_venc" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_ispcpu_gt",
		0xB0, 0x3f, 64, 1, 0, "clk_ispcpu_div" },
	{ CLK_DIV_ISP_I3C, "clkdiv_isp_i3c", "clkgt_isp_i3c",
		0xA8, 0x3f00, 64, 1, 0, "clkdiv_isp_i3c" },
};

static const char *const clk_vcodec_syspll0_p [] = { "clk_vcodbus_mux", "clk_ppll0_m2" };
static const char *const clk_mux_vcodecbus_p [] = {
		"clk_invalid", "clk_ppll2b_m2", "clk_sys_ini", "clk_invalid",
		"clk_ppll1_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_fll_src", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_venc_p [] = {
		"clk_invalid", "clk_ppll2b_m2", "clk_ppll0_m2", "clk_invalid",
		"clk_ppll1_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll2_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const char *const clk_mux_ispi2c_p [] = { "clk_media_tcxo", "clk_isp_i2c_media" };
static const char *const clk_mux_ispcpu_p [] = {
		"clk_invalid", "clk_ppll2b_m2", "clk_ppll0_m2", "clk_invalid",
		"clk_ppll1_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_ppll2_m2", "clk_invalid", "clk_invalid", "clk_invalid",
		"clk_invalid", "clk_invalid", "clk_invalid", "clk_invalid"
};
static const struct mux_clock media2crg_mux_clks[] = {
	{ CLK_VCODEC_SYSPLL0, "clk_vcodec_syspll0", clk_vcodec_syspll0_p,
		ARRAY_SIZE(clk_vcodec_syspll0_p), 0x088, 0x10, CLK_MUX_HIWORD_MASK, "clk_vcodec_syspll0" },
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", clk_mux_vcodecbus_p,
		ARRAY_SIZE(clk_mux_vcodecbus_p), 0x080, 0xf, CLK_MUX_HIWORD_MASK, "clk_vcodbus_mux" },
	{ CLK_MUX_VENC, "clkmux_venc", clk_mux_venc_p,
		ARRAY_SIZE(clk_mux_venc_p), 0x80, 0xf0, CLK_MUX_HIWORD_MASK, "clkmux_venc" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", clk_mux_ispi2c_p,
		ARRAY_SIZE(clk_mux_ispi2c_p), 0x84, 0x1000, CLK_MUX_HIWORD_MASK, "clk_ispi2c_mux" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", clk_mux_ispcpu_p,
		ARRAY_SIZE(clk_mux_ispcpu_p), 0x80, 0xf000, CLK_MUX_HIWORD_MASK, "sel_ispcpu" },
};

static const struct gate_clock media2crg_gate_clks[] = {
	{ PCLK_GATE_ISP_QIC_SUBSYS, "pclk_isp_qic_subsys", "div_sysbus_pll", 0x10, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_isp_qic_subsys" },
	{ ACLK_GATE_ISP_QIC_SUBSYS, "aclk_isp_qic_subsys", "clk_spll", 0x10, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp_qic_subsys" },
	{ ACLK_GATE_ISP, "aclk_isp", "aclk_isp_qic_subsys", 0x10, 0x200000, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_isp" },
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_spll", 0x0, 0x200, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus" },
	{ CLK_GATE_VCODECBUS2DDR, "clk_vcodecbus2", "clk_vcodbus_div", 0x0, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vcodecbus2" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clkdiv_venc", 0x00, 0x20, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_vencfreq" },
	{ PCLK_GATE_VENC, "pclk_venc", "div_sysbus_pll", 0x00, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "pclk_venc" },
	{ ACLK_GATE_VENC, "aclk_venc", "clk_vcodbus_div", 0x00, 0x10, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "aclk_venc" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_ispi2c_mux", 0x10, 0x8, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispi2c" },
	{ CLK_GATE_ISP_SYS, "clk_isp_sys", "clk_media_tcxo", 0x10, 0x4, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_sys" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_ispcpu_div", 0x10, 0x2, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_ispcpu" },
	{ CLK_GATE_ISP_I3C, "clk_isp_i3c", "clkdiv_isp_i3c", 0x10, 0x100, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_isp_i3c" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_vcodbus_div", 0x00, 0x800, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clkdiv_venc", 0x00, 0x40, 0,
		NULL, 0, {0}, {0}, 0, 0, 0, "clk_atdiv_venc" },
};

static const struct scgt_clock media2crg_scgt_clks[] = {
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_vcodec_syspll0",
		0x0B8, 0, CLK_GATE_HIWORD_MASK | CLK_GATE_ALWAYS_ON_MASK, "clk_vcodbus_gt" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clkmux_venc",
		0xB8, 2, CLK_GATE_HIWORD_MASK, "clkgt_venc" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "sel_ispcpu",
		0xBC, 1, CLK_GATE_HIWORD_MASK, "clk_ispcpu_gt" },
	{ CLK_ANDGT_ISP_I3C, "clkgt_isp_i3c", "clk_ppll0_media",
		0xBC, 4, CLK_GATE_HIWORD_MASK, "clkgt_isp_i3c" },
};

static const struct xfreq_clock xfreq_clks[] = {
	{ CLK_CLUSTER0, "cpu-cluster.0", 0, 0, 0, 0x41C, {0x0001030A, 0x0}, {0x0001020A, 0x0}, "cpu-cluster.0" },
	{ CLK_CLUSTER1, "cpu-cluster.1", 1, 0, 0, 0x41C, {0x0002030A, 0x0}, {0x0002020A, 0x0}, "cpu-cluster.1" },
	{ CLK_G3D, "clk_g3d", 2, 0, 0, 0x41C, {0x0003030A, 0x0}, {0x0003020A, 0x0}, "clk_g3d" },
	{ CLK_DDRC_FREQ, "clk_ddrc_freq", 3, 0, 0, 0x41C, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_freq" },
	{ CLK_DDRC_MAX, "clk_ddrc_max", 5, 1, 0x8000, 0x230, {0x00040308, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_max" },
	{ CLK_DDRC_MIN, "clk_ddrc_min", 4, 1, 0x8000, 0x270, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_ddrc_min" },
	{ CLK_L1BUS_MIN, "clk_l1bus_min", 6, 1, 0x8000, 0x250, {0x00040309, 0x0}, {0x0004020A, 0x0}, "clk_l1bus_min" },
};

static const struct pmu_clock pmu_clks[] = {
	{ CLK_GATE_ABB_192, "clk_abb_192", "clkin_sys", 0x040, 0, 9, 0, "clk_abb_192" },
	{ CLK_GATE_ABB_384, "clk_abb_384", "clkin_sys", 0x040, 0, 9, 0, "clk_abb_384" },
	{ CLK_PMU32KA, "clk_pmu32ka", "clkin_ref", 0x4B, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32ka" },
	{ CLK_PMU32KB, "clk_pmu32kb", "clkin_ref", 0x4A, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kb" },
	{ CLK_PMU32KC, "clk_pmu32kc", "clkin_ref", 0x49, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmu32kc" },
	{ CLK_PMUAUDIOCLK, "clk_pmuaudioclk", "clk_sys_ini", 0x46, 0, INVALID_HWSPINLOCK_ID, 0, "clk_pmuaudioclk" },
	{ CLK_EUSB_38M4, "clk_eusb_38m4", "clk_sys_ini", 0x48, 0, INVALID_HWSPINLOCK_ID, 0, "clk_eusb_38m4" },
};

static const struct dvfs_clock dvfs_clks[] = {
	{ CLK_GATE_EDC0, "clk_edc0", "clk_edc0freq", 20, -1, 3, 1,
		{ 279000, 418000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_edc0" },
	{ CLK_GATE_VDEC, "clk_vdec", "clk_vdecfreq", 21, -1, 3, 1,
		{ 209000, 335000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_vdec" },
	{ CLK_GATE_VENC, "clk_venc", "clk_vencfreq", 24, -1, 3, 1,
		{ 279000, 418000, 642000 }, { 0, 1, 2, 3 }, 642000, 1, "clk_venc" },
	{ CLK_GATE_ISPFUNC, "clk_ispfunc", "clk_ispfuncfreq", 26, -1, 3, 1,
		{ 209000, 335000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_ispfunc" },
	{ CLK_GATE_ISPFUNC2, "clk_ispfunc2", "clk_ispfunc2freq", 38, -1, 3, 1,
		{ 209000, 335000, 558000 }, { 0, 1, 2, 3 }, 558000, 1, "clk_ispfunc2" },
	{ CLK_GATE_JPG_FUNC, "clk_jpg_func", "clk_jpg_funcfreq", 36, -1, 3, 1,
		{ 279000, 418000, 480000 }, { 0, 1, 2, 3 }, 480000, 1, "clk_jpg_func" },
};

static struct media_pll_info ppll2b_m1_info = MEDIA_PLL_INFO("clk_ppll2b_media", 1284000);
static struct media_pll_info ppll0_m1_info = MEDIA_PLL_INFO("clk_ppll0_media", 1671000);
static struct media_pll_info ppll1_m1_info = MEDIA_PLL_INFO("clk_ppll1_media", 1440000);
static struct media_pll_info ppll2_m1_info = MEDIA_PLL_INFO("clk_ppll2_media", 964000);
static struct media_pll_info *media1crg_plls[] = {
	&ppll2b_m1_info, &ppll0_m1_info, &ppll1_m1_info, &ppll2_m1_info,
};

static const struct fast_dvfs_clock fast_dvfs_clks_media1[] = {
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", { 0x60, 0x3c0, 6 }, { 0x60, 0x3f, 0 },
		{ 0x84, 0 }, { 0x10, 8 }, media1crg_plls, ARRAY_SIZE(media1crg_plls),
		{ 105000, 279000, 418000, 480000, 642000 }, 4, { 1, 1, 1, 2, 0 },
		{ 0x03C00080, 0x03C00080, 0x03C00080, 0x03C00100, 0x03C00040 },  { 16, 6, 4, 3, 2 },
		{ 0x003F000F, 0x003F0005, 0x003F0003, 0x003F0002, 0x003F0001 }, 0, "clk_edc0freq" },
};

static struct media_pll_info ppll2b_m2_info = MEDIA_PLL_INFO("clk_ppll2b_m2", 1284000);
static struct media_pll_info ppll0_m2_info = MEDIA_PLL_INFO("clk_ppll0_m2", 1671000);
static struct media_pll_info ppll1_m2_info = MEDIA_PLL_INFO("clk_ppll1_m2", 1440000);
static struct media_pll_info ppll2_m2_info = MEDIA_PLL_INFO("clk_ppll2_m2", 964000);
static struct media_pll_info *media2crg_plls[] = {
	&ppll2b_m2_info, &ppll0_m2_info, &ppll1_m2_info, &ppll2_m2_info,
};

static const struct fast_dvfs_clock fast_dvfs_clks_media2[] = {
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", { 0x080, 0xf00, 8 }, { 0x09c, 0x3f, 0 },
		{ 0x0bc, 0 }, { 0x10, 0 }, media2crg_plls, ARRAY_SIZE(media2crg_plls),
		{ 105000, 209000, 335000, 480000, 642000 }, 4, { 1, 1, 1, 2, 0 },
		{ 0x0F000200, 0x0F000200, 0x0F000200, 0x0F000400, 0x0F000100 }, { 16, 8, 5, 3, 2 },
		{ 0x003F000F, 0x003F0007, 0x003F0004, 0x003F0002, 0x003F0001 }, 0, "clk_ispfuncfreq" },
	{ CLK_GATE_ISPFUNC2FREQ, "clk_ispfunc2freq", { 0x084, 0xf00, 8 }, { 0x0a0, 0x3f, 0 },
		{ 0x0bc, 2 }, { 0x10, 6 }, media2crg_plls, ARRAY_SIZE(media2crg_plls),
		{ 105000, 209000, 335000, 558000, 642000 }, 4, { 1, 1, 1, 1, 0  },
		{ 0x0F000200, 0x0F000200, 0x0F000200, 0x0F000200, 0x0F000100 }, { 16, 8, 5, 3, 2 },
		{ 0x003F000F, 0x003F0007, 0x003F0004, 0x003F0002, 0x003F0001 }, 0, "clk_ispfunc2freq" },
};

static const struct fixed_factor_clock media1_stub_clks[] = {
	{ CLK_DSS_TCXO, "clk_dss_tcxo", "clk_sys_ini", 1, 1, 0, "clk_dss_tcxo" },
	{ CLK_GATE_DSSLITE1_TCXO, "clk_dss_lite1_tcxo", "clk_sys_ini", 1, 1, 0, "clk_dss_lite1_tcxo" },
	{ CLK_MUX_VIVOBUS, "clk_vivobus_mux", "clk_sys_ini", 1, 1, 0, "clk_vivobus_mux" },
	{ CLK_GATE_VIVOBUS_ANDGT, "clk_vivobus_gt", "clk_sys_ini", 1, 1, 0, "clk_vivobus_gt" },
	{ CLK_DIV_VIVOBUS, "clk_vivobus_div", "clk_sys_ini", 1, 1, 0, "clk_vivobus_div" },
	{ CLK_GATE_VIVOBUS, "clk_vivobus", "clk_sys_ini", 1, 1, 0, "clk_vivobus" },
	{ CLK_MUX_VDEC, "clkmux_vdec", "clk_sys_ini", 1, 1, 0, "clkmux_vdec" },
	{ CLK_ANDGT_VDEC, "clkgt_vdec", "clk_sys_ini", 1, 1, 0, "clkgt_vdec" },
	{ CLK_DIV_VDEC, "clkdiv_vdec", "clk_sys_ini", 1, 1, 0, "clkdiv_vdec" },
	{ CLK_GATE_VDECFREQ, "clk_vdecfreq", "clk_sys_ini", 1, 1, 0, "clk_vdecfreq" },
	{ PCLK_GATE_VDEC, "pclk_vdec", "clk_sys_ini", 1, 1, 0, "pclk_vdec" },
	{ ACLK_GATE_VDEC, "aclk_vdec", "clk_sys_ini", 1, 1, 0, "aclk_vdec" },
	{ CLK_JPG_FUNC_MUX, "clkmux_jpg_func", "clk_sys_ini", 1, 1, 0, "clkmux_jpg_func" },
	{ CLK_ANDGT_JPG_FUNC, "clkgt_jpg_func", "clk_sys_ini", 1, 1, 0, "clkgt_jpg_func" },
	{ CLK_DIV_JPG_FUNC, "clk_jpg_func_div", "clk_sys_ini", 1, 1, 0, "clk_jpg_func_div" },
	{ CLK_GATE_JPG_FUNCFREQ, "clk_jpg_funcfreq", "clk_sys_ini", 1, 1, 0, "clk_jpg_funcfreq" },
	{ ACLK_GATE_JPG, "aclk_jpg", "clk_sys_ini", 1, 1, 0, "aclk_jpg" },
	{ PCLK_GATE_JPG, "pclk_jpg", "clk_sys_ini", 1, 1, 0, "pclk_jpg" },
	{ CLK_GATE_ATDIV_VIVO, "clk_atdiv_vivo", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vivo" },
	{ CLK_GATE_ATDIV_VDEC, "clk_atdiv_vdec", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vdec" },
};

static const struct fixed_factor_clock media2_stub_clks[] = {
	{ PCLK_GATE_ISP_QIC_SUBSYS, "pclk_isp_qic_subsys", "clk_sys_ini", 1, 1, 0, "pclk_isp_qic_subsys" },
	{ ACLK_GATE_ISP_QIC_SUBSYS, "aclk_isp_qic_subsys", "clk_sys_ini", 1, 1, 0, "aclk_isp_qic_subsys" },
	{ ACLK_GATE_ISP, "aclk_isp", "clk_sys_ini", 1, 1, 0, "aclk_isp" },
	{ CLK_GATE_VCODECBUS, "clk_vcodecbus", "clk_sys_ini", 1, 1, 0, "clk_vcodecbus" },
	{ CLK_GATE_VCODECBUS2DDR, "clk_vcodecbus2", "clk_sys_ini", 1, 1, 0, "clk_vcodecbus2" },
	{ CLK_DIV_VCODECBUS, "clk_vcodbus_div", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_div" },
	{ CLK_GATE_VCODECBUS_GT, "clk_vcodbus_gt", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_gt" },
	{ CLK_VCODEC_SYSPLL0, "clk_vcodec_syspll0", "clk_sys_ini", 1, 1, 0, "clk_vcodec_syspll0" },
	{ CLK_MUX_VCODECBUS, "clk_vcodbus_mux", "clk_sys_ini", 1, 1, 0, "clk_vcodbus_mux" },
	{ CLK_MUX_VENC, "clkmux_venc", "clk_sys_ini", 1, 1, 0, "clkmux_venc" },
	{ CLK_ANDGT_VENC, "clkgt_venc", "clk_sys_ini", 1, 1, 0, "clkgt_venc" },
	{ CLK_DIV_VENC, "clkdiv_venc", "clk_sys_ini", 1, 1, 0, "clkdiv_venc" },
	{ CLK_GATE_VENCFREQ, "clk_vencfreq", "clk_sys_ini", 1, 1, 0, "clk_vencfreq" },
	{ PCLK_GATE_VENC, "pclk_venc", "clk_sys_ini", 1, 1, 0, "pclk_venc" },
	{ ACLK_GATE_VENC, "aclk_venc", "clk_sys_ini", 1, 1, 0, "aclk_venc" },
	{ CLK_MUX_ISPI2C, "clk_ispi2c_mux", "clk_sys_ini", 1, 1, 0, "clk_ispi2c_mux" },
	{ CLK_GATE_ISPI2C, "clk_ispi2c", "clk_sys_ini", 1, 1, 0, "clk_ispi2c" },
	{ CLK_GATE_ISP_SYS, "clk_isp_sys", "clk_sys_ini", 1, 1, 0, "clk_isp_sys" },
	{ CLK_MUX_ISPCPU, "sel_ispcpu", "clk_sys_ini", 1, 1, 0, "sel_ispcpu" },
	{ CLK_ANDGT_ISPCPU, "clk_ispcpu_gt", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_gt" },
	{ CLK_DIV_ISPCPU, "clk_ispcpu_div", "clk_sys_ini", 1, 1, 0, "clk_ispcpu_div" },
	{ CLK_GATE_ISPCPU, "clk_ispcpu", "clk_sys_ini", 1, 1, 0, "clk_ispcpu" },
	{ CLK_ANDGT_ISP_I3C, "clkgt_isp_i3c", "clk_sys_ini", 1, 1, 0, "clkgt_isp_i3c" },
	{ CLK_DIV_ISP_I3C, "clkdiv_isp_i3c", "clk_sys_ini", 1, 1, 0, "clkdiv_isp_i3c" },
	{ CLK_GATE_ISP_I3C, "clk_isp_i3c", "clk_sys_ini", 1, 1, 0, "clk_isp_i3c" },
	{ CLK_GATE_AUTODIV_VCODECBUS, "clk_atdiv_vcbus", "clk_sys_ini", 1, 1, 0, "clk_atdiv_vcbus" },
	{ CLK_GATE_ATDIV_VENC, "clk_atdiv_venc", "clk_sys_ini", 1, 1, 0, "clk_atdiv_venc" },
};

static const struct fixed_factor_clock fast_dvfs_clks_stub_media1[] = {
	{ CLK_GATE_EDC0FREQ, "clk_edc0freq", "clk_sys_ini", 1, 1, 0, "clk_edc0freq" },
};

static const struct fixed_factor_clock fast_dvfs_clks_stub_media2[] = {
	{ CLK_GATE_ISPFUNCFREQ, "clk_ispfuncfreq", "clk_sys_ini", 1, 1, 0, "clk_ispfuncfreq"},
	{ CLK_GATE_ISPFUNC2FREQ, "clk_ispfunc2freq", "clk_sys_ini", 1, 1, 0, "clk_ispfunc2freq"},
};

static void clk_crgctrl_init(struct device_node *np)
{
	struct clock_data *clk_crgctrl_data = NULL;
	int nr = ARRAY_SIZE(fixed_clks) +
		ARRAY_SIZE(fsm_pll_clks) +
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

	plat_clk_register_fsm_pll(fsm_pll_clks,
		ARRAY_SIZE(fsm_pll_clks), clk_crgctrl_data);

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
	int nr = ARRAY_SIZE(hsdtctrl_gate_clks) +
		ARRAY_SIZE(hsdtctrl_div_clks) +
		ARRAY_SIZE(hsdtctrl_scgt_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_gate(hsdtctrl_gate_clks,
		ARRAY_SIZE(hsdtctrl_gate_clks), clk_data);

	plat_clk_register_divider(hsdtctrl_div_clks,
		ARRAY_SIZE(hsdtctrl_div_clks), clk_data);

	plat_clk_register_scgt(hsdtctrl_scgt_clks,
		ARRAY_SIZE(hsdtctrl_scgt_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdtcrg,
	"hisilicon,clk-extreme-hsdt-crg", clk_hsdt_init);

static void clk_hsdt1_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(hsdt1ctrl_scgt_clks) +
		ARRAY_SIZE(hsdt1ctrl_div_clks) +
		ARRAY_SIZE(hsdt1ctrl_mux_clks) +
		ARRAY_SIZE(hsdt1ctrl_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_scgt(hsdt1ctrl_scgt_clks,
		ARRAY_SIZE(hsdt1ctrl_scgt_clks), clk_data);

	plat_clk_register_divider(hsdt1ctrl_div_clks,
		ARRAY_SIZE(hsdt1ctrl_div_clks), clk_data);

	plat_clk_register_mux(hsdt1ctrl_mux_clks,
		ARRAY_SIZE(hsdt1ctrl_mux_clks), clk_data);

	plat_clk_register_gate(hsdt1ctrl_gate_clks,
		ARRAY_SIZE(hsdt1ctrl_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_hsdt1crg,
	"hisilicon,clk-extreme-hsdt1-crg", clk_hsdt1_init);

static void clk_aocrg_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int nr = ARRAY_SIZE(aocrg_fsm_pll_clks) +
		ARRAY_SIZE(aocrg_scgt_clks) +
		ARRAY_SIZE(aocrg_div_clks) +
		ARRAY_SIZE(aocrg_mux_clks) +
		ARRAY_SIZE(aocrg_gate_clks);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;

	plat_clk_register_fsm_pll(aocrg_fsm_pll_clks,
		ARRAY_SIZE(aocrg_fsm_pll_clks), clk_data);

	plat_clk_register_scgt(aocrg_scgt_clks,
		ARRAY_SIZE(aocrg_scgt_clks), clk_data);

	plat_clk_register_divider(aocrg_div_clks,
		ARRAY_SIZE(aocrg_div_clks), clk_data);

	plat_clk_register_mux(aocrg_mux_clks,
		ARRAY_SIZE(aocrg_mux_clks), clk_data);

	plat_clk_register_gate(aocrg_gate_clks,
		ARRAY_SIZE(aocrg_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(clk_aocrg,
	"hisilicon,clk-extreme-sctrl", clk_aocrg_init);

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

static void clk_fast_dvfs_media1_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int no_media = media_check(MEDIA1);
	int nr;

	if (no_media)
		nr = ARRAY_SIZE(fast_dvfs_clks_stub_media1);
	else
		nr = ARRAY_SIZE(fast_dvfs_clks_media1);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;
	if (no_media)
		plat_clk_register_fixed_factor(fast_dvfs_clks_stub_media1,
			ARRAY_SIZE(fast_dvfs_clks_stub_media1), clk_data);
	else
		plat_clk_register_fast_dvfs_clk(fast_dvfs_clks_media1,
			ARRAY_SIZE(fast_dvfs_clks_media1), clk_data);
}
CLK_OF_DECLARE_DRIVER(clk_fast_dvfs_media1,
	"hisilicon,clk-extreme-fast-dvfs-media1", clk_fast_dvfs_media1_init);

static void clk_fast_dvfs_media2_init(struct device_node *np)
{
	struct clock_data *clk_data = NULL;
	int no_media = media_check(MEDIA2);
	int nr;

	if (no_media)
		nr = ARRAY_SIZE(fast_dvfs_clks_stub_media2);
	else
		nr = ARRAY_SIZE(fast_dvfs_clks_media2);

	clk_data = clk_init(np, nr);
	if (!clk_data)
		return;
	if (no_media)
		plat_clk_register_fixed_factor(fast_dvfs_clks_stub_media2,
			ARRAY_SIZE(fast_dvfs_clks_stub_media2), clk_data);
	else
		plat_clk_register_fast_dvfs_clk(fast_dvfs_clks_media2,
			ARRAY_SIZE(fast_dvfs_clks_media2), clk_data);
}
CLK_OF_DECLARE_DRIVER(clk_fast_dvfs_media2,
	"hisilicon,clk-extreme-fast-dvfs-media2", clk_fast_dvfs_media2_init);

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

