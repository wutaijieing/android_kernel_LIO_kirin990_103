/*
 * dpm_common.c
 *
 * dpm_common module
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/semaphore.h>
#ifdef CONFIG_LIBLINUX
#include <global_ddr_map_cdc_ace.h>
#else
#include <global_ddr_map.h>
#endif
#include <linux/platform_device.h>
#include <securec.h>
#include "freqdump_kernel.h"
#include "monitor_ap_m3_ipc.h"

#ifdef CONFIG_DPM_PLATFORM_MERCURY_CS2
#include "dpm_kernel_mercury_cs2.h"
#else
#ifdef CONFIG_DPM_PLATFORM_MERCURY
#include "dpm_kernel_mercury.h"
#include "mercury/freqdump_kernel.h"
#endif
#endif
#ifdef CONFIG_DPM_PLATFORM_JUPITER
#include "dpm_kernel_jupiter.h"
#endif
#ifdef CONFIG_DPM_PLATFORM_SATURN
#include "dpm_kernel_saturn.h"
#endif
#ifdef CONFIG_DPM_PLATFORM_EARTH
#include "dpm_kernel_earth.h"
#endif
#ifdef CONFIG_DPM_PLATFORM_MARS
#include "dpm_kernel_mars.h"
#endif
#ifdef CONFIG_DPM_PLATFORM_VENUS
#include "dpm_kernel_venus.h"
#endif
#ifdef CONFIG_DPM_PLATFORM_URANUS
#include "dpm_kernel_uranus.h"
#endif

#ifdef CONFIG_ITS
#include "soc_its_para.h"
#endif

#ifdef CONFIG_LOWPM_STATS
#include <linux/platform_drivers/dubai_lp_stats.h>
#endif

/* begin from eromitlab */
#if !defined(CONFIG_DPM_PLATFORM_MERCURY) && \
	!defined(CONFIG_DPM_PLATFORM_MERCURY_CS2) && \
	!defined(CONFIG_DPM_PLATFORM_JUPITER) && \
	!defined(CONFIG_DPM_PLATFORM_SATURN)  && \
	!defined(CONFIG_DPM_PLATFORM_EARTH)   && \
	!defined(CONFIG_DPM_PLATFORM_MARS)    && \
	!defined(CONFIG_DPM_PLATFORM_VENUS)   && \
	!defined(CONFIG_DPM_PLATFORM_URANUS)
#include "dpm_kernel.h"
#endif

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
#include "media_monitor.h"
#endif

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)		"[loadmonitor]:" fmt

typedef int (*IOCTRL_FUNC)(void __user *argp);
struct ioctrl_dpm_ldm_t {
	u_int ioctrl_cmd;
	IOCTRL_FUNC ioctrl_func;
};

typedef int (*IOCTRL_CONST_FUNC)(const void __user *argp);
struct const_ioctrl_dpm_ldm_t {
	u_int ioctrl_cmd;
	IOCTRL_CONST_FUNC ioctrl_func;
};

static struct semaphore g_dpm_sem;
static struct semaphore g_dpm_sem_update;
static struct semaphore g_dpm_sem_loadmonitor;
static unsigned int g_dpm_period = (1000000 * LOADMONITOR_PCLK);
static unsigned int g_ddr_old[2];
static unsigned int g_ddr_new[2];
static unsigned int g_ddr_bw_rd;
static unsigned int g_ddr_bw_wr;
static unsigned int g_ddr_suspend_switch;
static struct delayed_work g_dpm_delayed_work;
static struct workqueue_struct *g_monitor_dpm_wq;

#ifdef CONFIG_FREQDUMP_PLATFORM
unsigned int g_dpm_loadmonitor_data_peri[MAX_PERI_INDEX];
static u64 g_dpm_loadmonitor_data_ao[MAX_AO_INDEX];
static unsigned int g_dpm_period_ao = (1000 * LOADMONITOR_AO_CLK);
static struct freqdump_data_bos *g_dpm_freqdump_data;
#else
static unsigned int g_dpm_loadmonitor_data[MAX_MONITOR_IP_DEVICE];
static struct freqdump_data *g_dpm_freqdump_data;
#endif

#ifdef CONFIG_ITS
static its_cpu_power_t *g_its_cpu_power_data;
static size_t g_its_cpu_power_data_size;
#endif

#define LOADMONITOR_ON		1
#define LOADMONITOR_OFF		0
#define LOADMONITOR_SUSPEND	2
#define LOADMONITOR_RESUME	3

static int g_loadmonitor_status = LOADMONITOR_OFF;

static unsigned int g_dpmldm_major = 225;
static struct dpm_transmit_t *g_dpm_data;
static size_t g_dpm_data_size;
static struct dpm_cdev *g_dpm_ldm_cdevp;

void (*g_calc_monitor_time_media_hook)(int index, u32 media_pclk,
				       u64 *busy_nor, u64 *busy_low,
				       unsigned long monitor_delay);

static void enable_loadmonitor_and_queue(void)
{
	sec_loadmonitor_switch_enable(g_dpm_period,
					   g_dpm_period_ao,
					   ALL_MONITOR_EN);
	g_monitor_info.monitor_delay =
		g_dpm_period / LOADMONITOR_PCLK;
	queue_delayed_work(g_monitor_dpm_wq,
			   &g_dpm_delayed_work,
			   msecs_to_jiffies((u32)(g_monitor_info.monitor_delay / 1000)));
}

static int ioctrl_enable(const void __user *argp)
{
	int dpm_switch;
	int ret = OK;

	if (copy_from_user(&dpm_switch, argp, sizeof(int))) {
		ret = -EINVAL;
		return ret;
	}

	down(&g_dpm_sem_loadmonitor);

	if (dpm_switch == DPM_ON) {
		if (g_loadmonitor_status == LOADMONITOR_OFF) {
#ifdef CONFIG_DPM_PLATFORM_PLUTO
			if (g_dpm_period == 0) {
				pr_info("%s g_dpm_period is zero!\n",
					__func__);
				g_dpm_period = (1000000 * LOADMONITOR_PCLK);
			}
#endif
			pr_info("%s g_dpm_period is %u!\n", __func__,
				g_dpm_period);

			sec_chip_type_read();
			enable_loadmonitor_and_queue();
			g_loadmonitor_status = LOADMONITOR_ON;
		}
	} else if (dpm_switch == DPM_SUSPEND) {
		if (g_loadmonitor_status == LOADMONITOR_ON) {
			cancel_delayed_work_sync(&g_dpm_delayed_work);
			sec_loadmonitor_switch_disable();
			g_loadmonitor_status = LOADMONITOR_OFF;
		}
	} else if (dpm_switch == DPM_OFF) {
		if (g_loadmonitor_status == LOADMONITOR_ON) {
			cancel_delayed_work_sync(&g_dpm_delayed_work);
			sec_loadmonitor_switch_disable();
			clear_dpm_data();
			g_loadmonitor_status = LOADMONITOR_OFF;
		}
	} else {
		pr_err("dpm_ctrl input invalid parameter!\n");
		ret = -EINVAL;
	}

	pr_err("%s success, dpm_switch:%d, g_loadmonitor_status:%u!\n",
	       __func__, dpm_switch, g_loadmonitor_status);
	up(&g_dpm_sem_loadmonitor);
	return ret;
}

static int ioctrl_get_version(void __user *argp)
{
	int dpm_version;

	dpm_version = DPM_LDM_VERSION;
	if (copy_to_user(argp, &dpm_version, sizeof(dpm_version)) != 0) {
		pr_err("dpm_period get version failed!\n");
		return -EFAULT;
	}
	return OK;
}

static int ioctrl_get_ip_len(void __user *argp)
{
	int dpm_ipnum;

	g_dpm_data->length = MAX_MONITOR_IP_DEVICE + 1;
	dpm_ipnum = g_dpm_data->length;
	if (copy_to_user(argp, &dpm_ipnum, sizeof(dpm_ipnum)) != 0) {
		pr_err("dpm_period get version failed!\n");
		return -EFAULT;
	}
	return OK;
}

static int ioctrl_get_sample_time(void __user *argp)
{
	unsigned int period;
	int dpm_switch;
	int ret = OK;

	if (copy_from_user(&dpm_switch, argp, sizeof(dpm_switch)) != 0) {
		ret = -EINVAL;
		return ret;
	}
	if (dpm_switch == DPM_GET_MAX_PERIOD) {
		period = DPM_PERIOD_MAX / (LOADMONITOR_PCLK * 1000);
		if (copy_to_user(argp, &period, sizeof(period)) != 0) {
			pr_err("dpm_period get version failed!\n");
			ret = -EFAULT;
		}
	} else if (dpm_switch == DPM_GET_MIN_PERIOD) {
		period = DPM_PERIOD_MIN / (LOADMONITOR_PCLK * 1000);
		if (copy_to_user(argp, &period, sizeof(period)) != 0) {
			pr_err("dpm_period get version failed!\n");
			ret = -EFAULT;
		}
	} else {
		pr_err("dpm_ctrl input invalid parameter!\n");
		ret = -EINVAL;
	}
	return ret;
}

static int ioctrl_set_sample_time(const void __user *argp)
{
	unsigned int period;
	int ret = OK;

	if (copy_from_user(&period, argp, sizeof(period)) != 0) {
		ret = -EINVAL;
		return ret;
	}
	period = period * LOADMONITOR_PCLK * 1000;
	if (period > DPM_PERIOD_MAX || period < DPM_PERIOD_MIN) {
		pr_err("dpm period is invalid!\n");
		ret = -EINVAL;
	} else {
		g_dpm_period = (unsigned int)period;
#ifdef CONFIG_FREQDUMP_PLATFORM
		g_dpm_period_ao = (period / LOADMONITOR_PCLK) *
				  (LOADMONITOR_AO_CLK / 1000);
#endif
	}
	return ret;
}

static int ioctrl_get_device_data(void __user *argp)
{
	int ret = OK;

	if (g_dpm_data != NULL && g_dpm_data_size > 0) {
		if (copy_to_user(argp, g_dpm_data, g_dpm_data_size) != 0) {
			pr_err("dev->dpm_data_buf get failed!\n");
			ret = -EFAULT;
		}
	} else {
		pr_err("copy g_dpm_data fail\n");
		ret = -EINVAL;
	}

	return ret;
}

static int ioctrl_get_ddr_freq_count(void __user *argp)
{
	int ret = OK;
	int ddr_freq_num;

	ddr_freq_num = get_ddr_freq_num();
	if (ddr_freq_num == -EINVAL) {
		pr_err("%s freq num is invalid\n", __func__);
		return -EINVAL;
	}

	if (copy_to_user(argp, &ddr_freq_num, sizeof(ddr_freq_num))) {
		pr_err(" dpm get ddr freq num failed!\n");
		ret = -EFAULT;
	}
	return ret;
}

static int ioctrl_get_ddr_freq_info(void __user *argp)
{
	int ret = OK;
	char *ddr_data = NULL;
	int ddr_freq_num, j;
	unsigned int ddr_data_len, i;
	unsigned int pos = 0;
	u64 ddr_data_temp;

	ddr_freq_num = get_ddr_freq_num();
	if (ddr_freq_num == -EINVAL) {
		pr_err("%s freq num is invalid\n", __func__);
		return -EINVAL;
	}

	ddr_data_len = (IP_NAME_SIZE + ((unsigned int)ddr_freq_num * sizeof(u64))) * DDR_INFO_LEN;
	ddr_data = kzalloc(ddr_data_len, GFP_KERNEL);
	if (ddr_data == NULL) {
		pr_err("%s %d kzalloc fail\n", __func__, __LINE__);
		return -EINVAL;
	}

	for (i = 0; i < DDR_INFO_LEN; i++) {
		if (pos >= ddr_data_len) {
			pr_err("%s %d pos over flow\n", __func__, __LINE__);
			kfree(ddr_data);
			return -EINVAL;
		}
		ret = strncpy_s(ddr_data + pos, IP_NAME_SIZE, g_ddr_info_name[i],
				min_t(size_t, strlen(g_ddr_info_name[i]), IP_NAME_SIZE - 1));
		if (ret != EOK) {
			pr_err("%s %d copy ddr info name fail\n", __func__, __LINE__);
			kfree(ddr_data);
			return -EINVAL;
		}
		pos += IP_NAME_SIZE;
		for (j = 0; j < ddr_freq_num; j++) {
			ddr_data_temp = (u64)g_dubai_ddr_info[i][j];
			if (pos >= ddr_data_len) {
				pr_err("%s %d pos over flow\n", __func__, __LINE__);
				kfree(ddr_data);
				return -EINVAL;
			}
			ret = memcpy_s(ddr_data + pos, sizeof(u64), &ddr_data_temp, sizeof(u64));
			if (ret != EOK) {
				pr_err("%s %d copy ddr data fail\n", __func__, __LINE__);
				kfree(ddr_data);
				return -EINVAL;
			}
			pos += sizeof(u64);
		}
	}

	if (copy_to_user(argp, ddr_data, ddr_data_len) != 0) {
		pr_err("copy ddr data to user fail\n");
		ret = -EFAULT;
	}

	kfree(ddr_data);
	return ret;
}

#ifdef CONFIG_ITS
static int ioctrl_get_cpu_power(void __user *argp)
{
	int ret;

	if (g_its_cpu_power_data == NULL || g_its_cpu_power_data_size == 0)
		return -EFAULT;

	ret = get_its_power_result(g_its_cpu_power_data);
	if (ret != 0) {
		pr_err("dpm get ITS cpu power fail, ret %d\n", ret);
		return -EFAULT;
	}
	if (copy_to_user(argp, g_its_cpu_power_data,
			 g_its_cpu_power_data_size) != 0) {
		pr_err("dpm get g_its_cpu_power_data failed!\n");
		return -EFAULT;
	}
	return OK;
}

static int ioctrl_reset_cpu_power(const void __user *argp)
{
	int ret;

	(void)argp;
	ret = reset_power_result();
	if (ret != 0)
		pr_err("dpm reset ITS power data fail!\n");

	return ret;
}
#endif

static struct ioctrl_dpm_ldm_t dpm_ldm_func_table[] = {
	{ DPM_IOCTL_GET_SUPPORT, ioctrl_get_version },
	{ DPM_IOCTL_GET_IP_LENGTH, ioctrl_get_ip_len },
	{ DPM_IOCTL_GET_PERIOD_RANGE, ioctrl_get_sample_time },
	{ DPM_IOCTL_GET_IP_INFO, ioctrl_get_device_data },
#ifdef CONFIG_FREQDUMP_PLATFORM
	{ DPM_IOCTL_GET_DDR_FREQ_COUNT, ioctrl_get_ddr_freq_count },
	{ DPM_IOCTL_GET_DDR_FREQ_INFO, ioctrl_get_ddr_freq_info },
#endif
#ifdef CONFIG_ITS
	{ DPM_IOCTL_GET_CPU_POWER, ioctrl_get_cpu_power },
#endif
#ifdef CONFIG_LOWPM_STATS
	{ DPM_IOCTL_LPM3_VOTE_NUM, ioctrl_get_lpmcu_vote_num },
	{ DPM_IOCTL_LPM3_VOTE_DATA, ioctrl_lpmcu_vote_data },
	{ DPM_IOCTL_DDR_VOTE_NUM, ioctrl_get_ddr_vote_num },
	{ DPM_IOCTL_DDR_VOTE_DATA, ioctrl_ddr_vote_data },
	{ DPM_IOCTL_DDR_VOTE_DATA_CLR, ioctrl_clr_ddr_vote_data },
#endif
#ifdef CONFIG_DPM_PLATFORM_MERCURY
	{ DUBAI_IOCTL_GET_PERI_SIZE_INFO, ioctrl_get_dubai_peri_size },
	{ DUBAI_IOCTL_GET_PERI_DATA_INFO, ioctrl_get_dubai_peri_data },
#endif
};

static struct const_ioctrl_dpm_ldm_t dpm_ldm_const_func_table[] = {
	/* [100-25000] unit:millisecond default:1000 */
	{ DPM_IOCTL_SET_PERIOD, ioctrl_set_sample_time },
	{ DPM_IOCTL_SET_ENABLE, ioctrl_enable },
#ifdef CONFIG_ITS
	{ DPM_IOCTL_RESET_CPU_POWER, ioctrl_reset_cpu_power },
#endif
};

static long dpm_loadmonitor_ioctrl(struct file *file, u_int cmd, u_long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int ret = -EINVAL;
	size_t i;

	if (argp == NULL || file == NULL)
		return  -EFAULT;
	down(&g_dpm_sem);
	pr_err("%s type:%u.\n", __func__, cmd);
	for (i = 0; i < ARRAY_SIZE(dpm_ldm_func_table); i++) {
		if (dpm_ldm_func_table[i].ioctrl_cmd == cmd) {
			ret = dpm_ldm_func_table[i].ioctrl_func(argp);
			up(&g_dpm_sem);
			return ret;
		}
	}
	for (i = 0; i < ARRAY_SIZE(dpm_ldm_const_func_table); i++) {
		if (dpm_ldm_const_func_table[i].ioctrl_cmd == cmd) {
			ret = dpm_ldm_const_func_table[i].ioctrl_func(argp);
			break;
		}
	}
	up(&g_dpm_sem);
	return ret;
}

static int dpmldm_pm_callback(struct notifier_block *nb,
			      unsigned long action, void *ptr)
{
	if (nb == NULL || ptr == NULL)
		pr_info("dpmldm only for sc!\n");

	down(&g_dpm_sem_loadmonitor);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		pr_info("loadmonitor suspend +:status%u.\n",
			g_loadmonitor_status);
		if (g_loadmonitor_status == LOADMONITOR_ON) {
			cancel_delayed_work_sync(&g_dpm_delayed_work);
			sec_loadmonitor_switch_disable();
			g_loadmonitor_status = LOADMONITOR_SUSPEND;
		}
		pr_info("loadmonitor suspend -:status%u.\n",
			g_loadmonitor_status);
		break;

	case PM_POST_SUSPEND:
		pr_info("loadmonitor resume +:status%u.\n",
			g_loadmonitor_status);
		if (g_loadmonitor_status == LOADMONITOR_SUSPEND) {
			enable_loadmonitor_and_queue();
			g_ddr_suspend_switch = 1;
			g_loadmonitor_status = LOADMONITOR_ON;
		}
		pr_info("loadmonitor resume -:status%u.\n",
			g_loadmonitor_status);
		break;

	default:
		up(&g_dpm_sem_loadmonitor);
		return NOTIFY_DONE;
	}

	up(&g_dpm_sem_loadmonitor);
	return NOTIFY_OK;
}

static const struct file_operations dpm_loadmonitor_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dpm_loadmonitor_ioctrl,
};

static struct notifier_block dpmldm_pm_notif_block = {
	.notifier_call = dpmldm_pm_callback,
};

void clear_dpm_data(void)
{
	int i, ret;

	for (i = 0; i < MAX_MONITOR_IP_DEVICE; i++) {
#ifdef CONFIG_FREQDUMP_PLATFORM
		g_monitor_info.mdev[i].idle_nor_time = 0;
		g_monitor_info.mdev[i].busy_nor_time = 0;
		g_monitor_info.mdev[i].busy_low_time = 0;
		g_monitor_info.mdev[i].off_time = 0;
#else
		g_monitor_info.mdev[i].idle_time = 0;
		g_monitor_info.mdev[i].busy_time = 0;
#endif
	}
	g_ddr_bw_rd = 0;
	g_ddr_bw_wr = 0;
	ret = memset_s(g_dpm_data, g_dpm_data_size, 0, g_dpm_data_size);
	if (ret != EOK) {
		pr_err("%s g_dpm_data!\n", __func__);
		return;
	}
#ifdef CONFIG_DPM_PLATFORM_MERCURY
	ret = clear_dubai_peri_data();
	if (ret != EOK)
		pr_err("%s clear peri data error\n", __func__);
#endif
}

#ifdef CONFIG_FREQDUMP_PLATFORM
void calc_monitor_time(void)
{
	u64 idle_nor_rate = 0;
	u64 busy_nor_rate = 0;
	u64 busy_low_rate = 0;
	u64 off_rate = 0;
	unsigned long index_monitor_delay = 0;
	int i;
#ifndef CONFIG_LIBLINUX
	int ao_max_index = 0;
	int peri_max_index = 0;
#endif

	for (i = 0; i < MAX_MONITOR_IP_DEVICE; i++) {
		index_monitor_delay = g_monitor_info.monitor_delay;

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
		if (i < MAX_PERI_DEVICE - MAX_MEDIA_DEVICE) { /* zone peri */
#else
		if (i < MAX_PERI_DEVICE) { /* zone peri */
#endif
			idle_nor_rate =
				((u64)(g_dpm_loadmonitor_data_peri[i] /
				LOADMONITOR_PCLK) * 1000) /
				index_monitor_delay;
			busy_nor_rate =
				((u64)(g_dpm_loadmonitor_data_peri[
					MAX_PERI_DEVICE + i] /
				       LOADMONITOR_PCLK) * 1000) /
				index_monitor_delay;
			busy_low_rate =
				((u64)(g_dpm_loadmonitor_data_peri[
					MAX_PERI_DEVICE * 2 + i] /
				       LOADMONITOR_PCLK) * 1000) /
				index_monitor_delay;
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
		} else if ((i < MAX_PERI_DEVICE) &&
			   (i >= MAX_PERI_DEVICE - MAX_MEDIA_DEVICE)) {
			u32 media_pclk =
				g_media_info[INFO_MONITOR_FREQ] / 1000;
			idle_nor_rate =
				((u64)(g_dpm_loadmonitor_data_peri[i] /
					media_pclk) * 1000000) /
				index_monitor_delay;
			if (g_calc_monitor_time_media_hook == NULL) {
				busy_nor_rate =
					((u64)(g_dpm_loadmonitor_data_peri[
						MAX_PERI_DEVICE + i] /
					       media_pclk) * 1000000) /
					index_monitor_delay;
				busy_low_rate =
					((u64)(g_dpm_loadmonitor_data_peri[
						MAX_PERI_DEVICE * 2 + i] /
					       media_pclk) * 1000000) /
					index_monitor_delay;
			} else {
				g_calc_monitor_time_media_hook(
						i, media_pclk,
						&busy_nor_rate,
						&busy_low_rate,
						index_monitor_delay);
			}
#endif
		} else {   /* zone ao */
#ifndef CONFIG_LIBLINUX
			peri_max_index = i - MAX_PERI_DEVICE;
			ao_max_index = MAX_AO_DEVICE * MONITOR_STAT_NUM + i -
				       MAX_PERI_DEVICE;
			index_monitor_delay =
				g_dpm_loadmonitor_data_ao[ao_max_index] *
				index_monitor_delay;

			/* AO : mdev[i] read failed */
			if (peri_max_index < 0 || index_monitor_delay == 0) {
				pr_err("AO ip%d read failed,index:%d\n", i,
				       peri_max_index);
				return;
			}

			idle_nor_rate =
				((g_dpm_loadmonitor_data_ao[peri_max_index] *
				  1000000) / LOADMONITOR_AO_CLK) /
				index_monitor_delay;
			busy_nor_rate =
				((g_dpm_loadmonitor_data_ao[peri_max_index +
							    MAX_AO_DEVICE] *
				 1000000) / LOADMONITOR_AO_CLK) /
				index_monitor_delay;
			busy_low_rate =
				((g_dpm_loadmonitor_data_ao[peri_max_index + 2 *
							    MAX_AO_DEVICE] *
				 1000000) / LOADMONITOR_AO_CLK) /
				index_monitor_delay;
#else
			continue;
#endif
		}

		if ((idle_nor_rate + busy_nor_rate + busy_low_rate) > 1000)
			off_rate = 0;
		else
			off_rate = 1000 - idle_nor_rate - busy_nor_rate -
				   busy_low_rate;

		g_monitor_info.mdev[i].idle_nor_time +=
			g_monitor_info.monitor_delay * idle_nor_rate / 1000000;
		g_monitor_info.mdev[i].busy_nor_time +=
			g_monitor_info.monitor_delay * busy_nor_rate / 1000000;
		g_monitor_info.mdev[i].busy_low_time +=
			g_monitor_info.monitor_delay * busy_low_rate / 1000000;
		g_monitor_info.mdev[i].off_time +=
			g_monitor_info.monitor_delay * off_rate / 1000000;
	}
}
#else
void calc_monitor_time(void)
{
	unsigned int rate = 0;
	int i;

	for (i = 0; i < MAX_MONITOR_IP_DEVICE; i++) {
		if (i <= HF_MONITOR_NR)
			rate = ((g_dpm_loadmonitor_data[i] /
				 g_monitor_info.div_high) * 1000) /
			       (g_monitor_info.monitor_delay *
				LOADMONITOR_PCLK);
		else
			rate = ((g_dpm_loadmonitor_data[i]  /
				 g_monitor_info.div_low) * 1000) /
			       (g_monitor_info.monitor_delay *
				LOADMONITOR_PCLK);

		g_monitor_info.mdev[i].idle_time +=
			g_monitor_info.monitor_delay * (1000 - rate) / 1000000;
		g_monitor_info.mdev[i].busy_time +=
			g_monitor_info.monitor_delay * rate / 1000000;
	}
}
#endif

void calc_ddr_bw(void)
{
	unsigned int temp_rd = 0, temp_wr = 0;
	int ret;

	ret = memcpy_s(g_ddr_old, sizeof(g_ddr_old), g_ddr_new,
		       sizeof(g_ddr_old));
	if (ret != EOK) {
		pr_err("%s g_ddr_old!\n", __func__);
		return;
	}
	/* DMC0 */
	g_ddr_new[0] = g_dpm_freqdump_data->dmc_flux_rd[0];
	g_ddr_new[1] = g_dpm_freqdump_data->dmc_flux_wr[0];
	if (DDR_TYPE == 4) {
		/* DMC1 */
		g_ddr_new[0] += g_dpm_freqdump_data->dmc_flux_rd[1];
		g_ddr_new[1] += g_dpm_freqdump_data->dmc_flux_wr[1];
		/* DMC3 */
		g_ddr_new[0] += g_dpm_freqdump_data->dmc_flux_rd[3];
		g_ddr_new[1] += g_dpm_freqdump_data->dmc_flux_wr[3];
	}
	/* DMC2 */
	g_ddr_new[0] += g_dpm_freqdump_data->dmc_flux_rd[2];
	g_ddr_new[1] += g_dpm_freqdump_data->dmc_flux_wr[2];

	temp_rd =
		(g_ddr_new[0] >= g_ddr_old[0]) ? (g_ddr_new[0] - g_ddr_old[0]) :
		((0xffffffff - g_ddr_old[0]) + g_ddr_new[0] + 1);
	temp_wr =
		(g_ddr_new[1] >= g_ddr_old[1]) ? (g_ddr_new[1] - g_ddr_old[1]) :
		((0xffffffff - g_ddr_old[1]) + g_ddr_new[1] + 1);
	if (g_ddr_suspend_switch == 1) {
		g_ddr_bw_rd += 0;
		g_ddr_bw_wr += 0;
		g_ddr_suspend_switch = 0;
	} else {
		g_ddr_bw_rd += (temp_rd / 1000000) << 4;
		g_ddr_bw_wr += (temp_wr / 1000000) << 4;
	}
}

static int clear_mem_data(void)
{
	int ret;

#ifdef CONFIG_FREQDUMP_PLATFORM
	/* init loadmonitor & ddr_bw buf */
	ret = memset_s(g_dpm_freqdump_data, sizeof(struct freqdump_data_bos),
		       0, sizeof(struct freqdump_data_bos));
	if (ret != EOK) {
		pr_err("%s freqdump_data_bos!\n", __func__);
		return -EFAULT;
	}

	/* get data from bl31 */
	ret = memset_s(g_dpm_loadmonitor_data_peri,
		       sizeof(g_dpm_loadmonitor_data_peri), 0,
		       sizeof(g_dpm_loadmonitor_data_peri));
	if (ret != EOK) {
		pr_err("%s g_dpm_loadmonitor_data_peri!\n",
		       __func__);
		return -EFAULT;
	}
	ret = memset_s(g_dpm_loadmonitor_data_ao,
		       sizeof(g_dpm_loadmonitor_data_ao), 0,
		       sizeof(g_dpm_loadmonitor_data_ao));
	if (ret != EOK) {
		pr_err("%s g_dpm_loadmonitor_data_ao!\n", __func__);
		return -EFAULT;
	}
#else
	/* init loadmonitor & ddr_bw buf */
	ret = memset_s(g_dpm_freqdump_data, sizeof(struct freqdump_data), 0,
		       sizeof(struct freqdump_data));
	if (ret != EOK) {
		pr_err("%s ddr_bw_dpm_freqdump_data!\n", __func__);
		return -EFAULT;
	}

	/* get data from bl31 */
	ret = memset_s(g_dpm_loadmonitor_data, sizeof(g_dpm_loadmonitor_data),
		       0, sizeof(g_dpm_loadmonitor_data));
	if (ret != EOK) {
		pr_err("%s bl31_dpm_loadmonitor_data!\n", __func__);
		return -EFAULT;
	}
#endif
	return ret;
}

void update_dpm_data(struct work_struct *work)
{
	int i, ret;
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	u32 delay_time_media;
#endif

	(void)work;

	down(&g_dpm_sem_update);
	pr_info("%s: begin.\n", __func__);

#ifdef CONFIG_FREQDUMP_PLATFORM
	ret = clear_mem_data();
	if (ret != EOK) {
		up(&g_dpm_sem_update);
		return;
	}
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	delay_time_media = (g_dpm_period / DPM_PERIOD_MIN / 10) *
			   g_media_info[INFO_MONITOR_FREQ];
	ret = media_monitor_read(delay_time_media);
	if (ret != 0) {
		pr_err("%s media_loadmonitor_read error!\n", __func__);
		up(&g_dpm_sem_update);
		return;
	}
#else
	sec_loadmonitor_data_read(ALL_MONITOR_EN);
#endif

	memcpy_fromio((void *)g_dpm_loadmonitor_data_peri,
		      (void *)(g_loadmonitor_virt_addr),
		      sizeof(g_dpm_loadmonitor_data_peri));
	if (ao_loadmonitor_read(g_dpm_loadmonitor_data_ao)) {
		pr_err("%s ao_loadmonitor_read error!\n", __func__);
		up(&g_dpm_sem_update);
		return;
	}
#else
	ret = clear_mem_data();
	if (ret != EOK) {
		up(&g_dpm_sem_update);
		return;
	}
	sec_loadmonitor_data_read();
	memcpy_fromio((void *)g_dpm_loadmonitor_data,
		      (void *)(g_loadmonitor_virt_addr),
		      sizeof(g_dpm_loadmonitor_data));
#endif
	calc_monitor_time();
	sec_freqdump_read();
#ifdef CONFIG_FREQDUMP_PLATFORM
	/* this can not return, for user print */
	ret = monitor_ipc_send();
	if (ret)
		pr_err("hisi_dpm monitor_ipc_send error.\n");
	memcpy_fromio(g_dpm_freqdump_data, (void *)(g_freqdump_virt_addr),
		      sizeof(struct freqdump_data_bos));
#ifdef CONFIG_DPM_PLATFORM_MERCURY
	format_dubai_peri_data(g_dpm_freqdump_data->peri_vote[0],
			       g_dpm_freqdump_data->peri_vote[1]);
#endif
#else
	memcpy_fromio(g_dpm_freqdump_data, (void *)(g_freqdump_virt_addr),
		      sizeof(struct freqdump_data));
#endif
	calc_ddr_bw();
	g_dpm_data->length = MAX_MONITOR_IP_DEVICE + 1;
	for (i = 0; i < MAX_MONITOR_IP_DEVICE; i++) {
		g_dpm_data->data[i].busy_nor_time =
			g_monitor_info.mdev[i].busy_nor_time;
		g_dpm_data->data[i].idle_nor_time =
			g_monitor_info.mdev[i].idle_nor_time;
		g_dpm_data->data[i].busy_low_time =
			g_monitor_info.mdev[i].busy_low_time;
		g_dpm_data->data[i].off_time = g_monitor_info.mdev[i].off_time;
		ret = strncpy_s(g_dpm_data->data[i].name, IP_NAME_SIZE,
				g_monitor_info.mdev[i].name,
				strlen(g_monitor_info.mdev[i].name));
		if (ret != EOK) {
			pr_err("g_dpm_data->data[i].name error.\n");
			up(&g_dpm_sem_update);
			return;
		}
	}
	g_dpm_data->data[MAX_MONITOR_IP_DEVICE].off_time = g_ddr_bw_rd;
	g_dpm_data->data[MAX_MONITOR_IP_DEVICE].busy_nor_time = g_ddr_bw_wr;
	g_dpm_data->data[MAX_MONITOR_IP_DEVICE].busy_low_time = 0;
	g_dpm_data->data[MAX_MONITOR_IP_DEVICE].idle_nor_time = 0;

	ret = strncpy_s(g_dpm_data->data[MAX_MONITOR_IP_DEVICE].name,
			IP_NAME_SIZE, "DDR_BW", strlen("DDR_BW"));
	if (ret != EOK) {
		pr_err("g_dpm_data->data[MAX_MONITOR_IP_DEVICE].name error.\n");
		up(&g_dpm_sem_update);
		return;
	}

	pr_info("%s end.\n", __func__);
	up(&g_dpm_sem_update);
	queue_delayed_work(g_monitor_dpm_wq, &g_dpm_delayed_work,
			   msecs_to_jiffies(g_monitor_info.monitor_delay /
					    1000));
}

static int dpmldm_setup_cdev(struct dpm_cdev *dev, dev_t devno)
{
	int err;

	cdev_init(&dev->cdev, &dpm_loadmonitor_fops);
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		pr_err("%s failed !\n", __func__);

	return err;
}

static void dpm_sema_init(void)
{
	sema_init(&g_dpm_sem, 1);
	sema_init(&g_dpm_sem_update, 1);
	sema_init(&g_dpm_sem_loadmonitor, 1);
}

static int dpm_malloc_data(void)
{
	int ret = 0;

	g_dpm_data_size = sizeof(struct dpm_transmit_t) +
			  sizeof(struct ip_info) * (MAX_MONITOR_IP_DEVICE + 1);
	g_dpm_data = kzalloc(g_dpm_data_size, GFP_KERNEL);
	if (g_dpm_data == NULL) {
		ret = -ENOMEM;
		pr_err("g_dpm_data malloc failed!");
	}
	return ret;
}

#ifdef CONFIG_ITS
static its_malloc_data(void)
{
	int ret = 0;

	g_its_cpu_power_data_size = sizeof(its_cpu_power_t);
	g_its_cpu_power_data = kzalloc(g_its_cpu_power_data_size, GFP_KERNEL);
	if (g_its_cpu_power_data == NULL) {
		ret = -ENOMEM;
		pr_err("g_its_cpu_power_data kzalloc failed!");
	}
	return ret;
}
#endif

static int dpm_malloc_devno(dev_t *pdevno)
{
	int ret = 0;

	if (g_dpmldm_major != 0)
		ret = register_chrdev_region(*pdevno, 1, DPM_DEV_NAME);
	else
		ret = alloc_chrdev_region(pdevno, 0, 1, DPM_DEV_NAME);

	if (ret < 0) {
		pr_err("%s failed!", __func__);
		ret = -ENOMEM;
	} else {
		g_dpmldm_major = MAJOR(*pdevno);
	}
	return ret;
}

static int __init dpmldm_init(void)
{
	int ret = 0;
	struct device *pdevice = NULL;
	struct class *dpmldm_class = NULL;
	dev_t devno = MKDEV(g_dpmldm_major, 0);

	dpm_sema_init();

	if (dpm_malloc_data() != 0)
		goto fail_to_malloc_dpm_data;
#ifdef CONFIG_ITS
	if (its_malloc_data() != 0)
		goto fail_to_kzalloc_its_power_data;
#endif
	dpm_common_platform_init();

	if (dpm_malloc_devno(&devno) != 0)
		goto fail_to_register_chrdev_region;
	g_dpm_ldm_cdevp = kzalloc(sizeof(*g_dpm_ldm_cdevp), GFP_KERNEL);
	if (g_dpm_ldm_cdevp == NULL) {
		ret = -ENOMEM;
		pr_err("dpmldm fail_malloc\n");
		goto fail_to_kzalloc_mem;
	}
	ret = dpmldm_setup_cdev(g_dpm_ldm_cdevp, devno);
	if (ret) {
		pr_err("hisi dpm: cdev add error\n");
		goto fail_to_setup_cdev;
	}

	dpmldm_class = class_create(THIS_MODULE, DPM_DEV_NAME);
	if (IS_ERR(dpmldm_class)) {
		ret = -EFAULT;
		pr_err("hisi dpm: class_create error\n");
		goto fail_to_setup_cdev;
	}

	pdevice = device_create(dpmldm_class, NULL,
				(dev_t)MKDEV(g_dpmldm_major, 0),
				NULL, DPM_DEV_NAME);

	if (IS_ERR(pdevice)) {
		ret = -EFAULT;
		pr_err("hisi dpm: device_create error\n");
		goto fail_to_create_device;
	}

	g_dpm_freqdump_data = kzalloc(sizeof(*g_dpm_freqdump_data), GFP_KERNEL);
	if (g_dpm_freqdump_data == NULL) {
		ret = -ENOMEM;
		pr_err("dpm alloc mem failed!\n");
		goto fail_to_kzalloc_dpm_data_mem;
	}

	register_pm_notifier(&dpmldm_pm_notif_block);

	g_monitor_dpm_wq = create_freezable_workqueue("monitor-dpm-queue");
	if (g_monitor_dpm_wq == NULL) {
		pr_err("hisi dpm: alloc_workqueue error\n");
		ret = -EFAULT;
		goto fail_to_alloc_workqueue;
	}
	INIT_DELAYED_WORK(&g_dpm_delayed_work, update_dpm_data);

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	ret = platform_driver_register(&g_media_dpm_driver);
	if (ret) {
		pr_err("hisi dpm: register media dpm driver failed.\n");
		goto fail_to_reg_media;
	}
#endif

	pr_err("%s: success!\n", __func__);
	return ret;

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
fail_to_reg_media:
#endif
fail_to_alloc_workqueue:
fail_to_kzalloc_dpm_data_mem:
	device_destroy(dpmldm_class, (dev_t)MKDEV(g_dpmldm_major, 0));
fail_to_create_device:
	class_destroy(dpmldm_class);
fail_to_setup_cdev:
	kfree(g_dpm_ldm_cdevp);
	g_dpm_ldm_cdevp = NULL;
fail_to_kzalloc_mem:
	unregister_chrdev_region(devno, 1);
#ifdef CONFIG_ITS
	kfree(g_its_cpu_power_data);
	g_its_cpu_power_data = NULL;
fail_to_kzalloc_its_power_data:
#endif
fail_to_register_chrdev_region:
	kfree(g_dpm_data);
	g_dpm_data = NULL;
fail_to_malloc_dpm_data:
	return ret;
}

static void __exit dpmldm_exit(void)
{
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	platform_driver_unregister(&g_media_dpm_driver);
#endif
	pr_err("hisi_dpmldm removed!\n");
}
module_init(dpmldm_init);
module_exit(dpmldm_exit);
