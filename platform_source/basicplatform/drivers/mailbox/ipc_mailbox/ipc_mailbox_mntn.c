/*
 *
 * mailbox driver maintain utils.
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/fs.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#ifdef CONFIG_DFX_BB
#include <platform_include/basicplatform/linux/rdr_ap_hook.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#endif
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/timekeeping.h>
#include <linux/sort.h>
#include <securec.h>

#include "ipc_rproc_id_mgr.h"
#include "ipc_mailbox.h"
#include "ipc_mailbox_event.h"
#include "ipc_mailbox_mntn.h"

/*lint -e580 */

#define MAX_DBG_LOG_TYPE 32
#define MAX_DBG_FUNC_NAME_LEN 64
#define MAX_RPROC_ID IPC_RPROC_MAX_MBX_ID
#define MAX_RPROC_ID_POOL_CNT ((IPC_RPROC_MAX_MBX_ID / 8) + 1)

#define IPC_MNTN_OPEN 0xE551
#define MAILBOX_DUMP_TIME 10000000000UL
#define CONTINUOUS_FAIL_CNT_MAX 50
#define BIT_CNT_OF_BYTE 8

#define MAX_EVENT_POOL_CNT ((MAX_IPC_EVENT_NUM / 8) + 1)

#define def_mntn_event(_event, _event_handle) \
	{ _event, _event_handle }

#define CONTINUOUS_FAIL_JUDGE \
	(likely(g_mbox_mntn_data.continuous_fail_cnt < CONTINUOUS_FAIL_CNT_MAX))

#define MAX_SEND_RECORD_CNT  5
#define MAX_ASYNC_SEND_RECORD_CNT  5
#define NS_TO_US   1000

#undef pr_fmt
#define pr_fmt(fmt) "[ap_ipc]:" fmt

#define mntn_pr_err(fmt, args...)   pr_err(fmt "\n", ##args)
#define mntn_pr_info(fmt, args...)  pr_info(fmt "\n", ##args)

enum ipc_send_exception_type {
	MBX_SYNC_SUCCESS,
	MBX_SYNC_TASKLET_JAM_EX,
	MBX_SYNC_ISR_JAM_EX,
	MBX_SYNC_ACK_LOST_EX,
	MBX_ASYNC_TASK_FIFO_FULL_EX,
	MAX_MBX_EXCEPTION_TYPE
};

struct ipc_mbox_rt_rec {
	unsigned int rec_idx;
	unsigned int async_rec_idx;
	char snd_rec_cnt;
	u64 sync_send_dump_time;
	u64 async_send_dump_time;
	u64 sync_send_start_time;
	u64 sync_send_slice_begin;

	unsigned int sync_send_spend_time[MAX_SEND_RECORD_CNT];
	u64 async_send_timestamps[MAX_ASYNC_SEND_RECORD_CNT];
	enum ipc_send_exception_type exception[MAX_SEND_RECORD_CNT];

	unsigned int ex_cnt[MAX_MBX_EXCEPTION_TYPE];
};

static char *g_ipc_send_exception_name[MAX_MBX_EXCEPTION_TYPE] = {
	[MBX_SYNC_SUCCESS] = "MBX_SYNC_SUCCESS",
	[MBX_SYNC_TASKLET_JAM_EX] = "MBX_SYNC_TASKLET_JAM_EX",
	[MBX_SYNC_ISR_JAM_EX] = "MBX_SYNC_ISR_JAM_EX",
	[MBX_SYNC_ACK_LOST_EX] = "MBX_SYNC_ACK_LOST_EX",
	[MBX_ASYNC_TASK_FIFO_FULL_EX] = "MBX_ASYNC_TASK_FIFO_FULL_EX"
};

struct ipc_mbox_mntn_data {
	struct list_head *all_mdevs;
	unsigned char rproc_trace_flag[MAX_RPROC_ID_POOL_CNT];
	unsigned char event_trace_flag[MAX_EVENT_POOL_CNT];
	unsigned int ipc_dump_flag;
	/*
	 * use the continuous_fail_cnt to
	 * control the Continuous ipc timeout times
	 * which may overflow the kmesg log
	 */
	int continuous_fail_cnt;

	int is_rec_sync_time;
	int is_rec_async_time;
	struct ipc_mbox_rt_rec rt_rec[MAX_RPROC_ID];
};

static struct ipc_mbox_mntn_data g_mbox_mntn_data;

static inline char *ipc_get_send_exception_name(unsigned int ex)
{
	return (ex >= MAX_MBX_EXCEPTION_TYPE) ? "UNKOWN_EXCEPTION" :
		g_ipc_send_exception_name[ex];
}

static inline int ipc_is_ipc_dump_open(void)
{
	return g_mbox_mntn_data.ipc_dump_flag == IPC_MNTN_OPEN;
}

static struct ipc_mbox_rt_rec *ipc_get_mbox_rt_rec(int rproc_id)
{
	struct ipc_mbox_mntn_data *dbg_cfg = &g_mbox_mntn_data;

	if (rproc_id < 0 || rproc_id >=  MAX_RPROC_ID)
		return NULL;

	return &dbg_cfg->rt_rec[rproc_id];
}

/*
 * Enable ipc timeout dump mntn
 * IPC_MNTN_OPEN 0xE551, enable ipc timeout dump mntn
 */
int ipc_open_ipc_timestamp_dump(void)
{
	g_mbox_mntn_data.ipc_dump_flag = IPC_MNTN_OPEN;
	mntn_pr_err("close ipc dump flag success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_open_ipc_timestamp_dump);

/*
 * Disable the ipc timeout dump mntn
 */
int ipc_close_ipc_timestamp_dump(void)
{
	g_mbox_mntn_data.ipc_dump_flag = 0;
	mntn_pr_err("open ipc dump flag success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_close_ipc_timestamp_dump);

void ipc_mntn_reset_continuous_fail_cnt(void)
{
	g_mbox_mntn_data.continuous_fail_cnt = 0;
}
EXPORT_SYMBOL(ipc_mntn_reset_continuous_fail_cnt);

static void ipc_build_event_msg(char *str_buf,
	unsigned int buf_size, const char *fmt_msg, va_list args)
{
	int ret;

	if (!fmt_msg || !fmt_msg[0]) {
		str_buf[0] = 0;
		return;
	}

	/*lint -e592 */
	ret = vsnprintf(str_buf, buf_size - 1, fmt_msg, args);
	if (ret < 0) {
		mntn_pr_err("build event msg failed");
		return;
	}
	/*lint +e592 */
}

#ifdef CONFIG_IPC_MAILBOX_DEBUGFS
static int is_rproc_id_trace(int rproc_id)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (rproc_id < 0 || rproc_id >= MAX_RPROC_ID)
		return 0;

	return dbg_log_cfg->rproc_trace_flag[rproc_id / BIT_CNT_OF_BYTE] &
		(1UL << (unsigned int)(rproc_id % BIT_CNT_OF_BYTE));
}

static int is_event_trace(unsigned int event)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (event >= MAX_IPC_EVENT_NUM)
		return 0;

	return dbg_log_cfg->event_trace_flag[event / BIT_CNT_OF_BYTE] &
		(1UL << (event % BIT_CNT_OF_BYTE));
}

static void ipc_record_sync_send_spend_time(struct ipc_mbox_device *mbox)
{
	static struct ipc_mbox_mntn_data *dbg_cfg = &g_mbox_mntn_data;
	struct ipc_mbox_rt_rec *rec = NULL;

	if (!dbg_cfg->is_rec_sync_time)
		return;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	rec->sync_send_spend_time[rec->rec_idx] =
		ktime_get_ns() - rec->sync_send_start_time;
	rec->rec_idx = (rec->rec_idx + 1) % MAX_SEND_RECORD_CNT;
	if (rec->snd_rec_cnt < MAX_SEND_RECORD_CNT)
		rec->snd_rec_cnt++;
}

static void ipc_show_mbox_last_sync_time(struct ipc_mbox_rt_rec *rec)
{
	int i;
	static struct ipc_mbox_mntn_data *dbg_cfg = &g_mbox_mntn_data;
	u64 sum_spend_time, tmp_spend_time;

	if (!dbg_cfg->is_rec_sync_time)
		return;

	mntn_pr_err("Follow Message (Last %d sync spend time)",
		rec->snd_rec_cnt);
	mntn_pr_err("------------------------------------------------");
	mntn_pr_err("No.   %s   %s",
		"spend_time(us)", "sync_result");
	for (i = 0, sum_spend_time = 0; i < rec->snd_rec_cnt; i++) {
		tmp_spend_time = rec->sync_send_spend_time[i] / NS_TO_US;
		sum_spend_time += tmp_spend_time;
		mntn_pr_err("%2d   %15llu   %s", i, tmp_spend_time,
			ipc_get_send_exception_name(rec->exception[i]));
	}

	if (rec->snd_rec_cnt > 0) {
		mntn_pr_err("------------------------------------------------");
		mntn_pr_err("Average sync send spend time is : %ld(us)",
			sum_spend_time / rec->snd_rec_cnt);
	}
}

static void ipc_show_exception_times(struct ipc_mbox_rt_rec *rec)
{
	int ex_type;

	for (ex_type = MBX_SYNC_TASKLET_JAM_EX;
		ex_type < MAX_MBX_EXCEPTION_TYPE; ex_type++)
		mntn_pr_err("%30s happended %u",
			ipc_get_send_exception_name(ex_type),
			rec->ex_cnt[ex_type]);
}

int ipc_show_rproc_rt_rec(int rproc_id)
{
	struct ipc_mbox_rt_rec *rec = ipc_get_mbox_rt_rec(rproc_id);

	if (!rec) {
		mntn_pr_err("invalid input param rproc_id:%d", rproc_id);
		return ER_MNTN_INVALID_RPROC_ID;
	}

	mntn_pr_err("------------------------------------------------");
	mntn_pr_err("rproc_name:%s, rproc_id:%d",
		ipc_get_rproc_name(rproc_id), rproc_id);
	ipc_show_exception_times(rec);
	ipc_show_mbox_last_sync_time(rec);
	mntn_pr_err("------------------------------------------------");

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_show_rproc_rt_rec);

static int async_send_timestamps_cmp(const void *a, const void *b)
{
	return *(unsigned int *)a - *(unsigned int *)b;
}

static void async_send_timestamps_swap(void *a, void *b, int size)
{
	unsigned int  t = *(unsigned int *)a;
	*(unsigned int *)a = *(unsigned int *)b;
	*(unsigned int *)b = t;
}

int ipc_show_rproc_async_send_rec(int rproc_id)
{
	int i;
	unsigned int tmp_time;
	unsigned int async_snd_rec_cnt = 0;
	unsigned int async_sum_time = 0;
	static struct ipc_mbox_mntn_data *dbg_cfg = &g_mbox_mntn_data;
	struct ipc_mbox_rt_rec *rec = ipc_get_mbox_rt_rec(rproc_id);

	if (!dbg_cfg->is_rec_async_time) {
		mntn_pr_err("is_rec_async_time is invalid");
		return MBX_MNTN_SUCCESS;
	}

	if (!rec) {
		mntn_pr_err("invalid input param rproc_id:%d", rproc_id);
		return ER_MNTN_INVALID_RPROC_ID;
	}

	sort(rec->async_send_timestamps, MAX_SEND_RECORD_CNT,
		sizeof(unsigned int), async_send_timestamps_cmp,
			async_send_timestamps_swap);

	mntn_pr_err("------------------------------------------------");
	mntn_pr_err("rproc_name:%s, rproc_id:%d",
		ipc_get_rproc_name(rproc_id), rproc_id);
	for(i = 0; i < MAX_SEND_RECORD_CNT; i++)
		mntn_pr_err("%2d   %15llu", i, rec->async_send_timestamps[i]);
	mntn_pr_err("------------------------------------------------");

	for(i = MAX_SEND_RECORD_CNT - 1; i > 0; i--) {
		if(!rec->async_send_timestamps[i] || !rec->async_send_timestamps[i - 1])
			break;

		tmp_time = rec->async_send_timestamps[i] -
			rec->async_send_timestamps[i - 1];
		async_sum_time += tmp_time;
		async_snd_rec_cnt++;
	}
	mntn_pr_err("async_snd_rec_cnt = %d", async_snd_rec_cnt);
	if(async_snd_rec_cnt)
		mntn_pr_err("Average async send spend time is : %u(ns)",
			async_sum_time / async_snd_rec_cnt);

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_show_rproc_async_send_rec);

int ipc_start_rec_send_time(void)
{
	g_mbox_mntn_data.is_rec_sync_time = 1;
	mntn_pr_err("start mailbox sync send spend time record success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_start_rec_send_time);

int ipc_stop_rec_send_time(void)
{
	g_mbox_mntn_data.is_rec_sync_time = 0;
	mntn_pr_err("stop mailbox sync send spend time record success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_stop_rec_send_time);

int ipc_start_rec_async_send_time(void)
{
	g_mbox_mntn_data.is_rec_async_time = 1;
	mntn_pr_err("start mailbox async send spend time record success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_start_rec_async_send_time);

int ipc_stop_rec_async_send_time(void)
{
	int i,ret;

	if (!g_mbox_mntn_data.is_rec_async_time) {
		mntn_pr_err("is_rec_async_time is invalid");
		return MBX_MNTN_SUCCESS;
	}

	g_mbox_mntn_data.is_rec_async_time = 0;

	for(i = 0; i < MAX_RPROC_ID; i++) {
		ret = memset_s(g_mbox_mntn_data.rt_rec[i].async_send_timestamps,
				sizeof(g_mbox_mntn_data.rt_rec[i].async_send_timestamps), 0,
					sizeof(g_mbox_mntn_data.rt_rec[i].async_send_timestamps));
		if(ret != EOK)
			mntn_pr_err("%s:memset_s failed.ret.%d proic_id %d\n",
				__func__, ret, i);

		g_mbox_mntn_data.rt_rec[i].async_rec_idx = 0;
	}

	mntn_pr_err("stop mailbox async send spend time record success");
	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_stop_rec_async_send_time);

int ipc_trace_rproc_id(int rproc_id)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (rproc_id < 0 || rproc_id >= MAX_RPROC_ID) {
		mntn_pr_err("invalid rproc_id %d\n", rproc_id);
		return ER_MNTN_INVALID_RPROC_ID;
	}

	dbg_log_cfg->rproc_trace_flag[rproc_id / BIT_CNT_OF_BYTE] |=
		(1UL << (unsigned int)(rproc_id % BIT_CNT_OF_BYTE));

	mntn_pr_err("trace rproc{%d, %s} success",
		rproc_id, ipc_get_rproc_name(rproc_id));

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_trace_rproc_id);

int ipc_untrace_rproc_id(int rproc_id)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (rproc_id < 0 || rproc_id >= MAX_RPROC_ID) {
		mntn_pr_err("invalid rproc_id %d\n", rproc_id);
		return ER_MNTN_INVALID_RPROC_ID;
	}

	dbg_log_cfg->rproc_trace_flag[rproc_id / BIT_CNT_OF_BYTE] &=
		(~(1UL << (unsigned int)(rproc_id % BIT_CNT_OF_BYTE)));

	mntn_pr_err("untrace rproc{%d, %s} success",
		rproc_id, ipc_get_rproc_name(rproc_id));

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_untrace_rproc_id);

int ipc_untrace_all_ipc_func(void)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;
	int ret = memset_s(dbg_log_cfg->rproc_trace_flag,
		sizeof(dbg_log_cfg->rproc_trace_flag),
		0, sizeof(dbg_log_cfg->rproc_trace_flag));
	if (ret != EOK) {
		mntn_pr_err("memset rproc_trace_flag failed");
		return ER_MNTN_UNKOWN_ERR;
	}

	mntn_pr_err("untrace all rproc success");

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_untrace_all_ipc_func);

int ipc_trace_event(unsigned int event)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (event >= MAX_IPC_EVENT_NUM) {
		mntn_pr_err("invalid event:%u", event);
		return ER_MNTN_INVALID_EVENT;
	}

	dbg_log_cfg->event_trace_flag[event / BIT_CNT_OF_BYTE] |=
		(1UL << (event % BIT_CNT_OF_BYTE));

	mntn_pr_err("trace event{%u, %s} success",
		event, ipc_get_event_name(event));

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_trace_event);

int ipc_untrace_event(unsigned int event)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;

	if (event >= MAX_IPC_EVENT_NUM) {
		mntn_pr_err("invalid event:%u", event);
		return ER_MNTN_INVALID_EVENT;
	}

	dbg_log_cfg->event_trace_flag[event / BIT_CNT_OF_BYTE] &=
		(~(1UL << (event % BIT_CNT_OF_BYTE)));

	mntn_pr_err("untrace event{%u, %s} success",
		event, ipc_get_event_name(event));

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_untrace_event);

int ipc_untrace_all_event(void)
{
	struct ipc_mbox_mntn_data *dbg_log_cfg = &g_mbox_mntn_data;
	int ret = memset_s(dbg_log_cfg->event_trace_flag,
		sizeof(dbg_log_cfg->event_trace_flag),
		0, sizeof(dbg_log_cfg->event_trace_flag));
	if (ret != EOK) {
		mntn_pr_err("memset event_trace_flag failed");
		return ER_MNTN_UNKOWN_ERR;
	}

	mntn_pr_err("untrace all event success");

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_untrace_all_event);

int ipc_show_all_mntn_event(void)
{
	int i;

	mntn_pr_err("------------------------------------------------");
	for (i = 0; i < MAX_IPC_EVENT_NUM; i++)
		mntn_pr_err("event %s value is %d", ipc_get_event_name(i), i);
	mntn_pr_err("------------------------------------------------");

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_show_all_mntn_event);

int ipc_mbox_show_all_mdev_info(void)
{
	struct list_head *list = g_mbox_mntn_data.all_mdevs;
	struct ipc_mbox_device *mbox = NULL;
	int idx = 0;

	if (!list) {
		mntn_pr_err("Not regist mdev list");
		return ER_MNTN_NOT_READY_MDEV_LIST;
	}

	list_for_each_entry(mbox, list, node) {
		if (!mbox)
			continue;

		mbox->ops->show_mdev_info(mbox, idx);
		idx++;
	}

	return MBX_MNTN_SUCCESS;
}
EXPORT_SYMBOL(ipc_mbox_show_all_mdev_info);

void ipc_mntn_register_mdevs(struct list_head *list)
{
	if (!list) {
		mntn_pr_err("%s:list is null", __func__);
		return;
	}

	g_mbox_mntn_data.all_mdevs = list;
}
EXPORT_SYMBOL(ipc_mntn_register_mdevs);

static void ipc_mbox_sync_end_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;
	(void)fmt_msg;
	(void)args;

	ipc_record_sync_send_spend_time(mbox);
}

static void ipc_show_event_happend_msg(int rproc_id,
	unsigned int event, const char *fmt_msg, va_list args)
{
	char event_msg[MAX_EVENT_MSG_LEN] = {0};

	ipc_build_event_msg(event_msg, MAX_EVENT_MSG_LEN, fmt_msg, args);
	mntn_pr_err("mbox:%s event %s has happended, msg:[%s]",
		ipc_get_rproc_name(rproc_id), ipc_get_event_name(event),
		event_msg);
}

static void ipc_mbox_event_currency_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	int rproc_id;

	rproc_id = mbox->ops->get_rproc_id(mbox);
	if (!is_rproc_id_trace(rproc_id) && !is_event_trace(event))
		return;

	ipc_show_event_happend_msg(rproc_id, event, fmt_msg, args);
}

#else
void ipc_mntn_register_mdevs(struct list_head *list)
{
}
EXPORT_SYMBOL(ipc_mntn_register_mdevs);
#endif

/***************************************************************************/
static void ipc_mbox_sync_begin_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	struct ipc_mbox_rt_rec *rec = NULL;

	(void)event;
	(void)fmt_msg;
	(void)args;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	rec->sync_send_start_time = ktime_get_ns();

	if (mbox->ops->get_slice &&
		mbox->ops->get_slice(mbox, &rec->sync_send_slice_begin))
		return;
}

static inline void ipc_pr_continuous_fail_msg(const char *msg)
{
	if (CONTINUOUS_FAIL_JUDGE)
		mntn_pr_err("%s", msg);
}

static void ipc_record_sync_fail_time(struct ipc_mbox_device *mbox)
{
	struct ipc_mbox_rt_rec *rec = NULL;

	if (!ipc_is_ipc_dump_open())
		return;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	if (ktime_get_ns() - rec->sync_send_dump_time > MAILBOX_DUMP_TIME) {
#ifdef CONFIG_DFX_BB
		rdr_system_error(MODID_AP_S_MAILBOX, 0, 0);
#endif
		rec->sync_send_dump_time = ktime_get_ns();
	}
}

static void ipc_record_sync_exception_type(struct ipc_mbox_device *mbox,
	enum ipc_send_exception_type e_type)
{
	struct ipc_mbox_rt_rec *rec = NULL;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	rec->exception[rec->rec_idx] = e_type;
	rec->ex_cnt[e_type]++;
}

static void ipc_mbox_sync_fail(struct ipc_mbox_device *mbox,
	 const char *msg, enum ipc_send_exception_type e_type)
{
	ipc_record_sync_exception_type(mbox, e_type);
	ipc_pr_continuous_fail_msg(msg);
	ipc_record_sync_fail_time(mbox);
}

static void ipc_mbox_sync_tasklet_jsm_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;
	(void)fmt_msg;
	(void)args;

	ipc_mbox_sync_fail(
		mbox, "reason:TASKLET jam", MBX_SYNC_TASKLET_JAM_EX);
}

static void ipc_mbox_sync_isr_jam_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;
	(void)fmt_msg;
	(void)args;

	ipc_mbox_sync_fail(mbox, "reason:ISR jam", MBX_SYNC_ISR_JAM_EX);
}

static void ipc_mbox_sync_ack_lost_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;
	(void)fmt_msg;
	(void)args;

	ipc_mbox_sync_fail(mbox, "reason:ACK lost", MBX_SYNC_ACK_LOST_EX);
}

static void ipc_mbox_show_timeout_dump(struct ipc_mbox_device *mbox,
	const char *fmt_msg, va_list args)
{
	char event_msg[MAX_EVENT_MSG_LEN] = {0};

	ipc_build_event_msg(event_msg, MAX_EVENT_MSG_LEN, fmt_msg, args);
	mntn_pr_err("%s", event_msg);
	if (mbox->ops->status)
		mbox->ops->status(mbox);
}

static void ipc_mbox_show_slice_time(struct ipc_mbox_device *mbox)
{
	struct ipc_mbox_rt_rec *rec = NULL;
	u64 slice_end = 0;
	u64 system_diff, diff;

	if (mbox->ops->get_slice && mbox->ops->get_slice(mbox, &slice_end))
		return;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	diff = ktime_get_ns() - rec->sync_send_start_time;
	system_diff = slice_end - rec->sync_send_slice_begin;
	mntn_pr_err("system_diff_time : %llu, mntn_diff_time : %llu\n",
		system_diff, diff);
}

static void ipc_mbox_sync_timeout_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;

	g_mbox_mntn_data.continuous_fail_cnt++;

	if (!CONTINUOUS_FAIL_JUDGE)
		return;

	ipc_mbox_show_timeout_dump(mbox, fmt_msg, args);
	ipc_mbox_show_slice_time(mbox);
}

static void ipc_mbox_async_in_queue_fail_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	struct ipc_mbox_rt_rec *rec = NULL;

	(void)fmt_msg;
	(void)args;

	ipc_record_sync_exception_type(mbox, MBX_ASYNC_TASK_FIFO_FULL_EX);

	if (!ipc_is_ipc_dump_open())
		return;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	if (ktime_get_ns() - rec->async_send_dump_time >
		MAILBOX_DUMP_TIME) {
#ifdef CONFIG_DFX_BB
		rdr_system_error(MODID_AP_S_MAILBOX, 0, 0);
#endif
		rec->async_send_dump_time = ktime_get_ns();
	}
}

static void ipc_mbox_sync_proc_record_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	(void)event;

	ipc_pr_continuous_fail_msg(fmt_msg);
}

static void ipc_mbox_async_send_time_eh(struct ipc_mbox_device *mbox,
	unsigned int event, const char *fmt_msg, va_list args)
{
	static struct ipc_mbox_mntn_data *dbg_cfg = &g_mbox_mntn_data;
	struct ipc_mbox_rt_rec *rec = NULL;

	(void)fmt_msg;
	(void)args;

	if (!dbg_cfg->is_rec_async_time)
		return;

	rec = ipc_get_mbox_rt_rec(mbox->ops->get_rproc_id(mbox));
	if (!rec)
		return;

	rec->async_send_timestamps[rec->async_rec_idx] = ktime_get_ns();
	rec->async_rec_idx = (rec->async_rec_idx + 1) % MAX_ASYNC_SEND_RECORD_CNT;
}

struct ipc_mbox_mntn_event_register {
	unsigned int event;
	void (*event_handle)(struct ipc_mbox_device *mbox,
		unsigned int event, const char *fmt_msg, va_list args);
};

static struct ipc_mbox_mntn_event_register g_mbox_mntn_event_handles[] = {
	def_mntn_event(EVENT_IPC_SYNC_SEND_BEGIN, ipc_mbox_sync_begin_eh),

	def_mntn_event(EVENT_IPC_SYNC_TASKLET_JAM,
		ipc_mbox_sync_tasklet_jsm_eh),
	def_mntn_event(EVENT_IPC_SYNC_ISR_JAM,
		ipc_mbox_sync_isr_jam_eh),
	def_mntn_event(EVENT_IPC_SYNC_ACK_LOST,
		ipc_mbox_sync_ack_lost_eh),

	def_mntn_event(EVENT_IPC_SYNC_SEND_TIMEOUT, ipc_mbox_sync_timeout_eh),
	def_mntn_event(EVENT_IPC_ASYNC_IN_QUEUE_FAIL,
		ipc_mbox_async_in_queue_fail_eh),
	def_mntn_event(EVENT_IPC_SYNC_PROC_RECORD,
		ipc_mbox_sync_proc_record_eh),

	def_mntn_event(EVENT_IPC_ASYNC_TASK_SEND,
		ipc_mbox_async_send_time_eh),
#ifdef CONFIG_IPC_MAILBOX_DEBUGFS
	def_mntn_event(EVENT_IPC_SYNC_SEND_END,
		ipc_mbox_sync_end_eh),
	def_mntn_event(EVENT_IPC_SYNC_SEND_BEGIN,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_SYNC_SEND_END,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_SYNC_SEND_TIMEOUT,
		ipc_mbox_event_currency_eh),

	def_mntn_event(EVENT_IPC_SYNC_TASKLET_JAM,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_SYNC_ISR_JAM,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_SYNC_ACK_LOST,
		ipc_mbox_event_currency_eh),

	def_mntn_event(EVENT_IPC_SYNC_RECV_ACK,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_ASYNC_IN_QUEUE_FAIL,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_BEFORE_IRQ_TO_MDEV,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_AFTER_IRQ_TO_MDEV,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_RECV_BH,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_SEND_BH,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_MBOX_COMPLETE_STATUS_CHANGE,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_ASYNC_TASK_IN_QUEUE,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_ASYNC_TASK_SEND,
		ipc_mbox_event_currency_eh),

	def_mntn_event(EVENT_IPC_MBX_STATUS_CHANGE,
		ipc_mbox_event_currency_eh),
	def_mntn_event(EVENT_IPC_DEACTIVATE_BEGIN,
		ipc_mbox_event_currency_eh),
#endif
};

static int __init ipc_mbox_mntn_init(void)
{
	unsigned int i;
	int ret;
	struct ipc_mbox_mntn_event_register *event_reg = NULL;

	for (i = 0; i < ARRAY_SIZE(g_mbox_mntn_event_handles); i++) {
		event_reg = &g_mbox_mntn_event_handles[i];
		ret = ipc_regist_mailbox_event(
			event_reg->event, event_reg->event_handle);
		if (ret) {
			mntn_pr_err("regist mbox_event:%u failed",
				event_reg->event);
			goto unregister;
		}
	}

	return MBX_MNTN_SUCCESS;

unregister:
	while (i > 0) {
		event_reg = &g_mbox_mntn_event_handles[i - 1];
		ipc_unregist_mailbox_event(
			event_reg->event, event_reg->event_handle);
		i--;
	}
	return ret;
}
module_init(ipc_mbox_mntn_init);

static void __exit ipc_mbox_mntn_exit(void)
{
	unsigned int i;
	struct ipc_mbox_mntn_event_register *event_reg = NULL;

	for (i = 0; i < ARRAY_SIZE(g_mbox_mntn_event_handles); i++) {
		event_reg = &g_mbox_mntn_event_handles[i];
		ipc_unregist_mailbox_event(
			event_reg->event, event_reg->event_handle);
	}
}
module_exit(ipc_mbox_mntn_exit);
/*lint +e580 */
