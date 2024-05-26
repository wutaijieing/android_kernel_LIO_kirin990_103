/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * hongmeng kev&kbox log functions moudle
 *
 * hm_klog_process.c
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
#include "rdr_inner.h"
#include "rdr_print.h"
#include "rdr_field.h"
#include <platform_include/basicplatform/linux/util.h>
#include <securec.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/types.h>

#define HM_KLOG_BIN        "hm_klog.bin"
#define DTS_PATH_HM_KLOG   "/reserved-memory/hm_klog"
/* Define BOOL data type */
#define BOOL              _Bool
#define TRUE              1
#define FALSE             0

static u32	 g_resvd_mem_phy_addr = 0;
static u32	 g_resvd_mem_phy_size = 0;
static BOOL	 g_resvd_mem_save_flag = FALSE;

static int hm_save_last_log(const char *logpath, const void *buf, u32 len)
{
	BB_PRINT_START();
	if (logpath == NULL || buf == NULL || len > g_resvd_mem_phy_size) {
		BB_PRINT_ERR("%s(): args is error\n", __func__);
		return -1;
	}
	/* save hongmeng kev&kbox log to fs */
	(void)rdr_savebuf2fs(logpath, HM_KLOG_BIN, buf, len, 0);

	BB_PRINT_END();
	return 0;
}

static int is_hmklog_addr_size_valid(phys_addr_t paddr, size_t size)
{
	if((paddr < RESERVED_RDA_LOG_BACKUP_PHYMEM_BASE) || !size)
		return -1;
	if((paddr + size) < paddr)
		return -1;
	if((paddr + size) > (RESERVED_RDA_LOG_BACKUP_PHYMEM_BASE + RESERVED_RDA_LOG_BACKUP_PHYMEM_SIZE))
		return -1;
	return 0;
}
static void *hmklog_dfx_map(phys_addr_t paddr, size_t size)
{
	void *vaddr = NULL;
	int  ret;

	ret = is_hmklog_addr_size_valid(paddr, size);
	if (ret) {
		BB_PRINT_ERR("%s():Error BBox memory,ret =%d\n", __func__, ret);
		return NULL;
	}

	if (pfn_valid(RESERVED_RDA_LOG_BACKUP_PHYMEM_BASE >> PAGE_SHIFT)) {
		vaddr = bbox_vmap(paddr, size);
	} else {
		vaddr = ioremap_wc(paddr, size);
	}
	return vaddr;
}

static void hmklog_dfx_unmap(const void *vaddr)
{
	if (vaddr == NULL)
		return;
	if (pfn_valid(RESERVED_RDA_LOG_BACKUP_PHYMEM_BASE >> PAGE_SHIFT))
		vunmap(vaddr);
	else
		iounmap((void __iomem *)vaddr);
}

static int get_hmklog_rsvd_mem_dts(void)
{
	struct device_node *np = NULL;
	int ret;

	np = of_find_node_by_path(DTS_PATH_HM_KLOG);
	if (!np) {
		BB_PRINT_ERR("NOT FOUND dts path: %s!\n", DTS_PATH_HM_KLOG);
		return -1;
	}

	ret = of_property_read_u32_index(np, "reg", 1, &g_resvd_mem_phy_addr);
	if (ret) {
		BB_PRINT_ERR("failed to get g_resvd_mem_phy_addr resource! ret=%d\n", ret);
		return -1;
	}
	ret = of_property_read_u32_index(np, "reg", 3, &g_resvd_mem_phy_size);
	if (ret) {
		BB_PRINT_ERR("failed to get g_resvd_mem_phy_size resource! ret=%d\n", ret);
		return -1;
	}
	return 0;
}

int release_hmklog_reserved_memory(void)
{
	phys_addr_t addr;
	struct page *page = NULL;

	BB_PRINT_START();
	if (g_resvd_mem_save_flag) {
		BB_PRINT_ERR("%s(): hongmeng kev&kbox log backup reserved memory has been released!\n", __func__);
		return 0;
	}

	if ((g_resvd_mem_phy_addr == 0) || (g_resvd_mem_phy_size == 0)) {
		if (get_hmklog_rsvd_mem_dts()) {
			BB_PRINT_ERR("%s():can't find node int dts\n", __func__);
			return -1;
		}
	}

	for (addr = g_resvd_mem_phy_addr; addr < (g_resvd_mem_phy_addr + g_resvd_mem_phy_size); addr += PAGE_SIZE) {
		page = pfn_to_page(addr >> PAGE_SHIFT);
		if (PageReserved(page))
			free_reserved_page(page);
		else
			BB_PRINT_ERR("%s():page is not reserved\n", __func__);
#ifdef CONFIG_HIGHMEM
		if (PageHighMem(page))
			totalhigh_pages++;
#endif
	}

	memblock_free(g_resvd_mem_phy_addr, g_resvd_mem_phy_size);
	g_resvd_mem_save_flag = TRUE;
	BB_PRINT_END();
	return 0;
}

int save_hm_klog(char *path, u32 path_len)
{
	u32 reboot_type;
	int ret = 0;
	struct rdr_struct_s *g_hmklog_pbb = NULL;

	BB_PRINT_START();
	if (path == NULL) {
		BB_PRINT_ERR("%s():path is null", __func__);
		return -1;
	}

	if (path_len < PATH_MAXLEN) {
		BB_PRINT_ERR("%s(): path_len is invalid\n", __func__);
		return -1;
	}

	reboot_type = rdr_get_reboot_type();
	/* If the exception type is normal, do not need to save log */
	if (reboot_type < REBOOT_REASON_LABEL1 || (reboot_type >= REBOOT_REASON_LABEL4 &&
		reboot_type < REBOOT_REASON_LABEL5)) {
		BB_PRINT_ERR("%s():need not save dump file when boot,reboot_type = [0x%x]\n", __func__, reboot_type);
		return 0;
	}

	ret = get_hmklog_rsvd_mem_dts();
	if (ret) {
		BB_PRINT_ERR("%s():can't find node int dts, ret = %d\n", __func__, ret);
		return -1;
	}

	g_hmklog_pbb = hmklog_dfx_map(g_resvd_mem_phy_addr, g_resvd_mem_phy_size);
	if(g_hmklog_pbb == NULL) {
		BB_PRINT_ERR("hmklog_dfx_map g_hmklog_pbb faild\n");
		return -1;
	}

	ret = hm_save_last_log(path, (void *)g_hmklog_pbb, g_resvd_mem_phy_size);
	if(ret)
		BB_PRINT_ERR("%s():save hongmeng kev and kbox log fail,ret = %d\n", __func__, ret);

	hmklog_dfx_unmap(g_hmklog_pbb);
	g_hmklog_pbb = NULL;

	BB_PRINT_END();
	return ret;
}
