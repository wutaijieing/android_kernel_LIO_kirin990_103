/*
 * hwts_channel_manager.c
 *
 * about hwts channel manager
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "hwts_channel_manager.h"
#include "hwts_config.h"
#include "npu_user_common.h"
#include "npu_log.h"

static struct hwts_channel_manager g_hwts_channel_mngr;

static void hwts_channel_init(void)
{
	uint16_t i;
	for (i = 0; i < HWTS_CHANNEL_NUM; ++i) {
		struct hwts_channel *channel = &g_hwts_channel_mngr.channel[i];
		channel->id = i;
		channel->grp_id = MAX_UINT16_NUM;
		channel->sq_id = MAX_UINT16_NUM;
	}
}

static void hwts_group_init(void)
{
	uint16_t i;
	uint16_t start_channel_id;
	uint16_t channel_id;
	struct hwts_group *group = NULL;
	struct hwts_channel *channel = NULL;

	for (i = 0; i < HWTS_GROUP_NUM; ++i) {
		group = &g_hwts_channel_mngr.group[i];
		group->id = i;
		group->free_num = hwts_config_get_group_channel_count(i);
		INIT_LIST_HEAD(&(group->idle_channel_list));
		INIT_LIST_HEAD(&(group->work_channel_list));

		start_channel_id = hwts_config_get_group_start_channel_id(i);
		for (channel_id = 0; channel_id < group->free_num; ++channel_id) {
			channel = hwts_mngr_get_channel(channel_id + start_channel_id);
			channel->grp_id = i;
			list_add_tail(&(channel->node), &(group->idle_channel_list));
		}
	}
}

void hwts_channel_mngr_init (void)
{
	hwts_channel_init();
	hwts_group_init();
}

struct hwts_channel *hwts_mngr_get_channel(uint16_t channel_id)
{
	if (channel_id >= HWTS_CHANNEL_NUM) {
		npu_drv_warn("channel_id is invalid, id:%u", channel_id);
		return NULL;
	}

	return &(g_hwts_channel_mngr.channel[channel_id]);
}

struct hwts_channel *hwts_channel_mngr_alloc_channel(uint16_t group_id)
{
	struct hwts_group *group = NULL;
	struct hwts_channel *channel = NULL;

	if (group_id >= HWTS_GROUP_NUM) {
		npu_drv_warn("group_id is invalid, id:%u", group_id);
		return NULL;
	}

	group = &(g_hwts_channel_mngr.group[group_id]);
	if (group->free_num == 0) {
		npu_drv_warn("there is no idle hwts channel to be alloced");
		return NULL;
	}

	channel = list_first_entry(&(group->idle_channel_list), struct hwts_channel, node);
	list_move_tail(&(channel->node), &(group->work_channel_list));
	group->free_num--;

	npu_drv_info("alloc hwts channel. channel_id = %u", channel->id);
	return channel;
}

void hwts_channel_mngr_free_channel(struct hwts_channel *channel)
{
	if (channel == NULL) {
		npu_drv_warn("channel is NULL");
		return;
	}

	uint16_t group_id = channel->grp_id;
	struct hwts_group *group = &(g_hwts_channel_mngr.group[group_id]);

	channel->sq_id = MAX_UINT16_NUM;
	list_move_tail(&(channel->node), &(group->idle_channel_list));
	group->free_num++;

	npu_drv_info("free hwts channel. channel_id = %u", channel->id);
}

uint16_t hwts_channel_get_free_count(uint16_t group_id)
{
	if (group_id >= HWTS_GROUP_NUM) {
		npu_drv_warn("group_id is invalid, id:%u", group_id);
		return NULL;
	}

	return g_hwts_channel_mngr.group[group_id].free_num;
}
