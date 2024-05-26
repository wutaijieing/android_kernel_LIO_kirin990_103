/**
 * @file dksm_testcase.c
 * @brief test unit for dksm
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <linux/dma-buf.h>
#include <uapi/linux/sched/types.h>
#include "kunit.h"
#include "stub.h"
#include "dkmd_log.h"
#include "timeline/dkmd_timeline.h"
#include "timeline/dkmd_timeline_listener.h"
#include "fence/dkmd_acquire_fence.h"
#include "fence/dkmd_release_fence.h"
#include "chrdev/dkmd_chrdev.h"
#include "chrdev/dkmd_sysfs.h"
#include "buf_sync/dkmd_buf_sync.h"
#ifdef CONFIG_BUILD_TESTCASE_SUITE
#include "testcases.h"
#endif

static struct dkmd_timeline *g_test_timeline = NULL;
static uint32_t g_test_timeline_data = 0;
static struct task_struct *handle_thread = NULL;
struct raw_notifier_head nofitier_header;

const char test_timeline_name[] = "test_timeline";
static const char *test_timeline_get_name(struct dkmd_timeline_listener *listener)
{
	return test_timeline_name;
}

static bool test_timeline_is_signaled(struct dkmd_timeline_listener *listener, uint64_t timeline_value)
{
	bool ret = timeline_value > listener->pt_value;

	pr_info("timeline is signaled %d!\n", ret);

	return ret;
}

static int32_t test_timeline_handle_signal(struct dkmd_timeline_listener *listener)
{
	uint32_t *test_timeline_data = (uint32_t *)listener->listener_data;

	pr_info("data is handled!\n");

	(*test_timeline_data)++;

	return 0;
}

static void test_timeline_release(struct dkmd_timeline_listener *listener)
{
	uint32_t *test_timeline_data = (uint32_t *)listener->listener_data;

	pr_info("data is set zero!\n");

	*test_timeline_data = 0;
}

static struct dkmd_timeline_listener_ops g_test_timeline_listener_ops = {
	.get_listener_name = test_timeline_get_name,
	.is_signaled = test_timeline_is_signaled,
	.handle_signal = test_timeline_handle_signal,
	.release = test_timeline_release,
};

/**
 * @brief To create an asynchronous process usecases by timeline value
 *
 *  timeline >>>----o-----o-----o-----o-----o-----o-----o-----o------->>>
 *    │             │                 │
 *    │        +----------+      +----------+
 *    o--------│ listener │ ---- │ listener │ ------->   active pt_value
 *    │        +----------+      +----------+                 ^
 *    │             ^                 ^                       │
 *    │            hit               hit                      │
 *    +------------ │ --------------- │ -------------> inc timeline value
 *                  │                 │                       │
 *                  v                 v                       v
 *  kthread >>>-----o-----------------o-----------------------o------->>>
 *               (handle)          (handle)                (handle)
 *
 *  Timeline can differ according to the temporal value increasing value,
 *  the listener to the timeline according to the expected value registered
 *  values corresponding to the event handler.When value reaches the listener
 *  expected value of the timeline, it triggers processing threads processing
 *  function of dealing with the listener registration.
 *
 */
static void test_timeline_with_release(void)
{
	struct dkmd_timeline_listener *listener = NULL;
	struct dkmd_timeline *timeline = g_test_timeline;

	ASSERT_PTR_NOT_NULL(timeline->notifier);
	/* 3. register timeline notifier */
	raw_notifier_chain_register(&nofitier_header, timeline->notifier);

	/* 4. add timeline listener */
	listener = dkmd_timeline_alloc_listener(&g_test_timeline_listener_ops,
		&g_test_timeline_data, dkmd_timeline_get_next_value(timeline) + 1);
	ASSERT_PTR_NOT_NULL(listener);
	dkmd_timeline_add_listener(timeline, listener);

	/* 5. inc timeline value */
	dkmd_timeline_inc_step(timeline);

	/* 6. Meet a certain condition, notify listener */
	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);

	/* 7. check listener value */
	CHECK_EQUAL(dkmd_timeline_get_next_value(timeline), 2);
	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);

	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);
	CHECK_EQUAL(dkmd_timeline_get_next_value(timeline), 4);

	/* release set timeline data to zero */
	CHECK_EQUAL(g_test_timeline_data, 0);

	raw_notifier_chain_unregister(&nofitier_header, timeline->notifier);
}

static void test_timeline_without_release(void)
{
	struct dkmd_timeline_listener *listener = NULL;
	struct dkmd_timeline *timeline = g_test_timeline;

	/* 1. register timeline notifier */
	raw_notifier_chain_register(&nofitier_header, timeline->notifier);

	/* other case */
	g_test_timeline_listener_ops.release = NULL;
	listener = dkmd_timeline_alloc_listener(&g_test_timeline_listener_ops,
		&g_test_timeline_data, dkmd_timeline_get_next_value(timeline) + 1);
	ASSERT_PTR_NOT_NULL(listener);
	dkmd_timeline_add_listener(timeline, listener);

	/* 2. inc timeline value */
	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);

	/* 3. Meet a certain condition, notify listener */
	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);

	/* 4. check listener value */
	CHECK_EQUAL(g_test_timeline_data, 1);

	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);
	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);
	CHECK_EQUAL(dkmd_timeline_get_next_value(timeline), 8);

	/* listener already released, so data is not changed */
	CHECK_EQUAL(g_test_timeline_data, 1);
}

/**
 * @brief Test the following interface
 *    dkmd_buf_sync_lock_dma_buf
 *    dkmd_timeline_inc_step
 *    dkmd_timeline_get_next_value
 *
 */
static void test_timeline_dma_buf(void)
{
	struct dkmd_timeline *timeline = g_test_timeline;
	struct dkmd_dma_buf layer_dma_buf;

	/* 1. register timeline notifier */
	raw_notifier_chain_register(&nofitier_header, timeline->notifier);

	/* other case */
	CHECK_EQUAL(dkmd_buf_sync_lock_dma_buf(timeline, 0), 0);

	/* 2. inc timeline value */
	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);

	/* 3. Meet a certain condition, notify listener */
	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);

	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);
	raw_notifier_call_chain(&nofitier_header, 0x8, &g_test_timeline_data);
	CHECK_EQUAL(dkmd_timeline_get_next_value(timeline), 12);
}

/**
 * @brief Test the following interface
 *    dkmd_acquire_fence_create_fd
 *    dkmd_acquire_fence_wait
 *    dkmd_acquire_fence_signal
 *    dkmd_release_fence_create
 *    dkmd_timeline_inc_step
 *
 */
static void test_timeline_fence(void)
{
	spinlock_t lock;
	int32_t out_fence = 0;
	struct dma_fence *fence = NULL;
	struct dkmd_timeline *timeline = g_test_timeline;

	spin_lock_init(&lock);
	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	out_fence = dkmd_acquire_fence_create_fd(fence, &lock, 1);
	CHECK(out_fence > 0);

	CHECK_EQUAL(dkmd_acquire_fence_wait_fd(out_fence, MSEC_PER_SEC), -ETIMEDOUT);
	CHECK_EQUAL(dkmd_acquire_fence_signal(out_fence), 0);
	ksys_close(out_fence);

	out_fence = dkmd_acquire_fence_create_fd(fence, &lock, 2);
	CHECK(out_fence > 0);
	ksys_close(out_fence);

	out_fence = dkmd_release_fence_create(timeline);
	CHECK(out_fence > 0);
	dkmd_timeline_inc_step(timeline);
	dkmd_timeline_inc_step(timeline);
	CHECK_EQUAL(dkmd_acquire_fence_wait_fd(out_fence, MSEC_PER_SEC), 0);
	CHECK_EQUAL(dkmd_acquire_fence_signal(out_fence), 0);
	ksys_close(out_fence);
}

static ssize_t test_dkmd_chrdev_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return (ssize_t)strlen(buf);
}

static ssize_t test_dkmd_chrdev_debug_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute chrdev_attrs[] = {
	__ATTR(chrdev_debug1, S_IRUGO | S_IWUSR, test_dkmd_chrdev_debug_show, test_dkmd_chrdev_debug_store),
};

static DEVICE_ATTR(chrdev_debug2, 0444, test_dkmd_chrdev_debug_show, test_dkmd_chrdev_debug_store);

/**
 * @brief Test the following interface
 *    dkmd_create_chrdev
 *    dkmd_create_attrs
 *    dkmd_cleanup_attrs
 *    dkmd_sysfs_init
 *    dkmd_sysfs_attrs_append
 *    dkmd_sysfs_create
 *    dkmd_sysfs_remove
 *    dkmd_destroy_chrdev
 *
 */
static void test_dkmd_chrdev(void)
{
	struct dkmd_attr attrs;
	struct dkmd_chrdev chrdev = {
		.name = "test_chrdev",
	};

	dkmd_create_attrs(chrdev.chr_dev, chrdev_attrs, ARRAY_SIZE(chrdev_attrs));
	dkmd_cleanup_attrs(chrdev.chr_dev, chrdev_attrs, ARRAY_SIZE(chrdev_attrs));

	dkmd_sysfs_init(&attrs);

	dkmd_sysfs_attrs_append(&attrs, &dev_attr_chrdev_debug2.attr);
}

ku_test_info dksm_testcase[] = {
	{ "test_timeline_normal", test_timeline_with_release },
	{ "test_timeline_without_release", test_timeline_without_release },
	{ "test_timeline_dma_buf", test_timeline_dma_buf },
	{ "test_timeline_fence", test_timeline_fence },
	{ "test_dkmd_chrdev", test_dkmd_chrdev },
	KU_TEST_INFO_NULL
};

int32_t dksm_testcase_suite_init(void)
{
	struct kthread_worker *kworker = NULL;
	struct sched_param param = {
		.sched_priority = MAX_RT_PRIO - 1,
	};

	pr_info("dksm_testcase device init!\n");
	if (!g_test_timeline) {
		g_test_timeline = (struct dkmd_timeline *)kzalloc(sizeof(struct dkmd_timeline), GFP_KERNEL);
		CHECK_PTR_NOT_NULL(g_test_timeline);
	}

	/* 1. create kthread worker */
	kworker = (struct kthread_worker *)kzalloc(sizeof(struct kthread_worker), GFP_KERNEL);
	CHECK_PTR_NOT_NULL(kworker);
	kthread_init_worker(kworker);
	/* kthread_run instead ? */
	handle_thread = kthread_create(kthread_worker_fn, kworker, "timeline_test");
	CHECK_PTR_NOT_NULL(handle_thread);
	sched_setscheduler_nocheck(handle_thread, SCHED_FIFO, &param);
	wake_up_process(handle_thread);

	/* 2. init timeline */
	g_test_timeline->present_handle_worker = kworker;
	g_test_timeline->listening_isr_bit = 0x8;
	dkmd_timline_init(g_test_timeline, "test_timeline", &g_test_timeline_data);
	CHECK_EQUAL(dkmd_timeline_get_next_value(g_test_timeline), 1);
	CHECK_EQUAL(g_test_timeline_data, 0);

	return 0;
}

int32_t dksm_testcase_suite_clean(void)
{
	g_test_timeline_data = 0;
	kthread_stop(handle_thread);
	if (g_test_timeline) {
		if (g_test_timeline->present_handle_worker) {
			kfree(g_test_timeline->present_handle_worker);
			g_test_timeline->present_handle_worker = NULL;
		}

		kfree(g_test_timeline);
		g_test_timeline = NULL;
	}

	pr_info("dksm_testcase device deinit!\n");
	return 0;
}

#ifndef CONFIG_BUILD_TESTCASE_SUITE
ku_suite_info dksm_testcase_suites[]= {
	{"dksm_testcase_init", dksm_testcase_suite_init, dksm_testcase_suite_clean, dksm_testcase, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int32_t dksm_testcase_init(void)
{
	pr_info("++++++++++++++++++++++++++++ hello, dksm_testcase kunit ++++++++++++++++++++++++!\n");

	run_all_tests(dksm_testcase_suites,"/data/log/dksm_testcase");
	return 0;
}

static void dksm_testcase_exit(void)
{
	pr_info("-------------------------- bye, dksm_testcase kunit ---------------------!\n");
}

module_init(dksm_testcase_init);
module_exit(dksm_testcase_exit);
#endif

MODULE_LICENSE("GPL");