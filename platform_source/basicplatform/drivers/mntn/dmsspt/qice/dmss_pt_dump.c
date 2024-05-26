/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: initialization and save trace log of pattern trace
 * Create: 2021-10-28
 */

#define pr_fmt(fmt) "dmsspt: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/sched/clock.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/io.h>

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#endif

#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/spinlock.h>
#include <mntn_public_interface.h>
#include "dmss_pt_dump.h"
#include "dmss_pt_drv.h"
#include "rdr_inner.h"
#include "securec.h"

#define DATE_MAXLEN 14UL
#define DMSS_TRACE_NUM 2
#define TRACE0 0
#define TRACE1 1
static u8 *g_trace_buffer_vaddr;
static u8 *g_pt_rptr[DMSS_TRACE_MAX];
static int g_fs_init_flag;
static int g_is_tracing_dump_pt[DMSS_TRACE_MAX];
static u32 g_step;
static bool g_is_overflow;
static bool g_is_tracing_stop;
static struct pattern_trace_info g_pt_info;
static struct pattern_trace_stat g_pt_stat[DMSS_TRACE_NUM];
static struct semaphore *g_dump_pt_sem[DMSS_TRACE_MAX];
static struct file *pt_fp[DMSS_TRACE_MAX];
static char g_dmsspt_path[DMSS_TRACE_MAX][DMSSPT_PATH_MAXLEN];
static struct task_struct *dmsspt_main[DMSS_TRACE_MAX];
static int g_total_file_index;
static spinlock_t g_dmsspt_overflow_spinlock;
static spinlock_t g_dmsspt_stop_spinlock;
static spinlock_t g_dmsspt_total_file_spinlock;

#define PT_SEM_INIT(pt_num) \
	do { \
		static DEFINE_SEMAPHORE(dump_trace##pt_num##_sem); \
		sema_init(&dump_trace##pt_num##_sem, 0); \
		g_dump_pt_sem[pt_num] = &dump_trace##pt_num##_sem; \
	} while(0)

static DECLARE_COMPLETION(dmsspt_date);

struct pt_dumping_stat {
	u64 timestamp;
	int state; /* stop or tracing */
	int pt_state[DMSS_TRACE_MAX];
	int is_overflow;
};

static struct pt_dumping_stat dstat;
static struct dmsspt_region pt_region;

static void save_dynamic_alloc_area(phys_addr_t base, unsigned long len)
{
	struct dmsspt_region *reg = &pt_region;

	reg->base = base;
	reg->size = len;

	pr_err("REC_BUF:[0x%pK,0x%llx]\n", (void *)reg->base, reg->size);
}

static int dmsspt_reserve_area(struct reserved_mem *rmem)
{
	save_dynamic_alloc_area(rmem->base, rmem->size);

	return 0;
}

RESERVEDMEM_OF_DECLARE(dmss_pt, "dmsspt_trace_buffer", dmsspt_reserve_area);

static bool dmsspt_get_overflow_flag(void)
{
	bool overflow;

	spin_lock(&g_dmsspt_overflow_spinlock);
	overflow = g_is_overflow;
	spin_unlock(&g_dmsspt_overflow_spinlock);

	return overflow;
}

static void dmsspt_set_overflow_flag(bool is_overflow)
{
	spin_lock(&g_dmsspt_overflow_spinlock);
	g_is_overflow = is_overflow;
	spin_unlock(&g_dmsspt_overflow_spinlock);
}

static bool dmsspt_get_stop_flag(void)
{
	bool stop;

	spin_lock(&g_dmsspt_stop_spinlock);
	stop = g_is_tracing_stop;
	spin_unlock(&g_dmsspt_stop_spinlock);

	return stop;
}

static void dmsspt_set_stop_flag(bool is_stop)
{
	spin_lock(&g_dmsspt_stop_spinlock);
	g_is_tracing_stop = is_stop;
	spin_unlock(&g_dmsspt_stop_spinlock);
}

static int dmsspt_get_total_file_index(void)
{
	int index;

	spin_lock(&g_dmsspt_total_file_spinlock);
	index = g_total_file_index;
	spin_unlock(&g_dmsspt_total_file_spinlock);

	return index;
}

static void dmsspt_set_total_file_index(int index)
{
	spin_lock(&g_dmsspt_total_file_spinlock);
	g_total_file_index = index;
	spin_unlock(&g_dmsspt_total_file_spinlock);
}

static void dmsspt_increase_total_file_index(int num)
{
	spin_lock(&g_dmsspt_total_file_spinlock);
	g_total_file_index += num;
	spin_unlock(&g_dmsspt_total_file_spinlock);
}

void print_pt_buffer_info(void)
{
	pr_err("%s: phy base addr[0x%llx] !\n", __func__, pt_region.base);
	pr_err("%s: virt base addr[0x%llx] !\n", __func__, (u64)g_trace_buffer_vaddr);
	pr_err("%s: buffer size[0x%llx] !\n", __func__, pt_region.size);
	pr_err("%s: g_pt_rptr[0][0x%llx] !\n", __func__, (u64)g_pt_rptr[0]);
	pr_err("%s: g_pt_rptr[1][0x%llx] !\n", __func__, (u64)g_pt_rptr[1]);
	pr_err("%s: g_pt_rptr[2][0x%llx] !\n", __func__, (u64)g_pt_rptr[2]);
	pr_err("%s: g_pt_rptr[3][0x%llx] !\n", __func__, (u64)g_pt_rptr[3]);
	pr_err("%s: g_pt_info.pt_wptr[0][0x%llx] !\n", __func__, g_pt_info.pt_wptr[0]);
	pr_err("%s: g_pt_info.pt_wptr[1][0x%llx] !\n", __func__, g_pt_info.pt_wptr[1]);
	pr_err("%s: g_pt_info.pt_wptr[2][0x%llx] !\n", __func__, g_pt_info.pt_wptr[2]);
	pr_err("%s: g_pt_info.pt_wptr[3][0x%llx] !\n", __func__, g_pt_info.pt_wptr[3]);
}

int is_trace_overflow(u64 new_writep, u64 cur_writep, u64 readp)
{
	if (readp < cur_writep &&
	    (readp < new_writep && new_writep < cur_writep))
		goto oflw;

	if (readp > cur_writep &&
	    (new_writep < cur_writep || new_writep > readp))
		goto oflw;

	return 0;
oflw:
	pr_err("%s: new_writep [0x%llx], cur_writep [0x%llx], readp [0x%llx]!\n",
		__func__, new_writep, cur_writep, readp);
	return 1;
}

static char *dmsspt_get_timestamp(void)
{
	struct rtc_time tm;
	struct timeval tv;
	static char databuf[DATE_MAXLEN + 1];

	memset_s(databuf, sizeof(databuf), 0, DATE_MAXLEN + 1);

	memset_s(&tv, sizeof(struct timeval), 0, sizeof(struct timeval));
	memset_s(&tm, sizeof(struct rtc_time), 0, sizeof(struct rtc_time));
	do_gettimeofday(&tv);
	tv.tv_sec -= (__kernel_time_t)sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm((unsigned long)tv.tv_sec, &tm);

	(void)snprintf_s(databuf, DATE_MAXLEN + 1, DATE_MAXLEN, "%04d%02d%02d%02d%02d%02d",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);

	pr_err("%s: [%s] !\n", __func__, databuf);
	return databuf;
}

static u64 v2p(u64 vaddr)
{
	if (!vaddr || !(g_trace_buffer_vaddr) || !(pt_region.base))
		return 0;

	if (vaddr < (u64)g_trace_buffer_vaddr ||
	    vaddr > ((u64)g_trace_buffer_vaddr + pt_region.size))
		return 0;
	return ((vaddr - (u64)g_trace_buffer_vaddr) + pt_region.base);
}

static u64 p2v(u64 paddr)
{
	if (!(paddr) || !(g_trace_buffer_vaddr) || !(pt_region.base))
		return 0;

	if (paddr < pt_region.base ||
	    paddr > (pt_region.base + pt_region.size))
		return 0;

	return ((paddr -  pt_region.base) + (u64)g_trace_buffer_vaddr);
}

int dmsspt_is_finished(void)
{
	pr_info("%s: timestamp [%llx], state [%d].\n", __func__, dstat.timestamp, dstat.state);
	pr_info("%s: dstat.pt_state0 [%d] dstat.pt_state1 [%d] dstat.pt_state2 [%d] dstat.pt_state3 [%d].\n", __func__,
		dstat.pt_state[0], dstat.pt_state[1], dstat.pt_state[2], dstat.pt_state[3]);
	if (!dstat.timestamp)
		return 1;
	return dstat.state;
}

/*
 * Function: initialize parameters of dmsspt
 * Description: initialize the parameters of pattern trace
 * Input: pinfo: intlv gran
 * Return: 0: success -1: failure
 */
static int init_params_dmsspt_info(struct pattern_trace_info *pinfo, int num)
{
	u32 i;

	g_pt_info.intlv = pinfo->intlv;
	g_pt_info.dmi_num = pinfo->dmi_num;
	for (i = 0; i < g_pt_info.dmi_num; i++) {
		if (pinfo->pt_rptr[i])
			g_pt_rptr[i * 2 + num] = (u8 *)p2v(pinfo->pt_rptr[i]);
		else
			g_pt_rptr[i * 2 + num] = (u8 *)((u64)g_trace_buffer_vaddr +
					      ((u64)(i * 2 + num) * (u64)g_pt_info.intlv));

		if (!g_pt_rptr[i * 2 + num]) {
			pr_err("%s, g_pt_rptr[%u] is null!\n", __func__, i);
			return -1;
		}

		g_pt_info.pt_wptr[i * 2 + num] = p2v(pinfo->pt_wptr[i]);
		if (!g_pt_info.pt_wptr[i * 2 + num]) {
			pr_err("%s, g_pt_info.pt_wptr[%u] is null!\n", __func__, i);
			return -1;
		}
	}

	/* determine the read step size according to the interleaving mode */
	g_step = g_pt_info.intlv;
	pr_err("%s:g_step = %d.\n", __func__, g_step);

	return 0;
}

/*
 * Function: initialize parameters
 * Description: Initialize trace record parameters
 * Input: pinfo: pt related interleaving and buffer related information
	pstat: pt status information
 * Return: 0: success -1: failure
 */
static int init_params(struct pattern_trace_info *pinfo,
		       struct pattern_trace_stat *pstat, int num)
{
	u32 i;

	if (init_params_dmsspt_info(pinfo, num) != 0)
		return -1;
	dstat.timestamp = pstat->trace_begin_time;
	dstat.state = 0;
	g_total_file_index = 0;

	for (i = 0; i < g_pt_info.dmi_num; i++)
		dstat.pt_state[i * 2 + num] = 0;
	pr_err("%s: dstat.pt_state0 [%d] dstat.pt_state1 [%d] dstat.pt_state2 [%d] dstat.pt_state3 [%d].\n",
		__func__, dstat.pt_state[0], dstat.pt_state[1], dstat.pt_state[2], dstat.pt_state[3]);
	dstat.is_overflow = FALSE;
	pr_err("%s: initial pt params, begin time [%llu]!\n",
	       __func__, pstat->trace_begin_time);

	return 0;
}

static int check_wptr_range(struct pattern_trace_info *pinfo, int trace_num)
{
	if (trace_num >= (int)g_pt_info.dmi_num)
		return 0;

	if (pinfo->pt_wptr[trace_num] < pt_region.base ||
		pinfo->pt_wptr[trace_num] > (pt_region.base + pt_region.size)) {
		pr_err("%s: pinfo->pt_wptr[%d] 0x%llx is invalid!\n",
			__func__, trace_num, pinfo->pt_wptr[trace_num]);
		print_pt_buffer_info();
		return -1;
	}
	return 0;
}

static int dmsspt_fs_init(void)
{
	int ret;
	if (g_fs_init_flag == 0) {
		while (rdr_wait_partition(PATH_MNTN_PARTITION, 1000) != 0)
				;
		ret = rdr_create_dir(DMSSPT_ROOT);
		if (ret != 0) {
			pr_err("%s, create dir [%s] failed!\n", __func__, DMSSPT_ROOT);
			return ret;
		}
		g_fs_init_flag = DMSSPT_FS_INIT_MAGIC;
	}
	return 0;
}

/*
 * Function: check the parameters of dmsspt
 * Description: check the parameters
 * Input: pinfo: pt intlv info and buffer info pstat: pt status message
	is_tracing_stop: trace is or is not stop
 * Return: 0: success -1: failure 1: pattern trace is running
 */
static int dmsspt_check_params(struct pattern_trace_info *pinfo,
			       struct pattern_trace_stat *pstat, int num)
{
	int ret = 0;

	if (g_fs_init_flag == 0) {
		pr_err("%s, data file system hasn't been ready!\n", __func__);
		return -1;
	}

	if (IS_ERR_OR_NULL(pinfo) || IS_ERR_OR_NULL(pstat)) {
		pr_err("%s:pinfo or pstat is NULL.\n", __func__);
		return -1;
	}

	if (dstat.timestamp == pstat->trace_begin_time &&
	    dstat.is_overflow == TRUE) {
		pr_err("%s: pattern trace is overflow!\n", __func__);
		return -1;
	}

	if (DMSSPT_NOINIT_MAGIC == g_pt_info.intlv_mode) {
		ret = init_params(pinfo, pstat, num);
		if (ret != 0) {
			pr_err("%s:  init_params failed!\n", __func__);
			return ret;
		}
	}

	if (!dstat.state && (dstat.timestamp != pstat->trace_begin_time)) {
		pr_err("%s: last tracing [timestamp:%llu] hasn't been finished!\n",
			__func__, dstat.timestamp);
		return 1;
	}

	return ret;
}

/*
 * Function: save trace information of dmsspt
 * Description: Interface function that saves dmss pattern trace to file
 * Input: pinfo: pt related interleaving and buffer related information
	pstat: pt status information
 * Return: 0: success  -1: failure  1: last trace save is not over
 */
int dmsspt_save_trace(struct pattern_trace_info *pinfo,
		      struct pattern_trace_stat *pstat, bool is_tracing_stop, int num)
{
	int i;
	int ret = 0;

	if (dmsspt_check_params(pinfo, pstat, num) != 0) {
		for (i = 0; i < DMSS_TRACE_MAX; i++) {
			 /* forbid release the semaphore repeatedly */
			if (g_is_overflow == TRUE)
				up(g_dump_pt_sem[i]);
		}
		return -1;
	}

	g_pt_stat[num] = *pstat;

	for (i = 0; i < pinfo->dmi_num; i++) {
		if (check_wptr_range(pinfo, i))
			return -1;

		if (is_trace_overflow(p2v(pinfo->pt_wptr[i]),
				      g_pt_info.pt_wptr[i * 2 + num],
				      (u64)g_pt_rptr[i * 2 + num])) {
			pr_err("%s: DMSS pattern trace%d is overflow!\n",
			      __func__, i);
			pr_err("%s: new_writep[0x%llx], cur_writep[0x%llx], readp[0x%llx]",
			      __func__, p2v(pinfo->pt_wptr[i]),
			      g_pt_info.pt_wptr[i * 2 + num],
			      (u64)g_pt_rptr[i * 2 + num]);
			g_is_overflow = TRUE;
			dstat.is_overflow = TRUE;
			ret = -1;
			break;
		}

		/* update the current write pointer */
		if (i < (int)g_pt_info.dmi_num)
			g_pt_info.pt_wptr[i * 2 + num] = (u64)g_trace_buffer_vaddr + (pinfo->pt_wptr[i] - pt_region.base);

		/* release the semaphore */
		if (g_is_tracing_dump_pt[i * 2 + num] == 0) {
			up(g_dump_pt_sem[i * 2 + num]);
			pr_err("%s: DMSS pattern trace%d release the semaphore!\n",
			      __func__, i * 2 + num);
		}
	}

	g_is_tracing_stop = is_tracing_stop;

	return ret;
}

static void waiting_for_tracing_finished(int is_sync)
{
	/* wait for tracing finished */
	while ((dmsspt_is_finished() == 0) && (is_sync == 1))
		msleep(1000);
}

/*
 * func: initialize the trace function 's parameters
 * input: buf_old_size - pattern trace 's reseved space size
 */
static int test_dmsspt_dump_params_init(u64 buffer_size,
					struct pattern_trace_stat *stat,
					struct pattern_trace_info *info,
					u32 intlv, u32 intlv_mode,
					u64 *buf_old_size)
{
	int ret = 0;

	if (!pt_region.base) {
		pr_err("%s, invalid params\n", __func__);
		return -1;
	}

	if (buffer_size > pt_region.size)
		buffer_size = pt_region.size;

	memset_s(stat, sizeof(struct pattern_trace_stat),
		 0, sizeof(struct pattern_trace_stat));
	memset_s(info, sizeof(struct pattern_trace_stat),
		 0, sizeof(struct pattern_trace_info));
	*buf_old_size = pt_region.size;
	pt_region.size = buffer_size * SIZE_1KB * SIZE_1KB;
	memset_s(g_trace_buffer_vaddr, *buf_old_size, 0xff, *buf_old_size);
	memset_s(g_trace_buffer_vaddr, pt_region.size, 0x0, pt_region.size);

	stat->trace_begin_time = sched_clock();
	pr_err("%s, trace_begin_time [%llu]\n", __func__,
						stat->trace_begin_time);

	info->intlv = intlv;
	info->intlv_mode = intlv_mode;
	info->dmi_num = TRACE_MODE_MAX;
	info->pt_wptr[TRACE_MODE0] = (u64)pt_region.base;
	info->pt_wptr[TRACE_MODE1] = (u64)pt_region.base + info->intlv;
	info->pt_wptr[TRACE_MODE2] = (u64)pt_region.base + info->intlv;
	info->pt_wptr[TRACE_MODE3] = (u64)pt_region.base + info->intlv;
	info->pt_rptr[TRACE_MODE0] = 0;
	info->pt_rptr[TRACE_MODE1] = 0;
	info->pt_rptr[TRACE_MODE2] = 0;
	info->pt_rptr[TRACE_MODE3] = 0;

	return ret;
}

/*
 * func: set the data pattern of two trace
 * input: trace_no - the current trace number
 *	  *data - the current data pattern value
 *	  pt - the current stored virtual address
 */
static u8 test_dmsspt_dump_set_data_pattern(struct pattern_trace_info *info,
					     u8 trace_no, u8 *data, u8 *pt)
{
	u64 temp;

	if (info->pt_wptr[trace_no] < (u64)v2p((u64)pt))
		pt = g_trace_buffer_vaddr;

	while (info->pt_wptr[trace_no] > (u64)v2p((u64)pt)) {
		memset_s(pt, (u64)info->intlv, *data, (u64)info->intlv);
		memset_s(pt + pt_region.size/PT_REGION_PART_NUM,
			(u64)info->intlv,
			 *data,
			 (u64)info->intlv);
		temp = (u64)pt;
		temp += ((u64)info->intlv * PT_REGION_PART_NUM);
		pt = (u8 *)temp;
	}
	(*data)++;

	return *data;
}

/*
 * func: update the write pointer
 * input: tracing_stop - test trace 's current running/stop status
 *	  buffer_end - the reserved space's end physical address
 */
static int test_dmsspt_dump_adjust_write_ptr(struct pattern_trace_info *info,
					     bool tracing_stop,
					     u64 *record_data_size,
					     u64 data_velocity_second,
					     u8 *buffer_end)
{
	if (tracing_stop) {
		info->pt_wptr[TRACE_MODE0] = pt_region.base;
		info->pt_wptr[TRACE_MODE1] = pt_region.base + info->intlv;
		info->pt_wptr[TRACE_MODE2] = pt_region.base + info->intlv;
		info->pt_wptr[TRACE_MODE3] = pt_region.base + info->intlv;
		return 1;
	}
	msleep(1000);

	if (*record_data_size > data_velocity_second) {
		info->pt_wptr[TRACE_MODE0] += data_velocity_second *
						SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE1] += data_velocity_second
						* SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE2] += data_velocity_second *
						SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE3] += data_velocity_second
						* SIZE_1KB * SIZE_1KB;
		*record_data_size -= data_velocity_second;
	} else {
		info->pt_wptr[TRACE_MODE0] += (*record_data_size) *
						SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE1] += (*record_data_size) *
						SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE2] += (*record_data_size) *
						SIZE_1KB * SIZE_1KB;
		info->pt_wptr[TRACE_MODE3] += (*record_data_size) *
						SIZE_1KB * SIZE_1KB;
		*record_data_size = 0;
	}

	pr_err("%s, buffer_end[0x%llx], wptr0[0x%llx] wptr1[0x%llx], wptr2[0x%llx] wptr3[0x%llx]\n",
		__func__, (u64)buffer_end,
		info->pt_wptr[TRACE_MODE0], info->pt_wptr[TRACE_MODE1], info->pt_wptr[TRACE_MODE2], info->pt_wptr[TRACE_MODE3]);

	if (info->pt_wptr[TRACE_MODE0] >= (u64)buffer_end)
		info->pt_wptr[TRACE_MODE0] = (info->pt_wptr[TRACE_MODE0] -
					      (u64)buffer_end) +
					     (u64)pt_region.base;
	if (info->pt_wptr[TRACE_MODE1] >= (u64)buffer_end)
		info->pt_wptr[TRACE_MODE1] =
					      (info->pt_wptr[TRACE_MODE1] -
					       (u64)buffer_end) +
					      (u64)pt_region.base;
	if (info->pt_wptr[TRACE_MODE2] >= (u64)buffer_end)
		info->pt_wptr[TRACE_MODE2] =
					      (info->pt_wptr[TRACE_MODE2] -
					       (u64)buffer_end) +
					      (u64)pt_region.base;
	if (info->pt_wptr[TRACE_MODE3] >= (u64)buffer_end)
		info->pt_wptr[TRACE_MODE3] =
					      (info->pt_wptr[TRACE_MODE3] -
					       (u64)buffer_end) +
					      (u64)pt_region.base;
	g_pt_info.pt_wptr[TRACE_MODE0] = p2v(info->pt_wptr[TRACE_MODE0]);
	g_pt_info.pt_wptr[TRACE_MODE1] = p2v(info->pt_wptr[TRACE_MODE1]);
	g_pt_info.pt_wptr[TRACE_MODE2] = p2v(info->pt_wptr[TRACE_MODE2]);
	g_pt_info.pt_wptr[TRACE_MODE3] = p2v(info->pt_wptr[TRACE_MODE3]);

	return 0;
}

/*
 * func: test trace's store function, including large amounts of data,
 *	 buffer overflow, data validity and data integrity
 * input: buffer_size - trace buffer's size (less than dmsspt's reserved space
 *	 size), unit:MB
 *	  data_size - the amount of data (data_size equal to buffer_size * 2,
 *	 because with two trace)
 *	  write_velocity - the amount of data per second (less or more than
 *	 data_size is okay, recommend write_velocity equal to data_size)
 *	  intlv - interweaving granularity (integer multiple of 256 bytes,
 *	 recommend 256Bytes)
 *	  intlv_mode - interweaving mode (don's care, recommend value is 1)
 *	  is_sync - whether synchronously waiting until the file saved (1: waiting, 0: have saved)
 */
int test_dmsspt_dump_trace(u64 buffer_size, u64 data_size,
			   u64 write_velocity, u32 intlv,
			   u32 intlv_mode, int is_sync)
{
	int i;
	int ret;
	struct pattern_trace_stat pt_stat;
	struct pattern_trace_info pt_info;
	u8 *pt[TRACE_MODE_MAX];
	u8 data;
	bool is_tracing_stop = 0;
	u8 *pt_buffer_end;
	u64 buffer_old_size;

	ret = test_dmsspt_dump_params_init(buffer_size, &pt_stat,
					   &pt_info, intlv, intlv_mode,
					   &buffer_old_size);
	if (ret != 0)
		return ret;

	data = 0;
	pt_buffer_end = (u8 *)pt_region.base + pt_region.size;
	pr_err("%s, pt_buffer_end [0x%llx]\n", __func__, (u64)pt_buffer_end);
	pt[TRACE_MODE0] = (u8 *)p2v(pt_info.pt_wptr[TRACE_MODE0]);
	pt[TRACE_MODE1] = (u8 *)p2v(pt_info.pt_wptr[TRACE_MODE1]);
	pt[TRACE_MODE2] = (u8 *)p2v(pt_info.pt_wptr[TRACE_MODE2]);
	pt[TRACE_MODE3] = (u8 *)p2v(pt_info.pt_wptr[TRACE_MODE3]);

	while (1) {
		pr_err("%s, buffer_base[0x%llx], wptr0[0x%llx] wptr1[0x%llx]\n",
			__func__, pt_region.base,
			pt_info.pt_wptr[TRACE_MODE0],
			pt_info.pt_wptr[TRACE_MODE1],
			pt_info.pt_wptr[TRACE_MODE2],
			pt_info.pt_wptr[TRACE_MODE3]);

		if (!data_size) {
			is_tracing_stop = 1;
			pt_stat.trace_end_time = sched_clock();
			pr_err("%s, trace_end_time [%llu]\n", __func__,
							pt_stat.trace_end_time);
		}

		for (i = 0; i < TRACE_MODE_MAX; i++)
			data = test_dmsspt_dump_set_data_pattern(&pt_info, i,
								 &data, pt[i]);

		ret = dmsspt_save_trace(&pt_info, &pt_stat, is_tracing_stop, 0);
		if (ret != 0) {
			pr_err("%s:dmsspt_save_trace failed.\n", __func__);
			break;
		}

		ret = dmsspt_save_trace(&pt_info, &pt_stat, is_tracing_stop, 1);
		if (ret != 0) {
			pr_err("%s:dmsspt_save_trace failed.\n", __func__);
			break;
		}

		ret = test_dmsspt_dump_adjust_write_ptr(&pt_info,
							is_tracing_stop,
							&data_size,
							write_velocity,
							pt_buffer_end);
		if (ret == 1)
			break;
	}

	waiting_for_tracing_finished(is_sync);
	pt_region.size = buffer_old_size;
	return ret;
}

int dmsspt_save_trace_reentry_test(void)
{
	/* u64 buffer_size, u64 data_size, u64 write_velocity, u32 intlv, u32 intlv_mode, int is_sync */
	test_dmsspt_dump_trace(10UL, 20UL, 5UL, 256, 1, 0);
	return test_dmsspt_dump_trace(10UL, 20UL, 5UL, 256, 1, 0);
}

static int dmsspt_update_pt_header(int trace_num, struct pattern_trace_stat *stat)
{
	struct pattern_trace_header pt_header;
	struct file *trace_fp;
	ssize_t ret;

	pr_err("%s:begin.\n", __func__);
	trace_fp = filp_open(g_dmsspt_path[trace_num], O_RDWR, (umode_t)DMSSPT_FILE_LIMIT);
	if (IS_ERR_OR_NULL(trace_fp) || trace_fp == NULL) {
		pr_err("%s, open file [%s] failed!\n", __func__, g_dmsspt_path[trace_num]);
		return -1;
	}

	vfs_llseek(trace_fp, 0L, SEEK_SET);
	pr_err("%s: finish pt_fp[%d] fpos [0x%llx].\n", __func__, trace_num, trace_fp->f_pos);/*lint !e613 */
	pt_header.trace_begin_time = stat->trace_begin_time;
	pt_header.pad0 = 0;
	pt_header.trace_end_time = stat->trace_end_time;
	pt_header.pad1 = 0;
	pt_header.trace_cur_address = stat->trace_cur_address[trace_num];
	pt_header.trace_pattern_cnt = stat->trace_pattern_cnt[trace_num];
	pt_header.trace_roll_cnt = stat->trace_roll_cnt[trace_num];
	pt_header.trace_unaligned_mode = stat->trace_unaligned_mode;
	ret = vfs_write(trace_fp, (char *)&pt_header, sizeof(struct pattern_trace_header), &(trace_fp->f_pos));/*lint !e613 */
	if (sizeof(struct pattern_trace_header) != (u64)ret) {
		pr_err("%s:write file %s exception with ret %ld.\n",
		       __func__,  trace_fp->f_path.dentry->d_iname, ret);/*lint !e613 */
		filp_close(trace_fp, NULL);/*lint !e668 */
		return -1;
	}
	vfs_fsync(trace_fp, 0);
	filp_close(trace_fp, NULL);/*lint !e668 */
	pr_err("%s:end.\n", __func__);
	return 0;
}

static void dmsspt_handle_overlap(int trace_num)
{
	u64 delta;
	u64 remainder;

	delta = (u64)g_trace_buffer_vaddr + pt_region.size - (u64)g_pt_rptr[trace_num];
	remainder = delta % g_step;

	if (trace_num >= (int)DMSS_TRACE_MAX) {
		pr_err("%s:Bypass owerlap check, trace_num [%d].\n", __func__, trace_num);
		return;
	}

	if (!remainder && !delta) {
		g_pt_rptr[trace_num] = g_trace_buffer_vaddr + g_step;
		pr_err("%s:trace4 overlap has occured, trace_num [%d].\n", __func__, trace_num);
	} else if (!remainder && (delta == g_step)) {
		g_pt_rptr[trace_num] = g_trace_buffer_vaddr;
		pr_err("%s:trace0 overlap has occured, trace_num [%d].\n", __func__, trace_num);
	} else {
		if ((g_pt_rptr[trace_num] - g_trace_buffer_vaddr) % g_step == 0)
			g_pt_rptr[trace_num] += (DMSS_TRACE_MAX - 1) * g_step;
	}
}

static int dmsspt_save_file(struct file *trace_fp, int trace_num)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(trace_fp)) {
		pr_err("%s:trace_fp is null.\n", __func__);
		return -1;
	}

	if (!g_step) {
		pr_err("%s:g_step is invalid zero.\n", __func__);
		return -1;
	}

	pr_err("%s: g_pt_rptr [0x%llx] g_pt_info.pt_wptr [0x%llx]\n",
		__func__, (u64)g_pt_rptr[trace_num], g_pt_info.pt_wptr[trace_num]);
	while (g_pt_rptr[trace_num] != (u8 *)g_pt_info.pt_wptr[trace_num]) {
		if (dmsspt_get_overflow_flag() == TRUE)
			goto finish;

		/* When the file reaches the DMSSPT_FILE_MAX_LEN,
			the new file is stored because the file system has a file size limit */
		if (trace_fp->f_pos >= (loff_t)DMSSPT_FILE_MAX_LEN) {
			pr_err("%s:dmsspt file reached the size of 0x%lx.\n", __func__, DMSSPT_FILE_MAX_LEN);
			return 0;
		}

		if (g_pt_rptr[trace_num] > (g_trace_buffer_vaddr + pt_region.size)) {
			pr_err("%s: rptr[%llx] overstep.  g_trace_buffer_vaddr [%llx]\n", __func__,
				(u64)g_pt_rptr[trace_num], (u64)g_trace_buffer_vaddr);
			ret  = -1;
			goto fail;
		}

		ret = (int)vfs_write(trace_fp, (char *)g_pt_rptr[trace_num], (u64)DMSSPT_UNIT_SIZE, &(trace_fp->f_pos));
		if (ret != DMSSPT_UNIT_SIZE) {
			pr_err("%s:write file %s exception with ret %d fpos [0x%llx]!!!\n",
			       __func__, trace_fp->f_path.dentry->d_iname, ret, trace_fp->f_pos);
			goto fail;
		}

		g_pt_rptr[trace_num] += DMSSPT_UNIT_SIZE;
		dmsspt_handle_overlap(trace_num);
	}
	pr_err("%s: while loop done, g_pt_rptr=%llx\n", __func__, (u64)g_pt_rptr[trace_num]);

	if (dmsspt_get_stop_flag() == TRUE)
		goto finish;
	else
		return 0;

finish:
	if (trace_num == DMSS_TRACE0 || trace_num == DMSS_TRACE2)
		ret = dmsspt_update_pt_header(trace_num, &g_pt_stat[TRACE0]);
	else if (trace_num == DMSS_TRACE1 || trace_num == DMSS_TRACE3)
		ret = dmsspt_update_pt_header(trace_num, &g_pt_stat[TRACE1]);
	pr_err("%s: tracing is finished!\n", __func__);
fail:
	pr_err("%s:ret [%d]!\n", __func__, ret);
	return ret;
}

static struct file *dmsspt_open(int trace_num, int index)
{
	int flags;
	char path[DMSSPT_PATH_MAXLEN] = {'\0'};

	pr_err("%s: enter!\n", __func__);
	if (index == 0) {
		memset_s(g_dmsspt_path[trace_num], DMSSPT_PATH_MAXLEN - 1, 0, DMSSPT_PATH_MAXLEN - 1);
		g_dmsspt_path[trace_num][DMSSPT_PATH_MAXLEN - 1] = '\0';

		wait_for_completion(&dmsspt_date);
		(void)snprintf_s(g_dmsspt_path[trace_num], DMSSPT_PATH_MAXLEN, DMSSPT_PATH_MAXLEN - 1, "%s%d_%s",
			DMSSPT_ROOT"dmsspt", trace_num, dmsspt_get_timestamp());
		complete(&dmsspt_date);

		strncpy_s(path, DMSSPT_PATH_MAXLEN, g_dmsspt_path[trace_num], DMSSPT_PATH_MAXLEN - 1);
	} else {
		(void)snprintf_s(path, DMSSPT_PATH_MAXLEN, DMSSPT_PATH_MAXLEN - 1, "%s.%d", g_dmsspt_path[trace_num], index);
	}

	pr_err("%s:open file [%s]!\n", __func__, g_dmsspt_path[trace_num]);
	flags = O_CREAT | O_RDWR;
	return filp_open(path, flags, (umode_t)DMSSPT_FILE_LIMIT);
}

static int dmsspt_thread_data_check_init(void *data)
{
	int trace_num;
	int ret;

	if ((u64)data > 0) {
		trace_num = (int)((u64)data-(u64)DMSSPT_NUM_OFFSET);
		if (trace_num >= DMSS_TRACE_MAX || trace_num < 0) {
			pr_err("%s, trace_num [%d] is invalid!\n", __func__,
								   trace_num);
			return -1;
		}
	} else {
		pr_err("%s: para data must not be null.\n", __func__);
		return -1;
	}

	ret = dmsspt_fs_init();
	if (ret != 0)
		return ret;

	return trace_num;
}

static void dmsspt_finish(int trace_num)
{
	int finish = 1;
	int i;

	if (!IS_ERR_OR_NULL(pt_fp[trace_num])) {
		pr_err("%s: close pt fp.\n", __func__);
		vfs_fsync(pt_fp[trace_num], 0);
		filp_close(pt_fp[trace_num], NULL);

		pt_fp[trace_num] = NULL;
		dstat.pt_state[trace_num] = 1;
	}

	/* wait until the four threads are saved */
	for (i = 0; i < DMSS_TRACE_MAX; i++) {
		pr_err("%s: before while,trace_num [%d], pt_fp0 [%llx] pt_fp1 [%llx] pt_fp2 [%llx] pt_fp3 [%llx].\n",
			__func__, i, (u64)pt_fp[0], (u64)pt_fp[1], (u64)pt_fp[1], (u64)pt_fp[1]);
		pr_err("%s: trace_num [%d], dstat.pt_state0 [%d] dstat.pt_state1 [%d] dstat.pt_state2 [%d] dstat.pt_state3 [%d].\n",
			__func__, i, dstat.pt_state[0], dstat.pt_state[1], dstat.pt_state[2], dstat.pt_state[3]);
		if (dstat.pt_state[i] != 1) {
			pr_err("%s: wait for other threads finished,", __func__);
			pr_err("trace_num [%d], dstat.pt_state0 [%d] dstat.pt_state1 [%d] dstat.pt_state2 [%d] dstat.pt_state3 [%d].\n",
					i, dstat.pt_state[0], dstat.pt_state[1], dstat.pt_state[2], dstat.pt_state[3]);
			finish = 0;
			break;
		}
	}

	pr_err("%s: finish [%d].\n", __func__, finish);
	if (finish != 0) {
		dmsspt_set_stop_flag(FALSE);
		dmsspt_set_overflow_flag(FALSE);
		dstat.state = 1;
		g_pt_info.intlv = 0;
		g_pt_info.intlv_mode = DMSSPT_NOINIT_MAGIC;
		memset_s(&g_pt_stat[TRACE0], sizeof(struct pattern_trace_stat), 0, sizeof(struct pattern_trace_stat));
		memset_s(&g_pt_stat[TRACE1], sizeof(struct pattern_trace_stat), 0, sizeof(struct pattern_trace_stat));
		dmsspt_stop_store(1);
	}
}

/*
 * func: initialize trace file's function
 * input: trace_no - trace number
 *	  file_index - trace file number
 */
static struct file *dmsspt_file_init(long trace_no, int *file_index)
{
	if (IS_ERR_OR_NULL(pt_fp[trace_no])) {
		pt_fp[trace_no] = dmsspt_open((int)trace_no, *file_index);
		if (IS_ERR_OR_NULL(pt_fp[trace_no])) {
			pr_err("%s:create file %s err.\n", __func__,
						      g_dmsspt_path[trace_no]);
			return NULL;
		}

		(*file_index)++;
		if ((*file_index) > 1)
			pr_err("%s:file_index %d is too big.\n", __func__,
							 *file_index);

		vfs_llseek(pt_fp[trace_no],
			 sizeof(struct pattern_trace_header),
			 SEEK_SET);
	}

	return pt_fp[trace_no];
}

static void dmss_pattern_trace_dump_thread_stop_trace(void)
{
	dmsspt_set_stop_flag(TRUE);
	dmsspt_stop_store(1);
}

static int dmss_pattern_trace_dump_thread_init_handle(long trace_no,
						      int *file_index)
{
	unsigned long jiffies_priv;

	jiffies_priv = msecs_to_jiffies(DMSSPT_DUMP_TIMEOUT *
				 (u32)MSEC_PER_SEC);
	/* acquire the semaphore within a specified time(jiffies_priv) */
	if (down_timeout(g_dump_pt_sem[trace_no], (long)jiffies_priv) != 0) {
		if (dmsspt_get_stop_flag() == FALSE || pt_fp[trace_no] == 0)
			return SAVE_CONTINUE;
		pr_err("%s: trace_dump_thread%ld, g_is_tracing_stop[%d].\n",
				__func__, trace_no, g_is_tracing_stop);
	}

	/* the flag control the semaphore's release[up(semaphore)] */
	g_is_tracing_dump_pt[trace_no] = 1;
	if (dmsspt_get_overflow_flag() == TRUE) {
		dmsspt_set_overflow_flag(FALSE);
		return SAVE_FAIL;
	}

	if (dmsspt_file_init(trace_no, file_index) == NULL)
		return SAVE_CONTINUE;

	return 0;
}

static int dmss_pattern_trace_dump_create_file(int *file_index, long trace_no)
{
	/* Because the file system with file size limitation,
	   when the current file's size reach to DMSSPT_FILE_MAX_LEN,
	   and create one new file continue to record */
	if (pt_fp[trace_no]->f_pos >= (loff_t)DMSSPT_FILE_MAX_LEN &&
	    g_pt_rptr[trace_no] != (u8 *)g_pt_info.pt_wptr[trace_no]) {
		vfs_fsync(pt_fp[trace_no], 0);
		filp_close(pt_fp[trace_no], NULL);
		pt_fp[trace_no] = dmsspt_open((int)trace_no, *file_index);
		if (IS_ERR_OR_NULL(pt_fp[trace_no])) {
			pr_err("%s:create file %s err.\n", __func__,
						      g_dmsspt_path[trace_no]);
			return SAVE_CONTINUE;
		}
		(*file_index)++;
		return SAVE_AGAIN;
	}

	return 0;
}

static int dmss_pattern_trace_dump_thread(void *data)
{
	int ret;
	int index = 0;
	int trace_num;
	int again = 1;

	trace_num = dmsspt_thread_data_check_init(data);
	if (trace_num < TRACE_MODE0 || trace_num > TRACE_MODE3)
		return -1;

	while (kthread_should_stop() == 0) {
		ret = dmss_pattern_trace_dump_thread_init_handle(trace_num,
								 &index);
		if (ret == SAVE_CONTINUE) {
			continue;
		} else if (ret == SAVE_FAIL) {
			pr_err("%s[%d]: overflow.\n", __func__, trace_num);
			dmss_pattern_trace_dump_thread_stop_trace();
			goto save_fail;
		}

		while(again == 1) {
			if (dmsspt_get_total_file_index() >= TOTAL_FILE_INDEX_MAX) {
				pr_err("%s[%d]: file index reached to maxinum.\n",
					__func__, trace_num);
				dmsspt_set_total_file_index(0);
				dmss_pattern_trace_dump_thread_stop_trace();
				goto save_fail;
			}

			ret = dmsspt_save_file(pt_fp[trace_num], trace_num);
			if (ret) {
				pr_err("%s:dmsspt_save_file err.\n", __func__);
				dmss_pattern_trace_dump_thread_stop_trace();
				goto save_fail;
			}

			ret = dmss_pattern_trace_dump_create_file(&index, trace_num);
			if (ret == SAVE_CONTINUE) {
				again = 0;
			} else if (ret == SAVE_AGAIN) {
				dmsspt_increase_total_file_index(1);
				continue;
			}
		}
		if (again == 0) {
			again = 1;
			continue;
		}

save_fail:
		if (dmsspt_get_stop_flag() == TRUE || dmsspt_get_overflow_flag() == TRUE) {
			index = 0;
			dmsspt_finish(trace_num);
		}
		g_is_tracing_dump_pt[trace_num] = 0;
	}

	return 0;
}

struct dmsspt_region get_dmsspt_buffer_region(void)
{
	return pt_region;
}

static int __init dmsspt_init (void)
{
	long i, j;

	if (!check_mntn_switch(MNTN_DMSSPT)) {
		pr_err("%s, MNTN_DMSSPT is closed!\n", __func__);
		return 0;
	}

	if (!pt_region.base) {
		pr_err("%s, DMSSPT dynamic reserved memory is failed!\n", __func__);
		return 0;
	}

	g_trace_buffer_vaddr = (u8 *)ioremap_wc(pt_region.base, pt_region.size);
	if (!g_trace_buffer_vaddr) {
		pr_err("%s, g_trace_buffer_vaddr ioremap failed!\n", __func__);
		return -1;
	}
	pr_err("%s, g_trace_buffer_vaddr [0x%pK] buffer size [0x%llx]!\n",
			__func__, g_trace_buffer_vaddr, pt_region.size);

	spin_lock_init(&g_dmsspt_overflow_spinlock);
	spin_lock_init(&g_dmsspt_stop_spinlock);
	spin_lock_init(&g_dmsspt_total_file_spinlock);

	init_completion(&dmsspt_date);
	complete(&dmsspt_date);

	for (i = 0; i < DMSS_TRACE_MAX; i++) {
		PT_SEM_INIT(i);
		pr_err("%s, create thread dmsspt_dump_thread%ld.\n",
				__func__, i);
		dmsspt_main[i] = kthread_run(dmss_pattern_trace_dump_thread,
				(void *)(i + DMSSPT_NUM_OFFSET), "dmsspt_dthrd%ld", i);
		if (!dmsspt_main[i]) {
			pr_err("%s, create thread dmsspt_dump_thread failed.\n",
					__func__);
			for (j = 0; j < DMSS_TRACE_MAX; j++) {
				if (dmsspt_main[j])
					kthread_stop(dmsspt_main[j]);
			}
			return -1;
		}
	}
	g_pt_info.intlv_mode = DMSSPT_NOINIT_MAGIC;

	pr_err("%s, success!\n", __func__);
	return 0;
}

static void __exit dmsspt_exit(void)
{
	int i;

	for (i = 0; i < DMSS_TRACE_MAX; i++) {
		if (dmsspt_main[i])
			kthread_stop(dmsspt_main[i]);
	}
}

core_initcall(dmsspt_init);
module_exit(dmsspt_exit);

MODULE_LICENSE("GPL");