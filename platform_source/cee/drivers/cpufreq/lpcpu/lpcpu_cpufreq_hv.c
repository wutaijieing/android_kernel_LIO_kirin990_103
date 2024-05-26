/*
 * lpcpu_cpufreq_hv.c
 *
 * cpufreq driver for those support hw_vote
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/platform_drivers/hw_vote.h>
#include <linux/platform_drivers/lpcpu_cpufreq_req.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/version.h>

#if !defined(CONFIG_ARCH_PLATFORM) || !defined(CONFIG_AB_APCP_NEW_INTERFACE)
#include <linux/platform_drivers/hisi_cpufreq_req.h>
#endif
#ifdef CONFIG_DRG
#include <linux/drg.h>
#endif

#define VERSION_ELEMENTS	1
static unsigned int g_hv_cpufreq_version = 0;

struct private_data {
	struct opp_table *opp_table;
	struct device *cpu_dev;
	struct hvdev *cpu_hvdev;
	bool have_static_opps;
};

struct cpufreq_req_wrap {
	union {
		struct freq_qos_request *request;
		struct notifier_block nb;
	} data;
	int cpu;
	unsigned int freq;
};

int lpcpu_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
	int ret;
	struct cpufreq_req_wrap *req_wrap = NULL;
	struct cpufreq_policy *policy = NULL;

	if (req == NULL || cpu < 0 || (unsigned int)cpu >= nr_cpu_ids)
		return -EINVAL;

	policy = cpufreq_cpu_get_raw((unsigned int)cpu);
	if (policy == NULL) {
		pr_err("%s: no policy of cpu%d\n", __func__, cpu);
		return -EINVAL;
	}

	req_wrap = (struct cpufreq_req_wrap *)req;
	req_wrap->data.request = (struct freq_qos_request *)
		kzalloc(sizeof(struct freq_qos_request), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(req_wrap->data.request)) {
		pr_err("%s: alloc freq qos fail\n", __func__);
		return -EINVAL;
	}

	ret = freq_qos_add_request(&policy->constraints,
				   req_wrap->data.request, FREQ_QOS_MIN, 0);
	if (ret < 0) {
		pr_err("%s: freq qos add req fail\n", __func__);
		goto free_request;
	}

	return 0;

free_request:
	kfree(req_wrap->data.request);
	return -ENOMEM;
}
EXPORT_SYMBOL(lpcpu_cpufreq_init_req);

void lpcpu_cpufreq_update_req(struct cpufreq_req *req, unsigned int freq)
{
	int ret;
	struct cpufreq_req_wrap *req_wrap = NULL;

	if (unlikely(IS_ERR_OR_NULL(req)))
		return;

	req_wrap = (struct cpufreq_req_wrap *)req;
	ret = freq_qos_update_request(req_wrap->data.request, freq);
	if (ret < 0)
		pr_err("%s: update freq req fail%d\n", __func__, ret);
}
EXPORT_SYMBOL(lpcpu_cpufreq_update_req);

void lpcpu_cpufreq_exit_req(struct cpufreq_req *req)
{
	struct cpufreq_req_wrap *req_wrap = NULL;

	if (unlikely(IS_ERR_OR_NULL(req)))
		return;

	req_wrap = (struct cpufreq_req_wrap *)req;

	freq_qos_remove_request(req_wrap->data.request);
	kfree(req_wrap->data.request);
}
EXPORT_SYMBOL(lpcpu_cpufreq_exit_req);

#if !defined(CONFIG_ARCH_PLATFORM) || !defined(CONFIG_AB_APCP_NEW_INTERFACE)
/* for old plat */
int hisi_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
	return lpcpu_cpufreq_init_req(req, cpu);
}
EXPORT_SYMBOL(hisi_cpufreq_init_req);

void hisi_cpufreq_update_req(struct cpufreq_req *req, unsigned int freq)
{
	lpcpu_cpufreq_update_req(req, freq);
}
EXPORT_SYMBOL(hisi_cpufreq_update_req);

void hisi_cpufreq_exit_req(struct cpufreq_req *req)
{
	lpcpu_cpufreq_exit_req(req);
}
EXPORT_SYMBOL(hisi_cpufreq_exit_req);
#endif

static struct freq_attr *cpufreq_hv_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,   /* Extra space for boost-attr if required */
	NULL,
};

static int hv_set_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct private_data *priv = policy->driver_data;
	struct hvdev *cpu_hvdev = priv->cpu_hvdev;
	unsigned long freq = policy->freq_table[index].frequency;
	int ret;

	ret = hv_set_freq(cpu_hvdev, freq);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	if (ret == 0)
		arch_set_freq_scale(policy->related_cpus, freq,
				    policy->cpuinfo.max_freq);
#endif

	return ret;
}

static void hv_cpufreq_policy_cur_init(struct hvdev *cpu_hvdev,
				       struct cpufreq_policy *policy)
{
	int ret;
	unsigned int freq_khz = 0;

	ret = hv_get_result(cpu_hvdev, &freq_khz);
	if (ret != 0 || freq_khz == 0) {
		pr_err("%s:find freq%u fail: %d\n", __func__, freq_khz, ret);
		policy->cur = policy->freq_table[0].frequency;
		return;
	}
	policy->cur = freq_khz;
}

static struct hvdev *cpu_hvdev_init(struct device *cpu_dev)
{
	struct device_node *np = NULL;
	struct hvdev *cpu_hvdev = NULL;
	const char *ch_name = NULL;
	const char *vsrc = NULL;
	int ret;

	np = cpu_dev->of_node;
	if (IS_ERR_OR_NULL(np)) {
		pr_err("%s: cpu_dev no dt node!\n", __func__);
		goto err_out;
	}

	ret = of_property_read_string_index(np, "freq-vote-channel", 0,
					    &ch_name);
	if (ret != 0) {
		pr_err("[%s]parse freq-vote-channel fail!\n", __func__);
		goto err_out;
	}

	ret = of_property_read_string_index(np, "freq-vote-channel", 1,
					    &vsrc);
	if (ret != 0) {
		pr_err("[%s]parse vote src fail!\n", __func__);
		goto err_out;
	}

	cpu_hvdev = hvdev_register(cpu_dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(cpu_hvdev))
		pr_err("%s: cpu_hvdev register fail!\n", __func__);

err_out:
	return cpu_hvdev;
}

static int hv_cpufreq_init(struct cpufreq_policy *policy)
{
	struct cpufreq_frequency_table *freq_table = NULL;
	struct opp_table *opp_table = NULL;
	struct private_data *priv = NULL;
	struct device *cpu_dev = NULL;
	unsigned int transition_latency;
	int ret;

	cpu_dev = get_cpu_device(policy->cpu);
	if (cpu_dev == NULL) {
		pr_err("failed to get cpu%d device\n", policy->cpu);
		return -ENODEV;
	}

	/* Only support "operating-points-v2" bindings */
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, policy->cpus);
	if (ret != 0) {
		pr_err("opp get sharing cpus fail%d\n", ret);
		return ret;
	}

	opp_table = dev_pm_opp_set_supported_hw(cpu_dev, &g_hv_cpufreq_version,
						VERSION_ELEMENTS);
	if (IS_ERR(opp_table)) {
		dev_err(cpu_dev, "fail to set supported hw %d\n", ret);
		return PTR_ERR(opp_table);
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		ret = -ENOMEM;
		goto out_put_hw;
	}

	/*
	 * Initialize OPP tables for all policy->cpus. They will be shared by
	 * all CPUs which have marked their CPUs shared with OPP bindings.
	 *
	 * For platforms not using operating-points-v2 bindings, we do this
	 * before updating policy->cpus. Otherwise, we will end up creating
	 * duplicate OPPs for policy->cpus.
	 *
	 * OPPs might be populated at runtime, don't check for error here
	 */
	if (dev_pm_opp_of_cpumask_add_table(policy->cpus) == 0)
		priv->have_static_opps = true;

	/*
	 * But we need OPP table to function so if it is not there let's
	 * give platform code chance to provide it for us.
	 */
	ret = dev_pm_opp_get_opp_count(cpu_dev);
	if (ret <= 0) {
		dev_dbg(cpu_dev, "OPP table is not ready, deferring probe\n");
		ret = -EPROBE_DEFER;
		goto out_free_opp;
	}
	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table);
	if (ret != 0) {
		dev_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		goto out_free_opp;
	}

	priv->cpu_dev = cpu_dev;
	priv->opp_table = opp_table;
	priv->cpu_hvdev = cpu_hvdev_init(cpu_dev);

	policy->driver_data = priv;
	policy->freq_table = freq_table;
	policy->suspend_freq = dev_pm_opp_get_suspend_opp_freq(cpu_dev) / 1000;

	/* Support turbo/boost mode */
	if (policy_has_boost_freq(policy)) {
		/* This gets disabled by core on driver unregister */
		ret = cpufreq_enable_boost_support();
		if (ret != 0)
			goto out_free_cpufreq_table;
		cpufreq_hv_attr[1] = &cpufreq_freq_attr_scaling_boost_freqs;
	}
	transition_latency = dev_pm_opp_get_max_transition_latency(cpu_dev);
	if (!transition_latency)
		transition_latency = CPUFREQ_ETERNAL;

	policy->cpuinfo.transition_latency = transition_latency;

	policy->dvfs_possible_from_any_cpu = true;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	dev_pm_opp_of_register_em(cpu_dev, policy->cpus);
#else
	dev_pm_opp_of_register_em(policy->cpus);
#endif

	hv_cpufreq_policy_cur_init(priv->cpu_hvdev, policy);

	return 0;

out_free_cpufreq_table:
	dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table);
out_free_opp:
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(policy->cpus);
	kfree(priv);
out_put_hw:
	dev_pm_opp_put_supported_hw(opp_table);

	return ret;
}

static int hv_cpufreq_online(struct cpufreq_policy *policy __maybe_unused)
{
	/* We did light-weight tear down earlier, nothing to do here */
	return 0;
}

static int hv_cpufreq_offline(struct cpufreq_policy *policy __maybe_unused)
{
	/*
	 * Preserve policy->driver_data and don't free resources on light-weight
	 * tear down.
	 */
	return 0;
}

static void cpu_hvdev_exit(struct hvdev *cpu_hvdev, unsigned int cpu)
{
	if (hvdev_remove(cpu_hvdev) != 0)
		pr_err("cpu%u unregister hvdev fail\n", cpu);
}

static int hv_cpufreq_exit(struct cpufreq_policy *policy)
{
	struct private_data *priv = policy->driver_data;

#ifdef CONFIG_DRG
	drg_cpufreq_unregister(policy);
#endif
	dev_pm_opp_free_cpufreq_table(priv->cpu_dev, &policy->freq_table);
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(policy->related_cpus);
	cpu_hvdev_exit(priv->cpu_hvdev, policy->cpu);
	dev_pm_opp_put_supported_hw(priv->opp_table);

	kfree(priv);

	return 0;
}

static unsigned int hv_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(cpu);

	if (policy == NULL) {
		pr_err("%s: No policy associated to cpu: %u\n", __func__, cpu);
		return 0;
	}

	return policy->cur;
}

static void hv_cpufreq_ready(struct cpufreq_policy *policy __maybe_unused)
{
#ifdef CONFIG_DRG
	drg_cpufreq_register(policy);
#endif
}

static struct cpufreq_driver hv_cpufreq_driver = {
	.flags = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK |
		 CPUFREQ_IS_COOLING_DEV | CPUFREQ_HAVE_GOVERNOR_PER_POLICY,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = hv_set_target,
	.get = hv_cpufreq_get,
	.init = hv_cpufreq_init,
	.exit = hv_cpufreq_exit,
	.online = hv_cpufreq_online,
	.offline = hv_cpufreq_offline,
	.name = "cpufreq_hv",
	.attr = cpufreq_hv_attr,
	.suspend = cpufreq_generic_suspend,
	.ready = hv_cpufreq_ready,
};

int lpcpu_get_cpu_version(unsigned int *cpu_version)
{
	const char *target_cpu = NULL;
	const char *support_efuse = NULL;
	const char *name_str = "target_cpu";
	struct device_node *tarcpu_np = NULL;
	struct device_node *support_target_np = NULL;
	int ret, index;

	if (cpu_version == NULL) {
		pr_err("%s input para invalid\n", __func__);
		return -EINVAL;
	}

	support_target_np = of_find_compatible_node(NULL, NULL,
						    "hisilicon,supportedtarget");
	if (support_target_np == NULL) {
		pr_err("%s Failed to find compatible node:supportedtarget\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_string(support_target_np, "support_efuse",
				      &support_efuse);
	if (ret != 0) {
		pr_err("%s Failed to read support_efuse\n", __func__);
		goto err_support_target_np_op;
	}

	if (!strncmp(support_efuse, "true", strlen("true")))
		name_str = "efuse_cpu";

	tarcpu_np = of_find_compatible_node(NULL, NULL, "hisilicon,targetcpu");
	if (tarcpu_np == NULL) {
		pr_err("%s Failed to find compatible node:targetcpu\n",
		       __func__);
		ret = -ENODEV;
		goto err_support_target_np_op;
	}

	ret = of_property_read_string(tarcpu_np, name_str, &target_cpu);
	if (ret != 0) {
		pr_err("%s Failed to read %s\n", __func__, name_str);
		goto err_node_op;
	}

	ret = of_property_match_string(support_target_np, "support_name",
				       target_cpu);
	if (ret < 0) {
		pr_err("%s Failed to get support_name\n", __func__);
		goto err_node_op;
	}

	index = ret;
	*cpu_version = BIT((unsigned int)index);
	ret = 0;

err_node_op:
	of_node_put(tarcpu_np);
err_support_target_np_op:
	of_node_put(support_target_np);

	return ret;
}

static int hv_cpufreq_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = of_find_compatible_node(NULL, NULL,
							 "arm,generic-bL-cpufreq");
	if (np == NULL)
		return -ENODEV;
	of_node_put(np);

	ret = lpcpu_get_cpu_version(&g_hv_cpufreq_version);
	if (ret != 0)
		return ret;

	ret = cpufreq_register_driver(&hv_cpufreq_driver);
	if (ret)
		dev_err(&pdev->dev, "failed register driver: %d\n", ret);

	return ret;
}

static int hv_cpufreq_remove(struct platform_device *pdev __maybe_unused)
{
	cpufreq_unregister_driver(&hv_cpufreq_driver);
	return 0;
}

static struct platform_driver hv_cpufreq_platdrv = {
	.driver = {
		.name	= "cpufreq_hv",
	},
	.probe		= hv_cpufreq_probe,
	.remove		= hv_cpufreq_remove,
};
module_platform_driver(hv_cpufreq_platdrv);

static struct platform_device *hv_cpufreq_platdev = NULL;

static int __init lpcpu_cpufreq_hv_platdev_init(void)
{
	int ret;

	hv_cpufreq_platdev =
		platform_device_register_simple("cpufreq_hv",
						-1, NULL, 0);
	ret = PTR_ERR_OR_ZERO(hv_cpufreq_platdev);
	if (ret != 0)
		pr_err("lpcpu hv cpufreq platdev init fail\n");
	return ret;
}
device_initcall(lpcpu_cpufreq_hv_platdev_init);

MODULE_ALIAS("platform:cpufreq_hv");
MODULE_DESCRIPTION("cpufreq driver for hwvote");
MODULE_LICENSE("GPL v2");
