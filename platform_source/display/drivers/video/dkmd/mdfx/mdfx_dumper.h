/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef MDFX_DUMPER_H
#define MDFX_DUMPER_H

#include <linux/list.h>


#define MAX_BACKTRACE_COUNT 5
#define MAX_BACKTRACE_LEN 40960 // 40K

#define next_task_node(p) \
		list_entry(rcu_dereference((p)->tasks.next), struct task_struct, tasks)

#define mdfx_for_each_process(p) \
	for (p = &init_task ; (p = next_task_node(p)) != &init_task ; )


#define mdfx_dump_msg(buf, len, fmt, ...) \
	do { \
		if ((len) < MAX_BACKTRACE_LEN && buf) \
			len += scnprintf(buf + (len), MAX_BACKTRACE_LEN - (len), fmt, ##__VA_ARGS__); \
	} while (0)


#endif
