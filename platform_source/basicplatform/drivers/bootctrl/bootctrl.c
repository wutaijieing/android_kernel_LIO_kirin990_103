/* Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 * FileName: proc_boot_partition.c
 * Description: add proc_boot_partition node for AB partition in kernel.
 * Author: hisi
 * Create: 2020-07-10
 */

#include "bootctrl.h"
#include "bootctrl_make.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/printk.h>
#include <platform_include/see/secboot.h>

static struct semaphore bootctrl_sem;

#ifdef CONFIG_DFX_DEBUG_FS
static bool bootctrl_register_write_test = false;
static bool bootctrl_partition_write_test = false;
void bootctrl_set_register_write_test(void)
{
	bootctrl_register_write_test = true;
}

void bootctrl_set_partition_write_test(void)
{
	bootctrl_partition_write_test = true;
}

bool bootctrl_get_register_write_test(void)
{
	if (bootctrl_register_write_test) {
		bootctrl_register_write_test = false;
		return true;
	}

	return false;
}

bool bootctrl_get_partition_write_test(void)
{
	if (bootctrl_partition_write_test) {
		bootctrl_partition_write_test = false;
		return true;
	}

	return false;
}

#endif

static bool bootctrl_is_slot_valid(unsigned long slot)
{
	if ((slot == BOOT_SLOT_A) || (slot == BOOT_SLOT_B))
		return true;

	return false;
}

static int boot_partition_get_slot(void)
{
	int type, slot;

	type = partition_get_storage_type();
	if ((type != XLOADER_A) && (type != XLOADER_B)) {
		pr_err("%s: get boot partition failed!\n", __func__);
		return BOOT_SLOT_MAX;
	}

	slot = (type == XLOADER_B) ? BOOT_SLOT_B : BOOT_SLOT_A;
	return slot;
}

static int boot_partition_set_slot(int slot)
{
	unsigned int boot_type;
	int ret;

#ifdef CONFIG_DFX_DEBUG_FS
	if (bootctrl_get_register_write_test())
		return 0;
#endif

	boot_type = (slot == BOOT_SLOT_B) ? XLOADER_B : XLOADER_A;
	ret = partition_set_storage_type(boot_type);
	if (ret)
		pr_err("%s: boot partition type set error\n", __func__);

	return ret;
}

static int boot_partition_show(struct seq_file *m, void *v)
{
	int slot;

	slot = boot_partition_get_slot();
	if (!bootctrl_is_slot_valid(slot))
		seq_printf(m, "get device boot partition type error\n");
	else
		seq_printf(m, "%d\n", slot);

	return 0;
}

static int boot_partition_open(struct inode *p_inode, struct file *p_file)
{
	return single_open(p_file, boot_partition_show, NULL);
}

#ifdef CONFIG_DFX_DEBUG_FS
static ssize_t boot_partition_write(struct file *p_file,
				    const char __user *userbuf, size_t count,
				    loff_t *ppos)
{
	char buf, slot;
	int ret;

	if ((!userbuf) || (count == 0)) {
		pr_err("%s: Input is invalid!\n", __func__);
		return -1;
	}

	if (copy_from_user(&buf, userbuf, sizeof(char))) {
		pr_err("%s: copy from user failed!\n", __func__);
		return -1;
	}

	slot = buf - '0';
	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: slot %d is invalid, only support 0 or 1!\n", __func__, slot);
		return -1;
	}

	/* ensure only one process can visit at the same time */
	if (down_interruptible(&bootctrl_sem))
		return -EBUSY;

	ret = boot_partition_set_slot(slot);
	if (ret) {
		pr_err("%s: boot partition type set error\n", __func__);
		up(&bootctrl_sem);
		return -1;
	}

	up(&bootctrl_sem);
	return (ssize_t)count;
}
#endif

static int boot_partition_read_bootctrl(struct bootctrl_info *boot_info)
{
	int ret;
	int fd = -1;
	mm_segment_t old_fs;
	char fullpath[BOOT_CTRL_PTN_PATH_SIZE] = { 0 };

	/* 1. find the partition path name */
	ret = flash_find_ptn_s("boot_ctrl", fullpath, sizeof(fullpath));
	if (ret) {
		pr_err("%s: flash_find_ptn_s fail, ret = %d\n", __func__, ret);
		return ret;
	}
	/*lint -e501*/
	old_fs = get_fs();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	set_fs(KERNEL_DS);
#else
	set_fs(get_ds());
#endif
	/*lint +e501*/

	/* 2. open file */
	fd = SYS_OPEN(fullpath, O_RDONLY, 0);
	if (fd < 0) {
		pr_err("%s: open boot_ctrl failed, fd = %d!\n", __func__, fd);
		ret = -ENODEV;
		goto out;
	}

	/* 3. read data */
	ret = SYS_READ((unsigned int)fd, (char *)boot_info,
		       sizeof(struct bootctrl_info));
	if ((ret < 0) || ((uint32_t)ret != sizeof(struct bootctrl_info))) {
		pr_err("%s: read boot_ctrl data error!\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ret = 0;
out:
	if (fd >= 0)
		SYS_CLOSE((unsigned int)fd);
	set_fs(old_fs);
	return ret;
}

static int boot_partition_write_bootctrl(struct bootctrl_info *boot_info)
{
	int ret;
	int fd = -1;
	mm_segment_t old_fs;
	char fullpath[BOOT_CTRL_PTN_PATH_SIZE] = { 0 };

	/* 1. find the partition path name */
	ret = flash_find_ptn_s("boot_ctrl", fullpath, sizeof(fullpath));
	if (ret) {
		pr_err("%s: flash_find_ptn_s fail, ret = %d\n", __func__, ret);
		return ret;
	}
	/*lint -e501*/
	old_fs = get_fs();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	set_fs(KERNEL_DS);
#else
	set_fs(get_ds());
#endif

	/*lint +e501*/

	/* 2. open file */
	fd = SYS_OPEN(fullpath, O_RDWR, 0);
	if (fd < 0) {
		pr_err("%s: open boot_ctrl failed, fd = %d!\n", __func__, fd);
		ret = -ENODEV;
		goto out;
	}

	/* 3. write data */
	ret = SYS_WRITE((unsigned int)fd, (char *)boot_info,
			sizeof(struct bootctrl_info));
	if ((ret < 0) || ((uint32_t)ret != sizeof(struct bootctrl_info))) {
		pr_err("%s: write boot_ctrl data error!\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ret = SYS_FSYNC((unsigned int)fd);
	if (ret < 0) {
		pr_err("%s: sys_fsync failed, ret = %d\n", __func__, ret);
		goto out;
	}
	ret = 0;

out:
	if (fd >= 0)
		SYS_CLOSE((unsigned int)fd);
	set_fs(old_fs);
	return ret;
}

static int boot_partition_set_slot_active(unsigned int set_slot)
{
	int ret;
#ifndef CONFIG_RECOVERY_AB_PARTITION
	struct bootctrl_info boot_info;

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return -1;
	}

#ifdef CONFIG_DFX_DEBUG_FS
	if (bootctrl_get_partition_write_test())
		return 0;
#endif

	/* 0. set flag use interface from vendor/hisi/bootctrl */
	set_slot_active(&boot_info, set_slot);
	boot_info.crc32 = 0;
	boot_info.crc32 = calculate_crc32((unsigned char *)&boot_info,
					  sizeof(struct bootctrl_info));

	/* 1. write boot_ctrl partition */
	ret = boot_partition_write_bootctrl(&boot_info);
	if (ret) {
		pr_err("%s: write bootctrl fail.\n", __func__);
		return ret;
	}
#endif
	/* 2. set ufs or emmc bootlun register */
	ret = boot_partition_set_slot(set_slot);
	if (ret) {
		pr_err("%s: boot partition type set error.\n", __func__);
		return ret;
	}

	return ret;
}

static bool boot_partition_ioctl_check(u_int cmd, void __user *argp)
{
	int ret = 0;

	if (!argp) {
		pr_err("%s: The input arg is null.\n", __func__);
		return false;
	}

	/* check device type */
	if (_IOC_TYPE(cmd) != 'B') {
		pr_err("%s: command type [%c] error!\n", __func__,
		       _IOC_TYPE(cmd));
		return false;
	}

	/* check number */
	if (_IOC_NR(cmd) >= BOOT_IOCTL_MAX) {
		pr_err("%s: command numer (0x%x) exceeded!\n", __func__,
		       _IOC_NR(cmd));
		return false;
	}

	/* check access mode */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	if ((_IOC_DIR(cmd) & _IOC_READ) || (_IOC_DIR(cmd) & _IOC_WRITE))
		ret = !access_ok(argp, _IOC_SIZE(cmd));
#else
	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, argp, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, argp, _IOC_SIZE(cmd));
#endif
	if (ret)
		return false;

	return true;
}

bool check_slot_as_expected(unsigned long slot)
{
	enum boot_slot_type cur_slot;
#ifndef CONFIG_RECOVERY_AB_PARTITION
	struct bootctrl_info boot_info;

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return false;
	}

	/* 1. read boot_ctrl check active slot */
	cur_slot = get_active_slot(&boot_info, BOOT_SELECT_COMMON);
	if (slot != cur_slot) {
		pr_err("%s: cur_slot 0x%x in disk is not equal to set_slot 0x%lx.\n",
				__func__, cur_slot, slot);
		return false;
	}
#endif
	/* 2. read register and check */
	cur_slot = boot_partition_get_slot();
	if (slot != cur_slot) {
		pr_err("%s: cur_slot 0x%x form register is not equal to set_slot 0x%lx.\n",
				__func__, cur_slot, slot);
		return false;
	}

	return true;
}

static int bootctrl_set_active(const void __user *argp)
{
	unsigned long slot;
	int ret, retries;
	bool valid = false;

	ret = (int)copy_from_user(&slot, argp, sizeof(unsigned long));
	if (ret) {
		pr_err("%s: copy from user fail.\n", __func__);
		return -EFAULT;
	}

	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: Slot(0x%lx) only support 0 or 1!\n", __func__, slot);
		return -EFAULT;
	}

	for (retries = 0; retries < BOOT_CTRL_RETRIES; retries++) {
		ret = boot_partition_set_slot_active(slot);
		if (ret == 0) {
			valid = check_slot_as_expected(slot);
			if (valid)
				break;
		}
	}

	if (ret || !valid)
		return -EFAULT;

	return ret;
}

static int bootctrl_get_active(void __user *argp)
{
	unsigned long slot;
	int ret;

	slot = (unsigned long)boot_partition_get_slot();
	if (!bootctrl_is_slot_valid(slot))
		return -EFAULT;

	ret = (int)copy_to_user(argp, &slot, sizeof(unsigned long));
	if (ret)
		pr_err("%s: copy to user fail.\n", __func__);

	return ret;
}

/*
 * Returns 1 if the slot has been marked as successful, 0 if it's not the case,
 * and -errno on error.
 */
static int is_slot_marked_successful(const void __user *argp)
{
	struct bootctrl_info boot_info;
	unsigned long slot;
	int ret;

	ret = (int)copy_from_user(&slot, argp, sizeof(unsigned long));
	if (ret) {
		pr_err("%s: copy from user fail.\n", __func__);
		return -EFAULT;
	}

	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: Slot(0x%lx) only support 0 or 1!\n", __func__, slot);
		return -EFAULT;
	}

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return -1;
	}

	if ((boot_info.slot_info[slot].flags & BOOT_SLOT_SUCCESSFUL) == BOOT_SLOT_SUCCESSFUL) {
		pr_info("%s: slot 0x%lx is marked successful!\n", __func__, slot);
		return 1;
	}

	pr_info("%s: slot 0x%lx is not marked successful!\n", __func__, slot);
	return 0;
}

/*
 * Returns 1 if the slot is bootable, 0 if it's not, and -errno on error.
 */
static int is_slot_bootable(const void __user *argp)
{
	struct bootctrl_info boot_info;
	unsigned long slot;
	int ret;

	ret = (int)copy_from_user(&slot, argp, sizeof(unsigned long));
	if (ret) {
		pr_err("%s: copy from user fail.\n", __func__);
		return -EFAULT;
	}

	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: Slot(0x%lx) only support 0 or 1!\n", __func__, slot);
		return -EFAULT;
	}

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return -1;
	}

	if ((boot_info.slot_info[slot].flags & BOOT_SLOT_NONBOOTABLE) != BOOT_SLOT_NONBOOTABLE) {
		pr_info("%s: slot 0x%lx is bootable!\n", __func__, slot);
		return 1;
	}

	pr_info("%s: slot 0x%lx is unbootable!\n", __func__, slot);
	return 0;
}

static int set_slot_unbootable(const void __user *argp)
{
	struct bootctrl_info boot_info;
	unsigned long slot;
	int ret;

	ret = (int)copy_from_user(&slot, argp, sizeof(unsigned long));
	if (ret) {
		pr_err("%s: copy from user fail.\n", __func__);
		return -EFAULT;
	}

	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: Slot(0x%lx) only support 0 or 1!\n", __func__, slot);
		return -EFAULT;
	}

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return -1;
	}

	boot_info.slot_info[slot].flags |= BOOT_SLOT_NONBOOTABLE;
	boot_info.crc32 = 0;
	boot_info.crc32 = calculate_crc32((unsigned char *)&boot_info,
					  sizeof(struct bootctrl_info));
	ret = boot_partition_write_bootctrl(&boot_info);
	if (ret)
		pr_err("%s: write bootctrl fail.\n", __func__);

	return ret;
}

static int set_slot_successful(const void __user *argp)
{
	struct bootctrl_info boot_info;
	unsigned long slot;
	int ret;

	ret = (int)copy_from_user(&slot, argp, sizeof(unsigned long));
	if (ret) {
		pr_err("%s: copy from user fail.\n", __func__);
		return -EFAULT;
	}

	if (!bootctrl_is_slot_valid(slot)) {
		pr_err("%s: Slot(0x%lx) only support 0 or 1!\n", __func__, slot);
		return -EFAULT;
	}

	if (boot_partition_read_bootctrl(&boot_info)) {
		pr_err("%s: read boot_ctrl fail!\n", __func__);
		return -1;
	}

	boot_info.slot_info[slot].flags |= BOOT_SLOT_SUCCESSFUL;
	boot_info.slot_info[slot].retry_remain = BOOT_SLOT_RETRY_COUNT;
	boot_info.crc32 = 0;
	boot_info.crc32 = calculate_crc32((unsigned char *)&boot_info,
					  sizeof(struct bootctrl_info));
	ret = boot_partition_write_bootctrl(&boot_info);
	if (ret) {
		pr_err("%s: write bootctrl fail.\n", __func__);
		return ret;
	}

	seb_trigger_update_version();

	return 0;
}

static long boot_partition_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret;
	void __user *argp = (void __user *)arg;

	if (!boot_partition_ioctl_check(cmd, argp)) {
		pr_err("%s: invalid para!\n", __func__);
		return -EFAULT;
	}

	/* ensure only one process can visit at the same time */
	if (down_interruptible(&bootctrl_sem))
		return -EBUSY;

	switch (cmd) {
	case BOOTCTRLSETACTIVE:
		ret = bootctrl_set_active(argp);
		if (ret)
			pr_err("%s: bootctrl set active fail.\n", __func__);
		break;
	case BOOTCTRLGETACTIVE:
		ret = bootctrl_get_active(argp);
		if (ret)
			pr_err("%s: bootctrl get active fail.\n", __func__);
		break;
	case BOOTCTRLSETUNBOOTABLE:
		ret = set_slot_unbootable(argp);
		break;
	case BOOTCTRLMARKSUCCESSFUL:
		ret = set_slot_successful(argp);
		break;
	case BOOTCTRLISSLOTBOOTABLE:
		ret = is_slot_bootable(argp);
		break;
	case BOOTCTRLISSLOTSUCCESSFUL:
		ret = is_slot_marked_successful(argp);
		break;
	default:
		pr_err("%s: Unknow command!\n", __func__);
		ret = -ENOTTY;
		break;
	}

	up(&bootctrl_sem);
	return (long)ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops boot_partition_fops = {
	.proc_open = boot_partition_open,
	.proc_read = seq_read,
#ifdef CONFIG_DFX_DEBUG_FS
	.proc_write = boot_partition_write,
#endif
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
	.proc_ioctl = boot_partition_ioctl,
#ifdef CONFIG_COMPAT
	.proc_compat_ioctl = boot_partition_ioctl,
#endif
};
#else
static const struct file_operations boot_partition_fops = {
	.open = boot_partition_open,
	.read = seq_read,
#ifdef CONFIG_DFX_DEBUG_FS
	.write = boot_partition_write,
#endif
	.llseek = seq_lseek,
	.release = single_release,
	.unlocked_ioctl = boot_partition_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = boot_partition_ioctl,
#endif
};
#endif

static int __init proc_boot_partition_init(void)
{
	sema_init(&bootctrl_sem, 1);
	proc_create("boot_partition", 0660, NULL, &boot_partition_fops);
	return 0;
}

fs_initcall(proc_boot_partition_init);
