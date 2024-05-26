#include <linux/version.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>
#include <linux/tick.h>
#include <securec.h>
#ifdef CONFIG_HW_VOTE
#include <linux/platform_drivers/hw_vote.h>
#endif
#include "governor.h"


struct proxy_load {
	unsigned long long idle_time;
	unsigned long long total_time;
	unsigned long load;
};

struct proxy_df_device {
	struct devfreq *df;
	struct hvdev *hv;
	struct opp_table *table;
	struct proxy_load *load;
	void *governor_data;
	unsigned long vm_min;
	unsigned long vm_max;
	cpumask_t cpu_mask;
	char governor[DEVFREQ_NAME_LEN];
};

#define KHZ	1000

static int proxy_devfreq_target(struct device *dev,
				unsigned long *freq, unsigned int flags)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);
	struct dev_pm_opp *opp = NULL;

	if (*freq < cdev->vm_min)
		*freq = cdev->vm_min;
	if (cdev->vm_max > 0 && *freq > cdev->vm_max)
		*freq = cdev->vm_max;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		dev_err(dev, "Failed to get opp\n");
		return PTR_ERR(opp);
	}

	dev_pm_opp_put(opp);
#ifdef CONFIG_HW_VOTE
	return hv_set_freq(cdev->hv, *freq / KHZ);
#else
	return 0;
#endif
}

static unsigned long get_cpu_load(int cpu, struct proxy_load *load_info)
{
	unsigned long long total = load_info->total_time;
	unsigned long long idle = load_info->idle_time;
	unsigned long long busy;
	load_info->idle_time = get_cpu_idle_time_us(cpu, &load_info->total_time);

	total = load_info->total_time - total;
	busy = total - load_info->idle_time + idle;
	if (total == 0)
		return load_info->load;
	load_info->load = (unsigned long) div_u64(busy * 100, total);
	return load_info->load;
}

static int proxy_devfreq_get_status(struct device *dev,
				  struct devfreq_dev_status *stat)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);
	int cpu;
	unsigned int i = 0;
	unsigned long cpu_load = 0;
	stat->total_time = 100UL;
	stat->busy_time = 0;
	stat->current_frequency = cdev->df->previous_freq;

	for_each_cpu(cpu, &cdev->cpu_mask) {
		cpu_load = max(get_cpu_load(cpu, &cdev->load[i]), cpu_load);
		stat->busy_time = cpu_load;
		i++;
	}
	return 0;
}

static ssize_t store_freq(struct device *dev, size_t offset,
			  const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev->parent);
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);
	struct devfreq *df = cdev->df;
	unsigned long freq, *target = (typeof(target))((char *)cdev + offset);
	int ret;

	if (sscanf_s(buf, "%lu", &freq) != 1)
		return -EINVAL;

	mutex_lock(&df->lock);
	*target = freq;
	ret = update_devfreq(df);
	mutex_unlock(&df->lock);

	if (ret == 0)
		ret = (int)count;

	return ret;
}

static ssize_t store_vm_min(struct device *dev,
			    struct device_attribute *attr __maybe_unused,
			    const char *buf, size_t count)
{
	return store_freq(dev, offsetof(struct proxy_df_device, vm_min),
			  buf, count);
}

static ssize_t store_vm_max(struct device *dev,
			    struct device_attribute *attr __maybe_unused,
			    const char *buf, size_t count)
{
	return store_freq(dev, offsetof(struct proxy_df_device, vm_max),
			  buf, count);
}

static ssize_t show_freq(struct device *dev, size_t offset, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev->parent);
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);
	struct devfreq *df = cdev->df;
	unsigned long *freq = (typeof(freq))((char *)cdev + offset);
	int ret;

	mutex_lock(&df->lock);
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%lu\n", *freq);
	mutex_unlock(&df->lock);

	return ret;
}

static ssize_t show_vm_min(struct device *dev,
			   struct device_attribute *attr __maybe_unused,
			   char *buf)
{
	return show_freq(dev, offsetof(struct proxy_df_device, vm_min), buf);
}

static ssize_t show_vm_max(struct device *dev,
			   struct device_attribute *attr __maybe_unused,
			   char *buf)
{
	return show_freq(dev, offsetof(struct proxy_df_device, vm_max), buf);
}

static ssize_t show_cpu_mask(struct device *dev,
			     struct device_attribute *attr __maybe_unused,
			     char *buf)
{
	struct platform_device *pdev = to_platform_device(dev->parent);
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);
	int err = 0;

	err = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1,
			 "%lx\n", cpumask_bits(&cdev->cpu_mask)[0]);

	return err;
}

static DEVICE_ATTR(vm_min, 0640, show_vm_min, store_vm_min);
static DEVICE_ATTR(vm_max, 0640, show_vm_max, store_vm_max);
static DEVICE_ATTR(cpu_mask, 0640, show_cpu_mask, NULL);
static const struct attribute *proxy_df_attrs[] = {
	&dev_attr_vm_min.attr,
	&dev_attr_vm_max.attr,
	&dev_attr_cpu_mask.attr,
	NULL,
};

enum {
	HV_CH_NAME,
	HV_VSRC,
	HV_ARRAY_SIZE,
};

static int proxy_of_parse(struct device *dev, struct proxy_df_device *cdev,
			  struct devfreq_dev_profile *df_profile)
{
	struct device_node *np = dev->of_node;
	const char *gov = NULL;
	unsigned int value = 0;
	int ret;

	/* parse cpu mask */
	ret = of_property_read_u32(np, "cpu-mask", &value);
	if (ret != 0) {
		dev_err(dev, "Failed to get cpu mask\n");
		return ret;
	}

	cpumask_bits(&cdev->cpu_mask)[0] |= value;
	value = cpumask_weight(&cdev->cpu_mask);
	if (value != 0) {
		cdev->load = devm_kzalloc(dev, value * sizeof(struct proxy_load),
					 GFP_KERNEL);
		if (IS_ERR_OR_NULL(cdev->load)) {
			dev_err(dev, "Failed to alloc load\n");
			return -ENOMEM;
		}
	}

	if (of_property_read_u32(np, "polling-ms", &value) != 0) {
		dev_err(dev, "Failed to get polling ms\n");
	} else {
		df_profile->polling_ms = value;
	}

	/* parse governor */
	ret = of_property_read_string(np, "governor", &gov);
	if (ret != 0) {
		dev_err(dev, "Failed ot parse governor\n");
	} else {
		ret = strncpy_s(cdev->governor, sizeof(cdev->governor),
				gov, sizeof(cdev->governor) - 1);
	}

	/* parse governor data here if needed */
	return ret;
}

#ifdef CONFIG_HW_VOTE
static struct hvdev *proxy_hvdev_register(struct device *dev)
{
	struct device_node *np = dev->of_node;
	const char *vote_ch[HV_ARRAY_SIZE] = {0};
	int ret;

	ret = of_property_read_string_array(np, "freq-vote-channel",
					    vote_ch, HV_ARRAY_SIZE);
	if (ret != HV_ARRAY_SIZE) {
		dev_err(dev, "Failed to parse freq-vote-channel for proxy!\n");
		return NULL;
	}

	return hvdev_register(dev, vote_ch[HV_CH_NAME], vote_ch[HV_VSRC]);
}

static void proxy_hvdev_unregister(struct device *dev, struct hvdev *proxy_hv)
{
	int ret = hvdev_remove(proxy_hv);
	if (ret != 0)
		dev_err(dev, "Failed to unregister hvdev for proxy\n");
}
#endif

static int proxy_devfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct proxy_df_device *cdev = NULL;
	struct devfreq_dev_profile *df_profile = NULL;
	unsigned int version = 1;
	int ret;

	cdev = devm_kzalloc(dev, sizeof(struct proxy_df_device), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cdev))
		return -ENOMEM;

	df_profile = devm_kzalloc(dev, sizeof(struct devfreq_dev_profile), GFP_KERNEL);
	if (IS_ERR_OR_NULL(df_profile))
		return -ENOMEM;

	ret = proxy_of_parse(dev, cdev, df_profile);
	if (ret != 0)
		return ret;

#ifdef CONFIG_HW_VOTE
	unsigned int freq_khz;
	cdev->hv = proxy_hvdev_register(dev);
	if (IS_ERR_OR_NULL(cdev->hv)) {
		dev_err(dev, "Failed to register hvdev for proxy\n");
		return -ENODEV;
	}
	ret = hv_get_result(cdev->hv, &freq_khz);
	if (ret == 0)
		df_profile->initial_freq = (unsigned long)freq_khz * KHZ;
#endif

	platform_set_drvdata(pdev, cdev);

	/* NOTICE: current version is fixed to 1, it is also possible
	 * to get it from dts if needed
	 */
	cdev->table = dev_pm_opp_set_supported_hw(dev, &version, 1);
	if (IS_ERR_OR_NULL(cdev->table)) {
		dev_err(dev, "Failed to set supported hw\n");
		ret = -ENODEV;
		goto unregister_hv;
	}

	if (dev_pm_opp_of_add_table(dev) != 0) {
		dev_err(dev, "Failed to add freq table\n");
		ret = -ENODEV;
		goto put_supportde_hw;
	}

	df_profile->target = proxy_devfreq_target;
	df_profile->get_dev_status = proxy_devfreq_get_status;
	cdev->df = devm_devfreq_add_device(dev, df_profile,
					   cdev->governor,
					   cdev->governor_data);
	if (IS_ERR_OR_NULL(cdev->df)) {
		dev_err(dev, "Failed to initialize proxy devfreq\n");
		ret = -ENODEV;
		goto remove_table;
	}

	ret = sysfs_create_files(&cdev->df->dev.kobj, proxy_df_attrs);
	if (ret != 0)
		dev_err(dev, "Failed to add proxy devfreq sysfs\n");

	return 0;

remove_table:
	dev_pm_opp_of_remove_table(dev);
put_supportde_hw:
	dev_pm_opp_put_supported_hw(cdev->table);
unregister_hv:
#ifdef CONFIG_HW_VOTE
	proxy_hvdev_unregister(dev, cdev->hv);
#endif

	return ret;
}

static int proxy_devfreq_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct proxy_df_device *cdev = platform_get_drvdata(pdev);

	sysfs_remove_files(&cdev->df->dev.kobj, proxy_df_attrs);
	devm_devfreq_remove_device(dev, cdev->df);
	dev_pm_opp_of_remove_table(dev);
	dev_pm_opp_put_supported_hw(cdev->table);
#ifdef CONFIG_HW_VOTE
	proxy_hvdev_unregister(dev, cdev->hv);
#endif

	return 0;
}

static const struct of_device_id proxy_devfreq_of_match[] = {
	{
		.compatible = "proxy-devfreq",
	},
	{},
};

MODULE_DEVICE_TABLE(of, proxy_devfreq_of_match);

static struct platform_driver proxy_devfreq_driver = {
	.probe = proxy_devfreq_probe,
	.remove = proxy_devfreq_remove,
	.driver = {
		.name = "proxy_devfreq",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(proxy_devfreq_of_match),
	},
};

module_platform_driver(proxy_devfreq_driver);

