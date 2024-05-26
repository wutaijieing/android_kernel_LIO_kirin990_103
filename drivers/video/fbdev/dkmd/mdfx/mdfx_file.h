/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef MDFX_FILE_H
#define MDFX_FILE_H

#include <linux/errno.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

#define DIR_INVALID (-1)

struct mdfx_redu_file_node {
	struct list_head node;

	__kernel_time_t modify_time;
	char *file_name;
};


void mdfx_file_data_init(struct mdfx_file_spec *file);
int mdfx_file_get_size(const char *filename);
int mdfx_create_dir(const char *path);
int mdfx_remove_redundant_files(const char *dir_path, const char *include_str, int max_file_count);
struct file* mdfx_file_open(const char *filename);
int mdfx_file_write(struct file *file, const char *buf, uint32_t buf_len);
int mdfx_file_read(struct file *file, const char *buf, uint32_t buf_len);
int mdfx_file_save(const char *filename, const char *buf, uint32_t buf_len);
void mdfx_file_get_name(char *filename, const char *modulename, const char *include_str, uint32_t len);
int mdfx_file_query_spec(struct mdfx_pri_data *data, void __user *argp);
void mdfx_file_get_dir_name(char *dir_name, const char *modulename, uint32_t len);

#endif /* MDFX_PRI_H */
