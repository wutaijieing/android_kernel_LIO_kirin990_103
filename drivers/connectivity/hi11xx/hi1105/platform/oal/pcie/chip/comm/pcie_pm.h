

#ifndef __PCIE_PM_HISI_H
#define __PCIE_PM_HISI_H

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_host.h"
extern uint32_t g_pcie_pcs_tracer_enable;

static inline uint32_t pcs_trace_linkup_flag(void)
{
    return (uint32_t)((uint32_t)(g_pcie_pcs_tracer_enable >> 4) & 0xf);
}

static inline uint32_t pcs_trace_speedchange_flag(void)
{
    return (uint32_t)(g_pcie_pcs_tracer_enable & 0xf);
}

int32_t oal_pcie_pm_chip_init(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_hi1103(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_hi1105(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_shenkuo(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_bisheng(oal_pcie_res *pst_pci_res, int32_t device_id);
int32_t pcie_pm_chip_init_hi1161(oal_pcie_res *pst_pci_res, int32_t device_id);
#endif

#endif
