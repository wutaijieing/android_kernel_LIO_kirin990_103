/*
 * ipc_wake.h
 *
 * wakeup ipc
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef __H_PM_MNTN_IPC_WAKE_H__
#define __H_PM_MNTN_IPC_WAKE_H__

#include <linux/seq_file.h>

int init_ipc_table(const struct device_node *lp_dn);
void show_wake_ipc(struct seq_file *s);

#ifdef CONFIG_SR_DEBUG
void wake_ipc_debug(int ipc_state, int mbx, int src, int dst);
#else
void wake_ipc_debug(int ipc_state, int mbx, int src, int dst) {}
#endif

#endif /* __H_PM_MNTN_IPC_WAKE_H__ */
