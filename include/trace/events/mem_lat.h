// SPDX-License-Identifier: GPL-2.0-only
/*
 * mem_lat.h
 *
 * memory latency trace events
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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mem_lat

#if !defined(_TRACE_MEM_LAT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MEM_LAT_H

#include <linux/tracepoint.h>
#include <linux/trace_events.h>

#ifdef CONFIG_DEVFREQ_GOV_MEMLAT
TRACE_EVENT(memlat_dev_meas,

	TP_PROTO(const char *name, unsigned int dev_id, unsigned long inst,
		 unsigned long mem, unsigned long freq, unsigned int ratio),

	TP_ARGS(name, dev_id, inst, mem, freq, ratio),

	TP_STRUCT__entry(
		__string(name, name)
		__field(unsigned int, dev_id)
		__field(unsigned long, inst)
		__field(unsigned long, mem)
		__field(unsigned long, freq)
		__field(unsigned int, ratio)
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->dev_id = dev_id;
		__entry->inst = inst;
		__entry->mem = mem;
		__entry->freq = freq;
		__entry->ratio = ratio;
	),

	TP_printk("dev:%s id=%u inst=%lu mem=%lu freq=%lu ratio=%u",
		__get_str(name),
		__entry->dev_id,
		__entry->inst,
		__entry->mem,
		__entry->freq,
		__entry->ratio)
);

TRACE_EVENT(memlat_dev_update,

	TP_PROTO(const char *name, unsigned int dev_id, unsigned long inst,
		 unsigned long mem, unsigned long freq, unsigned long vote),

	TP_ARGS(name, dev_id, inst, mem, freq, vote),

	TP_STRUCT__entry(
		__string(name, name)
		__field(unsigned int, dev_id)
		__field(unsigned long, inst)
		__field(unsigned long, mem)
		__field(unsigned long, freq)
		__field(unsigned long, vote)
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->dev_id = dev_id;
		__entry->inst = inst;
		__entry->mem = mem;
		__entry->freq = freq;
		__entry->vote = vote;
	),

	TP_printk("dev:%s id=%u inst=%lu mem=%lu freq=%lu vote=%lu",
		__get_str(name),
		__entry->dev_id,
		__entry->inst,
		__entry->mem,
		__entry->freq,
		__entry->vote)
);
#endif

#ifdef CONFIG_DEVFREQ_DEVBW
TRACE_EVENT(memlat_set_ddr_freq,

	TP_PROTO(const char *reason, int cpu,
		 unsigned long min_core_freq, unsigned long cpu_freq, unsigned long new_ddr_freq),

	TP_ARGS(reason, cpu, min_core_freq, cpu_freq, new_ddr_freq),

	TP_STRUCT__entry(
		__string(reason, reason)
		__field(int, cpu)
		__field(unsigned long, min_core_freq)
		__field(unsigned long, cpu_freq)
		__field(unsigned long, new_ddr_freq)
	),

	TP_fast_assign(
		__assign_str(reason, reason);
		__entry->cpu = cpu;
		__entry->min_core_freq = min_core_freq;
		__entry->cpu_freq = cpu_freq;
		__entry->new_ddr_freq = new_ddr_freq;
	),

	TP_printk("reason=%s  cpu=%d min_core_freq=%lu cpu_freq=%lu new_ddr_freq=%lu",
		__get_str(reason),
		__entry->cpu,
		__entry->min_core_freq,
		__entry->cpu_freq,
		__entry->new_ddr_freq)
);

TRACE_EVENT(memlat_set_dev_freq,

	TP_PROTO(const char *name, const char *reason, int cpu,
		 unsigned long min_core_freq, unsigned long cpu_freq, unsigned long new_ddr_freq),

	TP_ARGS(name, reason, cpu, min_core_freq, cpu_freq, new_ddr_freq),

	TP_STRUCT__entry(
		__string(name, name)
		__string(reason, reason)
		__field(int, cpu)
		__field(unsigned long, min_core_freq)
		__field(unsigned long, cpu_freq)
		__field(unsigned long, new_ddr_freq)
	),

	TP_fast_assign(
		__assign_str(name, name);
		__assign_str(reason, reason);
		__entry->cpu = cpu;
		__entry->min_core_freq = min_core_freq;
		__entry->cpu_freq = cpu_freq;
		__entry->new_ddr_freq = new_ddr_freq;
	),

	TP_printk("dev:%s reason=%s  cpu=%d min_core_freq=%lu cpu_freq=%lu new_ddr_freq=%lu",
		__get_str(name),
		__get_str(reason),
		__entry->cpu,
		__entry->min_core_freq,
		__entry->cpu_freq,
		__entry->new_ddr_freq)
);
#endif
#endif /* _TRACE_MEM_LAT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
