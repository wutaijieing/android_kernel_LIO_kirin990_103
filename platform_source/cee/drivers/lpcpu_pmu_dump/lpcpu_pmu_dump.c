/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * lpcpu_pmu_dump.c
 *
 * Lpcpu pmu dump tools
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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
#define pr_fmt(fmt)     "lpcpu_pmu_dump: " fmt

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/timekeeping.h>
#include <linux/printk.h>
#include <linux/debugfs.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <securec.h>
#include "lpcpu_pmu_dump.h"

#define STATIC_H static

#define outp32(addr, val) writel(val, addr)

static struct st_pmu_dump *g_pmu_dump;

STATIC_H void print_reg_info(struct st_pmu_dump *pmu_dump)
{
	int coreid;
	int i;

	for (coreid = 0; coreid < MAX_CORE_NUM; coreid++) {
		pr_err("cpu[%d] debug_base[%pK] pmu_base[%pK] "
		       "PMCNTENSET[0x%x] PMCCNTR_L_OFFSET[0x%x] "
		       "PMCCNTR_H_OFFSET[0x%x] PMCR[0x%x] PMLAR[0x%x] "
		       "EDLAR[0%x] OSLAR[0x%x]\n",
		       coreid, pmu_dump->cpu_conf[coreid].cpu_base_addr_phy,
		       pmu_dump->cpu_conf[coreid].pmu_base_addr_phy,
		       pmu_dump->cpu_conf[coreid].count_enable_set_offset,
		       pmu_dump->cpu_conf[coreid].cycle_count_l_offset,
		       pmu_dump->cpu_conf[coreid].cycle_count_h_offset,
		       pmu_dump->cpu_conf[coreid].pmcr_offset,
		       pmu_dump->cpu_conf[coreid].pmlar_offset,
		       pmu_dump->cpu_conf[coreid].edlar_offset,
		       pmu_dump->cpu_conf[coreid].oslar_offset);

		pr_err("event type offset:\n");
		for (i = 0; i < MAX_EVENT_NUM; i++)
			pr_err("    0x%x\n", pmu_dump->cpu_conf[coreid].event_list[i].type_offset);

		pr_err("event count offset:\n");
		for (i = 0; i < MAX_EVENT_NUM; i++)
			pr_err("    0x%x\n", pmu_dump->cpu_conf[coreid].event_list[i].count_offset);
	}
}

STATIC_H void check_return_value(int ret, const char *string)
{
	if (ret != 0)
		pr_err("%s", string);
}

STATIC_H int init_reg_from_dts(struct device_node *pmu_root_node,
			       struct st_pmu_dump *pmu_dump)
{
	struct device_node *np = NULL;
	u8 coreid = 0;
	struct device_node *debug_reg = NULL;
	struct device_node *pmu_reg = NULL;
	u32 eventtype[MAX_EVENT_NUM * 2] = {0};
	u32 eventcount[MAX_EVENT_NUM * 2] = {0};
	int i;
	int ret;
	unsigned int ret_all;

	for_each_child_of_node(pmu_root_node, np) {
		if (strncmp(np->name, "cpu", 3))  /* compare 3 chars len */
			continue;

		ret = of_property_read_u8(np, "id", &coreid);
		if (ret != 0 || coreid >= MAX_CORE_NUM) {
			pr_err("invalid core id defined!\n");
			goto error;
		}

		ret = of_property_read_u32(np, "debug_base",
					   (u32 *)&pmu_dump->cpu_conf[coreid].cpu_base_addr_phy);
		check_return_value(ret, "invalid debug_base defined!\n");
		ret_all = !!ret;

		ret = of_property_read_u32(np, "pmu_base",
					   (u32 *)&pmu_dump->cpu_conf[coreid].pmu_base_addr_phy);
		check_return_value(ret, "invalid pmu_base defined!\n");
		ret_all |= (unsigned int)!!ret;

		debug_reg = of_parse_phandle(np, "debug", 0);
		check_return_value(!debug_reg, "invalid debug node defined!\n");
		ret_all |= (unsigned int)!debug_reg;

		pmu_reg = of_parse_phandle(np, "pmu", 0);
		check_return_value(!pmu_reg, "invalid pmu node defined!");
		ret_all |= (unsigned int)!pmu_reg;

		ret = of_property_read_u32_array(pmu_reg, "PMEVTYPER0",
						 eventtype,
						 ARRAY_SIZE(eventtype));
		check_return_value(ret, "invalid PMEVTYPER0 offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32_array(pmu_reg, "PMEVCNTR0",
						 eventcount,
						 ARRAY_SIZE(eventcount));
		check_return_value(ret, "invalid PMEVCNTR0 offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(pmu_reg, "PMCNTENSET",
					   &pmu_dump->cpu_conf[coreid].count_enable_set_offset);
		check_return_value(ret, "invalid PMCNTENSET offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(pmu_reg, "PMCCNTR_L_OFFSET",
					   &pmu_dump->cpu_conf[coreid].cycle_count_l_offset);
		check_return_value(ret, "invalid PMCCNTR_L_OFFSET offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(pmu_reg, "PMCCNTR_H_OFFSET",
					   &pmu_dump->cpu_conf[coreid].cycle_count_h_offset);
		check_return_value(ret, "invalid PMCCNTR_H_OFFSET offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(pmu_reg, "PMCR",
					   &pmu_dump->cpu_conf[coreid].pmcr_offset);
		check_return_value(ret, "invalid PMCR offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(pmu_reg, "PMLAR",
					   &pmu_dump->cpu_conf[coreid].pmlar_offset);

		check_return_value(ret, "invalid PMLAR offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(debug_reg, "EDLAR",
					   &pmu_dump->cpu_conf[coreid].edlar_offset);
		check_return_value(ret, "invalid EDLAR offset defined!\n");
		ret_all |= (unsigned int)!!ret;

		ret = of_property_read_u32(debug_reg, "OSLAR",
					   &pmu_dump->cpu_conf[coreid].oslar_offset);
		check_return_value(ret, "invalid OSLAR offset defined!\n");
		ret_all |= (unsigned int)!!ret;
		if (ret_all != 0) {
			pr_err("dts read failed!\n");
			goto error;
		}

		for (i = 0; i < MAX_EVENT_NUM; i++) {
			/* ignore the size */
			int idx = i * 2;  /* 2 in one step */

			pmu_dump->cpu_conf[coreid].event_list[i].type_offset = (int)eventtype[idx];
		}

		for (i = 0; i < MAX_EVENT_NUM; i++) {
			/* ignore the size */
			int idx = i * 2;   /* 2 in one step */

			pmu_dump->cpu_conf[coreid].event_list[i].count_offset = (int)eventcount[idx];
		}
	}

	return 0;

error:

	return -EINVAL;
}

STATIC_H void pmu_reset_data(struct st_pmu_dump *pmu_dump)
{
	int i, j;

	pmu_dump->bind_cpu_id = DEFAULT_BIND_CORE;
	pmu_dump->period_us = DEFAULT_PERIOD_US;
	pmu_dump->delay_time_ms = 0;

	for (i = 0; i < MAX_CORE_NUM; i++) {
		pmu_dump->cpu_conf[i].is_selected = 0;

		for (j = 0; j < MAX_EVENT_NUM; j++)
			pmu_dump->cpu_conf[i].event_list[j].type = -1;
	}

	(void)memset_s((void *)&pmu_dump->pmu_log,
		       sizeof(pmu_dump->pmu_log), 0, sizeof(pmu_dump->pmu_log));
}

STATIC_H void bind_thread(int bind_cpu_id)
{
	pid_t curr_pid;

	curr_pid = __task_pid_nr_ns(current, PIDTYPE_PID, NULL);
	if (curr_pid > 0) {
		if (sched_setaffinity(curr_pid, cpumask_of(bind_cpu_id)))
			pr_err("sched_setaffinity failed!\n");
	}
}

STATIC_H void pmu_event_int(struct st_pmu_dump *pmu_dump)
{
	int index;
	int event_index;

	for (index = 0; index < MAX_CORE_NUM; index++) {
		if (pmu_dump->cpu_conf[index].is_selected != 1)
			continue;

		outp32(pmu_dump->cpu_conf[index].cpu_base_addr_virt +
		       pmu_dump->cpu_conf[index].edlar_offset,
		       UNLOCK_MAGIC);

		outp32(pmu_dump->cpu_conf[index].cpu_base_addr_virt +
		       pmu_dump->cpu_conf[index].oslar_offset,
		       OS_UNLOCK);

		outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
		       pmu_dump->cpu_conf[index].pmlar_offset,
		       UNLOCK_MAGIC);

		outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
		       pmu_dump->cpu_conf[index].pmcr_offset, 0x7);

		for (event_index = 0; event_index < MAX_EVENT_NUM; event_index++) {
			struct event_conf *conf =
				&pmu_dump->cpu_conf[index].event_list[event_index];

			if (conf->type != -1)
				outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
				       conf->type_offset,
				       conf->type);
		}

		outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
		       pmu_dump->cpu_conf[index].count_enable_set_offset,
		       0x8000003f);
	}
}

static void pmu_event_enable(struct st_pmu_dump *pmu_dump, int enable)
{
	int index;

	for (index = 0; index < MAX_CORE_NUM; index++) {
		if (pmu_dump->cpu_conf[index].is_selected == 1) {
			if (enable != 0)
				outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
				       pmu_dump->cpu_conf[index].pmcr_offset, 0x7);
			else
				outp32(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
				       pmu_dump->cpu_conf[index].pmcr_offset, 0x0);
		}
	}
}

STATIC_H void pmu_dump_head(struct st_pmu_dump *pmu_dump)
{
	int index;
	int event_index;
	int *p_cur_pos = &pmu_dump->pmu_log.cur_pos;
	char *mem_addr = pmu_dump->pmu_log.mem_addr;
	int ret;

	if (*p_cur_pos < 0 || *p_cur_pos >= PMU_LOG_SIZE)
		return;
	ret = snprintf_s(mem_addr + *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos - 1,
			 "time(s)\t\t");
	if (ret < 0) {
		pr_err("%s time snprintf_s failed\n", __func__);
		return;
	}
	*p_cur_pos += ret;
	if (*p_cur_pos < 0 || *p_cur_pos >= PMU_LOG_SIZE)
		return;
	for (index = 0; index < MAX_CORE_NUM; index++) {
		if (pmu_dump->cpu_conf[index].is_selected != 1)
			continue;

		ret = snprintf_s(mem_addr + *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos - 1,
				 "CPU%d_cycle_L\tCPU%d_cycle_H\t",
				 index, index);
		if (ret < 0) {
			pr_err("%s CPU cycle snprintf_s failed\n", __func__);
			return;
		}
		*p_cur_pos += ret;
		if (*p_cur_pos < 0 || *p_cur_pos >= PMU_LOG_SIZE)
			return;

		for (event_index = 0; event_index < MAX_EVENT_NUM; event_index++) {
			struct event_conf *conf =
				&pmu_dump->cpu_conf[index].event_list[event_index];

			if (conf->type == -1)
				continue;

			ret = snprintf_s(mem_addr + *p_cur_pos,
					 PMU_LOG_SIZE - *p_cur_pos,
					 PMU_LOG_SIZE - *p_cur_pos - 1,
					 "CPU%d(0x%02x)\t",
					 index, conf->type);
			if (ret < 0) {
				pr_err("%s CPU event snprintf_s failed\n", __func__);
				return;
			}

			*p_cur_pos += ret;
			if (*p_cur_pos < 0 || *p_cur_pos >= PMU_LOG_SIZE)
				return;
		}
	}

	ret = snprintf_s(mem_addr + *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos - 1,
			 "\n");
	if (ret < 0) {
		pr_err("%s newline snprintf_s failed\n", __func__);
		return;
	}
	*p_cur_pos += ret;
}

STATIC_H void pmu_event_dump(struct st_pmu_dump *pmu_dump)
{
	int index;
	int event_index;
	int *p_cur_pos = &pmu_dump->pmu_log.cur_pos;
	char *mem_addr = pmu_dump->pmu_log.mem_addr;
	unsigned long timestamp_ns;
	int ret;

	pmu_event_enable(pmu_dump, 0);

	if (pmu_dump->dump_head == 0) {
		pmu_dump_head(pmu_dump);
		pmu_dump->dump_head++;
	}

	if (*p_cur_pos < 0)
		return;

	if (*p_cur_pos > PMU_LOG_DEADLINE) {
		pmu_event_enable(pmu_dump, 1);
		return;
	}

	timestamp_ns = ktime_get_ns();
	ret = snprintf_s(mem_addr + *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos - 1,
			 "%d.%06d\t",
			 (int)(timestamp_ns / NSEC_PER_SEC),
			 (int)((timestamp_ns / NSEC_PER_USEC) % USEC_PER_SEC));
	if (ret < 0) {
		pr_err("%s timestamp snprintf_s failed\n", __func__);
		return;
	}
	*p_cur_pos += ret;
	if (*p_cur_pos >= PMU_LOG_SIZE)
		return;

	for (index = 0; index < MAX_CORE_NUM; index++) {
		if (pmu_dump->cpu_conf[index].is_selected != 1)
			continue;

		ret = snprintf_s(mem_addr + *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos - 1,
				 "%08u\t",
				 readl(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
				       pmu_dump->cpu_conf[index].cycle_count_l_offset));
		if (ret < 0) {
			pr_err("%s cycle_count_l snprintf_s failed\n", __func__);
			return;
		}
		*p_cur_pos += ret;
		if (*p_cur_pos >= PMU_LOG_SIZE)
			return;

		ret = snprintf_s(mem_addr + *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos,
				 PMU_LOG_SIZE - *p_cur_pos - 1,
				 "%08u\t",
				 readl(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
				       pmu_dump->cpu_conf[index].cycle_count_h_offset));
		if (ret < 0) {
			pr_err("%s cycle_count_h snprintf_s failed\n", __func__);
			return;
		}
		*p_cur_pos += ret;
		if (*p_cur_pos < 0 || *p_cur_pos >= PMU_LOG_SIZE)
			return;

		for (event_index = 0; event_index < MAX_EVENT_NUM; event_index++) {
			struct event_conf *conf =
				&pmu_dump->cpu_conf[index].event_list[event_index];

			if (conf->type == -1)
				continue;

			ret = snprintf_s(mem_addr + *p_cur_pos,
					 PMU_LOG_SIZE - *p_cur_pos,
					 PMU_LOG_SIZE - *p_cur_pos - 1,
					 "%08u\t",
					 readl(pmu_dump->cpu_conf[index].pmu_base_addr_virt +
					       conf->count_offset));
			if (ret < 0) {
				pr_err("%s event count snprintf_s failed\n", __func__);
				return;
			}

			*p_cur_pos += ret;
			if (*p_cur_pos >= PMU_LOG_SIZE)
				return;
		}
	}

	ret = snprintf_s(mem_addr + *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos,
			 PMU_LOG_SIZE - *p_cur_pos - 1,
			 "\n");
	if (ret < 0) {
		pr_err("%s newline snprintf_s failed\n", __func__);
		return;
	}
	*p_cur_pos += ret;

	pmu_event_enable(pmu_dump, 1);
}

STATIC_H enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	ktime_t ktime;
	struct st_pmu_dump *pmu_dump = container_of(timer, struct st_pmu_dump,
						    hr_timer);

	if (pmu_dump->state == 0)
		return HRTIMER_NORESTART;

	ktime = ktime_set(0, pmu_dump->period_us * 1000);  /* 1000us is 1ms */

	/* read reg, dump to file */
	pmu_event_dump(pmu_dump);

	/* set delay time */
	(void)hrtimer_forward(timer, hrtimer_cb_get_time(timer), ktime);

	return HRTIMER_RESTART;
}

STATIC_H void hrtimer_do_start(struct st_pmu_dump *pmu_dump)
{
	ktime_t ktime;

	/* Set the scheduled time */
	ktime = ktime_set(0, pmu_dump->period_us * 1000);  /* 1000us is 1ms */

	/* Initializing the hrtimer */
	hrtimer_init(&pmu_dump->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	/* set the callback function */
	pmu_dump->hr_timer.function = &hrtimer_callback;

	/* start hrtimer */
	hrtimer_start(&pmu_dump->hr_timer, ktime, HRTIMER_MODE_REL);
}

STATIC_H int pmu_do_start(struct st_pmu_dump *pmu_dump)
{
	char file_name[256] = {0};
	static int test_num;
	struct timeval hrtv;
	struct tm  time_readable;
	int i;
	int ret;

	if (pmu_dump->state == 1) {
		pr_err("pmu dump:start&end must be twin words!\n");
		return -1;
	}
	pmu_dump->state = 1;

	/* create a log file */
	do_gettimeofday(&hrtv);
	time_to_tm(hrtv.tv_sec, 0 - sys_tz.tz_minuteswest * 60, &time_readable);
	ret = snprintf_s(file_name, sizeof(file_name), sizeof(file_name) - 1,
			 PMU_LOG_PATH"%04ld_%02d%02d_%02d%02d%02d_%d.txt",
			 time_readable.tm_year + 1900, time_readable.tm_mon + 1,
			 time_readable.tm_mday, time_readable.tm_hour,
			 time_readable.tm_min,  time_readable.tm_sec,
			 test_num++);
	if (ret < 0) {
		pr_err("%s snprintf_s failed\n", __func__);
		return -1;
	}

	pmu_dump->pmu_log.fd = filp_open(file_name, O_CREAT | O_RDWR, 0644);
	if (pmu_dump->pmu_log.fd == NULL) {
		pr_err("filp_open err!\n");
		return -1;
	}

	/* Apply for memory and record pmu_log */
	if (pmu_dump->pmu_log.mem_addr == NULL)
		pmu_dump->pmu_log.mem_addr = vmalloc(PMU_LOG_SIZE);
	if (pmu_dump->pmu_log.mem_addr == NULL) {
		pr_err("vmalloc mem_addr err!\n");
		filp_close(pmu_dump->pmu_log.fd, NULL);
		return -1;
	}

	/* system start, clear memory */
	(void)memset_s(pmu_dump->pmu_log.mem_addr, PMU_LOG_SIZE, 0, PMU_LOG_SIZE);
	pmu_dump->dump_head = 0;
	pmu_dump->pmu_log.cur_pos = 0;

	if (pmu_dump->delay_time_ms)
		mdelay((unsigned long)(unsigned int)pmu_dump->delay_time_ms);

	bind_thread(pmu_dump->bind_cpu_id);

	/* create address mapping */
	for (i = 0; i < MAX_CORE_NUM; i++) {
		pr_err("%s: core %d selected = %d\n", __func__, i,
		       pmu_dump->cpu_conf[i].is_selected);
		if (pmu_dump->cpu_conf[i].is_selected == 1) {
			if (pmu_dump->cpu_conf[i].cpu_base_addr_virt == NULL)
				pmu_dump->cpu_conf[i].cpu_base_addr_virt =
					ioremap((uintptr_t)pmu_dump->cpu_conf[i].cpu_base_addr_phy,
						CPU_DEBUG_SIZE);

			if (pmu_dump->cpu_conf[i].cpu_base_addr_virt == NULL) {
				pr_err("cpu%d cpu base ioremap err!\n", i);
				return -1;
			}

			if (pmu_dump->cpu_conf[i].pmu_base_addr_virt == NULL)
				pmu_dump->cpu_conf[i].pmu_base_addr_virt =
					ioremap((uintptr_t)pmu_dump->cpu_conf[i].pmu_base_addr_phy,
						CPU_PMU_SIZE);

			if (pmu_dump->cpu_conf[i].pmu_base_addr_virt == NULL) {
				pr_err("cpu%d pmu base ioremap err!\n", i);
				return -1;
			}
		}
	}

	/* set pmu event */
	pmu_event_int(pmu_dump);

	/* Set the callback function, time, and start the clock. */
	hrtimer_do_start(pmu_dump);

	pmu_event_enable(pmu_dump, 1);

	return 0;
}

STATIC_H int pmu_do_end(struct st_pmu_dump *pmu_dump)
{
	loff_t pos = 0;
	ssize_t write_len;
	int *p_cur_pos = &pmu_dump->pmu_log.cur_pos;
	int i;

	/* stop state, free up memory */
	if (pmu_dump->state == 0) {
		pr_err("pmu dump:start&end must be twin words!\n");
		return -1;
	}

	/*
	 * wait the timer to finish
	 */
	hrtimer_cancel(&pmu_dump->hr_timer);

	pmu_event_enable(pmu_dump, 0);

	if ((pmu_dump->pmu_log.fd != NULL) && *p_cur_pos != 0) {
		pmu_dump->oldfs = get_fs(); /*lint !e501 */
		set_fs((mm_segment_t)(long)KERNEL_DS);   /*lint !e501 */
		write_len = vfs_write(pmu_dump->pmu_log.fd,
				      (char __user *)pmu_dump->pmu_log.mem_addr,
				      *p_cur_pos, &pos);

		filp_close(pmu_dump->pmu_log.fd, NULL);
		set_fs(pmu_dump->oldfs);
	}

	if (pmu_dump->pmu_log.mem_addr != NULL) {
		vfree(pmu_dump->pmu_log.mem_addr);
		pmu_dump->pmu_log.mem_addr = NULL;
	}

	for (i = 0; i < MAX_CORE_NUM; i++) {
		if (pmu_dump->cpu_conf[i].is_selected == 1) {
			if (pmu_dump->cpu_conf[i].cpu_base_addr_virt) {
				iounmap(pmu_dump->cpu_conf[i].cpu_base_addr_virt);
				pmu_dump->cpu_conf[i].cpu_base_addr_virt = NULL;
			}

			if (pmu_dump->cpu_conf[i].pmu_base_addr_virt) {
				iounmap(pmu_dump->cpu_conf[i].pmu_base_addr_virt);
				pmu_dump->cpu_conf[i].pmu_base_addr_virt = NULL;
			}
		}
	}

	pmu_reset_data(pmu_dump);
	pmu_dump->state = 0;

	return 0;
}

STATIC_H void print_usage(void)
{
	pr_err("Usage:===================================\n");
	pr_err("-C [core_id] ----- bind this thread to the given core (default cpu3)\n");
	pr_err("    eg: -C 3\n");
	pr_err("-T [period_us] ----- set sample period (default 1s)\n");
	pr_err("    eg: -T 100, 100us\n");
	pr_err("-R ----- reset all\n");
	pr_err("-1 ------  start dump\n");
	pr_err("-0 ------  end dump\n");
	pr_err("-D [delay_time_ms]  ----- set delay time (ms)\n");
	pr_err("    eg: -D 1000, 1s\n");
	pr_err("-E [core_id_mask] [event1] | [event2] | [event3]  ---- set  eventlist\n");
	pr_err("    eg: -E 0x30 0x1 | 0x2 | 0x3 | 0x4 | 0x5 | 0x6\n");
	pr_err("        set cpu4 and cpu5\n");
	pr_err("        eventlist  0x1 | 0x2 | 0x3 | 0x4 | 0x5 | 0x6, six event\n");
}

STATIC_H void print_info(struct st_pmu_dump *pmu_dump)
{
	int i;

	pr_info("+++++++++++++Begin+++++++++++++\n");
	pr_info("pmu_dump addr=%pK\n", pmu_dump);
	pr_info("dump_head=%d\n", pmu_dump->dump_head);
	pr_info("state=%d\n", pmu_dump->state);
	pr_info("bind_cpu_id=%d\n", pmu_dump->bind_cpu_id);
	pr_info("period_us=%ld\n", pmu_dump->period_us);
	pr_info("delay_time_ms=%d\n", pmu_dump->delay_time_ms);
	pr_info("pmu_log.fd=%pK\n", pmu_dump->pmu_log.fd);
	pr_info("pmu_log.cur_pos=%d\n", pmu_dump->pmu_log.cur_pos);
	pr_info("pmu_log.mem_addr=%pK\n", pmu_dump->pmu_log.mem_addr);

	for (i = 0; i < MAX_CORE_NUM; i++) {
		pr_info("cpu_conf[%d].cpu_base_addr_phy=%pK\n", i,
			pmu_dump->cpu_conf[i].cpu_base_addr_phy);
		pr_info("cpu_conf[%d].cpu_base_addr_virt=%pK\n", i,
			pmu_dump->cpu_conf[i].cpu_base_addr_virt);
		pr_info("cpu_conf[%d].pmu_base_addr_phy=%pK\n", i,
			pmu_dump->cpu_conf[i].pmu_base_addr_phy);
		pr_info("cpu_conf[%d].pmu_base_addr_virt=%pK\n", i,
			pmu_dump->cpu_conf[i].pmu_base_addr_virt);
		pr_info("cpu_conf[%d].is_selected=%d\n", i,
			pmu_dump->cpu_conf[i].is_selected);
		pr_info("cpu_conf[%d].event_list= %x %x %x %x %x %x\n",
			i,
			pmu_dump->cpu_conf[i].event_list[0].type,
			pmu_dump->cpu_conf[i].event_list[1].type,
			pmu_dump->cpu_conf[i].event_list[2].type,
			pmu_dump->cpu_conf[i].event_list[3].type,
			pmu_dump->cpu_conf[i].event_list[4].type,
			pmu_dump->cpu_conf[i].event_list[5].type);
	}

	pr_info("------------- End -------------\n");
}

STATIC_H int parse_eventlist(const char *buf, struct st_pmu_dump *pmu_dump)
{
	int cpu_id_mask;
	int event_num = 1;
	/* eventlist[0] = '0xXX' core mast */
	int eventlist[MAX_EVENT_NUM + 1] = {0};
	int i, j;
	const char cmd[256] = "%*s 0x%x 0x%x | 0x%x | 0x%x | 0x%x | 0x%x | 0x%x  ";

	for (i = 0; buf[i] != '\0'; i++) {
		if (buf[i] == '|')
			event_num++;
	}

	if (event_num > MAX_EVENT_NUM) {
		pr_err("%s invalid event_num = %d\n", __func__, event_num);
		return -1;
	}

	pr_err("%s cmd is %s\n", __func__, cmd);

	/*
	 * jump the "-E/-e"
	 */

	if (strlen(buf) < 2) {
		pr_err("Invalid buf %s\n", buf);
		return -1;
	}

	pr_err("%s buf = %s\n", __func__, buf);

	i = sscanf_s(buf, cmd, &eventlist[0],
		     &eventlist[1], &eventlist[2],
		     &eventlist[3], &eventlist[4],
		     &eventlist[5], &eventlist[6]);

	pr_err("%s i = %d\n", __func__, i);

	if (i <= 0) {
		pr_err("Invalid buf %s\n", buf);
		return -1;
	}

	if (i != (1 + event_num)) {
		pr_err("%s only parse %d args\n", __func__, i);
		return -1;
	}

	if (eventlist[0] < 0 || eventlist[0] >= (0x1 << MAX_CORE_NUM)) {
		pr_err("cpu id err. %d\n", eventlist[0]);
		return -1;
	}

	cpu_id_mask = eventlist[0];

	pr_err("%s cpu_id_mask = 0x%x\n", __func__, cpu_id_mask);

	for (i = 0; i < MAX_CORE_NUM; i++) {
		if ((unsigned int)cpu_id_mask & (0x1 << (unsigned int)i)) {
			pmu_dump->cpu_conf[i].is_selected = 1;

			for (j = 0; j < event_num; j++) {
				int idx = j + 1;

				pmu_dump->cpu_conf[i].event_list[j].type = eventlist[idx];
			}
		}
	}

	return 0;
}

static int check_core(const char *buf, struct st_pmu_dump *pmu_dump)
{
	char temp[TEMP_LEN_MAX] = {0};
	int temp_int[MAX_EVENT_NUM + 1] = {0};

	if (sscanf_s(buf, "%s %d", temp, TEMP_LEN_MAX, &temp_int[0]) != 2 ||
	    temp_int[0] < 0 || temp_int[0] >= MAX_CORE_NUM) {
		pr_err("%s invalid buf=%s, temp=%s, temp_int[0]=%d\n",
		       __func__, buf, temp, temp_int[0]);
		return -1;
	}

	pmu_dump->bind_cpu_id = temp_int[0];

	return 0;
}

static int check_time(const char *buf, struct st_pmu_dump *pmu_dump)
{
	char temp[TEMP_LEN_MAX] = {0};
	int temp_int[MAX_EVENT_NUM + 1] = {0};

	if (sscanf_s(buf, "%s %d", temp, TEMP_LEN_MAX, &temp_int[0]) != 2 ||
	    temp_int[0] <= 0) {
		pr_err("%s invalid buf=%s, temp=%s, temp_int[0]=%d\n",
		       __func__, buf, temp, temp_int[0]);
		return -1;
	}

	pmu_dump->period_us = (unsigned int)temp_int[0];

	return 0;
}

static int check_delay(const char *buf, struct st_pmu_dump *pmu_dump)
{
	char temp[TEMP_LEN_MAX] = {0};
	int temp_int[MAX_EVENT_NUM + 1] = {0};

	if (sscanf_s(buf, "%s %d", temp, TEMP_LEN_MAX, &temp_int[0]) != 2 ||
	    temp_int[0] <= 0) {
		pr_err("%s invalid buf=%s, temp=%s, temp_int[0]=%d\n",
		       __func__, buf, temp, temp_int[0]);
		return -1;
	}

	pmu_dump->delay_time_ms = temp_int[0];

	return 0;
}

static int hisi_pmu_dump_func(const char *buf, struct st_pmu_dump *pmu_dump)
{
	if (buf[1] == 'C' || buf[1] == 'c') {
		check_core(buf, pmu_dump);
		pr_info("set bind_cpu_id = %d\n", pmu_dump->bind_cpu_id);
	} else if (buf[1] == 'T' || buf[1] == 't') {
		check_time(buf, pmu_dump);
		pr_info("set period %ld us\n", pmu_dump->period_us);
	} else if (buf[1] == 'D' || buf[1] == 'd') {
		check_delay(buf, pmu_dump);
		pr_info("set delay_time %d ms\n", pmu_dump->delay_time_ms);
	} else if (buf[1] == 'E' || buf[1] == 'e') {
		if (parse_eventlist(buf, pmu_dump))
			goto ERROR;
	} else if (buf[1] == 'R' || buf[1] == 'r') {
		pmu_reset_data(pmu_dump);
	} else if (buf[1] == 'h') {
		print_info(pmu_dump);
	} else if (buf[1] == 'i') {
		print_reg_info(pmu_dump);
	} else {
		goto ERROR;
	}

	return 0;
ERROR:
	return -1;
}

static ssize_t hisi_pmu_dump_write(struct file *fp, const char __user *ch,
				   size_t count, loff_t *ppos)
{
	int ret = 0;
	char buf[1024];
	struct st_pmu_dump *pmu_dump = (struct st_pmu_dump *)fp->private_data;

	if (pmu_dump == NULL) {
		pr_err("pmu_dump is null\n");
		return -EFAULT;
	}

	if (ch == NULL) {
		pr_err("ch is null\n");
		return -EFAULT;
	}

	if (count >= sizeof(buf)) {
		pr_err("count err\n");
		return -EACCES;
	}

	(void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
	if (copy_from_user(buf, ch, min_t(size_t, sizeof(buf) - 1, count))) {
		pr_err("copy_from_user err\n");
		return -EACCES;
	}

	if (buf[0] != '-')
		goto ERROR;

	if (buf[1] == '1') {
		if (pmu_do_start(pmu_dump))
			return -EACCES;
	} else if (buf[1] == '0') {
		pmu_do_end(pmu_dump);
	} else {
		ret = hisi_pmu_dump_func(buf, pmu_dump);
	}

	if (ret != 0)
		goto ERROR;
	return count;

ERROR:
	pr_err("error cmd: %s\n", buf);
	print_usage();
	return -EACCES;
}

static int hisi_pmu_dump_open(struct inode *node, struct file *fp)
{
	fp->private_data = node->i_private;
	return 0;
}

static ssize_t hisi_pmu_dump_read(struct file *fp, char __user *ch,
				  size_t count, loff_t *ppos)
{
	return 0;
}

static const struct file_operations fileops = {
	.owner = THIS_MODULE,
	.open = hisi_pmu_dump_open,
	.read = hisi_pmu_dump_read,
	.write = hisi_pmu_dump_write,
};

static int __init hisi_pmu_dump_init(void)
{
	struct dentry *rootdir = NULL;
	struct device_node *dts_node = NULL;

	g_pmu_dump = kzalloc(sizeof(struct st_pmu_dump), GFP_KERNEL);

	if (g_pmu_dump == NULL) {
		pr_err("alloc pmu_dump fail!\n");
		return -ENOMEM;
	}

	g_pmu_dump->state = 0;
	g_pmu_dump->dump_head = 0;
	g_pmu_dump->oldfs = 0;
	g_pmu_dump->pmu_log.mem_addr = NULL;
	g_pmu_dump->pmu_log.fd = NULL;
	g_pmu_dump->pmu_log.cur_pos = 0;

	dts_node = of_find_node_by_name(NULL, "hisi_pmu_dump");
	if (dts_node != NULL) {
		/*
		 * read the register address from
		 * the dts.
		 */
		if (init_reg_from_dts(dts_node, g_pmu_dump)) {
			pr_err("fail to init reg from dts!!\n");
			goto error;
		}
	} else {
		pr_err("fail to find hisi_pmu_dump node!\n");
		goto error;
	}

	pmu_reset_data(g_pmu_dump);
	rootdir = debugfs_create_dir("hisi_pmu_dump", NULL);
	debugfs_create_file("state", 0660, rootdir, g_pmu_dump, &fileops);

	pr_info("init okay\n");
	return 0;

error:
	kfree(g_pmu_dump);
	g_pmu_dump = NULL;
	pr_err("init failed\n");
	return -EINVAL;
}

static void __exit hisi_pmu_dump_exit(void)
{
	if (g_pmu_dump != NULL) {
		kfree(g_pmu_dump);
		g_pmu_dump = NULL;
	}
}

module_init(hisi_pmu_dump_init);
module_exit(hisi_pmu_dump_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Lpcpu Pmu Dump Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
