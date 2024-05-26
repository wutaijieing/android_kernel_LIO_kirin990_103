/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub ipc link shm.
 * Create: 2021-10-14
 */
#ifndef __IOMCU_IPC_LINK_SHM_H__
#define __IOMCU_IPC_LINK_SHM_H__

#include <linux/types.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t ipc_shm_alloc(size_t size, unsigned int context, unsigned int *offset);
void ipc_shm_free(uintptr_t addr);

unsigned int ipc_shm_get_capacity(void);
void *ipc_shm_get_recv_buf(const unsigned int offset);

#ifdef __cplusplus
}
#endif
#endif
