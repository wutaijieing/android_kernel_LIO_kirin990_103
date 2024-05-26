/*
 * loadmonitor_common.c
 *
 * loadmonitor module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include <linux/clk.h>
#include <linux/types.h>
#include "securec.h"
#include "loadmonitor_kernel.h"
#include <loadmonitor.h>
#include <loadmonitor_media_kernel.h>
#include <loadmonitor_common_kernel.h>
#include <asm/compiler.h>
#include <platform_include/see/bl31_smc.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)		"[loadmonitor&freqdump]:" fmt

#ifndef __asmeq
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"
#endif

struct loadmonitor_data g_loadmonitor_data_kernel;
/* default sample time: 1s */
struct delay_time_with_freq g_monitor_delay_value = {
	.monitor_enable_flags = ENABLE_NONE,
	.peri0 = LOADMONITOR_PERI_FREQ_CS,
	.peri1 = LOADMONITOR_PERI_FREQ_CS,
	.ao = MONITOR_AO_FREQ_CS,
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	.media0 = LOADMONITOR_MEDIA0_FREQ,
	.media1 = LOADMONITOR_MEDIA1_FREQ,
#endif
};

noinline int atfd_service_freqdump_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

/* cs iom7 code start */
static int ao_loadmonitor_read(void)
{
	struct loadmonitor_sigs sigs;
	u32 i = 0; /* sensor id, in AO, from 32 to 63 */
	u32 samples;

	int ret = _ao_loadmonitor_read(&sigs, sizeof(sigs));
	if (ret != 0 && ret != -ENODEV) { /* -ENODEV:closed by DTS config */
		pr_err("%s: get signal cnt error! ret:%d\n", __func__, ret);
		return -EINVAL;
	}

	for (; i < MAX_SIG_CNT_PER_IP; ++i) {
		if ((g_loadmonitor_data_kernel.ips.ao.mask & BIT(i)) != 0) {
			samples = sigs.sig[i].samples;
			if (samples == 0)
				return -EINVAL;
			g_loadmonitor_data_kernel.ips.ao.ip[i].idle =
				(u32)(sigs.sig[i].count[DATA_INDEX_IDLE] /
				      samples);
			g_loadmonitor_data_kernel.ips.ao.ip[i].busy_norm_volt =
				(u32)(sigs.sig[i].count[DATA_INDEX_BUSY_NORMAL] /
				      samples);
			g_loadmonitor_data_kernel.ips.ao.ip[i].buys_low_volt =
				(u32)(sigs.sig[i].count[DATA_INDEX_BUSY_LOW] /
				      samples);
			g_loadmonitor_data_kernel.ips.ao.samples = samples;
		}
	}
	return 0;
}

static int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq)
{
	int ret;
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("%s input error\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR(monitor_clk)) {
		pr_err("%s peri clk init error\n", __func__);
		return -1;
	}

	/* set frequency */
	ret = clk_set_rate(monitor_clk, clk_freq);
	if (ret < 0) {
		pr_err("%s peri clk set rate error\n", __func__);
		clk_put(monitor_clk);
		return -1;
	}
	/* enable clk_loadmonitor */
	ret = clk_prepare_enable(monitor_clk);
	if (ret != 0) {
		clk_put(monitor_clk);
		pr_err("%s peri clk enable error\n", __func__);
		return -1;
	}

	clk_put(monitor_clk);

	return 0;
}

static int peri_monitor_clk_disable(const char *clk_name)
{
	struct clk *monitor_clk = NULL;

	if (clk_name == NULL) {
		pr_err("%s input error\n", __func__);
		return -1;
	}

	monitor_clk = clk_get(NULL, clk_name);
	if (IS_ERR(monitor_clk)) {
		pr_err("%s peri clk disable error\n", __func__);
		return -1;
	}

	/* disable clk_loadmonitor */
	clk_disable_unprepare(monitor_clk);
	clk_put(monitor_clk);

	return 0;
}

static int all_peri_clk_init(u32 en_flags)
{
	int ret;

	if ((en_flags & ENABLE_PERI0) == ENABLE_PERI0) {
		ret = peri_monitor_clk_init("clk_loadmonitor",
					    MONITOR_PERI_FREQ_CS);
		if (ret != 0) {
			pr_err("%s peri_monitor0_clk_init error\n",
			       __func__);
			return ret;
		}
	}

#ifndef CONFIG_PERI1_NO_KERNEL_CLK
	if ((en_flags & ENABLE_PERI1) == ENABLE_PERI1) {
		ret = peri_monitor_clk_init("clk_loadmonitor_l",
					    MONITOR_PERI_FREQ_CS);
		if (ret != 0) {
			pr_err("%s peri_monitor1_clk_init error\n", __func__);
			return ret;
		}
	}
#endif

	pr_info("%s en_flags:%#X, end\n", __func__, en_flags);
	return 0;
}

static int all_peri_clk_disable(u32 en_flags)
{
	int ret;

	if ((en_flags & ENABLE_PERI0) == ENABLE_PERI0) {
		ret = peri_monitor_clk_disable("clk_loadmonitor");
		if (ret != 0) {
			pr_err("%s peri_monitor0_clk_disable error\n",
			       __func__);
			return ret;
		}
		pr_info("%s peri0 clk disable succ. en_flags:%#X\n", __func__, en_flags);
	}

#ifndef CONFIG_PERI1_NO_KERNEL_CLK
	if ((en_flags & ENABLE_PERI1) == ENABLE_PERI1) {
		ret = peri_monitor_clk_disable("clk_loadmonitor_l");
		if (ret != 0) {
			pr_err("%s peri_monitor1_clk_disable error\n",
			       __func__);
			return ret;
		}
		pr_info("%s peri1 clk disable succ. en_flags:%#X\n", __func__, en_flags);
	}
#endif
	pr_info("%s all_clk_disable end\n", __func__);

	return 0;
}

void sec_loadmonitor_peri_enable(void)
{
	if ((g_monitor_delay_value.monitor_enable_flags & ENABLE_PERI0) ==
	    ENABLE_PERI0) {
		(void)atfd_service_freqdump_smc(
			(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
			(u64)g_monitor_delay_value.peri0,
			(u64)0,
			(u64)ENABLE_PERI0);
		pr_info("%s peri0 enable success, en_flags:%#X, sample time:%#x\n", __func__,
			g_monitor_delay_value.monitor_enable_flags,
			g_monitor_delay_value.peri0);
	}
	if ((g_monitor_delay_value.monitor_enable_flags & ENABLE_PERI1) ==
	    ENABLE_PERI1) {
		(void)atfd_service_freqdump_smc(
			(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
			(u64)g_monitor_delay_value.peri1,
			(u64)0,
			(u64)ENABLE_PERI1);
		pr_info("%s peri1 enable success, en_flags:%#X, sample time:%#x\n",
			__func__, g_monitor_delay_value.monitor_enable_flags,
			g_monitor_delay_value.peri1);
	}
}

void sec_loadmonitor_switch_enable(bool open_sr_ao_monitor)
{
	int ret;

	ret = all_peri_clk_init(g_monitor_delay_value.monitor_enable_flags);
	if (ret != 0)
		return;
	sec_loadmonitor_peri_enable();

	if ((g_monitor_delay_value.monitor_enable_flags & ENABLE_AO) ==
	    ENABLE_AO && !open_sr_ao_monitor) {
		ret = ao_loadmonitor_enable(g_monitor_delay_value.ao,
					    MONITOR_AO_FREQ_CS);
		if (ret != 0) {
			pr_err("%s ao_loadmonitor_enable error, ret:%d.\n",
			       __func__, ret);
			return;
		}
		pr_info("%s ao enable succ.sample times:%#x, freq:%#X\n",
			__func__, g_monitor_delay_value.ao, MONITOR_AO_FREQ_CS);
	}
	pr_info("%s end. en_flags:%#X, open sr ao monitor(no disable, no enable):%d\n", __func__,
		g_monitor_delay_value.monitor_enable_flags, (int)(open_sr_ao_monitor));
}

/* eng and user version loadmonitor disable interface; sr ao no need disable */
void sec_loadmonitor_switch_disable(u32 en_flags, bool open_sr_ao_monitor)
{
	int ret;

	(void)atfd_service_freqdump_smc(
		(u64)FREQDUMP_LOADMONITOR_SVC_DISABLE,
		(u64)0, (u64)0, (u64)0);
	pr_info("%s atf loadmonitor disable succ. en_flags:%#X\n", __func__, en_flags);

	(void)all_peri_clk_disable(en_flags);

	if ((en_flags & ENABLE_AO) == ENABLE_AO && !open_sr_ao_monitor) {
		ret = ao_loadmonitor_disable();
		if (ret != 0) {
			pr_err("%s ao_loadmonitor_disable error,ret:%d\n",
			       __func__, ret);
			return;
		}
		pr_info("%s ao disable succ. en_flags:%#X\n", __func__, en_flags);
	}
	pr_info("%s end. en_flags:%#X,open sr ao monitor(no sr ao disable):%d\n",
		__func__, en_flags, (int)(open_sr_ao_monitor));
}

void sec_loadmonitor_data_read(unsigned int enable_flags)
{
	(void)atfd_service_freqdump_smc(
		(u64)FREQDUMP_LOADMONITOR_SVC_REG_RD,
		(u64)enable_flags, (u64)(uintptr_t)loadmonitor_phy_addr,
		(u64)0);
}

void sec_loadmonitor_data_get(unsigned int enable_flags)
{
	(void)atfd_service_freqdump_smc(
		(u64)LOADMONITOR_SVC_REG_GET,
		(u64)enable_flags, (u64)(uintptr_t)loadmonitor_phy_addr,
		(u64)0);
	pr_info("%s: copy data from var to shared mem:%#X\n",
		__func__, enable_flags);
}

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
static void read_media_loadmonitor_data(u32 flags)
{
	int ret;

	if ((flags & ENABLE_MEDIA0) == ENABLE_MEDIA0) {
		ret = media0_monitor_read(g_monitor_delay_value.media0);
		if (ret != 0)
			pr_err("%s: media0 loadmonitor read error:%#X\n", __func__, flags);
		else
			pr_info("%s: media0 loadmonitor read success:%#X\n", __func__, flags);
	}
	if ((flags & ENABLE_MEDIA1) == ENABLE_MEDIA1) {
		ret = media1_monitor_read(g_monitor_delay_value.media1);
		if (ret != 0)
			pr_err("%s: media1 loadmonitor read error:%#X\n", __func__, flags);
		else
			pr_info("%s: media1 loadmonitor read success:%#X\n", __func__, flags);
	}
}
#endif

static void read_peri_loadmonitor_data(u32 flags)
{
	if ((flags & ENABLE_PERI0) == ENABLE_PERI0 || (flags & ENABLE_PERI1) == ENABLE_PERI1) {
		sec_loadmonitor_data_read(flags);
		pr_info("%s: peri loadmonitor read success:%#X\n", __func__, flags);
	}
}

static void read_ddr_and_peri_vote_loadmonitor_data(u32 flags)
{
	if ((flags & (ENABLE_DDR | ENABLE_VOTE)) == (ENABLE_DDR | ENABLE_VOTE)) {
		sec_loadmonitor_data_read(flags);
		pr_info("%s: ddr and peri vote loadmonitor read success:%#X\n", __func__, flags);
	}
}

static void read_ao_loadmonitor_data(u32 flags)
{
	if ((flags & ENABLE_AO) == ENABLE_AO) {
		if (ao_loadmonitor_read() != 0)
			pr_err("%s : ao_loadmonitor_read error\n", __func__);
		else
			pr_info("%s: ao loadmonitor read success\n", __func__);
	}
}

static void get_load_monitor_data_from_bl31(u32 flags)
{
	sec_loadmonitor_data_get(flags);
	memcpy_fromio((void *)(&g_loadmonitor_data_kernel),
		      (void *)(g_loadmonitor_virt_addr),
		      sizeof(g_loadmonitor_data_kernel));
}

/* eng and user version loadmonitor interface */
int read_format_cs_loadmonitor_data(u32 flags)
{
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	read_media_loadmonitor_data(flags & ENABLE_MEDIA);
#endif
	read_peri_loadmonitor_data(flags & ENABLE_PERI);
	read_ddr_and_peri_vote_loadmonitor_data(flags & (ENABLE_DDR | ENABLE_VOTE));
	get_load_monitor_data_from_bl31(flags);

	read_ao_loadmonitor_data(flags & ENABLE_AO);
	pr_info("%s:end:%#X\n", __func__, flags);
	return OK;
}
