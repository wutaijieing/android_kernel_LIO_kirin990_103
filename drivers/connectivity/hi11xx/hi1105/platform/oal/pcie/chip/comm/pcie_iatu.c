

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_host.h"

int32_t oal_pcie_get_bar_region_info(oal_pcie_res *pst_pci_res,
                                     oal_pcie_region **region_base, uint32_t *region_num)
{
    if (pst_pci_res->chip_info.cb.pcie_get_bar_region_info == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_get_bar_region_info is null");
        return -OAL_ENODEV;
    }
    return pst_pci_res->chip_info.cb.pcie_get_bar_region_info(pst_pci_res, region_base, region_num);
}

int32_t oal_pcie_set_outbound_membar(oal_pcie_res *pst_pci_res, oal_pcie_iatu_bar *pst_iatu_bar)
{
    if (pst_pci_res->chip_info.cb.pcie_set_outbound_membar == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_set_outbound_membar is null");
        return -OAL_ENODEV;
    }
    return pst_pci_res->chip_info.cb.pcie_set_outbound_membar(pst_pci_res, pst_iatu_bar);
}

#endif

