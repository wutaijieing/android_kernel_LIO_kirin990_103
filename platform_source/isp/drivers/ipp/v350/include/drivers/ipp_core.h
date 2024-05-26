/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    ipp_core.h
 * Description:
 *
 * Date         2020-04-16 17:36:21
 ******************************************************************/
#ifndef _IPP_CORE_H_INCLUDED__
#define _IPP_CORE_H_INCLUDED__

enum HISP_CPE_IRQ_TYPE {
	CPE_IRQ_0 = 0, // ACPU
	MAX_HISP_CPE_IRQ
};

struct hispcpe_s {
	struct miscdevice miscdev;
	struct platform_device *pdev;
	int initialized;
	struct wakeup_source *ipp_wakelock;
	struct mutex ipp_wakelock_mutex;

	struct mutex open_lock;
	unsigned int open_refs;

	struct mutex dev_lock;
	int refs_power_up;
	unsigned int mapbuf_ready;

	unsigned int irq_num;
	unsigned int reg_num;
	int irq[MAX_HISP_CPE_IRQ];
	struct resource *r[MAX_HISP_CPE_REG];
	void __iomem *reg[MAX_HISP_CPE_REG];

	unsigned int sid;
	unsigned int ssid;
#ifdef IPP_KERNEL_USE_PCIE_VERIFY
	unsigned int pci_flag;
#endif
};

struct ipp_bypass_cfg_s {
	unsigned int is_bypass_ipp; // 1:bypass ipp test; 0:not bypass
};

#define HISP_IPP_PWRUP          _IOWR('C', 0x1001, int)
#define HISP_IPP_PWRDN          _IOWR('C', 0x1002, int)
#define HISP_IPP_CFG_CHECK      _IOWR('C', 0x1003, int)
#define HISP_IPP_PATH_REQ       _IOWR('C', 0x2002, int)
#define HISP_IPP_MAP_BUF        _IOWR('C', 0x200C, int)
#define HISP_IPP_UNMAP_BUF      _IOWR('C', 0x200D, int)
#define HISP_IPP_BYPASS_CHECK	_IOWR('C', 0x200E, int)
#ifdef CONFIG_IPP_DEBUG
#define HISP_IPP_GF_REQ         _IOWR('C', 0x2001, int)
#define HISP_IPP_ORB_ENH_REQ    _IOWR('C', 0x2004, int)
#define HISP_IPP_ARFEATURE_REQ  _IOWR('C', 0x2005, int)
#define HISP_REORDER_REQ        _IOWR('C', 0x2006, int)
#define HISP_COMPARE_REQ        _IOWR('C', 0x2007, int)
#define HISP_IPP_MC_REQ       	_IOWR('C', 0x2008, int)
#define HISP_IPP_HIOF_REQ       _IOWR('C', 0x2003, int)
#define HISP_CPE_MAP_IOMMU      _IOWR('C', 0x200A, int)
#define HISP_CPE_UNMAP_IOMMU    _IOWR('C', 0x200B, int)
#endif

int hipp_adapter_probe(struct platform_device *pdev);
void hipp_adapter_remove(void);
int hipp_adapter_register_irq(int irq);
void __iomem *hipp_get_regaddr(unsigned int type);
int get_hipp_smmu_info(int *sid, int *ssid);

#endif
/* **************************************end*********************************** */
