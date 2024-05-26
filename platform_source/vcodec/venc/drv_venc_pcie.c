/*
 * drv_venc_pcie.c
 *
 * This is for test of pcie link mode
 *
 * Copyright (c) 2021-2021 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/iommu.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <platform_include/basicplatform/linux/pcie-kport-api.h>
#include "smmu.h"
#include "drv_common.h"
#ifdef VENC_MCORE_ENABLE
#include "drv_venc_mcore.h"
#include "drv_venc_ipc.h"
#endif
#include "venc_regulator.h"

#define VENC_PCIE_RC_ID             0x00 /* fpga: 0x0, udp: 0x1 */
#define VENC_PCIE_BAR_ID            0x00
#define VENC_PCIE_REMAP_TARGET_ADDR 0xe9000000
#define VENC_PCIE_VENDOR        0x19E5
#define VENC_PCIE_DEVICE        0x369B
#define VENC_PCIE_SUBVENDER     0
#define VENC_PCIE_SUBDEVICE     0
#define VENC_PCIE_CLASS         0
#define VENC_PCIE_CLASS_MASK    0
#define VENC_PCIE_DRIVER_DATA   0
#define VENC_PCIE_IRQ_OFFSET    0

enum MCU_IRQ {
	INTR_VENC_NORM_TO_GIC = 0,
	INTR_VENC_WATCHDOG_TO_GIC,
	INTR_VENC_IPC_INT_0_TO_GIC,
	INTR_VENC_IPC_MBX_0_TO_GIC
};

static struct pci_device_id venc_pcie_devid[] = {
	{
		vendor        :     VENC_PCIE_VENDOR,
		device        :     VENC_PCIE_DEVICE,
		subvendor     :     VENC_PCIE_SUBVENDER,
		subdevice     :     VENC_PCIE_SUBDEVICE,
		class         :     VENC_PCIE_CLASS,
		class_mask    :     VENC_PCIE_CLASS_MASK,
		driver_data   :     VENC_PCIE_DRIVER_DATA,
	},
	{ 0, }
};

uint32_t *g_reg_base = NULL;
extern struct venc_config g_venc_config;

MODULE_DEVICE_TABLE(pci, venc_pcie_devid);

int32_t venc_get_dts_config_info(struct platform_device *pdev)
{
	g_venc_config.venc_conf_com.core_num = 1;
	g_venc_config.venc_conf_com.fpga_flag = 1;
	// main function address in mcu(defined by ENTRY_ADDR)
	g_venc_config.venc_conf_com.mcore_code_base = 0xe92b0000 + 0x200;
	return 0;
}

int32_t venc_regulator_update(struct clock_info *clock_info)
{
	return 0;
}

static int32_t write_power_node(char *node_name)
{
	const char *cmd = "1";
	mm_segment_t oldfs;
	struct file *file = filp_open(node_name, O_RDWR, 0);
	if (IS_ERR(file)) {
		VCODEC_ERR_VENC("open %s failed", node_name);
		return VCODEC_FAILURE;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(file, cmd, strlen(cmd), &file->f_pos);

	set_fs(oldfs);
	filp_close(file, NULL);
	return 0;
}

int32_t process_encode_timeout(void)
{
	uint32_t i;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	for (i = 0; i < g_venc_config.venc_conf_com.core_num; i++) {
		if (venc->ctx[i].status == VENC_TIME_OUT) {
#ifdef VENC_MCORE_ENABLE
			venc_mcore_log_dump();
#endif
			VCODEC_WARN_VENC("core_id: %d timeout", i);
		}
	}

	return 0;
}

int32_t venc_regulator_enable(void)
{
	int32_t ret = 0;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	struct venc_context *ctx = &venc->ctx[0];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ctx->core_id = 0;
	osal_init_timer(&venc->ctx[0].timer, venc->ops.encode_timeout);//lint !e571
#else
	osal_init_timer(&venc->ctx[0].timer, venc->ops.encode_timeout, 0);
#endif
	/* The regulator enable step
	* 1: poweron venc by script
	* 2: poweron tbu by code
	*/
#ifdef SMMU_V3
	if (venc_smmu_tbu_init(0))
		goto del_timer;
#endif
	ctx->status = VENC_IDLE;
	ctx->first_cfg_flag = true;

	return 0;

del_timer:
	osal_del_timer(&venc->ctx[0].timer, true);

	VCODEC_ERR_VENC("regulator enable failed");
	return VCODEC_FAILURE;
}

int32_t venc_regulator_disable(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
#ifdef SMMU_V3
	venc_smmu_tbu_deinit();
#endif
	if (osal_del_timer(&venc->ctx[0].timer, true) == 0)
		VCODEC_WARN_VENC("timer is pending, when power off");

	return 0;
}
#ifdef VENC_MCORE_ENABLE
irqreturn_t pcie_ipc_mbx0_int(int32_t irq, void *dev_id)
{
	VCODEC_FATAL_VENC("ipc_mbx0 irq number:%d", irq);
	venc_ipc_recive_msg(MBX0);
	return IRQ_HANDLED;
}

irqreturn_t pcie_ipc_int0_int(int32_t irq, void *dev_id)
{
	VCODEC_FATAL_VENC("ipc_int0 irq number:%d", irq);
	venc_ipc_recive_ack(MBX2);
	return IRQ_HANDLED;
}

irqreturn_t pcie_watchdog_int(int32_t irq, void *dev_id)
{
	VCODEC_FATAL_VENC("watchdog irq number:%d", irq);
	return IRQ_HANDLED;
}
#endif

irqreturn_t pcie_encode_done(int32_t irq, void *dev_id)
{
	VCODEC_FATAL_VENC("encode done irq number:%d", irq);
	venc_drv_encode_done(irq, dev_id);
	return IRQ_HANDLED;
}

static int venc_pcie_irq_request(struct pci_dev *pdev)
{
	int ret;
	uint32_t irq_num;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	int nvec = pci_alloc_irq_vectors(pdev, 1, 4, PCI_IRQ_MSI);
	if (nvec != 4) {
		VCODEC_FATAL_VENC("pci enable msi range failed %d", nvec);
		return VCODEC_FAILURE;
	}

	irq_num = pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_NORM_TO_GIC;
	ret = request_irq(irq_num, pcie_encode_done, 0, "pcie_device", NULL);
	if (ret < 0) {
		VCODEC_FATAL_VENC("request encode irq failed");
		goto error5;
	}
	venc->ctx[0].irq_num.normal = irq_num;
#ifdef VENC_MCORE_ENABLE
	ret = venc_ipc_init(&venc->ctx[0]);
	if (ret != 0) {
		VCODEC_FATAL_VENC("ipc init failed");
		goto error4;
	}

	irq_num = pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_WATCHDOG_TO_GIC;
	ret = request_irq(irq_num, pcie_watchdog_int, 0, "pcie_device", NULL);
	if (ret < 0) {
		VCODEC_FATAL_VENC("request wtdg irq failed");
		goto error3;
	}

	irq_num = pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_IPC_INT_0_TO_GIC;
	ret = request_irq(irq_num, pcie_ipc_int0_int, 0, "pcie_device", NULL);
	if (ret < 0) {
		VCODEC_FATAL_VENC("request ipc_int0_iqr irq failed");
		goto error2;
	}

	irq_num = pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_IPC_MBX_0_TO_GIC;
	ret = request_irq(irq_num, pcie_ipc_mbx0_int, 0, "pcie_device", NULL);
	if (ret < 0) {
		VCODEC_FATAL_VENC("request ipc_mbx0_iqr irq failed");
		goto error1;
	}
#endif
	return 0;

#ifdef VENC_MCORE_ENABLE
error1:
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_IPC_INT_0_TO_GIC, NULL);
error2:
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_WATCHDOG_TO_GIC, NULL);
error3:
	venc_ipc_deinit(&venc->ctx[0]);
error4:
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_NORM_TO_GIC, NULL);
#endif
error5:
	pci_free_irq_vectors(pdev);

	return VCODEC_FAILURE;
}

static void venc_pcie_irq_free(struct pci_dev *pdev)
{
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_NORM_TO_GIC, NULL);
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_WATCHDOG_TO_GIC, NULL);
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_IPC_INT_0_TO_GIC, NULL);
	free_irq(pdev->irq + VENC_PCIE_IRQ_OFFSET + INTR_VENC_IPC_MBX_0_TO_GIC, NULL);
	pci_free_irq_vectors(pdev);
}

static void venc_pcie_remove(struct pci_dev *pdev)
{
	venc_pcie_irq_free(pdev);
	vcodec_munmap(g_reg_base);
	pci_disable_device(pdev);
}

static int venc_pcie_suspend(struct pci_dev *pdev, pm_message_t state)
{
	VCODEC_INFO_VENC("enter suspend");
	return 0;
}

static int venc_pcie_resume(struct pci_dev *pdev)
{
	VCODEC_INFO_VENC("enter resume");
	return 0;
}

static int venc_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	u64 bar_addr;
	venc_entry_t *venc = NULL;

	if (pci_enable_device(pdev)) {
		VCODEC_ERR_VENC("can't enable PCI device");
		return VCODEC_FAILURE;
	}

	venc = platform_get_drvdata(venc_get_device());
	if (venc == NULL) {
		VCODEC_ERR_VENC("venc pcide device ptr is NULL");
		goto err;
	}

	pci_set_master(pdev);
	pcie_kport_enumerate(VENC_PCIE_RC_ID);
	bar_addr = pcie_set_mem_outbound(VENC_PCIE_RC_ID, pdev, VENC_PCIE_BAR_ID, VENC_PCIE_REMAP_TARGET_ADDR);
	if (bar_addr == 0) {
		VCODEC_ERR_VENC("pcie set mem outbound failed");
		goto err;
	}

	g_reg_base = (uint32_t *)vcodec_mmap((uint32_t) bar_addr, 0x1000000);
	if (!g_reg_base) {
		VCODEC_ERR_VENC("pcie mem map failed");
		goto err;
	}
	venc->ctx[0].reg_base = g_reg_base + 0x280000 / 4;
	VCODEC_INFO_VENC("pcie base addr 0x%x, regbase= %pK", bar_addr, venc->ctx[0].reg_base);

	venc_get_dts_config_info(venc_get_device());
	if (venc_pcie_irq_request(pdev)) {
		VCODEC_INFO_VENC("PCIe request irq failed");
		vcodec_munmap(g_reg_base);
		goto err;
	}

	VCODEC_INFO_VENC("pcie venc drv probe succuess\n");
	return 0;
err:
	pci_disable_device(pdev);
	return VCODEC_FAILURE;
}

static struct pci_driver venc_pcie_driver = {
	node:      {},
	name:      "venc_pcie",
	id_table:  venc_pcie_devid,
	probe:     venc_pcie_probe,
	remove:    venc_pcie_remove,
	suspend:   venc_pcie_suspend,
	resume:    venc_pcie_resume,
};

static int __init venc_pcie_device_init(void)
{
	int ret = pci_register_driver(&venc_pcie_driver);
	if (!ret)
		VCODEC_INFO_VENC("init success");

	return ret;
}

static void __exit venc_pcie_device_exit(void)
{
	pci_unregister_driver(&venc_pcie_driver);
	VCODEC_INFO_VENC("exit");
}

module_init(venc_pcie_device_init);
module_exit(venc_pcie_device_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VENC PCIE DRIVER");
MODULE_VERSION("1.0");
