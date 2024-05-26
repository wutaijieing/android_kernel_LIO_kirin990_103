/*
 * gpu_top.c
 *
 * PM QOS for gpu top
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#ifdef CONFIG_PLATFORM_QOS
#include <linux/platform_drivers/platform_qos.h>
#else
#include <linux/pm_qos.h>
#endif
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <securec.h>


#include "gpu_top.h"
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#define TOPFREQ_BUF_MAX		20
#define IPC_MAIL_MAX		8
#define MHZ		1000000
#define DEFAULT_RATIO	2

#define GPUTOP_UNLINK_VALUE	PM_QOS_GPUTOP_FREQ_UPLIMIT_DEFAULT_VALUE

struct gpudev_data {
	struct device *dev_link;
	unsigned long freq_link;
	unsigned long cur_freq;
	unsigned long min_freq;
	unsigned long max_freq;
	unsigned int ratio;
	bool sysfs_init;
	bool link;
};

struct gt_qos_nb {
	struct notifier_block nb;
	struct device *dev;
};

#ifdef CONFIG_PLATFORM_QOS
static const int g_qos_dn = PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT;
static const int g_qos_up = PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT;
static struct platform_qos_request g_qos_req_perf;
static struct platform_qos_request g_qos_req_thm;
#else
static const int g_qos_dn = PM_QOS_GPUTOP_FREQ_DNLIMIT;
static const int g_qos_up = PM_QOS_GPUTOP_FREQ_UPLIMIT;

static struct pm_qos_request g_qos_req_perf;
static struct pm_qos_request g_qos_req_thm;
#endif

static struct device *g_gtdev;

DEFINE_MUTEX(gtop_mutex);

static int freq_scale_request(struct device *dev, unsigned long freq)
{
	int ret;

	rproc_id_t rproc_id = IPC_ACPU_LPM3_MBX_1;

	/* msg0 is message cmd, msg1 is gputop frequency */
	unsigned int msg[IPC_MAIL_MAX] = { 0x0003031C, 0x0 };

	msg[1] = freq / MHZ;

	ret = RPROC_ASYNC_SEND(rproc_id, (mbox_msg_t *)msg, IPC_MAIL_MAX);
	if (ret != 0)
		dev_err(dev, "%s , line %d, send error\n", __func__, __LINE__);

	return ret;
}

static void gputop_freq_range_init(struct device *dev, struct gpudev_data *data)
{
	unsigned long freq = 0;
	unsigned long min = ~0;
	unsigned long max = 0;
	struct dev_pm_opp *opp = NULL;

	do {
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
			break;
		dev_pm_opp_put(opp);
		if (freq > max)
			max = freq;
		if (freq < min)
			min = freq;
		freq++;
	} while (1);

	data->min_freq = min;
	data->max_freq = max;
}

void update_gputop_linked_freq(unsigned long freq)
{
	unsigned long target_freq;
	struct dev_pm_opp *opp = NULL;
	struct device *dev = NULL;
	struct gpudev_data *data = NULL;

	mutex_lock(&gtop_mutex);

	dev = g_gtdev;
	if (IS_ERR_OR_NULL(dev))
		goto ret_handle;

	data = dev_get_drvdata(dev);

	data->freq_link = freq;
	if (data->link) {
		target_freq = data->freq_link * data->ratio;
		opp = devfreq_recommended_opp(dev, &target_freq, 0);
		if (IS_ERR(opp)) {
			dev_err(dev, "%s:not find freq%lu\n",
				__func__, target_freq);
			goto ret_handle;
		}
		dev_pm_opp_put(opp);
		data->cur_freq = target_freq;
	}

ret_handle:
	mutex_unlock(&gtop_mutex);
}

static int gtop_qos_notifier_call(struct notifier_block *nb,
				  unsigned long type,
				  void *devp)
{
	int qos_dn;
	int qos_up;
	unsigned long freq;
	unsigned long target_freq;
	int ret = 0;
	struct dev_pm_opp *opp = NULL;
	struct gt_qos_nb *qos_nb = container_of(nb, struct gt_qos_nb, nb);
	struct device *dev = NULL;
	struct gpudev_data *data = NULL;

#ifdef CONFIG_PLATFORM_QOS
	qos_dn = platform_qos_request(g_qos_dn);
	qos_up = platform_qos_request(g_qos_up);
#else
	qos_dn = pm_qos_request(g_qos_dn);
	qos_up = pm_qos_request(g_qos_up);
#endif
	if (qos_dn < 0 || qos_up < 0)
		return -EINVAL;

	mutex_lock(&gtop_mutex);

	dev = qos_nb->dev;
	if (IS_ERR_OR_NULL(dev)) {
		ret = -ENODEV;
		goto ret_handle;
	}
	data = dev_get_drvdata(dev);

	if (((unsigned long)qos_up * MHZ) <= data->max_freq || qos_dn == 0) {
		/* enable link */
		freq = 0;
		data->link = true;
		target_freq = data->freq_link * data->ratio;
	} else {
		/* cancel link & set freq */
		freq = (unsigned long)qos_dn * MHZ;
		target_freq = freq;
		data->link = false;
	}

	opp = devfreq_recommended_opp(dev, &target_freq, 0);
	if (IS_ERR(opp)) {
		dev_err(dev, "%s:not find freq%lu\n", __func__, target_freq);
		ret = -EINVAL;
		goto ret_handle;
	}
	dev_pm_opp_put(opp);
	data->cur_freq = target_freq;

ret_handle:
	mutex_unlock(&gtop_mutex);

	if (ret == 0)
		ret = freq_scale_request(dev, freq);

	return ret;
}

static struct gt_qos_nb g_gtop_qos_notifier = {
	.nb = {.notifier_call = gtop_qos_notifier_call},
};

static void gputop_pm_qos_add(struct device *dev)
{
	int ret;

	g_gtop_qos_notifier.dev = dev;

#ifdef CONFIG_PLATFORM_QOS
	ret = platform_qos_add_notifier(g_qos_dn, &(g_gtop_qos_notifier.nb));
#else
	ret = pm_qos_add_notifier(g_qos_dn, &(g_gtop_qos_notifier.nb));
#endif
	if (ret)
		dev_err(dev, "fail to register down limit notifier\n");

#ifdef CONFIG_PLATFORM_QOS
	ret = platform_qos_add_notifier(g_qos_up, &(g_gtop_qos_notifier.nb));
#else
	ret = pm_qos_add_notifier(g_qos_up, &(g_gtop_qos_notifier.nb));
#endif
	if (ret)
		dev_err(dev, "fail to register up limit notifier\n");

#ifdef CONFIG_PLATFORM_QOS
	platform_qos_add_request(&g_qos_req_perf, g_qos_dn, 0);
#else
	pm_qos_add_request(&g_qos_req_perf, g_qos_dn, 0);
#endif
}

static void gputop_pm_qos_remove(struct device *dev)
{
	int ret;

	g_gtop_qos_notifier.dev = NULL;

#ifdef CONFIG_PLATFORM_QOS
	ret = platform_qos_remove_notifier(g_qos_dn, &(g_gtop_qos_notifier.nb));
#else
	ret = pm_qos_remove_notifier(g_qos_dn, &(g_gtop_qos_notifier.nb));
#endif
	if (ret)
		dev_err(dev, "fail to delete down notifier\n");

#ifdef CONFIG_PLATFORM_QOS
	ret = platform_qos_remove_notifier(g_qos_up, &(g_gtop_qos_notifier.nb));
#else
	ret = pm_qos_remove_notifier(g_qos_up, &(g_gtop_qos_notifier.nb));
#endif
	if (ret)
		dev_err(dev, "fail to delete up notifier\n");
}

void ipa_gputop_freq_limit(unsigned long freq)
{
#ifdef CONFIG_PLATFORM_QOS
	if (freq == 0 && platform_qos_request_active(&g_qos_req_thm)) {
		platform_qos_remove_request(&g_qos_req_thm);
		return;
	}
#else
	if (freq == 0 && pm_qos_request_active(&g_qos_req_thm)) {
		pm_qos_remove_request(&g_qos_req_thm);
		return;
	}
#endif

#ifdef CONFIG_PLATFORM_QOS
	if (!platform_qos_request_active(&g_qos_req_thm))
		platform_qos_add_request(&g_qos_req_thm, g_qos_up, freq);
	else
		platform_qos_update_request(&g_qos_req_thm, freq);
#else
	if (!pm_qos_request_active(&g_qos_req_thm))
		pm_qos_add_request(&g_qos_req_thm, g_qos_up, freq);
	else
		pm_qos_update_request(&g_qos_req_thm, freq);
#endif
}

static ssize_t show_cur_freq(struct device *dev,
			     struct device_attribute *attr,
			     char * const buf)
{
	unsigned int freq_mhz;
	ssize_t count = 0;
	struct gpudev_data *data = dev_get_drvdata(dev);

	freq_mhz = data->cur_freq / MHZ;

	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "%u\n", freq_mhz);

	return count;
}
static DEVICE_ATTR(cur_freq, 0440, show_cur_freq, NULL);

static ssize_t set_perf_freq(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	int freq;
	char *const wbuf = kstrdup(buf, GFP_KERNEL);

	if (!wbuf)
		return -ENOMEM;

	ret = sscanf_s(wbuf, "%d", &freq);
	if (ret != 1) {
		kfree(wbuf);
		dev_err(dev, "%s: para wrong\n", __func__);
		return -ENOMEM;
	}

	kfree(wbuf);

#ifdef CONFIG_PLATFORM_QOS
	platform_qos_update_request(&g_qos_req_perf, freq);
#else
	pm_qos_update_request(&g_qos_req_perf, freq);
#endif

	return count;
}
static DEVICE_ATTR(perf_freq, 0200, NULL, set_perf_freq);

static ssize_t show_freq_available(struct device *dev,
				   struct device_attribute *attr,
				   char * const buf)
{
	struct dev_pm_opp *opp = NULL;
	ssize_t count = 0;
	unsigned long freq = 0;

	do {
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
			break;
		dev_pm_opp_put(opp);

		count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
				   "%lu ", freq);
		freq++;
	} while (1);

	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2), "\n");

	return count;
}
static DEVICE_ATTR(available_frequencies, 0440, show_freq_available, NULL);

#ifdef CONFIG_GPUTOP_FREQ_DEBUG
static ssize_t show_gputop_info(struct device *dev,
				struct device_attribute *attr,
				char * const buf)
{
	ssize_t count = 0;
	struct gpudev_data *data = dev_get_drvdata(dev);

	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "freq_link:%lu\n", data->freq_link);
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "cur_freq:%lu\n", data->cur_freq);
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "ratio:%u\n", data->ratio);
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "link:%s\n", data->link ? "true" : "false");
#ifdef CONFIG_PLATFORM_QOS
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "perf qos:%d\n", platform_qos_request(g_qos_dn));
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "limit qos:%d\n", platform_qos_request(g_qos_up));
#else
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "perf qos:%d\n", pm_qos_request(g_qos_dn));
	count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
			   "limit qos:%d\n", pm_qos_request(g_qos_up));
#endif

	return count;
}
static DEVICE_ATTR(info, 0440, show_gputop_info, NULL);

static ssize_t set_limit_freq(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	int freq;
	char *const wbuf = kstrdup(buf, GFP_KERNEL);

	if (!wbuf)
		return -ENOMEM;

	ret = sscanf_s(wbuf, "%d", &freq);
	if (ret != 1) {
		kfree(wbuf);
		dev_err(dev, "%s: para wrong\n", __func__);
		return -ENOMEM;
	}

	kfree(wbuf);

	ipa_gputop_freq_limit(freq);

	return count;
}
static DEVICE_ATTR(limit_freq, 0200, NULL, set_limit_freq);
#endif

static struct attribute *gtop_attrs[] = {
	&dev_attr_cur_freq.attr,
	&dev_attr_perf_freq.attr,
	&dev_attr_available_frequencies.attr,
#ifdef CONFIG_GPUTOP_FREQ_DEBUG
	&dev_attr_info.attr,
	&dev_attr_limit_freq.attr,
#endif
	NULL
};

static const struct attribute_group gtop_attr_group = {
	.attrs = gtop_attrs,
};

static int gputop_probe(struct platform_device *pdev)
{
	int err;
	struct gpudev_data *data = NULL;
	struct device *dev = &pdev->dev;

	err = dev_pm_opp_of_add_table(dev);
	if (err) {
		dev_err(dev, "add opp table fail%d\n", err);
		return err;
	}

	data = devm_kzalloc(dev, sizeof(struct gpudev_data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "alloc private data fail\n");
		return -ENOMEM;
	}

	data->ratio = DEFAULT_RATIO;
	data->sysfs_init = false;
	data->link = true;

	mutex_lock(&gtop_mutex);

	dev_set_drvdata(dev, data);
	g_gtdev = dev;

	gputop_freq_range_init(dev, data);

	mutex_unlock(&gtop_mutex);

	gputop_pm_qos_add(dev);

	err = sysfs_create_group(&dev->kobj, &gtop_attr_group);
	if (err) {
		dev_err(dev, "SysFS group creation failed\n");
		return err;
	}
	data->sysfs_init = true;

	return 0;
}

static int gputop_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gpudev_data *data = dev_get_drvdata(dev);

	mutex_lock(&gtop_mutex);

	g_gtdev = NULL;
	gputop_pm_qos_remove(dev);

	if (data->sysfs_init) {
		data->sysfs_init = false;
		sysfs_remove_group(&dev->kobj, &gtop_attr_group);
	}

	mutex_unlock(&gtop_mutex);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id gputop_of_match[] = {
	{.compatible = "hisilicon,gputop",},
	{},
};

MODULE_DEVICE_TABLE(of, gputop_of_match);
#endif

static struct platform_driver gputop_driver = {
	.probe  = gputop_probe,
	.remove = gputop_remove,
	.driver = {
			.name = "hisi_gputop",
			.owner = THIS_MODULE,
			.of_match_table = of_match_ptr(gputop_of_match),
		},
};

static int __init gputop_init(void)
{
	return platform_driver_register(&gputop_driver);
}
device_initcall(gputop_init);

static void __exit gputop_exit(void)
{
	platform_driver_unregister(&gputop_driver);
}
module_exit(gputop_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpu top pm qos");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
