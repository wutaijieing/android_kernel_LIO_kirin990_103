/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Contexthub share memory driver
 * Create: 2019-10-01
 */
#ifndef __IOMCU_SHMEM_H__
#define __IOMCU_SHMEM_H__

#include <linux/types.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <iomcu_ddr_map.h>

#ifdef __cplusplus
extern "C" {
#endif

int contexthub_shmem_init(void);
void iomcu_shm_exit(void);
unsigned int shmem_get_capacity(void);
int ipcshm_send(struct write_info *wr, struct pkt_header *pkt, struct read_info *rd);
void *ipcshm_get_data(const char *buf);

#ifdef __cplusplus
}
#endif
#endif
