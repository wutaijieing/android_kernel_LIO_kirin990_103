

#define HISI_LOG_TAG "[HCC_TEST]"
#include "oal_hcc_host_if.h"
#include "oal_ext_if.h"
#include "plat_pm_wlan.h"
#include "plat_pm.h"
#include "securec.h"
#include "hisi_ini.h"
#include "plat_cali.h"
#if (defined _PRE_PLAT_FEATURE_HI110X_PCIE) || (defined _PRE_PLAT_FEATURE_HI116X_PCIE)
#include "pcie_host.h"
#include "pcie_linux.h"
#endif
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#include "oal_dft.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

typedef struct _hcc_test_data_ {
    int32_t mode_idx;
    int32_t pkt_rcvd;      /* packet received */
    int32_t pkt_sent;      /* packet sent */
    int32_t pkt_gen;       /* packet generate */
    int32_t pkt_len;       /* packet  length */
    unsigned long trans_timeout; /* msec */
    uint64_t total_rcvd_bytes;
    uint64_t total_sent_bytes;
    uint64_t trans_time;
    uint64_t trans_time_us;
    uint64_t throughput;
    hsdio_trans_test_info trans_info;
} hcc_test_data;

struct hcc_test_event {
    oal_mutex_stru mutex_lock; /* sdio test task lock */
    int32_t errorno;

    oal_workqueue_stru *test_workqueue;
    oal_work_stru test_work;
    hcc_test_data test_data;

    /* hcc perf started,for data transfer */
    int32_t started;

    int32_t rx_stop;

    /* hcc perf statistic */
    ktime_t start_time;
    /* The last update time */
    ktime_t last_time;

    /* To hcc test sync */
    oal_completion test_done;
    oal_completion test_trans_done;

    hcc_queue_type hcc_test_queue;

    uint8_t pad_payload;

    uint8_t test_value;
    uint8_t verified;

    unsigned long start_tick;
    unsigned long tick_timeout;

    /* sdio test thread and seam */
};

struct hcc_test_stru {
    const char *mode;
    uint16_t start_cmd;
    const char *mode_desc;
};

/* 全局变量定义 */
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
OAL_STATIC oal_completion g_pcie_test_trans_done;
OAL_STATIC hcc_pcie_test_request_stru g_pcie_test_request_ack;
#endif
OAL_STATIC int32_t g_test_force_stop = 0;
OAL_STATIC struct hcc_test_event *g_hcc_test_event_gt = NULL;

OAL_STATIC struct hcc_test_stru g_hcc_test_stru[HCC_TEST_CASE_COUNT] = {
    [HCC_TEST_CASE_TX] = { "tx",   HCC_TEST_CMD_START_TX,   "HCC_TX_MODE" },
    [HCC_TEST_CASE_RX] = { "rx",   HCC_TEST_CMD_START_RX,   "HCC_RX_MODE" },
    [HCC_TEST_CASE_LOOP] = { "loop", HCC_TEST_CMD_START_LOOP, "HCC_LOOP_MODE" },
};

OAL_STATIC oal_kobject *g_sysfs_hcc_object = NULL;
OAL_STATIC int32_t hcc_test_normal_start(uint16_t test_type);
OAL_STATIC int32_t hcc_send_test_cmd(uint8_t *cmd, int32_t hdr_len, int32_t cmd_len);
OAL_STATIC int32_t hcc_test_start(uint16_t start_cmd);
OAL_STATIC int hcc_test_set_case(hcc_test_data *data);

OAL_STATIC uint64_t hcc_test_get_trans_time(ktime_t start_time, ktime_t stop_time)
{
    ktime_t trans_time;
    uint64_t trans_us;

    trans_time = ktime_sub(stop_time, start_time);
#ifdef _PRE_COMMENT_CODE_
    oal_io_print("start time:%llu, stop time:%llu, trans_time:%llu\n",
                 ktime_to_us(start_time), ktime_to_us(stop_time), ktime_to_us(trans_time));
#endif
    trans_us = (uint64_t)ktime_to_us(trans_time);
    if (trans_us == 0) {
        trans_us = 1;
    }

    return trans_us;
}

OAL_STATIC uint64_t hcc_test_utilization_ratio_gen(uint64_t payload_size, uint64_t transfer_size)
{
    uint64_t ret;
    payload_size = payload_size * 1000; /* 1000 计算占有率 */
    if (transfer_size) {
        ret = div_u64(payload_size, transfer_size);
    } else {
        ret = 0;
    }
    return ret;
}

/* 统计发送方向的丢包率，接收方向默认不丢包 */
OAL_STATIC uint32_t hcc_test_tx_pkt_loss_gen(uint32_t tx_pkts, uint32_t actual_tx_pkts)
{
    uint32_t ul_loss;

    if (tx_pkts == actual_tx_pkts || !tx_pkts) {
        return 0;
    }
    if (tx_pkts < actual_tx_pkts) {
        return 0;
    }

    ul_loss = tx_pkts - actual_tx_pkts;
    return ul_loss * 1000 / tx_pkts; /* 1000 计算占有率 */
}

OAL_STATIC void hcc_test_throughput_cac(uint64_t trans_bytes, ktime_t start_time, ktime_t stop_time)
{
    uint64_t trans_us;
    uint64_t temp;
    uint64_t us_to_s; /* converted  usecond to second */

    trans_us = hcc_test_get_trans_time(start_time, stop_time);
    temp = (trans_bytes);
    temp = temp * 1000u;
    temp = temp * 1000u;
    /* 17是推算出的，无实际意义 */
    temp = (temp >> 17);
    temp = div_u64(temp, trans_us);

    us_to_s = trans_us;
    g_hcc_test_event_gt->test_data.trans_time_us = trans_us;
    do_div(us_to_s, 1000000u);
    g_hcc_test_event_gt->test_data.throughput = temp;
    g_hcc_test_event_gt->test_data.trans_time = us_to_s;
}

OAL_STATIC void hcc_test_throughput_gen(void)
{
    if (g_hcc_test_event_gt->test_data.mode_idx == HCC_TEST_CASE_TX) {
        hcc_test_throughput_cac(g_hcc_test_event_gt->test_data.total_sent_bytes,
                                g_hcc_test_event_gt->start_time,
                                g_hcc_test_event_gt->last_time);
    } else if (g_hcc_test_event_gt->test_data.mode_idx == HCC_TEST_CASE_RX) {
        hcc_test_throughput_cac(g_hcc_test_event_gt->test_data.total_rcvd_bytes,
                                g_hcc_test_event_gt->start_time,
                                g_hcc_test_event_gt->last_time);
    } else if (g_hcc_test_event_gt->test_data.mode_idx == HCC_TEST_CASE_LOOP) {
        hcc_test_throughput_cac(g_hcc_test_event_gt->test_data.total_rcvd_bytes +
                                g_hcc_test_event_gt->test_data.total_sent_bytes,
                                g_hcc_test_event_gt->start_time,
                                g_hcc_test_event_gt->last_time);
    }
}
OAL_STATIC void hcc_test_rcvd_data(oal_netbuf_stru *pst_netbuf)
{
    int32_t ret;
    int32_t filter_flag = 0;

    /* 计算总共数据包长度 */
    if (oal_unlikely(g_hcc_test_event_gt->test_data.pkt_len != oal_netbuf_len(pst_netbuf))) {
        if (printk_ratelimit()) {
            oal_io_print("[E]recvd netbuf pkt len:%d,but request len:%d\n",
                         oal_netbuf_len(pst_netbuf),
                         g_hcc_test_event_gt->test_data.pkt_len);
        }
        filter_flag = 1;
    }

    if (g_hcc_test_event_gt->verified) {
        int32_t i;
        int32_t flag = 0;
        uint8_t *data = oal_netbuf_data(pst_netbuf);
        for (i = 0; i < oal_netbuf_len(pst_netbuf); i++) {
            if (*(data + i) != g_hcc_test_event_gt->test_value) {
                flag = 1;
                oal_io_print("[E]data wrong, [i:%d] value:%x should be %x\n",
                             i, *(data + i), g_hcc_test_event_gt->test_value);
                break;
            }
        }

        if (flag) {
            oal_print_hex_dump(data, oal_netbuf_len(pst_netbuf), HEX_DUMP_GROUP_SIZE, "hcc rx verified ");
            filter_flag = 1;
        }
    }

    if (!filter_flag) {
        /* filter_flag=1 时接收的数据包不符合要求，则过滤掉 */
        g_hcc_test_event_gt->test_data.pkt_rcvd++;
        g_hcc_test_event_gt->test_data.total_rcvd_bytes += oal_netbuf_len(pst_netbuf);
        g_hcc_test_event_gt->last_time = ktime_get();
    }

    if (g_hcc_test_event_gt->test_data.mode_idx != HCC_TEST_CASE_RX) {
        return;
    }
    if (time_after(jiffies, (g_hcc_test_event_gt->start_tick + g_hcc_test_event_gt->tick_timeout))) {
        if (!g_hcc_test_event_gt->rx_stop) {
            oal_io_print("RxTestTimeIsUp\n");
            ret = hcc_send_message(hcc_get_handler(HCC_EP_GT_DEV), H2D_MSG_STOP_SDIO_TEST);
            if (ret) {
                oal_io_print("send message failed, ret=%d", ret);
            }
            g_hcc_test_event_gt->rx_stop = 1;
        }
    }
}

OAL_STATIC void hcc_test_rcvd_cmd(oal_netbuf_stru *pst_netbuf)
{
    int32_t ret;
    hcc_test_cmd_stru cmd;
    ret = memcpy_s(&cmd, sizeof(hcc_test_cmd_stru), oal_netbuf_data(pst_netbuf), sizeof(hcc_test_cmd_stru));
    if (ret != EOK) {
        oal_io_print("memcpy_s err!\n");
    }
    switch (cmd.cmd_type) {
        case HCC_TEST_CMD_STOP_TEST:
            ret = memcpy_s(&g_hcc_test_event_gt->test_data.trans_info, sizeof(hsdio_trans_test_info),
                hcc_get_test_cmd_data(oal_netbuf_data(pst_netbuf)), sizeof(hsdio_trans_test_info));
            if (ret != EOK) {
                oal_io_print("memcpy_s err!\n");
            }
            g_hcc_test_event_gt->last_time = ktime_get();
            oal_io_print("hcc_test_rcvd:cmd %d recvd!\n", cmd.cmd_type);
            oal_complete(&g_hcc_test_event_gt->test_trans_done);
            break;
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
        case HCC_TEST_CMD_PCIE_MAC_LOOPBACK_TST:
        case HCC_TEST_CMD_PCIE_PHY_LOOPBACK_TST:
        case HCC_TEST_CMD_PCIE_REMOTE_PHY_LOOPBACK_TST:
            ret = memcpy_s(&g_pcie_test_request_ack, sizeof(g_pcie_test_request_ack),
                (void *)hcc_get_test_cmd_data(oal_netbuf_data(pst_netbuf)), sizeof(g_pcie_test_request_ack));
            if (ret != EOK) {
                oal_io_print("memcpy_s err!\n");
            }
            oal_complete(&g_pcie_test_trans_done);
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie test done type=%s",
                (cmd.cmd_type == HCC_TEST_CMD_PCIE_MAC_LOOPBACK_TST) ? "mac" : "phy");
            break;
#endif
        default:
            break;
    }
}

/*
 * Prototype    : hcc_test_rcvd
 * Description  : test pkt rcvd
 * Input        : main_type   uint8_t
 *                test_type   uint8_t
 *                skb         sk_buff handle
 *                context     context handle
 * Return Value : succ or fail
 */
OAL_STATIC int32_t hcc_test_rcvd(struct hcc_handler *hcc, uint8_t stype,
                                 hcc_netbuf_stru *pst_hcc_netbuf,
                                 uint8_t *pst_context)
{
    oal_netbuf_stru *pst_netbuf = pst_hcc_netbuf->pst_netbuf;
    oal_reference(pst_context);
    oal_reference(hcc);

    if (oal_likely(stype == HCC_TEST_SUBTYPE_DATA)) {
        hcc_test_rcvd_data(pst_netbuf);
    } else if (stype == HCC_TEST_SUBTYPE_CMD) {
        hcc_test_rcvd_cmd(pst_netbuf);
    } else {
        /* unkown subtype */
        oal_io_print("receive unkown stype:%d\n", stype);
    }

    oal_netbuf_free(pst_netbuf);

    return OAL_SUCC;
}

/*
 * Prototype    : hcc_test_sent
 * Description  : test pkt sent
 * Input        : uint8_t  test_type, uint8_t gen_type
 * Return Value : succ or fail
 */
OAL_STATIC int32_t hcc_test_sent(struct hcc_handler *hcc, struct hcc_transfer_param *param, uint16_t start_cmd)
{
    int32_t ret;
    uint8_t pad_payload = g_hcc_test_event_gt->pad_payload;
    oal_netbuf_stru *pst_netbuf;
    /*
    * 1) alloc memory for skb,
    * 2) skb free when send after dequeue from tx queue
    * */
    pst_netbuf = hcc_netbuf_alloc(g_hcc_test_event_gt->test_data.pkt_len + pad_payload);
    if (pst_netbuf == NULL) {
        oal_io_print("hwifi alloc skb fail.\n");
        return -OAL_EFAIL;
    }

    if (pad_payload) {
        oal_netbuf_reserve(pst_netbuf, pad_payload);
    }

    if (g_hcc_test_event_gt->test_value) {
        memset_s(oal_netbuf_put(pst_netbuf, g_hcc_test_event_gt->test_data.pkt_len),
                 g_hcc_test_event_gt->test_data.pkt_len,
                 g_hcc_test_event_gt->test_value, g_hcc_test_event_gt->test_data.pkt_len);
    } else {
        oal_netbuf_put(pst_netbuf, g_hcc_test_event_gt->test_data.pkt_len);
    }

    if (start_cmd == HCC_TEST_SUBTYPE_DATA) {
        g_hcc_test_event_gt->test_data.total_sent_bytes += oal_netbuf_len(pst_netbuf);
    }

    ret = hcc_tx(hcc, pst_netbuf, param);
    if (ret != OAL_SUCC) {
        oal_netbuf_free(pst_netbuf);
    }

    return ret;
}

OAL_STATIC int32_t hcc_send_test_cmd(uint8_t *cmd, int32_t hdr_len, int32_t cmd_len)
{
    int32_t ret;
    oal_netbuf_stru *pst_netbuf = NULL;
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_GT_DEV);
    if (hcc == NULL) {
        return -OAL_EFAIL;
    }

    pst_netbuf = hcc_netbuf_alloc(cmd_len);
    if (pst_netbuf == NULL) {
        oal_io_print("hwifi alloc skb fail.\n");
        return -OAL_EFAIL;
    }

    if (hdr_len > cmd_len) {
        hdr_len = cmd_len;
    }

    if (memcpy_s(oal_netbuf_put(pst_netbuf, cmd_len), cmd_len, cmd, hdr_len) != EOK) {
        oal_netbuf_free(pst_netbuf);
        oal_io_print("memcpy_s error, destlen=%d, srclen=%d\n ", cmd_len, hdr_len);
        return -OAL_EFAIL;
    }

    hcc_hdr_param_init(&st_hcc_transfer_param,
                       HCC_ACTION_TYPE_TEST,
                       HCC_TEST_SUBTYPE_CMD,
                       0,
                       HCC_FC_WAIT,
                       g_hcc_test_event_gt->hcc_test_queue);
    ret =  hcc_tx(hcc, pst_netbuf, &st_hcc_transfer_param);
    if (ret != OAL_SUCC) {
        oal_netbuf_free(pst_netbuf);
    }

    return ret;
}

OAL_STATIC int32_t hcc_test_rx_start(uint16_t start_cmd)
{
    uint32_t cmd_len;
    int32_t ret;
    hcc_test_cmd_stru *pst_cmd = NULL;
    hcc_trans_test_rx_info *pst_rx_info = NULL;
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_GT_DEV);

    if (hcc == NULL) {
        return -OAL_EFAIL;
    }
    cmd_len = sizeof(hcc_test_cmd_stru) + sizeof(hcc_trans_test_rx_info);
    pst_cmd = (hcc_test_cmd_stru *)oal_memalloc(cmd_len);
    if (pst_cmd == NULL) {
        return -OAL_EFAIL;
    }

    oal_init_completion(&g_hcc_test_event_gt->test_trans_done);
    g_hcc_test_event_gt->test_data.pkt_rcvd = 0;
    g_hcc_test_event_gt->test_data.pkt_sent = 0;
    g_hcc_test_event_gt->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event_gt->test_data.total_sent_bytes = 0;
    g_hcc_test_event_gt->test_data.throughput = 0;
    g_hcc_test_event_gt->test_data.trans_time = 0;
    g_hcc_test_event_gt->start_tick = jiffies;
    g_hcc_test_event_gt->last_time = g_hcc_test_event_gt->start_time = ktime_get();

    memset_s((void *)pst_cmd, cmd_len, 0, cmd_len);
    pst_cmd->cmd_type = start_cmd;
    pst_cmd->cmd_len = cmd_len;

    pst_rx_info = (hcc_trans_test_rx_info *)hcc_get_test_cmd_data(pst_cmd);

    pst_rx_info->total_trans_pkts = g_hcc_test_event_gt->test_data.pkt_gen;
    pst_rx_info->pkt_len = g_hcc_test_event_gt->test_data.pkt_len;
    pst_rx_info->pkt_value = g_hcc_test_event_gt->test_value;

    if (hcc_send_test_cmd((uint8_t *)pst_cmd, pst_cmd->cmd_len, pst_cmd->cmd_len) != OAL_SUCC) {
        oal_free(pst_cmd);
        return -OAL_EFAIL;
    }

    oal_free(pst_cmd);

    g_hcc_test_event_gt->last_time = ktime_get();

    /* 等待回来的CMD命令 */
    ret = wait_for_completion_interruptible(&g_hcc_test_event_gt->test_trans_done);
    if (ret < 0) {
        oal_io_print("Test Event  terminated ret=%d\n", ret);
        ret = -OAL_EFAIL;
        hcc_send_message(hcc, H2D_MSG_STOP_SDIO_TEST);
    }

    if (g_test_force_stop) {
        hcc_send_message(hcc, H2D_MSG_STOP_SDIO_TEST);
        g_test_force_stop = 0;
        oal_msleep(100); // wait 100ms for stop message
    }

    oal_complete(&g_hcc_test_event_gt->test_done);
    return ret;
}

OAL_STATIC int32_t hcc_test_wait_trans_done(hcc_test_cmd_stru *cmd)
{
    int32_t ret;
    int32_t retry_count = 0;

    forever_loop() {
        ret = wait_for_completion_interruptible_timeout(&g_hcc_test_event_gt->test_trans_done,
            oal_msecs_to_jiffies(5000));
        if (ret < 0) {
            oal_io_print("Test Event  terminated ret=%d\n", ret);
            ret = -OAL_EFAIL;
            hcc_print_current_trans_info(0);
            hcc_send_test_cmd((uint8_t *)cmd, sizeof(*cmd), cmd->cmd_len);
        } else if (ret == 0) {
            /* cmd response timeout */
            if (retry_count++ < 1) {
                oal_msleep(100);
                hcc_send_test_cmd((uint8_t *)cmd, sizeof(*cmd), cmd->cmd_len);
                g_hcc_test_event_gt->last_time = ktime_get();
                oal_io_print("resend the stop cmd!retry count:%d\n", retry_count);
                continue;
            } else {
                oal_io_print("resend the stop cmd timeout!retry count:%d\n", retry_count);
                ret = -OAL_EFAIL;
            }
        } else {
            if (g_test_force_stop) {
                hcc_send_test_cmd((uint8_t *)cmd, sizeof(*cmd), cmd->cmd_len);
                g_hcc_test_event_gt->last_time = ktime_get();
                g_test_force_stop = 0;
                oal_io_print("normal start force stop\n");
                oal_msleep(100);
            }
            ret = OAL_SUCC;
        }
        break;
    }

    oal_complete(&g_hcc_test_event_gt->test_done);
    return ret;
}

OAL_STATIC int32_t hcc_test_normal_start(uint16_t start_cmd)
{
    int32_t ret, i;
    hcc_test_cmd_stru cmd = {0};
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler *hcc = NULL;

    hcc_hdr_param_init(&st_hcc_transfer_param, HCC_ACTION_TYPE_TEST, HCC_TEST_SUBTYPE_DATA, 0, HCC_FC_WAIT,
                       g_hcc_test_event_gt->hcc_test_queue);
    hcc = hcc_get_handler(HCC_EP_GT_DEV);
    if (hcc == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wifi is closed!");
        return -OAL_EFAIL;
    }

    oal_init_completion(&g_hcc_test_event_gt->test_trans_done);
    g_hcc_test_event_gt->test_data.pkt_rcvd = 0;
    g_hcc_test_event_gt->test_data.pkt_sent = 0;
    g_hcc_test_event_gt->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event_gt->test_data.total_sent_bytes = 0;
    g_hcc_test_event_gt->test_data.throughput = 0;
    g_hcc_test_event_gt->test_data.trans_time = 0;

    cmd.cmd_type = start_cmd;
    cmd.cmd_len = sizeof(hcc_test_cmd_stru) + sizeof(hsdio_trans_test_info);
    if (hcc_send_test_cmd((uint8_t *)&cmd, sizeof(cmd), cmd.cmd_len) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    /* wait the device recv the cmd,change the test mode! */
    oal_msleep(50);

    g_hcc_test_event_gt->last_time = g_hcc_test_event_gt->start_time = ktime_get();
    g_hcc_test_event_gt->start_tick = jiffies;

    for (i = 0; i < g_hcc_test_event_gt->test_data.pkt_gen; i++) {
        ret = hcc_test_sent(hcc, &st_hcc_transfer_param, HCC_TEST_SUBTYPE_DATA);
        if (ret < 0) {
            oal_io_print("hcc test gen pkt send fail.\n");
            break;
        }

        g_hcc_test_event_gt->test_data.pkt_sent++;
        g_hcc_test_event_gt->last_time = ktime_get();

        if (time_after(jiffies, (g_hcc_test_event_gt->start_tick + g_hcc_test_event_gt->tick_timeout))) {
            oal_io_print("TestTimeIsUp\n");
            break;
        }

        if (oal_unlikely(g_hcc_test_event_gt->started == OAL_FALSE)) {
            ret = -OAL_EFAIL;
            break;
        }
    }

    cmd.cmd_type = HCC_TEST_CMD_STOP_TEST;
    hcc_send_test_cmd((uint8_t *)&cmd, sizeof(cmd), cmd.cmd_len);
    g_hcc_test_event_gt->last_time = ktime_get();

    return hcc_test_wait_trans_done(&cmd);
}

OAL_STATIC int32_t hcc_test_start(uint16_t start_cmd)
{
    oal_io_print("%s Test start.\n", g_hcc_test_stru[g_hcc_test_event_gt->test_data.mode_idx].mode);
    if (g_hcc_test_event_gt->test_data.mode_idx == HCC_TEST_CASE_RX) {
        return hcc_test_rx_start(start_cmd);
    } else {
        return hcc_test_normal_start(start_cmd);
    }
}

/*
 * Prototype    :hcc_test_work
 * Description  :sdio test work_struct function
 * Input        :main_type char
 * Return Value : succ or fail
 */
OAL_STATIC void hcc_test_work(oal_work_stru *work)
{
    uint16_t start_cmd;
    int32_t ret;

    start_cmd = g_hcc_test_stru[g_hcc_test_event_gt->test_data.mode_idx].start_cmd;

    /* hcc test start */
    ret = hcc_test_start(start_cmd);
    if (ret == -OAL_EFAIL) {
        g_hcc_test_event_gt->errorno = ret;
        oal_complete(&g_hcc_test_event_gt->test_done);
        oal_io_print("hcc test work start test pkt send fail. ret = %d\n", ret);
        return;
    }
}

#define hcc_test_fill_print_buf(fmt, arg...) \
    do { \
        ret = snprintf_s(buf + count, buf_len - count, buf_len - count - 1, fmt, ##arg); \
        count += ret >= 0 ? ret : 0; \
    } while (0)

OAL_STATIC ssize_t hcc_test_print_thoughput(char *buf, uint32_t buf_len)
{
    int ret;
    int32_t count = 0;
    const char *mode_str = NULL;
    int32_t tmp_mode_idx;

    if (buf == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "buf is null");
        return 0;
    }

    tmp_mode_idx = g_hcc_test_event_gt->test_data.mode_idx;
    mode_str = "unknown";
    if ((tmp_mode_idx >= 0) && (tmp_mode_idx < oal_array_size(g_hcc_test_stru))) {
        mode_str = g_hcc_test_stru[tmp_mode_idx].mode;
    }
    hcc_test_throughput_gen();
    hcc_test_fill_print_buf("Test_Mode: %s\n", mode_str);
    hcc_test_fill_print_buf("Actual sent %d pkts, request %llu bytes\n",
        g_hcc_test_event_gt->test_data.pkt_sent,
        ((uint64_t)g_hcc_test_event_gt->test_data.pkt_sent) *
        (uint64_t)g_hcc_test_event_gt->test_data.pkt_len);
    hcc_test_fill_print_buf("Actual rcvd %d pkts, request %llu bytes\n",
        g_hcc_test_event_gt->test_data.pkt_rcvd,
        ((uint64_t)g_hcc_test_event_gt->test_data.pkt_rcvd) *
        (uint64_t)g_hcc_test_event_gt->test_data.pkt_len);
    hcc_test_fill_print_buf("PayloadSend %llu bytes, ActualSend  %llu bytes\n",
        g_hcc_test_event_gt->test_data.total_sent_bytes,
        g_hcc_test_event_gt->test_data.trans_info.total_h2d_trans_bytes);
    hcc_test_fill_print_buf("PayloadRcvd %llu bytes, ActualRecv  %llu bytes\n",
        g_hcc_test_event_gt->test_data.total_rcvd_bytes,
        g_hcc_test_event_gt->test_data.trans_info.total_d2h_trans_bytes);
    hcc_test_fill_print_buf("Hcc Utilization Ratio %llu\n",
        hcc_test_utilization_ratio_gen(g_hcc_test_event_gt->test_data.total_sent_bytes +
        g_hcc_test_event_gt->test_data.total_rcvd_bytes,
        g_hcc_test_event_gt->test_data.trans_info.total_h2d_trans_bytes +
        g_hcc_test_event_gt->test_data.trans_info.total_d2h_trans_bytes));
    hcc_test_fill_print_buf("TxPackageLoss %u, pkt_sent: %d actual_tx_pkts: %u\n",
        hcc_test_tx_pkt_loss_gen(g_hcc_test_event_gt->test_data.pkt_sent,
        g_hcc_test_event_gt->test_data.trans_info.actual_tx_pkts),
        g_hcc_test_event_gt->test_data.pkt_sent, g_hcc_test_event_gt->test_data.trans_info.actual_tx_pkts);
    hcc_test_fill_print_buf("Requet Generate %d pkts\n", g_hcc_test_event_gt->test_data.pkt_gen);
    hcc_test_fill_print_buf("Per-package Length %d\n", g_hcc_test_event_gt->test_data.pkt_len);
    hcc_test_fill_print_buf("TranserTimeCost %llu Seconds, %llu microsecond\n",
        g_hcc_test_event_gt->test_data.trans_time, g_hcc_test_event_gt->test_data.trans_time_us);
    hcc_test_fill_print_buf("Throughput %u Mbps\n", (int32_t)g_hcc_test_event_gt->test_data.throughput);

    return count;
}

/*
 * Prototype    : hcc_test_get_para
 * Description  : get test param
 * Input        : dev   device_handle
 *                attr  device_attribute handle
 *                buf   data buf handle
 * Return Value : get test param
 */
OAL_STATIC ssize_t hcc_test_get_para(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    if (dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev is null");
        return 0;
    }

    if (attr == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "attr is null");
        return 0;
    }

    return hcc_test_print_thoughput(buf, PAGE_SIZE - 1);
}

/*
 * Prototype    : hcc_test_set_para
 * Description  : set test param
 */
OAL_STATIC ssize_t hcc_test_set_para(struct kobject *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    hcc_test_data data = {0};
    int32_t tmp_pkt_len;
    int32_t tmp_pkt_gen;
    char mode[OAL_SAVE_MODE_BUFF_SIZE] = {0};
    char *tmp = mode;
    int32_t i;

    if (buf == NULL) {
        oal_io_print("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        oal_io_print("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        oal_io_print("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if ((count == 0) || (count >= sizeof(mode)) || (buf[count] != '\0')) {
        oal_io_print("input illegal!%s\n", __FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf_s(buf, "%15s %d %d", mode, sizeof(mode), &tmp_pkt_len, &tmp_pkt_gen) < 1)) {
        oal_io_print("error input,must input more than 1 arguments!\n");
        return -OAL_EINVAL;
    }

    for (i = 0; i < oal_array_size(g_hcc_test_stru); i++) {
        /* find mode if match */
        if (sysfs_streq(g_hcc_test_stru[i].mode, tmp)) {
            break;
        }
    }

    if (i == oal_array_size(g_hcc_test_stru)) {
        oal_io_print("unknown test mode.%s\n", mode);
        return -OAL_EINVAL;
    }

    data.pkt_len = tmp_pkt_len;
    data.pkt_gen = tmp_pkt_gen;
    data.mode_idx = i;
    data.trans_timeout = ~0UL;

    if (hcc_test_set_case(&data) != OAL_SUCC) {
        oal_io_print("hcc test set fail\n");
    }

    return count;
}


OAL_STATIC struct kobj_attribute g_dev_attr_test =
    __ATTR(test, S_IRUGO | S_IWUSR, hcc_test_get_para, hcc_test_set_para);

OAL_STATIC struct attribute *g_hcc_test_sysfs_entries[] = {
    &g_dev_attr_test.attr,
    NULL
};

OAL_STATIC struct attribute_group g_hcc_test_attribute_group = {
    .name = "test",
    .attrs = g_hcc_test_sysfs_entries,
};

int hcc_test_set_case(hcc_test_data *data)
{
    int ret;
    int errorno;
    if (oal_warn_on(data == NULL)) {
        return -OAL_EINVAL;
    }

    if (oal_unlikely(g_hcc_test_event_gt->test_workqueue == NULL)) {
        oal_io_print("wifi probe failed, please retry.\n");
        return -OAL_EBUSY;
    }

    mutex_lock(&g_hcc_test_event_gt->mutex_lock);
    if (g_hcc_test_event_gt->started == OAL_TRUE) {
        oal_io_print("sdio test task is processing, wait for end and reinput.\n");
        mutex_unlock(&g_hcc_test_event_gt->mutex_lock);
        return -OAL_EINVAL;
    }

    oal_io_print("%s Test Start,test pkts:%d,pkt len:%d\n",
                 g_hcc_test_stru[data->mode_idx].mode, data->pkt_gen, data->pkt_len);

    g_hcc_test_event_gt->started = OAL_TRUE;
    g_hcc_test_event_gt->rx_stop = 0;
    g_hcc_test_event_gt->errorno = OAL_SUCC;
    g_hcc_test_event_gt->tick_timeout = oal_msecs_to_jiffies(data->trans_timeout);

    memcpy_s(&g_hcc_test_event_gt->test_data, sizeof(hcc_test_data), data, sizeof(hcc_test_data));

    g_test_force_stop = 0;
    oal_reinit_completion(g_hcc_test_event_gt->test_done);

    queue_work(g_hcc_test_event_gt->test_workqueue, &g_hcc_test_event_gt->test_work);
    ret = wait_for_completion_interruptible(&g_hcc_test_event_gt->test_done);
    if (ret < 0) {
        oal_io_print("Test Event  terminated ret=%d\n", ret);
        g_hcc_test_event_gt->started = OAL_FALSE;
        oal_complete(&g_hcc_test_event_gt->test_trans_done);
        g_test_force_stop = 1;
        cancel_work_sync(&g_hcc_test_event_gt->test_work);
    }

    oal_io_print("Test Done.ret=%d\n", g_hcc_test_event_gt->errorno);

    hcc_test_throughput_gen();

    memcpy_s(data, sizeof(hcc_test_data), &g_hcc_test_event_gt->test_data, sizeof(hcc_test_data));
    g_hcc_test_event_gt->started = OAL_FALSE;
    errorno = g_hcc_test_event_gt->errorno;
    mutex_unlock(&g_hcc_test_event_gt->mutex_lock);
    return errorno;
}

int32_t hcc_test_init_event(void)
{
    /* alloc memory for perf_action pointer */
    g_hcc_test_event_gt = kzalloc(sizeof(*g_hcc_test_event_gt), GFP_KERNEL);
    if (g_hcc_test_event_gt == NULL) {
        oal_io_print("error kzalloc g_hcc_test_event_gt mem.\n");
        return -OAL_ENOMEM;
    }

    g_hcc_test_event_gt->hcc_test_queue = DATA_LO_QUEUE;
    g_hcc_test_event_gt->test_value = 0x5a;

    /* mutex lock init */
    mutex_init(&g_hcc_test_event_gt->mutex_lock);

    oal_init_completion(&g_hcc_test_event_gt->test_done);
    oal_init_completion(&g_hcc_test_event_gt->test_trans_done);

    /* init hcc_test param */
    g_hcc_test_event_gt->test_data.mode_idx = -1;
    g_hcc_test_event_gt->test_data.pkt_len = 0;
    g_hcc_test_event_gt->test_data.pkt_sent = 0;
    g_hcc_test_event_gt->test_data.pkt_gen = 0;
    g_hcc_test_event_gt->started = OAL_FALSE;

    /* create workqueue */
    g_hcc_test_event_gt->test_workqueue = create_singlethread_workqueue("gt_test");
    if (g_hcc_test_event_gt->test_workqueue == NULL) {
        oal_io_print("work queue create fail.\n");
        kfree(g_hcc_test_event_gt);
        g_hcc_test_event_gt = NULL;
        return -OAL_ENOMEM;
    }
    INIT_WORK(&g_hcc_test_event_gt->test_work, hcc_test_work);
    return OAL_SUCC;
}

int32_t hcc_test_init_module_gt(struct hcc_handler *hcc)
{
    int32_t ret;
    oal_kobject *pst_root_object = NULL;

    if (oal_unlikely(hcc == NULL)) {
        oal_warn_on(1);
        return OAL_FAIL;
    }

    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        oal_io_print("[E]get root sysfs object failed!\n");
        return -OAL_EFAIL;
    }

    g_sysfs_hcc_object = kobject_create_and_add("hcc_gt", pst_root_object);
    if (g_sysfs_hcc_object == NULL) {
        oal_io_print("[E]object create add failed!\n");
        return -OAL_EFAIL;
    }
    ret = oal_debug_sysfs_create_group(g_sysfs_hcc_object, &g_hcc_test_attribute_group);
    if (ret) {
        oal_io_print("sysfs create sdio_sysfs_entries group fail.ret=%d\n", ret);
        goto free_object;
    }

    ret = hcc_rx_register(hcc, HCC_ACTION_TYPE_TEST, hcc_test_rcvd, NULL);
    if (ret != OAL_SUCC) {
        oal_io_print("error %d register callback for rx.\n", ret);
        ret = -OAL_EFAIL;
        goto free_group;
    }
    ret = hcc_test_init_event();
    if (ret != OAL_SUCC) {
        oal_io_print("error %d init event .\n", ret);
        goto free_rx_register;
    }
    return ret;
free_rx_register:
    hcc_rx_unregister(hcc, HCC_ACTION_TYPE_TEST);
free_group:
    oal_debug_sysfs_remove_group(g_sysfs_hcc_object, &g_hcc_test_attribute_group);
free_object:
    kobject_put(g_sysfs_hcc_object);
    return ret;
}

void hcc_test_exit_module_gt(struct hcc_handler *hcc)
{
    if (g_hcc_test_event_gt->test_workqueue != NULL) {
        destroy_workqueue(g_hcc_test_event_gt->test_workqueue);
        g_hcc_test_event_gt->test_workqueue = NULL;
    }

    oal_debug_sysfs_remove_group(g_sysfs_hcc_object, &g_hcc_test_attribute_group);
    kobject_put(g_sysfs_hcc_object);
    hcc_rx_unregister(hcc, HCC_ACTION_TYPE_TEST);
    kfree(g_hcc_test_event_gt);
    g_hcc_test_event_gt = NULL;
}
#endif
