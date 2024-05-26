/*
 * pcm_trace.c
 *
 * trace pcm info
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

struct pcm_trace_stream {
	struct list_head list;
	struct device *dev;
	struct tm tm;
	uint32_t dir;
	uint32_t stream_bytes;
	uint32_t ref_cnt;
};

struct pcm_trace_dev {
	struct list_head pcm_list;
	struct device *dev;
	uint32_t streams;
};

static void pcm_trace_trigger(void *priv)
{
	struct pcm_trace_dev *di = priv;
	struct pcm_trace_stream *pcm = NULL;
	int index = 0;

	if (!di)
		return;

	list_for_each_entry(pcm, &di->pcm_list, list) {
		hwlog_info("%d-%s open:%u-%u-%u %u:%u:%u,ref_cnt=%u,%s bytes=%u\n",
			index, dev_name(pcm->dev),
			pcm->tm.tm_year + TRACE_BASIC_YEAR, pcm->tm.tm_mon + 1,
			pcm->tm.tm_mday, pcm->tm.tm_hour, pcm->tm.tm_min, pcm->tm.tm_sec,
			pcm->ref_cnt,
			(pcm->dir == 0) ? "write" : "read",
			pcm->stream_bytes);

		index++;
	}
}

struct pcm_trace_stream *pcm_trace_find_pcm(struct pcm_trace_dev *di,
	struct device *dev)
{
	struct pcm_trace_stream *pcm = NULL;

	list_for_each_entry(pcm, &di->pcm_list, list) {
		if (!strcmp(dev_name(dev), dev_name(pcm->dev)))
			return pcm;
	}

	return NULL;
}

static void pcm_trace_open(struct pcm_trace_dev *di, struct device *dev,
	void *data, uint32_t size)
{
	struct pcm_trace_stream *pcm = NULL;

	if (!data || (size != sizeof(uint32_t)))
		return;

	pcm = pcm_trace_find_pcm(di, dev);
	if (pcm) {
		pcm->ref_cnt++;
		hwlog_info("pcm %s open ref_cnt=%u\n",  dev_name(dev), pcm->ref_cnt);
		return;
	}

	if (trace_real_log())
		hwlog_info("%s open now\n", dev_name(dev));

	pcm = kzalloc(sizeof(*pcm), GFP_KERNEL);
	if (!pcm)
		return;

	pcm->dev = dev;
	pcm->dir = *(uint32_t *)data;
	pcm->ref_cnt++;
	trace_get_time(&pcm->tm);
	INIT_LIST_HEAD(&pcm->list);
	list_add_tail(&pcm->list, &di->pcm_list);
}

static void pcm_trace_close(struct pcm_trace_stream *pcm,
	void *data, uint32_t size)
{
	if (trace_real_log())
		hwlog_info("%s open:%u-%u-%u %u:%u:%u, ref_cnt=%u, close now\n",
			dev_name(pcm->dev),
			pcm->tm.tm_year + TRACE_BASIC_YEAR,
			pcm->tm.tm_mon + 1,
			pcm->tm.tm_mday, pcm->tm.tm_hour,
			pcm->tm.tm_min, pcm->tm.tm_sec,
			pcm->ref_cnt);

	if (pcm->ref_cnt > 0)
		pcm->ref_cnt--;

	if (pcm->ref_cnt > 0)
		return;

	list_del_init(&pcm->list);
	kfree(pcm);
}

static void pcm_trace_write(struct pcm_trace_stream *pcm,
	void *data, uint32_t size)
{
	if (!data || (size != sizeof(uint32_t)))
		return;

	if (pcm->dir != 0)
		return;

	pcm->stream_bytes += *(uint32_t *)data;
}

static void pcm_trace_read(struct pcm_trace_stream *pcm,
	void *data, uint32_t size)
{
	if (!data || (size != sizeof(uint32_t)))
		return;

	/* 1: capture */
	if (pcm->dir != 1)
		return;

	pcm->stream_bytes += *(uint32_t *)data;
}

static void pcm_trace_proc_cmd(struct pcm_trace_stream *pcm,
	uint32_t cmd, void *data, uint32_t size)
{
	switch (cmd) {
	case PCM_TRACE_CLOSE:
		pcm_trace_close(pcm, data, size);
		break;
	case PCM_TRACE_WRITE:
		pcm_trace_write(pcm, data, size);
		break;
	case PCM_TRACE_READ:
		pcm_trace_read(pcm, data, size);
		break;
	default:
		break;
	}
}

static void pcm_trace_report(void *priv, struct device *dev, uint32_t cmd,
	void *data, uint32_t size)
{
	struct pcm_trace_dev *di = priv;
	struct pcm_trace_stream *pcm = NULL;

	if (!di || !dev)
		return;

	if (cmd == PCM_TRACE_OPEN) {
		pcm_trace_open(di, dev, data, size);
	} else {
		pcm = pcm_trace_find_pcm(di, dev);
		if (!pcm) {
			hwlog_err("pcm %s not open for %u\n",  dev_name(dev), cmd);
			return;
		}

		pcm_trace_proc_cmd(pcm, cmd, data, size);
	}
}

static void pcm_trace_free(void *priv)
{
	struct pcm_trace_dev *di = priv;
	struct pcm_trace_stream *pcm = NULL;
	struct pcm_trace_stream *tpcm = NULL;

	if (!di)
		return;

	list_for_each_entry_safe(pcm, tpcm, &di->pcm_list, list) {
		list_del_init(&pcm->list);
		kfree(pcm);
	}

	devm_kfree(di->dev, di);
}

static int pcm_trace_probe(struct platform_device *pdev)
{
	struct atrace_client *client = NULL;
	struct pcm_trace_dev *di = NULL;
	struct device *dev = &pdev->dev;

	di = devm_kzalloc(dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	di->dev = dev;
	INIT_LIST_HEAD(&di->pcm_list);

	client = atrace_register(ATRACE_PCM_ID, &pdev->dev);
	if (!client) {
		devm_kfree(dev, di);
		return -ENOMEM;
	}
	client->report = pcm_trace_report;
	client->trigger = pcm_trace_trigger;
	client->reset = NULL;
	client->free = pcm_trace_free;
	client->priv = di;

	return 0;
}

static int pcm_trace_remove(struct platform_device *pdev)
{
	atrace_unregister(ATRACE_PCM_ID);
	return 0;
}

static const struct of_device_id pcm_trace_match_table[] = {
	{ .compatible = "huawei,pcm-trace" },
	{},
};

static struct platform_driver pcm_trace_driver = {
	.driver = {
		.name = "pcm_trace",
		.owner = THIS_MODULE,
		.of_match_table = pcm_trace_match_table,
	},
	.probe = pcm_trace_probe,
	.remove = pcm_trace_remove,
};
module_platform_driver(pcm_trace_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei pcm trace driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
