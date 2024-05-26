/*
 * IPP Head File
 *
 * Copyright (c) 2018 IPP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HIPP_CORE_H_
#define _HIPP_CORE_H_

enum HIPP_CLK_TYPE {
	IPPCORE_CLK = 0,
	IPP_CLK_MAX
};

enum HIPP_CLK_STATUS {
	IPP_CLK_DISABLE = 0,
	IPP_CLK_EN = 1,
	IPP_CLK_STATUS_MAX
};

enum hipp_wait_mode_type_e {
	WAIT_MODE_GF = 0,
	WAIT_MODE_ORB = 1,
	MAX_WAIT_MODE_TYPE
};

enum hipp_clk_rate_type_e {
	HIPPCORE_RATE_TBR = 0,
	HIPPCORE_RATE_NOR = 1,
	HIPPCORE_RATE_HSVS = 2,
	HIPPCORE_RATE_SVS = 3,
	HIPPCORE_RATE_OFF = 4,
	MAX_HIPP_CLKRATE_TYPE
};

struct hispcpe_s {
	struct platform_device *pdev;
	int initialized;
	struct regulator *media2_supply;
	struct regulator *cpe_supply;
	struct regulator *smmu_tcu_supply;

	unsigned int reg_num;
	struct resource *r[MAX_HISP_CPE_REG];
	void __iomem *reg[MAX_HISP_CPE_REG];

	unsigned int clock_num;
	struct clk *ippclk[IPP_CLK_MAX];
	unsigned int ippclk_value[IPP_CLK_MAX];
	unsigned int ippclk_normal[IPP_CLK_MAX];
	unsigned int ippclk_hsvs[IPP_CLK_MAX];
	unsigned int ippclk_svs[IPP_CLK_MAX];
	unsigned int ippclk_off[IPP_CLK_MAX];
	unsigned int jpeg_current_rate;
	unsigned int jpeg_rate[MAX_HIPP_COM];
	unsigned int sid;
	unsigned int ssid;
};

struct transition_info {
	unsigned int source_rate;
	unsigned int transition_rate;
};

#define HIPP_TRAN_NUM 4

static struct transition_info jpeg_trans_rate[HIPP_TRAN_NUM] = {
	{ 640000000, 214000000},
	{ 480000000, 160000000},
	{ 400000000, 200000000},
};

#define DTS_NAME_IPP "ipp"
#define hipp_min(a, b) (((a) < (b)) ? (a) : (b))

#endif
