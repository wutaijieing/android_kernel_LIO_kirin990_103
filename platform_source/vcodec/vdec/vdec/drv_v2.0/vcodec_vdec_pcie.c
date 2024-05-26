#include <linux/errno.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <platform_include/basicplatform/linux/pcie-kport-api.h>

#include "vcodec_vdec.h"
#include "vcodec_vdec_plat.h"
#include "vfmw_intf.h"
#include "vcodec_vdec_regulator.h"

#define VDEC_PCIE_VENDOR        0x19E5
#define VDEC_PCIE_DEVICE        0x369C
#define VDEC_PCIE_SUBVENDER     0
#define VDEC_PCIE_SUBDEVICE     0
#define VDEC_PCIE_CLASS         0
#define VDEC_PCIE_CLASS_MASK    0
#define VDEC_PCIE_DRIVER_DATA   0
#define VDEC_PCIE_IRQ_OFFSET    1

#define VDEC_PCIE_RC_ID         0x0
#define VDEC_PCIE_BAR_ID        0x0

typedef struct {
	dev_t dev_num;
	struct cdev cdev;
	struct device *device;
	struct class  *device_class;
	int32_t pcie_irq_handled;
} pcie_drv_entry;

pcie_drv_entry g_vdec_pcie_entry;

static struct pci_device_id vdec_pcie_devid[] = {
	{
		vendor        :     VDEC_PCIE_VENDOR,
		device        :     VDEC_PCIE_DEVICE,
		subvendor     :     VDEC_PCIE_SUBVENDER,
		subdevice     :     VDEC_PCIE_SUBDEVICE,
		class         :     VDEC_PCIE_CLASS,
		class_mask    :     VDEC_PCIE_CLASS_MASK,
		driver_data   :     VDEC_PCIE_DRIVER_DATA,
	},

	{ 0, }
};

MODULE_DEVICE_TABLE(pci, vdec_pcie_devid);

static int vdec_pcie_registers_map(struct pci_dev *pdev)
{
	int32_t i;
	phys_addr_t pcie_ep_reg_addr;
	unsigned long pcie_ep_reg_size;
	uint32_t pcie_mmap_host_start_reg;
	vdec_plat *plt = vdec_plat_get_entry();

	if (pci_enable_device(pdev)) {
		dprint(PRN_ERROR, "can't enable pcie device\n");
		return -1;
	}

	pci_set_master(pdev);
	pcie_mmap_host_start_reg = plt->dts_info.pcie_mmap_host_start_reg;
	pcie_ep_reg_addr = pcie_set_mem_outbound(VDEC_PCIE_RC_ID, pdev, VDEC_PCIE_BAR_ID, pcie_mmap_host_start_reg);
	pcie_ep_reg_size = pci_resource_len(pdev, 0);

	if (pcie_ep_reg_addr == 0 || pcie_ep_reg_size == 0) {
		dprint(PRN_ERROR, "pcie_set_mem_outbound failed\n");
		goto err;
	}
	dprint(PRN_ALWS, "vdec pcie reg map : pcie ep_reg_addr : %#x, pcie ep_reg_size %#x\n", pcie_ep_reg_addr, pcie_ep_reg_size);

	/* convert dec host reg addr to pcie ep reg addr */
	for (i = 0; i < MAX_INNER_MODULE; i++)
		plt->dts_info.module_reg[i].reg_phy_addr = (plt->dts_info.module_reg[i].reg_phy_addr & 0x00FFFFFF) | pcie_ep_reg_addr;

	return 0;
err:
	pci_disable_device(pdev);
	return -1;
}

static int vdec_pcie_irq_request(struct pci_dev *pdev)
{
	int nvec;
	int ret;
	int irq_req = 4;
	int irq_flag = PCI_IRQ_MSI;
	int vdec_iqr;

	nvec = pci_alloc_irq_vectors(pdev, 1, irq_req, irq_flag);
	if (nvec != irq_req) {
		dprint(PRN_ERROR, "pci enable msi range failed %d\n", nvec);
		return -1;
	}

	vdec_iqr = pdev->irq + VDEC_PCIE_IRQ_OFFSET;
	ret = request_irq(vdec_iqr, vfmw_isr, IRQF_SHARED, "vdec pcie msi", (void *)&g_vdec_pcie_entry.pcie_irq_handled);
	if (ret) {
		pci_free_irq_vectors(pdev);
		dprint(PRN_ERROR, "request irq failed");
		return -1;
	}

	return 0;
}

static struct file_operations vdec_pcie_fops = {
	.owner       = THIS_MODULE,
	.open        = NULL,
	.release     = NULL,
	.unlocked_ioctl = NULL,
	.compat_ioctl   = NULL,
};

static int vdec_pcie_setup_cdev(pcie_drv_entry *entry, const struct file_operations *fops)
{
	int err;

	cdev_init(&entry->cdev, &vdec_pcie_fops);
	entry->cdev.owner = THIS_MODULE;
	err = alloc_chrdev_region(&entry->dev_num, 0, 1, "vdec_pcie");
	if (err) {
		dprint(PRN_ERROR, "alloc chrdev region failed\n");
		return -1;
	}

	err = cdev_add(&entry->cdev, entry->dev_num, 1);
	if (err) {
		dprint(PRN_ERROR, "cdev add failed\n");
		goto unregister_region;
	}

	entry->device_class = class_create(THIS_MODULE, "vdec_pcie");
	if (IS_ERR(entry->device_class)) {
		dprint(PRN_ERROR, "fail to creat vdec pcie class\n");
		goto dev_del;
	}

	entry->device = device_create(entry->device_class, NULL, entry->dev_num, "%s", "vdec_pcie");
	if (IS_ERR(entry->device)) {
		dprint(PRN_ERROR, "fail to creat vdec pcie vdec_pcie\n");
		goto cls_destroy;
	}

	return 0;

cls_destroy:
	class_destroy(entry->device_class);
	entry->device_class = NULL;
dev_del:
	cdev_del(&entry->cdev);
unregister_region:
	unregister_chrdev_region(entry->dev_num, 1);

	return -1;
}

static int vdec_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int err = 0;

	(void)memset_s(&g_vdec_pcie_entry, sizeof(pcie_drv_entry), 0, sizeof(pcie_drv_entry));

	err = vdec_pcie_registers_map(pdev);
	if (err)
		return -ENODEV;

	err = vdec_pcie_irq_request(pdev);
	if (err)
		goto pcie_irq_request_error;

	err = vdec_pcie_setup_cdev(&g_vdec_pcie_entry, &vdec_pcie_fops);
	if (err)
		goto pcie_setup_cdev_error;

	dprint(PRN_ALWS, "success\n");
	return 0;

pcie_setup_cdev_error:
	free_irq(pdev->irq + VDEC_PCIE_IRQ_OFFSET, (void *)&g_vdec_pcie_entry.pcie_irq_handled);
	pci_free_irq_vectors(pdev);
pcie_irq_request_error:
	pci_disable_device(pdev);
	return -ENODEV;
}

static void vdec_pcie_remove(struct pci_dev *pdev)
{
	pcie_drv_entry *entry = &g_vdec_pcie_entry;
	if (IS_ERR_OR_NULL(entry->device_class))
		return;

	device_destroy(entry->device_class, entry->dev_num);
	class_destroy(entry->device_class);

	cdev_del(&entry->cdev);
	unregister_chrdev_region(entry->dev_num, 1);
}

int32_t vdec_plat_regulator_enable(void)
{
	vdec_plat *plt = vdec_plat_get_entry();

	vdec_check_ret(plt->plt_init, VCODEC_FAILURE);
	vdec_mutex_lock(&plt->vdec_plat_mutex);
	plt->power_flag = 1;
	vdec_mutex_unlock(&plt->vdec_plat_mutex);

	dprint(PRN_CTRL, "success\n");
	return VCODEC_SUCCESS;
}

void vdec_plat_regulator_disable(void)
{
	vdec_plat *plt = vdec_plat_get_entry();

	vdec_mutex_lock(&plt->vdec_plat_mutex);
	plt->power_flag = 0;
	vdec_mutex_unlock(&plt->vdec_plat_mutex);

	dprint(PRN_CTRL, "success\n");
}

static struct pci_driver vdec_pcie_driver = {
	node:      {},
	name:      "vdec_pcie",
	id_table:  vdec_pcie_devid,
	probe:     vdec_pcie_probe,
	remove:    vdec_pcie_remove,
	suspend:   NULL,
	resume:    NULL,
};

static int __init vdec_pcie_device_init(void)
{
	if (pci_register_driver(&vdec_pcie_driver))
		return -1;

	return 0;
}

static void __exit vdec_pcie_device_exit(void)
{
	pci_unregister_driver(&vdec_pcie_driver);
}

module_init(vdec_pcie_device_init);
module_exit(vdec_pcie_device_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VDEC PCIE DRIVER");
MODULE_VERSION("1.0");

