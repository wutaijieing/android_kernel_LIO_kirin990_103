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
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/sched/clock.h>
#include <linux/fs.h>
#include <linux/sched/task_stack.h>
#include <asm/stacktrace.h>
#include <asm/traps.h>
#include <linux/slab.h>
#include <linux/dirent.h>

#include "mdfx_utils.h"
#include "mdfx_dumper.h"
#include "mdfx_priv.h"
#include "../dpu/azalea/dpu_fb.h"

#define FREQ_AND_VOLTAGE_BUF_LEN 1024
#define BUF_SIZE 8192

static char *g_dump_buf = NULL;
static uint32_t g_dump_buf_len;

/*
 * Dump out the contents of some kernel memory nicely...
 */
static void mdfx_dump_mem(const char *lvl, const char *str,
			unsigned long bottom, unsigned long top, int64_t id)
{
	unsigned long first;
	mm_segment_t fs;
	int i;

	/*
	 * We need to switch to kernel mode so that we can use __get_user
	 * to safely read from kernel space.
	 */
	fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "%s%s(0x%016lx to 0x%016lx)\n", lvl, str, bottom, top);

	for (first = bottom & ~31; first < top; first += 32) {
		unsigned long p;
		char str[sizeof(" 12345678") * 8 + 1]; //lint !e578

		memset(str, ' ', sizeof(str));
		str[sizeof(str) - 1] = '\0';

		for (p = first, i = 0; i < (32 / 8) && (p < top); i++, p += 8) {
			if (p >= bottom && p < top) {
				unsigned long val;
				if (__get_user(val, (unsigned long *)(uintptr_t)p) == 0)
					sprintf(str + i * 17, " %016lx", val); //lint !e530
				else
					sprintf(str + i * 17, " ????????????????"); //lint !e585
			}
		}
		mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "%s%04lx:%s\n", lvl, first & 0xffff, str);
	}

	set_fs(fs);
}

static inline void mdfx_dump_backtrace_entry(unsigned long where, int64_t id)
{
	/*
	 * Note that 'where' can have a physical address, but it's not handled.
	 */
	mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "[<%p>] %pS\n", (void *)(uintptr_t)where, (void *)(uintptr_t)where);
}

static void mdfx_dump_backtrace(struct pt_regs *regs, struct task_struct *tsk, int64_t id)
{
	struct stackframe frame;
	unsigned long last_pc = 0;
	unsigned long count = 0;
	unsigned long stack;
	int ret;
	bool skip = false;

	if (!tsk)
		tsk = current;

	if (!try_get_task_stack(tsk))
		return;

	if (tsk == current) {
		frame.fp = (unsigned long)(uintptr_t)__builtin_frame_address(0);
		frame.pc = (unsigned long)(uintptr_t)dump_backtrace;
	} else {
		/*
		 * task blocked in __switch_to
		 */
		frame.fp = thread_saved_fp(tsk);
		frame.pc = thread_saved_pc(tsk);
	}

	mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "Call trace:\n");

	skip = !!regs;
	while (1) {
		/* skip until specified stack frame */
		if (!skip) {
			/* to solve the dump backtrace infinite loop problem */
			if (frame.pc == last_pc) {
				count++;
				if (count >= MAX_BACKTRACE_COUNT) {
					mdfx_pr_err("count is overflow");
					mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "(backtrace infinite loop:fp %p pc %p)\n",
							(void *)(uintptr_t)frame.fp, (void *)(uintptr_t)frame.pc);
					break;
				}
			} else {
				last_pc = frame.pc;
				count = 0;
			}

			mdfx_dump_backtrace_entry(frame.pc, id);
		} else if (frame.fp == regs->regs[29]) {
			skip = false;
			/*
			 * Mostly, this is the case where this function is
			 * called in panic/abort. As exception handler's
			 * stack frame does not contain the corresponding pc
			 * at which an exception has taken place, use regs->pc
			 * instead.
			 */
			mdfx_dump_backtrace_entry(regs->pc, id);
		}

		ret = unwind_frame(tsk, &frame);
		if (ret < 0) {
			mdfx_pr_err("ret=%d, count=%lu", ret, count);
			break;
		}

		if (in_entry_text(frame.pc)) {
			stack = frame.fp - offsetof(struct pt_regs, stackframe);
			if (on_accessible_stack(tsk, stack))
				mdfx_dump_mem("", "Exception stack", stack,
					 stack + sizeof(struct pt_regs), id);
		}
	}

	put_task_stack(tsk);
}

static void mdfx_dump_stack_print_info(struct task_struct *tsk, int64_t id)
{
	struct task_struct *task = NULL;
	int tmp;

	task = (tsk == NULL) ? current : tsk;
#ifdef CONFIG_SMP
#ifdef CONFIG_THREAD_INFO_IN_TASK
	tmp = task->cpu;
#else
	tmp = -1;
#endif
#else
	tmp = 0;
#endif

	mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "Comm: %.20s Current CPU: %d PID: %d\n", task->comm, tmp, task->pid);

	/*
	 * Some threads'name of android is the same in different process.
	 * So we need to get the tgid and the comm of thread's group_leader.
	 */
	mdfx_dump_msg(g_dump_buf, g_dump_buf_len, "TGID: %d Comm: %.20s\n",
		task->tgid, task->group_leader->comm);
}

static void mdfx_show_stack(struct task_struct *tsk, struct pt_regs *regs, int64_t id)
{
	mdfx_dump_stack_print_info(tsk, id);
	mdfx_dump_backtrace(regs, tsk, id);
	barrier();
}

/*
 * dfx_dump_stack - dump the current task information and its stack trace
 */
#ifdef CONFIG_SMP
static atomic_t mdfx_bt_dump_lock = ATOMIC_INIT(-1);
static void mdfx_dump_stack(struct task_struct *tsk, struct pt_regs *regs, int64_t id)
{
	int was_locked;
	int old;
	int cpu;

	/*
	 * Permit this cpu to perform nested stack dumps while serialising
	 * against other CPUs
	 */
retry:
	cpu = smp_processor_id();
	old = atomic_cmpxchg(&mdfx_bt_dump_lock, -1, cpu); //lint !e571

	if (old == -1) {
		was_locked = 0;
	} else if (old == cpu) {
		was_locked = 1;
	} else {
		cpu_relax();
		goto retry;
	}

	mdfx_show_stack(tsk, regs, id);

	if (!was_locked)
		atomic_set(&mdfx_bt_dump_lock, -1);
}
#else
static void mdfx_dump_stack(struct task_struct *tsk, struct pt_regs *regs, int64_t id)
{
	mdfx_show_stack(tsk, regs, id);
}
#endif

static DEFINE_SPINLOCK(show_stack_lock);
static void mdfx_show_runing_cpu_stack(void *dummy)
{
	struct task_struct *task = NULL;
	unsigned long flags;
	int64_t id = 0;

	if (idle_cpu(smp_processor_id()))
		return;

	spin_lock_irqsave(&show_stack_lock, flags);

	mdfx_pr_err("dump stack cpu:%d", smp_processor_id());
	show_stack(NULL, NULL);

	/* look up all runnable task, and then dump its backtrace */
	mdfx_for_each_process(task) {
		if (task != NULL && !task->state)
			mdfx_dump_stack(task, NULL, id);
	}

	mdfx_pr_info("dump_buf_len=%u", g_dump_buf_len);

	spin_unlock_irqrestore(&show_stack_lock, flags);
}

static void mdfx_dump_runnable_task(int64_t id, const char *event_name)
{
	char include_str[MAX_FILE_NAME_LEN / 2] = {0};
	int ret;

	if (IS_ERR_OR_NULL(event_name))
		return;

	if (IS_ERR_OR_NULL(g_dump_buf)) {
		g_dump_buf = kzalloc(MAX_BACKTRACE_LEN, GFP_KERNEL);
		if (IS_ERR_OR_NULL(g_dump_buf))
			return;

		g_dump_buf_len = 0;
	}

	smp_call_function(mdfx_show_runing_cpu_stack, NULL, 1);

	mdfx_pr_info("g_dump_buf_len=%u", g_dump_buf_len);
	if (event_name[0] != '\0')
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "event[%s]_id[%lld]_backtrace", event_name, id);
	else
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "%s_id[%lld]_backtrace", MDFX_NORMAL_NAME_PREFIX, id);
	if (ret > 0)
		mdfx_save_buffer_to_file(include_str, MODULE_NAME_DUMPER, g_dump_buf, g_dump_buf_len);

	kfree(g_dump_buf);
	g_dump_buf = NULL;
	g_dump_buf_len = 0;
}

static void mdfx_dump_freq_and_voltage(struct dpu_fb_data_type *dpufd, int64_t id, const char *event_name)
{
	char include_str[MAX_FILE_NAME_LEN / 2] = {0};
	char freq_and_voltage_buf[FREQ_AND_VOLTAGE_BUF_LEN] = {0};
	int ret;

	if (IS_ERR_OR_NULL(dpufd) || IS_ERR_OR_NULL(event_name)) {
		mdfx_pr_err("param is null");
		return;
	}
	ret = snprintf(freq_and_voltage_buf, FREQ_AND_VOLTAGE_BUF_LEN - 1, \
		"aclk_dss:[%llu]\n"\
		"pclk_dss:[%llu]\n"\
		"clk_edc0:[%llu]\n"\
		"clk_ldi0:[%llu]\n"\
		"clk_ldi1:[%llu]\n"\
		"clk_dss_axi_mm:[%llu]\n"\
		"pclk_mmbuf:[%llu]\n"\
		"clk_txdphy0_ref:[%llu]\n"\
		"clk_txdphy1_ref:[%llu]\n"\
		"clk_txdphy0_cfg:[%llu]\n"\
		"clk_txdphy1_cfg:[%llu]\n"\
		"pclk_dsi0:[%llu]\n"\
		"pclk_dsi1:[%llu]\n"\
		"pclk_pctrl:[%llu]\n"\
		"clk_dpctrl_16m:[%llu]\n"\
		"pclk_dpctrl:[%llu]\n"\
		"aclk_dpctrl:[%llu]\n",
		(uint64_t)clk_get_rate(dpufd->dss_axi_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_dss_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pri_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pxl0_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pxl1_clk),
		(uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_mmbuf_clk),
		(uint64_t)clk_get_rate(dpufd->dss_dphy0_ref_clk),
		(uint64_t)clk_get_rate(dpufd->dss_dphy1_ref_clk),
		(uint64_t)clk_get_rate(dpufd->dss_dphy0_cfg_clk),
		(uint64_t)clk_get_rate(dpufd->dss_dphy1_cfg_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_dsi0_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_dsi1_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_pctrl_clk),
		(uint64_t)clk_get_rate(dpufd->dss_auxclk_dpctrl_clk),
		(uint64_t)clk_get_rate(dpufd->dss_pclk_dpctrl_clk),
		(uint64_t)clk_get_rate(dpufd->dss_aclk_dpctrl_clk));
	if (ret < 0)
		return;

	if (event_name[0] != '\0')
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "event[%s]_id[%lld]_freq_and_voltage", event_name, id);
	else
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "%s_id[%lld]_freq_and_voltage", MDFX_NORMAL_NAME_PREFIX, id);
	if (ret > 0)
		mdfx_save_buffer_to_file(include_str, MODULE_NAME_DUMPER, freq_and_voltage_buf, strlen(freq_and_voltage_buf));
}

static void mdfx_dump_cmdlist(struct dpu_fb_data_type *dpufd, int64_t id, const char *event_name)
{
	uint32_t cmdlist_idxs_prev = 0;
	char include_str[MAX_FILE_NAME_LEN / 2] = {0};
	char filename[MAX_FILE_NAME_LEN] = {0};
	int ret;
	struct file *fd = NULL;
	mm_segment_t old_fs;

	if (IS_ERR_OR_NULL(dpufd) || IS_ERR_OR_NULL(event_name)) {
		mdfx_pr_err("param is null");
		return;
	}
	mdfx_pr_info("enter +");
	if (event_name[0] != '\0')
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "event[%s]_id[%lld]_cmdlist", event_name, id);
	else
		ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "%s_id[%lld]_cmdlist", MDFX_NORMAL_NAME_PREFIX, id);
	if (ret < 0)
		return;
	mdfx_file_get_name(filename, MODULE_NAME_DUMPER, include_str, MAX_FILE_NAME_LEN);
	filename[MAX_FILE_NAME_LEN - 1] = '\0';

	ret = mdfx_create_dir(filename);
	if (ret != 0) {
		mdfx_pr_err("create dir failed name=%s\n", filename);
		return;
	}

	fd = mdfx_file_open(filename);
	if (IS_ERR_OR_NULL(fd)) {
		mdfx_pr_err("filp_open returned:filename %s, error %ld\n",
			filename, PTR_ERR(fd));
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	(void)dpu_cmdlist_get_cmdlist_idxs(&(dpufd->ov_req_prev), &cmdlist_idxs_prev, NULL);
	dpu_cmdlist_dump_all_node_for_mdfx(dpufd, cmdlist_idxs_prev, fd);

	set_fs(old_fs);
	filp_close(fd, NULL);

	mdfx_pr_info("-");
}

/*
 * dumper dumps various of system info: frequency and voltage, cmdlist, cpu runnable, client callstack.
 * if the client gave callback interface, execute it.
 * dynamic allocated memory space that param data pointed, take care of releasing it.
 */
static void mdfx_dump_detail(void *data)
{
	dump_info_func dumper_cb = NULL;
	uint64_t visitor_types;
	uint32_t type_bit;
	int64_t visitor_id;
	uint64_t detail;
	struct mdfx_event_desc *desc = NULL;

	if (IS_ERR_OR_NULL(data)) {
		mdfx_pr_err("data is null");
		return;
	}
	desc = (struct mdfx_event_desc *)data;

	detail = mdfx_event_get_action_detail(desc, ACTOR_DUMPER);
	if (detail == 0) {
		kfree(desc->detail);
		kfree(desc);
		return;
	}
	mdfx_pr_info("detail=%llu", detail);

	if (ENABLE_BITS_FIELD(detail, DUMP_FREQ_AND_VOLTAGE)) {
		mdfx_dump_freq_and_voltage(dpufd_list[PRIMARY_PANEL_IDX], desc->id, desc->event_name);
		DISABLE_BITS_FIELD(detail, DUMP_FREQ_AND_VOLTAGE);
	}

	if (ENABLE_BITS_FIELD(detail, DUMP_CMDLIST)) {
		mdfx_dump_cmdlist(dpufd_list[PRIMARY_PANEL_IDX], desc->id, desc->event_name);
		DISABLE_BITS_FIELD(detail, DUMP_CMDLIST);
	}

	if (ENABLE_BITS_FIELD(detail, DUMP_CPU_RUNNABLE) || ENABLE_BITS_FIELD(detail, DUMP_CLIENT_CALLSTACK)) {
		mdfx_dump_runnable_task(desc->id, desc->event_name);
		DISABLE_BITS_FIELD(detail, DUMP_CPU_RUNNABLE);
		DISABLE_BITS_FIELD(detail, DUMP_CLIENT_CALLSTACK);
	}

	visitor_types = desc->relevance_visitor_types;
	for (type_bit = 0;  visitor_types != 0; type_bit++) {
		if (!ENABLE_BIT64(visitor_types, type_bit))
			continue;

		mdfx_pr_info("call mdfx_get_visitor_id, type:%llu", BIT64(type_bit));
		// Do other dump action, if need mdfx dumper do it.
		// at last, need call callback to dump visitor information
		visitor_id = mdfx_get_visitor_id(&g_mdfx_data->visitors, BIT64(type_bit));
		if (visitor_id != INVALID_VISITOR_ID) {
			dumper_cb = mdfx_visitor_get_dump_cb(&g_mdfx_data->visitors, visitor_id);
			if (dumper_cb)
				dumper_cb(visitor_id, detail);
		}

		DISABLE_BIT64(visitor_types, type_bit);
	}
	kfree(desc->detail);
	kfree(desc);
}

static void mdfx_dump_func(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc)
{
	struct mdfx_event_desc *desc_buf = NULL;

	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(desc))
		return;

	desc_buf = mdfx_copy_event_desc(desc);
	if (IS_ERR_OR_NULL(desc_buf))
		return;

	mdfx_saver_triggle_thread(&mdfx_data->dump_saving_thread, desc_buf, mdfx_dump_detail);
	mdfx_pr_info("triggle dump_saving_thread");
}

static int mdfx_dump_event_act(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc)
{
	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(desc))
		return -1;

	if (!MDFX_HAS_CAPABILITY(mdfx_data->mdfx_caps, MDFX_CAP_DUMPER))
		return 0;

	mdfx_pr_info("enter mdfx_dump_event_act");
	mdfx_dump_func(mdfx_data, desc);
	return 0;
}

static int mdfx_dump_sysinfo(struct mdfx_pri_data *data, const void __user *argp)
{
	struct mdfx_dump_desc dumper_desc;
	struct mdfx_event_desc event_desc = {0};
	uint64_t detail[ACTOR_MAX] = {0};

	if (IS_ERR_OR_NULL(data) || IS_ERR_OR_NULL(argp))
		return -1;

	if (!argp)
		return -1;

	if (copy_from_user(&dumper_desc, argp, sizeof(dumper_desc))) {
		mdfx_pr_err("dumper copy from user fail");
		return -1;
	}

	event_desc.id = dumper_desc.id;
	event_desc.relevance_visitor_types = mdfx_get_visitor_type(&data->visitors, dumper_desc.id);
	detail[ACTOR_DUMPER] = dumper_desc.dump_infos;
	event_desc.actions |= BIT64(ACTOR_DUMPER);
	event_desc.detail_count = ACTOR_MAX;
	event_desc.detail = &detail[0];
	mdfx_dump_func(data, &event_desc);
	return 0;
}

struct mdfx_actor_ops_t dumper_ops = {
	.act = mdfx_dump_event_act,
	.do_ioctl = mdfx_dump_sysinfo,
};

void mdfx_dumper_init_actor(struct mdfx_actor_t *actor)
{
	if (IS_ERR_OR_NULL(actor))
		return;

	actor->actor_type = ACTOR_DUMPER;
	actor->ops = &dumper_ops;
}

