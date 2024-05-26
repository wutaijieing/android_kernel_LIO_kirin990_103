
#if defined(PLATFORM_DEBUG_ENABLE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef _PRE_PLAT_FEATURE_HI116X_PCIE
#define HI11XX_LOG_MODULE_NAME "[ETE_DBG_H]"
#define HISI_LOG_TAG           "[ETE]"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/pci.h>
#endif
#include "oal_util.h"
#include "pcie_host.h"
#include "pcie_linux.h"
#include "plat_pm_wlan.h"
#include "oal_hcc_host_if.h"
#include "hi1161/pcie_ctrl_rb_regs.h"

/* ete memcpy test marcors define */
#define ETE_MEMCPY_TX 1
#define ETE_MEMCPY_RX 2
#define ETE_MEMCPY_LOOP 3

#define ETE_MEMCPY_MINFO_MSIZE 4096 /* PageSize */
#define ETE_MEMCPY_MINFO_MAGIC  0xbeaf0008
#define ETE_MEMCPY_IINFO_RESULT_OFFSET (ETE_MEMCPY_MINFO_MSIZE / 2)

#define PCIE_ETE_MEMCPY_TEST_MSG 255

#define ETE_MEMCPY_BUFF_MSIZE 65536 /* Max ete copy size 64KB */
#define ETE_MEMCPY_BUFF_DATA  0x5a

/* debug flag bits */
#define ETE_MEMCPY_DBG_DATA_VERFIY (1 << 0)
#define ETE_MEMCPY_DBG_ACP_RW      (1 << 1)
#define ETE_MEMCPY_DBG_SCHD_CHECK  (1 << 2)

/* 均按照小端数据处理 */
typedef struct _ete_memcpy_test_minfo_ {
    uint32_t magic;
    uint32_t mode; // tx,rx,loop
    uint32_t burst_size;
    uint32_t data; // sizeof char, memset for deivce
    uint32_t runtime;
    uint64_t tx_buff_slave_addr64;
    uint64_t rx_buff_slave_addr64;
    uint32_t debug;
} ete_memcpy_test_minfo;

typedef struct _ete_memcpy_test_result_ {
    uint32_t magic;
    int32_t ret;
    uint64_t transfer_size; /* 总共传输数据大小，单方向 */
    uint64_t transfer_time; /* 总共花费的时间开销,吞吐率在Host计算 */
    uint64_t transfer_count; /* 每搬运一个burst, 递增 */
} ete_memcpy_test_result;

int32_t g_ete_memcpy_debug = (ETE_MEMCPY_DBG_DATA_VERFIY | ETE_MEMCPY_DBG_ACP_RW);
oal_debug_module_param(g_ete_memcpy_debug, int, S_IRUGO | S_IWUSR);

#define ETE_MEMCPY_THROUGHPUT_PARAM_SIZE 50
char g_ete_memcpy_throughput[ETE_MEMCPY_THROUGHPUT_PARAM_SIZE] = { 0 };
oal_debug_module_param_string(ete_throughput, g_ete_memcpy_throughput, ETE_MEMCPY_THROUGHPUT_PARAM_SIZE, S_IRUGO);

/* ete memcpy test infos */
unsigned long *g_minfo_dma_vaddr = NULL; /* 存放管理信息，host ddr, nocache */
dma_addr_t g_minfo_dma_paddr = 0;
uint64_t g_minfo_pci_slave_addr = 0;

unsigned long *g_tx_buff_dma_vaddr = NULL; /* ete tx 源端内存 */
dma_addr_t g_tx_buff_dma_addr = 0;
uint64_t g_tx_buff_pci_slave_addr = 0;

unsigned long *g_rx_buff_dma_vaddr = NULL; /* ete rx 目的端内存 */
dma_addr_t g_rx_buff_dma_addr = 0;
uint64_t g_rx_buff_pci_slave_addr = 0;

static int32_t oal_get_pcie_ete_memtrans_addr(oal_pcie_linux_res *pst_pcie_res, uint32_t reg_low, uint32_t reg_high,
                                              pci_addr_map *reg_low_map, pci_addr_map *reg_high_map)
{
    int32_t ret;

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res->comm_res, reg_low, reg_low_map);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reg_low 0x%x convert failed!", reg_low);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res->comm_res, reg_high, reg_high_map);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reg_high 0x%x convert failed!", reg_high);
        return ret;
    }

    return OAL_SUCC;
}

static int32_t oal_pcie_ete_memtrans_minfo_alloc(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev)
{
    int32_t ret;
    g_minfo_dma_vaddr = dma_alloc_coherent(&pst_pci_dev->dev, ETE_MEMCPY_MINFO_MSIZE,
                                           &g_minfo_dma_paddr,
                                           GFP_KERNEL);
    if (g_minfo_dma_vaddr == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dma alloc minfo mem failed, size=%d", ETE_MEMCPY_MINFO_MSIZE);
        return -OAL_ENOMEM;
    }

    ret = oal_pcie_host_slave_address_switch(pst_pcie_res, (uint64_t)g_minfo_dma_paddr,
                                             &g_minfo_pci_slave_addr, OAL_TRUE);
    if (ret != OAL_SUCC) {
        return ret;
    }

    memset_s(g_minfo_dma_vaddr, ETE_MEMCPY_MINFO_MSIZE, 0, ETE_MEMCPY_MINFO_MSIZE);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ete memcpy test minfo iova:0x%llx va:0x%p , pci slave:0x%llx",
                         (uint64_t)g_minfo_dma_paddr, g_minfo_dma_vaddr, g_minfo_pci_slave_addr);

    return OAL_SUCC;
}


static void oal_pcie_ete_memtrans_minfo_free(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev)
{
    if (g_minfo_dma_vaddr == NULL) {
        return;
    }
    dma_free_coherent(&pst_pci_dev->dev, ETE_MEMCPY_MINFO_MSIZE, g_minfo_dma_vaddr, g_minfo_dma_paddr);
    g_minfo_dma_vaddr = NULL;
}

static int32_t oal_pcie_ete_memtrans_txbuff_alloc(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                                  uint32_t size)
{
    int32_t ret;
    g_tx_buff_dma_vaddr = dma_alloc_coherent(&pst_pci_dev->dev, size, &g_tx_buff_dma_addr, GFP_KERNEL);
    if (g_tx_buff_dma_vaddr == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dma alloc minfo mem failed, size=%d", size);
        return -OAL_ENOMEM;
    }

    ret = oal_pcie_host_slave_address_switch(pst_pcie_res, (uint64_t)g_tx_buff_dma_addr,
                                             &g_tx_buff_pci_slave_addr, OAL_TRUE);
    if (ret != OAL_SUCC) {
        return ret;
    }

    memset_s(g_tx_buff_dma_vaddr, size, ETE_MEMCPY_BUFF_DATA, size);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ete memcpy test txbuf iova:0x%llx va:0x%p , pci slave:0x%llx",
                         (uint64_t)g_tx_buff_dma_addr, g_tx_buff_dma_vaddr, g_tx_buff_pci_slave_addr);

    return OAL_SUCC;
}

static void oal_pcie_ete_memtrans_txbuff_free(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                              uint32_t size)
{
    if (g_tx_buff_dma_vaddr == NULL) {
        return;
    }
    dma_free_coherent(&pst_pci_dev->dev, size, g_tx_buff_dma_vaddr, g_tx_buff_dma_addr);
    g_tx_buff_dma_vaddr = NULL;
}


static int32_t oal_pcie_ete_memtrans_rxbuff_alloc(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                                  uint32_t size)
{
    int32_t ret;
    g_rx_buff_dma_vaddr = dma_alloc_coherent(&pst_pci_dev->dev, size, &g_rx_buff_dma_addr, GFP_KERNEL);
    if (g_rx_buff_dma_vaddr == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dma alloc minfo mem failed, size=%d", size);
        return -OAL_ENOMEM;
    }

    ret = pcie_if_hostca_to_devva(0, (uint64_t)g_rx_buff_dma_addr, &g_rx_buff_pci_slave_addr);
    if (ret != OAL_SUCC) {
        return ret;
    }

    memset_s(g_rx_buff_dma_vaddr, size, 0, size);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ete memcpy test rxbuf iova:0x%llx va:0x%p , pci slave:0x%llx",
                         (uint64_t)g_rx_buff_dma_addr, g_rx_buff_dma_vaddr, g_rx_buff_pci_slave_addr);

    return OAL_SUCC;
}

static void oal_pcie_ete_memtrans_rxbuff_free(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                              uint32_t size)
{
    if (g_rx_buff_dma_vaddr == NULL) {
        return;
    }
    dma_free_coherent(&pst_pci_dev->dev, size, g_rx_buff_dma_vaddr, g_rx_buff_dma_addr);
    g_rx_buff_dma_vaddr = NULL;
}

/* 组装测试用例信息头 */
static void oal_pcie_ete_memtrans_build_minfo(uint32_t mode_int,
                                              uint32_t burst_size,
                                              uint32_t runtime)
{
    ete_memcpy_test_minfo *test_minfo = (ete_memcpy_test_minfo *)g_minfo_dma_vaddr;
    ete_memcpy_test_result *result =
        (ete_memcpy_test_result *)((uintptr_t)g_minfo_dma_vaddr + ETE_MEMCPY_IINFO_RESULT_OFFSET);
    test_minfo->mode = mode_int;
    test_minfo->burst_size = burst_size;
    test_minfo->data = ETE_MEMCPY_BUFF_DATA;
    test_minfo->debug = g_ete_memcpy_debug;
    test_minfo->runtime = runtime;
    test_minfo->tx_buff_slave_addr64 = g_tx_buff_pci_slave_addr;
    test_minfo->rx_buff_slave_addr64 = g_rx_buff_pci_slave_addr;
    test_minfo->magic = ETE_MEMCPY_MINFO_MAGIC;

    result->ret = -OAL_EINTR;
}

int32_t pcie_ete_memtrans_data_check(ete_memcpy_test_minfo* test_minfo)
{
    uint32_t i;
    uint32_t len = test_minfo->burst_size;
    uint8_t* dev_buf_addr = (uint8_t*)(uintptr_t)g_rx_buff_dma_vaddr;
    if (!(test_minfo->debug & ETE_MEMCPY_DBG_DATA_VERFIY)) {
        return OAL_SUCC;
    }
    for (i = 0; i < len; i++) {
        uint8_t c = *(dev_buf_addr + i);
        if (c != (uint8_t)test_minfo->data) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "addr offset: 0x%x err data:0x%x expect:0x%x", i, c,
                (uint8_t)test_minfo->data);
            return -OAL_EFAUL;
        }
    }
    return OAL_SUCC;
}

static int32_t oal_pcie_ete_memtrans_wait_case_sched(ete_memcpy_test_minfo* test_minfo, ete_memcpy_test_result *result,
                                                     uint64_t *last_cnt, uint32_t *timeout_cnt,
                                                     const uint32_t max_timeout)
{
    if (test_minfo->debug & ETE_MEMCPY_DBG_SCHD_CHECK) { // check ete whether work
        if (*last_cnt != result->transfer_count) {
            *timeout_cnt = 0;
        } else {
            if (++(*timeout_cnt) > max_timeout) {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "wait ete memcpy test done timeout!");
                return -OAL_ETIMEDOUT;
            }
        }
    }
    return OAL_SUCC;
}

/* 等待device测试完成 */
static int32_t oal_pcie_ete_memtrans_test_wait_done(oal_pcie_linux_res *pst_pcie_res)
{
    int32_t ret;
    unsigned long timeout;
    uint64_t last_cnt = 0;
    uint32_t timeout_cnt = 0;
    const uint32_t max_timeout =  10; // per burst size, timeout 1 second!
    ete_memcpy_test_minfo* test_minfo = (ete_memcpy_test_minfo*)g_minfo_dma_vaddr;
    ete_memcpy_test_result *result =
        (ete_memcpy_test_result *)((uintptr_t)g_minfo_dma_vaddr + ETE_MEMCPY_IINFO_RESULT_OFFSET);
    forever_loop() {
        if (oal_unlikely(pst_pcie_res->comm_res->link_state < PCI_WLAN_LINK_WORK_UP)) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "link err, link_state:%s",
                                 oal_pcie_get_link_state_str(pst_pcie_res->comm_res->link_state));
            return -OAL_ENODEV;
        }

        if (result->magic == ETE_MEMCPY_MINFO_MAGIC) {
            /* memcpy test done! */
            break;
        }
        ret = oal_pcie_ete_memtrans_wait_case_sched(test_minfo, result, &last_cnt, &timeout_cnt, max_timeout);
        if (ret != OAL_SUCC) {
            return ret;
        }
        timeout = msleep_interruptible(100); // 100ms can recv interrupt signal
        if (timeout != 0) {
            // current thread was interrupted
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "current thread was interrupted!");
            return -OAL_ETIMEDOUT;
        }
    }

    if (result->ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid result ret=%d from device",
                             result->ret);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "transfer count=%llu", result->transfer_count);
        return result->ret;
    }

    /* 检查搬运是否真正完成 */
    if ((test_minfo->mode == ETE_MEMCPY_RX) || (test_minfo->mode == ETE_MEMCPY_LOOP)) {
        ret = pcie_ete_memtrans_data_check(test_minfo);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    return OAL_SUCC;
}

static char* oal_pcie_get_ete_memtrans_test_modestr(uint32_t mode)
{
    switch (mode) {
        case ETE_MEMCPY_TX:
            return "tx";
        case ETE_MEMCPY_RX:
            return "rx";
        case ETE_MEMCPY_LOOP:
            return "loop";
        default:
            return "unkown";
    }
}

#define THROUGHPUT_GBPS_SHIFT 30
#define THROUGHPUT_MBPS_SHIFT 20
#define THROUGHPUT_KBPS_SHIFT 10
/* 计算吞吐率 */
static void oal_pcie_ete_memtrans_test_calc(void)
{
    int32_t ret;
    uint64_t trans_ms;
    uint64_t trans_size;
    uint64_t temp;
    uint32_t mode_int;
    char* str = NULL;
    uint32_t shift;
    ete_memcpy_test_minfo* test_minfo = (ete_memcpy_test_minfo*)g_minfo_dma_vaddr;
    ete_memcpy_test_result *result =
        (ete_memcpy_test_result *)((uintptr_t)g_minfo_dma_vaddr + ETE_MEMCPY_IINFO_RESULT_OFFSET);
    trans_ms = result->transfer_time;
    trans_size = result->transfer_count * (uint64_t)test_minfo->burst_size;
    mode_int = test_minfo->mode;
    temp = trans_size * 8 * 1000; // 8 bits, bps, 1000: us to ms
    temp = div_u64(temp, trans_ms);
    if (temp >> THROUGHPUT_MBPS_SHIFT) {
        shift = THROUGHPUT_MBPS_SHIFT;
        str = "Mbps";
    } else if (temp >> THROUGHPUT_KBPS_SHIFT) {
        shift = THROUGHPUT_KBPS_SHIFT;
        str = "Kbps";
    } else {
        shift = 0;
        str = "bps";
    }
    /* Throughput Log Output */
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "Result:");
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ETE Memcpy Test Mode:%s",
                         oal_pcie_get_ete_memtrans_test_modestr(test_minfo->mode));
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ETE Transer BurstSize: %u (Bytes), burst count: %llu ",
                         test_minfo->burst_size, result->transfer_count);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ETE Transer Size: %llu (Bytes)",  result->transfer_size);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ETE Transer Time: %llu (ms)",  result->transfer_time);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "ETE Transer Throught: %llu %s",  temp >> shift, str);
    ret = snprintf_s(g_ete_memcpy_throughput, ETE_MEMCPY_THROUGHPUT_PARAM_SIZE,
                     ETE_MEMCPY_THROUGHPUT_PARAM_SIZE - 1, "%llu %s", temp >> shift, str);
    if (ret <= 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "snprintf failed, ret=%d", ret);
    }
}

/* reinit result mem etc. */
static void oal_pcie_ete_memtrans_mode_reinit(void)
{
    int32_t ret;
    ret = snprintf_s(g_ete_memcpy_throughput, ETE_MEMCPY_THROUGHPUT_PARAM_SIZE,
                     ETE_MEMCPY_THROUGHPUT_PARAM_SIZE - 1, "FAIL");
    if (ret <= 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "oal_pcie_ete_memtrans_mode_reinit::snprintf failed, ret=%d", ret);
    }
}

static int32_t oal_pcie_ete_memtrans_build_addrinfo(uint32_t *low, uint32_t *high, uint32_t *low_r, uint32_t *high_r,
                                                    pci_addr_map *reg_low_map, pci_addr_map *reg_high_map)
{
    *low = (uint32_t)g_minfo_pci_slave_addr;
    *high = (uint32_t)(g_minfo_pci_slave_addr >> 32); // 取高32bits
    /* 先写地位再写高位，device先读高位再读地位 */
    oal_writel(*low, reg_low_map->va);
    oal_writel(*high, reg_high_map->va);
    /* 回读确认，PCIE链路可能不正常 */
    *low_r = oal_readl(reg_low_map->va);
    *high_r = oal_readl(reg_high_map->va);
    if ((*low_r != *low) || (*high_r != *high)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "low_r(0x%x) != low(0x%x) or high_r(0x%x) != high(0x%x)",
                             *low_r, *low, *high_r, *high);
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set ete memcpy minfo address done");
    return OAL_SUCC;
}

static int32_t oal_pcie_ete_memtrans_para_check(oal_pcie_linux_res *pst_pcie_res,
                                                oal_pci_dev_stru *pst_pci_dev, uint32_t mode_int,
                                                uint32_t burst_size, uint32_t runtime,
                                                uint32_t reg_low, uint32_t reg_high,
                                                pci_addr_map *reg_low_map,
                                                pci_addr_map *reg_high_map)
{
    int32_t ret;
    if ((burst_size == 0) || (burst_size > ETE_MEMCPY_BUFF_MSIZE)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid burst_size %u", burst_size);
        return -OAL_EINVAL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "mode_int=%u , burst_size=%u, runtime=%u, reg_low=0x%x, reg_high=0x%x",
                         mode_int, burst_size, runtime, reg_low, reg_high);

    ret = oal_get_pcie_ete_memtrans_addr(pst_pcie_res, reg_low, reg_high, reg_low_map, reg_high_map);
    if (ret != OAL_SUCC) {
        return ret;
    }

    return OAL_SUCC;
}

/*
 * ete memcpy 模式回环测试, ete outstanding能力弱于mac, 峰值数据完全代表pcie slave口的性能
 * 1. wifi 子系统上电下载完firmware,不启动业务代码， 此时才能读写pktmem内存
 * 2. ete memcpy测试用例通过2个32bits寄存器下发dma_nocache ddr 地址和wcpu交互
 * 3. ddr 单独的ram 记录返回的测试结果
 */
static int32_t oal_pcie_ete_memcpy_mode_test(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                             uint32_t mode_int, uint32_t burst_size, uint32_t runtime,
                                             uint32_t reg_low, uint32_t reg_high)
{
    int32_t ret;
    uint32_t low, high, low_r, high_r;
    pci_addr_map reg_low_map, reg_high_map;

    ret = oal_pcie_ete_memtrans_para_check(pst_pcie_res, pst_pci_dev, mode_int, burst_size, runtime, reg_low,
                                           reg_high, &reg_low_map, &reg_high_map);
    if (ret != OAL_SUCC) {
        return ret;
    }

    ret = oal_pcie_ete_memtrans_minfo_alloc(pst_pcie_res, pst_pci_dev);
    if (ret != OAL_SUCC) {
        return ret;
    }

    if (mode_int & ETE_MEMCPY_TX) {
        ret = oal_pcie_ete_memtrans_txbuff_alloc(pst_pcie_res, pst_pci_dev, burst_size);
        if (ret != OAL_SUCC) {
            goto failed_tx_buff_alloc;
        }
    }

    if (mode_int & ETE_MEMCPY_RX) {
        ret = oal_pcie_ete_memtrans_rxbuff_alloc(pst_pcie_res, pst_pci_dev, burst_size);
        if (ret != OAL_SUCC) {
            goto failed_rx_buff_alloc;
        }
    }

    ret = oal_pcie_ete_memtrans_build_addrinfo(&low, &high, &low_r, &high_r, &reg_low_map, &reg_high_map);
    if (ret != OAL_SUCC) {
        goto done;
    }

    oal_pcie_ete_memtrans_build_minfo(mode_int, burst_size, runtime);

    /* 启动发送 */
    ret = oal_pcie_send_message_to_dev(pst_pcie_res, PCIE_ETE_MEMCPY_TEST_MSG);
    if (ret != OAL_SUCC) {
        goto done;
    }

    /* 回读确认测试结果 */
    ret = oal_pcie_ete_memtrans_test_wait_done(pst_pcie_res);
    if (ret != OAL_SUCC) {
        goto done;
    }

    oal_pcie_ete_memtrans_test_calc();

    /* 清空寄存器，共存WIFI下电AON不会清空 */
    oal_writel(0, (void*)reg_low_map.va);
    oal_writel(0, (void*)reg_high_map.va);

done:
    oal_pcie_ete_memtrans_rxbuff_free(pst_pcie_res, pst_pci_dev, burst_size);
failed_rx_buff_alloc:
    oal_pcie_ete_memtrans_txbuff_free(pst_pcie_res, pst_pci_dev, burst_size);
failed_tx_buff_alloc:
    oal_pcie_ete_memtrans_minfo_free(pst_pcie_res, pst_pci_dev);
    return ret;
}

#define ETE_TEST_MODE_SIZE  20
#define ETE_MEMCPY_TEST_STR "ete_memcpy_test"
int32_t oal_pcie_testcase_ete_memcpy_test(oal_pcie_linux_res *pst_pcie_res, oal_pci_dev_stru *pst_pci_dev,
                                          const char *buf)
{
    /*
     * trigger ete ip test, tx(h2d) rx(d2h) loop(h->d->h), read&write by pcie slave
     * echo test_case ete_memcpy_test tx/rx/loop  burstsize(dec) runtime(ms)  reg0(hex--low_addr) reg1(hex--high_addr)
     */
    int32_t ret;
    uint32_t burst_size = 0;
    uint32_t runtime = 0;
    uint32_t reg_low = 0;
    uint32_t reg_high = 0;
    uint32_t mode_int = 0;
    char mode[ETE_TEST_MODE_SIZE] = { 0 };
    oal_pcie_ete_memtrans_mode_reinit();
    buf = buf + OAL_STRLEN(ETE_MEMCPY_TEST_STR);
    for (; *buf == ' '; buf++);
    if (sscanf_s(buf, "%s %u %u 0x%x 0x%x", mode, ETE_TEST_MODE_SIZE, &burst_size, &runtime, &reg_low, &reg_high) !=
        5) {
        pci_print_log(PCI_LOG_ERR, "[ETE_TEST] input err:%s", buf);
        return OAL_SUCC;
    }

    /* argument check */
    if (sysfs_streq(mode, "tx")) {
        mode_int = 1;
    } else if (sysfs_streq(mode, "rx")) {
        mode_int = 2;
    } else if (sysfs_streq(mode, "loop")) {
        mode_int = 3;
    } else {
        pci_print_log(PCI_LOG_ERR, "[ETE_TEST] input err:%s, mode_int invalid=%s", buf, mode);
        return OAL_SUCC;
    }

    if ((burst_size == 0) || (runtime == 0) || (reg_low == 0) || (reg_high == 0)) {
        pci_print_log(PCI_LOG_ERR, "[ETE_TEST] input err:%s, invalid size/time/reg", buf);
        return OAL_SUCC;
    }
    /*
     * 使用GT测试时，直接使用asic_test版本的FW，不会初始化GT业务；使用WIFI测试时，参考110x ete_debug.c,
     * 加载WIFI FW，但不启动WIFI业务
     */
    ret = oal_pcie_ete_memcpy_mode_test(pst_pcie_res, pst_pci_dev, mode_int, burst_size, runtime, reg_low, reg_high);
    if (ret != OAL_SUCC) {
        pci_print_log(PCI_LOG_ERR, "[ETE_TEST] ete memcpy modetest failed=%d", ret);
    }
    return OAL_SUCC;
}
#endif
#endif
