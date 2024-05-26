/*
 * vfmw.h
 *
 * This is for vfmw.
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

#ifndef VFMW_H
#define VFMW_H

#include "vfmw_ext.h"

#ifdef VCODEC_TVP_SUPPORT
#define  TVP_CHAN_NUM 2
#else
#define  TVP_CHAN_NUM 0
#endif

#ifdef CFG_MAX_CHAN_NUM
#define VFMW_CHAN_NUM CFG_MAX_CHAN_NUM
#else
#define VFMW_CHAN_NUM 32
#endif

#define VDEC_OK 0
#define VDEC_ERR (-1)

#define uint64_ptr(ptr) ((void *)(uintptr_t)(ptr))
#define ptr_uint64(ptr) ((uint64_t)(uintptr_t)(ptr))

typedef struct {
	UADDR phy_addr;
	UADDR mmu_addr;
	uint64_t vir_addr;
	uint32_t length;
	mem_mode mode;
	uint8_t asz_name[16];
	vcodec_bool tvp;
	uint32_t h_handle;
} vfmw_mem;

typedef struct {
	uint64_t fd;
	uint8_t is_cached;
	uint8_t *vir_addr;
	UADDR phy_addr;
	UADDR mmu_addr;
	uint32_t length;
	mem_mode mode;
	vcodec_handle vdec_handle;
	vcodec_handle ssm_handle;
	uint32_t protect_id;
	uint32_t buff_id;
	vcodec_bool is_map_iova;
} mem_record;

typedef enum {
	NO_SEC_MODE,
	SEC_MODE
} sec_mode;

#endif

