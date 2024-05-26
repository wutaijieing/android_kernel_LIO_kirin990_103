

#ifndef __PCIE_PCS_DEYE_PATTERN_H__
#define __PCIE_PCS_DEYE_PATTERN_H__

#include "oal_types.h"
#include "pcie_pcs_regs.h"
int32_t pcie_pcs_deye_pattern_draw(pcs_reg_ops *reg_ops, uintptr_t base_addr, uint32_t lane, uint8_t *buff,
    int32_t size);
#endif

