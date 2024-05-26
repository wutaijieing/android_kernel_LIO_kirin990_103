

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE

/* 通过APB,PCIE MEMPROT,SSI等接口访问PCIE PHY寄存器 */
#include "pcie_pcs_regs.h"
#include "ssi_common.h"

static void pcie_pcs_set_bits32_ssi(uintptr_t reg, uint32_t pos, uint32_t bits, uint32_t val)
{
    uint32_t reg_t;
    if (bits == 32) { // 32bits
        ssi_write_value32_test((uint32_t)reg, val);
    } else {
        reg_t = ssi_read_value32_test((uint32_t)reg);
        reg_t &= ~(((1UL << bits) - 1) << pos); // clear mask vlaue
        reg_t |= (val & ((1UL << bits) - 1)) << pos;
        ssi_write_value32_test((uint32_t)reg, reg_t);
    }
}

static uint32_t pcie_pcs_get_bits32_ssi(uintptr_t reg, uint32_t pos, uint32_t bits)
{
    if (bits == 32) { // 32bits
        return ssi_read_value32_test((uint32_t)reg);
    } else {
        return (ssi_read_value32_test((uint32_t)reg) >> pos) & (((uint32_t)1 << bits) - 1);
    }
}

/* 通过rc mem口访问 pcs tracer registers */
static void pcie_pcs_set_bits32_memport(uintptr_t reg, uint32_t pos, uint32_t bits, uint32_t val)
{
    uint32_t reg_t;
    if (bits == 32) { // 32bits
        oal_writel(val, reg);
    } else {
        reg_t = oal_readl(reg);
        reg_t &= ~(((1UL << bits) - 1) << pos); // clear mask vlaue
        reg_t |= (val & ((1UL << bits) - 1)) << pos;
        oal_writel(reg_t, reg);
    }
}

static uint32_t pcie_pcs_get_bits32_memport(uintptr_t reg, uint32_t pos, uint32_t bits)
{
    if (bits == 32) { // 32bits
        return oal_readl(reg);
    } else {
        return (oal_readl(reg) >> pos) & (((uint32_t)1 << bits) - 1);
    }
}

static void pcie_pcs_set_bits32_default(uintptr_t reg, uint32_t pos, uint32_t bits, uint32_t val)
{
}

static uint32_t pcie_pcs_get_bits32_default(uintptr_t reg, uint32_t pos, uint32_t bits)
{
    return 0;
}


int32_t pcie_pcs_ref_get(uint32_t dump_type)
{
    if (dump_type == PCIE_PCS_DUMP_BY_SSI) {
        /* lock ssi */
        if (ssi_try_lock()) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "ssi try lock failed, bypass pcs tracer!");
            return -OAL_EFAIL;
        }

        if (ssi_check_device_isalive() < 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "ssi check device is not alive!");
            return -OAL_EFAIL;
        }
    }

    return OAL_SUCC;
}

void pcie_pcs_ref_put(uint32_t dump_type)
{
    if (dump_type == PCIE_PCS_DUMP_BY_SSI) {
        ssi_unlock();
    }
}

/*  leacy function, remove later */
void pcie_pcs_get_bitops(pcs_set_bits32 *set_bits32, pcs_get_bits32 *get_bits32, uint32_t dump_type)
{
    if (dump_type == PCIE_PCS_DUMP_BY_SSI) {
        *set_bits32 = pcie_pcs_set_bits32_ssi;
        *get_bits32 = pcie_pcs_get_bits32_ssi;
    } else if ((dump_type == PCIE_PCS_DUMP_BY_MEMPORT) || (dump_type == PCIE_PCS_DUMP_BY_SOC)) {
        *set_bits32 = pcie_pcs_set_bits32_memport;
        *get_bits32 = pcie_pcs_get_bits32_memport;
    } else {
        *set_bits32 = pcie_pcs_set_bits32_default;
        *get_bits32 = pcie_pcs_get_bits32_default;
    }
}

/* reg_ops must set a func ops. can't be NULL ops */
int32_t pcie_pcs_set_regs_ops(pcs_reg_ops *reg_ops, uint32_t dump_type)
{
    if (dump_type == PCIE_PCS_DUMP_BY_SSI) {
        reg_ops->set_bits32 = pcie_pcs_set_bits32_ssi;
        reg_ops->get_bits32 = pcie_pcs_get_bits32_ssi;
    } else if ((dump_type == PCIE_PCS_DUMP_BY_MEMPORT) || (dump_type == PCIE_PCS_DUMP_BY_SOC)) {
        reg_ops->set_bits32 = pcie_pcs_set_bits32_memport;
        reg_ops->get_bits32 = pcie_pcs_get_bits32_memport;
    } else {
        reg_ops->set_bits32 = pcie_pcs_set_bits32_default;
        reg_ops->get_bits32 = pcie_pcs_get_bits32_default;
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}
#endif