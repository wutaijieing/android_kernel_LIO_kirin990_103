/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2019-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#ifndef _MEMORY_GROUP_MANAGER_H_
#define _MEMORY_GROUP_MANAGER_H_

#include <linux/mm.h>
#include <linux/of.h>
#include <linux/version.h>

#include <mali_malisw.h>
#ifdef CONFIG_MALI_LAST_BUFFER
#include <linux/platform_drivers/mm_lb.h>
#endif

#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
typedef int vm_fault_t;
#endif

#define MEMORY_GROUP_MANAGER_NR_GROUPS (16)

struct memory_group_manager_device;
struct memory_group_manager_import_data;

/**
 * struct memory_group_manager_ops - Callbacks for memory group manager
 *                                   operations
 *
 * @mgm_alloc_page:           Callback to allocate physical memory in a group
 * @mgm_free_page:            Callback to free physical memory in a group
 * @mgm_get_import_memory_id: Callback to get the group ID for imported memory
 * @mgm_update_gpu_pte:       Callback to modify a GPU page table entry
 * @mgm_vmf_insert_pfn_prot:  Callback to map a physical memory page for the CPU
 */
struct memory_group_manager_ops {
	/*
	 * mgm_alloc_page - Allocate a physical memory page in a group
	 *
	 * @mgm_dev:  The memory group manager through which the request is
	 *            being made.
	 * @group_id: A physical memory group ID. The meaning of this is defined
	 *            by the systems integrator. Its valid range is
	 *            0 .. MEMORY_GROUP_MANAGER_NR_GROUPS-1.
	 * @gfp_mask: Bitmask of Get Free Page flags affecting allocator
	 *            behavior.
	 * @order:    Page order for physical page size (order=0 means 4 KiB,
	 *            order=9 means 2 MiB).
	 *
	 * Return: Pointer to allocated page, or NULL if allocation failed.
	 */
	struct page *(*mgm_alloc_page)(
		struct memory_group_manager_device *mgm_dev, int group_id,
		gfp_t gfp_mask, unsigned int order);

	/*
	 * mgm_free_page - Free a physical memory page in a group
	 *
	 * @mgm_dev:  The memory group manager through which the request
	 *            is being made.
	 * @group_id: A physical memory group ID. The meaning of this is
	 *            defined by the systems integrator. Its valid range is
	 *            0 .. MEMORY_GROUP_MANAGER_NR_GROUPS-1.
	 * @page:     Address of the struct associated with a page of physical
	 *            memory that was allocated by calling the mgm_alloc_page
	 *            method of the same memory pool with the same values of
	 *            @group_id and @order.
	 * @order:    Page order for physical page size (order=0 means 4 KiB,
	 *            order=9 means 2 MiB).
	 */
	void (*mgm_free_page)(
		struct memory_group_manager_device *mgm_dev, int group_id,
		struct page *page, unsigned int order);

	/*
	 * mgm_update_sizes - update the size to group_id in a group
	 *
	 * @mgm_dev:  The memory group manager through which the request
	 *            is being made.
	 * @group_id: A physical memory group ID. The meaning of this is
	 *            defined by the systems integrator. Its valid range is
	 *            0 .. MEMORY_GROUP_MANAGER_NR_GROUPS-1.
	 * @order:    Page order for physical page size (order=0 means 4 KiB,
	 *            order=9 means 2 MiB).
	 * @is_alloc: true for alloc or false for free.
	 */
	void (*mgm_update_sizes) (
		struct memory_group_manager_device *mgm_dev, int group_id,
		unsigned int order, bool is_alloc);

	/*
	 * mgm_get_import_memory_id - Get the physical memory group ID for the
	 *                            imported memory
	 *
	 * @mgm_dev:     The memory group manager through which the request
	 *               is being made.
	 * @data: Pointer to the data which describes imported memory.
	 *
	 * Note that provision of this call back is optional, where it is not
	 * provided this call back pointer must be set to NULL to indicate it
	 * is not in use.
	 *
	 * Return: The memory group ID to use when mapping pages from this
	 *         imported memory.
	 */
	int (*mgm_get_import_memory_id)(
		struct memory_group_manager_device *mgm_dev,
		struct memory_group_manager_import_data *data);

	/*
	 * gpu_map_page -- Map a physical address to the GPU
	 *
	 * This function can be used to encode extra information into the
	 * GPU's page table entry.
	 * @mmu_level: The level the page table is being built for.
	 * @pte: The prebuilt page table entry from KBase, either in lpae
	 * or aarch64 depending on the drivers configuration. This should be
	 * decoded to determine the physical address and other properties of
	 * the mapping the manager requires.
	 *
	 * Return: A page table entry prot for KBase to store in the
	 * page tables.
	 */
	u64 (*gpu_map_page)(u8 mmu_level, u64 pte);

	/*
	 * mgm_update_gpu_pte - Modify a GPU page table entry for a memory group
	 *
	 * @mgm_dev:   The memory group manager through which the request
	 *             is being made.
	 * @group_id:  A physical memory group ID. The meaning of this is
	 *             defined by the systems integrator. Its valid range is
	 *             0 .. MEMORY_GROUP_MANAGER_NR_GROUPS-1.
	 * @mmu_level: The level of the page table entry in @ate.
	 * @pte:       The page table entry to modify, in LPAE or AArch64 format
	 *             (depending on the driver's configuration). This should be
	 *             decoded to determine the physical address and any other
	 *             properties of the mapping the manager requires.
	 *
	 * This function allows the memory group manager to modify a GPU page
	 * table entry before it is stored by the kbase module (controller
	 * driver). It may set certain bits in the page table entry attributes
	 * or in the physical address, based on the physical memory group ID.
	 *
	 * Return: A modified GPU page table entry to be stored in a page table.
	 */
	u64 (*mgm_update_gpu_pte)(struct memory_group_manager_device *mgm_dev,
			int group_id, int mmu_level, u64 pte);

	/*
	 * mgm_vmf_insert_pfn_prot - Map a physical page in a group for the CPU
	 *
	 * @mgm_dev:   The memory group manager through which the request
	 *             is being made.
	 * @group_id:  A physical memory group ID. The meaning of this is
	 *             defined by the systems integrator. Its valid range is
	 *             0 .. MEMORY_GROUP_MANAGER_NR_GROUPS-1.
	 * @vma:       The virtual memory area to insert the page into.
	 * @addr:      A virtual address (in @vma) to assign to the page.
	 * @pfn:       The kernel Page Frame Number to insert at @addr in @vma.
	 * @pgprot:    Protection flags for the inserted page.
	 *
	 * Called from a CPU virtual memory page fault handler. This function
	 * creates a page table entry from the given parameter values and stores
	 * it at the appropriate location (unlike mgm_update_gpu_pte, which
	 * returns a modified entry).
	 *
	 * Return: Type of fault that occurred or VM_FAULT_NOPAGE if the page
	 *         table entry was successfully installed.
	 */
	vm_fault_t (*mgm_vmf_insert_pfn_prot)(
		struct memory_group_manager_device *mgm_dev, int group_id,
		struct vm_area_struct *vma, unsigned long addr,
		unsigned long pfn, pgprot_t pgprot);

	/*
	 * remap_vmalloc_range -- map vmalloc pages to userspace
	 *
	 * @mgm_dev: The memory group manager the request is being made through.
	 * @vma: vma to cover (map full range of vma).
	 * @addr: vmalloc memory.
	 * @pgoff: number of pages into addr before first page to map.
	 */
	int (*remap_vmalloc_range)(struct memory_group_manager_device *mgm_dev,
		struct vm_area_struct *vma, void *addr,
		unsigned long pgoff);

	/*
	 * vmap  - map an array of pages into virtually contiguous space
	 * @pages: array of page pointers
	 * @count: number of pages to map
	 * @offset: number of normal pages offset to the start of
	 * virtual space. pages[offset] is the beginning of the
	 * normal memory pages.
	 * @flags: vm_area->flags
	 * @prot: page protection for the mapping
	 *
	 * Maps @count pages from @pages into contiguous kernel virtual space.
	 */
	void *(*vmap)(struct page **pages, unsigned int count, unsigned int offset,
		unsigned long flags, pgprot_t prot);
};

/**
 * struct memory_group_manager_device - Device structure for a memory group
 *                                      manager
 *
 * @ops:   Callbacks associated with this device
 * @data:  Pointer to device private data
 * @owner: pointer to owning module
 *
 * In order for a systems integrator to provide custom behaviors for memory
 * operations performed by the kbase module (controller driver), they must
 * provide a platform-specific driver module which implements this interface.
 *
 * This structure should be registered with the platform device using
 * platform_set_drvdata().
 */
struct memory_group_manager_device {
	struct memory_group_manager_ops ops;
	void *data;
	struct module *owner;
};


enum memory_group_manager_import_type {
	MEMORY_GROUP_MANAGER_IMPORT_TYPE_DMA_BUF
};

/**
 * struct memory_group_manager_import_data - Structure describing the imported
 *                                           memory
 *
 * @type:  - type of imported memory
 * @u:     - Union describing the imported memory
 *
 */
struct memory_group_manager_import_data {
	enum memory_group_manager_import_type type;
	union {
		struct dma_buf *dma_buf;
	} u;
};

#endif /* _MEMORY_GROUP_MANAGER_H_ */
