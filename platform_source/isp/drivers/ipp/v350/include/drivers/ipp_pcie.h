/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    ipp_core.h
 * Description:
 *
 * Date         2020-04-16 17:36:21
 ******************************************************************/

#ifndef _IPP_PCIE_H_INCLUDED__
#define _IPP_PCIE_H_INCLUDED__

#if defined(CONFIG_IPP_DEBUG) && defined(IPP_KERNEL_USE_PCIE_VERIFY)
#define IPP_PCIE_VENDER            0x19e5
#define IPP_PCIE_DEVICE            0x369E
#define IPP_PCIE_RC_ID             0
#define IPP_PCIE_BAR_ID            0

#define IPP_PCIE_OUTBOUND_START    0xE8000000
#define IPP_PCIE_IRQ_NUM           0x4
#define SIZE_16M                   0x01000000

#define IPP_PCIE_READY             1
#define IPP_PCIE_NREADY            0

struct ipp_pcie_cfg {
	struct pci_dev *pci_dev;
	struct device *tcu_dev;
	unsigned int pci_flag;
	void __iomem *ipp_pci_reg;
	unsigned int ipp_pci_addr;
	unsigned int ipp_pci_size;
	unsigned int ipp_reg_base;
	unsigned int ipp_pci_ready;
};

struct ipp_pcie_cfg *hipp_get_pcie_cfg(void);
void *hispcpe_pcie_reg_addr(unsigned long start);
int hispcpe_pcie_init(struct device *tcu_dev, unsigned int pci_flag);
void hispcpe_pcie_deinit(void);

extern int pcie_kport_enumerate(u32 rc_idx);
extern u64 pcie_set_mem_outbound(u32 rc_id, struct pci_dev *dev, int bar, u64 target);
#endif
#endif

/* **************************************end*********************************** */
