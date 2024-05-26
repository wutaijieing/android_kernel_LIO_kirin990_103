/*
 * teek_app_load.c
 *
 * function declaration for load app operations for kernel CA.
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "teek_app_load.h"
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "session_manager.h"
#include "ko_adapt.h"

static int32_t teek_open_app_file(struct file *fp, char **file_buf, uint32_t total_img_len)
{
	loff_t pos = 0;
	mm_segment_t old_fs;
	uint32_t read_size;
	char *file_buffer = NULL;

	if (total_img_len == 0 || total_img_len > MAX_IMAGE_LEN) {
		tloge("img len is invalied %u\n", total_img_len);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	file_buffer = vmalloc(total_img_len);
	if (!file_buffer) {
		tloge("alloc TA file buffer(size=%u) failed\n", total_img_len);
		return TEEC_ERROR_GENERIC;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	read_size = (uint32_t)koadpt_vfs_read(fp, file_buffer, total_img_len, &pos);

	set_fs(old_fs);

	if (read_size != total_img_len) {
		tloge("read ta file failed, read size/total size=%u/%u\n", read_size, total_img_len);
		vfree(file_buffer);
		return TEEC_ERROR_GENERIC;
	}

	*file_buf = file_buffer;

	return TEEC_SUCCESS;
}

static int32_t teek_read_app(const char *load_file, char **file_buf, uint32_t *file_len)
{
	int32_t ret;
	struct file *fp = NULL;

	fp = filp_open(load_file, O_RDONLY, 0);
	if (!fp || IS_ERR(fp)) {
		tloge("open file error %ld\n", PTR_ERR(fp));
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	*file_len = (uint32_t)(fp->f_inode->i_size);

	ret = teek_open_app_file(fp, file_buf, *file_len);
	if (ret != TEEC_SUCCESS)
		tloge("do read app fail\n");

	if (fp != NULL) {
		filp_close(fp, 0);
		fp = NULL;
	}

	return ret;
}

void teek_free_app(bool load_app_flag, char **file_buf)
{
	if (load_app_flag && file_buf != NULL && *file_buf != NULL) {
		vfree(*file_buf);
		*file_buf = NULL;
	}
}

int32_t teek_get_app(const char *ta_path, char **file_buf, uint32_t *file_len)
{
	int32_t ret;

	/* ta path is NULL or user CA means no need to load TA */
	if (!ta_path || current->mm != NULL)
		return TEEC_SUCCESS;

	if (!file_buf || !file_len) {
		tloge("load app params invalied\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	ret = teek_read_app(ta_path, file_buf, file_len);
	if (ret != TEEC_SUCCESS)
		tloge("teec load app error %d\n", ret);

	return ret;
}
