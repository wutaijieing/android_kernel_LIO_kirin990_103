

#ifndef __PCIE_CHIP_BISHENG_H__
#define __PCIE_CHIP_BISHENG_H__

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_host.h"

int32_t oal_pcie_chip_info_init_bisheng(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t oal_pcie_board_init_bisheng(linkup_fixup_ops *ops);
#endif
#endif
