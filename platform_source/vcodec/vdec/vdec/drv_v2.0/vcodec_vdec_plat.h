/*
 * vcodec_vdec_plat.h
 *
 * This is for vdec platform
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef VCODEC_VDEC_PLAT_H
#define VCODEC_VDEC_PLAT_H
#include "dbg.h"

#define vdec_check_ret(cond, ret) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "assert condition %s not match\n", #cond); \
			return ret; \
		} \
	} while (0)

#define vdec_init_mutex(lock) \
	do { \
		mutex_init(lock); \
	} while (0)

#define vdec_mutex_lock(lock) \
	do { \
		mutex_lock(lock); \
	} while (0)

#define vdec_mutex_lock_interruptible(lock) \
	({int mutex_lock_ret; \
		do { \
			mutex_lock_ret = mutex_lock_interruptible(lock); \
		} while (0); \
	mutex_lock_ret;})

#define vdec_mutex_unlock(lock) \
	do { \
		mutex_unlock(lock); \
	} while (0)

typedef struct {
	bool (*vcodec_vdec_device_bypass)(void);
	bool (*smmu_need_cfg_max_tok_trans)(void);
} vdec_plat_ops;

int32_t vdec_device_probe(void *plt);

int32_t get_vdec_peri_state(uint32_t *state_info);

vdec_plat_ops *get_vdec_plat_ops(void);
#endif
