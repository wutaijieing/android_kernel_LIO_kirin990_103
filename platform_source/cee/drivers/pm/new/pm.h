/*
 * pm.h
 *
 * define public funcs used in pm module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#ifndef __H_LOW_PM_H__
#define __H_LOW_PM_H__

#include <linux/plist.h>

enum sr_mntn_prio {
	SR_MNTN_PRIO_C = 0,
	SR_MNTN_PRIO_H = 10,
	SR_MNTN_PRIO_M = 50,
	SR_MNTN_PRIO_L = 100,
};

struct sr_mntn {
	struct plist_node node;
	const char *owner;
	bool enable;
	int (*suspend)(void);
	int (*resume)(void);
};

#ifdef CONFIG_SR
int register_sr_mntn(struct sr_mntn *ops, enum sr_mntn_prio prio);
void unregister_sr_mntn(struct sr_mntn *ops);
#else
static inline int register_sr_mntn(struct sr_mntn *ops, enum sr_mntn_prio prio) { return 0; }
static inline void unregister_sr_mntn(struct sr_mntn *ops) {}
#endif

#endif /* __H_LOW_PM_H__ */
