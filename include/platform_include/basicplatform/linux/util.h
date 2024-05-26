/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * util.h
 *
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
#ifndef __MACH_UTIL_H__
#define __MACH_UTIL_H__

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <platform_include/basicplatform/linux/dfx_mntn_switch.h>
#include <linux/version.h>

#define MNTN_SWITCH_VALID_SIZE 64

extern int  check_mntn_switch(int feature);
extern int get_mntn_switch_value(int feature);
extern u32 atoi(char *s);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
struct proc_dir_entry *dfx_create_stats_proc_entry(const char *name, mode_t mode,
				const struct proc_ops *proc_fops, void *data);
void dfx_remove_stats_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_memory_proc_entry(const char *name, mode_t mode,
				const struct proc_ops *proc_fops, void *data);
void dfx_remove_memory_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_log_proc_entry(const char *name, mode_t mode,
				const struct proc_ops *proc_fops, void *data);
void dfx_remove_log_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_pstore_proc_entry(const char *name, mode_t mode,
				const struct proc_ops *proc_fops, void *data);
#ifdef CONFIG_FACTORY_MODE
struct proc_dir_entry *dfx_create_ddrtest_proc_entry(const char *name, mode_t mode,
				onst struct proc_ops *proc_fops, void *data);
void dfx_remove_ddrtest_proc_entry(const char *name);
#endif
#else
struct proc_dir_entry *dfx_create_stats_proc_entry(const char *name, mode_t mode,
				const struct file_operations *proc_fops, void *data);
void dfx_remove_stats_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_memory_proc_entry(const char *name, mode_t mode,
				const struct file_operations *proc_fops, void *data);
void dfx_remove_memory_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_log_proc_entry(const char *name, mode_t mode,
				const struct file_operations *proc_fops, void *data);
void dfx_remove_log_proc_entry(const char *name);

struct proc_dir_entry *dfx_create_pstore_proc_entry(const char *name, mode_t mode,
				const struct file_operations *proc_fops, void *data);
void dfx_remove_pstore_proc_entry(const char *name);

#ifdef CONFIG_FACTORY_MODE
struct proc_dir_entry *dfx_create_ddrtest_proc_entry(const char *name, mode_t mode,
				const struct file_operations *proc_fops, void *data);
#endif
#endif /* END 5.10 */
void dfx_remove_pstore_proc_entry(const char *name);

void create_dump_virt_mem_proc_file(char *name, void *virt_addr, size_t size);

#endif

