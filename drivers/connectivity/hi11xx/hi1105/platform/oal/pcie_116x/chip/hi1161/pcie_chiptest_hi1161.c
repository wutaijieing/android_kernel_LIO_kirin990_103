/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: pcie chip test
 * Author: platform
 * Create: 2021-03-09
 * History:
 */

#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "oal_thread.h"
#include "oam_ext_if.h"
#include "pcie_host.h"
#include "pcie_reg.h"
#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"
#include "plat_firmware.h"
#include "plat_pm_wlan.h"
#include "board.h"
#include "securec.h"

int32_t oal_pcie_ip_factory_test_hi1161(hcc_bus *pst_bus, int32_t test_count)
{
    oal_print_hi11xx_log(HI11XX_LOG_ERR, "not implement!!");
    return -OAL_EFAIL;
}

int32_t oal_pcie_chiptest_init_hi1161(oal_pcie_linux_res *pst_pci_res, int32_t device_id)
{
    pst_pci_res->ep_res.chip_info.cb.pcie_ip_factory_test = oal_pcie_ip_factory_test_hi1161;
    return OAL_SUCC;
}
