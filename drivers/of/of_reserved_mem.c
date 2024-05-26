// SPDX-License-Identifier: GPL-2.0+
/*
 * Device tree based initialization code for reserved memory.
 *
 * Copyright (c) 2013, 2015 The Linux Foundation. All Rights Reserved.
 * Copyright (c) 2013,2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 * Author: Marek Szyprowski <m.szyprowski@samsung.com>
 * Author: Josh Cartwright <joshc@codeaurora.org>
 */

#define pr_fmt(fmt)	"OF: reserved mem: " fmt

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/mm.h>
#include <linux/sizes.h>
#include <linux/of_reserved_mem.h>
#include <linux/sort.h>
#include <linux/slab.h>
#include <linux/memblock.h>

#if defined(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#define DT_MEM_RESERVED  "dt_mem_reserved"
#endif

#define MAX_RESERVED_REGIONS	64
static struct reserved_mem reserved_mem[MAX_RESERVED_REGIONS];
static int reserved_mem_count;
static unsigned long reserved_mem_size_info;
#if defined(CONFIG_DEBUG_FS)
static int dynamic_mem_reserved_count = 0;
static const char *dynamic_mem_reserved_array[MAX_RESERVED_REGIONS];
static int cma_mem_reserved_count = 0;
static const char *cma_mem_reserved_array[MAX_RESERVED_REGIONS];
#endif

#ifdef CONFIG_NEED_CHANGE_MAPPING
struct sec_memory_block sec_memblk_sp[MAX_SEC_MEMBLK_SP] = {
	{ .name = "mm-iris-sta-cma-pool", .base = 0, .size = 0, },
};
#endif

static int __init early_init_dt_alloc_reserved_memory_arch(phys_addr_t size,
	phys_addr_t align, phys_addr_t start, phys_addr_t end, bool nomap,
	phys_addr_t *res_base)
{
	phys_addr_t base;

	end = !end ? MEMBLOCK_ALLOC_ANYWHERE : end;
	align = !align ? SMP_CACHE_BYTES : align;
	base = memblock_find_in_range(start, end, size, align);
	if (!base)
		return -ENOMEM;

	*res_base = base;
	if (nomap)
		return memblock_remove(base, size);

	return memblock_reserve(base, size);
}

/**
 * fdt_reserved_mem_save_node() - save fdt node for second pass initialization
 */
void __init fdt_reserved_mem_save_node(unsigned long node, const char *uname,
				      phys_addr_t base, phys_addr_t size)
{
	struct reserved_mem *rmem = &reserved_mem[reserved_mem_count];

	if (reserved_mem_count == ARRAY_SIZE(reserved_mem)) {
		pr_err("not enough space for all defined regions.\n");
		return;
	}

	rmem->fdt_node = node;
	rmem->name = uname;
	rmem->base = base;
	rmem->size = size;
#if defined(CONFIG_DEBUG_FS)
	if((of_get_flat_dt_prop(node, "reusable", NULL))
		&& (cma_mem_reserved_count < MAX_RESERVED_REGIONS)) {
		cma_mem_reserved_array[cma_mem_reserved_count] = rmem->name;
		cma_mem_reserved_count++;
	}
#endif

	reserved_mem_count++;
	return;
}

/**
 * __reserved_mem_alloc_size() - allocate reserved memory described by
 *	'size', 'alignment'  and 'alloc-ranges' properties.
 */
static int __init __reserved_mem_alloc_size(unsigned long node,
	const char *uname, phys_addr_t *res_base, phys_addr_t *res_size)
{
	int t_len = (dt_root_addr_cells + dt_root_size_cells) * sizeof(__be32);
	phys_addr_t start = 0, end = 0;
	phys_addr_t base = 0, align = 0, size;
	int len;
	const __be32 *prop;
	bool nomap;
	int ret;

	prop = of_get_flat_dt_prop(node, "size", &len);
	if (!prop)
		return -EINVAL;

	if (len != dt_root_size_cells * sizeof(__be32)) {
		pr_err("invalid size property in '%s' node.\n", uname);
		return -EINVAL;
	}
	size = dt_mem_next_cell(dt_root_size_cells, &prop);

	prop = of_get_flat_dt_prop(node, "alignment", &len);
	if (prop) {
		if (len != dt_root_addr_cells * sizeof(__be32)) {
			pr_err("invalid alignment property in '%s' node.\n",
				uname);
			return -EINVAL;
		}
		align = dt_mem_next_cell(dt_root_addr_cells, &prop);
	}

	nomap = of_get_flat_dt_prop(node, "no-map", NULL) != NULL;

	/* Need adjust the alignment to satisfy the CMA requirement */
	if (IS_ENABLED(CONFIG_CMA)
	    && of_flat_dt_is_compatible(node, "shared-dma-pool")
	    && of_get_flat_dt_prop(node, "reusable", NULL)
	    && !nomap) {
		unsigned long order =
			max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

		align = max(align, (phys_addr_t)PAGE_SIZE << order);
	}

	prop = of_get_flat_dt_prop(node, "alloc-ranges", &len);
	if (prop) {

		if (len % t_len != 0) {
			pr_err("invalid alloc-ranges property in '%s', skipping node.\n",
			       uname);
			return -EINVAL;
		}

		base = 0;

		while (len > 0) {
			start = dt_mem_next_cell(dt_root_addr_cells, &prop);
			end = start + dt_mem_next_cell(dt_root_size_cells,
						       &prop);

			ret = early_init_dt_alloc_reserved_memory_arch(size,
					align, start, end, nomap, &base);
			if (ret == 0) {
				pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
					uname, &base,
					(unsigned long)size / SZ_1M);
				break;
			}
			len -= t_len;
		}

	} else {
		ret = early_init_dt_alloc_reserved_memory_arch(size, align,
							0, 0, nomap, &base);
		if (ret == 0)
			pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
				uname, &base, (unsigned long)size / SZ_1M);
	}

	if (base == 0) {
		pr_info("failed to allocate memory for node '%s'\n", uname);
		return -ENOMEM;
	}
#if defined(CONFIG_DEBUG_FS)
	if(dynamic_mem_reserved_count < MAX_RESERVED_REGIONS) {
		dynamic_mem_reserved_array[dynamic_mem_reserved_count] = uname;
		dynamic_mem_reserved_count++;
	}
#endif

	*res_base = base;
	*res_size = size;

	return 0;
}

static const struct of_device_id __rmem_of_table_sentinel
	__used __section("__reservedmem_of_table_end");

/**
 * __reserved_mem_init_node() - call region specific reserved memory init code
 */
static int __init __reserved_mem_init_node(struct reserved_mem *rmem)
{
	extern const struct of_device_id __reservedmem_of_table[];
	const struct of_device_id *i;
	int ret = -ENOENT;

	for (i = __reservedmem_of_table; i < &__rmem_of_table_sentinel; i++) {
		reservedmem_of_init_fn initfn = i->data;
		const char *compat = i->compatible;

		if (!of_flat_dt_is_compatible(rmem->fdt_node, compat))
			continue;

		ret = initfn(rmem);
		if (ret == 0) {
			pr_info("initialized node %s, compatible id %s\n",
				rmem->name, compat);
			break;
		}
	}
	return ret;
}

static int __init __rmem_cmp(const void *a, const void *b)
{
	const struct reserved_mem *ra = a, *rb = b;

	if (ra->base < rb->base)
		return -1;

	if (ra->base > rb->base)
		return 1;

	/*
	 * Put the dynamic allocations (address == 0, size == 0) before static
	 * allocations at address 0x0 so that overlap detection works
	 * correctly.
	 */
	if (ra->size < rb->size)
		return -1;
	if (ra->size > rb->size)
		return 1;

	return 0;
}

static void __init __rmem_check_for_overlap(void)
{
	int i;

	if (reserved_mem_count < 2)
		return;

	sort(reserved_mem, reserved_mem_count, sizeof(reserved_mem[0]),
	     __rmem_cmp, NULL);
	for (i = 0; i < reserved_mem_count - 1; i++) {
		struct reserved_mem *this, *next;

		this = &reserved_mem[i];
		next = &reserved_mem[i + 1];

		if (this->base + this->size > next->base) {
			phys_addr_t this_end, next_end;

			this_end = this->base + this->size;
			next_end = next->base + next->size;
			pr_err("OVERLAP DETECTED!\n%s (%pa--%pa) overlaps with %s (%pa--%pa)\n",
			       this->name, &this->base, &this_end,
			       next->name, &next->base, &next_end);
		}
	}
}

#ifdef CONFIG_NEED_CHANGE_MAPPING
static void sec_memblk_addr_set(struct reserved_mem *rmem)
{
	struct sec_memory_block *sec_mem = NULL;
	unsigned long node = rmem->fdt_node;
	int i;

	if (!IS_ENABLED(CONFIG_CMA)) {
		pr_err("%s: no CONFIG_CMA\n", __func__);
		return;
	}

	for (i = 0; i < MAX_SEC_MEMBLK_SP; i++) {
		sec_mem = &sec_memblk_sp[i];
		if (of_flat_dt_is_compatible(node, sec_mem->name)) {
			sec_mem->base = rmem->base;
			sec_mem->size = rmem->size;
		}
	}
}
#endif

/**
 * fdt_init_reserved_mem() - allocate and init all saved reserved memory regions
 */
void __init fdt_init_reserved_mem(void)
{
	int i;

	/* check for overlapping reserved regions */
	__rmem_check_for_overlap();

	for (i = 0; i < reserved_mem_count; i++) {
		struct reserved_mem *rmem = &reserved_mem[i];
		unsigned long node = rmem->fdt_node;
		int len;
		const __be32 *prop;
		int err = 0;
		bool nomap;

		nomap = of_get_flat_dt_prop(node, "no-map", NULL) != NULL;
		prop = of_get_flat_dt_prop(node, "phandle", &len);
		if (!prop)
			prop = of_get_flat_dt_prop(node, "linux,phandle", &len);
		if (prop)
			rmem->phandle = of_read_number(prop, len/4);

		if (rmem->size == 0)
			err = __reserved_mem_alloc_size(node, rmem->name,
						 &rmem->base, &rmem->size);
		if (err == 0) {
			err = __reserved_mem_init_node(rmem);
			if (err != 0 && err != -ENOENT) {
				pr_info("node %s compatible matching fail\n",
					rmem->name);
				memblock_free(rmem->base, rmem->size);
				if (nomap)
					memblock_add(rmem->base, rmem->size);
			}
		}

#ifdef CONFIG_NEED_CHANGE_MAPPING
		sec_memblk_addr_set(rmem);
#endif
		if (of_get_flat_dt_prop(node, "visible_to_users", NULL))
			continue;
		reserved_mem_size_info += rmem->size;
	}
}

static inline struct reserved_mem *__find_rmem(struct device_node *node)
{
	unsigned int i;

	if (!node->phandle)
		return NULL;

	for (i = 0; i < reserved_mem_count; i++)
		if (reserved_mem[i].phandle == node->phandle)
			return &reserved_mem[i];
	return NULL;
}

struct rmem_assigned_device {
	struct device *dev;
	struct reserved_mem *rmem;
	struct list_head list;
};

static LIST_HEAD(of_rmem_assigned_device_list);
static DEFINE_MUTEX(of_rmem_assigned_device_mutex);

/**
 * of_reserved_mem_device_init_by_idx() - assign reserved memory region to
 *					  given device
 * @dev:	Pointer to the device to configure
 * @np:		Pointer to the device_node with 'reserved-memory' property
 * @idx:	Index of selected region
 *
 * This function assigns respective DMA-mapping operations based on reserved
 * memory region specified by 'memory-region' property in @np node to the @dev
 * device. When driver needs to use more than one reserved memory region, it
 * should allocate child devices and initialize regions by name for each of
 * child device.
 *
 * Returns error code or zero on success.
 */
int of_reserved_mem_device_init_by_idx(struct device *dev,
				       struct device_node *np, int idx)
{
	struct rmem_assigned_device *rd;
	struct device_node *target;
	struct reserved_mem *rmem;
	int ret;

	if (!np || !dev)
		return -EINVAL;

	target = of_parse_phandle(np, "memory-region", idx);
	if (!target)
		return -ENODEV;

	if (!of_device_is_available(target)) {
		of_node_put(target);
		return 0;
	}

	rmem = __find_rmem(target);
	of_node_put(target);

	if (!rmem || !rmem->ops || !rmem->ops->device_init)
		return -EINVAL;

	rd = kmalloc(sizeof(struct rmem_assigned_device), GFP_KERNEL);
	if (!rd)
		return -ENOMEM;

	ret = rmem->ops->device_init(rmem, dev);
	if (ret == 0) {
		rd->dev = dev;
		rd->rmem = rmem;

		mutex_lock(&of_rmem_assigned_device_mutex);
		list_add(&rd->list, &of_rmem_assigned_device_list);
		mutex_unlock(&of_rmem_assigned_device_mutex);

		dev_info(dev, "assigned reserved memory node %s\n", rmem->name);
	} else {
		kfree(rd);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(of_reserved_mem_device_init_by_idx);

/**
 * of_reserved_mem_device_init_by_name() - assign named reserved memory region
 *					   to given device
 * @dev: pointer to the device to configure
 * @np: pointer to the device node with 'memory-region' property
 * @name: name of the selected memory region
 *
 * Returns: 0 on success or a negative error-code on failure.
 */
int of_reserved_mem_device_init_by_name(struct device *dev,
					struct device_node *np,
					const char *name)
{
	int idx = of_property_match_string(np, "memory-region-names", name);

	return of_reserved_mem_device_init_by_idx(dev, np, idx);
}
EXPORT_SYMBOL_GPL(of_reserved_mem_device_init_by_name);

/**
 * of_reserved_mem_device_release() - release reserved memory device structures
 * @dev:	Pointer to the device to deconfigure
 *
 * This function releases structures allocated for memory region handling for
 * the given device.
 */
void of_reserved_mem_device_release(struct device *dev)
{
	struct rmem_assigned_device *rd, *tmp;
	LIST_HEAD(release_list);

	mutex_lock(&of_rmem_assigned_device_mutex);
	list_for_each_entry_safe(rd, tmp, &of_rmem_assigned_device_list, list) {
		if (rd->dev == dev)
			list_move_tail(&rd->list, &release_list);
	}
	mutex_unlock(&of_rmem_assigned_device_mutex);

	list_for_each_entry_safe(rd, tmp, &release_list, list) {
		if (rd->rmem && rd->rmem->ops && rd->rmem->ops->device_release)
			rd->rmem->ops->device_release(rd->rmem, dev);

		kfree(rd);
	}
}
EXPORT_SYMBOL_GPL(of_reserved_mem_device_release);

/**
 * of_reserved_mem_lookup() - acquire reserved_mem from a device node
 * @np:		node pointer of the desired reserved-memory region
 *
 * This function allows drivers to acquire a reference to the reserved_mem
 * struct based on a device node handle.
 *
 * Returns a reserved_mem reference, or NULL on error.
 */
struct reserved_mem *of_reserved_mem_lookup(struct device_node *np)
{
	const char *name;
	int i;

	if (!np->full_name)
		return NULL;

	name = kbasename(np->full_name);
	for (i = 0; i < reserved_mem_count; i++)
		if (!strcmp(reserved_mem[i].name, name))
			return &reserved_mem[i];

	return NULL;
}
EXPORT_SYMBOL_GPL(of_reserved_mem_lookup);

unsigned long dt_memory_reserved_sizeinfo_get(void)
{
	return reserved_mem_size_info;
}
EXPORT_SYMBOL_GPL(dt_memory_reserved_sizeinfo_get);

#if defined(CONFIG_DEBUG_FS)

static int dt_memory_reserved_debug_show(struct seq_file *m, void *private)
{
	struct reserved_mem *dt_mem_resrved = m->private;
	struct reserved_mem *rmem;
	int i, j;
	int cma = 0;
	int dynamic = 0;

	seq_printf(m, "  num [start            ....              end]      [size]       [d/s]     [cma]       [ name ]\n");

	for (i = 0; i < reserved_mem_count; i++) {
		cma = 0;
		dynamic = 0;
		rmem = &(dt_mem_resrved[i]);

		/* look for dynamic reserve memory node */
		for(j = 0; j < dynamic_mem_reserved_count; j++) {
			if(!strcmp(rmem->name, dynamic_mem_reserved_array[j])) {
				dynamic = 1;
				break;
			}
		}

		/* look for cma reserve memory node */
		for(j = 0; j < cma_mem_reserved_count; j++) {
			if(!strcmp(rmem->name, cma_mem_reserved_array[j])) {
				cma = 1;
				break;
			}
		}

		seq_printf(m, "%4d: ", i);
		seq_printf(m, "[0x%016llx..0x%016llx]  %8lluKB   %8s %8s        %8s \n",
			   (unsigned long long)rmem->base,
			   (unsigned long long)(rmem->base + rmem->size - 1),
			   (unsigned long long)rmem->size /SZ_1K ,
			   (dynamic == 1)?"d":"s",
			   (cma == 1)?"Y":"N",
			   rmem->name);
	}

	return 0;
}

static int dt_memory_reserved_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, dt_memory_reserved_debug_show, inode->i_private);
}

static const struct file_operations dt_memory_reserved_debug_fops = {
	.open = dt_memory_reserved_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init dt_memory_reserved_init_debugfs(void)
{
	struct dentry *root = debugfs_create_dir(DT_MEM_RESERVED, NULL);
	if (!root)
		return -ENXIO;
	debugfs_create_file("dt_memory_reserved", S_IRUGO, root, reserved_mem, &dt_memory_reserved_debug_fops);

	return 0;
}
__initcall(dt_memory_reserved_init_debugfs);

#endif /* CONFIG_DEBUG_FS */
