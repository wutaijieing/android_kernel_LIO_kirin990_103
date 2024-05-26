/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * ipc_vm_dev.c
 *
 * add ipc vm dev for hm_uvmm
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <soc_acpu_baseaddr_interface.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include "ipc_mailbox_dev.h"
#include "ipc_mailbox.h"

#define IPC_VM_DEV_NAME        "ipc_dev"

#define IPC_IOMAP_NUM 5
#define IPCACKMSG 0x00000000
#define AUTOMATIC_ACK_CONFIG (1 << 0)
#define IPC_OFFSET_MAX       (0x1000)

#define IPC_MAILBOX_HVC_ID  0xAABB0000
#define IPC_SEND_SYNC       0xAABB0001
#define IPC_SEND_ASYNC      0xAABB0002
#define IPC_READL           0xAABB0003
#define IPC_WRITEL          0xAABB0004
#define IPC_HW_REC          0xAABB0005

#define IPC_REG_SPC         IPC_REG_SPACE
#define IPC_CAPABILITY      8
#define IPC_SRC_ID_0        0
#define IPC_SRC_ID_1        1
#define IPC_MAX_RECV        9

struct rproc_info {
	rproc_id_t rproc_id;
	rproc_msg_t *msg; /* hva */
	rproc_msg_len_t len;
	rproc_msg_t *ack_buffer; /* hva */
	rproc_msg_len_t ack_buffer_len;
};

struct rproc_reg {
	unsigned int phys_base;
	unsigned int offset;
	unsigned int *val;
};

struct rproc_hw_rec {
	unsigned int phys_base;
	unsigned int mbox_channel;
	unsigned int reg_spc;
	unsigned int *val;
	unsigned int capability;
	unsigned int rproc_src_id_0;
	unsigned int rproc_src_id_1;
};

struct ipc_ioremap {
	unsigned int phys_base;
	void __iomem *base;
};
struct ipc_vm_dev_device {
	struct device *dev;
	int     dev_major;
	struct class *dev_class;
	struct ipc_ioremap reg_map[IPC_IOMAP_NUM];
	int map_num;
};

static DEFINE_MUTEX(ipc_dev_lock);
struct ipc_vm_dev_device *g_ipc_dev = NULL;

static rproc_id_t g_unusable_list[] = {
	IPC_ACPU_LPM3_MBX_3,
	IPC_ACPU_IOM3_MBX_2,
	IPC_ACPU_IOM3_MBX_3,
	IPC_AO_ACPU_IOM3_MBX1,
	IPC_ACPU_LPM3_MBX_6,
};

static int ipc_check_rproc_id(rproc_id_t rproc_id)
{
	int i;
	for (i = 0; i < sizeof(g_unusable_list) / sizeof(g_unusable_list[0]); i++) {
		if (rproc_id == g_unusable_list[i]) {
			pr_err("[%s] %u can't used by guest\n", __func__, rproc_id);
			return -EINVAL;
		}
	}
	return 0;
}

static int ipc_check_hw_rec(struct rproc_hw_rec *hw_rec)
{
	if ((hw_rec->val == NULL) ||
		(hw_rec->mbox_channel > IPC_MAX_RECV) ||
		(hw_rec->reg_spc != IPC_REG_SPC) ||
		(hw_rec->capability != IPC_CAPABILITY) ||
		(hw_rec->rproc_src_id_0 != IPC_SRC_ID_0) ||
		(hw_rec->rproc_src_id_1 != IPC_SRC_ID_1)) {
		pr_err("[%s] hw_rec invalid, channel = %u, spc = %u, cap = %u, sid0 = %u, sid1 = %u\n",
			__func__, hw_rec->mbox_channel, hw_rec->reg_spc, hw_rec->capability,
			hw_rec->rproc_src_id_0, hw_rec->rproc_src_id_1);
		return -EINVAL;
	}
	return 0;
}

static int ipc_copy_rproc(struct rproc_info *rproc, unsigned long args)
{
	void __user *user_args = NULL;
	unsigned int ret;

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user(rproc, user_args, sizeof(*rproc));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
static int ipc_vm_dev_sync_send(unsigned long args)
{
	int ret;
	struct rproc_info rproc = {0};
	mbox_msg_t tx_buffer[MBOX_CHAN_DATA_SIZE] = {0};
	rproc_msg_t ack_buffer[MBOX_CHAN_DATA_SIZE] = {0};

	ret = ipc_copy_rproc(&rproc, args);
	if (ret != 0) {
		pr_err("[%s] ipc_copy_rproc error %d\n", __func__);
		return -EFAULT;
	}

	if (rproc.len > MBOX_CHAN_DATA_SIZE) {
		pr_err("[%s] len error %d\n", __func__, rproc.len);
		return -EFAULT;
	}

	ret = (int)copy_from_user(tx_buffer, (void __user *)(rproc.msg),
		sizeof(mbox_msg_t) * rproc.len);
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}
	ret = ipc_check_rproc_id(rproc.rproc_id);
	if (ret != 0)
		return -EFAULT;

	ret = RPROC_SYNC_SEND(rproc.rproc_id,
		tx_buffer, rproc.len, ack_buffer, MBOX_CHAN_DATA_SIZE);
	if (ret != 0) {
		pr_err("%s: mail_id:%d rproc sync send err, err = %d\n",
			__func__, rproc.rproc_id, ret);
		return -EINVAL;
	}

	if (rproc.ack_buffer_len > 0 && rproc.ack_buffer_len <= MBOX_CHAN_DATA_SIZE) {
		ret = (int)copy_to_user((void __user *)rproc.ack_buffer, ack_buffer,
				rproc.ack_buffer_len * sizeof(rproc_msg_t));
		if (ret != 0) {
			pr_err("[%s] copy_to_user %d, ack len %d\n", __func__, ret, rproc.ack_buffer_len);
			ret = -EFAULT;
		}
	}

	return 0;
}

static int ipc_vm_dev_async_send(unsigned long args)
{
	int ret;
	mbox_msg_t tx_buffer[MBOX_CHAN_DATA_SIZE] = {0};
	struct rproc_info rproc = {0};

	ret = ipc_copy_rproc(&rproc, args);
	if (ret != 0) {
		pr_err("[%s] ipc_copy_rproc error %d\n", __func__);
		return -EFAULT;
	}

	if (rproc.len > MBOX_CHAN_DATA_SIZE) {
		pr_err("[%s] len error %d\n", __func__, rproc.len);
		return -EFAULT;
	}

	ret = (int)copy_from_user(tx_buffer, (void __user *)(rproc.msg),
		sizeof(mbox_msg_t) * rproc.len);
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}
	ret = ipc_check_rproc_id(rproc.rproc_id);
	if (ret != 0)
		return -EFAULT;

	ret = RPROC_ASYNC_SEND(rproc.rproc_id, tx_buffer, rproc.len);
	if (ret != 0) {
		pr_err("%s: mail_id:%d rproc sync send err, err = %d\n",
		  __func__, rproc.rproc_id, ret);
		return -EINVAL;
	}

	return 0;
}

static void __iomem * ipc_vm_dev_ioremap(unsigned int phys_base)
{
	struct ipc_vm_dev_device *vm_dev = g_ipc_dev;
	int i;
	int num;

	num = (vm_dev->map_num < IPC_IOMAP_NUM) ? vm_dev->map_num : IPC_IOMAP_NUM;

	/* find */
	for (i = 0; i < num; i++) {
		if(vm_dev->reg_map[i].phys_base == phys_base)
			return vm_dev->reg_map[i].base;
	}

	if (i == IPC_IOMAP_NUM) {
		pr_err("[%s] vm_dev->map_num %d fail\n", __func__, vm_dev->map_num);
		i = IPC_IOMAP_NUM - 1;
		iounmap(vm_dev->reg_map[i].base);
	}

	/* not find */
	vm_dev->reg_map[i].phys_base = phys_base;
	vm_dev->reg_map[i].base = ioremap(phys_base, sizeof(unsigned int));
	if (vm_dev->reg_map[i].base == NULL) {
		vm_dev->reg_map[i].phys_base = 0;
		pr_err("[%s] ioremap fail\n", __func__);
		return NULL;
	}
	vm_dev->map_num++;

	return vm_dev->reg_map[i].base;
}

#ifdef CONFIG_DEBUG_FS
static int ipc_vm_dev_readl(unsigned long args)
{
	unsigned int ret;
	unsigned int val;
	void __iomem *v_addr = NULL;
	void __user *user_args = NULL;
	struct rproc_reg reg = {0};

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user(&reg, user_args, sizeof(reg));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}

	v_addr = ipc_vm_dev_ioremap(reg.phys_base);
	if (v_addr == NULL) {
		pr_err("[%s] ipc_vm_dev_ioremap failed!\n", __func__);
		return -EINVAL;
	}
	v_addr = v_addr + reg.offset;
	val = __raw_readl(v_addr);

	ret = copy_to_user((void __user *)reg.val, &val, sizeof(unsigned int));
	if (ret != 0) {
		pr_err("[%s] copy_to_user %d\n", __func__, ret);
		ret = -EFAULT;
	}

	return 0;
}

static int ipc_vm_dev_writel(unsigned long args)
{
	unsigned int ret;
	unsigned int val;
	void __iomem *v_addr = NULL;
	void __user *user_args = NULL;
	struct rproc_reg reg = {0};

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user(&reg, user_args, sizeof(reg));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}

	ret = copy_from_user(&val, reg.val, sizeof(unsigned int));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}

	v_addr = ipc_vm_dev_ioremap(reg.phys_base);
	if (v_addr == NULL) {
		pr_err("[%s] ipc_vm_dev_ioremap failed!\n", __func__);
		return -EINVAL;
	}
	v_addr = v_addr + reg.offset;
	__raw_writel(val, v_addr);

	return 0;
}
#endif

static int vdev_ipc_address_check(unsigned int phys_base, unsigned int offset)
{
	if (phys_base != SOC_ACPU_AO_IPC_NS_BASE_ADDR && phys_base != SOC_ACPU_IPC_NS_BASE_ADDR) {
		pr_err("[%s] error %pK\n", __func__, phys_base);
		return -1;
	}

	if (offset >= IPC_OFFSET_MAX) {
		pr_err("[%s] error 0x%x\n", __func__, offset);
		return -1;
	}

	return 0;
}

static inline unsigned int _vdev_ipc_readl(
	unsigned int phys_base, unsigned int offset)
{
	void __iomem *v_addr = NULL;

	if(vdev_ipc_address_check(phys_base, offset)) {
		pr_err("[%s] address check fail\n", __func__);
		return 0;
	}

	v_addr = ipc_vm_dev_ioremap(phys_base);
	if (v_addr == NULL) {
		pr_err("[%s] ipc_vm_dev_ioremap failed!\n", __func__);
		return -EINVAL;
	}
	v_addr = v_addr + offset;
	return __raw_readl(v_addr);
}

static inline void _vdev_ipc_writel(
	unsigned int phys_base, unsigned int val, unsigned int offset)
{
	void __iomem *v_addr = NULL;

	if (vdev_ipc_address_check(phys_base, offset)) {
		pr_err("[%s] address check fail\n", __func__);
		return;
	}

	v_addr = ipc_vm_dev_ioremap(phys_base);
	if (v_addr == NULL) {
		pr_err("[%s] ipc_vm_dev_ioremap failed!\n", __func__);
		return;
	}
	v_addr = v_addr + offset;
	__raw_writel(val, v_addr);
}

static inline unsigned int _vdev_ipc_read(
	struct rproc_hw_rec *hw_rec, int index)
{
	return _vdev_ipc_readl(hw_rec->phys_base,
		(unsigned int)ipc_mbx_data(hw_rec->mbox_channel,
		(unsigned int)index, hw_rec->reg_spc));
}

static inline void _vdev_ipc_write(
	struct rproc_hw_rec *hw_rec, u32 data, int index)
{
	_vdev_ipc_writel(hw_rec->phys_base, data,
		ipc_mbx_data(hw_rec->mbox_channel, (unsigned int)index, hw_rec->reg_spc));
}


static inline unsigned int _vdev_ipc_cpu_imask_get(
	struct rproc_hw_rec *hw_rec)
{
	return _vdev_ipc_readl(hw_rec->phys_base,
		(unsigned int)ipc_mbx_imask(hw_rec->mbox_channel, hw_rec->reg_spc));
}

static inline void _vdev_ipc_cpu_iclr(
	struct rproc_hw_rec *hw_rec, unsigned int toclr)
{
	_vdev_ipc_writel(hw_rec->phys_base, toclr,
		(unsigned int)ipc_mbx_iclr(hw_rec->mbox_channel, hw_rec->reg_spc));
}

static inline unsigned int _vdev_ipc_status(struct rproc_hw_rec *hw_rec)
{
	return _vdev_ipc_readl(hw_rec->phys_base,
		(unsigned int)ipc_mbx_mode(hw_rec->mbox_channel, hw_rec->reg_spc));
}

static inline void _vdev_ipc_send(struct rproc_hw_rec *hw_rec, unsigned int tosend)
{
	_vdev_ipc_writel(hw_rec->phys_base, tosend,
		(unsigned int)ipc_mbx_send(hw_rec->mbox_channel, hw_rec->reg_spc));
}

static void ipc_vdev_clr_irq_and_ack(struct rproc_hw_rec *hw_rec)
{
	unsigned int status;
	unsigned int imask;
	unsigned int todo;
	int i;

	/*
	 * Temporarily, local processor will clean msg register,
	 * and ack zero for an ipc from remote processors.
	 */
	for (i = 0; i < hw_rec->capability; i++)
		_vdev_ipc_write(hw_rec, IPCACKMSG, i);

	imask = _vdev_ipc_cpu_imask_get(hw_rec);

	/*
	 * Get the irq unmask core bits, and clear the irq according to the
	 * unmask core bits,
	 * because the irq to be sure triggered to the unmasked cores
	 */
	todo = (ipc_bitmask(hw_rec->rproc_src_id_0) |
		ipc_bitmask(hw_rec->rproc_src_id_1)) & (~imask);

	_vdev_ipc_cpu_iclr(hw_rec, todo);

	status = _vdev_ipc_status(hw_rec);
	if ((DESTINATION_STATUS & status) &&
		(!(AUTOMATIC_ACK_CONFIG & status)))
		_vdev_ipc_send(hw_rec, todo);
}

static void ipc_vdev_receive_msg(
	struct rproc_hw_rec *hw_rec, mbox_msg_t *rx_buffer)
{
	int i;

	for (i = 0; i < hw_rec->capability; i++)
		rx_buffer[i] = _vdev_ipc_read(hw_rec, i);

	ipc_vdev_clr_irq_and_ack(hw_rec);
}

static int ipc_vm_dev_hw_rec(unsigned long args)
{
	int ret;
	void __iomem *v_addr = NULL;
	void __user *user_args = NULL;
	struct rproc_hw_rec hw_rec = {0};
	unsigned int val[MBOX_CHAN_DATA_SIZE] = {0};

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user(&hw_rec, user_args, sizeof(hw_rec));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}
	ret = ipc_check_hw_rec(&hw_rec);
	if (ret != 0)
		return -EINVAL;

	ipc_vdev_receive_msg(&hw_rec, val);

	ret = copy_to_user((void __user *)hw_rec.val, &val,
			MBOX_CHAN_DATA_SIZE * sizeof(unsigned int));
	if (ret != 0) {
		pr_err("[%s] copy_to_user %d error\n", __func__, ret);
		ret = -EFAULT;
	}

	return ret;
}

static long ipc_vm_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long args)
{
	int ret = 0;

	mutex_lock(&ipc_dev_lock);
	switch (cmd) {
	case IPC_SEND_SYNC:
		ret = ipc_vm_dev_sync_send(args);
		break;
	case IPC_SEND_ASYNC:
		ret = ipc_vm_dev_async_send(args);
		break;
#ifdef CONFIG_DEBUG_FS
	case IPC_READL:
		ret = ipc_vm_dev_readl(args);
		break;
	case IPC_WRITEL:
		ret = ipc_vm_dev_writel(args);
		break;
#endif
	case IPC_HW_REC:
		ret = ipc_vm_dev_hw_rec(args);
		break;
	default:
		ret = -EINVAL;
		pr_err("[%s] invalid cmd value\n", __func__);
	}
	mutex_unlock(&ipc_dev_lock);

	return ret;
}

const static struct file_operations ipc_vm_dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = ipc_vm_dev_ioctl,
};

static int __init ipc_vm_dev_init(void)
{
	struct ipc_vm_dev_device *vm_dev = NULL;
	struct device *pdevice = NULL;
	int ret = 0;

	vm_dev = (struct ipc_vm_dev_device *)kzalloc(
			sizeof(struct ipc_vm_dev_device), GFP_KERNEL);
	if (!vm_dev) {
		pr_err("%s failed to alloc pvm_dev struct\n", __func__);
		return -ENOMEM;
	}

	vm_dev->dev_major = register_chrdev(0, IPC_VM_DEV_NAME, &ipc_vm_dev_fops);
	if (vm_dev->dev_major < 0) {
		pr_err("[%s] unable to get dev_majorn",__func__);
		ret = -EFAULT;
		goto dev_major_err;
	}

	mutex_lock(&ipc_dev_lock);
	vm_dev->dev_class = class_create(THIS_MODULE, IPC_VM_DEV_NAME);
	if (IS_ERR(vm_dev->dev_class)) {
		pr_err("[%s] class_create error\n",__func__);
		ret = -EFAULT;
		goto dev_class_err;
	}

	pdevice = device_create(vm_dev->dev_class, NULL,
			MKDEV((unsigned int)vm_dev->dev_major, 0), NULL, IPC_VM_DEV_NAME);
	if (IS_ERR(pdevice)) {
		pr_err("ipc_vm_dev: device_create error\n");
		ret = -EFAULT;
		goto dev_create_err;
	}

	g_ipc_dev = vm_dev;
	mutex_unlock(&ipc_dev_lock);

	pr_err("%s success\n", __func__);
	return ret;

dev_create_err:
	class_destroy(vm_dev->dev_class);
	vm_dev->dev_class = NULL;

dev_class_err:
	unregister_chrdev(vm_dev->dev_major, IPC_VM_DEV_NAME);
	vm_dev->dev_major = 0;

dev_major_err:

	kfree(vm_dev);
	return ret;
}

static void __exit  ipc_vm_dev_exit(void)
{
	if (g_ipc_dev == NULL)
		return;

	device_destroy(g_ipc_dev->dev_class, MKDEV((unsigned int)g_ipc_dev->dev_major, 0));
	class_destroy(g_ipc_dev->dev_class);
	g_ipc_dev->dev_class = NULL;
	unregister_chrdev(g_ipc_dev->dev_major, IPC_VM_DEV_NAME);
	g_ipc_dev->dev_major = 0;

	kfree(g_ipc_dev);
	g_ipc_dev = NULL;
}

module_init(ipc_vm_dev_init);
module_exit(ipc_vm_dev_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ipc vm dev module");
