

#include "pcie_linux.h"
#include "pcie_host.h"
#include "oal_util.h"
#include "oal_net.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"

#include "mac_frame.h"
#include "host_hal_ops.h"
#include "host_hal_ring.h"
#include "host_hal_dscr.h"
#include "host_hal_device.h"
#include "host_irq_shenkuo.h"
#include "host_mac_shenkuo.h"
#include "host_dscr_shenkuo.h"
#include "host_hal_ops_shenkuo.h"

#include "hmac_rx_data.h"
#include "hmac_user.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "hmac_tx_complete.h"
#include "host_hal_ext_if.h"
#ifdef _PRE_WLAN_FEATURE_CSI
#include "host_csi_shenkuo.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_IRQ_SHENKUO_C

#define oal_make_word64(lsw, msw) ((((uint64_t)(msw) << 32) & 0xFFFFFFFF00000000) | (lsw))


static uint32_t shenkuo_get_skb_from_ring_entry(uint64_t hw_dscr_addr, hal_host_ring_ctl_stru *ring_ctl,
    hal_host_device_stru *hal_dev, oal_netbuf_stru **pp_skb)
{
    int32_t                      ret;
    dma_addr_t                   host_iova   = 0;
    uint32_t                     pre_num     = 0;
    hal_host_rx_alloc_list_stru *alloc_list  = &hal_dev->host_rx_normal_alloc_list; /* 目前仅用normal_list */

    if (oal_unlikely(hw_dscr_addr == 0)) {
        oam_error_log1(0, OAM_SF_RX, "{hal_get_skb_from_ring_entry:rd_idx[%d], addr is zero!}",
            ring_ctl->un_read_ptr.st_read_ptr.bit_read_ptr);
        return OAL_FAIL;
    }

    if (hal_dev->dscr_prev == hw_dscr_addr) {
        oam_error_log1(0, OAM_SF_RX, "{hal_get_skb_from_ring_entry:duplicate dscr 0x%x!}", hw_dscr_addr);
    }

    hal_dev->dscr_prev = hw_dscr_addr;
    ret = pcie_if_devva_to_hostca(HCC_EP_WIFI_DEV, hw_dscr_addr, (uint64_t *)&host_iova);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_RX, "{hal_get_skb_from_ring_entry:devva2hostca fail.}");
        return OAL_FAIL;
    }

    /* 中断下半部补充netbuf可能也会操作此链表 */
    oal_spin_lock(&alloc_list->lock);
    *pp_skb = hal_alloc_list_delete_netbuf(hal_dev, alloc_list, host_iova, &pre_num);
    oal_spin_unlock(&alloc_list->lock);

    if (*pp_skb == NULL) {
        oam_error_log0(hal_dev->device_id, OAM_SF_RX, "shenkuo_get_skb_from_ring_entry:not found skb");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


uint32_t shenkuo_host_rx_ring_entry_get(hal_host_device_stru *hal_dev,
    hal_host_ring_ctl_stru *ring_ctl, oal_netbuf_head_stru *netbuf_head)
{
    uint16_t             count;
    uint64_t             hw_dscr_addr;
    uint32_t             ret;
    oal_netbuf_stru     *netbuf = NULL;

    if (oal_unlikely(oal_any_null_ptr3(hal_dev, ring_ctl, netbuf_head))) {
        oam_error_log0(0, OAM_SF_RX, "{hal_host_rx_ring_entry_get::input para null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 从HAL获取ring元素的个数 */
    ret = hal_ring_get_entry_count(ring_ctl, &count);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_RX, "{hal_host_rx_ring_entry_get::return code %d.}", ret);
        return ret;
    }

    /* 循环处理每一个元素 */
    while (count) {
        hw_dscr_addr = 0;
        if (hal_comp_ring_wait_valid(ring_ctl) != OAL_SUCC) {
            break;
        }

        /* 读取ring上的元素 */
        ret = hal_ring_get_entries(ring_ctl, (uint8_t *)&hw_dscr_addr, 1);
        if (oal_unlikely(ret != OAL_SUCC)) {
            oam_error_log1(0, OAM_SF_RX, "{hal_host_rx_ring_entry_get::return code:%d.}", ret);
            break;
        }

        /* 从物理地址获取skb地址 */
        ret = shenkuo_get_skb_from_ring_entry(hw_dscr_addr,  ring_ctl, hal_dev, &netbuf);
        if (oal_unlikely(ret != OAL_SUCC)) {
            count--;
            continue;
        }

        oal_netbuf_add_to_list_tail(netbuf, netbuf_head);
        count--;
    }

    /* 更新complete ring的rptr指针 */
    return hal_ring_set_sw2hw(ring_ctl);
}


void shenkuo_host_rx_mpdu_que_push(hal_rx_mpdu_que *rx_mpdu,
    oal_netbuf_head_stru *netbuf_head, uint32_t *ori_skb_num)
{
    oal_netbuf_head_stru *rx_que = NULL;
    unsigned long         lock_flag;

    oal_spin_lock_irq_save(&rx_mpdu->st_spin_lock, &lock_flag);
    rx_que = &(rx_mpdu->ast_rx_mpdu_list[rx_mpdu->cur_idx]);
    *ori_skb_num = oal_netbuf_get_buf_num(rx_que);
    oal_netbuf_queue_splice_tail_init(netbuf_head, rx_que);
    oal_spin_unlock_irq_restore(&rx_mpdu->st_spin_lock, &lock_flag);
}
OAL_STATIC void shenkuo_irq_host_rx_complete_notify(hal_host_device_stru *hal_device)
{
    frw_event_stru           *event = NULL;
    frw_event_mem_stru       *event_mem = NULL;
    hal_host_rx_event        *wlan_rx_event = NULL;

    event_mem = frw_event_alloc_m(sizeof(hal_host_rx_event));
    if (event_mem == NULL) {
        return;
    }

    event = frw_get_event_stru(event_mem);
    frw_event_hdr_init(&(event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_DDR_DRX,
                       HAL_WLAN_DDR_DRX_EVENT_SUBTYPE,
                       sizeof(hal_host_rx_event),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       0, hal_device->device_id, 0);

    wlan_rx_event = (hal_host_rx_event *)event->auc_event_data;
    wlan_rx_event->hal_dev = hal_device;
    frw_event_dispatch_event(event_mem);
    frw_event_free_m(event_mem);
    return;
}

OAL_STATIC void shenkuo_irq_host_rx_complete_isr(hal_host_device_stru *hal_device, uint32_t mask)
{
    uint32_t                     ret;
    oal_netbuf_head_stru         rx_msdu_que;
    hal_rx_mpdu_que             *rx_mpdu = NULL;
    uint32_t                     ori_skb_num = 0;

    host_start_record_performance(RX_ISR_PROC);

    if (!hal_device->inited) {
        oam_error_log1(0, OAM_SF_RX, "irq_host_mac_isr:: device[%d] %d not inited", hal_device->device_id);
        shenkuo_clear_host_mac_int_status(hal_device, mask);
        return;
    }
    hal_host_rx_add_buff(hal_device, HAL_RX_DSCR_NORMAL_PRI_QUEUE);
    rx_mpdu = &(hal_device->st_rx_mpdu);
    oal_netbuf_list_head_init(&rx_msdu_que);
    /* 从complete ring上读取元素，并存入临时netbuf上报链表 */
    ret = shenkuo_host_rx_ring_entry_get(hal_device, &hal_device->st_host_rx_complete_ring, &rx_msdu_que);
    if (ret != OAL_SUCC) {
        shenkuo_clear_host_mac_int_status(hal_device, mask);
        return;
    }

    /* 将临时链表中的元素存入乒乓队列 */
    shenkuo_host_rx_mpdu_que_push(rx_mpdu, &rx_msdu_que, &ori_skb_num);
    shenkuo_clear_host_mac_int_status(hal_device, mask);

    /* 如果乒乓链表没有数据，抛事件到下半部处理 */
    if (!ori_skb_num) {
        shenkuo_irq_host_rx_complete_notify(hal_device);
    } else {
        oam_info_log1(0, OAM_SF_RX, "irq_host_mac_isr:: ori_skb_num[%d] %d not zero", ori_skb_num);
    }

    host_end_record_performance(ori_skb_num, RX_ISR_PROC);
}


void shenkuo_irq_host_common_timer_isr(hal_host_device_stru *hal_dev, uint32_t mask)
{
#ifdef _PRE_WLAN_FEATURE_VSP
    frw_event_mem_stru *event_mem = NULL;
    frw_event_stru *common_timeout_event = NULL;
    hal_host_common_timerout_stru *common_timerout = NULL;
    hal_mac_common_timer *timer = NULL;
    shenkuo_clear_host_mac_int_status(hal_dev, mask);

    timer = shenkuo_get_host_mac_common_timer_ptr(16);
    if (timer == NULL) {
        oam_warning_log0(0, 0, "{shenkuo_irq_host_common_timer_isr::timer not register}");
        return;
    }
    if (timer->func) {
        timer->func(timer);
    }
    event_mem = frw_event_alloc_m(sizeof(hal_host_common_timerout_stru));
    if (event_mem == NULL) {
        oam_error_log0(0, OAM_SF_FTM, "{shenkuo_irq_host_common_timer_isr::alloc event_mem failed.}");
        return;
    }
    common_timeout_event = frw_get_event_stru(event_mem);
    frw_event_hdr_init(&(common_timeout_event->st_event_hdr), FRW_EVENT_TYPE_HOST_DDR_DRX,
                       HAL_WLAN_COMMON_TIMEOUT_IRQ_EVENT_SUBTYPE,
                       sizeof(hal_host_location_isr_stru), FRW_EVENT_PIPELINE_STAGE_0,
                       0, hal_dev->device_id, 0);

    common_timerout = (hal_host_common_timerout_stru *)common_timeout_event->auc_event_data;
    common_timerout->hal_device_id = hal_dev->device_id;
    common_timerout->chip_id = hal_dev->chip_id;
    common_timerout->timer = timer;
    frw_event_dispatch_event(event_mem);
    frw_event_free_m(event_mem);
#endif
}

#ifdef _PRE_WLAN_FEATURE_CSI

static void shenkuo_host_ftm_csi_event(hal_host_device_stru *hal_device, hal_host_location_isr_stru *ftm_csi_isr_event)
{
    frw_event_mem_stru *event_mem = NULL;
    frw_event_stru *hmac_ftm_csi_event = NULL;

    /* 抛加入完成事件到WAL */
    event_mem = frw_event_alloc_m(sizeof(hal_host_location_isr_stru));
    if (event_mem == NULL) {
        oam_error_log0(0, OAM_SF_FTM, "{shenkuo_host_ftm_csi_event::alloc event_mem failed.}\r\n");
        return;
    }
    ftm_csi_isr_event->chip_id = hal_device->chip_id;
    ftm_csi_isr_event->hal_device_id = hal_device->device_id;
    /* 填写事件 */
    hmac_ftm_csi_event = frw_get_event_stru(event_mem);

    /* 填写事件头 */
    frw_event_hdr_init(&(hmac_ftm_csi_event->st_event_hdr), FRW_EVENT_TYPE_HOST_DDR_DRX,
                       HAL_WLAN_FTM_IRQ_EVENT_SUBTYPE,
                       sizeof(hal_host_location_isr_stru), FRW_EVENT_PIPELINE_STAGE_0,
                       0, hal_device->device_id, 0);

    if (memcpy_s(hmac_ftm_csi_event->auc_event_data, sizeof(hal_host_location_isr_stru), ftm_csi_isr_event,
        sizeof(hal_host_location_isr_stru)) != EOK) {
        oam_error_log0(0, OAM_SF_FTM, "shenkuo_host_ftm_csi_event::memcpy fail!");
        frw_event_free_m(event_mem);
        return;
    }

    /* 分发事件 */
    frw_event_dispatch_event(event_mem);
    frw_event_free_m(event_mem);
}

static uint32_t shenkuo_host_ftm_csi_addr_report(uint64_t info_addr_lsb, uint64_t info_addr_msb, uint64_t *addr)
{
    *addr = oal_make_word64(oal_readl(info_addr_lsb), oal_readl(info_addr_msb));

    if (*addr == 0) {
        oam_warning_log0(0, OAM_SF_FTM, "{shenkuo_host_ftm_csi_addr_report:get addr fail.}");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

void shenkuo_irq_host_ftm_csi_isr(hal_host_device_stru *hal_dev, uint32_t mask)
{
    hal_host_location_isr_stru ftm_csi_isr_event;

    shenkuo_clear_host_mac_int_status(hal_dev, mask);

    ftm_csi_isr_event.ftm_csi_info = shenkuo_get_host_ftm_csi_locationinfo(hal_dev);

    /* 只用到低两位bit. */
    ftm_csi_isr_event.ftm_csi_info &= SHENKUO_MAC_RPT_LOCATION_INFO_MASK_MASK;

    // 考虑ftm、csi全上报的情况，取flag bit0 bit1分别进行判断 bit0:csi、bit1:ftm
    if (!ftm_csi_isr_event.ftm_csi_info) {
        oam_error_log0(0, OAM_SF_FTM, "shenkuo_irq_host_ftm_csi_isr::no ftm csi rpt!");
        return;
    }

    if (ftm_csi_isr_event.ftm_csi_info & BIT0) { // CSI上报
        if (shenkuo_host_ftm_csi_addr_report(hal_dev->csi_regs.csi_info_lsb,
            hal_dev->csi_regs.csi_info_msb, &ftm_csi_isr_event.csi_info_addr) != OAL_SUCC) {
            return;
        }
        shenkuo_host_ftm_csi_event(hal_dev, &ftm_csi_isr_event);
    }

    if (ftm_csi_isr_event.ftm_csi_info & BIT1) { // FTM上报
        if (shenkuo_host_ftm_csi_addr_report(hal_dev->ftm_regs.ftm_info_lsb,
            hal_dev->ftm_regs.ftm_info_msb, &ftm_csi_isr_event.ftm_info_addr) != OAL_SUCC) {
            return;
        }
        shenkuo_host_ftm_csi_event(hal_dev, &ftm_csi_isr_event);
    }
}
#endif

/*
 * 功能描述   : irq_host_tx_complete_isr中断服务例程
 * 修改历史   :
 * 1.日    期   : 2020年05月10日
 *   作    者   : lingxuemeng
 *   修改内容   : 新生成函数
 */
void shenkuo_irq_host_tx_complete_isr(hal_host_device_stru *hal_dev, uint32_t mask)
{
    host_start_record_performance(TX_ISR_PROC);
    shenkuo_clear_host_mac_int_status(hal_dev, mask);

    if (!hal_dev->host_ba_ring_regs_inited) {
        oam_error_log0(0, OAM_SF_RX, "irq_host_tx_complete_isr:: host tx int not inited.\n");
        return;
    }

    hmac_set_tx_comp_triggered(hal_dev->device_id);
    hmac_tx_comp_scheduled();

    host_end_record_performance(1, TX_ISR_PROC);
}

void shenkuo_irq_host_mac_isr(hal_host_device_stru *hal_dev)
{
    uint32_t               state = 0;
    uint32_t               mask = 0;
    host_intr_status_union int_state;

    if (hal_dev == NULL) {
        oal_io_print("irq_host_mac_isr:: input para err.\n");
        return;
    }
    if (!hal_dev->intr_inited) {
        oam_error_log3(0, OAM_SF_RX, "{irq_host_mac_isr error, dev_id %d, tx_inited %d, status 0x%x.}",
            hal_dev->device_id, hal_dev->intr_inited, hal_dev->mac_regs.irq_status_mask_reg);
        return;
    }

    /* 读取MAC中断状态,中断mask寄存器 */
    shenkuo_get_host_mac_int_status(hal_dev, &state);
    shenkuo_get_host_mac_int_mask(hal_dev, &mask);

    /* 一次处理所有中断 */
    state &= ~mask;
    int_state.u32 = state;
    while (state) {
        /* 保留最低置1位，其它位为0 */
        int_state.u32 = (uint32_t)((uint32_t)(-state) & state);  //lint !e501
        state &= ~((uint32_t)int_state.u32);
        if (int_state.bits.rpt_host_intr_rx_complete != 0) {
            /* 接收数据报文中断 */
            shenkuo_irq_host_rx_complete_isr(hal_dev, int_state.u32);
        } else if (int_state.bits.rpt_host_intr_tx_complete) {
            /* 发送完成上报中断 */
            shenkuo_irq_host_tx_complete_isr(hal_dev, int_state.u32);
        } else if (int_state.bits.rpt_host_intr_ba_info_ring_overrun) {
            /* BA info ring满中断, 需要加速BA info ring的处理 */
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: BA INFO RING OVERRUN.}\r\n");
            shenkuo_irq_host_tx_complete_isr(hal_dev, int_state.u32);
#ifdef _PRE_WLAN_FEATURE_CSI
        } else if (int_state.bits.rpt_host_intr_location_complete != 0) {
            shenkuo_irq_host_ftm_csi_isr(hal_dev, int_state.u32);
#endif
        } else if (int_state.bits.rpt_host_intr_norm_ring_empty != 0) {
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: NORM FREE RING EMPTY.}\r\n");
        } else if (int_state.bits.rpt_host_intr_small_ring_empty != 0) {
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: SMALL FREE RING EMPTY.}\r\n");
        } else if (int_state.bits.rpt_host_intr_ba_win_exceed != 0) {
            /* 接收到的BA窗口越界异常中断 */
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: BA WIN EXCEED.}\r\n");
        } else if (int_state.bits.rpt_host_intr_rx_ppdu_desc != 0) {
            /* RX PPDU描述符Complete Ring上报中断 */
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: INTR RX PPDU DESC.}\r\n");
        } else if (int_state.bits.rpt_host_intr_data_ring_overrun != 0) {
            /* 数据帧Complete Ring满写异常中断 */
            oam_warning_log0(0, OAM_SF_ANY, "{shenkuo_irq_host_mac_isr:: DATA RING OVERRUN.}\r\n");
        } else if (int_state.bits.rpt_host_intr_common_timer != 0) {
            shenkuo_irq_host_common_timer_isr(hal_dev, int_state.u32);
            /* 通用计时器中断 */
        } else {
            oam_warning_log1(0, OAM_SF_ANY, "{hal_irq_host_mac_isr error, state.u32:0x%x.}", int_state.u32);
        }
    }
    shenkuo_clear_host_mac_int_status(hal_dev, int_state.u32);
}


void shenkuo_mac_irq_host_all(void *p_dev)
{
    hal_host_device_stru *hal_dev = (hal_host_device_stru *)p_dev;
    shenkuo_irq_host_mac_isr(hal_dev);
}


void shenkuo_irq_host_phy_isr(hal_host_device_stru *hal_dev)
{
    uint32_t               state = 0;
    uint32_t               mask = 0;
    phy_intr_rpt_union     int_state;

    if (hal_dev == NULL) {
        oal_io_print("host_phy_isr input para err\n");
        return;
    }
    if (!hal_dev->intr_inited) {
        oam_error_log3(0, OAM_SF_RX, "{irq_host_phy_isr error, dev_id %d, tx_inited %d, status 0x%x.}",
            hal_dev->device_id, hal_dev->intr_inited, hal_dev->phy_regs.irq_status_reg);
        return;
    }
    /* 读取MAC中断状态寄存器 */
    shenkuo_get_host_phy_int_status(hal_dev, &state);

    /* 读取中断mask寄存器 */
    shenkuo_get_host_phy_int_mask(hal_dev, &mask);

    state &= ~mask;
    int_state.u32 = state;
    shenkuo_clear_host_phy_int_status(hal_dev, int_state.u32);
}

void shenkuo_phy_irq_host_all(void *p_dev)
{
    hal_host_device_stru *hal_dev = (hal_host_device_stru *)p_dev;
    shenkuo_irq_host_phy_isr(hal_dev);
}


void shenkuo_host_chip_irq_init(void)
{
    static pcie_wlan_callback_group_stru st_isr_cb;

    memset_s(&st_isr_cb, sizeof(pcie_wlan_callback_group_stru),
        0, sizeof(pcie_wlan_callback_group_stru));

    st_isr_cb.pcie_mac_2g_isr_callback.para = (void *)hal_get_host_device(0);
    st_isr_cb.pcie_mac_2g_isr_callback.pf_func = shenkuo_mac_irq_host_all;
    st_isr_cb.pcie_phy_2g_isr_callback.para = (void *)hal_get_host_device(0);
    st_isr_cb.pcie_phy_2g_isr_callback.pf_func = shenkuo_phy_irq_host_all;

    st_isr_cb.pcie_mac_5g_isr_callback.para = (void *)hal_get_host_device(1);
    st_isr_cb.pcie_mac_5g_isr_callback.pf_func = shenkuo_mac_irq_host_all;
    st_isr_cb.pcie_phy_5g_isr_callback.para = (void *)hal_get_host_device(1);
    st_isr_cb.pcie_phy_5g_isr_callback.pf_func = shenkuo_phy_irq_host_all;

    pcie_wlan_func_register(&st_isr_cb);
    oam_warning_log0(0, OAM_SF_RX, "{host chip irq registered succ.}");
}
