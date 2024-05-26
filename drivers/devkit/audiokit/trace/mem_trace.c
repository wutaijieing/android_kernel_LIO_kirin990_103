/*
 * mem_trace.c
 *
 * trace mem map
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

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include "audio_trace.h"
#include "trace_utils.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG audio_trace
HWLOG_REGIST();

struct mem_trace_list {
	struct list_head list;
	struct tm tm;
	struct mem_trace_info info;
};

struct mem_trace_dev {
	struct device *dev;
	struct list_head mem_list;
	uint32_t mem_total_bytes;
};

static void mem_trace_print(int index, const struct mem_trace_list *mlist)
{
	if ((mlist->info.extra_size == sizeof(int32_t)) && mlist->info.extra)
		hwlog_info("%d-%s map:%u-%u-%u %u:%u:%u,%pk %x %u extra=%d\n",
			index, mlist->info.tag,
			mlist->tm.tm_year + TRACE_BASIC_YEAR, mlist->tm.tm_mon + 1,
			mlist->tm.tm_mday, mlist->tm.tm_hour,
			mlist->tm.tm_min, mlist->tm.tm_sec,
			&mlist->info.phy_addr, mlist->info.mmap_hdl,
			mlist->info.mem_bytes,
			*((int32_t *)mlist->info.extra));
	else
		hwlog_info("%d-%s map:%u-%u-%u %u:%u:%u,%pk %x %u\n",
			index, mlist->info.tag,
			mlist->tm.tm_year + TRACE_BASIC_YEAR, mlist->tm.tm_mon + 1,
			mlist->tm.tm_mday, mlist->tm.tm_hour,
			mlist->tm.tm_min, mlist->tm.tm_sec,
			&mlist->info.phy_addr, mlist->info.mmap_hdl,
			mlist->info.mem_bytes);
}

static void mem_trace_trigger(void *priv)
{
	struct mem_trace_dev *di = priv;
	struct mem_trace_list *mlist = NULL;
	int index = 0;

	if (!di)
		return;

	hwlog_info("mem total=%u\n", di->mem_total_bytes);
	list_for_each_entry(mlist, &di->mem_list, list) {
		mem_trace_print(index, mlist);
		index++;
	}
}

static void mem_trace_map(struct mem_trace_dev *di,
	void *data, uint32_t size)
{
	struct mem_trace_info *info = data;
	struct mem_trace_list *mlist = NULL;

	if (!info || (size != sizeof(*info)) || !info->tag)
		return;

	if (trace_real_log())
		hwlog_info("%s map now %pk %x %u\n", info->tag,
			info->phy_addr, info->mmap_hdl, info->mem_bytes);

	mlist = kzalloc(sizeof(*mlist), GFP_KERNEL);
	if (!mlist)
		return;

	mlist->info.tag =  kzalloc(strlen(info->tag) + 1, GFP_KERNEL);
	if (!mlist->info.tag) {
		kfree(mlist);
		return;
	}

	strcpy(mlist->info.tag, info->tag);
	INIT_LIST_HEAD(&mlist->list);
	mlist->info.mem_bytes = info->mem_bytes;
	mlist->info.phy_addr = info->phy_addr;
	mlist->info.mmap_hdl = info->mmap_hdl;
	list_add_tail(&mlist->list, &di->mem_list);
	trace_get_time(&mlist->tm);
	di->mem_total_bytes += info->mem_bytes;

	if (!info->extra || (info->extra_size == 0))
		return;

	mlist->info.extra = kzalloc(info->extra_size, GFP_KERNEL);
	if (!mlist->info.extra)
		return;

	mlist->info.extra_size = info->extra_size;
	memcpy(mlist->info.extra, info->extra, info->extra_size);
}

static void mem_trace_free_node(struct mem_trace_list *node)
{
	if (node->info.extra) {
		kfree(node->info.extra);
		node->info.extra = NULL;
	}

	if (node->info.tag) {
		kfree(node->info.tag);
		node->info.tag = NULL;
	}

	kfree(node);
}

static void mem_trace_unmap(struct mem_trace_dev *di,
	void *data, uint32_t size)
{
	struct mem_trace_info *info = data;
	struct mem_trace_list *mlist = NULL;
	struct mem_trace_list *tlist = NULL;

	if (!info || (size != sizeof(*info)) || !info->tag)
		return;

	if (trace_real_log())
		hwlog_info("%s unmap now %pk %x %u\n", info->tag,
			info->phy_addr, info->mmap_hdl, info->mem_bytes);

	list_for_each_entry_safe(mlist, tlist, &di->mem_list, list) {
		if (info->mmap_hdl != mlist->info.mmap_hdl)
			continue;

		if (strcmp(info->tag, mlist->info.tag))
			hwlog_err("%s:%u-%u-%u %u:%u:%u, %pk:%pk %x, %d-%d, tag:%s,%s\n",
				__func__,
				mlist->tm.tm_year + TRACE_BASIC_YEAR,
				mlist->tm.tm_mon + 1, mlist->tm.tm_mday,
				mlist->tm.tm_hour, mlist->tm.tm_min,
				mlist->tm.tm_sec,
				&info->phy_addr, mlist->info.phy_addr,
				info->mmap_hdl,
	 			info->mem_bytes, mlist->info.mem_bytes,
	 			info->tag, mlist->info.tag);

		if (di->mem_total_bytes >= mlist->info.mem_bytes)
			di->mem_total_bytes -= mlist->info.mem_bytes;
		else
			di->mem_total_bytes = 0;

		list_del_init(&mlist->list);
		mem_trace_free_node(mlist);
		return;
	}
}

static void mem_trace_report(void *priv, struct device *dev, uint32_t cmd,
	void *data, uint32_t size)
{
	struct mem_trace_dev *di = priv;

	if (!di)
		return;

	if (cmd == MEM_TRACE_MAP)
		mem_trace_map(di, data, size);
	else if (cmd == MEM_TRACE_UNMAP)
		mem_trace_unmap(di, data, size);
}

static void mem_trace_reset(void *priv)
{
	struct mem_trace_dev *di = priv;
	struct mem_trace_list *mlist = NULL;
	struct mem_trace_list *tlist = NULL;

	if (!di)
		return;

	list_for_each_entry_safe(mlist, tlist, &di->mem_list, list) {
		list_del_init(&mlist->list);
		mem_trace_free_node(mlist);
	}
	di->mem_total_bytes = 0;
}

static void mem_trace_free(void *priv)
{
	struct mem_trace_dev *di = priv;

	if (!di)
		return;

	mem_trace_reset(priv);
	devm_kfree(di->dev, di);
}

static int mem_trace_probe(struct platform_device *pdev)
{
	struct atrace_client *client = NULL;
	struct mem_trace_dev *di = NULL;
	struct device *dev = &pdev->dev;

	di = devm_kzalloc(dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	di->dev = dev;
	INIT_LIST_HEAD(&di->mem_list);

	client = atrace_register(ATRACE_MEM_ID, &pdev->dev);
	if (!client) {
		devm_kfree(dev, di);
		return -ENOMEM;
	}
	client->report = mem_trace_report;
	client->trigger = mem_trace_trigger;
	client->reset = mem_trace_reset;
	client->free = mem_trace_free;
	client->priv = di;

	return 0;
}

static int mem_trace_remove(struct platform_device *pdev)
{
	atrace_unregister(ATRACE_MEM_ID);
	return 0;
}

static const struct of_device_id mem_trace_match_table[] = {
	{ .compatible = "huawei,audio-mem-trace" },
	{},
};

static struct platform_driver mem_trace_driver = {
	.driver = {
		.name = "mem_trace",
		.owner = THIS_MODULE,
		.of_match_table = mem_trace_match_table,
	},
	.probe = mem_trace_probe,
	.remove = mem_trace_remove,
};
module_platform_driver(mem_trace_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei mem trace driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
