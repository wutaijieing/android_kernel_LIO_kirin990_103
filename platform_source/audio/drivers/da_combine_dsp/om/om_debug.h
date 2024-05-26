/*
 * om_debug.h
 *
 * debug for da_combine codec
 *
 * Copyright (c) 2015 Huawei Technologies Co., Ltd.
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

#ifndef __DA_COMBINE_DSP_OM_DEBUG_H__
#define __DA_COMBINE_DSP_OM_DEBUG_H__

#include <linux/platform_drivers/da_combine_dsp/da_combine_dsp_misc.h>

int da_combine_dsp_debug_init(const struct da_combine_dsp_config *config);
void da_combine_dsp_debug_deinit(void);
ssize_t da_combine_dsp_misc_read(struct file *file, char __user *buf,
	size_t nbytes, loff_t *pos);
ssize_t da_combine_dsp_misc_write(struct file *file,
	const char __user *buf, size_t nbytes, loff_t *pos);
unsigned int da_combine_read_mlib(unsigned char *arg, unsigned int len);
int da_combine_dsp_get_dmesg(const void __user *arg);
int da_combine_save_dsp_log(const void *data);
int da_combine_check_memory(const struct da_combine_param_io_buf *param);
int da_combine_wakeup_suspend_handle(const void *data);
int da_combine_set_wakeup_hook(const struct da_combine_param_io_buf *param);
void da_combine_wakeup_pcm_hook_start(void);
void da_combine_wakeup_pcm_hook_stop(void);
void da_combine_adjust_log_sequence(char *log_buf, const size_t log_size);

#endif
