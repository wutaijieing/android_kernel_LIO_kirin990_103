/*
* 版权所有 (c) 华为技术有限公司 2020-2020
* 功能说明   : psd频谱上报功能
* 作者       : liyuting 30008159
* 创建日期   : 2021年3月24日
*/

#include "host_psd_bisheng.h"
#include "mac_common.h"
#include "oal_kernel_file.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_PSD_C
#define PSD_DEFAULT_SHIFT_VALUE 0xFFFFFFFF /* 若需要移位4字节，将这四字节初始化为全1 */
#define FILE_LEN 128


static uint32_t bisheng_psd_get_filename(bisheng_psd_fft_info_stru *psd_info, uint8_t *filename, uint8_t file_len)
{
    int32_t str_len;
    /* 获取文件路径\data\log\psd\ */
    str_len = snprintf_s(filename, file_len, file_len - 1, "/data/log/psd/psd_%d.txt", psd_info->bit_sd_index);
    if (str_len < 0) {
        oam_error_log1(0, OAM_SF_FTM, "hmac_proc_location_action::snprintf_s error! str_len %d", str_len);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


void bisheng_psd_print_to_file(bisheng_psd_fft_info_stru *psd_info, oal_file *file, uint16_t psd_info_size)
{
    uint32_t fft_cnt;
    uint32_t fft_num;
    fft_cnt = 128 << (psd_info->bit_sd_data_length);
    if (fft_cnt > psd_info_size) {
        return;
    }
    oal_kernel_file_print(file, "addr: 0x%x\n", psd_info->next_fft_addr_lsb);
    oal_kernel_file_print(file, "index: %d\n", psd_info->bit_sd_index);
    oal_kernel_file_print(file, "bandwidth: %d\n", psd_info->bit_sd_bw);
    oal_kernel_file_print(file, "data_length: %d\n", psd_info->bit_sd_data_length);
    oal_kernel_file_print(file, "11b_sig: %d\n", psd_info->bit_sd_11b_sig);
    oal_kernel_file_print(file, "time_rssi dbm: %d\n", (int8_t)psd_info->bit_sd_time_rssi_dbm);
    oal_kernel_file_print(file, "rssi_max dbm: -%d\n", psd_info->bit_sd_rssi_max);
    oal_kernel_file_print(file, "rssi_max_index: %d\n", psd_info->bit_sd_rssi_max_index);
    oal_kernel_file_print(file, "nb_carr_num: %d\n", psd_info->bit_sd_nb_carr_num);
    oal_kernel_file_print(file, "ofdm_sig: %d\n", psd_info->bit_sd_ofdm_sig);
    oal_kernel_file_print(file, "start_time: %d\n", psd_info->sd_start_time);
    oal_kernel_file_print(file, "timr_stamp: %d\n", psd_info->sd_report_time_stamp);

    for (fft_num = 0; fft_num < fft_cnt; fft_num++) {
        if (fft_num % 8 == 0) {
            oal_kernel_file_print(file, "\n");
        }
        oal_kernel_file_print(file, "%d ", psd_info->fft[fft_num]);
    }
    oal_kernel_file_print(file, "\n");
}

/*
 * 功能描述  : 将psd信息写入内核文件
 */
void bisheng_psd_rpt_to_file(oal_netbuf_stru *netbuf, uint16_t psd_info_size)
{
    uint8_t filename[FILE_LEN];
    uint32_t ret;
    uint32_t *temp = NULL;
    bisheng_psd_fft_info_stru *psd_info = NULL;
    oal_file *file = NULL;
    oal_mm_segment_t old_fs;

    if (netbuf == NULL) {
        return;
    }
    temp = (uint32_t *)oal_netbuf_data(netbuf);
    if (temp == NULL) {
        return;
    }
    if (*temp == PSD_DEFAULT_SHIFT_VALUE) {
        temp += 1; // 移位4个字节
    }
    psd_info = (bisheng_psd_fft_info_stru *)temp;

    ret = bisheng_psd_get_filename(psd_info, filename, FILE_LEN);
    if (ret != OAL_SUCC) {
        return;
    }
    file = oal_kernel_file_open(filename, OAL_O_RDWR | OAL_O_CREAT | OAL_O_APPEND);
    if (IS_ERR_OR_NULL(file)) {
        oam_error_log0(0, OAM_SF_FTM, "{bisheng_psd_rpt_to_file: *************** save file failed}");
        return;
    }
    old_fs = oal_get_fs();

    bisheng_psd_print_to_file(psd_info, file, psd_info_size);

    oal_kernel_file_close(file);
    oal_set_fs(old_fs);
}
