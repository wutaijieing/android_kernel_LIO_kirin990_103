

#ifndef __HMAC_VSP_H__
#define __HMAC_VSP_H__

#include "wlan_types.h"
#include "mac_vap.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "host_hal_device.h"
#ifdef CONFIG_VCODEC_VSP_SUPPORT
#include <media/native/vcodec_vsp_buffer.h>
#else
#include "hmac_vsp_vcodec_stub.h"
#endif
#include "hmac_tx_msdu_dscr.h"
#include "hmac_vsp_stat.h"

#ifdef _PRE_WLAN_FEATURE_VSP
#define VSP_MSDU_CB_LEN 64

#define VSP_TIMESTAMP_NOT_USED 0
#define VSP_TIMESTAMP_VENC 1
#define VSP_TIMESTAMP_WIFI 2

#define VSP_FRAME_ID_MASK 0x3F
#define VSP_FRAME_ID_TOTAL 64
#define VSP_INVALID_FRAME_ID 0xff

#define VSP_LAYER_FEEDBACK_SIZE 2

#define MAX_VSP_INFO_COUNT 2
#pragma pack(push, 1)
/* VSPbuffer头描述 */
struct vsp_msdu_hdr {
    uint8_t ra[WLAN_MAC_ADDR_LEN];
    uint8_t sa[WLAN_MAC_ADDR_LEN];
    uint16_t type;
    uint16_t video : 1,
             layer_num : 3,
             slice_num : 4,
             last : 1,
             max_layer : 3,
             ver : 4;
    uint16_t number;
    uint16_t len;
    uint32_t frame_id : 6,
             ts_type : 2,
             rsv_bits : 24;
    uint32_t timestamp;
    uint32_t cfg_pkt : 1,
             reserve : 31;
};
typedef struct vsp_msdu_hdr vsp_msdu_hdr_stru;

typedef struct {
    uint8_t ra[WLAN_MAC_ADDR_LEN];
    uint8_t sa[WLAN_MAC_ADDR_LEN];
    uint16_t type;
    uint16_t frm_id : 6,
            peer : 1,
            reserv : 1,
            layer_num : 4,
            slice_id : 4;
    uint16_t layer_succ_num[];
} vsp_feedback_frame;
#pragma pack(pop)

typedef struct {
    uint32_t max_transmit_dly_us;
    uint32_t vsync_interval_us;
    uint32_t slice_interval_us;
    uint32_t feedback_dly_us;
    uint8_t slices_per_frm;
    uint8_t layer_num;
    uint16_t feedback_pkt_len;
} vsp_param;

typedef struct {
    oal_dlist_head_stru entry;
    tx_layer_ctrl *head; /* layer链表头 */
    tx_layer_ctrl *curr; /* 当前发送的layer */
    uint16_t tx_pos; /* 下一个要发送的packet idx */
    uint8_t slice_id;
    uint8_t frame_id;
    uint32_t arrive_ts;
    uint32_t vsync_ts; /* 记录与本video frame对应的vsync时间，用于计算超时时间 */
    uint32_t pkts_in_ring;
    uint8_t feedback;
    uint8_t is_rpt;
    uint8_t retrans_cnt;
    send_result tx_result;
} vsp_tx_slice_stru;

typedef struct {
    oal_dlist_head_stru head;
    oal_spin_lock_stru lock;
    uint32_t len;
} vsp_slice_queue_stru;

struct vsp_sink_stat {
    hmac_vsp_stat_stru rate;
    uint32_t rx_detail[MAX_LAYER_NUM];
    uint32_t netbuf_recycle_cnt;
};

struct vsp_source_stat {
    hmac_vsp_stat_stru rate;
    uint32_t feedback_cnt;
    uint32_t feedback_detail[MAX_LAYER_NUM];
};

typedef struct {
    oal_bool_enum_uint8 used;
    oal_bool_enum_uint8 enable;          /* vsp是否使能 */
    oal_spin_lock_stru tx_lock;
    uint8_t mode;                                  /* 发送端，接收端 */
    vsp_slice_queue_stru free_queue;
    vsp_slice_queue_stru comp_queue;
    hal_mac_common_timer mac_common_timer;         /* host通用定时器 */
    uint8_t timer_running;
    rx_slice_mgmt **rx_slice_tbl;                      /* 接收paket处理 */
    uint16_t slices_in_tbl; /* 接收端当前正在接收的slice计数，支持多slice联合传输 */
    hmac_tid_info_stru *tid_info;
    hmac_user_stru *hmac_user;                     /* vsp对应的hmac_user */
    hmac_vap_stru *hmac_vap;
    hal_host_device_stru *host_hal_device;         /* host device结构 */
    uint32_t last_vsync; /* 发送端记录最近一次vsync中断时tsf计数 */
    uint8_t next_timeout_frm;
    uint8_t next_timeout_slice;
    uint8_t last_rx_comp_frm;
    uint8_t last_rx_comp_slice;
    uint32_t timer_ref_vsync; /* 当前正在计时的frame参照的vsync，接收端和发送端都使用 */
    uint32_t pkt_ts_vsync; /* 发送端packet的时间戳 */
    /* 接收端记录最近一次超时的时间戳，对应上一个next_timeout_{frm,slice}计数，用来识别最近超时的slice的数据 */
    uint32_t last_timeout_ts;
    struct task_struct *evt_loop;
    unsigned long evt_flags;
    oal_wait_queue_head_stru evt_wq;
    oal_netbuf_head_stru feedback_q;
    vsp_param param;
    struct vsp_sink_stat sink_stat;
    struct vsp_source_stat source_stat;
} hmac_vsp_info_stru;

typedef struct {
    rx_slice_mgmt *(*alloc_slice_mgmt)(uint32_t);
    void (*rx_slice_done)(rx_slice_mgmt*);
    void (*wifi_tx_pkg_done)(send_result*);
} hmac_vsp_vcodec_ops;
extern const hmac_vsp_vcodec_ops *g_vsp_vcodec_ops;
#define vsp_vcodec_alloc_slice_mgmt(size) g_vsp_vcodec_ops->alloc_slice_mgmt(size)
#define vsp_vcodec_rx_slice_done(rx_slice) g_vsp_vcodec_ops->rx_slice_done(rx_slice)
#define vsp_vcodec_tx_pkt_done(snd_res) g_vsp_vcodec_ops->wifi_tx_pkg_done(snd_res)

hmac_vsp_info_stru *hmac_vsp_init(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, uint8_t mode);
void hmac_vsp_deinit(hmac_vsp_info_stru *vsp_info);
hmac_vsp_info_stru *hmac_get_vsp_source(void);
void hmac_vsp_dump_packet(vsp_msdu_hdr_stru *hdr, uint16_t len);
void hmac_vsp_record_vsync_ts(hmac_vsp_info_stru *vsp_info);
uint32_t hmac_vsp_get_timestamp(hmac_vsp_info_stru *vsp_info);
void hmac_vsp_start_timer(hmac_vsp_info_stru *vsp_info, uint32_t timeout_us);
void hmac_vsp_stop_timer(hmac_vsp_info_stru *vsp_info);
void hmac_vsp_set_timeout_for_slice(uint8_t slice_id, hmac_vsp_info_stru *vsp_info);
void hmac_vsp_set_vcodec_ops(const hmac_vsp_vcodec_ops *ops);
hmac_vsp_info_stru *hmac_get_vsp_by_idx(uint8_t idx);

static inline uint8_t hmac_is_vsp_user(hmac_user_stru *hmac_user)
{
    hmac_vsp_info_stru *vsp_info = hmac_user->vsp_hdl;

    return vsp_info && vsp_info->enable;
}

static inline uint8_t hmac_is_vsp_tid(hmac_user_stru *hmac_user, uint8_t tidno)
{
    return hmac_is_vsp_user(hmac_user) && (tidno == WLAN_TIDNO_VSP);
}

static inline void hmac_vsp_inc_next_timeout_slice(hmac_vsp_info_stru *vsp_info)
{
    if (++vsp_info->next_timeout_slice >= vsp_info->param.slices_per_frm) {
        vsp_info->next_timeout_slice = 0;
        vsp_info->next_timeout_frm = (vsp_info->next_timeout_frm + 1) & VSP_FRAME_ID_MASK;
    }
}

static inline uint32_t hmac_vsp_calc_runtime(uint32_t start, uint32_t end)
{
    return end - start;
}

static inline uint8_t hmac_vsp_tid_ring_empty(hmac_vsp_info_stru *vsp_info)
{
    return hmac_tid_ring_empty(&vsp_info->tid_info->tx_ring);
}

static inline void hmac_vsp_slice_queue_init(vsp_slice_queue_stru *queue)
{
    queue->len = 0;
    oal_dlist_init_head(&queue->head);
    oal_spin_lock_init(&queue->lock);
}

typedef uint8_t (* hmac_vsp_slice_func)(vsp_tx_slice_stru *, void *);
static inline void hmac_vsp_slice_queue_foreach_safe(vsp_slice_queue_stru *queue, hmac_vsp_slice_func func, void *param)
{
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    oal_dlist_search_for_each_safe(entry, entry_tmp, &queue->head) {
        if (func(oal_dlist_get_entry(entry, vsp_tx_slice_stru, entry), param) == OAL_RETURN) {
            break;
        }
    }
}

static inline uint8_t hmac_vsp_slice_cnt(vsp_tx_slice_stru *slice, void *param)
{
    (*(uint8_t *)param)++;

    return OAL_CONTINUE;
}

static inline uint32_t __hmac_vsp_slice_queue_len(vsp_slice_queue_stru *queue)
{
    uint32_t len = 0;

    hmac_vsp_slice_queue_foreach_safe(queue, hmac_vsp_slice_cnt, &len);

    return len;
}

static inline uint32_t hmac_vsp_slice_queue_len(vsp_slice_queue_stru *queue)
{
    uint32_t len;

    oal_spin_lock(&queue->lock);
    len = __hmac_vsp_slice_queue_len(queue);
    oal_spin_unlock(&queue->lock);

    return len;
}

static inline uint8_t hmac_vsp_slice_queue_empty(vsp_slice_queue_stru *queue)
{
    return oal_dlist_is_empty(&queue->head);
}

static inline void __hmac_vsp_slice_enqueue(vsp_slice_queue_stru *queue, vsp_tx_slice_stru *slice)
{
    oal_dlist_add_tail(&slice->entry, &queue->head);
}

static inline void hmac_vsp_slice_enqueue(vsp_slice_queue_stru *queue, vsp_tx_slice_stru *slice)
{
    oal_spin_lock(&queue->lock);
    __hmac_vsp_slice_enqueue(queue, slice);
    oal_spin_unlock(&queue->lock);
}

static inline void __hmac_vsp_slice_enqueue_head(vsp_slice_queue_stru *queue, vsp_tx_slice_stru *slice)
{
    oal_dlist_add_head(&slice->entry, &queue->head);
}

static inline void hmac_vsp_slice_enqueue_head(vsp_slice_queue_stru *queue, vsp_tx_slice_stru *slice)
{
    oal_spin_lock(&queue->lock);
    __hmac_vsp_slice_enqueue_head(queue, slice);
    oal_spin_unlock(&queue->lock);
}

static inline vsp_tx_slice_stru *__hmac_vsp_slice_dequeue(vsp_slice_queue_stru *queue)
{
    return oal_container_of(oal_dlist_delete_head(&queue->head), vsp_tx_slice_stru, entry);
}

static inline vsp_tx_slice_stru *hmac_vsp_slice_dequeue(vsp_slice_queue_stru *queue)
{
    vsp_tx_slice_stru *slice = NULL;

    oal_spin_lock(&queue->lock);
    slice = __hmac_vsp_slice_dequeue(queue);
    oal_spin_unlock(&queue->lock);

    return slice;
}

static inline vsp_tx_slice_stru *hmac_vsp_slice_queue_peek(vsp_slice_queue_stru *queue)
{
    return oal_container_of(queue->head.pst_next, vsp_tx_slice_stru, entry);
}

static inline mac_user_stru *hmac_vsp_info_get_mac_user(hmac_vsp_info_stru *vsp_info)
{
    return &vsp_info->hmac_user->st_user_base_info;
}

static inline mac_vap_stru *hmac_vsp_info_get_mac_vap(hmac_vsp_info_stru *vsp_info)
{
    return &vsp_info->hmac_vap->st_vap_base_info;
}

static inline uint8_t *hmac_vsp_info_get_mac_ra(hmac_vsp_info_stru *vsp_info)
{
    return hmac_vsp_info_get_mac_user(vsp_info)->auc_user_mac_addr;
}

static inline uint8_t *hmac_vsp_info_get_mac_sa(hmac_vsp_info_stru *vsp_info)
{
    return mac_mib_get_StationID(hmac_vsp_info_get_mac_vap(vsp_info));
}

static inline uint8_t hmac_vsp_info_get_vap_id(hmac_vsp_info_stru *vsp_info)
{
    return hmac_vsp_info_get_mac_vap(vsp_info)->uc_vap_id;
}

static inline uint16_t hmac_vsp_info_get_user_id(hmac_vsp_info_stru *vsp_info)
{
    return hmac_vsp_info_get_mac_user(vsp_info)->us_assoc_id;
}

static inline uint16_t hmac_vsp_info_get_feedback_pkt_len(hmac_vsp_info_stru *vsp_info)
{
    return vsp_info->param.feedback_pkt_len;
}

static inline void hmac_vsp_info_set_last_timeout_ts(hmac_vsp_info_stru *vsp_info, uint32_t last_timeout_ts)
{
    vsp_info->last_timeout_ts = last_timeout_ts;
}

static inline uint8_t hmac_vsp_info_next_timeout_info_invalid(hmac_vsp_info_stru *vsp_info)
{
    return vsp_info->next_timeout_frm == VSP_INVALID_FRAME_ID;
}

static inline uint8_t hmac_vsp_info_last_rx_comp_info_invalid(hmac_vsp_info_stru *vsp_info)
{
    return vsp_info->last_rx_comp_frm == VSP_INVALID_FRAME_ID;
}

#endif
#endif
