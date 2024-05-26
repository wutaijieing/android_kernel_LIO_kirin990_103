/*
 * loadmonitor_k.c
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

#include "loadmonitor_kernel.h"
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
#include <global_ddr_map.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <securec.h>
#include <dubai_peri_kernel.h>
#include <loadmonitor_media_kernel.h>
#include <monitor_ap_m3_ipc.h>
#include <loadmonitor_common_kernel.h>
#ifdef CONFIG_LOWPM_STATS
#include <linux/platform_drivers/dubai_lp_stats.h>
#endif

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)		"[loadmonitor&freqdump]:" fmt

#define DDR_BW_RW_BIT_OFFSET		4
#define DMC_CHANNEL_SIZE		4
#define FREQDUMP_REG_LEN		2

static struct semaphore g_on_off_loadmonitor;
static struct semaphore g_rw_loadmonitor;
unsigned int g_dpm_period = (1000000 * LOADMONITOR_PCLK);

static unsigned int g_ddr_bw_rd;
static unsigned int g_ddr_bw_wr;
static unsigned int g_ddr_suspend_switch;
static struct delayed_work g_dpm_delayed_work;
static struct workqueue_struct *g_monitor_dpm_wq;

#define LOADMONITOR_SUSPEND		2
#define LOADMONITOR_RESUME		3
#define LOADMONITOR_DATA_ERR	(-1)
#define LOADMONITOR_CLEAR_ERR	(-2)
unsigned int g_loadmonitor_status = LOADMONITOR_OFF;
void __iomem *g_loadmonitor_virt_addr;
struct monitor_dump_info g_monitor_info;

static unsigned int g_dpmldm_major = 225;
static struct dpm_transmit_t *g_dpm_data;
static size_t g_dpm_data_size;
static struct dpm_cdev *g_dpm_ldm_cdevp;
static u64 g_ao_data_begin_mdelay;
static u64 g_ao_data_end_mdelay;
static bool g_open_sr_ao_monitor;
static s64 g_print_err_data;
static bool g_has_find_err_log;

phys_addr_t freqdump_phy_addr;
phys_addr_t loadmonitor_phy_addr;
void __iomem *g_freqdump_virt_addr;

static void update_ao_data_time_interval(void)
{
	g_ao_data_begin_mdelay = g_ao_data_end_mdelay;
	g_ao_data_end_mdelay = div_u64(ktime_get_boottime_ns(), NSEC_PER_MSEC);
}

static void set_loadmonitor_enable_flag(void)
{
	if (g_monitor_delay_value.monitor_enable_flags == ENABLE_NONE)
		g_monitor_delay_value.monitor_enable_flags = ENABLE_ALL;
	pr_info("%s monitor enable flags:%#X\n",
		__func__, g_monitor_delay_value.monitor_enable_flags);
}

static int enable_monitor(void)
{
	if (g_loadmonitor_status == LOADMONITOR_OFF) {
		set_loadmonitor_enable_flag();
		sec_loadmonitor_switch_enable(false);

		g_ao_data_begin_mdelay = div_u64(ktime_get_boottime_ns(),
						 NSEC_PER_MSEC);
		g_ao_data_end_mdelay = g_ao_data_begin_mdelay;

		g_monitor_info.monitor_delay =
			g_dpm_period / LOADMONITOR_PCLK;
		queue_delayed_work(g_monitor_dpm_wq, &g_dpm_delayed_work,
				   msecs_to_jiffies(g_monitor_info.monitor_delay /
						    1000));
		g_loadmonitor_status = LOADMONITOR_ON;
		if (g_print_err_data != -1)
			g_print_err_data = 0;
		g_has_find_err_log = false;
		pr_info("%s dpm on success, delay time:%lldms, data count:%lld\n",
			__func__, g_monitor_info.monitor_delay / 1000, g_print_err_data);
		return 0;
	} else if (g_loadmonitor_status == LOADMONITOR_ON) {
		pr_err("%s monitor has on\n", __func__);
		return 0;
	}
	pr_err("%s monitor on err, monitor status:%u\n", __func__,
	       g_loadmonitor_status);
	return -EINVAL;
}

static int disable_monitor(void)
{
	int monitor_clear;

	if (g_loadmonitor_status == LOADMONITOR_ON) {
		cancel_delayed_work_sync(&g_dpm_delayed_work);
		sec_loadmonitor_switch_disable(
			g_monitor_delay_value.monitor_enable_flags,
			false);
		monitor_clear = clear_dpm_data();
		g_loadmonitor_status = LOADMONITOR_OFF;
		pr_info("%s dpm off success, flag:0x%x,%d\n", __func__,
			g_monitor_delay_value.monitor_enable_flags, monitor_clear);
		return 0;
	} else if (g_loadmonitor_status == LOADMONITOR_OFF) {
		pr_err("%s monitor has off\n", __func__);
		return 0;
	}

	pr_err("%s monitor off err, monitor status:%u\n", __func__,
	       g_loadmonitor_status);
	return -EINVAL;
}

static int ioctrl_enable(const void __user *argp)
{
	int dpm_switch;
	int ret = OK;

	if (copy_from_user(&dpm_switch, argp, sizeof(dpm_switch)) != 0) {
		ret = -EINVAL;
		return ret;
	}

	if (dpm_switch == DPM_ON) {
		ret = enable_monitor();
	} else if (dpm_switch == DPM_OFF) {
		ret = disable_monitor();
	} else {
		pr_err("dpm_ctrl input invalid parameter!\n");
		ret = -EINVAL;
	}
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

	g_dpm_data->length = MAX_NUM_LOGIC_IP_DEVICE + 1;
	dpm_ipnum = MAX_NUM_LOGIC_IP_DEVICE + 1;
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
			pr_err("dpm_period get version failed\n");
			ret = -EFAULT;
		}
	} else if (dpm_switch == DPM_GET_MIN_PERIOD) {
		period = DPM_PERIOD_MIN / (LOADMONITOR_PCLK * 1000);
		if (copy_to_user(argp, &period, sizeof(period)) != 0) {
			pr_err("dpm_period get version failed\n");
			ret = -EFAULT;
		}
	} else {
		pr_err("dpm_ctrl input invalid parameter\n");
		ret = -EINVAL;
	}
	return ret;
}

static int ioctrl_enable_sr_ao_monitor(const void __user *argp)
{
	int open_sr_ao_monitor = 0;
	int ret = OK;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&open_sr_ao_monitor, argp,
			   sizeof(open_sr_ao_monitor)) != 0) {
		return -EINVAL;
	}
	g_open_sr_ao_monitor = (bool)open_sr_ao_monitor;
	pr_info("%s ao open at sr:%d\n", __func__, (int)g_open_sr_ao_monitor);
	return ret;
}

static int ioctrl_on_off_err_log(const void __user *argp)
{
	int err_log = 0;
	int ret = OK;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&err_log, argp,
			   sizeof(err_log)) != 0) {
		return -EINVAL;
	}

	if (err_log)
		g_print_err_data = -1;
	else
		g_print_err_data = 0;

	pr_info("%s print err data:%lld\n", __func__, g_print_err_data);
	return ret;
}

static int ioctrl_get_device_data(void __user *argp)
{
	int ret = OK;

	down(&g_rw_loadmonitor);
	if (g_dpm_data != NULL && g_dpm_data_size > 0) {
		if (copy_to_user(argp, g_dpm_data, g_dpm_data_size) != 0) {
			pr_err("dev->g_dpm_data_buf get failed\n");
			ret = -EFAULT;
		}
	} else {
		pr_err("copy dpm_data fail\n");
		ret = -EINVAL;
	}
	up(&g_rw_loadmonitor);
	return ret;
}

/* on suspend time u can not enable loadmonitor */
static int ioctrl_read_clear_disable_enable(void __user *argp)
{
	int ret;

	ret = ioctrl_get_device_data(argp);
	if (ret != OK)
		return LOADMONITOR_DATA_ERR;
	ret = clear_dpm_data();
	if (ret != OK)
		return LOADMONITOR_CLEAR_ERR;
	ret = disable_monitor();
	if (ret != OK) {
		pr_err("%s disable error.loadmonitor enter sr:%u\n", __func__,
		       g_loadmonitor_status);
		return 0;
	}
	ret = enable_monitor();
	if (ret != OK) {
		pr_err("%s enable error.loadmonitor enter sr:%u\n", __func__,
		       g_loadmonitor_status);
		return 0;
	}
	return 0;
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
	if (copy_to_user(argp, &ddr_freq_num, sizeof(ddr_freq_num)) != 0) {
		pr_err(" dpm get ddr freq num failed\n");
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

	ddr_data_len = (IP_NAME_SIZE + (ddr_freq_num * sizeof(u64))) * DDR_INFO_LEN;
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

static int ioctrl_set_device_enable_flags(const void __user *argp)
{
	enum EN_FLAGS monitor_flags;

	if (copy_from_user(&monitor_flags, argp, sizeof(monitor_flags)) != 0)
		return -EINVAL;
	if (monitor_flags != ENABLE_NONE &&
	    ((u32)monitor_flags & ENABLE_ALL) == (u32)monitor_flags) {
		g_monitor_delay_value.monitor_enable_flags = (u32)monitor_flags;
	} else {
		pr_err("%s set flags error:%#X\n", __func__,
		       (u32)monitor_flags);
		return -EINVAL;
	}

	pr_info("%s set flags:%#X\n", __func__, g_monitor_delay_value.monitor_enable_flags);
	return 0;
}

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
static int ioctrl_set_sample_time(const void __user *argp)
{
	unsigned int period;

	if (copy_from_user(&period, argp, sizeof(period)) != OK)
		return -EINVAL;

	if (period > DPM_PERIOD_MAX_SEC || period < DPM_PERIOD_MIN_SEC) {
		pr_err("%s %d dpm period is invalid %u\n", __func__, __LINE__, period);
		return -EINVAL;
	}

	g_monitor_delay_value.peri0 = period * (LOADMONITOR_PERI_FREQ_CS / 1000);
	g_monitor_delay_value.peri1 = period * (LOADMONITOR_PERI_FREQ_CS / 1000);
	g_monitor_delay_value.ao = period * (LOADMONITOR_AO_FREQ / 1000);
	g_monitor_delay_value.media0 = period * (LOADMONITOR_MEDIA0_FREQ / 1000);
	g_monitor_delay_value.media1 = period * (LOADMONITOR_MEDIA1_FREQ / 1000);
	g_dpm_period = period * LOADMONITOR_PCLK * 1000;
	pr_err("%s flag:0x%x,sample_time:%ums,dpm_period:%u,peri0:%u,peri1:%u,ao:%u,"
	       "media0:%u,media1:%u\n", __func__, g_monitor_delay_value.monitor_enable_flags,
	       period, g_dpm_period, g_monitor_delay_value.peri0,
	       g_monitor_delay_value.peri1, g_monitor_delay_value.ao, g_monitor_delay_value.media0,
	       g_monitor_delay_value.media1);

	return OK;
}
#else
static int ioctrl_set_sample_time(const void __user *argp)
{
	unsigned int period;

	if (copy_from_user(&period, argp, sizeof(period)) != OK)
		return -EINVAL;

	if (period > DPM_PERIOD_MAX_SEC || period < DPM_PERIOD_MIN_SEC) {
		pr_err("%s %d dpm period is invalid %u\n", __func__, __LINE__, period);
		return -EINVAL;
	}

	g_monitor_delay_value.peri0 = period * (LOADMONITOR_PERI_FREQ_CS / 1000);
	g_monitor_delay_value.peri1 = period * (LOADMONITOR_PERI_FREQ_CS / 1000);
	g_monitor_delay_value.ao = period * (LOADMONITOR_AO_FREQ / 1000);
	g_dpm_period = period * LOADMONITOR_PCLK * 1000;
	pr_err("%s flag:0x%x,sample_time:%ums,dpm_period:%u,peri0:%u,peri1:%u,ao:%u\n", __func__,
	       g_monitor_delay_value.monitor_enable_flags,
	       period, g_dpm_period, g_monitor_delay_value.peri0,
	       g_monitor_delay_value.peri1, g_monitor_delay_value.ao);

	return OK;
}
#endif

static struct ioctrl_dpm_ldm_t dpm_ldm_func_table[] = {
	{ DPM_IOCTL_GET_SUPPORT, ioctrl_get_version },
	{ DPM_IOCTL_GET_IP_LENGTH, ioctrl_get_ip_len },
	{ DPM_IOCTL_GET_PERIOD_RANGE, ioctrl_get_sample_time },
	{ DPM_IOCTL_GET_IP_INFO, ioctrl_get_device_data },
	{ DPM_IOCTL_GET_DDR_FREQ_COUNT, ioctrl_get_ddr_freq_count },
	{ DPM_IOCTL_GET_DDR_FREQ_INFO, ioctrl_get_ddr_freq_info },
#ifdef CONFIG_DPM_HWMON
	{ DPM_IOCTL_DPM_PERI_NUM, ioctl_dpm_get_peri_num },
	{ DPM_IOCTL_DPM_PERI_DATA, ioctl_dpm_get_peri_data },
	{ DPM_IOCTL_DPM_GPU_DATA, ioctl_dpm_get_gpu_data },
	{ DPM_IOCTL_DPM_NPU_DATA, ioctl_dpm_get_npu_data },
#ifdef CONFIG_MODEM_DPM
	{ DPM_IOCTL_DPM_MODEM_NUM, ioctl_dpm_get_mdm_num },
	{ DPM_IOCTL_DPM_MODEM_DATA, ioctl_dpm_get_mdm_data },
#endif
#endif
#ifdef CONFIG_LOWPM_STATS
	{ DPM_IOCTL_LPM3_VOTE_NUM, ioctrl_get_lpmcu_vote_num },
	{ DPM_IOCTL_LPM3_VOTE_DATA, ioctrl_lpmcu_vote_data },
	{ DPM_IOCTL_DDR_VOTE_NUM, ioctrl_get_ddr_vote_num },
	{ DPM_IOCTL_DDR_VOTE_DATA, ioctrl_ddr_vote_data },
	{ DPM_IOCTL_DDR_VOTE_DATA_CLR, ioctrl_clr_ddr_vote_data },
#endif
	{ DUBAI_IOCTL_GET_PERI_SIZE_INFO, ioctrl_get_dubai_peri_size },
	{ DUBAI_IOCTL_GET_PERI_DATA_INFO, ioctrl_get_dubai_peri_data },
	{ DUBAI_IOCTL_READ_CLEAR_DISABLE_ENABLE,
	 ioctrl_read_clear_disable_enable },
#ifdef CONFIG_ITS
	{ DPM_IOCTL_GET_CPU_POWER, ioctrl_get_cpu_power },
	{ DPM_IOCTL_RESET_CPU_POWER, ioctrl_reset_cpu_power },
#endif
};

static struct const_ioctrl_dpm_ldm_t dpm_ldm_const_func_table[] = {
	/* [100-25000] unit:millisecond default:1000 */
	{ DPM_IOCTL_SET_PERIOD,  ioctrl_set_sample_time },
	{ DPM_IOCTL_SET_ENABLE, ioctrl_enable },
	{ DPM_IOCTL_SET_FLAGS, ioctrl_set_device_enable_flags },
	{ DUBAI_IOCTL_ENABLE_SR_AO_MONITOR, ioctrl_enable_sr_ao_monitor },
	{ DUBAI_IOCTL_ON_OFF_ERR_LOG, ioctrl_on_off_err_log },
#ifdef CONFIG_DPM_HWMON
	{ DPM_IOCTL_DPM_ENABLE, ioctl_dpm_enable_module },
#endif
};

static long dpm_loadmonitor_ioctrl(struct file *file, u_int cmd, u_long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int ret = -EINVAL;
	size_t i;

	if (argp == NULL || file == NULL)
		return -EFAULT;
	down(&g_on_off_loadmonitor);
	for (i = 0; i < ARRAY_SIZE(dpm_ldm_func_table); i++) {
		if (dpm_ldm_func_table[i].ioctrl_cmd == cmd) {
			ret = dpm_ldm_func_table[i].ioctrl_func(argp);
			up(&g_on_off_loadmonitor);
			return ret;
		}
	}
	for (i = 0; i < ARRAY_SIZE(dpm_ldm_const_func_table); i++) {
		if (dpm_ldm_const_func_table[i].ioctrl_cmd == cmd) {
			ret = dpm_ldm_const_func_table[i].ioctrl_func(argp);
			break;
		}
	}
	up(&g_on_off_loadmonitor);
	return ret;
}

static int dpmldm_pm_callback(struct notifier_block *nb, unsigned long action, void *ptr)
{
	if (nb == NULL || ptr == NULL)
		pr_info("dpmldm only for sc\n");

	down(&g_on_off_loadmonitor);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		pr_info("loadmonitor suspend +:status%u\n",
			g_loadmonitor_status);
		if (g_loadmonitor_status == LOADMONITOR_ON) {
			cancel_delayed_work_sync(&g_dpm_delayed_work);
			sec_loadmonitor_switch_disable(
				g_monitor_delay_value.monitor_enable_flags,
				g_open_sr_ao_monitor);
			g_loadmonitor_status = LOADMONITOR_SUSPEND;
		}
		pr_info("loadmonitor suspend -:status%u\n",
			g_loadmonitor_status);
		break;

	case PM_POST_SUSPEND:
		pr_info("loadmonitor resume +:status%u\n", g_loadmonitor_status);
		if (g_loadmonitor_status == LOADMONITOR_SUSPEND) {
			sec_loadmonitor_switch_enable(g_open_sr_ao_monitor);
			g_monitor_info.monitor_delay = g_dpm_period /
						       LOADMONITOR_PCLK;
			queue_delayed_work(g_monitor_dpm_wq, &g_dpm_delayed_work,
					   msecs_to_jiffies(g_monitor_info.monitor_delay /
							    1000));
			g_ddr_suspend_switch = 1;
			g_loadmonitor_status = LOADMONITOR_ON;
		}
		pr_info("loadmonitor resume -:status%u\n", g_loadmonitor_status);
		break;

	default:
		up(&g_on_off_loadmonitor);
		return NOTIFY_DONE;
	}

	up(&g_on_off_loadmonitor);
	return NOTIFY_OK;
}

static const struct file_operations g_dpm_loadmonitor_fops = {
	.owner = THIS_MODULE,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dpm_loadmonitor_ioctrl,
#endif
	.unlocked_ioctl = dpm_loadmonitor_ioctrl,
};

static struct notifier_block g_dpmldm_pm_notif_block = {
	.notifier_call = dpmldm_pm_callback,
};

int clear_dpm_data(void)
{
	int ret;
	size_t i;

	down(&g_rw_loadmonitor);
	for (i = 0; i < MAX_NUM_LOGIC_IP_DEVICE; i++) {
		g_monitor_info.mdev[i].idle_nor_time = 0;
		g_monitor_info.mdev[i].busy_nor_time = 0;
		g_monitor_info.mdev[i].busy_low_time = 0;
		g_monitor_info.mdev[i].off_time = 0;
		g_monitor_info.mdev[i].name[0] = '\0';
	}
	g_ddr_bw_rd = 0;
	g_ddr_bw_wr = 0;
	ret = memset_s(g_dpm_data, g_dpm_data_size, 0, g_dpm_data_size);
	if (ret != EOK) {
		pr_err("%s memset_s monitor err, ret:%d\n", __func__, ret);
		up(&g_rw_loadmonitor);
		return ret;
	}
	ret = clear_dubai_peri_data();
	up(&g_rw_loadmonitor);
	return ret;
}

static bool ip_need_calc(struct ips_info *info, int ip_index)
{
	return ((info->flag &
		g_monitor_delay_value.monitor_enable_flags) ==
		info->flag && (info->mask & BIT((u32)ip_index)) != 0);
}

static inline bool is_ao_data(struct ips_info *info)
{
	return info->name == LOADMONITOR_AO_TICK;
}

static int set_device_sample_status(struct ips_info *info,
				    struct ip_status *status,
				    int dev_index, int sensor_index)
{
	u64 idle_nor_time;
	u64 busy_nor_time;
	u64 busy_low_time;
	u64 off_time;
	unsigned int freq;
	u64 ao_mdelay;
	u64 ao_on_mtime;
	u32 samples;
	int ret;

	freq = info->freq;
	samples = info->samples;

	if (freq == 0) {
		pr_err("%s freq equal zero", __func__);
		return -1;
	};
	idle_nor_time = 1000 * (u64)status->idle / freq;
	busy_nor_time = 1000 * (u64)status->busy_norm_volt / freq;
	busy_low_time = 1000 * (u64)status->buys_low_volt / freq;

	if (g_print_err_data != -1)
		++g_print_err_data;
	if (is_ao_data(info)) {
		idle_nor_time = idle_nor_time * samples;
		busy_nor_time = busy_nor_time * samples;
		busy_low_time = busy_low_time * samples;
		ao_mdelay = g_ao_data_end_mdelay - g_ao_data_begin_mdelay;
		ao_on_mtime = idle_nor_time + busy_nor_time + busy_low_time;
		if (ao_on_mtime > ao_mdelay)
			off_time = 0;
		else
			off_time = ao_mdelay - ao_on_mtime;
	} else {
		if (idle_nor_time + busy_nor_time + busy_low_time > 1000) {
#ifndef CONFIG_COMMON_FREQDUMP_PLATFORM
			off_time = 1000;
			idle_nor_time = 0;
			busy_nor_time = 0;
			busy_low_time = 0;
#else
			off_time = 0;
			if (g_print_err_data == -1 ||
			    g_print_err_data <= MAX_MONITOR_IP_DEVICE) {
				pr_info("%s freq:%u, idle:%u, norm:%u, low:%u, dev_index:%d, sensor_index:%d\n",
					__func__, freq, status->idle, status->busy_norm_volt,
					status->buys_low_volt, dev_index, sensor_index);
				g_has_find_err_log = true;
			}
#endif
		} else {
			off_time = 1000 - idle_nor_time - busy_nor_time -
				   busy_low_time;
		}
	}

	ret = sprintf_s(g_monitor_info.mdev[dev_index].name,
			IP_NAME_MAX_SIZE, "%c%02d",
			info->name,
			sensor_index);
	if (ret == -1) {
		pr_err("%s sprintf_s err device type:%c, index:%d\n",
		       __func__, info->name, sensor_index);
		return ret;
	}
	g_monitor_info.mdev[dev_index].idle_nor_time += idle_nor_time;
	g_monitor_info.mdev[dev_index].busy_nor_time += busy_nor_time;
	g_monitor_info.mdev[dev_index].busy_low_time += busy_low_time;
	g_monitor_info.mdev[dev_index].off_time += off_time;
	return ret;
}

static void calc_monitor_time(void)
{
	int ret, i, j;
	int ips_num = sizeof(struct loadmonitor_ips) / sizeof(struct ips_info);
	int dev_index;
	struct ip_status *status = NULL;
	struct ips_info *info = (struct ips_info *)(&g_loadmonitor_data_kernel.ips);

	if (!g_has_find_err_log && g_print_err_data != -1)
		g_print_err_data = 0;

	for (i = 0; i < ips_num; i++, info++) {
		status = info->ip;
		for (j = 0; j < MAX_SENSOR_NUM; j++, status++) {
			dev_index = i * MAX_SENSOR_NUM + j;
			if (ip_need_calc(info, j)) {
				ret = set_device_sample_status(info, status,
							       dev_index, j);
				if (ret == -1)
					continue;
			} else {
				ret = sprintf_s(g_monitor_info.mdev[dev_index].name,
						IP_NAME_MAX_SIZE, "%c%c%c",
						info->name, '-', '-');
				if (ret == -1) {
					pr_err("%s ip type:%c, index:%d\n",
						__func__, info->name, j);
					continue;
				}
			}
		}
	}
}

static void calc_ddr_bw(void)
{
#if defined DDR_BANDWIDTH_CALC_IN_ATF
	if (g_ddr_suspend_switch == 1) {
		g_ddr_bw_rd += 0;
		g_ddr_bw_wr += 0;
		g_ddr_suspend_switch = 0;
	} else {
		g_ddr_bw_rd += g_loadmonitor_data_kernel.ddr.rd_bandwidth;
		g_ddr_bw_wr += g_loadmonitor_data_kernel.ddr.wr_bandwidth;
	}
#endif
}

void update_dpm_data(struct work_struct *work)
{
	size_t i;
	int ret;

	down(&g_rw_loadmonitor);
	pr_info("%s begin\n", __func__);

	(void)work;
	(void)memset_s((void *)(&g_loadmonitor_data_kernel), sizeof(g_loadmonitor_data_kernel), 0,
		       sizeof(g_loadmonitor_data_kernel));

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	g_monitor_delay_value.media0 = (g_dpm_period / DPM_PERIOD_MIN / 10) *
		LOADMONITOR_MEDIA_FREQ * 1000000;
	g_monitor_delay_value.media1 = g_monitor_delay_value.media0;
#endif
	read_format_cs_loadmonitor_data(g_monitor_delay_value.monitor_enable_flags);
	update_ao_data_time_interval();

	calc_monitor_time();
	format_dubai_peri_data(&g_loadmonitor_data_kernel.peri);

	ret = monitor_ipc_send();
	if (ret != 0)
		pr_err("dpm : monitor_ipc_send error\n");

	calc_ddr_bw();
	g_dpm_data->length = MAX_NUM_LOGIC_IP_DEVICE + 1;
	for (i = 0; i < MAX_NUM_LOGIC_IP_DEVICE; i++) {
		g_dpm_data->data[i].busy_nor_time =
			g_monitor_info.mdev[i].busy_nor_time;
		g_dpm_data->data[i].idle_nor_time =
			g_monitor_info.mdev[i].idle_nor_time;
		g_dpm_data->data[i].busy_low_time =
			g_monitor_info.mdev[i].busy_low_time;
		g_dpm_data->data[i].off_time =
			g_monitor_info.mdev[i].off_time;
		ret = strncpy_s(g_dpm_data->data[i].name, IP_NAME_SIZE,
				g_monitor_info.mdev[i].name, IP_NAME_MAX_SIZE);
		if (ret != EOK)
			pr_err("%s dpm strncpy_s is err. ret:%d\n",
			       __func__, ret);
	}
	g_dpm_data->data[MAX_NUM_LOGIC_IP_DEVICE].off_time = g_ddr_bw_rd;
	g_dpm_data->data[MAX_NUM_LOGIC_IP_DEVICE].busy_nor_time = g_ddr_bw_wr;
	g_dpm_data->data[MAX_NUM_LOGIC_IP_DEVICE].busy_low_time = 0;
	g_dpm_data->data[MAX_NUM_LOGIC_IP_DEVICE].idle_nor_time = 0;

	ret = strncpy_s(g_dpm_data->data[MAX_NUM_LOGIC_IP_DEVICE].name,
			IP_NAME_SIZE, "DDR_BW--", strlen("DDR_BW--"));
	if (ret != EOK)
		pr_err("%s ddr strncpy_s is err. ret:%d.\n", __func__, ret);
	up(&g_rw_loadmonitor);
	queue_delayed_work(g_monitor_dpm_wq, &g_dpm_delayed_work,
			   msecs_to_jiffies(g_monitor_info.monitor_delay / 1000));
}

static int dpmldm_setup_cdev(struct dpm_cdev *dev, dev_t devno)
{
	int ret;

	cdev_init(&dev->cdev, &g_dpm_loadmonitor_fops);
	ret = cdev_add(&dev->cdev, devno, 1);
	if (ret != 0)
		pr_err("%s failed !\n", __func__);
	return ret;
}

/* contain loadmonitor and freqdump */
static int ioremap_addr(void)
{
	struct device_node *np = NULL;
	int ret;
	u32 data[FREQDUMP_REG_LEN] = {0};

	np = of_find_compatible_node(NULL, NULL, "platform,freqdump");
	if (np == NULL) {
		pr_err("%s : dts%s node not found\n",
		       __func__, "platform,freqdump");
		return -ENODEV;
	}
	ret = of_property_read_u32_array(np, "reg", data, FREQDUMP_REG_LEN);
	if (ret != 0) {
		pr_err("%s get freqdump attribute failed\n",
		       __func__);
		ret = -ENODEV;
		goto fail_to_read_reg;
	}
	of_node_put(np);
	np = NULL;

	if (data[0] != SHARED_MEMORY_OFFSET)
		pr_err("%s shared mem offset error. kernel:%d, bl31:%d\n",
		       __func__, data[0], SHARED_MEMORY_OFFSET);

	freqdump_phy_addr = ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE +
			    data[0];
	loadmonitor_phy_addr = freqdump_phy_addr;

	g_freqdump_virt_addr = ioremap(freqdump_phy_addr, (size_t)data[1]);
	if (g_freqdump_virt_addr == NULL) {
		pr_err("%s freqdump ioremap failed\n", __func__);
		ret = -ENOMEM;
		goto fail_to_ioremap;
	}
	if (data[1] != SHARED_MEMORY_SIZE)
		pr_err("%s shared mem size error. kernel:%d, bl31:%d\n",
		       __func__, data[0], SHARED_MEMORY_SIZE);

	memset_io(g_freqdump_virt_addr, 0, (size_t)data[1]);
	g_loadmonitor_virt_addr = g_freqdump_virt_addr;
	return 0;
fail_to_read_reg:
	of_node_put(np);
	np = NULL;
fail_to_ioremap:
	return ret;
}

static void free_dpm_ldm_data(void)
{
	if (g_dpm_data != NULL) {
		kfree(g_dpm_data);
		g_dpm_data = NULL;
	}
#ifdef CONFIG_ITS
	free_its_cpu_power_mem();
#endif
}

static int malloc_dpm_ldm_data(void)
{
	int ret;

	g_dpm_data_size = sizeof(struct dpm_transmit_t) +
			  sizeof(struct ip_info) *
			  (MAX_NUM_LOGIC_IP_DEVICE + 1);
	g_dpm_data = kzalloc(g_dpm_data_size, GFP_KERNEL);
	if (g_dpm_data == NULL) {
		ret = -ENOMEM;
		pr_err("%s dpm_data malloc failed\n", __func__);
		goto fail_malloc_mem;
	}
#ifdef CONFIG_ITS
	if (!create_its_cpu_power_mem()) {
		ret = -ENOMEM;
		pr_err("g_its_cpu_power_data kzalloc failed\n");
		goto fail_malloc_mem;
	}
#endif
	return 0;
fail_malloc_mem:
	free_dpm_ldm_data();
	return ret;
}

static int __init dpmldm_init(void)
{
	int ret = 0;
	struct device *pdevice = NULL;
	struct class *dpmldm_class = NULL;
	dev_t devno = MKDEV(g_dpmldm_major, 0);

	sema_init(&g_on_off_loadmonitor, 1);
	sema_init(&g_rw_loadmonitor, 1);

	if (g_dpmldm_major)
		ret = register_chrdev_region(devno, 1, DPM_DEV_NAME);
	else
		ret = alloc_chrdev_region(&devno, 0, 1, DPM_DEV_NAME);

	if (ret < 0) {
		pr_err("%s register device node failed\n", __func__);
		goto fail_to_register_chrdev_region;
	} else {
		g_dpmldm_major = MAJOR(devno);
	}
	g_dpm_ldm_cdevp = kzalloc(sizeof(*g_dpm_ldm_cdevp), GFP_KERNEL);
	if (g_dpm_ldm_cdevp == NULL) {
		ret = -ENOMEM;
		pr_err("%s fail_malloc\n", __func__);
		goto fail_to_kzalloc_mem;
	}
	ret = dpmldm_setup_cdev(g_dpm_ldm_cdevp, devno);
	if (ret != 0) {
		pr_err("%s cdev add error\n", __func__);
		goto fail_to_setup_cdev;
	}
	dpmldm_class = class_create(THIS_MODULE, DPM_DEV_NAME);
	if (IS_ERR(dpmldm_class)) {
		ret = -EFAULT;
		pr_err("%s class_create error\n", __func__);
		goto fail_to_setup_cdev;
	}

	pdevice = device_create(dpmldm_class, NULL,
				(dev_t)MKDEV(g_dpmldm_major, 0), NULL,
				DPM_DEV_NAME);

	if (IS_ERR(pdevice)) {
		ret = -EFAULT;
		pr_err("%s device_create error\n", __func__);
		goto fail_to_create_device;
	}

	register_pm_notifier(&g_dpmldm_pm_notif_block);

	g_monitor_dpm_wq = create_freezable_workqueue("monitor-dpm-queue");
	if (g_monitor_dpm_wq == NULL) {
		pr_err("%s alloc_workqueue error\n", __func__);
		ret = -EFAULT;
		goto fail_to_alloc_workqueue;
	}
	INIT_DELAYED_WORK(&g_dpm_delayed_work, update_dpm_data);

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	ret = platform_driver_register(&media_dpm_driver);
	if (ret != 0) {
		pr_err("%s register media dpm driver failed\n", __func__);
		goto fail_to_reg_media;
	}
#endif

	ret = ioremap_addr();
	if (ret != 0) {
		pr_err("%s ioremap err\n", __func__);
		goto fail_to_remap_err;
	}
	ret = malloc_dpm_ldm_data();
	if (ret != 0) {
		pr_err("%s dpm vote data malloc err\n", __func__);
		goto fail_to_remap_err;
	}
	pr_info("%s suscess\n", __func__);

	return ret;

fail_to_remap_err:
#ifdef CONFIG_ENABLE_MIDEA_MONITOR
	(void)platform_driver_unregister(&media_dpm_driver);
fail_to_reg_media:
#endif
	flush_workqueue(g_monitor_dpm_wq);
	destroy_workqueue(g_monitor_dpm_wq);
fail_to_alloc_workqueue:
	(void)unregister_pm_notifier(&g_dpmldm_pm_notif_block);
	device_destroy(dpmldm_class, (dev_t)MKDEV(g_dpmldm_major, 0));
fail_to_create_device:
	class_destroy(dpmldm_class);
fail_to_setup_cdev:
	kfree(g_dpm_ldm_cdevp);
	g_dpm_ldm_cdevp = NULL;
fail_to_kzalloc_mem:
	unregister_chrdev_region(devno, 1);
fail_to_register_chrdev_region:
	return ret;
}

static void __exit dpmldm_exit(void)
{
	if (g_freqdump_virt_addr != NULL) {
		iounmap(g_freqdump_virt_addr);
		g_freqdump_virt_addr = NULL;
	}
	free_dpm_ldm_data();
	(void)unregister_pm_notifier(&g_dpmldm_pm_notif_block);
	pr_err("%s dpmldm removed\n", __func__);
}

module_init(dpmldm_init);
module_exit(dpmldm_exit);
