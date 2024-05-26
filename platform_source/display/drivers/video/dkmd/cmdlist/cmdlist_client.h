/**
 * @file cmdlist_client.h
 * @brief Implementing interface function for cmdlist node
 *
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

#ifndef __CMDLIST_CLIENT_H__
#define __CMDLIST_CLIENT_H__

#include <dkmd_cmdlist.h>
#include "cmdlist_interface.h"

extern int32_t g_cmdlist_dump_enable;
struct chash_table;
struct cnode;

struct cmdlist_table {
	uint32_t scene_num;
	/* each scene has owner table, which would manager cmdlist node in it */
	struct chash_table ht;
};

struct cmdlist_client {
	struct cnode hash_node;
	uint32_t scene_id;
	uint32_t key;
	struct cmdlist_node *node;

	uint32_t node_type;
};

/**
 * @brief init cmdlist device and cmdlist table
 *
 * @param cmd_table cmdlist client table
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_client_table_setup(struct cmdlist_table *cmd_table);

/**
 * @brief release cmdlist table and list
 *
 * @param cmd_table cmdlist client table
 */
void cmdlist_client_table_release(struct cmdlist_table *cmd_table);

/**
 * @brief create cmdlist node by type, node_size and dst_addr
 *
 * @param cmd_table cmdlist client table
 * @param user_client use this client type, node_size and dst_addr
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_create_client(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief release cmdlist client by scene_id and cmdlist_id
 *
 * @param cmd_table cmdlist client table
 * @param user_client use this client scene_id and cmdlist_id
 */
int32_t cmdlist_release_client(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief append cmdlist node which recorded by user_client with append_next_id to this scene cmdlist node
 *
 * @param cmd_table cmdlist client table
 * @param user_client use this client id and append_next_id
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_append_next_client(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief signal this current scene cmdlist node
 *
 * @param cmd_table cmdlist client table
 * @param user_client use this client id
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_signal_client(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief Dump this current scene cmdlist node
 *
 * @param cmd_table cmdlist client table
 * @param user_client use this client id
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_dump_client(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief Dump all the same scene cmdlist nodes
 *
 * @param cmd_table cmdlist client table
 * @param user_client use the scene_id
 * @return int32_t 0: success -1: fail
 */
int32_t cmdlist_dump_scene(struct cmdlist_table *cmd_table, struct cmdlist_node_client *user_client);

/**
 * @brief get cmdlist client by scene id and cmdlist id
 *
 * @param cmd_table cmdlist client table
 * @param scene_id
 * @param cmdlist_id
 * @return struct cmdlist_client*
 */
struct cmdlist_client *cmdlist_get_current_client(struct cmdlist_table *cmd_table,
	uint32_t scene_id, uint32_t cmdlist_id);

#endif
