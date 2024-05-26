/*
 * 版权所有 (c) 华为技术有限公司 2022-2022
 * 功能说明 : CHBA 信道序列文件
 * 作    者 : duankaiyong
 * 创建日期 : 2022年3月24日
 */

#ifdef _PRE_WLAN_CHBA_MGMT
#include "oal_ext_if.h"
#include "oam_ext_if.h"

#include "hmac_chba_channel_sequence.h"
#include "hmac_chba_function.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_sync.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_CHANNEL_SEQUENCE_C

#define CHBA_CHANNEL_SEQ_MIN_CHAN_NUMBER 36 /* chba信道序列支持的最小主20MHz信道号 */
#define CHBA_CHANNEL_SEQ_MAX_CHAN_NUMBER 165 /* chba信道序列支持的最大主20MHz信道号 */

static uint32_t chba_channel_seq_level_to_bitmap(uint32_t level)
{
    uint32_t chba_channel_seq_level_to_bitmap_table[CHBA_CHANNEL_SEQ_LEVEL_BUTT] = {
        CHBA_CHANNEL_SEQ_BITMAP_25_PERCENT,
        CHBA_CHANNEL_SEQ_BITMAP_50_PERCENT,
        CHBA_CHANNEL_SEQ_BITMAP_75_PERCENT
    };

    if (level >= CHBA_CHANNEL_SEQ_LEVEL_BUTT) {
        return CHBA_CHANNEL_SEQ_BITMAP_100_PERCENT;
    }
    return chba_channel_seq_level_to_bitmap_table[level];
}

/* 配置 信道序列 bitmap */
void hmac_chba_set_channel_seq_bitmap(hmac_chba_vap_stru *chba_vap_info, uint32_t channel_seq_bitmap)
{
    chba_vap_info->channel_sequence_bitmap = channel_seq_bitmap;
}

/* 配置CHBA第一工作信道 */
static void hmac_chba_set_first_work_channel(hmac_chba_vap_stru *chba_vap_info, mac_channel_stru *first_work_channel)
{
    chba_vap_info->work_channel = *first_work_channel;
}

/* 配置CHBA第二工作信道 */
static void hmac_chba_set_sec_work_channel(hmac_chba_vap_stru *chba_vap_info, mac_channel_stru *sec_work_channel)
{
    chba_vap_info->second_work_channel = *sec_work_channel;
}


/* 判断是否是单CHBA VAP工作 */
static oal_bool_enum_uint8 mac_device_is_single_chba_up(mac_device_stru *mac_device)
{
    uint8_t vap_idx;

    for (vap_idx = 0; vap_idx < mac_device->uc_vap_num; vap_idx++) {
        mac_vap_stru *mac_vap = NULL;
        mac_vap = mac_res_get_mac_vap(mac_device->auc_vap_id[vap_idx]);
        if (oal_unlikely(mac_vap == NULL)) {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_device_is_single_chba_up:vap is null! vap id is %d",
                mac_device->auc_vap_id[vap_idx]);
            continue;
        }
        if (!mac_is_chba_mode(mac_vap) && hmac_vap_is_up(mac_vap)) {
            return OAL_FALSE;
        }
    }
    return OAL_TRUE;
}

/*
 * 函数名:hmac_chba_is_channel_seq_params_legal
 * 功能:检查信道序列配置参数是否合法
 */
static oal_bool_enum hmac_chba_is_channel_seq_params_legal(chba_channel_sequence_params_stru *chan_seq_params)
{
    if (chan_seq_params->channel_seq_level > CHBA_CHANNEL_SEQ_LEVEL_BUTT) {
        return OAL_FALSE;
    }

    if (chan_seq_params->first_work_channel < CHBA_CHANNEL_SEQ_MIN_CHAN_NUMBER ||
        chan_seq_params->first_work_channel > CHBA_CHANNEL_SEQ_MAX_CHAN_NUMBER) {
        return OAL_FALSE;
    }

    if (chan_seq_params->sec_work_channel < CHBA_CHANNEL_SEQ_MIN_CHAN_NUMBER ||
        chan_seq_params->sec_work_channel > CHBA_CHANNEL_SEQ_MAX_CHAN_NUMBER) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

static uint32_t hmac_chba_h2d_channel_seq_params(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint32_t ret;

    /***************************************************************************
       抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_CHANNEL_SEQ_PARAMS, len, params);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CHBA, "{hmac_chba_set_channel_seq_params:fail send event to dmac}");
    }
    return ret;
}

/*
 * 函数名:hmac_chba_set_channel_seq_params
 * 功能:设置CHBA 信道序列参数
 */
uint32_t hmac_chba_set_channel_seq_params(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint32_t channel_seq_bitmap;
    oal_bool_enum_uint8 is_single_chba_up;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    mac_device_stru *mac_device = NULL;
    chba_channel_sequence_params_stru *channel_seq_params = NULL;

    if (oal_any_null_ptr2(mac_vap, params)) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_set_channel_seq_params:ptr is NULl!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    chba_vap_info = hmac_chba_get_chba_vap_info(hmac_vap);
    if (chba_vap_info == NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CHBA, "{hmac_chba_set_channel_seq_params:chba_vap_info is NULl!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    channel_seq_params = (chba_channel_sequence_params_stru *)params;

    /* 检查信道序列配置参数是否合法 */
    if (hmac_chba_is_channel_seq_params_legal(channel_seq_params) == OAL_FALSE) {
        oam_warning_log3(mac_vap->uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_set_channel_seq_params:illegal params:seq_level[%d], first_chan[%d], second_chan[%d]}",
            channel_seq_params->channel_seq_level, channel_seq_params->first_work_channel,
            channel_seq_params->sec_work_channel);
        return OAL_FAIL;
    }

    /* 根据信道序列等级, 配置channle_sequenc_bitmap */
    channel_seq_bitmap = chba_channel_seq_level_to_bitmap(channel_seq_params->channel_seq_level);
    hmac_chba_set_channel_seq_bitmap(chba_vap_info, channel_seq_bitmap);

    /* 单CHBA vap工作场景，才能配置STA 工作信道 */
    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    is_single_chba_up = mac_device_is_single_chba_up(mac_device);
    if (is_single_chba_up == OAL_TRUE) {
        /* 单CHBA场景，根据命令信道参数构造工作信道结构体 */
        mac_channel_stru tmp_channel = chba_vap_info->work_channel;

        /* 配置social channel 和工作信道 */
        tmp_channel.uc_chan_number = channel_seq_params->first_work_channel;
        if (mac_get_channel_idx_from_num(tmp_channel.en_band, tmp_channel.uc_chan_number,
            &tmp_channel.uc_idx) == OAL_SUCC) {
            hmac_config_social_channel(chba_vap_info, tmp_channel.uc_chan_number, tmp_channel.en_bandwidth);
            hmac_chba_set_first_work_channel(chba_vap_info, &tmp_channel);
        }

        /* 配置CHBA第二工作信道 */
        tmp_channel.uc_chan_number = channel_seq_params->sec_work_channel;
        if (mac_get_channel_idx_from_num(tmp_channel.en_band, tmp_channel.uc_chan_number,
            &tmp_channel.uc_idx) == OAL_SUCC) {
            hmac_chba_set_sec_work_channel(chba_vap_info, &tmp_channel);
        }
    }
    oam_warning_log4(mac_vap->uc_vap_id, OAM_SF_CHBA,
        "hmac_chba_set_channel_seq_params: level %d, bitmap 0x%08x, first_work_channel %d, sec_work_channel %d",
        channel_seq_params->channel_seq_level, chba_vap_info->channel_sequence_bitmap,
        chba_vap_info->work_channel.uc_chan_number, chba_vap_info->second_work_channel.uc_chan_number);

    /* 更新信道序列参数后，更新beacon/pnf下发到device */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
    /* 将信道序列配置同步到dmac */
    return hmac_chba_h2d_channel_seq_params(mac_vap, len, params);
}


void hmac_chba_update_channel_seq(mac_vap_stru *mac_vap)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    chba_channel_sequence_params_stru channel_seq_params;

    if (mac_vap == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_chba_update_channel_seq:mac_vap is NULL");
        return;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    chba_vap_info = hmac_chba_get_chba_vap_info(hmac_vap);
    if (chba_vap_info == NULL) {
        return;
    }

    /* 02A 不支持STA+CHBA DBAC, 默认配置信道序列为全1 */
    channel_seq_params.channel_seq_level = CHBA_CHANNEL_SEQ_LEVEL_BUTT;
    channel_seq_params.first_work_channel = chba_vap_info->work_channel.uc_chan_number;
    channel_seq_params.sec_work_channel = chba_vap_info->work_channel.uc_chan_number;

    /* 同步到device */
    hmac_chba_set_channel_seq_params(mac_vap, sizeof(channel_seq_params), (uint8_t *)&channel_seq_params);
}

#endif /* _PRE_WLAN_CHBA_MGMT */
