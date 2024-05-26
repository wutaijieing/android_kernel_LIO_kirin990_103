/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "dkmd_log.h"

#include "cmdlist_interface.h"
#include "chash.h"
#include "cmdlist_node.h"
#include "cmdlist_client.h"
#include "cmdlist_drv.h"
#include "dkmd_acquire_fence.h"

#define BUF_SYNC_TIMEOUT_MSEC (1 * MSEC_PER_SEC)
#define ITEM_CHECK_SLIDE_NUM 4
#define ITEM_CHECK_TOTAL_NUM 64

static struct clist_head *get_valid_cmdlist_header(uint32_t scene_id, uint32_t cmdlist_id)
{
	struct cmdlist_client *client = NULL;
	struct cmdlist_table *cmd_table = &g_cmdlist_priv.cmd_table;

	client = cmdlist_get_current_client(cmd_table, scene_id, cmdlist_id);
	if (unlikely(!client)) {
		dpu_pr_err("invalid scene_id:%u, cmdlist_id:%u!", scene_id, cmdlist_id);
		return NULL;
	}

	if (unlikely(client->node_type != SCENE_NOP_TYPE)) {
		dpu_pr_err("invalid scene header client type=%d!", client->node_type);
		return NULL;
	}

	if (unlikely(!client->node)) {
		dpu_pr_err("header client node is null!");
		return NULL;
	}

	return client->node->list_node;
}

/* To check cmdlist items constraint: the sum of 4(ITEM_CHECK_SLIDE_NUM) consecutive nodes' item should no
 * less than 64(ITEM_CHECK_TOTAL_NUM)
 */
static int32_t cmdlist_check_cmd_items_valid(struct clist_head *list_header)
{
	uint32_t node_count = 0;
	uint32_t item_count = 0;
	struct cmdlist_node *node = NULL, *_node_ = NULL;
	struct cmdlist_node *slide_right = NULL;
	struct cmdlist_node *slide_left = clist_first_entry(list_header, typeof(*slide_left), cnode);

	if (unlikely(!slide_left))
		return 0;

	clist_for_each_entry_safe(node, _node_, list_header, typeof(*node), cnode) {
		++node_count;
		item_count += node->list_cmd_header->total_items.bits.items_count;
		if (node_count == ITEM_CHECK_SLIDE_NUM)
			break;
	}

	// total node number is less than ITEM_CHECK_SLIDE_NUM, match the constraint
	if (node_count < ITEM_CHECK_SLIDE_NUM)
		return 0;

	if (item_count < ITEM_CHECK_TOTAL_NUM) {
		dpu_pr_err("First %d nodes' total item number is less than %d", ITEM_CHECK_SLIDE_NUM, ITEM_CHECK_TOTAL_NUM);
		return -1;
	}

	// node now is at slide right
	for (slide_right = cnode_entry(&node->cnode, typeof(*node), cnode);
		!clist_is_tail(list_header, &slide_right->cnode);
		slide_right = clist_next_entry(slide_right, list_header, typeof(*node), cnode)) {
		++node_count;
		slide_left = clist_next_entry(slide_left, list_header, typeof(*node), cnode);
		item_count -= slide_left->list_cmd_header->total_items.bits.items_count;
		item_count += slide_right->list_cmd_header->total_items.bits.items_count;
		if (item_count < ITEM_CHECK_TOTAL_NUM) {
			dpu_pr_err("%d nodes' total item number beginning at node %u is less than %d",
				ITEM_CHECK_SLIDE_NUM, node_count, ITEM_CHECK_TOTAL_NUM);
			return -1;
		}
	}

	return 0;
}

static void dkmd_cmdlist_exec_hardware_link(struct clist_head *list_header)
{
	struct cmdlist_node *node = NULL, *_node_ = NULL;
	struct cmdlist_node *prev_node = NULL;
	struct cmdlist_client *client = NULL;

	int32_t retval = cmdlist_check_cmd_items_valid(list_header);
	dpu_assert(retval);

	/**
	* Block scenarios exist multiple DM nodes, each DM node corresponding to
	* the task of a complete so each DM node in a node need to configure
	* the task end flag.
	*
	*  scene_header
	*       ├── DM_node0
	*       │     └── node ── ... ── node(task end)
	*       │
	*       └── DM_node1 --- cldx --- sclx (reuse operator isn't need to link)
	*              │          └── node --- node
	*              │
	*              └── node ── ... ── node(task end)
	*
	* Here are some constraints:
	* 1. A task cannot have short continuous multiple nodes.
	*    When a task include multiple configuration node, arbitrary continuous
	*    four nodes cmd_item is greater than the sum of 64, or it requires nodes
	*    cmd_item is not greater than 4.
	* 2. Command node contains the number of cmd_items (namely the total_items
	*    cmd_header) cannot be equal to 1, if there are any cmd_item only one,
	*    you can copy the data, and make total_items equal to 2.
	* 3. "Configuration mode" and "transport node", only switch once, can't switch
	*    over and over again, Generally, transport configuration node is always in
	*    the front.
	* 4. Using a one-dimensional transport mode, must ensure that the payload size
	*    is 16 byte(128bit) alignment.
	*/
	clist_for_each_entry_safe(node, _node_, list_header, typeof(*node), cnode) {
		/**
		 * @brief this fence fd maybe not effect fd, but can make said fence
		 * also has not been released, so need wait here.
		 */
		if (node) {
			client = (struct cmdlist_client *)node->client;
			if (node->fence)
				dkmd_acquire_fence_wait(node->fence, BUF_SYNC_TIMEOUT_MSEC);
		}

		if (prev_node && prev_node->list_cmd_header && node) {
			if (client && ((client->node_type & DM_TRANSPORT_TYPE) != 0)) {
				prev_node->list_cmd_header->cmd_flag.bits.task_end = 1;
				prev_node->list_cmd_header->cmd_flag.bits.last = 1;
			} else {
				prev_node->list_cmd_header->cmd_flag.bits.task_end = 0;
				prev_node->list_cmd_header->cmd_flag.bits.last = 0;
				prev_node->list_cmd_header->next_list = node->cmd_header_addr;
			}
		}

		if (client && ((client->node_type & OPR_REUSE_TYPE) == 0))
			prev_node = node;
	}

	/* dump all linked node, include dm, cfg, reused node */
	if (g_cmdlist_dump_enable) {
		clist_for_each_entry_safe(node, _node_, list_header, typeof(*node), cnode) {
			if (node && (node->scene_id != 7))
				cmdlist_dump_node(node);
		}
	}
}

/**
 * @brief According to scene id and cmdlist id found it a frame of configuration
 *       information, and then set up the hardware connection, get the frame header
 *       configuration address at the same time
 *
 * @param scene_id hardware scene id: 0~3 online composer scene, 4~6 offline composer scene
 * @param cmdlist_id cmdlist_id should be the scene header
 * @return dma_addr_t the frame header configuration address (DM data node header addr)
 */
dma_addr_t dkmd_cmdlist_get_dma_addr(uint32_t scene_id, uint32_t cmdlist_id)
{
	dma_addr_t cmdlist_flush_addr = 0;
	struct clist_head *list_header = NULL;
	struct cmdlist_node *header_node = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	down(&cmd_priv->table_sem);
	list_header = get_valid_cmdlist_header(scene_id, cmdlist_id);
	if (unlikely(!list_header))
		goto out_return;

	dkmd_cmdlist_exec_hardware_link(list_header);

	header_node = clist_first_entry(list_header, typeof(*header_node), cnode);
	if (header_node)
		cmdlist_flush_addr = header_node->cmd_header_addr;

out_return:
	up(&cmd_priv->table_sem);
	return cmdlist_flush_addr;
}
EXPORT_SYMBOL(dkmd_cmdlist_get_dma_addr);

/**
 * @brief According to scene id and cmdlist id release all cmdlist node in the scene cmdlist,
 *        locked for the principle
 *
 * @param scene_id hardware scene id: 0~3 online composer scene, 4~6 offline composer scene
 * @param cmdlist_id cmdlist_id should be the scene header
 */
void dkmd_cmdlist_release_locked(uint32_t scene_id, uint32_t cmdlist_id)
{
	struct cmdlist_client *client = NULL;
	struct clist_head *list_header = NULL;
	struct cmdlist_node *node = NULL, *_node_ = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.scene_id = scene_id,
		.id = cmdlist_id,
	};

	down(&cmd_priv->table_sem);
	list_header = get_valid_cmdlist_header(scene_id, cmdlist_id);
	if (unlikely(!list_header))
		goto out_return;

	clist_for_each_entry_safe(node, _node_, list_header, typeof(*node), cnode) {
		if (node && node->client) {
			client = (struct cmdlist_client *)node->client;
			dpu_pr_debug("--> release client<%u, %pK>\n", client->key, client->node);
			cmdlist_release_node(node);
			chash_remove(&client->hash_node);
			kfree(client);
		}
	}

out_return:
	/* release scene header */
	cmdlist_release_client(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);
}
EXPORT_SYMBOL(dkmd_cmdlist_release_locked);

uint32_t cmdlist_create_user_client(uint32_t scene_id, uint32_t node_type, uint32_t dst_dma_addr, uint32_t size)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.id = 0,
		.scene_id = scene_id,
		.node_size = size,
		.type = node_type,
		.dst_addr = dst_dma_addr,
	};

	if (!(node_type & SCENE_NOP_TYPE))
		user_client.node_size += 16; /* add cmdlist header size */

	down(&cmd_priv->table_sem);

	cmdlist_create_client(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);

	return user_client.id;
}
EXPORT_SYMBOL(cmdlist_create_user_client);

void *cmdlist_get_payload_addr(uint32_t scene_id, uint32_t cmdlist_id)
{
	void *payload_addr = NULL;
	struct cmdlist_client *client = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	down(&cmd_priv->table_sem);

	client = cmdlist_get_current_client(&cmd_priv->cmd_table, scene_id, cmdlist_id);
	if (unlikely(!client || !client->node)) {
		dpu_pr_err("invalid cmdlist client!");
		up(&cmd_priv->table_sem);
		return NULL;
	}
	payload_addr = (void *)client->node->list_cmd_item;

	up(&cmd_priv->table_sem);

	return payload_addr;
}
EXPORT_SYMBOL(cmdlist_get_payload_addr);

uint32_t cmdlist_get_phy_addr(uint32_t scene_id, uint32_t cmdlist_id)
{
	uint32_t header_phy_addr = 0;
	struct cmdlist_client *client = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	down(&cmd_priv->table_sem);

	client = cmdlist_get_current_client(&cmd_priv->cmd_table, scene_id, cmdlist_id);
	if (unlikely(!client || !client->node)) {
		dpu_pr_err("invalid cmdlist client!");
		up(&cmd_priv->table_sem);
		return header_phy_addr;
	}
	header_phy_addr = client->node->cmd_header_addr;

	up(&cmd_priv->table_sem);

	return header_phy_addr;
}
EXPORT_SYMBOL(cmdlist_get_phy_addr);

int32_t cmdlist_append_client(uint32_t scene_id, uint32_t scene_header_id, uint32_t cmdlist_id)
{
	int ret;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.scene_id = scene_id,
		.id = scene_header_id,
		.append_next_id = cmdlist_id,
	};

	down(&cmd_priv->table_sem);

	ret = cmdlist_append_next_client(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);

	return ret;
}
EXPORT_SYMBOL(cmdlist_append_client);

int32_t cmdlist_flush_client(uint32_t scene_id, uint32_t cmdlist_id)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.scene_id = scene_id,
		.id = cmdlist_id,
		.valid_payload_size = 0,
	};

	down(&cmd_priv->table_sem);

	cmdlist_signal_client(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);

	return 0;
}
EXPORT_SYMBOL(cmdlist_flush_client);

void dkmd_set_reg(uint32_t scene_id, uint32_t cmdlist_id, uint32_t addr, uint32_t value)
{
	struct cmdlist_client *client = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	down(&cmd_priv->table_sem);

	client = cmdlist_get_current_client(&cmd_priv->cmd_table, scene_id, cmdlist_id);
	if (unlikely(!client || !client->node)) {
		dpu_pr_err("invalid cmdlist client!");
		up(&cmd_priv->table_sem);
		return;
	}
	up(&cmd_priv->table_sem);

	cmdlist_set_reg(client->node, addr, value);
}
EXPORT_SYMBOL(dkmd_set_reg);

int32_t dkmd_cmdlist_dump_scene(uint32_t scene_id)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.scene_id = scene_id,
	};

	down(&cmd_priv->table_sem);

	cmdlist_dump_scene(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);

	return 0;
}
EXPORT_SYMBOL(dkmd_cmdlist_dump_scene);

int32_t dkmd_cmdlist_dump_by_id(uint32_t scene_id, uint32_t cmdlist_id)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;
	struct cmdlist_node_client user_client = {
		.scene_id = scene_id,
		.id = cmdlist_id,
	};

	down(&cmd_priv->table_sem);

	cmdlist_dump_client(&cmd_priv->cmd_table, &user_client);

	up(&cmd_priv->table_sem);

	return 0;
}
EXPORT_SYMBOL(dkmd_cmdlist_dump_by_id);
