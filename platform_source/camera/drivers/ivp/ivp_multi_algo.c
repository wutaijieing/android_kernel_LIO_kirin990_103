/*
 * multi-algorithm memory management for ivp
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "ivp_multi_algo.h"
#include <linux/slab.h>
#include "securec.h"
#include "ivp.h"
#include "ivp_log.h"
#include "ivp_ioctl.h"

/* record for algo nodes */
bool algo_ddr_record[ALGO_DDR_RECORD_MAX];
bool algo_core0_iram_record[ALGO_IRAM_RECORD_MAX];
bool algo_core1_iram_record[ALGO_IRAM_RECORD_MAX];
bool algo_core0_dram_data_record[ALGO_DRAM_DATA_RECORD_MAX];
bool algo_core1_dram_data_record[ALGO_DRAM_DATA_RECORD_MAX];
bool algo_core0_dram_bss_record[ALGO_DRAM_BSS_RECORD_MAX];
bool algo_core1_dram_bss_record[ALGO_DRAM_BSS_RECORD_MAX];

/* two cores use same ddr-zone for text sections */
bool dyn_core_loaded[2];

void init_algo_mem_info(struct ivp_device *pdev,
		const unsigned int algo_id)
{
	if (!pdev || (algo_id >= IVP_ALGO_NODE_MAX)) {
		ivp_err("invalid input param pdev or algo_id");
		return;
	}
	pdev->algo_mem_info[algo_id].algo_start_addr = 0;
	pdev->algo_mem_info[algo_id].algo_rel_addr   = 0;
	pdev->algo_mem_info[algo_id].algo_rel_count  = 0;
	pdev->algo_mem_info[algo_id].algo_ddr_addr   = 0;
	pdev->algo_mem_info[algo_id].algo_ddr_vaddr  = 0;
	pdev->algo_mem_info[algo_id].algo_ddr_size   = 0;
	pdev->algo_mem_info[algo_id].algo_func_addr  = 0;
	pdev->algo_mem_info[algo_id].algo_func_vaddr = 0;
	pdev->algo_mem_info[algo_id].algo_func_size  = 0;
	pdev->algo_mem_info[algo_id].algo_data_addr  = 0;
	pdev->algo_mem_info[algo_id].algo_data_vaddr = 0;
	pdev->algo_mem_info[algo_id].algo_data_size  = 0;
	pdev->algo_mem_info[algo_id].algo_bss_addr   = 0;
	pdev->algo_mem_info[algo_id].algo_bss_vaddr  = 0;
	pdev->algo_mem_info[algo_id].algo_bss_size   = 0;
}

int ivp_algo_mem_init(struct ivp_device *pdev)
{
	int i;

	loge_and_return_if_cond(-EINVAL, !pdev,
		"invalid input param pdev");
	pdev->core_status = FREE;
	pdev->algo_mem_info = (struct ivp_algo_mem_info *)kzalloc(
		sizeof(struct ivp_algo_mem_info) * IVP_ALGO_NODE_MAX, GFP_KERNEL);
	if (!pdev->algo_mem_info) {
		ivp_err("kmalloc ivp algo_mem fail");
		return -ENOMEM;
	}

	for (i = 0; i < IVP_ALGO_NODE_MAX; i++) {
		pdev->algo_mem_info[i].occupied = FREE;
		init_algo_mem_info(pdev, i);
	}
	return EOK;
}

static int ivp_base_record_info(const struct ivp_device *pdev,
		const unsigned int segment_type, struct record_node *node)
{
	switch (segment_type) {
	case SEGMENT_DDR_TEXT:
		node->mem_pool = algo_ddr_record;
		node->pool_size = ALGO_DDR_RECORD_MAX;
		node->node_size = ALGO_DDR_NODE_SIZE;
		break;

	case SEGMENT_IRAM_TEXT:
		if (pdev->core_id == IVP_CORE0_ID)
			node->mem_pool = algo_core0_iram_record;
		else
			node->mem_pool = algo_core1_iram_record;
		node->pool_size = ALGO_IRAM_RECORD_MAX;
		node->node_size = ALGO_MEM_NODE_SIZE;
		break;

	case SEGMENT_DRAM0_DATA:
		if (pdev->core_id == IVP_CORE0_ID)
			node->mem_pool = algo_core0_dram_data_record;
		else
			node->mem_pool = algo_core1_dram_data_record;
		node->pool_size = ALGO_DRAM_DATA_RECORD_MAX;
		node->node_size = ALGO_MEM_NODE_SIZE;
		break;

	case SEGMENT_DRAM0_BSS:
		if (pdev->core_id == IVP_CORE0_ID)
			node->mem_pool = algo_core0_dram_bss_record;
		else
			node->mem_pool = algo_core1_dram_bss_record;
		node->pool_size = ALGO_DRAM_BSS_RECORD_MAX;
		node->node_size = ALGO_MEM_NODE_SIZE;
		break;

	default:
		ivp_err("invalid segment type");
		return -EINVAL;
	}

	return 0;
}

static int ivp_alloc_record_info(const struct ivp_device *pdev,
		const unsigned int algo_id, const unsigned int segment_type,
		struct record_node *node)
{
	if (ivp_base_record_info(pdev, segment_type, node) != 0)
		return -EINVAL;

	if (segment_type == SEGMENT_DDR_TEXT) {
		node->size_zone = pdev->algo_mem_info[algo_id].algo_ddr_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_ddr_size % node->node_size != 0)
			node->size_zone += 1;
	} else if (segment_type == SEGMENT_IRAM_TEXT) {
		node->size_zone = pdev->algo_mem_info[algo_id].algo_func_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_func_size % node->node_size != 0)
			node->size_zone += 1;
	} else if (segment_type == SEGMENT_DRAM0_DATA) {
		node->size_zone = pdev->algo_mem_info[algo_id].algo_data_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_data_size % node->node_size != 0)
			node->size_zone += 1;
	} else if (segment_type == SEGMENT_DRAM0_BSS) {
		node->size_zone = pdev->algo_mem_info[algo_id].algo_bss_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_bss_size % node->node_size != 0)
			node->size_zone += 1;
	}
	return 0;
}

static int ivp_free_record_info(struct ivp_device *pdev,
		const unsigned int algo_id, const unsigned int segment_type,
		struct record_node *node)
{
	if (ivp_base_record_info(pdev, segment_type, node) != 0)
		return -EINVAL;

	if (segment_type == SEGMENT_DDR_TEXT) {
		node->start_zone = (pdev->algo_mem_info[algo_id].algo_ddr_addr -
			(pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[BASE_SECT_INDEX].len)) /
			node->node_size;
		node->size_zone = pdev->algo_mem_info[algo_id].algo_ddr_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_ddr_size % node->node_size != 0)
			node->size_zone += 1;
		pdev->algo_mem_info[algo_id].algo_ddr_addr = 0;
	} else if (segment_type == SEGMENT_IRAM_TEXT) {
		node->start_zone = (pdev->algo_mem_info[algo_id].algo_func_addr -
			IVP_IRAM_TEXT_SEGMENT_ADDR) / node->node_size;
		node->size_zone = pdev->algo_mem_info[algo_id].algo_func_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_func_size % node->node_size != 0)
			node->size_zone += 1;
		pdev->algo_mem_info[algo_id].algo_func_addr = 0;
	} else if (segment_type == SEGMENT_DRAM0_DATA) {
		node->start_zone = (pdev->algo_mem_info[algo_id].algo_data_addr -
			IVP_DRAM0_DATA_SEGMENT_ADDR) / node->node_size;
		node->size_zone = pdev->algo_mem_info[algo_id].algo_data_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_data_size % node->node_size != 0)
			node->size_zone += 1;
		pdev->algo_mem_info[algo_id].algo_data_addr = 0;
	} else if (segment_type == SEGMENT_DRAM0_BSS) {
		node->start_zone = (pdev->algo_mem_info[algo_id].algo_bss_addr -
			IVP_DRAM0_BSS_SEGMENT_ADDR) / node->node_size;
		node->size_zone = pdev->algo_mem_info[algo_id].algo_bss_size /
			node->node_size;
		if (pdev->algo_mem_info[algo_id].algo_bss_size % node->node_size != 0)
			node->size_zone += 1;
		pdev->algo_mem_info[algo_id].algo_bss_addr = 0;
	}
	return 0;
}

static int ivp_alloc_algo_node(struct ivp_device *pdev,
		const unsigned int algo_id, const unsigned int segment_type)
{
	unsigned int best;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int k;
	struct record_node node;
	(void)memset_s(&node, sizeof(node), 0, sizeof(node));

	if (ivp_alloc_record_info(pdev, algo_id, segment_type, &node) != 0) {
		ivp_err("get alloc record info failed");
		return -EINVAL;
	}
	best = node.pool_size + 1;
	while (i < node.pool_size) {
		while (j < node.pool_size && node.mem_pool[j]) {
			i++;
			j++;
		}
		while (j < node.pool_size && !node.mem_pool[j])
			j++;
		if ((j - i >= node.size_zone) && (j - i < best)) {
			best = j - i;
			node.start_zone = i;
		}
		i = j;
	}

	if (best <= node.pool_size) {
		for (k = node.start_zone; k < node.start_zone + node.size_zone; k++)
			node.mem_pool[k] = true;
		if (segment_type == SEGMENT_DDR_TEXT)
			pdev->algo_mem_info[algo_id].algo_ddr_addr =
				(pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr +
				pdev->ivp_comm.sects[BASE_SECT_INDEX].len) +
				node.start_zone * node.node_size;
		else if (segment_type == SEGMENT_IRAM_TEXT)
			pdev->algo_mem_info[algo_id].algo_func_addr =
				IVP_IRAM_TEXT_SEGMENT_ADDR + node.start_zone * node.node_size;
		else if (segment_type == SEGMENT_DRAM0_DATA)
			pdev->algo_mem_info[algo_id].algo_data_addr =
				IVP_DRAM0_DATA_SEGMENT_ADDR + node.start_zone * node.node_size;
		else if (segment_type == SEGMENT_DRAM0_BSS)
			pdev->algo_mem_info[algo_id].algo_bss_addr =
				IVP_DRAM0_BSS_SEGMENT_ADDR + node.start_zone * node.node_size;
		ivp_info("core[%u]-algo[%u] alloc memory pool_size[%u] node_size[%u] \
			start_zone[%u] size_zone[%u]", pdev->core_id, algo_id,
			node.pool_size, node.node_size, node.start_zone, node.size_zone);
		return 0;
	}
	return -ENOMEM;
}

int ivp_free_algo_zone(struct ivp_device *pdev,
		unsigned int algo_id, unsigned int segment_type)
{
	unsigned int k;
	struct record_node node;
	loge_and_return_if_cond(-EINVAL, (!pdev || (algo_id >= IVP_ALGO_NODE_MAX)),
		"invalid input param pdev or algo_id");
	if (ivp_free_record_info(pdev, algo_id, segment_type, &node) != 0) {
		ivp_err("get free record info failed");
		return -EINVAL;
	}

	if (node.pool_size <= node.start_zone + node.size_zone) {
		ivp_warn("invalid start[%x] size[%x]", node.start_zone, node.size_zone);
		return -EINVAL;
	}

	for (k = node.start_zone; k < node.start_zone + node.size_zone; k++)
		node.mem_pool[k] = false;
	ivp_info("core[%u]-algo[%u] free memory pool_size[%u] node_size[%u] \
		start_zone[%u] size_zone[%u]", pdev->core_id, algo_id,
		node.pool_size, node.node_size, node.start_zone, node.size_zone);

	return 0;
}

int ivp_alloc_algo_mem(struct ivp_device *pdev,
		const XT_Elf32_Phdr header, const unsigned int algo_id)
{
	loge_and_return_if_cond(-EINVAL, (!pdev || (algo_id >= IVP_ALGO_NODE_MAX)),
		"invalid input param pdev or algo_id");
	if (header.p_vaddr < IVP_ALGO_MAX_SIZE) {
		pdev->algo_mem_info[algo_id].algo_ddr_vaddr = header.p_vaddr;
		pdev->algo_mem_info[algo_id].algo_ddr_size = header.p_memsz;
		if (ivp_alloc_algo_node(pdev, algo_id, SEGMENT_DDR_TEXT) != 0)
			goto err_ivp_alloc_mem;
	} else if (header.p_vaddr == IVP_IRAM_TEXT_SEGMENT_ADDR) {
		pdev->algo_mem_info[algo_id].algo_func_vaddr = header.p_vaddr;
		pdev->algo_mem_info[algo_id].algo_func_size = header.p_memsz;
		if (ivp_alloc_algo_node(pdev, algo_id, SEGMENT_IRAM_TEXT) != 0)
			goto err_ivp_alloc_mem;
	} else if (header.p_vaddr == IVP_DRAM0_DATA_SEGMENT_ADDR) {
		pdev->algo_mem_info[algo_id].algo_data_vaddr = header.p_vaddr;
		pdev->algo_mem_info[algo_id].algo_data_size = header.p_memsz;
		if (ivp_alloc_algo_node(pdev, algo_id, SEGMENT_DRAM0_DATA) != 0)
			goto err_ivp_alloc_mem;
	} else if (header.p_vaddr == IVP_DRAM0_BSS_SEGMENT_ADDR) {
		pdev->algo_mem_info[algo_id].algo_bss_vaddr = header.p_vaddr;
		pdev->algo_mem_info[algo_id].algo_bss_size = header.p_memsz;
		if (ivp_alloc_algo_node(pdev, algo_id, SEGMENT_DRAM0_BSS) != 0)
			goto err_ivp_alloc_mem;
	} else {
		ivp_err("invalid segment address");
		goto err_ivp_alloc_mem;
	}
	return 0;

err_ivp_alloc_mem:
	ivp_err("alloc memory fail");
	if (pdev->algo_mem_info[algo_id].algo_ddr_addr != 0)
		ivp_free_algo_zone(pdev, algo_id, SEGMENT_DDR_TEXT);
	if (pdev->algo_mem_info[algo_id].algo_func_addr != 0)
		ivp_free_algo_zone(pdev, algo_id, SEGMENT_IRAM_TEXT);
	if (pdev->algo_mem_info[algo_id].algo_data_addr != 0)
		ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_DATA);
	if (pdev->algo_mem_info[algo_id].algo_bss_addr != 0)
		ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_BSS);
	return -EINVAL;
}