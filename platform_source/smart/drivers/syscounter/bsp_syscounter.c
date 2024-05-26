/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: Syscounter driver
 * Create: 2016-06-28
 */

#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/time64.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <asm/arch_timer.h>
#include <linux/platform_drivers/bsp_syscounter.h>
#include <linux/syscore_ops.h>
#include <linux/workqueue.h>
#include <securec.h>

#define RECORD_NEED_SYNC_PERIOD  0

static struct syscnt_device *syscnt_dev;
/*lint -e580 -e644*/
int syscounter_to_timespec64(u64 syscnt, struct timespec64 *ts)
{
	struct syscnt_device *dev = syscnt_dev;
	unsigned long flags;
	time64_t sec_delta, r_tv_sec;
	long nsec_delta, r_tv_nsec;
	u64 cnt_delta, r_syscnt;

	if (ts == NULL)
		return -EINVAL;
	if (dev == NULL)
		return -ENXIO;

	spin_lock_irqsave(&dev->sync_lock, flags);
	r_syscnt = dev->record.syscnt;
	r_tv_sec = dev->record.ts.tv_sec;
	r_tv_nsec = dev->record.ts.tv_nsec;
	spin_unlock_irqrestore(&dev->sync_lock, flags);

	if (syscnt >= r_syscnt)
		cnt_delta = syscnt - r_syscnt;
	else
		cnt_delta = r_syscnt - syscnt;

	sec_delta = (time64_t)(cnt_delta / dev->clock_rate);
	nsec_delta = (long)((cnt_delta % dev->clock_rate) * NSEC_PER_SEC / dev->clock_rate);

	if (syscnt >= r_syscnt) {
		ts->tv_sec = r_tv_sec + sec_delta;
		ts->tv_nsec = r_tv_nsec + nsec_delta;
		if (ts->tv_nsec >= NSEC_PER_SEC) {
			ts->tv_sec++;
			ts->tv_nsec -= NSEC_PER_SEC;
		}
	} else {
		ts->tv_sec = r_tv_sec - sec_delta;
		if (r_tv_nsec >= nsec_delta) {
			ts->tv_nsec = r_tv_nsec - nsec_delta;
		} else {
			ts->tv_sec--;
			ts->tv_nsec = (NSEC_PER_SEC - nsec_delta) + r_tv_nsec;
		}
	}

	return 0;
}
EXPORT_SYMBOL(syscounter_to_timespec64);

u64 bsp_get_syscount(void)
{
	struct syscnt_device *dev = syscnt_dev;
	union syscnt_val syscnt;
	unsigned long flags;

	if (dev == NULL || dev->base == NULL)
		return 0;

	spin_lock_irqsave(&dev->r_lock, flags);
	syscnt.val_lh32.l32 = (u32)readl(dev->base + SYSCOUNTER_L32);
	syscnt.val_lh32.h32 = (u32)readl(dev->base + SYSCOUNTER_H32);
	spin_unlock_irqrestore(&dev->r_lock, flags);

	return syscnt.val;
}
EXPORT_SYMBOL(bsp_get_syscount);
/*lint +e580 -e592*/
#ifdef CONFIG_DFX_DEBUG_FS
static ssize_t show_current_time(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timespec64 ts = { 0 };
	struct timespec64 ts_convert = { 0 };
	u64 syscnt;
	u64 arch_timer;
	int count;
	struct syscnt_device *d = syscnt_dev;
	unsigned long flags;

	if (d == NULL) {
		count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1, "%s", "device not found\n");
		return count;
	}

	spin_lock_irqsave(&d->sync_lock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ktime_get_boottime_ts64(&ts);
#else
	get_monotonic_boottime64(&ts);
#endif
	syscnt = bsp_get_syscount();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	arch_timer = __arch_counter_get_cntvct();
#else
	arch_timer = arch_counter_get_cntvct();
#endif
	spin_unlock_irqrestore(&d->sync_lock, flags);
	(void)syscounter_to_timespec64(syscnt, &ts_convert);

	count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1,
			   "syscnt = %lld, arch_timer = %lld, tv.sec = %ld, tv.nsec = %ld, c_tv.sec = %ld, c_tv.nsec = %ld\n",
			   syscnt, arch_timer, ts.tv_sec, ts.tv_nsec, ts_convert.tv_sec, ts_convert.tv_nsec);

	return count;
}

static ssize_t show_current_record(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct syscnt_device *d = syscnt_dev;
	int count;

	if (d == NULL) {
		count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1, "%s", "device not found\n");
	} else {
		count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1,
				   "syscnt = %lld, tv.sec = %ld, tv.nsec = %ld\n",
				   d->record.syscnt, d->record.ts.tv_sec, d->record.ts.tv_nsec);
	}

	return count;
}
/*lint +e644*/
static ssize_t show_debug_syscount(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timespec64 ts_convert = { 0 };
	struct syscnt_device *d = syscnt_dev;
	int count;

	if (d == NULL) {
		count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1, "%s", "device not found\n");
	} else {
		(void)syscounter_to_timespec64(d->debug_syscnt, &ts_convert);
		count = snprintf_s(buf, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE - 1,
				   "%s : syscnt = %lld, tv.sec = %ld, tv.nsec = %ld\n",
				   __func__, d->debug_syscnt, ts_convert.tv_sec, ts_convert.tv_nsec);
	}

	return count;
}
/*lint +e592*/
static ssize_t store_debug_syscount(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	u64 val;
	struct syscnt_device *d = syscnt_dev;

	if (d == NULL)
		return -EINVAL;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	d->debug_syscnt = val;

	return size;
}

/*lint -save -e84 -e846 -e514 -e778 -e866*/
static DEVICE_ATTR(cur_time, S_IRUGO, show_current_time, NULL);
static DEVICE_ATTR(cur_record, S_IRUGO, show_current_record, NULL);
static DEVICE_ATTR(syscnt_debug, S_IRUGO | S_IWUSR, show_debug_syscount, store_debug_syscount);
/*lint -restore*/

static const struct attribute *syscnt_attributes[] = {
	&dev_attr_cur_time.attr,
	&dev_attr_cur_record.attr,
	&dev_attr_syscnt_debug.attr,
	NULL,
};
#endif

#if RECORD_NEED_SYNC_PERIOD
static void bsp_syscounter_sync_work(struct work_struct *work)
{
	unsigned long flags;
	struct syscnt_device *d = syscnt_dev;

	if (d == NULL)
		return;

	spin_lock_irqsave(&d->sync_lock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ktime_get_boottime_ts64(&d->record.ts);
	ktime_get_real_ts64(&d->record.utc);
#else
	get_monotonic_boottime64(&d->record.ts);
	getnstimeofday(&d->record.utc);
#endif
	d->record.syscnt = bsp_get_syscount();
	spin_unlock_irqrestore(&d->sync_lock, flags);

	schedule_delayed_work(&d->sync_record_work, round_jiffies_relative(msecs_to_jiffies(d->sync_interval)));
}
#endif

static int bsp_syscounter_suspend(void)
{
	struct syscnt_device *d = syscnt_dev;

	pr_info("%s ++", __func__);

	if (d == NULL) {
		pr_err("%s syscnt device is NULL\n", __func__);
		return -EXDEV;
	}

#if RECORD_NEED_SYNC_PERIOD
	(void)cancel_delayed_work(&d->sync_record_work);
#endif

	pr_info("%s --", __func__);

	return 0;
}

static void bsp_syscounter_resume(void)
{
	unsigned long flags;
	struct syscnt_device *d = syscnt_dev;

	pr_info("%s ++", __func__);

	if (d == NULL) {
		pr_err("%s syscnt device is NULL\n", __func__);
		return;
	}

	spin_lock_irqsave(&d->sync_lock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ktime_get_boottime_ts64(&d->record.ts);
	ktime_get_real_ts64(&d->record.utc);
#else
	get_monotonic_boottime64(&d->record.ts);
	getnstimeofday(&d->record.utc);
#endif
	d->record.syscnt = bsp_get_syscount();
	spin_unlock_irqrestore(&d->sync_lock, flags);

#if RECORD_NEED_SYNC_PERIOD
	schedule_delayed_work(&d->sync_record_work, round_jiffies_relative(msecs_to_jiffies(d->sync_interval)));
#endif

	pr_info("%s --", __func__);
}

/* sysfs resume/suspend bits for timekeeping */
static struct syscore_ops bsp_syscounter_syscore_ops = {
	.resume     = bsp_syscounter_resume,
	.suspend    = bsp_syscounter_suspend,
};
/*lint -e446 -e86*/
static int bsp_syscounter_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct syscnt_device *d = NULL;
	int ret;
	unsigned long flags;

	d = kzalloc(sizeof(struct syscnt_device), GFP_KERNEL);
	if (d == NULL) {
		dev_err(dev, "kzalloc mem error\n");
		return -ENOMEM;
	}
	syscnt_dev = d;

	ret = of_property_read_u64(dev->of_node, "clock-rate", &d->clock_rate);
	if (ret != 0) {
		dev_err(dev, "read clock-rate from dts error\n");
		goto err_probe;
	}

#if RECORD_NEED_SYNC_PERIOD
	ret = of_property_read_u32(dev->of_node, "sync-interval", &d->sync_interval);
	if (ret != 0) {
		dev_err(dev, "read sync-interval from dts error\n");
		goto err_probe;
	}
#endif

	d->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (d->res == NULL) {
		dev_err(dev, "platform_get_resource error\n");
		ret = -ENOMEM;
		goto err_probe;
	}

	d->base = ioremap(d->res->start, resource_size(d->res));
	if (d->base == NULL) {
		dev_err(dev, "ioremap baseaddr error\n");
		ret = -ENOMEM;
		goto err_probe;
	}

	spin_lock_init(&d->r_lock);
	spin_lock_init(&d->sync_lock);

	spin_lock_irqsave(&d->sync_lock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ktime_get_boottime_ts64(&d->record.ts);
	ktime_get_real_ts64(&d->record.utc);
#else
	get_monotonic_boottime64(&d->record.ts);
	getnstimeofday(&d->record.utc);
#endif
	d->record.syscnt = bsp_get_syscount();
	spin_unlock_irqrestore(&d->sync_lock, flags);

	platform_set_drvdata(pdev, d);
	register_syscore_ops(&bsp_syscounter_syscore_ops);

#if RECORD_NEED_SYNC_PERIOD
	INIT_DELAYED_WORK(&d->sync_record_work, bsp_syscounter_sync_work);
	schedule_delayed_work(&d->sync_record_work, round_jiffies_relative(msecs_to_jiffies(d->sync_interval)));
#endif

#ifdef CONFIG_DFX_DEBUG_FS
	ret = sysfs_create_files(&dev->kobj, syscnt_attributes);
	if (ret != 0)
		dev_err(dev, "create sysfs error\n");
#endif

	dev_info(dev, "%s success\n", __func__);

	return 0;

err_probe:
	kfree(d);
	syscnt_dev = NULL;
	return ret;
}
/*lint +e446*/
static int bsp_syscounter_remove(struct platform_device *pdev)
{
	struct syscnt_device *d = syscnt_dev;

	unregister_syscore_ops(&bsp_syscounter_syscore_ops);
	if (d != NULL) {
		iounmap(d->base);
		kfree(d);
		syscnt_dev = NULL;
	}
	return 0;
}

static const struct of_device_id bsp_syscounter_of_match[] = {
	{ .compatible = "hisilicon,syscounter-driver" },
	{ }
};

static struct platform_driver bsp_syscounter_driver = {
	.probe  = bsp_syscounter_probe,
	.remove = bsp_syscounter_remove,
	.driver	= {
		.name =	"bsp-syscounter",
		.owner = THIS_MODULE,/*lint -e64*/
		.of_match_table = of_match_ptr(bsp_syscounter_of_match),
	},
};

static int __init bsp_syscounter_init(void)
{
	return platform_driver_register(&bsp_syscounter_driver);/*lint -e64*/
}

static void __exit bsp_syscounter_exit(void)
{
	platform_driver_unregister(&bsp_syscounter_driver);
}
/*lint +e86*/
module_init(bsp_syscounter_init);
module_exit(bsp_syscounter_exit);

MODULE_LICENSE("GPL");
