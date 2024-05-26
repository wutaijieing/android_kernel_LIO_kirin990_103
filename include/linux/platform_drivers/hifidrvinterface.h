/*
 * hifidrvinterface.h
 *
 * header file of the interface between the HiFi and DRV
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

/*
 * notice:  不要包含任何其他头文件!!!
 * 请同时修改device \common\audio\hifidrvinterface.h，保持两份文件一致!!!
 * 新增消息ID跟hisi\include\audio\ipc\里面的保持一致
 */

#ifndef __HIFIDRVINTERFACE_H__
#define __HIFIDRVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*
 * 实体名称  : AP_HIFI_CCPU_RESET_REQ_STRU
 * 功能描述  : AP通知HIFI C核复位的消息结构体
 * 补充说明  ：需要与modem\include\med\hifidrvinterface.h文件中的数据结构保持绝对一致
 */
typedef struct {
	unsigned short                      uhwMsgId;     /* 0xDDE1 */
	unsigned short                      uhwReserve;
} AP_HIFI_CCPU_RESET_REQ_STRU;

/*
 * 实体名称  : HIFI_AP_CCPU_RESET_CNF_STRU
 * 功能描述  : 响应ID_AP_HIFI_CCPU_RESET_REQ，如果hifi有语音业务时，停止语音业务，回复AP
 */
typedef struct {
	unsigned short                      uhwMsgId;     /* 0xDDE2 */
	unsigned short                      uhwResult;    /* 0表示succ，1表示fail */
} HIFI_AP_CCPU_RESET_CNF_STRU;

enum AUDIO_MSG_ENUM {
	/* A核通知HIFI C 核复位消息ID */
	ID_AP_HIFI_CCPU_RESET_REQ           = 0xDDE1,
	ID_AP_HIFI_CCPU_RESET_DONE_IND      = 0xDF36,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* __HIFIDRVINTERFACE_H__ */
