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

#ifndef MDFX_PRIV_H
#define MDFX_PRIV_H

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/unistd.h>
#include <platform_include/display/linux/dpu_mdfx.h>
#include <linux/printk.h>

#include "mdfx_visitor.h"
#include "mdfx_file.h"
#include "mdfx_event.h"
#include "mdfx_saver.h"
#include "mdfx_actor.h"
#include "mdfx_utils.h"
#include "mdfx_debug.h"

#define FILENAME_LEN      512
#define DIR_PATH_LEN      1024
#define DIR_DEFAULT_MODE  0777  // default mode when creating a file or dir
#define FILE_DEFAULT_MODE 0664
#define SYSTEM_UID        1000
#define ROOT_UID          0
#define SYSTEM_GID        1000
#define MDFX_MAX_ATTRS_NUM 10

#define DFX_FILEDIR         "/data/log/mdfx"
#define DFX_FILEDIR_BACKUP "/data/log/android_logs/mdfx"     // when DFX_FILEDIR or subdirs not exist, store files here
#define DFX_FILEDIR_FOR_USER_SPACE         "/data/vendor/mdfx"
#define MODULE_NAME_DUMPER   "dumper"
#define MODULE_NAME_SAVER   "saver"
#define MODULE_NAME_LOGGER  "logger"
#define MODULE_NAME_TRACING "tracing"
#define DTS_COMP_DFX_NAME   "hisilicon,media_dfx"    /* dts compatible */

/* print for media dfx self */
#define mdfx_pr_info(msg, ...) pr_info("[MDFX][%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__)
#define mdfx_pr_warn(msg, ...) pr_warning("[MDFX][%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__)
#define mdfx_pr_err(msg, ...)  pr_err("[MDFX][%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__)

/*
 * mdfx device private data
 */
struct mdfx_pri_data {
	uint32_t ref_cnt;
	uint32_t mdfx_caps;

	struct mdfx_file_spec file_spec;
	struct mdfx_visitors_t visitors;
	struct mdfx_event_manager_t event_manager;
	struct mdfx_saver_thread_t log_saving_thread;
	struct mdfx_saver_thread_t dump_saving_thread;
	struct mdfx_saver_thread_t trace_saving_thread;
	struct mdfx_actor_t actors[ACTOR_MAX];
	struct mdfx_debugger_t debugger;

	uint32_t attrs_count;
	struct attribute *sysfs_attrs[MDFX_MAX_ATTRS_NUM];
	struct attribute_group attr_group;
	uint32_t var_log_file_count;
	uint32_t var_event_log_file_count;
};

struct mdfx_device_data {
	dev_t devno;
	struct class *dev_class;
	struct device *dev;
	struct device *dfx_cdevice;
	struct cdev cdev;

	struct mdfx_pri_data data;
};

int mdfx_read_dtsi(struct mdfx_device_data *dfx_data);

#endif /* MDFX_PRI_H */
