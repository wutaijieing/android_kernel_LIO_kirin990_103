

#ifndef __PCIE_IATU_H__
#define __PCIE_IATU_H__

#include "oal_types.h"
#include "pcie_linux.h"

int32_t oal_pcie_bar_init(oal_pcie_linux_res *pcie_res);
void oal_pcie_bar_exit(oal_pcie_linux_res *pcie_res);
void oal_pcie_iatu_reg_dump(oal_pcie_linux_res *pci_res);
void oal_pcie_iatu_outbound_dump_by_membar(const void *outbound_addr, uint32_t index);
#endif
