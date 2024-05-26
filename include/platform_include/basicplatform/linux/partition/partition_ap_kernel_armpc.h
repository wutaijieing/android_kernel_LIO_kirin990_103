/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2010-2019. All rights reserved.
 * Description: partition table for armpc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PARTITION_AP_KERNEL_ARMPC_H__
#define __PARTITION_AP_KERNEL_ARMPC_H__

#define PTN_ERROR (-1)

enum partition_boot_type {
	XLOADER_A = 1,
	XLOADER_B = 2,
	ERROR_BOOT_TYPE = 3
};

#define PART_DFX                       "dfx"
#define PART_KERNEL                    "kernel"

typedef int (*SET_BOOT_TYPE_FUNC)(unsigned int ptn_type);
typedef int (*GET_BOOT_TYPE_FUNC)(void);

struct partition_operators {
	SET_BOOT_TYPE_FUNC set_boot_partition_type;
	GET_BOOT_TYPE_FUNC get_boot_partition_type;
};

#ifdef CONFIG_PARTITION_TABLE
void partition_operation_register(struct partition_operators *ops);
int partition_get_storage_type(void);
int partition_set_storage_type(unsigned int ptn_type);
int flash_find_ptn_s(const char *ptn_name, char *bdev_path, unsigned int pblkname_length);
#else
static inline void partition_operation_register(struct partition_operators *ops)
{
}
static inline int partition_get_storage_type(void)
{
	return ERROR_BOOT_TYPE;
}
static inline int partition_set_storage_type(unsigned int ptn_type)
{
	return 0;
}
static inline int flash_find_ptn_s(const char *ptn_name, char *bdev_path,
				   unsigned int pblkname_length)
{
	return 0;
}
#endif

/* this interface is not support for armpc, define here only for bootdevice compile */
static inline int flash_get_ptn_index(const char *bdev_path)
{
	return 0;
}

#endif

