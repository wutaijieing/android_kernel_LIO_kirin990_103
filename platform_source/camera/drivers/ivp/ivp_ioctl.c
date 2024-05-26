/*
 * This file implements ioctl for node dev-ivp
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

#include "ivp_ioctl.h"
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/mm_iommu.h>
#include <linux/syscalls.h>
#include <linux/clk-provider.h>
#include <linux/bitops.h>
#include <linux/firmware.h>
#include <platform_include/see/efuse_driver.h>
#include "securec.h"
#include "ivp_manager.h"
#include "ivp.h"
#include "ivp_log.h"
#include "ivp_reg.h"
#include "ivp_sec.h"
#include "ivp_map.h"
#ifdef CONFIG_IVP_SMMU_V3
#include "ivp_smmuv3.h"
#else
#include "ivp_smmu.h"
#endif
#ifdef CONFIG_IVP_PG_DISABLE
#include "soc_spec_info.h"
#endif

#define IVP_CORE0_AVAILABLE       1
#ifdef MULTIPLE_ALGO
#include "ivp_multi_algo.h"
#endif

static int ivp_poweron_remap(struct ivp_device *ivp_devp)
{
	int ret;
	unsigned int section_index;

#ifdef SEC_IVP_ENABLE
	if (ivp_devp->ivp_comm.ivp_secmode == SECURE_MODE) {
		ret = ivp_sec_poweron_remap(ivp_devp);
	} else
#endif
	{
#ifdef MULTIPLE_ALGO
		section_index = BASE_SECT_INDEX;
#else
		section_index = DDR_SECTION_INDEX;
#endif
		ret = ivp_remap_addr_ivp2ddr(ivp_devp,
			ivp_devp->ivp_comm.sects[section_index].ivp_addr,
			ivp_devp->ivp_comm.ivp_meminddr_len,
			ivp_devp->ivp_comm.sects[section_index].acpu_addr << IVP_MMAP_SHIFT);
	}
	if (ret)
		ivp_err("remap addr failed %d", ret);

#ifdef IVP_FAMA_SUPPORT
	ivp_fama_addr_32bit_2_64bit(ivp_devp);
#endif

	return ret;
}

static int ivp_dev_poweron(struct ivp_device *pdev)
{
	int ret;

	mutex_lock(&pdev->ivp_comm.ivp_wake_lock_mutex);
	if (!pdev->ivp_comm.ivp_power_wakelock->active) {
		__pm_stay_awake(pdev->ivp_comm.ivp_power_wakelock);
		ivp_info("ivp power on enter, wake lock");
	}
	mutex_unlock(&pdev->ivp_comm.ivp_wake_lock_mutex); /*lint !e456*/

	ret = ivp_poweron_pri(pdev);
	if (ret) {
		ivp_err("power on pri setting failed [%d]", ret);
		goto err_ivp_poweron_pri;
	}

	/* set auto gate clk etc. */
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE)
		ivp_dev_set_dynamic_clk(pdev, IVP_DISABLE);
	else
		ivp_dev_set_dynamic_clk(pdev, IVP_ENABLE);

	ret = ivp_poweron_remap(pdev);
	if (ret) {
		ivp_err("power on remap setting failed [%d]", ret);
		goto err_ivp_poweron_remap;
	}

	/* After reset, enter running mode */
	ivp_hw_set_ocdhalt_on_reset(pdev, 0);

	/* Put ivp in stall mode */
	ivp_hw_runstall(pdev, IVP_RUNSTALL_STALL);
	/* Reset ivp core */
	ivp_hw_enable_reset(pdev);

	/* Boot from IRAM. */
	ivp_hw_set_bootmode(pdev, IVP_BOOT_FROM_IRAM);

	/* Disable system reset, let ivp core leave from reset */
	ivp_hw_disable_reset(pdev);

#ifdef MULTIPLE_ALGO
	ret = ivp_algo_mem_init(pdev);
#endif
	return ret; /*lint !e454*/

err_ivp_poweron_remap:
	ivp_poweroff_pri(pdev);

err_ivp_poweron_pri:
	mutex_lock(&pdev->ivp_comm.ivp_wake_lock_mutex);
	if (pdev->ivp_comm.ivp_power_wakelock->active) {
		__pm_relax(pdev->ivp_comm.ivp_power_wakelock);
		ivp_err("ivp power on failed, wake unlock");
	}
	mutex_unlock(&pdev->ivp_comm.ivp_wake_lock_mutex); /*lint !e456*/

	return ret; /*lint !e454*/
}

void ivp_dev_poweroff(struct ivp_device *pdev)
{
	int ret;
#ifdef MULTIPLE_ALGO
	int i;
#endif

	if (!pdev) {
		ivp_err("invalid input param pdev");
		return;
	}
	ret = ivp_poweroff_pri(pdev);
	if (ret)
		ivp_err("power on private setting failed:%d", ret);

	mutex_lock(&pdev->ivp_comm.ivp_wake_lock_mutex);
	if (pdev->ivp_comm.ivp_power_wakelock->active) {
		__pm_relax(pdev->ivp_comm.ivp_power_wakelock);
		ivp_info("ivp power off, wake unlock");
	}

#ifdef MULTIPLE_ALGO
	pdev->core_status = (unsigned int)INVALID;
	dyn_core_loaded[pdev->core_id] = false;
	for (i = 0; i < IVP_ALGO_NODE_MAX; i++) {
		if (pdev->algo_mem_info[i].occupied == WORK) {
			if (pdev->algo_mem_info[i].algo_ddr_addr)
				ivp_free_algo_zone(pdev, i, SEGMENT_DDR_TEXT);
			if (pdev->algo_mem_info[i].algo_func_addr)
				ivp_free_algo_zone(pdev, i, SEGMENT_IRAM_TEXT);
			if (pdev->algo_mem_info[i].algo_data_addr)
				ivp_free_algo_zone(pdev, i, SEGMENT_DRAM0_DATA);
			if (pdev->algo_mem_info[i].algo_bss_addr)
				ivp_free_algo_zone(pdev, i, SEGMENT_DRAM0_BSS);
		}
		pdev->algo_mem_info[i].occupied = INVALID;
		init_algo_mem_info(pdev, i);
	}
	kfree(pdev->algo_mem_info);
	pdev->algo_mem_info = NULL;
#endif
	mutex_unlock(&pdev->ivp_comm.ivp_wake_lock_mutex); /*lint !e456*/
}

void ivp_poweroff(struct ivp_device *pdev)
{
	if (!pdev) {
		ivp_err("invalid input param pdev");
		return;
	}

	if (atomic_read(&pdev->ivp_comm.poweron_success) != 0) {
		ivp_err("maybe ivp dev not poweron success");
		return;
	}

	ivp_deinit_resethandler(pdev);

	ivp_hw_runstall(pdev, IVP_RUNSTALL_STALL);
	if (ivp_hw_query_runstall(pdev) != IVP_RUNSTALL_STALL)
		ivp_err("failed to stall ivp");
	ivp_hw_clr_wdg_irq(pdev);

	disable_irq(pdev->ivp_comm.wdg_irq);
	free_irq(pdev->ivp_comm.wdg_irq, pdev);

	if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE) {
		disable_irq(pdev->ivp_comm.dwaxi_dlock_irq);
		free_irq(pdev->ivp_comm.dwaxi_dlock_irq, pdev);
		ivp_dev_smmu_deinit(pdev);
#ifdef SEC_IVP_V300
#ifdef SEC_IVP_ENABLE
	} else {
		ivp_check_and_wait_wfi(pdev);
		ivp_clear_map_info();
		ivp_dev_smmu_deinit(pdev);
#endif
#endif
	}

	ivp_dev_poweroff(pdev);
#ifdef SEC_IVP_ENABLE
	if (pdev->ivp_comm.ivp_sec_support && pdev->ivp_comm.ivp_secmode) {
		if (ivp_destroy_secimage_thread(pdev))
			ivp_err("ivp_destroy_secimage_thread failed!");
	}
#endif

	pdev->ivp_comm.ivp_secmode = NOSEC_MODE;
	atomic_inc(&pdev->ivp_comm.poweron_access);
	atomic_inc(&pdev->ivp_comm.poweron_success);
}

static int ivp_accessible_check(struct ivp_device *pdev)
{
	if (atomic_read(&pdev->ivp_comm.poweron_access) == 0) {
		ivp_err("maybe ivp dev has power on");
		return -EBUSY;
	}
	return 0;
}

static int ivp_sec_init_process(struct ivp_device *pdev,
		const struct ivp_power_up_info *pu_info)
{
	int ret = 0;
	if (pu_info->sec_mode == SECURE_MODE &&
		pdev->ivp_comm.ivp_sec_support == 0) {
		ivp_err("ivp don't support secure mode");
		return -EINVAL;
	}

	pdev->ivp_comm.ivp_secmode = (unsigned int)pu_info->sec_mode;
	pdev->ivp_comm.ivp_sec_buff_fd = pu_info->sec_buff_fd;

#ifdef SEC_IVP_ENABLE
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE) {
		if (pu_info->sec_buff_size != IVP_SEC_BUFF_SIZE) {
			ivp_err("ivp sec buff size not correct");
			return -EINVAL;
		}
#ifdef SEC_IVP_V300
		ret = ivp_init_map_info();
		if (ret) {
			ivp_err("failed to init map info ret:%d", ret);
			return ret;
		}
#endif
		ret = ivp_create_secimage_thread(pdev);
		if (ret) {
			ivp_err("create sec ivp thread failed, ret:%d", ret);
			return ret;
		}
	}
#endif

	return ret;
}

static void ivp_sec_deinit_process(struct ivp_device *pdev)
{
#ifdef SEC_IVP_ENABLE
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE)
		ivp_destroy_secimage_thread(pdev);
#else
	(void)pdev;
#endif
}

static int ivp_smmu_init_process(struct ivp_device *pdev)
{
	int ret;
	if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE) {
		ret = ivp_dev_smmu_init(pdev);
		if (ret) {
			ivp_err("failed to init smmu");
			return -1;
		}
#ifdef SEC_IVP_V300
#ifdef SEC_IVP_ENABLE
	} else {
		ret = ivp_dev_smmu_init(pdev);
		if (ret) {
			ivp_err("failed to init smmu");
			return -1;
		}
#endif
#endif
	}

	return 0;
}

static void ivp_smmu_deinit_process(struct ivp_device *pdev)
{
	if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE)
		ivp_dev_smmu_deinit(pdev);
#ifdef SEC_IVP_V300
#ifdef SEC_IVP_ENABLE
	else
		ivp_dev_smmu_deinit(pdev);
#endif
#endif
}

static int ivp_irq_init_process(struct ivp_device *pdev)
{
	int ret;

	if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE) {
		ret = request_irq(pdev->ivp_comm.dwaxi_dlock_irq, ivp_dwaxi_irq_handler,
			0, "ivp_dwaxi_irq", (void *)pdev);
		if (ret) {
			ivp_err("failed to request dwaxi irq.%d", ret);
			return ret;
		}
	}

	ret = request_irq(pdev->ivp_comm.wdg_irq, ivp_wdg_irq_handler,
		0, "ivp_wdg_irq", (void *)pdev);
	if (ret) {
		ivp_err("failed to request wdg irq.%d", ret);
		if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE)
			free_irq(pdev->ivp_comm.dwaxi_dlock_irq, pdev);

		return ret;
	}

	return 0;
}

void ivp_irq_deinit_process(struct ivp_device *pdev)
{
	free_irq(pdev->ivp_comm.wdg_irq, pdev);
	if (pdev->ivp_comm.ivp_secmode == NOSEC_MODE)
		free_irq(pdev->ivp_comm.dwaxi_dlock_irq, pdev);
}

static int ivp_poweron(struct ivp_device *pdev,
		const struct ivp_power_up_info *pu_info)
{
	int ret;

	if (ivp_accessible_check(pdev))
		return -EBUSY;

	atomic_dec(&pdev->ivp_comm.poweron_access);
	atomic_set(&pdev->ivp_comm.wdg_sleep, 0);
	sema_init(&pdev->ivp_comm.wdg_sem, 0);

	ret = ivp_sec_init_process(pdev, pu_info);
	if (ret)
		goto err_ivp_sec_process;

	ret = ivp_dev_poweron(pdev);
	if (ret < 0) {
		ivp_err("failed to power on ivp");
		goto err_ivp_dev_poweroff;
	}

	ret = ivp_smmu_init_process(pdev);
	if (ret)
		goto err_ivp_smmu_process;

	ret = ivp_irq_init_process(pdev);
	if (ret)
		goto err_ivp_irq_process;

	ret = ivp_init_resethandler(pdev);
	if (ret) {
		ivp_err("failed to init reset handler");
		goto err_ivp_init_resethandler;
	}

	ivp_info("open ivp device success");
	atomic_dec(&pdev->ivp_comm.poweron_success);

	return ret;

err_ivp_init_resethandler:
	ivp_irq_deinit_process(pdev);
err_ivp_irq_process:
	ivp_smmu_deinit_process(pdev);
err_ivp_smmu_process:
	ivp_dev_poweroff(pdev);
err_ivp_dev_poweroff:
	ivp_sec_deinit_process(pdev);
err_ivp_sec_process:
	pdev->ivp_comm.ivp_secmode = NOSEC_MODE;
	atomic_inc(&pdev->ivp_comm.poweron_access);
	ivp_dsm_error_notify(DSM_IVP_OPEN_ERROR_NO);

	ivp_info("poweron ivp device fail");
	return ret;
}

static long ioctl_power_up(struct file *fd, unsigned long args)
{
	long ret = -EINVAL;
	struct ivp_power_up_info info = {0};
	struct ivp_device *pdev = NULL;
#ifdef CONFIG_IVP_PG_DISABLE
	static unsigned int module_freq_level = FREQ_INVALID;

	if (module_freq_level == FREQ_INVALID)
		module_freq_level = get_module_freq_level(kernel, ivp);
	ivp_info("module freq level is %u", module_freq_level);

	if (module_freq_level == FREQ_LEVEL2) {
		ivp_info("lite chip");
		return -EINVAL;
	}
#endif
	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = (struct ivp_device *)fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}
	mutex_lock(&pdev->ivp_comm.ivp_ioctl_mutex);
	mutex_lock(&pdev->ivp_comm.ivp_power_up_off_mutex);
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		goto power_up_err;
	}

	if (info.sec_mode != SECURE_MODE && info.sec_mode != NOSEC_MODE) {
		ivp_err("invalid input secMode value:%d", info.sec_mode);
		goto power_up_err;
	}
	if (info.sec_mode == SECURE_MODE && info.sec_buff_fd < 0) {
		ivp_err("invalid sec buffer fd value:%d", info.sec_buff_fd);
		goto power_up_err;
	}

	ret = ivp_poweron(pdev, &info);
power_up_err:
	mutex_unlock(&pdev->ivp_comm.ivp_power_up_off_mutex);
	mutex_unlock(&pdev->ivp_comm.ivp_ioctl_mutex);
	return ret;
}

static long ioctl_power_down(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = NULL;
	if (!fd || !fd->private_data) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = (struct ivp_device *)fd->private_data;
	mutex_lock(&pdev->ivp_comm.ivp_power_up_off_mutex);
#ifdef IVP_DUAL_CORE
	ivp_info("power down ivp %u", pdev->core_id);
#endif
	ivp_poweroff(pdev);
	mutex_unlock(&pdev->ivp_comm.ivp_power_up_off_mutex);
	return EOK;
}

static long ioctl_sect_count(struct file *fd, unsigned long args)
{
	long ret;
	struct ivp_device *pdev = NULL;
	unsigned int sect_count;
	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or odev");
		return -EINVAL;
	}
	sect_count = (unsigned int)pdev->ivp_comm.sect_count;

	ivp_info("get img sect num:%#x", sect_count);
	ret = (long)copy_to_user((void *)(uintptr_t)args,
		&sect_count, sizeof(sect_count));

	return ret;
}

static long ioctl_hidl_map(struct file *fd, unsigned long args)
{
	struct ivp_map_info info;
	struct ivp_device *pdev = NULL;
	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE)
		return -EINVAL;

	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if (ivp_map_hidl_fd(&pdev->ivp_comm.ivp_pdev->dev, &info)) {
		ivp_err("ivp_map_hidl_fd fail");
		return -EPERM;
	}

	if (copy_to_user((void *)(uintptr_t)args, &info, sizeof(info))) {
		ivp_err("copy to user failed");
		return -ENOMEM;
	}

	return EOK;
}

static long ioctl_hidl_unmap(struct file *fd, unsigned long args)
{
	struct ivp_map_info info;
	struct ivp_device *pdev = NULL;
	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE)
		return -EINVAL;

	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if (ivp_unmap_hidl_fd(&pdev->ivp_comm.ivp_pdev->dev, &info)) {
		ivp_err("ivp_unmap_hidl_fd fail");
		return -EPERM;
	}

	if (copy_to_user((void *)(uintptr_t)args, &info, sizeof(info))) {
		ivp_err("copy to user failed");
		return -ENOMEM;
	}

	return EOK;
}

static long ioctl_fd_info(struct file *fd, unsigned long args)
{
#ifdef SEC_IVP_V300
	struct ivp_fd_info info;
	struct ivp_device *pdev = NULL;

	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}
	if (pdev->ivp_comm.ivp_secmode != SECURE_MODE)
		return -EINVAL;
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if (info.mapped == 0) {
		if (ivp_init_sec_fd(&info)) {
			ivp_err("ivp init sec fd fail");
			return -ENOMEM;
		}
	} else {
		if (ivp_deinit_sec_fd(&info)) {
			ivp_err("ivp deinit sec fd fail");
			return -ENOMEM;
		}
	}

	if (copy_to_user((void *)(uintptr_t)args, &info, sizeof(info))) {
		ivp_err("copy to user failed");
		return -ENOMEM;
	}
#else
	if (!fd || !args) {
		ivp_err("invalid input args or fd");
		return -EINVAL;
	}
#endif

	return EOK;
}

static long ioctl_sect_info(struct file *fd, unsigned long args)
{
	long ret;
	struct ivp_sect_info info;
	struct ivp_device *pdev = NULL;
	unsigned int sect_count;
	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	sect_count = (unsigned int)pdev->ivp_comm.sect_count;
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if (info.index >= sect_count) {
		ivp_err("index is out of range.index:%u, sec_count:%u",
			info.index, sect_count);
		return -EINVAL;
	}

	if ((pdev->ivp_comm.ivp_secmode == SECURE_MODE) ||
		(info.info_type == SECURE_MODE)) {
		ret = (long)copy_to_user((void *)(uintptr_t)args,
			&pdev->ivp_comm.sec_sects[info.index],
			sizeof(struct ivp_sect_info));
		ivp_dbg("name:%s, ivp_addr:0x%x, acpu_addr:0x%lx",
			pdev->ivp_comm.sec_sects[info.index].name,
			pdev->ivp_comm.sec_sects[info.index].ivp_addr,
			pdev->ivp_comm.sec_sects[info.index].acpu_addr);
	} else {
		ret = (long)copy_to_user((void *)(uintptr_t)args,
			&pdev->ivp_comm.sects[info.index],
			sizeof(struct ivp_sect_info));
	}

	return ret;
}

static int ivp_copy_section(struct ivp_device *pdev,
		const struct image_section_header *image_sect, const unsigned int *source)
{
	unsigned int i;
	unsigned int *mem_addr = NULL;
	void *mem = NULL;
	bool ddr_flag = false;
	unsigned long ivp_ddr_addr;
	unsigned int offset = 0;
	errno_t ret;

	if ((image_sect->vaddr >= pdev->ivp_comm.sects[SECT_START_NUM].ivp_addr) &&
		(image_sect->vaddr <= (pdev->ivp_comm.sects[SECT_START_NUM].ivp_addr +
			pdev->ivp_comm.sects[SECT_START_NUM].len))) {
		ddr_flag = true;
	} else {
		ddr_flag = false;
	}

	if (ddr_flag == true) {
		ivp_ddr_addr = (pdev->ivp_comm.sects[SECT_START_NUM].acpu_addr <<
			IVP_MMAP_SHIFT) + image_sect->vaddr -
			pdev->ivp_comm.sects[SECT_START_NUM].ivp_addr;
		mem = ivp_vmap(ivp_ddr_addr, image_sect->size, &offset);
	} else {
		/*lint -e446 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		mem = ioremap_nocache(image_sect->vaddr,
				image_sect->size);
#else
		mem = ioremap(image_sect->vaddr,
				image_sect->size);
#endif
		/*lint +e446 */
	}

	if (!mem) {
		ivp_err("can't map base address");
		return -EINVAL;
	}
	mem_addr = mem;
	if (ddr_flag == true) {
		ret = memcpy_s(mem_addr, image_sect->size,
				source, image_sect->size);
		if (ret != EOK) {
			ivp_err("memcpy_s fail, ret [%d]", ret);
			vunmap(mem - offset);
			return -EINVAL;
		}
	} else {
		for (i = 0; i < image_sect->size / 4; i++) /* 4 is sizeof xt-int */
			*(mem_addr + i) = *(source + i);
	}

	if (ddr_flag == true)
		vunmap(mem - offset);
	else
		iounmap(mem);

	return EOK;
}

static int ivp_load_section(struct ivp_device *pdev,
		const struct image_section_header *image_sect, const struct firmware *fw)
{
	unsigned int *source = NULL;
	int ret;

	switch (image_sect->type) {
	case IMAGE_SECTION_TYPE_EXEC:
	case IMAGE_SECTION_TYPE_DATA: {
		source = (unsigned int *)(fw->data + image_sect->offset);
		ret = ivp_copy_section(pdev, image_sect, source);
		if (ret < 0)
			return -EINVAL;
	}
	break;
	case IMAGE_SECTION_TYPE_BSS:
	break;
	default: {
		ivp_err("unsupported section type %d", image_sect->type);
		return -EINVAL;
	}
	}

	return EOK;
}

static int ivp_get_validate_section_info(struct ivp_device *pdev,
		const struct firmware *fw,
		struct image_section_header *psect_header,
		unsigned int index)
{
	unsigned int offset;
	errno_t ret;

	if (!psect_header) {
		ivp_err("input para is invalid");
		return -EINVAL;
	}
	offset = sizeof(struct file_header) + sizeof(*psect_header) * index;
	if ((offset + sizeof(*psect_header)) > fw->size) {
		ivp_err("image index is err");
		return -EINVAL;
	}
	ret = memcpy_s(psect_header, sizeof(*psect_header),
		fw->data + offset, sizeof(*psect_header));
	if (ret != EOK) {
		ivp_err("memcpy_s fail, ret [%d]", ret);
		return -EINVAL;
	}

	if ((psect_header->offset + psect_header->size) > fw->size) {
		ivp_err("get invalid offset 0x%x", psect_header->offset);
		return -EINVAL;
	}
#ifdef IVP_DUAL_CORE
	if ((psect_header->vaddr < pdev->ivp_comm.sects[SECT_START_NUM].ivp_addr) ||
		(psect_header->vaddr > (pdev->ivp_comm.sects[SECT_START_NUM].ivp_addr +
			pdev->ivp_comm.sects[SECT_START_NUM].len)))
		psect_header->vaddr += pdev->base_offset;
#endif
	/* 0,1,2,3 represent array index */
	if (((psect_header->vaddr >= pdev->ivp_comm.sects[0].ivp_addr) &&
		(psect_header->vaddr < (pdev->ivp_comm.sects[0].ivp_addr +
			pdev->ivp_comm.sects[0].len))) ||
		((psect_header->vaddr >= pdev->ivp_comm.sects[1].ivp_addr) &&
		(psect_header->vaddr < (pdev->ivp_comm.sects[1].ivp_addr +
			pdev->ivp_comm.sects[1].len))) ||
		((psect_header->vaddr >= pdev->ivp_comm.sects[2].ivp_addr) &&
		(psect_header->vaddr < (pdev->ivp_comm.sects[2].ivp_addr +
			pdev->ivp_comm.sects[2].len))) ||
		((psect_header->vaddr >= pdev->ivp_comm.sects[3].ivp_addr) &&
		(psect_header->vaddr < (pdev->ivp_comm.sects[3].ivp_addr +
			pdev->ivp_comm.sects[3].len))))
		return EOK;

	ivp_err("get invalid addr");
	return -EINVAL;
}

static int ivp_load_firmware(struct ivp_device *pdev,
		const char *filename, const struct firmware *fw)
{
	unsigned int idx;
	struct file_header mheader;
	errno_t ret;

	ret = memcpy_s(&mheader, sizeof(mheader), fw->data, sizeof(mheader));
	if (ret != EOK) {
		ivp_err("memcpy_s fail, ret [%d]", ret);
		return -EINVAL;
	}
	ivp_info("start loading image %s, section counts 0x%x",
		filename, mheader.sect_count);
	for (idx = 0; idx < mheader.sect_count; idx++) {
		struct image_section_header sect;
		(void)memset_s(&sect, sizeof(sect), 0, sizeof(sect));
		if (ivp_get_validate_section_info(pdev, fw, &sect, idx)) {
			ivp_err("get section %d fails", idx);
			return -EINVAL;
		}
		if (ivp_load_section(pdev, &sect, fw)) {
			ivp_err("load section %d fails", idx);
			return -EINVAL;
		}
	}
	ivp_info("finish loading image %s", filename);
	return 0;
}

static int ivp_check_image(const struct firmware *fw)
{
	errno_t ret;
	struct file_header mheader;

	if (sizeof(mheader) > fw->size) {
		ivp_err("image file mheader is err");
		return -EINVAL;
	}
	ret = memcpy_s(&mheader, sizeof(mheader), fw->data, sizeof(mheader));
	if (ret != EOK) {
		ivp_err("memcpy_s fail, ret [%d]", ret);
		return -EINVAL;
	}

	if (strncmp(mheader.name, "IVP:", strlen("IVP:")) != 0) {
		ivp_err("image file header is not for IVP");
		return -EINVAL;
	}

	if (fw->size != mheader.image_size) {
		ivp_err("request_firmware size 0x%zx mheader size 0x%x",
			fw->size, mheader.image_size);
		return -EINVAL;
	}
	return 0;
}

#ifdef MULTIPLE_ALGO
static int ivp_segment_type(const XT_Elf32_Phdr *program_header)
{
	if (program_header->p_vaddr < IVP_ALGO_MAX_SIZE)
		return SEGMENT_DDR_TEXT;
	else if (program_header->p_vaddr == IVP_IRAM_TEXT_SEGMENT_ADDR)
		return SEGMENT_IRAM_TEXT;
	else if (program_header->p_vaddr == IVP_DRAM0_DATA_SEGMENT_ADDR)
		return SEGMENT_DRAM0_DATA;
	else if (program_header->p_vaddr == IVP_DRAM0_BSS_SEGMENT_ADDR)
		return SEGMENT_DRAM0_BSS;
	ivp_info("abnormal segment type");
	return -EINVAL;
}

static void *ivp_ioremap(const struct ivp_device *pdev,
		const XT_Elf32_Phdr *program_header, unsigned int algo_id,
		int segment_type)
{
	unsigned int map_addr;
	if (segment_type == SEGMENT_IRAM_TEXT) {
		map_addr = pdev->algo_mem_info[algo_id].algo_func_addr + pdev->base_offset;
	} else if (segment_type == SEGMENT_DRAM0_DATA) {
		map_addr = pdev->algo_mem_info[algo_id].algo_data_addr + pdev->base_offset;
	} else if (segment_type == SEGMENT_DRAM0_BSS) {
		map_addr = pdev->algo_mem_info[algo_id].algo_bss_addr + pdev->base_offset;
	} else {
		ivp_info("abnormal segment type");
		return NULL;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	return ioremap_nocache(map_addr, program_header->p_memsz); //lint !e446
#else
	return ioremap(map_addr, program_header->p_memsz); //lint !e446
#endif
}

static int ivp_load_algo_segment(struct ivp_device *pdev,
		const struct firmware *fw, const XT_Elf32_Phdr *program_header,
		const unsigned int algo_id)
{
	unsigned long ivp_ddr_addr;
	void *mem = NULL;
	unsigned int offset = 0;
	unsigned int *mem_addr = NULL;
	unsigned int i;
	int segment_type = ivp_segment_type(program_header);
	unsigned int *source = (unsigned int *)(fw->data + program_header->p_offset);

	if (segment_type == SEGMENT_DDR_TEXT) {
		ivp_ddr_addr = pdev->algo_mem_info[algo_id].algo_ddr_addr -
			(pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[BASE_SECT_INDEX].len) +
			((pdev->ivp_comm.sects[BASE_SECT_INDEX].acpu_addr << IVP_MMAP_SHIFT) +
				pdev->ivp_comm.sects[BASE_SECT_INDEX].len);
		ivp_info("vaddr[%#x]  memsz[%x]", program_header->p_vaddr,
			program_header->p_memsz);
		mem = ivp_vmap(ivp_ddr_addr, program_header->p_memsz, &offset);
	} else {
		mem = ivp_ioremap(pdev, program_header, algo_id, segment_type);
	}

	if (!mem) {
		ivp_err("can't map base address");
		return -EINVAL;
	}
	mem_addr = mem;
	if (segment_type == SEGMENT_DDR_TEXT) {
		if (memcpy_s(mem_addr, program_header->p_memsz, source,
			program_header->p_memsz) != EOK) {
			ivp_err("memcpy_s fail");
			vunmap(mem - offset);
			return -EINVAL;
		}
	} else {
		/* 4 stand for byte wide */
		for (i = 0; i < program_header->p_memsz / 4; i++)
			*(mem_addr + i) = *(source + i);
	}

	if (mem) {
		if (segment_type == SEGMENT_DDR_TEXT)
			vunmap(mem - offset);
		else
			iounmap(mem);
	}

	return 0;
}

static unsigned int ivp_get_algo_start(struct ivp_device *pdev,
		XT_Elf32_Ehdr *elf_head, unsigned int algo_id)
{
	return elf_head->e_entry +
		pdev->algo_mem_info[algo_id].algo_ddr_addr -
		pdev->algo_mem_info[algo_id].algo_ddr_vaddr;
}

static int ivp_get_dyn_info(struct ivp_device *pdev,
		XT_Elf32_Phdr *dynamic_header, const struct firmware *fw,
		unsigned int algo_id)
{
	errno_t ret;
	unsigned int index = 0;
	unsigned int offset;
	XT_Elf32_Dyn dyn_entry;

	do {
		offset = dynamic_header->p_offset + sizeof(XT_Elf32_Dyn) * index;
		ret = memcpy_s(&dyn_entry, sizeof(XT_Elf32_Dyn),
			fw->data + offset, sizeof(XT_Elf32_Dyn));
		if (ret != EOK) {
			ivp_err("memcpy_s dyn_entry failed");
			return -EINVAL;
		}
		switch ((Elf32_Word)dyn_entry.d_tag) {
		case DT_RELA:
			pdev->algo_mem_info[algo_id].algo_rel_addr =
				dyn_entry.d_un.d_ptr +
				pdev->algo_mem_info[algo_id].algo_ddr_addr -
				pdev->algo_mem_info[algo_id].algo_ddr_vaddr;
			break;
		case DT_RELASZ:
			pdev->algo_mem_info[algo_id].algo_rel_count =
				dyn_entry.d_un.d_val / sizeof(XT_Elf32_Rela);
			break;
		default:
			break;
		}
		index++;
	} while (dyn_entry.d_tag != DT_NULL);

	return 0;
}

/* get avaliable algo node and apply memory */
static int ivp_get_algo_node(struct ivp_device *pdev,
		const struct firmware *fw, XT_Elf32_Ehdr *elf_head)
{
	int ret;
	unsigned int i;
	unsigned int j;
	unsigned int offset;

	for (i = 0; i < IVP_ALGO_NODE_MAX; i++) {
		if (pdev->algo_mem_info[i].occupied == FREE) {
			XT_Elf32_Phdr header;
			init_algo_mem_info(pdev, i);
			for (j = 0; j < elf_head->e_phnum; j++) {
				offset = elf_head->e_phoff + elf_head->e_phentsize * j;
				ret = memcpy_s(&header, sizeof(header),
					fw->data + offset, elf_head->e_phentsize);
				if (ret != EOK) {
					ivp_err("memcpy_s program header failed[%x]", ret);
					return -EINVAL;
				}
				if (header.p_type != PT_LOAD || header.p_memsz == 0) {
					continue; /* support dynamic header */
				}
				ret = ivp_alloc_algo_mem(pdev, header, i);
				if (ret != 0) {
					ivp_err("alloc algo memory fail[%x]", ret);
					return ret;
				}
			}
			ivp_info("find available algo node [%u][%u]",
				pdev->core_id, i);
			return i;
		}
	}
	ivp_err("fail found available algo node");
	for (i = 0; i < IVP_ALGO_NODE_MAX; i++)
		ivp_err("[%u][%u] status[%u]", pdev->core_id, i,
			pdev->algo_mem_info[i].occupied);

	return -EINVAL;
}

static int ivp_load_foreach_segment(struct ivp_device *pdev,
		const struct firmware *fw, const XT_Elf32_Ehdr *elf_head,
		const int algo_id)
{
	errno_t ret;
	unsigned int i;
	XT_Elf32_Phdr program_header;
	unsigned int offset;

	for (i = 0; i < elf_head->e_phnum; i++) {
		offset = elf_head->e_phoff + elf_head->e_phentsize * i;
		ret = memcpy_s(&program_header, sizeof(program_header),
			fw->data + offset, elf_head->e_phentsize);
		if (ret != EOK) {
			ivp_err("memcpy_s program_header failed[%x]", ret);
			return -EINVAL;
		}
		if ((program_header.p_offset + program_header.p_memsz) > fw->size) {
			ivp_err("get invalid offset 0x%x", program_header.p_offset);
			return -EINVAL;
		}
		if (program_header.p_type == PT_LOAD && program_header.p_memsz > 0) {
			ret = ivp_load_algo_segment(pdev, fw, &program_header, algo_id);
			if (ret) {
				ivp_err("ivp_load_algo_segment failed");
				return -EINVAL;
			}
		} else if (program_header.p_type == PT_DYNAMIC) {
			ivp_get_dyn_info(pdev, &program_header, fw, algo_id);
		}
	}
	return EOK;
}

static int ivp_load_algo_firmware(struct ivp_device *pdev,
		const struct firmware *fw)
{
	int algo_id;
	errno_t ret;
	XT_Elf32_Ehdr elf_head;

	ivp_info("start load ivp algo");
	ret = memset_s((char *)&elf_head, sizeof(elf_head), 0, sizeof(elf_head));
	if (ret != EOK) {
		ivp_err("memset_s elf_head failed[%x]", ret);
		return ret;
	}
	ret = memcpy_s(&elf_head, sizeof(elf_head), fw->data, sizeof(elf_head));
	if (ret != EOK) {
		ivp_err("memcpy_s elf_head failed[%x]", ret);
		return ret;
	}

	algo_id = ivp_get_algo_node(pdev, fw, &elf_head);
	if (algo_id < 0) {
		ivp_err("get algo node failed");
		return algo_id;
	}

	ret = ivp_load_foreach_segment(pdev, fw, &elf_head, algo_id);
	if (ret != EOK) {
		ivp_err("load foreach segments failed");
		if (pdev->algo_mem_info[algo_id].algo_ddr_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DDR_TEXT);
		if (pdev->algo_mem_info[algo_id].algo_func_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_IRAM_TEXT);
		if (pdev->algo_mem_info[algo_id].algo_data_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_DATA);
		if (pdev->algo_mem_info[algo_id].algo_bss_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_BSS);
		return -EINVAL;
	}

	pdev->algo_mem_info[algo_id].algo_start_addr = ivp_get_algo_start(pdev,
		&elf_head, algo_id);
	pdev->algo_mem_info[algo_id].occupied = WORK;
	ivp_info("set algo[%x] status work", algo_id);

	return algo_id;
}

static int ivp_load_algo_image(struct ivp_device *pdev, const char *name)
{
	int ret;
	const struct firmware *firmware = NULL;
	struct device *dev = NULL;

	if (!name) {
		ivp_err("ivp image file name is invalid in function");
		return -EINVAL;
	}
	ivp_info("load algo name[%s]", name);
	dev = pdev->ivp_comm.device.this_device;
	if (!dev) {
		ivp_err("ivp miscdevice element struce device is null");
		return -EINVAL;
	}
	ret = request_firmware(&firmware, name, dev);
	if (ret) {
		ivp_err("request_firmware return error value %d for file (%s)",
			ret, name);
		return ret;
	}

	ret = ivp_load_algo_firmware(pdev, firmware);
	release_firmware(firmware);
	return ret;
}

static long ioctl_algo_info(struct file *fd, unsigned long args)
{
	long ret;
	struct ivp_algo_info info;
	struct ivp_device *pdev = NULL;

	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}
	ivp_info("current running core_id: %u", pdev->core_id);
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if (info.algo_id >= IVP_ALGO_NODE_MAX) {
		ivp_err("invalid algo id %d", info.algo_id);
		return -EINVAL;
	}

	info.algo_start = pdev->algo_mem_info[info.algo_id].algo_start_addr;
	info.rel_addr = pdev->algo_mem_info[info.algo_id].algo_rel_addr;
	info.rel_count = pdev->algo_mem_info[info.algo_id].algo_rel_count;
	info.ddr_addr = pdev->algo_mem_info[info.algo_id].algo_ddr_addr;
	info.ddr_vaddr = pdev->algo_mem_info[info.algo_id].algo_ddr_vaddr;
	info.ddr_size = pdev->algo_mem_info[info.algo_id].algo_ddr_size;
	info.func_addr = pdev->algo_mem_info[info.algo_id].algo_func_addr;
	info.func_vaddr = pdev->algo_mem_info[info.algo_id].algo_func_vaddr;
	info.func_size = pdev->algo_mem_info[info.algo_id].algo_func_size;
	info.data_addr = pdev->algo_mem_info[info.algo_id].algo_data_addr;
	info.data_vaddr = pdev->algo_mem_info[info.algo_id].algo_data_vaddr;
	info.data_size = pdev->algo_mem_info[info.algo_id].algo_data_size;
	info.bss_addr = pdev->algo_mem_info[info.algo_id].algo_bss_addr;
	info.bss_vaddr = pdev->algo_mem_info[info.algo_id].algo_bss_vaddr;
	info.bss_size = pdev->algo_mem_info[info.algo_id].algo_bss_size;

	ret = (long)copy_to_user((void *)(uintptr_t)args,
		&info, sizeof(struct ivp_algo_info));
	return ret;
}

static long ioctl_unload_algo(struct file *fd, unsigned long args)
{
	unsigned int algo_id;
	struct ivp_device *pdev = NULL;

	if (!fd || !args || !fd->private_data) {
		ivp_err("invalid input para");
		return -EINVAL;
	}

	pdev = fd->private_data;
	if ((pdev->core_id != IVP_CORE0_ID) &&
		(pdev->core_id != IVP_CORE1_ID)) {
		ivp_err("invalid input core id");
		return -EINVAL;
	}
	if (copy_from_user(&algo_id, (void *)(uintptr_t)args,
			sizeof(unsigned int)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}
	if (pdev->core_status != WORK ||
		algo_id >= IVP_ALGO_NODE_MAX) {
		ivp_err("invalid core[%x] algo[%x]",
			pdev->core_status, algo_id);
		return -EINVAL;
	}

	if (pdev->algo_mem_info[algo_id].occupied == WORK) {
		pdev->algo_mem_info[algo_id].occupied = FREE;
		if (pdev->algo_mem_info[algo_id].algo_ddr_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DDR_TEXT);
		if (pdev->algo_mem_info[algo_id].algo_func_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_IRAM_TEXT);
		if (pdev->algo_mem_info[algo_id].algo_data_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_DATA);
		if (pdev->algo_mem_info[algo_id].algo_bss_addr)
			ivp_free_algo_zone(pdev, algo_id, SEGMENT_DRAM0_BSS);

		init_algo_mem_info(pdev, algo_id);
		ivp_info("finish algo[%u][%u] unload", pdev->core_id, algo_id);
		return 0;
	}
	ivp_err("algo[%u][%u] is not work status", pdev->core_id, algo_id);
	return -EINVAL;
}

static long ioctl_load_algo(struct file *fd, unsigned long args)
{
	struct ivp_image_info info;
	struct ivp_device *pdev = NULL;

	if (!fd || !args || !fd->private_data) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}

	pdev = fd->private_data;
	mutex_lock(&pdev->ivp_comm.ivp_load_image_mutex);
	if (memset_s((char *)&info, sizeof(info), 0, sizeof(info)) != EOK) {
		ivp_err("memset_s fail");
		goto err_ivp_unlok;
	}

	if (copy_from_user(&info, (void __user *)(uintptr_t)args, sizeof(info))) {
		ivp_err("invalid input param size");
		goto err_ivp_unlok;
	}

	info.name[sizeof(info.name) - 1] = '\0';
	if ((info.length > (sizeof(info.name) - 1)) ||
		info.length != strlen(info.name)) {
		ivp_err("image file:but pass param length:%u", info.length);
		goto err_ivp_unlok;
	}

	if ((pdev->core_id != IVP_CORE0_ID) &&
		(pdev->core_id != IVP_CORE1_ID)) {
		ivp_err("invalid input core id");
		goto err_ivp_unlok;
	}

	if (pdev->core_status != WORK) {
		ivp_err("invalid core status");
		goto err_ivp_unlok;
	}

	info.algo_id = ivp_load_algo_image(pdev, info.name);
	mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);

	if (info.algo_id < 0 || info.algo_id >= IVP_ALGO_NODE_MAX) {
		ivp_err("invalid algo id %d", info.algo_id);
		return -EINVAL;
	}

	if (copy_to_user((void *)(uintptr_t)args, &info, sizeof(info))) {
		ivp_err("copy to user failed");
		return -ENOMEM;
	}

	ivp_info("finish algo [%u][%d] load", pdev->core_id, info.algo_id);
	return EOK;

err_ivp_unlok:
	mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
	return -EINVAL;
}

static int ivp_core_loaded_check(const struct ivp_device *pdev)
{
	/* two cores use same ddr-zone for text sections */
	if ((dyn_core_loaded[IVP_CORE0_ID] &&
			pdev->core_id == IVP_CORE1_ID) ||
			(dyn_core_loaded[IVP_CORE1_ID] &&
			pdev->core_id == IVP_CORE0_ID)) {
		ivp_info("core rodata sections had loaded");
		return 0;
	}
	return -1;
}

static void *ivp_ddr_vmap(struct ivp_device *pdev,
		const XT_Elf32_Phdr *program_header, unsigned int *offset)
{
	unsigned long ivp_ddr_addr;
	ivp_ddr_addr = (pdev->ivp_comm.sects[BASE_SECT_INDEX].acpu_addr <<
		IVP_MMAP_SHIFT) + program_header->p_vaddr -
		pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr;
	ivp_info("vaddr[%#x]  memsz[%x]", program_header->p_vaddr,
		program_header->p_memsz);
	return ivp_vmap(ivp_ddr_addr, program_header->p_memsz, offset);
}

static int ivp_load_core_segment(struct ivp_device *pdev,
		const struct firmware *fw, const XT_Elf32_Phdr *program_header)
{
	unsigned int *mem_addr = NULL;
	void *mem = NULL;
	bool ddr_flag = false;
	unsigned int offset = 0;
	unsigned int i;
	unsigned int *source = (unsigned int *)(fw->data + program_header->p_offset);

	if (program_header->p_vaddr >= pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr &&
		(program_header->p_vaddr <= (pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr +
		pdev->ivp_comm.sects[BASE_SECT_INDEX].len)))
		ddr_flag = true;

	if (ddr_flag) {
		if (!ivp_core_loaded_check(pdev))
			return 0;
		mem = ivp_ddr_vmap(pdev, program_header, &offset);
	} else {
		/*lint -e446 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		mem = ioremap_nocache(program_header->p_vaddr + pdev->base_offset,
				program_header->p_memsz);
#else
		mem = ioremap(program_header->p_vaddr + pdev->base_offset,
				program_header->p_memsz);
#endif
		/*lint +e446 */
		ivp_info("load xtensa memory at offset %#x", program_header->p_vaddr +
			pdev->base_offset);
	}

	if (!mem) {
		ivp_err("can't map base address");
		return -EINVAL;
	}
	mem_addr = mem;
	if (ddr_flag) {
		if (memcpy_s(mem_addr, program_header->p_memsz, source,
			program_header->p_memsz) != EOK) {
			ivp_err("memcpy_s fail");
			vunmap(mem - offset);
			return -EINVAL;
		}
	} else {
		/* 4 stand for byte wide */
		for (i = 0; i < program_header->p_memsz / 4; i++)
			*(mem_addr + i) = *(source + i);
	}

	if (mem) {
		if (ddr_flag)
			vunmap(mem - offset);
		else
			iounmap(mem);
	}

	return 0;
}

static int ivp_get_core_segment(struct ivp_device *pdev,
		const struct firmware *fw, const XT_Elf32_Phdr *program_header)
{
	unsigned int p_vaddr = program_header->p_vaddr + pdev->base_offset;

	if (((program_header->p_offset + program_header->p_memsz) > fw->size) ||
		program_header->p_type != PT_LOAD || program_header->p_memsz == 0) {
		ivp_err("program_header is abnormal");
		return -EINVAL;
	}
	if ((p_vaddr >= pdev->ivp_comm.sects[DRAM0_SECT_INDEX].ivp_addr &&
			(p_vaddr < (pdev->ivp_comm.sects[DRAM0_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[DRAM0_SECT_INDEX].len))) ||
		(p_vaddr >= pdev->ivp_comm.sects[DRAM1_SECT_INDEX].ivp_addr &&
			(p_vaddr < (pdev->ivp_comm.sects[DRAM1_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[DRAM1_SECT_INDEX].len))) ||
		(p_vaddr >= pdev->ivp_comm.sects[IRAM_SECT_INDEX].ivp_addr &&
			(p_vaddr < (pdev->ivp_comm.sects[IRAM_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[IRAM_SECT_INDEX].len))) ||
		(program_header->p_vaddr >= pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr &&
			(program_header->p_vaddr <
			(pdev->ivp_comm.sects[BASE_SECT_INDEX].ivp_addr +
			pdev->ivp_comm.sects[BASE_SECT_INDEX].len))))
		return EOK;

	ivp_err("get invalid addr");
	return -EINVAL;
}

static int ivp_load_core_firmware(struct ivp_device *pdev,
		const struct firmware *fw)
{
	errno_t ret;
	unsigned int i;
	XT_Elf32_Ehdr elf_head;

	ivp_info("start analyze ivp core[%u]", pdev->core_id);
	ret = memset_s((char *)&elf_head, sizeof(elf_head), 0, sizeof(elf_head));
	if (ret != EOK) {
		ivp_err("memset_s failed %d", ret);
		return ret;
	}
	ret = memcpy_s(&elf_head, sizeof(elf_head), fw->data, sizeof(elf_head));
	if (ret != EOK) {
		ivp_err("memcpy_s failed %d", ret);
		return ret;
	}

	/* Standard ELF head magic is :0x7f E L F */
	if (elf_head.e_ident[ELF_MAGIC0] != 0x7F ||
		elf_head.e_ident[ELF_MAGIC1] != 'E' ||
		elf_head.e_ident[ELF_MAGIC2] != 'L' ||
		elf_head.e_ident[ELF_MAGIC3] != 'F') {
		ivp_err("check ivp core elf head fail");
		return -EINVAL;
	}

	for (i = 0; i < elf_head.e_phnum; i++) {
		XT_Elf32_Phdr program_header;
		unsigned int offset = elf_head.e_phoff + elf_head.e_phentsize * i;
		ret = memcpy_s(&program_header, sizeof(program_header),
			fw->data + offset, elf_head.e_phentsize);
		if (ret != EOK) {
			ivp_err("memcpy_s failed %d", ret);
			return -EINVAL;
		}
		if (program_header.p_memsz == 0) {
			continue;
		}

		if (ivp_get_core_segment(pdev, fw, &program_header) != EOK) {
			ivp_err("get segment %u fails", i);
			return -EINVAL;
		}
		ret = ivp_load_core_segment(pdev, fw, &program_header);
		if (ret) {
			ivp_err("load core segment failed");
			return ret;
		}
	}
	return EOK;
}

static int ivp_load_core_image(struct ivp_device *pdev,
		struct ivp_image_info info)
{
	int ret;
	const struct firmware *firmware = NULL;
	struct device *dev = NULL;
	dev = pdev->ivp_comm.device.this_device;
	if (!dev) {
		ivp_err("ivp miscdevice element struce device is null");
		return -EINVAL;
	}
	ret = request_firmware(&firmware, info.name, dev);
	if (ret) {
		ivp_err("request_firmware return error value %d for file (%s)",
			ret, info.name);
		return ret;
	}
	ret = ivp_load_core_firmware(pdev, firmware);
	release_firmware(firmware);
	return ret;
}

static long ioctl_load_core(struct file *fd, unsigned long args)
{
	long ret;
	errno_t rc;
	struct ivp_image_info info;
	struct ivp_device *pdev = NULL;

	if (!fd || !args || !fd->private_data) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if ((pdev->core_id != IVP_CORE0_ID) &&
		(pdev->core_id != IVP_CORE1_ID)) {
		ivp_err("invalid input core id");
		return -EINVAL;
	}
	if (pdev->core_status != FREE) {
		ivp_err("invalid core status %u", pdev->core_status);
		return -EINVAL;
	}
	if (ivp_hw_query_runstall(pdev) == IVP_RUNSTALL_RUN) {
		ivp_err("invalid ivp status:ivp alredy run");
		return -EINVAL;
	}

	mutex_lock(&pdev->ivp_comm.ivp_load_image_mutex);
	rc = memset_s((char *)&info, sizeof(info), 0, sizeof(info));
	if (rc != EOK) {
		ivp_err("memset_s fail, rc:%d", rc);
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return rc;
	}
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	info.name[sizeof(info.name) - 1] = '\0';
	if (strcmp((const char *)info.name, IVP_DYNAMIC_CORE_NAME)) {
		ivp_err("image name is not %s", info.name);
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	ret = ivp_load_core_image(pdev, info);
	if (ret == EOK) {
		pdev->core_status = WORK;
		dyn_core_loaded[pdev->core_id] = true;
		ivp_info("load core[%u] success", pdev->core_id);
	}
	mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
	return ret;
}
#endif

#ifdef IVP_DUAL_CORE
const char *ivp_get_chip_type(void)
{
	const char *soc_spec = NULL;
	struct device_node *np = of_find_compatible_node(NULL, NULL, "hisilicon, soc_spec");
	if (!np) {
		ivp_err("no soc_spec dts node");
		return EFUSE_SOC_ERROR;
	}

	if (of_property_read_string(np, "soc_spec_set", &soc_spec)) {
		ivp_err("no soc_spec_set dts info");
		of_node_put(np);
		return EFUSE_SOC_ERROR;
	}
	of_node_put(np);
	ivp_info("soc_spec_set is %s", soc_spec);

	return soc_spec;
}

static void ivp_update_availabel_core(struct ivp_device *pdev)
{
	long ret;
	const char *dts_str = NULL;
	unsigned int buffer[EFUSE_IVP_BUFFER_NUM] = {0};

	dts_str = ivp_get_chip_type();
	if (!strncmp(dts_str, EFUSE_SOC_ERROR, strlen(EFUSE_SOC_ERROR)) ||
		!strncmp(dts_str, EFUSE_SOC_UNKNOWN, strlen(EFUSE_SOC_UNKNOWN))) {
		pdev->available_core = EFUSE_IVP_FAIL;
		ivp_err("soc spec is %s:", dts_str);
	} else if (!strncmp(dts_str, "normal", strlen("normal"))) {
		pdev->available_core = EFUSE_IVP_ALL_AVAILABLE;
	} else if (!strncmp(dts_str, "lite-normal", strlen("lite-normal")) ||
		!strncmp(dts_str, "wifi-only-normal", strlen("wifi-only-normal")) ||
		!strncmp(dts_str, "lite2-normal", strlen("lite2-normal")))  {
		pdev->available_core = 0x01;
	} else if (!strncmp(dts_str, "lite", strlen("lite")) ||
		!strncmp(dts_str, "wifi-only", strlen("wifi-only")) ||
		!strncmp(dts_str, "lite2", strlen("lite2"))) {
		/* from efuse bit 2276~2371 buffer[0]:bit31~2307 buffer[1]:bit0~2308 */
		ret = efuse_read_value(buffer, EFUSE_IVP_BUFFER_NUM,
			EFUSE_FN_RD_PARTIAL_PASSP2);
		if (ret != EOK) {
			ivp_err("efuse_read_value fail, ret:%lx", ret);
			pdev->available_core = EFUSE_IVP_FAIL;
			return;
		}
		pdev->available_core |=
			(buffer[efuse_buf_pos(EFUSE_IVP_CORE0_POS)] >>
			efuse_bit_pos(EFUSE_IVP_CORE0_POS)) & 0x01;
		if (pdev->available_core == 0)
			pdev->available_core |=
				((buffer[efuse_buf_pos(EFUSE_IVP_CORE1_POS)] >>
				efuse_bit_pos(EFUSE_IVP_CORE1_POS)) & 0x01) << 1;
	} else {
		pdev->available_core = 0;
	}

	return;
}
#endif

static long ioctl_available_core(struct file *fd, unsigned long args)
{
	long ret;
	struct ivp_device *pdev = NULL;
	unsigned int ret_arg;
#ifdef IVP_DUAL_CORE
	static char updated_core_num = 0;
#endif

	if (!fd) {
		ivp_err("invalid input fd");
		return -EINVAL;
	}
	pdev = fd->private_data;
	if (!args || !pdev) {
		ivp_err("invalid input args or odev");
		return -EINVAL;
	}
#ifdef IVP_DUAL_CORE
	if (updated_core_num == 0) {
		ivp_update_availabel_core(pdev);
		updated_core_num = 1;
	}
	ret_arg = pdev->available_core;
#else
	ret_arg = IVP_CORE0_AVAILABLE;
#endif
	/* bit 0: core0, bit1: core1; 0 fail, 1 pass */
	ivp_info("get available_core:%#x", ret_arg);
	ret = (long)copy_to_user((void *)(uintptr_t)args,
		&ret_arg, sizeof(ret_arg));

	return ret;
}

static int ivp_load_image(struct ivp_device *pdev, const char *name)
{
	int ret;
	const struct firmware *firmware = NULL;
	struct device *dev = NULL;

	if (!name) {
		ivp_err("ivp image file is invalid");
		return -EINVAL;
	}

	dev = pdev->ivp_comm.device.this_device;
	if (!dev) {
		ivp_err("ivp miscdevice is invalid");
		return -EINVAL;
	}
	ret = request_firmware(&firmware, name, dev);
	if (ret) {
		ivp_err("return error:%d for file:%s", ret, name);
		return ret;
	}

	ret = ivp_check_image(firmware);
	if (ret != 0) {
		release_firmware(firmware);
		ivp_err("check ivp image %s fail value 0x%x ", name, ret);
		return ret;
	}

	ret = ivp_load_firmware(pdev, name, firmware);
	release_firmware(firmware);
	return ret;
}

static long ioctl_load_firmware(struct file *fd, unsigned long args)
{
	long ret;
	struct ivp_image_info info;
	errno_t rc;
	struct ivp_device *pdev = fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	if (ivp_hw_query_runstall(pdev) == IVP_RUNSTALL_RUN) {
		ivp_err("invalid ivp status:ivp alredy run");
		return -EINVAL;
	}
	mutex_lock(&pdev->ivp_comm.ivp_load_image_mutex);
	rc = memset_s((char *)&info, sizeof(info), 0, sizeof(info));
	if (rc != EOK) {
		ivp_err("memcpy_s fail, rc:%d", rc);
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	if (copy_from_user(&info, (void *)(uintptr_t)args, sizeof(info)) != 0) {
		ivp_err("invalid input param size");
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	info.name[sizeof(info.name) - 1] = '\0';
	if ((info.length > (sizeof(info.name) - 1)) ||
		info.length <= IVP_IMAGE_SUFFIX_LENGTH ||
		info.length != strlen(info.name)) {
		ivp_err("image file:but pass param length:%d", info.length);
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	if (strcmp((const char *)&info.name[info.length - IVP_IMAGE_SUFFIX_LENGTH],
		IVP_IMAGE_SUFFIX)) {
		ivp_err("image is not bin file");
		mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
		return -EINVAL;
	}
	if (pdev->ivp_comm.ivp_secmode == SECURE_MODE)
		ret = ivp_sec_loadimage(pdev);
	else
		ret = ivp_load_image(pdev, info.name);

	mutex_unlock(&pdev->ivp_comm.ivp_load_image_mutex);
	return ret;
}

static long ioctl_dsp_run(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}
	if (ivp_check_resethandler(pdev) == 1)
		ivp_dev_run(pdev);
	else
		ivp_err("ivp image not upload");

	return EOK;
}

static long ioctl_dsp_suspend(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	long ret;
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ret = ivp_dev_suspend(pdev);

	return ret;
}

static long ioctl_dsp_resume(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	if (ivp_check_resethandler(pdev) == 1) {
		ivp_dev_resume(pdev);
		return EOK;
	} else {
		ivp_err("ivp image not upload");
		return -ENODEV;
	}
}

static long ioctl_dsp_stop(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ivp_dev_stop(pdev);

	return EOK;
}

static long ioctl_query_runstall(struct file *fd, unsigned long args)
{
	long ret;
	unsigned int runstall;
	struct ivp_device *pdev = fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	runstall = (u32)ivp_hw_query_runstall(pdev);
	ret = (long)copy_to_user((void *)(uintptr_t)args,
			&runstall, sizeof(runstall));

	return ret;
}

static long ioctl_query_waiti(struct file *fd, unsigned long args)
{
	long ret;
	unsigned int waiti;
	struct ivp_device *pdev = fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	waiti = (u32)ivp_hw_query_waitmode(pdev);
	ret = (long)copy_to_user((void *)(uintptr_t)args, &waiti, sizeof(waiti));

	return ret;
}

static long ioctl_trigger_nmi(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ivp_hw_trigger_nmi(pdev);

	return EOK;
}

static long ioctl_watchdog(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	long ret;
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ret = ivp_dev_keep_on_wdg(pdev);

	return ret;
}

static long ioctl_watchdog_sleep(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ivp_dev_sleep_wdg(pdev);

	return EOK;
}

static long ioctl_smmu_invalidate_tlb(struct file *fd,
		unsigned long args __attribute__((unused)))
{
#ifdef CONFIG_IVP_SMMU
	struct ivp_device *pdev = fd->private_data;
	long ret;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ret = ivp_dev_smmu_invalid_tlb(pdev);

	return ret;
#else
	(void)fd;
	return 0;
#endif
}

static long ioctl_bm_init(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ivp_dev_hwa_enable(pdev);
	return EOK;
}

static long ioctl_clk_level(struct file *fd, unsigned long args)
{
	unsigned int level = 0;
	struct ivp_device *pdev = fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	if (copy_from_user(&level, (void *)(uintptr_t)args,
				sizeof(unsigned int)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	ivp_change_clk(pdev, level);
	return EOK;
}

static long ioctl_dump_dsp_status(struct file *fd,
		unsigned long args __attribute__((unused)))
{
	struct ivp_device *pdev = fd->private_data;

	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}

	ivp_dump_status(pdev);

	return EOK;
}

#ifdef IVP_CHIPTYPE_SUPPORT
inline void ivp_write_chip_type_value(struct ivp_device *pdev,
		unsigned int chip_type)
{
	ivp_reg_write(pdev, IVP_REG_OFF_RESEVER_REG, chip_type);
}

static long ioctl_query_chip_type(struct file *fd, unsigned long args)
{
	unsigned int chip_type = 0;
	struct ivp_device *pdev = fd->private_data;

	if (!args || !pdev) {
		ivp_err("invalid input args or pdev");
		return -EINVAL;
	}

	if (copy_from_user(&chip_type, (void *)(uintptr_t)args,
			sizeof(unsigned int)) != 0) {
		ivp_err("invalid input param size");
		return -EINVAL;
	}

	if ((chip_type == V250_CS2_ID) || (chip_type == V250_CS1_ID)) {
		ivp_write_chip_type_value(pdev, chip_type);
	} else {
		ivp_err("invalid chiptype value");
		return -EINVAL;
	}

	return EOK;
}
#endif

typedef long (*ivp_ioctl_func_t)(struct file *fd, unsigned long args);
struct ivp_ioctl_ops {
	unsigned int cmd;
	ivp_ioctl_func_t func;
};

#define IVP_IOCTL_OPS(_cmd, _func) {.cmd = (_cmd), .func = (_func)}

/* ioctl_normal_ops do not depend on ivp powered up */
static struct ivp_ioctl_ops ioctl_normal_ops[] = {
	IVP_IOCTL_OPS(IVP_IOCTL_SECTINFO, ioctl_sect_info),
	IVP_IOCTL_OPS(IVP_IOCTL_SECTCOUNT, ioctl_sect_count),
	IVP_IOCTL_OPS(IVP_IOCTL_POWER_UP, ioctl_power_up),
	IVP_IOCTL_OPS(IVP_IOCTL_HIDL_MAP, ioctl_hidl_map),
	IVP_IOCTL_OPS(IVP_IOCTL_HIDL_UNMAP, ioctl_hidl_unmap),
	IVP_IOCTL_OPS(IVP_IOCTL_AVAILABLE_CORE, ioctl_available_core),
};

/* ioctl_actived_ops must be executed after powered up */
static struct ivp_ioctl_ops ioctl_actived_ops[] = {
	IVP_IOCTL_OPS(IVP_IOCTL_POWER_DOWN, ioctl_power_down),
	IVP_IOCTL_OPS(IVP_IOCTL_LOAD_FIRMWARE, ioctl_load_firmware),
#ifdef MULTIPLE_ALGO
	IVP_IOCTL_OPS(IVP_IOCTL_LOAD_CORE, ioctl_load_core),
	IVP_IOCTL_OPS(IVP_IOCTL_LOAD_ALGO, ioctl_load_algo),
	IVP_IOCTL_OPS(IVP_IOCTL_UNLOAD_ALGO, ioctl_unload_algo),
	IVP_IOCTL_OPS(IVP_IOCTL_ALGOINFO, ioctl_algo_info),
#endif
	IVP_IOCTL_OPS(IVP_IOCTL_DSP_RUN, ioctl_dsp_run),
	IVP_IOCTL_OPS(IVP_IOCTL_DSP_SUSPEND, ioctl_dsp_suspend),
	IVP_IOCTL_OPS(IVP_IOCTL_DSP_RESUME, ioctl_dsp_resume),
	IVP_IOCTL_OPS(IVP_IOCTL_DSP_STOP, ioctl_dsp_stop),
	IVP_IOCTL_OPS(IVP_IOCTL_QUERY_RUNSTALL, ioctl_query_runstall),
	IVP_IOCTL_OPS(IVP_IOCTL_QUERY_WAITI, ioctl_query_waiti),
	IVP_IOCTL_OPS(IVP_IOCTL_TRIGGER_NMI, ioctl_trigger_nmi),
	IVP_IOCTL_OPS(IVP_IOCTL_WATCHDOG, ioctl_watchdog),
	IVP_IOCTL_OPS(IVP_IOCTL_WATCHDOG_SLEEP, ioctl_watchdog_sleep),
	IVP_IOCTL_OPS(IVP_IOCTL_BM_INIT, ioctl_bm_init),
	IVP_IOCTL_OPS(IVP_IOCTL_CLK_LEVEL, ioctl_clk_level),
	IVP_IOCTL_OPS(IVP_IOCTL_DUMP_DSP_STATUS, ioctl_dump_dsp_status),
	IVP_IOCTL_OPS(IVP_IOCTL_SMMU_INVALIDATE_TLB, ioctl_smmu_invalidate_tlb),
#ifdef IVP_CHIPTYPE_SUPPORT
	IVP_IOCTL_OPS(IVP_IOCTL_QUERY_CHIP_TYPE, ioctl_query_chip_type),
#endif
	IVP_IOCTL_OPS(IVP_IOCTL_FDINFO, ioctl_fd_info),
};

static ivp_ioctl_func_t ivp_get_ioctl_func(unsigned int cmd,
		struct ivp_ioctl_ops *ops_tbl, uint32_t tbl_size)
{
	uint32_t idx;
	for (idx = 0; idx < tbl_size; idx++) {
		if (cmd == ops_tbl[idx].cmd)
			return ops_tbl[idx].func;
	}
	return NULL;
}

long ivp_ioctl(struct file *fd, unsigned int cmd, unsigned long args)
{
	long ret;
	ivp_ioctl_func_t func = NULL;
	struct ivp_device *pdev = NULL;
	if (!fd) {
		ivp_err("invalid input param fd");
		return -EINVAL;
	}
	ivp_info("received ioctl command(0x%08x)", cmd);

	func = ivp_get_ioctl_func(cmd, ioctl_normal_ops,
		ARRAY_SIZE(ioctl_normal_ops));
	if (func)
		return func(fd, args);
	pdev = fd->private_data;
	if (!pdev) {
		ivp_err("invalid param pdev");
		return -EINVAL;
	}
	mutex_lock(&pdev->ivp_comm.ivp_ioctl_mutex);
	if (atomic_read(&pdev->ivp_comm.poweron_success) != 0) {
		mutex_unlock(&pdev->ivp_comm.ivp_ioctl_mutex);
		ivp_err("ioctl cmd is error %u since ivp not power", cmd);
		return -EINVAL;
	}
	func = ivp_get_ioctl_func(cmd, ioctl_actived_ops,
		ARRAY_SIZE(ioctl_actived_ops));
	if (!func) {
		ivp_err("invalid ioctl command(0x%08x) received", cmd);
		mutex_unlock(&pdev->ivp_comm.ivp_ioctl_mutex);
		return -EINVAL;
	}
	if (cmd == IVP_IOCTL_WATCHDOG)
		mutex_unlock(&pdev->ivp_comm.ivp_ioctl_mutex);
	ret = func(fd, args);
	if (cmd != IVP_IOCTL_WATCHDOG)
		mutex_unlock(&pdev->ivp_comm.ivp_ioctl_mutex);
	return ret;
}
