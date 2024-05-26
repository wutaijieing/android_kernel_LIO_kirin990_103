/*
 * linux_proc.c
 *
 * This is for linux_proc impl.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "vfmw_osal.h"
#include "linux_proc.h"
#include "linux_osal.h"

#define PROC_BUFFER_SIZE 512
static proc_ops g_proc_items[PROC_BUFFER_SIZE];

int32_t linux_proc_open(struct inode *inode, struct file *file)
{
	proc_ops *proc = NULL;

	if (!inode || !file)
		return -ENOSYS;

	proc = PDE_DATA(inode);
	if (!proc)
		return -ENOSYS;

	if (proc->read)
		return single_open(file, proc->read, proc);

	return -ENOSYS;
}

ssize_t linux_proc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	struct seq_file *s = VCODEC_NULL;
	proc_ops *proc = VCODEC_NULL;

	if (!file || !buf || !ppos)
		return -ENOSYS;

	s = file->private_data;
	proc = s->private;

	if (proc->write)
		return proc->write(file, buf, count, ppos);

	return -ENOSYS;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops g_fops = {
	.proc_open = linux_proc_open,
	.proc_read = seq_read,
	.proc_write = linux_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations g_fops = {
	.owner = THIS_MODULE,
	.open = linux_proc_open,
	.read = seq_read,
	.write = linux_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int32_t linux_proc_create(uint8_t *proc_name, void *read, void *write)
{
	uint8_t asz_buf[16];
	int32_t str_ret;
	uint32_t i;
	uint32_t count;
	struct proc_dir_entry *entry = VCODEC_NULL;
	proc_ops *pst_item = VCODEC_NULL;

	count = sizeof(g_proc_items) / sizeof(g_proc_items[0]);
	if (!proc_name)
		return -1;

	str_ret = snprintf_s(asz_buf, sizeof(asz_buf),
		sizeof(asz_buf) - 1, "%s", proc_name);
	if (str_ret < 0)
		return -1;

	for (i = 0; i < count; i++) {
		if ((!g_proc_items[i].read) && (!g_proc_items[i].write)) {
			pst_item = &g_proc_items[i];

			(void)strncpy_s(pst_item->proc_name, sizeof(pst_item->proc_name),
				proc_name, sizeof(pst_item->proc_name) - 1);
			pst_item->proc_name[sizeof(pst_item->proc_name) - 1] = 0;

			pst_item->read = (proc_read_fn)read;
			pst_item->write = (proc_write_fn)write;
			break;
		}
	}

	if (i >= count) {
		OS_PRINT("ERROR: add %s proc entry fail over LIMIT!\n", proc_name);
		return -1;
	}

	entry = proc_create_data(proc_name, 0644, VCODEC_NULL, &g_fops, (void *)pst_item); // 0644: File Permissions
	if (!entry) {
		OS_PRINT("Create %s proc entry fail!\n", proc_name);
		return -1;
	}

	if (!pst_item) {
		OS_PRINT("Create %s proc entry fail!\n", proc_name);
		return -1;
	}

	return 0;
}

void linux_proc_destroy(uint8_t *proc_name)
{
	uint8_t asz_buf[16];
	int32_t str_ret;
	uint32_t i, count;

	count = sizeof(g_proc_items) / sizeof(g_proc_items[0]);

	if (!proc_name)
		return;

	str_ret = snprintf_s(asz_buf, sizeof(asz_buf),
		sizeof(asz_buf) - 1, "%s", proc_name);
	if (str_ret < 0)
		return;

	for (i = 0; i < count; i++) {
		proc_ops *pst_item = &g_proc_items[i];

		if (strncmp(proc_name, pst_item->proc_name,
			sizeof(pst_item->proc_name)) == 0) {
			remove_proc_entry(proc_name, VCODEC_NULL);
			pst_item->read = VCODEC_NULL;
			pst_item->write = VCODEC_NULL;
			break;
		}
	}
}

void linux_proc_dump(void *page, int32_t page_count, int32_t *used_bytes,
	int8_t from_shr, const int8_t *format, ...)
{
	OS_VA_LIST arg_list;
	int32_t total_char;
	int8_t str[PROC_BUFFER_SIZE];

	if (from_shr != 0) {
		seq_printf((struct seq_file *)page, "%s", format);
		return;
	}

	OS_VA_START(arg_list, format);
	total_char = vsnprintf_s(str, PROC_BUFFER_SIZE, PROC_BUFFER_SIZE - 1,
		format, arg_list);
	OS_VA_END(arg_list);

	if (total_char < 0)
		return;

	seq_printf((struct seq_file *)page, "%s", str);
}

int32_t linux_proc_init(void)
{
	return 0;
}

void linux_proc_exit(void)
{
}
