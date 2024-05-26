/*
 * tvp_adapter.h
 *
 * This is vcodec utils.
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

#ifndef TVP_ADAPTER_H
#define TVP_ADAPTER_H

#include "vcodec_type.h"
#include "drv_mem.h"

enum {
	VCODEC_CMD_ID_INIT = 1,
	VCODEC_CMD_ID_EXIT,
	VCODEC_CMD_ID_SUSPEND,
	VCODEC_CMD_ID_RESUME,
	VCODEC_CMD_ID_CONTROL,
	VCODEC_CMD_ID_RUN_PROCESS,
	VCODEC_CMD_ID_GET_IMAGE,
	VCODEC_CMD_ID_RELEASE_IMAGE,
	VCODEC_CMD_ID_CONFIG_INPUT_BUFFER,
	VCODEC_CMD_ID_READ_PROC,
	VCODEC_CMD_ID_WRITE_PROC,
	VCODEC_CMD_ID_MEM_CPY = 20,
	VCODEC_CMD_ID_CFG_MASTER,
};

enum sec_venc_state {
	SEC_VENC_OFF,
	SEC_VENC_ON,
};

int32_t init_kernel_ca(void);
void deinit_kernel_ca(void);
void config_master(enum sec_venc_state state, uint32_t core_id);

#endif
