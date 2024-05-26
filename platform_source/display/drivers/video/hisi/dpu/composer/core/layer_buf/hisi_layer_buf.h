/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_DISP_LAYER_BUF_H
#define HISI_DISP_LAYER_BUF_H

#include <linux/list.h>
#include <linux/types.h>
#include "hisi_disp_composer.h"
#include "hisi_disp.h"

#ifdef CONFIG_DKMD_DPU_DEBUG
extern int g_debug_layerbuf_log;

#define disp_buf_pr_info(msg, ...)  \
	do { \
		if (g_debug_layerbuf_log) \
			disp_pr_info(msg,  __VA_ARGS__); \
	} while (0)

#define disp_buf_pr_debug(msg, ...)  \
	do { \
		if (g_debug_layerbuf_log) \
			disp_pr_debug(msg,  __VA_ARGS__); \
	} while (0)
#else
#define disp_buf_pr_info(msg, ...)
#define disp_buf_pr_debug(msg, ...)
#endif

struct disp_layer_buf {
	struct list_head list_node;

	struct dma_buf *buf_handle;
	int32_t shared_fd;

};

int hisi_layerbuf_lock_pre_buf(struct list_head *pre_buf_list, struct hisi_display_frame *frame);
int hisi_layerbuf_lock_post_wb_buf(struct list_head *wb_buf_list, struct hisi_display_frame *frame);
void hisi_layerbuf_unlock_buf(struct list_head *buf_list);
void hisi_layerbuf_move_buf_to_tl(struct list_head *buf_list, struct hisi_disp_timeline *timeline);


#endif /* HISI_DISP_LAYER_BUF_H */
