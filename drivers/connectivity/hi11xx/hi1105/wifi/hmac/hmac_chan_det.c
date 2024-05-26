/*
* 版权所有 (c) 华为技术有限公司 2020-2020
* 功能说明   : 信道测量上报功能
* 作者       : shaojiayao 569451
* 创建日期   : 2021年11月9日
*/

/* 1 头文件包含 */
#include "hmac_chan_det.h"
#include "oal_kernel_file.h"
#include "hd_event.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHAN_DET_C
#define FILE_LEN 128

/* 2 全局变量定义 */
uint8_t g_hmac_chan_det_cnt = 0;

/* 3 函数定义 */
static uint32_t hmac_chan_det_get_filename(uint8_t *filename, uint8_t file_len)
{
    int32_t str_len;

    if (g_hmac_chan_det_cnt > HMAC_CHAN_DET_FILE_MAX_NUM) {
        oam_warning_log0(0, OAM_SF_FTM, "hmac_chan_det_get_filename:: file over 200");
        g_hmac_chan_det_cnt = 0;
        return OAL_FAIL; // 脚本完成清理文件之后，则不需要此行return,执行g_hmac_chan_det_cnt = 0;
    }
    g_hmac_chan_det_cnt++;
    /* 获取文件路径\data\log\psd\ */
    str_len = snprintf_s(filename, file_len, file_len - 1,
                         "/data/log/psd/chan_det_%u.txt", g_hmac_chan_det_cnt);
    if (str_len < 0) {
        oam_error_log1(0, OAM_SF_FTM, "hmac_proc_location_action::snprintf_s error! str_len %d", str_len);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

void hmac_chan_det_print_to_file(chan_det_rpt_stru *chan_det_result, oal_file *file)
{
    uint8_t idx;
    d2h_chan_det_rpt_sttu *chan_rpt = &chan_det_result->chan_rpt;

    oal_kernel_file_print(file, "det_index:%d\n", g_hmac_chan_det_cnt);
    oal_kernel_file_print(file, "rx_direct_time:%d\n", chan_rpt->rx_direct_time);
    oal_kernel_file_print(file, "rx_nondir_time:%d\n", chan_rpt->rx_nondir_time);
    oal_kernel_file_print(file, "tx_time:%d\n", chan_rpt->tx_time);
    oal_kernel_file_print(file, "free_time:%d\n", chan_rpt->free_time);
    oal_kernel_file_print(file, "abort_time_us:%d\n", chan_rpt->abort_time_us);

    oal_kernel_file_print(file, "duty_cyc_ratio_20:%d\n", chan_rpt->duty_cyc_ratio_20);
    oal_kernel_file_print(file, "duty_cyc_ratio_40:%d\n", chan_rpt->duty_cyc_ratio_40);
    oal_kernel_file_print(file, "duty_cyc_ratio_80:%d\n", chan_rpt->duty_cyc_ratio_80);
    oal_kernel_file_print(file, "duty_cyc_ratio_160:%d\n", chan_rpt->duty_cyc_ratio_160);

    oal_kernel_file_print(file, "intf_det_avg_rssi_20:%d\n", chan_rpt->intf_det_avg_rssi_20);
    oal_kernel_file_print(file, "intf_det_avg_rssi_40:%d\n", chan_rpt->intf_det_avg_rssi_40);
    oal_kernel_file_print(file, "intf_det_avg_rssi_80:%d\n", chan_rpt->intf_det_avg_rssi_80);
    oal_kernel_file_print(file, "intf_det_avg_rssi_160:%d\n", chan_rpt->intf_det_avg_rssi_160);

    oal_kernel_file_print(file, "per20m_idle_pwr:");
    for (idx = 0; idx < 8; idx++) {
        oal_kernel_file_print(file, "%d ", chan_rpt->per20m_idle_pwr[idx]);
    }
    oal_kernel_file_print(file, "\n");

    oal_kernel_file_print(file, "per20m_idle_time:");
    for (idx = 0; idx < 8; idx++) {
        oal_kernel_file_print(file, "%d ", chan_rpt->per20m_idle_time[idx]);
    }
    oal_kernel_file_print(file, "\n");

    oal_kernel_file_print(file, "chan_ratio:");
    for (idx = 0; idx < 7; idx++) {
        oal_kernel_file_print(file, "%d ", chan_rpt->chan_ratio[idx]);
    }
    oal_kernel_file_print(file, "\n");

    oal_kernel_file_print(file, "free_power:");
    for (idx = 0; idx < 7; idx++) {
        oal_kernel_file_print(file, "%d ", chan_rpt->free_power[idx]);
    }
    oal_kernel_file_print(file, "\n");

    oal_kernel_file_print(file, "mac_free_time:");
    for (idx = 0; idx < 7; idx++) {
        oal_kernel_file_print(file, "%d ", chan_det_result->mac_free_time[idx]);
    }
    oal_kernel_file_print(file, "\n");

    oal_kernel_file_print(file, "phy_free_time:");
    for (idx = 0; idx < 7; idx++) {
        oal_kernel_file_print(file, "%d ", chan_det_result->phy_free_time[idx]);
    }
}

uint32_t hmac_chan_det_rpt_event_process(frw_event_mem_stru *event_mem)
{
    frw_event_stru *event = NULL;
    chan_det_rpt_stru *chan_det_result = NULL;
    uint8_t filename[FILE_LEN];
    uint32_t ret;
    oal_file *file = NULL;
    oal_mm_segment_t old_fs;

    /* 获取事件信息 */
    event = frw_get_event_stru(event_mem);
    chan_det_result = (chan_det_rpt_stru *)(event->auc_event_data);
    ret = hmac_chan_det_get_filename(filename, FILE_LEN);
    if (ret != OAL_SUCC) {
        return ret;
    }
    file = oal_kernel_file_open(filename, OAL_O_RDWR | OAL_O_CREAT | OAL_O_APPEND);
    if (IS_ERR_OR_NULL(file)) {
        return ret;
    }
    old_fs = oal_get_fs();

    hmac_chan_det_print_to_file(chan_det_result, file);

    oal_kernel_file_close(file);
    oal_set_fs(old_fs);

    return OAL_SUCC;
}