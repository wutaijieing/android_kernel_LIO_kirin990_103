

#ifndef __PCIE_PM_HISI_H
#define __PCIE_PM_HISI_H

#include "pcie_linux.h"

int32_t oal_pcie_pm_chip_init(oal_pcie_linux_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_hi1161(oal_pcie_linux_res *pst_pci_res, int32_t device_id);
#endif
