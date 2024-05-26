
#ifdef _PRE_CONFIG_HISI_CONN_SOFTWDFT
#define HISI_LOG_TAG "[SOFT_WDT]"
#include "oal_softwdt.h"
#include "oal_net.h"
#include "oal_workqueue.h"

/* 4s timeout,8s bug_on */
#define OAL_SOFTWDT_DEFAULT_TIMEOUT 4000 /* 4s */

typedef struct _hisi_conn_softwdt_ {
    uint32_t kick_time;
    oal_delayed_work wdt_delayed_work;
    oal_workqueue_stru *wdt_wq;
    oal_timer_list_stru wdt_timer;
    uint32_t wdt_timeout_count;
} hisi_conn_softwdt;

OAL_STATIC hisi_conn_softwdt g_hisi_softwdt;

#ifdef PLATFORM_SSI_FULL_LOG
OAL_STATIC int32_t g_disable_wdt_flag = 0;
#else
OAL_STATIC int32_t g_disable_wdt_flag = 1;
#endif
oal_debug_module_param(g_disable_wdt_flag, int, S_IRUGO | S_IWUSR);

OAL_STATIC void oal_softwdt_kick(void)
{
    oal_timer_start(&g_hisi_softwdt.wdt_timer, OAL_SOFTWDT_DEFAULT_TIMEOUT);
}

OAL_STATIC void oal_softwdt_timeout_log(void)
{
    printk(KERN_ERR "Thread: xmit = %lu hcc = %lu rxdata = %lu rxtask %lu frw %lu\n\r",
        g_hisi_softwdt_check.xmit_cnt, g_hisi_softwdt_check.hcc_cnt,
        g_hisi_softwdt_check.rxdata_cnt, g_hisi_softwdt_check.rxtask_cnt,
        g_hisi_softwdt_check.frw_cnt);
    printk(KERN_ERR "Rxthread: enq = %lu napi_sched = %lu netif %lu napi_poll = %lu\n\r",
        g_hisi_softwdt_check.napi_enq_cnt, g_hisi_softwdt_check.napi_sched_cnt,
        g_hisi_softwdt_check.netif_rx_cnt, g_hisi_softwdt_check.napi_poll_cnt);
    printk(KERN_ERR "sched rxdata: napi = %lu ap = %lu sta = %lu lan = %lu msdu = %lu rpt = %lu \n\r",
        g_hisi_softwdt_check.rxshed_napi, g_hisi_softwdt_check.rxshed_ap,
        g_hisi_softwdt_check.rxshed_sta, g_hisi_softwdt_check.rxshed_lan,
        g_hisi_softwdt_check.rxshed_msdu, g_hisi_softwdt_check.rxshed_rpt);
}
OAL_STATIC void oal_softwdt_timeout(oal_timeout_func_para_t data)
{
    oal_reference(data);
    g_hisi_softwdt.wdt_timeout_count++;

    if (g_hisi_softwdt.wdt_timeout_count == 1) { /* 第一次超时 */
#ifdef CONFIG_PRINTK
        printk(KERN_WARNING "hisi softwdt timeout first time,keep try...\n");
#else
        oal_io_print("hisi softwdt timeout first time,keep try...\n");
#endif
        oal_softwdt_kick();
    }

    if (g_hisi_softwdt.wdt_timeout_count >= 2) { /* 第2次及以上次超时 */
#ifdef CONFIG_PRINTK
        printk(KERN_EMERG "[E]hisi softwdt timeout second time, dump system stack\n");
        oal_softwdt_timeout_log();
#else
        oal_io_print("hisi softwdt timeout second time, dump system stack!\n");
#endif
        declare_dft_trace_key_info("oal_softwdt_timeout", OAL_DFT_TRACE_EXCEP);
        oal_warn_on(1);
        g_hisi_softwdt.wdt_timeout_count = 0;
    }
}


OAL_STATIC void oal_softwdt_feed_task(oal_work_stru *pst_work)
{
    oal_softwdt_kick();
    oal_queue_delayed_work_on(0, g_hisi_softwdt.wdt_wq, &g_hisi_softwdt.wdt_delayed_work,
        oal_msecs_to_jiffies(g_hisi_softwdt.kick_time));
}

int32_t oal_softwdt_init(void)
{
    if (g_disable_wdt_flag) {
        return OAL_SUCC;
    }

    memset_s((void *)&g_hisi_softwdt, sizeof(g_hisi_softwdt), 0, sizeof(g_hisi_softwdt));
    oal_timer_init(&g_hisi_softwdt.wdt_timer, OAL_SOFTWDT_DEFAULT_TIMEOUT, oal_softwdt_timeout, 0);
    g_hisi_softwdt.kick_time = OAL_SOFTWDT_DEFAULT_TIMEOUT / 2;  // 2s
    oal_init_delayed_work(&g_hisi_softwdt.wdt_delayed_work, oal_softwdt_feed_task);
    g_hisi_softwdt.wdt_wq = oal_create_workqueue("softwdt");
    if (oal_unlikely(g_hisi_softwdt.wdt_wq == NULL)) {
        oal_io_print("hisi soft wdt create workqueue failed!\n");
        return -OAL_ENOMEM;
    }

    /* start wdt */
    oal_queue_delayed_work_on(0, g_hisi_softwdt.wdt_wq, &g_hisi_softwdt.wdt_delayed_work, 0);
    return OAL_SUCC;
}
oal_module_symbol(oal_softwdt_init);

void oal_softwdt_exit(void)
{
    if (g_disable_wdt_flag) {
        return;
    }

    oal_timer_delete_sync(&g_hisi_softwdt.wdt_timer);
    oal_cancel_delayed_work_sync(&g_hisi_softwdt.wdt_delayed_work);
    oal_destroy_workqueue(g_hisi_softwdt.wdt_wq);
}
oal_module_symbol(oal_softwdt_exit);
#endif
