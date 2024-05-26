
#ifndef __OAM_STAT_WIFI_H__
#define __OAM_STAT_WIFI_H__

#include "wlan_types.h"
#include "platform_spec_1103.h"
#include "wlan_oneimage_define.h"

/* 统计相关宏 */
#define OAM_MAC_ERROR_TYPE_CNT   25
#define OAM_RX_DSCR_QUEUE_CNT    2
#define OAM_IRQ_FREQ_STAT_MEMORY 50

#define oam_stat_vap_incr(_uc_vap_id, _member, _num)                                 \
    do {                                                                             \
        if ((_uc_vap_id) < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT) {                           \
            g_oam_stat_info.ast_vap_stat_info[_uc_vap_id].ul_##_member += (_num); \
        }                                                                            \
    } while (0)
#define oam_stat_get_stat_all() (&g_oam_stat_info)

/* 设备级别统计信息结构体 */
typedef struct {
    /* 中断个数统计信息 */
    /* 总的中断请求个数 */
    uint32_t ul_irq_cnt;

    /* SOC五类中断计数 */
    uint32_t ul_pmu_err_irq_cnt;      /* 电源错误中断 */
    uint32_t ul_rf_over_temp_irq_cnt; /* RF过热中断 */
    uint32_t ul_unlock_irq_cnt;       /* CMU PLL失锁中断 */
    uint32_t ul_mac_irq_cnt;          /* Mac业务中断 */
    uint32_t ul_pcie_slv_err_irq_cnt; /* PCIE总线错误中断 */

    /* pmu/buck错误子中断源计数 */
    uint32_t ul_buck_ocp_irq_cnt;   /* BUCK过流中断 */
    uint32_t ul_buck_scp_irq_cnt;   /* BUCK短路中断 */
    uint32_t ul_ocp_rfldo1_irq_cnt; /* PMU RFLDO1过流中断 */
    uint32_t ul_ocp_rfldo2_irq_cnt; /* PMU RFLDO2过流中断 */
    uint32_t ul_ocp_cldo_irq_cnt;   /* PMU CLDO过流中断 */

    /* MAC子中断源计数(与MACINTERRUPT_STATUS寄存器对应) */
    uint32_t ul_rx_complete_cnt;                           /* 数据帧接收完成 */
    uint32_t ul_tx_complete_cnt;                           /* 发送完成 */
    uint32_t ul_hi_rx_complete_cnt;                        /* 管理帧、控制帧接收完成 */
    uint32_t ul_error_cnt;                                 /* error中断 */
    uint32_t aul_tbtt_cnt[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT]; /* tbtt中断,0~3代表ap的，4代表sta的 */
    uint32_t ul_abort_intr_start;
    uint32_t ul_abort_intr_end;
    uint32_t ul_radar_detect_cnt; /* 检测到雷达 */
    uint32_t ul_pa_tx_suspended;
    uint32_t ul_de_authentication;
    uint32_t ul_one_packet;
    uint32_t ul_abort_intr_done;
    uint32_t ul_ch_statistics_cnt; /* 信道统计完成中断 */
    uint32_t ul_wow_prep_done;
    uint32_t ul_other_int_cnt; /* 其它 */

    /* MAC错误中断类型, 注意此处枚举值与错误中断状态寄存器的位一一对应 ! */
    uint32_t aul_mac_error_cnt[OAM_MAC_ERROR_TYPE_CNT];

    /* 接收中断上报频度统计 */
    uint32_t ul_normal_rx_idx; /* 当前中断时间戳所在位置 */
    uint32_t ul_hi_rx_idx; /* 当前中断时间戳所在位置 */
    uint32_t aul_rx_freq[OAM_RX_DSCR_QUEUE_CNT][OAM_IRQ_FREQ_STAT_MEMORY]; /* 接收频度统计，时间戳精度10ns */
    uint32_t ul_rx_freq_stop_flag; /* 接收频度标志，一旦上报fifo overun之后则停止统计 */

    /* 中断时延统计, 单位10ns */
    uint32_t ul_max_delay;   /* 最大时长 */
    uint32_t ul_min_delay;   /* 最小时长 */
    uint32_t ul_delay_sum;   /* 总时长 */
    uint32_t ul_timer_start; /* 开始时间记录 */
} oam_device_stat_info_stru;

/* VAP级别统计信息结构 */
typedef struct {
    /* 接收包统计 */
    /* 发往lan的数据包统计 */
    uint32_t ul_rx_pkt_to_lan;   /* 接收流程发往以太网的数据包数目，MSDU */
    uint32_t ul_rx_bytes_to_lan; /* 接收流程发往以太网的字节数 */

    /* 接收流程遇到严重错误(描述符异常等)，释放所有MPDU的统计 */
    uint32_t ul_rx_ppdu_dropped; /* 硬件上报的vap_id错误，释放的MPDU个数 */

    /* 软件统计的接收到MPDU个数，正常情况下应该与MAC硬件统计值相同 */
    uint32_t ul_rx_mpdu_total_num; /* 接收流程上报到软件进行处理的MPDU总个数 */

    /* MPDU级别进行处理时发生错误释放MPDU个数统计 */
    uint32_t ul_rx_ta_check_dropped;        /* 检查发送端地址异常，释放一个MPDU */
    uint32_t ul_rx_da_check_dropped;        /* 检查目的端地址异常，释放一个MPDU */
    uint32_t ul_rx_phy_perform_dropped;     /* 测试phy性能，将帧直接释放 */
    uint32_t ul_rx_key_search_fail_dropped;
    uint32_t ul_rx_tkip_mic_fail_dropped;   /* 接收描述符status为 tkip mic fail释放MPDU */
    uint32_t ul_rx_replay_fail_dropped;     /* 重放攻击，释放一个MPDU */
    uint32_t ul_rx_11i_check_fail_dropped;
    uint32_t ul_rx_wep_check_fail_dropped;
    uint32_t ul_rx_alg_process_dropped;    /* 算法处理返回失败 */
    uint32_t ul_rx_null_frame_dropped;     /* 接收到空帧释放(之前节能特性已经对其进行处理) */
    uint32_t ul_rx_abnormal_dropped;       /* 其它异常情况释放MPDU */
    uint32_t ul_rx_mgmt_abnormal_dropped;  /* 接收到管理帧出现异常，比如vap或者dev为空等 */
    uint32_t ul_rx_ack_dropped;            /* ack直接过滤掉 */
    uint32_t ul_rx_pspoll_process_dropped; /* 处理ps-poll的时候释放包 */
    uint32_t ul_rx_bar_process_dropped;    /* 处理block ack req释放包 */
    uint32_t ul_rx_abnormal_cnt;           /* 处理MPDU时发现异常的次数，并非丢包数目 */

    uint32_t ul_rx_no_buff_dropped;              /* 组播帧或者wlan_to_wlan流程申请buff失败 */
    uint32_t ul_rx_defrag_process_dropped;       /* 去分片处理失败 */
    uint32_t ul_rx_de_mic_fail_dropped;          /* mic码校验失败 */
    uint32_t ul_rx_portvalid_check_fail_dropped; /* 接收到数据帧，安全检查失败释放MPDU */

    /* 接收到组播帧个数 */
    uint32_t ul_rx_mcast_cnt;

    /* 管理帧统计 */
    uint32_t aul_rx_mgmt_num[WLAN_MGMT_SUBTYPE_BUTT]; /* VAP接收到各种管理帧的数目统计 */

    /* 发送包统计 */
    /* 从lan接收到的数据包统计 */
    uint32_t ul_tx_pkt_num_from_lan; /* 从lan过来的包数目,MSDU */
    uint32_t ul_tx_bytes_from_lan;   /* 从lan过来的字节数 */

    /* 发送流程发生异常导致释放的数据包统计，MSDU级别 */
    uint32_t ul_tx_abnormal_msdu_dropped; /* 异常情况释放MSDU个数，指vap或者user为空等异常 */
    uint32_t ul_tx_security_check_faild;  /* 安全检查失败释放MSDU */
    uint32_t ul_tx_abnormal_mpdu_dropped; /* 异常情况释放MPDU个数，指vap或者user为空等异常 */
    uint32_t ul_tx_uapsd_process_dropped; /* UAPSD特性处理失败，释放MPDU个数 */
    uint32_t ul_tx_psm_process_dropped;   /* PSM特性处理失败，释放MPDU个数 */
    uint32_t ul_tx_alg_process_dropped;   /* 算法处理认为应该丢包，释放MPDU个数 */

    /* 发送完成中发送成功与失败的数据包统计，MPDU级别 */
    uint32_t ul_tx_mpdu_succ_num;      /* 发送MPDU总个数 */
    uint32_t ul_tx_mpdu_fail_num;      /* 发送MPDU失败个数 */
    uint32_t ul_tx_ampdu_succ_num;     /* 发送成功的AMPDU总个数 */
    uint32_t ul_tx_ampdu_bytes;        /* 发送AMPDU总字节数 */
    uint32_t ul_tx_mpdu_in_ampdu;      /* 属于AMPDU的MPDU发送总个数 */
    uint32_t ul_tx_ampdu_fail_num;     /* 发送AMPDU失败个数 */
    uint32_t ul_tx_mpdu_fail_in_ampdu; /* 属于AMPDU的MPDU发送失败个数 */

    /* 组播转单播发送流程统计 */
    uint32_t ul_tx_m2u_ucast_cnt;    /* 组播转单播 单播发送成功个数 */
    uint32_t ul_tx_m2u_ucast_droped; /* 组播转单播 单播发送失败个数 */
    uint32_t ul_tx_m2u_mcast_cnt;    /* 组播转单播 组播发送成功个数 */
    uint32_t ul_tx_m2u_mcast_droped; /* 组播转单播 组播发送成功个数 */

    /* 管理帧统计 */
    uint32_t aul_tx_mgmt_num_sw[WLAN_MGMT_SUBTYPE_BUTT]; /* VAP挂到硬件队列上的各种管理帧的数目统计 */
    uint32_t aul_tx_mgmt_num_hw[WLAN_MGMT_SUBTYPE_BUTT]; /* 发送完成中各种管理帧的数目统计 */
} oam_vap_stat_info_stru;

/* USER级别统计信息结构 */
typedef struct {
    /* 接收包统计，从某个用户处接收到的包统计(TO DS) */
    uint32_t ul_rx_mpdu_num;   /* 从某个用户接收到的MPDU数目 */
    uint32_t ul_rx_mpdu_bytes; /* 某个用户接收到的MPDU字节数 */

    /* 发送包统计，发送给某个用户的包统计(FROM DS)，粒度是TID(具体到从VAP的某个TID发出) */
    uint32_t aul_tx_mpdu_succ_num[WLAN_TIDNO_BUTT];      /* 发送MPDU总个数 */
    uint32_t aul_tx_mpdu_bytes[WLAN_TIDNO_BUTT];         /* 发送MPDU总字节数 */
    uint32_t aul_tx_mpdu_fail_num[WLAN_TIDNO_BUTT];      /* 发送MPDU失败个数 */
    uint32_t aul_tx_ampdu_succ_num[WLAN_TIDNO_BUTT];     /* 发送AMPDU总个数 */
    uint32_t aul_tx_ampdu_bytes[WLAN_TIDNO_BUTT];        /* 发送AMPDU总字节数 */
    uint32_t aul_tx_mpdu_in_ampdu[WLAN_TIDNO_BUTT];      /* 属于AMPDU的MPDU发送总个数 */
    uint32_t aul_tx_ampdu_fail_num[WLAN_TIDNO_BUTT];     /* 发送AMPDU失败个数 */
    uint32_t aul_tx_mpdu_fail_in_ampdu[WLAN_TIDNO_BUTT]; /* 属于AMPDU的MPDU发送失败个数 */
    uint32_t ul_tx_ppdu_retries;                         /* 空口重传的PPDU计数 */
} oam_user_stat_info_stru;

/* 统计结构信息，包括设备级别，vap级别，user级别的各种统计信息 */
typedef struct {
    /* 设备级别的统计信息 */
    oam_device_stat_info_stru ast_dev_stat_info[WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC];

    /* VAP级别的统计信息 */
    oam_vap_stat_info_stru ast_vap_stat_info[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];

    /* USER级别的统计信息 */
    oam_user_stat_info_stru ast_user_stat_info[WLAN_USER_MAX_USER_LIMIT];
} oam_stat_info_stru;

extern oam_stat_info_stru g_oam_stat_info;
uint32_t oam_stats_report_info_to_sdt(uint8_t en_ota_type);
uint32_t oam_stats_clear_vap_stat_info(uint8_t uc_vap_id);
uint32_t oam_stats_clear_user_stat_info(uint16_t us_usr_id);
#endif