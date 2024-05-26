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
#include "hisi_dm.h"
#include "cmdlist.h"
#include "cmdlist_drv.h"
#include "cmdlist_dev.h"

extern struct cmdlist_private g_dpu_cmdlist_priv;

static struct cmdlist_dm_addr_info dm_tlb_info[] = {
	/* ------- scene_id -------------- DM_addr --------- DM_size ------ */
	{DPU_SCENE_ONLINE_0,  DM_INPUTDATA_ST_ADDR0,  0x1000, }, /* 4K */
	{DPU_SCENE_ONLINE_1,  DM_INPUTDATA_ST_ADDR1,  0x1000, }, /* 4K */
	{DPU_SCENE_ONLINE_2,  DM_INPUTDATA_ST_ADDR2,  0xA00,  }, /* 2.5K */
	{DPU_SCENE_ONLINE_3,  DM_INPUTDATA_ST_ADDR3,  0xA00,  }, /* 2.5K */
	{DPU_SCENE_OFFLINE_0, DM_INPUTDATA_ST_ADDR4,  0xA00,  }, /* 2.5K */
	{DPU_SCENE_OFFLINE_1, DM_INPUTDATA_ST_ADDR5,  0xA00,  }, /* 2.5K */
	{DPU_SCENE_OFFLINE_2, DM_INPUTDATA_ST_ADDR6,  0xA00,  }, /* 2.5K */
	{DPU_SCENE_INITAIL,  DM_DPP_INITAIL_ST_ADDR, 0x800,  }, /* 2K */
	{DPU_SCENE_SECURITY, DM_SECURITY_ST_ADDR,    0x400,  }, /* 1K */
};

static int cmdlist_alloc_items(struct cmdlist_client *client)
{
	struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

	if (IS_ERR_OR_NULL(priv->memory_pool)) {
		pr_err("check memory_pool failed!\n");
		return -1;
	}

	client->node_size = client->cmd_mode ? \
		roundup(sizeof(struct cmd_header) + sizeof(struct hisi_dm_param), PAGE_SIZE) : \
		roundup(sizeof(struct cmd_header) + sizeof(struct cmd_item) * ITEMS_MAX_NUM, PAGE_SIZE);

	client->list_cmd_header = (struct cmd_header *)(uintptr_t)gen_pool_dma_alloc(priv->memory_pool,
		client->node_size, &client->cmd_header_addr);
	if (!client->list_cmd_header) {
		pr_err("cmd header alloc failed!\n");
		return -1;
	}
	memset(client->list_cmd_header, 0, client->node_size);

	client->list_cmd_header->cmd_flag.bits.cmd_mode = client->cmd_mode;
	client->list_cmd_header->cmd_flag.bits.id = 0;
	client->list_cmd_header->cmd_flag.bits.pending = 0;
	client->list_cmd_header->cmd_flag.bits.last = 1;
	client->list_cmd_header->cmd_flag.bits.nop = 0;
	client->list_cmd_header->cmd_flag.bits.task_end = 1;
	client->list_cmd_header->cmd_flag.bits.valid_flag = 0xA5;
	client->list_cmd_header->cmd_flag.bits.exec = 0x0;
	client->list_cmd_header->cmd_flag.bits.event_list = 0;

	client->list_cmd_item = (struct cmd_item *)(client->list_cmd_header + 1);
	client->cmd_item_addr = client->cmd_header_addr + sizeof(struct cmd_header);

	pr_debug("alloc node header_addr[0x%x].list_cmd_header[0x%x].node_size[0x%x]",
		client->cmd_header_addr, client->list_cmd_header, client->node_size);

	client->list_cmd_header->list_addr = client->cmd_item_addr;

	/* new CONFIG_MODE client has no payload data */
	client->payload_size = (client->cmd_mode == TRANSPORT_MODE) ? ITEMS_MAX_NUM : 0;

	if (client->cmd_mode == TRANSPORT_MODE) {
		client->list_cmd_header->total_items.bits.dma_dst_addr = (DACC_OFFSET + dm_tlb_info[client->scene_id].dm_data_addr) >> 2;
		client->list_cmd_header->total_items.bits.items_count = dm_tlb_info[client->scene_id].dm_data_size >> 4;
	}

	return 0;
}

/**
 * dpu_cmdlist_create_client - create special cmdlist client
 * @header_list: scene header list
 * @id: dpu work scene mode id(0..3: online mode, 4..6: offline mode etc.)
 * @cmd_mode: cmdlist work mode(0: config mode, 1: transport mode)
 *
 * Create special cmdlist client which will be linked to scene_list.
 */
struct cmdlist_client *__dpu_cmdlist_create_client(struct list_head *header_list,
	enum SCENE_ID id, enum CMDLIST_MODE cmd_mode)
{
	struct cmdlist_client *client = NULL;
	// struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

	client = (struct cmdlist_client *)kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		pr_err("cmdlist create client failed!\n");
		return NULL;
	}
	down(&g_dpu_cmdlist_priv.sem);
	client->cmd_mode = cmd_mode;
	client->scene_id = id;
	client->enable = (uint32_t)dpu_cmdlist_enable;

	if (cmdlist_alloc_items(client) != 0) {
		pr_err("cmdlist alloc items failed!\n");
		kfree(client);
		up(&g_dpu_cmdlist_priv.sem);
		return NULL;
	}
	/* user need to initail this paramter @base */
	// client->base = priv->dev_reg_base;
	client->scene_list_header = header_list;
	client->list_node.next = NULL;
	list_add_tail(&client->list_node, header_list);
	up(&g_dpu_cmdlist_priv.sem);

	return client;
}

struct cmdlist_client *dpu_cmdlist_create_client(enum SCENE_ID id, enum CMDLIST_MODE cmd_mode)
{
	struct list_head *scene_header_list = NULL;
	struct cmdlist_client *client = NULL;

	scene_header_list = (struct list_head *)kzalloc(sizeof(*scene_header_list), GFP_KERNEL);
	if (!scene_header_list) {
		pr_err("alloc header_list failed!\n");
		return NULL;
	}
	INIT_LIST_HEAD(scene_header_list);

	client =  __dpu_cmdlist_create_client(scene_header_list, id, cmd_mode);
	if (!client) {
		pr_err("cmdlist create client failed!\n");
		kfree(scene_header_list);
		return NULL;
	}

	pr_debug("[%s] --> return client[0x%x].header_addr[0x%x].items_addr[0x%x].scene[%d].mode[%d] !\n",
		__func__, client, client->cmd_header_addr, client->cmd_item_addr, client->scene_id, client->cmd_mode);

	return client;
}
EXPORT_SYMBOL(dpu_cmdlist_create_client);

/**
 * cmdlist_release_client - release and free all client nodes on scene_list
 * @client: any client node on the scene_list
 *
 * Release and free all cmdlist nodes on scene_list containing special client.
 */
int cmdlist_release_client(struct cmdlist_client *client)
{
	struct cmdlist_client *node = NULL;
	struct cmdlist_client *_node_ = NULL;
	struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

	if (!client) {
		pr_err("client is null!\n");
		return -1;
	}

	if (!priv || IS_ERR_OR_NULL(priv->memory_pool)) {
		pr_err("cmdlist private is null!\n");
		return -1;
	}

	down(&g_dpu_cmdlist_priv.sem);
	uint32_t node_cnt = 0;
	list_for_each_entry_safe(node, _node_, client->scene_list_header, list_node) {
		if (node_cnt >= 2)
			break;

		if (node != NULL) {
			list_del(&node->list_node);
			if (node->node_size != 0) {
				pr_debug("node[0x%x] --> free header_addr[0x%x].list_cmd_header[0x%x].node_size[0x%x]",
					node, node->cmd_header_addr, node->list_cmd_header, node->node_size);
				gen_pool_free(priv->memory_pool, (unsigned long)node->list_cmd_header, node->node_size);
				node->list_cmd_header = NULL;
				node->node_size = 0;
			}
			kfree(node);
			node = NULL;
		}
		node_cnt++;
	}
	up(&g_dpu_cmdlist_priv.sem);

	return 0;
}
EXPORT_SYMBOL(cmdlist_release_client);

static struct cmdlist_client *cmdlist_get_valid_client(struct cmdlist_client *client)
{
	struct cmdlist_client *last_client = NULL;
	struct cmdlist_client *new_client = NULL;

	if (!client) {
		pr_err("client is null! \n");
		return NULL;
	}

	last_client = list_last_entry(client->scene_list_header, typeof(*client), list_node);
	if ((last_client->list_cmd_header == NULL) || (last_client->list_cmd_item == NULL)) {
		pr_err("last_client payload is null! \n");
		return NULL;
	}

	if (last_client->payload_size >= ITEMS_MAX_NUM - 1) {
		new_client = __dpu_cmdlist_create_client(client->scene_list_header, client->scene_id, CONFIG_MODE);
		if (new_client) {
			last_client->list_cmd_header->next_list = new_client->cmd_header_addr;
		}
		return new_client;
	}

	return last_client;
}

/**
 * dpu_cmdlist_set_reg - set cmdlist items append on special client
 * @client: special cmdlist client pointer
 * @addr: base on dpu memory map, such as cmdlist_offset is 0x12000
 * @value: actual value to be configured
 *
 * This function will get the last position client list pointer,
 * if the newest client's payload_size oversize node_size (transport mode cmdlist client),
 * need create new cmdlist node list and append cmd_items on the newest cmdlist.
 */
int dpu_cmdlist_set_reg(struct cmdlist_client *client, uint32_t addr, uint32_t value)
{
	uint32_t prev_item_base_addr = 0, new_addr = 0;
	struct cmdlist_client *new_client = NULL;
	struct cmd_item *current_cmd_item = NULL, *new_cmd_item = NULL;

	new_client = cmdlist_get_valid_client(client);
	if (!new_client) {
		pr_err("new_client is null!\n");
		return -1;
	}

	new_addr = addr >> 2;
	new_client->list_cmd_header->total_items.bits.items_count = new_client->payload_size + 1;
	current_cmd_item = (struct cmd_item *)(new_client->list_cmd_item + new_client->payload_size);

	prev_item_base_addr = current_cmd_item->cmd_item.cmd_mode_0.addr0;
	if (prev_item_base_addr == 0) {
		current_cmd_item->cmd_item.cmd_mode_0.addr0 = new_addr;
		current_cmd_item->cmd_item.cmd_mode_0.data0 = value;
		current_cmd_item->cmd_item.cmd_mode_0.cnt = 0;
		return 0;
	}

	/* 0x3F: bit[5:0] */
	if ((new_addr > prev_item_base_addr) && (new_addr - prev_item_base_addr < 0x3F)) {
		if (current_cmd_item->cmd_item.cmd_mode_0.addr1 == 0) {
			current_cmd_item->cmd_item.cmd_mode_0.addr1 = new_addr - prev_item_base_addr;
			current_cmd_item->cmd_item.cmd_mode_0.data1 = value;
			current_cmd_item->cmd_item.cmd_mode_0.cnt = 1;
			return 0;
		} else if (current_cmd_item->cmd_item.cmd_mode_0.addr2 == 0) {
			current_cmd_item->cmd_item.cmd_mode_0.addr2 = new_addr - prev_item_base_addr;
			current_cmd_item->cmd_item.cmd_mode_0.data2 = value;
			current_cmd_item->cmd_item.cmd_mode_0.cnt = 2;
			return 0;
		}
	}

	/* need get new cmd item */
	new_client->payload_size++;
	new_cmd_item = (struct cmd_item *)(new_client->list_cmd_item + new_client->payload_size);
	new_cmd_item->cmd_item.cmd_mode_0.addr0 = new_addr;
	new_cmd_item->cmd_item.cmd_mode_0.data0 = value;
	new_cmd_item->cmd_item.cmd_mode_0.cnt = 0;

	/* record items num */
	new_client->list_cmd_header->total_items.bits.items_count = new_client->payload_size + 1;

	return 0;
}
EXPORT_SYMBOL(dpu_cmdlist_set_reg);

static void cmdlist_save_file(char *filename, const char *buf, uint32_t buf_len)
{
	ssize_t write_len = 0;
	struct file *fd = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;

	if (!filename) return;

	if (!buf) return;

	fd = filp_open(filename, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(fd)) {
		pr_err("filp_open returned:filename %s, error %ld\n", filename, PTR_ERR(fd));
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);  //lint !e501

	write_len = vfs_write(fd, (char __user *)buf, buf_len, &pos);

	pos = 0;
	set_fs(old_fs);
	filp_close(fd, NULL);
}

/**
 * dpu_cmdlist_dump_client - dump and save cmdlist nodes on scene_list
 * @client: special cmdlist client pointer
 *
 * Dump all cmdlist nodes on scene_list containing special client.
 */
void dpu_cmdlist_dump_client(struct cmdlist_client *client)
{
	uint32_t i = 0;
	struct cmdlist_client *node = NULL;
	struct cmdlist_client *_node_ = NULL;
	struct cmd_item *temp_item = NULL;
	uint32_t *temp_dm = NULL;
	char filename[256] = {0};
	uint32_t items_count = 0;

	if (!client) return;

	list_for_each_entry_safe(node, _node_, client->scene_list_header, list_node) {
		if (node == NULL) {
			pr_debug("the node is null \n");
			return;
		}
		pr_debug("client: 0x%x, node: 0x%x!\n", client, node);

		pr_debug("   cmd_flag    |   next_list   |   total_items   |   list_addr  \n");
		pr_debug(" --------------+---------------+-----------------+--------------\n");
		pr_debug("   0x%8x  |   0x%8x  |   0x%8x    |   0x%8x \n",
			node->list_cmd_header->cmd_flag.ul32, node->list_cmd_header->next_list,
			node->list_cmd_header->total_items.ul32, node->list_cmd_header->list_addr);

		items_count = node->list_cmd_header->total_items.bits.items_count;

		if (node->cmd_mode == 1) {
			temp_dm = (uint32_t *)node->list_cmd_item;
			items_count <<= 2;
			pr_debug("hisi_dm_param size: 0x%x items_count: %d\n",
				sizeof(struct hisi_dm_param), items_count);
			for ( i = 0; i <= items_count; i += 4) {
				pr_debug("addr_offset:0x%x value:0x%08x 0x%08x 0x%08x 0x%08x \n",
				i * 4, temp_dm[i], temp_dm[i + 1], temp_dm[i + 2], temp_dm[i + 3]);
			}
		} else {
			pr_debug("items_count: %d \n", items_count);
			for (i = 0; i < items_count; i++) {
				temp_item = &(node->list_cmd_item[i]);
				pr_debug("set addr:0x%08x value:0x%08x addr1:0x%02x value:0x%08x addr2:0x%02x value:0x%08x\n",
					(temp_item->cmd_item.cmd_mode_0.addr0 << 2), temp_item->cmd_item.cmd_mode_0.data0,
					temp_item->cmd_item.cmd_mode_0.addr1, temp_item->cmd_item.cmd_mode_0.data1,
					temp_item->cmd_item.cmd_mode_0.addr2, temp_item->cmd_item.cmd_mode_0.data2);
			}
		}
	}
}
EXPORT_SYMBOL(dpu_cmdlist_dump_client);

void cmdlist_flush_client(struct cmdlist_client *client)
{
	cmdlist_config_start(client);

	struct hisi_dm_param *dm_param = (struct hisi_dm_param *)client->list_cmd_item;
	dm_param->cmdlist_addr.cmdlist_dpp_addr.value = client->list_cmd_header->next_list;
	dm_param->cmdlist_addr.cmdlist_h_addr.value = 0xFFFFFFFF;

	writel(0xFFFFFFFF, DPU_DM_CMDLIST_ADDR_H_ADDR(client->base + DACC_OFFSET +
		dm_tlb_info[client->scene_id].dm_data_addr));

	writel(client->cmd_header_addr, DPU_DM_CMDLIST_ADDR0_ADDR(client->base + DACC_OFFSET +
		dm_tlb_info[client->scene_id].dm_data_addr));

	writel(client->list_cmd_header->next_list, DPU_DM_CMDLIST_ADDR11_ADDR(client->base + DACC_OFFSET +
		dm_tlb_info[client->scene_id].dm_data_addr));

	client->list_cmd_header->next_list = 0;

	pr_debug("[%s] --> flush client[0x%x].header_addr[0x%x].items_addr[0x%x].scene[%d].mode[%d] !\n",
		__func__, client, client->cmd_header_addr, client->cmd_item_addr, client->scene_id, client->cmd_mode);

	dpu_cmdlist_dump_client(client);
}
EXPORT_SYMBOL(cmdlist_flush_client);