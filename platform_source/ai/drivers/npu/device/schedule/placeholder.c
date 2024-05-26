/*
 * placeholder.c
 *
 * about placeholder
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
#include "placeholder.h"

struct placeholder_manager g_pholder_mngr;

int pholder_mngr_init(void)
{
	int i;

	INIT_LIST_HEAD(&g_pholder_mngr.idle_list);

	for (i = 0; i < NPU_MAX_MODEL_ID; i++)
		list_add_tail(&g_pholder_mngr.buffer[i].node, &g_pholder_mngr.idle_list);

	return 0;
}

struct placeholder *pholder_mngr_alloc_node(void)
{
	struct placeholder *pholder = NULL;

	pholder = list_first_entry_or_null(&g_pholder_mngr.idle_list, struct placeholder, node);

	if (pholder != NULL)
		list_del(&pholder->node);

	return pholder;
}

void pholder_mngr_free_node(struct placeholder *pholder)
{
	list_move_tail(&pholder->node, &g_pholder_mngr.idle_list);
}
