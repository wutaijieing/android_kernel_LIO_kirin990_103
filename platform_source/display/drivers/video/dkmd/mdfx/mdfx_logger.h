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
#ifndef MDFX_LOGGER_H
#define MDFX_LOGGER_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <platform_include/display/linux/dpu_mdfx.h>

#define LOG_ALIGN      __alignof__(struct log_node)
#define LOGGER_BUF_LEN            (128 * 1024)
#define LOGGER_EACH_TEXT_MAX_LEN  512
#define LOGGER_HEADER_MAX_LEN     100
#define LOGGER_BUF_MAX_COUNT      4

#define LOGGER_LOG_NODE_LEN  sizeof(struct log_node)
#define LOGGER_ALIGN_UP(n)  ((n + LOG_ALIGN - 1) & (~(LOG_ALIGN - 1)))
#define LOGGER_GET_BUF_INDEX(index) ((index) % (LOGGER_BUF_MAX_COUNT))

struct log_node {
	uint32_t len;			/* length of entire record */
	uint32_t text_len;		/* length of text buffer */
};

struct mdfx_log_buf_t {
	/** index and sequence number of the first record stored in the buffer */
	uint32_t first_idx;

	/** index and sequence number of the next record to store in the buffer */
	uint32_t next_idx;

	/** record the event name, if event triggle to save log buf */
	char event_name[MDFX_EVENT_NAME_MAX];

	char *log_buf;

	/** each log buf need a lock to aviod multi thread write a same log buf */
	raw_spinlock_t buf_lock;

	int64_t visitor_id;
};

/*
 * buffer copied before writed to file,
 * avoid effections from high I/O time delay to kmsg_buf's immediate updating
 */
struct mdfx_logger_t {
	struct mdfx_log_buf_t log_bufs[LOGGER_BUF_MAX_COUNT];

	uint32_t using_index; // which log buf is using to write msg
	uint32_t saving_index; // which log buf is saving to file
	struct mdfx_log_buf_t *using_log_buf;

	// lock used to sync threads that access same logger
	raw_spinlock_t logger_lock;

	int64_t visitor_id;
};

void mdfx_logger_init(struct mdfx_logger_t *logger, int64_t id);
void mdfx_logger_deinit(struct mdfx_logger_t *logger);


#endif
