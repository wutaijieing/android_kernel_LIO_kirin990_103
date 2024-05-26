/*
 * ddr_tracking.c
 *
 * tracking for ddr
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/hwspinlock.h>
#include <asm/uaccess.h>
#include <global_ddr_map.h>
#include <ddr_define.h>

#define DTS_NAME_DDR_TRACKING		"hisilicon,ddr_tracking"
#define DDR_DFS_FILE_LIMIT		0660
#define DDR_DFS_CMD_MAX_SIZE		16
/* ddr dfs trakcing file buf is 4KB */
#define DDR_DFS_FILE_BUF_SIZE		4096
#define DDR_DFS_HWLOCK_ID		22
#define DDR_DFS_HWLOCK_TIMEOUT		1000
/* ddr dfs tracking file maximun size is 1GB */
#define DDR_DFS_TRACK_FILE_SIZE		0x40000000UL
#define DDR_DFS_TRACK_SIZE		1024
#define DDR_DFS_TRACK_EXT_SIZE		32
#define TIMESPEC_NUM		2

struct ddrdfs_tracking_lpbuf {
	unsigned int wp;
	unsigned int timespec_l;
	unsigned int timespec_h;
	unsigned char buf[DDR_DFS_TRACK_SIZE + DDR_DFS_TRACK_EXT_SIZE];
};

struct hisi_ddr_tracking_info {
	struct platform_device *pdev;
	void __iomem *dfs_base;
	bool dfs_status;
	struct file *dfs_fp;
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *debugfs_root;
#endif
};

struct ddrdfs_tracking_apbuf {
	unsigned int wp;
	unsigned char buf[DDR_DFS_FILE_BUF_SIZE];
};

const char *g_dfs_tracking_pname = "/data/hisi_logs/running_trace/ddr_dfs";
static struct hisi_ddr_tracking_info *g_tracking_info;
static struct ddrdfs_tracking_apbuf *g_dfs_tracking_apbuf;
static struct task_struct *g_dfs_task;
static struct hwspinlock *g_ddr_dfs_hwlock;
static DEFINE_MUTEX(g_dfs_tracking_lock);

static int hisi_ddr_dfs_tracking_save(void)
{
	int count, remain_size;
	int ret = 0;
	unsigned char *buf = NULL;
	long file_size;
	mm_segment_t old_fs;

	remain_size = g_dfs_tracking_apbuf->wp;
	if (remain_size == 0)
		return 0;

	if (remain_size > DDR_DFS_FILE_BUF_SIZE)
		remain_size = DDR_DFS_FILE_BUF_SIZE;

	g_dfs_tracking_apbuf->wp = 0;
	buf = g_dfs_tracking_apbuf->buf;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	while (remain_size > 0) {
		if (g_tracking_info->dfs_fp->f_pos >=
		    (loff_t)DDR_DFS_TRACK_FILE_SIZE) {
			ret = -EFBIG;
			pr_err("ddr dfs tracking file is too big, stop tracking!\n");
			goto exit_save;
		}

		count = vfs_write(g_tracking_info->dfs_fp, buf, remain_size,
				  &g_tracking_info->dfs_fp->f_pos);
		if (count <= 0) {
			pr_err("write dfs tracking file failed!\n");
			ret = -EFAULT;
			goto exit_save;
		}
		remain_size -= count;
		buf += count;
	}

exit_save:
	set_fs(old_fs);
	return ret;
}

static int dfs_thread_func(void *data)
{
	int ret;
	unsigned int timespec_prev[TIMESPEC_NUM] = {0};
	unsigned int timespec_now[TIMESPEC_NUM];
	unsigned int lp_wp, ap_wp;
	unsigned char *lp_buf = NULL;
	unsigned char *ap_buf = NULL;
	struct ddrdfs_tracking_lpbuf *lpbuf =
		(struct ddrdfs_tracking_lpbuf *)g_tracking_info->dfs_base;

	g_tracking_info->dfs_fp = filp_open(g_dfs_tracking_pname,
					    O_CREAT | O_WRONLY,
					    DDR_DFS_FILE_LIMIT);
	if (IS_ERR(g_tracking_info->dfs_fp)) {
		pr_err("open dfs tracking file failed!\n");
		goto exit_without_close;
	}

	while (!kthread_should_stop()) {
		timespec_now[0] = lpbuf->timespec_l;
		timespec_now[1] = lpbuf->timespec_h;

		if (timespec_prev[0] == timespec_now[0] &&
		    timespec_prev[1] == timespec_now[1])
			goto sleep;

		timespec_prev[0] = timespec_now[0];
		timespec_prev[1] = timespec_now[1];

		mutex_lock(&g_dfs_tracking_lock);
		ret = hwspin_lock_timeout(g_ddr_dfs_hwlock,
					  DDR_DFS_HWLOCK_TIMEOUT);
		BUG_ON((int)(IS_ERR(ret)));

		lp_wp = lpbuf->wp;
		lp_buf = lpbuf->buf;
		ap_wp = g_dfs_tracking_apbuf->wp;
		ap_buf = g_dfs_tracking_apbuf->buf;

		if ((lp_wp + ap_wp) > DDR_DFS_FILE_BUF_SIZE) {
			hwspin_unlock(g_ddr_dfs_hwlock);
			if (hisi_ddr_dfs_tracking_save() != 0) {
				mutex_unlock(&g_dfs_tracking_lock);
				goto exit_with_close;
			}
			mutex_unlock(&g_dfs_tracking_lock);
			continue;
		}

		ret = memcpy_s(ap_buf + ap_wp, DDR_DFS_FILE_BUF_SIZE - ap_wp,
			       lp_buf, lp_wp);
		if (ret != 0)
			pr_err("copy from lp_buf fail!\n");

		g_dfs_tracking_apbuf->wp += lp_wp;

		hwspin_unlock(g_ddr_dfs_hwlock);
		mutex_unlock(&g_dfs_tracking_lock);
sleep:
		msleep(1);
	}
	hisi_ddr_dfs_tracking_save();
exit_with_close:
	filp_close(g_tracking_info->dfs_fp, NULL);
	g_tracking_info->dfs_fp = NULL;

exit_without_close:
	g_tracking_info->dfs_status = false;
	return 0;
}

static int hisi_ddr_dfs_tracking_show(struct seq_file *m, void *v)
{
	if (m == NULL)
		return -EINVAL;
	seq_printf(m, "%s\n", g_tracking_info->dfs_status ?
		   "running" : "stopped");

	return 0;
}

static int hisi_ddr_dfs_tracking_open(struct inode *inode, struct file *file)
{
	return single_open(file, hisi_ddr_dfs_tracking_show, inode->i_private);
}

static ssize_t hisi_ddr_dfs_tracking_write(struct file *filp,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
	char kbuf[DDR_DFS_CMD_MAX_SIZE] = {0};
	int ret = 0;
	bool status = false;

	if (buf == NULL || count == 0 || count >= DDR_DFS_CMD_MAX_SIZE) {
		pr_err("buf is NULL!\n");
		ret = -EINVAL;
		goto exit_without_unlock;
	}

	if (copy_from_user(kbuf, buf, count) != 0) {
		pr_err("copy from user fail!\n");
		ret = -EFAULT;
		goto exit_without_unlock;
	}

	ret = strtobool(kbuf, &status);
	if (ret < 0) {
		pr_err("invalid status!\n");
		goto exit_without_unlock;
	}

	mutex_lock(&g_dfs_tracking_lock);

	if (!g_tracking_info->dfs_status && status) {
		g_dfs_task = kthread_run(dfs_thread_func, NULL, "dfs_tracking");
		if (g_dfs_task == NULL) {
			ret = -EFAULT;
			goto exit_with_unlock;
		}
		g_tracking_info->dfs_status = status;
	}

	if (g_tracking_info->dfs_status && !status) {
		kthread_stop(g_dfs_task);
		g_dfs_task = NULL;
	}

	/* write success */
	ret = count;

exit_with_unlock:
	mutex_unlock(&g_dfs_tracking_lock);
exit_without_unlock:
	return ret;
}

static const struct file_operations g_hisi_ddr_tracking_dfs_fops = {
	.open = hisi_ddr_dfs_tracking_open,
	.read = seq_read,
	.write = hisi_ddr_dfs_tracking_write,
	.release = single_release,
};

static int hisi_ddr_tracking_probe(struct platform_device *pdev)
{
	struct hisi_ddr_tracking_info *info = NULL;
	struct resource *res = NULL;
	int ret = 0;

	info = devm_kzalloc(&pdev->dev, sizeof(struct hisi_ddr_tracking_info),
			    GFP_KERNEL);
	if (info == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	g_dfs_tracking_apbuf = (struct ddrdfs_tracking_apbuf *)
		devm_kzalloc(&pdev->dev, sizeof(struct ddrdfs_tracking_apbuf),
			     GFP_KERNEL);
	if (g_dfs_tracking_apbuf == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "get resource failed!");
		ret = -ENXIO;
		goto exit;
	}

	info->dfs_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->dfs_base)) {
		dev_err(&pdev->dev, "unable to get resource!\n");
		ret = PTR_ERR(info->dfs_base);
		goto exit;
	}

	g_ddr_dfs_hwlock = hwspin_lock_request_specific(DDR_DFS_HWLOCK_ID);
	if (g_ddr_dfs_hwlock == NULL) {
		dev_err(&pdev->dev, "unable to get ddr dfs hwspin lock!\n");
		ret = PTR_ERR(g_ddr_dfs_hwlock);
		goto exit;
	}

#ifdef CONFIG_DFX_DEBUG_FS
	info->debugfs_root = debugfs_create_dir("ddr_tracking", NULL);
	if (info->debugfs_root == NULL) {
		dev_err(&pdev->dev, "create root debugfs failed!!\n");
		ret = -ENOENT;
		goto exit;
	}

	if (debugfs_create_file("dfs", S_IRUSR | S_IWUSR, info->debugfs_root,
				NULL, &g_hisi_ddr_tracking_dfs_fops) == NULL) {
		dev_err(&pdev->dev, "create debugfs file failed!\n");
		debugfs_remove_recursive(info->debugfs_root);
		ret = -ENOENT;
		goto exit;
	}
#endif
	info->pdev = pdev;
	platform_set_drvdata(pdev, info);
	g_tracking_info = info;

exit:
	dev_info(&pdev->dev, "initialize ddr tracking %s.\n",
		 ret ? "failed!" : "success.");
	return ret;
}

static int hisi_ddr_tracking_remove(struct platform_device *pdev)
{
	struct hisi_ddr_tracking_info *info = platform_get_drvdata(pdev);

#ifdef CONFIG_DFX_DEBUG_FS
	debugfs_remove_recursive(info->debugfs_root);
#endif
	platform_set_drvdata(pdev, NULL);

	if (g_dfs_task != NULL) {
		kthread_stop(g_dfs_task);
		g_dfs_task = NULL;
	}

	g_tracking_info = NULL;
	g_dfs_tracking_apbuf = NULL;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id g_hisi_ddr_tracking_of_match[] = {
	{ .compatible = DTS_NAME_DDR_TRACKING, },
	{ },
};
MODULE_DEVICE_TABLE(of, g_hisi_ddr_tracking_of_match);
#endif

static struct platform_driver g_hisi_ddr_tracking_driver = {
	.probe = hisi_ddr_tracking_probe,
	.remove = hisi_ddr_tracking_remove,
	.driver = {
		.name = DTS_NAME_DDR_TRACKING,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_hisi_ddr_tracking_of_match),
	},
};

module_platform_driver(g_hisi_ddr_tracking_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DDR Tracking Dirver");
