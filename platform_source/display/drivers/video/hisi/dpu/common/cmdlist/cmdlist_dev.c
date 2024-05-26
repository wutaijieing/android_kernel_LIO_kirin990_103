/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "cmdlist_dev.h"
#include "cmdlist_drv.h"

const uint32_t cmd_dma_addr_offset = 0x012000;

static void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	uint32_t temp = 0;

	temp = readl(addr);
	temp &= ~(mask << bs);

	writel(temp | ((val & mask) << bs), addr);
}

void cmdlist_config_on(struct cmdlist_client *client)
{
	set_reg(DPU_CMD_CLK_SEL_ADDR(client->base + cmd_dma_addr_offset), 0xFFFFFFFF, 32, 0);
	set_reg(DPU_CMD_CLK_EN_ADDR(client->base + cmd_dma_addr_offset), 0xFFFFFFFF, 32, 0);

	/* cmdlist int */
	set_reg(DPU_CMD_CMDLIST_CH_INTE_ADDR(client->base + cmd_dma_addr_offset, client->scene_id), 0xFFFFFFFF, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_INTC_ADDR(client->base + cmd_dma_addr_offset, client->scene_id), 0xFFFFFFFF, 32, 0);

	/* ch_enable */
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 0), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 1), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 2), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 3), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 4), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 5), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 6), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 7), 1, 1, 0);
	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 8), 1, 1, 0);

	set_reg(DPU_CMD_CMDLIST_CH_CTRL_ADDR(client->base + cmd_dma_addr_offset, 8), 1, 1, 0);
}

void cmdlist_config_start(struct cmdlist_client *client)
{
	struct cmdlist_client *cmdlist_node = NULL;

	if (!client) return;

	dsb(sy);
	cmdlist_config_on(client);
	set_reg(DPU_CMD_TWO_SAME_FRAME_BYPASS_ADDR(client->base + cmd_dma_addr_offset), 0x1, 1, 0);

	cmdlist_node = list_last_entry(client->scene_list_header, typeof(*client), list_node);
	cmdlist_node->list_cmd_header->cmd_flag.bits.task_end = 1;
	cmdlist_node->list_cmd_header->cmd_flag.bits.last = 1;

	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 0), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 1), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 2), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 3), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 4), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 5), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 6), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 7), 0, 32, 0);
	set_reg(DPU_CMD_CMDLIST_CH_DBG_ADDR(client->base + cmd_dma_addr_offset, 8), 0, 32, 0);

	cmdlist_node = list_first_entry(client->scene_list_header, typeof(*client), list_node);
}

