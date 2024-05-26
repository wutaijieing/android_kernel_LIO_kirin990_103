/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub IPC send api.
 * Create: 2017-06-08
 */

#ifndef __INPUTHUB_API_H__
#define __INPUTHUB_API_H__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "contexthub_recovery.h"


#define CONTEXTHUB_HEADER_SIZE (sizeof(struct pkt_header) + sizeof(unsigned int))

/*
 * 注册 contexthub 消息通知
 */
extern int register_mcu_event_notifier(int tag, int cmd, int (*notify)(const struct pkt_header *head));
/*
 * 注销 contexthub 消息通知
 */
extern int unregister_mcu_event_notifier(int tag, int cmd, int (*notify) (const struct pkt_header *head));
/*
 * 获取contexthub模式
 */
extern int get_sensor_mcu_mode(void);
/*
 * 发送IPC消息给contexthub
 */
int send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, const char *buf,
			 size_t count);
/*
 * 发送IPC消息给contexthub并等待回复
 */
int send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, const char *buf,
				  size_t count, struct read_info *rd);
/*
 * 发送IPC消息给contexthub nolock
 */
int send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type, unsigned int subtype, const char *buf,
				size_t count);
#endif
