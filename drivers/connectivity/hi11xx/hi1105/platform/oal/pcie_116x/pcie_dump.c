

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

char *g_pcie_link_state_str[PCI_WLAN_LINK_BUTT + 1] = {
    [PCI_WLAN_LINK_DOWN] = "linkdown",
    [PCI_WLAN_LINK_DEEPSLEEP] = "deepsleep",
    [PCI_WLAN_LINK_UP] = "linkup",
    [PCI_WLAN_LINK_MEM_UP] = "memup",
    [PCI_WLAN_LINK_RES_UP] = "resup",
    [PCI_WLAN_LINK_WORK_UP] = "workup",
    [PCI_WLAN_LINK_BUTT] = "butt"
};

OAL_STATIC int32_t oal_pcie_print_pcie_regs(oal_pcie_linux_res *pst_pci_res, uint32_t base, uint32_t size)
{
    int64_t i;
    uint32_t value;
    void *pst_mem = NULL;
    pci_addr_map addr_map;
    size = OAL_ROUND_UP(size, 4); /* 计算4字节对齐后的长度，默认进位 */
    if ((size == 0) || (size > 0xffff)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "size invalid %u\n", size);
        return -OAL_EFAIL;
    }

    if (oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, base, &addr_map) != OAL_SUCC) {
        /* share mem 地址未映射! */
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", base);
        return -OAL_EFAIL;
    }

    pst_mem = vmalloc(size);
    if (pst_mem == NULL) {
        pci_print_log(PCI_LOG_WARN, "vmalloc mem size %u failed", size);
    } else {
        memset_s(pst_mem, size, 0, size);
    }

    for (i = 0; i < (int32_t)size; i += sizeof(uint32_t)) { /* 每次偏移4字节 */
        value = oal_readl((void *)(uintptr_t)(addr_map.va + i));
        if (value == 0xffffffff) {
            if (oal_pcie_device_check_alive(pst_pci_res) != OAL_SUCC) {
                if (pst_mem != NULL) {
                    vfree(pst_mem);
                }
                return -OAL_ENODEV;
            }
        }

        if (pst_mem != NULL) {
            oal_writel(value, (uintptr_t)(pst_mem + i));
        } else {
            oal_io_print("%8llx:%8x\n", (int64_t)base + i, value);
        }
    }

    if (pst_mem != NULL) {
        if (i) {
            pci_print_log(PCI_LOG_INFO, "dump regs base 0x%x", base);
#ifdef CONFIG_PRINTK
            /* print to kenrel msg, 32B per */
            print_hex_dump(KERN_INFO, "pcie regs: ", DUMP_PREFIX_OFFSET, 32, 4,
                           pst_mem, i, false); /* 内核函数固定的传参 */
#endif
        }

        vfree(pst_mem);
    }

    return OAL_SUCC;
}

int32_t oal_pcie_print_device_transfer_info(oal_pcie_linux_res *pst_pcie_res)
{
    int32_t ret;
    pci_addr_map *addr_map = NULL;
    pcie_stats *stats = NULL;

    addr_map = &pst_pcie_res->ep_res.st_device_stat_map;
    stats = &pst_pcie_res->ep_res.st_device_stat;
    if (addr_map->va == 0) {
        pci_print_log(PCI_LOG_INFO, "st_device_stat_map null:va:%lu, pa:%lu\n", addr_map->va, addr_map->pa);
        return -OAL_EFAUL;
    }
    ret = oal_pcie_device_check_alive(pst_pcie_res);
    if (ret != OAL_SUCC) {
        return ret;
    }

    pci_print_log(PCI_LOG_INFO, "show device info:");
    oal_pcie_io_trans((uintptr_t)stats, addr_map->va, sizeof(*stats));

    pci_print_log(PCI_LOG_INFO, "d2h fifo_full:%u, fifo_notfull:%u ringbuf_hit:%u, ringbuf_miss:%u\n",
        stats->d2h_stats.stat.fifo_full, stats->d2h_stats.stat.fifo_notfull,
        stats->d2h_stats.stat.ringbuf_hit, stats->d2h_stats.stat.ringbuf_miss);
    pci_print_log(PCI_LOG_INFO, "d2h dma_busy:%u, dma_idle:%u fifo_ele_empty:%u doorbell count:%u\n",
        stats->d2h_stats.stat.fifo_dma_busy, stats->d2h_stats.stat.fifo_dma_idle,
        stats->d2h_stats.stat.fifo_ele_empty, stats->d2h_stats.stat.doorbell_isr_count);

    pci_print_log(PCI_LOG_INFO, "d2h push_fifo_count:%u done_isr_count:%u dma_work_list_stat:%u "
        "dma_free_list_stat:%u dma_pending_list_stat:%u", stats->d2h_stats.stat.push_fifo_count,
        stats->d2h_stats.stat.done_isr_count, stats->d2h_stats.stat.dma_work_list_stat,
        stats->d2h_stats.stat.dma_free_list_stat, stats->d2h_stats.stat.dma_pending_list_stat);

    pci_print_log(PCI_LOG_INFO,
        "h2d fifo_full:%u, fifo_notfull:%u ringbuf_hit:%u, ringbuf_miss:%u fifo_ele_empty:%u\n",
        stats->h2d_stats.stat.fifo_full, stats->h2d_stats.stat.fifo_notfull, stats->h2d_stats.stat.ringbuf_hit,
        stats->h2d_stats.stat.ringbuf_miss, stats->h2d_stats.stat.fifo_ele_empty);

    pci_print_log(PCI_LOG_INFO,
        "h2d push_fifo_count:%u done_isr_count:%u dma_work_list_stat:%u dma_free_list_stat:%u "
        "dma_pending_list_stat:%u", stats->h2d_stats.stat.push_fifo_count,
        stats->h2d_stats.stat.done_isr_count, stats->h2d_stats.stat.dma_work_list_stat,
        stats->h2d_stats.stat.dma_free_list_stat, stats->h2d_stats.stat.dma_pending_list_stat);

    pci_print_log(PCI_LOG_INFO,
        "comm_stat l1_wake_force_push_cnt:%u l1_wake_l1_hit:%u l1_wake_l1_miss:%u "
        "l1_wake_state_err_cnt:%u l1_wake_timeout_cnt:%u l1_wake_timeout_max_cnt:%u",
        stats->comm_stat.l1_wake_force_push_cnt, stats->comm_stat.l1_wake_l1_hit,
        stats->comm_stat.l1_wake_l1_miss, stats->comm_stat.l1_wake_state_err_cnt,
        stats->comm_stat.l1_wake_timeout_cnt, stats->comm_stat.l1_wake_timeout_max_cnt);
    if (stats->comm_stat.l1_wake_force_push_cnt) {
        declare_dft_trace_key_info("l1_wake_force_push_error", OAL_DFT_TRACE_FAIL);
    }
    if (stats->comm_stat.l1_wake_state_err_cnt) {
        declare_dft_trace_key_info("l1_wake_state_err", OAL_DFT_TRACE_FAIL);
    }
    return OAL_SUCC;
}

OAL_STATIC void oal_pcie_print_all_ringbuf_info(oal_pcie_linux_res *pst_pci_res)
{
    int32_t i;

    oal_pcie_print_ringbuf_info(&pst_pci_res->ep_res.st_ringbuf.st_d2h_buf, PCI_LOG_INFO);
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        oal_pcie_print_ringbuf_info(&pst_pci_res->ep_res.st_ringbuf.st_h2d_buf[i], PCI_LOG_INFO);
    }
    oal_pcie_print_ringbuf_info(&pst_pci_res->ep_res.st_ringbuf.st_d2h_msg, PCI_LOG_INFO);
    oal_pcie_print_ringbuf_info(&pst_pci_res->ep_res.st_ringbuf.st_h2d_msg, PCI_LOG_INFO);
}

static void oal_pcie_print_comm_info(oal_pcie_linux_res *pst_pci_res)
{
    if (pst_pci_res->ep_res.stat.intx_total_count) {
        pci_print_log(PCI_LOG_INFO, "intx_total_count:%u", pst_pci_res->ep_res.stat.intx_total_count);
    }
    if (pst_pci_res->ep_res.stat.intx_tx_count) {
        pci_print_log(PCI_LOG_INFO, "intx_tx_count:%u", pst_pci_res->ep_res.stat.intx_tx_count);
    }
    if (pst_pci_res->ep_res.stat.intx_rx_count) {
        pci_print_log(PCI_LOG_INFO, "intx_rx_count:%u", pst_pci_res->ep_res.stat.intx_rx_count);
    }
    if (pst_pci_res->ep_res.stat.done_err_cnt) {
        pci_print_log(PCI_LOG_INFO, "done_err_cnt:%u", pst_pci_res->ep_res.stat.done_err_cnt);
    }
    if (pst_pci_res->ep_res.stat.h2d_doorbell_cnt) {
        pci_print_log(PCI_LOG_INFO, "h2d_doorbell_cnt:%u", pst_pci_res->ep_res.stat.h2d_doorbell_cnt);
    }
    if (pst_pci_res->ep_res.stat.d2h_doorbell_cnt) {
        pci_print_log(PCI_LOG_INFO, "d2h_doorbell_cnt:%u", pst_pci_res->ep_res.stat.d2h_doorbell_cnt);
    }
    if (pst_pci_res->ep_res.st_rx_res.stat.rx_count) {
        pci_print_log(PCI_LOG_INFO, "rx_count:%u", pst_pci_res->ep_res.st_rx_res.stat.rx_count);
    }
    if (pst_pci_res->ep_res.st_rx_res.stat.rx_done_count) {
        pci_print_log(PCI_LOG_INFO, "rx_done_count:%u", pst_pci_res->ep_res.st_rx_res.stat.rx_done_count);
    }
    if (pst_pci_res->ep_res.st_rx_res.stat.alloc_netbuf_failed) {
        pci_print_log(PCI_LOG_INFO, "alloc_netbuf_failed:%u", pst_pci_res->ep_res.st_rx_res.stat.alloc_netbuf_failed);
    }
    if (pst_pci_res->ep_res.st_rx_res.stat.map_netbuf_failed) {
        pci_print_log(PCI_LOG_INFO, "map_netbuf_failed:%u", pst_pci_res->ep_res.st_rx_res.stat.map_netbuf_failed);
    }
}

static void oal_pcie_print_one_transfer_stat_info(oal_pcie_ep_res *ep_res)
{
    int32_t i, j;
    uint32_t len;
    uint32_t total_len = 0;

    /* tx info */
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        if (ep_res->st_tx_res[i].stat.tx_count) {
            pci_print_log(PCI_LOG_INFO, "[qid:%d]tx_count:%u", i, ep_res->st_tx_res[i].stat.tx_count);
        }
        if (ep_res->st_tx_res[i].stat.tx_done_count) {
            pci_print_log(PCI_LOG_INFO, "[qid:%d]tx_done_count:%u", i,
                          ep_res->st_tx_res[i].stat.tx_done_count);
        }
        len = oal_netbuf_list_len(&ep_res->st_tx_res[i].txq);
        if (len) {
            pci_print_log(PCI_LOG_INFO, "[qid:%d]len=%d", i, len);
            total_len += len;
        }

        pci_print_log(PCI_LOG_INFO, "[qid:%d]tx ringbuf cond is %d",
                      i, oal_atomic_read(&ep_res->st_tx_res[i].tx_ringbuf_sync_cond));
    }

    /* burst info */
    for (i = 0; i < PCIE_EDMA_WRITE_BUSRT_COUNT + 1; i++) {
        if (ep_res->st_rx_res.stat.rx_burst_cnt[i]) {
            pci_print_log(PCI_LOG_INFO, "rx burst %d count:%u", i, ep_res->st_rx_res.stat.rx_burst_cnt[i]);
        }
    }

    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        for (j = 0; j < PCIE_EDMA_READ_BUSRT_COUNT + 1; j++) {
            if (ep_res->st_tx_res[i].stat.tx_burst_cnt[j]) {
                pci_print_log(PCI_LOG_INFO, "tx qid %d burst %d count:%u",
                              i, j, ep_res->st_tx_res[i].stat.tx_burst_cnt[j]);
            }
        }
    }
}

static void oal_pcie_print_transfer_stat_info(oal_pcie_linux_res *pst_pci_res)
{
    oal_pcie_print_one_transfer_stat_info(&pst_pci_res->ep_res);
}

void oal_pcie_print_transfer_info(oal_pcie_linux_res *pst_pci_res, uint64_t print_flag)
{
    if (pst_pci_res == NULL) {
        return;
    }

    pci_print_log(PCI_LOG_INFO, "pcie transfer info:");
    oal_pcie_print_comm_info(pst_pci_res);
    oal_pcie_print_transfer_stat_info(pst_pci_res);
    oal_pcie_print_all_ringbuf_info(pst_pci_res);

    /* dump pcie hardware info */
    if (oal_unlikely(pst_pci_res->comm_res->link_state >= PCI_WLAN_LINK_WORK_UP)) {
        if (board_get_host_wakeup_dev_state(W_SYS) == 1) {
            /* gpio is high axi is alive */
            if (print_flag & HCC_PRINT_TRANS_FLAG_DEVICE_REGS) {
                oal_pcie_print_pcie_regs(pst_pci_res, PCIE_CTRL_BASE_ADDR, 0x4c8 + 0x4);
                oal_pcie_print_pcie_regs(pst_pci_res, PCIE_DMA_CTRL_BASE_ADDR, 0x30 + 0x4);
            }

            if (print_flag & HCC_PRINT_TRANS_FLAG_DEVICE_STAT) {
                /* dump pcie status */
                oal_pcie_print_device_transfer_info(pst_pci_res);
            }
        }
    } else {
        pci_print_log(PCI_LOG_INFO, "pcie is %s", g_pcie_link_state_str[pst_pci_res->comm_res->link_state]);
    }
}

OAL_STATIC void oal_pcie_iatu_reg_dump_by_viewport(oal_pcie_linux_res *pci_res)
{
    int32_t index;
    int32_t ret;
    uint32_t reg = 0;
    int32_t region_num;
    iatu_viewport_off vp;
    oal_pcie_region *region_base;
    oal_pci_dev_stru *pci_dev = pci_res->comm_res->pcie_dev;

    region_num = pci_res->comm_res->regions.region_nums;
    region_base = pci_res->comm_res->regions.pst_regions;
    for (index = 0; index < region_num; index++, region_base++) {
        vp.bits.region_dir = HI_PCI_IATU_INBOUND;
        vp.bits.region_index = (uint32_t)index; /* iatu index */
        ret = oal_pci_write_config_dword(pci_dev, HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword);
        if (ret) {
            pci_print_log(PCI_LOG_ERR, "dump write [0x%8x:0x%8x] pcie failed, ret=%d\n",
                          HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword, ret);
            break;
        }

        ret = oal_pci_read_config_dword(pci_dev, HI_PCI_IATU_VIEWPORT_OFF, &reg);
        if (ret) {
            pci_print_log(PCI_LOG_ERR, "dump read [0x%8x] pcie failed, ret=%d\n", HI_PCI_IATU_VIEWPORT_OFF, ret);
            break;
        }

        pci_print_log(PCI_LOG_INFO, "INBOUND iatu index:%d 's register:\n", index);

        if (reg != vp.as_dword) {
            pci_print_log(PCI_LOG_ERR, "dump write [0x%8x:0x%8x] pcie viewport failed value still 0x%8x\n",
                          HI_PCI_IATU_VIEWPORT_OFF, vp.as_dword, reg);
            break;
        }

        print_pcie_config_reg(pci_dev, HI_PCI_IATU_VIEWPORT_OFF);

        print_pcie_config_reg(pci_dev, hi_pci_iatu_region_ctrl_1_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_region_ctrl_2_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_lwr_base_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_upper_base_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_limit_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_lwr_target_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));

        print_pcie_config_reg(pci_dev, hi_pci_iatu_upper_target_addr_off_inbound_i(HI_PCI_IATU_BOUND_BASE_OFF));
    }
}

OAL_STATIC void oal_pcie_iatu_reg_dump_by_membar(oal_pcie_linux_res *pci_res)
{
    void *inbound_addr = NULL;

    int32_t index;
    int32_t region_num;
    oal_pcie_region *region_base = NULL;

    if (pci_res->comm_res->st_iatu_bar.st_region.vaddr == NULL) {
        pci_print_log(PCI_LOG_ERR, "iatu bar1 vaddr is null");
        return;
    }

    inbound_addr = pci_res->comm_res->st_iatu_bar.st_region.vaddr;

    region_num = pci_res->comm_res->regions.region_nums;
    region_base = pci_res->comm_res->regions.pst_regions;
    for (index = 0; index < region_num; index++, region_base++) {
        if (index >= 16) { /* dump size为0x2000，一次偏移0x200，这里16代表dump了所有空间 */
            break;
        }

        pci_print_log(PCI_LOG_INFO, "INBOUND iatu index:%d 's register:\n", index);

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_region_ctrl_1_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_region_ctrl_1_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_region_ctrl_2_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_region_ctrl_2_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_lwr_base_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_lwr_base_addr_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_upper_base_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_upper_base_addr_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_limit_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_limit_addr_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_lwr_target_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_lwr_target_addr_off_inbound_i");

        oal_pcie_print_config_reg_bar(pci_res,
                                      hi_pci_iatu_upper_target_addr_off_inbound_i(hi_pci_iatu_inbound_base_off(index)),
                                      "hi_pci_iatu_upper_target_addr_off_inbound_i");
    }
}

/* bar and iATU table config */
/* set ep inbound, host->device */
void oal_pcie_iatu_reg_dump(oal_pcie_linux_res *pci_res)
{
    if (pci_res->ep_res.chip_info.membar_support == OAL_FALSE) {
        oal_pcie_iatu_reg_dump_by_viewport(pci_res);
    } else {
        oal_pcie_iatu_reg_dump_by_membar(pci_res);
    }
}

void oal_pcie_regions_info_dump(oal_pcie_comm_res *comm_res)
{
    int32_t index;
    int32_t region_num;
    oal_pcie_region *region_base;

    region_num = comm_res->regions.region_nums;
    region_base = comm_res->regions.pst_regions;

    if (oal_warn_on(!comm_res->regions.inited)) {
        return;
    }

    if (region_num) {
        oal_io_print("regions[%d] info dump\n", region_num);
    }

    for (index = 0; index < region_num; index++, region_base++) {
        oal_io_print("[%15s]va:0x%p, pa:0x%llx, [pci start:0x%8llx end:0x%8llx],[cpu start:0x%8llx end:0x%8llx],\
                     size:%u, flag:0x%x\n",
                     region_base->name,
                     region_base->vaddr,
                     region_base->paddr,
                     region_base->pci_start,
                     region_base->pci_end,
                     region_base->cpu_start,
                     region_base->cpu_end,
                     region_base->size,
                     region_base->flag);
    }
}


void oal_pcie_iatu_outbound_dump_by_membar(const void *outbound_addr, uint32_t index)
{
    if (outbound_addr == NULL) {
        pci_print_log(PCI_LOG_ERR, "iatu bar1 vaddr is null");
        return;
    }

    pci_print_log(PCI_LOG_INFO, "OUTBOUND iatu index:%d 's register:\n", index);
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_region_ctrl_1_off_outbound_i",
                  oal_readl(outbound_addr
                  + hi_pci_iatu_region_ctrl_1_off_outbound_i(hi_pci_iatu_outbound_base_off(index))));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_region_ctrl_2_off_outbound_i",
                  oal_readl(outbound_addr + hi_pci_iatu_region_ctrl_2_off_outbound_i(
                      hi_pci_iatu_outbound_base_off(index))));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_lwr_base_addr_off_outbound_i",
                  hi_pci_iatu_lwr_base_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_upper_base_addr_off_outbound_i",
                  hi_pci_iatu_upper_base_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_limit_addr_off_outbound_i",
                  hi_pci_iatu_limit_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_lwr_target_addr_off_outbound_i",
                  hi_pci_iatu_lwr_target_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
    pci_print_log(PCI_LOG_INFO, "%s : 0x%x", "hi_pci_iatu_upper_target_addr_off_outbound_i",
                  hi_pci_iatu_upper_target_addr_off_outbound_i(hi_pci_iatu_outbound_base_off(index)));
}

void oal_pcie_print_ringbuf_info(pcie_ringbuf *pst_ringbuf, pci_log_type level)
{
    if (oal_warn_on(pst_ringbuf == NULL)) {
        return;
    }

    /* dump the ringbuf info */
    pci_print_log(level, "ringbuf[0x%p] idx:%u, rd:%u, wr:%u, size:%u, item_len:%u, item_mask:0x%x, base_addr:0x%llx",
                  pst_ringbuf,
                  pst_ringbuf->idx,
                  pst_ringbuf->rd,
                  pst_ringbuf->wr,
                  pst_ringbuf->size,
                  pst_ringbuf->item_len,
                  pst_ringbuf->item_mask,
                  pst_ringbuf->base_addr);
}
