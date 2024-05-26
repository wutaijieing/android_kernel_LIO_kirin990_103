/**
 * @file dkmd_acquire_fence.h
 * @brief To provide an interface timing synchronization function
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DKMD_ACQUIRE_FENCE_H
#define DKMD_ACQUIRE_FENCE_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/dma-fence.h>

#define ACQUIRE_FENCE_TIMEOUT_MSEC (4000) // 4s

/**
 * @brief Get a file descriptor with given fence on given value.
 *
 * @fence: [in] the fence to initialize
 * @lock:  [in] the irqsafe spinlock to use for locking this fence
 * @return: 0 for success, otherwise, fail
 */
int32_t dkmd_acquire_fence_create_fd(struct dma_fence *fence, spinlock_t *lock, int32_t value);

/**
 * @brief create and init a fence file descriptor
 *
 * @param lock the irqsafe spinlock to use for locking this fence
 * @param value same as scene id
 * @return struct dma_fence* fence
 */
struct dma_fence *dkmd_alloc_acquire_fence(spinlock_t *lock, int32_t value);

/**
 * @brief sleep until the fence is signaled or timeout.
 *
 * @fence: fence struct
 * @timeout:  [in] expire time for fence wait in ms, or MAX_SCHEDULE_TIMEOUT
 * @return: 0 for success, otherwise, fail
 */
int32_t dkmd_acquire_fence_wait(struct dma_fence *fence, long timeout);

/**
 * @brief sleep until the fence is signaled or timeout.
 *
 * @fence_fd: [in] the fence fd to wait
 * @timeout:  [in] expire time for fence wait in ms, or MAX_SCHEDULE_TIMEOUT
 * @return: 0 for success, otherwise, fail
 */
int32_t dkmd_acquire_fence_wait_fd(int32_t fence_fd, long timeout);

/**
 * @brief signal and release fence by struction
 *
 * @fence: fence struct
 */
void dkmd_acquire_fence_signal_release(struct dma_fence *fence);

/**
 * @brief signal completion of a fence
 *
 * @fence_fd: [in] the fence to signal, fd must be the same with create pid
 * @return:   0 for success, otherwise, fail
 */
int32_t dkmd_acquire_fence_signal(int32_t fence_fd);

#endif
