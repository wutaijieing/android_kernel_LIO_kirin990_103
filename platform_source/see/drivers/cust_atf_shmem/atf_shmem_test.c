/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Test for atf share memory function.
 * Create : 2022/03/18
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
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
#include <linux/kernel.h>
#include <linux/arm-smccc.h>
#include <atf_shmem.h>
#include <platform_include/see/bl31_smc.h>
/*
 * This file is designed to test atf share memory function.
 * In adb shell, use the following commands to test:
 * "echo 0 > proc/atf_shmem_test" to get atf shmem size
 * "echo 1 > proc/atf_shmem_test" to test share_to_atf_in
 * "echo 2 > proc/atf_shmem_test" to test share_to_atf_out
 * "echo 3 > proc/atf_shmem_test" to test share_to_atf_inout
 * "echo 4 > proc/atf_shmem_test" to test atf_normal_call
 */

#define TO_S_DATA                    0xbe
#define FROM_S_DATA                  0xaf
#define NORMAL_DATA                  0xef

#define FID_COMPONENT_SHARE_IN       make_fid(FID_ATF_FRAMEWORK_TEST, 0x00)
#define FID_COMPONENT_SHARE_OUT      make_fid(FID_ATF_FRAMEWORK_TEST, 0x01)
#define FID_COMPONENT_SHARE_INOUT    make_fid(FID_ATF_FRAMEWORK_TEST, 0x02)
#define FID_COMPONENT_NORMAL_CALL    make_fid(FID_ATF_FRAMEWORK_TEST, 0x03)

static uint8_t g_shmem_addr[4096] = { 0 };
static char g_local_buf[4096] = { 0 };

struct test_handle {
	int test_id;
	int (*test_func)(int);
	char func_name[64];
};

enum {
	GET_SHMEM_SIZE = 0,
	SHARE_TO_ATF_IN,
	SHARE_TO_ATF_OUT,
	SHARE_TO_ATF_INOUT,
	ATF_NORMAL_CALL,
	ATF_NORMAL_CALL_ERROR,
};

static int get_shmem_size(int data);
static int share_to_atf_in(int data);
static int share_to_atf_out(int data);
static int share_to_atf_inout(int data);
static int atf_normal_call(int data);
static int atf_normal_call_error(int data);

static struct test_handle handle_arr[] = {
	[GET_SHMEM_SIZE] = {
		GET_SHMEM_SIZE, get_shmem_size, "get_shmem_size"
	},
	[SHARE_TO_ATF_IN] = {
		SHARE_TO_ATF_IN, share_to_atf_in, "share_to_atf_in"
	},
	[SHARE_TO_ATF_OUT] = {
		SHARE_TO_ATF_OUT, share_to_atf_out, "share_to_atf_out"
	},
	[SHARE_TO_ATF_INOUT] = {
		SHARE_TO_ATF_INOUT, share_to_atf_inout, "share_to_atf_inout"
	},
	[ATF_NORMAL_CALL] = {
		ATF_NORMAL_CALL, atf_normal_call, "atf_normal_call"
	},
	[ATF_NORMAL_CALL_ERROR] = {
		ATF_NORMAL_CALL_ERROR, atf_normal_call_error, "atf_normal_call_error"
	},
};

static int get_shmem_size(int data)
{
	int size = (int)get_atf_shmem_size();

	if (size == 0) {
		pr_err("shmem size error\n");
		return -1;
	}
	return 0;
}

static int share_to_atf_in(int data)
{
	u64 size;
	int ret = 0;
	int loop;
	struct arm_smccc_res res = { 0 };

	pr_err("share_to_atf_in test.\n");
	size = get_atf_shmem_size();
	if (size == 0) {
		pr_err("%s get_atf_shmem_size error, size: %llx\n", __func__, size);
		return -EPERM;
	}

	for (loop = 0; loop < (int)size; loop++)
		g_shmem_addr[loop] = TO_S_DATA;

	ret = smccc_with_shmem(FID_COMPONENT_SHARE_IN, SHMEM_IN, (u64)(uintptr_t)g_shmem_addr, size, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_err("%s smccc_with_shmem error, ret: %d\n", __func__, ret);
		return -EPERM;
	}

	pr_err("%s test ok!\n", __func__);
	return 0;
}

static int share_to_atf_out(int data)
{
	u64 size;
	int ret = 0;
	int loop;
	struct arm_smccc_res res = { 0 };

	pr_err("share_to_atf_out test.\n");
	size = get_atf_shmem_size();
	if (size == 0) {
		pr_err("%s get_atf_shmem_size error, size: %llx\n", __func__, size);
		return -EPERM;
	}

	ret = smccc_with_shmem(FID_COMPONENT_SHARE_OUT, SHMEM_OUT, (u64)(uintptr_t)g_shmem_addr, size, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_err("%s smccc_with_shmem error, ret: %d\n", __func__, ret);
		return -EPERM;
	}

	for (loop = 0; loop < (int)size; loop++) {
		if(g_shmem_addr[loop] != FROM_S_DATA) {
			pr_err("%s test error\n", __func__);
			return -EPERM;
		}
	}

	pr_err("%s test ok!\n", __func__);
	return ret;
}

static int atf_normal_call_error(int data)
{
	struct arm_smccc_res res = { 0 };

	/* error test to see log in atf */
	arm_smccc_smc(FID_COMPONENT_SHARE_IN, SHMEM_IN, NORMAL_DATA,
		      NORMAL_DATA, NORMAL_DATA, 0, 0, 0, &res);
	arm_smccc_smc(FID_COMPONENT_SHARE_OUT, SHMEM_OUT, NORMAL_DATA,
		      NORMAL_DATA, NORMAL_DATA, 0, 0, 0, &res);
	arm_smccc_smc(FID_COMPONENT_SHARE_INOUT, SHMEM_INOUT, NORMAL_DATA,
		      NORMAL_DATA, NORMAL_DATA, 0, 0, 0, &res);

	return 0;
}

static int atf_normal_call(int data)
{
	struct arm_smccc_res res = { 0 };
	arm_smccc_smc(FID_COMPONENT_NORMAL_CALL,NORMAL_DATA, NORMAL_DATA,
		      NORMAL_DATA, NORMAL_DATA, 0, 0, 0, &res);

	if (res.a0 == 0) {
		pr_err("atf normal call test okay\n");
		return 0;
	}

	return -1;
}

static int share_to_atf_inout(int data)
{
	u64 size;
	int ret;
	int loop;
	struct arm_smccc_res res = { 0 };

	pr_err("share_to_atf_inout test.\n");
	size = get_atf_shmem_size();
	if (size == 0) {
		pr_err("%s get_atf_shmem_size error, size: %llx\n", __func__, size);
		return -EPERM;
	}

	for (loop = 0; loop < (int)size; loop++)
		g_shmem_addr[loop] = TO_S_DATA;

	ret = smccc_with_shmem(FID_COMPONENT_SHARE_INOUT, SHMEM_INOUT, (u64)(uintptr_t)g_shmem_addr, size, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_err("%s smccc_with_shmem error, ret: %d\n", __func__, ret);
		return -EPERM;
	}

	for (loop = 0; loop < (int)size; loop++) {
		if(g_shmem_addr[loop] != FROM_S_DATA) {
			pr_err("%s out error\n", __func__);
			return -EPERM;
		}
	}

	pr_err("%s test ok!\n", __func__);
	return ret;
}

static ssize_t shmemt_write(struct file *filp, const char __user *buf,
			    size_t len, loff_t *ppos)
{
	int retd;
	ssize_t buf_size;
	int test_data;

	if (buf == NULL) {
		pr_err("%s: error, pointer buf is NULL\n", __func__);
		return -EFAULT;
	}
	buf_size = (ssize_t)min(len, (size_t)(sizeof(g_local_buf) - 1));
	if (copy_from_user(g_local_buf, buf, buf_size))
		return -EFAULT;

	retd = kstrtoint(g_local_buf, 0, &test_data);
	if (retd)
		goto out;

	if (test_data < 0 || (unsigned int)test_data >= ARRAY_SIZE(handle_arr)) {
		pr_err("input number %d error\n", test_data);
		goto out;
	}

	if (!handle_arr[test_data].test_func) {
		pr_err("test case %d not implement\n", test_data);
		goto out;
	}

	retd = handle_arr[test_data].test_func(test_data);
	if (retd)
		pr_err("%s: handle_arr return %d\n", handle_arr[test_data].func_name, retd);

out:
	return buf_size;
}

static int shmemt_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t shmemt_read(struct file *fid, char __user *buf,
			   size_t size, loff_t *ppos)
{
	pr_err("fake read\n");
	return size;
}

static const struct proc_ops shmem_test_proc_fops = {
	.proc_open           = shmemt_open,
	.proc_read           = shmemt_read,
	.proc_write          = shmemt_write,
};

static int __init proc_shmemt_init(void)
{
	proc_create("atf_shmem_test", 0640, NULL, &shmem_test_proc_fops);
	return 0;
}
fs_initcall(proc_shmemt_init);
