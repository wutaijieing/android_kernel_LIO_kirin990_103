/*
 *
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <platform_include/basicplatform/linux/dfx_pstore.h>
#include <platform_include/basicplatform/linux/mntn_dump.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/uaccess.h> /* For copy_to_user */
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <securec.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#define PR_LOG_TAG BLACKBOX_TAG
#define LOG_TAG "dfx persist store"
#define PERSIST_STORE_NAMELEN 64 /* PSTORE_NAMELEN */

struct persist_store_info {
	struct list_head node;
	char name[PERSIST_STORE_NAMELEN];
	size_t size;
	char data[0];
};

static DEFINE_RAW_SPINLOCK(persist_store_lock);
LIST_HEAD(__list_persist_store);

/* read persist store file content */
static ssize_t persist_store_file_read(struct file *file,
				char __user *userbuf, size_t bytes,
				loff_t *off)
{
	struct persist_store_info *info = NULL;
	ssize_t copy;
	size_t size;

	if ((!file) || (!off))
		return -EFAULT;

	if (!userbuf) {
		pr_err("%s(), userbuf is NULL\n", __func__);
		return 0;
	}

	info = (struct persist_store_info *)file->private_data;

	if (!info) {
		pr_err("%s(), the proc file don't be created in advance\n", __func__);
		return 0;
	}

	size = info->size;

	if ((*off < 0) || (*off > (loff_t)size)) {
		pr_err("%s(), read offset error\n", __func__);
		return 0;
	}

	if (*off == (loff_t)size) {
		/* end of file */
		return 0;
	}

	copy = (ssize_t)min(bytes, (size_t)(size - (size_t)*off));

	if (copy_to_user(userbuf, info->data + *off, copy)) {
		pr_err("%s(): copy to user error\n", __func__);
		copy = -EFAULT;
		goto copy_err;
	}

	*off += copy;

copy_err:
	return copy;
}

static int persist_store_file_open(struct inode *inode, struct file *file)
{
	if ((!inode) || (!file))
		return -EFAULT;

	file->private_data = PDE_DATA(inode);

	if (list_empty(&__list_persist_store)) {
		pr_err("%s(): dfx pstore has not init yet\n", __func__);
		return -EFAULT;
	}
	return 0;
}

static int persist_store_file_release(struct inode *inode,
				struct file *file)
{
	if (!file)
		return -EFAULT;

	file->private_data = NULL;
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops g_persist_store_file_fops = {
	.proc_open = persist_store_file_open,
	.proc_read = persist_store_file_read,
	.proc_release = persist_store_file_release,
};
#else
static const struct file_operations g_persist_store_file_fops = {
	.open = persist_store_file_open,
	.read = persist_store_file_read,
	.release = persist_store_file_release,
};
#endif
/*
 * get pstore inode and save its memory info to a list
 * History
 * Modification : Created function
 */
void dfx_save_pstore_log(const char *name, const void *data, size_t size)
{
	struct persist_store_info *info = NULL;

	/* as a public interface, we should check the parameter */
	if (IS_ERR_OR_NULL(name) || IS_ERR_OR_NULL(data)) {
		pr_err("%s(): parameter is NULL\n", __func__);
		return;
	}

	if (!size) {
		pr_err("%s(): size is zero\n", __func__);
		return;
	}
	info = kzalloc(sizeof(*info) + size, GFP_ATOMIC);
	if (IS_ERR_OR_NULL(info)) {
		pr_err("%s(), kzalloc fail !\n", __func__);
		return;
	}

	if (strncpy_s(info->name, PERSIST_STORE_NAMELEN, name, PERSIST_STORE_NAMELEN - 1) != EOK) {
		pr_err("%s(), strncpy_s fail !\n", __func__);
		kfree(info);
		return;
	}

	info->size = size;
	if (memcpy_s((void *)info->data, info->size, data, size) != EOK) {
		pr_err("%s(): memcpy_s %s failed, size is 0x%lx\n", __func__, info->name, (unsigned long)size);
		kfree(info);
		return;
	}

	pr_info("bbox save persist store:%s, size: 0x%lx\n", info->name, (unsigned long)size);

	raw_spin_lock(&persist_store_lock);
	list_add(&info->node, &__list_persist_store);
	raw_spin_unlock(&persist_store_lock);
}

/*
 * create mntn pstore node for kernel reading
 */
void dfx_create_pstore_entry(void)
{
	struct persist_store_info *info = NULL;
	struct persist_store_info *n = NULL;
	struct proc_dir_entry *pde = NULL;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		pde = dfx_create_pstore_proc_entry(info->name, S_IRUSR | S_IRGRP,
			&g_persist_store_file_fops, info);
		if (!pde) {
			list_del(&info->node);
			kfree(info);
			info = NULL;
		}
	}
}

/*
 * remove mntn pstore node
 */
void dfx_remove_pstore_entry(void)
{
	struct persist_store_info *info = NULL;
	struct persist_store_info *n = NULL;

	list_for_each_entry_safe(info, n, &__list_persist_store, node)
		dfx_remove_pstore_proc_entry(info->name);
}

/*
 * free persist pstore memory
 */
void dfx_free_persist_store(void)
{
	struct persist_store_info *info = NULL;
	struct persist_store_info *n = NULL;
	unsigned int nums = 0;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		list_del(&info->node);
		kfree(info);
		info = NULL;
		nums++;
	}

	pr_info("%s done, quantities is %u\n", __func__, nums);
}

/*
 * register console buffer of psotre for generated rdr_psotre log
*/
void rdr_creat_console_log(void)
{
	struct persist_store_info *info = NULL;
	struct persist_store_info *n = NULL;
	struct proc_dir_entry *pde = NULL;
	void *pstore_addr = NULL;
	void *console_addr = NULL;
	void *last_applog_addr = NULL;

	pstore_addr = bbox_vmap(RESERVED_PSTORE_PHYMEM_BASE,
				RESERVED_PSTORE_PHYMEM_SIZE);
	if (pstore_addr == NULL) {
		pr_err("%s():%d:bbox_vmap pstore_addr fail\n", __func__, __LINE__);
		return;
	}

	last_applog_addr = pstore_addr +
			RESERVED_PSTORE_PHYMEM_SIZE - LAST_APPLOG_SIZE;
	console_addr = last_applog_addr - LAST_KMSG_SIZE;
	dfx_save_pstore_log("rdr_console", (const void *)console_addr, LAST_KMSG_SIZE);
	vunmap(pstore_addr);

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		if (!strncmp(info->name, "rdr_console", strlen("rdr_console"))) {
			pde = dfx_create_pstore_proc_entry((const char *)info->name, S_IRUSR | S_IRGRP,
				&g_persist_store_file_fops, info);
			if (!pde)
				pr_err("creat rdr_console log fail!\n");
			break;
		}
	}
}
void rdr_remove_console_log(void)
{
	struct persist_store_info *info = NULL;
	struct persist_store_info *n = NULL;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		if (!strncmp(info->name, "rdr_console", strlen("rdr_console"))) {
			pr_err("remove rdr_console log !\n");
			dfx_remove_pstore_proc_entry((const char *)info->name);
			list_del(&info->node);
			kfree(info);
			info = NULL;
			break;
		}
	}
}

#ifdef CONFIG_DFX_PSTORE
void registe_info_to_mntndump(char *big_oops_buf, size_t big_oops_buf_sz)
{
	int ret;
	struct mdump_pstore *head;

	if (!big_oops_buf || !big_oops_buf_sz)
		return;

	ret = register_mntn_dump(MNTN_DUMP_PSTORE_RAMOOPS,
			(unsigned int)sizeof(struct mdump_pstore), (void **)&head);
	if (ret) {
		pr_err("register compression buf info fail\n");
		return;
	}
	if (!head) {
		pr_err("head is NULL!\n");
		return;
	}

	head->magic = MNTNDUMP_MAGIC;
	head->ramoops_addr = virt_to_phys(big_oops_buf);
	head->ramoops_size = big_oops_buf_sz;
	head->crc = 0;
	head->crc = checksum32((u32 *)head, sizeof(struct mdump_pstore));
}
#endif
