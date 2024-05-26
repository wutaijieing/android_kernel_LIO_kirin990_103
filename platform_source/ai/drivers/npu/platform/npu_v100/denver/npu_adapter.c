/*
 * npu_adapter.c
 *
 * about npu adapter
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
#include "npu_adapter.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/dma-direction.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/platform_drivers/mm_svm.h>
#include <securec.h>

#include "npu_platform_resource.h"
#include "npu_platform_register.h"
#include "npu_common.h"
#include "dts/npu_reg.h"
#include "npu_pm_framework.h"
#include <securec.h>
#include "npu_svm.h"

int npu_plat_powerup_smmu(struct device *dev)
{
	int ret;
	unused(dev);
	ret = mm_smmu_poweron(0);
	if (ret != 0) {
		npu_drv_err("mm smmu poweron failed\n");
		return ret;
	}

	ret = npu_plat_powerup_tbu();
	if (ret != 0) {
		npu_drv_warn("powerup tbu failed ret = %d\n", ret);
		return ret;
	}
	return 0;
}
int npu_plat_svm_bind(struct npu_dev_ctx *dev_ctx,
	struct task_struct *task, void **svm_dev)
{
	*svm_dev = (void*)mm_svm_bind_task(dev_ctx->npu_dev, task);
	if (*svm_dev == NULL) {
		npu_drv_err("mm svm bind task failed \n");
		return -EBUSY; // likely bound by other process
	}
	return 0;
}

int npu_plat_pm_powerup(struct npu_dev_ctx *dev_ctx, u32 work_mode)
{
	int ret = 0;
	u32 *stage = NULL;
	npu_drv_boot_time_tag("start powerup till_npucpu \n");
	cond_return_error(dev_ctx == NULL, -EINVAL, "invalid para\n");

	stage = &dev_ctx->power_stage;

	if (*stage == NPU_PM_DOWN) {
		ret = npu_plat_powerup_till_npucpu(work_mode);
		if (ret != 0) {
			npu_drv_warn("powerup till npucpu failed ,ret=%d\n",
				ret);
			goto failed;
		}
		*stage = NPU_PM_NPUCPU;
	}
	npu_drv_boot_time_tag("start powerup till ts \n");

	if (*stage == NPU_PM_NPUCPU) {
		ret = npu_plat_powerup_till_ts(work_mode, NPU_SC_TESTREG0_OFFSET);
		if (ret != 0) {
			npu_drv_warn("powerup till ts failed ret=%d\n", ret);
			goto ts_failed;
		}
		*stage = NPU_PM_TS;
	}
	npu_drv_boot_time_tag("start powerup smmu \n");

	if (*stage == NPU_PM_TS) {
		// power up smmu in non_secure npu mode
		if (work_mode != NPU_SEC) {
			ret = npu_plat_powerup_smmu(dev_ctx->npu_dev);
			cond_goto_error(ret != 0, smmu_failed, ret, ret,
				"powerup smmu failed ret=%d\n", ret);
		} else {
			npu_drv_warn("secure power up ,no need to power up smmu"
				"in linux non_secure world, smmu power up will"
				"be excuted on tee secure world \n");
		}
	}
	*stage = NPU_PM_UP;
	npu_plat_set_npu_power_status(DRV_NPU_POWER_ON_FLAG);
	npu_drv_warn("pm powerup success \n");
	return 0;

smmu_failed:
	(void)npu_plat_powerdown_ts(NPU_SC_TESTREG8_OFFSET, work_mode);
	// continue even if gic grace exit failed
	*stage = NPU_PM_NPUCPU;
ts_failed:
	(void)npu_plat_powerdown_npucpu(0x1 << 5, work_mode);
	(void)npu_plat_powerdown_nputop();
	*stage = NPU_PM_DOWN;
failed:
	return ret;
}

int npu_plat_pm_open(uint32_t devid)
{
	unused(devid);
	if (npu_plat_ioremap(NPU_REG_POWER_STATUS) != 0) {
		npu_drv_err("ioremap failed\n");
		return -1;
	}
	return 0;
}

int npu_plat_pm_release(uint32_t devid)
{
	unused(devid);
	npu_plat_iounmap(NPU_REG_POWER_STATUS);
	return 0;
}

int npu_plat_res_mailbox_send(void *mailbox, int mailbox_len,
	const void *message, int message_len)
{
	u8 *message_buf = NULL;
	int ret;

	cond_return_error(message_len > mailbox_len, -1,
		"message len =%d, too long", message_len);

	message_buf = vmalloc(mailbox_len);
	cond_return_error(message_buf == NULL, -1, "message buf alloc failed");

	ret = memset_s(message_buf, mailbox_len, 0, mailbox_len);
	cond_goto_error(ret != 0, OUT, ret, ret, "memset failed. ret=%d\n", ret);

	ret = memcpy_s(message_buf, mailbox_len, message, mailbox_len);
	cond_goto_error(ret != 0, OUT, ret, ret, "memcpy failed. ret=%d\n", ret);

	memcpy_toio(mailbox, message_buf, mailbox_len);
	mb();
OUT:
	vfree(message_buf);
	return ret;
}

void __iomem *npu_plat_sram_remap(struct platform_device *pdev,
	resource_size_t sram_addr, resource_size_t sram_size)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	return devm_ioremap(&pdev->dev, sram_addr, sram_size);
#else
	return devm_ioremap_nocache(&pdev->dev, sram_addr, sram_size);
#endif
}

void npu_plat_sram_unmap(struct platform_device *pdev, void *sram_addr)
{
	devm_iounmap(&pdev->dev, (void __iomem*)sram_addr);
	return;
}

int npu_plat_poweroff_smmu(uint32_t devid)
{
	npu_clear_pid_ssid_table(devid, 0, 0);
	npu_plat_powerdown_tbu(0);
	(void)mm_smmu_poweroff(0); /* 0: sysdma smmu */
	return 0;
}

int npu_plat_pm_powerdown(uint32_t devid, u32 work_mode, u32 *stage)
{
	int ret = 0;
	npu_plat_set_npu_power_status(DRV_NPU_POWER_OFF_FLAG);

	if (*stage == NPU_PM_UP) {
		if (work_mode != NPU_SEC) {
			ret = npu_plat_poweroff_smmu(devid);
			if (ret != 0)
				npu_drv_err("poweroff smmu failed\n");
		} else {
			npu_drv_warn("secure power down ,no need to power down smmu"
				"in linux non_secure world, smmu power down has"
				"been excuted on tee secure world \n");
		}

		*stage = NPU_PM_TS;
	}

	if (*stage == NPU_PM_TS) {
		ret = npu_plat_powerdown_ts(NPU_SC_TESTREG8_OFFSET, work_mode);
		// continue even if gic grace exit failed
		*stage = NPU_PM_NPUCPU;
	}

	if (*stage == NPU_PM_NPUCPU) {
		ret = npu_plat_powerdown_npucpu(0x1<<5, work_mode);
		if (ret != 0)
			npu_drv_err("pm power down fail\n");
		ret = npu_plat_powerdown_nputop();
		if (ret != 0)
			npu_drv_err("power down nputop fail\n");
	}
	*stage = NPU_PM_DOWN;

	npu_drv_warn("pm power down success\n");

	return ret;
}

int npu_plat_res_ctrl_core(struct npu_dev_ctx *dev_ctx, u32 core_num)
{
	unused(dev_ctx);
	unused(core_num);
	return 0;
}

int npu_testreg_poll_wait(u64 offset)
{
	unused(offset);
	return -1;
}

int npu_testreg_clr(u64 offset)
{
	unused(offset);
	return 0;
}

int npu_smmu_evt_register_notify(struct notifier_block *n)
{
	return mm_smmu_evt_register_notify(n);
}

int npu_smmu_evt_unregister_notify(struct notifier_block *n)
{
	return mm_smmu_evt_unregister_notify(n);
}

int npu_plat_switch_hwts_aicore_pool(struct npu_dev_ctx *dev_ctx,
	struct npu_work_mode_info *work_mode_info, uint32_t power_status)
{
	unused(dev_ctx);
	unused(work_mode_info);
	unused(power_status);
	return 0;
}

void npu_plat_aicore_pmu_enable(uint32_t subip_set)
{
	unused(subip_set);
	return;
}

void npu_plat_mntn_reset(void)
{
	return;
}

int npu_plat_dev_pm_suspend(void)
{
	return 0;
}

void npu_exception_timeout_record(struct npu_dev_ctx *dev_ctx)
{
	(void)(dev_ctx);
	return;
}

int npu_plat_bypass_status(void)
{
	return NPU_NON_BYPASS;
}
