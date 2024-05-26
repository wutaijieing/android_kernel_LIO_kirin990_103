/*
 *  driver, hisp_rproc.c
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include <linux/miscdevice.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <global_ddr_map.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <securec.h>
#include "hisp_internel.h"
#include "isp_ddr_map.h"

#define ISP_MEM_SIZE              0x10000
#define ISP_BOOT_MEM_ATTR         (IOMMU_EXEC | IOMMU_READ)
#define PART_ISPFW_SIZE           0x00E00000
#define ISP_BIN_RSC_OFFSET        0x3000
#define HISP_BW_SHRMEM_OFFSET     0xF000

struct hisp_nsec {
	int loadbin;
	unsigned int isp_bin_state;
	unsigned int vqda;
	dma_addr_t isp_bw_pa;
	u64 pgd_base;
	void *isp_bw_va;
	void *isp_bin_vaddr;
	struct device *device;
	struct platform_device *isp_pdev;
	struct task_struct *loadispbin;
#ifdef DEBUG_HISP
	struct isp_pcie_cfg *isp_pcie_cfg;
#endif
	struct mutex pwrlock;
};

static struct hisp_nsec hisp_nsec_dev;

#ifdef DEBUG_HISP
#include "hisp_pcie.h"

static struct isp_pcie_cfg nsec_isp_pcie_cfg;

struct isp_pcie_cfg *hisp_get_pcie_cfg(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	if (dev->isp_pcie_cfg == NULL) {
		pr_err("[%s] Failed: isp pcie cfg not set\n", __func__);
		return NULL;
	}

	return dev->isp_pcie_cfg;
}

int hisp_check_pcie_stat(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	if (dev->isp_pcie_cfg->isp_pci_inuse_flag != ISP_PCIE_MODE)
		return ISP_NORMAL_MODE;

	if (dev->isp_pcie_cfg->isp_pci_ready != ISP_PCIE_READY) {
		pr_err("[%s] isp in pcie mode, but pcie not ready\n", __func__);
		return ISP_PCIE_NREADY;
	}
	pr_info("[%s] isp pcie inuse, status ready\n", __func__);
	return ISP_PCIE_MODE;
}

static int hisp_pcie_getdts(struct device_node *np)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret;

	ret = of_property_read_u32(np, "isp-usepcie-flag", &dev->isp_pcie_cfg->isp_pci_inuse_flag);
	if (ret < 0)
		pr_err("[%s] Failed: isp_pci_inuse_flag.%d\n", __func__, ret);

	pr_info("enable_pcie_flag.0x%x of_property_read_u32.%d\n",
			dev->isp_pcie_cfg->isp_pci_inuse_flag, ret);

	return 0;
}
#endif

int hisp_nsec_bin_mode(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	return dev->loadbin;
}

int hisp_wakeup_binload_kthread(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int timeout = 1000;

	pr_info("[%s] : +\n", __func__);

	if (hisp_nsec_bin_mode() == 0)
		return 0;

	if (dev->isp_bin_state == 1)
		return 0;

	if (dev->loadispbin == NULL) {
		pr_err("%s:Failed : loadispbin is NULL\n", __func__);
		return -ENOMEM;
	}

	wake_up_process(dev->loadispbin);
	do {
		timeout--;
		mdelay(10);
		if (dev->isp_bin_state == 1)
			break;
	} while (timeout > 0);

	pr_info("[%s] : isp_bin_state.%d, timeout.%d\n",
		__func__, dev->isp_bin_state, timeout);

	return timeout > 0 ? 0 : -1;
}

u64 hisp_nsec_get_remap_pa(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	if (dev->isp_bw_pa)
		return dev->isp_bw_pa;

	return 0;
}

u64 hisp_get_ispcpu_shrmem_nsecpa(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	return (dev->isp_bw_pa + HISP_BW_SHRMEM_OFFSET);
}

void *hisp_get_ispcpu_shrmem_nsecva(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	return (dev->isp_bw_va + HISP_BW_SHRMEM_OFFSET);
}

#ifdef DEBUG_HISP
static int hisp_update_elf_load(char **buffer, int *length)
{
	int ret = -1;
	char pathname[128] = { 0 };
	struct file *fp = NULL;
	mm_segment_t fs;
	loff_t pos = 0;
	struct kstat m_stat;

	if (buffer == NULL)
		return -EINVAL;

	ret = snprintf_s(pathname, sizeof(pathname), sizeof(pathname) - 1, "%s",
		"/system/vendor/firmware/isp_fw.elf");
	if (ret < 0) {
		pr_err("[%s]:snprintf_s firmware failed.%d\n", __func__, ret);
		return ret;
	}

	/*get resource*/
	fp = filp_open(pathname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("[%s] : filp_open(%s) failed\n", __func__, pathname);
		return -ENOENT;
	}

	*buffer = vmalloc(PART_ISPFW_SIZE);
	if (*buffer == NULL)
		goto err;

	ret = vfs_llseek(fp, 0, SEEK_SET);
	if (ret < 0)
		goto err;

	fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501 */

	ret = vfs_stat(pathname, &m_stat);
	if (ret < 0) {
		pr_err("[%s]Failed :%s vfs_stat:%d\n", __func__, pathname, ret);
		set_fs(fs);
		goto err;
	}
	*length = m_stat.size;

	pos = fp->f_pos;/*lint !e613 */
	ret = vfs_read(fp, (char __user *)*buffer, *length, &pos);
	if (ret != *length) {
		pr_err("[%s] failed: ret.%d, len.%d\n", __func__, ret, *length);
		set_fs(fs);
		goto err;
	}
	set_fs(fs);

	filp_close(fp, NULL);/*lint !e668 */

	return 0;

err:
	filp_close(fp, NULL);/*lint !e668 */
	return -EPERM;
}

static int hisp_elf_seg_check(u32 da, u32 memsz, u32 filesz,
		u32 offset, int fw_size)
{
	pr_debug("phdr: da 0x%x memsz 0x%x filesz 0x%x\n",
			da, memsz, filesz);

	if (filesz > memsz) {
		pr_err("[%s] failed : bad phdr filesz 0x%x memsz 0x%x\n",
			__func__, filesz, memsz);
		return -EINVAL;
	}
	if (offset + filesz > (u32)fw_size) {
		pr_err("[%s] failed : truncated fw: need 0x%x avail 0x%x\n",
			__func__, offset + filesz, fw_size);
		return -EINVAL;
	}

	return 0;
}

static int hisp_loadbin_elf_load_segments(const u8 *data, void *dst, int fw_size)
{
	int i;
	int ret = 0;
	u32 start_addr = 0;
	struct elf32_hdr *ehdr = NULL;
	struct elf32_phdr *phdr = NULL;

	ehdr = (struct elf32_hdr *)data;
	phdr = (struct elf32_phdr *)(data + ehdr->e_phoff);

	start_addr = phdr->p_paddr;
	pr_debug("[%s] start_addr.0x%x\n", __func__, start_addr);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		u32 da = phdr->p_paddr;
		u32 memsz = phdr->p_memsz;
		u32 filesz = phdr->p_filesz;
		u32 offset = phdr->p_offset;
		void *ptr = NULL;

		if (phdr->p_type != PT_LOAD)
			continue;

		ret = hisp_elf_seg_check(da, memsz, filesz, offset, fw_size);
		if (ret < 0) {
			pr_err("[%s]:hisp_elf_seg_check failed\n", __func__);
			break;
		}

		/* grab the kernel address for this device address */
		ptr = dst + da - start_addr;
		if (ptr == NULL) {
			pr_err("[%s] : bad phdr da 0x%x mem 0x%x\n",
				__func__, da, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz) {
			ret = memcpy_s(ptr, filesz,
					data + phdr->p_offset, filesz);
			if (ret != 0) {
				pr_err("memcpy_s isp elf_data fail.%d\n", ret);
				break;
			}
		}

		/*
		 * Zero out remaining memory for this segment.
		 *
		 * This isn't strictly required since dma_alloc_coherent already
		 * did this for us. albeit harmless, we may consider removing
		 * this.
		 */
		if (memsz > filesz) {
			ret = memset_s(ptr + filesz, memsz - filesz,
				0, memsz - filesz);
			if (ret != 0) {
				pr_err("ptr + filesz to 0 fail.%d\n", ret);
				break;
			}
		}
	}

	return ret;
}

static int hisp_loadbin_elf_load(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret = -1;
	int length = 0;
	void *isp_elf_vaddr = NULL;

	pr_info("[%s] : +\n", __func__);

	if (dev->isp_bin_vaddr == NULL) {
		pr_err("[%s] Failed : isp_bin_vaddr.NULL\n", __func__);
		return -ENOMEM;
	}

	ret = hisp_update_elf_load((char **)&isp_elf_vaddr, &length);
	if (ret < 0) {
		if (isp_elf_vaddr != NULL)
			vfree(isp_elf_vaddr);
		pr_err("[%s] Failed : hisp_update_elf_load\n", __func__);
		return ret;
	}

	ret = hisp_loadbin_elf_load_segments(isp_elf_vaddr,
				dev->isp_bin_vaddr, length);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_loadbin_elf_load_segments\n", __func__);
		pr_err("[%s] isp_elf_vaddr.0x%pK\n", __func__, isp_elf_vaddr);
		pr_err("[%s] bin_va.0x%pK\n", __func__, dev->isp_bin_vaddr);
		pr_err("[%s] length.0x%x\n", __func__, length);
	}
	vfree(isp_elf_vaddr);

	pr_info("[%s] : -\n", __func__);
	return ret;
}
#endif

int hisp_request_nsec_rsctable(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	pr_info("[%s] +\n", __func__);
	if (dev->isp_bin_vaddr == NULL) {
		pr_err("[%s] isp_bin_vaddr.%pK\n", __func__,
				dev->isp_bin_vaddr);
		return -EINVAL;
	}

	rproc->cached_table = kmemdup(dev->isp_bin_vaddr + ISP_BIN_RSC_OFFSET,
					SZ_4K, GFP_KERNEL);
	if (!rproc->cached_table)
		return -ENOMEM;

	rproc->table_ptr = rproc->cached_table;
	rproc->table_sz = SZ_4K;

	pr_info("[%s] -\n", __func__);

	return 0;
}

int hisp_loadbin_debug_elf(struct rproc *rproc)
{
#ifdef DEBUG_HISP
	int err = 0;

	if (!hisp_nsec_bin_mode())
		return err;

	if (!hisp_nsec_boot_mode())
		return err;

	err = hisp_loadbin_elf_load();
	if (err < 0) {
		pr_err("[%s] Failed : hisp_loadbin_elf_load.0x%x\n", __func__, err);
		if (err != -ENOENT)
			return err;
	}

	rproc->state = RPROC_OFFLINE;
	if (rproc->table_ptr) {
		rproc->table_ptr = NULL;
		rproc->table_sz = 0;
	}
#endif

	return 0;
}

int hisp_loadbin_segments(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret = 0;
	void *ptr = NULL;
	bool is_iomem = 0;

	/* go through the available ELF segments */
	u64 da = TEXT_BASE;
	size_t memsz = ISP_TEXT_SIZE;

	pr_info("[%s] : +\n", __func__);
	/* grab the kernel address for this device address */
	ptr = rproc_da_to_va(rproc, da, memsz, &is_iomem);
	pr_info("[%s] : text section ptr = %pK\n", __func__, ptr);
	if (ptr == NULL) {
		pr_err("[%s]:bad phdr da 0x%llx mem 0x%lx\n", __func__, da, memsz);
		ret = -EINVAL;
		return ret;
	}
	/* put the segment where the remote processor expects it */
	ret = memcpy_s(ptr, memsz, dev->isp_bin_vaddr, memsz);
	if (ret != 0) {
		pr_err("%s:memcpy text vaddr to ptr fail.%d\n", __func__, ret);
		return ret;
	}

	/* go through the available ELF segments */
	da = DATA_BASE;
	memsz = ISP_BIN_DATA_SIZE;
	ptr = rproc_da_to_va(rproc, da, memsz, &is_iomem);
	pr_info("[%s] : data section ptr = %pK\n", __func__, ptr);
	if (ptr == NULL) {
		pr_err("%s:bad phdr da 0x%llx mem 0x%lx\n", __func__, da, memsz);
		ret = -EINVAL;
		return ret;
	}
	/* put the segment where the remote processor expects it */
	ret = memcpy_s(ptr, memsz,
			dev->isp_bin_vaddr + ISP_TEXT_SIZE, memsz);
	if (ret != 0) {
		pr_err("%s:memcpy data vaddr to ptr fail.%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

void hisp_nsec_boot_dump(struct rproc *rproc, unsigned int size)
{
#ifdef DEBUG_HISP
	void *va = NULL;
	unsigned int i = 0;

	if (hisp_sec_boot_mode())
		return;

	va = hisp_rproc_get_imgva(rproc);
	pr_info("[%s] addr.0x%pK, size.0x%x\n", __func__, va, size);
	for (i = 0; i < size; i += 4)
		pr_info("0x%08x 0x%08x 0x%08x 0x%08x\n",
			*((unsigned int *)va + i + 0), *((unsigned int *)va + i + 1),/*lint !e737 !e835 */
			*((unsigned int *)va + i + 2), *((unsigned int *)va + i + 3));/*lint !e737 */
#endif
}

int hisp_nsec_jpeg_powerup(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret = 0;

	pr_info("[%s] +\n", __func__);
	mutex_lock(&dev->pwrlock);
	ret = hisp_pwr_subsys_powerup();
	if (ret != 0) {
		pr_err("[%s] Failed : ispup.%d\n", __func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	ret = hisp_ispcpu_qos_cfg();
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_ispcpu_qos_cfg.%d\n",
				__func__, ret);
		goto isp_down;
	}
	hisp_smc_set_nonsec();

	ret = hisp_pwr_core_nsec_init(dev->pgd_base);
	if (ret != 0) {
		pr_err("[%s] hisp_pwr_core_nsec_init.%d\n", __func__, ret);
		goto isp_down;
	}
	mutex_unlock(&dev->pwrlock);
	pr_info("[%s] -\n", __func__);

	return 0;

isp_down:
	if ((hisp_pwr_subsys_powerdn()) != 0)
		pr_err("[%s] Failed : ispdn\n", __func__);

	mutex_unlock(&dev->pwrlock);
	pr_info("[%s] -\n", __func__);

	return ret;
}

/*lint -save -e631 -e613*/
int hisp_nsec_jpeg_powerdn(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret;

	pr_info("%s: +\n", __func__);
	mutex_lock(&dev->pwrlock);
	ret = hisp_pwr_core_nsec_exit();
	if (ret != 0)
		pr_err("%s: hisp_pwr_core_nsec_exit failed, ret.%d\n",
			__func__, ret);

	ret = hisp_pwr_subsys_powerdn();
	if (ret != 0)
		pr_err("%s: hisp_pwr_subsys_powerdn failed, ret.%d\n",
			__func__, ret);

	mutex_unlock(&dev->pwrlock);
	pr_info("%s: -\n", __func__);

	return 0;
}
/*lint -restore */

static int hisp_nsec_bootimg_init(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	struct iommu_domain *domain = NULL;
	void *va = NULL;
	int ret;

	va = hisp_rproc_get_imgva(rproc);
	domain = hisp_rproc_get_domain(rproc);
	if (!va || !domain) {
		pr_err("%s: image va or domain is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = memcpy_s(dev->isp_bw_va, ISP_MEM_SIZE, va, PAGE_SIZE);
	if (ret != 0) {
		pr_err("%s: copy text to boot image fail\n", __func__);
		return ret;
	}

	if (hisp_smmuv3_mode()) {
		ret = iommu_map(domain, 0, dev->isp_bw_pa,
			PAGE_SIZE, ISP_BOOT_MEM_ATTR);
		if (ret != 0) {
			pr_err("%s iommu_map failed: ret %d len\n",
				__func__, ret);
			return ret;
		}
	}

	return 0;
}

static int hisp_nsec_bootimg_release(struct rproc *rproc)
{
	struct device *device = rproc->dev.parent;
	size_t phy_len = 0;

	if (!hisp_smmuv3_mode())
		return 0;

	phy_len = mm_iommu_unmap_fast(device, 0, PAGE_SIZE);
	if (phy_len != PAGE_SIZE) {
		pr_err("%s: iommu_unmap failed: phy_len 0x%lx size 0x%lx\n",
				__func__, phy_len, PAGE_SIZE);
		return -EINVAL;
	}

	return 0;
}

static int hisp_nsec_vqmem_map(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	struct iommu_domain *domain;
	u64 pa;
	size_t size;
	u32 flags;
	int ret;

	domain = hisp_rproc_get_domain(rproc);
	if (!domain) {
		pr_err("%s: domain is NULL\n", __func__);
		return -ENOMEM;
	}

	hisp_rproc_get_vqmem(rproc, &pa, &size, &flags);
	ret = iommu_map(domain, dev->vqda, pa, size, flags);
	if (ret != 0) {
		pr_err("%s iommu_map failed: ret %d len\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static int hisp_nsec_vqmem_unmap(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	struct iommu_domain *domain;
	u64 pa;
	size_t size;
	u32 flags;
	size_t unmapped = 0;

	domain = hisp_rproc_get_domain(rproc);
	if (!domain) {
		pr_err("%s: domain is NULL\n", __func__);
		return -ENOMEM;
	}

	hisp_rproc_get_vqmem(rproc, &pa, &size, &flags);
	if (hisp_smmuv3_mode()) {
		unmapped = mm_iommu_unmap_fast(dev->device,
				dev->vqda, size);
	} else {
		unmapped = iommu_unmap(domain, dev->vqda, size);
	}

	if (unmapped != size) {
		pr_err("%s, unmap fail, len 0x%lx\n", __func__, unmapped);
		return -EINVAL;
	}

	return 0;
}

int hisp_nsec_device_enable(struct rproc *rproc)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	unsigned int canary = 0;
	int ret;

	mutex_lock(&dev->pwrlock);
	ret = hisp_pwr_subsys_powerup();
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_pwr_subsys_powerup.%d\n", __func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	ret = hisp_ispcpu_qos_cfg();
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_ispcpu_qos_cfg.%d\n", __func__, ret);
		goto isp_down;
	}
	hisp_smc_set_nonsec();

	ret = hisp_pwr_core_nsec_init(dev->pgd_base);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_pwr_core_nsec_init.%d\n",
			__func__, ret);
		goto isp_down;
	}

	ret = hisp_nsec_bootimg_init(rproc);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_nsec_bootimg_init.%d\n",
			__func__, ret);
		goto isp_exit;
	}

	get_random_bytes(&canary, sizeof(canary));
	ret = hisp_wait_rpmsg_completion(rproc);
	if (ret != 0)
		goto boot_exit;

	ret = hisp_nsec_vqmem_map(rproc);
	if (ret != 0)
		goto boot_exit;

	ret = hisp_pwr_cpu_nsec_dst(dev->isp_bw_pa, canary);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_pwr_cpu_nsec_dst.%d\n",
			__func__, ret);
		goto vq_exit;
	}
	mutex_unlock(&dev->pwrlock);
#ifdef CONFIG_HISP_DPM
	hisp_dpm_probe();
#endif

	return 0;

vq_exit:
	if (hisp_nsec_vqmem_unmap(rproc))
		pr_err("[%s] Failed : vq mem unmap fail\n", __func__);
boot_exit:
	if (hisp_nsec_bootimg_release(rproc))
		pr_err("[%s] Failed : boot_image_release\n", __func__);
isp_exit:
	if (hisp_pwr_core_nsec_exit())
		pr_err("[%s] Failed : hisp_pwr_core_nsec_exit\n", __func__);
isp_down:
	if (hisp_pwr_subsys_powerdn())
		pr_err("[%s] Failed : hisp_pwr_subsys_powerdn\n", __func__);

	mutex_unlock(&dev->pwrlock);

	return ret;
}

int hisp_nsec_device_disable(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	struct rproc *rproc = platform_get_drvdata(dev->isp_pdev);
	int ret;
#ifdef CONFIG_HISP_DPM
	hisp_dpm_release();
#endif
	mutex_lock(&dev->pwrlock);
	ret = hisp_nsec_vqmem_unmap(rproc);
	if (ret != 0)
		pr_err("[%s] Failed : vq mem unmap fail\n", __func__);

	ret = hisp_nsec_bootimg_release(rproc);
	if (ret != 0)
		pr_err("[%s] boot_image_release failed, ret.%d\n",
			__func__, ret);

	ret = hisp_pwr_cpu_nsec_rst();
	if (ret != 0)
		pr_err("[%s] hisp_pwr_cpu_nsec_rst failed, ret.%d\n",
			__func__, ret);

	ret = hisp_pwr_core_nsec_exit();
	if (ret != 0)
		pr_err("[%s] hisp_pwr_core_nsec_exit failed, ret.%d\n",
			__func__, ret);

	ret = hisp_pwr_subsys_powerdn();
	if (ret != 0)
		pr_err("[%s] hisp_pwr_subsys_powerdn failed, ret.%d\n",
			__func__, ret);
	mutex_unlock(&dev->pwrlock);

	return 0;
}
EXPORT_SYMBOL(hisp_nsec_device_disable);

u64 hisp_nsec_get_pgd(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	return dev->pgd_base;
}

int hisp_nsec_set_pgd(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	struct hisp_shared_para *param = NULL;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		pr_err("[%s] Failed : param.%pK\n", __func__, param);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}
	param->dynamic_pgtable_base = dev->pgd_base;
	hisp_unlock_sharedbuf();

	return 0;
}

static int hisp_bin_load(void *data)
{
#define PART_ISPFW_SIZE 0x00E00000
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret = -1;
	unsigned int fw_size;

	pr_info("[%s] : +\n", __func__);
	fw_size = ISP_FW_SIZE > PART_ISPFW_SIZE ? ISP_FW_SIZE : PART_ISPFW_SIZE;
	dev->isp_bin_vaddr = vmalloc(fw_size);
	if (dev->isp_bin_vaddr == NULL) {
		pr_err("[%s] Failed : vmalloc.%pK\n",
			__func__, dev->isp_bin_vaddr);
		return -ENOMEM;
	}

	ret = hisp_bsp_read_bin("isp_firmware", 0, PART_ISPFW_SIZE,
					dev->isp_bin_vaddr);
	if (ret < 0) {
		vfree(dev->isp_bin_vaddr);
		dev->isp_bin_vaddr = NULL;
		pr_err("[%s] Failed : hisp_bsp_read_bin.%d\n", __func__, ret);
		return ret;
	}
	dev->isp_bin_state = 1;
	pr_info("[%s] : -\n", __func__);

	return 0;
}

static int hisp_remap_rsc(struct hisp_nsec *dev)
{
	dev->isp_bw_va = dma_alloc_coherent(dev->device, ISP_MEM_SIZE,
					&dev->isp_bw_pa, GFP_KERNEL);
	if (dev->isp_bw_va == NULL) {
		pr_err("[%s] isp_bw_va failed\n", __func__);
		return -ENOMEM;
	}
	pr_info("[%s] isp_bw_va.%pK\n", __func__, dev->isp_bw_va);

	return 0;
}

static void hisp_unmap_rsc(struct hisp_nsec *dev)
{
	if (dev->isp_bw_va != NULL)
		dma_free_coherent(dev->device, ISP_MEM_SIZE,
				dev->isp_bw_va, dev->isp_bw_pa);

	dev->isp_bw_va = NULL;
}

static int hisp_nsec_getdts(struct platform_device *pdev,
		struct hisp_nsec *dev)
{
	struct device *device = &pdev->dev;
	struct device_node *np = device->of_node;
	int ret;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -ENODEV;
	}

	pr_info("[%s] +\n", __func__);
	ret = of_property_read_u32(np, "useloadbin", &dev->loadbin);
	if (ret < 0) {/*lint !e64 */
		pr_err("[%s] Failed: loadbin.0x%x of_property_read_u32.%d\n",
			__func__, dev->loadbin, ret);
		return ret;
	}

	ret = of_property_read_u32(np, "isp-ipc-addr", &dev->vqda);
	if (ret < 0) {/*lint !e64 */
		pr_err("[%s] Failed: vqda.0x%x of_property_read_u32.%d\n",
			__func__, dev->vqda, ret);
		return ret;
	}

	pr_info("[%s] vqda.0x%x\n", __func__, dev->vqda);

#ifdef DEBUG_HISP
	ret = hisp_pcie_getdts(np);
	if (ret < 0) {
		pr_err("Failed : hisp_pcie_getdts.%d\n", ret);
		return ret;
	}
#endif

	pr_info("[%s] -\n", __func__);

	return 0;
}

#ifdef DEBUG_HISP
static int hisp_nsec_pcie_probe(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret;

	if (dev->isp_pcie_cfg->isp_pci_inuse_flag == 0)
		return ISP_NORMAL_MODE;

	dev->isp_pcie_cfg->isp_pci_ready = ISP_PCIE_NREADY;

	ret = hisp_pcie_register();
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_pcie_register\n", __func__);
		goto clean_isp_pcie;
	}

	ret = hisp_get_pcie_outbound(dev->isp_pcie_cfg);
	if (ret != ISP_PCIE_READY) {
		pr_err("[%s] Failed : hisp_get_pcie_outbound\n", __func__);
		goto clean_isp_pcie;
	}

	dev->isp_pcie_cfg->isp_pci_reg = ioremap(dev->isp_pcie_cfg->isp_pci_addr, dev->isp_pcie_cfg->isp_pci_size);

	pr_info("[%s] pci reg VA: 0x%llx\n", __func__, dev->isp_pcie_cfg->isp_pci_reg);

	return ISP_PCIE_MODE;

clean_isp_pcie:
	hisp_pcie_unregister();
	return ret;
}

static void hisp_nsec_pcie_remove(void)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	if (dev->isp_pcie_cfg->isp_pci_inuse_flag == 0)
		return;

	iounmap(dev->isp_pcie_cfg->isp_pci_reg);
	(void)hisp_pcie_unregister();
	dev->isp_pcie_cfg->isp_pci_ready = ISP_PCIE_NREADY;
}
#endif

int hisp_nsec_probe(struct platform_device *pdev)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret;

	pr_alert("[%s] +\n", __func__);
	dev->device = &pdev->dev;
	dev->isp_pdev = pdev;

#ifdef DEBUG_HISP
	dev->isp_pcie_cfg = &nsec_isp_pcie_cfg;
#endif

	ret = hisp_remap_rsc(dev);
	if (ret != 0) {
		pr_err("[%s] failed, isp_remap_src.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_nsec_getdts(pdev, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_nsec_getdts.%d.\n",
				__func__, ret);
		goto hisp_nsec_getdts_fail;
	}

	dev->pgd_base = kernel_domain_get_ttbr(dev->device);
#ifdef DEBUG_HISP
	ret = hisp_nsec_pcie_probe();
	if (ret == ISP_PCIE_NREADY)
		goto hisp_nsec_getdts_fail;
#endif

	dev->isp_bin_state = 0;
	dev->isp_bin_vaddr = NULL;
	if (dev->loadbin) {
		dev->loadispbin = kthread_create(hisp_bin_load,
			NULL, "loadispbin");
		if (IS_ERR(dev->loadispbin)) {
			pr_err("[%s] Failed : kthread_create.%ld\n",
				__func__, PTR_ERR(dev->loadispbin));
			goto loadbin_fail;
		}
	}

	mutex_init(&dev->pwrlock);
	pr_alert("[%s] -\n", __func__);

	return 0;

loadbin_fail:
#ifdef DEBUG_HISP
	hisp_nsec_pcie_remove();
#endif

hisp_nsec_getdts_fail:
	hisp_unmap_rsc(dev);

	return ret;
}

int hisp_nsec_remove(struct platform_device *pdev)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;

	mutex_destroy(&dev->pwrlock);

	if (dev->loadbin) {
		if (dev->loadispbin != NULL) {
			kthread_stop(dev->loadispbin);
			dev->loadispbin = NULL;
		}
	}

	dev->isp_bin_state = 0;
	dev->isp_bin_vaddr = NULL;
#ifdef DEBUG_HISP
	hisp_nsec_pcie_remove();
#endif
	hisp_unmap_rsc(dev);
	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar nsec device driver");

#ifdef DEBUG_HISP
#define SECU_MAX_SIZE 64
#define TMP_MAX_SIZE 0x1000

static int hisp_boot_mode_show_part(char *tmp, ssize_t *size)
{
	int ret;

	ret = snprintf_s(tmp, SECU_MAX_SIZE, SECU_MAX_SIZE - 1,
			"\nISP Types(Current ISP Type:%u):\n",
			hisp_get_boot_mode());
	if (ret < 0) {
		pr_err("[%s] Failed : Current ISP Type\n", __func__);
		return ret;
	}
	*size += ret;

	ret = snprintf_s(tmp + *size, SECU_MAX_SIZE, SECU_MAX_SIZE - 1, "%s\n",
		"0. SEC_CASE(if current platform support, e.g. V120)");
	if (ret < 0) {
		pr_err("[%s] Failed : SEC_CASE\n", __func__);
		return ret;
	}
	*size += ret;

	ret = snprintf_s(tmp + *size, SECU_MAX_SIZE, SECU_MAX_SIZE - 1, "%s\n",
		"1. NONSEC_CASE(if current platform support, e.g. V120)");
	if (ret < 0) {
		pr_err("[%s] Failed : SEC_CASE\n", __func__);
		return ret;
	}
	*size += ret;

	return 0;
}

ssize_t hisp_boot_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;
	char *tmp = NULL;
	int ret = 0;

	pr_info("[%s] +\n", __func__);

	tmp = kzalloc(TMP_MAX_SIZE, GFP_KERNEL);
	if (tmp == NULL)
		return 0;

	ret = hisp_boot_mode_show_part(tmp, &size);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_boot_mode_show_part.%d\n", __func__, ret);
		goto show_err;
	}

	ret = snprintf_s(tmp + size, SECU_MAX_SIZE, SECU_MAX_SIZE - 1, "%s\n",
				"3. INVAL_CASE");
	if (ret < 0) {
		pr_err("[%s] Failed : INVAL_CASE\n", __func__);
		goto show_err;
	}
	size += ret;

	ret = snprintf_s(tmp + size, SECU_MAX_SIZE, SECU_MAX_SIZE - 1, "\n%s\n",
				"ISP Type Set:");
	if (ret < 0) {
		pr_err("[%s] Failed : ISP Type Set\n", __func__);
		goto show_err;
	}
	size += ret;

	ret = snprintf_s(tmp + size, SECU_MAX_SIZE, SECU_MAX_SIZE - 1, "%s\n",
				"e.g. echo SEC_CASE > sec_nsec_isp:");
	if (ret < 0) {
		pr_err("[%s] Failed : sec_nsec_isp\n", __func__);
		goto show_err;
	}
	size += ret;

show_err:
	ret = memcpy_s((void *)buf, TMP_MAX_SIZE, (void *)tmp, TMP_MAX_SIZE);
	if (ret != 0)
		pr_err("[%s] Failed : memcpy_s buf from tmp fail\n", __func__);

	kfree((void *)tmp);

	pr_info("[%s] -\n", __func__);

	return size;
}

ssize_t hisp_boot_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	pr_info("[%s] +\n", __func__);
	if (!strncmp("SEC_CASE", buf, sizeof("SEC_CASE") - 1)) {
		pr_info("%s: SEC_CASE.\n", __func__);
		hisp_set_boot_mode(SEC_CASE);
	} else if (!strncmp("NONSEC_CASE", buf, sizeof("NONSEC_CASE") - 1)) {
		pr_info("%s: NONSEC_CASE.\n", __func__);
		hisp_set_boot_mode(NONSEC_CASE);
	} else {
		pr_info("%s: INVAL_CASE.\n", __func__);
	}
	pr_info("[%s] -\n", __func__);

	return count; /*lint !e713*/
}

static int hisp_update_binload(char *buffer)
{
	int ret          = -1;
	char *pathname   = "/system/vendor/firmware/isp.bin";
	struct file *fp = NULL;
	mm_segment_t fs;
	loff_t pos = 0;
	struct kstat m_stat;
	int length;

	if (buffer == NULL) {
		pr_err("[%s] : buffer is null\n", __func__);
		return -EINVAL;
	}

	/*get resource*/
	fp = filp_open(pathname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("[%s] : filp_open(%s) failed\n", __func__, pathname);
		goto error;
	}

	ret = vfs_llseek(fp, 0, SEEK_SET);
	if (ret < 0) {
		pr_err("[%s] : seek ops failed, ret %d\n", __func__, ret);
		goto error2;
	}

	fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501 */

	ret = vfs_stat(pathname, &m_stat);
	if (ret < 0) {
		pr_err("%s:Failed :%s vfs_stat: %d\n", __func__, pathname, ret);
		set_fs(fs);
		goto error2;
	}
	length = m_stat.size;

	pos = fp->f_pos;/*lint !e613 */
	ret = vfs_read(fp, (char __user *)buffer, length, &pos);/*lint !e613 */
	if (ret != length) {
		pr_err("read ops failed, ret=%d(len=%d)\n", ret, length);
		set_fs(fs);
		goto error2;
	}
	set_fs(fs);

	filp_close(fp, NULL);/*lint !e668 */

	return 0;

error2:
	filp_close(fp, NULL);/*lint !e668 */
error:
	pr_err("[%s] : failed\n", __func__);
	return -EPERM;
}

ssize_t hisp_loadbin_update_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	struct hisp_nsec *dev = &hisp_nsec_dev;
	int ret = 0;
	int status;

	pr_info("[%s] : +\n", __func__);

	status = hisp_rproc_enable_status();
	if (status > 0) {
		pr_err("[%s] : isp_rproc had enabled, status.0x%x\n",
			__func__, status);
		return -ENODEV;
	}

	if (dev->isp_bin_vaddr == NULL) {
		pr_err("[%s] Failed : isp_bin_vaddr.NULL\n", __func__);
		return -ENOMEM;
	}

	ret = hisp_update_binload(dev->isp_bin_vaddr);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_update_binload\n", __func__);
		return -ENOMEM;
	}
	pr_info("[%s] : -\n", __func__);
	return 0;
}
#endif
