/*
 * Copyright (c) Hisilicon technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: RPMB general driver
 * Create: 2012-05-01
 * History: 2019-03-18 structure optimization
 */
#define pr_fmt(fmt) "rpmb:" fmt

#include "vendor_rpmb.h"
#include <linux/version.h>
#include <linux/cpumask.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm-generic/fcntl.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/types.h>
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/types.h>
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
#include <linux/bootmem.h>
#endif
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/compiler.h>
#include <global_ddr_map.h>
#include <linux/time.h>

#include <asm/byteorder.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
#include <asm/compiler.h>
#else
#include <linux/arm-smccc.h>
#include <uapi/linux/sched/types.h>
#endif
#include <asm/hardirq.h>

#include <linux/platform_drivers/rpmb.h>
#include "rpmb_fs.h"
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/bootdevice.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#ifndef CONFIG_DFX_TIME
#include <linux/sched/clock.h>
#endif

#ifdef CONFIG_RPMB_DMD_REPORT
#ifdef CONFIG_LOG_EXCEPTION
#include <log/hiview_hievent.h>
#endif

#define RPMB_DMD_REPORT_ID 925205502
#define RPMB_DMD_TIME_INTERVAL 1000000000
#define RPMB_DMD_MAX_COUNT 5
#define MANFID_NUM 6
#define UFS_VENDOR_TOSHIBA 0x198
#define UFS_VENDOR_SAMSUNG 0x1CE
#define UFS_VENDOR_SKHYNIX 0x1AD
#define UFS_VENDOR_HI1861 0x8B6
#define UFS_VENDOR_MICRON 0x12C
#define UFS_VENDOR_SANDISK 0x145
unsigned int ufs_magic_manfid[MANFID_NUM][2] = { { UFS_VENDOR_TOSHIBA, 0 },
						 { UFS_VENDOR_SAMSUNG, 1 },
						 { UFS_VENDOR_SKHYNIX, 2 },
						 { UFS_VENDOR_HI1861, 3 },
						 { UFS_VENDOR_MICRON, 4 },
						 { UFS_VENDOR_SANDISK, 5 } };
struct rpmb_record_content {
	enum rpmb_state state;
	int errno;
	unsigned int type;
};

static struct rpmb_record_content rpmb_info_record;
#endif

#ifdef CONFIG_RPMB_TIME_DEBUG
static unsigned short g_time_debug = 0;
static u64 g_os_start_time = 0;
static u64 g_os_end_time = 0;
static u64 g_mspc_read_cost_time = 0;
static u64 g_mspc_write_cost_time = 0;
static u64 g_mspc_counter_cost_time = 0;
static u32 g_mspc_read_blks = 0;
static u32 g_mspc_write_blks = 0;
static u32 g_mspc_counter_blks = 0;
static u64 g_os_read_cost_time = 0;
static u64 g_os_write_cost_time = 0;
static u64 g_os_counter_cost_time = 0;
#endif
static u64 g_mspc_start_time = 0;
static u64 g_mspc_end_time = 0;
static u64 g_work_queue_start = 0;
static u64 g_atf_start_time = 0;
static u64 g_atf_end_time = 0;
u64 g_rpmb_ufs_start_time = 0;

struct vendor_rpmb {
	struct rpmb_request *rpmb_request;
	struct device *dev;
	struct block_device *blkdev;
	struct task_struct *rpmb_task;
	int wake_up_condition;
	wait_queue_head_t wait;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct wakeup_source *wake_lock;
#else
	struct wakeup_source wake_lock;
#endif
	enum rpmb_version dev_ver;
	struct rpmb_operation *ops;
};

struct {
	struct rpmb_operation *ops;
	enum bootdevice_type type;
} rpmb_device[BOOT_DEVICE_MAX];

static struct vendor_rpmb vendor_rpmb;
static enum bootdevice_type rpmb_support_device = BOOT_DEVICE_EMMC;
static int rpmb_drivers_init_status = RPMB_DRIVER_IS_NOT_READY;
static int rpmb_device_init_status = RPMB_DEVICE_IS_NOT_READY;
static int rpmb_key_status = KEY_NOT_READY;

static void rpmb_key_status_check(void);

DEFINE_MUTEX(rpmb_counter_lock);

void rpmb_print_frame_buf(const char *name, const void *buf, int len, int format)
{
	pr_err("%s \n", name);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, format, 1, buf, len,
		       false);
}

noinline int atfd_rpmb_smc(u64 _function_id, u64 _arg0, u64 _arg1,
				u64 _arg2)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct arm_smccc_res res;

	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
		0, 0, 0, 0, &res);
	return (int)res.a0;
#else
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;

	asm volatile(__asmeq("%0", "x0")
		     __asmeq("%1", "x1")
		     __asmeq("%2", "x2")
		     __asmeq("%3", "x3")

		     "smc    #0\n"
		     : "+r"(function_id)
		     : "r"(arg0), "r"(arg1), "r"(arg2));

	return (int)function_id;
#endif
}

#ifdef CONFIG_RPMB_DMD_REPORT
static unsigned int get_ufs_magic_manfid()
{
	int i;
	unsigned int ufs_manfid = get_bootdevice_manfid();

	for (i = 0; i < MANFID_NUM; i++) {
		if (ufs_magic_manfid[i][0] == ufs_manfid)
			return ufs_magic_manfid[i][1];
	}

	return MANFID_NUM;
}

static void rpmb_report_hievent(void)
{
	int ret;
	static u64 dmd_time = 0;
	u64 dxf_time = 0;
	static unsigned int dmd_count = 0;
#ifdef CONFIG_LOG_EXCEPTION
	struct hiview_hievent *event = NULL;
#endif

	/* do not upload when time interval less than 1000ms or exceed 5 */
#ifdef CONFIG_DFX_TIME
	dxf_time = dfx_getcurtime();
#else
	dxf_time = local_clock();
#endif
	if ((dxf_time - dmd_time) < RPMB_DMD_TIME_INTERVAL) {
		pr_err("[%s]:time interval less than 1s\n", __func__);
		return;
	}
	if (dmd_count >= RPMB_DMD_MAX_COUNT) {
		pr_err("[%s]: count exceed 5 times\n", __func__);
		return;
	}

#ifdef CONFIG_DFX_TIME
	dmd_time = dfx_getcurtime();
#else
	dmd_time = local_clock();
#endif
	dmd_count++;

#ifdef CONFIG_LOG_EXCEPTION
	event = hiview_hievent_create(RPMB_DMD_REPORT_ID);
	if (event == NULL) {
		pr_err("rpmb create hievent failed\n");
		return;
	}
	hiview_hievent_put_integral(event, "TYPE", rpmb_info_record.type);
	hiview_hievent_put_integral(event, "STATE", rpmb_info_record.state);
	hiview_hievent_put_integral(event, "ERRNO", rpmb_info_record.errno);
	hiview_hievent_put_integral(event, "OCCUR", g_atf_start_time);
	hiview_hievent_put_integral(event, "START", g_mspc_start_time);
	hiview_hievent_put_integral(event, "FINISH", g_mspc_end_time);
	hiview_hievent_put_integral(event, "CALLBACK", g_atf_end_time);
	ret = hiview_hievent_report(event);
	if (ret < 0)
		pr_err("rpmb report hievent failed\n");
	hiview_hievent_destroy(event);
#endif
}

static void rpmb_record_hievent(enum rpmb_state state, int errno)
{
	rpmb_info_record.type = get_ufs_magic_manfid();
	rpmb_info_record.state = state;
	rpmb_info_record.errno = errno;
}
#endif

void rpmb_dump_io_latency(void)
{
	struct rpmb_operation *rpmb_ops = vendor_rpmb.ops;

	/* dump rpmb IO latency */
	if (rpmb_ops->time_stamp_dump)
		rpmb_ops->time_stamp_dump();
}

void mspc_report_rpmb(void)
{
	/* rpmb time record */
	pr_err("[%s]: atf start %llu, start %llu, workqueue start %llu, "
	       "ufs start %llu, end %llu , atf end %llu\n",
	       __func__, g_atf_start_time, g_mspc_start_time,
	       g_work_queue_start, g_rpmb_ufs_start_time, g_mspc_end_time,
	       g_atf_end_time);

	rpmb_dump_io_latency();
#ifdef CONFIG_RPMB_DMD_REPORT
	pr_err("[%s]: devices type = %d\n", __func__, rpmb_info_record.type);
	/* upload DMD report */
	rpmb_report_hievent();
#endif
}

/*
 * Return the member of rpmb_request. This function only is used in
 * rpmb folder, and not for other module.
 */
struct rpmb_request *rpmb_get_request(void)
{
	if (vendor_rpmb.rpmb_request)
		return vendor_rpmb.rpmb_request;
	else
		return NULL;
}

int rpmb_get_dev_ver(enum rpmb_version *ver)
{
	enum rpmb_version version = vendor_rpmb.dev_ver;

	if (!ver)
		return RPMB_ERR_DEV_VER;

	if (version <= RPMB_VER_INVALID || version >= RPMB_VER_MAX) {
		pr_err("Error: invalid rpmb dev ver: 0x%x\n", version);
		return RPMB_ERR_DEV_VER;
	}

	*ver = version;

	return RPMB_OK;
}

/*
 * @ops: driver's rpmb operation pointer.
 * @device_type: enum value of bootdevice_type.
 * This function is used to register ufs/mmc rpmb operation.
 */
int rpmb_operation_register(struct rpmb_operation *ops,
			    enum bootdevice_type device_type)
{
	if (!ops) {
		pr_err("%s: ops is null\n", __func__);
		return -EINVAL;
	}

	if (BOOT_DEVICE_MAX <= device_type) {
		pr_err("%s: device_type:%d exceed max type\n", __func__,
		       device_type);
		return -EDOM;
	}

	if (rpmb_device[device_type].ops) {
		pr_err("%s: ops index:%d has already been register\n",
		       __func__, device_type);
		return -EINVAL;
	}

	rpmb_device[device_type].type = device_type;
	rpmb_device[device_type].ops = ops;

	return 0;
}

static struct rpmb_operation *
rpmb_operation_get(enum bootdevice_type device_type)
{
	if (BOOT_DEVICE_MAX <= device_type) {
		pr_err("%s: device_type:%d exceed max type\n", __func__,
		       device_type);
		return NULL;
	}

	if (!rpmb_device[device_type].ops) {
		pr_err("%s: ops index:%d is not exist\n", __func__,
		       device_type);
		return NULL;
	}

	return rpmb_device[device_type].ops;
}

void rpmb_mspc_time_stamp(enum rpmb_state state, unsigned int blks,
			   struct rpmb_operation *rpmb_ops)
{
	u64 temp_cost_time;
	uint16_t debug_enable = 0;

#ifdef CONFIG_RPMB_TIME_DEBUG
	debug_enable = g_time_debug;
#endif

#ifdef CONFIG_DFX_TIME
	g_mspc_end_time = dfx_getcurtime();
#else
	g_mspc_end_time = local_clock();
#endif
	temp_cost_time = g_mspc_end_time - g_mspc_start_time;
	if (temp_cost_time > RPMB_TIMEOUT_TIME_IN_KERNEL || debug_enable) {
		pr_err("[%s]rpmb access cost time is more than 800ms, "
		       "start[%llu], workqueue start time[%llu], "
		       "ufs start time[%llu],end[%llu], state[%d]!\n",
		       __func__, g_mspc_start_time, g_work_queue_start,
		       g_rpmb_ufs_start_time, g_mspc_end_time, (int32_t)state);
		if (rpmb_ops && rpmb_ops->time_stamp_dump)
			rpmb_ops->time_stamp_dump();
	}

#ifdef CONFIG_RPMB_TIME_DEBUG
	switch (state) {
	case RPMB_STATE_RD:
		if (g_mspc_read_cost_time < temp_cost_time) {
			g_mspc_read_cost_time = temp_cost_time;
			g_mspc_read_blks = blks;
		}
		break;
	case RPMB_STATE_CNT:
	case RPMB_STATE_WR_CNT:
		if (g_mspc_counter_cost_time < temp_cost_time) {
			g_mspc_counter_cost_time = temp_cost_time;
			g_mspc_counter_blks = blks;
		}
		break;
	case RPMB_STATE_WR_DATA:
		if (g_mspc_write_cost_time < temp_cost_time) {
			g_mspc_write_cost_time = temp_cost_time;
			g_mspc_write_blks = blks;
		}
		break;
	default:
		break;
	}

	if (g_time_debug)
		pr_err("[%s]time cost ,operation = [%d]: [%llu]\n", __func__,
		       (int32_t)state, temp_cost_time);
#endif
	return;
}

int wait_mspc_rpmb_request_is_finished(void)
{
	int i;
	struct rpmb_request *request = vendor_rpmb.rpmb_request;

	/* wait 20s timeout (2000 * 10ms) */
	for (i = 0; i < 2000; i++) {
		if (request->rpmb_request_status == 0) {
			break;
		}
		msleep(10);
	}

	if (i == 2000) {
		pr_err("rpmb request get result from device timeout\n");
		return -1;
	}

	return 0;
}

int check_mspc_rpmb_request_is_cleaned(void)
{
	int i;
	struct rpmb_request *request = vendor_rpmb.rpmb_request;

	/* wait 10-15ms timeout (1000 * 10us) */
	for (i = 0; i < 1000; i++) {
		if (request->rpmb_request_status != 0) {
			pr_err("rpmb request is not cleaned in 100-150ms\n");
			return -1;
		}
		usleep_range((unsigned long)10, (unsigned long)15);
	}

	return 0;
}

int32_t mspc_exception_to_reset_rpmb(void)
{
	int i;
	struct rpmb_request *request = vendor_rpmb.rpmb_request;

	request->rpmb_exception_status = RPMB_MARK_EXIST_STATUS;
	for (i = 0; i < 3; i++) {
		if (wait_mspc_rpmb_request_is_finished())
			return -1;
		/* if request finished status is not cleaned in 100-150ms,
		 * we will retry to wait for rpmb request finished
		 */
		if (check_mspc_rpmb_request_is_cleaned())
			pr_err("Front rpmb request is done, but request is "
			       "not cleaned\n");
		else
			break;
	}
	request->rpmb_exception_status = 0;

	return 0;
}

#ifdef CONFIG_RPMB_STORAGE_INTERFACE
static void storage_work_routine(void)
{
	int result;
	int ret;
	struct storage_rw_packet *storage_request =
		(struct storage_rw_packet *)vendor_rpmb.rpmb_request->frame;

	result = storage_issue_work(storage_request);
	if (result)
		pr_err("[%s]: storage request failed, result = %d\n", __func__,
		       result);

	vendor_rpmb.wake_up_condition = 0;

	ret = atfd_rpmb_smc((u64)RPMB_SVC_REQUEST_DONE, (u64)(long)result,
				 (u64)0, (u64)0);
	if (ret)
		pr_err("[%s]: storgae read_write failed 0x%x\n", __func__, ret);
}
#endif

static void rpmb_work_routine(void)
{
	int result = RPMB_ERR_DEV_VER;
	int ret;
	struct rpmb_request *request = vendor_rpmb.rpmb_request;
	struct rpmb_operation *rpmb_ops = vendor_rpmb.ops;

#ifdef CONFIG_PRODUCT_ARMPC
	if (rpmb_key_status == KEY_NOT_READY)
		rpmb_key_status_check();
#endif

	/* No key, we do not send read and write request
	 * (hynix device not support no key to read more than 4K)
	 */
#ifdef CONFIG_RPMB_SET_MULTI_KEY
	if (rpmb_key_status == KEY_NOT_READY &&
	    request->info.state != RPMB_STATE_KEY &&
	    request->key_status != KEY_ALL_READY) {
#else
	if (rpmb_key_status == KEY_NOT_READY) {
#endif
		result = RPMB_ERR_KEY;
		pr_err("[%s]:rpmb key is not ready, do not transfer read and"
		       "write request to device, result = %d\n",
		       __func__, result);
		goto out;
	}

	g_atf_start_time = request->rpmb_atf_start_time;
#ifdef CONFIG_DFX_TIME
	g_work_queue_start = dfx_getcurtime();
#else
	g_work_queue_start = local_clock();
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	__pm_wakeup_event(vendor_rpmb.wake_lock, jiffies_to_msecs(2 * HZ));
#else
	__pm_wakeup_event(&vendor_rpmb.wake_lock, jiffies_to_msecs(2 * HZ));
#endif
	if (rpmb_ops && rpmb_ops->issue_work)
		result = rpmb_ops->issue_work(request);
	if (result)
		pr_err("[%s]:rpmb request done failed, result = %d\n", __func__,
		       result);

out:
	/* mark mspc rpmb request end time */
	rpmb_mspc_time_stamp(request->info.state,
			      request->info.current_rqst.blks, rpmb_ops);
	vendor_rpmb.wake_up_condition = 0;

	ret = atfd_rpmb_smc((u64)RPMB_SVC_REQUEST_DONE, (u64)(long)result,
				 (u64)0, (u64)0);
	if (ret) {
		pr_err("[%s]:state %d, region %d, trans blks %d, "
		       "rpmb request done from bl31 failed, ret 0x%x\n",
		       __func__, request->info.state,
		       request->info.rpmb_region_num,
		       request->info.current_rqst.blks, ret);
		rpmb_print_frame_buf("frame failed",
				     (void *)&request->error_frame, 512, 16);
	}
	g_atf_end_time = request->rpmb_atf_end_time;
#ifdef CONFIG_RPMB_DMD_REPORT
	rpmb_record_hievent(request->info.state, ret);
#endif
}

static void rpmb_request_dispatch(void)
{
#ifdef CONFIG_RPMB_STORAGE_INTERFACE
	struct rpmb_shared_request_status *shared_request_status =
		(struct rpmb_shared_request_status *)vendor_rpmb.rpmb_request;

	if (shared_request_status->general_state == STORAGE_REQUEST)
		storage_work_routine();
	else
#endif
		rpmb_work_routine();
}

/*
 * vendor_rpmb_active - handle rpmb request from ATF
 */
void vendor_rpmb_active(void)
{
#ifdef CONFIG_RPMB_TIME_DEBUG
	if (g_time_debug)
		pr_err("[%s]request start,operation = [%d]\n", __func__,
		       (int32_t)vendor_rpmb.rpmb_request->info.state);

#endif
	/* mark mspc rpmb request start time */
#ifdef CONFIG_DFX_TIME
	g_mspc_start_time = dfx_getcurtime();
#else
	g_mspc_start_time = local_clock();
#endif
	vendor_rpmb.wake_up_condition = 1;
	wake_up(&vendor_rpmb.wait);
}
EXPORT_SYMBOL(vendor_rpmb_active);

#ifdef CONFIG_RPMB_TIME_DEBUG
static int clear_rpmb_max_time(void *data, u64 *val)
{
	struct rpmb_request *request = vendor_rpmb.rpmb_request;
	g_mspc_read_cost_time = 0;
	g_mspc_write_cost_time = 0;
	g_mspc_counter_cost_time = 0;
	g_mspc_read_blks = 0;
	g_mspc_write_blks = 0;
	g_os_read_cost_time = 0;
	g_os_write_cost_time = 0;
	g_os_counter_cost_time = 0;
	g_work_queue_start = 0;
	g_rpmb_ufs_start_time = 0;
	request->rpmb_debug.test_mspc_atf_read_time = 0;
	request->rpmb_debug.test_mspc_atf_write_time = 0;
	request->rpmb_debug.test_mspc_atf_counter_time = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clear_rpmb_time_fops, clear_rpmb_max_time, NULL,
			"%llu\n");
static int read_rpmb_max_time(void *data, u64 *val)
{
	struct rpmb_request *request = vendor_rpmb.rpmb_request;
	pr_err("----------------RPMB------------------\n");
	pr_err("rpmb max read time in ap:%llu, in atf:%llu, blks:%d\n",
	       g_mspc_read_cost_time,
	       request->rpmb_debug.test_mspc_atf_read_time, g_mspc_read_blks);
	pr_err("rpmb max write time in ap:%llu, in atf:%llu, blks:%d\n",
	       g_mspc_write_cost_time,
	       request->rpmb_debug.test_mspc_atf_write_time,
	       g_mspc_write_blks);
	pr_err("rpmb max counter time in ap:%llu, in atf:%llu, blks:%d\n",
	       g_mspc_counter_cost_time,
	       request->rpmb_debug.test_mspc_atf_counter_time,
	       g_mspc_write_blks);
	pr_err("os max read time in ap:%llu\n", g_os_read_cost_time);
	pr_err("os max write time in ap:%llu\n", g_os_write_cost_time);
	pr_err("os max counter time in ap:%llu\n", g_os_counter_cost_time);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(read_rpmb_time_fops, read_rpmb_max_time, NULL,
			"%llu\n");

static void rpmb_time_stamp_debugfs_init(void)
{
	/* create */
	struct dentry *debugfs_rpmb;
	debugfs_rpmb = debugfs_create_dir("rpmb_time", NULL);
	if (!debugfs_rpmb) {
		pr_err("%s debugfs_rpmb is NULL\n", __func__);
		return;
	}
	debugfs_create_file("read_rpmb_time", S_IRUSR, debugfs_rpmb, NULL,
			    &read_rpmb_time_fops);
	debugfs_create_file("clear_rpmb_time", S_IRUSR, debugfs_rpmb, NULL,
			    &clear_rpmb_time_fops);
	/* only for test print */
	debugfs_create_u16("print_enable", S_IRUSR | S_IWUSR, debugfs_rpmb,
			   &g_time_debug);
}
#endif

static void rpmb_key_status_check(void)
{
	int ret = RPMB_ERR_DEV_VER;
	struct rpmb_operation *rpmb_ops = vendor_rpmb.ops;

	if (rpmb_ops && rpmb_ops->key_status)
		ret = rpmb_ops->key_status();

	if (!ret)
		rpmb_key_status = KEY_READY;
	else
		rpmb_key_status = KEY_NOT_READY;
}

u32 get_rpmb_support_key_num(void)
{
	unsigned int i;
	u32 key_num = 0;
	u8 rpmb_region_enable;

	rpmb_region_enable = get_rpmb_region_enable();

	for (i = 0; i < MAX_RPMB_REGION_NUM; i++) {
		if (((rpmb_region_enable >> i) & 0x1))
			key_num++;
	}

	if (key_num == 0)
		pr_err("failed: get_rpmb_support_key_num is zero, rpmb is not support\n");

	return key_num;
}

int get_rpmb_key_status(void)
{
	int result;

	if (get_rpmb_support_key_num() == 1) {
		return rpmb_key_status;
	} else {
		result = atfd_rpmb_smc((u64)RPMB_SVC_MULTI_KEY_STATUS,
					    (u64)0x0, (u64)0x0, (u64)0x0);
		if (result == KEY_NOT_READY || result == KEY_READY) {
			return result;
		} else {
			pr_err("get_rpmb_key_status failed, result = %d\n",
			       result);
			return KEY_REQ_FAILED;
		}
	}
}

int get_rpmb_init_status_quick(void)
{
	return rpmb_drivers_init_status;
}

#ifdef CONFIG_RPMB_KEY_ENABLE
static ssize_t rpmb_key_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t ret;
	struct rpmb_operation *rpmb_ops = vendor_rpmb.ops;
	struct rpmb_request *request = vendor_rpmb.rpmb_request;

	ret = strncmp(buf, "set_key", count);
	if (ret) {
		pr_err("invalid key set command\n");
		return ret;
	}

	if (rpmb_ops && rpmb_ops->key_store)
		ret = rpmb_ops->key_store(dev, request);

	if (!ret) {
		rpmb_key_status = KEY_READY;
		return count;
	}

	return -EINVAL;
}

/* according to rpmb_key_status to check the key is ready */
static ssize_t rpmb_key_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	rpmb_key_status_check();
	if (rpmb_key_status == KEY_READY)
		strncpy(buf, "true", sizeof("true")); /* unsafe_function_ignore: strncpy */
	else
		strncpy(buf, "false", sizeof("false")); /* unsafe_function_ignore: strncpy */
	return (ssize_t)strlen(buf);
}

static DEVICE_ATTR(rpmb_key, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP),
		   rpmb_key_show, rpmb_key_store);
#endif

#define WAIT_INIT_TIMES 3000
int get_rpmb_init_status(void)
{
	int i;
	int32_t ret;
	/*lint -e501*/
	mm_segment_t oldfs = get_fs();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	set_fs(KERNEL_DS);
#else
	set_fs(get_ds());
#endif
	/*lint +e501*/
	for (i = 0; i < WAIT_INIT_TIMES; i++) {
		if (rpmb_support_device == BOOT_DEVICE_EMMC)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
			ret = (int32_t)ksys_access(EMMC_RPMB_BLOCK_DEVICE_NAME, 0);
#else
			ret = (int32_t)sys_access(EMMC_RPMB_BLOCK_DEVICE_NAME, 0);
#endif
		else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
			ret = (int32_t)ksys_access(UFS_RPMB_BLOCK_DEVICE_NAME, 0);
#else
			ret = (int32_t)sys_access(UFS_RPMB_BLOCK_DEVICE_NAME, 0);
#endif
		if (!ret && rpmb_device_init_status)
			break;

		usleep_range((unsigned long)3000, (unsigned long)5000);
	}
	if (i == WAIT_INIT_TIMES) {
		pr_err("wait for device init timeout, ret = %d\n", ret);
		rpmb_drivers_init_status = RPMB_DRIVER_IS_NOT_READY;
	} else {
		rpmb_drivers_init_status = RPMB_DRIVER_IS_READY;
	}

	set_fs(oldfs);
	/* when device is OK and rpmb key is not ready, we will check
	 * the key status
	 */
	if (rpmb_drivers_init_status == RPMB_DRIVER_IS_READY &&
	    rpmb_key_status == KEY_NOT_READY) {
		rpmb_key_status_check();
		pr_err("[%s]:rpmb key status{%d}\n", __func__, rpmb_key_status);
	}

	return rpmb_drivers_init_status;
}

#ifdef CONFIG_RPMB_TIME_DEBUG
void rpmb_os_time_stamp(enum rpmb_op_type operation)
{
	u64 temp_cost_time;
#ifdef CONFIG_DFX_TIME
	g_os_end_time = dfx_getcurtime();
#else
	g_os_end_time = local_clock();
#endif
	temp_cost_time = g_os_end_time - g_os_start_time;
	switch (operation) {
	case RPMB_OP_RD:
		if (g_os_read_cost_time < temp_cost_time)
			g_os_read_cost_time = temp_cost_time;
		break;
	case RPMB_OP_WR_CNT:
		if (g_os_counter_cost_time < temp_cost_time)
			g_os_counter_cost_time = temp_cost_time;
		break;
	case RPMB_OP_WR_DATA:
		if (g_os_write_cost_time < temp_cost_time)
			g_os_write_cost_time = temp_cost_time;
		break;
	default:
		break;
	}
	if (g_time_debug)
		pr_err("[%s]time cost ,operation = [%d]: [%llu]\n", __func__,
		       (int32_t)operation, temp_cost_time);

	return;
}
#endif

int vendor_rpmb_ioctl_cmd(enum func_id id, enum rpmb_op_type operation,
			struct storage_blk_ioc_rpmb_data *storage_data)
{
	int ret = RPMB_ERR_DEV_VER;
	struct rpmb_operation *rpmb_ops = vendor_rpmb.ops;

	if (rpmb_ops && rpmb_ops->is_emulator) {
		pr_err("[%s]: rpmb not support emulator.\n", __func__);
		return RPMB_ERR_IOCTL;
	}

	if (!storage_data)
		return RPMB_INVALID_PARA;

	if (get_rpmb_init_status() == RPMB_DRIVER_IS_NOT_READY) {
		pr_err("[%s]:rpmb init is not ready\n", __func__);
		ret = RPMB_ERR_INIT;
		goto out;
	}
#ifdef CONFIG_RPMB_TIME_DEBUG
	if (g_time_debug)
		pr_err("[%s]request start,operation = [%d]\n", __func__,
		       (int32_t)operation);

	/* mark TEE rpmb request start time */
#ifdef CONFIG_DFX_TIME
	g_os_start_time = dfx_getcurtime();
#else
	g_os_start_time = local_clock();
#endif
#endif
	/* No key, we do not send read and write request(hynix device not
	 * support no key to read more than 4K)
	 */
	if ((rpmb_key_status == KEY_NOT_READY) &&
	    (operation != RPMB_OP_WR_CNT)) {
		ret = RPMB_ERR_KEY;
		pr_err("[%s]:rpmb key is not ready, do not transfer read and "
		       "write request to device, result = %d\n",
		       __func__, ret);
		goto out;
	}

	if (rpmb_ops && rpmb_ops->ioctl)
		ret = rpmb_ops->ioctl(id, operation, storage_data);
	if (ret)
		pr_err("%s: ret %d\n", __func__, ret);

out:
#ifdef CONFIG_RPMB_TIME_DEBUG
	/* mark TEE rpmb request end time */
	rpmb_os_time_stamp(operation);
#endif
	return ret;
}

static irqreturn_t vendor_rpmb_active_handle(int irq, void *dev)
{
	vendor_rpmb_active();
	return IRQ_HANDLED;
}
/* create a virtual device for dma_alloc */
#define SECURE_STORAGE_NAME "secure_storage"
#define RPMB_DEVICE_NAME "rpmb"
static int rpmb_device_init(void)
{
	struct device *pdevice = NULL;
	struct class *rpmb_class = NULL;
	struct device_node *np = NULL;
	struct device_node *irq_np = NULL;
	enum rpmb_version version;
	dma_addr_t rpmb_request_phy;
	unsigned long mem_size;
	phys_addr_t bl31_smem_base =
		ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;
	u32 data[2] = { 0 };
	unsigned int irq;
	int ret;

	rpmb_class = class_create(THIS_MODULE, SECURE_STORAGE_NAME);
	if (IS_ERR(rpmb_class))
		return (int)PTR_ERR(rpmb_class);

	pdevice = device_create(rpmb_class, NULL, MKDEV(0, 0), NULL,
				RPMB_DEVICE_NAME);
	if (IS_ERR(pdevice))
		goto err_class_destroy;

	vendor_rpmb.dev = pdevice;

#ifdef CONFIG_RPMB_KEY_ENABLE
	if (device_create_file(pdevice, &dev_attr_rpmb_key)) {
		pr_err("rpmb error: unable to create sysfs attributes\n");
		goto err_device_destroy;
	}
#endif

	np = of_find_compatible_node(NULL, NULL, "vendor,share-memory-rpmb");
	if (!np) {
		pr_err("rpmb err of_find_compatible_node");
		goto err_device_remove_file;
	}

	if (of_property_read_u32_array(np, "reg", &data[0],
				       (unsigned long)2)) {
		pr_err("rpmb get share_mem_address failed\n");
		goto err_node;
	}

	irq_np = of_find_compatible_node(NULL, NULL, "vendor,rpmb");
	if (irq_np) {
		irq = irq_of_parse_and_map(irq_np, 0);
		ret = devm_request_irq(vendor_rpmb.dev, irq, vendor_rpmb_active_handle,
			IRQF_NO_SUSPEND, RPMB_DEVICE_NAME, NULL);
		if (ret < 0) {
			pr_err("device irq %u request failed %d", irq, ret);
			goto err_node;
		}
	}
	rpmb_request_phy = bl31_smem_base + data[0];
	mem_size = data[1];

	vendor_rpmb.rpmb_request = ioremap_wc(rpmb_request_phy, mem_size);
	if (!vendor_rpmb.rpmb_request)
		goto err_node;

	ret = atfd_rpmb_smc((u64)RPMB_SVC_REQUEST_ADDR,
			rpmb_request_phy,
			(u64)rpmb_support_device, (u64)0x0);
	if (ret) {
		pr_err("rpmb set shared memory address failed, ret=0x%x\n"
						, ret);
		goto err_ioremap;
	}

	version = (enum rpmb_version)atfd_rpmb_smc(
		(u64)RPMB_SVC_GET_DEV_VER, (u64)0, (u64)0, (u64)0);
	if (version <= RPMB_VER_INVALID || version >= RPMB_VER_MAX) {
		pr_err("Error: invalid rpmb dev ver: 0x%x\n", version);
		goto err_ioremap;
	}

	vendor_rpmb.dev_ver = version;

	return 0;

err_ioremap:
	iounmap(vendor_rpmb.rpmb_request);
	vendor_rpmb.rpmb_request = NULL;
err_node:
	of_node_put(np);
err_device_remove_file:
#ifdef CONFIG_RPMB_KEY_ENABLE
	device_remove_file(pdevice, &dev_attr_rpmb_key);
err_device_destroy:
#endif
	device_destroy(rpmb_class, pdevice->devt);
err_class_destroy:
	class_destroy(rpmb_class);
	return -1;
}

static int rpmb_work_thread(void *arg)
{
	set_freezable();
	while (!kthread_should_stop()) {
		wait_event_freezable(vendor_rpmb.wait,
				     vendor_rpmb.wake_up_condition);
		rpmb_request_dispatch();
	}
	return 0;
}

#if defined(CONFIG_PRODUCT_CDC) || defined(CONFIG_PRODUCT_CDC_ACE)
bool rpmb_get_wake_lock_status(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct wakeup_source *wake_lock = vendor_rpmb.wake_lock;
#else
	struct wakeup_source *wake_lock = &vendor_rpmb.wake_lock;
#endif

	if (wake_lock)
		return wake_lock->active;
	else
		return false;
}

#ifdef CONFIG_RPMB_DEBUG_FS
void rpmb_wake_lock_debug_set(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct wakeup_source *wake_lock = vendor_rpmb.wake_lock;
#else
	struct wakeup_source *wake_lock = &vendor_rpmb.wake_lock;
#endif

	if (wake_lock && !wake_lock->active) {
		__pm_stay_awake(wake_lock);
		pr_err("%s: rpmb wake lock debug set\n", __func__);
	}
}

void rpmb_wake_lock_debug_clear(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct wakeup_source *wake_lock = vendor_rpmb.wake_lock;
#else
	struct wakeup_source *wake_lock = &vendor_rpmb.wake_lock;
#endif

	if (wake_lock && wake_lock->active) {
		__pm_relax(wake_lock);
		pr_err("%s: rpmb wake lock debug clear\n", __func__);
	}
}
#endif
#endif

static int __init rpmb_init(void)
{
	struct sched_param param;

	vendor_rpmb.wake_up_condition = 0;
	init_waitqueue_head(&vendor_rpmb.wait);
	vendor_rpmb.rpmb_task =
		kthread_create(rpmb_work_thread, NULL, "rpmb_task");
	param.sched_priority = 1;
	sched_setscheduler(vendor_rpmb.rpmb_task, SCHED_FIFO, &param);
	wake_up_process(vendor_rpmb.rpmb_task);

	rpmb_support_device = get_bootdevice_type();
	vendor_rpmb.ops = rpmb_operation_get(rpmb_support_device);
	if (!vendor_rpmb.ops) {
		pr_err("%s: cannot get rpmb ops\n", __func__);
		return -1;
	}

	if (vendor_rpmb.ops->is_emulator) {
		pr_err("[%s]: rpmb not support emulator.\n", __func__);
		return -1;
	}

	if (rpmb_device_init())
		return -1;
	else
		rpmb_device_init_status = RPMB_DEVICE_IS_READY;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	vendor_rpmb.wake_lock = wakeup_source_register(
			vendor_rpmb.dev, "hisi-rpmb-wakelock");
	if (!vendor_rpmb.wake_lock)
		pr_err("[%s]: rpmb wake_lock init failed\n", __func__);
#else
	wakeup_source_init(&vendor_rpmb.wake_lock,
		       "hisi-rpmb-wakelock");
#endif

#ifdef CONFIG_RPMB_TIME_DEBUG
	rpmb_time_stamp_debugfs_init();
#endif

#ifdef CONFIG_RPMB_DEBUG_FS
	pr_err("%s: rpmb init ok!\n", __func__);
#endif
	return 0;
}
late_initcall(rpmb_init);
MODULE_AUTHOR("qianziye");
MODULE_DESCRIPTION("Vendor Secure RPMB");
MODULE_LICENSE("GPL v2");
