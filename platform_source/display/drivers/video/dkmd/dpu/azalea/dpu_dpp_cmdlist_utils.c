 /*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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
#include "dpu_fb.h"
#include <soc_dss_interface.h>

// Definiton
#define DPP_CMDLIST_HEADER_LEN (SZ_1K)
#define DPP_CMDLIST_ROI_THRESHHOLD (3)   // 1/3
#define DPP_CMDLIST_ADDR_OFFSET 0x3FFFF
#define MAX_DATA_PER_ITEM (3)
#define get_item_nums(x) ((x) / MAX_DATA_PER_ITEM)
#define DPP_CMDLIST_LAST_CH_CLEAR_NONE 0xF

/* platform dependent */
#define VACTIVE_CNT_BIT (14)
#define DPP_CMDLIST_OFFSET DPU_CMDLIST_MAX
#define DPP_CMDLIST_MCTL_OFFSET DSS_MCTRL_CTL5_OFFSET

#define DPP_GMP_LUT_LENGTH LCP_GMP_LUT_LENGTH
#define DPP_DEGAMMA_LUT_LENGTH LCP_IGM_LUT_LENGTH
#define DPP_XCC_TABLE_LENGTH LCP_XCC_LUT_LENGTH
#define DPP_GAMMA_LUT_LENGTH GAMA_LUT_LENGTH

// Declaration
uint8_t g_dpp_cmdlist_state;
uint8_t g_dpp_cmdlist_bypass_count;

extern uint32_t g_dyn_sw_default;

static spinlock_t g_dpe_irq_lock;

// Internal API
static void dpp_dump_cmdlist_node_items(const struct cmd_item *item, uint32_t count)
{
	uint32_t index = 0;
	uint32_t addr = 0;
	uint32_t inter = (count > 10) ? (count / 10) : 1;

	if (g_debug_dpp_cmdlist_dump == 2)
		inter = 1;
	for (index = 0; index < count; index += inter) {
		addr = item[index].reg_addr.bits.add0;
		addr = addr & CMDLIST_ADDR_OFFSET;
		addr = addr << 2;
		if ((addr & 0xFF000) >= 0xDE000 && (addr & 0xFF000) <= 0xF7000 &&
			(g_debug_dpp_cmdlist_dump == 2))
			continue;
		DPU_FB_INFO("set addr:0x%x value:0x%x add1:0x%x value:0x%x "
			"add2:0x%x value:0x%x item_index:%u\n",
			addr, item[index].data0,
			item[index].reg_addr.bits.add1 << 2, item[index].data1,
			item[index].reg_addr.bits.add2 << 2 , item[index].data2,
			index);
	}

	if ((count > 0) && (index - inter != count - 1)) {
		index = count - 1;
		addr = item[index].reg_addr.bits.add0;
		addr = addr & CMDLIST_ADDR_OFFSET;
		addr = addr << 2;
		DPU_FB_INFO("set addr:0x%x value:0x%x add1:0x%x value:0x%x "
			"add2:0x%x value:0x%x item_index:%u\n",
			addr, item[index].data0,
			item[index].reg_addr.bits.add1 << 2, item[index].data1,
			item[index].reg_addr.bits.add2 << 2 , item[index].data2,
			index);
	}
}

static void dpp_cmdlist_node_dump (const struct dpu_fb_data_type *dpufd,
	uint32_t item_num, uint8_t region, uint8_t buffer)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t count;

	if (g_debug_dpp_cmdlist_dump == 0)
		return;

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data is NULL");
		return;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	if (node->node_type == CMDLIST_NODE_FRAME)
		DPU_FB_INFO("node type = Frame node\n");
	else
		DPU_FB_ERR("ERROR node type:%d\n", node->node_type);

	DPU_FB_INFO("\t qos  | flag | pending | tast_end | last  | event_list | list_addr  | \
		next_list  | count | id | is_used | exec| intr | nop\n");
	DPU_FB_INFO("\t------+---------+------------+------------+------------+------------\n");
	DPU_FB_INFO("\t 0x%2x | 0x%2x |0x%6x | 0x%6x | 0x%3x | 0x%8x | 0x%8x | 0x%8x | \
		0x%3x | 0x%1x| 0x%4x  | 0x%1x | 0x%1x | 0x%x\n",
		node->list_header->flag.bits.qos,
		node->list_header->flag.bits.valid_flag,
		node->list_header->flag.bits.pending,
		node->list_header->flag.bits.task_end,
		node->list_header->flag.bits.last,
		node->list_header->flag.bits.event_list,
		node->list_header->list_addr, node->list_header->next_list,
		node->list_header->total_items.bits.count,
		node->list_header->flag.bits.id,
		node->is_used, node->list_header->flag.bits.exec,
		node->list_header->flag.bits.interrupt,
		node->list_header->flag.bits.nop);

	if (item_num != 0)
		count = item_num;
	else
		count = node->list_header->total_items.bits.count;
	dpp_dump_cmdlist_node_items(node->list_item, count);
}

static size_t dpp_cmdlist_get_pool_size(const struct dpu_fb_data_type *dpufd)
{
	return dpufd->sum_cmdlist_pool_size / (DPU_CMDLIST_MAX + 1);
}

static void dpp_cmdlist_data_free(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer)
{
	dpp_cmdlist_data_t *cmdlist_data = NULL;

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_DEBUG("dpp_cmdlist_data[%d][%d] is already NULL!\n",
			region, buffer);
		return;
	}
	cmdlist_data = dpufd->dpp_cmdlist_data[region][buffer];

	dpu_cmdlist_node_free(dpufd, cmdlist_data->cmdlist_node);
	cmdlist_data->cmdlist_node = NULL;

	kfree(cmdlist_data);
	cmdlist_data = NULL;
	dpufd->dpp_cmdlist_data[region][buffer] = NULL;
	DPU_FB_DEBUG("dpp_cmdlist_data[%d][%d] has been free!\n", region, buffer);
}

static struct dss_cmdlist_node* dpp_cmdlist_node_alloc(struct dpu_fb_data_type *dpufd)
{
	size_t header_len, item_len;
	size_t header_buffer_size, dpp_lut_size;

	DPU_FB_INFO("+");

	header_len = roundup(DPP_CMDLIST_HEADER_LEN, PAGE_SIZE);
	header_buffer_size = header_len * DPP_CMDLIST_TOTAL_BUFFER;
	item_len = (dpp_cmdlist_get_pool_size(dpufd) -
		header_buffer_size) / DPP_CMDLIST_TOTAL_BUFFER;

	// verify item_len is enough for store all dpp setting
	dpp_lut_size = dpu_display_effect_get_dpp_lut_size(dpufd);
	if (item_len < dpp_lut_size) {
		DPU_FB_ERR("cmdlist pool size:%lu is not enough to store %d dpp lut %lu\n",
			dpp_cmdlist_get_pool_size(dpufd),
			DPP_CMDLIST_TOTAL_BUFFER, dpp_lut_size);
		return NULL;
	}

	return dpu_cmdlist_node_alloc(dpufd, header_len, item_len);
}

static dpp_cmdlist_data_t* dpp_cmdlist_data_alloc(struct dpu_fb_data_type *dpufd)
{
	dpp_cmdlist_data_t *cmdlist_data = NULL;

	cmdlist_data = (dpp_cmdlist_data_t *)kmalloc(sizeof(dpp_cmdlist_data_t),
		GFP_ATOMIC);
	if (cmdlist_data == NULL) {
		DPU_FB_ERR("failed to kmalloc cmdlist_data!\n");
		return NULL;
	}
	memset(cmdlist_data, 0, sizeof(dpp_cmdlist_data_t));

	cmdlist_data->cmdlist_node = dpp_cmdlist_node_alloc(dpufd);
	if (cmdlist_data->cmdlist_node == NULL) {
		DPU_FB_ERR("failed to dpu_dpp_cmdlist_node_alloc!\n");
		kfree(cmdlist_data);
		cmdlist_data = NULL;
	}

	return cmdlist_data;
}

static int dpp_cmdlist_set_node(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer)
{
	struct dss_cmdlist_node *node = NULL;

	DPU_FB_INFO("+");

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data[%d][%d] is NULL!\n",
			region, buffer);
		return -EINVAL;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL!\n");
		return -EINVAL;
	}

	/* fill the header and item info */
	node->list_header->flag.bits.id = region;
	node->list_header->flag.bits.pending = 0x1;
	node->list_header->flag.bits.event_list = 8;
	node->list_header->flag.bits.task_end = 1;
	node->list_header->flag.bits.last = 1;
	node->list_header->flag.bits.valid_flag = CMDLIST_NODE_VALID;
	node->list_header->flag.bits.exec = 0x1;
	node->list_header->flag.bits.interrupt = 1;
	node->list_header->flag.bits.nop = 0;
	node->list_header->flag.bits.qos = 0;
	node->list_header->list_addr = node->item_phys;
	node->list_header->next_list = node->header_phys;
	node->is_used = 1;
	node->node_type = CMDLIST_NODE_FRAME;
	node->item_flag = 0;
	node->reserved = 0;

	DPU_FB_INFO("dpp_cmdlist_data[%d][%d] has been set\n", region, buffer);
	return 0;
}

static void dpp_cmdlist_reset_data(dpp_cmdlist_data_t *cmdlist_data)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t item_count;

	if (cmdlist_data == NULL)
		return;

	cmdlist_data->buffer_status = EN_DPP_CMDLIST_STATUS_FREE;
	memset(cmdlist_data->dpp_module, 0, MAX_DPP_MODULE_NUM * sizeof(uint8_t));
	cmdlist_data->dpp_module_num = 0;

	node = cmdlist_data->cmdlist_node;
	if (node == NULL)
		return;

	if (node->list_header->total_items.bits.count > 0) {
		item_count = MIN(node->list_header->total_items.bits.count, node->item_len / sizeof(struct cmd_item));
		memset(node->list_item, 0, item_count * sizeof(struct cmd_item));
		node->list_header->total_items.bits.count = 0;
	}
	node->item_index = 0;
	node->item_flag = 0;
}

// called by isr routine
static void dpp_cmdlist_set_read_ptr(struct dpu_fb_data_type *dpufd, uint8_t region)
{
	uint8_t buffer_rd;

	buffer_rd = dpufd->dpp_cmdlist_buffer_rd[region];
	buffer_rd = buffer_rd % DPP_CMDLIST_BUFFER_NUM;
	if (dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status ==
		EN_DPP_CMDLIST_STATUS_WORKING) {
		if (g_debug_dpp_cmdlist_debug)
			DPU_FB_INFO("dpp_cmdlist_data set free: region=%d, buffer_rd=%d\n", region, buffer_rd);

		dpp_cmdlist_reset_data(dpufd->dpp_cmdlist_data[region][buffer_rd]);
		dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status =
			EN_DPP_CMDLIST_STATUS_FINISHED;

		buffer_rd++;
		if (buffer_rd >= DPP_CMDLIST_BUFFER_NUM)
			buffer_rd = 0;

		dpufd->dpp_cmdlist_buffer_rd[region] = buffer_rd;
	} else {
		DPU_FB_WARNING("wrong buffer state:0x%x, region=%d, buffer_rd=%d\n",
			dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status,
			region, buffer_rd);
		// force to reset staus to free
		dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status =
			EN_DPP_CMDLIST_STATUS_FREE;
	}
}

// called by isr routine
static void dpp_cmdlist_bypass_buffer(struct dpu_fb_data_type *dpufd, uint8_t region)
{
	uint8_t buffer_rd;

	buffer_rd = dpufd->dpp_cmdlist_buffer_rd[region];
	buffer_rd = buffer_rd % DPP_CMDLIST_BUFFER_NUM;
	if (dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status ==
		EN_DPP_CMDLIST_STATUS_READY) {
		if (g_debug_dpp_cmdlist_debug)
			DPU_FB_INFO("dpp_cmdlist_data bypass buffer: region=%d, buffer_rd=%d\n",
				region, buffer_rd);

		dpp_cmdlist_reset_data(dpufd->dpp_cmdlist_data[region][buffer_rd]);
		dpufd->dpp_cmdlist_data[region][buffer_rd]->buffer_status =
			EN_DPP_CMDLIST_STATUS_FINISHED;

		buffer_rd++;
		if (buffer_rd >= DPP_CMDLIST_BUFFER_NUM)
			buffer_rd = 0;

		dpufd->dpp_cmdlist_buffer_rd[region] = buffer_rd;
	}
}

static uint8_t dpp_cmdlist_get_write_ptr(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint32_t moudle)
{
	uint8_t buffer_wr;
	unsigned long flags = 0;
	uint8_t buffer_status;

	spin_lock_irqsave(&dpufd->dpp_cmdlist_lock, flags);

	buffer_wr = dpufd->dpp_cmdlist_buffer_wr[region];

	DPU_FB_DEBUG("region=%d, wr_buffer=%d, status=0x%x\n",
		region, dpufd->dpp_cmdlist_buffer_wr[region],
		dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status);

	if ((dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status &
		(moudle + EN_DPP_CMDLIST_STATUS_READY)) ==
		(moudle + EN_DPP_CMDLIST_STATUS_READY)) {
		// this module's buffer already received, try write next first
		DPU_FB_DEBUG("this module's buffer already received, module=0x%x\n",
			moudle);
		buffer_wr++;
		if (buffer_wr >= DPP_CMDLIST_BUFFER_NUM)
			buffer_wr = 0;

		buffer_status = dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status;
		if (buffer_status == EN_DPP_CMDLIST_STATUS_FREE ||
			buffer_status == EN_DPP_CMDLIST_STATUS_FINISHED) {
			dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status =
				EN_DPP_CMDLIST_STATUS_FREE;
			dpufd->dpp_cmdlist_buffer_wr[region] = buffer_wr;
			goto normal_out;
		}
		DPU_FB_DEBUG("next buff is not free, next_buffer=%d, status=0x%x\n",
			buffer_wr, dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status);
	}

	buffer_wr = dpufd->dpp_cmdlist_buffer_wr[region];
	buffer_status = dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status;
	if (buffer_status == EN_DPP_CMDLIST_STATUS_WORKING ||
		buffer_status == EN_DPP_CMDLIST_STATUS_FINISHED) {
		if (buffer_status == EN_DPP_CMDLIST_STATUS_FINISHED)
			dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status =
				EN_DPP_CMDLIST_STATUS_FREE;

		// try write next
		buffer_wr++;
		if (buffer_wr >= DPP_CMDLIST_BUFFER_NUM)
			buffer_wr = 0;

		buffer_status = dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status;
		if (buffer_status == EN_DPP_CMDLIST_STATUS_FREE ||
			buffer_status == EN_DPP_CMDLIST_STATUS_FINISHED) {
			dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status =
				EN_DPP_CMDLIST_STATUS_FREE;
			dpufd->dpp_cmdlist_buffer_wr[region] = buffer_wr;
			goto normal_out;
		} else {
			DPU_FB_WARNING("no buffer can be written! status=0x%x\n",
				dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status);
			goto error_out;
		}
	}

	buffer_status = dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status;
	if (buffer_status == EN_DPP_CMDLIST_STATUS_FREE ||
		(buffer_status & 0xF0) == EN_DPP_CMDLIST_STATUS_READY) {
		goto normal_out;
	} else {
		// if lock got, should not be EN_DPP_CMDLIST_STATUS_PREPARING
		DPU_FB_WARNING("check buffer state!\n");
		goto error_out;
	}

normal_out:
	dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status =
		EN_DPP_CMDLIST_STATUS_PREPARING +
		(dpufd->dpp_cmdlist_data[region][buffer_wr]->buffer_status & 0xF);
	spin_unlock_irqrestore(&dpufd->dpp_cmdlist_lock, flags);
	return buffer_wr;

error_out:
	spin_unlock_irqrestore(&dpufd->dpp_cmdlist_lock, flags);
	return DPP_CMDLIST_BUFFER_NUM;
}

static void dpp_cmdlist_write_complete(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer, uint32_t moudle)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dpufd->dpp_cmdlist_lock, flags);

	dpufd->dpp_cmdlist_data[region][buffer]->buffer_status =
		EN_DPP_CMDLIST_STATUS_READY +
		(dpufd->dpp_cmdlist_data[region][buffer]->buffer_status & 0xF);
	dpufd->dpp_cmdlist_data[region][buffer]->buffer_status |= moudle;

	spin_unlock_irqrestore(&dpufd->dpp_cmdlist_lock, flags);
}

static uint32_t dpp_cmdlist_get_module_index(
	dpp_cmdlist_data_t *dpp_cmdlist_data,
	uint32_t module)
{
	uint8_t i;
	uint32_t index = 0;
	bool is_exist = false;

	for (i = 0; i < dpp_cmdlist_data->dpp_module_num; i++) {
		if (dpp_cmdlist_data->dpp_module[i] == module) {
			is_exist = true;
			DPU_FB_DEBUG("module 0x%x exist!\n", module);
			break;
		}
	}

	if (!is_exist) {
		if (dpp_cmdlist_data->dpp_module_num == MAX_DPP_MODULE_NUM) {
			DPU_FB_ERR("error module!\n");
			index = 0;
		} else {
			dpp_cmdlist_data->dpp_module[dpp_cmdlist_data->dpp_module_num] = module;
			dpp_cmdlist_data->dpp_module_num++;
			index = dpp_cmdlist_data->cmdlist_node->item_index;
		}
	} else {
		index = dpp_cmdlist_data->cmdlist_node->item_index;
	}

	DPU_FB_DEBUG("index is %d\n", index);
	return index;
}

static void dpp_cmdlist_set_reg(struct dss_cmdlist_node *node, uint32_t addr,
	uint32_t value, uint8_t bw, uint8_t bs)
{
	uint64_t mask;
	uint32_t index = 0;
	uint32_t new_addr = 0;
	uint32_t old_addr = 0;
	int condition = 0;

	if (!dpu_check_cmdlist_paremeters_validate(bw, bs))
		return;

	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	if (node->node_type == CMDLIST_NODE_NOP) {
		DPU_FB_ERR("can't set register value to NOP node!");
		return;
	}

	index = node->item_index;
	if ((index + 1) >= (node->item_len / sizeof(struct cmd_item))) {
		DPU_FB_WARNING("index=%d is larger than %lu!\n",
			(index + 1),
			(node->item_len / sizeof(struct cmd_item)));
		return;
	}

	new_addr = (addr >> 2) & CMDLIST_ADDR_OFFSET;
	old_addr = node->list_item[index].reg_addr.ul32 & CMDLIST_ADDR_OFFSET;
	condition = (((new_addr - old_addr) < MAX_ITEM_OFFSET) && (new_addr >= old_addr));
	mask = ((uint64_t)1 << bw) - 1;

	if (bw != 32) { // not set full bits of an 4 Bytes register
		node->list_item[index].reg_addr.bits.add0 = new_addr;
		node->list_item[index].data0 = value;
		node->list_item[index].data1 = (uint32_t) ((~(mask << bs)) & 0xFFFFFFFF);
		node->list_item[index].data2 = (uint32_t) (((mask & value) << bs) & 0xFFFFFFFF);
		node->list_item[index].reg_addr.bits.cnt = 3;
	} else {
		node->list_item[index].reg_addr.bits.add0 = new_addr;
		node->list_item[index].data0 = value;
		node->list_item[index].reg_addr.bits.cnt = 0;
	}
	index++;

	node->item_index = index;
	node->list_header->total_items.bits.count = index;
}

static uint32_t dpp_cmdlist_config_gmp(struct dss_cmdlist_node *node,
	uint32_t old_addr, uint32_t index, const uint32_t *gmp_lut_low32,
	const uint32_t *gmp_lut_high4, uint32_t gmp_length)
{
	uint32_t new_addr;
	uint32_t value;
	uint32_t i, item_index, lut_index;
	uint32_t last;

	if ((index + (gmp_length * 2 + 2) / MAX_DATA_PER_ITEM + 1) >= get_max_item(node->item_len)) {
		DPU_FB_WARNING("index=%u is larger than %lu!\n",
			(index + (gmp_length * 2 + 2) / MAX_DATA_PER_ITEM + 1),
			get_max_item(node->item_len));
		return node->item_index;
	}

	last = gmp_length * 2 % MAX_DATA_PER_ITEM;
	gmp_length = round_down(gmp_length * 2, MAX_DATA_PER_ITEM); // one node save three words data

	for (i = 0; i < gmp_length; i += MAX_DATA_PER_ITEM) {
		item_index = i;
		lut_index = item_index / 2;
		if (item_index % 2 == 0)
			value = gmp_lut_low32[lut_index];
		else
			value = gmp_lut_high4[lut_index];

		new_addr = old_addr + item_index * 4;
		new_addr = (new_addr >> 2) & DPP_CMDLIST_ADDR_OFFSET;
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add0 = new_addr;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data0 = value;

		item_index++;
		lut_index = item_index / 2;
		if (item_index % 2 == 0)
			value = gmp_lut_low32[lut_index];
		else
			value = gmp_lut_high4[lut_index];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add1 = 1;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data1 = value;

		item_index++;
		lut_index = item_index / 2;
		if (item_index % 2 == 0)
			value = gmp_lut_low32[lut_index];
		else
			value = gmp_lut_high4[lut_index];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add2 = 2;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data2 = value;

		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.cnt = 2;
	}
	index += get_item_nums(i);

	// fill last if need
	if (last > 0) {
		item_index = i;
		lut_index = item_index / 2;
		if (item_index % 2 == 0)
			value = gmp_lut_low32[lut_index];
		else
			value = gmp_lut_high4[lut_index];
		new_addr = old_addr + item_index * 4;
		new_addr = (new_addr >> 2) & DPP_CMDLIST_ADDR_OFFSET;
		node->list_item[index].reg_addr.bits.add0 = new_addr;
		node->list_item[index].data0 = value;
		if (last > 1)
			node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add1 = 1;

		node->list_item[index].reg_addr.bits.cnt = last - 1;
		index++;
	}

	node->item_index = index;
	node->list_header->total_items.bits.count = index;

	return index;
}

static void dpp_cmdlist_set_gmp(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer, const struct gmp_info *gmp)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t index = 0;
	uint32_t addr = 0;
	char __iomem *gmp_base = NULL;
	char __iomem *gmp_lut_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data is NULL");
		return;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	DPU_FB_DEBUG("+ region=%d, buffer=%d", region, buffer);

	gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET + region * DPP_GMP_BASE;
	gmp_lut_base = dpufd->dss_base + DSS_DPP_GMP_LUT_OFFSET +
		region * DPP_GMP_LUT_BASE;

	index = dpp_cmdlist_get_module_index(dpufd->dpp_cmdlist_data[region][buffer],
		EN_DPP_CH_MODULE_GMP);

	// gmp lut
	addr = (uint32_t)(gmp_lut_base - dpufd->dss_base + dpufd->dss_base_phy);

	if (gmp->gmp_mode & GMP_ENABLE)
		dpp_cmdlist_config_gmp(node, addr, index, gmp->gmp_lut_low32bit,
			gmp->gmp_lut_high4bit, DPP_GMP_LUT_LENGTH);

	addr = (uint32_t)(gmp_base + GMP_EN - dpufd->dss_base + dpufd->dss_base_phy);
	dpp_cmdlist_set_reg(node, addr, (gmp->gmp_mode & GMP_ENABLE), 32, 0);
}

static uint32_t dpp_cmdlist_config_degamma(struct dss_cmdlist_node *node,
	uint32_t old_addr, uint32_t index, const uint32_t *degamma_lut,
	uint32_t degamma_length)
{
	uint32_t new_addr;
	uint32_t value;
	uint32_t i, item_index;

	// just consider odd length currently
	degamma_length = (degamma_length + 1) / 2;

	if (index + (degamma_length / MAX_DATA_PER_ITEM + 1) >= get_max_item(node->item_len)) {
		DPU_FB_WARNING("index=%u is larger than %lu!\n",
			index + (degamma_length / MAX_DATA_PER_ITEM + 1),
			get_max_item(node->item_len));
		return node->item_index;
	}

	for (i = 0; i < degamma_length; i += MAX_DATA_PER_ITEM) {
		item_index = i;
		value = (degamma_lut[item_index * 2 + 1] << 16) | degamma_lut[item_index * 2];

		new_addr = old_addr + i * 4;
		new_addr = (new_addr >> 2) & DPP_CMDLIST_ADDR_OFFSET;

		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add0 = new_addr;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data0 = value;

		item_index++;
		value = (degamma_lut[item_index * 2 + 1] << 16) | degamma_lut[item_index * 2];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add1 = 1;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data1 = value;

		item_index++;
		if (item_index == degamma_length - 1)
			value = degamma_lut[item_index * 2];
		else
			value = (degamma_lut[item_index * 2 + 1] << 16) | degamma_lut[item_index * 2];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add2 = 2;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data2 = value;

		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.cnt = 2;
	}
	index += get_item_nums(degamma_length);

	node->item_index = index;
	node->list_header->total_items.bits.count = index;

	return index;
}

static void dpp_cmdlist_set_degamma(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer, const struct degamma_info *degamma)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t index = 0;
	uint32_t addr = 0;
	char __iomem *degamma_base = NULL;
	char __iomem *degamma_lut_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data is NULL");
		return;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	DPU_FB_DEBUG("+ region=%d, buffer=%d", region, buffer);

	degamma_base = dpufd->dss_base + DSS_DPP_DEGAMMA_OFFSET +
		region * DPP_DEGAMMA_BASE;
	degamma_lut_base = dpufd->dss_base + DSS_DPP_DEGAMMA_LUT_OFFSET +
		region * DPP_DEGAMMA_BASE;

	index = dpp_cmdlist_get_module_index(
		dpufd->dpp_cmdlist_data[region][buffer], EN_DPP_CH_MODULE_DEGAMMA);

	// degamma lut
	if (degamma->degamma_enable) {
		addr = (uint32_t)(degamma_lut_base + U_DEGAMA_R_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_degamma(node, addr, index,
			degamma->degamma_r_lut, DPP_DEGAMMA_LUT_LENGTH);

		addr = (uint32_t)(degamma_lut_base + U_DEGAMA_G_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_degamma(node, addr, index,
			degamma->degamma_g_lut, DPP_DEGAMMA_LUT_LENGTH);

		addr = (uint32_t)(degamma_lut_base + U_DEGAMA_B_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_degamma(node, addr, index,
			degamma->degamma_b_lut, DPP_DEGAMMA_LUT_LENGTH);
	}

	addr = (uint32_t)(degamma_base + DEGAMA_EN - dpufd->dss_base +
		dpufd->dss_base_phy);
	dpp_cmdlist_set_reg(node, addr, degamma->degamma_enable, 32, 0);
}

static uint32_t dpp_cmdlist_config_gamma(struct dss_cmdlist_node *node,
	uint32_t addr, uint32_t index,
	const uint32_t *gamma_lut, uint32_t gamma_length)
{
	uint32_t new_addr;
	uint32_t value;
	uint32_t i, item_index;

	// just consider odd length currently
	gamma_length = (gamma_length + 1) / 2;

	if (index + (gamma_length / MAX_DATA_PER_ITEM + 1) >= get_max_item(node->item_len)) {
		DPU_FB_WARNING("index=%u is larger than %lu!\n",
			index + (gamma_length / MAX_DATA_PER_ITEM + 1),
			get_max_item(node->item_len));
		return node->item_index;
	}

	for (i = 0; i < gamma_length; i += MAX_DATA_PER_ITEM) {
		item_index = i;
		value = (gamma_lut[item_index * 2 + 1] << 16) | gamma_lut[item_index * 2];
		new_addr = addr + i * 4;
		new_addr = (new_addr >> 2) & DPP_CMDLIST_ADDR_OFFSET;
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add0 = new_addr;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data0 = value;

		item_index++;
		value = (gamma_lut[item_index * 2 + 1] << 16) | gamma_lut[item_index * 2];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add1 = 1;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data1 = value;

		item_index++;
		if (item_index == gamma_length - 1)
			value = gamma_lut[item_index * 2];
		else
			value = (gamma_lut[item_index * 2 + 1] << 16) | gamma_lut[item_index * 2];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add2 = 2;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data2 = value;

		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.cnt = 2;
	}
	index += get_item_nums(gamma_length);

	node->item_index = index;
	node->list_header->total_items.bits.count = index;

	return index;
}

static void dpp_cmdlist_set_gamma(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer, const struct gama_info *gama)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t index = 0;
	uint32_t addr = 0;
	char __iomem *gama_base = NULL;
	char __iomem *gamma_lut_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data is NULL");
		return;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	DPU_FB_DEBUG("+ region=%d, buffer=%d", region, buffer);

	gama_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET +
		region * DPP_GAMMA_BASE;
	gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET +
		region * DPP_GAMMA_BASE;

	index = dpp_cmdlist_get_module_index(
		dpufd->dpp_cmdlist_data[region][buffer], EN_DPP_CH_MODULE_GAMMA);

	// gamma lut
	if (gama->gama_enable) {
		addr = (uint32_t)(gamma_lut_base + U_GAMA_R_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_gamma(node, addr, index,
			gama->gama_r_lut, DPP_GAMMA_LUT_LENGTH);

		addr = (uint32_t)(gamma_lut_base + U_GAMA_G_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_gamma(node, addr, index,
			gama->gama_g_lut, DPP_GAMMA_LUT_LENGTH);

		addr = (uint32_t)(gamma_lut_base + U_GAMA_B_COEF -
			dpufd->dss_base + dpufd->dss_base_phy);
		index = dpp_cmdlist_config_gamma(node, addr, index,
			gama->gama_b_lut, DPP_GAMMA_LUT_LENGTH);
	}

	addr = (uint32_t)(gama_base + GAMA_EN - dpufd->dss_base +
		dpufd->dss_base_phy);
	dpp_cmdlist_set_reg(node, addr, gama->gama_enable, 32, 0);
}

static void dpp_cmdlist_config_xcc(struct dss_cmdlist_node *node,
	uint32_t addr, uint32_t index,
	const uint32_t *xcc_table, uint32_t xcc_length)
{
	uint32_t new_addr;
	uint32_t value;
	uint32_t i, item_index;

	if ((index + xcc_length / MAX_DATA_PER_ITEM + 1) >= get_max_item(node->item_len)) {
		DPU_FB_WARNING("index=%u is larger than %lu!\n",
			(index + xcc_length / MAX_DATA_PER_ITEM + 1),
			get_max_item(node->item_len));
		return;
	}

	// just consider 3 mutiple currently
	for (i = 0; i < xcc_length; i += MAX_DATA_PER_ITEM) {
		item_index = i;
		value = xcc_table[item_index];
		new_addr = addr + i * 4;
		new_addr = (new_addr >> 2) & DPP_CMDLIST_ADDR_OFFSET;
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add0 = new_addr;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data0 = value;

		item_index++;
		value = xcc_table[item_index];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add1 = 1;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data1 = value;

		item_index++;
		value = xcc_table[item_index];
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.add2 = 2;
		node->list_item[index + i / MAX_DATA_PER_ITEM].data2 = value;

		node->item_flag = MAX_DATA_PER_ITEM;
		node->list_item[index + i / MAX_DATA_PER_ITEM].reg_addr.bits.cnt = 2;
	}
	index += get_item_nums(xcc_length);

	node->item_index = index;
	node->list_header->total_items.bits.count = index;
}

static void dpp_cmdlist_set_xcc(struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer, const struct xcc_info *xcc)
{
	struct dss_cmdlist_node *node = NULL;
	uint32_t index = 0;
	uint32_t addr = 0;
	char __iomem *xcc_lut_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->dpp_cmdlist_data[region][buffer] == NULL) {
		DPU_FB_ERR("dpp_cmdlist_data is NULL");
		return;
	}

	node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (node == NULL) {
		DPU_FB_ERR("node is NULL");
		return;
	}

	DPU_FB_DEBUG("+ region=%d, buffer=%d", region, buffer);

	xcc_lut_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET +
		region * DPP_XCC_LUT_BASE;

	index = dpp_cmdlist_get_module_index(
		dpufd->dpp_cmdlist_data[region][buffer], EN_DPP_CH_MODULE_XCC);

	// XCC
	addr = (uint32_t)(xcc_lut_base + XCC_COEF_00 - dpufd->dss_base +
		dpufd->dss_base_phy);

	if (xcc->xcc_enable)
		dpp_cmdlist_config_xcc(node, addr, index, xcc->xcc_table,
		DPP_XCC_TABLE_LENGTH);

	addr = (uint32_t)(xcc_lut_base + XCC_EN - dpufd->dss_base +
		dpufd->dss_base_phy);
	dpp_cmdlist_set_reg(node, addr, xcc->xcc_enable, 32, 0);
}

static int dpp_cmdlist_config_reset(const struct dpu_fb_data_type *dpufd, bool reset)
{
	char __iomem *cmdlist_base = NULL;
	const uint32_t offset = CMDLIST_REG_OFFSET;
	const uint32_t i = DPP_CMDLIST_OFFSET;
	int status_temp = 0;
	int ret = 0;

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		return -EINVAL;
	}

	cmdlist_base = dpufd->dss_base + DSS_CMDLIST_OFFSET;
	status_temp = inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
	if (((uint32_t)status_temp & 0x800) == 0x0) {
		DPU_FB_WARNING("CH15 not idle");
		if (reset)
			set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset,
				0x6, 3, 2);
		else
			ret = -1;
	}

	return ret;
}

static int dpp_cmdlist_config_start(const struct dpu_fb_data_type *dpufd,
	uint8_t region, uint8_t buffer)
{
	char __iomem *mctl_base;
	char __iomem *cmdlist_base;
	struct dss_cmdlist_node *cmdlist_node = NULL;
	const uint32_t offset = CMDLIST_REG_OFFSET;
	const uint32_t i = DPP_CMDLIST_OFFSET;
	uint32_t status_temp, ints_temp;

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		return -EINVAL;
	}

	mctl_base = dpufd->dss_base + DPP_CMDLIST_MCTL_OFFSET;
	cmdlist_base = dpufd->dss_base + DSS_CMDLIST_OFFSET;

	DPU_FB_DEBUG("+");

	status_temp = inp32(cmdlist_base + CMDLIST_CH0_STATUS + i * offset);
	ints_temp = inp32(cmdlist_base + CMDLIST_CH0_INTS + i * offset);

	cmdlist_node = dpufd->dpp_cmdlist_data[region][buffer]->cmdlist_node;
	if (cmdlist_node == NULL) {
		DPU_FB_ERR("cmdlist_node is NULL!\n");
		return -EINVAL;
	}

	outp32(cmdlist_base + CMDLIST_ADDR_MASK_EN, BIT(i));
	set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x0, 1, 6);
	set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x5, 3, 2);
	set_reg(cmdlist_base + CMDLIST_CH0_STAAD + i * offset,
		cmdlist_node->header_phys, 32, 0);
	set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x1, 1, 0);
	set_reg(cmdlist_base + CMDLIST_CH0_INTE + i * offset, 0xFF, 8, 0);

	if (((status_temp & 0xF) == 0x0) || ((ints_temp & 0x2) == 0x2))
		set_reg(cmdlist_base + CMDLIST_SWRST, 0x1, 1, i);
	else
		DPU_FB_WARNING("status_temp=0x%x, ints_temp=0x%x\n",
			status_temp, ints_temp);

	outp32(cmdlist_base + CMDLIST_ADDR_MASK_DIS, (1 << i));
	set_reg(cmdlist_base + CMDLIST_CH0_CTRL + i * offset, 0x5, 3, 2);
	set_reg(mctl_base + MCTL_CTL_TOP, 0x1, 2, 0);
	set_reg(mctl_base + MCTL_CTL_ST_SEL, 0x1, 1, 0);
	set_reg(mctl_base + MCTL_CTL_SW_ST, 0x1, 1, 0);

	DPU_FB_DEBUG("-");
	return 0;
}

/*lint -e514*/
static uint8_t dpp_cmdlist_get_config_region(bool in_roi)
{
	uint32_t dst_channel;
	uint8_t region = DPP_CMDLIST_REGION_NUM;

	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;
	DPU_FB_DEBUG("dst_channel %d\n", dst_channel);
	if (in_roi ^ (bool)dst_channel)
		region = 0;
	else
		region = 1;

	return region;
}
/*lint -e514*/

static uint8_t dpp_cmdlist_get_config_dpp(uint8_t config_region)
{
	bool in_roi = false;

	if ((config_region == CONFIG_REGION_TOP &&
		g_dpp_cmdlist_state == EN_DPP_CMDLIST_ENABLE_TOP) ||
		(config_region == CONFIG_REGION_BOTTOM &&
			g_dpp_cmdlist_state == EN_DPP_CMDLIST_ENABLE_BOTTOM))
		in_roi = true;
	else
		in_roi = false;

	return dpp_cmdlist_get_config_region(in_roi);
}

// External API
bool dpu_dpp_cmdlist_get_enable(void)
{
	return (g_dpp_cmdlist_state >= EN_DPP_CMDLIST_ENABLE_TOP ? true : false);
}

void dpu_dpp_cmdlist_init(struct dpu_fb_data_type *dpufd)
{
	uint8_t i,j;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	// only support primary display
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	for (i = 0; i < DPP_CMDLIST_REGION_NUM; i++)
		for (j = 0; j < DPP_CMDLIST_BUFFER_NUM; j++) {
			dpufd->dpp_cmdlist_data[i][j] = dpp_cmdlist_data_alloc(dpufd);
			if (dpp_cmdlist_set_node(dpufd, i, j) < 0)
				goto err_out;
		}

	spin_lock_init(&dpufd->dpp_cmdlist_ch_status);
	spin_lock_init(&dpufd->dpp_cmdlist_lock);
	spin_lock_init(&g_dpe_irq_lock);

	g_dpp_cmdlist_state = EN_DPP_CMDLIST_DISABLE;
	g_dpp_cmdlist_delay = CMDLIST_DELAY_STATUS_COUNT;

	return;

err_out:
	for (i = 0; i < DPP_CMDLIST_REGION_NUM; i++)
		for (j = 0; j < DPP_CMDLIST_BUFFER_NUM; j++)
			dpp_cmdlist_data_free(dpufd, i, j);
}

void dpu_dpp_cmdlist_interrupt_on(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_data_type * dpufd_dpp_cmdlist = NULL;
	unsigned long flags = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	spin_lock_irqsave(&g_dpe_irq_lock, flags);
	if (dpufd->index != AUXILIARY_PANEL_IDX) {
		dpe_interrupt_mask(dpufd);
		dpe_interrupt_clear(dpufd);
		dpe_irq_enable(dpufd);
		dpe_interrupt_unmask(dpufd);
	} else {
		if (dpufd->fb2_irq_on_flag == false) {
			if (dpufd->fb2_irq_force_on_flag == false) {
				dpe_interrupt_mask(dpufd);
				dpe_interrupt_clear(dpufd);
				dpe_irq_enable(dpufd);
				dpe_interrupt_unmask(dpufd);
			}
			dpufd->fb2_irq_on_flag = true;
		}
	}

	// dpp cmdlist use fb2 intr, so fb2 config need be set on along with fb0
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dpufd_dpp_cmdlist = dpufd_list[AUXILIARY_PANEL_IDX]; // force use AUXILIARY_PANEL
		if (dpufd_dpp_cmdlist->fb2_irq_force_on_flag == false) {
			if (dpufd->fb2_irq_on_flag == false) {
				dpe_interrupt_mask(dpufd_dpp_cmdlist);
				dpe_interrupt_clear(dpufd_dpp_cmdlist);
				dpe_irq_enable(dpufd_dpp_cmdlist);
				dpe_interrupt_unmask(dpufd_dpp_cmdlist);
			}
			dpufd_dpp_cmdlist->fb2_irq_force_on_flag = true;
		}
		dpu_dpe_cmdlist_interrupt_unmask(dpufd_dpp_cmdlist);
		dpu_dpe_vactive_line_interrupt(dpufd, true);
	}
	spin_unlock_irqrestore(&g_dpe_irq_lock, flags);
}

void dpu_dpp_cmdlist_interrupt_off(struct dpu_fb_data_type *dpufd)
{
	unsigned long flags = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	spin_lock_irqsave(&g_dpe_irq_lock, flags);
	if (dpufd->index != AUXILIARY_PANEL_IDX) {
		dpe_interrupt_mask(dpufd);
		dpe_irq_disable_nosync(dpufd);
	} else {
		if (dpufd->fb2_irq_on_flag == true) {
			if (dpufd->fb2_irq_force_on_flag == false) {
				dpe_interrupt_mask(dpufd);
				dpe_irq_disable_nosync(dpufd);
			}
			dpufd->fb2_irq_on_flag = false;
		}
	}

	// dpp cmdlist use fb2 intr, so fb2 config need be set off along with fb0
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd_list[AUXILIARY_PANEL_IDX]->fb2_irq_force_on_flag == true) {
			if (dpufd_list[AUXILIARY_PANEL_IDX]->fb2_irq_on_flag == false) {
				dpe_interrupt_mask(dpufd_list[AUXILIARY_PANEL_IDX]);
				dpe_irq_disable_nosync(dpufd_list[AUXILIARY_PANEL_IDX]);
			}
			dpufd_list[AUXILIARY_PANEL_IDX]->fb2_irq_force_on_flag = false;
		}
		dpu_dpe_vactive_line_interrupt(dpufd, false);
	}
	spin_unlock_irqrestore(&g_dpe_irq_lock, flags);
}

struct dss_rect effect_get_combined_roi(const struct roi_rect *roi,
	uint32_t roi_region_count)
{
	struct dss_rect rect = { 0, 0, 0, 0 };
	uint8_t i;
	uint8_t count = 0;

	if (!roi) {
		DPU_FB_ERR("roi is NULL\n");
		return rect;
	}

	if (roi_region_count > DPP_BUF_ROI_REGION_COUNT) {
		DPU_FB_ERR("roi region count out of bounds!\n");
		return rect;
	}

	// since one roi is used currently, temply support one roi only
	for (i = 0; i < roi_region_count; i++) {
		if ((roi[i].top_left_x < roi[i].bottom_right_x) &&
			(roi[i].top_left_y < roi[i].bottom_right_y)) {
			rect.x = roi[i].top_left_x;
			rect.y = roi[i].top_left_y;
			rect.w = roi[i].bottom_right_x - rect.x + 1;
			rect.h = roi[i].bottom_right_y - rect.y + 1;
			count++;
			DPU_FB_INFO("[effect]roi: %d, %d, %d, %d\n",
				rect.x, rect.y, rect.w, rect.h);
		}
	}

	if (count > 1)
		DPU_FB_WARNING("[effect]more than one roi is valid!\n");

	return rect;
}

void dpu_dpp_cmdlist_roi_config(struct dpu_fb_data_type *dpufd,
	struct dss_rect roi)
{
	bool is_roi_valid = false;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	// only support primary display
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	if (g_dpp_cmdlist_state == EN_DPP_CMDLIST_NOT_INIT) {
		DPU_FB_ERR("dpp_cmdlist has not beed init!\n");
		return;
	}

	if (g_dpp_cmdlist_state == EN_DPP_CMDLIST_ENABLE_TOP ||
		g_dpp_cmdlist_state == EN_DPP_CMDLIST_ENABLE_BOTTOM)
		is_roi_valid = true;

	if (is_roi_valid) {
		if (roi.y == 0) {
			dpufd->dpp_cmdlist_isr_vactive_cnt = roi.h;
		} else {
			dpufd->dpp_cmdlist_isr_vactive_cnt =
				dpufd->panel_info.yres - roi.h;
			// patch value 0.6x to limit not exceed ROI setting
			if (dpufd->dpp_cmdlist_isr_vactive_cnt > dpufd->panel_info.yres / 2)
				dpufd->dpp_cmdlist_isr_vactive_cnt = dpufd->panel_info.yres * 3 / 5;
		}
	} else {
		dpufd->dpp_cmdlist_isr_vactive_cnt = 0;
	}

	// enable or disable line interrupt
	dpu_dpe_vactive_line_interrupt(dpufd, is_roi_valid);
}

static int dpu_dpp_cmdlist_roi_gei_region(struct dpu_fb_data_type *dpufd,
	uint8_t config_region, uint8_t* dpp_out)
{
	if (!dpu_dpp_cmdlist_get_enable())
		return -EINVAL;

	*dpp_out = dpp_cmdlist_get_config_dpp(config_region);
	if ((*dpp_out) >= DPP_CMDLIST_REGION_NUM) {
		DPU_FB_ERR("error config_dpp:%d", *dpp_out);
		return -EINVAL;
	}

	if (config_region == CONFIG_REGION_TOP &&
		(g_dpp_cmdlist_delay == CMDLIST_DELAY_STATUS_ENABLE || g_dpp_cmdlist_delay == CMDLIST_DELAY_STATUS_WAIT)) {
		g_dpp_cmdlist_delay = CMDLIST_DELAY_STATUS_WAIT;
		DPU_FB_INFO("wait for refresh frame to complete\n");
		return -EINVAL;
	}

	if (config_region == CONFIG_REGION_BOTTOM &&
		g_dpp_cmdlist_delay == CMDLIST_DELAY_STATUS_WAIT) {
		g_dpp_cmdlist_delay = CMDLIST_DELAY_STATUS_DISABLE;
		DPU_FB_INFO("the current frame allows to configure TOP\n");
	}

	if (config_region == CONFIG_REGION_BOTTOM &&
		dpufd->dpp_cmdlist_isr_vactive_cnt == 0) {
		DPU_FB_INFO("wait BIT_VACTIVE_CNT take effect at next frame\n");
		return -EINVAL;
	}

	return 0;
}

// Note: this function called by isr routine, which should be called sequetially
void dpu_dpp_cmdlist_region_start(struct dpu_fb_data_type *dpufd,
	uint8_t config_region)
{
	bool reset = false;
	uint8_t config_dpp;
	uint8_t buffer_rd, last_buffer_rd;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	if (dpufd->effect_info[disp_panel_id].dpp_cmdlist_type == DPP_CMDLIST_TYPE_ROI) {
		if (dpu_dpp_cmdlist_roi_gei_region(dpufd, config_region, &config_dpp))
			return;
	} else if (dpufd->effect_info[disp_panel_id].dpp_cmdlist_type == DPP_CMDLIST_TYPE_COMMON) {
		config_dpp = config_region;
	} else {
		return;
	}

	spin_lock(&dpufd->dpp_cmdlist_lock);
	buffer_rd = dpufd->dpp_cmdlist_buffer_rd[config_dpp];

	// verify sequence
	if (config_dpp == dpufd->dpp_cmdlist_region) {
		DPU_FB_DEBUG("set same cmdlist region!");
		if (dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status ==
			EN_DPP_CMDLIST_STATUS_WORKING) {
			// reset status for re-work
			dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status =
				EN_DPP_CMDLIST_STATUS_READY;
			reset = true;
		}
	}

	if ((dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status & 0xF0) !=
		EN_DPP_CMDLIST_STATUS_READY) {
		DPU_FB_DEBUG("CMDLIST status not ready. config_dpp=%d, "
			"buffer_rd=%d, status=0x%x",
			config_dpp, buffer_rd,
			dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status);
		spin_unlock(&dpufd->dpp_cmdlist_lock);
		return;
	}

	if (g_debug_dpp_cmdlist_debug)
		DPU_FB_INFO("config_dpp=%d, buffer_rd=%d, stutas=0x%x, config_region=%d",
			config_dpp, buffer_rd,
			dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status,
			config_region);

	// start working, not write anymore
	dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status =
		EN_DPP_CMDLIST_STATUS_WORKING;

	// check last region start complete
	last_buffer_rd = dpufd->dpp_cmdlist_buffer_rd[dpufd->dpp_cmdlist_region];
	if (dpufd->dpp_cmdlist_data[dpufd->dpp_cmdlist_region][last_buffer_rd]->buffer_status ==
		EN_DPP_CMDLIST_STATUS_WORKING && config_dpp != dpufd->dpp_cmdlist_region) {
		// reset last status to free
		dpp_cmdlist_set_read_ptr(dpufd, dpufd->dpp_cmdlist_region);
		reset = true;
		DPU_FB_INFO("reset last cmdlist to free: region=%d, buffer_rd=%d, last_buffer_rd=%d",
			dpufd->dpp_cmdlist_region, buffer_rd, last_buffer_rd);
	}

	if (!dpp_cmdlist_config_reset(dpufd, reset)) {
		dpp_cmdlist_node_dump(dpufd, 0, config_dpp, buffer_rd);
		if (!dpp_cmdlist_config_start(dpufd, config_dpp, buffer_rd))
			dpufd->dpp_cmdlist_region = config_dpp; // start success
		else
			dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status =
				EN_DPP_CMDLIST_STATUS_READY;
	} else {
		dpufd->dpp_cmdlist_data[config_dpp][buffer_rd]->buffer_status =
			EN_DPP_CMDLIST_STATUS_READY;
	}

	spin_unlock(&dpufd->dpp_cmdlist_lock);
}

void dpu_dpp_cmdlist_region_complete(void)
{
	uint32_t ints_temp;
	const uint32_t offset = CMDLIST_REG_OFFSET;
	const uint32_t i = DPP_CMDLIST_OFFSET;
	struct dpu_fb_data_type *dpufd_primary;
	uint32_t *ch_status = NULL;
	uint32_t dst_channel = 0;

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd_primary == NULL) {
		DPU_FB_ERR("dpufd_primary is NULL");
		return;
	}

	ch_status = dpufd_primary->effect_info[dpufd_primary->panel_info.disp_panel_id].dpp_chn_status;
	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;

	// read intr
	ints_temp = inp32(dpufd_primary->dss_base + DSS_CMDLIST_OFFSET +
		CMDLIST_CH0_INTS + i * offset);
	if ((ints_temp & 0x2) == 0x2) {
		// cmdlist write complete here
		DPU_FB_DEBUG("dpp cmdlist all done ints=0x%x", ints_temp);
		spin_lock(&dpufd_primary->dpp_cmdlist_lock);
		dpp_cmdlist_set_read_ptr(dpufd_primary,
			dpufd_primary->dpp_cmdlist_region);
		spin_unlock(&dpufd_primary->dpp_cmdlist_lock);

		spin_lock(&dpufd_primary->dpp_cmdlist_ch_status);
		*(ch_status + dst_channel) = DPP_CHN_CONFIG_READY;
		*(ch_status + ((~dst_channel) & 0x1)) = DPP_CHN_NEED_CONFIG;
		spin_unlock(&dpufd_primary->dpp_cmdlist_ch_status);
	}

	// clear intr
	outp32(dpufd_primary->dss_base + DSS_CMDLIST_OFFSET + CMDLIST_CH0_INTC +
		i * offset, ints_temp);
}

int dpu_dpp_cmdlist_set_lut_roi(struct dpu_fb_data_type *dpufd,
	uint32_t modules, struct dpp_buf_info *buf_info, bool in_roi)
{
	uint8_t region, wr_buffer;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	if (buf_info == NULL) {
		DPU_FB_ERR("info is NULL");
		return -EINVAL;
	}
	if (dpufd->panel_power_on != true) {
		DPU_FB_DEBUG("power is off, needn't to set dpp cmdlist");
		return 0;
	}

	region = dpp_cmdlist_get_config_region(in_roi);
	if (region >= DPP_CMDLIST_REGION_NUM) {
		DPU_FB_ERR("wrong state, can not set by cmdlist!");
		return -EINVAL;
	}

	if ((modules & DSS_EFFECT_MODULE_LCP_GMP) &&
		dpufd->effect_ctl.lcp_gmp_support &&
		buf_info->gmp_buf_save_status != DPP_BUF_INVALIED) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, region,
			EN_DPP_CH_MODULE_GMP);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_gmp(dpufd, region, wr_buffer, &buf_info->gmp);
			dpp_cmdlist_write_complete(dpufd, region, wr_buffer,
				EN_DPP_CH_MODULE_GMP);
			buf_info->gmp_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
		}
	}

	if ((modules & DSS_EFFECT_MODULE_LCP_IGM) &&
		dpufd->effect_ctl.lcp_igm_support &&
		buf_info->degama_buf_save_status != DPP_BUF_INVALIED) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, region,
			EN_DPP_CH_MODULE_DEGAMMA);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_degamma(dpufd, region, wr_buffer,
				&buf_info->degamma);
			dpp_cmdlist_write_complete(dpufd, region, wr_buffer,
				EN_DPP_CH_MODULE_DEGAMMA);
			buf_info->degama_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
		}
	}

	if ((modules & DSS_EFFECT_MODULE_LCP_XCC) &&
		dpufd->effect_ctl.lcp_xcc_support &&
		buf_info->xcc_buf_save_status != DPP_BUF_INVALIED) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, region,
			EN_DPP_CH_MODULE_XCC);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_xcc(dpufd, region, wr_buffer, buf_info->xcc);
			dpp_cmdlist_write_complete(dpufd, region, wr_buffer,
				EN_DPP_CH_MODULE_XCC);
			buf_info->xcc_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
		}
	}

	if ((modules & DSS_EFFECT_MODULE_GAMMA) &&
		dpufd->effect_ctl.gamma_support &&
		buf_info->gama_buf_save_status != DPP_BUF_INVALIED) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, region,
			EN_DPP_CH_MODULE_GAMMA);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_gamma(dpufd, region, wr_buffer,
				&buf_info->gama);
			dpp_cmdlist_write_complete(dpufd, region, wr_buffer,
				EN_DPP_CH_MODULE_GAMMA);
			buf_info->gama_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
		}
	}

	return 0;
}

// patch function to limit ROI location
void dpu_effect_dpproi_verify(struct dpu_fb_data_type *dpufd,
	struct dss_effect_info *effect_info)
{
	uint32_t enable_temp;
	compat_pointer(table_temp) a;

	dpu_check_and_no_retval((!dpufd || !effect_info), ERR, "dpufd or effect_info is NULL\n");

	if (effect_info->dpp_roi[0].top_left_y == 0 &&
		effect_info->dpp_roi[0].bottom_right_y != dpufd->panel_info.yres - 1) {
		effect_info->dpp_roi[0].top_left_y = effect_info->dpp_roi[0].bottom_right_y + 1;
		effect_info->dpp_roi[0].bottom_right_y = dpufd->panel_info.yres - 1;

		if ((effect_info->modules & DSS_EFFECT_MODULE_GAMMA) == DSS_EFFECT_MODULE_GAMMA) {
			enable_temp = effect_info->gamma.enable;
			effect_info->gamma.enable = effect_info->gamma.roi_enable;
			effect_info->gamma.roi_enable = enable_temp;

			a.table_temp = effect_info->gamma.gamma_r_table;
			effect_info->gamma.gamma_r_table = effect_info->gamma.gamma_roi_r_table;
			effect_info->gamma.gamma_roi_r_table = a.table_temp;

			a.table_temp = effect_info->gamma.gamma_g_table;
			effect_info->gamma.gamma_g_table = effect_info->gamma.gamma_roi_g_table;
			effect_info->gamma.gamma_roi_g_table = a.table_temp;

			a.table_temp = effect_info->gamma.gamma_b_table;
			effect_info->gamma.gamma_b_table = effect_info->gamma.gamma_roi_b_table;
			effect_info->gamma.gamma_roi_b_table = a.table_temp;
		}
	}
}

void dpu_display_effect_set_dpp_config_type(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return ;
	}

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);
	pinfo = &(dpufd->panel_info);

	if (dpu_check_panel_feature_support(pinfo)) {
		dpufd->effect_info[pinfo->disp_panel_id].dpp_cmdlist_type = DPP_CMDLIST_TYPE_ROI;
	} else {
#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
		if (g_debug_dpp_cmdlist_type == 1)
			dpufd->effect_info[pinfo->disp_panel_id].dpp_cmdlist_type = DPP_CMDLIST_TYPE_COMMON;
		else
			dpufd->effect_info[pinfo->disp_panel_id].dpp_cmdlist_type = DPP_CMDLIST_TYPE_NONE;
#else
		dpufd->effect_info[pinfo->disp_panel_id].dpp_cmdlist_type = DPP_CMDLIST_TYPE_NONE;
#endif
	}

	return;
}

static int dump_buffer_status(struct dpu_fb_data_type *dpufd)
{
	int i,j;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	for (i = 0; i < DPP_CMDLIST_REGION_NUM; i++) {
		for (j = 0; j < DPP_CMDLIST_BUFFER_NUM; j++)
			DPU_FB_INFO("CMD list buffer status: %d ; DPP: %d ; IDX: %d ",
				dpufd->dpp_cmdlist_data[i][j]->buffer_status, i, j);
	}

	return 0;
}

int dpu_effect_dpp_cmdlist_set_lut_common(struct dpu_fb_data_type *dpufd,
	struct dpp_buf_info *buf_info, uint32_t dpp_sel)
{
	uint8_t wr_buffer;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	if (buf_info == NULL) {
		DPU_FB_ERR("info is NULL");
		return -EINVAL;
	}

	if (dpufd->dpp_cmdlist_start_config != true) {
		DPU_FB_DEBUG("power is off, needn't to set dpp cmdlist");
		return 0;
	}

	if (dpufd->effect_ctl.lcp_gmp_support &&
		buf_info->gmp_buf_save_status != DPP_BUF_INVALIED &&
		(buf_info->gmp_buf_cfg_status & (1 << dpp_sel))) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, dpp_sel,
			EN_DPP_CH_MODULE_GMP);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_gmp(dpufd, dpp_sel, wr_buffer, &buf_info->gmp);
			dpp_cmdlist_write_complete(dpufd, dpp_sel, wr_buffer, EN_DPP_CH_MODULE_GMP);
			buf_info->gmp_buf_cfg_status &= ~((uint32_t)1 << dpp_sel);
		}
	}

	if (dpufd->effect_ctl.lcp_igm_support &&
		buf_info->degama_buf_save_status != DPP_BUF_INVALIED &&
		(buf_info->degama_buf_cfg_status & (1 << dpp_sel))) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, dpp_sel,
			EN_DPP_CH_MODULE_DEGAMMA);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_degamma(dpufd, dpp_sel, wr_buffer, &buf_info->degamma);
			dpp_cmdlist_write_complete(dpufd, dpp_sel, wr_buffer, EN_DPP_CH_MODULE_DEGAMMA);
			buf_info->degama_buf_cfg_status &= ~((uint32_t)1 << dpp_sel);
		}
	}

	if (dpufd->effect_ctl.lcp_xcc_support &&
		buf_info->xcc_buf_save_status != DPP_BUF_INVALIED &&
		(buf_info->xcc_buf_cfg_status & (1 << dpp_sel))) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, dpp_sel,
			EN_DPP_CH_MODULE_XCC);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_xcc(dpufd, dpp_sel, wr_buffer, buf_info->xcc);
			dpp_cmdlist_write_complete(dpufd, dpp_sel, wr_buffer, EN_DPP_CH_MODULE_XCC);
			buf_info->xcc_buf_cfg_status &= ~((uint32_t)1 << dpp_sel);
		}
	}

	if (dpufd->effect_ctl.gamma_support &&
		buf_info->gama_buf_save_status != DPP_BUF_INVALIED &&
		(buf_info->gama_buf_cfg_status & (1 << dpp_sel))) {
		wr_buffer = dpp_cmdlist_get_write_ptr(dpufd, dpp_sel,
				EN_DPP_CH_MODULE_GAMMA);
		if (wr_buffer < DPP_CMDLIST_BUFFER_NUM) {
			dpp_cmdlist_set_gamma(dpufd, dpp_sel, wr_buffer, &buf_info->gama);
			dpp_cmdlist_write_complete(dpufd, dpp_sel, wr_buffer, EN_DPP_CH_MODULE_GAMMA);
			buf_info->gama_buf_cfg_status &= ~((uint32_t)1 << dpp_sel);
		}
	}

	return 0;
}

static bool dpu_dpp_cmdlist_buffer_need_update(struct dpu_fb_data_type *dpufd,
	int8_t config_region)
{
	uint8_t buffer_rd;

	buffer_rd = dpufd->dpp_cmdlist_buffer_rd[config_region];

	/* same dpp whith last time  && empty buffer */
	if ((dpufd->dpp_cmdlist_data[config_region][buffer_rd]->buffer_status & 0xF0) ==
			EN_DPP_CMDLIST_STATUS_READY)
		return true;

	return false;
}

static bool dpu_dpp_cmdlist_buffer_check_empty(struct dpu_fb_data_type *dpufd)
{
	int i,j;
	int count_finished, count_free;

	count_free = count_finished = 0;
	for (i = 0; i < DPP_CMDLIST_REGION_NUM; i++) {
		for (j = 0; j < DPP_CMDLIST_BUFFER_NUM; j++) {
			if (dpufd->dpp_cmdlist_data[i][j]->buffer_status == EN_DPP_CMDLIST_STATUS_FINISHED)
				count_finished++;

			if (dpufd->dpp_cmdlist_data[i][j]->buffer_status == EN_DPP_CMDLIST_STATUS_FREE)
				count_free++;
		}
	}

	if ((count_free == DPP_CMDLIST_REGION_NUM * DPP_CMDLIST_BUFFER_NUM) ||
		(count_finished == DPP_CMDLIST_REGION_NUM * DPP_CMDLIST_BUFFER_NUM))
		return TRUE;
	else
		return FALSE;
}

void dpu_effect_dpp_cmdlist_config(struct dpu_fb_data_type *dpufd,
	uint32_t buff_sel)
{
	uint32_t dst_channel = 0;
	uint32_t *ch_status = NULL;
	unsigned long flags = 0;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "[effect]dpufd is NULL!\n");

	dpp_buff_mngr = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_buf);
	if (!dpufd->dpp_cmdlist_start_config) {
		DPU_FB_DEBUG("[effect] fb_shutdown or panel power down");
		return;
	}

	/* If buffer is empty, dpp_cmdlist_buffer_last_chn status need to be clear */
	if (dpu_dpp_cmdlist_buffer_check_empty(dpufd)) {
		dpufd->dpp_cmdlist_buffer_last_chn = 0xF;
		if (g_debug_dpp_cmdlist_debug)
			DPU_FB_INFO("DPP cmdlist flip clear!");
	}

	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;
	if (dst_channel == dpufd->dpp_cmdlist_buffer_last_chn)
		dst_channel = (dpufd->dpp_cmdlist_buffer_last_chn & 0x1) ? 0 : 1;

	ch_status = dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_chn_status;

	if (buff_sel != DPP_EXTENAL_ROI_BUF) {
		if (g_debug_dpp_cmdlist_debug) {
			dump_buffer_status(dpufd);
			DPU_FB_INFO("DPP[%d]: before rd  %d ,wr  %d ", dst_channel,
				dpufd->dpp_cmdlist_buffer_rd[dst_channel], dpufd->dpp_cmdlist_buffer_wr[dst_channel]);
		}

		(void)dpu_effect_dpp_cmdlist_set_lut_common(dpufd,
			&(dpp_buff_mngr->buf_info_list[dpufd->panel_info.disp_panel_id][buff_sel]), dst_channel);

		spin_lock_irqsave(&dpufd->dpp_cmdlist_ch_status, flags);

		if (*(ch_status + dst_channel) == DPP_CHN_NEED_CONFIG)
			*(ch_status + dst_channel) = DPP_CHN_UPDATE_READY;

		spin_unlock_irqrestore(&dpufd->dpp_cmdlist_ch_status, flags);
		dpufd->dpp_cmdlist_buffer_last_chn = dst_channel;

		if (g_debug_dpp_cmdlist_debug) {
			dump_buffer_status(dpufd);
			DPU_FB_INFO("DPP[%d]: after rd  %d ,wr  %d ", dst_channel,
				dpufd->dpp_cmdlist_buffer_rd[dst_channel], dpufd->dpp_cmdlist_buffer_wr[dst_channel]);
		}
	} else {
		DPU_FB_DEBUG("this channel %d  status is %d, this channel isn't update  %d \n",
			dst_channel, *(ch_status + dst_channel), buff_sel);
	}
}

static bool dpu_is_full_screen(dss_overlay_t *pov_req, uint32_t panel_width, uint32_t panel_height)
{
	if (!pov_req)
		return true;

	if (pov_req->dirty_rect.x == 0 && pov_req->dirty_rect.y == 0 &&
			pov_req->dirty_rect.w == 0 && pov_req->dirty_rect.h == 0)
		return true;

	if (pov_req->dirty_rect.x == 0 && pov_req->dirty_rect.y == 0 &&
			pov_req->dirty_rect.w == panel_width && pov_req->dirty_rect.h == panel_height)
		return true;

	return false;
}

static bool check_dirty_region_empty(struct dpu_fb_data_type *dpufd)
{
	if(!dpu_is_full_screen(&(dpufd->ov_req), dpufd->panel_info.xres, dpufd->panel_info.yres))
		return true;

	if(!dpu_is_full_screen(&(dpufd->ov_req_prev), dpufd->panel_info.xres, dpufd->panel_info.yres))
		return true;

	if(!dpu_is_full_screen(&(dpufd->ov_req_prev_prev), dpufd->panel_info.xres, dpufd->panel_info.yres))
		return true;

	return false;
}

void dpu_dpp_cmdlist_action(struct dpu_fb_data_type *dpufd, uint32_t isr_s2)
{
	uint32_t *ch_status = NULL;
	uint32_t dst_channel = 0;
	uint32_t cur_channel = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

#if defined(CONFIG_DPU_FB_V600)
	if (isr_s2 & BIT_QUARTER_FRM) {
		DPU_FB_INFO("vstart irq is late! isr:0x%x\n", isr_s2);
		return;
	}
#endif
	ch_status = dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_chn_status;
	cur_channel = g_dyn_sw_default;
	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;
	if ((dst_channel != dpufd->dpp_cmdlist_region) &&
		dpu_dpp_cmdlist_buffer_need_update(dpufd, dst_channel)) {
		if (check_dirty_region_empty(dpufd)) {
			DPU_FB_INFO("DPP CMDLIST need bypss dirty region");
			return;
		}

		spin_lock(&dpufd->dpp_cmdlist_ch_status);

		*(ch_status + dst_channel) = DPP_CHN_CONFIG_DOING;
		spin_unlock(&dpufd->dpp_cmdlist_ch_status);
		dpu_dpp_cmdlist_region_start(dpufd, dst_channel);
		g_dpp_cmdlist_bypass_count = 0;
	} else {
		if ((cur_channel == dpufd->dpp_cmdlist_region) &&
			dpu_dpp_cmdlist_buffer_need_update(dpufd, cur_channel)) {
				g_dpp_cmdlist_bypass_count++;
				if (g_dpp_cmdlist_bypass_count == 3) {
					spin_lock(&dpufd->dpp_cmdlist_lock);

					/* cur channel have some buffer isn't updated, but dst channel have nothing.
					 * We should bypass the cur channel buffer to let dpp cmdlist buffer rotate in order.
					 * This mode will discard the last effect config, so DE need make sure the last effect
					 * config need be sent double.
					 */
					dpp_cmdlist_bypass_buffer(dpufd, cur_channel);
					spin_unlock(&dpufd->dpp_cmdlist_lock);
					dpufd->dpp_cmdlist_buffer_last_chn = DPP_CMDLIST_LAST_CH_CLEAR_NONE;
					g_dpp_cmdlist_bypass_count = 0;
				}
			}
	}
}
/*lint -e530*/

// Platform-dependent API
// register maybe different, need re-write according different platform
void dpu_dpe_vactive_line_interrupt(const struct dpu_fb_data_type *dpufd, bool enable)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
#if defined(CONFIG_DPU_FB_V510) || defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	// no need config dsi1 here
	if (enable && dpufd->dpp_cmdlist_isr_vactive_cnt > 0) {
		set_reg(dpufd->mipi_dsi0_base + MIPI_LDI_CPU_ITF_INT_MSK, 0x0,
			1, VACTIVE_CNT_BIT);
		outp32(dpufd->mipi_dsi0_base + MIPI_LDI_VINACT_CNT,
			dpufd->dpp_cmdlist_isr_vactive_cnt); // sync with ROI
	} else {
		set_reg(dpufd->mipi_dsi0_base + MIPI_LDI_CPU_ITF_INT_MSK, 0x1,
			1, VACTIVE_CNT_BIT);
		outp32(dpufd->mipi_dsi0_base + MIPI_LDI_VINACT_CNT, 0);
	}
#endif
}

void dpu_dpe_cmdlist_interrupt_unmask(const struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
#if defined(CONFIG_DPU_FB_V510) || defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	// BIT_OFF_CAM_CMDLIST15_INTS
	set_reg(dpufd->dss_base + GLB_CPU_OFF_CAM_INT_MSK, 0x0, 1, 1);
#endif
}

size_t dpu_display_effect_get_dpp_lut_size(const struct dpu_fb_data_type *dpufd)
{
	size_t size = 0;
	size_t length;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return 0;
	}

	length = DPP_GMP_LUT_LENGTH * 2;          // GMP lut word size
	length = get_cmdlist_item_len(length);    // covert to cmdlist item size
	size += length;

	length = (DPP_GAMMA_LUT_LENGTH + 1) / 2;  // Gamma lut word size, two value one word
	length = get_cmdlist_item_len(length);    // covert to cmdlist item size
	size += length * 3;                       // R G B three channel

	length = (DPP_DEGAMMA_LUT_LENGTH + 1) / 2; // Degamma lut word size
	length = get_cmdlist_item_len(length);     // covert to cmdlist item size
	size += length * 3;                        // R G B three channel

	size += DPP_XCC_TABLE_LENGTH;              // XCC lut word size
	length = get_cmdlist_item_len(length);     // covert to cmdlist item size
	size += length * 3;

	// covert to byte size
	size *= BYTES_PER_TABLE_ELEMENT;

	return size;
}

void dpu_dpp_cmdlist_reset(struct dpu_fb_data_type *dpufd)
{
	uint32_t i,j;
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	for (i = 0; i < DPP_CMDLIST_REGION_NUM; i++) {
		for (j = 0; j < DPP_CMDLIST_BUFFER_NUM; j++)
			dpp_cmdlist_reset_data(dpufd->dpp_cmdlist_data[i][j]);
		dpufd->dpp_cmdlist_buffer_wr[i] = 0;
		dpufd->dpp_cmdlist_buffer_rd[i] = 0;
	}
	dpufd->dpp_cmdlist_region = 0;
	dpufd->dpp_cmdlist_buffer_last_chn = DPP_CMDLIST_LAST_CH_CLEAR_NONE;
	dpufd->dpp_cmdlist_isr_vactive_cnt = 0;
	dpufd->dpp_cmdlist_none_update_frame = 0xFF;
	dpufd->dpp_cmdlist_start_config = true;

	g_dpp_cmdlist_bypass_count = 0;
}

void dpu_dpp_cmdlist_close(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	dpufd->dpp_cmdlist_start_config = false;
}

bool dpu_dpp_cmdlist_active(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return false;
	}

	if (dpufd->dpp_cmdlist_none_update_frame < DPP_CMDLIST_NONE_UPDATE_LIMIT)
		return true;

	return false;
}

