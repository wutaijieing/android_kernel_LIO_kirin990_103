/*
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __DTS_CLOCK_H
#define __DTS_CLOCK_H
/*clk_crgctrl*/
#define CLKIN_SYS		0
#define CLKIN_REF		1
#define CLK_FLL_SRC		2
#define CLK_PPLL0		3
#define CLK_PPLL1		4
#define CLK_PPLL2		5
#define CLK_PPLL3		6
#define CLK_PPLL5		7
#define CLK_SCPLL		8
#define CLK_PPLL6		9
#define CLK_PPLL7		10
#define CLK_SPLL		11
#define CLK_MODEM_BASE		12
#define CLK_LBINTPLL		13
#define CLK_LBINTPLL_1		14
#define CLK_PPLL_PCIE		15
#define PCLK		16
#define CLK_GATE_PPLL0		17
#define CLK_GATE_PPLL1		18
#define CLK_GATE_PPLL2		19
#define CLK_GATE_PPLL3		20
#define CLK_GATE_PPLL5		21
#define CLK_GATE_PPLL6		22
#define CLK_GATE_PPLL7		23
#define CLK_GATE_PPLL0_MEDIA		24
#define CLK_GATE_PPLL2_MEDIA		25
#define CLK_GATE_PPLL3_MEDIA		26
#define CLK_GATE_PPLL7_MEDIA		27
#define CLK_SYS_INI		28
#define CLK_DIV_SYSBUS		29
#define CLK_PPLL_EPS		30
#define PCLK_GPIO0		31
#define PCLK_GPIO1		32
#define PCLK_GPIO2		33
#define PCLK_GPIO3		34
#define PCLK_GPIO4		35
#define PCLK_GPIO5		36
#define PCLK_GPIO6		37
#define PCLK_GPIO7		38
#define PCLK_GPIO8		39
#define PCLK_GPIO9		40
#define PCLK_GPIO10		41
#define PCLK_GPIO11		42
#define PCLK_GPIO12		43
#define PCLK_GPIO13		44
#define PCLK_GPIO14		45
#define PCLK_GPIO15		46
#define PCLK_GPIO16		47
#define PCLK_GPIO17		48
#define PCLK_GPIO18		49
#define PCLK_GPIO19		50
#define PCLK_GATE_WD0_HIGH		51
#define CLK_GATE_WD0_HIGH		52
#define PCLK_GATE_WD0		53
#define PCLK_GATE_WD1		54
#define PCLK_GATE_DSI0		55
#define PCLK_GATE_DSI1		56
#define CLK_GATE_CODECSSI		57
#define PCLK_GATE_CODECSSI		58
#define CLK_FACTOR_TCXO		59
#define CLK_GATE_TIMER5_A		60
#define CLK_GATE_MMC_USBDP		61
#define PCLK_GATE_IOC		62
#define PCLK_GATE_MMC1_PCIE		63
#define CLK_ATDVFS		64
#define ATCLK		65
#define ACLK_GATE_PERF_STAT		66
#define PCLK_GATE_PERF_STAT		67
#define CLK_GATE_PERF_STAT		68
#define CLK_DIV_CSSYSDBG		69
#define CLK_GATE_CSSYSDBG		70
#define CLK_GATE_DMAC		71
#define NPU_PLL6_FIX		72
#define CLK_GATE_DMA_IOMCU		73
#define CLK_GATE_SOCP_ACPU		74
#define CLK_GATE_SOCP_DEFLATE		75
#define CLK_GATE_TCLK_SOCP		76
#define CLK_GATE_TIME_STAMP_GT		77
#define CLK_GATE_IPF		78
#define CLK_GATE_IPF_PSAM		79
#define ACLK_GATE_MAA		80
#define CLK_GATE_MAA_REF		81
#define CLK_GATE_SPE		82
#define HCLK_GATE_SPE		83
#define CLK_GATE_SPE_REF		84
#define ACLK_GATE_AXI_MEM		85
#define CLK_GATE_AXI_MEM		86
#define CLK_GATE_AXI_MEM_GS		87
#define CLK_GATE_VCODECBUS2DDR		88
#define CLK_GATE_SD		89
#define CLK_SD_SYS		90
#define CLK_SDIO_SYS		91
#define CLK_DIV_A53HPM		92
#define CLK_DIV_320M		93
#define CLK_GATE_UART1		94
#define CLK_GATE_UART4		95
#define PCLK_GATE_UART1		96
#define PCLK_GATE_UART4		97
#define CLK_GATE_UART2		98
#define CLK_GATE_UART5		99
#define PCLK_GATE_UART2		100
#define PCLK_GATE_UART5		101
#define CLK_GATE_UART0		102
#define PCLK_GATE_UART0		103
#define CLK_FACTOR_UART0		104
#define CLK_UART0_DBG		105
#define CLK_GATE_I2C3		106
#define CLK_GATE_I2C4		107
#define CLK_GATE_I2C6_ACPU		108
#define CLK_GATE_I2C7		109
#define PCLK_GATE_I2C3		110
#define PCLK_GATE_I2C4		111
#define PCLK_GATE_I2C6_ACPU		112
#define PCLK_GATE_I2C7		113
#define CLK_GATE_I3C4		114
#define PCLK_GATE_I3C4		115
#define CLK_GATE_SPI1		116
#define CLK_GATE_SPI4		117
#define PCLK_GATE_SPI1		118
#define PCLK_GATE_SPI4		119
#define CLK_GATE_USB3OTG_REF		120
#define CLK_FACTOR_USB3PHY_PLL		121
#define CLKIN_SYS_DIV		122
#define CLK_GATE_ABB_USB		123
#define CLK_GATE_UFSPHY_REF		124
#define CLK_GATE_UFSIO_REF		125
#define CLK_GATE_AO_ASP		126
#define PCLK_GATE_PCTRL		127
#define CLK_GATE_PWM		128
#define CLK_GATE_BLPWM		129
#define PCLK_GATE_BLPWM_PERI		130
#define CLK_SYSCNT_DIV		131
#define CLK_GATE_GPS_REF		132
#define CLK_MUX_GPS_REF		133
#define CLK_GATE_MDM2GPS0		134
#define CLK_GATE_MDM2GPS1		135
#define CLK_GATE_MDM2GPS2		136
#define CLK_GATE_LDI0		137
#define PERI_VOLT_HOLD		138
#define PERI_VOLT_MIDDLE		139
#define PERI_VOLT_LOW		140
#define EPS_VOLT_HIGH		141
#define EPS_VOLT_MIDDLE		142
#define EPS_VOLT_LOW		143
#define VENC_VOLT_HOLD		144
#define VDEC_VOLT_HOLD		145
#define EDC_VOLT_HOLD		146
#define EFUSE_VOLT_HOLD		147
#define LDI0_VOLT_HOLD		148
#define SEPLAT_VOLT_HOLD		149
#define CLK_FIX_DIV_DPCTRL		150
#define CLK_GATE_DPCTRL_16M		151
#define CLK_GATE_ISP_I2C_MEDIA		152
#define CLK_GATE_ISP_SNCLK0		153
#define CLK_GATE_ISP_SNCLK1		154
#define CLK_GATE_ISP_SNCLK2		155
#define CLK_GATE_ISP_SNCLK3		156
#define CLK_ISP_SNCLK_FAC		157
#define CLK_GATE_RXDPHY0_CFG		158
#define CLK_GATE_RXDPHY1_CFG		159
#define CLK_GATE_RXDPHY2_CFG		160
#define CLK_GATE_RXDPHY3_CFG		161
#define CLK_GATE_TXDPHY0_CFG		162
#define CLK_GATE_TXDPHY0_REF		163
#define CLK_GATE_TXDPHY1_CFG		164
#define CLK_GATE_TXDPHY1_REF		165
#define CLK_FACTOR_RXDPHY		166
#define PCLK_GATE_LOADMONITOR		167
#define CLK_GATE_LOADMONITOR		168
#define PCLK_GATE_LOADMONITOR_L		169
#define CLK_GATE_LOADMONITOR_L		170
#define PCLK_GATE_LOADMONITOR_2		171
#define CLK_GATE_LOADMONITOR_2		172
#define CLK_GATE_MEDIA_TCXO		173
#define CLK_GATE_AO_HIFD		174
#define CLK_DIV_HIFD_PLL		175
#define CLK_UART6		176
#define CLK_GATE_I2C0		177
#define CLK_GATE_I2C1		178
#define CLK_GATE_I2C2		179
#define CLK_GATE_SPI0		180
#define CLK_FAC_180M		181
#define CLK_GATE_IOMCU_PERI0		182
#define CLK_GATE_SPI2		183
#define CLK_GATE_UART3		184
#define CLK_GATE_UART8		185
#define CLK_GATE_UART7		186
#define OSC32K		187
#define OSC19M		188
#define CLK_480M		189
#define CLK_INVALID		190
#define AUTODIV_ISP_DVFS		191
#define AUTODIV_ISP		192
#define CLK_GATE_ATDIV_MMC0		193
#define CLK_GATE_ATDIV_DMA		194
#define CLK_GATE_ATDIV_CFG		195
#define CLK_GATE_ATDIV_SYS		196
#define CLK_FPGA_1P92		197
#define CLK_FPGA_2M		198
#define CLK_FPGA_10M		199
#define CLK_FPGA_19M		200
#define CLK_FPGA_20M		201
#define CLK_FPGA_24M		202
#define CLK_FPGA_26M		203
#define CLK_FPGA_27M		204
#define CLK_FPGA_32M		205
#define CLK_FPGA_40M		206
#define CLK_FPGA_48M		207
#define CLK_FPGA_50M		208
#define CLK_FPGA_57M		209
#define CLK_FPGA_60M		210
#define CLK_FPGA_64M		211
#define CLK_FPGA_80M		212
#define CLK_FPGA_100M		213
#define CLK_FPGA_160M		214

/* clk_hsdt_crg */
#define CLK_GATE_SCPLL		0
#define CLK_GATE_PCIEPHY_REF		1
#define CLK_GATE_PCIE1PHY_REF		2
#define PCLK_GATE_PCIE_SYS		3
#define PCLK_GATE_PCIE1_SYS		4
#define PCLK_GATE_PCIE_PHY		5
#define PCLK_GATE_PCIE1_PHY		6
#define HCLK_GATE_SDIO		7
#define CLK_GATE_SDIO		8
#define CLK_GATE_PCIEAUX		9
#define CLK_GATE_PCIEAUX1		10
#define ACLK_GATE_PCIE		11
#define ACLK_GATE_PCIE1		12
#define CLK_GATE_DP_AUDIO_PLL_AO		13

/* clk_mmc0_crg */
#define HCLK_GATE_USB3OTG		0
#define ACLK_GATE_USB3OTG		1
#define HCLK_GATE_SD		2
#define PCLK_GATE_DPCTRL		3
#define ACLK_GATE_DPCTRL		4

/* clk_sctrl */
#define PCLK_GPIO20		0
#define PCLK_GPIO21		1
#define CLK_GATE_TIMER5_B		2
#define CLK_GATE_TIMER5		3
#define CLK_GATE_SPI		4
#define PCLK_GATE_SPI		5
#define CLK_GATE_USB2PHY_REF		6
#define PCLK_GATE_RTC		7
#define PCLK_GATE_RTC1		8
#define PCLK_AO_GPIO0		9
#define PCLK_AO_GPIO1		10
#define PCLK_AO_GPIO2		11
#define PCLK_AO_GPIO3		12
#define PCLK_AO_GPIO4		13
#define PCLK_AO_GPIO5		14
#define PCLK_AO_GPIO6		15
#define PCLK_AO_GPIO29		16
#define PCLK_AO_GPIO30		17
#define PCLK_AO_GPIO31		18
#define PCLK_AO_GPIO32		19
#define PCLK_AO_GPIO33		20
#define PCLK_AO_GPIO34		21
#define PCLK_AO_GPIO35		22
#define PCLK_AO_GPIO36		23
#define PCLK_GATE_SYSCNT		24
#define CLK_GATE_SYSCNT		25
#define CLK_ASP_BACKUP		26
#define CLK_ASP_CODEC		27
#define CLK_GATE_ASP_SUBSYS		28
#define CLK_GATE_ASP_TCXO		29
#define CLK_GATE_DP_AUDIO_PLL		30
#define CLK_GATE_AO_CAMERA		31
#define PCLK_GATE_AO_LOADMONITOR		32
#define CLK_GATE_AO_LOADMONITOR		33
#define CLK_GATE_HIFD_TCXO		34
#define CLK_GATE_HIFD_FLL		35
#define CLK_GATE_HIFD_PLL		36

/* clk_iomcu_crgctrl */
#define CLK_I2C1_GATE_IOMCU		0

/* clk_media1_crg */
#define PCLK_GATE_ISP_NOC_SUBSYS		0
#define ACLK_GATE_ISP_NOC_SUBSYS		1
#define ACLK_GATE_MEDIA_COMMON		2
#define ACLK_GATE_NOC_DSS		3
#define PCLK_GATE_MEDIA_COMMON		4
#define PCLK_GATE_NOC_DSS_CFG		5
#define PCLK_GATE_MMBUF_CFG		6
#define PCLK_GATE_DISP_NOC_SUBSYS		7
#define ACLK_GATE_DISP_NOC_SUBSYS		8
#define PCLK_GATE_DSS		9
#define ACLK_GATE_DSS		10
#define ACLK_GATE_ISP		11
#define CLK_GATE_VIVOBUS_TO_DDRC		12
#define CLK_GATE_EDC0		13
#define CLK_GATE_LDI1		14
#define CLK_GATE_ISPI2C		15
#define CLK_GATE_ISP_SYS		16
#define CLK_GATE_ISPCPU		17
#define CLK_GATE_ISPFUNCFREQ		18
#define CLK_GATE_ISPFUNC2FREQ		19
#define CLK_GATE_ISPFUNC3FREQ		20
#define CLK_GATE_ISPFUNC4FREQ		21
#define CLK_GATE_ISP_I3C		22
#define CLK_GATE_MEDIA_COMMONFREQ		23
#define CLK_GATE_BRG		24
#define PCLK_GATE_MEDIA1_LM		25
#define CLK_GATE_LOADMONITOR_MEDIA1		26
#define CLK_GATE_FD_FUNC		27
#define CLK_GATE_JPG_FUNCFREQ		28
#define ACLK_GATE_JPG		29
#define PCLK_GATE_JPG		30
#define ACLK_GATE_NOC_ISP		31
#define PCLK_GATE_NOC_ISP		32
#define CLK_GATE_FDAI_FUNCFREQ		33
#define ACLK_GATE_ASC		34
#define CLK_GATE_DSS_AXI_MM		35
#define CLK_GATE_MMBUF		36
#define PCLK_GATE_MMBUF		37
#define CLK_GATE_ATDIV_VIVO		38
#define CLK_GATE_ATDIV_ISPCPU		39

/* clk_media2_crg */
#define CLK_GATE_VDECFREQ		0
#define PCLK_GATE_VDEC		1
#define ACLK_GATE_VDEC		2
#define CLK_GATE_VENCFREQ		3
#define PCLK_GATE_VENC		4
#define ACLK_GATE_VENC		5
#define ACLK_GATE_VENC2		6
#define CLK_GATE_VENC2FREQ		7
#define PCLK_GATE_MEDIA2_LM		8
#define CLK_GATE_LOADMONITOR_MEDIA2		9
#define CLK_GATE_IVP32DSP_TCXO		10
#define CLK_GATE_IVP32DSP_COREFREQ		11
#define CLK_GATE_AUTODIV_VCODECBUS		12
#define CLK_GATE_ATDIV_VDEC		13
#define CLK_GATE_ATDIV_VENC		14

/* clk_pctrl */
#define CLK_GATE_USB_TCXO_EN		0

/* clk_xfreqclk */
#define CLK_CLUSTER0		0
#define CLK_CLUSTER1		1
#define CLK_G3D		2
#define CLK_DDRC_FREQ		3
#define CLK_DDRC_MAX		4
#define CLK_DDRC_MIN		5

/* clk_pmuctrl */
#define CLK_GATE_ABB_192		0
#define CLK_PMU32KA		1
#define CLK_PMU32KB		2
#define CLK_PMU32KC		3
#define CLK_PMUAUDIOCLK		4

#define CLK_GATE_VDEC		0
#define CLK_GATE_VENC		1
#define CLK_GATE_VENC2		2
#define CLK_GATE_ISPFUNC		3
#define CLK_GATE_JPG_FUNC		4
#define CLK_GATE_FDAI_FUNC		5
#define CLK_GATE_MEDIA_COMMON		6
#define CLK_GATE_IVP32DSP_CORE		7
#define CLK_GATE_ISPFUNC2		8
#define CLK_GATE_ISPFUNC3		9
#define CLK_GATE_ISPFUNC4		10
#define CLK_GATE_HIFACE		11

#endif	/* __DTS_CLOCK_H */
