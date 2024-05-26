/*
 * audio_trace.h
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

#ifndef _AUDIO_TRACE_H_
#define _AUDIO_TRACE_H_

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/device.h>

enum {
	ATRACE_START_ID = 0,
	ATRACE_PCM_ID = ATRACE_START_ID,
	ATRACE_MEM_ID,
	ATRACE_MAX_ID,
};

#define ATRACE_CMD_SECTION  10000

enum {
	PCM_TRACE_OPEN = ATRACE_CMD_SECTION * ATRACE_PCM_ID,
	PCM_TRACE_CLOSE,
	PCM_TRACE_WRITE,
	PCM_TRACE_READ,

	MEM_TRACE_MAP = ATRACE_CMD_SECTION * ATRACE_MEM_ID,
	MEM_TRACE_UNMAP,
};

struct mem_trace_info {
	char *tag;
	phys_addr_t phy_addr;
	size_t mem_bytes;
	int32_t mmap_hdl;
	int32_t extra_size;
	void *extra;
};

struct atrace_client {
	struct list_head list;
	uint32_t id;
	struct device *dev;
	/* client is responsible for init follow */
	void *priv;
	void (*trigger)(void *);
	void (*report)(void *, struct device *, uint32_t, void *, uint32_t);
	void (*reset)(void *);
	void (*free)(void *);
};

#ifdef CONFIG_AUDIO_TRACE
struct atrace_client *atrace_register(uint32_t id, struct device *dev);
void atrace_unregister(uint32_t id);
void atrace_report(struct device *dev, uint32_t cmd, void*data, uint32_t size);
void atrace_trigger(bool reset);
#else
static inline struct atrace_client *atrace_register(uint32_t id, struct device *dev)
{
	return NULL;
}

static inline void atrace_unregister(uint32_t id)
{
}

static inline void atrace_report(struct device *dev, uint32_t cmd,
	void *data, uint32_t size)
{
}

static inline void atrace_trigger(bool reset)
{
}
#endif /* CONFIG_AUDIO_TRACE */

#endif /* _AUDIO_TRACE_H_ */