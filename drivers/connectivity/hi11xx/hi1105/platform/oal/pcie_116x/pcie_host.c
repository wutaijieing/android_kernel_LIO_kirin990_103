

#define HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define HISI_LOG_TAG           "[PCIE]"
#include "pcie_host.h"
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "securec.h"
#include "board.h"
#include "chip/comm/pcie_soc.h"
#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"
#include "oal_thread.h"
#include "oam_ext_if.h"
#include "pcie_chip.h"
#include "pcie_iatu.h"
#include "pcie_reg.h"
#include "plat_firmware.h"
#include "plat_pm.h"
#include "plat_pm_wlan.h"

#ifdef _PRE_WLAN_PKT_TIME_STAT
#include <hwnet/ipv4/wifi_delayst.h>
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCIE_HOST_C
OAL_STATIC oal_kobject *g_conn_syfs_pci_object = NULL;

int32_t g_hipcie_loglevel = PCI_LOG_INFO;
oal_debug_module_param(g_hipcie_loglevel, int, S_IRUGO | S_IWUSR);

/* soft fifo code had deleted 2020-8-21 master */
int32_t g_pcie_soft_fifo_enable = 0;
oal_debug_module_param(g_pcie_soft_fifo_enable, int, S_IRUGO | S_IWUSR);

int32_t g_pcie_ringbuf_bugfix_enable = 1;
oal_debug_module_param(g_pcie_ringbuf_bugfix_enable, int, S_IRUGO | S_IWUSR);

int32_t g_pcie_dma_data_check_enable = 0; /* Wi-Fi关闭时可以修改此标记 */
oal_debug_module_param(g_pcie_dma_data_check_enable, int, S_IRUGO | S_IWUSR);

int32_t g_pcie_dma_data_rx_hdr_init = 0; /* Wi-Fi关闭时可以修改此标记 */
oal_debug_module_param(g_pcie_dma_data_rx_hdr_init, int, S_IRUGO | S_IWUSR);

/* 0 memcopy from kernel ,1 memcopy from self */
int32_t g_pcie_memcopy_type = 0;
oal_debug_module_param(g_pcie_memcopy_type, int, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL_GPL(g_pcie_memcopy_type);

char *g_pci_loglevel_format[] = {
    KERN_ERR "[PCIE][ERR] ",
    KERN_WARNING "[PCIE][WARN]",
    KERN_INFO "[PCIE][INFO]",
    KERN_INFO "[PCIE][DBG] ",
};

/* Function Declare */
void oal_pcie_tx_netbuf_free(oal_pcie_linux_res *pst_pci_res, oal_netbuf_stru *pst_netbuf);
int32_t oal_pcie_ringbuf_read_rd(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type, int32_t ep_id);
int32_t oal_pcie_ringbuf_write(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type, uint8_t *buf,
                               uint32_t len);
uint32_t oal_pcie_comm_ringbuf_freecount(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type);

/* 函数定义 */
OAL_STATIC void oal_pcie_io_trans64_sub(uint64_t *dst, uint64_t *src, int32_t size)
{
    int32_t remain = size;

    forever_loop()
    {
        if (remain < 8) { /* 8bytes 最少传输字节数，剩余数据量不足时退出 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            rmb();
            wmb(); /* dsb */
#endif
            break;
        }
        if (remain >= 128) { /* 128 bytes, 根据剩余数据量拷贝，一次性拷贝128字节 */
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            remain -= 128;         // 128 bytes
        } else if (remain >= 64) { /* 64bytes 根据剩余数据量拷贝，一次性拷贝64字节 */
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            remain -= 64;          // 64 bytes
        } else if (remain >= 32) { /* 32 bytes 根据剩余数据量拷贝，一次性拷贝32字节 */
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            remain -= 32;          // 32 bytes
        } else if (remain >= 16) { /* 16 bytes 根据剩余数据量拷贝，一次性拷贝16字节 */
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            remain -= 16; // 16 bytes
        } else {
            *((volatile uint64_t *)dst++) = *((volatile uint64_t *)src++);
            remain -= 8; /* 剩余数据量为8到15字节时，只拷贝8字节 */
        }
    }
}

void oal_pcie_io_trans32(uint32_t *dst, uint32_t *src, int32_t size)
{
    int32_t remain = size;

    forever_loop()
    {
        if (remain < 4) { /* 4 bytes 最少传输字节数，剩余数据量不足时退出 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            rmb();
            wmb(); /* dsb */
#endif
            break;
        }
        if (remain >= 64) { /* 64 bytes 根据剩余数据量拷贝，一次性拷贝64字节 */
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            remain -= 64;          // 64 bytes
        } else if (remain >= 32) { /* 32 bytes 根据剩余数据量拷贝，一次性拷贝32字节 */
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            remain -= 32;          // 32 bytes
        } else if (remain >= 16) { /* 16 bytes 根据剩余数据量拷贝，一次性拷贝16字节 */
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            remain -= 16;         // 16 bytes
        } else if (remain >= 8) { /* 8 bytes 根据剩余数据量拷贝，一次性拷贝8字节 */
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            remain -= 8; // 8 bytes
        } else {
            *((volatile uint32_t *)dst++) = *((volatile uint32_t *)src++);
            remain -= 4; /* 4 bytes 剩余数据量为4到7字节时，只拷贝4字节 */
        }
    }
}

void oal_pcie_io_trans64(void *dst, void *src, int32_t size)
{
    uint32_t copy_size;
    int32_t remain = size;
    forever_loop()
    {
        if (remain < 4) { /* 4 bytes 最少传输字节数，剩余数据量不足时退出 */
            break;
        }

        if ((!((uintptr_t)src & 0x7)) && (remain >= 8)) {    /* 8bytes */
            copy_size = OAL_ROUND_DOWN((uint32_t)remain, 8); /* 清除低3bit，保证8字节对齐 */
            remain -= (int32_t)copy_size;
            oal_pcie_io_trans64_sub(dst, src, (int32_t)copy_size);
            src += copy_size;
            dst += copy_size;
        } else if ((!((uintptr_t)src & 0x3)) && (remain >= 4)) { /* 4bytes */
            remain -= 4;                                         /* 拷贝4字节，则长度减少4 */
            *((volatile uint32_t *)dst) = *((volatile uint32_t *)src);
            /* 每次偏移4字节 */
            dst += sizeof(uint32_t);
            src += sizeof(uint32_t);
        } else {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid argument, dst:%pK , src:%pK, remain:%d", dst, src, remain);
        }
    }
}

int32_t oal_pcie_memport_copy_test(void)
{
    unsigned long burst_size = 4096; // 4096 one mem page
    unsigned long timeout;
    unsigned long total_size;
    declare_time_cost_stru(cost);
    void *buff_src = NULL;
    void *buff_dst = NULL;
    uint64_t trans_size, us_to_s;

    buff_src = oal_memalloc(burst_size);
    if (buff_src == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %lu buff failed", burst_size);
        return -OAL_EFAIL;
    }

    buff_dst = oal_memalloc(burst_size);
    if (buff_dst == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %lu buff failed", burst_size);
        oal_free(buff_src);
        return -OAL_EFAIL;
    }

    oal_get_time_cost_start(cost);
    timeout = jiffies + oal_msecs_to_jiffies(2000); // 2000ms timeout
    total_size = 0;
    forever_loop()
    {
        if (oal_time_after(jiffies, timeout)) {
            break;
        }

        oal_pcie_io_trans((uintptr_t)buff_dst, (uintptr_t)buff_src, burst_size);
        total_size += burst_size;
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);

    us_to_s = time_cost_var_sub(cost);
    trans_size = ((total_size * 1000u * 1000u) >> PCIE_TRANS_US_OFFSET_BITS); // 1000 calc
    trans_size = div_u64(trans_size, us_to_s);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "memcopy: %llu Mbps, trans_time:%llu us, tran_size:%lu", trans_size, us_to_s,
                         total_size);

    oal_free(buff_src);
    oal_free(buff_dst);
    return 0;
}

hcc_bus *oal_pcie_get_bus(uint32_t dev_id)
{
    hcc_bus *pst_bus = NULL;
    hcc_bus_dev *pst_bus_dev = hcc_get_bus_dev(dev_id);

    if (pst_bus_dev == NULL) {
        return NULL;
    }
    pst_bus = pst_bus_dev->cur_bus;

    if (hcc_bus_isvalid(pst_bus) != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus %d is not ready", HCC_BUS_PCIE);
        return NULL;
    }

    return pst_bus;
}

hcc_bus *oal_pcie_get_current_bus(void)
{
    return oal_pcie_get_bus(hcc_get_main_dev());
}

oal_bool_enum_uint8 oal_pcie_link_state_up(uint8_t dev_id)
{
    hcc_bus *hi_bus = oal_pcie_get_bus(dev_id);
    oal_pcie_linux_res *pci_lres = NULL;

    if (hi_bus == NULL) {
        return OAL_FALSE;
    }

    pci_lres = (oal_pcie_linux_res *)hi_bus->data;
    if (pci_lres == NULL) {
        return OAL_FALSE;
    }

    return pci_lres->comm_res->link_state == PCI_WLAN_LINK_WORK_UP;
}
oal_module_symbol(oal_pcie_link_state_up);

int32_t oal_pcie_device_check_alive(oal_pcie_linux_res *pst_pci_res)
{
    int32_t ret;
    uint32_t value;
    pci_addr_map addr_map;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, pst_pci_res->ep_res.chip_info.addr_info.glb_ctrl, &addr_map);
    if (oal_unlikely(ret != OAL_SUCC)) {
        /* share mem 地址未映射! */
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", 0x50000000);
        return -OAL_EFAIL;
    }

    value = oal_readl((void *)addr_map.va);
    if (value == 0x101) {
        return OAL_SUCC;
    } else {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "pcie maybe linkdown, glbctrl=0x%x", value);
        return -OAL_EFAIL;
    }
}

/* first power init */
int32_t oal_pcie_transfer_res_init(oal_pcie_linux_res *pst_pci_res)
{
    return oal_ete_transfer_res_init(pst_pci_res);
}

void oal_pcie_transfer_res_exit(oal_pcie_linux_res *pst_pci_res)
{
    oal_ete_transfer_res_exit(pst_pci_res);
}

int32_t oal_pcie_check_link_state(oal_pcie_linux_res *pst_pci_res)
{
    int32_t ret;
    pci_addr_map addr_map;
    uint32_t share_mem_address; /* Device cpu地址 */
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = pst_pci_res->comm_res->pcie_dev;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, pst_pci_res->ep_res.chip_info.addr_info.sharemem_addr,
                                    &addr_map);
    if (oal_unlikely(ret != OAL_SUCC)) {
        /* share mem 地址未映射! */
        pci_print_log(PCI_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n",
                      pst_pci_res->ep_res.chip_info.addr_info.sharemem_addr);
        return OAL_FALSE;
    }

    /* Get sharemem's dev_cpu address */
    oal_pcie_io_trans((uintptr_t)&share_mem_address, addr_map.va, sizeof(share_mem_address));

    if (share_mem_address == 0xFFFFFFFF) {
        uint32_t version = 0;

        declare_dft_trace_key_info("pcie_detect_linkdown", OAL_DFT_TRACE_EXCEP);

        ret = oal_pci_read_config_dword(pst_pci_dev, 0x0, &version);

        /* if pci config succ, pcie link still up, but pci master is down,
         * rc mem port is issue or ep axi bus is gating
         * if pci config failed, pcie maybe link down */
        pci_print_log(PCI_LOG_INFO, "read pci version 0x%8x ret=%d, host wakeup dev gpio:%d", version, ret,
                      hcc_bus_get_sleep_state(pst_pci_res->pst_bus));
        do {
            oal_pcie_change_link_state(pst_pci_res, PCI_WLAN_LINK_DOWN);

#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
            if (board_get_dev_wkup_host_state(W_SYS) == 0) {
                (void)ssi_dump_err_regs(SSI_ERR_PCIE_CHECK_LINK_FAIL);
            } else {
                pci_print_log(PCI_LOG_INFO, "dev wakeup gpio is high, dev maybe panic");
            }
#endif
            oal_pci_disable_device(pst_pci_dev);
            oal_disable_pcie_irq(pst_pci_res);
        } while (0);
        return OAL_FALSE;
    } else {
        return OAL_TRUE;
    }
}

/* device shared mem write */
int32_t oal_pcie_write_dsm32(oal_pcie_linux_res *pcie_res, pcie_shared_device_addr_type type, uint32_t val)
{
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    if (oal_warn_on((uint32_t)type >= (uint32_t)PCIE_SHARED_ADDR_BUTT)) {
        pci_print_log(PCI_LOG_WARN, "invalid device addr type:%d", type);
        return -OAL_EINVAL;
    }

    if (oal_warn_on(ep_res == NULL)) {
        pci_print_log(PCI_LOG_ERR, "pci res is null");
        return -OAL_ENODEV;
    }

    if (oal_warn_on(ep_res->st_device_shared_addr_map[type].va == 0)) {
        pci_print_log(PCI_LOG_ERR, "dsm type:%d va is null", type);
        return -OAL_ENODEV;
    }

    oal_writel(val, (void *)ep_res->st_device_shared_addr_map[type].va);

    return OAL_SUCC;
}

/* device shared mem read */
int32_t oal_pcie_read_dsm32(oal_pcie_linux_res *pcie_res, pcie_shared_device_addr_type type, uint32_t *val)
{
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;
    if (oal_warn_on((uint32_t)type >= (uint32_t)PCIE_SHARED_ADDR_BUTT)) {
        pci_print_log(PCI_LOG_WARN, "invalid device addr type:%d", type);
        return -OAL_EINVAL;
    }

    if (oal_warn_on(ep_res == NULL)) {
        pci_print_log(PCI_LOG_ERR, "pci res is null");
        return -OAL_ENODEV;
    }

    if (oal_warn_on(ep_res->st_device_shared_addr_map[type].va == 0)) {
        pci_print_log(PCI_LOG_ERR, "dsm type:%d va is null", type);
        return -OAL_ENODEV;
    }

    *val = oal_readl((void *)ep_res->st_device_shared_addr_map[type].va);

    return OAL_SUCC;
}

int32_t oal_pcie_set_device_soft_fifo_enable(oal_pcie_linux_res *pst_pci_res)
{
    if (pst_pci_res->comm_res->revision >= PCIE_REVISION_5_00A) {
        return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_SOFT_FIFO_ENABLE, !!g_pcie_soft_fifo_enable);
    } else {
        return OAL_SUCC;
    }
}

int32_t oal_pcie_set_device_ringbuf_bugfix_enable(oal_pcie_linux_res *pst_pci_res)
{
    return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_RINGBUF_BUGFIX, !!g_pcie_ringbuf_bugfix_enable);
}

int32_t oal_pcie_set_device_aspm_dync_disable(oal_pcie_linux_res *pst_pci_res, uint32_t disable)
{
    uint32_t value = 0;
    int32_t ret = oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_ASPM_DYNC_CTL, disable);
    oal_pcie_read_dsm32(pst_pci_res, PCIE_SHARED_ASPM_DYNC_CTL, &value);
    oal_reference(value);
    return ret;
}

int32_t oal_pcie_set_device_dma_check_enable(oal_pcie_linux_res *pst_pci_res)
{
    return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_SOFT_DMA_CHECK, !!g_pcie_dma_data_check_enable);
}

/* 电压拉偏初始化 */
int32_t oal_pcie_voltage_bias_init(oal_pcie_linux_res *pst_pci_res)
{
    if (pst_pci_res->ep_res.chip_info.cb.pcie_voltage_bias_init == NULL) {
        return -OAL_ENODEV;
    }
    return pst_pci_res->ep_res.chip_info.cb.pcie_voltage_bias_init(pst_pci_res);
}

int32_t oal_pcie_copy_to_device_by_dword(oal_pcie_linux_res *pst_pci_res, void *ddr_address, unsigned long start,
                                         uint32_t data_size)
{
    uint64_t i;
    int32_t ret;
    uint32_t value;
    uint32_t length;
    pci_addr_map addr_map;

    length = data_size;

    if (oal_unlikely(((uintptr_t)ddr_address & 0x3) || (data_size & 0x3))) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "ddr address %lu, length 0x%x is invalid", (uintptr_t)ddr_address,
                             length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, start + length - 1, &addr_map);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%x  invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, start, &addr_map);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    for (i = 0; i < length; i += sizeof(uint32_t)) { /* 每次偏移4字节 */
        value = oal_readl((uintptr_t)(ddr_address + i));
        oal_writel(value, (void *)(uintptr_t)(addr_map.va + i));
    }

    return (int32_t)data_size;
}

int32_t oal_pcie_copy_from_device_by_dword(oal_pcie_linux_res *pst_pci_res, void *ddr_address, unsigned long start,
                                           uint32_t data_size)
{
    uint32_t i;
    int32_t ret;
    uint32_t value;
    unsigned long length;
    pci_addr_map addr_map;

    length = (unsigned long)data_size;

    if (oal_unlikely(((uintptr_t)ddr_address & 0x3) || (data_size & 0x3))) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "ddr address %lu, length 0x%lx is invalid", (uintptr_t)ddr_address,
                             length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, start + length - 1, &addr_map);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, start, &addr_map);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    for (i = 0; i < (uint32_t)length; i += sizeof(uint32_t)) { /* 每次偏移4字节 */
        value = oal_readl((void *)(addr_map.va + i));
        oal_writel(value, ddr_address + i);
    }

    return (int32_t)data_size;
}

int32_t oal_pcie_config_l1ss_poweron_time(oal_pci_dev_stru *pst_pci_dev)
{
    int32_t ret;
    int32_t ep_l1ss_pm;
    uint32_t reg_old, reg_new;

    ep_l1ss_pm = oal_pci_find_ext_capability(pst_pci_dev, PCI_EXT_CAP_ID_L1SS);
    if (!ep_l1ss_pm) {
        pci_print_log(PCI_LOG_ERR, "can't get PCI_EXT_CAP_ID_L1SS");
        return -OAL_EFAIL;
    }

    /* ASPM L1sub power_on_time(0x0:0us 0x28:10us 0x61:120us), pcie phy will hold and wait */
    ret = oal_pci_read_config_dword(pst_pci_dev, ep_l1ss_pm + PCI_L1SS_CTL2, &reg_old);
    if (ret) {
        pci_print_log(PCI_LOG_ERR, "read %d failed", ep_l1ss_pm + PCI_L1SS_CTL2);
        return -OAL_EFAIL;
    }

    /* Power On Value bit[7:3] & Scale bit[1:0] */
    reg_new = reg_old & 0xFFFFFF00;

    ret = oal_pci_write_config_dword(pst_pci_dev, ep_l1ss_pm + PCI_L1SS_CTL2, reg_new);
    if (ret) {
        pci_print_log(PCI_LOG_ERR, "write %d failed", ep_l1ss_pm + PCI_L1SS_CTL2);
        return -OAL_EFAIL;
    } else {
        pci_print_log(PCI_LOG_INFO, "write %d ok", ep_l1ss_pm + PCI_L1SS_CTL2);
    }

    pci_print_log(PCI_LOG_INFO, "Aspm T-power on time change from 0x%x to 0x%x", reg_old, reg_new);
    return OAL_SUCC;
}

int32_t oal_pcie_device_phy_config_reg(oal_pcie_linux_res *pst_pci_res, uint32_t pcie_phy_pa, uint32_t pos,
                                       uint32_t bits, uint32_t val)
{
    int32_t ret;
    uint32_t reg;
    pci_addr_map addr_map;

    /* phy addr */
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, pcie_phy_pa, &addr_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", pcie_phy_pa);
        return -OAL_EFAIL;
    }

    pci_print_log(PCI_LOG_INFO, "get dev address 0x%x success", pcie_phy_pa);

    reg = oal_readl(addr_map.va);
    oal_setl_bits((void *)addr_map.va, pos, bits, val);
    pci_print_log(PCI_LOG_INFO, "PCIe phy reg config: change from 0x%x to 0x%x", reg, oal_readl(addr_map.va));

    return OAL_SUCC;
}

OAL_STATIC void oal_pcie_ctrl_base_address_exit(oal_pcie_ep_res *ep_res)
{
    ep_res->pst_pci_ctrl_base = NULL;
    ep_res->pst_pci_dbi_base = NULL;
    ep_res->pst_ete_base = NULL;
}

OAL_STATIC int32_t oal_pcie_ctrl_base_address_init(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;
    pci_addr_map st_map;
    oal_pcie_ep_res *ep_res = &pcie_res->ep_res;

    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->chip_info.addr_info.pcie_ctrl, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", ep_res->chip_info.addr_info.pcie_ctrl);
        oal_pcie_regions_info_dump(pcie_res->comm_res);
        return -OAL_EFAIL;
    }
    ep_res->pst_pci_ctrl_base = (void *)st_map.va;

    if (ep_res->chip_info.edma_support == OAL_TRUE) {
        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->chip_info.addr_info.edma_ctrl, &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", ep_res->chip_info.addr_info.edma_ctrl);
            oal_pcie_regions_info_dump(pcie_res->comm_res);
            oal_pcie_ctrl_base_address_exit(ep_res);
            return -OAL_EFAIL;
        }
    }

    /* dbi base */
    ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->chip_info.addr_info.dbi, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", PCIE_CONFIG_BASE_ADDRESS);
        oal_pcie_regions_info_dump(pcie_res->comm_res);
        oal_pcie_ctrl_base_address_exit(ep_res);
        return -OAL_EFAIL;
    }
    ep_res->pst_pci_dbi_base = (void *)st_map.va;

    if (ep_res->chip_info.ete_support == OAL_TRUE) {
        /* ete base */
        ret = oal_pcie_inbound_ca_to_va(pcie_res->comm_res, ep_res->chip_info.addr_info.ete_ctrl, &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "get dev address 0x%x failed", ep_res->chip_info.addr_info.ete_ctrl);
            oal_pcie_regions_info_dump(pcie_res->comm_res);
            oal_pcie_ctrl_base_address_exit(ep_res);
            return -OAL_EFAIL;
        }
        ep_res->pst_ete_base = (void *)st_map.va;
        ep_res->chip_info.cb.ete_address_init(pcie_res);
    }

    pci_print_log(PCI_LOG_INFO, "ctrl base addr init succ, pci va:0x%p, dbi va:0x%p, ete va:0x%p",
                  ep_res->pst_pci_ctrl_base, ep_res->pst_pci_dbi_base, ep_res->pst_ete_base);
    return OAL_SUCC;
}

/* 配置BAR,IATU等设备资源 */
int32_t oal_pcie_dev_init(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;
    oal_pci_dev_stru *pci_dev = pcie_res->comm_res->pcie_dev;

    if (oal_warn_on(pci_dev == NULL)) {
        return -OAL_ENODEV;
    }

    if (atomic_read(&pcie_res->comm_res->refcnt) <= 1) {
        ret = oal_pcie_bar_init(pcie_res);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    ret = oal_pcie_ctrl_base_address_init(pcie_res);
    if (ret != OAL_SUCC) {
        oal_pcie_bar_exit(pcie_res);
        return ret;
    }

    return OAL_SUCC;
}

void oal_pcie_dev_deinit(oal_pcie_linux_res *pcie_res)
{
    oal_pci_dev_stru *pci_dev = pcie_res->comm_res->pcie_dev;
    if (oal_warn_on(pci_dev == NULL)) {
        return;
    }
    oal_pcie_ctrl_base_address_exit(&pcie_res->ep_res);
    oal_pcie_bar_exit(pcie_res);
}

int32_t oal_pcie_h2d_doorbell(oal_pcie_linux_res *pst_pci_res)
{
    /* 敲铃,host->device ringbuf 有数据更新,2个队列共享一个中断 */
    pst_pci_res->ep_res.stat.h2d_doorbell_cnt++;
    pci_print_log(PCI_LOG_DBG, "oal_pcie_h2d_doorbell,cnt:%u", pst_pci_res->ep_res.stat.h2d_doorbell_cnt);
    if (oal_unlikely(pst_pci_res->comm_res->link_state <= PCI_WLAN_LINK_DOWN)) {
        pci_print_log(PCI_LOG_WARN, "pcie is linkdown");
        return -OAL_ENODEV;
    }
    oal_writel(PCIE_H2D_DOORBELL_TRIGGER_VALUE, pst_pci_res->ep_res.pst_pci_ctrl_base + PCIE_H2D_DOORBELL_OFF);
    return OAL_SUCC;
}

int32_t oal_pcie_ringbuf_h2d_refresh(oal_pcie_linux_res *pst_pci_res)
{
    int32_t ret;
    int32_t i;
    pcie_share_mem_stru st_share_mem;
    pci_addr_map st_map; /* DEVICE CPU地址 */

    oal_pcie_io_trans((uintptr_t)&st_share_mem, pst_pci_res->ep_res.dev_share_mem.va, sizeof(pcie_share_mem_stru));

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, st_share_mem.ringbuf_res_paddr, &st_map);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "invalid ringbuf device address 0x%x, map failed\n", st_share_mem.ringbuf_res_paddr);
        oal_print_hex_dump((uint8_t *)&st_share_mem, sizeof(st_share_mem), HEX_DUMP_GROUP_SIZE, "st_share_mem: ");
        return -OAL_ENOMEM;
    }

    /* h->h */
    memcpy_s(&pst_pci_res->ep_res.st_ringbuf_map, sizeof(pci_addr_map), &st_map, sizeof(pci_addr_map));

    /* device的ringbuf管理结构同步到Host */
    /* 这里重新刷新h2d ringbuf 指针 */
    oal_pcie_io_trans((uintptr_t)&pst_pci_res->ep_res.st_ringbuf, (uintptr_t)pst_pci_res->ep_res.st_ringbuf_map.va,
                      sizeof(pst_pci_res->ep_res.st_ringbuf));

    /* 初始化RX BUFF */
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        unsigned long offset;
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res->comm_res, pst_pci_res->ep_res.st_ringbuf.st_h2d_buf[i].base_addr,
                                        &st_map);
        if (ret != OAL_SUCC) {
            pci_print_log(PCI_LOG_ERR, "invalid d2h ringbuf[%d] base address 0x%llx, map failed, ret=%d\n", i,
                          pst_pci_res->ep_res.st_ringbuf.st_h2d_buf[i].base_addr, ret);
            return -OAL_ENOMEM;
        }
        memcpy_s(&pst_pci_res->ep_res.st_tx_res[i].ringbuf_data_dma_addr, sizeof(pci_addr_map), &st_map,
                 sizeof(pci_addr_map));
        offset =
            ((uintptr_t)&pst_pci_res->ep_res.st_ringbuf.st_h2d_buf[i]) - ((uintptr_t)&pst_pci_res->ep_res.st_ringbuf);
        pst_pci_res->ep_res.st_tx_res[i].ringbuf_ctrl_dma_addr.pa = pst_pci_res->ep_res.st_ringbuf_map.pa + offset;
        pst_pci_res->ep_res.st_tx_res[i].ringbuf_ctrl_dma_addr.va = pst_pci_res->ep_res.st_ringbuf_map.va + offset;
    }

    return OAL_SUCC;
}

OAL_STATIC int32_t oal_pcie_sysfs_init(oal_pcie_linux_res *pcie_res)
{
    int32_t ret;
    oal_kobject *pst_root_object = NULL;

    if (g_conn_syfs_pci_object != NULL) {
        return OAL_SUCC;
    }

    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        pci_print_log(PCI_LOG_ERR, "[E]get pci root sysfs object failed!\n");
        return -OAL_EFAIL;
    }

    g_conn_syfs_pci_object = kobject_create_and_add("pci", pst_root_object);
    if (g_conn_syfs_pci_object == NULL) {
        ret = -OAL_ENODEV;
        pci_print_log(PCI_LOG_ERR, "sysfs create kobject_create_and_add pci fail\n");
        goto fail_g_conn_syfs_pci_object;
    }

    ret = oal_pcie_sysfs_group_create(g_conn_syfs_pci_object);
    if (ret) {
        ret = -OAL_ENOMEM;
        pci_print_log(PCI_LOG_ERR, "sysfs create g_hpci_attribute_group group fail.ret=%d\n", ret);
        goto fail_create_pci_group;
    }

    return OAL_SUCC;
fail_create_pci_group:
    kobject_put(g_conn_syfs_pci_object);
    g_conn_syfs_pci_object = NULL;
fail_g_conn_syfs_pci_object:
    return ret;
}

OAL_STATIC void oal_pcie_sysfs_exit(oal_pcie_linux_res *pst_pcie_res)
{
    if (g_conn_syfs_pci_object == NULL) {
        return;
    }
    oal_pcie_sysfs_group_remove(g_conn_syfs_pci_object);
    kobject_put(g_conn_syfs_pci_object);
    g_conn_syfs_pci_object = NULL;
}

int32_t oal_pcie_host_init(oal_pcie_linux_res *pcie_res)
{
    int32_t i, ret;

    if (oal_netbuf_cb_size() < sizeof(pcie_cb_dma_res) + sizeof(struct hcc_tx_cb_stru)) {
        pci_print_log(PCI_LOG_ERR, "pcie cb is too large,[cb %lu < pcie cb %lu + hcc cb %lu]\n",
                      (unsigned long)oal_netbuf_cb_size(), (unsigned long)sizeof(pcie_cb_dma_res),
                      (unsigned long)sizeof(struct hcc_tx_cb_stru));
        return -OAL_ENOMEM;
    }

    /* 初始化tx/rx队列 */
    oal_spin_lock_init(&pcie_res->ep_res.st_rx_res.lock);
    oal_netbuf_list_head_init(&pcie_res->ep_res.st_rx_res.rxq);
    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        oal_atomic_set(&pcie_res->ep_res.st_tx_res[i].tx_ringbuf_sync_cond, 0);
        oal_spin_lock_init(&pcie_res->ep_res.st_tx_res[i].lock);
        oal_netbuf_list_head_init(&pcie_res->ep_res.st_tx_res[i].txq);
    }

    oal_spin_lock_init(&pcie_res->ep_res.st_message_res.d2h_res.lock);
    oal_spin_lock_init(&pcie_res->ep_res.st_message_res.h2d_res.lock);

    ret = oal_pcie_sysfs_init(pcie_res);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "oal_pcie_sysfs_init failed, ret=%d\n", ret);
        return ret;
    }

    ret = oal_pcie_chip_info_init(pcie_res);
    if (ret != OAL_SUCC) {
        oal_pcie_sysfs_exit(pcie_res);
        return ret;
    }

    mutex_init(&pcie_res->ep_res.st_rx_mem_lock);

    oal_ete_host_init(pcie_res);

    if (atomic_read(&pcie_res->comm_res->refcnt) <= 1) {
        oal_pcie_change_link_state(pcie_res, PCI_WLAN_LINK_UP);
    }

    return ret;
}

void oal_pcie_host_exit(oal_pcie_linux_res *pcie_res)
{
    oal_ete_host_exit(pcie_res);
    mutex_destroy(&pcie_res->ep_res.st_rx_mem_lock);
    oal_pcie_sysfs_exit(pcie_res);
}
