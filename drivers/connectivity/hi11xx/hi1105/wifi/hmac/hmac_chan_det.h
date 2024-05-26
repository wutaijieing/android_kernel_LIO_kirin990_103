/*
* 版权所有 (c) 华为技术有限公司 2020-2020
* 功能说明   : 信道测量上报功能.h文件
* 作者       : shaojiayao 569451
* 创建日期   : 2021年11月9日
*/
#ifndef __HMAC_CHAN_DET_H__
#define __HMAC_CHAN_DET_H__
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hal_common.h"

#define HMAC_CHAN_DET_FILE_MAX_NUM 200

uint32_t hmac_chan_det_rpt_event_process(frw_event_mem_stru *event_mem);

#endif /* end of __HMAC_CHAN_DET_H__ */