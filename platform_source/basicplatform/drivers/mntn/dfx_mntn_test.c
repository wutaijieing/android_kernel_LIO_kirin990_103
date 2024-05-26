/*
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define MERROR_LEVEL    1
#define MWARNING_LEVEL  1
#define MNOTICE_LEVEL   1
#define MINFO_LEVEL     1
#define MDEBUG_LEVEL    0
#define MLOG_TAG        "mntn"

#include <linux/module.h>        /* Needed by all modules */
#include <linux/kernel.h>        /* Needed for KERN_ERR */
#include <linux/init.h>          /* Needed for the macros */
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/preempt.h>
#include <linux/smpboot.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#ifdef CONFIG_DFX_NOC_MODID_REGISTER
#include <platform_include/basicplatform/linux/dfx_noc_modid_para.h>
#endif
#include <linux/arm-smccc.h>
#include <platform_include/see/bl31_smc.h>
#include <mntn_public_interface.h>
#include "mntn_log.h"

#define PR_LOG_TAG MNTN_TEST_TAG

static bool rdr_syserr_process_for_ap_loop;

/*******************************************************************************
* Function:            dfx_syserr_loop_enable
* Description:         enable rdr_syserr_process_for_ap loop test function
* Input:               int enable, 0 : disable test !0 : enable test
* Output:              NA
* Return:              NA
* Other:               NA
********************************************************************************/
void dfx_syserr_loop_enable(int enable)
{
	rdr_syserr_process_for_ap_loop = (0 == enable) ? false : true;
}

void dfx_syserr_loop_test(void)
{
	if (rdr_syserr_process_for_ap_loop) {
		do {} while (1);
	}
}

static bool bl31_mntn_tst_en;

/*******************************************************************************
* Function:            bl31_mntn_tst_enable
* Description:         enable bl31 mntn test function
* Input:               int enable, 0 : disable test !0 : enable test
* Output:              NA
* Return:              NA
* Other:               NA
********************************************************************************/
void bl31_mntn_tst_enable(int enable)
{
	bl31_mntn_tst_en = (0 == enable) ? false : true;
}

static noinline u64 __bl31_mntn_tst(u64 type)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MNTN_BB_TST_FID_VALUE, type, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}

u32 bl31_mntn_tst(u32 type)
{
	mlog_e("%s: type %u\n", __func__, type);

	if (bl31_mntn_tst_en) {
		return (u32)__bl31_mntn_tst(type);
	} else {
		return 1;
	}
}

enum WDT_TST_LOCK_TYPE {
	WDT_TST_LOCK_SPINLOCK,
	WDT_TST_LOCK_MUTEX,
	WDT_TST_LOCK_SEM,
	WDT_TST_LOCK_NONE
};

static bool wdt_tst_enabled; /* default value is false */
static spinlock_t wdt_tst_a_spinlock;
static spinlock_t wdt_tst_b_spinlock;
static struct workqueue_struct *wdt_tst_wq_a;
static struct work_struct wdt_tst_w_a;
static struct workqueue_struct *wdt_tst_wq_b;
static struct work_struct wdt_tst_w_b;
static struct hrtimer wdt_irq_tst_hrtimer;
static struct mutex wdt_tst_a_mutex;
static struct mutex wdt_tst_b_mutex;
static int wdt_tst_lock_type;

/*
	WDT PROCESS TEST
*/
static void dfx_wdt_tst_proc_body(void *data)
{
	mlog_e("in while\n");
	asm("b .");
}

/*
	WDT LOCK TEST
*/
#define WDT_TST_LOCK_A() \
do { \
	if (WDT_TST_LOCK_SPINLOCK == wdt_tst_lock_type) { \
		spin_lock_irqsave(&wdt_tst_a_spinlock, flagsa); \
	} else { \
		mutex_lock(&wdt_tst_a_mutex); \
	} \
} while (0)

#define WDT_TST_UNLOCK_A() \
do { \
	if (WDT_TST_LOCK_SPINLOCK == wdt_tst_lock_type) { \
		spin_unlock_irqrestore(&wdt_tst_a_spinlock, flagsa); \
	} else { \
		mutex_unlock(&wdt_tst_a_mutex); \
	} \
} while (0)

#define WDT_TST_LOCK_B() \
do { \
	if (WDT_TST_LOCK_SPINLOCK == wdt_tst_lock_type) { \
		spin_lock_irqsave(&wdt_tst_b_spinlock, flagsb); \
	} else { \
		mutex_lock(&wdt_tst_b_mutex); \
	} \
} while (0)

#define WDT_TST_UNLOCK_B() \
do { \
	if (WDT_TST_LOCK_SPINLOCK == wdt_tst_lock_type) { \
		spin_unlock_irqrestore(&wdt_tst_b_spinlock, flagsb); \
	} else { \
		mutex_unlock(&wdt_tst_b_mutex); \
	} \
} while (0)

static int dfx_wdt_tst_lock_a(void)
{
	unsigned long flagsa = 0;
	unsigned long flagsb = 0;

	mlog_e("lock A to in\n");
	WDT_TST_LOCK_A(); /*lint !e456*/

	mlog_e("lock A in, flags=0x%x\n", (unsigned int)flagsa);
	mdelay(2000);
	mlog_e("lock B to in\n");
	WDT_TST_LOCK_B(); /*lint !e456*/

	mlog_e("during lock AB\n");

	WDT_TST_UNLOCK_A(); /*lint !e456*/
	mlog_e("lock A to out\n");
	WDT_TST_UNLOCK_B(); /*lint !e456*/
	mlog_e("lock A out\n");

	return 0; /*lint !e454*/
}

static int dfx_wdt_tst_lock_b(void)
{
	unsigned long flagsa = 0;
	unsigned long flagsb = 0;

	mlog_e("lock B to in\n");
	WDT_TST_LOCK_B(); /*lint !e456*/
	mlog_e("lock B in, flags=0x%x\n", (unsigned int)flagsb);
	mdelay(2000);
	mlog_e("lock A to in\n");
	WDT_TST_LOCK_A(); /*lint !e456*/

	mlog_e("during lock BA\n");

	WDT_TST_UNLOCK_A(); /*lint !e456*/
	mlog_e("lock B to out\n");
	WDT_TST_UNLOCK_B(); /*lint !e456*/
	mlog_e("lock B out\n");

	return 0; /*lint !e454*/
}

static void dfx_wdt_tst_lock_a_work_func(struct work_struct *work)
{
	dfx_wdt_tst_lock_a();
}

static void dfx_wdt_tst_lock_b_work_func(struct work_struct *work)
{
	dfx_wdt_tst_lock_b();
}

/*
	WDT IRQ TEST
*/
static enum hrtimer_restart dfx_wdt_tst_irq_body(struct hrtimer *timer)
{
	mlog_e("in while\n");
	asm("b .");

	return HRTIMER_NORESTART;
}

static void dfx_wdt_tst_irq_hrtimer(void *data)
{
	mlog_e("cpu = %d", get_cpu());
	put_cpu();
	hrtimer_init(&wdt_irq_tst_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	wdt_irq_tst_hrtimer.function = dfx_wdt_tst_irq_body;
	hrtimer_start(&wdt_irq_tst_hrtimer,
			  ktime_set(1, 0), /* ktime_set(second, msecond) */
			  HRTIMER_MODE_REL | HRTIMER_MODE_PINNED); /*lint !e655*/
}

/*******************************************************************************
* Function:            dfx_mntn_test_enable
* Description:         enable test function
* Input:               int enable, 0 : disable test !0 : enable test
* Output:              NA
* Return:              NA
* Other:               NA
********************************************************************************/
void dfx_mntn_test_enable(int enable)
{
	wdt_tst_enabled = (0 == enable) ? false : true;
}


/*
	test IF
	while in process
*/
int dfx_wdt_tst_proc(int cpu)
{
	if (!wdt_tst_enabled) {
		mlog_i("wdt tst not enabled\n");
		return 1;
	}

	/* cpu should = 0 */
	mlog_i("cpu = %d\n", cpu);
	smp_call_function_single(cpu, dfx_wdt_tst_proc_body, NULL, 0);

	return 0;
}

/*
	test IF
	dead lock
*/
int dfx_wdt_tst_lock(int type)
{
	int cpu_zero = 0;
	int cpu_one = 1;

	if (!wdt_tst_enabled) {
		mlog_i("wdt tst not enabled\n");
		return 1;
	}

	if (!(cpu_online(cpu_zero) && cpu_online(cpu_one))) {
		mlog_i("[%s] not all cpus are online. \n", __func__);
		return 1;
	}

	switch (type) {
	case 0: /* spinlock */
		wdt_tst_lock_type = WDT_TST_LOCK_SPINLOCK;
		mlog_i("spinlock\n");
		break;
	case 1: /* mutex */
		wdt_tst_lock_type = WDT_TST_LOCK_MUTEX;
		mlog_i("mutex\n");
		break;
	default:
		wdt_tst_lock_type = WDT_TST_LOCK_NONE;
		mlog_i("none\n");
		break;
	}

	if (wdt_tst_lock_type != WDT_TST_LOCK_NONE) {
		queue_work_on(cpu_zero, wdt_tst_wq_a, &wdt_tst_w_a);
		queue_work_on(cpu_one, wdt_tst_wq_b, &wdt_tst_w_b);
	}

	return 0;
}

/*
	test IF
	while in irq
*/
int dfx_wdt_tst_irq(int cpu)
{
	if (!wdt_tst_enabled) {
		mlog_e("wdt tst not enabled\n");
		return 1;
	}

	/* cpu should = 0 */
	mlog_e("src cpu = %d, dst cpu = %d\n", get_cpu(), cpu);
	put_cpu();
	smp_call_function_single(cpu, dfx_wdt_tst_irq_hrtimer, NULL, 0);

	return 0;
}


static spinlock_t wdt_tst_lockup_spinlock;
static DEFINE_PER_CPU(struct task_struct *, tst_lockup_task);
static unsigned int wdt_tst_lockup_cpu = 0xff;

static int tst_lockup_should_run(unsigned int cpu)
{
	return true;
}

static void dfx_wdt_tst_hard_lockup(unsigned int cpu)
{
	unsigned long flag;

	if (wdt_tst_lockup_cpu != cpu)
		return;

	mlog_e("lockup cpu = %d\n", cpu);

	spin_lock_irqsave(&wdt_tst_lockup_spinlock, flag);
	do {} while (1);
}

static void dfx_wdt_tst_soft_lockup(unsigned int cpu)
{
	if (wdt_tst_lockup_cpu != cpu)
		return;

	mlog_e("lockup cpu = %d\n", cpu);

	spin_lock(&wdt_tst_lockup_spinlock);
	do {} while (1);
}

static struct smp_hotplug_thread tst_lockup_threads = {
	.store              = &tst_lockup_task,
	.thread_should_run  = tst_lockup_should_run,
	.thread_comm        = "tst_lockup/%u",
};

static void _dfx_wdt_tst_lockup(void *data)
{
	mlog_e("cpu = %d", smp_processor_id());
	wake_up_process(__this_cpu_read(tst_lockup_task));
}

int dfx_wdt_tst_lockup(int cpu, bool is_soft)
{
	struct cpumask cpumask;
	int	           err;

	mlog_e("local cpu = %d, lockup cpu = %d\n", smp_processor_id(), cpu);

	if (is_soft) {
		tst_lockup_threads.thread_fn = dfx_wdt_tst_soft_lockup;
	} else {
		tst_lockup_threads.thread_fn = dfx_wdt_tst_hard_lockup;
	}

	wdt_tst_lockup_cpu = (unsigned int)cpu;
	cpumask_clear(&cpumask);
	cpumask_set_cpu(cpu, &cpumask);

	err = smpboot_register_percpu_thread(&tst_lockup_threads);
	if (err) {
		mlog_e("Failed to create lockup threads\n");
		return 1;
	}

	smp_call_function_single(cpu, _dfx_wdt_tst_lockup, NULL, 0);

	return 0;
}

static int __init dfx_mntn_test_module_init(void)
{
	/*
	   for lock test, spinlock
	 */
	spin_lock_init(&wdt_tst_lockup_spinlock);

	spin_lock_init(&wdt_tst_a_spinlock);
	spin_lock_init(&wdt_tst_b_spinlock);

	wdt_tst_wq_a = create_workqueue("wdt_tst_wq_a");
	if (NULL == wdt_tst_wq_a) {
		mlog_e("wdt_tst_wq_a workqueue create failed\n");
		return -EFAULT;
	}
	INIT_WORK(&wdt_tst_w_a, dfx_wdt_tst_lock_a_work_func);

	wdt_tst_wq_b = create_workqueue("wdt_tst_wq_b");
	if (NULL == wdt_tst_wq_b) {
		mlog_e("wdt_tst_wq_b workqueue create failed\n");
		return -EFAULT;
	}
	INIT_WORK(&wdt_tst_w_b, dfx_wdt_tst_lock_b_work_func);

	/*
	   for lock test, mutex
	 */
	mutex_init(&wdt_tst_a_mutex);
	mutex_init(&wdt_tst_b_mutex);

	/* set wdt_tst_enabled false */
	wdt_tst_enabled = false;

	return 0;
}

static void __exit dfx_mntn_test_module_exit(void)
{
}


module_init(dfx_mntn_test_module_init);
module_exit(dfx_mntn_test_module_exit);

void data_abt_test(void *info)
{
	panic_on_oops = 1;	 /* force panic */

	/* cppcheck-suppress **/
	BUG_ON(1);
}
/*

*/
int dfx_bc_panic_tst_cpu(int cpu)
{
	if (!cpu_online(cpu))
		return 0;

	/* cpu should = 0 */
	mlog_i("cpu = %d\n", cpu);
	smp_call_function_single(cpu, data_abt_test, NULL, 0);
	return 0;
}

u32 dfx_mntn_test_startkernel_panic(void)
{
	u32 *valuep = NULL;

#ifndef CONFIG_DFX_AGING_WDT
	if (check_mntn_switch(MNTN_AGING_WDT)) {
		/* cppcheck-suppress * */
		return (*valuep); /* [false alarm]:trigger a panic for debug, disabled in user version */ /* lint !e413 */
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

static int __init dfx_mntn_test_bc_panic_init(void)
{
	return 0;
}
fs_initcall(dfx_mntn_test_bc_panic_init);

#ifdef CONFIG_DFX_NOC_MODID_REGISTER
extern void noc_modid_register(struct noc_err_para_s noc_err_info, u32 modid);

void dfx_noc_modid_test(u32 i, u32 j, unsigned int m, u32 modid)
{
	struct noc_err_para_s noc_err_v1;
	noc_err_v1.masterid = i;
	noc_err_v1.targetflow = j;
	noc_err_v1.bus = m;
	noc_modid_register(noc_err_v1, modid);
}
#endif

#ifdef CONFIG_DFX_DEBUG_FS
bool g_trigger_startup_panic;
bool g_trigger_shutdown_deadloop;

int mntn_test_startup_panic(void)
{
	u32 *valuep = NULL;
	u32 try_var;

	if (g_trigger_startup_panic) {
		/* cppcheck-suppress * */
		try_var = *valuep; /* [false alarm]:trigger a panic for debug, disabled in user versi on */ /* lint !e413 */
		return try_var;
	}

	return 0;
}

int mntn_test_shutdown_deadloop(void)
{
	if (g_trigger_shutdown_deadloop)
		while (1)
			msleep(500);

	return 0;
}


static int __init early_parse_startup_panic(char *p)
{
	if (!strncmp(p, "enable", strlen("enable")))
		g_trigger_startup_panic = true;
	else
		g_trigger_startup_panic = false;

	return 0;
}

early_param("mntn_startup_panic", early_parse_startup_panic);

static int __init early_parse_shutdown_deadloop(char *p)
{
	if (!strncmp(p, "enable", strlen("enable")))
		g_trigger_shutdown_deadloop = true;
	else
		g_trigger_shutdown_deadloop = false;

	return 0;
}

early_param("mntn_shutdown_deadloop", early_parse_shutdown_deadloop);
#endif

static int __init early_parse_early_ap_panic(char *p)
{
	u32 *valuep = NULL;
	bool trigger_earyl_ap_panic = false;

	if (!strncmp(p, "enable", sizeof("enable")))
		trigger_earyl_ap_panic = true;
	else
		trigger_earyl_ap_panic = false;

	if (trigger_earyl_ap_panic) {
		mlog_e("early_ap_panic happen\n");
		*valuep = 1;
	}

	return 0;
}
early_param("early_ap_panic", early_parse_early_ap_panic);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mntn test");
