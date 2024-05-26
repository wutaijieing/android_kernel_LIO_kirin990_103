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
#ifndef MDFX_VISITOR_H
#define MDFX_VISITOR_H

#include "mdfx_logger.h"

#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <platform_include/display/linux/dpu_mdfx.h>

#define DEFAULT_MAX_TRACING_BUF_SIZE     (64 * 1024)
#define DEFAULT_DELETE_HEAD_TRACING_SIZE 1024

#define INVALID_VISITOR_ID (-1)
#define GET_VISITOR_ID(atomic_id)    (atomic_read(&atomic_id))
#define CREATE_VISITOR_ID(atomic_id) (atomic_inc_return(&atomic_id))

struct mdfx_pri_data;

struct mdfx_tracing_item {
	struct list_head node;
	uint32_t msg_len;
	char* msg;
};

// manager a tracing buffer for each visitor
struct mdfx_tracing_buf {
	uint32_t tracing_size;
	struct list_head tracing_list;
};

struct mdfx_visitor_node {
	struct list_head node;
	int64_t id;
	uint64_t type;
	int64_t pid;

	dump_info_func dumper_cb;
	struct mdfx_logger_t logger;
	struct mdfx_tracing_buf tracing[TRACING_TYPE_MAX];
};

struct mdfx_visitors_t {
	atomic_t id;
	struct semaphore visitors_sem;
	struct list_head visitor_list;
};

extern struct mdfx_pri_data *g_mdfx_data;

int64_t mdfx_get_visitor_id(struct mdfx_visitors_t *visitors, uint64_t type);
uint64_t mdfx_get_visitor_type(struct mdfx_visitors_t *visitors, int64_t id);
int64_t mdfx_get_visitor_pid(struct mdfx_visitors_t *visitors, int64_t id);
uint64_t mdfx_visitor_get_tracing_size(struct mdfx_visitors_t *visitors, int64_t id);
struct mdfx_tracing_buf* mdfx_visitor_get_tracing_buf(
	struct mdfx_visitors_t *visitors, int64_t id, uint32_t tracing_type);
inline uint32_t mdfx_visitor_get_max_tracing_buf_size(void);
dump_info_func mdfx_visitor_get_dump_cb(struct mdfx_visitors_t *visitors, int64_t id);
int mdfx_add_visitor(struct mdfx_pri_data *data, void __user *argp);
int mdfx_remove_visitor(struct mdfx_pri_data *data, const void __user *argp);
void mdfx_visitors_init(struct mdfx_visitors_t *visitors);
void mdfx_visitors_deinit(struct mdfx_visitors_t *visitors);
struct mdfx_logger_t* mfx_get_visitor_logger(int64_t id);


#endif
