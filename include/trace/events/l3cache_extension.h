/*
 * l3cache_extension.h
 *
 * L3Cache extension trace
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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
#define TRACE_SYSTEM l3cache_extension

#if !defined(_TRACE_L3CACHE_EXTENSION_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_L3CACHE_EXTENSION_H

#include <linux/tracepoint.h>

TRACE_EVENT(l3extension_switch,
	TP_PROTO(const char *on),
	TP_ARGS(on),
	TP_STRUCT__entry(
		__string(on, on)
	),
	TP_fast_assign(
		__assign_str(on, on);
	),

	TP_printk("switch = %s",
		  __get_str(on))
);

TRACE_EVENT(update_polling_info,
	TP_PROTO(unsigned long miss_bw, unsigned int lb_hit_rate, unsigned int l3_hit_rate,
		 unsigned long delta),
	TP_ARGS(miss_bw, lb_hit_rate, l3_hit_rate, delta),
	TP_STRUCT__entry(
		__field(unsigned long, miss_bw)
		__field(unsigned int, lb_hit_rate)
		__field(unsigned int, l3_hit_rate)
		__field(unsigned long, delta)
	),
	TP_fast_assign(
		__entry->miss_bw = miss_bw;
		__entry->lb_hit_rate = lb_hit_rate;
		__entry->l3_hit_rate = l3_hit_rate;
		__entry->delta = delta;
	),

	TP_printk("miss_bw = %lu lb_hit_rate = %u l3_hit_rate = %u delta = %lu us",
		  __entry->miss_bw, __entry->lb_hit_rate, __entry->l3_hit_rate, __entry->delta)
);

#endif /* _TRACE_L3CACHE_EXTENSION_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
