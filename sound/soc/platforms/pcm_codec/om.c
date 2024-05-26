/*
 * om.c
 *
 * om module for pcm codec
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#include "om.h"

#ifdef AUDIO_PCM_CODEC_DEBUG
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/fs.h>

#include "audio_log.h"
#include "platform_io.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "pcm_codec"
#else
#define LOG_TAG "audio_pcm"
#endif

#define MAX_FILE_NAME_LEN 256
#define DUMP_MAX_FRAME_CNT (50 * 60 * 1000)
#define DUMP_PRINT_INTERVAL 50
#define DUMP_FILE_ACCESS 0640

struct dump_info {
	struct file *fp;
	unsigned int frame_cnt;
	char file_name[MAX_FILE_NAME_LEN];
};

static int g_dump_flag;

static struct dump_info g_dump_info[PCM_DEVICE_TOTAL][DUMP_CNT] = {
	{
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev0_play_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev0_play_dma_32bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev0_record_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev0_record_dma_32bit.pcm" },
	},
	{
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev1_play_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev1_play_dma_32bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev1_record_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev1_record_dma_32bit.pcm" },
	},
	{
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev2_play_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev2_play_dma_32bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev2_record_ap_16bit.pcm" },
		{ NULL, 0, "/data/vendor/log/audio_logs/audio_hook/dev2_record_dma_32bit.pcm" },
	},
};

static void _close_all_dump_file(void)
{
	int i, j;

	for (i = 0; i < PCM_DEVICE_TOTAL; i++) {
		for (j = 0; j < DUMP_CNT; j++) {
			if (g_dump_info[i][j].fp != NULL) {
				filp_close(g_dump_info[i][j].fp, 0);
				g_dump_info[i][j].fp = NULL;
			}
		}
	}
}

static void _open_dump_file(struct dump_info *dump)
{
	int file_flag = O_RDWR | O_CREAT | O_APPEND;
	int fd;

	if (dump->fp != NULL) {
		AUDIO_LOGW("file is opened, file name: %s", dump->file_name);
		return;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	fd = ksys_access(dump->file_name, 0);
#else
	fd = sys_access(dump->file_name, 0);
#endif
	if (fd != 0)
		AUDIO_LOGW("need create dir %s", dump->file_name);

	dump->fp = filp_open(dump->file_name, file_flag, DUMP_FILE_ACCESS);
	if (IS_ERR(dump->fp)) {
		AUDIO_LOGE("open file fail: %s", dump->file_name);
		dump->fp = NULL;
		return;
	}

	AUDIO_LOGI("open file ok:%s, fp:%pK", dump->file_name, dump->fp);
}

static void _open_all_dump_file(void)
{
	int i, j;

	for (i = 0; i < PCM_DEVICE_TOTAL; i++) {
		for (j = 0; j < DUMP_CNT; j++)
			_open_dump_file(&g_dump_info[i][j]);
	}
}

void pcm_codec_set_dump_flag(int flag)
{
	g_dump_flag = flag;
	AUDIO_LOGI("dump flag %d", g_dump_flag);

	if (g_dump_flag == 0)
		_close_all_dump_file();
	else
		_open_all_dump_file();
}
EXPORT_SYMBOL(pcm_codec_set_dump_flag);

void pcm_codec_dump_data(char *buf, unsigned int size, int device, enum DUMP_DATA_TYPE dump_type)
{
	int write_size;
	struct dump_info *dump = NULL;
	mm_segment_t old_fs;

	if (buf == NULL || size == 0) {
		AUDIO_LOGE("buf is null or size is zero");
		return;
	}

	if (device >= PCM_DEVICE_TOTAL || device < 0) {
		AUDIO_LOGE("invalid device:%d", device);
		return;
	}

	if (dump_type >= DUMP_CNT) {
		AUDIO_LOGE("invalid dump_type:%d", dump_type);
		return;
	}

	if (g_dump_flag == 0)
		return;

	dump = &g_dump_info[device][dump_type];
	if (dump->fp == NULL) {
		AUDIO_LOGE("fp is null");
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (dump->frame_cnt < DUMP_MAX_FRAME_CNT) {
		write_size = vfs_write(dump->fp, buf, size, &dump->fp->f_pos);
		if (write_size < 0) {
			AUDIO_LOGE("write file fail addr:%pK size:%d", buf, size);
		} else {
			dump->frame_cnt++;
			if (dump->frame_cnt % DUMP_PRINT_INTERVAL == 0)
				AUDIO_LOGI("writed size:%d cnt:%d, fp:%pK", write_size, dump->frame_cnt, dump->fp);
		}
	}

	set_fs(old_fs);
}

#endif
