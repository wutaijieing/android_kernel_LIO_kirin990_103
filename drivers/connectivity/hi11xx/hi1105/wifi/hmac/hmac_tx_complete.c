

#include "hmac_tx_complete.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_host_tx_data.h"
#include "hmac_soft_retry.h"
#include "mac_common.h"
#include "host_hal_device.h"
#include "host_hal_ring.h"
#include "host_hal_ext_if.h"
#include "pcie_host.h"
#include "wlan_spec.h"
#include "host_hal_ops.h"
#include "hmac_vsp_if.h"
#include "hmac_vsp_source.h"
#include "hmac_tid_sche.h"
#include "hmac_tx_switch.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_COMPLETE_C

#define HMAC_TX_MSDU_MAX_CNT 120

static uint32_t hmac_tx_complete_netbuf_retrans(hmac_tid_info_stru *tid_info, oal_netbuf_stru *netbuf)
{
    hal_tx_msdu_dscr_info_stru tx_msdu_dscr_info = { 0 };

    hal_tx_msdu_dscr_info_get(netbuf, &tx_msdu_dscr_info);

    if (oal_likely(tx_msdu_dscr_info.tx_count < HMAC_TX_MSDU_MAX_CNT)) {
        return OAL_FAIL;
    }

    return hmac_tx_soft_retry_process(tid_info, netbuf);
}

static void hmac_tx_complete_netbuf_process(
    hmac_tid_info_stru *tid_info, hmac_msdu_info_ring_stru *tx_ring, un_rw_ptr release_ptr)
{
    oal_netbuf_stru *netbuf = hmac_tx_ring_get_netbuf(tx_ring, release_ptr);
    msdu_info_stru *msdu_info = NULL;

    if (oal_unlikely(netbuf == NULL)) {
        return;
    }

    /*
     * 先进行dma unmap操作, 无效msdu dscr(skb->data)的cache line, 保证软重传获取的数据包含MAC回填值
     * 此时skb->data不在cache中, 软重传访问skb->data会load cache
     */
    msdu_info = hmac_tx_get_ring_msdu_info(tx_ring, release_ptr.st_rw_ptr.bit_rw_ptr);
    hmac_tx_unmap_msdu_dma_addr(tx_ring, msdu_info, netbuf->len);

    /* 高流量(~2.8Gbps)不进行软重传, 避免load cache造成额外开销 */
    if (hmac_tid_ring_tx_opt(tid_info)) {
        hmac_tx_ring_release_netbuf(tx_ring, netbuf, release_ptr.st_rw_ptr.bit_rw_ptr);
        return;
    }

    if (hmac_tx_complete_netbuf_retrans(tid_info, netbuf) == OAL_SUCC) {
        return;
    }

    hmac_tx_ring_release_netbuf(tx_ring, netbuf, release_ptr.st_rw_ptr.bit_rw_ptr);
}


static void hmac_tx_complete_msdu_info_process(hmac_tid_info_stru *tid_info, hmac_msdu_info_ring_stru *tx_ring)
{
    un_rw_ptr release_ptr = { .rw_ptr = tx_ring->release_index };
    un_rw_ptr target_ptr = { .rw_ptr = tx_ring->base_ring_info.read_index };

    while (hmac_tx_rw_ptr_compare(target_ptr, release_ptr) == RING_PTR_GREATER) {
        hmac_tx_complete_netbuf_process(tid_info, tx_ring, release_ptr);

        host_cnt_inc_record_performance(TX_BH_PROC);

        hmac_tx_reset_msdu_info(tx_ring, release_ptr);
        hmac_tx_msdu_ring_inc_release_ptr(tx_ring);

        release_ptr.rw_ptr = tx_ring->release_index;
    }
}


static void hmac_tx_complete_tid_ring_process(hmac_user_stru *hmac_user, hmac_tid_info_stru *tid_info)
{
    hmac_msdu_info_ring_stru *tx_ring = &tid_info->tx_ring;
    hmac_vap_stru *hmac_vap = mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    uint16_t last_release_ptr = tx_ring->release_index;

    if (oal_unlikely(hmac_vap == NULL)) {
        return;
    }

    oal_spin_lock(&tx_ring->tx_comp_lock);

    if (oal_unlikely(hmac_tx_ring_rw_ptr_check(tx_ring) != OAL_TRUE)) {
        oal_spin_unlock(&tx_ring->tx_comp_lock);
        oam_warning_log3(0, OAM_SF_TX, "{hmac_tx_complete_tid_ring_process::tid[%d], rptr[%d], wptr[%d]}",
            tid_info->tid_no, tx_ring->base_ring_info.read_index, tx_ring->base_ring_info.write_index);
        return;
    }

    hmac_tx_complete_msdu_info_process(tid_info, tx_ring);
    hmac_host_ring_tx_resume(hmac_vap, hmac_user, tid_info);

    /*
     * 本次发送完成处理释放过帧才判断是否切换ring, 否则可能存在如下时序(假设ring中有10个帧):
     * 1. 第一次发送完成中断上半部, 此时发帧数5/10
     * 2. 第二次发送完成中断上半部, 此时发帧数10/10
     * 3. 第一次发送完成中断下半部, 读到的read指针为10, netbuf全部释放, ring空, 下发ring切换事件
     * 4. 第二次发送完成中断下半部, 读到的read指针为10, 无netbuf释放, ring空, 下发ring切换事件
     * 导致ring切换事件重复下发
     */
    if (last_release_ptr != tx_ring->release_index) {
        hmac_tx_mode_h2d_switch_process(tid_info);
    }

    oal_spin_unlock(&tx_ring->tx_comp_lock);
}

typedef struct {
    hmac_user_stru *hmac_user;
    uint8_t tid_no;
    uint8_t rptr_valid;
    uint16_t rptr;
} hmac_tx_comp_info_stru;

static uint32_t hmac_tx_complete_update_tid_ring_rptr(
    hmac_tid_info_stru *tid_info, hmac_tx_comp_info_stru *tx_comp_info)
{
    uintptr_t rptr_addr;

    if (tx_comp_info->rptr_valid) {
        tid_info->tx_ring.base_ring_info.read_index = tx_comp_info->rptr;
        return OAL_SUCC;
    }

    rptr_addr = tid_info->tx_ring_device_info->word_addr[BYTE_OFFSET_3];
    if (oal_unlikely(rptr_addr == 0)) {
        return OAL_FAIL;
    }

    wlan_pm_wakeup_dev_lock();

    tid_info->tx_ring.base_ring_info.read_index = (uint16_t)oal_readl(rptr_addr);

    return OAL_SUCC;
}

static inline oal_bool_enum_uint8 hmac_tid_is_host_ring_tx(hmac_tid_info_stru *tid_info)
{
    uint8_t ring_tx_mode = (uint8_t)oal_atomic_read(&tid_info->ring_tx_mode);

    return ring_tx_mode == HOST_RING_TX_MODE || ring_tx_mode == H2D_SWITCHING_MODE;
}

static void hmac_tx_complete_tid_process(hmac_tx_comp_info_stru *tx_comp_info, uint8_t tx_comp_info_cnt)
{
    uint8_t idx;
    hmac_tid_info_stru *tid_info = NULL;

    for (idx = 0; idx < tx_comp_info_cnt; idx++) {
        tid_info = &(tx_comp_info[idx].hmac_user->tx_tid_info[tx_comp_info[idx].tid_no]);

        if (oal_unlikely(!hmac_tid_is_host_ring_tx(tid_info))) {
            continue;
        }

        oal_smp_mb();

        if (oal_unlikely(hmac_tx_complete_update_tid_ring_rptr(tid_info, &tx_comp_info[idx]) != OAL_SUCC)) {
            continue;
        }
#ifdef _PRE_WLAN_FEATURE_VSP
        if (hmac_is_vsp_source_tid(tid_info)) {
            hmac_vsp_tx_complete_process();
            continue;
        }
#endif
        hmac_tx_complete_tid_ring_process(tx_comp_info[idx].hmac_user, tid_info);

        if (hmac_tid_sche_allowed(tid_info) && oal_likely(!hmac_tx_ring_switching(tid_info))) {
            hmac_tid_schedule();
        }
    }
}

#define HMAC_INVALID_BA_INFO_COUNT (hal_host_ba_ring_depth_get() + 1)


static inline uint16_t hmac_tx_get_ba_info_count(hal_host_device_stru *hal_dev)
{
    uint16_t ba_info_count = 0;
    hal_host_ring_ctl_stru *ba_info_ring = NULL;

    if (hal_dev == NULL) {
        return ba_info_count;
    }
    ba_info_ring = &hal_dev->host_ba_info_ring;
    if (hal_ring_get_entry_count(ba_info_ring, &ba_info_count) != OAL_SUCC) {
        return HMAC_INVALID_BA_INFO_COUNT;
    }

    return ba_info_count;
}


static void hmac_tx_get_ba_info(
    hal_host_device_stru *hal_device, uint16_t ba_info_cnt, oal_dlist_head_stru *ba_info_list)
{
    uint32_t index;
    uint8_t ba_info_data[BA_INFO_DATA_SIZE];
    hal_tx_ba_info_stru *ba_info = NULL;
    hal_host_ring_ctl_stru *ba_info_ring = NULL;

    if (hal_device == NULL) {
        return;
    }
    ba_info_ring = &hal_device->host_ba_info_ring;
    for (index = 0; index < ba_info_cnt; index++) {
        if (oal_unlikely(hal_ring_get_entries(ba_info_ring, ba_info_data, 1) != OAL_SUCC)) {
            oam_error_log3(0, OAM_SF_TX, "{hmac_tx_get_ba_info:: count[%d] read[%d] write[%d]}",
                ba_info_cnt, ba_info_ring->un_read_ptr.read_ptr, ba_info_ring->un_write_ptr.write_ptr);
            return;
        }

        ba_info = (hal_tx_ba_info_stru *)oal_memalloc(sizeof(hal_tx_ba_info_stru));
        if (oal_unlikely(ba_info == NULL)) {
            oam_error_log0(0, OAM_SF_TX, "{hmac_tx_get_ba_info::alloc ba_info failed}");
            break;
        }

        hal_tx_ba_info_dscr_get(ba_info_data, BA_INFO_DATA_SIZE, ba_info);
        oal_dlist_add_tail(&ba_info->entry, ba_info_list);
    }

    /* 尽快更新ba info ring rptr, 避免ring满, 后续再进行其他ba info处理 */
    hal_ring_set_sw2hw(ba_info_ring);

    return;
}

static inline oal_bool_enum_uint8 hmac_tx_complete_ba_info_valid(hal_tx_ba_info_stru *ba_info)
{
    return ba_info->ba_info_vld && ba_info->tid_no < WLAN_TIDNO_BUTT && ba_info->user_id < HAL_MAX_TX_BA_LUT_SIZE;
}

static void hmac_tx_complete_record_ba_info(
    hmac_user_stru *hmac_user, hal_tx_ba_info_stru *ba_info, hmac_tx_comp_info_stru *tx_comp_info, uint8_t *info_cnt)
{
    uint8_t idx;

    for (idx = 0; idx < *info_cnt; idx++) {
        if (hmac_user->st_user_base_info.us_assoc_id == tx_comp_info[idx].hmac_user->st_user_base_info.us_assoc_id &&
            ba_info->tid_no == tx_comp_info[idx].tid_no) {
            return;
        }
    }

    tx_comp_info[*info_cnt].hmac_user = hmac_user;
    tx_comp_info[*info_cnt].tid_no = ba_info->tid_no;
    tx_comp_info[*info_cnt].rptr_valid = ba_info->rptr_valid;
    tx_comp_info[*info_cnt].rptr = ba_info->rptr;
    (*info_cnt)++;
}

static void hmac_tx_complete_process_ba_info(
    hal_tx_ba_info_stru *ba_info, hmac_tx_comp_info_stru *tx_comp_info, uint8_t *tx_comp_info_cnt)
{
    hmac_user_stru *hmac_user = NULL;

    if (oal_unlikely(!hmac_tx_complete_ba_info_valid(ba_info))) {
        return;
    }

    hmac_user = hmac_get_user_by_lut_index(ba_info->user_id);
    if (oal_unlikely(hmac_user == NULL)) {
        return;
    }

    /* 记录待处理user tid, 筛选掉重复的user tid ba info, 避免反复处理 */
    hmac_tx_complete_record_ba_info(hmac_user, ba_info, tx_comp_info, tx_comp_info_cnt);
}

#define HMAC_BA_INFO_COUNT 64


static void hmac_tx_complete_process_ba_info_list(oal_dlist_head_stru *ba_info_list)
{
    uint8_t tx_comp_info_cnt = 0;
    hmac_tx_comp_info_stru tx_comp_info[HMAC_BA_INFO_COUNT];
    hal_tx_ba_info_stru *ba_info = NULL;
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    oal_dlist_search_for_each_safe(entry, entry_tmp, ba_info_list) {
        ba_info = (hal_tx_ba_info_stru *)oal_dlist_get_entry(entry, hal_tx_ba_info_stru, entry);

        if (oal_likely(tx_comp_info_cnt < HMAC_BA_INFO_COUNT)) {
            hmac_tx_complete_process_ba_info(ba_info, tx_comp_info, &tx_comp_info_cnt);
        }

        oal_dlist_delete_entry(&ba_info->entry);
        oal_free(ba_info);
    }

    hmac_tx_complete_tid_process(tx_comp_info, tx_comp_info_cnt);
}

OAL_STATIC void hmac_tx_complete_process(hal_host_device_stru *hal_device)
{
    oal_dlist_head_stru ba_info_list;
    uint16_t ba_info_cnt = hmac_tx_get_ba_info_count(hal_device);
    if (oal_unlikely(ba_info_cnt == 0 || ba_info_cnt >= HMAC_INVALID_BA_INFO_COUNT)) {
        return;
    }

    oal_dlist_init_head(&ba_info_list);
    hmac_tx_get_ba_info(hal_device, ba_info_cnt, &ba_info_list);
    hmac_tx_complete_process_ba_info_list(&ba_info_list);
}

hmac_tx_comp_stru g_tx_comp_mgmt = { .tx_comp_thread = NULL};

void hmac_tx_comp_init(void)
{
    uint8_t hal_dev_id;

    for (hal_dev_id = 0; hal_dev_id < WLAN_DEVICE_MAX_NUM_PER_CHIP; hal_dev_id++) {
        hmac_clear_tx_comp_triggered(hal_dev_id);
    }
}

static void hmac_tx_comp_thread_process(void)
{
    uint8_t hal_dev_id;

    host_start_record_performance(TX_BH_PROC);

    for (hal_dev_id = 0; hal_dev_id < WLAN_DEVICE_MAX_NUM_PER_CHIP; hal_dev_id++) {
        if (!hmac_get_tx_comp_triggered(hal_dev_id)) {
            continue;
        }

        hmac_clear_tx_comp_triggered(hal_dev_id);

        if (!oal_pcie_link_state_up(HCC_EP_WIFI_DEV)) {
            oam_warning_log0(0, OAM_SF_TX, "{hmac_hal_dev_tx_complete_process::pcie link down!}");
            continue;
        }

        hmac_tx_complete_process(hal_get_host_device(hal_dev_id));
    }

    host_end_record_performance(host_cnt_get_record_performance(TX_BH_PROC), TX_BH_PROC);
}

int32_t hmac_tx_comp_thread(void *p_data)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct sched_param param;

    param.sched_priority = 97;
    oal_set_thread_property(current, SCHED_FIFO, &param, -10); /* set nice -10 */

    allow_signal(SIGTERM);

    while (oal_likely(!down_interruptible(&g_tx_comp_mgmt.tx_comp_sema))) {
        if (oal_kthread_should_stop()) {
            break;
        }

        hmac_tx_comp_thread_process();
    }
#endif

    return OAL_SUCC;
}
