

#ifndef _PCIE_PCS_REGS_H_
#define _PCIE_PCS_REGS_H_
#include "oal_util.h"

#define PCIE_PCS_DUMP_BY_SSI 0
#define PCIE_PCS_DUMP_BY_MEMPORT 1
#define PCIE_PCS_DUMP_BY_SOC 2

#define pcie_pcs_get_bits_from_value(value, pos, bits) \
    (((uint32_t)value >> (uint32_t)(pos)) & (((uint32_t)1 << (bits)) - 1))

typedef void (*pcs_set_bits32)(uintptr_t reg, uint32_t pos, uint32_t bits, uint32_t val);
typedef uint32_t (*pcs_get_bits32)(uintptr_t reg, uint32_t pos, uint32_t bits);

typedef struct _pcs_reg_ops_ {
    pcs_set_bits32 set_bits32;
    pcs_get_bits32 get_bits32;
} pcs_reg_ops;

void pcie_pcs_ref_put(uint32_t dump_type);
int32_t pcie_pcs_ref_get(uint32_t dump_type);
void pcie_pcs_get_bitops(pcs_set_bits32 *set_bits32, pcs_get_bits32 *get_bits32, uint32_t dump_type);
int32_t pcie_pcs_set_regs_ops(pcs_reg_ops *reg_ops, uint32_t dump_type);

static inline void pcie_pcs_set_bits32(pcs_reg_ops *reg_ops, uintptr_t reg, uint32_t pos, uint32_t bits, uint32_t val)
{
    reg_ops->set_bits32(reg, pos, bits, val);
}

static inline uint32_t pcie_pcs_get_bits32(pcs_reg_ops *reg_ops, uintptr_t reg, uint32_t pos, uint32_t bits)
{
    return reg_ops->get_bits32(reg, pos, bits);
}

static inline void pcie_pcs_clr_bit(pcs_reg_ops *reg_ops, uintptr_t reg, uint32_t pos)
{
    pcie_pcs_set_bits32(reg_ops, reg, pos, 1, 0);
}

static inline void pcie_pcs_set_bit(pcs_reg_ops *reg_ops, uintptr_t reg, uint32_t pos)
{
    pcie_pcs_set_bits32(reg_ops, reg, pos, 1, 1);
}

#endif
