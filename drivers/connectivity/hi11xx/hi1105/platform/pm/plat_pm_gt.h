

#ifndef __PLAT_PM_GT_H__
#define __PLAT_PM_GT_H__

/* 其他头文件包含 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/mutex.h>
#include <linux/kernel.h>
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION))
#include <linux/pm_wakeup.h>
#endif
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>

#include <linux/fb.h>
#endif
#include "oal_hcc_bus.h"

#include "oal_ext_if.h"

#ifdef WIN32
#include "plat_type.h"
#endif

#define GT_WAKUP_MSG_WAIT_TIMEOUT    3000
#define GT_SLEEP_MSG_WAIT_TIMEOUT    10000
#define GT_POWEROFF_ACK_WAIT_TIMEOUT 1000
#define GT_OPEN_BCPU_WAIT_TIMEOUT    1000
#define GT_HALT_BCPU_TIMEOUT         1000
#define GT_SLEEP_TIMER_PERIOD        20  /* 睡眠定时器20ms定时 */
#define GT_SLEEP_DEFAULT_CHECK_CNT   5   /* 默认100ms */
#define GT_SLEEP_LONG_CHECK_CNT      20  /* 入网阶段,延长至400ms */
#define GT_SLEEP_FAST_CHECK_CNT      1   /* fast sleep,20ms */
#define GT_WAKELOCK_HOLD_TIME        500 /* hold wakelock 500ms */
#define GT_WAKELOCK_NO_TIMEOUT       (-1) /* hold wakelock no timeout */
#define GT_BUS_SEMA_TIME             (6 * HZ)   /* 6s 等待信号量 */

// 100us for WL_DEV_WAKEUP hold lowlevel
#define GT_WAKEUP_DEV_EVENT_DELAY_US (0) // 150 us, reserve to 0us

#define GT_SDIO_MSG_RETRY_NUM      3
#define GT_WAKEUP_FAIL_MAX_TIMES   0               /* 连续多少次wakeup失败，可进入DFR流程 */
#define GT_PACKET_CHECK_TIME       5000            /* 唤醒后，每5s打印一次报文个数用于持锁是否异常的debug */
#define GT_SLEEP_FORBID_CHECK_TIME (2 * 60 * 1000) /* 连续2分钟sleep forbid */

#define GT_PM_MODULE "[gt]"

enum GT_PM_SLEEP_STAGE {
    GT_SLEEP_STAGE_INIT = 0,    // 初始
    GT_SLEEP_REQ_SND = 1,       // sleep request发送完成
    GT_SLEEP_ALLOW_RCV = 2,     // 收到allow sleep response
    GT_SLEEP_DISALLOW_RCV = 3,  // 收到allow sleep response
    GT_SLEEP_CMD_SND = 4,       // 允许睡眠reg设置完成
};

#define ALLOW_IDLESLEEP    1
#define DISALLOW_IDLESLEEP 0

#define GT_PM_POWERUP_EVENT   3
#define GT_PM_POWERDOWN_EVENT 2
#define GT_PM_SLEEP_EVENT     1
#define GT_PM_WAKEUP_EVENT    0

/* STRUCT 定义 */
typedef uint8_t (*gt_srv_get_pm_pause_func)(void);
typedef void (*gt_srv_open_notify)(uint8_t);
typedef void (*gt_srv_pm_state_notify)(uint8_t);
typedef void (*gt_srv_data_wkup_print_en_func)(uint8_t);

struct gt_srv_callback_handler {
    gt_srv_get_pm_pause_func p_gt_srv_get_pm_pause_func;
    gt_srv_open_notify p_gt_srv_open_notify;
    gt_srv_pm_state_notify p_gt_srv_pm_state_notify;
};

struct gt_pm_s {
    hcc_bus *pst_bus;

    oal_uint gt_pm_enable;
    oal_uint gt_power_state;
    oal_uint apmode_allow_pm_flag; // ap模式下，是否允许下电操作,1:允许,0:不允许

    volatile oal_uint gt_dev_state;
    uint8_t wakeup_err_count; // 连续唤醒失败次数
    uint8_t fail_sleep_count; // 连续睡眠失败次数

    oal_workqueue_stru *pst_pm_wq; // pm work quque
    oal_work_stru st_wakeup_work; // 唤醒work
    oal_work_stru st_sleep_work; // sleep work
    oal_work_stru st_ram_reg_test_work; // ram_reg_test work

    oal_timer_list_stru st_watchdog_timer;
    oal_wakelock_stru st_pm_wakelock;
    oal_wakelock_stru st_delaysleep_wakelock;

    uint32_t packet_cnt; // 睡眠周期内统计的packet个数
    uint32_t packet_total_cnt; // 从上次唤醒至今的packet个数，定期输出for debug
    unsigned long packet_check_time;
    unsigned long sleep_forbid_check_time;
    uint32_t wdg_timeout_cnt; // timeout check cnt
    uint32_t wdg_timeout_curr_cnt; // timeout check current cnt
    volatile oal_uint sleep_stage; // 睡眠过程阶段标识

    oal_completion st_open_bcpu_done;
    oal_completion st_close_bcpu_done;
    oal_completion st_close_done;
    oal_completion st_wakeup_done;
    oal_completion st_sleep_request_ack;
    oal_completion st_halt_bcpu_done;
    oal_completion gt_powerup_done;

    uint32_t wkup_src_print_en;

    struct gt_srv_callback_handler gt_srv_handler;

    /* 维测统计 */
    uint32_t open_cnt;
    uint32_t close_cnt;
    uint32_t close_done_callback;
    uint32_t wakeup_succ;
    uint32_t wakeup_succ_work_submit;
    uint32_t wakeup_dev_ack;
    uint32_t wakeup_done_callback;
    uint32_t wakeup_fail_wait_sdio;
    uint32_t wakeup_fail_timeout;
    uint32_t wakeup_fail_set_reg;
    uint32_t wakeup_fail_submit_work;

    uint32_t sleep_succ;
    uint32_t sleep_feed_wdg_cnt;
    uint32_t sleep_fail_request;
    uint32_t sleep_fail_wait_timeout;
    uint32_t sleep_fail_set_reg;
    uint32_t sleep_request_host_forbid;
    uint32_t sleep_fail_forbid;
    uint32_t sleep_fail_forbid_cnt; /* forbid 计数，当睡眠成功后清除，维测信息 */
    uint32_t sleep_work_submit;

    uint32_t tid_not_empty_cnt;
    uint32_t is_ddr_rx_cnt;
    uint32_t ring_has_mpdu_cnt;
};

typedef struct gt_memdump_s {
    int32_t addr;
    int32_t len;
    int32_t en;
} gt_memdump_t;

/* EXTERN VARIABLE */
extern uint8_t g_gt_min_fast_ps_idle;
extern uint8_t g_gt_max_fast_ps_idle;
extern uint8_t g_gt_auto_ps_screen_on;
extern uint8_t g_gt_auto_ps_screen_off;
extern uint8_t g_gt_ps_mode;
extern uint16_t g_download_rate_limit_pps;
extern uint8_t g_gt_fast_ps_dyn_ctl;
extern uint8_t g_gt_custom_cali_done;
extern uint8_t g_gt_custom_cali_cnt;
/* EXTERN FUNCTION */
struct gt_pm_s *gt_pm_get_drv(void);
void gt_pm_debug_sleep(void);
void gt_pm_debug_wakeup(void);
void gt_pm_dump_host_info(void);
void gt_pm_dump_device_info(void);
struct gt_pm_s *gt_pm_init(void);
oal_uint gt_pm_exit(void);
uint32_t gt_pm_is_poweron(void);
int32_t gt_pm_open(void);
uint32_t gt_pm_close(void);
uint32_t gt_pm_close_by_shutdown(void);
oal_uint gt_pm_init_dev(void);
oal_uint gt_pm_wakeup_dev(void);
void gt_pm_wakeup_dev_lock(void);
oal_uint gt_pm_wakeup_host(void);
oal_uint gt_pm_open_bcpu(void);
oal_uint gt_pm_state_get(void);
uint32_t gt_pm_enable(void);
uint32_t gt_pm_disable(void);
uint32_t gt_pm_statesave(void);
uint32_t gt_pm_staterestore(void);
uint32_t gt_pm_disable_check_wakeup(int32_t flag);
struct gt_srv_callback_handler *gt_pm_get_srv_handler(void);
void gt_pm_wakeup_dev_ack(void);
void gt_pm_set_timeout(uint32_t timeout);
int32_t gt_pm_poweroff_cmd(void);
int32_t gt_pm_shutdown_bcpu_cmd(void);
void gt_pm_feed_wdg(void);
int32_t gt_pm_stop_wdg(struct gt_pm_s *gt_pm_info);
void gt_pm_info_clean(void);
void gt_pm_wkup_src_debug_set(uint32_t en);
uint32_t gt_pm_wkup_src_debug_get(void);
gt_memdump_t *get_gt_memdump_cfg(void);
gt_memdump_t *get_gt_memdump_cfg(void);

int32_t gt_device_mem_check(int32_t l_runing_test_mode);
int32_t gt_device_mem_check_result(unsigned long long *time);
void gt_device_mem_check_work(oal_work_stru *pst_worker);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
int32_t gt_pm_register_notifier(struct notifier_block *nb);
void gt_pm_unregister_notifier(struct notifier_block *nb);
#endif

#ifdef CONFIG_HUAWEI_DSM
void hw_110x_register_dsm_client(void);
void hw_110x_unregister_dsm_client(void);
#endif
int32_t gt_power_fail_process(int32_t error);

#endif
