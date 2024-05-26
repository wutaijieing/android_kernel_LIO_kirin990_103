/*
* 版权所有 (c) 华为技术有限公司 2020-2020
* 功能说明   : psd频谱上报功能
* 作者       : liyuting 30008159
* 创建日期   : 2021年3月24日
*/
#ifndef __HOST_PSD_BISHENG_H__
#define __HOST_PSD_BISHENG_H__
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hal_common.h"


/* 频谱扫描头信息结构体 */
typedef struct {
    uint32_t next_fft_addr_lsb;
    uint32_t next_fft_addr_msb;

    /* WORD 1: Head information 0 */
    uint32_t bit_sd_bw             : 3;   /* 信号带宽，0为无效，1为20M,2为40M,3为80M,4为160M，其他的为无效值 */
    uint32_t bit_sd_resv0          : 1;   /* 保留位 */
    uint32_t bit_sd_index          : 14;  /* PSD分析包计数，最大255*64,14bit */
    uint32_t bit_sd_resv1          : 2;   /* 保留位 */
    uint32_t bit_sd_data_length    : 2;   /* 数据长度,不包括4words的报文头:0:128,1:256,2:512,3:1024 */
    uint32_t bit_sd_resv2          : 1;   /* 保留位 */
    uint32_t bit_sd_11b_sig        : 1;   /* 11B信号指示。0-非11b；1-11b信号 */
    uint32_t bit_sd_time_rssi_dbm  : 8;   /* 时域rssi的dBm值(8bit有符号值，单位：1dBm) */

    /* WORD 2: Head information 1 */
    uint32_t bit_sd_rssi_max       : 8;   /* 所有子载波中最大rssi的dBm值精度为1dBm,该值表示的意义为rpt_psd_rssi_max*（-1dBm） */
    uint32_t bit_sd_rssi_max_index : 10;  /* 所有子载波中最大rssi的索引值(FFT正序,10bit无符号数) */
    uint32_t bit_sd_nb_carr_num    : 11;  /* 由窄带干扰的频谱宽度检测出的窄带信号所占子载波数(10bit无符号数) */
    uint32_t bit_sd_ofdm_sig       : 3;   /* WIFI信号类型指示信号：1-11ag；2-11n；3-11ac；4-11ax，其他为无效 */

    /* WORD 3: Head information 2 */
    uint32_t sd_start_time;            /* PSD分析周期内PSD数据包上报时间，精度为429.4967296s(2^32/10us) */

    /* WORD 4: Head information 3 */
    uint32_t sd_report_time_stamp;     /* PSD分析周期的起始时刻，单位0.1us */

    int8_t fft[1024];
} bisheng_psd_fft_info_stru;

void bisheng_psd_rpt_to_file(oal_netbuf_stru *netbuf, uint16_t psd_info_size);

#endif /* end of __HOST_PSD_BISHENG_H__ */
