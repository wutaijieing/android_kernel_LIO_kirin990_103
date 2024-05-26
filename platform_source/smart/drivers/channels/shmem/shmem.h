/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: Contexthub share memory driver
 * Create: 2016-04-01
 */
#ifndef __CONTEXTHUB_SHMEM_H__
#define __CONTEXTHUB_SHMEM_H__

#ifdef CONFIG_INPUTHUB_30
#include <platform_include/smart/linux/iomcu_shmem.h>
#else

#include <platform_include/smart/linux/base/ap/protocol.h>

#include <iomcu_ddr_map.h>

#endif
/*
 * sharemem消息发送接口，将数据复制到DDR中，并发送IPC通知contexthub处理
 */
extern int shmem_send(enum obj_tag module_id, const void *usr_buf, unsigned int usr_buf_size);

/*
 * Contexthub sharemem驱动初始化
 */
extern int contexthub_shmem_init(void);

/*
 * 将sharemem包中的数据从DDR中取出，如果是大数据包消息同时启动workqueue回复IPC确认
 */
extern const struct pkt_header *shmempack(const char *buf, unsigned int length);

/*
 * 获得sharemem数据发送的size上限
 */
extern unsigned int shmem_get_capacity(void);

/*
 * sharemem消息发送后，收到contexthub的回复确认后up信号量
 */
extern int shmem_send_resp(const struct pkt_header *head);

#endif
