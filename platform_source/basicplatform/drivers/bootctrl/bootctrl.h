/* Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * FileName: bootctrl.h
 * Description: add proc_boot_partition node for AB partition in kernel.
 * Author: hisi
 * Create: 2020-07-10
 */

#ifndef __BOOT_CTRL_H__
#define __BOOT_CTRL_H__

#include <linux/version.h>

enum BOOT_IOCTL_OPERATION {
	BOOT_IOCTL_SET_ACTIVE = 26,
	BOOT_IOCTL_GET_ACTIVE,
	BOOT_IOCTL_SET_UNBOOTABLE,
	BOOT_IOCTL_IS_SLOT_BOOTABLE,
	BOOT_IOCTL_MARK_SUCCESSFUL,
	BOOT_IOCTL_IS_SLOT_SUCCESSFUL,
	BOOT_IOCTL_MAX
};

#define BOOTCTRLSETACTIVE         _IOWR('B', BOOT_IOCTL_SET_ACTIVE, unsigned long)
#define BOOTCTRLGETACTIVE         _IOWR('B', BOOT_IOCTL_GET_ACTIVE, unsigned long)
#define BOOTCTRLSETUNBOOTABLE     _IOWR('B', BOOT_IOCTL_SET_UNBOOTABLE, unsigned long)
#define BOOTCTRLISSLOTBOOTABLE    _IOWR('B', BOOT_IOCTL_IS_SLOT_BOOTABLE, unsigned long)
#define BOOTCTRLMARKSUCCESSFUL    _IOWR('B', BOOT_IOCTL_MARK_SUCCESSFUL, unsigned long)
#define BOOTCTRLISSLOTSUCCESSFUL  _IOWR('B', BOOT_IOCTL_IS_SLOT_SUCCESSFUL, unsigned long)

#define BOOT_CTRL_PTN_PATH_SIZE 64
#define BOOT_CTRL_RETRIES 3

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
#define SYS_OPEN ksys_open
#define SYS_READ ksys_read
#define SYS_WRITE ksys_write
#define SYS_CLOSE ksys_close
#define SYS_FSYNC ksys_fsync
#else
#define SYS_OPEN sys_open
#define SYS_READ sys_read
#define SYS_WRITE sys_write
#define SYS_CLOSE sys_close
#define SYS_FSYNC sys_fsync
#endif

#endif
