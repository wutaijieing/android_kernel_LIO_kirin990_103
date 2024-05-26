
/*
 * dpm_hwmon_v3.c
 *
 * dpm interface for component
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "dpm_hwmon_v3.h"
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <securec.h>

LIST_HEAD(g_dpm_hwmon_ops_list);
DEFINE_MUTEX(g_dpm_hwmon_ops_list_lock);

#ifdef CONFIG_DPM_HWMON_PWRSTAT
#include <soc_acpu_baseaddr_interface.h>
#include <soc_powerstat_interface.h>
#include <soc_crgperiph_interface.h>

#define PERI_CRG_SIZE	0xFFF
#define PWR_STAT_SIZE	0xFFF
#define DPM_DATA_ADDR	0xC0000000
#define DPM_DATA_SIZE	0x40000000
#define PWRSTAT_CHANNEL_MAX	23

#define BUFFER_SIZE	30

static void __iomem *g_peri_crg_base;
static void __iomem *g_pwr_stat_base;

void dpm_sample(struct dpm_hwmon_ops *pos)
{
	(void)pos;
}

static int map_powerstate_range(struct dpm_hwmon_ops *dpm_ops)
{
	char buffer[BUFFER_SIZE];
	int ret = 0;
	struct device_node *np = NULL;

	ret = snprintf_s(buffer, BUFFER_SIZE * sizeof(char), (BUFFER_SIZE - 1) * sizeof(char),
			 "hisilicon,lp_power%s", dpm_module_table[dpm_ops->dpm_module_id]);
	if (ret < 0) {
		pr_err("%s: snprintf_s buffer err.\n", __func__);
		return ret;
	}
	np = of_find_compatible_node(NULL, NULL, buffer);
	if (np == NULL) {
		pr_err("%s: %s not find.\n", __func__, buffer);
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "power-start", &(dpm_ops->power_start));
	if (ret < 0) {
		pr_err("%s no power start!\n", __func__);
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "power-end", &(dpm_ops->power_end));
	if (ret < 0) {
		pr_err("%s no power end!\n", __func__);
		return -ENODEV;
	}
	return 0;
}

/* mode:0-total_energy 1-total_enegy,dyn_energy 2-energy,icg_cnt 3-energy,pmu_cnt */
void powerstat_enable(unsigned int mode, unsigned int channels,
		      unsigned int interval, unsigned int sample_time)
{
	/* step 1,2 */
	writel(BIT(SOC_CRGPERIPH_PEREN0_gt_pclk_power_stat_START) |
	       BIT(SOC_CRGPERIPH_PEREN0_gt_clk_power_stat_START),
	       SOC_CRGPERIPH_PEREN0_ADDR(g_peri_crg_base));
	writel(BIT(SOC_CRGPERIPH_PERRSTDIS0_ip_rst_power_stat_START),
	       SOC_CRGPERIPH_PERRSTDIS0_ADDR(g_peri_crg_base));

	/* step 3, init channel */
	writel(channels, SOC_POWERSTAT_CHNL_EN0_ADDR(g_pwr_stat_base));

	/* step 4,5 interval and sample time */
	writel(interval, SOC_POWERSTAT_INTERVAL_ADDR(g_pwr_stat_base));
	writel(sample_time, SOC_POWERSTAT_SAMPLE_TIME_ADDR(g_pwr_stat_base));

	writel(mode, SOC_POWERSTAT_WORK_MODE_CFG_ADDR(g_pwr_stat_base));
	if ((mode & BIT(SOC_POWERSTAT_WORK_MODE_CFG_sample_mode_START)) == 0) {
		writel(0x485E, SOC_POWERSTAT_SRAM_CTRL_ADDR(g_pwr_stat_base));
	} else {
		writel(0x4858, SOC_POWERSTAT_SRAM_CTRL_ADDR(g_pwr_stat_base));

		writel(0xf, SOC_POWERSTAT_SFIFO_CFG_ADDR(g_pwr_stat_base));
		writel(DPM_DATA_ADDR, SOC_POWERSTAT_SEQ_ADDR_CFG0_ADDR(g_pwr_stat_base));
		writel(0, SOC_POWERSTAT_SEQ_ADDR_CFG1_ADDR(g_pwr_stat_base));
		writel(DPM_DATA_SIZE, SOC_POWERSTAT_SEQ_SPACE_CFG0_ADDR(g_pwr_stat_base));
		writel(0, SOC_POWERSTAT_SEQ_SPACE_CFG1_ADDR(g_pwr_stat_base));
	}

	/* step 7,8 */
	writel(0xffff, SOC_POWERSTAT_AXI_INTF_CFG_ADDR(g_pwr_stat_base));

	/* step 9 en sample */
	writel(1, SOC_POWERSTAT_SAMPLE_EN_ADDR(g_pwr_stat_base));
}

void powerstat_disable(void)
{
	/* step 1 disable sample */
	writel(0, SOC_POWERSTAT_SAMPLE_EN_ADDR(g_pwr_stat_base));

	mdelay(1);

	writel(0x485E, SOC_POWERSTAT_SRAM_CTRL_ADDR(g_pwr_stat_base));

	/* step 4,5 */
	writel(BIT(SOC_CRGPERIPH_PERRSTEN0_ip_rst_power_stat_START),
	       SOC_CRGPERIPH_PERRSTEN0_ADDR(g_peri_crg_base));
	writel(BIT(SOC_CRGPERIPH_PERDIS0_gt_pclk_power_stat_START) |
	       BIT(SOC_CRGPERIPH_PERDIS0_gt_clk_power_stat_START),
	       SOC_CRGPERIPH_PERDIS0_ADDR(g_peri_crg_base));
}

void powerstat_config(struct dpm_hwmon_ops *pos, int timer_span, int total_count, int mode)
{
	unsigned int i;
	unsigned int channels = 0;

	for (i = pos->power_start; i < (pos->power_end + 1); i++)
		channels |= BIT(i);

	powerstat_disable();
	powerstat_enable((unsigned int)mode, channels,
			 (unsigned int)timer_span, (unsigned int)total_count);
}

static unsigned long long read_dpm_total_energy(unsigned int start, unsigned int end)
{
	unsigned int channel;
	unsigned long long energy = 0;

	if (end >= PWRSTAT_CHANNEL_MAX)
		return 0;
	for (channel = start; channel < (end + 1); channel++)
		energy += readq(SOC_POWERSTAT_DPM_TOTAL_ENERGY_LOW_ADDR(g_pwr_stat_base, channel));
	return energy;
}

unsigned long long get_dpm_power(struct dpm_hwmon_ops *pos)
{
	unsigned long long total_power = 0;

	if (pos == NULL)
		return total_power;

	total_power = read_dpm_total_energy(pos->power_start, pos->power_end);
	return total_power;
}

unsigned long long get_part_power(struct dpm_hwmon_ops *pos,
				  unsigned int start, unsigned int end)
{
	unsigned long long total_power = 0;

	if (pos == NULL || end > (pos->power_end - pos->power_start))
		return total_power;

	total_power = read_dpm_total_energy(pos->power_start + start,
					    pos->power_start + end);
	return total_power;
}

void dpm_iounmap(void)
{
	if (g_peri_crg_base != NULL) {
		iounmap(g_peri_crg_base);
		g_peri_crg_base = NULL;
	}
	if (g_pwr_stat_base != NULL) {
		iounmap(g_pwr_stat_base);
		g_pwr_stat_base = NULL;
	}
}

int dpm_ioremap(void)
{
	g_peri_crg_base = ioremap(SOC_ACPU_PERI_CRG_BASE_ADDR, PERI_CRG_SIZE);
	g_pwr_stat_base = ioremap(SOC_ACPU_PWR_STAT_BASE_ADDR, PWR_STAT_SIZE);
	if (g_peri_crg_base == NULL || g_pwr_stat_base == NULL) {
		dpm_iounmap();
		return -EFAULT;
	}
	return 0;
}
#else
#include <linux/ktime.h>
#include <linux/workqueue.h>

#define DPM_SAMPLE_INTERVAL		1000
static struct delayed_work g_dpm_hwmon_work;

static inline void dpm_iounmap(void) {}
static inline int dpm_ioremap(void) {return 0; }

static void free_dpm_ops_mem(struct dpm_hwmon_ops *dpm_ops)
{
	if (dpm_ops->total_power_table != NULL) {
		kfree(dpm_ops->total_power_table);
		dpm_ops->total_power_table = NULL;
	}
#ifdef CONFIG_DPM_HWMON_DEBUG
	if (dpm_ops->dyn_power_table != NULL) {
		kfree(dpm_ops->dyn_power_table);
		dpm_ops->dyn_power_table = NULL;
	}
	if (dpm_ops->dpm_counter_table != NULL) {
		kfree(dpm_ops->dpm_counter_table);
		dpm_ops->dpm_counter_table = NULL;
	}
#endif
}
static int malloc_dpm_ops_mem(struct dpm_hwmon_ops *dpm_ops)
{
#ifdef CONFIG_DPM_HWMON_DEBUG
	if ((dpm_ops->dpm_cnt_len > 0 && dpm_ops->dpm_cnt_len < DPM_BUFFER_SIZE) &&
	    (dpm_ops->dpm_power_len > 0 && dpm_ops->dpm_power_len < DPM_BUFFER_SIZE)) {
		dpm_ops->total_power_table = kzalloc(sizeof(unsigned int) *
						     dpm_ops->dpm_power_len,
						     GFP_KERNEL);
		dpm_ops->dyn_power_table = kzalloc(sizeof(unsigned int) *
						   dpm_ops->dpm_power_len,
						   GFP_KERNEL);
		dpm_ops->dpm_counter_table = kzalloc(sizeof(unsigned int) *
						     dpm_ops->dpm_cnt_len,
						     GFP_KERNEL);
		if (dpm_ops->total_power_table == NULL ||
		    dpm_ops->dyn_power_table == NULL ||
		    dpm_ops->dpm_counter_table == NULL)
			return -ENOMEM;
	} else {
		return -EINVAL;
	}
#else
	if (dpm_ops->dpm_power_len > 0 && dpm_ops->dpm_power_len < DPM_BUFFER_SIZE) {
		dpm_ops->total_power_table = kzalloc(sizeof(unsigned long long) *
						     dpm_ops->dpm_power_len,
						     GFP_KERNEL);
		if (dpm_ops->total_power_table == NULL)
			return -ENOMEM;
	} else {
		return -EINVAL;
	}
#endif
	return 0;
}

unsigned long long get_dpm_power(struct dpm_hwmon_ops *pos)
{
	unsigned int i;
	unsigned long long total_power = 0;

	if (pos == NULL)
		return total_power;

	for (i = 0; i < pos->dpm_power_len; i++)
		total_power += pos->total_power_table[i];

	return total_power;
}

unsigned long long get_part_power(struct dpm_hwmon_ops *pos,
				  unsigned int start, unsigned int end)
{
	unsigned int i;
	unsigned long long total_power = 0;

	if (pos == NULL || end >= pos->dpm_power_len)
		return total_power;

	for (i = start; i < (end + 1); i++)
		total_power += pos->total_power_table[i];

	return total_power;
}
void dpm_sample(struct dpm_hwmon_ops *pos)
{
	if (pos == NULL)
		return;
	if (pos->hi_dpm_update_power() < 0)
		pr_err("DPM %d sample fail\n", pos->dpm_module_id);
}

static void dpm_hwmon_sample_func(struct work_struct *work)
{
	struct dpm_hwmon_ops *pos = NULL;

	if (!g_dpm_report_enabled)
		goto restart_work;
	list_for_each_entry(pos, &g_dpm_hwmon_ops_list, ops_list)
		dpm_sample(pos);

restart_work:
	queue_delayed_work(system_freezable_power_efficient_wq,
			   &g_dpm_hwmon_work,
			   msecs_to_jiffies(DPM_SAMPLE_INTERVAL));
}

#endif

int dpm_hwmon_register(struct dpm_hwmon_ops *dpm_ops)
{
	struct dpm_hwmon_ops *pos = NULL;
	int ret, err;

	if (dpm_ops == NULL || dpm_ops->dpm_module_id < 0 ||
	    dpm_ops->dpm_module_id >= DPM_MODULE_NUM) {
		pr_err("%s LINE %d dpm_ops is invalid\n", __func__, __LINE__);
		return -EINVAL;
	}

	mutex_lock(&g_dpm_hwmon_ops_list_lock);
	list_for_each_entry(pos, &g_dpm_hwmon_ops_list, ops_list) {
		if (dpm_ops->dpm_module_id == pos->dpm_module_id) {
			pr_err("%s LINE %d dpm module %d has registered\n",
			       __func__, __LINE__, dpm_ops->dpm_module_id);
			mutex_unlock(&g_dpm_hwmon_ops_list_lock);
			return -EINVAL;
		}
	}
	list_add(&dpm_ops->ops_list, &g_dpm_hwmon_ops_list);
	mutex_unlock(&g_dpm_hwmon_ops_list_lock);
#ifdef CONFIG_DPM_HWMON_PWRSTAT
	ret = map_powerstate_range(dpm_ops);
	if (ret < 0) {
		pr_err("%s: map dtsi fail", __func__);
		goto err_handler;
	}
#else
	ret = malloc_dpm_ops_mem(dpm_ops);
	if (ret < 0) {
		pr_err("%s: malloc mem fail", __func__);
		goto err_handler;
	}
#endif

	dpm_ops->module_enabled = false;
	pr_err("dpm hwmon %d register!\n", dpm_ops->dpm_module_id);
	return ret;

err_handler:
	err = dpm_hwmon_unregister(dpm_ops);
	if (err < 0)
		pr_err("%s LINE %d register fail", __func__, __LINE__);
	return ret;
}

int dpm_hwmon_unregister(struct dpm_hwmon_ops *dpm_ops)
{
	struct dpm_hwmon_ops *pos = NULL;
	struct dpm_hwmon_ops *tmp = NULL;

	if (dpm_ops == NULL) {
		pr_err("%s LINE %d dpm_ops is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	mutex_lock(&g_dpm_hwmon_ops_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_dpm_hwmon_ops_list, ops_list) {
		if (dpm_ops->dpm_module_id == pos->dpm_module_id) {
#ifndef CONFIG_DPM_HWMON_PWRSTAT
			free_dpm_ops_mem(dpm_ops);
#endif
			list_del_init(&pos->ops_list);
			break;
		}
	}
	mutex_unlock(&g_dpm_hwmon_ops_list_lock);
	return 0;
}

void dpm_enable_module(struct dpm_hwmon_ops *pos, bool dpm_switch)
{
	if (pos != NULL)
		pos->module_enabled = dpm_switch;
}

bool get_dpm_enabled(struct dpm_hwmon_ops *pos)
{
	if (pos != NULL)
		return pos->module_enabled;
	else
		return false;
}

int dpm_hwmon_prepare(void)
{
#ifndef CONFIG_DPM_HWMON_PWRSTAT
	/* dpm workqueue initialize */
	INIT_DEFERRABLE_WORK(&g_dpm_hwmon_work, dpm_hwmon_sample_func);
	queue_delayed_work(system_freezable_power_efficient_wq,
			   &g_dpm_hwmon_work,
			   msecs_to_jiffies(DPM_SAMPLE_INTERVAL));
#endif
	if (dpm_ioremap() < 0)
		return -EFAULT;
	return 0;
}

void dpm_hwmon_end(void)
{
#ifdef CONFIG_DPM_HWMON_PWRSTAT
	powerstat_disable();
#else
	cancel_delayed_work(&g_dpm_hwmon_work);
#endif
	dpm_iounmap();
}
