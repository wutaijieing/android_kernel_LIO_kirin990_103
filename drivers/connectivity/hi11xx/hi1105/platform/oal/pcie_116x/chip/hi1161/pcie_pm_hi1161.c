/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: pcie pm
 * Author: platform
 * Create: 2021-03-09
 * History:
 */

#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_host.h"
#include "oal_types.h"


int32_t oal_pcie_device_phy_config_hi1161(oal_pcie_linux_res *pcie_res)
{
    oal_reference(&pcie_res->ep_res);
    return OAL_SUCC;
}

int32_t pcie_pm_chip_init_hi1161(oal_pcie_linux_res *pcie_res, int32_t device_id)
{
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    ep_res->chip_info.cb.pm_cb.pcie_device_aspm_init = NULL;
    ep_res->chip_info.cb.pm_cb.pcie_device_auxclk_init = NULL;
    ep_res->chip_info.cb.pm_cb.pcie_device_aspm_ctrl = NULL;
    ep_res->chip_info.cb.pm_cb.pcie_device_phy_config = oal_pcie_device_phy_config_hi1161;
    return OAL_SUCC;
}
