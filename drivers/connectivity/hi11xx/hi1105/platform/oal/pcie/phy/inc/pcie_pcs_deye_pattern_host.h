

#ifndef __PCIE_PCS_DEYE_PATTERN_HOST_H__
#define __PCIE_PCS_DEYE_PATTERN_HOST_H__

#include "oal_types.h"
#include "pcie_pcs_regs.h"

#define PCIE_PCS_DEYE_PARTTERN_SIZE 8192

/* 开始PHY电子眼图测试 */
int32_t pcie_pcs_deye_pattern_host_test(uintptr_t base_addr, uint32_t lane, uint32_t dump_type);
#endif