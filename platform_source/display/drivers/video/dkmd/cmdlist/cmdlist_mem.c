/**
 * @file cmdlist_mem.c
 * @brief Cmdlist memory management interface
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
#include "dkmd_log.h"

#include "chash.h"
#include "cmdlist_node.h"
#include "cmdlist_client.h"
#include "cmdlist_dev.h"
#include "cmdlist_drv.h"
#include "dkmd_acquire_fence.h"

void *cmdlist_mem_alloc(uint32_t size, dma_addr_t *phy_addr, uint32_t *out_buffer_size)
{
	void *vir_addr = NULL;
	uint32_t alloc_size;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	if (!phy_addr || !out_buffer_size) {
		dpu_pr_err("check input param error!\n");
		return NULL;
	}
	*out_buffer_size = 0;

	if (IS_ERR_OR_NULL(cmd_priv->memory_pool)) {
		dpu_pr_err("check memory_pool failed!\n");
		return NULL;
	}

	alloc_size = roundup(size, PAGE_SIZE / 4);
	vir_addr = (void *)(uintptr_t)gen_pool_dma_alloc(cmd_priv->memory_pool, alloc_size, phy_addr);
	if (!vir_addr) {
		dpu_pr_err("gen pool alloc failed, size:%#x!\n", alloc_size);
		return NULL;
	}
	memset(vir_addr, 0, alloc_size);
	*out_buffer_size = alloc_size;
	dpu_pr_debug("mem_alloc --> vir_addr:%#x, size:%#x\n", vir_addr, alloc_size);

	return vir_addr;
}

void cmdlist_mem_free(void *vir_addr, uint32_t size)
{
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	if (IS_ERR_OR_NULL(cmd_priv->memory_pool)) {
		dpu_pr_err("check memory_pool failed!\n");
		return;
	}
	dpu_pr_debug("mem_free --> vir_addr:%#x, size:%#x\n", vir_addr, size);
	gen_pool_free(cmd_priv->memory_pool, (unsigned long)(uintptr_t)vir_addr, size);
	vir_addr = NULL;
}
