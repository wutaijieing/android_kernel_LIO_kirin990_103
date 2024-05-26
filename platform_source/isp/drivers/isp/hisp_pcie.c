/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    hisp_pcie.c
 * Description:
 *
 * Version      1.0
 ******************************************************************/

#include "hisp_internel.h"
#include "hisp_pcie.h"

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
#include <linux/platform_drivers/mm_ion.h>
#include <linux/genalloc.h>
#include <linux/mm_iommu.h>
#include <linux/version.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>

#include <linux/delay.h>
#include <linux/types.h>

#include <soc_ipcns_interface.h>

#ifdef DEBUG_HISP
struct pci_dev *hisp_pcie_dev;
unsigned long g_isp_bar_addr;
unsigned long g_isp_bar_size;

static int hisp_mbx_start_cfg(void __iomem *base,
		unsigned int srcmbox, unsigned int dstmbox,
		unsigned int srccore, unsigned int dstcore)
{
	void __iomem *addr = 0;
	u_mbx_iclr   mbx_iclr;
	u_mbx_source mbx_source;
	u_mbx_dset   mbx_dset;
	u_mbx_imst   mbx_imask;
	int timeout = 2000;

	/* IPC_LOCK */
	addr = isp_ipc_ipc_lock_addr(base);
	__raw_writel(IPC_UNLOCK_KEY, (volatile unsigned int *)(uintptr_t)addr);

	/* mbx_iclr */
	addr = isp_ipc_mbx_iclr_addr(base, dstmbox);
	mbx_iclr.reg.int_clear = 1 << srccore;
	__raw_writel(mbx_iclr.value, (volatile unsigned int *)(uintptr_t)addr);
	/* mbx_source */
	addr = isp_ipc_mbx_source_addr(base, srcmbox);
	mbx_source.value = __raw_readl((const volatile unsigned int *)(uintptr_t)addr);

	if (mbx_source.reg.source != 0 && mbx_source.reg.source != 1 << srccore) {
		pr_err("Failed : r.mbx_source.reg.source.%d", mbx_source.reg.source);
		return -1;
	}

	do {
		/* mbx_source */
		addr = isp_ipc_mbx_source_addr(base, srcmbox);
		mbx_source.reg.source = 1 << srccore;
		__raw_writel(mbx_source.value, (volatile unsigned int *)(uintptr_t)addr);
		mbx_source.value = __raw_readl((const volatile unsigned int *)(uintptr_t)addr);
	} while (!mbx_source.reg.source && timeout-- > 0);

	if (timeout < 0) {
		pr_err("Failed : timeout.%d, mbx_source.reg.source.0x%x",
				timeout, mbx_source.reg.source);
		return -1;
	}

	/* mbx_dset */
	addr = isp_ipc_mbx_dset_addr(base, srcmbox);
	mbx_dset.reg.dset = 1 << dstcore;
	__raw_writel(mbx_dset.value, (volatile unsigned int *)(uintptr_t)addr);

	/* IRQ MASK ENABLE */
	addr = isp_ipc_mbx_imask_addr(base, srcmbox);
	mbx_imask.reg.int_mask_status = ISPCPU_INT_UNMASK;
	__raw_writel(mbx_imask.value, (volatile unsigned int *)(uintptr_t)addr);

	return 0;
}

int hisp_pcie_send_ipc2isp(rproc_msg_t *msg, rproc_msg_len_t len)
{
	void __iomem *addr;
	void __iomem *base;
	u_mbx_mode mbx_mode;
	u_mbx_iclr mbx_iclr;
	u_mbx_send mbx_send;
	int cnt;
	struct isp_pcie_cfg *cfg;

	cfg = hisp_get_pcie_cfg();
	base = cfg->isp_pci_reg + ISP_IPC_OFFSET_PCIE;
	hisp_mbx_start_cfg(base, ISPCPU_BASE_MBOXID_ISPCPU0, ISPCPU_BASE_MBOXID_ACPU0,
			ISPCPU_BASE_CPUID_ACPU, ISPCPU_BASE_CPUID_ISPCPU);

	addr = isp_ipc_mbx_data0_addr(base, ISPCPU_BASE_MBOXID_ISPCPU0);
	__raw_writel(msg[0], addr);
	pr_debug("msg[%d]: 0x%x to 0x%llx", 0, msg[0], addr);

	cnt = 2000;

	do {
		/* mbx_mode */
		addr = isp_ipc_mbx_mode_addr(base, ISPCPU_BASE_MBOXID_ISPCPU0);
		mbx_mode.value = __raw_readl((const volatile unsigned int *)(uintptr_t)addr);

		if (mbx_mode.reg.state_status == 0x2)
			break;

		if (mbx_mode.reg.state_status == 0x8) {
			/* mbx_iclr */
			addr = isp_ipc_mbx_iclr_addr(base, ISPCPU_BASE_MBOXID_ISPCPU0);
			mbx_iclr.reg.int_clear = (1 << ISPCPU_BASE_CPUID_ACPU);
			__raw_writel(mbx_iclr.value, (volatile unsigned int *)(uintptr_t)addr);
		}
	} while (cnt -- > 0);

	if (cnt <= 0)
		pr_err("Failed : timeout.%d, mbx_mode.reg.state_status.0x%x",
			cnt, mbx_mode.reg.state_status);

	/* mbx_send */
	addr = isp_ipc_mbx_send_addr(base, ISPCPU_BASE_MBOXID_ISPCPU0);
	mbx_send.reg.send = (1 << ISPCPU_BASE_CPUID_ACPU);
	__raw_writel(mbx_send.value, (volatile unsigned int *)(uintptr_t)addr);

	pr_info("send over\n");

	return 0;
}

int hisp_pcie_outbound(struct pci_dev *dev)
{
	int ret;

	if (pci_enable_device(dev)) {
		pr_err("[%s]: Cannot enable PCI device\n", __func__);
		return -EINVAL;
	}

	pci_set_master(dev);

	/* rearrange pcie mem map to zone [0~2M] */
	/* rc1, bar0, bar_start = 0xE9000000 */
	g_isp_bar_addr = pcie_set_mem_outbound(ISP_PCIE_RC_ID, dev, ISP_PCIE_BAR_ID, ISP_PCIE_OUTBOUND_START);
	g_isp_bar_size = pci_resource_len(dev, 0);
	pr_info("[%s]: bar0 addr is 0x%x, size is 0x%x\n", __func__, g_isp_bar_addr, g_isp_bar_size);

	if ((g_isp_bar_addr == 0) || (g_isp_bar_size == 0)) {
		pr_err("[%s]: Get pci resource failed, addr = %llx, size = %lu\n",
			__func__, g_isp_bar_addr, g_isp_bar_size);
		return -EINVAL;
	}

	return 0;
}

void hisp_pcie_remove(struct pci_dev *pdev)
{
	return;
}

int hisp_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret = 0;

	pr_info("%s: bus %x, slot %x, vendor %x, device %x\n",
		 __func__, pdev->bus->number, PCI_SLOT(pdev->devfn),
		 pdev->vendor, pdev->device);
	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret < 0) {
		pr_err("[%s] Failed : dma_set\n", __func__);
		return ret;
	}

	/* pcie : pci_request_mem_regions, bar start/size */
	ret = hisp_pcie_outbound(pdev);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_pcie_outbound\n", __func__);
		goto pcie_remove;
	}

	/* attach to global pcie dev*/
	hisp_pcie_dev = pdev;

	return 0;

pcie_remove:
	hisp_pcie_remove(pdev);
	return -ENODEV;
}

static struct pci_device_id hisp_pcie_tbl[] = {
	{
		vendor:      ISP_PCIE_VENDER,
		device:      ISP_PCIE_DEVICE,
		subvendor:   0,
		subdevice:   0,
		class:       0,
		class_mask:  0,
		driver_data: 0,
	},
	{0,}
};
MODULE_DEVICE_TABLE(pci, hisp_pcie_tbl);

struct pci_driver hisp_pcie_driver = {
	node:        {},
	name :       "pcie_isp",
	id_table :   hisp_pcie_tbl,
	probe :      hisp_pcie_probe,
	remove :     hisp_pcie_remove,
	suspend :    NULL,
	resume :     NULL,
};

int hisp_get_pcie_outbound(struct isp_pcie_cfg *cfg)
{
	if (cfg == NULL) {
		pr_err("[%s] Failed : cfg.%pK\n", __func__, cfg);
		return ISP_PCIE_NREADY;
	}

	if (hisp_pcie_dev == NULL) {
		pr_err("[%s] maybe vendorID or deviceID is wrong\n", __func__);
		cfg->isp_pci_ready = ISP_PCIE_NREADY;
		return ISP_PCIE_NREADY;
	}

	cfg->isp_pci_dev = hisp_pcie_dev;
	cfg->isp_pci_irq = cfg->isp_pci_dev->irq;
	cfg->isp_pci_addr = g_isp_bar_addr;
	cfg->isp_pci_size = g_isp_bar_size;
	cfg->isp_pci_ready = ISP_PCIE_READY;

	return ISP_PCIE_READY;
}

int hisp_pcie_register(void)
{
#ifdef CONFIG_PCIE_KPORT_EP_FPGA_VERIFY
	pci_register_driver(&hisp_pcie_driver);

	return pcie_kport_enumerate(ISP_PCIE_RC_ID);
#else
	return ISP_PCIE_NREADY;
#endif
}

int hisp_pcie_unregister(void)
{
	return 0;
}
#endif
