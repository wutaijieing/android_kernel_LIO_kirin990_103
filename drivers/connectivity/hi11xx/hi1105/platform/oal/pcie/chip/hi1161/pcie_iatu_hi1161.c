/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: pcie iatu
 * Author: platform
 * Create: 2021-03-09
 * History:
 */

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_host.h"
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
#include "pcie_linux.h"
#include "pcie_reg.h"
#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"
#include "plat_firmware.h"
#include "plat_pm_wlan.h"
#include "board.h"
#include "securec.h"
#include "plat_pm.h"

#include "chip/hi1161/pcie_soc_hi1161.h"

OAL_STATIC oal_pcie_region g_hi1161_pcie_regions[] = {
    {
        .pci_start = 0x00000000,
        .pci_end   = 0x000C7FFF,
        .cpu_start = 0x00000000,
        .cpu_end   = 0x000C7FFF, /* 800KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_WCPU_ITCM0"
    },
#if (defined _PRE_WLAN_CHIP_ASIC)
    {
        .pci_start = 0x00800000,
        .pci_end   = 0x008C7FFF,
        .cpu_start = 0x00800000,
        .cpu_end   = 0x008C7FFF, /* 800KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_WCPU_ITCM1"
    },
    {
        .pci_start = 0x02000000,
        .pci_end   = 0x0207FFFF,
        .cpu_start = 0x02000000,
        .cpu_end   = 0x0207FFFF, /* 512KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_SHARE_PKTRAM"
    },
    {
        .pci_start = 0x40000000,
        .pci_end   = 0x4001EFFF,
        .cpu_start = 0x40000000,
        .cpu_end   = 0x4001EFFF, /* 124KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_AON_WAPB"
    },
    {
        .pci_start = 0x40105000,
        .pci_end   = 0x40106FFF,
        .cpu_start = 0x40105000,
        .cpu_end   = 0x40106FFF,
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_WCPU_CTL"
    },
    {
        .pci_start = 0x41026000,
        .pci_end   = 0x41027FFF,
        .cpu_start = 0x41026000,
        .cpu_end   = 0x41027FFF, /* 8KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_B_TRACE_MEM"
    },
    {
        .pci_start = 0x42026000,
        .pci_end   = 0x42027FFF,
        .cpu_start = 0x42026000,
        .cpu_end   = 0x42027FFF, /* 8KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_GLE_TRACE_MEM"
    },
    {
        .pci_start = 0x44400000,
        .pci_end   = 0x4445FFFF,
        .cpu_start = 0x44400000,
        .cpu_end   = 0x4445FFFF, /* 384KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_GLERAM0"
    },
    {
        .pci_start = 0x44480000,
        .pci_end   = 0x444F7FFF,
        .cpu_start = 0x44480000,
        .cpu_end   = 0x444F7FFF, /* 480KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_GLERAM1"
    },
    {
        .pci_start = 0x44600000,
        .pci_end   = 0x446C7FFF,
        .cpu_start = 0x44600000,
        .cpu_end   = 0x446C7FFF, /* 800KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_BITCM"
    },
    {
        .pci_start = 0x44700000,
        .pci_end   = 0x44733FFF,
        .cpu_start = 0x44700000,
        .cpu_end   = 0x44733FFF, /* 208KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_BDTCM"
    },
#endif
    {
        .pci_start = 0x50000000 + 0x2000000,
        .pci_end   = 0x5009FFFF + 0x2000000,
        .cpu_start = 0x50000000,
        .cpu_end   = 0x5009FFFF, /* 512KB + 128K */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_GMEM"
    },
    {
        .pci_start = 0x500A0000,
        .pci_end   = 0x500C7fff,
        .cpu_start = 0x500A0000,
        .cpu_end   = 0x500C7fff, /* 160KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_DSP0_ITCM"
    },
    {
        .pci_start = 0x500C8000,
        .pci_end   = 0x500FFFFF,
        .cpu_start = 0x500C8000,
        .cpu_end   = 0x500FFFFF, /* 224KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_DSP1_ITCM"
    },
    {
        .pci_start = 0x50100000 + 0x2000000,
        .pci_end   = 0x5017FFFF + 0x2000000,
        .cpu_start = 0x50100000,
        .cpu_end   = 0x5017FFFF, /* 512KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_PKTRAM"
    },
    {
        .pci_start = 0x50800000 + 0x2000000,
        .pci_end   = 0x508BFFFF + 0x2000000,
        .cpu_start = 0x50800000,
        .cpu_end   = 0x508BFFFF, /* 768KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_DSP0_DTCM"
    },
    {
        .pci_start = 0x50900000 + 0x2000000,
        .pci_end   = 0x509BFFFF + 0x2000000,
        .cpu_start = 0x50900000,
        .cpu_end   = 0x509BFFFF, /* 768KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_DSP1_DTCM"
    },
    {
        .pci_start = 0x51C4C000,
        .pci_end   = 0x51C4CFFF,
        .cpu_start = 0x51C4C000,
        .cpu_end   = 0x51C4CFFF,
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_PCIE_HOST_CTRL"
    },
    {
        .pci_start = 0x51C42000,
        .pci_end   = 0x51C42FFF,
        .cpu_start = 0x51C42000,
        .cpu_end   = 0x51C42FFF, /* 4KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_CTL"
    },
    {
        .pci_start = 0x53107000,
        .pci_end   = 0x5310CFFF,
        .cpu_start = 0x53107000,
        .cpu_end   = 0x5310CFFF,
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_G_PCIE_SUB"
    },
    {
        .pci_start = 0x51E00000,
        .pci_end   = 0x51E0AFFF,
        .cpu_start = 0x51E00000,
        .cpu_end   = 0x51E0AFFF, /* 44KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_DSP0_CTL"
    },
    {
        .pci_start = 0x51F00000,
        .pci_end   = 0x51F0AFFF,
        .cpu_start = 0x51F00000,
        .cpu_end   = 0x51F0AFFF, /* 44KB */
        .flag      = OAL_IORESOURCE_REG,
        .name      = "HI1161_DSP1_CTL"
    },
};
int32_t oal_pcie_get_bar_region_info_hi1161(oal_pcie_res *pst_pci_res,
                                            oal_pcie_region **region_base, uint32_t *region_num)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "1161 %s region map", hi110x_is_asic() ? "asic" : "fpga");
    *region_num = oal_array_size(g_hi1161_pcie_regions);
    *region_base = &g_hi1161_pcie_regions[0];

    return OAL_SUCC;
}

int32_t oal_pcie_set_outbound_membar_hi1161(oal_pcie_res *pst_pci_res, oal_pcie_iatu_bar *pst_iatu_bar)
{
    int32_t ret;
    ret = oal_pcie_set_outbound_iatu_by_membar(pst_iatu_bar->st_region.vaddr,
                                               0, HISI_PCIE_SLAVE_START_ADDRESS,
                                               HISI_PCIE_MASTER_START_ADDRESS,
                                               HISI_PCIE_IP_REGION_SIZE);
    if (ret) {
        return ret;
    }
    return OAL_SUCC;
}
#endif
