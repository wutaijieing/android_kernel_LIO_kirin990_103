/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/platform_drivers/gpufreq_v2.h>
#include <linux/pm_opp.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>
#include <linux/clk.h>
#include <linux/platform_drivers/hw_vote.h>
#include <platform_include/maligpu/linux/gpu_ipa_thermal.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/drg.h>
#include <linux/version.h>
#include <trace/events/power.h>
#include "gpu_fhss.h"
#include "gpu_top.h"
#include "gpufreq_fs.h"
#include "link/gpuflink.h"
#include "securec.h"

#define GPU_DEFAULT_GOVERNOR	"gpu_scene_aware"
#define DEFAULT_POLLING_MS	20
#define KHZ	1000

enum gpufreq_cb_index {
	GF_CB_IDX_BUSYTIME,
	GF_CB_IDX_VSYNC_HIT,
	GF_CB_IDX_CL_BOOST,
	GF_CB_IDX_USER_TARGET,
	GF_CB_IDX_COREMASK,
	GF_CB_IDX_MAX
};

struct gpufreq_device {
	struct device *dev;
	struct devfreq *df;
	struct clk *clk;
#ifdef CONFIG_GPUFREQ_HW_VOTE
	struct hvdev *hv;
#endif
};
struct gpufreq_user_callback {
	unsigned int type;
	int (*cb)(void);
	unsigned int flag;
};

static struct gpufreq_device *g_gfdev = NULL;
static DEFINE_MUTEX(gpufreq_mutex);

static struct gpufreq_user_callback g_gf_user_cb[] = {
	[GF_CB_IDX_BUSYTIME] = {
		.type = GF_CB_BUSYTIME,
	},
	[GF_CB_IDX_VSYNC_HIT] = {
		.type = GF_CB_VSYNC_HIT,
	},
	[GF_CB_IDX_CL_BOOST] = {
		.type = GF_CB_CL_BOOST,
	},
	[GF_CB_IDX_USER_TARGET] = {
		.type = GF_CB_USER_TARGET,
	},
	[GF_CB_IDX_COREMASK] = {
		.type = GF_CB_CORE_MASK,
	},
};

static struct devfreq_data g_gpufreq_priv_data = {
	.vsync_hit = 0,
	.cl_boost = 0,
};

struct gpufreq_target_limit {
	unsigned long (*target)(unsigned long);
	struct list_head node;
};

int gpufreq_register_callback(unsigned int type, int (*cb)(void))
{
	unsigned int i;

	if (cb == NULL)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(g_gf_user_cb); i++) {
		if (g_gf_user_cb[i].type != type)
			continue;

		if (g_gf_user_cb[i].cb != NULL) {
			pr_err("%s, repeat register", __func__);
			return -EEXIST;
		}

		g_gf_user_cb[i].cb = cb;
		return 0;
	}

	return -EPERM;
}
EXPORT_SYMBOL(gpufreq_register_callback);

static int gpufreq_get_para(unsigned int index, int up_limit, int def)
{
	int para;
	int (*get_para)(void) = g_gf_user_cb[index].cb;

	if (get_para != NULL)
		para = get_para();
	else
		para = def;

	return min(para, up_limit);
}

static int gpufreq_get_busytime(void)
{
	return gpufreq_get_para(GF_CB_IDX_BUSYTIME, 100, 0);
}

static int gpufreq_get_vsync_hit(void)
{
	return gpufreq_get_para(GF_CB_IDX_VSYNC_HIT, 1, 0);
}

static int gpufreq_get_cl_boost(void)
{
	return gpufreq_get_para(GF_CB_IDX_CL_BOOST, 1, 0);
}

static int gpufreq_coremask_enqueue(void)
{
	if (!g_gf_user_cb[GF_CB_IDX_COREMASK].cb)
		return 0;

	return g_gf_user_cb[GF_CB_IDX_COREMASK].cb();
}

#ifdef CONFIG_GPUFREQ_VOTE
int gpufreq_register_min_req(struct dev_pm_qos_request *req, int value)
{
	int ret;

	if (req == NULL)
		return -EINVAL;

	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		mutex_unlock(&gpufreq_mutex);
		return -EPERM;
	}

	ret =  dev_pm_qos_add_request(g_gfdev->dev, req, DEV_PM_QOS_MIN_FREQUENCY, value);
	mutex_unlock(&gpufreq_mutex);

	return ret;
}
EXPORT_SYMBOL(gpufreq_register_min_req);

int gpufreq_register_max_req(struct dev_pm_qos_request *req, int value)
{
	int ret;

	if (req == NULL)
		return -EINVAL;

	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		mutex_unlock(&gpufreq_mutex);
		return -EPERM;
	}
	ret =  dev_pm_qos_add_request(g_gfdev->dev, req, DEV_PM_QOS_MAX_FREQUENCY, value);
	mutex_unlock(&gpufreq_mutex);

	return ret;
}
EXPORT_SYMBOL(gpufreq_register_max_req);

int gpufreq_unregister_request(struct dev_pm_qos_request *req)
{
	if (req == NULL)
		return -EINVAL;

	return dev_pm_qos_remove_request(req);
}
EXPORT_SYMBOL(gpufreq_unregister_request);

int gpufreq_vote_request(struct dev_pm_qos_request *req, int value)
{
	if (req == NULL)
		return -EINVAL;

	return dev_pm_qos_update_request(req, value);
}
EXPORT_SYMBOL(gpufreq_vote_request);
#endif

static int gpufreq_driver_register(struct gpufreq_device *gfdev)
{
	struct device *dev = gfdev->dev;
#ifdef CONFIG_GPUFREQ_HW_VOTE
	gfdev->hv = hvdev_register(dev, "gpu-freq", "vote-src-1");
	if (!gfdev->hv) {
		dev_err(dev, "register hvdev fail!\n");
		return -ENODEV;
	}
#endif
	gfdev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(gfdev->clk)) {
		dev_err(dev, "Failed to get clk\n");
		return -ENOENT;
	}

	return 0;
}

static int gpufreq_driver_deregister(struct gpufreq_device *gfdev)
{
	int ret = 0;

#ifdef CONFIG_GPUFREQ_HW_VOTE
	if (gfdev->hv != NULL)
		ret = hvdev_remove(g_gfdev->hv);
#endif
	if (gfdev->clk != NULL)
		devm_clk_put(gfdev->dev, gfdev->clk);

	return ret;
}

static unsigned long get_gpu_freq(struct gpufreq_device *gfdev __maybe_unused)
{
	unsigned long freq_hz = 0;
	unsigned int freq_khz = 0;
#ifdef CONFIG_GPUFREQ_HW_VOTE
	if (g_gfdev->hv)
		hv_get_last(g_gfdev->hv, &freq_khz);
	freq_hz = (unsigned long)freq_khz * KHZ;
#else
	if (g_gfdev->clk)
		freq_hz = clk_get_rate(g_gfdev->clk);
#endif
	return freq_hz;
}

static int set_gpu_freq(struct gpufreq_device *gfdev, unsigned long freq)
{
#ifdef CONFIG_GPUFREQ_HW_VOTE
	if (hv_set_freq(gfdev->hv, (unsigned int)freq / KHZ))
#else
	if (clk_set_rate(gfdev->clk, freq))
#endif
		return -ENODEV;

	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#ifdef CONFIG_GPU_IPA_THERMAL
#ifdef CONFIG_GPUTOP_FREQ
static bool g_ipa_freq_limit;
#endif

static unsigned long get_gpu_ipa_freqlimit(unsigned long freq)
{
	unsigned long freq_limit;
	int ret;

	ret = ithermal_get_ipa_gpufreq_limit(freq, &freq_limit);
	if (ret < 0)
		return -ENODEV;
#ifdef CONFIG_GPUTOP_FREQ
	if (freq_limit < freq && !g_ipa_freq_limit) {
		/* set link */
		ipa_gputop_freq_limit(1);
		g_ipa_freq_limit = true;
	}
	if (freq_limit >= freq && g_ipa_freq_limit) {
		/* cancel link */
		ipa_gputop_freq_limit(0);
		g_ipa_freq_limit = false;
	}
#endif
	return freq_limit;
}
#endif
#endif

static int gpufreq_set_frequency(struct device *dev,
				 unsigned long *_freq,
				 unsigned int flags)
{
	unsigned long freq;
	unsigned long old_freq;
	struct dev_pm_opp *opp = NULL;
	struct gpufreq_device *gfdev = (struct gpufreq_device *)dev->platform_data;
	struct devfreq *df = NULL;
	int ret;

	df = gfdev->df;
	old_freq = df->previous_freq;

	opp = devfreq_recommended_opp(dev, _freq, flags);
	if (IS_ERR_OR_NULL(opp)) {
		dev_err(dev, "Failed to get opp\n");
		return -ENOENT;
	}

	freq = dev_pm_opp_get_freq(opp);
	dev_pm_opp_put(opp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	freq = get_gpu_ipa_freqlimit(freq);
#endif
	if (old_freq == freq)
		goto update_target;

	gpuflink_notifiy(freq);

	ret = set_gpu_freq(gfdev, freq);
	if (ret != 0)
		dev_err(dev, "Failed to set gpu frequency, [%lu->%lu]\n", old_freq, freq);
update_target:
	*_freq = freq;
	gpufreq_coremask_enqueue();
	update_gputop_linked_freq(freq);
	return 0;
}

unsigned long gpufreq_get_cur_freq(void)
{
	unsigned long freq = 0;

	if (g_gfdev == NULL)
		return 0;

	freq = get_gpu_freq(g_gfdev);

	return freq;
}
EXPORT_SYMBOL(gpufreq_get_cur_freq);

static int gpufreq_get_dev_status(struct device *dev,
				  struct devfreq_dev_status *stat)
{
	int busytime;
	struct gpufreq_device *gfdev = (struct gpufreq_device *)dev->platform_data;
	struct devfreq_data *priv_data = &g_gpufreq_priv_data;
#ifdef CONFIG_GPU_IPA_THERMAL
	int ret;
#endif

	busytime = gpufreq_get_busytime();
	if (busytime < 0)
		return -EINVAL;

	stat->busy_time = (unsigned long)busytime;
	stat->total_time = 100;

	stat->current_frequency = get_gpu_freq(gfdev);

	priv_data->vsync_hit = gpufreq_get_vsync_hit();
	priv_data->cl_boost = gpufreq_get_cl_boost();
	stat->private_data = (void *)priv_data;

#ifdef CONFIG_GPU_IPA_THERMAL
	ret = memcpy_s(&gfdev->df->last_status, sizeof(gfdev->df->last_status),
		       stat, sizeof(*stat));
	if (ret != 0) {
		dev_err(dev, "copy devfreq status fail\n");
		return ret;
	}
#endif

	return 0;
}

static struct devfreq_dev_profile g_gpufreq_profile = {
	.polling_ms	= DEFAULT_POLLING_MS,
	.target		= gpufreq_set_frequency,
	.get_dev_status	= gpufreq_get_dev_status,
};

static void gpu_freq_init(struct gpufreq_device *gfdev)
{
#ifdef CONFIG_GPUFREQ_HW_VOTE
	unsigned int freq = 0;
	unsigned long freq_hz;
	struct dev_pm_opp *opp = NULL;
	struct device *dev = gfdev->dev;
	int ret;

	if (!gfdev->hv) {
		g_gpufreq_profile.initial_freq = 0;
		return;
	}

	hv_get_last(gfdev->hv, &freq);
	freq_hz = (unsigned long)freq * KHZ;

	opp = dev_pm_opp_find_freq_ceil(dev, &freq_hz);
	if (IS_ERR(opp))
		freq_hz = 0;
	else
		dev_pm_opp_put(opp);

	/* update last freq in hv driver */
	ret = hv_set_freq(gfdev->hv, freq_hz / KHZ);
	if (ret != 0)
		dev_err(dev, "hv_set_freq fail\n");
	g_gpufreq_profile.initial_freq = freq_hz;
#else
	g_gpufreq_profile.initial_freq = clk_get_rate(gfdev->clk);
#endif
}

static int gpufreq_opp_init(struct device *gpufreq_dev)
{
	int ret;
	int opp_count;

	ret = dev_pm_opp_of_add_table(gpufreq_dev);
	if (ret != 0)
		return ret;

	opp_count = dev_pm_opp_get_opp_count(gpufreq_dev);
	if (opp_count <= 0)
		return -ENOENT;

	return ret;
}

static int gpufreq_devfreq_init(struct gpufreq_device *gfdev)
{
	struct devfreq *df = NULL;
	struct device *dev = gfdev->dev;

	gpu_freq_init(gfdev);

	dev_set_name(dev, "gpufreq");
	df = devfreq_add_device(dev,
				&g_gpufreq_profile,
				GPU_DEFAULT_GOVERNOR,
				NULL);
	if (IS_ERR_OR_NULL(df))
		return -ENODEV;

	gfdev->df = df;

	return 0;
}

static int gpufreq_create(struct gpufreq_device *gfdev)
{
	struct device *dev = gfdev->dev;
	int ret;

	ret = gpufreq_opp_init(dev);
	if (ret != 0) {
		dev_err(dev, "Failed to init opp\n");
		return ret;
	}

	ret = fhss_init(dev);
	if (ret != 0)
		dev_err(dev, "Failed to init fhss\n");

	ret = gpufreq_driver_register(gfdev);
	if (ret != 0) {
		dev_err(dev, "Failed to init hv_vote\n");
		return ret;
	}

	ret = gpufreq_devfreq_init(gfdev);
	if (ret != 0) {
		dev_err(dev, "Failed to add gpu devfreq\n");
		return ret;
	}

	ret = gpufreq_fs_register(gfdev->dev);
	if (ret != 0)
		return ret;

	ret = gpuflink_init(gfdev->df);
	if (ret != 0)
		dev_err(dev, "Failed to init gpuflink\n");

#ifdef CONFIG_GPU_IPA_THERMAL
	ithermal_gpu_cdev_register(gfdev->dev->of_node, gfdev->df);
#endif
	drg_devfreq_register(gfdev->df);

	return 0;
}

int gpufreq_start(void)
{
	int ret;

	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		pr_err("%s, gpufreq device not initial", __func__);
		mutex_unlock(&gpufreq_mutex);
		return -EPERM;
	}
	ret = gpufreq_create(g_gfdev);
	mutex_unlock(&gpufreq_mutex);
	return ret;
}
EXPORT_SYMBOL(gpufreq_start);

void gpufreq_devfreq_suspend(void)
{
	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		mutex_unlock(&gpufreq_mutex);
		return;
	}
	if (g_gfdev->df != NULL) {
		gpuflink_suspend(g_gfdev->df);
		devfreq_suspend_device(g_gfdev->df);
	}

	mutex_unlock(&gpufreq_mutex);
}
EXPORT_SYMBOL(gpufreq_devfreq_suspend);

void gpufreq_devfreq_resume(void)
{
	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		mutex_unlock(&gpufreq_mutex);
		return;
	}
	if (g_gfdev->df != NULL) {
		devfreq_resume_device(g_gfdev->df);
		gpuflink_resume(g_gfdev->df);
	}

	mutex_unlock(&gpufreq_mutex);
}
EXPORT_SYMBOL(gpufreq_devfreq_resume);

int gpufreq_term(void)
{
	int ret = 0;

	mutex_lock(&gpufreq_mutex);
	if (g_gfdev == NULL) {
		mutex_unlock(&gpufreq_mutex);
		return 0;
	}

	if (!IS_ERR_OR_NULL(g_gfdev->df)) {
		drg_devfreq_unregister(g_gfdev->df);
		ithermal_gpu_cdev_unregister();
		ret = devfreq_remove_device(g_gfdev->df);
	}

	gpufreq_fs_deregister();
	ret += gpufreq_driver_deregister(g_gfdev);

	mutex_unlock(&gpufreq_mutex);
	return ret;
}
EXPORT_SYMBOL(gpufreq_term);

static int gpufreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gpufreq_device *gfdev = NULL;

	gfdev = devm_kzalloc(dev, sizeof(struct gpufreq_device), GFP_KERNEL);
	if (!gfdev)
		return -ENOMEM;

	gfdev->dev = dev;
	dev->platform_data = gfdev;
	g_gfdev = gfdev;
	return 0;
}

static int gpufreq_resume(struct platform_device *pdev)
{
#ifdef CONFIG_GPUFREQ_HW_VOTE
	struct device *dev = &pdev->dev;
	struct gpufreq_device *gfdev = NULL;
	struct devfreq *df = NULL;
	int ret = 0;
	unsigned long freq;

	gfdev = (struct gpufreq_device *)dev->platform_data;
	df = gfdev->df;
	if (IS_ERR_OR_NULL(df)) {
		dev_dbg(dev,"%s devfreq not registered, hv-vote resume skip\n", __func__);
		return 0;
	}

	freq = df->previous_freq;

	/* update last freq in hv driver */
	if (gfdev->hv != NULL) {
		ret = hv_set_freq(gfdev->hv, freq / KHZ);
		if (ret != 0)
			dev_err(dev,"%s recover frequency failed%d\n", __func__, ret);
	}
	return ret;
#endif
}

static int gpufreq_remove(struct platform_device *pdev __maybe_unused)
{
	g_gfdev = NULL;
	return 0;
}

static const struct of_device_id gpufreq_of_match[] = {
	{.compatible = "gpu,gpufreq",},
	{},
};
MODULE_DEVICE_TABLE(of, gpufreq_of_match);

static struct platform_driver gpufreq_driver = {
	.probe  = gpufreq_probe,
	.remove = gpufreq_remove,
	.resume = gpufreq_resume,
	.driver = {
		.name = "gpu-devfreq",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(gpufreq_of_match),
	},
};

static int __init gpufreq_init(void)
{
	return platform_driver_register(&gpufreq_driver);
}
subsys_initcall(gpufreq_init);

static void __exit gpufreq_exit(void)
{
	platform_driver_unregister(&gpufreq_driver);
}
module_exit(gpufreq_exit);

MODULE_DESCRIPTION("gpu devfreq driver");
MODULE_LICENSE("GPL v2");
