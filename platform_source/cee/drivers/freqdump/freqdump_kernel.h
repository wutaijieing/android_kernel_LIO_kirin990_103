/*
 * freqdump_kernel.h
 *
 * freqdump
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __FREQDUMP_KERNEL_H__
#define __FREQDUMP_KERNEL_H__

#include <linux/types.h>

#ifndef CONFIG_FREQDUMP_PLATFORM
#define FREQDUMP_BUFF_SIZE	1024
#else
#define FREQDUMP_BUFF_SIZE	(1024 << 1)
#endif
/*
 * this value should not less than
 * MAX_MONITOR_IP_PERI_CS * MONITOR_STAT_NUM * (sizeof(int) + 1) *
 * MAX_MONITOR_IP_DEVICE_AO * (MONITOR_STAT_NUM + 1) * (sizeof(long long) + 1)
 */
#ifdef CONFIG_FREQDUMP_PLATFORM_VENUS
#define LOADMONITOR_BUFF_SIZE	2500
#else
#define LOADMONITOR_BUFF_SIZE	2000
#endif

#define OK	0

#define MONITOR_STAT_NUM	3

enum ENABLE_FLAGS {
	PERI_MONITOR_EN = 0,
	AO_MONITOR_EN,
	ALL_MONITOR_EN,
};

enum PLAT_FLAG {
	ES = 0,
	CS,
};

#ifndef CONFIG_FREQDUMP_PLATFORM
struct freqdump_data {
	unsigned int apll[3][2];
	unsigned int a53_l_pllsel;
	unsigned int a53_b_pllsel;
	unsigned int ppll[4][2];
	unsigned int g3dclksel;
	unsigned int g3dclkdiv;
	unsigned int g3dclkdiv_stat;
	unsigned int ddrpllsel;
	unsigned int ddrclkdiv;
	unsigned int peri_usage[3];
	unsigned int dmss_asi_status;
	unsigned int dmc0_curr_sta;
	unsigned int dmc0_ddrc_cfg_wm;
	unsigned int dmc0_flux_wr;
	unsigned int dmc0_flux_rd;
	unsigned int dmc0_ddrc_intsts;
	unsigned int dmc0_tmon_aref;
	unsigned int dmc1_curr_sta;
	unsigned int dmc1_flux_wr;
	unsigned int dmc1_flux_rd;
	unsigned int dmc2_curr_sta;
	unsigned int dmc2_flux_wr;
	unsigned int dmc2_flux_rd;
	unsigned int dmc3_curr_sta;
	unsigned int dmc3_flux_wr;
	unsigned int dmc3_flux_rd;
	unsigned int qosbuf0_cmd;
	unsigned int qosbuf1_cmd;
	unsigned int a53_b_temp;
	unsigned int a53_l_temp;
	unsigned int gpu_temp;
	unsigned int peri_temp;
	unsigned int ddr_temp;
	unsigned int a53_b_buck;
	unsigned int a53_b_vol;
	unsigned int a53_l_buck;
	unsigned int a53_l_vol;
	unsigned int gpu_buck;
	unsigned int gpu_vol;
	unsigned int ddr_vol;
	unsigned int peri_vol;
	unsigned int battery_vol;
	unsigned int battery_current;
	unsigned int socid;
	unsigned int ddr_type;
	unsigned int clkdiv2;
	unsigned int clkdiv9;
};
#else
enum MODULE_CLK {
	LITTLE_CLUSTER_CLK = 0,
	MID_CLUSTER_CLK,
	BIG_CLUSTER_CLK,
	GPU_CLK,
	DDR_CLKA,
	DDR_CLKB,
	DMSS_CLK,
	L3_CLK,
	MAX_MODULE_CLK,
};

enum MODULE_TEMP {
	LITTLE_CLUSTER_TEMP = 0,
	BIG_CLUSTER_TEMP,
	GPU_TEMP,
	PERI_TEMP,
	MIDDLE_CLUSTER_TEMP,
	MAX_MODULE_TEMP,
};

enum MODULE_POWER {
	LITTLE_CLUSTER_POWER = 0,
	MID_CLUSTER_POWER,
	BIG_CLUSTER_POWER,
	GPU_POWER,
	DDR_POWER,
	PERI_POWER,
	MAX_MODULE_POWER,
};

enum MODULE_ADC_BUCK {
	BUCK_0_I = 0,
	BUCK_1_I,
	BUCK_2_I,
	BUCK_3_I,
	MAX_MODULE_ADC_BUCK,
};

enum MODULE_DDR_DIVCFG {
	DDR_DIVCFGA = 0,
	DDR_DIVCFGB,
	DMSS_DIVCFG,
	MAX_DDR_DIVCFG,
};

#define MAX_MODULE_BUCK 4

struct freqdump_data_bos {
	/* clk */
	unsigned int apll[6][2];
	unsigned int ppll[5][2];
	unsigned int clksel[MAX_MODULE_CLK];
	unsigned int ddrclkdiv[MAX_DDR_DIVCFG];
	unsigned int clkdiv2;
	unsigned int clkdiv9;
	unsigned int ddr_dumreg_pllsel;
	/* memory */
	unsigned int dmss_asi_status;
	unsigned int dmc_curr_stat[4];
	unsigned int dmc_flux_rd[4];
	unsigned int dmc_flux_wr[4];
	unsigned int dmc0_ddrc_cfg_wm;
	unsigned int dmc0_ddrc_intsts;
	unsigned int dmc0_tmon_aref;
	unsigned int qosbuf_cmd[2];
	/* temperature */
	unsigned int temp[MAX_MODULE_TEMP];
	/* power */
	unsigned int buck[MAX_MODULE_BUCK];
	unsigned int voltage[MAX_MODULE_POWER];
	unsigned int mem_voltage[MAX_MODULE_POWER];
	unsigned int peri_usage[4];
	unsigned int end_reserved;
	unsigned int adc_buck_i[MAX_MODULE_ADC_BUCK];
	unsigned int adc_vol[2];
	unsigned int clkdiv0;
	unsigned int clkdiv23;
	unsigned int clkdiv25;
	unsigned int cluster_pwrstat;
	unsigned int peri_vote[3];
	unsigned int flags;
	unsigned int npu[6][2];
};
#endif

extern void __iomem *g_loadmonitor_virt_addr;
extern void __iomem *g_freqdump_virt_addr;
void sec_freqdump_read(void);

#ifndef CONFIG_FREQDUMP_PLATFORM
void sec_loadmonitor_data_read(void);
void sec_loadmonitor_switch_enable(unsigned int delay_value);
#else
int ao_loadmonitor_read(u64 *addr);
void sec_chip_type_read(void);
void sec_loadmonitor_data_read(unsigned int enable_flags);
void sec_loadmonitor_switch_enable(unsigned int delay_value_peri,
					unsigned int delay_value_ao,
					unsigned int enable_flags);
#endif
int atfd_service_freqdump_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq);
int peri_monitor_clk_disable(const char *clk_name);
void sec_loadmonitor_switch_disable(void);
#ifdef CONFIG_NPU_PM_DEBUG
/*
 * data[6][2] used to store NPU module freq & voltage info
 * name[6][10] used to store NPU module name, max length is 9bytes
 */
int get_npu_freq_info(void *data, int size, char (*name)[10], int size_name);
#endif

#endif
