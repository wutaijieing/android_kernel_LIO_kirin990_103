

#ifndef __HMAC_TX_DATA_H__
#define __HMAC_TX_DATA_H__


/* 1 其他头文件包含 */
#include "mac_frame.h"
#include "mac_frame_inl.h"
#include "mac_common.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_mgmt_classifier.h"
#include "mac_resource.h"
#include "mac_mib.h"
#include "wlan_chip_i.h"
#include "hmac_multi_netbuf_amsdu.h"
#include "hmac_tx_data_macro.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_function.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_DATA_H

typedef struct {
    uint8_t host_ring_tx_switch;
    uint8_t device_ring_tx_switch;
    uint8_t rx_switch;
    uint8_t device_loop_sched;
    uint8_t tx_ring_switch;
    uint8_t host_update_wptr;       /* skb入ring时wptr更新方式: PCIE直接写或抛事件 */
    uint8_t ring_switch_independent; /* Tx ring与Rx Ring同源切换 */
    uint8_t soft_irq_sched;         /* 软中断调度使能 */
    uint8_t netbuf_alloc_opt;
    uint8_t tx_netbuf_limit;
} wlan_trx_switch_stru;
extern wlan_trx_switch_stru g_trx_switch;
/* 9 OTHERS定义 */
static inline uint8_t hmac_host_ring_tx_enabled(void)
{
    return g_trx_switch.host_ring_tx_switch;
}

static inline uint8_t hmac_device_ring_tx_enabled(void)
{
    return g_trx_switch.device_ring_tx_switch;
}

static inline uint8_t hmac_device_loop_sched_enabled(void)
{
    return g_trx_switch.device_loop_sched;
}

static inline uint8_t hmac_tx_ring_switch_enabled(void)
{
    return g_trx_switch.tx_ring_switch;
}
static inline uint8_t hmac_rx_ring_switch_enabled(void)
{
    return g_trx_switch.rx_switch;
}
static inline uint8_t hmac_tx_ring_soft_irq_sched_enabled(void)
{
    return g_trx_switch.soft_irq_sched;
}

static inline uint8_t hmac_host_update_wptr_enabled(void)
{
    return g_trx_switch.host_update_wptr;
}

static inline uint8_t hmac_tx_ring_switch_sync_mode(void)
{
    return g_trx_switch.ring_switch_independent;
}

static inline oal_bool_enum_uint8 hmac_ring_tx_enabled(void)
{
    return (hmac_host_ring_tx_enabled() || hmac_device_ring_tx_enabled()) ? OAL_TRUE : OAL_FALSE;
}

static inline oal_bool_enum_uint8 hmac_device_ring_tx_only(void)
{
    return (!hmac_host_ring_tx_enabled() && hmac_device_ring_tx_enabled()) ? OAL_TRUE : OAL_FALSE;
}

void hmac_tx_set_frame_htc(hmac_vap_stru *pst_hmac_vap,
    mac_tx_ctl_stru *pst_tx_ctl, hmac_user_stru *pst_hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4);
void hmac_tx_set_qos_frame_htc(hmac_vap_stru *pst_hmac_vap,
    mac_tx_ctl_stru *pst_tx_ctl, hmac_user_stru *pst_hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4);


OAL_STATIC OAL_INLINE oal_netbuf_stru *hmac_tx_get_next_mpdu(oal_netbuf_stru *pst_buf, uint8_t uc_netbuf_num)
{
    oal_netbuf_stru *pst_next_buf = NULL;
    uint32_t netbuf_index;

    if (oal_unlikely(pst_buf == NULL)) {
        return NULL;
    }

    pst_next_buf = pst_buf;
    for (netbuf_index = 0; netbuf_index < uc_netbuf_num; netbuf_index++) {
        pst_next_buf = oal_netbuf_list_next(pst_next_buf);
    }

    return pst_next_buf;
}


OAL_STATIC OAL_INLINE void hmac_tx_netbuf_list_enqueue(oal_netbuf_head_stru *pst_head,
    oal_netbuf_stru *pst_buf, uint8_t uc_netbuf_num)
{
    uint32_t netbuf_index;
    oal_netbuf_stru *pst_buf_next = NULL;

    if (oal_unlikely(oal_any_null_ptr2(pst_head, pst_buf))) {
        return;
    }

    for (netbuf_index = 0; netbuf_index < uc_netbuf_num; netbuf_index++) {
        pst_buf_next = oal_netbuf_list_next(pst_buf);
        oal_netbuf_add_to_list_tail(pst_buf, pst_head);
        pst_buf = pst_buf_next;
    }
}


OAL_STATIC OAL_INLINE void hmac_tx_set_frame_ctrl(uint32_t qos,
    mac_tx_ctl_stru *pst_tx_ctl, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4)
{
    mac_ieee80211_qos_htc_frame_stru *pst_hdr = NULL;
    if (qos == HMAC_TX_BSS_QOS) {
        /* 设置帧控制字段 */
        mac_hdr_set_frame_control((uint8_t *)pst_hdr_addr4, (WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_QOS));

        /* 更新帧头长度 */
        if (OAL_FALSE == MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)) {
            pst_hdr = (mac_ieee80211_qos_htc_frame_stru *)pst_hdr_addr4;
            /* 设置QOS控制字段 */
            pst_hdr->bit_qc_tid = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
            pst_hdr->bit_qc_eosp = 0;
            pst_hdr->bit_qc_ack_polocy = MAC_GET_CB_ACK_POLACY(pst_tx_ctl);
            pst_hdr->bit_qc_amsdu = MAC_GET_CB_IS_AMSDU(pst_tx_ctl);
            pst_hdr->qos_control.bit_qc_txop_limit = 0;
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_FRAME_LEN;
        } else {
            /* 设置QOS控制字段 */
            pst_hdr_addr4->bit_qc_tid = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
            pst_hdr_addr4->bit_qc_eosp = 0;
            pst_hdr_addr4->bit_qc_ack_polocy = MAC_GET_CB_ACK_POLACY(pst_tx_ctl);
            pst_hdr_addr4->bit_qc_amsdu = MAC_GET_CB_IS_AMSDU(pst_tx_ctl);
            pst_hdr_addr4->qos_control.qc_txop_limit = 0;
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_4ADDR_FRAME_LEN;
        }
    } else {
        /* 设置帧控制字段 */
        mac_hdr_set_frame_control((uint8_t *)pst_hdr_addr4, WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_DATA);

        /* 非QOS数据帧帧控制字段设置 */
        if (MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)) {
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_4ADDR_FRAME_LEN;
        } else {
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_FRAME_LEN;
        }
    }
}

#ifdef _PRE_WLAN_FEATURE_HIEX

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_enable_himit_htc(hmac_user_stru *user, mac_tx_ctl_stru *tx_ctl)
{
    return ((MAC_USER_HIMIT_ENABLED(&user->st_user_base_info) == OAL_TRUE) &&
            MAC_GET_CB_IS_GAME_MARKED_FRAME(tx_ctl) && ((MAC_GET_CB_ACK_POLACY(tx_ctl) == WLAN_TX_NORMAL_ACK) ||
            (MAC_GET_CB_ACK_POLACY(tx_ctl) == WLAN_TX_BLOCK_ACK))) ? OAL_TRUE : OAL_FALSE;
}
#endif

OAL_STATIC OAL_INLINE void hmac_tx_set_sta_mode_address(hmac_vap_stru *vap, hmac_user_stru *user,
    mac_tx_ctl_stru *tx_ctl, mac_ether_header_stru *ether_hdr, mac_ieee80211_qos_htc_frame_addr4_stru *hdr)
{
    /* From DS标识位设置 */
    mac_hdr_set_from_ds((uint8_t *)hdr, 0);

    /* to DS标识位设置 */
    mac_hdr_set_to_ds((uint8_t *)hdr, 1);

    /* Set Address1 field in the WLAN Header with BSSID */
    oal_set_mac_addr(hdr->auc_address1, user->st_user_base_info.auc_user_mac_addr);

    if (ether_hdr->us_ether_type == oal_byteorder_host_to_net_uint16(ETHER_LLTD_TYPE)) {
        /* Set Address2 field in the WLAN Header with the source address */
        oal_set_mac_addr(hdr->auc_address2, (uint8_t *)ether_hdr->auc_ether_shost);
    } else {
        /* Set Address2 field in the WLAN Header with the source address */
        oal_set_mac_addr(hdr->auc_address2, mac_mib_get_StationID(&vap->st_vap_base_info));
    }

    if (MAC_GET_CB_IS_AMSDU(tx_ctl)) { /* AMSDU情况，地址3填写BSSID */
        /* Set Address3 field in the WLAN Header with the BSSID */
        oal_set_mac_addr(hdr->auc_address3, user->st_user_base_info.auc_user_mac_addr);
    } else {
        /* Set Address3 field in the WLAN Header with the destination address */
        oal_set_mac_addr(hdr->auc_address3, (uint8_t *)ether_hdr->auc_ether_dhost);
    }
}

static inline void hmac_tx_set_ap_address(hmac_vap_stru *vap, mac_tx_ctl_stru *tx_ctl, mac_ether_header_stru *ether_hdr,
    mac_ieee80211_qos_htc_frame_addr4_stru *hdr)
{
    /* From DS标识位设置 */
    mac_hdr_set_from_ds((uint8_t *)hdr, 1);

    /* to DS标识位设置 */
    mac_hdr_set_to_ds((uint8_t *)hdr, 0);

    /* Set Address1 field in the WLAN Header with destination address */
    oal_set_mac_addr(hdr->auc_address1, (uint8_t *)ether_hdr->auc_ether_dhost);

    /* Set Address2 field in the WLAN Header with the BSSID */
    oal_set_mac_addr(hdr->auc_address2, vap->st_vap_base_info.auc_bssid);

    if (MAC_GET_CB_IS_AMSDU(tx_ctl)) { /* AMSDU情况，地址3填写BSSID */
        /* Set Address3 field in the WLAN Header with the BSSID */
        oal_set_mac_addr(hdr->auc_address3, vap->st_vap_base_info.auc_bssid);
    } else {
        /* Set Address3 field in the WLAN Header with the source address */
        oal_set_mac_addr(hdr->auc_address3, (uint8_t *)ether_hdr->auc_ether_shost);
    }
}

#ifdef _PRE_WLAN_CHBA_MGMT

static void hmac_set_chba_mac_addr(hmac_vap_stru *hmac_vap, mac_ether_header_stru *ether_hdr,
    mac_ieee80211_qos_htc_frame_addr4_stru *mac_hdr)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    uint8_t auc_bssid[WLAN_MAC_ADDR_LEN];

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        oam_warning_log0(0, 0, "CHBA: hmac_set_chba_mac_addr: chba_vap_info is null.");
        return;
    }
    oal_set_mac_addr(auc_bssid, hmac_vap->st_vap_base_info.auc_bssid);

    /* From DS标识位设置 */
    mac_hdr_set_from_ds((uint8_t *)mac_hdr, 0);
    /* to DS标识位设置 */
    mac_hdr_set_to_ds((uint8_t *)mac_hdr, 0);
    /* Set Address1 field in the WLAN Header with destination address */
    oal_set_mac_addr(mac_hdr->auc_address1, (uint8_t *)ether_hdr->auc_ether_dhost);
    /* Set Address2 field in the WLAN Header with the StationID */
    oal_set_mac_addr(mac_hdr->auc_address2, mac_mib_get_StationID(&hmac_vap->st_vap_base_info));
    /* Set Address3 field in the WLAN Header with the BSSID */
    oal_set_mac_addr(mac_hdr->auc_address3, (uint8_t *)auc_bssid);
}
#endif

#ifdef _PRE_WINDOWS_SUPPORT
static uint32_t hmac_tx_switch_addresses_fips(mac_tx_ctl_stru *tx_ctl,
    mac_ieee80211_qos_htc_frame_addr4_stru *hdr_old, mac_ieee80211_qos_htc_frame_addr4_stru *hdr_new)
{
    hdr_new->bit_frag_num = 0;
    hdr_new->bit_seq_num = 0;
    mac_hdr_set_from_ds((uint8_t *)(&hdr_new->st_frame_control), hdr_old->st_frame_control.bit_from_ds);
    mac_hdr_set_to_ds((uint8_t *)(&hdr_new->st_frame_control), hdr_old->st_frame_control.bit_to_ds);
    oal_set_mac_addr((uint8_t *)(hdr_new->auc_address1), (uint8_t *)(hdr_old->auc_address1));
    oal_set_mac_addr((uint8_t *)(hdr_new->auc_address2), (uint8_t *)(hdr_old->auc_address2));
    oal_set_mac_addr((uint8_t *)(hdr_new->auc_address3), (uint8_t *)(hdr_old->auc_address3));
    if (MAC_GET_CB_IS_4ADDRESS(tx_ctl)) {
        oal_set_mac_addr((uint8_t *)(hdr_new->auc_address4), (uint8_t *)(hdr_old->auc_address4));
    }
    return OAL_SUCC;
}
#endif


OAL_STATIC OAL_INLINE uint32_t hmac_tx_set_addresses(hmac_vap_stru *pst_vap,
    hmac_user_stru *pst_user, mac_tx_ctl_stru *pst_tx_ctl, mac_ether_header_stru *pst_ether_hdr,
    mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr)
{
    /* 分片号置成0，后续分片特性需要重新赋值 */
    pst_hdr->bit_frag_num = 0;
    pst_hdr->bit_seq_num = 0;
#ifdef _PRE_WLAN_CHBA_MGMT
    /* chba组网下设置帧mac地址，tods和from ds暂时都设置为0，bssid设置为固定地址 */
    if ((g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA)) &&
        (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (pst_vap->st_vap_base_info.chba_mode == CHBA_MODE)) {
        hmac_set_chba_mac_addr(pst_vap, pst_ether_hdr, pst_hdr);
        return OAL_SUCC;
    }
#endif
    /* From AP */
    if ((pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) && (!(MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)))) {
        hmac_tx_set_ap_address(pst_vap, pst_tx_ctl, pst_ether_hdr, pst_hdr);
    } else if ((pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
               (!(MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)))) {
        hmac_tx_set_sta_mode_address(pst_vap, pst_user, pst_tx_ctl, pst_ether_hdr, pst_hdr);
    } else if (MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)) {
        /* WDS */
        /* TO DS标识位设置 */
        mac_hdr_set_to_ds((uint8_t *)pst_hdr, 1);

        /* From DS标识位设置 */
        mac_hdr_set_from_ds((uint8_t *)pst_hdr, 1);

        /* 地址1是 RA */
        oal_set_mac_addr(pst_hdr->auc_address1, pst_user->st_user_base_info.auc_user_mac_addr);

        /* 地址2是 TA (当前只有BSSID) */
        oal_set_mac_addr(pst_hdr->auc_address2, mac_mib_get_StationID(&pst_vap->st_vap_base_info));

        if (MAC_GET_CB_IS_AMSDU(pst_tx_ctl)) { /* AMSDU情况，地址3和地址4填写BSSID */
            if (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
                /* 地址3是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address3, pst_user->st_user_base_info.auc_user_mac_addr);

                /* 地址4也是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address4, pst_user->st_user_base_info.auc_user_mac_addr);
            } else if (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
                /* 地址3是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address3, mac_mib_get_StationID(&pst_vap->st_vap_base_info));

                /* 地址4也是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address4, mac_mib_get_StationID(&pst_vap->st_vap_base_info));
            }
        } else {
            /* 地址3是 DA */
            oal_set_mac_addr(pst_hdr->auc_address3, (uint8_t *)pst_ether_hdr->auc_ether_dhost);

            /* 地址4是 SA */
            oal_set_mac_addr(pst_hdr->auc_address4, (uint8_t *)pst_ether_hdr->auc_ether_shost);
        }
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_ba_is_setup(hmac_user_stru *pst_hmac_user, uint8_t uc_tidno)
{
    if (oal_unlikely(pst_hmac_user == NULL) || uc_tidno >= WLAN_TID_MAX_NUM) {
        return OAL_FALSE;
    }
    return (pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status == DMAC_BA_COMPLETE);
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_vap_ba_is_setup(hmac_user_stru *pst_hmac_user)
{
    uint8_t uc_tidno;

    if (pst_hmac_user == NULL) {
        return OAL_FALSE;
    }

    for (uc_tidno = 0; uc_tidno < WLAN_TID_MAX_NUM; uc_tidno++) {
        if (pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status == DMAC_BA_COMPLETE) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE void hmac_tx_ba_del(hmac_vap_stru *pst_hmac_vap,
    hmac_user_stru *pst_hmac_user, uint8_t uc_tidno)
{
    mac_action_mgmt_args_stru st_action_args; /* 用于填写ACTION帧的参数 */

    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action = MAC_BA_ACTION_DELBA;
    st_action_args.arg1 = uc_tidno;
    st_action_args.arg2 = MAC_ORIGINATOR_DELBA;
    st_action_args.arg3 = MAC_UNSPEC_REASON;
    st_action_args.puc_arg5 = pst_hmac_user->st_user_base_info.auc_user_mac_addr;
    hmac_mgmt_tx_action(pst_hmac_vap, pst_hmac_user, &st_action_args);
}
OAL_STATIC OAL_INLINE void hmac_need_del_ba_check(hmac_vap_stru *pst_hmac_vap,
    hmac_user_stru *pst_hmac_user, uint8_t uc_tidno)
{
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 判断需要删除BA,配置命令处可能已经删除BA,因此需要再次判断删除BA条件 */
    oal_spin_lock_bh(&pst_hmac_vap->st_ampdu_lock);
    if (OAL_FALSE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info) &&
        (OAL_TRUE == hmac_tid_ba_is_setup(pst_hmac_user, uc_tidno))) {
        hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
    }

    oal_spin_unlock_bh(&pst_hmac_vap->st_ampdu_lock);
#else
    hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
#endif
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_ba_is_supp_ampdu(hmac_user_stru *hmac_user)
{
    wlan_ciper_protocol_type_enum_uint8 cipher_type = hmac_user->st_user_base_info.st_key_info.en_cipher_type;

    return ((cipher_type == WLAN_80211_CIPHER_SUITE_NO_ENCRYP) || (cipher_type == WLAN_80211_CIPHER_SUITE_CCMP) ||
            (cipher_type == WLAN_80211_CIPHER_SUITE_CCMP_256) || (cipher_type == WLAN_80211_CIPHER_SUITE_GCMP) ||
            (cipher_type == WLAN_80211_CIPHER_SUITE_GCMP_256) ||
            (wlan_chip_is_support_hw_wapi() && cipher_type == WLAN_80211_CIPHER_SUITE_WAPI));
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_ba_vap_is_supp_aggr(hmac_vap_stru *hmac_vap)
{
    if (!(mac_mib_get_TxAggregateActived(&hmac_vap->st_vap_base_info) ||
        (hmac_vap->st_vap_base_info.st_cap_flag.bit_rifs_tx_on))) {
        oam_info_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                      "{hmac_tid_need_ba_session::en_tx_aggr_on of vap is off");
        return OAL_FALSE;
    }

    if (mac_mib_get_TxBASessionNumber(&hmac_vap->st_vap_base_info) >= g_wlan_spec_cfg->max_tx_ba) {
        oam_info_log1(0, OAM_SF_BA, "{hmac_tid_need_ba_session::uc_tx_ba_session_num[%d] exceed spec",
            mac_mib_get_TxBASessionNumber(&hmac_vap->st_vap_base_info));
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_ba_session_set_up_is_allowed(
    hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, uint8_t tidno)
{
    hmac_tid_stru *pst_hmac_tid_info = &(hmac_user->ast_tid_info[tidno]);
    /* 对tid对应的tx BA会话状态加锁 */
    oal_spin_lock_bh(&(pst_hmac_tid_info->st_ba_tx_info.st_ba_status_lock));
    if ((pst_hmac_tid_info->st_ba_tx_info.en_ba_status == DMAC_BA_INIT) &&
        (pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps < HMAC_ADDBA_EXCHANGE_ATTEMPTS)) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                         "{hmac_tid_need_ba_session::ba_status[%d]}", pst_hmac_tid_info->st_ba_tx_info.en_ba_status);
        pst_hmac_tid_info->st_ba_tx_info.en_ba_status = DMAC_BA_INPROGRESS;
        pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps++;
    } else {
        oam_info_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                      "{hmac_tid_need_ba_session::addba_attemps[%d] of tid[%d] is COMPLETE}",
                      pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps, tidno);
        /* 对tid对应的tx BA会话状态解锁 */
        oal_spin_unlock_bh(&(pst_hmac_tid_info->st_ba_tx_info.st_ba_status_lock));
        return OAL_FALSE;
    }
    /* 对tid对应的tx BA会话状态解锁 */
    oal_spin_unlock_bh(&(pst_hmac_tid_info->st_ba_tx_info.st_ba_status_lock));
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE oal_bool_enum hmac_tid_need_ba_tid_judge(uint8_t tidno)
{
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    if ((mac_get_mcast_ampdu_switch() == OAL_TRUE && tidno == WLAN_TIDNO_VOICE) || tidno == WLAN_TIDNO_BCAST) {
        return OAL_FALSE;
    }
#endif

    if (tidno == WLAN_TIDNO_BCAST) {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_need_ba_session(hmac_vap_stru *pst_hmac_vap,
    hmac_user_stru *pst_hmac_user, uint8_t uc_tidno, oal_netbuf_stru *pst_buf)
{
    /* 该tid下不允许建BA，配置命令需求 */
    if (pst_hmac_user->ast_tid_info[uc_tidno].en_ba_handle_tx_enable == OAL_FALSE) {
        return OAL_FALSE;
    }

    if (OAL_TRUE == hmac_tid_ba_is_setup(pst_hmac_user, uc_tidno)) {
        if (OAL_TRUE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info)) {
            return OAL_FALSE;
        }

        hmac_need_del_ba_check(pst_hmac_vap, pst_hmac_user, uc_tidno);
    }

    /* 配置命令不允许建立聚合时返回 */
    if (OAL_FALSE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info)) {
        return OAL_FALSE;
    }

    /* 建立BA会话，需要判断VAP的AMPDU的支持情况，因为需要实现建立BA会话时，一定发AMPDU */
    if (OAL_FALSE == hmac_user_xht_support(pst_hmac_user)) {
        if (pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status != DMAC_BA_INIT) {
            hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
        }

        return OAL_FALSE;
    }

    /* wep/tkip不支持11n及以上协议 */
    if (!hmac_tid_ba_is_supp_ampdu(pst_hmac_user)) {
        return OAL_FALSE;
    }

    if (WLAN_ADDBA_MODE_AUTO != mac_mib_get_AddBaMode(&pst_hmac_vap->st_vap_base_info)) {
        return OAL_FALSE;
    }

    /* tid7不建立聚合，供VIP帧使用 */
    /* 组播聚合场景，tid6不建立聚合 */
    if (hmac_tid_need_ba_tid_judge(uc_tidno) == OAL_FALSE) {
        return OAL_FALSE;
    }

    if (wlan_chip_check_need_setup_ba_session() == OAL_FALSE) {
        return OAL_FALSE;
    }

    /* 判断HMAC VAP的是否支持聚合 */
    if (!hmac_tid_ba_vap_is_supp_aggr(pst_hmac_vap)) {
        return OAL_FALSE;
    }
    /* 针对关闭WMM，非QOS帧处理 */
    if (pst_hmac_user->st_user_base_info.st_cap_info.bit_qos == OAL_FALSE) {
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{UnQos Frame pass!!}");
        return OAL_FALSE;
    }

    if (!hmac_tid_ba_session_set_up_is_allowed(pst_hmac_vap, pst_hmac_user, uc_tidno)) {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_tx_data.h */
