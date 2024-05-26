/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * lpcpu_pmu_dump.h
 *
 * lpcpu_pmu_dump Head File
 *
 * Copyright (c) 2017-2020 Huawei Technologies Co., Ltd.
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
#ifndef __LPCPU_PMU_DUMP_H__
#define __LPCPU_PMU_DUMP_H__

#include <linux/fs.h>
#include <linux/io.h>
#include <linux/hrtimer.h>
#include <linux/uaccess.h>

#define DEFAULT_BIND_CORE	3
#define DEFAULT_PERIOD_US	1000000

/* 100M */
#define PMU_LOG_SIZE		(100 * 1024 * 1024)
#define PMU_LOG_PATH		"/data/vendor/hwcdump/"
/*
 * The test result shows that the maximum length of each record is 718 bytes.
 * To facilitate calculation, 1024 bytes are used here.
 * Eight cores are recorded at the same time.
 * Each core has six count values, and each count value is 0xffffffff.
 */
#define PMU_LOG_DEADLINE	(PMU_LOG_SIZE - 1024)
#define TEMP_LEN_MAX		256
#define MAX_CORE_NUM		8
#define MAX_EVENT_NUM		6

#define CPU_DEBUG_SIZE		0x1000
#define CPU_PMU_SIZE		0x1000
#define UNLOCK_MAGIC		0xC5ACCE55
#define OS_UNLOCK		0x0

struct event_conf {
	int type_offset;
	int type;
	int count_offset;
};

struct st_cpu_conf {
	char __iomem *cpu_base_addr_phy;
	char __iomem *cpu_base_addr_virt;
	char __iomem *pmu_base_addr_phy;
	char __iomem *pmu_base_addr_virt;
	/* -E core_mask event1 ... */
	struct event_conf event_list[MAX_EVENT_NUM];

	/* Performance Monitors Count Enable Set Register */
	u32 count_enable_set_offset;

	/* Performance Monitors Cycle Count Registr */
	u32 cycle_count_l_offset;
	u32 cycle_count_h_offset;

	/* Performance Monitors Control Register */
	u32 pmcr_offset;

	/* Performance Monitors Lock Access Register */
	u32 pmlar_offset;

	/* External Debug Lock Access Register */
	u32 edlar_offset;

	/* OS Lock Access Register */
	u32 oslar_offset;

	int is_selected;
};

struct st_pmu_log {
	struct file *fd;
	char *mem_addr;
	int cur_pos;
};

struct st_pmu_dump {
	/* state:0-stop  1-run */
	int state;
	/* 0, need print file head: 1, The file header has been printed */
	int dump_head;
	/* -C core_num */
	int bind_cpu_id;
	/* -D delay_ms */
	int delay_time_ms;
	/* -T period_us */
	unsigned long period_us;
	/* Save Context */
	mm_segment_t oldfs;
	/* Timing correlation */
	struct hrtimer hr_timer;
	/* Saved log configuration */
	struct st_pmu_log pmu_log;
	struct st_cpu_conf cpu_conf[MAX_CORE_NUM];
};

#endif /* __LPCPU_PMU_DUMP_H__ */
