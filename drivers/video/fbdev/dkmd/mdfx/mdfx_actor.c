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
#include "mdfx_priv.h"
#include "mdfx_actor.h"

static uint32_t mdfx_get_caps_of_actor(uint32_t type)
{
	switch (type) {
	case ACTOR_DUMPER:
		return MDFX_CAP_DUMPER;
	case ACTOR_TRACING:
		return MDFX_CAP_TRACING;
	case ACTOR_LOGGER:
		return MDFX_CAP_LOGGER;
	case ACTOR_SAVER:
		return MDFX_CAP_SAVER;
	default:
		return MDFX_CAP_NULL;
	}
}

void mdfx_actors_init(struct mdfx_pri_data *mdfx_data)
{
	uint32_t i;
	struct mdfx_actor_ops_t *actor_ops = NULL;

	if (IS_ERR_OR_NULL(mdfx_data))
		return;

	mdfx_dumper_init_actor(&mdfx_data->actors[ACTOR_DUMPER]);
	mdfx_tracing_init_actor(&mdfx_data->actors[ACTOR_TRACING]);
	mdfx_logger_init_actor(&mdfx_data->actors[ACTOR_LOGGER]);

	for (i = ACTOR_DUMPER; i < ACTOR_MAX; i++) {
		actor_ops = mdfx_data->actors[i].ops;
		if (actor_ops && actor_ops->act)
			mdfx_event_register_listener(i, actor_ops->act);
	}
}

int mdfx_actor_do_ioctl(struct mdfx_pri_data *mdfx_data, const void __user *argp, uint32_t type)
{
	struct mdfx_actor_t* actor = NULL;

	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(argp))
		return -1;

	if (type >= ACTOR_MAX)
		return -1;

	if (!MDFX_HAS_CAPABILITY(mdfx_data->mdfx_caps, mdfx_get_caps_of_actor(type)))
		return 0;

	actor = &mdfx_data->actors[type];
	if (actor->ops && actor->ops->do_ioctl)
		return actor->ops->do_ioctl(mdfx_data, argp);

	return 0;
}



