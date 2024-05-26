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
#ifndef MDFX_TRACING_H
#define MDFX_TRACING_H

#include "mdfx_visitor.h"

extern struct mdfx_pri_data *g_mdfx_data;

void mdfx_tracing_free_buf_list(struct mdfx_tracing_buf *tracing_buf);

#endif
