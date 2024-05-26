

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "oal_hcc_host_if.h"


int32_t oal_pcie_ip_factory_test_bisheng(hcc_bus *pst_bus, int32_t test_count)
{
    oal_print_hi11xx_log(HI11XX_LOG_ERR, "not implement!!");
    return -OAL_EFAIL;
}

int32_t oal_pcie_chiptest_init_bisheng(oal_pcie_res *pst_pci_res, int32_t device_id)
{
    pst_pci_res->chip_info.cb.pcie_ip_factory_test = oal_pcie_ip_factory_test_bisheng;
    return OAL_SUCC;
}
#endif
