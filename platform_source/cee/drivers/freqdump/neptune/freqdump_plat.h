/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: monitor device status file
 * Author: duyouxuan
 * Create: 2019-03-29
 */
#ifndef __FREQDUMP_PLAT_H__
#define __FREQDUMP_PLAT_H__

#include <stdbool.h>
#include <tsensor_interface.h>
#include <loadmonitor_plat.h>

enum MODULE_CLK {
	LITTLE_CLUSTER_CLK = 0,
	MID_CLUSTER_CLK,
	BIG_CLUSTER_CLK,
	DDR_CLK,
	DMSS_CLK,
	L3_CLK,
	MAX_MODULE_CLK,
};

enum CPU_CLUSTER_IDX {
	LITTLE_CLUSTER_IDX = 0,
	MID_CLUSTER_IDX,
	BIG_CLUSTER_IDX,
	L3_CLUSTER_IDX,
	MAX_CPU_IDX,
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
	GPU_POWER, /* gpu core voltage, current */
	DDR_POWER,
	PERI_POWER,
	GPU_TOP_POWER, /* gpu top voltage, current */
	MAX_MODULE_POWER,
};

#define MAX_MODULE_BUCK	4

enum MODULE_DDR_DIVCFG {
	DDR_DIVCFGA = 0,
	DDR_DIVCFGB,
	DDR_DIVCFGAB,
	DMSS_DIVCFG,
	MAX_DDR_DIVCFG,
};

enum GPU_DOMAIN {
	GPU_DOMAIN_CORE = 0,
	GPU_DOMAIN_TOP,
	GPU_DOMAIN_MAX,
};

enum GPU_STATUS {
	GPU_STATUS_OFF = 0,
	GPU_STATUS_ON,
	GPU_STATUS_MAX,
};

struct gpu_status {
	unsigned int pwr_status;
	bool display_en;
	unsigned int freq; /* MHz */
};

struct lpmcu_stat {
	unsigned int freq; /* MHz */
	unsigned int utilization;
	unsigned int utilization_max;
};

#define GPU_DIVCFG	(DDR_DIVCFGA)
#define MAX_NPU_MODULE_SHOW	6
#define MAX_APLL_TYPE	6
#define MAX_APLL_CTRL	2
#define MAX_PPLL_TYPE	5
#define MAX_PPLL_CTRL	2
#define MAX_DDR_CHANNEL	4
#define MAX_QOSBUF_LEVEL	2
#define MAX_PERI_USAGE_TYPE	4
#define MAX_PERI_VOTE_TYPE	5
#define NPU_FREQ_VOLT	2
#define MAX_NPU_NAME_LEN	10

struct freqdump_data {
	/* clk */
	unsigned int apll[MAX_APLL_TYPE][MAX_APLL_CTRL];
	unsigned int ppll[MAX_PPLL_TYPE][MAX_PPLL_CTRL];
	unsigned int clksel[MAX_MODULE_CLK];
	unsigned int ddrclkdiv[MAX_DDR_DIVCFG];
	unsigned int clkdiv2;
	unsigned int clkdiv9;
	unsigned int ddr_dumreg_pllsel;
	unsigned int vdmdiv[MAX_CPU_IDX];
	/* memory */
	unsigned int dmss_asi_status;
	unsigned int dmc_curr_stat[MAX_DDR_CHANNEL];
	struct loadmonitor_ddr ddr;
	unsigned int dmc0_ddrc_cfg_wm;
	unsigned int dmc0_ddrc_intsts;
	unsigned int dmc0_tmon_aref;
	unsigned int ddr_type;
	unsigned int qosbuf_cmd[MAX_QOSBUF_LEVEL];
	/* temperature */
	unsigned int temp[MAX_MODULE_TEMP];
	/* power */
	unsigned int buck[MAX_MODULE_BUCK];
	unsigned int voltage[MAX_MODULE_POWER];
	unsigned int mem_voltage[MAX_MODULE_POWER];
	unsigned int peri_usage[MAX_PERI_USAGE_TYPE];
	unsigned int end_reserved;
	unsigned int clkdiv0;
	unsigned int clkdiv23;
	unsigned int clkdiv25;
	unsigned int cluster_pwrstat;
	unsigned int peri_vote[MAX_PERI_VOTE_TYPE];
	unsigned int flags;
	/* NPU freq and voltage info */
	unsigned int npu[MAX_NPU_MODULE_SHOW][NPU_FREQ_VOLT];
	/* Max NPU module name length is 9bytes */;
	char npu_name[MAX_NPU_MODULE_SHOW][MAX_NPU_NAME_LEN];
	unsigned int tsensor_temp[SENSOR_UNKNOWN_MAX];
	struct gpu_status gpu_stat[GPU_DOMAIN_MAX];
	struct lpmcu_stat lpmcu_stat;
};

struct freqdump_and_monitor_data {
	struct loadmonitor_data monitor;
	struct freqdump_data freqdump;
};

enum FREQ_PLAT_FLAG {
	FREQ_ES = 0,
	FREQ_CS = 1,
};

__attribute__((weak)) struct freqdump_data freqdump_data_value;
__attribute__((weak)) struct freqdump_and_monitor_data freqdump_and_monitor_data_value;

#endif /* __FREQDUMP_PLAT_H__ */
