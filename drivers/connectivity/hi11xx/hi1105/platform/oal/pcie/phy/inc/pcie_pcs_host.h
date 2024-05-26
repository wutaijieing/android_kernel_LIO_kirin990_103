/*
 * 版权所有 (c) 华为技术有限公司 2012-2019
 * 功能说明   : pcie pcs host header file
 * 创建日期   : 2022年03月09日
 */

#ifndef __PCIE_PCS_HOST_H__
#define __PCIE_PCS_HOST_H__

#include "oal_types.h"
#include "pcie_host.h"
void oal_pcie_device_phy_config_single_lane(oal_pcie_res *pst_pci_res, uint32_t base_addr);
void oal_pcie_device_phy_disable_l1ss_rekey(oal_pcie_res *pst_pci_res, uint32_t base_addr);
void oal_pcie_phy_ini_config_init(void);
#endif

