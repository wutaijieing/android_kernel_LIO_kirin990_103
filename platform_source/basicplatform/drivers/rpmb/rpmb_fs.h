/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: storage read-write for common partition
 * Create: 2020-02-14
 */

#ifndef __RPMB_FS_H__
#define __RPMB_FS_H__

#include <linux/sizes.h>
#include <linux/types.h>
#ifdef CONFIG_RPMB_STORAGE_INTERFACE
#include <platform_include/basicplatform/linux/partition/partition_macro.h>
#endif

#define READ_REQ 0x01
#define WRITE_REQ 0x02

#endif
