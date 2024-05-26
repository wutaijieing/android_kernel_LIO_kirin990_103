

#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_iatu.h"
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
#include "pcie_chip.h"
#include "oal_thread.h"
#include "oam_ext_if.h"

#include "pcie_reg.h"
#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"
#include "plat_firmware.h"
#include "plat_pm_wlan.h"
#include "board.h"
#include "securec.h"
#include "plat_pm.h"
#include "chip/comm/pcie_soc.h"

#ifdef _PRE_WLAN_PKT_TIME_STAT
#include <hwnet/ipv4/wifi_delayst.h>
#endif

OAL_STATIC oal_pcie_bar_info g_en_bar_tab[] = {
    /*
     * 1103 4.7a 一个BAR [8MB]， 5.0a 为两个BAR[Bar0 8M  BAR1 16KB]
     * (因为1103 是64bit bar,所以对应bar index寄存器, 是对应bar index=2,
     *  参考 __pci_read_base 最后一行),
     * 第二个BAR 直接用MEM 方式 访问IATU表
     */
    {
        .bar_idx = OAL_PCI_BAR_0,
    },
};

/* 函数定义 */
int32_t oal_pcie_disable_regions(oal_pcie_linux_res *pcie_res)
{
    if (oal_warn_on(pcie_res == NULL)) {
        return -OAL_ENODEV;
    }

    pcie_res->comm_res->regions.inited = 0;
    pci_print_log(PCI_LOG_DBG, "disable_regions");
    return OAL_SUCC;
}

int32_t oal_pcie_enable_regions(oal_pcie_linux_res *pcie_res)
{
    if (oal_warn_on(pcie_res == NULL)) {
        return -OAL_ENODEV;
    }

    pcie_res->comm_res->regions.inited = 1;

    pci_print_log(PCI_LOG_DBG, "enable_regions");
    return OAL_SUCC;
}


OAL_STATIC int32_t oal_pcie_set_inbound_by_viewport(oal_pcie_linux_res *pcie_res)
{
    uint32_t reg = 0;
    uint32_t ret;
    edma_paddr_t start, target, end;
    iatu_viewport_off vp;
    iatu_region_ctrl_2_off ctr2;
    int32_t index;
    int32_t region_num;
    oal_pcie_region *region_base;
    oal_pci_dev_stru *pci_dev = pcie_res->comm_res->pcie_dev;
    region_num = pcie_res->comm_res->regions.region_nums;
    region_base = pcie_res->comm_res->regions.pst_regions;

    for (index = 0; index < region_num; index++, region_base++) {
        vp.bits.region_dir = HI_PCI_IATU_INBOUND;
        vp.bits.region_index = (uint32_t)index; /* iatu index */
        ret = (uint32_t)oal_pci_write_config_dword(pci_dev, HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword);
        if (ret) {
            pci_print_log(PCI_LOG_ERR, "write [0x%8x:0x%8x] pcie failed, ret=%u\n",
                          HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword, ret);
            return -OAL_EIO;
        }

        /* 是否需要回读等待 */
        ret = (uint32_t)oal_pci_read_config_dword(pci_dev, HI_PCI_IATU_VIEWPORT_OFF, &reg);
        if (ret) {
            pci_print_log(PCI_LOG_ERR, "read [0x%8x] pcie failed, index:%d, ret=%u\n",
                          HI_PCI_IATU_VIEWPORT_OFF, index, ret);
            return -OAL_EIO;
        }

        if (reg != vp.as_dword) {
            /* 1.viewport 没有切换完成 2. iatu配置个数超过了Soc的最大个数 */
            pci_print_log(PCI_LOG_ERR,
                          "write [0x%8x:0x%8x] pcie viewport failed value still 0x%8x, region's index:%d\n",
                          HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword, reg, index);
            return -OAL_EIO;
        }

        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_region_ctrl_1_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF),
                                                    0x0);

        ctr2.as_dword = 0;
        ctr2.bits.region_en = 1;
        ctr2.bits.bar_num = region_base->bar_info->bar_idx;
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_region_ctrl_2_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF),
                                                    ctr2.as_dword);

        /* Host侧64位地址的低32位地址 */
        start.addr = region_base->bus_addr;
        pci_print_log(PCI_LOG_INFO, "PCIe inbound bus addr:0x%llx", start.addr);
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_lwr_base_addr_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF),
                                                    start.bits.low_addr);
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_upper_base_addr_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF),
                                                    start.bits.high_addr);

        end.addr = start.addr + region_base->size - 1;
        if (start.bits.high_addr != end.bits.high_addr) {
            /* 如果跨了4G地址应该多配置一个iatu表项，待增加 */
            pci_print_log(PCI_LOG_ERR, "iatu high 32 bits must same![start:0x%llx, end:0x%llx]", start.addr, end.addr);
            return -OAL_EIO;
        }
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_limit_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF),
                                                    end.bits.low_addr);

        /* Device侧对应的地址(PCI看到的地址) */
        target.addr = region_base->pci_start;
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_lwr_target_addr_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF), target.bits.low_addr);
        ret |= (uint32_t)oal_pci_write_config_dword(pci_dev,
                                                    hi_pci_iatu_upper_target_addr_off_inbound_i(
                                                        HI_PCI_IATU_BOUND_BASE_OFF),
                                                    target.bits.high_addr);
    }

    /* 配置命令寄存器                                                                         */
    /* BIT0 = 1(I/O Space Enable), BIT1 = 1(Memory Space Enable), BIT2 = 1(Bus Master Enable) */
    ret |= (uint32_t)oal_pci_write_config_word(pci_dev, 0x04, 0x7);
    if (ret) {
        pci_print_log(PCI_LOG_ERR, "pci write iatu config failed ret=%d\n", ret);
        return -OAL_EIO;
    }

    if (pci_dbg_condtion()) {
        oal_pcie_iatu_reg_dump(pcie_res);
    }
    return OAL_SUCC;
}

OAL_STATIC int32_t oal_pcie_set_inbound_by_membar(oal_pcie_linux_res *pcie_res)
{
    void *inbound_addr = NULL;

    int32_t ret;
    edma_paddr_t start, target, end;
    iatu_region_ctrl_2_off ctr2;
    int32_t index;
    int32_t region_num;
    oal_pcie_region *region_base;
    oal_pci_dev_stru *pci_dev = pcie_res->comm_res->pcie_dev;
    region_num = pcie_res->comm_res->regions.region_nums;
    region_base = pcie_res->comm_res->regions.pst_regions;

    if (pcie_res->comm_res->st_iatu_bar.st_region.vaddr == NULL) {
        pci_print_log(PCI_LOG_ERR, "iatu bar1 vaddr is null");
        return -OAL_ENOMEM;
    }

    inbound_addr = pcie_res->comm_res->st_iatu_bar.st_region.vaddr;

    for (index = 0; index < region_num; index++, region_base++) {
        if (index >= PCIE_INBOUND_REGIONS_MAX) { /* 设置的大小为0x2000，一次偏移0x200，设置了所有空间 */
            pci_print_log(PCI_LOG_ERR, "iatu regions too many, start:0x%llx", region_base->bar_info->start);
            break;
        }

        oal_writel(0x0, inbound_addr + hi_pci_iatu_region_ctrl_1_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));

        ctr2.as_dword = 0;
        ctr2.bits.region_en = 1;
        ctr2.bits.bar_num = region_base->bar_info->bar_idx;
        oal_writel(ctr2.as_dword,
                   inbound_addr + hi_pci_iatu_region_ctrl_2_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));

        /* Host侧64位地址的低32位地址 */
        start.addr = region_base->bus_addr;
        pci_print_log(PCI_LOG_INFO, "PCIe inbound bus addr:0x%llx", start.addr);
        oal_writel(start.bits.low_addr,
                   inbound_addr + hi_pci_iatu_lwr_base_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));
        oal_writel(start.bits.high_addr,
                   inbound_addr + hi_pci_iatu_upper_base_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));

        end.addr = start.addr + region_base->size - 1;
        if (start.bits.high_addr != end.bits.high_addr) {
            /* 如果跨了4G地址应该多配置一个iatu表项，待增加 */
            pci_print_log(PCI_LOG_ERR, "iatu high 32 bits must same![start:0x%llx, end:0x%llx]", start.addr, end.addr);
            return -OAL_EIO;
        }
        oal_writel(end.bits.low_addr,
                   inbound_addr + hi_pci_iatu_limit_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));

        /* Device侧对应的地址(PCI看到的地址) */
        target.addr = region_base->pci_start;
        oal_writel(target.bits.low_addr,
                   inbound_addr + hi_pci_iatu_lwr_target_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));
        oal_writel(target.bits.high_addr,
                   inbound_addr + hi_pci_iatu_upper_target_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)));
    }

    if (index) {
        /* 回读可以保证之前的IATU立刻生效 */
        uint32_t callback_read;
        callback_read = oal_readl(inbound_addr +
                                  hi_pci_iatu_region_ctrl_1_off_inbound_i(hi_pci_iatu_inbound_base_off(0)));
        oal_reference(callback_read);
    }

    /* 配置命令寄存器                                                                         */
    /* BIT0 = 1(I/O Space Enable), BIT1 = 1(Memory Space Enable), BIT2 = 1(Bus Master Enable) */
    ret = oal_pci_write_config_word(pci_dev, 0x04, 0x7);
    if (ret) {
        pci_print_log(PCI_LOG_ERR, "pci write iatu config failed ret=%d\n", ret);
        return -OAL_EIO;
    }

    if (pci_dbg_condtion()) {
        oal_pcie_iatu_reg_dump(pcie_res);
    }

    return OAL_SUCC;
}

OAL_STATIC int32_t oal_pcie_set_inbound(oal_pcie_linux_res *pcie_res)
{
    if (pcie_res->ep_res.chip_info.membar_support == OAL_FALSE) {
        return oal_pcie_set_inbound_by_viewport(pcie_res);
    } else {
        return oal_pcie_set_inbound_by_membar(pcie_res);
    }
}

int32_t oal_pcie_set_outbound_iatu_by_membar(void* pst_bar1_vaddr, uint32_t index, uint64_t src_addr,
                                             uint64_t dst_addr, uint64_t limit_size)
{
    /* IATU 对齐要求,开始结束地址按照4K对齐 */
    iatu_region_ctrl_1_off ctr1;
    iatu_region_ctrl_2_off ctr2;
    void *bound_addr = pst_bar1_vaddr;

    uint32_t src_addrl, src_addrh, value;
    uint32_t dst_addrl, dst_addrh;
    uint64_t limit_addr = src_addr + limit_size - 1;

    if (oal_warn_on(pst_bar1_vaddr == NULL)) {
        pci_print_log(PCI_LOG_ERR, "pst_bar1_vaddr is null");
        return -OAL_ENODEV;
    }

    src_addrl = (uint32_t)src_addr;
    src_addrh = (uint32_t)(src_addr >> 32); /* 32 high bits */
    dst_addrl = (uint32_t)dst_addr;
    dst_addrh = (uint32_t)(dst_addr >> 32); /* 32 high bits */

    ctr1.as_dword = 0;

    if (limit_addr >> 32) { // 32 hight bits
        ctr1.bits.inc_region_size = 1; /* more than 4G */
    }

    ctr2.as_dword = 0;
    ctr2.bits.region_en = 1;
    ctr2.bits.bar_num = 0x0;

    oal_writel(ctr1.as_dword, bound_addr +
               hi_pci_iatu_region_ctrl_1_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    oal_writel(ctr2.as_dword, bound_addr +
               hi_pci_iatu_region_ctrl_2_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));

    oal_writel(src_addrl, bound_addr +
               hi_pci_iatu_lwr_base_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    oal_writel(src_addrh, bound_addr +
               hi_pci_iatu_upper_base_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));

    oal_writel((uint32_t)limit_addr, bound_addr +
               hi_pci_iatu_limit_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    // 32 hight bits
    oal_writel((uint32_t)(limit_addr >> 32), bound_addr +
               hi_pci_iatu_uppr_limit_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));

    oal_writel(dst_addrl, bound_addr +
               hi_pci_iatu_lwr_target_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    oal_writel(dst_addrh,
               bound_addr + hi_pci_iatu_upper_target_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));

    value = oal_readl(bound_addr + hi_pci_iatu_lwr_base_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    if (value != src_addrl) {
        pci_print_log(PCI_LOG_ERR,
                      "callback read 0x%x failed src_addr=0x%llx, dst_addr=0x%llx limit=0x%llx, index = %u",
                      value, src_addr, dst_addr,  limit_addr, index);
        oal_pcie_iatu_outbound_dump_by_membar(pst_bar1_vaddr, index);
        return -OAL_EFAIL;
    } else {
        pci_print_log(PCI_LOG_INFO, "outbound  src_addr=0x%llx, dst_addr=0x%llx limit=0x%llx, index = %u",
                      src_addr, dst_addr, limit_addr, index);
        return OAL_SUCC;
    }
}

int32_t oal_pcie_set_outbound_by_membar(oal_pcie_linux_res *pcie_res)
{
    return oal_pcie_set_outbound_membar(pcie_res, &pcie_res->comm_res->st_iatu_bar);
}

/* set ep outbound, device->host */
OAL_STATIC int32_t oal_pcie_set_outbound(oal_pcie_linux_res *pcie_res)
{
    if (pcie_res->ep_res.chip_info.membar_support == OAL_TRUE) {
        return oal_pcie_set_outbound_by_membar(pcie_res);
    }
    return OAL_SUCC;
}

OAL_STATIC void oal_pcie_iatu_exit(oal_pcie_linux_res *pcie_res)
{
}

int32_t oal_pcie_iatu_init(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;

    if (pcie_res == NULL) {
        return -OAL_ENODEV;
    }

    if (!pcie_res->comm_res->regions.inited) {
        pci_print_log(PCI_LOG_ERR, "pcie regions is disabled, iatu config failed");
        return -OAL_EIO;
    }

    ret = oal_pcie_set_inbound(pcie_res);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "pcie inbound set failed ret=%d\n", ret);
        return ret;
    }

    ret = oal_pcie_set_outbound(pcie_res);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "pcie outbound set failed ret=%d\n", ret);
        return ret;
    }

    /* mem方式访问使能 */
    oal_pcie_change_link_state(pcie_res, PCI_WLAN_LINK_MEM_UP);
    return OAL_SUCC;
}


OAL_STATIC void oal_pcie_regions_exit(oal_pcie_linux_res *pcie_res)
{
    int32_t index;
    uint32_t region_num;
    oal_pcie_region *region_base = NULL;
    pci_print_log(PCI_LOG_INFO, "oal_pcie_regions_exit\n");
    pcie_res->comm_res->regions.inited = 0;

    region_num = pcie_res->comm_res->regions.region_nums;
    region_base = pcie_res->comm_res->regions.pst_regions;

    /* 释放申请的地址空间 */
    for (index = 0; index < region_num; index++, region_base++) {
        if (region_base->vaddr != NULL) {
            oal_iounmap(region_base->vaddr);
            oal_release_mem_region(region_base->paddr, region_base->size);
            region_base->vaddr = NULL;
        }
    }
}

void oal_pcie_bar1_exit(oal_pcie_iatu_bar *pst_iatu_bar)
{
    if (pst_iatu_bar->st_region.vaddr == NULL) {
        return;
    }

    oal_iounmap(pst_iatu_bar->st_region.vaddr);
    oal_release_mem_region(pst_iatu_bar->st_region.paddr, pst_iatu_bar->st_region.size);
    pst_iatu_bar->st_region.vaddr = NULL;
}

OAL_STATIC void oal_pcie_iatu_bar_exit(oal_pcie_linux_res *pcie_res)
{
    oal_pcie_bar1_exit(&pcie_res->comm_res->st_iatu_bar);
}

int32_t oal_pcie_bar1_init(oal_pcie_iatu_bar* pst_iatu_bar)
{
    oal_resource *pst_res = NULL;
    oal_pcie_bar_info *bar_base = NULL;
    oal_pcie_region *region_base = NULL;

    if (pst_iatu_bar->st_bar_info.size == 0) {
        return OAL_SUCC;
    }

    bar_base = &pst_iatu_bar->st_bar_info;
    region_base = &pst_iatu_bar->st_region;

    /* Bar1 专门用于配置 iatu表 */
    region_base->vaddr = NULL;            /* remap 后的虚拟地址 */
    region_base->paddr = bar_base->start; /* Host CPU看到的物理地址 */
    region_base->bus_addr = 0x0;
    region_base->res = NULL;
    region_base->bar_info = bar_base;
    region_base->size = bar_base->size;
    region_base->name = "iatu_bar1";
    region_base->flag = OAL_IORESOURCE_REG;

    pst_res = oal_request_mem_region(region_base->paddr, region_base->size, region_base->name);
    if (pst_res == NULL) {
        goto failed_request_region;
    }

    /* remap */
    if (region_base->flag & OAL_IORESOURCE_REG) {
        /* 寄存器映射成非cache段, 不需要刷cache */
        region_base->vaddr = oal_ioremap_nocache(region_base->paddr, region_base->size);
    } else {
        /* cache 段，注意要刷cache */
        region_base->vaddr = oal_ioremap(region_base->paddr, region_base->size);
    }

    if (region_base->vaddr == NULL) {
        oal_release_mem_region(region_base->paddr, region_base->size);
        goto failed_remap;
    }

    /* remap and request succ. */
    region_base->res = pst_res;

    pci_print_log(PCI_LOG_INFO, "iatu bar1 virtual address:%p", region_base->vaddr);

    return OAL_SUCC;
failed_remap:
    oal_iounmap(region_base->vaddr);
    oal_release_mem_region(region_base->paddr, region_base->size);
    region_base->vaddr = NULL;
failed_request_region:
    return -OAL_ENOMEM;
}

OAL_STATIC int32_t oal_pcie_iatu_bar_init(oal_pcie_linux_res *pcie_res)
{
    return oal_pcie_bar1_init(&pcie_res->comm_res->st_iatu_bar);
}

OAL_STATIC int32_t oal_pcie_regions_init(oal_pcie_linux_res *pcie_res)
{
    /* 初始化DEVICE 每个段分配的HOST物理地址，然后做remap */
    void *vaddr = NULL;
    int32_t index, region_idx;
    int32_t bar_num, region_num;
    uint32_t bar_used_size;
    oal_pcie_bar_info *bar_base = NULL;
    oal_pcie_region *region_base = NULL;
    oal_resource *pst_res = NULL;

    if (oal_warn_on(pcie_res->comm_res->regions.inited)) {
        /* 不能重复初始化 */
        return -OAL_EBUSY;
    }

    bar_num = pcie_res->comm_res->regions.bar_nums;
    region_num = pcie_res->comm_res->regions.region_nums;

    bar_base = pcie_res->comm_res->regions.pst_bars;
    region_base = pcie_res->comm_res->regions.pst_regions;

    /* 清空regions的特定字段 */
    for (index = 0; index < region_num; index++, region_base++) {
        region_base->vaddr = NULL; /* remap 后的虚拟地址 */
        region_base->paddr = 0x0;  /* Host CPU看到的物理地址 */
        region_base->bus_addr = 0x0;
        region_base->res = NULL;
        region_base->bar_info = NULL;
        region_base->size = region_base->pci_end - region_base->pci_start + 1;
    }

    region_idx = 0;
    bar_used_size = 0;
    bar_base = pcie_res->comm_res->regions.pst_bars;
    region_base = pcie_res->comm_res->regions.pst_regions;

    for (index = 0; index < bar_num; index++, bar_base++) {
        for (; region_idx < region_num; region_idx++, region_base++) {
            /* BAR可用的起始地址 */
            if (bar_base->start + bar_used_size + region_base->size - 1 > bar_base->end) {
                /* 这个BAR地址空间不足 */
                pci_print_log(PCI_LOG_ERR,
                              "index:%d,region_idx:%d, start:0x%llx ,end:0x%llx, used_size:0x%x, region_size:%u\n",
                              index, region_idx, bar_base->start, bar_base->end, bar_used_size, region_base->size);
                break;
            }

            region_base->paddr = bar_base->start + bar_used_size;
            region_base->bus_addr = bar_base->bus_start + bar_used_size;
            bar_used_size += region_base->size;
            region_base->bar_info = bar_base;
            pci_print_log(PCI_LOG_INFO, "bar idx:%d, region idx:%d, region paddr:0x%llx, region_size:%u\n",
                          index, region_idx, region_base->paddr, region_base->size);
        }
    }

    if (region_idx < region_num) {
        /* 地址不够用 */
        pci_print_log(PCI_LOG_ERR, "bar address range is too small, region_idx %d < region_num %d\n",
                      region_idx, region_num);
        return -OAL_ENOMEM;
    }

    pci_print_log(PCI_LOG_INFO, "Total region num:%d, size:%u\n", region_num, bar_used_size);

    region_base = pcie_res->comm_res->regions.pst_regions;
    for (index = 0; index < region_num; index++, region_base++) {
        if (!region_base->flag) {
            continue;
        }

        pst_res = oal_request_mem_region(region_base->paddr, region_base->size, region_base->name);
        if (pst_res == NULL) {
            goto failed_remap;
        }

        /* remap */
        if (region_base->flag & OAL_IORESOURCE_REG) {
            /* 寄存器映射成非cache段, 不需要刷cache */
            vaddr = oal_ioremap_nocache(region_base->paddr, region_base->size);
        } else {
            /* cache 段，注意要刷cache */
            vaddr = oal_ioremap(region_base->paddr, region_base->size);
        }

        if (vaddr == NULL) {
            oal_release_mem_region(region_base->paddr, region_base->size);
            goto failed_remap;
        }

        /* remap and request succ. */
        region_base->res = pst_res;
        region_base->vaddr = vaddr; /* Host Cpu 可以访问的虚拟地址 */
    }

    oal_pcie_enable_regions(pcie_res);

    pci_print_log(PCI_LOG_INFO, "oal_pcie_regions_init succ\n");
    return OAL_SUCC;
failed_remap:
    pci_print_log(PCI_LOG_ERR, "request mem region failed, addr:0x%llx, size:%u, name:%s\n",
                  region_base->paddr, region_base->size, region_base->name);
    oal_pcie_regions_exit(pcie_res);
    return -OAL_EIO;
}

void oal_pcie_bar_exit(oal_pcie_linux_res *pcie_res)
{
    oal_pcie_iatu_exit(pcie_res);
    oal_pcie_regions_exit(pcie_res);
    oal_pcie_iatu_bar_exit(pcie_res);
}


OAL_STATIC int32_t oal_pcie_mem_bar_init(oal_pci_dev_stru *pst_pci_dev, oal_pcie_bar_info *bar_curr)
{
    /* Get Bar Address */
    bar_curr->size = oal_pci_resource_len(pst_pci_dev, PCIE_IATU_BAR_INDEX);
    if (bar_curr->size == 0) {
        pci_print_log(PCI_LOG_ERR, "bar 1 size is zero, start:0x%lx, end:0x%lx",
                      oal_pci_resource_start(pst_pci_dev, PCIE_IATU_BAR_INDEX),
                      oal_pci_resource_end(pst_pci_dev, PCIE_IATU_BAR_INDEX));
        return -OAL_EIO;
    }

    bar_curr->end = oal_pci_resource_end(pst_pci_dev, PCIE_IATU_BAR_INDEX);
    bar_curr->start = oal_pci_resource_start(pst_pci_dev, PCIE_IATU_BAR_INDEX);
    bar_curr->bus_start = oal_pci_bus_address(pst_pci_dev, PCIE_IATU_BAR_INDEX);
    bar_curr->bar_idx = PCIE_IATU_BAR_INDEX;

    pci_print_log(PCI_LOG_INFO,
                  "preapre for bar idx:%u, phy start:0x%llx, end:0x%llx, bus address 0x%lx size:0x%x, flags:0x%lx\n",
                  PCIE_IATU_BAR_INDEX,
                  bar_curr->start,
                  bar_curr->end,
                  (uintptr_t)bar_curr->bus_start,
                  bar_curr->size,
                  oal_pci_resource_flags(pst_pci_dev, PCIE_IATU_BAR_INDEX));

    return OAL_SUCC;
}

int32_t oal_pcie_bar_init(oal_pcie_linux_res *pcie_res)
{
    int32_t index;
    int32_t ret;
    int32_t bar_num;
    int32_t region_num = 0;
    oal_pcie_bar_info *bar_base = NULL;
    oal_pcie_region *region_base = NULL;

    oal_pci_dev_stru *pst_pci_dev = pcie_res->comm_res->pcie_dev;

    /* 暂时只考虑1103 */
    bar_num = (int32_t)oal_array_size(g_en_bar_tab);
    bar_base = &g_en_bar_tab[0];

    ret = oal_pcie_get_bar_region_info(pcie_res, &region_base, &region_num);
    if (ret != OAL_SUCC) {
        return ret;
    }

    pci_print_log(PCI_LOG_INFO, "bar_num:%d, region_num:%d\n", bar_num, region_num);

    pcie_res->comm_res->regions.pst_bars = bar_base;
    pcie_res->comm_res->regions.bar_nums = bar_num;

    pcie_res->comm_res->regions.pst_regions = region_base;
    pcie_res->comm_res->regions.region_nums = region_num;

    /* 这里不映射，iatu配置要和映射分段对应 */
    for (index = 0; index < bar_num; index++) {
        /*
         * 获取Host分配的硬件地址资源,1103为8M大小,
         * 1103 4.7a 对应一个BAR, 5.0a 对应2个bar,
         * 其中第二个bar用于配置iatu表
         */
        oal_pcie_bar_info *bar_curr = bar_base + index;
        uint8_t bar_idx = bar_curr->bar_idx;

        /* pci resource built in pci_read_bases kernel. */
        bar_curr->start = oal_pci_resource_start(pst_pci_dev, bar_idx);
        bar_curr->end = oal_pci_resource_end(pst_pci_dev, bar_idx);
        bar_curr->bus_start = oal_pci_bus_address(pst_pci_dev, bar_idx);
        bar_curr->size = oal_pci_resource_len(pst_pci_dev, bar_idx);

        pci_print_log(PCI_LOG_INFO,
                      "preapre for bar idx:%u, \
                      phy start:0x%llx, end:0x%llx, bus address 0x%lx size:0x%x, flags:0x%lx\n",
                      bar_idx,
                      bar_curr->start, bar_curr->end,
                      (uintptr_t)bar_curr->bus_start,
                      bar_curr->size,
                      oal_pci_resource_flags(pst_pci_dev, bar_idx));
    }

    /* 是否支持BAR1 */
    if (pcie_res->ep_res.chip_info.membar_support == OAL_TRUE) {
        /* Get Bar Address */
        oal_pcie_bar_info *bar_curr = &pcie_res->comm_res->st_iatu_bar.st_bar_info;
        ret = oal_pcie_mem_bar_init(pst_pci_dev, bar_curr);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    ret = oal_pcie_iatu_bar_init(pcie_res);
    if (ret != OAL_SUCC) {
        return ret;
    }

    ret = oal_pcie_regions_init(pcie_res);
    if (ret != OAL_SUCC) {
        oal_pcie_iatu_bar_exit(pcie_res);
        return ret;
    }

    ret = oal_pcie_iatu_init(pcie_res);
    if (ret != OAL_SUCC) {
        oal_pcie_regions_exit(pcie_res);
        oal_pcie_iatu_bar_exit(pcie_res);
        return ret;
    }
    pci_print_log(PCI_LOG_INFO, "bar init succ");
    return OAL_SUCC;
}


int32_t oal_pcie_get_bar_region_info(oal_pcie_linux_res *pcie_res,
                                     oal_pcie_region **region_base, uint32_t *region_num)
{
    if (pcie_res->ep_res.chip_info.cb.pcie_get_bar_region_info == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_get_bar_region_info is null");
        return -OAL_ENODEV;
    }
    return pcie_res->ep_res.chip_info.cb.pcie_get_bar_region_info(pcie_res, region_base, region_num);
}

int32_t oal_pcie_set_outbound_membar(oal_pcie_linux_res *pcie_res, oal_pcie_iatu_bar *pst_iatu_bar)
{
    if (pcie_res->ep_res.chip_info.cb.pcie_set_outbound_membar == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_set_outbound_membar is null");
        return -OAL_ENODEV;
    }
    return pcie_res->ep_res.chip_info.cb.pcie_set_outbound_membar(pcie_res, pst_iatu_bar);
}
