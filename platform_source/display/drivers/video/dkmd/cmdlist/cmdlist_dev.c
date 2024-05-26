/**
 * @file cmdlist_dev.c
 * @brief Provide equipment related interfaces
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include "dkmd_log.h"

#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include "chash.h"
#include "cmdlist_node.h"
#include "cmdlist_client.h"
#include "cmdlist_dev.h"
#include "cmdlist_drv.h"

struct cmdlist_ioctl_data {
	uint32_t ioctl_cmd;
	int32_t (* ioctl_func)(struct cmdlist_table *cmd_table, struct cmdlist_node_client *);
};

static struct cmdlist_ioctl_data g_ioctl_data[] = {
	{ CMDLIST_CREATE_CLIENT, cmdlist_create_client },
	{ CMDLIST_SIGNAL_CLIENT, cmdlist_signal_client },
	{ CMDLIST_RELEASE_CLIENT, cmdlist_release_client },
	{ CMDLIST_APPEND_NEXT_CLIENT, cmdlist_append_next_client },
	{ CMDLIST_DUMP_USER_CLIENT, cmdlist_dump_client },
	{ CMDLIST_DUMP_SCENE_CLIENT, cmdlist_dump_scene },
};

static int32_t cmdlist_open(struct inode *inode, struct file *file)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	if (!file) {
		dpu_pr_err("fail to open!");
		return -EINVAL;
	}

	file->private_data = NULL;
	if (cmd_priv->device_initialized == 0) {
		dpu_pr_err("device is not initialized!");
		return -EINVAL;
	}
	file->private_data = cmd_priv;

	down(&cmd_priv->table_sem);
	if (cmd_priv->ref_count == 0) {
		dpu_pr_info("pool memery status: %d, %d!",
			gen_pool_size(cmd_priv->memory_pool), gen_pool_avail(cmd_priv->memory_pool));
		cmdlist_client_table_setup(&cmd_priv->cmd_table);
	}
	cmd_priv->ref_count++;
	dpu_pr_info("cmd_priv->ref_count=%d!", cmd_priv->ref_count);

	up(&cmd_priv->table_sem);

	return 0;
}

static int32_t cmdlist_close(struct inode *inode, struct file *file)
{
	struct cmdlist_private *cmd_priv = NULL;

	if (!file) {
		dpu_pr_err("fail to open!");
		return -EINVAL;
	}

	cmd_priv = (struct cmdlist_private *)file->private_data;
	if (!cmd_priv) {
		dpu_pr_err("cmd_priv is null!");
		return -EINVAL;
	}

	down(&cmd_priv->table_sem);
	cmd_priv->ref_count--;
	if (cmd_priv->ref_count == 0) {
		dpu_pr_info("pool memery status: %d, %d!",
			gen_pool_size(cmd_priv->memory_pool), gen_pool_avail(cmd_priv->memory_pool));
		cmdlist_client_table_release(&cmd_priv->cmd_table);
	}
	up(&cmd_priv->table_sem);

	return 0;
}

static int32_t cmdlist_dev_mmap(struct cmdlist_private *cmd_priv, struct vm_area_struct *vma)
{
	int32_t ret = 0;
	unsigned long size;
	unsigned long addr;

	if (!vma) {
		dpu_pr_err("vma is null!");
		return -EINVAL;
	}

	addr = vma->vm_start;
	size = vma->vm_end - vma->vm_start;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if ((size == 0) || (size > cmd_priv->sum_pool_size)) {
		dpu_pr_err("mmap error size:%#x", size);
		return -EINVAL;
	}

	ret = remap_pfn_range(vma, addr, (cmd_priv->pool_phy_addr >> PAGE_SHIFT), size, vma->vm_page_prot);
	if (ret != 0) {
		dpu_pr_err("failed to remap_pfn_range! ret=%d", ret);
		return ret;
	}

	return ret;
}

static int32_t cmdlist_mmap(struct file *file, struct vm_area_struct *vma)
{
	int32_t ret = 0;
	struct cmdlist_private *cmd_priv = NULL;

	if (!file) {
		dpu_pr_err("fail to open!");
		return -EINVAL;
	}

	cmd_priv = (struct cmdlist_private *)file->private_data;
	if (!cmd_priv) {
		dpu_pr_err("cmd_priv is null!");
		return -EINVAL;
	}

	down(&cmd_priv->table_sem);
	if (cmd_priv->ref_count <= 0) {
		dpu_pr_err("dev is not open correct!");
		up(&cmd_priv->table_sem);
		return -EINVAL;
	}

	ret = cmdlist_dev_mmap(cmd_priv, vma);

	up(&cmd_priv->table_sem);

	return ret;
}

static int32_t get_cmdlist_device_info(struct cmdlist_private *cmd_priv, unsigned long arg)
{
	struct cmdlist_info info = {0};
	void __user *argp = (void __user *)(uintptr_t)arg;

	info.pool_size = cmd_priv->sum_pool_size;
	info.viraddr_base = (uint64_t)(uintptr_t)cmd_priv->pool_vir_addr;
	if (copy_to_user(argp, &info, sizeof(info))) {
		dpu_pr_err("copy_to_user failed!");
		return -EFAULT;
	}

	dpu_pr_info("pool_size=%d, viraddr_base=%pK", info.pool_size, info.viraddr_base);

	return 0;
}

static int32_t cmdlist_ioctl_user_client_handler(struct cmdlist_private *cmd_priv, uint32_t cmd, unsigned long arg)
{
	int32_t i;
	int32_t ret = -EINVAL;
	struct cmdlist_node_client user_client;
	struct cmdlist_node_client __user *argp = (struct cmdlist_node_client __user *)(uintptr_t)arg;

	if (copy_from_user(&user_client, argp, sizeof(user_client)) != 0) {
		dpu_pr_err("copy for user failed! ret=%d", ret);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(g_ioctl_data); i++) {
		if (cmd == g_ioctl_data[i].ioctl_cmd) {
			ret = g_ioctl_data[i].ioctl_func(&cmd_priv->cmd_table, &user_client);
			break;
		}
	}

	if (copy_to_user(argp, &user_client, sizeof(user_client)) != 0) {
		cmdlist_release_client(&cmd_priv->cmd_table, &user_client);
		dpu_pr_err("copy_to_user failed!");
		ret = -EFAULT;
	}

	return ret;
}

static long cmdlist_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
	long ret = 0;
	struct cmdlist_private *cmd_priv = NULL;

	if (!file || !arg) {
		dpu_pr_err("file or arg is null!");
		return -EINVAL;
	}

	cmd_priv = (struct cmdlist_private *)file->private_data;
	if (!cmd_priv) {
		dpu_pr_err("cmd_priv is null!");
		return -EINVAL;
	}

	down(&cmd_priv->table_sem);
	if (cmd_priv->ref_count <= 0) {
		dpu_pr_err("dev is not open correct!");
		up(&cmd_priv->table_sem);
		return -EINVAL;
	}

	switch (cmd) {
	case CMDLIST_INFO_GET:
		ret = get_cmdlist_device_info(cmd_priv, arg);
		break;
	default:
		ret = cmdlist_ioctl_user_client_handler(cmd_priv, cmd, arg);
		break;
	}
	up(&cmd_priv->table_sem);

	return ret;
}

static const struct file_operations cmdlist_fops = {
	.owner = THIS_MODULE,
	.open = cmdlist_open,
	.release = cmdlist_close,
	.unlocked_ioctl = cmdlist_ioctl,
	.compat_ioctl = cmdlist_ioctl,
	.mmap = cmdlist_mmap,
};

int32_t cmdlist_device_setup(struct cmdlist_private *cmd_priv)
{
	int32_t ret = 0;

	cmd_priv->chr_major = register_chrdev(0, DEV_NAME_CMDLIST, &cmdlist_fops);
	if (cmd_priv->chr_major < 0 ) {
		dpu_pr_err("error %d registering cmdlist chrdev!", cmd_priv->chr_major);
		return -ENXIO;
	}

	cmd_priv->chr_class = class_create(THIS_MODULE, "cmdlist");
	if (IS_ERR_OR_NULL(cmd_priv->chr_class)) {
		dpu_pr_err("create cmdlist class fail!");
		ret = PTR_ERR(cmd_priv->chr_class);
		goto err_chr_class;
	}

	cmd_priv->chr_dev = device_create(cmd_priv->chr_class, 0,
		MKDEV((unsigned)cmd_priv->chr_major, 0), NULL, DEV_NAME_CMDLIST);
	if (IS_ERR_OR_NULL(cmd_priv->chr_dev)) {
		dpu_pr_err("create cmdlist char device fail!");
		ret = PTR_ERR(cmd_priv->chr_dev);
		goto err_chr_device;
	}

	if (cmdlist_client_table_setup(&cmd_priv->cmd_table) != 0) {
		dpu_pr_err("cmd table setup failed!");
		ret = -EINVAL;
		goto err_chr_device;
	}
	dev_set_drvdata(cmd_priv->chr_dev, cmd_priv);

	dpu_pr_debug("add cmdlist chr_dev device succ!");
	return ret;

err_chr_device:
	class_destroy(cmd_priv->chr_class);
err_chr_class:
	unregister_chrdev(cmd_priv->chr_major, DEV_NAME_CMDLIST);
	return ret;
}

void cmdlist_device_release(struct cmdlist_private *cmd_priv)
{
	if (!cmd_priv) {
		dpu_pr_err("cmd_priv is null!");
		return;
	}

	cmdlist_client_table_release(&cmd_priv->cmd_table);

	if (cmd_priv->chr_major > 0) {
		device_destroy(cmd_priv->chr_class, MKDEV((unsigned)cmd_priv->chr_major, 0));
		class_destroy(cmd_priv->chr_class);
		unregister_chrdev(cmd_priv->chr_major, DEV_NAME_CMDLIST);
		cmd_priv->chr_major = 0;
		dpu_pr_debug("unregister cmdlist chrdev device succ!");
	}
}
