/*
 * perf_ctrl.c
 *
 * interfaces defined for perf_ctrl
 *
 * Copyright (c) 2016-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "perf_ctrl: " fmt

#include <linux/compat.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/perf_ctrl.h>

static unsigned long get_dev_cap(void)
{
	unsigned long cap = 0;

	cap |= BIT(CAP_AI_SCHED_COMM_CMD);
#ifdef CONFIG_SCHED_RTG
	cap |= BIT(CAP_RTG_CMD);
#endif
#ifdef CONFIG_RENDER_RT
	cap |= BIT(CAP_RENDER_RT_CMD);
#endif

	return cap;
}

static int perf_ctrl_get_dev_cap(void __user *uarg)
{
	unsigned long cap = get_dev_cap();

	if (uarg == NULL)
		return -EINVAL;

	if (copy_to_user(uarg, &cap, sizeof(unsigned long))) {
		pr_err("%s: copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

typedef int (*perf_ctrl_func)(void __user *arg);

static perf_ctrl_func g_func_array[PERF_CTRL_MAX_NR] = {
	NULL, /* reserved */
	perf_ctrl_get_sched_stat,
	NULL,
	perf_ctrl_get_ipa_stat,
	perf_ctrl_get_ddr_flux,
	perf_ctrl_get_related_tid,
	perf_ctrl_get_drg_dev_freq,
	perf_ctrl_get_thermal_cdev_power,
	perf_ctrl_set_frame_rate,
	perf_ctrl_set_frame_margin,
	perf_ctrl_set_frame_status,
	perf_ctrl_set_task_rtg,
	perf_ctrl_set_rtg_cpus,
	perf_ctrl_set_rtg_freq,
	perf_ctrl_set_rtg_freq_update_interval,
	perf_ctrl_set_rtg_util_invalid_interval,
	perf_ctrl_get_gpu_fence,
	init_render_rthread,
	get_render_rthread,
	stop_render_rthread,
	get_render_hrthread,
	destroy_render_rthread,
	perf_ctrl_set_rtg_load_mode,
	perf_ctrl_set_rtg_ed_params,
	perf_ctrl_get_dev_cap,
	perf_ctrl_set_vip_prio,
	perf_ctrl_set_favor_small_cap,
	perf_ctrl_set_task_rtg_min_freq,
	perf_ctrl_set_task_min_util,
	perf_ctrl_get_gpu_buffer_size,
	perf_ctrl_lb_set_policy,
	perf_ctrl_lb_reset_policy,
	perf_ctrl_lb_lite_info,
	perf_ctrl_enable_gpu_lb,
	perf_ctrl_get_cpu_busy_time,
	perf_ctrl_set_task_max_util,
	perf_ctrl_get_task_yield_time,
	perf_ctrl_set_task_l3c_part,

	/* NOTE: add from here */
};

/* main function entry point */
static long perf_ctrl_ioctl(struct file *file, unsigned int cmd,
			    unsigned long arg)
{
	void __user *uarg = (void __user *)(uintptr_t)arg;
	unsigned int func_id = _IOC_NR(cmd);

	if (uarg == NULL) {
		pr_err("%s: invalid user uarg\n", __func__);
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != PERF_CTRL_MAGIC) {
		pr_err("%s: PERF_CTRL_MAGIC fail, TYPE=%d\n",
		       __func__, _IOC_TYPE(cmd));
		return -EINVAL;
	}

	if (func_id >= PERF_CTRL_MAX_NR) {
		pr_err("%s: PERF_CTRL_MAX_NR fail, _IOC_NR(cmd)=%d, MAX_NR=%d\n",
		       __func__, _IOC_NR(cmd), PERF_CTRL_MAX_NR);
		return -EINVAL;
	}

	if (g_func_array[func_id] == NULL) {
		pr_err("cmd error, here is default, cmd=%u\n", cmd);
		return -EINVAL;
	}

	return (*g_func_array[func_id])(uarg);
}

#ifdef CONFIG_COMPAT
static long perf_ctrl_compact_ioctl(struct file *file,
				    unsigned int cmd, unsigned long arg)
{
	return perf_ctrl_ioctl(file, cmd, (unsigned long)(compat_ptr(arg)));
}
#endif

static int perf_ctrl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int perf_ctrl_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations perf_ctrl_fops = {
	.owner = THIS_MODULE,
#ifdef CONFIG_COMPAT
	.compat_ioctl = perf_ctrl_compact_ioctl,
#endif
	.unlocked_ioctl = perf_ctrl_ioctl,
	.open = perf_ctrl_open,
	.release = perf_ctrl_release,
};

static struct miscdevice perf_ctrl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hisi_perf_ctrl",
	.fops = &perf_ctrl_fops,
};

static int __init perf_ctrl_dev_init(void)
{
	int err;

	err = misc_register(&perf_ctrl_device);
	if (err != 0)
		return err;

	return 0;
}

static void __exit perf_ctrl_dev_exit(void)
{
	misc_deregister(&perf_ctrl_device);
}

module_init(perf_ctrl_dev_init);
module_exit(perf_ctrl_dev_exit);
