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
#ifndef MDFX_ACTOR_H
#define MDFX_ACTOR_H

#include <linux/types.h>
#include "mdfx_event.h"

struct mdfx_pri_data;

typedef int (*ioctl_func)(struct mdfx_pri_data *mdfx_data, const void __user *argp);

struct mdfx_actor_ops_t {
	listener_act_func act;
	ioctl_func do_ioctl;
};

struct mdfx_actor_t {
	uint32_t actor_type;
	struct mdfx_actor_ops_t *ops;
};

void mdfx_actors_init(struct mdfx_pri_data *mdfx_data);
int mdfx_actor_do_ioctl(struct mdfx_pri_data *mdfx_data, const void __user *argp, uint32_t type);
void mdfx_tracing_init_actor(struct mdfx_actor_t *actor);
void mdfx_dumper_init_actor(struct mdfx_actor_t *actor);
void mdfx_logger_init_actor(struct mdfx_actor_t *actor);


#endif
