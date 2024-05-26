/**
 * @file cmdlist_interface.h
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

#ifndef CMDLIST_INTERFACE_H
#define CMDLIST_INTERFACE_H

#include <linux/types.h>

#define INVALID_CMDLIST_ID 0

/**
 * @brief According to the scene id and cmdlist id for
 *        DM configuration dma register address
 *
 * @param scene_id hardware scene id: 0~3 online composer scene, 4~6 offline composer scene
 * @param cmdlist_id cmdlist_id should be the scene header
 * @return dma_addr_t DM dma register addres which be loaded by dacc hardware
 */
extern dma_addr_t dkmd_cmdlist_get_dma_addr(uint32_t scene_id, uint32_t cmdlist_id);

/**
 * @brief According to scene id and cmdlist id release all cmdlist node in the scene cmdlist,
 *        locked for the principle
 *
 * @param scene_id hardware scene id: 0~3 online composer scene, 4~6 offline composer scene
 * @param cmdlist_id cmdlist_id should be the scene header
 */
extern void dkmd_cmdlist_release_locked(uint32_t scene_id, uint32_t cmdlist_id);

/**
 * @brief Create cmdlist client for user to generate register configration
 *        locked for the principle
 *
 * @param sceneId which scene hardware work in
 * @param nodeType node type of user need
 * @param dstDmaAddr where would be writed for DM Header Node
 * @param size payload size user need to consider
 */
uint32_t cmdlist_create_user_client(uint32_t scene_id, uint32_t node_type, uint32_t dst_dma_addr, uint32_t size);

void *cmdlist_get_payload_addr(uint32_t scene_id, uint32_t cmdlist_id);

uint32_t cmdlist_get_phy_addr(uint32_t scene_id, uint32_t cmdlist_id);

int32_t cmdlist_append_client(uint32_t scene_id, uint32_t scene_header_id, uint32_t cmdlist_id);

int32_t cmdlist_flush_client(uint32_t scene_id, uint32_t cmdlist_id);

void dkmd_set_reg(uint32_t scene_id, uint32_t cmdlist_id, uint32_t addr, uint32_t value);

int32_t dkmd_cmdlist_dump_scene(uint32_t scene_id);

int32_t dkmd_cmdlist_dump_by_id(uint32_t scene_id, uint32_t cmdlist_id);

#endif