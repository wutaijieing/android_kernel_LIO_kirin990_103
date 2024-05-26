/*
 * hisi_ddr_ddrcflux_common.h
 *
 * ddrcflux common message
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __HISI_DDR_DDRCFLUX_COMMON_H__
#define __HISI_DDR_DDRCFLUX_COMMON_H__
#include <soc_sctrl_interface.h>
#include <ddr_define.h>

#define ddr_flux(_addr, _offset, _byte_len, _head) \
{ \
	.addr = (_addr), \
	.offset = (_offset),\
	.byte_len = (_byte_len), \
	.head = (_head), \
}

#define DDRC_DMC_NUM_MAX		4
#define DDRC_QOSB_NUM_MAX		2
#define SLICE_INFO_LEN		6

#ifndef SCBAKDATA7_CH_NUM_BIT
#define SCBAKDATA7_CH_NUM_BIT		17
#endif

#define DDRC_1CH_FLAG		1
#define DDRC_2CH_FLAG		0
#define ddrflux_dbg(fmt, args...) \
	pr_debug("[%s]"fmt"\n", __func__, ##args)
#define ddrflux_err(fmt, args...) \
	pr_err("[%s:%d]"fmt"\n", __func__, __LINE__, ##args)

#define ASI_REG_GROUP1		0x1
#define ASI_REG_GROUP2		0x2
#define ASI_REG_GROUP3		0x3
#define REGISTER_BYTES_WIDTH		4
#define MULTI_INTERVAL		10
#define virt(ADDR, n)		(ioremap(ADDR, PAGE_ALIGN(SZ_4K * (n))))

#define MIN_STATISTIC_INTERVAL_UNSEC		100
#define MIN_STATISTIC_INTERVAL_SEC		1000
#define DMSS_REG_NUM_PER_ASI		3L
#define QOSB_REG_NUM_PER_QOSB		5L
#define DMC_REG_NUM_PER_DMC		19L

#define BIT_QOSB_PERF_EN	SOC_DDRC_QOSB_QOSB_CFG_PERF_perf_en_START

#define DDRC_DRAM_TYPE_MASK		0xF
#define DRAM_TYPE_MSG_LEN		10
#define MODULE_NAME		"ddr_flux_irq"
#define DEFAULT_SLICE_NAME		"test"
#define DDR_DMC_STD_ID_MASK_SHIFT		16
#define MAX_DDRFLUX_PULL_DATA		0x06400000 /* 100M */
#define NS_DDRC_TCLK		1000000000 /* 1ns */
#define TIMER_IRQ		91
#define SLICE_LEN		6
#define SCBAKDATA4		SOC_SCTRL_SCBAKDATA4_ADDR(0)
#define MASK_DDR		0xf00
#define DDRFLUX_PULL_OFFSET		0x0
#define DDRFLUX_EN_OFFSET		0x178
#define DDRFLUX_DRAMTYPE_OFFSET		0x180
#define DDRFLUX_STAID_OOFFSET		0x188

#define CMD_SUM_0_OFFSET		0x124
#define CMD_CNT_0_OFFSET		0x180
#define OSTD_CNT_0_OFFSET		0x1e0
#define WR_SUM_0_OFFSET		0x1e4
#define RD_SUM_0_OFFSET		0x1e8

#define CMD_SUM_1_OFFSET		0x124
#define CMD_CNT_1_OFFSET		0x180
#define OSTD_CNT_1_OFFSET		0x1e0
#define WR_SUM_1_OFFSET		0x1e4
#define RD_SUM_1_OFFSET		0x1e8
#define DDR_FLUX_QOSB_LEN		4

#ifndef REG_BASE_DMSS
#ifdef SOC_ACPU_DMSS_CFG_BASE_ADDR
#define REG_BASE_DMSS		SOC_ACPU_DMSS_CFG_BASE_ADDR
#else
#define REG_BASE_DMSS		SOC_ACPU_DMSS_BASE_ADDR
#endif
#endif

#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_W	0x55bbcce1UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_R	0x55bbcce2UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_DRAM_R	0x55bbcce3UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_STDID_W	0x55bbcce4UL

enum hisi_plat {
	LACERTA = 0,
	CETUS,
	DRACO,
	TRIONES,
	PERSEUS,
	URSA,
	CASSIOPEIA,
	CEPHEUS,
	MONOCEROS,
	PISCES,
	CHAMAELEON,
	PLAT_MAX,
};

enum ddrc_base {
	DMSS = 0,
	QOSB0,
	QOSB1,
	DMC00,
	DMC01,
	DMC10,
	DMC11,
};

/* usr para */
enum ddrc_flux_para {
	STA_ID = 0,
	STA_ID_MASK,
	INTERVAL,
	SUM_TIME,
	IRQ_AFFINITY,
	DDRC_UNSEC_PASS,
	DMSS_ENABLE,
	QOSBUF_ENABLE,
	DMC_ENABLE,
	DMCCMD_ENABLE,
	DMCDATA_ENABLE,
	DMCLATENCY_ENABLE,
	DDRC_FLUX_MAX,
};

enum ddr_mode {
	RESERVED = 0,
	LPDDR,
	LPDDR2,
	LPDDR3,
	DDR,
	DDR2,
	DDR3,
	DDR4,
	LPDDR4,
	LPDDR5,
};

struct ddrflux_pull {
	unsigned int addr;
	unsigned int offset;
	unsigned int byte_len;
	const char *head;
};

struct slice_info {
	char name[SLICE_INFO_LEN];
};

struct hisi_bw_timer {
	spinlock_t lock;
	void __iomem *base;
	struct clk *clk;
	struct clk *pclk;
	u32 irq;
	u32 irq_per_cpu;
};

struct ddrflux_data {
	char slice[SLICE_LEN];
	u64 ddrc_time;
	u32 ddr_tclk_ns;
	u32 ddrflux_data[0];
};

struct ddrc_flux_device {
	spinlock_t lock;
	void __iomem *ddrflux_base;
	void __iomem *dmss_base;
#ifdef DDRC_QOSB_PLATFORM
	void __iomem *qosb_base[DDRC_QOSB_NUM_MAX];
#endif
	void __iomem *dmc_base[DDRC_DMC_NUM_MAX];

#ifdef DDRC_EXMBIST_PCLK_GATE
	void __iomem *exmbist_base;
#endif

	void __iomem *flux_en_va;
	void __iomem *dramtype_va;
	void __iomem *flux_pull_va;
	void __iomem *flux_staid_va;
	dma_addr_t flux_en_dma_pa;
	dma_addr_t dramtype_dma_pa;
	dma_addr_t flux_pull_dma_pa;
	dma_addr_t flux_staid_pa;
	u32 katf_data_len;
	struct hisi_bw_timer *ddrc_flux_timer;
	int dram_type;
	struct clk *ddrc_freq_clk;
	void __iomem *sctrl;
	struct slice_info slice;
};

#endif /* __HISI_DDR_DDRCFLUX_COMMON_H__ */
