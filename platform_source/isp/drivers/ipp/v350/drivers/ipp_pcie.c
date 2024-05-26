/******************************************************************
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2018. All rights reserved.
 * File name    ipp_core.C
 * Description:
 *
 * Date         2020-04-16 17:36:21
 ******************************************************************/

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/iommu.h>
#include <linux/pm_wakeup.h>
#include <linux/ion.h>
#include <linux/genalloc.h>
#include <linux/version.h>
#include <linux/mm_iommu.h>
#include <linux/platform_drivers/mm_svm.h>
#include <platform_include/isp/linux/hipp_common.h>
#include <linux/delay.h>
#include <linux/types.h>
#include "ipp.h"
#include "memory.h"
#include "ipp_core.h"
#include "ipp_pcie.h"
#include "ipp_smc.h"

struct ipp_pcie_cfg hispcpe_pcie_cfg;

struct ipp_pcie_cfg *hipp_get_pcie_cfg(void)
{
	return &hispcpe_pcie_cfg;
}

void *hispcpe_pcie_reg_addr(unsigned long start)
{
	struct ipp_pcie_cfg *dev = &hispcpe_pcie_cfg;

	if ((dev->ipp_pci_reg == NULL) || (dev->pci_flag == 0)) {
		pr_err("%s: ipp pcie isn't enabled.%d\n", __func__, dev->pci_flag);
		return NULL;
	}

	return dev->ipp_pci_reg + start - IPP_PCIE_OUTBOUND_START;
}

int hispcpe_pcie_powerup(void)
{
	int ret;
	struct ipp_pcie_cfg *dev = NULL;

	dev = hipp_get_pcie_cfg();
	if (dev->pci_flag == 0)
		return 0;

	pr_err("[%s] pcie +\n", __func__);
	ret = atfhipp_vbus_pcie_pwrup(dev->pci_flag, dev->ipp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_ipp_pwrup ret = %d\n",
			   __func__, ret);
		return ret;
	}

	ret = mm_smmu_poweron(dev->tcu_dev);
	if (ret != 0) {
		pr_err("[%s] Failed: smmu tcu.%d\n", __func__, ret);
		(void)atfhipp_vbus_pcie_pwrdn(dev->pci_flag, dev->ipp_pci_addr);
	}

	ret = atfhipp_ipp_pcie_pwrup(dev->pci_flag, dev->ipp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_ipp_pwrup ret = %d\n",
			   __func__, ret);
		mm_smmu_poweroff(dev->tcu_dev);
		(void)atfhipp_vbus_pcie_pwrdn(dev->pci_flag, dev->ipp_pci_addr);
	}

	pr_err("[%s] pcie -\n", __func__);
	return ret;
}

int hispcpe_pcie_powerdn(void)
{
	int ret;
	struct ipp_pcie_cfg *dev;

	dev = hipp_get_pcie_cfg();
	if (dev->pci_flag == 0)
		return 0;

	pr_err("[%s] pcie +\n", __func__);
	ret = atfhipp_ipp_pcie_pwrdn(dev->pci_flag, dev->ipp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_ipp_pcie_pwrdn ret = %d\n",
			   __func__, ret);
		return -ENODEV;
	}

	ret = mm_smmu_poweroff(dev->tcu_dev);
	if (ret != 0)
		pr_err("[%s] Failed: smmu tcu regulator_disable.%d\n",
			   __func__, ret);

	ret = atfhipp_vbus_pcie_pwrdn(dev->pci_flag, dev->ipp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : atfhipp_vbus_pcie_pwrdn ret = %d\n",
			   __func__, ret);
		return -ENODEV;
	}

	pr_err("[%s] pcie -\n", __func__);
	return ret;
}

static int hispcpe_pcie_msi(struct pci_dev *pdev)
{
	const int irq_req = IPP_PCIE_IRQ_NUM;
	int irq_flag = PCI_IRQ_MSI | PCI_IRQ_AFFINITY;
	int ret = 0;

	pr_info("[%s] +\n", __func__);
	ret = pci_alloc_irq_vectors(pdev, 1, irq_req, irq_flag);
	if (ret != irq_req) {
		pr_err("[%s]: __pci_enable_msi_range failed, ret = %d\n", __func__, ret);
		return -EINVAL;
	}

	pr_info("[%s]: ipp msi is enabled, irq = %d, ret = %d\n", __func__, pdev->irq, ret);
	pr_info("[%s] -\n", __func__);

	return 0;
}

static int hispcpe_pcie_outbound(struct pci_dev *pdev)
{
	unsigned long bar0_addr;
	unsigned long bar0_size;
	struct ipp_pcie_cfg *dev = &hispcpe_pcie_cfg;

	pr_info("[%s] +\n", __func__);
	if (pci_enable_device(pdev)) {
		pr_err("[%s]: Cannot enable PCI device\n", __func__);
		return -EINVAL;
	}

	pci_set_master(pdev);
	/* rearrange pcie mem map to zone [0~2M] */
	/* rc1, bar0, bar_start = 0xE8000000 */
	pcie_set_mem_outbound(IPP_PCIE_RC_ID, pdev, IPP_PCIE_BAR_ID, IPP_PCIE_OUTBOUND_START);
	bar0_addr = (unsigned long)pci_resource_start(pdev, 0);
	bar0_size = (unsigned long)pci_resource_len(pdev, 0);
	pr_info("[%s]: bar0 addr is 0x%x, size is 0x%x\n", __func__, bar0_addr, bar0_size);

	if ((bar0_addr == 0) || (bar0_size == 0)) {
		pr_err("[%s]: Get pci resource failed, addr = %llx, size = %lu\n",
			   __func__, bar0_addr, bar0_size);
		return -EINVAL;
	}

	dev->ipp_pci_addr = bar0_addr;
	dev->ipp_pci_size = bar0_size;
	pr_info("[%s] -\n", __func__);

	return ISP_IPP_OK;
}

int hispcpe_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret = 0;

	pr_info("%s: bus %x, slot %x, vendor %x, device %x\n",
		__func__, pdev->bus->number, PCI_SLOT(pdev->devfn),
		pdev->vendor, pdev->device);

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret < 0) {
		pr_err("[%s] Failed : dma_set\n", __func__);
		return ret;
	}

	/* pcie : pci_request_mem_regions, bar start/size */
	ret = hispcpe_pcie_outbound(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_pcie_outbound\n", __func__);
		goto pcie_remove;
	}

	/* pcie : pci irqs */
	ret = hispcpe_pcie_msi(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_pcie_msi\n", __func__);
		goto pcie_remove;
	}

	/* attach to global pcie dev */
	hispcpe_pcie_cfg.pci_dev = pdev;
	return ISP_IPP_OK;

pcie_remove:
	return -ENODEV;
}

int hispcpe_pcie_remove(struct pci_dev *pdev)
{
	kfree(pdev);
	return ISP_IPP_OK;
}

static struct pci_device_id hispcpe_pcie_tbl[] = {
	{
vendor:
		IPP_PCIE_VENDER,
device :
		IPP_PCIE_DEVICE,
		subvendor :   0,
		subdevice :   0,
		class :       0,
			class_mask :  0,
			driver_data : 0,
		},
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hispcpe_pcie_tbl);

struct pci_driver hispcpe_pcie_driver = {
node:
	{},
name :       "pcie_ippcore"
	,
id_table :
	hispcpe_pcie_tbl,
probe :
	hispcpe_pcie_probe,
remove :
	hispcpe_pcie_remove,
suspend :
	NULL,
resume :
	NULL,
};

static int hispcpe_pcie_register(void)
{
	pci_register_driver(&hispcpe_pcie_driver);
	pcie_kport_enumerate(IPP_PCIE_RC_ID);
	return ISP_IPP_OK;
}

void hispcpe_pcie_unregister(void)
{
	struct ipp_pcie_cfg *dev = &hispcpe_pcie_cfg;

	if (dev->pci_flag == 0)
		pr_err("%s: ipp pcie isn't enabled, maybe under ISP debugging\n", __func__);
}

int hispcpe_attach_pcie(struct device *tcu_dev, unsigned int flag)
{
	struct ipp_pcie_cfg *dev = &hispcpe_pcie_cfg;

	dev->ipp_pci_ready = IPP_PCIE_NREADY;

	if (tcu_dev == NULL) {
		pr_err("[%s] Failed : tcu dev.NULL\n", __func__);
		return -EINVAL;
	}
	dev->tcu_dev = tcu_dev;
	dev->pci_flag = flag;
	dev->ipp_reg_base = IPP_PCIE_OUTBOUND_START;
	dev->ipp_pci_ready = IPP_PCIE_READY;

	return ISP_IPP_OK;
}

int hispcpe_pcie_init(struct device *tcu_dev, unsigned int pci_flag)
{
	int ret;
	struct ipp_pcie_cfg *dev = &hispcpe_pcie_cfg;

	if (pci_flag == 0) {
		pr_err("ipp pcie isn't enabled, maybe under ISP debugging\n");
		dev->pci_flag = 0;
		return 0;
	}

	ret = hispcpe_pcie_register();
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_pcie_register\n", __func__);
		goto clean_ipp_pcie;
	}

	ret = hispcpe_attach_pcie(tcu_dev, pci_flag);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_pcie_register\n", __func__);
		goto clean_ipp_pcie;
	}

	if (dev->ipp_pci_ready == IPP_PCIE_NREADY) {
		pr_err("[%s] ipp pcie isn't ready\n", __func__);
		goto clean_ipp_pcie;
	}

	pr_err("[%s] pci reg ioremap: from 0x%x to 0x%llx\n",
		__func__, dev->ipp_pci_addr, dev->ipp_pci_size);

	dev->ipp_pci_reg = ioremap(dev->ipp_pci_addr, dev->ipp_pci_size);
	if (dev->ipp_pci_reg == NULL) {
		loge("[%s] Can't remap register window\n", __func__);
		goto clean_ipp_pcie;
	}

	return 0;

clean_ipp_pcie:
	hispcpe_pcie_unregister();
	return -1;
}

void hispcpe_pcie_deinit(void)
{
	hispcpe_pcie_unregister();
}

