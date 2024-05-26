/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: API for authorization certificate operation.
 * Create : 2018/11/10
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/thread_info.h>
#include <linux/kernel_stat.h>
#include <linux/sched/cputime.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/thread_info.h>
#include <asm/io.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/uidgid.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/arm-smccc.h>
/*
 * This file is designed to test secure-timer&aocc-key function for cc712.
 * Before and after system SR, the secure-timer value should increase
 * and the aocc-key should be same.
 * In adb shell, use the following commands to test:
 * "echo 2 > proc/secure-test" to test secure_timer_SR
 * "echo 3 > proc/secure-test" to test aocc_key_SR
 * "echo 0 > proc/secure-test" to test atf smc cost time
 */

#define ATF_TEST_FID 0xCA000002

static char g_local_buf[4096];

struct test_handle {
	int test_id;
	int (*test_func)(int);
	char func_name[64];
};

/*
 * use a_b_c here
 * and keep it the same order in atf
 */
enum {
	ATF_SMC_TIME = 0,
	ATF_MEM_PERFORMANCE,
	CC_CLOCK_SR,
	CC_KEY_SR,
	ATF_EXT_MEMINFO_TEST,
};

int cc_clock_sr(int data);
int cc_key_sr(int data);
int atf_smc_time(int data);
int atf_ext_meminfo_test(int data);

static struct test_handle handle_arr[] = {
	[ATF_SMC_TIME] = {
		ATF_SMC_TIME, atf_smc_time, "atf_smc_time"
	},
	[CC_CLOCK_SR] = {
		CC_CLOCK_SR, cc_clock_sr, "cc_clock_sr"
	},
	[CC_KEY_SR] = {
		CC_KEY_SR, cc_key_sr, "cc_key_sr"
	},
	[ATF_EXT_MEMINFO_TEST] = {
		ATF_EXT_MEMINFO_TEST, atf_ext_meminfo_test,
		"atf_ext_meminfo_test"
	}
};

noinline int atf_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	struct arm_smccc_res res = {0};

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return (int)res.a0;
}

int cc_clock_sr(int data)
{
	static int clk_sr_time = 0;
	int ret;

	pr_err("%s %d\n", __func__, data);
	if (!(clk_sr_time / 2)) {
		ret = atf_smc(ATF_TEST_FID, CC_CLOCK_SR, 0, 0);
		if (ret)
			pr_err("%s suspend error\n", __func__);
	} else {
		ret = atf_smc(ATF_TEST_FID, CC_CLOCK_SR, 1, 0);
		if (ret)
			pr_err("%s resume error\n", __func__);
	}
	clk_sr_time++;
	return ret;
}

int cc_key_sr(int data)
{
	static int key_sr_time = 0;
	int ret;

	pr_err("%s %d\n", __func__, data);
	if (!(key_sr_time / 2)) {
		ret = atf_smc(ATF_TEST_FID, CC_KEY_SR, 0, 0);
		if (ret)
			pr_err("%s suspend error\n", __func__);
	} else {
		ret = atf_smc(ATF_TEST_FID, CC_KEY_SR, 1, 0);
		if (ret)
			pr_err("%s resume error\n", __func__);
	}
	key_sr_time++;
	return ret;
}

int atf_smc_time(int data)
{
	unsigned long long start_time;
	unsigned long long end_time;
	int ret = 0;
	int i;

	pr_err("%s %d\n", __func__, data);
	start_time = ktime_get_ns();
	for (i = 0; i < 1000; i++)
		ret += atf_smc(ATF_TEST_FID, ATF_SMC_TIME, 0, 0);
	end_time = ktime_get_ns();

	if (ret)
		pr_err("%s return %d\n", __func__, ret);
	if (start_time < end_time)
		pr_err("atf smc cost[%llu] ns\n", (end_time - start_time)/1000);
	else
		pr_err("get time fail, end_time = %llu, start_time = %llu\n",
		       end_time, start_time);
	return ret;
}

int atf_ext_meminfo_test(int data)
{
	int ret;

	pr_err("%s %d\n", __func__, data);
	ret = atf_smc(ATF_TEST_FID, ATF_EXT_MEMINFO_TEST, 0, 0);
	if (ret)
		pr_err("%s atf_smc error, ret = %d\n", __func__, ret);

	pr_err("%s test ok!\n", __func__);
	return ret;
}

static ssize_t sect_write(struct file *filp, const char __user *buf,
			  size_t len, loff_t *ppos)
{
	int ret;
	ssize_t buf_size;
	int test_data;

	if (buf == NULL) {
		pr_err("%s: error, pointer buf is NULL\n", __func__);
		return -EFAULT;
	}
	buf_size = (ssize_t)min(len, (size_t)(sizeof(g_local_buf) - 1));
	if (copy_from_user(g_local_buf, buf, buf_size))
		return -EFAULT;

	ret = kstrtoint(g_local_buf, 0, &test_data);
	if (ret)
		goto out;

	if (test_data < 0 || (unsigned int)test_data >= ARRAY_SIZE(handle_arr)) {
		pr_err("input number %d error\n", test_data);
		goto out;
	}

	if (!handle_arr[test_data].test_func) {
		pr_err("test case %d not implement\n", test_data);
		goto out;
	}

	ret = handle_arr[test_data].test_func(test_data);
	if (ret)
		pr_err("%s: handle_arr return %d\n", __func__, ret);

out:
	return buf_size;
}

static int sect_proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sect_read(struct file *fid, char __user *buf,
			 size_t size, loff_t *ppos)
{
	pr_err("fake read\n");
	return size;
}

static void sect_showinfo(struct seq_file *m, struct file *filp)
{
	seq_printf(m, "current test data %s\n", "secure test");
}

static const struct file_operations sect_proc_fops = {
	.open           = sect_proc_open,
	.write          = sect_write,
	.read           = sect_read,
	.show_fdinfo    = sect_showinfo,
};

static int __init proc_sect_init(void)
{
	proc_create("secure-test", 0666, NULL, &sect_proc_fops);
	return 0;
}
fs_initcall(proc_sect_init);
