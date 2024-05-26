/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: bl31_hibernate source file
 * Create : 2020/03/06
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/gfp.h>
#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/memory.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/arm-smccc.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <platform_include/see/hisi_bl31_hibernate.h>
#include "platform_include/see/bl31_smc.h"

#define HIBERNATE_RPMB_OK    0x0
#define HIBERNATE_RPMB_BUSY  0x1
#define HIBERNATE_RPMB_ERROR 0x2

static struct hibernate_meminfo_t g_hibernate_mem;
static bool g_store_k_success;

/* addr and size is kernel and bl31 share memory info */
static int bl31_hibernate_smc(u64 function_id, u64 *addr, u64 *size)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 0, 0, 0, 0, 0, 0, 0, &res);
	if (addr && size) {
		*addr = res.a1;
		*size = res.a2;
		pr_info("smc funcid 0x%llx, smem addr = 0x%llx, size = 0x%llx\n",
			function_id, *addr, *size);
	}

	return (int)res.a0;
}

static int get_kmem(size_t size)
{
	if (!g_hibernate_mem.kmem_vaddr) {
		g_hibernate_mem.kmem_vaddr = vmalloc(size);
		if (!g_hibernate_mem.kmem_vaddr) {
			pr_info("bl31_hibernate get kmem fail, try again\n");
			g_hibernate_mem.kmem_vaddr = vmalloc(size);
			if (!g_hibernate_mem.kmem_vaddr) {
				pr_err("bl31_hibernate fail to alloc memory\n");
				return -1;
			}
		}
	}

	return 0;
}

static void free_kmem(void)
{
	if (g_hibernate_mem.kmem_vaddr) {
		memset(g_hibernate_mem.kmem_vaddr, 0, g_hibernate_mem.addr_size);
		vfree(g_hibernate_mem.kmem_vaddr);
		g_hibernate_mem.kmem_vaddr = NULL;
	} else {
		pr_info("kmem is null, no need to free\n");
	}
}

int bl31_hibernate_freeze(void)
{
	int ret;
	u64 addr = 0;
	u64 size = 0;
	/* add for remove pclint-446, if using g_hibernate_mem.paddr in ioremap */
	phys_addr_t paddr;

	pr_info("%s entering.\n", __func__);
	ret = bl31_hibernate_smc(BL31_HIBERNATE_FREEZE, &addr, &size);
	if (ret != 0) {
		pr_err("%s error! smc fail! ret %x\n", __func__, ret);
		return -1;
	}

	if (addr == 0 || size == 0) {
		pr_err("error, smem is zero, addr 0x%llx size 0x%llx\n", addr, size);
		return -1;
	}

	if (g_hibernate_mem.paddr == 0 && g_hibernate_mem.addr_size == 0) {
		g_hibernate_mem.paddr = (phys_addr_t)addr;
		g_hibernate_mem.addr_size = size;
	}
	if ((u64)(g_hibernate_mem.paddr) != addr ||
	    g_hibernate_mem.addr_size != size) {
		pr_err("error, smem info is invalid, Not equal to last time\n");
		return -1;
	}

	if (!g_hibernate_mem.smem_vaddr) {
		paddr = g_hibernate_mem.paddr;
		g_hibernate_mem.smem_vaddr = (void *)ioremap(paddr, size);
		if (!g_hibernate_mem.smem_vaddr) {
			pr_err("%s: remap memory for vaddr failed\n", __func__);
			return -1;
		}
	}

	ret = get_kmem((size_t)g_hibernate_mem.addr_size);
	if (ret != 0) {
		pr_err("%s: error, get kmem fail\n", __func__);
		return -1;
	}

	memcpy(g_hibernate_mem.kmem_vaddr, g_hibernate_mem.smem_vaddr,
	       g_hibernate_mem.addr_size);

	pr_info("%s success\n", __func__);
	return ret;
}

int bl31_hibernate_restore(void)
{
	int ret;

	pr_info("%s entering.\n", __func__);
	if (!g_hibernate_mem.kmem_vaddr || !g_hibernate_mem.smem_vaddr) {
		pr_err("%s: error, params are invalid\n", __func__);
		return -1;
	}

	memcpy(g_hibernate_mem.smem_vaddr, g_hibernate_mem.kmem_vaddr,
	       g_hibernate_mem.addr_size);
	free_kmem();

	ret = bl31_hibernate_smc(BL31_HIBERNATE_RESTORE, NULL, NULL);
	if (ret != 0) {
		pr_err("%s error! smc fail! ret %x\n", __func__, ret);
		return -1;
	}

	pr_info("%s success\n", __func__);
	return ret;
}

struct hibernate_meminfo_t get_bl31_hibernate_mem_info(void)
{
	return g_hibernate_mem;
}

#define READ_STATUS_MAX_CNT 200
/* wait 0.005 * 200 = 1s for device deal */
static int query_status(void)
{
	int ret;
	int status_cnt = 0;

	do {
		status_cnt++;
		mdelay(5);
		ret = bl31_hibernate_smc(BL31_HIBERNATE_QUERY_STATUS, NULL, NULL);
	} while (ret == HIBERNATE_RPMB_BUSY && status_cnt < READ_STATUS_MAX_CNT);

	return ret;
}

int bl31_hibernate_store_k(void)
{
	int ret;

	pr_info("%s entering.\n", __func__);
	g_store_k_success = false;
	ret = bl31_hibernate_smc(BL31_HIBERNATE_STORE_K, NULL, NULL);
	if (ret != 0) {
		pr_err("%s error! smc fail! ret %x\n", __func__, ret);
		return -1;
	}

	ret = query_status();
	if (ret != HIBERNATE_RPMB_OK) {
		pr_err("error! write k fail, status %s\n",
		       (ret == HIBERNATE_RPMB_BUSY) ? "busy" : "error");
		return -1;
	}

	g_store_k_success = true;
	pr_info("%s success\n", __func__);
	return 0;
}

int bl31_hibernate_clean_k(void)
{
	int ret;

	pr_info("%s entering.\n", __func__);
	if (!g_store_k_success) {
		pr_info("bl31_hibernate store k fail. no need to clean\n");
		return 0;
	}
	ret = bl31_hibernate_smc(BL31_HIBERNATE_CLEAN_K, NULL, NULL);
	if (ret != 0) {
		pr_err("%s error! smc fail! ret %x\n", __func__, ret);
		return -1;
	}

	ret = query_status();
	if (ret != HIBERNATE_RPMB_OK) {
		pr_err("error! clean k fail, status %s\n",
		       (ret == HIBERNATE_RPMB_BUSY) ? "busy" : "error");
		return -1;
	}

	g_store_k_success = false;
	pr_info("%s success\n", __func__);
	return 0;
}

int bl31_hibernate_get_k(void)
{
	int ret;

	pr_info("%s entering.\n", __func__);
	ret = bl31_hibernate_smc(BL31_HIBERNATE_GET_K, NULL, NULL);
	if (ret != 0) {
		pr_err("%s error! smc fail! ret %x\n", __func__, ret);
		return -1;
	}

	pr_info("%s success\n", __func__);
	return 0;
}

static int bl31_hibernate_notify(struct notifier_block *nb,
				 unsigned long mode,
				 void *_unused)
{
	int ret = 0;

	switch (mode) {
	case PM_HIBERNATION_PREPARE:
		ret = bl31_hibernate_store_k();
		break;
	case PM_POST_HIBERNATION:
		ret = bl31_hibernate_clean_k();
		break;
	case PM_RESTORE_PREPARE:
		ret = bl31_hibernate_get_k();
		break;
	default:
		break;
	}

	if (ret != 0) {
		pr_err("%s event %lx error\n", __func__, mode);
		return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}

static struct notifier_block bl31_hibernate_nb = {
	.notifier_call = bl31_hibernate_notify,
};

static int __init bl31_hibernate_init(void)
{
	int ret;

	ret = register_pm_notifier(&bl31_hibernate_nb);
	if (ret != 0)
		pr_err("Can't register bl31_hibernate notifier ret %d\n", ret);

	return 0;
}

late_initcall(bl31_hibernate_init);
