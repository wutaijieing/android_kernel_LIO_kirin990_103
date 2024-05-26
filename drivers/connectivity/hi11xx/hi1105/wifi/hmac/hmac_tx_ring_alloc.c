

#include "hmac_tx_ring_alloc.h"
#include "hmac_tid_sche.h"
#include "hmac_tid_update.h"
#include "hmac_host_tx_data.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_config.h"
#include "pcie_linux.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_RING_ALLOC_C

typedef struct {
    uint32_t tx_ring_device_base_addr; /* Device Tx Ring TableµÄDev SRAMµØÖ· */
    hmac_tx_ring_device_info_stru tx_ring_device_info[WLAN_ASSOC_USER_MAX_NUM][WLAN_TIDNO_BUTT];
} hmac_tx_ring_device_info_mgmt_stru;

hmac_tx_ring_device_info_mgmt_stru g_tx_ring_device_info_mgmt = { 0 };

static inline uint8_t hmac_get_ring_tx_mode(void)
{
    return hmac_host_ring_tx_enabled() ? HOST_RING_TX_MODE : DEVICE_RING_TX_MODE;
}

static inline void hmac_set_tid_info(hmac_tid_info_stru *tid_info, uint16_t user_index, uint8_t tid)
{
    tid_info->tid_no = tid;
    tid_info->user_index = user_index;
    oal_atomic_set(&tid_info->ring_tx_mode, hmac_get_ring_tx_mode());
    oal_atomic_set(&tid_info->tid_scheduled, OAL_FALSE);
    oal_netbuf_head_init(&tid_info->tid_queue);
}

static hmac_tx_ring_device_info_stru *hmac_get_tx_ring_device_info(uint16_t user_lut_id, uint8_t tid)
{
    return &g_tx_ring_device_info_mgmt.tx_ring_device_info[user_lut_id][tid];
}

static uint32_t hmac_get_device_tx_ring_base_addr(uint8_t lut_idx, uint8_t tid)
{
    return g_tx_ring_device_info_mgmt.tx_ring_device_base_addr + TX_RING_INFO_LEN * (lut_idx * WLAN_TIDNO_BUTT + tid);
}

static uint32_t hmac_tx_ring_device_addr_init(
    hmac_tx_ring_device_info_stru *tx_ring_device_info, uint8_t user_lut_idx, uint8_t tid)
{
    uint64_t hostva = 0;
    int32_t ret;
    uint32_t word_index;
    uint32_t user_tid_ring_base_addr = hmac_get_device_tx_ring_base_addr(user_lut_idx, tid);

    for (word_index = 0; word_index < TX_RING_INFO_WORD_NUM; word_index++) {
        ret = oal_pcie_devca_to_hostva(HCC_EP_WIFI_DEV, user_tid_ring_base_addr, &hostva);
        if (ret != OAL_SUCC) {
            oam_error_log1(0, OAM_SF_TX, "{hmac_tx_set_user_tid_ring_addr::devca to hostva failed[%d]}", ret);
            return ret;
        }

        tx_ring_device_info->word_addr[word_index] = hostva;
        user_tid_ring_base_addr += sizeof(uint32_t);
    }

    return OAL_SUCC;
}

static uint32_t hmac_tx_ring_device_info_init(
    hmac_tx_ring_device_info_stru *ring_device_info, uint8_t user_lut_index, uint8_t tid)
{
    memset_s(ring_device_info, sizeof(hmac_tx_ring_device_info_stru), 0, sizeof(hmac_tx_ring_device_info_stru));

    return hmac_tx_ring_device_addr_init(ring_device_info, user_lut_index, tid);
}

#define HMAC_TX_RING_MAX_AMSDU_NUM 7
static uint32_t hmac_set_tx_ring(hmac_tid_info_stru *tid_info, hmac_user_stru *hmac_user, uint8_t tid)
{
    tid_info->tx_ring_device_info = hmac_get_tx_ring_device_info(hmac_user->lut_index, tid);
    if (hmac_tx_ring_device_info_init(tid_info->tx_ring_device_info, hmac_user->lut_index, tid) != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_CFG, "{hmac_user_set_ring::tx ring device info init failed!}");
        return OAL_FAIL;
    }

    tid_info->tx_ring.base_ring_info.size = hal_host_tx_tid_ring_size_get();
    tid_info->tx_ring.base_ring_info.read_index = 0;
    tid_info->tx_ring.base_ring_info.write_index = 0;
    tid_info->tx_ring.base_ring_info.max_amsdu_num = (tid != WLAN_TIDNO_BCAST) ? HMAC_TX_RING_MAX_AMSDU_NUM : 0;
    tid_info->ring_sync_cnt = 0;

    oal_atomic_set(&tid_info->tx_ring.msdu_cnt, 0);
    oal_atomic_set(&tid_info->tx_ring.last_period_tx_msdu, 0);
    oal_atomic_set(&tid_info->ring_sync_th, 1);
    oal_atomic_set(&tid_info->tid_sche_th, 1);
    oal_atomic_set(&tid_info->ring_tx_opt, OAL_FALSE);
    oal_atomic_set(&tid_info->tid_updated, OAL_FALSE);

    oal_spin_lock_init(&tid_info->tx_ring.tx_lock);
    oal_spin_lock_init(&tid_info->tx_ring.tx_comp_lock);

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static uint32_t hmac_tx_dma_alloc_ring_buf(hmac_msdu_info_ring_stru *tx_ring, uint32_t host_ring_buf_size)
{
    dma_addr_t host_ring_dma_addr = 0;
    void *host_ring_vaddr = NULL;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oam_warning_log2(0, 0, "{hmac_tx_dma_alloc_ring_buf::dev[%d] size[%d]}",
        *(uint32_t*)&pcie_dev->dev, host_ring_buf_size);

    host_ring_vaddr = dma_alloc_coherent(&pcie_dev->dev, host_ring_buf_size,
                                         &host_ring_dma_addr, GFP_KERNEL);

    oam_warning_log2(0, 0, "{hmac_tx_dma_alloc_ring_buf::va[0x%x] da[0x%x]}", host_ring_vaddr, host_ring_dma_addr);

    if (host_ring_vaddr == NULL) {
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    tx_ring->host_ring_buf = (uint8_t *)host_ring_vaddr;
    tx_ring->host_ring_dma_addr = (uint64_t)host_ring_dma_addr;
    tx_ring->host_ring_buf_size = host_ring_buf_size;

    return OAL_SUCC;
}

static uint32_t hmac_tx_dma_release_ring_buf(hmac_msdu_info_ring_stru *tx_ring)
{
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    oam_warning_log1(0, OAM_SF_TX, "{hmac_tx_dma_release_ring_buf::size[%d]}", tx_ring->host_ring_buf_size);

    if ((pcie_dev == NULL) || (tx_ring->host_ring_buf == NULL) ||
        (!tx_ring->host_ring_dma_addr) || (!tx_ring->host_ring_buf_size)) {
        return OAL_FAIL;
    }

    dma_free_coherent(&pcie_dev->dev, tx_ring->host_ring_buf_size,
                      tx_ring->host_ring_buf, (dma_addr_t)tx_ring->host_ring_dma_addr);

    return OAL_SUCC;
}
#else
static uint32_t hmac_tx_dma_alloc_ring_buf(hmac_msdu_info_ring_stru *tx_ring, uint32_t host_ring_buf_size)
{
    dma_addr_t host_ring_dma_addr;
    void *host_ring_vaddr = oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, host_ring_buf_size, OAL_FALSE);

    if (host_ring_vaddr == NULL) {
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    tx_ring->host_ring_buf = (uint8_t *)host_ring_vaddr;
    tx_ring->host_ring_dma_addr = (uint64_t)host_ring_vaddr;
    tx_ring->host_ring_buf_size = host_ring_buf_size;

    return OAL_SUCC;
}

static uint32_t hmac_tx_dma_release_ring_buf(hmac_msdu_info_ring_stru *tx_ring)
{
    oal_mem_free_m(tx_ring->host_ring_buf, OAL_TRUE);

    return OAL_SUCC;
}
#endif

uint32_t hmac_alloc_tx_ring(hmac_msdu_info_ring_stru *tx_ring)
{
    uint32_t ret;
    uint16_t host_ring_size = hal_host_tx_tid_ring_depth_get(tx_ring->base_ring_info.size);
    uint32_t host_ring_buf_size = host_ring_size * TX_RING_MSDU_INFO_LEN;
    uint32_t host_netbuf_buf_size = host_ring_size * sizeof(oal_netbuf_stru *);

    ret = hmac_tx_dma_alloc_ring_buf(tx_ring, host_ring_buf_size);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_tx_host_ring_alloc::dma alloc host buf failed[%d]}", ret);
        return ret;
    }

    tx_ring->netbuf_list = (oal_netbuf_stru **)oal_memalloc(host_netbuf_buf_size);
    if (tx_ring->netbuf_list == NULL) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_host_ring_alloc::kmalloc netbuflist failed}");
        hmac_tx_dma_release_ring_buf(tx_ring);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    memset_s(tx_ring->host_ring_buf, host_ring_buf_size, 0, host_ring_buf_size);
    memset_s(tx_ring->netbuf_list, host_netbuf_buf_size, 0, host_netbuf_buf_size);

    return OAL_SUCC;
}

static void hmac_reset_tx_ring(hmac_msdu_info_ring_stru *tx_ring)
{
    memset_s(&tx_ring->base_ring_info, sizeof(msdu_info_ring_stru), 0, sizeof(msdu_info_ring_stru));
    tx_ring->host_ring_buf = NULL;
    tx_ring->host_ring_dma_addr = 0;
    tx_ring->host_ring_buf_size = 0;
    tx_ring->release_index = 0;
    oal_atomic_set(&tx_ring->msdu_cnt, 0);
    if (tx_ring->netbuf_list != NULL) {
        oal_free(tx_ring->netbuf_list);
        tx_ring->netbuf_list = NULL;
    }
}

uint32_t hmac_init_tx_ring(hmac_user_stru *hmac_user, uint8_t tid)
{
    hmac_tid_info_stru *tid_info = &hmac_user->tx_tid_info[tid];

    if ((hmac_ring_tx_enabled() != OAL_TRUE) || tid == WLAN_TIDNO_BCAST) {
        return OAL_SUCC;
    }

    memset_s(tid_info, sizeof(hmac_tid_info_stru), 0, sizeof(hmac_tid_info_stru));
    hmac_set_tid_info(tid_info, hmac_user->st_user_base_info.us_assoc_id, tid);
    hmac_tid_schedule_list_enqueue(tid_info);

    if (hmac_device_ring_tx_only() == OAL_TRUE) {
        oal_atomic_set(&tid_info->tx_ring.enabled, OAL_TRUE);
        return OAL_SUCC;
    }

    wlan_pm_wakeup_dev_lock();

    if (hmac_set_tx_ring(tid_info, hmac_user, tid) != OAL_SUCC) {
        return OAL_FAIL;
    }

    if (hmac_alloc_tx_ring(&tid_info->tx_ring) != OAL_SUCC) {
        hmac_reset_tx_ring(&tid_info->tx_ring);
        return OAL_FAIL;
    }

    hmac_tx_reg_write_ring_info(tid_info, TID_CMDID_CREATE);
    hmac_tid_update_list_enqueue(tid_info);

    oal_atomic_set(&tid_info->tx_ring.enabled, OAL_TRUE);

    return OAL_SUCC;
}

static inline void hmac_tx_tid_release(oal_netbuf_head_stru *tid_queue)
{
    oal_netbuf_stru *netbuf = NULL;

    while ((netbuf = hmac_tid_netbuf_dequeue(tid_queue)) != NULL) {
        oal_netbuf_free(netbuf);
    }
}

void hmac_tx_ring_release_all_netbuf(hmac_msdu_info_ring_stru *tx_ring)
{
    un_rw_ptr release_ptr = { .rw_ptr = tx_ring->release_index };
    un_rw_ptr target_ptr = { .rw_ptr = tx_ring->base_ring_info.write_index };
    oal_netbuf_stru *netbuf = NULL;

    while (hmac_tx_rw_ptr_compare(target_ptr, release_ptr) == RING_PTR_GREATER) {
        netbuf = hmac_tx_ring_get_netbuf(tx_ring, release_ptr);
        if (netbuf != NULL) {
            hmac_tx_ring_release_netbuf(tx_ring, netbuf, release_ptr.st_rw_ptr.bit_rw_ptr);
        }

        hmac_tx_reset_msdu_info(tx_ring, release_ptr);
        hmac_tx_msdu_ring_inc_release_ptr(tx_ring);

        release_ptr.rw_ptr = tx_ring->release_index;
    }
}

static inline void hmac_tx_ring_release_netbuf_record(hmac_msdu_info_ring_stru *tx_ring)
{
    if (tx_ring->netbuf_list == NULL) {
        return;
    }

    oal_free(tx_ring->netbuf_list);
    tx_ring->netbuf_list = NULL;
}

void hmac_tx_host_ring_release(hmac_msdu_info_ring_stru *tx_ring)
{
    oal_spin_lock(&tx_ring->tx_lock);
    oal_spin_lock(&tx_ring->tx_comp_lock);

    hmac_tx_ring_release_all_netbuf(tx_ring);
    hmac_tx_ring_release_netbuf_record(tx_ring);

    if (hmac_tx_dma_release_ring_buf(tx_ring) != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_host_ring_release::host buf release failed}");
    }

    hmac_reset_tx_ring(tx_ring);

    oal_spin_unlock(&tx_ring->tx_comp_lock);
    oal_spin_unlock(&tx_ring->tx_lock);
}

static void hmac_tx_tid_info_dump(hmac_tid_info_stru *tid_info)
{
    oam_warning_log4(0, 0, "hmac_tx_tid_info_dump::user[%d] tid[%d] ring_tx_mode[%d] msdu_cnt[%d]",
        tid_info->user_index, tid_info->tid_no,
        oal_atomic_read(&tid_info->ring_tx_mode), oal_atomic_read(&tid_info->tx_ring.msdu_cnt));

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oam_warning_log3(0, 0, "hmac_tx_tid_info_dump::release[%d] rptr[%d] wptr[%d]",
        READ_ONCE(tid_info->tx_ring.release_index), READ_ONCE(tid_info->tx_ring.base_ring_info.read_index),
        READ_ONCE(tid_info->tx_ring.base_ring_info.write_index));
#else
    oam_warning_log3(0, 0, "hmac_tx_tid_info_dump::release[%d] rptr[%d] wptr[%d]",
        tid_info->tx_ring.release_index, tid_info->tx_ring.base_ring_info.read_index,
        tid_info->tx_ring.base_ring_info.write_index);
#endif
}

#define HMAC_WAIT_TX_RING_EMPTY_MAX_CNT 50
void hmac_wait_tx_ring_empty(hmac_tid_info_stru *tid_info)
{
    uint8_t wait_cnt = 0;

    while (oal_atomic_read(&tid_info->tx_ring.msdu_cnt) && wait_cnt < HMAC_WAIT_TX_RING_EMPTY_MAX_CNT) {
        if (++wait_cnt == HMAC_WAIT_TX_RING_EMPTY_MAX_CNT) {
            hmac_tx_tid_info_dump(tid_info);
        }
        oal_mdelay(2);
    }
}

static void hmac_tx_ring_deinit(hmac_user_stru *hmac_user, uint8_t tid)
{
    hmac_tid_info_stru *tid_info = &hmac_user->tx_tid_info[tid];
    hmac_vap_stru *hmac_vap = NULL;

    if (!oal_atomic_read(&tid_info->tx_ring.enabled)) {
        return;
    }

    oal_atomic_set(&tid_info->tx_ring.enabled, OAL_FALSE);
    hmac_tid_schedule_list_delete(tid_info);
    hmac_tx_tid_release(&tid_info->tid_queue);

    if (!hmac_host_ring_tx_enabled()) {
        return;
    }

    hmac_wait_tx_ring_empty(tid_info);

    wlan_pm_wakeup_dev_lock();

    hmac_tid_update_list_delete(tid_info);
    hmac_tx_reg_write_ring_info(tid_info, TID_CMDID_DEL);
    hmac_tx_host_ring_release(&tid_info->tx_ring);

    hmac_vap = mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (hmac_vap == NULL) {
        return;
    }

    hmac_host_ring_tx_resume(hmac_vap, hmac_user, tid_info);
}

void hmac_user_tx_ring_deinit(hmac_user_stru *hmac_user)
{
    uint32_t tid;

    if (hmac_ring_tx_enabled() != OAL_TRUE) {
        return;
    }

    for (tid = 0; tid < WLAN_TID_MAX_NUM; tid++) {
        hmac_tx_ring_deinit(hmac_user, tid);
    }
}

#define get_low_32bits(val)  ((uint32_t)(((uint64_t)(val)) & 0x00000000FFFFFFFFUL))
#define get_high_32bits(val) ((uint32_t)((((uint64_t)(val)) & 0xFFFFFFFF00000000UL) >> 32UL))
static void hmac_sync_host_ba_ring_addr(uint8_t hal_dev_id)
{
    hal_host_ring_ctl_stru *ba_ring_ctl = &hal_get_host_device(hal_dev_id)->host_ba_info_ring;
    host_ba_ring_info_sync_stru host_ba_ring_info = {
        .hal_dev_id = hal_dev_id,
        .size = ba_ring_ctl->entries,
        .lsb = get_low_32bits(ba_ring_ctl->devva),
        .msb = get_high_32bits(ba_ring_ctl->devva),
    };

    (void)hmac_config_send_event(mac_res_get_mac_vap(0), WLAN_CFGID_HOST_BA_INFO_RING_SYN,
                                 sizeof(host_ba_ring_info_sync_stru), (uint8_t *)&host_ba_ring_info);
}

uint32_t hmac_set_tx_ring_device_base_addr(frw_event_mem_stru *frw_mem)
{
    frw_event_stru *frw_event = NULL;
    mac_d2h_tx_ring_base_addr_stru *d2h_tx_ring_base_addr = NULL;

    if (frw_mem == NULL) {
        return OAL_FAIL;
    }

    if (!hmac_host_ring_tx_enabled()) {
        return OAL_SUCC;
    }

    frw_event = frw_get_event_stru(frw_mem);
    d2h_tx_ring_base_addr = (mac_d2h_tx_ring_base_addr_stru *)frw_event->auc_event_data;

    g_tx_ring_device_info_mgmt.tx_ring_device_base_addr = d2h_tx_ring_base_addr->tx_ring_base_addr;

    hal_host_ba_ring_regs_init(HAL_DEVICE_ID_MASTER);
    hal_host_ba_ring_regs_init(HAL_DEVICE_ID_SLAVE);
    hmac_sync_host_ba_ring_addr(HAL_DEVICE_ID_SLAVE);

    return OAL_SUCC;
}
