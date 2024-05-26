/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef __CMDLIST_DEV_H__
#define __CMDLIST_DEV_H__

#include <soc_dte_define.h>
#include "cmdlist.h"

struct cmdlist_dm_addr_info {
	enum SCENE_ID scene_id;
	u_int32_t dm_data_addr;
	u_int32_t dm_data_size;
};

struct cmdlist_client;

void cmdlist_config_start(struct cmdlist_client *client);

#endif