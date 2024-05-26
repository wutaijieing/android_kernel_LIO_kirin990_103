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

#ifndef MDFX_UTILS_H
#define MDFX_UTILS_H

#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/errno.h>

#define MAX_FILE_NAME_LEN 512

#define PRIx64 "llx"
#define PRIu64 "llu"

#define log_and_return_if(cond, msg) \
	do { \
		if (cond) { \
			mdfx_pr_err(msg); \
			return; \
		} \
	} while(0)

struct dfx_time {
	struct tm tm_rtc;
	struct timeval tv;
};

struct mdfx_pri_data;

struct dfx_time mdfx_get_rtc_time(void);
void mdfx_sysfs_init(struct mdfx_pri_data *mdfx_data);
void mdfx_sysfs_append_attrs(struct mdfx_pri_data *mdfx_data, struct attribute *attr);
void mdfx_sysfs_create_attrs(struct platform_device *pdev);
void mdfx_sysfs_remove_attrs(struct platform_device *pdev);
uint32_t mdfx_get_bits(uint64_t value);
uint32_t mdfx_get_msg_header(char *buf, uint32_t len);
bool mdfx_user_mode(void);
int mdfx_query_sysinfo(struct mdfx_pri_data *mdfx_data, void __user *argp);


#endif /* MDFXUTILS_H */
