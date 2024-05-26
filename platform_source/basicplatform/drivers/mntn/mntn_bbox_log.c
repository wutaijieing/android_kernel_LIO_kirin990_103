/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * bbox log functions moudle
 *
 * mntn_bbox_log.c
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
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/io.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include "blackbox/rdr_print.h"

static u32 reserved_bbox_log_phymem_addr;
static u32 reserved_bbox_log_phymem_size;
static void *g_bboxlog_vddr = NULL;
#define DTS_PATH_MNTN_BBOX_LOG    "/reserved-memory/bbox_log"

static int get_bbox_log_dts_config(void)
{
	struct device_node *np = NULL;
	int ret;

	np = of_find_node_by_path(DTS_PATH_MNTN_BBOX_LOG);
	if (!np) {
		BB_PRINT_ERR("NOT FOUND dts path: %s!\n", DTS_PATH_MNTN_BBOX_LOG);
		return -1;
	}

	ret = of_property_read_u32_index(np, "reg", 1, &reserved_bbox_log_phymem_addr);
	if (ret) {
		BB_PRINT_ERR("failed to get reserved_bbox_log_phymem_addr resource! ret=%d\n", ret);
		return -1;
	}
	ret = of_property_read_u32_index(np, "reg", 3, &reserved_bbox_log_phymem_size);
	if (ret) {
		BB_PRINT_ERR("failed to get reserved_bbox_log_phymem_size resource! ret=%d\n", ret);
		return -1;
	}
	return 0;
}

static void *bbox_addr_map(phys_addr_t paddr, size_t size, phys_addr_t max_paddr, size_t max_size)
{
	void *vaddr = NULL;

	if (paddr < max_paddr || !size || ((paddr + size) < paddr) ||
	   (paddr + size) > (max_paddr + max_size)) {
		BB_PRINT_ERR("Error HM BBox memory\n");
		return NULL;
	}

	if (pfn_valid(max_paddr >> PAGE_SHIFT))
		vaddr = bbox_vmap(paddr, size);
	else
		vaddr = ioremap_wc(paddr, size);
	return vaddr;
}

static void bbox_addr_unmap(const void *vaddr, phys_addr_t max_paddr)
{
	if (vaddr == NULL)
		return;
	if (pfn_valid(max_paddr >> PAGE_SHIFT))
		vunmap(vaddr);
	else
		iounmap((void __iomem *)vaddr);
}

static void release_reserved_memory(phys_addr_t paddr, size_t size)
{
	phys_addr_t addr;
	struct page *page = NULL;

	BB_PRINT_START();

	if ((paddr == 0) || (size == 0)) {
		if (get_bbox_log_dts_config()) {
			BB_PRINT_ERR("%s(): can't find node int dts\n", __func__);
			return;
		}
	}

	for (addr = paddr; addr < (paddr + size); addr += PAGE_SIZE) {
		page = pfn_to_page(addr >> PAGE_SHIFT);
		if (PageReserved(page))
			free_reserved_page(page);
		else
			BB_PRINT_ERR("%s(): page is not reserved\n", __func__);
#ifdef CONFIG_HIGHMEM
		if (PageHighMem(page))
			totalhigh_pages++;
#endif
	}

	memblock_free(paddr, size);
	BB_PRINT_END();
	return;
}

void *get_bbox_log_vaddr(void)
{
	if (reserved_bbox_log_phymem_addr == 0 || reserved_bbox_log_phymem_size == 0) {
		if (get_bbox_log_dts_config())
			return NULL;
	}

	g_bboxlog_vddr = bbox_addr_map(reserved_bbox_log_phymem_addr, reserved_bbox_log_phymem_size,
			reserved_bbox_log_phymem_addr, reserved_bbox_log_phymem_size);
	return g_bboxlog_vddr;
}

void release_bbox_log_memory(void)
{
	if(g_bboxlog_vddr) {
		bbox_addr_unmap(g_bboxlog_vddr, reserved_bbox_log_phymem_addr);
		release_reserved_memory(reserved_bbox_log_phymem_addr, reserved_bbox_log_phymem_size);
		g_bboxlog_vddr = NULL;
	}
}

static int __init bbox_log_init(void)
{
	int ret;
	ret = get_bbox_log_dts_config();
	if (ret)
		BB_PRINT_ERR("%s(): get bbox_log config failed!\n", __func__);

	return ret;
}

pure_initcall(bbox_log_init);