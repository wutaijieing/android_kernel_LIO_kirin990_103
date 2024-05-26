/*
 * hisi-clk-test.c
 *
 * hisilicon clock test implement
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "clk-debug.h"
#include "../clk.h"
#include "../clk-kirin-common.h"
#include "../hisi-kirin-ppll.h"
#include "clk-debug-510.h"

#define REG_VADDR_MAP(phyAddr)          ioremap(phyAddr, sizeof(unsigned long))
#define REG_VADDR_UNMAP(virAddr)        iounmap(virAddr)

#define clklog_err(fmt, args...)  \
	pr_err("[hisi_clk_test] [%s]" fmt, __func__, ## args)
#define clklog_info(fmt, args...) \
	pr_info("[hisi_clk_test] [%s]" fmt, __func__, ## args)
#define clklog_debug(fmt, args...) do { } while (0)
#define seq_clklog(s, fmt, args...)                              \
	do {                                                     \
		if (s)                                           \
			seq_printf(s, fmt, ## args);             \
		else                                             \
			pr_err("[hisi_clk_test] " fmt, ## args); \
	} while (0)

/* check kernel data state matched to register */
#define MATCH                            0
#define PARTIAL_MATCH                   (-2)
#define MISMATCH                        (-1)

/* Fixed-Rate Clock Attrs: clock freq */
#define HISI_FIXED_RATE_ATTR            "clock-frequency"

/* General-Gate Clock Attrs: offset and bitmask */
#define HISI_GENERAL_GATE_REG_ATTR      "hisilicon,hi3xxx-clkgate"

/* Himask-Gate Clock Attrs */
#define HISI_HIMASK_ATTR                "hiword"
#define HISI_HIMASK_GATE_REG_ATTR       "hisilicon,clkgate"

/* MUX(SW) Clock Attrs */
#define HISI_MUX_REG_ATTR               "hisilicon,clkmux-reg"

/* DIV Clock Attr */
#define HISI_DIV_REG_ATTR               "hisilicon,clkdiv"

/* Fixed-DIV Clock Attr */
#define HISI_FIXED_DIV_ATTR1            "clock-div"
#define HISI_FIXED_DIV_ATTR2            "clock-mult"

/* PPLL Clock Attr */
#define HISI_PPLL_EN_ATTR               "hisilicon,pll-en-reg"
#define HISI_PPLL_GT_ATTR               "hisilicon,pll-gt-reg"
#define HISI_PPLL_BP_ATTR               "hisilicon,pll-bypass-reg"
#define HISI_PPLL_CTRL0_ATTR            "hisilicon,pll-ctrl0-reg"

/* PMU CLock Attr */
#define HISI_PMU_ATTR                   "pmu32khz"

/* Clock Types */
enum {
	HISI_CLOCK_FIXED = 0,
	HISI_CLOCK_GENERAL_GATE,
	HISI_CLOCK_HIMASK_GATE,
	HISI_CLOCK_MUX,
	HISI_CLOCK_DIV,
	HISI_CLOCK_FIXED_DIV,
	HISI_CLOCK_PPLL,
	HISI_CLOCK_PLL,
	HISI_CLOCK_PMU,
	HISI_CLOCK_IPC,
	HISI_CLOCK_ROOT,
	HISI_CLOCK_DVFS,
	MAX_CLOCK_TYPE,
};

struct clock_check_param {
	struct seq_file *s;
	struct device_node *np;
	struct clk_core *clk;
	int level;
	int parent_idx;
};

#define CLOCK_REG_GROUPS   5
struct clock_print_info {
	const char *clock_name;
	const char *clock_type;
	unsigned long rate;
	int clock_reg_groups;
	const char *crg_region[CLOCK_REG_GROUPS];
	int region_stat[CLOCK_REG_GROUPS];
	unsigned int reg[CLOCK_REG_GROUPS];
	unsigned char h_bit[CLOCK_REG_GROUPS];
	unsigned char l_bit[CLOCK_REG_GROUPS];
	unsigned int reg_val[CLOCK_REG_GROUPS];
	unsigned int enable_cnt;
	unsigned int prepare_cnt;
	int check_state;
	int parent_idx;
};

#define CLOCK_TYPE_NAME_LEN 64
#define CLOCK_TYPE_ALIAS_LEN 32
struct hisi_clock_type {
	char name[CLOCK_TYPE_NAME_LEN];
	int (*check_func)(struct clock_check_param *);
	char alias[CLOCK_TYPE_ALIAS_LEN];
};

static int check_fixed_rate(struct clock_check_param *param);
static int check_general_gate(struct clock_check_param *param);
static int check_himask_gate(struct clock_check_param *param);
static int check_mux_clock(struct clock_check_param *param);
static int check_div_clock(struct clock_check_param *param);
static int check_fixed_div_clock(struct clock_check_param *param);
static int check_kirin_ppll_clock(struct clock_check_param *param);
static int check_hisi_pll_clock(struct clock_check_param *param);
static int check_pmu_clock(struct clock_check_param *param);
static int check_ipc_clock(struct clock_check_param *param);
static int check_root_clock(struct clock_check_param *param);
static int check_dvfs_clock(struct clock_check_param *param);

#if defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
static int peri_power_up_ok(void);
#endif
#if defined(SOC_ACPU_SCTRL_BASE_ADDR)
static int sctrl_power_up_ok(void);
#endif
#if defined(SOC_ACPU_PMC_BASE_ADDR)
static int pmctrl_power_up_ok(void);
#endif
#if defined(SOC_ACPU_PCTRL_BASE_ADDR)
static int pctrl_power_up_ok(void);
#endif
#if defined(SOC_ACPU_MEDIA1_CRG_BASE_ADDR)
static int media1_power_up_ok(void);
#endif
#if defined(SOC_ACPU_MEDIA2_CRG_BASE_ADDR)
static int media2_power_up_ok(void);
#endif
#if defined(SOC_ACPU_IOMCU_CONFIG_BASE_ADDR)
static int iomcu_power_up_ok(void);
#endif
#if defined(SOC_ACPU_MMC1_SYS_CTRL_BASE_ADDR)
static int mmc1sysctrl_power_up_ok(void);
#endif
#if defined(SOC_ACPU_MMC0_CRG_BASE_ADDR)
static int mmc0_power_up_ok(void);
#endif
#if defined(SOC_ACPU_HSDT_CRG_BASE_ADDR)
static int hsdt_power_up_ok(void);
#endif
#if defined(SOC_ACPU_HSDT1_CRG_BASE_ADDR)
static int hsdt1_power_up_ok(void);
#endif
#if defined(SOC_ACPU_SPMI_BASE_ADDR)
static int pmuspmi_power_up_ok(void);
#endif

const struct hisi_clock_type clock_types[] = {
	{ "fixed-clock",                check_fixed_rate,       "Fixed-Rate"  },
	{ "hisilicon,hi3xxx-clk-gate",  check_general_gate,     "Gate"        },
	{ "hisilicon,clk-gate",         check_himask_gate,      "Himask-Gate" },
	{ "hisilicon,hi3xxx-clk-mux",   check_mux_clock,        "Mux(SW)"     },
	{ "hisilicon,hi3xxx-clk-div",   check_div_clock,        "Div"         },
	{ "fixed-factor-clock",         check_fixed_div_clock,  "Fixed-Factor"},
	{ "hisilicon,kirin-ppll-ctrl",  check_kirin_ppll_clock, "PPLL"        },
	{ "hisilicon,ppll-ctrl",        check_hisi_pll_clock,   "PLL"         },
	{ "hisilicon,clk-pmu-gate",     check_pmu_clock,        "PMU-Gate"    },
	{ "hisilicon,interactive-clk",  check_ipc_clock,        "IPC-Gate"    },
	{ "hisilicon,hi3xxx-xfreq-clk", check_root_clock,       "Root"        },
	{ "hisilicon,clkdev-dvfs",      check_dvfs_clock,       "DVFS"        },
};
const int clock_type_size = ARRAY_SIZE(clock_types);

/* CRG Regions Types */
enum {
#if defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_PERI,
#endif
#if defined(SOC_ACPU_SCTRL_BASE_ADDR)
	HISI_CLOCK_CRG_SCTRL,
#endif
#if defined(SOC_ACPU_PMC_BASE_ADDR)
	HISI_CLOCK_CRG_PMCTRL,
#endif
#if defined(SOC_ACPU_PCTRL_BASE_ADDR)
	HISI_CLOCK_CRG_PCTRL,
#endif
#if defined(SOC_ACPU_MEDIA1_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_MEDIA1,
#endif
#if defined(SOC_ACPU_MEDIA2_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_MEDIA2,
#endif
#if defined(SOC_ACPU_IOMCU_CONFIG_BASE_ADDR)
	HISI_CLOCK_CRG_IOMCU,
#endif
#if defined(SOC_ACPU_MMC1_SYS_CTRL_BASE_ADDR)
	HISI_CLOCK_CRG_MMC1,
#endif
#if defined(SOC_ACPU_MMC0_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_MMC0,
#endif
#if defined(SOC_ACPU_HSDT_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_HSDT,
#endif
#if defined(SOC_ACPU_HSDT1_CRG_BASE_ADDR)
	HISI_CLOCK_CRG_HSDT1,
#endif
#if defined(SOC_ACPU_SPMI_BASE_ADDR)
	HISI_CLOCK_CRG_PMU,
#endif
};

#define HISI_CRG_NAME_LEN 16
struct hisi_crg_region {
	unsigned int base_addr;
	int (*region_ok)(void);
	char crg_name[HISI_CRG_NAME_LEN];
};

const struct hisi_crg_region crg_regions[] = {
#if defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
	{SOC_ACPU_PERI_CRG_BASE_ADDR,      peri_power_up_ok,        "PERI"  },
#endif
#if defined(SOC_ACPU_SCTRL_BASE_ADDR)
	{SOC_ACPU_SCTRL_BASE_ADDR,         sctrl_power_up_ok,       "SCTRL" },
#endif
#if defined(SOC_ACPU_PMC_BASE_ADDR)
	{SOC_ACPU_PMC_BASE_ADDR,           pmctrl_power_up_ok,      "PMCTRL"},
#endif
#if defined(SOC_ACPU_PCTRL_BASE_ADDR)
	{SOC_ACPU_PCTRL_BASE_ADDR,         pctrl_power_up_ok,       "PCTRL" },
#endif
#if defined(SOC_ACPU_MEDIA1_CRG_BASE_ADDR)
	{SOC_ACPU_MEDIA1_CRG_BASE_ADDR,    media1_power_up_ok,      "MEDIA1"},
#endif
#if defined(SOC_ACPU_MEDIA2_CRG_BASE_ADDR)
	{SOC_ACPU_MEDIA2_CRG_BASE_ADDR,    media2_power_up_ok,      "MEDIA2"},
#endif
#if defined(SOC_ACPU_IOMCU_CONFIG_BASE_ADDR)
	{SOC_ACPU_IOMCU_CONFIG_BASE_ADDR,  iomcu_power_up_ok,       "IOMCU" },
#endif
#if defined(SOC_ACPU_MMC1_SYS_CTRL_BASE_ADDR)
	{SOC_ACPU_MMC1_SYS_CTRL_BASE_ADDR, mmc1sysctrl_power_up_ok, "MMC1"  },
#endif
#if defined(SOC_ACPU_MMC0_CRG_BASE_ADDR)
	{SOC_ACPU_MMC0_CRG_BASE_ADDR,      mmc0_power_up_ok,        "MMC0"  },
#endif
#if defined(SOC_ACPU_HSDT_CRG_BASE_ADDR)
	{SOC_ACPU_HSDT_CRG_BASE_ADDR,      hsdt_power_up_ok,        "HSDT"  },
#endif
#if defined(SOC_ACPU_HSDT1_CRG_BASE_ADDR)
	{SOC_ACPU_HSDT1_CRG_BASE_ADDR,     hsdt1_power_up_ok,       "HSDT1" },
#endif
#if defined(SOC_ACPU_SPMI_BASE_ADDR)
	{SOC_ACPU_SPMI_BASE_ADDR,          pmuspmi_power_up_ok,     "PMU"   },
#endif
};
const int crg_region_size = ARRAY_SIZE(crg_regions);

static int read_reg_u32(unsigned int hwaddr, unsigned int *value)
{
	void __iomem *reg = NULL;

	reg = REG_VADDR_MAP(hwaddr);
	if (!IS_ERR_OR_NULL(reg)) {
		if (value)
			*value = readl(reg);
		REG_VADDR_UNMAP(reg);
	} else {
		clklog_err("ioremap 0x%x error!\n", hwaddr);
		return -ENOMEM;
	}
	return 0;
}

static inline int update_line_offset(int offset, int count)
{
	if (count > 0)
		return offset + count;
	return offset;
}

static const char *get_clock_matched_state(int check_state)
{
	if (check_state == MATCH)
		return "[Y]";
	else if (check_state == PARTIAL_MATCH)
		return "[P]";
	else
		return "[N]";
}

#define HISI_CLOCK_PARENT_IDX_OFFSET  0x0000ffff
#define MAX_STR_BUF_SIZE              256
#define LINE_LEFT(offset)                      \
	(((MAX_STR_BUF_SIZE - (offset)) > 1) ? \
	(MAX_STR_BUF_SIZE - (offset)) : 1)
#define MAX_BIT_SIZE 32
#define MAX_LEVEL_NUM 16

struct transmit_info {
	char line[MAX_STR_BUF_SIZE];
	int offset;
};

static void print_clock_name(const struct clock_print_info *info,
	struct transmit_info *tran_info, int level)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, "%*s%-*s", level * 3 + 1,
			(info->parent_idx >= HISI_CLOCK_PARENT_IDX_OFFSET ? "*" : ""),
				50 - level * 3, info->clock_name); /* 50: length, 3: space */
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_type(const struct clock_print_info *info,
	struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, "%-13s", info->clock_type);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_rate(const struct clock_print_info *info,
	struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-10lu", info->rate);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_crg_region(const struct clock_print_info *info,
	struct transmit_info *tran_info, int i)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-6s%-5s", info->crg_region[i],
			(info->region_stat[i] ? "" : "[OFF]"));
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_default_reg(struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset,
		LINE_LEFT(tran_info->offset), LINE_LEFT(tran_info->offset) - 1,
			" %-17s %-10s", "NA", "NA");
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);
	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_reg_bit(const struct clock_print_info *info,
	struct transmit_info *tran_info, int i)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " 0x%08X[%2u:%-2u]",
			info->reg[i], info->h_bit[i], info->l_bit[i]);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_reg_value(const struct clock_print_info *info,
	struct transmit_info *tran_info, int i)
{
	int ret;

	if (i == 0) {
		ret = snprintf_s(tran_info->line + tran_info->offset,
			LINE_LEFT(tran_info->offset), LINE_LEFT(tran_info->offset) - 1,
				" 0x%08X", info->reg_val[i]);
	} else {
		ret = snprintf_s(tran_info->line + tran_info->offset,
			LINE_LEFT(tran_info->offset), LINE_LEFT(tran_info->offset) - 1,
				" 0x%08X\n", info->reg_val[i]);
	}
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_default_groups_reg(struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-11s %-17s %-10s",
			"NA", "NA", "NA");
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_enable_cnt(const struct clock_print_info *info,
	struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-6u", info->enable_cnt);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_prepare_cnt(const struct clock_print_info *info,
	struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-5u", info->prepare_cnt);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);

	tran_info->offset = update_line_offset(tran_info->offset, ret);
}

static void print_clock_check_state(const struct clock_print_info *info,
	struct transmit_info *tran_info)
{
	int ret;

	ret = snprintf_s(tran_info->line + tran_info->offset, LINE_LEFT(tran_info->offset),
		LINE_LEFT(tran_info->offset) - 1, " %-3s\n",
			get_clock_matched_state(info->check_state));
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);
}

static void print_clock_info(struct seq_file *s,
	const struct clock_print_info *info, int level)
{
	int ret, i;
	struct transmit_info tran_info;

	tran_info.offset = 0;
	if (level > MAX_LEVEL_NUM)
		return;

	(void)memset_s(tran_info.line, MAX_STR_BUF_SIZE, '\0', sizeof(tran_info.line));
	print_clock_name(info, &tran_info, level);
	print_clock_type(info, &tran_info);
	print_clock_rate(info, &tran_info);

	if (info->clock_reg_groups > 0) {
		print_clock_crg_region(info, &tran_info, 0);

		if (info->h_bit[0] >= MAX_BIT_SIZE ||
		    info->l_bit[0] >= MAX_BIT_SIZE) {
			print_clock_default_reg(&tran_info);
		} else {
			print_clock_reg_bit(info, &tran_info, 0);
			print_clock_reg_value(info, &tran_info, 0);
		}
	} else {
		print_clock_default_groups_reg(&tran_info);
	}

	print_clock_enable_cnt(info, &tran_info);
	print_clock_prepare_cnt(info, &tran_info);
	print_clock_check_state(info, &tran_info);

	seq_clklog(s, "%s", tran_info.line);

	for (i = 1; i < info->clock_reg_groups; i++) {
		tran_info.offset = 0;
		(void)memset_s(tran_info.line, MAX_STR_BUF_SIZE, '\0', sizeof(tran_info.line));
		ret = snprintf_s(tran_info.line + tran_info.offset, LINE_LEFT(tran_info.offset),
			LINE_LEFT(tran_info.offset) - 1, "%75s", "");
		if(ret == -1)
			pr_err("%s snprintf_s failed!\n", __func__);
		tran_info.offset = update_line_offset(tran_info.offset, ret);

		print_clock_crg_region(info, &tran_info, i);
		print_clock_reg_bit(info, &tran_info, i);
		print_clock_reg_value(info, &tran_info, i);

		seq_clklog(s, "%s", tran_info.line);
	}
}

#if defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
static int peri_power_up_ok(void)
{
	/* Peri should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_SCTRL_BASE_ADDR)
static int sctrl_power_up_ok(void)
{
	/* sctrl always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_PMC_BASE_ADDR)
static int pmctrl_power_up_ok(void)
{
	/* pmctrl should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_PCTRL_BASE_ADDR)
static int pctrl_power_up_ok(void)
{
	/* pctrl should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_MEDIA1_CRG_BASE_ADDR)
static int media1_power_up_ok(void)
{
	int ret = 0;
#if defined(SOC_CRGPERIPH_PERRSTSTAT5_ADDR) && \
	defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
	unsigned int value;
	unsigned int hwaddr;

	hwaddr = SOC_CRGPERIPH_PERRSTSTAT5_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR);
	ret = read_reg_u32(hwaddr, &value);
	if (ret)
		return ret;
#if defined(SOC_CRGPERIPH_PERRSTSTAT5_ip_rst_media_crg_START)
	ret = !(value & BIT(SOC_CRGPERIPH_PERRSTSTAT5_ip_rst_media_crg_START));
#endif
#endif
	return ret;
}
#endif

#if defined(SOC_ACPU_MEDIA2_CRG_BASE_ADDR)
static int media2_power_up_ok(void)
{
	int ret = 0;
#if defined(SOC_CRGPERIPH_PERRSTSTAT4_ADDR) && \
	defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
	unsigned int value;
	unsigned int hwaddr;

	hwaddr = SOC_CRGPERIPH_PERRSTSTAT4_ADDR(SOC_ACPU_PERI_CRG_BASE_ADDR);
	ret = read_reg_u32(hwaddr, &value);
	if (ret)
		return ret;
#if defined(SOC_CRGPERIPH_PERRSTSTAT4_ip_rst_media2_crg_START)
	ret = !(value & BIT(SOC_CRGPERIPH_PERRSTSTAT4_ip_rst_media2_crg_START));
#endif
#endif
	return ret;
}
#endif

#if defined(SOC_ACPU_IOMCU_CONFIG_BASE_ADDR)
static int iomcu_power_up_ok(void)
{
	/* iomcu is always on while system is normal */
	return 1;
}
#endif

#if defined(SOC_ACPU_MMC1_SYS_CTRL_BASE_ADDR)
static int mmc1sysctrl_power_up_ok(void)
{
	/* MMC1 should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_MMC0_CRG_BASE_ADDR)
static int mmc0_power_up_ok(void)
{
	/* MMC0 should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_HSDT_CRG_BASE_ADDR)
static int hsdt_power_up_ok(void)
{
	/* HSDT should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_HSDT1_CRG_BASE_ADDR)
static int hsdt1_power_up_ok(void)
{
	/* HSDT1 should be considered always on */
	return 1;
}
#endif

#if defined(SOC_ACPU_SPMI_BASE_ADDR)
static int pmuspmi_power_up_ok(void)
{
	/* PMU/SPMI always on */
	return 1;
}
#endif

static int get_base_hwaddr(struct device_node *np)
{
	struct device_node *parent = NULL;
	const unsigned int *in_addr = NULL;
	u64 hwaddr;
	int i;

	parent = of_get_parent(np);
	if (!parent) {
		clklog_err("node %s doesn't have parent node!\n", np->name);
		return -EINVAL;
	}

	in_addr = of_get_address(parent, 0, NULL, NULL);
	if (!in_addr) {
		clklog_err("node %s of_get_address get I/O addr err!\n",
			parent->name);
		return -EINVAL;
	}
	hwaddr = of_translate_address(parent, in_addr);
	if (hwaddr == OF_BAD_ADDR) {
		clklog_err("node %s of_translate_address err!\n",
			parent->name);
		return -EINVAL;
	}
	of_node_put(parent);

	for (i = 0; i < crg_region_size; i++) {
		if (crg_regions[i].base_addr == (unsigned int)hwaddr)
			return i;
	}
	return -ENODEV;
}

#define CHECK_REGION_IDX(region_idx) \
	(((region_idx) < 0) || ((region_idx) >= crg_region_size))

static int check_fixed_rate(struct clock_check_param *param)
{
	unsigned int fixed_rate = 0;
	unsigned long rate;
	struct device_node *np = NULL;
	int ret = MISMATCH;
	struct clock_print_info pinfo;

	np = param->np;
	if (of_property_read_u32(np, HISI_FIXED_RATE_ATTR, &fixed_rate)) {
		clklog_err("node %s doesn't have %s property!", np->name,
			HISI_FIXED_RATE_ATTR);
		return -EINVAL;
	}

	rate = clk_get_rate(param->clk->hw->clk);
	if (rate == (unsigned long)fixed_rate)
		ret = MATCH;

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_FIXED].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 0;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

static int __check_gate_state_matched(unsigned int reg_value,
	unsigned int gate_bitmask, unsigned int enable_count, bool always_on)
{
	int matched = MISMATCH;
	bool gate_onoff = ((reg_value & gate_bitmask) == gate_bitmask);
	bool enable_onoff = (enable_count > 0);

	if (gate_onoff && enable_onoff) {
		matched = MATCH;
	} else if (gate_onoff && !enable_onoff) {
		if (always_on)
			matched = MATCH;
		else
			matched = PARTIAL_MATCH;
	} else if (!gate_onoff && enable_onoff) {
		matched = MISMATCH;
	} else {
		matched = MATCH;
	}

	return matched;
}

#define GET_DVFS_DEVID_STR(is_dvfs_clock) \
	((is_dvfs_clock) ? "hisilicon,clk-devfreq-id" : "clock-id")
static void __print_dvfs_extra_info(struct clock_check_param *param,
	bool is_dvfs_clock)
{
	int ret;
	int offset = 0;
	int device_id = -1;
#if defined(SOC_ACPU_PMC_BASE_ADDR)
	unsigned int hwaddr_pmctrl;
#endif
	unsigned int ctrl4_data = 0;
	unsigned int ctrl5_data = 0;
	char line[MAX_STR_BUF_SIZE] = {0};

	if (of_property_read_u32(param->np, GET_DVFS_DEVID_STR(is_dvfs_clock),
		&device_id))
		return;

#if defined(SOC_ACPU_PMC_BASE_ADDR)
	hwaddr_pmctrl = crg_regions[HISI_CLOCK_CRG_PMCTRL].base_addr;
	(void)read_reg_u32(SOC_PMCTRL_PERI_CTRL4_ADDR(hwaddr_pmctrl),
		&ctrl4_data);
	(void)read_reg_u32(SOC_PMCTRL_PERI_CTRL5_ADDR(hwaddr_pmctrl),
		&ctrl5_data);
#endif

	(void)memset_s(line, MAX_STR_BUF_SIZE, '\0', sizeof(line));
	ret = snprintf_s(line + offset, LINE_LEFT(offset),
		LINE_LEFT(offset) - 1, "%76s", "");
	if (ret == -1)
		pr_err("%s default snprintf_s failed!\n", __func__);
	offset = update_line_offset(offset, ret);

	ret = snprintf_s(line + offset,
		LINE_LEFT(offset), LINE_LEFT(offset) - 1,
		"%-12s%-6dCTRL4:0x%08X CTRL5:0x%08X", "#DVFS-ID:",
		device_id, ctrl4_data, ctrl5_data);
	if (ret == -1)
		pr_err("%s device_id, ctrl4_data, ctrl5_data snprintf_s failed!\n",
			__func__);
	if (line[0] != '\0')
		seq_clklog(param->s, "%s\n", line);
}

static void __print_avs_extra_info(struct clock_check_param *param)
{
	int ret;
	int offset = 0;
	int avs_poll_id = -1;
#if defined(SOC_ACPU_SCTRL_BASE_ADDR) && defined(SOC_SCTRL_SCBAKDATA24_ADDR)
	unsigned int hwaddr_sctrl;
#endif
	unsigned int avs_data = 0;
	char line[MAX_STR_BUF_SIZE] = {0};

	if (of_property_read_u32(param->np, "hisilicon,clk-avs-id",
		&avs_poll_id))
		return;

#if defined(SOC_ACPU_SCTRL_BASE_ADDR) && defined(SOC_SCTRL_SCBAKDATA24_ADDR)
	hwaddr_sctrl = crg_regions[HISI_CLOCK_CRG_SCTRL].base_addr;
	(void)read_reg_u32(SOC_SCTRL_SCBAKDATA24_ADDR(hwaddr_sctrl),
		&avs_data);
#endif

	(void)memset_s(line, MAX_STR_BUF_SIZE, '\0', sizeof(line));
	ret = snprintf_s(line + offset, LINE_LEFT(offset),
		LINE_LEFT(offset) - 1, "%76s", "");
	if (ret == -1)
		pr_err("%s default snprintf_s failed!\n", __func__);
	offset = update_line_offset(offset, ret);

	ret = snprintf_s(line + offset, LINE_LEFT(offset),
		LINE_LEFT(offset) - 1,
		"%-12s%-6dAVS:  0x%08X", "#AVS-ID:", avs_poll_id, avs_data);
	if (ret == -1)
		pr_err("%s snprintf_s failed!\n", __func__);
	if (line[0] != '\0')
		seq_clklog(param->s, "%s\n", line);
}

static void print_dvfs_avs_extra_info(struct clock_check_param *param,
	bool is_dvfs_clock)
{
	struct device_node *np = NULL;
	unsigned int freq_table[DVFS_MAX_FREQ_NUM] = {0};
	unsigned int volt_table[DVFS_MAX_FREQ_NUM + 1] = {0};
	unsigned int sensitive_level = 0;
	char line[MAX_STR_BUF_SIZE] = {0};
	int offset = 0;
	int ret;
	unsigned int i;

	if (!IS_ENABLED(CONFIG_PERI_DVFS))
		return;

	np = param->np;
	if (!is_dvfs_clock && !of_property_read_bool(np, "peri_dvfs_sensitive"))
		return;
	if (of_property_read_u32(np, "hisilicon,clk-dvfs-level",
		&sensitive_level))
		return;
	if (of_property_read_u32_array(np, "hisilicon,sensitive-freq",
		&freq_table[0], sensitive_level))
		return;

	if (of_property_read_u32_array(np, "hisilicon,sensitive-volt",
		&volt_table[0], sensitive_level + 1))
		return;

	__print_dvfs_extra_info(param, is_dvfs_clock);
	__print_avs_extra_info(param);

	(void)memset_s(line, MAX_STR_BUF_SIZE, '\0', sizeof(line));
	ret = snprintf_s(line + offset, LINE_LEFT(offset),
		LINE_LEFT(offset) - 1, "%76s%-12s", "", "#DVFS-Freq:");
	if (ret == -1)
		pr_err("%s snprintf_s base failed!\n", __func__);
	offset = update_line_offset(offset, ret);

	for (i = 0; i < sensitive_level; i++) {
		ret = snprintf_s(line + offset, LINE_LEFT(offset),
			LINE_LEFT(offset) - 1, "%-6uKHz ", freq_table[i]);
		if(ret == -1)
			pr_err("%s snprintf_s freq_table failed!\n", __func__);
		offset = update_line_offset(offset, ret);
	}
	seq_clklog(param->s, "%s\n", line);

	offset = 0;
	(void)memset_s(line, MAX_STR_BUF_SIZE, '\0', sizeof(line));
	ret = snprintf_s(line + offset, LINE_LEFT(offset),
		LINE_LEFT(offset) - 1, "%76s%-12s", "", "#DVFS-Volt:");
	if (ret == -1)
		pr_err("%s snprintf_s base failed!\n", __func__);

	offset = update_line_offset(offset, ret);

	for (i = 0; i <= sensitive_level; i++) {
		ret = snprintf_s(line + offset, LINE_LEFT(offset),
			LINE_LEFT(offset) - 1, "VOLT_%-5u", volt_table[i]);
		if(ret == -1)
			pr_err("%s snprintf_s volt_table failed!\n", __func__);
		offset = update_line_offset(offset, ret);
	}
	seq_clklog(param->s, "%s\n", line);
}

static int print_info_init(struct clock_check_param *param,
	struct clock_print_info *ppinfo, unsigned int *pgdata, int length,
	const char *gate_type)
{
	unsigned long rate;
	struct device_node *np = NULL;
	int region_idx;
	int region_stat;
	unsigned int hwaddr;

	np = param->np;
	if (of_property_read_u32_array(np, gate_type, pgdata, length)) {
		clklog_err("node %s doesn't have %s property!",
			np->name, gate_type);
		return -EINVAL;
	}

	rate = clk_get_rate(param->clk->hw->clk);

	region_idx = get_base_hwaddr(np);
	if (CHECK_REGION_IDX(region_idx)) {
		clklog_err("Failed to get clk[%s] base addr!\n",
			param->clk->name);
		return -EINVAL;
	}

	region_stat = crg_regions[region_idx].region_ok();
	if (region_stat < 0) {
		clklog_err("check region %s err, err_code is %d\n",
			crg_regions[region_idx].crg_name, region_stat);
		return region_stat;
	}

	hwaddr = crg_regions[region_idx].base_addr + pgdata[0];

	ppinfo->clock_name         = param->clk->name;
	ppinfo->rate               = rate;
	ppinfo->clock_reg_groups   = 1;
	ppinfo->crg_region[0]      = crg_regions[region_idx].crg_name;
	ppinfo->region_stat[0]     = region_stat;
	ppinfo->reg[0]             = hwaddr;
	ppinfo->enable_cnt         = param->clk->enable_count;
	ppinfo->prepare_cnt        = param->clk->prepare_count;
	ppinfo->parent_idx         = param->parent_idx;

	return 0;
}


static int check_general_gate(struct clock_check_param *param)
{
	const unsigned int reg_stat_offset = 0x8; /* state offset */
	unsigned int gdata[2] = {0}; /* offset and efficient bits */
	struct device_node *np = NULL;
	int ret;
	int matched = MISMATCH;
	unsigned int value = 0;
	bool always_on = false;
	struct clock_print_info pinfo;

	np = param->np;
	if (of_property_read_bool(np, "always_on"))
		always_on = true;

	ret = print_info_init(param, &pinfo, gdata, 2,
		HISI_GENERAL_GATE_REG_ATTR);
	if (ret)
		return ret;

	/* crg region powered up */
	if (pinfo.region_stat[0] && gdata[1]) {
		ret = read_reg_u32(pinfo.reg[0] + reg_stat_offset, &value);
		if (ret)
			return ret;
		matched = __check_gate_state_matched(value, gdata[1],
			param->clk->enable_count, always_on);
	} else {
		/* fake gate clock Or crg region power down */
		matched = MATCH;
	}

	pinfo.clock_type         = clock_types[HISI_CLOCK_GENERAL_GATE].alias;
	pinfo.clock_reg_groups   = 1;
	pinfo.reg[0]             = pinfo.reg[0] + reg_stat_offset;
	pinfo.h_bit[0]           = (u8)(fls(gdata[1]) - 1);
	pinfo.l_bit[0]           = (u8)(ffs(gdata[1]) - 1);
	pinfo.reg_val[0]         = value;
	pinfo.check_state        = matched;
	print_clock_info(param->s, &pinfo, param->level);
	print_dvfs_avs_extra_info(param, false);

	return (matched == MISMATCH ? MISMATCH : MATCH);
}

static int check_himask_gate(struct clock_check_param *param)
{
	unsigned int gdata[2] = {0}; /* offset and efficient bits */
	struct device_node *np = NULL;
	int ret;
	int matched = MISMATCH;
	unsigned int value = 0;
	bool always_on = false;
	struct clock_print_info pinfo;

	np = param->np;
	if (!of_property_read_bool(np, HISI_HIMASK_ATTR)) {
		clklog_err("node %s doesn't have %s property!", np->name,
			HISI_HIMASK_ATTR);
		return -EINVAL;
	}

	if (of_property_read_bool(np, "always_on"))
		always_on = true;

	ret = print_info_init(param, &pinfo, gdata, 2,
		HISI_HIMASK_GATE_REG_ATTR);
	if (ret)
		return ret;

	/* crg region powered up */
	if (pinfo.region_stat[0]) {
		ret = read_reg_u32(pinfo.reg[0], &value);
		if (ret)
			return ret;
		matched = __check_gate_state_matched(value, BIT(gdata[1]),
			param->clk->enable_count, always_on);
	} else {
		/* crg region power down */
		matched = MATCH;
	}

	pinfo.clock_type         = clock_types[HISI_CLOCK_HIMASK_GATE].alias;
	pinfo.h_bit[0]           = gdata[1];
	pinfo.l_bit[0]           = gdata[1];
	pinfo.reg_val[0]         = value;
	pinfo.check_state        = matched;
	print_clock_info(param->s, &pinfo, param->level);

	return (matched == MISMATCH ? MISMATCH : MATCH);
}

static int __check_mux_state_matched(unsigned int reg_value,
	unsigned int mux_bitmask, unsigned int parent_idx)
{
	int matched = MISMATCH;
	unsigned int low_bit = (unsigned int)ffs(mux_bitmask);
	if (low_bit == 0)
		return -EINVAL;
	low_bit -= 1;

	if ((int)parent_idx >= HISI_CLOCK_PARENT_IDX_OFFSET)
		parent_idx -= HISI_CLOCK_PARENT_IDX_OFFSET;

	if ((reg_value & mux_bitmask) == (parent_idx << low_bit))
		matched = MATCH;

	return matched;
}

static int check_mux_clock(struct clock_check_param *param)
{
	unsigned int gdata[2] = {0}; /* offset and efficient bits */
	int ret;
	unsigned int value = 0;
	struct clock_print_info pinfo;

	ret = print_info_init(param, &pinfo, gdata, 2, HISI_MUX_REG_ATTR);
	if (ret)
		return ret;

	/* crg region powered up */
	if (pinfo.region_stat[0]) {
		ret = read_reg_u32(pinfo.reg[0], &value);
		if (ret)
			return ret;
		ret = __check_mux_state_matched(value, gdata[1],
			param->parent_idx);
	} else {
		/* crg region power down */
		ret = MATCH;
	}

	pinfo.clock_type         = clock_types[HISI_CLOCK_MUX].alias;
	pinfo.h_bit[0]           = (u8)(fls(gdata[1]) - 1);
	pinfo.l_bit[0]           = (u8)(ffs(gdata[1]) - 1);
	pinfo.reg_val[0]         = value;
	pinfo.check_state        = ret;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

static int __check_div_state_matched(unsigned int reg_value,
	unsigned int div_bitmask, unsigned long prate, unsigned long rate)
{
	int matched = MISMATCH;
	unsigned int low_bit = (unsigned int)ffs(div_bitmask);
	if (low_bit == 0)
		return -EINVAL;
	low_bit -= 1;

	if (!rate)
		return -EINVAL;

	if ((reg_value & div_bitmask) == ((prate / rate - 1) << low_bit))
		matched = MATCH;

	return matched;
}

static int check_div_clock(struct clock_check_param *param)
{
	unsigned int gdata[2] = {0}; /* offset and efficient bits */
	unsigned long prate, rate;
	struct clk_core *pclk = NULL;
	int ret;
	unsigned int value = 0;
	struct clock_print_info pinfo;

	pclk = __clk_core_get_parent(param->clk);
	if (IS_ERR_OR_NULL(pclk)) {
		clklog_err("failed to get clk[%s] parent clock!\n",
			param->clk->name);
		return -EINVAL;
	}
	rate = clk_get_rate(param->clk->hw->clk);
	prate = clk_get_rate(pclk->hw->clk);

	ret = print_info_init(param, &pinfo, gdata, 2, HISI_DIV_REG_ATTR);
	if (ret)
		return ret;

	/* crg region powered up */
	if (pinfo.region_stat[0]) {
		ret = read_reg_u32(pinfo.reg[0], &value);
		if (ret)
			return ret;
		ret = __check_div_state_matched(value, gdata[1], prate, rate);
	} else {
		/* crg region power down */
		ret = MATCH;
	}

	pinfo.clock_type         = clock_types[HISI_CLOCK_DIV].alias;
	pinfo.h_bit[0]           = (u8)(fls(gdata[1]) - 1);
	pinfo.l_bit[0]           = (u8)(ffs(gdata[1]) - 1);
	pinfo.reg_val[0]         = value;
	pinfo.check_state        = ret;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

static int check_fixed_div_clock(struct clock_check_param *param)
{
	unsigned long rate;
	unsigned long prate;
	unsigned int div = 1;
	unsigned int mult = 1;
	struct clk_core *pclk = NULL;
	struct device_node *np = NULL;
	int ret = MISMATCH;
	struct clock_print_info pinfo;

	np = param->np;
	if (of_property_read_u32(np, HISI_FIXED_DIV_ATTR1, &div)) {
		clklog_err("node %s doesn't have %s property!",
			np->name, HISI_FIXED_DIV_ATTR1);
		return -EINVAL;
	}
	if (of_property_read_u32(np, HISI_FIXED_DIV_ATTR2, &mult)) {
		clklog_err("node %s doesn't have %s property!",
			np->name, HISI_FIXED_DIV_ATTR2);
		return -EINVAL;
	}

	pclk = __clk_core_get_parent(param->clk);
	if (IS_ERR_OR_NULL(pclk)) {
		clklog_err("cannot get clk[%s] parent clock!\n",
			param->clk->name);
		return -EINVAL;
	}
	prate = clk_get_rate(pclk->hw->clk);
	rate = clk_get_rate(param->clk->hw->clk);
	if (!rate) {
		clklog_err("get clock[%s] rate is 0!\n", param->clk->name);
		return -EINVAL;
	}

	if (div != 0 && rate == (prate * mult / div))
		ret = MATCH;

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_FIXED_DIV].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 0;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

/* check PPLL configs */
#define CHECK_KIRIN_PLL_CONFIG(param, ret, lock_bit) do {             \
	(void)read_reg_u32(ppll_cfg.hwaddr_ctrl + ppll_cfg.pll_ctrl0_addr, &ppll_cfg.pll_ctrl0); \
	(void)read_reg_u32(ppll_cfg.hwaddr_ctrl + ppll_cfg.pll_ctrl1_addr, &ppll_cfg.pll_ctrl1); \
	(void)read_reg_u32(ppll_cfg.hwaddr + ppll_cfg.en_addr[0], &ppll_cfg.pll_en);             \
	(void)read_reg_u32(ppll_cfg.hwaddr + ppll_cfg.gt_addr[0], &ppll_cfg.pll_gt);             \
	(void)read_reg_u32(ppll_cfg.hwaddr + ppll_cfg.bp_addr[0], &ppll_cfg.pll_bp);             \
	if ((ppll_cfg.pll_en & BIT(ppll_cfg.en_addr[1])) &&                    \
	    (ppll_cfg.pll_gt & BIT(ppll_cfg.gt_addr[1])) &&                    \
	   !(ppll_cfg.pll_bp & BIT(ppll_cfg.bp_addr[1])) &&                    \
	    (ppll_cfg.pll_ctrl0 & BIT(lock_bit))) {                            \
		if ((param)->clk->enable_count > 0)                   \
			(ret) = MATCH;                                \
		else                                                  \
			(ret) = PARTIAL_MATCH;                        \
	} else {                                                      \
		if ((param)->clk->enable_count > 0)                   \
			(ret) = MISMATCH;                             \
		else                                                  \
			(ret) = MATCH;                                \
	}                                                             \
} while (0)

static int get_pll_ctrl_region_idx(struct device_node *np)
{
	int region_idx = -1;
	unsigned int pllctrl0_base = HS_PMCTRL;

	if (of_property_read_bool(np, "hisilicon,pll-ctrl0-base-reg")) {
		if (of_property_read_u32(np, "hisilicon,pll-ctrl0-base-reg",
			&pllctrl0_base)) {
			clklog_err("node %s doesn't have %s property!",
				np->name, HISI_PPLL_EN_ATTR);
			return -EINVAL;
		}
	}

	if (pllctrl0_base == HS_PMCTRL) {
#if defined(SOC_ACPU_PMC_BASE_ADDR)
		region_idx = HISI_CLOCK_CRG_PMCTRL;
#else
		/* No PMCTRL Region error */
		clklog_err("%s: No PMCTRL Region error!\n", __func__);
		return -EINVAL;
#endif
	} else if (pllctrl0_base == HS_CRGCTRL) {
#if defined(SOC_ACPU_PERI_CRG_BASE_ADDR)
		region_idx = HISI_CLOCK_CRG_PERI;
#else
		/* No CRGPERI Region error */
		clklog_err("%s: No CRGPERI Region error!\n", __func__);
		return -EINVAL;
#endif
	} else if (pllctrl0_base == HS_SYSCTRL) {
#if defined(SOC_ACPU_SCTRL_BASE_ADDR)
		region_idx = HISI_CLOCK_CRG_SCTRL;
#else
		/* No AO CRG Region error */
		clklog_err("%s: No AO CRG Region error!\n", __func__);
		return -EINVAL;
#endif
	} else {
		/* CRG Region error */
		clklog_err("%s CRG Region error!\n", __func__);
		return -EINVAL;
	}

	return region_idx;
}

#define __REG_ADDR_OFFSET 2
#define PLL_CTRL1_ADDR_SHIFT 0x4
struct ppll_check_param {
	unsigned int en_addr[__REG_ADDR_OFFSET];
	unsigned int gt_addr[__REG_ADDR_OFFSET];
	unsigned int bp_addr[__REG_ADDR_OFFSET];
	unsigned int pll_ctrl0_addr;
	unsigned int pll_ctrl1_addr;
	unsigned int hwaddr;
	unsigned int hwaddr_ctrl;
	unsigned int pll_ctrl0;
	unsigned int pll_ctrl1;
	unsigned int pll_en;
	unsigned int pll_gt;
	unsigned int pll_bp;
	int region_idx;
	int pll_ctrl_region_idx;
};

static int __read_ppll_clock_config(struct device_node *np,
	struct ppll_check_param *cfg)
{
	if (of_property_read_u32_array(np, HISI_PPLL_EN_ATTR,
		&cfg->en_addr[0], __REG_ADDR_OFFSET)) {
		clklog_debug("node %s doesn't have %s property!",
			np->name, HISI_PPLL_EN_ATTR);
		return -EINVAL;
	}
	if (of_property_read_u32_array(np, HISI_PPLL_GT_ATTR,
		&cfg->gt_addr[0], __REG_ADDR_OFFSET)) {
		clklog_debug("node %s doesn't have %s property!",
			np->name, HISI_PPLL_GT_ATTR);
		return -EINVAL;
	}
	if (of_property_read_u32_array(np, HISI_PPLL_BP_ATTR,
		&cfg->bp_addr[0], __REG_ADDR_OFFSET)) {
		clklog_debug("node %s doesn't have %s property!",
			np->name, HISI_PPLL_BP_ATTR);
		return -EINVAL;
	}
	if (of_property_read_u32(np, HISI_PPLL_CTRL0_ATTR,
		&cfg->pll_ctrl0_addr)) {
		clklog_debug("node %s doesn't have %s property!",
			np->name, HISI_PPLL_CTRL0_ATTR);
		return -EINVAL;
	}
	cfg->pll_ctrl1_addr = cfg->pll_ctrl0_addr + PLL_CTRL1_ADDR_SHIFT;

	return 0;
}

static void __fill_pinfo(struct clock_print_info *pinfo,
	struct ppll_check_param *cfg)
{
	pinfo->clock_reg_groups = CLOCK_REG_GROUPS;

	/* the first group regs */
	pinfo->crg_region[0]  = crg_regions[cfg->region_idx].crg_name;
	pinfo->region_stat[0] = 1;
	pinfo->reg[0]         = cfg->hwaddr + cfg->en_addr[0];
	pinfo->h_bit[0]       = cfg->en_addr[1];
	pinfo->l_bit[0]       = cfg->en_addr[1];
	pinfo->reg_val[0]     = cfg->pll_en;

	/* the second group regs */
	pinfo->crg_region[1]  = crg_regions[cfg->region_idx].crg_name;
	pinfo->region_stat[1] = 1;
	pinfo->reg[1]         = cfg->hwaddr + cfg->bp_addr[0];
	pinfo->h_bit[1]       = cfg->bp_addr[1];
	pinfo->l_bit[1]       = cfg->bp_addr[1];
	pinfo->reg_val[1]     = cfg->pll_bp;

	/* the third group regs */
	pinfo->crg_region[2]  = crg_regions[cfg->region_idx].crg_name;
	pinfo->region_stat[2] = 1;
	pinfo->reg[2]         = cfg->hwaddr + cfg->gt_addr[0];
	pinfo->h_bit[2]       = cfg->gt_addr[1];
	pinfo->l_bit[2]       = cfg->gt_addr[1];
	pinfo->reg_val[2]     = cfg->pll_gt;

	/* the fourth group regs */
	pinfo->crg_region[3]  = crg_regions[cfg->pll_ctrl_region_idx].crg_name;
	pinfo->region_stat[3] = 1;
	pinfo->reg[3]         = cfg->hwaddr_ctrl + cfg->pll_ctrl0_addr;
	pinfo->h_bit[3]       = 31;
	pinfo->l_bit[3]       = 0;
	pinfo->reg_val[3]     = cfg->pll_ctrl0;

	/* the fifth group regs */
	pinfo->crg_region[4]  = crg_regions[cfg->pll_ctrl_region_idx].crg_name;
	pinfo->region_stat[4] = 1;
	pinfo->reg[4]         = cfg->hwaddr_ctrl + cfg->pll_ctrl1_addr;
	pinfo->h_bit[4]       = 31;
	pinfo->l_bit[4]       = 0;
	pinfo->reg_val[4]     = cfg->pll_ctrl1;
}

static int check_kirin_ppll_clock(struct clock_check_param *param)
{
	struct ppll_check_param ppll_cfg;
	const unsigned int pll_lock_bit = 26; /* PLL lock state bit */
	int ret = MISMATCH;
	struct device_node *np = NULL;
	struct clock_print_info pinfo = {0};

	memset(&ppll_cfg, 0, sizeof(struct ppll_check_param));
	np = param->np;
	pinfo.clock_reg_groups = 0;
	if (__read_ppll_clock_config(np, &ppll_cfg) < 0)
		goto out;

	ppll_cfg.region_idx = get_base_hwaddr(np);
	if (CHECK_REGION_IDX(ppll_cfg.region_idx)) {
		clklog_err("Failed to get clk[%s] base addr!\n",
			param->clk->name);
		return -EINVAL;
	}

	ppll_cfg.pll_ctrl_region_idx = get_pll_ctrl_region_idx(np);
	if (ppll_cfg.pll_ctrl_region_idx < 0)
		return -EINVAL;
	ppll_cfg.hwaddr_ctrl =
		crg_regions[ppll_cfg.pll_ctrl_region_idx].base_addr;
	ppll_cfg.hwaddr = crg_regions[ppll_cfg.region_idx].base_addr;
	CHECK_KIRIN_PLL_CONFIG(param, ret, pll_lock_bit);
	pinfo.clock_reg_groups = CLOCK_REG_GROUPS;

out:
	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_PPLL].alias;
	pinfo.rate               = clk_get_rate(param->clk->hw->clk);
	if (pinfo.clock_reg_groups == CLOCK_REG_GROUPS)
		__fill_pinfo(&pinfo, &ppll_cfg);
	else if (pinfo.clock_reg_groups == 0)
		ret = MATCH;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return (ret == MISMATCH ? MISMATCH : MATCH);
}

static int check_hisi_pll_clock(struct clock_check_param *param)
{
	unsigned int pll_ctrl0 = 0;
	unsigned int pll_ctrl1 = 0;
	unsigned int pll_lock_stat = 0;
	unsigned int hwaddr = 0;
	int region_idx = 0;
	int region_stat = 0;
	struct hi3xxx_ppll_clk *ppll_clk = NULL;
	struct clock_print_info pinfo = {0};

	ppll_clk = container_of(param->clk->hw, struct hi3xxx_ppll_clk, hw);
	pinfo.clock_reg_groups = 0;
	if (ppll_clk->en_cmd[1] != SCPLL)
		goto out;

	region_idx = get_base_hwaddr(param->np);
	if (CHECK_REGION_IDX(region_idx)) {
		clklog_err("Failed to get clk[%s] base addr!\n",
			param->clk->name);
		return -EINVAL;
	}

	region_stat = crg_regions[region_idx].region_ok();
	if (region_stat < 0) {
		clklog_err("check region %s err, err_code is %d\n",
			crg_regions[region_idx].crg_name, region_stat);
		return region_stat;
	}

	hwaddr = crg_regions[region_idx].base_addr;
	if (region_stat) {
		pinfo.clock_reg_groups = 3; /* register groups */
		pll_ctrl0 = readl(SCPLL_GT_ACPU_ADDR(ppll_clk->addr));
		pll_ctrl1 = readl(SCPLL_EN_ACPU_ADDR(ppll_clk->addr));
		pll_lock_stat = readl(SCPLL_BP_ACPU_ADDR(ppll_clk->addr));
	}

out:
	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_PLL].alias;
	pinfo.rate               = clk_get_rate(param->clk->hw->clk);

	/* the first group regs */
	pinfo.crg_region[0]      = crg_regions[region_idx].crg_name;
	pinfo.region_stat[0]     = region_stat;
#if defined(SOC_HSDT_CRG_PCIEPLL_CTRL0_pciepll_en_START)
	pinfo.reg[0]             = SCPLL_EN_ACPU_ADDR(hwaddr);
#endif
	pinfo.h_bit[0]           = 31;
	pinfo.l_bit[0]           = 0;
	pinfo.reg_val[0]         = pll_ctrl0;

	/* the second group regs */
	pinfo.crg_region[1]      = crg_regions[region_idx].crg_name;
	pinfo.region_stat[1]     = region_stat;
#if defined(SOC_HSDT_CRG_PCIEPLL_CTRL0_pciepll_en_START)
	pinfo.reg[1]             = SCPLL_GT_ACPU_ADDR(hwaddr);
#endif
	pinfo.h_bit[1]           = 31;
	pinfo.l_bit[1]           = 0;
	pinfo.reg_val[1]         = pll_ctrl1;

	/* the third group regs */
	pinfo.crg_region[2]      = crg_regions[region_idx].crg_name;
	pinfo.region_stat[2]     = region_stat;
#if defined(SOC_HSDT_CRG_PCIEPLL_CTRL0_pciepll_en_START)
	pinfo.reg[2]             = SCPLL_LOCK_STAT(hwaddr);
#endif
	pinfo.h_bit[2]           = 31;
	pinfo.l_bit[2]           = 0;
	pinfo.reg_val[2]         = pll_lock_stat;

	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = MATCH;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return MATCH;
}

static int check_pmu_clock(struct clock_check_param *param)
{
#if defined(SOC_ACPU_SPMI_BASE_ADDR)
	unsigned int gdata[2] = {0}; /* offset and efficient bits */
	unsigned long rate;
	struct device_node *np = NULL;
	int matched = MISMATCH;
	int region_idx;
	unsigned int hwaddr;
	unsigned int value;
	struct clock_print_info pinfo;

	np = param->np;
	if (!of_property_read_bool(np, HISI_PMU_ATTR)) {
		clklog_err("node %s doesn't have %s property!",
			np->name, HISI_PMU_ATTR);
		return -EINVAL;
	}
	if (of_property_read_u32_array(np, HISI_HIMASK_GATE_REG_ATTR,
		&gdata[0], 2)) {
		clklog_err("node %s doesn't have %s property!",
			np->name, HISI_HIMASK_GATE_REG_ATTR);
		return -EINVAL;
	}

	rate = clk_get_rate(param->clk->hw->clk);
	if (!rate) {
		clklog_err("get clock[%s] rate is 0!\n", param->clk->name);
		return -EINVAL;
	}

	region_idx = get_base_hwaddr(np);
	if (CHECK_REGION_IDX(region_idx)) {
		clklog_err("get clk[%s] [SPMI/PMU: 0x%x] base addr wrong!\n",
			param->clk->name,
			crg_regions[HISI_CLOCK_CRG_PMU].base_addr);
		region_idx = HISI_CLOCK_CRG_PMU;
	}

	hwaddr = crg_regions[region_idx].base_addr;
	value = pmic_read_reg(gdata[0]);
	matched = __check_gate_state_matched(value, BIT(gdata[1]),
		param->clk->enable_count, false);

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_PMU].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 1;
	pinfo.crg_region[0]      = crg_regions[region_idx].crg_name;
	pinfo.region_stat[0]     = 1;
	pinfo.reg[0]             = gdata[0];
	pinfo.h_bit[0]           = gdata[1];
	pinfo.l_bit[0]           = gdata[1];
	pinfo.reg_val[0]         = value;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = matched;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return (matched == MISMATCH ? MISMATCH : MATCH);
#else
	return MATCH;
#endif
}

static int check_ipc_clock(struct clock_check_param *param)
{
	int ret = MATCH;
	unsigned long rate;
	struct clock_print_info pinfo;

	rate = clk_get_rate(param->clk->hw->clk);

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_IPC].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 0;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

static int check_root_clock(struct clock_check_param *param)
{
	int ret = MATCH;
	unsigned long rate;
	struct clock_print_info pinfo;

	rate = clk_get_rate(param->clk->hw->clk);

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_ROOT].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 0;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);

	return ret;
}

static int hisi_clock_check(struct seq_file *s, struct clk_core *clk,
	unsigned int level, int parent_idx)
{
	int ret = 0;
	int i;
	struct clock_check_param param;
	const char *compatible = NULL;

	param.s = s;
	param.clk = clk;
	param.level = (int)level;
	param.parent_idx = parent_idx;
	param.np = of_find_node_by_name(NULL, clk->name);
	if (IS_ERR_OR_NULL(param.np)) {
		clklog_err("node %s doesn't find!\n", clk->name);
		return -EINVAL;
	}

	if (of_property_read_string(param.np, "compatible", &compatible)) {
		clklog_err("node %s doesn't have compatible property!\n",
			param.np->name);
		return -EINVAL;
	}

	for (i = 0; i < clock_type_size; i++) {
		if (strcmp(compatible, clock_types[i].name) == 0) {
			if (clock_types[i].check_func)
				ret = clock_types[i].check_func(&param);
			return ret;
		}
	}

	clklog_err("clk %s doesn't find!\n", clk->name);
	return -ENODEV;
}

static int find_hisi_clock_parent_idx(struct clk_core *clk)
{
	int i;
	struct clk_core *parent = NULL;

	if (IS_ERR_OR_NULL(clk))
		return -1;

	if (clk->num_parents == 1)
		return 0;

	for (i = 0; i < clk->num_parents; i++) {
		parent = __clk_core_get_parent(clk);
		if (parent == NULL) {
			clklog_err("hisi clock struct err!\n");
			return -EINVAL;
		}

		if (strcmp(parent->name, clk->parents[i].name) == 0)
			return i;
	}

	return 0;
}

static int find_all_clock_sources(struct seq_file *s, struct clk_core *clk,
	unsigned int level, int parent_idx)
{
	int ret = 0;
	int i;
	struct clk_core *parent = NULL;

	if (IS_ERR_OR_NULL(clk))
		return -EINVAL;

	/* Consistency check and print clk state */
	ret += hisi_clock_check(s, clk, level, parent_idx);

	/* recursive finding all source clock sources */
	for (i = 0; i < clk->num_parents; i++) {
		struct clk *tclk = NULL;
		int pp_idx = 0;

		parent = __clk_core_get_parent(clk);
		if (parent == NULL) {
			clklog_err("hisi clock struct err!\n");
			return -EINVAL;
		}

		if (((i == 0) && (clk->parents[i].name == NULL)) ||
		    (strcmp(parent->name, clk->parents[i].name) == 0)) {
			if (parent_idx >= HISI_CLOCK_PARENT_IDX_OFFSET)
				pp_idx = HISI_CLOCK_PARENT_IDX_OFFSET;
		} else if (strcmp("clk_invalid", clk->parents[i].name) == 0) {
			continue; /* ignore clk_invalid */
		} else {
			tclk = __clk_lookup(clk->parents[i].name);
			parent = (tclk != NULL) ? tclk->core : NULL;
		}

		pp_idx += find_hisi_clock_parent_idx(parent);
		ret += find_all_clock_sources(s, parent, level + 1, pp_idx);
	}

	return ret;
}

static int __find_single_clock_sources(struct seq_file *s,
	struct clk_core *clk, unsigned int level)
{
	int i;

	if (clk->num_parents <= 0) /* Consistency check and print clk state */
		return hisi_clock_check(s, clk, level, -1);
	else if (clk->num_parents == 1)
		return hisi_clock_check(s, clk, level, 0);

	/* traversal all parent clocks */
	for (i = 0; i < clk->num_parents; i++) {
		struct clk_core *pclk = __clk_core_get_parent(clk);
		if (!IS_ERR_OR_NULL(pclk) &&
			strcmp(pclk->name, clk->parents[i].name) == 0)
			/*
			 * Consistency check and print clk state
			 */
			return hisi_clock_check(s, clk, level, i);
	}
	return 0;
}

static int find_single_clock_sources(struct seq_file *s, struct clk_core *clk,
	unsigned int level)
{
	int ret = 0;

	while (!IS_ERR_OR_NULL(clk)) {
		ret += __find_single_clock_sources(s, clk, level);

		/* upward to clock source */
		clk = __clk_core_get_parent(clk);
		level++;
	}

	return ret;
}

static int __find_hisi_clock_path(struct seq_file *s, struct clk_core *clk,
	bool leading, bool recur)
{
	const char *ONE_LINE_STR =
		"---------------------------------------------";
	int ret;

	if (IS_ERR_OR_NULL(clk))
		return -EINVAL;

	/* leading of table */
	if (leading) {
		seq_clklog(s, "%14s%47s%8s%13s%9s%24s%8s%6s%6s\n",
			"clock name", "clock type", "rate", "region",
			"reg", "reg value", "en_cnt", "p_cnt", "state");
		seq_clklog(s, "%s%s%s\n", ONE_LINE_STR, ONE_LINE_STR,
			ONE_LINE_STR);
	}

	if (recur)
		ret = find_all_clock_sources(s, clk, 0,
			find_hisi_clock_parent_idx(clk) +
			HISI_CLOCK_PARENT_IDX_OFFSET);
	else
		ret = find_single_clock_sources(s, clk, 0);

	/* last of table */
	if (leading) {
		seq_clklog(s, "%s%s%s\n", ONE_LINE_STR, ONE_LINE_STR,
			ONE_LINE_STR);
		seq_clklog(s, "[+  CLK: %s +][+ RET: %d +][+ RESULT: %s +]\n",
			clk->name, ret, (ret == 0 ? "Success" : "Failure"));
	}

	return ret;
}

static int find_hisi_clock_path(struct seq_file *s, const char *clock_name,
	bool leading, bool recur)
{
	struct clk_core *clk = NULL;

	if (IS_ERR_OR_NULL(clock_name))
		return -EINVAL;

	clk = hisi_find_clock(clock_name);
	if (IS_ERR_OR_NULL(clk)) {
		clklog_err("clock %s doesn't find!\n", clock_name);
		return -EINVAL;
	}

	return __find_hisi_clock_path(s, clk, leading, recur);
}

static int check_dvfs_clock(struct clock_check_param *param)
{
	int ret = MATCH;
	unsigned long rate;
	struct clock_print_info pinfo;
	const char *clk_friend = NULL;

	rate = clk_get_rate(param->clk->hw->clk);

	pinfo.clock_name         = param->clk->name;
	pinfo.clock_type         = clock_types[HISI_CLOCK_DVFS].alias;
	pinfo.rate               = rate;
	pinfo.clock_reg_groups   = 0;
	pinfo.enable_cnt         = param->clk->enable_count;
	pinfo.prepare_cnt        = param->clk->prepare_count;
	pinfo.check_state        = ret;
	pinfo.parent_idx         = param->parent_idx;
	print_clock_info(param->s, &pinfo, param->level);
	print_dvfs_avs_extra_info(param, true);

	if (of_property_read_string(param->np, "clock-friend-names",
		&clk_friend)) {
		clklog_err("DVFS clock %s no friend clock!\n",
			param->clk->name);
		return -EINVAL;
	}

	return find_hisi_clock_path(param->s, clk_friend, false,
		(param->parent_idx >= HISI_CLOCK_PARENT_IDX_OFFSET));
}

/* ***************** clock test plan begin ******************** */
#define MAX_CLOCK_RATE_NUM     8
#define MAX_CLOCK_NAME_LEN     64
struct clock_test_plan {
	char clock_name[MAX_CLOCK_NAME_LEN];
	unsigned long rates[MAX_CLOCK_RATE_NUM + 1];
};
static const char *clock_test_plan_help =
	"\n[Help Tips]:\nPlease use test_plan debugfs as follows:\n"
	"1) echo <clkname> [rate1] [rate2]...> /d/clock/test_one_clock/test_plan\n"
	"                       {NOTE: only support max to 8 clock rates!}\n"
	"2) cat /d/clock/test_one_clock/test_plan\n\n";
static DEFINE_MUTEX(clock_plan_lock);
static struct clock_test_plan *clock_plan;

#define CHECK_FIND_CLOCK_PATH(_s, _rc)  do {                          \
	if (_rc) {                                                    \
		ret = 1;                                              \
		seq_clklog((_s), "clock[%s] path check err! rc=%d\n", \
			plan->clock_name, (_rc));                     \
	}                                                             \
} while (0)

static int test_clock_plan(struct seq_file *s, struct clock_test_plan *plan)
{
	struct clk_core *clk = NULL;
	int ret;
	int rc;
	unsigned int i;
	unsigned long old_rate;

	clk = hisi_find_clock(plan->clock_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_clklog(s, "find clock: %s err!\n", plan->clock_name);
		return -EINVAL;
	}

	old_rate = clk_get_rate(clk->hw->clk);
	seq_clklog(s, "clock[%s] get old_rate[%lu]\n", plan->clock_name,
		old_rate);

	seq_clklog(s, "clock[%s] prepare and enable\n", plan->clock_name);
	ret = clk_prepare_enable(clk->hw->clk);
	if (ret) {
		seq_clklog(s, "clock[%s] prepare and enable err! ret=%d\n",
			plan->clock_name, ret);
		return ret;
	}
	rc = find_hisi_clock_path(s, plan->clock_name, true, false);
	CHECK_FIND_CLOCK_PATH(s, rc);

	for (i = 0; i <= MAX_CLOCK_RATE_NUM; i++) {
		if (!plan->rates[i]) {
			/* if last rate is equal to old_rate, ignore */
			if (i > 0 && plan->rates[i - 1] != old_rate)
				plan->rates[i] = old_rate;
			break;
		}
	}
	for (i = 0; i <= MAX_CLOCK_RATE_NUM && plan->rates[i]; i++) {
		seq_clklog(s, "********CLOCK[%s] RATE[%lu] TESTING ********\n",
			plan->clock_name, plan->rates[i]);
		rc = clk_set_rate(clk->hw->clk, plan->rates[i]);
		if (rc < 0) {
			ret = 1;
			seq_clklog(s, "clock[%s] set rate %lu err! rc = %d\n",
				plan->clock_name, plan->rates[i], rc);
		}
		rc = find_hisi_clock_path(s, plan->clock_name, true, false);
		CHECK_FIND_CLOCK_PATH(s, rc);
	}

	seq_clklog(s, "clock[%s] disable and unprepare:\n", plan->clock_name);
	clk_disable_unprepare(clk->hw->clk);
	rc = find_hisi_clock_path(s, plan->clock_name, true, false);
	CHECK_FIND_CLOCK_PATH(s, rc);

	return ret;
}

static int clk_testplan_show(struct seq_file *s, void *data)
{
	int ret;

	mutex_lock(&clock_plan_lock);
	if (IS_ERR_OR_NULL(clock_plan)) {
		seq_clklog(s, "No clock test plan buffered!\n");
		seq_clklog(s, "%s", clock_test_plan_help);
		goto out;
	}
	ret = test_clock_plan(s, clock_plan);
	if (ret < 0)
		goto out;

	if (s->size > 0 && s->size == s->count) {
		clklog_info("resize the seq_file buffer size (%u -> %u)!\n",
			(u32)s->size, (u32)s->size * 2);
		goto end;
	}

out:
	kfree(clock_plan);
	clock_plan = NULL;

end:
	mutex_unlock(&clock_plan_lock);

	return 0;
}

int clk_testplan_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_testplan_show, inode->i_private);
}

static int __analysis_clock_test_plan_string(char *str, int ret_val)
{
	char *t_str = NULL;
	int i;
	unsigned long rate;

	for (i = 0; str != NULL && *str && i <= MAX_CLOCK_RATE_NUM; i++) {
		t_str = strchr(str, ' ');
		if (t_str != NULL) {
			*t_str = '\0';
			t_str++;
		}

		if (i == 0) {
			if (strcpy_s(clock_plan->clock_name, MAX_CLOCK_NAME_LEN,
				str) != EOK) {
				clklog_err("clock name %s is too long!\n", str);
				return -EINVAL;
			}
			clklog_info("clock name: %s\n", clock_plan->clock_name);
		} else {
			/* at most 10 digits */
			rate = simple_strtoul(str, NULL, 10);
			if (!rate) {
				clklog_err("please input valid clk rate!\n");
				return -EINVAL;
			}
			clock_plan->rates[i - 1] = rate;
			clklog_info("clock rate-%d: %lu\n", i,
				clock_plan->rates[i - 1]);
		}
		str = t_str;
	}

	return ret_val;
}

#define MAX_STRING_LEN 128
ssize_t clk_testplan_write(struct file *filp, const char __user *ubuf,
	size_t cnt, loff_t *ppos)
{
	char *clkname_freq_str = NULL;
	int ret = (int)cnt;

	if (cnt == 0 || !ubuf) {
		clklog_err("Input string is NULL\n");
		return -EINVAL;
	}

	if (cnt >= MAX_STRING_LEN) {
		clklog_err("Input string is too long\n");
		return -EINVAL;
	}

	mutex_lock(&clock_plan_lock);
	if (!clock_plan) {
		clock_plan = kzalloc(sizeof(struct clock_test_plan),
			GFP_KERNEL);
		if (!clock_plan) {
			ret = -EINVAL;
			goto out;
		}
	} else {
		clklog_err("clock %s does not finish test_plan!\n",
			clock_plan->clock_name);
		goto out;
	}

	clkname_freq_str = kzalloc(cnt * sizeof(char), GFP_KERNEL);
	if (!clkname_freq_str) {
		ret = -EINVAL;
		goto out;
	}

	/* copy clock name from user space(the last symbol is newline). */
	if (copy_from_user(clkname_freq_str, ubuf, cnt - 1)) {
		clklog_err("Input string is too long\n");
		ret = -EINVAL;
		goto out;
	}
	clkname_freq_str[cnt - 1] = '\0';
	ret = __analysis_clock_test_plan_string(clkname_freq_str, cnt);

out:
	mutex_unlock(&clock_plan_lock);
	if (clkname_freq_str)
		kfree(clkname_freq_str);
	return ret;
}

#ifdef CONFIG_DFX_DEBUG_FS
const struct file_operations clk_testplan_fops = {
	.open    = clk_testplan_single_open,
	.read    = seq_read,
	.write   = clk_testplan_write,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif
/* ***************** clock test plan end ******************** */

/* ***************** get clock path begin ******************** */
static DEFINE_MUTEX(leaf_clock_name_lock);
static const int MAX_CLKNAME_LEN = 128;
static char *leaf_clock_name;
static const char *clock_get_path_help =
	"\nHelp Tips:\nPlease use get_path debugfs as follows:\n"
	"1) echo {clock name} [string] > /d/clock/test_one_clock/get_path\n"
	"2) cat /d/clock/test_one_clock/get_path\n\n";

static int clk_getpath_show(struct seq_file *s, void *data)
{
	int rc;
	bool recur = false;
	char *t_str = NULL;

	mutex_lock(&leaf_clock_name_lock);
	if (!leaf_clock_name) {
		seq_clklog(s, "No clock name buffered!\n");
		seq_clklog(s, "%s", clock_get_path_help);
		goto out;
	}

	t_str = strchr(leaf_clock_name, ' ');
	if (t_str != NULL) {
		*t_str = '\0';
		recur = true;
	}
	rc = find_hisi_clock_path(s, leaf_clock_name, true, recur);
	if (rc < 0)
		clklog_err("clock path check fail: rc = %d!\n", rc);
	if (s->size > 0 && s->size == s->count) {
		clklog_info("resize the seq_file buffer size (%u -> %u)!\n",
			(u32)s->size, (u32)s->size * 2); /* 2: times */
		/* restore leaf_clock_name string */
		if (t_str != NULL)
			*t_str = ' ';
		goto end;
	}

out:
	kfree(leaf_clock_name);
	leaf_clock_name = NULL;

end:
	mutex_unlock(&leaf_clock_name_lock);
	return 0;
}

int clk_getpath_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_getpath_show, inode->i_private);
}

ssize_t clk_getpath_write(struct file *filp, const char __user *ubuf,
	size_t cnt, loff_t *ppos)
{
	int ret = (int)cnt;

	if (cnt == 0 || !ubuf) {
		clklog_err("Input string is NULL\n");
		return -EINVAL;
	}

	if (cnt >= MAX_CLKNAME_LEN) {
		clklog_err("Input string is too long\n");
		return -EINVAL;
	}

	mutex_lock(&leaf_clock_name_lock);
	if (leaf_clock_name == NULL) {
		leaf_clock_name = kzalloc(sizeof(char) * MAX_CLKNAME_LEN,
			GFP_KERNEL);
		if (leaf_clock_name == NULL) {
			ret = -EINVAL;
			goto out;
		}
	} else {
		clklog_err("clock %s does not finish get_path!\n",
			leaf_clock_name);
		goto out;
	}

	/* copy clock name from user space(the last symbol is newline). */
	if (copy_from_user(leaf_clock_name, ubuf, cnt - 1)) {
		clklog_err("Input string is too long\n");
		ret = -EINVAL;
		goto out;
	}
	clklog_info("clock buffer: %s\n", leaf_clock_name);
out:
	mutex_unlock(&leaf_clock_name_lock);
	return ret;
}

#ifdef CONFIG_DFX_DEBUG_FS
const struct file_operations clk_getpath_fops = {
	.open    = clk_getpath_single_open,
	.read    = seq_read,
	.write   = clk_getpath_write,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif
/* ***************** get clock path end ******************** */

void create_clk_test_debugfs(struct dentry *pdentry)
{
	if (!pdentry)
		return;
	debugfs_create_file("get_path", PRIV_AUTH, pdentry, NULL,
		&clk_getpath_fops);
	debugfs_create_file("clk_testplan", PRIV_AUTH, pdentry, NULL,
		&clk_testplan_fops);
}
