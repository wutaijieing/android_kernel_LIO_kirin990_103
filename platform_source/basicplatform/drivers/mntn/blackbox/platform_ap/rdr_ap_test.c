/*
 * ap maintains and testable functions
 *
 * Copyright (c) 2001-2019 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/hardirq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <asm/cacheflush.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/spinlock.h>
#include <platform_include/basicplatform/linux/dfx_universal_wdt.h>
#include "rdr_ap_adapter.h"
#include <platform_include/basicplatform/linux/rdr_dfx_ap_ringbuffer.h>
#include <mntn_subtype_exception.h>
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include "../rdr_inner.h"
#include "../rdr_print.h"
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <linux/version.h>
#include <securec.h>
#include "../diaginfo/bbox_diaginfo.h"

#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/debug.h>
#endif

#define PR_LOG_TAG BLACKBOX_TAG

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define MSI_ARR_NUMBER0 0
#define MSI_ARR_NUMBER1 1
#define MSI_ARR_NUMBER2 2
#define MSI_ARR_NUMBER3 2
#define MSI_ARR_NUMBER4 4
#define MSI_ARR_NUMBER5 5
#define MSI_ARR_NUMBER6 6

#define IPC_MSG_TEST_SIZE1 1
#define IPC_MSG_TEST_SIZE2 2
#define IPC_MSG_TEST_SIZE3 3
#define IPC_MSG_TEST_SIZE4 4
#define IPC_MSG_TEST_SIZE5 5
#define IPC_MSG_TEST_SIZE6 6
#define IPC_MSG_TEST_SIZE7 7

#define DIAG_TEST_INDEX1 1
#define DIAG_TEST_INDEX2 2
#define DIAG_TEST_INDEX3 3
#define DIAG_TEST_INDEX4 4

#define DIAG_TEST_LEN1 30
#define DIAG_TEST_LEN2 40
#define DIAG_TEST_LEN3 28
#define DIAG_TEST_LEN4 35

/*
 * Description: Test to get version information
 * Date:        2015/02/06
 * Modification:Created function
 */
void test_get_product_version(void)
{
	char version[PRODUCT_VERSION_LEN];

	get_product_version(version, PRODUCT_VERSION_LEN);
}

/*
 * Description: test hung task
 * Date:        2015/02/06
 * Modification:Created function
 */
static int test_hung_task_thread_fn(void *data)
{
	int i;

	preempt_disable();
	for (i = 0; i < 1800; i++) { /* test 1800 times */
		set_current_state(TASK_UNINTERRUPTIBLE);
		mdelay(1000); /* sleep 1000ms */
	}
	return 0;
}

/*
 * Description: test hung task
 * Date:        2015/02/06
 * Modification:Created function
 */
int test_hung_task(void)
{
	if (!kthread_run(test_hung_task_thread_fn, NULL,
	     "test_hung_task_thread_fn")) {
		BB_PRINT_ERR(
		       "[%s], kthread_run:test_hung_task_thread_fn failed\n",
		       __func__);
		return -1;
	}
	BB_PRINT_ERR(
	       "[%s], kthread_run:test_hung_task_thread_fn start\n",
	       __func__);
	return 0;
}

/*
 * Description: Display the dying testament module to specify the task call stack information
 * Date:        2015/02/06
 * Modification:Created function
 */
void ap_exch_task_stack_dump(int task_pid)
{
	pid_t exc_pid = (pid_t)task_pid;
	struct task_struct *task = NULL;

	if (exc_pid < 0) {
		pr_info("exch_task_stack_dump:invalid param pid[0x%x]\n",
		       exc_pid);
		return;
	}

	task = find_task_by_vpid(exc_pid);
	if (task)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
		show_stack(find_task_by_vpid(exc_pid), NULL, KERN_ERR);
#else
		show_stack(find_task_by_vpid(exc_pid), NULL);
#endif
	else
		pr_info("exch_task_stack_dump:no such a task pid[0x%x]\n", exc_pid);
}

/*
 * Description: Display the contents of the memory address specified by the dying testament module,
 *              offset is the offset of the exception area to be accessed
 * Date:        2015/02/06
 * Modification:Created function
 */
void ap_exch_buf_show(unsigned int offset, unsigned int size)
{
	unsigned int add_offset;
	const u64 dfx_ap_addr = get_dfx_ap_addr();

	if (!offset || !size)
		pr_info("exch_buf_show:invalid param offset[0x%x] size[%u]\n",
		     offset, size);

	add_offset =
	    (offset / (sizeof(offset))) * sizeof(offset);

	ap_exch_hex_dump((unsigned char *)(uintptr_t)(dfx_ap_addr + add_offset),
			 size, 16); /* print 16 ASCII characters per line */
}

/*
 * Description: Print buf information, print characters if it is ASCII characters
 * input:       buf to be dumped address
 *              size content size
 *              per_row amount of data printed per line
 * Date:        2015/02/06
 * Modification:Created function
 */
void ap_exch_hex_dump(unsigned char *buf, unsigned int size, unsigned char per_row)
{
	int row;
	unsigned int i;
	unsigned char line[140]; /* 140: log buffer line max chars */
	unsigned int left = size;

	if (!buf)
		return;

	per_row = (per_row > 32) ? 32 : per_row; /* print 32 ASCII characters per line */
	if (!per_row)
		per_row = 16; /* print 16 ASCII characters per line */

	pr_info("Dump buffer [%pK] size [%u]:\n", buf, size);

#define to_char(a) (((a) > 9) ? ((a) - 10 + 'A') : ((a) + '0'))
#define is_printable(a) ((a) > 31 && (a) < 127)

	for (row = 0; left; row++) {
		if (memset_s(line, sizeof(line), ' ', sizeof(line)) != EOK) {
			pr_err("[%s:%d]: memset_s err\n]", __func__, __LINE__);
			return;
		}

		for (i = 0; (i < per_row) && left; i++, left--, buf++) {
			unsigned char val = *buf;

			/* The HEX value */
			line[(i * 3)] = to_char(val >> 4); /* 3 characters like 'FF ' */
			line[(i * 3) + 1] = to_char(val & 0x0F);

			/* the ASCII character is printed, otherwise "." is printed */
			line[(per_row * 3) + 2 + i] = /* 3 characters like 'FF ' 2: 3rd char */
			    is_printable(val) ? val : '.';
		}

		/* 3 characters like 'FF ' 2: 3rd char, Add '\0' to the end of the line */
		line[(per_row * 3) + 2 + per_row] = '\0';

		pr_info("[%4u]: %s\n", row * per_row, line);
	}

#undef to_char
#undef is_printable
}

/*
 * Description: Undefined instruction exception
 * Date:        2015/02/06
 * Modification:Created function
 */
int ap_exch_undef(void)
{
	((void(*)(void))0x12345678)();
	return 0;
}

/*
 * Description: Soft interrupt exception
 * Date:        2015/02/06
 * Modification:Created function
 */
int ap_exch_swi(void)
{
#ifdef CONFIG_ARM64
	__asm__("        SVC   0x1234   ");
#else
	__asm__("        SWI   0x1234   ");
#endif
	return 0;
}

/*
 * Description: prefetch abort
 * Date:        2015/02/06
 * Modification:Created function
 */
int ap_exch_pabt(void)
{
	((void(*)(void))0xe0000000)();
	return 0;
}

/*
 * Description: data abort
 * Date:        2015/02/06
 * Modification:Created function
 */
int ap_exch_dabt(void)
{
	*(int *)0xa0000000 = 0x12345678;
	return 0;
}

/*
 * Description: get time difference to collect the diff between and bboxslice
 * Date:        2015/07/10
 * Modification:Created function
 */
void ap_get_curtime_slice_diff(void)
{
	u64 diff_curtime, diff_slice;
	struct rdr_arctimer_s rdr_arctimer;

	diff_curtime = dfx_getcurtime();
	diff_slice = get_32k_abs_timer_value();

	rdr_arctimer_register_read(&rdr_arctimer);
	rdr_archtime_register_print(&rdr_arctimer, false);

	pr_info("printk_curtime is %llu, 32k_abs_timer_value is %llu\n",
	       diff_curtime, diff_slice);
}

#define dfx_bb_mod_tst(x) (DFX_BB_MOD_RANDOM_ALLOCATED_END + 1 + (x))

static bool g_bbox_tst_enabled = false;
static u32 g_bbox_tst_exce_size;

enum TEST_MODID {
	DFX_BB_MOD_TST_AP = dfx_bb_mod_tst(RDR_AP),
	DFX_BB_MOD_TST_CP = dfx_bb_mod_tst(RDR_CP),
	DFX_BB_MOD_TST_TEEOS = dfx_bb_mod_tst(RDR_TEEOS),
	DFX_BB_MOD_TST_HIFI = dfx_bb_mod_tst(RDR_HIFI),
	DFX_BB_MOD_TST_LPM3 = dfx_bb_mod_tst(RDR_LPM3),
	DFX_BB_MOD_TST_IOM3 = dfx_bb_mod_tst(RDR_IOM3),
	DFX_BB_MOD_TST_ISP = dfx_bb_mod_tst(RDR_ISP),
	DFX_BB_MOD_TST_IVP = dfx_bb_mod_tst(RDR_IVP),
	DFX_BB_MOD_TST_EMMC = dfx_bb_mod_tst(RDR_EMMC),
	DFX_BB_MOD_TST_MODEMAP = dfx_bb_mod_tst(RDR_MODEMAP),
	DFX_BB_MOD_TST_CLK = dfx_bb_mod_tst(RDR_CLK),
	DFX_BB_MOD_TST_REGULATOR = dfx_bb_mod_tst(RDR_REGULATOR),
	DFX_BB_MOD_TST_BFM = dfx_bb_mod_tst(RDR_BFM),
	DFX_BB_MOD_TST_GENERAL_SEE = dfx_bb_mod_tst(RDR_GENERAL_SEE),
};

static struct rdr_exception_info_s g_bbox_test_exc_info[] = {
	{
		.e_modid            = (u32)DFX_BB_MOD_TST_AP,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_AP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_AP,
		.e_reset_core_mask  = 0,
		.e_from_core        = RDR_AP,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = AP_S_ABNORMAL,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags   = RDR_SAVE_CONSOLE_MSG,
		.e_from_module      = "AP",
		.e_desc             = "bbox test,ap reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_CP,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_CP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_CP,
		.e_reset_core_mask  = RDR_CP,
		.e_from_core        = RDR_CP,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = CP_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "CP",
		.e_desc             = "bbox test,cp reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_TEEOS,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_TEEOS,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_TEEOS,
		.e_reset_core_mask  = RDR_TEEOS,
		.e_from_core        = RDR_TEEOS,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = TEE_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "TEEOS",
		.e_desc             = "bbox test,teeos reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_HIFI,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_HIFI,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HIFI,
		.e_reset_core_mask  = RDR_HIFI,
		.e_from_core        = RDR_HIFI,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = SOCHIFI_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HIFI",
		.e_desc             = "bbox test,sochifi reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_LPM3,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_LPM3,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_LPM3,
		.e_reset_core_mask  = RDR_LPM3,
		.e_from_core        = RDR_LPM3,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = LPM3_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "LPM3",
		.e_desc             = "bbox test,lpm3 reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_IOM3,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_IOM3,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_IOM3,
		.e_reset_core_mask  = RDR_IOM3,
		.e_from_core        = RDR_IOM3,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = IOM3_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "IOM3",
		.e_desc             = "bbox test,sensorhub reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_ISP,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_ISP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_ISP,
		.e_reset_core_mask  = RDR_ISP,
		.e_from_core        = RDR_ISP,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = ISP_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "ISP",
		.e_desc             = "bbox test,camera reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_IVP,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_IVP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_IVP,
		.e_reset_core_mask  = RDR_IVP,
		.e_from_core        = RDR_IVP,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = IVP_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "IVP",
		.e_desc             = "bbox test,IVP reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_EMMC,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_EMMC,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NOW,
		.e_notify_core_mask = RDR_AP,
		.e_reset_core_mask  = RDR_AP,
		.e_from_core        = RDR_EMMC,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = MMC_S_EXCEPTION,
		.e_exce_subtype     = MMC_EXCEPT_COLDBOOT,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "EMMC",
		.e_desc             = "bbox test,emmc reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_MODEMAP,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_MODEMAP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_MODEMAP,
		.e_reset_core_mask  = RDR_MODEMAP,
		.e_from_core        = RDR_MODEMAP,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = CP_S_MODEMAP,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "CP",
		.e_desc             = "bbox test,modem ap reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_BFM,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_BFM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_BFM,
		.e_reset_core_mask  = RDR_BFM,
		.e_from_core        = RDR_BFM,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = BFM_S_FW_BOOT_FAIL,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "BFM",
		.e_desc             = "bbox test,bfm reset system",
	}, {
		.e_modid            = (u32)DFX_BB_MOD_TST_GENERAL_SEE,
		.e_modid_end        = (u32)DFX_BB_MOD_TST_GENERAL_SEE,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_GENERAL_SEE,
		.e_reset_core_mask  = RDR_GENERAL_SEE,
		.e_from_core        = RDR_GENERAL_SEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = GENERAL_SEE_S_EXCEPTION,
		.e_exce_subtype     = 0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "GENERAL_SEE",
		.e_desc             = "bbox test,general_see reset system",
	},
};

u32 bbox_test_get_exce_size(void)
{
	u32 size = 0;

	if (g_bbox_tst_enabled == true)
		size = ARRAY_SIZE(g_bbox_test_exc_info);

	return size;
}

int bbox_test_registe_exception(void)
{
	u32 i, ret;

	if (g_bbox_tst_enabled == false) {
		pr_err("%s bbox_tst did not enabled\n", __func__);
		return -1;
	}

	for (i = 0; i < g_bbox_tst_exce_size; i++) {
		ret = rdr_register_exception(&g_bbox_test_exc_info[i]);
		if (ret == 0) {
			pr_err("%s register exception %u fail\n", __func__, i);
			return -1;
		}
	}

	return 0;
}

int bbox_test_unregiste_exception(void)
{
	u32 i;
	int ret;

	if (g_bbox_tst_enabled == false) {
		pr_err("%s bbox_tst did not enabled\n", __func__);
		return -1;
	}

	for (i = 0; i < g_bbox_tst_exce_size; i++) {
		ret = rdr_unregister_exception(g_bbox_test_exc_info[i].e_modid);
		if (ret < 0) {
			pr_err("%s unregister exception %u fail\n", __func__, i);
			return -1;
		}
	}

	return 0;
}

void bbox_test_enable(int enable)
{
	if (enable == 1) {
		g_bbox_tst_enabled = true;
		g_bbox_tst_exce_size = bbox_test_get_exce_size();
	} else {
		bbox_test_unregiste_exception();
		g_bbox_tst_enabled = false;
		g_bbox_tst_exce_size = 0;
	}
}

void bbox_test_print_exc(void)
{
	u32 i;

	if (g_bbox_tst_enabled == false)
		pr_err("%s bbox_tst did not enabled\n", __func__);

	pr_info(">>>>>enter blackbox %s: %.4d\n", __func__, __LINE__);

	for (i = 0; i < g_bbox_tst_exce_size; i++) {
		g_bbox_test_exc_info[i].e_desc[STR_EXCEPTIONDESC_MAXLEN - 1] = '\0';
		pr_err(" modid:          [0x%x]\n", g_bbox_test_exc_info[i].e_modid);
		pr_err(" exce_type:      [0x%x]\n", g_bbox_test_exc_info[i].e_exce_type);
		pr_err(" exce_subtype:   [0x%x]\n", g_bbox_test_exc_info[i].e_exce_subtype);
		pr_err(" desc:           [%s]\n",   g_bbox_test_exc_info[i].e_desc);
		pr_err("==========[%.2d]-e n d==========\n", i);
	}

	pr_info("<<<<< exit blackbox %s: %.4d\n", __func__, __LINE__);
}

#ifdef CONFIG_DFX_BB_DIAGINFO
static struct bbox_diaginfo_test_s {
	unsigned int index;
	unsigned int len;
	char *msg;
} g_diag[] = {
	{
		.index = DIAG_TEST_INDEX1,
		.len = DIAG_TEST_LEN1,
		.msg = "bbox diaginfo test1",
	}, {
		.index = DIAG_TEST_INDEX2,
		.len = DIAG_TEST_LEN2,
		.msg = "bbox diaginfo test2",
	}, {
		.index = DIAG_TEST_INDEX3,
		.len = DIAG_TEST_LEN3,
		.msg = "bbox diaginfo test3",
	}, {
		.index = DIAG_TEST_INDEX4,
		.len = DIAG_TEST_LEN4,
		.msg = "bbox diaginfo test4",
	},
};

int bbox_diaginfo_test(unsigned int err_id)
{
	struct diaginfo_map *diag = NULL;
	unsigned int i, map_size, errid;

	map_size = ARRAY_SIZE(g_diag);
	diag = get_diaginfo_map(err_id);
	if (!diag)
		pr_err("%s(): wrong errid %u\n", __func__, err_id);

	errid = err_id - SoC_DIAGINFO_START + 1;
	for (i = 0; i < map_size; i++) {
		if (errid == g_diag[i].index)
			break;
	}
	pr_info("%s(): index is:%u\n", __func__, i);

	if (i >= map_size) {
		pr_err("%s(), err_id did not match\n", __func__);
		return -1;
	}

	bbox_diaginfo_record(err_id, NULL, g_diag[i].msg);
	return 0;
}

int mntn_ipc_msg_test(u32 msg1, u32 msg2, u32 msg3, u32 msg4, u32 msg5, u32 msg6, u32 msg7)
{
	unsigned int msi[MAX_MAIL_SIZE] = {'\0'};
	unsigned int size = MAX_MAIL_SIZE;

	/* 0~6 IPC msg test */
	if (size >= IPC_MSG_TEST_SIZE1)
		msi[MSI_ARR_NUMBER0] = msg1;
	if (size >= IPC_MSG_TEST_SIZE2)
		msi[MSI_ARR_NUMBER1] = msg2;
	if (size >= IPC_MSG_TEST_SIZE3)
		msi[MSI_ARR_NUMBER2] = msg3;
	if (size >= IPC_MSG_TEST_SIZE4)
		msi[MSI_ARR_NUMBER3] = msg4;
	if (size >= IPC_MSG_TEST_SIZE5)
		msi[MSI_ARR_NUMBER4] = msg5;
	if (size >= IPC_MSG_TEST_SIZE6)
		msi[MSI_ARR_NUMBER5] = msg6;
	if (size >= IPC_MSG_TEST_SIZE7)
		msi[MSI_ARR_NUMBER6] = msg7;

	pr_err("%s(), size is %u\n", __func__, size);

	mntn_ipc_msg_nb(msi, MAX_MAIL_SIZE);

	return 0;
}

static int panic_diaginfo_test_notify(struct notifier_block *nb, unsigned long event, void *buf)
{
	pr_err("%s() begin\n", __func__);
	bbox_diaginfo_record(CPU_UP_FAIL, NULL, "diaginfo in panic lastkmsg");
	return 0;
}

static struct notifier_block acpu_panic_diaginfo_block = {
	.notifier_call = panic_diaginfo_test_notify,
	.priority = INT_MAX,
};

int panic_diaginfo_test(void)
{
	atomic_notifier_chain_register(&panic_notifier_list, &acpu_panic_diaginfo_block);
	pr_err("%s() begin\n", __func__);
	return 0;
}
/* diaginfo test end */
#endif

MODULE_LICENSE("GPL");

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
