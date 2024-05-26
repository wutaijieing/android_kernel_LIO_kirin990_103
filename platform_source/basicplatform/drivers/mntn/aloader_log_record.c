/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * record aloaderlog into filesystem when booting kernel
 *
 * aloader_log_record.c
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
#include "aloader_log_record.h"
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>   /* For copy_to_user */
#include <linux/vmalloc.h>   /* For vmalloc */
#include <linux/module.h>
#include <platform_include/basicplatform/linux/util.h> /* For mach_call_usermoduleshell */
#include <platform_include/basicplatform/linux/pr_log.h>
#include <global_ddr_map.h>
#include <securec.h>
#include <mntn_public_interface.h>

#include "blackbox/rdr_print.h"
#include "blackbox/platform_ap/rdr_ap_exception_logsave.h"

#define PR_LOG_TAG	ALOADER_LOG_TAG

static char *s_aloaderlog_buff = NULL;
static size_t s_aloaderlog_size;
static char *s_last_aloaderlog_buff = NULL;
static size_t s_last_aloaderlog_size;

static ssize_t last_aloaderlog_dump_file_read(struct file *file,
					char __user *userbuf,
					size_t bytes, loff_t *off)
{
	ssize_t copy;

	if (userbuf == NULL || off == NULL)
		return 0;

	if (*off >= (loff_t)s_last_aloaderlog_size) {
		BB_PRINT_ERR("%s: read offset error\n", __func__);
		return 0;
	}

	copy = (ssize_t) min(bytes, (size_t) (s_last_aloaderlog_size - *off));

	if (copy_to_user(userbuf, s_last_aloaderlog_buff + *off, copy)) {
		BB_PRINT_ERR("%s: copy to user error\n", __func__);
		return -EFAULT;
	}

	*off += copy;

	return copy;
}

static ssize_t aloaderlog_dump_file_read(struct file *file,
					char __user *userbuf, size_t bytes,
					loff_t *off)
{
	ssize_t copy;

	if (userbuf == NULL || off == NULL)
		return 0;

	if (*off >= (loff_t)s_aloaderlog_size) {
		BB_PRINT_ERR("%s: read offset error\n", __func__);
		return 0;
	}

	copy = (ssize_t) min(bytes, (size_t) (s_aloaderlog_size - *off));

	if (copy_to_user(userbuf, s_aloaderlog_buff + *off, copy)) {
		BB_PRINT_ERR("%s: copy to user error\n", __func__);
		return -EFAULT;
	}

	*off += copy;

	return copy;
}

static const struct proc_ops last_aloaderlog_dump_file_fops = {
	.proc_read = last_aloaderlog_dump_file_read,
};

static const struct proc_ops aloaderlog_dump_file_fops = {
	.proc_read = aloaderlog_dump_file_read,
};

static void aloader_logger_dump(char *start, unsigned int size,
				const char *str)
{
	unsigned int i;
	char *p = start;

	if (p == NULL)
		return;

	pr_info("*****************************%s_aloader_log begin*****************************\n", str);
	for (i = 0; i < size; i++) {
		if (start[i] == '\0')
			start[i] = ' ';
		if (start[i] == '\n') {
			start[i] = '\0';
			pr_info("%s_aloader_log: %s\n", str, p);
			start[i] = '\n';
			p = &start[i + 1];
		}
	}
	pr_info("******************************%s_aloader_log end******************************\n", str);
}

void mntn_dump_aloaderlog(void)
{
	if (s_last_aloaderlog_buff != NULL)
		aloader_logger_dump(s_last_aloaderlog_buff,
				       s_last_aloaderlog_size, "last");

	if (s_aloaderlog_buff != NULL)
		aloader_logger_dump(s_aloaderlog_buff, s_aloaderlog_size,
				       "current");
}

static bool need_dump_whole_log(struct aloaderlog_head *head)
{
	if (head->magic != ALOADER_MAGIC) {
		BB_PRINT_ERR("%s: aloaderlog magic number incorrect\n",
		       __func__);
		return true;
	}

	if (head->lastlog_start >= ALOADER_DUMP_LOG_SIZE ||
		head->lastlog_start < sizeof(*head) ||
		head->lastlog_offset >= ALOADER_DUMP_LOG_SIZE ||
		head->lastlog_offset < sizeof(*head)) {
		BB_PRINT_ERR("%s: last_aloaderlog offset incorrect\n",
		       __func__);
		return true;
	}

	if (head->log_start >= ALOADER_DUMP_LOG_SIZE ||
		head->log_start < sizeof(*head) ||
		head->log_offset >= ALOADER_DUMP_LOG_SIZE ||
		head->log_offset < sizeof(*head)) {
		BB_PRINT_ERR("%s: aloaderlog offset incorrect\n",
		       __func__);
		return true;
	}

	return false;
}

void save_aloader_log(const char *dst_dir_str)
{
	int ret;
	u32 len;
	char aloaderlog_path[NEXT_LOG_PATH_LEN];
	char last_aloaderlog_path[NEXT_LOG_PATH_LEN];

	if (dst_dir_str == NULL){
		BB_PRINT_ERR("%s():%d:dst_dir_str is NULL!\n", __func__, __LINE__);
		return;
	}

	/* Absolute path of the aloaderlog file */
	(void)memset_s(last_aloaderlog_path, NEXT_LOG_PATH_LEN, 0, NEXT_LOG_PATH_LEN);
	(void)memset_s(aloaderlog_path, NEXT_LOG_PATH_LEN, 0, NEXT_LOG_PATH_LEN);
	len = strlen(dst_dir_str);

	ret = memcpy_s(last_aloaderlog_path, NEXT_LOG_PATH_LEN - 1, dst_dir_str, len);
	if (ret != EOK) {
		BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
		return;
	}
	ret = memcpy_s(&last_aloaderlog_path[len], NEXT_LOG_PATH_LEN - strlen(last_aloaderlog_path),
			"/last_aloader_log", strlen("/last_aloader_log") + 1);
	if (ret != EOK) {
		BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
		return;
	}

	ret = memcpy_s(aloaderlog_path, NEXT_LOG_PATH_LEN - 1, dst_dir_str, len);
	if (ret != EOK) {
		BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
		return;
	}
	ret = memcpy_s(&aloaderlog_path[len], NEXT_LOG_PATH_LEN - strlen(aloaderlog_path), "/aloader_log",
			strlen("/aloader_log") + 1);
	if (ret != EOK) {
		BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
		return;
	}

	/* Generate the last_aloaderlog file */
	ret = rdr_copy_file_apend(last_aloaderlog_path, LAST_ALOADER_LOG_FILE);
	if (ret)
		BB_PRINT_ERR("rdr_copy_file_apend [%s] fail, ret = [%d]\n", LAST_ALOADER_LOG_FILE, ret);

	/* Generate the curr_aloaderlog file */
	ret = rdr_copy_file_apend(aloaderlog_path, ALOADER_LOG_FILE);
	if (ret)
		BB_PRINT_ERR("rdr_copy_file_apend [%s] fail, ret = [%d]\n", ALOADER_LOG_FILE, ret);
}

static int dump_init_save_log(struct aloaderlog_head *head, const char *aloaderlog_buff)
{
	const char *log_start = NULL;
	unsigned int log_size;
	unsigned int tmp_len;
	int ret;

	log_start = aloaderlog_buff + head->log_start;
	if (head->log_offset < head->log_start) {
		tmp_len = ALOADER_DUMP_LOG_SIZE - head->log_start;
		log_size = tmp_len + head->log_offset - sizeof(*head);

		s_aloaderlog_buff = vmalloc(log_size);
		if (s_aloaderlog_buff == NULL) {
			BB_PRINT_ERR(
				"%s: fail to vmalloc %#x bytes s_aloaderlog_buff\n",
				__func__, log_size);
			return -1;
		}
		ret = memcpy_s(s_aloaderlog_buff, log_size, log_start, tmp_len);
		if (ret != EOK) {
			BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
			vfree(s_aloaderlog_buff);
			s_aloaderlog_buff = NULL;
			return -1;
		}
		log_start = aloaderlog_buff + sizeof(*head);
		ret = memcpy_s(s_aloaderlog_buff + tmp_len, log_size - tmp_len, log_start,
				log_size - tmp_len);
		if (ret != EOK) {
			BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
			vfree(s_aloaderlog_buff);
			s_aloaderlog_buff = NULL;
			return -1;
		}
		s_aloaderlog_size = log_size;
	} else {
		log_size = head->log_offset - head->log_start;
		if (log_size > 0) {
			s_aloaderlog_buff = vmalloc(log_size);
			if (s_aloaderlog_buff == NULL) {
				BB_PRINT_ERR(
					"%s: fail to vmalloc %#x bytes s_aloaderlog_buff\n",
					__func__, log_size);
				return -1;
			}
			ret = memcpy_s(s_aloaderlog_buff, log_size, log_start, log_size);
			if (ret != EOK) {
				BB_PRINT_ERR("%s():%d:memcpy_s fail!\n", __func__, __LINE__);
				vfree(s_aloaderlog_buff);
				s_aloaderlog_buff = NULL;
				return -1;
			}
			s_aloaderlog_size = log_size;
		}
	}

	return 0;
}

static int dump_init_save_lastlog(struct aloaderlog_head *head, const char *aloaderlog_buff)
{
	const char *lastlog_start = NULL;
	unsigned int lastlog_size;
	unsigned int tmp_len;

	lastlog_start = aloaderlog_buff + head->lastlog_start;
	if (head->lastlog_offset < head->lastlog_start) {
		tmp_len = ALOADER_DUMP_LOG_SIZE - head->lastlog_start;
		lastlog_size = tmp_len + head->lastlog_offset - sizeof(*head);

		s_last_aloaderlog_buff = vmalloc(lastlog_size);
		if (s_last_aloaderlog_buff == NULL) {
			BB_PRINT_ERR(
				"%s: fail to vmalloc %#x bytes s_last_aloaderlog_buff\n",
				__func__, lastlog_size);
			return -1;
		}

		if (memcpy_s(s_last_aloaderlog_buff, lastlog_size, lastlog_start, tmp_len) != EOK) {
			BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
			vfree(s_last_aloaderlog_buff);
			s_last_aloaderlog_buff = NULL;
			return -1;
		}

		lastlog_start = aloaderlog_buff + sizeof(*head);

		if (memcpy_s(s_last_aloaderlog_buff + tmp_len, lastlog_size - tmp_len, lastlog_start,
			lastlog_size - tmp_len) != EOK) {
			BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
			vfree(s_last_aloaderlog_buff);
			s_last_aloaderlog_buff = NULL;
			return -1;
		}

		s_last_aloaderlog_size = lastlog_size;
	} else {
		lastlog_size = head->lastlog_offset - head->lastlog_start;
		if (lastlog_size > 0) {
			s_last_aloaderlog_buff = vmalloc(lastlog_size);
			if (s_last_aloaderlog_buff == NULL) {
				BB_PRINT_ERR(
					"%s: fail to vmalloc %#x bytes s_last_aloaderlog_buff\n",
					__func__, lastlog_size);
				return -1;
			}
			if (memcpy_s(s_last_aloaderlog_buff, lastlog_size, lastlog_start, lastlog_size) != EOK) {
				BB_PRINT_ERR("[%s:%d]: memcpy_s err\n]", __func__, __LINE__);
				vfree(s_last_aloaderlog_buff);
				s_last_aloaderlog_buff = NULL;
				return -1;
			}

			s_last_aloaderlog_size = lastlog_size;
		}
	}

	return 0;
}

/*
 * Description :  aloaderlog init
 *                check emmc leaves log to record
 *                if there is log, launch work queue, and create /proc/rdr/log/aloader_log
 * Input:         NA
 * Output:        NA
 * Return:        OK:success
 */
static int __init aloaderlog_dump_init(void)
{
	char *aloaderlog_buff = NULL;
	struct aloaderlog_head *head = NULL;
	bool use_ioremap = false;
	int ret = 0;

	if (!check_mntn_switch(MNTN_GOBAL_RESETLOG))
		return ret;

	if (pfn_valid(__phys_to_pfn(ALOADER_DUMP_LOG_ADDR))) {
		aloaderlog_buff = phys_to_virt(ALOADER_DUMP_LOG_ADDR);
	} else {
		use_ioremap = true;
		aloaderlog_buff =
		    ioremap_wc(ALOADER_DUMP_LOG_ADDR, ALOADER_DUMP_LOG_SIZE);
	}
	if (aloaderlog_buff == NULL) {
		BB_PRINT_ERR("%s: fail to get the virtual address of aloaderlog\n", __func__);
		return -1;
	}

	head = (struct aloaderlog_head *)aloaderlog_buff;

	if (need_dump_whole_log(head)) {
		head->lastlog_start = 0;
		head->lastlog_offset = 0;
		head->log_start = 0;
		head->log_offset = ALOADER_DUMP_LOG_SIZE;
	}

	ret = dump_init_save_lastlog(head, aloaderlog_buff);
	if (ret == -1)
		goto out;

	ret = dump_init_save_log(head, aloaderlog_buff);
	if (ret == -1)
		goto out;
out:
	if (use_ioremap && aloaderlog_buff != NULL)
		iounmap(aloaderlog_buff);

	if (s_last_aloaderlog_buff != NULL)
		dfx_create_log_proc_entry("last_aloader_log", S_IRUSR | S_IRGRP,
					     &last_aloaderlog_dump_file_fops, NULL);

	if (s_aloaderlog_buff != NULL)
		dfx_create_log_proc_entry("aloader_log", S_IRUSR | S_IRGRP,
					     &aloaderlog_dump_file_fops, NULL);

	return ret;
}

module_init(aloaderlog_dump_init);

/*
 * Description :  aloaderlog exit
 *                destroy the workqueue
 * Input:         NA
 * Output:        NA
 * Return:        NA
 */
static void __exit aloaderlog_dump_exit(void)
{
	dfx_remove_log_proc_entry("aloader_log");
	dfx_remove_log_proc_entry("last_aloader_log");

	if (s_last_aloaderlog_buff != NULL) {
		vfree(s_last_aloaderlog_buff);
		s_last_aloaderlog_buff = NULL;
	}

	s_last_aloaderlog_size = 0;
	if (s_aloaderlog_buff != NULL) {
		vfree(s_aloaderlog_buff);
		s_aloaderlog_buff = NULL;
	}
	s_aloaderlog_size = 0;
}

module_exit(aloaderlog_dump_exit);
