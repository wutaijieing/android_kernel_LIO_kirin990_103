/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: bl31_hibernate test source file
 * Create : 2020/03/06
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/gfp.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/memory.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/thread_info.h>
#include <linux/kernel_stat.h>
#include <linux/thread_info.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/uidgid.h>
#include <linux/kernel.h>
#include <linux/arm-smccc.h>

#include <platform_include/see/hisi_bl31_hibernate.h>
#include "platform_include/see/bl31_smc.h"

#define LOCAL_BUF_LEN 4096
static char g_local_buf[LOCAL_BUF_LEN];

#define HIBERNATE_CRYS_HEADER_SIZE 0x1000 /* 4K */
#define BL31_HIBERNATE_BL31LOG_SIZE 0x21000 /* 132K */
#define BL31_HIBERNATE_GIC_SIZE 0x4000 /* 16K */
#define BL31_HIBERNATE_TEEOS_SIZE 0x4000 /* 16K */
#define BL31_HIBERNATE_REGULATOR_SIZE 0x1000 /* 4K */

enum hibernate_mod_id {
	HIBERNATE_BL31_LOG = 0,
	HIBERNATE_GIC,
	HIBERNATE_TEEOS,
	HIBERNATE_REGULATOR,
	HIBERNATE_MOD_MAX,
};

struct hibernate_mem_header {
	enum hibernate_mod_id mid;
	int data_size; /* cause the callback func return 'int' */
	u32 magic_num;
};

/*
 * test cmd:
 *   0: modify_k1_test
 *   1: modify_enc_data_test
 */
static int bl31_hibernate_test_smc(u64 function_id, u64 cmd)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 0, 0, 0, cmd, 0, 0, 0, &res);

	return (int)res.a0;
}

static int modify_k1_test(void)
{
	int ret;

	/* test cmd num is defined in bl31: 0 is modify k1 test */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 0);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}

	pr_info("modify k1 in bl31 end\n");
	return ret;
}

static int modify_kernel_enc_data_test(void)
{
	int magic = 0x12345678;
	struct hibernate_meminfo_t meminfo = get_bl31_hibernate_mem_info();

	/* Randomly modify some data */
	memcpy(meminfo.kmem_vaddr + (meminfo.addr_size / 2), &magic, 1);
	memcpy(meminfo.kmem_vaddr + (meminfo.addr_size / 4), &magic, 1);
	memcpy(meminfo.kmem_vaddr + (meminfo.addr_size / 4 * 3), &magic, 1);

	pr_info("modify enc data in kernel end\n");
	return 0;
}

static int modify_bl31_enc_data_test(void)
{
	int ret;

	/* test cmd num is defined in bl31: 1 is modify bl31 enc data test */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 1);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}

	pr_info("%s success\n", __func__);
	return ret;
}

static int bl2_crys_params_test(void)
{
	int ret;

	/*
	 * test cmd num is defined in bl31:
	 * 2 is to test bl2 derive crys params randomly
	 */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 2);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}

	pr_info("%s success\n", __func__);
	return ret;
}

static int modify_bl31_log_header_test(void)
{
	int ret;

	/*
	 * test cmd num is defined in bl31:
	 * 3 is to test modify bl31 log header
	 */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 3);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}

	pr_info("%s success\n", __func__);
	return ret;
}

static int read_rpmb_test(void)
{
	int ret;

	/*
	 * test cmd num is defined in bl31:
	 * 4 is to test read&printf rpmb buf and do decrypt enc_k0
	 */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 4);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}

	mdelay(100); /* dealy 100ms wait rpmb deal */

	pr_info("%s success\n", __func__);
	return ret;
}

static int print_magic_test(void)
{
	u64 offset = 0;
	struct hibernate_mem_header *header = NULL;
	struct hibernate_meminfo_t meminfo = get_bl31_hibernate_mem_info();
	if (!meminfo.smem_vaddr) {
		pr_err("error! share mem have not been mapped\n");
		return -1;
	}

	pr_err("bl31 hibernate test: share mem vaddr=0x%llx, size=0x%llx\n",
		(u64)meminfo.paddr, meminfo.addr_size);

	offset += HIBERNATE_CRYS_HEADER_SIZE;
	header = (struct hibernate_mem_header *)((uintptr_t)meminfo.smem_vaddr + offset);
	pr_err("bl31 hibernate test: mid0 maigc = 0x%x, data_size0=0x%x\n",
		header->magic_num, header->data_size);

	offset += (BL31_HIBERNATE_BL31LOG_SIZE);
	header = (struct hibernate_mem_header *)((uintptr_t)meminfo.smem_vaddr + offset);
	pr_err("bl31 hibernate test: mid1 maigc = 0x%x, data_size0=0x%x\n",
		header->magic_num, header->data_size);

	offset += (BL31_HIBERNATE_GIC_SIZE);
	header = (struct hibernate_mem_header *)((uintptr_t)meminfo.smem_vaddr + offset);
	pr_err("bl31 hibernate test: mid2 maigc = 0x%x, data_size0=0x%x\n",
		header->magic_num, header->data_size);

	offset += (BL31_HIBERNATE_TEEOS_SIZE);
	header = (struct hibernate_mem_header *)((uintptr_t)meminfo.smem_vaddr + offset);
	pr_err("bl31 hibernate test: mid3 maigc = 0x%x, data_size0=0x%x\n",
		header->magic_num, header->data_size);
	return 0;
}

static int dummy_bl31_hibernate_freeze(void)
{
	int ret;
	/*
	 * test cmd num is defined in bl31:
	 * 5 is to test dummy_register
	 */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 5);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}
	ret = bl31_hibernate_freeze();
	if (ret != 0) {
		pr_err("bl31_hibernate_freeze fail! ret %x\n", ret);
		return -1;
	}
	pr_info("%s success\n", __func__);
	return ret;
}

static int dummy_bl31_hibernate_restore(void)
{
	int ret;
	/*
	 * test cmd num is defined in bl31:
	 * 5 is to test dummy_register
	 */
	ret = bl31_hibernate_test_smc(BL31_HIBERNATE_TEST, 5);
	if (ret != 0) {
		pr_err("smc fail! ret %x\n", ret);
		return -1;
	}
	ret = bl31_hibernate_restore();
	if (ret != 0) {
		pr_err("bl31_hibernate_restore fail! ret %x\n", ret);
		return -1;
	}
	pr_info("%s success\n", __func__);
	return ret;
}

#define HIBERNATE_TEST_FUNC_NUM 12
static int(*g_handle_arr[HIBERNATE_TEST_FUNC_NUM])(void) = {
	dummy_bl31_hibernate_freeze,
	dummy_bl31_hibernate_restore,
	bl31_hibernate_store_k,
	bl31_hibernate_get_k,
	bl31_hibernate_clean_k,
	modify_k1_test,
	modify_kernel_enc_data_test,
	modify_bl31_enc_data_test,
	print_magic_test,
	bl2_crys_params_test,
	modify_bl31_log_header_test,
	read_rpmb_test
};

static char *g_cmd_str[HIBERNATE_TEST_FUNC_NUM] = {
	"dummy_bl31_hibernate_freeze",
	"dummy_bl31_hibernate_restore",
	"bl31_hibernate_store_k",
	"bl31_hibernate_get_k",
	"bl31_hibernate_clean_k_data",
	"modify_k1_test",
	"modify_kernel_enc_data_test",
	"modify_bl31_enc_data_test",
	"print_magic_test",
	"bl2_crys_params_test",
	"modify_bl31_log_header_test",
	"read_rpmb_test"
};

/* ep: 'echo 0 > /proc/bl31-hibernate-test' */
static ssize_t sect_write(struct file *filp, const char __user *buf,
			  size_t len, loff_t *ppos)
{
	int ret;
	ssize_t buf_size;
	int test_data = 0;

	memset(g_local_buf, 0, sizeof(g_local_buf));
	buf_size = min(len, (size_t)(sizeof(g_local_buf) - 1));
	if (copy_from_user(g_local_buf, buf, buf_size)) {
		pr_err("copy_from_user error\n");
		return -EFAULT;
	}

	/* base can be 10 or 16 and it will automatic distinguish when set 0 */
	ret = kstrtoint(g_local_buf, 0, &test_data);
	if (ret != 0) {
		pr_err("kstrtoint error\n");
		goto out;
	}

	if (test_data < 0 || (u32)test_data >= HIBERNATE_TEST_FUNC_NUM) {
		pr_err("error! test case %d out range\n", test_data);
		goto out;
	} else {
		pr_info("handle_cmd is: %s\n", g_cmd_str[test_data]);
		ret = g_handle_arr[test_data]();
		if (ret != 0) {
			pr_err("handle case %d return %d\n", test_data, ret);
			goto out;
		}
	}

	pr_info("bl31 hibernate test %d end\n", test_data);
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
	return 0;
}

static void sect_showinfo(struct seq_file *m, struct file *filp)
{
	seq_printf(m, "current test data %s\n", "bl31 hibernate test");
}

static const struct file_operations sect_proc_fops = {
	.open        = sect_proc_open,
	.write       = sect_write,
	.read        = sect_read,
	.show_fdinfo = sect_showinfo,
};

static int __init proc_sect_init(void)
{
	proc_create("bl31-hibernate-test", 0666, NULL, &sect_proc_fops);
	return 0;
}
fs_initcall(proc_sect_init);

