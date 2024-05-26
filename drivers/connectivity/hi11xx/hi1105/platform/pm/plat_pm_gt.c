

#define HI11XX_LOG_MODULE_NAME     "[GT_PM]"
#define HI11XX_LOG_MODULE_NAME_VAR gt_pm_loglevel
#include <linux/module.h> /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/pm_wakeup.h>
#include "oal_hcc_bus.h"
#include "plat_type.h"
#include "plat_debug.h"
#include "board.h"
#include "plat_pm_gt.h"
#include "factory.h"
#include "plat_pm.h"
#include "oal_dft.h"
#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"
#include "bfgx_exception_rst.h"
#include "lpcpu_feature.h"
#include "securec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_PM_GT_C

static struct gt_pm_s *g_gt_pm_info = NULL;
static pm_callback_stru g_gt_pm_callback = {
    .pm_wakeup_dev = gt_pm_wakeup_dev,
    .pm_state_get = gt_pm_state_get,
    .pm_wakeup_host = gt_pm_wakeup_host,
    .pm_feed_wdg = gt_pm_feed_wdg,
    .pm_wakeup_dev_ack = gt_pm_wakeup_dev_ack,
    .pm_disable = gt_pm_disable_check_wakeup,
};

static RAW_NOTIFIER_HEAD(gt_pm_chain);

static int32_t g_ram_reg_test_result = OAL_SUCC;
static unsigned long long g_ram_reg_test_time = 0;
uint8_t g_gt_custom_cali_done = OAL_FALSE;
uint8_t g_gt_custom_cali_cnt = 0;
uint8_t g_gt_ps_mode = 1;
uint8_t g_gt_fast_ps_dyn_ctl = 0;  // app layer dynamic ctrl enable
uint8_t g_gt_min_fast_ps_idle = 1;
uint8_t g_gt_max_fast_ps_idle = 10;
uint8_t g_gt_auto_ps_screen_on = 5;
uint8_t g_gt_auto_ps_screen_off = 5;

EXPORT_SYMBOL_GPL(g_gt_ps_mode);
EXPORT_SYMBOL_GPL(g_gt_min_fast_ps_idle);
EXPORT_SYMBOL_GPL(g_gt_max_fast_ps_idle);
EXPORT_SYMBOL_GPL(g_gt_auto_ps_screen_on);
EXPORT_SYMBOL_GPL(g_gt_auto_ps_screen_off);
EXPORT_SYMBOL_GPL(g_gt_fast_ps_dyn_ctl);

/* 30000ms/100ms = 300 cnt */
static uint32_t g_gt_slp_req_forbid_limit = (30000) / (GT_SLEEP_TIMER_PERIOD * GT_SLEEP_DEFAULT_CHECK_CNT);
EXPORT_SYMBOL_GPL(g_gt_custom_cali_done);
EXPORT_SYMBOL_GPL(g_gt_custom_cali_cnt);


int32_t gt_pm_register_notifier(struct notifier_block *nb)
{
    return raw_notifier_chain_register(&gt_pm_chain, nb);
}

void gt_pm_unregister_notifier(struct notifier_block *nb)
{
    raw_notifier_chain_unregister(&gt_pm_chain, nb);
}

/*
 * 函 数 名  : gt_pm_get_drv
 * 功能描述  : 获取全局wlan结构
 * 返 回 值  : 初始化返回值，成功或失败原因
 */
struct gt_pm_s *gt_pm_get_drv(void)
{
    return g_gt_pm_info;
}

EXPORT_SYMBOL_GPL(gt_pm_get_drv);

/*
 * 函 数 名  : gt_pm_sleep_request
 * 功能描述  : 发送sleep 请求给device
 * 返 回 值  : SUCC/FAIL
 */
OAL_STATIC uint32_t gt_pm_sleep_request(struct gt_pm_s *gt_pm)
{
    int32_t ret;
    uint32_t time_left;
    unsigned int timeout = hi110x_get_emu_timeout(GT_SLEEP_MSG_WAIT_TIMEOUT);
    gt_pm->sleep_stage = GT_SLEEP_REQ_SND;

    oal_init_completion(&gt_pm->st_sleep_request_ack);

    if (oal_print_rate_limit(PRINT_RATE_MINUTE)) { /* 1分钟打印一次 */
        hcc_bus_chip_info(gt_pm->pst_bus, OAL_FALSE, OAL_FALSE);
    }

    ret = hcc_bus_send_message(gt_pm->pst_bus, H2D_MSG_SLEEP_REQ);
    if (ret != OAL_SUCC) {
        gt_pm->sleep_fail_request++;
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_sleep_request fail !\n");
        return OAL_ERR_CODE_SLEEP_FAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "sleep request send!");
    up(&gt_pm->pst_bus->rx_sema);

    time_left = oal_wait_for_completion_timeout(&gt_pm->st_sleep_request_ack,
                                                (uint32_t)oal_msecs_to_jiffies(timeout));
    if (time_left == 0) {
        gt_pm->sleep_fail_wait_timeout++;
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_sleep_work wait completion fail !\n");
        return OAL_ERR_CODE_SLEEP_FAIL;
    }

    return OAL_SUCC;
}

int32_t gt_pm_allow_sleep_callback(void *data)
{
    struct gt_pm_s *gt_pm = (struct gt_pm_s *)data;

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "gt_pm_allow_sleep_callback");

    gt_pm->sleep_stage = GT_SLEEP_ALLOW_RCV;
    oal_complete(&gt_pm->st_sleep_request_ack);

    return SUCCESS;
}

int32_t gt_pm_disallow_sleep_callback(void *data)
{
    struct gt_pm_s *gt_pm = (struct gt_pm_s *)data;

    if (oal_print_rate_limit(PRINT_RATE_SECOND)) { /* 1s打印一次 */
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "sleep request dev disalow, device busy");
    }

    gt_pm->sleep_stage = GT_SLEEP_DISALLOW_RCV;
    oal_complete(&gt_pm->st_sleep_request_ack);

    return SUCCESS;
}

OAL_STATIC hcc_switch_action g_plat_pm_switch_action;
OAL_STATIC int32_t gt_switch_action_callback(uint32_t dev_id, hcc_bus *old_bus, hcc_bus *new_bus, void *data)
{
    struct gt_pm_s *pst_gt_pm = NULL;

    if (data == NULL) {
        return -OAL_EINVAL;
    }

    if (dev_id != HCC_EP_GT_DEV) {
        /* ignore other wlan dev */
        return OAL_SUCC;
    }

    pst_gt_pm = (struct gt_pm_s *)data;

    /* Update new bus */
    pst_gt_pm->pst_bus = new_bus;
    pst_gt_pm->pst_bus->pst_pm_callback = &g_gt_pm_callback;

    return OAL_SUCC;
}

int32_t gt_pm_dts_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int ret;
    u32 host_gpio_sample_low = 0;
    struct device_node *np = NULL;
    np = of_find_compatible_node(NULL, NULL, DTS_NODE_HI110X_WIFI);
    if (np == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can't find node [%s]", DTS_NODE_HI110X_WIFI);
        return -OAL_ENODEV;
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110X_HOST_GPIO_SAMPLE, &host_gpio_sample_low);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "read prop [%s] fail, ret=%d", DTS_PROP_HI110X_HOST_GPIO_SAMPLE, ret);
        return ret;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "%s=%d", DTS_PROP_HI110X_HOST_GPIO_SAMPLE, host_gpio_sample_low);
#endif
    return OAL_SUCC;
}

int32_t gt_pm_wakeup_done_callback(void *data)
{
    struct gt_pm_s *gt_pm = (struct gt_pm_s *)data;

    oam_info_log0(0, OAM_SF_PWR, "gt_pm_wakeup_done_callback !");
    gt_pm->wakeup_done_callback++;
    gt_pm_wakeup_dev_ack();
    return SUCCESS;
}

int32_t gt_pm_close_done_callback(void *data)
{
    struct gt_pm_s *gt_pm = (struct gt_pm_s *)data;

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_close_done_callback !");

    /* 关闭RX通道，防止SDIO RX thread继续访问SDIO */
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_GT_DEV), OAL_BUS_STATE_RX);

    gt_pm->close_done_callback++;
    oal_complete(&gt_pm->st_close_done);

    oam_warning_log0(0, OAM_SF_PWR, "complete H2D_MSG_PM_WLAN_OFF done!");

    return SUCCESS;
}

void gt_pm_wakeup_work(oal_work_stru *pst_worker)
{
    oal_uint ret;

    oam_info_log0(0, OAM_SF_PWR, "gt_pm_wakeup_work start!\n");

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    ret = gt_pm_wakeup_dev();
    if (oal_unlikely(ret != OAL_SUCC)) {
        declare_dft_trace_key_info("wlan_wakeup_fail", OAL_DFT_TRACE_FAIL);
    }

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));

    declare_dft_trace_key_info("wlan_d2h_wakeup_succ", OAL_DFT_TRACE_SUCC);

    return;
}

int32_t gt_pm_work_submit(struct gt_pm_s *gt_pm, oal_work_stru *pst_worker)
{
    int32_t i_ret = 0;

    if (oal_work_is_busy(pst_worker)) {
        /* If comm worker is processing,
          we need't submit again */
        i_ret = -OAL_EBUSY;
        goto done;
    } else {
        oam_info_log1(0, OAM_SF_PWR, "GT %lX Worker Submit\n", (uintptr_t)pst_worker->func);
        if (queue_work(gt_pm->pst_pm_wq, pst_worker) == false) {
            i_ret = -OAL_EFAIL;
        }
    }
done:
    return i_ret;
}

OAL_STATIC uint32_t gt_pm_sleep_check(struct gt_pm_s *gt_pm)
{
    oal_bool_enum_uint8 en_gt_pause_pm = OAL_FALSE;
    if (gt_pm->gt_pm_enable == OAL_FALSE) {
        gt_pm_feed_wdg();
        return OAL_FAIL;
    }

    if (en_gt_pause_pm == OAL_TRUE) {
        gt_pm_feed_wdg();
        return OAL_FAIL;
    }

    if (gt_pm->gt_dev_state == HOST_ALLOW_TO_SLEEP) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "wakeuped,need not do again");
        gt_pm_feed_wdg();
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

static int gt_pm_submit_sleep_work(struct gt_pm_s *pm_data)
{
    if (pm_data->packet_cnt == 0) {
        pm_data->wdg_timeout_curr_cnt++;
        if ((pm_data->wdg_timeout_curr_cnt >= pm_data->wdg_timeout_cnt)) {
            if (gt_pm_work_submit(pm_data, &pm_data->st_sleep_work) == 0) {
                /* 提交了sleep work后，定时器不重启，避免重复提交sleep work */
                pm_data->sleep_work_submit++;
                pm_data->wdg_timeout_curr_cnt = 0;
                return OAL_SUCC;
            }
            oam_warning_log0(0, OAM_SF_PWR, "gt_pm_sleep_work submit fail,work is running !\n");
        }
    } else {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "plat:gt_pm_wdg_timeout %d have packet %d....",
                             pm_data->wdg_timeout_curr_cnt, pm_data->packet_cnt);

        pm_data->wdg_timeout_curr_cnt = 0;
        pm_data->packet_cnt = 0;

        /* 有报文收发,连续forbid sleep次数清零 */
        pm_data->sleep_fail_forbid = 0;
    }
    return -OAL_EFAIL;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
static void gt_pm_wdg_timeout_stub(unsigned long data)
{
    return;
}
#else
static void gt_pm_wdg_timeout_stub(struct timer_list *t)
{
    return;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
void gt_pm_wdg_timeout(unsigned long data)
#else
void gt_pm_wdg_timeout(struct timer_list *t)
#endif
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    struct gt_pm_s *pm_data = (struct gt_pm_s *)(uintptr_t)data;
#else
    struct gt_pm_s *pm_data = from_timer(pm_data, t, st_watchdog_timer);
#endif

    if (pm_data == NULL) {
        return;
    }
    oal_print_hi11xx_log(HI11XX_LOG_DBG, "gt_pm_wdg_timeout....%d", pm_data->wdg_timeout_curr_cnt);

    /* hcc bus switch process */
    hcc_bus_performance_core_schedule(HCC_EP_GT_DEV);

    g_pm_wlan_pkt_statis.total_trx_pkt_cnt = (g_pm_wlan_pkt_statis.ring_tx_pkt + g_pm_wlan_pkt_statis.h2d_tx_pkt +
            g_pm_wlan_pkt_statis.d2h_rx_pkt + g_pm_wlan_pkt_statis.host_rx_pkt);
    pm_data->packet_cnt += g_pm_wlan_pkt_statis.total_trx_pkt_cnt;

    pm_data->packet_total_cnt += g_pm_wlan_pkt_statis.total_trx_pkt_cnt;
    if (time_after(jiffies, pm_data->packet_check_time)) {
#ifdef _PRE_UART_PRINT_LOG
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "pkt_num:GT[%d]", pm_data->packet_total_cnt);
#else
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "pkt_num:GT[%d]", pm_data->packet_total_cnt);
#endif
        pm_data->packet_check_time = jiffies + msecs_to_jiffies(GT_PACKET_CHECK_TIME);
    }

    memset_s(&g_pm_wlan_pkt_statis, sizeof(pm_wlan_pkt_statis_stru), 0, sizeof(pm_wlan_pkt_statis_stru));

    /* 低功耗关闭时timer不会停 */
    if (pm_data->gt_pm_enable) {
        if (gt_pm_submit_sleep_work(pm_data) == OAL_SUCC) {
            return;
        }
    } else {
        pm_data->packet_cnt = 0;
    }

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "gt_pm_feed_wdg");
    gt_pm_feed_wdg();

    return;
}

OAL_STATIC void sleep_request_host_forbid_print(struct gt_pm_s *gt_pm,
                                                const uint32_t host_forbid_sleep_limit)
{
    if (gt_pm->sleep_request_host_forbid >= host_forbid_sleep_limit) {
        /* 防止频繁打印 */
        if (oal_print_rate_limit(10 * PRINT_RATE_SECOND)) { /* 10s打印一次 */
            int32_t allow_print;
            oam_warning_log2(0, OAM_SF_PWR, "gt_pm_sleep_work host forbid sleep %ld, forbid_cnt:%u",
                             gt_pm->sleep_stage, gt_pm->sleep_request_host_forbid);
            allow_print = oal_print_rate_limit(10 * PRINT_RATE_MINUTE); /* 10分钟打印一次 */
            hcc_bus_print_trans_info(gt_pm->pst_bus,
                                     allow_print ?
                                     (HCC_PRINT_TRANS_FLAG_DEVICE_STAT | HCC_PRINT_TRANS_FLAG_DEVICE_REGS) : 0x0);
        }
    } else {
        /* 防止频繁打印 */
        if (oal_print_rate_limit(10 * PRINT_RATE_SECOND)) { /* 10s打印一次 */
            oam_warning_log2(0, OAM_SF_PWR, "gt_pm_sleep_work host forbid sleep %ld, forbid_cnt:%u",
                             gt_pm->sleep_stage, gt_pm->sleep_request_host_forbid);
        }
    }
}

OAL_STATIC int32_t gt_pm_sleep_cmd_send(struct gt_pm_s *gt_pm)
{
    int32_t ret;
    uint8_t retry;

    oal_wlan_gpio_intr_enable(hbus_to_dev(gt_pm->pst_bus), OAL_FALSE);

    gt_pm->gt_dev_state = HOST_ALLOW_TO_SLEEP;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt sleep cmd send, pkt_num:[%d]", gt_pm->packet_total_cnt);

    for (retry = 0; retry < WLAN_SDIO_MSG_RETRY_NUM; retry++) {
        ret = hcc_bus_sleep_request(gt_pm->pst_bus);
        if (ret == OAL_SUCC) {
            break;
        }
        oam_error_log2(0, OAM_SF_PWR, "sleep_dev retry %d ret = %d", retry, ret);
        oal_msleep(SLEEP_10_MSEC);
    }

    /* after max retry still fail,log error */
    if (ret != OAL_SUCC) {
        gt_pm->sleep_fail_set_reg++;
        declare_dft_trace_key_info("wlan_sleep_cmd_fail", OAL_DFT_TRACE_FAIL);
        oam_error_log1(0, OAM_SF_PWR, "sleep_dev Fail ret = %d\r\n", ret);
        gt_pm->gt_dev_state = HOST_DISALLOW_TO_SLEEP;
        oal_wlan_gpio_intr_enable(hbus_to_dev(gt_pm->pst_bus), OAL_TRUE);
        return OAL_FAIL;
    }

    oal_wlan_gpio_intr_enable(hbus_to_dev(gt_pm->pst_bus), OAL_TRUE);

    gt_pm->sleep_fail_forbid_cnt = 0;
    gt_pm->sleep_fail_forbid = 0;

    return OAL_SUCC;
}

OAL_STATIC void trigger_bus_execp_check(struct gt_pm_s *gt_pm)
{
    if (hi11xx_get_os_build_variant() == HI1XX_OS_BUILD_VARIANT_ROOT) {
        if (oal_trigger_bus_exception(gt_pm->pst_bus, OAL_TRUE) == OAL_TRUE) {
            oal_print_hi11xx_log(HI11XX_LOG_WARN, "tigger dump device mem for device_forbid_sleep %d second",
                                 GT_SLEEP_FORBID_CHECK_TIME / 1000);  // 1000 ms
        }
    }
}

OAL_STATIC void gt_pm_sleep_forbid_debug(struct gt_pm_s *gt_pm)
{
    /* 多次debug对比计数，需声明为static变量 */
    static uint64_t g_old_tx, g_old_rx;
    static uint64_t g_new_tx, g_new_rx;

    gt_pm->sleep_fail_forbid++;
    if (gt_pm->sleep_fail_forbid == 1) {
        gt_pm->sleep_forbid_check_time = jiffies + msecs_to_jiffies(GT_SLEEP_FORBID_CHECK_TIME);
    } else if ((gt_pm->sleep_fail_forbid != 0) &&
               (time_after(jiffies, gt_pm->sleep_forbid_check_time))) {
        /* 暂时连续2分钟被forbid sleep，上报一次CHR，看大数据再决定做不做DFR */
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_SLEEP_FORBID);
        gt_pm->sleep_fail_forbid = 0;
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "device_forbid_sleep for %d second",
                             GT_SLEEP_FORBID_CHECK_TIME / 1000);  // 1000 ms
        trigger_bus_execp_check(gt_pm);
    }

    gt_pm->sleep_fail_forbid_cnt++;
    if (gt_pm->sleep_fail_forbid_cnt <= 1) {
        /* get hcc trans count */
        hcc_bus_get_trans_count(gt_pm->pst_bus, &g_old_tx, &g_old_rx);
    } else {
        /* sleep_fail_forbid_cnt > 1 */
        hcc_bus_get_trans_count(gt_pm->pst_bus, &g_new_tx, &g_new_rx);
        /* trans pending */
        if (gt_pm->sleep_fail_forbid_cnt >= g_gt_slp_req_forbid_limit) {
            /* maybe device memleak */
            declare_dft_trace_key_info("wlan_forbid_sleep_print_info", OAL_DFT_TRACE_SUCC);
            oam_warning_log2(0, OAM_SF_PWR,
                "gt_pm_sleep_work device forbid sleep %ld, forbid_cnt:%u try dump device mem info",
                gt_pm->sleep_stage, gt_pm->sleep_fail_forbid_cnt);
            oam_warning_log4(0, OAM_SF_PWR, "old[tx:%u rx:%u] new[tx:%u rx:%u]",
                             g_old_tx, g_old_rx, g_new_tx, g_new_rx);
            gt_pm->sleep_fail_forbid_cnt = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            hcc_print_current_trans_info(1);
#endif
            hcc_bus_send_message(gt_pm->pst_bus, H2D_MSG_DEVICE_MEM_DUMP);
        } else if ((gt_pm->sleep_fail_forbid_cnt % (g_gt_slp_req_forbid_limit / 10)) == 0) { // 打印10次
            oal_print_hi11xx_log(HI11XX_LOG_INFO,
                "sleep request too many forbid %ld, device busy, forbid_cnt:%u, old[tx:%u rx:%u] new[tx:%u rx:%u]",
                gt_pm->sleep_stage, gt_pm->sleep_fail_forbid_cnt,
                (uint32_t)g_old_tx, (uint32_t)g_old_rx, (uint32_t)g_new_tx, (uint32_t)g_new_rx);
        } else {
            oal_print_hi11xx_log(HI11XX_LOG_DBG,
                "sleep request forbid %ld, device busy, forbid_cnt:%u, old[tx:%u rx:%u] new[tx:%u rx:%u]",
                gt_pm->sleep_stage, gt_pm->sleep_fail_forbid_cnt,
                (uint32_t)g_old_tx, (uint32_t)g_old_rx, (uint32_t)g_new_tx, (uint32_t)g_new_rx);
        }
    }

    return;
}

OAL_STATIC uint32_t  gt_pm_sleep_cmd_proc(struct gt_pm_s *gt_pm)
{
    int32_t ret;

    if (gt_pm->sleep_stage == GT_SLEEP_ALLOW_RCV) {
        /* check host */
        ret = gt_pm_sleep_cmd_send(gt_pm);
        if (ret == OAL_FAIL) {
            return OAL_ERR_CODE_SLEEP_FAIL;
        }
    } else {
        gt_pm_sleep_forbid_debug(gt_pm);
        declare_dft_trace_key_info("wlan_forbid_sleep", OAL_DFT_TRACE_SUCC);
        return OAL_ERR_CODE_SLEEP_FORBID;
    }

    gt_pm->sleep_stage = GT_SLEEP_CMD_SND;
    gt_pm->sleep_succ++;
    gt_pm->fail_sleep_count = 0;

    /* 继续持锁500ms, 防止系统频繁进入退出PM */
    oal_wake_lock_timeout(&gt_pm->st_delaysleep_wakelock, GT_WAKELOCK_HOLD_TIME);

    /* pm wakelock非超时锁，释放 */
    oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hold deepsleep_wakelock %d ms", GT_WAKELOCK_HOLD_TIME);

    declare_dft_trace_key_info("wlan_sleep_ok", OAL_DFT_TRACE_SUCC);
    return OAL_SUCC;
}

OAL_STATIC void gt_pm_init_ext2(struct gt_pm_s *gt_pm)
{
    hcc_message_register(hcc_get_handler(HCC_EP_GT_DEV),
                         D2H_MSG_WAKEUP_SUCC,
                         gt_pm_wakeup_done_callback,
                         gt_pm);
    hcc_message_register(hcc_get_handler(HCC_EP_GT_DEV),
                         D2H_MSG_ALLOW_SLEEP,
                         gt_pm_allow_sleep_callback,
                         gt_pm);
    hcc_message_register(hcc_get_handler(HCC_EP_GT_DEV),
                         D2H_MSG_DISALLOW_SLEEP,
                         gt_pm_disallow_sleep_callback,
                         gt_pm);
    hcc_message_register(hcc_get_handler(HCC_EP_GT_DEV),
                         D2H_MSG_POWEROFF_ACK,
                         gt_pm_close_done_callback,
                         gt_pm);

    gt_pm->pst_bus->data_int_count = 0;
    gt_pm->pst_bus->wakeup_int_count = 0;

    gt_pm_dts_init();
}

static uint32_t gt_pm_sleep_request_host(struct gt_pm_s *gt_pm)
{
    const uint32_t host_forbid_sleep_limit = 10;
    int32_t ret = hcc_bus_sleep_request_host(gt_pm->pst_bus);
    if (ret != OAL_SUCC) {
        gt_pm->sleep_request_host_forbid++;
        declare_dft_trace_key_info("wlan_forbid_sleep_host", OAL_DFT_TRACE_SUCC);
        sleep_request_host_forbid_print(gt_pm, host_forbid_sleep_limit);
        return OAL_ERR_CODE_SLEEP_FORBID;
    } else {
        gt_pm->sleep_request_host_forbid = 0;
    }

    return OAL_SUCC;
}

OAL_STATIC void gt_pm_sleep_forbid_proc(struct gt_pm_s *gt_pm)
{
    gt_pm->fail_sleep_count = 0;
    gt_pm_feed_wdg();
    return;
}

OAL_STATIC void gt_pm_sleep_notify(struct gt_pm_s *gt_pm)
{
    /* HOST GT进入低功耗,通知业务侧关闭定时器 */
    if (gt_pm->gt_srv_handler.p_gt_srv_pm_state_notify != NULL) {
        gt_pm->gt_srv_handler.p_gt_srv_pm_state_notify(OAL_FALSE);
    }

    raw_notifier_call_chain(&gt_pm_chain, GT_PM_SLEEP_EVENT, (void *)gt_pm->pst_bus); /* sleep chain */
    return;
}

OAL_STATIC void gt_pm_sleep_fail_proc(struct gt_pm_s *gt_pm)
{
    gt_pm->fail_sleep_count++;
    gt_pm_feed_wdg();

    /* 失败超出门限，启动dfr流程 */
    if (gt_pm->fail_sleep_count > GT_WAKEUP_FAIL_MAX_TIMES) {
        oam_error_log1(0, OAM_SF_PWR, "Now ready to enter DFR process after [%d]times wlan_sleep_fail!",
                       gt_pm->fail_sleep_count);
        gt_pm->fail_sleep_count = 0;
        gt_pm_stop_wdg(gt_pm);
        hcc_bus_exception_submit(gt_pm->pst_bus, GT_WAKEUP_FAIL);
    }
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_WIFI_SLEEP_REQ);
}

void gt_pm_sleep_work(oal_work_stru *pst_worker)
{
#define SLEEP_DELAY_THRESH      0
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    uint32_t ret;

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    if (gt_pm_sleep_check(gt_pm) != OAL_SUCC) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        return;
    }

    // check host
    ret = gt_pm_sleep_request_host(gt_pm);
    if (ret != OAL_SUCC) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        gt_pm_sleep_forbid_proc(gt_pm);
        return;
    }

    ret = gt_pm_sleep_request(gt_pm);
    if (ret == OAL_ERR_CODE_SLEEP_FAIL) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        gt_pm_sleep_fail_proc(gt_pm);
        return;
    }

    ret = gt_pm_sleep_cmd_proc(gt_pm);
    if (ret == OAL_ERR_CODE_SLEEP_FAIL) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        gt_pm_sleep_fail_proc(gt_pm);
        return;
    } else if (ret == OAL_ERR_CODE_SLEEP_FORBID) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        gt_pm_sleep_forbid_proc(gt_pm);
        return;
    }

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));

    gt_pm_sleep_notify(gt_pm);

    return;
}

OAL_STATIC int32_t gt_pm_init_ext1(struct gt_pm_s *gt_pm)
{
    gt_pm->gt_pm_enable = OAL_FALSE;
    gt_pm->apmode_allow_pm_flag = OAL_TRUE; /* 默认允许下电 */

    /* work queue初始化 */
    gt_pm->pst_pm_wq = oal_create_singlethread_workqueue("gt_pm_wq");
    if (gt_pm->pst_pm_wq == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "Failed to create gt_pm_wq!");
        kfree(gt_pm);
        return OAL_FAIL;
    }
    /* register wakeup and sleep work */
    oal_init_work(&gt_pm->st_wakeup_work, gt_pm_wakeup_work);
    oal_init_work(&gt_pm->st_sleep_work, gt_pm_sleep_work);

    /* 初始化芯片自检work */
    oal_init_work(&gt_pm->st_ram_reg_test_work, gt_device_mem_check_work);

    /* sleep timer初始化 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
    init_timer(&gt_pm->st_watchdog_timer);
    gt_pm->st_watchdog_timer.data = (uintptr_t)gt_pm;
    gt_pm->st_watchdog_timer.function = (void *)gt_pm_wdg_timeout_stub;
#else
    timer_setup(&gt_pm->st_watchdog_timer, gt_pm_wdg_timeout_stub, 0);
#endif
    gt_pm->wdg_timeout_cnt = GT_SLEEP_DEFAULT_CHECK_CNT;
    gt_pm->wdg_timeout_curr_cnt = 0;
    gt_pm->packet_cnt = 0;
    gt_pm->packet_total_cnt = 0;
    gt_pm->packet_check_time = 0;
    gt_pm->sleep_forbid_check_time = 0;

    oal_wake_lock_init(&gt_pm->st_pm_wakelock, "gt_pm_wakelock");
    oal_wake_lock_init(&gt_pm->st_delaysleep_wakelock, "gt_delaysleep_wakelock");

    gt_pm->gt_power_state = POWER_STATE_SHUTDOWN;
    gt_pm->gt_dev_state = HOST_ALLOW_TO_SLEEP;
    gt_pm->sleep_stage = GT_SLEEP_STAGE_INIT;

    gt_pm->gt_srv_handler.p_gt_srv_get_pm_pause_func = NULL;
    gt_pm->gt_srv_handler.p_gt_srv_open_notify = NULL;
    gt_pm->gt_srv_handler.p_gt_srv_pm_state_notify = NULL;

    gt_pm->wkup_src_print_en = OAL_FALSE;

    g_gt_pm_info = gt_pm;

    oal_init_completion(&gt_pm->st_open_bcpu_done);
    oal_init_completion(&gt_pm->st_close_bcpu_done);
    oal_init_completion(&gt_pm->st_close_done);
    oal_init_completion(&gt_pm->st_wakeup_done);
    oal_init_completion(&gt_pm->gt_powerup_done);
    oal_init_completion(&gt_pm->st_sleep_request_ack);
    oal_init_completion(&gt_pm->st_halt_bcpu_done);

    return OAL_SUCC;
}

struct gt_pm_s *gt_pm_init(void)
{
    struct gt_pm_s *gt_pm = NULL;
    int32_t ret;

    gt_pm = kzalloc(sizeof(struct gt_pm_s), GFP_KERNEL);
    if (gt_pm == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "no mem to allocate gt_pm_data");
        return NULL;
    }

    memset_s(gt_pm, sizeof(struct gt_pm_s), 0, sizeof(struct gt_pm_s));
    memset_s((void *)&g_plat_pm_switch_action, sizeof(g_plat_pm_switch_action),
             0, sizeof(g_plat_pm_switch_action));
    g_plat_pm_switch_action.name = "plat_pm_gt";
    g_plat_pm_switch_action.switch_notify = gt_switch_action_callback;
    hcc_switch_action_register(&g_plat_pm_switch_action, (void *)gt_pm);

    gt_pm->pst_bus = hcc_get_bus(HCC_EP_GT_DEV);
    if (gt_pm->pst_bus == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc bus is NULL, failed to create gt_pm_wq!");
        kfree(gt_pm);
        return NULL;
    }
    gt_pm->pst_bus->pst_pm_callback = &g_gt_pm_callback;

    /* 圈复杂度优化 */
    ret = gt_pm_init_ext1(gt_pm);
    if (ret == OAL_FAIL) {
        return NULL;
    }

    gt_pm_init_ext2(gt_pm);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_init ok!");
    return gt_pm;
}

oal_uint gt_pm_exit(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return OAL_SUCC;
    }

    gt_pm_stop_wdg(gt_pm);

    oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
    oal_wake_unlock_force(&gt_pm->st_delaysleep_wakelock);

    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_WAKEUP_SUCC);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_WLAN_READY);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_ALLOW_SLEEP);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_DISALLOW_SLEEP);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_POWEROFF_ACK);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_OPEN_BCPU_ACK);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_CLOSE_BCPU_ACK);
    hcc_bus_message_unregister(gt_pm->pst_bus, D2H_MSG_HALT_BCPU);
    hcc_switch_action_unregister(&g_plat_pm_switch_action);

    kfree(gt_pm);

    g_gt_pm_info = NULL;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_exit ok!");

    return OAL_SUCC;
}

uint32_t gt_pm_is_poweron(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return OAL_FALSE;
    }

    if (gt_pm->gt_power_state == POWER_STATE_OPEN) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(gt_pm_is_poweron);

struct gt_srv_callback_handler *gt_pm_get_srv_handler(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return NULL;
    }

    return &gt_pm->gt_srv_handler;
}
EXPORT_SYMBOL_GPL(gt_pm_get_srv_handler);

OAL_STATIC int32_t gt_pm_custom_cali_wait_retry(struct gt_pm_s *gt_pm)
{
    int32_t loglevel;
    int32_t wait_relt;
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_GT_DEV);
    if (oal_unlikely(hcc == NULL)) {
        oam_error_log0(0, OAM_SF_PWR, "hcc is null!\n");
        return OAL_FAIL;
    }

    declare_dft_trace_key_info("gt_wait_custom_cali_fail_retry", OAL_DFT_TRACE_FAIL);
    oam_error_log1(0, OAM_SF_PWR, "gt_pm_wait_custom_cali timeout retry %d", HOST_WAIT_BOTTOM_INIT_TIMEOUT);

    hcc_print_current_trans_info(1);

    loglevel = hcc_set_all_loglevel(HI11XX_LOG_VERBOSE);

    hcc_sched_transfer(hcc);

    wait_relt = oal_wait_for_completion_killable_timeout(&gt_pm->pst_bus->st_device_ready,
                                                         oal_msecs_to_jiffies(HOST_WAIT_INIT_RETRY));
    if (wait_relt == 0) {
        hcc_print_current_trans_info(1);
        hcc_set_all_loglevel(loglevel);
        declare_dft_trace_key_info("wlan_wait_custom_cali_retry_fail", OAL_DFT_TRACE_FAIL);
        oam_error_log1(0, OAM_SF_PWR, "gt_pm_wait_custom_cali timeout retry failed %d", HOST_WAIT_INIT_RETRY);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                             CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_CALL_TIMEOUT);
    } else if (wait_relt == -ERESTARTSYS) {
        oam_warning_log0(0, OAM_SF_CFG, "gt_pm_open::receive interrupt single");
        hcc_set_all_loglevel(loglevel);
        return OAL_FAIL;
    } else {
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                             CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_CALL_TIMEOUT);
    }
    hcc_set_all_loglevel(loglevel);
    return OAL_SUCC;
}

OAL_STATIC int32_t gt_pm_custom_cali(struct gt_pm_s *gt_pm)
{
    int32_t wait_ret;

    // 初始化配置定制化参数
    gt_customize_h2d();

    oal_init_completion(&gt_pm->pst_bus->st_device_ready);

    if (g_custom_process_func.p_custom_cali_func == NULL) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_open::NO g_custom_process_func registered");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                             CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_NO_CUSTOM_CALL);

        return OAL_FAIL;
    }

    /* 如果校准下发成功则等待device ready；否则继续打开gt */
    if (g_custom_process_func.p_custom_cali_func() == OAL_SUCC) {
        wait_ret = oal_wait_for_completion_killable_timeout(&gt_pm->pst_bus->st_device_ready,
                                                            oal_msecs_to_jiffies(HOST_WAIT_BOTTOM_INIT_TIMEOUT));
        if (wait_ret == -ERESTARTSYS) {
            oam_warning_log0(0, OAM_SF_CFG, "gt_pm_open::receive interrupt single");
            return OAL_FAIL;
        }

        // 如果判断超时异常，再尝试一次
        if (wait_ret == 0) {
            if (gt_pm_custom_cali_wait_retry(gt_pm) != OAL_SUCC) {
                return OAL_FAIL;
            }
        }
    }

    return OAL_SUCC;
}

OAL_STATIC void gt_pm_open_notify(struct gt_pm_s *gt_pm)
{
    raw_notifier_call_chain(&gt_pm_chain, GT_PM_POWERUP_EVENT, (void *)gt_pm->pst_bus); /* powerup chain */
    gt_pm_enable();

    /* 将timeout值恢复为默认值，并启动定时器 */
    gt_pm_set_timeout(GT_SLEEP_DEFAULT_CHECK_CNT);

    /* gt开机成功后,通知业务侧 */
    if (gt_pm->gt_srv_handler.p_gt_srv_open_notify != NULL) {
        gt_pm->gt_srv_handler.p_gt_srv_open_notify(OAL_TRUE);
    }
}

OAL_STATIC int32_t gt_pm_open_check(struct gt_pm_s *gt_pm)
{
    if (gt_pm == NULL) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_open:gt_pm is NULL!");
        return OAL_FAIL;
    }

    if (gt_pm->pst_bus == NULL) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_open::get 110x bus failed!");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_POWER_ON_NON_BUS);
        return OAL_FAIL;
    }

    if (gt_pm->gt_power_state == POWER_STATE_OPEN) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm_open::aleady opened");
        return OAL_ERR_CODE_ALREADY_OPEN;
    }

    gt_pm->open_cnt++;
    return OAL_SUCC;
}

int32_t gt_pm_open(void)
{
    int32_t ret;
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    struct pm_top* pm_top_data = pm_get_top();
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_open enter");

    mutex_lock(&(pm_top_data->host_mutex));
    ret = gt_pm_open_check(gt_pm);
    if (ret != OAL_SUCC) {
        mutex_unlock(&(pm_top_data->host_mutex));
        return ret;
    }
    oal_wake_lock_timeout(&gt_pm->st_pm_wakelock, GT_WAKELOCK_NO_TIMEOUT);
    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_idle_sleep_vote DISALLOW::lpcpu_idle_sleep_vote ID_GT 1!");

    ret = gt_power_on();
    if (ret != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_open::wlan_power_on fail!");
        gt_pm->gt_power_state = POWER_STATE_SHUTDOWN;
        oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
        mutex_unlock(&(pm_top_data->host_mutex));
        if (ret != OAL_EINTR) {
            declare_dft_trace_key_info("wlan_power_on_fail", OAL_DFT_TRACE_FAIL);
        }
        return OAL_FAIL;
    }

    if (gt_pm_custom_cali(gt_pm) == OAL_FAIL) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_open::cali fail!");
        gt_power_fail_process(GT_POWER_ON_CALI_FAIL);
        gt_pm->gt_power_state = POWER_STATE_SHUTDOWN;
        oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
        mutex_unlock(&(pm_top_data->host_mutex));
        /* block here to debug */
        oal_chip_error_task_block();
        return OAL_FAIL;
    }

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_open::gt_pm_open SUCC!!");
    declare_dft_trace_key_info("gt_open_succ", OAL_DFT_TRACE_SUCC);
    gt_pm_open_notify(gt_pm);
    mutex_unlock(&(pm_top_data->host_mutex));

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_open);

OAL_STATIC uint32_t gt_pm_close_pre(struct gt_pm_s *gt_pm)
{
    if (gt_pm == NULL) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm is null");
        return OAL_FAIL;
    }

    if (!gt_pm->apmode_allow_pm_flag) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm_close,AP mode,do not shutdown power.");
        return OAL_ERR_CODE_FOBID_CLOSE_DEVICE;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_close start!!");
    gt_pm->close_cnt++;

    if (gt_pm->gt_power_state == POWER_STATE_SHUTDOWN) {
        return OAL_ERR_CODE_ALREADY_CLOSE;
    }

    gt_pm_disable();

    /* GT关闭前,通知业务侧 */
    if (gt_pm->gt_srv_handler.p_gt_srv_open_notify != NULL) {
        gt_pm->gt_srv_handler.p_gt_srv_open_notify(OAL_FALSE);
    }

    gt_pm_stop_wdg(gt_pm);
    gt_pm_info_clean();

    /* mask rx ip data interrupt */
    hcc_bus_rx_int_mask(hcc_get_bus(HCC_EP_GT_DEV));

    return OAL_SUCC;
}

OAL_STATIC uint32_t gt_pm_close_after(struct gt_pm_s *gt_pm)
{
    gt_pm->gt_power_state = POWER_STATE_SHUTDOWN;

    /* unmask rx ip data interrupt */
    hcc_bus_rx_int_unmask(hcc_get_bus(HCC_EP_GT_DEV));
    raw_notifier_call_chain(&gt_pm_chain, GT_PM_POWERDOWN_EVENT, (void *)gt_pm->pst_bus); /* powerdown chain */

    oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
    oal_wake_unlock_force(&gt_pm->st_delaysleep_wakelock);

    hcc_bus_wakelocks_release_detect(gt_pm->pst_bus);

    hcc_bus_reset_trans_info(gt_pm->pst_bus);

    return OAL_SUCC;
}

uint32_t gt_pm_close(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    struct pm_top* pm_top_data = pm_get_top();
    uint32_t ret;

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "gt_pm_close enter");
    mutex_lock(&(pm_top_data->host_mutex));

    ret = gt_pm_close_pre(gt_pm);
    if (ret != OAL_SUCC) {
        mutex_unlock(&(pm_top_data->host_mutex));
        return ret;
    }

    if (gt_power_off() != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_PWR, "wlan_power_off FAIL!\n");
        mutex_unlock(&(pm_top_data->host_mutex));
        declare_dft_trace_key_info("wlan_power_off_fail", OAL_DFT_TRACE_FAIL);
        return OAL_FAIL;
    }

    ret = gt_pm_close_after(gt_pm);
    if (ret != OAL_SUCC) {
        mutex_unlock(&(pm_top_data->host_mutex));
        return ret;
    }

    mutex_unlock(&(pm_top_data->host_mutex));

    hcc_dev_flowctrl_on(hcc_get_handler(HCC_EP_GT_DEV), 0);

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_close succ!\n");
    declare_dft_trace_key_info("wlan_close_succ", OAL_DFT_TRACE_SUCC);
    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_close);

uint32_t gt_pm_close_by_shutdown(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    struct pm_top* pm_top_data = pm_get_top();

    if (gt_pm == NULL) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm is null");
        return OAL_FAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_close_by_shutdown start!!");

    mutex_lock(&(pm_top_data->host_mutex));

    if (gt_pm->gt_power_state == POWER_STATE_SHUTDOWN) {
        mutex_unlock(&(pm_top_data->host_mutex));
        return OAL_ERR_CODE_ALREADY_CLOSE;
    }

    // 关闭低功耗, 否则可能导致低功耗睡眠消息发送失败
    gt_pm->gt_pm_enable = OAL_FALSE;
    gt_pm_stop_wdg(gt_pm);

    if (gt_power_off() != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_close_by_shutdown FAIL!\n");
    }

    gt_pm->gt_power_state = POWER_STATE_SHUTDOWN;

    mutex_unlock(&(pm_top_data->host_mutex));

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_close_by_shutdown succ!\n");
    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_close_by_shutdown);

uint32_t gt_pm_enable(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    if (gt_pm->gt_pm_enable == OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm_enable already enabled!");
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        return OAL_SUCC;
    }

    gt_pm->gt_pm_enable = OAL_TRUE;

    gt_pm_feed_wdg();

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_enable SUCC!");

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_enable);

uint32_t gt_pm_disable_check_wakeup(int32_t flag)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    if (gt_pm->gt_pm_enable == OAL_FALSE) {
        oam_warning_log0(0, OAM_SF_PWR, "gt_pm_disable already disabled!");
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        return OAL_SUCC;
    }

    if (flag == OAL_TRUE) {
        if (gt_pm_wakeup_dev() != OAL_SUCC) {
            oam_warning_log0(0, OAM_SF_PWR, "pm wake up dev fail!");
        }
    }

    gt_pm->gt_pm_enable = OAL_FALSE;

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));

    gt_pm_stop_wdg(gt_pm);

    oal_cancel_work_sync(&gt_pm->st_wakeup_work);
    oal_cancel_work_sync(&gt_pm->st_sleep_work);

    /* when we disable low power, we means never sleep, should hold wakelock */
    oal_wake_lock_timeout(&gt_pm->st_pm_wakelock, GT_WAKELOCK_NO_TIMEOUT);

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_disable hold deepsleep_wakelock SUCC!");

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_disable_check_wakeup);

uint32_t gt_pm_disable(void)
{
    return gt_pm_disable_check_wakeup(OAL_TRUE);
}
EXPORT_SYMBOL_GPL(gt_pm_disable);

oal_uint gt_pm_init_dev(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    int32_t ret;
    hcc_bus *pst_bus = NULL;

    if (gt_pm == NULL) {
        return OAL_FAIL;
    }

    pst_bus = hcc_get_bus(HCC_EP_GT_DEV);
    if (oal_warn_on(pst_bus == NULL)) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_init get non bus!");

        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_PM_INIT_NO_BUS);
        return OAL_FAIL;
    }

    oam_warning_log0(0, OAM_SF_PWR, "gt_pm_init_dev!\n");

    gt_pm->gt_dev_state = HOST_DISALLOW_TO_SLEEP;

    /* wait for bus wakeup */
    ret = down_timeout(&pst_bus->sr_wake_sema, GT_BUS_SEMA_TIME);
    if (ret == -ETIME) {
        oam_error_log0(0, OAM_SF_PWR, "host bus controller is not ready!");
        declare_dft_trace_key_info("gt_controller_wait_init_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_PM_SDIO_NO_READY);
        return OAL_FAIL;
    }
    up(&pst_bus->sr_wake_sema);

    return (oal_uint)hcc_bus_wakeup_request(gt_pm->pst_bus);
}

OAL_STATIC oal_uint gt_pm_pcie_wakeup_cmd(struct gt_pm_s *gt_pm)
{
    int32_t ret;

    oal_init_completion(&gt_pm->st_wakeup_done);
    /* 依赖回来的GPIO 做唤醒，此时回来的消息PCIE 还不确定是否已经唤醒，PCIE通道不可用 */
    oal_wlan_gpio_intr_enable(hbus_to_dev(gt_pm->pst_bus), OAL_FALSE);
    oal_atomic_set(&g_wakeup_dev_wait_ack, 1);
    oal_wlan_gpio_intr_enable(hbus_to_dev(gt_pm->pst_bus), OAL_TRUE);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt wakeup cmd send");
    ret = hcc_bus_wakeup_request(gt_pm->pst_bus);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wakeup request failed ret=%d", ret);
        declare_dft_trace_key_info("gt wakeup cmd send fail", OAL_DFT_TRACE_FAIL);
        (void)ssi_dump_err_regs(SSI_ERR_PCIE_GPIO_WAKE_FAIL);
        return OAL_FAIL;
    }

    gt_pm->gt_dev_state = HOST_DISALLOW_TO_SLEEP;

    return OAL_SUCC;
}

OAL_STATIC oal_uint gt_pm_wakeup_cmd_send(struct gt_pm_s *gt_pm)
{
    int32_t ret;
    hcc_bus *pst_bus = gt_pm->pst_bus;

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wait bus wakeup");

    /* wait for bus wakeup */
    ret = down_timeout(&pst_bus->sr_wake_sema, WLAN_BUS_SEMA_TIME);
    if (ret == -ETIME) {
        gt_pm->wakeup_fail_wait_sdio++;
        oam_error_log0(0, OAM_SF_PWR, "gt controller is not ready!");
        declare_dft_trace_key_info("gt_controller_wait_fail", OAL_DFT_TRACE_FAIL);
        return OAL_ERR_CODE_SEMA_TIMEOUT;
    }
    up(&pst_bus->sr_wake_sema);

    if (gt_pm->pst_bus->bus_type == HCC_BUS_PCIE) {
        if (gt_pm_pcie_wakeup_cmd(gt_pm) == OAL_FAIL) {
            return OAL_ERR_CODE_WAKEUP_FAIL_PROC;
        }
    } else {
        declare_dft_trace_key_info("oal_gt_wakeup_dev final fail", OAL_DFT_TRACE_EXCEP);
        return OAL_ERR_CODE_WAKEUP_FAIL_PROC;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint gt_pm_wakeup_wait_ack(struct gt_pm_s *gt_pm)
{
    uint32_t ret;

    ret = oal_wait_for_completion_timeout(&gt_pm->st_wakeup_done,
                                          (uint32_t)oal_msecs_to_jiffies(WLAN_WAKUP_MSG_WAIT_TIMEOUT));
    if (ret == 0) {
        int32_t sleep_state = hcc_bus_get_sleep_state(gt_pm->pst_bus);
        if ((sleep_state == DISALLOW_TO_SLEEP_VALUE) || (sleep_state < 0)) {
            if (oal_unlikely(sleep_state < 0)) {
                oam_error_log1(0, OAM_SF_PWR, "get state failed, sleep_state=%d", sleep_state);
            }

            gt_pm->wakeup_fail_timeout++;
            oam_warning_log0(0, OAM_SF_PWR, "oal_gt_wakeup_dev SUCC to set 0xf0 = 0");
            hcc_bus_sleep_request(gt_pm->pst_bus);
            gt_pm->gt_dev_state = HOST_ALLOW_TO_SLEEP;

            oam_error_log2(0, OAM_SF_PWR, "gt_pm_wakeup_dev [%d]wait device complete fail,wait time %d ms!",
                           gt_pm->wakeup_err_count, WLAN_WAKUP_MSG_WAIT_TIMEOUT);
            oal_print_hi11xx_log(HI11XX_LOG_INFO,
                                 KERN_ERR "gt_pm_wakeup_dev [%d]wait device complete fail,wait time %d ms!",
                                 gt_pm->wakeup_err_count, WLAN_WAKUP_MSG_WAIT_TIMEOUT);
            return OAL_ERR_CODE_WAKEUP_RETRY;
        } else {
            gt_pm->wakeup_fail_set_reg++;
            oam_error_log0(0, OAM_SF_PWR, "wakeup_dev Fail to set 0xf0 = 0");
            oal_print_hi11xx_log(HI11XX_LOG_INFO, KERN_ERR "wakeup_dev Fail to set 0xf0 = 0");
            gt_pm->gt_dev_state = HOST_ALLOW_TO_SLEEP;
            return OAL_ERR_CODE_WAKEUP_FAIL_PROC;
        }
    }

    declare_dft_trace_key_info("wlan_wakeup_succ", OAL_DFT_TRACE_SUCC);
    gt_pm->wakeup_succ++;
    gt_pm->wakeup_err_count = 0;
    gt_pm->wdg_timeout_curr_cnt = 0;
    gt_pm->packet_cnt = 0;
    gt_pm->packet_check_time = jiffies + msecs_to_jiffies(WLAN_PACKET_CHECK_TIME);
    gt_pm->packet_total_cnt = 0;
    gt_pm->sleep_fail_forbid_cnt = 0;
    return OAL_SUCC;
}

OAL_STATIC void gt_pm_wakeup_fail_proc(struct gt_pm_s *gt_pm)
{
    declare_dft_trace_key_info("wlan_wakeup_fail", OAL_DFT_TRACE_FAIL);
    gt_pm->wakeup_err_count++;

    /* pm唤醒失败超出门限，启动dfr流程 */
    if (gt_pm->wakeup_err_count > GT_WAKEUP_FAIL_MAX_TIMES) {
        oam_error_log1(0, OAM_SF_PWR, "Now ready to enter DFR process after [%d]times wlan_wakeup_fail!",
                       gt_pm->wakeup_err_count);
        gt_pm->wakeup_err_count = 0;
        hcc_bus_exception_submit(gt_pm->pst_bus, GT_WAKEUP_FAIL);
    }
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_WIFI_WKUP_DEV);
}

OAL_STATIC void gt_pm_wakeup_notify(struct gt_pm_s *gt_pm)
{
    /* HOST GT退出低功耗,通知业务侧开启定时器 */
    if (gt_pm->gt_srv_handler.p_gt_srv_pm_state_notify != NULL) {
        gt_pm->gt_srv_handler.p_gt_srv_pm_state_notify(OAL_TRUE);
    }
    oal_usleep_range(500, 510); // 睡眠时间在500到510us之间

    hcc_bus_wakeup_complete(gt_pm->pst_bus);

    raw_notifier_call_chain(&gt_pm_chain, GT_PM_WAKEUP_EVENT, (void *)gt_pm->pst_bus); /* wakeup chain */
}

OAL_STATIC oal_uint gt_pm_wakeup_check(struct gt_pm_s *gt_pm)
{
    if (gt_pm == NULL || gt_pm->pst_bus == NULL) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_wakeup_dev get non bus!\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_PM, CHR_PLAT_DRV_ERROR_PM_WKUP_NON_BUS);
        return OAL_FAIL;
    }

    if (gt_pm->gt_pm_enable == OAL_FALSE) {
        if (gt_pm->gt_dev_state == HOST_ALLOW_TO_SLEEP) {
            /* 唤醒流程没走完不允许发送数据 */
            oam_error_log0(0, OAM_SF_PWR, "wlan pm disabled but state == HOST_ALLOW_TO_SLEEP!\n");
            return OAL_ERR_CODE_WAKEUP_PROCESSING;
        } else {
            return OAL_SUCC;
        }
    }

    if (gt_pm->gt_dev_state == HOST_DISALLOW_TO_SLEEP) {
        return OAL_SUCC;
    }

    return OAL_SUCC_GO_ON;
}

oal_uint gt_pm_wakeup_dev(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    uint32_t ret;
    ktime_t time_start, time_stop;
    uint64_t trans_us;
    int i;

    ret = gt_pm_wakeup_check(gt_pm);
    if (ret != OAL_SUCC_GO_ON) {
        return ret;
    }

    time_start = ktime_get();
    oal_wake_lock_timeout(&gt_pm->st_pm_wakelock, GT_WAKELOCK_NO_TIMEOUT);

    for (i = 0; i <= GT_WAKEUP_FAIL_MAX_TIMES; i++) {
        ret = gt_pm_wakeup_cmd_send(gt_pm);
        if (ret != OAL_SUCC) {
            if (ret == OAL_ERR_CODE_WAKEUP_FAIL_PROC) {
                break;
            } else {
                oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
                return ret;
            }
        }

        ret = gt_pm_wakeup_wait_ack(gt_pm);
        if (ret == OAL_SUCC || ret == OAL_ERR_CODE_WAKEUP_FAIL_PROC) {
            break;
        }
    }

    if (ret != OAL_SUCC) {
        gt_pm_wakeup_fail_proc(gt_pm);
        oal_wake_unlock_force(&gt_pm->st_pm_wakelock);
        return OAL_FAIL;
    }

    gt_pm_feed_wdg();

    gt_pm_wakeup_notify(gt_pm);

    time_stop = ktime_get();
    trans_us = (uint64_t)ktime_to_us(ktime_sub(time_stop, time_start));
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wakeup dev succ, cost %llu us", trans_us);
    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(gt_pm_wakeup_dev);

void gt_pm_wakeup_dev_ack(void)
{
    struct gt_pm_s *gt_pm = NULL;

    if (oal_atomic_read(&g_wakeup_dev_wait_ack)) {
        gt_pm = gt_pm_get_drv();
        if (gt_pm == NULL) {
            return;
        }

        gt_pm->wakeup_dev_ack++;

        oal_complete(&gt_pm->st_wakeup_done);

        oal_atomic_set(&g_wakeup_dev_wait_ack, 0);
    }

    return;
}

oal_uint gt_pm_wakeup_host(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (oal_warn_on(gt_pm == NULL)) {
        oam_error_log0(0, OAM_SF_PWR, "gt_pm_wakeup_host: st_wlan_pm is null \n");
        return -OAL_FAIL;
    }

    oal_wake_lock_timeout(&gt_pm->st_pm_wakelock, GT_WAKELOCK_NO_TIMEOUT);
    oam_info_log0(0, OAM_SF_PWR, "gt_pm_wakeup_host get wakelock\n");

    /* work submit失败，只有work已经提交过了这种场景，wakelock锁不释放 */
    if (gt_pm_work_submit(gt_pm, &gt_pm->st_wakeup_work) != 0) {
        gt_pm->wakeup_fail_submit_work++;
    } else {
        gt_pm->wakeup_succ_work_submit++;
    }

    return OAL_SUCC;
}

void gt_pm_wakeup_dev_lock(void)
{
    oal_uint ret;

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    ret = gt_pm_wakeup_dev();
    if (oal_unlikely(ret != OAL_SUCC)) {
        declare_dft_trace_key_info("wlan_wakeup_fail", OAL_DFT_TRACE_FAIL);
    }

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
}
EXPORT_SYMBOL_GPL(gt_pm_wakeup_dev_lock);

void gt_pm_freq_adjust_work(oal_work_stru *pst_worker)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    if (gt_pm->gt_pm_enable == OAL_FALSE) {
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        return;
    }

    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
}

oal_uint gt_pm_state_get(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    return gt_pm->gt_dev_state;
}

void gt_pm_state_set(struct gt_pm_s *gt_pm, oal_uint state)
{
    gt_pm->gt_dev_state = state;
}

void gt_pm_set_timeout(uint32_t timeout)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return;
    }

    oam_warning_log1(0, OAM_SF_PWR, "gt_pm_set_timeout[%d]", timeout);
    gt_pm->wdg_timeout_cnt = timeout;
    gt_pm->wdg_timeout_curr_cnt = 0;
    gt_pm->packet_cnt = 0;
    gt_pm_feed_wdg();
}
EXPORT_SYMBOL_GPL(gt_pm_set_timeout);

void gt_pm_feed_wdg(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    gt_pm->sleep_feed_wdg_cnt++;

    if (g_download_rate_limit_pps != 0) {
        mod_timer(&gt_pm->st_watchdog_timer, jiffies + msecs_to_jiffies(SLEEP_10_MSEC));
    } else {
        mod_timer(&gt_pm->st_watchdog_timer, jiffies + msecs_to_jiffies(GT_SLEEP_TIMER_PERIOD));
    }
}

int32_t gt_pm_stop_wdg(struct gt_pm_s *pst_gt_pm_info)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gt_pm_stop_wdg");

    pst_gt_pm_info->wdg_timeout_curr_cnt = 0;
    pst_gt_pm_info->packet_cnt = 0;

    if (in_interrupt()) {
        return del_timer(&pst_gt_pm_info->st_watchdog_timer);
    } else {
        return del_timer_sync(&pst_gt_pm_info->st_watchdog_timer);
    }
}

int32_t gt_pm_poweroff_cmd(void)
{
    int32_t ret;

    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    oam_warning_log0(0, OAM_SF_PWR, "Send H2D_MSG_PM_WLAN_OFF cmd");

    hcc_tx_transfer_lock(hcc_get_handler(HCC_EP_GT_DEV));

    if (gt_pm_wakeup_dev() != OAL_SUCC) {
        (void)ssi_dump_err_regs(SSI_ERR_WLAN_POWEROFF_FAIL);
        hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
        return OAL_FAIL;
    }

    ret = hcc_bus_send_message(gt_pm->pst_bus, H2D_MSG_PM_WLAN_OFF);
    if (ret == OAL_SUCC) {
        ret = hcc_bus_poweroff_complete(gt_pm->pst_bus);
        if (ret != OAL_SUCC) {
            oam_error_log0(0, OAM_SF_PWR, "gt_pm_poweroff_cmd  wait device ACK timeout");
            (void)ssi_dump_err_regs(SSI_ERR_WLAN_POWEROFF_FAIL);
        } else {
            oam_warning_log0(0, OAM_SF_PWR, "gt_pm_poweroff_cmd  wait device ACK SUCC");
        }
    } else {
        oam_error_log0(0, OAM_SF_PWR, "fail to send H2D_MSG_PM_WLAN_OFF");
    }
    hcc_tx_transfer_unlock(hcc_get_handler(HCC_EP_GT_DEV));
    return ret;
}

void gt_pm_wkup_src_debug_set(uint32_t en)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return;
    }
    gt_pm->wkup_src_print_en = en;
}

EXPORT_SYMBOL_GPL(gt_pm_wkup_src_debug_set);

uint32_t gt_pm_wkup_src_debug_get(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if (gt_pm == NULL) {
        return OAL_FALSE;
    }
    return gt_pm->wkup_src_print_en;
}
EXPORT_SYMBOL_GPL(gt_pm_wkup_src_debug_get);

void gt_pm_dump_host_info(void)
{
    struct gt_pm_s *gt_pm_info = gt_pm_get_drv();
    struct oal_sdio *pst_sdio = oal_get_sdio_default_handler();

    if (gt_pm_info == NULL) {
        return;
    }

    oal_io_print("----------gt_pm_dump_host_info begin-----------\n");
    oal_io_print("power on:%ld, enable:%ld\n",
                 gt_pm_info->gt_power_state, gt_pm_info->gt_pm_enable);
    oal_io_print("dev state:%ld, sleep stage:%ld\n", gt_pm_info->gt_dev_state, gt_pm_info->sleep_stage);
    oal_io_print("open:%d,close:%d\n", gt_pm_info->open_cnt, gt_pm_info->close_cnt);
    if (pst_sdio != NULL) {
        oal_io_print("sdio suspend:%d,sdio resume:%d\n", pst_sdio->sdio_suspend, pst_sdio->sdio_resume);
    }
    oal_io_print("gpio_intr[no.%d]:%llu\n",
                 gt_pm_info->pst_bus->bus_dev->wlan_irq, gt_pm_info->pst_bus->gpio_int_count);
    oal_io_print("data_intr:%llu\n", gt_pm_info->pst_bus->data_int_count);
    oal_io_print("wakeup_intr:%llu\n", gt_pm_info->pst_bus->wakeup_int_count);
    oal_io_print("D2H_MSG_WAKEUP_SUCC:%d\n", gt_pm_info->pst_bus->msg[D2H_MSG_WAKEUP_SUCC].count);
    oal_io_print("D2H_MSG_ALLOW_SLEEP:%d\n", gt_pm_info->pst_bus->msg[D2H_MSG_ALLOW_SLEEP].count);
    oal_io_print("D2H_MSG_DISALLOW_SLEEP:%d\n", gt_pm_info->pst_bus->msg[D2H_MSG_DISALLOW_SLEEP].count);

    oal_io_print("wakeup_dev_wait_ack:%d\n", oal_atomic_read(&g_wakeup_dev_wait_ack));
    oal_io_print("wakeup_succ:%d\n", gt_pm_info->wakeup_succ);
    oal_io_print("wakeup_dev_ack:%d\n", gt_pm_info->wakeup_dev_ack);
    oal_io_print("wakeup_done_callback:%d\n", gt_pm_info->wakeup_done_callback);
    oal_io_print("wakeup_succ_work_submit:%d\n", gt_pm_info->wakeup_succ_work_submit);
    oal_io_print("wakeup_fail_wait_sdio:%d\n", gt_pm_info->wakeup_fail_wait_sdio);
    oal_io_print("wakeup_fail_timeout:%d\n", gt_pm_info->wakeup_fail_timeout);
    oal_io_print("wakeup_fail_set_reg:%d\n", gt_pm_info->wakeup_fail_set_reg);
    oal_io_print("wakeup_fail_submit_work:%d\n", gt_pm_info->wakeup_fail_submit_work);
    oal_io_print("sleep_succ:%d\n", gt_pm_info->sleep_succ);
    oal_io_print("sleep feed wdg:%d\n", gt_pm_info->sleep_feed_wdg_cnt);
    oal_io_print("sleep_fail_request:%d\n", gt_pm_info->sleep_fail_request);
    oal_io_print("sleep_fail_set_reg:%d\n", gt_pm_info->sleep_fail_set_reg);
    oal_io_print("sleep_fail_wait_timeout:%d\n", gt_pm_info->sleep_fail_wait_timeout);
    oal_io_print("sleep_fail_forbid:%d\n", gt_pm_info->sleep_fail_forbid);
    oal_io_print("sleep_work_submit:%d\n", gt_pm_info->sleep_work_submit);
    oal_io_print("wklock_cnt:%lu\n \n", gt_pm_info->pst_bus->st_bus_wakelock.lock_count);
    oal_io_print("tid_not_empty_cnt:%d\n", gt_pm_info->tid_not_empty_cnt);
    oal_io_print("is_in_ddr_rx_cnt:%d\n", gt_pm_info->is_ddr_rx_cnt);
    oal_io_print("ring_has_mpdu_cnt:%d\n", gt_pm_info->ring_has_mpdu_cnt);
    oal_io_print("----------gt_pm_dump_host_info end-----------\n");
}

void gt_pm_dump_device_info(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    hcc_bus_send_message(gt_pm->pst_bus, H2D_MSG_PM_DEBUG);
}

void gt_pm_info_clean(void)
{
    struct gt_pm_s *gt_pm_info = gt_pm_get_drv();

    gt_pm_info->pst_bus->data_int_count = 0;
    gt_pm_info->pst_bus->wakeup_int_count = 0;

    gt_pm_info->pst_bus->msg[D2H_MSG_WAKEUP_SUCC].count = 0;
    gt_pm_info->pst_bus->msg[D2H_MSG_ALLOW_SLEEP].count = 0;
    gt_pm_info->pst_bus->msg[D2H_MSG_DISALLOW_SLEEP].count = 0;

    gt_pm_info->wakeup_succ = 0;
    gt_pm_info->wakeup_dev_ack = 0;
    gt_pm_info->wakeup_done_callback = 0;
    gt_pm_info->wakeup_succ_work_submit = 0;
    gt_pm_info->wakeup_fail_wait_sdio = 0;
    gt_pm_info->wakeup_fail_timeout = 0;
    gt_pm_info->wakeup_fail_set_reg = 0;
    gt_pm_info->wakeup_fail_submit_work = 0;

    gt_pm_info->sleep_succ = 0;
    gt_pm_info->sleep_feed_wdg_cnt = 0;
    gt_pm_info->wakeup_done_callback = 0;
    gt_pm_info->sleep_fail_set_reg = 0;
    gt_pm_info->sleep_fail_wait_timeout = 0;
    gt_pm_info->sleep_fail_forbid = 0;
    gt_pm_info->sleep_work_submit = 0;
    gt_pm_info->tid_not_empty_cnt = 0;
    gt_pm_info->is_ddr_rx_cnt = 0;
    gt_pm_info->ring_has_mpdu_cnt = 0;

    return;
}

void gt_pm_debug_sleep(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if ((gt_pm != NULL) && (gt_pm->pst_bus != NULL)) {
        hcc_bus_sleep_request(gt_pm->pst_bus);

        gt_pm->gt_dev_state = HOST_ALLOW_TO_SLEEP;
    }
    return;
}

void gt_pm_debug_wakeup(void)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();

    if ((gt_pm != NULL) && (gt_pm->pst_bus != NULL)) {
        hcc_bus_wakeup_request(gt_pm->pst_bus);

        gt_pm->gt_dev_state = HOST_DISALLOW_TO_SLEEP;
    }
    return;
}

int32_t gt_power_fail_process(int32_t error)
{
    int32_t ret = GT_POWER_FAIL;
    struct gt_pm_s *gt_pm_info = gt_pm_get_drv();
    if (gt_pm_info == NULL) {
        ps_print_err("gt_pm_info is NULL!\n");
        return -FAILURE;
    }

    ps_print_err("gt power fail, error=[%d]\n", error);

    switch (error) {
        case GT_POWER_SUCCESS:
        case GT_POWER_PULL_POWER_GPIO_FAIL:
            break;

        case GT_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT:
            ret = -OAL_EINTR;
            board_power_off(G_SYS);
            break;
        case GT_POWER_ON_FIRMWARE_FILE_OPEN_FAIL:
        case GT_POWER_ON_CALI_FAIL:
            board_power_off(G_SYS);
            break;

        default:
            ps_print_err("error is undefined, error=[%d]\n", error);
            break;
    }

    return ret;
}

void gt_device_mem_check_work(oal_work_stru *pst_worker)
{
    struct gt_pm_s *gt_pm = gt_pm_get_drv();
    struct pm_top* pm_top_data = pm_get_top();

    mutex_lock(&(pm_top_data->host_mutex));
    hcc_bus_disable_state(gt_pm->pst_bus, OAL_BUS_STATE_ALL);
    g_ram_reg_test_result = device_mem_check(&g_ram_reg_test_time);
    hcc_bus_enable_state(gt_pm->pst_bus, OAL_BUS_STATE_ALL);
    hcc_bus_wake_unlock(gt_pm->pst_bus);
    mutex_unlock(&(pm_top_data->host_mutex));
}
