/*
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __CLK_ATHT_H
#define __CLK_ATHT_H

/* clk_aoncrg */
#define CLK_GATE_TIMER5		0
#define PCLK_GATE_SYSCNT		1
#define CLK_GATE_SYSCNT		2
#define CLKMUX_SYSCNT		3


/* clk_litecrg */
#define CLK_GATE_FNPLL1		0
#define CLK_GATE_PPLL0_MEDIA		1
#define CLK_GATE_FNPLL1_MEDIA		2
#define CLK_GATE_ULPLL_MEDIA		3
#define CLK_GATE_ODT_ACPU		4
#define CLK_GATE_ODT_DEFLATE		5
#define CLK_GATE_TCLK_ODT		6
#define CLKANDGT_EMMC_HF_SD_FLL		7
#define CLKANDGT_EMMC_HF_SD_PLL		8
#define CLK_EMMC_HF_SD_FLL_DIV		9
#define CLK_EMMC_HF_SD_PLL_DIV		10
#define CLK_MUX_EMMC_HF_SD		11
#define CLK_GATE_EMMC_HF_SD		12
#define CLKGT_ASP_CODEC		13
#define CLKDIV_ASP_CODEC		14
#define CLKGT_ASP_CODEC_FLL		15
#define CLKDIV_ASP_CODEC_FLL		16
#define CLK_MUX_ASP_CODEC		17
#define CLK_ASP_CODEC		18
#define CLK_GATE_ASP_SUBSYS		19
#define CLK_MUX_ASP_PLL		20
#define CLK_GATE_ASP_TCXO		21
#define CLK_GATE_TXDPHY0_CFG		22
#define CLK_GATE_TXDPHY0_REF		23


/* clk_crgctrl */
#define CLKIN_SYS		0
#define CLKIN_REF		1
#define CLK_FLL_SRC		2
#define CLK_SPLL		3
#define CLK_FNPLL1		4
#define CLK_MODEM_BASE		5
#define PCLK		6
#define CLK_GATE_PPLL0_M2		7
#define CLK_GATE_FNPLL1_M2		8
#define CLK_PPLL_EPS		9
#define CLK_SYS_INI		10
#define CLK_DIV_SYSBUS		11
#define CLK_DIV_HSDTBUS		12
#define PCLK_GATE_WD0		13
#define PCLK_GATE_DSS		14
#define CLK_DIV_AOBUS		15
#define CLK_USB2_DIV		16
#define AUTODIV_SYSBUS		17
#define PCLK_GATE_IOC		18
#define ATCLK		19
#define ACLK_GATE_PERF_STAT		20
#define PCLK_GATE_PERF_STAT		21
#define CLK_DIV_PERF_STAT		22
#define CLK_PERF_DIV_GT		23
#define CLK_GATE_PERF_STAT		24
#define CLK_GATE_PERF_CTRL		25
#define CLK_GATE_DMAC		26
#define CLK_GATE_DMA_IOMCU		27
#define CLK_GATE_TIME_STAMP_GT		28
#define CLK_DIV_TIME_STAMP		29
#define CLK_GATE_TIME_STAMP		30
#define CLK_GATE_PFA_TFT		31
#define ACLK_GATE_DSS		32
#define PCLK_GATE_DSI0		33
#define CLK_GATE_HIFACE		34
#define CLKANDGT_SDIO_HF_SD_FLL		35
#define CLKANDGT_SDIO_HF_SD_PLL		36
#define CLK_SDIO_HF_SD_FLL_DIV		37
#define CLK_SDIO_HF_SD_PLL_DIV		38
#define CLK_MUX_SDIO_HF_SD		39
#define CLK_GATE_SDIO_HF_SD		40
#define CLK_MUX_A53HPM		41
#define CLK_A53HPM_ANDGT		42
#define CLK_DIV_A53HPM		43
#define CLK_320M_PLL_GT		44
#define CLK_DIV_320M		45
#define CLK_GATE_UART1		46
#define CLK_GATE_UART4		47
#define PCLK_GATE_UART1		48
#define PCLK_GATE_UART4		49
#define CLK_GATE_UART2		50
#define CLK_GATE_UART5		51
#define PCLK_GATE_UART2		52
#define PCLK_GATE_UART5		53
#define PCLK_GATE_UART0		54
#define CLK_MUX_UART0		55
#define CLK_DIV_UART0		56
#define CLK_ANDGT_UART0		57
#define CLK_FACTOR_UART0		58
#define CLK_UART0_DBG		59
#define CLK_GATE_I2C2_ACPU		60
#define CLK_GATE_I2C3		61
#define CLK_GATE_I2C4		62
#define CLK_GATE_I2C6_ACPU		63
#define CLK_GATE_I2C7		64
#define PCLK_GATE_I2C2		65
#define PCLK_GATE_I2C3		66
#define PCLK_GATE_I2C4		67
#define PCLK_GATE_I2C6_ACPU		68
#define PCLK_GATE_I2C7		69
#define CLK_MUX_I2C		70
#define CLK_DIV_I2C		71
#define CLK_ANDGT_I2C		72
#define CLK_ANDGT_SPI1		73
#define CLK_DIV_SPI1		74
#define CLK_MUX_SPI1		75
#define CLK_GATE_SPI1		76
#define CLK_ANDGT_SPI2		77
#define CLK_DIV_SPI2		78
#define CLK_MUX_SPI2		79
#define CLK_GATE_SPI2		80
#define CLK_ANDGT_SPI3		81
#define CLK_DIV_SPI3		82
#define CLK_MUX_SPI3		83
#define CLK_GATE_SPI		84
#define PCLK_GATE_PCTRL		85
#define CLK_ANDGT_PTP		86
#define CLK_DIV_PTP		87
#define CLK_GATE_BLPWM		88
#define CLK_SYSCNT_DIV		89
#define CLK_GATE_GPS_REF		90
#define CLK_MUX_GPS_REF		91
#define CLK_GATE_MDM2GPS0		92
#define CLK_GATE_MDM2GPS1		93
#define CLK_GATE_MDM2GPS2		94
#define PERI_VOLT_HOLD		95
#define PERI_VOLT_MIDDLE		96
#define PERI_VOLT_LOW		97
#define VENC_VOLT_HOLD		98
#define VDEC_VOLT_HOLD		99
#define EDC_VOLT_HOLD		100
#define CLK_GATE_MEDIA2_TCXO		101
#define CLK_GT_ISP_I2C		102
#define CLK_DIV_ISP_I2C		103
#define CLK_GATE_ISP_I2C_MEDIA		104
#define CLK_GATE_ISP_SYS		105
#define CLK_GATE_ISP_I3C		106
#define CLK_GATE_ISP_SNCLK0		107
#define CLK_GATE_ISP_SNCLK1		108
#define CLK_ISP_SNCLK_MUX0		109
#define CLK_ISP_SNCLK_DIV0		110
#define CLK_ISP_SNCLK_MUX1		111
#define CLK_ISP_SNCLK_DIV1		112
#define CLK_ISP_SNCLK_FAC		113
#define CLK_ISP_SNCLK_ANGT		114
#define CLK_GATE_RXDPHY0_CFG		115
#define CLK_GATE_RXDPHY1_CFG		116
#define CLK_GATE_RXDPHY_CFG		117
#define CLK_MUX_RXDPHY_CFG		118
#define CLK_RXDPHY_FLL_DIV		119
#define CLKANDGT_RXDPHY_CFG_FLL		120
#define PCLK_GATE_LOADMONITOR		121
#define CLK_GATE_LOADMONITOR		122
#define CLK_DIV_LOADMONITOR		123
#define CLK_GT_LOADMONITOR		124
#define CLK_GATE_ISPFUNC2		125
#define CLK_GATE_ISPFUNC3		126
#define CLK_GATE_ISPFUNC4		127
#define CLK_GATE_ISPFUNC5		128
#define CLK_GATE_PFA_REF		129
#define HCLK_GATE_PFA		130
#define CLK_GATE_PFA		131
#define ACLK_GATE_DRA		132
#define CLK_UART6		133
#define CLK_GATE_I2C0		134
#define CLK_GATE_I2C1		135
#define CLK_GATE_I2C2		136
#define CLK_FAC_180M		137
#define CLK_GATE_IOMCU_PERI0		138
#define CLK_GATE_UART3		139
#define OSC32K		140
#define OSC19M		141
#define CLK_480M		142
#define CLK_INVALID		143
#define AUTODIV_CFGBUS		144
#define AUTODIV_ISP_DVFS		145
#define CLK_GATE_ATDIV_CFG		146
#define CLK_GATE_ATDIV_SYS		147
#define CLK_FPGA_1P92		148
#define CLK_FPGA_2M		149
#define CLK_FPGA_10M		150
#define CLK_FPGA_19M		151
#define CLK_FPGA_20M		152
#define CLK_FPGA_24M		153
#define CLK_FPGA_26M		154
#define CLK_FPGA_27M		155
#define CLK_FPGA_32M		156
#define CLK_FPGA_40M		157
#define CLK_FPGA_48M		158
#define CLK_FPGA_50M		159
#define CLK_FPGA_57M		160
#define CLK_FPGA_60M		161
#define CLK_FPGA_64M		162
#define CLK_FPGA_80M		163
#define CLK_FPGA_100M		164
#define CLK_FPGA_160M		165


/* clk_hsdt_crg */
#define PCLK_USB2_SYSCTRL		0
#define PCLK_USB2PHY		1
#define HCLK_USB2DRD		2
#define CLK_ANDGT_USB2_DIV		3
#define CLK_USB2_REF		4
#define CLK_MUX_USB2PHY_REF		5
#define CLK_USB2PHY_REF		6
#define CLK_GATE_SDIO_32K		7
#define ACLK_GATE_SDIO		8


/* clk_media1_crg */
#define CLK_MUX_VIVOBUS		0
#define CLK_GATE_VIVOBUS_ANDGT		1
#define CLK_DIV_VIVOBUS		2
#define CLK_GATE_VIVOBUS		3
#define CLK_MUX_EDC0		4
#define CLK_ANDGT_EDC0		5
#define CLK_DIV_EDC0		6
#define CLK_GATE_EDC0FREQ		7
#define CLK_MUX_VDEC		8
#define CLK_ANDGT_VDEC		9
#define CLK_DIV_VDEC		10
#define CLK_GATE_VDECFREQ		11
#define CLK_GATE_ATDIV_VIVO		12
#define CLK_GATE_ATDIV_VDEC		13


/* clk_media2_crg */
#define ACLK_GATE_ISP		0
#define CLK_GATE_VCODECBUS		1
#define CLK_DIV_VCODECBUS		2
#define CLK_GATE_VCODECBUS_GT		3
#define CLK_MUX_VCODECBUS		4
#define CLK_MUX_VENC		5
#define CLK_ANDGT_VENC		6
#define CLK_DIV_VENC		7
#define CLK_GATE_VENCFREQ		8
#define CLK_MUX_ISPI2C		9
#define CLK_GATE_ISPI2C		10
#define CLK_MUX_ISPCPU		11
#define CLK_ANDGT_ISPCPU		12
#define CLK_DIV_ISPCPU		13
#define CLK_GATE_ISPCPU		14
#define CLK_MUX_ISPFUNC		15
#define CLKANDGT_ISPFUNC		16
#define CLKDIV_ISPFUNC		17
#define CLK_GATE_ISPFUNCFREQ		18
#define CLK_GATE_AUTODIV_VCODECBUS		19
#define CLK_GATE_ATDIV_VENC		20


/* clk_xfreqclk */
#define CLK_CLUSTER0		0
#define CLK_CLUSTER1		1
#define CLK_G3D0		2
#define CLK_DDRC_FREQ0		3
#define CLK_DDRC_MAX0		4
#define CLK_DDRC_MIN0		5


/* clk_pmuctrl */
#define CLK_PMU32KA		0
#define CLK_PMU32KB		1
#define CLK_PMU32KC		2
#define CLK_PMUAUDIOCLK		3


/* clk_dvfs */
#define CLK_GATE_EDC0		0
#define CLK_GATE_VDEC		1
#define CLK_GATE_VENC		2
#define CLK_GATE_ISPFUNC		3


#endif /* __CLK_ATHT_H */
