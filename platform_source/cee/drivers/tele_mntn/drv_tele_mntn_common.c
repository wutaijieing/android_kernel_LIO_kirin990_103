/*
 * drv_tele_mntn_common.c
 *
 * tele mntn module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#include <drv_tele_mntn_common.h>
#include <drv_tele_mntn_platform.h>

static TELE_MNTN_DATA_SECTION u8 *g_tele_mntn_sctrl_base;
static TELE_MNTN_DATA_SECTION u8 *g_tele_mntn_rtc0_base;

static TELE_MNTN_DATA_SECTION struct acore_tele_mntn_stru_s *g_acore_tele_mntn;
static TELE_MNTN_DATA_SECTION struct lpmcu_tele_mntn_stru_s *g_lpmcu_tele_mntn;

struct lpmcu_tele_mntn_stru_s *lpmcu_tele_mntn_get(void)
{
	return g_lpmcu_tele_mntn;
}

struct acore_tele_mntn_stru_s *acore_tele_mntn_get(void)
{
	return g_acore_tele_mntn;
}

/*
 * Function name: tele_mntn_is_func_on
 * Description: on-off enabling judgement
 * Input:       type_id: type which need to save log
 * Output:      NA
 * Return:      true enable
 *              false disable
 */
bool tele_mntn_is_func_on(enum tele_mntn_type_id type_id)
{
	unsigned int onoff;
	unsigned int id = (unsigned int)type_id;

	if (g_tele_mntn_sctrl_base == NULL || id >= U64_BITS_LIMIT)
		return false;
	if (id >= U32_BITS_LIMIT) {
		onoff = readl((void *)SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base));
		id -= U32_BITS_LIMIT;
	} else {
		onoff = readl((void *)SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base));
	}
	if ((onoff & BIT(id)) != 0)
		return true;

	return false;
}

int tele_mntn_common_init(void)
{
	g_tele_mntn_sctrl_base =
		(unsigned char *)ioremap(SOC_ACPU_SCTRL_BASE_ADDR, TELE_MNTN_AXI_SIZE);
	g_tele_mntn_rtc0_base =
		(unsigned char *)ioremap(SOC_ACPU_RTC0_BASE_ADDR, TELE_MNTN_AXI_SIZE);
	if (g_tele_mntn_sctrl_base == NULL || g_tele_mntn_rtc0_base == NULL)
		return -EINVAL;
	if (M3_RDR_SYS_CONTEXT_BASE_ADDR != 0) {
		g_acore_tele_mntn =
			(struct acore_tele_mntn_stru_s *)M3_RDR_SYS_CONTEXT_KERNEL_STAT_ADDR;
		g_lpmcu_tele_mntn =
			(struct lpmcu_tele_mntn_stru_s *)M3_RDR_SYS_CONTEXT_LPMCU_STAT_ADDR;
	}

	return TELE_MNTN_OK;
}

void tele_mntn_common_exit(void)
{
	if (g_tele_mntn_sctrl_base != NULL) {
		iounmap(g_tele_mntn_sctrl_base);
		g_tele_mntn_sctrl_base = NULL;
	}
	if (g_tele_mntn_rtc0_base != NULL) {
		iounmap(g_tele_mntn_rtc0_base);
		g_tele_mntn_rtc0_base = NULL;
	}
}

unsigned long long tele_mntn_func_sw_get(void)
{
	unsigned long long sw_val;

	if (g_tele_mntn_sctrl_base == NULL)
		return 0;
	sw_val = (unsigned long long)readl((void *)SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base));
	sw_val = (sw_val << U32_BITS_LIMIT) |
	      (unsigned long long)readl((void *)SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base));

	return sw_val;
}

unsigned long long tele_mntn_func_sw_set(unsigned long long sw)
{
	if (g_tele_mntn_sctrl_base == NULL || sw > UINT_MAX)
		return 0;
	writel((unsigned int)(sw >> U32_BITS_LIMIT),
	       (void *)(SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base)));
	writel((unsigned int)(sw),
	       (void *)(SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base)));

	return sw;
}

unsigned long long tele_mntn_func_sw_bit_set(unsigned int bit)
{
	unsigned int sw_val;

	if (g_tele_mntn_sctrl_base == NULL || bit >= U64_BITS_LIMIT)
		return 0;
	if (bit >= U32_BITS_LIMIT) {
		sw_val = readl((void *)SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base));
		sw_val |= BIT(bit - U32_BITS_LIMIT);
		writel(sw_val,
		       (void *)(SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base)));
	} else {
		sw_val = readl((void *)SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base));
		sw_val |= BIT(bit);
		writel(sw_val,
		       (void *)(SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base)));
	}

	return tele_mntn_func_sw_get();
}

unsigned long long tele_mntn_func_sw_bit_clr(unsigned int bit)
{
	unsigned int sw_val;

	if (g_tele_mntn_sctrl_base == NULL || bit >= U64_BITS_LIMIT)
		return 0;
	if (bit >= U32_BITS_LIMIT) {
		sw_val = readl((void *)SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base));
		sw_val &= (~((unsigned int)BIT(bit - U32_BITS_LIMIT)));
		writel(sw_val,
		       (void *)(SOC_SCTRL_SCBAKDATA15_ADDR(g_tele_mntn_sctrl_base)));
	} else {
		sw_val = readl((void *)SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base));
		sw_val &= (~((unsigned int)BIT(bit)));
		writel(sw_val,
		       (void *)(SOC_SCTRL_SCBAKDATA5_ADDR(g_tele_mntn_sctrl_base)));
	}

	return tele_mntn_func_sw_get();
}

unsigned int get_slice_time(void)
{
	if (g_tele_mntn_sctrl_base == NULL)
		return 0;

	return readl((void *)SOC_SCTRL_SCBBPDRXSTAT1_ADDR(g_tele_mntn_sctrl_base));
}

unsigned int get_rtc_time(void)
{
	if (g_tele_mntn_rtc0_base == NULL)
		return 0;

	return readl((void *)SOC_RTCTIMERWDTV100_RTC_DR_ADDR(g_tele_mntn_rtc0_base));
}
