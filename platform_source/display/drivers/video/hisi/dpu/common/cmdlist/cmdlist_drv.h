/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef __CMDLIST_DRV_H__
#define __CMDLIST_DRV_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regulator/driver.h>
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mount.h>
#include <linux/genalloc.h>

#include "cmdlist.h"

#define DEV_NAME_CMDLIST "[DTE I] cmdlist"
#define DTS_COMP_CMDLIST "hisilicon,dpu_cmdlist"

#undef pr_fmt
#define pr_fmt(fmt)  DEV_NAME_CMDLIST": " fmt

/* 128 bit */
#define ONE_ITEM_SIZE (128 / 8)

/* One cmdlist node is inlcude cmd_header(size: 16Byte) and cmd_item[0..N](size: 16Byte)
 * N: tatal_items[13:0] = 0x3FFF = 16383
 */
#define ITEMS_MAX_NUM (0x3FFF)

extern bool dpu_cmdlist_enable;

enum cmdlist_arch_version {
	GENERIC_DEVICETREE_CMDLIST,
	CMDLIST_MAX,
};

struct cmdlist_match_data {
	enum cmdlist_arch_version version;
	void (* of_device_setup_impl) (struct platform_device *pdev);
	void (* of_device_release_impl) (struct platform_device *pdev);
};

struct cmdlist_private {
	/* platform device */
	struct platform_device *pdev;

	/* link devicetree dev */
	struct device *of_dev;

	/* chrdev info */
	uint32_t chr_major;
	struct class *chr_class;
	struct device *chr_dev;

	/* cmdlist dev info */
	void __iomem *dev_reg_base;

	/* dma coherent info */
	size_t sum_pool_size;
	void *pool_vir_addr;
	dma_addr_t pool_phy_addr;

	struct gen_pool *memory_pool;
	int device_initailed;
	struct semaphore sem;
};

#endif