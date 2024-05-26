

#ifndef __PCIE_HOST_116X_H__
#define __PCIE_HOST_116X_H__

#include "pcie_linux.h"
#include "oal_types.h"
#include "oal_hardware.h"
#include "oal_net.h"
#include "pcie_comm.h"

#define PCIE_RX_TRANS_FLAG 0xbeafbeaf
#define PCIE_DEBUG_MSG_LEN 100
#define PCIE_TRANS_US_OFFSET_BITS 17 /* ������ģ�������Ҫ */
/* 64bit bar, bar1 reference to  bar 2 */
#define PCIE_IATU_BAR_INDEX OAL_PCI_BAR_2
#define PCIE_RX_RINGBUF_SUPPLY_ALL 0xFFFFFFFFUL

typedef enum _pci_log_type_ {
    PCI_LOG_ERR,
    PCI_LOG_WARN,
    PCI_LOG_INFO,
    PCI_LOG_DBG,
    PCI_LOG_BUTT
} pci_log_type;

/*
 * when deepsleep not S/R, pcie is PCI_WLAN_LINK_UP,
 * when deepsleep under S/R, pcie is PCI_WLAN_LINK_DOWN,
 * when down firmware , pcie is PCI_WLAN_LINK_MEM_UP,
 * after wcpu main func up device ready, pcie is PCI_WLAN_LINK_DMA_UP,
 * we can't access pcie ep's AXI interface when it's  power down,
 * cased host bus error
 */
#define PCIE_GEN1 0x0
#define PCIE_GEN2 0x1

extern char *g_pcie_link_state_str[PCI_WLAN_LINK_BUTT + 1];
extern int32_t g_hipcie_loglevel;
extern char *g_pci_loglevel_format[];
#ifdef _PRE_PLAT_FEATURE_HI116X_PCIE
extern int32_t g_pcie_memcopy_type;
#endif
extern int32_t g_pcie_ringbuf_bugfix_enable;
extern int32_t g_pcie_dma_data_check_enable;
extern int32_t g_pcie_dma_data_rx_hdr_init;

#define pci_dbg_condtion() (oal_unlikely(g_hipcie_loglevel >= PCI_LOG_DBG))
OAL_STATIC OAL_INLINE void oal_pcie_log_record(pci_log_type type)
{
    if (oal_unlikely(type <= PCI_LOG_WARN)) {
        if (type == PCI_LOG_ERR) {
            declare_dft_trace_key_info("pcie error happend", OAL_DFT_TRACE_OTHER);
        }
        if (type == PCI_LOG_WARN) {
            declare_dft_trace_key_info("pcie warn happend", OAL_DFT_TRACE_OTHER);
        }
    }
}
#ifdef CONFIG_PRINTK
#define pci_print_log(loglevel, fmt, arg...)                                                                    \
    do {                                                                                                        \
        if (oal_unlikely(g_hipcie_loglevel >= loglevel)) {                                                        \
            printk("%s" fmt "[%s:%d]\n", g_pci_loglevel_format[loglevel] ? : "", ##arg, __FUNCTION__, __LINE__); \
            oal_pcie_log_record(loglevel);                                                                      \
        }                                                                                                       \
    } while (0)
#else
#define pci_print_log
#endif

OAL_STATIC OAL_INLINE void oal_pcie_print_config_reg(oal_pci_dev_stru *dev, int32_t reg_name, char *name)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    int32_t ret_t;
    uint32_t reg_t = 0;
    ret_t = oal_pci_read_config_dword(dev, reg_name, &reg_t);
    if (!ret_t) {
        oal_io_print(" [0x%8x:0x%8x]\n", reg_name, reg_t);
    } else {
        oal_io_print("read %s register failed, ret=%d\n", name, ret_t);
    }
#endif
}

#define print_pcie_config_reg(dev, reg_name)                 \
    do {                                                     \
        oal_pcie_print_config_reg(dev, reg_name, #reg_name); \
    } while (0)

typedef struct _oal_reg_bits_stru_ {
    uint32_t flag;
    uint32_t value;
    char *name;
} oal_reg_bits_stru;

typedef struct _oal_pcie_msi_stru_ {
    int32_t is_msi_support;
    oal_irq_handler_t *func; /* msi interrupt map */
    int32_t msi_num;       /* msi number */
} oal_pcie_msi_stru;


typedef int32_t (*pcie_chip_func)(oal_pcie_linux_res *, int32_t);
typedef struct _pcie_chip_id_stru_ {
    int32_t device_id;
    pcie_chip_func func;
} pcie_chip_id_stru;

/* function declare */
void oal_pcie_tx_netbuf_free(oal_pcie_linux_res *pcie_res, oal_netbuf_stru *netbuf);
int32_t oal_pcie_send_netbuf_list(oal_pcie_linux_res *pci_res, oal_netbuf_head_stru *head,
                                  pcie_h2d_ringbuf_qtype qtype);
int32_t oal_pcie_host_init(oal_pcie_linux_res *pcie_res);
void oal_pcie_host_exit(oal_pcie_linux_res *pci_res);
int32_t oal_pcie_dev_init(oal_pcie_linux_res *pci_res);
void oal_pcie_dev_deinit(oal_pcie_linux_res *pci_res);
int32_t oal_pcie_set_outbound_iatu_by_membar(void* pst_bar1_vaddr, uint32_t index, uint64_t src_addr,
                                             uint64_t dst_addr, uint64_t limit_size);
int32_t oal_pcie_set_outbound_by_membar(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_rx_netbuf_submit(oal_pcie_linux_res *pci_res, oal_netbuf_stru *netbuf);
void oal_pcie_release_rx_netbuf(oal_pcie_linux_res *pci_res, oal_netbuf_stru *netbuf);
int32_t oal_pcie_vaddr_isvalid(oal_pcie_comm_res *comm_res, const void *vaddr);
int32_t oal_pcie_inbound_ca_to_va(oal_pcie_comm_res *comm_res, uint64_t dev_cpuaddr,
                                  pci_addr_map *addr_map);
int32_t oal_pcie_inbound_va_to_ca(oal_pcie_comm_res *comm_res, uint64_t host_va, uint64_t *dev_cpuaddr);
int32_t oal_pcie_transfer_done(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_edma_tx_is_idle(oal_pcie_linux_res *pst_pci_res, pcie_h2d_ringbuf_qtype qtype);
int32_t oal_pcie_read_d2h_message(oal_pcie_linux_res *pst_pci_res, uint32_t *message);
int32_t oal_pcie_send_message_to_dev(oal_pcie_linux_res *pst_pci_res, uint32_t message);
int32_t oal_pcie_get_host_trans_count(oal_pcie_linux_res *pst_pci_res, uint64_t *tx, uint64_t *rx);
int32_t oal_pcie_edma_sleep_request_host_check(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_disable_regions(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_enable_regions(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_transfer_res_init(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_transfer_res_exit(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_iatu_init(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_check_link_state(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_set_l1pm_ctrl(oal_pcie_linux_res *pst_pci_res, int32_t enable);
int32_t oal_pcie_set_device_soft_fifo_enable(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_set_device_dma_check_enable(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_set_device_ringbuf_bugfix_enable(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_device_aspm_init(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_device_auxclk_init(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_copy_from_device_by_dword(oal_pcie_linux_res *pst_pci_res,
                                           void *ddr_address,
                                           uintptr_t start,
                                           uint32_t data_size);
int32_t oal_pcie_copy_to_device_by_dword(oal_pcie_linux_res *pst_pci_res,
                                         void *ddr_address,
                                         uintptr_t start,
                                         uint32_t data_size);
uintptr_t oal_pcie_slt_get_deivce_ram_cpuaddr(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_device_changeto_high_cpufreq(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_device_mem_scanall(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_get_gen_mode(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_set_voltage_bias_param(int32_t phy_0v9_bias, int32_t phy_1v8_bias);
int32_t oal_pcie_get_vol_reg_1v8_value(int32_t request_vol, uint32_t *pst_value);
int32_t oal_pcie_get_vol_reg_0v9_value(int32_t request_vol, uint32_t *pst_value);
int32_t oal_pcie_voltage_bias_init(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_print_transfer_info(oal_pcie_linux_res *pst_pci_res, uint64_t print_flag);
void oal_pcie_reset_transfer_info(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_host_pending_signal_check(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_host_pending_signal_process(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_device_phy_config(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_set_device_aspm_dync_disable(oal_pcie_linux_res *pst_pci_res, uint32_t disable);
void oal_pcie_device_xfer_pending_sig_clr(oal_pcie_linux_res *pst_pci_res, oal_bool_enum clear);
int32_t oal_pcie_share_mem_res_map(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_share_mem_res_unmap(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_ringbuf_res_unmap(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_ringbuf_res_map(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_ringbuf_h2d_refresh(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_host_slave_address_switch(oal_pcie_linux_res *pst_pci_res, uint64_t src_addr,
                                           uint64_t* dst_addr, int32_t is_host_iova);
int32_t oal_pcie_sysfs_group_create(oal_kobject *root_obj);
void oal_pcie_sysfs_group_remove(oal_kobject *root_obj);
void oal_pcie_voltage_bias_param_init(void);

int32_t oal_pcie_h2d_doorbell(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_d2h_doorbell(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_get_ca_by_pa(oal_pcie_comm_res *comm_res, uintptr_t paddr, uint64_t *cpuaddr);

#if defined(PLATFORM_DEBUG_ENABLE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#define ETE_MEMCPY_TEST_STR "ete_memcpy_test"
int32_t oal_pcie_testcase_ete_memcpy_test(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                          const char *buf);
#endif

uint32_t oal_pcie_ringbuf_freecount(pcie_ringbuf *pst_ringbuf);
hcc_bus *oal_pcie_get_bus(uint32_t dev_id);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* PCIE EP Slave �ڿ����ĵ�ַת�� --Ƭ������ͨ��PCIE������оƬDDR�ռ�
 * PCIE EP Slave �ڷ���, Ƭ��IP ����HOST DDR
 * ��ַת����оƬ��DDR�豸��ַת����device��SLAVE�ڵ�ַ
 *  ע�������DDR�豸��ַ(iova)����ֱ�ӵ������ַ�����Ƕ�ӦKernel��DMA��ַ
 *  �豸��ַ����ͨ��phys_to_virt/virt_to_physֱ��ת��
 */
int32_t pcie_if_hostca_to_devva(uint32_t ul_chip_id, uint64_t host_iova, uint64_t* addr);
int32_t pcie_if_devva_to_hostca(uint32_t ul_chip_id, uint64_t devva, uint64_t* addr);
oal_bool_enum_uint8 oal_pcie_link_state_up(uint8_t dev_id);
/* PCIE EP Master�ڿ����ĵ�ַת�� --��оƬ����(CPU)ͨ��PCIE����Ƭ��IP(RAM+�Ĵ���) */
int32_t oal_pcie_devca_to_hostva(uint32_t ul_chip_id, uint64_t dev_cpuaddr, uint64_t *host_va);
int32_t oal_pcie_get_dev_ca(uint32_t ul_chip_id, void *host_va, uint64_t* dev_cpuaddr);
#else
OAL_STATIC OAL_INLINE int32_t pcie_if_hostca_to_devva(uint32_t ul_chip_id, uint64_t host_iova, uint64_t* addr)
{
    *addr = host_iova;

    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE int32_t pcie_if_devva_to_hostca(uint32_t ul_chip_id, uint64_t devva, uint64_t* addr)
{
    *addr = devva;

    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 oal_pcie_link_state_up(uint8_t dev_id)
{
    oal_reference(dev_id);
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE int32_t oal_pcie_devca_to_hostva(uint32_t ul_chip_id, uint64_t dev_cpuaddr, uint64_t *host_va)
{
    *host_va = dev_cpuaddr;

    return OAL_SUCC;
}
#endif

int32_t oal_ete_host_init(oal_pcie_linux_res *pcie_res);
void  oal_ete_host_exit(oal_pcie_linux_res *pcie_res);
int32_t oal_pcie_device_phy_config_reg(oal_pcie_linux_res *pst_pci_res, uint32_t pcie_phy_pa,
                                       uint32_t pos, uint32_t bits, uint32_t val);
int32_t oal_pcie_config_l1ss_poweron_time(oal_pci_dev_stru *pst_pci_dev);
void oal_ete_print_transfer_info(oal_pcie_linux_res *pst_pci_res, uint64_t print_flag);
void oal_ete_reset_transfer_info(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_transfer_ete_done(oal_pcie_linux_res *pcie_res);
int32_t oal_ete_send_netbuf_list(oal_pcie_linux_res *pst_pci_res, oal_netbuf_head_stru *pst_head,
                                 hcc_netbuf_queue_type qtype);
int32_t oal_ete_ip_init(oal_pcie_linux_res *pst_pci_res);
void  oal_ete_ip_exit(oal_pcie_linux_res *pst_pci_res);
int32_t oal_ete_transfer_res_init(oal_pcie_linux_res *pst_pci_res);
void oal_ete_transfer_res_exit(oal_pcie_linux_res *pst_pci_res);
void oal_ete_transfer_deepsleep_clean(oal_pcie_linux_res *pst_pci_res);
int32_t oal_ete_transfer_deepwkup_restore(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_ete_tx_is_idle(oal_pcie_linux_res *pst_pci_res, hcc_netbuf_queue_type qtype);
void oal_ete_sched_rx_threads(oal_pcie_linux_res *pst_pci_res);
int32_t oal_ete_sleep_request_host_check(oal_pcie_linux_res *pst_pci_res);
int32_t oal_ete_host_pending_signal_check(oal_pcie_linux_res *pst_pci_res);
int32_t oal_ete_host_pending_signal_process(oal_pcie_linux_res *pst_pci_res);
void oal_ete_rx_res_clean(oal_pcie_linux_res *pst_pci_res);
void oal_ete_tx_res_clean(oal_pcie_linux_res *pst_pci_res, uint32_t force_clean);
int32_t oal_ete_rx_ringbuf_build(oal_pcie_linux_res *pcie_res);
int32_t oal_pcie_h2d_int(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_d2h_int(oal_pcie_linux_res *pst_pci_res);
int32_t oal_pcie_set_outbound_membar(oal_pcie_linux_res *pcie_res, oal_pcie_iatu_bar* pst_iatu_bar);
int32_t oal_pcie_bar1_init(oal_pcie_iatu_bar* pst_iatu_bar);
void oal_pcie_bar1_exit(oal_pcie_iatu_bar *pst_iatu_bar);
int32_t oal_pcie_print_device_mem(oal_pcie_linux_res *pst_pcie_res, uint32_t cpu_address, uint32_t length);
int32_t oal_pcie_device_check_alive(oal_pcie_linux_res *pst_pci_res);
void oal_pcie_regions_info_dump(oal_pcie_comm_res *comm_res);
int32_t oal_pcie_write_dsm32(oal_pcie_linux_res *pcie_res, pcie_shared_device_addr_type type, uint32_t val);
int32_t oal_pcie_read_dsm32(oal_pcie_linux_res *pcie_res, pcie_shared_device_addr_type type, uint32_t *val);
int32_t oal_pcie_print_device_transfer_info(oal_pcie_linux_res *pst_pcie_res);
int32_t oal_pcie_get_bar_region_info(oal_pcie_linux_res *pcie_res,
                                     oal_pcie_region **region_base, uint32_t *region_num);
void oal_pcie_print_debug_usages(void);
void oal_pcie_print_ringbuf_info(pcie_ringbuf *pst_ringbuf, pci_log_type level);
#ifdef _PRE_PLAT_FEATURE_HI110X_DUAL_PCIE
int32_t oal_pcie_dual_bar_init(oal_pcie_linux_res *pst_pci_res, oal_pci_dev_stru *pst_pci_dev);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* Inline functions */
OAL_STATIC OAL_INLINE void oal_pci_cache_flush(oal_pci_dev_stru *hwdev, dma_addr_t pa, int32_t size)
{
    if (oal_likely(size > 0)) {
        dma_sync_single_for_device(&hwdev->dev, pa, (size_t)size, PCI_DMA_TODEVICE);
    } else {
        oal_warn_on(1);
    }
}

/*
 * �� �� ��  : oal_pci_cache_flush
 * ��������  : ��Ч��cache
 */
OAL_STATIC OAL_INLINE void oal_pci_cache_inv(oal_pci_dev_stru *hwdev, dma_addr_t pa, int32_t size)
{
    if (oal_likely(size > 0)) {
        dma_sync_single_for_cpu(&hwdev->dev, pa, (size_t)size, PCI_DMA_FROMDEVICE);
    } else {
        oal_warn_on(1);
    }
}
#endif

/* 'offset' is a backplane address */
OAL_STATIC OAL_INLINE void oal_pcie_write_mem8(uintptr_t va, uint8_t data)
{
    *(volatile uint8_t *)(va) = (uint8_t)data;
}

OAL_STATIC OAL_INLINE uint8_t oal_pcie_read_mem8(uintptr_t va)
{
    volatile uint8_t data;

    data = *(volatile uint8_t *)(va);

    return data;
}

OAL_STATIC OAL_INLINE void oal_pcie_write_mem32(uintptr_t va, uint32_t data)
{
    *(volatile uint32_t *)(va) = (uint32_t)data;
}

OAL_STATIC OAL_INLINE void oal_pcie_write_mem16(uintptr_t va, uint16_t data)
{
    *(volatile uint16_t *)(va) = (uint16_t)data;
}

OAL_STATIC OAL_INLINE void oal_pcie_write_mem64(uintptr_t va, uint64_t data)
{
    *(volatile uint64_t *)(va) = (uint64_t)data;
}

OAL_STATIC OAL_INLINE uint16_t oal_pcie_read_mem16(uintptr_t va)
{
    volatile uint16_t data;

    data = *(volatile uint16_t *)(va);

    return data;
}

OAL_STATIC OAL_INLINE uint32_t oal_pcie_read_mem32(uintptr_t va)
{
    volatile uint32_t data;

    data = *(volatile uint32_t *)(va);

    return data;
}

OAL_STATIC OAL_INLINE uint64_t oal_pcie_read_mem64(uintptr_t va)
{
    volatile uint64_t data;

    data = *(volatile uint64_t *)(va);

    return data;
}

extern void oal_pcie_io_trans64(void *dst, void *src, int32_t size);
extern void oal_pcie_io_trans32(uint32_t *dst, uint32_t *src, int32_t size);
extern int32_t g_pcie_memcopy_type;
/* dst/src ��һ�˵�ַ��PCIE EP�࣬PCIE��burst��ʽ���� */
OAL_STATIC OAL_INLINE void oal_pcie_io_trans(uintptr_t dst, uintptr_t src, uint32_t size)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* ���簲ȫ�������� */
    if ((g_pcie_memcopy_type == 0) || (g_pcie_memcopy_type == 1)) {
        if (WARN((dst & 0x3), "invalid dst address 0x%lx", dst) ||
            WARN((src & 0x3), "invalid src address 0x%lx", src) ||
            WARN((size & 0x3), "invalid size address 0x%x", size)) {
            return;
        }
#ifdef CONFIG_64BIT
        oal_pcie_io_trans64((void *)dst, (void *)src, (int32_t)size);
#else
        oal_pcie_io_trans32((uint32_t *)dst, (uint32_t *)src, (int32_t)size);
#endif
    } else if (g_pcie_memcopy_type == 2) { /* ����2, 4�ֽڶ�д */
        uint32_t i;
        uint32_t value;
        /* �4�ֽڶ������, Test Code ��ʱ������ ���ֽڣ�˫�ֽ� */
        if (WARN((dst & 0x3), "invalid dst address 0x%lx", dst) ||
            WARN((src & 0x3), "invalid src address 0x%lx", src) ||
            WARN((size & 0x3), "invalid size address 0x%x", size)) {
            return;
        }

        for (i = 0; i < size; i += sizeof(uint32_t)) { /* ÿ��ƫ��4�ֽ� */
            value = oal_readl((void *)(src + i));
            oal_writel(value, (void *)(dst + i));
        }
    } else if (g_pcie_memcopy_type == 3) { /* ����3���ڴ��������д */
        if (WARN((dst & 0x3), "invalid dst address 0x%lx", dst) ||
            WARN((src & 0x3), "invalid src address 0x%lx", src) ||
            WARN((size & 0x3), "invalid size address 0x%x", size)) {
            return;
        }

        oal_pcie_io_trans32((uint32_t *)dst, (uint32_t *)src, (int32_t)size);
    }
#endif
}

int32_t pcie_ete_rx_cnt_host_ddr_mode_res_init(oal_pcie_linux_res *pcie_res);
int32_t pcie_ete_rx_cnt_host_ddr_mode_enable(uintptr_t base_address, uint32_t enable);
int32_t pcie_ete_rx_cnt_host_ddr_mode_res_release(oal_pcie_linux_res *pcie_res);

OAL_STATIC OAL_INLINE uint32_t pcie_ringbuf_len(pcie_ringbuf *pst_ringbuf)
{
    /* �޷��ţ��Ѿ������˷�ת */
    uint32_t len = (pst_ringbuf->wr - pst_ringbuf->rd);
    if (len == 0) {
        return 0;
    }
#ifdef _PRE_PLAT_FEATURE_PCIE_DEBUG
    if (len % pst_ringbuf->item_len) {
        oal_io_print("pcie_ringbuf_len, size:%u, wr:%u, rd:%u" NEWLINE,
                     pst_ringbuf->size,
                     pst_ringbuf->wr,
                     pst_ringbuf->rd);
    }
#endif
    if (pst_ringbuf->item_mask) {
        /* item len �����2��N���ݣ�����λ */
        len = len >> pst_ringbuf->item_mask;
    } else {
        len /= pst_ringbuf->item_len;
    }
    return len;
}

/* ��ӡ */
OAL_STATIC OAL_INLINE void oal_pcie_print_bits(void *data, uint32_t size)
{
#ifdef CONFIG_PRINTK
    int32_t ret;
    uint32_t value;
    char buf[32 * 3 + 1]; /* ��bit��ӡ������ӡ32bit��ÿ��bit3���ַ� */
    int32_t i;
    int32_t count = 0;

    if (size == 1) { /* 1��ʾҪ��ӡ����Ϊ1�ֽ� */
        value = (uint32_t) * (uint8_t *)data;
        oal_io_print("value= 0x%2x, =%u (dec) \n", value, value);
        oal_io_print("07 06 05 04 03 02 01 00\n");
    } else if (size == 2) { /* 2��ʾҪ��ӡ����Ϊ2�ֽ� */
        value = (uint32_t) * (uint16_t *)data;
        oal_io_print("value= 0x%4x, =%u (dec) \n", value, value);
        oal_io_print("15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00\n");
    } else if (size == 4) { /* 4��ʾҪ��ӡ����Ϊ4�ֽ� */
        value = (uint32_t) * (uint32_t *)data;
        oal_io_print("value= 0x%8x, =%u (dec) \n", value, value);
        oal_io_print("31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 "
            "15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00\n");
    } else {
        /* �����˴���Ĳ������������� */
        return;
    }

    /* �����ش�ӡ */
    for (i = size * 8 - 1; i >= 0; i--) { // 8 bits, 1 bit each time
        ret = snprintf_s(buf + count, sizeof(buf) - count, sizeof(buf) - count - 1, "%s",
                         (1u << (uint32_t)i) & value ? " 1 " : " 0 ");
        if (ret < 0) {
            pci_print_log(PCI_LOG_ERR, "log str format err line[%d]\n", __LINE__);
            return;
        }
        count += ret;
    }
    oal_io_print("%s\n", buf);
#else
    oal_reference(data);
    oal_reference(size);
#endif
}

OAL_STATIC OAL_INLINE oal_netbuf_stru *oal_pcie_rx_netbuf_alloc(uint32_t ul_size, int32_t gflag)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    return __netdev_alloc_skb(NULL, ul_size, gflag);
#else
    oal_reference(gflag);
    return oal_netbuf_alloc(ul_size, 0, 0);
#endif
}

OAL_STATIC OAL_INLINE void oal_pcie_shced_rx_hi_thread(oal_pcie_linux_res *pst_pci_res)
{
    oal_atomic_set(&pst_pci_res->ep_res.rx_hi_cond, 1);
    oal_wait_queue_wake_up_interrupt(&pst_pci_res->ep_res.st_rx_hi_wq);
}

OAL_STATIC OAL_INLINE void oal_pcie_shced_rx_normal_thread(oal_pcie_linux_res *pst_pci_res)
{
    oal_atomic_set(&pst_pci_res->ep_res.rx_normal_cond, 1);
    oal_wait_queue_wake_up_interrupt(&pst_pci_res->ep_res.st_rx_normal_wq);
}

OAL_STATIC OAL_INLINE void oal_pcie_print_config_reg_bar(oal_pcie_linux_res *pst_pci_res, uint32_t offset, char *name)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    uint32_t reg;
    reg = oal_readl(pst_pci_res->comm_res->st_iatu_bar.st_region.vaddr + offset);
    oal_io_print("%-50s [0x%8x:0x%8x]\n", name, offset, reg);
#else
    oal_reference(pst_pci_res);
    oal_reference(offset);
    oal_reference(name);
#endif
}
#ifndef _PRE_LINUX_TEST
OAL_STATIC OAL_INLINE char *oal_pcie_get_link_state_str(pci_wlan_link_state link_state)
{
    if (oal_warn_on(link_state > PCI_WLAN_LINK_BUTT)) {
        pci_print_log(PCI_LOG_WARN, "invalid link_state:%d", link_state);
        return "overrun";
    }

    if (g_pcie_link_state_str[link_state] == NULL) {
        return "unkown";
    }

    return g_pcie_link_state_str[link_state];
}

OAL_STATIC OAL_INLINE void oal_pcie_change_link_state(oal_pcie_linux_res *pcie_res, pci_wlan_link_state link_state_new)
{
    if (pcie_res->comm_res->link_state != link_state_new) {
        pci_print_log(PCI_LOG_INFO, "link_state change from %s to %s",
                      oal_pcie_get_link_state_str(pcie_res->comm_res->link_state),
                      oal_pcie_get_link_state_str(link_state_new));
    } else {
        pci_print_log(PCI_LOG_INFO, "link_state still %s", oal_pcie_get_link_state_str(link_state_new));
    }

    pcie_res->comm_res->link_state = link_state_new;
}
#endif
#endif
