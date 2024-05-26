

#ifndef __PCIE_LINUX_116X_H__
#define __PCIE_LINUX_116X_H__

#include "oal_util.h"
#include "oal_hcc_bus.h"
#include "oal_schedule.h"
#include "ete_host.h"
#include "external/pcie_interface.h"

#define OAL_PCIE_MIN_MPS 128

#define OAL_CONTINUOUS_LINK_UP_SUCC_TIMES 3

typedef enum _pcie_ep_ip_status_ {
    PCIE_EP_IP_POWER_DOWN = 0,
    PCIE_EP_IP_POWER_UP
} pcie_ep_ip_status;

typedef enum _pci_wlan_link_state_ {
    PCI_WLAN_LINK_DOWN = 0,  /* default state, PCIe not ready */
    PCI_WLAN_LINK_DEEPSLEEP, /* pcie linkdown, but soc sleep mode */
    PCI_WLAN_LINK_UP,        /* 物理链路已使能 */
    PCI_WLAN_LINK_MEM_UP,    /* IATU已经配置OK，可以访问AXI */
    PCI_WLAN_LINK_RES_UP,    /* RINGBUF OK */
    PCI_WLAN_LINK_WORK_UP,   /* 业务层可以访问PCIE */
    PCI_WLAN_LINK_BUTT
} pci_wlan_link_state;

#define  PCIE_HISI_VENDOR_ID             (0x19e5)
#define  PCIE_HISI_VENDOR_ID_0           (0x19e7)
#define  PCIE_HISI_DEVICE_ID_HI1103      (0x1103)
#define  PCIE_HISI_DEVICE_ID_HI1105      (0x1105)
#define  PCIE_HISI_DEVICE_ID_SHENKUOFPGA (0x1106)
#define  PCIE_HISI_DEVICE_ID_SHENKUO     (0x0006)
#define  PCIE_HISI_DEVICE_ID_BISHENG     (0xb001)
#define  PCIE_HISI_DEVICE_ID_HI1161      (0xb003)

#define PCIE_DUAL_PCI_MASTER_FLAG  0x0000
#define PCIE_DUAL_PCI_SLAVE_FLAG   0x0001

#define PCIE_POWER_ON                1
#define PCIE_POWER_OFF_WITH_L3MSG    0 /* power off with L3 message */
#define PCIE_POWER_OFF_WITHOUT_L3MSG 2 /* power off without L3 message */

typedef struct _pcie_callback_ {
    void (*pf_func)(void *para);
    void *para;
} pcie_callback_stru;

typedef struct _pcie_wlan_callback_group_ {
    pcie_callback_stru pcie_mac_2g_isr_callback;
    pcie_callback_stru pcie_mac_5g_isr_callback;
    pcie_callback_stru pcie_phy_2g_isr_callback;
    pcie_callback_stru pcie_phy_5g_isr_callback;
} pcie_wlan_callback_group_stru;

typedef struct _oal_pcie_bindcpu_cfg_ {
    uint8_t is_bind;    /* 自动绑核指令,需保证存放在最低位 */
    uint8_t irq_cpu;    /* 用户绑定硬中断命令 */
    uint8_t thread_cmd; /* 用户绑定线程命令 */
    oal_bool_enum_uint8 is_userctl; /* 是否根据用户命令绑核 */
}oal_pcie_bindcpu_cfg;

#ifdef _PRE_HISI_BINDCPU
typedef struct _cpu_cap_mask {
    struct cpumask fast_cpus;
    struct cpumask slow_cpus;
}cpu_cap_mask;
#endif
typedef struct _oal_pcie_h2d_stat_ {
    uint32_t tx_count;
    uint32_t tx_done_count;
    uint32_t tx_burst_cnt[PCIE_EDMA_READ_BUSRT_COUNT + 1];
} oal_pcie_h2d_stat;

typedef struct _oal_pcie_d2h_stat_ {
    uint32_t rx_count;
    uint32_t rx_done_count;
    uint32_t rx_burst_cnt[PCIE_EDMA_WRITE_BUSRT_COUNT + 1];
    uint32_t alloc_netbuf_failed;
    uint32_t map_netbuf_failed;
} oal_pcie_d2h_stat;

typedef struct _oal_pcie_trans_stat_ {
    uint32_t intx_total_count;
    uint32_t intx_tx_count;
    uint32_t intx_rx_count;
    uint32_t done_err_cnt;
    uint32_t h2d_doorbell_cnt;
    uint32_t d2h_doorbell_cnt;
} oal_pcie_trans_stat;

typedef struct _pci_addr_map__ {
    /* unsigned long 指针长度和CPU位宽等长 */
    uintptr_t va; /* 虚拟地址 */
    uintptr_t pa; /* 物理地址 */
} pci_addr_map;

typedef struct _pcie_cb_dma_res_ {
    edma_paddr_t paddr;
    uint32_t len;
} pcie_cb_dma_res;

typedef struct _pcie_h2d_res_ {
    /* device ringbuf 虚拟地址(数据) */
    pci_addr_map ringbuf_data_dma_addr; /* ringbuf buf地址 */
    pci_addr_map ringbuf_ctrl_dma_addr; /* ringbuf 控制结构体地址 */
    oal_netbuf_head_stru txq;           /* 正在发送中的netbuf队列 */
    oal_atomic tx_ringbuf_sync_cond;
    oal_spin_lock_stru lock;
    oal_pcie_h2d_stat stat;
} pcie_h2d_res;

typedef struct _pcie_d2h_res_ {
    /* device ringbuf 虚拟地址(数据) */
    pci_addr_map ringbuf_data_dma_addr; /* ringbuf buf地址 */
    pci_addr_map ringbuf_ctrl_dma_addr; /* ringbuf 控制结构体地址 */
    oal_netbuf_head_stru rxq;           /* 正在接收中的netbuf队列 */
    oal_spin_lock_stru lock;
    oal_pcie_d2h_stat stat;
} pcie_d2h_res;

typedef struct _pcie_h2d_message_res_ {
    pci_addr_map ringbuf_data_dma_addr; /* ringbuf buf地址 */
    pci_addr_map ringbuf_ctrl_dma_addr; /* ringbuf 控制结构体地址 */
    oal_spin_lock_stru lock;
} pcie_h2d_message_res;

typedef struct _pcie_d2h_message_res_ {
    pci_addr_map ringbuf_data_dma_addr; /* ringbuf buf地址 */
    pci_addr_map ringbuf_ctrl_dma_addr; /* ringbuf 控制结构体地址 */
    oal_spin_lock_stru lock;
} pcie_d2h_message_res;

typedef struct _pcie_message_res_ {
    pcie_h2d_message_res h2d_res;
    pcie_d2h_message_res d2h_res;
} pcie_message_res;

typedef struct _pcie_comm_rb_ctrl_res_ {
    pci_addr_map data_daddr; /* ringbuf buf地址 */
    pci_addr_map ctrl_daddr; /* ringbuf 控制结构体地址 */
    oal_spin_lock_stru lock;
} pcie_comm_rb_ctrl_res;

typedef struct _pcie_comm_ringbuf_res_ {
    pcie_comm_rb_ctrl_res comm_rb_res[PCIE_COMM_RINGBUF_BUTT];
} pcie_comm_ringbuf_res;

typedef struct _oal_pcie_bar_info_ {
    uint8_t bar_idx;
    uint64_t start; /* PCIe在Host分配到的总物理地址大小 */
    uint64_t end;

    /* PCIe 发出的总线地址空间， 和start 有可能一样，
      有可能不一样，这个值是配置到BAR 和iatu 的 SRC 地址 */
    uint64_t bus_start;

    uint32_t size;
} oal_pcie_bar_info;

#define oal_pcie_to_name(name) #name
typedef struct _oal_pcie_region_ {
    void *vaddr;  /* virtual address after remap */
    uint64_t paddr; /* PCIe在Host侧分配到的物理地址 */

    uint64_t bus_addr; /* PCIe RC 发出的总线地址 */

    /* pci为PCI看到的地址和CPU看到的地址 每个SOC 大小和地址可能有差异 */
    /* device pci address */
    uint64_t pci_start;
    uint64_t pci_end;
    /* Device侧CPU看到的地址 */
    uint64_t cpu_start;
    uint64_t cpu_end;
    uint32_t size;

    uint32_t flag; /* I/O type,是否需要刷Cache */

    oal_resource *res;
    char *name; /* resource name */

    oal_pcie_bar_info *bar_info; /* iatu 对应的bar信息 */
} oal_pcie_region;

/* IATU BAR by PCIe mem package */
typedef struct _oal_pcie_iatu_bar_ {
    oal_pcie_region st_region;
    oal_pcie_bar_info st_bar_info;
} oal_pcie_iatu_bar;

typedef struct _oal_pcie_regions_ {
    oal_pcie_region *pst_regions;
    int32_t region_nums; /* region nums */

    oal_pcie_bar_info *pst_bars;
    int32_t bar_nums;

    int32_t inited; /* 非0表示初始化过 */
} oal_pcie_regions;

typedef struct _oal_pcie_address_info_ {
    uint32_t edma_ctrl; /* edma ctrl base address */
    uint32_t pcie_ctrl; /* pcie ctrl base address */
    uint32_t dbi; /* pcie config base address */
    uint32_t ete_ctrl; /* pcie host ctrl address */
    uint32_t glb_ctrl;
    uint32_t pmu_ctrl;
    uint32_t pmu2_ctrl;
    uint32_t sharemem_addr; /* device reg1 */
    uint32_t boot_version; /* bootloader  */
} oal_pcie_address_info;

typedef struct _oal_pcie_linux_res_ oal_pcie_linux_res;
typedef struct _pcie_chip_pm_cb_ {
    int32_t (*pcie_device_auxclk_init)(oal_pcie_linux_res *pst_pci_res);
    int32_t (*pcie_device_aspm_init)(oal_pcie_linux_res *pst_pci_res);
    void    (*pcie_device_aspm_ctrl)(oal_pcie_linux_res *pst_pci_res, oal_bool_enum clear);
    int32_t (*pcie_device_phy_config)(oal_pcie_linux_res *pst_pci_res);
} pcie_chip_pm_cb;

typedef struct _pcie_chip_cb_ {
    uintptr_t (*get_test_ram_address)(void);
    int32_t (*pcie_voltage_bias_init)(oal_pcie_linux_res *);
    int32_t (*pcie_get_bar_region_info)(oal_pcie_linux_res *, oal_pcie_region **, uint32_t *);
    int32_t (*pcie_set_outbound_membar)(oal_pcie_linux_res *, oal_pcie_iatu_bar *);
    int32_t (*pcie_host_slave_address_switch)(oal_pcie_linux_res *, uint64_t, uint64_t*, int32_t);
    int32_t (*pcie_ip_factory_test)(hcc_bus *pst_bus, int32_t test_count);
    int32_t (*pcie_poweroff_complete)(oal_pcie_linux_res *);
    void (*ete_address_init)(oal_pcie_linux_res *);
    int32_t (*ete_intr_init)(oal_pcie_linux_res *);
    pcie_chip_pm_cb pm_cb;
} pcie_chip_cb;

typedef struct _pcie_chip_info_ {
    uint32_t dual_pci_support; /* 2 pcie_eps */
    uint32_t ete_support;
    uint32_t edma_support;
    uint32_t membar_support; /* iatu set by membar or config */

    oal_pcie_address_info addr_info;
    pcie_chip_cb  cb; // callback functions
} pcie_chip_info;

typedef struct {
    uint32_t version;  /* pcie version:vid + pid */
    uint32_t revision; /* ip version, 从pcie设备配置空间读取 */
    oal_pci_dev_stru *pcie_dev; /* Linux PCIe device hander */
    pci_wlan_link_state link_state;
    oal_atomic  refcnt;
    /* 根据Soc设计信息表刷新，不同的产品划分不一样, iATU必须对每个region分别映射 */
    oal_pcie_regions regions; /* Device地址划分 */

    /* Bar1 for iatu by mem package */
    oal_pcie_iatu_bar st_iatu_bar;
    struct pci_saved_state *default_state;
    struct pci_saved_state *state;
    uint32_t irq_stat; /* 0:enabled, 1:disabled */
    oal_spin_lock_stru st_lock;
#ifdef CONFIG_ARCH_KIRIN_PCIE
#ifndef _PRE_HI375X_PCIE
    struct kirin_pcie_register_event pcie_event;
#endif
#endif
    int32_t     pci_cnt; /* pcie ip count */
    oal_mutex_stru sr_lock; /* for dual pcie */
#ifdef _PRE_PLAT_FEATURE_HI110X_DUAL_PCIE
    struct pci_saved_state *default_state_dual;
    struct pci_saved_state *state_dual;
#ifdef CONFIG_ARCH_KIRIN_PCIE
    struct kirin_pcie_register_event pcie_event_dual;
#endif
#endif
} oal_pcie_comm_res;

typedef struct _oal_pcie_ep_res_ {
    uint32_t l1_err_cnt;
    oal_completion st_pcie_ready;
    pcie_ep_ip_status power_status;
    pci_addr_map dev_share_mem; /* Device share mem 管理结构体地址 */

    /* ringbuf 管理结构体,Host存放一份是因为PCIE访问效率没有DDR直接访问高 */
    pcie_ringbuf_res st_ringbuf;
    pci_addr_map st_ringbuf_map; /* device ringbuf在host侧的地址映射 */

    pci_addr_map st_device_stat_map;
    pcie_stats st_device_stat;

    /* RINGBUFF在Host侧对应的资源 */
    pcie_h2d_res st_tx_res[PCIE_H2D_QTYPE_BUTT];
    pcie_d2h_res st_rx_res;
    pcie_message_res st_message_res; /* Message Ringbuf */
    pcie_comm_ringbuf_res st_ringbuf_res;

    oal_pcie_trans_stat stat;

    /* PCIe Device 寄存器基地址,Host Virtual Address */
    void *pst_pci_dma_ctrl_base;
    void *pst_pci_ctrl_base;
    void *pst_pci_dbi_base;
    void *pst_ete_base;

    /* Rx 补充内存线程 2级线程 高优先级线程实时补充+低优先级补充 */
    struct task_struct *pst_rx_hi_task;
    struct task_struct *pst_rx_normal_task;

    oal_mutex_stru st_rx_mem_lock;
    oal_wait_queue_head_stru st_rx_hi_wq;
    oal_wait_queue_head_stru st_rx_normal_wq;

    oal_atomic rx_hi_cond;
    oal_atomic rx_normal_cond;
#ifdef CONFIG_ARCH_KIRIN_PCIE
    oal_atomic st_pcie_wakeup_flag;
#endif
    pcie_chip_info chip_info;
    oal_ete_res ete_info;
    pci_addr_map st_device_shared_addr_map[PCIE_SHARED_ADDR_BUTT];
} oal_pcie_ep_res;

struct _oal_pcie_linux_res_ {
    oal_pcie_comm_res *comm_res;
    oal_pcie_ep_res ep_res;
    hcc_bus *pst_bus;
    int32_t inited;
};

typedef struct _oal_pcie_res_node_ {
    oal_pcie_linux_res pcie_res;
    struct list_head list;
} oal_pcie_res_node;

oal_pcie_linux_res *oal_pcie_res_init(oal_pci_dev_stru *pst_pci_dev, uint32_t ep_id);
int32_t oal_pcie_res_deinit(oal_pcie_linux_res *pcie_res);
oal_pcie_linux_res *oal_get_default_pcie_handler(void);
oal_pcie_linux_res *oal_get_pcie_handler(int32_t dev_id);
int32_t pcie_wlan_func_register(pcie_wlan_callback_group_stru *p_func);

int32_t oal_wifi_platform_load_pcie(void);
void oal_wifi_platform_unload_pcie(void);

int32_t oal_pcie_set_loglevel(int32_t loglevel);
int32_t oal_pcie_set_hi11xx_loglevel(int32_t loglevel);
int32_t oal_disable_pcie_irq(oal_pcie_linux_res *pst_pci_lres);
int32_t oal_pcie_ip_factory_test(hcc_bus *pst_bus, int32_t test_count);
int32_t oal_pcie_ip_voltage_bias_init(hcc_bus *pst_bus);
int32_t oal_pcie_rc_slt_chip_transfer(hcc_bus *pst_bus, void *ddr_address,
                                      uint32_t data_size, int32_t direction);
int32_t oal_pcie_ip_init(hcc_bus *pst_bus);
int32_t oal_pcie_110x_working_check(void);

extern int32_t g_kirin_rc_idx;

oal_pci_dev_stru *oal_get_wifi_pcie_dev(void);
// for hcc bus ops
int32_t oal_pcie_get_state(hcc_bus *pst_bus, uint32_t mask);
void oal_enable_pcie_state(hcc_bus *pst_bus, uint32_t mask);
void oal_disable_pcie_state(hcc_bus *pst_bus, uint32_t mask);
int32_t oal_pcie_rx_netbuf(hcc_bus *pst_bus, oal_netbuf_head_stru *pst_head);
int32_t oal_pcie_send_msg(hcc_bus *pst_bus, uint32_t val);
int32_t oal_pcie_host_lock(hcc_bus *pst_bus);
int32_t oal_pcie_host_unlock(hcc_bus *pst_bus);
int32_t oal_pcie_sleep_request(hcc_bus *pst_bus);
int32_t oal_pcie_wakeup_request(hcc_bus *pst_bus);
int32_t oal_pcie_get_sleep_state(hcc_bus *pst_bus);
int32_t oal_pcie_rx_int_mask(hcc_bus *pst_bus, int32_t is_mask);
int32_t oal_pcie_power_ctrl(hcc_bus *hi_bus, hcc_bus_power_ctrl_type ctrl,
                            int (*func)(void *data), void *data);
int32_t oal_pcie_gpio_irq(hcc_bus *hi_bus, int32_t irq);
int32_t oal_pcie_gpio_rx_data(hcc_bus *hi_bus);
int32_t oal_pcie_reinit(hcc_bus *pst_bus);
int32_t oal_pcie_deinit(hcc_bus *pst_bus);
int32_t oal_pcie_patch_read(hcc_bus *pst_bus, uint8_t *buff, int32_t len, uint32_t timeout);
int32_t oal_pcie_patch_write(hcc_bus *pst_bus, uint8_t *buff, int32_t len);
int32_t oal_pcie_bindcpu(hcc_bus *pst_bus, uint32_t chan, int32_t is_bind);
int32_t oal_pcie_get_trans_count(hcc_bus *hi_bus, uint64_t *tx, uint64_t *rx);
void oal_pcie_chip_info(hcc_bus *pst_bus, uint32_t is_need_wakeup, uint32_t is_full_log);
void oal_pcie_print_trans_info(hcc_bus *hi_bus, uint64_t print_flag);
void oal_pcie_reset_trans_info(hcc_bus *hi_bus);
void oal_pcie_wlan_pm_vote(hcc_bus *hi_bus, uint8_t uc_allow);

int32_t oal_enable_pcie_irq(oal_pcie_linux_res *pst_pci_lres);
void pcie_bus_power_down_action(hcc_bus *pst_bus);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
int32_t oal_pcie_pm_control(struct pci_dev *pdev, u32 rc_idx, int power_option);
int32_t oal_pcie_power_notifiy_register(struct pci_dev *pdev, u32 rc_idx, int (*poweron)(void *data),
                                        int (*poweroff)(void *data), void *data);
#endif
int32_t oal_pcie_host_ip_init(oal_pcie_linux_res *pst_pci_lres);
#ifdef _PRE_CONFIG_PCIE_SHARED_INTX_IRQ
int32_t oal_enable_pcie_irq_with_request(oal_pcie_linux_res *pst_pci_lres);
#endif
int32_t oal_pcie_enable_device_func(oal_pcie_linux_res *pst_pci_lres);
void oal_pcie_print_chip_info(oal_pcie_linux_res *pst_pci_lres, uint32_t is_full_log);

int32_t oal_pcie_check_tx_param(hcc_bus *pst_bus, oal_netbuf_head_stru *pst_head,
                                hcc_netbuf_queue_type qtype);
void oal_pcie_rx_netbuf_hdr_init(oal_pci_dev_stru *hwdev, oal_netbuf_stru *pst_netbuf);
#if defined(CONFIG_ARCH_KIRIN_PCIE) || defined(_PRE_CONFIG_ARCH_HI1620S_KUNPENG_PCIE)
int32_t oal_pcie_device_wakeup_handler(const void *data);
#endif
void oal_pcie_edma_bus_ops_init(hcc_bus *pst_bus);
void oal_pcie_ete_bus_ops_init(hcc_bus *pst_bus);

int32_t oal_pcie_wakelock_active(hcc_bus *pst_bus);

int32_t oal_pcie_sr_para_check(oal_pcie_linux_res *pst_pci_lres,
                               hcc_bus **pst_bus, struct hcc_handler **pst_hcc);
int32_t oal_pcie_edma_suspend(oal_pci_dev_stru *pst_pci_dev, oal_pm_message_t state);
int32_t oal_pcie_edma_resume(oal_pci_dev_stru *pst_pci_dev);
int32_t oal_pcie_ete_suspend(oal_pci_dev_stru *pst_pci_dev, oal_pm_message_t state);
int32_t oal_pcie_ete_resume(oal_pci_dev_stru *pst_pci_dev);
int32_t oal_pcie_save_default_resource(oal_pcie_linux_res *pst_pci_lres);
#ifdef CONFIG_ARCH_KIRIN_PCIE
uint32_t oal_pcie_is_master_ip(oal_pci_dev_stru *pst_pci_dev);
int32_t oal_pcie_get_pcie_rc_idx(oal_pci_dev_stru *pst_pci_dev);
int32_t oal_pcie_enumerate(int32_t rc_idx);
#endif
int32_t oal_pcie_110x_init(void);

/* var declare */
extern int32_t g_pcie_print_once;
extern int32_t g_pcie_auto_disable_aspm;
extern int32_t g_hipci_msi_enable; /* 0 -intx 1-pci */
extern int32_t g_hipci_gen_select;
extern int32_t g_ft_pcie_aspm_check_bypass;
extern int32_t g_ft_pcie_gen_check_bypass;
extern int32_t g_hipci_sync_flush_cache_enable; // for device
extern int32_t g_hipci_sync_inv_cache_enable;   // for cpu
#endif
