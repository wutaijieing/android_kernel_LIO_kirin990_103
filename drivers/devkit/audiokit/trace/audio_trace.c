/*
 * audio_trace.c
 *
 * audio trace
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "audio_trace.h"
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/timex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/soc/qcom/pmic_glink.h>
#include <huawei_platform/log/hw_log.h>
#include <log/log_usertype.h>
#include "trace_utils.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG audio_trace
HWLOG_REGIST();

#define ATRACE_DEF_PERIOD        10 /* s */
#define ATRACE_AUTOT_PERIOD      3600 /* s */

struct atrace_dev {
	struct device *dev;
	struct dentry *root;
	struct delayed_work trace_work;
	uint32_t trace_period;
	uint32_t auto_trigger;
	uint32_t auto_cnts;
	struct list_head client_list;
	struct mutex client_lock;
};

static struct atrace_dev *g_atrace_dev;

struct atrace_client *atrace_find_client(struct atrace_dev *di, uint32_t id)
{
	struct atrace_client *client = NULL;

	list_for_each_entry(client, &di->client_list, list) {
		if (client->id == id)
			return client;
	}

	return NULL;
}

struct atrace_client *atrace_register(uint32_t id, struct device *dev)
{
	struct atrace_dev *di = g_atrace_dev;
	struct atrace_client *client = NULL;

	if (!di)
		return NULL;

	mutex_lock(&di->client_lock);
	client = atrace_find_client(di, id);
	if (client) {
		mutex_unlock(&di->client_lock);
		hwlog_err("client %d has register\n", id);
		return NULL;
	}

	client = devm_kzalloc(di->dev, sizeof(*client), GFP_KERNEL);
	if (client) {
		client->id = id;
		client->dev = dev;
		INIT_LIST_HEAD(&client->list);
		list_add_tail(&client->list, &di->client_list);
	}
	mutex_unlock(&di->client_lock);
	return client;
}

void atrace_unregister(uint32_t id)
{
	struct atrace_dev *di = g_atrace_dev;
	struct atrace_client *client = NULL;

	if (!di)
		return;

	mutex_lock(&di->client_lock);
	client = atrace_find_client(di, id);
	if (!client) {
		mutex_unlock(&di->client_lock);
		hwlog_err("client %d not register\n", id);
		return;
	}

	if (client->free)
		client->free(client->priv);

	list_del_init(&client->list);
	devm_kfree(di->dev, client);
	mutex_unlock(&di->client_lock);
}

void atrace_report(struct device *dev, uint32_t cmd, void *data, uint32_t size)
{
	struct atrace_dev *di = g_atrace_dev;
	struct atrace_client *client = NULL;
	uint32_t id = cmd / ATRACE_CMD_SECTION;

	if (!di)
		return;

	mutex_lock(&di->client_lock);
	client = atrace_find_client(di, id);
	if (client && client->report)
		client->report(client->priv, dev, cmd, data, size);
	mutex_unlock(&di->client_lock);
}
EXPORT_SYMBOL_GPL(atrace_report);

static void _atrace_trigger(struct atrace_dev *di)
{
	struct atrace_client *client = NULL;

	hwlog_info("audio trace start\n");
	mutex_lock(&di->client_lock);
	list_for_each_entry(client, &di->client_list, list) {
		if (!client->trigger)
			continue;

		hwlog_info("%s trace start\n", dev_name(client->dev));
		client->trigger(client->priv);
		hwlog_info("%s trace end\n", dev_name(client->dev));
	}
	mutex_unlock(&di->client_lock);
	hwlog_info("audio trace end\n");
}

static void atrace_reset(struct atrace_dev *di)
{
	struct atrace_client *client = NULL;

	hwlog_info("audio trace reset\n");
	mutex_lock(&di->client_lock);
	list_for_each_entry(client, &di->client_list, list) {
		if (!client->reset)
			continue;

		client->reset(client->priv);
	}
	mutex_unlock(&di->client_lock);
}

void atrace_trigger(bool reset)
{
	if (!g_atrace_dev)
		return;

	_atrace_trigger(g_atrace_dev);

	if (reset)
		atrace_reset(g_atrace_dev);
}
EXPORT_SYMBOL_GPL(atrace_trigger);

static int atrace_period_set(void *data, u64 val)
{
	struct atrace_dev *di = data;

	hwlog_info("%s: set trace period to %llu\n", __func__, val);
	di->trace_period = val;
	return 0;
}

static int atrace_period_get(void *data, u64 *val)
{
	struct atrace_dev *di = data;

	*val = di->trace_period;
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(atrace_period_fops,
	atrace_period_get, atrace_period_set, "%llu\n");

static int atrace_trigger_set(void *data, u64 val)
{
	struct atrace_dev *di = data;

	hwlog_info("%s\n", __func__);
	_atrace_trigger(di);
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(atrace_trigger_fops,
	NULL, atrace_trigger_set, "%llu\n");


static int atrace_auto_trigger_set(void *data, u64 val)
{
	struct atrace_dev *di = data;

	hwlog_info("%s: set auto_trigger to %llu\n", __func__, val);
	di->auto_trigger = val;
	return 0;
}

static int utrace_auto_trigger_get(void *data, u64 *val)
{
	struct atrace_dev *di = data;

	*val = di->auto_trigger;
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(atrace_auto_trigger_fops,
	utrace_auto_trigger_get, atrace_auto_trigger_set, "%llu\n");

static int atrace_real_log_set(void *data, u64 val)
{
	trace_real_log_set(val > 0);
	return 0;
}

static int utrace_real_log_get(void *data, u64 *val)
{
	*val = trace_real_log();
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(atrace_real_log_fops,
	utrace_real_log_get, atrace_real_log_set, "%llu\n");

static void atrace_create_debugfs(struct atrace_dev *di)
{
	di->root = debugfs_create_dir("audio_trace", NULL);
	if (!di->root) {
		hwlog_err("%s: create debugfs root node fail\n", __func__);
		return;
	}

	debugfs_create_file("work_period", 0600, di->root, di, &atrace_period_fops);
	debugfs_create_file("trigger", 0200, di->root, di, &atrace_trigger_fops);
	debugfs_create_file("auto_trigger_period", 0600, di->root, di,
		&atrace_auto_trigger_fops);
	debugfs_create_file("real_log", 0200, di->root, di, &atrace_real_log_fops);
}

static void atrace_work(struct work_struct *work)
{
	struct atrace_dev *di = container_of(work, struct atrace_dev,
		trace_work.work);

	di->auto_cnts += di->trace_period;
	if (di->auto_cnts >= di->auto_trigger) {
		di->auto_cnts = 0;
		_atrace_trigger(di);
	}

	queue_delayed_work(system_power_efficient_wq, &di->trace_work,
		msecs_to_jiffies(MSEC_PER_SEC * di->trace_period));
}

static bool atrace_check_version(void)
{
#ifndef DBG_AUDIO_TRACE
	unsigned int type = get_log_usertype();

	if ((type != BETA_USER) && (type != TEST_USER))
		return false;
#endif /* DBG_AUDIO_TRACE */

	return true;
}

static int atrace_probe(struct platform_device *pdev)
{
	int rc;
	struct atrace_dev *di = NULL;
	struct device *dev = &pdev->dev;

	if (!atrace_check_version())
		return 0;

	di = devm_kzalloc(dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = dev;
	di->trace_period = ATRACE_DEF_PERIOD;
	di->auto_trigger = ATRACE_AUTOT_PERIOD;
	mutex_init(&di->client_lock);
	INIT_LIST_HEAD(&di->client_list);
	INIT_DELAYED_WORK(&di->trace_work, atrace_work);
	platform_set_drvdata(pdev, di);
	atrace_create_debugfs(di);

	g_atrace_dev = di;
	rc = of_platform_populate(dev->of_node, NULL, NULL, dev);
	if (rc < 0)
		hwlog_err("failed to create child devices rc=%d\n", rc);

	queue_delayed_work(system_power_efficient_wq, &di->trace_work,
		msecs_to_jiffies(MSEC_PER_SEC * di->trace_period));
	return 0;
}

static int atrace_remove(struct platform_device *pdev)
{
	struct atrace_dev *di = platform_get_drvdata(pdev);
	int id;

	if (!di)
		return 0;

	debugfs_remove_recursive(di->root);
	di->root = NULL;
	for (id = ATRACE_START_ID; id < ATRACE_MAX_ID; id++)
		atrace_unregister(id);

	mutex_destroy(&di->client_lock);
	return 0;
}

static const struct of_device_id atrace_match_table[] = {
	{ .compatible = "huawei,audio-trace" },
	{},
};

static struct platform_driver atrace_driver = {
	.driver = {
		.name = "audio_trace",
		.owner = THIS_MODULE,
		.of_match_table = atrace_match_table,
	},
	.probe = atrace_probe,
	.remove = atrace_remove,
};
module_platform_driver(atrace_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei audio trace driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
