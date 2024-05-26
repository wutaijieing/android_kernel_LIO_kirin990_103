/*
 * tz_pm.c
 *
 * function for proc open,close session and invoke
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#include "tz_pm.h"
#include <securec.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include "tc_ns_client.h"
#include "teek_ns_client.h"
#include "tc_ns_log.h"

#define S4_ADDR_4G              0xffffffff
#define RESERVED_SECOS_PHYMEM_BASE                  0x22800000
#define RESERVED_SECOS_PHYMEM_SIZE                  (0x3000000)
#define RESERVED_SECOS_S4_BASE                      0x27760000
#define RESERVED_SECOS_S4_SIZE                      (0x100000)

static char *g_s4_kernel_mem_addr;
static char *g_s4_buffer_vaddr;
static uint64_t g_s4_buffer_paddr;
static uint32_t g_s4_buffer_size;

static void *tc_vmap(phys_addr_t paddr, size_t size)
{
	uint32_t i;
	void *vaddr = NULL;
	pgprot_t pgprot = PAGE_KERNEL;
	uintptr_t offset;
	uint32_t pages_count;
	struct page **pages = NULL;

	offset = paddr & ~PAGE_MASK;
	paddr &= PAGE_MASK;
	pages_count = PAGE_ALIGN(size + offset) / PAGE_SIZE;

	pages = kzalloc(sizeof(struct page *) * pages_count, GFP_KERNEL);
	if (pages == NULL)
		return NULL;

	for (i = 0; i < pages_count; i++)
		*(pages + i) = phys_to_page((uintptr_t)(paddr + PAGE_SIZE * i));

	vaddr = vmap(pages, pages_count, VM_MAP, pgprot);
	kfree(pages);
	if (vaddr == NULL)
		return NULL;

	return offset + (char *)vaddr;
}

static int tc_s4_alloc_crypto_buffer(struct device *dev,
	 char **kernel_mem_addr)
{
	if (RESERVED_SECOS_S4_BASE > S4_ADDR_4G) {
		tloge("addr is invalid\n");
		return -EFAULT;
	}

	g_s4_buffer_vaddr = tc_vmap(RESERVED_SECOS_S4_BASE, RESERVED_SECOS_S4_SIZE);
	if (g_s4_buffer_vaddr == NULL) {
		tloge("vmap failed for s4\n");
		return -EFAULT;
	}
	g_s4_buffer_paddr = RESERVED_SECOS_S4_BASE;
	g_s4_buffer_size = RESERVED_SECOS_S4_SIZE;

	*kernel_mem_addr = vmalloc(RESERVED_SECOS_PHYMEM_SIZE);
	if (*kernel_mem_addr == NULL) {
		vunmap(g_s4_buffer_vaddr);
		g_s4_buffer_paddr = 0;
		g_s4_buffer_vaddr = NULL;
		g_s4_buffer_size = 0;
		tloge("vmalloc failed for s4\n");
		return -ENOMEM;
	}

	return 0;
}

static void free_resource(const char *kernel_mem_addr)
{
	vunmap(g_s4_buffer_vaddr);
	vfree(kernel_mem_addr);
	g_s4_kernel_mem_addr = NULL;
	g_s4_buffer_paddr = 0;
	g_s4_buffer_vaddr = NULL;
	g_s4_buffer_size = 0;
}

static uint64_t tc_s4_suspend_or_resume(uint32_t power_op)
{
	u64 smc_id = (u64)power_op;
	u64 smc_ret = 0xffff;

	do {
		asm volatile (
			"mov x0, %[fid]\n"
			"smc #0\n"
			"str x0, [%[re0]]\n" :
			[fid] "+r"(smc_id) :
			[re0] "r"(&smc_ret) :
			"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			"x8", "x9", "x10", "x11", "x12", "x13",
			"x14", "x15", "x16", "x17");
	} while (0);

	isb();
	wmb();
	return smc_ret;
}

static uint64_t tc_s4_crypto_and_copy(uint32_t crypt_op,
	uint64_t middle_mem_addr,
	uintptr_t secos_mem,
	uint32_t size, uint32_t index)
{
	u64 smc_id = (u64)crypt_op;
	u64 arg0 = (u64)middle_mem_addr;
	u64 arg1 = (u64)secos_mem;
	u64 arg2 = (u64)size;
	u64 arg3 = (u64)index;
	u64 smc_ret = 0xffff;

	do {
		asm volatile (
			"mov x0, %[fid]\n"
			"mov x1, %[a1]\n"
			"mov x2, %[a2]\n"
			"mov x3, %[a3]\n"
			"mov x4, %[a4]\n"
			"smc #0\n"
			"str x0, [%[re0]]\n" :
			[fid] "+r"(smc_id),
			[a1] "+r"(arg0),
			[a2] "+r"(arg1),
			[a3] "+r"(arg2),
			[a4] "+r"(arg3) :
			[re0] "r"(&smc_ret) :
			"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			"x8", "x9", "x10", "x11", "x12", "x13",
			"x14", "x15", "x16", "x17");
	} while (0);

	isb();
	wmb();
	return smc_ret;
}

static int tc_s4_transfer_data(char *kernel_mem_addr, uint32_t crypt_op)
{
	uint32_t index = 0;
	uint32_t copied_size = 0;

	while (copied_size < RESERVED_SECOS_PHYMEM_SIZE) {
		if (crypt_op == TSP_S4_DECRYPT_AND_COPY) {
			if (memcpy_s(g_s4_buffer_vaddr, g_s4_buffer_size,
				kernel_mem_addr + copied_size,
				g_s4_buffer_size) != EOK) {
				tloge("mem copy for decrypt failed\n");
				return -EFAULT;
			}
		}

		if (tc_s4_crypto_and_copy(crypt_op, g_s4_buffer_paddr,
			RESERVED_SECOS_PHYMEM_BASE + copied_size,
			g_s4_buffer_size, index) != 0) {
			tloge("crypto and copy failed\n");
			return -EFAULT;
		}

		if (crypt_op == TSP_S4_ENCRYPT_AND_COPY) {
			if (memcpy_s(kernel_mem_addr + copied_size,
				g_s4_buffer_size, g_s4_buffer_vaddr,
				g_s4_buffer_size) != EOK) {
				tloge("mem copy for encrypt failed\n");
				return -EFAULT;
			}
		}

		copied_size += g_s4_buffer_size;
		index++;
	}

	return 0;
}

static int tc_s4_pm_ops(struct device *dev, uint32_t power_op,
			uint32_t crypt_op, char *kernel_mem_addr)
{
	int ret;

	if (power_op == TSP_S4_SUSPEND)
		g_s4_kernel_mem_addr = kernel_mem_addr;
	else
		kernel_mem_addr = g_s4_kernel_mem_addr;

	isb();
	wmb();

	/* notify TEEOS to suspend all pm driver */
	if (power_op == TSP_S4_SUSPEND) {
		ret = tc_s4_suspend_or_resume(power_op);
		if (ret != 0) {
			tloge("tc s4 suspend failed\n");
			return ret;
		}
	}

	ret = tc_s4_transfer_data(kernel_mem_addr, crypt_op);
	if (ret != 0) {
		tloge("transfer data failed, power_op=0x%x\n", power_op);
		return ret;
	}

	/* notify TEEOS to resume all pm driver */
	if (power_op == TSP_S4_RESUME) {
	ret = tc_s4_suspend_or_resume(power_op);
		if (ret != 0) {
			tloge("tc s4 resume failed\n");
			return ret;
		}
	}

	return 0;
}

int tc_s4_pm_suspend(struct device *dev)
{
	int ret;
	char *kernel_mem_addr = NULL;

	ret = tc_s4_alloc_crypto_buffer(dev, &kernel_mem_addr);
	if (ret != 0) {
		tloge("alloc buffer failed\n");
		return ret;
	}

	ret = tc_s4_pm_ops(dev, TSP_S4_SUSPEND, TSP_S4_ENCRYPT_AND_COPY, kernel_mem_addr);
	if (ret != 0) {
		free_resource(kernel_mem_addr);
		tloge("s4 suspend failed\n");
	}

	return ret;
}

int tc_s4_pm_resume(struct device *dev)
{
	int ret;

	ret = tc_s4_pm_ops(dev, TSP_S4_RESUME, TSP_S4_DECRYPT_AND_COPY, g_s4_kernel_mem_addr);
	if (ret != 0)
		tloge("s4 resume failed\n");

	free_resource(g_s4_kernel_mem_addr);
	return ret;
}
