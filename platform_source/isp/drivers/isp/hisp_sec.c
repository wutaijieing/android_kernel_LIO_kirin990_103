/*
 *  ISP driver, isp_hisp_smc.c
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/scatterlist.h>
#include <linux/printk.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/remoteproc.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/genalloc.h>
#include <linux/mm_iommu.h>
#include <linux/mutex.h>
#include <linux/iommu.h>
#include <linux/compiler.h>
#include <linux/cpumask.h>
#include <linux/uaccess.h>
#include <asm/compiler.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <global_ddr_map.h>
#include "teek_client_id.h"
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/isp/linux/hisp_mempool.h>
#include "platform_include/basicplatform/linux/partition/partition_macro.h"
#include <isp_ddr_map.h>
#include <linux/list.h>
#include <securec.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include "hisp_internel.h"

#define SEC_MAX_SIZE    64

enum a7mappint_type {
	A7BOOT = 0,
	A7TEXT,
	A7DATA,
	A7PGD,
	A7PMD,
	A7PTE,
	A7RDR,
	A7SHARED,
	A7VQ,
	A7VRING0,
	A7VRING1,
	A7HEAP,
	A7DYNAMIC,
	A7MDC,
	MAXA7MAPPING
};

enum isp_tsmem_offset_info {
	ISP_TSMEM_OFFSET = 0,
	ISP_CPU_PARM_OFFSET,
	ISP_SHARE_MEM_OFFSET,
	ISP_DYN_MEM_OFFSET,
	ISP_TSMEM_MAX,
};

struct isp_atfshrdmem_s {
	u64 sec_pgt;
	unsigned int sec_mem_size;
	struct isp_a7mapping_s a7mapping[MAXA7MAPPING];
};

struct hisp_sec {
	int use_ca_ta;
	int secmem_count;
	unsigned int is_heap_flag;
	int ispmem_reserved;
	unsigned int boardid;
	unsigned int rsctable_offset;
	unsigned int rsctable_size;
	unsigned int trusted_mem_size;
	unsigned int mapping_items;
	unsigned int share_mem_size;
	unsigned int sec_ta_enable;
	unsigned int boot_mem_ready_flag;
	unsigned int sec_poweron_status;
	unsigned long isp_iova_start;
	unsigned long isp_iova_size;
	unsigned int tsmem_offset[ISP_TSMEM_MAX];
	u64 atfshrd_paddr;
	u64 rsctable_paddr;
	u64 sec_boot_phymem_addr;
	void *atfshrd_vaddr;
	void *rsctable_vaddr_const;
	void *ap_dyna_array;
	struct device *device;
	struct platform_device *pdev;
	struct isp_atfshrdmem_s *shrdmem;
	struct isp_a7mapping_s *ap_dyna;
	struct gen_pool *isp_iova_pool;
	struct dma_buf *boot_dmabuf[HISP_SEC_BOOT_MAX_TYPE];
	struct dma_buf *pool_dmabuf[HISP_SEC_POOL_MAX_TYPE];
	struct hisp_ca_meminfo boot_mem[HISP_SEC_BOOT_MAX_TYPE];
	struct hisp_ca_meminfo rsv_mem[HISP_SEC_RSV_MAX_TYPE];
	struct hisp_ca_meminfo pool_mem[HISP_SEC_POOL_MAX_TYPE];
	struct mutex pwrlock;
	struct mutex ca_mem_mutex;
	struct mutex ta_status_mutex;
};

struct map_sglist_s {
	unsigned long long addr;
	unsigned int size;
};

struct hisp_sec hisp_sec_dev;
unsigned int map_type_info[MAP_TYPE_MAX];

static const char * const secmem_propname[] = {
	"a7-vaddr-boot",
	"a7-vaddr-text",
	"a7-vaddr-data",
	"a7-vaddr-pgd",
	"a7-vaddr-pmd",
	"a7-vaddr-pte",
	"a7-vaddr-rdr",
	"a7-vaddr-shrd",
	"a7-vaddr-vq",
	"a7-vaddr-vr0",
	"a7-vaddr-vr1",
	"a7-vaddr-heap",
	"a7-vaddr-a7dyna",
	"a7-vaddr-mdc",
};

static const char * const ca_bootmem_propname[] = {
	"ispcpu-text",
	"ispcpu-data",
};

static const char * const ca_rsvmem_propname[] = {
	"ispcpu-vr0",
	"ispcpu-vr1",
	"ispcpu-vq",
	"ispcpu-shrd",
	"ispcpu-rdr",
};

static const char * const ca_mempool_propname[] = {
	"ispcpu-dynamic-pool",
	"ispcpu-sec-pool",
	"ispcpu-ispsec-pool",
};

int hisp_ca_boot_mode(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->use_ca_ta;
}

u64 hisp_get_ispcpu_shrmem_secpa(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->atfshrd_paddr + dev->tsmem_offset[ISP_SHARE_MEM_OFFSET];
}

void *hisp_get_ispcpu_shrmem_secva(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->atfshrd_vaddr + dev->tsmem_offset[ISP_SHARE_MEM_OFFSET];
}

void *hisp_get_dyna_array(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->ap_dyna_array;
}

struct isp_a7mapping_s *hisp_get_ap_dyna_mapping(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->ap_dyna;
}

int hisp_sec_jpeg_powerup(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	u64 pgd_base;
	int ret = 0;

	mutex_lock(&dev->pwrlock);
	ret = hisp_pwr_subsys_powerup();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_pwr_subsys_powerup.%d\n",
			__func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	pgd_base = hisp_nsec_get_pgd();
	ret = hisp_pwr_core_sec_init(pgd_base);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_pwr_core_sec_init.%d\n",
			__func__, ret);
		goto err_jpegup;
	}
	mutex_unlock(&dev->pwrlock);

	return 0;

err_jpegup:
	if (hisp_pwr_subsys_powerdn() < 0)
		pr_err("[%s] Failed : err_jpegup hisp_pwr_subsys_powerdn\n",
			__func__);

	mutex_unlock(&dev->pwrlock);

	return ret;
}

int hisp_sec_jpeg_powerdn(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret;

	mutex_lock(&dev->pwrlock);
	ret = hisp_pwr_core_sec_exit();
	if (ret < 0)
		pr_err("[%s] Failed : hisp_pwr_core_sec_exit.%d\n",
			__func__, ret);

	ret = hisp_pwr_subsys_powerdn();
	if (ret < 0)
		pr_err("[%s] Failed : hisp_pwr_subsys_powerdn.%d\n",
			__func__, ret);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

static int hisp_sec_rsctablemem_init(struct hisp_sec *dev)
{
	dma_addr_t dma_addr = 0;

	dev->rsctable_vaddr_const = dma_alloc_coherent(dev->device,
				dev->rsctable_size, &dma_addr, GFP_KERNEL);
	if (dev->rsctable_vaddr_const == NULL) {
		pr_err("[%s] failed : rsctable_vaddr_const.%pK\n",
				__func__, dev->rsctable_vaddr_const);
		return -ENOMEM;
	}
	dev->rsctable_paddr = (unsigned long long)dma_addr;

	return 0;
}

static void hisp_sec_rsctablemem_exit(struct hisp_sec *dev)
{
	if (dev->rsctable_vaddr_const != NULL)
		dma_free_coherent(dev->device, dev->rsctable_size,
			dev->rsctable_vaddr_const, dev->rsctable_paddr);
	dev->rsctable_vaddr_const = NULL;
	dev->rsctable_paddr = 0;
}

u64 hisp_get_param_info_pa(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->atfshrd_paddr + dev->tsmem_offset[ISP_CPU_PARM_OFFSET];
}

void *hisp_get_param_info_va(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	if (dev->atfshrd_vaddr != NULL)
		return (dev->atfshrd_vaddr +
		       dev->tsmem_offset[ISP_CPU_PARM_OFFSET]);

	return NULL;
}

static int hisp_smc_shrmem_init(struct hisp_sec *dev)
{
	dma_addr_t dma_addr = 0;

	dev->atfshrd_vaddr = hisp_fstcma_alloc(&dma_addr,
		dev->share_mem_size, GFP_KERNEL);
	if (dev->atfshrd_vaddr == NULL) {
		pr_err("[%s] atfshrd_vaddr.%pK\n",
			__func__, dev->atfshrd_vaddr);
		return -ENOMEM;
	}

	dev->atfshrd_paddr = (unsigned long long)dma_addr;
	dev->shrdmem = (struct isp_atfshrdmem_s *)dev->atfshrd_vaddr;

	return 0;
}

static void hisp_smc_shrmem_exit(struct hisp_sec *dev)
{
	if (dev->atfshrd_vaddr != NULL)
		hisp_fstcma_free(dev->atfshrd_vaddr,
			dev->atfshrd_paddr, dev->share_mem_size);
	dev->atfshrd_paddr = 0;
	dev->atfshrd_vaddr = NULL;
}

void hisp_sec_set_ispcpu_palist(void *listmem,
		struct scatterlist *sg, unsigned int size)
{
	dma_addr_t dma_addr = 0;
	unsigned int len, set_size = 0;
	struct map_sglist_s *maplist = listmem;
	unsigned int last_counts = 0, last_len = 0;

	while (sg != NULL) {
		dma_addr = sg_dma_address(sg);
		if (dma_addr == 0)
			dma_addr = sg_phys(sg);

		len = sg->length;
		if (len == 0) {
			pr_err("[%s] break len.0x%x\n", __func__, len);
			break;
		}

		set_size += len;
		if (set_size > size) {
			pr_err("[%s] break size.(0x%x > 0x%x), len.0x%x\n",
					__func__, set_size, size, len);
			maplist->addr = (unsigned long long)dma_addr;
			maplist->size = len - (set_size - size);
			break;
		}

		maplist->addr = (unsigned long long)dma_addr;
		maplist->size = len;
		if (last_len != len) {
			if (last_len != 0) {
				pr_info("[%s] list.(%pK + %pK), ",
					__func__, listmem, maplist);
				pr_info("maplist.(0x%x X 0x%x)\n",
					last_counts, last_len);
			}
			last_counts = 1;
			last_len = len;
		} else {
			last_counts++;
		}

		maplist++;
		sg = sg_next(sg);
	}

	pr_info("[%s] list.(%pK + %pK), maplist.(0x%x X 0x%x)\n",
		__func__, listmem, maplist, last_counts, last_len);
	pr_info("%s: size.0x%x == set_size.0x%x\n", __func__, size, set_size);
}

int hisp_request_sec_rsctable(struct rproc *rproc)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] +\n", __func__);
	if (dev->rsctable_vaddr_const == NULL) {
		pr_err("[%s] rsctable_vaddr_const.%pK\n",
			__func__, dev->rsctable_vaddr_const);
		return -ENOMEM;
	}

	rproc->cached_table = kmemdup(dev->rsctable_vaddr_const,
				dev->rsctable_size, GFP_KERNEL);
	if (!rproc->cached_table)
		return -ENOMEM;

	rproc->table_ptr = rproc->cached_table;
	rproc->table_sz = dev->rsctable_size;
	pr_info("[%s] -\n", __func__);

	return 0;
}

u64 hisp_sec_get_remap_pa(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	if (dev->sec_boot_phymem_addr)
		return dev->sec_boot_phymem_addr;

	return 0;
}

static unsigned long hisp_set_secmem_addr_pa(unsigned int etype,
					unsigned int vaddr, unsigned int offset)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	struct rproc *rproc = platform_get_drvdata(dev->pdev);
	unsigned long addr = 0;
	unsigned long remap = 0;

	remap = (unsigned long)hisp_sec_get_remap_pa();
	switch (etype) {
	case A7TEXT:
	case A7PGD:
	case A7PMD:
	case A7PTE:
	case A7DATA:
		addr = (unsigned long)(remap + offset);
		break;
	case A7BOOT:
		addr = (unsigned long)(remap + offset);
		break;
	case A7HEAP:
		addr = (unsigned long)offset;
		break;
	case A7VRING0:
	case A7VRING1:
		addr = hisp_rproc_get_vringmem_pa(rproc, HISP_VRING0 + etype - A7VRING0);
		break;
	default:
		pr_debug("[%s] Failed : etype.0x%x\n", __func__, etype);
		return 0;
	}

	return addr;
}

static int hisp_sec_set_share_pararms(void)
{
	int ret = 0;
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	ret = hisp_smc_setparams(dev->atfshrd_paddr);
	if (ret < 0) {
		pr_err("[%s] hisp_smc_setparams.%d\n", __func__, ret);
		return -ENODEV;
	}
	return 0;
}

static int hisp_dynpool_ca_map(int sharefd, unsigned int size)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	if (dev->pool_mem[HISP_DYNAMIC_POOL].sharefd > 0) {
		pr_err("[%s] Failed : dynamic memory has already mapped\n",
			__func__);
		return -EINVAL;
	}

	dev->pool_mem[HISP_DYNAMIC_POOL].sharefd = sharefd;
	dev->pool_mem[HISP_DYNAMIC_POOL].size = size;

	ret = hisp_ca_dynmem_map(&dev->pool_mem[HISP_DYNAMIC_POOL],
			dev->device);
	if (ret < 0) {
		pr_err("[%s] Failed :hisp_ca_dynmem_map. ret.%d\n",
			__func__, ret);
		dev->pool_mem[HISP_DYNAMIC_POOL].sharefd = 0;
		dev->pool_mem[HISP_DYNAMIC_POOL].size = 0;
	}

	return ret;
}

static int hisp_dynpool_ca_unmap(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	if (dev->pool_mem[HISP_DYNAMIC_POOL].sharefd <= 0 ||
		dev->pool_mem[HISP_DYNAMIC_POOL].size == 0) {
		pr_err("[%s] Failed : memory has mapped. fd.%d, size.%u\n",
			__func__, dev->pool_mem[HISP_DYNAMIC_POOL].sharefd,
			dev->pool_mem[HISP_DYNAMIC_POOL].size);
		return -EINVAL;
	}

	ret = hisp_ca_dynmem_unmap(&dev->pool_mem[HISP_DYNAMIC_POOL],
			dev->device);
	if (ret < 0)
		pr_err("[%s] Failed :hisp_ca_dynmem_map. ret.%d\n",
			__func__, ret);

	dev->pool_mem[HISP_DYNAMIC_POOL].sharefd = 0;
	dev->pool_mem[HISP_DYNAMIC_POOL].size = 0;

	return ret;
}

static int hisp_secpool_ca_map(int sharefd, unsigned int size,
		unsigned int pool_num)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	if (dev->pool_mem[pool_num].sharefd > 0) {
		pr_err("[%s] Failed : sec memory has mapped\n", __func__);
		return -EINVAL;
	}

	dev->pool_mem[pool_num].sharefd = sharefd;
	dev->pool_mem[pool_num].size = size;

	ret = hisp_ca_sfdmem_map(&dev->pool_mem[pool_num]);
	if (ret < 0) {
		pr_err("[%s] Failed :hisp_ca_sfdmem_map. ret.%d\n",
			__func__, ret);
		dev->pool_mem[pool_num].sharefd = 0;
		dev->pool_mem[pool_num].size = 0;
	}

	return ret;
}

static int hisp_secpool_ca_unmap(unsigned int pool_num)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	if (dev->pool_mem[pool_num].sharefd <= 0 ||
		dev->pool_mem[pool_num].size == 0) {
		pr_err("[%s] Failed : sec memory has already mapped. fd.%d, size.%u\n",
			__func__, dev->pool_mem[pool_num].sharefd,
			dev->pool_mem[pool_num].size);
		return -EINVAL;
	}

	ret = hisp_ca_sfdmem_unmap(&dev->pool_mem[pool_num]);
	if (ret < 0)
		pr_err("[%s] Failed :hisp_ca_sfdmem_unmap. ret.%d\n",
			__func__, ret);

	dev->pool_mem[pool_num].sharefd = 0;
	dev->pool_mem[pool_num].size = 0;

	return ret;
}

static unsigned long hisp_sec_get_ispcpu_shrpa(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	return dev->atfshrd_paddr + dev->tsmem_offset[ISP_SHARE_MEM_OFFSET];
}

static int hisp_sec_vqmem_map(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	struct rproc *rproc = platform_get_drvdata(dev->pdev);
	u32 flag;
	int ret;

	hisp_rproc_get_vqmem(rproc,
		(u64 *)&dev->shrdmem->a7mapping[A7VQ].a7pa,
		(size_t *)&dev->shrdmem->a7mapping[A7VQ].size,
		&flag);

	ret = hisp_smc_vq_map();
	if (ret < 0) {
		pr_err("[%s] hisp_smc_vq_map fail.%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int hisp_sec_boot_init(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	u64 pgd_base;
	u64 remap = 0;
	unsigned long addr;
	int ret, index;

	if (!hisp_ca_boot_mode()) {
		for (index = 0; index < MAXA7MAPPING; index++) {
			addr = hisp_set_secmem_addr_pa(index,
					dev->shrdmem->a7mapping[index].a7va,
					dev->shrdmem->a7mapping[index].offset);
			if (addr != 0)
				dev->shrdmem->a7mapping[index].a7pa = addr;
		}

		remap = hisp_sec_get_remap_pa();
		if (remap == 0) {
			pr_err("[%s]ERR: remap addr.0 err!\n", __func__);
			return -ENODEV;
		}

		dev->shrdmem->sec_pgt = remap;
		dev->shrdmem->sec_mem_size = dev->trusted_mem_size;

		pgd_base = hisp_nsec_get_pgd();
		ret = hisp_pwr_core_sec_init(pgd_base);
		if (ret < 0) {
			pr_err("[%s] Failed : hispcore sec init.%d\n", __func__, ret);
			return ret;
		}

		ret = hisp_pwr_cpu_sec_init();
		if (ret < 0) {
			pr_err("[%s] Failed : hispcpu sec init.%d\n", __func__, ret);
			(void)hisp_pwr_core_sec_exit();
			return ret;
		}
	}

	return 0;
}

static void hisp_sec_boot_deinit(void)
{
	int ret = 0;

	if (!hisp_ca_boot_mode()) {
		ret = hisp_pwr_cpu_sec_exit();
		if (ret < 0)
			pr_err("[%s] Failed : cpu_sec_exit.%d\n", __func__, ret);

		ret = hisp_pwr_core_sec_exit();
		if (ret < 0)
			pr_err("[%s] Failed : core_sec_exit.%d\n", __func__, ret);
	}
}

static int hisp_sec_ispcpu_dst(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	struct rproc *rproc = platform_get_drvdata(dev->pdev);
	u32 flag;
	size_t size;
	int ret;

	if (!hisp_ca_boot_mode()) {
		ret = hisp_sec_vqmem_map();
		if (ret != 0)
			return ret;

		ret = hisp_pwr_cpu_sec_dst(dev->ispmem_reserved,
			dev->shrdmem->a7mapping[A7BOOT].a7pa);
		if (ret < 0) {
			pr_err("[%s] Failed : hispcpu sec dst.%d\n", __func__, ret);
			(void)hisp_smc_vq_unmap();
			return ret;
		}
	} else {
		dev->rsv_mem[HISP_SEC_VR0].pa = hisp_rproc_get_vringmem_pa(rproc, HISP_VRING0);
		dev->rsv_mem[HISP_SEC_VR1].pa = hisp_rproc_get_vringmem_pa(rproc, HISP_VRING1);
		hisp_rproc_get_vqmem(rproc, &dev->rsv_mem[HISP_SEC_VQ].pa, &size, &flag);
		dev->rsv_mem[HISP_SEC_VQ].size = (unsigned int)size;

		ret = hisp_pwr_cpu_ca_dst(dev->rsv_mem, HISP_SEC_RSV_MAX_TYPE);
		if (ret != 0) {
			pr_err("[%s] Failed : hispcpu ca dst.%d\n", __func__, ret);
			return ret;
		}
	}

	return 0;
}

static void hisp_sec_ispcpu_rst(void)
{
	int ret;

	if (!hisp_ca_boot_mode()) {
		ret = hisp_pwr_cpu_sec_rst();
		if (ret != 0)
			pr_err("[%s] Failed : hisp_pwr_cpu_sec_rst.%d\n",
				__func__, ret);

		ret = hisp_smc_vq_unmap();
		if (ret != 0)
			pr_err("[%s] Failed : hisp_smc_vq_unmap.%d\n",
				__func__, ret);
	} else {
		ret = hisp_pwr_cpu_ca_rst();
		if (ret != 0)
			pr_err("[%s] Failed : hisp_pwr_cpu_ca_rst.%d\n",
				__func__, ret);
	}
}

static int hisp_sec_device_do_enable(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	struct rproc *rproc = platform_get_drvdata(dev->pdev);
	int ret;

	ret = hisp_sec_boot_init();
	if (ret != 0)
		return ret;

	ret = hisp_wait_rpmsg_completion(rproc);
	if (ret != 0)
		goto err_loadfw;

	ret = hisp_sec_ispcpu_dst();
	if (ret != 0)
		goto err_loadfw;

	return 0;

err_loadfw:
	hisp_sec_boot_deinit();

	return ret;
}

unsigned int hisp_pool_mem_addr(unsigned int pool_num)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	unsigned int addr;

	mutex_lock(&dev->ca_mem_mutex);
	if (pool_num >= HISP_SEC_POOL_MAX_TYPE) {
		pr_err("[%s] Failed : pool num %u\n", __func__, pool_num);
		mutex_unlock(&dev->ca_mem_mutex);
		return 0;
	}

	addr = dev->pool_mem[pool_num].da;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] pool_num.%u iova.0x%x\n", __func__, pool_num, addr);
	return addr;
}

int hisp_secmem_size_get(unsigned int *trusted_mem_size)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] +", __func__);
	mutex_lock(&dev->ca_mem_mutex);
	if (dev->trusted_mem_size == 0) {
		pr_err("[%s] Failed : trusted mem size.%u\n",
				__func__, dev->trusted_mem_size);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	*trusted_mem_size = dev->trusted_mem_size;
	mutex_unlock(&dev->ca_mem_mutex);

	pr_info("[%s] -", __func__);

	return 0;
}
EXPORT_SYMBOL(hisp_secmem_size_get);

int hisp_secmem_pa_set(u64 sec_boot_phymem_addr, unsigned int trusted_mem_size)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] +", __func__);
	mutex_lock(&dev->ca_mem_mutex);
	if (dev->secmem_count > 0) {
		pr_err("[%s] Failed : secmem_count.%u\n",
				__func__, dev->secmem_count);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (sec_boot_phymem_addr == 0) {
		pr_err("[%s] Failed : sec_boot_phymem_addr is 0\n", __func__);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (trusted_mem_size < dev->trusted_mem_size) {
		pr_err("[%s] Failed : trusted_mem_size err.%u\n",
				__func__, trusted_mem_size);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dev->sec_boot_phymem_addr = sec_boot_phymem_addr;
	pr_debug("[%s] sec_boot_phymem_addr.0x%llx\n",
			__func__, dev->sec_boot_phymem_addr);
	dev->secmem_count++;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] - secmem_count.%u\n", __func__, dev->secmem_count);
	return 0;
}
EXPORT_SYMBOL(hisp_secmem_pa_set);

int hisp_secmem_pa_release(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	mutex_lock(&dev->ca_mem_mutex);
	if (dev->secmem_count <= 0) {
		pr_err("[%s] Failed : secmem_count.%u\n",
				__func__, dev->secmem_count);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}
	dev->secmem_count--;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] smem_count.%u\n", __func__, dev->secmem_count);
	return 0;
}
EXPORT_SYMBOL(hisp_secmem_pa_release);

int hisp_secmem_ca_map(unsigned int pool_num, int sharefd, unsigned int size)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret;

	pr_info("[%s] + %u", __func__, pool_num);

	mutex_lock(&dev->ca_mem_mutex);
	if (pool_num >= HISP_SEC_POOL_MAX_TYPE || sharefd < 0 || size == 0) {
		pr_err("[%s] Failed : pool num %u, sharefd.%d, size.%u\n",
			__func__, pool_num, sharefd, size);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if(!IS_ERR_OR_NULL(dev->pool_dmabuf[pool_num])) {
		pr_err("[%s] Failed :dev->pool_dmabuf not NULL!. type.%d\n",
			__func__, pool_num);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dev->pool_dmabuf[pool_num] = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(dev->pool_dmabuf[pool_num])) {
		pr_err("[%s] Failed : dma buf get fail. fd.%d\n",
			__func__, sharefd);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (dev->pool_dmabuf[pool_num]->size != size) {
		pr_err("[%s] Failed : size not equal. pool num.%d\n",
				__func__, pool_num);
		dma_buf_put(dev->pool_dmabuf[pool_num]);
		dev->pool_dmabuf[pool_num] = NULL;
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (pool_num == 0) {
		ret = hisp_dynpool_ca_map(sharefd, size);
		if (ret < 0) {
			pr_err("[%s] Failed :hisp_dynpool_ca_map. ret.%d\n",
				__func__, ret);
			goto map_err;
		}
	} else {
		ret = hisp_secpool_ca_map(sharefd, size, pool_num);
		if (ret < 0) {
			pr_err("[%s] Failed :secpool[%d] map. ret.%d\n",
				__func__, pool_num, ret);
			goto map_err;
		}
	}
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] -", __func__);
	return 0;
map_err:
	dma_buf_put(dev->pool_dmabuf[pool_num]);
	dev->pool_dmabuf[pool_num] = NULL;
	mutex_unlock(&dev->ca_mem_mutex);
	return ret;
}
EXPORT_SYMBOL(hisp_secmem_ca_map);

int hisp_secmem_ca_unmap(unsigned int pool_num)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	pr_info("[%s] + %u", __func__, pool_num);

	mutex_lock(&dev->ca_mem_mutex);
	if (pool_num >= HISP_SEC_POOL_MAX_TYPE) {
		pr_err("[%s] Failed : pool num %u\n", __func__, pool_num);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (pool_num == HISP_DYNAMIC_POOL) {
		ret = hisp_dynpool_ca_unmap();
		if (ret < 0)
			pr_err("[%s] Failed :hisp_dynpool_ca_unmap. ret.%d\n",
				__func__, ret);
	} else {
		ret = hisp_secpool_ca_unmap(pool_num);
		if (ret < 0)
			pr_err("[%s] Failed :secpool[%d]_unmap. ret.%d\n",
				__func__, pool_num, ret);
	}

	if(IS_ERR_OR_NULL(dev->pool_dmabuf[pool_num])) {
		pr_err("[%s] Failed :dev->pool_dmabuf is NULL. type.%d\n",
			__func__, pool_num);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dma_buf_put(dev->pool_dmabuf[pool_num]);
	dev->pool_dmabuf[pool_num] = NULL;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] -", __func__);
	return ret;
}
EXPORT_SYMBOL(hisp_secmem_ca_unmap);

int hisp_secboot_memsize_get_from_type(unsigned int type, unsigned int *size)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] +", __func__);

	mutex_lock(&dev->ca_mem_mutex);
	if (type >= HISP_SEC_BOOT_MAX_TYPE) {
		pr_err("[%s] Failed : wrong type.%u\n", __func__, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (dev->boot_mem[type].size == 0) {
		pr_err("[%s] Failed : boot_mem.size.%u, type.%u\n",
			__func__, dev->boot_mem[type].size, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	*size = dev->boot_mem[type].size;
	mutex_unlock(&dev->ca_mem_mutex);

	pr_info("[%s] -", __func__);
	return 0;
}
EXPORT_SYMBOL(hisp_secboot_memsize_get_from_type);

int hisp_secboot_info_set(unsigned int type, int sharefd)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] + %u", __func__, type);
	mutex_lock(&dev->ca_mem_mutex);
	if (sharefd < 0 || type >= HISP_SEC_BOOT_MAX_TYPE) {
		pr_err("[%s] Failed : sharefd.%d, type.%u\n",
			__func__, sharefd, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if(!IS_ERR_OR_NULL(dev->boot_dmabuf[type])) {
		pr_err("[%s] Failed :dev->boot_dmabuf not NULL!. type.%d\n",
			__func__, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dev->boot_dmabuf[type] = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(dev->boot_dmabuf[type])) {
		pr_err("[%s] Failed : dma buf get fail. fd.%d\n",
			__func__, sharefd);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	if (dev->boot_dmabuf[type]->size != dev->boot_mem[type].size) {
		pr_err("[%s] Failed : size not equal, type.%d\n", __func__, type);
		dma_buf_put(dev->boot_dmabuf[type]);
		dev->boot_dmabuf[type] = NULL;
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dev->boot_mem[type].sharefd = sharefd;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] -", __func__);
	return 0;
}
EXPORT_SYMBOL(hisp_secboot_info_set);

int hisp_secboot_info_release(unsigned int type)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	pr_info("[%s] +", __func__);
	mutex_lock(&dev->ca_mem_mutex);
	if (type >= HISP_SEC_BOOT_MAX_TYPE) {
		pr_err("[%s] Failed : type.%u\n",
			__func__, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dev->boot_mem[type].sharefd = 0;

	if(IS_ERR_OR_NULL(dev->boot_dmabuf[type])) {
		pr_err("[%s] Failed :dev->boot_dmabuf is NULL. type.%d\n",
			__func__, type);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	dma_buf_put(dev->boot_dmabuf[type]);
	dev->boot_dmabuf[type] = NULL;
	mutex_unlock(&dev->ca_mem_mutex);
	pr_info("[%s] -", __func__);
	return 0;
}
EXPORT_SYMBOL(hisp_secboot_info_release);

int hisp_secboot_prepare(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret, index;

	mutex_lock(&dev->ca_mem_mutex);
	if(dev->boot_mem_ready_flag) {
		pr_err("[%s] Failed : boot_mem_ready_flag.%d\n",
				__func__, dev->boot_mem_ready_flag);
		mutex_unlock(&dev->ca_mem_mutex);
		return -EINVAL;
	}

	for (index = HISP_SEC_TEXT; index < HISP_SEC_BOOT_MAX_TYPE; index++) {
		if (dev->boot_mem[index].sharefd == 0) {
			dev->boot_mem_ready_flag = 0;
			pr_err("[%s] Failed : boot type.%u, sharefd.%d\n",
				__func__, index, dev->boot_mem[index].sharefd);
			mutex_unlock(&dev->ca_mem_mutex);
			return -EINVAL;
		}
	}

	ret = hisp_ca_imgmem_config(dev->boot_mem, HISP_SEC_BOOT_MAX_TYPE);
	if (ret < 0) {
		dev->boot_mem_ready_flag = 0;
		pr_err("[%s] Failed :hisp_ca_imgmem_config. ret.%d\n",
			__func__, ret);
		mutex_unlock(&dev->ca_mem_mutex);
		return ret;
	}

	dev->boot_mem_ready_flag = 1;
	mutex_unlock(&dev->ca_mem_mutex);

	return 0;
}
EXPORT_SYMBOL(hisp_secboot_prepare);

int hisp_secboot_unprepare(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret, index;

	mutex_lock(&dev->ca_mem_mutex);

	for (index = HISP_SEC_TEXT; index < HISP_SEC_BOOT_MAX_TYPE; index++) {
		if (dev->boot_mem[index].sharefd == 0) {
			pr_err("[%s] boot type.%u, sharefd.%d is 0\n",
				__func__, index, dev->boot_mem[index].sharefd);
			mutex_unlock(&dev->ca_mem_mutex);
			return -EINVAL;
		}
	}

	ret = hisp_ca_imgmem_deconfig();
	if (ret < 0) {
		pr_err("[%s] Failed :hisp_ca_imgmem_deconfig. ret.%d\n",
			__func__, ret);
		mutex_unlock(&dev->ca_mem_mutex);
		return ret;
	}
	dev->boot_mem_ready_flag = 0;

	mutex_unlock(&dev->ca_mem_mutex);

	return 0;
}
EXPORT_SYMBOL(hisp_secboot_unprepare);

int hisp_sec_ta_enable(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	pr_info("[%s] +", __func__);

	mutex_lock(&dev->ta_status_mutex);
	if (dev->sec_ta_enable == 1) {
		pr_err("%s: ta already enable\n", __func__);
		mutex_unlock(&dev->ta_status_mutex);
		return -EINVAL;
	}

	ret = hisp_ca_ta_open();
	if (ret < 0) {
		pr_err("%s: fail: hisp_ca_ta_open\n", __func__);
		mutex_unlock(&dev->ta_status_mutex);
		return -EINVAL;
	}

	dev->sec_ta_enable = 1;
	mutex_unlock(&dev->ta_status_mutex);

	pr_info("[%s] -", __func__);

	return 0;
}
EXPORT_SYMBOL(hisp_sec_ta_enable);

int hisp_sec_ta_disable(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int index;
	int ret = 0;

	pr_info("[%s] +", __func__);

	mutex_lock(&dev->ta_status_mutex);
	if (dev->sec_poweron_status) {
		pr_err("%s: sec_poweron_status.%d\n", __func__,dev->sec_poweron_status);
		mutex_unlock(&dev->ta_status_mutex);
		return -EINVAL;
	}

	if (dev->sec_ta_enable == 0) {
		pr_err("%s: ta already disable\n", __func__);
		mutex_unlock(&dev->ta_status_mutex);
		return -EINVAL;
	}

	for (index = 0; index < HISP_SEC_POOL_MAX_TYPE; index++) {
		if (dev->pool_dmabuf[index] != NULL)
			(void)hisp_secmem_ca_unmap(index);
	}

	(void)hisp_secboot_unprepare();
	for (index = 0; index < HISP_SEC_BOOT_MAX_TYPE; index++)
		(void)hisp_secboot_info_release(index);

	ret = hisp_ca_ta_close();
	if (ret < 0) {
		pr_err("%s: fail: hisp_ca_ta_close\n", __func__);
		mutex_unlock(&dev->ta_status_mutex);
		return -EINVAL;
	}

	dev->sec_ta_enable = 0;
	mutex_unlock(&dev->ta_status_mutex);

	pr_info("[%s] -", __func__);
	return ret;
}
EXPORT_SYMBOL(hisp_sec_ta_disable);

int hisp_sec_pwron_prepare(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret;

	pr_info("[%s] +\n", __func__);
	ret = hisp_sec_set_share_pararms();
	if (ret < 0)
		pr_err("[%s] Failed : hisp_sec_set_share_pararms.%d\n", __func__, ret);

	ret = hisp_bsp_read_bin("isp_firmware", dev->rsctable_offset,
			dev->rsctable_size, dev->rsctable_vaddr_const);
	if (ret < 0) {
		pr_err("[%s] hisp_bsp_read_bin.%d\n", __func__, ret);
		return ret;
	}
	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hisp_sec_boot(void)
{
	int ret, err_ret;

	ret = hisp_pwr_subsys_powerup();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_pwr_subsys_powerup.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_ispcpu_qos_cfg();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_ispcpu_qos_cfg.%d\n", __func__, ret);
		goto err_ispinit;
	}

	ret = hisp_sec_device_do_enable();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_sec_device_do_enable.%d.\n",
				__func__, ret);
		goto err_ispinit;
	}

	return 0;

err_ispinit:
	err_ret = hisp_pwr_subsys_powerdn();
	if (err_ret < 0)
		pr_err("[%s]Fail:hisp_pwr_subsys_powerdn.%d\n", __func__, err_ret);

	return ret;
}

int hisp_sec_device_enable(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	if(hisp_ca_boot_mode() && (dev->boot_mem_ready_flag != 1)) {
		pr_err("[%s] Failed : boot_mem_ready_flag.%d\n",
			__func__, dev->boot_mem_ready_flag);
		return -EINVAL;
	}

	if (hisp_ca_boot_mode()) {
		mutex_lock(&dev->ta_status_mutex);
		if (dev->sec_ta_enable == 1) {
			dev->sec_poweron_status = 1;
		} else {
			pr_err("[%s] Failed : sec_ta_enable status=%d\n",
							__func__, dev->sec_ta_enable);
			mutex_unlock(&dev->ta_status_mutex);
			return -1;
		}
		mutex_unlock(&dev->ta_status_mutex);
	}

	mutex_lock(&dev->pwrlock);
	ret = hisp_sec_boot();
	if (ret != 0)
		pr_err("[%s] Failed : hisp_sec_boot.%d\n", __func__, ret);
	mutex_unlock(&dev->pwrlock);

	return ret;
}

int hisp_sec_device_disable(void)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret;

	mutex_lock(&dev->pwrlock);
	hisp_sec_ispcpu_rst();
	hisp_sec_boot_deinit();

	ret = hisp_pwr_subsys_powerdn();
	if (ret < 0)
		pr_err("[%s] Failed : ispsrtdn.%d\n", __func__, ret);

	mutex_unlock(&dev->pwrlock);

	if (hisp_ca_boot_mode()) {
		mutex_lock(&dev->ta_status_mutex);
		dev->sec_poweron_status = 0;
		mutex_unlock(&dev->ta_status_mutex);
		ret = hisp_sec_ta_disable();
		if (ret < 0)
			pr_err("[%s] Failed : hisp_sec_ta_disable.%d.\n",
					__func__, ret);
	}

	return 0;
}

static int hisp_sec_meminfo_init(struct platform_device *pdev)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret;

	if (pdev == NULL) {
		pr_err("[%s] Failed : platform_device is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = memset_s(dev, sizeof(struct hisp_sec),
		0, sizeof(struct hisp_sec));
	if (ret < 0) {
		pr_err("[%s] Failed : memset_s dev.%d.\n",
			__func__, ret);
		return -ENOMEM;
	}

	dev->device = &pdev->dev;
	dev->pdev = pdev;

	ret = memset_s(dev->rsv_mem,
		HISP_SEC_RSV_MAX_TYPE * sizeof(struct hisp_ca_meminfo),
		0, HISP_SEC_RSV_MAX_TYPE * sizeof(struct hisp_ca_meminfo));
	if (ret < 0) {
		pr_err("[%s] Failed : memset_s dev->rsv_mem.%d.\n",
			__func__, ret);
		return -ENOMEM;
	}

	ret = memset_s(dev->boot_mem,
		HISP_SEC_BOOT_MAX_TYPE * sizeof(struct hisp_ca_meminfo),
		0, HISP_SEC_BOOT_MAX_TYPE * sizeof(struct hisp_ca_meminfo));
	if (ret < 0) {
		pr_err("[%s] Failed : memset_s dev->boot_mem.%d.\n",
			__func__, ret);
		return -ENOMEM;
	}

	ret = memset_s(dev->pool_mem,
		HISP_SEC_POOL_MAX_TYPE * sizeof(struct hisp_ca_meminfo),
		0, HISP_SEC_POOL_MAX_TYPE * sizeof(struct hisp_ca_meminfo));
	if (ret < 0) {
		pr_err("[%s] Failed : memset_s dev->pool_mem.%d.\n",
			__func__, ret);
		return -ENOMEM;
	}

	return 0;
}

static int hisp_sec_rsvmem_getdts(int *flag)
{
	struct device_node *nod = NULL;
	const char *status = NULL;

	if (flag == NULL) {
		pr_err("[%s] input flag parameter is NULL!\n", __func__);
		return -ENODEV;
	}
	*flag = 0;

	nod = of_find_node_by_path("/reserved-memory/sec_camera");
	if (nod  == NULL) {/*lint !e838 */
		pr_err("[%s] Failed : of_find_node_by_path.%pK\n",
				__func__, nod);
		return -ENODEV;
	}

	if (of_property_read_string(nod, "status", &status)) {
		pr_err("[%s] Failed : of_property_read_string status\n",
				__func__);
		*flag = 1;
		return -ENODEV;
	}

	if (status && strncmp(status, "disabled", strlen("disabled")) != 0)
		*flag = 1;

	pr_err("[%s] trusted_ispmem_reserved %s\n", __func__, status);
	return 0;
}

static int hisp_sec_rsctable_getdts(struct device_node *np,
			struct hisp_sec *dev)
{
	int ret = 0;

	ret = of_property_read_u32(np, "rsctable-mem-offet",
			(unsigned int *)(&dev->rsctable_offset));
	if (ret < 0) {
		pr_err("[%s] Failed: rsctable_offset.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("rsctable_offset.0x%x of_property_read_u32.%d\n",
			dev->rsctable_offset, ret);

	ret = of_property_read_u32(np, "rsctable-mem-size",
			(unsigned int *)(&dev->rsctable_size));
	if (ret < 0) {
		pr_err("[%s] Failed: rsctable_size.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("rsctable_size.0x%x of_property_read_u32.%d\n",
			dev->rsctable_size, ret);

	return 0;
}

static int hisp_sec_trustedmem_getdts(struct device_node *np,
			struct hisp_sec *dev)
{
	unsigned int offset_num   = 0;
	unsigned int index = 0;
	int ret = 0;

	if (!hisp_ca_boot_mode()) {
		ret = of_property_read_u32(np, "trusted-smem-size",
				(unsigned int *)(&dev->trusted_mem_size));
		if (ret < 0) {
			pr_err("[%s] Failed: trusted_mem_size.ret.%d\n",
					__func__, ret);
			return -EINVAL;
		}
		pr_info("trusted_mem_size.0x%x of_property_read_u32.%d\n",
				dev->trusted_mem_size, ret);
	}

	ret = of_property_read_u32(np, "share-smem-size",
			(unsigned int *)(&dev->share_mem_size));
	if (ret < 0) {
		pr_err("[%s] Failed: share_mem_size.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("share_mem_size.0x%x of_property_read_u32.%d\n",
			dev->share_mem_size, ret);

	ret = of_property_read_u32(np, "trusted-smem-num",
			(unsigned int *)(&offset_num));
	if (ret < 0) {
		pr_err("[%s] Failed: tsmem-offset-num.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("tsmem-offset-num.0x%x of_property_read_u32.%d\n",
			offset_num, ret);

	if (offset_num <= ISP_TSMEM_MAX) {
		ret = of_property_read_u32_array(np, "trusted-smem-offset",
					dev->tsmem_offset, offset_num);
		if (ret < 0) {
			pr_info("[%s] Fail : trusted-smem-offset.ret.%d\n",
					__func__, ret);
			return -EINVAL;
		}

		for (index = 0; index < offset_num; index++)
			pr_info("[%s] trusted-smem-offset %d offest = 0x%x\n",
				__func__, index, dev->tsmem_offset[index]);
	}

	ret = hisp_sec_rsctable_getdts(np, dev);
	if (ret < 0)
		pr_err("[%s] Failed: rsctable_getdts.ret.%d\n", __func__, ret);

	return ret;
}

static int hisp_sec_early_getdts(struct platform_device *pdev,
			struct hisp_sec *dev)
{
	struct device *device = &pdev->dev;
	struct device_node *np = device->of_node;
	int ret = 0;

	ret = hisp_sec_rsvmem_getdts(&dev->ispmem_reserved);
	if (ret < 0)
		pr_err("[%s] Failed : ispmem_reserved.%d.\n", __func__, ret);
	pr_info("[%s] ispmem_reserved.%d\n", __func__, dev->ispmem_reserved);

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "use-ca-ta", &dev->use_ca_ta);
	if (ret < 0) {
		pr_err("Failed : use_ca_ta.%d.ret.%d\n", dev->use_ca_ta, ret);
		return -EINVAL;
	}
	pr_info("use_ca_ta.%d of_property_read_u32.%d\n", dev->use_ca_ta, ret);

	ret = of_property_read_u32(np, "isp-iova-start",
		(unsigned int *)(&dev->isp_iova_start));
	if (ret < 0) {
		pr_err("Failed : isp_iova_addr.0x%lx.ret.%d\n",
			dev->isp_iova_start, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "isp-iova-size",
		(unsigned int *)(&dev->isp_iova_size));
	if (ret < 0) {
		pr_err("Failed : isp_iova_size.0x%lx.ret.%d\n",
			dev->isp_iova_size, ret);
		return -EINVAL;
	}

	pr_info("isp_iova_addr.0x%lx isp_iova_size.0x%lx\n",
			dev->isp_iova_start, dev->isp_iova_size);

	ret = hisp_sec_trustedmem_getdts(np, dev);
	if (ret < 0) {
		pr_err("Failed : hisp_sec_trustedmem_getdts.%d\n", ret);
		return ret;
	}
	return 0;
}

static int hisp_sec_getdts(struct platform_device *pdev, struct hisp_sec *dev)
{
	struct device *device = &pdev->dev;
	struct device_node *np = device->of_node;
	int ret = 0;

	ret = hisp_sec_early_getdts(pdev, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: isp_atf_mem_getdts.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "mapping-items",
			(unsigned int *)(&dev->mapping_items));
	if (ret < 0) {
		pr_err("[%s] Failed: mapping-num.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("mapping-items.0x%x.ret.%d\n", dev->mapping_items, ret);

	return 0;
}

static int hisp_sec_orimem_getdts(struct hisp_sec *dev,
		unsigned int etype, unsigned long paddr)
{
	struct device_node *np = dev->device->of_node;
	int ret = 0;

	if ((np == NULL) || (dev->shrdmem == NULL) || (etype >= MAXA7MAPPING)) {
		pr_err("[%s] Failed: np.%pK, shrdmem.%pK, etype.0x%x >= 0x%x\n",
			__func__, np, dev->shrdmem, etype, MAXA7MAPPING);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, secmem_propname[etype],
		(unsigned int *)(&dev->shrdmem->a7mapping[etype]),
		dev->mapping_items);
	if (ret < 0) {
		pr_info("[%s] propname.%s, of_property_read_u32_array.%d\n",
				__func__, secmem_propname[etype], ret);
		return -EINVAL;
	}

	if (etype == A7VQ)
		dev->shrdmem->a7mapping[etype].reserve = 0;

	pr_info("[%s] propname.%s, array.%d.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
		__func__, secmem_propname[etype], ret,
		dev->shrdmem->a7mapping[etype].a7va,
		dev->shrdmem->a7mapping[etype].size,
		dev->shrdmem->a7mapping[etype].prot,
		dev->shrdmem->a7mapping[etype].offset,
		dev->shrdmem->a7mapping[etype].reserve);

	return 0;
}

static int hisp_ca_bootmem_getdts(struct hisp_sec *dev,
		unsigned int etype)
{
	struct device_node *np = dev->device->of_node;
	int ret = 0;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -EINVAL;
	}

	if (etype >= HISP_SEC_BOOT_MAX_TYPE) {
		pr_err("[%s] Failed : etype.(0x%x >= 0x%x)\n",
			__func__, etype, HISP_SEC_BOOT_MAX_TYPE);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, ca_bootmem_propname[etype],
		(unsigned int *)(&dev->boot_mem[etype]), dev->mapping_items);
	if (ret < 0) {
		pr_info("[%s] propname.%s, of_property_read_u32_array.%d\n",
			__func__, ca_bootmem_propname[etype], ret);
		return -EINVAL;
	}

	pr_info("[%s] propname.%s, array.%d.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
			__func__, ca_bootmem_propname[etype], ret,
			dev->boot_mem[etype].type, dev->boot_mem[etype].da,
			dev->boot_mem[etype].size, dev->boot_mem[etype].prot,
			dev->boot_mem[etype].sec_flag);

	return 0;
}

static int hisp_ca_rsvmem_getdts(struct hisp_sec *dev,
		unsigned int etype)
{
	struct device_node *np = dev->device->of_node;
	int ret = 0;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -EINVAL;
	}

	if (etype >= HISP_SEC_RSV_MAX_TYPE) {
		pr_err("[%s] Failed : etype.(0x%x >= 0x%x)\n",
			__func__, etype, HISP_SEC_RSV_MAX_TYPE);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, ca_rsvmem_propname[etype],
		(unsigned int *)(&dev->rsv_mem[etype]), dev->mapping_items);
	if (ret < 0) {
		pr_info("[%s] propname.%s, of_property_read_u32_array.%d\n",
			__func__, ca_rsvmem_propname[etype], ret);
		return -EINVAL;
	}

	pr_info("[%s] propname.%s, array.%d.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
			__func__, ca_rsvmem_propname[etype], ret,
			dev->rsv_mem[etype].type, dev->rsv_mem[etype].da,
			dev->rsv_mem[etype].size, dev->rsv_mem[etype].prot,
			dev->rsv_mem[etype].sec_flag);

	return 0;
}

static int hisp_ca_mempool_getdts(struct hisp_sec *dev,
		unsigned int etype)
{
	struct device_node *np = dev->device->of_node;
	int ret = 0;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -EINVAL;
	}

	if (etype >= HISP_SEC_POOL_MAX_TYPE) {
		pr_err("[%s] Failed : etype.(0x%x >= 0x%x)\n",
			__func__, etype, HISP_SEC_POOL_MAX_TYPE);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, ca_mempool_propname[etype],
		(unsigned int *)(&dev->pool_mem[etype]), dev->mapping_items);
	if (ret < 0) {
		pr_info("[%s] propname.%s, of_property_read_u32_array.%d\n",
				__func__, ca_mempool_propname[etype], ret);
		return -EINVAL;
	}
	pr_info("[%s] propname.%s, array.%d.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
			__func__, ca_mempool_propname[etype], ret,
			dev->pool_mem[etype].type, dev->pool_mem[etype].da,
			dev->pool_mem[etype].size, dev->pool_mem[etype].prot,
			dev->pool_mem[etype].sec_flag);

	return 0;
}

static int hisp_sec_mem_getdts(struct hisp_sec *dev)
{
	int index;
	int ret;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EPERM;
	}

	if (!hisp_ca_boot_mode()) {
		for (index = 0; index < MAXA7MAPPING; index++) {
			ret = hisp_sec_orimem_getdts(dev, index, 0);
			if (ret < 0)
				pr_err("[%s] Failed: hisp_meminit.%d\n",
					__func__, ret);
		}
	} else {
		for (index = 0; index < (HISP_SEC_BOOT_MAX_TYPE); index++) {
			ret = hisp_ca_bootmem_getdts(dev, index);
			if (ret < 0)
				pr_err("[%s] Failed: ca_bootmem_getdts.%d\n",
					__func__, ret);
		}

		for (index = 0; index < HISP_SEC_RSV_MAX_TYPE; index++) {
			ret = hisp_ca_rsvmem_getdts(dev, index);
			if (ret < 0)
				pr_err("[%s] Failed: ca_rsvmem_getdts.%d\n",
					__func__, ret);
		}

		for (index = 0; index < HISP_SEC_POOL_MAX_TYPE; index++) {
			ret = hisp_ca_mempool_getdts(dev, index);
			if (ret < 0)
				pr_err("[%s] Failed: ca_mempool_getdts.%d\n",
					__func__, ret);
		}
	}

	return 0;
}

static void hisp_sec_get_meminfo(struct hisp_sec *dev)
{
	unsigned int offset = 0;
	int ret;

	ret = hisp_sec_mem_getdts(dev);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_sec_mem_getdts.%d.\n", __func__, ret);
		return;
	}

	if (!hisp_ca_boot_mode()) {
		dev->shrdmem->a7mapping[A7RDR].a7pa = hisp_rdr_addr_get();
		dev->shrdmem->a7mapping[A7SHARED].a7pa = hisp_sec_get_ispcpu_shrpa();
		offset = dev->tsmem_offset[ISP_DYN_MEM_OFFSET];
		dev->shrdmem->a7mapping[A7DYNAMIC].a7pa
			= dev->atfshrd_paddr + offset;
		dev->ap_dyna_array = offset + dev->atfshrd_vaddr;
		dev->ap_dyna = &dev->shrdmem->a7mapping[A7DYNAMIC];
		dev->is_heap_flag = dev->shrdmem->a7mapping[A7HEAP].reserve;
		dev->shrdmem->a7mapping[A7SHARED].apva = hisp_get_ispcpu_shrmem_secva();
	} else {
		dev->rsv_mem[HISP_SEC_RDR].pa = hisp_rdr_addr_get();
		dev->rsv_mem[HISP_SEC_SHARE].pa = hisp_sec_get_ispcpu_shrpa();
	}
}

int hisp_sec_probe(struct platform_device *pdev)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	int ret = 0;

	ret = hisp_sec_meminfo_init(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : dev_info_init.%d.\n", __func__, ret);
		return -EINVAL;
	}

	ret = hisp_sec_getdts(pdev, dev);
	if (ret < 0) {
		pr_err("[%s] Failed : isp_atf_getdts.%d.\n", __func__, ret);
		return -EINVAL;
	}

	ret = hisp_smc_shrmem_init(dev);
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_smc_shrmem_init.%d.\n", __func__, ret);
		return -ENOMEM;
	}

	ret = hisp_sec_rsctablemem_init(dev);
	if (ret < 0) {
		pr_err("[%s] Failed: rsctablemem_init.%d.\n", __func__, ret);
		goto hisp_sec_rsctablemem_init_fail;
	}

	hisp_sec_get_meminfo(dev);

	hisp_ca_probe();

	ret = hisp_mempool_init(dev->isp_iova_start, dev->isp_iova_size);
	if (ret < 0) {
		pr_err("Failed : hisp_mempool_init.%d\n", ret);
		goto iommu_get_domain_for_dev_fail;
	}

	mutex_init(&dev->pwrlock);
	mutex_init(&dev->ca_mem_mutex);
	mutex_init(&dev->ta_status_mutex);

	return 0;

iommu_get_domain_for_dev_fail:
	hisp_ca_remove();
	hisp_sec_rsctablemem_exit(dev);

hisp_sec_rsctablemem_init_fail:
	hisp_smc_shrmem_exit(dev);

	return ret;
}

int hisp_sec_remove(struct platform_device *pdev)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;

	hisp_mempool_exit();
	hisp_ca_remove();
	mutex_destroy(&dev->pwrlock);
	mutex_destroy(&dev->ca_mem_mutex);
	mutex_destroy(&dev->ta_status_mutex);
	hisp_sec_rsctablemem_exit(dev);
	hisp_smc_shrmem_exit(dev);

	return 0;
}

MODULE_DESCRIPTION("ISP sec module");
MODULE_LICENSE("GPL");

#ifdef DEBUG_HISP
ssize_t hisp_sec_test_regs_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	struct hisp_sec *dev = (struct hisp_sec *)&hisp_sec_dev;
	char *s = buf;

	s += snprintf_s(s, SEC_MAX_SIZE, SEC_MAX_SIZE-1,
		"BoardID: 0x%x\n", dev->boardid);

	return (s - buf);
}

#endif
