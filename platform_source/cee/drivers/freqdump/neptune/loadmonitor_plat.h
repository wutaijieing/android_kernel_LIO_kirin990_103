/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: monitor device status file
 * Author: duyouxuan
 * Create: 2019-03-29
 */

#ifndef __LOADMONITOR_PLAT_H__
#define __LOADMONITOR_PLAT_H__

#define DDR_CHANNEL	4

#define freq_bit(nr)	(1UL << (nr))
#define dubai_u32_bit(nr)	(1U << (nr))

/* calc hexadecimal number count of 1. ex: 0xF result 4 */
#define bit_count(n) (((((n) - (((n) >> 1) & 033333333333) - \
		      (((n) >> 2) & 011111111111)) + \
		      (((n) - (((n) >> 1) & 033333333333) - \
		      (((n) >> 2) & 011111111111)) >> 3)) & 030707070707) % 63)

#define MAX_SENSOR_NUM	32
#define MONITOR_STAT_NUM	3

#define MONITOR0_IP_MASK	0x7FFFFFFF
#define MAX_PERI_MONITOR0_DEVICE	bit_count(MONITOR0_IP_MASK) /* 31 */

#define MONITOR1_IP_MASK	0x3FF440FF
#define MAX_PERI_MONITOR1_DEVICE	bit_count(MONITOR1_IP_MASK) /* 20 */

#define MEDIA_MONITOR0_IP_MASK	0x1FFFFFFC
#define MAX_MEDIA_MONITOR0_DEVICE	bit_count(MEDIA_MONITOR0_IP_MASK) /* 27 */

#define MEDIA_MONITOR1_IP_MASK	0xFFFFFFFF
#define MAX_MEDIA_MONITOR1_DEVICE	bit_count(MEDIA_MONITOR1_IP_MASK) /* 32 */

#define AO_MONITOR0_IP_MASK	0xFFE20F7F
#define MAX_MONITOR_IP_DEVICE_AO	bit_count(AO_MONITOR0_IP_MASK) /* 23 */

#define CPU_MONITOR_IP_MASK	0xFFFFFFFF
#define CPU_IP_COUNT	bit_count(CPU_MONITOR_IP_MASK) /* 32 */

#define NPU_MONITOR_IP_MASK	0xFFFFF
#define NPU_IP_COUNT	bit_count(NPU_MONITOR_IP_MASK) /* 20 */

#define MAX_PERI_MONITOR_DEVICE		(MAX_PERI_MONITOR0_DEVICE + \
					 MAX_PERI_MONITOR1_DEVICE)
#define MAX_MEDIA_MONITOR_DEVICE	(MAX_MEDIA_MONITOR0_DEVICE + \
					 MAX_MEDIA_MONITOR1_DEVICE)
#define MAX_PERI_AND_MEDIA_MONITOR_DEVICE	(MAX_PERI_MONITOR_DEVICE + \
						 MAX_MEDIA_MONITOR_DEVICE + \
						 CPU_IP_COUNT + NPU_IP_COUNT)
#define MAX_MONITOR_IP_DEVICE	(MAX_PERI_MONITOR_DEVICE + \
				 MAX_MEDIA_MONITOR_DEVICE + \
				 CPU_IP_COUNT + NPU_IP_COUNT + \
				 MAX_MONITOR_IP_DEVICE_AO)

#define CS_MAX_AO_INDEX	(MAX_MONITOR_IP_DEVICE_AO * \
			 (MONITOR_STAT_NUM + 1))
#define CS_MAX_PERI_INDEX	(MAX_PERI_AND_MEDIA_MONITOR_DEVICE * \
				 MONITOR_STAT_NUM)

#define U32_MAX_DIGIT	10
#define U64_MAX_DIGIT	20
#define LOADMONITOR_BUFF_SIZE	(CS_MAX_AO_INDEX * (U64_MAX_DIGIT + 1) + \
				 CS_MAX_PERI_INDEX * (U32_MAX_DIGIT + 1) + 1)

#define LOADMONITOR_PERI_FREQ_CS	166000000
#define MONITOR_PERI_FREQ_CS	166000000
#define LOADMONITOR_PCLK	166
#define LOADMONITOR_CPU_FREQ	166000000

#define LOADMONITOR_AO_FREQ	61440000
#define MONITOR_AO_FREQ_CS	61440000
#define LOADMONITOR_AO_CLK	61440

#define LOADMONITOR_MEDIA_FREQ	83
#define LOADMONITOR_MEDIA0_FREQ	83000000
#define LOADMONITOR_MEDIA1_FREQ	83000000

#define LOADMONITOR_NPU_FREQ	166000000
/* dtsi/arm64/xxx/xxx_bl31_kernel_share_mem.dtsi is related to share mem group */
#define SHARED_MEMORY_OFFSET	0x1e00
#define SHARED_MEMORY_SIZE	(1024 * 4)

#define DMC_FLUX_MAX_SIZE	4
#define DEFAULT_SAMPLES		1
#define LOADMONITOR_AO_TICK	'a'

struct ip_status {
	unsigned int idle;
	unsigned int busy_norm_volt;
	unsigned int buys_low_volt;
};

enum EN_FLAGS {
	ENABLE_NONE = 0x0,
	ENABLE_PERI0 = 0x2,
	ENABLE_PERI1 = 0x4,
	ENABLE_PERI = ENABLE_PERI0 | ENABLE_PERI1,
	ENABLE_NPU = 0x8,
	ENABLE_AO = 0x10,
	ENABLE_MEDIA0 = 0x20,
	ENABLE_MEDIA1 = 0x40,
	ENABLE_MEDIA = ENABLE_MEDIA0 | ENABLE_MEDIA1,
	ENABLE_CPU = 0x80,
	ENABLE_DDR = 0x10000,
	ENABLE_VOTE = dubai_u32_bit(17),
	ENABLE_ALL = ENABLE_PERI0 | ENABLE_MEDIA | ENABLE_CPU | ENABLE_AO |
		     ENABLE_DDR | ENABLE_VOTE,
};

struct ips_info {
	char name;
	unsigned int flag;
	unsigned int mask;
	unsigned int freq;
	unsigned int samples;
	struct ip_status ip[MAX_SENSOR_NUM];
};

/* notice: member has sequence */
struct loadmonitor_ips {
	struct ips_info peri0;
	struct ips_info peri1;
	struct ips_info npu;
	struct ips_info ao;
	struct ips_info media0;
	struct ips_info media1;
	struct ips_info cpu;
};

struct loadmonitor_ddr {
	unsigned int dmc_flux_rd[DMC_FLUX_MAX_SIZE];
	unsigned int dmc_flux_wr[DMC_FLUX_MAX_SIZE];
};

union soc_pmctrl_peri_vote_vol_rank_sft_0_union {
	unsigned int value;
	struct {
		unsigned int  peri_vote_vol_rank_0 : 2;
		unsigned int  peri_vote_vol_rank_1 : 2;
		unsigned int  peri_vote_vol_rank_2 : 2;
		unsigned int  peri_vote_vol_rank_3 : 2;
		unsigned int  peri_vote_vol_rank_4 : 2;
		unsigned int  peri_vote_vol_rank_5 : 2;
		unsigned int  peri_vote_vol_rank_6 : 2;
		unsigned int  peri_vote_vol_rank_7 : 2;
		unsigned int  reserved             : 16;
	} reg;
};

union soc_pmctrl_peri_vote_vol_rank_sft_1_union {
	unsigned int value;
	struct {
		unsigned int  peri_vote_vol_rank_8  : 2;
		unsigned int  peri_vote_vol_rank_9  : 2;
		unsigned int  peri_vote_vol_rank_10 : 2;
		unsigned int  peri_vote_vol_rank_11 : 2;
		unsigned int  peri_vote_vol_rank_12 : 2;
		unsigned int  peri_vote_vol_rank_13 : 2;
		unsigned int  peri_vote_vol_rank_14 : 2;
		unsigned int  peri_vote_vol_rank_15 : 2;
		unsigned int  reserved              : 16;
	} reg;
};

union soc_pmctrl_peri_vote_vol_valid_union {
	unsigned int value;
	struct {
		unsigned int  peri_vote_vol_valid_0  : 1;
		unsigned int  peri_vote_vol_valid_1  : 1;
		unsigned int  peri_vote_vol_valid_2  : 1;
		unsigned int  peri_vote_vol_valid_3  : 1;
		unsigned int  peri_vote_vol_valid_4  : 1;
		unsigned int  peri_vote_vol_valid_5  : 1;
		unsigned int  peri_vote_vol_valid_6  : 1;
		unsigned int  peri_vote_vol_valid_7  : 1;
		unsigned int  peri_vote_vol_valid_8  : 1;
		unsigned int  peri_vote_vol_valid_9  : 1;
		unsigned int  peri_vote_vol_valid_10 : 1;
		unsigned int  peri_vote_vol_valid_11 : 1;
		unsigned int  peri_vote_vol_valid_12 : 1;
		unsigned int  peri_vote_vol_valid_13 : 1;
		unsigned int  peri_vote_vol_valid_14 : 1;
		unsigned int  peri_vote_vol_valid_15 : 1;
		unsigned int  reserved               : 16;
	} reg;
};

struct dubai_peri {
	union soc_pmctrl_peri_vote_vol_rank_sft_0_union vote0;
	union soc_pmctrl_peri_vote_vol_rank_sft_1_union vote1;
	union soc_pmctrl_peri_vote_vol_valid_union enable;
};

struct loadmonitor_data {
	struct loadmonitor_ips ips;
	struct loadmonitor_ddr ddr;
	struct dubai_peri peri;
};

#define MAX_NUM_LOGIC_IP_DEVICE	(sizeof(struct loadmonitor_ips) / \
				 sizeof(struct ips_info) * MAX_SENSOR_NUM)

enum DIS_FLAGS {
	DIS_MEDIA0 = 0x1,
	DIS_MEDIA1 = 0X2,
	DIS_ALL = DIS_MEDIA0 | DIS_MEDIA1,
};

struct delay_time_with_freq {
	unsigned int monitor_enable_flags;
	unsigned int peri0;
	unsigned int peri1;
	unsigned int npu;
	unsigned int ao;
	unsigned int media0;
	unsigned int media1;
	unsigned int cpu;
};

/* disable and enable loadmonitor erea at atf */
enum LOADMONITOR_AREA {
	MONITOR_NPU,
	MONITOR_MEDIA1,
	MONITOR_MEDIA2,
	MONITOR_MAX,
};

/* data write addr */
typedef int (*loadmonitor_read)(void *data_addr);
/* second */
typedef int (*loadmonitor_enable)(int cycle_time);
struct loadmonitor_register {
	loadmonitor_enable enable;
	loadmonitor_read read;
};

__attribute__((weak)) struct loadmonitor_data loadmonitor_data_value = {
	.ips.peri0 = {
		'p', ENABLE_PERI0, MONITOR0_IP_MASK,
		LOADMONITOR_PERI_FREQ_CS, DEFAULT_SAMPLES, {}
	},
	.ips.peri1 = {
		'P', ENABLE_PERI1, MONITOR1_IP_MASK,
		LOADMONITOR_PERI_FREQ_CS, DEFAULT_SAMPLES, {}
	},
	.ips.npu = {
		'n', ENABLE_NPU, NPU_MONITOR_IP_MASK,
		LOADMONITOR_NPU_FREQ, DEFAULT_SAMPLES, {}
	},
	.ips.ao = {
		'a', ENABLE_AO, AO_MONITOR0_IP_MASK,
		MONITOR_AO_FREQ_CS, DEFAULT_SAMPLES, {}
	},
	/* different name is outside interface */
	.ips.media0 = {
		'm', ENABLE_MEDIA0, MEDIA_MONITOR0_IP_MASK,
		LOADMONITOR_MEDIA0_FREQ, DEFAULT_SAMPLES, {}
	},
	.ips.media1 = {
		'M', ENABLE_MEDIA1, MEDIA_MONITOR1_IP_MASK,
		LOADMONITOR_MEDIA1_FREQ, DEFAULT_SAMPLES, {}
	},
	.ips.cpu = {
		'c', ENABLE_CPU, CPU_MONITOR_IP_MASK,
		LOADMONITOR_CPU_FREQ, DEFAULT_SAMPLES, {}
	},
};

/* default sample time: 1s */
__attribute__((weak)) struct delay_time_with_freq g_monitor_delay_value = {
	.monitor_enable_flags = ENABLE_NONE,
	.peri0 = LOADMONITOR_PERI_FREQ_CS,
	.peri1 = LOADMONITOR_PERI_FREQ_CS,
	.npu = LOADMONITOR_NPU_FREQ,
	.ao = MONITOR_AO_FREQ_CS,
	.media0 = LOADMONITOR_MEDIA0_FREQ,
	.media1 = LOADMONITOR_MEDIA1_FREQ,
	.cpu = LOADMONITOR_CPU_FREQ,
};

void bl31_loadmonitor_enable(enum LOADMONITOR_AREA erea);
void bl31_loadmonitor_disable(enum LOADMONITOR_AREA erea);
void register_loadmonitor(enum LOADMONITOR_AREA area,
	loadmonitor_enable enable, loadmonitor_read read);

void hisi_sec_loadmonitor_npu_data_read(void);

#endif
