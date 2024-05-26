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

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "cmdlist.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_composer.h"
#include "hisi_disp_dacc.h"

/* init the cmdlist set reg ops */
struct dpu_module_ops g_cmdlist_module_context_ops = {
	.create_context = NULL,
	.release_context = NULL,
	.flush_context = NULL,
};

struct dpu_module_ops *hisi_module_get_context_ops(void)
{
	return &g_cmdlist_module_context_ops;
}

int hisi_module_init(struct dpu_module_desc *module_desc,
	struct dpu_module_ops ops, void *cookie)
{
	disp_check_and_ret_err(module_desc == NULL, -1, "module_desc is NULL");
	module_desc->cookie = cookie;
	module_desc->context = NULL;
	module_desc->module_ops = ops;
	module_desc->client = NULL;

	return 0;
}

void hisi_module_deinit(struct dpu_module_desc *module_desc)
{
	disp_check_and_no_retval_err(module_desc == NULL, "module_desc is NULL");
	module_desc->cookie = NULL;
	module_desc->context = NULL;
}

void hisi_module_set_reg(struct dpu_module_desc *module_desc, char __iomem *addr, uint32_t value)
{
	uint32_t new_addr = 0;
	struct cmdlist_client *client = NULL;
	uint32_t dm_regs_start = DSS_BASE + 0x31200; // scene6 start addr
	uint32_t dm_regs_end = DSS_BASE + 0x363FF; // scene0 end addr

	disp_check_and_no_retval_err(module_desc == NULL, "module_desc is NULL");

	client = module_desc->client;
	if (client && client->enable) {
		new_addr = (uint32_t)(addr - client->base + DSS_BASE);
		if ((new_addr >= dm_regs_start) && (new_addr <= dm_regs_end)) // not include dm regs
			return;

		disp_pr_debug("client[0x%x]: cmdlist set new_addr[0x%x]=0x%x\n", client, new_addr, value);
		dpu_cmdlist_set_reg(client, new_addr, value);
	} else {
		disp_pr_debug("reg set addr[0x%p]=0x%x\n", addr, value);
		outp32(addr, value);
	}
}

/* TODO: link each module private cmdlist client data, and call this function only once every frame. */
void hisi_module_flush(struct hisi_composer_device *device)
{
	uint32_t j;
	uint32_t addr0;
	struct cmdlist_client *client = NULL;
	struct cmdlist_client *node = NULL;
	struct cmdlist_client *_node_ = NULL;
	struct cmd_item *temp_item = NULL;

	disp_pr_debug(" ++++ ");
	disp_check_and_no_retval_err(device == NULL, "device is NULL");
	client = device->client;
	disp_check_and_no_retval_err(client == NULL, "client is NULL");

	if (client->enable == 0) {
		disp_pr_debug("flush --> client[0x%x]\n", client);
		dpu_cmdlist_dump_client(client);

		list_for_each_entry_safe(node, _node_, client->list_node.prev, list_node) {
			if ((node == NULL) | (node->cmd_mode == 1)) {
				disp_pr_info("continue to next node \n");
				continue;
			}
			for (j = 0; j < node->list_cmd_header->total_items.bits.items_count; j++) {
				temp_item = &(node->list_cmd_item[j]);
				addr0 = (temp_item->cmd_item.cmd_mode_0.addr0) << 2;
				if (temp_item->cmd_item.cmd_mode_0.addr0 != 0)  {
					outp32(client->base + addr0, temp_item->cmd_item.cmd_mode_0.data0);
				}

				if (temp_item->cmd_item.cmd_mode_0.addr1 != 0)  {
					outp32(client->base + addr0 + temp_item->cmd_item.cmd_mode_0.addr1, temp_item->cmd_item.cmd_mode_0.data1);
				}

				if (temp_item->cmd_item.cmd_mode_0.addr2 != 0)  {
					outp32(client->base + addr0 + temp_item->cmd_item.cmd_mode_0.addr2, temp_item->cmd_item.cmd_mode_0.data2);
				}
			}
		}
	} else {
		/* cmdlist start config */
		cmdlist_flush_client(client);
	}

	hisi_disp_dacc_scene_config(device->ov_data.scene_id, client->enable, client->base, &(device->frame));

	disp_pr_debug(" ---- ");
}
