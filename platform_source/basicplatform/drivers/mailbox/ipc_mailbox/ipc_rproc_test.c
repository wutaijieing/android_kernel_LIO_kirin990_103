/*
 * ipc_rproc_test.c
 *
 * ipc rproc test interface.
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/arm-smccc.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/version.h>
#include <soc_acpu_baseaddr_interface.h>
#include <securec.h>
#include "ipc_mailbox_dev.h"
#include "ipc_rproc_id_mgr.h"

#define PR_LOG_TAG AP_MAILBOX_TAG

#define FAST_MBOX (1 << 0)
#define COMM_MBOX (1 << 1)
#define SOURCE_MBOX (1 << 2)
#define DESTINATION_MBOX (1 << 3)

/* total pressure times once */
#define PRESSURE_TIMES 500
#define PERFORMANC_TIMES 100
/* show only 5 middle time cost */
#define PERFORMANC_SHOW_NUM 5
#define PERFORMANC_SHOW_INDEX (PERFORMANC_TIMES / PERFORMANC_SHOW_NUM)
/* the standard time cost, if longer, failed  */
#define PERFORMANC_FAIL_NSEC 2000000
#define SYNC_SEND 1
#define ASYNC_SEND 2

#define test_rproc_get_slice() 0xFFFFFFFF
#define TEST_RPROC_NULL ((void *)0)
#define MAX_TASK_LIMIT 11
#define TX_MAX_BUFFER 8

#define test_rproc_slice_diff(s, e)                                            \
	(((e) >= (s)) ? ((e) - (s)) : (0xFFFFFFFF - (s) + (e)))

#define ACK_BUFFER 0xFFFFFFFB
#define TASK_MAX_NAME_LEN 32

#define _MSG_ENTRY_LEN 28

#if SOC_ACPU_TIMER10_BASE_ADDR
#define TIMER_REG_BASE SOC_ACPU_TIMER10_BASE_ADDR
#else
#define TIMER_REG_BASE 0
#endif
#define TIMER_SIZE 0x1000
#define timerx_load(base, num) ((base) + (num * 0x20) + 0x0)
#define timerx_ctrl(base, num) ((base) + (num * 0x20) + 0x8)
#define timerx_ris(base, num) ((base) + (num * 0x20) + 0x10)
#define TIMER_ONESHOT_MODE 0xA1
#define TIMER_VALUE 0xFFFF
#define IPC_TEST_BUF_DATA ((OBJ_TEST << 16) | (CMD_TEST << 8))
#define RPROC_NUMBER 16
#define MAX_IPC_DEVICES_CNT 6
#define IPC_RESIDUE 100
#define MAILBOX_NO_USED 0
#define MAILBOX_USED 1
#define TS_CP_MSG_LEN 8

#define INVALID_CPU_BIT  0xFFFFFFFF
#define INVALID_RPROC_ID 0xFFFFFFFF
#define MAX_MAILBOX_CNT  100
#define CMDTYPE_TS_TEST_MSG 126
#define SYNC_SEND_ASYNC_INTERVAL 1000
#define SEND_TYPE_CNT  2
#define LPMCU_TEST_PRINT_TIMES 10000
#define IPC_CMD_PARAM_START_IDX 2
#define NS_TO_S 1000000000
#define TS_START_IDX 4
#define TS_END_IDX 23
#define WAIT_MBX_RECV_TIME 300
#define IPC_MDEV_FUNC 3

#define IPC_READL           0xAABB0003
#define IPC_WRITEL          0xAABB0004

enum ipc_type {
	PERI_NS_NORMAL_IPC = 0,
	AO_NS_IPC = 200,
	NPU_NS_IPC = 300,
	PERI_NS_CFG_IPC = 400
};

/* IPC Test case frame work */
enum ipc_test_mode {
	PERI_IPC_SYNC_TEST = 1,
	PERI_IPC_ASYNC_TEST = 2,
	AO_IPC_SYNC_TEST = 3,
	AO_IPC_ASYNC_TEST = 4,
	SINGLE_IPC_SYNC_TEST = 5,
	SINGLE_IPC_ASYNC_TEST = 6,
	MAILBOX_LPM_PRESS_TEST = 7,
	TEST_ALL_SEND_TO_LPMCU = 8,
	PERI_IPC_RECV_TEST = 9,
	AO_IPC_RECV_TEST = 10,
	SINGLE_IPC_ASYNC_TEST_WITH_ACK = 11,
	MAX_IPC_TEST_MODE_TYPE = 0xFF
};

enum ipc_sync_type {
	IPC_SYNC = 0,
	IPC_ASYNC
};

typedef int (*ipc_test_case_func)(int obj, int interval, int threads,
	union ipc_data *ipc_msg);

#define def_ipc_test_case(_case_name) \
	static int tc_##_case_name(int obj, int interval, int threads,\
		union ipc_data *ipc_msg)

/*lint -e773 */
#define add_ipc_test_case(_case_emu, _case_name) \
	[_case_emu] = tc_##_case_name
/*lint +e773 */

enum multi_task_sync_id {
	SYNC_AS_SYNC = 0,
	ASYNC_AS_SYNC,
	SYNC_AS_ASYNC,
	ASYNC_AS_ASYNC,
	MAX_SYNC_TYPE
};

struct test_rproc_cb {
	void *done_sema;
	void *sync_sema;
	unsigned int start_slice; /* for calculate boundwidth */
	unsigned int end_slice;   /* for calculate boundwidth */
	unsigned int msg_count;
	unsigned int sync_task_cnt;
	unsigned int done_task_cnt;
	unsigned int task_num;
	int check_ret;
};

typedef int (*multi_task_test_case_func)(
	unsigned char rp_id, unsigned int msg_len,
	int msg_num, struct test_rproc_cb *rproc_cb, rproc_msg_t *tx_buffer);

typedef int (*test_rproc_send_func)(unsigned int sync_id, unsigned char rp_id,
	unsigned int msg_len, int msg_num, struct test_rproc_cb *rproc_cb);

struct test_rproc_proc {
	test_rproc_send_func proc_cb;
	unsigned int sync_id;
	unsigned char rp_id;
	unsigned int msg_len;
	int msg_num;
	unsigned int task_num;
	struct test_rproc_cb *rproc_cb;
};

struct _tagts_msg_header_t {
	uint32_t msg_type : 1;  /* ts_msg_type */
	uint32_t cmd_type : 7;  /* ts_cmd_type */
	uint32_t sync_type : 1; /* ts_synctype */
	uint32_t res : 1;
	uint32_t msg_lenth : 14;
	uint32_t msg_index : 8;
} _ts_msg_header_t;

struct _tagipc_message {
	struct _tagts_msg_header_t msg_header;
	uint8_t ipc_data[_MSG_ENTRY_LEN];
};

struct mailbox_device {
	u8 func;
	int rproc_id;
	int used;
	unsigned int mbox_channel;
	const char *mbx_name;
	unsigned int src;
	unsigned int dest;
};

struct test_ipc_device {
	void __iomem *base;
	unsigned int ipc_type;
	unsigned int unlock;
	const char *ipc_name;
	const char *rproc_names[RPROC_NUMBER];
	unsigned int rproc_names_cnt;
	int mbx_cnt;
	unsigned int ipc_version;
	unsigned int lpm_cpu_bit;
	struct mailbox_device mboxes[MAX_MAILBOX_CNT];
	int vm_flag;
	unsigned int addr_phy;
};

struct test_ipc_device_group {
	struct test_ipc_device ipc_devs[MAX_IPC_DEVICES_CNT];
	int ipc_dev_cnt;
};

struct test_ipc_device_group g_test_ipc_devices;

static const char g_data_buf[] = "hello, ipc message";
static struct semaphore send_mutex_sema;
static struct semaphore task_mutex_sema;
static struct notifier_block npu_nb;

int interval_v;
int obj_mailbox;

static void __ipc_writel(struct test_ipc_device *idev, unsigned int val, unsigned int offset)
{
#ifndef CONFIG_LIBLINUX
	struct arm_smccc_res res;
#endif

	if(idev->vm_flag == 0) {
		__raw_writel(val, idev->base + offset);
		return;
	}

	/* trap to host */
#ifndef CONFIG_LIBLINUX
	arm_smccc_hvc(IPC_WRITEL, idev->addr_phy, offset, val, 0, 0, 0, 0, &res);
	if (res.a0)
		pr_err("%s error val %u", __func__, val);
#endif
}

static unsigned int __ipc_readl(struct test_ipc_device *idev, unsigned int offset)
{
#ifndef CONFIG_LIBLINUX
	struct arm_smccc_res res;
#endif

	if(!idev->vm_flag)
		return __raw_readl(idev->base + offset);

	/* trap to host */
#ifndef CONFIG_LIBLINUX
	arm_smccc_hvc(IPC_READL, idev->addr_phy, offset, 0, 0, 0, 0, 0, &res);
	return res.a0;
#endif
	return 0;
}

static unsigned int g_per_rproc_id_table[] = {
	IPC_ACPU_LPM3_MBX_1, IPC_ACPU_HIFI_MBX_1, IPC_ACPU_IOM3_MBX_1,
	IPC_ACPU_ISP_MBX_1, IPC_AO_ACPU_IOM3_MBX1, IPC_ACPU_IVP_MBX_1
};

static struct test_ipc_device *find_ipc_dev_by_type(unsigned int ipc_type)
{
	int idx;
	struct test_ipc_device *ipc_dev = NULL;

	for (idx = 0; idx < g_test_ipc_devices.ipc_dev_cnt; idx++) {
		ipc_dev = &g_test_ipc_devices.ipc_devs[idx];
		if (ipc_dev->ipc_type == ipc_type)
			return ipc_dev;
	}

	return NULL;
}

static struct mailbox_device *find_ipc_mbox_dev(
	struct test_ipc_device *ipc_dev, int rproc_id)
{
	int idx;
	struct mailbox_device *mbox = NULL;

	for (idx  = 0; idx < ipc_dev->mbx_cnt; idx++) {
		mbox = &ipc_dev->mboxes[idx];
		if (mbox->rproc_id == rproc_id && mbox->used == MAILBOX_USED)
			return mbox;
	}

	return NULL;
}

static int test_convert_to_rproc_id_by_ipc_type(int ipc_type, int mbox_channel)
{
	struct test_ipc_device *ipc_dev =
		find_ipc_dev_by_type((unsigned int)ipc_type);
	struct mailbox_device *mbox = NULL;
	int idx;

	if (!ipc_dev) {
		pr_err("xxxxxxxx %s:invalid ipc_type:%d xxxxxxxx\n",
			__func__, ipc_type);
		return -1;
	}

	for (idx = 0; idx < ipc_dev->mbx_cnt; idx++) {
		mbox = &ipc_dev->mboxes[idx];
		if (!mbox)
			continue;

		if (mbox->mbox_channel == mbox_channel)
			return mbox->rproc_id;
	}
	pr_err("xxxxxxxx Do not find mbox_id:%d xxxxxxxx\n", mbox_channel);

	return -1;
}

static int test_convert_to_rproc_id(int mbox_id)
{
	int ipc_type = (mbox_id / IPC_RESIDUE) * IPC_RESIDUE;

	return test_convert_to_rproc_id_by_ipc_type(
		ipc_type, mbox_id % IPC_RESIDUE);
}

static int test_rproc_send_sync_msg_sync(
	unsigned char rp_id, unsigned int msg_len,
	int msg_num, struct test_rproc_cb *rproc_cb, rproc_msg_t *tx_buffer)
{
	rproc_msg_t ack_buffer[TX_MAX_BUFFER] = {0};
	int ret;

	while (msg_num--) {
		ret = RPROC_SYNC_SEND(rp_id, tx_buffer, msg_len,
			ack_buffer, msg_len);
		/* obj_test shift left 16 bits,cmd_test shift left 8 bits */
		if (ret || IPC_TEST_BUF_DATA != ack_buffer[0] ||
			(ack_buffer[1] != ACK_BUFFER)) {
			pr_err("mbox:%d sync_snd error ret:%d ack[0x%x,0x%x]\r\n",
				RPROC_GET_MAILBOX_ID(rp_id),
				ret, ack_buffer[0], ack_buffer[1]);
			return -1;
		}
		if (rproc_cb)
			rproc_cb->msg_count++;
	}
	return 0;
}

static int test_rproc_send_async_msg_sync(
	unsigned char rp_id, unsigned int msg_len,
	int msg_num, struct test_rproc_cb *rproc_cb, rproc_msg_t *tx_buffer)
{
	int ret;

	while (msg_num--) {
		ret = RPROC_SYNC_SEND(rp_id, tx_buffer, msg_len, NULL, 0);
		if (ret) {
			pr_err("rproc send error, ret:%d, mbox:%d, sync 1!\r\n",
				ret, RPROC_GET_MAILBOX_ID(rp_id));
			return -1;
		}
		if (rproc_cb)
			rproc_cb->msg_count++;
	}
	return 0;
}

static int test_rproc_send_sync_msg_async(
	unsigned char rp_id, unsigned int msg_len,
	int msg_num, struct test_rproc_cb *rproc_cb, rproc_msg_t *tx_buffer)
{
	int ret;
	struct semaphore *complete_sema = NULL;
	unsigned int start_slice, end_slice;

	while (msg_num--) {
		complete_sema = kmalloc(sizeof(*complete_sema), GFP_KERNEL);
		if (!complete_sema)
			return -1;
		sema_init(complete_sema, 1);
		ret = RPROC_ASYNC_SEND(rp_id, tx_buffer, msg_len);
		if (ret) {
			pr_err("rproc send error, ret:%d, mbox:%d, sync 2!\r\n",
				ret, RPROC_GET_MAILBOX_ID(rp_id));
			kfree(complete_sema);
			return -1;
		}
		start_slice = test_rproc_get_slice();
		if (down_timeout(complete_sema,
			msecs_to_jiffies(SYNC_SEND_ASYNC_INTERVAL)) != 0) {
			pr_err("timeout mbox:%d rproc async send err!\r\n",
				RPROC_GET_MAILBOX_ID(rp_id));
			kfree(complete_sema);
			return -1;
		}
		end_slice = test_rproc_get_slice();
		pr_info("async send sync msg spend %d slice!\r\n",
			test_rproc_slice_diff(start_slice, end_slice));
		kfree(complete_sema);
		if (rproc_cb)
			rproc_cb->msg_count++;
	}
	return 0;
}

static int test_rproc_send_async_msg_async(
	unsigned char rp_id, unsigned int msg_len,
	int msg_num, struct test_rproc_cb *rproc_cb, rproc_msg_t *tx_buffer)
{
	int ret;

	while (msg_num--) {
		ret = RPROC_ASYNC_SEND(rp_id, tx_buffer, msg_len);
		if (ret) {
			pr_err("rproc send error, ret:%d, mbox:%d, sync 3!\r\n",
				ret, RPROC_GET_MAILBOX_ID(rp_id));
			return ret;
		}
		if (rproc_cb)
			rproc_cb->msg_count++;
	}
	return 0;
}

/*
 * Make multi task serial execute it one by one
 */
static void test_rproc_sync_task_update(struct test_rproc_cb *rproc_cb)
{
	unsigned int sync_task_cnt = 0;

	if (!rproc_cb)
		return;

	down(&task_mutex_sema);
	rproc_cb->sync_task_cnt--;
	sync_task_cnt = rproc_cb->sync_task_cnt;
	up(&task_mutex_sema);
	if (!sync_task_cnt &&
		(rproc_cb->sync_sema != TEST_RPROC_NULL)) {
		rproc_cb->start_slice = test_rproc_get_slice();
		up(rproc_cb->sync_sema);
	}

	/* Process synchronization */
	if (rproc_cb->sync_sema != TEST_RPROC_NULL) {
		down(rproc_cb->sync_sema);
		up(rproc_cb->sync_sema);
	}
}

static multi_task_test_case_func g_multi_task_test_cases[] = {
	[SYNC_AS_SYNC] = test_rproc_send_sync_msg_sync,
	[ASYNC_AS_SYNC] = test_rproc_send_async_msg_sync,
	[SYNC_AS_ASYNC] = test_rproc_send_sync_msg_async,
	[ASYNC_AS_ASYNC] = test_rproc_send_async_msg_async
};

static int test_rproc_msg_send(unsigned int sync_id, unsigned char rp_id,
	unsigned int msg_len, int msg_num, struct test_rproc_cb *rproc_cb)
{
	int ret;
	rproc_msg_t tx_buffer[TX_MAX_BUFFER] = {0};

	/* sync_id isn't more than 3 */
	if (sync_id >= MAX_SYNC_TYPE || !g_multi_task_test_cases[sync_id]) {
		pr_err("wrong sync id!\n");
		return -1;
	}

	test_rproc_sync_task_update(rproc_cb);

	tx_buffer[0] = IPC_TEST_BUF_DATA; /* 0x00120500 */
	ret = memcpy_s((void *)&tx_buffer[1],
		sizeof(tx_buffer) - sizeof(tx_buffer[0]),
		(void *)&g_data_buf[0], sizeof(g_data_buf));
	if (ret != EOK)
		pr_err("%s: mm failed, ret is %d\n", __func__, ret);

	ret = g_multi_task_test_cases[sync_id](rp_id, msg_len, msg_num,
			rproc_cb, tx_buffer);
	if (ret)
		return ret;
	pr_info("rproc send ok, mbox:%d, sync:%d!\n",
		RPROC_GET_MAILBOX_ID(rp_id), sync_id);

	return ret;
}

static int test_rproc_msg_send_entry(void *_proc)
{
	struct test_rproc_proc *rproc_proc = _proc;
	struct test_rproc_cb *rproc_cb = TEST_RPROC_NULL;
	unsigned int done_task_cnt = 0;

	if (!rproc_proc->proc_cb) {
		pr_err("test_rproc_msg_send_entry_proc_cb is NULL\n ");
		kfree(rproc_proc);
		return -ENODEV;
	}

	rproc_cb = rproc_proc->rproc_cb;
	rproc_cb->check_ret =
		rproc_proc->proc_cb(rproc_proc->sync_id, rproc_proc->rp_id,
			rproc_proc->msg_len, rproc_proc->msg_num, rproc_cb);
	kfree(rproc_proc);

	down(&task_mutex_sema);
	rproc_cb->done_task_cnt--;
	done_task_cnt = rproc_cb->done_task_cnt;
	up(&task_mutex_sema);

	/*
	 * If all task done, then raise the done semaphore to
	 * notify main thread all task done
	 */
	if (!done_task_cnt && (rproc_cb->done_sema != TEST_RPROC_NULL))
		up(rproc_cb->done_sema);
	return 0;
}

static void test_rproc_msg_create_task(test_rproc_send_func entry_ptr,
	unsigned int sync_id, unsigned char rp_id, unsigned int msg_len,
	int msg_num, unsigned int task_num, struct test_rproc_cb *rproc_cb)
{
	char task_name[TASK_MAX_NAME_LEN] = {0};
	int ret;
	struct test_rproc_proc *entry_param =
		kmalloc(sizeof(struct test_rproc_proc), GFP_KERNEL);

	if (!entry_param)
		return;

	if (memset_s(entry_param, sizeof(struct test_rproc_proc),
		0, sizeof(struct test_rproc_proc)) != EOK)
		pr_err("memset entry_param failed");

	entry_param->proc_cb = entry_ptr;
	entry_param->sync_id = sync_id;
	entry_param->rp_id = rp_id;
	entry_param->msg_len = msg_len;
	entry_param->msg_num = msg_num;
	entry_param->task_num = task_num;
	entry_param->rproc_cb = rproc_cb;

	ret = snprintf_s(task_name, sizeof(task_name),
		sizeof("trproc_task") + MAX_TASK_LIMIT, "trproc_task%d",
		(int)task_num);
	if (ret == -1)
		pr_err("%s:snprintf_s failed, ret is %d\n", __func__, ret);

	kthread_run(test_rproc_msg_send_entry, (void *)entry_param,
		task_name, task_num);
}

static int proc_multi_send_test_record(struct test_rproc_cb *rproc_cb,
	int rproc_id, unsigned int sync_id, unsigned int msg_len, int msg_num)
{
	unsigned int time_diff;

	rproc_cb->end_slice = test_rproc_get_slice();

	time_diff = test_rproc_slice_diff(
		rproc_cb->start_slice, rproc_cb->end_slice);
	pr_err("rp: %d, sync: %d, total:%d(4byte), latency: %d (slice)\n",
		rproc_id, sync_id,
		(int)(msg_len * msg_num * rproc_cb->task_num), (int)time_diff);

	if (rproc_cb->check_ret) {
		pr_err("%s send: CheckRet error\n", __func__);
		return -1;
	}

	if (msg_num * rproc_cb->task_num != rproc_cb->msg_count) {
		pr_err("%s send: MsgCount-%d error\n", __func__,
			(int)rproc_cb->msg_count);
		return -1;
	}

	pr_info("%s send: Success!\n", __func__);
	return 0;
}

static int test_rproc_msg_multi_send(
	unsigned int sync_id, unsigned char mbox_id,
	unsigned int msg_len, int msg_num, unsigned int task_num)
{
	struct test_rproc_cb rproc_cb = {0};
	int ret, rproc_id;

	rproc_cb.task_num = task_num;
	rproc_id = test_convert_to_rproc_id(mbox_id);
	if (rproc_id < 0) {
		pr_err("xxxxxxxx invalid mbox_id:%d xxxxxxxx\n", mbox_id);
		return -1;
	}

	pr_err("xxxxxxxx convert succ (m:%d, r:%d) xxxxxxxx\n",
		mbox_id, rproc_id);

	if ((int)msg_len > RPROC_GET_MBOX_CHAN_DATA_SIZE(rproc_id)) {
		pr_err("illegal message length!\n");
		return -1;
	}

	down(&send_mutex_sema);
	/* Create callback user handle */

	rproc_cb.done_sema = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	if (!rproc_cb.done_sema) {
		ret = -1;
		goto up_send_mutex_sema;
	}
	sema_init(rproc_cb.done_sema, 0);

	rproc_cb.sync_sema = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	if (!rproc_cb.sync_sema) {
		ret = -1;
		goto free_done_sema;
	}
	sema_init(rproc_cb.sync_sema, 0);
	rproc_cb.check_ret = -1;
	rproc_cb.sync_task_cnt = task_num;
	rproc_cb.done_task_cnt = task_num;
	rproc_cb.msg_count = 0;

	/*
	 * Create a promise task, the task entry function
	 * is the function that this core sends messages to other cores
	 */
	while (task_num) {
		test_rproc_msg_create_task(test_rproc_msg_send, sync_id,
			rproc_id, msg_len, msg_num, task_num, &rproc_cb);

		task_num--;
	}

	/* Waiting all task done */
	down(rproc_cb.done_sema);
	ret = proc_multi_send_test_record(
		&rproc_cb, rproc_id, sync_id, msg_len, msg_num);

	kfree(rproc_cb.sync_sema);

free_done_sema:
	kfree(rproc_cb.done_sema);

up_send_mutex_sema:
	up(&send_mutex_sema);
	return ret;
}

static void test_fill_send_to_lpmcu_head(union ipc_data *lpm3_msg)
{
	lpm3_msg->cmd_mix.cmd_src = OBJ_AP;
	lpm3_msg->cmd_mix.cmd_obj = OBJ_LPM3;
	lpm3_msg->cmd_mix.cmd = CMD_SETTING;
	lpm3_msg->cmd_mix.cmd_type = TYPE_TEST;
}

static int test_ipc_device_all_lpmcpu_mbx(struct test_ipc_device *ipc_dev,
	union ipc_data *lpm3_msg, int syntypye, int sleep_ms)
{
	int idx, err;
	struct mailbox_device *mbox = NULL;
	rproc_msg_t ack_buffer[2] = {0};

	for (idx = 0; idx < ipc_dev->mbx_cnt; idx++) {
		mbox = &ipc_dev->mboxes[idx];
		if (ipc_dev->lpm_cpu_bit != mbox->dest ||
			mbox->rproc_id < 0 || mbox->used != 1)
			continue;

		if (syntypye % SEND_TYPE_CNT)
			err = RPROC_ASYNC_SEND(mbox->rproc_id,
				(rproc_msg_t *)lpm3_msg,
				ARRAY_SIZE(lpm3_msg->data));
		else
			err = RPROC_SYNC_SEND(mbox->rproc_id,
				(rproc_msg_t *)lpm3_msg,
				ARRAY_SIZE(lpm3_msg->data),
				ack_buffer, ARRAY_SIZE(ack_buffer));
		if (err) {
			pr_err("xxxxx pressure test failed and break err:%d!\n",
				err);
			return err;
		}

		msleep(sleep_ms);
	}

	return 0;
}

static int test_all_send_to_lpmcu(void *arg)
{
	int err;
	unsigned int idx;
	union ipc_data *lpm3_msg = NULL;
	int syntypye, idata;

	lpm3_msg = kmalloc(sizeof(*lpm3_msg), GFP_KERNEL);
	if (!lpm3_msg)
		return -1;

	if (memset_s(lpm3_msg, sizeof(*lpm3_msg), 0, sizeof(*lpm3_msg)) != EOK)
		pr_err("memset lpm3_msg failed");

	test_fill_send_to_lpmcu_head(lpm3_msg);
	/*
	 * data[1]: 0xAABBCCDD
	 * AA: 00(sec or not)
	 * BB: mailbox id, lpmcu-->ap is 0
	 * CC: dest core (1:A53, 2:Maia, 4:Sensorhub, 8:LPMCU, 16:HIFI,
	 * 32:MODEM, 64:BBE16, 128:IVP)
	 * DD:	mode, 1 for autoack
	 */
	lpm3_msg->data[1] = 0x01000101;

	syntypye = 0;
	idata = 0;
	while (1) {
		syntypye++;
		idata++;
		if (syntypye % LPMCU_TEST_PRINT_TIMES == 0)
			pr_err("xxxxxxx pressure test have succeed %d cycles\n",
				syntypye);

		for (idx = IPC_CMD_PARAM_START_IDX;
			idx < ARRAY_SIZE(lpm3_msg->data); idx++)
			lpm3_msg->data[idx] = idata + (0x1 << (idx - 2));

		for (idx = 0;
			idx < (unsigned int)g_test_ipc_devices.ipc_dev_cnt;
			idx++) {
			err = test_ipc_device_all_lpmcpu_mbx(
				&g_test_ipc_devices.ipc_devs[idx], lpm3_msg,
				syntypye, *(int *)arg);
			if (err) {
				kfree(lpm3_msg);
				return -1;
			}
		}
	}

	kfree(lpm3_msg);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static int rproc_performance_send_update(struct timespec64 *time_spec,
	unsigned char rproc_id, unsigned int index, int *sleep_cnt)
#else
static int rproc_performance_send_update(struct timespec *time_spec,
	unsigned char rproc_id, unsigned int index, int *sleep_cnt)
#endif
{
	if (time_spec->tv_sec || time_spec->tv_nsec > PERFORMANC_FAIL_NSEC) {
		pr_err("rproc_perf:r-%d idx-%d cost %lu sec, %lu nsec\n",
			rproc_id, index,
			(unsigned long)time_spec->tv_sec, time_spec->tv_nsec);
		/*
		 * The remote processor like HIFI, SENSORHUB may
		 * fall into sleep and the first IPC will wakeup
		 * it, this may cost a lot time
		 */
		*sleep_cnt += 1;
		if (*sleep_cnt > 10)
			return 0;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static int calc_performance_time_spec(struct timespec64 *time_spec,
	struct timespec64 *time_begin, int rproc_id, int idx, int *sleep_cnt)
{
	struct timespec64 time_end = {0};

	ktime_get_ts64(&time_end);
#else
static int calc_performance_time_spec(struct timespec *time_spec,
	struct timespec *time_begin, int rproc_id, int idx, int *sleep_cnt)
{
	struct timespec time_end = {0};

	getnstimeofday(&time_end);
#endif
	time_spec->tv_sec = time_end.tv_sec - time_begin->tv_sec;
	if (time_end.tv_nsec < time_begin->tv_nsec && time_spec->tv_sec) {
		time_spec->tv_sec -= 1;
		time_spec->tv_nsec = time_end.tv_nsec + NS_TO_S -
			time_begin->tv_nsec;
	} else {
		time_spec->tv_nsec = time_end.tv_nsec - time_begin->tv_nsec;
	}
	/*
	 * if cost more than PERFORMANC_FAIL_NSEC,
	 * then break and fail
	 */
	return rproc_performance_send_update(
		time_spec, rproc_id, idx, sleep_cnt);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static unsigned int rproc_performance_cnt(struct timespec64 *ptimespec,
	unsigned char rproc_id, union ipc_data *msg, unsigned char len,
	unsigned char synctpye)
#else
static unsigned int rproc_performance_cnt(struct timespec *ptimespec,
	unsigned char rproc_id, union ipc_data *msg, unsigned char len,
	unsigned char synctpye)
#endif
{
	unsigned int idx;
	rproc_msg_t ack_buffer[2] = {0};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	struct timespec64 time_begin = {0};
#else
	struct timespec time_begin = {0};
#endif
	unsigned int ret = 0;
	int sleep_cnt = 0;

	for (idx = 0; idx < PRESSURE_TIMES; idx++) {
		/* count the first PERFORMANC_TIMES 's cost only */
		if (idx < PERFORMANC_TIMES)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
			ktime_get_ts64(&time_begin);
#else
			getnstimeofday(&time_begin);
#endif

		if (synctpye == SYNC_SEND) {
			ret |= (unsigned int)RPROC_SYNC_SEND(rproc_id,
				(rproc_msg_t *)msg, len, ack_buffer, 2);
		} else if (synctpye == ASYNC_SEND) {
			ret |= (unsigned int)RPROC_ASYNC_SEND(rproc_id,
				(rproc_msg_t *)msg, len);
		}

		if (idx < PERFORMANC_TIMES)
			ret |= (unsigned int)calc_performance_time_spec(
				&ptimespec[idx], &time_begin,
				rproc_id, idx, &sleep_cnt);

		if (ret)
			break;

		/* sleep 1ms for next test cycle */
		if (synctpye == ASYNC_SEND)
			usleep_range(1000, 2000);
	}

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static void show_rproc_performance(
	struct timespec64 *ptimespec, unsigned char rproc_id,
	unsigned char synctpye)
#else
static void show_rproc_performance(
	struct timespec *ptimespec, unsigned char rproc_id,
	unsigned char synctpye)
#endif
{
	unsigned int index;

	for (index = 0; index < PERFORMANC_TIMES; index++) {
		/* just show the some of the index */
		if (index % PERFORMANC_SHOW_INDEX != 0)
			continue;

		if (synctpye == SYNC_SEND)
			pr_err("rproc_perf:r-%d sync cost %lu sec, %lu nsec\n",
				rproc_id,
				(unsigned long)ptimespec[index].tv_sec,
				ptimespec[index].tv_nsec);
		else if (synctpye == ASYNC_SEND)
			pr_err("rproc_perf:r-%d async cost %lu sec, %lu nsec\n",
				rproc_id,
				(unsigned long)ptimespec[index].tv_sec,
				ptimespec[index].tv_nsec);
	}
}

static int test_async_send_to_lpmcu(void *arg)
{
	int err, mbox_lpm3;
	unsigned int i;
	union ipc_data *lpm3_msg = NULL;
	int rproc_id;

	lpm3_msg = kmalloc(sizeof(*lpm3_msg), GFP_KERNEL);
	if (!lpm3_msg)
		return -1;

	if (memset_s(lpm3_msg, sizeof(*lpm3_msg), 0, sizeof(*lpm3_msg)) != EOK)
		pr_err("memset lpm3_msg failed");

	test_fill_send_to_lpmcu_head(lpm3_msg);
	/*
	 * data[1]: 0xAABBCCDD
	 * AA: 00(sec or not)
	 * BB: mailbox id, lpmcu-->ap is 0
	 * CC: dest core (1:A53, 2:Maia, 4:Sensorhub, 8:LPMCU, 16:HIFI,
	 * 32:MODEM, 64:BBE16, 128:IVP)
	 * DD:	mode, 1 for autoack
	 */
	lpm3_msg->data[1] = 0x01000101;
	for (i = 2; i < ARRAY_SIZE(lpm3_msg->data); i++)
		lpm3_msg->data[i] = 0x11111111 * i;

	mbox_lpm3 = *(int *)arg;
	rproc_id = test_convert_to_rproc_id(mbox_lpm3);
	if (rproc_id < 0) {
		pr_err("xxxxxxxx invalid mbox_id:%d xxxxxxxx\n", mbox_lpm3);
		kfree(lpm3_msg);
		return -1;
	}

	while (1) {
		err = RPROC_ASYNC_SEND(rproc_id,
			(rproc_msg_t *)lpm3_msg, ARRAY_SIZE(lpm3_msg->data));
		if (err) {
			pr_err("xxxxxxxxx pressure test failed err %d!\n", err);
			kfree(lpm3_msg);
			return -1;
		}
		msleep(interval_v);
	}

	return 0;
}

static int mbox_recv_from_tscpu(
	struct notifier_block *nb, unsigned long len, void *msg)
{
	unsigned int i;

	pr_err("****[qqq]: fun, %s enter.\n\n", __func__);
	for (i = 0; i < len; ++i)
		pr_err("-------->msg[%d] = %#x\n", i, ((char *)msg)[i]);

	pr_err("\n\n");
	return 0;
}

static void test_npu_ipc_init(int from_npu_r_id)
{
	int ret;
	static int times;

	/* init just once */
	if (times)
		return;

	npu_nb.next = NULL;
	npu_nb.notifier_call = mbox_recv_from_tscpu;
	pr_err("***[npu]%s:enter.\n", __func__);
	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(from_npu_r_id, &npu_nb);
	if (ret)
		pr_err("%s:RPROC_MONITOR_REGISTER failed\n", __func__);

	times++;
}

static void prepare_tscpu_msg_for_suspend(struct _tagipc_message *tscpu_msg)
{
	unsigned int i;

	tscpu_msg->msg_header.cmd_type = 0; /* CMDTYPE TS_LOWPOWER_SUSPENDREQ */
	tscpu_msg->msg_header.msg_index = 0x0;
	tscpu_msg->msg_header.msg_lenth = ARRAY_SIZE(tscpu_msg->ipc_data);
	tscpu_msg->msg_header.msg_type = 0x0;  /* MSGTYPE_TS_SEND */
	tscpu_msg->msg_header.sync_type = 0x1; /* MSG_ASYNC */
	for (i = 0; i < ARRAY_SIZE(tscpu_msg->ipc_data); i++)
		tscpu_msg->ipc_data[i] = (i / 10 * 0x10) + i % 10;
}

static int test_async_send_to_tscpu_suspend(void *arg)
{
	int err, rproc_tscpu;
	struct _tagipc_message *tscp_msg = NULL;

	tscp_msg = kmalloc(sizeof(*tscp_msg), GFP_KERNEL);
	if (!tscp_msg)
		return -1;

	if (memset_s(tscp_msg, sizeof(*tscp_msg), 0, sizeof(*tscp_msg)) != EOK)
		pr_err("memset tscp_msg failed");

	prepare_tscpu_msg_for_suspend(tscp_msg);

	rproc_tscpu = *(int *)arg;
	/* send once for test */
	do {
		pr_err("****[test1]qqq fun:%s send msg to ts cpu\n", __func__);
		err = RPROC_ASYNC_SEND(rproc_tscpu, (rproc_msg_t *)tscp_msg,
			TS_CP_MSG_LEN);
		if (err) {
			pr_err("xxxxxxxxx pressure test failed,err:%d!\n", err);
			kfree(tscp_msg);
			return -1;
		}

		msleep(interval_v);
	} while (0);

	kfree(tscp_msg);

	return 0;
}

static void prepare_tscpu_msg_for_tscpu(struct _tagipc_message *tscpu_msg)
{
	unsigned int i;

	tscpu_msg->msg_header.cmd_type = CMDTYPE_TS_TEST_MSG;
	tscpu_msg->msg_header.msg_index = 0x0;
	tscpu_msg->msg_header.msg_lenth = ARRAY_SIZE(tscpu_msg->ipc_data);
	tscpu_msg->msg_header.msg_type = 0x0;  /* MSGTYPE_TS_SEND */
	tscpu_msg->msg_header.sync_type = 0x0; /* MSG_SYNC */

	for (i = 0; i < ARRAY_SIZE(tscpu_msg->ipc_data); i++) {
		if (i < TS_START_IDX || i > TS_END_IDX)
			tscpu_msg->ipc_data[i] = 0xff;
		else
			tscpu_msg->ipc_data[i] = (i / 10 * 0x10) + i % 10;
	}
}

static int test_async_send_to_tscpu(int to_npu_r_id, int from_npu_r_id)
{
	int err;
	struct _tagipc_message *tscp_msg = NULL;

	tscp_msg = kmalloc(sizeof(*tscp_msg), GFP_KERNEL);
	if (!tscp_msg)
		return -1;

	if (memset_s(tscp_msg, sizeof(*tscp_msg), 0, sizeof(*tscp_msg)) != EOK)
		pr_err("memset tscp_msg failed");

	test_npu_ipc_init(from_npu_r_id);
	prepare_tscpu_msg_for_tscpu(tscp_msg);

	/* send once for test */
	do {
		pr_err("****[test1]qqq fun:%s send msg to ts cpu\n", __func__);
		err = RPROC_ASYNC_SEND(to_npu_r_id, (rproc_msg_t *)tscp_msg,
			TS_CP_MSG_LEN);
		if (err) {
			pr_err("xxxxxxxxx pressure test failed, err %d!\n",
				err);
			break;
		}
		msleep(interval_v);
	} while (0);

	kfree(tscp_msg);
	return 0;
}

static int mbox_recv_test(
	struct notifier_block *nb, unsigned long len, void *msg)
{
	unsigned int i;

	pr_err("$$$$$$$$$$$$$$$$$$$$$----%s---$$$$$$$$$$$$$$$$$$$$$\n",
		__func__);
	for (i = 0; i < len; ++i) {
		pr_err("-------->msg[%d] = %#.8x\n", i, ((int *)msg)[i]);
		if (((int *)msg)[i] != 0x76543210)
			pr_err("$$$$$$$$$$$$$$$$$$$$$----%s receive error!\n",
				__func__);
	}

	return 0;
}

static void test_cfg_mbox_send_data(struct test_ipc_device *ipc_dev,
	int rproc_id)
{
	unsigned int reg_spc, mbox_id, idx;
	int channel_size;
	struct mailbox_device *mbox = find_ipc_mbox_dev(ipc_dev, rproc_id);

	pr_err("%s entry\n", __func__);
	if (!mbox) {
		pr_err("invalid rproc_id:%d\n", rproc_id);
		return;
	}

	if (SOURCE_MBOX & mbox->func) {
		pr_err("Not a send mailbox invalid mailbox:\n");
		return;
	}

	reg_spc = (ipc_dev->ipc_version == IPC_VERSION_APPLE) ?
		IPC_REG_SPACE : IPC_REG_SPACE_EX;
	mbox_id = mbox->mbox_channel;
	channel_size = RPROC_GET_MBOX_CHAN_DATA_SIZE(mbox->rproc_id);
	if (channel_size <= 0) {
		pr_err("invalid channel size cfg:%d:%d\n",
			rproc_id, channel_size);
		return;
	}

	pr_err("r:%d reg_spc:%u, channel_size:%d, m:%u, src:%u, dest:%u\n",
		rproc_id, reg_spc, channel_size,
		mbox_id, mbox->src, mbox->dest);

	/* unlock ipc */
	__ipc_writel(ipc_dev, ipc_dev->unlock,
		(unsigned int)ipc_lock(ipc_dev->ipc_version));

	/* release channel */
	__ipc_writel(ipc_dev,
		__ipc_readl(ipc_dev, (unsigned int)ipc_mbx_source(mbox_id, reg_spc)),
		(unsigned int)ipc_mbx_source(mbox_id, reg_spc));

	/* set LPMCU as MBX_SOURCE */
	__ipc_writel(ipc_dev, (unsigned int)ipc_bitmask(mbox->src),
		(unsigned int)ipc_mbx_source(mbox_id, reg_spc));
	__ipc_writel(ipc_dev, (unsigned int)ipc_bitmask(mbox->dest),
		(unsigned int)ipc_mbx_dset(mbox_id, reg_spc));

	for (idx = 0; idx < (unsigned int)channel_size; idx++)
		__ipc_writel(ipc_dev, 0x76543210, (unsigned int)ipc_mbx_data(mbox_id, idx, reg_spc));

	__ipc_writel(ipc_dev, (unsigned int)ipc_bitmask(mbox->src),
		(unsigned int)ipc_mbx_send(mbox_id, reg_spc));

	pr_err("%s end\n", __func__);
}

static int test_receive_mailbox(unsigned int ipc_type, int mbox_id)
{
	int ret, rproc_id;
	static struct notifier_block ao_ipc_nb;
	struct test_ipc_device *ipc_dev = find_ipc_dev_by_type(ipc_type);

	if (!ipc_dev) {
		pr_err("XXXXXX %s:Invalid ipc_type:%d XXXXXX",
			__func__, ipc_type);
		return -1;
	}

	rproc_id = test_convert_to_rproc_id_by_ipc_type(ipc_type, mbox_id);
	if (rproc_id < 0) {
		pr_err("xxxxxxxx invalid mbox_id:%d xxxxxxxx\n", mbox_id);
		return -1;
	}

	pr_err("xxxxxxxx convert succ (m:%d, r:%d) xxxxxxxx\n",
		mbox_id, rproc_id);

	ao_ipc_nb.next = NULL;
	ao_ipc_nb.notifier_call = mbox_recv_test;

	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(rproc_id, &ao_ipc_nb);
	if (ret) {
		pr_err("%s:RPROC_MONITOR_REGISTER failed\n", __func__);
		return -1;
	}

	test_cfg_mbox_send_data(ipc_dev, rproc_id);

	msleep(WAIT_MBX_RECV_TIME);
	RPROC_MONITOR_UNREGISTER(rproc_id, &ao_ipc_nb);

	return ret;
}

static union ipc_data *prepare_test_ipc_msg(void)
{
	union ipc_data *ipc_msg = kmalloc(sizeof(*ipc_msg), GFP_KERNEL);
	unsigned int i;

	if (!ipc_msg)
		return NULL;

	if (memset_s(ipc_msg, sizeof(*ipc_msg), 0, sizeof(*ipc_msg)) != EOK)
		pr_err("memset ipc_msg failed");

	for (i = 0; i < ARRAY_SIZE(ipc_msg->data); i++)
		ipc_msg->data[i] = 0x11111111 *
			(ARRAY_SIZE(ipc_msg->data) - i - 1);

	return ipc_msg;
}

static const char *convet_cpu_name(
	struct test_ipc_device *ipc_dev, unsigned int cpu_id)
{
	if (cpu_id >= ipc_dev->rproc_names_cnt)
		return "Invalid cpu";

	return ipc_dev->rproc_names[cpu_id];
}

static void show_ipc_test_info(struct test_ipc_device *ipc_dev)
{
	int idx;
	struct mailbox_device *mbox = NULL;

	pr_debug("%s%s%s", "--------------------------------",
		"--------------------------------",
		"--------------------------------");
	pr_debug("%14s %32s %8s %7s %9s   %9s %6s",
		"mbox_name", "rproc_name:id", "channel",
		"ipc_version", "from", "to", "used");
	pr_debug("%s%s%s", "------------------------------",
		"------------------------------",
		"------------------------------");
	for (idx = 0; idx < ipc_dev->mbx_cnt; idx++) {
		mbox = &ipc_dev->mboxes[idx];
		pr_debug("%14s %28s:%3d %8u %11d %9s -> %9s %6s",
			mbox->mbx_name,
			ipc_get_rproc_name(mbox->rproc_id), mbox->rproc_id,
			mbox->mbox_channel, ipc_dev->ipc_version,
			convet_cpu_name(ipc_dev, mbox->src),
			convet_cpu_name(ipc_dev, mbox->dest),
			((mbox->used == MAILBOX_NO_USED) ? "UNUSED" : "USED"));
	}
}

static void test_ipc_msg_send_by_type(int rproc_id,
	union ipc_data *ipc_msg, int ipc_msg_len, int sync_flg)
{
	int err;
	rproc_msg_t ack_buffer[2] = {0};

	if (sync_flg == IPC_ASYNC)
		err = RPROC_ASYNC_SEND(
			rproc_id, (rproc_msg_t *)ipc_msg, ipc_msg_len);
	else
		err = RPROC_SYNC_SEND(rproc_id,
			(rproc_msg_t *)ipc_msg, ipc_msg_len, ack_buffer, 2);
	if (err)
		pr_err("xxxxxxxxx mailbox channel %d: send err!\n",
			RPROC_GET_MAILBOX_ID(rproc_id));
	else
		pr_err("xxxxxxxxx %s channel %d: send ok, ack is 0x%x, 0x%x!\n",
			"mailbox", RPROC_GET_MAILBOX_ID(rproc_id),
			ack_buffer[0], ack_buffer[1]);
}

static int test_ipc_msg_send(unsigned int ipc_type,
	union ipc_data *ipc_msg, int ipc_msg_len, int interval, int sync_flg)
{
	int index;
	char *sync_name = (sync_flg == IPC_SYNC) ? "SYNC_SEND" : "ASYNC_SEND";
	struct test_ipc_device *ipc_dev = find_ipc_dev_by_type(ipc_type);

	if (!ipc_dev) {
		pr_err("XXXXXX invalid ipc_type:%d XXXXXX", ipc_type);
		return -1;
	}

	pr_err("xxxxxxxxx %s %s start! xxxxxxxxx\n",
		ipc_dev->ipc_name, sync_name);

	show_ipc_test_info(ipc_dev);
	for (index = 0; index < ipc_dev->mbx_cnt; index++) {
		if ((ipc_dev->mboxes[index].rproc_id != INVALID_RPROC_ID) &&
			(ipc_dev->mboxes[index].used != MAILBOX_NO_USED))
			test_ipc_msg_send_by_type(
				ipc_dev->mboxes[index].rproc_id,
				ipc_msg, ipc_msg_len, sync_flg);

		msleep(interval);
	}
	pr_err("xxxxxxxxx  %s %s end!  xxxxxxxxx\n",
		ipc_dev->ipc_name, sync_name);

	return 0;
}

static int test_single_ipc_send(int send_type, int mbox_id, int interval,
	int threads, union ipc_data *ipc_msg, int ipc_msg_len)
{
	int index, ret;
	int err = 0;
	rproc_msg_t ack_buffer[2] = {0};
	int rproc_id;

	rproc_id = test_convert_to_rproc_id(mbox_id);
	if (rproc_id < 0) {
		pr_err("xxxxxxxx invalid mbox_id:%d xxxxxxxx\n", mbox_id);
		return -1;
	}

	pr_err("xxxxxxxx convert succ (m:%d, r:%d) xxxxxxxx\n",
		mbox_id, rproc_id);
	for (index = 0; index < threads; index++) {
		if (send_type == SINGLE_IPC_SYNC_TEST)
			ret = RPROC_SYNC_SEND((rproc_id_t)rproc_id,
				(rproc_msg_t *)ipc_msg, ipc_msg_len,
				ack_buffer, 2);
		else
			ret = RPROC_ASYNC_SEND((rproc_id_t)rproc_id,
				(rproc_msg_t *)ipc_msg, ipc_msg_len);
		if (ret)
			err++;
		msleep(interval);
	}
	if (err > 0)
		pr_err("xxxxxxxxx mailbox channel %d: send err count: %d!\n",
			mbox_id, err);
	else
		pr_err("xxxxxxxxx mailbox channel %d send succeed\n", mbox_id);

	return err;
}

/* traverse all peri ipc channel in sync mode */
def_ipc_test_case(peri_ipc_sync_test)
{
	return test_ipc_msg_send(PERI_NS_NORMAL_IPC, ipc_msg,
		ARRAY_SIZE(ipc_msg->data), interval, IPC_SYNC);
}

/* traverse all peri ipc channel in async mode */
def_ipc_test_case(peri_ipc_async_test)
{
	return test_ipc_msg_send(PERI_NS_NORMAL_IPC, ipc_msg,
		ARRAY_SIZE(ipc_msg->data), interval, IPC_ASYNC);
}

/* traverse all ao ipc channel in sync mode */
def_ipc_test_case(ao_ipc_sync_test)
{
	return test_ipc_msg_send(AO_NS_IPC, ipc_msg,
		ARRAY_SIZE(ipc_msg->data), interval, IPC_SYNC);
}

/* traverse all ao ipc channel in async mode */
def_ipc_test_case(ao_ipc_async_test)
{
	return test_ipc_msg_send(AO_NS_IPC, ipc_msg,
		ARRAY_SIZE(ipc_msg->data), interval, IPC_ASYNC);
}

/* single mailbox sync send */
def_ipc_test_case(single_ipc_sync_test)
{
	return test_single_ipc_send(SINGLE_IPC_SYNC_TEST,
		obj, interval, threads, ipc_msg, ARRAY_SIZE(ipc_msg->data));
}

/* single mailbox async send */
def_ipc_test_case(single_ipc_async_test)
{
	return test_single_ipc_send(SINGLE_IPC_ASYNC_TEST,
		obj, interval, threads, ipc_msg, ARRAY_SIZE(ipc_msg->data));
}

/*
 * often used to lpmcu with mailbox-13 for async
 * pressure test, obj can change to other mailbox
 */
def_ipc_test_case(mailbox_lpm_press_test)
{
	int index;

	for (index = 0; index < threads; index++)
		kthread_run(test_async_send_to_lpmcu,
			(void *)&obj_mailbox, "async_to_lpmcu");

	return 0;
}

/*
 * send to lpmcu and will receive msg from lpmcu,
 * use for pressure test
 */
def_ipc_test_case(test_all_send_to_lpmcu)
{
	int index;

	for (index = 0; index < threads; index++)
		kthread_run(test_all_send_to_lpmcu, (void *)&interval_v,
			"all_to_lpmcu");

	return 0;
}

/* test peri ipc reveive test */
def_ipc_test_case(peri_ipc_recv_test)
{
	if (threads == 1)
		return test_receive_mailbox(PERI_NS_NORMAL_IPC, obj);

	pr_err("threads is error\n");
	return -1;
}

/* test ao ipc reveive test */
def_ipc_test_case(ao_ipc_recv_test)
{
	if (threads == 1)
		return test_receive_mailbox(AO_NS_IPC, obj);

	pr_err("threads is error\n");
	return -1;
}

static void test_ipc_async_ack_handle(rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len)
{
	if (!ack_buffer || !ack_buffer_len)
		pr_err("xxxxxxxxx Async send recv a ack msg timeout\n");
	else
		pr_err("xxxxxxxxx Async send recv a ack msg [0x%X][0x%X]\n",
			ack_buffer[0], ack_buffer[1]);
}

def_ipc_test_case(single_ipc_async_test_with_ack)
{
	int index, err, ret, rproc_id;
	unsigned int ipc_msg_len = ARRAY_SIZE(ipc_msg->data);

	rproc_id = test_convert_to_rproc_id(obj);
	if (rproc_id < 0) {
		pr_err("xxxxxxxx invalid mbox_id:%d xxxxxxxx\n", obj);
		return -1;
	}

	for (index = 0, err = 0; index < threads; index++) {
		ret = ipc_rproc_xfer_async_with_ack((rproc_id_t)rproc_id,
				(rproc_msg_t *)ipc_msg, ipc_msg_len,
				test_ipc_async_ack_handle);
		if (ret)
			err++;
		msleep(interval);
	}

	if (err > 0)
		pr_err("xxxxxxxxx mailbox channel %d: send err count: %d!\n",
			obj, err);
	else
		pr_err("xxxxxxxxx mailbox channel %d send succeed\n", obj);

	return err;
}

static ipc_test_case_func g_ipc_test_cases[MAX_IPC_TEST_MODE_TYPE] = {
	add_ipc_test_case(PERI_IPC_SYNC_TEST, peri_ipc_sync_test),
	add_ipc_test_case(PERI_IPC_ASYNC_TEST, peri_ipc_async_test),
	add_ipc_test_case(AO_IPC_SYNC_TEST, ao_ipc_sync_test),
	add_ipc_test_case(AO_IPC_ASYNC_TEST, ao_ipc_async_test),
	add_ipc_test_case(SINGLE_IPC_SYNC_TEST, single_ipc_sync_test),
	add_ipc_test_case(SINGLE_IPC_ASYNC_TEST, single_ipc_async_test),
	add_ipc_test_case(MAILBOX_LPM_PRESS_TEST, mailbox_lpm_press_test),
	add_ipc_test_case(TEST_ALL_SEND_TO_LPMCU, test_all_send_to_lpmcu),
	add_ipc_test_case(PERI_IPC_RECV_TEST, peri_ipc_recv_test),
	add_ipc_test_case(AO_IPC_RECV_TEST, ao_ipc_recv_test),
	add_ipc_test_case(SINGLE_IPC_ASYNC_TEST_WITH_ACK,
		single_ipc_async_test_with_ack)
};

static inline int is_invalid_ipc_test_type(int type)
{
	return type < 0 || type >= MAX_IPC_TEST_MODE_TYPE ||
		!g_ipc_test_cases[type];
}

/*
 * test all the ipc channel.
 * @type:
 *     1. send all peri ipc sync msg
 *     2. send all peri ipc async msg
 *     3. send all ao ipc sync msg
 *     4. send all ao ipc async msg
 *     5. single mailbox sync test
 *     6. single mailbox async test
 *     7. pressure test with lpmcu through all channels
 *     8. pressure test with lpmcu async
 *     9. peri ipc receive test
 *     10. AO ipc receive test
 *     11. single mailbox async with ack test
 *
 * @objmax:   the remote proccessor number to send ipc msg.  Sometimes like
 *            HIFI,ISP,etc. are in sleepmode and not suitable for pressure test.
 * @interval: to delay, ms
 * @threads:  thread num to create / times to send
 *
 * @return 0:success, others failed
 */
int test_all_kind_ipc(int type, int obj, int interval, int threads)
{
	int err;
	union ipc_data *ipc_msg = NULL;

	obj_mailbox = obj;
	interval_v = interval;

	ipc_msg = prepare_test_ipc_msg();
	if (!ipc_msg)
		return -1;

	if (is_invalid_ipc_test_type(type)) {
		pr_err("invalid test case type:%d\n", type);
		kfree(ipc_msg);
		return -1;
	}

	err = g_ipc_test_cases[type](obj, interval, threads, ipc_msg);
	kfree(ipc_msg);

	return err;
}

/*
 * Multi task sync send test
 * @msg_type
 *     0:SYNC_AS_SYNC
 *     1:ASYNC_AS_SYNC
 *     2:SYNC_AS_ASYNC
 *     3:ASYNC_AS_ASYNC
 * @task_num    multi test task num
 * @mbox_id     mailbox phyical channel id
 * @msg_len     test message len
 * @msg_num     test message num
 */
void test_rproc_multi_task_sync(unsigned int msg_type, unsigned char task_num,
	unsigned char mbox_id, unsigned int msg_len, int msg_num)
{
	int ret;
	unsigned int sync_id;

	if (task_num > MAX_TASK_LIMIT) {
		pr_err("illegal task_num!\n");
		return;
	}

	sync_id = (msg_type == 0) ? 0 : 1;
	ret = test_rproc_msg_multi_send(
		sync_id, mbox_id, msg_len, msg_num, task_num);
	if (ret)
		pr_err("test %s: Fail\r\n", __func__);
	else
		pr_err("test %s: Success\r\n", __func__);
}

/*
 * Multi task sync send test
 * @msg_type
 *     0:SYNC_AS_ASYNC
 *     1:ASYNC_AS_ASYNC
 * @task_num    multi test task num
 * @mbox_id     mailbox phyical channel id
 * @msg_len     test message len
 * @msg_num     test message num
 */
void test_rproc_multi_task_async(unsigned int msg_type, unsigned char task_num,
	unsigned char mbox_id, unsigned int msg_len, int msg_num)
{
	int ret;
	unsigned int sync_id;

	sync_id = (msg_type == 0) ? 2 : 3;
	ret = test_rproc_msg_multi_send(
		sync_id, mbox_id, msg_len, msg_num, task_num);
	if (ret)
		pr_err("%s: Fail\r\n", __func__);
	else
		pr_err("%s: Success\r\n", __func__);
}

/*
 * Single-thread external test function
 * @msg_type
 *     0:SYNC_AS_SYNC
 *     1:ASYNC_AS_SYNC
 * @task_num    multi test task num
 * @mbox_id     mailbox phyical channel id
 * @msg_len     test message len
 * @msg_num     test message num
 */
void test_rproc_single_task_sync(unsigned int msg_type, unsigned char mbox_id,
	unsigned int msg_len, int msg_num)
{
	int ret;
	unsigned int sync_id;

	sync_id = (msg_type == 0) ? 0 : 1;
	ret = test_rproc_msg_multi_send(sync_id, mbox_id, msg_len, msg_num, 1);
	if (ret)
		pr_err("test rproc_single_task_sync: Fail\r\n");
	else
		pr_err("test rproc_single_task_sync: Success\r\n");
}

/*
 * Single-thread external test function
 * @msg_type
 *     0:SYNC_AS_ASYNC
 *     1:ASYNC_AS_ASYNC
 * @task_num    multi test task num
 * @mbox_id     mailbox phyical channel id
 * @msg_len     test message len
 * @msg_num     test message num
 */
void test_rproc_single_task_async(unsigned int msg_type, unsigned char mbox_id,
	unsigned int msg_len, int msg_num)
{
	int ret;
	unsigned int sync_id;

	sync_id = (msg_type == 0) ? 2 : 3;
	ret = test_rproc_msg_multi_send(sync_id, mbox_id, msg_len, msg_num, 1);
	if (ret)
		pr_err("test rproc_single_task_async: Fail\r\n");
	else
		pr_err("test rproc_single_task_async: Success\r\n");
}

/*
 * tee sec ipc entry, this func may trigger timer interrtupt,
 * then tee timer intertupt handler call sec ipc func to send ipc msg
 * timer10 is useful or not should check every platform.
 */
int test_tee_os_ipc(void)
{
	void __iomem *test_ipc_timer_addr = NULL;
	unsigned int i, timer_irq_0, timer_irq_1, timeout;

	pr_err("%s test start\n", __func__);
	test_ipc_timer_addr = ioremap(TIMER_REG_BASE, TIMER_SIZE);/*lint !e446*/
	if (!test_ipc_timer_addr) {
		pr_err("%s timer base addr remap error\n", __func__);
		return -1;
	}
	for (i = 0; i < 10; i++) {
		/* config timer0 */
		writel(TIMER_VALUE, timerx_load(test_ipc_timer_addr, 0));
		writel(TIMER_ONESHOT_MODE, timerx_ctrl(test_ipc_timer_addr, 0));
		/* config timer1 */
		writel(TIMER_VALUE, timerx_load(test_ipc_timer_addr, 1));
		writel(TIMER_ONESHOT_MODE, timerx_ctrl(test_ipc_timer_addr, 1));
		/* timeout is 1000ms */
		timeout = 100;
		do {
			timeout--;
			/* sleep 10ms for next test cycle */
			usleep_range(10000, 11000);
			timer_irq_0 = readl(timerx_ris(test_ipc_timer_addr, 0));
			timer_irq_1 = readl(timerx_ris(test_ipc_timer_addr, 1));
			if (!timeout) {
				pr_err("%s test timeout\n", __func__);
				return -1;
			}
		} while (timer_irq_0 || timer_irq_1);
	}
	pr_err("%s test end\n", __func__);
	return 0;
}

static int test_get_rproc_id_for_performance(unsigned int objmax)
{
	unsigned int max_index =
		(objmax < ARRAY_SIZE(g_per_rproc_id_table)) && objmax > 0 ?
		objmax : ARRAY_SIZE(g_per_rproc_id_table);

	return g_per_rproc_id_table[max_index - 1];
}

/*
 * test all the core's performance with AP .
 * @objmax   the remote proccessor number to send ipc msg.  Sometimes like
 *           HIFI,ISP,etc. are in sleepmode and not suitable for pressure test.
 * @type
 *     1: sync send
 *     0: async send
 *
 * @return    0:success   not 0:failed
 */
int test_rproc_performance(unsigned char objmax, int type)
{
	union ipc_data *msg = NULL;
	int rproc_id;
	unsigned int ret = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	struct timespec64 *ptimespec = NULL;
#else
	struct timespec *ptimespec = NULL;
#endif

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg )
		return -1;
	ptimespec = kzalloc(sizeof(*ptimespec) * PERFORMANC_TIMES, GFP_KERNEL);
	if (!ptimespec) {
		kfree(msg);
		return -1;
	}

	/* we don't care the msg's content */
	if (memset_s((void *)msg, sizeof(*msg), 0xFF, sizeof(*msg)) != EOK)
		pr_err("%s: memset failed\n", __func__);

	rproc_id = test_get_rproc_id_for_performance(objmax);
	if (type == 1) {
		ret |= rproc_performance_cnt(
			ptimespec, rproc_id, msg, MAX_MAIL_SIZE, SYNC_SEND);
		show_rproc_performance(ptimespec, rproc_id, SYNC_SEND);
	} else if (type == 0) {
		ret |= rproc_performance_cnt(
			ptimespec, rproc_id, msg, MAX_MAIL_SIZE, ASYNC_SEND);
		show_rproc_performance(ptimespec, rproc_id, ASYNC_SEND);
	} else {
		ret |= rproc_performance_cnt(
			ptimespec, rproc_id, msg, MAX_MAIL_SIZE, SYNC_SEND);
		show_rproc_performance(ptimespec, rproc_id, SYNC_SEND);
		ret |= rproc_performance_cnt(
			ptimespec, rproc_id, msg, MAX_MAIL_SIZE, ASYNC_SEND);
		show_rproc_performance(ptimespec, rproc_id, ASYNC_SEND);
	}

	if (ret)
		pr_err("rproc_performance and pressure failed\n");
	else
		pr_err("rproc_performance and pressure pass\n");

	kfree(msg);
	kfree(ptimespec);

	return (int)ret;
}

/*
 * test npu ipc mailbox channel
 * @is_async
 *      1: test npu suspend
 *      0: npu ts test msg send
 */
void npu_ipc_test(int is_async)
{
	int rproc_tscpu = IPC_NPU_ACPU_NPU_MBX2;

	pr_err("****npu_ipc_test1 qqq. is_async=%d\n", is_async);
	if (is_async == 1)
		test_async_send_to_tscpu_suspend(&rproc_tscpu);
	else
		test_async_send_to_tscpu(
			IPC_NPU_ACPU_NPU_MBX2, IPC_NPU_NPU_ACPU_MBX1);
}

static int read_rproc_id(struct mailbox_device *mbox,
	struct device_node *mbox_node)
{
	int ret;
	const char *rproc_id_name = NULL;

	ret = of_property_read_string(mbox_node, "rproc", &rproc_id_name);
	if (ret || !rproc_id_name) {
		pr_err("Miss 'rproc' property\n");
		return -ENODEV;
	}

	mbox->rproc_id = ipc_find_rproc_id(rproc_id_name);
	if (mbox->rproc_id < 0) {
		pr_err("Invalid rproc:[%s] property\n", rproc_id_name);
		return -ENODEV;
	}

	return 0;
}

static int read_mbox_node(
	struct mailbox_device *mbox, struct device_node *mbox_node)
{
	unsigned int output[IPC_MDEV_FUNC] = {0};
	int ret;

	ret = of_property_read_u32(mbox_node, "used", &mbox->used);
	if (ret) {
		pr_err("%s has no tag <used>\n", mbox_node->name);
		mbox->used = MAILBOX_NO_USED;
	}

	ret = of_property_read_u32(mbox_node, "src_bit", (u32 *)&(mbox->src));
	if (ret) {
		pr_err("Miss 'src_bit' property\n");
		return ret;
	}

	ret = of_property_read_u32(mbox_node, "des_bit", (u32 *)&(mbox->dest));
	if (ret) {
		pr_err("Miss 'des_bit' property\n");
		return ret;
	}

	ret = of_property_read_u32(mbox_node, "index", &(mbox->mbox_channel));
	if (ret) {
		pr_err("Miss 'index' property\n");
		return ret;
	}

	ret = of_property_read_u32_array(mbox_node, "func", output, IPC_MDEV_FUNC);
	if (ret){
		pr_err("Miss 'func' property\n");
		return ret;
	}

	mbox->func |= (output[0] ? FAST_MBOX : COMM_MBOX); /* mbox_type */

	mbox->func |= (output[1] ? SOURCE_MBOX : 0); /* is_src_mbox */

	mbox->func |= (output[2] ? DESTINATION_MBOX : 0); /* is_des_mbox */

	mbox->mbox_channel = mbox->mbox_channel % IPC_RESIDUE;
	mbox->mbx_name = mbox_node->name;

	ret = read_rproc_id(mbox, mbox_node);
	if (ret)
		mbox->rproc_id = INVALID_RPROC_ID;

	return 0;
}

static int read_ipc_rproc_names(
	struct test_ipc_device *idev, struct device_node *ipc_node)
{
	const char *rproc_name = NULL;
	struct property *prop = NULL;

	idev->rproc_names_cnt = 0;
	of_property_for_each_string(ipc_node, "rproc_name", prop, rproc_name) {
		idev->rproc_names[idev->rproc_names_cnt++] = rproc_name;
		if (idev->rproc_names_cnt >= RPROC_NUMBER)
			break;
	}

	return 0;
}

static int calc_ipc_lpm_cpu_bit(
	struct test_ipc_device *idev, struct device_node *ipc_node)
{
	unsigned int idx;

	for (idx = 0; idx < idev->rproc_names_cnt; idx++)
		if (strstr(idev->rproc_names[idx], "LPMCU"))
			break;

	if (idx >= RPROC_NUMBER || idx >= idev->rproc_names_cnt) {
		pr_err("not find lpm cpu cfg in:%s\n", ipc_node->name);
		return 0;
	}

	idev->lpm_cpu_bit = idx;

	return 0;
}

static int read_ipc_node(
	struct test_ipc_device *idev, struct device_node *ipc_node)
{
	struct device_node *mbox_node = NULL;
	int idx, ret;
	struct resource res = {0};

	idev->base = of_iomap(ipc_node, 0);
	if (!idev->base) {
		pr_err("%s:iomap error\n", __func__);
		return -1;
	}

	ret = of_address_to_resource(ipc_node, 0, &res);
	if (ret < 0) {
		pr_err("no resource address\n");
		return -ENOMEM;
	}
	idev->addr_phy = res.start;

	ret = of_property_read_u32(ipc_node, "ipc_type", &(idev->ipc_type));
	if (ret) {
		pr_err("read prop 'ipc_type' error %d\n", ret);
		return -1;
	}

	ret = of_property_read_u32(
		ipc_node, "ipc_version", &(idev->ipc_version));
	if (ret) {
		pr_err("read prop 'ipc_version' error %d\n", ret);
		idev->ipc_version = IPC_VERSION_APPLE;
	}

	ret = of_property_read_u32(ipc_node, "unlock_key", &(idev->unlock));
	if (ret) {
		pr_err("prop \"unlock_key\" error %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(ipc_node, "vm_flag", &idev->vm_flag);
	if (ret == 0)
		pr_info("prop vm_flag set");

	ret = read_ipc_rproc_names(idev, ipc_node);
	if (ret)
		return ret;

	ret = calc_ipc_lpm_cpu_bit(idev, ipc_node);
	if (ret)
		return ret;

	idx = 0;
	for_each_child_of_node(ipc_node, mbox_node) {
		ret = read_mbox_node(&idev->mboxes[idx], mbox_node);
		if (ret) {
			pr_err("read mbox:%s node failed:%d\n",
				mbox_node->name, ret);
			continue;
		}

		idx++;
		if (idx >= MAX_MAILBOX_CNT) {
			pr_err("mailbox cnt outof bounds\n");
			break;
		}
	}

	idev->mbx_cnt = idx;
	idev->ipc_name = ipc_node->name;

	return 0;
}

static void show_mbox_dev_info(struct mailbox_device *mbox)
{
	pr_debug("-----------------------------------------------\n");
	pr_debug("mbx_name:%s\n", mbox->mbx_name);
	pr_debug("rproc:%s[%d]\n",
		ipc_get_rproc_name(mbox->rproc_id), mbox->rproc_id);
	pr_debug("rproc_id:%d\n", mbox->rproc_id);
	pr_debug("mbox_channel:%d\n", mbox->mbox_channel);
	pr_debug("used:%d\n", mbox->used);
	pr_debug("src:%d\n", mbox->src);
	pr_debug("dest:%d\n", mbox->dest);
}

static void show_ipc_device_info(struct test_ipc_device *idev)
{
	int i;

	pr_debug("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	pr_debug("ipc_name[type]:%s[%d]\n", idev->ipc_name, idev->ipc_type);
	pr_debug("ipc_version:%d\n", idev->ipc_version);
	pr_debug("lpm_cpu_bit:%u\n", idev->lpm_cpu_bit);
	pr_debug("mbx_cnt:%d\n", idev->mbx_cnt);
	for (i = 0; i < idev->mbx_cnt; i++)
		show_mbox_dev_info(&idev->mboxes[i]);
	pr_debug("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
}

static void prepre_ipc_test_info(void)
{
	int ret, idx;
	struct device_node *ipc_node = NULL;
	struct test_ipc_device *idev = NULL;
	const struct of_device_id mdev_match[] = {
		{
			.compatible = "hisilicon,HiIPCV230"
		}, {
			.compatible = "hisilicon,HiIPCV300"
		}, {}
	};

	idx = 0;
	for_each_matching_node(ipc_node, mdev_match) {
		if (idx >= MAX_IPC_DEVICES_CNT)
			break;

		idev = &g_test_ipc_devices.ipc_devs[idx];
		ret = read_ipc_node(idev, ipc_node);
		if (ret) {
			pr_err("read ipc node failed:ret:%d\n", ret);
			continue;
		}
		show_ipc_device_info(idev);
		idx++;
	}
	pr_debug("xxxxxxxxxxxxxx ipc_dev_cnt=%d xxxxxxxxxxxxxx\n", idx);

	g_test_ipc_devices.ipc_dev_cnt = idx;
}

static int __init test_rproc_init(void)
{
	sema_init(&send_mutex_sema, 1);
	sema_init(&task_mutex_sema, 1);
	prepre_ipc_test_info();
	return 0;
}

static void __exit test_rproc_exit(void)
{
}

module_init(test_rproc_init);
module_exit(test_rproc_exit);

