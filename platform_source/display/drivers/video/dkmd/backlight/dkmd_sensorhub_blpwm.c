/* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dkmd_sensorhub_blpwm.h"
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include "dkmd_log.h"
#include "dkmd_backlight_common.h"

static int g_blpwm_on;
#define BLPWM_ON 1
#define BLPWM_OFF 0
#define BLPWM_OUT_PRECISION 10000

#define SET_BACKLIGHT_PATH "/sys/devices/platform/huawei_sensor/sbl_setbacklight"

static int dkmd_write_file(struct file *file, const char *buf, uint32_t buf_len)
{
	ssize_t write_len = 0;
	loff_t pos = 0;
	mm_segment_t old_fs;

	if (!buf || !file) {
		dpu_pr_err("[backlight] buf is NULL\n");
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);  /*lint !e501*/

	write_len = vfs_write(file, (char __user *)buf, buf_len, &pos);
	if (write_len <= 0) {
		dpu_pr_err("vfs_write fail");
		return -1;
	}

	pos = 0;
	set_fs(old_fs);
	return 0;
}

static struct file* get_bl_sh_blpwm_file(void *private_data)
{
	struct bl_sh_blpwm_data *data = NULL;

	if (!private_data) {
		dpu_pr_err("private data is null");
		return NULL;
	}

	data = (struct bl_sh_blpwm_data *)private_data;
	return data->file;
}

static int send_bcaklight_to_shb(struct file *file, uint32_t bl_level)
{
	char buffer[32];  /* bl_set_by_sh_blpwm   is true */
	int bytes;

	if (!file) {
		dpu_pr_err("file is null");
		return -1;
	}

	bytes = snprintf(buffer, sizeof(buffer), "T%d\n", bl_level);

	return dkmd_write_file(file, buffer, bytes);
}

static int send_power_state_to_shb(struct file *file, uint32_t power_state)
{
	int ret;
	char buffer[32];  /* power status buff */
	int bytes;

	if (!file) {
		dpu_pr_err("file is null");
		return -1;
	}

	if (power_state == 0) /* off */
		bytes = snprintf(buffer, sizeof(buffer), "F\n");
	else
		bytes = snprintf(buffer, sizeof(buffer), "O\n");

	ret = dkmd_write_file(file, buffer, bytes);
	return ret;
}

static int dkmd_sh_blpwm_set_backlight(struct dkmd_bl_ctrl *bl_ctrl, uint32_t bl_level)
{
	uint32_t bl_max;

	if (!bl_ctrl) {
		dpu_pr_err("priv is NULL\n");
		return -EINVAL;
	}

	bl_max = bl_ctrl->bl_info->bl_max;
	if (!bl_max) {
		dpu_pr_err("BLPWM bl_info->bl_max=%d", bl_max);
		return -1;
	}

	dpu_pr_info("[backlight] shblpwm bl_level=%d", bl_level);

	bl_level = (bl_level * BLPWM_OUT_PRECISION) / bl_max;

	return send_bcaklight_to_shb(get_bl_sh_blpwm_file(bl_ctrl->private_data),
								 bl_level);
}

static int dkmd_sh_blpwm_on(void *private_data)
{
	int ret;

	if (g_blpwm_on == 1)
		return 0;

	ret = send_power_state_to_shb(get_bl_sh_blpwm_file(private_data), BLPWM_ON);
	if (ret != 0) {
		dpu_pr_err("sh_blpwm power on fail");
		return -1;
	}

	g_blpwm_on = 1;
	return ret;
}

static int dkmd_sh_blpwm_off(void *private_data)
{
	int ret;

	if (g_blpwm_on == 0)
		return 0;

	ret = send_power_state_to_shb(get_bl_sh_blpwm_file(private_data), BLPWM_OFF);
	if (ret != 0) {
		dpu_pr_err("sh_blpwm power off fail");
		return -1;
	}

	g_blpwm_on = 0;
	return ret;
}

static void dkmd_sh_blpwm_deinit(struct dkmd_bl_ctrl *bl_ctrl)
{
	struct bl_sh_blpwm_data *data = NULL;

	if (!bl_ctrl->private_data) {
		dpu_pr_err("private data is null");
		return;
	}

	data = (struct bl_sh_blpwm_data *)bl_ctrl->private_data;
	if (data->file)
		filp_close(data->file, NULL);

	kfree(bl_ctrl->private_data);
	bl_ctrl->private_data = NULL;
}

void dkmd_sh_blpwm_init(struct dkmd_bl_ctrl *bl_ctrl)
{
	struct dkmd_bl_ops *bl_ops = NULL;
	struct bl_sh_blpwm_data *data = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("[backlight] bl_ops is NULL");
		return;
	}
	bl_ops = &bl_ctrl->bl_ops;
	bl_ops->on = dkmd_sh_blpwm_on;
	bl_ops->off = dkmd_sh_blpwm_off;
	bl_ops->set_backlight = dkmd_sh_blpwm_set_backlight;
	bl_ops->deinit = dkmd_sh_blpwm_deinit;

	bl_ctrl->private_data = NULL;
	data = kzalloc(sizeof(struct bl_sh_blpwm_data), GFP_KERNEL);
	if (!data) {
		dpu_pr_err("sh blpwm data is null");
		return;
	}

	data->file = filp_open(SET_BACKLIGHT_PATH, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(data->file)) {
		dpu_pr_err("[backlight] filp_open returned:filename %s, error %ld",
			SET_BACKLIGHT_PATH, PTR_ERR(data->file));
		kfree(data);
		return;
	}

	bl_ctrl->private_data = data;
}
