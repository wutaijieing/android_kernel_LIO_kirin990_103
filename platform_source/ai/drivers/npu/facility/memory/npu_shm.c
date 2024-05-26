/*
 * npu_shm.c
 *
 * about npu shm
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "npu_shm.h"

#include <linux/idr.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <asm/uaccess.h>
#include <linux/gfp.h>
#include <asm/tlbflush.h>
#include <asm/io.h>

#include "mm_svm.h"

#define L2_MAP_ALIGN         2

struct npu_mem_desc *g_sq_desc;
struct npu_mem_info g_shm_desc[NPU_DEV_NUM][NPU_MAX_MEM];

phys_addr_t get_shm_phy_addr(u8 devid, u32 index)
{
	if (devid >= NPU_DEV_NUM || index >= NPU_MAX_MEM) {
		npu_drv_err("invalid id or mem_index %u", index);
		return 0;
	}
	return g_shm_desc[devid][index].phy_addr;
}

vir_addr_t get_shm_virt_addr(u8 devid, u32 index)
{
	if (devid >= NPU_DEV_NUM || index >= NPU_MAX_MEM) {
		npu_drv_err("invalid id or mem_index %u", index);
		return 0;
	}
	return g_shm_desc[devid][index].virt_addr;
}

struct npu_ts_sq_info *npu_calc_sq_info(u8 devid, u32 index)
{
	struct npu_ts_sq_info *sq = NULL;
	u64 addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;

	if (addr == 0)
		return NULL;

	if (index >= NPU_MAX_SQ_NUM)
		return NULL;

	sq = (struct npu_ts_sq_info *)(uintptr_t) (addr +
		(unsigned long)sizeof(struct npu_ts_sq_info) * index);

	return sq;
}

struct npu_ts_cq_info *npu_calc_cq_info(u8 devid, u32 index)
{
	struct npu_ts_cq_info *cq = NULL;
	u64 addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;

	if (addr == 0)
		return NULL;

	if (index >= NPU_MAX_CQ_NUM)
		return NULL;

	cq = (struct npu_ts_cq_info *)(uintptr_t) (addr +
		NPU_SQ_INFO_OCCUPY_SIZE +
		(unsigned long)sizeof(struct npu_ts_cq_info) * index);

	return cq;
}

struct npu_stream_info *npu_calc_stream_info(u8 devid, u32 index)
{
	struct npu_stream_info *stream_info = NULL;
	u64 addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;

	if (index >= NPU_MAX_STREAM_ID)
		return NULL;

	stream_info = (struct npu_stream_info *)(uintptr_t) (addr +
		NPU_SQ_INFO_OCCUPY_SIZE + NPU_CQ_INFO_OCCUPY_SIZE +
		(unsigned long)sizeof(struct npu_stream_info) * index);
	return stream_info;
}

int npu_shm_init(u8 dev_id)
{
	gfp_t gfp_flags = GFP_KERNEL | __GFP_COMP | __GFP_ZERO;
	char *tmp = NULL;
	struct npu_mem_desc *persistent_task_buf_desc = NULL;
	struct npu_platform_info *plat_info = NULL;
	u32 order = 0;
	int ret = 0;

	npu_drv_debug("dev_id = %u\n", dev_id);
	cond_return_error(dev_id >= NPU_DEV_NUM, -1,
		"illegal npu dev id = %d\n", dev_id);

	plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -1, "get plat info failed\n");

	order = (u32)NPU_MAX_INFO_ORDER;
	g_sq_desc = plat_info->resmem_info.sqcq_buf;
	tmp = (char *)(uintptr_t) __get_free_pages(gfp_flags, order);
	cond_return_error(tmp == NULL, -ENOMEM,
		"alloc share mem descriptor memory failed !\n");

	g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr = g_sq_desc->base +
		CHIP_BASEADDR_PA_OFFSET * dev_id;
	g_shm_desc[dev_id][NPU_SQ_MEM].size = g_sq_desc->len;

	g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr = virt_to_phys(tmp);
	g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = (vir_addr_t)(uintptr_t)tmp;
	g_shm_desc[dev_id][NPU_INFO_MEM].size = (1UL << order) * PAGE_SIZE;
#ifndef CONFIG_NPU_SWTS
	persistent_task_buf_desc = plat_info->resmem_info.persistent_task_buf;
	g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].phy_addr =
		persistent_task_buf_desc->base;
	g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].size =
		persistent_task_buf_desc->len;
	g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr = (vir_addr_t)(uintptr_t)ioremap_wc(
		g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].phy_addr,
		g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].size);
	cond_goto_error(g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr == 0,
		persistent_task_buff_init_failed, ret, -ENOMEM, "ioremap_wc failed!");
#endif
	gfp_flags = GFP_KERNEL | __GFP_ZERO;
	tmp = (char *)(uintptr_t)__get_free_pages(gfp_flags, 0);
	cond_goto_error(tmp == NULL,
		pad_mem_init_failed, ret, -ENOMEM, "alloc share mem pad memory failed\n");

	g_shm_desc[dev_id][NPU_PAD_MEM].phy_addr = virt_to_phys(tmp);
	g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr = (vir_addr_t)(uintptr_t)tmp;
	g_shm_desc[dev_id][NPU_PAD_MEM].size = PAGE_SIZE;

	npu_drv_debug("sq mem: phy_addr = 0x%llx, size = %lu\n",
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SQ_MEM].size);

	npu_drv_debug("info mem: virt_addr = 0x%llx, order = %u, size = %lu\n",
		g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr,
		order, g_shm_desc[dev_id][NPU_INFO_MEM].size);

	return 0;

pad_mem_init_failed:
	iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr);
	g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr = 0;
	g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].phy_addr = 0;
persistent_task_buff_init_failed:
	free_pages((unsigned long)g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr, order);
	g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = 0;
	g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr = 0;

	return ret;
}

void npu_shm_destroy(u8 dev_id)
{
	unsigned long addr;
	u32 order;

	if (dev_id >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id %d\n", dev_id);
		return;
	}

	order = (u32)NPU_MAX_INFO_ORDER;
	addr = (unsigned long)g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr;
	if (addr != 0) {
		free_pages(addr, order);
		g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = 0;
	}

	addr = (unsigned long)g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr;
	if (addr != 0) {
		free_pages(addr, 0);
		g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr = 0;
	}

	if (g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr != 0) {
		iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr);
		g_shm_desc[dev_id][NPU_PERSISTENT_TASK_BUFF].virt_addr = 0;
	}
}

struct npu_hwts_sq_info *npu_calc_hwts_sq_info(u8 devid, u32 index)
{
	struct npu_hwts_sq_info *hwts_sq_info = NULL;
	u64 addr;

	if (index >= NPU_MAX_HWTS_SQ_INFO_NUM) {
		npu_drv_err("illegal index=%u\n", index);
		return NULL;
	}

	addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;
	hwts_sq_info = (struct npu_hwts_sq_info *)(uintptr_t)(addr +
		NPU_HWTS_SQ_INFO_OFFSET +
		(unsigned long)sizeof(struct npu_hwts_sq_info) * index);

	return hwts_sq_info;
}

struct npu_hwts_cq_info *npu_calc_hwts_cq_info(u8 devid, u32 index)
{
	struct npu_hwts_cq_info *hwts_cq_info = NULL;
	u64 addr;

	if (index >= NPU_MAX_HWTS_CQ_INFO_NUM) {
		npu_drv_err("illegal index=%u\n", index);
		return NULL;
	}

	addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;
	hwts_cq_info = (struct npu_hwts_cq_info *)(uintptr_t)(addr +
		NPU_HWTS_CQ_INFO_OFFSET +
		(unsigned long)sizeof(struct npu_hwts_cq_info) * index);

	return hwts_cq_info;
}

struct npu_model_desc_info *npu_calc_model_desc_info(u8 devid, u32 index)
{
	struct npu_model_desc_info *model_desc_info = NULL;
	u64 addr;

	if (index >= NPU_MAX_MODEL_ID) {
		npu_drv_err("illegal model_id= %u\n", index);
		return NULL;
	}

	addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;
	model_desc_info = (struct npu_model_desc_info *)(uintptr_t)(addr +
		NPU_MODEL_INFO_OFFSET +
		(unsigned long)sizeof(struct npu_model_desc_info) * index);

	npu_drv_debug("npu devid= %u, model_id= %u, index= %u, addr= %pK\n",
		devid, index, index, model_desc_info);
	return model_desc_info;
}

struct npu_prof_info *npu_calc_profiling_info(u8 devid)
{
	u64 addr;
	struct npu_prof_info *profiling_info = NULL;

	addr = g_shm_desc[devid][NPU_INFO_MEM].virt_addr;
	profiling_info = (struct npu_prof_info *)(uintptr_t)(addr +
		NPU_PROFILINGL_INFO_OFFSET);

	return profiling_info;
}

int npu_shm_v200_init_sq_mem(u8 dev_id)
{
	vir_addr_t virt_addr;

	g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr = g_sq_desc->base +
		CHIP_BASEADDR_PA_OFFSET * dev_id;
	g_shm_desc[dev_id][NPU_SQ_MEM].size = g_sq_desc->len;
	virt_addr = (vir_addr_t)(uintptr_t)ioremap_wc(
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SQ_MEM].size);
	if (virt_addr == 0) {
		npu_drv_err("ioremap_wc failed!");
		return -1;
	}
	g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr = virt_addr;

	npu_drv_debug("sqcq mem: phy_addr = 0x%llx, size = %lu, virt_addr = 0x%llx, "
		"sq phy_addr = 0x%llx, cq phy_addr = 0x%llx, "
		"dfx sq phy_addr = 0x%llx, dfx cq phy_addr = 0x%llx\n",
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SQ_MEM].size,
		g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr,
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr + NPU_SQ_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr + NPU_SQ_OCCUPY_SIZE +
			NPU_CQ_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_SQ_MEM].phy_addr + NPU_SQ_OCCUPY_SIZE +
			NPU_CQ_OCCUPY_SIZE + NPU_DFX_SQ_OCCUPY_SIZE);

	return 0;
}

#ifdef CONFIG_NPU_PHY_MEM
int npu_shm_v200_init_swap_buff(u8 dev_id)
{
	vir_addr_t virt_addr;
	struct npu_platform_info *plat_info = NULL;
	struct npu_mem_desc *hwts_swap_buf = NULL;

	plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, NULL, "npu_plat_get_info error");

	hwts_swap_buf = plat_info->resmem_info.hwts_swap_buf;
	npu_drv_debug("hwts_swap_buf.base = 0x%llx, hwts_swap_buf.len = 0x%llx\n", hwts_swap_buf->base, hwts_swap_buf->len);

	g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].phy_addr = hwts_swap_buf->base;
	g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].size = hwts_swap_buf->len;
	virt_addr = (vir_addr_t)(uintptr_t)ioremap_wc(
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].size);
	if (virt_addr == 0) {
		npu_drv_err("ioremap_wc failed!");
		return -1;
	}

	g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].virt_addr = virt_addr;
	npu_drv_debug("swap buff mem: phy_addr = 0x%llx, size = %lu, virt_addr = 0x%llx\n",
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].size,
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].virt_addr);

	return 0;
}
#endif

int npu_shm_v200_init_info_mem(struct npu_platform_info *plat_info,
	u8 dev_id)
{
	vir_addr_t virt_addr;

	g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr =
		plat_info->resmem_info.info_buf->base;
	g_shm_desc[dev_id][NPU_INFO_MEM].size =
		plat_info->resmem_info.info_buf->len;
	virt_addr = (vir_addr_t)(uintptr_t)ioremap_wc(
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_INFO_MEM].size);
	g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = virt_addr;
	if (virt_addr == 0) {
		npu_drv_err("ioremap_wc failed!");
		return -1;
	}

	npu_drv_debug("info mem: virt_addr = 0x%llx, size = %lu, "
		"sqinfo phy_addr = 0x%llx, cqinfo phy_addr = 0x%llx, "
		"streaminfo phy_addr = 0x%llx, hwtssqinfo phy_addr = 0x%llx, "
		"hwtscqinfo phy_addr = 0x%llx, modelinfo phy_addr = 0x%llx\n",
		g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr,
		g_shm_desc[dev_id][NPU_INFO_MEM].size,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr +
			NPU_SQ_INFO_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr +
			NPU_SQ_INFO_OCCUPY_SIZE + NPU_CQ_INFO_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr +
			NPU_SQ_INFO_OCCUPY_SIZE +
			NPU_CQ_INFO_OCCUPY_SIZE + NPU_STREAM_INFO_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr +
			NPU_SQ_INFO_OCCUPY_SIZE + NPU_CQ_INFO_OCCUPY_SIZE +
			NPU_STREAM_INFO_OCCUPY_SIZE + NPU_HWTS_SQ_INFO_OCCUPY_SIZE,
		g_shm_desc[dev_id][NPU_INFO_MEM].phy_addr +
			NPU_SQ_INFO_OCCUPY_SIZE +
			NPU_CQ_INFO_OCCUPY_SIZE + NPU_STREAM_INFO_OCCUPY_SIZE +
			NPU_HWTS_SQ_INFO_OCCUPY_SIZE + NPU_HWTS_CQ_INFO_OCCUPY_SIZE);

	return 0;
}

int npu_shm_v200_init_pad_mem(u8 dev_id)
{
	gfp_t gfp_flags;
	char *tmp = NULL;

	gfp_flags = GFP_KERNEL | __GFP_ZERO;
	tmp = (char *)(uintptr_t)__get_free_pages(gfp_flags, 0);
	if (tmp == NULL) {
		npu_drv_err("alloc share mem pad memory failed.n");
		return -ENOMEM;
	}
	g_shm_desc[dev_id][NPU_PAD_MEM].phy_addr = virt_to_phys(tmp);
	g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr = (vir_addr_t)(uintptr_t)tmp;
	g_shm_desc[dev_id][NPU_PAD_MEM].size = PAGE_SIZE;

	npu_drv_debug("pad buf mem: phy_addr = 0x%llx, size = %lu\n",
		g_shm_desc[dev_id][NPU_PAD_MEM].phy_addr,
		g_shm_desc[dev_id][NPU_PAD_MEM].size);

	return 0;
}

int npu_shm_v200_init(u8 dev_id)
{
	struct npu_platform_info *plat_info = NULL;
	int ret;

	npu_drv_debug("dev_id = %u\n", dev_id);
	if (dev_id >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id = %d\n", dev_id);
		return -1;
	}

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat info failed\n");
		return -1;
	}

	g_sq_desc = plat_info->resmem_info.sqcq_buf;

	ret = npu_shm_v200_init_sq_mem(dev_id);
	if (ret != 0)
		return ret;

#ifdef CONFIG_NPU_PHY_MEM
	ret = npu_shm_v200_init_swap_buff(dev_id);
	if (ret != 0)
		return ret;
#endif

	ret = npu_shm_v200_init_info_mem(plat_info, dev_id);
	if (ret != 0) {
		npu_drv_err("init info mem failed!");
		goto init_info_mem_failed;
	}

	ret = npu_shm_v200_init_pad_mem(dev_id);
	if (ret != 0) {
		npu_drv_err("init info pad failed!");
		goto init_pad_mem_failed;
	}

	return 0;

init_pad_mem_failed:
	iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr);
	g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = 0;
init_info_mem_failed:
	iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr);
	g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr = 0;

	return ret;
}

void npu_shm_v200_destroy(u8 dev_id)
{
	unsigned long addr;

	if (dev_id >= NPU_DEV_NUM) {
		npu_drv_err("illegal npu dev id %d\n", dev_id);
		return;
	}

	if (g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr != 0) {
		iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr);
		g_shm_desc[dev_id][NPU_SQ_MEM].virt_addr = 0;
	}

#ifdef CONFIG_NPU_PHY_MEM
	if (g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].virt_addr != 0) {
		iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].virt_addr);
		g_shm_desc[dev_id][NPU_SWAP_BUFF_MEM].virt_addr = 0;
	}
#endif

	if (g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr != 0) {
		iounmap((void *)(
			uintptr_t)g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr);
		g_shm_desc[dev_id][NPU_INFO_MEM].virt_addr = 0;
	}

	addr = (unsigned long)g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr;
	if (addr != 0) {
		free_pages(addr, 0);
		g_shm_desc[dev_id][NPU_PAD_MEM].virt_addr = 0;
	}
}

int npu_devmem_swapin(struct vm_area_struct *vma, u64 devmem_base,
	unsigned long size, unsigned long align_size)
{
	struct mm_struct *mm = NULL;
	unsigned long vma_start_aligned;

	cond_return_error(vma == NULL, -EFAULT, "vma is NULL\n");

	vma_start_aligned = align_up(vma->vm_start, align_size);
	npu_drv_debug(
		"zap_vma_ptes.devmem_base=0x%llx, vma_start_aligned=0x%lx, size=0x%lx, align_size=0x%lx\n",
		devmem_base, vma_start_aligned, size, align_size);
	npu_drv_debug("vm_start = 0x%lx vm_end = 0x%lx vm_flags=0x%lx vm_page_prot=0x%llx",
		vma->vm_start, vma->vm_end, vma->vm_flags,
		(u64)vma->vm_page_prot.pgprot);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	zap_vma_ptes(vma, vma_start_aligned, size);
#else
	int err = zap_vma_ptes(vma, vma_start_aligned, size);
	if (err) {
		npu_drv_err(
			"zap_vma_ptes failed. size=0x%lx,align_size=0x%lx, err=%d\n",
			size, align_size, err);
		npu_drv_debug("vm_start = 0x%lx vm_end = 0x%lx vm_flags=0x%lx vm_page_prot=0x%lx",
			vma->vm_start, vma->vm_end, vma->vm_flags,
			vma->vm_page_prot.pgprot);
		return -EFAULT;
	}
#endif
	cond_return_error(size <= 0, -ENOMEM, " size = %lu\n", size);

	if (remap_pfn_range(vma, vma_start_aligned, __phys_to_pfn(devmem_base),
		size, vma->vm_page_prot)) {
		npu_drv_err("fail to map body mem. align_size=0x%lx\n", align_size);
		return -ENXIO;
	}

	mm = current->mm;
	cond_return_error(mm == NULL, -ENXIO);
	cond_return_error(mm_svm_flush_cache(mm, vma->vm_start, size), -ENXIO);

	return 0;
}

int npu_devmem_swapout(struct vm_area_struct *vma, unsigned long pad_base,
	unsigned long size, unsigned long align_size)
{
	struct mm_struct *mm = NULL;
	unsigned long vma_start_aligned;
	unsigned long pad_start;
	unsigned long vma_tmp;

	vma_start_aligned = align_up(vma->vm_start, align_size);

	npu_drv_debug("vma_start_aligned=0x%lx, size=0x%lx, align_size=0x%lx\n",
		vma_start_aligned, size, align_size);
	npu_drv_debug("vm_start = 0x%lx vm_end = 0x%lx vm_flags=0x%lx vm_page_prot=0x%llx",
		vma->vm_start, vma->vm_end, vma->vm_flags,
		(u64)vma->vm_page_prot.pgprot);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	zap_vma_ptes(vma, vma_start_aligned, size);
#else
	int err = zap_vma_ptes(vma, vma_start_aligned, size);
	if (err) {
		npu_drv_err("zap_vma_ptes failed. size=0x%lx, align_size=0x%lx, err=%d\n",
			size, align_size, err);
		npu_drv_debug("vm_start = 0x%lx vm_end = 0x%lx vm_flags=0x%lx vm_page_prot=0x%lx",
			vma->vm_start, vma->vm_end, vma->vm_flags,
			vma->vm_page_prot.pgprot);
		return -EFAULT;
	}
#endif

	vma_tmp = vma_start_aligned + size;
	cond_return_error(size <= 0, -ENOMEM, " size = %lu\n", size);
	for (pad_start = vma_start_aligned; pad_start < vma_tmp;
		pad_start += PAGE_SIZE) {
		if (remap_pfn_range(vma, pad_start, __phys_to_pfn(pad_base),
			PAGE_SIZE, vma->vm_page_prot)) {
			npu_drv_err("fail to map pad mem\n");
			return -ENXIO;
		}
	}

	mm = current->mm;
	cond_return_error(mm == NULL, -ENXIO);
	cond_return_error(mm_svm_flush_cache(mm, vma->vm_start, size), -ENXIO);

	return 0;
}

static int l2_mem_check_vma(struct vm_area_struct *vma, u64 *base,
	unsigned long *len)
{
#ifndef CONFIG_NPU_SWTS
	struct npu_mem_desc *l2_desc = NULL;
	struct npu_platform_info *plat_info = NULL;
	unsigned long l2_len;
	unsigned long vma_len;

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat_info failed\n");
		return -EFAULT;
	}

	l2_desc = &plat_info->dts_info.reg_desc[NPU_REG_L2BUF_BASE];
	if (l2_desc == NULL) {
		npu_drv_err("plat get reg desc failed\n");
		return -EFAULT;
	}

	if (vma->vm_end < vma->vm_start) {
		npu_drv_err("invalid vma\n");
		return -EFAULT;
	}
	l2_len = l2_desc->len + 1; // becasue of dts will minus 1
	npu_drv_warn("l2_len %lu\n", l2_len);
	vma_len = vma->vm_end - vma->vm_start;
	if (vma_len != l2_len * L2_MAP_ALIGN) {
		npu_drv_err("vma len %lu l2_len %lu l2 map align %d is not match\n",
			vma_len, l2_len, L2_MAP_ALIGN);
		return -EFAULT;
	}
	*base = l2_desc->base;
	*len = l2_len;
#endif
	return 0;
}

int l2_mem_swapin(struct vm_area_struct *vma)
{
	u64 l2_base = 0;
	unsigned long l2_len = 0;
	int err;

	err = l2_mem_check_vma(vma, &l2_base, &l2_len);
	if (err) {
		npu_drv_err("l2 check vma fail err %d\n", err);
		return -1;
	}
	err = npu_devmem_swapin(vma, l2_base, l2_len, l2_len);
	if (err) {
		npu_drv_err("devmem swapin failed. l2_len=0x%lx\n", l2_len);
		return -1;
	}
	npu_drv_warn("exit");
	return 0;
}

int l2_mem_swapout(struct vm_area_struct *vma, u8 dev_id)
{
	u64 l2_base = 0;
	unsigned long l2_len = 0;
	unsigned long pad_base;
	int err;

	npu_drv_debug(" enter");

	pad_base = g_shm_desc[dev_id][NPU_PAD_MEM].phy_addr;
	if (pad_base == 0) {
		npu_drv_err("invalid pad_base\n");
		return -EFAULT;
	}

	err = l2_mem_check_vma(vma, &l2_base, &l2_len);
	if (err) {
		npu_drv_err("l2 check vma fail dev id %u err %d\n", dev_id, err);
		return -1;
	}

	err = npu_devmem_swapout(vma, pad_base, l2_len, l2_len);
	if (err) {
		npu_drv_err("devmem swapout failed. l2_len=0x%lx\n", l2_len);
		return -1;
	}
	npu_drv_warn("exit");
	return 0;
}

int npu_map_l2_buff(const struct file *filp, struct vm_area_struct *vma,
	u8 dev_id)
{
	unsigned long pad_base, pad_start;
	u64 l2_base = 0;
	unsigned long l2_len = 0;
	int err;

	npu_drv_debug("map l2 buff enter");

	if ((vma == NULL) || (filp == NULL) || (dev_id >= NPU_DEV_NUM)) {
		npu_drv_err("invalid para\n");
		return -EFAULT;
	}

	err = l2_mem_check_vma(vma, &l2_base, &l2_len);
	if (err) {
		npu_drv_err("l2 check vma fail dev id %u err %d\n", dev_id, err);
		return -EFAULT;
	}

	npu_drv_debug("vma=%pK vm_mm=%pK vm_next=%pK vm_start=0x%lx, vm_end =0x%lx\n",
		vma, vma->vm_mm, vma->vm_next, vma->vm_start, vma->vm_end);

	/* we do not want to have this area swapped out, lock it */
	vma->vm_flags |= VM_LOCKED;

	pad_base = g_shm_desc[dev_id][NPU_PAD_MEM].phy_addr;
	if (pad_base == 0) {
		npu_drv_err("invalid mem base\n");
		return -EFAULT;
	}

	/**
	 * map head with the pad page
	 */
	for (pad_start = vma->vm_start; pad_start < vma->vm_end;
		pad_start += PAGE_SIZE) {
		if (remap_pfn_range(vma, pad_start, __phys_to_pfn(pad_base),
			PAGE_SIZE, vma->vm_page_prot)) {
			npu_drv_err("fail to map pad mem\n");
			return -ENXIO;
		}
	}

	npu_drv_debug("map l2 buff success");
	return 0;
}
