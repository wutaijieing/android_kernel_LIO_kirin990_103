/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hhee credtest debugfs
 * Create : 2017/12/6
 */

#include "hhee.h"
#include <asm/compiler.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/atomic.h>
#include <linux/cred.h>
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <platform_include/see/hkip_cred.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "cred_test: " fmt

static char g_local_buf[4096];

struct cred *back_cred;

enum test_case {
	TEST_CASE_COMMIT = 1,
	TEST_CASE_OVERRIDE,
	TEST_CASE_FAST_OVERRIDE,
	TEST_CASE_MAX
};

static char *helper = "echo x > log to start testcase:\n"
		      "echo 1 commit test\n"
		      "echo 2 override test\n"
		      "echo 3 fastoverride_test\n";

static int test_start;

static atomic64_t commit_test_count;
static atomic64_t commit_test_cost;

static atomic64_t override_test_count;
static atomic64_t override_test_cost;

static atomic64_t fastoverride_test_count;
static atomic64_t fastoverride_test_cost;

static int cred_test_show(struct seq_file *m, void *filp)
{
	if (!test_start) {
		seq_printf(m, "%s\n", helper);
		pr_err("%s\n", helper);
		return 0;
	}

	pr_err("commit_test      \t %d times \t cost %ld us\n"
	       "override_test    \t %d times \t cost %ld us\n"
	       "fastoverride_test\t %d times \t cost %ld us\n",
	       atomic64_read(&commit_test_count),
	       atomic64_read(&commit_test_cost),
	       atomic64_read(&override_test_count),
	       atomic64_read(&override_test_cost),
	       atomic64_read(&fastoverride_test_count),
	       atomic64_read(&fastoverride_test_cost));

	seq_printf(m,
		   "commit_test      \t %d times \t cost %ld us\n"
		   "override_test    \t %d times \t cost %ld us\n"
		   "fastoverride_test\t %d times \t cost %ld us\n",
		   atomic64_read(&commit_test_count),
		   atomic64_read(&commit_test_cost),
		   atomic64_read(&override_test_count),
		   atomic64_read(&override_test_cost),
		   atomic64_read(&fastoverride_test_count),
		   atomic64_read(&fastoverride_test_cost));
	return 0;
}

/* following functions are used for log read */
static int cred_test_open(struct inode *inode, struct file *pfile)
{
	return single_open(pfile, cred_test_show, NULL);
}

static void commit_test()
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct cred *new = NULL;
	struct timespec64 tv1, tv2;

	pr_err("%s\n", __func__);
	ktime_get_real_ts64(&tv1);
	new = prepare_creds();
	if (!new) {
		pr_err("prepare_creds error\n");
		return;
	}
	commit_creds(new);
	ktime_get_real_ts64(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000000 +
			     (tv2.tv_nsec - tv1.tv_nsec),
		     &commit_test_cost);
	current_cred();
#else
	struct cred *new = NULL;
	struct timeval tv1, tv2;

	pr_err("%s\n", __func__);
	do_gettimeofday(&tv1);
	new = prepare_creds();
	if (!new) {
		pr_err("prepare_creds error\n");
		return;
	}
	commit_creds(new);
	do_gettimeofday(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000 +
			     (tv2.tv_usec - tv1.tv_usec),
		     &commit_test_cost);
	current_cred();
#endif
}

static void override_test()
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	const struct cred *old = NULL;
	struct cred *new = NULL;
	struct timespec64 tv1, tv2;

	pr_err("%s\n", __func__);
	ktime_get_real_ts64(&tv1);
	new = prepare_creds();
	old = override_creds(new);
	revert_creds(old);
	ktime_get_real_ts64(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000000 +
			     (tv2.tv_nsec - tv1.tv_nsec),
		     &override_test_cost);
	current_cred();
	put_cred(new);
#else
	struct cred *old = NULL;
	struct cred *new = NULL;
	struct timeval tv1, tv2;

	pr_err("%s\n", __func__);
	do_gettimeofday(&tv1);
	new = prepare_creds();
	old = override_creds(new);
	revert_creds(old);
	do_gettimeofday(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000 +
			     (tv2.tv_usec - tv1.tv_usec),
		     &override_test_cost);
	current_cred();
	put_cred(new);
#endif
}

static void fastoverride_test()
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	const struct cred *saved = NULL;
	struct timespec64 tv1, tv2;

	pr_err("%s\n", __func__);
	ktime_get_real_ts64(&tv1);
	saved = hkip_override_creds(back_cred);
	hkip_revert_creds(saved);
	ktime_get_real_ts64(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000000 +
			     (tv2.tv_nsec - tv1.tv_nsec),
		     &fastoverride_test_cost);
	current_cred();
	return;
#else
	const struct cred *saved = NULL;
	struct timeval tv1, tv2;

	pr_err("%s\n", __func__);
	do_gettimeofday(&tv1);
	saved = hkip_override_creds(back_cred);
	hkip_revert_creds(saved);
	do_gettimeofday(&tv2);
	atomic64_add((tv2.tv_sec - tv1.tv_sec) * 1000000 +
			     (tv2.tv_usec - tv1.tv_usec),
		     &fastoverride_test_cost);
	current_cred();
	return;
#endif
}

static ssize_t cred_test_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *offp)
{
	int ret;
	int case_number;
	size_t buf_size;

	buf_size = min(count, (size_t)(sizeof(g_local_buf) - 1));
	if (copy_from_user(g_local_buf, buf, buf_size))
		return -EFAULT;

	ret = kstrtoint(g_local_buf, 0, &case_number);

	if (ret || case_number < 0 || case_number >= TEST_CASE_MAX) {
		return -EPERM;
	}

	test_start = 1;
	switch (case_number) {
	case TEST_CASE_COMMIT:
		atomic64_add(1, &commit_test_count);
		commit_test();
		break;
	case TEST_CASE_OVERRIDE:
		atomic64_add(1, &override_test_count);
		override_test();
		break;
	case TEST_CASE_FAST_OVERRIDE:
		atomic64_add(1, &fastoverride_test_count);
		fastoverride_test();
		break;
	default:
		break;
	}
	return (ssize_t)buf_size;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
const struct proc_ops credtest_fops = {
	.proc_open = cred_test_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_write = cred_test_write,
	.proc_release = single_release,
};
#else
const struct file_operations credtest_fops = {
	.open = cred_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = cred_test_write,
	.release = single_release,
};
#endif

int cred_test_init_debugfs(void)
{
	back_cred = prepare_creds();
	proc_create("cred_test", 0, NULL, &credtest_fops);
	return 0;
}

module_init(cred_test_init_debugfs);
